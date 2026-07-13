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
#include "caimap.hpp"
#include "caidata.hpp"
#include "cpathmap.h"

#include "logging.h"	// dave's logging system
#include "ai.h"			// AiHexCacheActive()

#include <vector>
#include <cstring>

extern CAIData *pGameData;		// pointer to API object for game data
extern CPathMap thePathMap;		// the map pathfinding object (no yield)
extern CException *pException;	// standard exception for yielding
extern CRITICAL_SECTION cs;	// used by threads

#define new DEBUG_NEW

//---------------------------------------------------------------------------
// Shared per-AI BASE map snapshot (new-game create speed-up).
//
// Every AI's CAIMap::UpdateMap(NULL) scans the whole world identically; the
// only per-player difference is whether each BUILDING hex is mine vs theirs.
// So the first AI of the game does the full scan and we save its result as a
// base; every later AI just memcpy's the base and re-derives the handful of
// building hexes for its own ownership. Built/used only while the setup hex
// cache is live (AiHexCacheActive); freed in AiHexCacheFree -> AiMapBaseFree.
// Setup runs on the main thread, so no locking is needed here.
//---------------------------------------------------------------------------
namespace
{
	WORD*            s_pAiBaseMap  = nullptr;
	int              s_aiBaseSize  = 0;
	int              s_aiBaseOcean = 0, s_aiBaseLand = 0, s_aiBaseLake = 0;
	std::vector<int> s_aiBldgOffsets;   // map offsets that carry a building bit
	bool             s_aiBaseValid = false;
}

