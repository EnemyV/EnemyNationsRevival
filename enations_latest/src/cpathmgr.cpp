////////////////////////////////////////////////////////////////////////////
//
//  CPathMgr.cpp : CPathMgr object implementation
//                 Divide and Conquer
//               
//  Last update:   10/29/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "STDAFX.H"
#include "CPathMgr.h"

#include "logging.h"	// dave's logging system

//#define TEST_RESULT1		// test GetAt improvement
//#define TEST_RESULT2			// test GetLowest improvement
//#define TEST_RESULT3			// anal level of testing

// BUGBUG these are here for diagnostics only
#include "CAIHex.hpp"
#include "CAIData.hpp"

extern CAIData *pGameData;		// pointer to API object for game data


CPathMgr thePathMgr;

#define new DEBUG_NEW
#define MAX_PATH_RANGE	80

// lookup table of bit values that represent headings
// that are valid for a given heading (offset to table)
// used by BOOL CPathMgr::IsValidHeading( 

// this table allows the current heading of the vehicle
// plus/minus 1 to each side
static unsigned char ucHeadings[] = {
	131, // 10000011
	7,   // 00000111
	14,  // 00001110
	28,  // 00011100
	56,  // 00111000
	112, // 01110000
	224, // 11100000
	193  // 11000001
};

/*
// this table allows the current heading of the vehicle
// plus/minus 2 to each side
static unsigned char ucHeadings[] = {
	199, // 11000111
	143, // 10001111
	31,  // 00011111
	62,  // 00111110
	124, // 01111100
	248, // 11111000
	241, // 11110001
	227  // 11100011
};
*/

//
// return the path via a CHexCoord array, passing the size
// back as the m_iX element of the first CHexCoord
//
CHexCoord *CPathMgr::GetPath( CVehicle *pVehicle,
	CHexCoord& hexFrom, CHexCoord& hexTo, 
	int& iPathLen, int iVehType, BOOL bVehBlock, BOOL bDirectPath )
{
#if PATH_TIMING
#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif
#endif

	// BUGBUG count types of calls
	m_iPaths++;
	if( pVehicle == NULL )
		m_iHP++;
	if( hexFrom.X() == hexTo.X() ||
		hexFrom.Y() == hexTo.Y() )
		m_iOrtho++;

	// BUGBUG initialization of iPathLen should be done outside
	// by the calling function of GetPath()
	iPathLen = 0;

	// prevent invalid CHexCoords from continuing
	if( hexFrom.X() < 0 || hexFrom.X() > m_iMapEX ||
		hexFrom.Y() < 0 || hexFrom.Y() > m_iMapEY ||
		hexTo.X() < 0 || hexTo.X() > m_iMapEX ||
		hexTo.Y() < 0 || hexTo.Y() > m_iMapEY )
	{
#if PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"\nFindPath from %d,%d to %d,%d has invalid CHexCoord \n", 
		hexFrom.X(), hexFrom.Y(), hexTo.X(), hexTo.Y() );
#endif
#endif
		return( NULL );
	}

	// set best cost threshhold so its very high
	// and it must match what is tested in GetLowestCost()
	m_iBestCost = 0xFFFE; //(m_iWidth * m_iHeight);

	// change to reflect new approach
	m_iFirst = 0;
	m_iLast = 0;
	m_iNextSlot = 0;
	m_iLowestBoth = 0;

	// no path from start to destination because we are there
	if( hexFrom == hexTo )
		return( NULL );

	m_hexFrom = hexFrom;
	m_hexTo = hexTo;
	m_bVehBlock = bVehBlock;
	// force vehicles to block at all times, to reduce jams
	//
	// no, because the client wants to ignore my advice, again,
	// and thus, let the jams begin ....
	// m_bVehBlock = TRUE;

	// get CTransportData pointer for the unit moving
	if( pVehicle != NULL )
		m_pTD = pVehicle->GetData();
	else
		m_pTD = theTransports.GetData(iVehType);

	// determine maximum cost of a single move based on 
	// this wheel type and use that as a factor with distance
	// to destination from current test cell.
	m_iDistFactor = m_iBestCost;
	//m_iDistFactor = 0;
	
	for( int t=CHex::city; t<CHex::num_types; ++t )
	{
		int iRtn = 
			theTerrain.GetData(t).GetWheelMult(m_pTD->GetWheelType());
		//iRtn *= 2;
		iRtn <<= 1;

		if( iRtn && iRtn < m_iDistFactor )
			m_iDistFactor = iRtn;
	}
	
	// compensate for lack of differences in terrain costs over distance
	if( m_iDistFactor == m_iBestCost )
		m_iDistFactor = 2;
	//m_iDistFactor >>= 1;

	// determine the maximum distance a path should take
	m_iMaxDist = (abs( hexFrom.X() - hexTo.X() ) + 
				  abs( hexFrom.Y() - hexTo.Y() )) * m_iDistFactor;

#if PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"\nFindPath from %d,%d to %d,%d wheeltype=%d ", 
		m_hexFrom.X(), m_hexFrom.Y(), 
		m_hexTo.X(), m_hexTo.Y(), m_pTD->GetWheelType() );
	if( pVehicle != NULL )
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"for vehicle id %ld of player %d ", 
		pVehicle->GetID(), pVehicle->GetOwner()->GetPlyrNum() );
#endif
#endif

	// consider the range to the destination and make adjustment
	// if range is > m_iMaxPath
	if( !bDirectPath )
		AdjustDestination();


	// set up hexFrom as first test cell
	CCell *pLastTest = NULL;
	CCell *pTest = NULL;
	CCell *pDestCell = NULL;
	CCell ccTest( m_hexFrom.X(), m_hexFrom.Y() );

	// set special state for hexFrom
	ccTest.m_iCost = 0;
	ccTest.m_iDist = 0;
	ccTest.m_iBoth = 0;

	// used for array	
	pTest = AddCellToArray( &ccTest );
#ifdef TEST_RESULT2
	TRAP ( pTest != GetCellAt(ccTest.m_iX,ccTest.m_iY) );
#endif
	CCell *pAdjCell = NULL;

	// if a vehicle exists, then determine its current heading
	// and record the appropriate cell in pTest->m_pFromCell
	int iAdjCells = 0;
#if USE_HEADINGS
	if( pVehicle != NULL &&
		!(m_pTD->GetVehFlags () & CTransportData::FL1hex) )
	{
		GetFromCell( pVehicle, pTest );
		iAdjCells = HEADINGS;
	}
	else
