// HoldTime.cpp : implementation file
//

#include "stdafx.h"
#include "sprite.h"
#include "HoldTime.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHoldTime dialog


CHoldTime::CHoldTime(CWnd* pParent /*=NULL*/)
	: CDialog(CHoldTime::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHoldTime)
	m_holdTime = 0;
	//}}AFX_DATA_INIT
}


void CHoldTime::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHoldTime)
	DDX_Text(pDX, IDC_HT_EDIT, m_holdTime);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHoldTime, CDialog)
	//{{AFX_MSG_MAP(CHoldTime)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHoldTime message handlers
