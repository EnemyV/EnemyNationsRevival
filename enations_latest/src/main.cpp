//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// main.cpp : implementation file
//

#include "stdafx.h"

#include <dib.h>

#include "lastplnt.h"
#include "player.h"
#include "relation.h"
#include "ipccomm.h"
#include "research.h"
#include "error.h"
#include "event.h"
#include "cutscene.h"
#include "icons.h"
#include "sfx.h"
#include "area.h"
#include "bitmaps.h"
#include "plyrlist.h"
#include "cdloc.h"
#include "toolbar.h"
#include "msgs.h"
#include "chat.h"

#include "ui.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


CBmBtnData theBmBtnData;
CTextBtnData theTextBtnData, theLargeTextBtnData, theCutTextBtnData;
CIcons theIcons;
CBitmapLib theBitmaps;



/////////////////////////////////////////////////////////////////////////////
// CWndMain

void CWndMain::Create ()
{

	m_bPauseOnActive = TRUE;
	m_progPos = loading;

	const DWORD dwExSty = WS_EX_APPWINDOW;
	const DWORD dwSty = WS_POPUP;
	if (CreateEx (dwExSty, theApp.m_sClsName, theApp.m_sAppName, dwSty, 0, 0, GetSystemMetrics (SM_CXSCREEN), 
												GetSystemMetrics (SM_CYSCREEN), NULL, NULL, NULL) == 0)
		ThrowError (ERR_RES_CREATE_WND);
}


BEGIN_MESSAGE_MAP(CWndMain, CWndBase)
	//{{AFX_MSG_MAP(CWndMain)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_MESSAGE (WM_MY_DISPLAYCHANGE, OnMyDisplayChange)
	ON_MESSAGE (WM_DISPLAYCHANGE, OnDisplayChange)
	ON_MESSAGE (WM_VPNOTIFY, OnNetMsg)
	ON_MESSAGE (WM_VPFLOWON, OnNetFlowOn)
	ON_MESSAGE (WM_VPFLOWOFF, OnNetFlowOff)
	ON_MESSAGE (WM_ACTIVATE_MUSIC, OnActivateMusicMsg)
	ON_WM_ACTIVATEAPP()
	ON_COMMAND(IDA_HIDE_TOOLBAR, OnHide)
	ON_COMMAND(IDA_UNHIDE_TOOLBAR, OnUnHide)
	ON_COMMAND(IDA_SAVE, OnSave)
	ON_COMMAND(IDA_AREA, OnArea)
	ON_COMMAND(IDA_BOSS, OnBoss)
	ON_COMMAND(IDA_HELP, OnHelp)
	ON_COMMAND(IDA_MAIL, OnMail)
	ON_COMMAND(IDA_OPTIONS, OnOptions)
	ON_COMMAND(IDA_WORLD, OnWorld)
	ON_COMMAND(IDA_RESEARCH, OnResearch)
	ON_COMMAND(IDA_DIPLOMAT, OnDiplomat)
	ON_COMMAND(IDA_BUILDINGS, OnBuildings)
	ON_COMMAND(IDA_VEHICLES, OnVehicles)
	ON_COMMAND(IDA_NEXT, OnNext)
	ON_COMMAND(IDA_PREV, OnPrev)
	ON_COMMAND(IDA_PAUSE, OnPause)
	ON_WM_QUERYENDSESSION()
	ON_COMMAND(IDA_CLOSE_APP, OnCloseApp)
	ON_WM_CLOSE()
	ON_WM_SYSCOMMAND()
	ON_WM_TIMER()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWndMain message handlers


int CWndMain::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

	if (CWndBase::OnCreate(lpCreateStruct) == -1)
		return -1;

	SendMessage (WM_SETICON, (WPARAM)TRUE, (LPARAM) theApp.LoadIcon (MAKEINTRESOURCE (IDI_MAIN)));

	return 0;
}

void CWndMain::LoadData ()
{

	Ptr< CColorFormat	>	ptrcolorformat;

	if ( theApp.Use8Bit() )
		ptrcolorformat = new CColorFormat( CColorFormat::DEPTH_EIGHT );
	else
		ptrcolorformat = new CColorFormat;
	char sBuf [3];
	if ( 8 == ptrcolorformat->GetBitsPerPixel ())
		strcpy ( sBuf, "08" );
	else
		strcpy ( sBuf, "24" );

	// load the wallpaper bitmap
	CMmio *pMmio = theDataFile.OpenAsMMIO ("misc", "MISC");

	pMmio->DescendRiff ('M', 'I', 'S', 'C');
	pMmio->DescendList ('W', 'L', sBuf[0], sBuf[1]);
	pMmio->DescendChunk ('D', 'A', 'T', 'A');

	m_pcdibWall = new CDIB ( *ptrcolorformat, CBLTFormat::DIB_MEMORY, CBLTFormat::DIR_BOTTOMUP );

	m_pcdibWall->Load( *pMmio );

	delete pMmio;
}

#ifdef BUGBUG
LRESULT CWndMain::OnCacheMsg (WPARAM wParam, LPARAM )
{

	theDiskCache.ProcessMessage ( (CCacheElem *) wParam );

	return (0);
}
#endif

static void MakeFullScreen ( CWnd * pWnd )
{

	if ( (pWnd != NULL) && (pWnd->m_hWnd != NULL) )
		{
		pWnd->SetWindowPos (NULL, 0, 0, theApp.m_iScrnX, theApp.m_iScrnY, SWP_NOZORDER);
		pWnd->InvalidateRect ( NULL );
		}
}

