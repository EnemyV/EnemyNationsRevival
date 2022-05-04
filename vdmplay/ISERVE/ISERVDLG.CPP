// iservdlg.cpp : implementation file
//

#include "stdafx.h"
#include "iserve.h"
#include "iservdlg.h"
#include "..\base.h"

#include <winsock.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
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

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
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
// CAboutDlg message handlers

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CenterWindow();
	
	// TODO: Add extra about dlg initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
// CIserveDlg dialog

CIserveDlg::CIserveDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIserveDlg::IDD, pParent), m_protocol(VPT_TCP), m_sesInfoSize(32), m_plrInfoSize(32), m_vpH(NULL)
{
	//{{AFX_DATA_INIT(CIserveDlg)
	m_strAddr = _T("");
	m_strName = _T("");
	m_strSocket = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	CWinApp* app = AfxGetApp();
	m_hIcon = app->LoadIcon(IDR_MAINFRAME);

	if (!stricmp(app->m_lpCmdLine, "/IPX"))
		m_protocol = VPT_IPX;

}

void CIserveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIserveDlg)
	DDX_Control(pDX, IDC_ServerList, m_ServerList);
	DDX_Text(pDX, IDC_EDIT_ADDR, m_strAddr);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_SOCKET, m_strSocket);
	//}}AFX_DATA_MAP
}

#define	IDM_START 		(WM_USER + 1)

