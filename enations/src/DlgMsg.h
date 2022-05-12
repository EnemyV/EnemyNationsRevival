// DlgMsg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgModelessMsg dialog

class CDlgModelessMsg : public CDialog
{
// Construction
public:
	CDlgModelessMsg(CWnd* pParent = NULL);   // standard constructor

	void Create ( const char * pMsg );

// Dialog Data
	//{{AFX_DATA(CDlgModelessMsg)
	enum { IDD = IDD_MODELESS_MSG };
	CString	m_sMsg;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgModelessMsg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgModelessMsg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
