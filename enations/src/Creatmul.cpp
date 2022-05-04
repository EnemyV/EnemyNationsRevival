//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// creatmul.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "player.h"
#include "help.h"
#include "error.h"

#include "creatmul.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


CCreateMultiBase::CCreateMultiBase (int iTyp) : CMultiBase (iTyp) 
{

	m_wndPlyrList.m_bServer = TRUE;
}

void CMultiBase::CreateWndChat () 
{ 

	theApp.GetDlgChat ()->EnableWindow (theGame.GetMyNetNum () != VP_LOCALMACHINE);
}

void CCreateMultiBase::ClosePick ()
{

	ASSERT_VALID (this);

	if (m_dlgCreatePublish.m_hWnd != NULL)
		m_dlgCreatePublish.DestroyWindow ();

	CMultiBase::ClosePick ();
}

void CCreateMultiBase::CloseAll ()
{

	ASSERT_VALID (this);

	ClosePick ();

	CMultiBase::CloseAll ();
}

#ifdef _DEBUG
void CCreateMultiBase::AssertValid() const
{

	ASSERT_VALID (&m_dlgCreatePublish);
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CCreateMulti
void CCreateMulti::Init ()
{

	ASSERT_VALID (this);
	theApp.Log ( "Create multi-player game" );

	theApp.DisableMain ();

	// set it as the server
	theGame.ctor ();
	theGame.SetServer (TRUE);
	theGame._SetIsNetGame ( TRUE );

	// load the data
	if ( ! theApp.LoadData () )
		{
		theApp.CreateMain ();
		return;
		}

	m_dlgCreateMulti.Create (this, CDlgCreateMulti::IDD, theApp.m_pMainWnd);
}

void CCreateMulti::ClosePick ()
{

	ASSERT_VALID (this);

	if (m_dlgCreateMulti.m_hWnd != NULL)
		m_dlgCreateMulti.DestroyWindow ();

	CCreateMultiBase::ClosePick ();
	CCreateNewBase::ClosePick ();
}

void CCreateMulti::CloseAll ()
{

	ASSERT_VALID (this);

	ClosePick ();

	CCreateMultiBase::CloseAll ();
	CCreateNewBase::CloseAll ();
}

#ifdef _DEBUG
void CCreateMulti::AssertValid() const
{

	CCreateBase::AssertValid ();

	ASSERT_VALID (&m_dlgCreateMulti);
	ASSERT_VALID (&m_dlgCreatePublish);
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CDlgCreateMulti dialog

CDlgCreateMulti::CDlgCreateMulti(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCreateMulti::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgCreateMulti)
	m_sDesc = _T("");
	m_iAi = -1;
	m_sName = _T("");
	m_iSize = -1;
	m_iPos = -1;
	m_sPlayer = _T("");
	m_strNumAI = _T("");
	//}}AFX_DATA_INIT
}


void CDlgCreateMulti::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgCreateMulti)
	DDX_Control(pDX, IDOK, m_btnOk);
	DDX_Text(pDX, IDC_CREATE_DESC, m_sDesc);
	DDX_Radio(pDX, IDC_CREATE_EASY, m_iAi);
	DDX_Text(pDX, IDC_CREATE_NAME, m_sName);
	DDX_Radio(pDX, IDC_CREATE_SMALL, m_iSize);
	DDX_Radio(pDX, IDC_PLAY_POS_0, m_iPos);
	DDX_Text(pDX, IDC_CREATE_PLAYER, m_sPlayer);
	DDX_Text(pDX, IDC_CREATE_NUM_AI, m_strNumAI);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgCreateMulti, CDialog)
	//{{AFX_MSG_MAP(CDlgCreateMulti)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	ON_EN_CHANGE(IDC_CREATE_DESC, OnChangeCreateText)
	ON_EN_CHANGE(IDC_CREATE_NAME, OnChangeCreateText)
	ON_EN_CHANGE(IDC_CREATE_PLAYER, OnChangeCreateText)
	ON_EN_CHANGE(IDC_CREATE_NUM_AI, OnChangeCreateText)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgCreateMulti message handlers

