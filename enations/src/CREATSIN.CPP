//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// creatsin.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "creatsin.h"
#include "player.h"
#include "help.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CCreateSingle

void CCreateSingle::Init ()
{

	ASSERT_VALID (this);
	theApp.Log ( "Create single-player game" );

	// set it as the server
	theGame.ctor ();
	theGame.SetServer (TRUE);
	theGame._SetIsNetGame ( FALSE );
	theGame.Open (TRUE);

	m_dlgCreateSingle.Create (this, CDlgCreateSingle::IDD, theApp.m_pMainWnd);
	theApp.DisableMain ();
}

void CCreateSingle::ClosePick ()
{

	CCreateBase::ClosePick ();
	CCreateNewBase::ClosePick ();

	if (m_dlgCreateSingle.m_hWnd != NULL)
		m_dlgCreateSingle.DestroyWindow ();
}

#ifdef _DEBUG
void CCreateSingle::AssertValid() const
{

	CCreateBase::AssertValid ();

	ASSERT_VALID (&m_dlgCreateSingle);
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CDlgCreateSingle dialog

CDlgCreateSingle::CDlgCreateSingle(CWnd* pParent /*=NULL*/)
	: CDialog (CDlgCreateSingle::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgCreateSingle)
	m_iAiLevel = 0;
	m_iSizeWorld = 0;
	m_iPosStart = 0;
	m_strAiNum = _T("");
	//}}AFX_DATA_INIT
}


void CDlgCreateSingle::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgCreateSingle)
	DDX_Control(pDX, IDOK, m_btnOk);
	DDX_Radio(pDX, IDC_CREATE_EASY, m_iAiLevel);
	DDX_Radio(pDX, IDC_CREATE_SMALL, m_iSizeWorld);
	DDX_Radio(pDX, IDC_PLAY_POS_0, m_iPosStart);
	DDX_Text(pDX, IDC_CREATE_NUM_AI, m_strAiNum);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgCreateSingle, CDialog)
	//{{AFX_MSG_MAP(CDlgCreateSingle)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	ON_EN_CHANGE(IDC_CREATE_NUM_AI, OnChangeCreateNumAi)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgCreateSingle message handlers

void CDlgCreateSingle::Create (CCreateSingle *pCs, UINT id, CWnd *pPar)
{

	ASSERT_VALID (this);

	m_pCs = pCs;
	CDialog::Create (id, pPar);
}

BOOL CDlgCreateSingle::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	UpdateData (TRUE);

	m_iAiLevel = theApp.GetProfileInt ("Create", "Difficultity", 0);
	m_iAiLevel = __minmax ( 0, 3, m_iAiLevel );
	m_strAiNum = theApp.GetProfileString ("Create", "ComputerOpponents", "2");
	int iNum = atoi ( m_strAiNum );
	iNum = __minmax ( 0, 20, iNum );
	m_strAiNum = IntToCString ( iNum );
	m_iSizeWorld = theApp.GetProfileInt ("Create", "Size", 1);
	m_iSizeWorld = __minmax ( 0, 2, m_iSizeWorld );
	m_iPosStart = theApp.GetProfileInt ("Create", "StartPosition", 1);
	m_iPosStart = __minmax ( 0, 3, m_iPosStart );

