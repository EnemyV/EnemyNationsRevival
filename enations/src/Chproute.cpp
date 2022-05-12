////////////////////////////////////////////////////////////////////////////
//
//  CHPRoute.cpp : CHPRouter object implementation
//                 Divide and Conquer AI
//               
//  Last update:  03/15/97
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "lastplnt.h"

#include "CHPRoute.hpp"
#include "CPathMap.h"
#include "netcmd.h"
#include "event.h"
#include "logging.h"	// dave's logging system


#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"

#define new DEBUG_NEW

#define EXCESS_MATERIALS 3000
#define MINIMUM_IDLE_TRUCKS 2

#define USE_CIRCULAR_LIST	1

extern CRITICAL_SECTION cs;	// used by threads
extern CPathMap thePathMap;		// the map pathfinding object (no yield)
extern CPathMgr thePathMgr;		// the actual pathfinding object
extern CAIData *pGameData;		// pointer to game data interface

void CHPRouter::MsgNewBldg (CBuilding const * pBldg)
{
	if( this == NULL )
		return;

	CMsgPlaceBldg msg (pBldg->GetHex (), pBldg->GetDir (), pBldg->GetData()->GetType ());
	msg.m_dwIDBldg = pBldg->GetID ();
	msg.ToNew ();
	RouteMessage (&msg);
}

void CHPRouter::MsgGiveBldg (CBuilding const * pBldg)
{
	if( this == NULL )
		return;

	CMsgPlaceBldg msg (pBldg->GetHex (), pBldg->GetDir (), pBldg->GetData()->GetType ());
	msg.m_dwIDBldg = pBldg->GetID ();
	msg.m_iPlyrNum = pBldg->GetOwner()->GetPlyrNum();
	msg.ToNew ();

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d can share player %d building %ld a type %d \n", 
		m_iPlayer, msg.m_iPlyrNum, msg.m_dwIDBldg, msg.m_iType );
#endif

	RouteMessage (&msg);
}

void CHPRouter::MsgBuiltBldg (CBuilding const * pBldg)
{
	if( this == NULL )
		return;

	CMsgBldgStat msg (pBldg);
	msg.m_iFlags = CMsgBldgStat::built;
	RouteMessage (&msg);
}

void CHPRouter::MsgOutMat (CBuilding const * pBldg)
{
	if( this == NULL )
		return;

	CMsgBldgStat msg (pBldg);
	msg.m_iFlags = CMsgBldgStat::out_mat;
	RouteMessage (&msg);
}

void CHPRouter::MsgDeleteUnit (CUnit const * pUnit)
{
	if( this == NULL )
		return;

	CMsgDeleteUnit msg (pUnit,0);
	RouteMessage (&msg);
}

void CHPRouter::MsgTakeBldg (CBuilding const * pBldg)
{
	if( this == NULL )
		return;

	CMsgDeleteUnit msg (pBldg,0);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d building %ld taken away \n", 
		m_iPlayer, msg.m_dwID );
#endif

	RouteMessage (&msg);
}

void CHPRouter::MsgTakeVeh (CVehicle const * pVeh)
{
	if( this == NULL )
		return;

	CMsgDeleteUnit msg (pVeh,0);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d transport truck %ld taken control by HP \n", 
		m_iPlayer, msg.m_dwID );
#endif

	RouteMessage (&msg);
}

void CHPRouter::MsgVehNew (CVehicle const * pVeh, DWORD dwIDBldg)
{
	if( this == NULL )
		return;

	CMsgVehNew msg (pVeh, dwIDBldg);
	RouteMessage (&msg);
}

void CHPRouter::MsgGiveVeh (CVehicle const * pVeh)
{
	if( this == NULL )
		return;

	CMsgVehNew msg (pVeh);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d transport truck %ld control returned \n", 
		m_iPlayer, msg.m_dwID );
#endif

	RouteMessage (&msg);
}

void CHPRouter::MsgArrived (CVehicle const * pVeh)
{
	if( this == NULL )
		return;

	CMsgVehDest msg (pVeh);
	RouteMessage (&msg);
}

void CHPRouter::MsgErrGoto (CVehicle const * pVeh)
{
	if( this == NULL )
		return;
#if 0
	// should use an event
	CString sMsg;
	sMsg.LoadString ( IDS_HPR_BLOCKED );
	theApp.m_wndBar.SetStatusText ( 0, sMsg );
#endif
	CMsgVehGoto msg (pVeh);
	msg.ToErr (NULL);
	RouteMessage (&msg);
}


void CHPRouter::RouteMessage( CNetCmd const *pMsg )
{
	if( this == NULL )
		return;

	CAIMsg aiMsg( pMsg );
	DoRouting( &aiMsg );
}

// create new HP router object
//
CHPRouter::CHPRouter( int iPlayer )
{
	m_iPlayer = iPlayer;

	m_plUnits = NULL;
	m_plTrucksAvailable = NULL;
	m_plBldgsNeed = NULL;
	m_plShipsAvailable = NULL;

	m_dwRocket = 0;

	for( int i=0; i<CMaterialTypes::num_types; ++i )
	{
		m_iStore[i] = 0;
		m_iNeeds[i] = 0;
		m_iOutput[i] = 0;
		m_aiMatsNeeded[i] = 0;
	}
}

//
// reprocess the game units and create new lists of
// units, buildings in need and trucks available
//
BOOL CHPRouter::Init( void )
{
	ASSERT( TestEverything() );

	m_wSeaportCnt = 0;
	m_wLumberCnt = 0;
	m_wCoalCnt = 0;
	m_wIronCnt = 0;
	m_wSmelterCnt = 0;

	m_bLastLumber = FALSE;
	m_bLastCoal = FALSE;
	m_bLastIron = FALSE;
	m_bLastSmelter = FALSE;

	if( m_plUnits != NULL )
	{
		m_plUnits->RemoveAll();
		delete m_plUnits;
	}

	// create a list to contain all player's units
    try
    {
    	m_plUnits = new CAIUnitList();
    }
    catch( CException e )
    {
    	if( m_plUnits != NULL )
    	{
    		delete m_plUnits;
    		m_plUnits = NULL;
    	}
		throw(ERR_CAI_BAD_NEW);
    }

	CVehicle *pVehicle = NULL;
	CBuilding *pBldg = NULL;
	DWORD dwDumb;

	EnterCriticalSection (&cs);
	POSITION pos = theBuildingMap.GetStartPosition();
	while (pos != NULL)
	{
		theBuildingMap.GetNextAssoc( pos, dwDumb, pBldg );

		// consider only those buildings of this player
		if( pBldg->GetOwner()->GetPlyrNum() == m_iPlayer )
		{
			CAIUnit *pUnit = NULL;
			try
			{
				// CAIUnit( DWORD dwID, int iOwner );
				pUnit = new CAIUnit( 
					pBldg->GetID(), m_iPlayer, 
					CUnit::building, pBldg->GetData()->GetType() );
				ASSERT_VALID( pUnit );
				m_plUnits->AddTail( (CObject *)pUnit );

				// be sure this unit starts clear
				pUnit->SetDataDW(0);
				pUnit->ClearParam();
				pUnit->SetStatus(0);
			}
			catch( CException e )
			{
				m_plUnits->RemoveAll();
				delete m_plUnits;
				m_plUnits = NULL;
				LeaveCriticalSection (&cs);

				throw(ERR_CAI_BAD_NEW);
			}

			// count special units
			if( pUnit != NULL )
			{
			if( pUnit->GetTypeUnit() == CStructureData::seaport )
				m_wSeaportCnt++;
			if( pUnit->GetTypeUnit() == CStructureData::lumber )
				m_wLumberCnt++;
			if( pUnit->GetTypeUnit() == CStructureData::coal )
				m_wCoalCnt++;
			if( pUnit->GetTypeUnit() == CStructureData::iron )
				m_wIronCnt++;
			if( pUnit->GetTypeUnit() == CStructureData::smelter )
				m_wSmelterCnt++;
			if( pUnit->GetTypeUnit() == CStructureData::rocket )
				m_dwRocket = pUnit->GetID();
			}
		}
	}

	pos = theVehicleMap.GetStartPosition ();
	while (pos != NULL)
	{
		theVehicleMap.GetNextAssoc( pos, dwDumb, pVehicle);

		// consider only those vehicles of this player
		if( pVehicle->GetOwner()->GetPlyrNum() == m_iPlayer )
		{
			try
			{
				// CAIUnit( DWORD dwID, int iOwner );
				CAIUnit *pUnit = new CAIUnit( 
					pVehicle->GetID(), m_iPlayer, 
					CUnit::vehicle, pVehicle->GetData()->GetType() );
				ASSERT_VALID( pUnit );
				m_plUnits->AddTail( (CObject *)pUnit );

				// be sure this unit starts clear
				pUnit->SetDataDW(0);
				pUnit->ClearParam();
				pUnit->SetStatus(0);
			}
			catch( CException e )
			{
				m_plUnits->RemoveAll();
				delete m_plUnits;
				m_plUnits = NULL;
				LeaveCriticalSection (&cs);

				throw(ERR_CAI_BAD_NEW);
			}
		}
	}
	LeaveCriticalSection (&cs);

	// now create available trucks, transport ships and 
	// buildings in need lists
	if( m_plTrucksAvailable != NULL )
	{
		m_plTrucksAvailable->RemoveAll();
		delete m_plTrucksAvailable;
		m_plTrucksAvailable = NULL;
	}
	if( m_plShipsAvailable != NULL )
	{
		m_plShipsAvailable->RemoveAll();
		delete m_plShipsAvailable;
		m_plShipsAvailable = NULL;
	}
	if( m_plBldgsNeed != NULL )
	{
		m_plBldgsNeed->RemoveAll();
		delete m_plBldgsNeed;
		m_plBldgsNeed = NULL;
	}
	try
	{
		m_plBldgsNeed = new CAIUnitList();
		m_plTrucksAvailable = new CAIUnitList();
		m_plShipsAvailable = new CAIUnitList();
	}
	catch( CException e )
	{
		if( m_plTrucksAvailable != NULL )
		{
			delete m_plTrucksAvailable;
			m_plTrucksAvailable = NULL;
		}
		if( m_plShipsAvailable != NULL )
		{
			delete m_plShipsAvailable;
			m_plShipsAvailable = NULL;
		}
		if( m_plBldgsNeed != NULL )
		{
			delete m_plBldgsNeed;
			m_plBldgsNeed = NULL;
		}

		m_plUnits->RemoveAll();
		delete m_plUnits;
		m_plUnits = NULL;

		throw(ERR_CAI_BAD_NEW);
	}

	GetBuildingNeeding();
	GetTrucksAvailable();
	GetShipsAvailable();

	ASSERT( TestEverything() );

	return TRUE;
}

CHPRouter::~CHPRouter()
{
	// these lists contain only pointers to
	// the instance of the CAIUnit used in the
	// main unit list, remove the pointers before
	// the list is deleted
	if( m_plTrucksAvailable != NULL )
	{
		m_plTrucksAvailable->RemoveAll();
		delete m_plTrucksAvailable;
	}
	if( m_plShipsAvailable != NULL )
	{
		m_plShipsAvailable->RemoveAll();
		delete m_plShipsAvailable;
	}	
	if( m_plBldgsNeed != NULL )
	{
		m_plBldgsNeed->RemoveAll();
		delete m_plBldgsNeed;
	}
	// this is the main unit list, so delete it
	// without removing the pointers and the unit's
	// instances will be deleted too
	if( m_plUnits != NULL )
	{
		delete m_plUnits;
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
void CHPRouter::DoRouting( CAIMsg *pMsg )
{
	ASSERT_VALID( this );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"\nCHPRouter for player %d ", m_iPlayer );
#endif // DEBUG_OUTPUT_ROUTE_HP

	// first and subsequent times thru
	if( pMsg == NULL )
		return;

	// may need to 
	BOOL bCheckCommodities = TRUE;

	// a new building that is placed or to be constructed
	if( pMsg->m_iMsg == CNetCmd::bldg_new )
	{
		// not this player's building
		//if( pMsg->m_idata3 != m_iPlayer )
		//	return;
		// not a building that needs/produces materials
		//if( pMsg->m_idata1 != CStructureData::rocket &&
		//	pMsg->m_idata1 < CStructureData::barracks_2 )
		//	return;

		bCheckCommodities = FALSE;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"bldg_new for player %d unit %ld will be a %d at %d,%d ",
			pMsg->m_idata3, pMsg->m_dwID, pMsg->m_idata1, pMsg->m_iX, pMsg->m_iY );
#endif // DEBUG_OUTPUT_ROUTE_HP

	ASSERT( TestEverything() );

		CAIUnit *pBldg = m_plUnits->GetUnitNY(pMsg->m_dwID);
		if( pBldg == NULL )
		{
			// need to add new building to m_plUnits
			UpdateUnits(pMsg);
			pBldg = m_plUnits->GetUnitNY(pMsg->m_dwID);
			if( pBldg == NULL )
				return;
			if( pBldg->GetTypeUnit() == CStructureData::rocket )
				m_dwRocket = pBldg->GetID();
				//GetRocketHex(pBldg);
		}

		// consider the commodity requirments
		if( pMsg->m_idata3 == m_iPlayer &&
			NeedsCommodities( pBldg ) )
		{
			// consider adding building to buildings need list
			if( m_plBldgsNeed->GetUnitNY( pBldg->GetID() )
				== NULL )
				m_plBldgsNeed->AddTail( (CObject *)pBldg );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"Player %d Building %ld needs commodites ",
		m_iPlayer, pBldg->GetID() );
#endif
			SetPriorities(); // prioritize construction/production
		}

	ASSERT( TestEverything() );

	}

	// a building status change
	if( pMsg->m_iMsg == CNetCmd::bldg_stat )
	{
		if( pMsg->m_idata3 != m_iPlayer )
			return;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"bldg_stat id=%ld  flags=%d  built=%d  type=%d  player=%d ",
			pMsg->m_dwID, pMsg->m_uFlags, pMsg->m_idata2, 
			pMsg->m_idata1, pMsg->m_idata3 );
#endif // DEBUG_OUTPUT_ROUTE_HP

	ASSERT( TestEverything() );

		// building is completed, or
		// the building has just started or
		// it is out of materials, 
		// so consider its material needs
		if( (pMsg->m_uFlags == CMsgBldgStat::built && pMsg->m_idata2 == 100) ||
			(pMsg->m_uFlags == CMsgBldgStat::built && pMsg->m_idata2 == 0) ||
			pMsg->m_uFlags == CMsgBldgStat::out_mat )
		{
			CAIUnit *pBldg = m_plUnits->GetUnitNY(pMsg->m_dwID);
			if( pBldg == NULL )
			{
				// need to add new building to m_plUnits
				UpdateUnits(pMsg);
				pBldg = m_plUnits->GetUnitNY(pMsg->m_dwID);
				if( pBldg == NULL )
					return;
			}

			// if the message reports that the construction is completed
			// then we need to count special types of buildings to help
			// later to determine if we have a 'no special' left situation
			if( pMsg->m_uFlags == CMsgBldgStat::built && 
				pMsg->m_idata2 == 100 )
			{
			if( pBldg->GetTypeUnit() == CStructureData::lumber )
				m_wLumberCnt++;
			if( pBldg->GetTypeUnit() == CStructureData::coal )
				m_wCoalCnt++;
			if( pBldg->GetTypeUnit() == CStructureData::iron )
				m_wIronCnt++;
			if( pBldg->GetTypeUnit() == CStructureData::smelter )
				m_wSmelterCnt++;
			}

			bCheckCommodities = FALSE;

			// consider the commodity requirments
			if( NeedsCommodities( pBldg ) )
			{
				// consider adding building to buildings need list
				if( m_plBldgsNeed->GetUnitNY( pBldg->GetID() )
					== NULL )
					m_plBldgsNeed->AddTail( (CObject *)pBldg );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"Player %d Building %ld needs commodites ",
		m_iPlayer, pBldg->GetID() );
#endif
				SetPriorities();	// prioritize construction/production
			}
		}

	ASSERT( TestEverything() );

	}

	// a unit was destroyed
	//
	if( pMsg->m_iMsg == CNetCmd::delete_unit )
	{
		//if( pMsg->m_idata3 != m_iPlayer )
		//	return;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"delete_unit  id=%ld player=%d ", 
			pMsg->m_dwID, pMsg->m_idata3 );
#endif // DEBUG_OUTPUT_ROUTE_HP

		CAIUnit *pUnit = m_plBldgsNeed->GetUnitNY(pMsg->m_dwID);
		if( pUnit != NULL )
		{
			// if a building is destroyed, then unassign trucks
			// and add them back to m_plTrucksAvailable
			// that are enroute and remove the building
			// from m_plBldgsNeed
			UnassignTrucks( pUnit );
			m_plBldgsNeed->RemoveUnit( pUnit->GetID(), FALSE );
		}
		else // possibly a truck
		{
			// consider if it is an
			// assigned truck for a building, and if so, then
			// remove its assignment, also if not assigned
			UnassignTrucks( pMsg->m_dwID );

			// remove it from m_plTrucksAvailable
			m_plTrucksAvailable->RemoveUnit( pMsg->m_dwID, FALSE );
		}

		// determine if this a special building of this player
		pUnit = m_plUnits->GetUnitNY(pMsg->m_dwID);
		if( pUnit == NULL )
			return;
		
		// if a special type of building was just deleted for the
		// HP Router player, then check m_plUnits for any more of
		// the special type, and set reserve-allowed flag
		if( pMsg->m_idata3 == m_iPlayer )
		{
			if( pUnit->GetType() == CUnit::building )
			{
				int iCnt = CountSpecialUnits( pUnit->GetTypeUnit() );
				if( pUnit->GetTypeUnit() == CStructureData::lumber &&
					iCnt == 1 )					
				{
					m_bLastLumber = TRUE;
				}
				else if( pUnit->GetTypeUnit() == CStructureData::coal &&
					iCnt == 1 )
				{
					m_bLastCoal = TRUE;
				}
				else if( pUnit->GetTypeUnit() == CStructureData::iron &&
					iCnt == 1 )
				{
					m_bLastIron = TRUE;
				}
				else if( pUnit->GetTypeUnit() == CStructureData::smelter &&
					iCnt == 1 )
				{
					m_bLastSmelter = TRUE;
				}
			}
		}

		// and now remove the unit
		m_plUnits->RemoveUnit( pMsg->m_dwID, TRUE );

		// re-prioritize construction/production
		SetPriorities(); 
	}

	// a new vehicle was built
	if( pMsg->m_iMsg == CNetCmd::veh_new )
	{
		if( pMsg->m_idata3 != m_iPlayer )
			return;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"veh_new player %d unit id: %ld  type: %d  at %d,%d ", 
	pMsg->m_idata3, pMsg->m_dwID, pMsg->m_idata1, pMsg->m_iX, pMsg->m_iY );
#endif // DEBUG_OUTPUT_ROUTE_HP

		CAIUnit *pUnit = m_plUnits->GetUnitNY(pMsg->m_dwID);
		if( pUnit == NULL )
		{
			// need to add new vehicle to m_plUnits
			UpdateUnits(pMsg);
			pUnit = m_plUnits->GetUnitNY(pMsg->m_dwID);
			if( pUnit == NULL )
				return;
		}

		// if vehicle is a truck, consider if it is assigned
		// already and if not, add it to available trucks list
		// and make sure it is an unassigned truck
		if( IsTruck(pUnit->GetID()) && !pUnit->GetDataDW() )
		{
			// truck may have materials
			if( HandleLoadedTruck(pUnit) )
			{
				// consider truck may already be in list, if not, add it
				if( m_plTrucksAvailable->GetUnitNY(pMsg->m_dwID) == NULL )
					m_plTrucksAvailable->AddTail( (CObject *)pUnit );
			}
		}
		else if( IsShip(pUnit->GetID()) )
		{
			// consider ship may already be in list, if not, add it
			if( m_plShipsAvailable->GetUnitNY(pMsg->m_dwID) == NULL )
				m_plShipsAvailable->AddTail( (CObject *)pUnit );
		}
		else
			return;

		// so make the unit move which
		// will generate a veh_dest message, and allow the
		// unit to be assigned later
		CHexCoord hexDest;
		if( GetStagingHex( pUnit, NULL, hexDest ) )
			SetDestination( pUnit->GetID(), hexDest );

		SetPriorities(); // prioritize construction/production
	}

	// a truck arrived at a source or destination building
	if( pMsg->m_iMsg == CNetCmd::veh_dest )
	{
		if( pMsg->m_idata3 != m_iPlayer )
			return;
		
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"\nveh_dest player %d unit id: %ld arrived at %d,%d \n", 
		pMsg->m_idata3, pMsg->m_dwID, pMsg->m_iX, pMsg->m_iY );
#endif // DEBUG_OUTPUT_ROUTE_HP

		DestinationResponse( pMsg );
	}

	// a truck is probably jammed up
	if( pMsg->m_iMsg == CNetCmd::err_veh_goto )
	{
		if( pMsg->m_idata3 != m_iPlayer )
			return;
		// need to respond
		VehicleErrorResponse(pMsg);
	}

	if( bCheckCommodities )
	{
		CheckWarehouses();

		// check all buildings in m_plBldgsNeed
		if( NeedsCommodities( NULL ) )
		{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"\nCHPRouter:: Player %d ALL Buildings being checked for commodites \n",
	m_iPlayer );
#endif // DEBUG_OUTPUT_ROUTE_HP

			SetPriorities();
		}
	}
	// each time thru, try to fill priorities of buildings
	// and set trucks params with what they need to get
	// and where they get it and take it
	FillPriorities();
}

void CHPRouter::VehicleErrorResponse( CAIMsg *pMsg )
{
	ASSERT_VALID( this );

	// get the unit that just arrived
	CAIUnit *pUnit = m_plUnits->GetUnitNY( pMsg->m_dwID );
	if( pUnit == NULL )
		return;

	// handle vehicles with a problem getting to destination
	if( pMsg->m_iMsg == CNetCmd::err_veh_goto ||
		pMsg->m_iMsg == CNetCmd::err_veh_traffic )
	{
		CHexCoord hexNext( pMsg->m_idata1, pMsg->m_idata2 );
		CHexCoord hexDest( pMsg->m_ieX, pMsg->m_ieY );
		CHexCoord hexVeh( pMsg->m_iX, pMsg->m_iY );

		// if the unit's current hex and dest hex match the message's
		// then the error message has not been acted on, otherwise
		// the unit is already responding to an error message
		CHexCoord hexGo(0,0);
		CHexCoord hexAt(0,0);
		EnterCriticalSection (&cs);
		CVehicle *pVeh = theVehicleMap.GetVehicle( pUnit->GetID() );
		if( pVeh != NULL )
		{
			hexAt = pVeh->GetHexHead();
			hexGo = pVeh->GetHexDest();
		}
		LeaveCriticalSection (&cs);
		if( !hexAt.X() && !hexAt.Y() )
			return;
		if( hexAt != hexVeh || hexGo != hexDest )
			return;

		if( GetStagingHex( pUnit, NULL, hexDest ) )
			SetDestination( pUnit->GetID(), hexDest );
	}
}
//
// this is going to have to deal with trucks looking for ships
// as well as buildings and ships looking for pickup/unload points
//
void CHPRouter::DestinationResponse( CAIMsg *pMsg )
{
	ASSERT_VALID( this );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::DestinationResponse() player %d respondinging to veh_dest ",
    	m_iPlayer );
