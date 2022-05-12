// comstatd.h : header file
//

#ifdef NOMFC
class CString 
{
public:

	CString() { m_str = NULL; }
	~CString() { if (m_str) free(m_str); }

	CString& operator=(LPCSTR s)
	{
		if (m_str) free(m_str);
		m_str = strdup(s);
		return *this;
	}

	operator LPCSTR() const { return m_str; }


	LPSTR m_str;
};


class CListBox { };
class CWnd { };
class CDataExchange { };
#define DECLARE_MESSAGE_MAP() 
#define afx_msg

class CDialog : public CWnd
{
public:
	 CDialog(UINT id, CWnd* pParnet = NULL) { m_hWnd = NULL; }
	 BOOL Create(UINT id, CWnd* pWnd = NULL);
	 void DestroyWindow() { ::DestroyWindow(m_hWnd); m_hWnd = NULL; }

	 virtual void OnOK();
	 virtual void OnCancel();
	 virtual void OnHelp() { }
	 virtual BOOL OnInitDialog() { return TRUE; }

	 HWND m_hWnd;


};
#endif

/////////////////////////////////////////////////////////////////////////////
// CComStatDlg dialog
class CDataLogger;
class CComStatDlg : public CDialog
{
// Construction
public:
	CComStatDlg(LPCSTR port, CDataLogger* log, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CComStatDlg)
	enum { IDD = IDD_COMM_STAT };
	CListBox	m_commTrace;
	CString	m_commStat;
	//}}AFX_DATA

    CDataLogger* m_log;
	CString      m_port;
	CString      m_number;

	void ShowNumber(LPCSTR num);
	void ShowStatus(UINT id);
	void ShowTrace(LPCSTR s, UINT size);
	BOOL Create(UINT id, CWnd* pWnd=NULL) { return CDialog::Create(id, pWnd); }
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CComStatDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL



// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CComStatDlg)
	virtual void OnCancel();
	afx_msg void OnHelp();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
