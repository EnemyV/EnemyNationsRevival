//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __TOOLBAR_H__
#define __TOOLBAR_H__


#include "resource.h"
#include "bmbutton.h"
#include "icons.h"
#include "ui.h"


const int BAR_BTN_X_SKIP = 4;						// x skip between buttons
const int BAR_BTN_Y_START = 4;					// where buttons start on the background
const int BAR_BTN_HT = 38;							// height of button backdrop
const int BAR_TEXT_HT = 28;							// height of text backdrop
const int TOOLBAR_HT = BAR_BTN_HT + BAR_TEXT_HT;


typedef void (* FNSTATUSLINE) (void * pData, CDC * pDc, CRect const & rDraw, CDIB * pDib, CPoint const & ptOff);


/////////////////////////////////////////////////////////////////////////////
// CWndUnitStat window

class CWndStatLine : public CWndStatBar
{
// Construction
public:
					CWndStatLine ();
	void		SetStatusFunc (void (* fnStat) (void * pData, CDC * pDc, CRect const & rDraw, CDIB * pDib, CPoint const & ptOff), 
													void * pData = NULL);
	void		SetText (char const * pText, CStatInst::IMPORTANCE iImp = CStatInst::status);
	void *	GetStatusData () const { return m_pFnData; }

	// Generated message map functions
protected:

	//{{AFX_MSG(CWndUnitStat)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	FNSTATUSLINE		m_fnStatus;
	void *					m_pFnData;
};


/////////////////////////////////////////////////////////////////////////////
// CWndBar window

const int NUM_BAR_BTNS = 8;
class CWndBar : public CWndAnim
{
// Construction
public:
	CWndBar ();

	void		Create ();
	void		CheckButtons ();
	void		_GotoScience ();

	void		InvalidateStatus ( void * pData );
	void		EnableButton (int ID, BOOL bEnable);

	void		SetStatusFunc (int iLine, void (* fn) (void * pData, CDC * pDc, CRect const & rDraw, CDIB * pDib, CPoint const & ptOff),
													void * pData = NULL);
	void		SetStatusText (int iLine, const char * psText = NULL, CStatInst::IMPORTANCE iImp = CStatInst::status);
#ifdef _CHEAT
	void		SetDebugText (int iLine, const char * psText);
#endif

	void		UpdateHelp (CWnd * pWnd);
	void		UpdateGas ();
	void		UpdatePower ();
	void		UpdatePeople ();
	void		UpdateFood ();
	void		UpdateTime ();

public:
	// Generated message map functions
	//{{AFX_MSG(CWndBar)
	afx_msg void GotoArea ();
	afx_msg void GotoWorld ();
	afx_msg void GotoChat ();
	afx_msg void GotoVehicles ();
	afx_msg void GotoBuildings ();
	afx_msg void GotoRelations ();
	afx_msg void GotoScience ();
	afx_msg void GotoFile ();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg LRESULT OnButtonMouseMove (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnStatusMouseMove (WPARAM wParam, LPARAM lParam);
	afx_msg void OnDestroy();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CBmButton 		m_BmBtns [NUM_BAR_BTNS];	// buttons

	CWndStatBar		m_wndStat [4];						// 4 status bars
							enum { gas, power, people, food };
	CWndStatBar		m_wndTime;								// time clock
	CWndStatLine	m_wndText [2];						// 2 text lines

	static const int aID [NUM_BAR_BTNS];
	static const int aBtn [NUM_BAR_BTNS];
	static const int aHelp [NUM_BAR_BTNS];
	static CString m_sChat1;
	static CString m_sChat2;
	static CString m_sScience;
	static CString m_sRelations;
};


#endif
