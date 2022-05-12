// TcpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "vpdiag.h"
#include "TcpDlg.h"
#include "net.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTcpDlg dialog


CTcpDlg::CTcpDlg(CWnd* pParent /*=NULL*/, BOOL ipx)
	: CDialog(CTcpDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTcpDlg)
	m_broadcast = FALSE;
	m_localAddress = _T("");
	m_port = ipx ? DEFAULT_IPX_PORT : DEFAULT_TCP_PORT;
	m_remoteAddress = _T("");
	m_server = FALSE;
	m_doEcho = FALSE;
	//}}AFX_DATA_INIT

	m_sock = NULL;
	m_ipx = ipx;
	m_autoMode = FALSE;
}

CTcpDlg::~CTcpDlg()
{
	delete m_sock;
}

void CTcpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTcpDlg)
	DDX_Control(pDX, IDC_DOECHO, m_doEchoCtl);
	DDX_Control(pDX, IDOK, m_startBtn);
	DDX_Control(pDX, IDC_REMOTEADDRESS, m_remoteAddressCtl);
	DDX_Control(pDX, IDC_MESSAGES, m_messages);
	DDX_Check(pDX, IDC_BROADCAST, m_broadcast);
	DDX_Text(pDX, IDC_LOCALADDRESS, m_localAddress);
	DDX_Text(pDX, IDC_PORT, m_port);
	DDX_Text(pDX, IDC_REMOTEADDRESS, m_remoteAddress);
	DDX_Check(pDX, IDC_SERVER, m_server);
	DDX_Check(pDX, IDC_DOECHO, m_doEcho);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTcpDlg, CDialog)
	//{{AFX_MSG_MAP(CTcpDlg)
	ON_BN_CLICKED(IDC_SERVER, OnServer)
	ON_BN_CLICKED(IDC_BROADCAST, OnBroadcast)
	ON_MESSAGE(WM_WINSOCK, OnWinsock)
	ON_BN_CLICKED(IDC_DOECHO, OnDoecho)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTcpDlg message handlers

void CTcpDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	UpdateData(TRUE);

	if (m_server)
	{
		m_startBtn.EnableWindow(FALSE);
		if (m_sock->WaitData(m_port, m_broadcast))
			UpdateData(FALSE);
	}
	else
	{
		m_sock->Ping(m_remoteAddress, m_port, m_broadcast);
	}


	return;


	
//	CDialog::OnOK();
}

void CTcpDlg::OnServer() 
{
	if (m_autoMode)
	{
		UpdateData(FALSE);
		return;
	}

	UpdateData(TRUE);

	m_remoteAddressCtl.EnableWindow(!m_server && !m_broadcast);
	if (!m_server)
	{
		m_doEcho = FALSE;
		UpdateData(FALSE);
	}
	m_doEchoCtl.EnableWindow(m_server);

	// TODO: Add your control notification handler code here
	
}

void CTcpDlg::OnBroadcast() 
{
	UpdateData(TRUE);

	m_remoteAddressCtl.EnableWindow(!m_server && !m_broadcast);
	// TODO: Add your control notification handler code here
	
}


LONG CTcpDlg::OnWinsock(WPARAM wParam, LPARAM lParam)
{

	if (m_sock)
	{
		CString sender, data;
		
		UINT port;

		if (m_sock->ReadData(data, sender, port, lParam))
		{
			char msg[512];

			wsprintf(msg, "From port %u at %s got: %s", port, (LPCSTR) sender, (LPCSTR) data);
			ShowMessage(msg);
			if (m_server && m_doEcho)
				m_sock->Reply(sender, port, data, data.GetLength()+1);
		}
	}

	return TRUE;
}

BOOL CTcpDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if (m_ipx)
	{
		SetWindowText("IPX Diagnostics");
		m_localAddress = Net_GetIpxAddress();
		m_sock = Net_MakeIpxSocket(m_hWnd, WM_WINSOCK);
	}
	else
	{
		m_localAddress = Net_GetTcpAddress();
		m_sock = Net_MakeTcpSocket(m_hWnd, WM_WINSOCK);
	}

	if (m_autoMode)
	{
		m_remoteAddressCtl.EnableWindow(!m_server);
		m_doEchoCtl.EnableWindow(m_server);
	}

	UpdateData(FALSE);
	if (!m_sock)
		m_startBtn.EnableWindow(FALSE);

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTcpDlg::ShowMessage(LPCSTR msg)
{
	int i = m_messages.AddString(msg);
	m_messages.SetTopIndex(i);
	m_messages.UpdateWindow();
}

void CTcpDlg::OnDoecho() 
{
	UpdateData(TRUE);
	// TODO: Add your control notification handler code here
	
}
