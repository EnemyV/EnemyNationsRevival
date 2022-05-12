////////////////////////////////////////////////////////////////////////////
//
//  CAIMap.cpp : CAIMap object implementation
//               Divide and Conquer AI
//               
//  Last update:    09/13/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "netapi.h"
#include "CAIMap.hpp"
#include "CAIData.hpp"
#include "CPathMap.h"

#include "logging.h"	// dave's logging system

extern CAIData *pGameData;		// pointer to API object for game data
extern CPathMap thePathMap;		// the map pathfinding object (no yield)
extern CException *pException;	// standard exception for yielding
extern CRITICAL_SECTION cs;	// used by threads

#define new DEBUG_NEW

//
// this class maintains a block of locations, in start block size chunks
//

CAIMap::~CAIMap( void )
{
	ASSERT_VALID( this );

	if( m_pMapUtil != NULL )
	{
		delete m_pMapUtil;
		m_pMapUtil = NULL;
	}

	if( m_pwaMap != NULL )
		delete [] m_pwaMap;
}

CAIMap::CAIMap( int iPlayer, CAIUnitList *pUnits,
	WORD wBaseCol, WORD wBaseRow, WORD wCols, WORD wRows )
{
	m_iPlayer = iPlayer;
	m_pwaMap = NULL;

	m_wRows = wRows;
	m_wCols = wCols;

	m_iBaseX = (int)wBaseCol;
	m_iBaseY = (int)wBaseRow;

	m_wBaseRow = 0;
	m_wBaseCol = 0;

	m_cMainRoads = (BYTE)0;

	m_iRoadCount = 0;
	m_iOcean = 0;
	m_iLake = 0;
	m_iLand = 0;
	
	Initialize();
	
	m_pMapUtil = new CAIMapUtil( m_pwaMap, pUnits, m_iBaseX, m_iBaseY,
		m_wBaseCol, m_wBaseRow, m_wCols, m_wRows, iPlayer );

	UpdateMap(NULL);

	ASSERT_VALID( this );
}

WORD CAIMap::GetRows( void )
{
	ASSERT_VALID( this );
	return m_wRows;
}

WORD CAIMap::GetCols( void )
{
	ASSERT_VALID( this );
	return m_wCols;
}

int CAIMap::GetPlayer( void )
{
	ASSERT_VALID( this );
	return m_iPlayer;
}

void CAIMap::SetMainRoad( BYTE cLayout )
{
	ASSERT_VALID( this );
	m_cMainRoads = cLayout;
}

BYTE CAIMap::GetMainRoad( void )
{
	ASSERT_VALID( this );
	return m_cMainRoads;
}

void CAIMap::ConfirmPlacement( CHexCoord& hex )
{
	int i = m_pMapUtil->GetMapOffset( hex.X(), hex.Y() );
	if( i >= m_iMapSize )
		return;

	WORD wStatusUtl = m_pMapUtil->GetStatus(i);
	WORD wStatusMap = m_pwaMap[i];
	if( wStatusUtl != wStatusMap )
		TRACE( "Map is corrupt at %d,%d \n\n", hex.X(), hex.Y() );
}

//
// BUGBUG just place the vehicle in the map's m_pwaMap[]
//
void CAIMap::PlaceFakeVeh( CHexCoord& hex, int iVeh )
{
	int i = m_pMapUtil->GetMapOffset( hex.X(), hex.Y() );
	if( i >= m_iMapSize )
		return;

	WORD wStatus = m_pwaMap[i];
	wStatus |= MSW_AI_VEHICLE;
	wStatus |= MSW_KNOWN;
	WORD wTemp = wStatus;
	wStatus = iVeh << 8;
	wStatus |= wTemp;
	m_pwaMap[i] = wStatus;
}

//
// BUGBUG unit ASSERT failure with placing buildings is fixed
// use this routine to update the AI map for building placement
//
void CAIMap::PlaceFakeBldg( CHexCoord& hex, int iBldg )
{
	int iWidthX, iWidthY;
	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData != NULL )
	{
		iWidthX = pBldgData->GetCX();
		iWidthY = pBldgData->GetCY();
	}
	else
		return;

	CHexCoord hexFake;
	
	for( int iY=0; iY<iWidthY; ++iY )
	{
		hexFake.Y( hex.Wrap(hex.Y()+iY) );

		for( int iX=0; iX<iWidthX; ++iX )
		{
			
			hexFake.X( hex.Wrap(hex.X()+iX) );

			int i = m_pMapUtil->GetMapOffset( hexFake.X(), hexFake.Y() );
			if( i >= m_iMapSize )
				return;

			WORD wStatus = m_pwaMap[i];
			if( wStatus & MSW_PLANNED_ROAD )
				wStatus ^= MSW_PLANNED_ROAD;
			if( wStatus & MSW_ROAD )
				wStatus ^= MSW_ROAD;
			
			wStatus |= MSW_AI_BUILDING;
			wStatus |= MSW_KNOWN;
			WORD wTemp = wStatus;
			wStatus = iBldg << 8;
			wStatus |= wTemp;

			m_pwaMap[i] = wStatus;

			// update city bounds
			m_pMapUtil->UpdateCityBounds( hexFake.X(), hexFake.Y() );
		}
	}
}

#ifdef _LOGOUT
void CAIMap::ReportFakeMap( void )
{
	// BUGBUG for testing, report the MAP AND roads
	m_pMapUtil->ReportPavedRoads();
}
#endif

