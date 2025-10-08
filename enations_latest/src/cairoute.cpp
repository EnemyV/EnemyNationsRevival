////////////////////////////////////////////////////////////////////////////
//
//  CAIRoute.cpp : CAIRouter object implementation
//                 Divide and Conquer AI
//
//  Last update:  03/16/97
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "cairoute.hpp"

#include "caidata.hpp"
#include "logging.h"  // dave's logging system
#include "stdafx.h"

#define new DEBUG_NEW

#if THREADS_ENABLED
extern CException* pException;  // standard exception for yielding
#endif

extern CAIData*         pGameData;  // pointer to game data interface
extern CRITICAL_SECTION cs;         // used by threads

CAIRouter::CAIRouter( CAIMap* pMap, CAIUnitList* plUnits, int iPlayer )
{
    m_pMap    = pMap;
    m_plUnits = plUnits;
    m_iPlayer = iPlayer;

    m_dwRocket = 0;
    m_bNeedGas = FALSE;

    m_plTrucksAvailable = NULL;
    m_plBldgsNeed       = NULL;
}

CAIRouter::~CAIRouter( )
{
    if ( m_plTrucksAvailable != NULL )
    {
        m_plTrucksAvailable->RemoveAll( );
        delete m_plTrucksAvailable;
    }
    if ( m_plBldgsNeed != NULL )
    {
        m_plBldgsNeed->RemoveAll( );
        delete m_plBldgsNeed;
    }
}

//
// first time thru, if m_plBldgsNeed is NULL, then create
// it, and add to it those buildings that need commodities
// and then prioritize them, then sort the list so that
// the highest priority buildings are at the front.  Also
// create m_plTrucksAvailable as list of all available trucks.
//
// then for each building in the m_plBldgsNeed, find the
// nearest truck out of m_plTrucksAvailable and assigned
// it to the building
//
// all other times thru, review list and make sure trucks
// enroute meet needs of building;  on bldg_stat messages
// consider needs of that building and if needed add to
// list;  on veh_dest messages consider if a building in
// list is that destination, and if so, reconsider needs
// of the building, and if needs fullfilled, remove from
// m_plBldgsNeed;
//
void CAIRouter::DoRouting( CAIMsg* pMsg )
{
    ASSERT_VALID( this );

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIRouter for player %d ", m_iPlayer );

    if ( pMsg != NULL )
    {
        CString sMsg = pGameData->GetMsgString( pMsg->m_iMsg );

        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "Message id %d is a %s ", pMsg->m_iMsg, (const char*)sMsg );
        sMsg.Empty( );

        if ( pMsg->m_iMsg == CNetCmd::bldg_new && pMsg->m_idata3 == m_iPlayer )
        {
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "player %d unit %ld will be a %d at %d,%d ", pMsg->m_idata3,
                       pMsg->m_dwID, pMsg->m_idata1, pMsg->m_iX, pMsg->m_iY );
        }
    }
#endif

    // first time thru, then create and set up lists
    if ( m_plBldgsNeed == NULL )
    {
        try
        {
            m_plBldgsNeed       = new CAIUnitList( );
            m_plTrucksAvailable = new CAIUnitList( );
        }
        catch ( CException* e )
        {
            if ( m_plTrucksAvailable != NULL )
            {
                delete m_plTrucksAvailable;
                m_plTrucksAvailable = NULL;
            }
            if ( m_plBldgsNeed != NULL )
            {
                delete m_plBldgsNeed;
                m_plBldgsNeed = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }

        GetBuildingNeeding( );
        GetTrucksAvailable( );
    }

    // first and subsequent times thru
    if ( pMsg == NULL )
        return;

    // a new building that is placed or to be constructed
    if ( pMsg->m_iMsg == CNetCmd::bldg_new )
    {
        // not this player's building
        if ( pMsg->m_idata3 != m_iPlayer )
            return;

        // not a building that needs/produces materials
        // if( pMsg->m_idata1 != CStructureData::rocket &&
        //	pMsg->m_idata1 < CStructureData::barracks_2 )
        //	return;

        CAIUnit* pBldg = m_plUnits->GetUnit( pMsg->m_dwID );
        if ( pBldg == NULL )
            return;

        // consider the commodity requirments
        if ( NeedsCommodities( pBldg ) )
        {
            // consider adding building to buildings need list
            if ( m_plBldgsNeed->GetUnit( pBldg->GetID( ) ) == NULL )
                m_plBldgsNeed->AddTail( (CObject*)pBldg );

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "Player %d Building %ld needs commodites ", m_iPlayer,
                       pBldg->GetID( ) );
#endif
            SetPriorities( );  // prioritize construction/production
        }
    }

    // a building status change
    if ( pMsg->m_iMsg == CNetCmd::bldg_stat )
    {
        if ( pMsg->m_idata3 != m_iPlayer )
            return;

        // building is completed, consider its needs
        if ( ( pMsg->m_uFlags == CMsgBldgStat::built && pMsg->m_idata2 == 100 ) ||
             pMsg->m_uFlags == CMsgBldgStat::out_mat )
        {
            CAIUnit* pBldg = m_plUnits->GetUnit( pMsg->m_dwID );
            if ( pBldg == NULL )
                return;

            // consider the commodity requirments
            if ( NeedsCommodities( pBldg ) )
            {
                // consider adding building to buildings need list
                if ( m_plBldgsNeed->GetUnit( pBldg->GetID( ) ) == NULL )
                    m_plBldgsNeed->AddTail( (CObject*)pBldg );

#ifdef _LOGOUT
                logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "Player %d Building %ld needs commodites ", m_iPlayer,
                           pBldg->GetID( ) );
#endif
                SetPriorities( );  // prioritize construction/production
            }
        }
    }

    // a unit was destroyed
    //
    if ( pMsg->m_iMsg == CNetCmd::delete_unit )
    {
        if ( pMsg->m_idata3 != m_iPlayer )
            return;

        // BOOL CAIRouter::IsValidUnit( DWORD dwID )
        if ( IsValidUnit( pMsg->m_dwID ) )
        {
            CAIUnit* pUnit = m_plBldgsNeed->GetUnit( pMsg->m_dwID );
            if ( pUnit != NULL )
            {
                // if a building is destroyed, then unassign trucks
                // and add them back to m_plTrucksAvailable
                // that are enroute and remove the building
                // from m_plBldgsNeed
                UnassignTrucks( pUnit );
                m_plBldgsNeed->RemoveUnit( pUnit->GetID( ), FALSE );
            }
            else  // possibly a truck
            {
                // consider if it is an
                // assigned truck for a building, and if so, then
                // remove its assignment, also if not assigned
                UnassignTrucks( pMsg->m_dwID );

                // remove it from m_plTrucksAvailable, but confirm it first
                m_plTrucksAvailable->RemoveUnit( pMsg->m_dwID, FALSE );
            }
        }
        else
        {
            m_plBldgsNeed->RemoveUnit( pMsg->m_dwID, FALSE );
            m_plTrucksAvailable->RemoveUnit( pMsg->m_dwID, FALSE );
        }
        SetPriorities( );  // prioritize construction/production
    }

    // a new vehicle was built
    if ( pMsg->m_iMsg == CNetCmd::veh_new )
    {
        if ( pMsg->m_idata3 != m_iPlayer )
            return;

        // if vehicle is a truck, consider if it is assigned
        // already and if not, add it to available trucks list
        CAIUnit* pTruck = m_plUnits->GetUnit( pMsg->m_dwID );
        if ( pTruck == NULL )
            return;

        // make sure it is an unassigned truck
        if ( pGameData->IsTruck( pTruck->GetID( ) ) && !pTruck->GetDataDW( ) )
        {
            // the truck may not be assigned to transport, which
            // means that when it is, its params are cleared
            if ( pTruck->GetTask( ) != IDT_SETTRANSPORT )
                pTruck->SetTask( IDT_SETTRANSPORT );

            // consider truck may already be in list, if not, add it
            if ( m_plTrucksAvailable->GetUnit( pMsg->m_dwID ) == NULL )
                m_plTrucksAvailable->AddTail( (CObject*)pTruck );
        }
        SetPriorities( );  // prioritize construction/production
    }
    // each time thru, try to fill priorities of buildings
    // and set trucks params with what they need to get
    // and where they get it and take it
    FillPriorities( );
}

//
// go thru the list of units, finding buildings that
// need materials, and add them to the list, and this
// only happens once
//
void CAIRouter::GetBuildingNeeding( void )
{
    POSITION pos = m_plUnits->GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pUnit = (CAIUnit*)m_plUnits->GetNext( pos );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

#if THREADS_ENABLED
            // BUGBUG this function must yield
            myYieldThread( );
            // if( myYieldThread() == TM_QUIT )
            //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
            if ( pUnit->GetOwner( ) != m_iPlayer )
                continue;

            // determine if this is a building
            if ( pUnit->GetType( ) == CUnit::building )
            {
                if ( pUnit->GetTypeUnit( ) == CStructureData::rocket )
                    m_dwRocket = pUnit->GetID( );

                // not a building that needs/produces materials
                // if( pUnit->GetTypeUnit() < CStructureData::barracks_2 )
                //	continue;

                // consider the commodity requirments
                if ( NeedsCommodities( pUnit ) )
                {
                    m_plBldgsNeed->AddTail( (CObject*)pUnit );

#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "Player %d Building %ld needs commodites \n", m_iPlayer,
                               pUnit->GetID( ) );
#endif
                }
            }
        }
    }
}

//
// now for each CAIUnit that is is in the list
// find the commodities that are needed, based on who needs
// what the soonest, and set the priority flag in the pUnit
//
void CAIRouter::SetPriorities( void )
{
#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::SetPriorities during Routing() for player %d ", m_iPlayer );
#endif

    POSITION pos = m_plBldgsNeed->GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pUnit = (CAIUnit*)m_plBldgsNeed->GetNext( pos );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
            // this is a building under construction,
            // or a building producing a vehicle or commodity
            // and it needs a commodity to continue
            //
            // this building is given an updated priority based
            // on its current state, with the priority stored
            // in the CAIUnit::m_pwaParams[CAI_UNASSIGNED]
            //
            // priority will be based on what is being built
            // for now, and BUGBUG later will also be influenced
            // by the state of the game
            //

            // make sure it still exists
            if ( !IsValidUnit( pUnit ) )
                continue;

            SetUnitPriority( pUnit );
        }
    }
}
//
// Process all buildings with a status of CAI_NEED_COMMODITY
// in a priority order.  For each building, find TRUCKS that
// either contain one of the commodities needed by the building
// or are empty and enroute to destination buildings which are
// not producing anything and possess a quantity of one of the
// needed commodities.  For each TRUCK found, set its status
// to be CAI_IN_USE.  For each commodity needed, for which a
// truck is found and assigned, reduce the priority of the
// building.  If no sources are available the loop must break
//


