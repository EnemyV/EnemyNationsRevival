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
#include "SDL2MFCPanel.h"
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
#include "cdloc.h"
#include "toolbar.h"
#include "msgs.h"
#include "chat.h"
#include "SDL2Dialogs.h"
#include "SDL2Video.h"
#include "GameWindow.h"

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
	if (CreateEx (dwExSty, theApp.m_sClsName.c_str(), theApp.m_sAppName.c_str(), dwSty, 0, 0, GetSystemMetrics (SM_CXSCREEN),
												GetSystemMetrics (SM_CYSCREEN), NULL, NULL, NULL) == 0)
		ThrowError (ERR_RES_CREATE_WND);
}

LRESULT CWndMain::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    // The MFC-style BEGIN_MESSAGE_MAP / ON_MESSAGE entries in this file are
    // killed by wndstub.h's macro overrides, so custom (user-range) messages
    // are never routed to their handlers.  Dispatch them here explicitly so
    // the virtual CWndStub::WindowProc sees them.
    switch (msg)
    {
        case WM_VPNOTIFY:         return OnNetMsg(wParam, lParam);
        case WM_VPFLOWOFF:        return OnNetFlowOff(wParam, lParam);
        case WM_VPFLOWON:         return OnNetFlowOn(wParam, lParam);
        case WM_ACTIVATE_MUSIC:   return OnActivateMusicMsg(wParam, lParam);
        case WM_MY_DISPLAYCHANGE: return OnMyDisplayChange(wParam, lParam);
        case WM_DISPLAYCHANGE:    return OnDisplayChange(wParam, lParam);
        default: break;
    }
    return CWndBase::WindowProc(msg, wParam, lParam);
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

typedef CWndStub _MainCmnWin;

static void MakeFullScreen ( _MainCmnWin * pWnd )
{

	if ( (pWnd != NULL) && (pWnd->m_hWnd != NULL) )
		{
		pWnd->SetWindowPos (NULL, 0, 0, theApp.m_iScrnX, theApp.m_iScrnY, SWP_NOZORDER);
		pWnd->InvalidateRect ( NULL );
		}
}

static void MoveToNew ( _MainCmnWin * pWnd, int xOld, int yOld )
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

