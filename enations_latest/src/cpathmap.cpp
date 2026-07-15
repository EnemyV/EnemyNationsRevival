////////////////////////////////////////////////////////////////////////////
//
//  CPathMap.cpp : CPathMap object implementation
//                 Divide and Conquer
//               
//  Last update:   10/29/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cai.h"
#include "cpathmap.h"

#include "caidata.hpp"
#include "logging.h"	// dave's logging system
#include "Perf.h"		// EN_PERF counters (path volume/cost split)
#include "enprobes.h"	// temp AI probe gates


//#define TEST_RESULT1		// test GetAt improvement
//#define TEST_RESULT2			// test GetLowest improvement

CPathMap thePathMap;

#define new DEBUG_NEW

//extern CRITICAL_SECTION cs;	// used by threads

//
// critical section bracketed version of GetRoadPath()
//
CHexCoord *CPathMap::GetRoadPath(
	CHexCoord& hexFrom, CHexCoord& hexTo, int& iPathLen, WORD *pMap,
	BOOL bAllowWater /*=FALSE*/, BOOL bRiverCrossing /*=TRUE*/,
	BOOL bWarRoad /*=FALSE*/ )
{
	// EN_PERF: time the whole call (incl. m_cs wait) + count searches/nodes.
	// Per-SEARCH counter ops only — CounterInc takes a global CS, so a
	// per-node counter in AddCellToArray would serialize the AI threads.
	Perf::ScopeCounter _t( "pathr.us" );
	EnterCriticalSection (&m_cs);
	m_iNextSlot = 0;	// early-outs skip the in-search reset; don't re-count
	CHexCoord *phcPath = _GetRoadPath( hexFrom, hexTo, iPathLen,
		pMap, bAllowWater, bRiverCrossing, bWarRoad );
	Perf::CounterInc( "pathr.calls" );
	Perf::CounterAdd( "pathr.nodes", m_iNextSlot );	// cells created this search
	LeaveCriticalSection (&m_cs);
	return( phcPath );
}

//
// actual GetRoadPath()
//
CHexCoord *CPathMap::_GetRoadPath(
	CHexCoord& hexFrom, CHexCoord& hexTo, int& iPathLen, WORD *pMap,
	BOOL bAllowWater, BOOL bRiverCrossing, BOOL bWarRoad )
{
	// war roads span half the map; the economy hang/arena silently NULLed
	// every long plan (PlanWarRoad fired, nothing was ever laid)
	m_bWarRoad = bWarRoad;
#if PATH_TIMING_ROAD
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif

	// no path from start to destination because we are there
	if( hexFrom == hexTo )
		return( NULL );

	m_pMap = pMap;
	m_iBaseX = 0;
	m_iBaseY = 0;
	m_iMapEX = (m_iBaseX + m_iWidth) - 1;
	m_iMapEY = (m_iBaseY + m_iHeight) - 1;

	m_pTD = theTransports.GetData(CTransportData::construction);
	if( m_pTD == NULL )
		return( NULL );

	// set best cost threshhold so its very high
	// and it must match what is tested in GetLowestCost()
	m_iBestCost = 0xFFFE; //(m_iWidth * m_iHeight);

	// change to reflect new approach
	m_iFirst = 0;
	m_iLast = 0;
	m_iNextSlot = 0;
	m_iLowestBoth = 0;
	// belt-and-suspenders vs ClearArray-at-exit: even if a prior search
	// leaked registrations past its ClearArray, a fresh generation here
	// guarantees this search can never see them
	NewSearchGen( );

	m_hexFrom = hexFrom;
	m_hexTo = hexTo;

	// set up hexFrom as first test cell
	//CCell *pTest = new CCell();
	CCell *pTest = NULL;
	CCell *pLastTest = NULL;
	CCell ccTest( m_hexFrom.X(), m_hexFrom.Y() );

	//pTest->m_iX = m_hexFrom.X();
	//pTest->m_iY = m_hexFrom.Y();

#if PATH_TIMING_ROAD
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"FindRoadPath from %d,%d to %d,%d  range is %d ", 
		m_hexFrom.X(), m_hexFrom.Y(), m_hexTo.X(), m_hexTo.Y(),
		theMap.GetRangeDistance(m_hexFrom, m_hexTo) );
#endif
#endif

	// set flags relative to the type of path to find, with the
	// m_bOverWater set, the path can contain land/water/land,
	// with m_bRoadPlanning set, the path crosses rivers.
	//
	if( bAllowWater )
		m_bOverWater = TRUE;
	else
		m_bRoadPlanning = bRiverCrossing;

	// set special state for hexFrom
	//pTest->m_iCost = 0;
	//pTest->m_iDist = 0;
	//pTest->m_iBoth = 0;
	ccTest.m_iCost = 0;
	ccTest.m_iDist = 0;
	ccTest.m_iBoth = 0;

	//AddCellToArray( pTest );
	//CCell *pAdjCell = pTest;
	//pTest = GetCellAt(pAdjCell->m_iX,pAdjCell->m_iY);
	//delete pAdjCell;
	pTest = AddCellToArray( &ccTest );
#ifdef TEST_RESULT2
	bugbug
	TRAP ( pTest != GetCellAt(ccTest.m_iX,ccTest.m_iY) );
