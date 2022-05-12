#ifndef __CDLOC_H__
#define __CDLOC_H__

// CdLoc.h : header file
//

BOOL CheckForCD ();

/////////////////////////////////////////////////////////////////////////////
// CDlgCdLoc dialog

class CDlgCdLoc : public CDialog
{
// Construction
public:
	CDlgCdLoc(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgCdLoc)
	enum { IDD = IDD_CD_LOC };
	CComboBox	m_cbDrives;
	CString	m_strStat;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgCdLoc)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgCdLoc)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
