// DlgMsg.cpp : implementation file
//

#include "stdafx.h"
#include "_windwrd.h"
#include "DlgMsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgMsg dialog


CDlgMsg::CDlgMsg(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMsg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgMsg)
	m_btnCheck = FALSE;
	m_sText = _T("");
	//}}AFX_DATA_INIT
}


void CDlgMsg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgMsg)
	DDX_Check(pDX, IDC_MSG_CHECK, m_btnCheck);
	DDX_Text(pDX, IDC_MSG_TEXT, m_sText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgMsg, CDialog)
	//{{AFX_MSG_MAP(CDlgMsg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgMsg message handlers

BOOL CDlgMsg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CenterWindow ();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgMsg::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

int CDlgMsg::MsgBox (UINT nIDPrompt, UINT nType, char const * psEntry, char const * psSection, int iDefault)
{

	CString sText;
	sText.LoadString (nIDPrompt);

	return ( MsgBox ( sText, nType, psEntry, psSection ) );
}

int CDlgMsg::MsgBox (char const * psPrompt, UINT nType, char const * psEntry, char const * psSection, int iDefault)
{

	// if they don't want to be warned - ok
	if ( ptheApp->GetProfileInt( psEntry, psSection, 0 ) != 0 )
		return iDefault;

	m_sText = psPrompt;
		
	int iRtn = DoModal ();
	iRtn = (iRtn == IDOK) ? IDYES : IDCANCEL;

	// do we turn this off for the future?
	if ( m_btnCheck )
		ptheApp->WriteProfileInt( psEntry, psSection, 1 );

	return ( iRtn );
}

void CDlgMsg::OnCancel() 
{

	UpdateData (TRUE);
	
	CDialog::OnCancel();
}
