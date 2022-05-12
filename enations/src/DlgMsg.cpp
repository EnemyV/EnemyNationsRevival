// DlgMsg.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "DlgMsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgModelessMsg dialog


CDlgModelessMsg::CDlgModelessMsg(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgModelessMsg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgModelessMsg)
	m_sMsg = _T("");
	//}}AFX_DATA_INIT
}


void CDlgModelessMsg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgModelessMsg)
	DDX_Text(pDX, IDS_MSG, m_sMsg);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgModelessMsg, CDialog)
	//{{AFX_MSG_MAP(CDlgModelessMsg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgModelessMsg message handlers

void CDlgModelessMsg::Create ( const char * pMsg )
{

	m_sMsg = pMsg;
	CDialog::Create ( IDD_MODELESS_MSG, theApp.m_wndMain.m_hWnd ? &(theApp.m_wndMain) : NULL );
}

void CDlgModelessMsg::PostNcDestroy() 
{
	
	CDialog::PostNcDestroy();

	delete this;
}
