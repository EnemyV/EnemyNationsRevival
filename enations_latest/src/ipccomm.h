//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __IPCCOMM_H__
#define __IPCCOMM_H__

////////////////////////////////////////////////////////////////////////////
//
//  IPCComm.h : IPC Main Window objects
//              Divide and Conquer
//               
//  Last update:    10/03/96   Before: 08/24/95
//
////////////////////////////////////////////////////////////////////////////

#include "IPCMsg.hpp"
#include "IPCChat.h"
#include "IPCRead.h"
#include "IPCSend.h"

#include "bmbutton.h"

#define NEW_BUTTONS	1
#define EN_CONTROLS 1

#if EN_CONTROLS

class CMyHeaderCtrl : public CHeaderCtrl
{
// Construction
public:
	CMyHeaderCtrl() {}

// Operations
	BOOL Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID );

// Implementation
public:
	void 	DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	// Generated message map functions
	//{{AFX_MSG(CMyHeaderCtrl)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif

class CEMailLB : public CListBox
{
	UINT m_uCharHeight;
	int m_iNameAt;
	int m_iSubjectAt;
public:
	CEMailLB( UINT uID, CWnd *pParent, CRect& rLoc );
	void LoadEmail( void );
	void SetStops( int iNameStop, int iSubjectStop );

	virtual void MeasureItem( LPMEASUREITEMSTRUCT lpMIS );
	virtual void DrawItem( LPDRAWITEMSTRUCT lpDIS );
	virtual int CompareItem( LPCOMPAREITEMSTRUCT lpCIS );
};

/////////////////////////////////////////////////////////////////////////////
// CWndComm Window

const int NUM_COMM_BTNS = 9;

class CWndComm : public CWndBase
{
	int m_iPlayer;				// id of this player
	long m_lSendWndSize;		// loword = width, hiword = height

public:
	CWndComm();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	void		Create ();
static	void		UpdateMail ();

	// email/chat/voice message handlers
	void IncomingMessage( CMsgIPC *pMsg );
	void MakeMessages( void );
	void ProcessIncomingChat( CMsgIPC *pMsg );
	void ProcessEmail( CMsgIPC *pMsg );

	// access handlers
	CWnd *ReadWndOpen( CEMsg *pMsg );
	CWnd *ChatWndOpen( CMsgIPC *pMsgIn );

	void CheckChatButton ();
	void KillAiChatWnd (CPlayer const * pPlyr);

#if NEW_BUTTONS
	void EnableButton (int ID, BOOL bEnable);
	int CreateButtons(void);
#endif

// Overrides

// Implementation
public:
	virtual ~CWndComm();

	// afx_msg void OnPaint();
	// Generated message map functions
	//{{AFX_MSG(CWndComm)
	afx_msg void OnClose();
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove( int x, int y );
	afx_msg void TipText (NMHDR * pTTT, LRESULT * result);	
	afx_msg void HdrChange (NMHDR * pTTT, LRESULT * result);	
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg	void OnLbnChange ();
	afx_msg	void OnLMBDC ();
	afx_msg	void OnRead ();
	afx_msg void OnSend ();
	afx_msg void OnReply ();
	afx_msg void OnForward ();
	afx_msg void OnDelete ();
	afx_msg void OnRefuse ();
	afx_msg void OnChat ();
	afx_msg void OnGlobalChat ();
	afx_msg void OnPlayerStatus ();
	afx_msg LONG OnEmailArrived( UINT, LONG );
	afx_msg LONG OnEmailUpdate( UINT, LONG );
	afx_msg LONG OnDestroyEmailWnd( UINT, LONG );
	afx_msg LONG OnReplyEmailWnd( UINT, LONG );
	afx_msg LONG OnForwardEmail( UINT, LONG );
	afx_msg LONG OnDeleteEmail( UINT, LONG );
	afx_msg LONG OnScrollEmail( UINT, LONG );
	afx_msg LONG OnUnHideWindow( UINT, LONG );
	afx_msg LONG OnCreateChatMsg( UINT, LONG );
	afx_msg LONG OnSendWndSize( UINT, LONG );
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnButtonMouseMove (WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// child windows
	CEMailLB		*m_plbEMail;

#if !NEW_BUTTONS
	CToolBarCtrl	m_tool;
#endif

#if EN_CONTROLS
	CMyHeaderCtrl		m_header;
	//CMyStatusBarCtrl	m_status;
#else
	CHeaderCtrl		m_header;
	//CStatusBarCtrl	m_status;
#endif

	int				m_xMin;
	int				m_yMin;

#if NEW_BUTTONS
	CBmButton 		m_BmBtns[NUM_COMM_BTNS];	// buttons
	static const int aCommID[NUM_COMM_BTNS];
	static const int aCommBtn[NUM_COMM_BTNS];
	static const int aCommHelp[NUM_COMM_BTNS];
#endif

};

#endif // __IPCCOMM_H__
