// vpdiagdl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVpdiagDlg dialog

class CVpdiagDlg : public CDialog
{
// Construction
public:
	CVpdiagDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CVpdiagDlg)
	enum { IDD = IDD_VPDIAG_DIALOG };
	CButton	m_testIpxBtn;
	CButton	m_testTcpBtn;
	CButton	m_testNetbiosBtn;
	CListBox	m_lanaListCtl;
	CString	m_ipxAddress;
	CString	m_tcpHostName;
	CString	m_tcpAddress;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVpdiagDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static void EnumLanaCallback(UINT lana, LPVOID context);

protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CVpdiagDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTestIpx();
	afx_msg void OnTestNetbios();
	afx_msg void OnTestTcp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