#endif

	// get the unit that just arrived
	CAIUnit *pUnit = m_plUnits->GetUnitNY( pMsg->m_dwID );
	if( pUnit == NULL )
	{
		return;
	}

	// consider it may be a truck transporting material
	//
	if( IsTruck(pUnit->GetID()) )
	{
		// get the unit of the hex which the truck
		// has just arrived in, and 
		CAIHex aiHex( pMsg->m_iX, pMsg->m_iY );
		GetCHexData( &aiHex );
		
		if( aiHex.m_iUnit == CUnit::building )
		{
			// if its dwID matches
			// the pUnit->GetDataDW() value it has arrived
			// at the building needing the materials it is
			// currenting carrying, and 
			if( aiHex.m_dwUnitID == pUnit->GetDataDW() )
			{
				CAIUnit *pBldg = 
					m_plUnits->GetUnitNY( pUnit->GetDataDW() );
				if( pBldg == NULL )
					return;

				// transfer the qty needed to the building 
				// and clear the truck of assignment
				BOOL bNeedMore = UnloadMaterials( pUnit, pBldg );

				// now move the truck out of the building and into
				// an unoccupied location some distance away from it
				CHexCoord hexDest;
				BOOL bFoundHex = GetStagingHex( pUnit, pBldg, hexDest );
				// can't get buidling, then try to get to the rocket
				if( !bFoundHex )
				{
					CAIUnit *pRocketBldg = 
						m_plUnits->GetUnitNY( m_dwRocket );
					if( pRocketBldg == NULL )
						return;
					bFoundHex = GetStagingHex( pUnit, pRocketBldg, hexDest );
				}
				if( bFoundHex )
					SetDestination( pUnit->GetID(), hexDest );

				// building still needs more materials, 
				// must rely on game reissuing 
				// out_mat building status message

				// if( bNeedMore )
				//	FakeOutMatMsg( pBldg );
					
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::DestinationResponse() player %d unload completed \n",m_iPlayer );
#endif
				return;

			}
			// a truck has arrived at a building, and does not
			// have a building assigned, which may indicate that
			// a restore has occurred, so there is no info in
			// the truck or building to tell the router what to
			// do, because the restore did a re-init and not a load
			else if( aiHex.m_dwUnitID && !pUnit->GetDataDW() )
			{
				CAIUnit *pBldg = 
					m_plUnits->GetUnitNY( aiHex.m_dwUnitID );
				if( pBldg == NULL )
					return;

				// transfer the qty needed to the building 
				// and clear the truck of assignment
				BOOL bNeedMore = UnloadMaterials( pUnit, pBldg );

				// now move the truck out of the building and into
				// an unoccupied location some distance away from it
				CHexCoord hexDest;
				if( GetStagingHex(pUnit, pBldg, hexDest) )
					SetDestination( pUnit->GetID(), hexDest );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::DestinationResponse() player %d re-init unload completed \n",m_iPlayer );
#endif

				// re-consider the commodity requirments
				if( NeedsCommodities( pBldg ) )
				{
					// consider adding building to buildings need list
					if( m_plBldgsNeed->GetUnitNY( pBldg->GetID() )
						== NULL )
						m_plBldgsNeed->AddTail( (CObject *)pBldg );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"Player %d Building %ld needs commodites ",m_iPlayer, pBldg->GetID() );
#endif
					SetPriorities();	// prioritize construction/production
				}
				return;

			}
			else if( aiHex.m_dwUnitID ) // non-zero means something here
			{
		// or a truck that must use a land/water/land path has just
		// arrived at a pickup up point that is a seaport, and in
		// that case the truck needs to load to a transport ship
		// and use its CAIUnit::params(CAI_ROUTE_X/Y) hex to find
		// the nearest seaport, and then have the transport ship
		// proceed to that nearest seaport
				if( ConsiderLandWater(pUnit, &aiHex) )
					return;

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

				LoadMaterials( pUnit, &aiHex );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::DestinationResponse() player %d load completed ",m_iPlayer );
#endif
				return;

			}
		}

		// make sure assigned building still exists
		if( pUnit->GetDataDW() )
		{
			CAIUnit *pBldg = 
				m_plUnits->GetUnitNY( pUnit->GetDataDW() );
			if( pBldg == NULL )
			{
			// no, so unassign the truck
			pUnit->SetDataDW(0);
			pUnit->ClearParam();
			pUnit->SetStatus(0);
			// and add it to the list
			CAIUnit *pTruck = 
				m_plTrucksAvailable->GetUnitNY( pUnit->GetID() );
			if( pTruck == NULL )
				m_plTrucksAvailable->AddTail( (CObject *)pUnit );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::DestinationResponse() player %d truck %ld is unassigned ",
pUnit->GetOwner(), pUnit->GetID() );
#endif

			return;
			}

			// make sure source buildings still exist?
			int iBestDist = 0xFFFF;
			BOOL bSourceGone = FALSE;
			CHexCoord hexTruck( aiHex.m_iX, aiHex.m_iY );
			CHexCoord hexBldg(0,0), hexBest(0,0);

			for( int i=0; i<CMaterialTypes::num_types; ++i )
			{
				if( pUnit->GetParamDW(i) )
				{
					// get next source building 
					hexBldg.X(0);
					hexBldg.Y(0);
					EnterCriticalSection (&cs);
					CBuilding *pBuilding = 
						theBuildingMap.GetBldg( pUnit->GetParamDW(i) );
					if( pBuilding != NULL )
						hexBldg = pBuilding->GetExitHex();
					LeaveCriticalSection (&cs);

					// a source building no longer exists
					if( !hexBldg.X() && !hexBldg.Y() )
					{
						pUnit->SetParamDW(i,0);
						bSourceGone = TRUE;
						continue;
					}

					// however, it is not the source at the hexTruck
					if( hexBldg == hexTruck )
					{
						bSourceGone = FALSE;
						break;
					}

					// get dist to reach building
					int iDist = theMap.GetRangeDistance( hexTruck, hexBldg );

					// find the one closest and save it
					if( iDist && iDist < iBestDist )
					{
						iBestDist = iDist;
						hexBest = hexBldg;
					}
				}
			}
			// engage
			if( bSourceGone )
			{
				// store the location of the next source
				pUnit->SetParam(CAI_ROUTE_X,hexBest.X());
				pUnit->SetParam(CAI_ROUTE_Y,hexBest.Y());
			}
		}

		// no building here so go to current destination
		CHexCoord hexDest( pUnit->GetParam(CAI_ROUTE_X),
			pUnit->GetParam(CAI_ROUTE_Y) );
		if( hexDest.X() || hexDest.Y() )
		{
			ConsiderLandWater( pUnit, hexDest );

			SetDestination( pUnit->GetID(), hexDest );
			return;
		}
		else
		{
			// unit is not going anywhere so if its a truck
			// put it back in the available truck list
			if( IsTruck(pUnit->GetID()) )
			{
				// be sure this unit starts clear
				pUnit->SetDataDW(0);
				pUnit->ClearParam();
				pUnit->SetStatus(0);
				// consider truck may already be in list, if not, add it
				if( m_plTrucksAvailable->GetUnitNY(pUnit->GetID()) == NULL )
					m_plTrucksAvailable->AddTail( (CObject *)pUnit );
			}
		}
	}
	else if( IsShip(pUnit->GetID()) )
	{
		// ship may be carrying a truck on a land/water/land route
		// and needs to unload the truck, in which case the current
		// location is both a seaport and the unload point of the ship
		// which is in CAIUnit::params(CAI_DEST_X/Y) for the ship
		//
		// or ship may have just arrived at a pickup point to load a
		// truck to embark on a land/water/land route, which are in
		// the CAIUnit::params(CAI_PREV_X/Y) for pickup point and
		// the CAIUnit::params(CAI_DEST_X/Y) for unload point
		// of the ship
		// 
		// once unloaded, then the truck will be sent to its land
		// destination which is in CAIUnit::params(CAI_ROUTE_X/Y)
		// by the default SetDestination action above
		// get the unit of the hex which the truck
		// has just arrived in, and 

		CAIHex aiHex( pMsg->m_iX, pMsg->m_iY );
		GetCHexData( &aiHex );
		
		// hex contains a building
		if( aiHex.m_iUnit == CUnit::building )
		{
			CAIUnit *pBldg = NULL;
			//
			// for the ship unit:
			// pUnit->GetParamDW(CAI_LOC_X) = truck to load
			// pUnit->GetParamDW(CAI_PREV_X) = seaport to load at
			// pUnit->GetParamDW(CAI_DEST_X) = seaport to unload at
			//
			// that building is the one this ship was heading to?
			//
			if( aiHex.m_dwUnitID == pUnit->GetParamDW(CAI_PREV_X) )
			{
				pBldg = m_plUnits->GetUnitNY( aiHex.m_dwUnitID );
				if( pBldg != NULL )
				{
					// this is the seaport we were looking for to pick
					// up a truck from so find the truck and load it
					CAIUnit *pTruck =
					m_plUnits->GetUnitNY( pUnit->GetParamDW(CAI_LOC_X) );
					if( pTruck != NULL )
					{
						// the ship is assigned to this truck
						ConsiderLandWater(pTruck, &aiHex);

						// if the truck is already at the seaport waiting
						// then the ship will load the truck
						return;
					}

					// something is wrong, the truck we are looking
					// for is no longer available, so we must unassign
					// the ship and move it off to a staging area

					// now unassign this ship
					UnAssignShip(pUnit);

					CHexCoord hexDest;
					if( GetStagingHex(pUnit, pBldg, hexDest) )
						SetDestination( pUnit->GetID(), hexDest );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::DestinationResponse() player %d unassigning ship %ld ",
	m_iPlayer, pUnit->GetID() );
#endif // DEBUG_OUTPUT_ROUTE_HP

					return;
				}
			}
			else if( aiHex.m_dwUnitID == pUnit->GetParamDW(CAI_DEST_X) )
			{
				pBldg = m_plUnits->GetUnitNY( aiHex.m_dwUnitID );
				if( pBldg != NULL )
				{
					// this is the seaport we were looking for to unload
					// unload the truck this ship is carrying
					UnloadCargo(pUnit->GetID());

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::DestinationResponse() player %d ship %ld unloading cargo ",
	m_iPlayer, pUnit->GetID() );
#endif // DEBUG_OUTPUT_ROUTE_HP

				}
				// now unassign this ship
				UnAssignShip(pUnit);
				// and stage it out of the way
				CHexCoord hexDest;
				if( GetStagingHex(pUnit, pBldg, hexDest) )
					SetDestination( pUnit->GetID(), hexDest );

				return;
			}
			else if( aiHex.m_dwUnitID ) // non-zero means something here
			{
				CHexCoord hexDest;
				BOOL bFoundHex = FALSE;
				// but it is not someplace the ship was trying to get to
				//
				// so go to the pickup seaport if it is the only one
				if( pUnit->GetParamDW(CAI_PREV_X) &&
					!pUnit->GetParamDW(CAI_DEST_X) )
				{
					bFoundHex = GetShipHex( pUnit->GetParamDW(CAI_PREV_X), hexDest );
				}
				else if( pUnit->GetParamDW(CAI_PREV_X) &&
					pUnit->GetParamDW(CAI_DEST_X) )
				{
					// or to the dropoff seaport if there is one
					bFoundHex = GetShipHex( pUnit->GetParamDW(CAI_DEST_X), hexDest );
				}
				if( bFoundHex )
					SetDestination( pUnit->GetID(), hexDest );
			}
		}
	}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::DestinationResponse() player %d completed ",m_iPlayer );
#endif

}

//
// a truck has been returned to HP Router control, and it may
// contain materials which need to be unloaded at the nearest
// warehouse (or building) before the truck goes into the 
// available truck list
//
BOOL CHPRouter::HandleLoadedTruck( CAIUnit *pTruck )
{
	pTruck->ClearParam();

	// get any cargo on the truck into m_iStore[]
	BOOL bHasCargo = FALSE;
	GetVehicleCargo(pTruck->GetID());
	for( int i=0; i<CMaterialTypes::num_types; ++i )
	{
		if( m_iStore[i] )
		{
			bHasCargo = TRUE;
			// save cargo into truck's local params at
			// offset that ids the type of material
			pTruck->SetParam( i, m_iStore[i] );
		}
	}
	// no cargo to be concerned with
	if( !bHasCargo )
		return TRUE;

	CHexCoord hexVeh;
	if( !GetVehicleHex( pTruck->GetID(), hexVeh ) )
		return FALSE;

#if DEBUG_OUTPUT_ROUTE
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CHPRouter::HandleLoadedTruck() player %d new truck %ld at %d,%d is loaded ", 
m_iPlayer, pTruck->GetID(), hexVeh.X(), hexVeh.Y() );
#endif

	// if still here, then the truck has cargo so find the nearest
	// warehouse (or building) that is not already got a truck assigned
	// and then assign this truck to it and set all the materials on
	// the truck as needed by the assigned building and send the truck
	// to the assigned building and return FALSE so it does not go into
	// the available truck list
	
	CHexCoord hexBldg, hexBest;
	int iDist, iBestDist = 0xFFFE;
	DWORD dwBest = 0;

    POSITION pos = m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pBldg = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pBldg != NULL )
        {
          	ASSERT_VALID( pBldg );
			
			if( pBldg->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a building
			if( pBldg->GetType() == CUnit::building )
			{
				// if only warehouses are considered then keep this
				// else comment it out
				if( pBldg->GetTypeUnit() != CStructureData::rocket &&
					pBldg->GetTypeUnit() != CStructureData::warehouse )
					continue;
				
				// does this building already have a truck assigned
				// for materials carried by this truck
				bHasCargo = FALSE;
				for( i=0; i<CMaterialTypes::num_types; ++i )
				{
					if( pTruck->GetParam(i) )
					{
						// building already has truck assigned
						if( pBldg->GetParamDW(i) )
							bHasCargo = TRUE;
					}
				}
				// skip this building
				if( bHasCargo )
					continue;

				// if still here then get distance
				if( !GetBldgExit( pBldg->GetID(), hexBldg ) )
					continue;
				
				iDist = theMap.GetRangeDistance( hexVeh, hexBldg );
				if( iDist > 0 && iDist < iBestDist )
				{
					iBestDist = iDist;
					hexBest = hexBldg;
					dwBest = pBldg->GetID();
				}
			}
		}
	}

	// no place to unload, what should I do?  Keep the truck
	if( !dwBest )
		return TRUE;

	CAIUnit *pBldgFound = m_plUnits->GetUnitNY(dwBest);
	// bad unit, what should I do?
	if( pBldgFound == NULL )
		return FALSE;

#if DEBUG_OUTPUT_ROUTE
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CHPRouter::HandleLoadedTruck() player %d building %ld at %d,%d found for unloading ", 
m_iPlayer, pBldgFound->GetID(), hexBest.X(), hexBest.Y() );
#endif

	WORD wStatus = 0;
	wStatus |= CAI_IN_USE;
	pTruck->SetStatus( wStatus );			// truck is now assigned
	pTruck->SetDataDW( pBldgFound->GetID() ); // id of building needing

	// set truck id at building's param for material and
	// set building's param at material for amount to unload
	// and record this commodity as needed
	for( i=0; i<CMaterialTypes::num_types; ++i )
	{
		if( pTruck->GetParam(i) )
			pBldgFound->SetParamDW( i, pTruck->GetID() );
		pBldgFound->SetParam(i,pTruck->GetParam(i));
	}
	pBldgFound->SetParam( CAI_UNASSIGNED, 0 );

	// add the unload building to the buildings need list
	if( m_plBldgsNeed->GetUnitNY( pBldgFound->GetID() ) == NULL )
		m_plBldgsNeed->AddTail( (CObject *)pBldgFound );

	// and go there
	//GetBldgExit( pBldgFound->GetID(), hexBldg );
	SetDestination( pTruck->GetID(), hexBest );
	pTruck->SetParam(CAI_ROUTE_X,hexBest.X());
	pTruck->SetParam(CAI_ROUTE_Y,hexBest.Y());

#if DEBUG_OUTPUT_ROUTE
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CHPRouter::HandleLoadedTruck() player %d loaded truck %ld going to %d,%d for unloading \n", 
m_iPlayer, pTruck->GetID(), hexBest.X(), hexBest.Y() );
#endif

	// TRUE will cause truck to be considered to be added 
	// to available truck list, FALSE will not
	return FALSE;
}

//
// called on bldg_new and veh_new messages to add a new
// building or vehicle unit to the main unit list
//
void CHPRouter::UpdateUnits( CAIMsg *pMsg )
{
	// determine unit type (building/vehicle)
	int iType = pMsg->m_iMsg == CNetCmd::veh_new 
		? CUnit::vehicle : CUnit::building;
	// a place message may not have owner
	int iPlayer = pMsg->m_idata3 ? pMsg->m_idata3 : m_iPlayer;

	CAIUnit *pUnit = NULL;
	try
	{
		// CAIUnit( DWORD dwID, int iOwner, int iType );
		pUnit = new CAIUnit( pMsg->m_dwID, 
				iPlayer, iType, pMsg->m_idata1 );
		ASSERT_VALID( pUnit );
		m_plUnits->AddTail( (CObject *)pUnit );

		// be sure this unit starts clear
		pUnit->SetDataDW(0);
		pUnit->ClearParam();
		pUnit->SetStatus(0);
	}
	catch( CException e )
	{
		// need to report this error occurred
		throw(ERR_CAI_BAD_NEW);
	}
	
	// increment count of seaports
	if( pUnit->GetType() == CUnit::building )
	{
		if( pUnit->GetOwner() == m_iPlayer )
		{
			if( pUnit->GetTypeUnit() == CStructureData::seaport )
				m_wSeaportCnt++;
		}
	}
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"\nCHPRouter::UpdateUnits(): player %d created new unit id=%ld type=%d \n",
m_iPlayer, pMsg->m_dwID, pMsg->m_idata1 );
#endif

	// what, you don't trust the constructor?
	pUnit->SetDataDW( (DWORD)0 );
	pUnit->SetTask( FALSE );
	pUnit->SetGoal( FALSE );
	pUnit->ClearParam();
}
//
// go thru the list of units, finding buildings that
// need materials, and add them to the list, and this
// only happens once
//
void CHPRouter::GetBuildingNeeding( void )
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
			if( pUnit->GetType() == CUnit::building )
			{
				// not a building that needs/produces materials
				//if( pUnit->GetTypeUnit() < CStructureData::barracks_2 )
				//	continue;

				// consider the commodity requirments
				if( NeedsCommodities( pUnit ) )
				{
					if( m_plBldgsNeed->GetUnitNY(pUnit->GetID()) == NULL )
						m_plBldgsNeed->AddTail( (CObject *)pUnit );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"Player %d Building %ld needs commodites ",m_iPlayer, pUnit->GetID() );
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
void CHPRouter::SetPriorities( void )
{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::SetPriorities during Routing() for player %d ", m_iPlayer );
#endif

    POSITION pos = m_plBldgsNeed->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pUnit = (CAIUnit *)m_plBldgsNeed->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );

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

			if( !IsValidUnit( pUnit ) )
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
void CHPRouter::FillPriorities( void )
{
	BOOL bTransportFound = FALSE;

	int iCnt = m_plBldgsNeed->GetCount();

	while( iCnt )
	{
		// take the building off the top of the list
		// instead of considering the priority
		CAIUnit *pBldg = (CAIUnit *)m_plBldgsNeed->RemoveHead();
		if( pBldg != NULL )
		{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::FillPriorities for player %d building %ld ", 
pBldg->GetOwner(), pBldg->GetID() );
#endif
			// returning FALSE keeps building in the m_plBldgsNeed
			// while TRUE removes it from m_plBldgsNeed
			bTransportFound = FindTransport( pBldg );

			if( !bTransportFound )
				m_plBldgsNeed->AddTail( (CObject *)pBldg );
		}
		
		iCnt--;
	}

	// Secondary Stocking of materials to specific buildings
	//
	// move qty > EXCESS_MATERIALS of steel, wood, copper
	// to barracks_2, light_0, light_1, light_2, heavy, repair,
	// shipyard_1, shipyard_3
	// 
	if( m_plTrucksAvailable->GetCount() > MINIMUM_IDLE_TRUCKS )
		SecondaryStocking( CMaterialTypes::steel, CStructureData::smelter,
			CStructureData::num_types );
	if( m_plTrucksAvailable->GetCount() > MINIMUM_IDLE_TRUCKS )
		SecondaryStocking( CMaterialTypes::lumber, CStructureData::lumber,
			CStructureData::num_types );
	if( m_plTrucksAvailable->GetCount() > MINIMUM_IDLE_TRUCKS )
		SecondaryStocking( CMaterialTypes::copper, CStructureData::copper,
			CStructureData::num_types );
}

