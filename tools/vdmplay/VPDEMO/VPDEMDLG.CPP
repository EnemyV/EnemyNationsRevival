// vpdemdlg.cpp : implementation file
//

#include "stdafx.h"
#include "vpdemo.h"
#include "vpdemdlg.h"
#include "dxfer.h"
#include "vpxfer.h"
#include "loadtest.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BOOL gLocalIni;


void vpMakeIniFile(LPSTR fName)
{
	*fName = 0;

	if (gLocalIni)
		lstrcpy(fName, ".\\");

	strcat(fName, VDMPLAY_INIFILE);
}

// --- Configuration params I/O -------
void vpFetchString(LPCSTR section, 
				   LPCSTR key, 
				   LPCSTR defVal, 
				   LPSTR pStr, 
				   int iLen)
{
	char fName[256];

	vpMakeIniFile(fName);

	*pStr = 0;
	GetPrivateProfileString(
			section, 
			key, 
			defVal,
			pStr, iLen,
			fName);
}

UINT vpFetchInt(LPCSTR section, LPCSTR key, UINT defVal)
{

	char fName[256];

	vpMakeIniFile(fName);

	return GetPrivateProfileInt(
			section, 
			key, 
			defVal,
		fName);
}

UINT vpFetchIntR(LPCSTR section, LPCSTR key, UINT defVal, UINT minVal, UINT maxVal)
{

	UINT val = vpFetchInt(section, key, defVal);
	if (minVal > val || val > maxVal)
		val = defVal;
	return val;
}


void vpStoreInt(LPCSTR section, LPCSTR key, UINT val)
{
	char fName[256];

	vpMakeIniFile(fName);

	char buf[30];

	itoa (val, buf, 10);

	WritePrivateProfileString(
			section, 
			key, 
			buf,
			fName);
}




void vpStoreString(LPCSTR section, LPCSTR key, LPCSTR val)
{
	char fName[256];

	vpMakeIniFile(fName);

	WritePrivateProfileString(
			section, 
			key, 
			val,
			fName);
}

void vpStoreIntD(LPCSTR section, LPCSTR key, UINT val, UINT defVal)
{
	if (val != defVal)
		vpStoreInt(section, key, val);
	else
		vpStoreString(section, key, NULL);
}



#define BIGSIZE (8*1024)

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
// CVpdemoDlg dialog

CVpdemoDlg::CVpdemoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVpdemoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVpdemoDlg)
	m_selectedSession = _T("");
	m_selectedPlayer = _T("");
	m_playerName = _T("");
	m_mustDeliver = FALSE;
	m_broadcast = FALSE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_vpH = NULL;
	m_vpSession = NULL;
	m_xfer = NULL;
	m_myId = 0;
	m_loadTest = NULL;
}

void CVpdemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVpdemoDlg)
	DDX_Control(pDX, IDC_STATICS, m_textFieldTitle);
	DDX_Control(pDX, IDC_EVENTLOG, m_eventLog);
	DDX_Control(pDX, IDC_PLAYERNAME, m_playerNameField);
	DDX_Control(pDX, IDC_TEXT, m_textField);
	DDX_Control(pDX, IDC_SESSIONLIST, m_sessionBox);
	DDX_Control(pDX, IDC_PLAYERLIST, m_playersBox);
	DDX_Control(pDX, IDC_SENDMSG, m_sendMsg);
	DDX_Control(pDX, IDC_JOIN, m_join);
	DDX_Control(pDX, IDC_CREATESESSION, m_createSession);
	DDX_LBString(pDX, IDC_SESSIONLIST, m_selectedSession);
	DDX_LBString(pDX, IDC_PLAYERLIST, m_selectedPlayer);
	DDX_Text(pDX, IDC_PLAYERNAME, m_playerName);
	DDX_Check(pDX, IDC_MUSTDELIVER, m_mustDeliver);
	DDX_Check(pDX, IDC_BROADCAST, m_broadcast);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVpdemoDlg, CDialog)
	//{{AFX_MSG_MAP(CVpdemoDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SENDMSG, OnSendMsg)
	ON_BN_CLICKED(IDC_CREATESESSION, OnCreateSession)
	ON_BN_CLICKED(IDC_JOIN, OnJoin)
	ON_MESSAGE(WM_VPNOTIFY, OnVpNotify)
	ON_MESSAGE(WM_VPFLOWOFF, OnVpFlowOFF)
	ON_MESSAGE(WM_VPFLOWON, OnVpFlowON)
	ON_BN_CLICKED(IDC_RESCAN, OnRescan)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_SENDBIG, OnSendbig)
	ON_BN_CLICKED(IDC_LOADTEST, OnLoadTest)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVpdemoDlg message handlers

