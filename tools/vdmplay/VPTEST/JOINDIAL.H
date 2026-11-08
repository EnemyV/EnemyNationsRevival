// joindial.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJoinDialog dialog

class CJoinDialog : public CDialog
{
// Construction
public:
	CJoinDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CJoinDialog)
	enum { IDD = IDD_JOINDIALOG };
	CString	m_playerName;
	CString	m_sessionName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJoinDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CJoinDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
