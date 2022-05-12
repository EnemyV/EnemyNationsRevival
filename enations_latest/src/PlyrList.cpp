// PlyrList.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "player.h"
#include "PlyrList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CConquerApp::ShowPlayerList ()
{

	if ( ( ! theGame.IsNetGame ()) || ( ! theGame.AmServer ()) )
		return;

	if ( m_pdlgPlyrList == NULL )
		m_pdlgPlyrList = new CDlgPlyrList ( &(theApp.m_wndMain) );

	if ( m_pdlgPlyrList->m_hWnd == NULL )
		m_pdlgPlyrList->Create (IDD_PLYR_LIST, &m_wndMain);

	m_pdlgPlyrList->ShowWindow (SW_SHOW);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgPlyrList dialog


CDlgPlyrList::CDlgPlyrList(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgPlyrList::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgPlyrList)
	m_btnTakeOver = FALSE;
	m_strPassWord = _T("");
	//}}AFX_DATA_INIT
}


void CDlgPlyrList::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgPlyrList)
	DDX_Control(pDX, IDC_PLYR_LIST, m_lstPlayers);
	DDX_Control(pDX, IDC_PLYR_KILL, m_btnKill);
	DDX_Check(pDX, IDC_PLYR_TAKE_OVER, m_btnTakeOver);
	DDX_Text(pDX, IDC_PLYR_PASSWORD, m_strPassWord);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPlyrList, CDialog)
	//{{AFX_MSG_MAP(CDlgPlyrList)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_PLYR_KILL, OnPlyrKill)
	ON_LBN_SELCHANGE(IDC_PLYR_LIST, OnSelchangePlyrList)
	ON_BN_CLICKED(IDC_PLYR_MINIMIZE, OnPlyrMinimize)
	ON_BN_CLICKED(IDC_PLYR_OPTIONS, OnPlyrOptions)
	ON_BN_CLICKED(IDC_PLYR_PAUSE, OnPlyrPause)
	ON_EN_CHANGE(IDC_PLYR_PASSWORD, OnChangePlyrPassword)
	ON_BN_CLICKED(IDC_PLYR_TAKE_OVER, OnPlyrTakeOver)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPlyrList message handlers