#ifndef	HACK_TEST_AI
	// shareware restrictions
	if ( ( theApp.IsShareware () ) || ( theApp.IsSecondDisk () ) )
		{
		m_iAiLevel = 1;
		m_strAiNum = "1";
		m_iSizeWorld = 1;
		m_iPosStart = 1;

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
#endif

	UpdateData (FALSE);
	
	CenterWindow (theApp.m_pMainWnd);

	OnChangeCreateNumAi();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgCreateSingle::OnChangeCreateNumAi() 
{

	UpdateData (TRUE);

	// num AI players must be > 0	
	int iNum = atoi (m_strAiNum);
	if (iNum <= 0)
		{
		m_btnOk.EnableWindow (FALSE);
		return;
		}

	// must be legit number
	CString sNum = IntToCString (iNum);
	if (sNum != m_strAiNum)
		{
		m_btnOk.EnableWindow (FALSE);
		return;
		}

	m_btnOk.EnableWindow (TRUE);
	
}

void CDlgCreateSingle::OnCancel() 
{
	
	ASSERT_VALID (this);

	theGame.Close ();
	CDialog::OnCancel();
	theApp.CreateMain ();
}

void CDlgCreateSingle::OnOK() 
{
	
	ASSERT_VALID (this);
	theApp.Log ( "Set game type" );

	UpdateData (TRUE);

	// is the number of AI players ok?
	m_pCs->m_iNumAi = atoi (m_strAiNum);
	if ( theApp.GetProfileInt ("Advanced", "IgnoreAiLimit", 0) == 0 )
		{
		int iMax = __max ( 0, theApp.GetCpuSpeed () - 100 );
		iMax = 3 + iMax / 20;
		if ( m_pCs->m_iNumAi > iMax * 2 )
			{
			CString sText;
			sText.LoadString ( IDS_AI_LIMIT_2_MAX );
			CString sNum = IntToCString ( 2 * iMax );
			csPrintf (&sText, (char const *) sNum);
			AfxMessageBox ( sText, MB_OK | MB_ICONSTOP );
			return;
			}
		else
			if ( m_pCs->m_iNumAi > iMax )
				{
				CString sText;
				sText.LoadString ( IDS_AI_LIMIT_MAX );
				CString sNum = IntToCString ( iMax );
				csPrintf (&sText, (char const *) sNum);
				if ( AfxMessageBox ( sText, MB_YESNO | MB_ICONSTOP ) != IDYES )
					return;
				}
		}

	// save the parameters we use
	theGame.m_iAi = m_pCs->m_iAi = m_iAiLevel;
	theGame.m_iSize = m_pCs->m_iSize = m_iSizeWorld;
	theGame.m_iPos = m_pCs->m_iPos = m_iPosStart;
	m_pCs->m_iNet = -1;

	theApp.WriteProfileInt ("Create", "Difficultity", m_iAiLevel);
	theApp.WriteProfileInt ("Create", "Size", m_iSizeWorld);
	theApp.WriteProfileInt ("Create", "ComputerOpponents", m_pCs->m_iNumAi);
	theApp.WriteProfileInt ("Create", "StartPosition", m_iPosStart);

	// switch to the pick race dialog
	if (m_pCs->m_dlgPickRace.m_hWnd == NULL)
		m_pCs->m_dlgPickRace.Create (m_pCs, this, CDlgPickRace::IDD, theApp.m_pMainWnd);

	m_pCs->m_dlgPickRace.ShowWindow (SW_SHOW);
	ShowWindow (SW_HIDE);
}

void CDlgCreateSingle::OnHelp() 
{
	
	theApp.WinHelp (HLP_CREATE_SINGLE, HELP_CONTEXT);
}


/////////////////////////////////////////////////////////////////////////////
// CCreateLoadSingle

void CCreateLoadSingle::Init ()
{

	ASSERT_VALID (this);
	theApp.Log ( "Load single-player game" );

	theApp.DisableMain ();

	theGame.ctor ();
	theGame.SetServer (TRUE);
	theGame._SetIsNetGame ( FALSE );

	// set it as the server
	if (theGame.LoadGame (theApp.m_pMainWnd, FALSE) != IDOK)
		{
		theApp.CreateMain ();
		return;
		}

	// if it's not a scenario pick the player
	if (theGame.GetScenario () < 0)
		{
		m_dlgPickPlayer.Create (this, IDD_PICK_PLAYER, theApp.m_pMainWnd);
		return;
		}

	theGame.SetHP (TRUE);
	theGame.SetAI (TRUE);
	theGame.GetMe()->SetState (CPlayer::ready);

	// it's a scenario - we know who we are
	if (theGame.StartGame (FALSE) != IDOK)
		{
		TRAP ();
		theApp.CloseWorld ();
		theApp.CreateMain ();
		}
}

void CCreateLoadSingle::ClosePick ()
{

	CCreateBase::ClosePick ();
	CCreateLoadBase::ClosePick ();
}

void CCreateLoadSingle::CloseAll ()
{

	CCreateBase::CloseAll ();
	CCreateLoadBase::CloseAll ();
}

#ifdef _DEBUG
void CCreateLoadSingle::AssertValid() const
{

	CCreateBase::AssertValid ();

	ASSERT_VALID (&m_dlgPickPlayer);
}
#endif


