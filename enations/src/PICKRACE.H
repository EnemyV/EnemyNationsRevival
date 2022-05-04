//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __PICKRACE_H__
#define __PICKRACE_H__

#include "resource.h"


// pickrace.h : header file
//

class CCreateBase;
class CCreateLoadBase;


/////////////////////////////////////////////////////////////////////////////
// CDlgPickWait dialog

class CDlgPickWait : public CDialog
{
// Construction
public:
	CDlgPickWait(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgPickWait)
	enum { IDD = IDD_PICK_WAIT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgPickWait)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgPickWait)
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CDlgPickRace dialog

class CDlgPickRace : public CDialog
{
// Construction
public:
	CDlgPickRace(CWnd* pParent = NULL);   // standard constructor

	void Create (CCreateBase * pCb, CDialog *pDlgPrev, UINT id, CWnd *pPar = NULL);

// Dialog Data
	//{{AFX_DATA(CDlgPickRace)
	enum { IDD = IDD_PICK_RACE };
	CWndOD< CButton >	m_btnOk;
	CListBox	m_lstRace;
	CString	m_sName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgPickRace)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	CString	m_sDesc;
	CDialog *				m_pdlgPrev;
	CCreateBase *		m_pCb;
	CRect						m_rFrame;			// frame around art
	CRect						m_rText;			// where to put the text
	CRect						m_rPicture;		// where to put the picture
	CDIB *					m_pDib;				// scratch DIB for painting
	CDIB *					m_pPicture;		// the race picture

	// Generated message map functions
	//{{AFX_MSG(CDlgPickRace)
	afx_msg void OnSelchangeRaceList();
	afx_msg void OnDblclkRaceList();
	afx_msg void OnChangeRaceName();
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnRacePrev();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CDlgPickPlayer dialog

class CDlgPickPlayer : public CDialog
{
// Construction
public:
	CDlgPickPlayer(CWnd* pParent = NULL);   // standard constructor

	void Create (CCreateBase * pCb, UINT id, CWnd *pPar = NULL);
	void UpdateBtns () { OnSelchangeRaceList(); }

	CDlgPickWait		m_dlgWait;

// Dialog Data
	//{{AFX_DATA(CDlgPickPlayer)
	enum { IDD = IDD_PICK_PLAYER };
	CWndOD< CButton >	m_btnOk;
	CListBox	m_lstRace;
	CString	m_sDesc;
	CString	m_sName;
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgPickPlayer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	CDialog *						m_pdlgPrev;
	CCreateBase *		m_pCb;

	// Generated message map functions
	//{{AFX_MSG(CDlgPickPlayer)
	afx_msg void OnSelchangeRaceList();
	afx_msg void OnDblclkRaceList();
	afx_msg void OnChangeRaceName();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnRacePrev();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif
