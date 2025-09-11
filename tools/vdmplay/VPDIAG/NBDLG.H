// NbDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNbDlg dialog

#include <afxwin.h>      // Main MFC header
#include <afxext.h>      // For extended MFC stuff (dialogs, etc.)
#include "resource.h"

class CNbDlg : public CDialog
{
// Construction
public:
	CNbDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNbDlg)
	enum { IDD = IDD_NETBIOS_DIALOG };
	CButton	m_serverChkBox;
	CListBox	m_lanas;
	CButton	m_startBtn;
	CListBox	m_messages;
	BOOL	m_server;
	CString	m_selectedLana;
	//}}AFX_DATA

	BOOL m_autoMode;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNbDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static void LoadLana(UINT lana, LPVOID context);
	static void FoundLana(UINT clientLana, UINT serverLana, LPVOID context);
	static void LookForServer(UINT lana, LPVOID context);

	void ShowMessage(LPCSTR msg);

protected:

	// Generated message map functions
	//{{AFX_MSG(CNbDlg)
	virtual void OnOK();
	afx_msg void OnDestroy();
	afx_msg void OnSave();
	afx_msg void OnClose();
	afx_msg LONG OnNbNotify(WPARAM w, LPARAM l);
	virtual BOOL OnInitDialog();
	afx_msg void OnServer();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
