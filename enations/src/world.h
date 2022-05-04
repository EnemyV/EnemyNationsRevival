//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __WORLD_H__
#define __WORLD_H__

#include <wndbase.h>

#include "bmbutton.h"
#include "terrain.h"



/////////////////////////////////////////////////////////////////////////////
// CWndWorld window

const int NUM_WORLD_BTNS = 4;

class CWndWorld : public CWndAnim
{
// Construction
public:
	CWndWorld ();
	~CWndWorld () { Close (); }

	void		Create ( BOOL bStart = FALSE );
	void		Close ();
	void		PaletteChange ();

	void		NewDir () { m_bNewDir = TRUE; }							// new direction (called by OnSize)
	void		NewMode () { m_bNewMode = TRUE; }						// button pressed or building built/destroyed
	void		NewLocation () { m_bNewLocation = TRUE; }		// area map moved
	void		NewAreaMap (CWndArea * pWnd);								// different area map on top (calls NewLocation)

	void		SetBldgHit () { if (! m_bIsRadar) return; m_bBldgHit = TRUE; InvalidateWindow (my_units); }
	void		SetVehHit () { if (! m_bIsRadar) return; InvalidateWindow (my_units); }

	void		VisibleOff () { if (m_iMode & visible) OnVisible (); }

	void		InvalidateWindow (RECT *pRect = NULL) { m_bUpdate = TRUE; }
	void		InvalidateWindow (int iMode);								// mark it to actually rerender on ReRender
				// note: below value << 4 set means it's disabled
				enum { resources = 0x01, visible = 0x02, my_units = 0x04, other_units = 0x08 };
	int			GetButtonState (int iBtn) const;
	void		SetButtonState (int iBtn, int iState);
				enum { pos_res = 0, pos_vis = 1, pos_mine = 2, pos_units = 3 };
				enum { up = 0, down = 1, disabled = 2 };
	void		CommandCenterChange ();

	void		ReRender ();										// mainloop - re-render the screen
	void		Draw () { m_dibwnd.Update (); }	// draw the rendered screen

protected:
	// Generated message map functions
	//{{AFX_MSG(CWndWorld)
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnUnits ();
	afx_msg void OnRes ();
	afx_msg void OnMine ();
	afx_msg void OnVisible ();
	afx_msg void OnCloseWin ();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	static const int aID [NUM_WORLD_BTNS];
	UINT		GetID (int iNum) {return (aID[iNum]);}

	void		_OnSize ();											// also for changing world/radar
	void		_NewDir ();											// new direction (called by OnSize)
	void		_NewMode ();											// button pressed or building built/destroyed
	void		_NewLocation ();									// area map moved

	void		SetMouseState ();
	int			ButtonOn (CPoint const & pt) const;
	void		ApplyColors (CDIB const *pDib);
	void		CaptureMouse () { ASSERT (! m_bCapMouse); SetCapture (); m_bCapMouse = TRUE; }
	void		ReleaseMouse () { ASSERT (m_bCapMouse); ReleaseCapture (); m_bCapMouse = FALSE; }

	CWndArea *		m_pWndArea;		// area map we represent

	// out WinG window
	CDIBWnd		m_dibwnd;					// window we blt into

	// we have several bitmaps to make updates FAST
	CDIB *				m_pdibGround0;				// this is the terrain starting at 0,0. 
																			//   It is rebuilt if the window size changes or the direction
	CDIB *				m_pdibBase;						// this is the terrain starting at the map UL corner
																			//   This includes resources (if on) and buildings
																			//   It is rebuilt if the area map position changes (from Ground0)
																			//   It is rebuilt when buildings change or buttons are pushed
 	CDIB *				m_pdibRadar;					// this is the radar that is placed over the map (has magenta) (scaled)
 	CDIB *				m_pdibButtons;				// this is the 4 buttons (scaled)

	int *					m_piRadarEdges;				// offset to left, right of radar opening

	int						m_xBtnUL;							// button location
	int						m_yBtnUL;
	int						m_xBtnLR;
	int						m_yBtnLR;

	int						m_cx;									// size of bitmap & window
	int						m_cy;
	int						m_cxLine;							// cx extended up to dib pitch
	long					m_lSizeBytes;
	int						m_iMode;

	int						m_yAdd;								// for map::window scaling
	int						m_yRem;
	int						m_xAdd;
	int						m_xRem;
	int						m_xDib;								// for copying off-screen buffer to window
	int						m_yDib;
	int						m_xDibBytes;
	int						m_iLenBytes;					// first half of line to copy

	int						m_iResOn;							// for blinking resources
	int						m_iFrameOn;

	BOOL					m_bLBtnDown;
	BOOL					m_bRBtnDown;
	BOOL					m_bCapMouse;

	BOOL					m_bNewDir;				// TRUE if need to render for new dir
	BOOL					m_bNewMode;				// TRUE if need to render for new mode
	BOOL					m_bNewLocation;		// TRUE if need to render for new location
	BOOL					m_bUpdate;				// TRUE if need to update
	BOOL					m_bBldgHit;				// TRUE if need to render buildings again
	BOOL					m_bIsRadar;				// TRUE if it's a radar unit

	CPoint				m_ptRMB;

	CString				m_sHelpRMB;
	CString				m_sHelp;
	CString				m_sHelpBtn[4];
	CString				m_sHelpBtnDis[4];
	CString				m_sHelpFace;
	CString				m_sDir [4];

	HCURSOR				m_hCurArrow;	// standard cursor
	HCURSOR				m_hCurCross;	// cross hairs
	HCURSOR				m_hCurGoto;		// cursor to send vehicle
	HCURSOR				m_hCurTarget;	// a targeting cursor
	HCURSOR				m_hCurSelect;	// select a unit
	HCURSOR				m_hCurMove;		// move the screen

	// colors for painting the window
	static DWORD		m_clrTerrain [CHex::num_types];		// same order as CHex m_bType
	static DWORD		m_clrResources [4];
	static DWORD		m_clrResHigh [4];
	static DWORD		m_clrLocation;
	static DWORD		m_clrHit;

	static COLORREF		m_rgbTerrain [CHex::num_types];		// same order as CHex m_bType
	static COLORREF		m_rgbResources [4];
	static COLORREF		m_rgbResHigh [4];
	static COLORREF		m_rgbLocation;
	static COLORREF		m_rgbHit;
};


#endif