static void MoveToNew ( CWnd * pWnd, int xOld, int yOld )
{

	if ( (pWnd == NULL) || (pWnd->m_hWnd == NULL) )
		return;

	CRect rect;
	if ( ! theApp.m_wndMain.IsIconic () )
		pWnd->GetWindowRect ( &rect );
	else
		{
		WINDOWPLACEMENT wp;
		pWnd->GetWindowPlacement ( &wp );
		rect = wp.rcNormalPosition;
		}

	int x = (rect.left * theApp.m_iScrnX) / xOld;
	int y = (rect.top * theApp.m_iScrnY) / yOld;
	int cx = (rect.Width () * theApp.m_iScrnX) / xOld;
	int cy = (rect.Height () * theApp.m_iScrnY) / yOld;
	x = __minmax ( 0, theApp.m_iScrnX - cx , x );
	y = __minmax ( 0, theApp.m_iScrnY - cy , y );
	pWnd->SetWindowPos (NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	pWnd->InvalidateRect ( NULL );
}

static void MoveSizeToNew ( CWnd * pWnd, int xOld, int yOld )
{

	if ( (pWnd == NULL) || (pWnd->m_hWnd == NULL) )
		return;

	CRect rect;
	if ( ! theApp.m_wndMain.IsIconic () )
		pWnd->GetWindowRect ( &rect );
	else
		{
		WINDOWPLACEMENT wp;
		pWnd->GetWindowPlacement ( &wp );
		rect = wp.rcNormalPosition;
		}

	int x = (rect.left * theApp.m_iScrnX) / xOld;
	int y = (rect.top * theApp.m_iScrnY) / yOld;
	int cx = (rect.Width () * theApp.m_iScrnX) / xOld;
	int cy = (rect.Height () * theApp.m_iScrnY) / yOld;
	x = __minmax ( 0, theApp.m_iScrnX - cx , x );
	y = __minmax ( 0, theApp.m_iScrnY - cy , y );
	if ( ( cx > 96 ) && ( cy > 48 ) )
		pWnd->SetWindowPos (NULL, x, y, cx, cy, SWP_NOZORDER);
	else
		pWnd->SetWindowPos (NULL, x, y, cx, cy, SWP_NOZORDER | SWP_NOSIZE);
	pWnd->InvalidateRect ( NULL );
}

// we want to make sure Windows is all set when we do this
// so we post a new message
LRESULT CWndMain::OnDisplayChange (WPARAM wParam, LPARAM lParam)
{

	PostMessage ( WM_MY_DISPLAYCHANGE, wParam, lParam );
	return (0);
}

// we want to make sure Windows is all set when we do this
LRESULT CWndMain::OnMyDisplayChange (WPARAM wParam, LPARAM lParam)
{

	// now set a timer because it's all screwed up
	if ( SetTimer (109, 1000, NULL) == 0 )
		OnDisplayChange2 ();

	return (0);
}

void CWndMain::OnDisplayChange2 ()
{

	// save the old size
	int xOld = theApp.m_iScrnX;
	int yOld = theApp.m_iScrnY;

	// get the size
	theApp.m_iScrnX = GetSystemMetrics (SM_CXSCREEN);
	theApp.m_iScrnY = GetSystemMetrics (SM_CYSCREEN);

	// any change?
	if ( (xOld == theApp.m_iScrnX) || (yOld == theApp.m_iScrnY) )
		return;

	// these windows are all full screen
	MakeFullScreen ( this );
	MakeFullScreen ( theApp.m_pdlgMain );
	MakeFullScreen ( &theApp.m_wndMovie );
	MakeFullScreen ( &theApp.m_wndCredits );
	MakeFullScreen ( &theApp.m_wndCutScene );

	// these are dialogs - just move, don't size
	MoveToNew ( theApp.m_pdlgRelations, xOld, yOld );
	MoveToNew ( theApp.m_pdlgFile, xOld, yOld );
	MoveToNew ( theApp.m_pdlgRsrch, xOld, yOld );
	MoveToNew ( theApp.GetDlgPause (), xOld, yOld );
	MoveToNew ( theApp.m_pdlgPlyrList, xOld, yOld );

	// these move & size
	MoveSizeToNew ( &theApp.m_wndWorld, xOld, yOld );
	MoveSizeToNew ( &theApp.m_wndChat, xOld, yOld );
	MoveSizeToNew ( &theApp.m_wndBldgs, xOld, yOld );
	MoveSizeToNew ( &theApp.m_wndVehicles, xOld, yOld );

	// and all area maps
	theAreaList.MoveSizeToNew ( xOld, yOld );

	// repaint everything
	::InvalidateRect (NULL, NULL, TRUE);
	return;
}

void CWndMain::OnSize(UINT nType, int cx, int cy)
{

	CWndBase::OnSize ( nType, cx, cy );

	// we need to put the toolbar at the bottom - if it exists
	if ( theApp.m_wndBar.m_hWnd == NULL )
		return;

	theApp.m_wndBar.SetWindowPos (NULL, 0, cy - TOOLBAR_HT, cx, TOOLBAR_HT, SWP_NOZORDER);
}

// change the mode to display
void CWndMain::SetProgPos ( PROG_POS ppMode )
{

	// invalidate if changed
	if ( ppMode != m_progPos )
		::InvalidateRect (NULL, NULL, TRUE);

	m_progPos = ppMode;

	// turn off license
	if ( (m_progPos == demo_license) || (m_progPos == retail_license) )
		{
//BUGBUG		EndLicense ();
		SetProgPos ( CWndMain::playing );
		return;
		}

	if ( (m_progPos != demo_license) && (m_progPos != retail_license) )
		{
		m_sText.Empty ();
		if (m_fnt.m_hObject != NULL)
			m_fnt.DeleteObject ();
		return;
		}
		
	CMmio *pMmio = theDataFile.OpenAsMMIO (NULL, "LANG");
	pMmio->DescendRiff ('L', 'A', 'N', 'G');
	pMmio->DescendList ('L', 'E', 'G', 'L');
	pMmio->DescendChunk ('L', 'I', 'C', m_progPos == demo_license ? '2' : '3');

	long lSize = pMmio->ReadLong ();
	pMmio->Read (m_sText.GetBuffer (lSize+2), lSize);
	delete pMmio;
	m_sText.ReleaseBuffer (lSize);

	// get the font
	CString sFont = theApp.GetProfileString ("StatusBar", "Font", "Newtown Italic");
	LOGFONT lf;
	int iFntHt = 36;
	CRect rect;
	GetClientRect (&rect);
	int iWinHt = rect.Height () / 2 + rect.Height () / 4;
	CWindowDC dc ( this );

	do
		{
		// kill old one
		if (m_fnt.m_hObject != NULL)
			m_fnt.DeleteObject ();

		memset (&lf, 0, sizeof (lf));
		lf.lfHeight = iFntHt;
		strncpy (lf.lfFaceName, sFont, LF_FACESIZE-1);
		m_fnt.CreateFontIndirect (&lf);

		// size it
		CFont * pOld = dc.SelectObject ( &m_fnt );
		GetClientRect (&rect);
		int iDif = rect.Width () / 4;
		rect.left += iDif;
		rect.right -= iDif;
		dc.DrawText ( m_sText, &rect, DT_CALCRECT | DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
		dc.SelectObject ( pOld );

		iFntHt --;
		}
	while ( (rect.Height () > iWinHt) && (iFntHt > 10) );

	// set a timer
	m_bLicTimer = TRUE;
	SetTimer (99, 20000, NULL);
}

void CWndMain::OnPaletteChanged(CWnd* pFocusWnd)
{

	TRAP ();
	if ( (m_progPos == loading) || (m_progPos == movie) || (m_progPos == exiting) )
		{
		// do NOT call CWndBase::
		CWnd::OnPaletteChanged (pFocusWnd);
		return;
		}

	// call CWndBase - we want a palette
	CWndBase::OnPaletteChanged (pFocusWnd);
}

BOOL CWndMain::OnQueryNewPalette()
{

	TRAP ();
	if ( (m_progPos == loading) || (m_progPos == movie) || (m_progPos == exiting) )
		{
		// do NOT call CWndBase::
		return ( CWnd::OnQueryNewPalette () );
		}

	// call CWndBase - we want a palette
	return ( CWndBase::OnQueryNewPalette () );
}

BOOL CWndMain::OnEraseBkgnd(CDC *) 
{
	return TRUE;
}

void CWndMain::OnPaint()
{
#ifdef _CHEAT
	CString sVer ("Version: " VER_STRING);
#ifdef _DEBUG
	sVer += " (debug, cheat)";
#else
	sVer += " (cheat)";
#endif
#endif

	CRect rect;
	GetClientRect (&rect);

	// draw black so no palette uglyness
	if ( (m_progPos == loading) || (m_progPos == movie) || (m_progPos == exiting) )
		{
		CPaintDC dc(this); // device context for painting

		CBrush brBlack;
		brBlack.CreateSolidBrush ( RGB (0, 0, 0) );
		dc.FillRect (&rect, &brBlack);

#ifdef _CHEAT
		TEXTMETRIC tm;
		dc.GetTextMetrics (&tm);
		dc.SetBkMode (TRANSPARENT);
		dc.TextOut (0, rect.bottom - tm.tmHeight, sVer);
#endif

		// no text on a movie
		if ( m_progPos == movie )
			return;

		LOGFONT lf;
		memset (&lf, 0, sizeof (lf));
		lf.lfHeight = 48;
		CFont fnt;
		fnt.CreateFontIndirect (&lf);
		CFont * pOldFont = dc.SelectObject (&fnt);

		dc.SetBkMode (TRANSPARENT);
		dc.SetTextColor ( RGB (255, 255, 255) );
		CString sLoad;
		if (m_progPos == exiting)
			sLoad.LoadString (IDS_LEAVING);
		else
			sLoad.LoadString (IDS_LOADING);
		dc.TextOut (0, 0, sLoad);
		dc.SelectObject (pOldFont);
		return;
		}

	CPaintDC dc(this); // device context for painting

	thePal.Paint (dc.m_hDC);
	dc.SetBkMode (TRANSPARENT);

	m_pcdibWall->Tile (dc, rect);

#ifdef _CHEAT
	TEXTMETRIC tm;
	dc.GetTextMetrics (&tm);
	dc.TextOut (0, rect.bottom - tm.tmHeight, sVer);
#endif

	if (m_progPos == game_end)
		{
		LOGFONT lf;
		memset (&lf, 0, sizeof (lf));
		lf.lfHeight = 48;
		CFont fnt;
		fnt.CreateFontIndirect (&lf);
		CFont * pOldFont = dc.SelectObject (&fnt);

		dc.SetTextColor ( RGB (255, 255, 255) );
		CString sLoad;
		sLoad.LoadString (IDS_EXIT_GAME);
		dc.TextOut (0, 0, sLoad);
		dc.SelectObject (pOldFont);
		thePal.EndPaint (dc.m_hDC);
		return;
		}

	if ( (m_progPos != demo_license) && (m_progPos != retail_license) )
		{
		thePal.EndPaint (dc.m_hDC);
		return;
		}

	// grab our font, size the text to center it
	CFont * pOldFont = dc.SelectObject ( &m_fnt );
	GetClientRect (&rect);
	int iDif = rect.Width () / 4;
	int iHt = rect.Height ();
	rect.left += iDif;
	rect.right -= iDif;
	dc.DrawText ( m_sText, &rect, DT_CALCRECT | DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
	rect.top = (iHt - rect.Height ()) / 2;
	rect.bottom = iHt;

	// draw dark bevel
	rect.OffsetRect (- 1, - 1 );
	dc.SetTextColor (PALETTERGB (9, 11, 20));
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
	rect.OffsetRect ( 1, 0 );
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);

	// draw light bevel
	rect.OffsetRect ( 0, 2 );
	dc.SetTextColor (PALETTERGB (76, 81, 118));
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
	rect.OffsetRect ( 1, 0 );
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);

	// draw face
	rect.OffsetRect (- 1, - 1);
	dc.SetTextColor (PALETTERGB (152, 162, 236));
	dc.DrawText ( m_sText, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);

	dc.SelectObject (pOldFont);
	thePal.EndPaint (dc.m_hDC);
}

const int NUM_SYS_COLORS = 28;
COLORREF aiDefSysClrs [NUM_SYS_COLORS];
BOOL bHaveDefSysClrs = FALSE;

int aiSysClrInd [NUM_SYS_COLORS] = {
			COLOR_3DDKSHADOW,
			COLOR_3DFACE,
			COLOR_3DHILIGHT,
			COLOR_3DLIGHT,
			COLOR_3DSHADOW,
			COLOR_ACTIVEBORDER,
			COLOR_ACTIVECAPTION,
			COLOR_APPWORKSPACE,
			COLOR_BACKGROUND,
			COLOR_BTNFACE,
			COLOR_BTNHILIGHT,
			COLOR_BTNSHADOW,
			COLOR_BTNTEXT,
			COLOR_CAPTIONTEXT,
			COLOR_GRAYTEXT,
			COLOR_HIGHLIGHT,
			COLOR_HIGHLIGHTTEXT,
			COLOR_INACTIVEBORDER,
			COLOR_INACTIVECAPTION,
			COLOR_INACTIVECAPTIONTEXT,
			COLOR_INFOBK,
			COLOR_INFOTEXT,
			COLOR_MENU,
			COLOR_MENUTEXT,
			COLOR_SCROLLBAR,
			COLOR_WINDOW,
			COLOR_WINDOWFRAME,
			COLOR_WINDOWTEXT
			};

COLORREF aiENSysClrs [NUM_SYS_COLORS] = {
			RGB (59, 48, 25),
			RGB (237, 191, 97),
			RGB (251, 239, 216),
			RGB (246, 223, 176),
			RGB (118, 96, 49),
			RGB (237, 191, 97),
			RGB (203, 135, 52),
			RGB (44, 63, 84),
			RGB (44, 63, 84),
			RGB (237, 191, 97),
			RGB (251, 239, 216),
			RGB (118, 96, 49),
			RGB (0, 0, 0),
			RGB (255, 255, 255),
			RGB (203, 135, 52),
			RGB (203, 135, 52),
			RGB (255, 255, 255),
			RGB (237, 191, 97),
			RGB (79, 56, 9),
			RGB (203, 135, 52),
			RGB (203, 135, 52),
			RGB (255, 255, 255),
			RGB (237, 191, 97),
			RGB (0, 0, 0),
			RGB (203, 135, 52),
			RGB (255, 255, 255),
			RGB (0, 0, 0),
			RGB (0, 0, 0)
};

COLORREF GetOurSysClr (int iInd)
{

	for (int iOn=0; iOn<NUM_SYS_COLORS; iOn++)
		if (aiSysClrInd [iOn] == iInd)
			return (aiENSysClrs [iOn]);

	ASSERT (FALSE);
	return (RGB (0, 0, 0));
}

void CWndMain::OnDestroy()
{

	m_progPos = exiting;

	// close down the net
	theGame.Close ();
	theNet.Close ( FALSE );

	delete m_pcdibWall;
	m_pcdibWall = NULL;

	// restore the default colors
	if ((theApp.m_bSetSysColors) && (bHaveDefSysClrs))
		::SetSysColors (NUM_SYS_COLORS, aiSysClrInd, aiDefSysClrs);

	thePal.Exit ();
	theApp.m_pMainWnd = NULL;
	theApp.CloseApp ();

	CWndBase::OnDestroy();
}

void CWndMain::OnActivateApp(BOOL bActive, HTASK hTask) 
{

	CWndBase::OnActivateApp (bActive, hTask);
	
	// turn the sound off, lower priority
	if (! bActive)
		{
		if ( theGame.HaveHP () )
			{
			if ( ( theGame.DoOper () ) && theApp.m_bPauseOnAct )
				{
				m_bPauseOnActive = TRUE;
				_OnPause ( TRUE );
				}
			else
				m_bPauseOnActive = FALSE;
			}
		else
			m_bPauseOnActive = FALSE;
//BUGBUG		theApp.SetThreadPriority (THREAD_PRIORITY_HIGHEST);
		theMusicPlayer.OnActivate ( FALSE );
		}

	// restore the default colors
	if ((theApp.m_bSetSysColors) && (! bActive))
		if (bHaveDefSysClrs)
			::SetSysColors (NUM_SYS_COLORS, aiSysClrInd, aiDefSysClrs);

	// Win32s locks up if we do the below code
	if (iWinType != W32s)
		{
		// tell the palette
		CWindowDC dc (this);
		thePal.Activate (m_hWnd, dc.m_hDC, bActive);
		}

	// redraw with new palette
	if (theApp.m_wndWorld.m_hWnd != NULL)
		theApp.m_wndWorld.PaletteChange ();

	// if we are activated we need to make the background window active for a second so its on top
	// of the window we came from
	if (bActive)
		{
		CWnd *pWnd = SetActiveWindow ();
		SetActiveWindow ();
		pWnd->SetActiveWindow ();
		}

	// set it to our system colors
	if ((theApp.m_bSetSysColors) && (bActive))
		{
		// get the default colors
		if (! bHaveDefSysClrs)
			{
			for (int iOn=0; iOn<NUM_SYS_COLORS; iOn++)
				aiDefSysClrs [iOn] = ::GetSysColor (aiSysClrInd [iOn]);
			bHaveDefSysClrs = TRUE;
			}

		// make this one solid (tooltip background)
		CWindowDC dc (this);
		aiENSysClrs [20] = dc.GetNearestColor (aiENSysClrs [20]);

		// set our colors
		::SetSysColors (NUM_SYS_COLORS, aiSysClrInd, aiENSysClrs);
		}

	// turn the sound on
	if (bActive)
		{
//BUGBUG		theApp.SetThreadPriority (THREAD_PRIORITY_NORMAL);
		PostMessage (WM_ACTIVATE_MUSIC, 0, 0);
		if ( ( theGame.HaveHP () ) && ( m_bPauseOnActive ) )
			_OnPause ( FALSE );
		}

	// repaint everything
	::InvalidateRect (NULL, NULL, TRUE);
}

// cause need to wait till we ARE active
LRESULT CWndMain::OnActivateMusicMsg (WPARAM , LPARAM )
{

	if ( ! theMusicPlayer.OnActivate ( TRUE ) )
		PostMessage (WM_ACTIVATE_MUSIC, 0, 0);
	return (0);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgFile dialog


CDlgFile::CDlgFile(CWnd* pParent /*=NULL*/)
	: CDialog(iWinType == W32s ? IDD_FILE1 : IDD_FILE, pParent)
{
	//{{AFX_DATA_INIT(CDlgFile)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgFile::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgFile)
	DDX_Control(pDX, IDC_FILE_SCENE, m_btnMission);
	if ( iWinType == W32s )
		{
		DDX_Control(pDX, IDC_FILE_SPEED, m_scrSpeed);
		DDX_Control(pDX, IDC_FILE_SOUND, m_scrSound);
		DDX_Control(pDX, IDC_FILE_MUSIC, m_scrMusic);
		}
	else
		{
		DDX_Control(pDX, IDC_FILE_MUSIC, m_sldMusic);
		DDX_Control(pDX, IDC_FILE_SOUND, m_sldSound);
		DDX_Control(pDX, IDC_FILE_SPEED, m_sldSpeed);
		}
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgFile, CDialog)
	//{{AFX_MSG_MAP(CDlgFile)
	ON_BN_CLICKED(IDC_FILE_SAVE, OnFileSave)
	ON_BN_CLICKED(IDC_FILE_HELP, OnFileHelp)
	ON_BN_CLICKED(IDC_FILE_EXIT, OnFileExit)
	ON_WM_DESTROY()
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_FILE_VERSION, OnFileVersion)
	ON_BN_CLICKED(IDC_FILE_MINIMIZE, OnFileMinimize)
	ON_WM_ACTIVATE()
	ON_BN_CLICKED(IDC_FILE_SCENE, OnFileScene)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void SaveExistingGame ()
{

	CWndArea * pWnd = theAreaList.GetTop ();
	if (pWnd == NULL)
		return;

	if ((pWnd->GetMode () == CWndArea::rocket_ready) || 
														(pWnd->GetMode () == CWndArea::rocket_pos) ||
														(pWnd->GetMode () == CWndArea::rocket_wait))
		{
		TRAP ();
		return;
		}

	if (AfxMessageBox (IDS_SAVE_OLD, MB_YESNO | MB_ICONQUESTION) == IDYES)
		theGame.SaveGame (NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CDlgFile message handlers

BOOL CDlgFile::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	if ( iWinType == W32s )
		{
		m_scrSpeed.SetScrollRange (0, NUM_SPEEDS - 1);
		m_scrSpeed.SetScrollPos (theApp.GetProfileInt ("Game", "Speed", NUM_SPEEDS/2));
		m_scrSound.SetScrollRange (0, 100);
		m_scrSound.SetScrollPos (theApp.GetProfileInt ("Game", "Sound", 100));
		m_scrMusic.SetScrollRange (0, 100);
		m_scrMusic.SetScrollPos (theApp.GetProfileInt ("Game", "Music", 100));
		}

	else
		{
		m_sldSpeed.SetLineSize (1);
		m_sldSpeed.SetPageSize (1);
		m_sldSpeed.SetRange (0, NUM_SPEEDS - 1);
		m_sldSpeed.SetSelection (0, NUM_SPEEDS - 1);
		m_sldSpeed.SetPos (theGame.GetGameMul ());
		m_sldSpeed.SetTicFreq (2);

		m_sldSound.SetLineSize (1);
		m_sldSound.SetPageSize (1);
		m_sldSound.SetRange (0, 100);
		m_sldSound.SetSelection (0, 100);
		m_sldSound.SetPos (theMusicPlayer.GetSoundVolume ());
		m_sldSound.SetTicFreq (10);
	
		m_sldMusic.SetLineSize (1);
		m_sldMusic.SetPageSize (1);
		m_sldMusic.SetRange (0, 100);
		m_sldMusic.SetSelection (0, 100);
		m_sldMusic.SetPos (theMusicPlayer.GetMusicVolume ());
		m_sldMusic.SetTicFreq (10);
		}
	
	SetState ();

	if ( (theGame.IsNetGame ()) && (theGame.AmServer ()) )
		{
		CString sBtn;
		sBtn.LoadString ( IDS_PLAYERS );
		m_btnMission.SetWindowText ( sBtn );
		}
	else
		m_btnMission.EnableWindow (theGame.GetScenario () >= 0);

	// if rebuilding use old pos
	if ( theGame.m_wpFile.length != 0 )
		SetWindowPlacement ( &(theGame.m_wpFile) );
	else
		{
		CenterWindow ();

		// save position
		theGame.m_wpFile.length = sizeof (WINDOWPLACEMENT);
		GetWindowPlacement ( &(theGame.m_wpFile) );
		}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgFile::OnActivate(UINT nState, CWnd*, BOOL)
{

	if ( theApp.m_wndMain.m_hWnd != NULL )
		theApp.m_wndMain._OnPause ( nState != WA_INACTIVE );

	// do we have a CD?
	if ( nState != WA_INACTIVE )
		CheckForCD ();
}

void CDlgFile::SetSpeed ()
{

	if ( iWinType == W32s )
		m_scrSpeed.SetScrollPos (theGame.GetGameMul ());
	else
		m_sldSpeed.SetPos (theGame.GetGameMul ());
}

void CDlgFile::SetState ()
{

	CWndArea * pWnd = theAreaList.GetTop ();
	if (pWnd == NULL)
		{
		GetDlgItem (IDC_FILE_SAVE)->EnableWindow (TRUE);
		return;
		}

	if ((pWnd->GetMode () == CWndArea::rocket_ready) || 
														(pWnd->GetMode () == CWndArea::rocket_pos) ||
														(pWnd->GetMode () == CWndArea::rocket_wait))
		GetDlgItem (IDC_FILE_SAVE)->EnableWindow (FALSE);
	else
		GetDlgItem (IDC_FILE_SAVE)->EnableWindow (TRUE);
}

void CDlgFile::OnDestroy() 
{

	if ( theApp.m_wndMain.m_hWnd != NULL )
		theApp.m_wndMain._OnPause ( FALSE );

	theApp.m_pdlgFile = NULL;

	CDialog::OnDestroy();
}

void CDlgFile::PostNcDestroy() 
{
	
	CDialog::PostNcDestroy();

	delete this;
}

void CDlgFile::OnFileScene() 
{
	
	if ( theGame.GetScenario () >= 0 )
		{
		theCutScene.PlayCutScene (theGame.GetScenario (), TRUE);
		return;
		}

	// list of players
	theApp.ShowPlayerList ();
}

void CDlgFile::OnFileSave() 
{

	theGame.SaveGame (this);
}

void CDlgFile::OnFileHelp() 
{
	
	theApp.WinHelp (0, HELP_CONTENTS);
}

void CDlgFile::OnFileVersion() 
{
	
	CDlgVer dlgVer (this);
	dlgVer.DoModal ();
}

void CDlgFile::OnFileMinimize() 
{
	
	theApp.Minimize ();
}

void CDlgFile::OnFileExit() 
{

	// switch to server, no HP if necessary
	if ( theGame.IsNetGame () && theGame.AmServer () && theGame.HaveHP () &&
									( theGame.GetAll().GetCount () > theGame.GetAi().GetCount () + 1 ) )
		{
		// make sure
		if (AfxMessageBox (IDS_CLIENT_QUIT, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL) == IDNO)
			return;

		// we're gone
		CPlayer * pPlr = theGame.GetMe ();
		CNetToAi msg ( pPlr );
		theGame.SetHP ( FALSE );
		theGame.AiTakeOverPlayer ( pPlr, TRUE);

		// tell the world
		theGame.PostToAllClients (&msg, sizeof (msg), FALSE);

		// ok, if we went from no, to 1 AI player we need to make ourselves visible again
		if ((theGame.GetNetJoin () == CGame::any) && (theGame.GetAi().GetCount () == 1))
			theNet.SetSessionVisibility (TRUE);

		// put up the game control list
		theApp.ShowPlayerList ();

		// close the game windows
		theApp.CloseDlgChat ();
		theApp.m_wndVehicles.DestroyWindow ();
		theApp.m_wndBldgs.DestroyWindow ();
		theApp.m_wndChat.DestroyWindow ();
		theApp.m_wndWorld.DestroyWindow ();
		theAreaList.DestroyAllWindows ();
		theApp.m_wndBar.DestroyWindow ();
		if (theApp.m_pdlgRelations != NULL)
			{
			theApp.m_pdlgRelations->DestroyWindow ();
			theApp.m_pdlgRelations = NULL;
			}
		if (theApp.m_pdlgRsrch != NULL)
			{
			theApp.m_pdlgRsrch->DestroyWindow ();
			theApp.m_pdlgRsrch = NULL;
			}
		CDialog::OnOK();
		return;
		}

	if (theApp.SaveGame (this))
		{
		CDialog::OnOK();
		theApp.CloseWorld ();
		}
}

void CDlgFile::OnOK() 
{

	int iSound;
	if ( iWinType == W32s )
		iSound = m_scrSound.GetScrollPos ();
	else
		iSound = m_sldSound.GetPos ();

	// un/load the sound effects as needed
	if ( iSound )
		{
		if ( ! theMusicPlayer.IsGroupLoaded ( SFXGROUP::play ) )
			{
			CDlgSaveMsg dlgMsg ( this );
			dlgMsg.m_sText.LoadString (IDS_LOAD_SFX);
			dlgMsg.Create (IDD_SAVE_MSG, this);
			theMusicPlayer.LoadGroup (SFXGROUP::play);
			dlgMsg.DestroyWindow ();
			}
		}
	else
		theMusicPlayer.UnloadGroup (SFXGROUP::play);
	
	CDialog::OnOK();
}

void CDlgFile::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{

	if ( iWinType == W32s )
		{
		int iNum = pScrollBar->GetScrollPos ();
		BOOL bSpeed = pScrollBar == &m_scrSpeed;

		switch (nSBCode)
			{
			case SB_LINELEFT :
				iNum--;
				break;
			case SB_LINERIGHT :
				iNum++;
				break;

			// page - move 1/2 window
			case SB_PAGELEFT :
				if ( bSpeed )
					iNum -= 4;
				else
					iNum -= 10;
				break;
			case SB_PAGERIGHT :
				if ( bSpeed )
					iNum += 4;
				else
					iNum += 10;
				break;

			// move to the end - go to the exact oppisate end of the map
			case SB_LEFT :
				iNum = 0;
				break;
			case SB_RIGHT :
				iNum = 100;
				break;

			case SB_THUMBPOSITION :
				iNum = nPos;
				break;

			default:
				CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
				return;
			}

		iNum = __max ( iNum, 0 );
		if ( bSpeed )
			iNum = __min ( iNum, NUM_SPEEDS - 1 );
		else
			iNum = __min ( iNum, 100 );
		pScrollBar->SetScrollPos ( iNum );
		}

	// speed
	if ( ((CSliderCtrl *) pScrollBar == &m_sldSpeed) || (pScrollBar == &m_scrSpeed) )
		{
		int iSpeed;
		if ( iWinType == W32s )
			iSpeed = m_scrSpeed.GetScrollPos ();
		else
			iSpeed = m_sldSpeed.GetPos ();
		ASSERT ((0 <= iSpeed) && (iSpeed < NUM_SPEEDS));
		theApp.WriteProfileInt ("Game", "Speed", iSpeed);

		if ( theGame.GetServerNetNum () == 0 )
			theGame.SetGameMul (iSpeed);
		else
			{
			CMsgGameSpeed msg ( iSpeed );
			theGame.PostToServer ( &msg, sizeof (msg) );
			}
		}

	else
		// sound
		if ( ((CSliderCtrl *) pScrollBar == &m_sldSound) || (pScrollBar == &m_scrSound) )
			{
			int iSound;
			if ( iWinType == W32s )
				iSound = m_scrSound.GetScrollPos ();
			else
				iSound = m_sldSound.GetPos ();
			theApp.WriteProfileInt ("Game", "Sound", iSound);
			theMusicPlayer.SetSoundVolume (iSound);
			}

		else
			// music
			if ( ((CSliderCtrl *) pScrollBar == &m_sldMusic) || (pScrollBar == &m_scrMusic) )
				{
				int iMusic;
				if ( iWinType == W32s )
					iMusic = m_scrMusic.GetScrollPos ();
				else
					iMusic = m_sldMusic.GetPos ();
				theApp.WriteProfileInt ("Game", "Music", iMusic);
				int iOld = theMusicPlayer.GetMusicVolume ();
				theMusicPlayer.SetMusicVolume (iMusic);

				if ((iOld == 0) && (iMusic != 0))
					{
					theMusicPlayer.StartMidiMusic ();
					theMusicPlayer.PlayMusicGroup (MUSIC::GetID (MUSIC::play_game), MUSIC::num_play_game);
					}
				}
	
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgSaveMsg dialog

CDlgSaveMsg::CDlgSaveMsg(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSaveMsg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgSaveMsg)
	m_sText = _T("");
	m_sStat = _T("");
	//}}AFX_DATA_INIT
}


void CDlgSaveMsg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgSaveMsg)
	DDX_Text(pDX, IDC_SAVE_NAME, m_sText);
	DDX_Text(pDX, IDC_SAVE_STATUS, m_sStat);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgSaveMsg, CDialog)
	//{{AFX_MSG_MAP(CDlgSaveMsg)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgSaveMsg message handlers

int CDlgSaveMsg::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CenterWindow (&theApp.m_wndMain);
	
	return 0;
}