BOOL CVpdemoDlg::OnInitDialog()
{

	gLocalIni = vpFetchInt("VDMPLAY", "LocalIni",  1);

	CDialog::OnInitDialog();
	CenterWindow();

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


	CVpdemoApp* app = (CVpdemoApp*) AfxGetApp();
	app->SelectProtocol(IDM_PROTOCOL_TCP, VPT_TCP);

	// InitVpStuff();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVpdemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CVpdemoDlg::OnPaint() 
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
HCURSOR CVpdemoDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}



struct MYSesInfo
{
	VPSESSIONINFO sesInfo;
	char moreData[10];
};


const VPGUID tlpGUID = { "TheLastPlanet" };

const int iMaxNameLen = VP_MAXPLAYERDATA;




CString CVpdemoDlg::GetAddressString(LPCVPNETADDRESS a)
{

	char buf[256];
	buf[0] = 0;

	vpGetAddressString(m_vpH, a, buf, sizeof(buf));

	CString tmp = buf;
	return tmp;
}





CString CVpdemoDlg::GetStationAddress()
{
	VPNETADDRESS addr;

	vpGetAddress(m_vpH, &addr);

	return GetAddressString(&addr);
}

void CVpdemoDlg::GetServerAddress()
{
	VPNETADDRESS addr;

	if (m_vpH && vpGetServerAddress(m_vpH, &addr))
	{
		CString tmp = GetAddressString(&addr);

		Log(tmp);
	}
	else
	{
		Log("NO Server Address!!!");
	}
}


int CVpdemoDlg::GetTransportKind()
{
#if 0
	char fName[256];
	lstrcpy(fName, ".\\");
	lstrcat(fName, VDMPLAY_INIFILE);

	char buf[16];

	GetPrivateProfileString("VPDEMO", "Transport", "TCP", buf, sizeof(buf), fName);
	
	if (!strcmpi(buf, "TCP"))
		return VPT_TCP;

	if (!strcmpi(buf, "IPX"))
		return VPT_IPX;

	if (!strcmpi(buf,"NETBIOS"))
		return VPT_NETBIOS;

	return VPT_TCP;
#else
	CVpdemoApp* app = (CVpdemoApp*) AfxGetApp();
	return app->m_transport;
#endif
}
		

void CVpdemoDlg::InitVpStuff()
{

	int tpCode = GetTransportKind();
	DWORD tpMask = vpSupportedTransports();


	if (!(tpMask &  (1 << tpCode)))
	{
		const char* tpText;

		switch(tpCode)
		{
			case VPT_TCP:
				tpText = "TCP Transport is not supported";
				break;
			case VPT_IPX:
				tpText = "IPX Transport is not supported";
				break;
			case VPT_NETBIOS:
				tpText = "NETBIOS Transport is not supported";
				break;
			case VPT_COMM:
				tpText = "NETBIOS Transport is not supported";
				break;
			case VPT_MODEM:
				tpText = "MODEM Transport is not supported";
				break;
		  	default:
				tpText = "UNKNOWN Transport is not supported";
				break;
		}

		AfxMessageBox(tpText);
		return;
	}			

	CVpdemoApp* app = (CVpdemoApp*) AfxGetApp();

	DWORD t1, t2;

	t1 = GetCurrentTime();

	m_vpH = vpStartup(1, &tlpGUID, iMaxNameLen, iMaxNameLen, GetTransportKind(), app->m_transportData);

	t2 = GetCurrentTime() - t1;

	if (!m_vpH)
	{
		AfxMessageBox("vpStartup FAILED");
		return;
	}

   	m_sendMsg.EnableWindow(FALSE);		

   	CString caption = "vpdemo at ";
	caption += GetStationAddress();
	
	SetWindowText(caption);
	m_textFieldTitle.SetWindowText("Session Name");
	 

}


BOOL CVpdemoDlg::EnumSessions()
{

	
	vpEnumSessions(m_vpH, m_hWnd, TRUE, NULL);

	

	m_vpstate = ENUMERATING;

	return TRUE;
 }



