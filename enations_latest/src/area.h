//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __AREA_H__
#define __AREA_H__

#include "ui.h"

// area.h : header file
//

const int NUM_AREA_BUTTONS = 17;
const int NUM_STATUS_TEXT = 2;
const int MIN_TEXT_WID = 20;						// first status is this length, second at least this
const int AREA_BTN_HT = 38;							// height of button backdrop
const int AREA_TEXT_HT = 28;						// height of text backdrop

const int AREA_BTN_X_SKIP = 4;					// x skip between buttons
const int AREA_BTN_Y_START = 4;					// where buttons start on the background


class CWndInfo;
class CWndOrders;
class CWndListUnits;
class CWndWorld;
class CAreaList;



/////////////////////////////////////////////////////////////////////////////
// CWndUnitStat window

class CWndUnitStat : public CWndStatBar
{
friend CUnit;
// Construction
public:
						CWndUnitStat ();
		void		SetUnit (CUnit * pUnit);
		CUnit *	GetUnit () const { return m_pUnit; }
		void		SetText (char const * pStr, CStatInst::IMPORTANCE iImp);
		void		UpdateStat ();

	// Generated message map functions
protected:

	//{{AFX_MSG(CWndUnitStat)
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CUnit *				m_pUnit;		// unit it is drawing for
};


/////////////////////////////////////////////////////////////////////////////
// CWndArea window

class CWndAreaStatic : public CWndBase
{
friend CWndArea;
public:
			CWndAreaStatic ();
			~CWndAreaStatic ();

	BOOL			PreCreate ();
	BOOL			OnCommand (WPARAM wParam, LPARAM lParam);
	void			SizeStatus ();
	void			EnableButton (int ID, BOOL bEnable);
	void			ShowButton (int ID, BOOL bShow);
	void			UpdateStat () { m_wndStat.UpdateStat (); }

	void			SetStatusText (char const * pStr, CStatInst::IMPORTANCE iImp) { m_wndStat.SetText (pStr, iImp); }
	void			EnableBar (BOOL bEnable)
												{ m_iNumStatusText = bEnable ? NUM_STATUS_TEXT - 1 : NUM_STATUS_TEXT; }

protected:
	//{{AFX_MSG(CWndAreaStatic)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnChildMouseMove (WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

			int						m_iXmin;
			int						m_iYmin;
			int						m_iStatusStrt;
			int						m_iStatusCraneStrt;
			int						m_iStatusNoCraneStrt;
			int						m_iNumStatusText;

			CBmButton			m_Btns [NUM_AREA_BUTTONS];
			CWndUnitStat	m_wndStat;								// unit status
};


/////////////////////////////////////////////////////////////////////////////
// CWndArea window

class CListUnits : public CList <CUnit *, CUnit *>
{
public:
			void		AddUnit (CUnit * pUnit, BOOL bDoList );
			void		RemoveUnit (CUnit * pUnit);
			void		RemoveAllUnits ( BOOL bDoList );
};


/////////////////////////////////////////////////////////////////////////////
// CWndArea window

class CWndArea : public CWndAnim
{
friend CWndAreaStatic;
friend CWndListUnits;
friend CWndWorld;
friend CAreaList;
// Construction
public:
	// for m_iMode
	enum { normal, normal_select, build_ready, build_loc, ask_tile, rocket_ready, rocket_pos, rocket_wait, veh_route, road_begin, road_set, repair_bldg };

	CWndArea ();
	~CWndArea ();

	BOOL		PreCreateWindow (CREATESTRUCT & cs);
	BOOL		OnCommand (WPARAM wParam, LPARAM lParam);
	void		GetClientRect( LPRECT lpRect ) const;
	void		SetStatusText (const char * psText = NULL, CStatInst::IMPORTANCE iImp = CStatInst::status)
																			{ m_WndStatic.SetStatusText (psText, iImp); }
	void		EnableButton (int ID, BOOL bEnable) { m_WndStatic.EnableButton (ID, bEnable); }
	void		ShowButton (int ID, BOOL bShow) { m_WndStatic.ShowButton (ID, bShow); }
	BOOL		IsButtonEnabled (int ID) const;

	void		SetButtonState ();
	void		SetMouseState ();
	void		BldgCurOff ();
	void		StatUnit (CUnit * pUnit);
	void		CheckZoomBtns ();

	void		GiveUnits ( CPlayer * pPlyr );

	CWnd *	_GetStatBar () { return &(m_WndStatic.m_wndStat); }

	void		GetPanAndVol (CUnit const * pUnit, int & iPan, int & iVol);
	void		SetDestAndSfx ( CVehicle * pVeh, CHexCoord const & hex );
	void		SetDestAndSfx ( CVehicle * pVeh, CSubHex const & sub );

	void		InvalidateSound () { m_bNewSound = TRUE; }
	void		UpdateSound ();

	CUnit *	GetStaticUnit () const { return m_WndStatic.m_wndStat.GetUnit (); }
	CUnit *	GetUnit () { return (m_pUnit); }
	CWndInfo *	GetInfo () { return (m_pWndInfo); }
	int			GetMode () const { return (m_iMode); }

