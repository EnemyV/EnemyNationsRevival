// advdlg.cpp : implementation file
//
#include "afxwin.h"
#include "afxdlgs.h"


#include "stdafx.h"
#include "vdmplay.h"
#include "resource.h"
#include "advdlg.h"
#include "base.h"
#include "help.h"

#ifndef WIN32
typedef unsigned char UCHAR, FAR* PUCHAR;
typedef DWORD ULONG;
typedef char FAR* PCHAR;
#endif

#include <nb30.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern void MakeIniFile(LPSTR);

CString FetchString (LPCSTR section, LPCSTR key, LPCSTR defVal="")
{
	char fName[256];
	char buf[256];

	MakeIniFile(fName);

	GetPrivateProfileString(
			section, 
			key, 
			defVal,
			buf, sizeof(buf),
			fName);


	CString  tmp = buf;

	return tmp;


}

UINT FetchInt(LPCSTR section, LPCSTR key, UINT defVal)
{

	char fName[256];

	MakeIniFile(fName);

	return GetPrivateProfileInt(
			section, 
			key, 
			defVal,
		fName);
}

void StoreInt(LPCSTR section, LPCSTR key, UINT val)
{
	char fName[256];

	MakeIniFile(fName);

	char buf[30];

	itoa (val, buf, 10);

	WritePrivateProfileString(
			section, 
			key, 
			buf,
			fName);
}

void StoreString(LPCSTR section, LPCSTR key, LPCSTR val)
{
	char fName[256];

	MakeIniFile(fName);

	WritePrivateProfileString(
			section, 
			key, 
			val,
			fName);
}



void FillPortCombo(CComboBox& aComboBox)
{
#ifdef WIN32
		for (int iPort=0; iPort<255; iPort++)
		{
			char sBuf[10];
			strcpy (sBuf, "COM");
			itoa (iPort, &sBuf[3], 10);
			HANDLE dev = CreateFile (sBuf, GENERIC_READ | GENERIC_WRITE, 0, NULL,
								OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
			if (dev != INVALID_HANDLE_VALUE)
			{
				CloseHandle (dev);
				aComboBox.AddString(sBuf);
			}
		}
#else
		for (int iPort=0; iPort<255; iPort++)
		{
			char sBuf[10];
			strcpy (sBuf, "COM");
			itoa (iPort, &sBuf[3], 10);
			int iHdl = OpenComm (sBuf, 1024, 1024);
			if (iHdl >= 0)
			{
				CloseComm (iHdl);
				aComboBox.AddString(sBuf);
			}
		}
#endif

}

void DoHelp(HWND hWnd, int idx)
{
	WinHelp(hWnd, "vdmplay.hlp", HELP_CONTEXT, idx);
}

/////////////////////////////////////////////////////////////////////////////
// CAdvIpxDdlg dialog


static void ParseIpxAddress(const CString& addr,  CString& netNum, CString& nodeNum, UINT& portNum)
{
	char buf[64];

	strcpy(buf, addr);

	char* dot = strchr(buf, '.');

	if (dot == NULL)
		return;

	*dot++ = 0;

	netNum = buf;

	char* colon = strchr(dot, ':');

	if (colon)
		*colon++ = 0;

	nodeNum = dot;
	

	if (!colon)
		return;

	char* comma = strchr(colon, ',');

	if (comma)
		*comma++ = 0;


	if (comma)
		portNum = atoi(comma);


}

CAdvIpxDdlg::CAdvIpxDdlg(BOOL serverMode, CWnd* pParent /*=NULL*/)
	: CDialog(CAdvIpxDdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAdvIpxDdlg)
	m_netNumber = _T("");
	m_nodeNumber = _T("");
	m_socketNumber = 0;
	//}}AFX_DATA_INIT

	m_serverMode = serverMode;

	if (m_serverMode)
	{
		CString regAddr;

		regAddr = FetchString("Ipx","RegistrationAddress");
		ParseIpxAddress(regAddr, m_netNumber, m_nodeNumber, m_socketNumber);
	}
	else
	{
		m_netNumber = FetchString("IPX", "ServerNet");
		m_nodeNumber = FetchString("IPX", "ServerNode");
		m_socketNumber = FetchInt("IPX", "WellKnownPort", DEF_IPX_PORT);
	}
	
			
			
}




void CAdvIpxDdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdvIpxDdlg)
	DDX_Text(pDX, IDC_IPX_NET_NUM, m_netNumber);
	DDX_Text(pDX, IDC_IPX_NODE_NUM, m_nodeNumber);
	DDX_Text(pDX, IDC_IPX_SOCKET, m_socketNumber);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdvIpxDdlg, CDialog)
	//{{AFX_MSG_MAP(CAdvIpxDdlg)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAdvIpxDdlg message handlers

static CString MakeIpxAddress(const CString& netNum, const CString& nodeNum, UINT portNum)
{
	CString tmp;

	if (netNum != "" && nodeNum != "")
	{
		if (!portNum)
			portNum = DEF_IPX_PORT;

		tmp = netNum;
		tmp += ".";
		tmp += nodeNum;
		tmp += ":,";

		char buf[10];

		itoa(portNum, buf, 10);
		tmp += buf;

	}

	return tmp;
}

void CAdvIpxDdlg::OnOK() 
{
	UpdateData(TRUE);

	if (m_serverMode)
	{
		CString regAddr = MakeIpxAddress(m_netNumber, m_nodeNumber, m_socketNumber);
		StoreString("Ipx", "RegistrationAddress", regAddr);

	}
	else
	{
		StoreString("Ipx", "ServerNet", m_netNumber);
	 	StoreString("Ipx", "ServerNode", m_nodeNumber);
		StoreInt("Ipx", "WellKnownPort", m_socketNumber);
	}

	// TODO: Add extra validation here
	
	CDialog::OnOK();
}
/////////////////////////////////////////////////////////////////////////////
// CAdvMdmDlg dialog


CAdvMdmDlg::CAdvMdmDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAdvMdmDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAdvMdmDlg)
	m_portStr = _T("");
	m_number = _T("");
	//}}AFX_DATA_INIT
}


void CAdvMdmDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdvMdmDlg)
	DDX_Control(pDX, IDC_MODEM_PORT, m_port);
	DDX_CBString(pDX, IDC_MODEM_PORT, m_portStr);
	DDX_Text(pDX, IDC_MODEM_NUMBER, m_number);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdvMdmDlg, CDialog)
	//{{AFX_MSG_MAP(CAdvMdmDlg)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	ON_BN_CLICKED(IDC_MODEM_ADV, OnModemAdv)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAdvMdmDlg message handlers

void CAdvMdmDlg::OnOK() 
{

	UpdateData(TRUE);
	StoreString("MODEM", "DIAL_NUMBER", m_number);
	StoreString("MODEM", "Port", m_portStr);
	 
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}


void CAdvMdmDlg::OnHelp() 
{
	DoHelp (m_hWnd, HLP_MODEM);
	// TODO: Add your control notification handler code here
	
}

void CAdvMdmDlg::OnModemAdv() 
{
	CAdvMdmSetting dlg(this);

	dlg.DoModal();

	// TODO: Add your control notification handler code here
	
}
/////////////////////////////////////////////////////////////////////////////
// CAdvMdmSetting dialog


CAdvMdmSetting::CAdvMdmSetting(CWnd* pParent /*=NULL*/)
	: CDialog(CAdvMdmSetting::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAdvMdmSetting)
	m_initStr = _T("");
	m_dialPrefix = _T("");
	m_dialSuffix = _T("");
	m_autoAnswer = _T("");
	//}}AFX_DATA_INIT

	m_initStr = FetchString ("MODEM", "CALL_INIT", "ATZ");
	m_autoAnswer = FetchString ("MODEM", "LISTEN_INIT", "ATS0=2");

	m_dialPrefix =  FetchString("MODEM", "DIAL_PREFIX", "ATDT");

	m_dialPrefix = FetchString ("MODEM", "DIAL_SUFFIX", "");



}

