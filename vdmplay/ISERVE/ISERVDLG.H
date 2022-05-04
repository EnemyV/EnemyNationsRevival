// iservdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIserveDlg dialog

class CIserveDlg : public CDialog
{
// Construction
public:
	CIserveDlg(CWnd* pParent = NULL);	// standard constructor

	void	UpdateCaption ();

// Dialog Data
	//{{AFX_DATA(CIserveDlg)
	enum { IDD = IDD_ISERVE_DIALOG };
	CListBox	m_ServerList;
	CString	m_strAddr;
	CString	m_strName;
	CString	m_strSocket;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIserveDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CIserveDlg)
	virtual BOOL OnInitDialog();
	afx_msg LONG OnStartMsg( UINT, LONG );
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnStart();
	afx_msg void OnStop();
	afx_msg void OnExit();
	afx_msg void OnOptions();
	afx_msg void OnHelp();
	afx_msg void OnAbout();
	afx_msg LONG OnVpNotify(WPARAM w, LPARAM l);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CString GetSessionData(LPCVPSESSIONINFO info);
 	void OnSessionEnumNotify(LPCVPMESSAGE m);
	void OnServerDownNotify(LPCVPMESSAGE m);

	CString  	GetStationAddress();
	CString 	GetAddressString(LPCVPNETADDRESS a);
	void 		FixStationName();

public:
	CString 	m_gameId;
	DWORD		m_sesInfoSize;
	DWORD   	m_plrInfoSize;
	int			m_protocol;
	VPHANDLE 	m_vpH;

};
/////////////////////////////////////////////////////////////////////////////
// CDlgSocket dialog

class CDlgSocket : public CDialog
{
// Construction
public:
	CDlgSocket(CWnd* pParent, int protocol);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgSocket)
	enum { IDD = IDD_SOCKET };
	int		m_iSocket;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgSocket)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgSocket)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int m_protocol;
};
