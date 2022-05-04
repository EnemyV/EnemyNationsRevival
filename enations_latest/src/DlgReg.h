// DlgReg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgReg dialog

class CDlgReg : public CDialog
{
// Construction
public:
	CDlgReg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgReg)
	enum { IDD = IDD_REGISTER };
	BOOL	m_bDone;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgReg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgReg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