//
// updates the m_pwaMap offset associated with
// the hex passed, based on game data
//
//	m_wBaseRow
//	m_wBaseCol
//	m_wRows
//	m_wCols
//
void CAIMap::UpdateHex( int iX, int iY )
{
	// determine offset in m_pwaMap that
	// represents the iX,iY hex
	// BUGBUG this calculation needs to be proved
	int i = m_pMapUtil->GetMapOffset( iX, iY );
	if( i >= m_iMapSize )
		return;

	// get location from game data
	// create AI copy
	CAIHex aiHex( iX, iY );
	// get location data from game data
	pGameData->GetCHexData(&aiHex);
	// determine status word for that location
	WORD wStatus = GetLocation( aiHex.m_iX, aiHex.m_iY );
	// examine location data and update status
	wStatus = m_pMapUtil->ConvertStatus( &aiHex, wStatus );
	// update map array with status
	SetLocation( aiHex.m_iX, aiHex.m_iY, wStatus );
}

//
// update the local map with known status, reflecting
// that a vehicle has entered a location
//
void CAIMap::UpdateLoc(	CAIMsg *pMsg )
{
	WORD wStatus = GetLocation( pMsg->m_iX, pMsg->m_iY );
	wStatus |= MSW_KNOWN;
	SetLocation( pMsg->m_iX, pMsg->m_iY, wStatus );
}

//
// retreive the current status of the game map,
// process it into a status word and save the
// word in the map array
//
void CAIMap::UpdateMap( CAIMsg *pMsg )
{
	ASSERT_VALID( this );
	
	if( m_pwaMap == NULL )
		return;

	WORD wStatus;

	if( pMsg != NULL )
	{
		if( pMsg->m_iMsg == CNetCmd::bldg_stat ||
			pMsg->m_iMsg == CNetCmd::road_done ||
			pMsg->m_iMsg == CNetCmd::err_build_bldg )
		{
			// get status before update of new/dead unit
			wStatus = GetLocation( pMsg->m_iX, pMsg->m_iY );

			// now update status to reflect hex
			UpdateHex( pMsg->m_iX, pMsg->m_iY );
		}

		// now update m_pMap from game data,
		// for hex groups adjacent to message hex 
		CAIHex aiHex( pMsg->m_iX, pMsg->m_iY );
		m_pMapUtil->UpdateAdjacentHexes( &aiHex );

		return;
	}	// end of if( pMsg != NULL )

	// non-null message hex updates should have returned

#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif

	// do a complete update of all hexes on the map
	// create AI copy to use for hexes
	CAIHex aiHex( 0, 0 );
	m_iOcean = 0;
	m_iLand = 0;
	m_iLake = 0;
	
	for( int i=0; i<m_iMapSize; ++i )
	{
		// this throws an exception because it is executing
		// from the game and not from a thread when called
		// at the start of a game

#if THREADS_ENABLED
	myYieldThread();
#endif
		m_pMapUtil->OffsetToXY( i, &aiHex.m_iX, &aiHex.m_iY );
		if( aiHex.m_iX < 0 || aiHex.m_iX >= (int)m_wCols ||
			aiHex.m_iY < 0 || aiHex.m_iY >= (int)m_wRows )
			continue;

		// get location from game data
		pGameData->GetCHexData(&aiHex);

		// help out goalmgr by counting ocean/land
		if( aiHex.m_cTerrain == CHex::ocean )
			++m_iOcean;
		else if( aiHex.m_cTerrain == CHex::lake )
			++m_iLake;
		else
			++m_iLand;

		// determine status word for that location
		wStatus = m_pwaMap[i];

		// examine location data and update status
		wStatus = m_pMapUtil->ConvertStatus( &aiHex, wStatus );

		// update map array with status
		m_pwaMap[i] = wStatus;
	}

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d map update took %ld ticks size is %d ", 
    	m_iPlayer, (dwEnd - dwStart), m_iMapSize );

#endif
}

void CAIMap::Initialize( void )
{
	ASSERT_VALID( this );
	
	m_iMapSize = int(m_wRows * m_wCols);
	try
	{
		m_pwaMap = new WORD[m_iMapSize];
		//memset( void *dest, int c, size_t count );
		memset( m_pwaMap, 0, (size_t)(m_iMapSize * sizeof( WORD )) );
	}
	catch( CException e )
	{
		if( m_pwaMap != NULL )
		{
			delete [] m_pwaMap;
			m_pwaMap = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}
}
//
// return the map status word for this location
//
WORD CAIMap::GetLocation( WORD wCol, WORD wRow )
{
	ASSERT_VALID( this );
	
	if( m_pwaMap == NULL )
		return FALSE;
		
	int i = m_pMapUtil->GetMapOffset( wCol, wRow );
	if( i < m_iMapSize )
		return( m_pwaMap[i] );
	return FALSE;
}
//
// store the status word in the map array for the passed location
//
void CAIMap::SetLocation( WORD wCol, WORD wRow, WORD wStatus )
{
	ASSERT_VALID( this );
	
	if( m_pwaMap == NULL )
		return;
		
	int i = m_pMapUtil->GetMapOffset( wCol, wRow );
	if( i < m_iMapSize )
		m_pwaMap[i] = wStatus;
}

//
// use m_RocketHex and the size of the rocket to determine
// the hexes around the rocket exit, and set only them to 
// be planned roads
//
void CAIMap::RocketRoad( void )
{
	// get size of the rocket
	int iWidth, iHeight;
	CStructureData const *pBldgData = 
		pGameData->GetStructureData( CStructureData::rocket );
	if( pBldgData == NULL )
		return;
	iWidth = pBldgData->GetCX();
	iHeight = pBldgData->GetCY();

	

	// determine the exit of the rocket
	CHexCoord hexExit = m_pMapUtil->m_RocketHex;
	hexExit.Xinc();
	hexExit.Yinc();

	// set the 5 adjacent hexes to the exit to be planned roads
	for( int i=0; i<MAX_ADJACENT; ++i )
	{
		CHexCoord hexRocket = hexExit;
		switch( i )
		{
			case 1:
				hexRocket.Ydec();
				hexRocket.Xinc();
				break;
			case 2:
				hexRocket.Xinc();
				break;
			case 3:
				hexRocket.Yinc();
				hexRocket.Xinc();
				break;
			case 4:
				hexRocket.Yinc();
				break;
			case 5:
				hexRocket.Yinc();
				hexRocket.Xdec();
				break;
			default:
				continue;
		}
		int j = m_pMapUtil->GetMapOffset( hexRocket.X(), hexRocket.Y() );
		if( j >= m_iMapSize )
				continue;

		// a planned road
		if( !(m_pwaMap[j] & MSW_AI_BUILDING) &&
			!(m_pwaMap[j] & MSW_PLANNED_ROAD) )
		{
			m_pwaMap[j] |= MSW_PLANNED_ROAD;
			m_iRoadCount++;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d planned rocket road laid at %d,%d ", 
    	m_iPlayer, hexRocket.X(), hexRocket.Y() );

#endif
		}
	}
}

