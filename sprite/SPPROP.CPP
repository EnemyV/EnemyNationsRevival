// SPProp.cpp : implementation file
//

#include "stdafx.h"
#include "sprite.h"
#include "SPProp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSPProp property page

IMPLEMENT_DYNCREATE(CSPProp, CPropertyPage)

CSPProp::CSPProp() : CPropertyPage(CSPProp::IDD)
{
	//{{AFX_DATA_INIT(CSPProp)
	//}}AFX_DATA_INIT

	m_width = 0;
	m_height = 0;
	m_xoff = 0;
	m_yoff = 0;
	m_holdTime = 0;
}

CSPProp::~CSPProp()
{
}

void CSPProp::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSPProp)
	DDX_Control(pDX, IDC_SPPROP_YOFF, m_yoffEdit);
	DDX_Control(pDX, IDC_SPPROP_XOFF, m_xoffEdit);
	DDX_Control(pDX, IDC_SPPROP_WIDTH, m_widthEdit);
	DDX_Control(pDX, IDC_SPPROP_HOLD, m_holdEdit);
	DDX_Control(pDX, IDC_SPPROP_HEIGHT, m_heightEdit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSPProp, CPropertyPage)
	//{{AFX_MSG_MAP(CSPProp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSPProp message handlers

BOOL CSPProp::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	CString		str;

	str.Format( "%d", m_width );
	m_widthEdit.SetWindowText( str );

	str.Format( "%d", m_height );
	m_heightEdit.SetWindowText( str );

	str.Format( "%d", m_xoff );
	m_xoffEdit.SetWindowText( str );

	str.Format( "%d", m_yoff );
	m_yoffEdit.SetWindowText( str );

	str.Format( "%d", m_holdTime );
	m_holdEdit.SetWindowText( str );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSPProp::OnOK() 
{
	CString str;

	m_xoffEdit.GetWindowText( str );
	m_xoff = atoi( str );

	m_yoffEdit.GetWindowText( str );
	m_yoff = atoi( str );

	m_holdEdit.GetWindowText( str );
	m_holdTime = atoi( str );
	
	CPropertyPage::OnOK();
}
