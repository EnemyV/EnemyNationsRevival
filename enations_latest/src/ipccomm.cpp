//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////
//
//  IPCComm.cpp : IPC Main Window objects
//                Divide and Conquer
//               
//  Last update:    10/03/96
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "lastplnt.h"
#include "player.h"
#include "IPCComm.h"
#include "IPCPlay.h"
#include "bitmaps.h"
#include "toolbar.h"
#include "error.h"
#include "event.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

// do we tell when mail comes in
BOOL bTellMail = TRUE;

// list of email messages
CEMsgList *plEmailMsgs;	
// list of players specific to IPC
CIPCPlayerList *plIPCPlayers = NULL;
// one player status dialog
CPlyrMsgStatusDlg *pPlayerDlg = NULL; 

static UINT buttons[] =
{
	// same order as in the bitmap 'toolbar.bmp'
	IDS_MAIL_READ,		// 26
	IDS_MAIL_SEND,		// 36
	IDS_MAIL_REPLY,		// 33
	IDS_MAIL_FORWARD,	// 15
	IDS_MAIL_DELETE,	// 11
	IDS_MAIL_REFUSE,	// 27
		ID_SEPARATOR,
	IDS_MAIL_CHAT,		// 46
	IDS_MAIL_CHAT_ANS,	// 7
		ID_SEPARATOR,
	IDS_MAIL_VOICE,
	IDS_MAIL_VOICE_ANS,
		ID_SEPARATOR,
	IDS_MAIL_OPTIONS	// 25
};

void CWndComm::UpdateMail ()
{

	if ( ! theGame.HaveHP () )
		return;
		
	if ( theApp.m_wndChat.m_hWnd == NULL )
		return;

	// can we create?
	theApp.m_wndChat.EnableButton( IDS_MAIL_SEND, theGame.GetMe ()->CanEMail () );
	theApp.m_wndChat.EnableButton( IDS_MAIL_CHAT, theGame.GetMe ()->CanChat () );
}

/////////////////////////////////////////////////////////////////////////////
// CToolBarCtrl create

int CreateToolBarCtrl (CToolBarCtrl & tool, int iCtrlID, int iBmID, UINT const * pBtnID, int iNumBtns, CWnd * pPar)
{

	CRect rect (0, 0, 0, 0);
	tool.Create (WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS, rect, pPar, iCtrlID);
	tool.SetOwner (pPar);
	tool.SetButtonStructSize (sizeof (TBBUTTON));
	tool.SetButtonSize (15);
	tool.AddBitmap (10, iBmID);
	TBBUTTON tbb;
	memset (&tbb, 0, sizeof (tbb));
	tbb.fsState = TBSTATE_ENABLED;

	for (int iOn=0, iBtn=0; iOn<iNumBtns; iOn++, pBtnID++)
		{
		if (*pBtnID == ID_SEPARATOR)
			{
			tbb.fsStyle = TBSTYLE_SEP;
			tbb.idCommand = 0;
			tbb.iBitmap = 0;
			tbb.iString = 0;
			}
		else
		  {
			tbb.fsStyle = TBSTYLE_BUTTON;
			tbb.idCommand = *pBtnID;
			tbb.iBitmap = iBtn;
			tbb.iString = iBtn;
			iBtn++;
		  }
		tool.AddButtons (1, &tbb);
		}

	tool.GetItemRect (iNumBtns - 1, &rect);
	return (rect.right);
}


#if EN_CONTROLS
/////////////////////////////////////////////////////////////////////////////
// CMyHeaderCtrl

BEGIN_MESSAGE_MAP(CMyHeaderCtrl, CHeaderCtrl)
	//{{AFX_MSG_MAP(CMyHeaderCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CMyHeaderCtrl::Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID )
{
	return( CHeaderCtrl::Create(dwStyle, rect, pParentWnd, nID) );
}

void CMyHeaderCtrl::DrawItem (LPDRAWITEMSTRUCT lpDIS)
{
	CRect rect (lpDIS->rcItem);

	// get dc to draw on
	CDC dc; 
	dc.Attach( lpDIS->hDC );

	// Geoff and Dave's mystery drawing stuff
	thePal.Paint (dc.m_hDC);
	theBitmaps.GetByIndex (DIB_TOOLBAR)->Tile (dc, rect);

	DWORD dwOldText = dc.SetTextColor( GetSysColor(COLOR_WINDOWTEXT) );
	int iOldMode = dc.SetBkMode( TRANSPARENT );

	int iX = lpDIS->rcItem.left + 1;
	int iY = lpDIS->rcItem.top + 1;

	// set up the header item flags to get string for this item
	/* this is not working for getting the string
	HD_ITEM hi;
	memset (&hi, 0, sizeof (hi));
	hi.mask = HDI_FORMAT | HDI_TEXT;
	hi.fmt = HDF_STRING | HDF_LEFT;

	BOOL bSuccess = GetItem( lpDIS->itemID, &hi );
	CString sName( hi.pszText );
	*/

	// try brute force
	CString sName;
	switch( lpDIS->itemID )
	{
	case 1:
		sName = "From";
		break;
	case 2:
		sName = "Subject";
		break;
	default:
	case 0:
		sName = "";
		break;
	}
	

	dc.TextOut( iX, iY, sName, sName.GetLength() );

	dc.SetTextColor( dwOldText );
	dc.SetBkMode( iOldMode );

	thePal.EndPaint (dc.m_hDC);
}

// turn off status bar completely
/*
/////////////////////////////////////////////////////////////////////////////
// CMyStatusBarCtrl

BEGIN_MESSAGE_MAP(CMyStatusBarCtrl, CStatusBarCtrl)
	//{{AFX_MSG_MAP(CMyStatusBarCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CMyStatusBarCtrl::Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID )
{
	return( CStatusBarCtrl::Create(dwStyle, rect, pParentWnd, nID) );
}

void CMyStatusBarCtrl::DrawItem (LPDRAWITEMSTRUCT lpDIS)
{
	CRect rect (lpDIS->rcItem);

	// get dc to draw on
	CDC dc; 
	dc.Attach( lpDIS->hDC );

	// Geoff and Dave's mystery drawing stuff
	thePal.Paint (dc.m_hDC);
	theBitmaps.GetByIndex (DIB_TOOLBAR)->Tile (dc, rect);

	DWORD dwOldText = dc.SetTextColor( GetSysColor(COLOR_WINDOWTEXT) );
	int iOldMode = dc.SetBkMode( TRANSPARENT );

	int iX = lpDIS->rcItem.left + 1;
	int iY = lpDIS->rcItem.top + 1;

	int iType;
	char szText[80];
	GetText( szText, lpDIS->itemID, &iType );
	CString sName( szText );

	dc.TextOut( iX, iY, sName, sName.GetLength() );

	dc.SetTextColor( dwOldText );
	dc.SetBkMode( iOldMode );
}
*/

#endif

/////////////////////////////////////////////////////////////////////////////
// CWndComm

#if NEW_BUTTONS

const int CWndComm::aCommID[NUM_COMM_BTNS] = {
				IDS_MAIL_READ,
				IDS_MAIL_SEND,
				IDS_MAIL_REPLY,
				IDS_MAIL_FORWARD,
				IDS_MAIL_DELETE,
				IDS_MAIL_REFUSE,
				IDS_MAIL_CHAT,
				IDS_MAIL_CHAT_ANS,
				IDS_MAIL_OPTIONS };

