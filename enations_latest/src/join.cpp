//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// join.cpp : implementation file
//

#include "stdafx.h"
#include "netapi.h"
#include "lastplnt.h"
#include "join.h"
#include "player.h"
#include "racedata.h"
#include "help.h"

#include "creatmul.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

int ListBoxFindDataPtr (CListBox const & lst, void const * pData);


/////////////////////////////////////////////////////////////////////////////
// CGameInfo

#ifdef _DEBUG
void CGameInfo::AssertValid() const
{

	CObject::AssertValid ();

	ASSERT ((0 <= m_iNumOpponents) && (m_iNumOpponents < 257));
	ASSERT ((0 <= m_iAIlevel) && (m_iAIlevel < 4));
	ASSERT ((0 <= m_iWorldSize) && (m_iWorldSize < 3));
	ASSERT_VALID_CSTRING (&m_sName);
	ASSERT_VALID_CSTRING (&m_sDesc);
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CJoinMulti

void CJoinMulti::Init ()
{

	ASSERT_VALID (this);

	// set it as the server
	theGame.ctor ();
	theGame.SetServer (FALSE);
	theGame._SetIsNetGame ( TRUE );

	theApp.DestroyMain ();

	// load the data
	if ( ! theApp.LoadData () )
		{
		theApp.CreateMain ();
		return;
		}

	m_dlgJoinPublish.Create (this, CDlgJoinPublish::IDD, theApp.m_pMainWnd);
}

void CJoinMulti::ClosePick ()
{

	ASSERT_VALID (this);

	if (m_dlgJoinPublish.m_hWnd != NULL)
		m_dlgJoinPublish.DestroyWindow ();

	if (m_dlgJoinGame.m_hWnd != NULL)
		m_dlgJoinGame.DestroyWindow ();

	CCreateNewBase::ClosePick ();
	CCreateLoadBase::ClosePick ();
	CMultiBase::ClosePick ();
}

void CJoinMulti::CloseAll ()
{

	ASSERT_VALID (this);

	ClosePick ();

	CCreateNewBase::CloseAll ();
	CCreateLoadBase::CloseAll ();
	CMultiBase::CloseAll ();
}

void CJoinMulti::GameLoaded ( void * pBuf, int iLen )
{

	// load the game file
	CDlgCreateStatus * pDlg = theApp.m_pCreateGame->GetDlgStatus ();
	if (pDlg != NULL)
		pDlg->SetPer ( 0 );

	CPlayer * pPlyrMe = theGame.GetMe ();
	CPlayer * pPlyrSrvr = theGame.GetServer ();

	// create the world
	theApp.NewWorld ();

	// decompress the file
	int iDecompLen;
	void * pDeComp = CoDec::Decompress ( pBuf, iLen, iDecompLen );
	free (pBuf);

	// put it in a CMemFile
	CMemFile filMem;
	filMem.Attach ( (BYTE *) pDeComp, iDecompLen );

	// serialize it
	CArchive ar (&filMem, CArchive::load);
	theGame.Serialize (ar);
	theGame.SetServer (FALSE);
	ar.Close ();

	filMem.Detach ();
	CoDec::FreeBuf ( pDeComp );
	filMem.Close ();

	// now set up players the way they are now
	m_wndPlyrList.RemoveAll ();
	POSITION pos;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
		{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		POSITION pos2;
		for (pos2 = theGame.m_lstLoad.GetTailPosition(); pos2 != NULL; )
			{
			CPlayer *pPlr2 = theGame.m_lstLoad.GetPrev (pos2);
			if ( pPlr2->GetPlyrNum () == pPlr->GetPlyrNum () )
				{
				pPlr->SetNetNum ( pPlr2->GetNetNum () );
				pPlr2->SetNetNum ( 0 );
				pPlr->SetAI ( pPlr2->IsAI () );
				break;
				}
			}

		pPlr->SetState (CPlayer::ready);
		if ( pPlr->GetPlyrNum () == pPlyrMe->GetPlyrNum () )
			{
			theGame._SetMe ( pPlr );
			pPlr->SetLocal ( TRUE );
			pPlr->SetAI ( FALSE );
			}
		else
			{
			pPlr->SetLocal ( FALSE );
			if ( pPlr->GetPlyrNum () == pPlyrSrvr->GetPlyrNum () )
				{
				theGame._SetServer ( pPlr );
				pPlr->SetAI ( FALSE );
				}
			}

		// new list box
		if ( pPlr->GetNetNum () != 0 )
			m_wndPlyrList.AddPlayer ( pPlr );
		}

	// drop lstLoad
	for (pos = theGame.m_lstLoad.GetTailPosition(); pos != NULL; )
		{
		CPlayer *pPlr = theGame.m_lstLoad.GetPrev (pos);
		delete pPlr;
		}
	theGame.m_lstLoad.RemoveAll ();

	CNetInitDone msg (theGame.GetMe ());
	theGame.PostToServer (&msg, sizeof (msg));

	// done
	pDlg = theApp.m_pCreateGame->GetDlgStatus ();
	if (pDlg != NULL)
		pDlg->SetPer ( 100 );
}

#ifdef _DEBUG
void CJoinMulti::AssertValid() const
{

	CMultiBase::AssertValid ();

	ASSERT_VALID (&m_dlgJoinPublish);
	ASSERT_VALID (&m_dlgJoinGame);
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CDlgJoin dialog

CDlgJoinPublish::CDlgJoinPublish (CWnd* pParent /*=NULL*/)
	: CDialog (CDlgJoinPublish::IDD, pParent)
{

	//{{AFX_DATA_INIT(CDlgJoinPublish)
	m_sName = _T("");
	m_NetRadio = 0;
	m_strPw = _T("");
	//}}AFX_DATA_INIT

	m_idOK = -1;
}

CDlgJoinPublish::~CDlgJoinPublish ()
{

}

void CDlgJoinPublish::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgJoinPublish)
	DDX_Control(pDX, IDC_CREATE_ADV, m_btnAdv);
	DDX_Control(pDX, IDC_JOIN_UNPUBLISH, m_btnUnPublish);
	DDX_Control(pDX, IDC_JOIN_NAME, m_StrName);
	DDX_Control(pDX, IDOK, m_BtnOk);
	DDX_Text(pDX, IDC_JOIN_NAME, m_sName);
	DDX_Text(pDX, IDC_CREATE_PW, m_strPw);
	//}}AFX_DATA_MAP

//	DDX_Radio(pDX, IDC_CREATE_TCPIP, m_NetRadio);
	if (pDX->m_bSaveAndValidate)
		m_NetRadio = GetCheckedRadioButton (IDC_CREATE_TCPIP, IDC_CREATE_DIRECT) - IDC_CREATE_TCPIP;
	else
		CheckRadioButton (IDC_CREATE_TCPIP, IDC_CREATE_DIRECT, IDC_CREATE_TCPIP + m_NetRadio);
}

