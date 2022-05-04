//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __IPCSEND_H__
#define __IPCSEND_H__

////////////////////////////////////////////////////////////////////////////
//
//  IPCSend.h : IPC Send Window objects
//              Divide and Conquer
//               
//  Last update: 10/03/96  -  08/23/95
//
////////////////////////////////////////////////////////////////////////////

#include "resource.h"
#include "IPCMsg.hpp"

class CSendCombo : public CComboBox
{
	UINT m_uCharHeight;
	int m_iSelect[MAX_CC];	// BUGBUG hard limit to number of TOs

public:

	CSendCombo();

	void OnParentInit( UINT uID, CWnd *pParent );

	BOOL IsSelected( int iAt );
	void SetSelected( int iAt );

	virtual void MeasureItem( LPMEASUREITEMSTRUCT lpMIS );
	virtual void DrawItem( LPDRAWITEMSTRUCT lpDIS );
	virtual int CompareItem( LPCOMPAREITEMSTRUCT lpCIS );
};


/////////////////////////////////////////////////////////////////////////////
// CDlgCompose dialog

class CDlgCompose : public CDialog
{
	int m_iNumPlyr;			// number of players found
	int m_iPlayer;			// id of this player

	CEMsg *m_pMsg;			// if opened as reply, the message
							// to be composed is already created
	LONG m_lSendWndSize;
	UINT m_uCharHeight;

	CSendCombo *m_pcbPlayers; // pointer to combo box with players

// Construction
public:
	// standard constructor
	CDlgCompose(CWnd* pParent = NULL,
		CEMsg *pReplyMsg = NULL, LONG lSizeSendWnd = 0L );   
	~CDlgCompose();

// Dialog Data

	//{{AFX_DATA(CDlgCompose)
	enum { IDD = IDD_MAIL_COMPOSE };
	CWndOD< CButton >	m_ctrlOK;		// send
	CWndOD< CButton >	m_ctrlCANCEL;	// cancel
	CEdit	m_ctrlText;		// message
	CEdit	m_ctrlSubject;	// subject
	CString	m_sSubject;		// subject
	CString	m_sText;		// message
	int		m_sTo;			// ?
	//}}AFX_DATA


	//CComboBox	m_ctrlTo;	// players

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgCompose)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgCompose)
	virtual BOOL OnInitDialog();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnSelectMailTo();	
	afx_msg void OnMove( int x, int y );
	afx_msg LONG OnUnHideWindow( UINT, LONG );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	int		m_xMin;
	int		m_yMin;

	void CleanUp( void );
	void InitToCombo( void );
};

#endif // __IPCSEND_H__