const int CWndComm::aCommBtn[NUM_COMM_BTNS] = {
				28,
				38,
				35,
				16,
				12,
				29,
				49,
				8,
				27 };

const int CWndComm::aCommHelp[NUM_COMM_BTNS] = {
				IDH_MAIL_READ,
				IDH_MAIL_SEND,
				IDH_MAIL_REPLY,
				IDH_MAIL_FORWARD,
				IDH_MAIL_DELETE,
				IDH_MAIL_REFUSE,
				IDH_MAIL_CHAT,
				IDH_MAIL_CHAT_ANS,
				IDH_MAIL_OPTIONS };

int CWndComm::CreateButtons()
{
	// create a rect for the first button
	CRect rect( BAR_BTN_X_SKIP, BAR_BTN_Y_START,
		BAR_BTN_X_SKIP + theBmBtnData.Width (), 
		BAR_BTN_Y_START + theBmBtnData.Height ());

	// create the buttons, and move the rect along to right
	CBmButton * pBtn = m_BmBtns;
	for( int iOn=0; iOn<NUM_COMM_BTNS; iOn++, pBtn++ )
	{
		pBtn->Create( aCommBtn[iOn], aCommHelp[iOn], &theBmBtnData, rect, 
			theBitmaps.GetByIndex( DIB_TOOLBAR ), this, aCommID[iOn] );
		rect.OffsetRect( theBmBtnData.Width () + BAR_BTN_X_SKIP, 0 );
	}
	rect.right += BAR_BTN_X_SKIP;

	// pass back the right side?
	return( rect.right );
}

void CWndComm::OnPaint() 
{

	CPaintDC dc(this); // device context for painting
	thePal.Paint (dc.m_hDC);
	
	CRect rect;
	CWndBase::GetClientRect (&rect);

	theBitmaps.GetByIndex (DIB_TOOLBAR)->Tile (dc, rect);

	thePal.EndPaint (dc.m_hDC);
	// Do not call CWndAnim::OnPaint() for painting messages
}

void CWndComm::EnableButton (int ID, BOOL bEnable)
{

	CBmButton *pBtn = (CBmButton *) GetDlgItem (ID);
	ASSERT_VALID (pBtn);
	if (pBtn)
		pBtn->EnableWindow (bEnable);
}

#endif

CWndComm::CWndComm()
{
	m_xMin = 12 * 16;
	m_yMin = HIWORD (GetDialogBaseUnits ()) * 6;
	m_lSendWndSize = 0L;
}

CWndComm::~CWndComm()
{

	delete plEmailMsgs;
	delete m_plbEMail;
	delete plIPCPlayers;
}


void CWndComm::IncomingMessage( CMsgIPC *pMsg )
{

	// set up a real CMsgIPC
	CMsgIPC msg (pMsg->m_iType);
	msg.m_iTo = pMsg->m_iTo;
	msg.m_iFrom = pMsg->m_iFrom;
	msg.m_iCC = pMsg->m_iCC;
	msg.m_iLen = pMsg->m_iLen;

	pMsg++;
	msg.m_sMessage = (char *) pMsg;
	msg.m_sSubject = ((char *) pMsg) + msg.m_sMessage.GetLength () + 1;

	// not really our message so don't process it
	if( msg.m_iTo != m_iPlayer )
		return;

	switch( msg.m_iType )
	{
		case CMsgIPC::email:
		// create a CEMsg based on data from incoming message
		// and add to message list and tell CCommWnd about it
			ProcessEmail(&msg);
			break;
		case CMsgIPC::chat:
		// tell CCommWnd about incoming chat message
			ProcessIncomingChat(&msg);
			break;
		case CMsgIPC::voice:
		// not yet implemented
		default:
			break;
	}
}

//
// process chat type messages received by this player
//
void CWndComm::ProcessIncomingChat( CMsgIPC *pMsg )
{

	// check our status setting for this type of message, this player
	CIPCPlayer *pPlayer = plIPCPlayers->GetPlayer( (WORD)pMsg->m_iFrom );
	CPlayer *pPlyr = theGame.GetPlayerByPlyr( pMsg->m_iFrom );
	if ((pPlayer == NULL) || (pPlyr == NULL))
		{
		ASSERT (FALSE);
		return;
		}
	if( !(pPlayer->m_wStatus & IPC_ACCEPT_CHAT) )
		{
		TRAP ();
		return;
		}

	// check list of CEMsgs for one with this
	// 'to' player and if found, then transfer
	// this message to it and call the window
	// of that message

	if (pPlayer->m_pwndChat != NULL )
	{
		// if so, then activate the already open window
		pPlayer->m_pwndChat->ShowWindow(SW_SHOW);
		// don't want to give it the keyboard		pPlayer->m_pwndChat->SetFocus();
		pPlayer->m_pwndChat->OnChatIn (pMsg);
		return;
	}

	// otherwise create a chat window
	// get parent location
	CRect rect;
	GetClientRect (&rect);

	CChatWnd *pWnd = new CChatWnd();

	// set who we are talking to
	pWnd->m_iTo = pPlyr->GetPlyrNum();

	// annouce incoming chat request in status window
	// get name of player of message and put in title
	CString sStatus;
	sStatus.LoadString (IDS_CHAT_STATUS);
	csPrintf (&sStatus, (const char *) pPlyr->GetName ());

	// turn off status bar completely
	/*
#if EN_CONTROLS
	m_status.SetText ( sStatus, 255, SBT_OWNERDRAW );
#else
	m_status.SetText ( sStatus, 255, 0 );
#endif
	*/
	

	CString sTitle;
	sTitle.LoadString (IDS_CHAT_TITLE);
	csPrintf (&sTitle, (const char *) pPlyr->GetName ());
	pWnd->Create (NULL, sTitle, dwPopWndStyle, rect, this);

	pWnd->SetFrom (pPlyr);
	pWnd->ShowWindow (SW_SHOW);
	pWnd->UpdateWindow ();

	theGame.Event (EVENT_HAVE_CALL, EVENT_NOTIFY);

	pWnd->OnChatIn (pMsg);
}

