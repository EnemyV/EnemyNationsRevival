// nbdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNbDlg dialog

class CNbDlg : public CDialog
{
// Construction
public:
	CNbDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNbDlg)
	enum { IDD = IDD_NETBIOS };
	CString	m_lana;
	CString	m_stationName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNbDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNbDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