#if 0
// the routine that currently works!!!!
void CHPRouter::FillPriorities( void )
{
	WORD wPriority = 0;
	DWORD dwPicked = 0;
	BOOL bTrucksRemain = TRUE;
	
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

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::FillPriorities for player %d building %ld priority %d ", 
m_iPlayer, pUnit->GetID(), pUnit->GetParam(CAI_UNASSIGNED) );
#endif

				if( !IsValidUnit( pUnit ) )
			 		continue;

				// what a KLUDGE!!!!!
				// now do a random test to decide if certain units
				// may get skipped for this priority selection
				//
				// first make sure there are more buildings needing
				// materials than there are trucks available
				if( m_plBldgsNeed->GetCount() > 
					m_plTrucksAvailable->GetCount() )
				{
					// specific units may get skipped
					if( pUnit->GetTypeUnit() == CStructureData::smelter ||
						pUnit->GetTypeUnit() == CStructureData::refinery ||
						pUnit->GetTypeUnit() == CStructureData::power_1 ||
						pUnit->GetTypeUnit() == CStructureData::power_2 )
					{
						// skip if ZERO returned
						if( !RandNum(1) )
							continue;
					}
				}

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
		CAIUnit *pBldg = m_plBldgsNeed->GetUnitNY( dwPicked );
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

	// Secondary Stocking of materials to specific buildings
	//
	// move qty > EXCESS_MATERIALS of steel, wood, copper
	// to barracks_2, light_0, light_1, light_2, heavy, repair,
	// shipyard_1, shipyard_3
	// 
	if( m_plTrucksAvailable->GetCount() > MINIMUM_IDLE_TRUCKS )
		SecondaryStocking( CMaterialTypes::steel, CStructureData::smelter,
			CStructureData::num_types );
	if( m_plTrucksAvailable->GetCount() > MINIMUM_IDLE_TRUCKS )
		SecondaryStocking( CMaterialTypes::lumber, CStructureData::lumber,
			CStructureData::num_types );
	if( m_plTrucksAvailable->GetCount() > MINIMUM_IDLE_TRUCKS )
		SecondaryStocking( CMaterialTypes::copper, CStructureData::copper,
			CStructureData::num_types );
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
void CHPRouter::SecondaryStocking( int iMat, int iFromBldg, int iToBldg )
{
	int iExcess=0, iBestFromExcess=0, iBestToExcess=0xFFFE;
	int iVehCount, iBestVehCount=0;
	CAIUnit *paiFrom = NULL;
	CAIUnit *paiTo = NULL;
	CAIUnit *pUnit = NULL;

   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;
			// determine if this is a building
			if( pUnit->GetType() != CUnit::building )
				continue;

			if( pUnit->GetTypeUnit() == iFromBldg ||
				pUnit->GetTypeUnit() == iToBldg )
			{

				// make sure the neither building does not already need material
				//if( pUnit->GetTypeUnit() == iFromBldg )
				//{
					CAIUnit *paiBldgNeed = 
						m_plBldgsNeed->GetUnitNY(pUnit->GetID());
					if( paiBldgNeed != NULL )
						continue;
				//}

				// this is a valid iFromBldg/iToBldg, so check how much iMat it has
				EnterCriticalSection (&cs);
				CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					// this uses criteria of selecting the from building with
					// the most over the excess threshhold
					if( pUnit->GetTypeUnit() == iFromBldg )
					{
						iExcess = pBldg->GetStore(iMat) - EXCESS_MATERIALS;
						if( iExcess > 0 && iExcess > iBestFromExcess )
						{
						iBestFromExcess = iExcess;
						paiFrom = pUnit;
						}
					}
					else if( pUnit->GetTypeUnit() == iToBldg )
					{
						pBldg->GetAccepts(&m_iNeeds[0]);
						if( !m_iNeeds[iMat] )
						{
							LeaveCriticalSection (&cs);
							continue;
						}

						iExcess = pBldg->GetStore(iMat);

						// if this building already has qty > EXCESS_MATERIALS
						// of the material, then skip this building for consideration
						if( iExcess < EXCESS_MATERIALS )
						{
							// this criteria uses:
							// selecting the to building with the least
							// of that material
							if( iExcess > 0 && iExcess < iBestToExcess )
							{
								iBestToExcess = iExcess;
								paiTo = pUnit;
							}
						}
					}
				}
				LeaveCriticalSection (&cs);
			}

			// possible multiple to building types
			if( iToBldg == CStructureData::num_types )
			{
				if( pUnit->GetTypeUnit() != CStructureData::light_0 &&
					pUnit->GetTypeUnit() != CStructureData::light_1 &&
					pUnit->GetTypeUnit() != CStructureData::light_2 &&
					pUnit->GetTypeUnit() != CStructureData::heavy &&
					pUnit->GetTypeUnit() != CStructureData::barracks_2 &&
					pUnit->GetTypeUnit() != CStructureData::repair &&
					pUnit->GetTypeUnit() != CStructureData::shipyard_1 &&
					pUnit->GetTypeUnit() != CStructureData::shipyard_3 )
					continue;

				// make sure the building does not already need material				
				CAIUnit *paiBldgNeed = 
					m_plBldgsNeed->GetUnitNY(pUnit->GetID());
				if( paiBldgNeed != NULL )
					continue;
				
				// this is a valid iToBldg, so check how much iMat it has
				EnterCriticalSection (&cs);
				CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					pBldg->GetAccepts(&m_iNeeds[0]);
					if( !m_iNeeds[iMat] )
					{
						LeaveCriticalSection (&cs);
						continue;
					}

					// if this building already has qty > EXCESS_MATERIALS
					// of the material, then skip this building for consideration
					iExcess = pBldg->GetStore(iMat);
					if( iExcess < EXCESS_MATERIALS )
					{
						iVehCount = GetVehicleCount( pBldg );
						if( iVehCount > iBestVehCount )
						{
							iBestVehCount = iVehCount;
							paiTo = pUnit;
						}

						// none of the factories are producing or repairing
						// so find the one with the least amount of material
						if( !iBestVehCount )
						{
						// this criteria uses:
						// selecting the to building with the least
						// of that material
							if( iExcess >= 0 && iExcess < iBestToExcess )
							{
							iBestToExcess = iExcess;
							paiTo = pUnit;
							}
						}
					}
				}
				LeaveCriticalSection (&cs);
			}
		}
	}

	// buildings not found with material above minimums
	if( paiFrom == NULL || paiTo == NULL )
		return;

	CAIUnit *pTruck = GetNearestTruck( paiFrom );
	if( pTruck == NULL )
	{

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
    	"CHPRouter::SecondaryStocking() no truck available, count=%d for player %d building id %ld ",
		 m_plTrucksAvailable->GetCount(), m_iPlayer, paiFrom->GetID() );
#endif
		return;
	}

	// check to be sure truck can get to the to building from the from building
	CHexCoord hexFrom(0,0), hexTo(0,0);
	EnterCriticalSection (&cs);
	CBuilding *pBldg = theBuildingMap.GetBldg( paiFrom->GetID() );
	if( pBldg != NULL )
		hexFrom = pBldg->GetExitHex();
	pBldg = theBuildingMap.GetBldg( paiTo->GetID() );
	if( pBldg != NULL )
		hexTo = pBldg->GetExitHex();
	LeaveCriticalSection (&cs);

	if( (!hexFrom.X() && !hexFrom.Y()) ||
		(!hexTo.X() && !hexTo.Y()) )
		return;

	// truck can't get from hex -> to hex
	BOOL bGotPath = FALSE;
	int iPathLen = 0;
	CHexCoord *pPath = thePathMgr.GetPath( 
		NULL, hexFrom, hexTo, iPathLen, pTruck->GetTypeUnit(), FALSE, TRUE );
	if( pPath != NULL )
	{
		// check last step
		if( iPathLen )
			iPathLen--;
		// if we did not get to the destination then either the path does
		// need to cross water or it was too long, either way do another
		// test this time using GetRoadPath()
		CHexCoord *pHex = &pPath[iPathLen];
		if( pHex->X() == hexTo.X() &&
			pHex->Y() == hexTo.Y() )
			bGotPath = TRUE;

		delete [] pPath;
	}
	else
		return;

	if( !bGotPath )
		return;

	// update the truck and building as need commodiaties
	pTruck->ClearParam();

	WORD wStatus = 0;
	wStatus |= CAI_IN_USE;
	pTruck->SetStatus( wStatus );			// truck is now assigned
	pTruck->SetDataDW( paiTo->GetID() );	// id of building needing

	// update the truck with the dwID of the building
	// that is a source for the material again using
	// offset of the material needed
	pTruck->SetParamDW( iMat, paiFrom->GetID() );
	pTruck->SetParam( iMat, iBestFromExcess );
	SetDestination( pTruck->GetID(), hexFrom );
	// remove pointer to truck from trucks available list
	m_plTrucksAvailable->RemoveUnit( pTruck->GetID(), FALSE );
	
	// record truck at the building that needs it
	paiTo->SetParamDW( iMat, pTruck->GetID() );
	paiTo->SetParam( iMat, iBestFromExcess );

	// force building priority to 0 on any material
	// assignment, and if other materials are still
	// unassigned, they will be ided next time
	paiTo->SetParam( CAI_UNASSIGNED, 0 );
	
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CHPRouter::SecondaryStocking() player %d transport truck %ld assigned to building %ld ", 
m_iPlayer, pTruck->GetID(), paiTo->GetID() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CHPRouter::m_plTrucksAvailable->GetCount() is %d ", 
m_plTrucksAvailable->GetCount() );
#endif

	// add to buildings need list
	if( m_plBldgsNeed->GetUnitNY(paiTo->GetID()) == NULL )
		m_plBldgsNeed->AddTail( (CObject *)paiTo );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CHPRouter::SecondaryStocking() Player %d Building %ld stocking up commodity %d ",
m_iPlayer, paiTo->GetID(), iMat );
#endif
}

// if this building is a factory with production
// underway, then use the quantity of vehicles to
// be built as a criteria
//
// if this building is a shipyard or repair with
// a repair in progress, then use the quantity of
// vehicles queued for repair as a criteria
int CHPRouter::GetVehicleCount( CBuilding *pBldg )
{
	int iVehCount = 0;
	if( pBldg->GetData()->GetUnionType() == CStructureData::UTvehicle )
	{
		CVehicleBuilding *pVehBldg = (CVehicleBuilding *)pBldg;
		// consider if a vehicle is in production, and if so, get count
		CBuildUnit const *pBuildVeh = pVehBldg->GetBldUnt();
		if( pBuildVeh != NULL )
		{
			iVehCount = 1;
			iVehCount += pVehBldg->GetNum();
		}
	}
	else if( pBldg->GetData()->GetUnionType() == CStructureData::UTrepair )
	{
		CRepairBuilding *pRepBldg = (CRepairBuilding *)pBldg;
		// is there a vehicle in getting a repair?
		CVehicle *pVehRepairing = pRepBldg->GetVehRepairing();
		if( pVehRepairing != NULL )
			iVehCount = 1; // need access to pRepBldg->m_lstNext->GetCount()
	}
	else if( pBldg->GetData()->GetUnionType() == CStructureData::UTshipyard )
	{
		CShipyardBuilding *pShipBldg = (CShipyardBuilding*)pBldg;
		// is there a vehicle in getting a repair?
		CVehicle *pVehRepairing = pShipBldg->GetVehRepairing();
		if( pVehRepairing != NULL )
		{
			iVehCount = 1; // need access to pShipBldg->m_lstNext->GetCount()
		}
		else // is a vehicle being constructed?
		{
			CBuildUnit const *pBuildVeh = pShipBldg->GetBldUnt();
			if( pBuildVeh != NULL )
			{
				iVehCount = 1;
				iVehCount += pShipBldg->GetNum();
			}
		}
	}
	return( iVehCount );
}

//
// indicate if there is a need for more trucks
//
BOOL CHPRouter::NeedTransports( void )
{
	if( !m_plTrucksAvailable->GetCount() )
		return TRUE;
	return FALSE;
}

//
// this building needs a truck for transporting materials,
// so find an available truck, then locate sources of the
// materials that are needed and route the truck to each
// source and pick up what is needed
//

BOOL CHPRouter::FindTransport( CAIUnit *pCAIBldg )
{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::FindTransport during Routing() for player %d ", m_iPlayer );
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"for building id %ld ", pCAIBldg->GetID() );
#endif

	// consider that trucks enroute are meeting the
	// the material needs of the building
	if( TrucksAreEnroute( pCAIBldg ) )
	{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::truck enroute to player %d building id %ld \n",
m_iPlayer, pCAIBldg->GetID() );
#endif
		// remove from m_plBldgsNeed list
		return( TRUE );
	}

	// first consider that there are no more trucks available
	// so there is no point in continuing onward
	if( !m_plTrucksAvailable->GetCount() )
	{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::FindTransport() no truck available for player %d building id %ld ",
m_iPlayer, pCAIBldg->GetID() );
#endif

		// GAME MESSAGE needed - no trucks available
		theGame.Event( EVENT_HPR_TRUCKS, EVENT_BAD );

		// stay in m_plBldgsNeed list
		return( FALSE );
	}

	// need exit hex
	CHexCoord hexExit;
	if( !GetBldgExit( pCAIBldg->GetID(), hexExit ) )
		return( TRUE );

	// find the nearest truck from the list, with preference
	// given for trucks containing needed materials, then empty
	// trucks then finally any truck, and return it to here
	CAIUnit *pTruck = GetNearestTruck( pCAIBldg );
	if( pTruck == NULL )
	{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::GetNearestTruck() reports %d trucks not available for player %d building id %ld \n",
m_plTrucksAvailable->GetCount(), m_iPlayer, pCAIBldg->GetID() );
#endif

		// GAME MESSAGE needed - no trucks available
		theGame.Event (EVENT_HPR_TRUCKS, EVENT_BAD );

		// stay in m_plBldgsNeed list
		return( FALSE );
	}

	// and assign it to this building
	pTruck->ClearParam();
	WORD wStatus = 0;
	wStatus |= CAI_IN_USE;
	pTruck->SetStatus( wStatus );			// truck is now assigned
	pTruck->SetDataDW( pCAIBldg->GetID() ); // id of building needing
	

	BOOL bAssigned = FALSE;
	BOOL bSourceFound = TRUE;
	int iBestDist = 0xFFFF, iDist;
	int iFirstDest = CMaterialTypes::num_types;
	int iCapacity = GetMaterialCapacity(pTruck);
	int iQtyToGet;

	// for each commodity possibly needed by this building
	// count how many items are needed by this building
	int iItems = 0;
	int iMinLoadQty = iCapacity;
	for( int i=0; i<CMaterialTypes::num_types; ++i )
	{
		// this commodity is needed
		if( pCAIBldg->GetParam(i) )
			iItems++;
	}
	if( iItems )
	{
		// if only one item then cut capacity so there is
		// some room left on this truck
		//if( iItems == 1 )
		//	iItems++;
		iMinLoadQty = iCapacity / iItems;
	}

	// but only allow for those buildings that need continous
	//if( !BuildingNeedsAlways(pCAIBldg) )
	//	iMinLoadQty = 0;

	int iMatNotFound = CMaterialTypes::num_types;
	// for each commodity possibly needed by this building
	for( i=0; i<CMaterialTypes::num_types; ++i )
	{
		// this commodity is needed
		if( pCAIBldg->GetParam(i) )
		{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::FindTransport() player %d building %ld type %d needs %d of material %d ",
m_iPlayer, pCAIBldg->GetID(), pCAIBldg->GetTypeUnit(), pCAIBldg->GetParam(i), i );
#endif
			// get pointer to the building nearest to this
			// building with the material (and does not 
			// also need it) and return it
			CAIUnit *pBldgSource = GetNearestSource( pTruck,
				i, pCAIBldg, &iDist, hexExit.X(), hexExit.Y() );

			// if there is no source for this material
			// there may be a building under construction
			// that will be
			if( pBldgSource == NULL )
			{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"unable to find source for material %d ", i );
#endif
				bSourceFound = FALSE;
				iMatNotFound = i;
				continue;
			}

			// only sources "found" get to this point
			// find the closest source building to be 1st dest
			if( iDist && iDist < iBestDist )
			{
				iBestDist = iDist;
				iFirstDest = i;
			}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"source for %d of material %d found in player %d building %ld ", 
pCAIBldg->GetParam(i), i, m_iPlayer, pBldgSource->GetID() );
#endif
			// set flag that the truck will get at least
			// one of the needed materials
			bAssigned = TRUE;

			// update the truck's WORD params with quantity 
			// needed, at the offset of the material needed
			//iQtyToGet = max( iMinLoadQty, pCAIBldg->GetParam(i) );
			iQtyToGet = min( iMinLoadQty, pCAIBldg->GetParam(i) );
			if( !iMinLoadQty )
			{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"iMinLoadQty=0,  iCapacity=%d,  iItems=%d", iCapacity, iItems );
#endif
				iQtyToGet = pCAIBldg->GetParam(i);
			}

			pTruck->SetParam( i, iQtyToGet );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"truck %d/%ld set to pick up %d of material %d from player %d building %ld ", 
pTruck->GetOwner(), pTruck->GetID(), pTruck->GetParam(i), i, 
pBldgSource->GetOwner(), pBldgSource->GetID() );
#endif

			// update the truck with the dwID of the building
			// that is a source for the material again using
			// offset of the material needed
			pTruck->SetParamDW( i, pBldgSource->GetID() );

			// and record truck 
			// at the building that needs it
			pCAIBldg->SetParamDW( i, pTruck->GetID() );

			// force building priority to 0 on any material
			// assignment, and if other materials are still
			// unassigned, they will be ided next time
			pCAIBldg->SetParam( CAI_UNASSIGNED, 0 );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::player %d transport truck %ld assigned to building %ld ", 
m_iPlayer, pTruck->GetID(), pCAIBldg->GetID() );
#endif
		}
	}

	// one material had no source yet, so make sure building priority
	// is reset so that the outer loop will break, and it will be
	// re-assigned priority on the next pass
	//if( !bSourceFound )
	//	pCAIBldg->SetParam( CAI_UNASSIGNED, 0 );

	// no sources were available so unassign the truck
	if( !bAssigned )
	{
		pTruck->SetStatus( 0 );	// truck is no longer assigned
		pTruck->SetDataDW( 0 );
		pTruck->ClearParam();

		// GAME MESSAGE needed - unable to pick up materials
		if( iMatNotFound < CMaterialTypes::num_types )
			theGame.Event( EVENT_HPR_NOPICKUP, EVENT_BAD, iMatNotFound );
		else // invalid material
			theGame.Event( EVENT_HPR_NOPICKUP, EVENT_BAD );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::player %d building %ld had no sources, truck %ld unassigned \n", 
m_iPlayer, pCAIBldg->GetID(), pTruck->GetID() );
#endif
		// keep building needing in the m_plBldgsNeed list
		return FALSE;
	}
	else // remove pointer to truck from trucks available list
		m_plTrucksAvailable->RemoveUnit( pTruck->GetID(), FALSE );

	// set the first destination for the truck
	if( iFirstDest < CMaterialTypes::num_types )
	{
		CAIUnit *pDestBldg = 
			m_plUnits->GetUnitNY( pTruck->GetParamDW(iFirstDest) );
		
		// get location of this destination
		CHexCoord hex;
		if( !GetBldgExit(pDestBldg->GetID(),hex) )
		{
			pTruck->SetStatus( 0 );	// truck is no longer assigned
			pTruck->SetDataDW( 0 );
			pTruck->ClearParam();
			// put truck back in list
			if( m_plTrucksAvailable->GetUnitNY(pTruck->GetID()) == NULL )
				m_plTrucksAvailable->AddTail( (CObject *)pTruck );

			// stay in m_plBldgsNeed list
			return FALSE;
		}

		pTruck->SetParam(CAI_ROUTE_X,hex.X()); // save the destination
		pTruck->SetParam(CAI_ROUTE_Y,hex.Y()); // hex for this source

		// this considers that a path to the current source destination
		// needs a ship, to be reached and if so, direct the truck
		// to a pickup point and a ship to pick it up
		ConsiderLandWater( pTruck, hex );

		// pTruck may need to go to a pickup point instead of the
		// hex of the source building
		SetDestination( pTruck->GetID(), hex );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d transport truck %ld enroute to source building %ld \n", 
		m_iPlayer, pTruck->GetID(), pDestBldg->GetID() );
#endif

	}

	// remove from m_plBldgsNeed list
	return TRUE;
}

		// need to consider if the truck can reach the destination
		// on its own or if it needs a transport ship for water travel
		// and so if the truck can get there then hex is destination
		// otherwise the ship and truck must meet based on the type
		// of ship available (landing_craft meet a closest point to
		// ship and truck AND light_cargo meets only at seaport)

		// run a road path from the truck to the building location
		// allowing water

		// if there is water in the path, then that means we need
		// a transport ship to help with the path.  The transport
		// should move the truck from the load point, to the point 
		// in the path that is the closest to the destination

		// if a transport ship is needed, find first on landing_craft
		// and then on light_cargo and take the nearest ship that is
		// not already assigned.  Assign with the dwID of the truck
		// in the GetParamDW()

		// not necessarily, because the road path is a direct path
		// that would cross water, even if there was a land path
		//
		// so that means run it first to get a path and if not
		// able to, run it again allowing water and see if path found

		// how does the loading hex get determined?

		// what happens when one or both are attacked?

		// based on ship type, route the truck to a load point and the
		// ship to corresponding pickup point.  both units must
		// wait upon arrival due to speed differences.

		// both units must be left alone by router until a veh_dest

		// if transport is needed, the truck must be flagged to indicate
		// that upon arrival, it is awaiting or loading to a ship


//
// determine if the path from the pTruck to the hex is such that
// a transport is needed to travel the water portion of a land/water
// path and return TRUE if needed and FALSE if not
//
BOOL CHPRouter::NeedsTransport( CAIUnit *pTruck, CHexCoord& hex )
{
	// get current location of truck
	CHexCoord hexVeh;
	if( !GetVehicleHex( pTruck->GetID(), hexVeh ) )
		return FALSE;

	if( hexVeh == hex )
		return FALSE;

	int iPathLen = 0;
	BOOL bNeedShip = FALSE;

	// this will get a true path for the truck, meaning that
	// the path is possibly land only, but circuitios in many steps
	// which could mean the path goes around water, but could also
	// be returned without reaching the destination, indicating an
	// intermediate destination was determined, which could be by
	// a water obstacle or by length of path

	//
	// GetPath() parameter usage is as follows:
	//
	// CVehicle *pVehicle - NULL means that iVehType is required
	// CHexCoord& hexFrom - current location
	// CHexCoord& hexTo - destination location
	// int& iPathLen - length of array returned
	// int iVehType - required if pVehicle is NULL, ignored otherwise
	// BOOL bVehBlock - default (FALSE) means that path goes thru
	//                  vehicles, TRUE means vehicles will block
	// BOOL bDirectPath - default (FALSE) means that GetPath() will
	//                    seek an optimum path, TRUE means that it
	//                    will return upon reaching the destination
	//                    on its first attempt
	//
	//CHexCoord *GetPath( CVehicle *pVehicle,
	//	CHexCoord& hexFrom, CHexCoord& hexTo, 
	//	int& iPathLen, int iVehType = 0, 
	//	BOOL bVehBlock = FALSE, BOOL bDirectPath = FALSE );

	CHexCoord *pPath = thePathMgr.GetPath( 
		NULL, hexVeh, hex, iPathLen, pTruck->GetTypeUnit(), FALSE, TRUE );
	if( pPath != NULL )
	{
		// check last step
		if( iPathLen )
			iPathLen--;
		// if we did not get to the destination then either the path does
		// need to cross water or it was too long, either way do another
		// test this time using GetRoadPath()
		CHexCoord *pHex = &pPath[iPathLen];
		if( pHex->X() != hex.X() ||
			pHex->Y() != hex.Y() )
			bNeedShip = TRUE;

		delete [] pPath;
	}
	else
		bNeedShip = TRUE;

	if( !bNeedShip )
		return FALSE;

	// if still here, first try did not reach dest
	// so then now try for a path with thePathMap
	iPathLen = 0;
	bNeedShip = FALSE;

	//CHexCoord *CPathMap::GetRoadPath( 
	//	CHexCoord& hexFrom, CHexCoord& hexTo, 
	//	int& iPathLen, WORD *pMap, 
	//	BOOL bAllowWater=FALSE, BOOL bRiverCrossing=TRUE );

	pPath = thePathMap.GetRoadPath( 
		hexVeh, hex, iPathLen, NULL, TRUE, TRUE );
	if( pPath != NULL )
	{
		CHexCoord hexGame;
		CHex *pGameHex;

		for( int i=0; i<iPathLen; ++i )
		{
			CHexCoord *pHex = &pPath[i];
			hexGame.X( pHex->X() );
			hexGame.Y( pHex->Y() );
			pGameHex = theMap.GetHex( hexGame );
			//if( pGameHex->IsWater() )
			if( pGameHex->GetType() == CHex::ocean ||
				pGameHex->GetType() == CHex::lake )
			{
				bNeedShip = TRUE;
				break;
			}
		}
		delete [] pPath;
	}
	else
		bNeedShip = TRUE;

	return( bNeedShip );
}

//
// the passed truck needs to get to hex, and so check for a land/water/land
// path required, and if so, then find the nearest seaport to the truck
// and the nearest unassigned transport to that seaport, and send them
// both to the seaport
//
void CHPRouter::ConsiderLandWater( CAIUnit *pTruck, CHexCoord& hex )
{
	// path does not need a transport either
	if( !NeedsTransport( pTruck, hex ) )
		return;

	// can't do it anyway, as there are not enough seaports
	if( m_wSeaportCnt < 2 )
	{
		// need to send game an error code for this
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::ConsiderLandWater() player %d truck %ld ",
	m_iPlayer, pTruck->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"not enough seaports %d \n", m_wSeaportCnt );
#endif // DEBUG_OUTPUT_ROUTE_HP

		// GAME MESSAGE needed - not enough seaports
		theGame.Event( EVENT_HPR_SEAPORTS, EVENT_BAD );

		return;
	}

	// okay, we need a ship, can we find one?
	CAIUnit *pShip = GetNearestShip( pTruck );
	if( pShip == NULL )
	{
		// need to send game an error code for this
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::ConsiderLandWater() player %d truck %ld ",
	m_iPlayer, pTruck->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"no transport ship available \n" );
