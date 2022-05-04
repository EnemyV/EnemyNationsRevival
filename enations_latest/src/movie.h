//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __MOVIE_H__
#define __MOVIE_H__

#include "wndbase.h"


/////////////////////////////////////////////////////////////////////////////
// CWndMovie window

class CWndMovie : public CWnd
{
// Construction
public:
	CWndMovie();

	void		AddMovie (char const * pFile);
	void		Create ( BOOL bRepeat );

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWndMovie)
	public:
	protected:
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWndMovie();

	BOOL		m_bInDestroy;

protected:
	void		OpenMovie (char * pName);
	void		CloseMovie ();
	void		CenterMovie ();
	void		PlayMovie ();
	void		NextMovie ();
	void		Quit ();

	// Generated message map functions
	//{{AFX_MSG(CWndMovie)
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg LRESULT OnMMMsg (WPARAM wParam, LPARAM lParam);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	BOOL				m_bShowLic;
	BOOL				m_bStartMusic;
	CString			m_sDataDir;
	CString			m_sPatchDir;
	DWORD				m_mciDevID;
	HWND				m_hWndAvi;
	CList <char *, char *>	m_lstFiles;
};


#endif