//
// construct a planned road from the exit hex of the passed building
// nearest exit hex of another building
//
void CAIMap::PlanRoad( DWORD dwID )
{
	CHexCoord hexFrom(0,0);
	int iFromType, iToType=CStructureData::city;
	EnterCriticalSection (&cs);
	CBuilding *pBldg = theBuildingMap.GetBldg( dwID );
	if( pBldg != NULL )
	{
		hexFrom = pBldg->GetExitHex();
		iFromType = pBldg->GetData()->GetType();
	}
	LeaveCriticalSection (&cs);

	if( !hexFrom.X() && !hexFrom.Y() )
		return;

	// use type of building passed, to determine to what other types
	// of buildings to run roads out to
	//
	// coal -> smelter
	// iron -> smelter
	// rocket -> smelter
	// smelter -> light_0 CStructureData::num_types
	// smelter -> light_1
	// smelter -> light_2
	// smelter -> heavy
	// smelter -> barracks_2
	// smelter -> shipyard_1
	// smelter -> shipyard_3
	// oil_well -> refinery
	// lumber -> rocket
	// copper -> heavy
	// copper -> shipyard_3
	//
	switch( iFromType )
	{
	case CStructureData::power_1:
		iToType = CStructureData::coal;
		break;
	case CStructureData::power_2:
		iToType = CStructureData::oil_well;
		break;
	case CStructureData::coal:
	case CStructureData::iron:
		iToType = CStructureData::smelter;
		break;
	case CStructureData::smelter:
		iToType = CStructureData::num_types;
		break;
	case CStructureData::oil_well:
		iToType = CStructureData::refinery;
		break;
	case CStructureData::refinery:
		iToType = CStructureData::oil_well;
		break;
	case CStructureData::lumber:
		iToType = CStructureData::rocket;
		break;
	case CStructureData::copper:
	case CStructureData::light_0:
	case CStructureData::light_1:
	case CStructureData::light_2:
	case CStructureData::heavy:
	case CStructureData::barracks_2:
	case CStructureData::shipyard_1:
	case CStructureData::shipyard_3:
		iToType = CStructureData::smelter;
	default:
		break;
	}

	int iDist, iBestDist=m_iMapSize;
	int iBestTypeDist=m_iMapSize;
	CHexCoord hex, hexBest, hexType;
	DWORD dwDumb;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIMap::PlanRoad() player %d for %ld from %d,%d ", 
		m_iPlayer, dwID, hexFrom.X(), hexFrom.Y() );

#endif

	EnterCriticalSection (&cs);

	POSITION pos = theBuildingMap.GetStartPosition();
	while (pos != NULL)
	{
		theBuildingMap.GetNextAssoc( pos, dwDumb, pBldg );
		ASSERT_VALID (pBldg);

		// consider only those building of this player
		if( pBldg->GetOwner()->GetPlyrNum() == m_iPlayer )
		{
			// skip the 'from' building
			if( pBldg->GetID() == dwID )
				continue;

			// no roads out to farms or lumber
			if( pBldg->GetData()->GetType() == CStructureData::farm ||
				pBldg->GetData()->GetType() == CStructureData::lumber )
				continue;

			if( iToType != CStructureData::city )
			{
				hex = pBldg->GetExitHex();
				iDist = pGameData->GetRangeDistance( hexFrom, hex );

				// run road to nearest factory
				if( iToType == CStructureData::num_types )
				{
					if( pBldg->GetData()->GetType() == CStructureData::coal ||
						pBldg->GetData()->GetType() == CStructureData::iron ||
						pBldg->GetData()->GetType() == CStructureData::light_0 ||
						pBldg->GetData()->GetType() == CStructureData::light_1 ||
						pBldg->GetData()->GetType() == CStructureData::light_2 ||
						pBldg->GetData()->GetType() == CStructureData::heavy ||
						pBldg->GetData()->GetType() == CStructureData::barracks_2 ||
						pBldg->GetData()->GetType() == CStructureData::shipyard_1 ||
						pBldg->GetData()->GetType() == CStructureData::shipyard_3 )
					{
						if( iDist && iDist < iBestTypeDist )
						{
							iBestTypeDist = iDist;
							hexType = hex;
						}
					}
				}
				else
				{
					if( pBldg->GetData()->GetType() == iToType )
					{
						if( iDist && iDist < iBestTypeDist )
						{
							iBestTypeDist = iDist;
							hexType = hex;
						}
					}
				}
			}

			// just get nearest building
			hex = pBldg->GetExitHex();
			iDist = pGameData->GetRangeDistance( hexFrom, hex );
			if( iDist && iDist < iBestDist )
			{
				iBestDist = iDist;
				hexBest = hex;
			}
		}
	}
	LeaveCriticalSection (&cs);

	// now find either the 1st road or the closest adjacent 
	// hex of the building's exit hex, to the hexFrom
	if( iBestTypeDist < m_iMapSize )
	{
		ConnectRoad( hexFrom, hexType );
	}
	else if( iBestDist < m_iMapSize )
	{
		ConnectRoad( hexFrom, hexBest );
	}
}

