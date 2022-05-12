//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////
//
//  IPCSend.cpp : IPC Send Email Window objects
//                Divide and Conquer
//               
//  Last update: 10/03/96   -   09/04/95
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "player.h"
#include "lastplnt.h"
#include "IPCSend.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


#if USE_FAKE_NET
// BUGBUG for testing, fake a network with this list
extern CObList *plFakeNetwork;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgCompose dialog

CDlgCompose::CDlgCompose(CWnd* pParent /*=NULL*/, 
	CEMsg *pReplyMsg /*=NULL*/, LONG lSendWndSize )
	: CDialog(CDlgCompose::IDD, pParent)
{

	//{{AFX_DATA_INIT(CDlgCompose)
	m_sSubject = _T("");
	m_sText = _T("");
	m_sTo = -1;
	//}}AFX_DATA_INIT


	m_lSendWndSize = lSendWndSize;
	m_xMin = m_yMin = 0;

	m_pMsg = pReplyMsg;
	m_iPlayer = theGame.GetMe()->GetPlyrNum();
	m_pcbPlayers = NULL;
}

CDlgCompose::~CDlgCompose()
{
	if( m_pcbPlayers != NULL )
	{
		delete m_pcbPlayers;
		m_pcbPlayers = NULL;
	}
}

void CDlgCompose::CleanUp( void )
{
	if( m_pcbPlayers != NULL )
	{
		m_pcbPlayers->ResetContent();
		delete m_pcbPlayers;
		m_pcbPlayers = NULL;
	}

	m_sSubject.Empty();
	m_sText.Empty();

	// this has been a reply message
	if( m_pMsg != NULL )
	{
		GetParent()->PostMessage( IPC_EMAIL_DELETE,
			MAKEWPARAM( m_pMsg->m_iID,0 ), NULL );
	}
}

void CDlgCompose::InitToCombo()
{
	// init the combo box
	// theGame.GetMe().GetName()
	if( m_pcbPlayers == NULL )
		return;

	m_iNumPlyr = 0;
	POSITION pos;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
	{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		ASSERT_VALID (pPlr);

		if ((! pPlr->IsAI ()) && (! pPlr->IsLocal ()))
			{
			m_pcbPlayers->AddString( pPlr->GetName() );
			m_pcbPlayers->SetItemDataPtr( m_iNumPlyr, (void*) pPlr );

			m_iNumPlyr++;
			}
	}
}


void CDlgCompose::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgCompose)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_MAIL_TEXT, m_ctrlText);
	DDX_Control(pDX, IDC_MAIL_SUBJECT, m_ctrlSubject);
	DDX_Text(pDX, IDC_MAIL_SUBJECT, m_sSubject);
	DDX_Text(pDX, IDC_MAIL_TEXT, m_sText);
	//}}AFX_DATA_MAP

	//DDX_Control(pDX, IDC_MAIL_TO, m_ctrlTo);
	//DDX_CBIndex(pDX, IDC_MAIL_TO, m_sTo);
}


BEGIN_MESSAGE_MAP(CDlgCompose, CDialog)
	//{{AFX_MSG_MAP(CDlgCompose)
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_CBN_SELENDOK(IDC_MAIL_TO, OnSelectMailTo)	
	ON_MESSAGE( IPC_UNHIDE_WINDOW, OnUnHideWindow)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgCompose message handlers

#if 0
void CDlgCompose::OnKillFocus( CWnd* )
{
	// since the game's toolbar is always on top, if the IPC Comm window
	// is to be maximized, then part of it will be hidden by the toolbar
	// if the window goes to full screen, so determine the size of a full
	// screen and reduce that by 2 times the size of the caption, which
	// hopefully will get the bottom of the window above the toolbar
	int iMaxHeight = GetSystemMetrics(SM_CYSCREEN) 
		- (GetSystemMetrics(SM_CYCAPTION) * 4);

	// check if we are slipping behind toolbar
	CRect rect;
	GetWindowRect( &rect );
	if( rect.top > (iMaxHeight - GetSystemMetrics(SM_CYCAPTION)) )
	{
		// save current height to use later for bottom adjustment
		int iHeight = rect.Height();
		rect.top = iMaxHeight - GetSystemMetrics(SM_CYCAPTION);
		// adjust bottom to current height to match new top
		rect.bottom = rect.top + iHeight;
		PostMessage( IPC_UNHIDE_WINDOW,
			MAKEWPARAM( rect.left, rect.top ), MAKELPARAM( rect.right, rect.bottom ) );
	}
}
#endif

