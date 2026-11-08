// vdmplayds.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "vdmplayds.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVdmplaydsApp

BEGIN_MESSAGE_MAP(CVdmplaydsApp, CWinApp)
	//{{AFX_MSG_MAP(CVdmplaydsApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVdmplaydsApp construction

CVdmplaydsApp::CVdmplaydsApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CVdmplaydsApp object

CVdmplaydsApp theApp;
