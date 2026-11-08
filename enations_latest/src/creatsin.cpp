//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// creatsin.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "creatsin.h"
#include "player.h"
#include "help.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CCreateSingle

void CCreateSingle::Init ()
{
	// CDlgCreateSingle removed (Phase 2d). The SDL2 flow constructs CCreateSingle
	// directly in SDL2_RunCreateSinglePlayerFlow(); this MFC entry point only
	// ran from CDlgMain::OnMainSingle (dead MFC main-menu fallback).
	ASSERT_VALID (this);
	theApp.Log ( "Create single-player game (MFC entry — no-op)" );
}

void CCreateSingle::ClosePick ()
{
	CCreateBase::ClosePick ();
	CCreateNewBase::ClosePick ();
}

#ifdef _DEBUG
void CCreateSingle::AssertValid() const
{
	CCreateBase::AssertValid ();
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CCreateLoadSingle

void CCreateLoadSingle::Init ()
{
	// CDlgPickPlayer removed (Phase 2d). The SDL2 flow does load + pick-player
	// via SDL2_RunLoadSinglePlayerFlow / SDL2PickPlayerDialog directly.
	// This MFC entry point only ran from CDlgMain::OnMainLoad (dead fallback).
	ASSERT_VALID (this);
	theApp.Log ( "Load single-player game (MFC entry — no-op)" );
}

void CCreateLoadSingle::ClosePick ()
{

	CCreateBase::ClosePick ();
	CCreateLoadBase::ClosePick ();
}

void CCreateLoadSingle::CloseAll ()
{

	CCreateBase::CloseAll ();
	CCreateLoadBase::CloseAll ();
}

#ifdef _DEBUG
void CCreateLoadSingle::AssertValid() const
{
	CCreateBase::AssertValid ();
}
#endif