void CWndMain::OnSave() 
{
	
	theGame.SaveGame (this);
}

void CWndMain::OnBoss() 
{
	
	theApp.Minimize ();
}

void CWndMain::OnHelp() 
{
	
	theApp.WinHelp (0, HELP_CONTENTS);
}

void CWndMain::OnHide() 
{

	if (theApp.m_wndBar.m_hWnd != NULL)
		theApp.m_wndBar.ShowWindow ( SW_HIDE );
}

void CWndMain::OnUnHide() 
{

	// bottom of CWndMain
	CRect rect;
	GetClientRect ( &rect );
	rect.top = rect.bottom - TOOLBAR_HT;
	ClientToScreen ( &rect );

	// not under the taskbar (all 4 sides)
	APPBARDATA abd;
	memset ( &abd, 0, sizeof (abd) );
	abd.cbSize = sizeof ( abd );
	SHAppBarMessage ( ABM_GETTASKBARPOS, &abd );
	if ( abd.rc.top > theApp.m_iScrnY / 2 )		// bottom
		{
		if ( abd.rc.top < rect.bottom )
			rect.OffsetRect ( 0, abd.rc.top - rect.bottom );
		}
	else
		if ( abd.rc.right < theApp.m_iScrnX / 2 )		// left
			rect.left = __max ( rect.left, abd.rc.right );
		else
			if ( abd.rc.left > theApp.m_iScrnX / 2 )		// right
				rect.right = __min ( rect.right, abd.rc.left );
			// we don't care if at top

	ScreenToClient ( &rect );
	theApp.m_wndBar.SetWindowPos (NULL, rect.left, rect.top, rect.Width (), TOOLBAR_HT, SWP_NOZORDER);

	if (theApp.m_wndBar.m_hWnd != NULL)
		theApp.m_wndBar.ShowWindow ( SW_SHOW );
}

