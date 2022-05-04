//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////
//
//  IPCChat.cpp : IPC Chat Window objects
//                Divide and Conquer
//               
//  Last update: 10/03/96   -   09/02/95
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "lastplnt.h"
#include "IPCChat.h"
#include "player.h"
#include "ipcplay.h"
#include "ipccomm.h"

IMPLEMENT_DYNCREATE(CChatWnd, CFrameWnd)
IMPLEMENT_DYNCREATE(CChatView, CEditView)
IMPLEMENT_DYNCREATE(CSendView, CEditView)


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


// list of email messages
extern CEMsgList *plEmailMsgs;	
// limit to the size of the chat history that is saved
const int iMaxChatHistory = 10000;

/////////////////////////////////////////////////////////////////////////////
// CChatWnd


CChatWnd::CChatWnd()
{

	// BUGBUG hardcode chat to self, need to be able
	// to select a player to chat with
	m_iTo = m_iFrom = theGame.GetMe()->GetPlyrNum ();
	m_bPanesOK = FALSE;
	m_iCur0 = 0;
	m_iCur1 = 0;
	m_iMinPaneHeight = 0;
}

CChatWnd::~CChatWnd()
{
}

BEGIN_MESSAGE_MAP(CChatWnd, CFrameWnd)
	//{{AFX_MSG_MAP(CChatWnd)
	ON_WM_CREATE()
	ON_WM_GETMINMAXINFO()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_BN_CLICKED(IDC_HANGUP, OnClose)
	ON_CBN_DBLCLK(IDC_SELECTPLAYER, OnSelectplayer)
	ON_CBN_SELCHANGE(IDC_SELECTPLAYER, OnSelectplayer)
	ON_MESSAGE( IPC_UNHIDE_WINDOW, OnUnHideWindow)
	//ON_EN_CHANGE(AFX_IDW_PANE_FIRST,OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CChatWnd message handlers

LONG CChatWnd::OnUnHideWindow( UINT uParam, LONG lParam )
{
	CRect rect( LOWORD( uParam ), HIWORD( uParam ),
		LOWORD( lParam ), HIWORD( lParam ) );

	MoveWindow( &rect, TRUE );
	return TRUE;
}

void CChatWnd::OnMove( int x, int y )
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

	CFrameWnd::OnMove(x, y);
}

#if 0
void CChatWnd::OnSetFocus( CWnd* )
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


void CChatWnd::OnKillFocus( CWnd* )
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
}
#endif

// called to set who you are talking to
void CChatWnd::SetFrom (CPlayer * pPlyr)
{

	ASSERT_VALID (this);
	ASSERT_VALID (pPlyr);

	CIPCPlayer *pPlayer = plIPCPlayers->GetPlayer( pPlyr->GetPlyrNum() );
	ASSERT_VALID (pPlayer);
	ASSERT (pPlayer->m_pwndChat == NULL);
	pPlayer->m_pwndChat = this;

	// update the player id we are chatting with
	m_iTo = pPlyr->GetPlyrNum();

	// turn off chat button if everyone has a chat window
	//((CWndComm *) GetParent ())->CheckChatButton ();

	// update the title
	CString sTitle;
	sTitle.LoadString (IDS_CHAT_TITLE);
	csPrintf (&sTitle, (const char *) pPlyr->GetName ());
	SetWindowText (sTitle);

	// we now disable the combo box & enable the hang-up
	m_pdbChatBar->GetDlgItem (IDC_SELECTPLAYER)->EnableWindow (FALSE);
	m_pdbChatBar->GetDlgItem (IDC_HANGUP)->EnableWindow (TRUE);
}


//
// a player was selected from the player combo box
//
void CChatWnd::OnSelectplayer() 
{
	// no toolbar yet
	if( m_pdbChatBar == NULL )
		return;

	CComboBox *pPlayerBox = 
		(CComboBox *)m_pdbChatBar->GetDlgItem( IDC_SELECTPLAYER );
	if( pPlayerBox != NULL )
	{
		int iRet = pPlayerBox->GetCurSel();
		if( iRet != CB_ERR )
		{
			CPlayer *pPlyr = (CPlayer *)pPlayerBox->GetItemDataPtr(iRet);
			if( pPlyr != NULL &&
				(int)pPlyr != -1 )
			{

				// see if we already have a chat session with this player
				CIPCPlayer *pPlayer = plIPCPlayers->GetPlayer( pPlyr->GetPlyrNum() );
				if ((pPlayer != NULL) && (pPlayer->m_pwndChat != NULL))
					{
					TRAP ();
					pPlayer->m_pwndChat->SetForegroundWindow ();
					return;
					}

				SetFrom (pPlyr);

				// get pointer to outbound chat pane
				CSendView *pSendView = (CSendView*)m_wndSplitter.GetPane(1,0);
				pSendView->SetPlayers( m_iFrom, m_iTo );
			}
		}
	}
}