#endif
		iAdjCells = CELLSAROUND;
	

	int iX,iY,iCnt;

	// BUGBUG hard code limit on pathing
	int iHang = (m_iWidth + m_iHeight); // 128
	if( bDirectPath )
		iHang *= 2; // 256
	else
		iHang += (theMap.GetRangeDistance(m_hexFrom, m_hexTo) * CELLSAROUND);

	int iTicks = 0;
	int iList = 1;

	// start looping to find a path
	while( TRUE )
	{
		if( AtDestination(pTest) &&
			pTest->m_iBoth == m_iBestCost )
		{
			break;
		}

		// for each adjacent cell to the test cell
		for( int i=0; i<iAdjCells; ++i )
		{
			// enact a heading criteria
			//if( !IsValidHeading(i,pTest) )
			//	continue;

			// get the adjacent cell x,y values
			// in the 'i' direction from pTest
#if USE_HEADINGS
			if( iAdjCells == HEADINGS )
				GetHeadingCell( i, pTest, iX, iY );
			else
#endif
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

			// the AddCellToArray() could fail if array has been exceeded
			if( pAdjCell == NULL )
			{
				iHang = 1; // cause early termination
				break;
			}

			// now get cost to enter pAdjCell from pTest,
			// and distance from pAdjCell to destination,
			// and make pAdjCell point to pTest
			GetCellCosts( i, pTest, pAdjCell );

			if( AtDestination(pAdjCell) )
			{
				pDestCell = pAdjCell;
#if PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"at dest with pAdjCell %d,%d  cost=%d  dist=%d  best=%d",
		pAdjCell->m_iX, pAdjCell->m_iY,
		pAdjCell->m_iCost, pAdjCell->m_iDist, m_iBestCost );
#endif
#endif
				if( (pAdjCell->m_iCost + pAdjCell->m_iDist)
					 < m_iBestCost )
				{
					if ( ( m_iBestCost == 0xFFFE ) && 
									( (pAdjCell->m_iCost + pAdjCell->m_iDist) != 0xFFFE ) )
						{
						m_iBestCost = (pAdjCell->m_iCost + pAdjCell->m_iDist);
						int iEnd = __min ( m_iNextSlot, m_iNumOfCells );
						CCell *pCell = m_paCells;
						while ( iEnd -- )
							NewBoth ( pCell ++ );
						}
					else
						m_iBestCost = (pAdjCell->m_iCost + pAdjCell->m_iDist);

					// consider only a direct path is needed
					// BUGBUG force direct paths for a while
					if( bDirectPath )
					{
						CHexCoord *phexPath = 
							CreateHexPath( iPathLen, pAdjCell );

						ClearArray();
#if PATH_TIMING
	dwEnd = timeGetTime();
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() direct for a %s took %ld ticks for %d steps ", 
		(const char *)m_pTD->GetDesc(), (dwEnd - dwStart), iPathLen );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"Took %d interations with %d cells in list \n", 
		iTicks, iList );
	if( phexPath == NULL )
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, "GetPath() direct NULL path returned\n" );
#endif
	m_iPathTicks += (int)(dwEnd - dwStart); // sum of ticks in paths
	m_iStepCnt += iPathLen;   // count of steps in paths
#endif

						return( phexPath );
					}
					else
					{
						// less than 1/2 the time remaining to search
						if( iHang < m_iWidth )
						{
						CHexCoord *phexPath = 
							CreateHexPath( iPathLen, pAdjCell );

						//ReportPath(phexPath,iPathLen);

						ClearArray();
#if PATH_TIMING
	dwEnd = timeGetTime();
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() early end for a %s took %ld ticks for %d steps ", 
		(const char *)m_pTD->GetDesc(), (dwEnd - dwStart), iPathLen );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"Took %d interations with %d cells in list \n", 
		iTicks, iList );
	if( phexPath == NULL )
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() early end NULL path returned\n" );
#endif
	m_iPathTicks += (int)(dwEnd - dwStart); // sum of ticks in paths
	m_iStepCnt += iPathLen;   // count of steps in paths
#endif

						return( phexPath );
						}
					}
				}
				else // cost is not the best
				{
					// pAdjCell could be being entered from a bridge
					if( !CanEnterBridge( pTest, pAdjCell ) )
						continue;

					// no entry possible into destination cell
					if( pAdjCell->m_iDist == 0xFFFE )
					{
						// return path based on last reachable cell
						CHexCoord *phexPath = 
							CreateHexPath( iPathLen, pTest );

#if PATH_TIMING
						m_hexTo.X( pTest->m_iX );
						m_hexTo.Y( pTest->m_iY );
#ifdef _LOGOUT
						ReportPath(phexPath,iPathLen);
#endif
#endif // PATH_TIMING

						ClearArray();
#if PATH_TIMING
	dwEnd = timeGetTime();
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() blocked for a %s took %ld ticks for %d steps ", 
		(const char *)m_pTD->GetDesc(), (dwEnd - dwStart), iPathLen );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"returning path to %d,%d instead of destination ",
		m_hexTo.X(), m_hexTo.Y() );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"Took %d interations with %d cells in list \n", 
		iTicks, iList );
#endif
	m_iPathTicks += (int)(dwEnd - dwStart); // sum of ticks in paths
	m_iStepCnt += iPathLen;   // count of steps in paths
#endif // PATH_TIMING
						// BUGBUG zero iTicks means this is an adjacent
						// cell that is both the destination and blocked
						if( !iTicks )
						{
							if( phexPath != NULL )
								delete [] phexPath;
							phexPath = NULL;
							iPathLen = 0;
						}
#if PATH_TIMING
#ifdef _LOGOUT
	if( phexPath == NULL )
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() blocked a NULL path returned " );
#endif
#endif // PATH_TIMING

						return( phexPath );
					}
				}
			}
		}

		// flag pTest so that it does not get re-picked
		pTest->m_iBoth = 0;
		NewBoth ( pTest );

		// get lowest combined cost cell in list to repeat
		pLastTest = pTest;
		pTest = GetLowestCost(iCnt);
		if( pTest == NULL )
		{
			if( !bDirectPath )
				pTest = GetClosestCell();

			break;
		}

		// consider we have reached the destination
		if( pTest->m_iBoth == m_iBestCost &&
			iCnt == 1 )
		{
			TRAP ( m_iBestCost == 0xFFFE );	// BUGBUG - problem on computing iCnt
			if( AtDestination(pTest) )
			{
				break;
			}
		}

		// put this in to prevent hangs
		iHang--;
		if( !iHang )
		{

#if PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"broke on HANG protection " );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"pTest is at %d,%d  cost=%d  dist=%d  both=%d ",
		pTest->m_iX, pTest->m_iY, pTest->m_iCost, pTest->m_iDist, pTest->m_iBoth );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"Took %d interations with %d cells in list \n", 
		iTicks, iList );
#endif
#endif

