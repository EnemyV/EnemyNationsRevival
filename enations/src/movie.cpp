//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// main.cpp : implementation file
//

#include "stdafx.h"
#include <vfw.h>
#include <digitalv.h>

#include "lastplnt.h"
#include "movie.h"
#include "sfx.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


DWORD			m_MainmciDevID = 0;

void CWndMovie::Create ( BOOL bRepeat )
{

	if ( bRepeat )
		m_bShowLic = FALSE;

	theApp.Log ( "Starting movie player" );
	if ( m_lstFiles.GetCount () <= 0 )
		{
		Quit ();
		return;
		}

	// turn off music
	if ( ! theMusicPlayer.IsRunning () )
		m_bStartMusic = FALSE;
	else
		{
		m_bStartMusic = TRUE;
		theMusicPlayer.OnActivate ( FALSE );
		}

	theApp.m_wndMain.SetProgPos ( CWndMain::movie );

	m_bInDestroy = FALSE;

	const char *pCls = AfxRegisterWndClass (0, theApp.LoadStandardCursor (IDC_ARROW),
																								NULL, theApp.LoadIcon (IDI_MAIN));
	const DWORD dwSty = WS_POPUP;
	if (CreateEx (0, pCls, theApp.m_sAppName, dwSty, 0, 0, theApp.m_iScrnX, 
												theApp.m_iScrnY, theApp.m_wndMain.m_hWnd, NULL, NULL) == 0)
		ThrowError (ERR_RES_CREATE_WND);

	ShowCursor (FALSE);
	InvalidateRect (NULL);
	ShowWindow (SW_SHOW);

	// open the player and attach
	theApp.Log ( "Opening movie device" );
	MCI_DGV_OPEN_PARMS mdop;
	memset ( &mdop, 0, sizeof (mdop) );
	mdop.lpstrDeviceType = "avivideo";
	if ( mciSendCommand (0, MCI_OPEN, MCI_OPEN_TYPE, (DWORD) &mdop) != 0 )
		{
		TRAP ();
		Quit ();
		return;
		}
	m_MainmciDevID = mdop.wDeviceID;

	// start the first movie
	NextMovie ();
}


BEGIN_MESSAGE_MAP(CWndMovie, CWnd)
	//{{AFX_MSG_MAP(CWndMovie)
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_MESSAGE (MM_MCINOTIFY, OnMMMsg)
	ON_WM_SIZE()
	ON_WM_CHAR()
	ON_WM_LBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWndMovie message handlers

CWndMovie::CWndMovie()
{

	m_mciDevID = 0;
	m_hWndAvi = 0;
	m_bInDestroy = FALSE;
	m_bShowLic = TRUE;
	m_bStartMusic = FALSE;
}

CWndMovie::~CWndMovie()
{

}

void CWndMovie::AddMovie (char const * pFile)
{
static BOOL bFirst = TRUE;

	if (bFirst)
		{
		bFirst = FALSE;
		m_sDataDir = theApp.GetProfileString ("Game", "DataFile", ".\\");
		m_sDataDir.ReleaseBuffer ( m_sDataDir.ReverseFind ('\\') + 1 );
		m_sDataDir += "avi\\";

		m_sPatchDir = theApp.GetProfileString ("Game", "Patch", "..\\data");
		if (! m_sPatchDir.IsEmpty ())
			if (m_sPatchDir[m_sPatchDir.GetLength()-1] != '\\')
				m_sPatchDir += "\\";
		m_sPatchDir += "avi\\";
		}

	CString sFile = m_sPatchDir + CString (pFile);
	CFileStatus fs;
	if (CFile::GetStatus (sFile, fs) == 0)
		{
		sFile = m_sDataDir + CString (pFile);
		if (CFile::GetStatus (sFile, fs) == 0)
			return;
		}

	char *pStore = new char [sFile.GetLength () + 2];
	strcpy (pStore, (char const *) sFile);
	m_lstFiles.AddTail ( pStore );
}

