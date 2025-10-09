////////////////////////////////////////////////////////////////////////////
//
//  CAIMgr.cpp :  CAIMgr object implementation
//                Divide and Conquer AI
//
//  Last update:  10/28/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "caimgr.hpp"

#include "caidata.hpp"
#include "caiinit.hpp"
#include "caisavld.hpp"
#include "caistart.hpp"
#include "cpathmap.h"
#include "logging.h"  // dave's logging system
#include "stdafx.h"
#include "version.h"

#define new DEBUG_NEW
#define AI_IDLE_LIMIT 20000


extern CAITaskList* plTaskList;  // standard CAITask list
extern CAIGoalList* plGoalList;  // standard CAIGoal list
// extern CWordArray *pwaRCIP;		// racial characteristics and initial pos
extern CWordArray*      pwaIG;       // initial goals
extern CException*      pException;  // standard exception for yielding
extern CRITICAL_SECTION cs;          // used by threads
extern CAIData*         pGameData;   // pointer to game data interface
extern CPathMap         thePathMap;  // the map pathfinding object (no yield)

extern void AiDeletePlayer( DWORD dwID );

CAIMgr::CAIMgr( int iPlayer )
{
    m_iPlayer = iPlayer;
    m_bIsAI   = TRUE;
    m_bIsDead = FALSE;

    m_iIdle = 0;
    for ( int i = 0; i < MAX_IDLE_FUNCTIONS; ++i ) m_bIdleFunction[i] = TRUE;

    m_pGoalMgr = NULL;
    m_pTaskMgr = NULL;
    m_pRouter  = NULL;
    m_plOpFors = NULL;
    m_plUnits  = NULL;

    m_pMap       = NULL;
    m_plMsgQueue = NULL;
    m_plTmpQueue = NULL;

    m_bRelationChange   = FALSE;
    m_bTaskChange       = FALSE;
    m_bGoalAssess       = FALSE;
    m_bProductionChange = FALSE;
    m_bMapChanged       = FALSE;
    m_bAttackOccurred   = FALSE;
    m_bVehicleArrived   = FALSE;
    m_bVehicleError     = FALSE;
    m_bUnitDestroyed    = FALSE;
    m_bLocChanged       = FALSE;
    m_bCheckResearch    = FALSE;
    m_bPlaceRocket      = FALSE;
    m_bPlayerDying      = FALSE;
    m_bScenario         = FALSE;
    m_bAttackUnit       = FALSE;
    m_bAttackPlayer     = FALSE;
    m_bPlaceBldg        = FALSE;
    m_bOutLOS           = FALSE;
    m_bUnitLoaded       = FALSE;
    m_bGiveUnit         = FALSE;

    m_iAIShooter = 0;
    m_iHPShooter = 0;

    // private critical section for accessing the message queue
    memset( &m_cs, 0, sizeof( m_cs ) );
    InitializeCriticalSection( &m_cs );

    // messages after prioritizing
    if ( m_plMsgQueue == NULL )
    {
        try
        {
            m_plMsgQueue = new CObList( );
            // m_plMsgQueue = new CList<CAIMsg*,CAIMsg*>();
        }
        catch ( CException* e )
        {
            // how should this be reported?
            m_plMsgQueue = NULL;
            throw( ERR_CAI_BAD_NEW );
        }
    }
    // messages in first in first out order
    if ( m_plTmpQueue == NULL )
    {
        try
        {
            m_plTmpQueue = new CObList( );
            // m_plTmpQueue = new CList<CAIMsg*,CAIMsg*>();
        }
        catch ( CException* e )
        {
            // how should this be reported?
            m_plTmpQueue = NULL;
            throw( ERR_CAI_BAD_NEW );
        }
    }

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr constructed for player %d difficulty is %d version %s", m_iPlayer,
               pGameData->m_iSmart, VER_STRING );
#endif

    ASSERT_VALID( this );
}

// BUGBUG called from threads function so it must perform incremental
// management and return
//
void CAIMgr::Manage( void )
{
    ASSERT_VALID( this );

    if ( m_bIsDead )
    {
        // probably need to send a message to the game so it will
        // call AiDeletePlayer() to actually delete this player
        AiDeletePlayer( (DWORD)this );
        myThreadTerminate( );
        return;
    }

    // priortize any messages held in tempory queue m_plTmpQueue
    PrioritizeMessage( );
    CAIMsg* pMsg = NULL;

    // get next message from message queue
    // need to critical section bracket this: EnterCriticalSection
    if ( !m_plMsgQueue->IsEmpty( ) )
    {
        EnterCriticalSection( &m_cs );
        pMsg = (CAIMsg*)m_plMsgQueue->RemoveHead( );
        LeaveCriticalSection( &m_cs );
        m_iIdle = 0;
    }
    else
        m_iIdle++;  // inc when there is no message to process

    // delete early so that it don't leak if closed
    // in the middle of this function
    if ( pMsg != NULL )
    {
        CAIMsg aMsg( pMsg );
        delete pMsg;
        pMsg = &aMsg;
    }

    // figure out what the message is about
    ProcessMessage( pMsg );
    if ( pMsg == NULL )
    {
        // reset all state flags
        m_bAttackOccurred   = FALSE;
        m_bProductionChange = FALSE;
        m_bGoalAssess       = FALSE;
        m_bRelationChange   = FALSE;
        m_bMapChanged       = FALSE;
        m_bVehicleArrived   = FALSE;
        m_bVehicleError     = FALSE;
        m_bUnitDestroyed    = FALSE;
        m_bLocChanged       = FALSE;
        m_bCheckResearch    = FALSE;
        m_bPlaceRocket      = FALSE;
        m_bPlayerDying      = FALSE;
        m_bScenario         = FALSE;
        m_bAttackUnit       = FALSE;
        m_bAttackPlayer     = FALSE;
        m_bPlaceBldg        = FALSE;
        m_bOutLOS           = FALSE;
        m_bUnitLoaded       = FALSE;
    }

    // test for idle time processing
    if ( m_iIdle > AI_IDLE_LIMIT )
    {
        m_iIdle = 0;

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::Manage() player %d idle function, messages %d ", m_iPlayer,
                   m_plMsgQueue->GetCount( ) );
#endif

        // call processes that could work without a message
        //
        // alternating down the m_bIdleFunction[] to cause
        // a different idle time process to occur

        // router might have stuff to do
        if ( m_bIdleFunction[0] )
        {
#ifdef _LOGOUT
            ReportPlayerStats( );
#endif
            // BUGBUG turn off by commenting
            m_pRouter->FillPriorities( );
            m_bIdleFunction[0] = FALSE;
        }
        // touch all the units
        else if ( m_bIdleFunction[1] )
        {
            // BUGBUG turn off by commenting
            m_pTaskMgr->AssignUnits( );
            m_bIdleFunction[1] = FALSE;
        }
        // update map completely
        else if ( m_bIdleFunction[2] )
        {
            // BUGBUG turn off by commenting
            m_pMap->UpdateMap( NULL );
            m_bIdleFunction[2] = FALSE;
        }
        // review construction task priorities
        else if ( m_bIdleFunction[3] )
        {
            // BUGBUG turn off by commenting
            m_pGoalMgr->UpdateConstructionTasks( NULL );
            m_bIdleFunction[3] = FALSE;
        }
        // review production task priorities
        else if ( m_bIdleFunction[4] )
        {
            // BUGBUG turn off by commenting
            m_pGoalMgr->UpdateProductionTasks( NULL );
            m_bIdleFunction[4] = FALSE;
        }
        else if ( m_bIdleFunction[5] )
        {
            // BUGBUG turn off by commenting
            m_pTaskMgr->BalancePatrols( );
            m_bIdleFunction[5] = FALSE;
        }
        else if ( m_bIdleFunction[6] )
        {
            // BUGBUG turn off by commenting
            HandleStuckVehicles( );
            m_bIdleFunction[6] = FALSE;
        }
        else
        {
            Sleep( pGameData->GetRandom( 100 ) );

            // reset this time and start over
            for ( int i = 0; i < MAX_IDLE_FUNCTIONS; ++i ) m_bIdleFunction[i] = TRUE;
        }
    }

    // an HP unit was given to another HP unit
    if ( m_bGiveUnit )
    {
        CAIUnit* pUnit = m_plUnits->GetOpForUnit( pMsg->m_dwID );
        if ( pUnit != NULL )
            pUnit->SetOwner( pMsg->m_idata1 );
    }

    if ( m_bPlayerDying )
    {
        UpdatePlayer( pMsg );
    }

    if ( m_bUnitDestroyed )
    {
        m_pRouter->DoRouting( pMsg );
        m_pGoalMgr->Assess( pMsg );
        UpdateUnits( pMsg );
    }

    if ( m_bUnitLoaded )
        LoadedResponse( pMsg );

    // one of our vehicles entered a new hex, so tell map
    if ( m_bLocChanged )
    {
        // UpdateLocation( pMsg );
        m_pMap->UpdateLoc( pMsg );
    }

    // scenario states
    if ( m_bScenario )
    {
        // scenarios start numbering at ZERO
        m_iScenario = pMsg->m_idata1 + 1;
        m_pGoalMgr->Scenario( m_iScenario );

        m_bGoalAssess = TRUE;
    }

    if ( m_bAttackUnit )
    {
        AttackUnit( pMsg );
    }
    if ( m_bAttackPlayer )
    {
        // pass id of player to attack
        m_pGoalMgr->AttackPlayer( pMsg->m_idata1 );
    }

    // the game needs a place to put a non-controlled building
    if ( m_bPlaceBldg )
    {
        PlaceBldg( pMsg );
    }

    // a prior attack was occuring
    if ( m_bOutLOS )
        OutLOSResponse( pMsg );

    if ( m_bAttackOccurred )
    {
        // the building/vehicle attacked should be
        // in the last stored message.
        // if vehicle, send an attack message to
        // the vehicle.
        // else if building, find nearest defending
        // vehicles, and if available, and not already
        // engaged, send CNetVehStat with building
        // as new destination
        AttackResponse( pMsg );

#if THREADS_ENABLED
        // BUGBUG this function must yield
        myYieldThread( );
        // if( myYieldThread() == TM_QUIT )
        //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        m_pGoalMgr->ConsiderThreats( pMsg );
    }

    if ( m_bVehicleArrived )
    {
        // m_pMap->UpdateLoc( pMsg );
        //  the vehicle reporting should be pMsg->m_dwID

        // find the vehicle in CAIUnit
        // find the current task of this vehicle
        // determine next task order if continuing
        // and issue new order to vehicle for task
        DestinationResponse( pMsg );
    }

    if ( m_bVehicleError )
    {
        VehicleErrorResponse( pMsg );
    }

    if ( m_bMapChanged )
    {
        m_pGoalMgr->SetMapChange( );
        m_pMap->UpdateMap( pMsg );

        if ( !m_bUnitDestroyed )
            UpdateUnits( pMsg );
    }

    // assess "goals needed" based on map status
    //    Are all threats being addressed
    //    Are all resources available
    if ( m_bGoalAssess )
    {
        // set states in goal manager
        if ( m_bProductionChange )
        {
            m_pGoalMgr->SetProductionChange( );
            if ( !m_bUnitDestroyed )
            {
                // special case because we are getting multiple
                // veh_new messages for the same vehicle
                /*
                if( pMsg->m_iMsg == CNetCmd::veh_new &&
                    pMsg->m_dwID && pMsg->m_idata3 == m_iPlayer )
                {
                    // make sure this unit does not already exist and if
                    // it does, then skip goalmgr::Assess() call
                    CAIUnit *pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
                    if( pUnit != NULL )
                        goto FinalUpdates;
                }
                */
                UpdateUnits( pMsg );
            }
        }

        // now assess the goals
        // with current CAIMsg *pMsg
        if ( !m_bUnitDestroyed )
            m_pGoalMgr->Assess( pMsg );
    }

    if ( m_bProductionChange )
    {
        m_pGoalMgr->CheckPlayer( );

        if ( !m_bAttackOccurred && pMsg != NULL )
            m_pGoalMgr->AdjustThreats( );

#if THREADS_ENABLED
        // BUGBUG this function must yield
        myYieldThread( );
        // if( myYieldThread() == TM_QUIT )
        //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

#ifdef _LOGOUT
        ReportPlayerStats( );
#endif
    }

    if ( m_bProductionChange || m_bUnitDestroyed )
    {
        // some production changes occur without effecting
        // goals or the map, such as the completion of the
        // construction of a building
        if ( !m_bGoalAssess && !m_bMapChanged && !m_bUnitDestroyed )
            UpdateUnits( pMsg );

#if THREADS_ENABLED
        // BUGBUG this function must yield
        myYieldThread( );
        // if( myYieldThread() == TM_QUIT )
        //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        m_pMap->PlanRoads( pMsg );

        // pass on goalmgr's opinion of gas level
        if ( m_pGoalMgr->NeedGas( ) )
            m_pRouter->m_bNeedGas = TRUE;
        else
            m_pRouter->m_bNeedGas = FALSE;

        if ( !m_bUnitDestroyed )
            m_pRouter->DoRouting( pMsg );

        // if trucks are needed for routing, the CAIMgr
        // needs to alert the goal manager of this need
        if ( m_pRouter->NeedTransports( ) )
        {
            // the truck may not be assigned to transport, which
            // means that when it is, its params are cleared
            // if( !m_pTaskMgr->AssignNewTrucks() )
            m_pGoalMgr->NeedTrucks( );
        }
    }

    // FinalUpdates:
    //  consider any changes to the tasks of this manager
    m_pTaskMgr->Manage( pMsg );

    if ( m_bCheckResearch )
    {
        m_pGoalMgr->CheckResearch( );
        m_pGoalMgr->UpdateConstructionTasks( NULL );
        m_pGoalMgr->UpdateProductionTasks( NULL );
        m_pTaskMgr->CheckFactorys( );
    }

    if ( m_bPlaceRocket )
        ReplaceRocket( pMsg );

    // reset all state flags
    m_bAttackOccurred   = FALSE;
    m_bProductionChange = FALSE;
    m_bGoalAssess       = FALSE;
    m_bRelationChange   = FALSE;
    m_bMapChanged       = FALSE;
    m_bVehicleArrived   = FALSE;
    m_bVehicleError     = FALSE;
    m_bUnitDestroyed    = FALSE;
    m_bLocChanged       = FALSE;
    m_bCheckResearch    = FALSE;
    m_bPlaceRocket      = FALSE;
    m_bPlayerDying      = FALSE;
    m_bScenario         = FALSE;
    m_bAttackUnit       = FALSE;
    m_bAttackPlayer     = FALSE;
    m_bPlaceBldg        = FALSE;
    m_bOutLOS           = FALSE;
    m_bUnitLoaded       = FALSE;
    m_bGiveUnit         = FALSE;
}

void CAIMgr::SetDead( void )
{
    m_bIsDead = TRUE;

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::The player %d is dead \n", m_iPlayer );
#endif
}

int CAIMgr::GetPlayer( void )
{
    ASSERT_VALID( this );

    return m_iPlayer;
}

BOOL CAIMgr::IsAI( void )
{
    return m_bIsAI;
}

void CAIMgr::SetAI( BOOL bAI )
{
    m_bIsAI = bAI;
}

//
// responds to a build_civ message and places a civ building
//
void CAIMgr::PlaceBldg( CAIMsg* pMsg )
{
    if ( pMsg == NULL )
        return;
    if ( pMsg->m_idata3 != m_iPlayer )
        return;

    m_pMap->PlaceBuilding( pMsg, m_plUnits );
}

void CAIMgr::UpdateLocation( CAIMsg* pMsg )
{
    CAIUnit* pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
    if ( pUnit != NULL )
        return;
    // only explorers cause update
    if ( pUnit->GetTask( ) == IDT_SCOUT )
        m_pMap->UpdateLoc( pMsg );
}

//
// the player in pMsg->m_idata3 is not this player and
// is dying, so remove all its units and the opfor
//
void CAIMgr::UpdatePlayer( CAIMsg* pMsg )
{
    if ( pMsg == NULL )
        return;
    if ( pMsg->m_idata3 == m_iPlayer )
        return;

    m_plUnits->RemoveUnits( pMsg->m_idata3 );
    m_plOpFors->RemoveOpFor( pMsg->m_idata3 );
    m_pMap->UpdateMap( NULL );
}

