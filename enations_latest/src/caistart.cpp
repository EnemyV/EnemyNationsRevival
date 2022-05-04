////////////////////////////////////////////////////////////////////////////
//
//  CAIStart.cpp :  CAIStart object implementation
//                  Divide and Conquer AI
//               
//  Last update:  09/04/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CAIStart.hpp"
#include "logging.h"	// dave's logging system


extern CAIData *pGameData;	// pointer to game data interface

#define new DEBUG_NEW


// these will be offsets to array that contains scores
// of the highest quantity of each resource
#define START_COAL		0
#define START_IRON		1
#define START_OIL		2
#define START_COPPER	3
#define START_MOLY		4
#define START_COUNT		5

//
// this object evaluates the start hex location passed
// and considers the surrounding pGameData->m_iHexPerBlk/2
// cells in trying to find the cell with the most buildable
// pGameData->m_iStructureSize sized blocks of cells
//
CAIStart::CAIStart( int iX, int iY )
{
	// start with what is passed as the default
	m_iBlockX = iX;
	m_iBlockY = iY;

	// calculate the number of hexes in a group
	m_iGrpSize = pGameData->m_iStructureSize *
		pGameData->m_iStructureSize;
	// and create a work area that size
	m_pgrpHexs = new CAIHex[m_iGrpSize];

	// the cells that can be considered in any direction
	//int iCellsSeen = (pGameData->m_iHexPerBlk - 1);
	int iCellsSeen = (pGameData->m_iHexPerBlk/2);
	m_iBlockSize = pGameData->m_iHexPerBlk;

	CHexCoord hex( iX, iY );
	hex.X( hex.Wrap(iX-iCellsSeen) );
	hex.Y( hex.Wrap(iY-iCellsSeen) );
	// save start hex of region for further calculations
	m_iBaseX = hex.X();
	m_iBaseY = hex.Y();

	// and initialize a word array to hold the values
	//m_iMapSize = pGameData->m_iHexPerBlk * pGameData->m_iHexPerBlk;
	m_iMapSize = m_iBlockSize * m_iBlockSize;
	try
	{
		m_pwaSeen = new WORD[m_iMapSize];
		//memset( m_pwaSeen, 0, (size_t)(m_iMapSize * sizeof( WORD )) );
	}
	catch( CException e )
	{
		throw(ERR_CAI_BAD_NEW);
	}

	// set up array to contain location and best quantity
	// of minerals found, and clear it out
	try
	{
		m_pmnHexs = new CAIHex[START_COUNT];
	}
	catch( CException e )
	{
		throw(ERR_CAI_BAD_NEW);
	}
}

void CAIStart::PickStartHex( void )
{
	memset( m_pwaSeen, 0, (size_t)(m_iMapSize * sizeof( WORD )) );

	CAIHex *paiHex = NULL;
	for( int i=0; i<START_COUNT; ++i )
	{
		paiHex = &m_pmnHexs[i];

		paiHex->m_iX = 0;
		paiHex->m_iY = 0;
		paiHex->m_iUnit = 0;
		paiHex->m_dwUnitID = (DWORD)0;
		paiHex->m_cTerrain = (BYTE)0xFF;
	}
	CHexCoord hexBase( m_iBaseX,m_iBaseY );
	CHexCoord hex;
	CAIHex aiHex(0,0);
	int iX, iY;

#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 

	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIStart::PickStartHex " );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"from block at %d,%d ", m_iBlockX, m_iBlockY );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"from %d,%d for %d hexes ", 
		m_iBaseX, m_iBaseY, m_iBlockSize );
