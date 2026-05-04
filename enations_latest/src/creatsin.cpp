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

	ASSERT_VALID (this);
	theApp.Log ( "Load single-player game" );

	theApp.DisableMain ();

	theGame.ctor ();
	theGame.SetServer (TRUE);
	theGame._SetIsNetGame ( FALSE );

	// set it as the server
	if (theGame.LoadGame (theApp.m_pMainWnd, FALSE) != IDOK)
		{
		theApp.CreateMain ();
		return;
		}

	// if it's not a scenario pick the player
	if (theGame.GetScenario () < 0)
		{
		m_dlgPickPlayer.Create (this, IDD_PICK_PLAYER, theApp.m_pMainWnd);
		return;
		}

	theGame.SetHP (TRUE);
	theGame.SetAI (TRUE);
	theGame.GetMe()->SetState (CPlayer::ready);

	// it's a scenario - we know who we are
	if (theGame.StartGame (FALSE) != IDOK)
		{
		TRAP ();
		theApp.CloseWorld ();
		theApp.CreateMain ();
		}
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

	ASSERT_VALID (&m_dlgPickPlayer);
}
#endif


