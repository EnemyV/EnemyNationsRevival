// sendmsgd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSendMsgDlg dialog

class CSendMsgDlg : public CDialog
{
// Construction
public:
	CSendMsgDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSendMsgDlg)
	enum { IDD = IDD_SENDMSGDIALOG };
	CString	m_messageText;
	CString	m_playerId;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSendMsgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSendMsgDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
