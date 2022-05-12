//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __CREDITS_H__
#define __CREDITS_H__

#include <wndbase.h>
#include <subclass.h>
#include "resource.h"

// credits.h : header file
//


class CCreditLine
{
public:
		CCreditLine () {}
		~CCreditLine () {}

	int				m_iAlign;		// 0 == left, 1 == center, 2 == right
	int				m_iSize;		// m_font[]
	int				m_iRtn;			// TRUE -> CR at end of line
	CString		m_sText;		// guess
};


/////////////////////////////////////////////////////////////////////////////
// CWndCredits

class CWndCredits : public CWndBase
{
// Construction
public:
	CWndCredits ();

	void		Create ();

// Attributes
public:

// Operations
public:

// Implementation
public:
	virtual ~CWndCredits ();

protected:

	// Generated message map functions
	//{{AFX_MSG(CWndCredits)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	int			m_iSpeed;
	DWORD		m_dwLastTime;
	int			m_iLineOn;
	int			m_iOffOn;
	int			m_iTopHt;
	CRect		m_rText;
	CFont		m_font[3];
	CArray <CCreditLine, CCreditLine>		m_arrLines;
};


#endif