	void		Create (CMapLoc const & ml, CUnit * pUnit, BOOL bFirst);
	void		ReRender ();
	void		DrawSelectionRect();
	void		RestoreSelectionRect();
	void		Draw();
	void		UnitDying (CUnit * pUnit);
	void		MaterialChange (CUnit const * pUnit);

	void		SelectNone () { m_lstUnits.RemoveAllUnits (TRUE); m_pUnit = NULL; }
	void		AddSelectUnit (CUnit * pUnit);
	void		SubSelectUnit (CUnit * pUnit);
	void		OnlySelectUnit (CUnit * pUnit);
	int			NumSelected () const { return m_lstUnits.GetCount (); }
	int			NumGiveable () const;
	void		GiveSelectedUnits ( CPlayer * pPlyr );

	void		InvalidateWindow (RECT * pRect = NULL);
	void		BuildOn (int iIndex);
	void		BuildWait ();
	void		SelectOff ();
	void		SetupStart ();
	void		SetupDone ();
	void		GotoOn (CVehicle *pUnit, int iMode, int iRouteType = 0, POSITION posRoute = 0);
	void		AttackOn (CVehicle *pUnit);
	void 		Center ( CMapLoc );
	void		Center (CUnit *pUnit);
	CAnimAtr &	GetAA () { return (m_aa); }

	void		ReCenter ();
	void		InvalidateStatus ();
	CWnd *	GetExpand ();

	void		MarkUpdateRect( CRect const * );

	CHexCoord	ToBuildUL (CHexCoord & hexCur);
	int		GetBuildDir () const { return ((m_aa.m_iDir + m_iBuildDir) & 3); }

	static	CString		sWndCls;

protected:
	CUnit * GetUnitOn (CSubHex & hex);
	void		CaptureMouse () { if (! m_bCapMouse) { SetCapture (); m_bCapMouse = TRUE; } }
	void		ReleaseMouse () { if (m_bCapMouse) { ReleaseCapture (); m_bCapMouse = FALSE; } }
	void		LoadStaticResources ();
	void		UnloadStaticResources ();
	void		ClrRoadIcons ();
	void		SetRoadIcons ( CHexCoord hex );

	void		PostNcDestroy ();

	// Generated message map functions
	//{{AFX_MSG(CWndArea)
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void LastCombat ();
	afx_msg void ZoomIn ();
	afx_msg void ZoomOut ();
	afx_msg void TurnClock ();
	afx_msg void TurnCounter ();
	afx_msg void ResClicked ();
	afx_msg void StopUnit ();
	afx_msg void ResumeUnit ();
	afx_msg void CenterUnit ();
	afx_msg void DestroyUnit ();
	afx_msg void StopDestroyUnit ();
	afx_msg void CurLeft ();
	afx_msg void CurUp ();
	afx_msg void CurRight ();
	afx_msg void CurDown ();
	afx_msg void RoadUnit ();
	afx_msg void CancelRoadUnit ();
	afx_msg void BuildUnit ();
	afx_msg void CancelBuildUnit ();
	afx_msg void RouteUnit ();
	afx_msg void UnloadUnit ();
	afx_msg void RetreatUnit ();
	afx_msg void RepairUnit ();
	afx_msg void OppoUnit ();
	afx_msg void CancelRepairUnit ();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnCloseWin ();
	afx_msg void OnDeselect();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CArray <DWORD, DWORD> m_aSaveSel[10];

	int						m_iMode;			// mode we are in

	int						m_cx;					// client window size
	int						m_cy;

	CUnit *				m_pUnit;			// the unit this window displays
	unsigned 			m_uFlags;			// what we have in the selection list
	enum {veh = 0x01, crane = 0x02, bldg = 0x04, fac = 0x08, 
						attk = 0x10, carrier = 0x20, carryable = 0x40, land_repairable = 0x80,
						sea_repairable = 0x0100, repair = 0x0200, non_crane = 0x0400, truck = 0x0800,
						non_truck = 0x1000, loaded = 0x2000, non_carrier = 0x4000, veh_hurt = 0x8000,
						destroying = 0x010000, can_stop = 0x020000, boat = 0x040000, lc_carryable = 0x080000 };

	unsigned			m_uMouseMode;	// what a LMB will do
							enum { lmb_normal, lmb_nothing, lmb_attack, lmb_goto, lmb_select, 
										lmb_repair_bldg, lmb_repair_self, lmb_load, lmb_unload };

	CRect					m_selRect;
	CPoint				m_selOrig;
	BOOL					m_bCapMouse;	// have we captured the mouse?
	CWndInfo *		m_pWndInfo;		// tooltip window

	CListUnits		m_lstUnits;
	BYTE *				m_pSelUnder;	// pixels under the selection box

	int						m_iXmin;
	int						m_iYmin;
	CWndAreaStatic m_WndStatic;	// the static window holding the buttons