void CWndMain::OnArea() 
{

	if (theApp.m_wndBar.m_hWnd != NULL)
		theApp.m_wndBar.GotoArea ();
}

void CWndMain::OnMail() 
{
	
	if (theApp.m_wndBar.m_hWnd != NULL)
		theApp.m_wndBar.GotoChat ();
}

void CWndMain::OnOptions() 
{

	if (theApp.m_wndBar.m_hWnd != NULL)
		theApp.m_wndBar.GotoFile ();
}

void CWndMain::OnWorld() 
{
	
	if (theApp.m_wndBar.m_hWnd != NULL)
		theApp.m_wndBar.GotoWorld ();
}

void CWndMain::OnResearch() 
{
	
	if (theApp.m_wndBar.m_hWnd != NULL)
		theApp.m_wndBar.GotoScience ();
}

void CWndMain::OnDiplomat() 
{
	
	if (theApp.m_wndBar.m_hWnd != NULL)
		theApp.m_wndBar.GotoRelations ();
}

void CWndMain::OnBuildings() 
{
	
	if (theApp.m_wndBar.m_hWnd != NULL)
		theApp.m_wndBar.GotoBuildings ();
}

void CWndMain::OnVehicles() 
{
	
	if (theApp.m_wndBar.m_hWnd != NULL)
		theApp.m_wndBar.GotoVehicles ();
}

