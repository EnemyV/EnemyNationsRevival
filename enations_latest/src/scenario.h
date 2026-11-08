//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __SCENARIO_H__
#define __SCENARIO_H__

#include "resource.h"
#include "new_game.h"

// scenario.h : header file
//

const int NUM_SCENARIOS = 12;


class CCreateScenario;

const int NUM_AI_IN_SCENARIO = 3;
const int SCENARIO_POS = 4;


/////////////////////////////////////////////////////////////////////////////
// CCreateScenario - holds everything for creating game.
// MFC pre-game CDlgScenario removed (Phase 2d) — replaced by SDL2ScenarioDialog
// which is driven by SDL2_RunCreateScenarioFlow().

class CCreateScenario : public CCreateBase, public CCreateNewBase
{
public:
		CCreateScenario () : CCreateBase (CCreateBase::scenario) {}
		~CCreateScenario () { Close (); }

		CCreateNewBase *	GetNew () { return (this); }

		void	Init ();
		void	Close ();

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


#endif