//
// the unit in the pMsg->m_dwID has possibly caused
// a need to update the m_plUnits list
//
void CAIMgr::UpdateUnits( CAIMsg* pMsg )
{
    if ( pMsg == NULL )
        return;

    // handle started construction or production completed
    if ( pMsg->m_iMsg == CNetCmd::veh_new || pMsg->m_iMsg == CNetCmd::bldg_new )
    {
        if ( !pMsg->m_dwID )
            return;

        // consider opfor units, for high difficulty games
        // if this is a rocket, then we want to add it even
        // though it has not been spotted
        if ( pMsg->m_idata3 != m_iPlayer )
        {
            if ( pGameData->m_iSmart )
            {
                if ( pMsg->m_iMsg == CNetCmd::bldg_new )
                {
                    if ( pMsg->m_idata1 != CStructureData::rocket )
                        return;
                }
                else
                    return;
            }
            else
                return;
        }

        // make sure this unit does not already exist
        CAIUnit* pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
        if ( pUnit != NULL )
            return;

        // determine unit type (building/vehicle)
        int iType = pMsg->m_iMsg == CNetCmd::veh_new ? CUnit::vehicle : CUnit::building;

        try
        {
            // CAIUnit( DWORD dwID, int iOwner,
            // int iType, int iTypeUnit );
            pUnit = new CAIUnit( pMsg->m_dwID, pMsg->m_idata3, iType, pMsg->m_idata1 );
            ASSERT_VALID( pUnit );
            m_plUnits->AddTail( (CObject*)pUnit );
        }
        catch ( CException* e )
        {
            // need to report this error occurred
            throw( ERR_CAI_BAD_NEW );
        }

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::UpdateUnits(): player %d created new unit id=%ld type=%d \n",
                   pUnit->GetOwner( ), pUnit->GetID( ), pUnit->GetTypeUnit( ) );
#endif
        // what, you don't trust the constructor?
        pUnit->SetDataDW( (DWORD)0 );
        pUnit->SetTask( FALSE );
        pUnit->SetGoal( FALSE );
        pUnit->ClearParam( );
    }

    WORD     wTask, wGoal;
    CAIUnit* pUnit;

    // handle 'dead' units
    if ( pMsg->m_iMsg == CNetCmd::delete_unit )
    {
        pUnit = (CAIUnit*)m_plUnits->GetUnit( pMsg->m_dwID );
        if ( pUnit != NULL )
        {
            // handle goals/tasks for this player only
            if ( pMsg->m_idata3 == m_iPlayer )
            {
                wTask = pUnit->GetTask( );
                wGoal = pUnit->GetGoal( );
                m_pTaskMgr->UnAssignTask( wTask, wGoal );
            }
            else
            {
                // clear any units targeting this guy
                // pUnit->SetDataDW(0);
                m_plUnits->ClearTarget( pMsg->m_dwID );
            }
            m_plUnits->RemoveUnit( pMsg->m_dwID );
        }

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::UpdateUnits(): player %d unit deleted: id=%ld player=%d \n",
                   m_iPlayer, pMsg->m_dwID, pMsg->m_idata3 );
#endif

        // AI is KIA
        if ( !m_plUnits->GetCount( ) )
            ResignGame( );
        return;
    }

    // if it was a construction begining by a crane
    // then capture the id of the building to build
    if ( pMsg->m_iMsg == CNetCmd::bldg_new && pMsg->m_dwID2 != NULL && pMsg->m_idata3 == m_iPlayer )
    {
        pUnit = (CAIUnit*)m_plUnits->GetUnit( pMsg->m_dwID2 );
        if ( pUnit != NULL )
            pUnit->SetDataDW( pMsg->m_dwID );  // building id
    }


    // if it was a building completed by a crane then
    // update all cranes working on the building as
    // to the building id and their tasks assigned
    if ( pMsg->m_iMsg == CNetCmd::bldg_stat && pMsg->m_uFlags == CMsgBldgStat::built && pMsg->m_idata2 == 100 &&
         pMsg->m_idata3 == m_iPlayer )
    {
#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::UpdateUnits(): player %d construction of a %d completed ",
                   m_iPlayer, pMsg->m_idata1 );
#endif


#if THREADS_ENABLED
        // BUGBUG this function must yield
        myYieldThread( );
        // if( myYieldThread() == TM_QUIT )
        //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        // reset the cranes that completed the building so they
        // can be available to help elsewhere

        POSITION pos = m_plUnits->GetHeadPosition( );
        while ( pos != NULL )
        {
            pUnit = (CAIUnit*)m_plUnits->GetNext( pos );
            if ( pUnit != NULL )
            {
                ASSERT_VALID( pUnit );

                if ( pUnit->GetOwner( ) != m_iPlayer )
                    continue;
                if ( pUnit->GetType( ) != CUnit::vehicle )
                    continue;
                if ( pUnit->GetDataDW( ) != pMsg->m_dwID )
                    continue;
                // handle cranes special

                if ( pUnit->GetTypeUnit( ) == CTransportData::construction )
                {
                    // this unit is a crane which just completed
                    // the construction of a building
                    wTask = pUnit->GetTask( );
                    wGoal = pUnit->GetGoal( );

#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                               "CAIMgr::UpdateUnits() player %d unit %ld unassigned from goal %d task %d ", m_iPlayer,
                               pUnit->GetID( ), wGoal, wTask );
#endif
                    // reset the task used for the construction
                    m_pTaskMgr->UnAssignTask( wTask, wGoal );

                    // reset the crane unit making it available again
                    pUnit->SetDataDW( (DWORD)0 );
                    pUnit->SetTask( FALSE );
                    pUnit->SetGoal( FALSE );
                    pUnit->ClearParam( );
                }
                else  // its a truck so don't unassign it
                {
                    if ( pGameData->IsTruck( pUnit->GetID( ) ) )
                        m_pRouter->UnassignTruck( pUnit->GetID( ) );
                }

                // now move the crane away from the building and into
                // an unoccupied location some distance away from it
                CHexCoord hexDest = m_pMap->m_pMapUtil->m_RocketHex;
                m_pMap->GetStagingHex( m_pMap->m_pMapUtil->m_RocketHex, 2, 2, pUnit->GetTypeUnit( ), hexDest );

                // if( hexDest.X() != m_pMap->m_RocketHex.X() &&
                //	hexDest.Y() != m_pMap->m_RocketHex.Y() )
                if ( hexDest == m_pMap->m_pMapUtil->m_RocketHex )
                {
                    // try again
                    m_pMap->GetStagingHex( m_pMap->m_pMapUtil->m_RocketHex, 2, 2, pUnit->GetTypeUnit( ), hexDest,
                                           FALSE );
                }
                pUnit->SetDestination( hexDest );
                m_pMap->PlaceFakeVeh( hexDest, pUnit->GetTypeUnit( ) );
            }
        }
    }

    if ( pMsg->m_iMsg == CNetCmd::err_build_road && pMsg->m_idata3 == m_iPlayer )
    {
        CAIUnit* pUnit = (CAIUnit*)m_plUnits->GetUnit( pMsg->m_dwID );
        if ( pUnit != NULL )
        {
            // this unit is a crane which just completed
            // the construction of a building
            wTask = pUnit->GetTask( );
            wGoal = pUnit->GetGoal( );

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                       "CAIMgr::UpdateUnits() player %d road building crane %ld road error goal %d task %d ", m_iPlayer,
                       pUnit->GetID( ), wGoal, wTask );
#endif
            // reset the task used for the construction
            m_pTaskMgr->UnAssignTask( wTask, wGoal );

            // reset the crane unit making it available again
            pUnit->SetDataDW( (DWORD)0 );
            pUnit->SetTask( FALSE );
            pUnit->SetGoal( FALSE );
            pUnit->ClearParam( );

            // now move the crane away from the rocket and into
            // an unoccupied location some distance away from it
            CHexCoord hexDest = m_pMap->m_pMapUtil->m_RocketHex;

            // m_pMap->GetStagingHex( pUnit, NULL, hexDest );
            // m_pMap->m_pMapUtil->FindStagingHex( m_pMap->m_RocketHex,
            //	1,1,pUnit->GetTypeUnit(), hexDest );
            m_pMap->GetStagingHex( m_pMap->m_pMapUtil->m_RocketHex, 2, 2, pUnit->GetTypeUnit( ), hexDest );
            if ( hexDest.X( ) != m_pMap->m_pMapUtil->m_RocketHex.X( ) &&
                 hexDest.Y( ) != m_pMap->m_pMapUtil->m_RocketHex.Y( ) )
            {
                pUnit->SetDestination( hexDest );
                m_pMap->PlaceFakeVeh( hexDest, pUnit->GetTypeUnit( ) );
            }
        }
    }

    // handle road done messages
    if ( pMsg->m_iMsg == CNetCmd::road_done && pMsg->m_idata3 == m_iPlayer )
    {
        CAIUnit* pUnit = (CAIUnit*)m_plUnits->GetUnit( pMsg->m_dwID );
        if ( pUnit != NULL )
        {
            // this unit is a crane which just completed
            // the construction of a building
            wTask = pUnit->GetTask( );
            wGoal = pUnit->GetGoal( );

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                       "CAIMgr::UpdateUnits() player %d road building crane %ld unassigned goal %d task %d ", m_iPlayer,
                       pUnit->GetID( ), wGoal, wTask );
#endif
            // update number of MSW_PLANNED_ROAD locations left
            m_pMap->m_iRoadCount--;
            // reset the task used for the construction
            m_pTaskMgr->UnAssignTask( wTask, wGoal );
            if ( m_pMap->m_iRoadCount )
                m_pGoalMgr->ConsiderRoads( );

            // reset the crane unit making it available again
            pUnit->SetDataDW( (DWORD)0 );
            pUnit->SetTask( FALSE );
            pUnit->SetGoal( FALSE );
            pUnit->ClearParam( );

            // now move the crane away from the rocket and into
            // an unoccupied location some distance away from it
            // CHexCoord hexDest = m_pMap->m_pMapUtil->m_RocketHex;
            CHexCoord hexDest( pMsg->m_iX, pMsg->m_iY );
            CHexCoord hexOld = hexDest;
            m_pMap->GetStagingHex( hexDest, 2, 2, pUnit->GetTypeUnit( ), hexDest );
            if ( hexDest.X( ) != hexOld.X( ) && hexDest.Y( ) != hexOld.Y( ) )
            {
                pUnit->SetDestination( hexDest );
                m_pMap->PlaceFakeVeh( hexDest, pUnit->GetTypeUnit( ) );
            }
            else
            {
                // m_pMap->GetStagingHex( pUnit, NULL, hexDest );
                // m_pMap->m_pMapUtil->FindStagingHex( m_pMap->m_RocketHex,
                //	1,1,pUnit->GetTypeUnit(), hexDest );
                hexDest = m_pMap->m_pMapUtil->m_RocketHex;
                m_pMap->GetStagingHex( m_pMap->m_pMapUtil->m_RocketHex, 2, 2, pUnit->GetTypeUnit( ), hexDest );
                if ( hexDest.X( ) != m_pMap->m_pMapUtil->m_RocketHex.X( ) &&
                     hexDest.Y( ) != m_pMap->m_pMapUtil->m_RocketHex.Y( ) )
                {
                    pUnit->SetDestination( hexDest );
                    m_pMap->PlaceFakeVeh( hexDest, pUnit->GetTypeUnit( ) );
                }
            }
        }
    }
}

//
// this is called during idle time processing
// to go thru all trucks and cranes and check to
// see if the truck or crane is stuck.  stuck
// is defined as not having arrived at its dest
//
void CAIMgr::HandleStuckVehicles( void )
{
#if THREADS_ENABLED
    myYieldThread( );
#endif

    // get current time for later use
    DWORD     dwNow = timeGetTime( ), dwUnitTime, dwDiff;
    CHexCoord hexDest, hexVeh, hex;
    BOOL      bIsWorking, bIsCarried;

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::HandleStuckVehicles() player %d called at %ld  ", m_iPlayer,
               dwNow );
#endif

    POSITION pos = m_plUnits->GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pUnit = (CAIUnit*)m_plUnits->GetNext( pos );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            if ( pUnit->GetOwner( ) != m_iPlayer )
                continue;
            if ( pUnit->GetType( ) != CUnit::vehicle )
                continue;
            if ( pUnit->GetTypeUnit( ) != CTransportData::construction &&
                 pUnit->GetTypeUnit( ) != CTransportData::heavy_truck )
                continue;

            bIsWorking = bIsCarried = FALSE;

            // get current destination of vehicle
            EnterCriticalSection( &cs );
            CVehicle* pVeh = theVehicleMap.GetVehicle( pUnit->GetID( ) );
            if ( pVeh == NULL )
            {
                LeaveCriticalSection( &cs );
                pUnit->SetParamDW( CAI_ROUTE_X, 0 );
                continue;
            }

            // test to be sure vehicle is not already carried
            if ( pVeh->GetTransport( ) != NULL )
                bIsCarried = TRUE;

            hexVeh  = pVeh->GetHexHead( );
            hexDest = pVeh->GetHexDest( );

            if ( pUnit->GetTypeUnit( ) == CTransportData::construction )
            {
                if ( pVeh->GetRouteMode( ) == CVehicle::run )
                    bIsWorking = TRUE;
            }

            LeaveCriticalSection( &cs );

            if ( bIsCarried || bIsWorking )
                continue;

            // has this vehicle been given time?
            dwUnitTime = pUnit->GetParamDW( CAI_ROUTE_X );  // last time
            if ( !dwUnitTime || dwUnitTime > dwNow )
            {
                pUnit->SetParamDW( CAI_ROUTE_X, timeGetTime( ) );
                continue;
            }

            // is it time to check this vehicle?
            dwDiff = dwNow - dwUnitTime;

            // 5 minute test
            if ( dwDiff > 300000 && dwDiff < 600000 )
            {
                // vehicle is not stuck, but is where it needs to be
                if ( hexVeh == hexDest )
                {
                    pUnit->SetParamDW( CAI_ROUTE_X, 0 );
                    pUnit->SetParamDW( CAI_ROUTE_Y, 0 );
                    continue;
                }

                // make unit go to last destination
                pUnit->SetDestination( hexDest );
                // last hex LOWORD(X), HIWORD(Y) occupied
                pUnit->SetParamDW( CAI_ROUTE_Y, (DWORD)MAKELPARAM( hexVeh.X( ), hexVeh.Y( ) ) );
#ifdef _LOGOUT
                logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                           "CAIMgr::HandleStuckVehicles() player %d unit %d a %d is stuck after 5 min, resent to %d,%d "
                           "at %ld ",
                           m_iPlayer, pUnit->GetID( ), pUnit->GetTypeUnit( ), hexDest.X( ), hexDest.Y( ), dwUnitTime );
#endif
            }

            // 10 minute test
            if ( dwDiff > 600000 )
            {
                // vehicle is not stuck, but is where it needs to be
                if ( hexVeh == hexDest )
                {
                    pUnit->SetParamDW( CAI_ROUTE_X, 0 );
                    pUnit->SetParamDW( CAI_ROUTE_Y, 0 );
                    continue;
                }

                // get unit's last location
                hex = hexDest;
                hexDest.X( LOWORD( pUnit->GetParamDW( CAI_ROUTE_Y ) ) );
                hexDest.Y( HIWORD( pUnit->GetParamDW( CAI_ROUTE_Y ) ) );

                int iDist = pGameData->GetRangeDistance( hexVeh, hexDest );

                // if still too far away
                if ( iDist > 2 )
                {
                    // make unit go to last destination
                    pUnit->SetDestination( hex );
                    // last hex LOWORD(X), HIWORD(Y) occupied
                    pUnit->SetParamDW( CAI_ROUTE_Y, (DWORD)MAKELPARAM( hexVeh.X( ), hexVeh.Y( ) ) );
#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                               "CAIMgr::HandleStuckVehicles() player %d unit %d a %d is stuck after 10 min, resent to "
                               "%d,%d at %ld ",
                               m_iPlayer, pUnit->GetID( ), pUnit->GetTypeUnit( ), hexDest.X( ), hexDest.Y( ),
                               dwUnitTime );
#endif
                }
                else
                {
                    // ignore this vehicle? how? no vehicles are ever ignored!
                    pUnit->SetDataDW( (DWORD)0 );

                    if ( pUnit->GetTypeUnit( ) == CTransportData::construction )
                    {
                        // unassign crane task
                        m_pTaskMgr->UnAssignTask( pUnit->GetTask( ), pUnit->GetGoal( ) );
                    }

                    pUnit->SetTask( FALSE );
                    pUnit->SetGoal( FALSE );
                    pUnit->ClearParam( );

                    // place the vehicle in the rocket?
                    hexDest = m_pMap->m_pMapUtil->m_RocketHex;
                    CMsgPlaceVeh msg( hexDest, hexDest, m_iPlayer, pUnit->GetTypeUnit( ) );
                    msg.m_dwID = pUnit->GetID( );
                    theGame.PostToServer( (CNetCmd*)&msg, sizeof( CMsgPlaceVeh ) );
#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                               "CAIMgr::HandleStuckVehicles() player %d unit %d a %d is stuck after 10 min, placed at "
                               "%d,%d at %ld ",
                               m_iPlayer, pUnit->GetID( ), pUnit->GetTypeUnit( ), hexDest.X( ), hexDest.Y( ),
                               dwUnitTime );
#endif
                }
            }
        }
    }
}

//
// tell the game, the AI gives up
//
void CAIMgr::ResignGame( void )
{
}

//
// determine if the message reports a deadly embrace
//
BOOL CAIMgr::IsEmbraced( CAIMsg* pMsg )
{
    CHexCoord hexNext( pMsg->m_idata1, pMsg->m_idata2 );
    CHexCoord hexDest( pMsg->m_ieX, pMsg->m_ieY );
    CHexCoord hexVeh( pMsg->m_iX, pMsg->m_iY );

    // next hex is probably occupied,
    // if it is a vehicle, then check its next hex
    // and if it is the same as hexVeh
    // then a deadly embrace exists

    CSubHex   sub( hexNext );
    CSubHex   shNext;
    CVehicle* pVehicle = NULL;
    EnterCriticalSection( &cs );
    // this will find only the first vehicle in the hex
    for ( int iy = 0; iy < 2; ++iy )
    {
        sub.y = ( hexNext.Y( ) * 2 ) + iy;

        for ( int ix = 0; ix < 2; ++ix )
        {
            sub.x    = ( hexNext.X( ) * 2 ) + ix;
            pVehicle = theVehicleHex.GetVehicle( sub.x, sub.y );
            if ( pVehicle != NULL )
                break;
        }
        if ( pVehicle != NULL )
            break;
    }

    if ( pVehicle != NULL )
        shNext = pVehicle->GetPtNext( );
    LeaveCriticalSection( &cs );

    // no vehicle means it must be a building which ain't going to move
    if ( pVehicle == NULL )
        return TRUE;

    hexDest.X( shNext.x / 2 );
    hexDest.Y( shNext.y / 2 );

    // blocking occupant has next hex same as hex of blocked vehicle
    if ( hexVeh == hexDest )
        return TRUE;

    return FALSE;
}

//
// respond to a vehicle error message arriving
//
void CAIMgr::VehicleErrorResponse( CAIMsg* pMsg )
{
    ASSERT_VALID( this );

    if ( pMsg->m_idata3 != m_iPlayer )
        return;

    // get the unit that just arrived
    CAIUnit* pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
    if ( pUnit == NULL )
        return;

    // handle vehicles with a problem getting to destination
    if ( pMsg->m_iMsg == CNetCmd::err_veh_goto || pMsg->m_iMsg == CNetCmd::err_veh_traffic )
    {
        CHexCoord hexNext( pMsg->m_idata1, pMsg->m_idata2 );
        CHexCoord hexDest( pMsg->m_ieX, pMsg->m_ieY );
        CHexCoord hexVeh( pMsg->m_iX, pMsg->m_iY );

        // if the unit's current hex and dest hex match the message's
        // then the error message has not been acted on, otherwise
        // the unit is already responding to an error message
        CHexCoord hexGo( 0, 0 );
        CHexCoord hexAt( 0, 0 );
        BOOL      bIsLoaded = FALSE;

        EnterCriticalSection( &cs );
        CVehicle* pVeh = theVehicleMap.GetVehicle( pUnit->GetID( ) );
        if ( pVeh != NULL )
        {
            hexAt = pVeh->GetHexHead( );
            hexGo = pVeh->GetHexDest( );

            if ( pVeh->GetTransport( ) != NULL )
                bIsLoaded = TRUE;
        }
        LeaveCriticalSection( &cs );

        // ignore loaded vehicles that are blocked
        if ( bIsLoaded )
            return;
        if ( !hexAt.X( ) && !hexAt.Y( ) )
            return;
        if ( hexAt != hexVeh || hexGo != hexDest )
            return;

        // check the unit
        // if it has no task, or a combat task
        // then it is probably being staged or on patrol
        // so then its next hex or dest hex could be occupied
        // causing the blockage, so a good response would be
        // find another staging hex to go to

        // else it has no task or if its task is IDT_TRANSPORT then redirect
        if ( !pUnit->GetTask( ) || pUnit->GetTask( ) == IDT_SETTRANSPORT || pUnit->GetTask( ) == IDT_ATTACKUNIT )
        {
            if ( pUnit->GetTask( ) == IDT_ATTACKUNIT )
                hexDest = hexNext;

            // armies have staging areas, which I think is really cool
            // they like cluster in spots that, i think, are betweenish their base and target
            m_pMap->m_pMapUtil->FindStagingHex( hexVeh, 2, 2, pUnit->GetTypeUnit( ), hexDest, FALSE );

            pUnit->SetDestination( hexDest );

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::VehicleErrorResponse() respondinging to vehicle error " );
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "redirecting player %d unit %ld to %d,%d \n", m_iPlayer,
                       pMsg->m_dwID, hexDest.X( ), hexDest.Y( ) );
#endif
            return;
        }

        if ( pUnit->GetTask( ) == IDT_PATROL )
        {
            // get a new patrol hex (patrol? this is staging is it not?)
            if ( pUnit->GetParamDW( CAI_PATROL ) )
            {
                // m_pGoalMgr->GetPatrolHex( pUnit, hexDest );

                m_pMap->m_pMapUtil->FindStagingHex( hexVeh, 2, 2, pUnit->GetTypeUnit( ), hexDest, FALSE );

                pUnit->SetDestination( hexDest );
#ifdef _LOGOUT
                logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                           "CAIMgr::VehicleErrorResponse() respondinging to vehicle error " );
                logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "player %d patrol unit %ld now going to %d,%d \n", m_iPlayer,
                           pMsg->m_dwID, hexDest.X( ), hexDest.Y( ) );
#endif
                return;
            }
        }

        if ( pUnit->GetTypeUnit( ) == CTransportData::construction )
        {
            // handle cranes special
            //
            CHexCoord hexSite;

            // get the location where the crane may be trying to build
            if ( pUnit->GetParam( CAI_FUEL ) == CNetCmd::build_road )
            {
                hexSite.X( pUnit->GetParam( CAI_DEST_X ) );
                hexSite.Y( pUnit->GetParam( CAI_DEST_Y ) );
            }
            else if ( pUnit->GetParam( CAI_FUEL ) == CNetCmd::build_bridge )
            {
                hexSite.X( pUnit->GetParam( CAI_PREV_X ) );
                hexSite.Y( pUnit->GetParam( CAI_PREV_Y ) );
            }
            else  // all other types of construction have a build/crane site
            {
                // build site
                hexSite.X( pUnit->GetParam( BUILD_AT_X ) );
                hexSite.Y( pUnit->GetParam( BUILD_AT_Y ) );
                // crane's adjacent location, right now they are the same
                // but DT may later change to require the crane adjacent
                hexSite.X( pUnit->GetParam( CAI_DEST_X ) );
                hexSite.Y( pUnit->GetParam( CAI_DEST_Y ) );
            }

            pUnit->SetParamDW( CAI_ROUTE_X, 0 );
            pUnit->SetParamDW( CAI_ROUTE_Y, 0 );

            // the crane is blocked from entering build site
            if ( hexNext == hexSite )
            {
                // if it is a building, then it is probably an opfor
                // building and therefore the crane needs to be unassigned
                // and run away and its task unassigned too
                //
                // make/get an AI copy of the building data
                BOOL     bOpfor     = FALSE;
                BOOL     bBuilding  = FALSE;
                CAIUnit* paiBlocker = m_plUnits->GetOpForUnit( pMsg->m_dwID2 );
                if ( paiBlocker != NULL )
                {
                    if ( paiBlocker->GetOwner( ) != m_iPlayer )
                        bOpfor = TRUE;
                    if ( paiBlocker->GetType( ) == CUnit::building )
                        bBuilding = TRUE;
                }
                else                   // no such unit or NULL m_dwID2
                    bBuilding = TRUE;  // something is wrong with terrain

                // react based on states
                //
                // a building means we need to pick another site
                if ( bBuilding )
                    m_pTaskMgr->ConstructionError( pMsg );

                // any opfors are not for cranes
                if ( bOpfor && paiBlocker != NULL )
                    m_pTaskMgr->RunAway( paiBlocker, pUnit, pMsg );
                else  // may be our own vehicles so try to
                {     // get out of the way
                    // m_pMap->m_pMapUtil->FindStagingHex(
                    //	hexNext,2,2,pUnit->GetTypeUnit(), hexDest, FALSE );

                    // pUnit->SetDestination( hexDest );
                    pUnit->SetDestination( hexSite );

#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                               "CAIMgr::VehicleErrorResponse() respondinging to vehicle error " );
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "redirecting player %d unit %ld to %d,%d site \n",
                               m_iPlayer, pMsg->m_dwID, hexDest.X( ), hexDest.Y( ) );
#endif
                }

                // else it must be a vehicle blocking, and if it is an
                // opfor vehicle, then the crane should run away else
                // if not opfor (its ours) so stage away
                return;
            }
        }

        // default response
        //
        // consider that a deadly embrace may be occurring
        //     NOTE: this when vehicles getting stuck with each other (methinks)
        //
        if ( !IsEmbraced( pMsg ) )
            hexVeh = hexNext;

        m_pMap->m_pMapUtil->FindStagingHex( hexVeh, 2, 2, pUnit->GetTypeUnit( ), hexDest, FALSE );

        pUnit->SetDestination( hexDest );

        pUnit->SetParamDW( CAI_ROUTE_X, 0 );
        pUnit->SetParamDW( CAI_ROUTE_Y, 0 );

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::VehicleErrorResponse() respondinging to vehicle error " );
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "redirecting player %d unit %ld to %d,%d \n", m_iPlayer, pMsg->m_dwID,
                   hexDest.X( ), hexDest.Y( ) );
#endif
    }
}

//
// the vehicle in the pMsg->m_dwID has just reached
// the destination previous ordered, consider the
// current assigned task, and determine the next
// CNetOrder order that should be issued for it
//
// Also, this must consider that the vehicle may
// need to be supplied.  As of this writing, the
// rules for supply have not been determined.
//
// Also, this handles a truck transporting material
// that arrived
//
void CAIMgr::DestinationResponse( CAIMsg* pMsg )
{
    ASSERT_VALID( this );

    if ( pMsg->m_idata3 != m_iPlayer )
        return;

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::DestinationResponse() player %d respondinging to veh_dest ",
               m_iPlayer );
#endif

    // get the unit that just arrived
    CAIUnit* pUnit = m_plUnits->GetUnit( pMsg->m_dwID );
    if ( pUnit == NULL )
        return;

    // an HP unit not controlled
    if ( !pUnit->IsControl( ) )
        return;

        // consider it may be in combat
        // if( (pUnit->GetStatus() & CAI_IN_COMBAT) )
        //	return;

#if 1
    // consider it may be a truck transporting material
    if ( ( pUnit->GetStatus( ) & CAI_IN_USE ) && pGameData->IsTruck( pUnit->GetID( ) ) )
    {
        // get the unit of the hex which the truck
        // has just arrived in, and
        CAIHex aiHex( pMsg->m_iX, pMsg->m_iY );
        pGameData->GetCHexData( &aiHex );

        if ( aiHex.m_iUnit == CUnit::building )
        {
            // if its dwID matches
            // the pUnit->GetDataDW() value it has arrived
            // at the building needing the materials it is
            // currenting carrying, and
            if ( aiHex.m_dwUnitID == pUnit->GetDataDW( ) )
            {
                CAIUnit* pBldg = m_plUnits->GetUnit( pUnit->GetDataDW( ) );
                if ( pBldg == NULL )
                    return;

                // transfer the qty needed to the building
                // and clear the truck of assignment
                BOOL bNeedMore = m_pRouter->UnloadMaterials( pUnit, pBldg );

                // try the building we just unloaded at
                CHexCoord hexDest;
                hexDest.X( pMsg->m_iX );
                hexDest.Y( pMsg->m_iY );
                m_pMap->GetStagingHex( hexDest, 2, 2, pUnit->GetTypeUnit( ), hexDest, FALSE );

                // try rocket if we did not get one
                if ( hexDest.X( ) == pMsg->m_iX && hexDest.Y( ) == pMsg->m_iY )
                {
                    hexDest = m_pMap->m_pMapUtil->m_RocketHex;
                    m_pMap->GetStagingHex( hexDest, 2, 2, pUnit->GetTypeUnit( ), hexDest, FALSE );
                }
#if 0
				// now move the truck out of the building and into
				// an unoccupied location some distance away from it
				// or in the vicinity of the rocket
				CHexCoord hexDest( 0xFFFE, 0xFFFE );
				m_pMap->GetStagingHex( m_pMap->m_pMapUtil->m_RocketHex,
					2,2,pUnit->GetTypeUnit(), hexDest, FALSE );

				// nothing open near the rocket
				if( hexDest.X() == 0xFFFE )
				{
					hexDest.X(pMsg->m_iX);
					hexDest.Y(pMsg->m_iY);
					m_pMap->GetStagingHex( hexDest,2,2,
						pUnit->GetTypeUnit(), hexDest, FALSE );
					
					// try the rocket one more time
					if( hexDest.X() == pMsg->m_iX &&
						hexDest.Y() == pMsg->m_iY )
					{
						hexDest = m_pMap->m_pMapUtil->m_RocketHex;
						m_pMap->GetStagingHex( hexDest,2,2,
							pUnit->GetTypeUnit(), hexDest, FALSE );
					}
				}
#endif
                pUnit->SetDestination( hexDest );
                m_pMap->PlaceFakeVeh( hexDest, pUnit->GetTypeUnit( ) );

                // building still needs more materials, so fake an
                // out_mat building status message
                if ( bNeedMore )
                    FakeOutMatMsg( pBldg );

#ifdef _LOGOUT
                logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::DestinationResponse() player %d unload completed \n",
                           m_iPlayer );
#endif
                return;
            }
            else if ( aiHex.m_dwUnitID )  // non-zero means something here
            {
                // else consider all the non-zero DWORD params
                // and if it matches one of them, then the truck
                // has arrived at one of the sources for material
                // needed and it needs to transfer the quantity
                // (WORD params) of the material from the source
                // building to the truck (pUnit) and then determine
                // which of the remaining sources (DWORD params) is
                // closest and set the destination of the truck to
                // go to that source building.  if no more source
                // buildings, the set the destination to the building
                // needing the material (pUnit->GetDataDW())

                m_pRouter->LoadMaterials( pUnit, &aiHex );

#ifdef _LOGOUT
                logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::DestinationResponse() player %d load completed ",
                           m_iPlayer );
#endif
                return;
            }
        }

        // no building here so go to current destination
        CHexCoord hexDest( pUnit->GetParam( CAI_ROUTE_X ), pUnit->GetParam( CAI_ROUTE_Y ) );
        if ( hexDest.X( ) || hexDest.Y( ) )
        {
            pUnit->SetDestination( hexDest );
            return;
        }
    }
#endif

    // handle any left over messages
    if ( pMsg->m_iMsg == CNetCmd::veh_dest || pMsg->m_iMsg == CNetCmd::unit_repaired )
    {
        m_pTaskMgr->GenerateTaskOrder( pUnit );
    }

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::DestinationResponse() player %d completed \n", m_iPlayer );
#endif
}

void CAIMgr::OutLOSResponse( CAIMsg* pMsg )
{
    ASSERT_VALID( this );

    // get previously targeted unit
    CAIUnit* pTarget = m_plUnits->GetOpForUnit( pMsg->m_dwID2 );
    if ( pTarget == NULL )
        return;

    // clear any record of the attack
    pTarget->ClearParam( );

    // get the unit attacking
    CAIUnit* pAttacker = m_plUnits->GetUnit( pMsg->m_dwID );
    if ( pAttacker == NULL )
        return;

    // clear target for this attacker
    pAttacker->SetDataDW( 0 );
    pAttacker->SetParam( CAI_TARGETTYPE, 0xFFFE );
    // let task manager take over the pAttacker
    m_pTaskMgr->GenerateTaskOrder( pAttacker );
}

//
// BUGBUG much of what this does, is now handled
// by the game's auto fire capabilities.  There is
// an opfor update of attacks that occurs at the
// end that is already handled by the goal manager.
//
// However, this does cause an attacked unit to
// formally attack back, outside of the auto fire
// response provided by the game.
//
// the building/vehicle in the pMsg->m_dwID was
// just attacked, so determine the appropriate
// combat task and a response CNetOrder order
//
void CAIMgr::AttackResponse( CAIMsg* pMsg )
{
    ASSERT_VALID( this );

    if ( pMsg->m_iMsg != CNetCmd::see_unit && pMsg->m_iMsg != CNetCmd::unit_attacked &&
         pMsg->m_iMsg != CNetCmd::unit_damage )
        return;

    // is this reporting an attack on our units
    if ( pMsg->m_idata3 != m_iPlayer )
    {
        // no, its relative to another player's units
        return;
    }

    // no attacking our own units
    if ( pMsg->m_idata2 == pMsg->m_idata3 )
        return;


    // get the unit attacked
    CAIUnit* pTargeted = m_plUnits->GetUnit( pMsg->m_dwID );
    if ( pTargeted == NULL )
        return;

    // an HP unit not controlled
    if ( !pTargeted->IsControl( ) )
        return;

    // scenario units do not respond here
    if ( pTargeted->GetTask( ) == IDT_ATTACKUNIT )
        return;

    // make sure the attacked unit is flagged as in combat
    if ( !( pTargeted->GetStatus( ) & CAI_IN_COMBAT ) )
    {
        WORD wStatus = pTargeted->GetStatus( ) | CAI_IN_COMBAT;
        pTargeted->SetStatus( wStatus );
    }

    CAIUnit* pAttacker = m_plUnits->GetOpForUnit( pMsg->m_dwID2 );
    if ( pAttacker == NULL )
        return;
    if ( pAttacker->GetOwner( ) == m_iPlayer )
        return;

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::AttackResponse() executing for player %d ", m_iPlayer );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::unit=%d plyr=%d has attacked unit=%ld plyr=%d dmg=%d ",
               pMsg->m_dwID2, pMsg->m_idata2, pMsg->m_dwID, pMsg->m_idata3, pMsg->m_idata1 );
#endif

    // get the attacking type (hard,soft,naval) of the attacker
    int iAttackerType = 0xFFFE;
    if ( pAttacker->GetType( ) == CUnit::building )
    {
        CStructureData const* pBldgData = pGameData->GetStructureData( pAttacker->GetTypeUnit( ) );
        if ( pBldgData == NULL )
            return;
        iAttackerType = pBldgData->GetTargetType( );
    }
    else if ( pAttacker->GetType( ) == CUnit::vehicle )
    {
        CTransportData const* pVehData = pGameData->GetTransportData( pAttacker->GetTypeUnit( ) );
        if ( pVehData == NULL )
            return;
        iAttackerType = pVehData->GetTargetType( );
    }
    else
        return;

    // if the targeted unit can attack the attacking type
    // of the attacker, then the target fires back
    if ( pTargeted->GetType( ) == CUnit::building )
    {
        EnterCriticalSection( &cs );
        CBuilding* pBldg = theBuildingMap.GetBldg( pTargeted->GetID( ) );
        if ( pBldg != NULL )
        {
            if ( !pBldg->GetAttack( iAttackerType ) )
            {
                LeaveCriticalSection( &cs );
                // buildings can't retreat
                return;
            }
        }
        LeaveCriticalSection( &cs );
        if ( pBldg == NULL )
            return;
    }
    else if ( pTargeted->GetType( ) == CUnit::vehicle )
    {
        EnterCriticalSection( &cs );
        CVehicle* pVeh = theVehicleMap.GetVehicle( pTargeted->GetID( ) );
        if ( pVeh != NULL )
        {
            // target unit can't attack the attacking unit
            if ( !pVeh->GetAttack( iAttackerType ) )
            {
                LeaveCriticalSection( &cs );
                // vehicle can retreat, so how to retreat?
                m_pTaskMgr->RunAway( pTargeted, pAttacker, pMsg );
                return;
            }
            else  // target can attack back
            {
                int       iRange = pVeh->GetRange( );
                CHexCoord hexTarget( pMsg->m_iX, pMsg->m_iY );
                CHexCoord hexAttacker( pMsg->m_ieX, pMsg->m_ieX );
                // but is it in range to do so?
                if ( pGameData->GetRangeDistance( hexTarget, hexAttacker ) > iRange )
                {
                    LeaveCriticalSection( &cs );
                    // vehicle needs to move into range to attack
                    m_pTaskMgr->MoveToRange( pTargeted, hexAttacker );
                    // and go ahead and send attack message
                    pTargeted->AttackUnit( pAttacker->GetID( ) );
                    pAttacker->AttackedBy( pTargeted->GetID( ) );
                    return;
                }
            }
        }
        LeaveCriticalSection( &cs );
        if ( pVeh == NULL )
            return;
    }
    else
        return;

    // right now, skip range check until DT responds
    pTargeted->AttackUnit( pAttacker->GetID( ) );
    pAttacker->AttackedBy( pTargeted->GetID( ) );
}

//
// this is a special scenario attack message that contains
// the unit to attack and the unit to attack with
//
void CAIMgr::AttackUnit( CAIMsg* pMsg )
{
    ASSERT_VALID( this );

    // this is a kludge because the game uses the same
    // message to indicate a need to attack both a unit
    // or an entire player, and ZERO target and attacking
    // unit means to attack the player
    if ( !pMsg->m_dwID && !pMsg->m_dwID2 )
    {
        m_bAttackPlayer = TRUE;
        return;
    }

    CAIUnit* pTargeted = m_plUnits->GetOpForUnit( pMsg->m_dwID );
    if ( pTargeted == NULL )
        return;

    CAIUnit* pAttacker = m_plUnits->GetUnit( pMsg->m_dwID2 );
    if ( pAttacker == NULL )
        return;

    // a little protection from the message
    if ( pAttacker->GetOwner( ) != m_iPlayer )
        return;

    // protection for unit being gone
    if ( pAttacker->GetType( ) == CUnit::building )
    {
        EnterCriticalSection( &cs );
        CBuilding* pBldg = theBuildingMap.GetBldg( pAttacker->GetID( ) );
        if ( pBldg == NULL )
        {
            LeaveCriticalSection( &cs );
            return;
        }
        LeaveCriticalSection( &cs );
    }
    else if ( pAttacker->GetType( ) == CUnit::vehicle )
    {
        EnterCriticalSection( &cs );
        CVehicle* pVeh = theVehicleMap.GetVehicle( pAttacker->GetID( ) );
        if ( pVeh == NULL )
        {
            LeaveCriticalSection( &cs );
            return;
        }
        LeaveCriticalSection( &cs );
    }
    else
        return;

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::AttackUnit() executing for player %d ", m_iPlayer );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "unit=%d plyr=%d is to attack unit=%ld plyr=%d in scenario ", pMsg->m_dwID2,
               pMsg->m_idata3, pMsg->m_dwID, pMsg->m_idata1 );
#endif

    // assign unit to this scenario
    pAttacker->ClearParam( );
    pAttacker->SetGoal( IDG_CONQUER );
    pAttacker->SetTask( IDT_ATTACKUNIT );
    // assign target
    pAttacker->SetDataDW( pTargeted->GetID( ) );
    // let task manager take over the pAttacker
    m_pTaskMgr->GenerateTaskOrder( pAttacker );
}

//
// respond to the unit_loaded message arriving
//
void CAIMgr::LoadedResponse( CAIMsg* pMsg )
{
    CAIUnit* pCarrier = m_plUnits->GetUnit( pMsg->m_dwID );
    if ( pCarrier == NULL )
        return;

    CAIUnit* pCargo = m_plUnits->GetUnit( pMsg->m_dwID2 );
    if ( pCargo == NULL )
        return;

    WORD wStatus = pCargo->GetStatus( );
    if ( wStatus & CAI_IN_USE )
    {
        wStatus ^= CAI_IN_USE;
        pCargo->SetStatus( wStatus );
    }
    wStatus = pCarrier->GetStatus( );
    if ( wStatus & CAI_IN_USE )
    {
        wStatus ^= CAI_IN_USE;
        pCarrier->SetStatus( wStatus );
    }
    m_pTaskMgr->GenerateTaskOrder( pCarrier );
}
//
// process the last received message and set CAIMgr states
// based on the type and purpose of the message
//
void CAIMgr::ProcessMessage( CAIMsg* pMsg )
{
    if ( pMsg == NULL )
        return;


#ifdef _DEBUG

    // if start dropping them that makes everything run nice and smooth,
    // and the AI still functions adequately, however this is going to probably 
    // break a lot of unknown things, so I'm only using it for testing
    static DWORD lastAttackFloodCheck = 0;
    static int   recentAttackCount    = 0;

    DWORD now = timeGetTime( );
    if ( now - lastAttackFloodCheck > 1000 )
    {  // Check every second
        if ( recentAttackCount > 20 ) // More than X attack messages per second
        {  
#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                       "Attack message flood detected: %d messages/sec - activating circuit breaker",
                       recentAttackCount );
#endif
            // Skip processing attack-related messages for a few seconds
            if ( pMsg->m_iMsg == CNetCmd::unit_attacked || pMsg->m_iMsg == CNetCmd::unit_damage ||
                 pMsg->m_iMsg == CNetCmd::see_unit )
            {
                // return;  // Drop the message to make things run fast, but who knows what that will break
            }
        }
        lastAttackFloodCheck = now;
        recentAttackCount    = 0;
    }

    if ( pMsg->m_iMsg == CNetCmd::unit_attacked || pMsg->m_iMsg == CNetCmd::unit_damage ||
         pMsg->m_iMsg == CNetCmd::see_unit )
    {
        recentAttackCount++;
    } 
#endif

    /*
                        // sent from server
    21	bldg_new,		// a new building is created
    22	bldg_stat,		// built/damage status of a bldg
    23	veh_new,		// a new vehicle is created
    24	veh_stat,		// damage status of a vehicle
    25	veh_loc,		// vehicle going to the next hex
    26	veh_dest,		// vehicle has arrived at its destination
    27	road_new,		// a new road is started
    28	road_done,		// a new road hex is completed
    29	unit_set_damage,// received damage level
    30	veh_set_dest,	// AI message to set dest

    */
    // let the games begin
    if ( pMsg->m_iMsg == CNetCmd::cmd_play )
    {
        m_bGoalAssess = TRUE;
    }

    // if( pMsg->m_iMsg == CNetCmd::cmd_pause )
    //	thePathMgr.ReportCounts();

    // vehicle cannot reach its next location
    if ( pMsg->m_iMsg == CNetCmd::err_veh_goto || pMsg->m_iMsg == CNetCmd::err_veh_traffic )
    {
        m_bVehicleError = TRUE;
    }

    if ( pMsg->m_iMsg == CNetCmd::unit_loaded )
        m_bUnitLoaded = TRUE;

    // if it was a construction/production completion
    if ( pMsg->m_iMsg == CNetCmd::bldg_new || pMsg->m_iMsg == CNetCmd::veh_new )
    //&& pMsg->m_idata3 == m_iPlayer )
    {
#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nplayer %d unit id: %ld  type: %d  at %d,%d ", pMsg->m_idata3,
                   pMsg->m_dwID, pMsg->m_idata1, pMsg->m_iX, pMsg->m_iY );
#endif
        m_bGoalAssess       = TRUE;
        m_bProductionChange = TRUE;
    }

    if ( pMsg->m_iMsg == CNetCmd::bldg_new || pMsg->m_iMsg == CNetCmd::veh_new )
        m_bCheckResearch = TRUE;

    // a building under construction has been completed
    // so it may need to be controled
    if ( pMsg->m_iMsg == CNetCmd::bldg_stat && pMsg->m_uFlags == CMsgBldgStat::built && pMsg->m_idata2 == 100 )
    {
        m_bProductionChange = TRUE;
    }

    // a building is out of material
    if ( pMsg->m_iMsg == CNetCmd::bldg_stat && pMsg->m_uFlags == CMsgBldgStat::out_mat )
    {
        m_bProductionChange = TRUE;
    }

    // if building or vehicle was destroyed
    if ( pMsg->m_iMsg == CNetCmd::delete_unit )
    {
        m_bGoalAssess    = TRUE;
        m_bMapChanged    = TRUE;
        m_bUnitDestroyed = TRUE;
    }

    if ( pMsg->m_iMsg == CNetCmd::give_unit )
        m_bGiveUnit = TRUE;

    // we're killing a player
    if ( pMsg->m_iMsg == CNetCmd::plyr_dying )
        m_bPlayerDying = TRUE;

    if ( pMsg->m_iMsg == CNetCmd::unit_repaired )
        m_bVehicleArrived = TRUE;

    if ( pMsg->m_iMsg == CNetCmd::scenario )  //&&
        // pMsg->m_idata3 == m_iPlayer )
        m_bScenario = TRUE;

    if ( pMsg->m_iMsg == CNetCmd::build_civ && pMsg->m_idata3 == m_iPlayer )
        m_bPlaceBldg = TRUE;

    if ( pMsg->m_iMsg == CNetCmd::scenario_atk && pMsg->m_idata3 == m_iPlayer )
        m_bAttackUnit = TRUE;

    if ( pMsg->m_iMsg == CNetCmd::out_of_LOS && pMsg->m_idata3 == m_iPlayer )
        m_bOutLOS = TRUE;

    // a unit was damaged
    if ( pMsg->m_iMsg == CNetCmd::unit_damage && pMsg->m_idata3 == m_iPlayer )
        m_bAttackOccurred = TRUE;

// BUGBUG this is for testing only
#if 1
    // count shooters
    if ( pMsg->m_iMsg == CNetCmd::unit_damage )
    {
        // count HP if he shot at us
        if ( pMsg->m_idata2 == 1 && pMsg->m_idata3 == m_iPlayer )
            m_iHPShooter++;
        // count AI if we shot at HP
        else if ( pMsg->m_idata2 == m_iPlayer && pMsg->m_idata3 == 1 )
            m_iAIShooter++;
    }
#endif

    if ( pMsg->m_iMsg == CNetCmd::see_unit )
        m_bAttackOccurred = TRUE;

    if ( pMsg->m_iMsg == CNetCmd::unit_attacked )
    {
        m_bAttackOccurred = TRUE;
        m_bGoalAssess     = TRUE;
    }


    // if it was a vehicle arriving at destination
    if ( pMsg->m_iMsg == CNetCmd::veh_dest )
    {

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nplayer %d unit id: %ld arrived at %d,%d ", pMsg->m_idata3,
                   pMsg->m_dwID, pMsg->m_iX, pMsg->m_iY );
#endif
        if ( pMsg->m_idata3 == m_iPlayer )
            m_bVehicleArrived = TRUE;
        m_bMapChanged = TRUE;
    }

    // a road hex was completed
    if ( pMsg->m_iMsg == CNetCmd::road_done || pMsg->m_iMsg == CNetCmd::road_new ||
         pMsg->m_iMsg == CNetCmd::err_build_road )
    {
#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nplayer %d unit id: %ld road at %d,%d mode=%d msgID=%d",
                   pMsg->m_idata3, pMsg->m_dwID, pMsg->m_iX, pMsg->m_iY, pMsg->m_idata2, pMsg->m_iMsg );
#endif
        if ( pMsg->m_idata3 == m_iPlayer )
            m_bMapChanged = TRUE;
    }

    if ( pMsg->m_iMsg == CNetCmd::err_build_bldg && pMsg->m_idata3 == m_iPlayer )
        m_bMapChanged = TRUE;

    if ( pMsg->m_iMsg == CNetCmd::err_place_bldg && pMsg->m_idata3 == m_iPlayer &&
         pMsg->m_idata1 == CStructureData::rocket )
        m_bPlaceRocket = TRUE;


    // if it was a vehicle entering a hex
    if ( pMsg->m_iMsg == CNetCmd::veh_loc && pMsg->m_idata3 == m_iPlayer )
    {
        m_bLocChanged = TRUE;
        return;  // skip debug output
    }

#ifdef _LOGOUT
    CString sMsg = pGameData->GetMsgString( pMsg->m_iMsg );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nProcessMessage(): %s msg=%d  player %d ", (const char*)sMsg,
               pMsg->m_iMsg, m_iPlayer );

    if ( pMsg->m_iMsg == CNetCmd::bldg_stat )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "id=%ld  flags=%d  built=%d  type=%d  player=%d ", pMsg->m_dwID,
                   pMsg->m_uFlags, pMsg->m_idata2, pMsg->m_idata1, pMsg->m_idata3 );
    }
    else if ( pMsg->m_iMsg == CNetCmd::err_veh_goto || pMsg->m_iMsg == CNetCmd::err_veh_traffic )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "id=%ld  hex=%d,%d  next=%d,%d  dest=%d,%d  player=%d ", pMsg->m_dwID,
                   pMsg->m_iX, pMsg->m_iY, pMsg->m_idata1, pMsg->m_idata2, pMsg->m_ieX, pMsg->m_ieY, pMsg->m_idata3 );
    }
    else if ( pMsg->m_iMsg == CNetCmd::unit_loaded )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "carrier=%ld  cargo=%d", pMsg->m_dwID, pMsg->m_dwID2 );
    }
    else if ( pMsg->m_iMsg == CNetCmd::unit_set_repair )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "player %d unit %ld  repaired=%d", pMsg->m_idata3, pMsg->m_dwID,
                   pMsg->m_idata1 );
    }
    else if ( pMsg->m_iMsg == CNetCmd::err_build_bldg )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "id=%ld  type=%d   hex=%d,%d  why=%d  crane=%ld", pMsg->m_dwID,
                   pMsg->m_idata1, pMsg->m_iX, pMsg->m_iY, pMsg->m_idata2, pMsg->m_dwID2 );
    }
    else if ( pMsg->m_iMsg == CNetCmd::unit_damage )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "player=%d  unit=%ld  dmg=%d   shooter player=%d  unit=%ld ",
                   pMsg->m_idata3, pMsg->m_dwID, pMsg->m_idata1, pMsg->m_idata2, pMsg->m_dwID2 );
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "AI Shots Fired %d   HP Shots Fired %d", m_iAIShooter, m_iHPShooter );
    }
    else if ( pMsg->m_iMsg == CNetCmd::delete_unit )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "player=%d  id=%ld ", pMsg->m_idata3, pMsg->m_dwID );
    }
    else if ( pMsg->m_iMsg == CNetCmd::build_civ )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "player=%d  type=%ld ", pMsg->m_idata3, pMsg->m_idata1 );
    }
    else if ( pMsg->m_iMsg == CNetCmd::scenario )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "m_iPlyrAi=%d  m_iScenario=%ld ", pMsg->m_idata3, pMsg->m_idata1 );
    }
    else if ( pMsg->m_iMsg == CNetCmd::out_of_LOS )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "player=%d  attacker=%ld   target=%d/%ld", pMsg->m_idata3, pMsg->m_dwID,
                   pMsg->m_idata2, pMsg->m_dwID2 );
    }
    else if ( pMsg->m_iMsg == CNetCmd::attack )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "attacker=%ld   target=%ld", pMsg->m_dwID, pMsg->m_dwID2 );
    }
    else if ( pMsg->m_iMsg == CNetCmd::bridge_new || pMsg->m_iMsg == CNetCmd::bridge_done )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "player=%d crane=%ld bridge=%ld mode=%d from %d,%d to %d,%d ",
                   pMsg->m_idata3, pMsg->m_dwID2, pMsg->m_dwID, pMsg->m_idata1, pMsg->m_iX, pMsg->m_iY, pMsg->m_ieX,
                   pMsg->m_ieY );
    }

    sMsg.Empty( );
#endif
}

/*
// the building status (% built, % damaged, paused, resumed, out of materials)
class CMsgBldgStat : public CNetCmd
{
public:
        CMsgBldgStat (CBuilding const *pBldg);
        enum { built = 0x01,
                     damaged = 0x02,
                     paused = 0x04,
                     resumed = 0x08,
                     out_mat = 0x10 };

        DWORD				m_dwID;				// ID of building
        int					m_iBuilt;			// % built
        int					m_iType;	// CDataStructure
        int					m_iFlags;
        int					m_iDamage;		// % damaged

#ifdef _DEBUG
public:
    void AssertValid() const;
#endif
};
*/

//
// this creates a fake CMsgBldgStat message for the passed building
// with its flag set to out of materials, for the sole purpose of
// getting the router to check the building for needs on the next
// message processing cycle
//
void CAIMgr::FakeOutMatMsg( CAIUnit* paiBldg )
{
    // BUGBUG unable to use critical sections because they caused a hang
    // EnterCriticalSection(&cs);
    CBuilding* pBldg = theBuildingMap.GetBldg( paiBldg->GetID( ) );
    if ( pBldg != NULL )
    {
        CMsgBldgStat msg( pBldg );
        // LeaveCriticalSection(&m_cs);

        msg.m_iFlags = CMsgBldgStat::out_mat;
        // the building is built
        msg.m_iBuilt = 100;

        // make like it came from the game
        MessageArrived( &msg );
        return;
    }
    // LeaveCriticalSection(&m_cs);
}

//
// have this AI player's goal manager handle opportunity fire
// when this is called directly from the game
//
BOOL CAIMgr::AutoFire( CUnit* pFiring, CUnit const* pTarget )
{
    // get target unit data for decision-making
    int   iPlayer    = pTarget->GetOwner( )->GetPlyrNum( );
    DWORD dwID       = pTarget->GetID( );
    int   utUnitType = (int)pTarget->GetUnitType( );

    // get debuging data for firing unit
    DWORD dwIDFiring    = pFiring->GetID( );
    int   utFiringType  = (int)pFiring->GetUnitType( );
    int   iPlayerFiring = pFiring->GetOwner( )->GetPlyrNum( );
    int   iTypeTarget, iTypeFiring;

    if ( utFiringType == CUnit::vehicle )
    {
        CVehicle* pVeh = (CVehicle*)pFiring;
        iTypeFiring    = pVeh->GetData( )->GetType( );

        // if carrying units unload them!
        if ( iPlayerFiring == m_iPlayer && pVeh->GetCargoCount( ) )
        {
            CAIUnit* paiUnit = m_plUnits->GetUnitNY( dwIDFiring );
            if ( paiUnit != NULL )
                paiUnit->UnloadCargo( );
        }
    }
    else if ( utFiringType == CUnit::building )
    {
        CBuilding* pBldg = (CBuilding*)pFiring;
        iTypeFiring      = pBldg->GetData( )->GetType( );
    }

    // enum {			// run time type identification - use GetUnitType ()
    //	building,
    //	vehicle,
    //	bridge };
    //
    //  there are some buildings that we NEVER want to fire on
    //  as they are offices and apartments, which is a waste
    if ( utUnitType == CUnit::building )
    {
        CBuilding* pBldg = (CBuilding*)pTarget;
        iTypeTarget      = pBldg->GetData( )->GetType( );
        if ( iTypeTarget != CStructureData::rocket && iTypeTarget < CStructureData::barracks_2 )
        {

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                       "\nCAIMgr::AutoFire() on player=%d  unit %ld  building type %d  RETURNED 0", iPlayer, dwID,
                       iTypeTarget );
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::AutoFire() firing player=%d  unit %ld  type %d/%d \n",
                       iPlayerFiring, dwIDFiring, utFiringType, iTypeFiring );
#endif
            return ( FALSE );
        }
    }
    else if ( utUnitType == CUnit::vehicle )
    {
        CVehicle* pVeh = (CVehicle*)pTarget;
        iTypeTarget    = pVeh->GetData( )->GetType( );
    }


#ifdef _LOGOUT
    BOOL bFire = m_pGoalMgr->AutoFire( iPlayer );

    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::AutoFire() on player=%d  unit %ld  type %d/%d  RETURNED %d",
               iPlayer, dwID, (int)utUnitType, iTypeTarget, (int)bFire );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::AutoFire() firing player=%d  unit %ld  type %d/%d \n",
               iPlayerFiring, dwIDFiring, utFiringType, iTypeFiring );

    return ( bFire );
#else
    return ( m_pGoalMgr->AutoFire( iPlayer ) );
#endif
}

//
// this function is called by the exported CAIMsg()
// when a message arrives for the player of this CAIMgr
//
void CAIMgr::MessageArrived( CNetCmd const* pNewMsg )
{
    ASSERT_VALID( this );

    CAIMsg* pMsg = NULL;
    if ( m_plTmpQueue != NULL )
    {
        try
        {
            // create an AI version of message
            pMsg = new CAIMsg( pNewMsg );
        }
        catch ( CException* e )
        {
            if ( pMsg != NULL )
                delete pMsg;

            throw( ERR_CAI_BAD_NEW );
        }
        EnterCriticalSection( &m_cs );
        m_plTmpQueue->AddTail( (CObject*)pMsg );
        LeaveCriticalSection( &m_cs );
    }
}

//
// this is a stub version, because it does no priortization
//
void CAIMgr::PrioritizeMessage( void )
{
    EnterCriticalSection( &m_cs );

    // the FIFO queue
    ASSERT_VALID( m_plTmpQueue );

    // the priortized message queue
    ASSERT_VALID( m_plMsgQueue );

    if ( m_plTmpQueue->IsEmpty( ) )
    {
        LeaveCriticalSection( &m_cs );
        return;
    }

    CAIMsg* pNewMsg = (CAIMsg*)m_plTmpQueue->RemoveHead( );
    // add message to end of prioritized queue
    if ( pNewMsg != NULL )
        m_plMsgQueue->AddTail( (CObject*)pNewMsg );

    LeaveCriticalSection( &m_cs );
}
#if 0
//
// insert this message into the queue based on the
// priority of the message, as determined by the type
// of message this is, higher is more priority
//
void CAIMgr::PrioritizeMessage(void) //( CAIMsg *pNewMsg )
{
	// messages that have come in, but not priortized
	// BUGBUG the ASSERT at line 399 of list_o.cpp fires
	// occasionally on this list pointer, and does
	// so with an invalid count of the items in the list.

	EnterCriticalSection(&m_cs);
	ASSERT_VALID( m_plTmpQueue );
	

	// the priortized message queue	
	
	ASSERT_VALID( m_plMsgQueue );
	
	
	// there are no un-priortized messages
	
	if( m_plTmpQueue->IsEmpty() )
	{
		LeaveCriticalSection(&m_cs);
		return;
	}

	// get 1st message from the FIFO queue
	//POSITION posQ = m_plTmpQueue->GetHeadPosition();
	CAIMsg *pNewMsg = (CAIMsg *)m_plTmpQueue->RemoveHead();
	while( pNewMsg != NULL )
	{
		ASSERT_VALID( pNewMsg );

		POSITION pos2 = NULL;
    	POSITION pos = m_plMsgQueue->GetHeadPosition();

		// no messages in the priority queue, so add and continue
		if( pos == NULL )
		{
			// add message to end of prioritized queue
			m_plMsgQueue->AddTail( (CObject *)pNewMsg );

			// now consider if more incoming messages exist
			pNewMsg = NULL;
			if( !m_plTmpQueue->IsEmpty() )
				pNewMsg = (CAIMsg *)m_plTmpQueue->RemoveHead();
			continue;
		}

    	while( pos != NULL )
    	{
        	CAIMsg *pMsg = (CAIMsg *)m_plMsgQueue->GetNext( pos );
        	if( pMsg != NULL )
        	{
          		ASSERT_VALID( pMsg );

				// continue as long as priority is higher or equal
				if( pMsg->m_iPriority >= pNewMsg->m_iPriority )
				{
					// there are no more messages in the queue
					if( pos == NULL )
					{
						// so the new message goes on the end
						m_plMsgQueue->AddTail( (CObject *)pNewMsg );
					}
					// save position value of prior message
					pos2 = pos;
					continue;
				}

				// message just checked is lower priority so
				// the value of pos2 is its pos value
				m_plMsgQueue->InsertBefore( pos2, (CObject *)pNewMsg );
				break;
        	}
			else
				m_plMsgQueue->AddTail( (CObject *)pNewMsg );
    	}

		// get next message from the FIFO queue
		if( m_plTmpQueue->IsEmpty() )
			break;

		pNewMsg = (CAIMsg *)m_plTmpQueue->RemoveHead();
		if( pNewMsg == NULL )
			break;

		ASSERT_VALID( pNewMsg );
	}
	LeaveCriticalSection(&m_cs);
}
#endif

//
// ask goal manger for the next research topic to study
//
int CAIMgr::NextTopic( CPlayer* pPlayer )
{
    if ( m_pGoalMgr != NULL )
    {
        m_bCheckResearch = TRUE;
        return ( m_pGoalMgr->NextResearchTopic( pPlayer ) );
    }
    return ( 0 );
}
//
// BUGBUG this function does not need to yield
//
// BUGBUG the initial position offsets are going to change!
//
// this function will review the CPlayer::m_InitAttrib[NUM_RACE_CHAR
// to NUM_RACE_CHUNKS-1] which are representing the player's initial
// position and supplies, and apply the CAIData::m_iSmart value to
// make adjustments
//
void CAIMgr::AdjustAttribs( CPlayer* pPlayer )
{
    ASSERT_VALID( this );
    ASSERT_VALID( pPlayer );


    // pick a race
    const int iStrong[]  = { 3, 6, 9 };
    const int iAvg[]     = { 0, 2, 7, 8, 11 };
    const int iWeak[]    = { 1, 4, 5, 10 };
    const int NUM_STRONG = sizeof( iStrong ) / sizeof( int );
    const int NUM_AVG    = sizeof( iAvg ) / sizeof( int );
    const int NUM_WEAK   = sizeof( iWeak ) / sizeof( int );

    int iRace;
    switch ( pGameData->m_iSmart )
    {
    case 0:  // easy
        if ( MyRand( ) & 0x0100 )
            iRace = iWeak[RandNum( NUM_WEAK - 1 )];
        else if ( MyRand( ) & 0x0100 )
            iRace = iAvg[RandNum( NUM_AVG - 1 )];
        else
            iRace = RandNum( ptheRaces->GetNumRaces( ) - 1 );
        break;

    case 1:  // moderate
        if ( MyRand( ) & 0x0100 )
            iRace = iAvg[RandNum( NUM_AVG - 1 )];
        else if ( MyRand( ) & 0x0100 )
            iRace = iWeak[RandNum( NUM_WEAK - 1 )];
        else
            iRace = RandNum( ptheRaces->GetNumRaces( ) - 1 );
        break;

    case 2:  // difficult
        if ( MyRand( ) & 0x0100 )
            iRace = iAvg[RandNum( NUM_AVG - 1 )];
        else if ( MyRand( ) & 0x0100 )
            iRace = iStrong[RandNum( NUM_STRONG - 1 )];
        else
            iRace = RandNum( ptheRaces->GetNumRaces( ) - 1 );
        break;

    case 3:  // impossible
        if ( MyRand( ) & 0x0100 )
            iRace = iStrong[RandNum( NUM_STRONG - 1 )];
        else if ( MyRand( ) & 0x0100 )
            iRace = iAvg[RandNum( NUM_AVG - 1 )];
        else
            iRace = RandNum( ptheRaces->GetNumRaces( ) - 1 );
        break;

    default:
        TRAP( );
        iRace = RandNum( ptheRaces->GetNumRaces( ) - 1 );
        break;
    }

    pPlayer->m_InitData.Set( &( ptheRaces[iRace] ), pGameData->m_iStartPos );
    pPlayer->SetName( ptheRaces[iRace].GetLine( ) );

    // AI hates water, never select an island
    pPlayer->m_InitData.SetSupplies( CRaceDef::island, 0 );

    // let difficulty determine ocean for game
    int iVal = pGameData->GetRandom( pGameData->m_iSmart );  // 1 is beach 0 is not
    if ( iVal )
        iVal = 0;  // more difficulty increases chance of no ocean
    else
        iVal = 1;
    // AI will never place anything on a small world if there is an ocean
    if ( pGameData->m_iHexPerBlk <= 32 )
        iVal = 0;

    pPlayer->m_InitData.SetSupplies( CRaceDef::ocean, iVal );

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::AdjustAttribs(): player %d ocean=%d \n", m_iPlayer, iVal );
#endif

    // increase starting resources
    for ( int iInd = 0; iInd <= CRaceDef::infantry; iInd++ )
    {
        int iNum = pPlayer->m_InitData.GetSupplies( iInd );
        if ( ( iNum == 0 ) && ( pGameData->m_iSmart > 1 ) )
        {
            if ( ( pGameData->m_iSmart > 2 ) || ( ( iInd != CRaceDef::med_scout ) && ( iInd != CRaceDef::med_tank ) &&
                                                  ( iInd != CRaceDef::light_art ) ) )
                iNum = RandNum( pGameData->m_iSmart - 1 );
        }
        else
            iNum += ( iNum * pGameData->m_iSmart ) / 2;
        pPlayer->m_InitData.SetSupplies( iInd, iNum );
    }

    // increase racial attributes
    const float fAdd[] = { -0.2, 0.0, 0.25, 0.5 };
    float       fExtra = fAdd[pGameData->m_iSmart];
    for ( int iInd = 0; iInd <= CRaceDef::num_race; iInd++ )
    {
        float fNum = pPlayer->m_InitData.GetRace( iInd );
        switch ( iInd )
        {
        case CRaceDef::build_bldgs:
        case CRaceDef::manf_materials:
        case CRaceDef::manf_vehicles:
        case CRaceDef::mine_prod:
        case CRaceDef::farm_prod:
            fNum += fNum * fExtra * 2.0;
            break;

        case CRaceDef::research:
        case CRaceDef::pop_grow:
        case CRaceDef::attack:
        case CRaceDef::defense:
            fNum += fNum * fExtra;
            break;

        case CRaceDef::pop_die:
        case CRaceDef::pop_eat:
            fNum -= fNum * fExtra / 2.0;
            break;
        }
        pPlayer->m_InitData.SetRace( iInd, fNum );
    }

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::AdjustAttribs(): player %d ", m_iPlayer );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "0  - build_bldgs" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "1  - manf_materials" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "2  - manf_vehicles" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "3  - mine_prod" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "4  - farm_prod" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "5  - research" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "6  - pop_grow" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "7  - pop_die" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "8  - pop_eat" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "9  - attack" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "10 - defense \n" );
#endif

#ifdef _LOGOUT
    for ( int j = 0; j < CRaceDef::num_race; j++ )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "attribute %d  new=%f", j, pPlayer->m_InitData.GetRace( j ) );
    }
#endif

#ifdef BUGBUG
    /* NEW WAY

    class CRaceDef

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
    // pick a race
    int iRace = pGameData->GetRandom( ptheRaces->GetNumRaces( ) - 1 );

    pPlayer->m_InitData.Set( &( ptheRaces[iRace] ), pGameData->m_iStartPos );
    pPlayer->SetName( ptheRaces[iRace].GetLine( ) );

    // BUGBUG additional adjustment to the standardized
    // difficulty setting based data should occur here

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::AdjustAttribs(): player %d ", m_iPlayer );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "0  - build_bldgs" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "1  - manf_materials" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "2  - manf_vehicles" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "3  - mine_prod" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "4  - farm_prod" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "5  - research" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "6  - pop_grow" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "7  - pop_die" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "8  - pop_eat" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "9  - attack" );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "10 - defense \n" );
#endif

    float fVal, fOldVal;

    // get initial supplies for this race
    int iaSupplies[CRaceDef::num_supplies];
    for ( int j = 0; j < CRaceDef::num_supplies; j++ ) iaSupplies[j] = pPlayer->m_InitData.GetSupplies( j );


    // RANDOM adjustment to characteristics based on difficulty
    for ( j = 0; j < CRaceDef::num_race; j++ )
    {
        // start with standard value
        fOldVal = pPlayer->m_InitData.GetRace( j );
        fVal    = fOldVal;

        // decision to make adjustment BASED off difficulty
        if ( pGameData->GetRandom( pGameData->m_iSmart ) )
        {
            // sub from eating and dying rates and add all others
            if ( j != CRaceDef::pop_die && j != CRaceDef::pop_eat )
                fVal += (float)( .1 * pGameData->m_iSmart );
            else
                fVal -= (float)( .1 * pGameData->m_iSmart );
        }
        pPlayer->m_InitData.SetRace( j, fVal );

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "attribute %d  old=%f  new=%f", j, fOldVal, fVal );
#endif
    }

    /*
        enum SUPPLIES {
                    food,
                    lumber,
                    steel,
                    copper,
                    gas,
                    coal,
                    iron,
                    oil,
                    people,

                    crane,
                    heavy_truck,
                    light_scout,
                    med_scout,
                    infantry_carrier,
                    light_tank,
                    med_tank,
                    light_art,
                    infantry,

                    island,
                    ocean,
                    num_supplies };

    */
    int iVal = 2000;
    if ( iaSupplies[CRaceDef::food] )
        iVal = iaSupplies[CRaceDef::food];
    // iVal += (1000 * pGameData->m_iSmart);
    pPlayer->m_InitData.SetSupplies( CRaceDef::food, iVal );

    iVal = 2000;
    if ( iaSupplies[CRaceDef::lumber] )
        iVal = iaSupplies[CRaceDef::lumber];
    // iVal += (1000 * pGameData->m_iSmart);
    // if( iVal < 2000 )
    //	iVal = 2000;
    pPlayer->m_InitData.SetSupplies( CRaceDef::lumber, iVal );

    iVal = 5000;
    if ( iaSupplies[CRaceDef::steel] )
        iVal = iaSupplies[CRaceDef::steel];
    // iVal += (1000 * pGameData->m_iSmart);
    // if( iVal < 1000 )
    //	iVal = 1000;
    pPlayer->m_InitData.SetSupplies( CRaceDef::steel, iVal );

    iVal = 0;
    if ( iaSupplies[CRaceDef::copper] )
    {
        iVal = iaSupplies[CRaceDef::copper];
        iVal += ( 10 * pGameData->m_iSmart );
    }
    pPlayer->m_InitData.SetSupplies( CRaceDef::copper, iVal );

    iVal = 2000;
    if ( iaSupplies[CRaceDef::gas] )
        iVal = iaSupplies[CRaceDef::gas];
    // iVal += (100 * pGameData->m_iSmart);
    pPlayer->m_InitData.SetSupplies( CRaceDef::gas, iVal );

    iVal = 2000;
    if ( iaSupplies[CRaceDef::coal] )
        iVal = iaSupplies[CRaceDef::coal];
    // iVal += (150 * pGameData->m_iSmart);
    pPlayer->m_InitData.SetSupplies( CRaceDef::coal, iVal );

    iVal = 2000;
    if ( iaSupplies[CRaceDef::iron] )
        iVal = iaSupplies[CRaceDef::iron];
    // iVal += (100 * pGameData->m_iSmart);
    pPlayer->m_InitData.SetSupplies( CRaceDef::iron, iVal );

    iVal = 2000;
    if ( iaSupplies[CRaceDef::oil] )
        iVal = iaSupplies[CRaceDef::oil];
    // iVal += (100 * pGameData->m_iSmart);
    pPlayer->m_InitData.SetSupplies( CRaceDef::oil, iVal );

    iVal = 100;
    if ( iaSupplies[CRaceDef::people] )
        iVal = iaSupplies[CRaceDef::people];
    // if( pGameData->GetRandom(pGameData->m_iSmart) )
    //	iVal += (10 * pGameData->m_iSmart);
    pPlayer->m_InitData.SetSupplies( CRaceDef::people, iVal );

    // bonus supplies for difficult and impossible
    if ( pGameData->m_iSmart > 1 )
    {
        iVal = iaSupplies[CRaceDef::gas];
        iVal += ( 400 * pGameData->m_iSmart );
        pPlayer->m_InitData.SetSupplies( CRaceDef::gas, iVal );

        iVal = iaSupplies[CRaceDef::steel];
        iVal += ( 300 * pGameData->m_iSmart );
        pPlayer->m_InitData.SetSupplies( CRaceDef::steel, iVal );

        iVal = iaSupplies[CRaceDef::coal];
        iVal += ( 200 * pGameData->m_iSmart );
        pPlayer->m_InitData.SetSupplies( CRaceDef::coal, iVal );

        iVal = iaSupplies[CRaceDef::iron];
        iVal += ( 200 * pGameData->m_iSmart );
        pPlayer->m_InitData.SetSupplies( CRaceDef::iron, iVal );
    }

    // limit how many types of units can be increased in number
    int iLimit = pGameData->m_iSmart;

    // infantry has best chance for a bump
    iVal = 2;
    if ( iaSupplies[CRaceDef::infantry] )
        iVal = iaSupplies[CRaceDef::infantry];
    if ( iLimit )
    {
        if ( pGameData->GetRandom( pGameData->m_iSmart ) )
        {
            iVal += 1;  // pGameData->GetRandom(pGameData->m_iSmart);
            iLimit--;
        }
    }
    pPlayer->m_InitData.SetSupplies( CRaceDef::infantry, iVal );

    // must have cranes and trucks to start with
    iVal = iaSupplies[CRaceDef::crane];
    if ( iLimit )
    {
        if ( pGameData->GetRandom( pGameData->m_iSmart ) )
        {
            iVal += 1;  // pGameData->GetRandom(pGameData->m_iSmart);
            if ( pGameData->m_iSmart < 2 )
                iLimit--;
        }
    }
    // minimum number of cranes on impossible
    if ( pGameData->m_iSmart == ( NUM_DIFFICUTY_LEVELS - 1 ) )
    {
        if ( iVal < 2 )
            iVal = 2;
    }
    else  // all other difficulty settings
    {
        if ( iVal < 1 )
            iVal = 1;
    }
    pPlayer->m_InitData.SetSupplies( CRaceDef::crane, iVal );

    // trucks start the same as cranes
    if ( iaSupplies[CRaceDef::heavy_truck] )
        iVal = iaSupplies[CRaceDef::heavy_truck];

    // can we still upgrade?
    if ( iLimit )
    {
        // let difficulty decide if there is a bump chance taken
        if ( pGameData->GetRandom( pGameData->m_iSmart ) )
        {
            iVal += 1;  // pGameData->GetRandom(pGameData->m_iSmart);
            if ( pGameData->m_iSmart < 2 )
                iLimit--;
        }
    }
    pPlayer->m_InitData.SetSupplies( CRaceDef::heavy_truck, iVal );

    // IFVs need to almost match infantry
    // if difficult or impossible
    if ( pGameData->m_iSmart >= 2 )
    {
        iVal = iaSupplies[CRaceDef::infantry_carrier];
        if ( iLimit )
        {
            if ( pGameData->GetRandom( pGameData->m_iSmart ) )
            {
                iVal += 1;  // pGameData->m_iSmart;
                iLimit--;
            }
        }
        pPlayer->m_InitData.SetSupplies( CRaceDef::infantry_carrier, iVal );

        // outriders
        iVal = iaSupplies[CRaceDef::light_scout];
        if ( iLimit )
        {
            if ( pGameData->GetRandom( pGameData->m_iSmart ) )
            {
                iVal += 1;  // pGameData->GetRandom(pGameData->m_iSmart);
                iLimit--;
            }
        }
        pPlayer->m_InitData.SetSupplies( CRaceDef::light_scout, iVal );
    }

    // BUGBUG make sure outriders exist at all difficulty levels
    if ( pGameData->m_iSmart > 1 && !iaSupplies[CRaceDef::light_scout] )
        pPlayer->m_InitData.SetSupplies( CRaceDef::light_scout, pGameData->m_iSmart );


    // only on impossible
    if ( pGameData->m_iSmart == ( NUM_DIFFICUTY_LEVELS - 1 ) )
    {
        // destructors - a hover tank
        iVal = iaSupplies[CRaceDef::light_tank];
        if ( iLimit )
        {
            if ( pGameData->GetRandom( pGameData->m_iSmart ) )
            {
                iVal += 1;  // pGameData->GetRandom(pGameData->m_iSmart);
                iLimit--;
            }
        }
        pPlayer->m_InitData.SetSupplies( CRaceDef::light_tank, iVal );
    }

    // spotters
    // if difficult or impossible
    if ( pGameData->m_iSmart >= 2 )
    {
        iVal = iaSupplies[CRaceDef::med_scout];
        if ( iLimit )
        {
            if ( pGameData->GetRandom( pGameData->m_iSmart ) )
            {
                iVal += 1;  // pGameData->GetRandom(pGameData->m_iSmart);
                iLimit--;
            }
        }
        pPlayer->m_InitData.SetSupplies( CRaceDef::med_scout, iVal );
    }

    // the boss tank
    if ( pGameData->m_iSmart == ( NUM_DIFFICUTY_LEVELS - 1 ) )
    {
        iVal = iaSupplies[CRaceDef::med_tank];
        if ( iLimit )
        {
            if ( pGameData->GetRandom( pGameData->m_iSmart ) )
            {
                iVal += 1;  // pGameData->GetRandom(pGameData->m_iSmart);
                iLimit--;
            }
        }
        pPlayer->m_InitData.SetSupplies( CRaceDef::med_tank, iVal );

        // mortar
        iVal = iaSupplies[CRaceDef::light_art];
        if ( iLimit )
        {
            if ( pGameData->GetRandom( pGameData->m_iSmart ) )
            {
                iVal += 1;  // pGameData->GetRandom(pGameData->m_iSmart);
                iLimit--;
            }
        }
        pPlayer->m_InitData.SetSupplies( CRaceDef::light_art, iVal );
    }

    // a bump remains and on a difficulty setting
    if ( iLimit && pGameData->m_iSmart )
    {
        // so force trucks to be bumped
        iVal = pPlayer->m_InitData.GetSupplies( CRaceDef::heavy_truck );
        iVal += iLimit;
        pPlayer->m_InitData.SetSupplies( CRaceDef::heavy_truck, iVal );
    }

    // BUGBUG
    // AI hates water, never select an island
    pPlayer->m_InitData.SetSupplies( CRaceDef::island, 0 );

    // let difficulty determine ocean for game
    iVal = pGameData->GetRandom( pGameData->m_iSmart );  // 1 is beach 0 is not
    if ( iVal )
        iVal = 0;  // more difficulty increases chance of no ocean
    else
        iVal = 1;

    // AI will never place anything on a small world if there is an ocean
    if ( pGameData->m_iHexPerBlk <= 32 )
        iVal = 0;

    pPlayer->m_InitData.SetSupplies( CRaceDef::ocean, iVal );

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::AdjustAttribs(): player %d ocean=%d \n", m_iPlayer, iVal );
#endif
#endif
}

//
// this gets called in response to an err_place_bldg for the rocket
//
void CAIMgr::ReplaceRocket( CAIMsg* pMsg )
{
    // may need to remove original rocket
    if ( pMsg->m_dwID )
    {
        CAIUnit* pUnit = (CAIUnit*)m_plUnits->GetUnit( pMsg->m_dwID );
        if ( pUnit != NULL )
            m_plUnits->RemoveUnit( pMsg->m_dwID );
    }

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::ReplaceRocket(): player %d rocket id=%ld type=%d \n", m_iPlayer,
               pMsg->m_dwID, pMsg->m_idata1 );
#endif

    CAIInitPos* pInit;
    try
    {
        // pInit = new CAIInitPos( m_iPlayer, m_plMaps, m_plUnits );
        pInit = new CAIInitPos( m_iPlayer, m_pMap, m_plUnits );
    }
    catch ( CException* e )
    {
        throw( ERR_CAI_BAD_NEW );
    }

    // try to place rocket again
    pInit->PlaceRocket( );
    delete pInit;

    m_pMap->RocketRoad( );
}

void CAIMgr::SetInitialPos( void )
{
    CAIInitPos* pInit;
    try
    {
        // pInit = new CAIInitPos( m_iPlayer, m_plMaps, m_plUnits );
        pInit = new CAIInitPos( m_iPlayer, m_pMap, m_plUnits );
    }
    catch ( CException* e )
    {
        throw( ERR_CAI_BAD_NEW );
    }

    //
    // place initial units
    pInit->DoIt( );
    delete pInit;
}

