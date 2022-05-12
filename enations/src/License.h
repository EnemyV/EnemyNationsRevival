// License.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgLicense dialog

class CDlgLicense : public CDialog
{
// Construction
public:
	CDlgLicense(int iText, BOOL bOK = FALSE , CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgLicense)
	enum { IDD = IDD_LICENSE };
	CWndOD< CButton >	m_btnYes;
	CWndOD< CButton >	m_btnOk;
	CWndOD< CButton >	m_btnCancel;
	CString	m_strLicense;
	//}}AFX_DATA

	int		m_iText;
	BOOL	m_bOK;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgLicense)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgLicense)
	virtual BOOL OnInitDialog();
	afx_msg void OnYes();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
