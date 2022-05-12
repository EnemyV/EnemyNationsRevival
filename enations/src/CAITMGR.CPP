////////////////////////////////////////////////////////////////////////////
//
//  CAITMgr.cpp :  CAITaskMgr object implementation
//                 the Last Planet AI
//               
//  Last update:    10/25/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "CAITMgr.hpp"
#include "CPathMgr.h"
#include "CAIData.hpp"
#include "logging.h"	// dave's logging system


#define new DEBUG_NEW

#if THREADS_ENABLED
extern CException *pException;		// standard exception for yielding
#endif

extern CAITaskList *plTaskList;	// standard task list
extern CAIGoalList *plGoalList;	// standard goal list
extern CAIData *pGameData;	// pointer to game data interface
extern CRITICAL_SECTION cs;	// used by threads
extern CPathMgr thePathMgr;		// the pathfinding object (no yield)

WORD wPatrols, wBuildings;

//
// upon creation, the task manager will process the goals associated
// with this player, and create a private list of the tasks needed
// to achieve those goals
//
CAITaskMgr::CAITaskMgr( BOOL bRestart, int iPlayer, CAIGoalMgr *pGoalMgr )
{
	m_iPlayer = iPlayer;
	m_bRestart = bRestart;
	
	ASSERT_VALID( pGoalMgr );
	m_pGoalMgr = pGoalMgr;
	
	ASSERT_VALID( this );
}

//
// process the goals of this manager and make
// sure all tasks of those goals are recorded
// in the manager's local list
//
// Consider if there is a CAIUnit vehicle of 
// message that needs additional orders
//
// Consider all CAIUnit vehicles without tasks
// and find task for them, and issue orders
// appropriate to their assigned task
//
void CAITaskMgr::Manage( CAIMsg *pMsg )
{
	ASSERT_VALID( this );
	
	// update the tasks of this manager if there
	// has been a change to its goal manager
	if( m_pGoalMgr != NULL &&
		m_pGoalMgr->GetGoalChange() )
	{
		ASSERT_VALID( m_pGoalMgr );

		// BUGBUG this maybe should not reset because other
		// things might want to happen based on goal change
		m_pGoalMgr->ResetGoalChange();

		// remove unwanted, add new, don't change existing
		UpdateTasks(pMsg);
	}

	if( pMsg == NULL )
		return;

	if( pMsg->m_iMsg == CNetCmd::bldg_stat &&
		pMsg->m_uFlags == CMsgBldgStat::built &&
		pMsg->m_idata2 == 100 &&
		pMsg->m_idata3 == m_iPlayer )
	{
		UnitControl( pMsg );
		// the construction trucks involved with this completion
		// are probably now available for re-assignment
		AssignUnits();

		return;
	}

	// this could be the signal to a patrolling unit
	if( pMsg->m_iMsg == CNetCmd::unit_attacked ||
		pMsg->m_iMsg == CNetCmd::unit_damage )
	{
		AttackAlert( pMsg );
		return;
	}

	if( pMsg->m_iMsg == CNetCmd::see_unit )
	{
		SpottingAlert( pMsg );
		return;
	}

	if( pMsg->m_iMsg == CNetCmd::unit_damage &&
		pMsg->m_idata3 == m_iPlayer )
	{
		// if unit has < 60% left
		RepairUnit( pMsg, DAMAGE_2 );
		return;
	}
	

	// on special other cases do an assignment
	//
	// start of game
	if( pMsg->m_iMsg == CNetCmd::cmd_play )
		AssignUnits();
	// an opfor rocket was found
	if( pMsg->m_iMsg == CNetCmd::bldg_new &&
		pMsg->m_idata3 != m_iPlayer &&
		pMsg->m_idata1 == CStructureData::rocket )
		AssignUnits();
	
	// or distribute units on patrol
	if( pMsg->m_iMsg == CNetCmd::bldg_new &&
		pMsg->m_idata3 == m_iPlayer )
	{
		BalancePatrols( pMsg );
	}

// also need to only process the units of the task mgr
// when there is reason to believe a unit is unassigned
// and not process by default on every cycle into this
//
	if( (pMsg->m_iMsg == CNetCmd::bldg_new ||
		pMsg->m_iMsg == CNetCmd::veh_new) &&
		pMsg->m_idata3 == m_iPlayer )
	{
		// if difficulty is IMPOSSIBLE, then stage an initial assault
		if( pMsg->m_idata1 == CStructureData::rocket &&
			pGameData->m_iSmart == (NUM_DIFFICUTY_LEVELS-1) )
			AssignInitialAssault();

		if( pMsg->m_dwID )
			UnitControl( pMsg );

		// go thru list of CAIUnits for this player
		// and for those units which do not have a
		// goal/task assigned, assign one and cause
		// task orders to be issued
		AssignUnits();

	}	// end of if message caused a change

	// deal with a construction site problem
	if( pMsg->m_iMsg == CNetCmd::err_build_bldg &&
		pMsg->m_idata3 == m_iPlayer )
		ConstructionError(pMsg);

	if( pMsg->m_iMsg == CNetCmd::road_done &&
		pMsg->m_idata3 == m_iPlayer )
		AssignUnits();

	if( pMsg->m_iMsg == CNetCmd::delete_unit &&
		pMsg->m_idata3 != m_iPlayer )
		ClearTarget(pMsg);
}

void CAITaskMgr::AssignInitialAssault(void)
{
	// select an IDT_PREPAREWAR task to handle initial assault
	// 
	CAITask *pTask = 
		m_pGoalMgr->m_plTasks->GetTask( IDT_PREPAREWAR, IDG_ADVDEFENSE );
	if( pTask == NULL )
	{
		pTask =
			m_pGoalMgr->m_plTasks->GetTask( IDT_PREPAREWAR, IDG_LANDWAR );
		if( pTask == NULL )
			return;
	}

	m_pGoalMgr->GetStagingArea(pTask);

	// get width/height to determine max units
	CHexCoord hcStart( pTask->GetTaskParam(CAI_LOC_X),
		pTask->GetTaskParam(CAI_LOC_Y) );
	CHexCoord hcEnd( pTask->GetTaskParam(CAI_PREV_X),
		pTask->GetTaskParam(CAI_PREV_Y) );
	CHexCoord hex;
	hex.X( hex.Wrap( hcEnd.X() - hcStart.X() ));
	hex.Y( hex.Wrap( hcEnd.Y() - hcStart.Y() ));

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::AssignInitialAssault() for player %d for goal %d task %d ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID() );

logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"staging area from %d,%d to %d,%d  width=%d height=%d ",
hcStart.X(), hcStart.Y(), hcEnd.X(), hcEnd.Y(), hex.X(), hex.Y() );
#endif

	// count infantry_carrier, infantry, light/med/heavy_tank, light_art
	// in units list that are not assigned, and assign up to 3/4 of area
	// or 3/4 of units counted, which ever is smaller
	int iCounts[STAGING_UNITTYPES];
	int iAssigned[STAGING_UNITTYPES];
	for( int i=0; i<STAGING_UNITTYPES; ++i )
	{
		iCounts[i] = 0;
		iAssigned[i] = 0;
	}

	int iMax=0;

TryAgain:

	int iUnits=0;

   	POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// skip own units assigned to other tasks
			if( pUnit->GetTask() || pUnit->GetGoal()  )
				continue;

			// count unassigned qualifying units
			iUnits++;

			switch( pUnit->GetTypeUnit() )
			{
				case CTransportData::light_tank:
				case CTransportData::med_tank:
				case CTransportData::heavy_tank:
				case CTransportData::heavy_scout:
					iCounts[0] += 1;
					break;
				//case CTransportData::infantry_carrier:
				//	iCounts[1] += 1;
				//	break;
				case CTransportData::light_art:
					iCounts[2] += 1;
					break;
				//case CTransportData::infantry:
				//	iCounts[3] += 1;
				//	break;
				default:
					// make adjustment to count if not qualifying
					if( iUnits )
						iUnits--;
					break;
			}
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		}
	}

	// no units were counted as available, so they must already
	// be assigned to another staging task
	if( !iUnits )
	{
   		pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
   		while( pos != NULL )
   		{   
       		CAIUnit *pUnit = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
       		if( pUnit != NULL )
       		{
       			ASSERT_VALID( pUnit );

				if( pUnit->GetOwner() != m_iPlayer )
					continue;

				// look for units that are assigned for staging
				// or patroling because they are combat units
				if( pUnit->GetTask() != IDT_PREPAREWAR &&
					pUnit->GetTask() != IDT_PATROL )
					continue;

				switch( pUnit->GetTypeUnit() )
				{
				case CTransportData::light_tank:
				case CTransportData::med_tank:
				case CTransportData::heavy_tank:
				case CTransportData::heavy_scout:
				//case CTransportData::infantry_carrier:
				case CTransportData::light_art:
				//case CTransportData::infantry:
					ClearTaskUnit(pUnit);
					break;
				default:
					break;
				}
			}
		}

#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

		// try one more time
		iMax++;
		if( iMax == 1 )
			goto TryAgain;
	}

	// adjust to use only 3/4 max
	iUnits=0;
	iMax = ((hex.X() * hex.Y()) / 4) * 3;
	//iUnits = (iCounts[3] / 4) * 3;
	//iAssigned[3] = iUnits; // inf
	iUnits = (iCounts[2] / 4) * 3;
	iAssigned[2] = iUnits; // art
	iAssigned[3] = iCounts[1]; // inf is same as ifv
	iAssigned[1] = iCounts[1]; // ifv
	iUnits = (iCounts[0] / 4) * 3;
	iAssigned[0] = iUnits; // tanks

	// force no inf and IFVs
	iAssigned[3] = 0;
	iAssigned[1] = 0;
	
	iUnits = 0;
	for( i=0; i<STAGING_UNITTYPES; ++i )
		iUnits += iAssigned[i];
	// and not all units
	if( iUnits > iMax )
		iUnits = iMax;

	// set up which unit types we want
	iMax = iUnits;
	iCounts[3] = CTransportData::infantry;
	iCounts[2] = CTransportData::light_art;
	iCounts[1] = CTransportData::infantry_carrier;
	iCounts[0] = 0;
	// starting with inf and moving toward tanks assign units
	for( i=STAGING_UNITTYPES-1; i>=0; --i )
	{
		iUnits = 0;
   		pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
   		while( pos != NULL )
   		{   
       		CAIUnit *pUnit = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
       		if( pUnit != NULL )
       		{
       			ASSERT_VALID( pUnit );

				if( pUnit->GetOwner() != m_iPlayer )
					continue;

				// skip own units assigned to other tasks
				if( pUnit->GetTask() || pUnit->GetGoal()  )
					continue;

				if( iCounts[i] )
				{
					if( pUnit->GetTypeUnit() == iCounts[i] )
					{
						iAssigned[i] -= 1;
						iMax--;
						iUnits++;
						pUnit->SetTask(pTask->GetID());
						pUnit->SetGoal(pTask->GetGoalID());
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AssignInitialAssault player %d unit %ld goal %d task %d ", 
pUnit->GetOwner(), pUnit->GetID(), pTask->GetGoalID(), pTask->GetID() );
#endif
					}
				}
				else
				{
					if( pUnit->GetTypeUnit() >= 
						CTransportData::light_tank &&
						pUnit->GetTypeUnit() <=
						CTransportData::heavy_tank )
					{
						iAssigned[i] -= 1;
						iMax--;
						iUnits++;
						pUnit->SetTask(pTask->GetID());
						pUnit->SetGoal(pTask->GetGoalID());
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AssignInitialAssault player %d unit %ld goal %d task %d ", 
pUnit->GetOwner(), pUnit->GetID(), pTask->GetGoalID(), pTask->GetID() );
#endif
					}
				}

#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			}
			// assigned all we wanted of this type
			if( iAssigned[i] <= 0 )
				break;
			if( iMax <= 0 )
				break;
		}

		// save count of units assigned
		iCounts[i] = iUnits;

		if( iMax <= 0 )
			break;
	}
	if( iCounts[0] )
	{
		iMax = iCounts[0] + pGameData->m_iSmart;
		iMax += pGameData->GetRandom(iMax);
	}
	else
		iMax = 0;
	pTask->SetTaskParam(CAI_TF_TANKS,iMax);

	if( iCounts[1] )
	{
		iMax = iCounts[1] + pGameData->m_iSmart;
		iMax += pGameData->GetRandom(iMax);
	}
	else
		iMax = 0;
	// force no IFVs
	pTask->SetTaskParam(CAI_TF_IFVS,0);

	if( iCounts[2] )
	{
		iMax = iCounts[2] + pGameData->m_iSmart;
		iMax += pGameData->GetRandom(iMax);
	}
	else
		iMax = 0;
	pTask->SetTaskParam(CAI_TF_ARTILLERY,iMax);

	if( iCounts[3] )
	{
		iMax = iCounts[3] + pGameData->m_iSmart;
		iMax += pGameData->GetRandom(iMax);
	}
	else
		iMax = 0;
	// force no infantry
	pTask->SetTaskParam(CAI_TF_INFANTRY,0);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AssignInitialAssault() for player %d for goal %d task %d ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID() );

logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"AFV=%d  IFV=%d  ART=%d  INF=%d \n",
pTask->GetTaskParam(CAI_TF_TANKS),pTask->GetTaskParam(CAI_TF_IFVS),
pTask->GetTaskParam(CAI_TF_ARTILLERY),pTask->GetTaskParam(CAI_TF_INFANTRY) );
#endif
}

//
// go thru list of CAIUnits for this player
// and for those units which do not have a
// goal/task assigned, assign one and cause
// task orders to be issued
//
void CAITaskMgr::AssignUnits( void )
{
	if( m_pGoalMgr->m_plUnits != NULL )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIUnit *pUnit = 
            	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
            if( pUnit != NULL )
            {
            	ASSERT_VALID( pUnit );

				if( pUnit->GetOwner() != m_iPlayer )
					continue;

				// do not assign
				// production tasks to buildings here, but
				// let it happen in UnitControl()
				if( pUnit->GetType() == CUnit::building )
					continue;

				pUnit->SetParamDW(CAI_ROUTE_X, 0 );
				pUnit->SetParamDW(CAI_ROUTE_Y, 0 );

				// unit has no task assigned
				// this should pick up new units
				// and units whose tasks are gone
				if( !pUnit->GetTask() &&
					!pUnit->GetGoal() )
				{
					AssignTask( pUnit );

					if( pUnit->GetType() == CUnit::vehicle )
					{
						CHexCoord hexVeh(0,0);
						EnterCriticalSection (&cs);
						CVehicle *pVehicle = 
						theVehicleMap.GetVehicle( pUnit->GetID() );
						if( pVehicle != NULL )
							hexVeh = pVehicle->GetHexHead();
						LeaveCriticalSection (&cs);
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AssignTask unit %ld player %d at %d,%d ", 
pUnit->GetID(), pUnit->GetOwner(), hexVeh.X(), hexVeh.Y() );
#endif
					}
				}
				// every unit processed gets this chance
				GenerateTaskOrder( pUnit );
            }
        }
	}
}

//
// for each building without a patrol
//   find the lowest rated building below it with a patrol
//   and reassign the first patroling unit of that building
//   to the building without a patrol
// end
//
void CAITaskMgr::BalancePatrols( void )
{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::BalancePatrols() player %d \n", m_iPlayer );
#endif

	CAIUnit *pBldg = NULL;
   	POSITION posB = m_pGoalMgr->m_plUnits->GetHeadPosition();
   	while( posB != NULL )
   	{   
       	pBldg = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posB );
       	if( pBldg != NULL )
       	{
       		ASSERT_VALID( pBldg );

			if( pBldg->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a building
			if( pBldg->GetType() == CUnit::building )
			{
				// skip these damn, worthless buildings
				if( pBldg->GetTypeUnit() >= CStructureData::apartment_1_1 &&
					pBldg->GetTypeUnit() <= CStructureData::office_3_2 )
					continue;

				if( !IsPatrolled(pBldg) )
				{
					FindAvailablePatrol(pBldg);
				}
			}
		}
	}
}

//
// check the patroling units for one that is patroling this building
//
BOOL CAITaskMgr::IsPatrolled( CAIUnit *pBldg )
{
   	POSITION posU = m_pGoalMgr->m_plUnits->GetHeadPosition();
   	while( posU != NULL )
   	{   
    	CAIUnit *pVeh = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posU );
       	if( pVeh != NULL )
       	{
       		ASSERT_VALID( pVeh );
			if( pVeh->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a vehicle
			if( pVeh->GetType() != CUnit::vehicle )
				continue;
			if( pVeh->GetTask() != IDT_PATROL )
				continue;

			// skip units onboard other units
			if( pGameData->IsLoaded(pVeh->GetID()) )
				continue;

			// is this vehicle patroling the passed building
			if( pVeh->GetParamDW(CAI_PATROL) 
				== pBldg->GetID() )
				return TRUE;
		}
	}
	return FALSE;
}

//
// for each lower rated building than what is passed, that is patroled
// then select the first patroling unit assigned and reassign it to
// the passed building
//
// use this for rating builings
// int CAIMapUtil::AssessTarget( CBuilding *pBldg, int iKindOf)
//
void CAITaskMgr::FindAvailablePatrol( CAIUnit *pPatrolBldg )
{
	int iLowRating = 0;
	EnterCriticalSection (&cs);
	CBuilding *pBuilding = 
		theBuildingMap.GetBldg( pPatrolBldg->GetID() );
	if( pBuilding != NULL )
	{
		iLowRating = 
			m_pGoalMgr->m_pMap->m_pMapUtil->AssessTarget(pBuilding,0);
	}
	LeaveCriticalSection (&cs);

	if( !iLowRating )
		return;


#if THREADS_ENABLED
	myYieldThread();
#endif

	int iRating;
	DWORD dwSelected = 0;

	CAIUnit *pBldg = NULL;
   	POSITION posB = m_pGoalMgr->m_plUnits->GetHeadPosition();
   	while( posB != NULL )
   	{   
       	pBldg = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posB );
       	if( pBldg != NULL )
       	{
       		ASSERT_VALID( pBldg );

			if( pBldg->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a building
			if( pBldg->GetType() == CUnit::building )
			{
				// skip these damn, worthless buildings
				if( pBldg->GetTypeUnit() >= CStructureData::apartment_1_1 &&
					pBldg->GetTypeUnit() <= CStructureData::office_3_2 )
					continue;

				// get rating of this building
				iRating = 0;
				EnterCriticalSection (&cs);
				pBuilding = 
					theBuildingMap.GetBldg( pBldg->GetID() );
				if( pBuilding != NULL )
				{
					iRating = 
						m_pGoalMgr->m_pMap->m_pMapUtil->AssessTarget(pBuilding,0);
				}
				LeaveCriticalSection (&cs);

				if( !iRating )
					continue;
				if( iRating >= iLowRating )
					continue;
				if( !IsPatrolled(pBldg) )
					continue;

#if THREADS_ENABLED
	myYieldThread();
#endif
				// only buildings with lower ratings get here
				iLowRating = iRating;
				dwSelected = pBldg->GetID();
			}
		}
	}

	// no lower rated building found without a patrol
	if( !dwSelected )
		return;

#if THREADS_ENABLED
	myYieldThread();
#endif

	// now select the first patroller for the dwSelected building
   	posB = m_pGoalMgr->m_plUnits->GetHeadPosition();
   	while( posB != NULL )
   	{   
       	pBldg = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posB );
       	if( pBldg != NULL )
       	{
       		ASSERT_VALID( pBldg );

			if( pBldg->GetOwner() != m_iPlayer )
				continue;
			// only vehilces
			if( pBldg->GetType() != CUnit::vehicle )
				continue;
			// that are patrolling
			if( pBldg->GetTask() != IDT_PATROL )
				continue;
			// skip units onboard other units
			if( pGameData->IsLoaded(pBldg->GetID()) )
				continue;

			// is this vehicle patroling the passed building
			if( pBldg->GetParamDW(CAI_PATROL) == dwSelected )
			{
				// can this vehicle patrol the passed building
				if( !CanPatrol(pBldg, pPatrolBldg) )
					continue;

				// if so, reassign the patroler and leave
				pBldg->SetParamDW(CAI_PATROL,
					pPatrolBldg->GetID());

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::BalancePatrols() player %d unit %ld now patroling %ld \n", 
pBldg->GetOwner(), pBldg->GetID(), pPatrolBldg->GetID() );
#endif
				return;
			}
		}
	}
}

//
// used mainly to ensure that ships only patrol buildings on the coast
// and are not assigned to land locked buildings
//
BOOL CAITaskMgr::CanPatrol( CAIUnit *pPatrolUnit, CAIUnit *pPatrolBldg )
{
	// get the type of unit that would patrol
	CTransportData const *pVehData = 
		theTransports.GetData(pPatrolUnit->GetTypeUnit());
	if( pVehData == NULL )
		return FALSE;

	// if it is a ship, then it can only patrol seaports and shipyards
	if( pVehData->IsBoat() )
	{
		if( pPatrolBldg->GetTypeUnit() != CStructureData::seaport &&
			pPatrolBldg->GetTypeUnit() != CStructureData::shipyard_1 &&
			pPatrolBldg->GetTypeUnit() != CStructureData::shipyard_3 )
			return FALSE;
	}
	return TRUE;
}
//
// this will balance out patrol units assigned to buildings
// such that when a building is unpatroled, and there are
// extra patrol units, that the building with the most units
// patroling will give up one for the new building
//
void CAITaskMgr::BalancePatrols( CAIMsg *pMsg )
{
	if( pMsg->m_dwID == (DWORD)0 )
		return;

	// skip these damn, worthless buildings
	if( pMsg->m_iMsg == CNetCmd::bldg_new &&
		pMsg->m_idata1 >= CStructureData::apartment_1_1 &&
		pMsg->m_idata1 <= CStructureData::office_3_2 )
		return;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::BalancePatrols() player %d new building %ld is a %d building \n", 
m_iPlayer, pMsg->m_dwID, pMsg->m_idata1 );
#endif

	BalancePatrols();

#if 0
	// count the vehicles on patrol and the buildings
	wPatrols = 0;
	wBuildings = 0;

	if( wPatrols || wBuildings )
		return;

#if THREADS_ENABLED
			// BUGBUG this function must yield
	myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	CAIUnit *pBldg = NULL;
   	POSITION posB = m_pGoalMgr->m_plUnits->GetHeadPosition();
   	while( posB != NULL )
   	{   
       	pBldg = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posB );
       	if( pBldg != NULL )
       	{
       		ASSERT_VALID( pBldg );

			if( pBldg->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a building
			if( pBldg->GetType() == CUnit::building )
			{
				// skip these damn, worthless buildings
				if( pBldg->GetTypeUnit() >= CStructureData::apartment_1_1 &&
					pBldg->GetTypeUnit() <= CStructureData::office_3_2 )
					continue;

				wBuildings++;
			}
			else if( pBldg->GetType() == CUnit::vehicle )
			{
				if( pBldg->GetTask() == IDT_PATROL )
				{					
					// skip units onboard other units
					if( pGameData->IsLoaded(pBldg->GetID()) )
						continue;

					wPatrols++;
				}
			}
		}
	}

	CAIUnit *pVeh = NULL;
	// if the vehicles < buildings  then find the first patroling
	// unit that is patroling the rocket and switch it to patrol
	// this building, otherwise if none are patroling the rocket
	// then just return
	if( wPatrols < wBuildings )
	{
   		POSITION posU = m_pGoalMgr->m_plUnits->GetHeadPosition();
   		while( posU != NULL )
   		{   
       		pVeh = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posU );
       		if( pVeh != NULL )
       		{
       			ASSERT_VALID( pVeh );
				if( pVeh->GetOwner() != m_iPlayer )
					continue;

				// determine if this is a vehicle
				if( pVeh->GetType() != CUnit::vehicle )
					continue;
				if( pVeh->GetTask() != IDT_PATROL )
					continue;

				// skip units onboard other units
				if( pGameData->IsLoaded(pVeh->GetID()) )
					continue;

				// is this vehicle patroling the rocket
				if( pVeh->GetParamDW(CAI_PATROL) == 
					m_pGoalMgr->m_dwRocket )
				{
					pVeh->SetParamDW(CAI_PATROL,pMsg->m_dwID);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::BalancePatrols() player %d unit %ld now patroling %ld \n", 
m_iPlayer, pVeh->GetID(), pMsg->m_dwID );
#endif
					return;
				}
			}
		}
		return;
	}

#if THREADS_ENABLED
			// BUGBUG this function must yield
	myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	// if still here, find the building with the most
	// patrol units assigned
	wPatrols = 0;
	CAIUnit *pBldgPicked = NULL;
	
   	posB = m_pGoalMgr->m_plUnits->GetHeadPosition();
   	while( posB != NULL )
   	{   
       	pBldg = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posB );
       	if( pBldg != NULL )
       	{
       		ASSERT_VALID( pBldg );

			if( pBldg->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a building
			if( pBldg->GetType() != CUnit::building )
				continue;

			// skip these damn, worthless buildings
			if( pBldg->GetTypeUnit() >= CStructureData::apartment_1_1 &&
				pBldg->GetTypeUnit() <= CStructureData::office_3_2 )
				continue;

			wBuildings = 0;
   			POSITION posU = m_pGoalMgr->m_plUnits->GetHeadPosition();
   			while( posU != NULL )
   			{   
       			pVeh = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posU );
       			if( pVeh != NULL )
       			{
       				ASSERT_VALID( pVeh );

					if( pVeh->GetOwner() != m_iPlayer )
						continue;

					// determine if this is a vehicle
					if( pVeh->GetType() != CUnit::vehicle )
						continue;
					if( pVeh->GetTask() != IDT_PATROL )
						continue;

					// skip units onboard other units
					if( pGameData->IsLoaded(pVeh->GetID()) )
						continue;

					// only vehicles with this task remain
					if( pVeh->GetParamDW(CAI_PATROL) == pBldg->GetID() )
						wBuildings++;
				}
			}
			if( wPatrols < wBuildings )
			{
				wPatrols = wBuildings;
				pBldgPicked = pBldg;
			}

#if THREADS_ENABLED
					// BUGBUG this function must yield
			myYieldThread();
					//if( myYieldThread() == TM_QUIT )
					//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

		}
	}
	if( pBldgPicked == NULL )
		return;

	// if still here then find the first patrol unit assigned
	// to pBldgPicked->GetID() and change it to pMsg->m_dwID
   	posB = m_pGoalMgr->m_plUnits->GetHeadPosition();
   	while( posB != NULL )
   	{   
       	pVeh = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posB );
       	if( pVeh != NULL )
       	{
       		ASSERT_VALID( pVeh );

			if( pVeh->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a vehicle
			if( pVeh->GetType() != CUnit::vehicle )
				continue;
			if( pVeh->GetTask() != IDT_PATROL )
				continue;

			if( pGameData->IsLoaded(pVeh->GetID()) )
				continue;

			// only vehicles with this task remain
			if( pVeh->GetParamDW(CAI_PATROL) == pBldgPicked->GetID() )
			{
				pVeh->SetParamDW(CAI_PATROL,pMsg->m_dwID);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::BalancePatrols() player %d unit %ld now patroling %ld \n", 
m_iPlayer, pVeh->GetID(), pMsg->m_dwID );
#endif
				return;
			}
		}
	}

#endif
}

void CAITaskMgr::CheckFactorys( void )
{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::CheckFactorys() executing for player %d ", m_iPlayer );
#endif

	if( m_pGoalMgr->m_plUnits != NULL )
	{
#if THREADS_ENABLED
	myYieldThread();
#endif
        POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIUnit *pUnit = 
            	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
            if( pUnit != NULL )
            {
            	ASSERT_VALID( pUnit );

				if( pUnit->GetOwner() != m_iPlayer )
					continue;

				if( pUnit->GetType() != CUnit::building )
					continue;

				BOOL bTestThis = FALSE;
				switch( pUnit->GetTypeUnit() )
				{
					case CStructureData::barracks_2:
					case CStructureData::heavy:
					case CStructureData::light_0:
					case CStructureData::light_1:
					case CStructureData::light_2:
					case CStructureData::shipyard_1:
					case CStructureData::shipyard_3:
						bTestThis = TRUE;
					default:
						break;
				}
				if( !bTestThis )
					continue;

				EnterCriticalSection (&cs);
				CBuilding *pBldg = 
					pGameData->GetBuildingData( 
					pUnit->GetOwner(), pUnit->GetID() );
				if( pBldg == NULL )
				{
					LeaveCriticalSection (&cs);
					continue;
				}
				BOOL bPaused = pBldg->IsPaused();
				LeaveCriticalSection (&cs);

				if( !bPaused )
					continue;

				// make sure unit is doing nothing
				ClearTaskUnit( pUnit );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAITaskMgr::CheckFactorys() trying to restart %d/%ld ", 
	pUnit->GetOwner(), pUnit->GetID() );
#endif
				// now assign a production task to it and
				// send the message to start producing a vehicle
				if( AssignProductionTask( pUnit ) )
					GenerateTaskOrder( pUnit );
			}
		}
	}
}