#endif
	if( pTest == NULL )
		return( FALSE );
	CCell *pAdjCell = NULL;
	
	int iX,iY;
	int iHang = (m_iWidth + m_iHeight) * 2;
	if( m_bWarRoad )
		iHang = 0xFFFF;
	int iTicks = 0;
	int iList = 1;

	// start looping to find a path
	while( TRUE )
	{
		if( AtDestination(pTest) )
			break;

		// for each adjacent cell to the test cell
		for( int i=0; i<CELLSAROUND; ++i )
		{
			// roads only want 0,2,4,6
			if( i & 1 )
				continue;

			// get the adjacent cell x,y values
			// in the 'i' direction from pTest
			GetCellAt( i, pTest, iX, iY );
			if( !iX && !iY )
				continue;

			// consider if that cell is already in list
			pAdjCell = GetCellAt(iX,iY);
			if( pAdjCell == NULL )
			{
				CCell aAdjCell( iX,iY );
				pAdjCell = AddCellToArray( &aAdjCell );
#ifdef TEST_RESULT2
				TRAP ( pAdjCell != GetCellAt(iX,iY) );
#endif

				iList++;
			}
			if( pAdjCell == NULL )
			{
				iHang = 1;
				break;
			}
			// 
			// get distance from pAdjCell to destination,
			// and make pAdjCell point to pTest
			GetCellCosts( pTest, pAdjCell );

			if( AtDestination(pAdjCell) ) //|| IsAlreadyRoad(pAdjCell) )
			{
				CHexCoord *phexPath = CreateHexPath( iPathLen, pAdjCell );

				ClearArray();

#if PATH_TIMING_ROAD
	dwEnd = timeGetTime();
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"GetRoadPath() for AI map took %ld ticks for ", 
	(dwEnd - dwStart));
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	" %d inerations with %d cells in list \n", 
	++iTicks, iList );
#endif
#endif
				m_bRoadPlanning = FALSE;
				m_bOverWater = FALSE;
				return( phexPath );
			}
		}

		// flag pTest so that it does not get re-picked
		pTest->m_iBoth = 0;
		NewBoth ( pTest );

		// get lowest combined cost cell in list to repeat
		pTest = GetLowestCost();
		if( pTest == NULL )
		{
			pTest = NULL;
			break;
		}

		// consider we have reached the destination
		if( AtDestination(pTest) )
			break;

		// put this in to prevent hangs
		iHang--;
		if( !iHang )
		{
			pTest = NULL;
			break;
		}

		iTicks++;
	}

	m_bRoadPlanning = FALSE;
	m_bOverWater = FALSE;

	CHexCoord *phexPath = CreateHexPath( iPathLen, pTest );
	
	ClearArray();

	if( pTest == NULL )
	{

#if PATH_TIMING_ROAD
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"GetRoadPath() for AI map failed to reach destination " );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	" %d inerations with %d cells in list \n", 
	++iTicks, iList );
#endif
#endif
		
		return( NULL );
	}

#if PATH_TIMING_ROAD
	dwEnd = timeGetTime();
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"GetRoadPath() for AI map took %ld ticks for ", 
		(dwEnd - dwStart));
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	" %d inerations with %d cells in list \n", 
	++iTicks, iList );
#endif
#endif

	return( phexPath );
}

//
// critical section bracketed version of GetPath()
//
BOOL CPathMap::GetPath( CHexCoord& hexFrom, CHexCoord& hexTo,
	int iBaseX, int iBaseY, WORD *pMap, int iVehType,
	BOOL bLongHang /*=FALSE*/, BOOL bWarPlanning /*=FALSE*/ )
{
	// this is a long expensive function that blocks everybody...
	// EN_PERF: per-search counters (see GetRoadPath note — never per-node).
	// path.us/path.calls vs path.nodes splits the verdict: volume-bound
	// (many calls, few nodes) -> cache quantization; node-bound (few calls,
	// many nodes) -> scratch/world-read structure work.
	Perf::ScopeCounter _t( "path.us" );
	EnterCriticalSection (&m_cs);
	m_iNextSlot = 0;	// early-outs skip the in-search reset; don't re-count
	BOOL bPath = _GetPath( hexFrom, hexTo,
		iBaseX, iBaseY, pMap, iVehType, bLongHang, bWarPlanning );
	Perf::CounterInc( "path.calls" );
	Perf::CounterAdd( "path.nodes", m_iNextSlot );	// cells created this search
	LeaveCriticalSection (&m_cs);
	return( bPath );
}