void CAIRouter::FillPriorities( void )
{
    BOOL bTrucksFound = TRUE;

    // just in case we get here before they get init-ed
    if ( m_plBldgsNeed == NULL || m_plTrucksAvailable == NULL )
        return;

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::FillPriorities for player %d \n ", m_iPlayer );
#endif

    // go thru the list several times, each time handling
    // the building with the highest current priority
    int iCntBldgs = m_plBldgsNeed->GetCount( );
    while ( iCntBldgs )
    {
        // now dwPicked contains the DWORD dwID of the CAIUnit
        // with the current highest priority
        CAIUnit* pBldg = (CAIUnit*)m_plBldgsNeed->RemoveHead( );

        // DNT - this has GPF'ed several times
        // it's coming here, not null but either deleted or being deleted
        if ( pBldg != NULL  ) 
        {
            try
            {
                CAIUnit* pTest = m_plUnits->GetUnit( pBldg->GetID( ) );
                if ( pTest != pBldg )
                    pBldg = NULL;
            }
            catch ( ... )
            {
                TRAP( );
                pBldg = NULL;
            }
        }

        if ( pBldg != NULL )
        {

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                       "CAIRouter::FillPriorities for player %d building %ld off head of list ", m_iPlayer,
                       pBldg->GetID( ) );
#endif
            bTrucksFound = FindTransport( pBldg );
        }
        // a TRUE does not put the building back in the list
        // while FALSE does add at the tail
        if ( !bTrucksFound )
            m_plBldgsNeed->AddTail( (CObject*)pBldg );

        iCntBldgs--;  // should only inerate same as count
    }

    // consider moving some critical materials to specific buildings
    //
    // move steel from smelter to barracks_2, light_0, light_1, light_2, heavy
    if ( m_plTrucksAvailable->GetCount( ) > MINIMUM_IDLE_TRUCKS_AI )
        IdleTruckTask( CMaterialTypes::steel, CStructureData::smelter, CStructureData::num_types );
    // move lumber from lumber to barracks_2, light_0, light_1, light_2, heavy
    if ( m_plTrucksAvailable->GetCount( ) > MINIMUM_IDLE_TRUCKS_AI )
        IdleTruckTask( CMaterialTypes::lumber, CStructureData::lumber, CStructureData::num_types );
    // move oil from oil_well to refinery
    if ( m_plTrucksAvailable->GetCount( ) > MINIMUM_IDLE_TRUCKS_AI )
        IdleTruckTask( CMaterialTypes::oil, CStructureData::oil_well, CStructureData::refinery );
    // move coal from coal and iron from iron to smelter
    if ( m_plTrucksAvailable->GetCount( ) > MINIMUM_IDLE_TRUCKS_AI )
        IdleTruckTask( CMaterialTypes::coal, CStructureData::coal, CStructureData::smelter );
    if ( m_plTrucksAvailable->GetCount( ) > MINIMUM_IDLE_TRUCKS_AI )
        IdleTruckTask( CMaterialTypes::iron, CStructureData::iron, CStructureData::smelter );
    // move xillium from mine to rocket
    if ( m_plTrucksAvailable->GetCount( ) > MINIMUM_IDLE_TRUCKS_AI )
        IdleTruckTask( CMaterialTypes::copper, CStructureData::copper, CStructureData::rocket );
}

#if 0

void CAIRouter::FillPriorities( void )
{
	WORD wPriority = 0;
	DWORD dwPicked = 0;
	BOOL bTrucksRemain = TRUE;
	
	// just in case we get here before they get init-ed
	if( m_plBldgsNeed == NULL ||
		m_plTrucksAvailable == NULL )
		return;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIRouter::FillPriorities for player %d \n ", m_iPlayer );
#endif

	// go thru the list several times, each time handling
	// the building with the highest current priority
	int iCntBldgs = m_plBldgsNeed->GetCount();
	while( iCntBldgs )
	{
    	POSITION pos = m_plBldgsNeed->GetHeadPosition();
    	while( pos != NULL )
    	{   
        	CAIUnit *pUnit = (CAIUnit *)m_plBldgsNeed->GetNext( pos );
        	if( pUnit != NULL )
        	{
          		ASSERT_VALID( pUnit );

#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIRouter::FillPriorities for player %d building %ld priority %d ", 
m_iPlayer, pUnit->GetID(), pUnit->GetParam(CAI_UNASSIGNED) );
#endif
				// make sure it still exists
				if( !IsValidUnit( pUnit ) )
			 		continue;

				// this is a building under construction,
				// or a building producing a vehicle or commodity
				// and it needs a commodity to continue and
				// there are none on site.
				//
				// get current building priority and compare
				if( pUnit->GetParam( CAI_UNASSIGNED ) 
					> wPriority )
				{
					wPriority = pUnit->GetParam( CAI_UNASSIGNED );
					dwPicked = pUnit->GetID();
                }
			}
		}	// end of while( pos != NULL ) for units
		
		// all building units have been processed
		if( !wPriority )
			break;
		
		iCntBldgs--; // should only inerate same as count
			
		// now dwPicked contains the DWORD dwID of the CAIUnit
		// with the current highest priority
		CAIUnit *pBldg = m_plBldgsNeed->GetUnit( dwPicked );
		if( pBldg != NULL )
		{
			bTrucksRemain = FindTransport( pBldg );
		}
		// BUGBUG when a building is needing a material for which
		// there is no source, and it has a higher priority than
		// other buildings which have sources for the materials
		// they need, then this loop needs to go on to the next
		// building and try to find transport for its usage
		if( !bTrucksRemain )
			break;
		
		// get next highest priority building
		wPriority = 0;
		dwPicked = 0;
	}

	// consider moving some critical materials to specific buildings
	//
	// move steel from smelter to barracks_2, light_0, light_1, light_2, heavy
	if( m_plTrucksAvailable->GetCount() > MINIMUM_IDLE_TRUCKS_AI )
		IdleTruckTask( CMaterialTypes::steel, CStructureData::smelter,
			CStructureData::num_types );
	// move oil from oil_well to refinery
	if( m_plTrucksAvailable->GetCount() > MINIMUM_IDLE_TRUCKS_AI )
		IdleTruckTask( CMaterialTypes::oil, CStructureData::oil_well,
			CStructureData::refinery );
	// move coal from coal and iron from iron to smelter
	if( m_plTrucksAvailable->GetCount() > MINIMUM_IDLE_TRUCKS_AI )
		IdleTruckTask( CMaterialTypes::coal, CStructureData::coal,
			CStructureData::smelter );
	if( m_plTrucksAvailable->GetCount() > MINIMUM_IDLE_TRUCKS_AI )
		IdleTruckTask( CMaterialTypes::iron, CStructureData::iron,
			CStructureData::smelter );
	// move xillium from mine to rocket
	if( m_plTrucksAvailable->GetCount() > MINIMUM_IDLE_TRUCKS_AI )
		IdleTruckTask( CMaterialTypes::copper, CStructureData::copper,
			CStructureData::rocket );
}

#endif

//
// find the iFromBldg type with the most of iMat, and then the
// iToBldg type with the least of iMat, and if that to-building
// is not in the m_plBldgsNeed, select the nearest truck from
// m_plTrucksAvailable, and assign it to pick up truck capacity
// pGameData->GetMaterialCapacity(pTruck) of iMat and deliver
// it to the to-building
//
void CAIRouter::IdleTruckTask( int iMat, int iFromBldg, int iToBldg )
{
    int      iExcess = 0, iBestFromExcess = 0, iBestToExcess = 0xFFFE;
    CAIUnit* paiFrom = NULL;
    CAIUnit* paiTo   = NULL;
    CAIUnit* pUnit   = NULL;

    POSITION pos = m_plUnits->GetHeadPosition( );
    while ( pos != NULL )
    {
        pUnit = (CAIUnit*)m_plUnits->GetNext( pos );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            if ( pUnit->GetOwner( ) != m_iPlayer )
                continue;
            // determine if this is a building
            if ( pUnit->GetType( ) != CUnit::building )
                continue;

            if ( pUnit->GetTypeUnit( ) == iFromBldg || pUnit->GetTypeUnit( ) == iToBldg )
            {

                // make sure the neither building does not already need material
                // if( pUnit->GetTypeUnit() == iFromBldg )
                //{
                CAIUnit* paiBldgNeed = m_plBldgsNeed->GetUnitNY( pUnit->GetID( ) );
                if ( paiBldgNeed != NULL )
                    continue;
                    //}

#if THREADS_ENABLED
                myYieldThread( );
#endif

                // this is a valid iFromBldg/iToBldg, so check how much iMat it has
                EnterCriticalSection( &cs );
                CBuilding* pBldg = theBuildingMap.GetBldg( pUnit->GetID( ) );
                if ( pBldg != NULL )
                {
                    if ( pUnit->GetTypeUnit( ) == iFromBldg )
                    {
                        iExcess = pBldg->GetStore( iMat ) - EXCESS_IDLE_MATERIALS;
                        if ( iExcess > 0 && iExcess > iBestFromExcess )
                        {
                            iBestFromExcess = iExcess;
                            paiFrom         = pUnit;
                        }
                    }
                    else if ( pUnit->GetTypeUnit( ) == iToBldg )
                    {
                        iExcess = pBldg->GetStore( iMat );
                        if ( iExcess > 0 && iExcess < iBestToExcess )
                        {
                            iBestToExcess = iExcess;
                            paiTo         = pUnit;
                        }
                    }
                }
                LeaveCriticalSection( &cs );
            }

            // possible multiple to building types
            if ( iToBldg == CStructureData::num_types )
            {
                if ( pUnit->GetTypeUnit( ) != CStructureData::light_0 &&
                     pUnit->GetTypeUnit( ) != CStructureData::light_1 &&
                     pUnit->GetTypeUnit( ) != CStructureData::light_2 &&
                     pUnit->GetTypeUnit( ) != CStructureData::heavy &&
                     pUnit->GetTypeUnit( ) != CStructureData::barracks_2 &&
                     pUnit->GetTypeUnit( ) != CStructureData::repair &&
                     pUnit->GetTypeUnit( ) != CStructureData::shipyard_1 &&
                     pUnit->GetTypeUnit( ) != CStructureData::shipyard_3 )
                    continue;

                // make sure the building does not already need material

                CAIUnit* paiBldgNeed = m_plBldgsNeed->GetUnitNY( pUnit->GetID( ) );
                if ( paiBldgNeed != NULL )
                    continue;


#if THREADS_ENABLED
                myYieldThread( );
#endif

                // this is a valid iToBldg, so check how much iMat it has
                EnterCriticalSection( &cs );
                CBuilding* pBldg = theBuildingMap.GetBldg( pUnit->GetID( ) );
                if ( pBldg != NULL )
                {
                    iExcess = pBldg->GetStore( iMat );
                    if ( iExcess > 0 && iExcess < iBestToExcess )
                    {
                        iBestToExcess = iExcess;
                        paiTo         = pUnit;
                    }
                }
                LeaveCriticalSection( &cs );
            }
        }
    }

    // buildings not found with material above minimums
    if ( paiFrom == NULL || paiTo == NULL )
        return;

    CAIUnit* pTruck = GetNearestTruck( paiFrom );
    if ( pTruck == NULL )
    {

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                   "CAIRouter::IdleTruckTask() no truck available, count=%d for player %d building id %ld ",
                   m_plTrucksAvailable->GetCount( ), m_iPlayer, paiFrom->GetID( ) );
