// StatusDlg.cpp : implementation file
//

#include "stdafx.h"
#include "sprite.h"
#include "StatusDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatusDlg dialog


CStatusDlg::CStatusDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStatusDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CStatusDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	Create( IDD_DLG_ERR, pParent );
}


void CStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStatusDlg)
	DDX_Control(pDX, IDC_LIST_ERR, m_lstErr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStatusDlg, CDialog)
	//{{AFX_MSG_MAP(CStatusDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatusDlg message handlers

void CStatusDlg::OnOK() 
{
	
	theApp.DestroyErrorDialog ();
}
