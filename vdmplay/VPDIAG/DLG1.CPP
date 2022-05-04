// Dlg1.cpp : implementation file
//

#include "stdafx.h"
#include "vpdiag.h"
#include "Dlg1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dlg1 dialog


Dlg1::Dlg1(CWnd* pParent /*=NULL*/)
	: CDialog(Dlg1::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dlg1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dlg1::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dlg1)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dlg1, CDialog)
	//{{AFX_MSG_MAP(Dlg1)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dlg1 message handlers