#if PATH_TIMING
			// what is the destination hex like?
			if( pGameData != NULL && m_pTD != NULL )
			{
				CHex *pGameHex = theMap.GetHex( m_hexTo );
#ifdef _LOGOUT
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"to hex %d,%d  CanTravelHex()=%d  CHex::GetUnits()=%d  CHex::GetType()=%d  CHex::GetAlt()=%d ",
		m_hexTo.X(), m_hexTo.Y(),
		(int)m_pTD->CanTravelHex( pGameHex ), (int)pGameHex->GetUnits(), 
		(int)pGameHex->GetType(), (int)pGameHex->GetAlt() );
#endif
			}
#endif  // PATH_TIMING

			if( !bDirectPath )
			{
				pTest = GetClosestCell();
#if PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"selected GetClosestCell() of %d,%d at %d \n", 
		pTest->m_iX, pTest->m_iY, pTest->m_iDist );
#endif
#endif
			}
			else
				pTest = NULL;
			break;
		}

		iTicks++;

	}

	if( pTest == NULL )
	{
#if PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"End of GetPath() NULL path returned " );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"Took %d interations with %d cells in list \n", 
		iTicks, iList );
#endif
#endif
		ClearArray();
		return( NULL );
	}

	// a break from trying has occurred and the dest was reached
	if( !iHang && pDestCell != NULL )
		pTest = pDestCell;

	// now walk the m_plCells and
	// create a CHexCoord[] to return to caller
	CHexCoord *phexPath = CreateHexPath( iPathLen, pTest );

#if PATH_TIMING
#ifdef _LOGOUT
	if( !iHang || iPathLen == 1 )
		// || m_pTD->GetType() == CTransportData::light_scout )
		ReportPath(phexPath,iPathLen);
#endif
#endif // PATH_TIMING

	// remove cells used in pathfinding
	ClearArray();
	
#if PATH_TIMING
	dwEnd = timeGetTime();
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() for a %s took %ld ticks for %d steps ", 
		(const char *)m_pTD->GetDesc(), (dwEnd - dwStart), iPathLen );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"from %d,%d to %d,%d ", m_hexFrom.X(), m_hexFrom.Y(),
		m_hexTo.X(), m_hexTo.Y() );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"Took %d interations with %d cells in list \n", 
		iTicks, iList );
#endif
	if( iHang )
	{
	m_iPathTicks += (int)(dwEnd - dwStart); // sum of ticks in paths
	m_iStepCnt += iPathLen;   // count of steps in paths
	}
	else
	{
	m_iHangTicks += (int)(dwEnd - dwStart); // sum of ticks in hangs
	m_iHangCnt += iPathLen;   // count of hang steps
	}
#endif

	// this may be a case of a 1 step path with the 
	// destination occupied, so do one last check
	// on the single step and return NULL if blocked
	if( /*!iHang || */ iPathLen == 1 )
	{
		CHexCoord *pHex = &phexPath[0];
		CHexCoord hexDest( pHex->X(), pHex->Y() );
		CHex *pGameHex = theMap.GetHex( hexDest );
		if( pGameHex != NULL )
		{
			// consider that a vehicle occupies the dest hex or
			// it cannot be entered
			BYTE bUnits = pGameHex->GetUnits();
			if( (bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr)) ||
				!m_pTD->CanTravelHex( pGameHex ) )
			{
				delete [] phexPath;
				phexPath = NULL;
				iPathLen = 0;
			}
		}
	}

#if PATH_TIMING
#ifdef _LOGOUT
	if( phexPath == NULL )
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, "last NULL path returned " );
#endif
#endif

	return( phexPath );
}

//
// while the range to the destination is greater than what is
// acceptable, make an adjustment by finding the mid point between
// m_hexFrom and the current m_hexTo and put the just found mid
// point into m_hexTo in lieu of the original destination
//
void CPathMgr::AdjustDestination( void )
{
	int iRange = theMap.GetRangeDistance( m_hexFrom, m_hexTo );
	if( iRange <= m_iMaxPath )
		return;

	while( iRange > m_iMaxPath )
	{
		ChangeDestination();

		// get new range from the game
		iRange = theMap.GetRangeDistance( m_hexFrom, m_hexTo );
		if( iRange < 0 )
		{
#if PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"CPathMgr::range problem from %d,%d to %d,%d   range=%d ",
		m_hexFrom.X(), m_hexFrom.Y(), m_hexTo.X(), m_hexTo.Y(), iRange );
#endif
#endif
			iRange = m_iMaxPath;
			continue;
		}

		// make sure we can test with CTransportData
		if( m_pTD == NULL )
			continue;

		// make sure new destination is passible
		CHex *pGameHex = theMap.GetHex( m_hexTo );
		if( pGameHex == NULL )
			continue;
		// new destination is not passible so stay in the loop
		if( !m_pTD->CanTravelHex( pGameHex ) )
			iRange = m_iMaxPath;
	}

#if 0 //PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"CPathMgr::AdjustDestination() from %d,%d to %d,%d   range=%d ",
		m_hexFrom.X(), m_hexFrom.Y(), m_hexTo.X(), m_hexTo.Y(), iRange );
#endif
#endif
}
//
// find the mid point between 
// m_hexFrom and the current m_hexTo and put the just found mid
// point into m_hexTo in lieu of the original destination
//
void CPathMgr::ChangeDestination( void )
{
	int iNewX, iNewY;

	// get 1/2 the distance on each axis
	int iDeltaX = abs( m_hexFrom.Diff(m_hexFrom.X() - m_hexTo.X()) ) / 2;
	int iDeltaY = abs( m_hexFrom.Diff(m_hexFrom.Y() - m_hexTo.Y()) ) / 2;

	if( m_hexFrom.Diff(m_hexFrom.X() - m_hexTo.X()) < 0 )
		iNewX = m_hexFrom.Wrap( (m_hexFrom.X() + iDeltaX) );
	else
		iNewX = m_hexFrom.Wrap( (m_hexFrom.X() - iDeltaX) );

	if( m_hexFrom.Diff(m_hexFrom.Y() - m_hexTo.Y()) < 0 )
		iNewY = m_hexFrom.Wrap( (m_hexFrom.Y() + iDeltaY) );
	else
		iNewY = m_hexFrom.Wrap( (m_hexFrom.Y() - iDeltaY) );

	if( !iNewX && !iNewY )
	{
		iNewX = 1;
	}
	m_hexTo.X(iNewX);
	m_hexTo.Y(iNewY);

#if PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() adjusted destination to %d,%d ",
		m_hexTo.X(), m_hexTo.Y() );
#endif
#endif
}