//
// determine if a path will reach hexTo from the location hexFrom
// and return TRUE if it does
//
BOOL CPathMap::_GetPath( CHexCoord& hexFrom, CHexCoord& hexTo,
	int iBaseX, int iBaseY, WORD *pMap, int iVehType, BOOL bLongHang,
	BOOL bWarPlanning )
{
	// no path from start to destination because we are there
	if( hexFrom == hexTo )
		return( FALSE );

	// the map to use
	m_pMap = pMap;
	if( pMap == NULL )
	{
		m_iBaseX = 0;
		m_iBaseY = 0;
	}
	else
	{
		m_iBaseX = iBaseX;
		m_iBaseY = iBaseY;
	}
	m_iMapEX = (m_iBaseX + m_iWidth) - 1;
	m_iMapEY = (m_iBaseY + m_iHeight) - 1;

	// iVehType may come in <0 indicating to try path with all
	// vehicle groups
	m_pTD = NULL;
	if( iVehType >= 0 && iVehType < CTransportData::num_types )
	{
		m_pTD = theTransports.GetData(iVehType);
	    if( m_pTD == NULL )
		    return( FALSE );
	}

	// set best cost threshhold so its very high
	// and it must match what is tested in GetLowestCost()
	m_iBestCost = 0xFFFE;

	// change to reflect new approach
	m_iFirst = 0;
	m_iLast = 0;
	m_iNextSlot = 0;
	m_iLowestBoth = 0;
	// belt-and-suspenders vs ClearArray-at-exit: even if a prior search
	// leaked registrations past its ClearArray, a fresh generation here
	// guarantees this search can never see them
	NewSearchGen( );

	m_hexFrom = hexFrom;
	m_hexTo = hexTo;

	// set up hexFrom as first test cell
	CCell *pTest = NULL;
	CCell ccTest( m_hexFrom.X(), m_hexFrom.Y() );
	CCell *pLastTest = NULL;

#if PATH_TIMING_MAP
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 

	TRACE( "FindMapPath from %d,%d to %d,%d  range is %d \n", 
		m_hexFrom.X(), m_hexFrom.Y(), m_hexTo.X(), m_hexTo.Y(),
		theMap.GetRangeDistance(m_hexFrom, m_hexTo) );
#endif

	m_bRoadPlanning = FALSE;
	m_bWarPlanning = bWarPlanning;	// war mode: rivers passable + bigger budget
	m_bWarRoad = FALSE;				// road-only flag; never active in GetPath
	m_bOverWater = FALSE;

	// set special state for hexFrom
	ccTest.m_iCost = 0;
	ccTest.m_iDist = 0;
	ccTest.m_iBoth = 0;

	pTest = AddCellToArray( &ccTest );
#ifdef TEST_RESULT2
	TRAP ( pTest != GetCellAt(ccTest.m_iX,ccTest.m_iY) );
#endif
	if( pTest == NULL )
		return( FALSE );
	CCell *pAdjCell = NULL;
	
	// the iHang controls how long the routine will try
	// to find a path before punting
	int iHang = m_iNumOfCells;
	if( !bLongHang )
		iHang = m_iNumOfCells * 2;

	// war mode: with the closed set each cell is popped at most once, so pops
	// are bounded by the war arena cap and the hang guard is a pure backstop.
	// (The old dist*8 raise never exceeded the default and never engaged.)
	if( m_bWarPlanning )
		iHang = 0xFFFF;

	int iX,iY;
	int iTicks = 0;
	int iList = 1;

#if EN_AI_PROBES_WAR && defined(_WIN32)
	// war-probe: which ceiling killed a war-planning search
	const char* pszWarFail = NULL;
#endif

	// start looping to find a path
	while( TRUE )
	{
		if( AtDestination(pTest) )
			break;

		// for each adjacent cell to the test cell
		for( int i=0; i<CELLSAROUND; ++i )
		{
			// get the adjacent cell x,y values
			// in the 'i' direction from pTest
			GetCellAt( i, pTest, iX, iY );

			// consider if that cell is already in list
			pAdjCell = GetCellAt(iX,iY);
			if( pAdjCell == NULL )
			{
				CCell aAdjCell( iX,iY );
				pAdjCell = AddCellToArray( &aAdjCell );
#ifdef TEST_RESULT2
				TRAP ( pAdjCell != GetCellAt(iX,iY) );
#endif

				iList++;
			}
			if( pAdjCell == NULL )
			{
#if EN_AI_PROBES_WAR && defined(_WIN32)
				if( pszWarFail == NULL )
					pszWarFail = "arena-full";
#endif
				iHang = 1;
				break;
			}
			//
			// get distance from pAdjCell to destination,
			// and make pAdjCell point to pTest
			GetCellCosts( pTest, pAdjCell );

			if( AtDestination(pAdjCell) )
			{
				ClearArray();
#if PATH_TIMING_MAP
				dwEnd = timeGetTime();
				TRACE( "GetPath() for AI map took %ld ticks for \n", 
					(dwEnd - dwStart));
				TRACE( " %d inerations with %d cells in list \n\n",
					++iTicks, iList );
#endif
#if EN_AI_PROBES_WAR && defined(_WIN32)
				if( bWarPlanning )
				{
					char szW[160];
					sprintf( szW, "[WARPATH] ok from %d,%d to %d,%d dist %d cells %d/%d\n",
						m_hexFrom.X( ), m_hexFrom.Y( ), m_hexTo.X( ), m_hexTo.Y( ),
						theMap.GetRangeDistance( m_hexFrom, m_hexTo ), m_iNextSlot, m_iCellAlloc );
					OutputDebugStringA( szW );
				}
#endif
				m_bWarPlanning = FALSE;
				return( TRUE );
			}
		}

		// flag pTest so that it does not get re-picked
		pTest->m_iBoth = 0;
		pTest->m_bClosed = 1;	// boolean search: expanded cells stay closed
		NewBoth ( pTest );

		// get lowest combined cost cell in list to repeat
		pTest = GetLowestCost();
		if( pTest == NULL )
		{
#if EN_AI_PROBES_WAR && defined(_WIN32)
			// open list dry: region walled off, or every frontier cell
			// overflowed the MAX_BOTH_INDEX bucket table
			if( pszWarFail == NULL )
				pszWarFail = "open-empty";
#endif
			break;
		}

		// consider we have reached the destination
		if( AtDestination(pTest) )
			break;

		// put this in to prevent hangs and to limit how long
		// the routine looks for a path
		iHang--;
		if( !iHang )
		{
#if EN_AI_PROBES_WAR && defined(_WIN32)
			if( pszWarFail == NULL )
				pszWarFail = "hang-exhausted";
#endif
			pTest = NULL;
			break;
		}

		iTicks++;
	}

	m_bWarPlanning = FALSE;
	ClearArray();

	if( pTest == NULL )
	{
#if PATH_TIMING_MAP
	TRACE( "GetPath() for AI map failed to reach destination \n" );
	TRACE( "after %d inerations with %d cells in list \n\n",
		++iTicks, iList );
#endif
#if EN_AI_PROBES_WAR && defined(_WIN32)
		if( bWarPlanning )
		{
			char szW[160];
			sprintf( szW, "[WARPATH] FAIL %s from %d,%d to %d,%d dist %d cells %d/%d\n",
				pszWarFail != NULL ? pszWarFail : "?",
				m_hexFrom.X( ), m_hexFrom.Y( ), m_hexTo.X( ), m_hexTo.Y( ),
				theMap.GetRangeDistance( m_hexFrom, m_hexTo ), m_iNextSlot, m_iCellAlloc );
			OutputDebugStringA( szW );
		}
#endif
		return( FALSE );
	}

#if PATH_TIMING_MAP
	dwEnd = timeGetTime();
	TRACE( "GetPath() for AI map took %ld ticks for \n",
		(dwEnd - dwStart));
	TRACE( " %d inerations with %d cells in list \n\n",
		++iTicks, iList );
#endif

#if EN_AI_PROBES_WAR && defined(_WIN32)
	if( bWarPlanning )
	{
		char szW[160];
		sprintf( szW, "[WARPATH] ok from %d,%d to %d,%d dist %d cells %d/%d\n",
			m_hexFrom.X( ), m_hexFrom.Y( ), m_hexTo.X( ), m_hexTo.Y( ),
			theMap.GetRangeDistance( m_hexFrom, m_hexTo ), m_iNextSlot, m_iCellAlloc );
		OutputDebugStringA( szW );
	}
#endif

	return( TRUE );
}