	CAnimAtr			m_aa;
	int						m_iBuild;			// the index of the building picking a site for
	int						m_iFound;			// foundcost return
	int						m_iBuildDir;	// direction to build in
	int						m_iRouteType;	// route type to add
	POSITION			m_posRoute;				// route item to add after

	CHexCoord *		m_phexRoadPath;					// road const path
	CSprite * *		m_ppUnderSprite;				// sprite road const icon replaced
	int						m_iNumRoadHex;					// how many items in above arrays

	BOOL					m_bUpdateAll;
	BOOL					m_bNewSound;

	BOOL					m_bRBtnDown;	// TRUE if RMB down
	BOOL					m_bNewPos;
	CPoint				m_ptRMDN;			// point RMB was pressed at
	CPoint				m_ptRMB;			// point moved from with RMB
	CPoint				m_ptLMB;			// point moved from with LMB
	int						m_iMoveCur;		// which move cur

	BOOL					m_bShowRes;		// show the resource tiles

	CHexCoord			m_hexRoadStart;//where the road starts
	CColorBuffer		m_colorbuffer;

	CScrollBar			m_scrollbarH;
	CScrollBar			m_scrollbarV;
	BOOL					m_bScrollBars;

	static LRESULT CALLBACK MouseProc (int  nCode, WPARAM  wParam,	LPARAM  lParam);

	// this is all global stuff that all the windows can share
	static	int						m_iCount;			// count of windows using this

	static	HHOOK					m_hhk;

	static	CString				m_sHelp;			// help in normal mode
	static	CString				m_sHelpBuild;	// what to build
	static	CString				m_sHelpRoad;	// help when building a road
	static	CString				m_sHelpCantBuild[9];
	static	CString				m_sHelpRMB;		// help when the RMB button is down
	static	CString				m_sHelpOkFarm;
	static	CString				m_sHelpBadFarm;
	static	CString				m_sHelpNoFarm;
	static	CString				m_sHelpOkMine;
	static	CString				m_sHelpBadMine;
	static	CString				m_sHelpNoMine;

	static	HCURSOR				m_hCurReg;		// standard cursor
	static	HCURSOR				m_hCurGoto[4];// cursor to send vehicle
	static	HCURSOR				m_hCurWait;		// waiting for ownership of tiles
	static	HCURSOR				m_hCurStart;	// waiting for path to be found
	static	HCURSOR				m_hCurRoadBgn[4]; // haven't started the road yet
	static	HCURSOR				m_hCurRoadSet[4];// laying out the road
	static	HCURSOR				m_hCurTarget[4];	// a targeting cursor
	static	HCURSOR				m_hCurSelect[4];	// select a unit
	static	HCURSOR				m_hCurRoute;	// set a route
	static	HCURSOR				m_hCurMove[9];		// move the screen
	static	HCURSOR				m_hCurLoad[4];		// load unit in vehicle
	static	HCURSOR				m_hCurUnload[4];	// unload unit in vehicle
	static	HCURSOR				m_hCurRepair;	// repair vehicle/bldg
	static	HCURSOR				m_hCurNoRepair;// not on a bldg we can repair

	static	CFont					m_fntInfo;		// font for info window
	static	CBrush				m_brInfo;			// info window
};


// tracks area maps
class CAreaList : public CList <CWndArea *, CWndArea *>
{
public:
	CAreaList ();

	void					EnableWindows ( BOOL bEnable );
	void					AddWindow (CWndArea *);
	void					DestroyWindow (CWndArea *);
	void					DestroyAllWindows ();
	void					SelectOff ();
	void					XilDiscovered ();
	int						GetNumWindows ();
	CWndArea *		BringToTop ();
	CWndArea *		GetTop ();
	void					MoveSizeToNew ( int xOld, int yOld );

	void					UnitDying (CUnit * pUnit);
	void					MaterialChange (CUnit const * pUnit) const;

	void					InvalidateStatus (CUnit const * pUnit) const;

	void					SetLastAttack ( CHexCoord const & _hex );
	CHexCoord &		GetLastAttack ( ) { return m_hexLastCombat; }
	BOOL					HaveAttack () const { return m_bLcSet; }

protected:
	CHexCoord			m_hexLastCombat;
	BOOL					m_bLcSet;
};


/////////////////////////////////////////////////////////////////////////////
// CWndInfo window

class CWndInfo : public CWndBase
{
// Construction
public:
	CWndInfo();

	CWndInfo *	Create (CPoint & pt, CUnit * pUnit, CWndArea * pPar);

	CUnit * GetUnit () { return (m_pUnit); }

// Attributes
public:

// Operations
public:
	void		Refigure ();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWndInfo)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWndInfo();

	// Generated message map functions
protected:
	int		FigureHt ();

	//{{AFX_MSG(CWndInfo)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CUnit *		m_pUnit;

	CDIB *		m_pdib;		// scratch DIB for building up image in paint
};

/////////////////////////////////////////////////////////////////////////////


extern CAreaList theAreaList;


#endif
