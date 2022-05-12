// tcpipxdl.cpp : implementation file
//

#include "stdafx.h"
#include "vpdemo.h"
#include "tcpipxdl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTcpIpxDlg dialog


CTcpIpxDlg::CTcpIpxDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTcpIpxDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTcpIpxDlg)
	m_port = _T("");
	m_serverAdress = _T("");
	//}}AFX_DATA_INIT
}


void CTcpIpxDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTcpIpxDlg)
	DDX_Text(pDX, IDC_PORT, m_port);
	DDX_Text(pDX, IDC_SERVER_ADDRESS, m_serverAdress);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTcpIpxDlg, CDialog)
	//{{AFX_MSG_MAP(CTcpIpxDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTcpIpxDlg message handlers