void CAITaskMgr::UnitControl( CAIMsg *pMsg )
{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::UnitControl() executing for player %d ", m_iPlayer );
#endif

	CMsgUnitControl msgUC;
	CAIUnit *pUnit = NULL;

	// start up production for selected units
	if( pMsg->m_iMsg == CNetCmd::bldg_stat &&
		pMsg->m_uFlags == CMsgBldgStat::built &&
		pMsg->m_dwID != NULL )
	{

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::UnitControl() bldg_stat player %d unit %ld type=%d ", 
		pMsg->m_idata3, pMsg->m_dwID, pMsg->m_idata1 );
#endif
		// consider what type of building
		switch( pMsg->m_idata1 )
		{
			// BUGBUG the top section may not be needed as these
			// types of producers seem to start on their own
			//
			// these buildings only produce one type of output
			// so just tell them to start up
			case CStructureData::lumber:
			case CStructureData::oil_well:
			case CStructureData::refinery:
			case CStructureData::coal:
			case CStructureData::iron:
			case CStructureData::copper:
			case CStructureData::power_1:
			case CStructureData::power_2:
			case CStructureData::power_3:
			case CStructureData::research:
			case CStructureData::smelter:
				/*
// start/stop/abandon/destroy unit
class CMsgUnitControl : public CNetCmd
{
public:
		CMsgUnitControl () : CNetCmd (unit_control) {}
		enum { stop, resume, abandon, num_cmds };
		CMsgUnitControl (CUnit const *pUnit, int iCmd);

		DWORD				m_dwID;				// ID of unit
		char				m_cCmd;				// command

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
				*/
				
				msgUC.m_dwID = pMsg->m_dwID;
				msgUC.m_cCmd = (char)CMsgUnitControl::resume;
				//theGame.PostToServer( (CNetCmd *)&msgUC, sizeof(CMsgUnitControl) );
				break;
			// the other types of production buildings
			// can produce multiple types of output so
			// one must be selected and a different message
			// sent to start up production
			case CStructureData::barracks_2:
			//case CStructureData::barracks_3:
			case CStructureData::heavy:
			case CStructureData::light_0:
			case CStructureData::light_1:
			case CStructureData::light_2:
			case CStructureData::shipyard_1:
			case CStructureData::shipyard_3:
				pUnit = m_pGoalMgr->m_plUnits->GetUnit(pMsg->m_dwID);
				if( pUnit != NULL )
				{
					// make sure unit is doing nothing
					ClearTaskUnit( pUnit );

					// now assign a production task to it and
					// send the message to start producing a vehicle
					if( AssignProductionTask( pUnit ) )
						GenerateTaskOrder( pUnit );
				}
				break;
			// then those buildings which don't produce anything, but mean well
			case CStructureData::command_center:
			case CStructureData::fort_1:
			case CStructureData::fort_2:
			case CStructureData::fort_3:
			case CStructureData::embassy:
			case CStructureData::repair:
				break;
			default:
				break;
		}
	}
	else if( pMsg->m_iMsg == CNetCmd::veh_new )
	{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::UnitControl() veh_new player %d unit %ld a %d at factory %ld ", 
		pMsg->m_idata3, pMsg->m_dwID, pMsg->m_idata1, pMsg->m_dwID2 );
#endif
		// if message is a veh_new, and there are buildings
		// that are assigned to this task, then this building
		// should consider having production stopped
		if( pMsg->m_dwID2 )
		{
			// need to be able to determine the building that just
			// produced this vehicle
			CAIUnit *pBldg = m_pGoalMgr->m_plUnits->GetUnit( pMsg->m_dwID2 );
			if( pBldg != NULL )
			{
				// make sure unit is doing nothing
				ClearTaskUnit( pBldg );
				// then check the production task this building has assigned
				// to it for a priority of ZERO and if so, stop the building
				// from producing any more until later
				if( AssignProductionTask( pBldg ) )
					GenerateTaskOrder( pBldg );
			}
		}
	}
}

//
// process the goals of this manager, check the tasks
// of each goal for existing in the task list of this
// manager, and if the task does not exist, remove it
// and if it does exist, mark it; then remove all tasks
// that are not marked
//
void CAITaskMgr::UpdateTasks( CAIMsg *pMsg )
{
	ASSERT_VALID( m_pGoalMgr->m_plTasks );

	// BUGBUG need a way to update tasks based on the
	// message data that just came in, so for now, if
	// the list has data and no message then return
	if( pMsg == NULL && m_pGoalMgr->m_plTasks->GetCount() )
		return;

#if THREADS_ENABLED
		// BUGBUG this function must yield
	myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	// go thru the goalmgr task list and
	// remove any task that is not have
	// its goal in the local goal list
	// and update any CAIUnit assigned
	// to that task
	POSITION pos = m_pGoalMgr->m_plTasks->GetHeadPosition();
    while( pos != NULL )
    {   
        CAITask *pTask = (CAITask *)m_pGoalMgr->m_plTasks->GetNext( pos );
        if( pTask != NULL )
        {
      		ASSERT_VALID( pTask );
			CAIGoal *pGoal = 
				m_pGoalMgr->GetGoal( pTask->GetGoalID() );

			// if goal not found, then task needs to be removed
			if( pGoal == NULL )
			{
				// before removing task, all CAIUnits with this
				// task/goal must have them cleared
    			POSITION posU = m_pGoalMgr->m_plUnits->GetHeadPosition();
    			while( posU != NULL )
    			{   
        			CAIUnit *pUnit = 
        				(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posU );
        			if( pUnit != NULL )
        			{
        				ASSERT_VALID( pUnit );

#if THREADS_ENABLED
						// BUGBUG this function must yield
						myYieldThread();
						//if( myYieldThread() == TM_QUIT )
						//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
            			if( pUnit->GetOwner() == m_iPlayer )
                		{
							if( pUnit->GetTask() == pTask->GetID() &&
								pUnit->GetGoal() == pTask->GetGoalID() )
							{
								pUnit->SetTask( FALSE );
								pUnit->SetGoal( FALSE );
								// there could be more than one
								// unit assigned to this task
							}
						}
        			}

    			}	// end of for each CAIUnit

				// now remove the task from the local list
				m_pGoalMgr->m_plTasks->RemoveTask( pTask->GetID(), 
					pTask->GetGoalID() );
			}
        }	// end of task is not NULL

    }	// end of for each task in local list

#if THREADS_ENABLED
		// BUGBUG this function must yield
	myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	// interate thru the local goal mgr's goal list
	// and add any task of the goal not found in
	// the local task list
	CAIGoal *pGoal = m_pGoalMgr->FirstGoal();
	while( pGoal != NULL )
	{
		int t=0;
		WORD wTaskID = pGoal->GetTaskAt(t);
		while( wTaskID )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			// check for task in local list
			CAITask *pTask = m_pGoalMgr->m_plTasks->GetTask( 
				wTaskID, pGoal->GetID() );

			// if task is not in local list
			if( pTask == NULL )
			{
				// then get it from the standard list
				pTask = plTaskList->GetTask( 
					wTaskID, pGoal->GetID() );
				if( pTask != NULL )
				{
					// and add it to the local list
					CAITask *pNewTask = pTask->CopyTask();
					ASSERT_VALID( pNewTask );
					m_pGoalMgr->m_plTasks->AddTail( (CObject *)pNewTask );
				}
			}
			// get next task of goal and continue
			wTaskID = pGoal->GetTaskAt( ++t );
		}
		// get next goal and continue
		pGoal = m_pGoalMgr->NextGoal();
	}
}

//
// unassign task so that it is available for assignment again
//
void CAITaskMgr::UnAssignTask( WORD wTask, WORD wGoal )
{
	CAITask *pTask = m_pGoalMgr->m_plTasks->GetTask(wTask,wGoal);
	if( pTask != NULL )
	{
		if( pTask->GetTaskParam(ORDER_TYPE) == CONSTRUCTION_ORDER )
		{
			pTask->SetTaskParam(BUILD_AT_X, 0 );
			pTask->SetTaskParam(BUILD_AT_Y, 0 );
			pTask->SetTaskParam(BUILDING_EXIT, 0 );
				pTask->SetStatus( UNASSIGNED_TASK );

			// non-building construction is continuous
			//if( pTask->GetOrderID() != CNetCmd::build_bldg )
			//
		}
		// let goal manager reset priority
		pTask->SetPriority( (BYTE)0 );
		

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::UnAssignTask() player %d for goal=%d task=%d priority=%d \n",
		m_iPlayer, wGoal, wTask, pTask->GetPriority() );
#endif
	}
}

//
// unassign task and unit
//
// err_build_bldg m_iWhy (pMsg->m_idata2) values are CGameMap::
// enum { ok, bldg_next, water_next, bldg_or_river, veh_in_way, 
// water, no_water, no_land_exit, no_water_exit, steep };
//
void CAITaskMgr::ConstructionError( CAIMsg *pMsg )
{
	// get the crane trying to construct the building
	CAIUnit *pUnit = 
		m_pGoalMgr->m_plUnits->GetUnit(pMsg->m_dwID2);
	if( pUnit == NULL )
		return;
	// get the task of that crane
	CAITask *pTask = m_pGoalMgr->m_plTasks->GetTask( 
		pUnit->GetTask(), pUnit->GetGoal() );
	if( pTask == NULL )
		return;

	// some errors may be recoverable, so try a number of times
	// to still build at this site if error is due to a vehicle
	// in the way (it may move) and num-of-tries is valid
	if( pMsg->m_idata2 == CGameMap::veh_in_way &&
		pUnit->GetParam(CAI_EFFECTIVE) < CGameMap::veh_in_way )
	{
		//
		// get id of building to build from the task and the
		// orientation of the exit hex for that building type
		int iBldg = (int)pTask->GetTaskParam(BUILDING_ID);
		int iDir = (int)pTask->GetTaskParam(BUILDING_EXIT);
		// building site
		CHexCoord hexSite(0,0);
		hexSite.X(pTask->GetTaskParam(BUILD_AT_X));
		hexSite.Y(pTask->GetTaskParam(BUILD_AT_Y));
		// increment count of times message was sent
		pUnit->SetParam( CAI_EFFECTIVE,
			(pUnit->GetParam(CAI_EFFECTIVE)+1) );
		// send message to try to still build here
		pGameData->BuildAt( hexSite, iBldg, iDir, pUnit );
		return;
	}

	// now unassign both the crane and the task
	ClearTaskUnit( pUnit );
	//UnAssignTask( pTask->GetID(), pTask->GetGoalID() );
	// but let task keep its priority
	pTask->SetTaskParam(BUILD_AT_X, 0 );
	pTask->SetTaskParam(BUILD_AT_Y, 0 );
	pTask->SetTaskParam(BUILDING_EXIT, 0 );
	pTask->SetStatus( UNASSIGNED_TASK );

	// and move the crane off of this site
	CHexCoord hexVeh( pMsg->m_iX, pMsg->m_iY );
	m_pGoalMgr->m_pMap->m_pMapUtil->FindStagingHex( hexVeh,
		1, 1, pUnit->GetTypeUnit(), hexVeh );

	pUnit->SetDestination( hexVeh );

	m_pGoalMgr->m_pMap->PlaceFakeVeh( hexVeh, 
		pUnit->GetTypeUnit() );
}


//
// this will select a production task based on the CStructureData 
// type of production facility that pUnit actually is.
//
// pUnit->GetParam(PRODUCTION_ID) contains the CStructureData type
// the goal manager should have set priorities in the production tasks
// by now (not implemented yet) which will in turn help select the task
//
BOOL CAITaskMgr::AssignProductionTask( CAIUnit *pUnit )
{
	CAITask *pTask = NULL;

	// the building may already be assigned to produce a vehicle
	// based on a currently assigned production task.  If the task
	// is ZERO priority and the building is 'stopped' then that
	// means that the goal manager wants production stopped.
	if( pUnit->GetTask() || pUnit->GetGoal() )
		pTask = m_pGoalMgr->m_plTasks->GetTask( 
			pUnit->GetTask(), pUnit->GetGoal() );

	EnterCriticalSection (&cs);
	CBuilding *pBldg = 
		pGameData->GetBuildingData( 
		pUnit->GetOwner(), pUnit->GetID() );
	if( pBldg == NULL )
	{
		LeaveCriticalSection (&cs);
		return FALSE;
	}
	BOOL bPaused = pBldg->IsPaused();
	LeaveCriticalSection (&cs);

	// a valid task is currently assigned
	if( pTask != NULL )
	{
		CMsgUnitControl msgUC;
		// if the task is non-zero priority and the building is 'stopped'
		// then that means the goal manager want production re-started
		if( pTask->GetPriority() > 0 && bPaused )
		{
			// re-start production
			msgUC.m_dwID = pUnit->GetID();
			msgUC.m_cCmd = (char)CMsgUnitControl::resume;
			theGame.PostToServer( (CNetCmd *)&msgUC, sizeof(CMsgUnitControl) );
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::AssignProductionTask() resuming task=%d, goal=%d, for unit %ld player %d ", 
	pTask->GetID(), pTask->GetGoalID(), pUnit->GetID(), pUnit->GetOwner() );
#endif
		}
		// if the task has ZERO priority and the building is running
		// then stop this vehicle's production
		else if( !pTask->GetPriority() && !bPaused )
		{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::AssignProductionTask() stopping task=%d, goal=%d, for unit %ld player %d ", 
	pTask->GetID(), pTask->GetGoalID(), pUnit->GetID(), pUnit->GetOwner() );
#endif
			// stop production
			msgUC.m_dwID = pUnit->GetID();
			msgUC.m_cCmd = (char)CMsgUnitControl::stop;
			theGame.PostToServer( (CNetCmd *)&msgUC, sizeof(CMsgUnitControl) );

			// and unassign the unit from this task by picking another
			goto PickATask;
		}
		// else if the task has a priority and the building is not paused
		// then just returning will continue production, apparently not
		// so resend start productin message

		return TRUE;
	}

	PickATask:
	// get a production task that this building can handle
	pTask = m_pGoalMgr->GetProductionTask(pUnit);
	//	m_pGoalMgr->m_plTasks->GetProductionTask(pUnit->GetTypeUnit());
	if( pTask != NULL )
	{
		pUnit->SetTask( pTask->GetID() );
		pUnit->SetGoal( pTask->GetGoalID() );

		// NOTE the task remains UNASSIGNED so that the same task 
		// to produce vehicles can be used by several buildings
		
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::Assign production task=%d, goal=%d, for unit %ld player %d ", 
		pTask->GetID(), pTask->GetGoalID(), pUnit->GetID(), m_iPlayer );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"to produce a %d type vehicle at a %d type building ",
		pTask->GetTaskParam(PRODUCTION_ITEM), pTask->GetTaskParam(PRODUCTION_ID1) );
#endif
		return TRUE;
	}
	else
	{
		// factory has no task and the factory is not paused, so pause it
		if( !bPaused )
		{
			CMsgUnitControl msgUC;
			// stop production
			msgUC.m_dwID = pUnit->GetID();
			msgUC.m_cCmd = (char)CMsgUnitControl::stop;
			theGame.PostToServer( (CNetCmd *)&msgUC, sizeof(CMsgUnitControl) );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::AssignProductionTask() stopping factory unit %ld player %d ", 
		pUnit->GetID(), pUnit->GetOwner() );
#endif
		}
	}
	return FALSE;
}

//
// find a task, based on its priority and this type
// of unit, to assign to this unit
//
void CAITaskMgr::AssignTask( CAIUnit *pUnit )
{
#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
	//if( myYieldThread() == TM_QUIT )
	//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
	// make sure all newly assigned units start with params clear
	ClearTaskUnit( pUnit );

	// determine the type of this unit
	WORD wType = 0xFFFF;
	if( pUnit->GetType() == CUnit::building )
	{
		// get access to CBuilding copied data for building
		CAICopy *pCopyCBuilding = 
			pUnit->GetCopyData( CAICopy::CBuilding );
		if( pCopyCBuilding != NULL )
			wType = (WORD)pCopyCBuilding->m_aiDataOut[CAI_TYPEBUILD];
	}
	else if( pUnit->GetType() == CUnit::vehicle )
	{
		// get access to CVehicle copied data for vehicle
		CAICopy *pCopyCVehicle = 
			pUnit->GetCopyData( CAICopy::CVehicle );
		if( pCopyCVehicle != NULL )
			wType = (WORD)pCopyCVehicle->m_aiDataOut[CAI_TYPEVEHICLE];
	}

	// consider type of unit
	if( pUnit->GetType() == CUnit::vehicle )
	{
		switch( (int)wType )
		{
			// if this unit is a construction truck, find the
			// highest priorty build task that is not assigned
			// and assign it
			case CTransportData::construction:
				AssignConstruction(pUnit);
				break;
			case CTransportData::light_scout:
			//case CTransportData::med_scout:
				AssignScout(pUnit);
				break;
			case CTransportData::med_truck:
			case CTransportData::heavy_truck:
			case CTransportData::light_cargo:           
				AssignTransport(pUnit);
				break;
			case CTransportData::med_scout:
			case CTransportData::heavy_scout:
			case CTransportData::infantry_carrier:
			//case CTransportData::light_tank:
			//case CTransportData::med_tank:
			case CTransportData::heavy_tank:
			//case CTransportData::light_art:
			case CTransportData::med_art:
			case CTransportData::heavy_art:
			case CTransportData::infantry:
			//case CTransportData::rangers:
				AssignCombat( pUnit );
				break;
			case CTransportData::rangers:
			case CTransportData::light_tank:
			case CTransportData::med_tank:
			case CTransportData::light_art:
			case CTransportData::gun_boat:
			case CTransportData::destroyer:
			case CTransportData::cruiser:
			case CTransportData::landing_craft:
				AssignNavy( pUnit );
			default:
				break;
		}
	}
	// this should happen in AssignUnit()
	//GenerateTaskOrder( pUnit );
}

void CAITaskMgr::AssignNavy( CAIUnit *pUnit )
{
	// get highest priority navy task
	CAITask *pTask = 
		m_pGoalMgr->m_plTasks->GetNavyTask( pUnit->GetTypeUnit() );
	if( pTask != NULL )
	{
		ClearTaskUnit( pUnit );
		pUnit->SetTask( pTask->GetID() );
		pUnit->SetGoal( pTask->GetGoalID() );
		

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AssignNavy task=%d, goal=%d, for unit %ld player %d ", 
pTask->GetID(), pTask->GetGoalID(), pUnit->GetID(), m_iPlayer );
#endif
		return;
	}
	// if no navy tasks, then consider IDT_PATROL/IDT_ESCORT
	// except for vehicles which do both combat and navy tasks
	if( pUnit->GetTypeUnit() == CTransportData::rangers ||
		pUnit->GetTypeUnit() == CTransportData::light_tank ||
		pUnit->GetTypeUnit() == CTransportData::med_tank ||
		pUnit->GetTypeUnit() == CTransportData::light_art )
	{
		AssignCombat(pUnit);
		return;
	}
	AssignPatrol(pUnit);
}

//
// land combat units only
//
void CAITaskMgr::AssignCombat( CAIUnit *pUnit )
{
	// get highest priority combat task
	CAITask *pTask = 
		m_pGoalMgr->m_plTasks->GetCombatTask( pUnit->GetTypeUnit() );
	if( pTask != NULL )
	{
		ClearTaskUnit( pUnit );

		// if this is an IDT_PREPAREWAR task, consider it may
		// have enough units already assigned to it, so each
		// IDT_PREPAREWAR task needs to keep track of assigned count
		if( pTask->GetID() == IDT_PREPAREWAR )
		{
			// if this staging task has enough assigned, go patrol
			if( IsStagingCompete(pTask,pUnit->GetTypeUnit()) )
			{
				// but consider there may be another IDT_PREPAREWAR
				// task that could be assigned
				pTask = 
					m_pGoalMgr->m_plTasks->GetCombatTask( 
					pUnit->GetTypeUnit(), pTask );

				if( pTask == NULL )
				{
					AssignPatrol(pUnit);
					goto TaskWasAssigned;
				}

				if( pTask->GetID() == IDT_PREPAREWAR )
				{
					if( IsStagingCompete(pTask,pUnit->GetTypeUnit()) )
					{
						AssignPatrol(pUnit);
						goto TaskWasAssigned;
					}
				}
			}
		}
		// assign it
		pUnit->SetTask( pTask->GetID() );
		pUnit->SetGoal( pTask->GetGoalID() );

TaskWasAssigned:

#ifdef _LOGOUT
if( pTask != NULL )
{
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::AssignCombat task=%d, goal=%d, for player %d unit %ld type %d ", 
pTask->GetID(), pTask->GetGoalID(), 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetTypeUnit() );
}
#endif
		return;
	}
	// if no combat tasks, then consider IDT_PATROL/IDT_ESCORT
	AssignPatrol(pUnit);
}

//
// both trucks and cargo ships fall into this
//
void CAITaskMgr::AssignTransport( CAIUnit *pUnit )
{
	CAITask *pTask = 
		m_pGoalMgr->m_plTasks->GetUnassignedTask(IDT_SETTRANSPORT);
	if( pTask != NULL )
	{
		pUnit->SetTask( pTask->GetID() );
		pUnit->SetGoal( pTask->GetGoalID() );
		//pTask->SetStatus( INPROCESS_TASK );
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AssignTransport task=%d, goal=%d, for unit %ld player %d ", 
pTask->GetID(), pTask->GetGoalID(), pUnit->GetID(), m_iPlayer );
#endif
	}
}

void CAITaskMgr::AssignPatrol( CAIUnit *pUnit )
{
	// get pointer to vehicle type data
	CTransportData const *pVehData = 
		theTransports.GetData(pUnit->GetTypeUnit());
	if( pVehData == NULL )
		return;

	
	// get highest priority patrol task, based on target type of unit
	// because CUnitData::naval defines sea vs land and they have
	// different patrol types
	CAITask *pTask = 
		m_pGoalMgr->GetPatrolTask(pVehData->GetTargetType());
	
	// if task found then assign it and 
	// order the vehicle to perform it
	if( pTask != NULL )
	{
		ClearTaskUnit( pUnit );

		// assign this unit to patrol task
		pUnit->SetTask( pTask->GetID() );
		pUnit->SetGoal( pTask->GetGoalID() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AssignPatrol %d, goal %d, for unit %ld player %d ", 
pTask->GetID(), pTask->GetGoalID(), pUnit->GetID(), m_iPlayer );
#endif
		return;
	}
	// unit has maybe passed through AssignScout()
	// unit has passed through AssignCombat()
	// unit has passed through AssignPatrol()
	//
	// and not gotten a task, so what to do?
	//
	// force a patrol task
	pUnit->SetGoal( IDG_ESTABLISH );
	pUnit->SetTask( IDT_PATROL );
	return;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::could not assign task to unit %ld player %d ", 
pUnit->GetID(), m_iPlayer );
#endif
}


void CAITaskMgr::AssignScout( CAIUnit *pUnit )
{
	// need to find an unassigned task that requires 
	// doing some scouting
	CAITask *pTask = m_pGoalMgr->m_plTasks->GetScoutTask();
	
	// if task found then assign it and 
	// order the vehicle to perform it
	if( pTask != NULL )
	{
		// allow difficulty to influence assignment of IDT_FIND* tasks
		// only one scout at a time does either of these
		if( pTask->GetID() == IDT_FINDOPFORCITY ||
			pTask->GetID() == IDT_FINDOPFORS )
		{
			BOOL bSwitchTask = TRUE;
			if( pGameData->m_iSmart )
			{
				// more difficulty increases chance of keeping IDT_FIND* task
				if( pGameData->GetRandom(NUM_DIFFICUTY_LEVELS) >
					pGameData->m_iSmart )
					bSwitchTask = FALSE;
			}
			if( bSwitchTask )
				pTask = m_pGoalMgr->m_plTasks->FindTask( IDT_SCOUT );
		}

		// only one scout at a time does either of these
		if( pTask->GetID() == IDT_FINDOPFORCITY ||
			pTask->GetID() == IDT_FINDOPFORS )
		{
			ClearTaskUnit( pUnit );

			pUnit->SetTask( pTask->GetID() );
			pUnit->SetGoal( pTask->GetGoalID() );
			
			WORD wCnt = 
				pTask->GetTaskParam(FROM_MATERIAL_ID);

			if( pGameData->m_iSmart )
			{
				if( (int)wCnt > pGameData->m_iSmart )
					pTask->SetStatus( INPROCESS_TASK );
			}
			else
				pTask->SetStatus( INPROCESS_TASK );

			pTask->SetTaskParam(FROM_MATERIAL_ID,(wCnt+1));

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AssignScout %d, goal %d, for unit %ld player %d ", 
pTask->GetID(), pTask->GetGoalID(), pUnit->GetID(), m_iPlayer );
#endif
			return;
		}
		else if( pTask->GetID() == IDT_SCOUT )
		{
			WORD wCnt = 
				pTask->GetTaskParam(TO_MATERIAL_ID);
			if( (int)wCnt < (pGameData->m_iSmart * 2) )
			{
				ClearTaskUnit( pUnit );

				pUnit->SetTask( pTask->GetID() );
				pUnit->SetGoal( pTask->GetGoalID() );
				pTask->SetTaskParam(TO_MATERIAL_ID,(wCnt+1));

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AssignScout %d, goal %d, for unit %ld player %d ", 
pTask->GetID(), pTask->GetGoalID(), pUnit->GetID(), pUnit->GetOwner() );
#endif
				return;
			}
		}
	}

	// if no IDT_SCOUT* tasks then consider combat tasks for the scout
	ClearTaskUnit( pUnit );
	AssignCombat(pUnit);

	// if a combat task was assigned we are done
	if( pUnit->GetTask() )
		return;

	// default assign this IDT_SCOUT task
	pTask = m_pGoalMgr->m_plTasks->FindTask( IDT_SCOUT );
	if( pTask != NULL )
	{
		pUnit->SetTask( pTask->GetID() );
		pUnit->SetGoal( pTask->GetGoalID() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AssignScout %d, goal %d, for unit %ld player %d ", 
pTask->GetID(), pTask->GetGoalID(), pUnit->GetID(), pUnit->GetOwner() );
#endif
	}
}

void CAITaskMgr::AssignConstruction( CAIUnit *pUnit )
{
	int iHang = 0;
	// need to find an unassigned task that requires 
	// building something
GetTask:
	CAITask *pTask = m_pGoalMgr->m_plTasks->GetConstructionTask();

	// add separate test to confirm that player can build building
	if( pTask != NULL )
	{
		// the task obtained is to build roads by default
		if( pTask->GetID() == IDT_CONSTRUCT )
			goto DefaultRoad;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::AssignConstruction() unit %ld player %d did not find task", 
		pUnit->GetID(), pUnit->GetOwner() );
#endif

		int iBldg = (int)pTask->GetTaskParam(BUILDING_ID);

		// add test to confirm this can be built by this player
		BOOL bCanBuild = FALSE;
		// by getting a pointer to that type of building
		CStructureData const *pBldgData = 
			pGameData->GetStructureData( iBldg );
		if( !iBldg || pBldgData == NULL )
		{
			pTask->SetStatus( UNASSIGNED_TASK );
			return;
		}

		// then a pointer to this player
		EnterCriticalSection (&cs);
		CPlayer *pPlayer = 
		pGameData->GetPlayerData( m_iPlayer );
		if( pPlayer == NULL )
		{
			LeaveCriticalSection (&cs);
			pTask->SetStatus( UNASSIGNED_TASK );
			return;
		}
		// get the opinion of the game based on this player
		bCanBuild = pBldgData->PlyrIsDiscovered(pPlayer);
		LeaveCriticalSection (&cs);

		// this assessment determined the building could not be built
		if( !bCanBuild )
		{

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::AssignConstruction() unit %ld player %d building %d not discovered ", 
		pUnit->GetID(), pUnit->GetOwner(), iBldg );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::AssignConstruction() for goal %d task %d priority %d ", 
		pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
#endif
			// flag the task, tell goalmgr to re-priortize and try again
			pTask->SetStatus( UNASSIGNED_TASK );
			m_pGoalMgr->UpdateConstructionTasks(NULL);

			// since this uses a goto, prevent a hang with hard code limit
			// and force road building
			if( iHang )
			{
				pTask = m_pGoalMgr->m_plTasks->FindTask( IDT_CONSTRUCT );
				if( pTask == NULL )
					return;
			}
			iHang++;
			goto GetTask;
		}
	}

DefaultRoad:
	// if task found then assign it and 
	// order the vehicle to perform it
	if( pTask != NULL )
	{
		ClearTaskUnit( pUnit );

		pUnit->SetTask( pTask->GetID() );
		pUnit->SetGoal( pTask->GetGoalID() );

		pTask->SetStatus( INPROCESS_TASK );
		pTask->SetTaskParam(BUILD_AT_X, 0 );
		pTask->SetTaskParam(BUILD_AT_Y, 0 );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::AssignConstruction() unit %ld player %d ", 
		pUnit->GetID(), pUnit->GetOwner() );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAITaskMgr::AssignConstruction() assigned to goal %d task %d priority %d ", 
		pTask->GetGoalID(), pTask->GetID(), pTask->GetPriority() );
#endif

	}
	else
	{
		// else if all construction tasks are assigned 
		// no tasks for building a road are available
		// then consider doing a repair
		// on a damaged building
		if( AssignRepair(pUnit) )
			return;
	}
}

