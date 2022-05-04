//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __IPCREAD_H__
#define __IPCREAD_H__

////////////////////////////////////////////////////////////////////////////
//
//  IPCRead.h : IPC Read Email Window objects
//              Divide and Conquer
//               
//  Last update:    10/03/96 - 08/21/95
//
////////////////////////////////////////////////////////////////////////////

#include "IPCMsg.hpp"

/////////////////////////////////////////////////////////////////////////////
// CWndMailRead window

class CWndMailRead : public CWndBase
{
	CEMsg *m_pMsg;	// message to read
	int m_iID;		// id of window (same as message id)

// Construction
public:
	CWndMailRead(CEMsg *pMsg);
	void		Create (CWnd * pPar);

// Attributes
public:

// Operations
public:
	int GetID() { return m_iID; }
	void SetNewMessage( CEMsg *pNewMsg );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWndMailRead)
	protected:
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWndMailRead();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWndMailRead)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnReply();
	afx_msg void OnForward();
	afx_msg void OnDelete();
	afx_msg void OnScroll();
	afx_msg void OnMove( int x, int y );
	afx_msg LONG OnUnHideWindow( UINT, LONG );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CToolBarCtrl		m_tool;
	CEdit				m_sub;
	CEdit				m_text;
	int					m_xMin;
	int					m_yMin;
};

#endif // __IPCREAD_H__