//
// construct a planned road from a hex adjacent to the hex 
// passed to the nearest road or planned road hex location
//
void CAIMap::PlanRoad( CAIHex *paiHex )
{
	CHexCoord hexFrom( paiHex->m_iX, paiHex->m_iY );
	CHexCoord hcFrom,hcTo,hex;

	// spiral search the hexes radiating out from paiHex
	int iStep = 1;
	while( iStep < pGameData->m_iHexPerBlk )
	{
		hcFrom.X( hex.Wrap(hex.X()-iStep) );
		hcFrom.Y( hex.Wrap(hex.Y()-iStep) );
		hcTo.X( hex.Wrap(hex.X()+iStep) );
		hcTo.Y( hex.Wrap(hex.Y()+iStep) );

		int iDeltax = abs( hex.Diff(hcTo.X()-hcFrom.X()) ) + 1;
		int iDeltay = abs( hex.Diff(hcTo.Y()-hcFrom.Y()) ) + 1;

		for( int iY=0; iY<iDeltay; ++iY )
		{
			hex.Y( hex.Wrap(hcFrom.Y()+iY) );

			for( int iX=0; iX<iDeltax; ++iX )
			{
				hex.X( hex.Wrap(hcFrom.X()+iX) );

#if THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hex.X() != hcFrom.X() &&
					hex.X() != hcTo.X() && 
					hex.Y() != hcFrom.Y() &&
					hex.Y() != hcTo.Y() )
					continue;

				int i = m_pMapUtil->GetMapOffset( hex.X(), hex.Y() );
				if( i < m_iMapSize )
				{
					// the first road/planned road encountered is the 
					// one that is closest to paiHex
					WORD wStatus = m_pwaMap[i];
					if( (wStatus & MSW_ROAD) ||
						(wStatus & MSW_PLANNED_ROAD) )
					{
						// connect the hex passed in with this hex
						// using MSW_PLANNED_ROAD to set a planned road
						ConnectRoad( hexFrom, hex );
						return;
					}
				}
			}
		}
		iStep++;
	}
}


//
// if bLayRoad == FALSE, then just run the route and
// test to be sure that road can be laid, returning TRUE
// if so, otherwise if bLayRoad == TRUE, then do the same
// thing but pave the planned road too.
//
// assume north->south and east->west connectors
//
BOOL CAIMap::ConnectRoad( CHexCoord& hexFrom, CHexCoord& hexTo )
{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIMap::ConnectRoad() player %d from %d,%d to %d,%d ", 
		m_iPlayer, hexFrom.X(), hexFrom.Y(), hexTo.X(), hexTo.Y() );

#endif

	//CHexCoord *CPathMap::GetRoadPath( 
	//	CHexCoord& hexFrom, CHexCoord& hexTo, 
	//	int& iPathLen, WORD *pMap, 
	//	BOOL bAllowWater=FALSE, BOOL bRiverCrossing=TRUE );

	int iPathLen = 0;
	CHexCoord *pRoadPath = 
		thePathMap.GetRoadPath( hexFrom, hexTo, iPathLen, m_pwaMap );
	if( pRoadPath != NULL )
	{
		CHexCoord hexGame;
		CHex *pGameHex;

		for( int i=0; i<iPathLen; ++i )
		{
			CHexCoord *pHex = &pRoadPath[i];

			WORD wStatus = GetLocation( pHex->X(), pHex->Y() );
			if( !(wStatus & MSW_PLANNED_ROAD) &&
				!(wStatus & MSW_ROAD) &&
				!(wStatus & MSW_AI_BUILDING) )
			{
				wStatus |= MSW_PLANNED_ROAD;
				SetLocation( pHex->X(), pHex->Y(), wStatus );
				m_iRoadCount++;

				// update city bounds
				m_pMapUtil->UpdateCityBounds( pHex->X(), pHex->Y() );

				hexGame.X( pHex->X() );
				hexGame.Y( pHex->Y() );
				pGameHex = theMap.GetHex( hexGame );
				if( pGameHex->GetType() == CHex::river )
				{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d planned river/road laid at %d,%d ", 
    	m_iPlayer, pHex->X(), pHex->Y() );

#endif
				}
				else
				{

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d planned road laid at %d,%d ", 
    	m_iPlayer, pHex->X(), pHex->Y() );

#endif
				}
			}
			else
			{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d could not lay road at %d,%d status=%d ", 
		m_iPlayer, pHex->X(), pHex->Y(), wStatus );

#endif
			}
		}

		delete [] pRoadPath;
		return( TRUE );
	}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nCAIMap::ConnectRoad() player %d NULL path returned \n",
    	m_iPlayer );

#endif

	return( FALSE );
}