void CWndMain::OnNext() 
{
	// TODO: Add your command handler code here
	
}

void CWndMain::OnPrev() 
{
	// TODO: Add your command handler code here
	
}

void CWndMain::OnPause () 
{

	// only the server can pause
	if ((! theGame.AmServer ()) || (theGame.GetState () != CGame::play))
		return;

	BOOL bPause = theGame.DoOper ();

	_EnableGameWindows ( ! bPause );

	_OnPause ( bPause );

	theApp.GetDlgPause ()->Show ( bPause ? CDlgPause::server : CDlgPause::off );
}

void CWndMain::_OnPause (BOOL bPause) 
{

	// only the server can pause
	if ((! theGame.AmServer ()) || (theGame.GetState () != CGame::play))
		return;

	// may already be set
	if ( bPause == ! theGame.DoOper () )
		return;

	// pause
	if ( bPause )
		{
		theGame.SetOper (FALSE);
		CNetCmd msg (CNetCmd::cmd_pause);
		theGame.PostToAllClients (&msg, sizeof (msg));
		}
	else
		{
		theGame.SetOper (TRUE);
		CNetCmd msg (CNetCmd::cmd_resume);
		theGame.PostToAllClients (&msg, sizeof (msg));
		}
}

void CWndMain::_EnableGameWindows ( BOOL bEnable )
{

	if ( theApp.m_wndWorld.m_hWnd != NULL )
		theApp.m_wndWorld.EnableWindow ( bEnable );
	if ( theApp.m_wndChat.m_hWnd != NULL )
		theApp.m_wndChat.EnableWindow ( bEnable );
	if ( theApp.m_wndBar.m_hWnd != NULL )
		theApp.m_wndBar.EnableWindow ( bEnable );
	if ( theApp.m_wndBldgs.m_hWnd != NULL )
		theApp.m_wndBldgs.EnableWindow ( bEnable );
	if ( theApp.m_wndVehicles.m_hWnd != NULL )
		theApp.m_wndVehicles.EnableWindow ( bEnable );
	if ( (theApp.m_pdlgRelations != NULL) && (theApp.m_pdlgRelations->m_hWnd != NULL) )
		theApp.m_pdlgRelations->EnableWindow ( bEnable );
	if ( (theApp.m_pdlgFile != NULL) && (theApp.m_pdlgFile->m_hWnd != NULL) )
		theApp.m_pdlgFile->EnableWindow ( bEnable );
	if ( (theApp.m_pdlgRsrch != NULL) && (theApp.m_pdlgRsrch->m_hWnd != NULL) )
		theApp.m_pdlgRsrch->EnableWindow ( bEnable );

	theAreaList.EnableWindows ( bEnable );

	// if disabling put the chat window on the top
	if ( ! bEnable )
		if ( theApp.m_pdlgChat != NULL )
			{
			CDlgChatAll * pDlg = theApp.GetDlgChat ();
			if ( pDlg != NULL )
				{
				pDlg->ShowWindow ( SW_SHOWNORMAL );
				pDlg->SetFocus ();
				}
			}
}