//
// go thru units, looking for buildings that are damaged
// and determine if there are buildings to repair
// and if none exist, then do not assign IDT_REPAIR task
// but if at least one does then do assign IDT_REPAIR task.
//
BOOL CAITaskMgr::AssignRepair(CAIUnit *pCrane)
{
	CAITask *pTask = m_pGoalMgr->m_plTasks->FindTask( IDT_REPAIR );
	if( pTask == NULL )
		return FALSE;

	if( m_pGoalMgr->m_plUnits != NULL )
	{
#if THREADS_ENABLED
	myYieldThread();
#endif
        POSITION posB = m_pGoalMgr->m_plUnits->GetHeadPosition();
        while( posB != NULL )
        {   
            CAIUnit *pUnitB = 
            	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posB );
            if( pUnitB != NULL )
            {
            	ASSERT_VALID( pUnitB );

				if( pUnitB->GetOwner() != m_iPlayer )
					continue;
				// only buildings are desired
				if( pUnitB->GetType() != CUnit::building )
					continue;

				BOOL bNeedRepair = FALSE;
				EnterCriticalSection (&cs);
				CBuilding *pBldg = 
					theBuildingMap.GetBldg( pUnitB->GetID() );
				if( pBldg != NULL )
				{
					if( pBldg->GetDamagePer() < DAMAGE_0 )
					{
						bNeedRepair = TRUE;
						// check for delete in progress
						if( pBldg->GetFlags() & CUnit::dying )
							bNeedRepair = FALSE;
					}
				}
				LeaveCriticalSection (&cs);

				// assign the task and let something else pick a building
				// and direct the crane to it
				if( bNeedRepair )
				{
					pCrane->SetGoal( pTask->GetGoalID() );
					pCrane->SetTask( pTask->GetID() );
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

//
// using the task assigned to this unit, consider
// if there is need to issue the task order to unit,
// and if there is, then issue the order
//
void CAITaskMgr::GenerateTaskOrder( CAIUnit *pUnit )
{
	if( pUnit == NULL )
		return;

	ASSERT_VALID( pUnit );

#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
	//if( myYieldThread() == TM_QUIT )
	//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	// no task is assigned to this unit
	if( !pUnit->GetTask() &&
		!pUnit->GetGoal() )
	{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::GenerateTaskOrder() unit %ld player %d has no goal or task ", 
pUnit->GetID(), pUnit->GetOwner() );
#endif
		// so assign one
		AssignTask( pUnit );
		if( !pUnit->GetTask() &&
			!pUnit->GetGoal() )
			return;
		return;
	}

	// find the task of this unit
	CAITask *pTask = NULL;

	pTask = m_pGoalMgr->m_plTasks->GetTask( 
		pUnit->GetTask(), pUnit->GetGoal() );
	if( pTask == NULL )
	{
		pTask = m_pGoalMgr->m_plTasks->FindTask( 
			pUnit->GetTask() );
		if( pTask == NULL )
		{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::GenerateTaskOrder() unit %ld player %d goal %d task %d not found ", 
pUnit->GetID(), pUnit->GetOwner(), pUnit->GetGoal(), pUnit->GetTask() );
#endif
			ClearTaskUnit( pUnit );

			return;
		}
	}

	// determine if this task needs another order
	// to be issued to the unit, if not clear the
	// task/goal of the unit and return
	//
	// identify CNetCmd order for the task of this unit
	//
	// construct parameters based on task
	//
	// issue order
	//
	switch( pTask->GetID() )
	{
		case IDT_MAKECONSTRUCT:
		case IDT_MAKELSCOUT:
		case IDT_MAKEMSCOUT:
		case IDT_MAKEHSCOUT:
		case IDT_MAKEINFANTRY:
		case IDT_MAKERANGERS:
		//case IDT_MAKEMARINES:
		//case IDT_MAKELTRUCK:
		//case IDT_MAKEMTRUCK:
		case IDT_MAKEHTRUCK:
		case IDT_MAKEIFV:
		case IDT_MAKELTANK:
		case IDT_MAKEMTANK:
		case IDT_MAKEHTANK:
		case IDT_MAKELART:
		case IDT_MAKEMART:
		case IDT_MAKEHART:
		case IDT_MAKELCARGOSHIP:
		//case IDT_MAKEMCARGOSHIP:
		case IDT_MAKEGUNSHIP:
		case IDT_MAKEDESTROYER:
		case IDT_MAKECRUISER:
		case IDT_MAKELANDCRAFT:
			// this is a production task so need to 
			// issue a build vehicle order to the 
			// CBuilding unit that has this task
			ProduceVehicle( pUnit, pTask );
			break;
		case IDT_PATROL:
			// a patrol destination needs to be selected
			// based on proximity of our buildings and
			// known opfor units
			PatrolArea( pUnit, pTask );
			break;
		case IDT_ESCORT:
			// this is a special staging task, that is used
			// to coordinate the assembly of a support force
		case IDT_PREPAREWAR:
			// this is serving as a GOTO LOCATION that
			// allows for vehicles (not needed at this time)
			// to be staged or placed in reserve
			StageUnit( pUnit, pTask );
			break;
		case IDT_SEEKINRANGE:
		case IDT_SEEKATSEA:
		case IDT_SEEKINWAR:
			// vehicle needs to consider nearest known OpFor
			// units, select one and be ordered into firing
			// range; or if in range, attack
			SeekOpfor( pUnit, pTask );
			break;
		case IDT_SCOUT:
			// vehicles are assigned a direction from the base
			// and will then seek their own destinations using
			// that direction and the vehicle's spotting range
			// to select the next destination to proceed to
			ExploreMap( pUnit, pTask );
			break;
		case IDT_BUILDFORT1:
		case IDT_BUILDFORT2:
		case IDT_BUILDFORT3:
		case IDT_BUILDEMBASSY:
			// update opfor data in map to consider while
			// selecting sites for these types of buildings
			m_pGoalMgr->SetMapOpFor();
			
		case IDT_BUILDLIGHT0:
		case IDT_BUILDLIGHT1:
		case IDT_BUILDLIGHT2:
		case IDT_BUILDHEAVYFAC:
		case IDT_BUILDBARRACKS2:
		case IDT_BUILDAPTS:
		case IDT_BUILDOFFICE:
		case IDT_BUILDCCC:
		case IDT_BUILDLUMBMILL:
		case IDT_BUILDFARM:
		case IDT_BUILDRD:
		case IDT_BUILDPOWERPT1:
		case IDT_BUILDPOWERPT2:
		case IDT_BUILDPOWERPT3:
		case IDT_BUILDCOALMINE:
		case IDT_BUILDIRONMINE:
		case IDT_BUILDCOPPERMINE:
		case IDT_BUILDSMELTER:
		case IDT_BUILDOILWELL:
		case IDT_BUILDREFINERY:
		case IDT_BUILDSEAPORT:
		case IDT_BUILDSHIPYARD1:
		case IDT_BUILDSHIPYARD3:
		case IDT_BUILDWAREHOUSE:
		case IDT_BUILDREPAIRFAC:
			// a construction task has been assigned to a 
			// construction truck to build the building indicated
			// in this task, so find the appropriate hex on
			// which to build and issue the appropriate
			// order to send the truck there to build it
			// or
			// a construction truck has just reached the
			// adjacent hex building site so issue the
			// appropriate order to build the building now
			ConstructBuilding( pUnit, pTask );
			break;
		case IDT_CONSTRUCT:
			// vehicle needs to be sent to build a road or 
			// bridge; determining which based on the terrain
			// at the site selected
			BuildRoad( pUnit, pTask );
			break;
		case IDT_REPAIR:
			// vehicle has been sent to repair a building,
			// bridge or road; and is either enroute or at
			// the site or needs to be sent to the site
			RepairConstruction( pUnit );
			break;
		case IDT_REPAIRING:
			RepairingUnit( pUnit );
			// vehicle is at a repair facility undergoing repair
			break;
		case IDT_FINDOPFORCITY:
		case IDT_FINDOPFORS:
			// find nearest opfor city hex
			ReconOpForCity( pUnit, pTask );
			break;
		case IDT_ATTACKUNIT: // IDG_CONQUER used in scenarios
			AttackUnit( pUnit, pTask );
			break;
		case IDT_SETTRANSPORT:
			// allow CRouteMgr to handle orders for transport
		default:
			break;
	}
}
//
// IDT_ATTACKUNIT
// this task indicates that a particular target is to be attacked
// and for the assigned unit to approach the target using the best
// defensive terrain within an area until arriving within range of
// the target, and at that time do a task switch to IDT_SEEKINWAR
// and leave it up to that process to assault the target
//
void CAITaskMgr::AttackUnit( CAIUnit *pUnit, CAITask * /* pTask */ )
{
	// if the target is destroyed, then switch goal/task to a null 
	// IDG_ESTABLISH/IDT_PATROL, then task switch leaving IDT_SEEKINWAR
	// as a seeker and IDT_PATROL as the switch task
	CAIUnit *pTarget = 
		m_pGoalMgr->m_plUnits->GetOpForUnit( pUnit->GetDataDW() );
	if( pTarget == NULL )
	{	
		pUnit->SetGoal(IDG_ESTABLISH);
		pUnit->SetTask(IDT_SEEKINWAR);
		pUnit->SetDataDW(0);
		return;
	}

	CHexCoord hexDest, hexVeh, hexTarget;
	EnterCriticalSection (&cs);
	CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
	if( pVehicle != NULL )
	{
		hexVeh = pVehicle->GetHexHead();
		hexDest = pVehicle->GetHexDest();
	}
	LeaveCriticalSection (&cs);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"\nCAITaskMgr::AttackUnit() unit %ld player %d at %d,%d and going to %d,%d ", 
	pUnit->GetID(), pUnit->GetOwner(), 
	hexVeh.X(), hexVeh.Y(), hexDest.X(), hexDest.Y() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAITaskMgr::AttackUnit() unit has scenario target %ld ", 
	pUnit->GetDataDW() );
#endif

	// if unit is just assigned, pick a random staging hex that
	// is around spotting distance from rocket and send unit there
	// after flagging unit as enroute
	if( !pUnit->GetParam(CAI_DAMAGE)  )
	{
		hexDest = hexVeh;

		m_pGoalMgr->m_pMap->m_pMapUtil->FindStagingHex( 
			hexVeh,1,1, pUnit->GetTypeUnit(), hexDest );
		m_pGoalMgr->m_pMap->PlaceFakeVeh( hexDest, 
			pUnit->GetTypeUnit() );

		pUnit->SetParam(CAI_DAMAGE,1); // flagging unit as enroute
		pUnit->SetParam(CAI_TARGETTYPE,0xFFFE); // and to target GetDataDW()
		pUnit->SetDestination( hexDest );	// staging location

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"\nCAITaskMgr::AttackUnit() unit %ld player %d in scenario enroute to %d,%d \n", 
	pUnit->GetID(), pUnit->GetOwner(), hexDest.X(), hexDest.Y() );
#endif

		return;
	}

	// get current location of the target
	if( pTarget->GetType() == CUnit::building )
	{
		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetDataDW() );
		if( pBldg != NULL )
		{
			hexTarget = pBldg->GetExitHex();
			// adjust to be adjacent to the exit hex
			m_pGoalMgr->m_pMap->GetCraneHex( hexTarget, hexTarget );
		}
		LeaveCriticalSection (&cs);
	}
	else if( pTarget->GetType() == CUnit::vehicle )
	{
		EnterCriticalSection (&cs);
		CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetDataDW() );
		if( pVehicle != NULL )
		{
			hexTarget = pVehicle->GetHexHead();
			// adjust to be adjacent to the head hex
			m_pGoalMgr->m_pMap->GetCraneHex( hexTarget, hexTarget );
		}
		LeaveCriticalSection (&cs);
	}
	// unable to find target so re-assign to patrol
	else
	{	
		pUnit->SetGoal(IDG_ESTABLISH);
		pUnit->SetTask(IDT_PATROL);
		pUnit->SetDataDW(0);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::AttackUnit() unit %ld player %d going to patrol \n", 
pUnit->GetID(), pUnit->GetOwner() );
#endif
		return;
	}

	int iDist = pGameData->GetRangeDistance( hexVeh, hexTarget );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::AttackUnit() unit %ld player %d has scenario target %ld ", 
pUnit->GetID(), pUnit->GetOwner(), pUnit->GetDataDW() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AttackUnit() target at %d,%d with range of %d \n",
hexTarget.X(), hexTarget.Y(), iDist );
#endif


	// if unit is within range of target, then task switch to IDT_SEEKINWAR
	if( InRange(pUnit,hexTarget) )
	{
		WORD wStatus = pUnit->GetStatus();
		wStatus |= CAI_IN_COMBAT;
		wStatus |= CAI_TASKSWITCH;
		pUnit->SetStatus( wStatus );

		// go ahead and attack it
		pUnit->AttackUnit( pTarget->GetID() );
		pTarget->AttackedBy( pUnit->GetID() );

		// save old task
		pUnit->SetParam( CAI_UNASSIGNED, pUnit->GetTask() );
		// assign unit to attacked
		pUnit->SetTask( IDT_SEEKINWAR );

		// moving helps the seeking unit
		// stay in contact with the target unit
		MoveToRange(pUnit,hexTarget);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::AttackUnit() unit %ld player %d in InRange %d,%d \n", 
pUnit->GetID(), pUnit->GetOwner(), hexTarget.X(), hexTarget.Y() );
#endif
	}
	// if unit is not within range of target
	// pick a random defensive hex between unit and target
	// not more than spotting range out from unit
	else
	{
		// unit has not reached last approach hex yet
		if( hexDest != hexVeh )
			return;

		m_pGoalMgr->m_pMap->m_pMapUtil->FindApproachHex(
			hexTarget, pUnit, hexDest );
		
		// either game is to be difficult or
		// could not get a location so try a nearby beeline
		// or if approach hex is moving away
		if( pGameData->m_iSmart > 1 ||  hexDest == hexVeh ||
			pGameData->GetRangeDistance(hexDest, hexTarget) >= iDist )
		{
			//void CAIMapUtil::FindDefenseHex( CHexCoord& hexAttacking, 
			//CHexCoord& hexDefending, int iWidth, int iHeight, 
			//CTransportData const *pVehData )
			CTransportData const *pVehData = 
				pGameData->GetTransportData( pUnit->GetTypeUnit());
			if( pVehData == NULL )
				return;

			hexDest = hexVeh;
			m_pGoalMgr->m_pMap->m_pMapUtil->FindDefenseHex( 
				hexTarget, hexDest, pVehData->_GetRange(), 
				pVehData->_GetRange(), pVehData );
		}

		pUnit->SetDestination( hexDest );	// go to approach location

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::AttackUnit() unit %ld player %d in scenario approaching %d,%d \n", 
pUnit->GetID(), pUnit->GetOwner(), hexDest.X(), hexDest.Y() );
#endif

	}
}

//
// this could be reached as a result of a destination message
// or as a result of the task being assigned to this unit
//
// if its a destination, then the unit may be where it needs
// to be, and thus should attack.   If the unit is not where
// it needs to be then SetDestination should be recalled.
// this is indicated by non-zero CAIUnits::m_dwData
//
// if its a task assignment, then an opfor unit must be selected
// to be the target of the seek, its location determined and a
// viable destination within firing range found and then the
// unit sent to the destination.
//
void CAITaskMgr::SeekOpfor( CAIUnit *pUnit, CAITask *pTask )
{
	CHexCoord hexDest,hexSeek;

#ifdef _LOGOUT
	int iTargetType = 0;
	int iTargetTypeUnit = 0;
	if( pUnit->GetDataDW() )
	{
		CAIUnit *pTarget = 
			m_pGoalMgr->m_plUnits->GetOpForUnit( pUnit->GetDataDW() );
		if( pTarget != NULL )
		{
			iTargetType = pTarget->GetType();
			iTargetTypeUnit = pTarget->GetTypeUnit();
		}
	}
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"\nCAITaskMgr::SeekOpfor() unit %ld player %d has task %d goal %d target %ld ", 
	pUnit->GetID(), pUnit->GetOwner(), pUnit->GetTask(), pUnit->GetGoal(), pUnit->GetDataDW() );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAITaskMgr::SeekOpfor() attacker type %d/%d  target type %d/%d \n",
		pUnit->GetType(), pUnit->GetTypeUnit(), iTargetType, iTargetTypeUnit );
#endif



	SeekNDestroy:

	// non-zero means an opfor unit is selected for attack
	if( pUnit->GetDataDW() )
	{
		CAIUnit *pTarget = 
			m_pGoalMgr->m_plUnits->GetOpForUnit( pUnit->GetDataDW() );
		if( pTarget == NULL )
		{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::SeekOpfor() player %d unit %ld target %ld not found in m_plUnits ", 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetDataDW() );
#endif
			// prior target is gone, reset and pick another
			pUnit->SetDataDW(0);
			pUnit->SetParam(CAI_TARGETTYPE,0xFFFE );
			goto SeekNDestroy;
		}

		// make sure that the proper leave occurs
		EnterCriticalSection (&cs);

		if( pUnit->GetParam(CAI_TARGETTYPE) == CUnit::building ||
			pUnit->GetParam(CAI_TARGETTYPE) == 0xFFFE )
		{
			CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetDataDW() );
			if( pBldg != NULL )
			{
				pUnit->SetParam(CAI_TARGETTYPE,CUnit::building );

				//hexDest = pBldg->GetHex();
				hexSeek = pBldg->GetExitHex();
				LeaveCriticalSection (&cs);

				if( InRange(pUnit,hexSeek) )
				{
					UnloadCargo(pUnit);

					pUnit->AttackUnit( pTarget->GetID() );
					pTarget->AttackedBy( pUnit->GetID() );
					// clear seeker count cause we've attacked now
					pTarget->SetParam(CAI_SPOTTING,0);
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::SeekOpfor() unit %ld player %d attacking building %ld \n", 
pUnit->GetID(), pUnit->GetOwner(), pTarget->GetID() );
#endif

#if 0
					// since rockets can now spawn other units its more important
					if( pTarget->GetTypeUnit() != CStructureData::rocket &&
						TargetVehicles(pUnit) )
					{
						pUnit->SetDataDW(0);
						pUnit->SetParam(CAI_TARGETTYPE,0xFFFE );
						goto SeekNDestroy;
					}
#endif
					return;
				}
				else
				{
					CHexCoord hexVeh(0,0);
					EnterCriticalSection (&cs);
					CVehicle *pVehicle = 
					theVehicleMap.GetVehicle( pUnit->GetID() );
					if( pVehicle != NULL )
					{
						hexVeh = pVehicle->GetHexHead();
						hexDest = pVehicle->GetHexDest();
					}
					LeaveCriticalSection (&cs);

					if( hexVeh.X() || hexVeh.Y() )
					{
						int iDist = 
						pGameData->GetRangeDistance( hexVeh, hexSeek );

						// get pointer to vehicle type data, in order to
						// consider early unloading of any cargo
						CTransportData const *pVehData = 
							theTransports.GetData(pUnit->GetTypeUnit());
						if( pVehData != NULL )
						{
							if( iDist < (pVehData->_GetRange() * 3) )
								UnloadCargo(pUnit);
						}

// display ranges when out of range
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"\nCAITaskMgr::SeekOpfor() unit %ld player %d at range %d to target %ld ", 
	pUnit->GetID(), pUnit->GetOwner(), iDist, pTarget->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAITaskMgr::SeekOpfor() unit %ld player %d at %d,%d seek to %d,%d ", 
	pUnit->GetID(), pUnit->GetOwner(), 
	hexVeh.X(), hexVeh.Y(), hexSeek.X(), hexSeek.Y() );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAITaskMgr::SeekOpfor() unit %ld player %d actual dest is %d,%d  \n", 
	pUnit->GetID(), pUnit->GetOwner(), 
	hexDest.X(), hexDest.Y() );
#endif
					}
				}

				// moving every time helps the seeking unit
				// stay in contact with the target unit
				if( pGameData->m_iSmart )
					MoveToRange(pUnit,hexSeek);

				return;
			}

			// if still here then target is not a building
			CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetDataDW() );
			if( pVehicle != NULL )
			{
				pUnit->SetParam(CAI_TARGETTYPE,CUnit::vehicle );

				hexSeek = pVehicle->GetHexHead();
				LeaveCriticalSection (&cs);

				if( InRange(pUnit,hexSeek) )
				{
					UnloadCargo(pUnit);

					pUnit->AttackUnit( pTarget->GetID() );
					pTarget->AttackedBy( pUnit->GetID() );
					// clear seeker count cause we've attacked now
					pTarget->SetParam(CAI_SPOTTING,0);
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::SeekOpfor() unit %ld player %d attacking vehicle %ld \n", 
pUnit->GetID(), pUnit->GetOwner(), pTarget->GetID() );
#endif
					return;
				}
				// moving every time helps the seeking unit
				// stay in contact with the target unit
				MoveToRange(pUnit,hexSeek);

				return;
			}
		}
		else if( pUnit->GetParam(CAI_TARGETTYPE) == CUnit::vehicle )
		{
			CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetDataDW() );
			if( pVehicle != NULL )
			{
				pUnit->SetParam(CAI_TARGETTYPE,CUnit::vehicle );

				hexSeek = pVehicle->GetHexHead();
				LeaveCriticalSection (&cs);

				if( InRange(pUnit,hexSeek) )
				{
					UnloadCargo(pUnit);

					pUnit->AttackUnit( pTarget->GetID() );
					pTarget->AttackedBy( pUnit->GetID() );
					// clear seeker count cause we've attacked now
					pTarget->SetParam(CAI_SPOTTING,0);
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::SeekOpfor() unit %ld player %d attacking vehicle %ld \n", 
pUnit->GetID(), pUnit->GetOwner(), pTarget->GetID() );
#endif
					return;
				}
				// moving every time helps the seeking unit
				// stay in contact with the target unit
				MoveToRange(pUnit,hexSeek);

				return;
			}
		}
		LeaveCriticalSection (&cs);

		// unit is gone, reset and pick another
		pUnit->SetDataDW(0);
		pUnit->SetParam(CAI_TARGETTYPE,0xFFFE );
	}

	// save the dwID of the opfor unit seeked, in this CAIUnit::m_dwData
	//
	// when the opfor unit is destroyed, clear the m_dwData with zero
	DWORD dwOpForUnit = 0;
	// depending on the type of seek, find the nearest opfor
	// unit of the seek, and move to range of the seek and fire
	if( pTask->GetID() == IDT_SEEKATSEA )
	{
		// CVehicle::landing_craft may have other CVehicles onboard,
		// that need to unload to participate and will occur InRange()

		// THREAT_TARGET, NEAREST_TARGET
		// CAI_NAVALATTACK, CAI_HARDATTACK (building), zero=any
		//
		// DWORD CAIGoalMgr::GetOpForUnit( int iHow, int iKindOf, CAIUnit *pUnit )
		// find threat opfor navy unit or if none
		dwOpForUnit = m_pGoalMgr->GetOpForUnit(
			THREAT_TARGET, CAI_NAVALATTACK, pUnit );
		if( dwOpForUnit )
		{
			pUnit->SetDataDW(dwOpForUnit);
			pUnit->SetParam(CAI_TARGETTYPE,CUnit::vehicle );
			goto SeekNDestroy;
		}

		// find nearest opfor building near shore or if none
		if( !dwOpForUnit )
		{
			dwOpForUnit = m_pGoalMgr->GetOpForUnit(
				NEAREST_TARGET, CAI_HARDATTACK, pUnit );
			if( dwOpForUnit )
			{
			pUnit->SetDataDW(dwOpForUnit);
			pUnit->SetParam(CAI_TARGETTYPE,CUnit::building );
			goto SeekNDestroy;
			}
		}

		// find nearest opfor unit regardless of type or if none
		if( !dwOpForUnit )
		{
			dwOpForUnit = m_pGoalMgr->GetOpForUnit(
				NEAREST_TARGET, 0, pUnit );
			if( dwOpForUnit )
			{
			pUnit->SetDataDW(dwOpForUnit);
			pUnit->SetParam(CAI_TARGETTYPE,0xFFFE );
			goto SeekNDestroy;
			}
		}
	}
	else if( pTask->GetID() == IDT_SEEKINWAR )
	{
		// CVehicle::infantry_carrier may have other CVehicles onboard,
		// that need to unload to participate and will occur InRange()

		// THREAT_TARGET, NEAREST_TARGET
		// CAI_SOFTATTACK (vehicle), CAI_HARDATTACK (building), zero=any
		//
		// find threat opfor land vehicle or if none
		dwOpForUnit = m_pGoalMgr->GetOpForUnit(
			THREAT_TARGET, CAI_SOFTATTACK, pUnit );
		if( dwOpForUnit )
		{
			pUnit->SetDataDW(dwOpForUnit);
			pUnit->SetParam(CAI_TARGETTYPE,CUnit::vehicle );
			goto SeekNDestroy;
		}

		// find best opfor building or if none
		if( !dwOpForUnit )
		{
			dwOpForUnit = m_pGoalMgr->GetOpForUnit(
				BEST_TARGET, CAI_HARDATTACK, pUnit );
			if( dwOpForUnit )
			{
			pUnit->SetDataDW(dwOpForUnit);
			pUnit->SetParam(CAI_TARGETTYPE,CUnit::building );
			goto SeekNDestroy;
			}
		}

		// find nearest opfor unit or if none
		if( !dwOpForUnit )
		{
			dwOpForUnit = m_pGoalMgr->GetOpForUnit(
				NEAREST_TARGET, 0, pUnit );
			if( dwOpForUnit )
			{
			pUnit->SetDataDW(dwOpForUnit);
			pUnit->SetParam(CAI_TARGETTYPE,0xFFFE );
			goto SeekNDestroy;
			}
		}
	}
	else if( pTask->GetID() == IDT_SEEKINRANGE )
	{
		// CVehicle::infantry_carrier may have other CVehicles onboard,
		// that need to unload to participate and will occur InRange()

		// THREAT_TARGET, BEST_TARGET, NEAREST_TARGET
		// CAI_SOFTATTACK (vehicle), CAI_HARDATTACK (building), zero=any
		// 
		dwOpForUnit = m_pGoalMgr->GetOpForUnit(
			BEST_TARGET, CAI_SOFTATTACK, pUnit );
		if( dwOpForUnit )
		{
			pUnit->SetDataDW(dwOpForUnit);
			pUnit->SetParam(CAI_TARGETTYPE,CUnit::vehicle );
			goto SeekNDestroy;
		}

		// find best threat opfor vehicle, within range,
		if( !dwOpForUnit )
		{
			dwOpForUnit = m_pGoalMgr->GetOpForUnit(
				THREAT_TARGET, CAI_SOFTATTACK, pUnit );
			if( dwOpForUnit )
			{
			pUnit->SetDataDW(dwOpForUnit);
			pUnit->SetParam(CAI_TARGETTYPE,CUnit::vehicle );
			goto SeekNDestroy;
			}
		}

		// or if none
		// find best target opfor building, within range,
		if( !dwOpForUnit )
		{
			dwOpForUnit = m_pGoalMgr->GetOpForUnit(
				BEST_TARGET, CAI_HARDATTACK, pUnit );
			if( dwOpForUnit )
			{
			pUnit->SetDataDW(dwOpForUnit);
			pUnit->SetParam(CAI_TARGETTYPE,CUnit::building );
			goto SeekNDestroy;
			}
		}

		// or if none
		// find nearest opfor building, within range,
		if( !dwOpForUnit )
		{
			dwOpForUnit = m_pGoalMgr->GetOpForUnit(
				NEAREST_TARGET, CAI_HARDATTACK, pUnit );
			if( dwOpForUnit )
			{
			pUnit->SetDataDW(dwOpForUnit);
			pUnit->SetParam(CAI_TARGETTYPE,CUnit::building );
			goto SeekNDestroy;
			}
		}

		// these units were switched from another task, in order
		// to support an attacked unit, so if there are not any
		// vehicles in range, then it does not need to go looking
		if( !dwOpForUnit )
		{
			if( !(pUnit->GetStatus() & CAI_TASKSWITCH) )
			{
				// or if none
				// find nearest opfor unit, outside of range
				dwOpForUnit = m_pGoalMgr->GetOpForUnit(
				NEAREST_TARGET, 0, pUnit );
				if( dwOpForUnit )
				{
				pUnit->SetDataDW(dwOpForUnit);
				pUnit->SetParam(CAI_TARGETTYPE,0xFFFE );
				goto SeekNDestroy;
				}
			}
		}

		// a staged vehicle, has been task switched, and no target yet
		if( !dwOpForUnit )
		{
			if( pUnit->GetParam(CAI_UNASSIGNED) == IDT_PREPAREWAR )
			{
				// find nearest opfor unit, outside of range
				dwOpForUnit = m_pGoalMgr->GetOpForUnit(
					NEAREST_TARGET, 0, pUnit );
				if( dwOpForUnit )
				{
				pUnit->SetDataDW(dwOpForUnit);
				pUnit->SetParam(CAI_TARGETTYPE,0xFFFE );
				goto SeekNDestroy;
				}
			}
		}
	}

	// could not find a target for the unit, so clear it
	// and unassign it and let the cycle begin later

	// before clearing, consider that the unit has been
	// task switched
	WORD wStatus = pUnit->GetStatus();
	if( (wStatus & CAI_TASKSWITCH) )
	{
		// reset to original task
		pUnit->SetTask( pUnit->GetParam(CAI_UNASSIGNED) );
		// turn off task switch flag
		wStatus ^= CAI_TASKSWITCH;
		wStatus ^= CAI_IN_COMBAT;
		pUnit->SetStatus(wStatus);

		// can't use ClearTaskUnit( pUnit ); to clear this one
		pUnit->ClearParam();
		pUnit->SetDataDW(0);
	}
	else
	{
		// consider unloading any cargo unit
		UnloadCargo(pUnit);

		ClearTaskUnit( pUnit );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::SeekOpfor() unit %ld player %d unassigned, target was %ld \n", 
pUnit->GetID(), pUnit->GetOwner(), dwOpForUnit );
#endif
		AssignCombat(pUnit);
	}
	CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
	if( pVehicle == NULL )
		return;
	hexDest = pVehicle->GetHexHead();

	m_pGoalMgr->m_pMap->m_pMapUtil->FindStagingHex( 
		hexDest,2,2,pUnit->GetTypeUnit(), hexDest, FALSE );

	pUnit->SetDestination( hexDest );

}

