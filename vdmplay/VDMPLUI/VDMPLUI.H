#ifndef __VDMPLUI_H__
#define	__VDMPLUI_H__

// vdmplui.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CNBDlg dialog

class CNBDlg : public CDialog
{
// Construction
public:
	CNBDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNBDlg)
	enum { IDD = IDD_NETBIOS };
	BYTE	m_lana;
	CString	m_stationName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNBDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNBDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CIpxDlg dialog

class CIpxDlg : public CDialog
{
// Construction
public:
	CIpxDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CIpxDlg)
	enum { IDD = IDD_IPX };
	UINT	m_portNumber;
	CString	m_serverNet;
	CString	m_serverNode;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIpxDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CIpxDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CTcpDlg dialog

class CTcpDlg : public CDialog
{
// Construction
public:
	CTcpDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTcpDlg)
	enum { IDD = IDD_TCP };
	UINT	m_portNumber;
	CString	m_serverAddress;
	CString	m_regSrv;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTcpDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTcpDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CMdmDlg dialog

class CMdmDlg : public CDialog
{
// Construction
public:
	CMdmDlg(BOOL forModem, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMdmDlg)
	enum { IDD = IDD_MODEM };
	CComboBox	m_portName;
	CString	m_callSetup;
	CString	m_dialNumber;
	CString	m_listenSetup;
	CString	m_portNameStr;
	//}}AFX_DATA

	BOOL m_forModem;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMdmDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMdmDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