#endif
        return;
    }

    // check to be sure truck can get to the to building from the from building
    CHexCoord hexFrom( 0, 0 ), hexTo( 0, 0 );
    EnterCriticalSection( &cs );
    CBuilding* pBldg = theBuildingMap.GetBldg( paiFrom->GetID( ) );
    if ( pBldg != NULL )
        hexFrom = pBldg->GetExitHex( );
    pBldg = theBuildingMap.GetBldg( paiTo->GetID( ) );
    if ( pBldg != NULL )
        hexTo = pBldg->GetExitHex( );
    LeaveCriticalSection( &cs );

    if ( ( !hexFrom.X( ) && !hexFrom.Y( ) ) || ( !hexTo.X( ) && !hexTo.Y( ) ) )
        return;

    // truck can't get from hex -> to hex
    if ( !m_pMap->m_pMapUtil->GetPathRating( hexFrom, hexTo, pTruck->GetTypeUnit( ) ) )
        return;

    // update the truck and building as need commodiaties
    pTruck->ClearParam( );

    WORD wStatus = 0;
    wStatus |= CAI_IN_USE;
    pTruck->SetStatus( wStatus );          // truck is now assigned
    pTruck->SetDataDW( paiTo->GetID( ) );  // id of building needing

    // update the truck with the dwID of the building
    // that is a source for the material again using
    // offset of the material needed
    pTruck->SetParamDW( iMat, paiFrom->GetID( ) );
    pTruck->SetParam( iMat, EXCESS_IDLE_MATERIALS );
    pTruck->SetDestination( hexFrom );

    pTruck->SetParamDW( CAI_ROUTE_X, 0 );
    pTruck->SetParamDW( CAI_ROUTE_Y, 0 );

    // remove pointer to truck from trucks available list
    m_plTrucksAvailable->RemoveUnit( pTruck->GetID( ), FALSE );

    // record truck at the building that needs it
    paiTo->SetParamDW( iMat, pTruck->GetID( ) );
    paiTo->SetParam( iMat, EXCESS_IDLE_MATERIALS );

    // force building priority to 0 on any material
    // assignment, and if other materials are still
    // unassigned, they will be ided next time
    paiTo->SetParam( CAI_UNASSIGNED, 0 );

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
               "CAIRouter::IdleTruckTask() player %d transport truck %ld assigned to building %ld ", m_iPlayer,
               pTruck->GetID( ), paiTo->GetID( ) );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::m_plTrucksAvailable->GetCount() is %d ",
               m_plTrucksAvailable->GetCount( ) );
#endif

    // add to buildings need list
    if ( m_plBldgsNeed->GetUnit( paiTo->GetID( ) ) == NULL )
        m_plBldgsNeed->AddTail( (CObject*)paiTo );

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
               "CAIRouter::IdleTruckTask() Player %d Building %ld needs idle commodity %d ", m_iPlayer, paiTo->GetID( ),
               iMat );
#endif
}

//
// indicate if there is a need for more trucks
//
BOOL CAIRouter::NeedTransports( void )
{
    if ( !m_plTrucksAvailable->GetCount( ) )
        return TRUE;
    return FALSE;
}

//
// this building needs a truck for transporting materials,
// so find an available truck, then locate sources of the
// materials that are needed and route the truck to each
// source and pick up what is needed
//

BOOL CAIRouter::FindTransport( CAIUnit* pCAIBldg )
{

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::FindTransport during Routing() for player %d ", m_iPlayer );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "for building id %ld ", pCAIBldg->GetID( ) );
#endif

    // first consider that there are no more trucks available
    // so there is no point in continuing onward
    if ( !m_plTrucksAvailable->GetCount( ) )
    {
#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::%d trucks available for player %d building id %ld ",
                   m_plTrucksAvailable->GetCount( ), m_iPlayer, pCAIBldg->GetID( ) );
#endif
        // put back in list
        return ( FALSE );
    }
    // consider that trucks enroute are meeting the
    // the material needs of the building
    if ( TrucksAreEnroute( pCAIBldg ) )
    {

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::truck enroute to player %d building id %ld ", m_iPlayer,
                   pCAIBldg->GetID( ) );
#endif

        // leave out of list
        return ( TRUE );
    }

    // get access to CBuilding copied data for building
    CAICopy* pCopyCBuilding = pCAIBldg->GetCopyData( CAICopy::CBuilding );
    if ( pCopyCBuilding == NULL )
        return ( FALSE );  // put back in list

    // find the nearest truck from the list, with preference
    // given for trucks containing needed materials, then empty
    // trucks then finally any truck, and return it to here
    CAIUnit* pTruck = GetNearestTruck( pCAIBldg );
    if ( pTruck == NULL )
    {

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                   "CAIRouter::FindTransport() no truck available, count=%d for player %d building id %ld ",
                   m_plTrucksAvailable->GetCount( ), m_iPlayer, pCAIBldg->GetID( ) );
#endif
        // put back in list
        return ( FALSE );
    }

    // and assign it to this building
    WORD wStatus = 0;
    wStatus |= CAI_IN_USE;
    pTruck->SetStatus( wStatus );             // truck is now assigned
    pTruck->SetDataDW( pCAIBldg->GetID( ) );  // id of building needing
    pTruck->ClearParam( );

    // get truck's location to help find source
    CHexCoord hexTruck;
    EnterCriticalSection( &cs );
    CVehicle* pVehicle = theVehicleMap.GetVehicle( pTruck->GetID( ) );
    if ( pVehicle != NULL )
        hexTruck = pVehicle->GetHexHead( );
    LeaveCriticalSection( &cs );


    BOOL bAssigned    = FALSE;
    BOOL bSourceFound = TRUE;
    int  iBestDist    = 0xFFFF, iDist;
    int  iFirstDest   = CMaterialTypes::num_types;
    int  iCapacity    = pGameData->GetMaterialCapacity( pTruck );
    int  iQtyToGet    = 0;
    int  iQtyNeeded   = 0;

    // for each commodity possibly needed by this building
    // count how many items are needed by this building
    int iItems      = 0;
    int iMinLoadQty = iCapacity;
    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
        // this commodity is needed
        if ( pCAIBldg->GetParam( i ) )
            iItems++;
    }
    if ( iItems )
    {
        // if only one item then cut capacity so there is
        // some room left on this truck
        // if( iItems == 1 )
        //	iItems++;
        iMinLoadQty = iCapacity / iItems;
    }

    // for each commodity possibly needed by this building
    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
        // this commodity is needed
        if ( pCAIBldg->GetParam( i ) )
        {
            iQtyNeeded += pCAIBldg->GetParam( i );

#if THREADS_ENABLED
            myYieldThread( );
#endif

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "player %d building %ld needs %d of material %d ", m_iPlayer,
                       pCAIBldg->GetID( ), pCAIBldg->GetParam( i ), i );
#endif
            // get pointer to the building nearest to this
            // building with the material (and does not
            // also need it) and return it
            CAIUnit* pBldgSource = GetNearestSource( i, pCAIBldg->GetParam( i ), &iDist, hexTruck.X( ), hexTruck.Y( ) );

            // if there is no source for this material
            // there may be a building under construction
            // that will be
            if ( pBldgSource == NULL )
            {
#ifdef _LOGOUT
                logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "unable to find source for material %d ", i );
#endif
                bSourceFound = FALSE;
                continue;
            }
            // find the closest source building to be 1st dest
            if ( iDist && iDist < iBestDist )
            {
                iBestDist  = iDist;
                iFirstDest = i;
            }

            // set flag that the truck will get at least
            // one of the needed materials
            bAssigned = TRUE;

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "source for material %d found in player %d building %ld ", i,
                       m_iPlayer, pBldgSource->GetID( ) );
#endif
            // update the truck's WORD params with quantity
            // needed, at the offset of the material needed
            iQtyToGet = max( iMinLoadQty, pCAIBldg->GetParam( i ) );
            pTruck->SetParam( i, iQtyToGet );

            // update the truck with the dwID of the building
            // that is a source for the material again using
            // offset of the material needed
            pTruck->SetParamDW( i, pBldgSource->GetID( ) );

            // and record truck at the source building
            // pBldgSource->SetParamDW( i, pTruck->GetID() );
            // and at the building that needs it
            pCAIBldg->SetParamDW( i, pTruck->GetID( ) );

            // force building priority to 0 on any material
            // assignment, and if other materials are still
            // unassigned, they will be ided next time
            pCAIBldg->SetParam( CAI_UNASSIGNED, 0 );

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                       "CAIRouter::player %d transport truck %ld assigned to building %ld ", m_iPlayer,
                       pTruck->GetID( ), pCAIBldg->GetID( ) );
#endif
        }
    }


    // no sources were available so unassign the truck
    if ( !bAssigned || !iQtyNeeded )
    {
        pTruck->SetStatus( 0 );  // truck is no longer assigned
        pTruck->SetDataDW( 0 );

        // no longer needs anything
        if ( !iQtyNeeded )
        {
#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d building %ld - building needed nothing \n",
                       m_iPlayer, pCAIBldg->GetID( ) );
#endif
            return TRUE;  // take off of list
        }

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d building %ld - could not find any sources \n",
                   m_iPlayer, pCAIBldg->GetID( ) );
#endif

        // put back in list
        return FALSE;
    }

    // set the first destination for the truck
    if ( iFirstDest < CMaterialTypes::num_types )
    {
        CAIUnit* pDestBldg = m_plUnits->GetUnit( pTruck->GetParamDW( iFirstDest ) );
        if ( pDestBldg == NULL )
        {
            pTruck->SetStatus( 0 );  // truck is no longer assigned
            pTruck->SetDataDW( 0 );

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d truck %ld  source building not found \n",
                       m_iPlayer, pTruck->GetID( ), pDestBldg->GetID( ) );
#endif
            // put back in list
            return FALSE;
        }

        CHexCoord hex( 0, 0 );
        pGameData->GetBldgExit( pDestBldg->GetID( ), hex );

        if ( !hex.X( ) && !hex.Y( ) )
        {
            pTruck->SetStatus( 0 );  // truck is no longer assigned
            pTruck->SetDataDW( 0 );

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d truck %ld  source building %ld at %d,%d \n",
                       m_iPlayer, pTruck->GetID( ), pDestBldg->GetID( ), hex.X( ), hex.Y( ) );
#endif
            // put back in list
            return FALSE;
        }

        pTruck->SetDestination( hex );
        pTruck->SetParam( CAI_ROUTE_X, hex.X( ) );
        pTruck->SetParam( CAI_ROUTE_Y, hex.Y( ) );

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                   "CAIRouter::player %d transport truck %ld enroute to source building %ld \n", m_iPlayer,
                   pTruck->GetID( ), pDestBldg->GetID( ) );
#endif
    }
    else
    {
#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d truck %ld - no first source building found \n",
                   m_iPlayer, pTruck->GetID( ) );
#endif
        pTruck->SetStatus( 0 );  // truck is no longer assigned
        pTruck->SetDataDW( 0 );
        // put back in list
        return FALSE;
    }


    // remove pointer to truck from trucks available list
    m_plTrucksAvailable->RemoveUnit( pTruck->GetID( ), FALSE );

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d m_plTrucksAvailable->GetCount() is %d  \n", m_iPlayer,
               m_plTrucksAvailable->GetCount( ) );
#endif

    // leave out of list
    return TRUE;
}


//
// create a list and
// find any unassigned truck and add it to the list
// and return it
//
void CAIRouter::GetTrucksAvailable( void )
{
    ASSERT_VALID( m_plUnits );

    if ( m_plTrucksAvailable == NULL )
        return;

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
                continue;

            // these vehicles are assigned
            if ( ( pUnit->GetStatus( ) & CAI_IN_USE ) )
                continue;

            // these vehicles are in combat
            if ( ( pUnit->GetStatus( ) & CAI_IN_COMBAT ) )
                continue;

            // look only at vehicles that are TRUCKS
            if ( !pGameData->IsTruck( pUnit->GetID( ) ) )
                continue;

            if ( pUnit->GetTask( ) != IDT_SETTRANSPORT )
                continue;

            if ( m_plTrucksAvailable->GetUnit( pUnit->GetID( ) ) == NULL )
                m_plTrucksAvailable->AddTail( (CObject*)pUnit );
        }
    }
}