static void MoveSizeToNew ( _MainCmnWin * pWnd, int xOld, int yOld )
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
	// CDlgMain excluded from build (Phase 2d) — SDL2MainMenu owns layout.
	// CWndMovie excluded from build (Phase 2d) — SDL2VideoPlayer is synchronous.
	MakeFullScreen ( &theApp.m_wndCredits );
	MakeFullScreen ( &theApp.m_wndCutScene );

	// these are dialogs - just move, don't size
	// MoveToNew for CDlgRelations removed (replaced by SDL2RelationsDialog)
	// MoveToNew for CDlgFile removed (Phase 2d) — SDL2FileDialog is modal.
	// MoveToNew for CDlgResearch removed (Phase 2d) — SDL2ResearchDialog is modal.
	// MoveToNew for CDlgPause removed — no longer a CWnd, centers itself on Show()
	// MoveToNew for CDlgPlyrList removed (Phase 2d) — SDL2PlayerListDialog is modal.

	// these move & size
	MoveSizeToNew ( &theApp.m_wndWorld, xOld, yOld );
	// m_wndChat skipped (Phase 2d-cont) — ChatStub has no CWnd surface, m_hWnd always NULL
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
	if ( !theApp.m_wndBar.IsCreated() )
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
		m_sText.clear ();
		if (m_fnt.m_hObject != NULL)
			m_fnt.DeleteObject ();
		return;
		}
		
	CMmio *pMmio = theDataFile.OpenAsMMIO (NULL, "LANG");
	pMmio->DescendRiff ('L', 'A', 'N', 'G');
	pMmio->DescendList ('L', 'E', 'G', 'L');
	pMmio->DescendChunk ('L', 'I', 'C', m_progPos == demo_license ? '2' : '3');

	long lSize = pMmio->ReadLong ();
	m_sText.resize (lSize + 2);
	pMmio->Read (&m_sText[0], lSize);
	delete pMmio;
	m_sText.resize (lSize);

	// get the font
	std::string sFont = EnGetProfileStdString("StatusBar", "Font", "Newtown Italic");
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
		strncpy (lf.lfFaceName, sFont.c_str(), LF_FACESIZE-1);
		m_fnt.CreateFontIndirect (&lf);

		// size it
		CFont * pOld = dc.SelectObject ( &m_fnt );
		GetClientRect (&rect);
		int iDif = rect.Width () / 4;
		rect.left += iDif;
		rect.right -= iDif;
		dc.DrawText ( m_sText.c_str(), -1, &rect, DT_CALCRECT | DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
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
		CWndBaseSuper::OnPaletteChanged(pFocusWnd);
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
		return ( CWndBaseSuper::OnQueryNewPalette() );
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
	std::string sVer ("Version: " VER_STRING);
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
		dc.TextOut (0, rect.bottom - tm.tmHeight, sVer.c_str(), (int)sVer.length());
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
		std::string sLoad = EnLoadStdString(m_progPos == exiting ? IDS_LEAVING : IDS_LOADING);
		dc.TextOut (0, 0, sLoad.c_str(), (int)sLoad.length());
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
	dc.TextOut (0, rect.bottom - tm.tmHeight, sVer.c_str(), (int)sVer.length());
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
		std::string sLoad = EnLoadStdString(IDS_EXIT_GAME);
		dc.TextOut (0, 0, sLoad.c_str(), (int)sLoad.length());
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
	dc.DrawText ( m_sText.c_str(), -1, &rect, DT_CALCRECT | DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
	rect.top = (iHt - rect.Height ()) / 2;
	rect.bottom = iHt;

	// draw dark bevel
	rect.OffsetRect (- 1, - 1 );
	dc.SetTextColor (PALETTERGB (9, 11, 20));
	dc.DrawText ( m_sText.c_str(), -1, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
	rect.OffsetRect ( 1, 0 );
	dc.DrawText ( m_sText.c_str(), -1, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);

	// draw light bevel
	rect.OffsetRect ( 0, 2 );
	dc.SetTextColor (PALETTERGB (76, 81, 118));
	dc.DrawText ( m_sText.c_str(), -1, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);
	rect.OffsetRect ( 1, 0 );
	dc.DrawText ( m_sText.c_str(), -1, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);

	// draw face
	rect.OffsetRect (- 1, - 1);
	dc.SetTextColor (PALETTERGB (152, 162, 236));
	dc.DrawText ( m_sText.c_str(), -1, &rect, DT_NOPREFIX | DT_LEFT | DT_WORDBREAK);

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

void CWndMain::OnActivateApp(BOOL bActive, DWORD hTask)
{

	CWndBase::OnActivateApp (bActive, hTask);
	
	// turn the sound off, lower priority
	if (! bActive)
		{
		if ( theGame.HaveHP () )
			{
			if ((theGame.ShouldOperate() ) && theApp.m_bPauseOnAct )
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
		auto pWnd = SetActiveWindow ();
		SetActiveWindow ();
		if (pWnd) pWnd->SetActiveWindow ();
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
		// Resume sound directly. This used to PostMessage(WM_ACTIVATE_MUSIC) to
		// defer until the app was truly active, but in the SDL2 port the MFC main
		// window is hidden and the posted message wasn't reliably dispatched — so
		// music paused on focus loss (OnActivate(FALSE) above) but never resumed.
		// OnActivate(TRUE) is safe to call synchronously here.
		theMusicPlayer.OnActivate ( TRUE );
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
// CDlgFile removed (Phase 2d) — replaced by SDL2FileDialog.
// SaveExistingGame is a free function used by netapi.cpp; kept here for
// legacy reasons.

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

	if (EnMessageBox(IDS_SAVE_OLD, MB_YESNO | MB_ICONQUESTION) == IDYES)
		theGame.SaveGame( (CWnd*)NULL );
}


/////////////////////////////////////////////////////////////////////////////
// CDlgSaveMsg — SDL2-rendered modeless save-progress indicator
//
// Owns an internal heap-allocated SDL2 dialog (deleted by GameWindow on
// EndDialog cleanup). UpdateData() syncs m_sText/m_sStat into the label
// widgets; GameWindow renders the dialog each frame, so the save loop's
// theApp.BaseYield() calls naturally pick up the refresh.

class _SaveProgressDialog : public SDL2Dialog
{
public:
	_SaveProgressDialog( GameWindow* gw )
		: SDL2Dialog( gw, "Enemy Nations - Saving", 360, 120 )
	{}

	void SetMessages( const std::string& text, const std::string& stat )
	{
		if ( m_lblText ) m_lblText->SetText( text );
		if ( m_lblStat ) m_lblStat->SetText( stat );
	}

	std::string m_initialText;
	std::string m_initialStat;

protected:
	void OnInit() override
	{
		const int margin = 10;
		const int halfH  = ( m_height - 30 ) / 2;
		m_lblText = AddWidget<SDL2Label>(
			m_x + margin, m_y + 30, m_width - margin * 2, halfH - 4,
			m_initialText );
		m_lblText->SetWrapped( true );
		m_lblText->SetTopAligned( true );

		m_lblStat = AddWidget<SDL2Label>(
			m_x + margin, m_y + 30 + halfH, m_width - margin * 2, halfH - 4,
			m_initialStat );
		m_lblStat->SetWrapped( true );
		m_lblStat->SetTopAligned( true );
	}

	// Progress dialog — no buttons. Block accidental ESC dismissal so
	// the save loop can close us programmatically when finished.
	void OnCancel() override { /* swallow */ }

private:
	SDL2Label* m_lblText = nullptr;
	SDL2Label* m_lblStat = nullptr;
};

CDlgSaveMsg::CDlgSaveMsg(CWnd* /*pParent*/)
	: m_pDlg( nullptr )
{
	m_sText.clear();
	m_sStat.clear();
}

CDlgSaveMsg::~CDlgSaveMsg()
{
	DestroyWindow();
}

void CDlgSaveMsg::Create( UINT /*nIDTemplate*/, CWnd* /*pParent*/ )
{
	if ( m_pDlg )
		return;

	GameWindow* gw = theApp.m_gameWindow ? theApp.m_gameWindow.get() : nullptr;
	if ( !gw )
		return;

	m_pDlg = new _SaveProgressDialog( gw );
	m_pDlg->m_initialText = m_sText;
	m_pDlg->m_initialStat = m_sStat;

	_SaveProgressDialog** ppDlg = &m_pDlg;
	m_pDlg->ShowNonModal( [ppDlg]( int /*result*/ ) { *ppDlg = nullptr; } );
}

void CDlgSaveMsg::DestroyWindow()
{
	if ( m_pDlg )
	{
		m_pDlg->EndDialog( 0 );  // fires onDone -> sets m_pDlg = nullptr
		// GameWindow's next cleanup pass deletes the dialog object itself.
		m_pDlg = nullptr;
	}
}

void CDlgSaveMsg::UpdateData( BOOL /*bSaveAndValidate*/ )
{
	if ( m_pDlg )
		m_pDlg->SetMessages( m_sText, m_sStat );
	// GameWindow renders the dialog per frame; theApp.BaseYield() in the
	// save loop pumps the frame, so the new text appears within a tick.
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

	if (theApp.m_wndBar.IsCreated())
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

	if (theApp.m_wndBar.IsCreated())
		theApp.m_wndBar.ShowWindow ( SW_SHOW );
}

void CWndMain::OnArea() 
{

	if (theApp.m_wndBar.IsCreated())
		theApp.m_wndBar.GotoArea ();
}

void CWndMain::OnMail() 
{
	
	if (theApp.m_wndBar.IsCreated())
		theApp.m_wndBar.GotoChat ();
}

void CWndMain::OnOptions() 
{

	if (theApp.m_wndBar.IsCreated())
		theApp.m_wndBar.GotoFile ();
}

void CWndMain::OnWorld() 
{
	
	if (theApp.m_wndBar.IsCreated())
		theApp.m_wndBar.GotoWorld ();
}

void CWndMain::OnResearch() 
{
	
	if (theApp.m_wndBar.IsCreated())
		theApp.m_wndBar.GotoScience ();
}

void CWndMain::OnDiplomat() 
{
	
	if (theApp.m_wndBar.IsCreated())
		theApp.m_wndBar.GotoRelations ();
}

void CWndMain::OnBuildings() 
{
	
	if (theApp.m_wndBar.IsCreated())
		theApp.m_wndBar.GotoBuildings ();
}

void CWndMain::OnVehicles() 
{
	
	if (theApp.m_wndBar.IsCreated())
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

	BOOL bPause = theGame.ShouldOperate();

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
	if ( bPause == !theGame.ShouldOperate() )
		return;

	// pause
	if ( bPause )
		{
            theGame.SetShouldOperate(FALSE);
		CNetCmd msg (CNetCmd::cmd_pause);
		theGame.PostToAllClients (&msg, sizeof (msg));
		}
	else
		{
            theGame.SetShouldOperate(TRUE);
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
	if ( theApp.m_wndBar.IsCreated() )
		theApp.m_wndBar.EnableWindow ( bEnable );
	if ( theApp.m_wndBldgs.m_hWnd != NULL )
		theApp.m_wndBldgs.EnableWindow ( bEnable );
	if ( theApp.m_wndVehicles.m_hWnd != NULL )
		theApp.m_wndVehicles.EnableWindow ( bEnable );
	// CDlgFile + CDlgResearch removed (Phase 2d) — both modal in SDL2 path.

	theAreaList.EnableWindows ( bEnable );

	// CDlgChatAll excluded from build (Phase 2d) — no chat-on-top behaviour.
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
		// Play the startup movie via SDL2VideoPlayer (Phase 2d — CWndMovie excluded).
		// Note: CConquerApp::InitInstance also kicks off intro videos via
		// SDL2VideoPlayer when SDL2 is up; this path covers the post-license
		// flow when the license dialog finishes after init.
		if ( (theApp.HaveIntro ()) && (EnGetProfileInt("Game", "NoIntro", 0) == 0)
		     && (theApp.m_gameWindow) )
			{
			try
				{
				SetProgPos ( CWndMain::movie );
				UpdateWindow ();

				SDL2VideoPlayer::PlayVideo( theApp.m_gameWindow.get(), "assets\\videos\\logo.mpg" );
				SDL2VideoPlayer::PlayVideo( theApp.m_gameWindow.get(), "assets\\videos\\intro.mpg" );
				}
			catch (...)
				{
				}
			}
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
              theGame.ResetPauseTimer();
              theGame.SetMessagesPaused(FALSE);

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


// CDlgPause — non-MFC modeless pause notification

const char* CDlgPause::s_className = "ENPauseMsg";
bool CDlgPause::s_classRegistered = false;

CDlgPause::CDlgPause(CWnd* pParent /*=NULL*/)
	: m_hWnd( NULL )
{
	m_sText.clear();
}

CDlgPause::~CDlgPause()
{
	if ( m_hWnd && ::IsWindow( m_hWnd ) )
	{
		::SetWindowLongPtr( m_hWnd, GWLP_USERDATA, 0 );
		::DestroyWindow( m_hWnd );
	}
	m_hWnd = NULL;
}

LRESULT CALLBACK CDlgPause::WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case WM_CREATE:
	{
		CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
		::SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pcs->lpCreateParams );
		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint( hwnd, &ps );
		CDlgPause* pThis = (CDlgPause*)::GetWindowLongPtr( hwnd, GWLP_USERDATA );
		if ( pThis )
		{
			RECT rc;
			::GetClientRect( hwnd, &rc );
			::SetBkMode( hdc, TRANSPARENT );
			RECT rcText = { 10, 10, rc.right - 10, rc.bottom - 10 };
			::DrawTextA( hdc, pThis->m_sText.c_str(), -1, &rcText, DT_WORDBREAK | DT_CENTER | DT_VCENTER | DT_SINGLELINE );
		}
		::EndPaint( hwnd, &ps );
		return 0;
	}
	}
	return ::DefWindowProc( hwnd, msg, wParam, lParam );
}

void CDlgPause::Repaint()
{
	if ( m_hWnd && ::IsWindow( m_hWnd ) )
		::InvalidateRect( m_hWnd, NULL, TRUE );
}

void CDlgPause::Show (int iMode)
{
	if ( iMode == off )
	{
		if ( m_hWnd && ::IsWindow( m_hWnd ) )
			::ShowWindow( m_hWnd, SW_HIDE );
		return;
	}

	// Create window if needed
	if ( m_hWnd == NULL )
	{
		if ( !s_classRegistered )
		{
			WNDCLASSEXA wc = {};
			wc.cbSize        = sizeof( wc );
			wc.lpfnWndProc   = WndProc;
			wc.hInstance      = ::GetModuleHandle( NULL );
			wc.hCursor        = ::LoadCursor( NULL, IDC_ARROW );
			wc.hbrBackground  = (HBRUSH)( COLOR_BTNFACE + 1 );
			wc.lpszClassName  = s_className;
			::RegisterClassExA( &wc );
			s_classRegistered = true;
		}

		int w = 280, h = 80;
		int x = ( ::GetSystemMetrics( SM_CXSCREEN ) - w ) / 2;
		int y = ( ::GetSystemMetrics( SM_CYSCREEN ) - h ) / 2;

		m_hWnd = ::CreateWindowExA(
			WS_EX_TOPMOST,
			s_className,
			"Enemy Nations",
			WS_POPUP | WS_BORDER,
			x, y, w, h,
			NULL, NULL,
			::GetModuleHandle( NULL ),
			this );
	}

	switch (iMode)
	  {
		case server :
			m_sText = EnLoadStdString(IDS_PAUSE_SERVER);
			Repaint();
			::ShowWindow( m_hWnd, SW_SHOW );
			::SetWindowPos( m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
			break;

		case client :
			m_sText = EnLoadStdString(IDS_PAUSE_CLIENT);
			Repaint();
			::ShowWindow( m_hWnd, SW_SHOW );
			::SetWindowPos( m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
			break;

		default:
			::ShowWindow( m_hWnd, SW_HIDE );
			break;
	  }
}

void CDlgPause::DestroyWindow()
{
	if ( m_hWnd && ::IsWindow( m_hWnd ) )
	{
		::SetWindowLongPtr( m_hWnd, GWLP_USERDATA, 0 );
		::DestroyWindow( m_hWnd );
	}
	m_hWnd = NULL;
}
