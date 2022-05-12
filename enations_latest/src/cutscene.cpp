//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// scenario.cpp : implementation file
//

#include "stdafx.h"
#include "cutscene.h"
#include "player.h"
#include "lastplnt.h"
#include "bitmaps.h"
#include "sfx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


CCutScene theCutScene;


/////////////////////////////////////////////////////////////////////////////
// CCutScene

CCutScene::CCutScene ()
{
}

CCutScene::~CCutScene ()
{
}

UINT CCutScene::PlayCutScene (int iScenario, BOOL bRepeat, BOOL bAsync)
{

	return (_PlayScene (bRepeat ? CWndCutScene::repeat : CWndCutScene::cut, iScenario, bAsync) );
}

void CCutScene::PlayEnd (int iEnd, BOOL bAsync)
{

	_PlayScene (iEnd, FALSE, bAsync);
}

UINT CCutScene::_PlayScene (int iTyp, int iScenario, BOOL bAsync)
{

	// disable the main window (and all children)
	theApp.m_wndBar.ShowWindow (SW_HIDE);

	theApp.m_wndCutScene.Create ( iTyp, iScenario );
	theApp.m_wndCutScene.ShowWindow (SW_SHOW);
	theApp.m_wndCutScene.UpdateWindow ();

	// kill what was playing
	theMusicPlayer.SoundsOff ();

	// start the music/voice
	switch (iTyp)
	  {
		case CWndCutScene::cut :
		case CWndCutScene::repeat :
			theMusicPlayer.PlayForegroundSound (VOICES::GetID (VOICES::cut_first + iScenario, 0), SFXPRIORITY::voice_pri );
			break;
		case CWndCutScene::win :
		case CWndCutScene::scenario_end :
			theMusicPlayer.PlayExclusiveMusic (MUSIC::GetID (MUSIC::win_game));
			break;
		case CWndCutScene::lose :
			theMusicPlayer.PlayExclusiveMusic (MUSIC::GetID (MUSIC::loose_game));
			break;
	  }

	// loop till done
	if ( ! bAsync )
		{
		while ( (theApp.m_wndCutScene.m_hWnd != NULL) &&
														(theApp.m_wndCutScene.GetRtn () == 0) )
			theApp.BaseYield ();

		theMusicPlayer.SoundsOff ();
		theMusicPlayer.UnloadGroup ( SFXGROUP::cut_scenes );

		if ( (iTyp == CWndCutScene::cut) || (iTyp == CWndCutScene::repeat) )
			{
			theMusicPlayer.StartMidiMusic ();
			theMusicPlayer.PlayMusicGroup (MUSIC::GetID (MUSIC::play_game), MUSIC::num_play_game);
			}
		}

	// bring it back
	if ( ( ! bAsync ) && ( (iTyp == CWndCutScene::cut) || (iTyp == CWndCutScene::repeat) ) )
		theApp.m_wndBar.ShowWindow (SW_SHOW);

	return ( theApp.m_wndCutScene.GetRtn () );
}

/////////////////////////////////////////////////////////////////////////////
// CWndCutScene

BEGIN_MESSAGE_MAP(CWndCutScene, CWndBase)
	//{{AFX_MSG_MAP(CWndCutScene)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_CHAR()
	ON_BN_CLICKED (IDC_SCENE_OK, OnSceneOk)
	ON_BN_CLICKED (IDC_SCENE_CANCEL, OnSceneCancel)
	ON_BN_CLICKED (IDC_SCENE_SAVE, OnSceneSave)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CWndCutScene::Create (int iTyp, int iScenario)
{

	m_iTyp = iTyp;
	m_iScenario = iScenario;
	m_iRtn = 0;

	if ( m_hWnd == NULL )
		{
		const char *pCls = AfxRegisterWndClass (0, theApp.LoadStandardCursor (IDC_ARROW),
																								NULL, theApp.LoadIcon (IDI_MAIN));

		const DWORD dwSty = WS_POPUP;	
		if (CreateEx (0, pCls, theApp.m_sAppName, dwSty, 0, 0, theApp.m_iScrnX, 
												theApp.m_iScrnY, theApp.m_wndMain.m_hWnd, NULL, NULL) == 0)
			ThrowError (ERR_RES_CREATE_WND);
		return;
		}

	// load new stuff
	OnDestroy ();
	OnCreate ( NULL );
}