//
// process email type messages received by this player
//
void CWndComm::ProcessEmail( CMsgIPC *pMsg )
{

	// check our status setting for this type of message, this player
	CIPCPlayer *pPlayer = plIPCPlayers->GetPlayer( (WORD)pMsg->m_iFrom );
	if( pPlayer == NULL )
		return;
	if( !(pPlayer->m_wStatus & IPC_ACCEPT_EMAIL) )
		return;

	// CEMsg( int iID, int iFrom, int iTo, 
	// const char *pzSubject, const char *pzMessage );
	CEMsg *pEMsg = NULL;
	try
	{
		pEMsg = new CEMsg( 
		(plEmailMsgs->m_iLastID+1), 
		pMsg->m_iFrom, pMsg->m_iTo,
		(const char *)pMsg->m_sSubject,
		(const char *)pMsg->m_sMessage );

		plEmailMsgs->AddTail( (CObject *)pEMsg );
	}
	catch (...)
	{
		if( pEMsg != NULL )
			delete pEMsg;

		return;
	}

	// set flag to indicate email
	pEMsg->m_wStatus |= MSG_IS_EMAIL;

	if( pEMsg->m_iID < 32000 )
		plEmailMsgs->m_iLastID = pEMsg->m_iID;
	else
		plEmailMsgs->m_iLastID = 0;

	int iRet = m_plbEMail->AddString( (LPCSTR)pEMsg->m_iID );
	// save the pointer to the CEMsg in the item
	if( iRet != LB_ERR &&
		iRet != LB_ERRSPACE )
		m_plbEMail->SetItemDataPtr( iRet, (void*)pEMsg );

	//m_status.SetText ("status line", 255, 0);
	if (bTellMail)
		{
		//bTellMail = FALSE;
		theGame.Event (EVENT_HAVE_MAIL, EVENT_NOTIFY);
		}
/*
#if EN_CONTROLS
	m_status.SetText( "INCOMING MESSAGE", 255, SBT_OWNERDRAW );
#else
	m_status.SetText( "INCOMING MESSAGE", 255, 0 );
#endif
*/
}

// afx_msg void OnKillFocus( CWnd* pNewWnd );
  

BEGIN_MESSAGE_MAP(CWndComm, CWndBase)
	//{{AFX_MSG_MAP(CWndComm)
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_WM_SYSCOMMAND()
	ON_NOTIFY (TTN_NEEDTEXT, IDT_MAIL, TipText)
	ON_NOTIFY (HDN_ITEMCHANGED, IDHC_MAIL, HdrChange)
	ON_WM_GETMINMAXINFO()
	ON_LBN_SELCHANGE (101, OnLbnChange)
	ON_LBN_DBLCLK (101, OnLMBDC )
	ON_COMMAND (IDS_MAIL_READ, OnRead)
	ON_COMMAND (IDS_MAIL_SEND, OnSend)
	ON_COMMAND (IDS_MAIL_REPLY, OnReply)
	ON_COMMAND (IDS_MAIL_FORWARD, OnForward)
	ON_COMMAND (IDS_MAIL_DELETE, OnDelete)
	ON_COMMAND (IDS_MAIL_REFUSE, OnRefuse)
	ON_COMMAND (IDS_MAIL_CHAT, OnChat)
	ON_COMMAND (IDS_MAIL_CHAT_ANS, OnGlobalChat)
	ON_COMMAND (IDS_MAIL_OPTIONS, OnPlayerStatus)
	ON_MESSAGE( IPC_MESSAGE_ARRIVED, OnEmailArrived)
	ON_MESSAGE( IPC_MESSAGE_UPDATE, OnEmailUpdate)
	ON_MESSAGE( IPC_DESTROY_READWND, OnDestroyEmailWnd)
	ON_MESSAGE( IPC_EMAIL_REPLY, OnReplyEmailWnd)
	ON_MESSAGE( IPC_EMAIL_FORWARD, OnForwardEmail)
	ON_MESSAGE( IPC_EMAIL_DELETE, OnDeleteEmail)
	ON_MESSAGE( IPC_EMAIL_SCROLL, OnScrollEmail)
	ON_MESSAGE( IPC_CREATE_CHAT, OnCreateChatMsg)
	ON_MESSAGE( IPC_SENDWND_SIZE, OnSendWndSize)
	ON_MESSAGE( IPC_UNHIDE_WINDOW, OnUnHideWindow)
	ON_MESSAGE (WM_BUTTONMOUSEMOVE, OnButtonMouseMove)
	ON_WM_PARENTNOTIFY()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndComm message handlers

LRESULT CWndComm::OnButtonMouseMove (WPARAM, LPARAM lParam)
{

	theApp.m_wndBar.SetStatusText (1, (char *) lParam);
	return (0);
}

LONG CWndComm::OnSendWndSize( UINT, LONG lParam )
{
	m_lSendWndSize = lParam;
	return TRUE;
}

LONG CWndComm::OnCreateChatMsg( UINT uParam, LONG lParam )
{
	// CEMsg( int iID, int iFrom, int iTo, 
	// const char *pzSubject, const char *pzMessage );
	CEMsg *pEMsg = NULL;
	try
	{
		pEMsg = new CEMsg( 
		(plEmailMsgs->m_iLastID+1), 
		LOWORD(lParam), HIWORD(lParam),
		"", "" );

		plEmailMsgs->AddTail( (CObject *)pEMsg );
	}
	catch (...)
	{
		if( pEMsg != NULL )
			delete pEMsg;

		return FALSE;
	}

	// set flag to indicate email
	pEMsg->m_wStatus |= MSG_IS_CHAT;
	// now record open chat window about this message
	pEMsg->m_pOpenWnd  = (CWnd *)uParam;

	if( pEMsg->m_iID < 32000 )
		plEmailMsgs->m_iLastID = pEMsg->m_iID;
	else
		plEmailMsgs->m_iLastID = 0;

	// cause the message list box to update its display
	m_plbEMail->LoadEmail();

	return TRUE; // I.. dont know??
}

LONG CWndComm::OnReplyEmailWnd( UINT uParam, LONG )
{
	if( plEmailMsgs != NULL )
	{
		ASSERT_VALID (plEmailMsgs);

		CEMsg *pMsg = plEmailMsgs->GetMessage( LOWORD( uParam ) );
		if( pMsg != NULL )
		{
			ASSERT_VALID( pMsg );

			// want to reply to this message so 
			// create a message
			CEMsg *pRepyMsg = NULL;
			// reply message TO is this message FROM
			// reply message SUBJECT is this message SUBJECT
			try
			{
				//CEMsg( int iID, int iFrom, int iTo, 
				//const char *pzSubject, const char *pzMessage );
				pRepyMsg = new CEMsg( 
					(plEmailMsgs->m_iLastID+1), 
					//pMsg->m_iFrom, pMsg->m_iTo,
					pMsg->m_iTo, pMsg->m_iFrom,
					pMsg->m_sSubject, "" );
				
				plEmailMsgs->AddTail( (CObject *)pRepyMsg );
			}
			catch (...)
			{
				if( pRepyMsg != NULL )
					delete pRepyMsg;

				return( FALSE );
			}

			if( pRepyMsg->m_iID < 32000 )
				plEmailMsgs->m_iLastID = pRepyMsg->m_iID;
			else
				plEmailMsgs->m_iLastID = 0;

			// open a send window with this message
			CDlgCompose *pDlg = new CDlgCompose( NULL, pRepyMsg, m_lSendWndSize );
			pRepyMsg->m_pOpenWnd = (CWnd *)pDlg; // NULL; 

			pDlg->Create (CDlgCompose::IDD, this);
		}
	}
	return TRUE;
}

LONG CWndComm::OnForwardEmail( UINT uParam, LONG )
{
	if( plEmailMsgs != NULL )
	{
		ASSERT_VALID (plEmailMsgs);

		CEMsg *pMsg = plEmailMsgs->GetMessage( LOWORD( uParam ) );
		if( pMsg != NULL )
		{
			ASSERT_VALID( pMsg );

			pMsg->m_iTo = 0;
			
			// open a send window to forward this message
			CDlgCompose *pDlg = new CDlgCompose( NULL, pMsg, m_lSendWndSize );
			pMsg->m_pOpenWnd = (CWnd *)pDlg; 
			pDlg->Create (CDlgCompose::IDD, this);
		}
	}
	return TRUE;
}

