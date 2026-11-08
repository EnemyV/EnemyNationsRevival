//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __CHAT_H__
#define __CHAT_H__


// chat.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgChatAll dialog — excluded from build (Phase 2d). SDL2ChatWindow is the
// intended replacement. The MFC declaration is kept for historical reference
// but not compiled — live callers only use CDlgChatAll* (always nullptr now).
class CDlgChatAll;

#if 0  // MFC_LEGACY_CHAT_DIALOG

class CDlgChatAll : public CDialog
{
// Construction
public:
	CDlgChatAll(CWnd* pParent = NULL);   // standard constructor

	void		NewMessage (const char *pMsg);

// Dialog Data
	//{{AFX_DATA(CDlgChatAll)
	enum { IDD = IDD_CHAT_INIT };
	CEdit	m_edtSend;
	CEdit	m_edtRcv;
	CString	m_strRcv;
	CString	m_strSend;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgChatAll)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy() { delete this; }
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgChatAll)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnReturn();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // MFC_LEGACY_CHAT_DIALOG


#endif