BOOL CDlgPlyrList::OnInitDialog() 
{

	CDialog::OnInitDialog();
	UpdateData (TRUE);
	
	m_btnKill.EnableWindow (FALSE);

	if ( ( theApp.IsShareware () ) || ( theApp.IsSecondDisk () ) )
		{
		m_btnTakeOver = FALSE;
		m_strPassWord = "";
		GetDlgItem (IDC_PLYR_TAKE_OVER)->EnableWindow (FALSE);
		GetDlgItem (IDC_PLYR_PASSWORD)->EnableWindow (FALSE);
		}
	else
		{
		m_btnTakeOver = theGame.GetNetJoin () == CGame::any;
		m_strPassWord = theGame.m_sPwJoin;
		}

	// list all players
	POSITION pos;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
		{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		int iInd = m_lstPlayers.AddString (pPlr->GetName ());
		m_lstPlayers.SetItemDataPtr (iInd, pPlr);
		}

	UpdateData (FALSE);
	CenterWindow ();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgPlyrList::OnDestroy()
{

	theApp.m_pdlgPlyrList = NULL;

	CDialog::OnDestroy();
}

void CDlgPlyrList::NameChange ( CPlayer * pPlyr )
{

	if ( (m_hWnd == NULL) || (m_lstPlayers.m_hWnd == NULL) )
		return;
		
	int iNum = m_lstPlayers.GetCount ();
	for (int iInd=0; iInd<iNum; iInd++)
		if (m_lstPlayers.GetItemDataPtr (iInd) == (void *) pPlyr)
			{
			m_lstPlayers.DeleteString ( iInd );
			int iInd = m_lstPlayers.AddString ( pPlyr->GetName () );
			m_lstPlayers.SetItemDataPtr ( iInd, pPlyr );
			return;
			}
	TRAP ();
}

void CDlgPlyrList::RemovePlayer ( CPlayer * pPlyr )
{

	if ( (m_hWnd == NULL) || (m_lstPlayers.m_hWnd == NULL) )
		return;

	int iNum = m_lstPlayers.GetCount ();
	for (int iInd=0; iInd<iNum; iInd++)
		if (m_lstPlayers.GetItemDataPtr (iInd) == (void *) pPlyr)
			{
			m_lstPlayers.DeleteString ( iInd );
			return;
			}
	TRAP ();
}

void CDlgPlyrList::OnPlyrKill() 
{
	
	int iInd = m_lstPlayers.GetCurSel ();
	if (iInd < 0)
		{
		ASSERT (FALSE);
		return;
		}
		
	CPlayer * pPlr = (CPlayer *) m_lstPlayers.GetItemDataPtr (iInd);
	ASSERT_VALID (pPlr);
	if (pPlr->IsAI ())
		{
		ASSERT (FALSE);
		return;
		}

	if (AfxMessageBox (IDS_DROP_PLAYER, MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;

	// ok, this will drop this guy and tell everyone when he is dropped
	// it will also have the AI take over
	m_btnKill.EnableWindow (FALSE);
	theNet.DeletePlayer (pPlr->GetNetNum ());
}

void CDlgPlyrList::OnSelchangePlyrList() 
{
	
	int iInd = m_lstPlayers.GetCurSel ();
	if (iInd < 0)
		{
		m_btnKill.EnableWindow (FALSE);
		return;
		}
		
	CPlayer * pPlr = (CPlayer *) m_lstPlayers.GetItemDataPtr (iInd);
	ASSERT_VALID (pPlr);
	if ( (pPlr->IsAI ()) || (pPlr->IsMe ()) )
		{
		m_btnKill.EnableWindow (FALSE);
		return;
		}

	m_btnKill.EnableWindow (TRUE);
}

void CDlgPlyrList::OnPlyrMinimize() 
{
	
	theApp.m_pMainWnd->ShowWindow (SW_MINIMIZE);
}

void CDlgPlyrList::OnPlyrOptions() 
{
	
	if (theApp.m_pdlgFile == NULL)
		theApp.m_pdlgFile = new CDlgFile (&theApp.m_wndMain);

	if (theApp.m_pdlgFile->m_hWnd == NULL)
		theApp.m_pdlgFile->Create (iWinType == W32s ? IDD_FILE1 : IDD_FILE, &theApp.m_wndMain);

	theApp.m_pdlgFile->ShowWindow (theApp.m_wndMain.IsIconic () ? SW_SHOW : SW_RESTORE);
	theApp.m_pdlgFile->SetFocus ();
}

void CDlgPlyrList::OnPlyrPause() 
{
	
	BOOL bPause = theGame.DoOper ();

	theApp.m_wndMain._EnableGameWindows ( ! bPause );

	theApp.m_wndMain._OnPause ( bPause );

	theApp.GetDlgPause ()->Show ( bPause ? CDlgPause::server : CDlgPause::off );
}

void CDlgPlyrList::OnChangePlyrPassword() 
{

	if ( ( theApp.IsShareware () ) || ( theApp.IsSecondDisk () ) )
		return;

	UpdateData (TRUE);
	if ( theGame.m_sPwJoin == m_strPassWord )
		return;

	theGame.m_sPwJoin = m_strPassWord;

	if ( (theGame.GetNetJoin () != CGame::any) || (theGame.GetAi().GetCount () <= 0) )
		return;

	// update with the new password
	CNetPublish *pMsg = CNetPublish::Alloc ( &theGame );
	theNet.UpdateSessionData (pMsg);
	delete [] pMsg;
}

void CDlgPlyrList::OnPlyrTakeOver() 
{

	if ( ( theApp.IsShareware () ) || ( theApp.IsSecondDisk () ) )
		return;

	UpdateData (TRUE);
	if ( m_btnTakeOver )
		{
		theGame.GetNetJoin () == CGame::any;
		if (theGame.GetAi().GetCount () > 0)
			theNet.SetSessionVisibility (TRUE);
		}
	else
		{
		theGame.GetNetJoin () == CGame::create;
		theNet.SetSessionVisibility (FALSE);
		}
}