int CWndCutScene::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

	// get the scene
	CMmio *pMmio = theDataFile.OpenAsMMIO ("misc", "MISC");

	pMmio->DescendRiff ('M', 'I', 'S', 'C');

	// get the proper bitmap
	char	szBPS[ 5 ];
	switch (m_iTyp)
	  {
		case scenario_end :
			strcpy (szBPS, "DN");
			break;
		case win :
			strcpy (szBPS, "WN");
			break;
		case lose :
			strcpy (szBPS, "LS");
			break;
		default:
			strcpy (szBPS, "CS");
			break;
	  }

	strcpy ( &szBPS[2], theApp.m_szOtherBPS );

	try
		{
		pMmio->DescendList  ( szBPS[0], szBPS[1], szBPS[2], szBPS[3] );
		pMmio->DescendChunk ('D', 'A', 'T', 'A');
		m_bTile = FALSE;
		}
	catch (...)
		{
		m_bTile = TRUE;
		delete pMmio;
		CMmio *pMmio = theDataFile.OpenAsMMIO ("misc", "MISC");
		pMmio->DescendRiff ('M', 'I', 'S', 'C');
		pMmio->DescendList ('W', 'L', szBPS[2], szBPS[3]);
		pMmio->DescendChunk ('D', 'A', 'T', 'A');
		}

	m_pdibPicture = new CDIB ( ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY, CBLTFormat::DIR_BOTTOMUP );
	m_pdibPicture->Load( *pMmio );
	delete pMmio;

	switch (m_iTyp)
	  {
		case scenario_end :
			OnCreateOther (IDS_YOU_END_SCENARIO);
			break;
		case win :
			OnCreateOther (IDS_YOU_WON);
			break;
		case lose :
			OnCreateOther (IDS_YOU_LOST);
			break;
		default:
			OnCreateCut ();
			break;
	  }

	InvalidateRect (NULL);
	return 0;
}