void CAITaskMgr::ClearTaskUnit( CAIUnit *pUnit )
{
	pUnit->ClearParam();
	pUnit->SetTask( FALSE );
	pUnit->SetGoal( FALSE );
	pUnit->SetDataDW(0);
	pUnit->SetStatus(0);
}

// IDT_REPAIRING
//
// this unit has been task switched to proceed to a repair factory
// and get repaired, and getting here is usually because of a 
// destination response
//
void CAITaskMgr::RepairingUnit( CAIUnit *paiUnit )
{
	CHexCoord hexBldg(0,0);
	CHexCoord hexRepair,hexDest;
	int iRepairPer = 0, iTypeRepair = 0;

	// get location, damage and destination of the unit to be repaired
	EnterCriticalSection (&cs);

	CVehicle *pVehicle = 
		theVehicleMap.GetVehicle( paiUnit->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return;
	}
	hexRepair = pVehicle->GetHexHead();
	hexDest = pVehicle->GetHexDest();
	iRepairPer = pVehicle->GetDamagePer();

	if( pVehicle->GetData()->IsBoat() )
		iTypeRepair = CStructureData::shipyard_1;
	else
		iTypeRepair = CStructureData::repair;

	// is vehicle dead and dying?
	if( pVehicle->GetFlags() & CUnit::dying )
		iRepairPer = 0;

	// has vehicle been assigned a repair depot
	if( paiUnit->GetDataDW() )
	{
		CBuilding *pBldg = 
		theBuildingMap.GetBldg( paiUnit->GetDataDW() );
		if( pBldg != NULL &&
			pBldg->GetData()->GetType() == CStructureData::repair )
		{
			hexBldg = pBldg->GetExitHex();
		}
	}
	LeaveCriticalSection (&cs);

	// unit is dying
	if( !iRepairPer )
		return;

	// vehicle has been repaired, and was sent out of the repair depot
	// by the game, and just arrived at its destination
	if( iRepairPer == DAMAGE_0 )
		goto SwitchBack;

	// repair building is gone or invalid, try for another
	if( !hexBldg.X() && !hexBldg.Y() )
	{
		// find nearest repair depot, returning exit hex in hexRepair
		CAIUnit *paiRepair = m_pGoalMgr->m_plUnits->GetClosestRepair(
		m_iPlayer, iTypeRepair, hexRepair );
		// no repair building remain
		if( paiRepair == NULL )
			goto SwitchBack;

		paiUnit->SetDataDW( paiRepair->GetID() );	// repair depot id
		paiUnit->SetDestination( hexRepair );	// repair depot location
		paiUnit->SetParam(CAI_FUEL,0);		// no repair message sent yet
		return;
	}

	//
	// when vehicle has just arrived at building, need to issue CMsgRepairVeh
	// but also, this function might get called after the vehicle has done
	// issued the message, but is not yet fully repaired, so how to detect
	// that and not reissue message?
	//
	//if( hexRepair == hexBldg )
	//	return;

	// already sent message to repair this vehicle
	if( paiUnit->GetParam(CAI_FUEL) )
		return;

	// vehicle is still enroute and got side tracked somehow
	if( hexRepair != hexBldg )
	{
		if( hexDest != hexBldg )
			paiUnit->SetDestination( hexBldg );
		return;
	}

	// we have arrived at the repair building, send message to game
	paiUnit->RepairVehicle();
	paiUnit->SetParam(CAI_FUEL,1); // only send the message once

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::RepairingUnit() player %d unit %d at %d,%d repair depot \n", 
paiUnit->GetOwner(), paiUnit->GetID(), hexBldg.X(), hexBldg.Y() );
#endif

	return;

	SwitchBack:
	// task switch back to original task?
	WORD wStatus = paiUnit->GetStatus();
	if( (wStatus & CAI_TASKSWITCH) )
	{
		wStatus ^= CAI_TASKSWITCH;
		paiUnit->SetStatus( wStatus );
		paiUnit->SetTask(paiUnit->GetParam(CAI_UNASSIGNED));
		paiUnit->SetParam(CAI_FUEL,0);
		paiUnit->SetDataDW(0);
	}
	else // something's wrong, so just unassign task/vehicle
	{
		paiUnit->SetStatus( 0 );
		ClearTaskUnit( paiUnit );
	}
}

// IDT_FINDOPFORCITY
//
// this will direct the unit to a location near the city of
// the opfor as indicated by the flag
//
void CAITaskMgr::ReconOpForCity( CAIUnit *pCbtVeh, CAITask *pTask )
{	
	CHexCoord hexDest,hexCity,hexVeh;
	int iOpForId, iSpotting, iType;
	CTransportData const *pVehData = NULL;

	EnterCriticalSection (&cs);
	CVehicle *pVehicle = theVehicleMap.GetVehicle( pCbtVeh->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return;
	}
	hexVeh = pVehicle->GetHexHead();
	iSpotting = pVehicle->GetSpottingRange();
	pVehData = pVehicle->GetData();
	iType = pVehData->GetType();
	LeaveCriticalSection (&cs);

	// easy level never seeks out opfor, so switch to IDT_SCOUT task
	if( !pGameData->m_iSmart )
	{
		pTask->SetStatus( UNASSIGNED_TASK );
		CAITask *pNewTask = 
			m_pGoalMgr->m_plTasks->FindTask( IDT_SCOUT );
		{
			if( pNewTask == NULL )
				goto ErrRet;

			// check count of units doing task and assign
			// this task if count is less than difficulty
			WORD wCnt = 
				pNewTask->GetTaskParam(TO_MATERIAL_ID);

			pCbtVeh->SetGoal( pNewTask->GetGoalID() );
			pCbtVeh->SetTask( pNewTask->GetID() );
			pNewTask->SetTaskParam(TO_MATERIAL_ID,(wCnt+1));

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::ReconOpForCity reassigned to %d, goal %d, unit %ld player %d ", 
pNewTask->GetID(), pNewTask->GetGoalID(), pCbtVeh->GetID(), pCbtVeh->GetOwner() );
#endif

			ExploreMap( pCbtVeh, pNewTask );
			return;
		}
	}

	// unit has reached an intermediate destination?
	// so resend the current destination of the task
	if( pTask->GetTaskParam(TO_MATERIAL_ID) )
	{

	// get location of vehicle, if it is within spotting
	// range of the rocket, then it needs to bolt on out
	// of here or move on to another opfor city?

		hexDest.X(pTask->GetTaskParam(TO_X));
		hexDest.Y(pTask->GetTaskParam(TO_Y));
		if( hexDest != hexVeh )
			pCbtVeh->SetDestination( hexDest );
		else 
		{
			// tell goal mgr that this opfor is known now
			iOpForId = pTask->GetTaskParam(TO_MATERIAL_ID);
			m_pGoalMgr->SetKnownOpFor(iOpForId);

			// find the nearest building again
			hexCity = hexVeh;
			pGameData->FindBuilding( 0, iOpForId, hexCity );

			// are we close enough to move on 
			int iRange = pGameData->GetRangeDistance(hexVeh,hexCity);
			if( iRange > iSpotting )
			{
				hexDest = hexVeh;
				m_pGoalMgr->m_pMap->m_pMapUtil->FindStagingHex( 
					hexCity,(iSpotting / 2), (iSpotting / 2), 
					iType, hexDest );

				// stick around and harrass the opfor
				goto GoRecon;
			}

			// close enough, so end this task
			pTask->SetTaskParam(TO_MATERIAL_ID,0);
			pTask->SetStatus( UNASSIGNED_TASK );

			// unassign unit
			ClearTaskUnit( pCbtVeh );
		}
		return;
	}


	// if still here, then the destination must be
	// determined and set in the task
	iOpForId = m_pGoalMgr->GetOpForId(WEAKEST_TARGET,hexVeh,TRUE);
	if( !iOpForId )
		iOpForId = m_pGoalMgr->GetOpForId(NEAREST_TARGET,hexVeh,TRUE);
	if( !iOpForId )
		iOpForId = m_pGoalMgr->GetOpForId(NEAREST_TARGET,hexVeh,FALSE);

	// no opfors known yet, so just explore
	if( !iOpForId )
	{
		ExploreMap( pCbtVeh, pTask );
		return;
	}

	// find the nearest building of the picked opfor
	hexCity = hexVeh;

	pGameData->FindBuilding( 0, iOpForId, hexCity );

	if( hexCity == hexVeh )
		goto ErrRet;

	hexDest = hexVeh;

	// let difficulty decide where to go, how to get there
	if( pGameData->m_iSmart > 2 )
	{
		m_pGoalMgr->m_pMap->m_pMapUtil->FindDefenseHex( 
		hexCity, hexDest, pVehData->_GetRange(), 
		pVehData->_GetRange(), pVehData );
	}
	else if( pGameData->m_iSmart > 1 )
	{
		m_pGoalMgr->m_pMap->m_pMapUtil->FindStagingHex( 
		hexCity, iSpotting, iSpotting, iType, hexDest );
	}
	else 
		m_pGoalMgr->m_pMap->m_pMapUtil->FindApproachHex(
		hexCity, pCbtVeh, hexDest );

	m_pGoalMgr->m_pMap->PlaceFakeVeh( hexDest, iType );

	GoRecon:
	// store the new destination and the opfor being scouted
	pTask->SetTaskParam(TO_X, hexDest.X());
	pTask->SetTaskParam(TO_Y, hexDest.Y());
	pTask->SetTaskParam(TO_MATERIAL_ID, iOpForId);

	pCbtVeh->SetDestination( hexDest );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::ReconOpForCity() player %d unit %ld going to %d,%d ", 
m_iPlayer, pCbtVeh->GetID(), hexDest.X(), hexDest.Y() );
#endif
	return;

	// can not get the info to do the task
	ErrRet:
	ClearTaskUnit( pCbtVeh );
	// unassign this task to try again
	pTask->SetTaskParam(TO_MATERIAL_ID,0);
	pTask->SetStatus( UNASSIGNED_TASK );
	// let goal manager reset priority
	pTask->SetPriority( (BYTE)0 );
}
//
// handle IDT_PREPAREWAR tasks by preparing an assault
// taskforce and staging it near the city, and routing units
// assigned this task to it
//
//  IDT_PREPAREWAR - assemble a task force
//
// params will be used to:
// 0,1 - start of staging block
// 2,3 - end of staging block
//
// for IDG_ADVDEFENSE and IDG_LANDWAR versions of task
// 4 - how many "light_tank,med_tank,heavy_tank" in TF
// 5 - how many "infantry_carrier" in TF
// 6 - how many "light_art,med_art,heavy_art" in TF
// 7 - how many "infantry,ranger" in TF
//
// for IDG_PIRATE version of task
// 4 - how many "cruiser"
// 5 - how many "destroyer"
//
// for IDG_SEAINVADE version of task
// 4 - how many "gun_boat"
// 5 - how many "marines"
// 6 - how many "light_tank,med_tank,light_art"
// 7 - how many "landing_craft"
//
void CAITaskMgr::StageUnit( CAIUnit *pCbtVeh, CAITask *pTask )
{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::StageUnit() for unit %ld player %d for goal %d task %d ", 
pCbtVeh->GetID(), pCbtVeh->GetOwner(), pTask->GetGoalID(), pTask->GetID() );
#endif

	// check pTask params, if all 0, then this is the first
	// unit assigned to be staged, so find a staging area
	if( !pTask->GetTaskParam(CAI_LOC_X) &&
		!pTask->GetTaskParam(CAI_LOC_Y) )
	{
		// finds staging area and sets numbers of units
		// to have in this taskforce
		m_pGoalMgr->GetStagingArea(pTask);

		// no staging area could be found
		if( !pTask->GetTaskParam(CAI_LOC_X) &&
			!pTask->GetTaskParam(CAI_LOC_Y) )
		{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::GetStagingArea() failed for player %d unit %ld type %d for goal %d task %d \n", 
pCbtVeh->GetOwner(), pCbtVeh->GetID(), pCbtVeh->GetTypeUnit(), 
pTask->GetGoalID(), pTask->GetID() );
#endif
			AssignPatrol(pCbtVeh);
			return;
		}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::StageUnit() player %d initiated for goal %d task %d ", 
m_iPlayer, pTask->GetGoalID(), pTask->GetID() );
if( pTask->GetGoalID() == IDG_ADVDEFENSE ||
	pTask->GetGoalID() == IDG_LANDWAR )
{
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"AFV=%d  IFV=%d  ART=%d  INF=%d \n",
pTask->GetTaskParam(CAI_TF_TANKS),pTask->GetTaskParam(CAI_TF_IFVS),
pTask->GetTaskParam(CAI_TF_ARTILLERY),pTask->GetTaskParam(CAI_TF_INFANTRY) );
}
// for IDG_SEAINVADE version of task
// #define CAI_TF_ARMOR		4
// #define CAI_TF_LANDING	5
// #define CAI_TF_SHIPS		6
// #define CAI_TF_MARINES	7
//
else if( pTask->GetGoalID() == IDG_SEAINVADE ||
	pTask->GetGoalID() == IDG_PIRATE )
{
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"SHIPS=%d  MARINE=%d  ARMOR=%d  LC=%d \n",
pTask->GetTaskParam(CAI_TF_SHIPS),pTask->GetTaskParam(CAI_TF_MARINES),
pTask->GetTaskParam(CAI_TF_ARMOR),pTask->GetTaskParam(CAI_TF_LANDING) );
}
#endif
		
		m_pGoalMgr->m_pMap->m_pMapUtil->FlagStagingArea( TRUE,
			pTask->GetTaskParam(CAI_LOC_X),pTask->GetTaskParam(CAI_LOC_Y),
			pTask->GetTaskParam(CAI_PREV_X),pTask->GetTaskParam(CAI_PREV_Y) );

		// since this staging task could be started with some
		// of the existing patrol units check all IDT_PATROL for
		// a possible task switch
		ConsiderPatrols(pTask);
	}

	BOOL bCarried = FALSE;
	CHexCoord hexVeh,hexDest;
	int iSpotting;
	EnterCriticalSection (&cs);
	CVehicle *pVehicle = theVehicleMap.GetVehicle( pCbtVeh->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return;
	}
	hexVeh = pVehicle->GetHexHead();
	hexDest = pVehicle->GetHexDest();
	iSpotting = pVehicle->GetSpottingRange();
	if( pVehicle->GetTransport() != NULL )
		bCarried = TRUE;
	LeaveCriticalSection (&cs);

	CHexCoord hcStart( pTask->GetTaskParam(CAI_LOC_X),
		pTask->GetTaskParam(CAI_LOC_Y) );
	CHexCoord hcEnd( pTask->GetTaskParam(CAI_PREV_X),
		pTask->GetTaskParam(CAI_PREV_Y) );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::StageUnit() unit %ld player %d at %d,%d dest is %d,%d  carried=%d ", 
pCbtVeh->GetID(), pCbtVeh->GetOwner(), hexVeh.X(), hexVeh.Y(),
hexDest.X(), hexDest.Y(), bCarried );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"staging area from %d,%d to %d,%d \n",
hcStart.X(), hcStart.Y(), hcEnd.X(), hcEnd.Y() );
#endif

	// compensate for the bug that some carried vehicles have hex!=dest
	if( bCarried && hexDest != hexVeh )
		hexDest == hexVeh;

	// if pCbtVeh current location is within staging area
	// then just return, as vehicle is where it needs to be
	if( hexVeh == hexDest &&
		m_pGoalMgr->m_pMap->m_pMapUtil->IsHexInArea( 
		hcStart, hcEnd, hexVeh) )
	{
		// mark vehicle as staged
		pCbtVeh->SetParam( CAI_UNASSIGNED, 0 );
		
		// consider enemy in range?
		if( InRange(hexVeh,iSpotting) )
		{
			// perform a task switch now to IDT_SEEKINRANGE
			pCbtVeh->SetDataDW(0);
			pCbtVeh->SetParam(CAI_TARGETTYPE,0xFFFE );

			WORD wStatus = pCbtVeh->GetStatus();
			wStatus |= CAI_IN_COMBAT;
			wStatus |= CAI_TASKSWITCH;
			pCbtVeh->SetStatus( wStatus );

			// save old task
			pCbtVeh->SetParam( CAI_UNASSIGNED, pCbtVeh->GetTask() );
			// assign unit to attacked
			pCbtVeh->SetTask( IDT_SEEKINRANGE );
			return;
		}
			

		LoadCargo( pCbtVeh, pTask );

		// except, this could be the last vehicle needed to
		// be staged, in order to launch the assault
		if( IsStagingCompete(pTask) )
		{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::StageUnit() completed with player %d unit %ld for goal %d task %d \n", 
m_iPlayer, pCbtVeh->GetID(), pTask->GetGoalID(), pTask->GetID() );
#endif
			// or the last vehicle to arrive at the assault destination
			if( pTask->GetStatus() == INPROCESS_TASK )
				pTask->SetStatus(COMPLETED_TASK);

			m_pGoalMgr->LaunchAssault(pTask);

			// need to unassign task, all units that were involved
			// are now at the assault destination and in SEEK* mode
			if( pTask->GetStatus() == COMPLETED_TASK )
			{
				// tell mapmgr that staging area is no longer in use
				m_pGoalMgr->m_pMap->m_pMapUtil->FlagStagingArea( FALSE,
					pTask->GetTaskParam(CAI_LOC_X),
					pTask->GetTaskParam(CAI_LOC_Y),
					pTask->GetTaskParam(CAI_PREV_X),
					pTask->GetTaskParam(CAI_PREV_Y) );

				pTask->SetStatus(UNASSIGNED_TASK);
				pTask->SetPriority(0);
				// clear task params for use
				for( int i=0; i<MAX_TASKPARAMS; ++i )
					pTask->SetTaskParam(i,0);
			}
		}
		return;
	}
	else
	{
		// if there are sufficient numbers of this unit type, already
		// assigned to this IDT_PREPAREWAR task, or this unit type is
		// not assigned to the task, then this unit must be unassigned
		if( !ContinueStaging(pCbtVeh, pTask) )
		{
			// flag task as full
			//pTask->SetPriority( (BYTE)0 );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::ContinueStaging() failed for player %d unit %ld type %d for goal %d task %d \n", 
pCbtVeh->GetOwner(), pCbtVeh->GetID(), pCbtVeh->GetTypeUnit(), 
pTask->GetGoalID(), pTask->GetID() );
#endif
			AssignPatrol(pCbtVeh);
			return;
		}
	}

#ifdef _LOGOUT
int iEnroute = 0;
if( pCbtVeh->GetParam(CAI_UNASSIGNED) == (WORD)INPROCESS_TASK )
	iEnroute = 1;

int iInUse = 0;
if( (pCbtVeh->GetStatus() & CAI_IN_USE) )
	iInUse = 1;

logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::StageUnit() unit %ld player %d   in use=%d  enroute=%d ", 
pCbtVeh->GetID(), pCbtVeh->GetOwner(), iInUse, iEnroute );
#endif

	// the unit is already in enroute to staging location
	// so there is no need to set another staging hex
	if( pCbtVeh->GetParam(CAI_UNASSIGNED) != (WORD)UNASSIGNED_TASK )
	{
		if( hexVeh != hexDest ) 
			return;
	}

	// unit is trying to load on somthing
	if( pCbtVeh->GetStatus() & CAI_IN_USE )
	{
		// not yet arrived
		if( hexVeh != hexDest )
			return;
		// already entered entered staging area, waiting to load
		if( m_pGoalMgr->m_pMap->m_pMapUtil->IsHexInArea( 
			hcStart, hcEnd, hexDest) )
			return;
	}

	// done with units being carried
	if( bCarried )
		return;

	// else use pTask params defined staging area, and pick a 
	// random location in that area, and send vehicle to it
	hexDest = hexVeh;

	// try to find a staging hex, 
	m_pGoalMgr->m_pMap->m_pMapUtil->FindStagingHex(
		pTask->GetTaskParam(CAI_LOC_X), pTask->GetTaskParam(CAI_LOC_Y),
		pTask->GetTaskParam(CAI_PREV_X), pTask->GetTaskParam(CAI_PREV_Y),
		hexDest, pCbtVeh->GetTypeUnit() );

	// could not find a staging hex, so try later on
	if( hexDest == hexVeh )
	{
		// need a way to make sure StageUnit() gets called again
		// and by unassigning this unit it will be, but the unit
		// may not be assigned when the task is re-staged and launched
		ClearTaskUnit( pCbtVeh );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"\nCAITaskMgr::StageUnit() player %d unit %ld could not stage, unassigning \n", 
		m_iPlayer, pCbtVeh->GetID() );
#endif

		return;
	}

	pCbtVeh->SetDestination( hexDest );

	// force occupation of staging hex in map
	m_pGoalMgr->m_pMap->PlaceFakeVeh( hexDest, pCbtVeh->GetTypeUnit() );
	// record that this unit is in the process of staging
	pCbtVeh->SetParam( CAI_UNASSIGNED, (WORD)INPROCESS_TASK );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAITaskMgr::StageUnit() player %d unit %ld going to %d,%d ", 
		m_iPlayer, pCbtVeh->GetID(), hexDest.X(), hexDest.Y() );
#endif
}

