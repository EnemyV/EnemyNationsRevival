// vpdemo.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "vpdemo.h"
#include "vpdemdlg.h"
#include "nbdlg.h"
#include "tsimcfg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVpdemoApp

BEGIN_MESSAGE_MAP(CVpdemoApp, CWinApp)
	//{{AFX_MSG_MAP(CVpdemoApp)
	ON_COMMAND(IDM_PROTOCOL_TCP, OnProtocolTcp)
	ON_COMMAND(IDM_PROTOCOL_NETBIOS, OnProtocolNetbios)
	ON_COMMAND(IDM_PROTOCOL_IPX, OnProtocolIpx)
	ON_COMMAND(ID_PROTOCOL_TAPI, OnProtocolTapi)
	ON_COMMAND(ID_PROTOCOL_MODEM, OnProtocolModem)
	ON_COMMAND(ID_PROTOCOL_DP, OnProtocolDp)
	ON_COMMAND(ID_PROTOCOL_COMM, OnProtocolComm)
	ON_COMMAND(ID_PROTOCOL_VPSTARTUP, OnProtocolVpstartup)
	ON_COMMAND(ID_PROTOCOL_VPENUMSES, OnProtocolVpenumses)
	ON_COMMAND(ID_PROTOCOL_ADVANCED_CL, OnProtocolAdvancedCl)
	ON_COMMAND(ID_PROTOCOL_ADVANCED_SRV, OnProtocolAdvancedSrv)
	ON_COMMAND(ID_CFGSIM, OnCfgsim)
	ON_COMMAND(IDM_GETSERVERADDR, OnGetserveraddr)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVpdemoApp construction

CVpdemoApp::CVpdemoApp()  : m_transportData(NULL)
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CVpdemoApp object

CVpdemoApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CVpdemoApp initialization

BOOL CVpdemoApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

//	Enable3dControls();
	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	CVpdemoDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}


void CVpdemoApp::SelectProtocol(UINT itemId, UINT transport)
{
   static UINT items[] = { IDM_PROTOCOL_TCP, 
						IDM_PROTOCOL_IPX   ,
						IDM_PROTOCOL_NETBIOS , 
						ID_PROTOCOL_COMM ,  
						ID_PROTOCOL_MODEM ,  
						ID_PROTOCOL_TAPI,  
						ID_PROTOCOL_DP,   
						0 };
						
						
	CMenu*  mnu = m_pMainWnd->GetMenu();
	
	for( const UINT * p = items; *p; p++)
	{
		mnu->CheckMenuItem(*p, *p == itemId ? MF_CHECKED : MF_UNCHECKED);
	}
	
	m_pMainWnd->DrawMenuBar();

	if (m_transport != transport)
	{
		delete m_transportData;
		m_transportData = NULL;
	}

	m_transport = transport;

}


void CVpdemoApp::OnProtocolTcp() 
{
	// TODO: Add your command handler code here

	
	SelectProtocol(IDM_PROTOCOL_TCP, VPT_TCP);

	
}

void CVpdemoApp::OnProtocolNetbios() 
{
	// TODO: Add your command handler code here

	SelectProtocol(IDM_PROTOCOL_NETBIOS, VPT_NETBIOS);
}

void CVpdemoApp::OnProtocolIpx() 
{
	// TODO: Add your command handler code here

	SelectProtocol(IDM_PROTOCOL_IPX, VPT_IPX);
}

void CVpdemoApp::OnProtocolTapi() 
{
	// TODO: Add your command handler code here

	SelectProtocol(ID_PROTOCOL_TAPI, VPT_TAPI);
	
}

void CVpdemoApp::OnProtocolModem() 
{
	// TODO: Add your command handler code here

	SelectProtocol(ID_PROTOCOL_MODEM, VPT_MODEM);
}

void CVpdemoApp::OnProtocolDp() 
{
	// TODO: Add your command handler code here
	
	SelectProtocol(ID_PROTOCOL_DP, VPT_DP);

	
}

void CVpdemoApp::OnProtocolComm() 
{

	// TODO: Add your command handler code here

	
	SelectProtocol(ID_PROTOCOL_COMM, VPT_COMM);

	
}


void CVpdemoApp::OnVpStartup() 
{

	// TODO: Add your command handler code here
	
}

void CVpdemoApp::OnProtocolVpstartup() 
{
	 CVpdemoDlg* dlg = 	(CVpdemoDlg*) m_pMainWnd;

	 dlg->InitVpStuff();
	// TODO: Add your command handler code here
	
}

void CVpdemoApp::OnProtocolVpenumses() 
{
	CVpdemoDlg* dlg = 	(CVpdemoDlg*) m_pMainWnd;

	if (dlg->EnumSessions())
	{
		CMenu*  mnu = m_pMainWnd->GetMenu();
	
		mnu->EnableMenuItem(ID_PROTOCOL_VPENUMSES, MF_DISABLED|MF_BYCOMMAND);
	}

	// TODO: Add your command handler code here
	
}

void CVpdemoApp::OnProtocolAdvancedCl() 
{
	// TODO: Add your command handler code here
	vpAdvDialog(NULL, m_transport, FALSE);
	
}

void CVpdemoApp::OnProtocolAdvancedSrv() 
{
	// TODO: Add your command handler code here
	vpAdvDialog(NULL, m_transport, TRUE);

}

void CVpdemoApp::OnCfgsim() 
{
	TSimCfg  dlg(m_pMainWnd);

	dlg.DoModal();
	// TODO: Add your command handler code here
	
}

void CVpdemoApp::OnGetserveraddr() 
{
	// TODO: Add your command handler code here
	CVpdemoDlg* dlg = 	(CVpdemoDlg*) m_pMainWnd;

	dlg->GetServerAddress();

	
}
