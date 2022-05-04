// StatusDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStatusDlg dialog

class CStatusDlg : public CDialog
{
// Construction
public:
	CStatusDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CStatusDlg)
	enum { IDD = IDD_DLG_ERR };
	CListBox	m_lstErr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatusDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CStatusDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