void CDlgCreateMulti::Create (CCreateMulti *pCm, UINT id, CWnd *pPar)
{

	ASSERT_VALID (this);

	m_pCm = pCm;
	CDialog::Create (id, pPar);
}

void CDlgCreateMulti::OnChangeCreateText () 
{
	
	UpdateData (TRUE);

	if ((m_sName.GetLength () <= 0) || (m_sDesc.GetLength () <= 0) ||
													(m_sPlayer.GetLength () <= 0))
		{
		m_btnOk.EnableWindow (FALSE);
		return;
		}

	// if no AI players grey the radio buttons
	int iNum = atoi (m_strNumAI);
	::EnableWindow ( ::GetDlgItem (m_hWnd, IDC_CREATE_MODERATE), iNum > 0 );
	if ( ( ! theApp.IsShareware () ) && ( ! theApp.IsSecondDisk () ) )
		{
		::EnableWindow ( ::GetDlgItem (m_hWnd, IDC_CREATE_EASY), iNum > 0 );
		::EnableWindow ( ::GetDlgItem (m_hWnd, IDC_CREATE_DIFFICULT), iNum > 0 );
		::EnableWindow ( ::GetDlgItem (m_hWnd, IDC_CREATE_IMPOSSIBLE), iNum > 0 );
		}

	// must be legit number
	CString sNum = IntToCString (iNum);
	if ( (! m_strNumAI.IsEmpty ()) && (sNum != m_strNumAI) )
		{
		m_btnOk.EnableWindow (FALSE);
		return;
		}

	m_btnOk.EnableWindow (TRUE);
}

void CDlgCreateMulti::OnOK() 
{
	
	ASSERT_VALID (this);
	theApp.Log ( "Set game type" );

	UpdateData (TRUE);

	// the strings can only be so long
	if (m_sName.GetLength () >= iMaxNameLen)
		{
		TRAP ();
		AfxMessageBox (IDS_NAME_TOO_LONG, MB_OK | MB_ICONSTOP);
		return;
		}
	if (sizeof (CNetPublish)+4+m_sName.GetLength()+m_sDesc.GetLength() > VP_MAXSENDDATA)
		{
		TRAP ();
		AfxMessageBox (IDS_DESC_TOO_LONG, MB_OK | MB_ICONSTOP);
		return;
		}

	m_pCm->m_iNumAi = atoi (m_strNumAI);
	if ( theApp.GetProfileInt ("Advanced", "IgnoreAiLimit", 0) == 0 )
		{
		int iMax = __max ( 0, theApp.GetCpuSpeed () - 100 );
		iMax = 3 + iMax / 20;
		if ( ! theGame.HaveHP () )
			iMax *= 2;
		if ( m_pCm->m_iNumAi > iMax * 2 )
			{
			TRAP ();
			CString sText;
			sText.LoadString ( IDS_AI_LIMIT_2_MAX );
			CString sNum = IntToCString ( 2 * iMax );
			csPrintf (&sText, (char const *) sNum);
			AfxMessageBox ( sText, MB_OK | MB_ICONSTOP );
			return;
			}
		else
			if ( m_pCm->m_iNumAi > iMax )
				{
				TRAP ();
				CString sText;
				sText.LoadString ( IDS_AI_LIMIT_MAX );
				CString sNum = IntToCString ( iMax );
				csPrintf (&sText, (char const *) sNum);
				if ( AfxMessageBox ( sText, MB_YESNO | MB_ICONSTOP ) != IDYES )
					return;
				}
		}

	// save the parameters we use
	m_pCm->m_iAi = m_iAi;
	m_pCm->m_iSize = m_iSize;
	m_pCm->m_iPos = m_iPos;
	m_pCm->m_iNet = -1;
	m_pCm->m_sGameName = m_sName;
	m_pCm->m_sGameDesc = m_sDesc;
	m_pCm->m_sName = m_sPlayer;

	theApp.WriteProfileInt ("Create", "Difficultity", m_iAi);
	theApp.WriteProfileInt ("Create", "Size", m_iSize);
	theApp.WriteProfileInt ("Create", "ComputerOpponents", m_pCm->m_iNumAi);
	theApp.WriteProfileInt ("Create", "StartPosition", m_iPos);
	theApp.WriteProfileString ("Create", "Name", m_sPlayer);
	theApp.WriteProfileString ("Create", "GameName", m_sName);
	theApp.WriteProfileString ("Create", "GameDesc", m_sDesc);

	// switch to the pick race dialog
	if (m_pCm->m_dlgCreatePublish.m_hWnd == NULL)
		m_pCm->m_dlgCreatePublish.Create (m_pCm, this, CDlgCreatePublish::IDD, theApp.m_pMainWnd);

	m_pCm->m_dlgCreatePublish.ShowWindow (SW_SHOW);
	ShowWindow (SW_HIDE);
}

