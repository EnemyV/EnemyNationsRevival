//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __NEW_GAME_H__
#define __NEW_GAME_H__

#include "resource.h"
#include "netapi.h"
#include "netcmd.h"
#include "racedata.h"
#include "pickrace.h"
#include "chat.h"

class CDlgPickRace;
class CDlgPickPlayer;
class CCreateNewBase;
class CCreateLoadBase;
class CCreateBase;
class SDL2CreateStatus;


const int NUM_PROTOCOLS = 7;
const int aPr[NUM_PROTOCOLS] = {
						VPT_TCP,
						VPT_IPX,
						VPT_NETBIOS,
						VPT_DP,
						VPT_MODEM,
						VPT_TAPI,
						VPT_COMM };

// CDlgCreateStatus removed (replaced by SDL2CreateStatus)
// Forward decl so CCreateBase can hold SDL2CreateStatus*
class SDL2CreateStatus;


/////////////////////////////////////////////////////////////////////////////
// CDlgPlayerList dialog

class CDlgPlayerList : public CDialog
{
// Construction
public:
	CDlgPlayerList(CWnd* pParent = NULL);   // standard constructor
	void Create (CCreateBase * pCb, UINT id, CWnd *pPar = NULL);

	void	RemoveAll () { m_lstPlayers.ResetContent (); }
	void	AddPlayer (CPlayer * pPlr);
	void	RemovePlayer (CPlayer *pPlr);
	void	SetNumPlayers ();
	void	UpdateBtns ();
	void 	UpdatePlyrStatus (CPlayer * pPlyr, int iStatus);

	CCreateBase *		m_pCb;
	BOOL						m_bServer;		// TRUE if server (can delete, start game)
	BOOL						m_bTimer;
	BOOL						m_bAddrShowing;
	int							m_iWid;
	int							m_iHtAddr;
	int							m_iHtNoAddr;

// Dialog Data
	//{{AFX_DATA(CDlgPlayerList)
	enum { IDD = IDD_PLAYER_LIST };
	CString	m_sAddr;
	CString	m_sIsAddr;
	CString	m_sVpVer;
	CString	m_sVer;
	CWndOD< CButton >	m_btnDelete;
	CWndOD< CButton >	m_btnOk;
	CWndOD< CButton >	m_btnNumPlayers;
	CWndOD< CButton >	m_btnAddr;
	CListBox	m_lstPlayers;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgPlayerList)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgPlayerList)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSelchangeCreateList();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnCreateDeletePlayer();
	afx_msg void OnShowAddr();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CCreateBase

class CCreateBase : public CObject
{
public:
		enum { scenario,
						single,
						create_net,
						join_net,
						load_single,
						load_multi,
						load_join,
						num_types };

		CCreateBase (int iTyp);
		virtual ~CCreateBase () { CloseAll (); }
		virtual void RemovePlayer (CPlayer *) {}
		virtual void UpdatePlyrStatus (CPlayer *, int) {}
		void	ToWorld ();
		virtual void AddPlayer (CPlayer *) {}
		virtual void	OnSessionEnum (LPCVPSESSIONINFO) {}
		virtual void	OnSessionClose (LPCVPSESSIONINFO) {}

		SDL2CreateStatus * GetDlgStatus () { ASSERT (m_psdlStatus != NULL); return (m_psdlStatus); }
		void		CreateDlgStatus ();
		void		ShowDlgStatus ();
		void		HideDlgStatus ();

		virtual void	Init () {}
		virtual void  ClosePick () {}
		virtual void  CloseAll ();
		virtual void  UpdateBtns () {}

		virtual CCreateNewBase *	GetNew () { ASSERT (FALSE); return (NULL); }
		virtual CCreateLoadBase *	GetLoad () { ASSERT (FALSE); return (NULL); }

		int						m_iTyp;
		int						m_iNet;				// net type (-1 -> no net)
		VPSESSIONID		m_ID;
		int						m_iAi;				// AI intelligence
		int						m_iNumAi;			// num AI players
		int						m_iSize;			// world size
		int						m_iPos;				// initial position
		int						m_iJoinUntil;	// can join until
		int						m_iNumPlayers;				// for loading/in-process games - how many total positions there are

		void *				m_pAdvNet;		// advanced net data

		CString				m_sName;			// player name
		CString				m_sRace;			// race name
		CString				m_sPw;				// password
		CString				m_sGameName;					// for create_net
		CString				m_sGameDesc;

		// SDL2 progress dialog (renders on SDL window)
		SDL2CreateStatus * GetSDL2Status() { return m_psdlStatus; }

protected:
		SDL2CreateStatus *	m_psdlStatus = nullptr;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

class CMultiBase : public CCreateBase
{
public:
		CMultiBase (int iTyp) : CCreateBase (iTyp) {}

		virtual void  ClosePick () {}
		virtual void  CloseAll ();

		void		CreateWndChat ();
		void		CreatePlyrList (CCreateBase * pCb);

		CDlgPlayerList		m_wndPlyrList;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

class CCreateNewBase
{
public:
		CCreateNewBase () {}
		
		virtual void  ClosePick ();
		virtual void  CloseAll () { ClosePick (); }

		CDlgPickRace			m_dlgPickRace;

		CInitData					m_InitData;
};

class CCreateLoadBase
{
public:
		CCreateLoadBase () { m_bReplace = FALSE; }

		virtual void  ClosePick ();
		virtual void  CloseAll () { ClosePick (); }

		CDlgPickPlayer			m_dlgPickPlayer;
		BOOL								m_bReplace;
};


#endif