//
// consider if the cell passed is the destination cell
//
BOOL CPathMap::AtDestination( CCell *pCell )
{
	if( pCell->m_iX == m_hexTo.X() &&
		pCell->m_iY == m_hexTo.Y() )
		return TRUE;

	return FALSE;
}

//
// get range to destination for 'to' cell
// and record distance and combine the two into 'to' both
//
void CPathMap::GetCellCosts( CCell *pFromCell, CCell *pToCell )
{
	// this cell is the start cell and should not be costed
	if( !pToCell->m_iCost )
		return;

	// already expanded: never re-open. Only _GetPath sets m_bClosed (its answer
	// is a reachability BOOL - no caller reads path or cost, so cost-optimality
	// is waste). The hex-range heuristic (1.5/diagonal) vs unit step cost made
	// the search re-pop each cell ~4x on long paths, exhausting iHang -> false
	// "unreachable" -> AI wars never launched. GetRoadPath doesn't close, so
	// road planning keeps its vanilla re-open behavior.
	if( pToCell->m_bClosed )
		return;

	if( AtDestination(pToCell) )
	{
		pToCell->m_pCellFrom = pFromCell;
		return;
	}

	// create hex for test cell
	CHexCoord hexDest( pToCell->m_iX, pToCell->m_iY );
	CHex *pGameHex = theMap.GetHex( hexDest );
	if( pGameHex == NULL )
		return;

	// BRIDGEBUG will need to create (CHex) pFromHex (CHexCoord) hexFrom
	// and test for pFromHex->GetUnits() & CHex::bridge and
	// if TRUE then use m_pTD->CanEnterHex( hexFrom, hexDest )
	// to determine if the hex can be entered and then not use
	// the m_pTD->CanTravelHex( pDestHex ) 

	// use different tests if planning a road OR planning war (both cross
	// rivers as candidate bridge sites; ocean/lake still fail CanTravelHex)
	if( m_bRoadPlanning || m_bWarPlanning )
	{
		// allow road to cross river, which create
		// candidate bridge locations
		if( pGameHex->GetType() != CHex::river )
		{
			// test to enter hex
			if( m_pTD != NULL )
			{
				if( !m_pTD->CanTravelHex( pGameHex ) )
					return;
			}
		}
	}
	else if( m_bOverWater )
	{
		// allow passage of all water
		//if( pGameHex->GetType() != CHex::river &&
		//	pGameHex->GetType() != CHex::ocean &&
		//	pGameHex->GetType() != CHex::lake )
		if( !pGameHex->IsWater() )
		{
			// test to enter hex
			if( m_pTD != NULL )
			{
				if( !m_pTD->CanTravelHex( pGameHex ) )
					return;
			}
		}
	}
	else
	{
		// test to enter hex
		if( m_pTD == NULL )
		{
			// need to try all base types of vehicles
			if( m_tdWheel != NULL &&
				!m_tdWheel->CanTravelHex( pGameHex ) )
				return;
			if( m_tdTrack != NULL &&
				!m_tdTrack->CanTravelHex( pGameHex ) )
				return;
			if( m_tdHover != NULL &&
				!m_tdHover->CanTravelHex( pGameHex ) )
				return;
			if( m_tdFoot != NULL &&
				!m_tdFoot->CanTravelHex( pGameHex ) )
				return;
		}
		else if( !m_pTD->CanTravelHex( pGameHex ) )
			return;
	}

	BYTE bUnits = pGameHex->GetUnits();
	if( bUnits & CHex::bldg && hexDest != m_hexTo )
		return;

	// check for buildings when the map is not NULL which
	// is except for when paths across water are desired
	if( m_pMap != NULL )
	{
		BOOL bBug = FALSE;
		if( pToCell->m_iX < 0 || pToCell->m_iX > m_iMapEX ||
			pToCell->m_iY < 0 || pToCell->m_iY > m_iMapEY )
			bBug = TRUE;
		if( bBug )
		{
#if PATH_TIMING
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
"\nCPathMap::GetCellCosts() for pFromCell=%d,%d  pToCell=%d,%d ", 
pFromCell->m_iX, pFromCell->m_iY, pToCell->m_iX, pToCell->m_iX );
logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
"for a path from %d,%d to %d,%d  m_iNextSlot=%d \n", 
m_hexFrom.X(), m_hexFrom.Y(), m_hexTo.X(), m_hexTo.Y(), m_iNextSlot );
#endif
#endif
			bBug = FALSE;
			if( m_bRoadPlanning )
				m_bRoadPlanning = TRUE;
			else
				m_bRoadPlanning = FALSE;
		}

		// get AI opinion of the cell
		int i = GetOffset( pToCell->m_iX, pToCell->m_iY );
		if( i < 0 || i >= m_iNumOfMapCells )
			return;

		// avoid buildings and impassible terrain
		if( (m_pMap[i] & MSW_AI_BUILDING) ||
			(m_pMap[i] & MSW_OPFOR_BUILDING) )
			return;
	}
	
	// ROAD AVOIDANCE (plan-time): a heavy per-hex step cost for planned-road hexes
	// that abut an own farm/lumber building routes the A* AROUND them (their
	// neighbors must stay pristine for fertility/yield). Additive, not exclusion:
	// the road still connects if no clean detour exists, just at higher cost. 40
	// beats any small ring detour yet stays well under MAX_BOTH_INDEX bucketing.
	int iStep = 1;
	if( m_bRoadPlanning && FarmLumberAdjacent( pToCell->m_iX, pToCell->m_iY ) )
		iStep += 40;

	// if this cost + cost to this point < what we already have
	// then save the value and the pointer to where we came from
	if( (iStep + pFromCell->m_iCost)
		< pToCell->m_iCost )
	{
			pToCell->m_iCost = (iStep + pFromCell->m_iCost);
			pToCell->m_pCellFrom = pFromCell;
	}
	else
		return;

	pToCell->m_iDist = 
		theMap.GetRangeDistance( hexDest, m_hexTo );
	pToCell->m_iBoth = pToCell->m_iCost + pToCell->m_iDist;
	NewBoth ( pToCell );
}

