//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __CUTSCENE_H__
#define __CUTSCENE_H__

#include <wndbase.h>
#include <subclass.h>
#include "resource.h"
#include "bmbutton.h"

// cutscene.h : header file
//


const int CUT_OK = 1;
const int CUT_CANCEL = 2;
const int IDC_SCENE_OK = 1000;
const int IDC_SCENE_CANCEL = 1001;
//const int IDC_SCENE_SAVE = 1002;


/////////////////////////////////////////////////////////////////////////////
// CCutScene

class CCutScene
{
public:
							CCutScene ();
							~CCutScene ();

		UINT			PlayCutScene (int iScenario, BOOL bRepeat, BOOL bAsync = FALSE);

		void			PlayEnd (int iEnd, BOOL bAsync = FALSE);

		UINT			_PlayScene (int iTyp, int iScenario = -1, BOOL bAsync = FALSE);
};

/////////////////////////////////////////////////////////////////////////////
// CCutScene

class CWndCutScene : public CWndBase
{
// Construction
public:
	CWndCutScene () { m_pdibPicture = NULL; }

	void		Create (int iTyp, int iScenario = -1);
				enum { cut, repeat, scenario_end, win, lose };

// Attributes
public:
	int			GetRtn () const { return m_iRtn; }

// Operations
public:

// Implementation
public:
	virtual ~CWndCutScene () {}

protected:
	void		OnCreateCut ();
	void		OnCreateOther (int id);
	void		OnPaintCut (CDC & dc);
	void		OnPaintWin (CDC & dc);
	void		OnPaintLose (CDC & dc);

	// Generated message map functions
	//{{AFX_MSG(CWndCutScene)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSceneOk();
	afx_msg void OnSceneCancel();
	afx_msg void OnSceneSave();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CDIB *			m_pdibPicture;
	CString			m_sText;
	int					m_iTyp;
	int					m_iRtn;
	int					m_iScenario;
	CFont				m_font;

	CTextButton m_btnOK;
	CTextButton m_btnCancel;
	CTextButton m_btnSave;
	BOOL				m_bTile;
};

/////////////////////////////////////////////////////////////////////////////
// CDlgScnDesc dialog

class CDlgScnDesc : public CDialog
{
// Construction
public:
	CDlgScnDesc(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgScnDesc)
	enum { IDD = IDD_SCENARIO_DESC };
	CWndOD< CButton >	m_btnOk;
	CWndOD< CButton >	m_btnSave;
	CString	m_strDesc;
	//}}AFX_DATA

	BOOL			m_bRepeat;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgScnDesc)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgScnDesc)
	virtual BOOL OnInitDialog();
	afx_msg void OnSceneSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


extern CCutScene theCutScene;


#endif
