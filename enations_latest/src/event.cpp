//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// event.cpp
//

#include "stdafx.h"
#include "event.h"
#include "lastplnt.h"
#include "player.h"
#include "sfx.h"

#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

static UINT hTimer = NULL;

COLORREF EventclrOk;
COLORREF EventclrWarn;
COLORREF EventclrBad;


const int aiRes [] = {
		IDS_EVENT_CONST_LOC,
		IDS_EVENT_CONST_CANT,
		IDS_EVENT_CONST_START,
		IDS_EVENT_HALTED,
		IDS_EVENT_CONST_DONE,
		IDS_EVENT_REPAIR_DONE,
		0,

		IDS_EVENT_ROAD_START,
		IDS_EVENT_ROAD_HALTED,
		IDS_EVENT_ROAD_DONE,

		IDS_EVENT_HALTED,
		IDS_EVENT_CONST_DONE,

		IDS_EVENT_MANF_HALT,

		IDS_EVENT_GOTO_CANT,
		0,
		IDS_EVENT_BLDG_UNDER,
		IDS_EVENT_BLDG_HURTING,
		IDS_EVENT_BLDG_DYING,

		IDS_EVENT_BLDG_UNDER,
		IDS_EVENT_ATK_TARGET,
		IDS_EVENT_ATK_DESTROYED,

		IDS_EVENT_HAVE_MAIL,
		IDS_EVENT_HAVE_CALL,
		IDS_EVENT_MINE_EMPTY,

		IDS_EVENT_POP_LOW,
		IDS_EVENT_GAS_LOW,
		IDS_EVENT_FOOD_LOW,
		IDS_EVENT_POWER_LOW,
		IDS_EVENT_GAS_OUT,
		IDS_EVENT_FOOD_OUT,

		IDS_EVENT_DEAD,
		IDS_EVENT_LEFT,
		IDS_EVENT_JOINED,

		IDS_EVENT_LOW_HOUSING,
		IDS_EVENT_LOW_OFFICE,
		IDS_EVENT_NEW_CRANE,
		IDS_EVENT_NEW_TRUCK,
		IDS_EVENT_NEW_RELATIONS,

		IDS_EVENT_HPR_TRUCKS,			// no trucks available
		IDS_EVENT_HPR_NOPICKUP,			// unable to pick up materials
		IDS_EVENT_HPR_NOACCESS,			// unable to reach materials
		IDS_EVENT_HPR_SEAPORTS,			// not enough seaports
		IDS_EVENT_HPR_CARGOSHIP,		// need a cargo ship
		IDS_EVENT_HPR_NOREACH,			// unable to reach seaport

		IDS_EVENT_CANT_BRIDGE,			// can't build bridges yet
		IDS_EVENT_HAVE_RADAR,				// now have radar

		IDS_EVENT_ROCKET_CANT_LAND,

		0, 0, 0 };	// these are in case I didn't do enough

const int aiSfx [] = {
		VOICES::tem_where_build,
		VOICES::tem_cant_build,
		VOICES::tem_const_start,
		0,
		VOICES::tem_bldg_comp,
		VOICES::tem_bldg_rep,
		VOICES::tem_under_atk,

		0,
		0,
		VOICES::tem_road_comp,

		0,
		VOICES::fac_veh_comp,

		VOICES::sys_out_mat,

		VOICES::com_stuck,
		VOICES::bld_enemy_close,
		VOICES::sys_under_atk,
		VOICES::bld_heavy_damage,
		VOICES::bld_almost_dead,
		VOICES::tem_under_atk,
		VOICES::com_obj_atk,
		VOICES::com_obj_des,

		VOICES::rec_mail,
		VOICES::rec_call,
		VOICES::sys_mine_out,

		VOICES::sys_low_pop,
		VOICES::sys_low_gas,
		VOICES::sys_low_food,
		VOICES::sys_low_power,
		VOICES::sys_out_gas,
		VOICES::sys_out_food,

		0, 0, 0,

		VOICES::sys_low_apt,
		VOICES::sys_low_ofc,
		0, 0,
		VOICES::sys_declare_war,

		0, 0,0,0,0,0,		// hp router

		VOICES::sys_cant_bridge,
		VOICES::sys_have_radar,

		VOICES::rocket_cant_land,

		0, 0, 0 };	// these are in case I didn't do enough

