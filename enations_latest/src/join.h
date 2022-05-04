//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __JOIN_H__
#define __JOIN_H__

// join.h : header file
//
#include "new_game.h"
#include "creatmul.h"

class CJoinMulti;


class CGameInfo : public CObject
{
public:
		CGameInfo () {}

		int					m_iNumOpponents;
		int					m_iAIlevel;
		int					m_iWorldSize;
		int					m_iPos;
		int					m_iNumPlayers;
		char				m_cFlags;
		CString			m_sName;
		CString			m_sDesc;
		VPSESSIONID	m_ID;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CDlgJoinPublish dialog

class CDlgJoinPublish : public CDialog
{
// Construction
public:
	CDlgJoinPublish(CWnd* pParent = NULL);   // standard constructor
	~CDlgJoinPublish ();
	void Create (CJoinMulti * pJm, UINT id, CWnd *pPar = NULL);

	CJoinMulti *		m_pJm;

// Dialog Data
	//{{AFX_DATA(CDlgJoinPublish)
	enum { IDD = IDD_JOIN_PUBLISH };
	CWndOD< CButton >	m_btnAdv;
	CWndOD< CButton >	m_btnUnPublish;
	CEdit	m_StrName;
	CWndOD< CButton >	m_BtnOk;
	CString	m_sName;
	int		m_NetRadio;
	CString	m_strPw;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	void	SetControls ();

	// Generated message map functions
	//{{AFX_MSG(CDlgJoinPublish)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	afx_msg void OnChangeCreateName();
	afx_msg void OnJoinUnpublish();
	afx_msg void OnJoinAdv();
	afx_msg void OnRadioChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	int						m_PrevConn;
	int						m_idOK;
};


/////////////////////////////////////////////////////////////////////////////
// CDlgJoinGame dialog

class CDlgJoinGame : public CDialog
{
// Construction
public:
	CDlgJoinGame (CWnd* pParent = NULL);	// standard constructor
	void Create (CJoinMulti * pJm, UINT id, CWnd *pPar = NULL);

	void	SetControls ();
	void	OnSessionEnum (LPCVPSESSIONINFO pSi);
	void	OnSessionClose (LPCVPSESSIONINFO pSi);

	CJoinMulti *		m_pJm;
	int							m_iLastInd;
	BOOL						m_bAddrShowing;
	int							m_iWid;
	int							m_iHtAddr;
	int							m_iHtNoAddr;
	BOOL						m_bTimer;

// Dialog Data
	//{{AFX_DATA(CDlgJoinGame)
	enum { IDD = IDD_JOIN_GAME };
	CWndOD< CButton >	m_btnPrev;
	CWndOD< CButton >	m_btnOK;
	CListBox	m_LstGames;
	CString	m_strAIlevel;
	CString	m_strNumAI;
	CString	m_strSize;
	CString	m_strDesc;
	CString	m_sPos;
	CString	m_strType;
	CString	m_sAddr;
	CString	m_sIsAddr;
	CString	m_sVpVer;
	CString	m_sVer;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CDlgJoinGame)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnJoinPrev ();
	afx_msg void OnHelp();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSelchangeJoinGames();
	afx_msg void OnDestroy();
	afx_msg void OnShowAddr();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CJoinPlayers dialog

class CDlgJoinPlayers : public CDialog
{
// Construction
public:
	CDlgJoinPlayers(CWnd* pParent = NULL);   // standard constructor
	void Create (CJoinMulti * pJm, UINT id, CWnd *pPar = NULL);

	void	AddPlayer (CPlayer * pPlr);
	void	RemovePlayer (CPlayer *pPlr);
	void	SetNumPlayers ();
	void 	UpdatePlyrStatus (CPlayer * pPlyr, int iStatus);


// Dialog Data
	//{{AFX_DATA(CDlgJoinPlayers)
	enum { IDD = IDD_JOIN_PLAYERS };
	CWndOD< CButton >	m_btnNumPlayers;
	CListBox	m_lstPlayers;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgJoinPlayers)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CJoinMulti *		m_pJm;

	// Generated message map functions
	//{{AFX_MSG(CDlgJoinPlayers)
	virtual BOOL OnInitDialog();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CJoin* - holds everything for creating game

class CJoinMulti : public CMultiBase, public CCreateNewBase, public CCreateLoadBase
{
public:
		CJoinMulti () : CMultiBase (CCreateBase::join_net) {}
		~CJoinMulti () { CloseAll (); }

		void	Init ();
		void	ClosePick ();
		void	CloseAll ();
		void RemovePlayer (CPlayer * pPlr) { m_wndPlyrList.RemovePlayer (pPlr); }

		void	GameLoaded ( void * pBuf, int iLen );

		CCreateNewBase *	GetNew ();
		CCreateLoadBase *	GetLoad ();
		void  UpdateBtns () { if (m_dlgPickPlayer.m_hWnd != NULL) m_dlgPickPlayer.UpdateBtns (); }

		void	MakeLoad () { m_iTyp = CCreateBase::load_join; }

		void 	UpdatePlyrStatus (CPlayer * pPlyr, int iStatus)
									{ if ( m_wndPlyrList.m_hWnd != NULL) m_wndPlyrList.UpdatePlyrStatus (pPlyr, iStatus); }
		void	AddPlayer (CPlayer * pPlyr)
									{ m_wndPlyrList.AddPlayer (pPlyr); }
		void	OnSessionEnum (LPCVPSESSIONINFO pSi)
									{ m_dlgJoinGame.OnSessionEnum (pSi); }
		void	OnSessionClose (LPCVPSESSIONINFO pSi)
									{ m_dlgJoinGame.OnSessionClose (pSi); }

		CDlgJoinPublish	m_dlgJoinPublish;
		CDlgJoinGame		m_dlgJoinGame;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


#endif
