//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __SCENARIO_H__
#define __SCENARIO_H__

#include "resource.h"
#include "new_game.h"

// scenario.h : header file
//

const int NUM_SCENARIOS = 12;


class CCreateScenario;

const int NUM_AI_IN_SCENARIO = 3;
const int SCENARIO_POS = 4;


/////////////////////////////////////////////////////////////////////////////
// CDlgScenario dialog

class CDlgScenario : public CDialog
{
// Construction
public:
	CDlgScenario (CWnd* pParent = NULL);   // standard constructor

	void Create (CCreateScenario *pCs, UINT id, CWnd *pPar = NULL);

// Dialog Data
	//{{AFX_DATA(CDlgScenario)
	enum { IDD = IDD_SCENARIO };
	CWndOD< CButton >	m_btnOk;
	int		m_iAi;
	int		m_iSize;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgScenario)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	CCreateScenario *		m_pCs;

	// Generated message map functions
	//{{AFX_MSG(CDlgScenario)
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CCreateScenario - holds everything for creating game

class CCreateScenario : public CCreateBase, public CCreateNewBase
{
public:
		CCreateScenario () : CCreateBase (CCreateBase::scenario) {}
		~CCreateScenario () { Close (); }

		CCreateNewBase *	GetNew () { return (this); }

		void	Init ();
		void	Close ();

		CDlgScenario		m_dlgScenario;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


#endif
