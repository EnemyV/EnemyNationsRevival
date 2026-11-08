// vdmplui.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxdllx.h>
#include "resource.h"
#include "vdmplui.h"
#include "vdmplay.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static AFX_EXTENSION_MODULE vdmpluiDLL = { NULL, NULL };

CString FetchString(LPCSTR section, LPCSTR key, LPCSTR defVal)
{
	char buf[256];

	GetPrivateProfileString(
			section, 
			key, 
			defVal,
			buf, sizeof(buf), ".\\vdmplay.ini");

	return buf;
}

UINT FetchInt(LPCSTR section, LPCSTR key, UINT defVal)
{

	return GetPrivateProfileInt(
			section, 
			key, 
			defVal,
		 ".\\vdmplay.ini");


}

void StoreInt(LPCSTR section, LPCSTR key, UINT val)
{
	char buf[256];

	wsprintf(buf, "%u", val);

	WritePrivateProfileString(
			section, 
			key, 
			buf,
			".\\vdmplay.ini");
}

void StoreString(LPCSTR section, LPCSTR key, LPCSTR val)
{

	WritePrivateProfileString(
			section, 
			key, 
			val,
			".\\vdmplay.ini");
}


extern "C" BOOL __declspec(dllexport)  vpAdvancedSetup(int protocol)
{
	CDialog* pDlg = NULL;
	BOOL ret = FALSE;

	switch(protocol)
	{
		case VPT_COMM:
		case VPT_MODEM:
			pDlg = new CMdmDlg(protocol == VPT_MODEM);
			break;
		case VPT_NETBIOS:
			pDlg = new CNBDlg;
			break;
		case VPT_TCP:
			pDlg = new CTcpDlg;
			break;
		case VPT_IPX:
			pDlg = new CIpxDlg;
			break;

		default:
			return FALSE;
	}

	if (pDlg)
	{
		ret = pDlg->DoModal() == IDOK;
		delete pDlg;
	}

	return ret;
}


extern "C" int  APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("VDMPLUI.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		AfxInitExtensionModule(vdmpluiDLL, hInstance);

		// Insert this DLL into the resource chain
		new CDynLinkLibrary(vdmpluiDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("VDMPLUI.DLL Terminating!\n");
	}
	return 1;   // ok
}
/////////////////////////////////////////////////////////////////////////////
// CNBDlg dialog


CNBDlg::CNBDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNBDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNBDlg)
	m_lana = 0;
	m_stationName = _T("");
	//}}AFX_DATA_INIT

	m_lana = FetchInt("NETBIOS", "LANA", 255);
	m_stationName = FetchString("NETBIOS", "StationName", "");


}


void CNBDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNBDlg)
	DDX_Text(pDX, IDC_LANA, m_lana);
	DDV_MinMaxByte(pDX, m_lana, 0, 255);
	DDX_Text(pDX, IDC_STATIONNAME, m_stationName);
	DDV_MaxChars(pDX, m_stationName, 16);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNBDlg, CDialog)
	//{{AFX_MSG_MAP(CNBDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNBDlg message handlers
/////////////////////////////////////////////////////////////////////////////
// CIpxDlg dialog


CIpxDlg::CIpxDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIpxDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CIpxDlg)
	m_portNumber = 0;
	m_serverNet = _T("");
	m_serverNode = _T("");
	//}}AFX_DATA_INIT

	m_portNumber = FetchInt("IPX", "WellKnownPort", 0);
	m_serverNet = FetchString("IPX", "ServerNet", "");
	m_serverNode = FetchString("IPX", "ServerNode", "");

}



void CIpxDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIpxDlg)
	DDX_Text(pDX, IDC_PORTNUMBER, m_portNumber);
	DDX_Text(pDX, IDC_SERVERNET, m_serverNet);
	DDV_MaxChars(pDX, m_serverNet, 4);
	DDX_Text(pDX, IDC_SERVERNODE, m_serverNode);
	DDV_MaxChars(pDX, m_serverNode, 12);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIpxDlg, CDialog)
	//{{AFX_MSG_MAP(CIpxDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CIpxDlg message handlers
