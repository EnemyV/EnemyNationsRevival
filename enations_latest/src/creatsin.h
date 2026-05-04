//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __CREATSIN_H__
#define __CREATSIN_H__

#include "new_game.h"

// creatsin.h : header file
//

class CCreateSingle;


/////////////////////////////////////////////////////////////////////////////
// CCreateSingle - holds everything for creating game.
// MFC pre-game CDlgCreateSingle removed (Phase 2d) — replaced by
// SDL2CreateSingleDialog driven by SDL2_RunCreateSinglePlayerFlow().

class CCreateSingle : public CCreateBase, public CCreateNewBase
{
public:
		CCreateSingle () : CCreateBase (CCreateBase::single) {}
		~CCreateSingle () { CloseAll (); }

		void	Init ();
		void  ClosePick ();
		void  CloseAll () { ClosePick (); }

		CCreateNewBase *	GetNew () { return (this); }

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

class CCreateLoadSingle : public CCreateBase, CCreateLoadBase
{
public:
		CCreateLoadSingle () : CCreateBase (CCreateBase::load_single) {}
		~CCreateLoadSingle () { CloseAll (); }

		CCreateLoadBase *	GetLoad () { return (this); }
		// CDlgPickPlayer removed (Phase 2d) — SDL2PickPlayerDialog does its own button state.
		void  UpdateBtns () {}

		void	Init ();
		void  ClosePick ();
		void  CloseAll ();

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


#endif