void CWndMovie::OpenMovie (char * pFile)
{

	// open the file
	MCI_DGV_OPEN_PARMS mdop;
	memset (&mdop, 0, sizeof (mdop) );
	mdop.lpstrElementName = pFile;
	mdop.dwStyle = WS_CHILD;
	mdop.hWndParent = m_hWnd;
	if ( mciSendCommand (0, MCI_OPEN, MCI_OPEN_ELEMENT | MCI_DGV_OPEN_PARENT | MCI_DGV_OPEN_WS,
													(DWORD) &mdop) != 0)
		{
		TRAP ();
		Quit ();
		return;
		}
	m_mciDevID = mdop.wDeviceID;

	// open the playback window
	MCI_DGV_WINDOW_PARMS mdwp;
	memset (&mdwp, 0, sizeof (mdwp) );
	mdwp.nCmdShow = SW_SHOW;
	if ( mciSendCommand (m_mciDevID, MCI_WINDOW, MCI_DGV_WINDOW_STATE, (DWORD) & mdwp) == -1)
		{
		TRAP ();
		Quit ();
		return;
		}

	// get the window size
	MCI_DGV_RECT_PARMS mdrp;
	memset ( &mdrp, 0, sizeof (mdrp) );
	if ( mciSendCommand (m_mciDevID, MCI_WHERE, MCI_DGV_WHERE_WINDOW, (DWORD) & mdrp) == -1)
		{
		TRAP ();
		Quit ();
		return;
		}

	// size it for 2X
	if ((mdrp.rc.right <= 320) && (mdrp.rc.bottom <= 240))
		{
		mdrp.rc.right *= 2;
		mdrp.rc.bottom *= 2;
		mciSendCommand (m_mciDevID, MCI_PUT, MCI_DGV_RECT | MCI_DGV_PUT_CLIENT | MCI_DGV_PUT_WINDOW, (DWORD) & mdrp);
		mciSendCommand (m_mciDevID, MCI_PUT, MCI_DGV_RECT | MCI_DGV_PUT_DESTINATION | MCI_DGV_PUT_FRAME, (DWORD) & mdrp);
		}

	// get the playback window handle
	MCI_DGV_STATUS_PARMS mdsp;
	memset (&mdsp, 0, sizeof (mdsp) );
	mdsp.dwItem = MCI_DGV_STATUS_HWND;
	if ( mciSendCommand (m_mciDevID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD) & mdsp) == -1)
		{
		TRAP ();
		Quit ();
		return;
		}
	m_hWndAvi = (HWND) mdsp.dwReturn;

	CenterMovie ();

	UpdateWindow ();

	PlayMovie ();
}

void CWndMovie::CenterMovie ()
{

	MCI_DGV_RECT_PARMS mdrp;
	memset (&mdrp, 0, sizeof (mdrp) );

	if ( mciSendCommand (m_mciDevID, MCI_WHERE, MCI_DGV_WHERE_DESTINATION, (DWORD) &mdrp) != 0)
		{
		TRAP ();
		Quit ();
		return;
		}

	CRect rect (mdrp.rc);
	CRect rectPar;
	GetClientRect (&rectPar);
	::SetWindowPos (m_hWndAvi, NULL, ( rectPar.Width () - rect.Width () ) / 2 - rect.left,
											( rectPar.Height () - rect.Height () ) / 2 - rect.top, 
											0, 0, SWP_NOSIZE | SWP_NOZORDER );

	// invalidate both windows
	InvalidateRect (NULL);
	::InvalidateRect ( m_hWndAvi, NULL, FALSE );
}

void CWndMovie::PlayMovie ()
{

	MCI_DGV_PLAY_PARMS mdpp;
	memset (&mdpp, 0, sizeof (mdpp) );
	mdpp.dwCallback = MAKELONG (m_hWnd, 0);

	if ( mciSendCommand (m_mciDevID, MCI_PLAY, MCI_NOTIFY, (DWORD) &mdpp) != 0)
		{
		TRAP ();
		Quit ();
		return;
		}
}

void CWndMovie::CloseMovie ()
{

	MCI_GENERIC_PARMS mgp;
	memset (&mgp, 0, sizeof (mgp) );
	mciSendCommand (m_mciDevID, MCI_CLOSE, 0, (DWORD) &mgp );

	m_mciDevID = 0;
	m_hWndAvi = 0;

	InvalidateRect (NULL);
	UpdateWindow ();
}