void CDlgCreateMulti::OnCancel() 
{

	ASSERT_VALID (this);

	theGame.Close ();
	CDialog::OnCancel();
	theApp.CreateMain ();
}

BOOL CDlgCreateMulti::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	UpdateData (TRUE);

	m_iAi = theApp.GetProfileInt ("Create", "Difficultity", 0);
	m_iAi = __minmax ( 0, 3, m_iAi );
	m_strNumAI = theApp.GetProfileString ("Create", "ComputerOpponents", "2");
	int iNum = atoi ( m_strNumAI );
	iNum = __minmax ( 0, 20, iNum );
	m_strNumAI = IntToCString ( iNum );
	m_iSize = theApp.GetProfileInt ("Create", "Size", 1);
	m_iSize = __minmax ( 0, 2, m_iSize );
	m_iPos = theApp.GetProfileInt ("Create", "StartPosition", 1);
	m_iPos = __minmax ( 0, 3, m_iPos );
	m_sPlayer = theApp.GetProfileString ("Create", "Name");
	m_sName = theApp.GetProfileString ("Create", "GameName");
	m_sDesc = theApp.GetProfileString ("Create", "GameDesc");

	// shareware restrictions
	if ( ( theApp.IsShareware () ) || ( theApp.IsSecondDisk () ) )
		{
		m_iAi = 1;
		m_strNumAI = "1";
		m_iSize = 1;
		m_iPos = 1;

		GetDlgItem (IDC_CREATE_EASY)->EnableWindow (FALSE);
		GetDlgItem (IDC_CREATE_DIFFICULT)->EnableWindow (FALSE);
		GetDlgItem (IDC_CREATE_IMPOSSIBLE)->EnableWindow (FALSE);
		GetDlgItem (IDC_CREATE_NUM_AI)->EnableWindow (FALSE);
		GetDlgItem (IDC_PLAY_POS_0)->EnableWindow (FALSE);
		GetDlgItem (IDC_PLAY_POS_2)->EnableWindow (FALSE);
		GetDlgItem (IDC_PLAY_POS_3)->EnableWindow (FALSE);
		GetDlgItem (IDC_CREATE_SMALL)->EnableWindow (FALSE);
		GetDlgItem (IDC_CREATE_LARGE)->EnableWindow (FALSE);
		}

	UpdateData (FALSE);
	
	CenterWindow (theApp.m_pMainWnd);
	OnChangeCreateText ();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgCreateMulti::OnHelp() 
{
	
	theApp.WinHelp (HLP_CREATE_MULTI, HELP_CONTEXT);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgCreatePublish dialog


CDlgCreatePublish::CDlgCreatePublish(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCreatePublish::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgCreatePublish)
	m_iConn = 0;
	m_iJoin = 0;
	m_bServer = FALSE;
	m_strPw = _T("");
	//}}AFX_DATA_INIT

	m_prevConn = -2;
	m_idOK = -2;
}

CDlgCreatePublish::~CDlgCreatePublish ()
{

}

