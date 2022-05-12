//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////
//
//  IPCRead.cpp : IPC Read Email Window objects
//                Divide and Conquer
//               
//  Last update:    10/03/96 - 09/02/95
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "lastplnt.h"
#include "player.h"
#include "IPCRead.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

extern BOOL bTellMail;

extern int CreateToolBarCtrl (CToolBarCtrl & tool, int iCtrlID, int iBmID, UINT const * pBtnID, int iNumBtns, CWnd * pPar);

static UINT buttons_read[] =
{
	// same order as in the bitmap 'toolbar.bmp'
	IDS_MAIL_REPLY,
	IDS_MAIL_FORWARD,
	IDS_MAIL_DELETE,
		ID_SEPARATOR,
	IDS_MAIL_PREV,
	IDS_MAIL_NEXT
};


/////////////////////////////////////////////////////////////////////////////
// CWndMailRead

CWndMailRead::CWndMailRead( CEMsg *pMsg )
{
	m_pMsg = pMsg;
	m_xMin = m_yMin = 0;
	m_iID = pMsg->m_iID;
}

CWndMailRead::~CWndMailRead()
{
}


BEGIN_MESSAGE_MAP(CWndMailRead, CWndBase)
	//{{AFX_MSG_MAP(CWndMailRead)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_COMMAND (IDS_MAIL_REPLY, OnReply)
	ON_COMMAND (IDS_MAIL_FORWARD, OnForward)
	ON_COMMAND (IDS_MAIL_DELETE, OnDelete)
	ON_COMMAND (IDS_MAIL_PREV, OnScroll)
	ON_COMMAND (IDS_MAIL_NEXT, OnScroll)
	ON_MESSAGE( IPC_UNHIDE_WINDOW, OnUnHideWindow)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndMailRead message handlers

void CWndMailRead::OnReply ()
{
	GetParent()->PostMessage( IPC_EMAIL_REPLY,
		MAKEWPARAM( m_iID,0 ), NULL );
}

void CWndMailRead::OnForward ()
{
	GetParent()->PostMessage( IPC_EMAIL_FORWARD,
		MAKEWPARAM( m_iID,0 ), NULL );
}

void CWndMailRead::OnDelete ()
{
	GetParent()->PostMessage( IPC_EMAIL_DELETE,
		MAKEWPARAM( m_iID,0 ), NULL );

	PostMessage( WM_CLOSE, NULL, NULL );
}

void CWndMailRead::OnScroll ()
{
	const MSG* pMsg = GetCurrentMessage();
	GetParent()->PostMessage( IPC_EMAIL_SCROLL,
		MAKEWPARAM( m_iID,0 ), (LPARAM)pMsg->wParam );
}

#if 0
void CWndMailRead::OnKillFocus( CWnd* )
{
	// since the game's toolbar is always on top, if the IPC Comm window
	// is to be maximized, then part of it will be hidden by the toolbar
	// if the window goes to full screen, so determine the size of a full
	// screen and reduce that by 2 times the size of the caption, which
	// hopefully will get the bottom of the window above the toolbar
	CRect rectTB;
	theApp.m_wndBar.GetWindowRect( &rectTB );

	// check if we are slipping behind toolbar
	CRect rect;
	GetWindowRect( &rect );

	if( rect.top > (rectTB.top - GetSystemMetrics(SM_CYCAPTION)) )
	{
		// save current height to use later for bottom adjustment
		int iHeight = rect.Height();
		rect.top = rectTB.top - GetSystemMetrics(SM_CYCAPTION);
		// adjust bottom to current height to match new top
		rect.bottom = rect.top + iHeight;
		PostMessage( IPC_UNHIDE_WINDOW,
			MAKEWPARAM( rect.left, rect.top ), MAKELPARAM( rect.right, rect.bottom ) );
	}
}
#endif