#if 0 //_LOGOUT
// BUGBUG this is for solving the repeating path problem
void CPathMgr::ReportPath( CHexCoord *phexPath, int iPathLen )
{
	if( m_pTD == NULL )
		return;

	if( phexPath == NULL )
	{
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() path returned is NULL " );
		return;
	}

	CHex *pGameHex = theMap.GetHex( m_hexFrom );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() from-hex is occupied with %d ",
		(int)pGameHex->GetUnits() );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() path returned is " );

	for( int i=0; i<iPathLen; ++i )
	{
		CHexCoord *pHex = &phexPath[i];
		
		CHexCoord hexDest( pHex->X(), pHex->Y() );
		pGameHex = theMap.GetHex( hexDest );
		if( pGameHex == NULL )
			return;

		CCell *pCell = GetCellAt(pHex->X(), pHex->Y());

	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"path step %d is %d,%d ", i, pHex->X(), pHex->Y() );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		" CanTravelHex()=%d  CHex::GetUnits()=%d  CHex::GetType()=%d  CHex::GetAlt()=%d ",
		(int)m_pTD->CanTravelHex( pGameHex ), (int)pGameHex->GetUnits(), 
		(int)pGameHex->GetType(), (int)pGameHex->GetAlt() );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		" dist %d   cost %d   both %d ", 
		pCell->m_iDist, pCell->m_iCost, pCell->m_iBoth );

	}
}
#endif

//
// create arrays of appropriate objects, based on whether in use
// by the game or using fake map for testing, and return the pointer
// to the array, with the size of the array in iPathLen
//
CHexCoord *CPathMgr::CreateHexPath( int& iPathLen, CCell *pDestCell )
{
	if( pDestCell == NULL )
		return( NULL );

#if USE_HEADINGS
	// first NULL out m_pCellFrom for the CCell representing m_hexFrom
	// which may have been set in order to find heading for m_hexFrom
	CCell *pFromCell = GetCellAt( m_hexFrom.X(),m_hexFrom.Y() );
	pFromCell->m_pCellFrom = NULL;
#endif

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
		logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"path step %d is %d,%d ", i, pHex->X(), pHex->Y() );
#endif
	}
#endif

#if DEBUG_OUTPUT_PATH
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"total path steps %d ", iPathLen );
#endif
#endif

	return( pHexPath );
}

//
// count the number of steps in the completed best path
//
int CPathMgr::GetPathCount( CCell *pDestCell )
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

//
// consider if the cell passed is the destination cell
//
BOOL CPathMgr::AtDestination( CCell *pCell )
{
	if( pCell->m_iX == m_hexTo.X() &&
		pCell->m_iY == m_hexTo.Y() )
		return TRUE;

	return FALSE;
}

//
// test for the destination being entered from a bridge
//
BOOL CPathMgr::CanEnterBridge( CCell *pFromCell, CCell *pToCell )
{
	// need this because multiple threads call thePathMgr and it is global
	if( m_pTD == NULL )
		return FALSE;

	// get game data for this cell
	CHexCoord hexDest( pToCell->m_iX, pToCell->m_iY );
	CHex *pDestHex = theMap.GetHex( hexDest );
	if( pDestHex == NULL )
		return FALSE;

	// create hex for from cell
	CHexCoord hexFrom( pFromCell->m_iX, pFromCell->m_iY );
	CHex *pFromHex = theMap.GetHex( hexFrom );
	if( pFromHex == NULL )
		return FALSE;

	// if there is bridge hex involved, we must do another test
	if( pDestHex->GetUnits() & CHex::bridge 
		|| pFromHex->GetUnits() & CHex::bridge )
	{
	// we don't know the unit here so we figure if the from hex is water & not a bridge
	// it is on the water, otherwise it is land/bridge.
	// this does stop moving along a bridge under it
	BOOL bOnWater = pFromHex->IsWater() & ((pFromHex->GetUnits() & CHex::bridge) == 0);
	if( !m_pTD->CanEnterHex(hexFrom, hexDest, bOnWater) )
			return FALSE;
		//if( !m_pTD->CanEnterHex(hexFrom, hexDest, pFromHex->IsWater()) )
		//	return FALSE;
	}
	return TRUE;
}

//
// get cost to enter 'from' cell into 'to' cell, and if less than
// the current cost in 'to' then record cost, and make 'to' cell
// point to 'from' cell also get range to destination for 'to' cell
// and record distance and combine the two into 'to' both
//
void CPathMgr::GetCellCosts( int iPos, CCell *pFromCell, CCell *pToCell )
{
	// this cell is the start cell and should not be costed
	if( !pToCell->m_iCost )
		return;
	// hex at 0,0 is never passable
	if( !pToCell->m_iX && !pToCell->m_iY )
		return;

	// need this because multiple threads call thePathMgr and it is global
	if( m_pTD == NULL )
		return;

	// get game data for this cell
	CHexCoord hexDest( pToCell->m_iX, pToCell->m_iY );
	CHex *pDestHex = theMap.GetHex( hexDest );
	if( pDestHex == NULL )
		return;

	// test to enter hex
	if( !m_pTD->CanTravelHex( pDestHex ) )
		return;

	// since DT will not change the terrain cost data for coastlines
	// to make them more expensive to travel, we will do the same
	// thing in code, which slows down the pathing process
	//
	// if terrain coastline 
	// then let's skip it
	// unless its the dest
	if(	pDestHex->GetType() == CHex::coastline )
	{
		// except of course, inf/outriders can travel coastline anytime
		if( m_pTD->GetWheelType() != CWheelTypes::walk &&
			m_pTD->GetWheelType() != CWheelTypes::hover &&
			hexDest != m_hexTo )
		{
			// but any vehicle on a bridge
			if( !(pDestHex->GetUnits() & CHex::bridge) )
				return;
		}
	}
	// BRIDGEBUG will need to create (CHex) pFromHex
	// and test for pFromHex->GetUnits() & CHex::bridge and
	// if TRUE then use m_pTD->CanEnterHex( hexFrom, hexDest )
	// to determine if the hex can be entered and then not use
	// the m_pTD->CanTravelHex( pDestHex ) 

	// create hex for from cell
	CHexCoord hexFrom( pFromCell->m_iX, pFromCell->m_iY );
	CHex *pFromHex = theMap.GetHex( hexFrom );
	if( pFromHex == NULL )
		return;

	// if there is bridge hex involved, we must do another test
	if( pDestHex->GetUnits() & CHex::bridge 
		|| pFromHex->GetUnits() & CHex::bridge )
	{
	// we don't know the unit here so we figure if the from hex is water & not a bridge
	// it is on the water, otherwise it is land/bridge.
	// this does stop moving along a bridge under it
	BOOL bOnWater = pFromHex->IsWater() & ((pFromHex->GetUnits() & CHex::bridge) == 0);
	if( !m_pTD->CanEnterHex(hexFrom, hexDest, bOnWater) )
			return;
		//if( !m_pTD->CanEnterHex(hexFrom, hexDest, pFromHex->IsWater()) )
		//	return;
	}

	// get cost from the game
	// int CGameMap::GetTerrainCost (CHexCoord const & hex, 
	// CHexCoord const & hexNext, int iDir, int iWheel)
	int iCost = theMap.GetTerrainCost( hexFrom, hexDest, 
		iPos, m_pTD->GetWheelType() );

	// do not allow ZERO cost terrain to proceed
	if( !iCost )
		return;

	// since DT will not change the terrain cost data to solve the 
	// problem in using roads, then we must to do it with code, 
	// so we need to multiply the cost of all non-road terrain
	// to increase the diff, which of course, slows down the process
	// of finding a path.  Oh, well I did try to tell him, but as
	// usual he would not listen.
	if(	pDestHex->GetType() != CHex::road )
		iCost <<= 1;

	// if this cost + cost to this point < what we already have
	// then save the value and the pointer to where we came from
	if( (iCost + pFromCell->m_iCost)
		< pToCell->m_iCost )
	{
		pToCell->m_iCost = (iCost + pFromCell->m_iCost);
		pToCell->m_pCellFrom = pFromCell;
	}
	else
		return;
	
	// prepare for occupation test
	pToCell->m_iDist = 0;

	// consider if hex is occupied
	//
	// might only be part of a unit if multi-hex units are allowed
	// or parts of other multi-hex units occupying hexes occupied
	// by this multi-hex unit
	//
	//CUnit *pUnit = pGameHex->GetUnit();
	BYTE bUnits = pDestHex->GetUnits();

	// force arrival at destinations, regardless of if occupied
	//if( pUnit != NULL && hexDest != m_hexTo )
	if( bUnits != 0 && hexDest != m_hexTo )
	{
		// unit is a building, consider if this entry hex
		// BUGBUG - but who owns it?
		//
		//if( pUnit->GetUnitType() == CUnit::building )
		//int iType = pUnit->GetUnitType();
		// BUGBUG dave needs a temp change when we update/sync
		// and he uses new hash system
		// if( iType != CUnit::vehicle )
		//if( iType == CUnit::building )
		if( bUnits & CHex::bldg )
		{
			// this is a hex of a building
			// and is not the destination hex for vehicle
			pToCell->m_iDist = 0xFFFE; // no entry
		}
		else if( bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr) )
		{
			// vehicle hexes are considered open unless block set
			if( m_bVehBlock )
				pToCell->m_iDist = 0xFFFE; // no entry
		}
	}
		

	// get distance to destination for enterable cells
	// and cost to enter is already in pCell->m_iCost
	if( !pToCell->m_iDist )
		pToCell->m_iDist = 
			(theMap.GetRangeDistance( hexDest, m_hexTo ) * m_iDistFactor);

	// else if distance is left alone then recalculate m_iBoth
	if( pToCell->m_iDist != 0xFFFE )
	{
		pToCell->m_iBoth = pToCell->m_iCost + pToCell->m_iDist;
	}
	else
		pToCell->m_iBoth = pToCell->m_iDist;
	NewBoth ( pToCell );

}