void CDlgCreatePublish::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgCreatePublish)
	DDX_Control(pDX, IDC_CREATE_ADV, m_btnAdv);
	DDX_Control(pDX, IDC_UNPUBLISH, m_btnUnpub);
	DDX_Control(pDX, IDOK, m_btnOk);
	DDX_Radio(pDX, IDC_CREATE_TCPIP, m_iConn);
	DDX_Radio(pDX, IDC_JOIN_NO_MORE, m_iJoin);
	DDX_Check(pDX, IDC_JOIN_SERVER, m_bServer);
	DDX_Text(pDX, IDC_CREATE_PW, m_strPw);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgCreatePublish, CDialog)
	//{{AFX_MSG_MAP(CDlgCreatePublish)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	ON_BN_CLICKED(IDC_CREATE_ADV, OnCreateAdv)
	ON_BN_CLICKED(IDC_UNPUBLISH, OnUnpublish)
	ON_BN_CLICKED(IDC_CREATE_DIRECT, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_DIRECTPLAY, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_IPX, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_MODEM, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_NETBIOS, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_TAPI, OnRadioChange)
	ON_BN_CLICKED(IDC_CREATE_TCPIP, OnRadioChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgCreatePublish message handlers

void CDlgCreatePublish::Create (CCreateMultiBase * pCm, CDialog *pPrev, UINT id, CWnd *pPar)
{

	ASSERT_VALID (this);

	m_pCm = pCm;
	m_pPrev = pPrev;
	CDialog::Create (id, pPar);
}

BOOL CDlgCreatePublish::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	UpdateData (TRUE);

	// make sure the protocol is ok
	m_iConn = theApp.GetProfileInt ("Create", "Connection", 0);
	m_iConn = __minmax ( 0, NUM_PROTOCOLS - 1, m_iConn );
	if (! CNetApi::SupportsProtocol (aPr [m_iConn]) )
		for (int iRad=0; iRad<NUM_PROTOCOLS; iRad++)
			if (CNetApi::SupportsProtocol (aPr[iRad]))
				{
				m_iConn = iRad;
				break;
				}

	m_iJoin = theApp.GetProfileInt ("Create", "JoinUntil", 0);
	m_iJoin = __minmax ( 0, 2, m_iJoin );
	m_bServer = theApp.GetProfileInt ("Create", "NoPlayer", FALSE);

	// if shareware - no server
	if ( ( theApp.IsShareware () ) || ( theApp.IsSecondDisk () ) )
		{
		m_bServer = FALSE;
		GetDlgItem (IDC_JOIN_SERVER)->EnableWindow (FALSE);
		m_iJoin = 0;
		GetDlgItem (IDC_JOIN_UNTIL_COLONIZE)->EnableWindow (FALSE);
		GetDlgItem (IDC_JOIN_FINAL_APPROVAL)->EnableWindow (FALSE);
		}

	// BUGBUG - this doesn't work so it's going away
	GetDlgItem (IDC_JOIN_FINAL_APPROVAL)->ShowWindow ( SW_HIDE );
	GetDlgItem (IDC_JOIN_FINAL_APPROVAL)->EnableWindow (FALSE);

	UpdateData (FALSE);
	
	CenterWindow (theApp.m_pMainWnd);
	UpdateBtns ();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgCreatePublish::OnCreateAdv() 
{
	
	UpdateData (TRUE);

	vpAdvDialog (m_hWnd, aPr[m_iConn], TRUE);
}

void CDlgCreatePublish::OnRadioChange() 
{

}

void CDlgCreatePublish::UpdateBtns() 
{
	
	ASSERT_VALID (this);

	UpdateData (TRUE);

	m_btnAdv.EnableWindow (theNet.GetMode () != CNetApi::opened);

	// set OK button text depending
	int ID, ID2;
	if (theNet.GetMode () == CNetApi::opened)
		{
		ID = IDS_CREATE_CREATE;
		ID2 = IDS_UNPUBLISH;
		}
	else
		{
		ID = IDS_CREATE_PUBLISH;
		ID2 = IDS_PREV;
		}
	if (ID != m_idOK)
		{
		m_idOK = ID;
		CString sBtn;
		sBtn.LoadString (ID);
		m_btnOk.SetWindowText (sBtn);

		sBtn.LoadString (ID2);
		m_btnUnpub.SetWindowText (sBtn);
		}

	// if we're connected can't change the protocol
	if (theNet.GetMode () != m_prevConn)
		{
		m_prevConn = theNet.GetMode ();
		if (theNet.GetMode () == CNetApi::opened)
			{
			for (int iRad=0; iRad<NUM_PROTOCOLS; iRad++)
				if (iRad == m_pCm->m_iNet)
					GetDlgItem (IDC_CREATE_TCPIP + iRad)->EnableWindow (TRUE);
				else
					GetDlgItem (IDC_CREATE_TCPIP + iRad)->EnableWindow (FALSE);
			}
		else
			{
			for (int iRad=0; iRad<NUM_PROTOCOLS; iRad++)
				if (CNetApi::SupportsProtocol (aPr[iRad]))
					GetDlgItem (IDC_CREATE_TCPIP + iRad)->EnableWindow (TRUE);
				else
					GetDlgItem (IDC_CREATE_TCPIP + iRad)->EnableWindow (FALSE);
			}
		}
}

void CDlgCreatePublish::OnUnpublish() 
{
	
	ASSERT_VALID (this);

	// if its connected then unpublish
	if (theNet.GetMode () == CNetApi::opened)
		{
		// remove all players
		m_pCm->m_wndPlyrList.m_lstPlayers.ResetContent ();
		m_pCm->m_wndPlyrList.SetNumPlayers ();
		m_pCm->m_wndPlyrList.ShowWindow (SW_HIDE);

		// kill the connection
		theGame.Close ();
		}

	// otherwise go to the prev dialog
	else
	  {
		if (m_pCm->m_iTyp == CCreateBase::load_multi)
			{
			TRAP ();
			}
		else
			{
			GetNew()->m_dlgCreateMulti.ShowWindow (SW_SHOW);
			ShowWindow (SW_HIDE);
			}
	  }

	UpdateBtns ();
}

void CDlgCreatePublish::OnOK() 
{

	ASSERT_VALID (this);
	char sBuf [40];
	sprintf ( sBuf, "Published game protocol: %d", m_iConn);
	theApp.Log ( sBuf );

	// save the parameters we use
	UpdateData (TRUE);
	m_pCm->m_iNet = m_iConn;
	m_pCm->m_iJoinUntil = m_iJoin;
	theGame.m_sPwJoin = m_pCm->m_sPw = m_strPw;

	theApp.WriteProfileInt ("Create", "Connection", m_iConn);
	theApp.WriteProfileInt ("Create", "JoinUntil", m_iJoin);
	theApp.WriteProfileInt ("Create", "NoPlayer", m_bServer);

	// create the game
	//   only publish if not yet published (could be here from a Prev)
	if (theNet.GetMode () == CNetApi::closed)
		{
		CNetPublish *pMsg = CNetPublish::Alloc (m_pCm);
		if (m_pCm->m_iTyp == CCreateBase::load_multi)
			{
			pMsg->m_cFlags |= CNetPublish::fload;
			pMsg->m_iNumPlayers = theGame.GetAll().GetCount ();
			}
		BOOL bErr = theNet.OpenServer (aPr[m_pCm->m_iNet], theApp.m_wndMain.m_hWnd, 
																					(LPCSTR) pMsg, (LPVOID) TRUE, NULL);
		delete [] pMsg;
		if (bErr)
			return;

		BOOL bServer = m_bServer;
		if (m_pCm->m_iTyp == CCreateBase::load_multi)
			bServer = TRUE;
		CPlayer * pPlrMe = theGame._GetMe ();
		theGame.Open (! bServer);
		theGame.m_iAi = m_pCm->m_iAi;
		theGame.m_iSize = m_pCm->m_iSize;
		theGame.m_iPos = m_pCm->m_iPos;
		theGame.m_sGameName = m_pCm->m_sGameName;
		theGame.m_sGameDesc = m_pCm->m_sGameDesc;

		if (! m_bServer)
			{

			// grab our player as ours
			if (m_pCm->m_iTyp == CCreateBase::load_multi)
				{
				// join the game
				theGame._SetMe ( pPlrMe );
				theGame.SetHP ( TRUE );
				theGame._SetServer ( pPlrMe );
				theGame.SetServer ( TRUE );
				pPlrMe->SetState (CPlayer::ready);
				}

			theGame.GetMe()->SetName (m_pCm->m_sName);
			CNetJoin * pJn = CNetJoin::Alloc (theGame.GetMe (), TRUE);
			theGame.GetMe()->SetNetNum (theNet.AddPlayer (pJn));
			delete [] pJn;
			}
		}

	// switch to the players & pick race/player dialog
	if (m_pCm->m_wndPlyrList.m_hWnd == NULL)
		m_pCm->CreatePlyrList (m_pCm);
	// tell the player window we're here
	if ( theGame.HaveHP () )
		m_pCm->m_wndPlyrList.AddPlayer (theGame.GetMe ());

	if ( m_pCm->m_iTyp != CCreateBase::load_multi )
		if ((theGame.HaveHP ()) && (GetNew()->m_dlgPickRace.m_hWnd == NULL))
			GetNew ()->m_dlgPickRace.Create (GetNew (), this, CDlgPickRace::IDD, theApp.m_pMainWnd);

	m_pCm->CreateWndChat ();
	m_pCm->m_wndPlyrList.ShowWindow (SW_SHOW);

	if ( m_pCm->m_iTyp == CCreateBase::load_multi )
		{
		m_pCm->GetDlgStatus()->SetMsg (IDS_READY_TO_GO);
		m_pCm->GetDlgStatus()->SetPer ( 100 );
		m_pCm->ShowDlgStatus ();
		}
	else
		if (theGame.HaveHP ())
			{
			GetNew ()->m_dlgPickRace.ShowWindow (SW_SHOW);
			GetNew ()->m_dlgPickRace.SetActiveWindow ();
			}

	// no HP on the server
	if ( ! theGame.HaveHP () )
	  theApp.ReadyToCreate ();
	else
		{
		UpdateBtns ();
		ShowWindow (SW_HIDE);
		}
}

void CDlgCreatePublish::OnCancel() 
{

	ASSERT_VALID (this);

	theGame.Close ();
	theNet.Close ( FALSE );
	CDialog::OnCancel();
	theApp.CreateMain ();
}

void CDlgCreatePublish::OnHelp() 
{
	
	theApp.WinHelp (HLP_CREATE_MULTI_PUBLISH, HELP_CONTEXT);
}


/////////////////////////////////////////////////////////////////////////////
// CCreateLoadMulti

void CCreateLoadMulti::Init ()
{

	ASSERT_VALID (this);
	theApp.Log ( "Load multi-player game" );

	theApp.DestroyMain ();

	// set it as the server
	theGame.ctor ();
	theGame.SetServer (TRUE);
	theGame._SetIsNetGame ( TRUE );

	// load the data
	if ( ! theApp.LoadData () )
		{
		theApp.CreateMain ();
		return;
		}

	// set it as the server
	if (theGame.LoadGame (theApp.m_pMainWnd, FALSE) != IDOK)
		{
		theApp.CreateMain ();
		return;
		}

	// disappear the status window till its time to go
	HideDlgStatus ();

	theNet.SetMode (CNetApi::closed);

	m_iAi = theGame.m_iAi;
	m_iNumAi = theGame.GetAi ().GetCount ();
	m_iSize = theGame.m_iSize;
	m_iPos = theGame.m_iPos;
	m_iNumPlayers = theGame.GetAll ().GetCount ();

	if (theGame.m_sGameName.IsEmpty ())
		m_sGameName = theApp.GetProfileString ("Create", "GameName");
	else
		m_sGameName = theGame.m_sGameName;
	if (theGame.m_sGameDesc.IsEmpty ())
		m_sGameDesc = theApp.GetProfileString ("Create", "GameDesc");
	else
		m_sGameDesc = theGame.m_sGameDesc;

	// switch to the name dialog
	if (m_dlgLoadName.m_hWnd == NULL)
		m_dlgLoadName.Create (this, CDlgLoadMulti::IDD, theApp.m_pMainWnd);

	m_dlgLoadName.ShowWindow (SW_SHOW);
}

void CCreateLoadMulti::ClosePick ()
{

	ASSERT_VALID (this);

	m_dlgLoadName.DestroyWindow ();

	CCreateMultiBase::ClosePick ();
	CCreateLoadBase::ClosePick ();
}

void CCreateLoadMulti::CloseAll ()
{

	ASSERT_VALID (this);

	ClosePick ();

	CCreateMultiBase::CloseAll ();
	CCreateLoadBase::CloseAll ();
}

#ifdef _DEBUG
void CCreateLoadMulti::AssertValid() const
{

	CCreateBase::AssertValid ();

	ASSERT_VALID (&m_dlgPickPlayer);
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CDlgLoadMulti dialog


CDlgLoadMulti::CDlgLoadMulti(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgLoadMulti::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgLoadMulti)
	m_sDesc = _T("");
	m_sName = _T("");
	//}}AFX_DATA_INIT
}


void CDlgLoadMulti::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgLoadMulti)
	DDX_Control(pDX, IDOK, m_btnOK);
	DDX_Text(pDX, IDC_CREATE_DESC, m_sDesc);
	DDX_Text(pDX, IDC_CREATE_NAME, m_sName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgLoadMulti, CDialog)
	//{{AFX_MSG_MAP(CDlgLoadMulti)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	ON_EN_CHANGE(IDC_CREATE_DESC, OnChangeText)
	ON_EN_CHANGE(IDC_CREATE_NAME, OnChangeText)
	ON_BN_CLICKED(IDC_UNPUBLISH, OnPrev)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgLoadMulti message handlers