void AiMapBaseFree( )
{
	delete[] s_pAiBaseMap;
	s_pAiBaseMap  = nullptr;
	s_aiBaseSize  = 0;
	s_aiBaseValid = false;
	s_aiBldgOffsets.clear();
	s_aiBldgOffsets.shrink_to_fit();
}

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
	m_iBridgeSpanFails = 0;
	m_bPendingBridge = FALSE;
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

    if ( m_pwaMap == NULL )
        return;

    if ( pGameData == NULL )
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

	// FAST PATH (AI create): the player-independent base map was already built
	// by the first AI of this game — copy it, then re-derive ONLY the building
	// hexes for THIS player's ownership (the sole per-player difference).
	if( AiHexCacheActive() && s_aiBaseValid && s_aiBaseSize == m_iMapSize )
	{
		memcpy( m_pwaMap, s_pAiBaseMap, (size_t)m_iMapSize * sizeof( WORD ) );
		m_iOcean = s_aiBaseOcean;
		m_iLand  = s_aiBaseLand;
		m_iLake  = s_aiBaseLake;

		for( size_t k = 0; k < s_aiBldgOffsets.size(); ++k )
		{
			int i = s_aiBldgOffsets[k];
			m_pMapUtil->OffsetToXY( i, &aiHex.m_iX, &aiHex.m_iY );
			pGameData->GetCHexData( &aiHex );
			// oldStatus 0 mirrors the original (each AI's m_pwaMap starts zeroed)
			m_pwaMap[i] = m_pMapUtil->ConvertStatus( &aiHex, 0 );
		}
		return;
	}

	// SLOW PATH: full scan (first AI of the game, or outside setup). When in
	// setup, capture the result as the shared base so the rest take the fast path.
	bool bBuildBase = AiHexCacheActive() && !s_aiBaseValid;
	if( bBuildBase )
		s_aiBldgOffsets.clear();

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

		if( bBuildBase && ( wStatus & ( MSW_AI_BUILDING | MSW_OPFOR_BUILDING ) ) )
			s_aiBldgOffsets.push_back( i );
	}

	if( bBuildBase )
	{
		delete[] s_pAiBaseMap;
		s_pAiBaseMap = new ( std::nothrow ) WORD[m_iMapSize];
		if( s_pAiBaseMap != NULL )
		{
			memcpy( s_pAiBaseMap, m_pwaMap, (size_t)m_iMapSize * sizeof( WORD ) );
			s_aiBaseSize  = m_iMapSize;
			s_aiBaseOcean = m_iOcean;
			s_aiBaseLand  = m_iLand;
			s_aiBaseLake  = m_iLake;
			s_aiBaseValid = true;
		}
		else
		{
			s_aiBldgOffsets.clear();   // OOM → just keep using the slow path
		}
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
	catch( CException* e )
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
			m_aPlannedRoad.push_back( j );	// index the new planned hex

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
	// Use this AI's own path instance (per-AI; no cross-AI lock contention).
	CPathMap& pathMap = ( m_pMapUtil && m_pMapUtil->m_pPathMap ) ? *m_pMapUtil->m_pPathMap : thePathMap;
	CHexCoord *pRoadPath =
		pathMap.GetRoadPath( hexFrom, hexTo, iPathLen, m_pwaMap );
	if( pRoadPath != NULL )
	{
		CHexCoord hexGame;
		CHex *pGameHex;

		// owner's bridge reach (tech-derived; 0 = no bridge tech) for the
		// span-aware crossing cap below
		int iMaxSpan = 0;
		{
			EnterCriticalSection( &cs );
			CPlayer *pPlyr = pGameData->GetPlayerData( m_iPlayer );
			if( pPlyr != NULL && pPlyr->CanBridge() )
				iMaxSpan = pPlyr->GetMaxSpan();
			LeaveCriticalSection( &cs );
		}

		int iSkipUntil = 0;   // > i while inside an unbridgeable river run
		for( int i=0; i<iPathLen; ++i )
		{
			CHexCoord *pHex = &pRoadPath[i];
			if( i < iSkipUntil )
				continue;

			// travel-passable is not build-able: the planner's A* routes over
			// coastline (vehicles drive it) but no road can be BUILT there --
			// each flagged coast hex wedged a crane at the run-end latch
			// (~2/min steady). Rivers STAY flagged: bridge discovery scans the
			// plan for spans (FindBridgeOnPlan/GetBridgingHexes).
			{
				CHex* pTerrHex = theMap.GetHex( *pHex );
				if( pTerrHex != NULL )
				{
					int iTT = pTerrHex->GetType();
					if( iTT == CHex::coastline || iTT == CHex::ocean || iTT == CHex::lake )
						continue;

					// span-aware crossing cap (operator): with bridge tech, drop
					// crossings longer than the reach. PRE-tech rivers STAY
					// planned (vanilla) - iMaxSpan=0 stripped every river from
					// every early plan, so bridge discovery (it scans the plan)
					// never saw a span: 0 bridges in the 14h fog-off run. The
					// pick-time gate keeps cranes off river hexes instead.
					if( iTT == CHex::river && iMaxSpan > 0 )
					{
						int j = i;
						while( j < iPathLen )
						{
							CHex* pRunHex = theMap.GetHex( pRoadPath[j] );
							if( pRunHex == NULL || pRunHex->GetType() != CHex::river )
								break;
							++j;
						}
						if( ( j - i ) > iMaxSpan )
						{
							iSkipUntil = j;   // drop the whole crossing from the plan
							continue;
						}
					}
				}
			}

			WORD wStatus = GetLocation( pHex->X(), pHex->Y() );
			if( !(wStatus & MSW_PLANNED_ROAD) &&
				!(wStatus & MSW_ROAD) &&
				!(wStatus & MSW_AI_BUILDING) )
			{
				wStatus |= MSW_PLANNED_ROAD;
				SetLocation( pHex->X(), pHex->Y(), wStatus );
				m_iRoadCount++;
				// index the new planned hex (flag was just newly set here)
				m_aPlannedRoad.push_back( m_pMapUtil->GetMapOffset( pHex->X(), pHex->Y() ) );

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
// plan a war road from the colony (rocket exit hex) toward an assault
// staging area.  reuses ConnectRoad's A* (river crossings enabled), so
// planned river hexes get bridged and m_iRoadCount is updated.
//
void CAIMap::PlanWarRoad( CHexCoord& hexTo )
{
	// no rocket placed yet
	if( !m_pMapUtil->m_RocketHex.X() && !m_pMapUtil->m_RocketHex.Y() )
		return;

	// colony origin = rocket exit hex (same derivation as RocketRoad)
	CHexCoord hexFrom = m_pMapUtil->m_RocketHex;
	hexFrom.Xinc();
	hexFrom.Yinc();

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC,
		"\nCAIMap::PlanWarRoad() player %d from %d,%d to %d,%d ",
		m_iPlayer, hexFrom.X(), hexFrom.Y(), hexTo.X(), hexTo.Y() );
#endif

	// A* routes through existing roads; the last reachable stretch is the road
	ConnectRoad( hexFrom, hexTo );

#if EN_AI_PROBES_WAR && defined(_WIN32)
	{
		// TEMP: war-road planning probe (operator needs to see war roads planned)
		char szW[96];
		sprintf( szW, "[WARROAD] plyr %d to %d,%d\n", m_iPlayer, hexTo.X(), hexTo.Y() );
		OutputDebugStringA( szW );
	}
#endif
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
		bCanBridge = pPlayer->CanBridge();   // was discarded -> bCanBridge stayed FALSE -> always returned early (bug #29)
	LeaveCriticalSection( &cs );
	if( !bCanBridge )
		return;

	// if still here then the player can build bridges
	CHexCoord hexBefore = hexSite;
	// scan the whole planned road (not just the 2-block rocket ring) for the
	// nearest river crossing to the crane; keeps IsBridgeSpan validation.
	FindBridgeOnPlan( hexSite, pUnit );

	// plan-index miss: fall back to the vanilla rocket-ring search (superset)
	if( hexBefore == hexSite )
		m_pMapUtil->FindBridgeHex( hexSite, pUnit );

	// a site was selected, so mark the map
	if( hexBefore != hexSite )
	{
		// crossing already claimed by a dispatched crane (or recently denied):
		// pretend no site so a second crane is never sent (soak17: 3 on one span)
		std::map<int, DWORD>::iterator itC =
			m_mBridgeDeny.find( m_pMapUtil->GetMapOffset( hexSite.X(), hexSite.Y() ) );
		if( itC != m_mBridgeDeny.end() )
		{
			if( timeGetTime() < itC->second )
			{
				hexSite = hexBefore;
				return;
			}
			m_mBridgeDeny.erase( itC );
		}
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
// nearest planned-road RIVER hex to the crane (hexSite in on entry) that forms a
// valid bridge span. iterates m_aPlannedRoad (whole road, not the old 2-block
// rocket ring) so crossings anywhere along a war road are found. same contract as
// CAIMapUtil::FindBridgeHex: on success sets hexSite to the span START hex and
// stores start in CAI_PREV_X/Y, end in CAI_DEST_X/Y; hexSite unchanged if none.
//
void CAIMap::FindBridgeOnPlan( CHexCoord& hexSite, CAIUnit *pUnit )
{
	CHexCoord hexCrane = hexSite;
	int iBestDist = m_iMapSize + 1;
	CHexCoord hexBestSite;
	int iPrevX = 0, iPrevY = 0, iDestX = 0, iDestY = 0;
	BOOL bFound = FALSE;

	size_t k = 0;
	while( k < m_aPlannedRoad.size() )
	{
		int off = m_aPlannedRoad[k];

		// lazy removal: entry no longer a planned road -> swap-and-pop
		if( off < 0 || off >= m_iMapSize || !( m_pwaMap[off] & MSW_PLANNED_ROAD ) )
		{
			m_aPlannedRoad[k] = m_aPlannedRoad.back();
			m_aPlannedRoad.pop_back();
			continue;
		}

#if THREADS_ENABLED
		myYieldThread();
#endif

		int iX, iY;
		m_pMapUtil->OffsetToXY( off, &iX, &iY );
		CHexCoord hexCand( iX, iY );

		// any water hex (river/lake/ocean) is a bridge candidate - the server
		// span check is water-agnostic (BuildBridge counts IsWater hexes)
		CHex *pGameHex = theMap.GetHex( hexCand );
		if( pGameHex == NULL || !pGameHex->IsWater() )
		{
			++k;
			continue;
		}

		// only validate candidates that could beat the current best
		int iDist = pGameData->GetRangeDistance( hexCrane, hexCand );
		if( iDist >= iBestDist )
		{
			++k;
			continue;
		}

		// IsBridgeSpan rewrites hexTest -> span start land hex and sets CAI_PREV/DEST
		CHexCoord hexTest = hexCand;
		if( m_pMapUtil->IsBridgeSpan( hexTest, pUnit ) )
		{
			// crane must REACH the span start; bridges are symmetric, so if the
			// near bank is inside an isolated pocket, build from the far bank
			BOOL bReach = m_pMapUtil->GetPathRating( hexCrane, hexTest );
			if( !bReach )
			{
				CHexCoord hexFar( pUnit->GetParam( CAI_DEST_X ), pUnit->GetParam( CAI_DEST_Y ) );
				if( m_pMapUtil->GetPathRating( hexCrane, hexFar ) )
				{
					int iSwapX = pUnit->GetParam( CAI_PREV_X ), iSwapY = pUnit->GetParam( CAI_PREV_Y );
					pUnit->SetParam( CAI_PREV_X, hexFar.X() );
					pUnit->SetParam( CAI_PREV_Y, hexFar.Y() );
					pUnit->SetParam( CAI_DEST_X, iSwapX );
					pUnit->SetParam( CAI_DEST_Y, iSwapY );
					hexTest = hexFar;
					bReach  = TRUE;
				}
			}
			if( !bReach )
			{
				++k;
				continue;
			}
			iBestDist   = iDist;
			hexBestSite = hexTest;
			iPrevX = pUnit->GetParam( CAI_PREV_X );
			iPrevY = pUnit->GetParam( CAI_PREV_Y );
			iDestX = pUnit->GetParam( CAI_DEST_X );
			iDestY = pUnit->GetParam( CAI_DEST_Y );
			bFound = TRUE;
		}
		++k;
	}

	if( bFound )
	{
		hexSite = hexBestSite;
		pUnit->SetParam( CAI_PREV_X, iPrevX );
		pUnit->SetParam( CAI_PREV_Y, iPrevY );
		pUnit->SetParam( CAI_DEST_X, iDestX );
		pUnit->SetParam( CAI_DEST_Y, iDestY );
	}
}

//
// clamped-crane bridge assist: a crane parked at hexAt cannot reach its task
// site hexSite. If the straight walk toward the site hits a river the owner
// can span, flag bank + crossing + landing as planned road -- the existing
// road/bridge pipeline (GetBridgingHexes -> IsBridgeSpan -> BuildBridgeAt)
// then discovers and builds it. TRUE = a crossing was planned.
//
BOOL CAIMap::PlanBridgeToward( CHexCoord const& hexAt, CHexCoord const& hexSite )
{
	// bridge tech + real span reach
	BOOL bCanBridge = FALSE;
	int iMaxSpan = 0;
	EnterCriticalSection( &cs );
	{
		CPlayer *pPlyr = pGameData->GetPlayerData( m_iPlayer );
		if( pPlyr != NULL )
		{
			bCanBridge = pPlyr->CanBridge();
			iMaxSpan   = pPlyr->GetMaxSpan();
		}
	}
	LeaveCriticalSection( &cs );
	if( !bCanBridge || iMaxSpan <= 0 )
	{
		// no tech at all and something needs crossing: strongest research signal
		m_iBridgeSpanFails++;
#if EN_AI_PROBES_ECON && defined(_WIN32)
		{
			char szB[96];
			sprintf( szB, "[BRIDGEMISS] plyr %d notech (canbridge %d span %d)\n", m_iPlayer, (int)bCanBridge, iMaxSpan );
			OutputDebugStringA( szB );
		}
#endif
		return FALSE;
	}

	// step toward the site until a water hex or the site. Three walk shapes:
	// staircase (dominant axis of remaining delta - the original), then the two
	// L-walks (x-leg-first, y-leg-first) - a river bending around the base is
	// invisible to the staircase (plyr 11 'noriver' at 391,462->413,390) but one
	// of the L-legs crosses it
	CHexCoord hexWalk, hexBank( 0, 0 );
	int iDir = -1;
	for( int iMode = 0; iMode < 3 && iDir < 0; iMode++ )
	{
		hexWalk = hexAt;
		for( int i = 0; i < 128 && iDir < 0; i++ )
		{
			int dx = (int)hexSite.X() - (int)hexWalk.X();
			int dy = (int)hexSite.Y() - (int)hexWalk.Y();
			if( !dx && !dy )
				break;
			CHexCoord hexNext = hexWalk;
			int iStepDir;
			BOOL bStepX;
			switch( iMode )
			{
			default: bStepX = ( ( dx >= 0 ? dx : -dx ) >= ( dy >= 0 ? dy : -dy ) ); break;  // staircase
			case 1:  bStepX = ( dx != 0 ); break;   // x leg first
			case 2:  bStepX = ( dy == 0 ); break;   // y leg first
			}
			if( bStepX )
			{
				if( dx > 0 ) { hexNext.Xinc(); iStepDir = 2; }
				else         { hexNext.Xdec(); iStepDir = 6; }
			}
			else
			{
				if( dy > 0 ) { hexNext.Yinc(); iStepDir = 4; }
				else         { hexNext.Ydec(); iStepDir = 0; }
			}
			CHex *pNextHex = theMap.GetHex( hexNext );
			if( pNextHex != NULL && pNextHex->IsWater() &&
				!( pNextHex->GetUnits() & CHex::bridge ) )	// bridged water is crossable: walk on to the NEXT gap
			{
				hexBank = hexWalk;
				iDir    = iStepDir;
			}
			else
				hexWalk = hexNext;
		}
	}
	if( iDir < 0 )
	{
#if EN_AI_PROBES_ECON && defined(_WIN32)
		{
			char szB[96];
			sprintf( szB, "[BRIDGEMISS] plyr %d noriver on walk %d,%d -> %d,%d\n", m_iPlayer,
				hexAt.X(), hexAt.Y(), hexSite.X(), hexSite.Y() );
			OutputDebugStringA( szB );
		}
#endif
		return FALSE;
	}

	// try the direct bank point, then slide along the bank +/-6 hexes for a
	// narrower crossing (the round-8 success at 436,541 was such a spot; the
	// direct point alone misses them)
	BOOL bShiftX = ( iDir == 0 || iDir == 4 );	// span runs along Y -> slide along X
	static const int aiOff[21] = { 0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5, 6, -6, 7, -7, 8, -8, 9, -9, 10, -10 };
	CHexCoord hexEnd( 0, 0 );
	BOOL bFoundSpan = FALSE;
	for( int k = 0; k < 21 && !bFoundSpan; k++ )
	{
		CHexCoord hexTry = hexBank;
		if( bShiftX )
			hexTry.X( CHexCoord::Wrap( hexBank.X() + aiOff[k] ) );
		else
			hexTry.Y( CHexCoord::Wrap( hexBank.Y() + aiOff[k] ) );

		// bank must be crane-traversable land with no building on it
		CHex *pBankHex = theMap.GetHex( hexTry );
		if( pBankHex == NULL || pBankHex->IsWater() ||
			!m_pMapUtil->m_tdWheel->CanTravelHex( pBankHex ) )
			continue;
		// server refused a span from this bank recently
		{
			std::map<int, DWORD>::iterator itD =
				m_mBridgeDeny.find( m_pMapUtil->GetMapOffset( hexTry.X(), hexTry.Y() ) );
			if( itD != m_mBridgeDeny.end() )
			{
				if( timeGetTime() < itD->second )
					continue;
				m_mBridgeDeny.erase( itD );
			}
		}
		BOOL bBlockedTry;
		EnterCriticalSection( &cs );
		bBlockedTry = ( theBuildingHex.GetBuilding( hexTry ) != NULL );
		LeaveCriticalSection( &cs );
		if( bBlockedTry )
			continue;

		// the hex ahead in the span direction must still be river
		CHexCoord hexAhead = hexTry;
		switch( iDir )
		{
			case 0: hexAhead.Ydec(); break;
			case 2: hexAhead.Xinc(); break;
			case 4: hexAhead.Yinc(); break;
			case 6: hexAhead.Xdec(); break;
		}
		CHex *pAheadHex = theMap.GetHex( hexAhead );
		if( pAheadHex == NULL || !pAheadHex->IsWater() )
			continue;

		// crossing must land on traversable unbuilt ground within the owner's span
		// (bRequirePlan FALSE: planning a NEW crossing over raw water - the
		// plan marks are laid after success, requiring them here = never succeed)
		if( m_pMapUtil->TryBridgeWalk( hexTry, iDir, iMaxSpan, hexEnd, FALSE ) )
		{
			hexBank    = hexTry;
			bFoundSpan = TRUE;
		}
	}
	if( !bFoundSpan )
	{
		// a river we WANT to cross but can't span anywhere nearby: research nudge
		m_iBridgeSpanFails++;
#if EN_AI_PROBES_ECON && defined(_WIN32)
		{
			char szB[112];
			sprintf( szB, "[BRIDGEMISS] plyr %d nowalk at bank %d,%d dir %d span %d\n", m_iPlayer,
				hexBank.X(), hexBank.Y(), iDir, iMaxSpan );
			OutputDebugStringA( szB );
		}
#endif
		return FALSE;
	}

	// flag bank -> river span -> landing as planned road
	CHexCoord hexMark = hexBank;
	for( int i = 0; i <= iMaxSpan + 2; i++ )
	{
		int j = m_pMapUtil->GetMapOffset( hexMark.X(), hexMark.Y() );
		if( j >= 0 && j < m_iMapSize &&
			!( m_pwaMap[j] & MSW_AI_BUILDING ) && !( m_pwaMap[j] & MSW_PLANNED_ROAD ) )
		{
			m_pwaMap[j] |= MSW_PLANNED_ROAD;
			m_iRoadCount++;
			m_aPlannedRoad.push_back( j );
		}
		if( hexMark == hexEnd )
			break;
		switch( iDir )
		{
			case 0: hexMark.Ydec(); break;
			case 2: hexMark.Xinc(); break;
			case 4: hexMark.Yinc(); break;
			case 6: hexMark.Xdec(); break;
		}
	}

	m_bPendingBridge = TRUE;
#if EN_AI_PROBES_ECON && defined(_WIN32)
	{
		char szB[128];
		sprintf( szB, "[BRIDGEPLAN] plyr %d clamped-crane crossing %d,%d dir %d\n",
			m_iPlayer, hexBank.X(), hexBank.Y(), iDir );
		OutputDebugStringA( szB );
	}
#endif
	return TRUE;
}

//
// dispatch-time claim: between "crane sent to the crossing" and "server accept
// marks the span" the crossing looked unclaimed, so every freed road crane
// re-took it (3 cranes on one span, soak17). Short TTL so a dead crane cannot
// lock the crossing forever.
//
void CAIMap::ClaimBridge( CHexCoord const& hexStart )
{
	m_mBridgeDeny[m_pMapUtil->GetMapOffset( hexStart.X(), hexStart.Y() )] =
		timeGetTime() + 3 * 60 * 1000;
}

//
// server refused the span (end-base check): unmark its hexes so road cranes
// skip it and deny replanning from that bank for 30 min
//
void CAIMap::DenyBridge( CHexCoord const& hexStart, CHexCoord const& hexEnd )
{
	int dx = CHexCoord::Diff( hexEnd.X() - hexStart.X() );
	int dy = CHexCoord::Diff( hexEnd.Y() - hexStart.Y() );
	int sx = ( dx > 0 ) - ( dx < 0 ), sy = ( dy > 0 ) - ( dy < 0 );
	CHexCoord hexMark = hexStart;
	for( int i = 0; i <= abs( dx ) + abs( dy ); i++ )
	{
		int j = m_pMapUtil->GetMapOffset( hexMark.X(), hexMark.Y() );
		if( j >= 0 && j < m_iMapSize && ( m_pwaMap[j] & MSW_PLANNED_ROAD ) )
		{
			m_pwaMap[j] &= ~MSW_PLANNED_ROAD;
			if( m_iRoadCount > 0 )
				m_iRoadCount--;
		}
		if( hexMark == hexEnd )
			break;
		hexMark.X( CHexCoord::Wrap( hexMark.X() + sx ) );
		hexMark.Y( CHexCoord::Wrap( hexMark.Y() + sy ) );
	}
	int jBank = m_pMapUtil->GetMapOffset( hexStart.X(), hexStart.Y() );
	m_mBridgeDeny[jBank] = timeGetTime() + 30 * 60 * 1000;
#if EN_AI_PROBES_ECON && defined(_WIN32)
	{
		char szB[96];
		sprintf( szB, "[BRIDGEDENY-AI] plyr %d span %d,%d unplanned + denied\n", m_iPlayer,
			hexStart.X(), hexStart.Y() );
		OutputDebugStringA( szB );
	}
#endif
}

//
// pick the nearest eligible planned-road hex to the crane (hexSite in/out).
// was a radius spiral over the unindexed map (FindRoadHex) -- a crane far from
// the plan "missed" even with hundreds of planned hexes elsewhere.
//
void CAIMap::GetRoadHex( CHexCoord& hexSite )
{
	CHexCoord hexOut;
	if( GetPlannedRoadNear( hexSite, hexOut ) )
	{
		hexSite = hexOut;

		// a site was selected, so mark the map planned -> road
		WORD wStatus = GetLocation( hexSite.X(), hexSite.Y() );
		if( wStatus & MSW_PLANNED_ROAD )
			wStatus ^= MSW_PLANNED_ROAD;
		wStatus |= MSW_ROAD;
		SetLocation( hexSite.X(), hexSite.Y(), wStatus );
	}
	// else: no eligible hex -> leave hexSite == crane pos so caller sees the miss
}

//
// neighbor hex carries an actual road or one of our buildings
//
BOOL CAIMap::NeighborHasRoadOrBldg( CHexCoord& hex )
{
	int j = m_pMapUtil->GetMapOffset( hex.X(), hex.Y() );
	if( j < 0 || j >= m_iMapSize )
		return FALSE;
	WORD w = m_pwaMap[j];
	return ( ( w & MSW_ROAD ) || ( w & MSW_AI_BUILDING ) ) ? TRUE : FALSE;
}

//
// ROAD AVOIDANCE (pick-time belt): TRUE if any of hex's 8 neighbors carries one
// of our own farm/lumber buildings, whose neighbor squares must stay road-free.
// AI-map high byte = GetBldgType (caimaput.cpp ConvertStatus), MSW_AI_BUILDING
// gates own buildings. Belt for OLD saves whose roads were planned before the
// pathfinder (layer 1) learned to route around these hexes.
//
BOOL CAIMap::NeighborIsFarmLumber( CHexCoord& hex )
{
	// 8-neighborhood offsets (N, NE, E, SE, S, SW, W, NW)
	static const int adx[MAX_ADJACENT] = {  0,  1,  1,  1,  0, -1, -1, -1 };
	static const int ady[MAX_ADJACENT] = { -1, -1,  0,  1,  1,  1,  0, -1 };

	for( int d = 0; d < MAX_ADJACENT; ++d )
	{
		CHexCoord hexT( CHexCoord::Wrap( hex.X() + adx[d] ),
			CHexCoord::Wrap( hex.Y() + ady[d] ) );
		WORD w = GetLocation( hexT.X(), hexT.Y() );
		if( !( w & MSW_AI_BUILDING ) )
			continue;
		int iType = w >> 8;	// base type via GetBldgType, per ConvertStatus
		if( iType == CStructureData::farm ||
			iType == CStructureData::lumber )
			return TRUE;
	}
	return FALSE;
}

//
// replicates CAIMapUtil::FindRoadHex eligibility (caimaput.cpp ~3162-3227):
// planned flag set, not a building, not river, no bldg/vehicle on the game hex,
// and an actual road/building at cardinal neighbor 0/2/4/6.
//
BOOL CAIMap::IsRoadHexEligible( int iOff, CHexCoord& hexRoad )
{
	WORD w = m_pwaMap[iOff];
	if( !( w & MSW_PLANNED_ROAD ) )
		return FALSE;
	if( ( w & MSW_AI_BUILDING ) || ( w & MSW_OPFOR_BUILDING ) )
		return FALSE;

	// skip planned road hexes on water (bridge candidates, not paveable roads)
	CHex *pGameHex = theMap.GetHex( hexRoad );
	if( pGameHex == NULL )
		return FALSE;
	if( pGameHex->IsWater() )
		return FALSE;

	BYTE bUnits = pGameHex->GetUnits();
	if( bUnits & CHex::bldg )						// a building sits here
		return FALSE;
	if( bUnits & ( CHex::ul | CHex::ur | CHex::ll | CHex::lr ) )	// a vehicle
		return FALSE;

	// ROAD AVOIDANCE (belt): never pave a hex abutting an own farm/lumber
	if( NeighborIsFarmLumber( hexRoad ) )
		return FALSE;

	// only accept planned roads adjacent to a real road/building at 0,2,4,6
	CHexCoord hexT;
	hexT = hexRoad; hexT.Ydec();  if( NeighborHasRoadOrBldg( hexT ) ) return TRUE;	// 0
	hexT = hexRoad; hexT.Xinc();  if( NeighborHasRoadOrBldg( hexT ) ) return TRUE;	// 2
	hexT = hexRoad; hexT.Yinc();  if( NeighborHasRoadOrBldg( hexT ) ) return TRUE;	// 4
	hexT = hexRoad; hexT.Xdec();  if( NeighborHasRoadOrBldg( hexT ) ) return TRUE;	// 6
	return FALSE;
}

//
// nearest eligible planned-road hex to hexCrane, via the exact index.
// lazy-drops entries whose MSW_PLANNED_ROAD flag is gone. Runs on this AI's
// own thread (planner + picker both do), same as every other m_pwaMap write --
// no locking needed. O(index size); ~1-2k entries.
//
BOOL CAIMap::GetPlannedRoadNear( CHexCoord& hexCrane, CHexCoord& hexOut )
{
	int iBestDist = m_iMapSize + 1;
	int iBestOff  = -1;
	CHexCoord hexBest;

	size_t k = 0;
	while( k < m_aPlannedRoad.size() )
	{
		int off = m_aPlannedRoad[k];

		// lazy removal: entry no longer a planned road -> swap-and-pop
		if( off < 0 || off >= m_iMapSize || !( m_pwaMap[off] & MSW_PLANNED_ROAD ) )
		{
			m_aPlannedRoad[k] = m_aPlannedRoad.back();
			m_aPlannedRoad.pop_back();
			continue;
		}

#if THREADS_ENABLED
		myYieldThread();	// same yield the old spiral did per candidate
#endif

		int iX, iY;
		m_pMapUtil->OffsetToXY( off, &iX, &iY );
		CHexCoord hexCand( iX, iY );

		// eligible-but-not-nearest stays indexed (may qualify once roads grow)
		if( IsRoadHexEligible( off, hexCand ) )
		{
			// dist>0: the old spiral scanned rings from step 1, never the crane's
			// own hex; excluding it keeps GetRoadHex's mark-vs-miss test intact
			int iDist = pGameData->GetRangeDistance( hexCrane, hexCand );
			if( iDist > 0 && iDist < iBestDist )
			{
				iBestDist = iDist;
				iBestOff  = off;
				hexBest   = hexCand;
			}
		}
		++k;
	}

	if( iBestOff < 0 )
		return FALSE;	// entries may remain, just none eligible right now

	hexOut = hexBest;
	return TRUE;
}

//
// count of live planned-road entries (prunes stale as it scans). Used to tell
// a disconnected plan (entries exist, none eligible) from true exhaustion.
//
int CAIMap::GetPlannedCount( void )
{
	size_t k = 0;
	while( k < m_aPlannedRoad.size() )
	{
		int off = m_aPlannedRoad[k];
		if( off < 0 || off >= m_iMapSize || !( m_pwaMap[off] & MSW_PLANNED_ROAD ) )
		{
			m_aPlannedRoad[k] = m_aPlannedRoad.back();
			m_aPlannedRoad.pop_back();
			continue;
		}
		++k;
	}
	return (int)m_aPlannedRoad.size();
}

//
// rebuild the runtime planned-road index from the map (e.g. after load, which
// reads the raw m_pwaMap buffer straight from the save).
//
void CAIMap::RebuildPlannedIndex( void )
{
	m_aPlannedRoad.clear();
	if( m_pwaMap == NULL )
		return;
	for( int i = 0; i < m_iMapSize; ++i )
		if( m_pwaMap[i] & MSW_PLANNED_ROAD )
			m_aPlannedRoad.push_back( i );

	// heal a saved corrupt count (seen -2406 in a live save) -- the index is
	// the source of truth
	if( m_iRoadCount < (int)m_aPlannedRoad.size() )
		m_iRoadCount = (int)m_aPlannedRoad.size();
}

//
// batch road: extend a just-picked road hex (already marked ROAD by GetRoadHex)
// into a STRAIGHT CARDINAL run of contiguous MSW_PLANNED_ROAD hexes so a crane
// can pave the strip in one order. Cardinal-only: the vehicle's NextRoadHex
// steps the longer axis one hex at a time, so a diagonal "run" would pave a
// staircase, not the hexes we mark. Marks every run hex PLANNED->ROAD so no
// other crane claims the strip. hexEnd = last run hex; returns hex count.
//
int CAIMap::GetRoadRun( const CHexCoord& hexStart, CHexCoord& hexEnd, int iMaxHexes )
{
	hexEnd = hexStart;
	if( iMaxHexes < 2 )
		return 1;

	// E, W, S, N (cardinal only)
	static const int adx[4] = {  1, -1,  0,  0 };
	static const int ady[4] = {  0,  0,  1, -1 };

	int iBaseCol = (int)m_wBaseCol, iBaseRow = (int)m_wBaseRow;
	int iCol = (int)hexStart.X(), iRow = (int)hexStart.Y();

	// pick the first cardinal direction with a planned-road neighbor
	int iDir = -1;
	for( int d = 0; d < 4; ++d )
	{
		int nx = iCol + adx[d], ny = iRow + ady[d];
		if( nx < iBaseCol || nx >= iBaseCol + (int)m_wCols ||
			ny < iBaseRow || ny >= iBaseRow + (int)m_wRows )
			continue;
		if( GetLocation( (WORD)nx, (WORD)ny ) & MSW_PLANNED_ROAD )
		{
			// ROAD AVOIDANCE (belt): don't run into a farm/lumber-adjacent hex
			CHexCoord hexN( nx, ny );
			if( NeighborIsFarmLumber( hexN ) )
				continue;
			iDir = d;
			break;
		}
	}
	if( iDir < 0 )
		return 1;	// no straight extension -- single-hex fallback

	// walk straight, claiming each planned-road hex, up to iMaxHexes total
	int iCount = 1;
	while( iCount < iMaxHexes )
	{
		int nx = iCol + adx[iDir], ny = iRow + ady[iDir];
		if( nx < iBaseCol || nx >= iBaseCol + (int)m_wCols ||
			ny < iBaseRow || ny >= iBaseRow + (int)m_wRows )
			break;
		WORD wStatus = GetLocation( (WORD)nx, (WORD)ny );
		if( !( wStatus & MSW_PLANNED_ROAD ) )
			break;
		// ROAD AVOIDANCE (belt): stop the run before a farm/lumber-adjacent hex
		CHexCoord hexRun( nx, ny );
		if( NeighborIsFarmLumber( hexRun ) )
			break;
		// stop at water: bridge candidates, never paveable (mirror IsRoadHexEligible)
		{
			CHex* pRunHex = theMap.GetHex( hexRun );
			if( pRunHex == NULL || pRunHex->IsWater() )
				break;
		}
		wStatus &= ~MSW_PLANNED_ROAD;	// claim: planned -> built
		wStatus |= MSW_ROAD;
		SetLocation( (WORD)nx, (WORD)ny, wStatus );
		iCol = nx; iRow = ny;
		hexEnd.X( nx ); hexEnd.Y( ny );
		++iCount;
	}
	return iCount;
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
	catch( CException* e )
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
	CHexCoord& hexPlace, int iVehType, CHexCoord* phexRocket /*=NULL*/ )
{
	int iOffset = pGameData->m_iHexPerBlk / 4;
	CHexCoord hcStart;
	hcStart.X( hcStart.Wrap( hexStart.X() + iOffset ));
	hcStart.Y( hcStart.Wrap( hexStart.Y() + iOffset ));
	CHexCoord hcEnd;
	hcEnd.X( hcEnd.Wrap( hexEnd.X() - iOffset ));
	hcEnd.Y( hcEnd.Wrap( hexEnd.Y() - iOffset ));

	// phexRocket non-NULL => initial AI vehicle placement: reject water hexes
	// for land vehicles and require a path back to the rocket (see FindStagingHex)
	m_pMapUtil->FindStagingHex( hcStart.X(), hcStart.Y(),
		hcEnd.X(), hcEnd.Y(), hexPlace, iVehType, FALSE, phexRocket );
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
void CAIMap::Save( CArchive& ar )
{
    // int iX, iY;
    // iX = m_RocketHex.X();
    // iY = m_RocketHex.Y();
    try
    {
        ar << m_iPlayer;
        ar << m_wRows;
        ar << m_wCols;
        ar << m_wBaseRow;
        ar << m_wBaseCol;
        // pFile->Write( (const void*)&iX, sizeof(int) );
        // pFile->Write( (const void*)&iY, sizeof(int) );
        ar << m_iBaseX;
        ar << m_iBaseY;
        ar << m_iMapSize;
        ar << m_iRoadCount;
        ar << m_iOcean;
        ar << m_iLake;
        ar << m_iLand;

        ar.Write( (const void*)m_pwaMap, ( sizeof( WORD ) * m_iMapSize ) );
    }
    catch ( CFileException* theException )
    {
        // BUGBUG how should write errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }
    // save the utility
    m_pMapUtil->Save( ar );
}

void CAIMap::Load( CArchive& ar, CAIUnitList* plUnits )
{
    // int iX, iY;

    try
    {
        ar >> m_iPlayer;
        ar >> m_wRows;
        ar >> m_wCols;
        ar >> m_wBaseRow;
        ar >> m_wBaseCol;
        // ar >> iX;
        // ar >> iY;
        ar >> m_iBaseX;
        ar >> m_iBaseY;
        ar >> m_iMapSize;
        ar >> m_iRoadCount;
        ar >> m_iOcean;
        ar >> m_iLake;
        ar >> m_iLand;
    }
    catch ( CFileException* theException )
    {
        // how should read errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }

    // m_RocketHex.X( iX );
    // m_RocketHex.Y( iY );

    // map size might have changed
    if ( m_pwaMap != NULL )
    {
        delete[] m_pwaMap;
        m_pwaMap = NULL;
    }

    try
    {
        m_pwaMap = new WORD[m_iMapSize];
        memset( m_pwaMap, 0, (size_t)( m_iMapSize * sizeof( WORD ) ) );
    }
    catch ( CException* theException )
    {
        if ( m_pwaMap != NULL )
        {
            delete[] m_pwaMap;
            m_pwaMap = NULL;
        }
        throw( ERR_CAI_BAD_NEW );
    }

    // read raw map buffer
    try
    {
        UINT uBytes = ar.Read( (void*)m_pwaMap, (UINT)( sizeof( WORD ) * m_iMapSize ) );
        if ( uBytes != (UINT)( sizeof( WORD ) * m_iMapSize ) )
            throw( ERR_CAI_BAD_FILE );
    }
    catch ( CFileException* theException )
    {
        throw( ERR_CAI_BAD_FILE );
    }

    m_pMapUtil->Load( ar, m_pwaMap, plUnits );

    // planned-road index is runtime-only (not serialized) -- rebuild from the
    // just-read map so post-load road picking works.
    RebuildPlannedIndex();
}

// end of CAIMap.cpp