//
// consider if there are any bridging canidates, and if so, then
// look for the best bridge site and return the start hex in hexSite
// and in GetParam(CAI_PREV_X/Y) and end in GetParam(CAI_DEST_X/Y)
// of the passed unit or leave hexSite unchanged to indicate no
// bridge should be built
//
void CAIMap::GetBridgingHexes( CHexCoord& hexSite, CAIUnit *pUnit )
{
	// first determine if the player can build bridges
	BOOL bCanBridge = FALSE;
	EnterCriticalSection( &cs );
	CPlayer *pPlayer = pGameData->GetPlayerData(m_iPlayer);
	if( pPlayer != NULL )
		pPlayer->CanBridge();
	LeaveCriticalSection( &cs );
	if( !bCanBridge )
		return;

	// if still here then the player can build bridges
	CHexCoord hexBefore = hexSite;
	m_pMapUtil->FindBridgeHex( hexSite, pUnit );

	// a site was selected, so mark the map
	if( hexBefore != hexSite )
	{
		CHexCoord hexStart(
		pUnit->GetParam(CAI_PREV_X),pUnit->GetParam(CAI_PREV_Y) );
		CHexCoord hexEnd(
		pUnit->GetParam(CAI_DEST_X),pUnit->GetParam(CAI_DEST_Y) );
		CHexCoord hexBridge;

		// bridge is vertical
		int iDelta = 0;
		if( hexStart.X() == hexEnd.X() )
		{
			iDelta = hexBridge.Diff(hexEnd.Y()-hexStart.Y());
		}
		// bridge is horizontal
		else if( hexStart.Y() == hexEnd.Y() )
		{
			iDelta = hexBridge.Diff(hexEnd.X()-hexStart.X());
		}
		else // an invalid bridge
			hexSite = hexBefore;

		iDelta = 0;
	}
}

//
// call utility and get planned road hex that is nearest
// power plant
//
void CAIMap::GetRoadHex( CHexCoord& hexSite )
{
	CHexCoord hexBefore = hexSite;
	m_pMapUtil->FindRoadHex( hexSite );

	// a site was selected, so mark the map
	if( hexBefore != hexSite )
	{
		WORD wStatus = GetLocation( hexSite.X(), hexSite.Y() );
		if( wStatus & MSW_PLANNED_ROAD )
			wStatus ^= MSW_PLANNED_ROAD;
		wStatus |= MSW_ROAD;
		SetLocation( hexSite.X(), hexSite.Y(), wStatus );
	}
}

// 
// find a hex, based on power plant settings
// suitable for locating a power plant, and
// return it in the CHexCoord reference
//
void CAIMap::PlacePowerPlant(CHexCoord& hex, int iBldg)
{
	int iWidthX, iWidthY;

	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData != NULL )
	{
		iWidthX = pBldgData->GetCX();
		iWidthY = pBldgData->GetCY();
	}

	CHexCoord hexBefore = hex;
	m_pMapUtil->m_bMinerals = FALSE;
	m_pMapUtil->FindSectionHex( iBldg, iWidthX, iWidthY, hex );
	m_pMapUtil->m_bMinerals = TRUE;

	// NO hex was selected
	if( hex == hexBefore )
		return;

	if( hex.X() >= (int)m_wCols || hex.Y() >= (int)m_wRows )
	{
		hex = hexBefore;
		m_pMapUtil->m_bMinerals = FALSE;
		m_pMapUtil->FindSectionHex( iBldg, iWidthX, iWidthY, hex );
		m_pMapUtil->m_bMinerals = TRUE;
	}
	// NO hex was selected
	if( hex == hexBefore )
		return;

	// BUGBUG this is a reporting function that
	// describes the hex group selected for construction
	// and should be removed for the release build
#ifdef _LOGOUT
	m_pMapUtil->ReportGroupHex( CStructureData::power, iWidthX,
		iWidthY, hex );
#endif

	// place fake building in map array
	PlaceFakeBldg( hex, iBldg );
}

void CAIMap::PlaceRocket( CHexCoord& hex )
{
	int iWidthX, iWidthY;
	// get size in hexes
	CStructureData const *pBldgData = 
		pGameData->GetStructureData( CStructureData::rocket );
	if( pBldgData != NULL )
	{
		iWidthX = pBldgData->GetCX();
		iWidthY = pBldgData->GetCY();
	}

	m_pMapUtil->m_bMinerals = FALSE;
	CAIHex aiHex( m_iBaseX, m_iBaseY );
	m_pMapUtil->FindCentralHex( &aiHex, iWidthX, iWidthY, hex );
	m_pMapUtil->m_bMinerals = TRUE;

	if( !hex.X() && !hex.Y() )
	{
		hex.X( m_iBaseX );
		hex.Y( m_iBaseY );
		m_pMapUtil->m_RocketHex = hex;
		m_pMapUtil->m_bMinerals = FALSE;
		m_pMapUtil->FindSectionHex( CStructureData::rocket, 
			iWidthX, iWidthY, hex );
		
		if( hex.X() >= (int)m_wCols || hex.Y() >= (int)m_wRows )
		{
			hex.X( m_iBaseX );
			hex.Y( m_iBaseY );
			m_pMapUtil->FindSectionHex( CStructureData::rocket, 
				iWidthX, iWidthY, hex );
		}
		m_pMapUtil->m_bMinerals = TRUE;
	}

	if( hex.X() >= (int)m_wCols || hex.Y() >= (int)m_wRows )
	{
		hex.X( m_iBaseX );
		hex.Y( m_iBaseY );
		m_pMapUtil->m_RocketHex = hex;
		m_pMapUtil->m_bMinerals = FALSE;
		m_pMapUtil->FindSectionHex( CStructureData::rocket, 
			iWidthX, iWidthY, hex );
		m_pMapUtil->m_bMinerals = TRUE;
	}

	m_pMapUtil->m_RocketHex = hex;

	// BUGBUG this is a reporting function that
	// describes the hex group selected for construction
	// and should be removed for the release build
#ifdef _LOGOUT
	m_pMapUtil->ReportGroupHex( CStructureData::rocket, iWidthX,
		iWidthY, hex );
#endif

	// place fake building in map array
	PlaceFakeBldg( hex, CStructureData::rocket );
}

