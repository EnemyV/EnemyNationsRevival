// vpdiag.cpp : Defines the class behaviors for the application.
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
// CVpdiagApp

BEGIN_MESSAGE_MAP(CVpdiagApp, CWinApp)
	//{{AFX_MSG_MAP(CVpdiagApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVpdiagApp construction

CVpdiagApp::CVpdiagApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CVpdiagApp object

CVpdiagApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CVpdiagApp initialization

BOOL CVpdiagApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CDialog* pDlg;
	BOOL serverMode = FALSE;

	int nResponse;

	if (m_lpCmdLine[0] == 0)
	{
		pDlg = new CVpdiagDlg;
	}
	else
	{
	  CString cmdLine = m_lpCmdLine;

	  BOOL tcp = FALSE;
	  BOOL ipx = FALSE;
	  BOOL netbios = FALSE;
	  if (!Net_Init(tcp, ipx, netbios))
		return FALSE;

	  cmdLine.MakeUpper();

	  if (-1 != cmdLine.Find("/SERVER"))
		  serverMode = TRUE;
	  if (ipx && (-1 != cmdLine.Find("/IPX")))
	  {
		  CTcpDlg* xDlg = new CTcpDlg(NULL, TRUE);
		  xDlg->m_autoMode = TRUE;
		  xDlg->m_server = serverMode;
		  pDlg = xDlg;
	  }
	  else if (tcp && (-1 != cmdLine.Find("/TCP")))
	  {
		  CTcpDlg* xDlg = new CTcpDlg(NULL, FALSE);
		  xDlg->m_autoMode = TRUE;
		  xDlg->m_server = serverMode;
		  pDlg = xDlg;
	  }
	  else if (netbios && (-1 != cmdLine.Find("/NETBIOS")))
	  {
		  CNbDlg* xDlg = new CNbDlg;
		  xDlg->m_autoMode = TRUE;
		  xDlg->m_server = serverMode;
		  pDlg = xDlg;
	  }
	}

	if (pDlg)
	{
		m_pMainWnd = pDlg;
		nResponse = pDlg->DoModal();
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
		m_pMainWnd = NULL;
		delete pDlg;
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