BOOL CWndMain::OnQueryEndSession() 
{

	if (!CWndBase::OnQueryEndSession())
		return FALSE;
	
	// do they want to leave?
	TRAP ();
	if (! theApp.SaveGame (this))
		{
		TRAP ();
		return FALSE;
		}
	TRAP ();
	
	return TRUE;
}

void CWndMain::OnCloseApp() 
{

	// if we aren't playing - exit
	if (theGame.GetState () != CGame::play)
		{
		TRAP ();
		CWndBase::OnClose();
		return;
		}

	if (! theApp.SaveGame (this))
		return;

	theApp.CloseWorld ();
}

void CWndMain::EndLicense ()
{

	if ( m_bLicTimer )
		{
		m_bLicTimer = FALSE;
		KillTimer ( 99 );
		}

	if ( m_progPos == demo_license )
		{
	  // Play the startup movie
		if ( (theApp.HaveIntro ()) && (theApp.GetProfileInt ("Game", "NoIntro", 0) == 0) )
			{
			try
				{
				SetProgPos ( CWndMain::movie );
				UpdateWindow ();

				theApp.m_wndMovie.AddMovie ("logo.avi");
				theApp.m_wndMovie.AddMovie ("headgame.avi");
				theApp.m_wndMovie.AddMovie ("intro.avi");
				theApp.m_wndMovie.Create ( FALSE );
				}
			catch (...)
				{
				theApp.PostIntro ();
				}
			}
		else
			theApp.PostIntro ();
		return;
		}

	if ( m_progPos != retail_license )
		{
		SetProgPos ( CWndMain::playing );
		UpdateWindow ();
		return;
		}

	SetProgPos ( CWndMain::playing );
	UpdateWindow ();
	theApp.PostIntro ();
}
		