void CWndMailRead::OnMove( int x, int y )
{
	// since the game's toolbar is always on top, if the IPC Comm window
	// is to be maximized, then part of it will be hidden by the toolbar
	// if the window goes to full screen, so determine the size of a full
	// screen and reduce that by 2 times the size of the caption, which
	// hopefully will get the bottom of the window above the toolbar
	CRect rectTB;
	theApp.m_wndBar.GetWindowRect( &rectTB );

	// WM_WINDOWPOSCHANGED
	// WM_CAPTURECHANGED
	// WM_MOVING
	// ON_WM_MOVE
	// WM_EXITSIZEMOVE
	// check if we are slipping behind toolbar
	CRect rect;
	GetWindowRect( &rect );

	if( rect.top > (rectTB.top - GetSystemMetrics(SM_CYCAPTION)) )
	{
		// save current height to use later for bottom adjustment
		int iHeight = rect.Height();
		rect.top = rectTB.top - GetSystemMetrics(SM_CYCAPTION);
		// adjust bottom to current height to match new top
		rect.bottom = rect.top + iHeight;
		PostMessage( IPC_UNHIDE_WINDOW,
			MAKEWPARAM( rect.left, rect.top ), MAKELPARAM( rect.right, rect.bottom ) );
	}

	CWndBase::OnMove(x, y);
}


LONG CWndMailRead::OnUnHideWindow( UINT uParam, LONG lParam )
{
	CRect rect( LOWORD( uParam ), HIWORD( uParam ),
		LOWORD( lParam ), HIWORD( lParam ) );

	MoveWindow( &rect, TRUE );
	return TRUE;
}


void CWndMailRead::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	
	ASSERT_VALID (this);

	if (lpMMI->ptMinTrackSize.x < m_xMin)
		lpMMI->ptMinTrackSize.x = m_xMin;

	if (lpMMI->ptMinTrackSize.y < m_yMin)
		lpMMI->ptMinTrackSize.y = m_yMin;

	// since the game's toolbar is always on top, if the IPC Comm window
	// is to be maximized, then part of it will be hidden by the toolbar
	// if the window goes to full screen, so determine the size of a full
	// screen and reduce that by 2 times the size of the caption, which
	// hopefully will get the bottom of the window above the toolbar
	CRect rectTB;
	theApp.m_wndBar.GetWindowRect( &rectTB );
	lpMMI->ptMaxTrackSize.y = rectTB.top - 1;
	lpMMI->ptMaxSize.y = rectTB.top - 1;

	CWndBase::OnGetMinMaxInfo(lpMMI);
}

void CWndMailRead::OnDestroy()
{
	// tell parent to update its message list box
	GetParent()->PostMessage( IPC_MESSAGE_UPDATE, NULL, NULL );
	// tell parent to remove this window from the open read list
	GetParent()->PostMessage( IPC_DESTROY_READWND, 
		MAKEWPARAM( m_iID,0 ), NULL );
		
	CWndBase::OnDestroy();
}

void CWndMailRead::Create (CWnd * pPar)
{

	ASSERT_VALID (this);
	ASSERT_VALID (pPar);

	// get name of player of message and put in title
	CString sName;
	CPlayer *pPlayer = theGame.GetPlayerByPlyr( m_pMsg->m_iFrom );
	if( pPlayer != NULL )
		sName = pPlayer->GetName();
	else
		sName = "Unknown player";

#if USE_FAKE_NET
	// make something appear
	if( sName.IsEmpty() )
		sName = "Eric";
#endif

	CString sTitle = "Read - [" + sName + "]";

	CRect rect;
	pPar->GetClientRect (&rect);
	pPar->ClientToScreen (&rect);

	CreateEx (0, theApp.m_sWndCls, sTitle, dwPopWndStyle, rect.left, 
		rect.top, rect.Width (), rect.Height(), pPar->m_hWnd, NULL);

	ShowWindow (SW_SHOW);
	UpdateWindow ();

	sName.Empty();
	sTitle.Empty();
}

void CWndMailRead::PostNcDestroy() 
{
	CWndBase::PostNcDestroy();

	delete this;
}