//
// using the actual sub-hex head and tail of the passed vehicle
// determine the vehicle's current heading, and then initialize
// a CCell for the hex that was left to reach the current from
// cell and secure the pFromCell->m_pCellFrom to point to it
//
void CPathMgr::GetFromCell( CVehicle *pVeh, CCell *pFromCell )
{
	int iHeading = pVeh->CalcBaseDir();
	CHexCoord hex( pFromCell->m_iX, pFromCell->m_iY );
	switch( iHeading )
	{
	case 1:
		hex.Yinc();
		hex.Xdec();
		break;
	case 2:
		hex.Xdec();
		break;
	case 3:
		hex.Ydec();
		hex.Xdec();
		break;
	case 4:
		hex.Ydec();
		break;
	case 5:
		hex.Xinc();
		hex.Ydec();
		break;
	case 6:
		hex.Xinc();
		break;
	case 7:
		hex.Xinc();
		hex.Yinc();
		break;
	case 0:
	default:
		hex.Ydec();
		break;
	}
	// create a CCell to point back to for heading
	CCell cc( hex.X(),hex.Y() );
	CCell * pCell = AddCellToArray( &cc );
	TRAP ();
#ifdef TEST_RESULT2
	TRAP ( pCell != GetCellAt( hex.X(),hex.Y() ) );
#endif
	pFromCell->m_pCellFrom = pCell;
}

//
// determine if iPos passed is a valid heading out of pFromCell based 
// on the heading used to enter pFromCell from pFromCell->m_pCellFrom
// and return TRUE if it is valid, or FALSE if not
//
BOOL CPathMgr::IsValidHeading( int iPos, CCell *pFromCell )
{
	// first cell, without a heading, will have NULL
	if( pFromCell->m_pCellFrom == NULL )
		return TRUE;

	// test if this is a single sub-hex vehicle?
	if( (m_pTD->GetVehFlags () & CTransportData::FL1hex) )
		return TRUE;

	// passed pFromCell was entered on a heading from m_pCellFrom
	CHexCoord hexTo( pFromCell->m_iX, pFromCell->m_iY );
	CHexCoord hexFrom( pFromCell->m_pCellFrom->m_iX, 
		pFromCell->m_pCellFrom->m_iY );

	// find the direction of the heading that was used to move
	// FROM pFromCell->m_pCellFrom TO pFromCell
	for( int i=0; i<CELLSAROUND; ++i )
	{
		CHexCoord hex = hexFrom;
		switch( i )
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
		if( hex == hexTo )
			break;
	}
	// 'i' now represents heading used to enter pFromCell
	// from pFromCell->m_pCellFrom, so use it to access
	// lookup table of candidate directions off of that 
	// heading in which each byte is a bit array with 1
	// in the bit that represents a candidate direction
	// for that heading
	unsigned char ucDirections = ucHeadings[i];
	unsigned char ucTest = 0;
	ucTest = 1 << iPos;
	// if iPos represents a valid heading return TRUE
	if( (ucDirections & ucTest) )
		return TRUE;

	return FALSE;
}

