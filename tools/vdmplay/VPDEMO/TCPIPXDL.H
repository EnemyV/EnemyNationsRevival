// tcpipxdl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTcpIpxDlg dialog

class CTcpIpxDlg : public CDialog
{
// Construction
public:
	CTcpIpxDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTcpIpxDlg)
	enum { IDD = IDD_TCPIPX };
	CString	m_port;
	CString	m_serverAdress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTcpIpxDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTcpIpxDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