BEGIN_MESSAGE_MAP(CDlgJoinPublish, CDialog)
	//{{AFX_MSG_MAP(CDlgJoinPublish)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	ON_EN_CHANGE(IDC_JOIN_NAME, OnChangeCreateName)
	ON_BN_CLICKED(IDC_JOIN_UNPUBLISH, OnJoinUnpublish)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CREATE_ADV, OnJoinAdv)
	ON_BN_CLICKED(IDC_CREATE_DIRECT, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_DIRECTPLAY, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_IPX, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_MODEM, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_NETBIOS, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_TAPI, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_TCPIP, OnRadioChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CDlgJoinPublish::Create (CJoinMulti *pJm, UINT id, CWnd *pPar)
{

	ASSERT_VALID (this);

	m_pJm = pJm;
	CDialog::Create (id, pPar);
}

void CDlgJoinPublish::SetControls ()
{

	ASSERT_VALID (this);

	UpdateData (TRUE);

	m_btnAdv.EnableWindow (theNet.GetMode () != CNetApi::joined);

	// set OK button text depending
	int ID;
	if (theNet.GetMode () == CNetApi::joined)
		ID = IDS_JOIN_JOIN;
	else
		ID = IDS_JOIN_PUBLISH;
	if (ID != m_idOK)
		{
		m_idOK = ID;
		CString sBtn;
		sBtn.LoadString (ID);
		m_BtnOk.SetWindowText (sBtn);
		}

	m_btnUnPublish.EnableWindow (theNet.GetMode () == CNetApi::joined);
	m_StrName.EnableWindow (theNet.GetMode () != CNetApi::joined);

	// if we're connected can't change the protocol
	if (theNet.GetMode () != m_PrevConn)
		{
		m_PrevConn = theNet.GetMode ();
		if (theNet.GetMode () == CNetApi::joined)
			{
			for (int iRad=0; iRad<NUM_PROTOCOLS; iRad++)
				if (iRad == m_NetRadio)
					GetDlgItem (IDC_CREATE_TCPIP + iRad)->EnableWindow (TRUE);
				else
					GetDlgItem (IDC_CREATE_TCPIP + iRad)->EnableWindow (FALSE);
			}
		else
			for (int iRad=0; iRad<NUM_PROTOCOLS; iRad++)
				if (CNetApi::SupportsProtocol (aPr[iRad]))
					GetDlgItem (IDC_CREATE_TCPIP + iRad)->EnableWindow (TRUE);
				else
					GetDlgItem (IDC_CREATE_TCPIP + iRad)->EnableWindow (FALSE);
		}

	m_BtnOk.EnableWindow (m_sName.GetLength () > 0);
}

/////////////////////////////////////////////////////////////////////////////
// CDlgJoinPublish message handlers

BOOL CDlgJoinPublish::OnInitDialog()
{

	m_PrevConn = theNet.GetMode () + 1;

	CDialog::OnInitDialog();

	UpdateData (TRUE);

	m_sName = theApp.GetProfileString ("Create", "Name");

	m_NetRadio = theApp.GetProfileInt ("Create", "Connection", 0);
	m_NetRadio = __minmax ( 0, NUM_PROTOCOLS - 1, m_NetRadio );
	if (! CNetApi::SupportsProtocol (aPr [m_NetRadio]) )
		for (int iRad=0; iRad<NUM_PROTOCOLS; iRad++)
			if (CNetApi::SupportsProtocol (aPr[iRad]))
				{
				m_NetRadio = iRad;
				break;
				}

	UpdateData (FALSE);

	CenterWindow (theApp.m_pMainWnd);

	SetControls ();

	ASSERT_VALID (this);
	return TRUE;
}

void CDlgJoinPublish::OnJoinAdv() 
{
	
	UpdateData (TRUE);

	vpAdvDialog (m_hWnd, aPr[m_NetRadio], FALSE);
}

void CDlgJoinPublish::OnRadioChange() 
{

}

void CDlgJoinPublish::OnOK()
{

	ASSERT_VALID (this);
	char sBuf [40];
	sprintf ( sBuf, "Published game protocol: %d", m_NetRadio);
	theApp.Log ( sBuf );

	// save the parameters we use
	UpdateData (TRUE);
	m_pJm->m_iNet = m_NetRadio;
	m_pJm->m_sName = m_sName;

	theApp.WriteProfileString ("Create", "Name", m_sName);
	theApp.WriteProfileInt ("Create", "Connection", m_NetRadio);

	// check for a name that is too long
	if (m_sName.GetLength () >= iMaxNameLen)
		{
		TRAP ();
		AfxMessageBox (IDS_NAME_TOO_LONG, MB_OK | MB_ICONSTOP);
		return;
		}

	if (theNet.GetMode () == CNetApi::closed)
		if (theNet.OpenClient (aPr[m_NetRadio], theApp.m_wndMain.m_hWnd, NULL))
			return;

	SetControls ();

	// switch to the players & pick race dialog
	if (m_pJm->m_dlgJoinGame.m_hWnd == NULL)
		m_pJm->m_dlgJoinGame.Create (m_pJm, CDlgJoinGame::IDD, theApp.m_pMainWnd);
	m_pJm->m_dlgJoinGame.ShowWindow (SW_SHOW);
	ShowWindow (SW_HIDE);
}

void CDlgJoinPublish::OnCancel()
{

	ASSERT_VALID (this);

	theGame.Close ();
	theNet.Close ( FALSE );
	CDialog::OnCancel();
	theApp.CreateMain ();
}

void CDlgJoinPublish::OnHelp()
{

	ASSERT_VALID (this);
	theApp.WinHelp (HLP_JOIN_PUBLISH, HELP_CONTEXT);
}

// if we're on a net - see if we can click ok
void CDlgJoinPublish::OnChangeCreateName()
{

	ASSERT_VALID (this);

	if (m_NetRadio == 0)
		m_BtnOk.EnableWindow (TRUE);
	else
		{
		m_StrName.GetWindowText (m_sName);
		m_BtnOk.EnableWindow (m_sName.GetLength () > 0);
		}
}

void CDlgJoinPublish::OnJoinUnpublish()
{

	ASSERT_VALID (this);

	// remove all players
	m_pJm->m_wndPlyrList.m_lstPlayers.ResetContent ();
	m_pJm->m_wndPlyrList.SetNumPlayers ();
	m_pJm->m_wndPlyrList.ShowWindow (SW_HIDE);

	// kill the connection
	theGame.Close ();
	memset (&(m_pJm->m_ID), 0, sizeof (m_pJm->m_ID));

	SetControls ();

	ASSERT_VALID (this);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgJoinGame dialog


CDlgJoinGame::CDlgJoinGame (CWnd* pParent /*=NULL*/)
	: CDialog (CDlgJoinGame::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgJoinGame)
	m_strAIlevel = "";
	m_strNumAI = "";
	m_strSize = "";
	m_strDesc = "";
	m_sPos = _T("");
	m_strType = _T("");
	//}}AFX_DATA_INIT

	m_sVer.LoadString ( IDS_EN_VER );
	m_sVer += VER_STRING;
	m_sVpVer.LoadString ( IDS_VP_VER );
	long lVer = CNetApi::GetVersion ();
	m_sVpVer += IntToCString (HIBYTE (HIWORD (lVer))) + "." +
												IntToCString (LOBYTE (HIWORD (lVer))) + "." +
												IntToCString (LOWORD (lVer));

	m_bAddrShowing = FALSE;
	m_bTimer = FALSE;
}

void CDlgJoinGame::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgJoinGame)
	DDX_Control(pDX, IDC_JOIN_PREV, m_btnPrev);
	DDX_Control(pDX, IDOK, m_btnOK);
	DDX_Control(pDX, IDC_JOIN_GAMES, m_LstGames);
	DDX_Text(pDX, IDC_JOIN_AI_LEVEL, m_strAIlevel);
	DDX_Text(pDX, IDC_JOIN_NUM_OPPONENTS, m_strNumAI);
	DDX_Text(pDX, IDC_JOIN_WORLD_SIZE, m_strSize);
	DDX_Text(pDX, IDC_JOIN_DESC, m_strDesc);
	DDX_Text(pDX, IDC_JOIN_POS, m_sPos);
	DDX_Text(pDX, IDC_JOIN_TYPE, m_strType);
	DDX_Text(pDX, IDC_PLYR_ADDR, m_sAddr);
	DDX_Text(pDX, IDC_PLYR_IS_ADDR, m_sIsAddr);
	DDX_Text(pDX, IDC_PLYR_VP_VER, m_sVpVer);
	DDX_Text(pDX, IDC_PLYR_EN_VER, m_sVer);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgJoinGame, CDialog)
	//{{AFX_MSG_MAP(CDlgJoinGame)
	ON_BN_CLICKED(IDC_JOIN_PREV, OnJoinPrev)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	ON_LBN_SELCHANGE(IDC_JOIN_GAMES, OnSelchangeJoinGames)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_PLYR_DO_ADDR, OnShowAddr)
	ON_LBN_DBLCLK(IDC_JOIN_GAMES, OnOK)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgJoinGame message handlers