LONG CWndComm::OnDeleteEmail( UINT uParam, LONG )
{
	if( plEmailMsgs != NULL )
	{
		ASSERT_VALID (plEmailMsgs);
		plEmailMsgs->RemoveMessage( LOWORD( uParam ) );

		// cause the message list box to update its display
		m_plbEMail->LoadEmail();

		// if count of email messages is 0, then turn off MAIL buttons
		if( !m_plbEMail->GetCount() ||
			m_plbEMail->GetCount() == LB_ERR )
		{
#if NEW_BUTTONS
			EnableButton( IDS_MAIL_READ, FALSE);
			EnableButton( IDS_MAIL_REPLY, FALSE);
			EnableButton( IDS_MAIL_FORWARD, FALSE);
			EnableButton( IDS_MAIL_DELETE, FALSE);
			EnableButton( IDS_MAIL_REFUSE, FALSE);
#endif
		}

	}
	return TRUE;
}

LONG CWndComm::OnScrollEmail( UINT uParam, LONG lParam )
{
	int iID = (int)LOWORD( uParam );
	int iCmd = (int)lParam;

	if( plEmailMsgs != NULL )
	{
		ASSERT_VALID (plEmailMsgs);

		CEMsg *pMsg = plEmailMsgs->GetMessage( iID );
		if( pMsg != NULL )
		{
			ASSERT_VALID( pMsg );

			CWndMailRead *pReadWnd = (CWndMailRead *)pMsg->m_pOpenWnd;
			if( pReadWnd != NULL )
			{
				pMsg = plEmailMsgs->GetNextPrevMsg( iID, iCmd );
				if( pMsg != NULL )
					pReadWnd->SetNewMessage( pMsg );
			}
		}
	}
	return TRUE;
}

LONG CWndComm::OnEmailUpdate( UINT, LONG )
{
	// cause the message list box to update its display
	m_plbEMail->LoadEmail();
	return TRUE;
}

LONG CWndComm::OnUnHideWindow( UINT uParam, LONG lParam )
{
	CRect rect( LOWORD( uParam ), HIWORD( uParam ),
		LOWORD( lParam ), HIWORD( lParam ) );

	MoveWindow( &rect, TRUE );
	return TRUE;
}

//
// uses fake network to make like a message came in
//
LONG CWndComm::OnEmailArrived( UINT, LONG )
{
#if USE_FAKE_NET
	if( plFakeNetwork != NULL && 
		!plFakeNetwork->IsEmpty() )
	{
		CMsgIPC *pNetMsg = (CMsgIPC *)plFakeNetwork->RemoveHead();
		IncomingMessage( pNetMsg );
	}
#endif
	return TRUE;
}


//
// remove Read Window indicated by uParam id passed
// from open read window list
//
LONG CWndComm::OnDestroyEmailWnd( UINT uParam, LONG )
{
	ASSERT_VALID( this );
	ASSERT_VALID( plEmailMsgs );
	
	if( plEmailMsgs != NULL )
	{
		ASSERT_VALID (plEmailMsgs);

		CEMsg *pMsg = plEmailMsgs->GetMessage( LOWORD( uParam ) );
		if( pMsg != NULL )
		{
			ASSERT_VALID( pMsg );
			pMsg->m_pOpenWnd = NULL;
		}

		// if all read windows are closed then
		// tool bar needs to be disabled
		if( ReadWndOpen( NULL ) == NULL )
			OnLbnChange();
	}
	return TRUE;
}

void CWndComm::Create ()
{

	CString sTitle;
	sTitle.LoadString (IDS_TITLE_CHAT_WND);
	if (CWndBase::CreateEx (0, theApp.m_sWndCls, sTitle, dwPopWndStyle,
					theApp.GetProfileInt (theApp.m_sResIni, "ChatX", 0),
					theApp.GetProfileInt (theApp.m_sResIni, "ChatY", theApp.m_iRow2),
					theApp.GetProfileInt (theApp.m_sResIni, "ChatEX", theApp.m_iCol1 + 1),
					theApp.GetProfileInt (theApp.m_sResIni, "ChatEY", theApp.m_iRow3 - theApp.m_iRow2 + 1),
					theApp.m_pMainWnd->m_hWnd, NULL) == 0)
		ThrowError (ERR_RES_CREATE_WND);

	// ok, now we DO know the min size. re-size if necessary
	CRect rect;
	GetWindowRect (&rect);
	if ((rect.Width () < m_xMin) || (rect.Height () < m_yMin))
		SetWindowPos (NULL, 0, 0, __max (rect.Width (), m_xMin),
										__max (rect.Height (), m_yMin), SWP_NOMOVE | SWP_NOZORDER);

	m_iPlayer = theGame.GetMe()->GetPlyrNum();
}