//
// this will construct and issue the message to the server
// that will cause the building (pBldg) to begin production
// of the vehicle indicated by the task (pTask)
//
void CAITaskMgr::ProduceVehicle( CAIUnit *pBldg, CAITask *pTask )
{
	if( pBldg == NULL )
		return;

	DWORD dwFactory = pBldg->GetID();
	int iVehType = pTask->GetTaskParam(PRODUCTION_ITEM);
/*
task parameters for a PRODUCTION_ORDER

#define ORDER_TYPE			0	// type of order to issue

// CAITask::m_pwaParams[] used for PRODUCTION_ORDER
#define PRODUCTION_ITEM		1	// type id of material or vehicle
#define PRODUCTION_TYPE		2   // material or vehicle to produce
#define PRODUCTION_ID1		3	// id of primary factory type
#define PRODUCTION_ID2		4	// id of secondary factory type
#define PRODUCTION_QTY		5	// how many to build
*/

/*
// tell a building to build a vehicle
class CMsgBuildVeh : public CNetCmd
{
public:
		CMsgBuildVeh () : CNetCmd (build_veh) {}
		CMsgBuildVeh (CBuilding const *pBldg, int iVehType, int iNum = 1);

		DWORD				m_dwID;				// ID of factory
		int					m_iVehType;		// type of vehicle to build
		int					m_iNum;				// how many

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
	CMsgBuildVeh msg;
	msg.m_dwID = dwFactory;
	msg.m_iVehType = iVehType;

	if( pGameData->m_iSmart >= 1 )
	{
		msg.m_iNum = m_pGoalMgr->GetNumVehToBuild(iVehType);
		// if building a lot of these, then shut down the task for awhile
		//if( msg.m_iNum > pGameData->m_iSmart )
		//	pTask->SetPriority(0);
	}
	else
		msg.m_iNum = 1;
	theGame.PostToServer( (CNetCmd *)&msg, sizeof(CMsgBuildVeh) );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::ProduceVehicle() sending CMsgBuildVeh for player %d unit %ld to build %d of %d", 
m_iPlayer, dwFactory, msg.m_iNum, iVehType );
#endif
	
}

//
// this handles water only ships for patrols
//
void CAITaskMgr::PatrolSea( CAIUnit *pUnit, CAITask *pTask, 
	CTransportData const *ptdShip )
{
	CHexCoord hexDest;
	int iX, iY;
	// this ship has just been assigned to patrol or is
	// being reassigned as a result of being lost or having
	// already completed the previous patrol
	if( !pUnit->GetParam(CAI_LOC_X) &&
		!pUnit->GetParam(CAI_LOC_Y) )
	{
		// get a patrol pattern from the goal manager
		// which will be stored in the unit params (and
		// not in the task params like land patrols)
		m_pGoalMgr->GetSeaPatrol(pUnit,pTask,ptdShip);

		iX = pUnit->GetParam(0);
		iY = pUnit->GetParam(1);
	}
	else // already patroling and may be at a patrol point
	{
		// get access to CVehicle copied data for vehicle
		CAICopy *pCopyCVehicle = 
			pUnit->GetCopyData( CAICopy::CVehicle );
		if( pCopyCVehicle == NULL )
			return;

		// get current location
		int iX = pCopyCVehicle->m_aiDataOut[CAI_LOC_X];
		int iY = pCopyCVehicle->m_aiDataOut[CAI_LOC_Y];

		// if at point 0, then go to point 1
		if( iX == pUnit->GetParam(0) &&
			iY == pUnit->GetParam(1) )
		{
			iX = pUnit->GetParam(2);
			iY = pUnit->GetParam(3);
		}
		// else if at point 1, then go to point 2
		else if( iX == pUnit->GetParam(2) &&
			iY == pUnit->GetParam(3) )
		{
			iX = pUnit->GetParam(4);
			iY = pUnit->GetParam(5);
		}
		// else if at point 2, then go to point 3
		else if( iX == pUnit->GetParam(4) &&
			iY == pUnit->GetParam(5) )
		{
			iX = pUnit->GetParam(6);
			iY = pUnit->GetParam(7);
		}
		// else if at point 3 or lost somewhere in between
		// then go to point 0 and get new patrol pattern
		else
		{
			iX = pUnit->GetParam(0);
			iY = pUnit->GetParam(1);

			pUnit->ClearParam();
		}
	}
	if( !iX && !iY )
	{
		ClearTaskUnit( pUnit );
		return;
	}

	hexDest.X(iX);
	hexDest.Y(iY);

	pUnit->SetDestination( hexDest );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::PatrolSea() player %d unit %ld going to %d,%d ", 
m_iPlayer, pUnit->GetID(), hexDest.X(), hexDest.Y() );
#endif
}


//
// This can be reached in 3 ways:  
// If the patrolling unit has reached a patrol destination
// and now needs to go back to other patrol endpoint
// or
// A new patrolling unit has been assigned to this task
// and needs to be assigned a patrol leg and be sent to
// that leg's first endpoint, after recalculating the
// area patrolled
// or
// via idle processing
//
void CAITaskMgr::PatrolArea( CAIUnit *pUnit, CAITask *pTask )
{
	// handles water only and land vehicles differently
	CTransportData const *pVehData = 
		pGameData->GetTransportData( pUnit->GetTypeUnit() );
	if( pVehData == NULL )
		return;

	// get game current info: number units carried, location 
	CHexCoord hexDest,hexVeh;
	int iCntCarried = 0;
	BOOL bCarried = FALSE;
	
	EnterCriticalSection (&cs);
	CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return;
	}
	hexVeh = pVehicle->GetHexHead();
	hexDest = pVehicle->GetHexDest();
	iCntCarried = pVehicle->GetCargoCount();
	if( pVehicle->GetTransport() != NULL )
		bCarried = TRUE;
	
	LeaveCriticalSection (&cs);

	// bCarried at this point means an unit carried on a transport
	if( bCarried )
		return;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::PatrolArea() player %d unit %ld is at %d,%d",
pUnit->GetOwner(), pUnit->GetID(), hexVeh.X(), hexVeh.Y() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::PatrolArea() patrol dest is %d,%d  game dest is %d,%d", 
pUnit->GetParam(CAI_DEST_X), pUnit->GetParam(CAI_DEST_Y), hexDest.X(), hexDest.Y() );
#endif

	// if a vehicle is an infantry_carrier, and has no onboard infantry
	// then find the nearest infantry with this task and assign carrier
	// to load that infantry and move towards it. if already assigned 
	// then continue to move toward it until infantry is loaded.
	if( pUnit->GetTypeUnit() == CTransportData::infantry_carrier )
	{
		if( !iCntCarried && !pUnit->GetParamDW(CAI_GRDDEFENSE) )
		{
			// return a TRUE if trying to link up, or FALSE
			// if there are nothing to link with
			bCarried = FindPatrolCargo( pUnit, pTask );
		}

		// enroute to assigned infantry unit to be picked up
		if( !iCntCarried && pUnit->GetParamDW(CAI_GRDDEFENSE) )
		{
			LoadPatrolCargo(pUnit);
			return;
		}
	}

	// if a vehicle is infantry, and is not loaded, then find nearest
	// infantry_carrier with the same task, that does not have an infantry
	// already loaded, and assign infantry and carrier to load that unit,
	// and move toward it.  if already assigned, then continue to move
	// toward carrier until infantry is loaded.
	if( pUnit->GetTypeUnit() == CTransportData::infantry ||
		pUnit->GetTypeUnit() == CTransportData::rangers )
	{
		// first consider that there are IFVs on patrol w/o infantry

		// this infantry unit is not carried and not linking yet
		if( !bCarried && !pUnit->GetParamDW(CAI_GRDDEFENSE) )
		{
			// return a TRUE if trying to link up, or FALSE
			// if there are nothing to link with
			bCarried = FindPatrolCarrier( pUnit, pTask );
		}

		// awaiting arrival of IFV to carry on patrol
		if( !bCarried && pUnit->GetParamDW(CAI_GRDDEFENSE) )
		{
			LoadPatrolCargo(pUnit);
			return;
		}
	}

	// bCarried at this point now means to not continue because
	// unit is a carrier/cargo unit and there is a carrier/cargo
	// that it is trying to link with so don't go and patrol yet
	if( bCarried )
		return;

	// the unit is awaiting a load or to be loaded
	if( pUnit->GetStatus() & CAI_IN_USE )
		return;

	// if a unit has 0 in GetParamDW(CAI_PATROL) then unit has just
	// been assigned patrol task, or come back from a task switch so 
	// get goal mgr to set up a patrol building, 
	if( !pUnit->GetParamDW(CAI_PATROL) )
	{
		// until # of vehicles exceeds number of buildings then there will
		// always be a building that needs a patrol from the next vehicle
		// that comes available

		DWORD dwBuilding = 0;
		if( pVehData->GetTargetType() == CUnitData::naval )
			dwBuilding = m_pGoalMgr->GetPatrolBuilding(pTask,TRUE);
		else
			dwBuilding = m_pGoalMgr->GetPatrolBuilding(pTask);
		if( !dwBuilding )
		{
			ClearTaskUnit( pUnit );
			return;
		}
		// assign unit to patrol this building or vehicle
		pUnit->SetParamDW( CAI_PATROL, dwBuilding );
		pUnit->SetParam( CAI_DEST_X, 0 );
		pUnit->SetParam( CAI_DEST_Y, 0 );

		hexDest = hexVeh;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::PatrolArea() player %d new unit %ld patroling %ld ", 
m_iPlayer, pUnit->GetID(), dwBuilding );
#endif
	}

	// if unit has a patrol building, and is here with current
	// location matching destination, then it is time to get
	// a new patrol location at firing range from patrol building
	// and set destination to it
	if( hexDest == hexVeh )
	{
		// unit has been given a patrol destination but the game
		// has not yet initiated movement toward it
		//if( pUnit->GetParam(CAI_DEST_X) ||
		//	pUnit->GetParam(CAI_DEST_Y) )
		//{
		//	if( pUnit->GetParam(CAI_DEST_X) != hexVeh.X() ||
		//		pUnit->GetParam(CAI_DEST_Y) != hexVeh.Y() )
		//		return;
		//}

		m_pGoalMgr->GetPatrolHex( pUnit, hexDest );
		if( hexDest != hexVeh )
		{
			pUnit->SetDestination( hexDest );

			// record patrol destination to deal with latency of game
			pUnit->SetParam( CAI_DEST_X, hexDest.X() );
			pUnit->SetParam( CAI_DEST_Y, hexDest.Y() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::PatrolArea() unit %ld player %d going to %d,%d ", 
pUnit->GetID(), pUnit->GetOwner(), hexDest.X(), hexDest.Y() );
#endif
			return;
		}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::PatrolArea() unit %ld player %d unassigning ", 
pUnit->GetID(), pUnit->GetOwner() );
#endif
		// could not find a patrol hex, so unassign unit and return
		ClearTaskUnit( pUnit );
		return;
	}
	// if unit has a patrol building, and is here with current
	// location different than destination, then it is still
	// enroute, so let it continue on
	else
	{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::PatrolArea() player %d unit %ld on patrol of %ld ", 
m_iPlayer, pUnit->GetID(), pUnit->GetParamDW(CAI_PATROL) );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"unit is at %d,%d and enroute to %d,%d \n",
hexVeh.X(), hexVeh.Y(), hexDest.X(), hexDest.Y() );
#endif
		return;
	}
}

//
// consider the passed task is an IDT_PREPAREWAR task
// that may have had its staging phase just competed
// so confirm by counting the types of units in the
// staging area, and consider a random fuzzy influence
// and return TRUE if close or on count, or FALSE if
// not close enough or not on count
//
//
//  IDT_PREPAREWAR - assemble a task force
//
// params will be used to:
// 0,1 - start of staging block
// 2,3 - end of staging block
//
// for IDG_ADVDEFENSE and IDG_LANDWAR versions of task
// 4 - how many "light_tank,med_tank,heavy_tank" in TF
// 5 - how many "infantry_carrier" in TF
// 6 - how many "light_art,med_art,heavy_art" in TF
// 7 - how many "infantry,ranger" in TF
//
// for IDG_PIRATE versions of task
// 4 - how many "cruiser"
// 5 - how many "destroyer"
//
// for IDG_SEAINVADE versions of task
// 4 - how many "gun_boat"
// 5 - how many "marines"
// 6 - how many "light_tank,med_tank,light_art"
// 7 - how many "landing_craft"
//
// bArea indicates that units must be in an area defined
// by staging area, otherwise just count those units that
// area assigned the task
//
BOOL CAITaskMgr::IsStagingCompete( CAITask *pTask, int iType /*=0*/ )
{	
	int iCounts[STAGING_UNITTYPES],iTaskCnt=0;
	for( int i=0; i<STAGING_UNITTYPES; ++i )
	{
		iCounts[i] = 0;
		// count units the task wants, sitting out in the upper end
		iTaskCnt += pTask->GetTaskParam( (i+STAGING_UNITTYPES) );
	}
	// a zeroed taskforce unit count means that the taskforce
	// cannot be staged, or has not be initialized yet
	// 
	if( !iTaskCnt )
	{
		if( !pTask->GetTaskParam(CAI_LOC_X) &&
			!pTask->GetTaskParam(CAI_LOC_Y) )
			return FALSE;

		// can't be staged for some reason
		return TRUE;
	}

	// FUZZY LAUNCH IS TURNED OFF, NOW MUST STAGE COMPLETELY
	//
	// first, randomly decide if this is a finite
	// or fuzzy decision.  if it is a fuzzy decision
	// then staging will be considered complete if
	// something over 1/2 the desired units are staged
	/*
	BOOL bIsFuzzy = FALSE;
	i = pGameData->GetRandom(NUM_DIFFICUTY_LEVELS);
	if( i == pGameData->m_iSmart )
		bIsFuzzy = TRUE;

	if( bIsFuzzy )
	{
		i = (iTaskCnt/2) + (pGameData->GetRandom((iTaskCnt/2)));
		iTaskCnt = i;
	}
	*/

	// BUGBUG force 4 units for testing
	// iTaskCnt = 4;

	// get staging bounds
	CHexCoord hcFrom( 
		pTask->GetTaskParam(CAI_LOC_X),
		pTask->GetTaskParam(CAI_LOC_Y) );
	CHexCoord hcTo( 
		pTask->GetTaskParam(CAI_PREV_X),
		pTask->GetTaskParam(CAI_PREV_Y) );
	CHexCoord hex,hexDest;

#if THREADS_ENABLED
			// BUGBUG this function must yield
	myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
	// now go thru units list, and for those units assigned 
	// to stage, get location and determine if they are in
	// the staging area
   	POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// skip own units not assigned to stage
			if( pUnit->GetTask() != pTask->GetID() ||
				pUnit->GetGoal() != pTask->GetGoalID() )
				continue;

			if( !iType )
			{
				EnterCriticalSection (&cs);
				CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
				if( pVehicle == NULL )
				{
					LeaveCriticalSection (&cs);
					continue;
				}
				hex = pVehicle->GetHexHead();
				hexDest = pVehicle->GetHexDest();
				LeaveCriticalSection (&cs);

				// the unit is awaiting a load or to be loaded
				if( pUnit->GetStatus() & CAI_IN_USE )
					continue;

			// unit not arriving at destination within area yet
			// could mean that it is trying to load another unit
			// or be loaded by another unit
				if( hex != hexDest )
					continue;

			// consider the unit based on it being in an area
			//BOOL CAIMapUtil::IsHexInArea( 
			//CHexCoord& hcStart, CHexCoord& hcEnd, CHexCoord& hex )
				if( !m_pGoalMgr->m_pMap->m_pMapUtil->IsHexInArea( 
					hcFrom, hcTo, hex) )
					continue;
			}

			// now, based on the type of assault that will be
			// conducted, count up the types of units staged
			if( pTask->GetGoalID() == IDG_ADVDEFENSE ||
				pTask->GetGoalID() == IDG_LANDWAR )
			{
				switch( pUnit->GetTypeUnit() )
				{
					//case CTransportData::light_scout: // light_scouts can't shoot
					//case CTransportData::med_scout:	// med_scouts patrol only
					case CTransportData::heavy_scout:
					case CTransportData::light_tank:
					case CTransportData::med_tank:
					case CTransportData::heavy_tank:
						iCounts[0] += 1;
						break;
					case CTransportData::infantry_carrier:
						iCounts[1] += 1;
						break;
					case CTransportData::light_art:
					case CTransportData::med_art:
					case CTransportData::heavy_art:
						iCounts[2] += 1;
						break;
					case CTransportData::infantry:
					case CTransportData::rangers:
						iCounts[3] += 1;
						break;
					default:
						break;
				}
			}
			else if( pTask->GetGoalID() == IDG_PIRATE ||
				pTask->GetGoalID() == IDG_SEAWAR )
			{
				switch( pUnit->GetTypeUnit() )
				{
					case CTransportData::cruiser:
						iCounts[0] += 1;
						break;
					case CTransportData::destroyer:
						iCounts[1] += 1;
						break;
					default:
						break;
				}
			}
			else if(pTask->GetGoalID() == IDG_SEAINVADE )
			{
				switch( pUnit->GetTypeUnit() )
				{
					case CTransportData::light_tank:
					case CTransportData::med_tank:
					case CTransportData::med_art:
						iCounts[0] += 1;
						break;
					case CTransportData::landing_craft:
						iCounts[1] += 1;
						break;
					case CTransportData::gun_boat:
						iCounts[2] += 1;
						break;
					case CTransportData::rangers:
						iCounts[3] += 1;
						break;
					default:
						break;
				}
			}
		}
	}		// end of counting all units

	// if just checking counts of assigned units then consider
	// that if a staging type has been completed a TRUE is returned
	if( iType )
	{
		int iTypeChecking = STAGING_UNITTYPES;

			if( pTask->GetGoalID() == IDG_ADVDEFENSE ||
				pTask->GetGoalID() == IDG_LANDWAR )
			{
				switch( iType )
				{
					//case CTransportData::light_scout: // light_scouts can't shoot
					// case CTransportData::med_scout:
					case CTransportData::heavy_scout:
					case CTransportData::light_tank:
					case CTransportData::med_tank:
					case CTransportData::heavy_tank:
						iTypeChecking = 0;
						break;
					case CTransportData::infantry_carrier:
						iTypeChecking = 1;
						break;
					case CTransportData::light_art:
					case CTransportData::med_art:
					case CTransportData::heavy_art:
						iTypeChecking = 2;
						break;
					case CTransportData::infantry:
					case CTransportData::rangers:
						iTypeChecking = 3;
						break;
					default:
						break;
				}
			}
			else if( pTask->GetGoalID() == IDG_PIRATE ||
				pTask->GetGoalID() == IDG_SEAWAR )
			{
				switch( iType )
				{
					case CTransportData::cruiser:
						iTypeChecking = 0;
						break;
					case CTransportData::destroyer:
						iTypeChecking = 1;
						break;
					default:
						break;
				}
			}
			else if(pTask->GetGoalID() == IDG_SEAINVADE )
			{
				switch( iType )
				{
					case CTransportData::light_tank:
					case CTransportData::med_tank:
					case CTransportData::med_art:
						iTypeChecking = 0;
						break;
					case CTransportData::landing_craft:
						iTypeChecking = 1;
						break;
					case CTransportData::gun_boat:
						iTypeChecking = 2;
						break;
					case CTransportData::rangers:
						iTypeChecking = 3;
						break;
					default:
						break;
				}
			}

			// has this staging type, been all completed
			if( iTypeChecking < STAGING_UNITTYPES )
			{
				if( iCounts[iTypeChecking] >=
				pTask->GetTaskParam( (iTypeChecking+STAGING_UNITTYPES) ))
					return TRUE;
			}
	}

	// don't even consider completion, unless there is at least 1 of
	// each staging unit types to be staged, that has staged
	for( i=0; i<STAGING_UNITTYPES; ++i )
	{
		if( pTask->GetTaskParam( (i+STAGING_UNITTYPES) ) && !iCounts[i] )
		{
			return FALSE;
		}
	}

	// then, using the fuzzy/finite decision factor
	// from above, determine if staging is completed
	int iCnt = 0;
	for( i=0; i<STAGING_UNITTYPES; ++i )
		iCnt += iCounts[i];
	if( iCnt >= iTaskCnt )
		return TRUE;

#ifdef _LOGOUT

logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::IsStagingCompete() player %d task %d goal %d ", 
m_iPlayer, pTask->GetID(), pTask->GetGoalID() );

if( pTask->GetGoalID() == IDG_ADVDEFENSE ||
	pTask->GetGoalID() == IDG_LANDWAR )
{
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"needs AFV=%d  IFV=%d  ART=%d  INF=%d  iTaskCnt=%d ",
pTask->GetTaskParam(CAI_TF_TANKS),pTask->GetTaskParam(CAI_TF_IFVS),
pTask->GetTaskParam(CAI_TF_ARTILLERY),pTask->GetTaskParam(CAI_TF_INFANTRY),
iTaskCnt );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"has AFV=%d  IFV=%d  ART=%d  INF=%d ",
iCounts[0],iCounts[1],iCounts[2],iCounts[3] );
}
else if( pTask->GetGoalID() == IDG_SEAINVADE )
{
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"needs SHIPS=%d  MARINE=%d  ARMOR=%d  LC=%d  iTaskCnt=%d ",
pTask->GetTaskParam(CAI_TF_SHIPS),pTask->GetTaskParam(CAI_TF_MARINES),
pTask->GetTaskParam(CAI_TF_ARMOR),pTask->GetTaskParam(CAI_TF_LANDING),
iTaskCnt );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"has SHIPS=%d  MARINE=%d  ARMOR=%d  LC=%d ",
iCounts[2],iCounts[3],iCounts[0],iCounts[1] );
}
else if( pTask->GetGoalID() == IDG_PIRATE )
{
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"needs CRUISERS=%d  DESTROYERS=%d  iTaskCnt=%d ",
pTask->GetTaskParam(CAI_TF_CRUISERS),pTask->GetTaskParam(CAI_TF_DESTROYERS),
iTaskCnt );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"has CRUISERS=%d  DESTROYERS=%d ",iCounts[0],iCounts[1] );
}
#endif

	return FALSE;
}

BOOL CAITaskMgr::ContinueStaging( CAIUnit *pStagingVeh, CAITask *pTask )
{
	// first consider that the task has not been initialized
	WORD wCnt = 0;
	for( int i=0; i<STAGING_UNITTYPES; ++i )
	{
		wCnt +=
			pTask->GetTaskParam((i+STAGING_UNITTYPES));
	}
	if( !wCnt )
		return TRUE;

	
	// only four types of units are used in a staging, which vary
	// depending on the task that is staged
	//int iMaxTypes = 4;
	int iStagingType = STAGING_UNITTYPES;
	// based on the type of assault that will be
	// conducted, determine if this unit is to be staged
	if( pTask->GetGoalID() == IDG_ADVDEFENSE ||
		pTask->GetGoalID() == IDG_LANDWAR )
	{
		switch( pStagingVeh->GetTypeUnit() )
		{
			//case CTransportData::light_scout: // light_scouts can't shoot
			//case CTransportData::med_scout:
			case CTransportData::heavy_scout:
			case CTransportData::light_tank:
			case CTransportData::med_tank:
			case CTransportData::heavy_tank:
				iStagingType=0;
				break;
			case CTransportData::infantry_carrier:
				iStagingType=1;
				break;
			case CTransportData::light_art:
			case CTransportData::med_art:
			case CTransportData::heavy_art:
				iStagingType=2;
				break;
			case CTransportData::infantry:
			case CTransportData::rangers:
				iStagingType=3;
				break;
				default:
				break;
		}
	}
	else if( pTask->GetGoalID() == IDG_PIRATE ||
		pTask->GetGoalID() == IDG_SEAWAR )
	{
		switch( pStagingVeh->GetTypeUnit() )
		{
			case CTransportData::cruiser:
				iStagingType=0;
				break;
			case CTransportData::destroyer:
				iStagingType=1;
				break;
			default:
				break;
		}
	}
	else if(pTask->GetGoalID() == IDG_SEAINVADE )
	{
		switch( pStagingVeh->GetTypeUnit() )
		{
			case CTransportData::light_tank:
			case CTransportData::med_tank:
			case CTransportData::med_art:
				iStagingType=0;
				break;
			case CTransportData::landing_craft:
				iStagingType=1;
				break;
			case CTransportData::gun_boat:
				iStagingType=2;
				break;
			case CTransportData::rangers:
				iStagingType=3;
				break;
			default:
				break;
		}
	}
	// this unit type is not a staging unit
	if( iStagingType == STAGING_UNITTYPES )
		return( FALSE );
	// this unit type is not used for staging by this task
	if( !pTask->GetTaskParam((iStagingType+STAGING_UNITTYPES)) )
		return( FALSE );

	// if still here, determine how many of each type of unit used for
	// staging this task, have been assigned to this task

	// declare and clear array to hold counts of units assigned to stage
	// with this task, by there type of unit
	int iCounts[STAGING_UNITTYPES];
	for( i=0; i<STAGING_UNITTYPES; ++i )
		iCounts[i] = 0;

#if THREADS_ENABLED
			// BUGBUG this function must yield
	myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
	// now go thru the units, assigned to this task for staging, and count
	// the units by unit type
   	POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// skip the staging unit passed in
			if( pUnit->GetID() == pStagingVeh->GetID() )
				continue;

			// skip own units not assigned to stage
			if( pUnit->GetTask() != pTask->GetID() ||
				pUnit->GetGoal() != pTask->GetGoalID() )
				continue;

			// now, based on the type of assault that will be
			// conducted, count up the types of units staged
			if( pTask->GetGoalID() == IDG_ADVDEFENSE ||
				pTask->GetGoalID() == IDG_LANDWAR )
			{
				switch( pUnit->GetTypeUnit() )
				{
					//case CTransportData::light_scout:
					//case CTransportData::med_scout:
					case CTransportData::heavy_scout:
					case CTransportData::light_tank:
					case CTransportData::med_tank:
					case CTransportData::heavy_tank:
						iCounts[0] += 1;
						break;
					case CTransportData::infantry_carrier:
						iCounts[1] += 1;
						break;
					case CTransportData::light_art:
					case CTransportData::med_art:
					case CTransportData::heavy_art:
						iCounts[2] += 1;
						break;
					case CTransportData::infantry:
					case CTransportData::rangers:
						iCounts[3] += 1;
						break;
					default:
						break;
				}
			}
			else if( pTask->GetGoalID() == IDG_PIRATE ||
				pTask->GetGoalID() == IDG_SEAWAR )
			{
				switch( pUnit->GetTypeUnit() )
				{
					case CTransportData::cruiser:
						iCounts[0] += 1;
						break;
					case CTransportData::destroyer:
						iCounts[1] += 1;
						break;
					default:
						break;
				}
			}
			else if(pTask->GetGoalID() == IDG_SEAINVADE )
			{
				switch( pUnit->GetTypeUnit() )
				{
					case CTransportData::light_tank:
					case CTransportData::med_tank:
					//case CTransportData::light_art:
					case CTransportData::med_art:
						iCounts[0] += 1;
						break;
					case CTransportData::landing_craft:
						iCounts[1] += 1;
						break;
					case CTransportData::gun_boat:
						iCounts[2] += 1;
						break;
					case CTransportData::rangers:
						iCounts[3] += 1;
						break;
					default:
						break;
				}
			}

		}
	}

	// now consider the count of units assigned to task already
	// and if this unit type count is greater than task needs
	// don't continue
	if( iCounts[iStagingType] >= 
		pTask->GetTaskParam((iStagingType+STAGING_UNITTYPES)) )
		return( FALSE );

	// continue staging this unit
	return( TRUE );
}

// this staging task could be started with some
// of the existing patrol units check all IDT_PATROL for
// a possible task reassignment
//
//
void CAITaskMgr::ConsiderPatrols( CAITask *pTask )
{
	// protect from a hang, because this function is called
	// by StageUnit() which in turn, on staging init will
	// call this function,
	if( !pTask->GetTaskParam(CAI_LOC_X) &&
		!pTask->GetTaskParam(CAI_LOC_Y) )
		return;

	// add up pTask's staging unit needs
    // "AFV=%d  IFV=%d  ART=%d  INF=%d \n\n",
	// #define CAI_TF_TANKS		4
	// #define CAI_TF_IFVS		5
	// #define CAI_TF_ARTILLERY	6
	// #define CAI_TF_INFANTRY	7
	//
	int iStagingUnits = pTask->GetTaskParam(CAI_TF_TANKS);
	iStagingUnits += pTask->GetTaskParam(CAI_TF_IFVS);
	iStagingUnits += pTask->GetTaskParam(CAI_TF_ARTILLERY);
	iStagingUnits += pTask->GetTaskParam(CAI_TF_INFANTRY);

	// BUGBUG turn off random influence during testing
	// pick a random number less than needs
	//iStagingUnits = pGameData->GetRandom(iStagingUnits);
	//if( !iStagingUnits )
	//	iStagingUnits = 1;
	
	// allocate arrays for dwID and dist for possible reassignment
	DWORD *pdwIDs = NULL;
	int *piDist = NULL;
	try
	{
		pdwIDs = new DWORD[iStagingUnits];
		piDist = new int[iStagingUnits];
	}
	catch( CException e )
	{
		if( pdwIDs != NULL )
			delete [] pdwIDs;
		if( piDist != NULL )
			delete [] piDist;

		throw(ERR_CAI_BAD_NEW);
	}
	for( int i=0; i<iStagingUnits; ++i )
	{
		pdwIDs[i] = (DWORD)0;
		piDist[i] = 0xFFFE;
	}

	// get defining hexes of the staging area
	CHexCoord hexStart( 
		pTask->GetTaskParam(CAI_LOC_X), pTask->GetTaskParam(CAI_LOC_Y) );
	CHexCoord hexEnd( 
		pTask->GetTaskParam(CAI_PREV_X), pTask->GetTaskParam(CAI_PREV_Y) );
	CHexCoord hexUnit;

#if THREADS_ENABLED
			// BUGBUG this function must yield
	myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	int iPatrolUnits = 0, iDist;
	// if less than number of units on patrol
   	POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetTask() == IDT_PATROL )
			{
				// selectively, by the goal of the task,
				// skip some units on patrol
				if( pTask->GetGoalID() == IDG_ADVDEFENSE ||
					pTask->GetGoalID() == IDG_LANDWAR )
				{
					switch( pUnit->GetTypeUnit() )
					{
						case CTransportData::rangers:
						case CTransportData::gun_boat:
						case CTransportData::destroyer:
						case CTransportData::cruiser:
						case CTransportData::landing_craft:
							continue;
						default:
							break;
					}
					// consider that this staging does not need a type of vehicle
					// so skip them if applicable
					if( !pTask->GetTaskParam(CAI_TF_TANKS) )
					{
						switch( pUnit->GetTypeUnit() )
						{
						case CTransportData::light_tank:
						case CTransportData::med_tank:
						case CTransportData::heavy_tank:
						//case CTransportData::med_scout:
						case CTransportData::heavy_scout:
							continue;
						default:	// all other unit types go to next test
							break;
						}
					}
					if( !pTask->GetTaskParam(CAI_TF_ARTILLERY) )
					{
						switch( pUnit->GetTypeUnit() )
						{
						case CTransportData::light_art:
						case CTransportData::med_art:
						case CTransportData::heavy_art:
							continue;
						default:
							break;	// all other unit types go to next test
						}
					}
					if( !pTask->GetTaskParam(CAI_TF_INFANTRY) )
					{
						switch( pUnit->GetTypeUnit() )
						{
						case CTransportData::infantry:
						case CTransportData::rangers:
							continue;
						default:
							break;	// all other unit types go to next test
						}
					}
					if( !pTask->GetTaskParam(CAI_TF_IFVS) &&
						pUnit->GetTypeUnit() ==
						CTransportData::infantry_carrier )
						continue;
				}
				else if( pTask->GetGoalID() == IDG_PIRATE )
				{
					switch( pUnit->GetTypeUnit() )
					{
						case CTransportData::destroyer:
						case CTransportData::cruiser:
							break;
						default:
							continue;
					}
				}
				else if(pTask->GetGoalID() == IDG_SEAINVADE )
				{
	// CAI_TF_ARMOR   - 4 - how many "light_tank,med_tank,light_art"
	// CAI_TF_LANDING - 5 - how many "landing_craft"
	// CAI_TF_SHIPS   - 6 - how many "cruiser,destroyer,gun_boat"
	// CAI_TF_MARINES - 7 - how many "marines"
					if( !pTask->GetTaskParam(CAI_TF_SHIPS) )
					{
						if( pUnit->GetTypeUnit() == 
							CTransportData::gun_boat )
							continue;
					}
					if( !pTask->GetTaskParam(CAI_TF_MARINES) )
					{
						if( pUnit->GetTypeUnit() == 
							CTransportData::rangers )
							continue;
					}

					if( !pTask->GetTaskParam(CAI_TF_ARMOR) )
					{
						switch( pUnit->GetTypeUnit() )
						{
						case CTransportData::light_tank:
						case CTransportData::med_tank:
						//case CTransportData::light_art:
						case CTransportData::med_art:
							continue;
						default:
							break;	// all other unit types go to next test
						}
					}
					if( !pTask->GetTaskParam(CAI_TF_LANDING) &&
						pUnit->GetTypeUnit() ==
						CTransportData::landing_craft )
						continue;
				}


				// now should only be combat units on patrol
				// that are appropriate for this goal/task
				iPatrolUnits++;
				
				// get position of patroling unit
				EnterCriticalSection (&cs);
				CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
				if( pVehicle == NULL )
				{
					LeaveCriticalSection (&cs);
					continue;
				}
				hexUnit = pVehicle->GetHexHead();

				// this is a unit onboard a carrier type unit
				if( pVehicle->GetTransport() != NULL )
				{
					LeaveCriticalSection (&cs);
					continue;
				}
				LeaveCriticalSection (&cs);

				// get average of distances to start/end of stage area
				iDist = pGameData->GetRangeDistance( hexUnit, hexStart );
				iDist += pGameData->GetRangeDistance( hexUnit, hexEnd );
				iDist /= 2;

				// looking for those units with lowest dist value
				if( iDist )
				{
					// first, make sure array fills up
					BOOL bAdded = FALSE;
					for( i=0; i<iStagingUnits; ++i )
					{
						if( !pdwIDs[i] )
						{
							pdwIDs[i] = pUnit->GetID();
							piDist[i] = iDist;
							bAdded = TRUE;
							break;
						}
					}
					if( bAdded )
						continue;

					// check all saved distances as compared to this dist
					// and replace saved if this is closer
					for( i=0; i<iStagingUnits; ++i )
					{
						if( iDist < piDist[i] )
						{
							pdwIDs[i] = pUnit->GetID();
							piDist[i] = iDist;
							break;
						}
					}
				}
			}
		}
	}

	// not enough units on patrol to take any
	if( iPatrolUnits <= iStagingUnits )
	{
		if( pdwIDs != NULL )
			delete [] pdwIDs;
		if( piDist != NULL )
			delete [] piDist;
		return;
	}
	
	// units in arrays can be reassigned to staging task
	for( i=0; i<iStagingUnits; ++i )
	{
		if( pdwIDs[i] )
		{
			CAIUnit *pUnit = m_pGoalMgr->m_plUnits->GetUnit( pdwIDs[i] );
			if( pUnit != NULL )
			{
				ClearTaskUnit( pUnit );

				pUnit->SetGoal( pTask->GetGoalID() );
				pUnit->SetTask( pTask->GetID() );

				WORD wStatus = pUnit->GetStatus();
				if( wStatus & CAI_IN_USE )
				{
					wStatus ^= CAI_IN_USE;
					pUnit->SetStatus(wStatus);
				}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::ConsiderPatrols() player %d unit %ld reassigned to task %d goal %d \n", 
pUnit->GetOwner(), pUnit->GetID(), pTask->GetID(), pTask->GetGoalID() );
#endif
				// the unit reassigned could be carrying other units
				// which need to be reassigned to the new task/goal
				SyncStageCargo( pUnit, pTask );

				StageUnit( pUnit, pTask );
			}
		}
	}
	
	// clean up
	if( pdwIDs != NULL )
		delete [] pdwIDs;
	if( piDist != NULL )
		delete [] piDist;
}

//
// go through the list of units with this task and set their
// destination to the location passed
//
void CAITaskMgr::SetTaskForceDestination( int iX, int iY, CAITask *pTask )
{
	CHexCoord hexDest(iX,iY);
	
	if( m_pGoalMgr->m_plUnits != NULL )
	{
#if THREADS_ENABLED
				// BUGBUG this function must yield
		myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIUnit *pUnit = 
            	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
            if( pUnit != NULL )
            {
            	ASSERT_VALID( pUnit );

				// unit has no task assigned
				// this should pick up new units
				// and units whose tasks are gone
				if( pUnit->GetTask() == pTask->GetID() &&
					pUnit->GetGoal() == pTask->GetGoalID() )
				{
					pUnit->SetDestination( hexDest );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::SetTaskForceDestination() player %d unit %ld going to %d,%d ", 
m_iPlayer, pUnit->GetID(), hexDest.X(), hexDest.Y() );
#endif
				}
			}
		}
	}
}

//
// if the unit is not at its destination, then get it new one
//
void CAITaskMgr::ExploreMap( CAIUnit *pUnit, CAITask * )
{
	CHexCoord hexVeh(0,0);
	CHexCoord hexDest(0,0);

	EnterCriticalSection (&cs);
	CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		ClearTaskUnit( pUnit );
		return;
	}
	hexVeh = pVehicle->GetHexHead();
	hexDest = pVehicle->GetHexDest();
	LeaveCriticalSection (&cs);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::ExploreMap() player %d unit %ld at %d,%d and going to %d,%d ", 