#endif

	for( iY=0; iY<m_iBlockSize; ++iY )
	{
		hex.Y( hex.Wrap(hexBase.Y()+iY) );

		for( iX=0; iX<m_iBlockSize; ++iX )
		{
			hex.X( hex.Wrap(hexBase.X()+iX) );

			aiHex.m_iX = hex.X();
			aiHex.m_iY = hex.Y();

			i = GetMapOffset( hex.X(), hex.Y() );
			if( i < 0 || i >= m_iMapSize )
				continue;

			m_pwaSeen[i] = 0;
			// only want cells that allow construction
			// in group size from any direction
			if( CountAround( &aiHex ) == MAX_ADJACENT )
			{
				m_pwaSeen[i] = 1;
			}
			// then find the best rating for a location for
			// the minerals that exist in the cluster around
			// each cell
			RateMinerals(hex);
		}
	}

	int iBestRating = m_iMapSize;
	int iBest = m_iMapSize;
	int iCnt = 0;
	int iRate = 0;

	
	for( iY=0; iY<m_iBlockSize; ++iY )
	{
		hex.Y( hex.Wrap(hexBase.Y()+iY) );

		for( iX=0; iX<m_iBlockSize; ++iX )
		{
			hex.X( hex.Wrap(hexBase.X()+iX) );
			
			i = GetMapOffset( hex.X(), hex.Y() );
			if( i < 0 || i >= m_iMapSize )
				continue;

			if( !m_pwaSeen[i] )
				continue;

			iRate = RateMineralDist(hex);
			if( iRate == m_iMapSize )
				iRate = 0;
			m_pwaSeen[i] = iRate;

			if( iRate && iRate < iBestRating )
			{
				iBestRating = iRate;
				iBest = i;
				iCnt = 0;
			}
			if( iRate == iBestRating )
				iCnt++;
		}
	}


	if( iCnt == 1 )
	{
		OffsetToXY( iBest, &m_iBlockX, &m_iBlockY );

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"start map took %ld ticks for map size of %d ", 
    	(dwEnd - dwStart), m_iMapSize );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"picking %d,%d \n", m_iBlockX, m_iBlockY );
#endif

		return;
	}

	iCnt = 0;
	// if still here, then there are more than one best
	// location for minerals, so clear all but the best
	for( iY=0; iY<m_iBlockSize; ++iY )
	{
		hex.Y( hex.Wrap(hexBase.Y()+iY) );

		for( iX=0; iX<m_iBlockSize; ++iX )
		{
			hex.X( hex.Wrap(hexBase.X()+iX) );
			
			i = GetMapOffset( hex.X(), hex.Y() );
			if( i < 0 || i >= m_iMapSize )
				continue;
			if( !m_pwaSeen[i] )
				continue;
			if( m_pwaSeen[i] == iBestRating )
			{
				m_pwaSeen[i] = 1;
				++iCnt;
			}
			else
				m_pwaSeen[i] = 0;
		}
	}


	iBestRating = m_iMapSize;
	iBest = m_iMapSize;
	iCnt = 0;

	// so now apply build multiplier
	// of build cluster around the location
	for( iY=0; iY<m_iBlockSize; ++iY )
	{
		hex.Y( hex.Wrap(hexBase.Y()+iY) );

		for( iX=0; iX<m_iBlockSize; ++iX )
		{
			hex.X( hex.Wrap(hexBase.X()+iX) );
			
			i = GetMapOffset( hex.X(), hex.Y() );
			if( i < 0 || i >= m_iMapSize )
				continue;

			if( !m_pwaSeen[i] )
				continue;

			m_pwaSeen[i] = GetBuildMultiplier(i);
			if( m_pwaSeen[i] < iBestRating )
			{
				iBestRating	= m_pwaSeen[i];
				iBest = i;
				iCnt = 0;
			}
			if( iBestRating	== m_pwaSeen[i] )
				iCnt++;
		}
	}


	// early out
	if( iCnt == 1 )
	{
		OffsetToXY( iBest, &m_iBlockX, &m_iBlockY );

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"start map took %ld ticks for map size of %d ", 
    	(dwEnd - dwStart), m_iMapSize );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"picking %d,%d \n", m_iBlockX, m_iBlockY );
