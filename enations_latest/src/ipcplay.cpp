//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////
//
//  IPCPlay.cpp:  CIPCPlayer, CIPCPlayerList, CPlyrMsgStatusDlg
//               
//  Last update:    08/25/95
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "lastplnt.h"
#include "player.h"
#include "IPCPlay.h"

IMPLEMENT_SERIAL( CIPCPlayer, CObject, 0 );
IMPLEMENT_SERIAL( CIPCPlayerList, CObList, 0 );

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


extern CIPCPlayerList *plIPCPlayers;
extern CPlyrMsgStatusDlg *pPlayerDlg;

//CIPCPlayer::CIPCPlayer( WORD wID )
CIPCPlayer::CIPCPlayer( const char *pszName, WORD wID )
{
	m_wID = wID;
	m_sName = pszName;
	m_wStatus = 0;
	m_pPlyr = NULL;
	m_pwndChat = NULL;

	// now set default status
	m_wStatus |= IPC_ACCEPT_EMAIL;
	m_wStatus |= IPC_ACCEPT_CHAT;
	m_wStatus |= IPC_ACCEPT_VOICE;
}

CIPCPlayer::~CIPCPlayer()
{
}

void CIPCPlayer::Serialize( CArchive& archive )
{
    ASSERT_VALID( this );

    CObject::Serialize( archive );

    if( archive.IsStoring() )
    {
		archive << m_wID;
		archive << m_sName;
		archive << m_wStatus;
		archive << (LONG) m_pPlyr->GetPlyrNum ();
	}
	else
	{
		archive >> m_wID;
		archive >> m_sName;
		archive >> m_wStatus;

		LONG l;
		TRAP ();
		archive >> l;
		m_pPlyr = theGame.GetPlayerByPlyr (l);
	}
}

///////////////////////////////////////////////////////////////////////////

void CIPCPlayerList::InitPlayers( void )
{

	if (GetCount () > 0)
		{
		ASSERT (FALSE);
		DeleteList ();
		}

	CIPCPlayer *pPlayer;
	POSITION pos;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
	{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		ASSERT_VALID (pPlr);
		
		if ((! pPlr->IsAI ()) && (! pPlr->IsLocal ()))
			{
			pPlayer = new CIPCPlayer( pPlr->GetName(), (WORD)pPlr->GetPlyrNum() );
			pPlayer->m_pPlyr = pPlr;

			AddTail( (CObject *)pPlayer );
			}
	}
}


CIPCPlayer *CIPCPlayerList::GetPlayer( CString& sName )
{
	ASSERT_VALID( this );

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
        CIPCPlayer *pPlayer = (CIPCPlayer *)GetNext( pos );
        if( pPlayer != NULL )
        {
        	ASSERT_VALID( pPlayer );

			if( pPlayer->m_sName == sName )
                return( pPlayer );
        }
    }
    return( NULL );
}

CIPCPlayer *CIPCPlayerList::GetPlayer( WORD wID )
{
	ASSERT_VALID( this );

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
        CIPCPlayer *pPlayer = (CIPCPlayer *)GetNext( pos );
        if( pPlayer != NULL )
        {
        	ASSERT_VALID( pPlayer );

            if( pPlayer->m_wID == wID )
                return( pPlayer );
        }
    }
    return( NULL );
}


void CIPCPlayerList::RemovePlayer( CString& sName )
{
	ASSERT_VALID( this );

    POSITION pos1, pos2;
    for( pos1 = GetHeadPosition(); 
        ( pos2 = pos1 ) != NULL; )
    {
        CIPCPlayer *pPlayer = (CIPCPlayer *)GetNext( pos1 );
        if( pPlayer == NULL )
            break;
            
        ASSERT_VALID( pPlayer );

		if( pPlayer->m_sName != sName )
            continue;
            
        pPlayer = (CIPCPlayer *)GetAt( pos2 );
        if( pPlayer != NULL )
        {
        	ASSERT_VALID( pPlayer );

        	RemoveAt( pos2 );
        	delete pPlayer;
        	break;
        }
    }
}

