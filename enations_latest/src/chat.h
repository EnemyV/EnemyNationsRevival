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
// CDlgChatAll dialog

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


#endif