VPPLAYERID CVpdemoDlg::GetPlayerId(const CString& pName)
{
	PlayerData* pd;

	if (m_players.Lookup(pName, pd))
	{
		return pd->id;
	}
	return 0;
}


	
void CVpdemoDlg::OnSendMsg() 
{
	
	if (!m_vpSession)
		return;
	
	UpdateData();

	VPPLAYERID senderId = GetPlayerId(m_playerName);
	VPPLAYERID toId = 0;

	if (senderId == 0)
		senderId = VP_SESSIONSERVER;

	DWORD flags = 0;
	
	if (m_broadcast)
	{
		flags |= VP_BROADCAST;
		toId = VP_ALLPLAYERS;
	}
	else
	{
		toId  = GetPlayerId(m_selectedPlayer);
	}

	if (m_mustDeliver)
		flags |= VP_MUSTDELIVER;

#pragma pack(1)
	struct 
	{
		VPMSGHDR  hdr;
		char text[256];
	} msg;
#pragma pack()

   	int l;
   	l = m_textField.GetLine(0, msg.text, (int) sizeof(msg.text));
	msg.text[l] = 0;


    if (!vpSendData(m_vpSession, toId, senderId, &msg, 1+strlen(msg.text)+sizeof(msg.hdr), flags, 0))
	{
		m_eventLog.AddString("**vpSendData failed");
	}


	
	// TODO: Add your control notification handler code here
	
}

void CVpdemoDlg::OnCreateSession() 
{
	// TODO: Add your control notification handler code here

	vpStopEnumSessions(m_vpH);
	char sessionName[64];

   	int l = m_textField.GetLine(0, sessionName, (int) sizeof(sessionName));
	sessionName[l] = 0;


	m_vpSession = vpCreateSession(m_vpH, m_hWnd, sessionName, 0, NULL);



	if (!m_vpSession)
	{
		AfxMessageBox("vpCreateSession Failed");
	 	return;
	}

	m_createSession.EnableWindow(FALSE);
	m_sendMsg.EnableWindow(TRUE);		
	m_textFieldTitle.SetWindowText("Message Text");

	CString caption = "Server ";

	caption += sessionName;
	caption += " at ";
	caption += GetStationAddress();



	SetWindowText(caption);
	m_join.SetWindowText("AddPlayer");
	m_vpstate = SERVER;
		
}


void CVpdemoDlg::DoJoin()
{
		
	  
	vpStopEnumSessions(m_vpH);

	UpdateData();

	


	SessionData* sd;
	if (m_sessions.Lookup(m_selectedSession, sd))
	{
		char tmp[iMaxNameLen];
		memset(tmp, 0, iMaxNameLen);
		strcpy(tmp, m_playerName);
		m_vpSession = vpJoinSession(m_vpH, m_hWnd, &sd->id, tmp, 0, (LPVOID) 1);
		if (!m_vpSession)
		{
			AfxMessageBox("vpJoinSession Failed");
			ClearSessionList();
			vpEnumSessions(m_vpH, m_hWnd, TRUE, NULL);
			return;
		}

		CString caption = "Client ";
		caption += m_playerName;
		caption += " at ";
		caption += GetStationAddress();
		caption += " connecting to ";
		caption += m_selectedSession;

		SetWindowText(caption);

		m_join.SetWindowText("Leave");
		m_createSession.EnableWindow(FALSE);
		m_vpstate = CLIENT;
		m_sendMsg.EnableWindow(TRUE);		
		m_textFieldTitle.SetWindowText("Message Text");
		return;
	}

  	vpEnumSessions(m_vpH, m_hWnd, TRUE, NULL);

		 	
}


void CVpdemoDlg::DoAddPlayer()
{

	UpdateData();

	VPPLAYERID id;

	PlayerData* pd = new PlayerData;

	if (!pd)
		return;

    if (!vpAddPlayer(m_vpSession, m_playerName, 0, (LPVOID) 1, &id))
	{
		AfxMessageBox("vpAddLocalPlayer failed");
		delete pd;
		return;
	}


	pd->index = m_playersBox.AddString(m_playerName);
	pd->id = id;
	m_players.SetAt(m_playerName, pd);
	m_myId = id;

		
}



void CVpdemoDlg::OnJoin() 
{

	
	switch (m_vpstate)
	{ 
		case ENUMERATING:
			DoJoin();
			break;
		case CLIENT:
			DoDisconnect();
			break;
		case SERVER:
			DoAddPlayer();
	}
}




void CVpdemoDlg::OnSessionEnumNotify(LPCVPMESSAGE m)
{
	
	if (!m)
		return;

	
	SessionData* sd = new SessionData;
	
	if (!sd)
		return;
	
	CString sName = m->u.sessionInfo->sessionName;

	sd->index = m_sessionBox.AddString(sName);
	sd->id = m->u.sessionInfo->sessionId;

	m_sessions.SetAt(sName, sd);

}

