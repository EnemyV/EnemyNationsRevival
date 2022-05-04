#ifndef __DLGMSG_H__
#define __DLGMSG_H__

// DlgMsg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgMsg dialog

class CDlgMsg : public CDialog
{
// Construction
public:
	CDlgMsg(CWnd* pParent = NULL);   // standard constructor

	int		MsgBox (char const * psPrompt, UINT nType, char const * psEntry, char const * psSection, int iDefault = IDYES );
	int		MsgBox (UINT nIDPrompt, UINT nType, char const * psEntry, char const * psSection, int iDefault = IDYES );

	CString		m_sSection;		// for check box
	CString		m_sEntry;

// Dialog Data
	//{{AFX_DATA(CDlgMsg)
	enum { IDD = IDD_MSG };
	BOOL	m_btnCheck;
	CString	m_sText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgMsg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgMsg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