void CAIMgr::SaveGame( CFile* pFile )
{

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::SaveGame() player %d \n", m_iPlayer );
#endif

    try
    {
        Save( pFile );  // does the CAIMgr data and includes messages

        m_plUnits->Save( pFile );
        m_pMap->Save( pFile );
        m_plOpFors->Save( pFile );
        m_pGoalMgr->Save( pFile );
        m_pRouter->Save( pFile );
        // m_pTaskMgr->Save( pFile );
    }
    catch ( CFileException* theException )
    {
        // how should write errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::SaveGame() completed \n\n" );
#endif
}
//  save the following:
//
//	m_iPlayer = iPlayer;
//	m_bIsAI = TRUE;
//  m_iBlockX
//  m_iBlockY
//
//  and the contents of m_plMsgQueue and m_plTmpQueue
//
void CAIMgr::Save( CFile* pFile )
{
    // CAIHexBuff aiHexBuff;

    try
    {
        pFile->Write( (const void*)&m_iPlayer, sizeof( int ) );
        pFile->Write( (const void*)&m_bIsAI, sizeof( BOOL ) );
        pFile->Write( (const void*)&m_iBlockX, sizeof( int ) );
        pFile->Write( (const void*)&m_iBlockY, sizeof( int ) );
    }
    catch ( CFileException* theException )
    {
        // how should write errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }

    MsgBuff mbMessage;

    // these are sorted in priority order, so they must
    // be restored as the first one out is head of list
    if ( m_plMsgQueue != NULL )
    {
        EnterCriticalSection( &m_cs );

        int iCnt = m_plMsgQueue->GetCount( );
        try
        {
            pFile->Write( (const void*)&iCnt, sizeof( int ) );
        }
        catch ( CFileException* theException )
        {
            // how should write errors be reported?
            LeaveCriticalSection( &m_cs );
            throw( ERR_CAI_BAD_FILE );
        }

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::SaveGame() saved %d messages from m_plMsgQueue ", iCnt );
#endif

        if ( iCnt )
        {
            POSITION pos = m_plMsgQueue->GetHeadPosition( );
            while ( pos != NULL )
            {
                CAIMsg* pMsg = (CAIMsg*)m_plMsgQueue->GetNext( pos );
                if ( pMsg != NULL )
                {
                    mbMessage.iMsg      = pMsg->m_iMsg;
                    mbMessage.iPriority = pMsg->m_iPriority;
                    mbMessage.uFlags    = pMsg->m_uFlags;
                    mbMessage.dwID      = pMsg->m_dwID;
                    mbMessage.iX        = pMsg->m_iX;
                    mbMessage.iY        = pMsg->m_iY;
                    mbMessage.ieX       = pMsg->m_ieX;
                    mbMessage.ieY       = pMsg->m_ieY;
                    mbMessage.idata1    = pMsg->m_idata1;
                    mbMessage.idata2    = pMsg->m_idata2;
                    mbMessage.idata3    = pMsg->m_idata3;
                    mbMessage.dwID2     = pMsg->m_dwID2;

                    try
                    {
                        pFile->Write( (const void*)&mbMessage, sizeof( MsgBuff ) );
                    }
                    catch ( CFileException* theException )
                    {
                        // how should write errors be reported?
                        LeaveCriticalSection( &m_cs );
                        throw( ERR_CAI_BAD_FILE );
                    }
                }
            }
        }
        LeaveCriticalSection( &m_cs );
    }

    // this queue is the first in, first out queue
    if ( m_plTmpQueue != NULL )
    {
        EnterCriticalSection( &m_cs );

        int iCnt = m_plTmpQueue->GetCount( );
        try
        {
            pFile->Write( (const void*)&iCnt, sizeof( int ) );
        }
        catch ( CFileException* theException )
        {
            // BUGBUG how should write errors be reported?
            LeaveCriticalSection( &m_cs );
            throw( ERR_CAI_BAD_FILE );
        }

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIMgr::SaveGame() saved %d messages from m_plTmpQueue ", iCnt );
#endif

        if ( iCnt )
        {
            POSITION pos = m_plTmpQueue->GetHeadPosition( );
            while ( pos != NULL )
            {
                CAIMsg* pMsg = (CAIMsg*)m_plTmpQueue->GetNext( pos );
                if ( pMsg != NULL )
                {
                    mbMessage.iMsg      = pMsg->m_iMsg;
                    mbMessage.iPriority = pMsg->m_iPriority;
                    mbMessage.uFlags    = pMsg->m_uFlags;
                    mbMessage.dwID      = pMsg->m_dwID;
                    mbMessage.iX        = pMsg->m_iX;
                    mbMessage.iY        = pMsg->m_iY;
                    mbMessage.ieX       = pMsg->m_ieX;
                    mbMessage.ieY       = pMsg->m_ieY;
                    mbMessage.idata1    = pMsg->m_idata1;
                    mbMessage.idata2    = pMsg->m_idata2;
                    mbMessage.idata3    = pMsg->m_idata3;
                    mbMessage.dwID2     = pMsg->m_dwID2;

                    try
                    {
                        pFile->Write( (const void*)&mbMessage, sizeof( MsgBuff ) );
                    }
                    catch ( CFileException* theException )
                    {
                        LeaveCriticalSection( &m_cs );
                        throw( ERR_CAI_BAD_FILE );
                    }
                }
            }
        }
        LeaveCriticalSection( &m_cs );
    }
}

void CAIMgr::LoadGame( CFile* pFile )
{
#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::LoadGame() \n" );
#endif

    try
    {
        Load( pFile );  // does the CAIMgr data and includes messages

        if ( m_plUnits == NULL )
            CreateCAUnits( );
        m_plUnits->Load( pFile );

        if ( m_pMap == NULL )
            CreateMap( );
        m_pMap->Load( pFile, m_plUnits );

        if ( m_plOpFors == NULL )
            CreateOpFors( );
        m_plOpFors->Load( pFile );

        if ( m_pGoalMgr == NULL )
            m_pGoalMgr = new CAIGoalMgr( TRUE, m_iPlayer, m_pMap, m_plUnits, m_plOpFors );
        m_pGoalMgr->Load( pFile, m_pMap, m_plUnits, m_plOpFors );
        // m_pTaskMgr->Load( pFile );

        if ( m_pRouter == NULL )
            m_pRouter = new CAIRouter( m_pMap, m_plUnits, m_iPlayer );
        m_pRouter->Load( pFile, m_plUnits );
    }
    catch ( CFileException* theException )
    {
        throw( ERR_CAI_BAD_FILE );
    }

    if ( m_pTaskMgr == NULL )
    {
        try
        {
            // CAITaskMgr::CAITaskMgr( BOOL bRestart, int iPlayer,
            //    CAIGoalMgr *pGoalMgr )
            m_pTaskMgr = new CAITaskMgr( FALSE, m_iPlayer, m_pGoalMgr );
        }
        catch ( CException* e )
        {
            if ( m_pTaskMgr != NULL )
            {
                delete m_pTaskMgr;
                m_pTaskMgr = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }
    }
}

void CAIMgr::Load( CFile* pFile )
{
    // CAIHexBuff aiHexBuff;

    // messages after prioritizing
    if ( m_plMsgQueue == NULL )
    {
        try
        {
            m_plMsgQueue = new CObList( );
        }
        catch ( CException* anException )
        {
            // BUGBUG how should memory errors be reported?
            m_plMsgQueue = NULL;
            throw( ERR_CAI_BAD_NEW );
        }
    }
    else
        m_plMsgQueue->RemoveAll( );

    // messages in first in first out order
    if ( m_plTmpQueue == NULL )
    {
        try
        {
            m_plTmpQueue = new CObList( );  // CList
        }
        catch ( CException* anException )
        {
            // BUGBUG how should memory errors be reported?
            m_plTmpQueue = NULL;
            throw( ERR_CAI_BAD_NEW );
        }
    }
    else
        m_plTmpQueue->RemoveAll( );

    int iCnt;
    try
    {
        pFile->Read( (void*)&iCnt, sizeof( int ) );
        m_iPlayer = iCnt;
        pFile->Read( (void*)&iCnt, sizeof( BOOL ) );
        m_bIsAI = iCnt;
        pFile->Read( (void*)&iCnt, sizeof( int ) );
        m_iBlockX = iCnt;
        pFile->Read( (void*)&iCnt, sizeof( int ) );
        m_iBlockY = iCnt;

        // now get the count of m_plMsgQueue messages
        pFile->Read( (void*)&iCnt, sizeof( int ) );
    }
    catch ( CFileException* theException )
    {
        // how should read errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }

    MsgBuff mbMessage;

    // get count of the prioritized messages
    if ( iCnt )
    {
        int iBytes;
        for ( int i = 0; i < iCnt; ++i )
        {
            // read file data into buffer
            try
            {
                iBytes = pFile->Read( (void*)&mbMessage, sizeof( MsgBuff ) );
            }
            catch ( CFileException* theException )
            {
                // how should read errors be reported?
                throw( ERR_CAI_BAD_FILE );
            }

            // BUGBUG how should read errors be reported?
            if ( iBytes != sizeof( MsgBuff ) )
                return;

            CAIMsg* pMsg = NULL;
            try
            {
                pMsg = new CAIMsg( );
            }
            catch ( CException* anException )
            {
                // how should memory errors be reported?
                throw( ERR_CAI_BAD_NEW );
            }

            // update the new message
            pMsg->m_iMsg      = mbMessage.iMsg;
            pMsg->m_uFlags    = mbMessage.uFlags;
            pMsg->m_iPriority = mbMessage.iPriority;
            pMsg->m_dwID      = mbMessage.dwID;
            pMsg->m_iX        = mbMessage.iX;
            pMsg->m_iY        = mbMessage.iY;
            pMsg->m_ieX       = mbMessage.ieX;
            pMsg->m_ieY       = mbMessage.ieY;
            pMsg->m_idata1    = mbMessage.idata1;
            pMsg->m_idata2    = mbMessage.idata2;
            pMsg->m_idata3    = mbMessage.idata3;
            pMsg->m_dwID2     = mbMessage.dwID2;

            // these are the prioritized messages
            m_plMsgQueue->AddTail( (CObject*)pMsg );
        }
    }

    // now get count of m_plTmpQueue messages
    try
    {
        // now get the count of m_plTmpQueue messages
        pFile->Read( (void*)&iCnt, sizeof( int ) );
    }
    catch ( CException* anException )
    {
        // how should read errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }

    if ( iCnt )
    {
        int iBytes;
        for ( int i = 0; i < iCnt; ++i )
        {
            // read file data into buffer
            try
            {
                iBytes = pFile->Read( (void*)&mbMessage, sizeof( MsgBuff ) );
            }
            catch ( CFileException* theException )
            {
                // how should read errors be reported?
                throw( ERR_CAI_BAD_FILE );
            }

            // BUGBUG how should read errors be reported?
            if ( iBytes != sizeof( MsgBuff ) )
                return;

            CAIMsg* pMsg = NULL;
            try
            {
                pMsg = new CAIMsg( );
            }
            catch ( CException* anException )
            {
                // how should memory errors be reported?
                throw( ERR_CAI_BAD_NEW );
            }

            // update the new message
            pMsg->m_iMsg      = mbMessage.iMsg;
            pMsg->m_uFlags    = mbMessage.uFlags;
            pMsg->m_iPriority = mbMessage.iPriority;
            pMsg->m_dwID      = mbMessage.dwID;
            pMsg->m_iX        = mbMessage.iX;
            pMsg->m_iY        = mbMessage.iY;
            pMsg->m_ieX       = mbMessage.ieX;
            pMsg->m_ieY       = mbMessage.ieY;
            pMsg->m_idata1    = mbMessage.idata1;
            pMsg->m_idata2    = mbMessage.idata2;
            pMsg->m_idata3    = mbMessage.idata3;
            pMsg->m_dwID2     = mbMessage.dwID2;

            // these are the prioritized messages
            m_plTmpQueue->AddTail( (CObject*)pMsg );
        }
    }
}

//
// an HP has left a multi-player game, and the AI is
// now assuming control of the HP's units and playing
// in place of the HP
//
void CAIMgr::AssumeControl( int iBlockX, int iBlockY )
{
    ASSERT_VALID( this );

    m_iBlockX = iBlockX;
    m_iBlockY = iBlockY;

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIMgr::AssumeControl() of player %d \n", m_iPlayer );
#endif

    CreateOpFors( );
    CreateCAUnits( );
    // now for each CBuilding and CVehicle of that HP
    // create a new CAIUnit
    ConvertHPUnits( );

    CreateMap( );
    CreateManagers( );

    pGameData->FindBuilding( CStructureData::rocket, m_iPlayer, m_pMap->m_pMapUtil->m_RocketHex );

#ifdef _LOGOUT
    CHexCoord hex;
    POSITION  pos = m_plUnits->GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pUnit = (CAIUnit*)m_plUnits->GetNext( pos );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            if ( pUnit->GetOwner( ) != m_iPlayer )
                continue;

            // determine if this is a building
            if ( pUnit->GetType( ) == CUnit::building )
            {
                EnterCriticalSection( &cs );
                CBuilding* pBldg = theBuildingMap.GetBldg( pUnit->GetID( ) );
                if ( pBldg != NULL )
                    hex = pBldg->GetExitHex( );
                LeaveCriticalSection( &cs );
            }
            else if ( pUnit->GetType( ) == CUnit::vehicle )
            {
                EnterCriticalSection( &cs );
                CVehicle* pVehicle = theVehicleMap.GetVehicle( pUnit->GetID( ) );
                if ( pVehicle != NULL )
                    hex = pVehicle->GetHexHead( );
                LeaveCriticalSection( &cs );
            }
        }

        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                   "CAIMgr::AssumeControl() of player %d unit %ld a %d/%d type at %d,%d \n", pUnit->GetOwner( ),
                   pUnit->GetID( ), pUnit->GetType( ), pUnit->GetTypeUnit( ), hex.X( ), hex.Y( ) );
    }

    ReportPlayerStats( );

#endif
}

void CAIMgr::CreateData( int iBlockX, int iBlockY )
{
    ASSERT_VALID( this );

    // BUGBUG need to evaluate start cell (iBlockX,iBlockY)
    // and select the cell with the most m_iStructureSize
    // blocks of cells in the region defined by start cell
    // +/- m_iHexPerBlk/2


    CAIStart aStartUp( iBlockX, iBlockY );
    // BUGBUG this uses new start hex module
    aStartUp.PickStartHex( );
    m_iBlockX = aStartUp.m_iBlockX;
    m_iBlockY = aStartUp.m_iBlockY;

    CreateOpFors( );

    CreateCAUnits( );

    CreateMap( );

    CreateManagers( );
}

void CAIMgr::CreateOpFors( void )
{
    if ( m_plOpFors == NULL )
    {
        try
        {
            m_plOpFors = new CAIOpForList( );
        }
        catch ( CException* e )
        {
            // BUGBUG how should this be reported?
            m_plOpFors = NULL;
            throw( ERR_CAI_BAD_NEW );
        }
    }
}

//
// process the units of the HP and create CAIUnits for each
// and while doing so, determine the known scope of the world
// for the HP so that CAIMaps can be created to reflect it
//
void CAIMgr::ConvertHPUnits( void )
{
    CVehicle*  pVehicle = NULL;
    CBuilding* pBldg    = NULL;
    DWORD      dwDumb;

    POSITION pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        theBuildingMap.GetNextAssoc( pos, dwDumb, pBldg );

        // consider only those buildings of this player
        if ( pBldg->GetOwner( )->GetPlyrNum( ) == m_iPlayer )
        {
            try
            {
                // CAIUnit( DWORD dwID, int iOwner );
                CAIUnit* pUnit =
                    new CAIUnit( pBldg->GetID( ), m_iPlayer, CUnit::building, pBldg->GetData( )->GetType( ) );
                ASSERT_VALID( pUnit );
                m_plUnits->AddTail( (CObject*)pUnit );
            }
            catch ( CException* e )
            {
                // need to report this error occurred
                throw( ERR_CAI_BAD_NEW );
            }
        }
    }

    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        theVehicleMap.GetNextAssoc( pos, dwDumb, pVehicle );

        // consider only those vehicles of this player
        if ( pVehicle->GetOwner( )->GetPlyrNum( ) == m_iPlayer )
        {
            try
            {
                // CAIUnit( DWORD dwID, int iOwner );
                CAIUnit* pUnit =
                    new CAIUnit( pVehicle->GetID( ), m_iPlayer, CUnit::vehicle, pVehicle->GetData( )->GetType( ) );
                ASSERT_VALID( pUnit );
                m_plUnits->AddTail( (CObject*)pUnit );
            }
            catch ( CException* e )
            {
                // need to report this error occurred
                throw( ERR_CAI_BAD_NEW );
            }
        }
    }

    // BUGBUG there is no need to know where the unit is!
    //
    // now iSX,iSY to iEX,iEY expresses the known scope of the
    // HP's holdings so the CAIMgr needs to construct additional
    // CAIMaps to reflect them
}

