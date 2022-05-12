//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __CREATMUL_H__
#define __CREATMUL_H__

#include "new_game.h"

// creatmul.h : header file
//

class CCreateMultiBase;
class CCreateMulti;
class CCreateLoadMulti;
class CPlayer;


/////////////////////////////////////////////////////////////////////////////
// CDlgLoadMulti dialog

class CDlgLoadMulti : public CDialog
{
// Construction
public:
	CDlgLoadMulti(CWnd* pParent = NULL);   // standard constructor

	void			Create (CCreateLoadMulti * pLm, UINT id, CWnd *pPar);

// Dialog Data
	//{{AFX_DATA(CDlgLoadMulti)
	enum { IDD = IDD_LOAD_MULTI };
	CWndOD< CButton >	m_btnOK;
	CString	m_sDesc;
	CString	m_sName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgLoadMulti)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

		CCreateLoadMulti * 	m_pLm;

	// Generated message map functions
	//{{AFX_MSG(CDlgLoadMulti)
	afx_msg void OnHelp();
	afx_msg void OnChangeText();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnPrev();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CDlgCreateMulti dialog

class CDlgCreateMulti : public CDialog
{
// Construction
public:
	CDlgCreateMulti(CWnd* pParent = NULL);   // standard constructor
	void Create (CCreateMulti * pCm, UINT id, CWnd *pPar = NULL);

	CCreateMulti *		m_pCm;

// Dialog Data
	//{{AFX_DATA(CDlgCreateMulti)
	enum { IDD = IDD_CREATE_MULTI };
	CWndOD< CButton >	m_btnOk;
	CString	m_sDesc;
	int		m_iAi;
	CString	m_sName;
	int		m_iSize;
	int		m_iPos;
	CString	m_sPlayer;
	CString	m_strNumAI;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgCreateMulti)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgCreateMulti)
	afx_msg void OnHelp();
	afx_msg void OnChangeCreateText();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeCreateNumAi();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CDlgCreatePublish dialog

class CDlgCreatePublish : public CDialog
{
// Construction
public:
	CDlgCreatePublish(CWnd* pParent = NULL);   // standard constructor
	~CDlgCreatePublish ();
	void Create (CCreateMultiBase * pCm, CDialog *pPrev, UINT id, CWnd *pPar = NULL);

	CCreateMulti *			GetNew ();
	CCreateLoadMulti *	GetLoad ();


	int								m_prevConn;			// track to not re-determine radio button enabling
	int								m_idOK;
	CCreateMultiBase * m_pCm;
	CDialog *					m_pPrev;

// Dialog Data
	//{{AFX_DATA(CDlgCreatePublish)
	enum { IDD = IDD_CREATE_PUBLISH };
	CWndOD< CButton >	m_btnAdv;
	CWndOD< CButton >	m_btnUnpub;
	CWndOD< CButton >	m_btnOk;
	int		m_iConn;
	int		m_iJoin;
	BOOL	m_bServer;
	CString	m_strPw;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgCreatePublish)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void		UpdateBtns ();

	// Generated message map functions
	//{{AFX_MSG(CDlgCreatePublish)
	afx_msg void OnHelp();
	afx_msg void OnCreateAdv();
	afx_msg void OnUnpublish();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CCreateMulti - holds everything for creating game

class CCreateMultiBase : public CMultiBase
{
public:
		CCreateMultiBase (int iTyp);

		void  ClosePick ();
		void  CloseAll ();

		void RemovePlayer (CPlayer * pPlr) { m_wndPlyrList.RemovePlayer (pPlr); }
		void UpdatePlyrStatus (CPlayer * pPlyr, int iStatus)
									{ if (m_wndPlyrList.m_hWnd != NULL) m_wndPlyrList.UpdatePlyrStatus (pPlyr, iStatus); }
		void AddPlayer (CPlayer * pPlyr)
									{ m_wndPlyrList.AddPlayer (pPlyr); }
		void  UpdateBtns () { if (m_wndPlyrList.m_hWnd != NULL) m_wndPlyrList.UpdateBtns (); }

		CDlgCreatePublish	m_dlgCreatePublish;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

class CCreateMulti : public CCreateMultiBase, public CCreateNewBase
{
public:
		CCreateMulti () : CCreateMultiBase (CCreateBase::create_net) {}
		~CCreateMulti () { CloseAll (); }

		CCreateNewBase *	GetNew () { return (this); }

		void	Init ();
		void	ClosePick ();
		void	CloseAll ();

		CDlgCreateMulti		m_dlgCreateMulti;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


class CCreateLoadMulti : public CCreateMultiBase, public CCreateLoadBase
{
public:
		CCreateLoadMulti () : CCreateMultiBase (CCreateBase::load_multi) {}
		~CCreateLoadMulti () { CloseAll (); }

		void  UpdateBtns () { if (m_dlgPickPlayer.m_hWnd != NULL) m_dlgPickPlayer.UpdateBtns (); }
		CCreateLoadBase *	GetLoad () { return (this); }

		void	Init ();
		void  ClosePick ();
		void  CloseAll ();

		CDlgLoadMulti				m_dlgLoadName;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


#endif