int CWndMailRead::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{

	if (CWndBase::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// create toolbar
	m_xMin = CreateToolBarCtrl (m_tool, IDT_MAIL_READ, IDB_TOOL_MAIL_READ, buttons_read,
														sizeof (buttons_read) / sizeof (UINT), this);
	CRect rect (0, 0, m_xMin, 20);
	AdjustWindowRect (&rect, dwPopWndStyle, FALSE);
	m_xMin = rect.Width () + 1;
	
	// create subject edit
	GetClientRect(&rect);
	CRect rEdit;
	rEdit.left = rect.left;
	rEdit.top = rect.top+1;
	rEdit.right = rect.right;
	rEdit.bottom = rEdit.top + 20;
	m_sub.Create( WS_CHILD | WS_VISIBLE | WS_BORDER| ES_LEFT |
		ES_READONLY, rEdit, this, 102);

	// create message edit
	rEdit.top = rEdit.bottom;
	rEdit.bottom = rect.bottom;
	m_text.Create( WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
		ES_LEFT | ES_MULTILINE | ES_READONLY, rEdit, this, 101);

	// load message body
	if( m_pMsg != NULL )
	{
		CString sBody;
		sBody = "RE: " + m_pMsg->m_sSubject;
		m_sub.SetWindowText( sBody );
		sBody.Empty();
		m_text.SetWindowText( m_pMsg->m_sMessage );
	}
	
	// determine real min size
	m_tool.GetClientRect (&rect);
	m_yMin = rect.Height () + HIWORD (GetDialogBaseUnits ()) * 3;
	rect.bottom = m_yMin;
	AdjustWindowRect (&rect, dwPopWndStyle, FALSE);
	m_yMin = rect.Height () + 1;

	// flag message as being read and is email
	m_pMsg->m_wStatus |= MSG_HAS_BEEN_READ;
	// message already flagged that it has an open window
	// m_pMsg->m_pOpenWnd = this;

	return 0;
}

void CWndMailRead::OnSize(UINT nType, int cx, int cy) 
{
	ASSERT_VALID (this);

	CWndBase::OnSize(nType, cx, cy);
	
	// tell the controls
	m_tool.AutoSize ();

	// fit the editbox to the window
	CRect rect, rTool;
	m_tool.GetClientRect (&rTool);
	GetClientRect (&rect);
	CRect rEdit;
	rEdit.top = rTool.bottom+1;
	rEdit.left = rect.left;
	rEdit.right = rect.right;
	rEdit.bottom = rEdit.top + 20;
	
	m_sub.SetWindowPos (NULL, rEdit.left, rEdit.top, rEdit.Width(), rEdit.Height(), SWP_NOZORDER);
	m_sub.InvalidateRect (NULL);

	rEdit.top = rEdit.bottom;
	rEdit.bottom = rect.bottom;
	m_text.SetWindowPos (NULL, rEdit.left, rEdit.top, rEdit.Width(), rEdit.Height(), SWP_NOZORDER);
	m_text.InvalidateRect (NULL);
}

void CWndMailRead::SetNewMessage( CEMsg *pNewMsg )
{

	// we've read a mail - so tell if another comes in
	bTellMail = TRUE;

	if( m_pMsg != NULL && pNewMsg != NULL )
	{
		m_pMsg->m_pOpenWnd = NULL;
		m_pMsg = pNewMsg;
		
		// flag message as being read and has an open read window
		m_pMsg->m_wStatus |= MSG_HAS_BEEN_READ;
		m_pMsg->m_pOpenWnd = (CWnd *)this;
		// save id of message being read
		m_iID = m_pMsg->m_iID;

		// get name of player of message and put in title
		CString sName;
		CPlayer *pPlayer = theGame.GetPlayerByPlyr( m_pMsg->m_iFrom );
		if( pPlayer != NULL )
			sName = pPlayer->GetName();
		else
			sName = "Unknown player";

#if USE_FAKE_NET
		// make something appear
		if( sName.IsEmpty() )
			sName = "Eric";
#endif

		CString sTitle = "Read - [" + sName + "]";
		SetWindowText( sTitle );
		sTitle.Empty();
		sName.Empty();

		sTitle = "RE: " + m_pMsg->m_sSubject;
		m_sub.SetWindowText( sTitle );
		sTitle.Empty();
		m_text.SetWindowText( m_pMsg->m_sMessage );
	}
}

// end of IPCRead.cpp