void CVpdemoDlg::OnServerDownNotify(LPCVPMESSAGE m)
{
	
	if (!m)
		return;

	
	
	CString sName = m->u.sessionInfo->sessionName;

	UINT index = m_sessionBox.FindString(0, sName);

  	if (index == LB_ERR)
		return;

	m_sessionBox.DeleteString(index);

    SessionData* sd = m_sessions[sName];
	if (sd)
		delete sd;

	m_sessions.SetAt(sName, NULL);

}

void CVpdemoDlg::Log(LPCSTR msg)
{
	if (m_eventLog.GetCount() > 100)
		m_eventLog.ResetContent();

	int i = m_eventLog.AddString(msg);
	m_eventLog.SetTopIndex(i);
}
 
void CVpdemoDlg::OnPlayerEnumNotify(LPCVPMESSAGE ) { }

void CVpdemoDlg::OnJoinNotify(LPCVPMESSAGE m) 
{ 

	if (m_vpstate == CLIENT && (m->userData == (LPVOID) 1))
	{
		m_myId = m->u.playerInfo->playerId;

		CString caption = "Client ";

		caption += m_playerName;
		caption += " at ";
		caption += GetStationAddress();
		caption += " connected to ";
		caption += m_selectedSession;


		SetWindowText(caption);
	}

	CString pName = m->u.playerInfo->playerName;
	PlayerData* pd = new PlayerData;
	pd->index = m_playersBox.AddString(pName);
	pd->id = m->u.playerInfo->playerId;
	m_players.SetAt(pName, pd);

	


}


void CVpdemoDlg::ClearPlayerList()
{
	
	m_playersBox.ResetContent();

	POSITION pos = m_players.GetStartPosition();
	PlayerData* pd;
	CString key;

	while(pos != NULL)
	{
		m_players.GetNextAssoc(pos, key, pd);
		delete pd;
	}

	m_players.RemoveAll();
}




void CVpdemoDlg::ClearSessionList()
{
	
	m_sessionBox.ResetContent();

	POSITION pos = m_sessions.GetStartPosition();
	SessionData* sd;
	CString key;

	while(pos != NULL)
	{
		m_sessions.GetNextAssoc(pos, key, sd);
		delete sd;
	}

	m_sessions.RemoveAll();
}


void CVpdemoDlg::DoDisconnect()
{

   ClearSessionList();
   ClearPlayerList();
   
   if (m_vpSession)
   {
    	vpCloseSession(m_vpSession, NULL);
  		m_vpSession = NULL;
		vpCleanup(m_vpH);
		InitVpStuff();

   }

   if (m_vpH)
   		vpEnumSessions(m_vpH, m_hWnd, TRUE, NULL);
   m_join.EnableWindow(FALSE);
   m_createSession.EnableWindow(TRUE);
   m_sendMsg.EnableWindow(FALSE);		
	m_textFieldTitle.SetWindowText("Session Name");

   m_join.SetWindowText("Join Session");
   m_vpstate = ENUMERATING;	
   SetWindowText("vpdemo");


}

BOOL CVpdemoDlg::GetPlayerName(VPPLAYERID id, CString& pName)
{
	if (id == VP_SESSIONSERVER)
	{
		pName = "**Server**";
		return TRUE;
	}


	POSITION pos = m_players.GetStartPosition();
	PlayerData* pd;
	CString key;

	while(pos != NULL)
	{
		m_players.GetNextAssoc(pos, key, pd);
		if (pd->id == id)
		{
			pName = key;
			return TRUE;
		}
	}

	return FALSE;
}


void CVpdemoDlg::OnLeaveNotify(LPCVPMESSAGE m) 
{ 

	POSITION pos = m_players.GetStartPosition();
	PlayerData* pd;
	CString key;

	while(pos != NULL)
	{
		m_players.GetNextAssoc(pos, key, pd);
		if (pd->id == m->u.playerInfo->playerId)
		{
			m_playersBox.DeleteString(pd->index);
			m_players.RemoveKey(key);
			delete pd;
			return;
		}

	}

}




