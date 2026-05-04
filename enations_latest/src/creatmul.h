//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __CREATMUL_H__
#define __CREATMUL_H__

#include "new_game.h"

// creatmul.h : header file
//

class CCreateMultiBase;
class CCreateMulti;
class CCreateLoadMulti;
class CPlayer;


/////////////////////////////////////////////////////////////////////////////
// CCreateMulti - holds everything for creating game.
// MFC pre-game dialogs CDlgCreateMulti, CDlgCreatePublish, CDlgLoadMulti
// removed (Phase 2d) — replaced by SDL2CreateNetDialog driven by
// SDL2_RunCreateNetworkFlow() / SDL2_RunLoadNetworkFlow().

class CCreateMultiBase : public CMultiBase
{
public:
		CCreateMultiBase (int iTyp);

		void  ClosePick ();
		void  CloseAll ();

		void RemovePlayer (CPlayer * pPlr) { m_wndPlyrList.RemovePlayer (pPlr); }
		void UpdatePlyrStatus (CPlayer * pPlyr, int iStatus)
									{ m_wndPlyrList.UpdatePlyrStatus (pPlyr, iStatus); }
		void AddPlayer (CPlayer * pPlyr)
									{ m_wndPlyrList.AddPlayer (pPlyr); }
		void  UpdateBtns () { m_wndPlyrList.UpdateBtns (); }

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

class CCreateMulti : public CCreateMultiBase, public CCreateNewBase
{
public:
		CCreateMulti () : CCreateMultiBase (CCreateBase::create_net) {}
		~CCreateMulti () { CloseAll (); }

		CCreateNewBase *	GetNew () { return (this); }

		void	Init ();
		void	ClosePick ();
		void	CloseAll ();

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


class CCreateLoadMulti : public CCreateMultiBase, public CCreateLoadBase
{
public:
		CCreateLoadMulti () : CCreateMultiBase (CCreateBase::load_multi) {}
		~CCreateLoadMulti () { CloseAll (); }

		// CDlgPickPlayer removed (Phase 2d) — SDL2PickPlayerDialog does its own button state.
		void  UpdateBtns () {}
		CCreateLoadBase *	GetLoad () { return (this); }

		void	Init ();
		void  ClosePick ();
		void  CloseAll ();

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


#endif