void CAdvMdmSetting::OnOK() 
{

	UpdateData(TRUE);
	StoreString("MODEM", "CALL_INIT", m_initStr);
	StoreString("MODEM", "LISTEN_INIT", m_autoAnswer);
 	StoreString("MODEM", "DIAL_PREFIX", m_dialPrefix);
	StoreString("MODEM", "DIAL_SUFFIX", m_dialSuffix);

	// TODO: Add extra validation here
	
	CDialog::OnOK();
}


void CAdvMdmSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdvMdmSetting)
	DDX_Text(pDX, IDC_CLIENT_INIT, m_initStr);
	DDX_Text(pDX, IDC_DIAL_PREFIX, m_dialPrefix);
	DDX_Text(pDX, IDC_DIAL_SUFFIX, m_dialSuffix);
	DDX_Text(pDX, IDC_SERVER_INIT, m_autoAnswer);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdvMdmSetting, CDialog)
	//{{AFX_MSG_MAP(CAdvMdmSetting)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAdvMdmSetting message handlers
/////////////////////////////////////////////////////////////////////////////
// CAdvNbDlg dialog


CAdvNbDlg::CAdvNbDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAdvNbDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAdvNbDlg)
	m_stationName = _T("");
	m_lanaStr = _T("");
	//}}AFX_DATA_INIT
}


void CAdvNbDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdvNbDlg)
	DDX_Control(pDX, IDC_NETBIOS_LANA, m_lanaCtl);
	DDX_Text(pDX, IDC_NETBIOS_STATION, m_stationName);
	DDX_CBString(pDX, IDC_NETBIOS_LANA, m_lanaStr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdvNbDlg, CDialog)
	//{{AFX_MSG_MAP(CAdvNbDlg)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAdvNbDlg message handlers

void CAdvNbDlg::OnOK() 
{
	UpdateData(TRUE);

	StoreString("Netbios", "Lana", m_lanaStr);
	StoreString("Netbios", "StationName", m_stationName);

	// TODO: Add extra validation here
	
	CDialog::OnOK();
}
/////////////////////////////////////////////////////////////////////////////
// CAdvTcpDlg dialog


CAdvTcpDlg::CAdvTcpDlg(BOOL serverMode, CWnd* pParent /*=NULL*/)
	: CDialog(CAdvTcpDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAdvTcpDlg)
	m_serverAddr = _T("");
	m_socket = 0;
	//}}AFX_DATA_INIT

	m_serverMode = serverMode;




}


void CAdvTcpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdvTcpDlg)
	DDX_Text(pDX, IDC_IP_SERVER_ADDR, m_serverAddr);
	DDX_Text(pDX, IDC_IP_SOCKET, m_socket);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdvTcpDlg, CDialog)
	//{{AFX_MSG_MAP(CAdvTcpDlg)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAdvTcpDlg message handlers

void CAdvTcpDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(TRUE);

	StoreInt ("TCP", "WellKnownPort", m_socket);

	LPCSTR addrKey = m_serverMode? "RegistrationAddress" : "ServerAddress";
	StoreString ("TCP", addrKey, m_serverAddr);

	
	CDialog::OnOK();
}

BOOL CAdvTcpDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_socket = FetchInt ("TCP", "WellKnownPort", DEF_TCP_PORT);
	LPCSTR addrKey = (m_serverMode) ? "RegistrationAddress" : "ServerAddress";
	m_serverAddr = FetchString ("TCP", addrKey, "");
	UpdateData(FALSE);
	// TODO: Add extra initialization here
	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



void CAdvIpxDdlg::OnHelp() 
{
	DoHelp (m_hWnd, HLP_IPX);
	// TODO: Add your control notification handler code here
	
}

void CAdvNbDlg::OnHelp() 
{
	DoHelp (m_hWnd, HLP_NETBIOS);
	// TODO: Add your control notification handler code here
	
}