void CVpdemoDlg::OnReadDataNotify(LPCVPMESSAGE m)
{ 
	CString srcName, dstName;

	if (!GetPlayerName(m->senderId, srcName))
	{
		AfxMessageBox("OnReadData: Bad fromId");
		return;
	}

	if (m->toId == VP_ALLPLAYERS)
	{
		dstName = "Everybody"; 
	} 
	else if (!GetPlayerName(m->toId, dstName))
	{
		AfxMessageBox("OnReadData: Bad toId");
		return;
	}


	if (!strncmp((LPCSTR) m->u.data, "**TEST", 6))
	{
		if (m_loadTest)
		{
			m_eventLog.AddString("Load test request ignored");
			return;
		}

		m_loadTest = new SlaveLoadTest(m_vpSession, m_myId);
	}

	CString msg = "Message from ";
	
	msg += srcName;
	msg += " to "; 
	msg += dstName;
	msg += " :";
	msg += (LPCSTR) m->u.data;


	m_eventLog.AddString(msg);

	if (!strncmp((LPCSTR) m->u.data, "**SEND:", 7))
	{
		if (m_xfer)
		{
			m_eventLog.AddString("Transfer request ignored");
			return;
		}

		DWORD sz = atol(7 + (LPCSTR) m->u.data);
		m_bigBuf = new short[sz/sizeof(short)];

		SetTimer(1, 1000, NULL);

		m_xfer = new CVPTransfer(m_vpSession);

		if (!m_xfer->ReceiveDataFrom(m->senderId, m->toId, m_bigBuf, sz))
		{
			m_eventLog.AddString("ReceiveDataFrom failed");
			delete m_xfer;
			m_xfer = NULL;
			delete [] m_bigBuf;
			m_bigBuf = NULL;
		}
	}
}

void CVpdemoDlg::OnSendDataNotify(LPCVPMESSAGE ) { }



void CVpdemoDlg::OnCloseSessionNotify(LPCVPMESSAGE m) 
{ 

   m_vpSession = NULL;  // !!!! THIS is important
   DoDisconnect();	
   	
   SetWindowText("Connection Lost");
  

}

void CVpdemoDlg::OnNetDownNotify(LPCVPMESSAGE m) 
{ 

   m_vpSession = NULL;  // !!!! THIS is important
   DoDisconnect();	
   	
   SetWindowText("Network layer down");
  

}


LONG CVpdemoDlg::OnVpFlowOFF(WPARAM w, LPARAM l)
{
	if (m_loadTest)
		m_loadTest->OnFlowOff();
	else
		m_eventLog.AddString("Got Flow OFF");
	return TRUE;
}

LONG CVpdemoDlg::OnVpFlowON(WPARAM w, LPARAM l)
{
	if (m_loadTest)
		m_loadTest->OnFlowOn();
	else
		m_eventLog.AddString("Got Flow ON");
	return TRUE;
}




LONG CVpdemoDlg::OnVpNotify(WPARAM w, LPARAM l)
{
	UINT n = VPGETNOTIFICATION(w);
	if (m_xfer)
	{
		if (m_xfer->ProcessNotification(n, (LPVPMESSAGE) l))
		{
			vpAcknowledge(m_vpH, (LPCVPMESSAGE) l);
			if (m_xfer->Done())
			{
				KillTimer(1);
				if (m_xfer->GetError() == 0)
					m_eventLog.AddString("Big Transfer OK");
				else
					m_eventLog.AddString("Big Transfer FAILED");
				
				
				delete m_xfer;
				m_xfer = NULL;
				
				int i = 0;
				for(i = 0; i < BIGSIZE; i++)
				{
					if (m_bigBuf[i] != i)
						break;
				}

				m_eventLog.AddString(i < BIGSIZE ? "Bad BigBuf" : "BigBuf OK");

				delete [] m_bigBuf;
				m_bigBuf = NULL;


			}
			return TRUE;
		}
	}

	if (m_loadTest)
	{
		if (m_loadTest->ProcessNotification(n, (LPVPMESSAGE) l))
		{
			vpAcknowledge(m_vpH, (LPCVPMESSAGE) l);
			if (m_loadTest->Done())
			{
				delete m_loadTest;
				m_loadTest = NULL;
			}
			return TRUE;
		}
	}


	switch(VPGETNOTIFICATION(w))
	{
		case VP_SESSIONENUM:
			OnSessionEnumNotify((LPCVPMESSAGE) l);
			break;

		case VP_PLAYERENUM:
			OnPlayerEnumNotify((LPCVPMESSAGE) l);
			break;

		case VP_JOIN:
			OnJoinNotify((LPCVPMESSAGE) l);
			break;

		case VP_LEAVE:
			OnLeaveNotify((LPCVPMESSAGE) l);
			break;

		case VP_READDATA:
			OnReadDataNotify((LPCVPMESSAGE) l);
			break;
	
		case VP_SENDDATA:
			OnSendDataNotify((LPCVPMESSAGE) l);
			break;

		case VP_SESSIONCLOSE:
			OnCloseSessionNotify((LPCVPMESSAGE) l);
			break;

		case VP_SERVERDOWN:
			OnServerDownNotify((LPCVPMESSAGE) l);
			break;

		case VP_NETDOWN:
			OnNetDownNotify((LPCVPMESSAGE) l);

	}

	vpAcknowledge(m_vpH, (LPCVPMESSAGE) l);
	// ! Don't touch l after acknwwledge
	 return TRUE;
}