//
// find the truck nearest the building from the list
// carrying enough material to meet the need of the
// building, and return a pointer to it
//
CAIUnit* CAIRouter::GetNearestTruck( CAIUnit* pCAIBldg )
{
    // get access to CBuilding copied data for building
    CAICopy* pCopyCBuilding = pCAIBldg->GetCopyData( CAICopy::CBuilding );
    if ( pCopyCBuilding == NULL )
        return ( NULL );

    // pCopyCBuilding->m_aiDataOut[CAI_LOC_X]
    // pCopyCBuilding->m_aiDataOut[CAI_LOC_Y]
    // will be used as the destination
    CHexCoord hexBuilding( pCopyCBuilding->m_aiDataOut[CAI_LOC_X], pCopyCBuilding->m_aiDataOut[CAI_LOC_Y] );

    CAIUnit* pNearest = NULL;
    int      iNearest = pGameData->m_iHexPerBlk * pGameData->m_iBlkPerSide;

    // all units in the list should be trucks
    POSITION pos = m_plTrucksAvailable->GetHeadPosition( );
    while ( pos != NULL )
    {
        // if the truck has been killed the pointer in this list
        // will be invalid
        CAIUnit* pTruck = (CAIUnit*)m_plTrucksAvailable->GetNext( pos );
        if ( pTruck != NULL )
        {
            ASSERT_VALID( pTruck );
            if ( !IsValidUnit( pTruck ) )
                continue;

            // BUGBUG the truck may be dirty from a prior assignment
            // if this breakpoint fires, that means this truck is
            // unassigned, but still carries the id of prior building
            if ( pTruck->GetDataDW( ) )
                continue;

            // get access to CVehicle copied data for vehicle
            CAICopy* pCopyCVehicle = pTruck->GetCopyData( CAICopy::CVehicle );
            // something happened to the truck?
            if ( pCopyCVehicle == NULL )
                continue;

#if THREADS_ENABLED
            myYieldThread( );
#endif
            // get current location of truck
            CHexCoord hexVeh( pCopyCVehicle->m_aiDataOut[CAI_LOC_X], pCopyCVehicle->m_aiDataOut[CAI_LOC_Y] );

            // get dist to reach building
            int iDist = pGameData->GetRangeDistance( hexVeh, hexBuilding );

            // adjust distance for each material
            // needed that this truck is carrying
            for ( int m = 0; m < CMaterialTypes::num_types; ++m )
            {
                // consider if this material is needed
                if ( pCAIBldg->GetParam( m ) && iDist > 1 )
                {
                    // if truck carries more than needed
                    // 1/2 the distance from truck to building
                    if ( pCopyCVehicle->m_aiDataIn[m] >= pCAIBldg->GetParam( m ) )
                        iDist /= 2;
                    // else truck has some of what is needed
                    else if ( pCopyCVehicle->m_aiDataIn[m] )
                        iDist--;
                }
            }

            if ( iDist && iDist < iNearest )
            {
                pNearest = pTruck;
                iNearest = iDist;
            }
        }
    }

    // there could be trucks, that have just been assigned to IDT_TRANSPORT
    // task and they have not been added to m_plTrucksAvailable

    return ( pNearest );
}

//
// validate the pointer that is in the local list
// with the what it should be, as it may be dead
//
BOOL CAIRouter::IsValidUnit( CAIUnit* pUnit )
{
    if ( pUnit == NULL )
        return FALSE;
    CAIUnit* pValid = m_plUnits->GetUnit( pUnit->GetID( ) );
    if ( pUnit == pValid )
        return TRUE;
    return FALSE;
}

BOOL CAIRouter::IsValidUnit( DWORD dwID )
{
    CAIUnit* pValid = m_plUnits->GetUnit( dwID );
    if ( pValid != NULL )
        return TRUE;
    return FALSE;
}


//
// get CAIUnit pointer of the building nearest to this
// location with the material (and does not
// also need it) and this quantity and return
// the pointer
//
CAIUnit* CAIRouter::GetNearestSource( int iMaterial, int iQtyNeeded, int* piDistBack, int iX, int iY )
{
    ASSERT_VALID( m_plUnits );

    CAIUnit* pClosest     = NULL;
    CAIUnit* pNextClosest = NULL;
    int      iNextBest    = 0;
    int      iBestDist    = 0xFFFF;
    int      iOnHand      = 0;

    CHexCoord hexNeed( iX, iY );
    CHexCoord hexMat;

    POSITION pos = m_plUnits->GetHeadPosition( );
    while ( pos != NULL )
    {
#if THREADS_ENABLED
        // BUGBUG this function must yield
        myYieldThread( );
        // if( myYieldThread() == TM_QUIT )
        //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAIUnit* pUnit = (CAIUnit*)m_plUnits->GetNext( pos );
        if ( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

            if ( pUnit->GetOwner( ) != m_iPlayer )
                continue;

            // determine if this is a building
            if ( pUnit->GetType( ) != CUnit::building )
                continue;

            // get access to CBuilding copied data for building
            CAICopy* pCopyCBuilding = pUnit->GetCopyData( CAICopy::CBuilding );
            if ( pCopyCBuilding == NULL )
                continue;

            // this building needs that same material and has
            // less than it needs
            if ( pUnit->GetParam( iMaterial ) >= pCopyCBuilding->m_aiDataIn[iMaterial] )
                continue;

            // is this a building under construction
            if ( pCopyCBuilding->m_aiDataOut[CAI_ISCONSTRUCTING] )
                continue;

            // then based on what this building does
            // determine if it could provide the material
            if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTmaterials )
            {
                // get pointer to CBuildMaterials copy data
                CAICopy* pCopyCBuildMaterials = pUnit->GetCopyData( CAICopy::CBuildMaterials );
                if ( pCopyCBuildMaterials == NULL )
                    continue;

                // does this building need the same material for
                // doing its production, and have less than needed
                if ( pCopyCBuildMaterials->m_aiDataIn[iMaterial] )
                    continue;

                // does this building produce the material needed
                if ( pCopyCBuildMaterials->m_aiDataOut[iMaterial] )
                {
#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                               "CAIRouter::player %d building %ld produces %d of material %d ", m_iPlayer,
                               pUnit->GetID( ), pCopyCBuildMaterials->m_aiDataOut[iMaterial], iMaterial );
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::and has %d in stores ",
                               pCopyCBuilding->m_aiDataIn[iMaterial] );
#endif
                }

                // does this building have the material needed?
                if ( pCopyCBuilding->m_aiDataIn[iMaterial] )
                {
#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d building %ld has material %d ",
                               m_iPlayer, pUnit->GetID( ), iMaterial );
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::with %d in stores ",
                               pCopyCBuilding->m_aiDataIn[iMaterial] );
#endif
                    // if less than we want, then record this source
                    // as a 'next best' choice if no full source is found
                    if ( pCopyCBuilding->m_aiDataIn[iMaterial] < iQtyNeeded )
                    {
                        if ( pCopyCBuilding->m_aiDataIn[iMaterial] > iNextBest )
                        {
                            iNextBest    = pCopyCBuilding->m_aiDataIn[iMaterial];
                            pNextClosest = pUnit;
                        }
                        continue;
                    }
                }
                else
                    continue;
            }
            else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTmine )
            {
                // get pointer to CBuildMine copy data
                CAICopy* pCopyCBuildMine = pUnit->GetCopyData( CAICopy::CBuildMine );
                if ( pCopyCBuildMine == NULL )
                    continue;

                // new feature of abandoned, added too late to incorporate
                // into the CAICopy object so do a direct access
                BOOL bIsDepleted = FALSE;
                EnterCriticalSection( &cs );
                CBuilding* pBldg = theBuildingMap.GetBldg( pUnit->GetID( ) );
                if ( pBldg != NULL )
                    bIsDepleted = pBldg->IsFlag( CUnit::abandoned );
                LeaveCriticalSection( &cs );
                // is this an abandoned building and has none of the material needed
                if ( bIsDepleted && !pCopyCBuilding->m_aiDataIn[iMaterial] )
                {
#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d building %ld a %d mine is abandoned ",
                               m_iPlayer, pUnit->GetID( ), pUnit->GetTypeUnit( ) );
#endif
                    continue;
                }

                // if the building mines what we are looking for
                if ( pCopyCBuildMine->m_aiDataOut[CAI_TYPEBUILD] == iMaterial )
                {
#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d building %ld mines material %d ",
                               m_iPlayer, pUnit->GetID( ), iMaterial );
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::and has %d in stores ",
                               pCopyCBuilding->m_aiDataIn[iMaterial] );
#endif
                }

                // does this building have the material needed?
                if ( pCopyCBuilding->m_aiDataIn[iMaterial] )
                {
                    // if less than we want, then record this source
                    // as a 'next best' choice if no full source is found
                    if ( pCopyCBuilding->m_aiDataIn[iMaterial] < iQtyNeeded )
                    {
                        if ( pCopyCBuilding->m_aiDataIn[iMaterial] > iNextBest )
                        {
                            iNextBest    = pCopyCBuilding->m_aiDataIn[iMaterial];
                            pNextClosest = pUnit;
                        }
                        continue;
                    }
                }
                else
                    continue;
            }
            // farms and lumber
            else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTfarm )
            {
                // get pointer to CBuildFarm copy data
                CAICopy* pCopyCBuildFarm = pUnit->GetCopyData( CAICopy::CBuildFarm );
                if ( pCopyCBuildFarm == NULL )
                    continue;

                // if the building farms what we are looking for
                if ( pCopyCBuildFarm->m_aiDataOut[CAI_TYPEBUILD] == iMaterial )
                {
#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d building %ld farms material %d ",
                               m_iPlayer, pUnit->GetID( ), iMaterial );
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::and has %d in stores ",
                               pCopyCBuilding->m_aiDataIn[iMaterial] );
#endif
                }

                // does this building have the material needed?
                if ( pCopyCBuilding->m_aiDataIn[iMaterial] )
                {
#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d building %ld has material %d ",
                               m_iPlayer, pUnit->GetID( ), iMaterial );
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::with %d in stores ",
                               pCopyCBuilding->m_aiDataIn[iMaterial] );
#endif
                    // if less than we want, then record this source
                    // as a 'next best' choice if no full source is found
                    if ( pCopyCBuilding->m_aiDataIn[iMaterial] < iQtyNeeded )
                    {
                        if ( pCopyCBuilding->m_aiDataIn[iMaterial] > iNextBest )
                        {
                            iNextBest    = pCopyCBuilding->m_aiDataIn[iMaterial];
                            pNextClosest = pUnit;
                        }
                        continue;
                    }
                }
                else
                    continue;
            }
            // all types of vehicles
            else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTvehicle )
            {
                // get pointer to CBuildUnit copy data
                CAICopy* pCopyCBuildUnit = pUnit->GetCopyData( CAICopy::CBuildUnit );
                if ( pCopyCBuildUnit == NULL )
                    continue;

                // does this building need the same material?
                if ( pCopyCBuildUnit->m_aiDataIn[iMaterial] )
                    continue;

                // does this building have any of the material?
                if ( pCopyCBuilding->m_aiDataIn[iMaterial] < iQtyNeeded )
                    continue;
            }
            else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTpower )
            {
                // get pointer to the CStructureData type for this unit
                CStructureData const* pBldgData =
                    pGameData->GetStructureData( pCopyCBuilding->m_aiDataOut[CAI_TYPEBUILD] );
                if ( pBldgData != NULL )
                {
                    // now get pointer to the CBuild* instance of building
                    CBuildPower* pPowerBldg = pBldgData->GetBldPower( );
                    if ( pPowerBldg == NULL )
                        continue;

                    // it uses this material for fuel for power
                    if ( pPowerBldg->GetInput( ) == iMaterial )
                        continue;
                }

                // does this building have the material needed?
                if ( pCopyCBuilding->m_aiDataIn[iMaterial] )
                {
#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d building %ld has material %d ",
                               m_iPlayer, pUnit->GetID( ), iMaterial );
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::with %d in stores ",
                               pCopyCBuilding->m_aiDataIn[iMaterial] );