//
// get the adjacent cell x,y values
// in the iPos direction from pFromCell
//
void CPathMap::GetCellAt( int iPos, CCell *pFromCell, int& iX, int& iY )
{
	CHexCoord hex( pFromCell->m_iX, pFromCell->m_iY );
	switch( iPos )
	{
		case 0:
			hex.Ydec();
			break;
		case 1:
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
			hex.Xdec();
			hex.Yinc();
			break;
		case 6:
			hex.Xdec();
			break;
		case 7:
			hex.Xdec();
			hex.Ydec();
			break;
	}
	iX = hex.X();
	iY = hex.Y();
}

//
// check the array for the cell with the lowest distance value
//
CCell *CPathMap::GetClosestCell( void )
{
	CCell *pcClosest = NULL;
	int iBestDist = 0xFFFE;

	// change to reflect new approach
	int iEnd = min( m_iNextSlot, m_iCellAlloc );	// war searches fill past m_iNumOfCells

	CCell *pCell = &m_paCells[0];
	for( int i=0; i<=iEnd; ++i, pCell++ )
	{
		if( pCell->m_iDist && pCell->m_iDist < iBestDist )
		{
			pcClosest = pCell;
			iBestDist = pCell->m_iDist;
		}
	}
	return( pcClosest );
}


// 
// considering the status of reaching the path as a guide,
// find the cell with the lowest cost and return it
//
// BUGBUG
// these functions are for using an array instead of a list
//

CCell *CPathMap::xGetLowestCost( void )
{
	CCell *pcLowest = NULL;
	int iCost = 0xFFFF;

	// change to reflect new approach
	int iEnd = min( m_iNextSlot, m_iCellAlloc );	// war searches fill past m_iNumOfCells

	CCell *pCell = &m_paCells[0];
	for( int i=0; i<iEnd; ++i, pCell++ )
	{
		// consider combined cost after dest reached 1st time
        if( pCell->m_iBoth && pCell->m_iBoth < iCost )
		{
			pcLowest = pCell;
			iCost = pCell->m_iBoth;
		}
	}

	return( pcLowest );
}

