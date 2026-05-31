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

    // Route custom Win32 messages that the dead MFC message map can no longer
    // register.  CWndStub::WindowProc handles standard WM_*, but WM_VPNOTIFY
    // etc. are user-range messages that need explicit dispatch here.
    virtual LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

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
	afx_msg void OnActivateApp(BOOL bActive, DWORD hTask);
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
	std::string	m_sText;				// license agreement
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


// CDlgRandNum removed (cheat seed-override dialog)

// CDlgAiPos removed (CHEAT-only AI position dialog)


// CDlgFile removed (Phase 2d) — replaced by SDL2FileDialog (in SDL2FileDialog.cpp).
// The MFC Save/Load/Options/Speed/Sound/Music/Exit dialog is gone; the SDL2
// equivalent is invoked from the toolbar Options button.


/////////////////////////////////////////////////////////////////////////////
// CDlgSaveMsg — modeless save progress indicator (SDL2-rendered).
//
// Owns an internal SDL2 dialog object that GameWindow renders per frame.
// UpdateData(FALSE) pushes m_sText/m_sStat into the dialog labels.

class _SaveProgressDialog;  // defined privately in main.cpp

class CDlgSaveMsg
{
public:
	CDlgSaveMsg(CWnd* pParent = NULL);
	~CDlgSaveMsg();

	void Create( UINT nIDTemplate, CWnd* pParent );
	void DestroyWindow();
	void UpdateData( BOOL bSaveAndValidate );

	std::string	m_sText;
	std::string	m_sStat;

private:
	_SaveProgressDialog* m_pDlg;  // owned by GameWindow (heap, deleted on EndDialog cleanup)
};


/////////////////////////////////////////////////////////////////////////////
// CDlgPause — modeless pause notification
// No longer inherits CDialog. Uses a Win32 popup window.

class CDlgPause
{
public:
	CDlgPause(CWnd* pParent = NULL);
	~CDlgPause();

	void Show( int iMode );
	void DestroyWindow();
	enum { server, client, off };

	HWND m_hWnd;
	std::string m_sText;

private:
	void Repaint();

	static LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static const char* s_className;
	static bool s_classRegistered;
};

#endif