/*
>x == newHex.x - oldHex.x
>y == newHex.y - oldHex.y
>
>you then try:
>
>newHex.x + x, newHex.y + y	// straight line
>newHex.x + __minmax (-1, 1, x - y), newHex.y + __minmax (-1, 1, x + y)	// soft right turn
>newHex.x + __minmax (-1, 1, y + x), newHex.y + __minmax (-1, 1, y - x)	// soft left turn
>
>#define __minmax(min,max,val)  (val < min ? min : (val > max ? max : val))
>
*/
void CPathMgr::GetHeadingCell( int iPos, CCell *pFromCell, int& iX, int& iY )
{
	// passed pFromCell was entered on a heading from m_pCellFrom
	CHexCoord hexTo( pFromCell->m_iX, pFromCell->m_iY );
	CHexCoord hexFrom( pFromCell->m_pCellFrom->m_iX, 
		pFromCell->m_pCellFrom->m_iY );

	// what if hexFrom.X() == 127 and hexTo.X() == 0 ?
	int x = hexTo.X() - hexFrom.X();
	x = x > 1 ? 1 : (x < -1 ? -1 : x);
	int y = hexTo.Y() - hexFrom.Y();
	y = y > 1 ? 1 : (y < -1 ? -1 : y);

	if( iPos == 0 ) // on same heading
	{
		iX = hexTo.Wrap( hexTo.X() + x );
		iY = hexTo.Wrap( hexTo.Y() + y );
	}
	else if( iPos == 1 ) // soft right turn from heading
	{
		iX = hexTo.Wrap( hexTo.X() + __minmax (-1, 1, x - y) );
		iY = hexTo.Wrap( hexTo.Y() + __minmax (-1, 1, x + y) );
	}
	else if( iPos == 2 ) // soft left turn from heading
	{
		iX = hexTo.Wrap( hexTo.X() + __minmax (-1, 1, y + x) );
		iY = hexTo.Wrap( hexTo.Y() + __minmax (-1, 1, y - x) );
	}
}