CCell* CPathMap::GetLowestCost( void )
{
    // use old method if out of range
    if ( ( m_iLowestBoth <= 0 ) || ( MAX_BOTH_INDEX <= m_iLowestBoth ) )
        return xGetLowestCost( );

    // advance m_iLowestBoth past empty buckets (cached for future calls)
    while ( m_iLowestBoth < MAX_BOTH_INDEX && m_acBoth[m_iLowestBoth] == NULL ) m_iLowestBoth++;

    // No valid bucket found: every remaining open cell has m_iBoth > MAX_BOTH_INDEX
    // (out-of-bucket by design — see cpathmgr.h; routine for long paths on big maps).
    // The linear fallback below is correct for them. This carried a TRAP() from
    // development, which on a 1024x1024 map fired CONSTANTLY during AI assault
    // pathing (every int3 = a debugger stack-walk; the storm froze the session).
    // Investigated 2026-06-10: benign designed-for fallback, not an invariant break.
    if ( m_iLowestBoth >= MAX_BOTH_INDEX )
        return xGetLowestCost( );

    // all cells in this bucket have the same m_iBoth value (cost + distance),
    // so returning the head is correct - they're all equally "lowest"
    CCell* pcLowest = m_acBoth[m_iLowestBoth];

#ifdef TEST_RESULT2
    TRAP( pcLowest != xGetLowestCost( ) );
#endif
    return pcLowest;
}
/*
CCell *CPathMap::GetLowestCost( void )
{

	// use old method if out of range
	if ( ( m_iLowestBoth <= 0 ) || ( MAX_BOTH_INDEX <= m_iLowestBoth ) )
		return xGetLowestCost ();

	// special case
	if ( ( m_acBoth [m_iLowestBoth] != NULL ) && 
										( m_acBoth [m_iLowestBoth]->m_pcNextBoth == NULL ) )
		{
#ifdef TEST_RESULT2
		TRAP ( m_acBoth [m_iLowestBoth] != xGetLowestCost () );
#endif
		return m_acBoth [m_iLowestBoth];
		}

	// find lowest entry
	CCell * * ppCellOn = &m_acBoth [m_iLowestBoth];
	int iNum = MAX_BOTH_INDEX - m_iLowestBoth;
	while ( ( *ppCellOn == NULL ) && ( iNum-- > 0 ) )
		ppCellOn ++;

	// something is wrong
	if ( iNum <= 0 )
		{
		TRAP ();
		return xGetLowestCost ();
		}

	// we now walk the linked list returning the lowest one
	CCell * pcLowest, * pCellOn;
	pcLowest = pCellOn = * ppCellOn;
	while ( pCellOn->m_pcNextBoth != NULL )
		{
		pCellOn = pCellOn->m_pcNextBoth;
		pcLowest = __min ( pcLowest, pCellOn );
		}

#ifdef TEST_RESULT2
	TRAP ( pcLowest != xGetLowestCost () );
#endif
	return( pcLowest );
}
*/

void CPathMap::NewBoth ( CCell * pTest )
{

	// first remove old
	if ( pTest->m_iBothIn != 0 )
		{
		// remove it
		if ( pTest->m_pcNextBoth != NULL )
			pTest->m_pcNextBoth->m_pcPrevBoth = pTest->m_pcPrevBoth;
		if ( pTest->m_pcPrevBoth == NULL )
			{
#ifdef TEST_RESULT2
			TRAP ( m_acBoth [pTest->m_iBothIn] != pTest );
#endif
			m_acBoth [pTest->m_iBothIn] = pTest->m_pcNextBoth;
			}
		else
			pTest->m_pcPrevBoth->m_pcNextBoth = pTest->m_pcNextBoth;
		}

	// add new
	pTest->m_pcPrevBoth = NULL;
	if ( (0 < pTest->m_iBoth) && (pTest->m_iBoth < MAX_BOTH_INDEX) )
		{
		CCell * pCelOn = m_acBoth [pTest->m_iBoth];
		pTest->m_pcNextBoth = pCelOn;
		if ( pCelOn != NULL )
			pCelOn->m_pcPrevBoth = pTest;
		m_acBoth [pTest->m_iBoth] = pTest;
		pTest->m_iBothIn = pTest->m_iBoth;
		if ( m_iLowestBoth == 0 )
			m_iLowestBoth = pTest->m_iBoth;
		else
			m_iLowestBoth = __min ( m_iLowestBoth, pTest->m_iBoth );
		}

	else
		{
		pTest->m_pcNextBoth = NULL;
		pTest->m_iBothIn = 0;
		}
}


CCell *CPathMap::GetCellAt( int iX, int iY )
{
	if( iX < 0 || iY < 0 ||
		iX > m_iMapEX ||
		iY > m_iMapEY )
		return( NULL );

	// flat-grid lookup; an entry is ours only if stamped by THIS search
	int i = ( m_iWidth * iY ) + iX;
	if( i < 0 || i >= m_iNumOfMapCells )
		return( NULL );
	if( m_pwCellGen[i] != m_wSearchGen )
		return( NULL );

	CCell * pCellFind = &m_paCells[ m_pwCellSlot[i] ];

#ifdef TEST_RESULT1

	CCell *pNewCell = &m_paCells[0];
	for( int j=0; j<m_iNextSlot; ++j, pNewCell++ )
	{
		if( pNewCell->m_iX == iX && pNewCell->m_iY == iY )
			{
#ifdef TEST_RESULT2
			TRAP ( pCellFind != pNewCell );
#endif
			goto found;
			}
	}
#ifdef TEST_RESULT2
	TRAP ( pCellFind != NULL );
#endif
found:
#endif

	return ( pCellFind );
}


int CPathMap::GetOffset( int iX, int iY )
{
	if( iX > m_iMapEX ||
		iY > m_iMapEY )
		return( m_iNumOfMapCells );

	int ix = iX - m_iBaseX;
	int iy = iY - m_iBaseY;
	int j = (m_iWidth * iy) + ix;
	return( j );
}

// ROAD AVOIDANCE (plan-time): TRUE if any of the 8 neighbors of iX,iY carries an
// own farm/lumber building. AI-map high byte = GetBldgType (caimaput.cpp
// ConvertStatus), MSW_AI_BUILDING gates own buildings. Cheap: 8 array reads,
// fast-fail on the MSW_AI_BUILDING bit; only called for road-planning nodes.
BOOL CPathMap::FarmLumberAdjacent( int iX, int iY )
{
	if( m_pMap == NULL )
		return( FALSE );

	for( int dy = -1; dy <= 1; ++dy )
	{
		int ny = iY + dy;
		if( ny < m_iBaseY || ny > m_iMapEY )
			continue;
		for( int dx = -1; dx <= 1; ++dx )
		{
			if( !dx && !dy )
				continue;
			int nx = iX + dx;
			if( nx < m_iBaseX || nx > m_iMapEX )
				continue;
			int i = GetOffset( nx, ny );
			if( i < 0 || i >= m_iNumOfMapCells )
				continue;
			WORD w = m_pMap[i];
			if( !( w & MSW_AI_BUILDING ) )
				continue;
			int iType = w >> 8;	// base type via GetBldgType, per ConvertStatus
			if( iType == CStructureData::farm ||
				iType == CStructureData::lumber )
				return( TRUE );
		}
	}
	return( FALSE );
}