void CWndMain::OnTimer(UINT nIDEvent) 
{

	KillTimer ( nIDEvent );

	switch ( nIDEvent )
	  {
		case 99 :
			m_bLicTimer = FALSE;
			EndLicense ();
			break;
		case 109 :
			OnDisplayChange2 ();
			break;

		// only stop messages for 10 seconds max
		case 119 :
			if ( theGame.AmServer () )
				{
				POSITION pos;
				for (pos = theGame.GetAll ().GetHeadPosition(); pos != NULL; )
					{
					CPlayer *pPlr = theGame.GetAll().GetNext (pos);
					pPlr->m_bPauseMsgs = FALSE;
					}
				}
			theGame.PauseTimerFired ();
			theGame.SetMsgsPaused ( FALSE );

			if ( theApp.m_pLogFile != NULL )
				{
				SYSTEMTIME st;
				char sBuf [200];
				GetLocalTime ( &st );
				sprintf ( sBuf, "Timer flow ON at %d:%d", st.wMinute, st.wSecond );
				theApp.Log ( sBuf );
				}
			break;
	  }
	
	CWndBase::OnTimer(nIDEvent);
}

void CWndMain::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{

	if ( (m_progPos == demo_license) || (m_progPos == retail_license) )
		EndLicense ();
	
	CWndBase::OnChar(nChar, nRepCnt, nFlags);
}

