// vpdiagdl.cpp : implementation file
//

#include "stdafx.h"
#include "vpdiag.h"
#include "vpdiagdl.h"
#include "nbdlg.h"
#include "tcpdlg.h"
#include "net.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVpdiagDlg dialog

CVpdiagDlg::CVpdiagDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVpdiagDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVpdiagDlg)
	m_ipxAddress = _T("");
	m_tcpHostName = _T("");
	m_tcpAddress = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVpdiagDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVpdiagDlg)
	DDX_Control(pDX, IDC_TESTIPX, m_testIpxBtn);
	DDX_Control(pDX, IDC_TESTTCP, m_testTcpBtn);
	DDX_Control(pDX, IDC_TESTNETBIOS, m_testNetbiosBtn);
	DDX_Control(pDX, IDC_LANALIST, m_lanaListCtl);
	DDX_Text(pDX, IDC_IPXADDRESS, m_ipxAddress);
	DDX_Text(pDX, IDC_TCPHOSTNAME, m_tcpHostName);
	DDX_Text(pDX, IDC_TCPADDRESS, m_tcpAddress);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVpdiagDlg, CDialog)
	//{{AFX_MSG_MAP(CVpdiagDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_TESTIPX, OnTestIpx)
	ON_BN_CLICKED(IDC_TESTNETBIOS, OnTestNetbios)
	ON_BN_CLICKED(IDC_TESTTCP, OnTestTcp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVpdiagDlg message handlers

BOOL CVpdiagDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	CString strAboutMenu;
	strAboutMenu.LoadString(IDS_ABOUTBOX);
	if (!strAboutMenu.IsEmpty())
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	

	BOOL tcp = FALSE;
	BOOL ipx = FALSE;
	BOOL netbios = FALSE;
	if (!Net_Init(tcp, ipx, netbios))
		return FALSE;


	m_testTcpBtn.EnableWindow(tcp);
	m_testIpxBtn.EnableWindow(ipx);
	m_testNetbiosBtn.EnableWindow(netbios);
	m_lanaListCtl.EnableWindow(netbios);

	if (tcp)
	{
		m_tcpHostName = Net_GetTcpHostName();
		m_tcpAddress = Net_GetTcpAddress();
	}

	if (ipx)
	{
		m_ipxAddress = Net_GetIpxAddress();
	}

	if (netbios)
	{
		Net_EnumLanas(EnumLanaCallback, this);
	}


    UpdateData(FALSE);
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVpdiagDlg::EnumLanaCallback(UINT lana, LPVOID context)
{
	CVpdiagDlg* dlg = (CVpdiagDlg*) context;
	char buf[8];

	wsprintf(buf,"%u",lana);
	dlg->m_lanaListCtl.AddString(buf);
}
	

void CVpdiagDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVpdiagDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVpdiagDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CVpdiagDlg::OnTestIpx() 
{
	CTcpDlg dlg(this, TRUE);

	dlg.DoModal();

	// TODO: Add your control notification handler code here
	
}

void CVpdiagDlg::OnTestNetbios() 
{
	CNbDlg dlg(this);

	dlg.DoModal();
	// TODO: Add your control notification handler code here
	
}

void CVpdiagDlg::OnTestTcp() 
{
	CTcpDlg dlg(this);

	dlg.DoModal();

	// TODO: Add your control notification handler code here
	
}