#endif // DEBUG_OUTPUT_ROUTE_HP

		// GAME MESSAGE needed - need a cargo ship
		theGame.Event( EVENT_HPR_CARGOSHIP, EVENT_BAD );

		// shit, we have a truck assigned, and possibly
		// loaded with materials for the assigned building
		// but it can't reach the building for lack of a 
		// ship, so it must move to the nearest seaport
	    // and wait for one to become available

		// return;
	}

	// get current location of ship
	CHexCoord hexShip;
	if( pShip != NULL )
		GetVehicleHex( pShip->GetID(), hexShip );

	// now get the truck's location
	CHexCoord hexVeh;
	GetVehicleHex( pTruck->GetID(), hexVeh );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::ConsiderLandWater() player %d truck %ld ",
	m_iPlayer, pTruck->GetID() );
	if( pShip != NULL )
		logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
		"trying to assign ship %ld \n", pShip->GetID() );
	else
		logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
		"no ship assigned \n" );
#endif // DEBUG_OUTPUT_ROUTE_HP

	// find the nearest seaport to the truck that the ship
	// can reach, and if found, select it as the destination
	// for both units and sent veh_goto messages for them
	DWORD dwSeaport;
	//CBuilding *pBldg;
	CHexCoord hexBest = hexVeh;
	CHexCoord hexBldg;
	int iBest = 0xFFFE;
	int iDist;
	int iPathLen = 0;
	CHexCoord *pPath = NULL;
	CHexCoord *pPathHex = NULL;
	

   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
    	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// looking for a seaport different than the one we are at
			if( pUnit->GetType() == CUnit::building &&
				pUnit->GetTypeUnit() == CStructureData::seaport )
			{
				// need to try to get both the ship and the truck
				// to the seaport via a path
				BOOL bCanGetThere = FALSE;

				if( pShip != NULL )
				{
					// change in hexBldg will mean it was found
					GetShipHex( pUnit->GetID(), hexBldg );
				// run a path out to the candidate seaport from this seaport
				// using the ship as a vehicle
				// CHexCoord *GetPath( CVehicle *pVehicle,
				// CHexCoord& hexFrom, CHexCoord& hexTo, 
				// int& iPathLen, int iVehType = 0, 
				// BOOL bVehBlock = FALSE, BOOL bDirectPath = FALSE );
					if( hexShip != hexBldg )
					{
	 					pPath = thePathMgr.GetPath( 
						NULL, hexShip, hexBldg, iPathLen, 
						pShip->GetTypeUnit(),
						FALSE, TRUE );
						if( pPath != NULL )
						{
							// check last step
							if( iPathLen )
								iPathLen--;

							pPathHex = &pPath[iPathLen];
							if( pPathHex->X() == hexBldg.X() &&
								pPathHex->Y() == hexBldg.Y() )
								bCanGetThere = TRUE;

							delete [] pPath;
						}
					}
					else
						bCanGetThere = TRUE;

					if( !bCanGetThere )
						continue;
				}

				GetBldgExit( pUnit->GetID(), hexBldg );

				// at this time  bCanGetThere == FALSE if there is no ship
				// but may be TRUE if there is and it can get there
				// now, can the truck get to its destination from that seaport?
				if( hexVeh != hexBldg )
				{
					bCanGetThere = FALSE;
					
					iPathLen = 0;
	 				pPath = thePathMgr.GetPath( 
					NULL, hexVeh, hexBldg, iPathLen, pTruck->GetTypeUnit(),
					FALSE, TRUE );
					if( pPath != NULL )
					{

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::ConsiderLandWater() player %d truck %ld ",
	m_iPlayer, pTruck->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"path returned of %d steps from %d,%d to %d,%d ",
	iPathLen, hexVeh.X(), hexVeh.Y(), hexBldg.X(), hexBldg.Y() );
#endif // DEBUG_OUTPUT_ROUTE_HP

					// check last step
					if( iPathLen )
						iPathLen--;

					pPathHex = &pPath[iPathLen];

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"last step of path is %d,%d \n",
	pPathHex->X(), pPathHex->Y() );
#endif // DEBUG_OUTPUT_ROUTE_HP

					if( pPathHex->X() == hexBldg.X() &&
						pPathHex->Y() == hexBldg.Y() )
						bCanGetThere = TRUE;

					delete [] pPath;
					}
				}
				else // truck is already at the seaport
				{
					iBest = 0;
					hexBest = hexBldg;
					dwSeaport = pUnit->GetID();
				}

				if( !bCanGetThere )
					continue;

				iDist = theMap.GetRangeDistance( hexVeh, hexBldg );
				if( iDist && iDist < iBest )
				{
					iBest = iDist;
					hexBest = hexBldg;
					dwSeaport = pUnit->GetID();
				}
			}
		}
	}

	// no seaports found that truck and ship can get to
	if( iBest == 0xFFFE )
	{
		// need to send game an error code for this
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::ConsiderLandWater() player %d truck %ld ",
	m_iPlayer, pTruck->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"can't reach seaports by truck or ship \n" );
#endif // DEBUG_OUTPUT_ROUTE_HP

		// GAME MESSAGE needed - unable to reach seaport
		theGame.Event( EVENT_HPR_NOREACH, EVENT_BAD );

		return;
	}

	WORD wStatus = 0;
	if( pShip != NULL )
	{
	// but what if the ship can't get there?
	//
	// for the ship unit:
	// pUnit->GetParamDW(CAI_LOC_X) = truck to load
	// pUnit->GetParamDW(CAI_PREV_X) = seaport to load at
	// pUnit->GetParamDW(CAI_DEST_X) = seaport to unload at
	//
		pShip->ClearParam();
		pShip->SetParamDW(CAI_LOC_X, pTruck->GetID() );
		pShip->SetParamDW(CAI_PREV_X, dwSeaport );
		
		wStatus |= CAI_IN_USE;
		pShip->SetStatus( wStatus );	// ship is now assigned

		// remove ship from available ship list
		m_plShipsAvailable->RemoveUnit( pShip->GetID(), FALSE );

		// hexBest is the pickup point for the truck,
		// but the ship must go in a different entrance
		hexBldg = hexBest;
		GetShipHex( dwSeaport, hexBldg );
		SetDestination( pShip->GetID(), hexBldg );

		// record that this truck has a ship assigned already
		wStatus = pTruck->GetStatus();
		if( wStatus & CAI_TRUCK_WAITING )
		{
			wStatus ^= CAI_TRUCK_WAITING;
			pTruck->SetStatus( wStatus );
		}
	}
	else // record that this truck needs a ship
	{
		wStatus = pTruck->GetStatus();
		if( !(wStatus & CAI_TRUCK_WAITING) )
		{
			wStatus |= CAI_TRUCK_WAITING;
			pTruck->SetStatus( wStatus );
		}
	}

	hex = hexBest;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::ConsiderLandWater() player %d truck %ld ",
		m_iPlayer, pTruck->GetID() );
	if( pShip != NULL )
	{
		logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
		"and ship %ld now enroute to %d,%d for loading \n", 
		pShip->GetID(), hexBest.X(), hexBest.Y() );
	}
#endif // DEBUG_OUTPUT_ROUTE_HP
}

//
// the passed truck has arrived at a building (data in pHex has dwID
// of building and the location just arrived at), which may be a seaport
// (or it could be a passed ship)
// to which the truck has been sent to be loaded on a transport for
// travel on a land/water/land route to the seaport nearest the hex
// in CAIUnit::params(CAI_ROUTE_X/Y), so if a transport is at the seaport,
// then issue a load_carrier message to load the truck on the transport
//
BOOL CHPRouter::ConsiderLandWater( CAIUnit *pUnit, CAIHex *pHex )
{
	// pHex->m_dwUnitID - is id of building that truck just arrived at
	//
	CAIUnit *pSeaport = 
		m_plUnits->GetUnitNY( pHex->m_dwUnitID );
	if( pSeaport == NULL )
		return FALSE;
	if( pSeaport->GetType() != CUnit::building )
		return FALSE;
	if( pSeaport->GetTypeUnit() != CStructureData::seaport )
		return FALSE;

	// should only continue if pSeaport is a seaport

	// look for light_cargo that has this trucks dwID in
	// pShip->GetParamDW(CAI_LOC_X) and this buildings dwID
	// in pShip->GetParamDW(CAI_PREV_X) and the ship's hex
	// is equal to pHex->m_iX/Y
	CAIUnit *pShip = NULL;
	CAIUnit *pTruck = NULL;
	if( IsTruck(pUnit->GetID()) )
		pTruck = pUnit;
	else if( IsShip(pUnit->GetID()) )
		pShip = pUnit;
	else
		return( FALSE );

	if( pShip == NULL && pTruck == NULL )
		return( FALSE );

	// if this is a truck and its list of sources contain this
	// seaport, then this could be a pick up instead
	if( pTruck != NULL )
	{
		BOOL bPickUp = FALSE;
		for( int i=0; i<CMaterialTypes::num_types; ++i )
		{
			if( !pTruck->GetParamDW(i) )
				continue;
			if( pTruck->GetParamDW(i) == 
				pSeaport->GetID() )
				bPickUp = TRUE;
		}
		if( bPickUp )
			return( FALSE );
	}

	// since dave chose to be inconsistent in handling vehicle unloads
	// we now must consider that the truck has arrived at a seaport
	// under several considerations:
	//
	// if this is a seaport and the truck was just disgorged into it
	// then it will be flagged, and if it is, unflag it and get it out
	// of the fucking seaport so the router can get control of it
	//
	if( pTruck != NULL && pShip == NULL )
	{
		CHexCoord hexDest;
		GetVehicleHex( pTruck->GetID(), hexDest );

		// do only if truck is cargo and has actually arrived here
		WORD wStatus = pTruck->GetStatus();
		if( (wStatus & CAI_IS_CARGO) && !IsVehicleCargo(pTruck->GetID()) &&
			(hexDest.X() == pHex->m_iX && hexDest.Y() == pHex->m_iY) )
		{
		// turn off status as cargo
		wStatus ^= CAI_IS_CARGO;
		pTruck->SetStatus( wStatus );

		// find staging hex outside the seaport
		if( GetStagingHex( pTruck, pSeaport, hexDest ) )
			SetDestination(pTruck->GetID(), hexDest); // and go to it

		return( TRUE );
		}
	}


	if( m_plUnits->GetCount() )
	{
   		POSITION pos = m_plUnits->GetHeadPosition();
   		while( pos != NULL )
   		{   
       		CAIUnit *paiUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       		if( paiUnit != NULL )
       		{
       			ASSERT_VALID( paiUnit );

				if( paiUnit->GetType() != CUnit::vehicle )
					continue;

				// could be looking for a ship
				if( pShip == NULL )
				{
					if( IsShip(paiUnit->GetID()) &&
						paiUnit->GetParamDW(CAI_LOC_X) == pTruck->GetID() &&
						paiUnit->GetParamDW(CAI_PREV_X) == pSeaport->GetID() )
					{
						pShip = paiUnit;
						break;
					}
				}

				// could be looking for a truck
				if( pTruck == NULL )
				{
					if( IsTruck(paiUnit->GetID()) &&
						pShip->GetParamDW(CAI_LOC_X) == paiUnit->GetID() )
					{
						pTruck = paiUnit;
						break;
					}
				}
			}
		}
	}

	CHexCoord hexSeaport;
	// if we have a null truck, then that means this ship is
	// no longer assigned
	if( pTruck == NULL )
	{
		// there is no seaport that we can reach 
		// so unassign ship
		UnAssignShip(pShip);

		//hexSeaport.X( 0xFFFE );
		//hexSeaport.Y( 0xFFFE );
		
		if( GetStagingHex(pShip, pSeaport, hexSeaport) )
			SetDestination( pShip->GetID(), hexSeaport );

		return( FALSE );
	}

	// if not found then try again to find a ship, 
	// but force the truck to wait with TRUE
	if( pShip == NULL )
	{
		// okay, we need a ship, can we find one?
		pShip = GetNearestShip( pTruck );
		if( pShip != NULL )
		{
			pShip->ClearParam();
			pShip->SetParamDW(CAI_LOC_X, pTruck->GetID() );
			pShip->SetParamDW(CAI_PREV_X, pSeaport->GetID() );
			WORD wStatus = 0;
			wStatus |= CAI_IN_USE;
			pShip->SetStatus( wStatus );	// ship is now assigned

			// remove ship from available ship list
			m_plShipsAvailable->RemoveUnit( pShip->GetID(), FALSE );

			// hexSeaport is the pickup point
			if( GetShipHex(pSeaport->GetID(), hexSeaport) )
				SetDestination( pShip->GetID(), hexSeaport );

			// record that this truck has a ship assigned already
			wStatus = pTruck->GetStatus();
			if( (wStatus & CAI_TRUCK_WAITING) )
			{
				wStatus ^= CAI_TRUCK_WAITING;
				pTruck->SetStatus( wStatus );
			}
		}
		return TRUE	;
	}
	
	// get current location of the truck
	CHexCoord hexTruck;
	GetVehicleHex( pTruck->GetID(), hexTruck );
	GetBldgExit( pSeaport->GetID(), hexSeaport );

	// and determine if truck is at the seaport
	// and if it is not then leave, but force wait with TRUE
	if( hexTruck != hexSeaport )
		return TRUE	;

	// get current location of ship
	CHexCoord hexShip;
	GetVehicleHex( pShip->GetID(), hexShip );

	// get ship exit location of the seaport
	GetShipHex( pSeaport->GetID(), hexSeaport );

	// and determine if ship is at the seaport
	// and if it is not then leave, but force wait with TRUE
	if( hexShip != hexSeaport )
		return TRUE	;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::ConsiderLandWater() player %d truck %ld and ship %ld ",
		m_iPlayer, pTruck->GetID(), pShip->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"are at seaport %ld for loading at %d,%d \n", 
		pSeaport->GetID(), hexSeaport.X(), hexSeaport.Y() );
#endif // DEBUG_OUTPUT_ROUTE_HP

	// if still here, we have found the ship assigned to the truck
	// and it and the truck are at the seaport, so find the seaport
	// that is nearest the pTruck->GetParam(CAI_ROUTE_X/Y) and record
	// that in the pShip, issue a load message to load the pTruck onto
	// the pShip and send the pShip to the other seaport
	CHexCoord hexDest( 
		pTruck->GetParam(CAI_ROUTE_X), pTruck->GetParam(CAI_ROUTE_Y) );

	CHexCoord *pPath = NULL;
	int iPathLen = 0;
	CHexCoord *pPathHex = NULL;

	// we are done with the seaport the truck and ship are at, so
	// get ready to find the seaport they need to get to
	pSeaport = NULL;
	int iNearest = 0xFFFE;
	
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
    	CAIUnit *paiBldg = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( paiBldg != NULL )
       	{
       		ASSERT_VALID( paiBldg );

			// looking for a seaport different than the one we are at
			if( paiBldg->GetType() == CUnit::building &&
				paiBldg->GetTypeUnit() == CStructureData::seaport &&
				paiBldg->GetID() != pHex->m_dwUnitID )
			{
				CHexCoord hexBldg;
				GetShipHex( paiBldg->GetID(), hexBldg );
				BOOL bCanGetThere = FALSE;

				// run a path out to the candidate seaport from this seaport
				// using the ship as a vehicle
	 			pPath = thePathMgr.GetPath( 
					NULL, hexSeaport, hexBldg, iPathLen, pShip->GetTypeUnit(),
					FALSE, TRUE );
				if( pPath != NULL )
				{
					// check last step
					if( iPathLen )
						iPathLen--;
					// if we did not get to the destination then either the path does
					// need to cross water or it was too long, either way do another
					// test this time using GetRoadPath()
					pPathHex = &pPath[iPathLen];
					if( pPathHex->X() == hexBldg.X() &&
						pPathHex->Y() == hexBldg.Y() )
						bCanGetThere = TRUE;

					delete [] pPath;
				}

				if( !bCanGetThere )
					continue;

				// now, can the truck get to its destination from that seaport?
				GetBldgExit( paiBldg->GetID(), hexBldg );
				bCanGetThere = FALSE;
				iPathLen = 0;
	 			pPath = thePathMgr.GetPath( 
					NULL, hexBldg, hexDest, iPathLen, pTruck->GetTypeUnit(),
					FALSE, TRUE );
				if( pPath != NULL )
				{
					// check last step
					if( iPathLen )
						iPathLen--;
					// if we did not get to the destination then either the path does
					// need to cross water or it was too long, either way do another
					// test this time using GetRoadPath()
					pPathHex = &pPath[iPathLen];
					if( pPathHex->X() == hexDest.X() &&
						pPathHex->Y() == hexDest.Y() )
						bCanGetThere = TRUE;

					delete [] pPath;
				}

				if( hexBldg == hexDest )
					bCanGetThere = TRUE;

				if( !bCanGetThere )
					continue;

				// now, how far is it from where the truck has to go?
				int iDist = theMap.GetRangeDistance( hexDest, hexBldg );
				if( iDist >= 0 && iDist < iNearest )
				{
					pSeaport = paiBldg;
					iNearest = iDist;
				}
			}
		}
	}

	if( pSeaport == NULL )
	{
		// there is no seaport that we can reach 
		// so unassign ship
		UnAssignShip(pShip);

		//hexDest.X( 0xFFFE );
		//hexDest.Y( 0xFFFE );
		
		if( GetStagingHex(pShip, pSeaport, hexDest) )
			SetDestination( pShip->GetID(), hexDest );

		// BUGBUG should report this to the HP somehow

		// GAME MESSAGE needed - unable to reach seaport
		theGame.Event( EVENT_HPR_NOREACH, EVENT_BAD );
	}
	else
	{
		// load truck onto ship
		LoadUnit( pShip->GetID(), pTruck->GetID() );
		// flag ship as cargo
		WORD wStatus = pTruck->GetStatus();
		wStatus |= CAI_IS_CARGO;
		pTruck->SetStatus( wStatus );
		// send ship to other seaport
		if( GetShipHex(pSeaport->GetID(), hexDest) )
			SetDestination( pShip->GetID(), hexDest );
		// record destination seaport for later use
		pShip->SetParamDW( CAI_DEST_X,pSeaport->GetID() );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::ConsiderLandWater() player %d truck %ld and ship %ld ",
		m_iPlayer, pTruck->GetID(), pShip->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"are enroute to 2nd seaport %ld for unloading at %d,%d \n", 
		pSeaport->GetID(), hexDest.X(), hexDest.Y() );
#endif // DEBUG_OUTPUT_ROUTE_HP

	}

	return TRUE	;
}

void CHPRouter::GetCHexData( CAIHex *pHex )
{
	ASSERT_VALID (this);

	// mark the hex as unknown
	pHex->m_iUnit = 0xFFFF;
	pHex->m_dwUnitID = (DWORD)0;
	pHex->m_cTerrain = (BYTE)0xFF;
	CHexCoord getHex( pHex->m_iX, pHex->m_iY );

	EnterCriticalSection (&cs);

	CHex *pGameHex = theMap.GetHex( getHex );

	// known hex means return is not NULL
	if( pGameHex != NULL )
	{
		// extract the terrain type from low nibble
		const int iType = pGameHex->GetType();
		pHex->m_cTerrain = (BYTE)iType;

		BYTE bUnits = pGameHex->GetUnits();
		// there may be units in this CHex
		if( bUnits != 0 )
		{
			// it is a hex of a building
			if( bUnits & CHex::bldg )
			{
				CBuilding *pBldg =
					theBuildingHex.GetBuilding( getHex );
				if( pBldg != NULL )
				{
					// NOTE: this does not identify the type
					// of building/vehicle unit, but indicates
					// that the unit is a building or vehicle
					pHex->m_iUnit = pBldg->GetUnitType();
					pHex->m_dwUnitID = pBldg->GetID();
				}
			}
		}
	}
	LeaveCriticalSection (&cs);
}

//
// returns an indication of the transported status of this vehicle
//
BOOL CHPRouter::IsVehicleCargo( DWORD dwID )
{
	BOOL bRet = FALSE;
	EnterCriticalSection (&cs);
	CVehicle *pVehicle = 
		theVehicleMap.GetVehicle( dwID );
	if( pVehicle != NULL )
	{
		if( pVehicle->GetTransport() != NULL )
			bRet = TRUE;
	}
	LeaveCriticalSection (&cs);
	return( bRet );
}

//
// loads a class global array for the materials carried by this vehicle
//
void CHPRouter::GetVehicleCargo( DWORD dwID )
{
	EnterCriticalSection (&cs);
	CVehicle *pVehicle = 
		theVehicleMap.GetVehicle( dwID );
	if( pVehicle != NULL )
	{
		for( int i=0; i<CMaterialTypes::num_types; ++i )
			m_iStore[i] = pVehicle->GetStore(i);
	}
	LeaveCriticalSection (&cs);
}
//
// gets the game's view of the current location of this vehicle
//
BOOL CHPRouter::GetVehicleHex( DWORD dwID, CHexCoord& hex )
{
	BOOL bFoundHex = FALSE;
	EnterCriticalSection (&cs);
	CVehicle *pVehicle = 
		theVehicleMap.GetVehicle( dwID );
	if( pVehicle != NULL )
	{
		hex = pVehicle->GetHexHead();
		bFoundHex = TRUE;
	}
	LeaveCriticalSection (&cs);
	return( bFoundHex );
}

//
// gets the game's view of the current ship exit hex of this building
//
BOOL CHPRouter::GetShipHex( DWORD dwID, CHexCoord& hex )
{
	BOOL bFoundHex = FALSE;
	EnterCriticalSection (&cs);
	CBuilding *pBldg = theBuildingMap.GetBldg( dwID );
	if( pBldg != NULL && pBldg->GetData()->HasShipExit() )
	{
		hex = pBldg->GetShipHex();
		bFoundHex = TRUE;
	}
	LeaveCriticalSection (&cs);
	return( bFoundHex );
}

//
// gets the game's view of the current exit hex of this building
//
BOOL CHPRouter::GetBldgExit( DWORD dwID, CHexCoord& hex )
{
	BOOL bFoundHex = FALSE;
	EnterCriticalSection (&cs);
	CBuilding *pBldg = theBuildingMap.GetBldg( dwID );
	if( pBldg != NULL )
	{
		hex = pBldg->GetExitHex();
		bFoundHex = TRUE;
	}
	LeaveCriticalSection (&cs);
	return( bFoundHex );
}