#endif
                    // if less than we want, then record this source
                    // as a 'next best' choice if no full source is found
                    if ( pCopyCBuilding->m_aiDataIn[iMaterial] < iQtyNeeded )
                    {
                        if ( pCopyCBuilding->m_aiDataIn[iMaterial] > iNextBest )
                        {
                            iNextBest    = pCopyCBuilding->m_aiDataIn[iMaterial];
                            pNextClosest = pUnit;
                        }
                        continue;
                    }
                }
                else
                    continue;
            }
            else  // if building does nothing it still may have material
            {
                // does this building have the material needed?
                if ( pCopyCBuilding->m_aiDataIn[iMaterial] )
                {
#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d building %ld has material %d ",
                               m_iPlayer, pUnit->GetID( ), iMaterial );
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::with %d in stores ",
                               pCopyCBuilding->m_aiDataIn[iMaterial] );
#endif
                    // if less than we want, then record this source
                    // as a 'next best' choice if no full source is found
                    if ( pCopyCBuilding->m_aiDataIn[iMaterial] < iQtyNeeded )
                    {
                        if ( pCopyCBuilding->m_aiDataIn[iMaterial] > iNextBest )
                        {
                            iNextBest    = pCopyCBuilding->m_aiDataIn[iMaterial];
                            pNextClosest = pUnit;
                        }
                        continue;
                    }
                }
                else
                    continue;
            }

            // this building has the material and does not use it

            // convert building location to CHexCorrd
            hexMat.X( pCopyCBuilding->m_aiDataOut[CAI_LOC_X] );
            hexMat.Y( pCopyCBuilding->m_aiDataOut[CAI_LOC_Y] );

            int iDist = theMap.GetRangeDistance( hexNeed, hexMat );
            if ( iDist && iDist < iBestDist )
            {
                iBestDist = iDist;
                pClosest  = pUnit;
            }
        }
    }

    // either the closest or the next closest pointer
    if ( pClosest == NULL && pNextClosest != NULL )
    {
        // get access to CBuilding copied data for building
        CAICopy* pCopyCBuilding = pNextClosest->GetCopyData( CAICopy::CBuilding );
        if ( pCopyCBuilding != NULL )
        {
            // convert building location to CHexCorrd
            hexMat.X( pCopyCBuilding->m_aiDataOut[CAI_LOC_X] );
            hexMat.Y( pCopyCBuilding->m_aiDataOut[CAI_LOC_Y] );
            iBestDist = theMap.GetRangeDistance( hexNeed, hexMat );
        }
        *piDistBack = iBestDist;
        return ( pNextClosest );
    }
    else if ( pClosest == NULL && pNextClosest == NULL )
    {
        // this compensates for the HP reserve material change
        pClosest = m_plUnits->GetUnit( m_dwRocket );
        if ( pClosest == NULL )
            return ( NULL );
        CAICopy* pCopyCBuilding = pClosest->GetCopyData( CAICopy::CBuilding );
        if ( pCopyCBuilding != NULL )
        {
            // convert building location to CHexCorrd
            hexMat.X( pCopyCBuilding->m_aiDataOut[CAI_LOC_X] );
            hexMat.Y( pCopyCBuilding->m_aiDataOut[CAI_LOC_Y] );
            iBestDist = theMap.GetRangeDistance( hexNeed, hexMat );
        }

        // put needed materials in rocket, which is selected as a source
        EnterCriticalSection( &cs );
        CBuilding* pBuilding = theBuildingMap.GetBldg( m_dwRocket );
        if ( pBuilding != NULL )
            pBuilding->AddToStore( iMaterial, iQtyNeeded );
        LeaveCriticalSection( &cs );
    }

    // return the best distance
    *piDistBack = iBestDist;

    return ( pClosest );
}
//
// check all trucks that are assigned to this building
//
BOOL CAIRouter::TrucksAreEnroute( CAIUnit* pBldg )
{
    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
        // is there an id present, if so its a truck
        // and are those materials still needed
        if ( pBldg->GetParamDW( i ) && pBldg->GetParam( i ) )
        {
            CAIUnit* pTruck = m_plUnits->GetUnit( pBldg->GetParamDW( i ) );
            if ( pTruck != NULL )
            {
                // if truck is assigned
                if ( pTruck->GetDataDW( ) == pBldg->GetID( ) )
                    return TRUE;
            }
            else
                pBldg->SetParamDW( i, 0 );  // truck is gone
        }
    }
    return FALSE;
}
//
// for each building need commodities, the truck passed has
// be destroyed, so the building to which this truck was
// assigned, needs to be toggled that it does not have
// this truck anymore
//
void CAIRouter::UnassignTrucks( DWORD dwTruckID )
{
    // get truck
    CAIUnit* pTruck = m_plUnits->GetUnit( dwTruckID );
    if ( pTruck == NULL )
        return;

    POSITION pos = m_plUnits->GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pBldg = (CAIUnit*)m_plUnits->GetNext( pos );
        if ( pBldg != NULL )
        {
            ASSERT_VALID( pBldg );

            // make sure it still exists
            if ( !IsValidUnit( pBldg ) )
                continue;

            // is this the building the truck is assigned to?
            if ( pTruck->GetDataDW( ) != pBldg->GetID( ) )
                continue;

            for ( int i = 0; i < CMaterialTypes::num_types; ++i )
            {
                // is there an id present, if so its a truck
                // and those materials still needed are in GetParam()
                // but the truck is gone now
                if ( pBldg->GetParamDW( i ) == dwTruckID )
                    pBldg->SetParamDW( i, 0 );
            }

            // need to put the building back in the needs list

            // consider adding building to buildings need list
            if ( m_plBldgsNeed->GetUnit( pBldg->GetID( ) ) == NULL )
                m_plBldgsNeed->AddTail( (CObject*)pBldg );
            {
#ifdef _LOGOUT
                logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "Player %d Building %ld now needs a new truck ", m_iPlayer,
                           pBldg->GetID( ) );
#endif
            }
        }
    }
}

//
// for each truck assigned to the building, unassign
// it and add it to the m_plTrucksAvailable
//
void CAIRouter::UnassignTrucks( CAIUnit* pBldg )
{
    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
        // is there an id present, if so its a truck
        // and are those materials still needed
        if ( pBldg->GetParamDW( i ) )
        {
            CAIUnit* pTruck = m_plUnits->GetUnit( pBldg->GetParamDW( i ) );
            if ( pTruck != NULL )
            {
                if ( m_plTrucksAvailable->GetUnit( pTruck->GetID( ) ) == NULL )
                {
                    // now unassign the truck
                    pTruck->SetDataDW( 0 );
                    pTruck->ClearParam( );
                    pTruck->SetStatus( 0 );
                    // and add it to the list
                    m_plTrucksAvailable->AddTail( (CObject*)pTruck );

#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                               "CAIRouter::player %d transport truck %ld unassigned and available ",
                               pTruck->GetOwner( ), pTruck->GetID( ) );
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::m_plTrucksAvailable->GetCount() is %d  \n",
                               m_plTrucksAvailable->GetCount( ) );
#endif
                }
            }
        }
    }
}

//
// this truck is no longer in use, so make it available
//
void CAIRouter::UnassignTruck( DWORD dwTruckID )
{
    // get pointer to the truck unit
    CAIUnit* pTruck = m_plUnits->GetUnit( dwTruckID );
    if ( pTruck != NULL )
    {
        // confirm it is not in the list, add if not in
        if ( m_plTrucksAvailable->GetUnit( pTruck->GetID( ) ) == NULL )
            m_plTrucksAvailable->AddTail( (CObject*)pTruck );

        // clear its assignment to the building
        pTruck->SetDataDW( (DWORD)0 );
        pTruck->ClearParam( );
        pTruck->SetStatus( 0 );

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d transport truck %ld unassigned and available ",
                   pTruck->GetOwner( ), pTruck->GetID( ) );
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::m_plTrucksAvailable->GetCount() is %d  \n",
                   m_plTrucksAvailable->GetCount( ) );
#endif
    }
}

//
// determine what the commodities are, that are needed to
// construct or produce for this specific building and
// return TRUE if there is a shortfall considering both
// what is enroute in assigned trucks and what is on-hand
//
// CMaterialTypes::num_types
//
//	enum { lumber,			// lumber - goods must be 0-based
//			steel,
//			copper,
//			moly,
//			goods,
//
//			food,
//			oil,
//			gas,
//			coal,
//			iron,
//			num_types,
//			num_build_types = food };	// goods + 1
//
BOOL CAIRouter::NeedsCommodities( CAIUnit* pCAIBldg )
{
    // get access to CBuilding copied data for building
    CAICopy* pCopyCBuilding = pCAIBldg->GetCopyData( CAICopy::CBuilding );
    if ( pCopyCBuilding == NULL )
        return FALSE;

        // citizen building need nothing
        // if( pCAIBldg->GetTypeUnit() >= CStructureData::apartment_1_1 &&
        //	pCAIBldg->GetTypeUnit() <= CStructureData::office_3_2 )
        //	return FALSE;

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "\nCAIRouter::NeedsCommodities for player %d ", m_iPlayer );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "checking building %d id=%ld ", pCopyCBuilding->m_aiDataOut[CAI_TYPEBUILD],
               pCAIBldg->GetID( ) );