#endif
		return;
	}

	// if still here, there are multiple best mineral cells
	// with multiple best cluster build multipliers so zero
	// out all but the best 
	iCnt = 0;
	for( iY=0; iY<m_iBlockSize; ++iY )
	{
		hex.Y( hex.Wrap(hexBase.Y()+iY) );

		for( iX=0; iX<m_iBlockSize; ++iX )
		{
			hex.X( hex.Wrap(hexBase.X()+iX) );
			
			i = GetMapOffset( hex.X(), hex.Y() );
			if( i < 0 || i >= m_iMapSize )
				continue;
			if( !m_pwaSeen[i] )
				continue;
			if( m_pwaSeen[i] == iBestRating )
			{
				m_pwaSeen[i] = 1;
				++iCnt;
			}
			else
				m_pwaSeen[i] = 0;
		}
	}


	// and randomly pick one
	iBest = pGameData->GetRandom(iCnt);
	iCnt = 0;
	for( i=0;i<m_iMapSize; ++i )
	{
		if( !m_pwaSeen[i] )
			continue;
		if( iBest == iCnt )
		{
			iBest = i;
			break;
		}
		iCnt++;
	}

	if( iBest || iCnt )
		OffsetToXY( iBest, &m_iBlockX, &m_iBlockY );

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"start map took %ld ticks for map size of %d ", 
    	(dwEnd - dwStart), m_iMapSize );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"picking %d,%d ", m_iBlockX, m_iBlockY );
#endif

}


void CAIStart::RateMinerals( CHexCoord& hex )
{
	CMinerals *pmn;
	//CHexCoord hex( paiHex->m_iX, paiHex->m_iY );
	CAIHex *pmnHex = NULL;
	int iMat,iQty,iDensity;

	// are there minerals in this location
	if( theMinerals.Lookup(hex, pmn) )
	{
		switch( pmn->GetType() )
		{
			case CMaterialTypes::coal:
				iMat = START_COAL;
				break;
			case CMaterialTypes::iron:
				iMat = START_IRON;
				break;
			case CMaterialTypes::oil:
				iMat = START_OIL;
				break;
			case CMaterialTypes::copper:
				iMat = START_COPPER;
				break;
			case CMaterialTypes::moly:
				iMat = START_MOLY;
				break;
			default:
				return;
		}
		// quantity of this mineral at this location
		iQty = pmn->GetQuantity();
		// pointer to CAIHex for this mineral
		pmnHex = &m_pmnHexs[iMat];

		if( (DWORD)iQty > pmnHex->m_dwUnitID )
		{
			pmnHex->m_iX = hex.X();
			pmnHex->m_iY = hex.Y();
			pmnHex->m_dwUnitID = (DWORD)iQty;
		}

		// now consider density, and save
		iMat = pmn->GetType();
		iDensity = pmn->GetDensity();
		pmnHex = &pGameData->m_paihDensity[iMat];
		if( iDensity > (int)pmnHex->m_cTerrain )
		{
			pmnHex->m_iX = hex.X();
			pmnHex->m_iY = hex.Y();
			pmnHex->m_cTerrain = (BYTE)iDensity;
			pmnHex->m_dwUnitID = (DWORD)iQty;
		}
	}
}

//
// for passed location get distance from the cells in the m_pmnHexs[]
//
int CAIStart::RateMineralDist( CHexCoord& hexFrom )
{
	//CHexCoord hexFrom( paiHex->m_iX, paiHex->m_iY );
	CHexCoord hexTo;
	int iDist, iDistTot = 0;
	int iBuffer = (CBuildFarm::GetCowDistance()*2);
	// use smaller buffer for large maps
	if( pGameData->m_iHexPerBlk > 64 )
		iBuffer = 1;

	// for each type of mineral
	for( int i=0; i<START_COUNT; ++i )
	{
		CAIHex *pmnHex = &m_pmnHexs[i];
		if( pmnHex->m_dwUnitID )
		{
			hexTo.X( pmnHex->m_iX );
			hexTo.Y( pmnHex->m_iY );
			iDist = 
				pGameData->GetRangeDistance( hexFrom, hexTo );
			// make sure that mineral is farther than buffer
			if( iDist <= iBuffer )
			{
				return( m_iMapSize );
			}
			iDistTot += iDist;
		}
	}

	// rate for location of forest terrain
	int iForests = RateForests(hexFrom);
	iDistTot *= iForests;

	return( iDistTot );
}

