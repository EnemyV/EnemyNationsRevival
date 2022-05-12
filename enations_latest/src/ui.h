//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __UI_H__
#define __UI_H__


#include "resource.h"
#include "to_wndrd.h"
#include "netapi.h"
#include "terrain.h"

// ui.h : header file
//

const	DWORD dwStatusWndStyle = WS_CLIPCHILDREN | WS_CHILD | WS_VISIBLE;
const	DWORD dwPopWndStyle = WS_CLIPCHILDREN | WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME;
void PaintBevel (CDC & dc, CRect & rEdge, unsigned uWid, CBrush & brTop, CBrush & brBottom);
void BuildBldgDest (CVehicle *pVeh, int iBldg, int iDir, CHexCoord & hex);

class CDlgMain;
class CUnit;
class CDlgBuildTransport;
class CDlgBuildStructure;
class CNetAnsTile;


void SaveExistingGame ();


/////////////////////////////////////////////////////////////////////////////
// CWndMain window

class CWndMain : public CWndBase
{
// Construction
public:
	CWndMain() {}

	void		Create ();

// Attributes
public:
	enum PROG_POS { loading, demo_license, movie, retail_license, playing, game_end, exiting };
	void				SetProgPos ( PROG_POS ppMode );
	PROG_POS		GetProgPos () const { return m_progPos; }
	void 				_OnPause (BOOL bPause);
	void				_EnableGameWindows ( BOOL bEnable );
	void				EndLicense ();
	void				LoadData ();

// Operations
public:

// Implementation
public:
	virtual ~CWndMain() {}

protected:
	void		OnDisplayChange2 ();
	void		DrawScreen ( CRect const & rectDst );

	// Generated message map functions
	//{{AFX_MSG(CWndMain)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnMyDisplayChange (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDisplayChange (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNetMsg (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNetFlowOn (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNetFlowOff (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnActivateMusicMsg (WPARAM wParam, LPARAM lParam);
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnHide();
	afx_msg void OnUnHide();
	afx_msg void OnSave();
	afx_msg void OnArea();
	afx_msg void OnBoss();
	afx_msg void OnHelp();
	afx_msg void OnMail();
	afx_msg void OnOptions();
	afx_msg void OnWorld();
	afx_msg void OnResearch();
	afx_msg void OnDiplomat();
	afx_msg void OnBuildings();
	afx_msg void OnVehicles();
	afx_msg void OnNext();
	afx_msg void OnPrev();
	afx_msg void OnPause();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg BOOL OnQueryNewPalette();
	afx_msg BOOL OnQueryEndSession();
	afx_msg void OnCloseApp();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CDIB *			m_pcdibWall;
	int					m_iWid;
	int					m_iHt;
	PROG_POS		m_progPos;
	CString			m_sText;				// license agreement
	CFont				m_fnt;
	BOOL				m_bPauseOnActive;
	BOOL				m_bLicTimer;
	int					m_iBtnDown;
};

inline LRESULT CWndMain::OnNetMsg (WPARAM wParam, LPARAM lParam)
		{ return (CNetApi::OnNetMsg (wParam, lParam)); }
inline LRESULT CWndMain::OnNetFlowOn (WPARAM , LPARAM )
		{ CNetApi::OnNetFlowOn (); return 0; }
inline LRESULT CWndMain::OnNetFlowOff (WPARAM , LPARAM )
		{ CNetApi::OnNetFlowOff (); return 0; }


/////////////////////////////////////////////////////////////////////////////
// CDlgRandNum dialog

class CDlgRandNum : public CDialog
{
// Construction
public:
	CDlgRandNum(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgRandNum)
	enum { IDD = IDD_DIALOG_RAND };
	CString	m_sNum;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgRandNum)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgRandNum)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CDlgAiPos dialog

class CDlgAiPos : public CDialog
{
// Construction
public:
	CDlgAiPos(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgAiPos)
	enum { IDD = IDD_AI_POS };
	CWndOD< CButton >	m_btnGoto;
	CListBox	m_lstBox;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgAiPos)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgAiPos)
	virtual void OnOK();
	afx_msg void OnDblclkListAi();
	afx_msg void OnSelchangeListAi();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CDlgFile dialog

class CDlgFile : public CDialog
{
// Construction
public:
	CDlgFile(CWnd* pParent = NULL);   // standard constructor

	void		SetState ();
	void		SetSpeed ();

// Dialog Data
	//{{AFX_DATA(CDlgFile)
	enum { IDD = IDD_FILE };
	CWndOD< CButton >	m_btnMission;
	CSliderCtrl	m_sldMusic;
	CSliderCtrl	m_sldSound;
	CSliderCtrl	m_sldSpeed;
	CScrollBar	m_scrSpeed;
	CScrollBar	m_scrSound;
	CScrollBar	m_scrMusic;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgFile)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgFile)
	virtual BOOL OnInitDialog();
	afx_msg void OnFileSave();
	afx_msg void OnFileHelp();
	afx_msg void OnFileExit();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnFileVersion();
	afx_msg void OnFileMinimize();
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnFileScene();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CDlgSaveMsg dialog

class CDlgSaveMsg : public CDialog
{
// Construction
public:
	CDlgSaveMsg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgSaveMsg)
	enum { IDD = IDD_SAVE_MSG };
	CString	m_sText;
	CString	m_sStat;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgSaveMsg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgSaveMsg)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CDlgPause dialog

class CDlgPause : public CDialog
{
// Construction
public:
	CDlgPause(CWnd* pParent = NULL);   // standard constructor

	void		Show (int iMode);
				enum { server, client, off };

// Dialog Data
	//{{AFX_DATA(CDlgPause)
	enum { IDD = IDD_PAUSE_MSG };
	CString	m_sText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgPause)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy() { delete this; }
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgPause)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