void CDlgCompose::OnMove( int x, int y )
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

	CDialog::OnMove(x, y);
}


LONG CDlgCompose::OnUnHideWindow( UINT uParam, LONG lParam )
{
	CRect rect( LOWORD( uParam ), HIWORD( uParam ),
		LOWORD( lParam ), HIWORD( lParam ) );

	MoveWindow( &rect, TRUE );
	return TRUE;
}

void CDlgCompose::OnSelectMailTo() 
{
	// set or unset the player where the dc occurred
	if( m_pcbPlayers == NULL )
		return;

	int iSel = m_pcbPlayers->GetCurSel();
	if( iSel < m_iNumPlyr )
		m_pcbPlayers->SetSelected(iSel);
}

void CDlgCompose::OnCancel() 
{
	CleanUp();

	CDialog::OnCancel();
}

void CDlgCompose::OnOK() 
{
	// m_ctrlTo selections contains the too
	// who am I for the from
	// 

	m_ctrlText.GetWindowText( m_sText );
	m_ctrlSubject.GetWindowText( m_sSubject );

	// create a CMsgIPC object with this stuff
	// for each player selected
	for( int i=0; i<m_iNumPlyr; ++i)
	{
		// a player was selected
		if( m_pcbPlayers->IsSelected(i) )
		{
			CMsgIPC msg ( CMsgIPC::email );

			// BUGBUG m_iTo should be determined by looking
			// up the player relative to the m_iSelect[iSel]
			// being toggled on, then the iSel equivilent in
			// the m_ctrlTo which will contain either the
			// CPlayer * or the id of the player

			// testing getting the player pointer
			CString sName;
			CPlayer *pPlayer = (CPlayer *)m_pcbPlayers->GetItemDataPtr(i);
			if( pPlayer != NULL &&
				(int)pPlayer != -1 )
				sName = pPlayer->GetName();

			if( m_pMsg == NULL )
			{				
				msg.m_iFrom = m_iPlayer;
#if USE_FAKE_NET
				// BUGBUG this only allows mail to player 1
				msg.m_iTo = m_iPlayer;
#else
				msg.m_iTo = pPlayer->GetPlyrNum();
#endif
			}
			else // this a reply to an existing message
			{
				//msg.m_iTo = m_pMsg->m_iFrom;
				msg.m_iTo = m_pMsg->m_iTo;
				msg.m_iFrom = m_iPlayer;
			}

			// now construct the rest of the message
			msg.m_iLen = m_sText.GetLength();
			msg.m_sSubject = m_sSubject;
			msg.m_sMessage = m_sText;

#if USE_FAKE_NET
			// at this point send the CMsgIPC to the network
			// but since there is no way to have a real net
			// during testing, lets fake one with a list
			if( plFakeNetwork != NULL )
				plFakeNetwork->AddTail( (CObject *)pMsg );
			// tell parent to go look
			GetParent()->PostMessage( IPC_MESSAGE_ARRIVED,
				NULL, NULL );
#else
			// the way to send a message?
			msg.PostToClient ();
#endif
		}
	}

	// get current size and tell parent about it
	CRect rDlg;
	GetWindowRect( &rDlg );
	GetParent()->PostMessage( IPC_SENDWND_SIZE, NULL,
		MAKELPARAM(rDlg.Width(), rDlg.Height()) );

	CleanUp();
	CDialog::OnOK();
}

void CDlgCompose::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
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

	CDialog::OnGetMinMaxInfo(lpMMI);
}

void CDlgCompose::PostNcDestroy() 
{
	
	CDialog::PostNcDestroy();

	delete this;
}