//
// calculate a forest factor by counting the forest terrain
// found in an area surrounding this location
//
int CAIStart::RateForests( CHexCoord& hexBase )
{
	CHexCoord hex,hcFrom,hcTo;

	hcFrom.X( hex.Wrap(hexBase.X()-pGameData->m_iStructureSize) );
	hcFrom.Y( hex.Wrap(hexBase.Y()-pGameData->m_iStructureSize) );
	hcTo.X( hex.Wrap(hexBase.X()+pGameData->m_iStructureSize) );
	hcTo.Y( hex.Wrap(hexBase.Y()+pGameData->m_iStructureSize) );

	int iDeltax = hex.Wrap(hcTo.X()-hcFrom.X());
	int iDeltay = hex.Wrap(hcTo.Y()-hcFrom.Y());
	int iForests = 1, iCnt = 0;
	iCnt = iDeltax * iDeltay;

	for( int iY=0; iY<iDeltay; ++iY )
	{
		hex.Y( hex.Wrap(hcFrom.Y()+iY) );

		for( int iX=0; iX<iDeltax; ++iX )
		{
			hex.X( hex.Wrap(hcFrom.X()+iX) );

		    CHex *pGameHex = theMap.GetHex( hex );
		    // known hex means return is not NULL
		    if( pGameHex == NULL )
			    continue;

			if( pGameHex->GetType() == CHex::forest )
				iForests++;
		}
	}

	// convert count to a factor
	if( iForests > (iCnt / 4) * 3 )
		iForests = 1;
	else if( iForests > (iCnt / 2) )
		iForests = 2;
    else if( iForests > (iCnt / 4) )
		iForests = 3;
	else
		iForests = 4;
	return( iForests );
}

//
// calculate the build multiplier for all
// adjacent groups of hexes for the hex 
// at the passed offset
//
int CAIStart::GetBuildMultiplier( int iOffset )
{
	// get the actual x,y at this offset
	CAIHex aiHex(0,0);
	OffsetToXY( iOffset, &aiHex.m_iX, &aiHex.m_iY );

	int iX, iY;
	// cumulative multiplier for all groups
	// adjacent to this hex
	int iMult = 0;
	for( int i=0; i<MAX_ADJACENT; ++i )
	{
		ClearGroupHexes();

		// determine start x,y of group in this
		// direction to pass to group hex load
		GetGroupXY( &aiHex, i, &iX, &iY );

		LoadGroupHexes( iX, iY );

		iMult += GetGroupMult();
	}
	return( iMult );
}
//
// calculate the build multiplier each
// hex in the current
// group of hexes in m_pgrpHexs[]
//
int CAIStart::GetGroupMult(void)
{
	int iMult = 0;
	CAIHex *paiGroup = NULL;
	CHexCoord hex;
	for( int i=0; i<m_iGrpSize; ++i )
	{
		paiGroup = &m_pgrpHexs[i];

		// get location from game data
		//pGameData->GetCHexData(paiGroup);
		hex.X( paiGroup->m_iX );
		hex.Y( paiGroup->m_iY );
		CHex *pGameHex = theMap.GetHex( hex );
		// known hex means return is not NULL
		if( pGameHex == NULL )
			continue;

		CTerrainData const *pTerrain = 
			pGameData->GetTerrainData( pGameHex->GetType() );
		if( pTerrain == NULL )
			continue;

		iMult += pTerrain->GetBuildMult();
	}
	return( iMult );
}