BEGIN_MESSAGE_MAP(CIserveDlg, CDialog)
	//{{AFX_MSG_MAP(CIserveDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(IDM_START, OnStartMsg)
	ON_COMMAND(IDC_START, OnStart)
	ON_COMMAND(IDC_STOP, OnStop)
	ON_COMMAND(IDC_EXIT, OnExit)
	ON_COMMAND(IDC_OPTIONS, OnOptions)
	ON_COMMAND(IDC_HELP, OnHelp)
	ON_COMMAND(IDC_ABOUT, OnAbout)
	ON_MESSAGE(WM_VPNOTIFY, OnVpNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIserveDlg message handlers

LONG CIserveDlg::OnStartMsg( UINT, LONG )
{

	OnStart ();
	return (0);
	
}

BOOL CIserveDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CenterWindow();


	int iSocket = -1;
	
	switch(m_protocol)
	{
	 	case VPT_TCP:
	 		iSocket = GetPrivateProfileInt ("TCP", "WellKnownPort", DEF_TCP_PORT, "vdmplay.ini");
			break;
		case VPT_IPX:
	 		iSocket = GetPrivateProfileInt ("IPX", "WellKnownPort", DEF_IPX_PORT, "vdmplay.ini");
			break;
	}
			
	if (iSocket != -1)
	{
		itoa (iSocket, m_strSocket.GetBuffer (40), 10);
		m_strSocket.ReleaseBuffer (-1);
		UpdateData (FALSE);
	}

	PostMessage (IDM_START);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CIserveDlg::OnPaint() 
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
HCURSOR CIserveDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CIserveDlg::FixStationName()
{
	if (m_protocol == VPT_TCP)
	{
		CString addr = m_strAddr.SpanExcluding(":");
		DWORD tcpAddr = inet_addr(addr);

		hostent* he = gethostbyaddr((LPSTR) &tcpAddr, sizeof(tcpAddr), PF_INET);
	
		if (he)
			m_strName = he->h_name;
	}
}



	
void CIserveDlg::OnStart() 
{
	VPGUID myGuid;

	memset(&myGuid, 0, sizeof(myGuid));

	GetPrivateProfileString("ISERVE", "GUID", "TESTGAME", myGuid.buf, sizeof(myGuid.buf), ".\\vdmplay.ini");
	
    m_sesInfoSize = GetPrivateProfileInt("ISERVE", "SessionInfoSize", 32, ".\\vdmplay.ini");
    m_plrInfoSize = GetPrivateProfileInt("ISERVE", "PlayerInfoSize", 32, ".\\vdmplay.ini");

	m_vpH = vpStartup(1, &myGuid, m_sesInfoSize, m_plrInfoSize, m_protocol, NULL);

   if (!m_vpH)
   {
	    if (m_protocol == VPT_TCP)
			AfxMessageBox("Requires TCP/IP to run");
		else
   		   AfxMessageBox("Can't initialize VDMPLAY");
		EndDialog(IDOK);
   }

   vpStartRegistrationServer(m_vpH, m_hWnd, NULL);

	m_strAddr = GetStationAddress();
	FixStationName();

	UpdateData (FALSE);

	GetMenu()->CheckMenuItem (IDC_START, MF_BYCOMMAND | MF_CHECKED);
	GetMenu()->CheckMenuItem (IDC_STOP, MF_BYCOMMAND | MF_UNCHECKED);

   UpdateCaption ();

	// TODO: Add your control notification handler code here
	
}

void CIserveDlg::OnStop() 
{

	m_strAddr = "";
	m_strName = "";
	m_ServerList.ResetContent ();
	UpdateData (FALSE);

	vpCleanup (m_vpH);
	m_vpH = NULL;

	GetMenu()->CheckMenuItem (IDC_START, MF_BYCOMMAND | MF_UNCHECKED);
	GetMenu()->CheckMenuItem (IDC_STOP, MF_BYCOMMAND | MF_CHECKED);

   UpdateCaption ();
}

void CIserveDlg::OnExit() 
{

	if (m_vpH)
		vpCleanup(m_vpH);
	EndDialog(IDOK);
}
	
void CIserveDlg::OnOptions ()
{

	CDlgSocket dlg (this, m_protocol);

	dlg.DoModal ();
}
	
void CIserveDlg::OnHelp ()
{

	::WinHelp (m_hWnd, "vdmplay.hlp", HELP_CONTEXT, 0);
}
	
void CIserveDlg::OnAbout ()
{

	CAboutDlg dlg;

	dlg.DoModal ();
}
	
CString CIserveDlg::GetAddressString(LPCVPNETADDRESS a)
{

	char buf[256];
	buf[0] = 0;

	vpGetAddressString(m_vpH, a, buf, sizeof(buf));

	CString tmp = buf;
	return tmp;
}


CString CIserveDlg::GetStationAddress()
{
	VPNETADDRESS addr;

	vpGetAddress(m_vpH, &addr);

	return GetAddressString(&addr);
}


LONG CIserveDlg::OnVpNotify(WPARAM w, LPARAM l)
{
	
	switch(VPGETNOTIFICATION(w))
	{
		case VP_SESSIONENUM:
			OnSessionEnumNotify((LPCVPMESSAGE) l);
			break;

		case VP_PLAYERENUM:

		case VP_JOIN:

		case VP_LEAVE:

		case VP_READDATA:
	
		case VP_SENDDATA:

		case VP_SESSIONCLOSE:

		case VP_SERVERDOWN:
			OnServerDownNotify((LPCVPMESSAGE) l);
			break;

	}

	vpAcknowledge(m_vpH, (LPCVPMESSAGE) l);
	// ! Don't touch l after acknwwledge
	 return TRUE;
}


CString CIserveDlg::GetSessionData(LPCVPSESSIONINFO info)
{
	CString sData;
	char sessionAddress[256];

	sessionAddress[0] = 0;
	vpGetAddressString(m_vpH, &info->sessionId, sessionAddress, sizeof(sessionAddress));

	sData = info->sessionName;
	sData += "@";
	sData += sessionAddress;

	return sData;
}



void CIserveDlg::OnSessionEnumNotify(LPCVPMESSAGE m)
{
	
	CString sessionData = GetSessionData(m->u.sessionInfo);

	m_ServerList.AddString(sessionData);

	UpdateCaption ();

}

void CIserveDlg::OnServerDownNotify(LPCVPMESSAGE m)
{
	
	CString sessionData = GetSessionData(m->u.sessionInfo);

	UINT index = m_ServerList.FindString(0, sessionData);

  	if (index == LB_ERR)
		return;

	m_ServerList.DeleteString(index);

	UpdateCaption ();
}

void CIserveDlg::UpdateCaption ()
{
	if (m_ServerList.GetCount())
	{
		SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);
	}
	else
	{
		SetIcon(AfxGetApp()->LoadIcon(IDI_DISCON), TRUE);
	}

	CString sTitle;
	sTitle.LoadString (m_vpH ? IDS_TITLE : IDS_TITLE_WHEN_OFF);

	char sBuf[80];
	sprintf (sBuf, sTitle, m_ServerList.GetCount ());
	SetWindowText(sBuf);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgSocket dialog


CDlgSocket::CDlgSocket(CWnd* pParent, int protocol /*=NULL*/)
	: CDialog(CDlgSocket::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgSocket)
	m_iSocket = 0;
	m_protocol = protocol;
	//}}AFX_DATA_INIT
}


void CDlgSocket::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgSocket)
	DDX_Text(pDX, IDC_EDIT_SOCKET, m_iSocket);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgSocket, CDialog)
	//{{AFX_MSG_MAP(CDlgSocket)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgSocket message handlers

void CDlgSocket::OnOK() 
{
	
	UpdateData (TRUE);
	char sBuf[20];
	itoa (m_iSocket, sBuf, 10);

	const char* protoStr;

	if (m_protocol == VPT_IPX)
		protoStr = "IPX";
	else
		protoStr = "TCP";
		
	WritePrivateProfileString (protoStr, "WellKnownPort", sBuf, ".\\vdmplay.ini");

	AfxMessageBox (IDS_NEW_SOCKET);

	CDialog::OnOK();
}

BOOL CDlgSocket::OnInitDialog() 
{

	CDialog::OnInitDialog();
	const char* protoStr;

	if (m_protocol == VPT_IPX)
		protoStr = "IPX";
	else
		protoStr = "TCP";
	
	m_iSocket = GetPrivateProfileInt (protoStr, "WellKnownPort", DEF_TCP_PORT, ".\\vdmplay.ini");
	UpdateData (FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