void CIPCPlayerList::RemovePlayer( WORD wID )
{
	ASSERT_VALID( this );

    POSITION pos1, pos2;
    for( pos1 = GetHeadPosition(); 
        ( pos2 = pos1 ) != NULL; )
    {
        CIPCPlayer *pPlayer = (CIPCPlayer *)GetNext( pos1 );
        if( pPlayer == NULL )
            break;
            
        ASSERT_VALID( pPlayer );

        if( pPlayer->m_wID != wID )
            continue;
            
        pPlayer = (CIPCPlayer *)GetAt( pos2 );
        if( pPlayer != NULL )
        {
        	ASSERT_VALID( pPlayer );

        	RemoveAt( pos2 );
        	delete pPlayer;
        	break;
        }
    }
}

void CIPCPlayerList::DeleteList( void )
{
	ASSERT_VALID( this );

    if( GetCount() )
    {
        POSITION pos = GetHeadPosition();
        while( pos != NULL )
        {   
            CIPCPlayer *pPlayer = (CIPCPlayer *)GetNext( pos );
            if( pPlayer != NULL )
            {
        		ASSERT_VALID( pPlayer );

                delete pPlayer;
            }
        }
    }
    RemoveAll();
}

CIPCPlayerList::~CIPCPlayerList()
{
	ASSERT_VALID( this );
    DeleteList();
}


/////////////////////////////////////////////////////////////////////////////
// CPlyrMsgStatusDlg dialog


CPlyrMsgStatusDlg::CPlyrMsgStatusDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPlyrMsgStatusDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPlyrMsgStatusDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPlyrMsgStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPlyrMsgStatusDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPlyrMsgStatusDlg, CDialog)
	//{{AFX_MSG_MAP(CPlyrMsgStatusDlg)
	ON_LBN_SELCHANGE(IDC_PLAYER_LIST, OnSelchangePlayerList)
	ON_WM_DESTROY()
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
	ON_WM_SYSCOMMAND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPlyrMsgStatusDlg message handlers

BOOL CPlyrMsgStatusDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	ASSERT (plIPCPlayers != NULL);

	CListBox *pcbPlayers = (CListBox *)GetDlgItem( IDC_PLAYER_LIST );
	if( pcbPlayers == NULL )
		return FALSE;

	int iNumPlyr = 0;
    POSITION pos = plIPCPlayers->GetHeadPosition();
    while( pos != NULL )
    {   
        CIPCPlayer *pPlayer = 
        	(CIPCPlayer *)plIPCPlayers->GetNext( pos );
        if( pPlayer != NULL )
        {
        	ASSERT_VALID( pPlayer );
			CPlayer *pPlyr = pPlayer->m_pPlyr;

			pcbPlayers->AddString( pPlyr->GetName() );
			//int iID = pPlyr->GetPlyrNum();
			//pcbPlayers->SetItemData( iNumPlyr++, (DWORD)iID );
			pcbPlayers->SetItemDataPtr( iNumPlyr++, (void*)pPlyr );
        }
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPlyrMsgStatusDlg::OnMeasureItem (int, LPMEASUREITEMSTRUCT lpMIS)
{

	ASSERT_VALID (this);

	lpMIS->itemHeight = theApp.TextHt ();
}

void CPlyrMsgStatusDlg::OnDrawItem (int, LPDRAWITEMSTRUCT lpDIS)
{
	if ( ODT_BUTTON == lpDIS->CtlType ) // GG
	{
		Default();
		return;
	}

	CPlayer * pPlr = (CPlayer *) lpDIS->itemData;
	if (pPlr == NULL)
		return;

	CDC *pDc = CDC::FromHandle (lpDIS->hDC);
	if (pDc == NULL)
		return;

	thePal.Paint (pDc->m_hDC);
	CFont *pOldFont = pDc->SelectObject( &theApp.TextFont() );
	if( pOldFont == NULL )
		return;

	if (lpDIS->itemState & ODS_SELECTED)
		{
		pDc->SetBkColor (pPlr->GetRGBColor ());
		pDc->SetTextColor (RGB (0, 0, 0));
		}
	else
	  {
		pDc->SetBkColor (RGB (255, 255, 255));
		pDc->SetTextColor (pPlr->GetRGBColor ());
	  }

	pDc->ExtTextOut (lpDIS->rcItem.left, lpDIS->rcItem.top, ETO_CLIPPED | ETO_OPAQUE,
			&(lpDIS->rcItem), pPlr->GetName (), strlen (pPlr->GetName ()), NULL);

	pDc->SelectObject( pOldFont );
}


void CPlyrMsgStatusDlg::OnOK() 
{
	CListBox *pcbPlayers = (CListBox *)GetDlgItem( IDC_PLAYER_LIST );
	if( pcbPlayers == NULL )
		return;

	int iRet = pcbPlayers->GetCurSel();
	if( iRet == CB_ERR )
		return;

	// get IPC player
	CString sItem;
	pcbPlayers->GetText( iRet, sItem );

	//CPlayer *pPlyr = (CPlayer *)pcbPlayers->GetItemDataPtr(iRet);
	//if( pPlyr == NULL )
	//	return;
	CIPCPlayer *pPlayer = 
		plIPCPlayers->GetPlayer( sItem );
	if( pPlayer == NULL )
		return;

	WORD wStatus = 0;
	CButton *pRB = NULL;
	pRB = (CButton *)GetDlgItem(IDC_ACCEPT_EMAIL);
	if( pRB != NULL )
	{
		if( pRB->GetCheck() )
		{
			//pPlayer->m_wStatus |= IPC_ACCEPT_EMAIL;
			//pPlayer->m_wStatus ^= IPC_IGNORE_EMAIL;
			wStatus |= IPC_ACCEPT_EMAIL;
		}
		else
		{
			//pPlayer->m_wStatus ^= IPC_ACCEPT_EMAIL;
			//pPlayer->m_wStatus |= IPC_IGNORE_EMAIL;
			wStatus |= IPC_IGNORE_EMAIL;
		}
	}

	pRB = (CButton *)GetDlgItem(IDC_ACCEPT_CHAT);
	if( pRB != NULL )
	{
		if( pRB->GetCheck() )
		{
			//pPlayer->m_wStatus |= IPC_ACCEPT_CHAT;
			//pPlayer->m_wStatus ^= IPC_IGNORE_CHAT;
			wStatus |= IPC_ACCEPT_CHAT;
		}
		else
		{
			//pPlayer->m_wStatus ^= IPC_ACCEPT_CHAT;
			//pPlayer->m_wStatus |= IPC_IGNORE_CHAT;
			wStatus |= IPC_IGNORE_CHAT;
		}
	}

	// turn off voice
#if 0
	pRB = (CButton *)GetDlgItem(IDC_ACCEPT_VOICE);
	if( pRB != NULL )
	{
		if( pRB->GetCheck() )
		{
			//pPlayer->m_wStatus |= IPC_ACCEPT_VOICE;
			//pPlayer->m_wStatus ^= IPC_IGNORE_VOICE;
			wStatus |= IPC_ACCEPT_VOICE;
		}
		else
		{
			//pPlayer->m_wStatus ^= IPC_ACCEPT_VOICE;
			//pPlayer->m_wStatus |= IPC_IGNORE_VOICE;
			wStatus |= IPC_IGNORE_VOICE;
		}
	}
#endif

	// update player
	pPlayer->m_wStatus = wStatus;
	
	CDialog::OnOK();
}

void CPlyrMsgStatusDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void CPlyrMsgStatusDlg::PostNcDestroy() 
{
	CDialog::PostNcDestroy();

	delete this;
}

void CPlyrMsgStatusDlg::OnSysCommand(UINT nID, LPARAM lParam)
{

	ASSERT_VALID (this);

	// for minimize hide it
	if ((nID == SC_MINIMIZE) || (nID == SC_CLOSE))
		{
		ShowWindow (SW_HIDE);
		return;
		}

	CDialog::OnSysCommand(nID, lParam);
}


void CPlyrMsgStatusDlg::OnDestroy() 
{
	if( pPlayerDlg != NULL )
		pPlayerDlg = NULL;

	CDialog::OnDestroy();
}

//
// selected a player from the combo box
//
void CPlyrMsgStatusDlg::OnSelchangePlayerList() 
{
	ASSERT (plIPCPlayers != NULL);

	CListBox *pcbPlayers = (CListBox *)GetDlgItem( IDC_PLAYER_LIST );
	if( pcbPlayers == NULL )
		return;

	int iRet = pcbPlayers->GetCurSel();
	if( iRet == CB_ERR )
		return;

	// get IPC player
	CString sItem;
	pcbPlayers->GetText( iRet, sItem );

	//CPlayer *pPlyr = (CPlayer *)pcbPlayers->GetItemDataPtr(iRet);
	//if( pPlyr == NULL )
	//	return;
	CIPCPlayer *pPlayer = 
		plIPCPlayers->GetPlayer( sItem );
	if( pPlayer == NULL )
		return;

	CButton *pRB = NULL;

	// consider email status, this player
	// and set radio buttons
	if( (pPlayer->m_wStatus & IPC_ACCEPT_EMAIL) &&
		!(pPlayer->m_wStatus & IPC_IGNORE_EMAIL) )
	{
		//IDC_ACCEPT_EMAIL
		pRB = (CButton *)GetDlgItem(IDC_ACCEPT_EMAIL);
		if( pRB != NULL )
			pRB->SetCheck(TRUE);
		//IDC_IGNORE_EMAIL
		pRB = (CButton *)GetDlgItem(IDC_IGNORE_EMAIL);
		if( pRB != NULL )
			pRB->SetCheck(FALSE);
	}
	else if( !(pPlayer->m_wStatus & IPC_ACCEPT_EMAIL) &&
		(pPlayer->m_wStatus & IPC_IGNORE_EMAIL) )
	{
		//IDC_ACCEPT_EMAIL
		pRB = (CButton *)GetDlgItem(IDC_ACCEPT_EMAIL);
		if( pRB != NULL )
			pRB->SetCheck(FALSE);
		//IDC_IGNORE_EMAIL
		pRB = (CButton *)GetDlgItem(IDC_IGNORE_EMAIL);
		if( pRB != NULL )
			pRB->SetCheck(TRUE);
	}

	// consider chat status, this player
	// and set radio buttons
	if( (pPlayer->m_wStatus & IPC_ACCEPT_CHAT) &&
		!(pPlayer->m_wStatus & IPC_IGNORE_CHAT) )
	{
		//IDC_ACCEPT_CHAT
		pRB = (CButton *)GetDlgItem(IDC_ACCEPT_CHAT);
		if( pRB != NULL )
			pRB->SetCheck(TRUE);
		//IDC_IGNORE_CHAT
		pRB = (CButton *)GetDlgItem(IDC_IGNORE_CHAT);
		if( pRB != NULL )
			pRB->SetCheck(FALSE);
	}
	else if( !(pPlayer->m_wStatus & IPC_ACCEPT_CHAT) &&
		(pPlayer->m_wStatus & IPC_IGNORE_CHAT) )
	{
		//IDC_ACCEPT_CHAT
		pRB = (CButton *)GetDlgItem(IDC_ACCEPT_CHAT);
		if( pRB != NULL )
			pRB->SetCheck(FALSE);
		//IDC_IGNORE_CHAT
		pRB = (CButton *)GetDlgItem(IDC_IGNORE_CHAT);
		if( pRB != NULL )
			pRB->SetCheck(TRUE);
	}

	// turn off voice support
#if 0
	// consider VOICE status, this player
	// and set radio buttons
	if( (pPlayer->m_wStatus & IPC_ACCEPT_VOICE) &&
		!(pPlayer->m_wStatus & IPC_IGNORE_VOICE) )
	{
		//IDC_ACCEPT_VOICE
		pRB = (CButton *)GetDlgItem(IDC_ACCEPT_VOICE);
		if( pRB != NULL )
			pRB->SetCheck(TRUE);
		//IDC_IGNORE_VOICE
		pRB = (CButton *)GetDlgItem(IDC_IGNORE_VOICE);
		if( pRB != NULL )
			pRB->SetCheck(FALSE);
	}
	else if( !(pPlayer->m_wStatus & IPC_ACCEPT_VOICE) &&
		(pPlayer->m_wStatus & IPC_IGNORE_VOICE) )
	{
		//IDC_ACCEPT_VOICE
		pRB = (CButton *)GetDlgItem(IDC_ACCEPT_VOICE);
		if( pRB != NULL )
			pRB->SetCheck(FALSE);
		//IDC_IGNORE_VOICE
		pRB = (CButton *)GetDlgItem(IDC_IGNORE_VOICE);
		if( pRB != NULL )
			pRB->SetCheck(TRUE);
	}
#endif
}