void CDlgLoadMulti::OnChangeText() 
{
	
	UpdateData (TRUE);
	m_btnOK.EnableWindow ((m_sName.GetLength () > 0) && (m_sDesc.GetLength () > 0));
}

void CDlgLoadMulti::Create (CCreateLoadMulti * pLm, UINT id, CWnd *pPar)
{

	ASSERT_VALID (this);

	m_pLm = pLm;
	CDialog::Create (id, pPar);
}

BOOL CDlgLoadMulti::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	UpdateData (TRUE);
	m_sName = m_pLm->m_sGameName;
	m_sDesc = m_pLm->m_sGameDesc;
	UpdateData (FALSE);
	
	CenterWindow (theApp.m_pMainWnd);
	OnChangeText ();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgLoadMulti::OnOK() 
{
	
	UpdateData (TRUE);
	theApp.Log ( "Set load game type" );

	theGame.m_sGameName = m_pLm->m_sGameName = m_sName;
	theGame.m_sGameDesc = m_pLm->m_sGameDesc = m_sDesc;
	theApp.WriteProfileString ("Create", "GameName", m_sName);
	theApp.WriteProfileString ("Create", "GameDesc", m_sDesc);

	// switch to the pick race dialog
	m_pLm->m_dlgPickPlayer.Create ( m_pLm, IDD_PICK_PLAYER, theApp.m_pMainWnd );

	ShowWindow (SW_HIDE);
}

void CDlgLoadMulti::OnCancel() 
{
	
	TRAP ();
	theGame.Close ();
	CDialog::OnCancel();
	theApp.CloseWorld ();
}

void CDlgLoadMulti::OnPrev() 
{
	// TODO: Add your control notification handler code here
	TRAP ();
}

void CDlgLoadMulti::OnHelp() 
{
	
	theApp.WinHelp (HLP_LOAD_MULTI, HELP_CONTEXT);
}


