//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// creatmul.cpp : implementation file
//
// MFC pre-game dialogs CDlgCreateMulti, CDlgCreatePublish, CDlgLoadMulti
// removed (Phase 2d) — replaced by SDL2 dialogs in SDL2Dialogs.cpp.
// This file now holds only the CCreateMulti / CCreateLoadMulti / CCreateMultiBase
// orchestrator state classes used by both the legacy MFC entry points
// (CDlgMain::OnMain*) and the live SDL2 flow helpers.

#include "stdafx.h"
#include "lastplnt.h"
#include "player.h"
#include "help.h"
#include "error.h"

#include "creatmul.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


CCreateMultiBase::CCreateMultiBase (int iTyp) : CMultiBase (iTyp)
{
	// CDlgPlayerList removed (Phase 2d) — server-flag tracking moved into the
	// SDL2 multiplayer flow's own dialog state.
}

void CMultiBase::CreateWndChat ()
{

	CDlgChatAll* pChat = theApp.GetDlgChat();
	if (pChat)
		pChat->EnableWindow (theGame.GetMyNetNum () != VP_LOCALMACHINE);
}

void CCreateMultiBase::ClosePick ()
{
	ASSERT_VALID (this);
	CMultiBase::ClosePick ();
}

void CCreateMultiBase::CloseAll ()
{
	ASSERT_VALID (this);

	ClosePick ();

	CMultiBase::CloseAll ();
}

#ifdef _DEBUG
void CCreateMultiBase::AssertValid() const
{
	CMultiBase::AssertValid ();
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CCreateMulti

void CCreateMulti::Init ()
{
	// CDlgCreateMulti removed (Phase 2d). The SDL2 flow constructs CCreateMulti
	// directly in SDL2_RunCreateNetworkFlow(); this MFC entry point only ran
	// from CDlgMain::OnMainCreate (dead MFC main-menu fallback).
	ASSERT_VALID (this);
	theApp.Log ( "Create multi-player game (MFC entry — no-op)" );
}

void CCreateMulti::ClosePick ()
{
	ASSERT_VALID (this);
	CCreateMultiBase::ClosePick ();
	CCreateNewBase::ClosePick ();
}

void CCreateMulti::CloseAll ()
{
	ASSERT_VALID (this);

	ClosePick ();

	CCreateMultiBase::CloseAll ();
	CCreateNewBase::CloseAll ();
}

#ifdef _DEBUG
void CCreateMulti::AssertValid() const
{
	CCreateBase::AssertValid ();
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CCreateLoadMulti

void CCreateLoadMulti::Init ()
{
	// CDlgLoadMulti removed (Phase 2d). The SDL2 flow constructs CCreateLoadMulti
	// directly in SDL2_RunLoadNetworkFlow(); this MFC entry point only ran from
	// CDlgMain::OnMainLoadMul (dead MFC main-menu fallback).
	ASSERT_VALID (this);
	theApp.Log ( "Load multi-player game (MFC entry — no-op)" );
}

void CCreateLoadMulti::ClosePick ()
{
	ASSERT_VALID (this);

	CCreateMultiBase::ClosePick ();
	CCreateLoadBase::ClosePick ();
}

void CCreateLoadMulti::CloseAll ()
{
	ASSERT_VALID (this);

	ClosePick ();

	CCreateMultiBase::CloseAll ();
	CCreateLoadBase::CloseAll ();
}

#ifdef _DEBUG
void CCreateLoadMulti::AssertValid() const
{
	CCreateBase::AssertValid ();
}
#endif
