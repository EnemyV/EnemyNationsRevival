// NbDlg.cpp : implementation file
//

#include "stdafx.h"
#include "vpdiag.h"
#include "NbDlg.h"
#include "net.h"
#include "../vputil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CNbDlg dialog


CNbDlg::CNbDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNbDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNbDlg)
	m_server = FALSE;
	m_selectedLana = _T("");
	//}}AFX_DATA_INIT
	m_autoMode = FALSE;
}


void CNbDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNbDlg)
	DDX_Control(pDX, IDC_SERVER, m_serverChkBox);
	DDX_Control(pDX, IDC_LANAS, m_lanas);
	DDX_Control(pDX, IDOK, m_startBtn);
	DDX_Control(pDX, IDC_LIST2, m_messages);
	DDX_Check(pDX, IDC_SERVER, m_server);
	DDX_LBString(pDX, IDC_LANAS, m_selectedLana);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNbDlg, CDialog)
	//{{AFX_MSG_MAP(CNbDlg)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_SAVE, OnSave)
	ON_BN_CLICKED(IDC_CLOSE, OnClose)
	ON_MESSAGE(WM_NBNOTIFY, OnNbNotify)
	ON_BN_CLICKED(IDC_SERVER, OnServer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CNbDlg::ShowMessage(LPCSTR msg)
{
	int i = m_messages.AddString(msg);
	m_messages.SetTopIndex(i);
	m_messages.UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CNbDlg message handlers

void CNbDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	UpdateData();
	Net_SetWindow(m_hWnd);
	m_startBtn.EnableWindow(FALSE);

	if (m_server)
	{
		CString  str;

		Net_EnumLanas(LoadLana, this);

		str.LoadString(IDS_GOTOCLIENT);
		ShowMessage(str);
	}
	else
	{
		Net_EnumLanas(LookForServer, this);
	}
}

void CNbDlg::LoadLana(UINT lana, LPVOID context)
{
	CNbDlg* dlg = (CNbDlg*) context;
	CString fmt;
	char buf[128];

	fmt.LoadString(IDS_INITLANA);
	wsprintf(buf, fmt, lana);
	dlg->ShowMessage(buf);

	Net_InitServerLana(lana);
}



void CNbDlg::LookForServer(UINT lana, LPVOID context)
{
	CNbDlg* dlg = (CNbDlg*) context;
	CString fmt;
	char buf[128];

	fmt.LoadString(IDS_SEARCHINGLANA);
	wsprintf(buf, fmt, lana);
	dlg->ShowMessage(buf);

	Net_LookForServerOnLana(lana, FoundLana, context);
}

void CNbDlg::FoundLana(UINT clientLana, UINT serverLana, LPVOID context)
{
	CNbDlg* dlg = (CNbDlg*) context;
	CString fmt;
	char buf[128];

	fmt.LoadString(IDS_FOUNDLANA);
 	wsprintf(buf, fmt, clientLana, serverLana);
	dlg->ShowMessage(buf);


}

LONG CNbDlg::OnNbNotify(WPARAM m, LPARAM l)
{
	char buf[10];

	wsprintf(buf, "%u", l);
	m_lanas.AddString(buf);
	return TRUE;
}


void CNbDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	Net_ResetLanas();
	// TODO: Add your message handler code here
	
}

void CNbDlg::OnSave() 
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);

	if (m_selectedLana != "")
	{
		vpStoreString("NETBIOS", "LANA", m_selectedLana);
	}
	

	EndDialog(IDOK);	
}

void CNbDlg::OnClose() 
{
	EndDialog(IDCANCEL);
	// TODO: Add your control notification handler code here
	
}

BOOL CNbDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if (m_autoMode)
	{
//		m_serverChkBox.EnableWindow(FALSE);
		UpdateData(FALSE);
	}
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNbDlg::OnServer() 
{
	if (m_autoMode)
		m_serverChkBox.SetCheck(m_server);
	// TODO: Add your control notification handler code here
	
}
