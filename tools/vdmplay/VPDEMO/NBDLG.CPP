// nbdlg.cpp : implementation file
//

#include "stdafx.h"
#include "vpdemo.h"
#include "nbdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNbDlg dialog


CNbDlg::CNbDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNbDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNbDlg)
	m_lana = _T("");
	m_stationName = _T("");
	//}}AFX_DATA_INIT
}


void CNbDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNbDlg)
	DDX_Text(pDX, IDC_LANA, m_lana);
	DDX_Text(pDX, IDC_STATION_NAME, m_stationName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNbDlg, CDialog)
	//{{AFX_MSG_MAP(CNbDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNbDlg message handlers