void CChatWnd::OnDestroy()
{

	if( m_pdbChatBar != NULL )
		delete m_pdbChatBar;

	CIPCPlayer *pPlayer = plIPCPlayers->GetPlayer (m_iTo);
	if ((pPlayer != NULL) && (pPlayer->m_pwndChat == this))
		{
		pPlayer->m_pwndChat = NULL;
		((CWndComm *) GetParent ())->CheckChatButton ();
		}

	CFrameWnd::OnDestroy();
}



// BUGBUG this is the incoming chat message process
// for control coded message of chat data
//
void CChatWnd::OnChatIn (CMsgIPC * pMsg)
{

	CChatView *pChatView = (CChatView *)m_wndSplitter.GetPane(0,0);
	if( pChatView == NULL )
		{
		TRAP ();
		return;
		}

	CString sOld;
	pChatView->GetEditCtrl().GetWindowText( sOld );

	// now, using the m_iCC in the message to
	// indicate where in the old message to
	// begin appending the new string just arrived
	
	if( sOld.IsEmpty() ||
		!pMsg->m_iCC )
	{
		pChatView->GetEditCtrl().SetWindowText( pMsg->m_sMessage );
	}
	else
	{
	// BUGBUG turn on to use replace selection approach
	// turn off to use complete replace with appending
#if 1
		//CEdit::ReplaceSel( LPCTSTR lpszNewText );
		// combine replacement of old text with new text
		// and hopefully scroll into view
		if( pMsg->m_iCC < sOld.GetLength() )
		{
			//pChatView->GetEditCtrl().SetReadOnly(FALSE);
			pChatView->GetEditCtrl().SetSel(
				pMsg->m_iCC,sOld.GetLength());

			pChatView->GetEditCtrl().ReplaceSel(
				(LPCTSTR)pMsg->m_sMessage );

			//for( int i=0; i<pMsg->m_sMessage.GetLength(); ++i )
			//	pChatView->GetEditCtrl().SendMessage(
			//		WM_CHAR, (TCHAR)pMsg->m_sMessage[i], 1 );

			//pChatView->GetEditCtrl().SetReadOnly(TRUE);
		}
		else
		{
			//pChatView->GetEditCtrl().SetReadOnly(FALSE);
			pChatView->GetEditCtrl().SetSel(pMsg->m_iCC,pMsg->m_iCC);

			pChatView->GetEditCtrl().ReplaceSel(
				(LPCTSTR)pMsg->m_sMessage );

			//for( int i=0; i<pMsg->m_sMessage.GetLength(); ++i )
			//	pChatView->GetEditCtrl().SendMessage(
			//		WM_CHAR, (TCHAR)pMsg->m_sMessage[i], 1 );

			//pChatView->GetEditCtrl().SetReadOnly(TRUE);
		}
#elif 0	// another style of replace 
		CString sSave;
		sSave = sOld.Left( pMsg->m_iCC );
		CString sNew = sSave + pMsg->m_sMessage;

		pChatView->GetEditCtrl().SetReadOnly(FALSE);
		pChatView->GetEditCtrl().SetSel( 0, sOld.GetLength() );
		pChatView->GetEditCtrl().ReplaceSel( sNew );
		pChatView->GetEditCtrl().SetReadOnly(TRUE);
#else	// use setwindow instead
		CString sSave;
		sSave = sOld.Left( pMsg->m_iCC );

		CString sNew = sSave + pMsg->m_sMessage;
		pChatView->GetEditCtrl().SetWindowText( sNew );
		pChatView->GetEditCtrl().SetSel( 
			sNew.GetLength(), sNew.GetLength(), TRUE );
		pChatView->GetEditCtrl().SendMessage( EM_SCROLLCARET, 0, 0 );

		sNew.Empty();
		sSave.Empty();
#endif
	}
	
// BUGBUG turn on to scroll last character into view, used
// with the complete replace with appending approach
#if 1
	// set the carat to the last character in the message
	pChatView->GetEditCtrl().GetWindowText( sOld );
	int i = sOld.GetLength() - 1;
	pChatView->GetEditCtrl().SetSel( i, i, FALSE );
	//pChatView->GetEditCtrl().SendMessage( EM_SCROLLCARET, 0, 0 );
#endif
}