BOOL CDlgCompose::OnInitDialog() 
{

	ASSERT_VALID (this);

	CDialog::OnInitDialog();
	
	// get the dialog size - this is our min
	CRect rect;
	GetWindowRect (&rect);

	// if available, then use previous size of send window
	if( m_lSendWndSize != 0L )
	{
		rect.right = rect.left + LOWORD(m_lSendWndSize);
		rect.bottom = rect.top + HIWORD(m_lSendWndSize);
		MoveWindow( &rect, TRUE );
	}
	m_xMin = rect.Width ();
	m_yMin = rect.Height ();

	// create owner draw combo
	m_pcbPlayers = new CSendCombo();
	m_pcbPlayers->OnParentInit( IDC_MAIL_TO, this );

	m_iNumPlyr = 0;

	if( m_pMsg == NULL )
	{
		InitToCombo();
	}
	else // get only the to player of the m_pMsg of a reply message
	{
		// creating a forwarded message
		if( !m_pMsg->m_iTo )
		{
			InitToCombo();

			m_sSubject.Empty();
			m_sSubject = "FWD: " + m_pMsg->m_sSubject;
			m_ctrlSubject.SetWindowText( m_sSubject );

			m_sText.Empty();
			m_sText = m_pMsg->m_sMessage;
			m_ctrlText.SetWindowText( m_sText );

			m_pMsg = NULL;

			SetWindowText( "Forward Message" );
		}
		else // creating a reply message
		{
			CPlayer *pPlayer = 
				(CPlayer *)theGame.GetPlayerByPlyr(m_pMsg->m_iTo);
			if( pPlayer != NULL && (int)pPlayer != -1 )
			{
			CString sName = pPlayer->GetName();
#if USE_FAKE_NET
			if( sName.IsEmpty() )
				sName = "Eric";
#endif
			m_pcbPlayers->AddString( sName );
			m_pcbPlayers->SetItemDataPtr( m_iNumPlyr, (void*) pPlayer );
			m_pcbPlayers->SetCurSel(m_iNumPlyr);
			m_pcbPlayers->SetSelected(m_iNumPlyr); //m_iSelect[0] = 1;
			m_iNumPlyr = 1;

			CString sTitle;
			sTitle = "Reply to [" + sName + "]";
			
			SetWindowText( sTitle );
			sName.Empty();
			

			if( m_pMsg != NULL )
				m_sSubject = "RE:" + m_pMsg->m_sSubject;
			else
				m_sSubject = sTitle;

			m_ctrlSubject.SetWindowText( m_sSubject );

			sTitle.Empty();
			}
			else
				m_sSubject = m_pMsg->m_sSubject;
		}
	}

	return TRUE;
}

void CDlgCompose::OnSize(UINT nType, int cx, int cy) 
{

	ASSERT_VALID (this);

	CDialog::OnSize(nType, cx, cy);

	if( m_pcbPlayers == NULL )
		return;
	if( m_pcbPlayers->m_hWnd == NULL)
		return;
 
	// we move the right side of to, subject, and right/bottom of edit
	CRect rect;
	GetClientRect (&rect);
	ClientToScreen (&rect);
	CRect rWnd;

	if( m_pcbPlayers != NULL &&
		m_pcbPlayers->m_hWnd != NULL)
	{
		m_pcbPlayers->GetWindowRect (&rWnd);
		m_pcbPlayers->SetWindowPos (NULL, 0, 0, 
			rect.right - rWnd.left, rWnd.Height (), 
			SWP_NOMOVE | SWP_NOZORDER);
	}

	m_ctrlSubject.GetWindowRect (&rWnd);
	m_ctrlSubject.SetWindowPos (NULL, 0, 0, rect.right - rWnd.left, rWnd.Height (), SWP_NOMOVE | SWP_NOZORDER);

	m_ctrlText.GetWindowRect (&rWnd);
	m_ctrlText.SetWindowPos (NULL, 0, 0, rect.right - rWnd.left, rect.bottom - rWnd.top, SWP_NOMOVE | SWP_NOZORDER);
}


/////////////////////////////////////////////////////////////////////////

void CSendCombo::OnParentInit( UINT uID, CWnd *pParent )
{
	// BUGBUG hard coded maximum number of copy to's
	// an array to toggle and track muliple selections
	// out of the m_ctrlTo combo box
	for( int i=0; i<MAX_CC; ++i)
		m_iSelect[i] = 0;

	CClientDC dc( pParent );
	CString sTest = "Test String";
	CSize csName = dc.GetTextExtent( sTest, sTest.GetLength() );
	m_uCharHeight = csName.cy + 1;
	sTest.Empty();

	BOOL bRet = SubclassDlgItem( uID, pParent );
}

CSendCombo::CSendCombo()
{
	CComboBox();
}

