// joindial.cpp : implementation file
//

#include "stdafx.h"
#include "vptest.h"
#include "joindial.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJoinDialog dialog


CJoinDialog::CJoinDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CJoinDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CJoinDialog)
	m_playerName = _T("");
	m_sessionName = _T("");
	//}}AFX_DATA_INIT
}


void CJoinDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJoinDialog)
	DDX_Text(pDX, IDC_PLAYERNAME, m_playerName);
	DDX_Text(pDX, IDC_SESSIONNAME, m_sessionName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CJoinDialog, CDialog)
	//{{AFX_MSG_MAP(CJoinDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CJoinDialog message handlers