//
// return the carrying capacity found for the unit indicated
//
int CHPRouter::GetMaterialCapacity( CAIUnit *paiUnit )
{
	int iQty = 0;

	EnterCriticalSection (&cs);

	// this code is being retained to allow future versions
	// to have buildings with a max capacity.  current tLP
	// allows unlimited storage at a building
#if 0
	if( paiUnit->GetType() == CUnit::building )
	{
		CBuilding *pBldg = theBuildingMap.GetBldg( paiUnit->GetID() );
		if( pBldg != NULL )
		{
			// BUGBUG GetMaxMaterials() is gone for buildings
			int iMax = pBldg->GetData()->GetMaxMaterials();
			int iStore = pBldg->GetTotalStore();
			iQty =  iMax - iStore;
		}
	}
#endif

	if( paiUnit->GetType() == CUnit::vehicle )
	{
		CVehicle *pVehicle = 
			theVehicleMap.GetVehicle( paiUnit->GetID() );
		if( pVehicle != NULL )
		{
			iQty = pVehicle->GetData()->GetMaxMaterials() - 
				pVehicle->GetTotalStore();
			if( iQty < 0 )
				iQty = 0;
			else
			{
				if( iQty > pVehicle->GetData()->GetMaxMaterials() )
					iQty = pVehicle->GetData()->GetMaxMaterials();
			}
		}
	}
		
	LeaveCriticalSection (&cs);

	return( iQty );
}
//
// create a list of the ships that are currently available to the
// router that can transport trucks over land/sea/land routes
//
void CHPRouter::GetShipsAvailable( void )
{
	ASSERT_VALID( m_plUnits );

	if( m_plShipsAvailable == NULL )
		return;

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
			if( pUnit->GetType() == CUnit::building )
				continue;

			// these vehicles are assigned
			if( (pUnit->GetStatus() & CAI_IN_USE) )
				continue;

			// these vehicles are in combat
			if( (pUnit->GetStatus() & CAI_IN_COMBAT) )
				continue;

			// look only at vehicles that are SHIPS
			if( !IsShip( pUnit->GetID() ) )
				continue;

			// and not currently carried by another unit
			if( IsVehicleCargo( pUnit->GetID() ) )
				continue;

			m_plShipsAvailable->AddTail( (CObject *)pUnit );
		}
	}
}


//
// create a list and 
// find any unassigned truck and add it to the list
// and return it
//
void CHPRouter::GetTrucksAvailable( void )
{
	ASSERT_VALID( m_plUnits );

	if( m_plTrucksAvailable == NULL )
		return;

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
			if( pUnit->GetType() == CUnit::building )
				continue;

			// these vehicles are assigned
			if( (pUnit->GetStatus() & CAI_IN_USE) )
				continue;

			// these vehicles are in combat
			if( (pUnit->GetStatus() & CAI_IN_COMBAT) )
				continue;

			// look only at vehicles that are TRUCKS
			if( !IsTruck( pUnit->GetID() ) )
				continue;

			// and not currently carried by another unit
			if( IsVehicleCargo( pUnit->GetID() ) )
				continue;

			if( m_plTrucksAvailable->GetUnitNY(pUnit->GetID()) == NULL )
				m_plTrucksAvailable->AddTail( (CObject *)pUnit );
		}
	}
}

//
// find the ship nearest the truck that can provide transport
// for the non-land portion of the path from the truck to its
// assigned building or source building
//
CAIUnit *CHPRouter::GetNearestShip( CAIUnit *pCAITruck )
{
	// there are no ships available
	if( !m_plShipsAvailable->GetCount() )
	{
		// but what if a ship has already been assigned to this truck?
   		POSITION pos = m_plUnits->GetHeadPosition();
   		while( pos != NULL )
   		{   
       		CAIUnit *pShip = (CAIUnit *)m_plUnits->GetNext( pos );
       		if( pShip != NULL )
       		{
       			ASSERT_VALID( pShip );

				if( pShip->GetOwner() != m_iPlayer )
					continue;

				if( pShip->GetType() == CUnit::building )
					continue;

				if( !IsShip(pShip->GetID()) )
					continue;

				if( pShip->GetParamDW(CAI_LOC_X) == 
					pCAITruck->GetID() )
					return( pShip );
			}
		}
		return( NULL );
	}

	// get current location of truck
	CHexCoord hexVeh;
	if( !GetVehicleHex( pCAITruck->GetID(), hexVeh ) )
		return( NULL );

	CAIUnit *pNearest = NULL;
	// we will look for some ship at least this close
	int iNearest = theGame.GetSideSize() + theGame.GetSideSize();

   	POSITION pos = m_plShipsAvailable->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pShip = (CAIUnit *)m_plShipsAvailable->GetNext( pos );
       	if( pShip != NULL )
       	{
       		ASSERT_VALID( pShip );

			if( !IsValidUnit( pShip ) )
			 	continue;

			// get current location of ship
			CHexCoord hexShip;
			if( !GetVehicleHex( pShip->GetID(), hexShip ) )
				continue;

			// get distance from truck
			int iDist = theMap.GetRangeDistance( hexVeh, hexShip );
			if( iDist && iDist < iNearest )
			{
				pNearest = pShip;
				iNearest = iDist;
			}
		}
	}
	return( pNearest );
}

// 
// find the truck nearest the building from the list
// carrying enough material to meet the need of the
// building, and return a pointer to it
//
CAIUnit *CHPRouter::GetNearestTruck( CAIUnit *pCAIBldg )
{
	// will be used as the destination
	CHexCoord hexBuilding;
	if( !GetBldgExit(pCAIBldg->GetID(),hexBuilding) )
		return( NULL );

	CAIUnit *pNearest = NULL;
	int iNearest = theGame.GetSideSize() + theGame.GetSideSize();

	// all units in the list should be trucks 
   	POSITION pos = m_plTrucksAvailable->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pTruck = (CAIUnit *)m_plTrucksAvailable->GetNext( pos );
       	if( pTruck != NULL )
       	{
       		ASSERT_VALID( pTruck );

			if( !IsValidUnit( pTruck ) )
			 	continue;

			// BUGBUG the truck may be dirty from a prior assignment
			// if this breakpoint fires, that means this truck is
			// unassigned, but still carries the id of prior building
			if( pTruck->GetDataDW() )
				continue;


			// also, the truck may be on a ship, and still not be assigned
			// to a building, thus it is not really available until it 
			// gets off of the ship, and then it might get off of the 
			// ship, on land that is not connected to the building, which
			// then makes it available, and land/sea/land routing should
			// get it where it needs to go

			// and not currently carried by another unit
			if( IsVehicleCargo( pTruck->GetID() ) )
				continue;

			// can't do land/sea/land routes yet, so truck must
			// be able to reach assigned building
			if( m_wSeaportCnt < 2 )
			{
				// if path needs a transport, then can't use this truck
				if( NeedsTransport( pTruck, hexBuilding ) )
					continue;
			}

			// get current location of truck
			CHexCoord hexVeh;
			if( !GetVehicleHex( pTruck->GetID(), hexVeh ) )
				continue;

			// and load up m_iStore[] with what its carrying
			GetVehicleCargo( pTruck->GetID() );

			// get dist to reach building
			int iDist = theMap.GetRangeDistance( hexVeh, hexBuilding );
				
			// adjust distance for each material 
			// needed that this truck is carrying
			for( int m=0; m<CMaterialTypes::num_types; ++m )
			{
				// consider if this material is needed
				if( pCAIBldg->GetParam(m) && iDist > 1 )
				{
					// if truck carries more than needed
					// 1/2 the distance from truck to building
					if( m_iStore[m] >=
						pCAIBldg->GetParam(m) )
						iDist /= 2;
					// else truck has some of what is needed
					else if( m_iStore[m] )
						iDist--;
				}
			}

			if( iDist && iDist < iNearest )
			{
				pNearest = pTruck;
				iNearest = iDist;
			}
		}
	}

	// there could be trucks, that have just been assigned to IDT_TRANSPORT
	// task and they have not been added to m_plTrucksAvailable

	return( pNearest );
}


//
// validate the pointer that is in the local list
// with the what it should be, as it may be dead
//
BOOL CHPRouter::IsValidUnit( CAIUnit *pUnit )
{
	ASSERT_VALID( m_plUnits );

	if( pUnit == NULL )
		return FALSE;
	CAIUnit *pValid =
		m_plUnits->GetUnit( pUnit->GetID() );
	if( pUnit == pValid )
		return TRUE;
	return FALSE;
}

// used below to figure if this is the best source
static void CheckBest ( CAIUnit * pUnit, int iDist, int iHave, CAIUnit * & pNextClosest, int & iNextDist, int & iNextBest )
{

	// no if same building
	if ( iDist == 0 )
		{
		TRAP ();
		return;
		}

	// if < 24 hexes take the max
	if ( ( iDist < 24 ) || ( iNextDist <= 0 ) )
		{
		if ( iHave > iNextBest )
			{
			iNextBest = iHave;
			iNextDist = iDist;
			pNextClosest = pUnit;
			}
		return;
		}

	int iNew = iHave / ( iDist * 2 );
	int iOld = iNextBest / ( iNextDist * 2 );
	// if == 0 then we take the closest
	if ( ( iNew == 0 ) || ( iOld == 0 ) )
		{
		if ( iDist < iNextDist )
			{
			iNextBest = iHave;
			iNextDist = iDist;
			pNextClosest = pUnit;
			}
		return;
		}

	// basically I'll travel another hex to get another 2 units
	if ( iNew > iOld )
		{
		iNextBest = iHave;
		pNextClosest = pUnit;
		}
}

//
// get CAIUnit pointer of the building nearest to this
// location with the material (and does not 
// also need it) and this quantity and return
// the pointer
//
CAIUnit *CHPRouter::GetNearestSource( CAIUnit *pTruck,
	int iMaterial, CAIUnit *pBldgNeeding, int *piDistBack, int iX, int iY )
{
	ASSERT_VALID( m_plUnits );

	CAIUnit *pClosest = NULL;
	CAIUnit *pNextClosest = NULL;
	int iNextBest = 0;
	int iBest = 0;
	int iNextDist = INT_MAX;
	int iBestDist = INT_MAX;
	int iOnHand = 0;
	int iQtyNeeded = pBldgNeeding->GetParam(iMaterial);

	CHexCoord hexNeed( iX, iY );
	CHexCoord hexMat;
	BOOL bIsConstructing = FALSE, bIsDepleted = FALSE;
	int iProduces = CStructureData::num_union_types;

   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			// turn off control check
			//if( pUnit->GetOwner() != m_iPlayer )
			//	continue;

			// determine if this is a building
			if( pUnit->GetType() != CUnit::building )
				continue;

			// this building needs that same material
			if( pUnit->GetParam(iMaterial) )
				continue;

			bIsConstructing = FALSE;
			bIsDepleted = FALSE;
			// need pBuilding->IsConstructing(); [CAI_ISCONSTRUCTING]
			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
			if( pBldg != NULL )
			{
				bIsConstructing = pBldg->IsConstructing();
				hexMat = pBldg->GetExitHex();
				// need pBuilding->GetData()->GetUnionType(); [CAI_PRODUCES]
				iProduces = pBldg->GetData()->GetUnionType();
				// record what's in storage of this material at this building
				m_iStore[iMaterial] = pBldg->GetStore(iMaterial);

				bIsDepleted = pBldg->IsFlag( CUnit::abandoned );
			}
			LeaveCriticalSection (&cs);

			// if not ours must be a farm or mine NOT under construction
			if( pUnit->GetOwner() != m_iPlayer )
				{
				if ( bIsConstructing )
					{
					TRAP ();
					continue;
					}
				if ( ( iProduces != CStructureData::UTmine ) &&
														( iProduces != CStructureData::UTfarm ) )
					{
					TRAP ();
					continue;
					}
				}

			// is this a building under construction
			if( bIsConstructing )
			{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::player %d building %ld/%d constructing %d ",
m_iPlayer, pUnit->GetID(), pUnit->GetTypeUnit(), bIsConstructing );
#endif
				continue;
			}
			// is this a depleted building with none of the material needed
			if( bIsDepleted && !m_iStore[iMaterial] )
			{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::player %d building %ld/%d depleted %d ",
m_iPlayer, pUnit->GetID(), pUnit->GetTypeUnit(), bIsDepleted );
#endif
				continue;
			}

			// if can't do land/sea/land routes yet, then truck must
			// be able to reach source building
			if( m_wSeaportCnt < 2 )
			{
				// if path needs a transport, then can't use this truck
				if( NeedsTransport( pTruck, hexMat ) )
					continue;
			}

			// then based on what this building does
			// determine if it could provide the material
			if( iProduces == CStructureData::UTmaterials )
			{
				// get CBuildMaterials data
				EnterCriticalSection (&cs);
				CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					CBuildMaterials const *pData =
						pBldg->GetData()->GetBldMaterials();
					if( pData != NULL )
					{
						m_iNeeds[iMaterial] = pData->GetInput(iMaterial);
						m_iOutput[iMaterial] = pData->GetOutput(iMaterial);
					}
				}
				LeaveCriticalSection (&cs);

				// does this building need the same material for
				// doing its production, and has less than needed?
				if( m_iNeeds[iMaterial] > m_iStore[iMaterial] )
				{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::player %d building %ld/%d needs material %d ",
m_iPlayer, pUnit->GetID(), pUnit->GetTypeUnit(), iMaterial );
#endif
					continue;
				}

				// does this building produce the material needed
				if( m_iOutput[iMaterial] )
				{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::player %d building %ld produces %d of material %d ",
m_iPlayer, pUnit->GetID(), m_iOutput[iMaterial], iMaterial );
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::and has %d in stores ", m_iStore[iMaterial] );
#endif
				}

				// does this building have the material needed?
				if( m_iStore[iMaterial] )
				{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::player %d building %ld has material %d ",
m_iPlayer, pUnit->GetID(), iMaterial );
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::with %d in stores ", m_iStore[iMaterial] );
#endif
					// if the building in need is rocket then this
					// is a material movement need and must exceed
					// what is considered maximum materials onhand
					if( pBldgNeeding->GetTypeUnit() ==
						CStructureData::rocket )
					{
						if( m_iStore[iMaterial] < EXCESS_MATERIALS )
							continue;
					}

					// if less than we want, then record this source
					// as a 'next best' choice if no full source is found
					if( m_iStore[iMaterial] < iQtyNeeded )
					{
						int iDist = theMap.GetRangeDistance ( hexNeed, hexMat );
						CheckBest ( pUnit, iDist, m_iStore[iMaterial], pNextClosest, iNextDist, iNextBest );
						continue;
					}
				}
				else
					continue;
			}
			else if( iProduces == CStructureData::UTmine )
			{
				int iMatMined = CMaterialTypes::num_types;

				// get CBuildMine data
				EnterCriticalSection (&cs);
				CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					CBuildMine const *pData =
						pBldg->GetData()->GetBldMine();
					iMatMined = pData->GetTypeMines();
				}
				LeaveCriticalSection (&cs);

				// something is wrong
				if( iMatMined == CMaterialTypes::num_types )
					continue;

				// if the building mines what we are looking for
				if( iMatMined == iMaterial )
				{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d building %ld mines material %d ",
		m_iPlayer, pUnit->GetID(), iMaterial );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::and has %d in stores ", m_iStore[iMaterial] );
#endif
				}

				// does this building have the material needed?
				if( m_iStore[iMaterial] )
				{
					// if the building in need is rocket then this
					// is a material movement need and must exceed
					// what is considered maximum materials onhand
					if( pBldgNeeding->GetTypeUnit() ==
						CStructureData::rocket )
					{
						if( m_iStore[iMaterial] < EXCESS_MATERIALS )
							continue;
					}

					// if less than we want, then record this source
					// as a 'next best' choice if no full source is found
					if( m_iStore[iMaterial] < iQtyNeeded )
					{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d building %ld has material %d but has less than needed",
		m_iPlayer, pUnit->GetID(), iMaterial );
#endif
						int iDist = theMap.GetRangeDistance ( hexNeed, hexMat );
						CheckBest ( pUnit, iDist, m_iStore[iMaterial], pNextClosest, iNextDist, iNextBest );
						continue;
					}
				}
				else
					continue;
			}
			// farms and lumber 
			else if( iProduces == CStructureData::UTfarm )
			{
				int iMatFarmed = CMaterialTypes::num_types;

				// get CBuildFarm data
				EnterCriticalSection (&cs);
				CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					CBuildFarm const *pData =
						pBldg->GetData()->GetBldFarm();
					iMatFarmed = pData->GetTypeFarm();
				}
				LeaveCriticalSection (&cs);

				// something is wrong
				if( iMatFarmed == CMaterialTypes::num_types )
					continue;

				// if the building farms what we are looking for
				if( iMatFarmed == iMaterial )
				{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d building %ld farms material %d ",
		m_iPlayer, pUnit->GetID(), iMaterial );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::and has %d in stores ", m_iStore[iMaterial] );
#endif
				}

				// does this building have the material needed?
				if( m_iStore[iMaterial] )
				{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d building %ld has material %d ",
		m_iPlayer, pUnit->GetID(), iMaterial );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::with %d in stores ", m_iStore[iMaterial] );
#endif
					// if the building in need is rocket then this
					// is a material movement need and must exceed
					// what is considered maximum materials onhand
					if( pBldgNeeding->GetTypeUnit() ==
						CStructureData::rocket )
					{
						if( m_iStore[iMaterial] < EXCESS_MATERIALS )
							continue;
					}

					// if less than we want, then record this source
					// as a 'next best' choice if no full source is found
					if( m_iStore[iMaterial] < iQtyNeeded )
					{
						int iDist = theMap.GetRangeDistance ( hexNeed, hexMat );
						CheckBest ( pUnit, iDist, m_iStore[iMaterial], pNextClosest, iNextDist, iNextBest );
						continue;
					}
				}
				else 
					continue;
			}
			// all types of vehicles
			else if( iProduces == CStructureData::UTvehicle )
			{
				// get CBuildUnit data
				EnterCriticalSection (&cs);
				CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					CVehicleBuilding *pVehBldg =
						(CVehicleBuilding *)pBldg;
					CBuildUnit const *pBuildVeh = pVehBldg->GetBldUnt();
					if( pBuildVeh != NULL )
						m_iNeeds[iMaterial] = pBuildVeh->GetInput(iMaterial);
					else
						m_iNeeds[iMaterial] = 1; // force continue below
				}
				LeaveCriticalSection (&cs);

				// does this building need the same material?
				if( m_iNeeds[iMaterial] > m_iStore[iMaterial] )
				{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"CHPRouter::player %d building %ld/%d needs material %d ",
m_iPlayer, pUnit->GetID(), pUnit->GetTypeUnit(), iMaterial );
#endif
					continue;
				}

				// does this building have any of the material?
				if( m_iStore[iMaterial] < iQtyNeeded )
					continue;
			}
			else if( iProduces == CStructureData::UTpower )
			{
				if( pUnit->GetTypeUnit() >=
					CStructureData::num_types )
					continue;

				// get pointer to the CStructureData type for this unit
				CStructureData const *pBldgData = 
					theStructures.GetData( pUnit->GetTypeUnit() );
				if( pBldgData != NULL )
				{
					// now get pointer to the CBuild* instance of building
					CBuildPower *pPowerBldg = pBldgData->GetBldPower();
					if( pPowerBldg == NULL )
						continue;

					// it uses this material for fuel for power
					if( pPowerBldg->GetInput() == iMaterial )
						continue;
				}

				// does this building have the material needed?
				if( m_iStore[iMaterial] )
				{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d building %ld has material %d ",
		m_iPlayer, pUnit->GetID(), iMaterial );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::with %d in stores ",m_iStore[iMaterial] );
#endif
					// if the building in need is rocket then this
					// is a material movement need and must exceed
					// what is considered maximum materials onhand
					if( pBldgNeeding->GetTypeUnit() ==
						CStructureData::rocket )
					{
						if( m_iStore[iMaterial] < EXCESS_MATERIALS )
							continue;
					}

					// if less than we want, then record this source
					// as a 'next best' choice if no full source is found
					if( m_iStore[iMaterial] < iQtyNeeded )
					{
						int iDist = theMap.GetRangeDistance ( hexNeed, hexMat );
						CheckBest ( pUnit, iDist, m_iStore[iMaterial], pNextClosest, iNextDist, iNextBest );
						continue;
					}
				}
				else 
					continue;
			}
			else // if building does nothing it still may have material
			{
				// does this building have the material needed?
				if( m_iStore[iMaterial] )
				{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d building %ld has material %d ",
		m_iPlayer, pUnit->GetID(), iMaterial );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::with %d in stores ",m_iStore[iMaterial] );
#endif
					// if the building in need is rocket then this
					// is a material movement need and must exceed
					// what is considered maximum materials onhand
					if( pBldgNeeding->GetTypeUnit() ==
						CStructureData::rocket )
					{
						if( m_iStore[iMaterial] < EXCESS_MATERIALS )
							continue;
					}

					// if less than we want, then record this source
					// as a 'next best' choice if no full source is found
					if( m_iStore[iMaterial] < iQtyNeeded )
					{
						int iDist = theMap.GetRangeDistance ( hexNeed, hexMat );
						CheckBest ( pUnit, iDist, m_iStore[iMaterial], pNextClosest, iNextDist, iNextBest );
						continue;
					}
				}
				else 
					continue;
			}

			// this building has the material and does not use it
			int iDist = theMap.GetRangeDistance( hexNeed, hexMat );
			if ( iDist < iBestDist )
				{
				iBest = m_iStore[iMaterial];
				iBestDist = iDist;
				pClosest = pUnit;
				}
		}
	}

	// ok, we now kill pNextClosest if pClosest is better (and best > 24 hexes)
	if ( pClosest && pNextClosest && ( iBestDist > 24 ) )
		{
		// I'll travel 4 times as far to get all of it
		if ( iQtyNeeded / iBestDist >= iNextBest / ( 4 * iNextDist ) )
			pNextClosest = NULL;
		}

	// either the closest or the next closest pointer
	if( pClosest == NULL && pNextClosest != NULL )
	{
		GetBldgExit( pNextClosest->GetID(), hexMat );
		iBestDist = theMap.GetRangeDistance( hexNeed, hexMat );

		*piDistBack = iBestDist;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d building %ld is next best source of material %d ",
		m_iPlayer, pNextClosest->GetID(), iMaterial );
#endif
		return( pNextClosest );
	}
	else if( pClosest == NULL && pNextClosest == NULL )
	{
		// no materials found, however, consider that the artifical
		// reserve is now inplace, so if the building in need is not
		// a 1st time of that type, and it is of the special types,

		// CStructureData::lumber )
		if( pBldgNeeding->GetTypeUnit() == CStructureData::lumber )
		{
			if( m_wLumberCnt && m_bLastLumber )
			{
				// set rocket as source
				pClosest = m_plUnits->GetUnitNY(m_dwRocket);
			}
		}
		// CStructureData::coal )
		else if( pBldgNeeding->GetTypeUnit() == CStructureData::coal )
		{
			if( m_wCoalCnt && m_bLastCoal )
			{
				// set rocket as source
				pClosest = m_plUnits->GetUnitNY(m_dwRocket);
			}
		}
		// CStructureData::iron )
		else if( pBldgNeeding->GetTypeUnit() == CStructureData::iron )
		{
			if( m_wIronCnt && m_bLastIron )
			{
				// set rocket as source
				pClosest = m_plUnits->GetUnitNY(m_dwRocket);
			}
		}
		// CStructureData::smelter )
		else if( pBldgNeeding->GetTypeUnit() == CStructureData::smelter )
		{
			if( m_wSmelterCnt && m_bLastSmelter )
			{
				// set rocket as source
				pClosest = m_plUnits->GetUnitNY(m_dwRocket);
			}
		}

		// a special building was detected and special reserve enabled,
		// so now find the distance to the rocket for return
		if( pClosest != NULL )
		{
			GetBldgExit( pClosest->GetID(), hexMat );
			iBestDist = theMap.GetRangeDistance( hexNeed, hexMat );
		}
	}

	// return the best distance
	*piDistBack = iBestDist;

	return( pClosest );
}