// BUGBUG the old way does not work under Win 95
//
#if 0
CSendCombo::CSendCombo( UINT uID, CWnd *pParent, CRect& rLoc )
{
	// BUGBUG hard coded maximum number of copy to's
	// an array to toggle and track muliple selections
	// out of the m_ctrlTo combo box
	for( int i=0; i<MAX_CC; ++i)
		m_iSelect[i] = 0;

	CClientDC dc( pParent );
	CString sTest = "Test String";
	CSize csName = dc.GetTextExtent( sTest, sTest.GetLength() );
	m_uCharHeight = csName.cy + 1;
	sTest.Empty();

	CComboBox();

	//65,2,119,76
	//BOOL Create( DWORD dwStyle, const RECT& rect, 
	//CWnd* pParentWnd, UINT nID );
	Create( CBS_DROPDOWNLIST | CBS_OWNERDRAWVARIABLE //| CBS_SORT 
		| CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP,
		rLoc, pParent, uID);
}
#endif

BOOL CSendCombo::IsSelected( int iAt )
{
	return( m_iSelect[iAt] );
}

void CSendCombo::SetSelected( int iSel )
{
	m_iSelect[iSel] = !m_iSelect[iSel];
}

void CSendCombo::DrawItem( LPDRAWITEMSTRUCT lpDIS )
{
	if( !lpDIS->itemData )
		return;

	// get dc to draw on
	CDC* pDC = CDC::FromHandle( lpDIS->hDC );
	if( pDC == NULL )
		return;
	// create background for default
	COLORREF crBackground = GetSysColor(COLOR_WINDOW);
	// get CPlayer at this index
	CPlayer *pPlayer = (CPlayer *)GetItemDataPtr(lpDIS->itemID);
	if( pPlayer == NULL ||
		(int)pPlayer == -1 )
		return;

	CString sName;
	sName = pPlayer->GetName();
			
	BOOL bRet = FALSE;
	CBitmap workBM;
	CDC workDC;
	workDC.CreateCompatibleDC( pDC );

	// this player has been selected
	if( m_iSelect[lpDIS->itemID] )
	{
		bRet = workBM.LoadBitmap(IDB_PLYR_SELECTED);
	}
	else
	{
		bRet = workBM.LoadBitmap(IDB_NO_SELECT);
	}
	if( !bRet )
		return;

	CBitmap *pOldWork = workDC.SelectObject( &workBM );

	DWORD dwOldText;
	int iOldMode;
	COLORREF crOldBackColor;

	int iX = lpDIS->rcItem.left + 1;
	int iY = lpDIS->rcItem.top + 1;

	// draw whole listbox item, normal
	if( lpDIS->itemAction & ODA_DRAWENTIRE )
	{
		// COLOR_WINDOW
		CBrush br( crBackground );
		pDC->FillRect( &lpDIS->rcItem, &br );
		crOldBackColor = pDC->SetBkColor( crBackground );
		ASSERT( crOldBackColor != 0x80000000 );

		// COLOR_WINDOWTEXT
		dwOldText = pDC->SetTextColor( 
			GetSysColor(COLOR_WINDOWTEXT) );
		iOldMode = pDC->SetBkMode( TRANSPARENT );

		pDC->BitBlt( iX, iY, IPC_MSG_BM_HEIGHT, IPC_MSG_BM_HEIGHT,
			&workDC, 0, 0, SRCCOPY );

		iX += IPC_MSG_BM_HEIGHT;
		pDC->TextOut( iX, iY, sName, sName.GetLength() );

		pDC->SetBkColor( crOldBackColor );
		pDC->SetTextColor( dwOldText );
		pDC->SetBkMode( iOldMode );
	}
	iX = lpDIS->rcItem.left + 1;

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

		iX += IPC_MSG_BM_HEIGHT;
		pDC->TextOut( iX, iY, sName, sName.GetLength() );

		pDC->SetBkColor( crOldBackColor );
		pDC->SetTextColor( dwOldText );
		pDC->SetBkMode( iOldMode );
	}
	iX = lpDIS->rcItem.left + 1;

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

		iX += IPC_MSG_BM_HEIGHT;
		pDC->TextOut( iX, iY, sName, sName.GetLength() );

		pDC->SetBkColor( crOldBackColor );
		pDC->SetTextColor( dwOldText );
		pDC->SetBkMode( iOldMode );
	}

	sName.Empty();
	workDC.SelectObject( pOldWork );
	workDC.DeleteDC();
	workBM.DeleteObject();
}

void CSendCombo::MeasureItem( LPMEASUREITEMSTRUCT lpMIS )
{
	lpMIS->itemHeight = IPC_MSG_BM_HEIGHT;
}

int CSendCombo::CompareItem( LPCOMPAREITEMSTRUCT lpCIS )
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

/////////////////////////////////////////////////////////////////////////////

// end of IPCSend.cpp