//
// get the adjacent cell x,y values
// in the iPos direction from pFromCell
//
void CPathMgr::GetCellAt( int iPos, CCell *pFromCell, int& iX, int& iY )
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
CCell *CPathMgr::GetClosestCell( void )
{
	CCell *pcClosest = NULL;
	int iBestDist = INT_MAX;

	// change to reflect new approach
	int iEnd = min( m_iNextSlot, m_iNumOfCells );

	CCell *pCell = &m_paCells[0];
	for( int i=0; i<iEnd; ++i, pCell++ )
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
CCell *CPathMgr::xGetLowestCost( int& iCnt )
{
	CCell *pcLowest = NULL;
	int iCost = 0xFFFE;
	iCnt = 0;

	// change to reflect new approach
	int iEnd = min( m_iNextSlot, m_iNumOfCells );

	CCell *pCell = &m_paCells[0];
	CCell *pEnd = pCell + iEnd;

	// consider combined cost after dest reached 1st time
	if( m_iBestCost != 0xFFFE )
		{
		for( ; pCell<pEnd; pCell++ )
			{
   		if ( pCell->m_iBoth < iCost && pCell->m_iBoth )
				{
				pcLowest = pCell;
				iCost = pCell->m_iBoth;
				iCnt = 0;
				}
			else
				if( pCell->m_iBoth == iCost )
					iCnt++;
			}
		return( pcLowest );
		}

	// until the destination is reached once, 
	// attempt direct path considering only distance
	for( ; pCell<pEnd; pCell++ )
		{
		if( pCell->m_iDist && pCell->m_iBoth && pCell->m_iDist < iCost )
			{
			pcLowest = pCell;
			iCost = pCell->m_iDist;
			iCnt = 0;
			}
//BUGBUG - eric had this but it makes no sense		if( pCell->m_iBoth == iCost )
// also eric did not have the else
		else
			if( pCell->m_iBoth && ( pCell->m_iDist == iCost ) )
				iCnt++;
		}

	return( pcLowest );
}

CCell *CPathMgr::GetLowestCost( int& iCnt )
{

#ifdef TEST_RESULT3
	{
	CCell * * ppCell = m_acBoth;
	for (int iOn=0; iOn<MAX_BOTH_INDEX; iOn++)
		{
		if ( *ppCell != NULL )
			{
			TRAP ( iOn < m_iLowestBoth );
			TRAP ( (*ppCell)->m_iBothIn != iOn );
			}
		ppCell ++;
		}
	}
#endif

	// use old method if out of range
	if ( ( m_iLowestBoth <= 0 ) || ( MAX_BOTH_INDEX <= m_iLowestBoth ) )
		return xGetLowestCost ( iCnt );

	// special case
	if ( ( m_acBoth [m_iLowestBoth] != NULL ) && 
										( m_acBoth [m_iLowestBoth]->m_pcNextBoth == NULL ) )
		{
		iCnt = 0;
#ifdef TEST_RESULT2
		int iTest;
		TRAP ( m_acBoth [m_iLowestBoth] != xGetLowestCost ( iTest ) );
		TRAP ( iTest != iCnt );
#endif
		return m_acBoth [m_iLowestBoth];
		}

	// find lowest entry
	CCell * * ppCellOn = &m_acBoth [m_iLowestBoth];
	int iNum = MAX_BOTH_INDEX - m_iLowestBoth;
	while ( ( *ppCellOn == NULL ) && ( iNum-- > 0 ) )
		ppCellOn ++;

	// no entries
	if ( iNum <= 0 )
		return xGetLowestCost ( iCnt );

	// we now walk the linked list returning the lowest one
	CCell * pcLowest, * pCellOn;
	pcLowest = pCellOn = * ppCellOn;
	iCnt = 0;
	while ( pCellOn->m_pcNextBoth != NULL )
		{
		pCellOn = pCellOn->m_pcNextBoth;
		pcLowest = __min ( pcLowest, pCellOn );
		iCnt ++;
		}

#ifdef TEST_RESULT2
	int iTest;
	TRAP ( pcLowest != xGetLowestCost ( iTest ) );
	TRAP ( iTest != iCnt );
#endif
	return( pcLowest );
}

void CPathMgr::NewBoth ( CCell * pTest )
{

#ifdef TEST_RESULT3
	{
	CCell * * ppCell = m_acBoth;
	for (int iOn=0; iOn<MAX_BOTH_INDEX; iOn++)
		{
		if ( *ppCell != NULL )
			{
			TRAP ( iOn < m_iLowestBoth );
			TRAP ( (*ppCell)->m_iBothIn != iOn );
			}
		ppCell ++;
		}
	}
#endif

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

	// common - get out fast
	if ( pTest->m_iBoth == 0 )
		{
		pTest->m_pcNextBoth = pTest->m_pcPrevBoth = NULL;
		pTest->m_iBothIn = 0;
#ifdef TEST_RESULT3
	{
	CCell * * ppCell = m_acBoth;
	for (int iOn=0; iOn<MAX_BOTH_INDEX; iOn++)
		{
		if ( *ppCell != NULL )
			{
			TRAP ( iOn < m_iLowestBoth );
			TRAP ( (*ppCell)->m_iBothIn != iOn );
			}
		ppCell ++;
		}
	}
#endif

		return;
		}

	// figure the cost
	int iCost;
	if ( m_iBestCost != 0xFFFE )
		iCost = pTest->m_iBoth;
	else
		iCost = pTest->m_iDist;

	// add new
	pTest->m_pcPrevBoth = NULL;
	if ( (0 < iCost) && (iCost < MAX_BOTH_INDEX) )
		{
		CCell * pCelOn = m_acBoth [iCost];
		pTest->m_pcNextBoth = pCelOn;
		if ( pCelOn != NULL )
			pCelOn->m_pcPrevBoth = pTest;
		m_acBoth [iCost] = pTest;
		pTest->m_iBothIn = iCost;
		if ( m_iLowestBoth == 0 )
			m_iLowestBoth = iCost;
		else
			m_iLowestBoth = __min ( m_iLowestBoth, iCost );
		}

	else
		{
		pTest->m_pcNextBoth = NULL;
		pTest->m_iBothIn = 0;
		}

#ifdef TEST_RESULT3
	{
	CCell * * ppCell = m_acBoth;
	for (int iOn=0; iOn<MAX_BOTH_INDEX; iOn++)
		{
		if ( *ppCell != NULL )
			{
			TRAP ( iOn < m_iLowestBoth );
			TRAP ( (*ppCell)->m_iBothIn != iOn );
			}
		ppCell ++;
		}
	}
#endif

}

CCell *CPathMgr::GetCellAt( int iX, int iY )
{

	DWORD dwKey = ( iX & 0xFFFF) | ( ( iY & 0xFFFF ) << 16 );
	CCell * pCellFind;
	// do we have it?
	if ( m_mapCell.Lookup ( dwKey, pCellFind ) == 0 )
		pCellFind = NULL;
	else
		{
		// only need to look if more than 1
		while ( pCellFind->m_pCellNext != NULL )
			{
			TRAP ();
			if ( ( pCellFind->m_iX == iX ) && ( pCellFind->m_iY == iY ) )
				{
				TRAP ();
				break;
				}
			TRAP ();
			pCellFind = pCellFind->m_pCellNext;
			}
#ifdef TEST_RESULT1
		TRAP ( ( pCellFind->m_iX != iX ) || ( pCellFind->m_iY != iY ) );
#endif
		}

#ifdef TEST_RESULT1
	CCell *pNewCell = &m_paCells[0];
	for( int j=0; j<m_iNextSlot; ++j, pNewCell++ )
	{
		if( pNewCell->m_iX == iX && pNewCell->m_iY == iY )
			{
			TRAP ( pNewCell != pCellFind );
			goto found;
			}
	}
	TRAP ( pCellFind != NULL );
found:
#endif

	return ( pCellFind );
}

CCell * CPathMgr::AddCellToArray( CCell *pCell )
{
	if( m_iNextSlot >= m_iNumOfCells )
		return NULL;

	CCell *pNewCell = &m_paCells[m_iNextSlot++];
	pNewCell->m_iX = pCell->m_iX;
	pNewCell->m_iY = pCell->m_iY;
	pNewCell->m_iCost = pCell->m_iCost;
	pNewCell->m_iDist = pCell->m_iDist;
	pNewCell->m_iBoth = pCell->m_iBoth;
	pNewCell->m_pCellFrom = pCell->m_pCellFrom;
	NewBoth ( pNewCell );

	DWORD dwKey = ( pCell->m_iX & 0xFFFF) | ( ( pCell->m_iY & 0xFFFF ) << 16 );
	CCell * pCellFind;
	// add first element to hash table
	if ( m_mapCell.Lookup ( dwKey, pCellFind ) == 0 )
		{
		m_mapCell.SetAt ( dwKey, pNewCell );
		pNewCell->m_pCellNext = NULL;
		}

	// add another element to a hash element
	else
		{
		TRAP ();
		while ( pCellFind->m_pCellNext != NULL )
			{
			TRAP ();
			pCellFind = pCellFind->m_pCellNext;
			}
		TRAP ();
		pCellFind->m_pCellNext = pNewCell;
		}

	return pNewCell;
}

//
// clear the contents of the array
//
void CPathMgr::ClearArray( void )
{
	const int iVal = 0xFFFE;
	const int iZero = 0;
	
	int iEnd = min( m_iNextSlot, m_iNumOfCells );

	m_mapCell.RemoveAll ();

	CCell *pCell = &m_paCells[0];
	for( int i=0; i<iEnd; ++i, pCell++ )
	{
		pCell->m_iX = iZero;
		pCell->m_iY = iZero;
		pCell->m_iCost = iVal;
		pCell->m_iDist = iVal;
		pCell->m_iBoth = iVal;
		pCell->m_pCellFrom = NULL;
		pCell->m_pCellNext = NULL;
		pCell->m_pcNextBoth = NULL;
		pCell->m_pcPrevBoth = NULL;
		pCell->m_iBothIn = 0;
	}

	m_iLowestBoth = 0;
	memset ( m_acBoth , 0, sizeof (m_acBoth) );
}

////////////////////////////////////////////////////////////////////////
//
// constructor with initialization
//
CPathMgr::CPathMgr( int iMapEX, int iMapEY )
{
	m_iWidth = iMapEX;
	m_iHeight = iMapEY;
	m_iMapEX = iMapEX - 1;
	m_iMapEY = iMapEY - 1;
	m_bVehBlock = TRUE;

	m_iNumOfCells = (m_iWidth + m_iHeight) * 2; // m_iWidth * m_iHeight;
	m_iNextSlot = 0;
	m_iLowestBoth = 0;
	memset ( m_acBoth , 0, sizeof (m_acBoth) );
	m_iFirst = m_iNumOfCells-1;
	m_iLast = 0;
	
	m_paCells = new CCell[m_iNumOfCells];

	m_mapCell.RemoveAll ();
	m_mapCell.InitHashTable ( GetPrime ( m_iNumOfCells * 2 ) );

	m_iNextSlot = m_iFirst;
	ClearArray();
	m_iNextSlot = 0;
	m_iLowestBoth = 0;

	return;
}


void CPathMgr::Close ()
{
	//ReportCounts();

	delete [] m_paCells;
	m_paCells = NULL;

	m_mapCell.RemoveAll ();
}


//
// separate initialization process
//
BOOL CPathMgr::Init( int iMapEX, int iMapEY )
{
	m_iPaths=0;	// count of all calls
	m_iOrtho=0;	// count of x==x or y==y paths
	m_iHP=0;	// count of non Vehicle paths
	m_iPathTicks=0; // sum of ticks in paths
	m_iStepCnt=0;   // count of steps in paths
	m_iHangTicks=0; // sum of ticks in hangs
	m_iHangCnt=0;   // count of hang steps

	m_iWidth = iMapEX;
	m_iHeight = iMapEY;
	m_iMapEX = iMapEX - 1;
	m_iMapEY = iMapEY - 1;

	m_iNumOfCells = (m_iWidth + m_iHeight) * 2; // m_iWidth * m_iHeight;
	m_iNextSlot = 0;
	m_iLowestBoth = 0;
	memset ( m_acBoth , 0, sizeof (m_acBoth) );
	m_iFirst = m_iNumOfCells-1;
	m_iLast = 0;
	m_iMaxPath = MAX_PATH_RANGE;

	// make adjustment if using large or medium worlds
	/*
	if( theGame.GetSideSize() > 64 )
		m_iMaxPath /= 3;
	else if( theGame.GetSideSize() >= 32 )
		m_iMaxPath /= 2;
	*/
		
	delete [] m_paCells;

	m_paCells = new CCell[m_iNumOfCells];

	m_mapCell.RemoveAll ();
	m_mapCell.InitHashTable ( GetPrime ( m_iNumOfCells + 1 ) );

	m_iNextSlot = m_iFirst;
	ClearArray();
	m_iNextSlot = 0;
	m_iLowestBoth = 0;

#if 1
	// BUGBUG used for solving repeating path problem
	m_lastFrom.X(0);
	m_lastFrom.Y(0);
	m_lastTo = m_lastFrom;
#endif

#if PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"\nCPathMgr::Init() for %d,%d  m_iNumOfCells=%d ", 
		iMapEX, iMapEX, m_iNumOfCells );
#endif
#endif

	return TRUE;
}