static int iLastEvent = -1;

void CGame::Event (int ID, int iTyp, CUnit *pUnit)
{

	if (pUnit == NULL)
		{
		TRAP ();
		return;
		}

	if ( ! pUnit->GetOwner()->IsMe () )
		return;

	char const * psText;
	switch (ID)
	  {
		case EVENT_CONST_CANT :
			psText = theStructures.GetData (((CVehicle *)pUnit)->GetBldgType ())->GetDesc ();
			break;

		case EVENT_BUILD_HALTED :
		case EVENT_BUILD_DONE :
			psText = theTransports.GetData (((CVehicleBuilding*)pUnit)->GetBldUnt()->GetVehType ())->GetDesc ();
			break;

		case EVENT_ATK_TARGET :
		case EVENT_ATK_DESTROYED : {
			CUnit * pTarget = pUnit->GetTarget ();
			if ((pTarget != NULL) && (pTarget->GetUnitType () == CUnit::building))
				psText = theStructures.GetData (((CBuilding *)pTarget)->GetData()->GetType ())->GetDesc ();
			else
				psText = NULL;
			break; }
	
		default:
			psText = pUnit->GetData()->GetDesc ();
			break;
	  }

	_Event ( ID, iTyp, psText, pUnit->GetVoice () );
}

void CGame::Event (int ID, int iTyp, CPlayer * pPlayer)
{

	if (pPlayer == NULL)
		{
		TRAP ();
		return;
		}

	if ( ! pPlayer->IsMe () )
		return;

	_Event ( ID, iTyp, pPlayer->GetName (), 0 );
}

void CGame::Event (int ID, int iTyp, int iVal)
{

	char const * psText;
	switch (ID)
	  {
		case EVENT_HPR_NOACCESS :
		case EVENT_HPR_NOPICKUP : {
			int iTyp = __minmax ( 0, CMaterialTypes::GetNumTypes (), iVal );
			psText = CMaterialTypes::GetDesc ( iTyp );
			break; }
		case EVENT_CONST_CANT :
			psText = theStructures.GetData (iVal)->GetDesc ();
			break;
		default:
			psText = NULL;
			break;
		}

	_Event ( ID, iTyp, psText, 0 );
}

void CGame::Event (int ID, int iTyp)
{

	_Event ( ID, iTyp, NULL, 0 );
}

// how much time (seconds) must pass before we say this again
const DWORD dwTimeBetween [] = {
		0 ,
		0 ,
		0 ,
		0 ,
		0 ,
		0 ,
		30 ,

		60,
		0,
		30,

		60,
		20,

		60,

		60,
		180,
		120,
		60,
		30,
		120,
		60,
		20,

		0,
		0,
		0,

		60,
		60,
		60,
		60,
		60,
		60,

		0,
		0,
		0,

		60,
		60,
		10,
		10,
		0,

		60,
		60,
		60,
		10,
		60,
		10,

		30,
		0,
		
		0,
		
		0, 0, 0 };	// these are in case I didn't do enough

DWORD dwLastTime [NUM_EVENTS];