m_iPlayer, pUnit->GetID(), hexVeh.X(), hexVeh.Y(), hexDest.X(), hexDest.Y() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::ExploreMap() last destination was %d,%d  with explore flag %d \n", 
pUnit->GetParam(CAI_DEST_X),pUnit->GetParam(CAI_DEST_Y),pUnit->GetParam(CAI_FUEL) );
#endif

	if( hexVeh != hexDest )
	{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::ExploreMap() player %d unit %ld enroute to %d,%d \n", 
m_iPlayer, pUnit->GetID(), hexDest.X(), hexDest.Y() );
#endif	
		return;
	}
	else // hexVeh == hexDest
	{
		// if unit has an exploration destination and that matches hexDest
		// then the unit arrived at its exploration destination
		if( pUnit->GetParam(CAI_DEST_X) || pUnit->GetParam(CAI_DEST_Y) )
		{
			if( pUnit->GetParam(CAI_DEST_X) == hexDest.X() &&
				pUnit->GetParam(CAI_DEST_Y) == hexDest.Y() )
			{
				pUnit->SetParam(CAI_DEST_X, 0 );
				pUnit->SetParam(CAI_DEST_Y, 0 );
				pUnit->SetParam(CAI_FUEL, 0 );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::ExploreMap() player %d unit %ld destination cleared %d,%d \n", 
m_iPlayer, pUnit->GetID(), pUnit->GetParam(CAI_DEST_X), pUnit->GetParam(CAI_DEST_Y) );
#endif	
			}
			else
			{
				// what if hexDest is not the exploration destination? 
				// and is instead a vehicle error redirction destination
				// or if the game has not yet processed the veh_goto
				//if( !pUnit->GetParam(CAI_FUEL) )
				//{
				hexDest.X(pUnit->GetParam(CAI_DEST_X));
				hexDest.Y(pUnit->GetParam(CAI_DEST_Y));
				
				// put in a limit so that the vehicle resets after
				// trying to get to the same destination and failing
				WORD wTries = pUnit->GetParam(CAI_FUEL) + 1;
				pUnit->SetParam(CAI_FUEL,wTries);

				if( wTries < CAI_FUEL )
				{
					pUnit->SetDestination( hexDest );
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::ExploreMap() player %d unit %ld going to %d,%d again\n", 
m_iPlayer, pUnit->GetID(), hexDest.X(), hexDest.Y() );
#endif	
					return;
				}
				else
				{
				pUnit->SetParam(CAI_DEST_X, 0 );
				pUnit->SetParam(CAI_DEST_Y, 0 );
				pUnit->SetParam(CAI_FUEL, 0 );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::ExploreMap() player %d unit %ld destination reset %d,%d \n", 
m_iPlayer, pUnit->GetID(), pUnit->GetParam(CAI_DEST_X), pUnit->GetParam(CAI_DEST_Y) );
#endif	
				}
			}
		}
		// else this is just another time thru while waiting for
		// the game to record the destination requested last time
	}

	// both must be zero to indicate a new destination is needed
	if( !pUnit->GetParam(CAI_DEST_X) && !pUnit->GetParam(CAI_DEST_Y) )
	{
		m_pGoalMgr->m_pMap->m_pMapUtil->GetExploreHex( 
			pUnit, hexDest, hexVeh );

		pUnit->SetDestination( hexDest );
		pUnit->SetParam(CAI_DEST_X, hexDest.X() );
		pUnit->SetParam(CAI_DEST_Y, hexDest.Y() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::ExploreMap() player %d unit %ld going to %d,%d \n", 
m_iPlayer, pUnit->GetID(), hexDest.X(), hexDest.Y() );
#endif	
	}
}


//
// find the highest rated building in need of repair and direct
// the crane to it, and upon arrival, issue CMsgRepairBldg message
// to begin repairing the building
//
BOOL CAITaskMgr::RepairConstruction( CAIUnit *pUnit )
{
	CHexCoord hexBldg(0,0);
	CHexCoord hexVeh(0,0);
	CHexCoord hexDest(0,0);
	CHexCoord hexTail(0,0);
	CSubHex shTail;

	// get game data
	EnterCriticalSection (&cs);
	CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
	if( pVehicle != NULL )
	{
		hexVeh = pVehicle->GetHexHead();
		shTail = pVehicle->GetPtTail();
		hexTail = shTail.ToCoord();
		hexDest = pVehicle->GetHexDest();
	}
	LeaveCriticalSection (&cs);

	// 0,0 is an invalid location
	if( !hexVeh.X() && !hexVeh.Y() )
		return FALSE;

	// a building to repair has already been selected
	if( pUnit->GetDataDW() )
	{
		int iDmgPer = 0;

		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetDataDW() );
		if( pBldg != NULL )
		{
			hexBldg = pBldg->GetExitHex();
			iDmgPer = pBldg->GetDamagePer();
		}
		LeaveCriticalSection (&cs);

		// building is fully repaired
		if( iDmgPer == DAMAGE_0 )
			goto DismissUnit;

		// crane is already at the building
		if( hexVeh == hexBldg && hexTail == hexBldg )
		{
			// unit has already sent message to repair building
			if( pUnit->GetParam(CAI_FUEL) )
				return TRUE;
			
			// send CMsgBuildBldg message
			pUnit->RepairBuilding();

			// flag unit has having sent message
			pUnit->SetParam(CAI_FUEL,1);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"RepairConstruction() plyr %d crane %ld CMsgRepairBldg sent for bldg %ld ", 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetDataDW() );
#endif
			return TRUE;
		}
		else 
		{
			// crane is already enroute to building
			if( hexBldg == hexDest )
				return TRUE;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"RepairConstruction() plyr %d crane %ld going to bldg %ld at %d,%d ", 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetDataDW(), hexBldg.X(), hexBldg.Y() );
#endif
			// send the crane to the building
			pUnit->SetDestination( hexBldg );
			return TRUE;
		}
	}
	else // need to select a building to repair
	{
		int iBestRating = 0, iRating;
		CAIUnit *paiBldg = NULL;

#if THREADS_ENABLED
				// BUGBUG this function must yield
		myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

		// go thru buildings, looking for buildings in need of repair
        POSITION posB = m_pGoalMgr->m_plUnits->GetHeadPosition();
        while( posB != NULL )
        {   
            CAIUnit *pUnitB = 
            	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( posB );
            if( pUnitB != NULL )
            {
            	ASSERT_VALID( pUnitB );

				if( pUnitB->GetOwner() != m_iPlayer )
					continue;
				// only buildings are desired
				if( pUnitB->GetType() != CUnit::building )
					continue;

				BOOL bNeedRepair = FALSE;
				EnterCriticalSection (&cs);
				CBuilding *pBldg = 
					theBuildingMap.GetBldg( pUnitB->GetID() );
				if( pBldg != NULL )
				{
					if( pBldg->GetDamagePer() < DAMAGE_0 )
					{
						bNeedRepair = TRUE;
						// check for delete in progress
						if( pBldg->GetFlags() & CUnit::dying )
							bNeedRepair = FALSE;

						hexBldg = pBldg->GetExitHex();

						// rate building's importance
						iRating = 
						m_pGoalMgr->m_pMap->m_pMapUtil->AssessTarget(pBldg,0);
					}
				}
				LeaveCriticalSection (&cs);

				if( !bNeedRepair )
					continue;

				// always pick the more valuable
				if( iRating > iBestRating )
				{
					iBestRating = iRating;
					paiBldg = pUnitB;
				}
			}
		}

		// selected a building to repair
		if( paiBldg != NULL )
		{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"RepairConstruction() plyr %d crane %ld going to bldg %ld at %d,%d ", 
pUnit->GetOwner(), pUnit->GetID(), paiBldg->GetID(), hexBldg.X(), hexBldg.Y() );
#endif
			pUnit->SetDataDW( paiBldg->GetID() );
			pUnit->SetDestination( hexBldg );
			return TRUE;
		}
		// no building need repair

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::RepairConstruction() player %d has no damaged buildings ", 
m_iPlayer );
#endif

	}

	DismissUnit:
	// find staging hex to move crane to
	//m_pGoalMgr->m_pMap->m_pMapUtil->FindStagingHex( 
	//	hexVeh, 1, 1, pUnit->GetTypeUnit(), hexDest );
	m_pGoalMgr->m_pMap->GetStagingHex( 
		hexVeh,1,1,pUnit->GetTypeUnit(), hexDest );
	
	if( hexVeh != hexDest )
	{
		pUnit->SetDestination( hexDest );
		m_pGoalMgr->m_pMap->PlaceFakeVeh( hexDest, 
			pUnit->GetTypeUnit() );
	}
			
	// unassign crane from task
	ClearTaskUnit( pUnit );
	return FALSE;
}

//
// go find the planned road section nearest a power center
// and go to that location, and upon arrival, build a road
// as soon as needed materials are present
//
void CAITaskMgr::BuildRoad( CAIUnit *pUnit, CAITask *pTask )
{
	// get intended site for building road or bridge
	int iX = pUnit->GetParam(CAI_DEST_X);
	int iY = pUnit->GetParam(CAI_DEST_Y);
	
	CHexCoord hexSite(0,0);
	CHexCoord hexVeh, hexDest;

	// get game data
	EnterCriticalSection (&cs);
	CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
	if( pVehicle != NULL )
	{
		hexSite = pVehicle->GetHexHead();
		hexDest = pVehicle->GetHexDest();
	}
	LeaveCriticalSection (&cs);

	// something is wrong
	if( !hexSite.X() && !hexSite.Y() )
	{
		// clear unit of task/goal and its params
		ClearTaskUnit( pUnit );
		return;
	}
	// save vehicle's location
	hexVeh = hexSite;

	// truck has arrived at the road/bridge construction site
	if( hexSite.X() == iX && hexSite.Y() == iY )
	{
		// unit has already sent message to build road
		if( pUnit->GetParam(CAI_FUEL) == CNetCmd::road_new )
			return;

		// is this crane to build a road?
		if( pUnit->GetParam(CAI_FUEL) == CNetCmd::build_road )
		{
			// crane arrived to build a road but 
			// the player is out of gas
			// road building takes GAS_PER_ROAD (5 units gas per road hex)
			if( !m_pGoalMgr->IsGasAvailable() )
			{
				// if still here, there ain't nothing to do so 
				pTask->SetStatus( UNASSIGNED_TASK );

				// clear unit of task/goal and its params
				ClearTaskUnit( pUnit );

				// tell the goalmgr we have an idle crane
				m_pGoalMgr->IdleCrane();

				return;
			}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"RoadBuilding() at %d,%d ", hexSite.X(), hexSite.Y() );
#endif

			// send start road construction message
			pGameData->BuildRoadAt( hexSite, pUnit );
			// flag unit as having sent the message already for this hex
			pUnit->SetParam(CAI_FUEL,CNetCmd::road_new);
			return;
		}

		// unit has already sent message to build bridge
		if( pUnit->GetParam(CAI_FUEL) == CNetCmd::bridge_new )
			return;

		// no it is assigned to build a bridge instead
		if( pUnit->GetParam(CAI_FUEL) == CNetCmd::build_bridge )
		{
			CHexCoord hexStart(
			pUnit->GetParam(CAI_PREV_X),pUnit->GetParam(CAI_PREV_Y) );
			CHexCoord hexEnd(
			pUnit->GetParam(CAI_DEST_X),pUnit->GetParam(CAI_DEST_Y) );

			// send start bridge building message
			pGameData->BuildBridgeAt( pUnit, hexStart, hexEnd );
			// flag unit as having sent the message already for this hex
			pUnit->SetParam(CAI_FUEL,CNetCmd::bridge_new);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"BridgeBuilding() from %d,%d to %d,%d ", 
hexStart.X(), hexStart.Y(), hexEnd.X(), hexEnd.Y() );
#endif

			return;
		}
	}
	else if( !iX && !iY ) // need to go to a build site
	{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"RoadBuilding() looking for building to repair" );
#endif
		// at the start of each search for a road build site,
		// check to see if any buildings need to be repaired
		if( RepairConstruction( pUnit ) )
		{
			// switch to repair buildings, because building(s) are damaged
			pUnit->SetTask( IDT_REPAIR );

			return;
		}
		// reassign unit to road/bridge/repair construction
		pUnit->SetGoal( IDG_ESTABLISH );
		pUnit->SetTask( IDT_CONSTRUCT );
		// get goal manager to tell us where to go
		//
		m_pGoalMgr->m_pMap->GetRoadHex( hexSite );

		// if the site returned is the current location of 
		// the truck then no site was found, so return
		if( hexSite.X() == hexVeh.X() &&
			hexSite.Y() == hexVeh.Y() )
		{

			// see if there is a bridge that needs to be built 
			//
			// BUGBUG turned off
#if 0
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"RoadBuilding() looking for bridge to build " );
#endif
			m_pGoalMgr->m_pMap->GetBridgingHexes( hexSite, pUnit );

			// a different hex returned means we found a bridge site
			// and its start/end are stored in pUnit->GetParam()
			if( hexSite.X() != hexVeh.X() ||
				hexSite.Y() != hexVeh.Y() )
			{
				// send truck to build site for one end
				pUnit->SetDestination( hexSite );
				// flag unit to send message to build a bridge
				pUnit->SetParam(CAI_FUEL,CNetCmd::build_bridge);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"BridgeBuilding() go to %d,%d ", hexSite.X(), hexSite.Y() );
#endif
				return;
			}
#endif

			// if still here, there ain't nothing to do so 
			pTask->SetStatus( UNASSIGNED_TASK );

			// clear unit of task/goal and its params
			ClearTaskUnit( pUnit );

			// no road was found so we must be out of roads
			m_pGoalMgr->m_pMap->m_iRoadCount = 0;
			// tell the goalmgr we have an idle crane
			m_pGoalMgr->IdleCrane();
			m_pGoalMgr->ConsiderRoads();

			// if crane is just sitting there, then make it move
			// over to nearby the rocket
			if( hexVeh == hexDest )
			{
				m_pGoalMgr->m_pMap->m_pMapUtil->FindStagingHex( 
					m_pGoalMgr->m_pMap->m_pMapUtil->m_RocketHex,
					4, 4, pUnit->GetTypeUnit(), hexDest );
				m_pGoalMgr->m_pMap->PlaceFakeVeh( hexDest, 
					pUnit->GetTypeUnit() );
				pUnit->SetDestination( hexDest );
			}
			return;
		}

		// road building takes GAS_PER_ROAD (5 units gas per road hex)
		if( !m_pGoalMgr->m_iGasHave )
		{
			// if still here, there ain't nothing to do so 
			pTask->SetStatus( UNASSIGNED_TASK );

			// clear unit of task/goal and its params
			ClearTaskUnit( pUnit );

			// tell the goalmgr we have an idle crane
			m_pGoalMgr->IdleCrane();

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"RoadBuilding() %d/%ld crane can't build road, out of gas", 
pUnit->GetOwner(), pUnit->GetID() );
#endif
			return;
		}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"RoadBuilding() go to %d,%d ", hexSite.X(), hexSite.Y() );
#endif
		// so send message to vehicle to goto adjacent hex and
		// update the task assigned to this construction truck
		// to reflect the hex already selected to build in so
		// when it arrives the CAIMgr will tell it to build this building
		//pTask->SetTaskParam(BUILD_AT_X, hexSite.X() );
		//pTask->SetTaskParam(BUILD_AT_Y, hexSite.Y() );
		pUnit->SetParam(CAI_DEST_X, hexSite.X() );
		pUnit->SetParam(CAI_DEST_Y, hexSite.Y() );
		// CAI_PREV_X
		
		// send truck to build site
		pUnit->SetDestination( hexSite );
		// flag unit to send message to build a road
		pUnit->SetParam(CAI_FUEL,CNetCmd::build_road);
	}
	else // vehicle is not at site but a site was selected
	{
		hexSite.X( pUnit->GetParam(CAI_DEST_X) );
		hexSite.Y( pUnit->GetParam(CAI_DEST_Y) );

		pUnit->SetDestination( hexSite );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"RoadBuilding() go to %d,%d again ", hexSite.X(), hexSite.Y() );
#endif
	}
}

//
// this function provides a construct a building order for the passed
// pUnit which has been assigned to pTask
//
void CAITaskMgr::ConstructBuilding( CAIUnit *pUnit, CAITask *pTask )
{
	CHexCoord hexVeh(0,0);
	CHexCoord hexDest(0,0);
	CHexCoord hexTail(0,0);
	CSubHex shTail;
	BOOL bIsMoving = FALSE;

	// get game data
	EnterCriticalSection (&cs);
	CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
	if( pVehicle != NULL )
	{
		hexVeh = pVehicle->GetHexHead();
		shTail = pVehicle->GetPtTail();
		hexTail = shTail.ToCoord();
		hexDest = pVehicle->GetHexDest();
		bIsMoving = (pVehicle->GetRouteMode() == CVehicle::moving) ? TRUE : FALSE;
	}
	LeaveCriticalSection (&cs);

	// 0,0 is an invalid location
	if( !hexVeh.X() && !hexVeh.Y() )
		return;

	//
	// get id of building to build from the task
	int iBldg = (int)pTask->GetTaskParam(BUILDING_ID);
	CHexCoord hexSite(0,0);
	hexSite.X(pTask->GetTaskParam(BUILD_AT_X));
	hexSite.Y(pTask->GetTaskParam(BUILD_AT_Y));

	// PREPARE for crane needing to be adjacent to build site
	// and not on the hex-of-the-building 
	CHexCoord hexCrane( 
		pUnit->GetParam(CAI_DEST_X),
		pUnit->GetParam(CAI_DEST_Y) );
			
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nConstructBuilding(): player %d crane %ld at %d,%d  hexSite=%d,%d  hexCrane=%d,%d",
pUnit->GetOwner(), pUnit->GetID(), hexVeh.X(), hexVeh.Y(), 
hexSite.X(), hexSite.Y(), hexCrane.X(), hexCrane.Y() );
#endif

	// arrived at the build site
	//if( hexSite == hexVeh && hexSite == hexTail )
	if( hexCrane == hexVeh && hexCrane == hexTail )
	{
		// send build message, once and only once
		if( pTask->GetStatus() == COMPLETED_TASK )
			return;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"ConstructBuilding(): player %d crane %ld On Site for a %d at %d,%d \n", 
		pUnit->GetOwner(), pUnit->GetID(), iBldg, 
		hexSite.X(), hexSite.Y() );
#endif
		// consider type of building, and adjust default direction of exit
		int iDir = 4;
		if( iBldg == CStructureData::seaport ||
			iBldg == CStructureData::shipyard_1 ||
			iBldg == CStructureData::shipyard_3 )
		{
			int iWhy = 0;
			EnterCriticalSection (&cs);
			pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
			if( pVehicle != NULL )
			{
				for( int i=0; i<4; ++i )
				{
				// use the game's opinion of building this building
				if( theMap.FoundationCost(hexSite, 
					iBldg, i, (CVehicle const *)pVehicle, NULL, &iWhy) < 0 )
					continue;

				iDir = i;
				break;
				}
			}
			LeaveCriticalSection (&cs);
			
			if( iDir == 4 )
			{
				ClearTaskUnit( pUnit );
				UnAssignTask( pTask->GetID(), pTask->GetGoalID() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"ConstructBuilding(): player %d could not find site for a %d ", 
m_iPlayer, iBldg );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"ConstructBuilding(): due to FoundationCost() last failure was %d ", iWhy );
#endif
				return;
			}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"ConstructBuilding(): FoundationCost() direction %d at %d,%d for shipyard/seaport", 
iDir, hexSite.X(), hexSite.Y() );
#endif

		}
		else
		{
			iDir = 0;
			// lets slow the AI thread down some more by randomly
			// selecting an exit orientation for non-veh-exit buildings
			// HasVehExit
			CStructureData const *pBldgData = 
				pGameData->GetStructureData( iBldg );
			if( pBldgData != NULL )
			{
				if( !pBldgData->HasVehExit() )
				{
					// square buildings
					if( pBldgData->GetCX() == 
						pBldgData->GetCY() )
					{
						iDir = pGameData->GetRandom(3);
					}
					else // non-square buildings
					{
						iDir = pGameData->GetRandom(1) * 2;
					}
				}
			}
		}


		// tell game to send message to truck to build now
		pGameData->BuildAt( hexSite, iBldg, iDir, pUnit );
		// flag it as completed so we don't resend the build message
		pTask->SetStatus( COMPLETED_TASK );
		// set exit orientation for future usage in error recovery
		pTask->SetTaskParam(BUILDING_EXIT, iDir );
		// flag unit as having sent one message, in case we get
		// a recoverable build error message later
		pUnit->SetParam(CAI_EFFECTIVE, 1 );

		// reset task occurs in response to the build completed
		// message that comes when it is done and is handled
		// by the CAIMgr which will call UnAssignTask()
	}
	else // need to get the truck to go to the build site
	{
		// need to find a place to build on
		if( !hexSite.X() && !hexSite.Y() )
		{
			hexSite = hexVeh;
			m_pGoalMgr->m_pMap->GetBuildHex( iBldg, hexSite );

			if( hexSite == hexVeh )
			{
				hexSite.X(0);
				hexSite.Y(0);
			}
		}

		// if the site returned is the current location of the
		// truck then no site was found, so unassign and return
		//if( hexSite == hexVeh )
		if( !hexSite.X() && !hexSite.Y() )
		{
			//pUnit->SetTask( FALSE );
			//pUnit->SetGoal( FALSE );
			ClearTaskUnit( pUnit );
			UnAssignTask( pTask->GetID(), pTask->GetGoalID() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"ConstructBuilding(): player %d could not find site for a %d. ", 
m_iPlayer, iBldg );
#endif
			return;
		}

		// at this point, a build site has not been reached, but the
		// flow returned to here, so if dest is not the build site
		// then setdestination() again, otherwise if still on that
		// destination, then let it go?



		// 
		// may not be able to reach the site?
		if( hexVeh != hexSite &&
			!m_pGoalMgr->m_pMap->m_pMapUtil->GetPathRating(hexVeh, hexSite) )
		{
			ClearTaskUnit( pUnit );
			UnAssignTask( pTask->GetID(), pTask->GetGoalID() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"ConstructBuilding(): player %d unit %ld could not reach site for a %d. ", 
m_iPlayer, pUnit->GetID(), iBldg ); 
#endif
			return;
		}

		// PREPARE for crane needing to be on adjacent hex to build site
		// m_pGoalMgr->m_pMap->GetCraneHex( hexSite, hexCrane );
		// remove next line when adjacent hex requirement is turned on
		hexCrane = hexSite;

		// unit is already going to build site
		if( hexDest == hexCrane )
		{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"ConstructBuilding(): player/unit %d/%ld, hexCrane %d,%d hexDest %d,%d ", 
		pUnit->GetOwner(), pUnit->GetID(), hexCrane.X(), hexCrane.Y(),
		hexDest.X(), hexDest.Y() );
#endif
			return;
		}

		// truck is already enroute to build site, but not there yet
		// also, this fires the first time through
		if( hexCrane != hexVeh )
		{
			// and only if the crane has been directed at least once
			if( pUnit->GetParam(CAI_DEST_X) || pUnit->GetParam(CAI_DEST_Y) )
			{
				// build_bldg message has already been sent, so stay here
				if( pUnit->GetParam(CAI_EFFECTIVE) )
					return;

				// crane is still moving and enroute to site
				if( bIsMoving &&
					hexDest == hexCrane )
					return;

				// check destination to make sure its still open
				CHex *pHex = theMap.GetHex( hexDest );
				if( pHex == NULL || (pHex->GetUnits() & CHex::bldg) )
				{
					ClearTaskUnit( pUnit );
					UnAssignTask( pTask->GetID(), pTask->GetGoalID() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"ConstructBuilding(): player %d unit %ld site for a %d has another building ", 
m_iPlayer, pUnit->GetID(), iBldg ); 
#endif
					return;
				}

				DWORD dwNow = theGame.GettimeGetTime();
				DWORD dwUnitTime = pUnit->GetParamDW(CAI_ROUTE_X); // last time
				if( !dwUnitTime )
					pUnit->SetParamDW(CAI_ROUTE_X,dwNow);
				else
				{
					if( dwUnitTime > dwNow )
						pUnit->SetParamDW(CAI_ROUTE_X, dwNow);
					else
					{
						DWORD dwDiff = dwNow - dwUnitTime;
						if( dwDiff < 30000 )
							return;
					}
				}
				// in case the crane is stalled, send goto again
				pUnit->SetDestination( hexCrane );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"ConstructBuilding(): player/unit %d/%ld, Going To %d,%d again ", 
		pUnit->GetOwner(), pUnit->GetID(), hexCrane.X(), hexCrane.Y() );
#endif
				return;
			}
		}

		// truck is already on site
		if( hexCrane == hexVeh )
		{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"ConstructBuilding(): player/unit %d/%ld, hexCrane %d,%d hexVeh %d,%d ", 
pUnit->GetOwner(), pUnit->GetID(), hexCrane.X(), hexCrane.Y(),
hexVeh.X(), hexVeh.Y());
#endif
			return;
		}

		// so send message to vehicle to goto build hex and
		// update the task assigned to this construction truck
		// to reflect the hex already selected to build in so
		// when it arrives the CAIMgr will tell it to build this building
		pTask->SetTaskParam(BUILD_AT_X, hexSite.X() );
		pTask->SetTaskParam(BUILD_AT_Y, hexSite.Y() );

		// tell the crane to go to the construction site
		pUnit->SetDestination( hexCrane );
		pUnit->SetParam(BUILD_AT_X, hexSite.X() );
		pUnit->SetParam(BUILD_AT_Y, hexSite.Y() );
		// crane site should be adjacent to hexSite
		pUnit->SetParam(CAI_DEST_X, hexCrane.X() );
		pUnit->SetParam(CAI_DEST_Y, hexCrane.Y() );
		pUnit->SetParam(CAI_EFFECTIVE, 0 );

		pUnit->SetParamDW(CAI_ROUTE_X, 0 );
		pUnit->SetParamDW(CAI_ROUTE_Y, 0 );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"ConstructBuilding(): player %d build a %d, Go To %d,%d ", 
pUnit->GetOwner(), iBldg, hexCrane.X(), hexCrane.Y() );
#endif

	}
}

//
// determine if any opfor units (not of this player) are
// withing spotting range of the hex passed
//
BOOL CAITaskMgr::InRange( CHexCoord& hexVeh, int iSpotting )
{
	// this function will return 0 if opfors are in range
	if( !m_pGoalMgr->m_pMap->m_pMapUtil->IsOutOpforRange(
		hexVeh, iSpotting) )
		return TRUE;

	return FALSE;
}

//
// determine if the unit passed is within range of the hex
//
BOOL CAITaskMgr::InRange(CAIUnit *pUnit,  CHexCoord& hex)
{
	int iRange = 0;
	CHexCoord hexUnit;
	if( pUnit->GetType() == CUnit::vehicle )
	{
		EnterCriticalSection (&cs);
		CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
		if( pVehicle != NULL )
		{
			hexUnit = pVehicle->GetHexHead();
			iRange = pVehicle->GetRange();
		}
		else
			iRange = 0;
		LeaveCriticalSection (&cs);
	}
	else if( pUnit->GetType() == CUnit::building )
	{
		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
		if( pBldg != NULL )
		{
			//hexUnit = pBldg->GetHex();
			hexUnit = pBldg->GetExitHex();
			iRange = pBldg->GetRange();
		}
		else
			iRange = 0;
		LeaveCriticalSection (&cs);
	}
	else
		return FALSE;

	if( !iRange )
		return FALSE;

	// add influence of difficulty setting
	iRange += pGameData->m_iSmart;
	// get distance to target
	int iDist = pGameData->GetRangeDistance( hexUnit, hex );

	CHex *pGameHex = theMap.GetHex( hex );
	if( pGameHex == NULL )
		return FALSE;

	BYTE bUnits = pGameHex->GetUnits();
	// building at hex, means it could be exit hex, meaning that
	// adjacent hexes could be in range, while this one is not
	if( (bUnits & CHex::bldg) )
	{
		CHexCoord hexAdj = hex;
		// for each direction around building hex
		for( int i=0; i<MAX_ADJACENT; ++i )
		{
			switch(i)
			{
				case 0:	// north
					hexAdj.Ydec();
					break;
				case 1:	// northeast
					hexAdj.Ydec();
					hexAdj.Xinc();
					break;
				case 2:
					hexAdj.Xinc();
					break;
				case 3:
					hexAdj.Yinc();
					hexAdj.Xinc();
					break;
				case 4:
					hexAdj.Yinc();
					break;
				case 5:
					hexAdj.Yinc();
					hexAdj.Xdec();
					break;
				case 6:
					hexAdj.Xdec();
					break;
				case 7:
					hexAdj.Xdec();
					hexAdj.Ydec();
					break;
				default:
					return FALSE;
			}

			pGameHex = theMap.GetHex( hexAdj );
			bUnits = pGameHex->GetUnits();
			if( (bUnits & CHex::bldg) )
			{
				if( pGameData->GetRangeDistance(hexUnit,hexAdj)
					<= iRange )
					return TRUE;
			}

			hexAdj = hex;
		}
	}
	
	if( iDist <= iRange )
		return TRUE;

	return FALSE;
}

