//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __IPCCHAT_H__
#define __IPCCHAT_H__

////////////////////////////////////////////////////////////////////////////
//
//  IPCChat.h : IPC Chat Window objects
//              Divide and Conquer
//               
//  Last update: 10/03/96   -   08/23/95
//
////////////////////////////////////////////////////////////////////////////

#include "IPCMsg.hpp"
#include "chatbar.h"


class CMySplitterWnd : public CSplitterWnd
{
	CString m_sLastSend;	// last sent message

public:
	CMySplitterWnd();
	virtual ~CMySplitterWnd();

// Implementation
protected:

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	afx_msg void OnChange();
	
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CChatWnd frame

class CChatWnd : public CFrameWnd
{
public:

	int m_iTo;		// player to whom we are chatting
	int m_iFrom;	// player we are

	BOOL m_bPanesOK; // flag set to indicate panes were created
	int m_iMinPaneHeight;	// minimum size of pane
	int m_iCur0;			// current size pane 0 & 1
	int m_iCur1;

	CChatBar *m_pdbChatBar;

	CString m_sLastSent;	// last contents of send edit

	DECLARE_DYNCREATE(CChatWnd)
public:
	CChatWnd();  // protected constructor used by dynamic creation

// Attributes
public:
	

// Operations
public:
	void		OnChatIn (CMsgIPC * pMsg);
	void		SetFrom (CPlayer * pPlyr);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChatWnd)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CChatWnd();

	// Generated message map functions
	//{{AFX_MSG(CChatWnd)
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnMove( int x, int y );
	afx_msg LONG OnUnHideWindow( UINT, LONG );
	afx_msg void OnSelectplayer();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CMySplitterWnd m_wndSplitter;
	//CSplitterWnd m_wndSplitter;
};


/////////////////////////////////////////////////////////////////////////////
// CChatView view

class CChatView : public CEditView
{
protected:
	CChatView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CChatView)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChatView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CChatView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CChatView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSendView view

class CSendView : public CEditView
{
	int m_iTo;			// the player we are chatting with
	int m_iFrom;		// who we are
	CWnd *m_pParent;	// parent window, but why?
	//CString m_sLastSend;	// the last chat message sent

protected:
	CSendView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CSendView)

// Attributes
public:

// Operations
public:
	void SetParent( CWnd *pParent );
	void SetPlayers( int iFrom, int iTo );
	void GetPlayers( int *piFrom, int *piTo );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSendView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CSendView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CSendView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//afx_msg void OnChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	//afx_msg void OnChar( UINT nChar, UINT nRepCnt, UINT nFlags );
};

/////////////////////////////////////////////////////////////////////////////

#endif // __IPCCHAT_H__