void CWndMovie::NextMovie ()
{

	if (m_mciDevID != 0)
		CloseMovie ();

	// start the next movie
	if ( m_lstFiles.GetCount () > 0 )
		{
		try
			{
			theApp.Log ( "Play movie" );
			char * pFile = m_lstFiles.RemoveHead ();

			OpenMovie ( pFile );
			delete [] pFile;
			}
		catch (...)
			{
			Quit ();
			}
		}
	else
		Quit ();
}

void CWndMovie::Quit ()
{

	theApp.Log ( "Ending movie player" );
	m_bInDestroy = TRUE;

	if (m_mciDevID != 0)
		CloseMovie ();

	if ( m_MainmciDevID != 0 )
		{
		MCI_GENERIC_PARMS mgp;
		memset (&mgp, 0, sizeof (mgp) );
		mciSendCommand (m_MainmciDevID, MCI_CLOSE, 0, (DWORD) &mgp );
		}

	theApp.m_wndMain.SetProgPos ( CWndMain::playing );
	theApp.m_wndMain.ShowWindow (SW_SHOW);
	theApp.m_wndMain.InvalidateRect (NULL);

	if ( m_bStartMusic )
		{
		m_bStartMusic = FALSE;
		theMusicPlayer.OnActivate ( TRUE );
		}

	if ( m_hWnd == NULL )
		{
		ShowCursor ( TRUE );
		theApp.PostIntro ();
		}
	else
		DestroyWindow ();

#ifdef BUGBUG
	if ( ( ! m_bShowLic ) || theApp.IsShareware () )
		theApp.PostIntro ();
#endif

	theApp.Log ( "Exiting movie player" );
}

LRESULT CWndMovie::OnMMMsg (WPARAM wParam, LPARAM lParam)
{

	switch ( wParam )
	  {
		case MCI_NOTIFY_SUCCESSFUL :
			theApp.Log ( "Movie ended" );
			CloseMovie ();
			if ( m_lstFiles.GetCount () > 0 )
				{
				NextMovie ();
				break;
				}

			Quit ();
			break;
	  }

	return (0);
}

BOOL CWndMovie::OnEraseBkgnd(CDC *) 
{
	return TRUE;
}

void CWndMovie::OnPaint()
{

	CPaintDC dc(this); // device context for painting

	CRect rect;
	GetClientRect (&rect);

	CBrush brBlack;
	brBlack.CreateSolidBrush ( RGB (0, 0, 0) );
	dc.FillRect (&rect, &brBlack);
}

void CWndMovie::OnDestroy()
{

	if (theApp.m_wndMain.GetProgPos () == CWndMain::movie)
		theApp.m_wndMain.SetProgPos ( CWndMain::playing );

	m_bInDestroy = TRUE;

	if (m_mciDevID != 0)
		CloseMovie ();

	if ( m_MainmciDevID != 0 )
		{
		MCI_GENERIC_PARMS mgp;
		memset (&mgp, 0, sizeof (mgp) );
		mciSendCommand (m_MainmciDevID, MCI_CLOSE, 0, (DWORD) &mgp );
		}
}

void CWndMovie::PostNcDestroy()
{

	// start the audio
	if ( m_bStartMusic )
		theMusicPlayer.OnActivate ( TRUE );

	ShowCursor ( TRUE );
	theApp.PostIntro ();

	m_bShowLic = FALSE;
}

void CWndMovie::OnSize(UINT nType, int cx, int cy) 
{

	CWnd::OnSize(nType, cx, cy);
	
	if (m_hWndAvi != NULL)
		{
		TRAP ();
		CenterMovie ();
		}
}

void CWndMovie::OnChar(UINT , UINT , UINT ) 
{
	
	if ( m_lstFiles.GetCount () > 0 )
		NextMovie ();
	else
		Quit ();
}

void CWndMovie::OnLButtonDown(UINT , CPoint )
{
	
	if ( m_lstFiles.GetCount () > 0 )
		NextMovie ();
	else
		Quit ();
}

void CWndMovie::OnMButtonDown(UINT , CPoint )
{
	
	TRAP ();
	if ( m_lstFiles.GetCount () > 0 )
		NextMovie ();
	else
		Quit ();
}

void CWndMovie::OnRButtonDown(UINT , CPoint )
{
	
	if ( m_lstFiles.GetCount () > 0 )
		NextMovie ();
	else
		Quit ();
}