//
// Initially create the CAUnits managed by this manager
//
void CAIMgr::CreateCAUnits( void )
{
    ASSERT_VALID( this );

    // create a list to contain all player's units
    try
    {
        m_plUnits = new CAIUnitList( );
    }
    catch ( CException* e )
    {
        if ( m_plUnits != NULL )
        {
            delete m_plUnits;
            m_plUnits = NULL;
        }
        throw( ERR_CAI_BAD_NEW );
    }

    if ( m_plUnits != NULL )
    {
        ASSERT_VALID( m_plUnits );
    }
}

void CAIMgr::CreateManagers( void )
{
    ASSERT_VALID( this );

    try
    {
        // m_pRouter = new CAIRouter( m_plMaps, m_plUnits, m_iPlayer );
        m_pRouter = new CAIRouter( m_pMap, m_plUnits, m_iPlayer );
    }
    catch ( CException* e )
    {
        throw( ERR_CAI_BAD_NEW );
    }

    try
    {
        // CAIGoalMgr::CAIGoalMgr( BOOL bRestart, int iPlayer,
        //  CAIMapList *plMaps, CAIUnitList *plUnits,
        //  CAIOpForList *plOpFors )
        // m_pGoalMgr = new CAIGoalMgr( FALSE, m_iPlayer,
        //	m_plMaps, m_plUnits, m_plOpFors );
        m_pGoalMgr = new CAIGoalMgr( FALSE, m_iPlayer, m_pMap, m_plUnits, m_plOpFors );
    }
    catch ( CException* e )
    {
        if ( m_pGoalMgr != NULL )
        {
            delete m_pGoalMgr;
            m_pGoalMgr = NULL;
        }
        throw( ERR_CAI_BAD_NEW );
    }


    if ( m_pGoalMgr == NULL )
        return;

    try
    {
        // CAITaskMgr::CAITaskMgr( BOOL bRestart, int iPlayer,
        //    CAIGoalMgr *pGoalMgr )
        m_pTaskMgr = new CAITaskMgr( FALSE, m_iPlayer, m_pGoalMgr );
    }
    catch ( CException* e )
    {
        if ( m_pGoalMgr != NULL )
        {
            delete m_pGoalMgr;
            m_pGoalMgr = NULL;
        }
        if ( m_pTaskMgr != NULL )
        {
            delete m_pTaskMgr;
            m_pTaskMgr = NULL;
        }
        throw( ERR_CAI_BAD_NEW );
    }
}
//
// BUGBUG consider moving the plMapList of CAUnits
// of this manager to be a member of CAIMap class
//
void CAIMgr::CreateMap( void )
{
    ASSERT_VALID( this );

    try
    {
        // BUGBUG the map size is not calculated this way
        m_pMap = new CAIMap( m_iPlayer, m_plUnits, m_iBlockX, m_iBlockY,
                             ( pGameData->m_iHexPerBlk * pGameData->m_iBlkPerSide ),
                             ( pGameData->m_iHexPerBlk * pGameData->m_iBlkPerSide ) );

        // now the map has been created, it also was initialized
        // and updated with the current status of the game world
    }
    catch ( CException* e )
    {
        if ( m_pMap != NULL )
        {
            delete m_pMap;
            m_pMap = NULL;
        }
        throw( ERR_CAI_BAD_NEW );
    }
}

CAIMgr::~CAIMgr( )
{
    ASSERT_VALID( this );

    DeleteCriticalSection( &m_cs );

    if ( m_pRouter != NULL )
    {
        delete m_pRouter;
        m_pRouter = NULL;
    }

    if ( m_pGoalMgr != NULL )
    {
        delete m_pGoalMgr;
        m_pGoalMgr = NULL;
    }

    if ( m_pTaskMgr != NULL )
    {
        delete m_pTaskMgr;
        m_pTaskMgr = NULL;
    }

    // delete CAI map list
    if ( m_pMap != NULL )
    {
        delete m_pMap;
        m_pMap = NULL;
    }

    // delete CAI units list
    if ( m_plUnits != NULL )
    {
        delete m_plUnits;
        m_plUnits = NULL;
    }

    // delete COpFor list
    if ( m_plOpFors != NULL )
    {
        delete m_plOpFors;
        m_plOpFors = NULL;
    }

    // delete message queues
    if ( m_plMsgQueue != NULL )
    {
        if ( m_plMsgQueue->GetCount( ) )
        {
            POSITION pos = m_plMsgQueue->GetHeadPosition( );
            while ( pos != NULL )
            {
                CAIMsg* pMsg = (CAIMsg*)m_plMsgQueue->GetNext( pos );
                if ( pMsg != NULL )
                {
                    delete pMsg;
                }
            }
        }
        m_plMsgQueue->RemoveAll( );

        delete m_plMsgQueue;
        m_plMsgQueue = NULL;
    }
    if ( m_plTmpQueue != NULL )
    {
        if ( m_plTmpQueue->GetCount( ) )
        {
            POSITION pos = m_plTmpQueue->GetHeadPosition( );
            while ( pos != NULL )
            {
                CAIMsg* pMsg = (CAIMsg*)m_plTmpQueue->GetNext( pos );
                if ( pMsg != NULL )
                {
                    delete pMsg;
                }
            }
        }
        m_plTmpQueue->RemoveAll( );

        delete m_plTmpQueue;
        m_plTmpQueue = NULL;
    }
}

#ifdef _LOGOUT

void CAIMgr::ReportPlayerStats( void )
{
    EnterCriticalSection( &cs );

    CPlayer* pPlayer = pGameData->GetPlayerData( m_iPlayer );
    if ( pPlayer == NULL )
    {
        LeaveCriticalSection( &cs );
        return;
    }

    /*
    int			GetPwrNeed () { ASSERT_VALID (this); return (m_iPwrNeed); }
    int			GetPplNeed () { ASSERT_VALID (this); return (m_iPplNeed); }
    int			GetPwrHave () { ASSERT_VALID (this); return (m_iPwrHave); }
    int			GetPplHave () { ASSERT_VALID (this); return (m_iPplHave); }
    int			GetFood () { ASSERT_VALID (this); return (m_iFood); }
    int			GetFoodProd () const { ASSERT_STRICT_VALID (this); return (m_iFoodProd); }
    float		GetPplMult () { ASSERT_VALID (this); return (m_fPplMult); }
    float		GetPwrMult () { ASSERT_VALID (this); return (m_fPwrMult); }
    float		GetConstProd () { ASSERT_VALID (this); return (m_fConstProd); }
    float		GetMtrlsProd () { ASSERT_VALID (this); return (m_fMtrlsProd); }
    float		GetManfProd () { ASSERT_VALID (this); return (m_fManfProd); }
    float		GetPopGrowth () { ASSERT_VALID (this); return (m_fPopGrowth); }
    float		GetPopDeath () { ASSERT_VALID (this); return (m_fPopDeath); }
    float		GetEatingRate () { ASSERT_VALID (this); return (m_fEatingRate); } printf
    */
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nPlayer %d current stats ", m_iPlayer );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "GetPwrNeed() %d, GetPwrHave() %d ", pPlayer->GetPwrNeed( ),
               pPlayer->GetPwrHave( ) );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "GetGasNeed() %d, GetGasHave() %d ", pPlayer->GetGasNeed( ),
               pPlayer->GetGasHave( ) );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "GetPplMult() %f, GetPwrMult() %f ", pPlayer->GetPplMult( ),
               pPlayer->GetPwrMult( ) );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "GetConstProd() %f, GetMtrlsProd() %f, GetManfProd() %f ",
               pPlayer->GetConstProd( ), pPlayer->GetMtrlsProd( ), pPlayer->GetManfProd( ) );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "GetPopGrowth() %f, GetPopDeath() %f", pPlayer->GetPopGrowth( ),
               pPlayer->GetPopDeath( ) );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "GetFood() %d GetFoodProd() %d GetFoodNeed() %d ", pPlayer->GetFood( ),
               pPlayer->GetFoodProd( ), pPlayer->GetFoodNeed( ) );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "GetPplBldg() %d  GetPplVeh() %d ", pPlayer->GetPplBldg( ),
               pPlayer->GetPplVeh( ) );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "GetPplTotal() %d  GetPplNeedBldg() %d  ", pPlayer->GetPplTotal( ),
               pPlayer->GetPplNeedBldg( ) );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "m_iOfcCap %ld, m_iAptCap %ld \n", pPlayer->m_iOfcCap, pPlayer->m_iAptCap );

    // just get enough people to test something!
    // if( pPlayer->GetPplHave() < 100 )
    //	pPlayer->AddPplHave(100);
    // and enough power
    // if( pPlayer->GetPwrHave() < 400 )
    //	pPlayer->AddPwrHave(400);
    // if( pPlayer->GetFood() < 2000 )
    //	pPlayer->AddFood(1000);

    LeaveCriticalSection( &cs );

    // now report the total on hand of
    // lumber, coal, iron, copper, oil, steel
    // by going through buildings and getting
    // stores and vehicles and getting onboard
    // if trucks and summing by material
    //
    SumUpMaterialOnHand( );
}

void CAIMgr::SumUpMaterialOnHand( void )
{
    // now report the total on hand of
    // lumber, coal, iron, copper, oil, steel
    // by going through buildings and getting
    // stores and vehicles and getting onboard
    // if trucks and summing by material
    int iaMats[CMaterialTypes::num_types];
    for ( int i = 0; i < CMaterialTypes::num_types; ++i ) iaMats[i] = 0;

    int iVehPeople  = 0;
    int iBldgPeople = 0;

    POSITION pos = m_plUnits->GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pUnit = (CAIUnit*)m_plUnits->GetNext( pos );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            if ( pUnit->GetOwner( ) != m_iPlayer )
                continue;

            // determine if this is a building
            if ( pUnit->GetType( ) == CUnit::building )
            {
                EnterCriticalSection( &cs );
                CBuilding* pBldg = theBuildingMap.GetBldg( pUnit->GetID( ) );
                if ( pBldg != NULL )
                {
                    iBldgPeople += pBldg->GetData( )->GetPeople( );

                    for ( int i = 0; i < CMaterialTypes::num_types; ++i ) iaMats[i] += pBldg->GetStore( i );
                }
                LeaveCriticalSection( &cs );
            }
            else if ( pUnit->GetType( ) == CUnit::vehicle )
            {
                EnterCriticalSection( &cs );
                CVehicle* pVehicle = theVehicleMap.GetVehicle( pUnit->GetID( ) );
                if ( pVehicle != NULL )
                {
                    iVehPeople += pVehicle->GetData( )->GetPeople( );

                    for ( int i = 0; i < CMaterialTypes::num_types; ++i ) iaMats[i] += pVehicle->GetStore( i );
                }
                LeaveCriticalSection( &cs );
            }
        }
    }
    /*
            enum { lumber,			// lumber - goods must be 0-based
                        steel,
                        copper,
                        moly,
                        goods,

                        food,
                        oil,
                        gas,
                        coal,
                        iron,
                        num_types,
                        num_build_types = food };	// goods + 1
    */
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "SumUpMaterialOnHand(): player %d people in buildings %d vehicles %d",
               m_iPlayer, iBldgPeople, iVehPeople );

    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "SumUpMaterialOnHand(): player %d material %d quantity %d", m_iPlayer,
                   i, iaMats[i] );
    }
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, " " );

#if 0
	CPlayer *pPlayer = pGameData->GetPlayerData(m_iPlayer);
	if( pPlayer == NULL )
		return;

	for( i=0; i<CRaceDef::num_supplies; i++)
	{
		logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
			"SumUpInitSupplies(): player %d supply %d quantity %d",
			m_iPlayer, i, pPlayer->m_InitData.GetSupplies(i) );
	}
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, " " );
#endif
}

#endif

/////////////////////////////////////////////////////////////////////////////

CAIMgrList::CAIMgrList( )
{
    LoadStandardData( );
}

void CAIMgrList::LoadStandardData( void )
{
    ASSERT_VALID( this );

    // declare a saveload utility object
    CAISavLd aSavLd( NULL );

    // create standard goals and task list
    try
    {
        plTaskList = new CAITaskList( );
        plGoalList = new CAIGoalList( );

        // int iSizeOne = NUM_RACE_CHAR + NUM_INIT_SPLYS;
        // int iSize = iSizeOne * NUM_DIFFICUTY_LEVELS;
        // pwaRCIP = new CWordArray();
        // pwaRCIP->SetSize( iSize );

        int iSize = NUM_INITIAL_GOALS * NUM_DIFFICUTY_LEVELS;
        pwaIG     = new CWordArray( );
        pwaIG->SetSize( iSize );
    }
    catch ( CException* e )
    {
        if ( plTaskList != NULL )
        {
            delete plTaskList;
            plTaskList = NULL;
        }
        if ( plGoalList != NULL )
        {
            delete plGoalList;
            plGoalList = NULL;
        }
        /*
        if( pwaRCIP != NULL )
        {
            pwaRCIP->RemoveAll();
            delete pwaRCIP;
            pwaRCIP = NULL;
        }
        */
        if ( pwaIG != NULL )
        {
            pwaIG->RemoveAll( );
            delete pwaIG;
            pwaIG = NULL;
        }

        throw( ERR_CAI_BAD_NEW );
    }

    aSavLd.LoadBinary( );
}

CAIMgrList::~CAIMgrList( void )
{
    ASSERT_VALID( this );

    // delete standard data
    if ( plTaskList != NULL )
    {
        delete plTaskList;
        plTaskList = NULL;
    }
    if ( plGoalList != NULL )
    {
        delete plGoalList;
        plGoalList = NULL;
    }
    /*
    if( pwaRCIP != NULL )
    {
        pwaRCIP->RemoveAll();
        delete pwaRCIP;
        pwaRCIP = NULL;
    }
    */
    if ( pwaIG != NULL )
    {
        pwaIG->RemoveAll( );
        delete pwaIG;
        pwaIG = NULL;
    }

    DeleteList( );
}

void CAIMgrList::RemoveManager( int iPlayer )
{
    ASSERT_VALID( this );

    POSITION pos1, pos2;
    for ( pos1 = GetHeadPosition( ); ( pos2 = pos1 ) != NULL; )
    {
        CAIMgr* pMgr = (CAIMgr*)GetNext( pos1 );
        if ( pMgr != NULL )
        {
            ASSERT_VALID( pMgr );

            if ( pMgr->GetPlayer( ) != iPlayer )
                continue;
        }

        pMgr = (CAIMgr*)GetAt( pos2 );
        if ( pMgr != NULL )
        {
            ASSERT_VALID( pMgr );

            RemoveAt( pos2 );
            delete pMgr;
            break;
        }
    }
}

//
// return a pointer to the CAIMgr object matching the id passed
//
CAIMgr* CAIMgrList::GetManager( int iPlayer )
{
    ASSERT_VALID( this );

    POSITION pos = GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIMgr* pMgr = (CAIMgr*)GetNext( pos );
        if ( pMgr != NULL )
        {
            ASSERT_VALID( pMgr );

            if ( pMgr->GetPlayer( ) == iPlayer )
                return ( pMgr );
        }
    }
    return ( NULL );
}

//
// delete CAIMgr objects in the list
//
void CAIMgrList::DeleteList( void )
{
    ASSERT_VALID( this );

    if ( GetCount( ) )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
        {
            CAIMgr* pMgr = (CAIMgr*)GetNext( pos );
            if ( pMgr != NULL )
            {
                ASSERT_VALID( pMgr );

                delete pMgr;
            }
        }
    }
    RemoveAll( );
}


// end of CAIMgr.cpp