int CWndComm::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{

	if (CWndBase::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// create list to save email messages until deleted
	plEmailMsgs = new CEMsgList();

	// initialize the last id used
	plEmailMsgs->m_iLastID = 0;

	// initialize the IPC status of the players
	plIPCPlayers = new CIPCPlayerList();

#if NEW_BUTTONS
	// create new buttons
	m_xMin = CreateButtons();
#else
	// create toolbar
	m_xMin = CreateToolBarCtrl (m_tool, IDT_MAIL, IDB_TOOL_MAIL, buttons,
				sizeof (buttons) / sizeof (UINT), this);
#endif

	// make adjustment for client area
	CRect rect (0, 0, m_xMin, 20);
	AdjustWindowRect (&rect, dwPopWndStyle, FALSE);

	// accumulate minimum width
	m_xMin = rect.Width () + 1;

#if NEW_BUTTONS
	rect.OffsetRect( 0, theBmBtnData.Height() );
#else
	// get toolbar's location
	m_tool.GetClientRect (&rect);
	rect.OffsetRect (0, rect.top);
#endif

	// create the header control
	//m_header.Create (WS_CHILD | WS_VISIBLE, rect, this, IDSB_MAIL);
	m_header.Create(
		WS_CHILD | WS_VISIBLE | CCS_NODIVIDER | HDS_HORZ | CCS_TOP, 
		rect, this, IDSB_MAIL);
	HD_ITEM hi;
	memset (&hi, 0, sizeof (hi));
	hi.mask = HDI_FORMAT | HDI_TEXT | HDI_WIDTH;

#if EN_CONTROLS	
	hi.fmt = HDF_LEFT | HDF_STRING | HDF_OWNERDRAW;
#else
	hi.fmt = HDF_LEFT | HDF_STRING;
#endif

	hi.cxy = rect.Width () / 8;
	hi.pszText = "";
	hi.cchTextMax = sizeof (hi.pszText);
	m_header.InsertItem (0, &hi);
	int iNameStop = hi.cxy;

	hi.cxy = rect.Width () / 2;
	hi.pszText = "From";
	hi.cchTextMax = sizeof (hi.pszText);
	m_header.InsertItem (1, &hi);
	int iSubjectStop = iNameStop + hi.cxy;

	hi.cxy = rect.Width () - rect.Width () / 3;
	hi.pszText = "Subject";
	hi.cchTextMax = sizeof (hi.pszText);
	m_header.InsertItem (2, &hi);

	// create a dummy area for the rest of the control
	hi.cxy = rect.Width() - 1;
	hi.pszText = "";
	hi.cchTextMax = sizeof( hi.pszText );
	m_header.InsertItem( 3, &hi );

	// turn off status bar completely
	/*
	// create status bar
	m_status.Create (WS_CHILD | WS_VISIBLE, rect, this, IDSB_MAIL);
	m_status.SetSimple (TRUE);

	// put something on the status bar
	CString sStatus = "Communication Initialized";
#if EN_CONTROLS
	m_status.SetText ( sStatus, 255, SBT_OWNERDRAW );
#else
	m_status.SetText ( sStatus, 255, 0 );
#endif
	*/


	// create listbox
	GetClientRect (&rect);
	m_plbEMail = new CEMailLB( 101, this, rect );

	// tell listbox about new header sizes
	m_plbEMail->SetStops( iNameStop, iSubjectStop );
	m_plbEMail->LoadEmail();
	
#if NEW_BUTTONS
	m_yMin = BAR_BTN_Y_START + theBmBtnData.Height();
#else
	// determine real min size
	m_tool.GetClientRect (&rect);
	m_yMin = rect.Height ();
#endif
	
	m_header.GetClientRect (&rect);
	m_yMin += rect.Height ();
	//m_yMin += m_list.GetItemHeight (0) * 3;
	m_yMin += m_plbEMail->GetItemHeight (0) * 3;
	
	// turn off status bar completely
#if 0
	m_status.GetClientRect (&rect);
	m_yMin += rect.Height ();
#endif

	rect.bottom = m_yMin;
	AdjustWindowRect (&rect, dwPopWndStyle, FALSE);
	m_yMin = rect.Height () + 1;
	
	// make sure IPC specific players are initialized
	plIPCPlayers->InitPlayers();

	OnLbnChange ();

	// if rebuilding use old pos
	if ( theGame.m_wpChat.length == 0 )
		{
		// save position
		theGame.m_wpChat.length = sizeof (WINDOWPLACEMENT);
		GetWindowPlacement ( &(theGame.m_wpChat) );
		}

	// can we create?
	UpdateMail ();

	return 0;
}

void CWndComm::OnClose() 
{
	// we just hide it	
	ShowWindow (SW_HIDE);
}

void CWndComm::OnSysCommand(UINT nID, LPARAM lParam)
{
	// for minimize hide it
	if ((nID == SC_MINIMIZE) || (nID == SC_CLOSE))
		{
		ShowWindow (SW_HIDE);
		return;
		}

	CWndBase::OnSysCommand(nID, lParam);
}

void CWndComm::OnSize(UINT nType, int cx, int cy) 
{
	CWndBase::OnSize(nType, cx, cy);

	// tell the controls
#if !NEW_BUTTONS
	m_tool.AutoSize ();
#endif

	// turn off status bar completely
	//m_status.SendMessage (WM_SIZE, nType, MAKELONG (cx, cy));


	// header	
	CRect rect, rectBar;
	GetClientRect (&rect);
#if NEW_BUTTONS
	rect.top = BAR_BTN_Y_START + theBmBtnData.Height();
#else
	m_tool.GetClientRect (&rectBar);
	rect.top = rectBar.bottom;
#endif
	// turn off status bar completely
	//m_status.GetClientRect (&rectBar);
	//rect.bottom -= rectBar.Height ();

	HD_LAYOUT hl;
	WINDOWPOS wp;
	memset (&hl, 0, sizeof (hl));
	memset (&wp, 0, sizeof (wp));
	hl.prc = &rect;
	hl.pwpos = &wp;
	m_header.Layout (&hl);
	
	m_header.SetWindowPos (NULL, wp.x, wp.y, wp.cx, wp.cy, SWP_NOZORDER);
	rect.top = wp.y + wp.cy;

	// fit the listbox to the window
	m_plbEMail->SetWindowPos (NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER);
	m_plbEMail->InvalidateRect (NULL);
	
	// tell listbox about new header sizes
	HD_ITEM hi;
	memset (&hi, 0, sizeof (hi));
	hi.mask = HDI_WIDTH;
	m_header.GetItem (0, &hi);

	int iNameStop = hi.cxy;

	m_header.GetItem (1, &hi);
	int iSubjectStop = iNameStop + hi.cxy;

	m_plbEMail->SetStops( iNameStop, iSubjectStop );

	// save position
	theGame.m_wpChat.length = sizeof (WINDOWPLACEMENT);
	GetWindowPlacement ( &(theGame.m_wpChat) );
}

void CWndComm::TipText (NMHDR * pTTT, LRESULT *)
{

	ASSERT_VALID (this);
	//TRAP ();

	((TOOLTIPTEXT *)pTTT)->lpszText = (LPSTR) ((TOOLTIPTEXT *)pTTT)->hdr.idFrom;
	((TOOLTIPTEXT *)pTTT)->hinst = AfxGetResourceHandle ();
}

void CWndComm::HdrChange (NMHDR *, LRESULT *)
{

	ASSERT_VALID (this);

	// tell listbox about new header sizes
	HD_ITEM hi;
	memset (&hi, 0, sizeof (hi));
	hi.mask = HDI_WIDTH;
	m_header.GetItem (0, &hi);

	int iNameStop = hi.cxy;

	m_header.GetItem (1, &hi);
	int iSubjectStop = iNameStop + hi.cxy;

	m_plbEMail->SetStops( iNameStop, iSubjectStop );
	m_plbEMail->LoadEmail();
}

void CWndComm::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{

	ASSERT_VALID (this);

	if (lpMMI->ptMinTrackSize.x < m_xMin)
		lpMMI->ptMinTrackSize.x = m_xMin;

	if (lpMMI->ptMinTrackSize.y < m_yMin)
		lpMMI->ptMinTrackSize.y = m_yMin;

/*
typedef struct tagWINDOWPLACEMENT {
    UINT  length;
    UINT  flags;
    UINT  showCmd;
    POINT ptMinPosition;
    POINT ptMaxPosition;
    RECT  rcNormalPosition;
} WINDOWPLACEMENT;
*/

	/*
	WINDOWPLACEMENT wp;
	BOOL bRet = GetWindowPlacement( &wp );
	int ix = 0, iy = 0;
	if( bRet )
	{
		if( wp.showCmd == SW_SHOWMINIMIZED )
		{
			ix = wp.ptMinPosition.x;
			iy = wp.ptMinPosition.y;
		}
		else if( wp.showCmd == SW_SHOWMAXIMIZED )
		{
			ix = wp.ptMaxPosition.x;
			iy = wp.ptMaxPosition.y;
		}
		else if( wp.showCmd == SW_SHOWNORMAL )
		{
			ix = wp.rcNormalPosition.left;
			iy = wp.rcNormalPosition.top;
		}	
	}
	*/

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

void CWndComm::OnMove( int x, int y )
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

	CWndBase::OnMove(x, y);
}

#if 0
void CWndComm::OnKillFocus( CWnd* )
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

void CWndComm::OnLMBDC()
{
	ASSERT_VALID (this);
	
	int iRet = m_plbEMail->GetCurSel();
	if( iRet == LB_ERR )
		return;

	// get selected message
	CEMsg *pMsg = (CEMsg *)m_plbEMail->GetItemDataPtr(iRet);
	if( pMsg == NULL )
		return;

	if( (pMsg->m_wStatus & MSG_IS_CHAT) )
		OnChat();
	else if( (pMsg->m_wStatus & MSG_IS_EMAIL) )
		OnRead();
	else
		return;
}

void CWndComm::OnRead ()
{
	ASSERT_VALID (this);
	
	int iRet = m_plbEMail->GetCurSel();
	if( iRet == LB_ERR )
		return;

	// get selected message
	CEMsg *pMsg = (CEMsg *)m_plbEMail->GetItemDataPtr(iRet);
	if( pMsg == NULL )
		return;

	// check all mail read windows to see if a window
	// with this message is already open
	CWndMailRead *pReadWnd = NULL;
	if( (pReadWnd = (CWndMailRead *)ReadWndOpen( pMsg ))
		!= NULL )
	{
		// if so, then activate the already open window
		pReadWnd->ShowWindow(SW_RESTORE);
		pReadWnd->SetFocus();
		return;
	}
	// if not, then open a new read window with this 
	// message and add it to the open read window list
	pReadWnd = new CWndMailRead( pMsg );
	pMsg->m_pOpenWnd = (CWnd *)pReadWnd;
	pReadWnd->Create (this);
}

//
// process the stored message list for a chat message
// that is the same from and to, and if found check
// its open window pointer for an open chat window
// and if non-null return it
//
CWnd *CWndComm::ChatWndOpen( CMsgIPC *pMsgIn )
{
	ASSERT_VALID (this);
	
	if( plEmailMsgs != NULL )
	{
		ASSERT_VALID (plEmailMsgs);

    	if( plEmailMsgs->GetCount() )
    	{
        	POSITION pos = plEmailMsgs->GetHeadPosition();
        	while( pos != NULL )
        	{   
            	CEMsg *pMsg = 
            		(CEMsg *)plEmailMsgs->GetNext( pos );
            	if( pMsg != NULL )
            	{
        			ASSERT_VALID( pMsg );

					// skip non-chat messages
					if( !(pMsg->m_wStatus & MSG_IS_CHAT) )
						continue;

                	if( pMsg->m_iTo == pMsgIn->m_iTo &&
						pMsg->m_iFrom == pMsgIn->m_iFrom )
					{
						// transfer incoming IPC chat message
						// to chat session message
						pMsg->m_sSubject = pMsgIn->m_sSubject;
						pMsg->m_sMessage = pMsgIn->m_sMessage;
						pMsg->m_iCC = pMsgIn->m_iCC;

						return( pMsg->m_pOpenWnd );
					}
            	}
        	}
    	}
	}

	return( NULL );
}

CWnd *CWndComm::ReadWndOpen( CEMsg *pReadMsg )
{
	ASSERT_VALID (this);
	
	if( plEmailMsgs != NULL )
	{
		ASSERT_VALID (plEmailMsgs);

    	if( plEmailMsgs->GetCount() )
    	{
        	POSITION pos = plEmailMsgs->GetHeadPosition();
        	while( pos != NULL )
        	{   
            	CEMsg *pMsg = 
            		(CEMsg *)plEmailMsgs->GetNext( pos );
            	if( pMsg != NULL )
            	{
        			ASSERT_VALID( pMsg );

					// skip non-email messages
					if( !(pMsg->m_wStatus & MSG_IS_EMAIL) )
						continue;

					// non-null message means test for this
					// message having an open read window
					if( pReadMsg != NULL )
					{
                		if( pMsg->m_iID == pReadMsg->m_iID )
							return( pMsg->m_pOpenWnd );
					}
					else // null message means test for any open window
					{
						if( pMsg->m_pOpenWnd != NULL )
							return( pMsg->m_pOpenWnd );
					}
            	}
        	}
    	}
	}

	return( NULL );
}

void CWndComm::OnSend ()
{
	ASSERT_VALID (this);

	CDlgCompose *pDlg = new CDlgCompose( NULL, NULL, m_lSendWndSize);
	pDlg->Create (CDlgCompose::IDD, this);
}

void CWndComm::OnReply ()
{
	ASSERT_VALID (this);

	int iRet = m_plbEMail->GetCurSel();
	if( iRet == LB_ERR )
		return;

	// get selected message
	CEMsg *pMsg = (CEMsg *)m_plbEMail->GetItemDataPtr(iRet);
	if( pMsg == NULL )
		return;

	PostMessage( IPC_EMAIL_REPLY,
		MAKEWPARAM( pMsg->m_iID,0 ), NULL );
}

void CWndComm::OnForward ()
{
	ASSERT_VALID (this);

	int iRet = m_plbEMail->GetCurSel();
	if( iRet == LB_ERR )
		return;

	// get selected message
	CEMsg *pMsg = (CEMsg *)m_plbEMail->GetItemDataPtr(iRet);
	if( pMsg == NULL )
		return;

	PostMessage( IPC_EMAIL_FORWARD,
		MAKEWPARAM( pMsg->m_iID,0 ), NULL );
}

void CWndComm::OnDelete ()
{
	ASSERT_VALID (this);

	int iRet = m_plbEMail->GetCurSel();
	if( iRet == LB_ERR )
		return;

	// get selected message
	CEMsg *pMsg = (CEMsg *)m_plbEMail->GetItemDataPtr(iRet);
	if( pMsg == NULL )
		return;

	// delete this item from the list box right now
	m_plbEMail->DeleteString( iRet );

	// now post a message to delete it from the global list
	PostMessage( IPC_EMAIL_DELETE,
		MAKEWPARAM( pMsg->m_iID,0 ), NULL );
}

void CWndComm::OnPlayerStatus()
{
	ASSERT_VALID (this);

	if( pPlayerDlg == NULL )
	{
		pPlayerDlg= new CPlyrMsgStatusDlg(NULL);
		pPlayerDlg->Create (CPlyrMsgStatusDlg::IDD, this);
	}
	else
		pPlayerDlg->ShowWindow(SW_SHOW);
}

void CWndComm::OnRefuse ()
{
	ASSERT_VALID (this);

	int iRet = m_plbEMail->GetCurSel();
	if( iRet == LB_ERR )
		return;

	// get selected message
	CEMsg *pEMsg = (CEMsg *)m_plbEMail->GetItemDataPtr(iRet);
	if( pEMsg == NULL )
		return;
	// can't refuse a message that has been read
	if( (pEMsg->m_wStatus & MSG_HAS_BEEN_READ) )
		return;

	CMsgIPC msg ( CMsgIPC::email );

	// return to sender
	msg.m_iTo = pEMsg->m_iFrom;
	msg.m_iFrom = pEMsg->m_iTo;
	msg.m_sSubject = "REFUSED: " + pEMsg->m_sSubject;
	msg.m_sMessage = pEMsg->m_sMessage;

	// delete refused message
	PostMessage( IPC_EMAIL_DELETE,
		MAKEWPARAM( pEMsg->m_iID,0 ), NULL );

#if USE_FAKE_NET
	// BUGBUG fake sending it over the network
	if( plFakeNetwork != NULL )
		plFakeNetwork->AddTail( (CObject *)pIPCMsg );
	PostMessage( IPC_MESSAGE_ARRIVED, NULL, NULL );
#else
	// the way to send a message?
	//TRAP ();
	msg.PostToClient ();
#endif
}

void CWndComm::OnGlobalChat ()
{

	ASSERT_VALID (this);
	// bring up chat
	theApp.GetDlgChat ()->ShowWindow ( SW_SHOWNORMAL );
	theApp.GetDlgChat ()->SetFocus ();
}

void CWndComm::OnChat ()
{

	ASSERT_VALID (this);

	CString sTitle;
	sTitle.LoadString (IDS_CHAT_TITLE);
	csPrintf (&sTitle, " ");

	// get parent location
	CRect rect;
	GetClientRect (&rect);

	// turn off default creation of new chat window
#if 0
	CChatWnd *pWnd = new CChatWnd();

	pWnd->Create (NULL, sTitle, dwPopWndStyle, rect, this);
	pWnd->ShowWindow (SW_SHOW);
	pWnd->UpdateWindow ();
#endif

	// go thru the players, if there is a chat window already assigned
	// to that player then show it, otherwise set flag that a new chat
	// window is allowed
	CChatWnd *pWnd = NULL;
	BOOL bNew = FALSE;
	POSITION pos = plIPCPlayers->GetHeadPosition();
	while( pos != NULL )
	{   
		CIPCPlayer *pPlayer = (CIPCPlayer *)plIPCPlayers->GetNext( pos );
		ASSERT_VALID( pPlayer );
		if( pPlayer->m_pwndChat == NULL )
			bNew = TRUE;
		else
		{
			// an existing chat session, and since we don't know if
			// the window was minimized, show them all
			pWnd = pPlayer->m_pwndChat;
			pWnd->ShowWindow( SW_SHOWNORMAL );
			pWnd->UpdateWindow();
		}
	}

	// there is at least one player without a chat session
	if( bNew )
	{
		pWnd = new CChatWnd();
		pWnd->Create( NULL, sTitle, dwPopWndStyle, rect, this );
		pWnd->ShowWindow( SW_SHOW );
		pWnd->UpdateWindow();
	}
}

void CWndComm::OnLbnChange ()
{

	ASSERT_VALID (this);

	int iNum = m_plbEMail->GetSelCount ();
	
	if (iNum != 1)
		{
#if NEW_BUTTONS
		EnableButton( IDS_MAIL_READ, FALSE);
		EnableButton( IDS_MAIL_REPLY, FALSE);
		EnableButton( IDS_MAIL_FORWARD, FALSE);
#else
		m_tool.SetState (IDS_MAIL_READ, TBSTATE_INDETERMINATE);
		m_tool.SetState (IDS_MAIL_REPLY, TBSTATE_INDETERMINATE);
		m_tool.SetState (IDS_MAIL_FORWARD, TBSTATE_INDETERMINATE);
#endif
		}
	else
	  {
#if NEW_BUTTONS
		EnableButton( IDS_MAIL_READ, TRUE);
		EnableButton( IDS_MAIL_REPLY, TRUE);
		EnableButton( IDS_MAIL_FORWARD, TRUE);
#else
		m_tool.SetState (IDS_MAIL_READ, TBSTATE_ENABLED);
		m_tool.SetState (IDS_MAIL_REPLY, TBSTATE_ENABLED);
		m_tool.SetState (IDS_MAIL_FORWARD, TBSTATE_ENABLED);
#endif
	  }

	if (iNum == 0)
	{
#if NEW_BUTTONS
		EnableButton( IDS_MAIL_DELETE, FALSE);
		EnableButton( IDS_MAIL_REFUSE, FALSE);
#else
		m_tool.SetState (IDS_MAIL_DELETE, TBSTATE_INDETERMINATE);
		m_tool.SetState (IDS_MAIL_REFUSE, TBSTATE_INDETERMINATE);
#endif
	}
	else
	{
		int iRet = m_plbEMail->GetCurSel();
		if( iRet != LB_ERR )
		{
			// get selected message
			CEMsg *pMsg = (CEMsg *)m_plbEMail->GetItemDataPtr(iRet);
			if( pMsg != NULL )
			{
				if( (pMsg->m_wStatus & MSG_IS_EMAIL) &&
					(pMsg->m_wStatus & MSG_HAS_BEEN_READ) )
				{
#if NEW_BUTTONS
					EnableButton( IDS_MAIL_DELETE, TRUE);
					EnableButton( IDS_MAIL_REFUSE, FALSE);
#else
					m_tool.SetState (IDS_MAIL_REFUSE, TBSTATE_INDETERMINATE);
					m_tool.SetState (IDS_MAIL_DELETE, TBSTATE_ENABLED);
#endif
					return;
				}
			}
		}
#if NEW_BUTTONS
		EnableButton( IDS_MAIL_DELETE, TRUE);
		EnableButton( IDS_MAIL_REFUSE, TRUE);
#else
		m_tool.SetState (IDS_MAIL_DELETE, TBSTATE_ENABLED);
		m_tool.SetState (IDS_MAIL_REFUSE, TBSTATE_ENABLED);
#endif
	}
}

//////////////////////////////////////////////////////////////////////////
// owner draw list box for mail messages

void CEMailLB::SetStops( int iNameStop, int iSubjectStop )
{
	m_iNameAt = iNameStop;
	m_iSubjectAt = iSubjectStop;
}

void CEMailLB::LoadEmail(void)
{
	if( plEmailMsgs == NULL )
		return;

	ResetContent();
	int i = 0;
    POSITION pos = plEmailMsgs->GetHeadPosition();
    while( pos != NULL )
    {   
        CEMsg *pMsg = (CEMsg *)plEmailMsgs->GetNext( pos );
        if( pMsg != NULL )
        {
        	ASSERT_VALID( pMsg );

		int iRet = AddString( (LPCSTR)pMsg->m_iID );
		if( iRet != LB_ERR &&
			iRet != LB_ERRSPACE )
			SetItemDataPtr( iRet, (void*)pMsg );
        }
    }
}

CEMailLB::CEMailLB( UINT uID, CWnd *pParent, CRect& rLoc )
{
	CClientDC dc( pParent );
	CString sTest = "Test String";
	CSize csName = dc.GetTextExtent( sTest, sTest.GetLength() );
	m_uCharHeight = csName.cy + 1;
	sTest.Empty();

	CListBox();

	Create (WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
		LBS_EXTENDEDSEL | LBS_MULTIPLESEL | LBS_DISABLENOSCROLL |
		LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | LBS_OWNERDRAWFIXED, 
		rLoc, pParent, uID);
}

//
// draw a line item for the message listbox
//
void CEMailLB::DrawItem( LPDRAWITEMSTRUCT lpDIS )
{
		if( !lpDIS->itemData )
			return;

		// get dc to draw on
		CDC* pDC = CDC::FromHandle( lpDIS->hDC );
		if( pDC == NULL )
			return;
		// create background for default
		COLORREF crBackground = GetSysColor(COLOR_WINDOW);
		// get CEMsg message at this index
		CEMsg *pMsg = (CEMsg *)GetItemDataPtr(lpDIS->itemID);
		if( pMsg == NULL )
			return;

		//IPC_MSG_BM_HEIGHT
		BOOL bRet = FALSE;
		CBitmap workBM;
		CDC workDC;
		workDC.CreateCompatibleDC( pDC );

		// based on message type and status, load bitmap to show
		if( (pMsg->m_wStatus & MSG_IS_CHAT) )
		{
			bRet = workBM.LoadBitmap(IDB_CHAT_MSG);
		}
		else if( (pMsg->m_wStatus & MSG_IS_EMAIL) )
		{
			if( (pMsg->m_wStatus & MSG_HAS_BEEN_READ) )
				bRet = workBM.LoadBitmap(IDB_EMAIL_READ);
			else
				bRet = workBM.LoadBitmap(IDB_EMAIL_MSG);
		}
		else if( (pMsg->m_wStatus & MSG_IS_VOICE) )
		{
			bRet = workBM.LoadBitmap(IDB_VOC_MSG);
		}
		if( !bRet )
			return;

		CBitmap *pOldWork = workDC.SelectObject( &workBM );

		CString sName;
		CPlayer *pPlayer = theGame.GetPlayerByPlyr( pMsg->m_iFrom );
		if( pPlayer != NULL )
			sName = pPlayer->GetName();
		else
			sName = "Unknown player";

#if USE_FAKE_NET
		if( sName.IsEmpty() )
			sName = "Eric";
#endif
		int iWidth = lpDIS->rcItem.right - lpDIS->rcItem.left;

		int iX = lpDIS->rcItem.left + 1;
		int iY = lpDIS->rcItem.top + 1;
		DWORD dwOldText;
		int iOldMode;
		COLORREF crOldBackColor;

		// draw whole listbox item, normal
		if( lpDIS->itemAction & ODA_DRAWENTIRE )
		{
			// COLOR_WINDOW
			CBrush br( crBackground );
			pDC->FillRect( &lpDIS->rcItem, &br );
			crOldBackColor = pDC->SetBkColor( crBackground );
			ASSERT( crOldBackColor != 0x80000000 );

			dwOldText = pDC->SetTextColor( 
				GetSysColor(COLOR_WINDOWTEXT) );
			iOldMode = pDC->SetBkMode( TRANSPARENT );

			pDC->BitBlt( iX, iY, IPC_MSG_BM_HEIGHT, IPC_MSG_BM_HEIGHT,
				&workDC, 0, 0, SRCCOPY );

			iX = lpDIS->rcItem.left + m_iNameAt; //(iWidth / 8);
			pDC->TextOut( iX, iY, sName, sName.GetLength() );
			iX = lpDIS->rcItem.left + m_iSubjectAt; //(iWidth / 4);
			pDC->TextOut( iX, iY, 
				pMsg->m_sSubject, pMsg->m_sSubject.GetLength() );
		
			pDC->SetBkColor( crOldBackColor );
			pDC->SetTextColor( dwOldText );
			pDC->SetBkMode( iOldMode );
		}

		// item has been selected, draw highlighted
		if( (lpDIS->itemState & ODS_SELECTED) &&
		    (lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
		{
			CBrush br( GetSysColor(COLOR_HIGHLIGHT) );
			pDC->FillRect( &lpDIS->rcItem, &br );

			dwOldText = pDC->SetTextColor( 
				GetSysColor(COLOR_HIGHLIGHTTEXT) );
			iOldMode = pDC->SetBkMode( TRANSPARENT );

			pDC->BitBlt( iX, iY, IPC_MSG_BM_HEIGHT, IPC_MSG_BM_HEIGHT,
				&workDC, 0, 0, SRCCOPY );

			iX = lpDIS->rcItem.left + m_iNameAt;
			pDC->TextOut( iX, iY, sName, sName.GetLength() );
			iX = lpDIS->rcItem.left + m_iSubjectAt;
			pDC->TextOut( iX, iY, 
				pMsg->m_sSubject, pMsg->m_sSubject.GetLength() );
		
			pDC->SetTextColor( dwOldText );
			pDC->SetBkMode( iOldMode );
		}

		// item has been de-selected, remove highlight
		if( !(lpDIS->itemState & ODS_SELECTED) &&
		    (lpDIS->itemAction & ODA_SELECT) )
		{
			CBrush br( crBackground );
			pDC->FillRect( &lpDIS->rcItem, &br );

			dwOldText = pDC->SetTextColor( 
				GetSysColor(COLOR_WINDOWTEXT) );
			iOldMode = pDC->SetBkMode( TRANSPARENT );

			pDC->BitBlt( iX, iY, IPC_MSG_BM_HEIGHT, IPC_MSG_BM_HEIGHT,
				&workDC, 0, 0, SRCCOPY );

			iX = lpDIS->rcItem.left + m_iNameAt;
			pDC->TextOut( iX, iY, sName, sName.GetLength() );
			iX = lpDIS->rcItem.left + m_iSubjectAt;
			pDC->TextOut( iX, iY, 
				pMsg->m_sSubject, pMsg->m_sSubject.GetLength() );
		
			pDC->SetTextColor( dwOldText );
			pDC->SetBkMode( iOldMode );
		}

		sName.Empty();
		workDC.SelectObject( pOldWork );
		workDC.DeleteDC();
		workBM.DeleteObject();
}

void CEMailLB::MeasureItem( LPMEASUREITEMSTRUCT lpMIS )
{
	lpMIS->itemHeight = IPC_MSG_BM_HEIGHT;
}

int CEMailLB::CompareItem( LPCOMPAREITEMSTRUCT lpCIS )
{
	WORD uID1 = (WORD)lpCIS->itemData1;
	WORD uID2 = (WORD)lpCIS->itemData2;
	if( uID1 == uID2 )
		return(0);
	if( uID1 < uID2 )
		return(-1);
	else
		return(1);
}

// end of IPCComm.cpp

void CWndComm::OnDestroy() 
{

	delete plEmailMsgs;
	plEmailMsgs = NULL;

	delete m_plbEMail;
	m_plbEMail = NULL;

	delete plIPCPlayers;
	plIPCPlayers = NULL;

	CWndBase::OnDestroy();
}

void CWndComm::CheckChatButton ()
{

#ifdef BUGBUG
	BOOL bDisable = TRUE;
	POSITION pos = plIPCPlayers->GetHeadPosition();
	while (pos != NULL)
		{   
		CIPCPlayer *pPlayer = (CIPCPlayer *) plIPCPlayers->GetNext (pos);
    ASSERT_VALID (pPlayer);
		if (pPlayer->m_pwndChat == NULL)
			{
			bDisable = FALSE;
			break;
			}
		}

#endif
}

void CWndComm::KillAiChatWnd (CPlayer const * pPlyr)
{

	if (plIPCPlayers == NULL)
		return;

	POSITION pos = plIPCPlayers->GetHeadPosition();
	while (pos != NULL)
		{   
		CIPCPlayer *pPlayer = (CIPCPlayer *) plIPCPlayers->GetNext (pos);
    ASSERT_VALID (pPlayer);
		if ((pPlayer->m_pPlyr == pPlyr) && (pPlayer->m_pwndChat != NULL))
			{
			pPlayer->m_pwndChat->DestroyWindow ();
			pPlayer->m_pwndChat = NULL;
			break;
			}
		}
}