// BUGBUG this is the incoming chat message process
// for receiving a complete message at one time
//
#if 0
LONG CChatWnd::OnChatIn( UINT, LONG lParam )
{
	//CEMsg *pMsg = (CEMsg *)lParam;
	int iFrom = LOWORD(lParam);
	int iTo = HIWORD(lParam);

	CEMsg *pMsg = GetChatMsg( iFrom, iTo );
	if( pMsg == NULL )
		return FALSE;

	// display message in the incoming chat pane
	((CChatView *)(m_wndSplitter.GetPane(0,0)))->GetEditCtrl().SetWindowText( 
		pMsg->m_sMessage );

	// should be using only one message for the
	// entire chat session with this player
	if( m_pMsg == NULL )
		m_pMsg = pMsg;
	else
	{
		// consider replacing the chat session message
		if( m_pMsg->m_iID != pMsg->m_iID )
		{
			GetParent()->PostMessage( IPC_EMAIL_DELETE,
				MAKEWPARAM( m_pMsg->m_iID,0 ), NULL );
			m_pMsg = pMsg;
		}
	}

	return TRUE;
}
#endif

BOOL CChatWnd::OnCreateClient (LPCREATESTRUCT, CCreateContext* pContext) 
{	
	// create toolbar using my chat bar approach
	m_pdbChatBar = new CChatBar( this );
	BOOL bRet = m_pdbChatBar->Create( this, IDD_CHATBAR, //"ChatBar",
		CBRS_TOP, IDD_CHATBAR );
	m_pdbChatBar->GetDlgItem (IDC_HANGUP)->EnableWindow (FALSE);

	m_pdbChatBar->SetWindowPos( &wndTop, 0,0,0,0, SWP_SHOWWINDOW );
	RecalcLayout();
	if( bRet )
		m_pdbChatBar->InitBar();

	CRect rBar;
	m_pdbChatBar->GetWindowRect( &rBar );
// OnCommand ON_COMMAND
	if (m_wndSplitter.CreateStatic (this,2,1))
	{
		CClientDC dc(this);
		CString sText;
		GetWindowText( sText );
		CSize csText = dc.GetTextExtent( sText, sText.GetLength() );
		m_iMinPaneHeight = csText.cy + (csText.cy/4);
		sText.Empty();

		CRect rect;
		GetClientRect(&rect);
		CSize size = rect.Size();
		size.cy -= rBar.Height();
		size.cy -= (size.cy/3)*2;
		if (m_wndSplitter.CreateView (0, 0, RUNTIME_CLASS (CChatView), size, pContext))
	 	{
			if (m_wndSplitter.CreateView (1, 0, RUNTIME_CLASS (CSendView), CSize(0,0), pContext))
			{
				CSendView *pSendView = (CSendView*)m_wndSplitter.GetPane(1,0);
				SetActiveView( (CView*)pSendView );

				// BUGBUG force chat between player 1
				// later the players will be set by the
				// player selection combo box and a 
				// CEMsg will be created to help control
				// the number of chat windows opened
				// for a given player
				pSendView->SetParent( this );
				pSendView->SetPlayers( m_iFrom, m_iTo );

				// so for now create 
				// the CEMsg here with default from/to
				m_bPanesOK = TRUE;

				int iCur0, iCur1, iMin;
				m_wndSplitter.GetRowInfo( 0, iCur0, iMin );
				m_wndSplitter.GetRowInfo( 1, iCur1, iMin );
				return TRUE;	
			}
		}
	}
	return FALSE;
}

int CChatWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}