void CGame::_Event (int ID, int iTyp, char const * psText, int iVoice)
{
	ASSERT ((sizeof (aiRes) / sizeof (int)) == NUM_EVENTS);
	ASSERT ((sizeof (aiSfx) / sizeof (int)) == NUM_EVENTS);

	if ((ID < 0) || (NUM_EVENTS <= ID))
		{
		ASSERT (FALSE);
		return;
		}

	// if no HP then no events
	if ( ! theGame.HaveHP () )
		return;

	// not during cut scenes
	if ( !theGame.ShouldOperate() )
		return;

	// don't repeat ourselves too often
	if ( theGame.GettimeGetTime () - dwLastTime [ID] < dwTimeBetween [ID] * 1000 )
		return;
	dwLastTime [ID] = theGame.GettimeGetTime ();

	// if its clear it - do it now and leave
	if (iTyp == EVENT_OFF)
		{
		if (ID != iLastEvent)
			return;
		if (hTimer != NULL)
			{
			theApp.m_wndBar.KillTimer (hTimer);
			hTimer = NULL;
			}
		theApp.m_wndBar.SetStatusText (0, "");
		return; 
		}

	TRAP (ID == EVENT_ROAD_START);
	TRAP (ID == EVENT_VEH_UNDER_ATK);
	TRAP (ID == EVENT_PLAYER_LEFT);
	TRAP (ID == EVENT_PLAYER_JOINED);
	TRAP (ID == EVENT_HPR_SEAPORTS);
	TRAP (ID == EVENT_HPR_CARGOSHIP);
	TRAP (ID == EVENT_HPR_NOREACH);

	int iRes = aiRes [ID];
	int iSfx = aiSfx [ID];

	CString sMsg;
	sMsg.LoadString (iRes);
	if (psText != NULL)
		csPrintf (&sMsg, psText);

	CStatInst::IMPORTANCE iImp;
	int iShow;
	switch (iTyp)
	  {
		case EVENT_NOTIFY :
			iImp = CStatInst::status;
			iShow = 10;
			break;
		case EVENT_WARN :
			iImp = CStatInst::warn;
			iShow = 20;
			break;
		case EVENT_BAD :
			iImp = CStatInst::critical;
			iShow = 40;
			break;

		default:
			return;
	  }

	iLastEvent = ID;
	theApp.m_wndBar.SetStatusText (0, sMsg, iImp);

	// we kill the message after iShow seconds
	if (hTimer != NULL)
		theApp.m_wndBar.KillTimer (hTimer);
	hTimer = theApp.m_wndBar.SetTimer (99, iShow * 1000, NULL);

	if (iSfx > 0)
		{
		// keep at least 3 seconds apart
		static DWORD dwLastVoice = 0;
		if ( theGame.GettimeGetTime () - dwLastVoice > 3 * 1000 )
			{
			dwLastVoice = theGame.GettimeGetTime ();
			theMusicPlayer.PlayForegroundSound (VOICES::GetID (iSfx, iVoice), SFXPRIORITY::voice_pri );
			}
		}
}

static int aiMSfx [] = {
		VOICES::com_sel_first,
		VOICES::tem_sel_first,
		VOICES::fac_sel_first,
		VOICES::com_ok_first,
		0,
		0,
		VOICES::sci_first
};
static int aiNumMSfx [] = {
		VOICES::num_com_sel - 1,
		VOICES::num_tem_sel - 1,
		VOICES::num_fac_sel - 1,
		VOICES::num_com_ok - 1,
		0,
		0,
		VOICES::num_sci - 1
};

void CGame::MulEvent (int ID, CUnit * pUnit)
{

	ASSERT ((0 <= ID) && (ID < NUM_MEVENTS));
	ASSERT ((sizeof (aiMSfx) / sizeof (int)) == NUM_MEVENTS);

	int iSfx = aiMSfx [ID] + RandNum (aiNumMSfx [ID]);
	if (iSfx > 0)
		{
		int iVoice;
		if (pUnit == NULL)
			iVoice = RandNum (1);
		else
			iVoice = pUnit->GetVoice ();
		theMusicPlayer.PlayForegroundSound (VOICES::GetID (iSfx, iVoice), SFXPRIORITY::voice_pri );
		}
}


void CWndBar::OnTimer(UINT nIDEvent) 
{

	// kill it	
	theApp.m_wndBar.KillTimer (hTimer);
	hTimer = NULL;

	// clear the status bar
	theApp.m_wndBar.SetStatusText (0, "");

	CWndAnim::OnTimer(nIDEvent);
}
