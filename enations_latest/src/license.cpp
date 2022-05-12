// License.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "License.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgLicense dialog


CDlgLicense::CDlgLicense(int iText, BOOL bOK, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgLicense::IDD, pParent)
{

	m_iText = iText;
	m_bOK = bOK;

	//{{AFX_DATA_INIT(CDlgLicense)
	m_strLicense = _T("");
	//}}AFX_DATA_INIT
}


void CDlgLicense::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgLicense)
	DDX_Control(pDX, IDYES, m_btnYes);
	DDX_Control(pDX, IDOK, m_btnOk);
	DDX_Control(pDX, IDCANCEL, m_btnCancel);
	DDX_Text(pDX, IDC_LICENSE, m_strLicense);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgLicense, CDialog)
	//{{AFX_MSG_MAP(CDlgLicense)
	ON_BN_CLICKED(IDYES, OnYes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgLicense message handlers

BOOL CDlgLicense::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if ( m_bOK )
		{
		m_btnYes.EnableWindow (FALSE);
		m_btnYes.ShowWindow (SW_HIDE);
		m_btnCancel.EnableWindow (FALSE);
		m_btnCancel.ShowWindow (SW_HIDE);
		}
	else
		{
		m_btnOk.EnableWindow (FALSE);
		m_btnOk.ShowWindow (SW_HIDE);
		}

	// set the string
	CMmio *pMmio = theDataFile.OpenAsMMIO (NULL, "LANG");
	pMmio->DescendRiff ('L', 'A', 'N', 'G');
	pMmio->DescendList ('L', 'E', 'G', 'L');
	pMmio->DescendChunk ('L', 'I', 'C', '0' + m_iText);

	long lSize = pMmio->ReadLong ();
	pMmio->Read (m_strLicense.GetBuffer (lSize+2), lSize);

	delete pMmio;
	m_strLicense.ReleaseBuffer (lSize);

	UpdateData (FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgLicense::OnYes() 
{

	EndDialog (IDOK);	
}
