// cdirdlg.h : header file
//

#ifndef CDIRDLG_H
#define CDIRDLG_H

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// cdirdlg dialog

class cdirdlg : public CDialog
{
// Construction
public:
	cdirdlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(cdirdlg)
	enum { IDD = IDD_DIRDIALOG };
	CComboBox	m_driveList;
	CListBox	m_dirList;
	//}}AFX_DATA
	int prevDriveSel;
	char aCurrentDirSel[ MAX_PATH ];

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cdirdlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(cdirdlg)
	afx_msg void OnDblclkDirlist();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeDrivelist();
	afx_msg void OnSelchangeDirlist();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
