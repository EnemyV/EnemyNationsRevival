// UtDemdlg.cpp : implementation file
//

#include "stdafx.h"
#include "UtDemo.h"
#include "UtDemdlg.h"

#include "vdmplay.h"

typedef DWORD    (*TvpSupportedTransports)(); 
TvpSupportedTransports 	pvpSupportedTransports; 


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUtDemoDlg dialog

CUtDemoDlg::CUtDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUtDemoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUtDemoDlg)
	m_edit = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CUtDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUtDemoDlg)
	DDX_Text(pDX, IDC_EDIT, m_edit);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CUtDemoDlg, CDialog)
	//{{AFX_MSG_MAP(CUtDemoDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUtDemoDlg message handlers

BOOL CUtDemoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CenterWindow();
	
	// TODO: Add extra initialization here

	DWORD tp = 0;

#if 0
	HMODULE hLib = LoadLibrary("VDMPLAY.DLL");

	pvpSupportedTransports = (TvpSupportedTransports) GetProcAddress(hLib, "vpSupportedTransports");
	
	if (pvpSupportedTransports)
	{
		tp = pvpSupportedTransports();
	}
	
	FreeLibrary(hLib);
#else

	DWORD ver = vpGetVersion();
#endif		 
 

	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CUtDemoDlg::OnPaint() 
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
HCURSOR CUtDemoDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CUtDemoDlg::OnOK() 
{
	// TODO: Add extra validation here

	DWORD ver = vpGetVersion();
	char buf[128];
	
	wsprintf(buf, "vpGetVersion() = %08lx", ver);
	
	m_edit = buf;

	UpdateData(FALSE);	
	

}