void CVpdemoDlg::OnOK() 
{
	// TODO: Add extra validation here
	if (m_vpH)
	{
		vpCleanup(m_vpH);
		m_vpH = NULL;
	}
	
	CDialog::OnOK();
}

void CVpdemoDlg::OnRescan() 
{
	if (m_vpstate == ENUMERATING)
	{
		vpStopEnumSessions(m_vpH);
		ClearSessionList();
		vpEnumSessions(m_vpH, m_hWnd, TRUE, NULL);
	
		CString caption = "vpdemo at ";
		caption += GetStationAddress();
		SetWindowText(caption);


	}
	
}

void CVpdemoDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	if (m_xfer)
	{
		m_xfer->OnTimer();
		if (m_xfer->Done())
		{
			KillTimer(1);
			if (m_xfer->GetError() == 0)
				m_eventLog.AddString("Big Transfer OK");
			else
				m_eventLog.AddString("Big Transfer FAILED");
				
				
			delete m_xfer;
			m_xfer = NULL;

			delete m_bigBuf;
			m_bigBuf = NULL;
		}
	}
	
	if (m_loadTest)
	{
		m_loadTest->OnTimer();
		if (m_loadTest->Done())
		{
			m_eventLog.AddString("Load test DONE");
			delete m_loadTest;
			m_loadTest = NULL;
		}
	}


	CDialog::OnTimer(nIDEvent);
}


void CVpdemoDlg::OnSendbig() 
{

	if (!m_xfer)
	{
		UpdateData();


		VPPLAYERID senderId = GetPlayerId(m_playerName);
		VPPLAYERID toId = 0;

		if (senderId == 0)
			senderId = VP_SESSIONSERVER;

		toId  = GetPlayerId(m_selectedPlayer);
	
		m_bigBuf = new short[BIGSIZE];
		for(int i = 0; i < BIGSIZE; i++)
			m_bigBuf[i] = i;


		struct 
		{
			VPMSGHDR  hdr;
			char text[256];
		} msg;

       	wsprintf(msg.text, "**SEND:%lu", BIGSIZE*sizeof(short));

	    if (!vpSendData(m_vpSession, toId, senderId, &msg, 1+strlen(msg.text)+sizeof(msg.hdr), VP_MUSTDELIVER, 0))
		{
			m_eventLog.AddString("Failure to send the transfer request");;
		}


		SetTimer(1, 1000, NULL);

		m_xfer = new CVPTransfer(m_vpSession);

		if (!m_xfer->SendDataTo(toId, senderId, m_bigBuf, BIGSIZE*sizeof(short)))
		{
			m_eventLog.AddString("SendDataTo failed");
			delete m_xfer;
			m_xfer = NULL;
			delete [] m_bigBuf;
			m_bigBuf = NULL;
		}
		

	}
		

	// TODO: Add your control notification handler code here
	
}

void CVpdemoDlg::OnLoadTest() 
{
	if (m_myId == 0)
	{
		AfxMessageBox("Not joined");
		return;
	}

	struct 
	{
		VPMSGHDR  hdr;
		char text[256];
	} msg;

    wsprintf(msg.text, "**TEST");

    if (!vpSendData(m_vpSession, VP_ALLPLAYERS, m_myId, 
		&msg, 1+strlen(msg.text)+sizeof(msg.hdr), 
		VP_MUSTDELIVER, 0))
	{
		Log("Failure to send simulation request");;
	}


	MasterLoadTest* mload =  new MasterLoadTest(m_vpSession, m_myId,
		m_playersBox.GetCount() -1);
	m_loadTest = mload;

	Log("Starting simulation");

	DWORD tmFreq = mload->m_tickFreq; 
	SetTimer(1, tmFreq, NULL);

	// TODO: Add your control notification handler code here
	
}