void CChatWnd::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{

	ASSERT_VALID (this);
	// since the game's toolbar is always on top, if the IPC Comm window
	// is to be maximized, then part of it will be hidden by the toolbar
	// if the window goes to full screen, so determine the size of a full
	// screen and reduce that by 2 times the size of the caption, which
	// hopefully will get the bottom of the window above the toolbar
	CRect rectTB;
	theApp.m_wndBar.GetWindowRect( &rectTB );
	lpMMI->ptMaxTrackSize.y = rectTB.top - 1;
	lpMMI->ptMaxSize.y = rectTB.top - 1;

	CFrameWnd::OnGetMinMaxInfo(lpMMI);
}

void CChatWnd::OnSize( UINT nType, int cx, int cy )
{

	CFrameWnd::OnSize(nType, cx, cy);

	CRect rBar;
	if( m_pdbChatBar != NULL && m_pdbChatBar->m_hWnd != NULL )
	{
		m_pdbChatBar->GetWindowRect( &rBar );
		m_pdbChatBar->SetWindowPos( &wndTop, 0,0,cx,rBar.Height(), 
			SWP_SHOWWINDOW );
		RecalcLayout();
	}

	if( m_bPanesOK ) // m_wndSplitter.m_hWnd != NULL )
	{
		int iRow0, iRow1, iMin;

		// if members have not been set, then this is the
		// first time through, so set them to create ratio
		if( !m_iCur0 && !m_iCur1 )
		{
			m_wndSplitter.GetRowInfo( 0, iRow0, iMin );
			m_wndSplitter.GetRowInfo( 1, iRow1, iMin );
			m_iCur0 = iRow0;
			m_iCur1 = iRow1;
		}
		else
		{
			m_wndSplitter.GetRowInfo( 0, iRow0, iMin );
			m_wndSplitter.GetRowInfo( 1, iRow1, iMin );
			// set fixed relationship in pane size
			int iNewRow0 = (cy - rBar.Height()) / 3;
			int iNewRow1 = iNewRow0 * 2;
			m_wndSplitter.SetRowInfo( 0, iNewRow0, iMin );
			m_wndSplitter.SetRowInfo( 1, iNewRow1, iMin );
			m_wndSplitter.RecalcLayout();

			// BUGBUG try to get the new size
			// this does not work because the new size is
			// only available for one pane
#if 0
			// calculate new ideal row size
			int iNewRow = (cy - rBar.Height());// / 3;
			iMin = m_iCur0 + m_iCur1;
			iRow0 = (iNewRow * m_iCur0) / iMin;
			iRow1 = iNewRow - iRow0;
			m_iCur0 = iRow0;
			m_iCur1 = iRow1;

			m_wndSplitter.SetRowInfo( 0, m_iCur0, m_iMinPaneHeight );
			m_wndSplitter.SetRowInfo( 1, m_iCur1, m_iMinPaneHeight );
			m_wndSplitter.RecalcLayout();
#endif
		}
	}
}

void CChatWnd::OnClose()
{
	CFrameWnd::OnClose();
}



/////////////////////////////////////////////////////////////////////////////
// CChatView

CChatView::CChatView()
{
}

CChatView::~CChatView()
{
}

BEGIN_MESSAGE_MAP(CChatView, CEditView)
	//{{AFX_MSG_MAP(CChatView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CChatView drawing

void CChatView::OnDraw(CDC* )
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CChatView diagnostics

#ifdef _DEBUG
void CChatView::AssertValid() const
{
	CEditView::AssertValid();
}