void CAdvTcpDlg::OnHelp() 
{
	DoHelp (m_hWnd,  HLP_TCPIP);
	// TODO: Add your control notification handler code here
	
}

void CAdvMdmSetting::OnHelp() 
{
	DoHelp (m_hWnd, HLP_ADV_MODEM);

	// TODO: Add your control notification handler code here
	
}


/////////////////////////////////////////////////////////////////////////////
// CAdvComDlg dialog


CAdvComDlg::CAdvComDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAdvComDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAdvComDlg)
	m_portStr = _T("");
	//}}AFX_DATA_INIT
}


void CAdvComDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdvComDlg)
	DDX_Control(pDX, IDC_DIRECT_PORT, m_port);
	DDX_CBString(pDX, IDC_DIRECT_PORT, m_portStr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdvComDlg, CDialog)
	//{{AFX_MSG_MAP(CAdvComDlg)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CAdvComDlg::OnHelp() 
{
	DoHelp (m_hWnd, HLP_COMM);

	// TODO: Add your control notification handler code here
	
}


/////////////////////////////////////////////////////////////////////////////
// CAdvComDlg message handlers

void CAdvComDlg::OnOK() 
{
	UpdateData(TRUE);
 	StoreString("COMM", "Port", m_portStr);
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

BOOL CAdvMdmDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	FillPortCombo(m_port);
	m_portStr = FetchString("MODEM", "Port");
	m_number = FetchString("MODEM", "DIAL_NUMBER");
	UpdateData(FALSE);
		
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CAdvComDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	FillPortCombo(m_port);
	m_portStr = FetchString("COMM", "Port");

	UpdateData(FALSE);

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


extern HINSTANCE vphInst;

class CResourceKeeper 
{
public:
	CResourceKeeper(HINSTANCE h = vphInst)
	{
	   m_hInst = AfxGetResourceHandle();
	   AfxSetResourceHandle(h);
	}

	~CResourceKeeper()
	{
	   AfxSetResourceHandle(m_hInst);
	}

private:
	HINSTANCE m_hInst;
};

void doAdvDialog (HWND hWnd, int iProtocol, BOOL bServer)
{


	if (! (vpSupportedTransports() & (1 << iProtocol)) )
		return;
	
	CResourceKeeper keeper;

	switch (iProtocol)
	  {
		case VPT_TCP :
			{ 
				CAdvTcpDlg dlg(bServer);
				dlg.DoModal();
			}
			break;
		case VPT_IPX :
			{ 
				CAdvIpxDdlg dlg(bServer);
				dlg.DoModal();
			}
			break;
		case VPT_NETBIOS:
			{ 
				CAdvNbDlg dlg;
				dlg.DoModal();
			}

			break;
		case VPT_COMM :
			{ 
				CAdvComDlg dlg;
				dlg.DoModal();
			}

			break;
		case VPT_MODEM :
			{ 
				CAdvMdmDlg dlg;
				dlg.DoModal();
			}
			break;
	  }
}





BOOL CAdvNbDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_stationName = FetchString("Netbios", "StationName");
	m_lanaStr = FetchString("Netbios", "Lana");

	NCB ncb;
	LANA_ENUM lEnum;
	UCHAR ret;

	memset(&ncb, 0, sizeof(ncb));

	ncb.ncb_command = NCBENUM;
   	ncb.ncb_length = sizeof(lEnum);
	ncb.ncb_buffer = (PUCHAR) &lEnum;

	ret = Netbios(&ncb);

	if (NRC_GOODRET != ret)
	{
		m_lanaCtl.AddString("ENUM Call failed");
		UpdateData(FALSE);
		return TRUE;	
	}


	for(int i = 0; i < lEnum.length; i++)
	{
		char buf[16];

		wsprintf(buf, "%u", lEnum.lana[i]);
		m_lanaCtl.AddString(buf);
	}

	UpdateData(FALSE);
		
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