//
// count the pGameData->m_iStructureSize sized groups of
// buildable cells and return the count
//
int CAIStart::CountAround( CAIHex *paiHex )
{
	// check to see if this terrain allows a build
	// if not, then return(0);
	if( !IsHexValidToBuild( paiHex ) )
		return(0);

	// then if still here, consider its adjacent groups
	int iCnt = 0;
	for( int i=0; i<MAX_ADJACENT; ++i )
	{
		if( IsGroupBuildable( paiHex, i ) )
			iCnt++;
	}
	if( iCnt < (MAX_ADJACENT/2) )
		iCnt = 0;
	return( iCnt );
}

BOOL CAIStart::IsGroupBuildable( CAIHex *paiHex, int iPos )
{
	ClearGroupHexes();

	int isx,isy;

	// determine start x,y to pass to load
	GetGroupXY( paiHex, iPos, &isx, &isy );

	LoadGroupHexes( isx, isy );

	// now for each hex in the group
	// consider if constuction is valid
	CAIHex *paiGroup;
	for( int i=0; i<m_iGrpSize; ++i )
	{
		paiGroup = &m_pgrpHexs[i];
		if( !IsHexValidToBuild(paiGroup) )
			return FALSE;
	}
	return TRUE;
}

BOOL CAIStart::IsHexValidToBuild( CAIHex *paiHex )
{
	// get location from game data, non-zero return
	// indicates sloped terrain, can't build there
	//if( pGameData->GetCHexData(paiHex) )
	//	return FALSE;
	//pGameData->GetCHexData(paiHex);
	CHexCoord hex( paiHex->m_iX, paiHex->m_iY );
	CHex *pGameHex = theMap.GetHex( hex );
	// known hex means return is not NULL
	if( pGameHex == NULL )
		return FALSE;
	int iTerrain = pGameHex->GetType();

	// consider the type of terrain from the map
	if( iTerrain == CHex::ocean ||
		iTerrain == CHex::river ||
		iTerrain == CHex::swamp ||
		iTerrain == CHex::desert ||
		iTerrain == CHex::mountain ||
		iTerrain == CHex::hill )
		return FALSE;

	return TRUE;
}

void CAIStart::GetGroupXY( CAIHex *paiHex, int iPos, int *piX, int *piY )
{
	CHexCoord hex( paiHex->m_iX, paiHex->m_iY );
	switch( iPos )
	{
		// north group
		case 0:
			*piX = paiHex->m_iX;
			hex.Y( hex.Wrap(paiHex->m_iY - pGameData->m_iStructureSize) );
			*piY = hex.Y();
			break;
		// northeast
		case 1:
			hex.X( hex.Wrap(paiHex->m_iX + pGameData->m_iStructureSize) );
			*piX = hex.X();
			hex.Y( hex.Wrap(paiHex->m_iY - pGameData->m_iStructureSize) );
			*piY = hex.Y();
			break;
		// east
		case 2:
			hex.X( hex.Wrap(paiHex->m_iX + pGameData->m_iStructureSize) );
			*piX = hex.X();
			*piY = paiHex->m_iY;
			break;
		// southeast
		case 3:
			hex.X( hex.Wrap(paiHex->m_iX + pGameData->m_iStructureSize) );
			*piX = hex.X();
			hex.Y( hex.Wrap(paiHex->m_iY + pGameData->m_iStructureSize) );
			*piY = hex.Y();
			break;
		// south
		case 4:
			*piX = paiHex->m_iX;
			hex.Y( hex.Wrap(paiHex->m_iY + pGameData->m_iStructureSize) );
			*piY = hex.Y();
			break;
		// southwest
		case 5:
			hex.X( hex.Wrap(paiHex->m_iX - pGameData->m_iStructureSize) );
			*piX = hex.X();
			hex.Y( hex.Wrap(paiHex->m_iY + pGameData->m_iStructureSize) );
			*piY = hex.Y();
			break;
		// west
		case 6:
			hex.X( hex.Wrap(paiHex->m_iX - pGameData->m_iStructureSize) );
			*piX = hex.X();
			*piY = paiHex->m_iY;
			break;
		// northwest
		case 7:
			hex.X( hex.Wrap(paiHex->m_iX - pGameData->m_iStructureSize) );
			*piX = hex.X();
			hex.Y( hex.Wrap(paiHex->m_iY - pGameData->m_iStructureSize) );
			*piY = hex.Y();
			break;
		default:
			*piX = paiHex->m_iX;
			*piY = paiHex->m_iY;
			break;
	}
}