#endif

    // declare and init an array to hold materials needed
    int aiMatsNeeded[CMaterialTypes::num_types];
    for ( int i = 0; i < CMaterialTypes::num_types; ++i ) aiMatsNeeded[i] = 0;

    CStructureData const* pBldgData = NULL;

    // is this building in need of repairs
    if ( pCopyCBuilding->m_aiDataOut[CAI_DAMAGE] < DAMAGE_0 )
    {
        EnterCriticalSection( &cs );
        CBuilding* pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID( ) );
        if ( pBldg != NULL )
        {
            // go thru materials needed, and record needs
            for ( int i = 0; i < CMaterialTypes::num_build_types; ++i )
            {
                // repair while under construction
                if ( pCopyCBuilding->m_aiDataOut[CAI_ISCONSTRUCTING] )
                {
                    if ( pBldg->GetBldgMatReq( i, FALSE ) )
                        aiMatsNeeded[i] = pBldg->GetBldgMatReq( i, FALSE );
                }
                else  // repair after constructed
                {
                    if ( pBldg->GetBldgMatRepair( i ) )
                        aiMatsNeeded[i] = pBldg->GetBldgMatRepair( i );
                }
            }
        }
        LeaveCriticalSection( &cs );
    }
    // is this a building under construction
    else if ( pCopyCBuilding->m_aiDataOut[CAI_ISCONSTRUCTING] )
    {
        // get pointer to CStructureData copy data
        CAICopy* pCopyCStructureData = pCAIBldg->GetCopyData( CAICopy::CStructureData );
        if ( pCopyCStructureData == NULL )
            return FALSE;

        // go thru materials needed, and record needs
        for ( int i = 0; i < CMaterialTypes::num_types; ++i )
        {
            // update materials needed to produce materials
            if ( pCopyCStructureData->m_aiDataIn[i] )
                aiMatsNeeded[i] = pCopyCStructureData->m_aiDataIn[i];
        }
    }
    // building is built and running
    else if ( !pCopyCBuilding->m_aiDataOut[CAI_ISPAUSED] )
    {
        // does this building produce materials?
        if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTmaterials )
        {
            // get pointer to CBuildMaterials copy data
            CAICopy* pCopyCBuildMaterials = pCAIBldg->GetCopyData( CAICopy::CBuildMaterials );
            if ( pCopyCBuildMaterials == NULL )
                return FALSE;

            // get pointer to the CStructureData type for this unit
            pBldgData = pGameData->GetStructureData( pCAIBldg->GetTypeUnit( ) );
            if ( pBldgData == NULL )
                return FALSE;

            CBuildMaterials const* pBldgMats = pBldgData->GetBldMaterials( );
            if ( pBldgMats == NULL )
                return FALSE;

            int iNumMinutes = 5;
            if ( pCAIBldg->GetTypeUnit( ) == CStructureData::refinery )
                iNumMinutes = 150;

            // go thru materials needed, and record needs
            for ( int i = 0; i < CMaterialTypes::num_types; ++i )
            {
                // update materials needed to produce materials
                if ( pCopyCBuildMaterials->m_aiDataIn[i] )
                {
                    // get 5 minutes worth
                    aiMatsNeeded[i] = ( pCopyCBuildMaterials->m_aiDataIn[i] * iNumMinutes );
                    // aiMatsNeeded[i] =
                    //	(pCopyCBuildMaterials->m_aiDataIn[i] *
                    //	pBldgMats->GetTime());

#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                               "building type %d for player %d id=%ld needs %d/%d of material %d ",
                               pCAIBldg->GetTypeUnit( ), m_iPlayer, pCAIBldg->GetID( ),
                               pCopyCBuildMaterials->m_aiDataIn[i], aiMatsNeeded[i], i );
#endif
                }
            }
        }
        // or this building produces vehicles?
        else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTvehicle )
        {
            // get pointer to CBuildUnit copy data
            CAICopy* pCopyCBuildUnit = pCAIBldg->GetCopyData( CAICopy::CBuildUnit );
            if ( pCopyCBuildUnit == NULL )
            {

#ifdef _LOGOUT
                logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                           "building type %d for player %d id=%ld has no vehicle in production ",
                           pCAIBldg->GetTypeUnit( ), m_iPlayer, pCAIBldg->GetID( ) );
#endif
                return FALSE;
            }

            // go thru materials needed, and record needs
            for ( int i = 0; i < CMaterialTypes::num_types; ++i )
            {
                // update materials needed for vehicle
                if ( pCopyCBuildUnit->m_aiDataIn[i] )
                {
                    aiMatsNeeded[i] = ( pCopyCBuildUnit->m_aiDataIn[i] * 2 );

#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                               "vehicle building type %d for player %d id=%ld needs %d of material %d ",
                               pCAIBldg->GetTypeUnit( ), m_iPlayer, pCAIBldg->GetID( ), pCopyCBuildUnit->m_aiDataIn[i],
                               i );
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "has %d in stores right now ",
                               pCopyCBuilding->m_aiDataIn[i] );
#endif
                }
            }
        }
        // else is a power plant
        else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTpower )
        {
            // get pointer to the CStructureData type for this unit
            pBldgData = pGameData->GetStructureData( pCopyCBuilding->m_aiDataOut[CAI_TYPEBUILD] );
            if ( pBldgData != NULL )
            {
                // now get pointer to the CBuild* instance of building
                CBuildPower* pPowerBldg = pBldgData->GetBldPower( );
                if ( pPowerBldg == NULL )
                    return FALSE;

                // it uses some material for power
                if ( pPowerBldg->GetInput( ) >= 0 )
                {
                    // get id of material needed for power
                    int i = pPowerBldg->GetInput( );

                    // actually should try to get as much as the
                    // truck assigned can carry, but we don't know
                    // which truck will be assigned!

                    // try get enough so bldg keep's an hour's worth
                    int iPowerFuel = (int)( ( 86400L / (LONG)pPowerBldg->GetRate( ) ) / 24 );
                    if ( iPowerFuel > pCopyCBuilding->m_aiDataIn[i] )
                        aiMatsNeeded[i] = iPowerFuel;
                    else
                        aiMatsNeeded[i] = 0;
                }
            }
        }
        // else is a mine, which don't need anything
        // else if( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] ==
        //	CStructureData::UTmine )
        //{
        //}
        else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTshipyard )
        {
            // get CBuilding data
            EnterCriticalSection( &cs );
            CBuilding* pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID( ) );
            if ( pBldg != NULL )
            {
                CShipyardBuilding* pShipBldg = (CShipyardBuilding*)pBldg;

                // is there a vehicle in getting a repair?
                CVehicle* pVehRepairing = pShipBldg->GetVehRepairing( );
                if ( pVehRepairing != NULL )
                {
                    // get the repair data for that vehicle
                    CBuildShipyard const* pBuildShipyard = (CBuildShipyard const*)pShipBldg->GetData( );
                    CBuildUnit const*     pBuildVeh = pBuildShipyard->GetUnit( pVehRepairing->GetData( )->GetType( ) );
                    if ( pBuildVeh != NULL )
                    {
                        for ( int i = 0; i < CMaterialTypes::num_build_types; ++i )
                            aiMatsNeeded[i] = pBuildVeh->GetInput( i );
                    }
                }
                else  // is a vehicle being constructed?
                {
                    CBuildUnit const* pBuildVeh = pShipBldg->GetBldUnt( );
                    if ( pBuildVeh != NULL )
                    {
                        for ( int i = 0; i < CMaterialTypes::num_build_types; ++i )
                            aiMatsNeeded[i] = pBuildVeh->GetInput( i );
                    }
                }
            }
            LeaveCriticalSection( &cs );

#ifdef _LOGOUT
            // BUGBUG this for loop just reports
            for ( int i = 0; i < CMaterialTypes::num_types; ++i )
            {
                // update materials needed for shipyard
                if ( aiMatsNeeded[i] )
                {
                    logPrintf(
                        LOG_PRI_ALWAYS, LOG_AI_MISC, "shipyard type %d for player %d id=%ld needs %d of material %d ",
                        pCAIBldg->GetTypeUnit( ), pCAIBldg->GetOwner( ), pCAIBldg->GetID( ), aiMatsNeeded[i], i );
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "has %d in stores right now ",
                               pCopyCBuilding->m_aiDataIn[i] );
                }
            }
#endif
        }
        else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTrepair )
        {
            // get pointer to CBuildUnit copy data
            CAICopy* pCopyCBuildUnit = pCAIBldg->GetCopyData( CAICopy::CBuildUnit );
            if ( pCopyCBuildUnit == NULL )
            {

#ifdef _LOGOUT
                logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                           "building type %d for player %d id=%ld has no vehicle in for repair ",
                           pCAIBldg->GetTypeUnit( ), pCAIBldg->GetOwner( ), pCAIBldg->GetID( ) );
#endif
                return FALSE;
            }

            // a vehicle is being repaired so
            // go thru materials needed, and record needs
            for ( int i = 0; i < CMaterialTypes::num_types; ++i )
            {
                // update materials needed for vehicle
                if ( pCopyCBuildUnit->m_aiDataIn[i] )
                {
                    aiMatsNeeded[i] = pCopyCBuildUnit->m_aiDataIn[i];

#ifdef _LOGOUT
                    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                               "repair building type %d for player %d id=%ld needs %d of material %d ",
                               pCAIBldg->GetTypeUnit( ), m_iPlayer, pCAIBldg->GetID( ), pCopyCBuildUnit->m_aiDataIn[i],
                               i );
#endif
                }
            }
        }
        else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UThousing )
        {
        }
        else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTmine )
        {
        }
        else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTfarm )
        {
        }
        else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTresearch )
        {
        }
        else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTcommand )
        {
        }
        else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTembassy )
        {
        }
        else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTfort )
        {
        }
        else  // unknown production type?
            return ( FALSE );
    }

#if THREADS_ENABLED
    // BUGBUG this function must yield
    myYieldThread( );
#endif

    // now apply what is enroute to building via trucks
    // that contain the needed materials

    POSITION pos = m_plUnits->GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIUnit* pTruck = (CAIUnit*)m_plUnits->GetNext( pos );
        if ( pTruck != NULL )
        {
            ASSERT_VALID( pTruck );

            if ( pTruck->GetOwner( ) != m_iPlayer )
                continue;

            // determine if this is a building
            if ( pTruck->GetType( ) == CUnit::building )
                continue;

            // is truck assigned to this building
            if ( ( pTruck->GetStatus( ) & CAI_IN_USE ) && ( pTruck->GetDataDW( ) == pCAIBldg->GetID( ) ) )
            {
#if THREADS_ENABLED
                // BUGBUG this function must yield
                myYieldThread( );
                // if( myYieldThread() == TM_QUIT )
                //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
                // get access to CVehicle copied data for truck
                CAICopy* pCopyCVehicle = pTruck->GetCopyData( CAICopy::CVehicle );
                if ( pCopyCVehicle == NULL )
                    continue;

                // consider quantity of material carried
                // for if it helps out on the needs of building
                for ( int i = 0; i < CMaterialTypes::num_types; ++i )
                {
                    // a needed material of the building
                    if ( aiMatsNeeded[i] )
                    {
                        // apply what the enroute truck is carrying
                        if ( aiMatsNeeded[i] > pCopyCVehicle->m_aiDataIn[i] )
                            aiMatsNeeded[i] -= pCopyCVehicle->m_aiDataIn[i];  // NOTE: changed - to -=, assuming
                                                                              // previous state was a typo
                        else
                            aiMatsNeeded[i] = 0;
                    }
                }
#if 0
				// consider that building may still be in need
				// of material, that is available at a building
				// to which the assigned truck is in enroute
				for( i=0; i<CMaterialTypes::num_types; ++i )
				{
					// a needed material of the building
					if( aiMatsNeeded[i] )
					{
						// truck has a destination for that material
						if( pTruck->GetParamDW(i) )
							aiMatsNeeded[i] = 0;
					}
				}
#endif
            }
        }
    }


    BOOL bNeedSomething = FALSE;
    // now apply what is on-hand at the building against needs
    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
        // update needed commoditiy array
        // with id offset of commodity, needed amount, on-hand amount
        SetNeeded( i, aiMatsNeeded, pCopyCBuilding->m_aiDataIn[i] );

        // if a material is still needed
        if ( aiMatsNeeded[i] )
            bNeedSomething = TRUE;
    }

    // only change the buildings params if something is needed
    if ( bNeedSomething )
    {
        for ( int i = 0; i < CMaterialTypes::num_types; ++i )
        {
            pCAIBldg->SetParam( i, aiMatsNeeded[i] );
        }
        // clear the last param offset to use for priority
        pCAIBldg->SetParam( CAI_UNASSIGNED, 0 );
    }

    return ( bNeedSomething );
}
//
// update the pwaParams array of the unit with the
// amount needed - amount on hand or zero if no need
//
void CAIRouter::SetNeeded( int iMat, int iaNeeded[], int iOnHand )
{
    int iAmt = 0;
    if ( iaNeeded[iMat] > iOnHand )
        iAmt = iaNeeded[iMat] - iOnHand;
    iaNeeded[iMat] = iAmt;
}