//
// determine the places that the unit can move to, to get into
// range of the hex passed, and pick one based on best defense
//
void CAITaskMgr::MoveToRange(CAIUnit *pUnit,  CHexCoord hex)
{
	// only vehicles can move
	if( pUnit->GetType() != CUnit::vehicle )
	{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAITaskMgr::MoveToRange() unit %ld player %d not vehicle  ", 
	pUnit->GetID(), pUnit->GetOwner() );
#endif
		return;
	}

    // initialize range to zero to use as early out later
	int iRange = 0;
	CHexCoord hexUnit,hexDest;

	CTransportData const *pVehData = NULL;
	BOOL bIsMoving = FALSE;

	EnterCriticalSection (&cs);
	CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
	if( pVehicle != NULL )
	{
		iRange = pVehicle->GetRange();
		if( iRange < 0 )
			iRange = 1;
		hexUnit = pVehicle->GetHexHead();
		hexDest = pVehicle->GetHexDest();
		pVehData = pVehicle->GetData();

		if( pVehicle->GetRouteMode() == CVehicle::moving )
			bIsMoving = TRUE;
	}
	LeaveCriticalSection (&cs);

	if( iRange < 1 )
	{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAITaskMgr::MoveToRange() unit %ld player %d has range %d  ", 
	pUnit->GetID(), pUnit->GetOwner(), iRange );
#endif
		return;
	}

	// 
	// reduce range to improve hits on difficulty
	if( pGameData->m_iSmart && iRange > 1 )
		iRange--;

	// move to the defensible hex within iRange
	//
	//void CAIMapUtil::FindDefenseHex( CHexCoord& hexAttacking, 
	//CHexCoord& hexDefending, int iWidth, int iHeight, 
	//CTransportData const *pVehData )
	CHexCoord hexRange = hexUnit;
	m_pGoalMgr->m_pMap->m_pMapUtil->FindDefenseHex( 
		hex, hexRange, iRange, iRange, pVehData );

	// unit may already be on the way to the 'in range' hex
	if( hexDest == hexRange &&
		bIsMoving )
	{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAITaskMgr::MoveToRange() unit %ld player %d already going to %d,%d ", 
	pUnit->GetID(), pUnit->GetOwner(), hexRange.X(), hexRange.Y() );
#endif
		return;
	}
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAITaskMgr::MoveToRange() unit %ld player %d going to %d,%d ", 
	pUnit->GetID(), pUnit->GetOwner(), hexRange.X(), hexRange.Y() );
#endif

	// move into range
	pUnit->SetDestination( hexRange );
}

//
// consider the unit passed, and try to load it or find units
// for it to load, relative to the task assigned
// 
void CAITaskMgr::LoadCargo( CAIUnit *pUnit, CAITask *pTask )
{
	if( pUnit->GetTypeUnit() == CTransportData::infantry_carrier )
	{
		// troop carrier has arrived in area, find unloaded troops
		// if carrier has room remaining, then move to them and
		// issue CMsgLoadCarrier message
		//LoadIFV(pUnit,pTask);
		return;
	}
	else if( pUnit->GetTypeUnit() == CTransportData::landing_craft )
	{
		// landing craft has arrived in area, find unloaded troops
		// if carrier has room remaining, then move to them and
		// issue CMsgLoadCarrier message
		LoadLandingCraft(pUnit,pTask);
		return;
	}
	else // only troops can be carried for land based staging
	{
		// except for sea borne assaults
		if( pTask->GetGoalID() == IDG_SEAINVADE )
		{
			if( //pUnit->GetTypeUnit() != CTransportData::light_tank &&
				pUnit->GetTypeUnit() != CTransportData::med_tank &&
				//pUnit->GetTypeUnit() != CTransportData::light_art &&
				pUnit->GetTypeUnit() != CTransportData::rangers )
				return;
		}
		else if( pTask->GetTaskParam(CAI_TF_IFVS) )
		{
			if( pUnit->GetTypeUnit() != CTransportData::infantry &&
				pUnit->GetTypeUnit() != CTransportData::rangers )
				return;
		}
		else
			return;
	}
	// if still here, the pUnit must be a troop unit, so find nearest
	// troop carrier with same goal/task, with room remaining, move 
	// toward it if possible and issue CMsgLoadCarrier message
	LoadTroops(pUnit,pTask);
}

//
// this is a specialized load for landing_craft, which does the
// same basic function of LoadIFV(), but instead of moving just
// the carrier, this will also try to move the cargo, cause the
// carrier can't move on any land and that the cargo may not be
// in the staging area
//
void CAITaskMgr::LoadLandingCraft( CAIUnit *pUnit, CAITask *pTask )
{
	CSubHex subhexHead(0,0);
	CSubHex subhexTail(0,0);
	int iCnt = 0;
	
	EnterCriticalSection (&cs);
	CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return;
	}
	iCnt = pVehicle->GetCargoCount();
	if( iCnt < MAX_CARGO )
	{
		subhexHead = pVehicle->GetPtHead();
		subhexTail = pVehicle->GetPtTail();
	}
	LeaveCriticalSection (&cs);

	// carrier is too full
	if( !subhexHead.x && !subhexHead.y )
		return;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadLandingCraft(): player %d carrier unit %ld is a %d vehicle carrying %d units\n", 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetTypeUnit(), iCnt );
#endif

	// now for each troop unit, with the same goal/task as the
	// carrier unit, which is not marked as loaded already, find
	// the nearest that is inside the staging area, and move to
	// be adjacent to it
	CHexCoord hexUnit = subhexHead.ToCoord();
	int iDist=0;
	CAIUnit *puTroop = FindTroopToLoad( 
		hexUnit, pTask, iDist, CTransportData::rangers );
	if( puTroop != NULL )
	{
		// ready to load
		if( iDist < 3 )
		{
			// need to only send this once

			pUnit->LoadUnit( puTroop->GetID() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadLandingCraft(): player %d a carrier %d unit %ld loading %ld a %d vehicle ", 
pUnit->GetOwner(), pUnit->GetTypeUnit(), pUnit->GetID(), 
puTroop->GetID(), puTroop->GetTypeUnit() );
#endif
		}
		else
		{
			// if the troop to load is on land, then the landing_craft
			// can't move toward the troop and the troop to load must move
			// toward the landing craft
			subhexHead = Rotate( 0, subhexHead, subhexTail );
			puTroop->SetDestination( subhexHead );
		}
	}
}

//
// the passed unit is a carrier, so find troops of the same
// task that it can load, and do it, if it has room
//
void CAITaskMgr::LoadIFV( CAIUnit *pUnit, CAITask *pTask )
{
	CHexCoord hexCarrier;
	CSubHex subhexUnit(0,0);

	EnterCriticalSection (&cs);

	CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return;
	}
	if( pVehicle->GetCargoCount() < MAX_CARGO )
	{
		subhexUnit = pVehicle->GetPtTail();
	}
	LeaveCriticalSection (&cs);

	// carrier is too full
	if( !subhexUnit.x && !subhexUnit.y )
		return;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadIFV(): player %d carrier unit %ld is a %d vehicle ", 
pUnit->GetOwner(), pUnit->GetID(), pUnit->GetTypeUnit() );
#endif

	// now for each troop unit, with the same goal/task as the
	// carrier unit, which is not marked as loaded already, find
	// the nearest that is inside the staging area, and move to
	// be adjacent to it
	CHexCoord hexUnit = subhexUnit.ToCoord();
	hexCarrier = hexUnit;
	int iDist=0;
	CAIUnit *puTroop = FindTroopToLoad( hexUnit, pTask, iDist, 0 );
	if( puTroop != NULL )
	{
		// ready to load
		if( iDist == 1 )
		{
			pUnit->LoadUnit( puTroop->GetID() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadIFV(): player %d unit %ld loaded %ld a %d troop \n", 
pUnit->GetOwner(), pUnit->GetID(), 
puTroop->GetID(), puTroop->GetTypeUnit() );
#endif
		}
		else
		{
			CHexCoord hexTroops(0,0);
			EnterCriticalSection (&cs);

			CVehicle *pVehicle = 
				theVehicleMap.GetVehicle( puTroop->GetID() );
			if( pVehicle != NULL )
				hexTroops = pVehicle->GetHexDest();
			LeaveCriticalSection (&cs);
			
			// the troops are KIA
			if( !hexTroops.X() && !hexTroops.Y() )
				return;

			// need to move toward unit to load
			if( hexTroops != hexCarrier )
			{
				pUnit->SetDestination( hexUnit );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadIFV(): player %d carrier %ld moving to %ld troop at %d,%d \n", 
pUnit->GetOwner(), pUnit->GetID(), puTroop->GetID(), 
hexUnit.X(), hexUnit.Y() );
#endif
			}
		}
	}
}
//
// for each unit, with this goal/task, in the area of the task
// not already loaded, return the closest unit's pointer
// 
CAIUnit *CAITaskMgr::FindTroopToLoad( CHexCoord& hexCarrier, 
	CAITask *pTask, int& iDist, int iMarines )
{
	int iBestDist = 0xFFFE;
	CAIUnit *pCargo = NULL;
	CHexCoord hexBest; //,hexDest;
	int iRange = 0;

	if( m_pGoalMgr->m_plUnits != NULL )
	{
		CHexCoord hexCargo;
		// get staging bounds
		CHexCoord hcFrom( 
			pTask->GetTaskParam(CAI_LOC_X),
			pTask->GetTaskParam(CAI_LOC_Y) );
		CHexCoord hcTo( 
			pTask->GetTaskParam(CAI_PREV_X),
			pTask->GetTaskParam(CAI_PREV_Y) );

#if THREADS_ENABLED
				// BUGBUG this function must yield
		myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

        POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIUnit *pUnit = 
            	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
            if( pUnit != NULL )
            {
            	ASSERT_VALID( pUnit );

				if( pUnit->GetOwner() != m_iPlayer )
					continue;

				if( pUnit->GetGoal() != pTask->GetGoalID() ||
					pUnit->GetTask() != pTask->GetID() )
					continue;

				// only want specific types of troops
				if( iMarines )
				{
					if( pUnit->GetTypeUnit() != CTransportData::rangers &&
						//pUnit->GetTypeUnit() != CTransportData::light_tank &&
						pUnit->GetTypeUnit() != CTransportData::med_tank )
						//pUnit->GetTypeUnit() != CTransportData::light_art )
						continue;
				}
				else // infantry_carrier loads only
				{
					if( pUnit->GetTypeUnit() != CTransportData::infantry &&
						pUnit->GetTypeUnit() != CTransportData::rangers )
						continue;
				}

				EnterCriticalSection (&cs);

				CVehicle *pVehicle = 
					theVehicleMap.GetVehicle( pUnit->GetID() );

				// no vehicle pointer or already carried on a troop unit
				if( pVehicle == NULL ||
					pVehicle->GetTransport() != NULL )
				{
					LeaveCriticalSection (&cs);
					continue;
				}
				hexCargo = pVehicle->GetHexHead();
				//hexDest =  pVehicle->GetHexDest();
				
				LeaveCriticalSection (&cs);

				// is vehicle in staging area
				if( !m_pGoalMgr->m_pMap->m_pMapUtil->IsHexInArea( 
					hcFrom, hcTo, hexCargo) )
					continue;

				// if still here, then unit is a troop, not loaded
				// assigned to the same task as carrier, and in area
				iRange = 
					pGameData->GetRangeDistance( hexCarrier, hexCargo );
				if( iRange && iRange < iBestDist )
				{
					pCargo = pUnit;
					iBestDist = iRange;
					hexBest = hexCargo;
				}
			}
		}
	}

	// a troop unit was found, that could be loaded
	if( pCargo != NULL )
	{
		// consider that carrier may need to move toward cargo
		iDist = iBestDist;
		if( iBestDist > 1 )
			hexCarrier = hexBest;
	}
	return( pCargo );
}
//
// the unit passed is a troop unit, and it needs to find a carrier
// (based on task) that is of the same goal/task and within the
// staging area, but for the time being only handle land based carriers
//
void CAITaskMgr::LoadTroops( CAIUnit *puCargo, CAITask *pTask )
{
	int iBestDist = 0xFFFE;
	//CHexCoord hexBest,hexBestDest;
	int iRange = 0;

	// marines, light/med tanks, and light_art are allowed in landing craft
	int iCarrierType = 0;
	if( puCargo->GetTypeUnit() == CTransportData::rangers ||
		//puCargo->GetTypeUnit() == CTransportData::light_tank ||
		puCargo->GetTypeUnit() == CTransportData::med_tank )
		//puCargo->GetTypeUnit() == CTransportData::light_art )
		iCarrierType = CTransportData::landing_craft;
	else // infantry only
		iCarrierType = CTransportData::infantry_carrier;

	if( m_pGoalMgr->m_plUnits != NULL )
	{
		CHexCoord hexCargo, hexCarrier, hexCarrierDest;
		CSubHex shBestTail,subHexTail,subHexCargo;
		CSubHex shBestHead,subHexHead;
		CAIUnit *puCarrier = NULL;

		EnterCriticalSection (&cs);
		CVehicle *pVehicle = 
			theVehicleMap.GetVehicle( puCargo->GetID() );
		if( pVehicle == NULL )
		{
			LeaveCriticalSection (&cs);
			return;
		}
		// consider this cargo unit may already be transported
		if( pVehicle->GetTransport() != NULL )
		{
#ifdef _LOGOUT
			DWORD dwCarrier = pVehicle->GetTransport()->GetID();
#endif
			LeaveCriticalSection (&cs);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadTroops(): player %d unit %ld already loaded on %ld \n", 
puCargo->GetOwner(), puCargo->GetID(), dwCarrier );
#endif

			return;
		}

		hexCargo = pVehicle->GetHexHead();
		subHexCargo = pVehicle->GetPtHead();
		LeaveCriticalSection (&cs);


#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadTroops(): player %d unit %ld is a %d troop at subhex %d,%d \n", 
puCargo->GetOwner(), puCargo->GetID(), puCargo->GetTypeUnit(),
subHexCargo.x, subHexCargo.y );
#endif

		// get staging bounds
		CHexCoord hcFrom( 
			pTask->GetTaskParam(CAI_LOC_X),
			pTask->GetTaskParam(CAI_LOC_Y) );
		CHexCoord hcTo( 
			pTask->GetTaskParam(CAI_PREV_X),
			pTask->GetTaskParam(CAI_PREV_Y) );
		CSubHex subHex;

#if THREADS_ENABLED
				// BUGBUG this function must yield
		myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

        POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIUnit *pUnit = 
            	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
            if( pUnit != NULL )
            {
            	ASSERT_VALID( pUnit );

				if( pUnit->GetOwner() != m_iPlayer )
					continue;

				if( pUnit->GetGoal() != pTask->GetGoalID() ||
					pUnit->GetTask() != pTask->GetID() )
					continue;

				// CTransportData::IsCarrier() returns TRUE for any carrier
				if( pUnit->GetTypeUnit() != iCarrierType )
					continue;
				
				EnterCriticalSection (&cs);

				CVehicle *pVehicle = 
					theVehicleMap.GetVehicle( pUnit->GetID() );

				// no vehicle pointer or already carrying capacity
				if( pVehicle == NULL )
				{
					LeaveCriticalSection (&cs);
					continue;
				}
				if( pVehicle->GetCargoCount() >= MAX_CARGO )
				{
					LeaveCriticalSection (&cs);
					continue;
				}
				subHexTail = pVehicle->GetPtTail();
				subHexHead = pVehicle->GetPtHead();
				hexCarrier = pVehicle->GetHexHead();
				hexCarrierDest = pVehicle->GetHexDest();

				LeaveCriticalSection (&cs);

				// is vehicle in staging area?
				if( !m_pGoalMgr->m_pMap->m_pMapUtil->IsHexInArea( 
					hcFrom, hcTo, hexCarrier) )
					continue;

				// switch to tail hex for all but LCs
				if( iCarrierType != CTransportData::landing_craft )
				{
					iRange = 
						subHexCargo.Dist( subHexTail );
					// BUGBUG this may work
					subHex = Rotate( 0, subHexHead, subHexTail );
					hexCarrier = subHex.ToCoord();
				}
				else // range for LCs uses sub-hex head
				{
					iRange = subHexCargo.Dist( subHexHead );
					// BUGBUG this may work
					subHex = Rotate( 0, subHexTail, subHexHead );
					hexCarrier = subHex.ToCoord();
				}
					
				// can cargo unit make it to the carrier?
				if( !m_pGoalMgr->m_pMap->m_pMapUtil->GetPathRating(
					hexCargo, hexCarrier, puCargo->GetTypeUnit()) )
					continue;

				// range is in subhexs
				if( iRange < iBestDist )
				{
					puCarrier = pUnit;
					iBestDist = iRange;
					shBestTail = subHexTail;
					shBestHead = subHexHead;
				}
			}
		}

		if( puCarrier != NULL )
		{
			// need to move troop unit to carrier
			if( iBestDist > 2 )
			{
				// turn carrier hex to tail hex using one of Dave's
				// mysterious functions, and send vehicle to it
				if( iCarrierType == CTransportData::infantry_carrier )
					subHexHead = Rotate( 0, shBestTail, shBestHead );
				else
					subHexHead = Rotate( 0, shBestHead, shBestTail );

				puCargo->SetDestination( subHexHead );

				// using shBestTail, find an adjacent hex for destination
				// that is not the same as shBestHead, and preferably one
				// that is opposite the head
				// puCargo->SetDestination( shBestTail );

				// set flag to indicate that a load is in process
				if( !(puCargo->GetStatus() & CAI_IN_USE) )
				{
					WORD wStatus = puCargo->GetStatus();
					wStatus |= CAI_IN_USE;
					puCargo->SetStatus(wStatus);
				}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadTroops(): p:%d troop %ld moving to %ld carrier type %d at subhex %d,%d ", 
puCargo->GetOwner(), puCargo->GetID(), puCarrier->GetID(), 
puCarrier->GetTypeUnit(), subHexHead.x, subHexHead.y );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::LoadTroops(): carrier head=%d,%d tail=%d,%d  subhex dist=%d from troop\n", 
shBestHead.x, shBestHead.y, shBestTail.x, shBestTail.y, iBestDist );
#endif

			}
			else // issue load order
			{

				puCarrier->LoadUnit( puCargo->GetID() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadTroops(): player %d carrier unit %ld loaded %ld a %d unit \n", 
puCarrier->GetOwner(), puCarrier->GetID(), 
puCargo->GetID(), puCargo->GetTypeUnit() );
#endif
			}
		}
#ifdef _LOGOUT
		else
		{
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::LoadTroops(): player %d unit %ld could not find carrier ", 
puCargo->GetOwner(), puCargo->GetID() );
		}
#endif
	}
}


//
// handle unloading cargo units, considering that the carrier unit
// may be on terrain that cannot support the cargo unit
//
void CAITaskMgr::UnloadCargo( CAIUnit *pUnit )
{
	if( pUnit->GetTypeUnit() != CTransportData::infantry_carrier &&
		pUnit->GetTypeUnit() != CTransportData::landing_craft )
		return;

	CHexCoord hexHead;
	BOOL bIsLoaded = FALSE;
	EnterCriticalSection (&cs);

	CVehicle *pVehicle = 
		theVehicleMap.GetVehicle( pUnit->GetID() );

	// a valid vehicle and carrying?
	if( pVehicle != NULL )
	{
		hexHead = pVehicle->GetHexHead();
		// there are vehicles loaded onboard
		if( pVehicle->GetCargoCount() )
			bIsLoaded = TRUE;
	}
	LeaveCriticalSection (&cs);

	if( !bIsLoaded )
		return;

	// landing craft must be adjacent to land to unload
	if( pUnit->GetTypeUnit() == CTransportData::landing_craft )
	{
		if( !m_pGoalMgr->m_pMap->m_pMapUtil->IsWaterAdjacent(hexHead,1,1) )
		{
			m_pGoalMgr->m_pMap->m_pMapUtil->FindLandingHex(hexHead);
			pUnit->SetDestination( hexHead );
		}
	}
	
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::UnloadCargo() unit %ld player %d unloading any cargo \n", 
pUnit->GetID(), pUnit->GetOwner() );
#endif

	pUnit->UnloadCargo();
}

//
// handle the unit_attacked message
//
// need a clarification on when this message is sent
// to the AI
// 
void CAITaskMgr::AttackAlert( CAIMsg *pMsg )
{
	DWORD dwAI, dwOpfor;
	// unit targeted was us
	if( pMsg->m_idata3 == m_iPlayer )
	{
		dwAI = pMsg->m_dwID;
		dwOpfor = pMsg->m_dwID2;
	}
	else
		return;
/*
		case CNetCmd::unit_attacked:
			pAttackedMsg = (CMsgAttackUnit *)pMsg;
			m_uFlags = 0;
			m_iPriority = 90;
			m_dwID = pAttackedMsg->m_dwIDtarget;	// receiving player's unit
			m_iX = pAttackedMsg->m_hexTarget.X();
			m_iY = pAttackedMsg->m_hexTarget.Y();
			m_ieX = pAttackedMsg->m_hexMe.X();
			m_ieY = pAttackedMsg->m_hexMe.Y();
			m_idata1 = 0;
			m_idata2 = pAttackedMsg->m_iPlyrNumMe;
			m_idata3 = pAttackedMsg->m_iPlyrNumTarget; // receiving player id
			m_dwID2 = pAttackedMsg->m_dwIDme;
			break;

		case CNetCmd::unit_damage:
			pDmgMsg = (CMsgUnitDamage *)pMsg;
			m_uFlags = 0;
			m_iPriority = 80;
			m_dwID = pDmgMsg->m_dwIDTarget;			// ID of unit being shot at
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = pDmgMsg->m_iDamageShot;
			m_idata2 = pDmgMsg->m_iPlyrShoot;
			m_idata3 = pDmgMsg->m_iPlyrTarget;
			m_dwID2 = pDmgMsg->m_dwIDShoot;
			break;

CAITaskMgr::AttackAlert(): Target=35 Attacker=22 
CAITaskMgr::AttackAlert(): Target player=2 Attacker player=1 

*/

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::AttackAlert(): Target=%ld Attacker=%ld ",
pMsg->m_dwID, pMsg->m_dwID2 );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AttackAlert(): Target player=%d Attacker player=%d \n",
pMsg->m_idata3, pMsg->m_idata2 );
#endif

	CAIUnit *pTarget = m_pGoalMgr->m_plUnits->GetUnit( pMsg->m_dwID );
	if( pTarget == NULL )
		return;
	// cause attacker to be added to list if not already there
	CAIUnit *pAttacker = m_pGoalMgr->m_plUnits->GetOpForUnit( pMsg->m_dwID2 );
	if( pAttacker == NULL )
		return;

	// scenario units do not respond here
	if( pTarget->GetTask() == IDT_ATTACKUNIT )
		return;

	if( pAttacker->GetOwner() == m_iPlayer )
		return;

	// let's do something special for moderate difficulty
	// we have a 3 in 4 chance to not attack
	/*
	if( pGameData->m_iSmart == 1 )
	{
		if( pGameData->GetRandom(NUM_DIFFICUTY_LEVELS) )
			return;
	}
	*/

	// consider targeting unit that attacked target
	if( pTarget->GetDataDW() )
	{
		if( pTarget->GetDataDW() != pMsg->m_dwID2 )
		{
			if( pTarget->GetTask() >= IDT_SEEKINRANGE &&
				pTarget->GetTask() <= IDT_SEEKATSEA )
			{
			pTarget->SetDataDW(pMsg->m_dwID2);
			
			pTarget->SetParam(CAI_TARGETTYPE,pAttacker->GetType());
			//pTarget->SetParam(CAI_TARGETTYPE,0xFFFE );
			
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AttackAlert(): Player %d Unit %ld has targeted %ld ",
 pTarget->GetOwner(), pTarget->GetID(), pTarget->GetDataDW() );
#endif
			}
		}
		//else
		//{
		//	pTarget->AttackUnit( pMsg->m_dwID2 );
		//	return;
		//}
	}
	// always shoot back, if not our side
	pTarget->AttackUnit( pMsg->m_dwID2 );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AttackAlert(): Player %d Unit %ld has attacked %ld \n",
 pTarget->GetOwner(), pTarget->GetID(), pMsg->m_dwID2 );
#endif


	// get goalmgr's opinion of opfor/our combat strengths
	BOOL bIamStronger = m_pGoalMgr->RealityCheck(pMsg->m_dwID2);
	bIamStronger = TRUE;

	// determine location of attack
	CHexCoord hexAttacked( pMsg->m_ieX, pMsg->m_ieY );

	// consider it may not be worth going to the rescue
	if( !bIamStronger )
	{
		// get location of opfor rocket, then its distance from
		// hexAttacked and then location of our rocket, and its
		// distance from hexAttacked
		CHexCoord hexOpfor;
		pGameData->FindBuilding( 
			CStructureData::rocket, pMsg->m_idata2, hexOpfor );
		CHexCoord hexRocket;
		pGameData->FindBuilding( 
			CStructureData::rocket, pTarget->GetOwner(), hexRocket );

		// if dist to opfor is < dist to ours, then not worth it
		if( pGameData->GetRangeDistance(hexOpfor,hexAttacked) <
			pGameData->GetRangeDistance(hexRocket,hexAttacked) )
			return;
	}

	BOOL bIsBuildingTarget = FALSE;
	if( pTarget->GetType() == CUnit::building )
		bIsBuildingTarget = TRUE;

	if( pTarget->GetOwner() == m_iPlayer && 
		!bIsBuildingTarget )
		UnloadCargo( pTarget );

	// consider building may be abandoned
	if( bIsBuildingTarget )
	{
		BOOL bIsAbandoned = FALSE;
		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( pTarget->GetID() );
		if( pBldg != NULL )
			bIsAbandoned = pBldg->IsFlag( CUnit::abandoned );	
		LeaveCriticalSection (&cs);
		// this an abandoned building, so don't bother
		if( bIsAbandoned )
			return;
	}
	// find nearest combat unit with proper task
	
	CHexCoord hexVeh;
	int iBest=0xFFFE,iDist;
	CAIUnit *pRescuer = NULL;

	if( m_pGoalMgr->m_plUnits != NULL )
	{

	// now if task of the rescue unit is IDT_PATROL or IDT_SCOUT
	// then task switch to IDT_SEEKINRANGE

        POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIUnit *pUnit = 
            	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
            if( pUnit != NULL )
            {
            	ASSERT_VALID( pUnit );

				if( pUnit->GetOwner() != m_iPlayer )
					continue;
				// skip the attacked unit
				if( pUnit->GetID() == dwAI )
					continue;
				// only vehicles are desired
				if( pUnit->GetType() != CUnit::vehicle )
					continue;

				// that are doing these things
				if( pUnit->GetTask() != IDT_PATROL &&
					pUnit->GetTask() != IDT_SCOUT &&
					pUnit->GetTask() != IDT_PREPAREWAR )
					continue;

				// if the target was a building, then it may have
				// a patrol vehicl assigned, which would be ided
				// in the patroling unit's params
				if( bIsBuildingTarget && pUnit->GetTask() == IDT_PATROL &&
					pUnit->GetParamDW(CAI_PATROL) == pTarget->GetID() )
				{
					pRescuer = pUnit;
					break;
				}

				BOOL bIsLoaded = FALSE;
				BOOL bCanShoot = FALSE;
				int iSpotting = 0;
				EnterCriticalSection (&cs);
				CVehicle *pVehicle = 
					theVehicleMap.GetVehicle( pUnit->GetID() );
				if( pVehicle == NULL )
				{
					LeaveCriticalSection (&cs);
					continue;
				}
				// if vehicle is loaded on a vehicle,
				// then consider that the carrier may be
				// the target 
				if( pVehicle->GetTransport() != NULL )
				{
					// if carrier is not the target, skip 
					// this vehicle for consideration
					if( pVehicle->GetTransport()->GetID() !=
						pTarget->GetID() )
						bIsLoaded = TRUE;
				}
				hexVeh = pVehicle->GetHexHead();
				iSpotting = pVehicle->GetSpottingRange();

				// check vehicle's ability to fight
				if( pVehicle->GetAttack(CUnitData::soft) ||
					pVehicle->GetAttack(CUnitData::hard) ||
					pVehicle->GetAttack(CUnitData::naval) )
					bCanShoot = TRUE;

				LeaveCriticalSection (&cs);

				if( bIsLoaded )
					continue;
				if( !bCanShoot )
					continue;

				
				// if the hexAttacked -> hexVeh


#if THREADS_ENABLED
	myYieldThread();
#endif

#ifdef _DNT
				// can it get to the attacker?
				if( !m_pGoalMgr->m_pMap->m_pMapUtil->GetPathRating(
					hexVeh, hexAttacked, pUnit->GetTypeUnit()) )
				{
					// or just get within range of attacker
					if( !m_pGoalMgr->GetPathRating(
						hexVeh, hexAttacked, pUnit->GetTypeUnit()) )
						continue;
				}
#endif

				// distance to attacker from this unit
				iDist = pGameData->GetRangeDistance( hexVeh, hexAttacked );

				// but not if this is a staging unit, and it is more
				// than twice spotting away
				/*
				if( pUnit->GetTask() == IDT_PREPAREWAR )
				{
					if( iDist > (iSpotting * 2) )
						continue;
				}
				*/

				// we want the closest
				if( iDist && iDist < iBest )
				{
					// can it get to the attacker?
					if( !m_pGoalMgr->m_pMap->m_pMapUtil->GetPathRating(
						hexVeh, hexAttacked, pUnit->GetTypeUnit()) )
					{
						// or just get within range of attacker
						if( !m_pGoalMgr->GetPathRating(
							hexVeh, hexAttacked, pUnit->GetTypeUnit()) )
							{
							TRAP ();
							continue;
							}
					}

					iBest = iDist;
					pRescuer = pUnit;
				}
			}
		}
	}
	if( pRescuer != NULL )
	{
		WORD wStatus = pRescuer->GetStatus();
		wStatus |= CAI_IN_COMBAT;
		wStatus |= CAI_TASKSWITCH;
		pRescuer->SetStatus( wStatus );

		// save old task
		pRescuer->SetParam( CAI_UNASSIGNED, pRescuer->GetTask() );
		// assign unit to support attacked unit
		pRescuer->SetTask( IDT_SEEKINRANGE );

		MoveToRange(pRescuer, hexAttacked);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::AttackAlert(): Player %d Unit %ld has task switched \n",
 m_iPlayer, pRescuer->GetID() );
