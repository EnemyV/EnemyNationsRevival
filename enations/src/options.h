//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __OPTIONS_H__
#define __OPTIONS_H__


// options.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgOptions dialog

class CDlgOptions : public CDialog
{
// Construction
public:
	CDlgOptions (CWnd* pParent=NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgOptions)
	enum { IDD = IDD_OPTIONS };
	CSliderCtrl	m_sldSpeed;
	CSliderCtrl	m_sldSound;
	CSliderCtrl	m_sldMusic;
	CScrollBar	m_scrSpeed;
	CScrollBar	m_scrSound;
	CScrollBar	m_scrMusic;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgOptions)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgOptions)
	afx_msg void OnFileAdv();
	afx_msg void OnFileHelp();
	afx_msg void OnFileVersion();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CDlgAdvOptions dialog

class CDlgAdvOptions : public CDialog
{
// Construction
public:
	CDlgAdvOptions(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgAdvOptions)
	enum { IDD = IDD_ADV_OPTIONS };
	CComboBox	m_lstLang;
	int		m_iBlt;
	int		m_iMusic;
	int		m_iDepth;
	BOOL	m_iScroll;
	BOOL	m_iPause;
	int		m_iZoom;
	int		m_iRes;
	BOOL	m_iIntro;
	BOOL	m_iSysColors;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgAdvOptions)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgAdvOptions)
	afx_msg void OnHelp();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif

