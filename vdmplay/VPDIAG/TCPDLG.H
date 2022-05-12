// TcpDlg.h : header file
//

class CNetSocket;
/////////////////////////////////////////////////////////////////////////////
// CTcpDlg dialog

class CTcpDlg : public CDialog
{
// Construction
public:
	CTcpDlg(CWnd* pParent = NULL, BOOL ipx=FALSE);   // standard constructor
	~CTcpDlg();

// Dialog Data
	//{{AFX_DATA(CTcpDlg)
	enum { IDD = IDD_TCP_DIALOG };
	CButton	m_doEchoCtl;
	CButton	m_startBtn;
	CEdit	m_remoteAddressCtl;
	CListBox	m_messages;
	BOOL	m_broadcast;
	CString	m_localAddress;
	UINT	m_port;
	CString	m_remoteAddress;
	BOOL	m_server;
	BOOL	m_doEcho;
	//}}AFX_DATA

	BOOL	m_autoMode;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTcpDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	void ShowMessage(LPCSTR msg);

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTcpDlg)
	virtual void OnOK();
	afx_msg void OnServer();
	afx_msg void OnBroadcast();
	afx_msg LONG OnWinsock(WPARAM, LPARAM);
	virtual BOOL OnInitDialog();
	afx_msg void OnDoecho();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	CNetSocket* m_sock;
	BOOL m_ipx;
};