void CWndMain::OnLButtonDown(UINT nFlags, CPoint point) 
{

	if ( (m_progPos == demo_license) || (m_progPos == retail_license) )
		EndLicense ();
	
	CWndBase::OnLButtonDown(nFlags, point);
}

void CWndMain::OnMButtonDown(UINT nFlags, CPoint point) 
{

	if ( (m_progPos == demo_license) || (m_progPos == retail_license) )
		EndLicense ();
	
	CWndBase::OnMButtonDown(nFlags, point);
}

void CWndMain::OnRButtonDown(UINT nFlags, CPoint point) 
{

	if ( (m_progPos == demo_license) || (m_progPos == retail_license) )
		EndLicense ();
	
	CWndBase::OnRButtonDown(nFlags, point);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgPause dialog


CDlgPause::CDlgPause(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgPause::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgPause)
	m_sText = _T("");
	//}}AFX_DATA_INIT
}


void CDlgPause::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgPause)
	DDX_Text(pDX, IDC_PAUSE_TEXT, m_sText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPause, CDialog)
	//{{AFX_MSG_MAP(CDlgPause)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPause message handlers

BOOL CDlgPause::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	CenterWindow ();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgPause::Show (int iMode)
{

	if ( m_hWnd == NULL )
		Create (IDD_PAUSE_MSG, &(theApp.m_wndMain));

	switch (iMode)
	  {
		case server :
			UpdateData (TRUE);
			m_sText.LoadString (IDS_PAUSE_SERVER);
			UpdateData (FALSE);
			CenterWindow ();
			ShowWindow ( SW_SHOW );
			SetWindowPos (&wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			theApp.m_wndMain.SetActiveWindow ();
			break;

		case client :
			UpdateData (TRUE);
			m_sText.LoadString (IDS_PAUSE_CLIENT);
			UpdateData (FALSE);
			CenterWindow ();
			ShowWindow ( SW_SHOW );
			SetWindowPos (&wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			theApp.m_wndMain.SetActiveWindow ();
			break;

		default:
			ShowWindow ( SW_HIDE );
			break;
	  }
}