//
// find the hex that is on known hexes, that can
// best support the production type building that
// is being passed, and return hex
//
void CAIMap::PlaceProducer( int iBldg, CHexCoord& hex)
{
	int iWidthX, iWidthY;
	// get size in hexes
	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData != NULL )
	{
		iWidthX = pBldgData->GetCX();
		iWidthY = pBldgData->GetCY();
	}
	else 
		return;

	// unless the building needs minerals, minerals are screened off
	if( pBldgData->GetBldgType() != CStructureData::UTmine )
		m_pMapUtil->m_bMinerals = FALSE;
	else
		m_pMapUtil->m_bMinerals = TRUE;

	// determine sections based on planned and
	// actual roads, avoiding OPFOR buildings too
	// pick a section based on this building type
	CHexCoord hexBefore = hex;
	m_pMapUtil->FindSectionHex( iBldg, //&aiHex, 
		iWidthX, iWidthY, hex );

	if( hex.X() < 0 || hex.X() >= (int)m_wCols || 
		hex.Y() < 0 || hex.Y() >= (int)m_wRows )
	{
		hex = hexBefore;
		m_pMapUtil->FindSectionHex( iBldg, iWidthX, iWidthY, hex );

		if( hex.X() < 0 || hex.X() >= (int)m_wCols || 
			hex.Y() < 0 || hex.Y() >= (int)m_wRows )
			hex = hexBefore;
	}

	// restore default
	m_pMapUtil->m_bMinerals = TRUE;

	// no site was found
	if( hexBefore == hex )
		return;

	// BUGBUG this is a reporting function that
	// describes the hex group selected for construction
	// and should be removed for the release build
#ifdef _LOGOUT
	m_pMapUtil->ReportGroupHex( iBldg, iWidthX,
		iWidthY, hex );
#endif

	// place fake building in map array
	PlaceFakeBldg( hex, iBldg );
}

//
// stage scouts around the base hex
//
void CAIMap::PlaceScout(CHexCoord& hex)
{
	//void CAIMap::GetStagingHex( CHexCoord& hexNearBy, 
	//int iWidth, int iHeight, int iVehType, CHexCoord& hexDest )

	CHexCoord hexNearBy( m_iBaseX, m_iBaseY );
	GetStagingHex( hexNearBy, 1, 1, CTransportData::light_scout, hex );

	if( hex.X() >= (int)m_wCols || hex.Y() > (int)m_wRows )
	{
		BOOL bBefore = m_pMapUtil->m_bMinerals;
		m_pMapUtil->m_bMinerals = FALSE;
		m_pMapUtil->m_bMinerals = TRUE;
		m_pMapUtil->m_bMinerals = bBefore;
	}
}

void CAIMap::PlaceVehicleNextTo( int /*iBldg*/, CHexCoord& hex)
{
	CAIHex aiHex( m_iBaseX, m_iBaseY );
	m_pMapUtil->m_bMinerals = FALSE;

	// find hex adjacent to this building, and since it is
	// a vehicle, then default width/height to 1
	//if( !m_pMapUtil->FindAdjacentHex( iBldg, &aiHex, 1, 1, hex ) )

		// else find hex nearest to base hex
		// that is not planned road, road
		// or other building
		m_pMapUtil->FindCentralHex( &aiHex, 1, 1, hex );

	m_pMapUtil->m_bMinerals = TRUE;
}

//
// the game needs a location for an uncontrolled building
// so based on the type of building, find a suitable location
// and return the hex
//
void CAIMap::PlaceBuilding( CAIMsg *pMsg, CAIUnitList *plUnits )
{
	int iWidthX, iWidthY;
	// get size in hexes
	CStructureData const *pBldgData = 
		pGameData->GetStructureData( pMsg->m_idata1 );
	if( pBldgData != NULL )
	{
		iWidthX = pBldgData->GetCX();
		iWidthY = pBldgData->GetCY();
	}
	else 
		return;

	CHexCoord hexPlace(0,0);

	m_pMapUtil->m_bMinerals = FALSE;
	m_pMapUtil->FindSectionHex( pMsg->m_idata1, 
		iWidthX, iWidthY, hexPlace );
	m_pMapUtil->m_bMinerals = TRUE;

	if( !hexPlace.X() && !hexPlace.Y() )
		return;

	// send a message back to the game with hex selected
	// first create a CAIUnit for it
	CAIUnit *pUnit = NULL;
	try
	{
		// CAIUnit( DWORD dwID, int iOwner );
		pUnit = new CAIUnit( theGame.GetID(), 
			m_iPlayer, CUnit::building, pMsg->m_idata1 );
		ASSERT_VALID( pUnit );
		plUnits->AddTail( (CObject *)pUnit );
	}
	catch( CException e )
	{
		// BUGBUG need to report this error occurred
		throw(ERR_CAI_BAD_NEW);
	}

	// place fake building in map array
	PlaceFakeBldg( hexPlace, pMsg->m_idata1 );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"PlaceBuilding() player %d building %ld a %d at %d,%d \n", 
		pUnit->GetOwner(), pUnit->GetID(), pUnit->GetTypeUnit(), 
		hexPlace.X(), hexPlace.Y() );