//
// determine if the hex passed is within the bounds of the area
// formed by hcStart -> hcEnd, inclusive and considering wrapping
//
BOOL CAIStart::IsHexInStartArea( CHexCoord& hex )
{
	CHexCoord hexIn;

	int x,y;
	for( y=0; y<m_iBlockSize; ++y )
	{
		hexIn.Y( hexIn.Wrap( (m_iBaseY + y) ) );

		for( x=0; x<m_iBlockSize; ++x )
		{
			hexIn.X( hexIn.Wrap( (m_iBaseX + x) ) );
			
			if( hexIn == hex )
				return( TRUE );
		}
	}
	return( FALSE );
}

//
// convert the passed hex location into an offset
// to access the map status words array
//
int CAIStart::GetMapOffset( int iX, int iY )
{
	// BUGBUG this calculation needs to be proved
	int i = (iY - m_iBaseY) * m_iBlockSize;
	i += (iX - m_iBaseX);
	return( i );
}

void CAIStart::OffsetToXY( int iOffset, int *piX, int *piY )
{
	int iX,iY;
	// convert map array offset to game map X,Y
	if( iOffset >= m_iBlockSize )
	{
		// calculate relative to this map
		iY = iOffset / m_iBlockSize;
		iX = iOffset - (iY * m_iBlockSize);
	}
	else
	{
		iY = 0;
		iX = iOffset;
	}
	// convert to game map
	CHexCoord hex(0,0);
	hex.X( hex.Wrap(m_iBaseX+iX) );
	hex.Y( hex.Wrap(m_iBaseY+iY) );
	*piX = hex.X();
	*piY = hex.Y();
	//*piX = iX;
	//*piY = iY;
}

void CAIStart::LoadGroupHexes( int isx, int isy )
{
	// start hex of group, use for wrap and inc
	CHexCoord hexLoad( isx, isy );
	CAIHex *paiLoad = NULL;
	// counter for group hexes
	int i=0; 
	// now load group hexes based on start/end
	for( int iY=0; iY<pGameData->m_iStructureSize; ++iY )
	{
		for( int iX=0; iX<pGameData->m_iStructureSize; ++iX )
		{
			if( i < m_iGrpSize )
			{
				paiLoad = &m_pgrpHexs[i++];
				paiLoad->m_iX = hexLoad.X();
				paiLoad->m_iY = hexLoad.Y();
			}
			else 
				break;

			// advance to next col
			hexLoad.Xinc();
		}
		// advance to next row
		hexLoad.Yinc();
		// reset to first col
		hexLoad.X( isx );
	}
}

void CAIStart::ClearGroupHexes()
{
	CAIHex *paiHex = NULL;
	for( int i=0; i<m_iGrpSize; ++i )
	{
		paiHex = &m_pgrpHexs[i];

		paiHex->m_iX = 0;
		paiHex->m_iY = 0;
		paiHex->m_iUnit = 0;
		paiHex->m_dwUnitID = (DWORD)0;
		paiHex->m_cTerrain = (BYTE)0xFF;
	}
}

CAIStart::~CAIStart()
{
	delete [] m_pgrpHexs;
	delete [] m_pwaSeen;
	delete [] m_pmnHexs;
}

// end of CAIStart.cpp
