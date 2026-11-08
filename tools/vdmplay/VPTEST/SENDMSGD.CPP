// sendmsgd.cpp : implementation file
//

#include "stdafx.h"
#include "vptest.h"
#include "sendmsgd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSendMsgDlg dialog


CSendMsgDlg::CSendMsgDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSendMsgDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSendMsgDlg)
	m_messageText = _T("");
	m_playerId = _T("");
	//}}AFX_DATA_INIT
}


void CSendMsgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSendMsgDlg)
	DDX_Text(pDX, IDC_MESSAGETEXT, m_messageText);
	DDX_Text(pDX, IDC_PLAYERID, m_playerId);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSendMsgDlg, CDialog)
	//{{AFX_MSG_MAP(CSendMsgDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSendMsgDlg message handlers
