
/////////////////////////////////////////////////////////////////////////////
// CAdvComDlg dialog

class CAdvComDlg : public CDialog
{
// Construction
public:
	CAdvComDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAdvComDlg)
	enum { IDD = IDD_ADV_COMM };
	CComboBox	m_port;
	CString	m_portStr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdvComDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAdvComDlg)
	virtual void OnOK();
	afx_msg void OnHelp();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////
// CAdvIpxDdlg dialog

class CAdvIpxDdlg : public CDialog
{
// Construction
public:
	CAdvIpxDdlg(BOOL serverMode, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAdvIpxDdlg)
	enum { IDD = IDD_ADV_IPX };
	CString	m_netNumber;
	CString	m_nodeNumber;
	UINT	m_socketNumber;
	//}}AFX_DATA

	BOOL m_serverMode;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdvIpxDdlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAdvIpxDdlg)
	virtual void OnOK();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CAdvMdmDlg dialog

class CAdvMdmDlg : public CDialog
{
// Construction
public:
	CAdvMdmDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAdvMdmDlg)
	enum { IDD = IDD_ADV_MODEM };
	CComboBox	m_port;
	CString	m_portStr;
	CString	m_number;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdvMdmDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAdvMdmDlg)
	virtual void OnOK();
	afx_msg void OnHelp();
	afx_msg void OnModemAdv();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CAdvMdmSetting dialog

class CAdvMdmSetting : public CDialog
{
// Construction
public:
	CAdvMdmSetting(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAdvMdmSetting)
	enum { IDD = IDD_ADV_MODEM_SETTINGS };
	CString	m_initStr;
	CString	m_dialPrefix;
	CString	m_dialSuffix;
	CString	m_autoAnswer;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdvMdmSetting)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAdvMdmSetting)
	afx_msg void OnHelp();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CAdvNbDlg dialog

class CAdvNbDlg : public CDialog
{
// Construction
public:
	CAdvNbDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAdvNbDlg)
	enum { IDD = IDD_ADV_NETBIOS };
	CComboBox	m_lanaCtl;
	CString	m_stationName;
	CString	m_lanaStr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdvNbDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAdvNbDlg)
	virtual void OnOK();
	afx_msg void OnHelp();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CAdvTcpDlg dialog

class CAdvTcpDlg : public CDialog
{
// Construction
public:
	CAdvTcpDlg(BOOL serverMode, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAdvTcpDlg)
	enum { IDD = IDD_ADV_TCPIP };
	CString	m_serverAddr;
	UINT	m_socket;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdvTcpDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	BOOL m_serverMode;
// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAdvTcpDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