//
// constructor does not initialize
//
CPathMgr::CPathMgr( void )
{
	m_iWidth = 0;
	m_iHeight = 0;
	m_iMapEX = 0;
	m_iMapEY = 0;
	m_iDistFactor = 0;
	m_bVehBlock = TRUE;

	m_iLowestBoth = 0;
	memset ( m_acBoth , 0, sizeof (m_acBoth) );
	m_paCells = NULL;
}

CPathMgr::~CPathMgr()
{
	delete [] m_paCells;

	m_mapCell.RemoveAll ();
}

#if 0
void CPathMgr::ReportCounts( void )
{
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"\nTotal path calls %d  ortho %d  HP %d", 
		m_iPaths,m_iOrtho,m_iHP );
	if( !m_iStepCnt )
		m_iStepCnt = 1;
	if( m_iPathTicks > m_iStepCnt )
	{
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"path ticks %d  steps %d  ticks/step %d", 
		m_iPathTicks,m_iStepCnt,m_iPathTicks/m_iStepCnt );
	}
	else if( m_iPathTicks )
	{
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"path ticks %d  steps %d  steps/tick %d", 
		m_iPathTicks,m_iStepCnt,m_iStepCnt/m_iPathTicks );
	}
	if( !m_iHangCnt )
		m_iHangCnt = 1;
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"hang ticks %d  steps %d  ticks/step %d \n", 
		m_iHangTicks,m_iHangCnt,m_iHangTicks/m_iHangCnt );
}
#endif

//
// one of Dave's CGameMap object member functions
//
int CGameMap::GetTravelTime( CHexCoord const & hexSrc, 
	CHexCoord const & hexDest, int iVehType )
{
	CHexCoord hcFrom = hexSrc;
	CHexCoord hcTo = hexDest;
	// get the path
	int iPathLen;
	CHexCoord *pHexPath = thePathMgr.GetPath( NULL, hcFrom, 
		hcTo, iPathLen, iVehType );

	// get the wheel type again
	CTransportData const *pTD = theTransports.GetData(iVehType);
	if( pTD == NULL )
		return( 0 );

	int iWheel = pTD->GetWheelType();

	// now cost it out
	int iPathCost = 0;
	CHexCoord fromHex = hexSrc;
	CHexCoord toHex;
	for( int i=0; i<iPathLen; ++i )
	{
		toHex = pHexPath[i];
		int iPos = thePathMgr.GetCellDirection( fromHex, toHex );

		// bad position returned
		if( iPos == CELLSAROUND )
			break;
		// add up the final costs
		iPathCost += theMap.GetTerrainCost( 
			fromHex, toHex, iPos, iWheel );
		// move along the path
		fromHex = toHex;
	}

	delete [] pHexPath;
	return( iPathCost );
}

//
// return the cells around direction used to go fromHex toHex
//
int CPathMgr::GetCellDirection( CHexCoord& fromHex, CHexCoord& toHex )
{
	// consider 0
	CHexCoord nextHex = fromHex;
	nextHex.Ydec();
	if( nextHex.m_iY == toHex.m_iY &&
		nextHex.m_iX == toHex.m_iX )
		return( 0 );
	
	// consider 1
	nextHex = fromHex;
	nextHex.Ydec();
	nextHex.Xinc();
	if( nextHex.m_iY == toHex.m_iY &&
		nextHex.m_iX == toHex.m_iX )
		return( 1 );

	// consider 2
	nextHex = fromHex;
	nextHex.Xinc();
	if( nextHex.m_iY == toHex.m_iY &&
		nextHex.m_iX == toHex.m_iX )
		return( 2 );

	// consider 3
	nextHex = fromHex;
	nextHex.Yinc();
	nextHex.Xinc();
	if( nextHex.m_iY == toHex.m_iY &&
		nextHex.m_iX == toHex.m_iX )
		return( 3 );

	// consider 4
	nextHex = fromHex;
	nextHex.Yinc();
	if( nextHex.m_iY == toHex.m_iY &&
		nextHex.m_iX == toHex.m_iX )
		return( 4 );

	// consider 5
	nextHex = fromHex;
	nextHex.Yinc();
	nextHex.Xdec();
	if( nextHex.m_iY == toHex.m_iY &&
		nextHex.m_iX == toHex.m_iX )
		return( 5 );

	// consider 6
	nextHex = fromHex;
	nextHex.Xdec();
	if( nextHex.m_iY == toHex.m_iY &&
		nextHex.m_iX == toHex.m_iX )
		return( 6 );

	// consider 7
	nextHex = fromHex;
	nextHex.Ydec();
	nextHex.Xdec();
	if( nextHex.m_iY == toHex.m_iY &&
		nextHex.m_iX == toHex.m_iX )
		return( 7 );

	return( CELLSAROUND );
}

/////////////////////////////////////////////////////////////////////////
//
// CCell
//
// the basic cell object
//
/////////////////////////////////////////////////////////////////////////

CCell::CCell()
{
	m_iX = 0;
	m_iY = 0;
	m_iCost = 0xFFFE;
	m_iDist = 0xFFFE;
	m_iBoth = 0xFFFE;
	m_pCellFrom = NULL;
	m_pCellNext = NULL;
	m_pcNextBoth = NULL;
	m_pcPrevBoth = NULL;
	m_iBothIn = 0;
}

CCell::CCell( int iX, int iY )
{
	m_iX = iX;
	m_iY = iY;
	m_iCost = 0xFFFE;
	m_iDist = 0xFFFE;
	m_iBoth = 0xFFFE;
	m_pCellFrom = NULL;
	m_pCellNext = NULL;
	m_pcNextBoth = NULL;
	m_pcPrevBoth = NULL;
	m_iBothIn = 0;
}

// end of CPathMgr.cpp
