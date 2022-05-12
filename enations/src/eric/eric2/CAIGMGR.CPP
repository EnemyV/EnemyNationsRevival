////////////////////////////////////////////////////////////////////////////
//
//  CAIGMgr.cpp :  CAIGoalMgr object declaration
//                 the Last Planet - AI
//               
//  Last update:    07/22/97
//
//  Copyright 1995,1996,1997 Glacier Edge Technology - All Rights Reserved
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CAIGMgr.hpp"
#include "CAIData.hpp"
#include "CAITargt.h"

#include "logging.h"	// dave's logging system

#define new DEBUG_NEW

extern CAIGoalList *plGoalList;		// standard goal list
extern CAITaskList *plTaskList;		// standard task list

extern CException *pException;		// standard exception for yielding
extern CAIData *pGameData;			// pointer to game data interface
extern CRITICAL_SECTION cs;			// used by threads

// pre-determine economic oriented research path - not used anymore
//
/*
static int aiEconPath[CRsrchArray::num_types] =
{
	CRsrchArray::farm_1,CRsrchArray::medium_facilities,CRsrchArray::medium_vehicle,
	CRsrchArray::mine_1,CRsrchArray::const_1,CRsrchArray::manf_1,
	CRsrchArray::large_facilities,CRsrchArray::mine_2,CRsrchArray::const_2,
	CRsrchArray::manf_2,CRsrchArray::bridge,CRsrchArray::fire_control,
	CRsrchArray::manf_3,CRsrchArray::gas_turbine,CRsrchArray::nuclear,
	CRsrchArray::advanced_facilities,CRsrchArray::heavy_vehicle,CRsrchArray::const_3,
	CRsrchArray::armored_vehicle,CRsrchArray::tanks,CRsrchArray::artillery,
	CRsrchArray::balloons,CRsrchArray::gliders,CRsrchArray::fortification,
	CRsrchArray::prop_planes,CRsrchArray::jet_planes,CRsrchArray::rockets,
	CRsrchArray::atk_1,CRsrchArray::def_1,CRsrchArray::range_1,
	CRsrchArray::acc_1,CRsrchArray::spot_1,CRsrchArray::spot_2,
	CRsrchArray::range_1,CRsrchArray::atk_2,CRsrchArray::def_2,
	CRsrchArray::acc_2,CRsrchArray::range_3,CRsrchArray::atk_3,             
	CRsrchArray::def_3,CRsrchArray::acc_3,CRsrchArray::sailboats,
	CRsrchArray::motorboats,CRsrchArray::heavy_naval,CRsrchArray::spot_3,
	CRsrchArray::copper,CRsrchArray::landing_craft,CRsrchArray::cargo_handling,
	0,                   0,                 0,    0
};
*/

//
// pre-determined combat oriented research path
//
static int aiCbtPath[CRsrchArray::num_types] =
{
	CRsrchArray::fire_control,CRsrchArray::armored_vehicle,CRsrchArray::medium_vehicle,
	CRsrchArray::medium_facilities,CRsrchArray::tanks,CRsrchArray::artillery,
	CRsrchArray::large_facilities,CRsrchArray::heavy_vehicle,CRsrchArray::fortification,
	CRsrchArray::advanced_facilities,CRsrchArray::balloons,CRsrchArray::gliders,
	CRsrchArray::prop_planes,CRsrchArray::jet_planes,CRsrchArray::rockets,
	CRsrchArray::atk_1,CRsrchArray::def_1,CRsrchArray::range_1,
	CRsrchArray::acc_1,CRsrchArray::spot_1,CRsrchArray::spot_2,
	CRsrchArray::range_1,CRsrchArray::atk_2,CRsrchArray::def_2,
	CRsrchArray::acc_2,CRsrchArray::range_3,CRsrchArray::atk_3,             
	CRsrchArray::def_3,CRsrchArray::acc_3,CRsrchArray::sailboats,
	CRsrchArray::motorboats,CRsrchArray::heavy_naval,CRsrchArray::spot_3,
	CRsrchArray::bridge,CRsrchArray::farm_1,CRsrchArray::mine_1,
	CRsrchArray::manf_1,CRsrchArray::const_1,CRsrchArray::mine_2,
	CRsrchArray::manf_2,CRsrchArray::gas_turbine,CRsrchArray::const_2,
	CRsrchArray::manf_3,CRsrchArray::const_3,CRsrchArray::nuclear,
	CRsrchArray::copper,CRsrchArray::landing_craft,CRsrchArray::cargo_handling,
	0,                   0,                 0,    0
};


static int aiEricPath[CRsrchArray::num_types] =
{
	CRsrchArray::armored_vehicle,CRsrchArray::tanks,CRsrchArray::fire_control,
	CRsrchArray::medium_facilities,CRsrchArray::medium_vehicle,CRsrchArray::gas_turbine,
	CRsrchArray::artillery,CRsrchArray::large_facilities,CRsrchArray::heavy_vehicle,
	CRsrchArray::fortification,CRsrchArray::nuclear,CRsrchArray::const_1,
	CRsrchArray::advanced_facilities,CRsrchArray::copper,CRsrchArray::manf_1,
	CRsrchArray::mine_1,CRsrchArray::rockets,
	CRsrchArray::atk_1,CRsrchArray::def_1,CRsrchArray::range_1,
	CRsrchArray::acc_1,CRsrchArray::spot_1,CRsrchArray::spot_2,
	CRsrchArray::range_1,CRsrchArray::atk_2,CRsrchArray::def_2,
	CRsrchArray::acc_2,CRsrchArray::range_3,CRsrchArray::atk_3,             
	CRsrchArray::def_3,CRsrchArray::acc_3,CRsrchArray::sailboats,
	CRsrchArray::motorboats,CRsrchArray::heavy_naval,CRsrchArray::spot_3,
	CRsrchArray::bridge,CRsrchArray::farm_1,CRsrchArray::mine_2,
	CRsrchArray::manf_2,CRsrchArray::const_2,CRsrchArray::manf_3,
	CRsrchArray::const_3,CRsrchArray::landing_craft,CRsrchArray::cargo_handling,
	CRsrchArray::balloons,CRsrchArray::gliders,
	CRsrchArray::prop_planes,CRsrchArray::jet_planes,
	CRsrchArray::radio,CRsrchArray::mail,CRsrchArray::email,CRsrchArray::telephone
};


// very naval oriented
static int aiStevePath[CRsrchArray::num_types] =
{
	CRsrchArray::armored_vehicle,CRsrchArray::tanks,CRsrchArray::medium_facilities,
	CRsrchArray::copper,CRsrchArray::radio,CRsrchArray::sailboats,
	CRsrchArray::motorboats,CRsrchArray::cargo_handling,CRsrchArray::gas_turbine,
	CRsrchArray::fire_control,CRsrchArray::fortification,CRsrchArray::medium_vehicle,
	CRsrchArray::artillery,CRsrchArray::large_facilities,CRsrchArray::heavy_vehicle,
	CRsrchArray::nuclear,CRsrchArray::landing_craft,CRsrchArray::mine_1,
	CRsrchArray::advanced_facilities,CRsrchArray::rockets,CRsrchArray::heavy_naval,
	CRsrchArray::const_1,CRsrchArray::manf_1,CRsrchArray::atk_1,
	CRsrchArray::def_1,CRsrchArray::range_1,CRsrchArray::acc_1,
	CRsrchArray::spot_1,CRsrchArray::spot_2,CRsrchArray::range_1,
	CRsrchArray::atk_2,CRsrchArray::def_2,CRsrchArray::acc_2,
	CRsrchArray::range_3,CRsrchArray::atk_3,CRsrchArray::def_3,
	CRsrchArray::acc_3,CRsrchArray::spot_3,CRsrchArray::bridge,
	CRsrchArray::farm_1,CRsrchArray::mine_2,CRsrchArray::manf_2,
	CRsrchArray::const_2,CRsrchArray::manf_3,CRsrchArray::const_3,
	CRsrchArray::balloons,CRsrchArray::gliders,CRsrchArray::prop_planes,
	CRsrchArray::jet_planes,CRsrchArray::mail,CRsrchArray::email,CRsrchArray::telephone
};

static int aiKeithPath[CRsrchArray::num_types] =
{
	CRsrchArray::fire_control,CRsrchArray::medium_facilities,CRsrchArray::armored_vehicle,
	CRsrchArray::tanks,CRsrchArray::gas_turbine,CRsrchArray::large_facilities,
	CRsrchArray::nuclear,CRsrchArray::artillery,CRsrchArray::medium_vehicle,
	CRsrchArray::fortification,CRsrchArray::mine_1,CRsrchArray::manf_1,
	CRsrchArray::const_1,CRsrchArray::heavy_vehicle,CRsrchArray::advanced_facilities,
	CRsrchArray::const_2,CRsrchArray::manf_2,CRsrchArray::copper,
	CRsrchArray::rockets,CRsrchArray::manf_3,CRsrchArray::const_3,
	CRsrchArray::atk_1,CRsrchArray::def_1,CRsrchArray::range_1,
	CRsrchArray::acc_1,CRsrchArray::spot_1,CRsrchArray::spot_2,
	CRsrchArray::range_1,CRsrchArray::atk_2,CRsrchArray::def_2,
	CRsrchArray::acc_2,CRsrchArray::range_3,CRsrchArray::atk_3,             
	CRsrchArray::def_3,CRsrchArray::acc_3,CRsrchArray::sailboats,
	CRsrchArray::motorboats,CRsrchArray::heavy_naval,CRsrchArray::spot_3,
	CRsrchArray::bridge,CRsrchArray::farm_1,CRsrchArray::mine_2,
	CRsrchArray::landing_craft,CRsrchArray::cargo_handling,CRsrchArray::balloons,
	CRsrchArray::gliders,CRsrchArray::prop_planes,CRsrchArray::jet_planes,
	CRsrchArray::radio,CRsrchArray::mail,CRsrchArray::email,CRsrchArray::telephone
};

/*
//
// upon receipt, this had 6 duplicates and one dead end path so it is not used
//
static int aiDavePath[CRsrchArray::num_types] =
{
	CRsrchArray::fire_control,CRsrchArray::artillery,CRsrchArray::medium_facilities,
	CRsrchArray::large_facilities,CRsrchArray::armored_vehicle,CRsrchArray::tanks,
	CRsrchArray::fortification,CRsrchArray::medium_vehicle,CRsrchArray::copper,
	CRsrchArray::heavy_vehicle,CRsrchArray::rockets,CRsrchArray::gas_turbine,
	CRsrchArray::nuclear,CRsrchArray::atk_1,CRsrchArray::def_1,
	CRsrchArray::acc_1,CRsrchArray::range_1,CRsrchArray::spot_1,
	CRsrchArray::atk_2,CRsrchArray::def_2,CRsrchArray::acc_2,
	CRsrchArray::range_2,CRsrchArray::spot_2,CRsrchArray::advanced_facilities,
	CRsrchArray::atk_3,CRsrchArray::acc_3,CRsrchArray::range_3,
	CRsrchArray::spot_3,CRsrchArray::def_3,
    CRsrchArray::manf_1,CRsrchArray::const_1,CRsrchArray::mine_1,
	CRsrchArray::farm_1,CRsrchArray::manf_2,CRsrchArray::const_2,
	CRsrchArray::mine_2,CRsrchArray::manf_3,CRsrchArray::const_3,
	CRsrchArray::bridge,
	CRsrchArray::sailboats,CRsrchArray::motorboats,CRsrchArray::landing_craft,
	CRsrchArray::heavy_naval,CRsrchArray::balloons,CRsrchArray::gliders,
	CRsrchArray::prop_planes,CRsrchArray::jet_planes,CRsrchArray::cargo_handling,
	CRsrchArray::radio,CRsrchArray::mail,CRsrchArray::email,CRsrchArray::telephone
};
*/

CAIGoalMgr::CAIGoalMgr( BOOL bRestart, int iPlayer,
	CAIMap *pMap, CAIUnitList *plUnits, CAIOpForList *plOpFors )
{
	m_iPlayer = iPlayer;
	m_bRestart = bRestart;
	
	m_pMap = pMap;
	m_plUnits = plUnits;
	m_plOpFors = plOpFors;

	m_plTasks = NULL;
	m_plGoalList = NULL;
	m_pos = NULL;

	m_bProductionChange = FALSE;
	m_bGoalChange = FALSE;
	m_bMapChanged = FALSE;
	m_bOceanWorld = FALSE;
	m_bLakeWorld = FALSE;
	m_bGunsOrButter = FALSE;
	m_bNeedTrucks = FALSE;
	m_iNeedApt = FALSE;
	m_iNeedOffice = FALSE;
	
	m_iLastFood = 0;
	m_iScenario = 0;
	m_dwRocket = 0;

	m_pwaRatios = NULL;
	m_pwaUnits = NULL;
	m_pwaBldgs = NULL;
	m_pwaMatOnHand = NULL;
	m_pwaAttribs = NULL;
	m_pwaBldgGoals = NULL;
	m_pwaVehGoals = NULL;
	m_pwaMatGoals = NULL;

	m_cPriority = (BYTE)100;

	// start building coal power plants
	m_iPowerLvl = 1;
	for( int i=0; i<CRsrchArray::num_types; ++i )
		m_iRDPath[i] = 0;
	
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIGoalMgr::Initialize() Player=%d ", m_iPlayer );
#endif

	Initialize();
	GetVehicleNeeds();
	
	ASSERT_VALID( this );
}


//
// if CAIMgr::m_bGoalAssess flag is set, then reassess the game, 
// otherwise use the current assessment based on:
//
//    influence map
//    opfor sightings
//    player condition
//    current goal list
//    current task list priortized
//
void CAIGoalMgr::Assess( CAIMsg *pMsg )
{
	ASSERT_VALID( this );

#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
	//if( myYieldThread() == TM_QUIT )
	//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

#ifdef _LOGOUT
	CString sMsg = pGameData->GetMsgString(pMsg->m_iMsg);
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIGoalMgr::Assess() Player=%d ", m_iPlayer );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"Message id %d is a %s ", pMsg->m_iMsg, 
    	(const char *)sMsg );

	sMsg.Empty();
#endif

	if( pMsg->m_iMsg == CNetCmd::unit_damage &&
		pMsg->m_idata3 != m_iPlayer )
		return;

	// BUGBUG this is for testing, but may actually be
	// a good idea to do in any game?
	// an opfor rocket was found
	if( pMsg->m_iMsg == CNetCmd::bldg_new &&
		pMsg->m_idata3 != m_iPlayer &&
		pMsg->m_idata1 == CStructureData::rocket )
	{
		// consider upgrading to IDG_BASICDEFENSE
		CAIGoal *pGoal = m_plGoalList->GetGoal(IDG_BASICDEFENSE);
		if( pGoal == NULL )
		{
			// basic goals for defending the area
			AddGoal(IDG_BASICDEFENSE);
			m_bGoalChange = TRUE;
		}

		// more aggressive set of goals added?
		if( m_pwaAttribs[CRaceDef::attack] >= 90 &&
			pGameData->m_iSmart )
		{
			// consider upgrading to IDG_ADVDEFENSE
			pGoal = m_plGoalList->GetGoal(IDG_ADVDEFENSE);
			if( pGoal == NULL )
			{
				AddGoal(IDG_ADVDEFENSE);
				m_bGoalChange = TRUE;
			}
			m_bGunsOrButter = TRUE;
		}
	}
	
	// SET STATES
	// message indicating to get started
	if( pMsg->m_iMsg == CNetCmd::cmd_play )
	{
		m_bGoalChange = TRUE;
		m_bOceanWorld = TRUE;
		m_bLakeWorld = TRUE;

		// check with map for too little ocean to matter
		if( m_pMap->m_iOcean < (m_pMap->m_iLand/2) )  // was 10
			m_bOceanWorld = FALSE;
		if( m_pMap->m_iLake < (m_pMap->m_iLand/4) )  // was 20
			m_bLakeWorld = FALSE;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIGoalMgr::m_iPlayer=%d m_bOceanWorld=%d m_bLakeWorld=%d ",
		m_iPlayer, m_bOceanWorld, m_bLakeWorld );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"m_iLand/m_iOcean/m_iLake %d/%d/%d \n", 
		m_pMap->m_iLand, m_pMap->m_iOcean, m_pMap->m_iLake );
#endif

		if( (m_bOceanWorld || m_bLakeWorld)
			&& pGameData->m_iSmart )
		{
			// protection for sea borne units
			CAIGoal *pGoal = m_plGoalList->GetGoal(IDG_SHORES);
			if( pGoal == NULL)
				AddGoal(IDG_SHORES);

			// load naval biased research path
			for( int i=0; i<CRsrchArray::num_types; ++i )
				m_iRDPath[i] = aiStevePath[i];

			// BUGBUG for testing
			//pGoal = m_plGoalList->GetGoal(IDG_SEAINVADE);
			//if( pGoal == NULL)
			//	AddGoal(IDG_SEAINVADE);
			m_bGunsOrButter = TRUE;
		}

		// update map utility with goalmgr's opinion
		m_pMap->m_pMapUtil->m_bOceanWorld = m_bOceanWorld;
		m_pMap->m_pMapUtil->m_bLakeWorld = m_bLakeWorld;
	}

	// new unit was created
	//
	// opfor new units are being reported here too
	if( (pMsg->m_iMsg == CNetCmd::veh_new ||
		pMsg->m_iMsg == CNetCmd::delete_unit ||
		pMsg->m_iMsg == CNetCmd::bldg_new) &&
		pMsg->m_idata3 == m_iPlayer )
		m_bProductionChange = TRUE;

	// something happened to units
	if( pMsg->m_iMsg == CNetCmd::unit_damage &&
		pMsg->m_idata3 == m_iPlayer )
	{
		// consider upgrading to IDG_ADVDEFENSE
		CAIGoal *pGoal = m_plGoalList->GetGoal(IDG_ADVDEFENSE);
		if( pGoal == NULL )
		{
			AddGoal(IDG_ADVDEFENSE);
			m_bGoalChange = TRUE;
		}
		m_bMapChanged = TRUE;
		m_bGunsOrButter = TRUE;
	}

	// consider scenario
	if( pMsg->m_iMsg == CNetCmd::scenario )
	{
		m_bGoalChange = TRUE;
	}


	// handle opfor separately
	
	if( pMsg->m_iMsg == CNetCmd::delete_unit
		&&
		pMsg->m_idata3 != m_iPlayer )
		UpdateSummarys( pMsg );
	



	// OPERATE ON STATES FROM HERE ON
	if( m_bProductionChange || m_bGoalChange )
	{
		UpdateStagingTasks();
		UpdateSummarys( pMsg );
		//ConsiderRoads();
	}
		
	// on these state changes consider task priorities
	if( m_bProductionChange || 
	    m_bGoalChange ||
		m_bMapChanged )
		UpdateTaskPriorities( pMsg );
	
    // only perform this when a production/
    // construction completed message is received
	// or on a goal change
    if( m_bProductionChange || m_bGoalChange )
		ConsiderProduction(pMsg);

	m_bGoalChange = FALSE;
	m_bProductionChange = FALSE;
	m_bMapChanged = FALSE;
}

//
// called to record a state change
//
void CAIGoalMgr::SetProductionChange( void )
{
	ASSERT_VALID( this );
	m_bProductionChange = TRUE;
}
void CAIGoalMgr::SetMapChange( void )
{
	ASSERT_VALID( this );
	m_bMapChanged = TRUE;
}

BOOL CAIGoalMgr::GetGoalChange(void)
{
	return m_bGoalChange;
}

void CAIGoalMgr::ResetGoalChange( void )
{
	ASSERT_VALID( this );
	m_bGoalChange = FALSE;
}

//
// called by CAIMgr for AI guidance in opening
// fire on an opfor unit
//
BOOL CAIGoalMgr::AutoFire( int iPlayer )
{
	CAIOpFor *pOpFor = m_plOpFors->GetOpFor( iPlayer );
	// a known opfor
	if( pOpFor != NULL )
	{
		// on the highest difficulty AI players have auto-alliance
		if( pOpFor->IsAI() && pGameData->m_iSmart > 1 )
		{
			// except for on MODERATE level, leave a chance that
			// the AI will shoot another AI player
			/*
			if( pGameData->m_iSmart == 1 )
			{
				if( !pGameData->GetRandom(NUM_DIFFICUTY_LEVELS) )
					return( TRUE );
			}
			*/
			return( FALSE );
		}

		// highest level of difficult always fires back
		if( !pOpFor->IsAI() && pGameData->m_iSmart )
		{
			// except for on MODERATE level, when not at war then
			// make a fuzzy decision to fire
			/*
			if( pGameData->m_iSmart == 1 && !pOpFor->AtWar() )
			{
				if( pGameData->GetRandom(NUM_DIFFICUTY_LEVELS) )
					return( FALSE );
			}
			*/
			return( TRUE );
		}

		return( pOpFor->AtWar() );
	}
	else // unknown opfor
	{
		// consider this AI Player's warlike tendancy
		if( m_pwaAttribs[CRaceDef::attack] > 100 )
			return( TRUE );

		return( FALSE );
	}
}

//
// a message was received that did not report an
// attack or damage to this AI Player so for each 
// opfor known, reduce threat factor
//
void CAIGoalMgr::AdjustThreats( void )
{

    if( m_plOpFors->GetCount() )
    {
        POSITION pos = m_plOpFors->GetHeadPosition();
        while( pos != NULL )
        {   
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

            CAIOpFor *pOpFor = (CAIOpFor *)m_plOpFors->GetNext( pos );
            if( pOpFor != NULL )
            {
            	ASSERT_VALID( pOpFor );
				pOpFor->AdjustThreat();
			}
		}
	}
}

//
// this is called by the CAIMgr upon receipt of an
// CNetCmd::unit_damage message
//
// need to update status of the opfor from the message
//
// need to review and update goals pertaining to war
//
void CAIGoalMgr::ConsiderThreats( CAIMsg *pMsg )
{
	// need to update for new messages
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIGoalMgr::ConsiderThreats() for %d executing ", 
    m_iPlayer );
#endif

	CAIGoal *pGoal = NULL;
	CAIOpFor *pOpFor = NULL;
	m_bGoalChange = FALSE;

	// message reporting an attack on AI's units
	if( pMsg->m_iMsg == CNetCmd::unit_damage )
	{
		// get unit fired upon
		CAIUnit *pTarget = m_plUnits->GetUnit( pMsg->m_dwID );
		if( pTarget == NULL )
		{
			// unit might have been destroyed
			return;
		}
		if( pTarget->GetOwner() != m_iPlayer )
			return;

		// id of the attacking unit is used to get id of opfor
		CAIUnit *pAttacker = m_plUnits->GetOpForUnit( pMsg->m_dwID2 );
		if( pAttacker == NULL )
		{
			// unit might have been destroyed
			return;
		}
		
		int iOpForID = pAttacker->GetOwner();
		pOpFor = m_plOpFors->GetOpFor( iOpForID );

		// new CAIOpFor is needed
		if( pOpFor == NULL )
		{
			pOpFor = m_plOpFors->AddOpFor( iOpForID );
			if( pOpFor == NULL )
				return;
#if THREADS_ENABLED
					// BUGBUG this function must yield
					myYieldThread();
					//if( myYieldThread() == TM_QUIT )
					//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			// this will take a while to initialize
			pOpFor->UpdateCounts(NULL);
			pOpFor->UpdateCounts(pMsg);
		}
		else
			pOpFor->UpdateCounts(pMsg);

		// update attack counts by unit type
		if( pTarget->GetType() == CUnit::building )
			pOpFor->AddBldgAttack( pTarget->GetTypeUnit() );
		else if( pTarget->GetType() == CUnit::vehicle )
			pOpFor->AddUnitAttack( pTarget->GetTypeUnit() );

		// make sure standard defend goal is in place
		pGoal = m_plGoalList->GetGoal(IDG_ADVDEFENSE);
		if( pGoal == NULL )
		{
			AddGoal(IDG_ADVDEFENSE);
			m_bGoalChange = TRUE;
		}

		// if difficulty is high, set at war, irregularly
		if( pGameData->m_iSmart )
		{
			if( pOpFor->IsAI() )
			{
				if( pOpFor->AtWar() )
				{
					// the higher the difficulty, then the harder
					// it will be to turn off being at war with AI
					if( pGameData->GetRandom(NUM_DIFFICUTY_LEVELS)
						< pGameData->m_iSmart )
						pOpFor->SetWar(FALSE);

					if( pGameData->m_iSmart == 
						(NUM_DIFFICUTY_LEVELS - 1) )
						pOpFor->SetWar(FALSE);
				}
				else
				{
					if( pGameData->m_iSmart == 
						(NUM_DIFFICUTY_LEVELS - 1) )
						pOpFor->SetWar(FALSE);
					else
					{
						// the higher the difficulty, then the harder
						// it will be to turn on being at war with AI
						if( pGameData->GetRandom(NUM_DIFFICUTY_LEVELS)
							>= pGameData->m_iSmart )
							pOpFor->SetWar(TRUE);
					}
				}
			}
			// if we are being shot at, then turn on combat production
			m_bGunsOrButter = TRUE;
		}

	// need to consider that unit just attacked needs
	// support, so each combat unit of the manager, that is
	// either IDT_PATROL, IDT_PREPAREWAR, should switch to a
	// IDT_SEEKINRANGE and help attacked unit.  How?

	// get location of attacking and attacked units
	// form search space by extending the extremes
	// of their locations

	// go through the units of this mgr and look for
	// units both in the area and that have the above tasks

	// for each unit found, set task to IDT_SEEKINRANGE and
	// select cooridinated targeting upon the attacking unit
	// and any other opfor units in the search space and send
	// destination message to cause approach

		//GetSupport( pTarget, pAttacker, pMsg );
	}

	

	// they are in use, check the current 
	// counts of all opfor units on each siting/kia
	// and based on difficulty and combat settings
	// of this goal mgr, add goals to match opfors
	// unit counts triggering when AI's units are
	// easy=110% ave=100% hard=80% imp=60% of opfors
	// so that ratio between ai/opfor unit counts becomes 
	// easy=80% ave=100% hard=150% imp=200% of opfors

	//
	// an opfor unit was spotted
	//
	if( pMsg->m_iMsg == CNetCmd::see_unit )
	{
		// get unit just spotted
		CAIUnit *pTarget = m_plUnits->GetOpForUnit( pMsg->m_dwID2 );
		if( pTarget == NULL )
			return;

		if( pTarget->GetOwner() == m_iPlayer )
			return;

		int iOpForID = pTarget->GetOwner();
		//pOpFor = m_plOpFors->GetOpFor( iOpForID );
		pOpFor = GetOpFor( iOpForID );

		// new CAIOpFor is needed
		if( pOpFor == NULL )
		{
			pOpFor = m_plOpFors->AddOpFor( iOpForID );
			if( pOpFor == NULL )
				return;
			// this will take a while to initialize
			pOpFor->UpdateCounts(NULL);
		}
		else
		{
#if THREADS_ENABLED
					// BUGBUG this function must yield
					myYieldThread();
					//if( myYieldThread() == TM_QUIT )
					//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			// count the number of known opfor units of this type
			WORD wCnt = 0;
   			POSITION pos = m_plUnits->GetHeadPosition();
   			while( pos != NULL )
   			{   
    			CAIUnit *paiOpFor = (CAIUnit *)m_plUnits->GetNext( pos );
    			if( paiOpFor != NULL )
    			{
    				ASSERT_VALID( paiOpFor );

					if( paiOpFor->GetOwner() != iOpForID )
						continue;
					if( paiOpFor->GetTypeUnit() ==
						pTarget->GetTypeUnit() )
						wCnt++;
				}
			}
			// and update opfor with that count
			if( pTarget->GetType() == CUnit::building )
				pOpFor->SetBuilding( pTarget->GetTypeUnit(), wCnt );
			else if( pTarget->GetType() == CUnit::vehicle )
				pOpFor->SetVehicle( pTarget->GetTypeUnit(), wCnt );
		}


		// must have spotted any building
		if( pTarget->GetType() == CUnit::building )
		{
			// on rockets, set advance defense goal
			if( pTarget->GetTypeUnit() == CStructureData::rocket &&
				pGameData->m_iSmart )
			{
				pGoal = m_plGoalList->GetGoal(IDG_ADVDEFENSE);
				if( pGoal == NULL )
				{
					AddGoal(IDG_ADVDEFENSE);
					m_bGoalChange = TRUE;
				}
			}

			// opfor has ship building so start up invasion prep
			pGoal = m_plGoalList->GetGoal(IDG_SEAINVADE);
			if( pGoal == NULL )
			{
				if( pOpFor->GetBuilding(CStructureData::seaport) ||
					pOpFor->GetBuilding(CStructureData::shipyard_1) ||
					pOpFor->GetBuilding(CStructureData::shipyard_3) )
				{
					AddGoal(IDG_SEAINVADE);
					m_bGoalChange = TRUE;
				}
			}
		}
		else if( pTarget->GetType() == CUnit::vehicle )
		{
			// add IDG_REPELL if possible invasion fleet enroute
			pGoal = m_plGoalList->GetGoal(IDG_REPELL);
			if( pGoal == NULL )
			{
				if( pOpFor->GetVehicle(CTransportData::gun_boat) ||
					pOpFor->GetVehicle(CTransportData::destroyer) ||
					pOpFor->GetVehicle(CTransportData::cruiser) ||
					pOpFor->GetVehicle(CTransportData::landing_craft) )
				{
					AddGoal(IDG_REPELL);
					m_bGoalChange = TRUE;
				}
			}

			// add IDG_PIRATE if Opfor known cargo ships
			pGoal = m_plGoalList->GetGoal(IDG_PIRATE);
			if( pGoal == NULL )
			{
				if( pOpFor->GetVehicle(CTransportData::light_cargo) ||
					pOpFor->GetVehicle(CTransportData::gun_boat) ||
					pOpFor->GetVehicle(CTransportData::destroyer) )
				{
					AddGoal(IDG_PIRATE);
					m_bGoalChange = TRUE;
				}
			}
		}
	}

	// adjust general warfare goals if needed, however by
	// the time the game has run for a bit, these will all
	// be set
	if( pOpFor != NULL )
	{
		if( pOpFor->AtWar() )
		{
			// add IDG_LANDWAR if now at war with this OpFor
			pGoal = m_plGoalList->GetGoal(IDG_LANDWAR);
			if( pGoal == NULL )
			{
				AddGoal(IDG_LANDWAR);
				m_bGoalChange = TRUE;
			}
			pGoal = m_plGoalList->GetGoal(IDG_ADVPRODUCTION);
			if( pGoal == NULL )
			{
				AddGoal(IDG_ADVPRODUCTION);
				m_bGoalChange = TRUE;
			}
			// add IDG_SEAWAR if at war
			pGoal = m_plGoalList->GetGoal(IDG_SEAWAR);
			if( pGoal == NULL )
			{
				AddGoal(IDG_SEAWAR);
				m_bGoalChange = TRUE;
			}
			m_bGunsOrButter = TRUE;
		}
		else // not at war
		{
			if( pOpFor->GetRelations() < NEUTRAL )
			{
				// consider upgrading to IDG_ADVDEFENSE
				pGoal = m_plGoalList->GetGoal(IDG_ADVDEFENSE);
				if( pGoal == NULL )
				{
					AddGoal(IDG_ADVDEFENSE);
					m_bGoalChange = TRUE;
				}
			}
			
			if( pOpFor->GetRelations() < PEACE )
			{
				// IDG_BASICDEFENSE does not exist, consider adding it
				pGoal = m_plGoalList->GetGoal(IDG_BASICDEFENSE);
				if( pGoal == NULL )
				{
					AddGoal(IDG_BASICDEFENSE);
					m_bGoalChange = TRUE;
				}

				pGoal = m_plGoalList->GetGoal(IDG_SHORES);
				if( pGoal == NULL )
				{
					AddGoal(IDG_SHORES);
					m_bGoalChange = TRUE;
				}

				m_bGunsOrButter = FALSE;
			}

			// by the time a command center is built, its time
			// to start a war!
			if( m_pwaBldgs[CStructureData::command_center] )
			{
				// add IDG_LANDWAR if now at war with this OpFor
				pGoal = m_plGoalList->GetGoal(IDG_LANDWAR);
				if( pGoal == NULL )
				{
					AddGoal(IDG_LANDWAR);
					m_bGoalChange = TRUE;
				}
				pGoal = m_plGoalList->GetGoal(IDG_ADVPRODUCTION);
				if( pGoal == NULL )
				{
					AddGoal(IDG_ADVPRODUCTION);
					m_bGoalChange = TRUE;
				}
			}
		}
	}

	if( pMsg->m_iMsg == CNetCmd::unit_damage )
	{
		// get unit that just attacked, and by doing so it is added
		// to the known units list if not present already in there
		CAIUnit *pAttacker = m_plUnits->GetOpForUnit( pMsg->m_dwID2 );
		if( pAttacker == NULL )
			return;

		if( pAttacker->GetOwner() == m_iPlayer )
			return;

		// for each type of combat unit
		//   for which a build task exists for it
		//   and that m_pwaUnits[] <= m_pwaVehGoals[]
		//   then increase that task's build qty
    	POSITION pos = m_plTasks->GetHeadPosition();
    	while( pos != NULL )
    	{   
        	CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
        	if( pTask != NULL )
        	{
        		ASSERT_VALID( pTask );

				// that produce something
				if( pTask->GetTaskParam(ORDER_TYPE) == 
					PRODUCTION_ORDER &&
					pTask->GetTaskParam(PRODUCTION_TYPE) == 
					CStructureData::UTvehicle )
				{
					// get id of the vehicle being produced
					int iVeh = pTask->GetTaskParam(PRODUCTION_ITEM);
					if( iVeh >= m_iNumUnits )
						continue;

					// BUGBUG does land combat vehicles only
					if( !pGameData->IsCombatVehicle(iVeh) )
						continue;

#if THREADS_ENABLED
					// BUGBUG this function must yield
					myYieldThread();
					//if( myYieldThread() == TM_QUIT )
					//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
					// make adjustment only when a goal has been established
					if( m_pwaVehGoals[iVeh] )
					{
						if( m_pwaVehGoals[iVeh] < m_plOpFors->GetVehicleCnt(iVeh,TRUE) )
						{
							WORD wQty = 
							pTask->GetTaskParam(PRODUCTION_QTY) + 1;
							pTask->SetTaskParam(PRODUCTION_QTY,wQty);
							m_bGoalChange = TRUE;
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::ConsiderThreats() player %d goal %d task %d qty increased to %d ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID(), pTask->GetTaskParam(PRODUCTION_QTY) );
#endif
						}
					}
				}
			}
		}
		m_bGunsOrButter = TRUE;
	}

	if( m_bGoalChange )
	{
		UpdateConstructionTasks(pMsg);
		UpdateProductionTasks(pMsg);

	    GetVehicleNeeds();
	}
}

//
// handle the scenario_atk message in its "attack the HP" form
//
void CAIGoalMgr::AttackPlayer( int iOpforID )
{
	if( !iOpforID )
		return;
	if( iOpforID == m_iPlayer )
		return;

	CAIOpFor *pOpFor = m_plOpFors->GetOpFor( iOpforID );
	if( pOpFor == NULL )
		return;

	// need to add defference to AI players based on difficulty
	// no, this is for scenarios, so attack away
	if( !pOpFor->AtWar() )
		pOpFor->SetWar(TRUE);

	// make sure appropriate goals exist
	CAIGoal *pGoal = NULL;
	pGoal = m_plGoalList->GetGoal(IDG_CONQUER);
	if( pGoal == NULL )
	{
		AddGoal(IDG_CONQUER);
		m_bGoalChange = TRUE;
	}
	pGoal = m_plGoalList->GetGoal(IDG_LANDWAR);
	if( pGoal == NULL )
	{
		AddGoal(IDG_LANDWAR);
		m_bGoalChange = TRUE;
	}
	if( m_bOceanWorld )
	{
	// add IDG_SEAWAR 
		pGoal = m_plGoalList->GetGoal(IDG_SEAWAR);
		if( pGoal == NULL )
		{
			AddGoal(IDG_SEAWAR);
			m_bGoalChange = TRUE;
		}
	// add IDG_REPELL if at war and known Opfor naval invade
		if( pOpFor->GetVehicle(CTransportData::gun_boat) ||
			pOpFor->GetVehicle(CTransportData::destroyer) ||
			pOpFor->GetVehicle(CTransportData::cruiser) ||
			pOpFor->GetVehicle(CTransportData::landing_craft) )
		{
			pGoal = m_plGoalList->GetGoal(IDG_REPELL);
			if( pGoal == NULL )
			{
				AddGoal(IDG_REPELL);
				m_bGoalChange = TRUE;
			}
		}
	// add IDG_SEAINVADE if at war and known Opfor seaport
		if( pOpFor->GetBuilding(CStructureData::seaport) ||
			pOpFor->GetBuilding(CStructureData::shipyard) )
		{
			pGoal = m_plGoalList->GetGoal(IDG_SEAINVADE);
			if( pGoal == NULL )
			{
				AddGoal(IDG_SEAINVADE);
				m_bGoalChange = TRUE;
			}
		}
	}
	if( m_bLakeWorld )
	{
	// add IDG_SHORES
		pGoal = m_plGoalList->GetGoal(IDG_SHORES);
		if( pGoal == NULL )
		{
			AddGoal(IDG_SHORES);
			m_bGoalChange = TRUE;
		}
	}

	// then for each patrol unit, pick best targets and attack
	
#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
	//if( myYieldThread() == TM_QUIT )
	//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	// and then rate that many known opfor targets and assign 2 of
	// the patrol units to attack each target by assigning the
	// scenario_atk goal/task
	DWORD dwTarget = 0;
   	POSITION posU = m_plUnits->GetHeadPosition();
   	while( posU != NULL )
   	{   
    	CAIUnit *pVeh = (CAIUnit *)m_plUnits->GetNext( posU );
    	if( pVeh != NULL )
    	{
    		ASSERT_VALID( pVeh );

			if( pVeh->GetOwner() != m_iPlayer )
				continue;

			if( pVeh->GetTask() != IDT_PATROL )
				continue;

			// get highest rated target, belonging to this opfor
			// that this patrol vehicle can attack
			// that has the fewest number of units targeting it
			dwTarget = GetOpforTarget( 
				pOpFor->GetPlayerID(), pVeh->GetTypeUnit() );
			if( dwTarget )
			{
				// take unit off of patrol
				pVeh->ClearParam();
				pVeh->SetGoal( IDG_CONQUER );
				pVeh->SetTask( IDT_ATTACKUNIT );
				// assign target to unit
				pVeh->SetDataDW( dwTarget );
				// and make unit move

				CHexCoord hexVeh(0,0);
				EnterCriticalSection (&cs);
				CVehicle *pVehicle = theVehicleMap.GetVehicle( pVeh->GetID() );
				if( pVehicle != NULL )
					hexVeh = pVehicle->GetHexHead();
				LeaveCriticalSection (&cs);

				if( hexVeh.X() || hexVeh.Y() )
				{
					CHexCoord hexDest = hexVeh;
					// find staging hex to move unit to
					m_pMap->m_pMapUtil->FindStagingHex( 
						hexVeh, 1, 1, pVeh->GetTypeUnit(), hexDest );

					if( hexVeh != hexDest )
						pVeh->SetDestination( hexDest );
				}
			}
		}
	}
}

//
// make sure the appropriate goals/task
// are loaded based on this scenario
//
void CAIGoalMgr::Scenario( int iScenario )
{
	CAIGoal *pGoal = m_plGoalList->GetGoal(IDG_CONQUER);
	if( pGoal == NULL )
	{
		AddGoal(IDG_CONQUER);
		m_bGoalChange = TRUE;
	}
	m_iScenario = iScenario;
}

//
// consider the construction task passed if building a scenario
// specific building and if the appropriate scenario is underway
// and the right building task is passed, then max priority
//
void CAIGoalMgr::ScenarioPriority( CAITask *pTask )
{
	if( !m_iScenario )
		return;

	int iBldg = (int)pTask->GetTaskParam(BUILDING_ID);
	switch( m_iScenario )
	{
		case 1:
			switch(iBldg)
			{
				case CStructureData::farm:
				case CStructureData::lumber:
					pTask->SetPriority( 99 );
				default:
					break;
			}
			break;
		case 2:
			switch(iBldg)
			{
				case CStructureData::iron:
				case CStructureData::coal:
				case CStructureData::smelter:
					pTask->SetPriority( 99 );
				default:
					break;
			}
			break;
		case 3:
			switch(iBldg)
			{
				case CStructureData::oil_well:
				case CStructureData::refinery:				
				case CStructureData::power_1:
				//case CStructureData::command_center:
				//case CStructureData::research:
				//case CStructureData::light_0:
				//case CStructureData::light_1:
					pTask->SetPriority( 99 );
				default:
					break;
			}
			break;
		case 4:
			switch(iBldg)
			{
				case CStructureData::research:
					pTask->SetPriority( 99 );
				default:
					break;
			}
			break;
		case 5:
			if( iBldg == CStructureData::repair )
				pTask->SetPriority( 99 );
			break;
		case 6:
			if( iBldg == CStructureData::copper )
				pTask->SetPriority( 99 );
			break;
		case 8:
			if( iBldg == CStructureData::heavy )
				pTask->SetPriority( 99 );
			break;
		default:
			break;
	}
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIGoalMgr::player %d scenario building goal %d task %d priority %d ", 
		m_iPlayer, pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
#endif
}

//
//
void CAIGoalMgr::ConsiderProduction( CAIMsg *pMsg )
{
	ASSERT_VALID( this );

#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
	//if( myYieldThread() == TM_QUIT )
	//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
	
	if( pMsg == NULL )
		return;

	// on all 'new' messages, make sure it is ours
	if( pMsg->m_iMsg == CNetCmd::bldg_new ||
		pMsg->m_iMsg == CNetCmd::veh_new )
	{
		if( pMsg->m_idata3 != m_iPlayer )
			return;
	}

	// when rocket has be placed, save dwID
	if( pMsg->m_iMsg == CNetCmd::bldg_new &&
		pMsg->m_idata1 == CStructureData::rocket )
		m_dwRocket = pMsg->m_dwID;
	// start vehicles must add to goals
	if( pMsg->m_iMsg == CNetCmd::veh_new &&
		pMsg->m_dwID2 == m_dwRocket )
		SetStartVehicles(pMsg);	

	// used to record the vehicle, if applicable just built
	// or affected by the message
	int iVeh = m_iNumUnits;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::ConsiderProduction() for %d executing ", m_iPlayer );
#endif

	/*
		build_bldg,			// build a building
		build_veh,			// build a vehicle
		trans_mat,			// transfer materials between units
		unit_damage,		// notify of damage level
						// sent from server?
		bldg_new,			// a new building is created
		bldg_stat,			// built/damage status of a bldg
		veh_new,			// a new vehicle is created
		veh_stat,			// damage status of a vehicle
		unit_set_damage,	// received damage level
	*/
	switch( pMsg->m_iMsg )
	{
		// this initializes all arrays from scratch
		//case CNetCmd::NET_CMD_PLAY:
		case CNetCmd::cmd_play:
			GetCommodityNeeds();
			GetVehicleNeeds();
			return;
		// the rest are responses to events
		//case CNetCmd::build_bldg:
		//case CNetCmd::build_veh:
		case CNetCmd::trans_mat:
		case CNetCmd::bldg_new:
		case CNetCmd::bldg_stat:
			GetCommodityNeeds();
			GetVehicleNeeds();
			break;
		case CNetCmd::veh_new:		// may need materials now
			GetCommodityNeeds();
		case CNetCmd::veh_stat:
			GetVehicleNeeds();
			iVeh = pMsg->m_idata1;	// record vehicle type effected
			break;
		case CNetCmd::unit_damage:		// need to only check these
		case CNetCmd::unit_set_damage:	// every so often
			GetCommodityNeeds();
			GetVehicleNeeds();
			break;
		default:
			return;
	}

	// consider the building just completed and if it
	// is a vehicle factory, or a material producer then
	// update the truck goals
	if( pMsg->m_iMsg == CNetCmd::bldg_new )
		ConsiderTrucks( pMsg );

	// for any need count > goal count, add a goal
	// to construct or produce the needed item and
	// update goal count with new goal

	for( int i=0; i<m_iNumMats; ++i )
	{
		if( m_pwaMatGoals[i] > m_pwaMatOnHand[i] )
			ProduceMaterials(i);
	}


	for( i=0; i<m_iNumUnits; ++i )
	{
		if( (m_pwaVehGoals[i] >
			m_pwaUnits[i]) &&
			i == iVeh )
			ProduceVehicles(i);
	}

}

// consider the building just completed and if it
// is a vehicle factory, or a material producer then
// update the truck goals
//
void CAIGoalMgr::ConsiderTrucks( CAIMsg *pMsg )
{
	BOOL bAddTruck = FALSE;
	switch( pMsg->m_idata1 )
	{
	case CStructureData::smelter:
	case CStructureData::light_0:
	case CStructureData::light_1:
	case CStructureData::light_2:
	case CStructureData::heavy:
	case CStructureData::barracks_2:
	case CStructureData::shipyard_1:
	case CStructureData::shipyard_3:
		bAddTruck = TRUE;
	default:
		break;
	}
	if( !bAddTruck )
		return;

	CAITask *pTask = m_plTasks->FindTask( IDT_MAKEHTRUCK );
	if( pTask == NULL )
	{
		AddGoal(IDG_ADVPRODUCTION);
		m_bGoalChange = TRUE;
		return;
	}
	// update quantity to produce so vehicle goals get updated too
	int iQty = pTask->GetTaskParam(PRODUCTION_QTY) + 1;
	if( iQty > (pGameData->m_iSmart * 3) )
		iQty = pGameData->m_iSmart * 3;
	if( !iQty )
		iQty = 6;
	pTask->SetTaskParam(PRODUCTION_QTY,iQty);
	m_bGoalChange = TRUE;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::ConsiderTrucks() player %d task=%d priority=%d qty=%d \n", 
m_iPlayer, pTask->GetID(), (int)pTask->GetPriority(), 
pTask->GetTaskParam(PRODUCTION_QTY) );
#endif
}

//
// determine if CTransportData::construction vehicles need
// to have a goal added to build more
//
BOOL CAIGoalMgr::NeedCranes( void )
{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	// first determine if there vehicle factories producing cranes
	BOOL bMakingCranes = FALSE;
	BOOL bCranesUnassigned = FALSE;
    POSITION pos = m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );
			
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// check all types of vehicle factorys
			if( pUnit->GetTypeUnit() == CStructureData::light_0 ||
				pUnit->GetTypeUnit() == CStructureData::light_1 ||
				pUnit->GetTypeUnit() == CStructureData::light_2 ||
				pUnit->GetTypeUnit() == CStructureData::heavy )
			{
				if( pUnit->GetTask() == IDT_MAKECONSTRUCT )
				{
					bMakingCranes = TRUE;
					break;
				}
			}
			// check for cranes themselves
			if( pUnit->GetTypeUnit() == CTransportData::construction )
			{
				if( !pUnit->GetTask() )
				{
					bCranesUnassigned = TRUE;
					break;
				}
			}
		}
	}

	if( bMakingCranes || bCranesUnassigned )
		return FALSE;
	return TRUE;
}

//
// this is called directly by the CAIMgr as a result of
// the CAIRouter reporting a need of trucks in routing
//
void CAIGoalMgr::NeedTrucks( void )
{
	// only possible truck production tasks with tLP
	//
	// IDT_MAKEHTRUCK
	// IDT_MAKEMTRUCK

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	// first determine if there are any available vehicle factories
	BOOL bMakeTrucks = FALSE;
    POSITION pos = m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );
			
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// for time being, only make trucks in vehicle factory
			if( pUnit->GetTypeUnit() == CStructureData::light_0 )
			{
				if( pUnit->GetTask() != IDT_MAKEHTRUCK &&
					pUnit->GetTask() != IDT_MAKEMTRUCK )
					bMakeTrucks = TRUE;
			}
		}
	}
	if( !bMakeTrucks )
	{
		// need to increase the priority of building
		// a light_0 vehicle factory
		m_bNeedTrucks = TRUE;
		return;
	}
	m_bNeedTrucks = FALSE;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIGoalMgr::NeedTrucks() for %d executing ", m_iPlayer );
#endif

	CAITask *pTask = m_plTasks->FindTask( IDT_MAKEHTRUCK );
	//if( pTask == NULL )
	//	pTask = m_plTasks->FindTask( IDT_MAKEMTRUCK );
	if( pTask == NULL )
	{
		AddGoal(IDG_ADVPRODUCTION);
		m_bGoalChange = TRUE;
		return;
	}

	if( pTask->GetTaskParam(ORDER_TYPE) != PRODUCTION_ORDER &&
		pTask->GetTaskParam(PRODUCTION_TYPE) != CStructureData::UTvehicle )
		return;

	// get standard task data
	CAITask *pStdTask = plTaskList->GetTask(pTask->GetID(),
		pTask->GetGoalID() );
	if( pStdTask == NULL )
	{
		pStdTask = plTaskList->GetTask(pTask->GetID(),0);
		if( pStdTask == NULL )
			return;
	}

	// priority has already been raised for this task
	if( pTask->GetPriority() > pStdTask->GetPriority() )
		return;

	// now initialize task with triple priority
	BYTE cPriority = (BYTE)((int)pStdTask->GetPriority() * 3);
	if( (int)cPriority > 99 )
		cPriority = (BYTE)99;
	pTask->SetPriority(	cPriority );

	// update quantity to produce so vehicle goals get updated too
	int iQty = pTask->GetTaskParam(PRODUCTION_QTY) + 1;
	pTask->SetTaskParam(PRODUCTION_QTY,iQty);
	m_bGoalChange = TRUE;
	m_bGunsOrButter = FALSE;
	m_bNeedTrucks = TRUE;

	UpdateVehGoals();

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::NeedTrucks() player %d task=%d priority=%d qty=%d \n", 
m_iPlayer, pTask->GetID(), (int)pTask->GetPriority(), 
pTask->GetTaskParam(PRODUCTION_QTY) );
#endif
}

//
// called by CAIMgr to provide CAIRouter with inference on the
// priority to assign to refineries needing materials
//
BOOL CAIGoalMgr::NeedGas( void )
{
	// just getting started means there is no power needed or gas
	if( !m_iPowerNeed && !m_iGasHave )
		return FALSE;

	// if there is no refinery, there is no way to make gas
	if( !m_pwaBldgs[CStructureData::refinery] )
		return FALSE;

	if( !m_iGasHave )
	{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::NeedGas() player %d \n", m_iPlayer );
#endif

		// consider we need another refinery
		if( m_pwaBldgs[CStructureData::refinery] ==
			m_pwaBldgGoals[CStructureData::refinery] &&
			m_pwaBldgs[CStructureData::oil_well] ==
			m_pwaBldgGoals[CStructureData::oil_well] )
			ProduceMaterials(CMaterialTypes::gas);

		return TRUE;
	}

	return FALSE;
}


void CAIGoalMgr::ConsiderRoads( void )
{
	// get standard task data
	CAITask *pStdTask = plTaskList->FindTask( IDT_CONSTRUCT );
	if( pStdTask == NULL )
		return;

	CAITask *pRoadTask = m_plTasks->GetTask(
		IDT_CONSTRUCT,IDG_ESTABLISH);
	if( pRoadTask == NULL )
		return;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::ConsiderRoads() for %d ", m_iPlayer );
#endif

	if( !m_iGasHave && m_pMap->m_iRoadCount )
	{
		pRoadTask->SetPriority(	pStdTask->GetPriority() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::ConsiderRoads() player %d out of gas %d with roads %d", 
m_iPlayer, m_iGasHave,  m_pMap->m_iRoadCount );
#endif
		return;
	}

	if( !m_pMap->m_iRoadCount )
	{
		pRoadTask->SetPriority(	0 );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::ConsiderRoads() player %d out of roads %d", 
m_iPlayer, m_pMap->m_iRoadCount );
#endif
		return;
	}
	else
	{
		if( !pRoadTask->GetPriority() )
			pRoadTask->SetPriority(	pStdTask->GetPriority() );
	}

#if 0
	// consider if it is right to bump the priority of roadbuilding
	int iCnt = 0; // m_pwaBldgs[CStructureData::refinery];
	switch(pGameData->m_iSmart)
	{
	case 1:
		iCnt += m_pwaBldgs[CStructureData::light_1];
		iCnt += m_pwaBldgs[CStructureData::power_2];
		iCnt += m_pwaBldgs[CStructureData::light_0];
		break;
	case 2:
		iCnt += m_pwaBldgs[CStructureData::repair];
		iCnt += m_pwaBldgs[CStructureData::power_3];
		iCnt += m_pwaBldgs[CStructureData::heavy];
		break;
	default:
	case 3:
		iCnt += m_pwaBldgs[CStructureData::light_2];
		iCnt += m_pwaBldgs[CStructureData::heavy];
		iCnt += m_pwaBldgs[CStructureData::command_center];
		iCnt += m_pwaBldgs[CStructureData::fort_2];
		break;
	}

	// difficult rises in value, meaning that iCnt rising to beyond
	// difficulty value turns off priority road building, so as the
	// player is tougher, we want to build roads longer?
	if( pGameData->m_iSmart < iCnt )
	{
		pRoadTask->SetPriority(	pStdTask->GetPriority() );
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::ConsiderRoads() player %d road priority completed", m_iPlayer );
#endif
		return;
	}
#endif

	// priority has already been raised for this task
	if( pRoadTask->GetPriority() > pStdTask->GetPriority() )
	{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::ConsiderRoads() player %d road priority already set high", m_iPlayer );
#endif
		return;
	}

	// now initialize task so priority is greater than most buildings
	BYTE cPriority = (BYTE)((int)pStdTask->GetPriority() * 6);
	if( (int)cPriority > 99 )
		cPriority = (BYTE)99;
	pRoadTask->SetPriority(	cPriority );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::ConsiderRoads() player %d goal %d task %d priority %d", 
m_iPlayer, pRoadTask->GetGoalID(), pRoadTask->GetID(), pRoadTask->GetPriority() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::ConsiderRoads() player %d  gas %d  roads %d", 
m_iPlayer, m_iGasHave,  m_pMap->m_iRoadCount );
#endif
}

//
// make sure we do not spend all our gas on road building
//
BOOL CAIGoalMgr::IsGasAvailable( void )
{
	if( m_iGasHave > (10 * GAS_PER_ROAD) )
		//((m_pMap->m_iRoadCount/10) * GAS_PER_ROAD) )
		return TRUE;
	else
		return FALSE;
}

//
// the taskmgr reports that a crane has nothing to do so
// consider if this is a good time to build another research
//
void CAIGoalMgr::IdleCrane( void )
{
	// got to have at least one already built to consider more
	if( !m_pwaBldgs[CStructureData::research] )
		return;

	// only consider relative to difficulty setting
	if( m_pwaBldgGoals[CStructureData::research] >=
		pGameData->m_iSmart ||
		m_pwaBldgs[CStructureData::research] >=
		pGameData->m_iSmart )
		return;

	// and if current reseach institutes goals are met
	if( m_pwaBldgs[CStructureData::research] >=
		m_pwaBldgGoals[CStructureData::research] )
	{
		WORD wCnt = m_pwaBldgGoals[CStructureData::research];
		m_pwaBldgGoals[CStructureData::research] = wCnt + 1;
	}


}

BOOL CAIGoalMgr::CheckAbandonedBuildings( void )
{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::CheckAbandonedBuildings() for %d ", m_iPlayer );
#endif

	BOOL bHaveAbandoned = FALSE;
	for( int iBldg = CStructureData::lumber; 
			 iBldg <= CStructureData::copper; ++iBldg )
	{
		if( iBldg == CStructureData::refinery )
			continue;

		// only continue if count of these buildings
		// equal goals for these buildings
		if( m_pwaBldgs[iBldg] != m_pwaBldgGoals[iBldg] )
			continue;

		if( IsAbandonedBuilding(iBldg) )
		{
			bHaveAbandoned = TRUE;
			m_pwaBldgGoals[iBldg] += 1;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::CheckAbandonedBuildings() player %d  abandoned building %d cnt %d goals %d", 
m_iPlayer, iBldg, m_pwaBldgs[iBldg], m_pwaBldgGoals[iBldg] );
#endif
		}
	}
	return( bHaveAbandoned );
}

BOOL CAIGoalMgr::IsAbandonedBuilding( int iBldg )
{
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a building
			if( pUnit->GetType() != CUnit::building )
				continue;
			if( pUnit->GetTypeUnit() != iBldg )
				continue;

#if THREADS_ENABLED
	myYieldThread();
#endif

			BOOL bIsDepleted = FALSE;
			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
			if( pBldg != NULL )
				bIsDepleted = pBldg->IsFlag( CUnit::abandoned );	
			LeaveCriticalSection (&cs);

			// is this an abandoned building
			if( bIsDepleted )
				return( bIsDepleted );
		}
	}
	return( FALSE );
}

//
// this is called by the CAIMgr upon receipt
// of any CNetCmd::bldg_new message
//
void CAIGoalMgr::CheckResearch( void )
{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::CheckResearch() for %d \n", m_iPlayer );
#endif

	// make sure we have a research center before continuing
	if( !m_pwaBldgs[CStructureData::research] )
		return;
	// and that we have goals matching difficulty level
	if( m_pwaBldgGoals[CStructureData::research] < pGameData->m_iSmart )
		m_pwaBldgGoals[CStructureData::research] = pGameData->m_iSmart;
	if( !m_pwaBldgs[CStructureData::research] )
		m_pwaBldgGoals[CStructureData::research] = 1;

	int iTopic = CRsrchArray::num_types;
	EnterCriticalSection( &cs );

	CPlayer *pPlayer = pGameData->GetPlayerData(m_iPlayer);
	if( pPlayer == NULL )
	{
		LeaveCriticalSection( &cs );
		return;
	}
	// its busy researching
	if( pPlayer->GetRsrchItem() != 0 )
	{
		LeaveCriticalSection( &cs );
		return;
	}

	// get a topic to research
	iTopic = NextResearchTopic( pPlayer );
	LeaveCriticalSection( &cs );

	if( !iTopic )
		return;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::CheckResearch() for %d researching %d \n", 
pPlayer->GetPlyrNum(), iTopic );
#endif

	// start a new one
	CMsgRsrch msg( pPlayer->GetPlyrNum(), iTopic );
	theGame.PostToServer( &msg, sizeof(msg) );
}

//
// return the topic to be researched
//
int CAIGoalMgr::NextResearchTopic( CPlayer *pPlayer )
{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::NextResearchTopic() player %d is looking", m_iPlayer );
#endif

	// just go through the path assigned this AI
	for( int i=0; i<CRsrchArray::num_types; ++i )
	{
		// BUGBUG not yielding here because this called by the game

		int iTopic = m_iRDPath[i];
		if( !iTopic )
			continue;
		// this assumes that topic is discovered test occurs too
		if( pPlayer->CanRsrch(iTopic) )
		{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::NextResearchTopic() for %d is %d \n", m_iPlayer, iTopic );
#endif

			return( iTopic );
		}
	}
	return(0);
}



//
// check the current status of the player and make sure the
// current goals cover the player's needs, considering too
// this will get called alot, so make changes subltly
//
void CAIGoalMgr::CheckPlayer( void )
{
	EnterCriticalSection (&cs);

	CPlayer *pPlayer = pGameData->GetPlayerData(m_iPlayer);
	if( pPlayer == NULL )
	{
		LeaveCriticalSection (&cs);
		return;
	}
	// current power levels at this moment
	m_iPowerNeed = pPlayer->GetPwrNeed();
	m_iPowerHave = pPlayer->GetPwrHave();
	m_iGasHave = pPlayer->GetGasHave();
	m_iGasNeed = pPlayer->GetGasNeed();

	

	int iOfcCap = pPlayer->m_iOfcCap;
	int iOfcNeed = pPlayer->GetPplBldg();
	iOfcCap += iOfcCap >> 1;

	int iAptCap = pPlayer->m_iAptCap;
	int iAptNeed = pPlayer->GetPplTotal();
	iAptCap += iAptCap >> 1;

	LeaveCriticalSection (&cs);

	if( m_iPowerHave < m_iPowerNeed )
	{
		// get the amount of power, current Tasks will produce
		int iPowerTasks = GetPowerToBe();
		// absolute difference of need/have is amount
		int iPowerDiff = (m_iPowerNeed-m_iPowerHave);
		// less have than need is difference
		if( iPowerTasks < iPowerDiff )
			AddPowerTask();
	}

	// need gas
	if( m_iGasHave < m_iGasNeed )
	{
		// must already have an oil well
		if( m_pwaBldgs[CStructureData::oil_well] )
		{
			// and users of oil outnumber wells
			if( (m_pwaBldgs[CStructureData::refinery] +
				m_pwaBldgs[CStructureData::power_2]) >
				m_pwaBldgs[CStructureData::oil_well] )
			{
				// but limit number of oil wells based on difficulty
				if( m_pwaBldgGoals[CStructureData::oil_well]
					<= (pGameData->m_iSmart+1) )
					m_pwaBldgGoals[CStructureData::oil_well] = pGameData->m_iSmart+1;
			}

			if( m_pwaBldgs[CStructureData::refinery] ==
				m_pwaBldgGoals[CStructureData::refinery] )
			{
				if( m_pwaBldgs[CStructureData::refinery] <
					pGameData->m_iSmart &&
					m_pwaBldgs[CStructureData::refinery] <=
					((m_pwaBldgGoals[CStructureData::oil_well]+1)/2) )
				{
					m_pwaBldgGoals[CStructureData::refinery] += 1;
				}
			}
		}
	}

	// on difficult levels, if the command_center exists
	// then adjusts coal and iron to difficulty level
	if( pGameData->m_iSmart &&
		m_pwaBldgs[CStructureData::command_center] )
	{
		if( m_pwaBldgs[CStructureData::coal] <
			pGameData->m_iSmart )
		{
			// make adjustment only if count == goals
			if( m_pwaBldgs[CStructureData::coal] ==
				m_pwaBldgGoals[CStructureData::coal] )
				//ProduceMaterials(CMaterialTypes::coal);
				m_pwaBldgGoals[CStructureData::coal] = pGameData->m_iSmart+1;
		}
		if( m_pwaBldgs[CStructureData::iron] <
			pGameData->m_iSmart )
		{
			// make adjustment only if count == goals
			if( m_pwaBldgs[CStructureData::iron] ==
				m_pwaBldgGoals[CStructureData::iron] )
				//ProduceMaterials(CMaterialTypes::iron);
				m_pwaBldgGoals[CStructureData::iron] = pGameData->m_iSmart+1;
		}
	}

	// BUGBUG
	// temporarily make sure that certain buildings exist before
	// allowing the apts or offices buildings to be considered
	if( !m_pwaBldgs[CStructureData::power_1] ||
		!m_pwaBldgs[CStructureData::lumber] ||
		!m_pwaBldgs[CStructureData::smelter] ||
		!m_pwaBldgs[CStructureData::farm] ||
		!m_pwaBldgs[CStructureData::refinery] ||
		!m_pwaBldgs[CStructureData::coal] ||
		!m_pwaBldgs[CStructureData::iron] )
		return;

	// 
	// pPlayer->m_iOfcCap - is capcity of offices
	// pPlayer->GetPplBldg() - number of people need offices
	//
	// pPlayer->m_iAptCap - is capacity of apartments
	// pPlayer->GetPlpTotal() - number of people need apts
	//
	if( iOfcCap > iOfcNeed )
		m_iNeedOffice = FALSE;
	else
		m_iNeedOffice = TRUE;

	if( iAptCap > iAptNeed )
		m_iNeedApt = FALSE;
	else
		m_iNeedApt = TRUE;
}
//
// add a power plant task to an existing goal based on
// research level
//
void CAIGoalMgr::AddPowerTask()
{
	// make sure certain buildings exist before adding power task
	if( !m_pwaBldgs[CStructureData::power_1] ||
		!m_pwaBldgs[CStructureData::oil_well] ||
		!m_pwaBldgs[CStructureData::refinery] ||
		!m_pwaBldgs[CStructureData::lumber] ||
		!m_pwaBldgs[CStructureData::farm] ||
		!m_pwaBldgs[CStructureData::research] ||
		!m_pwaBldgs[CStructureData::smelter] ||
		!m_pwaBldgs[CStructureData::coal] ||
		!m_pwaBldgs[CStructureData::iron] )
		return;


#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::AddPowerTask() for %d ", m_iPlayer );
#endif

	CAIGoal *pGoal = m_plGoalList->GetGoal(IDG_EMERPOWER);
	if( pGoal == NULL )
	{
		AddGoal(IDG_EMERPOWER);
		m_bGoalChange = TRUE;
		UpdateConstructionTasks(NULL);
		//return;
	}
	
	int iTask = 0, iBldg;
	switch( m_iPowerLvl )
	{
		case 1:
			// add coal power plant
			iTask = IDT_BUILDPOWERPT1;
			iBldg = CStructureData::power_1;
			break;
		case 2:
			iTask = IDT_BUILDPOWERPT2;
			iBldg = CStructureData::power_2;
			break;
		case 3:
			iTask = IDT_BUILDPOWERPT3;
			iBldg = CStructureData::power_3;
			break;
		default:
			return;
	}

	// make sure that the goalmgr does not go overboard on power
	if( m_pwaBldgs[iBldg] > pGameData->m_iSmart )
		return;

	CAITask *pTask = m_plTasks->FindTask( iTask );
	if( pTask != NULL )
	{
		int iQty = pTask->GetTaskParam(PRODUCTION_QTY);
		iQty++;
		pTask->SetTaskParam(PRODUCTION_QTY,iQty);
		iQty = m_pwaBldgGoals[iBldg] + 1;
		m_pwaBldgGoals[iBldg] = iQty;
	}
}

//
// the material passed has been observed to be in
// insufficient quantities to meets needs for it
//
// review the status of existing buildings that produce
// that material, and consider if another instance of
// that building should be built, or if a supplier of
// the materials needed by that building is needed,
// or if there is a problem in moving materials to
// that building, such as needed to solve this shortage
//
void CAIGoalMgr::ProduceMaterials( int iMat )
{
	BOOL bBuild=FALSE;
	int iBldg = 0;
	WORD wTask,wGoal;
	CString sMat;

	// based on the material, set the type of building,
	// the id of the task to update, and the id of the
	// goal to add if the task does not exist in list
	switch( iMat )
	{
		case CMaterialTypes::lumber:
			iBldg = CStructureData::lumber;
			wTask = IDT_BUILDLUMBMILL;
			wGoal = IDG_ADDLUMBER;
			sMat = "lumber";
			break;
		case CMaterialTypes::steel:
			iBldg = CStructureData::smelter;
			wTask = IDT_BUILDSMELTER;
			wGoal = IDG_ADDSTEEL;
			sMat = "steel";
			break;
		case CMaterialTypes::copper:
			iBldg = CStructureData::copper;
			wTask = IDT_BUILDCOPPERMINE;
			wGoal = IDG_ADDCOPPER;
			sMat = "copper";
			break;
		case CMaterialTypes::food:
			iBldg = CStructureData::farm;
			wTask = IDT_BUILDFARM;
			wGoal = IDG_ADDFARM;
			sMat = "food";
			break;
		case CMaterialTypes::oil:
			iBldg = CStructureData::oil_well;
			wTask = IDT_BUILDOILWELL;
			wGoal = IDG_ADDOILWELL;
			sMat = "oil";
			break;
		case CMaterialTypes::gas:
			iBldg = CStructureData::refinery;
			wTask = IDT_BUILDREFINERY;
			wGoal = IDG_ADDREFINERY;
			sMat = "gas";
			break;
		case CMaterialTypes::coal:
			iBldg = CStructureData::coal;
			wTask = IDT_BUILDCOALMINE;
			wGoal = IDG_ADDCOAL;
			sMat = "coal";
			break;
		case CMaterialTypes::iron:
			iBldg = CStructureData::iron;
			wTask = IDT_BUILDIRONMINE;
			wGoal = IDG_ADDIRON;
			sMat = "iron";
			break;
		default:
			break;
	}
	// decide if a new building (a producer of that material)
	// should be added or not
	if( !AddNewProducer(iBldg,iMat) )
		return;

	CAITask *pTask = m_plTasks->FindTask( wTask );
	if( pTask != NULL )
	{
		int iQty = pTask->GetTaskParam(PRODUCTION_QTY);
		iQty++;
		pTask->SetTaskParam(PRODUCTION_QTY,iQty);


#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIGoalMgr::ProduceMaterials() player %d for %s ", 
    	m_iPlayer, (const char *)sMat );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"onhand=%d  need=%d  increased task %d to quantity %d ", 
		m_pwaMatOnHand[iMat], m_pwaMatGoals[iMat], wTask, iQty ); 
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"bldgs=%d  need=%d \n", m_pwaBldgs[iBldg], m_pwaBldgGoals[iBldg] ); 
#endif
	}
	else
	{
		AddGoal(wGoal);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIGoalMgr::ProduceMaterials() for %d ", m_iPlayer );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"onhand=%d  need=%d  added goal %d ", 
		m_pwaMatOnHand[iMat], m_pwaMatGoals[iMat], wGoal ); 
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"bldgs=%d  need=%d \n", m_pwaBldgs[iBldg], m_pwaBldgGoals[iBldg] ); 
#endif
	}
	m_bGoalChange = TRUE;
}

// BUGBUG this may not be handling food right
//
BOOL CAIGoalMgr::AddNewProducer( int iBldg, int iMat)
{
	// this must be here for testing cause it
	// keeps all materials but food from getting added
	//if( iMat != CMaterialTypes::food )
	//	return( FALSE );
	
	// also for testing?  forces an add
	// do add if there are no building goals
	if( !m_pwaBldgGoals[iBldg] )
		return( TRUE );

	// don't add if count of buildings < building goals
	if( m_pwaBldgs[iBldg] <
		m_pwaBldgGoals[iBldg] )
		return( FALSE );
	
	// don't add if needs are less than twice what we have
	// except for food which is considered to add when it
	// is detected as decreasing
	if( iMat != CMaterialTypes::food )
	{
		if( m_pwaMatGoals[iMat] <
			(m_pwaMatOnHand[iMat] * 2) )
			return( FALSE );
	}

	// at this point, we have existing buildings >= building goals,
	// and material needs are less than twice material onhand

	// if these building are under construction or waiting
	// materials, then don't add, otherwise add
    POSITION pos = m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );
			
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetType() == CUnit::building &&
				pUnit->GetTypeUnit() == iBldg )
			{
				BOOL bConstructing = FALSE;
				BOOL bWaiting = FALSE;
				EnterCriticalSection (&cs);
				CBuilding *pBldg = 
					theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					bConstructing = pBldg->IsConstructing();
					bWaiting = pBldg->IsWaiting();
				}
				LeaveCriticalSection (&cs);

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				if( bConstructing || bWaiting )
					return( FALSE );
			}
		}
	}

	// provide inference that no coal or iron mines are
	// added if a smelter does not yet exist
	if( iBldg == CStructureData::coal ||
		iBldg == CStructureData::iron )
	{
		if( !m_pwaBldgs[CStructureData::smelter] )
			return( FALSE );
	}

	// provide inference that if one of the critical
	// buildings do not yet exist, then no additional smelter
	if( iBldg == CStructureData::smelter )
	{
		if( !m_pwaBldgs[CStructureData::power_1] ||
			!m_pwaBldgs[CStructureData::oil_well] ||
			!m_pwaBldgs[CStructureData::refinery] ||
			!m_pwaBldgs[CStructureData::lumber] ||
			!m_pwaBldgs[CStructureData::farm] ||
			!m_pwaBldgs[CStructureData::research] ||
			!m_pwaBldgs[CStructureData::coal] ||
			!m_pwaBldgs[CStructureData::iron] )
			return( FALSE );
	}

	// now go thru list of tasks that can build this type of building
	// and if the task is unassigned don't add 
   	pos = m_plTasks->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
       	if( pTask != NULL )
       	{
       		ASSERT_VALID( pTask );

			// only want construction tasks
			if( pTask->GetTaskParam(ORDER_TYPE) 
				!= CONSTRUCTION_ORDER )
				continue;

			if( iBldg == pTask->GetTaskParam(BUILDING_ID) )
			{
				if( pTask->GetStatus() == UNASSIGNED_TASK )
				{
					if( !pTask->GetPriority() )
					{
						// get standard task data
						CAITask *pStdTask = 
							plTaskList->GetTask(pTask->GetID(),0);
						if( pStdTask == NULL )
							continue;
						// reset priority here to get faster assignment
						pTask->SetPriority( pStdTask->GetPriority() );
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::AddNewProducer() player %d for a %d building, goal %d task %d reset priority %d ", 
m_iPlayer, iBldg, pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::material %d  on-hand %d  needed %d ", 
iMat, m_pwaMatOnHand[iMat], m_pwaMatGoals[iMat] );
#endif
						m_pwaBldgGoals[iBldg]++;

					}
					// task now has, or did have, a priority
				
					return( FALSE );
				}
				// the task is assigned to a crane and a site has been selected
				if( pTask->GetTaskParam(BUILD_AT_X) ||
					pTask->GetTaskParam(BUILD_AT_Y) )
					return( FALSE );
				
				// if have less buildings built, than the task requests
				// then there is no reason to add more to the task
				if( m_pwaBldgs[iBldg] <
					pTask->GetTaskParam(PRODUCTION_QTY) )
					return( FALSE );

#if THREADS_ENABLED
	myYieldThread();
#endif
			}
		}
	}
	// adding one to count of number of this type to build
	return( TRUE );
}


void CAIGoalMgr::ProduceVehicles( int iVeh )
{
	int iBldg1=CStructureData::num_types;
	int iBldg2=CStructureData::num_types;
	WORD wTask=0, wGoal=0;

	switch( iVeh )
	{
		case CTransportData::construction:
			if( !NeedCranes() )
				return;
		case CTransportData::light_scout:
			iBldg1 = CStructureData::light_0;			
			iBldg2 = CStructureData::light_1;
			break;
		case CTransportData::heavy_scout:
		case CTransportData::med_tank:
			iBldg1 = CStructureData::light_2;
			iBldg2 = CStructureData::heavy;
			break;
		case CTransportData::med_scout:
		case CTransportData::infantry_carrier:
		case CTransportData::light_tank:
		case CTransportData::light_art:
			iBldg1 = CStructureData::light_1;
			iBldg2 = CStructureData::light_2;
			break;
		case CTransportData::med_art:
			iBldg1 = CStructureData::light_2;
			iBldg2 = CStructureData::heavy;
			break;
		case CTransportData::heavy_tank:
		case CTransportData::heavy_art:
			iBldg1 = CStructureData::heavy;
			iBldg2 = CStructureData::num_types;
			break;
		case CTransportData::light_cargo:
			iBldg1 = CStructureData::shipyard_1;
			iBldg2 = CStructureData::num_types;
			break;
		case CTransportData::destroyer:
		case CTransportData::cruiser:
			iBldg1 = CStructureData::shipyard_3;
			iBldg2 = CStructureData::num_types;
			break;
		case CTransportData::gun_boat:
		case CTransportData::landing_craft:
			iBldg1 = CStructureData::shipyard_1;
			iBldg2 = CStructureData::shipyard_3;
			break;
		case CTransportData::infantry:
		case CTransportData::rangers:
		case CTransportData::marines:
			iBldg1 = CStructureData::barracks_2;
			//iBldg2 = CStructureData::barracks_3;
			break;
		default:
			break;
	}
	if( iBldg1 == CStructureData::num_types &&
		iBldg2 == CStructureData::num_types )
		return;

	// decide if a new building (a producer of that vehicle)
	// should be added or not
	if( !AddNewFactory( iBldg1,iBldg2,wTask,wGoal) )
		return;

	CAITask *pTask = m_plTasks->FindTask( wTask );
	if( pTask != NULL )
	{
		int iQty = pTask->GetTaskParam(PRODUCTION_QTY);
		iQty++;
		pTask->SetTaskParam(PRODUCTION_QTY,iQty);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIGoalMgr::ProduceVehicles() player %d for a %d ", 
		m_iPlayer, iVeh );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"vehicles=%d  need=%d  increased task %d to quantity %d ", 
		m_pwaUnits[iVeh], m_pwaVehGoals[iVeh], wTask, iQty ); 
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"factories=%d  need=%d ", m_pwaBldgs[iBldg1], m_pwaBldgGoals[iBldg1] ); 
	if( iBldg2 < m_iNumBldgs )
		logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"factories=%d  need=%d ", m_pwaBldgs[iBldg2], m_pwaBldgGoals[iBldg2] ); 
#endif
	}
	else
	{
		AddGoal(wGoal);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIGoalMgr::ProduceVehicles() for %d ", m_iPlayer );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"vehicles=%d  need=%d  added goal %d \n", 
		m_pwaUnits[iVeh], m_pwaVehGoals[iVeh], wGoal );
#endif
	}

	// If an actual change was made
	m_bGoalChange = TRUE;
}

//
// consider if either of the 2 factory types need to be built
// in order to produce more of the vehicle type passed
// by considering if more goals exist for this type of factory
// than does exist in units, and if not, is one being built
// and if not, then return TRUE to cause the factory construction
// tasks to be increase or a goal to add the construction of those
// types of factories
//
BOOL CAIGoalMgr::AddNewFactory( int iBldg1, int iBldg2, 
	WORD& wTask, WORD& wGoal )
{
	// don't add if count of buildings < building goals
	if( iBldg1 < CStructureData::num_types )
	{
		if( m_pwaBldgs[iBldg1] <
			m_pwaBldgGoals[iBldg1] )
			return( FALSE );
	}
	if( iBldg2 < CStructureData::num_types )
	{
		if( m_pwaBldgs[iBldg2] <
			m_pwaBldgGoals[iBldg2] )
			return( FALSE );
	}

	// if these building are under construction or waiting
	// materials, then don't add, otherwise add
    POSITION pos = m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );
			
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetType() == CUnit::building &&
				(pUnit->GetTypeUnit() == iBldg1 ||
				 pUnit->GetTypeUnit() == iBldg2) )
			{
				BOOL bConstructing = FALSE;
				BOOL bWaiting = FALSE;
				EnterCriticalSection (&cs);
				CBuilding *pBldg = 
					theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					bConstructing = pBldg->IsConstructing();
					bWaiting = pBldg->IsWaiting();

					// consider if it is a factory, and if so, if it is
					// currently not producing a vehicle, then no add
					if( !bConstructing && !bWaiting )
					{
						CBuildUnit const *pBuildVeh = NULL;

						if( pBldg->GetData()->GetUnionType() == 
							CStructureData::UTvehicle )
						{
							CVehicleBuilding *pVehBldg =
								(CVehicleBuilding *)pBldg;
							pBuildVeh = pVehBldg->GetBldUnt();
							if( pBuildVeh == NULL )
								bWaiting = TRUE;
						}
						else if( pBldg->GetData()->GetUnionType() == 
							CStructureData::UTshipyard )
						{
							CShipyardBuilding *pShipBldg = 
								(CShipyardBuilding*)pBldg;
							pBuildVeh = pShipBldg->GetBldUnt();
							if( pBuildVeh == NULL )
								bWaiting = TRUE;
						}
					}
				}
				LeaveCriticalSection (&cs);

				if( bConstructing || bWaiting )
					return( FALSE );
			}
		}
	}


	// now go thru list of tasks that can build these types 
	// of buildings and if the task is unassigned don't add 
   	pos = m_plTasks->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
       	if( pTask != NULL )
       	{
       		ASSERT_VALID( pTask );

			if( iBldg1 == pTask->GetTaskParam(BUILDING_ID) ||
				iBldg2 == pTask->GetTaskParam(BUILDING_ID) )
			{
				int iBldg = pTask->GetTaskParam(BUILDING_ID);

				if( pTask->GetStatus() == UNASSIGNED_TASK )
				{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
					if( !pTask->GetPriority() )
					{
						// get standard task data
						CAITask *pStdTask = 
							plTaskList->GetTask(pTask->GetID(),0);
						if( pStdTask == NULL )
							continue;
						// reset priority here to get faster assignment
						pTask->SetPriority( pStdTask->GetPriority() );
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIGoalMgr::player %d AddNewFactory() construction goal %d task %d priority %d ", 
	m_iPlayer, pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
#endif
					}
					// task now has a priority
				
					return( FALSE );
				}
				// the task is assigned to a crane and a site has been selected
				if( pTask->GetTaskParam(BUILD_AT_X) ||
					pTask->GetTaskParam(BUILD_AT_Y) )
					return( FALSE );
				
				// if have less buildings built, than the task requests
				// then there is no reason to add more to the task
				if( m_pwaBldgs[iBldg] <
					pTask->GetTaskParam(PRODUCTION_QTY) )
					return( FALSE );

				// pass back the goal and task
				wGoal = pTask->GetGoalID();
				wTask = pTask->GetID();
			}
		}
	}
	// adding one to count of number of this type to build
	return( TRUE );
}


//
// if the message is a veh_new that indicates a placement
// instead of a production completion, then the task that
// produces that type of vehicle needs to have its param
// of PRODUCTION_QTY updated
//
void CAIGoalMgr::SetStartVehicles( CAIMsg *pMsg )
{
	if( pMsg->m_idata3 != m_iPlayer )
		return;
		
	if( pMsg->m_idata1 < m_iNumUnits )
	{
		// for each task
    	POSITION pos = m_plTasks->GetHeadPosition();
    	while( pos != NULL )
    	{   
        	CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
        	if( pTask != NULL )
        	{
        		ASSERT_VALID( pTask );
				if( pTask->GetTaskParam(ORDER_TYPE)
					== PRODUCTION_ORDER &&
					pTask->GetTaskParam(PRODUCTION_TYPE)
					== CStructureData::UTvehicle &&
					(int)pTask->GetTaskParam(PRODUCTION_ITEM)
					== pMsg->m_idata1 )
				{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
					int iCnt = 
						(int)pTask->GetTaskParam(PRODUCTION_QTY) + 1;
					pTask->SetTaskParam(PRODUCTION_QTY, (WORD)iCnt);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIGoalMgr::SetStartVehicles for player %d ", m_iPlayer );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"task %d goal %d producing %d quantity %d ", 
		pTask->GetID(), pTask->GetGoalID(),
		pTask->GetTaskParam(PRODUCTION_ITEM), pTask->GetTaskParam(PRODUCTION_QTY) );
#endif

					return;
				}
			}
		}
	}
}

//
// determine the number of vehicles to build of this type
//
int CAIGoalMgr::GetNumVehToBuild( int iVehType )
{
	if( iVehType < m_iNumUnits )
	{
		// first determine if more than one of these factory
		// types exist

		// consider building the number of vehicles needed
		// to reach the goals for this type of vehicle
		if( m_pwaUnits[iVehType] < m_pwaVehGoals[iVehType] )
		{
			int iCnt = m_pwaVehGoals[iVehType] - m_pwaUnits[iVehType];
			//iCnt >>= 1; // build 1/2 the remaining to the goal
			if( !iCnt )
				iCnt = 1;

			if( iCnt > pGameData->m_iSmart )
				iCnt = pGameData->m_iSmart + 1;
			
			return( iCnt );
		}
		else
			return(1);
	}
	return(0);
}

//
// go thru tasks and add up the power that can be produced
// if all the build power plant tasks were completed
//
int CAIGoalMgr::GetPowerToBe()
{
	int iPower = 0;
	// for each task
   	POSITION pos = m_plTasks->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
       	if( pTask != NULL )
       	{
       		ASSERT_VALID( pTask );
			// IDT_BUILDPOWERPT1
			// IDT_BUILDPOWERPT2
			// IDT_BUILDPOWERPT3
			if( pTask->GetID() == IDT_BUILDPOWERPT1 ||
				pTask->GetID() == IDT_BUILDPOWERPT2 ||
				pTask->GetID() == IDT_BUILDPOWERPT3 )
			{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				// how to get power produced?
				// get bldg for the task
				int iBldg = pTask->GetTaskParam(BUILDING_ID);
				int iQty = pTask->GetTaskParam(PRODUCTION_QTY);

				// look it up in structures
				CStructureData const *pBldgData = 
				pGameData->GetStructureData( iBldg );
				if( pBldgData != NULL )
				{
					// now get pointer to the CBuild* instance of building
					CBuildPower *pPowerBldg = pBldgData->GetBldPower();
					if( pPowerBldg != NULL )
					{
						// find power output 
						if( iQty )
							iPower += (pPowerBldg->GetPower() * iQty);
						else
							iPower += pPowerBldg->GetPower();
					}
				}
			}
		}
	}
	return( iPower );
}

void CAIGoalMgr::GetVehicleNeeds()
{
	// needs of vehicles
	// based on a ratio for each vehicle type determined
	// by the attribs for this player
	if( m_pwaVehGoals != NULL )
		ClearVehGoals();
	else
		InitVehGoals();

	UpdateVehGoals();
}

//
// go thru the tasks and count up what the total number of vehicles
// of each type, that the manager thinks it should have
//
void CAIGoalMgr::UpdateVehGoals( void )
{
	// clear count of goals based on tasks
	for( int i=0; i<m_iNumBldgs; ++i )
	{
		m_pwaBldgGoals[i] = 0;
		m_pwaBldgs[i] = 0;
		
	}
	for( i=0; i<m_iNumUnits; ++i )
	{
		m_pwaVehGoals[i] = 0;
		m_pwaUnits[i] = 0;
	}

#if THREADS_ENABLED
	myYieldThread();
#endif

	// go thru the units and update the counts by type
    POSITION pos = m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );
			
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			i = pUnit->GetTypeUnit();

			if( pUnit->GetType() == CUnit::building )
			{
				if( i < m_iNumBldgs )
					m_pwaBldgs[i]++;
			}
			else if( pUnit->GetType() == CUnit::vehicle )
			{
				if( i < m_iNumUnits )
					m_pwaUnits[i]++;
			}
		}
	}

#if THREADS_ENABLED
	myYieldThread();
#endif

	// now go thru all tasks of this manager
    pos = m_plTasks->GetHeadPosition();
    while( pos != NULL )
    {   
		// this manager's task list
        CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

			// now based on the task, determine
			// if construction or production
			// is being ordered
			if( pTask->GetType() != ORDER_TASK )
				continue;

			// if production is ordered
			// then update m_pwaVehGoals
			if( pTask->GetTaskParam(ORDER_TYPE)
				== PRODUCTION_ORDER &&
				pTask->GetTaskParam(PRODUCTION_TYPE)
				== CStructureData::UTvehicle )
			{
				int iVeh = 
					(int)pTask->GetTaskParam(PRODUCTION_ITEM);
				if( iVeh < m_iNumUnits )
				{
					WORD wCnt = m_pwaVehGoals[iVeh];
					// producing stuff can be in multiple quantities
					wCnt += pTask->GetTaskParam(PRODUCTION_QTY);

					m_pwaVehGoals[iVeh] = wCnt;
				}
			}
			else if( pTask->GetTaskParam(ORDER_TYPE)
				== CONSTRUCTION_ORDER &&
				pTask->GetOrderID() == CNetCmd::build_bldg )
			{
				int iBldg =
					(int)pTask->GetTaskParam(BUILDING_ID);
				WORD wCnt = m_pwaBldgGoals[iBldg];

				// some buildings have a quantity
				//if( pTask->GetTaskParam(PRODUCTION_QTY) )
				//	wCnt += pTask->GetTaskParam(PRODUCTION_QTY);
				//else
					wCnt++;
				m_pwaBldgGoals[iBldg] = wCnt;
			}
		}
	}

#if THREADS_ENABLED
	myYieldThread();
#endif

	int iCnt, iRatio, iOpfor, iAgress;

	iRatio = pGameData->m_iSmart;
	iCnt = pGameData->m_iSmart + 1;

	// force the R&D and oil_wells to be difficulty level
	m_pwaBldgGoals[CStructureData::research] = pGameData->m_iSmart;
	if( !m_pwaBldgGoals[CStructureData::research] )
		m_pwaBldgGoals[CStructureData::research] = 1;

	m_pwaBldgGoals[CStructureData::oil_well] = iCnt;
	m_pwaBldgGoals[CStructureData::iron] = iCnt;
	m_pwaBldgGoals[CStructureData::coal] = iCnt;

	// qualified increase in smelters
	if( m_pwaBldgs[CStructureData::iron] >= 
		m_pwaBldgGoals[CStructureData::iron] &&
		m_pwaBldgs[CStructureData::coal] >=
		m_pwaBldgGoals[CStructureData::coal] )
		m_pwaBldgGoals[CStructureData::smelter] += (iCnt/2);

	// never need more than one rep fac
	m_pwaBldgGoals[CStructureData::repair] = 1;

	// force the forts and combat factorys to be a factor of difficulty and defend
	
	m_pwaBldgGoals[CStructureData::fort_1] += (iCnt +
		pGameData->GetRandom(iCnt) );
	m_pwaBldgGoals[CStructureData::fort_2] += (iCnt +
		pGameData->GetRandom(iRatio) );
	m_pwaBldgGoals[CStructureData::fort_3] += iCnt;

	//m_pwaBldgGoals[CStructureData::barracks_2] += iRatio;
	if( m_pwaBldgGoals[CStructureData::light_2] < iRatio )
		m_pwaBldgGoals[CStructureData::light_2] = (iCnt/2) + 1;
	if( m_pwaBldgGoals[CStructureData::heavy] < iRatio )
		m_pwaBldgGoals[CStructureData::heavy] = (iCnt/2);



	if( m_pwaAttribs[CRaceDef::defense] >= 90 )
	{
		iRatio = m_pwaAttribs[CRaceDef::defense] - 90;
		if( iRatio >= 10 )
			iRatio /= 10;
		else
			iRatio = pGameData->m_iSmart;

		m_pwaBldgGoals[CStructureData::fort_1] += iRatio;
	}

	// calculate an agressive factor
	if( m_pwaAttribs[CRaceDef::attack] >= 90 )
	{
		iAgress = iRatio;

		iRatio = m_pwaAttribs[CRaceDef::attack] - 90;
		if( iRatio >= 10 )
			iRatio /= 10;
		else 
			iRatio = pGameData->m_iSmart;

		iAgress += iRatio;

		if( m_pwaAttribs[CRaceDef::manf_vehicles] >= 90 )
		{
			if( iRatio >= 10 )
				iRatio /= 10;
			else 
				iRatio = pGameData->m_iSmart;

			iAgress += iRatio;
		}
	}
	else
		iAgress = pGameData->m_iSmart;


	// now vehicle goal counts are plain, unadjusted counts
	// of number of production orders for various vehicles

	// now adjust the goal counts by applying attributes 
	// and opfor unit counts as influences
	
	for( i=0; i<m_iNumUnits; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		// get attribute ratio
		iRatio = m_pwaRatios[i];
		iCnt = m_pwaVehGoals[i];

		// apply ratio to normal goal count
		m_pwaVehGoals[i] = (iCnt + iRatio);

		// add average count of opfor vehicles of this type
		iOpfor = m_plOpFors->GetVehicleCnt(i,TRUE);
		// don't allow 0 of this vehicle to hurt the goal count
		if( m_pwaVehGoals[i] && !iOpfor )
			iOpfor = 1;
		if( m_pwaVehGoals[i] < iOpfor )
		{
			iCnt = m_pwaVehGoals[i];
			iCnt += (iOpfor * pGameData->m_iSmart);
			m_pwaVehGoals[i] += iCnt;
		}

		// add bump for combat vehicles
		if( pGameData->IsCombatVehicle(i) )
		{
			iCnt = iAgress + pGameData->GetRandom( pGameData->m_iSmart );

			if( i == CTransportData::infantry_carrier ||
				i == CTransportData::infantry ||
				i == CTransportData::rangers )
				iCnt /= 2;
			else
				iCnt *= (pGameData->m_iSmart + 1);

			m_pwaVehGoals[i] += iCnt;
		}
	}

	iCnt = pGameData->m_iSmart + 1;
	// adjust production of trucks once weapons factorys built
	if( m_pwaBldgs[CStructureData::light_2] >=
		m_pwaBldgGoals[CStructureData::light_2] )
		m_pwaVehGoals[CTransportData::heavy_truck] = (4 * iCnt);
	else
		m_pwaVehGoals[CTransportData::heavy_truck] = (3 * iCnt);

	// stop over production of trucks
	if( m_pwaVehGoals[CTransportData::heavy_truck] > (4 * iCnt) )
		m_pwaVehGoals[CTransportData::heavy_truck] = (4 * iCnt);

	// stop over production of cranes
	if( m_pwaVehGoals[CTransportData::construction] > (2 * iCnt) )
	{
		m_pwaVehGoals[CTransportData::construction] = (2 * iCnt);
	}
		
	// increase gunboats and destroyers
	if( m_bOceanWorld || m_bLakeWorld )
	{
		m_pwaVehGoals[CTransportData::gun_boat] += (4 * iCnt);
		m_pwaVehGoals[CTransportData::destroyer] += (2 * iCnt);
	}
}


// m_pwaMatGoals = how much of each material is needed
//
// m_pwaMatOnHand = how much of each material is on hand
//
// determine commodity quantities needed for
// current production in buildings
// and store them in CAIGoalMgr::m_pwaMatGoals
//
// calculate on-hand quantities of commodities
// available for use in construction/production
// and store them in CAIGoalMgr::m_pwaMatOnHand
//
void CAIGoalMgr::GetCommodityNeeds( void )
{
	// initialize arrays to start
	if( m_pwaMatOnHand != NULL )
		ClearMatOnHand(); //DelMatOnHand();
	else
		InitMatOnHand();

	if( m_pwaMatGoals != NULL )
		ClearMatGoals(); //DelMatGoals();
	else
		InitMatGoals();
	
	// for each building
	//   sum up commodities needed for production
	//   regardless of what is on-hand at building
	//   and sum up commodities in storage
	// for each vehicle
	//   sum up commodities enroute
    POSITION pos = m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );
			
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetType() == CUnit::building )
			{

				// get access to CBuilding copied data for building
				CAICopy *pCopyCBuilding = 
					pUnit->GetCopyData( CAICopy::CBuilding );
				if( pCopyCBuilding == NULL )
					continue;

				// determine what is in construction/production
				if( pCopyCBuilding->m_aiDataOut[CAI_ISCONSTRUCTING] )
				{
					// this building is under construction

					// get pointer to CStructureData copy data
					CAICopy *pCopyCStructureData = 
						pUnit->GetCopyData( CAICopy::CStructureData );
					if( pCopyCStructureData != NULL )
					{
						// add the materials needed to material goals
						for( int i=0; i<CMaterialTypes::num_build_types; ++i )
						{
#if 0 //THREADS_ENABLED
							// BUGBUG this function must yield
							myYieldThread();
							//if( myYieldThread() == TM_QUIT )
							//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
							WORD wCnt =
								m_pwaMatGoals[i] +
								(WORD)pCopyCStructureData->m_aiDataIn[i];
							m_pwaMatGoals[i] = wCnt;
						}
					}
				}

				// does this building produce materials?
				if( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == 
					CStructureData::UTmaterials )
				{
						// get pointer to CBuildMaterials copy data
						CAICopy *pCopyCBuildMaterials = 
							pUnit->GetCopyData( CAICopy::CBuildMaterials );
						if( pCopyCBuildMaterials == NULL )
							continue;

						// go thru materials needed, and record needs
						for( int i=0; i<CMaterialTypes::num_types; ++i )
						{
#if 0 //THREADS_ENABLED
							// BUGBUG this function must yield
							myYieldThread();
							//if( myYieldThread() == TM_QUIT )
							//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
							// add the materials needed to material goals
							WORD wCnt =
								m_pwaMatGoals[i] +
								(WORD)pCopyCBuildMaterials->m_aiDataIn[i];
							m_pwaMatGoals[i] = wCnt;

							// add the materials produced to on-hand
							wCnt = m_pwaMatOnHand[i] +
								(WORD)pCopyCBuildMaterials->m_aiDataOut[i];
							m_pwaMatOnHand[i] = wCnt;
						}
				}
				// or this building produces vehicles?
				else if( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == 
					CStructureData::UTvehicle )
				{
						// get pointer to CBuildUnit copy data
						CAICopy *pCopyCBuildUnit = 
							pUnit->GetCopyData( CAICopy::CBuildUnit );
						if( pCopyCBuildUnit == NULL )
							continue;

						// go thru materials needed, and record needs
						for( int i=0; i<CMaterialTypes::num_types; ++i )
						{
#if 0 //THREADS_ENABLED
							// BUGBUG this function must yield
							myYieldThread();
							//if( myYieldThread() == TM_QUIT )
							//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
							// update materials needed for vehicle
							WORD wCnt =
								m_pwaMatGoals[i] +
								(WORD)pCopyCBuildUnit->m_aiDataIn[i];
							m_pwaMatGoals[i] = wCnt;
						}
				}
				else if( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == 
					CStructureData::UTpower )
				{
						// get pointer to the CStructureData type for this unit
						CStructureData const *pBldgData = 
							pGameData->GetStructureData( 
							pCopyCBuilding->m_aiDataOut[CAI_TYPEBUILD] );
						if( pBldgData != NULL )
						{
							// now get pointer to the CBuild* instance of building
							CBuildPower *pPowerBldg = pBldgData->GetBldPower();
							if( pPowerBldg == NULL )
								continue;

							// it uses some material for power
							if( pPowerBldg->GetInput() >= 0 )
							{
								// get id of material needed for power
								int i = pPowerBldg->GetInput();

								// try get an hours's worth
								m_pwaMatGoals[i] += 
									(int)((86400L / (LONG)pPowerBldg->GetRate()) / 24);
								//(int)((86400L / (LONG)pPowerBldg->GetRate()) * 24) / 24;
							}
						}
				}
#if 0
				else if( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == 
					CStructureData::UTfarm )
				{
					// get pointer to the CStructureData type for this unit
					CStructureData const *pBldgData = 
						pGameData->GetStructureData( 
						pCopyCBuilding->m_aiDataOut[CAI_TYPEBUILD] );
					if( pBldgData != NULL )
					{
						CBuildFarm *pFarmBldg = pBldgData->GetBldFarm();
						if( pFarmBldg == NULL )
							continue;

						int i = pFarmBldg->GetTypeFarm();
						m_pwaMatGoals[i] += 
						pFarmBldg->GetQuantity() * 60;
					}
				}
#endif
				
				// add the materials in storage to on hand
				for( int i=0; i<m_iNumMats; ++i )
				{
					WORD wCnt = m_pwaMatOnHand[i] +
						(WORD)pCopyCBuilding->m_aiDataIn[i];
					m_pwaMatOnHand[i] = wCnt;
				}

			}
			else if( pUnit->GetType() == CUnit::vehicle )
			{
				// get access to CVehicle copied data for vehicle
				CAICopy *pCopyCVehicle = 
					pUnit->GetCopyData( CAICopy::CVehicle );
				if( pCopyCVehicle == NULL )
					continue;

				// add the materials on board to on hand
				for( int i=0; i<m_iNumMats; ++i )
				{
					WORD wCnt = m_pwaMatOnHand[i] +
						(WORD)pCopyCVehicle->m_aiDataIn[i];
					m_pwaMatOnHand[i] = wCnt;
				}
			}
		}
	}
	// end of each building
	// end of each vehicle

	// handle food separatly, if it is decreasing, then
	// consider producing a farm.  may have already decided
	// that, so once a farm is scheduled, let it get built

	int iFood = 0;
	CPlayer *pPlayer = pGameData->GetPlayerData(m_iPlayer);
	if( pPlayer != NULL )
	{
		// needs per minute
		m_pwaMatGoals[CMaterialTypes::food] = pPlayer->GetFoodNeed();
		// production per minute
		m_pwaMatOnHand[CMaterialTypes::food] =  pPlayer->GetFoodProd();
		// actual on hand
		iFood = pPlayer->GetFood();
	}
	else
		return;

	// food production is lower than consumption
	if( m_pwaMatGoals[CMaterialTypes::food] >
		m_pwaMatOnHand[CMaterialTypes::food] )
	{
		// if actual on-hand is > than 2 hour's production then
		// the need to build a farm is reduced
		if( (m_pwaMatOnHand[CMaterialTypes::food] * 120) < iFood )
		{
			// needs == onhand will deter farm construction
			m_pwaMatGoals[CMaterialTypes::food] = 
			m_pwaMatOnHand[CMaterialTypes::food];
		}
		else
		{
			// increase the difference to encourage farm construction
			m_pwaMatGoals[CMaterialTypes::food] = 
			m_pwaMatOnHand[CMaterialTypes::food] * 120;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetCommodityNeeds: need food for player %d ", m_iPlayer );
#endif
		}
	}
}

WORD CAIGoalMgr::GetVehGoals( WORD wOffset )
{
	ASSERT_VALID( this );
	if( m_pwaVehGoals != NULL )
	{
		//ASSERT_VALID( m_pwaVehGoals );
		ASSERT( (int)wOffset < m_iNumUnits );
		
		if( (int)wOffset < m_iNumUnits )
			return( m_pwaVehGoals[wOffset] );
		else
		{
			WORD wCnt = 0;
			for( int i=0; i<m_iNumUnits; ++i)
				wCnt += m_pwaVehGoals[i];
			return( wCnt );
		}
	}
	return( FALSE );
}

//
// return the count of vehicles indicated
// by the offset passed
//
WORD CAIGoalMgr::GetVehicle( WORD wOffset )
{
	ASSERT_VALID( this );
	if( m_pwaUnits != NULL )
	{
		//ASSERT_VALID( m_pwaUnits );
		ASSERT( (int)wOffset < m_iNumUnits );
		
		if( (int)wOffset < m_iNumUnits )
			return( m_pwaUnits[wOffset] );
		else
		{
			WORD wCnt = 0;
			for( int i=0; i<m_iNumUnits; ++i)
			{
#if 0 //THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				wCnt += m_pwaUnits[i];
			}
			return( wCnt );
		}
	}
	return( FALSE );
}
// BUGBUG: This needs to consider that the specific vehicle
//         or building needs to only be counted once
//
// update count of vehicles at offset passed
//
void CAIGoalMgr::SetVehicle( WORD wOffset, WORD wCnt )
{
	ASSERT_VALID( this );
	if( m_pwaUnits != NULL )
	{
		//ASSERT_VALID( m_pwaUnits );
		ASSERT( (int)wOffset < m_iNumUnits );
		
		if( (int)wOffset < m_iNumUnits )
			m_pwaUnits[wOffset] = wCnt;
	}
}

//
// return the count of buildings indicated
// by the offset passed
//
WORD CAIGoalMgr::GetBuilding( WORD wOffset )
{
	ASSERT_VALID( this );
	if( m_pwaBldgs != NULL )
	{
		//ASSERT_VALID( m_pwaBldgs );
		ASSERT( (int)wOffset < m_iNumBldgs );
		
		if( (int)wOffset < m_iNumBldgs )
			return( m_pwaBldgs[wOffset] );
		else
		{
			WORD wCnt = 0;
			for( int i=0; i<m_iNumBldgs; ++i)
			{
#if 0 //THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				wCnt += m_pwaBldgs[i];
			}
			return( wCnt );
		}
	}
	return( FALSE );
}


// BUGBUG: This needs to consider that the specific vehicle
//         or building needs to only be counted once
//
// update count of buildings at offset passed
//
void CAIGoalMgr::SetBuilding( WORD wOffset, WORD wCnt )
{
	ASSERT_VALID( this );
	if( m_pwaBldgs != NULL )
	{
		//ASSERT_VALID( m_pwaBldgs );
		ASSERT( (int)wOffset < m_iNumBldgs );
		
		if( (int)wOffset < m_iNumBldgs )
			m_pwaBldgs[wOffset] = wCnt;
	}
}

//
// review task list, consider current state and message
// and make adjustments to task priorities
//
/*
	unit_damage,		// notify of damage level
						// sent from server
	bldg_new,			// a new building is created
	bldg_stat,			// built/damage status of a bldg
	veh_new,			// a new vehicle is created
	veh_stat,			// damage status of a vehicle
	veh_loc,			// vehicle going to the next hex
	veh_dest,			// vehicle has arrived at its destination
	road_new,			// start a new road hex
	road_done,			// a new road hex is completed
	unit_set_damage,	// received damage level
	unit_abandoned,		// a unit has been abandoned
	unit_destroying,	// a unit is being destroyed
	delete_unit,		// a unit is gone
*/
void CAIGoalMgr::UpdateTaskPriorities( CAIMsg *pMsg )
{
	BOOL bRunConstructionUpdate = CheckAbandonedBuildings();
	ConsiderRoads();

	// if message involves a building status change
	// it could be a newly completed building or
	// a building stoped for materials or
	// just been damaged (again) or destroyed
	//
	// BUGBUG how do we know this is our building?
	//
	if( pMsg->m_iMsg == CNetCmd::bldg_stat &&
		pMsg->m_idata3 == m_iPlayer )
	{
		// the building was completed
		if( pMsg->m_uFlags == CMsgBldgStat::built &&
		    pMsg->m_idata2 == 100 )
		{
			UpdateConstructionTasks(pMsg);
			bRunConstructionUpdate = FALSE;
		}
		// the building is out of materials for producing
		// what it is currently producing
		else if( pMsg->m_uFlags == CMsgBldgStat::out_mat )
		{
	 		UpdateProductionTasks(pMsg);
		}
	}

	// a building was placed for construction
	if( pMsg->m_iMsg == CNetCmd::bldg_new &&
		pMsg->m_idata3 == m_iPlayer )
	{
		UpdateConstructionTasks(pMsg);
		bRunConstructionUpdate = FALSE;

		UpdateProductionTasks(pMsg);
	}

	// play or road or bridge then update only construction tasks
	if( pMsg->m_iMsg == CNetCmd::road_done ||
		pMsg->m_iMsg == CNetCmd::cmd_play )
	{
		UpdateConstructionTasks(pMsg);
		bRunConstructionUpdate = FALSE;
	}

	// if message reports attacked
	// then update only combat production tasks
	// and combat directive tasks
	//
	// BUGBUG how do we know this is our unit?
	//
	if( pMsg->m_iMsg == CNetCmd::unit_damage &&
		pMsg->m_idata3 == m_iPlayer )
	{
		UpdateCombatTasks(pMsg);
		//UpdateInfoTasks(pMsg);
	}

	// if message reports a vehicle completed
	if( pMsg->m_iMsg == CNetCmd::veh_new &&
		pMsg->m_idata3 == m_iPlayer )
	{
		UpdateProductionTasks(pMsg);
		UpdateCombatTasks(pMsg);
		//UpdateInfoTasks(pMsg);
	}

	// an AI unit has been destroyed
	if( pMsg->m_iMsg == CNetCmd::delete_unit &&
		pMsg->m_idata3 == m_iPlayer )
	{
		UpdateProductionTasks(pMsg);
		UpdateInfoTasks(pMsg);
	}

	if( pMsg->m_iMsg == CNetCmd::bldg_new &&
		pMsg->m_idata3 != m_iPlayer )
	{
		UpdateCombatTasks(pMsg);
		//UpdateInfoTasks(pMsg);
	}

	// a scenario has started
	if( pMsg->m_iMsg == CNetCmd::scenario &&
		pMsg->m_idata3 == m_iPlayer )
	{
		UpdateConstructionTasks(pMsg);
		bRunConstructionUpdate = FALSE;

		UpdateProductionTasks(pMsg);
		UpdateCombatTasks(pMsg);
	}

	if( bRunConstructionUpdate )
		UpdateConstructionTasks(pMsg);
}
//
// usually only called with a delete_unit message and
// will update INFO_TASK tasks, if that unit was assigned
// to an INFO_TASK task
//
void CAIGoalMgr::UpdateInfoTasks( CAIMsg *pMsg )
{
	if( pMsg->m_iMsg == CNetCmd::delete_unit &&
		pMsg->m_idata3 == m_iPlayer )
	{
		// m_dwID contains that unit
		// get its pointer and update
		CAIUnit *pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
		if( pUnit == NULL )
			return;

		POSITION pos = m_plTasks->GetHeadPosition();
		while( pos != NULL )
		{   
       		CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
			if( pTask != NULL )
			{
        		ASSERT_VALID( pTask );

				if( pTask->GetType() != INFO_TASK )
					continue;

				if( pTask->GetID() != pUnit->GetTask() ||
					pTask->GetGoalID() != pUnit->GetGoal() )
					continue;

				// get standard task data
				CAITask *pStdTask = 
					plTaskList->GetTask(pTask->GetID(),0);
				if( pStdTask == NULL )
					continue;

#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

				// make these tasks available for assignment
				if( pTask->GetID() == IDT_FINDOPFORCITY ||
					pTask->GetID() == IDT_FINDOPFORS )
				{
					pTask->SetPriority( pStdTask->GetPriority() );
					pTask->SetTaskParam(TO_MATERIAL_ID,0);
					pTask->SetStatus( UNASSIGNED_TASK );
				}

				// update count of scouts assigned
				if( pTask->GetID() == IDT_SCOUT )
				{
					WORD wCnt = 
						pTask->GetTaskParam(TO_MATERIAL_ID);
					if( wCnt )
					{
						pTask->SetTaskParam(TO_MATERIAL_ID,(wCnt-1));
						pTask->SetPriority( pStdTask->GetPriority() );
					}
				}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::UpdateInfoTasks() player %d info task %d of goal %d has priority of %d \n",
m_iPlayer, pTask->GetID(), pTask->GetGoalID(), pTask->GetPriority() );
#endif

			}
		}
	}
}


//
//
//
void CAIGoalMgr::UpdateCombatTasks( CAIMsg * /*pMsg*/ )
{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::UpdateCombatTasks() for player %d ", m_iPlayer );
#endif

	// there could be up to 4 IDT_PREPAREWAR tasks in the list
	// each in various stages of staging and/or assaulting
	//
	// if task has not completely staged, continue with its
	// priority or consider raising it
	//
	// if no staging tasks then all vehicles patrol
	// else vehicles go to staging first
	//
	// some staging tasks should get priority over others
	//
	// when a staging first starts, go look at patroling vehicles
	// being assigned to staging
	//
	// vehicles that are staged for a given staging task will have
	// dest and current hex equal, same goal and task, and occupying
	// a hex in the task staging area, so first time, flag the unit
	// as staged, and just check that on all other passes
	//
	// for non-staged tasks: IDG_ADVDEFENSE < IDG_LANDWAR
	//                       IDG_PIRATE < IDG_SEAINVADE
	//                       IDG_LANDWAR == IDG_SEAINVADE
	// in priority 
	//
	// if a task is unassigned then 
	//    if the task has no priority, then based on goal, set it
	//    else task has a priority so leave it alone
	// else if task is INPROCESS_TASK then it has staged at least once
	//    and needs priority set to zero
	// else if task is COMPLETED_TASK then it has launched assault
	//    and needs priority set to zero
	//    

    POSITION pos = m_plTasks->GetHeadPosition();
    while( pos != NULL )
    {   
       	CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
       	if( pTask != NULL )
       	{
       		ASSERT_VALID( pTask );

			if( pTask->GetID() == IDT_PREPAREWAR )
			{
				// if a staging task is not UNASSIGNED_TASK
				// then it has staged once or launched assault
				if( pTask->GetStatus() != UNASSIGNED_TASK )
				{
					pTask->SetPriority( (BYTE)0 );
				}
				else // a task is still staging or about to stage
				{
					// only zero priority tasks need to be considered
					// for increased priority
					//
					if( !pTask->GetPriority() )
					{
						// some non-combat need has priority
						//
						if( !m_bGunsOrButter )
							continue;

						// get standard task data and reset priority
						CAITask *pStdTask = 
						plTaskList->GetTask(pTask->GetID(),0);
						if( pStdTask != NULL )
						{
							// once into game, these goals will be encouraged
							// by increasing the priority of their staging task
							if( pTask->GetGoalID() == IDG_LANDWAR ||
								pTask->GetGoalID() == IDG_SEAINVADE )
								pTask->SetPriority( 
								pStdTask->GetPriority() * 2 );
							else
								pTask->SetPriority( 
								pStdTask->GetPriority() );
						}
					}
				}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"player %d goal %d task %d priority %d \n", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
#endif
			}
		}
	}
}


//
// priortize combat tasks which can include
//
//
// considerations:
//
// there are only 4 basic combat functions:
//
// 
// patroling
// escorting
// assemble for assault
// seeking of some kind
//
// priority should be that IDT_PATROL gets highest until
// an opfor rocket landing has been detected, and then
// at that time IDT_PREPAREWAR and IDT_PATROL should be
// alternated with highest priority, upon at war with any 
// opfor, IDT_PREPAREWAR should get 2 to 1 priority
//
// messages that will cause this function to fire:
// unit_damage
// veh_new
//

//
// priorize the tasks in the task list that will build new
// buildings or roads or bridges, that are not assigned
//
void CAIGoalMgr::UpdateConstructionTasks( CAIMsg *pMsg )
{
	// NOTE this message signals the loss of a unit, but after
	// it is processed, some other priority might force it to
	// go away, so what is needed is a triggering priority
	// that will allow override

	// if the pMsg == delete_unit 
	//m_iNumBldgs
	int iBldgDead = m_iNumBldgs;
	if( pMsg != NULL && 
		pMsg->m_iMsg == CNetCmd::delete_unit )
	{
		// m_dwID contains that unit
		// get its pointer and update
		CAIUnit *pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
		if( pUnit != NULL )
		{
			// that vehicle type to build task priority
			iBldgDead = pUnit->GetTypeUnit();
		}
	}

	// determine what vehicles are needed for staging
	// then later, as priority is being set for vehicle factories
	// make sure that all vehicles needed for staging have at
	// least one of the factories of the type needed to build
	// those vehicles
	WORD awTypes[CTransportData::num_types];
	for( int i=0; i<CTransportData::num_types; ++i )
		awTypes[i] = 0;
	SetAssaultStagingVehicle( awTypes );


	BOOL bPowerOnly = FALSE;
	if( m_iPowerHave < m_iPowerNeed )
		bPowerOnly = TRUE;

	BOOL bNeedDefenses = FALSE;
	if( m_pwaBldgs[CStructureData::light_2] &&
		m_pwaBldgs[CStructureData::light_1] )
		bNeedDefenses = TRUE;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIGoalMgr::UpdateConstructionTasks() for %d executing ", m_iPlayer );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIGoalMgr::UpdateConstructionTasks() waterworld %d LakeWorld %d guns/butter %d scenario %d poweronly=%d", 
   	m_bOceanWorld, m_bLakeWorld, m_bGunsOrButter, m_iScenario, bPowerOnly );
#endif

	// are any construction tasks in the list that do
	// not have buildings built yet?  If so, then leave
	// their priority alone, otherwise if count of built
	// >= goal count to have, set priority to 0, otherwise
	// reset priority to that task's standard priority
    POSITION pos = m_plTasks->GetHeadPosition();
    while( pos != NULL )
    {   
        CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );
			if( pTask->GetTaskParam(ORDER_TYPE) == CONSTRUCTION_ORDER )
			{
				// handle non-building construction tasks separately
				if( pTask->GetOrderID() == CNetCmd::build_road )
				{
					// reset if there are MSW_PLANNED_ROAD left
					if( !pTask->GetPriority() && 
						m_pMap->m_iRoadCount )
					{
						// reset to standard priority
						CAITask *pStdTask = 
							plTaskList->FindTask( pTask->GetID() );
						if( pStdTask != NULL )
							pTask->SetPriority( pStdTask->GetPriority() );
					}
					continue;
				}
				// handle repair tasks separately
				if( pTask->GetOrderID() == CNetCmd::repair_bldg ||
					pTask->GetOrderID() == CNetCmd::repair_veh )
				{
					if( !pTask->GetPriority() )
					{
						// reset to standard priority
						CAITask *pStdTask = 
							plTaskList->FindTask( pTask->GetID() );
						if( pStdTask != NULL )
							pTask->SetPriority( pStdTask->GetPriority() );
					}
					continue;
				}

				// get id of building to build from the task
				int iBldg = (int)pTask->GetTaskParam(BUILDING_ID);
				if( iBldg >= m_iNumBldgs )
					continue;

				if( pTask->GetStatus() == UNASSIGNED_TASK )
				{

#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
					// add test to confirm this can be built by this player
					BOOL bCanBuild = FALSE;
					// by getting a pointer to that type of building
					CStructureData const *pBldgData = 
						pGameData->GetStructureData( iBldg );
					if( pBldgData == NULL )
						continue;

					// then a pointer to this player
					EnterCriticalSection (&cs);
					CPlayer *pPlayer = 
						pGameData->GetPlayerData( m_iPlayer );
					if( pPlayer == NULL )
					{
						LeaveCriticalSection (&cs);
						continue;
					}
					// get the opinion of the game based on this player
					bCanBuild = pBldgData->PlyrIsDiscovered(pPlayer);
					LeaveCriticalSection (&cs);

					// construction task that builds apts and offices
					// is automatically discovered by AI players so
					// that a specific type of apt/office can be built
					if( pBldgData->GetUnionType() == CStructureData::UThousing 
						&& bCanBuild )
					{
						// but, there may not be artwork for them,
						if( pTask->GetID() == IDT_BUILDAPTS ||
							pTask->GetID() == IDT_BUILDOFFICE )
						{
							// so check first, and if not, default
							if( !(pBldgData->GetUnitFlags() & CUnitData::FLhaveArt) )
							{
								WORD wBldg = CStructureData::apartment_1_1;
								if( pTask->GetID() == IDT_BUILDOFFICE )
									wBldg = CStructureData::office_2_1;
								pTask->SetTaskParam(BUILDING_ID,wBldg);
							}
						}
					}

					// and set no priority and leave if not able to build yet
					if( !bCanBuild )
					{
						pTask->SetPriority( (BYTE)0 );
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIGoalMgr::player %d goal %d task %d has not discovered ", 
		m_iPlayer, pTask->GetGoalID(), pTask->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"building type %d with count=%d and goals=%d ",
		iBldg, m_pwaBldgs[iBldg], m_pwaBldgGoals[iBldg] );
#endif
						continue;
					}

					// update power plant level we can build
					if( iBldg == CStructureData::power_2 )
						m_iPowerLvl = 2;
					else if( iBldg == CStructureData::power_3 )
						m_iPowerLvl = 3;

					// if there is no water in the world, then
					// there is no need to build seaports/shipyards
					// so set priority to 0 each time
					if( !m_bOceanWorld && !m_bLakeWorld )
					{
						if( iBldg == CStructureData::seaport ||
							iBldg == CStructureData::shipyard_1 ||
							iBldg == CStructureData::shipyard_3 )
						{
							pTask->SetPriority( (BYTE)0 );
							continue;
						}
					}
					if( m_bLakeWorld && !m_bOceanWorld )
					{
						if( iBldg == CStructureData::seaport )
						{
							pTask->SetPriority( (BYTE)0 );
							continue;
						}
					}

					// begin with standard task's priority
					CAITask *pStdTask = 
						plTaskList->GetTask( pTask->GetID(), 0 );

					// there is water and we are at war
					if( (m_bOceanWorld || m_bLakeWorld) && m_bGunsOrButter )
					{
						if( iBldg == CStructureData::seaport ||
							iBldg == CStructureData::shipyard_1 ||
							iBldg == CStructureData::shipyard_3 )
						{
							pTask->SetPriority( (pStdTask->GetPriority() * 2) );
						}
					}


#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIGoalMgr::player %d goal %d task %d current priority %d ", 
		m_iPlayer, pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"for building type %d, with count=%d and goals=%d ",
		iBldg, m_pwaBldgs[iBldg], m_pwaBldgGoals[iBldg] );
#endif

					// is there at least one of this type of building?
					if( m_pwaBldgs[iBldg] ||
						!pTask->GetPriority() )
					{
						if( m_pwaBldgs[iBldg] >=
							m_pwaBldgGoals[iBldg] )
						{
							m_pwaBldgGoals[iBldg] = m_pwaBldgs[iBldg];
							pTask->SetPriority( (BYTE)0 );
						}
						else
						{
							// reset to standard priority for a default
							if( pStdTask != NULL )
								pTask->SetPriority( pStdTask->GetPriority() );

							// if task is to build an oil power plant
							// then if there is not a refinery built
							// then the power_2 should have zero priority
							if( iBldg == CStructureData::power_2 )
							{
								if( !m_pwaBldgs[CStructureData::refinery] )
									pTask->SetPriority( (BYTE)0 );
							}
							// if task is to build a refinery 
							// then if there is not an oil_well built
							// then the refinery should have zero priority
							if( iBldg == CStructureData::refinery )
							{
								if( !m_pwaBldgs[CStructureData::oil_well] )
									pTask->SetPriority( (BYTE)0 );
							}
							

							// try to stay with total of opfors for this building
							//if( m_plOpFors->GetBuildingCnt(iBldg) <
							//	m_pwaBldgs[iBldg] )
							//	pTask->SetPriority( (BYTE)0 );

							// farms need special handling
							if( iBldg == CStructureData::farm ) 
							{
								if( m_pwaMatGoals[CMaterialTypes::food] >
									m_pwaMatOnHand[CMaterialTypes::food] )
									pTask->SetPriority( 99 );
							}
						}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIGoalMgr::player %d reset construction goal %d task %d priority %d assigned %d ", 
		m_iPlayer, pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority(), pTask->GetStatus() );
#endif
					}

					// add some inference when none of a type exists
					if( !m_pwaBldgs[iBldg] )
					{
						// refineries need an oil well first
						if( iBldg == CStructureData::refinery )
						{
							if( !m_pwaBldgs[CStructureData::oil_well] )
								pTask->SetPriority( (BYTE)0 );
						}

						// when difficulty setting, and no camp, and 
						// minimum buildings exist and no infantry left
						// then increase the priority of camps
						if( iBldg == CStructureData::barracks_2 &&
							pGameData->m_iSmart )
						{
							if( m_pwaBldgs[CStructureData::smelter] &&
								m_pwaBldgs[CStructureData::coal] &&
								m_pwaBldgs[CStructureData::iron] &&
								!m_pwaUnits[CTransportData::infantry] )
							{
								if( pStdTask != NULL )
									pTask->SetPriority( 
									(pStdTask->GetPriority() * 2) );
							}
						}
					}

					// need power, so only allow power plants
					// or fuel production for power plants
					// or farms or smelters
					if( bPowerOnly && iBldg != CStructureData::farm &&
						iBldg != CStructureData::smelter )
					{
						if( pBldgData->GetUnionType() != 
							CStructureData::UTpower &&
							pBldgData->GetUnionType() != 
							CStructureData::UTmine )
						{
							// only if we have no power at all
							if( !m_iPowerHave )
								pTask->SetPriority( (BYTE)0 );

							// except for research cause we need it
							if( iBldg == CStructureData::research )
							{
								if( m_pwaBldgs[iBldg] < m_pwaBldgGoals[iBldg] )
								{
									if( pStdTask != NULL && !pTask->GetPriority() )
										pTask->SetPriority( pStdTask->GetPriority() );
								}
							}
						}
						else
						{
							if( m_pwaBldgs[iBldg] < m_pwaBldgGoals[iBldg] )
							{
								if( pStdTask != NULL && !pTask->GetPriority() )
									pTask->SetPriority( pStdTask->GetPriority() );
							}

							// if task is to build an oil power plant
							// then if there is not a refinery built
							// or a task to build a refinery with higher
							// priority, then it should have zero priority
							if( iBldg == CStructureData::power_2 )
							{
								if( m_pwaBldgs[CStructureData::refinery] )
								{
									if( pStdTask != NULL )
										pTask->SetPriority( pStdTask->GetPriority() );
								}
								else
									pTask->SetPriority( (BYTE)0 );
							}

							if( iBldg == CStructureData::power_1 ||
								iBldg == CStructureData::power_2 ||
								iBldg == CStructureData::power_3 )
							{
								if( m_pwaBldgs[iBldg] &&
									m_pwaBldgs[iBldg] >= m_pwaBldgGoals[iBldg] )
									pTask->SetPriority( (BYTE)0 );
							}
						}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIGoalMgr::player %d reset power/fuel goal %d task %d priority %d ", 
		m_iPlayer, pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
#endif
					}

					// construction task that builds apts and offices
					if( pBldgData->GetUnionType() ==
						CStructureData::UThousing )
					{
						if( m_iNeedApt && 
							(iBldg >= CStructureData::apartment_1_1 &&
							iBldg <= CStructureData::apartment_2_4) )
							pTask->SetPriority( (BYTE)99 );

						if( m_iNeedOffice && 
							(iBldg >= CStructureData::office_2_1 &&
							iBldg <= CStructureData::office_3_1) )
							pTask->SetPriority( (BYTE)99 );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIGoalMgr::player %d reset citizen building goal %d task %d priority %d ", 
		m_iPlayer, pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
#endif
					}


					// construction tasks that build factories 
					// that can produce vehicles needed for staging 
					// need to be reviewed for needing to be built
					if( pBldgData->GetUnionType() ==
						CStructureData::UTvehicle ||
						pBldgData->GetUnionType() ==
						CStructureData::UTshipyard )
					{
						// only if there are none of this type of factory
						if( !m_pwaBldgs[iBldg] )
						{
							// for each type of vehicle this factory type
							// can build, if one of them is needed for 
							// staging, then double std priority of task
							if( pBldgData->GetUnionType() ==
								CStructureData::UTvehicle )
							{
								CBuildVehicle *pBldVeh = 
									pBldgData->GetBldVehicle();
								if( pBldVeh != NULL )
								{
									for(int i=0;i<pBldVeh->GetSize();++i )
									{
										CBuildUnit const *pBldUnit =
											pBldVeh->GetUnit(i);
										if( pBldUnit != NULL && pBldUnit->GetVehType() 
											< CTransportData::num_types )
										{
											if( awTypes[pBldUnit->GetVehType()] )
											{
												if( m_pwaBldgs[CStructureData::smelter] )
												{
													pTask->SetPriority( 
													pStdTask->GetPriority()*2 );
													break;
												}
											}
										}
									}
								}
							}
							else if( pBldgData->GetUnionType() ==
								CStructureData::UTshipyard )
							{
								CBuildShipyard *pBldShip = 
									pBldgData->GetBldShipyard();
								if( pBldShip != NULL )
								{
									for(int i=0; i<pBldShip->GetSize(); ++i )
									{
										CBuildUnit const *pBldUnit =
											pBldShip->GetUnit(i);
										if( pBldUnit != NULL && pBldUnit->GetVehType() 
											< CTransportData::num_types )
										{
											if( awTypes[pBldUnit->GetVehType()] )
											{
												if( m_pwaBldgs[CStructureData::smelter] )
												{
													pTask->SetPriority( 
													pStdTask->GetPriority()*2 );
													break;
												}
											}
										}
									}
								}
							}

							// there is not a factory for building trucks
							// and we need trucks, so consider high priority
							if( m_bNeedTrucks &&
								iBldg == CStructureData::light_0 )
							{
								// but we must have certain structures built
								if( m_pwaBldgs[CStructureData::smelter] &&
									m_pwaBldgs[CStructureData::coal] &&
									m_pwaBldgs[CStructureData::iron] )
								{ 
									// and count of trucks is less than goals
									if( m_pwaUnits[CTransportData::heavy_truck] <
										m_pwaVehGoals[CTransportData::heavy_truck] )
										pTask->SetPriority( 99 );
								}
							}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIGoalMgr::player %d vehicle factory goal %d task %d priority %d ", 
		m_iPlayer, pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
#endif
						}
					}

					// this unit type was just lost
					if( iBldgDead < m_iNumBldgs )
					{
						if( !pTask->GetPriority() )
						{
							// reset to standard priority
							if( pStdTask != NULL )
							{
								pTask->SetPriority( pStdTask->GetPriority() );


#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIGoalMgr::player %d revive construction goal %d task %d priority %d ", 
		m_iPlayer, pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
#endif
								return;
							}
						}
					}

					// if not yet built a building of the types needed
					// for a scenario, then it gets priority
					if( !m_pwaBldgs[iBldg] )
					{
						ScenarioPriority(pTask);
					}

					// time to start building defenses
					if( bNeedDefenses )
					{
						// the building to be built is a pillbox,bunker,fort
						if( iBldg >= CStructureData::fort_1 &&
							iBldg <= CStructureData::fort_2 )
						{
							// it still has a rating, so it can be built
							if( pTask->GetPriority() )
							{
								if( pStdTask != NULL )
									pTask->SetPriority( (pStdTask->GetPriority() * 3) );
							}
						}
					}
				}
			}
		}
	}

	// now what?
}

void CAIGoalMgr::ClearStagingVehicle( int iVeh, WORD *awTypes )
{
	switch( iVeh )
	{
		case CTransportData::light_tank:
			awTypes[CTransportData::light_tank] = 0;
			break;
		case CTransportData::light_art:
			awTypes[CTransportData::light_art] = 0;
			break;
		case CTransportData::med_tank:
		case CTransportData::heavy_tank:
		case CTransportData::heavy_scout:
			awTypes[CTransportData::med_tank] = 0;
			awTypes[CTransportData::heavy_tank] = 0;
			awTypes[CTransportData::heavy_scout] = 0;
			break;
		case CTransportData::med_art:
		case CTransportData::heavy_art:
			awTypes[CTransportData::med_art] = 0;
			awTypes[CTransportData::heavy_art] = 0;
			break;
		case CTransportData::infantry:
		case CTransportData::rangers:
			awTypes[CTransportData::infantry] = 0;
			awTypes[CTransportData::rangers] = 0;
			break;
		case CTransportData::infantry_carrier:
			awTypes[CTransportData::infantry_carrier] = 0;
			break;
		case CTransportData::cruiser:
			awTypes[CTransportData::cruiser] = 0;
			break;
		case CTransportData::destroyer:
			awTypes[CTransportData::destroyer] = 0;
			break;
		case CTransportData::gun_boat:
			awTypes[CTransportData::gun_boat] = 0;
			break;
		case CTransportData::landing_craft:
			awTypes[CTransportData::landing_craft] = 0;
			break;
		default:
			break;
	}
}

//
// zero out the vehicle type families, relative to the vehicles
// that are currently in production at some factory
//
void CAIGoalMgr::ProducingStagingVehicle( WORD *awTypes )
{
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetType() == CUnit::building )
			{
				CAITask *pTask = m_plTasks->GetTask( 
					pUnit->GetTask(), pUnit->GetGoal() );
				if( pTask == NULL )
					continue;

				// that produce something
				if( pTask->GetTaskParam(ORDER_TYPE) == PRODUCTION_ORDER )
				{
					if( pTask->GetTaskParam(PRODUCTION_TYPE) == 
						CStructureData::UTvehicle ||
						pTask->GetTaskParam(PRODUCTION_TYPE) == 
						CStructureData::UTshipyard )
					{
						// get id of the vehicle being produced
						int iVeh = pTask->GetTaskParam(PRODUCTION_ITEM);
						ClearStagingVehicle( iVeh, awTypes );
#if THREADS_ENABLED
			// BUGBUG this function must yield
						myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
					}
				}
			}
		}
	}
}

//
// go thru the staging assault tasks (IDT_PREPAREWAR) and
// determine all the types of vehicles that are needed for
// staging and indicate with NON-ZERO at offset=type in
// the awTypes[]
//
void CAIGoalMgr::SetAssaultStagingVehicle( WORD *awTypes )
{
    POSITION pos = m_plTasks->GetHeadPosition();
    while( pos != NULL )
    {   
        CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

			if( pTask->GetID() == IDT_PREPAREWAR )
			{
				if( pTask->GetGoalID() == IDG_ADVDEFENSE ||
					pTask->GetGoalID() == IDG_LANDWAR )
				{
					if( pTask->GetTaskParam(CAI_TF_TANKS) )
					{
						awTypes[CTransportData::med_scout] = 1;
						awTypes[CTransportData::heavy_scout] = 1;
						awTypes[CTransportData::light_tank] = 1;
						awTypes[CTransportData::med_tank] = 1;
						awTypes[CTransportData::heavy_tank] = 1;
					}
					if( pTask->GetTaskParam(CAI_TF_ARTILLERY) )
					{
						awTypes[CTransportData::light_art] = 1;
						awTypes[CTransportData::med_art] = 1;
						awTypes[CTransportData::heavy_art] = 1;
					}
					if( pTask->GetTaskParam(CAI_TF_INFANTRY) )
					{
						awTypes[CTransportData::infantry] = 1;
						awTypes[CTransportData::rangers] = 1;
					}
					if( pTask->GetTaskParam(CAI_TF_IFVS) )
					{
						awTypes[CTransportData::infantry_carrier] = 1;
					}
				}
				else if( pTask->GetGoalID() == IDG_PIRATE )
				{
					if( pTask->GetTaskParam(CAI_TF_CRUISERS) )
					{
						awTypes[CTransportData::cruiser] = 1;
					}
					if( pTask->GetTaskParam(CAI_TF_DESTROYERS) )
					{
						awTypes[CTransportData::destroyer] = 1;
					}
				}
				else if(pTask->GetGoalID() == IDG_SEAINVADE )
				{
					if( pTask->GetTaskParam(CAI_TF_SHIPS) )
					{
						awTypes[CTransportData::gun_boat] = 1;
					}
					if( pTask->GetTaskParam(CAI_TF_MARINES) )
					{
						awTypes[CTransportData::rangers] = 1;
					}
					if( pTask->GetTaskParam(CAI_TF_ARMOR) )
					{
						awTypes[CTransportData::light_tank] = 1;
						awTypes[CTransportData::med_tank] = 1;
						awTypes[CTransportData::light_art] = 1;
					}
					if( pTask->GetTaskParam(CAI_TF_LANDING) )
					{
						awTypes[CTransportData::landing_craft] = 1;
					}
				}
			}
		}
	}
}
#if 1
//
// for IDG_ADVDEFENSE and IDG_LANDWAR versions of task
// CAI_TF_TANKS		4 - how many "light_tank,med_tank,heavy_tank" in TF
// CAI_TF_IFVS		5 - how many "infantry_carrier" in TF
// CAI_TF_ARTILLERY	6 - how many "light_art,med_art,heavy_art" in TF
// CAI_TF_INFANTRY	7 - how many "infantry,ranger" in TF
//
// for IDG_PIRATE versions of task
// CAI_TF_CRUISERS		4 - how many "cruiser"
// CAI_TF_DESTROYERS	5 - how many "destroyer"
//
// for IDG_SEAINVADE version of task
// CAI_TF_ARMOR   - 4 - how many "light_tank,med_tank,light_art"
// CAI_TF_LANDING - 5 - how many "landing_craft"
// CAI_TF_SHIPS   - 6 - how many "cruiser,destroyer,gun_boat"
// CAI_TF_MARINES - 7 - how many "ranger"
//
void CAIGoalMgr::UpdateStagingTasks( void )
{
	// only going to update IDT_PREPAREWAR tasks for
	// IDG_SEAINVADE, IDG_ADVDEFENSE and IDG_LANDWAR goals
	CAITask *pInvadeTask = NULL;
	CAITask *pDefenseTask = NULL;
	CAITask *pWarTask = NULL;
	CAITask *pTask = NULL;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateStagingTasks() for %d executing ", m_iPlayer );
#endif

	// now go thru task list and find active IDT_PREPAREWAR tasks
	// and for each up to STAGING_TASKS, record the goal id in
	// wCounts[j][STAGING_UPDATES-1]
	
    POSITION pos = m_plTasks->GetHeadPosition();
    while( pos != NULL )
    {   
        pTask = (CAITask *)m_plTasks->GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

			if( pTask->GetID() == IDT_PREPAREWAR )
			{
				// not an active staging task
				if( !pTask->GetTaskParam(CAI_LOC_X) &&
					!pTask->GetTaskParam(CAI_LOC_Y) )
					continue;

				if( pTask->GetGoalID() == IDG_SEAINVADE &&
					pInvadeTask == NULL)
				{
					pInvadeTask = pTask->CopyTask();
					pInvadeTask->SetGoalID( pTask->GetGoalID() );
				}
				else if( pTask->GetGoalID() == IDG_ADVDEFENSE &&
					pDefenseTask == NULL)
				{
					pDefenseTask = pTask->CopyTask();
					pDefenseTask->SetGoalID( pTask->GetGoalID() );
				}
				else if( pTask->GetGoalID() == IDG_LANDWAR &&
					pWarTask == NULL)
				{
					pWarTask = pTask->CopyTask();
					pWarTask->SetGoalID( pTask->GetGoalID() );
				}
			}
		}
	}

#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
#endif

	if( pInvadeTask != NULL)
	{
		for( int i=0; i<MAX_TASKPARAMS; ++i )
			pInvadeTask->SetTaskParam( i, 0 );
	}
	if( pDefenseTask != NULL)
	{
		for( int i=0; i<MAX_TASKPARAMS; ++i )
			pDefenseTask->SetTaskParam( i, 0 );
	}
	if( pWarTask != NULL)
	{
		for( int i=0; i<MAX_TASKPARAMS; ++i )
			pWarTask->SetTaskParam( i, 0 );
	}

#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
#endif

	WORD wCnt;
	// now go thru the units assigned to staging tasks and count
	// them in the unit catagory for their task
   	pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;
			if( pUnit->GetType() != CUnit::vehicle )
				continue;
			if( pUnit->GetTask() != IDT_PREPAREWAR )
				continue;

			// now, based on the type of support that will be
			// conducted, count up the types of units staged
			if( pUnit->GetGoal() == IDG_ADVDEFENSE ||
				pUnit->GetGoal() == IDG_LANDWAR )
			{
				switch( pUnit->GetTypeUnit() )
				{
					//case CTransportData::light_scout:
					//case CTransportData::med_scout:
					case CTransportData::heavy_scout:
					case CTransportData::light_tank:
					case CTransportData::med_tank:
					case CTransportData::heavy_tank:
						if( pUnit->GetGoal() == IDG_ADVDEFENSE &&
							pDefenseTask != NULL )
						{
							wCnt = 
								pDefenseTask->GetTaskParam(CAI_TF_TANKS) + 1;
							pDefenseTask->SetTaskParam(CAI_TF_TANKS,wCnt);
						}
						else if( pUnit->GetGoal() == IDG_LANDWAR &&
							pWarTask != NULL )
						{
							wCnt = 
								pWarTask->GetTaskParam(CAI_TF_TANKS) + 1;
							pWarTask->SetTaskParam(CAI_TF_TANKS,wCnt);
						}
						break;
					case CTransportData::infantry_carrier:
						if( pUnit->GetGoal() == IDG_ADVDEFENSE &&
							pDefenseTask != NULL )
						{
							wCnt = 
								pDefenseTask->GetTaskParam(CAI_TF_IFVS) + 1;
							pDefenseTask->SetTaskParam(CAI_TF_IFVS,wCnt);
						}
						else if( pUnit->GetGoal() == IDG_LANDWAR &&
							pWarTask != NULL )
						{
							wCnt = 
								pWarTask->GetTaskParam(CAI_TF_IFVS) + 1;
							pWarTask->SetTaskParam(CAI_TF_IFVS,wCnt);
						}
						break;
					case CTransportData::light_art:
					case CTransportData::med_art:
					case CTransportData::heavy_art:
						if( pUnit->GetGoal() == IDG_ADVDEFENSE &&
							pDefenseTask != NULL )
						{
							wCnt = 
								pDefenseTask->GetTaskParam(CAI_TF_ARTILLERY) + 1;
							pDefenseTask->SetTaskParam(CAI_TF_ARTILLERY,wCnt);
						}
						else if( pUnit->GetGoal() == IDG_LANDWAR &&
							pWarTask != NULL )
						{
							wCnt = 
								pWarTask->GetTaskParam(CAI_TF_ARTILLERY) + 1;
							pWarTask->SetTaskParam(CAI_TF_ARTILLERY,wCnt);
						}
						break;
					case CTransportData::infantry:
					case CTransportData::rangers:
						if( pUnit->GetGoal() == IDG_ADVDEFENSE &&
							pDefenseTask != NULL )
						{
							wCnt = 
								pDefenseTask->GetTaskParam(CAI_TF_INFANTRY) + 1;
							pDefenseTask->SetTaskParam(CAI_TF_INFANTRY,wCnt);
						}
						else if( pUnit->GetGoal() == IDG_LANDWAR &&
							pWarTask != NULL )
						{
							wCnt = 
								pWarTask->GetTaskParam(CAI_TF_INFANTRY) + 1;
							pWarTask->SetTaskParam(CAI_TF_INFANTRY,wCnt);
						}
						break;
					default:
						break;
				}
			}
			else if(pUnit->GetGoal() == IDG_SEAINVADE )
			{
				if( pInvadeTask == NULL )
					continue;

				switch( pUnit->GetTypeUnit() )
				{
// CAI_TF_ARMOR   - 4 - how many "light_tank,med_tank,light_art"
// CAI_TF_LANDING - 5 - how many "landing_craft"
// CAI_TF_SHIPS   - 6 - how many "cruiser,destroyer,gun_boat"
// CAI_TF_MARINES - 7 - how many "ranger"
					case CTransportData::light_tank:
					case CTransportData::med_tank:
					case CTransportData::light_art:
						wCnt = 
							pInvadeTask->GetTaskParam(CAI_TF_ARMOR) + 1;
						pInvadeTask->SetTaskParam(CAI_TF_ARMOR,wCnt);
						break;
					case CTransportData::landing_craft:
						wCnt = 
							pInvadeTask->GetTaskParam(CAI_TF_LANDING) + 1;
						pInvadeTask->SetTaskParam(CAI_TF_LANDING,wCnt);
						break;
					case CTransportData::gun_boat:
						wCnt = 
							pInvadeTask->GetTaskParam(CAI_TF_SHIPS) + 1;
						pInvadeTask->SetTaskParam(CAI_TF_SHIPS,wCnt);
						break;
					case CTransportData::rangers:
						wCnt = 
							pInvadeTask->GetTaskParam(CAI_TF_MARINES) + 1;
						pInvadeTask->SetTaskParam(CAI_TF_MARINES,wCnt);
						break;
					default:
						break;
				}
			}
		}
	}

#ifdef _LOGOUT
if( pInvadeTask != NULL )
{
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"UpdateStagingTasks() for goal %d task %d IDG_SEAINVADE assignments", 
pInvadeTask->GetGoalID(), pInvadeTask->GetID() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"SHIPS=%d  MARINE=%d  ARMOR=%d  LC=%d \n",
pInvadeTask->GetTaskParam(CAI_TF_SHIPS),pInvadeTask->GetTaskParam(CAI_TF_MARINES),
pInvadeTask->GetTaskParam(CAI_TF_ARMOR),pInvadeTask->GetTaskParam(CAI_TF_LANDING) );
}
if( pDefenseTask != NULL )
{
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"UpdateStagingTasks() for goal %d task %d IDG_ADVDEFENSE assignments", 
pDefenseTask->GetGoalID(), pDefenseTask->GetID() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"AFV=%d  IFV=%d  ART=%d  INF=%d \n",
pDefenseTask->GetTaskParam(CAI_TF_TANKS),pDefenseTask->GetTaskParam(CAI_TF_IFVS),
pDefenseTask->GetTaskParam(CAI_TF_ARTILLERY),pDefenseTask->GetTaskParam(CAI_TF_INFANTRY) );
}
if( pWarTask != NULL )
{
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"UpdateStagingTasks() for goal %d task %d IDG_LANDWAR assignments", 
pWarTask->GetGoalID(), pWarTask->GetID() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"AFV=%d  IFV=%d  ART=%d  INF=%d \n",
pWarTask->GetTaskParam(CAI_TF_TANKS),pWarTask->GetTaskParam(CAI_TF_IFVS),
pWarTask->GetTaskParam(CAI_TF_ARTILLERY),pWarTask->GetTaskParam(CAI_TF_INFANTRY) );
}
#endif

#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
#endif

	CAITask *pActualTask = NULL;
	WORD wUnitCat = 0;
	// now wCounts[][] contains the counts of units assigned to
	// staging tasks by unit catagory
	//
	// try to update seaborne assault first
	pTask = m_plTasks->GetTask( IDT_PREPAREWAR, IDG_SEAINVADE );
	if( pInvadeTask != NULL && pTask != NULL )
	{
		int i = 2;
		while( i )
		{
			if( i == 2 )
				wUnitCat = CAI_TF_ARMOR;
			else if( 1 == 1 )
				wUnitCat = CAI_TF_MARINES;
			else
				break;
			i--;

			// if there are fewer units assigned of this unit catagory
			// of the IDG_SEAINVADE task than what is needed then
			if( pInvadeTask->GetTaskParam(wUnitCat) &&
				(pInvadeTask->GetTaskParam(wUnitCat)
				< pTask->GetTaskParam(wUnitCat)) )
			{
				// consider if IDG_ADVDEFENSE or IDG_LANDWAR tasks
				// are also less than needed and if so, then try 
				// to reassign some of those unit-catagory units
				// to the IDG_SEAINVADE task
				if( pDefenseTask != NULL )
				{
					// get actual IDG_ADVDEFENSE task
					pActualTask = m_plTasks->GetTask( 
						pDefenseTask->GetID(), pDefenseTask->GetGoalID() );
					if( pActualTask != NULL )
					{
					// task has less of this unit catagory 
					// assigned than is needed
					if( pDefenseTask->GetTaskParam(wUnitCat) &&
						(pDefenseTask->GetTaskParam(wUnitCat)
						< pActualTask->GetTaskParam(wUnitCat)) )
						ConsiderReassignment( 
							pTask, pInvadeTask, pActualTask, wUnitCat );
					}
				}
				else if( pWarTask != NULL )
				{
					// get actual IDG_LANDWAR task
					pActualTask = m_plTasks->GetTask( 
						pWarTask->GetID(), pWarTask->GetGoalID() );
					if( pActualTask != NULL )
					{
					// task has less of this unit catagory 
					// assigned than is needed
					if( pWarTask->GetTaskParam(wUnitCat) &&
						(pWarTask->GetTaskParam(wUnitCat)
						< pActualTask->GetTaskParam(wUnitCat)) )
						ConsiderReassignment( 
							pTask, pInvadeTask, pActualTask, wUnitCat );
					}
				}
			}
		}
		goto Cleanup; //return;
	}

	pTask = m_plTasks->GetTask( IDT_PREPAREWAR, IDG_ADVDEFENSE );
	if( pDefenseTask != NULL && pTask != NULL )
	{
		for( wUnitCat=CAI_TF_TANKS;
			 wUnitCat<=CAI_TF_INFANTRY; ++wUnitCat )
		{
			if( pDefenseTask->GetTaskParam(wUnitCat) &&
				(pDefenseTask->GetTaskParam(wUnitCat) < 
				pTask->GetTaskParam(wUnitCat)) )
			{
				// consider if IDG_LANDWAR task
				// is also less than needed and if so, then try 
				// to reassign some of those unit-catagory units
				// to the IDG_ADVDEFENSE task
				if( pWarTask != NULL )
				{
					// get actual IDG_LANDWAR task
					pActualTask = m_plTasks->GetTask( 
						pWarTask->GetID(), pWarTask->GetGoalID() );
					if( pActualTask != NULL )
					{
					// task has less of this unit catagory 
					// assigned than is needed
					if( pWarTask->GetTaskParam(wUnitCat) &&
						(pWarTask->GetTaskParam(wUnitCat)
						< pActualTask->GetTaskParam(wUnitCat)) )
						ConsiderReassignment( 
							pTask, pDefenseTask, pActualTask, wUnitCat );
					}
				}
			}
		}
		goto Cleanup; //return;
	}

	pTask = m_plTasks->GetTask( IDT_PREPAREWAR, IDG_LANDWAR );
	if( pWarTask != NULL && pTask != NULL )
	{
		for( wUnitCat=CAI_TF_TANKS;
			 wUnitCat<=CAI_TF_INFANTRY; ++wUnitCat )
		{
			if( pWarTask->GetTaskParam(wUnitCat) &&
				(pWarTask->GetTaskParam(wUnitCat) < 
				pTask->GetTaskParam(wUnitCat)) )
			{
				// consider if IDG_ADVDEFENSE task
				// is also less than needed and if so, then try 
				// to reassign some of those unit-catagory units
				// to the IDG_LANDWAR task
				if( pDefenseTask != NULL )
				{
					// get actual IDG_ADVDEFENSE task
					pActualTask = m_plTasks->GetTask( 
						pDefenseTask->GetID(), pDefenseTask->GetGoalID() );
					if( pActualTask != NULL )
					{
					// task has less of this unit catagory 
					// assigned than is needed
					if( pDefenseTask->GetTaskParam(wUnitCat) &&
						(pDefenseTask->GetTaskParam(wUnitCat)
						< pActualTask->GetTaskParam(wUnitCat)) )
						ConsiderReassignment( 
							pTask, pWarTask, pActualTask, wUnitCat );
					}
				}
			}
		}
	}

	Cleanup:
	if( pInvadeTask != NULL)
		delete pInvadeTask;
	if( pDefenseTask != NULL)
		delete pDefenseTask;
	if( pWarTask != NULL)
		delete pWarTask;
}

//
// process the units assigned to the pFromTask, and if that unit matches
// the iUnitCat (unit catagory) passed, then reassign that unit to the
// pToTask up to the number of units of that iUnitCat passed of pToTask
//
void CAIGoalMgr::ConsiderReassignment( CAITask *pToTask, 
	CAITask *pCntTask, CAITask *pFromTask, WORD wUnitCat )
{
	// how many units of that catagory can we transfer
	int iTransfering = 
		pToTask->GetTaskParam(wUnitCat) - pCntTask->GetTaskParam(wUnitCat);
	if( iTransfering <= 0 )
		return;

#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
#endif

   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			// done with reassignment
			if( !iTransfering )
				return;

			if( pUnit->GetOwner() != m_iPlayer )
				continue;
			if( pUnit->GetType() != CUnit::vehicle )
				continue;
			// skip units not assigned to pFromTask
			if( pUnit->GetGoal() != pFromTask->GetGoalID() ||
				pUnit->GetTask() != pFromTask->GetID() )
				continue;

			BOOL bSkipIt = TRUE;
			// make sure unit belongs to passed iUnitCat and
			// is appropriate for the pToTask catagory
			if( pToTask->GetGoalID() == IDG_SEAINVADE )
			{
				if( wUnitCat == CAI_TF_ARMOR )
				{
					switch( pUnit->GetTypeUnit() )
					{
					case CTransportData::light_tank:
					case CTransportData::med_tank:
					case CTransportData::light_art:
						bSkipIt = FALSE;
					default:
						break;
					}
				}
				else if( wUnitCat == CAI_TF_MARINES )
				{
					if( pUnit->GetTypeUnit() == 
						CTransportData::rangers )
						bSkipIt = FALSE;
				}
			}
			else if( pToTask->GetGoalID() == IDG_ADVDEFENSE ||
				pToTask->GetGoalID() == IDG_LANDWAR )
			{
				if( wUnitCat == CAI_TF_TANKS )
				{
					switch( pUnit->GetTypeUnit() )
					{
					//case CTransportData::light_scout:
					//case CTransportData::med_scout:
					case CTransportData::heavy_scout:
					case CTransportData::light_tank:
					case CTransportData::med_tank:
					case CTransportData::heavy_tank:
						bSkipIt = FALSE;
					default:
						break;
					}
				}
				else if( wUnitCat == CAI_TF_IFVS )
				{
					if( pUnit->GetTypeUnit() == 
						CTransportData::infantry_carrier )
						bSkipIt = FALSE;
				}
				else if( wUnitCat == CAI_TF_ARTILLERY )
				{
					switch( pUnit->GetTypeUnit() )
					{
					case CTransportData::light_art:
					case CTransportData::med_art:
					case CTransportData::heavy_art:
						bSkipIt = FALSE;
					default:
						break;
					}
				}
				else if( wUnitCat == CAI_TF_INFANTRY )
				{
					switch( pUnit->GetTypeUnit() )
					{
					case CTransportData::infantry:
					case CTransportData::rangers:
						bSkipIt = FALSE;
					default:
						break;
					}
				}
			}

			if( bSkipIt )
				continue;

			// if still here, then the unit is appropriate for the
			// unit catagory and should be reassigned to the pToTask
			// and then find a staging hex to force the unit to move

			// special handling for carrier units
			if( pUnit->GetTypeUnit() == CTransportData::infantry_carrier )
				pUnit->UnloadCargo();

			// now reassign the unit to the pToTask
			pUnit->SetGoal( pToTask->GetGoalID() );
			pUnit->SetTask( pToTask->GetID() );

			// now find the current location of the vehicle
			CHexCoord hexVeh(0,0);
			EnterCriticalSection (&cs);
			CVehicle *pVehicle = 
				theVehicleMap.GetVehicle( pUnit->GetID() );
			if( pVehicle != NULL )
				hexVeh = pVehicle->GetHexHead();
			LeaveCriticalSection (&cs);

			if( !hexVeh.X() && !hexVeh.Y() )
				continue;

			// use the current location to find a staging hex
			m_pMap->m_pMapUtil->FindStagingHex( hexVeh,
				2, 2, pUnit->GetTypeUnit(), hexVeh, FALSE );
			// and send the vehicle there to force a veh_dest
			pUnit->SetDestination( hexVeh );
			// reduce the number of units to be reassigned
			iTransfering--;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::ConsiderReassignment() for player %d unit %ld a %d vehicle ", 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetTypeUnit() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"has been reassigned to goal %d task %d and going to %d,%d \n", 
pUnit->GetGoal(), pUnit->GetTask(), hexVeh.X(), hexVeh.Y() );
#endif
		}
	}
}
#endif

//
// process production tasks and make adjustments to priority
// based on the status of game
//
void CAIGoalMgr::UpdateProductionTasks( CAIMsg *pMsg )
{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateProductionTasks() for %d executing ", m_iPlayer );
#endif

	// if the pMsg == delete_unit 
	//m_iNumUnits//m_iNumBldgs
	int iVehDead = m_iNumUnits;
	if( pMsg != NULL && 
		pMsg->m_iMsg == CNetCmd::delete_unit )
	{
		// m_dwID contains that unit
		// get its pointer and update
		CAIUnit *pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
		if( pUnit != NULL )
		{
			// that vehicle type to build task priority
			iVehDead = pUnit->GetTypeUnit();
		}
	}

	// determine what vehicles are needed for staging
	WORD awTypes[CTransportData::num_types];
	for( int i=0; i<CTransportData::num_types; ++i )
		awTypes[i] = 0;
	SetAssaultStagingVehicle( awTypes );
	int iBestRatio = 0;

	// specially consider trucks


    POSITION pos = m_plTasks->GetHeadPosition();
    while( pos != NULL )
    {   
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

			// only interested in unassigned tasks
			if( pTask->GetStatus() != UNASSIGNED_TASK )
				continue;

			// that produce something
			if( pTask->GetTaskParam(ORDER_TYPE) == PRODUCTION_ORDER )
			{
				if( pTask->GetTaskParam(PRODUCTION_TYPE) == 
					CStructureData::UTvehicle ||
					pTask->GetTaskParam(PRODUCTION_TYPE) == 
					CStructureData::UTshipyard )
				{
					// get id of the vehicle being produced
					int iVeh = pTask->GetTaskParam(PRODUCTION_ITEM);
					if( iVeh >= m_iNumUnits )
					{
						pTask->SetPriority( (BYTE)0 );
						continue;
					}

					// add test to confirm this can be produced by this player
					// and set no priority and leave if not able to build yet
					if( !CanBuildVehType(iVeh) )
					{
						pTask->SetPriority( (BYTE)0 );
						continue;
					}

					// this unit type was just lost, and it is the
					// type of vehicle produced by this task
					if( iVehDead < m_iNumUnits &&
						iVeh == iVehDead )
					{
						if( !pTask->GetPriority() )
						{
						// reset to standard priority
						CAITask *pStdTask = 
							plTaskList->GetTask( pTask->GetID(), 0 );
						if( pStdTask != NULL )
							pTask->SetPriority( pStdTask->GetPriority() );
						return;
						}
					}

					// if the count of this type of vehicle is more
					// than the goals for this vehicle, then lower priority
					// this is supposed to shut off production
					if( m_pwaUnits[iVeh] >
						m_pwaVehGoals[iVeh] )
					{
						pTask->SetPriority( (BYTE)0 );
					}
					else // this is supposed to restart production
					{
						// get ready to set standard priority
						CAITask *pStdTask = 
							plTaskList->GetTask( pTask->GetID(), 0 );
						if( pStdTask != NULL )
						{
							// if this type of vehicle is involved with
							// a staging task then bump up its priority
							if( awTypes[iVeh] )
							{
								// to max if there are yet to be units
								if( !m_pwaUnits[iVeh] )
									pTask->SetPriority( 98 );
								else 
								{
									// this will have the effect of increasing
									// priority on those vehicle types that have
									// the fewest built of their goals to build
									i = m_pwaVehGoals[iVeh] / m_pwaUnits[iVeh];
									if( i > iBestRatio )
									{
										iBestRatio = i;
										pTask->SetPriority( 
										pStdTask->GetPriority()*3 );
									}
									else // just double it
										pTask->SetPriority( 
										pStdTask->GetPriority()*2 );
								}
							}
							else // vehicle type not being staged
							{
								// start with standard priority, and allow
								// other considerations to change that
								pTask->SetPriority( pStdTask->GetPriority() );

								if( !m_pwaUnits[iVeh] )
								{
									if( m_pwaAttribs[CRaceDef::attack] > 100 )
									{
										if( pGameData->IsCombatVehicle(iVeh) )
											pTask->SetPriority( 
											pStdTask->GetPriority()*2 );
									}
									else
									{
										pTask->SetPriority( 
											pStdTask->GetPriority()*2 );
									}
								}
								else
								{
									// this will have the effect of increasing
									// priority on those vehicle types that have
									// the fewest built of their goals to build
									i = m_pwaVehGoals[iVeh] / m_pwaUnits[iVeh];
									if( i > iBestRatio )
									{
										iBestRatio = i;

										pTask->SetPriority( 
											pStdTask->GetPriority()*2 );
									}
								}
							}

							// only for trucks!
							if( iVeh == CTransportData::heavy_truck ||
								iVeh == CTransportData::med_truck )
							{
								if( m_pwaUnits[iVeh] < MINIMUM_IDLE_TRUCKS_AI ||
									m_bNeedTrucks )
								{
									pTask->SetPriority( 99 );
									m_bNeedTrucks = FALSE;
								}
							}
							
							// kludge for scouts to always have one
							if( !m_pwaUnits[CTransportData::light_scout] &&
								iVeh == CTransportData::light_scout )
							{
								pTask->SetPriority( 98 );

							}
							// ceiling
							if( pTask->GetPriority() > 100 )
								pTask->SetPriority( 98 );
						}
					}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::player %d set production goal %d task %d new priority %d ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"for vehicle type %d, with count=%d and goals=%d staging=%d ",
iVeh, m_pwaUnits[iVeh], m_pwaVehGoals[iVeh], awTypes[iVeh] );
#endif
				}
			}
		}
	}
}

//
// considering the passed message, update the summary
// counts of buildings and vehicles to reflect change
// indicated by message
//
void CAIGoalMgr::UpdateSummarys( CAIMsg *pMsg )
{
/*
	if Dave cannot ensure single veh_new messages for new units
	then this function may need to change:

	BOOL bOldRestart = m_bRestart;
	m_bRestart = FALSE;

	if( (pMsg->m_iMsg == CNetCmd::veh_new ||
		pMsg->m_iMsg == CNetCmd::delete_unit)
		&&
		pMsg->m_idata3 == m_iPlayer )
	{
		ClearVehSummary();
		InitVehSummary( FALSE );	// indicate do not create arrays
	}

	if( (pMsg->m_iMsg == CNetCmd::bldg_new ||
		pMsg->m_iMsg == CNetCmd::delete_unit)
		&&
		pMsg->m_idata3 == m_iPlayer )
	{
		ClearBldgSummary();			// NEED TO WRITE THIS FUNCTION
		InitBldgSummary( FALSE );	// indicate do not create arrays
	}

	m_bRestart = bOldRestart;

	if( pMsg->m_idata3 != m_iPlayer )
	{
		// the opfor involved
		CAIOpFor *pOpFor = GetOpFor(pMsg->m_idata3);
		if( pOpFor == NULL )
			return;
		// an opfor new vehicle/building
		if( pMsg->m_iMsg == CNetCmd::veh_new ||
			pMsg->m_iMsg == CNetCmd::bldg_new )
		{
			pOpFor->UpdateCounts(pMsg);
		}
		else // delete_unit - must handle here to access unit list
		{
			CAIUnit *pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
			if( pUnit == NULL )
				return;
			WORD wCnt;
			if( pUnit->GetType() == CUnit::building )
			{
				wCnt = pOpFor->GetBuilding( pUnit->GetTypeUnit() );
				if( wCnt )
					wCnt--;
				pOpFor->SetBuilding( pUnit->GetTypeUnit(),wCnt );
			}
			else if( pUnit->GetType() == CUnit::vehicle )
			{
				wCnt = pOpFor->GetVehicle( pUnit->GetTypeUnit() );
				if( wCnt )
					wCnt--;
				pOpFor->SetVehicle( pUnit->GetTypeUnit(),wCnt );
			}
		}
	}
*/

	if( pMsg == NULL )
	{
		return;
	}
	// if it was our construction/production completion
	if( pMsg->m_iMsg == CNetCmd::bldg_new &&
		pMsg->m_idata3 == m_iPlayer )
	{
		// make sure this unit already exists
		CAIUnit *pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
		if( pUnit == NULL )
			return;

		// get id/offset of building just built
		int iAt = pMsg->m_idata1;
		if( iAt < m_iNumBldgs )
		{
			// need to capture the id of the rocket
			if( !m_dwRocket )
			{
				if( iAt == CStructureData::rocket )
					m_dwRocket = pMsg->m_dwID;
			}

			WORD wCnt = m_pwaBldgs[iAt] + 1;
			m_pwaBldgs[iAt] = wCnt;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateSummarys() player %d for BUILDING %d, count=%d ", 
m_iPlayer, iAt, wCnt );
#endif
		}
	}
	// a new one of our vehicles was just produced
	else if( pMsg->m_iMsg == CNetCmd::veh_new &&
		pMsg->m_idata3 == m_iPlayer )
	{
		// placement messages have the rocket id as producer
		// so ZERO it to indicate to other processes this is
		// a placement message
		if( pMsg->m_dwID2 == m_dwRocket )
			pMsg->m_dwID2 = 0;

		// make sure this unit already exists before counting it
		CAIUnit *pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
		if( pUnit == NULL )
			return;

		// get id/offset of vehicle just produced
		int iAt = pMsg->m_idata1;
		if( iAt < m_iNumUnits )
		{
			WORD wCnt = m_pwaUnits[iAt] + 1;
			m_pwaUnits[iAt] = wCnt;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateSummarys() player %d for VEHICLE %d, count=%d ", 
m_iPlayer, iAt, wCnt );
#endif
		}
	}
	//
	// one of our existing units was destroyed
	// 
	// based on the type of unit (building/vehicle) then
	// update the appropriate array using the unit type
	// as the offset
	//
	else if( pMsg->m_iMsg == CNetCmd::delete_unit &&
		pMsg->m_idata3 == m_iPlayer )
	{
		// make sure the unit is still there
		CAIUnit *pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
		if( pUnit == NULL )
			return;

		if( pUnit->GetType() == CUnit::building )
		{
			// get id/offset of building just destroyed
			int iAt = pUnit->GetTypeUnit();
			if( iAt < m_iNumBldgs &&
				m_pwaBldgs[iAt] )
			{
				WORD wCnt = m_pwaBldgs[iAt] - 1;
				m_pwaBldgs[iAt] = wCnt;
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateSummarys() player %d reduced for BUILDING %d, count=%d ", 
m_iPlayer, iAt, wCnt );
#endif
			}
		}
		else if( pUnit->GetType() == CUnit::vehicle )
		{
			// get id/offset of vehicle just destroyed
			int iAt = pUnit->GetTypeUnit();
			if( iAt < m_iNumUnits &&
				m_pwaUnits[iAt] )
			{
				WORD wCnt = m_pwaUnits[iAt] - 1;
				m_pwaUnits[iAt] = wCnt;
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateSummarys() player %d for VEHICLE %d, count=%d ", 
m_iPlayer, iAt, wCnt );
#endif
			}
		}
	}
	//
	// OPFOR units
	//
	// set up opfor for 
	//
	else if( (pMsg->m_iMsg == CNetCmd::veh_new ||
		pMsg->m_iMsg == CNetCmd::bldg_new ||
		pMsg->m_iMsg == CNetCmd::delete_unit) &&
		pMsg->m_idata3 != m_iPlayer )
	{
		// the opfor involved
		CAIOpFor *pOpFor = GetOpFor(pMsg->m_idata3);
		if( pOpFor == NULL )
			return;
		// an opfor new vehicle/building
		if( pMsg->m_iMsg == CNetCmd::veh_new ||
			pMsg->m_iMsg == CNetCmd::bldg_new )
		{
			pOpFor->UpdateCounts(pMsg);
			if( pMsg->m_iMsg == CNetCmd::veh_new )
			{
				// update goals for this vehicle type
				WORD wCnt = m_pwaVehGoals[pMsg->m_idata1] + 1;
				m_pwaVehGoals[pMsg->m_idata1] = wCnt;
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateSummarys() player %d added 1 for PLAYER %d VEHICLE %ld TYPE %d ", 
m_iPlayer, pMsg->m_idata3, pMsg->m_dwID, pMsg->m_idata1 );
#endif
			}
#ifdef _LOGOUT
			else
			{
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateSummarys() player %d added 1 for PLAYER %d BUILDING %ld TYPE %d ", 
m_iPlayer, pMsg->m_idata3, pMsg->m_dwID, pMsg->m_idata1 );
			}
#endif
		}
		else // delete_unit - must handle here to access unit list
		{
			CAIUnit *pUnit = m_plUnits->GetOpForUnit( pMsg->m_dwID );
			if( pUnit == NULL )
				return;
			WORD wCnt;
			if( pUnit->GetType() == CUnit::building )
			{
				wCnt = pOpFor->GetBuilding( pUnit->GetTypeUnit() );
				if( wCnt )
					wCnt--;
				pOpFor->SetBuilding( pUnit->GetTypeUnit(),wCnt );
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateSummarys() player %d subtracted 1 for PLAYER %d BUILDING %ld TYPE %d ", 
m_iPlayer, pMsg->m_idata3, pMsg->m_dwID, pUnit->GetTypeUnit() );
#endif
			}
			else if( pUnit->GetType() == CUnit::vehicle )
			{
				wCnt = pOpFor->GetVehicle( pUnit->GetTypeUnit() );
				if( wCnt )
					wCnt--;
				pOpFor->SetVehicle( pUnit->GetTypeUnit(),wCnt );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateSummarys() player %d subtracted 1 for PLAYER %d VEHICLE %ld TYPE %d ", 
m_iPlayer, pMsg->m_idata3, pMsg->m_dwID, pUnit->GetTypeUnit() );
#endif
				// update goals for this vehicle type
				//wCnt = m_pwaVehGoals[pMsg->m_idata1];
				//if( wCnt )
				//	m_pwaVehGoals[pMsg->m_idata1] = wCnt - 1;
			}

			// turned off killopfor
#if 0
			// consider a kill'em off tactic
			if( pUnit->GetType() == CUnit::building &&
				pUnit->GetTypeUnit() == CStructureData::rocket )
			{
			
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateSummarys() player %d rocket destroyed PLAYER %d VEHICLE %ld TYPE %d ", 
m_iPlayer, pMsg->m_idata3, pMsg->m_dwID, pUnit->GetTypeUnit() );
#endif
			//(!pOpFor->GetCombat() || 
				if( pOpFor->GetBuilding(m_iNumBldgs) < 5 && !pOpFor->IsAI() )
					KillOpfor(pMsg->m_idata3); // passing the opfor id
			}
#endif

		}
	}

	// BUGBUG other messages are not being handled
	// but NULL is allowed to return
}
//
// get combat strength of this unit's player and compare to combat
// strength of opfor of unit/player passed and return TRUE if this
// player is stronger or > 3/4 of opfor strength, else FALSE
//
BOOL CAIGoalMgr::RealityCheck( DWORD dwOpforID )
{
	CAIUnit *pOpForUnit = m_plUnits->GetOpForUnit( dwOpforID );
	if( pOpForUnit == NULL )
		return TRUE;

	CAIOpFor *pOpFor = GetOpFor( pOpForUnit->GetOwner() );
	if( pOpFor == NULL )
		return TRUE;

	int iOpForStrength = (pOpFor->GetCombat() / 4) * 3;
	if( GetCombat() >= iOpForStrength )
		return TRUE;

	return FALSE;
}

int CAIGoalMgr::GetCombat( void )
{
#if THREADS_ENABLED
		// BUGBUG this function must yield
	myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	int iStrength = 0;
	for( int i=0; i<m_iNumUnits; i++ )
	{
		switch( i )
		{
			case CTransportData::light_scout:
			case CTransportData::infantry_carrier:
			case CTransportData::landing_craft:
			case CTransportData::infantry:
			case CTransportData::rangers:
			case CTransportData::marines:
				iStrength += (m_pwaUnits[i] * 1 );
				break;
			case CTransportData::med_scout:
			case CTransportData::light_tank:
			case CTransportData::light_art:
			case CTransportData::gun_boat:
				iStrength += (m_pwaUnits[i] * 2 );
				break;
			case CTransportData::heavy_scout:
			case CTransportData::med_tank:
			case CTransportData::med_art:
			case CTransportData::destroyer:
				iStrength += (m_pwaUnits[i] * 4 );
				break;
			case CTransportData::heavy_tank:
			case CTransportData::heavy_art:
			case CTransportData::cruiser:
				iStrength += (m_pwaUnits[i] * 6 );
				break;
			default:
				break;
		}
	}
	return( iStrength );
}
//
// this will get the opfor of the id passed or try
// to add it if it does not exist, and if new and
// added, it will do a complete review of the opfor
//
CAIOpFor *CAIGoalMgr::GetOpFor( int iOpForID )
{
	CAIOpFor *pOpFor = m_plOpFors->GetOpFor( iOpForID );

	// new CAIOpFor is needed
	if( pOpFor == NULL )
	{
		pOpFor = m_plOpFors->AddOpFor( iOpForID );
		if( pOpFor == NULL )
			return(pOpFor);
		// BUGBUG this will take a while to initialize
		pOpFor->UpdateCounts(NULL);
	}
	return(pOpFor);
}
//
// for some PRODUCTION_ORDERs, use the PRODUCTION_QTY param
// to increase/decrease the number of a type of vehicle to
// build instead of adding another production goal
//
void CAIGoalMgr::AddGoal( WORD wGoal )
{
	// get goal from standard goal list
	CAIGoal *pGoal = plGoalList->GetGoal( wGoal );
	if( pGoal != NULL )
	{
		try
		{
			CAIGoal *pNewGoal = pGoal->CopyGoal();
			m_plGoalList->AddTail( (CObject *)pNewGoal );
		}
		catch( CException e )
		{
			throw(ERR_CAI_BAD_NEW);
		}

		// add tasks of this goal to local task list
		InitTasks( pGoal );
	}
}

//
// perform new game initial setup, or existing game restart
//
void CAIGoalMgr::Initialize( void )
{
	ASSERT_VALID( this );
	
	if( plGoalList == NULL )
		return;

	ASSERT_VALID( plGoalList );
    
	// create new goal list for this player
	try
	{
		m_plGoalList = new CAIGoalList();
	}
	catch( CException e )
	{
		if( m_plGoalList != NULL )
		{
			delete m_plGoalList;
			m_plGoalList = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}


	// set up initial goals from standard list of goals
	// by using the game difficulty setting to select
	// an initial set of goals, and loading them for
	// use by this goal manager
	plGoalList->Initialize( m_plGoalList );

	m_bGoalChange = TRUE;

	// BUGBUG need to access CPlayer::GetAttrib() for each NUM_RACE_CHUNKS
	// to obtain characteristics of this player and save them
	InitPlayer();
	InitializeTasks();
	
	InitVehSummary();
	InitVehGoals();
	InitBldgSummary();
	InitBldgGoals();
	InitMatOnHand();
	InitMatGoals();
}

void CAIGoalMgr::InitPlayer( void )
{
/*
	enum RACE {
				build_bldgs,
				manf_materials,
				manf_vehicles,
				mine_prod,
				farm_prod,
				research,
				pop_grow,
				pop_die,
				pop_eat,
				attack,
				defense,
				num_race };
*/
	try
	{
		// new way - 
		m_iNumAttribs = CRaceDef::num_race;
		m_pwaAttribs = new WORD[m_iNumAttribs];
		
		for( int i=0; i<m_iNumAttribs; ++i )
		{
			m_pwaAttribs[i] = 0;
		}
	}
	catch( CException e )
	{
		if( m_pwaAttribs != NULL )
		{
			delete [] m_pwaAttribs;
			m_pwaAttribs = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}

	// get current player attributes
	if( !m_bRestart )
	{
		EnterCriticalSection (&cs);
		CPlayer const *pPlayer = pGameData->GetPlayerData( m_iPlayer );
		if( pPlayer != NULL )
		{
			for( int i=0; i<m_iNumAttribs; ++i )
			{
			m_pwaAttribs[i] = (WORD)(pPlayer->m_InitData.GetRace(i) * 100);
			}
		}
		LeaveCriticalSection (&cs);
	}

/*
new way

	enum RACE {
				build_bldgs,
				manf_materials,
				manf_vehicles,
				mine_prod,
				farm_prod,
				research,
				pop_grow,
				pop_die,
				pop_eat,
				attack,
				defense,
				num_race };

*/
	try
	{
		//m_iNumRatios = theTransports.GetNumTransports();
		m_iNumRatios = CTransportData::num_types;
		m_pwaRatios = new WORD[m_iNumRatios];
		
		for( int i=0; i<m_iNumRatios; ++i )
		{
			// all ratios default to 1 so that goal counts
			// and opfor counts effect production on a 1:1
			// basis at a minimum
			m_pwaRatios[i] = 1;
		}
	}
	catch( CException e )
	{
		if( m_pwaRatios != NULL )
		{
			delete [] m_pwaRatios;
			m_pwaRatios = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}

	// loaded games do not need initialization that follows
	if( m_bRestart )
		return;

	// now initialize needed vehicle ratio per type factory
	// based on deviation from 100 for certain attributes
	//
	// combat_vehicle_ratio = 1;
	//
	int iRatio = 1;
	int iBase = 100;
	if( m_pwaAttribs[CRaceDef::attack] > iBase )
		iRatio += (m_pwaAttribs[CRaceDef::attack] - iBase) / 10;
	if( m_pwaAttribs[CRaceDef::defense] > iBase )
		iRatio += (m_pwaAttribs[CRaceDef::defense] - iBase) / 10;

	// now set the various combat vehicle ratios
	/*
		light_scout,
		med_scout,
		heavy_scout,
		infantry_carrier,
		light_tank,
		med_tank,
		heavy_tank,
		light_art,
		med_art,
		heavy_art,
		infantry,
		rangers,
		marines, 
	*/
	m_pwaRatios[CTransportData::light_scout] = 1;
	m_pwaRatios[CTransportData::med_scout] = iRatio;
	m_pwaRatios[CTransportData::heavy_scout] = iRatio;
	m_pwaRatios[CTransportData::infantry_carrier] = iRatio;
	m_pwaRatios[CTransportData::light_tank] = iRatio;
	m_pwaRatios[CTransportData::med_tank] = iRatio;
	m_pwaRatios[CTransportData::heavy_tank] = iRatio;
	m_pwaRatios[CTransportData::light_art] = iRatio;
	m_pwaRatios[CTransportData::med_art] = iRatio;
	m_pwaRatios[CTransportData::heavy_art] = iRatio;
	m_pwaRatios[CTransportData::infantry] = iRatio;
	m_pwaRatios[CTransportData::rangers] = iRatio;
	m_pwaRatios[CTransportData::marines] = iRatio;

	// transport_truck_ratio = 1;
	iRatio = 1;
	if( m_pwaAttribs[CRaceDef::manf_vehicles] > iBase )
		iRatio += (m_pwaAttribs[CRaceDef::manf_vehicles] - iBase) / 10;
	if( m_pwaAttribs[CRaceDef::build_bldgs] > iBase )
		iRatio += (m_pwaAttribs[CRaceDef::build_bldgs] - iBase) / 10;
	iRatio /= 2;
	/*
		light_truck,		// CUnit::m_iType
		med_truck,
		heavy_truck,
		light_cargo,
		med_cargo,
		heavy_cargo,
	*/
	m_pwaRatios[CTransportData::med_truck] = iRatio;
	m_pwaRatios[CTransportData::heavy_truck] = iRatio;
	m_pwaRatios[CTransportData::light_cargo] = iRatio;

	//
	// now set up research topic path
	//
	int *piPath = NULL;

	//if( m_pwaAttribs[CRaceDef::attack] > iBase )
	//	piPath = aiCbtPath;
	//else
	//	piPath = aiEricPath;

	// randomly pick from experts
	int iPath = pGameData->GetRandom(iBase);
	if( iPath < 33 )
		piPath = aiCbtPath;
	else if( iPath < 66 )
		piPath = aiKeithPath;
	else
		piPath = aiEricPath;

	for( int i=0; i<CRsrchArray::num_types; ++i )
		m_iRDPath[i] = piPath[i];
}

//
// perform new game initial setup, or existing game restart
//
void CAIGoalMgr::InitializeTasks( void )
{
	ASSERT_VALID( this );

		// make sure the task list is clean
		if( m_plTasks != NULL )
			DeleteTasks();

		// create new task list for this mgr
		try
		{
			m_plTasks = new CAITaskList();
		}
		catch( CException e )
		{
			// need to report this is a problem
			DeleteTasks();
			throw(ERR_CAI_BAD_NEW);
		}

		// loaded games do not need initialization
		if( m_bRestart )
			return;
		
		// interate thru the local goal mgr's goal list
		// and create tasks associated with that goal
		CAIGoal *pGoal = FirstGoal();
		while( pGoal != NULL )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

			InitTasks( pGoal );
			pGoal = NextGoal();
		}
}
//
// create the tasks associated with the passed goal
// and add them to the task mgr's task list
//
void CAIGoalMgr::InitTasks( CAIGoal *pGoal )
{
	ASSERT_VALID( this );
	ASSERT_VALID( pGoal );
	
	if( pGoal != NULL )
	{
		// now for each task of this goal, 
		// copy it from the standard list,
		// and add it to this task mgr's task list
		int i=0;
		int t=0;
		WORD wQty = 0;
		WORD wTaskID = pGoal->GetTaskAt(t);
		while( wTaskID )
		{
			// get from standard task list
			CAITask *pTask = plTaskList->GetTask( 
				wTaskID, FALSE );

			if( pTask != NULL )
			{
#if 0 //THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				ASSERT_VALID( pTask );

				// copy this task information to a new task
				CAITask *pNewTask = pTask->CopyTask();
				pNewTask->SetGoalID( pGoal->GetID() );

				// if a build apt or build office then
				// select a type of apt/building to build
				// when either of them is needed
				if( pNewTask->GetID() == IDT_BUILDAPTS )
				{
					WORD wBldg = CStructureData::apartment_1_1
						+ pGameData->GetRandom(3);
					
					pNewTask->SetTaskParam(BUILDING_ID,wBldg);
						//CStructureData::apartment_2_2);
				}

				if( pNewTask->GetID() == IDT_BUILDOFFICE )
				{
					WORD wBldg = CStructureData::office_2_1
						+ pGameData->GetRandom(3);

					pNewTask->SetTaskParam(BUILDING_ID,wBldg);
						//CStructureData::office_2_4);
				}

				// for AI Player add up to difficulty
				// level number of additional R&D centers
				if( pNewTask->GetID() == IDT_BUILDRD ||
					pNewTask->GetID() == IDT_BUILDREFINERY )
				{
					wQty = pNewTask->GetTaskParam(PRODUCTION_QTY)
						+ pGameData->m_iSmart;
					pNewTask->SetTaskParam(PRODUCTION_QTY,wQty);
				}

				if( pNewTask->GetTaskParam(ORDER_TYPE) == PRODUCTION_ORDER )
				{
					if( pNewTask->GetTaskParam(PRODUCTION_TYPE) == 
						CStructureData::UTvehicle ||
						pNewTask->GetTaskParam(PRODUCTION_TYPE) == 
						CStructureData::UTshipyard )
					{
						// get id of the vehicle being produced
						int iVeh = pNewTask->GetTaskParam(PRODUCTION_ITEM);
						if( iVeh < m_iNumUnits )
						{
						// add test to confirm this can be produced by this player
						// and set no priority and leave if not able to build yet
							if( !CanBuildVehType(iVeh) )
								pNewTask->SetPriority( (BYTE)0 );
						}
					}
				}

				if( pNewTask->GetTaskParam(ORDER_TYPE) == CONSTRUCTION_ORDER )
				{
					if( pNewTask->GetOrderID() == CNetCmd::build_bldg )
					{
						// get id of building to build from the task
						int iBldg = (int)pNewTask->GetTaskParam(BUILDING_ID);
						if( iBldg < m_iNumBldgs )
						{
							// add test to confirm this can be built by this player
							BOOL bCanBuild = FALSE;
							// by getting a pointer to that type of building
							CStructureData const *pBldgData = 
								pGameData->GetStructureData( iBldg );
							if( pBldgData == NULL )
								continue;

							// then a pointer to this player
							EnterCriticalSection (&cs);
							CPlayer *pPlayer = 
								pGameData->GetPlayerData( m_iPlayer );
							if( pPlayer == NULL )
							{
								LeaveCriticalSection (&cs);
								continue;
							}
							// get the opinion of the game based on this player
							bCanBuild = pBldgData->PlyrIsDiscovered(pPlayer);
							LeaveCriticalSection (&cs);

							if( !bCanBuild )
								pNewTask->SetPriority( (BYTE)0 );
						}
					}
				}

				// on initialization only,
				// allow game difficulty to influence 
				// the number of vehicles initially set
				// in a vehicle production task
				if( !m_bRestart )
				{
					if( pNewTask->GetTaskParam(ORDER_TYPE) == 
						PRODUCTION_ORDER &&
						pNewTask->GetTaskParam(PRODUCTION_TYPE) == 
						CStructureData::UTvehicle )
					{
						wQty = pNewTask->GetTaskParam(PRODUCTION_QTY)
							+ (WORD)pGameData->m_iSmart;
						pNewTask->SetTaskParam(PRODUCTION_QTY,wQty);
					}
				}

				// save it locally
				m_plTasks->AddTail( (CObject *)pNewTask );
			}
			// get the next task of this goal
			wTaskID = pGoal->GetTaskAt( ++t );
		}
	}
}

void CAIGoalMgr::InitMatOnHand( void )
{
	try
	{
		m_iNumMats = CMaterialTypes::num_types;
		m_pwaMatOnHand = new WORD[m_iNumMats];
		//m_pwaMatOnHand->SetSize( CMaterialTypes::num_types );
		for( int i=0; i<m_iNumMats; ++i )
		{
#if 0 //THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			m_pwaMatOnHand[i] = 0;
		}
	}
	catch( CException e )
	{
		if( m_pwaMatOnHand != NULL )
		{
			//m_pwaMatOnHand->RemoveAll();
			delete [] m_pwaMatOnHand;
			m_pwaMatOnHand = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}
}

void CAIGoalMgr::InitMatGoals( void )
{
	try
	{
		m_iNumMats = CMaterialTypes::num_types;
		m_pwaMatGoals = new WORD[m_iNumMats];
		
		for( int i=0; i<m_iNumMats; ++i )
		{
#if 0 //THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			m_pwaMatGoals[i] = 0;
		}
	}
	catch( CException e )
	{
		if( m_pwaMatGoals != NULL )
		{
			//m_pwaMatGoals->RemoveAll();
			delete [] m_pwaMatGoals;
			m_pwaMatGoals = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}
}

void CAIGoalMgr::InitBldgGoals( void )
{
	try
	{
		
		m_pwaBldgGoals = new WORD[m_iNumBldgs];
		
		for( int i=0; i<m_iNumBldgs; ++i )
		{
#if 0 //THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			m_pwaBldgGoals[i] = 0;
		}
	}
	catch( CException e )
	{
		if( m_pwaBldgGoals != NULL )
		{
			//m_pwaBldgGoals->RemoveAll();
			delete [] m_pwaBldgGoals;
			m_pwaBldgGoals = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}
}

void CAIGoalMgr::InitBldgSummary( void )
{
	// create summary count array for buildings
	try
	{
		//m_iNumBldgs = theStructures.GetNumBuildings();
		m_iNumBldgs = CStructureData::num_types;
		m_pwaBldgs = new WORD[m_iNumBldgs];
		//m_pwaBldgs->SetSize( theStructures.GetNumBuildings() );
		for( int i=0; i<m_iNumBldgs; ++i )
		{
#if 0 //THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			m_pwaBldgs[i] = 0;
		}
	}
	catch( CException e )
	{
		if( m_pwaBldgs != NULL )
		{
			//m_pwaBldgs->RemoveAll();
			delete [] m_pwaBldgs;
			m_pwaBldgs = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}

	// loaded games do not need initialization
	if( m_bRestart )
		return;
		
	// now go thru the m_plUnits looking for buildings
	// and count up each type of buildings
	ASSERT_VALID( m_plUnits );

   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a vehicle
			if( pUnit->GetType() == CUnit::vehicle )
				continue;

			// get access to CBuilding copied data for building
			CAICopy *pCopyCBuilding = 
				pUnit->GetCopyData( CAICopy::CBuilding );
			if( pCopyCBuilding == NULL )
				continue;

			if( pCopyCBuilding->m_aiDataOut[CAI_TYPEBUILD]
				< m_iNumBldgs )
			{
				int iAt = pCopyCBuilding->m_aiDataOut[CAI_TYPEBUILD];
				WORD wCnt = m_pwaBldgs[iAt] + 1;
				m_pwaBldgs[iAt] = wCnt;
			}
		}
	}
}

void CAIGoalMgr::InitVehGoals( void )
{
	try
	{
		m_pwaVehGoals = new WORD[m_iNumUnits];
		
		for( int i=0; i<m_iNumUnits; ++i )
		{
#if 0 //THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			m_pwaVehGoals[i] = 0;
		}
	}
	catch( CException e )
	{
		if( m_pwaVehGoals != NULL )
		{
			//m_pwaVehGoals->RemoveAll();
			delete [] m_pwaVehGoals;
			m_pwaVehGoals = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}
}

void CAIGoalMgr::ClearVehGoals( void )
{
	if( m_pwaVehGoals != NULL )
	{
		for( int i=0; i<m_iNumUnits; ++i )
		{
#if 0 //THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			m_pwaVehGoals[i] = 0;
		}
	}
}

void CAIGoalMgr::ClearVehSummary( void )
{
	if( m_pwaUnits != NULL )
	{
		for( int i=0; i<m_iNumUnits; ++i )
		{
#if 0 //THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			m_pwaUnits[i] = 0;
		}
	}
}

void CAIGoalMgr::InitVehSummary( void )
{
	// create summary count array for vehicles
	try
	{
		//m_iNumUnits = theTransports.GetNumTransports();
		m_iNumUnits = CTransportData::num_types;
		m_pwaUnits = new WORD[m_iNumUnits];
		
		for( int i=0; i<m_iNumUnits; ++i )
		{
			m_pwaUnits[i] = 0;
		}
	}
	catch( CException e )
	{
		if( m_pwaUnits != NULL )
		{
			//m_pwaUnits->RemoveAll();
			delete [] m_pwaUnits;
			m_pwaUnits = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}

	// loaded games do not need initialization
	if( m_bRestart )
		return;
		
	// now go thru the m_plUnits looking for vehicles
	// and count up each type of unit
	ASSERT_VALID( m_plUnits );

   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a vehicle
			if( pUnit->GetType() != CUnit::vehicle )
				continue;

			int iAt = pUnit->GetTypeUnit();
			if( iAt < m_iNumUnits )
			{
				WORD wCnt = m_pwaUnits[iAt] = 1;
				m_pwaUnits[iAt] = wCnt;
			}
		}
	}
}
//
// used to start interating thru this mgr's goal list
//
CAIGoal *CAIGoalMgr::FirstGoal( void )
{
	ASSERT_VALID( this );
	ASSERT_VALID( m_plGoalList );

	m_pos = m_plGoalList->GetHeadPosition();
	if( m_pos != NULL )
		return( (CAIGoal *)m_plGoalList->GetNext( m_pos ) );
	return NULL;
}
//
// used to obtain the next goal in this mgr's goal list
//
CAIGoal *CAIGoalMgr::NextGoal( void )
{
	ASSERT_VALID( this );
	ASSERT_VALID( m_plGoalList );

	if( m_pos != NULL )
		return( (CAIGoal *)m_plGoalList->GetNext( m_pos ) );
	return NULL;
}
CAIGoal *CAIGoalMgr::GetGoal( WORD wID )
{
	ASSERT_VALID( this );
	ASSERT_VALID( m_plGoalList );

	return( (CAIGoal *)m_plGoalList->GetGoal( wID ) );
}

void CAIGoalMgr::AddTask( CAITask *pTask )
{
	ASSERT_VALID( this );
	ASSERT_VALID( pTask );
	ASSERT_VALID( m_plTasks );

	if( m_plTasks != NULL && pTask != NULL )
		m_plTasks->AddTail( (CObject *)pTask );
}

void CAIGoalMgr::RemoveTask( CAITask *pTask )
{
	ASSERT_VALID( this );
	ASSERT_VALID( pTask );
	ASSERT_VALID( m_plTasks );

	m_plTasks->RemoveTask( pTask->GetID(), pTask->GetGoalID() );
}

//
// the unit passed is a production factory of some type that may
// have a task already assigned, so make sure that all tasks for 
// production needed for all staging have been assigned, to all
// factories that can be assigned, and that all vehicles needed
// for staging, that can be produced, are being produced.
//
// if all these conditions are being met, then allow priority
// values of tasks to help select a task for this unit
//
CAITask *CAIGoalMgr::GetProductionTask(CAIUnit *pUnit)
{
	CAITask *pPickedTask = NULL;

	// determine what vehicles are needed for staging
	WORD awTypes[CTransportData::num_types];
	for( int i=0; i<CTransportData::num_types; ++i )
		awTypes[i] = 0;
	SetAssaultStagingVehicle( awTypes );

	// go through the factories and zero out awTypes[] for
	// those vehicles that are needed to be staged, which
	// are currently in production at a factory
	ProducingStagingVehicle( awTypes );

	// now find the highest priority production task this factory
	// and player can produce that is still needed for staging
	int iBest = 0;


    POSITION pos = m_plTasks->GetHeadPosition();
    while( pos != NULL )
    {   
        CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

			// only interested in unassigned tasks
			if( pTask->GetStatus() != UNASSIGNED_TASK )
				continue;

			// that produce something
			if( pTask->GetTaskParam(ORDER_TYPE) == PRODUCTION_ORDER )
			{
				if( pTask->GetTaskParam(PRODUCTION_TYPE) == 
					CStructureData::UTvehicle ||
					pTask->GetTaskParam(PRODUCTION_TYPE) == 
					CStructureData::UTshipyard )
				{
					// can this vehicle, from the task, be produced at this factory
					if( pTask->GetTaskParam(PRODUCTION_ID1) != 
						pUnit->GetTypeUnit() &&
						pTask->GetTaskParam(PRODUCTION_ID2) != 
						pUnit->GetTypeUnit() )
						continue;

					// get id of the vehicle being produced
					int iVeh = pTask->GetTaskParam(PRODUCTION_ITEM);
					if( iVeh >= m_iNumUnits )
						continue;

					// not being staged
					if( !awTypes[iVeh] )
						continue;

					// produced enough of this type
					if( m_pwaUnits[iVeh] >
						m_pwaVehGoals[iVeh] )
						continue;

#if THREADS_ENABLED
		// BUGBUG this function must yield
					myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

					// test to confirm this can be produced by this player
					if( !CanBuildVehType(iVeh) )
						continue;

					if( pTask->GetPriority() > iBest )
					{
						iBest = pTask->GetPriority();
						pPickedTask = pTask;
					}
				}
			}
		}
	}

	// if the task has not been assigned yet, then fall back to old way
	if( pPickedTask == NULL )
		pPickedTask = m_plTasks->GetProductionTask(pUnit->GetTypeUnit());

	return(pPickedTask);
}

//
// an IDT_PREPAREWAR task has completed, and now the staged units
// must be ordered to assault the city of an opfor
//
void CAIGoalMgr::LaunchAssault( CAITask *pTask )
{
	if( !m_plOpFors->GetCount() )
	{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::LaunchAssault() player %d goal %d task %d no opfors ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID() );
#endif
		return;  // could not launch, just return?
	}

	CHexCoord hexCity(pTask->GetTaskParam(CAI_LOC_X),
		pTask->GetTaskParam(CAI_LOC_Y));

	// determine this AI's attitude, each time, because the human player
	// may have changed the game's difficulty, or the AI may have made
	// an attribute adjustment of its own to change its play style
	int iAttitude = 0;
	int iBase = 100; // this is the base attribute value
	if( m_pwaAttribs[CRaceDef::attack] > iBase )
		iAttitude += m_pwaAttribs[CRaceDef::attack] - iBase;
	if( m_pwaAttribs[CRaceDef::defense] > iBase )
		iAttitude += m_pwaAttribs[CRaceDef::defense] - iBase;

	// now apply difficulty factor
	iAttitude += (pGameData->m_iSmart * iAttitude);

	// if there is only one opfor known, then based on relations,
	// this AI's combat attribute and a fuzzy factor, launch an attack
	// or a 'recon in strength' toward them
	if( m_plOpFors->GetCount() == 1 )
	{
        //POSITION pos = m_plOpFors->GetHeadPosition();
        //if( pos != NULL )
		CAIOpFor *pOpFor = (CAIOpFor *)m_plOpFors->GetHead();
		if( pOpFor != NULL )
        {   
			// on higher difficulty, automatic alliance with other AI players!
			//
			if( pGameData->m_iSmart &&
				pOpFor->IsAI() && !pOpFor->AtWar() )
			{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::LaunchAssault() player %d goal %d task %d opfor is AI ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID() );
#endif
				return;  // could not launch, just return?
			}

			FindAssaultTarget( hexCity, pTask, pOpFor );
			if( hexCity.X() == pTask->GetTaskParam(CAI_LOC_X) && 
				hexCity.Y() == pTask->GetTaskParam(CAI_LOC_Y) )
			{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::LaunchAssault() player %d goal %d task %d no opfor target ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID() );
#endif
				return;
			}

			if( !IsTargetReachable( hexCity, pTask ) )
			{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::LaunchAssault() player %d goal %d task %d can't reach target ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID() );
#endif
				// must do something with the units assigned to this
				// this assault, because not being able to reach the
				// target will not change by itself
				CancelAssault( pTask );
				
				return;
			}

			// if not at war, then based on this AI's combat attribute
			// and the difficulty level of the game, make a fuzzy decision
			// to go ahead and launch, or just recon in strength
			if( pOpFor->AtWar() )
			{
				LaunchAssault(pTask,hexCity);
			}
			else // not at war, yet
			{
				if( pTask->GetStatus() != UNASSIGNED_TASK ||
					pGameData->GetRandom(iBase) < iAttitude )
					LaunchAssault(pTask,hexCity);
				else
					ReconInForce(pTask,hexCity);
			}
		}

		// flag the task to indicate it is staged and thus ready
		// to go to the assault destination determined in either
		// LaunchAssault() and ReconInForce()
		if( pTask->GetStatus() != COMPLETED_TASK )
			pTask->SetStatus(INPROCESS_TASK);

		return;  // could not launch, just return?
	}

	// if there is more than one opfor known, then attack the one
	// with the worst relations.
	int iMeanest = (int)ALLIANCE;
	int iOpforID = 0;
    POSITION pos = m_plOpFors->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIOpFor *pOpFor = (CAIOpFor *)m_plOpFors->GetNext( pos );
        if( pOpFor != NULL )
        {
           	ASSERT_VALID( pOpFor );

			if( pOpFor->IsAI() && !pOpFor->AtWar() )
				continue;

			if( pOpFor->GetAttitude() < iMeanest )
			{
				iMeanest = pOpFor->GetAttitude();
				iOpforID = pOpFor->GetPlayerID();
			}
		}
	}
	if( !iOpforID )
		return;  // could not launch, just return?
	
	CAIOpFor *pOpFor = m_plOpFors->GetOpFor(iOpforID);
	if( pOpFor == NULL )
		return;  // could not launch, just return?

	FindAssaultTarget( hexCity, pTask, pOpFor );
	if( hexCity.X() == pTask->GetTaskParam(CAI_LOC_X) && 
		hexCity.Y() == pTask->GetTaskParam(CAI_LOC_Y) )
		return;

	if( !IsTargetReachable( hexCity, pTask ) )
		return;

	// if not at war, then based on this AI's combat attribute
	// and the difficulty level of the game, make a fuzzy decision
	// to go ahead and launch, or just recon in strength
	if( pOpFor->AtWar() )
	{
		LaunchAssault(pTask,hexCity);
	}
	else // not at war, yet
	{
		// make a random decision, relative to attitude
		if( pTask->GetStatus() != UNASSIGNED_TASK ||
			pGameData->GetRandom(iBase) < iAttitude )
			LaunchAssault(pTask,hexCity);
		else
			ReconInForce(pTask,hexCity);
	}

	// flag the task to indicate it is staged and thus ready
	// to go to the assault destination determined in either
	// LaunchAssault() and ReconInForce()
	if( pTask->GetStatus() != COMPLETED_TASK )
		pTask->SetStatus(INPROCESS_TASK);
}
//
// when attack is launched, the hcTo has opfor's rocket location,
// then each staged unit, finds a staging hex near the destination
// and is ordered to proceed to the staging hex and task is changed
// to appropriate SEEK* task
//
void CAIGoalMgr::LaunchAssault( CAITask *pTask, CHexCoord hcTo )
{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::LaunchAssault() player %d goal %d task %d at %d,%d ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID(), hcTo.X(), hcTo.Y() );
#endif

	// need to have a flag set that says this assault is
	// there, and now needs to switch to IDT_SEEK* task and do it!

	WORD wNewTask;
	int iTypeVeh;
	// a water based raid
	if( pTask->GetGoalID() == IDG_PIRATE ||
		pTask->GetGoalID() == IDG_SEAWAR )
	{
		wNewTask = IDT_SEEKATSEA;
		// using this vehicle because it needs the deepest water
		iTypeVeh = CTransportData::cruiser;
	}
	else if( pTask->GetGoalID() == IDG_SEAINVADE )
	{
		wNewTask = IDT_SEEKATSEA;
		// using this vehicle because it needs to reach shore
		iTypeVeh = CTransportData::landing_craft;
	}
	else // a land based raid
	{
		if( pTask->GetGoalID() == IDG_LANDWAR )
			wNewTask = IDT_SEEKINWAR;
		else if( pTask->GetGoalID() == IDG_ADVDEFENSE )
			wNewTask = IDT_SEEKINRANGE;
		else
			wNewTask = IDT_PATROL;

		// using this vehicle because spotting = 10 and wheel = 1
		iTypeVeh = CTransportData::med_scout;
	}

	CTransportData const *pVehData = 
		pGameData->GetTransportData( iTypeVeh );
	if( pVehData == NULL )
		return;

	CHexCoord hexDest = hcTo;
	// just get the units into the area, they will go on from there
	int iSpotting = pVehData->_GetSpottingRange();

	// right now pick a staging hex instead of a focus hex
	//m_pMap->m_pMapUtil->FindStagingHex( hcTo, iSpotting, iSpotting,
	//	 iTypeVeh, hexDest, FALSE );
	// make sure we got something new
	//if( hexDest == hcTo )
	//	return;

	// get size of previous staging area
	CHexCoord hex;
	int iDeltaX = hex.Wrap(pTask->GetTaskParam(CAI_PREV_X) -
		pTask->GetTaskParam(CAI_LOC_X));
	int iDeltaY = hex.Wrap(pTask->GetTaskParam(CAI_PREV_Y) -
		pTask->GetTaskParam(CAI_LOC_Y));

	// now use hcStart and hcEnd with deltas to define new areas
	CHexCoord hcStart, hcEnd;
	hcStart.X( hcStart.Wrap(hexDest.X() - (iDeltaX/2)) );
	hcStart.Y( hcStart.Wrap(hexDest.Y() - (iDeltaY/2)) );
	hcEnd.X( hcStart.Wrap(hexDest.X() + (iDeltaX/2)) );
	hcEnd.Y( hcStart.Wrap(hexDest.Y() + (iDeltaY/2)) );

	// finally for each unit with this goal/task set its destination
	// to be a location within the new area, that is not already 
	// selected to be the destination of another unit on the task
	UpdateTaskForce( pTask, hcStart, hcEnd, wNewTask );
}
//
// instead of going right to the heart of the opfor city,
// go to a midpoint and keep the IDT_PREPAREWAR task in
// place, and record new staging area, and send all units
// of this task to the new staging area, and also consider
// that the destination may need to be CHex::ocean or can't
// be CHex::ocean based on a water/land staging
//
void CAIGoalMgr::ReconInForce( CAITask *pTask, CHexCoord hcTo )
{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::ReconInForce() player %d goal %d task %d at %d,%d ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID(), hcTo.X(), hcTo.Y() );
#endif

	// first, use location of base & destination, to find midpoint 
	CHexCoord hcBase( m_pMap->m_iBaseX,m_pMap->m_iBaseY );
	CHexCoord hcMid, hex, hcStart, hcEnd;
	int iDist = hcBase.Diff( (hcBase.X() - hcTo.X()) );
	if( iDist < 0 )
	{
		iDist *= -1;
		hcMid.X( hex.Wrap(hcBase.X() + (iDist/2)) );
	}
	else
	{
		hcMid.X( hex.Wrap(hcTo.X() + (iDist/2)) );
	}
	iDist = hcBase.Diff( (hcBase.Y() - hcTo.Y()) );
	if( iDist < 0 )
	{
		iDist *= -1;
		hcMid.Y( hex.Wrap(hcBase.Y() + (iDist/2)) );
	}
	else
	{
		hcMid.Y( hex.Wrap(hcTo.Y() + (iDist/2)) );
	}

	if( pTask->GetGoalID() == IDG_PIRATE ||
		pTask->GetGoalID() == IDG_SEAWAR ||
		pTask->GetGoalID() == IDG_SEAINVADE )
	{
		// hcMid must be CHex::ocean
		// get terrain from game data
		CHex *pGameHex = theMap.GetHex( hcMid );
		if( pGameHex != NULL )
		{
			// and if it is not ocean, then find some ocean
			if( pGameHex->GetType() != CHex::ocean )
			{
				hex = hcMid;
				m_pMap->m_pMapUtil->FindNearestWater( hcMid );
				// was water found?
				if( hex == hcMid )
					return; // what else can I do at this point???
			}

			CTransportData const *pVehData = 
				pGameData->GetTransportData( CTransportData::cruiser );
			if( pVehData == NULL )
				return; // what else can I do at this point???

			// get some patrol points from the area, that this ship
			// can travel
			CAIHex *pHexes = m_pMap->m_pMapUtil->FindWaterPatrol(
				hcMid, pVehData );
			if( pHexes == NULL )
				return; // what else can I do at this point???

			// pHexes contains the points that should be the bounds of
			// the CHex::ocean staging area for the recon in strength
			// and converting them to 2 coordinates in rect style
			ConvertPatrolPoints( pHexes, hcStart, hcEnd );
			delete [] pHexes;

			// now set task params with new area to stage in
			pTask->SetTaskParam(CAI_LOC_X,hcStart.X());
			pTask->SetTaskParam(CAI_LOC_Y,hcStart.Y());
			pTask->SetTaskParam(CAI_PREV_X,hcEnd.X());
			pTask->SetTaskParam(CAI_PREV_Y,hcEnd.Y());
		}
		else
			return; // what else can I do at this point???
	}
	else
	{
		// hcMid can't be CHex::ocean
		CHex *pGameHex = theMap.GetHex( hcMid );
		if( pGameHex != NULL )
		{
			// and if it is ocean, find land
			if( pGameHex->GetType() == CHex::ocean ||
				pGameHex->GetType() == CHex::river )
			{
				hex = hcMid;
				// actually find the nearest non-water
				m_pMap->m_pMapUtil->FindNearestWater( hcMid,FALSE );
				// was land found?
				if( hex == hcMid )
					return; // what else can I do at this point???
			}
			// get size of previous staging area
			int iDeltaX = hcMid.Wrap(pTask->GetTaskParam(CAI_PREV_X) -
				pTask->GetTaskParam(CAI_LOC_X));
			int iDeltaY = hcMid.Wrap(pTask->GetTaskParam(CAI_PREV_Y) -
				pTask->GetTaskParam(CAI_LOC_Y));
			//int iDeltaX = abs( hcMid.Diff( pTask->GetTaskParam(CAI_PREV_X) -
			//	pTask->GetTaskParam(CAI_LOC_X)));
			//int iDeltaY = abs( hcMid.Diff( pTask->GetTaskParam(CAI_PREV_Y) -
			//	pTask->GetTaskParam(CAI_LOC_Y)));

			// now use hcStart and hcEnd with deltas to define new area
			hcStart.X( hex.Wrap(hcMid.X() - (iDeltaX/2)) );
			hcStart.Y( hex.Wrap(hcMid.Y() - (iDeltaY/2)) );
			hcEnd.X( hex.Wrap(hcMid.X() + (iDeltaX/2)));
			hcEnd.Y( hex.Wrap(hcMid.Y() + (iDeltaY/2)));

			// now set task params with new area to stage in
			pTask->SetTaskParam(CAI_LOC_X,hcStart.X());
			pTask->SetTaskParam(CAI_LOC_Y,hcStart.Y());
			pTask->SetTaskParam(CAI_PREV_X,hcEnd.X());
			pTask->SetTaskParam(CAI_PREV_Y,hcEnd.Y());
		}
		else
			return; // what else can I do at this point???
	}

	// finally for each unit with this goal/task set its destination
	// to be a location within the new area, that is not already 
	// selected to be the destination of another unit on the task

	UpdateTaskForce( pTask, hcStart, hcEnd, 0 );
}

//
// the target selected for this assault was not reachable so
// this task needs to be reset
//
void CAIGoalMgr::CancelAssault( CAITask *pTask )
{
	CHexCoord hexVeh;
	BOOL bHasOnBoard;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::CancelAssault() player %d goal %d task %d ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID() );
#endif

	pTask->SetStatus(COMPLETED_TASK);

   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetTask() == pTask->GetID() &&
				pUnit->GetGoal() == pTask->GetGoalID() )
			{
#if THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
#endif
				// clear this unit
				pUnit->SetTask(0);
				pUnit->SetGoal(0);
				pUnit->ClearParam();
				pUnit->SetDataDW(0);

				hexVeh.X(0);
				hexVeh.Y(0);

				bHasOnBoard = FALSE;

				EnterCriticalSection (&cs);
				CVehicle *pVehicle = 
					theVehicleMap.GetVehicle( pUnit->GetID() );
				if( pVehicle != NULL )
				{
					// consider that this unit may be carrying other units
					// and if so, then be sure it does an unload 
					if( pVehicle->GetCargoCount() )
						bHasOnBoard = TRUE;

					hexVeh = pVehicle->GetHexHead();
				}
				LeaveCriticalSection (&cs);

				if( !hexVeh.X() && !hexVeh.Y() )
					continue;

				if( bHasOnBoard )
					pUnit->UnloadCargo();

				m_pMap->m_pMapUtil->FindStagingHex( hexVeh,
					1, 1, pUnit->GetTypeUnit(), hexVeh );

				pUnit->SetDestination( hexVeh );

				m_pMap->PlaceFakeVeh( hexVeh, pUnit->GetTypeUnit() );
			}
		}
	}
}

//
// for each unit with this goal/task set its destination
// to be a location within the new area, that is not already 
// selected to be the destination of another unit on the task
//
void CAIGoalMgr::UpdateTaskForce( CAITask *pTask, 
	CHexCoord& hcStart, CHexCoord& hcEnd, WORD wNewTask )
{
	CHexCoord hex = hcStart;
	BOOL bIsOnBoard = FALSE;
	
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			if( pUnit->GetOwner() != m_iPlayer )
				continue;
			
			WORD wStatus = pUnit->GetStatus();
			if( (wStatus & CAI_IN_COMBAT) )
				continue;

			if( pUnit->GetTask() == pTask->GetID() &&
				pUnit->GetGoal() == pTask->GetGoalID() )
			{
				// get the vehicle type of this unit
				CTransportData const *pVehData = 
					pGameData->GetTransportData( pUnit->GetTypeUnit() );
				if( pVehData == NULL )
					continue;

				// consider that this unit may be onboard another unit
				// and if so, then don't give it a destination yet
				bIsOnBoard = FALSE;
				if( pVehData->IsCarryable() )
				{
					EnterCriticalSection (&cs);

					CVehicle *pVehicle = 
						theVehicleMap.GetVehicle( pUnit->GetID() );

					// no vehicle pointer or already carried not a troop unit
					if( pVehicle != NULL )
					{
						if( pVehicle->GetTransport() != NULL )
							bIsOnBoard = TRUE;
						hex = pVehicle->GetHexHead();
					}
					LeaveCriticalSection (&cs);
				}

				// only uncarried units need to move to within the area
				// defined by hcStart and hcEnd
				if( !bIsOnBoard )
				{
					m_pMap->m_pMapUtil->FindStagingHex(hcStart.X(), hcStart.Y(),
						hcEnd.X(), hcEnd.Y(), hex, pUnit->GetTypeUnit(), TRUE );

					pUnit->SetDestination(hex);
					m_pMap->PlaceFakeVeh( hex, pUnit->GetTypeUnit() );
				}


				// mark unit as enroute to staging
				pUnit->SetParam( CAI_UNASSIGNED, (int)INPROCESS_TASK );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateTaskForce() player %d unit %d goal %d task %d new task %d ", 
pUnit->GetOwner(), pUnit->GetID(), pTask->GetGoalID(), pTask->GetID(), wNewTask );
#endif
				// time to change the task to SEEK*
				if( wNewTask )
				{
					// ships only
					if( pVehData->GetBaseType() == CTransportData::ship )
					{
						pUnit->SetTask(IDT_SEEKATSEA);
					}
					// this should include all combat but ships
					else if( pVehData->GetBaseType() != CTransportData::non_combat )
					{
						if( wNewTask != IDT_SEEKATSEA )
							pUnit->SetTask(wNewTask);
						else
							pUnit->SetTask(IDT_SEEKINRANGE);
					}
					else
						pUnit->SetTask(wNewTask);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateTaskForce() player %d unit %d goal %d new task %d ", 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetGoal(), pUnit->GetTask() );
#endif
				}
			}
		}
	}
}

#if 0
//
// for each unit with this goal/task set its destination
// to be a location within the new area, that is not already 
// selected to be the destination of another unit on the task
//
void CAIGoalMgr::UpdateTaskForce( CAITask *pTask, 
	CHexCoord& hcStart, CHexCoord& hcEnd, WORD wNewTask )
{
	CHexCoord hex = hcStart;
	BOOL bIsOnBoard = FALSE;
	BOOL bHasCargo = FALSE;
	
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			if( pUnit->GetOwner() != m_iPlayer )
				continue;
			
			WORD wStatus = pUnit->GetStatus();
			if( (wStatus & CAI_IN_COMBAT) )
				continue;

			if( pUnit->GetTask() == pTask->GetID() &&
				pUnit->GetGoal() == pTask->GetGoalID() )
			{
				// get the vehicle type of this unit
				CTransportData const *pVehData = 
					pGameData->GetTransportData( pUnit->GetTypeUnit() );
				if( pVehData == NULL )
					continue;

				// consider that this unit may be onboard another unit
				// and if so, then don't give it a destination yet
				bIsOnBoard = FALSE;
				bHasCargo = FALSE;
				if( pVehData->IsCarryable() )
				{
					EnterCriticalSection (&cs);

					CVehicle *pVehicle = 
						theVehicleMap.GetVehicle( pUnit->GetID() );

					// no vehicle pointer or already carried not a troop unit
					if( pVehicle != NULL )
					{
						if( pVehicle->m_pTransport != NULL )
							bIsOnBoard = TRUE;
						else if( pVehicle->m_lstCargo.GetCount() )
							bHasCargo = TRUE;
					}
					LeaveCriticalSection (&cs);
				}

				// now find a hex within the area, to which to send it
				while( TRUE && !bIsOnBoard )
				{
#if THREADS_ENABLED
					// BUGBUG this function must yield
					myYieldThread();
					//if( myYieldThread() == TM_QUIT )
					//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
					// found one, send the veh_goto message
					CHex *pGameHex = theMap.GetHex( hex );
					if( pGameHex != NULL &&
						pVehData->CanTravelHex(pGameHex) )
					{
						// check for a building or other vehicle in the hex
						BYTE bUnits = pGameHex->GetUnits();
						if( !(bUnits & CHex::bldg) &&
			    			!(bUnits & (CHex::ul | CHex::ur | 
			    			  CHex::ll | CHex::lr)) )
						{
							// if this is a landing_craft, 
							// it needs to find a coastal hex in the area,
							// and then prepare to unload cargo unit
							// upon reaching it
							if( pVehData->GetType() == 
								CTransportData::landing_craft )
							{
								//CAIHex aiHex( hex.X(), hex.Y() );
								//if( !m_pMap->m_pMapUtil->IsWaterAdjacent( 
								//	hex, 2, 2) )
								if( bHasCargo )
								{
									if( !m_pMap->m_pMapUtil->IsLandingArea(hex) )
										goto NextHex;
								}
							}

							// an assault destination!
							if( !bIsOnBoard )
								pUnit->SetDestination(hex);

							// this will advance the test hex to the next hex
							hex.Xinc();
							if( hex.X() == hcEnd.X() )
							{
								hex.X( hcStart.X() );
								// this will keep going until a hit occurs
								hex.Yinc();
							}
							break; // out of the while(TRUE)
						}
					}

					NextHex:
					// try another hex in the area
					hex.Xinc();
					if( hex.X() == hcEnd.X() )
					{
						hex.X( hcStart.X() );
						hex.Yinc();

						// at the end of the staging area, then
						// just start over again
						if( hex.Y() == hcEnd.Y() )
						{
							hex.Y( hcStart.Y() );
						}
					}
				} // end of while(TRUE)

				// mark unit as enroute to staging
				pUnit->SetParam( CAI_UNASSIGNED, (int)INPROCESS_TASK );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateTaskForce() player %d unit %ld goal %d task %d new task %d ", 
pUnit->GetOwner(), pUnit->GetID(), pTask->GetGoalID(), pTask->GetID(), wNewTask );
#endif
				// time to change the task to SEEK*
				if( wNewTask )
				{
					// ships only
					if( pVehData->GetBaseType() == CTransportData::ship )
					{
						pUnit->SetTask(IDT_SEEKATSEA);
					}
					// this should include all combat but ships
					else if( pVehData->GetBaseType() == CTransportData::combat ||
						pVehData->GetBaseType() == CTransportData::artillery ||
						pVehData->GetBaseType() == CTransportData::troops )
					{
						if( wNewTask != IDT_SEEKATSEA )
							pUnit->SetTask(wNewTask);
						else
							pUnit->SetTask(IDT_SEEKINRANGE);
					}
					else
						pUnit->SetTask(wNewTask);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::UpdateTaskForce() player %d unit %ld goal %d new task %d ", 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetGoal(), pUnit->GetTask() );
#endif
				}
			}
		}
	}
}
#endif

//
// based on the type of assault, and difficulty, select a
// target build to stage to for the assault
//
void CAIGoalMgr::FindAssaultTarget( 
	CHexCoord& hexTarget, CAITask *pTask, CAIOpFor *pOpFor )
{
	int iTarget = 0;
	if( pGameData->m_iSmart > 1 )
	{
		if( pTask->GetGoalID() == IDG_PIRATE ||
			pTask->GetGoalID() == IDG_SEAWAR ||
			pTask->GetGoalID() == IDG_SEAINVADE )
			iTarget = CStructureData::shipyard;
		else
			iTarget = CStructureData::rocket;
	}

	// get the ship hex for seaborne assaults
	pGameData->FindBuilding( 
		iTarget, pOpFor->GetPlayerID(), hexTarget );

	// no target found
	if( hexTarget.X() == pTask->GetTaskParam(CAI_LOC_X) && 
		hexTarget.Y() == pTask->GetTaskParam(CAI_LOC_Y) )
	{
		if( pTask->GetGoalID() == IDG_PIRATE ||
			pTask->GetGoalID() == IDG_SEAWAR ||
			pTask->GetGoalID() == IDG_SEAINVADE )
			iTarget = CStructureData::seaport;
		else
			iTarget = 0;

		// get the ship hex for seaborne assaults
		pGameData->FindBuilding( 
			iTarget, pOpFor->GetPlayerID(), hexTarget );
	}

	// no target found
	if( hexTarget.X() == pTask->GetTaskParam(CAI_LOC_X) && 
		hexTarget.Y() == pTask->GetTaskParam(CAI_LOC_Y) )
	{
		pGameData->FindBuilding( 
			CStructureData::rocket, pOpFor->GetPlayerID(), hexTarget );
	}

	// adjust target to nearest ocean from hexTarget
	int iWhere = CHex::plain;
	if( pTask->GetGoalID() == IDG_PIRATE ||
		pTask->GetGoalID() == IDG_SEAWAR )
		iWhere = CHex::ocean;
	else if( pTask->GetGoalID() == IDG_SEAINVADE )
		iWhere = CHex::coastline;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::FindAssaultTarget() player %d goal %d task %d  near %d,%d  terrain %d", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID(), 
hexTarget.X(), hexTarget.Y(), iWhere );
#endif

	m_pMap->m_pMapUtil->FindAssaultHex( hexTarget, iWhere );
}

// for each corner of the staging area, try to get a path to the
// target, or adjacent to target, and on first yes, return TRUE
//
// using the corners of the staging area, and the type of task
// attempt to determine if the vehicles assigned and staged, can
// reach the target.  If there are vehicles that cannot, then it
// may be that the assault is land based and the target is on island
// or the assault it sea based and the target is too far inland
//
BOOL CAIGoalMgr::IsTargetReachable( CHexCoord& hexTarget, CAITask *pTask )
{
	CTransportData const *pVehData = NULL;
	CHexCoord hex;

	for( int i=0; i<4; ++i )
	{
		switch(i)
		{
		case 0:
			hex.X(pTask->GetTaskParam(CAI_LOC_X));
			hex.Y(pTask->GetTaskParam(CAI_LOC_Y));
			break;
		case 1:
			hex.X(pTask->GetTaskParam(CAI_PREV_X));
			hex.Y(pTask->GetTaskParam(CAI_LOC_Y));
			break;
		case 2:
			hex.X(pTask->GetTaskParam(CAI_PREV_X));
			hex.Y(pTask->GetTaskParam(CAI_PREV_Y));
			break;
		case 3:
			hex.X(pTask->GetTaskParam(CAI_LOC_X));
			hex.Y(pTask->GetTaskParam(CAI_PREV_X));
			break;
		}
	// sea based assault
	//
	if( pTask->GetGoalID() == IDG_PIRATE ||
		pTask->GetGoalID() == IDG_SEAWAR ||
		pTask->GetGoalID() == IDG_SEAINVADE )
	{
		if( pTask->GetGoalID() == IDG_SEAINVADE )
			pVehData = 
				pGameData->GetTransportData( CTransportData::landing_craft );
		else
			pVehData = 
				pGameData->GetTransportData( CTransportData::cruiser );
		if( pVehData == NULL )
			return FALSE; // what else can I do at this point???
		
		// first consider if the mid-point of staging area can be traveled
		CHex *pGameHex = theMap.GetHex( hex );
		if( pGameHex == NULL )
			return FALSE;
		if( !pVehData->CanTravelHex(pGameHex) )
			continue;

		// now consider if the target can be traveled
		pGameHex = theMap.GetHex( hexTarget );
		if( pGameHex == NULL )
			return FALSE;
		if( !pVehData->CanTravelHex(pGameHex) )
		{
			// if not, look for adjacent water
			if( !m_pMap->m_pMapUtil->IsWaterAdjacent( hexTarget, 2, 2) )
				continue;
		}
		// consider if the base type of unit can get from hex->hexTarget
		if( !m_pMap->m_pMapUtil->GetPathRating( 
			hex, hexTarget, pVehData->GetType() ))
			continue;
	}
	else // land based assault
	{
		pVehData = 
			pGameData->GetTransportData( CTransportData::med_scout );
		if( pVehData == NULL )
			return FALSE; // what else can I do at this point???

		// first consider if the mid-point of staging area can be traveled
		CHex *pGameHex = theMap.GetHex( hex );
		if( pGameHex == NULL )
			return FALSE;
		if( !pVehData->CanTravelHex(pGameHex) )
			continue;

		// now consider if the target can be traveled
		pGameHex = theMap.GetHex( hexTarget );
		if( pGameHex == NULL )
			return FALSE;
		if( !pVehData->CanTravelHex(pGameHex) )
			continue;

		// consider if the base type of unit can get from hex->hexTarget
		if( !m_pMap->m_pMapUtil->GetPathRating( 
			hex, hexTarget, pVehData->GetType() ))
			continue;
	}

	return TRUE;
	}
	return FALSE;
}


#if 0
//
// using the corners of the staging area, and the type of task
// attempt to determine if the vehicles assigned and staged, can
// reach the target.  If there are vehicles that cannot, then it
// may be that the assault is land based and the target is on island
// or the assault it sea based and the target is too far inland
//
BOOL CAIGoalMgr::IsTargetReachable( CHexCoord& hexTarget, CAITask *pTask )
{
	// determine mid-point of staging area
	CHexCoord hex;
	int iDeltaX = hex.Wrap(
		pTask->GetTaskParam(CAI_PREV_X) - pTask->GetTaskParam(CAI_LOC_X));
	int iDeltaY = hex.Wrap(
		pTask->GetTaskParam(CAI_PREV_Y) - pTask->GetTaskParam(CAI_LOC_X));
	hex.X( hex.Wrap( pTask->GetTaskParam(CAI_LOC_X) + iDeltaX/2 ));
	hex.Y( hex.Wrap( pTask->GetTaskParam(CAI_LOC_Y) + iDeltaY/2 ));

	CTransportData const *pVehData = NULL;
	// sea based assault
	//
	if( pTask->GetGoalID() == IDG_PIRATE ||
		pTask->GetGoalID() == IDG_SEAWAR ||
		pTask->GetGoalID() == IDG_SEAINVADE )
	{
		if( pTask->GetGoalID() == IDG_SEAINVADE )
			pVehData = 
				pGameData->GetTransportData( CTransportData::landing_craft );
		else
			pVehData = 
				pGameData->GetTransportData( CTransportData::cruiser );
		if( pVehData == NULL )
			return FALSE; // what else can I do at this point???
		
		// first consider if the mid-point of staging area can be traveled
		CHex *pGameHex = theMap.GetHex( hex );
		if( pGameHex == NULL )
			return FALSE;
		if( !pVehData->CanTravelHex(pGameHex) )
			return FALSE;

		// now consider if the target can be traveled
		pGameHex = theMap.GetHex( hexTarget );
		if( pGameHex == NULL )
			return FALSE;
		if( !pVehData->CanTravelHex(pGameHex) )
		{
			// if not, look for adjacent water
			if( !m_pMap->m_pMapUtil->IsWaterAdjacent( hexTarget, 2, 2) )
				return FALSE;
		}
	}
	else // land based assault
	{
		pVehData = 
			pGameData->GetTransportData( CTransportData::med_scout );
		if( pVehData == NULL )
			return FALSE; // what else can I do at this point???

		// first consider if the mid-point of staging area can be traveled
		CHex *pGameHex = theMap.GetHex( hex );
		if( pGameHex == NULL )
			return FALSE;
		if( !pVehData->CanTravelHex(pGameHex) )
			return FALSE;

		// now consider if the target can be traveled
		pGameHex = theMap.GetHex( hexTarget );
		if( pGameHex == NULL )
			return FALSE;
		if( !pVehData->CanTravelHex(pGameHex) )
			return FALSE;

		// consider if the base type of unit can get from hex->hexTarget
		if( !m_pMap->m_pMapUtil->GetPathRating( 
			hex, hexTarget, pVehData->GetType() ))
			return FALSE;
	}

	return TRUE;
}
#endif

//
// convert the points returned (up to 4) to 2 CHexCoords
// that define the largest area
//
void CAIGoalMgr::ConvertPatrolPoints( CAIHex *pHexes, 
	CHexCoord& hcStart, CHexCoord& hcEnd )
{
	int iDeltaX = 0;
	int iDeltaY = 0;
	int iDelta;
	CHexCoord hex;
	// start with the first one
	CAIHex *pHex = &pHexes[0];
	hcStart.X( pHex->m_iX );
	hcStart.Y( pHex->m_iY );
	hcEnd = hcStart;

	for( int i=1; i<NUM_PATROL_POINTS; ++i )
	{
		pHex = &pHexes[i];
		if( !pHex->m_iX	&& !pHex->m_iY )
			continue;

		iDelta = abs( hcStart.X() - pHex->m_iX );
		if( iDelta > iDeltaX )
		{
			hcEnd.X( pHex->m_iX );
			iDeltaX = iDelta;
		}
		iDelta = abs( hcStart.Y() - pHex->m_iY );
		if( iDelta > iDeltaY )
		{
			hcEnd.Y( pHex->m_iY );
			iDeltaY = iDelta;
		}
	}
}
//
// when staging includes infantry, then the area needs to be
// closer to the buildings that produce them
//
void CAIGoalMgr::FindInfStagingArea( CAITask *pTask )
{
	CHexCoord hexBldg(0,0);
	// go thru units, looking for certain buildings:
	// CStructureData::barracks_2
	// CStructureData::barracks_3
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			if( pUnit->GetOwner() != m_iPlayer )
				continue;
			if( pUnit->GetType() != CUnit::building )
				continue;
			// only want buildings that produce infantry,rangers,marines
			if( pUnit->GetTypeUnit() != CStructureData::barracks_2 )
				continue;
				//pUnit->GetTypeUnit() != CStructureData::barracks_3 )
				

			// get location
			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
			if( pBldg != NULL )
			{
				hexBldg = pBldg->GetExitHex();
			}
			LeaveCriticalSection (&cs);

	//
	// capture the hex location of first of any of these found
	// use that hex location as a starting anchor point for a
	// staging area that extends outside the current city bounds
	//
			if( hexBldg.X() || hexBldg.Y() )
				break;
		}
	}
	if( !hexBldg.X() && !hexBldg.Y() )
		return;

	// by calculating width/height already in task params
	// applying the width to location anchor, to find -/+
	// location that is fartherest from rocket location
	// which becomes anchor if it is not another building
	CHexCoord hcFrom, hcTo;
	hcFrom.X(pTask->GetTaskParam(CAI_LOC_X));
	hcFrom.Y(pTask->GetTaskParam(CAI_LOC_Y));
	hcTo.X(pTask->GetTaskParam(CAI_PREV_X));
	hcTo.Y(pTask->GetTaskParam(CAI_PREV_Y));

	m_pMap->m_pMapUtil->FindInfStagingArea( hexBldg, hcFrom, hcTo );

	// now set task params with area to stage in
	pTask->SetTaskParam(CAI_LOC_X,hcFrom.X());
	pTask->SetTaskParam(CAI_LOC_Y,hcFrom.Y());
	pTask->SetTaskParam(CAI_PREV_X,hcTo.X());
	pTask->SetTaskParam(CAI_PREV_Y,hcTo.Y());
}

void CAIGoalMgr::GetNewStagingArea( CAITask *pTask )
{
	// now consider saving base hex of the map and resetting
	// it based on where the taskforce needs to be staged
	// and getting an area from the map
	//
	int iWidth, iHeight, iRand;
	CHexCoord hcFrom(0,0), hcTo(0,0);
	
	if( !pTask->GetTaskParam(CAI_LOC_X) &&
		!pTask->GetTaskParam(CAI_LOC_Y) )
	{
		// right now just leave base as is and get a random sized
		// area (which determines max units to assign)
		iRand = pGameData->GetRandom( pGameData->m_iHexPerBlk/8 );
		iWidth = 4 + iRand;
		iRand = pGameData->GetRandom( pGameData->m_iHexPerBlk/8 );
		iHeight = 4 + iRand;
	}
	else // this is a restaging, get old size
	{
		hcFrom.X(pTask->GetTaskParam(CAI_LOC_X));
		hcFrom.Y(pTask->GetTaskParam(CAI_LOC_Y));
		hcTo.X(pTask->GetTaskParam(CAI_PREV_X));
		hcTo.Y(pTask->GetTaskParam(CAI_PREV_Y));

		m_pMap->m_pMapUtil->FlagStagingArea( FALSE,
			pTask->GetTaskParam(CAI_LOC_X),pTask->GetTaskParam(CAI_LOC_Y),
			pTask->GetTaskParam(CAI_PREV_X),pTask->GetTaskParam(CAI_PREV_Y) );

		iWidth = hcFrom.Wrap(hcTo.X()-hcFrom.X()) + 2;
		iHeight = hcFrom.Wrap(hcTo.Y()-hcFrom.Y()) + 2;
		hcFrom.X(0);
		hcFrom.Y(0);
		hcTo.X(0);
		hcTo.Y(0);
	}

	// based on the type of staging task, attempt to
	// stage near specific types of buildings, which 
	// will only happen after some buildings are built
	int iType = CTransportData::combat;
	if( pTask->GetGoalID() == IDG_PIRATE ||
		pTask->GetGoalID() == IDG_SEAINVADE )
		iType = CTransportData::ship;
	
	/*	CTransportData
		enum TRANS_BASE_TYPE {	non_combat,
						artillery,
						troops,
						ship,
						combat };
	*/
	CHexCoord hexBldg;
	WORD wCnt = 0;
	BOOL bIsStaged = FALSE;
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;
			if( pUnit->GetType() != CUnit::building )
				continue;

			// ocean based staging uses different buildings
			if( pTask->GetGoalID() == IDG_PIRATE ||
				pTask->GetGoalID() == IDG_SEAINVADE )
			{
				if( pUnit->GetTypeUnit() != CStructureData::shipyard_1 &&
					pUnit->GetTypeUnit() != CStructureData::shipyard_3 &&
					pUnit->GetTypeUnit() != CStructureData::seaport )
					continue;
			}
			else // only want buildings that produce combat vehicles
			{
				if( pUnit->GetTypeUnit() != CStructureData::barracks_2 &&
					//pUnit->GetTypeUnit() != CStructureData::barracks_3 &&
					pUnit->GetTypeUnit() != CStructureData::light_1 &&
					pUnit->GetTypeUnit() != CStructureData::light_2 &&
					pUnit->GetTypeUnit() != CStructureData::heavy )
					continue;
			}
			wCnt++;

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

			// get location
			hexBldg.X(0);
			hexBldg.Y(0);
			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
			if( pBldg != NULL )
			{
				hexBldg = pBldg->GetExitHex();
			}
			LeaveCriticalSection (&cs);
			// unit not found for some reason
			if( !hexBldg.X() && !hexBldg.Y() )
				continue;

			// skip this phase for tasks using ships
			if( iType == CTransportData::ship )
				continue;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetNewStagingArea() player %d near unit %ld at %d,%d for goal %d task %d ", 
pUnit->GetOwner(), pUnit->GetID(), hexBldg.X(), hexBldg.Y(), 
pTask->GetGoalID(), pTask->GetID() );
#endif

			bIsStaged = m_pMap->m_pMapUtil->GetStagingAreaNear( 
				hexBldg, hcFrom, hcTo, iWidth, iHeight, iType );
		}
		if( bIsStaged )
			break;
	}

	// if building were found, then use the old way 
	if( !bIsStaged && wCnt )
	{
	// there should only be 2 ocean based staging tasks
	if( pTask->GetGoalID() == IDG_PIRATE )
		m_pMap->m_pMapUtil->GetWaterStagingArea( CTransportData::cruiser,
			hcFrom, hcTo, iWidth, iHeight );
	else if( pTask->GetGoalID() == IDG_SEAINVADE )
		m_pMap->m_pMapUtil->GetWaterStagingArea( CTransportData::landing_craft,
			hcFrom, hcTo, iWidth, iHeight );

	// handles no staging area found and land only
	if( !hcFrom.X() && !hcFrom.Y() )
		wCnt = 0;
	}

	// no buildings were found to stage near, so it must be
	// the 1st staging or an early staging of the game so
	// stage near a corner of the block
	if( !bIsStaged && !wCnt )
	{
		CHexCoord hexStart,hexEnd;
		// determine the start of the block based on the start hex
		if( m_pMap->m_pMapUtil->m_RocketHex.X() > pGameData->m_iHexPerBlk )
			hexStart.X( (m_pMap->m_pMapUtil->m_RocketHex.X() 
			/ pGameData->m_iHexPerBlk) * pGameData->m_iHexPerBlk );
		else
			hexStart.X( 0 );

		if( m_pMap->m_pMapUtil->m_RocketHex.Y() > pGameData->m_iHexPerBlk )
			hexStart.Y( (m_pMap->m_pMapUtil->m_RocketHex.Y() / 
			pGameData->m_iHexPerBlk) * pGameData->m_iHexPerBlk );
		else
			hexStart.Y( 0 );

		hexEnd.X( hexEnd.Wrap( (hexStart.X() + pGameData->m_iHexPerBlk) ) );
		hexEnd.Y( hexEnd.Wrap( (hexStart.Y() + pGameData->m_iHexPerBlk) ) );

		// for each of the four corners of the block
		for( int i=0; i<NUM_PATROL_POINTS; ++i )
		{
			hexBldg = hexStart;

			switch(i)
			{
				case 0:		
					hexBldg.Xinc();
					hexBldg.Yinc();
					break;
				case 1:
					hexBldg.X( hexEnd.Wrap( (hexStart.X() + pGameData->m_iHexPerBlk) ) );
					hexBldg.Xdec();
					hexBldg.Yinc();
					break;
				case 2:
					hexBldg.X( hexEnd.Wrap( (hexStart.X() + pGameData->m_iHexPerBlk) ) );
					hexBldg.Y( hexEnd.Wrap( (hexStart.Y() + pGameData->m_iHexPerBlk) ) );
					hexBldg.Xdec();
					hexBldg.Ydec();
					break;
				case 3:
				default:
					hexBldg.Y( hexEnd.Wrap( (hexStart.Y() + pGameData->m_iHexPerBlk) ) );
					hexBldg.Xinc();
					hexBldg.Ydec();
					break;
			}
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetNewStagingArea() player %d no unit found nearby for goal %d task %d ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetNewStagingArea() using %d,%d for a reference \n", hexBldg.X(), hexBldg.Y() );
#endif
			// test hexBldg to be water if goal is sea based and skip
			if( pTask->GetGoalID() == IDG_PIRATE ||
				pTask->GetGoalID() == IDG_SEAINVADE )
			{
				CHex *pGameHex = theMap.GetHex( hexBldg );
				if( pGameHex == NULL )
					continue;

				
				if( pGameHex->GetType() != CHex::ocean &&
					pGameHex->GetType() != CHex::lake )
					continue;

			}

			bIsStaged = m_pMap->m_pMapUtil->GetStagingAreaNear( 
				hexBldg, hcFrom, hcTo, iWidth, iHeight, iType );

			if( bIsStaged )
				break;
		}
	}

	// now set task params with area to stage in
	pTask->SetTaskParam(CAI_LOC_X,hcFrom.X());
	pTask->SetTaskParam(CAI_LOC_Y,hcFrom.Y());
	pTask->SetTaskParam(CAI_PREV_X,hcTo.X());
	pTask->SetTaskParam(CAI_PREV_Y,hcTo.Y());
}

//
// determine if this vehicle type can be built by this player
//
BOOL CAIGoalMgr::CanBuildVehType( int iVehType )
{
	BOOL bCanBuild = FALSE;
	CTransportData const *pVehData = 
		pGameData->GetTransportData( iVehType );
	if( pVehData == NULL )
		return( bCanBuild );
	else
	{
		// get the opinion of the game based on this player
		//EnterCriticalSection (&cs);
		CPlayer *pPlayer = 
			pGameData->GetPlayerData( m_iPlayer );
		if( pPlayer != NULL )
			bCanBuild = pVehData->PlyrIsDiscovered(pPlayer);
		//LeaveCriticalSection (&cs);
	}
	return( bCanBuild );
}

//
// find a place to stage, considering that staging must be
// sensitive to the goal of the task:
// IDG_ADVDEFENSE,IDG_LANDWAR,IDG_PIRATE, IDG_SEAINVADE
//
void CAIGoalMgr::GetStagingArea( CAITask *pTask )
{
	// clear task params for use
	for( int i=0; i<MAX_TASKPARAMS; ++i )
		pTask->SetTaskParam(i,0);

	GetNewStagingArea( pTask );

	int iWidth, iHeight, iTotalUnits; //, iUnitStaged;
	CHexCoord hcFrom, hcTo;
	hcFrom.X(pTask->GetTaskParam(CAI_LOC_X));
	hcFrom.Y(pTask->GetTaskParam(CAI_LOC_Y));
	hcTo.X(pTask->GetTaskParam(CAI_PREV_X));
	hcTo.Y(pTask->GetTaskParam(CAI_PREV_Y));
	iWidth = hcFrom.Wrap(hcTo.X()-hcFrom.X());
	iHeight = hcFrom.Wrap(hcTo.Y()-hcFrom.Y());
	//iWidth = abs( hcFrom.Diff( hcTo.X()-hcFrom.X() ));
	//iHeight = abs( hcFrom.Diff( hcTo.Y()-hcFrom.Y() ));
	iTotalUnits = iWidth + iHeight;
	//iUnitStaged = 0;

	// no staging area could be found
	if( !hcFrom.X() && !hcFrom.Y() &&
		!hcTo.X() && !hcTo.Y() )
		return;

					
					

	// need to be sensitive to the goal
	//
	// for IDG_PIRATE versions of task
	// 4 - how many "cruiser"
	// 5 - how many "destroyer"
	if( pTask->GetGoalID() == IDG_PIRATE )
	{
		if( CanBuildVehType(CTransportData::cruiser) )
			i = m_pwaVehGoals[CTransportData::cruiser] / 2;
		else
			i = 0;

		pTask->SetTaskParam( CAI_TF_CRUISERS,pGameData->GetRandom(i) );
		iTotalUnits -= pTask->GetTaskParam(CAI_TF_CRUISERS);

		// reduced total, make sure there is some room left
		if( iTotalUnits > 0 )
		{
			if( CanBuildVehType(CTransportData::destroyer) )
				i = m_pwaVehGoals[CTransportData::destroyer] / 2;
			else
				i = 0;
			if( i > iTotalUnits )
				i = iTotalUnits;
			pTask->SetTaskParam( CAI_TF_DESTROYERS,pGameData->GetRandom(i) );
		}

		// make sure of at least one
		if( !pTask->GetTaskParam(CAI_TF_CRUISERS) &&
			!pTask->GetTaskParam(CAI_TF_DESTROYERS) )
		{
			if( CanBuildVehType(CTransportData::cruiser) )
				pTask->SetTaskParam( CAI_TF_CRUISERS,1 );
			if( CanBuildVehType(CTransportData::destroyer) )
				pTask->SetTaskParam( CAI_TF_DESTROYERS,1 );
		}
	}
	// for IDG_SEAINVADE version of task
	// CAI_TF_ARMOR   - 4 - how many "light_tank,med_tank,light_art"
	// CAI_TF_LANDING - 5 - how many "landing_craft"
	// CAI_TF_SHIPS   - 6 - how many "cruiser,destroyer,gun_boat"
	// CAI_TF_MARINES - 7 - how many "marines"
	//
	if( pTask->GetGoalID() == IDG_SEAINVADE )
	{
		i = 0;
		if( CanBuildVehType(CTransportData::rangers) )
		{
			i = m_pwaVehGoals[CTransportData::rangers];
			i += pGameData->m_iSmart;
		}
		if( i > (iTotalUnits/3) )
			i = iTotalUnits / 3;
		pTask->SetTaskParam( CAI_TF_MARINES,pGameData->GetRandom(i) );

		if( !pTask->GetTaskParam(CAI_TF_MARINES) )
		{
			if( CanBuildVehType(CTransportData::rangers) )
				pTask->SetTaskParam( CAI_TF_MARINES,1 );
		}
		iTotalUnits -= pTask->GetTaskParam(CAI_TF_MARINES);
		//iUnitStaged += pTask->GetTaskParam(CAI_TF_MARINES);

		if( iTotalUnits > 0 )
		{
			i = 0;
			if( CanBuildVehType(CTransportData::light_tank) )
				i = m_pwaVehGoals[CTransportData::light_tank];
			if( CanBuildVehType(CTransportData::med_tank) )
				i += m_pwaVehGoals[CTransportData::med_tank];
			if( CanBuildVehType(CTransportData::med_art) )
				i += m_pwaVehGoals[CTransportData::med_art];
			i /= NUM_DIFFICUTY_LEVELS;
			if(i)
				i += pGameData->m_iSmart;
			if( i > (iTotalUnits/2) )
				i = (iTotalUnits/2);

			pTask->SetTaskParam( CAI_TF_ARMOR,pGameData->GetRandom(i) );
			if( !pTask->GetTaskParam(CAI_TF_ARMOR) )
				pTask->SetTaskParam( CAI_TF_ARMOR,1 );

			// sanity check, match armor with marines
			if( pTask->GetTaskParam(CAI_TF_ARMOR) >
				pTask->GetTaskParam(CAI_TF_MARINES) )
				pTask->SetTaskParam( CAI_TF_ARMOR,
					pTask->GetTaskParam(CAI_TF_MARINES) );

			iTotalUnits -= pTask->GetTaskParam(CAI_TF_ARMOR);
			//iUnitStaged += pTask->GetTaskParam(CAI_TF_ARMOR);

			if( iTotalUnits > 0 )
			{
				i = pTask->GetTaskParam(CAI_TF_MARINES) +
					pTask->GetTaskParam(CAI_TF_ARMOR);
				if( i > iTotalUnits )
					i = iTotalUnits;

				// reduce to a minimum number of carriers + 1
				i /= MAX_CARGO;
				i++;
				// need as many landers to cary
				pTask->SetTaskParam( CAI_TF_LANDING,i );
				// apply to running total
				iTotalUnits -= pTask->GetTaskParam(CAI_TF_LANDING);
			}
		}

		if( iTotalUnits > 0 )
		{
			i = 0;
			if( CanBuildVehType(CTransportData::gun_boat) )
				i = m_pwaVehGoals[CTransportData::gun_boat];

			i /= 2;
			if( i > iTotalUnits )
				i = iTotalUnits;
			pTask->SetTaskParam( CAI_TF_SHIPS,pGameData->GetRandom(i) );
		}



	// when staging includes landing craft, then the area needs to be
	// closer to the shore 
	//	if( pTask->GetTaskParam(CAI_TF_LANDING) )
	//		FindMarineStagingArea( pTask );
	}

	// for IDG_ADVDEFENSE and IDG_LANDWAR versions of task
	// 4 - how many "light_tank,med_tank,heavy_tank" in TF
	// 5 - how many "infantry_carrier" in TF
	// 6 - how many "light_art,med_art,heavy_art" in TF
	// 7 - how many "infantry,ranger" in TF
	//
	// for now, use current veh goals for units to set 
	// tanks, inf carrier, artillery and infantry counts
	if( pTask->GetGoalID() == IDG_ADVDEFENSE ||
		pTask->GetGoalID() == IDG_LANDWAR )
	{
		i = 0;
		if( CanBuildVehType(CTransportData::light_tank) )
			i = m_pwaVehGoals[CTransportData::light_tank];
		if( CanBuildVehType(CTransportData::med_tank) )
			i += m_pwaVehGoals[CTransportData::med_tank];
		if( CanBuildVehType(CTransportData::heavy_tank) )
			i += m_pwaVehGoals[CTransportData::heavy_tank];
		if( CanBuildVehType(CTransportData::heavy_scout) )
			i += m_pwaVehGoals[CTransportData::heavy_scout];

		// no armor goals, so this is at the beginning of a game
		if(!i)
		{
			// count the number of unassigned light_tank
			// thru heavy_tank we know about
   			POSITION pos = m_plUnits->GetHeadPosition();
   			while( pos != NULL )
   			{   
    			CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
    			if( pUnit != NULL )
    			{
    				ASSERT_VALID( pUnit );

					if( pUnit->GetOwner() != m_iPlayer )
						continue;

					if( !pUnit->GetTask() &&
						(pUnit->GetTypeUnit() >= CTransportData::light_tank &&
						pUnit->GetTypeUnit() <= CTransportData::heavy_tank) )
						i++;
				}
			}
		}


		i /= 2;
		if(i)
			i += pGameData->m_iSmart;
		if( i > iTotalUnits )
			i = iTotalUnits;
		pTask->SetTaskParam( CAI_TF_TANKS,pGameData->GetRandom(i) );
		// possible default staging force
		if( i && !pTask->GetTaskParam(CAI_TF_TANKS) )
			pTask->SetTaskParam( CAI_TF_TANKS,(pGameData->m_iSmart+1) );
		iTotalUnits -= pTask->GetTaskParam(CAI_TF_TANKS);

		if( iTotalUnits <= 0 )
			return;

		i = 0;
		if( CanBuildVehType(CTransportData::light_art) )
			i = m_pwaVehGoals[CTransportData::light_art];
		if( CanBuildVehType(CTransportData::med_art) )
			i += m_pwaVehGoals[CTransportData::med_art];
		//i += m_pwaVehGoals[CTransportData::heavy_art];
		i /= NUM_DIFFICUTY_LEVELS;
		if(i)
			i += pGameData->m_iSmart;
		if( i > iTotalUnits )
			i = iTotalUnits;
		pTask->SetTaskParam( CAI_TF_ARTILLERY,pGameData->GetRandom(i) );
		// possible default staging force
		if( i && !pTask->GetTaskParam(CAI_TF_ARTILLERY) )
			pTask->SetTaskParam( CAI_TF_ARTILLERY,(pGameData->m_iSmart+1) );
	// BUGBUG force values for testing
	//	pTask->SetTaskParam( CAI_TF_ARTILLERY,0 );

		iTotalUnits -= pTask->GetTaskParam(CAI_TF_ARTILLERY);
	//	iUnitStaged += pTask->GetTaskParam(CAI_TF_ARTILLERY);

		if( iTotalUnits <= 0 )
			return;

		i = 0;
		if( CanBuildVehType(CTransportData::infantry) )
			i = m_pwaVehGoals[CTransportData::infantry];
		if( CanBuildVehType(CTransportData::rangers) )
			i += m_pwaVehGoals[CTransportData::rangers];
		i /= NUM_DIFFICUTY_LEVELS;
		if(i)
			i += pGameData->m_iSmart;
		if( i > (iTotalUnits/2) )
			i = (iTotalUnits/2);
		int j = pGameData->GetRandom(i);
		j += pGameData->m_iSmart;
		pTask->SetTaskParam( CAI_TF_INFANTRY, j );
		// possible default staging force
		if( i && !pTask->GetTaskParam(CAI_TF_INFANTRY) )
			pTask->SetTaskParam( CAI_TF_INFANTRY,(pGameData->m_iSmart+1) );

	// BUGBUG force values for infantry
		pTask->SetTaskParam( CAI_TF_INFANTRY,0 );
	//
		if( CanBuildVehType(CTransportData::infantry_carrier) )
		{
			// only as many IFVs as necessary to carry infantry
			i = pTask->GetTaskParam(CAI_TF_INFANTRY) / MAX_CARGO;
			i++;
			pTask->SetTaskParam( CAI_TF_IFVS, i );
			// BUGBUG force no IFVs in assaults
			pTask->SetTaskParam( CAI_TF_IFVS, 0 );
		}

		// default staged land force
		// be gentle for the easy level, but gotta have something
		if( !pGameData->m_iSmart ) 
		{
			if( !pTask->GetTaskParam(CAI_TF_INFANTRY) )
			{
				pTask->SetTaskParam( CAI_TF_INFANTRY,
					m_pwaVehGoals[CTransportData::infantry] / 2 );
			}
		}

		// force armor heavy
		if( iTotalUnits )
		{
			i = pTask->GetTaskParam(CAI_TF_TANKS) + (iTotalUnits - 1);
			pTask->SetTaskParam( CAI_TF_TANKS,i );
		}

		// BUGBUG turn off sanity check so we end up armor heavy
		/*
		// need to have a sanity check, so we don't end up armor heavy
		i = pTask->GetTaskParam(CAI_TF_INFANTRY);
		i += pTask->GetTaskParam(CAI_TF_ARTILLERY);
		i += pTask->GetTaskParam(CAI_TF_IFVS);
		if( i < pTask->GetTaskParam(CAI_TF_TANKS) )
			pTask->SetTaskParam(CAI_TF_TANKS,i);

		i = pTask->GetTaskParam(CAI_TF_INFANTRY);
		i += pTask->GetTaskParam(CAI_TF_ARTILLERY);
		i += pTask->GetTaskParam(CAI_TF_IFVS);
		i += pTask->GetTaskParam(CAI_TF_TANKS);

		// must have some thing
		if( !i )
		{
			pTask->SetTaskParam(CAI_TF_INFANTRY,
				(pGameData->m_iSmart + 1) );
			pTask->SetTaskParam(CAI_TF_TANKS,
				(pGameData->m_iSmart + 1) );
			if( CanBuildVehType(CTransportData::infantry_carrier) )
			{
				pTask->SetTaskParam(CAI_TF_IFVS,
					(pGameData->m_iSmart + 1) );
			}
		}
		*/

	}
}

// get highest rated target, belonging to this opfor
// that this type of vehicle can attack
// that has the fewest number of units targeting it
//
DWORD CAIGoalMgr::GetOpforTarget( int iOpForID, int iVehType )
{
	if( iOpForID == m_iPlayer )
		return(0);

	// handle water only vehicles differently
	CTransportData const *pVehData = 
		pGameData->GetTransportData( iVehType );
	if( pVehData == NULL )
		return(0);

	int iTargetType = 0;
	if( pVehData->GetTargetType() == CUnitData::naval )
		iTargetType = CUnitData::naval;

	// CAI_AMMO param is the count of units that have
	// targeted an opfor, but not necessarily attacked
	// CAI_FIRERATE param is the rating of that unit
	// as a target

	int iBestRating = 0, iRating;
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
    	CAIUnit *pOpforUnit = (CAIUnit *)m_plUnits->GetNext( pos );
    	if( pOpforUnit != NULL )
    	{
    		ASSERT_VALID( pOpforUnit );

			if( pOpforUnit->GetOwner() != iOpForID )
				continue;

			if( pOpforUnit->GetOwner() == m_iPlayer )
				continue;

			// already under attack
			if( pOpforUnit->AttackCount() )
				continue;

			pVehData = 
				pGameData->GetTransportData( pOpforUnit->GetTypeUnit() );
			if( pVehData == NULL )
				continue;

			// deal with navy vs. navy and land vs. land targeting
			if( iTargetType == CUnitData::naval )
			{
				// skip if not naval type
				if( pVehData->GetTargetType() != CUnitData::naval )
					continue;
			}
			else
			{
				// skip if a navy type
				if( pVehData->GetTargetType() == CUnitData::naval )
					continue;
			}

			BOOL bIsAbandoned = FALSE;
			if( !pOpforUnit->GetParam(CAI_FIRERATE) )
			{
				int i = 0;
				if( pOpforUnit->GetType() == CUnit::building )
				{
				iRating = 0;
				EnterCriticalSection (&cs);
				CBuilding *pBuilding =
					theBuildingMap.GetBldg( pOpforUnit->GetID() );
				if( pBuilding != NULL )
				{
					iRating = m_pMap->m_pMapUtil->AssessTarget(pBuilding,0);
					bIsAbandoned = pBuilding->IsFlag( CUnit::abandoned );

					// abandoned buildings only get 1/2 rating
					if( bIsAbandoned && iRating )
						iRating >>= 1;
				}
				LeaveCriticalSection (&cs);
		
				// use target type to start offset to factor
				i = NUM_COMBINED_UNITS * pOpforUnit->GetType();
				}
				else if( pOpforUnit->GetType() == CUnit::vehicle )
				{
				iRating = 0;
				EnterCriticalSection (&cs);
				CVehicle *pVehicle =
					theVehicleMap.GetVehicle( pOpforUnit->GetID() );
				if( pVehicle != NULL )
					iRating = m_pMap->m_pMapUtil->AssessTarget(pVehicle,0);
				LeaveCriticalSection (&cs);
				// use target type to start offset to factor
				i = NUM_COMBINED_UNITS * 
					(CStructureData::num_types + pOpforUnit->GetType());
				}
				else
					continue;

				// apply attacker type to offset to factor
				i += iVehType;
				// get factor and apply to rating
				iRating += (caTargetAttack[i] * iRating);

				if( iRating < 0 )
					iRating = 0;

				// store target rating locally to avoid having to get it again
				pOpforUnit->SetParam( CAI_FIRERATE, iRating );
			}
			else
				iRating = pOpforUnit->GetParam(CAI_FIRERATE);

			if( iRating > iBestRating )
				iBestRating = iRating;

		}
	}

#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
	//if( myYieldThread() == TM_QUIT )
	//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	iRating = iBestRating / 2; // provide some tolerance
	iBestRating = m_plUnits->GetCount(); // and make threshhold high

	// now select the target based on those with a rating as high
	// or higher than iRating, with the lowest count of units that
	// already have targeted them
	CAIUnit *pTarget = NULL;
   	pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
    	CAIUnit *pOpforUnit = (CAIUnit *)m_plUnits->GetNext( pos );
    	if( pOpforUnit != NULL )
    	{
    		ASSERT_VALID( pOpforUnit );

			if( pOpforUnit->GetOwner() != iOpForID )
				continue;

			// already under attack
			if( pOpforUnit->AttackCount() )
				continue;

			if( pOpforUnit->GetParam(CAI_FIRERATE) >= iRating )
			{
				// check the number of units targeting this opfor unit
				if( pOpforUnit->GetParam(CAI_AMMO) < iBestRating )
				{
					iBestRating = pOpforUnit->GetParam(CAI_AMMO);
					pTarget = pOpforUnit;
				}
			}
		}
	}

	if( pTarget != NULL )
	{
		iRating = pTarget->GetParam(CAI_AMMO) + 1;
		pTarget->SetParam(CAI_AMMO,iRating);
		return( pTarget->GetID() );
	}
	return(0);
}

//
// find the opfor unit that qualifies based on the params passed
// and with which we are at war or hostile towards
//
// iHow = THREAT_TARGET, BEST_TARGET, NEAREST_TARGET
// iKindOf = CAI_NAVALATTACK (navy), CAI_SOFTATTACK (vehicle), 
//           CAI_HARDATTACK (building), zero=any
// pUnit = unit that may attack
// 
DWORD CAIGoalMgr::GetOpForUnit( int iHow, int iKindOf, CAIUnit *pUnit )
{
	if( pUnit->GetOwner() != m_iPlayer )
		return(0);	

	// first consider only those opfor units in range
	// with war or hostile relations
	int iArea = 0, iRange = 0;
	int iSpotting = 0;
	int iThreat = 0;
	int iBest = 0;
	CHexCoord hexUnit;
	int iAttackTypes[CUnitData::num_attacks];

	// unit seeking is a vehicle
	if( pUnit->GetType() == CUnit::vehicle )
	{
		// get location
		EnterCriticalSection (&cs);
		CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
		if( pVehicle != NULL )
		{
			hexUnit = pVehicle->GetHexHead();

			// range is in subhexes? convert to CHexCoords
			iArea = pVehicle->GetRange() * 2;
			iSpotting = pVehicle->GetSpottingRange();

			iAttackTypes[CUnitData::soft] =
				pVehicle->GetAttack(CUnitData::soft);
			iAttackTypes[CUnitData::hard] =
				pVehicle->GetAttack(CUnitData::hard);
			iAttackTypes[CUnitData::naval] =
				pVehicle->GetAttack(CUnitData::naval);
		}
		else
			iArea = 0;
		LeaveCriticalSection (&cs);
	}
	// unit seeking is a building
	//
	else if( pUnit->GetType() == CUnit::building )
	{
		// get location
		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
		if( pBldg != NULL )
		{
			//hexUnit = pBldg->GetHex();
			hexUnit = pBldg->GetExitHex();
			// range is in CHexCoords per DT 1/23/96 phone call
			iArea = pBldg->GetRange();
			iSpotting = pBldg->GetSpottingRange();

			iAttackTypes[CUnitData::soft] =
				pBldg->GetAttack(CUnitData::soft);
			iAttackTypes[CUnitData::hard] =
				pBldg->GetAttack(CUnitData::hard);
			iAttackTypes[CUnitData::naval] =
				pBldg->GetAttack(CUnitData::naval);
		}
		else
			iArea = 0;
		LeaveCriticalSection (&cs);
	}
	else
		return(0);

	// set a factor that controls how far out the expansion of
	// the area will go to find a target.  this gets /=2 later
	// which will eventually expand to block size
	int iFactor = 4;
	// NOTE - can return to here if no target found
	//        with changes in iArea to increase search space
	TryTryAgain:

	DWORD dwOpForUnit, dwSeekTo = 0;
	int iBestUnit = 0;
	int iClosest = 0xFFFE;
	int iOpForID;
	CHexCoord hcStart, hcEnd, hcAt;
	// use iRange +/- hexUnit to form boundaries of an area 
	// around hexUnit to consider for targets in range
	hcStart.X( hcAt.Wrap(hexUnit.X() - iArea) );
	hcStart.Y( hcAt.Wrap(hexUnit.Y() - iArea) );
	hcEnd.X( hcAt.Wrap(hexUnit.X() + iArea) );
	hcEnd.Y( hcAt.Wrap(hexUnit.Y() + iArea) );

	CHex *pGameHex;
	CSubHex sub( hcAt );
	CStructureData const *pSd = NULL;
	CTransportData const *pTd = NULL;

	// BUGBUG - any changes to CHexCoord implementation will break this!
	int iDeltaX = hcStart.Wrap(hcEnd.X() - hcStart.X());
	int iDeltaY = hcStart.Wrap(hcEnd.Y() - hcStart.Y());
	int iX,iY,i;

	// the hexes checked
	for( iY=0; iY<iDeltaY; ++iY )
	{
		hcAt.Y( hcAt.Wrap(hcStart.Y()+iY) );

		for( iX=0; iX<iDeltaX; iX++ )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			hcAt.X( hcAt.Wrap(hcStart.X() + iX) );

			pGameHex = theMap.GetHex( hcAt );
			if( pGameHex == NULL )
				continue;

			BYTE bUnits = pGameHex->GetUnits();
			if( !bUnits )
				continue;
			
			iOpForID = 0;
			iThreat = 0;
			iBest = 0;
			dwOpForUnit = 0;
			int iVehPlayer, iType;
			
			// a building is not here, and a vehicle is
			if( !(bUnits & CHex::bldg) &&
			    (bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr)) )
			{
				// looking for vehicles (land and navy) or any unit
				if( !iKindOf || 
					(iKindOf == CAI_SOFTATTACK) ||
					(iKindOf == CAI_NAVALATTACK) )
				{
					CVehicle *pVehicle = NULL;
					
					BOOL bIsCargo = FALSE;
					// this will find only the first vehicle in the hex
					for( int iy=0; iy<2; ++iy )
					{
						sub.y = (hcAt.Y() * 2) + iy;

						for( int ix=0; ix<2; ++ix )
						{
							sub.x = (hcAt.X() * 2) + ix;

							EnterCriticalSection (&cs);
							pVehicle = 
								theVehicleHex.GetVehicle( sub.x, sub.y );
							if( pVehicle != NULL )
							{
								iVehPlayer = 
									pVehicle->GetOwner()->GetPlyrNum();
								iOpForID = 
									pVehicle->GetOwner()->GetPlyrNum();
								dwOpForUnit = pVehicle->GetID();
								if( pVehicle->GetTransport() != NULL )
									bIsCargo = TRUE;
							}
							LeaveCriticalSection (&cs);

							if( iVehPlayer == m_iPlayer )
							{
								iOpForID = 0;
								dwOpForUnit = 0;
								pVehicle = NULL;
								continue;
							}
							else
								break;
						}
						if( pVehicle != NULL )
						{
							break;
						}
						else
						{
							dwOpForUnit = 0;
							iOpForID = 0;
						}
					}

					if( pVehicle != NULL )
					{
						// don't attack our own units
						if( iOpForID == m_iPlayer )
						{
							dwOpForUnit = 0;
							continue;
						}
						// skip vehicles that are cargo
						if( bIsCargo )
						{
							dwOpForUnit = 0;
							continue;
						}

						// skip vehicles we can't reach, which will have
						// the effect of ships not targeting buildings
						// and land combat vehicles not targeting ships
						// but will keep from targeting units that can't
						// be reached
						if( pUnit->GetType() == CUnit::vehicle )
						{
							pGameData->GetVehicleHex( pUnit->GetID(), hexUnit );
							if( !hexUnit.X() && !hexUnit.Y() )
							{
								dwOpForUnit = 0;
								continue;
							}

							if( !m_pMap->m_pMapUtil->GetPathRating(
								hexUnit, hcAt, pUnit->GetTypeUnit()) )
							{
								dwOpForUnit = 0;
								continue;
							}
						}

						if( iHow == THREAT_TARGET )
						{
							EnterCriticalSection (&cs);
							pVehicle = 
								theVehicleMap.GetVehicle( dwOpForUnit );
							if( pVehicle != NULL )
							{
								iThreat = 
								AssessThreat( pVehicle, iKindOf);
							}
							LeaveCriticalSection (&cs);
						}
						else if( iHow == BEST_TARGET )
						{
							EnterCriticalSection (&cs);
							pVehicle = 
								theVehicleMap.GetVehicle( dwOpForUnit );
							if( pVehicle != NULL )
							{
								iBest = 
								m_pMap->m_pMapUtil->AssessTarget(pVehicle,iKindOf);

								pTd = pVehicle->GetData();
						
								// determine offset to array of 
								// target/attacker factors and apply
								i = NUM_COMBINED_UNITS * (
									CStructureData::num_types + pTd->GetType());

								// apply attacker type to offset to factor
								i += pUnit->GetTypeUnit();
								// get factor and apply to rating
								iBest += (caTargetAttack[i] * iBest);
							}
							LeaveCriticalSection (&cs);
						}
					}
				}
			} // end of testing for vehicle

			// NOTE: this is order sensitive to iOpForID, mean don't move it

			// a building is here
			if( bUnits & CHex::bldg )
			{
				BOOL bIsAbandoned = FALSE;

				// looking for buildings or any unit
				if( !iOpForID && 
					(!iKindOf || 
					(iKindOf == CAI_HARDATTACK)) )
				{	
					EnterCriticalSection (&cs);
					CBuilding *pBuilding =
						theBuildingHex.GetBuilding( hcAt );
					if( pBuilding != NULL )
					{
						iOpForID = 
							pBuilding->GetOwner()->GetPlyrNum();
						iType =
							pBuilding->GetData()->GetType();
						dwOpForUnit =
							pBuilding->GetID();
						bIsAbandoned = 
							pBuilding->IsFlag( CUnit::abandoned );
					}
					LeaveCriticalSection (&cs);

					// don't attack our own units
					if( iOpForID == m_iPlayer || 
						bIsAbandoned )
					{
						dwOpForUnit = 0;
						continue;
					}

					// consider no CStructureData::city through
					// CStructureData::office_3_2 type buildings
					if( iType == CStructureData::city ) //&&
						//iType <= CStructureData::office_3_2 )
					{
						dwOpForUnit = 0;
						continue;
					}

					// skip buildings that can't be reached
					if( pUnit->GetType() == CUnit::vehicle )
					{
						pGameData->GetVehicleHex( pUnit->GetID(), hexUnit );
						if( !hexUnit.X() && !hexUnit.Y() )
						{
							dwOpForUnit = 0;
							continue;
						}

						if( !m_pMap->m_pMapUtil->GetPathRating(
							hexUnit, hcAt, pUnit->GetTypeUnit()) )
						{
							dwOpForUnit = 0;
							continue;
						}
					}

					if( iHow == THREAT_TARGET )
					{
						EnterCriticalSection (&cs);
						pBuilding = 
							theBuildingMap.GetBldg( dwOpForUnit );
						if( pBuilding != NULL )
						{
							iThreat = 
							AssessThreat(pBuilding,iKindOf);
							bIsAbandoned = 
								pBuilding->IsFlag( CUnit::abandoned );

							if( iThreat && bIsAbandoned )
								iThreat >>= 1;
						}
						LeaveCriticalSection (&cs);
					}
					else if( iHow == BEST_TARGET )
					{
						EnterCriticalSection (&cs);
						pBuilding = 
							theBuildingMap.GetBldg( dwOpForUnit );
						if( pBuilding != NULL )
						{
							iBest = 
							m_pMap->m_pMapUtil->AssessTarget(pBuilding,iKindOf);

							pSd = pBuilding->GetData();

							// determine offset to array of 
							// target/attacker factors and apply
							i = NUM_COMBINED_UNITS * pSd->GetType();
							// apply attacker type to offset to factor
							i += pUnit->GetTypeUnit();
							// get factor and apply to rating
							iBest += (caTargetAttack[i] * iBest);

							bIsAbandoned = 
								pBuilding->IsFlag( CUnit::abandoned );
							if( iBest && bIsAbandoned )
								iBest >>= 1;
						}
						LeaveCriticalSection (&cs);
					}
				}
			} // end of testing for a building


			// nothing here
			if( !iOpForID )
				continue;
			if( iOpForID == m_iPlayer )
				continue;

			// are we hostile or at war with this opfor?
			CAIOpFor *pOpFor = m_plOpFors->GetOpFor(iOpForID);
			if( pOpFor == NULL )
				continue;

			if( iBest < 0 )
				iBest = 0;

			// possible automatic alliance with other AI players!
			if( pOpFor->IsAI() && !pOpFor->AtWar() )
				continue;

			// a unit was selected
			if( dwOpForUnit )
			{
				// then make sure it is a known unit by this player
				if( m_plUnits->GetOpForUnit(dwOpForUnit) == NULL )
					continue;
			}

			// closer to the attacking unit
			if( iHow == NEAREST_TARGET )
			{
				iRange = pGameData->GetRangeDistance( hexUnit, hcAt );
				if( iRange && iRange < iClosest )
				{
					iClosest = iRange;
					dwSeekTo = dwOpForUnit;
				}
			}
			else if( iHow == THREAT_TARGET ) // more threatening target
			{
				if( iThreat > iBestUnit )
				{
					iBestUnit = iThreat;
					dwSeekTo = dwOpForUnit;
				}
			}
			else if( iHow == BEST_TARGET ) // higher value target
			{
				if( iBest > 0  &&  iBest > iBestUnit )
				{
					iBestUnit = iBest;
					dwSeekTo = dwOpForUnit;
				}
			}
		}
	}

#ifdef _LOGOUT
	if( dwSeekTo )
	{
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIGoalMgr::GetOpForUnit() player %d unit %ld selected target %ld on first try", 
	pUnit->GetOwner(), pUnit->GetID(), dwSeekTo);
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIGoalMgr::GetOpForUnit() iHow=%d  [BEST=310 NEAREST=311 THREAT=312 WEAKEST=313]", 
	iHow );
	}
#endif

	// no target found so expand to spotting distance and try again,
	// then expand to 2xblocksize, and finally fail if still no target
	if( !dwSeekTo )
	{
		if( iArea < iSpotting )
			iArea = iSpotting;
		else if( iArea < pGameData->m_iHexPerBlk &&
			iHow == NEAREST_TARGET )
		{
			// a factor that controls how far out the expansion of
			// the area will go to find a target.
			// which will eventually expand to block size
			if( iFactor )
				iArea = pGameData->m_iHexPerBlk / iFactor;
			// expand the area to search by half
			iFactor /= 2;
		}
		else // unable to spot a target
		{
			// always get a target?!? Yuck!!!!
			//
			// if all else has failed, then go thru the units list
			// and find the closest target which the attacking unit
			// can attack and select it regardless of type
			//
			if( !dwSeekTo &&
				iHow == NEAREST_TARGET )
			{
				if( pUnit->GetType() == CUnit::vehicle )
				{
					pGameData->GetVehicleHex( pUnit->GetID(), hexUnit );
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetOpForUnit() player %d unit %ld find nearest target to %d,%d", 
pUnit->GetOwner(), pUnit->GetID(), hexUnit.X(), hexUnit.Y() );
#endif
				}

				iClosest = 0xFFFE;

				// find the nearest enemy target of any player to hexUnit
				// that can be attacked
   				POSITION pos = m_plUnits->GetHeadPosition();
   				while( pos != NULL )
   				{   
       				CAIUnit *pTargetUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       				if( pTargetUnit != NULL )
       				{
       					ASSERT_VALID( pTargetUnit );

						// only want non-player units
						if( pTargetUnit->GetOwner() == m_iPlayer )
							continue;

						CAIOpFor *pOpFor = 
							m_plOpFors->GetOpFor(pTargetUnit->GetOwner());
						if( pOpFor == NULL )
							continue;
						// possible alliance with other AI players!
						if( pOpFor->IsAI() && !pOpFor->AtWar() )
							continue;

						hcAt.X(0);
						hcAt.Y(0);

						// determine if this is a building
						if( pTargetUnit->GetType() == CUnit::building )
						{
							// skip unimportant buildings
							if( pTargetUnit->GetTypeUnit() != 
								CStructureData::rocket &&
								pTargetUnit->GetTypeUnit() < 
								CStructureData::barracks_2 )
								continue;

							EnterCriticalSection (&cs);
							CBuilding *pBldg = 
							theBuildingMap.GetBldg( pTargetUnit->GetID() );
							if( pBldg != NULL )
							{
								hcAt = pBldg->GetExitHex();
								iThreat = pBldg->GetData()->GetTargetType();
							}
							LeaveCriticalSection (&cs);
						}
						else if( pTargetUnit->GetType() == CUnit::vehicle )
						{
							EnterCriticalSection (&cs);
							CVehicle *pVehicle = 
								theVehicleMap.GetVehicle( pTargetUnit->GetID() );
							if( pVehicle != NULL )
							{
								// skip vehicles that are cargo
								if( pVehicle->GetTransport() == NULL )
								{
								hcAt = pVehicle->GetHexHead();
								iThreat = pVehicle->GetData()->GetTargetType();
								}
							}
							LeaveCriticalSection (&cs);
						}

						// protect against unit not found
						if( !hcAt.X() && !hcAt.Y() )
							continue;
						// attacker cannot attack this type of target
						if( !iAttackTypes[iThreat] )
							continue;

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

						// skip targets that can't be reached
						if( pUnit->GetType() == CUnit::vehicle && 
							!m_pMap->m_pMapUtil->GetPathRating(
							hexUnit, hcAt, pUnit->GetTypeUnit()) )
							continue;

						// find the closest target
						iRange = pGameData->GetRangeDistance( hexUnit, hcAt );
						if( iRange && iRange < iClosest )
						{
							iClosest = iRange;
							dwSeekTo = pTargetUnit->GetID();
						}
					}
				}

#ifdef _LOGOUT
	if( dwSeekTo )
	{
		logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIGoalMgr::GetOpForUnit() player %d unit %ld selected nearest target %ld", 
		pUnit->GetOwner(), pUnit->GetID(), dwSeekTo);
	}
#endif
			}
			return(dwSeekTo); // failed to find a target
		}
		goto TryTryAgain;
	}
	return(dwSeekTo);
}

//
// rate the threat posed by the passed unit type, considering
// the kind of target it is expected to be
//
int CAIGoalMgr::AssessThreat(CVehicle *pData, int iKindOf)
{
	int iAssessment = 0;
	switch( iKindOf )
	{
		case 0:					// (any) type of unit comparison
			iAssessment += pData->GetAttack(CUnitData::soft);
			iAssessment += pData->GetAttack(CUnitData::hard);
			iAssessment += pData->GetAttack(CUnitData::naval);
			// GetAccuracy() and GetFireRate() use low values for best
			if( pData->GetAccuracy() )
				iAssessment += (iAssessment / pData->GetAccuracy());
			if( pData->GetFireRate() )
				iAssessment += (iAssessment / pData->GetFireRate());
			break;
		case CAI_NAVALATTACK:	// (navy) only
			iAssessment += pData->GetAttack(CUnitData::naval);
			// GetAccuracy() and GetFireRate() use low values for best
			if( pData->GetAccuracy() )
				iAssessment += (iAssessment / pData->GetAccuracy());
			if( pData->GetFireRate() )
				iAssessment += (iAssessment / pData->GetFireRate());
			break;
		case CAI_SOFTATTACK: 	// (vehicle) a land unit
		case CAI_HARDATTACK:	// (building)
			iAssessment += pData->GetAttack(CUnitData::soft);
			iAssessment += pData->GetAttack(CUnitData::hard);
			// GetAccuracy() and GetFireRate() use low values for best
			if( pData->GetAccuracy() )
				iAssessment += (iAssessment / pData->GetAccuracy());
			if( pData->GetFireRate() )
				iAssessment += (iAssessment / pData->GetFireRate());
			break;
		default:
			break;
	}
	return( iAssessment );
}

//
// rate the threat posed by the passed unit type, considering
// the kind of target it is expected to be
//
int CAIGoalMgr::AssessThreat(CBuilding *pData, int iKindOf)
{
	int iAssessment = 0;
	switch( iKindOf )
	{
		case 0:					// (any) type of unit comparison
			iAssessment += pData->GetAttack(CUnitData::soft);
			iAssessment += pData->GetAttack(CUnitData::hard);
			iAssessment += pData->GetAttack(CUnitData::naval);
			// GetAccuracy() and GetFireRate() use low values for best
			if( pData->GetAccuracy() )
				iAssessment += (iAssessment / pData->GetAccuracy());
			if( pData->GetFireRate() )
				iAssessment += (iAssessment / pData->GetFireRate());
			break;
		case CAI_NAVALATTACK:	// (navy) only
			iAssessment += pData->GetAttack(CUnitData::naval);
			// GetAccuracy() and GetFireRate() use low values for best
			if( pData->GetAccuracy() )
				iAssessment += (iAssessment / pData->GetAccuracy());
			if( pData->GetFireRate() )
				iAssessment += (iAssessment / pData->GetFireRate());
			break;
		case CAI_SOFTATTACK: 	// (vehicle) a land unit
		case CAI_HARDATTACK:	// (building)
			iAssessment += pData->GetAttack(CUnitData::soft);
			iAssessment += pData->GetAttack(CUnitData::hard);
			// GetAccuracy() and GetFireRate() use low values for best
			if( pData->GetAccuracy() )
				iAssessment += (iAssessment / pData->GetAccuracy());
			if( pData->GetFireRate() )
				iAssessment += (iAssessment / pData->GetFireRate());
			break;
		default:
			break;
	}
	return( iAssessment );
}

//
// this sucks!
//
// this a function dave wants, so that when the HP has "no chance"
// that the AI player will gang up and wipe out the remaining HP
// buildings and vehicles.  Instead of just ending the game, which
// is the best solution to this problem, thielen wants this crap.
//
// so be it
//
void CAIGoalMgr::KillOpfor( int iOpFor )
{
	// find the task to do it with for land units
	CAITask *pLandTask = NULL;
	pLandTask = m_plTasks->GetTask( IDT_SEEKINWAR, IDG_LANDWAR );
	if( pLandTask == NULL )
		pLandTask = m_plTasks->GetTask( IDT_SEEKINRANGE, IDG_ESTABLISH );

	// and sea units too
	CAITask *pSeaTask = NULL;
	pSeaTask = m_plTasks->GetTask( IDT_SEEKATSEA, IDG_SEAWAR );
	if( pSeaTask == NULL )
		pSeaTask = m_plTasks->GetTask( IDT_SEEKATSEA, IDG_SHORES );

	// 
	CHexCoord hexTarget, hexVeh;
	int iSpotting;
	CAIUnit *pTarget = NULL;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::KillOpfor() player %d to kill player %d", m_iPlayer, iOpFor );
#endif

	// now for every vehicle doing a patrol task, switch to the appropriate
	// seek task and select a target?
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetType() == CUnit::vehicle )
			{
				// looking for only units with certain tasks tasks
				if( pUnit->GetTask() != IDT_PATROL &&
					pUnit->GetTask() != IDT_SCOUT &&
					pUnit->GetTask() != IDT_SEEKINWAR &&
					pUnit->GetTask() != IDT_SEEKINRANGE &&
					pUnit->GetTask() != IDT_SEEKATSEA )
					continue;

				BOOL bFoundIt = FALSE;
				EnterCriticalSection (&cs);
				CVehicle *pVehicle = 
					theVehicleMap.GetVehicle( pUnit->GetID() );
				if( pVehicle != NULL )
				{
					hexVeh = pVehicle->GetHexHead();
					iSpotting = pVehicle->GetSpottingRange();
					bFoundIt = TRUE;
				}
				LeaveCriticalSection (&cs);
				if( !bFoundIt )
					continue;

				// find closest opfor building to this unit
				hexTarget = hexVeh;
				pGameData->FindBuilding( 0, iOpFor, hexTarget );
				// can unit get there?
				if( !m_pMap->m_pMapUtil->GetPathRating(
					hexVeh, hexTarget, pUnit->GetTypeUnit()) )
					continue;

				// set to appropriate task based on land combat units
				if( pGameData->IsCombatVehicle(pUnit->GetTypeUnit()) )
				{
					if( pUnit->GetTask() == IDT_PATROL ||
						pUnit->GetTask() == IDT_SCOUT )
					{
						// set pLandTask if not NULL and
						// get staging hex near hexTarget
						if( pLandTask == NULL )
							continue;

						pUnit->SetTask( pLandTask->GetID() );
						pUnit->SetGoal( pLandTask->GetGoalID() );
					}
					if( pUnit->GetTask() == IDT_SEEKINWAR ||
						pUnit->GetTask() == IDT_SEEKINRANGE )
					{
						// if current target is of the above opfor
						// then leave this unit alone, else clear
						// target and get staging hex near hexTarget
						pTarget = m_plUnits->GetOpForUnit( pUnit->GetDataDW() );
						if( pTarget != NULL )
						{
							if( pTarget->GetOwner() == iOpFor )
								continue;
						}
					}
				}
				else // or sea combat units
				{
					if( pUnit->GetTask() == IDT_PATROL )
					{
						// set pSeaTask if not NULL and
						// get staging hex near hexTarget
						if( pSeaTask == NULL )
							continue;

						pUnit->SetTask( pSeaTask->GetID() );
						pUnit->SetGoal( pSeaTask->GetGoalID() );
					}
					if( pUnit->GetTask() == IDT_SEEKATSEA )
					{
						// if current target is of the above opfor
						// then leave this unit alone, else clear
						// target and get staging hex near hexTarget
						pTarget = m_plUnits->GetOpForUnit( pUnit->GetDataDW() );
						if( pTarget != NULL )
						{
							if( pTarget->GetOwner() == iOpFor )
								continue;
						}
					}
				}
				//pUnit->SetStatus(0);
				//pUnit->ClearParam();

				// clearing target
				pUnit->SetDataDW(0);
				pUnit->SetParam(CAI_TARGETTYPE,0xFFFE );

				// get staging hex and send unit there
				m_pMap->m_pMapUtil->FindStagingHex( 
					hexTarget, iSpotting, iSpotting,
					pUnit->GetTypeUnit(), hexVeh, FALSE );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::KillOpfor() player/vehicle %d/%ld a %d unit is now on mop up", 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetTypeUnit() );
#endif
				pUnit->SetDestination( hexVeh );
			}
		}
	}
}

#if 0

//
// this considers that unit just attacked needs
// support, so each combat unit of the manager, that is
// either IDT_PATROL, IDT_PREPAREWAR, should switch to a
// IDT_SEEKINRANGE and help attacked unit.  How?
//
// get location of attacking and attacked units
// form search space by extending the extremes
// of their locations
//
// go through the units of this mgr and look for
// units both in the area and that have the above tasks
//
// for each unit found, set task to IDT_SEEKINRANGE and
// select cooridinated targeting upon the attacking unit
// and any other opfor units in the search space and send
// destination message to cause approach
//
void CAIGoalMgr::GetSupport( CAIUnit *pTarget, CAIUnit *pAttacker, 
	CAIMsg * /*pMsg*/ )
{
	// protect from attacking own units
	if( pTarget->GetOwner() != m_iPlayer )
		return;
	if( pAttacker->GetOwner() == m_iPlayer )
		return;
	// we have enough support
	if( pAttacker->AttackCount() >= CAI_RANGE )
		return;

	CHexCoord hcTarget(0,0), hcAttacker(0,0);
	BOOL bWaterOnly;

	// get target location
	if( pTarget->GetType() == CUnit::vehicle )
	{
		EnterCriticalSection (&cs);
		CVehicle *pVehicle = theVehicleMap.GetVehicle( pTarget->GetID() );
		if( pVehicle != NULL )
		{
			hcTarget = pVehicle->GetHexHead();
			if( pVehicle->GetData()->GetBaseType() == 
				CTransportData::ship )
				bWaterOnly = TRUE;
		}
		LeaveCriticalSection (&cs);
	}
	else if( pTarget->GetType() == CUnit::building )
	{
		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( pTarget->GetID() );
		if( pBldg != NULL )
			hcTarget = pBldg->GetExitHex();
			//hcTarget = pBldg->GetHex();
			
		LeaveCriticalSection (&cs);
	}
	else
		return;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIGoalMgr::GetSupport() player %d for %ld", 
pTarget->GetOwner(), pTarget->GetID() );
#endif

	// int CGameMap::LineOfSight (CUnit const * pSrc, CUnit const * pDest) const
	// get attacker location
	if( pAttacker->GetType() == CUnit::vehicle )
	{
		EnterCriticalSection (&cs);
		CVehicle *pVehicle = theVehicleMap.GetVehicle( pAttacker->GetID() );
		if( pVehicle != NULL )
		{
			hcAttacker = pVehicle->GetHexHead();
			if( pVehicle->GetData()->GetBaseType() == 
				CTransportData::ship )
				bWaterOnly = TRUE;
		}
		LeaveCriticalSection (&cs);
	}
	else if( pAttacker->GetType() == CUnit::building )
	{
		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( pAttacker->GetID() );
		if( pBldg != NULL )
			hcAttacker = pBldg->GetExitHex();
			//hcAttacker = pBldg->GetHex();
		LeaveCriticalSection (&cs);
	}
	else
		return;

	// the attacker is already dead
	if( !hcAttacker.X() && !hcAttacker.Y() )
	{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetSupport() support unneeded as attacker is dead" ); 
#endif
		return;
	}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetSupport() attacked by unit %ld of player %d from %d,%d ", 
pAttacker->GetID(), pAttacker->GetOwner(), hcAttacker.X(), hcAttacker.Y() );
#endif


	// make sure the appropriate goal/task is available
	CAIGoal *pGoal = NULL;
	if( bWaterOnly )
	{
		// IDG_SEAWAR - IDT_ESCORT for supporting navy units
		pGoal = m_plGoalList->GetGoal(IDG_SEAWAR);
		if( pGoal == NULL )
		{
			AddGoal(IDG_SEAWAR);
			m_bGoalChange = TRUE;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetSupport() unable to provide sea support at this time" );
#endif
			return;
		}
	}
	else
	{
		// IDG_ADVDEFENSE - IDT_ESCORT for support land units
		pGoal = m_plGoalList->GetGoal(IDG_ADVDEFENSE);
		if( pGoal == NULL )
		{
			AddGoal(IDG_ADVDEFENSE);
			m_bGoalChange = TRUE;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetSupport() unable to provide land support at this time" );
#endif
			return;
		}
	}
	// find the task for support
	CAITask *pTask = NULL;
	pTask = m_plTasks->GetTask( IDT_ESCORT, pGoal->GetID() );	
	if( pTask == NULL )
		return;
	if( pTask->GetStatus() != UNASSIGNED_TASK )
	{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetSupport() support assemblying at this time" );
#endif
			return;
	}

	int iRange;
	CHexCoord hex;
	// count the types of vehicles that can provide support and
	// assign vehicles to task
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetTask() == IDT_SEEKINRANGE )
				continue;

			if( pUnit->GetType() == CUnit::vehicle )
			{
				// IDT_PATROL, IDT_SCOUT tasks
				if( pUnit->GetTask() )
				{
					if( pUnit->GetTask() != IDT_PATROL &&
						pUnit->GetTask() != IDT_SCOUT )
						continue;
				}
				else // only or combat vehicle without an assigned task
				{
					if( !pGameData->IsCombatVehicle(
						pUnit->GetTypeUnit()) )
						continue;
				}
				iRange = 0;
				EnterCriticalSection (&cs);
				CVehicle *pVehicle = theVehicleMap.GetVehicle( pAttacker->GetID() );
				if( pVehicle != NULL )
				{
					hex = pVehicle->GetHexHead();
					iRange = pVehicle->GetRange();
				
					// consider the target is a naval unit, 
					// so to shoot back, this unit must have naval attack
					if( bWaterOnly )
					{
						if( !pVehicle->GetAttack(CUnitData::naval) )
						{
						LeaveCriticalSection (&cs);
						continue;
						}
					}
				}
				LeaveCriticalSection (&cs);

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				// need to allow attacks where range and attack ability
				// allow the attack, but not unless the unit can get
				// within range				
				if( pGameData->GetRangeDistance(hex, hcAttacker) <= iRange )
				{
					// can unit get there?
					if( !m_pMap->m_pMapUtil->GetPathRating(
						hex, hcAttacker, pUnit->GetTypeUnit()) )
						continue;

					pUnit->ClearParam();
					// limit the number of attackers on one unit
					if( !pAttacker->AttackCount() )
					{
						pUnit->SetDataDW( pAttacker->GetID() );
						pAttacker->AttackedBy( pUnit->GetID() );
					}

					// assign unit to support attacked unit
					pUnit->SetParam(CAI_TARGETTYPE,0xFFFE );
					pUnit->SetStatus(0);
					pUnit->SetGoal( pTask->GetGoalID() );
					pUnit->SetTask( pTask->GetID() );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIGoalMgr::GetSupport() player %d for %ld, supporting unit %ld assigned ", 
		pTarget->GetOwner(), pTarget->GetID(), pUnit->GetID() );
#endif
				}
			}
		}
	}

	// count the number of each type of unit assigned
	WORD wCounts[STAGING_UNITTYPES];
	for( int i=0; i<STAGING_UNITTYPES; ++i )
		wCounts[i] = 0;

	//SetSupportStagingVehicle( wCounts, pTask );
   	pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetTask() != pTask->GetID() &&
				pUnit->GetGoal() != pTask->GetGoalID() )
				continue;

			// now, based on the type of support that will be
			// conducted, count up the types of units staged
			if( pTask->GetGoalID() == IDG_ADVDEFENSE )
			{
				switch( pUnit->GetTypeUnit() )
				{
					//case CTransportData::light_scout:
					//case CTransportData::med_scout:
					case CTransportData::heavy_scout:
					case CTransportData::light_tank:
					case CTransportData::med_tank:
					case CTransportData::heavy_tank:
						wCounts[0] += 1;
						break;
					case CTransportData::infantry_carrier:
						wCounts[1] += 1;
						break;
					case CTransportData::light_art:
					case CTransportData::med_art:
					case CTransportData::heavy_art:
						wCounts[2] += 1;
						break;
					case CTransportData::infantry:
					case CTransportData::rangers:
						wCounts[3] += 1;
						break;
					default:
						break;
				}
			}
			else if( pTask->GetGoalID() == IDG_SEAWAR )
			{
				switch( pUnit->GetTypeUnit() )
				{
					case CTransportData::cruiser:
						wCounts[0] += 1;
						break;
					case CTransportData::destroyer:
						wCounts[1] += 1;
						break;
					default:
						break;
				}
			}
		}
	}

	// now set up task just like a staging
	iRange = 0;
	for( i=0; i<STAGING_UNITTYPES; ++i )
	{
		pTask->SetTaskParam( (i+STAGING_UNITTYPES), wCounts[i] );
		// find the most of a type of units
		if( (int)wCounts[i] > iRange )
			iRange = wCounts[i];
	}
	if( !iRange )
	{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetSupport() no units available at this time" );
#endif
			return;
	}

	// use most of a type of unit to define staging area size
	iRange++;

	// determine type of staging area needed
	int iType = CTransportData::combat;
	if( pTask->GetGoalID() == IDG_SEAWAR )
		iType = CTransportData::ship;

	CHexCoord hcFrom(0,0), hcTo(0,0); 
	m_pMap->m_pMapUtil->GetStagingAreaNear( 
		hcTarget, hcFrom, hcTo, iRange, iRange, iType );
	if( !hcFrom.X() && !hcFrom.Y() )
	{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetSupport() unable to find staging area" );
#endif
			return;
	}

	// now set task params with area to stage in
	pTask->SetTaskParam(CAI_LOC_X,hcFrom.X());
	pTask->SetTaskParam(CAI_LOC_Y,hcFrom.Y());
	pTask->SetTaskParam(CAI_PREV_X,hcTo.X());
	pTask->SetTaskParam(CAI_PREV_Y,hcTo.Y());
	
	m_pMap->m_pMapUtil->FlagStagingArea( TRUE,
		pTask->GetTaskParam(CAI_LOC_X),pTask->GetTaskParam(CAI_LOC_Y),
		pTask->GetTaskParam(CAI_PREV_X),pTask->GetTaskParam(CAI_PREV_Y) );

	// and now send assigned units to staging area
   	pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetTask() != pTask->GetID() &&
				pUnit->GetGoal() != pTask->GetGoalID() )
				continue;

			hex = hcTarget;
			// try to find a staging hex, 
			m_pMap->m_pMapUtil->FindStagingHex(
				pTask->GetTaskParam(CAI_LOC_X), pTask->GetTaskParam(CAI_LOC_Y),
				pTask->GetTaskParam(CAI_PREV_X), pTask->GetTaskParam(CAI_PREV_Y),
				hex, pUnit->GetTypeUnit() );

			pUnit->SetDestination( hex );

			// force occupation of staging hex in map
			m_pMap->PlaceFakeVeh( hex, pUnit->GetTypeUnit() );
			// record that this unit is in the process of staging
			pUnit->SetParam( CAI_UNASSIGNED, (int)INPROCESS_TASK );

#if DEBUG_TASK_MGR
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIGoalMgr::GetSupport() player %d unit %ld going to %d,%d ", 
		pUnit->GetOwner(), pUnit->GetID(), hex.X(), hex.Y() );
#endif

		}
	}

	// finally, flag the task
	pTask->SetStatus(INPROCESS_TASK);


#if 0 // turn off old way
	int iRange;
	CHexCoord hex;
	// now go thru the units and for those units within
	// MIN_RANGE number of hexes from either attacker or target
	// and also has the appropriate tasks, then switch them
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;
			
			WORD wStatus = pUnit->GetStatus();
			if( (wStatus & CAI_IN_COMBAT) )
				continue;

			if( pUnit->GetTask() == IDT_SEEKINRANGE )
				continue;

			// determine if this is a building
			if( pUnit->GetType() == CUnit::building )
			{
				iRange = 0;
				// if this building is in range attack attacker
				EnterCriticalSection (&cs);
				CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					hex = pBldg->GetExitHex();
					//hex = pBldg->GetHex();
					iRange = pBldg->GetRange();
				
					// consider the target is a naval unit, 
					// so to shoot back, this unit must have naval attack
					if( bWaterOnly )
					{
						if( !pBldg->GetAttack(CUnitData::naval) )
						{
						LeaveCriticalSection (&cs);
						continue;
						}
					}
				}
				LeaveCriticalSection (&cs);

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				if( pGameData->GetRangeDistance( hex, hcAttacker ) 
					<= iRange )
				{
					if( pAttacker->AttackCount() < CAI_RANGE )
					{
						pUnit->AttackUnit( pAttacker->GetID() );
						pAttacker->AttackedBy( pUnit->GetID() );
						wStatus |= CAI_IN_COMBAT;
						pUnit->SetStatus( wStatus );
					}
					else
						return;
				}
			}
			else if( pUnit->GetType() == CUnit::vehicle )
			{
				// IDT_PATROL, IDT_PREPAREWAR, IDT_ESCORT IDT_SCOUT tasks
				if( pUnit->GetTask() )
				{
					if( pUnit->GetTask() != IDT_PREPAREWAR &&
						pUnit->GetTask() != IDT_PATROL &&
						pUnit->GetTask() != IDT_SCOUT &&
						pUnit->GetTask() != IDT_ESCORT )
						continue;
				}
				else // only or combat vehicle without an assigned task
				{
					if( !pGameData->IsCombatVehicle(
						pUnit->GetTypeUnit()) )
						continue;
				}

				if( pUnit->GetDataDW() )
				{
					// already attacking the attacker
					if( pUnit->GetDataDW() == pAttacker->GetID() )
						continue;
				}

				iRange = 0;
				EnterCriticalSection (&cs);
				CVehicle *pVehicle = theVehicleMap.GetVehicle( pAttacker->GetID() );
				if( pVehicle != NULL )
				{
					hex = pVehicle->GetHexHead();
					iRange = pVehicle->GetRange();
				
					// consider the target is a naval unit, 
					// so to shoot back, this unit must have naval attack
					if( bWaterOnly )
					{
						if( !pVehicle->GetAttack(CUnitData::naval) )
						{
						LeaveCriticalSection (&cs);
						continue;
						}
					}
				}
				LeaveCriticalSection (&cs);

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				// need to allow attacks where range and attack ability
				// allow the attack, but not unless the unit can get
				// within range				
				if( pGameData->GetRangeDistance(hex, hcAttacker) <= iRange )
				{
					// can unit get there?
					if( !m_pMap->m_pMapUtil->GetPathRating(
						hex, hcAttacker, pUnit->GetTypeUnit()) )
						continue;

					pUnit->SetDataDW(0);
					pUnit->SetParam(CAI_TARGETTYPE,0xFFFE );

					// limit the number of attackers on one unit
					if( !pAttacker->AttackCount() )
					{
						pUnit->AttackUnit( pAttacker->GetID() );
						pUnit->SetDataDW( pAttacker->GetID() );

						pAttacker->AttackedBy( pUnit->GetID() );
					}

					wStatus |= CAI_IN_COMBAT;
					wStatus |= CAI_TASKSWITCH;
					pUnit->SetStatus( wStatus );

					// save old task
					pUnit->SetParam( CAI_UNASSIGNED, pUnit->GetTask() );
					// assign unit to support attacked unit
					pUnit->SetTask( IDT_SEEKINRANGE );

					CTransportData const *pVehData = 
						pGameData->GetTransportData( pUnit->GetTypeUnit());
					if( pVehData == NULL )
						return;

					m_pMap->m_pMapUtil->FindDefenseHex( 
						hex, hcAttacker, iRange, iRange, pVehData );
					pUnit->SetDestination( hcAttacker );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIGoalMgr::GetSupport() player %d for %ld, supporting unit %ld assigned ", 
		pTarget->GetOwner(), pTarget->GetID(), pUnit->GetID() );
#endif
				}
			}
			else
				continue;
		}
	}
#endif // turn off old way

}

#endif // turn off completely

//
// this finds the nearest known opfor, and sets the map utility
// with the opfor flag and rocket hex location
//
void CAIGoalMgr::SetMapOpFor( void )
{
	CHexCoord hexRocket;
	CHexCoord hexNearest(m_pMap->m_iBaseX,m_pMap->m_iBaseY);
	CAIOpFor *pOpFor = m_plOpFors->GetNearest( hexNearest, TRUE );
	if( pOpFor != NULL )
	{
		m_pMap->m_pMapUtil->m_bUseOpFor = TRUE;
		pGameData->FindBuilding( 
			CStructureData::rocket, pOpFor->GetPlayerID(), hexRocket );
		m_pMap->m_pMapUtil->m_hexOpFor = hexRocket;
	}
	else
	{
		m_pMap->m_pMapUtil->m_bUseOpFor = FALSE;
	}
}

void CAIGoalMgr::SetKnownOpFor( int iOpForID )
{
	CAIOpFor *pOpFor = m_plOpFors->GetOpFor(iOpForID);
	if( pOpFor != NULL )
		pOpFor->SetKnown(TRUE);
}

//
// return the id of the opfor that qualifies based on type passed
//
int CAIGoalMgr::GetOpForId( int iType, CHexCoord hex, BOOL bKnown )
{
	CAIOpFor *pOpFor = NULL;
	if( iType == WEAKEST_TARGET )
	{
		pOpFor = m_plOpFors->GetWeakest(bKnown);
	}
	else if( iType == NEAREST_TARGET )
	{
		CHexCoord hexNearest;

		if( !hex.X() && !hex.Y() )
		{
			// get location of this mgr's rocket
			CBuilding *pBldg = 
				pGameData->GetBuildingData( CAI_OPFOR_UNIT,m_dwRocket );
			if( pBldg != NULL )
				hexNearest = pBldg->GetExitHex();
				//hexNearest = pBldg->GetHex();
		}
		else
			hexNearest = hex;

		pOpFor = m_plOpFors->GetNearest( hexNearest, bKnown );
	}
	if( pOpFor != NULL && 
		pOpFor->GetPlayerID() != m_iPlayer )
		return( pOpFor->GetPlayerID() );

	return(0);
}
//
// use this function to determine what base hex to use for 
// scouting tasks
//
void CAIGoalMgr::GetScoutBaseHex( CAITask *pTask )
{
	// right now, default to current base hex of the map
	pTask->SetTaskParam(FROM_X,m_pMap->m_iBaseX);
	pTask->SetTaskParam(FROM_Y,m_pMap->m_iBaseY);
}

//
// find a new patrol pattern for this water only unit of
// type ptdShip, and return them in the unit's params
//
void CAIGoalMgr::GetSeaPatrol( CAIUnit *pShip, CAITask *pTask, 
	CTransportData const *ptdShip )
{
	CAIHex *phexPts = NULL;
	int iPattern = 0;

	// the task can contain up to four patrol base hexes
	// which the goal manager determines.  each base hex
	// is used to define patrol points that cover that
	// base hex.  a unit is assigned to a patrol base
	// hex with the value 1-4 and stored in unit params
	// at the CAI_UNASSIGNED offset
	if( !pTask->GetTaskParam(0) &&
		!pTask->GetTaskParam(1) )
	{
		phexPts = m_pMap->m_pMapUtil->FindWaterPatrol(
			pTask->GetGoalID() );
		if( phexPts != NULL )
		{
			for( int i=0; i<NUM_PATROL_POINTS; i++ )
			{
				CAIHex *pHex = &phexPts[i];
				switch(i)
				{
				case 0:
					pTask->SetTaskParam( 0, pHex->m_iX );
					pTask->SetTaskParam( 1, pHex->m_iY );
					break;
				case 1:
					pTask->SetTaskParam( 2, pHex->m_iX );
					pTask->SetTaskParam( 3, pHex->m_iY );
					break;
				case 2:
					pTask->SetTaskParam( 4, pHex->m_iX );
					pTask->SetTaskParam( 5, pHex->m_iY );
					break;
				case 3:
					pTask->SetTaskParam( 6, pHex->m_iX );
					pTask->SetTaskParam( 7, pHex->m_iY );
					break;
				default:
					break;
				}
			}
			delete [] phexPts;
		}
		else
		{
			pShip->ClearParam();
			return;
		}
	}

	// then process all water only units with this task
	// and determine the hex not currently patrolled and
	// select it, or if all are patrolled then select
	// the one with the fewest units patrolling it
	BYTE cAssigned[NUM_PATROL_POINTS];
	for( int i=0; i<NUM_PATROL_POINTS; ++i )
		cAssigned[i] = 0;

	POSITION pos = m_plUnits->GetHeadPosition();
	while( pos != NULL )
	{   
   		CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
   		if( pUnit != NULL )
   		{
   			ASSERT_VALID( pUnit );

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			if( pUnit->GetOwner() != m_iPlayer )
				continue;
			if( pShip->GetID() == pUnit->GetID() )
				continue;

			if( pUnit->GetTask() == pTask->GetID() &&
				pUnit->GetGoal() == pTask->GetGoalID() )
			{
				// patrol patterns + 1 is stored here
				if( pUnit->GetParam(CAI_UNASSIGNED) )
				{
					cAssigned[pUnit->GetParam(CAI_UNASSIGNED)-1] = 1;
				}
			}
		}
	}

	// select one of the patrol patterns by picking one not
	// previously assigned and if all all, then a random one
	for( i=0; i<NUM_PATROL_POINTS; ++i )
	{
		if( !cAssigned[i] )
		{
			iPattern = (i+1);
			break;
		}
	}
	if( !iPattern )
	{
		i = pGameData->GetRandom(NUM_PATROL_POINTS);
		if( i < NUM_PATROL_POINTS )
			iPattern = (i+1);
	}

	// the iPattern value indicates the hex in the task's
	// params to which this unit will patrol
	pShip->SetParam( CAI_UNASSIGNED, iPattern );

	CHexCoord hexProtect(0,0);
	iPattern--;
	switch(iPattern)
	{
		case 0:
			hexProtect.X( pTask->GetTaskParam(0) );
			hexProtect.Y( pTask->GetTaskParam(1) );
			break;
		case 1:
			hexProtect.X( pTask->GetTaskParam(2) );
			hexProtect.Y( pTask->GetTaskParam(3) );
			break;
		case 2:
			hexProtect.X( pTask->GetTaskParam(4) );
			hexProtect.Y( pTask->GetTaskParam(5) );
			break;
		case 3:
			hexProtect.X( pTask->GetTaskParam(6) );
			hexProtect.Y( pTask->GetTaskParam(7) );
			break;
		default:
			break;
	}

	// ask map's utility to find patrol points based on
	// hex passed which serves as base of patrol AND the
	// waterdepth ability of the passed ship unit
	phexPts = 
		m_pMap->m_pMapUtil->FindWaterPatrol( hexProtect, ptdShip );
	if( phexPts != NULL )
	{
		for( i=0; i<NUM_PATROL_POINTS; i++ )
		{
			CAIHex *pHex = &phexPts[i];
			switch(i)
			{
				case 0:
					pShip->SetParam( 0, pHex->m_iX );
					pShip->SetParam( 1, pHex->m_iY );
					break;
				case 1:
					pShip->SetParam( 2, pHex->m_iX );
					pShip->SetParam( 3, pHex->m_iY );
					break;
				case 2:
					pShip->SetParam( 4, pHex->m_iX );
					pShip->SetParam( 5, pHex->m_iY );
					break;
				case 3:
					pShip->SetParam( 6, pHex->m_iX );
					pShip->SetParam( 7, pHex->m_iY );
					break;
				default:
					break;
			}
		}
		delete [] phexPts;
	}
}

//
// return the count of important buildings we have built
//
WORD CAIGoalMgr::GetBuildingCnt( void )
{
	WORD wCnt = 0;
	if( m_pwaBldgs != NULL )
	{
		for( int i=CStructureData::barracks_2; i<m_iNumBldgs; ++i )
		{
			wCnt += m_pwaBldgs[i];
		}
	}
	return( wCnt );
}

//
// see if a path to within range can be reached
//
BOOL CAIGoalMgr::GetPathRating( CHexCoord& hexVeh, CHexCoord& hexAttacked, int iVehType )
{
	CTransportData const *pVehData = 
		pGameData->GetTransportData( iVehType);
	if( pVehData == NULL )
		return FALSE;

	// get CTransportData for unit type
	// then for each hex within firing range
	// test for a path and on first path return
	// a spiral search outward from the attacked hex
	CHexCoord hcFrom, hcTo, hcAt;
	int iStep = 1;
	int iMaxSteps = pVehData->_GetRange() - 1;
	while( iStep < iMaxSteps )
	{
		hcFrom.X( hcAt.Wrap(hexAttacked.X()-iStep) );
		hcFrom.Y( hcAt.Wrap(hexAttacked.Y()-iStep) );
		hcTo.X( hcAt.Wrap(hexAttacked.X()+iStep) );
		hcTo.Y( hcAt.Wrap(hexAttacked.Y()+iStep) );

		int iDeltax = hcAt.Wrap(hcTo.X()-hcFrom.X()) + 1;
		int iDeltay = hcAt.Wrap(hcTo.Y()-hcFrom.Y()) + 1;

		for( int iY=0; iY<iDeltay; ++iY )
		{
			hcAt.Y( hcAt.Wrap(hcFrom.Y()+iY) );

			for( int iX=0; iX<iDeltax; ++iX )
			{
				hcAt.X( hcAt.Wrap(hcFrom.X()+iX) );

				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hcAt.X() != hcFrom.X() &&
					hcAt.X() != hcTo.X() && 
					hcAt.Y() != hcFrom.Y() &&
					hcAt.Y() != hcTo.Y() )
					continue;

				CHex *pGameHex = theMap.GetHex( hcAt );
				if( pGameHex == NULL )
					continue;

				BYTE bUnits = pGameHex->GetUnits();
				// skip buildings
				if( (bUnits & CHex::bldg) )
					continue;

				// check the game to see if vehicle can travel
				if( !pVehData->CanTravelHex( pGameHex ) )
					continue;

				if( m_pMap->m_pMapUtil->GetPathRating(hexVeh, hcAt, iVehType) )
					return TRUE;
			}
		}
	}
	return FALSE;
}

//
// first, need at least one unit patroling each building
// 
DWORD CAIGoalMgr::GetPatrolBuilding( CAITask *pTask, BOOL bSeaBuilding /*=FALSE*/ )
{
	// for each building, count number of vehicles with pTask
	// and the building assigned to the vehicle 
	int iBest = 0xFFFE;
	CAIUnit *pProtected = NULL;
   	
   	POSITION posB = m_plUnits->GetHeadPosition();
   	while( posB != NULL )
   	{   
       	CAIUnit *pBldg = (CAIUnit *)m_plUnits->GetNext( posB );
       	if( pBldg != NULL )
       	{
       		ASSERT_VALID( pBldg );

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			if( pBldg->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a building
			if( pBldg->GetType() != CUnit::building )
				continue;
			
			// skip these damn, worthless buildings
			//if( pBldg->GetTypeUnit() >= CStructureData::apartment_1_1 &&
			//	pBldg->GetTypeUnit() <= CStructureData::office_3_2 )
			//	continue;

			if( bSeaBuilding )
			{
				if( pBldg->GetTypeUnit() != CStructureData::seaport &&
					pBldg->GetTypeUnit() != CStructureData::shipyard_1 &&
					pBldg->GetTypeUnit() != CStructureData::shipyard_3 )
					continue;
			}

			// count the units patroling this building
			int iCnt = 0;
   			POSITION posU = m_plUnits->GetHeadPosition();
   			while( posU != NULL )
   			{   
       			CAIUnit *pVeh = (CAIUnit *)m_plUnits->GetNext( posU );
       			if( pVeh != NULL )
       			{
       				ASSERT_VALID( pVeh );

#if THREADS_ENABLED
					// BUGBUG this function must yield
					myYieldThread();
					//if( myYieldThread() == TM_QUIT )
					//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
					if( pVeh->GetOwner() != m_iPlayer )
						continue;

					// determine if this is a vehicle
					if( pVeh->GetType() != CUnit::vehicle )
						continue;
					if( pVeh->GetGoal() != pTask->GetGoalID() ||
						pVeh->GetTask() != pTask->GetID() )
						continue;
					// only vehicles with this task remain
					if( pVeh->GetParamDW(CAI_PATROL) == pBldg->GetID() )
						iCnt++;
				}
			}
			// this usually grabs the building with the fewest 
			// number of units patrolling it
			if( iCnt < iBest )
			{
				iBest = iCnt;
				pProtected = pBldg;
			}
		}
	}

	if( pProtected != NULL )
	{
		// if any building needs a patrol, then iBest will be 0
		// and returning id will add a patrol
		if( !iBest )
			return( pProtected->GetID() );

		// every building has at least one patrolling unit
		// so consider patroling cranes
		DWORD dwBldg = pProtected->GetID();
		pProtected = NULL;

		// don't start patroling cranes until there is at least
		// one other building (other than the rocket) present
		if( !GetBuildingCnt() )
			return( dwBldg );

		// now go through all the cranes
		POSITION posC = m_plUnits->GetHeadPosition();
   		while( posC != NULL )
   		{   
       		CAIUnit *pCrane = (CAIUnit *)m_plUnits->GetNext( posC );
       		if( pCrane != NULL )
       		{
       			ASSERT_VALID( pCrane );

#if THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif				
				if( pCrane->GetOwner() != m_iPlayer )
					continue;

				// patrol landing_craft and cargo ships
				if( bSeaBuilding )
				{
					if( pCrane->GetTypeUnit() != 
						CTransportData::landing_craft &&
						pCrane->GetTypeUnit() != 
						CTransportData::light_cargo )
						continue;
				}
				else
				{
					// determine if this is a crane
					if( pCrane->GetTypeUnit() != 
						CTransportData::construction )
						continue;
				}

				// found a vehicle to patrol
				// count the units patroling this vehicle
				int iCnt = 0;
   				POSITION posU = m_plUnits->GetHeadPosition();
   				while( posU != NULL )
   				{   
       				CAIUnit *pVeh = (CAIUnit *)m_plUnits->GetNext( posU );
       				if( pVeh != NULL )
       				{
       					ASSERT_VALID( pVeh );

#if 0 //THREADS_ENABLED
						// BUGBUG this function must yield
						myYieldThread();
						//if( myYieldThread() == TM_QUIT )
						//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
						if( pVeh->GetOwner() != m_iPlayer )
							continue;

						// determine if this is a vehicle
						if( pVeh->GetType() != CUnit::vehicle )
							continue;
						if( pVeh->GetGoal() != pTask->GetGoalID() ||
							pVeh->GetTask() != pTask->GetID() )
							continue;
						// only vehicles with this task remain
						if( pVeh->GetParamDW(CAI_PATROL) == pCrane->GetID() )
						{
							iCnt++;
							break;
						}
					}
				} // end of looking for patrols of these vehicles


				// if no patrols on this vehicle, then consider patrolling it
				if( !iCnt )
				{
					// no other vehicles has been picked, so auto pick this one
					if( pProtected == NULL )
						pProtected = pCrane;
					else
					{
						// a pProtected has already been selected, so if it
						// has a goal/task assigned, it stays selected
						// else if the pCrane has goal/task assigned it
						// replaces pProtected
						if( !pProtected->GetGoal() && !pProtected->GetTask() )
						{
							if( !pCrane->GetGoal() && 
								!pCrane->GetTask() )
								pProtected = pCrane;
						}
					}
				}
			}
		}
		// if a vehicle was selected, then patrol it
		if( pProtected != NULL )
			return( pProtected->GetID() );
		// otherwise patrol the building with the fewest
		else
			return( dwBldg );
	}

	// all current buildings and needed vehicles have patrols,
	return(0);
}

void CAIGoalMgr::GetPatrolHex( CAIUnit *pUnit, CHexCoord& hex )
{
#if 0 //DEBUG_OUTPUT_GMGR
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetPatrolHex() for player %d unit %ld to patrol %ld ", 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetParamDW(CAI_PATROL) );
#endif

	// the building/crane to patrol is in pUnit->GetParamDW(CAI_PATROL)
	// find a random hex on the edge of pUnit's spotting range
	// that is at least spotting range away, that the pUnit can
	// find a path to from its current location
	CHexCoord hexVeh = hex;
	CHexCoord hexProtect(0,0);
	EnterCriticalSection (&cs);
	CBuilding *pBldg = 
		theBuildingMap.GetBldg( pUnit->GetParamDW(CAI_PATROL) );
	if( pBldg != NULL )
		hexProtect = pBldg->GetExitHex();
	else
	{
		CVehicle *pVehicle = 
			theVehicleMap.GetVehicle( pUnit->GetParamDW(CAI_PATROL) );
		if( pVehicle != NULL )
			hexProtect = pVehicle->GetHexHead();
	}
	LeaveCriticalSection (&cs);

	if( !hexProtect.X() && !hexProtect.Y() )
	{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetPatrolHex() patrol %ld objective not found ", 
pUnit->GetParamDW(CAI_PATROL) );
#endif
		return;
	}
	CTransportData const *pVehData = 
		pGameData->GetTransportData( pUnit->GetTypeUnit() );
	if( pVehData == NULL )
		return;

	//int iSpotting = pVehData->_GetSpottingRange() / 2;
	int iSpotting = pVehData->_GetSpottingRange() >> 1;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIGoalMgr::GetPatrolHex() for player %d unit %ld to patrol %ld at %d,%d", 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetParamDW(CAI_PATROL),
hexProtect.X(), hexProtect.Y() );
#endif

	// right now pick a staging hex within spotting range of building return in hex
	m_pMap->m_pMapUtil->FindStagingHex( hexProtect, iSpotting, iSpotting,
		 pUnit->GetTypeUnit(), hex, FALSE );

	// could not get a staging hex
	if( hexProtect == hex )
		hex = hexVeh;
	else
	{
		// if no path possible to hex
		// hex = hexVeh;
		if( !m_pMap->m_pMapUtil->GetPathRating( 
			hexVeh, hex, pVehData->GetType() ))
			hex = hexVeh;
	}
}

//
// consider patrol assignments
//
CAITask *CAIGoalMgr::GetPatrolTask( int iType )
{
	ASSERT_VALID( this );

	CAITask *pPickedTask = m_plTasks->GetPatrolTask(iType);
	if( pPickedTask == NULL )
		return( NULL );

	BOOL bSeaPatrol = FALSE;
	if( pPickedTask->GetGoalID() == IDG_SEAWAR ||
	    pPickedTask->GetGoalID() == IDG_REPELL ||
		pPickedTask->GetGoalID() == IDG_PIRATE ||
		pPickedTask->GetGoalID() == IDG_SEAINVADE ||
		pPickedTask->GetGoalID() == IDG_SHORES )
		bSeaPatrol = TRUE;

	// if all buildings/vehicles have a patrol, then return NULL
	DWORD dwBldg = GetPatrolBuilding( pPickedTask, bSeaPatrol );
	if( !dwBldg )
		return( NULL );

	// otherwise take the task
	return( pPickedTask );
}


//
// an existing game is being re-started, so goals should come
// from data out of a saved game file
//
void CAIGoalMgr::Load( CFile *pFile, CAIMap *pMap, 
	CAIUnitList *plUnits, CAIOpForList *plOpFors )
{
	m_pMap = pMap;
	m_plUnits = plUnits;
	m_plOpFors = plOpFors;
	
	m_iNeedApt = FALSE;
	m_iNeedOffice = FALSE;

	// get rocket id
	pFile->Read( (void*)&m_dwRocket, sizeof(DWORD) );
	// get guns/butter flag
	pFile->Read( (void*)&m_bGunsOrButter, sizeof(BOOL) );
	// get last food value
	pFile->Read( (void*)&m_iLastFood, sizeof(int) );
	pFile->Read( (void*)&m_iScenario, sizeof(int) );
	pFile->Read( (void*)&m_bOceanWorld, sizeof(BOOL) );
	pFile->Read( (void*)&m_bLakeWorld, sizeof(BOOL) );
	// not saving m_bNeedTrucks so it will be reset naturally
	
	// first, make sure goal list is empty
	if( m_plGoalList != NULL )
		m_plGoalList->DeleteList();

	int iCnt;
	pFile->Read( (void*)&iCnt, sizeof(int) );

	// if there are goals to read
	if( iCnt )
	{
		GoalBuff goalbuffer;
		int iBytes;
		for( int i=0; i<iCnt; ++i )
		{
			// read file data into buffer
			iBytes = 
				pFile->Read( (void*)&goalbuffer, sizeof(GoalBuff) );

			if( iBytes != sizeof(GoalBuff) )
				return;

			// count actual tasks for this goal
			for( int j=0; j<NUM_INITIAL_GOALS; ++j )
			{
				if( !goalbuffer.iTasks[j] )
					break;
			}
			// j contains number of actual tasks
			// create array for tasks
			CWordArray *pwaTasks = new CWordArray();
			pwaTasks->SetSize(j);
			for( j=0; j<pwaTasks->GetSize(); ++j )
				pwaTasks->SetAt( j, goalbuffer.iTasks[j] );

			//CAIGoal( WORD wID, BYTE cType, CWordArray *pwaTasks );
			CAIGoal *pGoal = new CAIGoal( (WORD)goalbuffer.iID,
				(BYTE)goalbuffer.iType, pwaTasks );
			m_plGoalList->AddTail( (CObject *)pGoal );

			// clean up array
			pwaTasks->RemoveAll();
			delete pwaTasks;
		}
	}

	TaskBuff taskbuffer;
	// make sure task list is empty
	if( m_plTasks != NULL )
		m_plTasks->DeleteList();

	// now, read count of tasks
	pFile->Read( (void*)&iCnt, sizeof(int) );
	
	if( iCnt )
	{
		int iBytes;
		for( int i=0; i<iCnt; ++i )
		{
			// read file data into buffer
			iBytes = 
				pFile->Read( (void*)&taskbuffer, sizeof(TaskBuff) );

			if( iBytes != sizeof(TaskBuff) )
				return;

			// create array for tasks params
			CWordArray *pwaParams = new CWordArray();
			pwaParams->SetSize(MAX_TASKPARAMS);
			for( int j=0; j<pwaParams->GetSize(); ++j )
				pwaParams->SetAt( j, taskbuffer.iParams[j] );
			//	CAITask( WORD wID, BYTE cType, BYTE cPriority, 
			//		WORD wOrderID, CWordArray *pwaParams );
			CAITask *pTask = new CAITask( (WORD)taskbuffer.iID,
				(BYTE)taskbuffer.iType,(BYTE)taskbuffer.iPriority,
				(WORD)taskbuffer.iOrderID, pwaParams );

			pTask->SetGoalID( (WORD)taskbuffer.iGoal );

			m_plTasks->AddTail( (CObject *)pTask );

			// clean up array
			pwaParams->RemoveAll();
			delete pwaParams;
		}
	}

	// now do all the arrays, 
	// which means the arrays have to be set up
	// but not necessarily initialized as the load does that

	pFile->Read( (void*)m_pwaRatios, 
		(sizeof(WORD)*m_iNumRatios) );
	pFile->Read( (void*)m_pwaUnits, 
		(sizeof(WORD)*m_iNumUnits) );
	pFile->Read( (void*)m_pwaBldgs, 
		(sizeof(WORD)*m_iNumBldgs) );
	pFile->Read( (void*)m_pwaMatOnHand, 
		(sizeof(WORD)*m_iNumMats) );
	pFile->Read( (void*)m_pwaAttribs, 
		(sizeof(WORD)*m_iNumAttribs) );
	pFile->Read( (void*)m_pwaBldgGoals, 
		(sizeof(WORD)*m_iNumBldgs) );
	pFile->Read( (void*)m_pwaVehGoals, 
		(sizeof(WORD)*m_iNumUnits) );
	pFile->Read( (void*)m_pwaMatGoals, 
		(sizeof(WORD)*m_iNumMats) );
	pFile->Read( (void*)m_iRDPath, 
		(sizeof(int)*CRsrchArray::num_types) );

	// after a load, the lists should all be reinitialized
	m_bGoalChange = TRUE;
}
//
// save goals data as an existing game into a saved game file
//
void CAIGoalMgr::Save( CFile *pFile )
{
	// save rocket id
	pFile->Write( (const void*)&m_dwRocket, sizeof(DWORD) );
	// save guns/butter flag
	pFile->Write( (const void*)&m_bGunsOrButter, sizeof(BOOL) );
	// save last food value
	pFile->Write( (const void*)&m_iLastFood, sizeof(int) );
	pFile->Write( (const void*)&m_iScenario, sizeof(int) );
	pFile->Write( (const void*)&m_bOceanWorld, sizeof(BOOL) );
	pFile->Write( (const void*)&m_bLakeWorld, sizeof(BOOL) );
	

	GoalBuff goalbuffer;

	// next, write count of goals
	int iCnt = m_plGoalList->GetCount();
	pFile->Write( (const void*)&iCnt, sizeof(int) );

	// if there are goals to write
	if( iCnt )
	{
    	POSITION pos = m_plGoalList->GetHeadPosition();
    	while( pos != NULL )
    	{   
        	CAIGoal *pGoal = (CAIGoal *)m_plGoalList->GetNext( pos );
        	if( pGoal != NULL )
        	{
        		ASSERT_VALID( pGoal );

				// move data to buffer
				goalbuffer.iID = (int)pGoal->GetID();
				goalbuffer.iType = (int)pGoal->GetType();

				for( int i=0; i<NUM_INITIAL_GOALS; ++i )
					goalbuffer.iTasks[i] = 0;
				
				i = 0;
				while( pGoal->GetTaskAt(i) )
				{
					goalbuffer.iTasks[i] = (int)pGoal->GetTaskAt(i);
					++i;
				}

				// now write buffer out
				pFile->Write( 
					(const void*)&goalbuffer, sizeof(GoalBuff) );
        	}
    	}
	}

	// now, write count of tasks
	iCnt = m_plTasks->GetCount();
	pFile->Write( (const void*)&iCnt, sizeof(int) );

	if( iCnt )
	{
		TaskBuff taskbuffer;
    	POSITION pos = m_plTasks->GetHeadPosition();
    	while( pos != NULL )
    	{   
        	CAITask *pTask = (CAITask *)m_plTasks->GetNext( pos );
        	if( pTask != NULL )
        	{
        		ASSERT_VALID( pTask );

				// move data to buffer
				taskbuffer.iID = (int)pTask->GetID();
				taskbuffer.iGoal = (int)pTask->GetGoalID();
				taskbuffer.iType = (int)pTask->GetType();
				taskbuffer.iPriority = (int)pTask->GetPriority();
				taskbuffer.iOrderID = (int)pTask->GetOrderID();

				for( int i=0; i<MAX_TASKPARAMS; ++i )
					taskbuffer.iParams[i] = 0;
				for( i=0; i<MAX_TASKPARAMS; ++i )
					taskbuffer.iParams[i] = (int)pTask->GetTaskParam(i);
				
				// now write buffer out
				pFile->Write( 
					(const void*)&taskbuffer, sizeof(TaskBuff) );
        	}
    	}
	}
	// now do all the arrays
	pFile->Write( (const void*)m_pwaRatios, 
		(sizeof(WORD)*m_iNumRatios) );
	pFile->Write( (const void*)m_pwaUnits, 
		(sizeof(WORD)*m_iNumUnits) );
	pFile->Write( (const void*)m_pwaBldgs, 
		(sizeof(WORD)*m_iNumBldgs) );
	pFile->Write( (const void*)m_pwaMatOnHand, 
		(sizeof(WORD)*m_iNumMats) );
	pFile->Write( (const void*)m_pwaAttribs, 
		(sizeof(WORD)*m_iNumAttribs) );
	pFile->Write( (const void*)m_pwaBldgGoals, 
		(sizeof(WORD)*m_iNumBldgs) );
	pFile->Write( (const void*)m_pwaVehGoals, 
		(sizeof(WORD)*m_iNumUnits) );
	pFile->Write( (const void*)m_pwaMatGoals, 
		(sizeof(WORD)*m_iNumMats) );
	pFile->Write( (const void*)m_iRDPath, 
		(sizeof(int)*CRsrchArray::num_types) );
	
}
//
// clean up on destruction
//
CAIGoalMgr::~CAIGoalMgr()
{
	ASSERT_VALID( this );

#if 0 //DEBUG_OUTPUT_GMGR
	ReportSummaries();
#endif

	if( m_pwaRatios != NULL )
	{
		//m_pwaRatios->RemoveAll();
		delete [] m_pwaRatios;
		m_pwaRatios = NULL;
	}

	if( m_plGoalList != NULL )
	{
		ASSERT_VALID( m_plGoalList );

		delete m_plGoalList;
		m_plGoalList = NULL;
	}

	if( m_pwaUnits != NULL )
	{
		//ASSERT_VALID( m_pwaUnits );

		//m_pwaUnits->RemoveAll();
		delete [] m_pwaUnits;
		m_pwaUnits = NULL;
	}
	if( m_pwaBldgs != NULL )
	{
		//ASSERT_VALID( m_pwaBldgs );

		//m_pwaBldgs->RemoveAll();
		delete [] m_pwaBldgs;
		m_pwaBldgs = NULL;
	}
	if( m_pwaAttribs != NULL )
	{
		//ASSERT_VALID( m_pwaAttribs );

		//m_pwaAttribs->RemoveAll();
		delete [] m_pwaAttribs;
		m_pwaAttribs = NULL;
	}
	DelBldgGoals();
	DelVehGoals();
	DelMatGoals();
	DelMatOnHand();
	DeleteTasks();
}

void CAIGoalMgr::DelBldgGoals( void )
{
	if( m_pwaBldgGoals != NULL )
	{
		//ASSERT_VALID( m_pwaBldgGoals );

		//m_pwaBldgGoals->RemoveAll();
		delete [] m_pwaBldgGoals;
		m_pwaBldgGoals = NULL;
	}
}
void CAIGoalMgr::DelVehGoals( void )
{
	if( m_pwaVehGoals != NULL )
	{
		//SERT_VALID( m_pwaVehGoals );

		//m_pwaVehGoals->RemoveAll();
		delete [] m_pwaVehGoals;
		m_pwaVehGoals = NULL;
	}
}

void CAIGoalMgr::ClearMatOnHand( void )
{
	if( m_pwaMatOnHand != NULL )
	{
		for( int i=0; i<m_iNumMats; ++i )
		{
			m_pwaMatOnHand[i] = 0;
		}
	}
}

void CAIGoalMgr::ClearMatGoals( void )
{
	if( m_pwaMatGoals != NULL )
	{
		for( int i=0; i<m_iNumMats; ++i )
		{
			m_pwaMatGoals[i] = 0;
		}
	}
}

void CAIGoalMgr::DelMatOnHand( void )
{
	if( m_pwaMatOnHand != NULL )
	{
		//ASSERT_VALID( m_pwaMatOnHand );

		//m_pwaMatOnHand->RemoveAll();
		delete [] m_pwaMatOnHand;
		m_pwaMatOnHand = NULL;
	}
}

void CAIGoalMgr::DelMatGoals( void )
{
	if( m_pwaMatGoals != NULL )
	{
		//ASSERT_VALID( m_pwaMatGoals );

		//m_pwaMatGoals->RemoveAll();
		delete [] m_pwaMatGoals;
		m_pwaMatGoals = NULL;
	}
}

void CAIGoalMgr::DeleteTasks( void )
{
	if( m_plTasks != NULL )
	{
		delete m_plTasks;
		m_plTasks = NULL;
	}
}

#if 0
/*
		//
		//
		//
		construction,
		oil_truck,
		med_truck,
		heavy_truck,
		light_scout,
		med_scout,
		heavy_scout,
		infantry_carrier,
		light_tank,
		med_tank,
		heavy_tank,
		light_art,
		med_art,
		heavy_art,
		light_cargo,
		gun_boat,
		destroyer,
		cruiser,
		landing_craft,
		infantry,
		rangers,
		marines, 

		// above are only placed by computer
		//
		//                  topics requiring
		//                  this building
		barracks_2,			1,45
		barracks_3,			52
		command_center,		3,18,21,28,34,36,37,43,46,50
		embassy,
		farm,				36
		fort_1,				40,43,46,50
		fort_2,				41,44,47,51
		fort_3,				42,45,48,52
		heavy,				5,30,39,41,45,48,52
		light_1,			3,11,18,27,29,37,43,46,50
		light_2,			4,13,19,27,38,40,44,47,51
		lumber,				17,31,34
		oil_well,			25,35
		refinery,			25,32,49
		coal,				34
		iron,
		copper,				35
		power_1,
		power_2,
		power_3,
		research,
		repair,
		seaport,
		shipyard_1,
		shipyard_3,			42
		smelter,			11,12,17,27,49
		warehouse,
		light_0,			2,6,12,17,28
		num_types };


static int aiEconPath[CRsrchArray::num_types]
{
	farm_1,              medium_facilities, medium_vehicle,
	mine_1,				 const_1,           manf_1,
	large_facilities,    mine_2,		    const_2,
	manf_2,              bridge,            fire_control,
	manf_3,              gas_turbine,       nuclear,
	advanced_facilities, heavy_vehicle,     const_3,
	armored_vehicle,     tanks,             artillery,
	balloons,            gliders,           fortification,
	prop_planes,         jet_planes,        rockets,
	atk_1,               def_1,             range_1,
	acc_1,               spot_1,            spot_2,
	range_1,             atk_2,             def_2,
	acc_2,               range_3,           atk_3,             
	def_3,               acc_3,             sailboats,
	motorboats,          heavy_naval,       spot_3,
	copper,              landing_craft,     cargo_handling,
	0,                   0,                 0,    0
};

static int aiCbtPath[CRsrchArray::num_types]
{
	fire_control,        medium_facilities, medium_vehicle,
	armored_vehicle,     tanks,             artillery,
	large_facilities,    heavy_vehicle,     fortification,
	advanced_facilities, balloons,          gliders,
	prop_planes,         jet_planes,        rockets,
	atk_1,               def_1,             range_1,
	acc_1,               spot_1,            spot_2,
	range_1,             atk_2,             def_2,
	acc_2,               range_3,           atk_3,             
	def_3,               acc_3,             sailboats,
	motorboats,          heavy_naval,       spot_3,
	bridge,              farm_1,            mine_1,
	manf_1,              const_1,           mine_2,
	manf_2,              gas_turbine,       const_2,
	manf_3,              const_3,		    nuclear,
	copper,              landing_craft,     cargo_handling,
	0,                   0,                 0,    0
};



	CRsrchArray::enum {	
					nothing,0
					balloons,1
					gliders,2
					prop_planes,3
					jet_planes,4
					rockets,5
					sailboats,6
					motorboats,7
					cargo_handling,8
					fire_control,9
					landing_craft,10
					heavy_naval,11
					medium_vehicle,12
					heavy_vehicle,13
					armored_vehicle,14
					artillery,15
					tanks,16
					medium_facilities,17
					large_facilities,18
					advanced_facilities,19
					fortification,20
					radio,21
					mail,22
					email,23
					telephone,24
					gas_turbine,25
					nuclear,26
					bridge,27
					const_1,28
					const_2,29
					const_3,30
					manf_1,31
					manf_2,32
					manf_3,33
					mine_1,34
					mine_2,35
					farm_1,36
					spot_1,37
					spot_2,38
					spot_3,39
					range_1,40
					range_2,41
					range_3,42
					atk_1,43
					atk_2,44
					atk_3,45
					def_1,46
					def_2,47
					def_3,48
					copper,49
					acc_1,50
					acc_2,51
					acc_3,52
					CRsrchArray::num_types	};

*/
#endif

// end of CAIGMgr.cpp
