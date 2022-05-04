//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __RELATION_H__
#define __RELATION_H__


// relation.h : header file
//

#ifdef _CHEAT
class CDlgStats;
extern CDlgStats * pDlgStats;
#endif
class CPlayer;


/////////////////////////////////////////////////////////////////////////////
// CDlgRelations dialog

class CDlgRelations : public CDialog
{
// Construction
public:
	CDlgRelations(CWnd* pParent = NULL);   // standard constructor

	static void	NewRelations (CPlayer * pPlyr, int iLevel);
	static void	NewAreaSelection ();

	void	NewAreaSelection ( BOOL bItemsSel );
	void	ChangedIfAi ();
	void	RemovePlayer ( CPlayer * pPlr );

// Dialog Data
	//{{AFX_DATA(CDlgRelations)
	enum { IDD = IDD_RELATIONS };
	CWndOD< CButton >	m_btnGive;
	CListBox	m_lstPlayers;
	int		m_btnRelations;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgRelations)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy() { delete this; }
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgRelations)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDblclkRelList();
	afx_msg void OnSelchangeRelList();
	afx_msg void OnRadio();
	afx_msg void OnDestroy();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnPlyrGive();
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#ifdef _CHEAT

/////////////////////////////////////////////////////////////////////////////
// CDlgStats dialog

class CDlgStats : public CDialog
{
// Construction
public:
	CDlgStats(CWnd* pParent = NULL);   // standard constructor

	void		Update ();

// Dialog Data
	//{{AFX_DATA(CDlgStats)
	enum { IDD = IDD_PLYR_STAT };
	CString	m_strFood;
	CString	m_strGas;
	CString	m_strName;
	CString	m_strPeople;
	CString	m_strPower;
	//}}AFX_DATA

	int		m_iPlyrNum;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgStats)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy() { delete this; }
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgStats)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif


#endif