/////////////////////////////////////////////////////////////////////////////
// CTcpDlg dialog


CTcpDlg::CTcpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTcpDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTcpDlg)
	m_portNumber = 0;
	m_serverAddress = _T("");
	m_regSrv = _T("");
	//}}AFX_DATA_INIT

	m_portNumber = FetchInt("TCP", "WellKnownPort", 0);
	m_serverAddress = FetchString("TCP", "ServerAddress", "");
	m_regSrv = 	FetchString("TCP", "RegistrationAddress", "");
}


void CTcpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTcpDlg)
	DDX_Text(pDX, IDC_PORTNUMBER, m_portNumber);
	DDX_Text(pDX, IDC_SERVERADDR, m_serverAddress);
	DDX_Text(pDX, IDC_REGSERVER, m_regSrv);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTcpDlg, CDialog)
	//{{AFX_MSG_MAP(CTcpDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTcpDlg message handlers
/////////////////////////////////////////////////////////////////////////////
// CMdmDlg dialog


CMdmDlg::CMdmDlg(BOOL forModem, CWnd* pParent /*=NULL*/)
	: CDialog(CMdmDlg::IDD, pParent) , m_forModem(forModem)
{
	//{{AFX_DATA_INIT(CMdmDlg)
	m_callSetup = _T("");
	m_dialNumber = _T("");
	m_listenSetup = _T("");
	m_portNameStr = _T("");
	//}}AFX_DATA_INIT

	LPCSTR section = forModem ? "MODEM" : "COMM";

	m_portNameStr = FetchString(section, "Port", "COM1");

	if (forModem)
	{
		m_callSetup = FetchString(section, "INIT_DIAL", "");
		m_listenSetup = FetchString(section, "INIT_LISTEN", "");
		m_dialNumber = 	FetchString(section, "DIAL_NUMBER", "");
	}


}


void CMdmDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMdmDlg)
	DDX_Control(pDX, IDC_PORTNAME, m_portName);
	DDX_Text(pDX, IDC_CALLSETUP, m_callSetup);
	DDX_Text(pDX, IDC_DIALNUMBER, m_dialNumber);
	DDX_Text(pDX, IDC_LISTENSETUP, m_listenSetup);
	DDX_CBString(pDX, IDC_PORTNAME, m_portNameStr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMdmDlg, CDialog)
	//{{AFX_MSG_MAP(CMdmDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMdmDlg message handlers

BOOL CMdmDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMdmDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	
	UpdateData();
	StoreString(m_forModem ? "MODEM" : "COMM", "Port", m_portNameStr);

   	if (m_forModem)
	{
		StoreString("MODEM", "INIT_DIAL", m_callSetup);
		StoreString("MODEM", "INIT_LISTEN", m_listenSetup);
		StoreString("MODEM", "DIAL_NUMBER", m_dialNumber);
	}

	

	CDialog::OnOK();
}

void CNBDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData();
	StoreString("NETBIOS", "StationName", m_stationName);
	
	if (m_lana == 255)
	{
		StoreString("NETBIOS", "LANA", "");
	}
	else
	{
		StoreInt("NETBIOS", "LANA", m_lana);
	}

	
	CDialog::OnOK();
}

void CIpxDlg::OnOK() 
{
	// TODO: Add extra validation here

	UpdateData();
	StoreString("IPX", "ServerNet", m_serverNet);
	StoreString("IPX", "ServerNode", m_serverNode);
	StoreInt("IPX", "WellKnownPort", m_portNumber);
	CDialog::OnOK();
}

void CTcpDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData();
	StoreString("TCP", "ServerAddress", m_serverAddress);
	StoreInt("TCP", "WellKnownPort", m_portNumber);
	StoreString("TCP", "RegistrationAddress", m_regSrv);
		
	CDialog::OnOK();
}