// (was: identical implementation to CPathMgr::AddCellToArray — now gen-grid)
CCell * CPathMap::AddCellToArray( CCell *pCell )
{

	// war planning + war roads may use the full map-sized allocation;
	// everything else keeps the original arena cap
	int iCap = ( m_bWarPlanning || m_bWarRoad ) ? m_iCellAlloc : m_iNumOfCells;
	if( m_iNextSlot >= iCap )
		return (NULL) ;

	CCell *pNewCell = &m_paCells[m_iNextSlot];
	pNewCell->m_iX = pCell->m_iX;
	pNewCell->m_iY = pCell->m_iY;
	pNewCell->m_iCost = pCell->m_iCost;
	pNewCell->m_iDist = pCell->m_iDist;
	pNewCell->m_iBoth = pCell->m_iBoth;
	pNewCell->m_pCellFrom = pCell->m_pCellFrom;
	// recycled slots are no longer pre-cleared by ClearArray — reset the
	// intrusive-list state BEFORE NewBoth reads m_iBothIn/m_pcPrevBoth
	pNewCell->m_pCellNext = NULL;
	pNewCell->m_pcNextBoth = NULL;
	pNewCell->m_pcPrevBoth = NULL;
	pNewCell->m_iBothIn = 0;
	pNewCell->m_bClosed = 0;
	NewBoth ( pNewCell );

	// register in the coord->cell grid for this search
	if( pCell->m_iX >= 0 && pCell->m_iY >= 0 )
	{
		int i = ( m_iWidth * pCell->m_iY ) + pCell->m_iX;
		if( i >= 0 && i < m_iNumOfMapCells )
		{
			m_pwCellGen[i] = m_wSearchGen;
			m_pwCellSlot[i] = (WORD)m_iNextSlot;
		}
	}
	m_iNextSlot++;

	return pNewCell;
}

//
// clear the contents of the array
//
void CPathMap::ClearArray( void )
{
	// Cells are invalidated wholesale by bumping the search generation —
	// the old per-cell reinit loop and the hash-map RemoveAll are gone
	// (recycled slots are fully re-initialized in AddCellToArray instead).
	NewSearchGen( );

	m_iLowestBoth = 0;
	memset ( m_acBoth , 0, sizeof (m_acBoth) );
}

//
// start a new search generation: O(1) "clear" of the coord->cell lookup.
// On WORD wrap (every 65535 searches) do one real grid clear so stamps
// from 65535 searches ago can't read as current.
//
void CPathMap::NewSearchGen( void )
{
	if ( ++m_wSearchGen == 0 )
	{
		if ( m_pwCellGen != NULL )
			memset( m_pwCellGen, 0, m_iNumOfMapCells * sizeof( WORD ) );
		m_wSearchGen = 1;
	}
}

CHexCoord *CPathMap::CreateHexPath( int& iPathLen, CCell *pDestCell )
{
	if( pDestCell == NULL )
		return( NULL );

	// now count the number of cells in the path, by
	// following the CCell::m_pCellFrom to the next cell
	int i = GetPathCount( pDestCell );
	if( !i )
		return( NULL );

	// create a hex path array that size to return
	CHexCoord *pHexPath = NULL;
 	pHexPath = new CHexCoord[i];
	iPathLen = i;


	// now go thru path, reversing step order
	CCell *pNext, *pThis;
	pThis = pDestCell;
	while( TRUE )
	{
		if( pThis == NULL )
			break;

		CHexCoord *pHex = &pHexPath[--i];
		pHex->X( pThis->m_iX );
		pHex->Y( pThis->m_iY );

		// at the end
		if( !i )
			break;

		// move to next cell in path
		pNext = pThis->m_pCellFrom;
		pThis = pNext;
	}
	
	
#if DEBUG_OUTPUT_PATH
	for( i=0; i<iPathLen; ++i )
	{
		CHexCoord *pHex = &pHexPath[i];
#ifdef _LOGOUT
		logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"path step %d is %d,%d", i, pHex->X(), pHex->Y() );
#endif
	}
#endif

#if DEBUG_OUTPUT_PATH
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"total path steps %d \n", iPathLen );
#endif
#endif

	return( pHexPath );
}

//
// count the number of steps in the completed best path
//
int CPathMap::GetPathCount( CCell *pDestCell )
{
	// now go thru path, reversing step order
	int iCnt = 0;
	CCell *pNext, *pThis;
	pThis = pDestCell;
	while( TRUE )
	{
		if( pThis->m_pCellFrom == NULL )
			break;

		// move to next cell in path
		pNext = pThis->m_pCellFrom;
		pThis = pNext;
		iCnt++;
	}
	return( iCnt );
}


