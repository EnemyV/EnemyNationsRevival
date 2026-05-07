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
		std::string	m_sName;
		std::string	m_sDesc;
		VPSESSIONID	m_ID;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CJoin* - holds everything for joining a multiplayer game.
// MFC dialogs CDlgJoinPublish, CDlgJoinGame, CDlgJoinPlayers removed
// (Phase 2d) — replaced by SDL2JoinNetDialog driven by SDL2_RunJoinNetworkFlow().

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
		// CDlgPickPlayer removed (Phase 2d) — SDL2PickPlayerDialog does its own button state.
		void  UpdateBtns () {}

		void	MakeLoad () { m_iTyp = CCreateBase::load_join; }

		void 	UpdatePlyrStatus (CPlayer * pPlyr, int iStatus)
									{ m_wndPlyrList.UpdatePlyrStatus (pPlyr, iStatus); }
		void	AddPlayer (CPlayer * pPlyr)
									{ m_wndPlyrList.AddPlayer (pPlyr); }
		// MFC CDlgJoinGame::OnSessionEnum/Close paths removed; SDL2 join flow
		// performs session enumeration directly via the netapi callbacks.
		void	OnSessionEnum (LPCVPSESSIONINFO) {}
		void	OnSessionClose (LPCVPSESSIONINFO) {}

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


#endif