void CChatView::Dump(CDumpContext& dc) const
{
	CEditView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChatView message handlers

BOOL CChatView::PreCreateWindow(CREATESTRUCT& cs) 
{
	BOOL ret = CEditView::PreCreateWindow(cs);
	cs.style = AFX_WS_DEFAULT_VIEW | //ES_AUTOHSCROLL |
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY;
	return ret;
}

/////////////////////////////////////////////////////////////////////////////
// CSendView

CSendView::CSendView()
{

	m_iTo = m_iFrom = theGame.GetMe()->GetPlyrNum ();
	m_pParent = NULL;
}

CSendView::~CSendView()
{
}

void CSendView::SetParent( CWnd *pParent )
{
	m_pParent = pParent;
}

void CSendView::SetPlayers( int iFrom, int iTo )
{
	m_iTo = iTo;
	m_iFrom = iFrom;
}
void CSendView::GetPlayers( int *piFrom, int *piTo )
{
	*piFrom = m_iFrom;
	*piTo = m_iTo;
}

BEGIN_MESSAGE_MAP(CSendView, CEditView)
	//{{AFX_MSG_MAP(CSendView)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//ON_EN_CHANGE(AFX_IDW_PANE_FIRST,OnChange)
	//ON_EN_CHANGE(AFX_IDW_PANE_LAST,OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//ON_WM_CHAR()

/////////////////////////////////////////////////////////////////////////////
// CSendView drawing

void CSendView::OnDraw(CDC*)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CSendView diagnostics

#ifdef _DEBUG
void CSendView::AssertValid() const
{
	CEditView::AssertValid();
}

void CSendView::Dump(CDumpContext& dc) const
{
	CEditView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSendView message handlers

BOOL CSendView::PreCreateWindow(CREATESTRUCT& cs) 
{

	BOOL ret = CEditView::PreCreateWindow(cs);
	cs.style = AFX_WS_DEFAULT_VIEW | WS_VSCROLL |
		ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL;
	return (ret);
}

#if 0
// BUGBUG this is the routine that sends a 
// control value to indicate where to begin
// appending the new part of the message
//
void CSendView::OnChange( void )
{
	if( !m_iTo )
	{
		MessageBox( "Select a player to chat with",
			"Chat Session Error", 
			MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK );
		return;
	}

	// now send the string to the chatting player
	CMsgIPC *pMsg;
	try
	{
		pMsg = new CMsgIPC( CMsgIPC::chat );
	}
	catch (...)
	{
		return;
	}

	pMsg->m_iTo = m_iTo;
	pMsg->m_iFrom = m_iFrom;
	pMsg->m_iLen = 1;
	pMsg->m_sSubject = "wants to chat?";
}
#endif

// BUGBUG this is the routine that processes chat
// when the player decides to send the text via 
// pressing RETURN
//
#if 0
void CSendView::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if( (nChar != VK_RETURN) || (nRepCnt != 1))
	{
		CEditView::OnChar(nChar,nRepCnt,nFlags);
		return;
	}
	else
	{
		if( !m_iTo )
		{
			MessageBox( "Select a player to chat with",
				"Chat Session Error", 
				MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK );
			return;
		}

		CString sChatOut;
		GetEditCtrl().GetWindowText( sChatOut );

		// is this the first message sent?
		if( m_sLastSend.IsEmpty() )
		{
			m_sLastSend = sChatOut + "\r\n";
			sChatOut.Empty();
			sChatOut = m_sLastSend;
		}
		else
		{
			// no, then we need to strip off what
			// was sent before
			int iAt = sChatOut.Find( m_sLastSend );
			if( iAt == -1 ) // not found
			{
				m_sLastSend.Empty();
				m_sLastSend = sChatOut + "\r\n";
			}
			else
			{
				// now iAt is the index in sChatOut
				// where m_sLastSend was found so 
				// adding the length of m_sLastSend
				// should give the beginning of that
				// part of sChatOut not sent
				iAt += m_sLastSend.GetLength();
				m_sLastSend.Empty();
				// so extract that part of sChatOut
				// not sent yet
				m_sLastSend = sChatOut.Right(iAt) + "\r\n";
				sChatOut.Empty();
				sChatOut = m_sLastSend;
			}
		}

		// now send the string to the chatting player
		CMsgIPC *pMsg;
		try
		{
			pMsg = new CMsgIPC( CMsgIPC::chat );
		}
		catch (...)
		{
			return;
		}

		pMsg->m_iTo = m_iTo;
		pMsg->m_iFrom = m_iFrom;
		pMsg->m_iLen = sChatOut.GetLength();
		pMsg->m_sSubject = "wants to chat?";
		pMsg->m_sMessage = sChatOut;

		// send message (with IPC message pointer) to parent
		m_pParent->PostMessage( IPC_CHAT_OUT,
			NULL, (LPARAM)pMsg );

		CEditView::OnChar(nChar,nRepCnt,nFlags);
		//sChatOut.Empty();
		//GetEditCtrl().SetWindowText( sChatOut );
	}
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CMySplitterWnd

CMySplitterWnd::CMySplitterWnd()
{
	CSplitterWnd();
}

CMySplitterWnd::~CMySplitterWnd()
{
}

BEGIN_MESSAGE_MAP(CMySplitterWnd, CSplitterWnd)
	//ON_COMMAND( EN_CHANGE, OnChange )
	//ON_EN_CHANGE(AFX_IDW_PANE_FIRST,OnChange)
	//ON_EN_CHANGE(AFX_IDW_PANE_LAST,OnChange)
END_MESSAGE_MAP()

BOOL CMySplitterWnd::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	BOOL bDoIt = FALSE;

	// trap the notification message the hard way
	if( HIWORD(wParam) == EN_CHANGE )
	{
		OnChange();
		return TRUE;
	}
	return CSplitterWnd::OnCommand(wParam, lParam);
}

//
// handles EN_CHANGE notification from splitter window?
//
void CMySplitterWnd::OnChange()
{
	CSendView *pSendView = (CSendView *)GetPane(1,0);
	if( pSendView == NULL )
		return;

	// now send the string to the chatting player
	CMsgIPC msg ( CMsgIPC::chat );

	// get current string from send out pane
	CString sOld;
	pSendView->GetEditCtrl().GetWindowText( sOld );

	int iTo, iFrom;
	pSendView->GetPlayers( &iFrom, &iTo );

	msg.m_iTo = iTo;
	msg.m_iFrom = iFrom;
	msg.m_iLen = 1;
	
	// test for exceeding maximum chat history size
	if( sOld.GetLength() >= iMaxChatHistory )
	{
#if 0
		// extract last 10% of history
		CString sSent = sOld.Right( iMaxChatHistory/10 );
#endif
		// get the length of the oldest line entered
		int iLen = pSendView->GetEditCtrl().LineLength(0);
		int iOld = sOld.GetLength() - iLen;

		CString sSent = sOld.Right( iOld );
		m_sLastSend.Empty();
		m_sLastSend = sSent;
		int i = sSent.GetLength();

		// reset the control to the 10% we are keeping
		//pSendView->GetEditCtrl().SetSel( 0, i, TRUE );
		//pSendView->GetEditCtrl().Clear();
		pSendView->GetEditCtrl().SetWindowText( sSent );
		pSendView->GetEditCtrl().SetSel( i, i, FALSE );
		

		// replace incoming chat edit control entirely
		msg.m_iCC = 0;
		msg.m_sMessage = sSent;
		sSent.Empty();

		// send it and return
		GetParent()->PostMessage( IPC_CHAT_OUT, NULL, (LPARAM) &msg );
		return;
	}

	// compare sOld with sLastSend and find index of
	// where difference begins, then save it in m_iCC
	// and the string from that point on in m_sMessage
	// and update sLastSend in the same way as chat in will
	if( m_sLastSend.IsEmpty() ||
		sOld.GetLength() < m_sLastSend.GetLength() )
	{
		msg.m_iCC = 0;
		msg.m_sMessage = sOld;
		m_sLastSend = sOld;
	}
	else
	{
		// process looking for a change
		for( int i=0; i<m_sLastSend.GetLength(); ++i )
		{
			if( m_sLastSend[i] != sOld[i] )
			{
				msg.m_iCC = i;
				// extract the remaining part of the current text
				msg.m_sMessage = sOld.Right( (sOld.GetLength()-i) );
				break;
			}
		}
		// update m_sLastSend
		if( i < sOld.GetLength() )
		{
			if( msg.m_iCC != i )
			{
				msg.m_iCC = i;
				msg.m_sMessage = sOld.Right( (sOld.GetLength()-i) );
			}

			CString sSent = m_sLastSend.Left(i);
			m_sLastSend.Empty();
			m_sLastSend = sSent + msg.m_sMessage;
			sSent.Empty();
		}
		else	// send no message
		{
			return; 
		}
	}

	// BUGBUG - this sucks - we could send just the insert location & char or
	// delete location & length - but no time to fix
	if ( msg.m_sMessage.GetLength () > VP_MAXSENDDATA-sizeof (msg) )
		{
		AfxMessageBox ( IDS_MAX_CHAT, MB_OK );
		return;
		}

	// the way to send a message?
	msg.PostToClient ();
}

// end of IPCChat.cpp