#endif
	}
}

//
// handle the see_unit message
// 
/*
		case CNetCmd::see_unit:		// see a unit that previously was unseen
			pSeeUnit = (CMsgSeeUnit *)pMsg;
			m_uFlags = 0;
			m_iPriority = 90;
			m_dwID = pSeeUnit->m_dwIDme;
			m_iX = pSeeUnit->m_hexMe.X();
			m_iY = pSeeUnit->m_hexMe.Y();
			m_ieX = pSeeUnit->m_hexSpot.X();
			m_ieY = pSeeUnit->m_hexSpot.Y();
			m_idata1 = 0;
			m_idata2 = pSeeUnit->m_iPlyrNumSpot;
			m_idata3 = pSeeUnit->m_iPlyrNumMe;
			m_dwID2 = pSeeUnit->m_dwIDspot;
			break;

CAITaskMgr::SpottingAlert(): Me Unit=10 Spot Unit=26 
CAITaskMgr::SpottingAlert(): Me player=2 Spot player=1 

*/
// when the AI unit spots an opfor, we want to check the AI
// unit's task, and if its IDT_PATROL, IDT_SCOUT or IDT_PREPAREWAR 
// then the unit must task switch to IDT_SEEKINRANGE.  AI units
// with other tasks, should just continue.
//
// when the AI unit is spotted by an opfor it just continues
//
void CAITaskMgr::SpottingAlert( CAIMsg *pMsg )
{
	if( pMsg->m_idata3 != m_iPlayer )
		return;
	
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::SpottingAlert(): Me Unit=%ld Spot Unit=%ld ",
pMsg->m_dwID, pMsg->m_dwID2 );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::SpottingAlert(): Me player=%d Spot player=%d \n",
pMsg->m_idata3, pMsg->m_idata2 );
#endif

	// get pointer to spotted unit
	CAIUnit *pOpForUnit = 
		m_pGoalMgr->m_plUnits->GetOpForUnit( pMsg->m_dwID2 );
	if( pOpForUnit == NULL )
		return;

	CAIUnit *paiUnit = m_pGoalMgr->m_plUnits->GetUnit( pMsg->m_dwID );
	if( paiUnit == NULL )
		return;

	// right now, just return for all non-vehicle spottings
	// otherwise later, will look for nearest combat vehicle
	if( paiUnit->GetType() != CUnit::vehicle )
		return;

	// if unit is a truck or crane, 
	// then make it run to a retreat hex?
	CHexCoord hexSpotted(pMsg->m_ieX, pMsg->m_ieY);
	CHexCoord hexVeh = hexSpotted;
	if( paiUnit->GetTypeUnit() == CTransportData::construction ||
		paiUnit->GetTypeUnit() == CTransportData::light_scout ||
		pGameData->IsTruck(paiUnit->GetID()) )
	{
		m_pGoalMgr->m_pMap->m_pMapUtil->FindRetreatHex( 
			paiUnit, hexVeh, NULL, hexSpotted );

		// turn off explore destination flag for scouts that run
		if( paiUnit->GetTypeUnit() == CTransportData::light_scout )
			paiUnit->SetParam(CAI_FUEL,0);

		paiUnit->SetDestination(hexVeh);
	}

	// consider that the spotted unit may be unimportant
	if( pOpForUnit->GetType() == CUnit::building )
	{
		// skip unimportant buildings
		if( pOpForUnit->GetTypeUnit() != CStructureData::rocket &&
			pOpForUnit->GetTypeUnit() < CStructureData::barracks_2 )
			return;
	}

	// consider that opfor of spotting unit is AI player
	CAIOpFor *pOpFor = m_pGoalMgr->GetOpFor(pOpForUnit->GetOwner());
	if( pOpFor == NULL )
		return;
	// higher difficulty has auto AI player alliance
	if( pGameData->m_iSmart && pOpFor->IsAI() )
		return;
	// non-AI, easy and no war then skip, otherwise attack
	if( !pGameData->m_iSmart && !pOpFor->AtWar() )
		return;

	// let's do something special for moderate difficulty
	// we have a 3 in 4 chance to not attack
	/*
	if( pGameData->m_iSmart == 1 )
	{
		if( pGameData->GetRandom(NUM_DIFFICUTY_LEVELS) )
			return;
	}
	*/

	// unit is already switched or involved with an attack
	if( (paiUnit->GetStatus() & CAI_IN_COMBAT) ||
		(paiUnit->GetStatus() & CAI_TASKSWITCH) )
		return;

	// perform a task switch and go get'em by moving into range
	if( (paiUnit->GetTask() == IDT_PATROL ||
		paiUnit->GetTask() == IDT_SCOUT) &&
		paiUnit->GetTypeUnit() != CTransportData::light_scout )
		//paiUnit->GetTask() == IDT_PREPAREWAR )
	{
		WORD wStatus = paiUnit->GetStatus();
		wStatus |= CAI_IN_COMBAT;
		wStatus |= CAI_TASKSWITCH;
		paiUnit->SetStatus( wStatus );

		// save old task
		paiUnit->SetParam( CAI_UNASSIGNED, paiUnit->GetTask() );
		// assign unit to support attacked unit
		paiUnit->SetTask( IDT_SEEKINRANGE );
		paiUnit->SetDataDW( pMsg->m_dwID2 );

		UnloadCargo(paiUnit);

		CHexCoord hex( pMsg->m_ieX, pMsg->m_ieY );
		MoveToRange(paiUnit, hex);

		// record that this spotted unit is being sought
		if( pOpForUnit != NULL )
		{
			WORD wCnt = pOpForUnit->GetParam(CAI_SPOTTING);
			pOpForUnit->SetParam(CAI_SPOTTING, (wCnt+1) );
		}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::SpottingAlert(): Unit Task Switched " );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"player %d unit %ld type=%d recon near %d,%d \n",
paiUnit->GetOwner(), paiUnit->GetID(),paiUnit->GetTypeUnit(),
pMsg->m_ieX, pMsg->m_ieY );
#endif

	}
	else // unit was spotted by a unit not on patrol
	{
		if( pOpForUnit != NULL )
		{
			// consider that some units are already seeking this guy
			if( pOpForUnit->GetParam(CAI_SPOTTING) >
				pGameData->m_iSmart )
				return;
		}

		// if still here, then find the patroling/scouting/staging
		// unit that is the closest to the spotted unit and sic'em
		int iBest=0xFFFE,iDist;
		CAIUnit *pRescuer = NULL;

		if( m_pGoalMgr->m_plUnits != NULL )
		{
#if THREADS_ENABLED
				// BUGBUG this function must yield
			myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

        	POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
        	while( pos != NULL )
        	{   
            	CAIUnit *pUnit = 
            	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
            	if( pUnit != NULL )
            	{
            		ASSERT_VALID( pUnit );

					if( pUnit->GetOwner() != m_iPlayer )
						continue;
					// that are doing these things
					if( pUnit->GetTask() != IDT_PATROL &&
						pUnit->GetTask() != IDT_SCOUT )
						continue;

					// only vehicles are desired
					if( pUnit->GetType() != CUnit::vehicle )
						continue;

					BOOL bIsLoaded = FALSE;
					EnterCriticalSection (&cs);
					CVehicle *pVehicle = 
						theVehicleMap.GetVehicle( pUnit->GetID() );
					if( pVehicle == NULL )
					{
						LeaveCriticalSection (&cs);
						continue;
					}
					hexVeh = pVehicle->GetHexHead();
					if( pVehicle->GetTransport() != NULL )
						bIsLoaded = TRUE;

					// make sure unit can attack something or skip it
					if( !pVehicle->GetAttack(CUnitData::soft) &&
						!pVehicle->GetAttack(CUnitData::hard) &&
						!pVehicle->GetAttack(CUnitData::naval) )
						bIsLoaded = TRUE;

					LeaveCriticalSection (&cs);

					if( bIsLoaded )
						continue;

					// can it get to the attacker?
					if( !m_pGoalMgr->m_pMap->m_pMapUtil->GetPathRating(
						hexVeh, hexSpotted, pUnit->GetTypeUnit()) )
						continue;

					// we want the closest
					iDist = 
					pGameData->GetRangeDistance( hexVeh, hexSpotted );
					if( iDist && iDist < iBest )
					{
						iBest = iDist;
						pRescuer = pUnit;
					}
				}
			}
		}

		if( pRescuer != NULL )
		{
			WORD wStatus = pRescuer->GetStatus();
			wStatus |= CAI_IN_COMBAT;
			wStatus |= CAI_TASKSWITCH;
			pRescuer->SetStatus( wStatus );

			// save old task
			pRescuer->SetParam( CAI_UNASSIGNED, pRescuer->GetTask() );
			// assign unit to support attacked unit
			pRescuer->SetTask( IDT_SEEKINRANGE );

			MoveToRange(pRescuer, hexSpotted);

			// record that this spotted unit is being sought
			if( pOpForUnit != NULL )
			{
				WORD wCnt = pOpForUnit->GetParam(CAI_SPOTTING);
				pOpForUnit->SetParam(CAI_SPOTTING, (wCnt+1) );
			}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAITaskMgr::SpottingAlert(): Unit Task Switched " );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"player %d unit %ld type=%d recon near %d,%d \n",
pRescuer->GetOwner(), pRescuer->GetID(),pRescuer->GetTypeUnit(),
pMsg->m_ieX, pMsg->m_ieY );
#endif

		}
	}
}

/*
	case CNetCmd::unit_set_damage:
		pSetDmgMsg = (CMsgUnitSetDamage *)pMsg;
		m_uFlags = 0;
		m_iPriority = 94;
		m_dwID = pSetDmgMsg->m_dwIDDamage;
		m_iX = 0;
		m_iY = 0;
		m_ieX = 0;
		m_ieY = 0;
		m_idata1 = pSetDmgMsg->m_iDamageLevel;
		m_idata2 = pSetDmgMsg->m_iPlyrShoot;
		m_idata3 = pSetDmgMsg->m_iPlyrTarget;
		m_dwID2 = pSetDmgMsg->m_dwIDShoot;
		break;
*/
//
// this is called when unit_damage arrives
//
void CAITaskMgr::RepairUnit( CAIMsg *pMsg, int iDmgPer )
{
	if( pMsg->m_idata3 != m_iPlayer )
		return;

	CAIUnit *paiUnit = 
		m_pGoalMgr->m_plUnits->GetUnit( pMsg->m_dwID );
	if( paiUnit == NULL )
		return;

	// right now, just return for all non-vehicle spottings
	// otherwise later, will look for nearest combat vehicle
	if( paiUnit->GetType() != CUnit::vehicle )
		return;

	CHexCoord hexRepair, hexDest;
	int iRepairPer=0,iTypeRepair=0;
	EnterCriticalSection (&cs);

	CVehicle *pVehicle = 
		theVehicleMap.GetVehicle( paiUnit->GetID() );

	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return;
	}
	hexRepair = pVehicle->GetHexHead();
	hexDest = pVehicle->GetHexDest();
	iRepairPer = pVehicle->GetDamagePer();

	// check for delete in progress, and force return if so
	if( pVehicle->GetFlags() & CUnit::dying )
		iRepairPer = iDmgPer;

	if( pVehicle->GetData()->IsBoat() )
		iTypeRepair = CStructureData::shipyard_1;
	else
		iTypeRepair = CStructureData::repair;

	LeaveCriticalSection (&cs);
	
	// not damaged enough
	if( iRepairPer > iDmgPer )
		return;

	// find nearest repair depot, returning exit hex in hexRepair
	CAIUnit *paiRepair = m_pGoalMgr->m_plUnits->GetClosestRepair(
		m_iPlayer, iTypeRepair, hexRepair );
	if( paiRepair == NULL )
	{
		// if unit is a truck or crane, 
		// then make it run to a retreat hex?
		if( paiUnit->GetTypeUnit() == CTransportData::construction ||
			pGameData->IsTruck(paiUnit->GetID()) )
		{
			m_pGoalMgr->m_pMap->m_pMapUtil->FindRetreatHex( 
				paiUnit, hexRepair, NULL, hexRepair );

			paiUnit->SetDestination(hexRepair);
		}
		return;
	}

	// task switch to IDT_REPAIRING 
	WORD wStatus = paiUnit->GetStatus();
	wStatus ^= CAI_IN_COMBAT;
	wStatus |= CAI_TASKSWITCH;
	paiUnit->SetStatus( wStatus );

	// save old task
	paiUnit->SetParam( CAI_UNASSIGNED, paiUnit->GetTask() );
	paiUnit->SetDataDW( paiRepair->GetID() );
	paiUnit->SetTask(IDT_REPAIRING);

	if( hexDest != hexRepair )
	{
		paiUnit->SetDestination(hexRepair);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::RepairUnit(): player %d unit %ld going to repair at %d,%d \n",
paiUnit->GetOwner(), paiUnit->GetID(),hexRepair.X(),hexRepair.Y() );
#endif
	}
}

//
// an AI unit has been attacked, and needs to run from the attacker
//
void CAITaskMgr::RunAway( CAIUnit *pTargeted, 
	CAIUnit *pAttacker, CAIMsg *pMsg )
{
	CHexCoord hexFlee( pMsg->m_iX, pMsg->m_iY );
	CHexCoord hexAway( pMsg->m_ieX, pMsg->m_ieY );

	pTargeted->SetParamDW(CAI_ROUTE_X, 0 );
	pTargeted->SetParamDW(CAI_ROUTE_Y, 0 );

	m_pGoalMgr->m_pMap->m_pMapUtil->FindRetreatHex( 
		pTargeted, hexFlee, pAttacker, hexAway );
	
	if( hexFlee.X() != pMsg->m_iX ||
		hexFlee.Y() != pMsg->m_iY )
		pTargeted->SetDestination( hexFlee );
}
//
// this will sync the goal/task of units carried by a carrier unit
// with the goal/task of the task that is passed
//
void CAITaskMgr::SyncStageCargo( CAIUnit *pCarrier, CAITask *pTask )
{
	EnterCriticalSection (&cs);
	CVehicle *pVehicle = 
		theVehicleMap.GetVehicle( pCarrier->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return;
	}
	// are there any units onboard?
	if( !pVehicle->GetCargoCount() )
	{
		LeaveCriticalSection (&cs);
		return;
	}
	// go through list of units on board the carrier
	POSITION pos = pVehicle->GetCargoHeadPosition ();
	while (pos != NULL)
	{
		CVehicle *pVehCargo = pVehicle->GetCargoNext (pos);
		if( pVehCargo != NULL )
		{
			// there must be a bug in the game, as this vehicle is
			// not really carried?
			if( pVehCargo->GetTransport() != pVehicle )
				continue;
			// get AI copy of this unit
			CAIUnit *paiCargo = 
				m_pGoalMgr->m_plUnits->GetUnit( pVehCargo->GetID() );
			if( paiCargo != NULL )
			{
				// reassign cargo to same task as carrier
				ClearTaskUnit( paiCargo );

				paiCargo->SetGoal( pTask->GetGoalID() );
				paiCargo->SetTask( pTask->GetID() );

				WORD wStatus = paiCargo->GetStatus();
				if( wStatus & CAI_IN_USE )
				{
					wStatus ^= CAI_IN_USE;
					paiCargo->SetStatus(wStatus);
				}
			}
		}
	}
	LeaveCriticalSection (&cs);
}
//
// pUnit is an IFV looking for infantry to load or infantry
// waiting for IFV to arrive for load
// confirm the other unit is still around
// determine if in range to load, if so load
// else make sure IFV is on the way
//
void CAITaskMgr::LoadPatrolCargo( CAIUnit *pUnit )
{
	// get AI copy of this unit
	CAIUnit *paiCargo = NULL;
	CAIUnit *paiCarrier = NULL;

	if( pUnit->GetTypeUnit() == CTransportData::infantry_carrier )
	{
		paiCargo =
		m_pGoalMgr->m_plUnits->GetUnit( pUnit->GetParamDW(CAI_GRDDEFENSE) );
		// infantry is dead
		if( paiCargo == NULL )
		{
			pUnit->SetParamDW(CAI_GRDDEFENSE, 0);
			return;
		}
		paiCarrier = pUnit;
	}
	else
	{
		paiCarrier =
		m_pGoalMgr->m_plUnits->GetUnit( pUnit->GetParamDW(CAI_GRDDEFENSE) );
		// IFV is dead
		if( paiCarrier == NULL )
		{
			pUnit->SetParamDW(CAI_GRDDEFENSE, 0);
			return;
		}
		paiCargo = pUnit;
	}
	
	CHexCoord hexCargo,hexCarrier,hexCarrierDest,hexCargoDest;
	CSubHex subHexHead,subHexTail,subHexCargo;
	BOOL bIsLoaded = FALSE;

	EnterCriticalSection (&cs);
	CVehicle *pVehicle = 
		theVehicleMap.GetVehicle( paiCargo->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return;
	}
	hexCargo = pVehicle->GetHexHead();
	hexCargoDest = pVehicle->GetHexDest();
	subHexCargo = pVehicle->GetPtHead();
	// consider if the infantry is already loaded
	if( pVehicle->GetTransport() != NULL )
		bIsLoaded = TRUE;

	pVehicle = 
		theVehicleMap.GetVehicle( paiCarrier->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return;
	}

	subHexTail = pVehicle->GetPtTail();
	subHexHead = pVehicle->GetPtHead();
	hexCarrier = pVehicle->GetHexHead();
	hexCarrierDest = pVehicle->GetHexDest();
	// consider that the IFV already has cargo
	if( pVehicle->GetCargoCount() )
		bIsLoaded = TRUE;

	LeaveCriticalSection (&cs);
		
	// either the infantry is already loaded or the IFV has 
	// already loaded another unit
	if( bIsLoaded )
	{
		paiCarrier->SetParamDW(CAI_GRDDEFENSE, 0);
		paiCargo->SetParamDW(CAI_GRDDEFENSE, 0);
		return;
	}

	int iRange = subHexCargo.Dist( subHexTail );
	if( iRange > 1 )
	{
		// stop moving IFV and move infantry to tail of IFV
		if( hexCarrierDest == hexCarrier &&
			iRange < 3 )
		{
			//subHexHead = Rotate( 0, subHexTail, subHexHead );
			//paiCargo->SetDestination( subHexHead );
			paiCargo->SetDestination( subHexTail );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadPatrolCargo(): player %d troop %ld moving to %ld carrier at subhex %d,%d \n", 
paiCargo->GetOwner(), paiCargo->GetID(), paiCarrier->GetID(), 
subHexTail.x, subHexTail.y ); //subHexHead.x, subHexHead.y );
#endif
		}
		else
		{
			if( hexCargoDest != hexCarrierDest )
				paiCargo->SetDestination( hexCarrierDest );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadPatrolCargo(): player %d troop %ld enroute to %ld carrier at %d,%d \n", 
paiCargo->GetOwner(), paiCargo->GetID(), paiCarrier->GetID(), 
hexCarrierDest.X(), hexCarrierDest.Y() );
#endif
		}
	}
	else // close enough to load
	{
		paiCarrier->LoadUnit( paiCargo->GetID() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::LoadPatrolCargo(): player %d carrier unit %ld loaded %ld a %d unit \n", 
paiCarrier->GetOwner(), paiCarrier->GetID(), 
paiCargo->GetID(), paiCargo->GetTypeUnit() );
#endif
		WORD wStatus = paiCarrier->GetStatus();
		wStatus |= CAI_IN_USE;
		paiCarrier->SetStatus(wStatus);
		paiCarrier->SetParamDW(CAI_GRDDEFENSE, 1);

		wStatus = paiCargo->GetStatus();
		wStatus |= CAI_IN_USE;
		paiCargo->SetStatus(wStatus);
		paiCargo->SetParamDW(CAI_GRDDEFENSE, 0);
		paiCargo->SetParam( CAI_DEST_X, 0 );
		paiCargo->SetParam( CAI_DEST_Y, 0 );
	}
}

//
// look for an infantry or ranger unit with the same task that
// is not assigned to be carried "GetParamDW(CAI_GRDDEFENSE) != 0"
// and assign it to this carrier and move toward it to range 1
// and if none found return FALSE, and TRUE if one found
//
BOOL CAITaskMgr::FindPatrolCargo( CAIUnit *pCarrier, CAITask *pTask )
{
	BOOL bFound = FALSE;
	int iDist, iBest=0xFFFE;
	CAIUnit *pCargo = NULL;
	CHexCoord hexCarrier, hexCargo;

	EnterCriticalSection (&cs);
	CVehicle *pVehicle = 
		theVehicleMap.GetVehicle( pCarrier->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return( FALSE );
	}
	hexCarrier = pVehicle->GetHexHead();
	LeaveCriticalSection (&cs);

    POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
       	CAIUnit *pUnit = 
          	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetGoal() != pTask->GetGoalID() ||
				pUnit->GetTask() != pTask->GetID() )
				continue;

			if( pUnit->GetTypeUnit() != CTransportData::infantry &&
				pUnit->GetTypeUnit() != CTransportData::rangers )
				continue;

			// already flagged to be carried on patrol
			if( pUnit->GetParamDW(CAI_GRDDEFENSE) )
				continue;

			// flagged as in the process of being loaded
			if( (pUnit->GetStatus() & CAI_IN_USE) )
				continue;

			EnterCriticalSection (&cs);
			pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
			if( pVehicle == NULL )
			{
				LeaveCriticalSection (&cs);
				continue;
			}
			hexCargo = pVehicle->GetHexHead();

			// test to be sure vehicle is not already carried
			if( pVehicle->GetTransport() != NULL )
				hexCarrier = hexCargo;

			LeaveCriticalSection (&cs);

			// we want the closest
			iDist = 
			pGameData->GetRangeDistance( hexCarrier, hexCargo );
			if( iDist && iDist < iBest )
			{
				iBest = iDist;
				pCargo = pUnit;
			}
		}
	}

	if( pCargo != NULL )
	{
		bFound = TRUE;
		// consider moving toward cargo
		if( iBest > 3 )
		{
			hexCargo.X(0);
			hexCargo.Y(0);
			EnterCriticalSection (&cs);
			pVehicle = theVehicleMap.GetVehicle( pCargo->GetID() );
			if( pVehicle != NULL )
				hexCargo = pVehicle->GetHexHead();
			LeaveCriticalSection (&cs);
			
			if( pVehicle == NULL )
				return( FALSE );

			if( hexCargo.X() || hexCargo.Y() )
			{
				pCarrier->SetDestination( hexCargo );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::FindPatrolCargo() carrier %d of player %d going to ",
pCarrier->GetID(), pCarrier->GetOwner() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"cargo unit %ld at %d,%d \n", pCargo->GetID(), 
hexCargo.X(), hexCargo.Y() );
#endif
			}
		}
		// effect assignments
		pCarrier->SetParamDW(CAI_GRDDEFENSE, pCargo->GetID());
		pCargo->SetParamDW(CAI_GRDDEFENSE, pCarrier->GetID());
	}

	return( bFound );
}

//
// look for an infantry_carrier unit with the same task that
// is not assigned to a cargo "GetParamDW(CAI_GRDDEFENSE) != 0"
// and assign it to this cargo and move toward its tail and when
// within one, issue load message, and if none found return FALSE
// and TRUE if one found
//
BOOL CAITaskMgr::FindPatrolCarrier( CAIUnit *pCargo, CAITask *pTask )
{
	BOOL bFound = FALSE;
	int iDist, iBest=0xFFFE;
	CAIUnit *pCarrier = NULL;
	CHexCoord hexCarrier, hexCargo;

	EnterCriticalSection (&cs);
	CVehicle *pVehicle = 
		theVehicleMap.GetVehicle( pCargo->GetID() );
	if( pVehicle == NULL )
	{
		LeaveCriticalSection (&cs);
		return( FALSE );
	}
	hexCargo = pVehicle->GetHexHead();
	LeaveCriticalSection (&cs);

    POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
       	CAIUnit *pUnit = 
          	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			if( pUnit->GetGoal() != pTask->GetGoalID() ||
				pUnit->GetTask() != pTask->GetID() )
				continue;

			// land only with IFVs
			if( pUnit->GetTypeUnit() != CTransportData::infantry_carrier )
				continue;

			// skip vehicles enroute to pick up an infantry
			if( pUnit->GetParamDW(CAI_GRDDEFENSE) )
				continue;

			hexCarrier.X(0);
			hexCarrier.Y(0);
			EnterCriticalSection (&cs);
			pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
			if( pVehicle == NULL )
			{
				LeaveCriticalSection (&cs);
				continue;
			}

			if( !pVehicle->GetCargoCount() )
				hexCarrier = pVehicle->GetHexHead();
			LeaveCriticalSection (&cs);

			// skip vehicles already carrying
			if( !hexCarrier.X() && !hexCarrier.Y() )
				continue;

			// we want the closest
			iDist = 
			pGameData->GetRangeDistance( hexCarrier, hexCargo );
			if( iDist && iDist < iBest )
			{
				iBest = iDist;
				pCarrier = pUnit;
			}
		}
	}

	if( pCarrier != NULL )
	{		
		CSubHex subHexTail,subHexHead;
		EnterCriticalSection (&cs);
		pVehicle = theVehicleMap.GetVehicle( pCarrier->GetID() );
		if( pVehicle != NULL )
		{
			subHexTail = pVehicle->GetPtTail();
			subHexHead = pVehicle->GetPtHead();
		}

		LeaveCriticalSection (&cs);

		if( pVehicle == NULL )
			return( FALSE );

		// effect assignments
		pCarrier->SetParamDW(CAI_GRDDEFENSE, pCargo->GetID());
		pCargo->SetParamDW(CAI_GRDDEFENSE, pCarrier->GetID());

		CSubHex subHexCargo( hexCargo.X()*2, hexCargo.Y()*2 );

		iDist = subHexCargo.Dist( subHexTail );

		if( iDist > 1 )
		{
			// turn carrier hex to tail hex using one of Dave's
			// mysterious functions, and send vehicle to it
			subHexHead = Rotate( 0, subHexTail, subHexHead );
			pCargo->SetDestination( subHexHead );
			bFound = TRUE;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::FindPatrolCarrier(): cargo unit %ld of player %d ",
pCargo->GetID(), pCargo->GetOwner() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"moving toward carrier %ld at sub-hex %d,%d \n", 
pCarrier->GetID(), subHexHead.x, subHexHead.y );
#endif
		}
		else
		{
			pCarrier->LoadUnit( pCargo->GetID() );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAITaskMgr::FindPatrolCarrier(): carrier %ld of player %d loaded %ld a %d unit \n", 
pCarrier->GetOwner(), pCarrier->GetID(), 
pCargo->GetID(), pCargo->GetTypeUnit() );
#endif
		}
	}

	return( bFound );
}
//
// consider known opfor vehicles of the owner that the passed unit
// is targeting and if any exist, return TRUE, else FALSE
//
BOOL CAITaskMgr::TargetVehicles( CAIUnit *pAIUnit )
{
	CAIUnit *pOpForUnit = 
		m_pGoalMgr->m_plUnits->GetOpForUnit( pAIUnit->GetDataDW() );
	if( pOpForUnit == NULL )
		return FALSE;

	if( m_pGoalMgr->m_plUnits != NULL )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIUnit *pUnit = 
            	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
            if( pUnit != NULL )
            {
            	ASSERT_VALID( pUnit );

				if( pUnit->GetOwner() != pOpForUnit->GetOwner() )
					continue;
				if( pUnit->GetType() == CUnit::vehicle )
					return TRUE;
			}
		}
	}
	return FALSE;
}
//
// a delete_unit message has just been received so clear
// any units targeting the destroyed unit, so they will
// select another target or be reassigned a new task
//
void CAITaskMgr::ClearTarget( CAIMsg *pMsg )
{
	if( m_pGoalMgr->m_plUnits != NULL )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        POSITION pos = m_pGoalMgr->m_plUnits->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIUnit *pUnit = 
            	(CAIUnit *)m_pGoalMgr->m_plUnits->GetNext( pos );
            if( pUnit != NULL )
            {
            	ASSERT_VALID( pUnit );

				if( pUnit->GetOwner() != m_iPlayer )
					continue;

				if( pUnit->GetTask() == IDT_SEEKINRANGE ||
					pUnit->GetTask() == IDT_SEEKATSEA ||
					pUnit->GetTask() == IDT_SEEKINWAR )
				{
					// this unit had been attacking the unit
					// that was just reported as destroyed
					if( pUnit->GetDataDW() ==
						pMsg->m_dwID )
					{
						pUnit->SetDataDW(0);
						pUnit->SetParam(CAI_TARGETTYPE,0xFFFE );
					}
				}
			}
		}
	}
}
//
// clean up on destruction
//
CAITaskMgr::~CAITaskMgr()
{
	ASSERT_VALID( this );
    //DeleteTasks();
}

// end of CAITMgr.cpp
