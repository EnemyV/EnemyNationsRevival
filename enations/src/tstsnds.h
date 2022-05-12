// tstsnds.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgTestSounds dialog

class CDlgTestSounds : public CDialog
{
// Construction
public:
	CDlgTestSounds(CWnd* pParent = NULL);   // standard constructor
	int		GetTrack ();

// Dialog Data
	//{{AFX_DATA(CDlgTestSounds)
	enum { IDD = IDD_TEST_SOUNDS };
	CWndOD< CButton >	m_btnShut;
	CWndOD< CButton >	m_btnFore;
	CWndOD< CButton >	m_btnExcl;
	CWndOD< CButton >	m_btnBack;
	CString	m_strDesc;
	CString	m_strFormat;
	int		m_iTrack;
	int		m_iMidi;
	int		m_iGroup;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgTestSounds)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgTestSounds)
	virtual BOOL OnInitDialog();
	afx_msg void OnPlayBack();
	afx_msg void OnPlayExcl();
	afx_msg void OnPlayFore();
	afx_msg void OnSfxGroup();
	afx_msg void OnPlayShutup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