BOOL CHPRouter::TrucksAreEnroute( CAIUnit *pBldg )
{
	for( int i=0; i<CMaterialTypes::num_types; ++i )
	{
		// is there an id present, if so its a truck
		// and are those materials still needed
		if( pBldg->GetParamDW(i) &&
			pBldg->GetParam(i) )
		{
			CAIUnit *pTruck = 
				m_plUnits->GetUnitNY(pBldg->GetParamDW(i));
			if( pTruck != NULL )
			{
				// if truck is assigned
				if( pTruck->GetDataDW() ==
					pBldg->GetID() )
					return TRUE;
			}
			else
				pBldg->SetParamDW(i,0); // truck is gone
		}
	}
	return FALSE;
}

//
// ship is no longer assigned
//
void CHPRouter::UnAssignShip( CAIUnit *pShip )
{
	pShip->ClearParam();
	pShip->SetStatus( 0 );	
	pShip->SetDataDW(0);
	m_plShipsAvailable->AddTail( (CObject *)pShip );
}

//
// for each building need commodities, the truck passed has
// been destroyed, so the building to which this truck was 
// assigned, needs to be toggled that it does not have
// this truck anymore
//
void CHPRouter::UnassignTrucks( DWORD dwTruckID )
{
	CAIUnit *pTruck = 
		m_plUnits->GetUnitNY(dwTruckID);
	if( pTruck == NULL )
		return;

    POSITION pos = m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pBldg = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pBldg != NULL )
        {
          	ASSERT_VALID( pBldg );

			if( !IsValidUnit( pBldg ) )
			 	continue;

			// is this the building the truck is assigned to?
			if( pTruck->GetDataDW() != pBldg->GetID() )
				continue;

			for( int i=0; i<CMaterialTypes::num_types; ++i )
			{
				// is there an id present, if so its a truck
				// and those materials still needed are in GetParam()
				// but the truck is gone now
				if( pBldg->GetParamDW(i) == dwTruckID )
					pBldg->SetParamDW(i,0);
			}

			// need to put the building back in the needs list

			// consider adding building to buildings need list
			if( m_plBldgsNeed->GetUnitNY( pBldg->GetID() )
				== NULL )
				m_plBldgsNeed->AddTail( (CObject *)pBldg );
			{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"Player %d Building %ld now needs a new truck ",
		m_iPlayer, pBldg->GetID() );
#endif
			}
		}
	}

	// now unassign any ships that may be assigned to this truck
}

//
// for each truck assigned to the building, unassign
// it and add it to the m_plTrucksAvailable
//
void CHPRouter::UnassignTrucks( CAIUnit *pBldg )
{
	DWORD dwID = 0;
	for( int i=0; i<CMaterialTypes::num_types; ++i )
	{
		// is there an id present, if so its a truck
		// and are those materials still needed
		dwID = pBldg->GetParamDW(i);
		if( dwID )
		{
			CAIUnit *pTruck = 
				m_plUnits->GetUnitNY(dwID);
			if( pTruck != NULL )
			{
				if( m_plTrucksAvailable->GetUnitNY(pTruck->GetID()) 
					== NULL )
				{
					// now unassign the truck
					pTruck->SetDataDW(0);
					pTruck->ClearParam();
					pTruck->SetStatus(0);
					// and add it to the list
					m_plTrucksAvailable->AddTail( (CObject *)pTruck );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d truck %ld is unassigned ",
		pTruck->GetOwner(), pTruck->GetID() );
#endif

				}
			}
		}
	}
	// the damn optimizing compiler seems to not want me to be
	// able to debug the code above so I'm adding this to protect
	// against not unassigning the truck from a destroyed building
	//
	// process all units and if there is this pBldg->GetID() in
	// the unit's GetDataDW() it must be an assigned truck so
	// get it unassigned
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
			if( pUnit->GetType() == CUnit::building )
				continue;
			// should be just trucks left
			if( pUnit->GetDataDW() == pBldg->GetID() )
			{
				// now unassign the truck
				pUnit->SetDataDW(0);
				pUnit->ClearParam();
				pUnit->SetStatus(0);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d truck %ld is unassigned again ",
		pUnit->GetOwner(), pUnit->GetID() );
#endif

				// and possibly add it to the available truck list
				CAIUnit *pTruck = 
					m_plTrucksAvailable->GetUnitNY(pUnit->GetID());
				if( pTruck == NULL )
					m_plTrucksAvailable->AddTail( (CObject *)pUnit );
			}
		}
	}
}

// go thru the units list, looking for buildings that have excess
// materials, if some then add 
void CHPRouter::CheckWarehouses( void )
{
	if( !HaveExcessMaterials() )
		return;

	// add rocket to m_plBldgsNeed list
	CAIUnit *pRocket = m_plUnits->GetUnitNY(m_dwRocket);
	if( pRocket == NULL )
		return;

	if( m_plBldgsNeed->GetUnitNY( m_dwRocket ) == NULL )
	{
		m_plBldgsNeed->AddTail( (CObject *)pRocket );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"\nPlayer %d rocket %ld added for excess materials ",
		m_iPlayer, pRocket->GetID() );
#endif
	}
}

BOOL CHPRouter::NeedsCommodities( CAIUnit *pCAIBldg )
{
	if( pCAIBldg != NULL )
		return( _NeedsCommodities(pCAIBldg) );

	BOOL bNeeds = FALSE;
	CAIUnit *pUnit = NULL;

	// NULL building means that all buildings in m_plBldgsNeed
	// must be checked to determine if they still need materials
    POSITION pos1, pos2;
    for( pos1 = m_plBldgsNeed->GetHeadPosition(); 
        ( pos2 = pos1 ) != NULL; )
    {
		BOOL bNeedsStill = TRUE;

        pUnit = (CAIUnit *)m_plBldgsNeed->GetNext( pos1 );
        if( pUnit != NULL )
        {
            ASSERT_VALID( pUnit );

			if( !IsValidUnit( pUnit ) )
			 	continue;

			// makes adjustments to the needs of the building
			if( _NeedsCommodities(pUnit) )
				bNeeds = TRUE;
			else 
			{
				// building may still need materials and should
				// remain in the list, because a truck has been
				// assigned and is enroute
				bNeedsStill = TrucksAreEnroute( pUnit );
			}
        }

		// or removes it from the needs list
		if( !bNeedsStill )
		{
			pUnit = (CAIUnit *)m_plBldgsNeed->GetAt( pos2 );
			if( pUnit != NULL )
			{
				ASSERT_VALID( pUnit );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"\nCHPRouter::NeedsCommodities for player %d ", m_iPlayer );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"building %d id=%ld removed from needs list", 
		pUnit->GetTypeUnit(), pUnit->GetID() );
#endif
        		m_plBldgsNeed->RemoveAt( pos2 );
			}
		}
    }

	return( bNeeds );
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
BOOL CHPRouter::_NeedsCommodities( CAIUnit *pCAIBldg )
{
	CHexCoord hexBldg;
	int iProduces = CStructureData::num_union_types;
	BOOL bIsConstructing = FALSE;
	BOOL bIsPaused = FALSE;
	BOOL bNeedRepair = FALSE;
	BOOL bIsWaiting = FALSE;

	// need pBuilding->IsConstructing(); [CAI_ISCONSTRUCTING]
	// int	GetBldgMatRepair (int iInd) const;
	EnterCriticalSection (&cs);
	CBuilding *pBldg = 
		theBuildingMap.GetBldg( pCAIBldg->GetID() );
	if( pBldg != NULL )
	{
		// building has been damaged
		if( pBldg->GetDamagePer() < DAMAGE_0 )
			bNeedRepair = TRUE;

		bIsConstructing = pBldg->IsConstructing();
		bIsPaused = pBldg->IsPaused();
		bIsWaiting = pBldg->IsWaiting();
		hexBldg = pBldg->GetExitHex();
		// need pBuilding->GetData()->GetUnionType(); [CAI_PRODUCES]
		iProduces = pBldg->GetData()->GetUnionType();
		// record what's in storage of this material at this building
		for( int i=0; i<CMaterialTypes::num_types; ++i )
			m_iStore[i] = pBldg->GetStore(i);
	}
	LeaveCriticalSection (&cs);

	// we only handle our own buildings
	if( pCAIBldg->GetOwner() != m_iPlayer )
		{
		TRAP ();
		return FALSE;
		}

	// something is wrong
	if( iProduces == CStructureData::num_union_types )
		return FALSE;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"\nCHPRouter::NeedsCommodities for player %d ", m_iPlayer );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"checking building %d id=%ld IsConstructing=%d IsPaused=%d IsWaiting=%d ", 
		pCAIBldg->GetTypeUnit(), pCAIBldg->GetID(), 
		(int)bIsConstructing, (int)bIsPaused, (int)bIsWaiting );
#endif

	// clear material needs working arrays
	for( int i=0; i<CMaterialTypes::num_types; ++i )
	{
		m_aiMatsNeeded[i] = 0;
		m_iNeeds[i] = 0;
	}

	// is this a building in need of repairs
	if( bNeedRepair )
	{
		// dave is making a new function CBuilding::GetNextMinuteMat()
		// to return the materials needed by the building for the next
		// minute of use

		EnterCriticalSection (&cs);
		CBuilding *pBldg = 
			theBuildingMap.GetBldg( pCAIBldg->GetID() );
		if( pBldg != NULL )
		{
			// go thru materials needed, and record needs
			for( i=0; i<CMaterialTypes::num_build_types; ++i )
			{
				// repair while under construction
				if( bIsConstructing )
				{
					if( pBldg->GetBldgMatReq(i, FALSE) )
						m_aiMatsNeeded[i] = pBldg->GetBldgMatReq(i, FALSE);
				}
				else // just repair
				{
					if( pBldg->GetBldgMatRepair(i) )
					{
						m_aiMatsNeeded[i] = pBldg->GetBldgMatReq(i, FALSE);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"building %d id=%ld needs %d of material %d for repair", 
pCAIBldg->GetTypeUnit(), pCAIBldg->GetID(),
pBldg->GetBldgMatRepair(i), i );
#endif
					}
				}
			}
		}
		LeaveCriticalSection (&cs);
	}

	// is this a building under construction
	if( bIsConstructing && !bNeedRepair )
	{
		// get pointer to the CStructureData type for this unit
		CStructureData const *pBldgData = 
			theStructures.GetData( pCAIBldg->GetTypeUnit() );
		if( pBldgData == NULL )
			return FALSE;

		/*
		int iMat = pBldgData->GetBuild(0);
		iMat = pBldgData->GetBuild(1);
		iMat = pBldgData->GetBuild(2);
		iMat = pBldgData->GetBuild(3);
		iMat = pBldgData->GetBuild(4);
		iMat = pBldgData->GetBuild(5);
		iMat = pBldgData->GetBuild(6);
		iMat = pBldgData->GetBuild(7);
		iMat = pBldgData->GetBuild(8);
		iMat = pBldgData->GetBuild(9);
		iMat = 0;
		*/

		CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
		if( pBldg != NULL )
			{
			EnterCriticalSection (&cs);
			for( i=0; i<CMaterialTypes::num_build_types; ++i )
				m_aiMatsNeeded[i] = pBldg->GetBldgMatReq(i, FALSE);
			LeaveCriticalSection (&cs);
			}
		else
			{
			TRAP ();
			for( i=0; i<CMaterialTypes::num_build_types; ++i )
				m_aiMatsNeeded[i] = pBldgData->GetBuild(i);
			}

#ifdef BUGBUG
		// go thru materials needed, and record needs
		for( i=0; i<CMaterialTypes::num_build_types; ++i )
		{
			// update materials needed to construct this building
			//iMat = pBldgData->GetBuild(i);
			//if( iMat )
			//	m_aiMatsNeeded[i] = iMat;
			if( pBldgData->GetBuild(i) )
				m_aiMatsNeeded[i] = pBldgData->GetBuild(i);
		}
#endif
	}
	else if( !bIsPaused ) // building is built and running
	{
		// does this building produce materials?
		if( iProduces == CStructureData::UTmaterials )
		{
		// dave is making a new function CBuilding::GetNextMinuteMat()
		// to return the materials needed by the building for the next
		// minute of use
			int iNumMinutes = 10;
			if( pCAIBldg->GetTypeUnit() == CStructureData::refinery )
			{
				// allow difficulty to influence material amount
				iNumMinutes = 300;
				if( pGameData != NULL )
					iNumMinutes -= (pGameData->GetRandom(pGameData->m_iSmart) * 50);
			}

			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
			if( pBldg != NULL )
			{
				
				for( i=0; i<CMaterialTypes::num_types; ++i )
				{
					m_iNeeds[i] = pBldg->GetNextMinuteMat(i) * iNumMinutes;
				}
			}
			else
			{
				LeaveCriticalSection (&cs);
				return FALSE;
			}
			LeaveCriticalSection (&cs);

			// go thru materials needed, and record needs
			for( i=0; i<CMaterialTypes::num_types; ++i )
			{
				// update materials needed to produce materials
				if( m_iNeeds[i] )
				{
					m_aiMatsNeeded[i] += m_iNeeds[i];

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"material building type %d for player %d id=%ld needs %d of material %d ", 
		pCAIBldg->GetTypeUnit(), m_iPlayer, pCAIBldg->GetID(),
		m_iNeeds[i], i );
#endif
				}
			}
		}
		// or this building produces vehicles?
		else if( iProduces == CStructureData::UTvehicle )
		{
		// dave is making a new function CBuilding::GetNextMinuteMat()
		// to return the materials needed by the building for the next
		// minute of use

			CBuildUnit const *pBuildVeh = NULL;
			// get CBuildUnit data
			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
			if( pBldg != NULL )
			{
				CVehicleBuilding *pVehBldg =
					(CVehicleBuilding *)pBldg;
				pBuildVeh = pVehBldg->GetBldUnt();
				if( pBuildVeh != NULL )
				{
					for( i=0; i<CMaterialTypes::num_build_types; ++i )
						m_iNeeds[i] = pBldg->GetNextMinuteMat(i);
//BUGBUG						m_iNeeds[i] = pBuildVeh->GetInput(i);
				}
			}
			LeaveCriticalSection (&cs);

			// NULL pointer means no CBuildUnit data
			if( pBuildVeh == NULL )
			{

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"building type %d for player %d id=%ld has no vehicle in production ", 
		pCAIBldg->GetTypeUnit(), m_iPlayer, pCAIBldg->GetID() );
#endif
				return FALSE;
			}

			// go thru materials needed, and record needs
			for( i=0; i<CMaterialTypes::num_types; ++i )
			{
				// update materials needed for vehicle
				if( m_iNeeds[i] )
				{
					m_aiMatsNeeded[i] += m_iNeeds[i];

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"vehicle building type %d for player %d id=%ld needs %d of material %d ", 
		pCAIBldg->GetTypeUnit(), m_iPlayer, pCAIBldg->GetID(),
		m_aiMatsNeeded[i], i );
#endif
				}
			}
		}
		// else is a power plant
		else if( iProduces == CStructureData::UTpower )
		{
		// dave is making a new function CBuilding::GetNextMinuteMat()
		// to return the materials needed by the building for the next
		// minute of use
			int iTime = 120;

			if( pGameData != NULL )
			{
				switch( pGameData->m_iSmart )
				{
				case 0:				// easy
					iTime *= 3;
					break;
				case 1:				// moderate
					iTime *= 2;
					break;
				case 3:				// impossible
					iTime -= pGameData->GetRandom(pGameData->m_iSmart);
					break;
				case 2:				// difficult
				default:
					break;
				}
			}

			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
			if( pBldg != NULL )
			{
				
				for( i=0; i<CMaterialTypes::num_types; ++i )
				{
					m_aiMatsNeeded[i] += pBldg->GetNextMinuteMat(i) * iTime;
				}
			}
			else
			{
				LeaveCriticalSection (&cs);
				return FALSE;
			}
			LeaveCriticalSection (&cs);

		}
		else if( iProduces == CStructureData::UTshipyard )
		{
			for( i=0; i<CMaterialTypes::num_types; ++i )
				m_iNeeds[i] = 0;

		// dave is making a new function CBuilding::GetNextMinuteMat()
		// to return the materials needed by the building for the next
		// minute of use

			// get CBuilding data
			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
			if( pBldg != NULL )
			{
				CShipyardBuilding *pShipBldg = 
					(CShipyardBuilding*)pBldg;

				// is there a vehicle in getting a repair?
				CVehicle *pVehRepairing = pShipBldg->GetVehRepairing();
				if( pVehRepairing != NULL )
				{
					// get the repair data for that vehicle
					CBuildRepair const *pBuildRep = 
						(CBuildRepair const *)pShipBldg->GetData();
					CBuildUnit const *pBuildVeh = 
						pBuildRep->GetRepair(pVehRepairing->GetData()->GetType());
					if( pBuildVeh != NULL )
					{
						for( i=0; i<CMaterialTypes::num_build_types; ++i )
							{
							TRAP ();
							m_iNeeds[i] = pBldg->GetNextMinuteMat(i);
							}
//BUGBUG							m_iNeeds[i] = pBuildVeh->GetInput(i);
					}
				}
				else // is a vehicle being constructed?
				{
					CBuildUnit const *pBuildVeh = pShipBldg->GetBldUnt();
					if( pBuildVeh != NULL )
					{
						for( i=0; i<CMaterialTypes::num_build_types; ++i )
							m_iNeeds[i] = pBuildVeh->GetInput(i);
					}
				}
			}
			LeaveCriticalSection (&cs);

			// go thru materials needed, and record needs
			for( i=0; i<CMaterialTypes::num_types; ++i )
			{
				// update materials needed for vehicle
				if( m_iNeeds[i] )
				{
					m_aiMatsNeeded[i] += m_iNeeds[i];

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
"shipyard building type %d for player %d id=%ld needs %d of material %d ", 
pCAIBldg->GetTypeUnit(), m_iPlayer, pCAIBldg->GetID(), m_aiMatsNeeded[i], i );
#endif
				}
			}
		}
		else if( iProduces == CStructureData::UTrepair )
		{
		// dave is making a new function CBuilding::GetNextMinuteMat()
		// to return the materials needed by the building for the next
		// minute of use

			CBuildUnit const *pBuildVeh = NULL;
			// get CBuildUnit data
			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
			if( pBldg != NULL )
			{
				CRepairBuilding *pRepBldg = 
					(CRepairBuilding *)pBldg;
				// is there a vehicle in getting a repair?
				CVehicle *pVehRepairing = pRepBldg->GetVehRepairing();
				if( pVehRepairing == NULL && !bNeedRepair )
				{
					LeaveCriticalSection (&cs);
					return FALSE;
				}

				if( pVehRepairing != NULL )
				{
					// get the repair data for that vehicle
					CBuildRepair const *pBuildRep = pRepBldg->GetData();

					// BUGBUG may get total materials needed from using
					// pBuildRep->GetTotalRepair(iInd) 

					pBuildVeh = 
					pBuildRep->GetRepair(pVehRepairing->GetData()->GetType());
					if( pBuildVeh != NULL )
					{
						for( i=0; i<CMaterialTypes::num_build_types; ++i )
							m_iNeeds[i] = pBldg->GetNextMinuteMat(i);
//BUGBUG							m_iNeeds[i] = pBuildVeh->GetInput(i);
					}
				}
			}
			LeaveCriticalSection (&cs);

			// NULL pointer means no CBuildUnit data
			if( pBuildVeh == NULL && !bNeedRepair )
			{

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"building repair type %d for player %d id=%ld has no vehicle in for repair ", 
		pCAIBldg->GetTypeUnit(), pCAIBldg->GetOwner(), pCAIBldg->GetID() );
#endif
				return FALSE;
			}

			// go thru materials needed, and record needs
			for( i=0; i<CMaterialTypes::num_types; ++i )
			{
				// update materials needed for vehicle
				if( m_iNeeds[i] )
				{
					m_aiMatsNeeded[i] += (m_iNeeds[i] * 2);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"repair building %d for player %d id=%ld needs %d of material %d ", 
		pCAIBldg->GetTypeUnit(), m_iPlayer, pCAIBldg->GetID(),
		m_iNeeds[i], i );
#endif
				}
			}
		}
		else if( iProduces == CStructureData::UThousing )
		{
			// housing does not need anything
		}
		else if( iProduces == CStructureData::UTmine )
		{
			// a mine, don't need anything
		}
		else if( iProduces == CStructureData::UTfarm )
		{
			// a farm, don't need anything
		}
		else if( iProduces == CStructureData::UTresearch )
		{
			// a research, don't need anything
		}
		else if( iProduces == CStructureData::UTcommand )
		{
			// a command center, don't need anything
		}
		else if( iProduces == CStructureData::UTembassy )
		{
			// a embassy, don't need anything
		}
		else if( iProduces == CStructureData::UTfort )
		{
			// a fort, don't need anything
		}
		else if( iProduces == CStructureData::UTwarehouse )
		{
			// no excess materials get moved if less trucks are available
			if( m_plTrucksAvailable->GetCount() <= MINIMUM_IDLE_TRUCKS )
			{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"player %d too few trucks %d available for warehouse %ld/%d ", 
	m_iPlayer, m_plTrucksAvailable->GetCount(),
	pCAIBldg->GetID(), pCAIBldg->GetTypeUnit() );
#endif
				return( FALSE );
			}

			// if excess materials > EXCESS_MATERIALS units of anything exist in 
			// the storage of all buildings but other warehouses, then
			// this warehouse should be set to need the excess of that
			// material
			for( i=0; i<CMaterialTypes::num_types; ++i )
				m_iNeeds[i] = 0;

			// update m_iNeeds[] with excess materials not in this building
			SetExcessMaterials( pCAIBldg );

			// go thru materials needed, and record needs
			for( i=0; i<CMaterialTypes::num_types; ++i )
			{
				m_aiMatsNeeded[i] += m_iNeeds[i];

#ifdef _LOGOUT
	if( m_iNeeds[i] )
	{
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"building %d player %d id=%ld wants excess %d of material %d ", 
		pCAIBldg->GetTypeUnit(), m_iPlayer, pCAIBldg->GetID(),
		m_iNeeds[i], i );
	}
