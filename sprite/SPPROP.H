#ifndef SPPROP_H
#define SPPROP_H

// SPProp.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSPProp dialog

class CSPProp : public CPropertyPage
{
	DECLARE_DYNCREATE(CSPProp)

// Construction
public:
	CSPProp();
	~CSPProp();

// Dialog Data
	//{{AFX_DATA(CSPProp)
	enum { IDD = IDD_SPPROP_PAGE };
	CEdit	m_yoffEdit;
	CEdit	m_xoffEdit;
	CEdit	m_widthEdit;
	CEdit	m_holdEdit;
	CEdit	m_heightEdit;
	//}}AFX_DATA

	int		m_width, m_height;
	int		m_xoff, m_yoff;
	int		m_holdTime;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSPProp)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSPProp)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

#endif