void CDlgJoinGame::Create (CJoinMulti *pJm, UINT id, CWnd *pPar)
{

	ASSERT_VALID (this);

	m_pJm = pJm;
	CDialog::Create (id, pPar);
}

void CDlgJoinGame::SetControls ()
{

	ASSERT_VALID (this);

	CString sText;
	if (theNet.GetMode () != CNetApi::joined)
		{
		sText.LoadString (IDS_JOIN_JOIN);
		m_btnOK.SetWindowText (sText);
		sText.LoadString (IDS_PREV);
		m_btnPrev.SetWindowText (sText);
		}
	else
	  {
		sText.LoadString (IDS_CREATE_CHOOSE);
		m_btnOK.SetWindowText (sText);
		sText.LoadString (IDS_UNPUBLISH);
		m_btnPrev.SetWindowText (sText);
	  }

	if (theNet.GetMode () == CNetApi::joined)
		{
		sText.LoadString (IDS_JOIN1_TITLE_SEL);
		csPrintf (&sText, (const char *) m_pJm->m_sGameName);
		}
	else
		sText.LoadString (IDS_JOIN1_TITLE);
	SetWindowText (sText);
}

BOOL CDlgJoinGame::OnInitDialog()
{

	m_iLastInd = -1;

	CDialog::OnInitDialog();

	CenterWindow (theApp.m_pMainWnd);

	SetControls ();

	UpdateData (TRUE);
	CString sTemp;
	sTemp.LoadString ( IDS_YOUR_ADDR );
	m_sAddr = sTemp + theNet.GetAddress ();

	sTemp.LoadString ( IDS_IS_ADDR );
	m_sIsAddr = theNet.GetIServeAddress ();
	if ( m_sIsAddr.IsEmpty () )
		{
		m_bTimer = TRUE;
		SetTimer ( 109, 2000, NULL );
		m_sIsAddr.LoadString ( IDS_NONE );
		char sName [160];
		GetPrivateProfileString ( "TCP", "ServerAddress", "iserve.windward.net",
																					sName, 158, "vdmplay.ini" );
		csPrintf ( &m_sIsAddr, (const char *) sName );
		}
	m_sIsAddr = sTemp + m_sIsAddr;
	UpdateData (FALSE);

	CRect rect, rect2;
	GetWindowRect ( & rect );
	m_iWid = rect.Width ();
	m_iHtAddr = rect.Height ();
	GetDlgItem ( IDC_PLYR_ADDR )->GetWindowRect ( &rect2 );
	m_iHtNoAddr = rect2.top - rect.top;
	SetWindowPos ( NULL, 0, 0, m_iWid, m_iHtNoAddr, SWP_NOMOVE | SWP_NOZORDER );

	ASSERT_VALID (this);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDlgJoinGame::OnShowAddr ()
{

	m_bAddrShowing = ! m_bAddrShowing;

	SetWindowPos ( NULL, 0, 0, m_iWid, m_bAddrShowing ? m_iHtAddr : m_iHtNoAddr,
											SWP_NOMOVE | SWP_NOZORDER );
	CString sText;
	sText.LoadString ( m_bAddrShowing ? IDS_ADDR_OFF : IDS_ADDR );
	SetDlgItemText ( IDC_PLYR_DO_ADDR, sText );
}

void CDlgJoinGame::OnTimer(UINT nIDEvent) 
{

	// see if we've got it
	CString sIserve = theNet.GetIServeAddress ();
	if ( sIserve.IsEmpty () )
		return;

	// we've got it - kill the timer
	m_bTimer = FALSE;
	KillTimer ( nIDEvent );

	UpdateData (TRUE);
	m_sIsAddr.LoadString ( IDS_IS_ADDR );
	m_sIsAddr += sIserve;
	UpdateData (FALSE);
}

// actually << Prev
void CDlgJoinGame::OnJoinPrev ()
{

	ASSERT_VALID (this);

	// prev window if not joined
	if (theNet.GetMode () != CNetApi::joined)
		{
		ShowWindow (SW_HIDE);
		m_pJm->m_dlgJoinPublish.ShowWindow (SW_SHOW);
		}

	// we are going to unjoin from a game
	else
	  {
		if (m_pJm->m_wndPlyrList.m_hWnd != NULL)
			m_pJm->m_wndPlyrList.DestroyWindow ();
		theGame.Close ();
		memset (&(m_pJm->m_ID), 0, sizeof (m_pJm->m_ID));
		if (theNet.StartEnum ())
			{
			TRAP ();
			ShowWindow (SW_HIDE);
			m_pJm->m_dlgJoinPublish.ShowWindow (SW_SHOW);
			}
		SetControls ();
	  }
}

void CDlgJoinGame::OnOK()
{

	ASSERT_VALID (this);

	// if we did a prev, see if we changed games we are joining
	if (theNet.GetMode () == CNetApi::joined)
		{
		TRAP ();
		ShowWindow (SW_HIDE);
		if (m_pJm->m_iTyp == CCreateBase::load_join)
			m_pJm->m_dlgPickPlayer.ShowWindow (SW_SHOW);
		else
			m_pJm->m_dlgPickRace.ShowWindow (SW_SHOW);
		return;
		}

	// disconnect from present game
	if (theNet.GetMode () == CNetApi::joined)
		{
		TRAP ();
		theGame.Close ();
		memset (&(m_pJm->m_ID), 0, sizeof (m_pJm->m_ID));
		}

	// get the currently selected line
	int iInd = m_LstGames.GetCurSel ();
	if (iInd < 0)
		return;
	CGameInfo *pCgi = (CGameInfo *) m_LstGames.GetItemDataPtr (iInd);
	ASSERT_VALID (pCgi);
	m_pJm->m_ID = pCgi->m_ID;
	m_pJm->m_iAi = pCgi->m_iAIlevel;
	m_pJm->m_iNumAi = pCgi->m_iNumOpponents;
	m_pJm->m_iSize = pCgi->m_iWorldSize;
	m_pJm->m_iPos = pCgi->m_iPos;
	m_pJm->m_sGameName = pCgi->m_sName;
	m_pJm->m_sGameDesc = pCgi->m_sDesc;

	// set according to type of game
	if (pCgi->m_cFlags & (CNetPublish::fload | CNetPublish::finprogress))
		m_pJm->MakeLoad ();

	// tell the game we're joining
	theNet.StopEnum ();
	theGame.Open (TRUE);

	theGame.m_iAi = m_pJm->m_iAi;
	theGame.m_iSize = m_pJm->m_iSize;
	theGame.m_iPos = m_pJm->m_iPos;
	theGame.m_sGameName = m_pJm->m_sGameName;
	theGame.m_sGameDesc = m_pJm->m_sGameDesc;

	m_pJm->m_sGameName = pCgi->m_sName;
	theGame.GetMe()->SetName (m_pJm->m_sName);
	CNetJoin * pJn = CNetJoin::Alloc (theGame.GetMe (), FALSE);
	if (theNet.Join (&(m_pJm->m_ID), pJn))
		{
		TRAP ();
		delete [] pJn;
		SetControls ();
		theGame.Close ();
		memset (&(m_pJm->m_ID), 0, sizeof (m_pJm->m_ID));
		return;
		}
	delete [] pJn;

	// switch to the next set of windows
	if (m_pJm->m_wndPlyrList.m_hWnd == NULL)
		m_pJm->CreatePlyrList (m_pJm);

	ShowWindow (SW_HIDE);
	m_pJm->CreateWndChat ();
	m_pJm->m_wndPlyrList.ShowWindow (SW_SHOW);

	if (m_pJm->m_iTyp != CCreateBase::load_join)
		{
		if (m_pJm->m_dlgPickRace.m_hWnd == NULL)
			m_pJm->m_dlgPickRace.Create (m_pJm, this, CDlgPickRace::IDD, theApp.m_pMainWnd);
		m_pJm->m_dlgPickRace.ShowWindow (SW_SHOW);
		m_pJm->m_dlgPickRace.SetActiveWindow ();
		}
	else
		{
		m_pJm->m_dlgPickPlayer.Create ( m_pJm, IDD_PICK_PLAYER, theApp.m_pMainWnd );
		m_pJm->m_dlgPickPlayer.ShowWindow (SW_SHOW);
		m_pJm->m_dlgPickPlayer.SetActiveWindow ();
		}

	SetControls ();
}

void CDlgJoinGame::OnCancel()
{

	ASSERT_VALID (this);

	theGame.Close ();
	theNet.Close ( FALSE );
	CDialog::OnCancel();

	theApp.CreateMain ();
}

void CDlgJoinGame::OnHelp()
{

	theApp.WinHelp (HLP_JOIN_GAME, HELP_CONTEXT);
}

void CDlgJoinGame::OnSelchangeJoinGames()
{

	ASSERT_VALID (this);

	// if one is selected we can enable join
	int iInd = m_LstGames.GetCurSel ();
	m_btnOK.EnableWindow (iInd >= 0);

	// set up the button
	SetControls ();

	// set up the description
	UpdateData (TRUE);
	if (iInd < 0)
		{
_bad_data:
		m_strDesc = "";
		m_strNumAI = "";
		m_strAIlevel = "";
		m_strSize = "";
		m_strType = "";
		m_sPos = "";
		}
	else

		{
		CGameInfo *pCgi = (CGameInfo *) m_LstGames.GetItemDataPtr (iInd);
		if ( (pCgi == NULL) || (pCgi == (CGameInfo *) -1) )
			goto _bad_data;
		ASSERT_VALID (pCgi);
		m_pJm->m_sGameName = pCgi->m_sName;
		m_strDesc = pCgi->m_sDesc;
		m_strNumAI = IntToCString (pCgi->m_iNumOpponents);
		m_strAIlevel.LoadString (IDS_JOIN_AI_0 + pCgi->m_iAIlevel);
		m_strSize.LoadString (IDS_JOIN_SIZE_0 + pCgi->m_iWorldSize);

		INT ID = IDS_POS_MIN_CIV;
		switch (pCgi->m_iPos)
		  {
			case 1 :
				ID = IDS_POS_FULL_CIV;
				break;
			case 2 :
				ID = IDS_POS_MIN_COM;
				break;
			case 3 :
				ID = IDS_POS_FULL_COM;
				break;
		  }
		m_sPos.LoadString (ID);

		ID = IDS_JOIN_NEW;
		if (pCgi->m_cFlags & CNetPublish::fload)
			ID = IDS_JOIN_LOAD;
		else
			if (pCgi->m_cFlags & CNetPublish::finprogress)
				ID = IDS_JOIN_INPROGRESS;
		m_strType.LoadString (ID);
		}

	UpdateData (FALSE);
	m_iLastInd = iInd;
}

void CDlgJoinGame::OnSessionEnum (LPCVPSESSIONINFO pSi)
{

	ASSERT_VALID (this);
	CNetPublish * pPub = (CNetPublish *) (pSi->sessionName);

	// if its a different version - ignore it
	WORD wTst = 0;
#ifdef _DEBUG
	wTst |= CNetPublish::fdebug;
#endif
#ifdef _CHEAT
	wTst |= CNetPublish::fcheat;
#endif

	if ((pPub->m_iGameID != TLP_GAME_ID) || (pPub->m_cVerMajor != VER_MAJOR) ||
					(pPub->m_cVerMinor != VER_MINOR) || 
//					(pPub->m_cVerRelease < 10) ||
//					(pPub->m_cVerRelease != VER_RELEASE) || 
					((pPub->m_cFlags & (CNetPublish::fdebug | CNetPublish::fcheat)) != wTst))
		return;

	// do the passwords match?
	char const *pPw = pPub->GetPw ();
	if ((*pPw != 0) && (stricmp (pPw, m_pJm->m_dlgJoinPublish.m_strPw)))
		return;

	// join in progress isn't working
	if ( pPub->m_cFlags & CNetPublish::finprogress )
		return;

	// if it exists, change to the new values
	char const * pGame = pPub->GetGameName ();
	int iInd = m_LstGames.FindStringExact (-1, pGame);

	if (iInd >= 0)
		{
		CGameInfo *pCgi = (CGameInfo *) m_LstGames.GetItemDataPtr (iInd);
		ASSERT_VALID (pCgi);
		pCgi->m_iNumOpponents = pPub->m_iNumOpponents;
		pCgi->m_iAIlevel = pPub->m_iAIlevel;
		pCgi->m_iWorldSize = pPub->m_iWorldSize;
		pCgi->m_iPos = pPub->m_iPos;
		pCgi->m_iNumPlayers = pPub->m_iNumPlayers;
		pCgi->m_cFlags = pPub->m_cFlags;
		pCgi->m_sName = pGame;
		pCgi->m_sDesc = pPub->GetDesc ();
		pCgi->m_ID = pSi->sessionId;

		OnSelchangeJoinGames();
		return;
		}

	// add it to the list
	CGameInfo *pCgi = new CGameInfo ();
	pCgi->m_iNumOpponents = pPub->m_iNumOpponents;
	pCgi->m_iAIlevel = pPub->m_iAIlevel;
	pCgi->m_iWorldSize = pPub->m_iWorldSize;
	pCgi->m_iPos = pPub->m_iPos;
	pCgi->m_iNumPlayers = pPub->m_iNumPlayers;
	pCgi->m_cFlags = pPub->m_cFlags;
	pCgi->m_sName = pGame;
	pCgi->m_sDesc = pPub->GetDesc ();
	pCgi->m_ID = pSi->sessionId;

	iInd = m_LstGames.AddString (pGame);
	if (iInd >= 0)
		m_LstGames.SetItemDataPtr (iInd, pCgi);
	else
		delete pCgi;
}

void CDlgJoinGame::OnSessionClose (LPCVPSESSIONINFO pSi)
{

	// if we have started the game then we go back to join
	if (theGame.GetState () >= CGame::init)
		{
		TRAP;
		goto StartOver;
		}

	// if we have alist box - fix it up
	if (m_LstGames.m_hWnd != NULL)
		{
		// which one?
		int iInd = -1;
		BOOL bKill;
		if (pSi == NULL)
	  	{
			iInd = m_LstGames.FindStringExact (-1, m_pJm->m_sGameName);
			bKill = TRUE;
			}
		else
	  	{
			bKill = FALSE;
			int iMax = m_LstGames.GetCount ();
			for (int iTry=0; iTry<iMax; iTry++)
				{
				CGameInfo *pCgi = (CGameInfo *) m_LstGames.GetItemDataPtr (iTry);
				if (! memcmp (&(pSi->sessionId), &(pCgi->m_ID), sizeof (VPSESSIONID)))
					{
					iInd = iTry;
					if (! memcmp (&(pSi->sessionId), &(m_pJm->m_ID), sizeof (VPSESSIONID)))
						bKill = TRUE;
					break;
					}
				}
	  	}

		// if we didn't find it - leave
		if (iInd < 0)
			{
			SetControls ();
			return;
			}

		// remove it from the list of available games
		CGameInfo *pCgi = (CGameInfo *) m_LstGames.GetItemDataPtr (iInd);
		ASSERT_VALID (pCgi);
		m_LstGames.DeleteString (iInd);
		delete pCgi;
		OnSelchangeJoinGames ();

		// if it's not the game we joined - leave
		if (! bKill)
			{
			SetControls ();
			return;
			}
		}

StartOver:
	// if we had joined it - switch back to the join window
	theGame.Close ();
	memset (&(m_pJm->m_ID), 0, sizeof (m_pJm->m_ID));

	if (m_pJm->m_iTyp == CCreateBase::load_join)
		{
		if (m_pJm->m_dlgPickPlayer.m_hWnd != NULL)
			m_pJm->m_dlgPickPlayer.DestroyWindow ();
		}
	else
		if (m_pJm->m_dlgPickRace.m_hWnd != NULL)
			m_pJm->m_dlgPickRace.DestroyWindow ();

	m_pJm->HideDlgStatus ();
	theApp.CloseDlgChat ();
	if (m_pJm->m_wndPlyrList.m_hWnd != NULL)
		m_pJm->m_wndPlyrList.DestroyWindow ();

	if (theNet.StartEnum ())
		{
		theNet.Close ( FALSE );
		if (m_pJm->m_dlgJoinPublish.m_hWnd == NULL)
			{
			theApp.CreateMain ();
			return;
			}

		ShowWindow (SW_HIDE);
		m_pJm->m_dlgJoinPublish.ShowWindow (SW_SHOW);
		}
	else
		ShowWindow (SW_SHOW);

	SetControls ();
}

void CDlgJoinGame::OnDestroy()
{

	if ( m_bTimer )
		{
		m_bTimer = FALSE;
		KillTimer ( 109 );
		}

	// free up the ptrs in the listbox
	int iNum = m_LstGames.GetCount ();
	for (int iInd=0; iInd<iNum; iInd++)
		{
		CGameInfo *pCgi = (CGameInfo *) m_LstGames.GetItemDataPtr (iInd);
		ASSERT_VALID (pCgi);
		delete pCgi;
		}
	// so I'm paranoid - so shoot me
	m_LstGames.ResetContent ();

	CDialog::OnDestroy();
}