////////////////////////////////////////////////////////////////////////
//
// separate initialization process
//
BOOL CPathMap::Init( int iMapEX, int iMapEY )
{
	// thePathMap is a single GLOBAL object shared by every AI player's worker
	// thread; GetPath()/GetRoadPath() serialize through m_cs. Init() can be
	// called DURING gameplay (ai.cpp/caidata.cpp, on map resize/expansion) while
	// other AI threads are mid-path. The original code tore down the shared
	// state — delete[] m_paCells, m_mapCell.RemoveAll(), and even
	// DeleteCriticalSection(&m_cs)/InitializeCriticalSection — with NO lock held.
	// That frees the std::map nodes and cell array another thread is walking, and
	// destroys the very lock another thread is inside, producing red-black-tree
	// heap corruption (STATUS_HEAP_CORRUPTION 0xc0000374, seen via AI combat
	// pathfinding SeekOpfor->FindDefenseHex->GetPath). Hold m_cs across the
	// rebuild, and do NOT destroy/recreate the lock here (it is created once in
	// the constructor and destroyed in ~CPathMap/Close).
	//
	// Note: m_cs is created in the constructor, so it is valid on entry here even
	// on the first Init().
	EnterCriticalSection( &m_cs );

	m_iWidth = iMapEX;
	m_iHeight = iMapEY;

	m_iNumOfCells = (m_iWidth + m_iHeight) * 4; // m_iWidth * m_iHeight;
	m_iNumOfMapCells = m_iWidth * m_iHeight;
	m_iNextSlot = 0;
	memset ( m_acBoth , 0, sizeof (m_acBoth) );
	m_iLowestBoth = 0;
	m_iFirst = m_iNumOfCells-1;
	m_iLast = 0;

	if( m_paCells != NULL )
		delete [] m_paCells;

	// slot indices live in a WORD grid — clamp the arena if a giant map ever
	// pushes (W+H)*4 past what a WORD can index (none of our maps do)
	TRAP( m_iNumOfCells > 0xFFFF );
	if( m_iNumOfCells > 0xFFFF )
		m_iNumOfCells = 0xFFFF;

	// war-planning arena sized to the MAP, not the perimeter: perimeter*8
	// (=16384 on 1024 maps) was exhausted by EVERY cross-map reachability
	// search - soak52: 917/917 [WARPATH] FAILs were arena-full, ZERO honest
	// open-empty, so all long-range war/sea targets read false-unreachable.
	// WORD slot grid caps it at 0xFFFF. Every non-war search keeps the old
	// m_iNumOfCells cap in AddCellToArray - economy pathing unchanged
	m_iCellAlloc = m_iNumOfMapCells;
	if( m_iCellAlloc > 0xFFFF )
		m_iCellAlloc = 0xFFFF;
	if( m_iCellAlloc < m_iNumOfCells * 2 )
		m_iCellAlloc = m_iNumOfCells * 2 > 0xFFFF ? 0xFFFF : m_iNumOfCells * 2;

	m_paCells = new CCell[m_iCellAlloc];

	// (re)build the coord->cell lookup grids; gen 0 == "never touched"
	delete [] m_pwCellGen;
	delete [] m_pwCellSlot;
	m_pwCellGen  = new WORD[m_iNumOfMapCells];
	m_pwCellSlot = new WORD[m_iNumOfMapCells];
	memset( m_pwCellGen, 0, m_iNumOfMapCells * sizeof( WORD ) );
	m_wSearchGen = 0;

	m_tdWheel = theTransports.GetData( CTransportData::construction );
	m_tdTrack = theTransports.GetData( CTransportData::infantry_carrier );
	m_tdHover = theTransports.GetData( CTransportData::light_scout );
	m_tdFoot  = theTransports.GetData( CTransportData::infantry );

	m_iNextSlot = m_iFirst;
	ClearArray();
	m_iNextSlot = 0;

#if PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH,
		"\nCPathMap::Init() for %d,%d  m_iNumOfCells=%d  m_iNumOfMapCells=%d ",
		iMapEX, iMapEX, m_iNumOfCells, m_iNumOfMapCells );
#endif
#endif

	LeaveCriticalSection( &m_cs );

	return TRUE;
}

//
// constructor does not initialize
//
CPathMap::CPathMap( void )
{
	m_iWidth = 0;
	m_iHeight = 0;
	m_iMapEX = 0;
	m_iMapEY = 0;
	m_iDistFactor = 0;
	m_paCells = NULL;
	m_iCellAlloc = 0;
	m_bRoadPlanning = FALSE;
	m_bWarPlanning = FALSE;
	m_bWarRoad = FALSE;
	m_bOverWater = FALSE;
	m_tdWheel = NULL;
	m_tdTrack = NULL;
	m_tdHover = NULL;
	m_tdFoot  = NULL;
	m_pTD = NULL;

	m_pwCellGen = NULL;
	m_pwCellSlot = NULL;
	m_wSearchGen = 0;

	memset ( m_acBoth , 0, sizeof (m_acBoth) );
	m_iLowestBoth = 0;

	// Create the lock here, ONCE, so Init() (which may run during gameplay while
	// AI threads are pathing) never has to create/destroy it. m_bCsInited tracks
	// ownership so the destructor/Close() delete it exactly once.
	memset( &m_cs, 0, sizeof( m_cs ) );
	InitializeCriticalSection( &m_cs );
	m_bCsInited = TRUE;
}

CPathMap::~CPathMap()
{
	if ( m_bCsInited )
	{
		DeleteCriticalSection(&m_cs);
		m_bCsInited = FALSE;
	}

	delete [] m_paCells;
	delete [] m_pwCellGen;
	delete [] m_pwCellSlot;
}

void CPathMap::Close ()
{
	delete [] m_paCells;
	m_paCells = NULL;
	delete [] m_pwCellGen;
	m_pwCellGen = NULL;
	delete [] m_pwCellSlot;
	m_pwCellSlot = NULL;

	if ( m_bCsInited )
	{
		DeleteCriticalSection(&m_cs);
		m_bCsInited = FALSE;
	}
}

// end of CPathMap.cpp