// BUGBUG this function does not handle all priority
//
// determine the priority for the building passed
// based on what the building does
//
void CAIRouter::SetUnitPriority( CAIUnit* pCAIBldg )
{
    ASSERT_VALID( this );
    ASSERT_VALID( pCAIBldg );

    // get access to CBuilding copied data for building
    CAICopy* pCopyCBuilding = pCAIBldg->GetCopyData( CAICopy::CBuilding );
    if ( pCopyCBuilding == NULL )
        return;

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::SetUnitPriority for player %d building %ld priority %d ",
               m_iPlayer, pCAIBldg->GetID( ), pCAIBldg->GetParam( CAI_UNASSIGNED ) );
#endif

    // consider the needs vs assignment
    BOOL bTruckStillNeeded = FALSE;

    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        // if material is needed
        if ( pCAIBldg->GetParam( i ) )
        {
            // is a truck assigned to transport it?
            if ( !pCAIBldg->GetParamDW( i ) )
                bTruckStillNeeded = TRUE;
        }
    }

    // if the building currently has no priority and
    // yet has a truck assigned for a needed material
    // then let its priority stay as zero
    if ( !bTruckStillNeeded )
        return;

    // make sure there is no priority
    WORD wPriority = 0;
    pCAIBldg->SetParam( CAI_UNASSIGNED, wPriority );

    // if building a building under construction	  add 30
    if ( pCopyCBuilding->m_aiDataOut[CAI_ISCONSTRUCTING] )
    {
        wPriority += 30;

        //   if building::event is set					  add 20
        if ( pCopyCBuilding->m_aiDataOut[CAI_ISWAITING] )
            wPriority += 20;

        //   if war and constructing new building         add 15
    }
    //   if no war and vehicle or new building        add 10

    // damaged in need of repair
    if ( pCopyCBuilding->m_aiDataOut[CAI_DAMAGE] < DAMAGE_0 )
        wPriority += 20;

    if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTmaterials )
    {
        // get access to CBuildMaterials copied data for building
        CAICopy* pCopyCBuildMaterials = pCAIBldg->GetCopyData( CAICopy::CBuildMaterials );
        if ( pCopyCBuildMaterials == NULL )
            return;

        // BUGBUG goods are irrelevent now
        // if pBldg is producing a composite material add 50
        if ( pCopyCBuildMaterials->m_aiDataOut[CAI_COAL] || pCopyCBuildMaterials->m_aiDataOut[CAI_STEEL] ||
             pCopyCBuildMaterials->m_aiDataOut[CAI_IRON] )
            wPriority += 50;
        // else if pBldg is producing a material          add 40
        else
            wPriority += 30;

        // consider that we are out of gas
        if ( pCAIBldg->GetTypeUnit( ) == CStructureData::refinery )
        {
            if ( m_bNeedGas )
                wPriority += 30;
        }
    }
    else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTvehicle )
    {
        // get pointer to CBuildUnit copy data
        CAICopy* pCopyCBuildUnit = pCAIBldg->GetCopyData( CAICopy::CBuildUnit );
        if ( pCopyCBuildUnit == NULL )
        {
            wPriority += 10;
        }
        else
        {
            // if pBldg is producing TRUCK                    add 20
            // BUGBUG is this the correct way to
            // determine that a vehicle is a truck
            if ( pCopyCBuildUnit->m_aiDataIn[CAI_TYPEVEHICLE] == CTransportData::med_truck ||
                 pCopyCBuildUnit->m_aiDataIn[CAI_TYPEVEHICLE] == CTransportData::heavy_truck )
            {
                // initially add for no trucks
                if ( m_plTrucksAvailable->GetCount( ) )
                    wPriority += 20;
                else
                    wPriority += 40;

                if ( m_plTrucksAvailable->GetCount( ) < MINIMUM_IDLE_TRUCKS_AI )
                    wPriority += 40;

                // and add even more for no trucks if several
                // buildings are waiting for materials
                if ( !m_plTrucksAvailable->GetCount( ) && m_plBldgsNeed->GetCount( ) > 3 )
                    wPriority += 30;
            }
            // if no war and vehicle or new building          add 10
            else
                wPriority += 30;
        }
        // BUGBUG need to apply these influences
        // if total TRUCKS > factorys                   sub 10
        // if total TRUCKS > factorys+commodities used  sub 10
        // if war and combat vehicle                      add 20
    }
    // repair,
    else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTrepair )
    {
        wPriority += 10;
        // get pointer to CBuildUnit copy data
        CAICopy* pCopyCBuildUnit = pCAIBldg->GetCopyData( CAICopy::CBuildUnit );
        if ( pCopyCBuildUnit != NULL )
            wPriority += 20;
    }
    else if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTshipyard )
    {
        wPriority += 10;
        // get CBuilding data
        EnterCriticalSection( &cs );
        CBuilding* pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID( ) );
        if ( pBldg != NULL )
        {
            CShipyardBuilding* pShipBldg = (CShipyardBuilding*)pBldg;

            // is there a ship in getting a repair?
            CVehicle* pVehRepairing = pShipBldg->GetVehRepairing( );
            if ( pVehRepairing != NULL )
                wPriority += 20;
            else  // a ship is being constructed
            {
                CBuildUnit const* pBuildVeh = pShipBldg->GetBldUnt( );
                if ( pBuildVeh != NULL )
                    wPriority += 30;
            }
        }
        LeaveCriticalSection( &cs );
    }
    else
    {
        // producing something other than building/vehicle
        // housing,			// place to live
        // command,			// command center
        // embassy,
        // police,				// police & fire
        // fort,
        // power,				// power plant
        if ( pCopyCBuilding->m_aiDataOut[CAI_PRODUCES] == CStructureData::UTpower )
            wPriority += 60;
        else
            wPriority += 10;

        // research,
        // warehouse,		// market or warehouse
    }

    pCAIBldg->SetParam( CAI_UNASSIGNED, wPriority );

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::SetUnitPriority for player %d building %ld new priority %d ",
               m_iPlayer, pCAIBldg->GetID( ), pCAIBldg->GetParam( CAI_UNASSIGNED ) );
#endif
}
//
// transfer the qtys needed of materials carried
// by the truck to the building
// and clear the truck of assignment
//
BOOL CAIRouter::UnloadMaterials( CAIUnit* pTruck, CAIUnit* pBldg )
{
    // get most current data for the truck
    CAICopy* pCopyCVehicle = pTruck->GetCopyData( CAICopy::CVehicle );
    if ( pCopyCVehicle == NULL )
        return ( FALSE );

    // update the building with its needs
    BOOL bStillNeeds = NeedsCommodities( pBldg );
    bStillNeeds      = FALSE;

    // game message
    CMsgTransMat msg;
    msg.m_dwIDSrc  = pTruck->GetID( );
    msg.m_dwIDDest = pBldg->GetID( );
    memset( msg.m_aiMat, 0, sizeof( msg.m_aiMat ) );

    // now go through the materials needed
    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
        // if the building needs this material and the vehicle has some
        if ( pCopyCVehicle->m_aiDataIn[i] || pBldg->GetParam( i ) )
        {
            // record this material quantity to be transferred
            msg.m_aiMat[i] = pCopyCVehicle->m_aiDataIn[i];

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                       "CAIRouter::player %d transport truck %ld delivered %d of %d at building %ld ", m_iPlayer,
                       pTruck->GetID( ), msg.m_aiMat[i], i, pBldg->GetID( ) );
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "which showed needing %d of %d ", pBldg->GetParam( i ), i );
#endif

            // update the buildings record of need considering that
            // more of the desired material may have been delivered
            // than what was needed by the building
            if ( msg.m_aiMat[i] > pBldg->GetParam( i ) )
                pBldg->SetParam( i, 0 );
            else
                pBldg->SetParam( i, ( pBldg->GetParam( i ) - msg.m_aiMat[i] ) );

            // and assigned truck id
            pBldg->SetParamDW( i, 0 );
        }
        else  // truck has none of this material
        {
            // but the building still needs some
            // and that was the truck to deliver it
            // so make sure the building unassigns
            // that truck from that material for
            // use by SetUnitPriority() later
            if ( pBldg->GetParam( i ) > 0 && pBldg->GetParamDW( i ) == pTruck->GetID( ) )
                pBldg->SetParamDW( i, 0 );
        }
        // consider if building still needs something
        if ( pBldg->GetParam( i ) > 0 )
            bStillNeeds = TRUE;
    }
    // send message to the game to cause the transfer
    theGame.PostToServer( (CNetCmd*)&msg, sizeof( CMsgTransMat ) );

    // now unassign the truck
    pTruck->SetDataDW( 0 );
    pTruck->ClearParam( );
    pTruck->SetStatus( 0 );
    // and add back to the available truck list
    if ( m_plTrucksAvailable->GetUnit( pTruck->GetID( ) ) == NULL )
        m_plTrucksAvailable->AddTail( (CObject*)pTruck );

#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d transport truck %ld unassigned and available ",
               m_iPlayer, pTruck->GetID( ) );
    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::m_plTrucksAvailable->GetCount() is %d  \n",
               m_plTrucksAvailable->GetCount( ) );

    logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d building %ld needs %d \n", m_iPlayer, pBldg->GetID( ),
               (int)bStillNeeds );
#endif

    if ( bStillNeeds )
        SetUnitPriority( pBldg );
    else  // remove building from the building needs list
        m_plBldgsNeed->RemoveUnit( pBldg->GetID( ), FALSE );

    return ( bStillNeeds );
}

//
// consider all the non-zero DWORD params of the truck
// and if the unit of the hex matches one of them, then the truck
// has arrived at one of the sources for material
// needed and it needs to transfer the quantity
// (from WORD params) of the material from the source
// building to the truck  and then determine
// which of the remaining sources (DWORD params) is
// closest and set the destination of the truck to
// go to that source building.  if no more source
// buildings, the set the destination to the building
// needing the material (pUnit->GetDataDW())
//
void CAIRouter::LoadMaterials( CAIUnit* pTruck, CAIHex* paiHex )
{
    // get current capacity of the truck
    int      iCapacity    = pGameData->GetMaterialCapacity( pTruck );
    BOOL     bMoreSources = FALSE;
    CAIUnit* pBldg        = NULL;

    // game message
    CMsgTransMat msg;
    msg.m_dwIDSrc  = paiHex->m_dwUnitID;
    msg.m_dwIDDest = pTruck->GetID( );
    memset( msg.m_aiMat, 0, sizeof( msg.m_aiMat ) );

    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
#if THREADS_ENABLED
        // BUGBUG this function must yield
        myYieldThread( );
        // if( myYieldThread() == TM_QUIT )
        //	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        if ( !pTruck->GetParamDW( i ) )
            continue;

        // we only want materials to get from this building
        if ( pTruck->GetParamDW( i ) == paiHex->m_dwUnitID )
        {
            // find the building for this material, could be more
            // than one material at this building
            pBldg = m_plUnits->GetUnit( paiHex->m_dwUnitID );
            if ( pBldg == NULL )
                continue;

            // get access to CBuilding copied data for building
            CAICopy* pCopyCBuilding = pBldg->GetCopyData( CAICopy::CBuilding );
            if ( pCopyCBuilding == NULL )
                continue;

            // get the lesser of what's needed vs. what's carried
            int iQtyNeeded = min( pTruck->GetParam( i ), pCopyCBuilding->m_aiDataIn[i] );

            // record this material quantity to be transferred
            msg.m_aiMat[i] = min( iQtyNeeded, iCapacity );
            iCapacity -= msg.m_aiMat[i];

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                       "CAIRouter::player %d transport truck %ld loaded %d of %d from source building %ld ", m_iPlayer,
                       pTruck->GetID( ), msg.m_aiMat[i], i, pBldg->GetID( ) );
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                       "CAIRouter::truck wanted %d, source building had %d, capacity left %d ", pTruck->GetParam( i ),
                       pCopyCBuilding->m_aiDataIn[i], iCapacity );
#endif

            // clear the truck so that it can be directed to
            // the next source building destination
            pTruck->SetParamDW( i, 0 );
        }
        else
            bMoreSources = TRUE;
    }
    // send message to the game to cause the transfer to occur
    theGame.PostToServer( (CNetCmd*)&msg, sizeof( CMsgTransMat ) );


    // if all sources have been visited, then it is time to go
    // to the building with the needed materials
    if ( !bMoreSources )
    {
        pBldg = m_plUnits->GetUnit( pTruck->GetDataDW( ) );
        if ( pBldg != NULL )
        {
            CHexCoord hex( 0, 0 );
            pGameData->GetBldgExit( pBldg->GetID( ), hex );

            if ( !hex.X( ) && !hex.Y( ) )
            {
                pTruck->SetStatus( 0 );  // truck is no longer assigned
                pTruck->SetDataDW( 0 );
                return;
            }

            pTruck->SetDestination( hex );
            pTruck->SetParam( CAI_ROUTE_X, hex.X( ) );
            pTruck->SetParam( CAI_ROUTE_Y, hex.Y( ) );

            pTruck->SetParamDW( CAI_ROUTE_X, 0 );
            pTruck->SetParamDW( CAI_ROUTE_Y, 0 );

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                       "CAIRouter::player %d transport truck %ld enroute to assigned building %ld ", m_iPlayer,
                       pTruck->GetID( ), pBldg->GetID( ) );
#endif
        }
        return;
    }

    // now figure out the nearest remaining source for material
    // and send a message to direct the truck to it
    int       iBestDist = 0xFFFF;
    CAIUnit*  pBestBldg = NULL;
    CHexCoord hexTruck( paiHex->m_iX, paiHex->m_iY );
    CHexCoord hexBldg, hexBest;

    for ( int i = 0; i < CMaterialTypes::num_types; ++i )
    {
#if 0  // THREADS_ENABLED
       //  BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        if ( pTruck->GetParamDW( i ) )
        {
            // get CAIUnit for the next source building
            pBldg = m_plUnits->GetUnit( pTruck->GetParamDW( i ) );
            if ( pBldg == NULL )
            {
                pTruck->SetParamDW( i, 0 );
                continue;
            }

            // get access to CBuilding copied data for building
            CAICopy* pCopyCBuilding = pBldg->GetCopyData( CAICopy::CBuilding );
            if ( pCopyCBuilding == NULL )
                continue;

            hexBldg.X( pCopyCBuilding->m_aiDataOut[CAI_LOC_X] );
            hexBldg.Y( pCopyCBuilding->m_aiDataOut[CAI_LOC_Y] );

            // get dist to reach building
            int iDist = pGameData->GetRangeDistance( hexTruck, hexBldg );

            // find the one closest and save it
            if ( iDist && iDist < iBestDist )
            {
                iBestDist = iDist;
                pBestBldg = pBldg;
                hexBest   = hexBldg;
            }
        }
    }
    // engage
    if ( pBestBldg != NULL )
    {
        // run a path to the next destination to see if the truck
        // can actually get there
        if ( !m_pMap->m_pMapUtil->GetPathRating( hexTruck, hexBest, pTruck->GetTypeUnit( ) ) )
        {
#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                       "CAIRouter::player %d truck %ld can't reach next dest of building %ld at %d,%d from %d,%d ",
                       m_iPlayer, pTruck->GetID( ), pBestBldg->GetID( ), hexBest.X( ), hexBest.Y( ), hexTruck.X( ),
                       hexTruck.Y( ) );
#endif
            pTruck->SetParam( CAI_ROUTE_X, hexBest.X( ) );
            pTruck->SetParam( CAI_ROUTE_Y, hexBest.Y( ) );

            // find staging hex near the next destination
            m_pMap->GetStagingHex( hexBest, 2, 2, pTruck->GetTypeUnit( ), hexBest, FALSE );

            // if none found, then try for a staging hex near the rocket
            if ( hexBest.X( ) == pTruck->GetParam( CAI_ROUTE_X ) && hexBest.Y( ) == pTruck->GetParam( CAI_ROUTE_Y ) )
            {
                // if not then find a staging hex near rocket to go to
                pGameData->GetBldgExit( m_dwRocket, hexBest );
                m_pMap->GetStagingHex( hexBest, 2, 2, pTruck->GetTypeUnit( ), hexBest, FALSE );
            }
            pTruck->SetDestination( hexBest );

#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "CAIRouter::player %d truck %ld going to temp staging at %d,%d \n",
                       m_iPlayer, pTruck->GetID( ), hexBest.X( ), hexBest.Y( ) );
#endif
            return;
        }

        // truck can reach next source so get going
        pTruck->SetDestination( pBestBldg );
        pTruck->SetParam( CAI_ROUTE_X, hexBest.X( ) );
        pTruck->SetParam( CAI_ROUTE_Y, hexBest.Y( ) );

        pTruck->SetParamDW( CAI_ROUTE_X, 0 );
        pTruck->SetParamDW( CAI_ROUTE_Y, 0 );

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC,
                   "CAIRouter::player %d transport truck %ld enroute to source building %ld \n", m_iPlayer,
                   pTruck->GetID( ), pBestBldg->GetID( ) );
#endif
    }
}

//
// this will cause an out_mat message to be faked
//
BOOL CAIRouter::BuildingNeedsAlways( CAIUnit* pBldg )
{
    BOOL bNeedsAlways = FALSE;
    switch ( pBldg->GetTypeUnit( ) )
    {
    case CStructureData::power_1:
    case CStructureData::power_2:
    // case CStructureData::power_3:
    case CStructureData::refinery:
    case CStructureData::smelter:
        bNeedsAlways = TRUE;
    default:
        break;
    }
    return ( bNeedsAlways );
}

void CAIRouter::Save( CFile* pFile )
{
    try
    {
        pFile->Write( (const void*)&m_iPlayer, sizeof( int ) );
        pFile->Write( (const void*)&m_dwRocket, sizeof( DWORD ) );
    }
    catch ( CFileException* theException )
    {
        // how should write errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }

    // save the count and just the DWORD id of units in the
    // the available truck and building needs lists

    // first do the trucks
    int iCnt = m_plTrucksAvailable->GetCount( );
    try
    {
        pFile->Write( (const void*)&iCnt, sizeof( int ) );
    }
    catch ( CFileException* theException )
    {
        // how should write errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }
    if ( iCnt )
    {
        POSITION pos = m_plTrucksAvailable->GetHeadPosition( );
        while ( pos != NULL )
        {
            CAIUnit* pUnit = (CAIUnit*)m_plTrucksAvailable->GetNext( pos );
            if ( pUnit != NULL )
            {
                DWORD dwID = pUnit->GetID( );
                try
                {
                    pFile->Write( (const void*)&dwID, sizeof( DWORD ) );
                }
                catch ( CFileException* theException )
                {
                    // how should write errors be reported?
                    throw( ERR_CAI_BAD_FILE );
                }
            }
        }
    }

    // now do the buildings
    iCnt = m_plBldgsNeed->GetCount( );
    try
    {
        pFile->Write( (const void*)&iCnt, sizeof( int ) );
    }
    catch ( CFileException* theException )
    {
        // how should write errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }
    if ( iCnt )
    {
        POSITION pos = m_plBldgsNeed->GetHeadPosition( );
        while ( pos != NULL )
        {
            CAIUnit* pUnit = (CAIUnit*)m_plBldgsNeed->GetNext( pos );
            if ( pUnit != NULL )
            {
                DWORD dwID = pUnit->GetID( );
                try
                {
                    pFile->Write( (const void*)&dwID, sizeof( DWORD ) );
                }
                catch ( CFileException* theException )
                {
                    // how should write errors be reported?
                    throw( ERR_CAI_BAD_FILE );
                }
            }
        }
    }
}

void CAIRouter::Load( CFile* pFile, CAIUnitList* plUnits )
{
    // newly loaded units
    m_plUnits = plUnits;

    // first get the player id
    try
    {
        pFile->Read( (void*)&m_iPlayer, sizeof( int ) );
        pFile->Read( (void*)&m_dwRocket, sizeof( DWORD ) );
    }
    catch ( CException* anException )
    {
        // how should read errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }

    // make sure lists are empty and valid
    if ( m_plTrucksAvailable != NULL )
    {
        m_plTrucksAvailable->RemoveAll( );
    }
    else  // need to allocate for the list
    {
        try
        {
            m_plTrucksAvailable = new CAIUnitList( );
        }
        catch ( CException* e )
        {
            // how should memory errors be reported?
            throw( ERR_CAI_BAD_NEW );
        }
    }

    if ( m_plBldgsNeed != NULL )
    {
        m_plBldgsNeed->RemoveAll( );
    }
    else  // need to allocate for the list
    {
        try
        {
            m_plBldgsNeed = new CAIUnitList( );
        }
        catch ( CException* e )
        {
            // how should memory errors be reported?
            throw( ERR_CAI_BAD_NEW );
        }
    }

    // the available truck and building needs lists do not create
    // new copies of the CAIUnits that are contained within, but
    // instead rely on those existing instances in the CAIMgr's
    // m_plUnits to provide the actual pointer to the instance
    int   iCnt;
    DWORD dwID;
    // now get count of trucks
    try
    {
        pFile->Read( (void*)&iCnt, sizeof( int ) );
    }
    catch ( CException* anException )
    {
        // how should read errors be reported?
        throw( ERR_CAI_BAD_NEW );
    }
    // and read them one at a time
    for ( int i = 0; i < iCnt; ++i )
    {
        try
        {
            pFile->Read( (void*)&dwID, sizeof( DWORD ) );
        }
        catch ( CException* anException )
        {
            // how should read errors be reported?
            throw( ERR_CAI_BAD_NEW );
        }
        CAIUnit* pUnit = m_plUnits->GetUnit( dwID );
        if ( pUnit != NULL )
        {
            if ( m_plTrucksAvailable->GetUnit( pUnit->GetID( ) ) == NULL )
                m_plTrucksAvailable->AddTail( (CObject*)pUnit );
        }
    }

    // now get count of buildings in need
    try
    {
        pFile->Read( (void*)&iCnt, sizeof( int ) );
    }
    catch ( CException* anException )
    {
        // how should read errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }
    // and read them one at a time
    for ( int i = 0; i < iCnt; ++i )
    {
        try
        {
            pFile->Read( (void*)&dwID, sizeof( DWORD ) );
        }
        catch ( CException* anException )
        {
            // how should read errors be reported?
            throw( ERR_CAI_BAD_FILE );
        }
        CAIUnit* pUnit = m_plUnits->GetUnit( dwID );
        if ( pUnit != NULL )
            m_plBldgsNeed->AddTail( (CObject*)pUnit );
    }
}

// end of CAIRoute.cpp
