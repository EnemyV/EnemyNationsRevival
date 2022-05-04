// ProgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "sprite.h"
#include "ProgDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CSpriteApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog


CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProgressDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProgressDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	Create( IDD_PROGRESSDLG );
	CenterWindow ();
}

CProgressDlg::CProgressDlg( CWnd *pParent, int stepCnt, int stepSize )
	: CDialog( CProgressDlg::IDD, pParent )
{
	//{{AFX_DATA_INIT(CProgressDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	Create( IDD_PROGRESSDLG );

	m_progressCtl.SetRange( 0, stepCnt );
	m_progressCtl.SetStep( stepSize );
	m_progressCtl.SetPos( 0 );
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgressDlg)
	DDX_Control(pDX, IDC_PROGRESS, m_progressCtl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
	//{{AFX_MSG_MAP(CProgressDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg message handlers

void CProgressDlg::OnCancel() 
{
	::PostMessage( theApp.m_pMainWnd->GetSafeHwnd(), WM_USER_STOP, 0, 0L );

	CDialog::OnCancel();
}