#endif
			}
		}
		else // unknown production type?
			return( FALSE );

	}

	// now apply what is enroute to building via trucks
	// that contain the needed materials

    POSITION pos = m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pTruck = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pTruck != NULL )
        {
        	ASSERT_VALID( pTruck );

			if( pTruck->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a building
			if( pTruck->GetType() == CUnit::building )
				continue;

			// is truck assigned to this building
			if( (pTruck->GetStatus() & CAI_IN_USE) &&
				(pTruck->GetDataDW() == pCAIBldg->GetID()) )
			{
				// get access to CVehicle data for truck
				// and load up m_iStore[] with what its carrying
				GetVehicleCargo( pTruck->GetID() );

				// consider quantity of material carried
				// for if it helps out on the needs of building
				for( i=0; i<CMaterialTypes::num_types; ++i )
				{
					// a needed material of the building
					if( m_aiMatsNeeded[i] )
					{
						// apply what the enroute truck is carrying
						if( m_aiMatsNeeded[i] > m_iStore[i] )
							m_aiMatsNeeded[i] - m_iStore[i];
						else
							m_aiMatsNeeded[i] = 0;

					}
				}

// BUGBUG - maybe we need to leave the need present, despite the truck
// being enroute, so that when the unload occurs, the building gets
// processed for a needed material when zero quantity is delivered

#if 0
				// consider that building may still be in need
				// of material, that is available at a building
				// to which the assigned truck is in enroute
				for( i=0; i<CMaterialTypes::num_types; ++i )
				{
					// a needed material of the building
					if( m_aiMatsNeeded[i] )
					{
						// truck has a destination for that material
						if( pTruck->GetParamDW(i) )
							m_aiMatsNeeded[i] = 0;
					}
				}
#endif
			}
		}
	}

	// get what materials are at the building into m_iStore[]
	EnterCriticalSection (&cs);
	pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
	if( pBldg != NULL )
	{
		for( int i=0; i<CMaterialTypes::num_types; ++i )
			m_iStore[i] = pBldg->GetStore(i);
	}
	LeaveCriticalSection (&cs);

	// if working with a warehouse, then don't count 
	// materials already at that warehouse
	if( iProduces == CStructureData::UTwarehouse &&
		!bIsPaused && !bIsConstructing )
	{
		for( int i=0; i<CMaterialTypes::num_types; ++i )
			m_iStore[i] = 0;
	}

	BOOL bNeedSomething = FALSE;
	// now apply what is on-hand at the building against needs
	for( i=0; i<CMaterialTypes::num_types; ++i )
	{
		// update needed commoditiy array
		// with id offset of commodity, needed amount, on-hand amount
		SetNeeded( i, m_aiMatsNeeded, m_iStore[i] );

		// if a material is still needed
		if( m_aiMatsNeeded[i] )
			bNeedSomething = TRUE;
		else
			pCAIBldg->SetParam( i, 0 );
	}

	// only change the buildings params if something is needed
	if( bNeedSomething )
	{
		for( i=0; i<CMaterialTypes::num_types; ++i )
		{
			pCAIBldg->SetParam( i, m_aiMatsNeeded[i] );

#ifdef _LOGOUT
			if( pCAIBldg->GetParam(i) )
			{
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"building type %d player %d id=%ld needs %d materials %d from m_aiMatsNeeded[]", 
		pCAIBldg->GetTypeUnit(), m_iPlayer, pCAIBldg->GetID(),
		pCAIBldg->GetParam(i), i );
			}
#endif
		}
		// clear the last param offset to use for priority
		pCAIBldg->SetParam( CAI_UNASSIGNED, 0 );
	}
	else
	{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"building type %d for player %d id=%ld does not need materials ", 
		pCAIBldg->GetTypeUnit(), m_iPlayer, pCAIBldg->GetID() );
	// display what is recorded
	for( i=0; i<CMaterialTypes::num_types; ++i )
	{
		logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
			"material %d needed %d stored %d ", i, pCAIBldg->GetParam(i), m_iStore[i] );
	}
#endif
	}

	return( bNeedSomething );
}

//
// go thru all the buildings and look for excess materials
// returning TRUE on any found
//
BOOL CHPRouter::HaveExcessMaterials( void )
{
	BOOL bExcessExists = FALSE;

    POSITION pos = m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pCAIBldg = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pCAIBldg != NULL )
        {
        	ASSERT_VALID( pCAIBldg );

			if( pCAIBldg->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a building
			if( pCAIBldg->GetType() != CUnit::building )
				continue;

			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
			if( pBldg != NULL )
			{
				// skip any warehouses
				if( pBldg->GetData()->GetUnionType() ==
					CStructureData::UTwarehouse )
				{
					LeaveCriticalSection (&cs);
					continue;
				}

				for( int i=0; i<CMaterialTypes::num_types; ++i )
				{
					if( pBldg->GetStore(i) > EXCESS_MATERIALS )
						bExcessExists = TRUE;
				}
			}
			LeaveCriticalSection (&cs);

			if( bExcessExists )
				return TRUE;
		}
	}
	return FALSE;
}

//
// go thru all buildings and record excess materials > EXCESS_MATERIALS
// found at any buildings in the m_iNeeds[]
// 
void CHPRouter::SetExcessMaterials( CAIUnit *pWarehouseBldg )
{
    POSITION pos = m_plUnits->GetHeadPosition();
    while( pos != NULL )
    {   
        CAIUnit *pCAIBldg = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pCAIBldg != NULL )
        {
        	ASSERT_VALID( pCAIBldg );

			if( pCAIBldg->GetOwner() != m_iPlayer )
				continue;

			// determine if this is a building
			if( pCAIBldg->GetType() != CUnit::building )
				continue;

			// skip warehouse being considered for routing
			if( pCAIBldg->GetID() == pWarehouseBldg->GetID() )
				continue;

			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
			if( pBldg != NULL )
			{
				// skip other warehouses
				if( pBldg->GetData()->GetUnionType() ==
					CStructureData::UTwarehouse )
				{
					LeaveCriticalSection (&cs);
					continue;
				}

				// update m_iNeeds[] with any excess materials
				for( int i=0; i<CMaterialTypes::num_types; ++i )
				{
					if( pBldg->GetStore(i) > EXCESS_MATERIALS )
						m_iNeeds[i] += pBldg->GetStore(i) - EXCESS_MATERIALS;
				}
			}
			LeaveCriticalSection (&cs);
		}
	}
}

//
// update the pwaParams array of the unit with the
// amount needed - amount on hand or zero if no need
//
void CHPRouter::SetNeeded( int iMat, int iaNeeded[], int iOnHand )
{
	int iAmt = 0;
	if( iaNeeded[iMat] > iOnHand )
		iAmt = iaNeeded[iMat] - iOnHand;
	iaNeeded[iMat] = iAmt;
}

// BUGBUG this function does not handle all priority
//
// determine the priority for the building passed
// based on what the building does
//
void CHPRouter::SetUnitPriority( CAIUnit *pCAIBldg )
{
	ASSERT_VALID( this );
	ASSERT_VALID( pCAIBldg );

	BOOL bIsConstructing = FALSE;
	BOOL bIsWaiting = FALSE;
	BOOL bNeedRepair = FALSE;
	int iProduces = CStructureData::num_union_types, i=0;

	// need pBuilding->IsConstructing(); [CAI_ISCONSTRUCTING]
	EnterCriticalSection (&cs);
	CBuilding *pBldg = 
		theBuildingMap.GetBldg( pCAIBldg->GetID() );
	if( pBldg != NULL )
	{
		// building has been damaged
		if( pBldg->GetDamagePer() < DAMAGE_0 )
			bNeedRepair = TRUE;

		bIsConstructing = pBldg->IsConstructing();
		bIsWaiting = pBldg->IsWaiting();
		// need pBuilding->GetData()->GetUnionType(); [CAI_PRODUCES]
		iProduces = pBldg->GetData()->GetUnionType();
	}
	LeaveCriticalSection (&cs);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::SetUnitPriority for player %d building %ld priority %d ", 
		m_iPlayer, pCAIBldg->GetID(), pCAIBldg->GetParam(CAI_UNASSIGNED) );
#endif
	
	// consider the needs vs assignment
	BOOL bTruckStillNeeded = FALSE;

	for( i=0; i<CMaterialTypes::num_types; ++i )
	{
		// if material is needed
		if( pCAIBldg->GetParam(i) )
		{
			// is a truck assigned to transport it?
			if( !pCAIBldg->GetParamDW(i) )
				bTruckStillNeeded = TRUE;
		}
	}

	// if the building currently has no priority and
	// yet has a truck assigned for a needed material
	// then let its priority stay as zero
	if( !bTruckStillNeeded )
		return;

	// make sure there is no priority
	WORD wPriority = 0;
	pCAIBldg->SetParam( CAI_UNASSIGNED, wPriority );
	
	// if building a building under construction	  add 30
	if( bIsConstructing )
	{
		wPriority += 30;

	//   if building::event is set					  add 20
		if( bIsWaiting )
			wPriority += 20;

	//   if war and constructing new building         add 15
	}
	//   if no war and vehicle or new building        add 10

	if( bNeedRepair )
		wPriority += 20;

	if( iProduces == CStructureData::UTmaterials )
	{
		// get CBuildMaterials data
		EnterCriticalSection (&cs);
		CBuilding *pBldg = 
			theBuildingMap.GetBldg( pCAIBldg->GetID() );
		if( pBldg != NULL )
		{
			CBuildMaterials const *pData =
				pBldg->GetData()->GetBldMaterials();
			if( pData != NULL )
			{
				for( i=0; i<CMaterialTypes::num_types; ++i )
				{
					m_iOutput[i] = pData->GetOutput(i);
				}
			}
			else
			{
				LeaveCriticalSection (&cs);
				return;
			}
		}
		else
		{
			LeaveCriticalSection (&cs);
			return;
		}
		LeaveCriticalSection (&cs);

		// BUGBUG goods are irrelevent now
		// if pBldg is producing a composite material add 50
		if( m_iOutput[CAI_LUMBER]
		 || m_iOutput[CAI_STEEL]
		 || m_iOutput[CAI_COAL]
		 || m_iOutput[CAI_IRON]
		 )
			wPriority += 50;
		// else if pBldg is producing a material          add 40 
		else
			wPriority += 30;
	}
	else if( iProduces == CStructureData::UTvehicle )
	{
		CBuildUnit const *pBuildVeh = NULL;
		// get CBuildUnit data
		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
		if( pBldg != NULL )
		{
			CVehicleBuilding *pVehBldg =
				(CVehicleBuilding *)pBldg;
			pBuildVeh = pVehBldg->GetBldUnt();
		}
		LeaveCriticalSection (&cs);

		// NULL pointer means no CBuildUnit data
		if( pBuildVeh == NULL )
			wPriority += 10;
		else
		{
			// if pBldg is producing TRUCK                    add 20
			// BUGBUG is this the correct way to
			// determine that a vehicle is a truck
			if( pBuildVeh->GetVehType() == CTransportData::med_truck ||
				pBuildVeh->GetVehType() == CTransportData::heavy_truck )
			{
				wPriority += 40;

				if( m_plTrucksAvailable->GetCount() < MINIMUM_IDLE_TRUCKS )
					wPriority += 30;
			}
			else
				wPriority += 30;

		}
		// BUGBUG need to apply these influences
	  	// if total TRUCKS > factorys                   sub 10
	  	// if total TRUCKS > factorys+commodities used  sub 10
		// if war and combat vehicle                      add 20
	}
	else if( iProduces == CStructureData::UTshipyard )
	{
		wPriority += 10;

		// get CBuilding data
		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
		if( pBldg != NULL )
		{
			CShipyardBuilding *pShipBldg = 
				(CShipyardBuilding*)pBldg;

			// is there a vehicle in getting a repair?
			CVehicle *pVehRepairing = pShipBldg->GetVehRepairing();
			if( pVehRepairing != NULL )
			{
				// get the repair data for that vehicle
				CBuildRepair const *pBuildRep = 
					(CBuildRepair const *)pShipBldg->GetData();
				CBuildUnit const *pBuildVeh = 
					pBuildRep->GetRepair(pVehRepairing->GetData()->GetType());
				if( pBuildVeh != NULL )
					wPriority += 20;
			}
			else // is a vehicle being constructed?
			{
				CBuildUnit const *pBuildVeh = pShipBldg->GetBldUnt();
				if( pBuildVeh != NULL )
					wPriority += 20;
			}
		}
		LeaveCriticalSection (&cs);
	}
	else if( iProduces == CStructureData::UTwarehouse )
	{
		int iStore = 0;
		int iLowStore = 0xFFFE;
		DWORD dwStore = 0;
		// check all warehouses to see which
		// has the least combined materials
		// but only check for warehouses 
		// that are in m_plBldgsNeed and if
		// this warehouse has least, set it to
		// higher priority than other warehouses
       	POSITION pos = m_plBldgsNeed->GetHeadPosition();
        while( pos != NULL )
        {
			iStore = 0;
            CAIUnit *pUnit = (CAIUnit *)m_plBldgsNeed->GetNext( pos );
            if( pUnit != NULL )
            {
				if( pUnit->GetOwner() != m_iPlayer )
					continue;

				if( pUnit->GetTypeUnit() != CStructureData::rocket &&
					pUnit->GetTypeUnit() != CStructureData::warehouse )
					continue;
				// left should be only warehouse type buildings
				// get storage
	
				EnterCriticalSection (&cs);
				CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					for( i=0; i<CMaterialTypes::num_types; ++i )
						iStore += pBldg->GetStore(i);
				}
				LeaveCriticalSection (&cs);
			}

			if( iStore < iLowStore )
			{
				dwStore = pUnit->GetID();
				iLowStore = iStore;
			}
		}
		
		// this warehouse has the least materials
		if( !dwStore || dwStore == pCAIBldg->GetID() )
			wPriority += 10;
	}
	else if( iProduces == CStructureData::UTrepair )
	{
		wPriority += 10;

		CBuildUnit const *pBuildVeh = NULL;
		// get CBuildUnit data
		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
		if( pBldg != NULL )
		{
			CRepairBuilding *pRepBldg = (CRepairBuilding *)pBldg;
			// is there a vehicle in getting a repair?
			CVehicle *pVehRepairing = pRepBldg->GetVehRepairing();
			if( pVehRepairing != NULL )
				wPriority += 20;
		}
		LeaveCriticalSection (&cs);
	}
	else if( iProduces == CStructureData::UTmine )
	{
		wPriority += 50;
	}
	else if( iProduces == CStructureData::UTfarm )
	{
		// give farms a priority
		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
		if( pBldg != NULL )
		{
			if( pBldg->GetData()->GetBldFarm()->GetTypeFarm() ==
				CMaterialTypes::lumber )
				wPriority += 50;
			else
				wPriority += 30;
		}
		LeaveCriticalSection (&cs);
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
		if( iProduces == CStructureData::UTpower )
		{
			wPriority += 40;

			// if the fuel for this power plant is zero then increase priority
			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pCAIBldg->GetID() );
			if( pBldg != NULL )
			{
				for( i=0; i<CMaterialTypes::num_types; ++i )
				{
					if( pBldg->GetNextMinuteMat(i) > pBldg->GetStore(i) )
					{
						wPriority += 40;
						break;
					}
				}
			}
			LeaveCriticalSection (&cs);
			
		}
		else
			wPriority += 10;
		// research,
		// repair,
		// warehouse,		// market or warehouse
	}

	pCAIBldg->SetParam( CAI_UNASSIGNED, wPriority );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::SetUnitPriority for player %d building %ld new priority %d ", 
		m_iPlayer, pCAIBldg->GetID(), pCAIBldg->GetParam(CAI_UNASSIGNED) );
#endif
}
//
// transfer the qtys needed of materials carried
// by the truck to the building 
// and clear the truck of assignment
//
BOOL CHPRouter::UnloadMaterials( CAIUnit *pTruck, CAIUnit *pBldg )
{
	CHexCoord hexVeh;
	if( !GetVehicleHex( pTruck->GetID(), hexVeh ) )
		return( TRUE );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d transport truck %ld unloading at building %ld ", 
		m_iPlayer, pTruck->GetID(), pBldg->GetID() );
	for( int j=0; j<CMaterialTypes::num_types; ++j )
		logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
		" needing %d in quantity of %d", j, pBldg->GetParam(j) );
#endif

	// and load up m_iStore[] with what its carrying
	GetVehicleCargo( pTruck->GetID() );

	BOOL bStillNeeds = FALSE;

	// game message
	CMsgTransMat msg;
	msg.m_dwIDSrc = pTruck->GetID();
	msg.m_dwIDDest = pBldg->GetID();
	memset( msg.m_aiMat, 0, sizeof (msg.m_aiMat));
	
	// now go through the materials needed
	for( int i=0; i<CMaterialTypes::num_types; ++i )
	{
		// if the building needs this material or the truck has some
		// BUGBUG to fix "lazy trucks" 
		if( m_iStore[i] || pBldg->GetParam(i) )
		{
			// record the material quantity from truck to be transferred
			msg.m_aiMat[i] = m_iStore[i];

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d transport truck %ld delivered %d of %d at building %ld ", 
		m_iPlayer, pTruck->GetID(), msg.m_aiMat[i], i, pBldg->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"which showed needing %d of %d ", pBldg->GetParam(i), i );
#endif

			// update the buildings record of need considering that
			// more of the desired material may have been delivered
			// than what was needed by the building
			if( msg.m_aiMat[i] > pBldg->GetParam(i) )
				pBldg->SetParam(i,0);
			else
				pBldg->SetParam( i, (pBldg->GetParam(i) - msg.m_aiMat[i]) );

			// and assigned truck id
			pBldg->SetParamDW( i, 0);
		}
		else // truck has none of this material and building does not need?
		{
			// but the building still needs some
			// and that was the truck to deliver it
			// so make sure the building unassigns
			// that truck from that material for
			// use by SetUnitPriority() later
			if( pBldg->GetParam(i) > 0 &&
				pBldg->GetParamDW(i) == pTruck->GetID() )
				pBldg->SetParamDW(i,0);
		}

		// consider if building still needs something
		if( pBldg->GetParam(i) > 0 )
		{
			bStillNeeds = TRUE;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d building %ld ", 
		m_iPlayer, pBldg->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"which showed still needing %d of %d ", pBldg->GetParam(i), i );
#endif
		}
	}
	// send message to the game to cause the transfer
	//theGame.PostToServer( (CNetCmd *)&msg, sizeof(CMsgTransMat) );
	theGame.PostToClient( m_iPlayer, (CNetCmd *)&msg, sizeof(CMsgTransMat) );

	// now unassign the truck
	pTruck->SetDataDW(0);
	pTruck->ClearParam();
	pTruck->SetStatus(0);
	// and add back to the available truck list
	if( m_plTrucksAvailable->GetUnitNY(pTruck->GetID()) == NULL )
		m_plTrucksAvailable->AddTail( (CObject *)pTruck );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d transport truck %ld unassigned and available and building needs %d \n", 
		m_iPlayer, pTruck->GetID(), (int)bStillNeeds );
#endif

	if( bStillNeeds )
		SetUnitPriority( pBldg );
	else // remove building from the building needs list
		m_plBldgsNeed->RemoveUnit( pBldg->GetID(), FALSE );

	return( bStillNeeds );
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
void CHPRouter::LoadMaterials( CAIUnit *pTruck, CAIHex *paiHex )
{
	// get current capacity of the truck
	int iCapacity = GetMaterialCapacity(pTruck);
	BOOL bMoreSources = FALSE;
	BOOL bHasLoaded = FALSE;
	BOOL bSpecialReserve = FALSE;
	CAIUnit *pBldg = NULL;

	// game message
	CMsgTransMat msg;
	msg.m_dwIDSrc = paiHex->m_dwUnitID;
	msg.m_dwIDDest = pTruck->GetID();
	memset( msg.m_aiMat, 0, sizeof (msg.m_aiMat));

	for( int i=0; i<CMaterialTypes::num_types; ++i )
	{
		if( !pTruck->GetParamDW(i) )
			continue;

		// this flag is needed because on a load game, the router
		// is having to re-init all units, meanining prior truck
		// cargo and assignments are lost and this truck will have
		// all its params zeroed.
		bHasLoaded = TRUE;

		// we only want materials to get from this building
		if( pTruck->GetParamDW(i) == paiHex->m_dwUnitID )
		{
			// find the building for this material, could be more 
			// than one material at this building
			pBldg = m_plUnits->GetUnitNY( paiHex->m_dwUnitID );
			if( pBldg == NULL )
				continue;

			EnterCriticalSection (&cs);
			CBuilding *pBuilding = 
				theBuildingMap.GetBldg( pBldg->GetID() );
			if( pBuilding != NULL )
			{
				// record what's in storage of this material at this building
				for( int j=0; j<CMaterialTypes::num_types; ++j )
					m_iStore[j] = pBuilding->GetStore(j);
			}
			else
			{
				LeaveCriticalSection (&cs);
				continue;
			}
			LeaveCriticalSection (&cs);

			// get the lesser of what's needed vs. what's at the building
			int iQtyNeeded = 
				min( pTruck->GetParam(i), m_iStore[i] );

			// consider if this is the rocket
			if( pBldg->GetType() == CUnit::building &&
				pBldg->GetTypeUnit() == CStructureData::rocket )
			{
				// since is is, then there may be a special reserve status
				// in place, so check the building to which the truck is
				// assigned to determine if it is a special building
				CAIUnit *pBldgAssigned = 
					m_plUnits->GetUnitNY( pTruck->GetDataDW() );
				if( pBldgAssigned != NULL )
				{
					if( pBldgAssigned->GetTypeUnit() == CStructureData::lumber 
						&& m_bLastLumber )
					{
						bSpecialReserve = TRUE;
						m_bLastLumber = FALSE;
					}
					else if( pBldgAssigned->GetTypeUnit() == CStructureData::coal
						&& m_bLastCoal )
					{
						bSpecialReserve = TRUE;
						m_bLastCoal = FALSE;
					}
					else if( pBldgAssigned->GetTypeUnit() == CStructureData::iron
						&& m_bLastIron )
					{
						bSpecialReserve = TRUE;
						m_bLastIron = FALSE;
					}
					else if( pBldgAssigned->GetTypeUnit() == CStructureData::smelter
						&& m_bLastSmelter )
					{
						bSpecialReserve = TRUE;
						m_bLastSmelter = FALSE;
					}
				}
			}

			if( !bSpecialReserve )
			{
			// dave does not want materials distributed around
			// the existing buildings so do not use the capacity
			//
			// record this material quantity to be transferred
			msg.m_aiMat[i] = min( iQtyNeeded, iCapacity );
			//msg.m_aiMat[i] = iQtyNeeded;
			iCapacity -= msg.m_aiMat[i];

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d transport truck %ld loaded %d of %d from source building %ld ", 
		m_iPlayer, pTruck->GetID(), msg.m_aiMat[i], i, pBldg->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::truck wanted %d, source building had %d, capacity left %d ",
		pTruck->GetParam(i), m_iStore[i], iCapacity );
#endif
			}
			else // a special reserve status is set so plug in the materials
			{
				EnterCriticalSection (&cs);
				CVehicle *pVehicle = 
					theVehicleMap.GetVehicle( pTruck->GetID() );
				if( pVehicle != NULL )
				{
					// reserve is the difference between what is there
					// and what is needed
					if( pTruck->GetParam(i) > m_iStore[i] )
					{
						// reserve materials created out of thin bits
						pVehicle->AddToStore( i, 
							(pTruck->GetParam(i) - m_iStore[i]) );

						// set up message to load difference
						msg.m_aiMat[i] = m_iStore[i];
					}
					else
						msg.m_aiMat[i] = pTruck->GetParam(i);
							
				}
				LeaveCriticalSection (&cs);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d transport truck %ld loaded %d of %d from source building %ld ", 
		m_iPlayer, pTruck->GetID(), msg.m_aiMat[i], i, pBldg->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::truck needed %d of reserve materials and got them ",
		(pTruck->GetParam(i) - m_iStore[i]) );
#endif
			}

			// clear the truck so that it can be directed to
			// the next source building destination
			pTruck->SetParamDW(i,0);
			
		}
		else
			bMoreSources = TRUE;
	}


	// send message to the game to cause the transfer to occur
	if( bHasLoaded ) //&& !bSpecialReserve )
	{
		//theGame.PostToServer( (CNetCmd *)&msg, sizeof(CMsgTransMat) );
		theGame.PostToClient( m_iPlayer, (CNetCmd *)&msg, sizeof(CMsgTransMat) );
	}
	else
	{
		// nothing loaded so unassign the truck
		pTruck->SetDataDW(0);
		pTruck->ClearParam();
		pTruck->SetStatus(0);
		// and add back to the available truck list
		if( m_plTrucksAvailable->GetUnitNY(pTruck->GetID()) == NULL )
			m_plTrucksAvailable->AddTail( (CObject *)pTruck );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d transport truck %ld unassigned and available \n", 
		m_iPlayer, pTruck->GetID() );