#endif

	//CMsgPlaceBldg (CHexCoord const & hex, int iDir, int iBldg);
	CMsgPlaceBldg msg( hexPlace, 0, pUnit->GetTypeUnit() );
	msg.m_dwIDBldg = pUnit->GetID();
	msg.m_iPlyrNum = pUnit->GetOwner();

	theGame.PostToServer( (CNetCmd *)&msg, sizeof(CMsgPlaceBldg) );

#ifdef _LOGOUT
	m_pMapUtil->ReportGroupHex( pUnit->GetTypeUnit(),
		iWidthX,iWidthY,hexPlace );
#endif

}

void CAIMap::GetStartHex( CHexCoord& hexStart, CHexCoord& hexEnd, 
	CHexCoord& hexPlace, int iVehType )
{
	int iOffset = pGameData->m_iHexPerBlk / 4;
	CHexCoord hcStart;
	hcStart.X( hcStart.Wrap( hexStart.X() + iOffset ));
	hcStart.Y( hcStart.Wrap( hexStart.Y() + iOffset ));
	CHexCoord hcEnd;
	hcEnd.X( hcEnd.Wrap( hexEnd.X() - iOffset ));
	hcEnd.Y( hcEnd.Wrap( hexEnd.Y() - iOffset ));

	m_pMapUtil->FindStagingHex( hcStart.X(), hcStart.Y(), 
		hcEnd.X(), hcEnd.Y(), hexPlace, iVehType );
}

// now ask map to do the work and find a place to stage
//
// find a staging hex and return in hexDest
//
void CAIMap::GetStagingHex( CHexCoord& hexNearBy, 
	int iWidth, int iHeight, int iVehType, CHexCoord& hexDest, 
	BOOL bExclude /*=TRUE*/ )
{
	m_pMapUtil->m_bMinerals = FALSE;
	m_pMapUtil->FindStagingHex( hexNearBy, 
		iWidth, iHeight, iVehType, hexDest, bExclude );
	m_pMapUtil->m_bMinerals = TRUE;
}

//
// consider the passed message, if it reports a building
// constructed, check for a road or planned road that is 
// adjacent to the hex-of-the-building, and if there is
// none, then determine the nearest existing road or 
// planned road and plan a road to it
//
// any other message causes a return
//
// road or planned road adjacent to hex-of-the-building
// causes a return
//
void CAIMap::PlanRoads( CAIMsg *pMsg )
{
	// only lay roads on bldg_stat when building is completed
	if( pMsg->m_iMsg != CNetCmd::bldg_stat ||
		pMsg->m_idata3 != m_iPlayer )
		return;

	if( pMsg->m_uFlags != CMsgBldgStat::built ||
		pMsg->m_idata2 != 100 )
		return;

	// get latest area updated, based on hex of message
	UpdateMap( pMsg );

	// the id of the building
	DWORD dwID = pMsg->m_dwID;
	// do not lay roads out to farms or lumber
	if( pMsg->m_idata1 == CStructureData::farm ||
		pMsg->m_idata1 == CStructureData::lumber )
		return;

	// plan road from this building's exit hex to nearest other exit hex
	PlanRoad( dwID );
}

//
// find an unoccupied hex for the pUnitToStage which is
// nearby (no closer than width or height from) pUnitNearby
//
void CAIMap::GetStagingHex( CAIUnit *pUnitToStage, 
	CAIUnit *pUnitNearby, CHexCoord& hexDest )
{
	int iWidth = 1;
	int iHeight = 1;
	CHexCoord hexNearBy;

	// its been a while since yielding
#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
	//if( myYieldThread() == TM_QUIT )
	//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	if( pUnitNearby == NULL )
	{
		EnterCriticalSection (&cs);
		CVehicle *pVehicle = 
			theVehicleMap.GetVehicle( pUnitToStage->GetID() );
		if( pVehicle == NULL )
		{
			LeaveCriticalSection (&cs);
			return;
		}
		hexNearBy = pVehicle->GetHexHead();
		LeaveCriticalSection (&cs);

		// all vehicles are 1x1 in size
	}
	else
	{
		if( pUnitNearby->GetType() == CUnit::vehicle )
		{
			EnterCriticalSection (&cs);
			CVehicle *pVehicle = 
				theVehicleMap.GetVehicle( pUnitNearby->GetID() );
			if( pVehicle == NULL )
			{
				LeaveCriticalSection (&cs);
				return;
			}
			hexNearBy = pVehicle->GetHexHead();
			LeaveCriticalSection (&cs);

			// all vehicles are 1x1 in size
		}
		else if( pUnitNearby->GetType() == CUnit::building )
		{
			EnterCriticalSection (&cs);
			CBuilding *pBldg = theBuildingMap.GetBldg( pUnitNearby->GetID() );
			if( pBldg == NULL )
			{
				LeaveCriticalSection (&cs);
				return;
			}
			hexNearBy = pBldg->GetExitHex();
			// get building size
			iWidth = pBldg->GetData()->GetCX();
			iHeight = pBldg->GetData()->GetCY();
			LeaveCriticalSection (&cs);
		}
	}
	// now ask map to do the work and find a place to stage
	GetStagingHex( hexNearBy, iWidth, iHeight, 
		pUnitToStage->GetTypeUnit(), hexDest );
}

