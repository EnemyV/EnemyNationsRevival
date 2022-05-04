//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __CREATSIN_H__
#define __CREATSIN_H__

#include "new_game.h"

// creatsin.h : header file
//

class CCreateSingle;


/////////////////////////////////////////////////////////////////////////////
// CDlgCreateSingle dialog

class CDlgCreateSingle : public CDialog
{
// Construction
public:
	CDlgCreateSingle(CWnd* pParent = NULL);   // standard constructor
	void Create (CCreateSingle * pCs, UINT id, CWnd *pPar = NULL);

	CCreateSingle *		m_pCs;

// Dialog Data
	//{{AFX_DATA(CDlgCreateSingle)
	enum { IDD = IDD_CREATE_SINGLE };
	CWndOD< CButton >	m_btnOk;
	int		m_iAiLevel;
	int		m_iSizeWorld;
	int		m_iPosStart;
	CString	m_strAiNum;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgCreateSingle)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgCreateSingle)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnHelp();
	afx_msg void OnChangeCreateNumAi();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CCreateSingle - holds everything for creating game

class CCreateSingle : public CCreateBase, public CCreateNewBase
{
public:
		CCreateSingle () : CCreateBase (CCreateBase::single) {}
		~CCreateSingle () { CloseAll (); }

		void	Init ();
		void  ClosePick ();
		void  CloseAll () { ClosePick (); }

		CCreateNewBase *	GetNew () { return (this); }

		CDlgCreateSingle		m_dlgCreateSingle;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

class CCreateLoadSingle : public CCreateBase, CCreateLoadBase
{
public:
		CCreateLoadSingle () : CCreateBase (CCreateBase::load_single) {}
		~CCreateLoadSingle () { CloseAll (); }

		CCreateLoadBase *	GetLoad () { return (this); }
		void  UpdateBtns () { if (m_dlgPickPlayer.m_hWnd != NULL) m_dlgPickPlayer.UpdateBtns (); }

		void	Init ();
		void  ClosePick ();
		void  CloseAll ();

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


#endif