#endif
		return;
	}

	
	// if all sources have been visited, then it is time to go
	// to the building with the needed materials
	if( !bMoreSources )
	{
		pBldg = m_plUnits->GetUnitNY( pTruck->GetDataDW() );
		if( pBldg != NULL )
		{
			CHexCoord hex;
			if( !GetBldgExit(pBldg->GetID(),hex) )
			{
				pTruck->SetStatus( 0 );	// truck is no longer assigned
				pTruck->SetDataDW( 0 );
				return;
			}

			pTruck->SetParam(CAI_ROUTE_X,hex.X());
			pTruck->SetParam(CAI_ROUTE_Y,hex.Y());

		// this considers that a path to the current source destination
		// needs a ship, to be reached and if so, direct the truck
		// to a pickup point and a ship to pick it up
			ConsiderLandWater( pTruck, hex );

			SetDestination( pTruck->GetID(), hex );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d transport truck %ld enroute to assigned building %ld ", 
		m_iPlayer, pTruck->GetID(), pBldg->GetID() );
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::which shows currently needing:" );

	for( i=0; i<CMaterialTypes::num_types; ++i )
	{
		logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
			" material %d in quantity of %d", i, pBldg->GetParam(i) );
	}
#endif
		}
		return;
	}

	// now figure out the nearest remaining source for material
	// and send a message to direct the truck to it
	int iBestDist = 0xFFFF;
	CAIUnit *pBestBldg = NULL;
	CHexCoord hexTruck( paiHex->m_iX, paiHex->m_iY );
	CHexCoord hexBldg,hexBest;

	for( i=0; i<CMaterialTypes::num_types; ++i )
	{
		if( pTruck->GetParamDW(i) )
		{
			// get CAIUnit for the next source building 
			pBldg = m_plUnits->GetUnitNY( pTruck->GetParamDW(i) );
			if( pBldg == NULL )
			{
				pTruck->SetParamDW(i,0);
				continue;
			}
			GetBldgExit(pBldg->GetID(),hexBldg);

			// get dist to reach building
			int iDist = theMap.GetRangeDistance( hexTruck, hexBldg );

			// find the one closest and save it
			if( iDist && iDist < iBestDist )
			{
				iBestDist = iDist;
				pBestBldg = pBldg;
				hexBest = hexBldg;
			}
		}
	}
	// engage
	if( pBestBldg != NULL )
	{
		// saves the location of the building in case a route is needed
		pTruck->SetParam(CAI_ROUTE_X,hexBest.X());
		pTruck->SetParam(CAI_ROUTE_Y,hexBest.Y());

		// this considers that a path to the current source destination
		// needs a ship, to be reached and if so, direct the truck
		// to a pickup point and a ship to pick it up
		ConsiderLandWater( pTruck, hexBest );

		SetDestination( pTruck->GetID(), hexBest );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_HP_ROUTER, 
	"CHPRouter::player %d transport truck %ld enroute to source building %ld \n", 
		m_iPlayer, pTruck->GetID(), pBestBldg->GetID() );
#endif
	}
}

//
// this will cause an out_mat message to be faked
//
BOOL CHPRouter::BuildingNeedsAlways( CAIUnit *pBldg )
{
	BOOL bNeedsAlways = FALSE;
	switch( pBldg->GetTypeUnit() )
	{
		case CStructureData::power_1:
		case CStructureData::power_2:
		//case CStructureData::power_3:
		case CStructureData::refinery:
		case CStructureData::smelter:
			bNeedsAlways = TRUE;
		default:
			break;
	}
	return( bNeedsAlways );
}

//
// determine if the dwID passed belongs to a carrier ship
// 
BOOL CHPRouter::IsShip( DWORD dwID )
{
	EnterCriticalSection (&cs);

	int iVeh = 0;
	CVehicle *pVehicle = theVehicleMap.GetVehicle( dwID );
	if( pVehicle != NULL )
		iVeh = pVehicle->GetData()->GetType();

	LeaveCriticalSection (&cs);

	//if( iVeh == CTransportData::landing_craft ||
	//	iVeh == CTransportData::light_cargo )
	if( iVeh == CTransportData::light_cargo )
		return TRUE;

	return FALSE;
}

//
// determine if the dwID passed belongs to a truck
// 
BOOL CHPRouter::IsTruck( DWORD dwID )
{
	EnterCriticalSection (&cs);

	int iVeh = 0;
	CVehicle *pVehicle = theVehicleMap.GetVehicle( dwID );
	if( pVehicle != NULL )
		iVeh = pVehicle->GetData()->GetType();

	LeaveCriticalSection (&cs);

	if( iVeh == CTransportData::med_truck ||
		iVeh == CTransportData::heavy_truck )
		return TRUE;

	return FALSE;
}


void CHPRouter::Save( CFile *pFile )
{
	//ar.Flush();
	//CFile *pFile = ar.GetFile();

	try
	{
		pFile->Write( (const void*)&m_dwRocket, sizeof(DWORD) );
	}
    catch( CFileException theException )
    {
		// how should write errors be reported?
    	throw(ERR_CAI_BAD_FILE);
    }

	try
	{
		pFile->Write( (const void*)&m_iPlayer, sizeof(int) );
		pFile->Write( (const void*)&m_bLastLumber, sizeof(BOOL) );
		pFile->Write( (const void*)&m_bLastCoal, sizeof(BOOL) );
		pFile->Write( (const void*)&m_bLastIron, sizeof(BOOL) );
		pFile->Write( (const void*)&m_bLastSmelter, sizeof(BOOL) );
	}
    catch( CFileException theException )
    {
		// how should write errors be reported?
    	throw(ERR_CAI_BAD_FILE);
    }

	// save the units
	m_plUnits->Save( pFile );

	// save the count and just the DWORD id of units in the
	// the available truck and building needs lists 

	// first do the trucks
	int iCnt = m_plTrucksAvailable->GetCount();
	try
	{
		pFile->Write( (const void*)&iCnt, sizeof(int) );
	}
    catch( CFileException theException )
    {
		// how should write errors be reported?
    	throw(ERR_CAI_BAD_FILE);
    }
	if( iCnt )
	{
       	POSITION pos = m_plTrucksAvailable->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIUnit *pUnit = (CAIUnit *)m_plTrucksAvailable->GetNext( pos );
            if( pUnit != NULL )
            {
				DWORD dwID = pUnit->GetID();
				try
				{
					pFile->Write( (const void*)&dwID, sizeof(DWORD) );
				}
    			catch( CFileException theException )
    			{
					// how should write errors be reported?
    				throw(ERR_CAI_BAD_FILE);
    			}
			}
		}
	}

	// now do the ships
	
	iCnt = m_plShipsAvailable->GetCount();
	try
	{
		pFile->Write( (const void*)&iCnt, sizeof(int) );
	}
    catch( CFileException theException )
    {
		// how should write errors be reported?
    	throw(ERR_CAI_BAD_FILE);
    }
	if( iCnt )
	{
       	POSITION pos = m_plShipsAvailable->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIUnit *pUnit = (CAIUnit *)m_plShipsAvailable->GetNext( pos );
            if( pUnit != NULL )
            {
				DWORD dwID = pUnit->GetID();
				try
				{
					pFile->Write( (const void*)&dwID, sizeof(DWORD) );
				}
    			catch( CFileException theException )
    			{
					// how should write errors be reported?
    				throw(ERR_CAI_BAD_FILE);
    			}
			}
		}
	}

	// now do the buildings
	iCnt = m_plBldgsNeed->GetCount();
	try
	{
		pFile->Write( (const void*)&iCnt, sizeof(int) );
	}
    catch( CFileException theException )
    {
		// how should write errors be reported?
    	throw(ERR_CAI_BAD_FILE);
    }
	if( iCnt )
	{
       	POSITION pos = m_plBldgsNeed->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIUnit *pUnit = (CAIUnit *)m_plBldgsNeed->GetNext( pos );
            if( pUnit != NULL )
            {
				DWORD dwID = pUnit->GetID();
				try
				{
					pFile->Write( (const void*)&dwID, sizeof(DWORD) );
				}
    			catch( CFileException theException )
    			{
					// how should write errors be reported?
    				throw(ERR_CAI_BAD_FILE);
    			}
			}
		}
	}
}

void CHPRouter::Load( CFile *pFile )
{
	/*
	ar.Flush();
	CFile *pFile = ar.GetFile();
	if( pFile == NULL )
		throw(ERR_CAI_BAD_FILE);
	*/

	try
	{
		pFile->Read( (void*)&m_dwRocket, sizeof(DWORD) );
	}
    catch( CException anException )
    {
		// how should read errors be reported?
        throw(ERR_CAI_BAD_FILE);
    }
	
	// first get the player id
	try
	{
		pFile->Read( (void*)&m_iPlayer, sizeof(int) );
		pFile->Read( (void*)&m_bLastLumber, sizeof(BOOL) );
		pFile->Read( (void*)&m_bLastCoal, sizeof(BOOL) );
		pFile->Read( (void*)&m_bLastIron, sizeof(BOOL) );
		pFile->Read( (void*)&m_bLastSmelter, sizeof(BOOL) );
	}
    catch( CException anException )
    {
		// how should read errors be reported?
        throw(ERR_CAI_BAD_FILE);
    }

	// load units
	if( m_plUnits != NULL )
	{
		m_plUnits->RemoveAll();
		delete m_plUnits;
	}

	// create a list to contain all player's units
    try
    {
    	m_plUnits = new CAIUnitList();
    }
    catch( CException e )
    {
    	if( m_plUnits != NULL )
    	{
    		delete m_plUnits;
    		m_plUnits = NULL;
    	}
		throw(ERR_CAI_BAD_NEW);
    }
	// and load them
	m_plUnits->Load( pFile );

	
	// make sure lists are empty and valid
	if( m_plTrucksAvailable != NULL )
	{
		m_plTrucksAvailable->RemoveAll();
	}
	else // need to allocate for the list
	{
		try
		{
			m_plTrucksAvailable = new CAIUnitList();
		}
		catch( CException e )
		{
			// how should memory errors be reported?
        	throw(ERR_CAI_BAD_NEW);
		}
	}

	if( m_plShipsAvailable != NULL )
	{
		m_plShipsAvailable->RemoveAll();
	}
	else // need to allocate for the list
	{
		try
		{
			m_plShipsAvailable = new CAIUnitList();
		}
		catch( CException e )
		{
			// how should memory errors be reported?
        	throw(ERR_CAI_BAD_NEW);
		}
	}
	

	if( m_plBldgsNeed != NULL )
	{
		m_plBldgsNeed->RemoveAll();
	}
	else // need to allocate for the list
	{
		try
		{
			m_plBldgsNeed = new CAIUnitList();
		}
		catch( CException e )
		{
			// how should memory errors be reported?
        	throw(ERR_CAI_BAD_NEW);
		}
	}

	// the available truck and building needs lists do not create
	// new copies of the CAIUnits that are contained within, but
	// instead rely on those existing instances in the CAIMgr's
	// m_plUnits to provide the actual pointer to the instance
	int iCnt;
	DWORD dwID;
	// now get count of trucks
	try
	{
		pFile->Read( (void*)&iCnt, sizeof(int) );
	}
    catch( CException anException )
    {
		// how should read errors be reported?
        throw(ERR_CAI_BAD_FILE);
    }
	// and read them one at a time
	for( int i=0; i<iCnt; ++i )
	{
		try
		{
			pFile->Read( (void*)&dwID, sizeof(DWORD) );
		}
    	catch( CException anException )
    	{
			// how should read errors be reported?
        	throw(ERR_CAI_BAD_FILE);
    	}
		CAIUnit *pUnit = m_plUnits->GetUnitNY(dwID );
		if( pUnit != NULL )
			m_plTrucksAvailable->AddTail( (CObject *)pUnit );
	}

	
	// get a count of the ships available
	try
	{
		pFile->Read( (void*)&iCnt, sizeof(int) );
	}
    catch( CException anException )
    {
		// how should read errors be reported?
        throw(ERR_CAI_BAD_FILE);
    }
	// and read them one at a time
	for( i=0; i<iCnt; ++i )
	{
		try
		{
			pFile->Read( (void*)&dwID, sizeof(DWORD) );
		}
    	catch( CException anException )
    	{
			// how should read errors be reported?
        	throw(ERR_CAI_BAD_FILE);
    	}
		CAIUnit *pUnit = m_plUnits->GetUnitNY(dwID );
		if( pUnit != NULL )
			m_plShipsAvailable->AddTail( (CObject *)pUnit );
	}

	// now get count of buildings in need
	try
	{
		pFile->Read( (void*)&iCnt, sizeof(int) );
	}
    catch( CException anException )
    {
		// how should read errors be reported?
        throw(ERR_CAI_BAD_FILE);
    }
	// and read them one at a time
	for( i=0; i<iCnt; ++i )
	{
		try
		{
			pFile->Read( (void*)&dwID, sizeof(DWORD) );
		}
    	catch( CException anException )
    	{
			// how should read errors be reported?
        	throw(ERR_CAI_BAD_FILE);
    	}
		CAIUnit *pUnit = m_plUnits->GetUnitNY(dwID );
		if( pUnit != NULL )
			m_plBldgsNeed->AddTail( (CObject *)pUnit );
	}

	// now, need to clear and load counts of special units
	m_wSeaportCnt = 0;
	m_wLumberCnt = 0;
	m_wCoalCnt = 0;
	m_wIronCnt = 0;
	m_wSmelterCnt = 0;

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

			if( pUnit->GetTypeUnit() == CStructureData::seaport )
				m_wSeaportCnt++;
			if( pUnit->GetTypeUnit() == CStructureData::lumber )
				m_wLumberCnt++;
			if( pUnit->GetTypeUnit() == CStructureData::coal )
				m_wCoalCnt++;
			if( pUnit->GetTypeUnit() == CStructureData::iron )
				m_wIronCnt++;
			if( pUnit->GetTypeUnit() == CStructureData::smelter )
				m_wSmelterCnt++;
		}
	}
}

BOOL CHPRouter::GetStagingHex( CAIUnit *paiTruck, 
	CAIUnit *paiBldg, CHexCoord& hexDest )
{
	int iWidth = 2;
	int iHeight = 2;
	CHexCoord hexNearBy;

	if( paiBldg != NULL )
	{
		EnterCriticalSection (&cs);
		CBuilding *pBldg = theBuildingMap.GetBldg( paiBldg->GetID() );
		if( pBldg == NULL )
		{
			LeaveCriticalSection (&cs);
			return( FALSE );
		}
		hexNearBy = pBldg->GetExitHex();
		// get building size
		iWidth = pBldg->GetData()->GetCX();
		iHeight = pBldg->GetData()->GetCY();
		LeaveCriticalSection (&cs);
	}
	else
	{
		EnterCriticalSection (&cs);
		CVehicle *pVehicle = 
			theVehicleMap.GetVehicle( paiTruck->GetID() );
		if( pVehicle == NULL )
		{
			LeaveCriticalSection (&cs);
			return( FALSE );
		}
		hexNearBy = pVehicle->GetHexHead();
		LeaveCriticalSection (&cs);
	}

	// get location of vehicle, and set up to find paths
	CHexCoord hexVeh;
	if( !GetVehicleHex( paiTruck->GetID(), hexVeh ) )
		return( FALSE );

	CHexCoord *pPath = NULL;
	int iPathLen = 0;
	CHexCoord *pPathHex = NULL;

	// can the vehicle at least, get to the nearby hex?
	BOOL bCanGetThere = FALSE;
	// run a path out to the candidate seaport from this seaport
	// using the ship as a vehicle
	pPath = thePathMgr.GetPath( 
		NULL, hexVeh, hexNearBy, iPathLen, paiTruck->GetTypeUnit(),
		FALSE, TRUE );
	if( pPath != NULL )
	{
		// check last step
		if( iPathLen )
			iPathLen--;

		pPathHex = &pPath[iPathLen];
		if( pPathHex->X() == hexNearBy.X() &&
			pPathHex->Y() == hexNearBy.Y() )
			bCanGetThere = TRUE;

		delete [] pPath;
	}
	if( hexVeh == hexNearBy )
		bCanGetThere = TRUE;
	if( !bCanGetThere )
		return( FALSE );


	// get pointer to vehicle type data
	CTransportData const *pVehData = 
		theTransports.GetData( paiTruck->GetTypeUnit() );
	if( pVehData == NULL )
		return( FALSE );

	CHexCoord hcFrom,hcTo,hcAt;

	// first time thru, look only for plain or road terrain
	// and then if no hex found, try for any terrain
	BOOL bIsTruck = IsTruck( paiTruck->GetID() );
	BOOL bPlainRoad = TRUE;

TryAgain:

	// a spiral search outward from the building's exit
	int iStep = max( iWidth, iHeight ) + 1;
	int iMaxSteps = (iWidth * iHeight) + iStep;
	while( iStep < iMaxSteps )
	{
		hcFrom.X( hexNearBy.Wrap(hexNearBy.X()-iStep) );
		hcFrom.Y( hexNearBy.Wrap(hexNearBy.Y()-iStep) );
		hcTo.X( hexNearBy.Wrap(hexNearBy.X()+iStep) );
		hcTo.Y( hexNearBy.Wrap(hexNearBy.Y()+iStep) );

		int iDeltax = hexNearBy.Wrap(hcTo.X()-hcFrom.X()) + 1;
		int iDeltay = hexNearBy.Wrap(hcTo.Y()-hcFrom.Y()) + 1;

		for( int iY=0; iY<iDeltay; ++iY )
		{
			hcAt.Y( hexNearBy.Wrap(hcFrom.Y()+iY) );

			for( int iX=0; iX<iDeltax; ++iX )
			{
				hcAt.X( hexNearBy.Wrap(hcFrom.X()+iX) );

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
				// and vehicles
				if( (bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr)) )
					continue;

				// check the game to see if vehicle can travel
				if( !pVehData->CanTravelHex( pGameHex ) )
					continue;

				// for trucks only, allow only plains or road
				if( bIsTruck && bPlainRoad )
				{
					// now dave wants 1st plains,desert,rough,forest
					// that is next to a road, so only consider roads
					if( pGameHex->GetType() != CHex::road )
						continue;

					// check for terrain adjacent to this road and
					// return open hex in hcAt
					if( !GetNearRoad(hcAt) )
						continue;

				}
				if( pGameHex->GetType() == CHex::coastline )
					continue;

				hexDest = hcAt;
				return( TRUE );
			}
		}
		iStep++;
	}

	// for trucks only, consider if a try has been made
	// to just find plains or roads
	if( bIsTruck && bPlainRoad )
	{
		bPlainRoad = FALSE;
		goto TryAgain;
	}

	return( FALSE );
}

// check for terrain adjacent to this road and
// return open hex in hcAt
//
BOOL CHPRouter::GetNearRoad( CHexCoord& hexRoad )
{
	// for each direction
	for( int i=0; i<MAX_ADJACENT; ++i )
	{
		CHexCoord hex = hexRoad;

		switch(i)
		{
			case 0:	// north
				hex.Ydec();
				break;
			case 1:	// northeast
				hex.Ydec();
				hex.Xinc();
				break;
			case 2:
				hex.Xinc();
				break;
			case 3:
				hex.Yinc();
				hex.Xinc();
				break;
			case 4:
				hex.Yinc();
				break;
			case 5:
				hex.Yinc();
				hex.Xdec();
				break;
			case 6:
				hex.Xdec();
				break;
			case 7:
				hex.Xdec();
				hex.Ydec();
				break;
			default:
				return FALSE;
		}

		// get terrain from game data
		CHex *pGameHex = theMap.GetHex( hex );
		BYTE bUnits = pGameHex->GetUnits();
		// buildings is here
		if( (bUnits & CHex::bldg) )
			continue;
		// vehicles are here
		if( (bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr)) )
			continue;

		// must be plains,desert,rough,forest
		if( pGameHex->GetType() != CHex::plain &&
			pGameHex->GetType() != CHex::desert &&
			pGameHex->GetType() != CHex::rough &&
			pGameHex->GetType() != CHex::forest )
			continue;

		// and not near a building
		if( IsRoadNearBuilding(hexRoad) )
			continue;

		// this hex is adjacent to a road, on the correct terrain,
		// is open, and is not near a building
		hexRoad = hex;
		return TRUE;
	}
	return FALSE;
}

//
// the passed hex is supposed to be a candidate road hex for
// staging, so check to make sure there are no buildings adj
// and return TRUE if building found adjacent
//
BOOL CHPRouter::IsRoadNearBuilding( CHexCoord& hexRoad )
{
	// for each direction
	for( int i=0; i<MAX_ADJACENT; ++i )
	{
		CHexCoord hex = hexRoad;

		switch(i)
		{
			case 0:	// north
				hex.Ydec();
				break;
			case 1:	// northeast
				hex.Ydec();
				hex.Xinc();
				break;
			case 2:
				hex.Xinc();
				break;
			case 3:
				hex.Yinc();
				hex.Xinc();
				break;
			case 4:
				hex.Yinc();
				break;
			case 5:
				hex.Yinc();
				hex.Xdec();
				break;
			case 6:
				hex.Xdec();
				break;
			case 7:
				hex.Xdec();
				hex.Ydec();
				break;
			default:
				return FALSE;
		}
			
		// get terrain from game data
		CHex *pGameHex = theMap.GetHex( hex );
		BYTE bUnits = pGameHex->GetUnits();
		// buildings is adjacent
		if( (bUnits & CHex::bldg) )
			return TRUE;
	}
	return FALSE;
}

// 
// count the number of buildings in the list of this
// special type of building
//
int CHPRouter::CountSpecialUnits( int iTypeUnit )
{
	int iCnt = 0;
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

			if( pUnit->GetTypeUnit() == iTypeUnit )
				iCnt++;
		}
	}
	return( iCnt );
}

//
// use this specifically for HP Router set destinations instead
// of the CAIUnit::SetDestination()
//
void CHPRouter::SetDestination( DWORD dwID, CHexCoord& hexDest )
{
	CMsgVehSetDest msg( dwID, hexDest, CVehicle::moving );
	theGame.PostToClient( m_iPlayer, (CNetCmd *)&msg, sizeof(CMsgVehSetDest) );
}

//
// issue the message to load the cargo unit
//
void CHPRouter::LoadUnit( DWORD dwCarrier, DWORD dwCargo )
{
/* // load a carrier
class CMsgLoadCarrier : public CNetCmd
{
public:
		CMsgLoadCarrier () : CNetCmd (load_carrier) {}
		CMsgLoadCarrier (CVehicle const * pCargo, CVehicle const * pCarrier);

	DWORD		m_dwIDCargo;			// unit to carry
	DWORD		m_dwIDCarrier;		// unit carrying it
};
*/
	CMsgLoadCarrier msg;
	msg.m_dwIDCargo = dwCargo;
	msg.m_dwIDCarrier = dwCarrier;
	theGame.PostToClient( m_iPlayer, (CNetCmd *)&msg, sizeof(CMsgLoadCarrier) );
}

//
// issue the message to unload the carrier unit
//
void CHPRouter::UnloadCargo( DWORD dwCarrier )
{
	CMsgUnloadCarrier msg;
	msg.m_dwID = dwCarrier;
	
	theGame.PostToClient( m_iPlayer, (CNetCmd *)&msg, sizeof(CMsgUnloadCarrier) );
}

// end of CHPRoute.cpp