void CWndCutScene::OnCreateCut ()
{

	// get the text
	CMmio * pMmio = theDataFile.OpenAsMMIO (NULL, "LANG");
	pMmio->DescendRiff ('L', 'A', 'N', 'G');
	pMmio->DescendList ('S', 'C', 'E', 'N');
	pMmio->DescendChunk ('T', 'X', 'T', (char) ('A' + m_iScenario));

	long lSize = pMmio->ReadLong ();
	pMmio->Read (m_sText.GetBuffer (lSize+2), lSize);
	
	delete pMmio;
	m_sText.ReleaseBuffer (lSize);

	CRect rect;
	GetClientRect (&rect);
	int iHt = (rect.Height () * 3) / 4;
	int iFntHt = ( 28 * rect.Height () ) / 1280;

	// get the font
	CWindowDC dc (this);
	do
		{
		if (m_font.m_hObject != NULL)
			m_font.DeleteObject ();

		LOGFONT lf;
		memset (&lf, 0, sizeof (lf));
		lf.lfHeight = iFntHt;
		strcpy (lf.lfFaceName, "Milford");
		m_font.CreateFontIndirect (&lf);
	
		// will this font fit?
		GetClientRect (&rect);
		rect.left = (rect.right * 2) / 3;
		rect.right = (rect.right * 95) / 100;
		CFont * pOldFont = dc.SelectObject (&m_font);
		dc.DrawText ( m_sText, &rect, DT_CALCRECT | DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
		dc.SelectObject (pOldFont);
		iFntHt --;
		}
	while ( (rect.Height () > iHt) && (iFntHt >= 12) );

	// button location
	GetClientRect (&rect);
	iHt = rect.Height ();
	rect.top = __min (rect.bottom - 40, rect.bottom - iHt / 20);
	rect.OffsetRect (0, - rect.Height () / 4);
	int iWid = rect.Width ();
	rect.left = iWid / 18;
	rect.right = rect.left + iWid / 9;
	int iAdd = (iWid / 9 ) * 2;

	// set up buttons
	m_btnOK.Create (IDS_OK, 0, &theCutTextBtnData, rect, m_pdibPicture, this, IDC_SCENE_OK);
	rect.OffsetRect (iAdd, 0);

	if (m_iTyp != repeat)
		{
		m_btnCancel.Create (IDS_CANCEL, 0, &theCutTextBtnData, rect, m_pdibPicture, this, IDC_SCENE_CANCEL);
		rect.OffsetRect (iAdd, 0);

		m_btnSave.Create (IDS_SAVE, 0, &theCutTextBtnData, rect, m_pdibPicture, this, IDC_SCENE_SAVE);
		}

	// no save on 1st scenario
	if (m_iTyp == cut)
		m_btnSave.EnableWindow (m_iScenario > 0);
}

void CWndCutScene::OnCreateOther (int idRes)
{

	// get the text
	m_sText.LoadString (idRes);

	CRect rect;
	GetClientRect (&rect);
	int iWid = (rect.Width () * 2) / 3;
	int iFntHt = 2 * rect.Width () / m_sText.GetLength ();

	// get the font
	CWindowDC dc (this);
	do
		{
		if (m_font.m_hObject != NULL)
			m_font.DeleteObject ();

		LOGFONT lf;
		memset (&lf, 0, sizeof (lf));
		lf.lfHeight = iFntHt;
		strcpy (lf.lfFaceName, "Milford");
		m_font.CreateFontIndirect (&lf);
	
		// will this font fit?
		GetClientRect (&rect);
		CFont * pOldFont = dc.SelectObject (&m_font);
		dc.DrawText ( m_sText, &rect, DT_CALCRECT | DT_NOPREFIX | DT_CENTER | DT_TOP | DT_SINGLELINE);
		dc.SelectObject (pOldFont);
		iFntHt --;
		}
	while ( (rect.Width () > iWid) && (iFntHt >= 12) );

	// button location
	GetClientRect (&rect);
	int iHt = rect.Height ();
	rect.top = __min (rect.bottom - 40, rect.bottom - iHt / 16);
	rect.OffsetRect (0, - rect.Height () / 4);
	iWid = rect.Width ();
	rect.left = rect.Width () - (iWid / 9) - rect.Height () / 4;
	rect.right = rect.left + iWid / 9;

	int iRes = ( m_iTyp == scenario_end ) ? IDS_CONTINUE : IDS_CANCEL;
	m_btnOK.Create ( iRes, 0, &theCutTextBtnData, rect, m_pdibPicture, this, IDC_SCENE_OK );
}

void CWndCutScene::OnChar (UINT nChar, UINT, UINT )
{

	if ( (nChar == VK_RETURN)	&& (m_btnOK.IsWindowEnabled ()) )
		OnSceneOk ();
	else
		if ( (nChar == VK_ESCAPE)	&& (m_btnCancel.IsWindowEnabled ()) )
			OnSceneCancel ();
}

void CWndCutScene::OnSceneOk()
{

	m_iRtn = CUT_OK;

	if ( m_iTyp != scenario_end )
		DestroyWindow ();
	else
		{
		m_btnOK.EnableWindow ( FALSE );
		m_btnOK.ShowWindow ( SW_HIDE );
		}
}

void CWndCutScene::OnSceneCancel()
{

	m_iRtn = CUT_CANCEL;

	if ( m_iTyp != scenario_end )
		DestroyWindow ();
	else
		{
		TRAP ();
		m_btnOK.EnableWindow ( FALSE );
		m_btnOK.ShowWindow ( SW_HIDE );
		}
}

void CWndCutScene::OnSceneSave()
{

	ASSERT (this);	
	ASSERT (TestEverything ());

	theMusicPlayer.SoundsOff ();

	// save the game
	theGame.SaveGame (this);
}

void CWndCutScene::OnDestroy()
{

	delete m_pdibPicture;
	m_pdibPicture = NULL;
}

BOOL CWndCutScene::OnEraseBkgnd(CDC *) 
{
	
	return (1);
}

void CWndCutScene::OnPaint()
{

	CPaintDC dc(this); // device context for painting

	thePal.Paint (dc.m_hDC);

	// the picture
	CRect rect;
	GetClientRect (&rect);
	if ( m_bTile )
		m_pdibPicture->Tile ( dc, rect );
	else
		m_pdibPicture->StretchBlt ( dc, rect, m_pdibPicture->GetRect () );

	switch (m_iTyp)
	  {
		case scenario_end :
		case win :
			OnPaintWin (dc);
			break;
		case lose :
			OnPaintLose (dc);
			break;
		default:
			OnPaintCut (dc);
			break;
	  }

	thePal.EndPaint (dc.m_hDC);
}

void CWndCutScene::OnPaintCut (CDC & dc)
{

	CRect rect;
	GetClientRect (&rect);

	// set it up
	CFont * pOldFont = dc.SelectObject (&m_font);
	dc.SetBkMode (TRANSPARENT);

	// the text
	rect.left = (rect.right * 2) / 3;
	rect.right = (rect.right * 95) / 100;

	// get height
	int iHt = rect.Height ();
	dc.DrawText ( m_sText, &rect, DT_CALCRECT | DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
	if (rect.Height () < iHt)
		rect.OffsetRect (0, (iHt - rect.Height ()) / 2);

	dc.SetTextColor (PALETTERGB (0, 0, 0));
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
	rect.OffsetRect ( -1, -1 );
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
	rect.OffsetRect ( -1, -1 );
	dc.SetTextColor (PALETTERGB (255, 251, 120));
//BUGBUG	dc.SetTextColor (PALETTERGB (230, 251, 120));
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);

	dc.SelectObject (pOldFont);
}

void CWndCutScene::OnPaintWin (CDC & dc)
{

	CRect rect;
	GetClientRect (&rect);

	// set it up
	CFont * pOldFont = dc.SelectObject (&m_font);
	dc.SetBkMode (TRANSPARENT);

	// location
	rect.top = rect.Height () / 4;

	dc.SetTextColor (PALETTERGB (78, 90, 98));
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_CENTER | DT_TOP | DT_SINGLELINE);

	rect.OffsetRect (-4, -4);
	dc.SetTextColor (PALETTERGB (255, 244, 221));
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_CENTER | DT_TOP | DT_SINGLELINE);

	dc.SelectObject (pOldFont);
}

void CWndCutScene::OnPaintLose (CDC & dc)
{

	CRect rect;
	GetClientRect (&rect);

	// set it up
	CFont * pOldFont = dc.SelectObject (&m_font);
	dc.SetBkMode (TRANSPARENT);

	// location
	rect.top = rect.Height () / 4;

	dc.SetTextColor (PALETTERGB (148, 100, 70));
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_CENTER | DT_TOP | DT_SINGLELINE);

	rect.OffsetRect (-4, -4);
	dc.SetTextColor (PALETTERGB (57, 0, 0));
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_CENTER | DT_TOP | DT_SINGLELINE);

	dc.SelectObject (pOldFont);
}

