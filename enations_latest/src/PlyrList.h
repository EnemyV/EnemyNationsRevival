// PlyrList.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgPlyrList dialog

class CDlgPlyrList : public CDialog
{
// Construction
public:
	CDlgPlyrList(CWnd* pParent = NULL);   // standard constructor

	void		NameChange ( CPlayer * pPlyr );
	void		RemovePlayer ( CPlayer * pPlyr );

// Dialog Data
	//{{AFX_DATA(CDlgPlyrList)
	enum { IDD = IDD_PLYR_LIST };
	CListBox	m_lstPlayers;
	CWndOD< CButton >	m_btnKill;
	BOOL	m_btnTakeOver;
	CString	m_strPassWord;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgPlyrList)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy() { delete this; }
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgPlyrList)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnPlyrKill();
	afx_msg void OnSelchangePlyrList();
	afx_msg void OnPlyrMinimize();
	afx_msg void OnPlyrOptions();
	afx_msg void OnPlyrPause();
	afx_msg void OnChangePlyrPassword();
	afx_msg void OnPlyrTakeOver();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