//
// determine a location for the crane to be adjacent to the site
//
void CAIMap::GetCraneHex( CHexCoord& hexSite, CHexCoord& hexCrane )
{
	// check 4 adjacent hexes to the hexSite for the crane
	//
	// valid locations for a crane building a 2x2 are: 2,3,5,6,7,8,A,B
	//
	//   1234
	//   5XX6
	//   7XX8
	//   9ABC
	//
	for( int i=0; i<MAX_ADJACENT; ++i )
	{
		CHexCoord hexAdj = hexSite;
		//
		// using MAX_ADJACENT pattern
		//
		//   701
		//   6X2
		//   543
		//
		switch( i )
		{
			case 0:
				hexAdj.Ydec();
				break;
			case 1:
				hexAdj.Ydec();
				hexAdj.Xinc();
				break;
			//case 7:
			//	hexAdj.Ydec();
			//	hexAdj.Xdec();
			//	break;
			case 6:
				hexAdj.Xdec();
				break;
			case 5:
				hexAdj.Yinc();
				hexAdj.Xdec();
				break;
			default:
				continue;
		}
		int j = m_pMapUtil->GetMapOffset( hexAdj.X(), hexAdj.Y() );
		if( j >= m_iMapSize )
			continue;

		CHex *pGameHex = theMap.GetHex( hexAdj );
		if( pGameHex == NULL )
			continue;

		BYTE bUnits = pGameHex->GetUnits();
		// skip buildings
		if( (bUnits & CHex::bldg) )
			continue;
		// and vehicles
		if( (bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr)) )
			continue;

		if( !m_pMapUtil->m_tdWheel->CanTravelHex(pGameHex) )
			continue;

		hexCrane = hexAdj;
		return;
	}
}

//
// determine the best location to use to
// construct the building ided by iBldg
//
void CAIMap::GetBuildHex( int iBldg, CHexCoord& hexSite )
{
#if THREADS_ENABLED
	// BUGBUG this function must yield
	myYieldThread();
	//if( myYieldThread() == TM_QUIT )
	//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData == NULL )
		return;

	// this is the building group type
	switch( pBldgData->GetBldgType() )
	{
		case CStructureData::power:	// nearest other CStructureData::power or
								// most centralized known hex if none
#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif
			PlacePowerPlant( hexSite, iBldg );

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"PlacePowerPlant() player %d for a %d took %ld ticks \n", 
		m_iPlayer, iBldg, (dwEnd - dwStart) );
#endif
			break;
		default:

#ifdef _LOGOUT
	dwStart = timeGetTime(); 
#endif
			PlaceProducer( iBldg, hexSite );

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"PlaceProducer() player %d for a %d took %ld ticks \n", 
		m_iPlayer, iBldg, (dwEnd - dwStart) );

#endif
			break;
	}
}

void CAIMap::Save( CFile *pFile )
{
	//int iX, iY;
	//iX = m_RocketHex.X();
	//iY = m_RocketHex.Y();
	try
	{
		pFile->Write( (const void*)&m_iPlayer, sizeof(int) );
		pFile->Write( (const void*)&m_wRows, sizeof(WORD) );
		pFile->Write( (const void*)&m_wCols, sizeof(WORD) );
		pFile->Write( (const void*)&m_wBaseRow, sizeof(WORD) );
		pFile->Write( (const void*)&m_wBaseCol, sizeof(WORD) );
		//pFile->Write( (const void*)&iX, sizeof(int) );
		//pFile->Write( (const void*)&iY, sizeof(int) );
		pFile->Write( (const void*)&m_iBaseX, sizeof(int) );
		pFile->Write( (const void*)&m_iBaseY, sizeof(int) );
		pFile->Write( (const void*)&m_iMapSize, sizeof(int) );
		pFile->Write( (const void*)&m_iRoadCount, sizeof(int) );
		pFile->Write( (const void*)&m_iOcean, sizeof(int) );
		pFile->Write( (const void*)&m_iLake, sizeof(int) );
		pFile->Write( (const void*)&m_iLand, sizeof(int) );
		
		pFile->Write( (const void*)m_pwaMap, (sizeof(WORD) * m_iMapSize) );
	}
    catch( CFileException theException )
    {
		// BUGBUG how should write errors be reported?
    	throw(ERR_CAI_BAD_FILE);
    }
	// save the utility
	m_pMapUtil->Save( pFile );
}

void CAIMap::Load( CFile *pFile, CAIUnitList *plUnits )
{
	//int iX, iY;

		pFile->Read( (void*)&m_iPlayer, sizeof(int) );
		pFile->Read( (void*)&m_wRows, sizeof(WORD) );
		pFile->Read( (void*)&m_wCols, sizeof(WORD) );
		pFile->Read( (void*)&m_wBaseRow, sizeof(WORD) );
		pFile->Read( (void*)&m_wBaseCol, sizeof(WORD) );
		//pFile->Read( (void*)&iX, sizeof(int) );
		//pFile->Read( (void*)&iY, sizeof(int) );
		pFile->Read( (void*)&m_iBaseX, sizeof(int) );
		pFile->Read( (void*)&m_iBaseY, sizeof(int) );
		pFile->Read( (void*)&m_iMapSize, sizeof(int) );
		pFile->Read( (void*)&m_iRoadCount, sizeof(int) );
		pFile->Read( (void*)&m_iOcean, sizeof(int) );
		pFile->Read( (void*)&m_iLake, sizeof(int) );
		pFile->Read( (void*)&m_iLand, sizeof(int) );
		

	//m_RocketHex.X( iX );
	//m_RocketHex.Y( iY );

	// map size might have changed
	if( m_pwaMap != NULL )
		delete [] m_pwaMap;

	try
	{
		m_pwaMap = new WORD[m_iMapSize];
		memset( m_pwaMap, 0, (size_t)(m_iMapSize * sizeof( WORD )) );
	}
	catch( CException theException )
	{
		if( m_pwaMap != NULL )
		{
			delete [] m_pwaMap;
			m_pwaMap = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}

	pFile->Read( (void*)m_pwaMap, (sizeof(WORD) * m_iMapSize) );

	m_pMapUtil->Load( pFile, m_pwaMap, plUnits );
}

// end of CAIMap.cpp
