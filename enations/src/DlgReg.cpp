// DlgReg.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "DlgReg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgReg dialog


CDlgReg::CDlgReg(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgReg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgReg)
	m_bDone = FALSE;
	//}}AFX_DATA_INIT
}


void CDlgReg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgReg)
	DDX_Check(pDX, IDC_REG_DONE, m_bDone);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgReg, CDialog)
	//{{AFX_MSG_MAP(CDlgReg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgReg message handlers

CString GetDefaultApp ( char const *pExt, char const *pDef, char const *pCmdLine )
{

	// in case of error
	CString sRtn = pDef + CString ( " " ) + pCmdLine;

	// get the extension
	char cmd [258];
	HKEY key;
	if (RegOpenKeyEx (HKEY_CLASSES_ROOT, pExt, NULL, KEY_READ, &key) != ERROR_SUCCESS)
		return sRtn;

	// read it in
	unsigned long iLen = 256;
	DWORD dwTyp;
	if (RegQueryValueEx (key, "", NULL, &dwTyp, (BYTE*) cmd, &iLen) != ERROR_SUCCESS)
		return sRtn;

	RegCloseKey (key);
	if ( dwTyp != REG_SZ )
		return sRtn;

	// now find the app for this key value
	CString sKey = cmd + CString ( "\\shell\\open\\command" );

	if (RegOpenKeyEx (HKEY_CLASSES_ROOT, sKey, NULL, KEY_READ, &key) != ERROR_SUCCESS)
		return sRtn;

	iLen = 256;
	if (RegQueryValueEx (key, "", NULL, &dwTyp, (BYTE*) cmd, &iLen) != ERROR_SUCCESS)
		return sRtn;

	RegCloseKey (key);
	if ( dwTyp != REG_SZ )
		return sRtn;

	// put command line in it (may have %1)
	sRtn = cmd;
	int iInd = sRtn.Find ( '%' );
	if ( ( iInd < 0 ) || ( sRtn [iInd+1] != '1' ) )
		return sRtn + " " + pCmdLine;
		
	csPrintf ( &sRtn, pCmdLine );
	return sRtn;
}

void CDlgReg::OnOK()
{

	UpdateData (TRUE);
	if ( m_bDone )
		theApp.WriteProfileInt ( "Warnings", "Register", 1 );

	// get the browser
	CString sCmd = GetDefaultApp ( ".html", "netscape", "http://www.windward.net/register/index.html" );

	STARTUPINFO si;
	memset ( &si, 0, sizeof (si) );
	si.cb = sizeof (si);
	si.wShowWindow = SW_SHOWMAXIMIZED;
	si.dwFlags = STARTF_USESHOWWINDOW;
	PROCESS_INFORMATION pi;

	if ( CreateProcess ( NULL, (char *) (char const *) sCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ) == 0 )
		AfxMessageBox ( IDS_BAD_REG, MB_OK | MB_ICONSTOP | MB_TASKMODAL );
	else
		{
		theApp.m_wndMain.UpdateWindow ();
		WaitForSingleObject ( pi.hProcess, 10 * 1000 );
		CloseHandle ( pi.hProcess );
		CloseHandle ( pi.hThread );
		AfxMessageBox ( IDS_GOOD_REG, MB_OK | MB_ICONSTOP | MB_TASKMODAL );
		}

	CDialog::OnOK ();
}

void CDlgReg::OnCancel()
{

	UpdateData (TRUE);
	if ( m_bDone )
		theApp.WriteProfileInt ( "Warnings", "Register", 1 );

	CDialog::OnCancel ();
}
