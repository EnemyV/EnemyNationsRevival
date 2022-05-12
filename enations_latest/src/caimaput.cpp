////////////////////////////////////////////////////////////////////////////
//
//  CAIMapUt.cpp : CAIMapUtil object declaration
//                 Divide and Conquer AI
//               
//  Last update:    07/25/97
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CAIMapUt.hpp"
#include "CPathMap.h"
#include "CAIData.hpp"

#include "logging.h"	// dave's logging system

extern CAIData *pGameData;		// pointer to API object for game data
extern CException *pException;	// standard exception for yielding
extern CPathMap thePathMap;		// the map pathfinding object (no yield)
extern CPathMgr thePathMgr;		// the actual pathfinding object
extern CRITICAL_SECTION cs;			// used by threads

#define new DEBUG_NEW
#define MAX_MINERAL_DENSITY	128

CAIHex *CAIMapUtil::CreatePathPoints( int iSX, int iSY, int iEX, int iEY )
{
	// now create a CAIHex array to return, it will be cleaned up
	// by the goal manager that processes it for the task manager
	CAIHex *pHexPath = new CAIHex[NUM_PATROL_POINTS];
	if( pHexPath == NULL )
		return( NULL );

	CAIHex *pHex = NULL;
	for( int i=0; i<NUM_PATROL_POINTS; ++i )
	{
		pHex = &pHexPath[i];
		pHex->m_iX = 0;
		pHex->m_iY = 0;
		pHex->m_iUnit = 0xFFFF;
		pHex->m_dwUnitID = 0;
		pHex->m_cTerrain = (BYTE)0xFF;
	}

	pHex = &pHexPath[0];
	pHex->m_iX = iSX;
	pHex->m_iY = iSY;

	pHex = &pHexPath[1];
	pHex->m_iX = iEX;
	pHex->m_iY = iSY;

	pHex = &pHexPath[2];
	pHex->m_iX = iEX;
	pHex->m_iY = iEY;

	pHex = &pHexPath[3];
	pHex->m_iX = iSX;
	pHex->m_iY = iEY;

	return( pHexPath );
}

WORD CAIMapUtil::GetStatus( int iAt )
{
	if( iAt < m_iMapSize )
		return( m_pMap[m_iMapSize] );
	return( m_iMapSize );
}


#if 0
BOOL CAIMapUtil::FindAdjacentHex( int iBldg, CAIHex *pBaseHex, 
	int iWidthX, int iWidthY, CHexCoord& hexFound )
{
	// first, for each building of this type, 
	// that belongs to this AI player, 
	// rate its adjacent cells (considering group size of building)
	// for building multiplier and distance from pBaseHex

	// find out the size of this building
	int iBldgW, iBldgH;
	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData != NULL )
	{
		iBldgW = pBldgData->GetCX();
		iBldgH = pBldgData->GetCY();
	}
	else
		return FALSE;

	// and the build multiplier
	CHexCoord hFrom;
	CHexCoord hTo( pBaseHex->m_iX, pBaseHex->m_iY );
	int iNearDist = m_iMapSize*2;
	int iNearHex = m_iMapSize;
	CAIHex aiHex;

	// start with all cells == 0
	ClearWorkMap();
	int iCnt = 0;

	// for each unit of this map
	//    if this building then rate dist + build modifier
	//    for all adjacent, non-building, occupy-able hexes
	// 
	POSITION pos = m_plUnits->GetHeadPosition();
	while( pos != NULL )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// if this is a building
			if( pUnit->GetType() == CUnit::building )
			{
				// BUGBUG change to use a value in CAIUnit
				// get access to CBuilding copied data for building
				CAICopy *pCopyCBuilding = pUnit->GetCopyData( 
					CAICopy::CBuilding );
				if( pCopyCBuilding != NULL )
				{

					// is this a member of the group of type of
					// the building we want?
					if( pCopyCBuilding->m_aiDataOut[CAI_BUILDTYPE]
						== pBldgData->GetBldgType() )
					{
					// get location of building
					aiHex.m_iX = pCopyCBuilding->m_aiDataOut[CAI_LOC_X];
					aiHex.m_iY = pCopyCBuilding->m_aiDataOut[CAI_LOC_Y];
					}
					else
						continue;
				}
				else // this may be placement
				{
					if( pUnit->GetParam(CAI_TYPEBUILD)
						== pBldgData->GetBldgType() )
					{
						// get location of building
						aiHex.m_iX = pUnit->GetParam(CAI_LOC_X);
						aiHex.m_iY = pUnit->GetParam(CAI_LOC_Y);
					}
					else
						continue;
				}
			// if this unit is the iBldg we are looking for
			// then 
			// for adjacent, empty hexes that have not been
			// rated (zero)
			//   get dist to pBaseHex and build modifier
			//   and store it in m_pwaWork

				RateAdjacentCells( &aiHex, iBldgW, iBldgH,
					iWidthX, iWidthY, pBaseHex );
				iCnt++;
				int i = GetMapOffset( aiHex.m_iX, aiHex.m_iY );
				if( i < m_iMapSize )
					i = m_pwaWork[i];
			}
		}
	}
	// no buildings found of the type we are interested in
	if( !iCnt )
		return FALSE;

	// now find the lowest rating
	iNearDist = m_iMapSize;
	iNearHex = m_iMapSize;
	iCnt = 0;
	// process the work array to do this
	for( int i=0; i<m_iMapSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		// AI's view reports something there
		if( m_pMap[i] )
		{
			m_pwaWork[i] = 0;
			continue;
		}

		// flagged as a valid, adjacent hex
		if( m_pwaWork[i] )
		{
			// closest check
			if( m_pwaWork[i] < iNearDist )
			{
				iNearDist = m_pwaWork[i];
				iNearHex = i;
				iCnt = 0;
			}
			if( m_pwaWork[i] == iNearDist )
				iCnt++;
		}
	}

	// early out, with only one best
	if( iCnt == 1 )
	{
		OffsetToXY( iNearHex, &aiHex.m_iX, &aiHex.m_iY );
		hexFound.X( aiHex.m_iX );
		hexFound.Y( aiHex.m_iY );

		return TRUE;
	}

	// if still here, there are many best, so clear all that
	// are not equal to the best and count the best
	iCnt = 0;
	for( i=0; i<m_iMapSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		if( m_pwaWork[i] == iNearDist )
		{
			iNearHex = i;
			iCnt++;
		}
		else 
			m_pwaWork[i] = 0;
	}

	// more than one was good, so pick one
	if( iCnt > 1 )
	{
		iNearHex = pGameData->GetRandom( iCnt );
		iNearHex = FindNth( iNearHex );
	}
	OffsetToXY( iNearHex, &aiHex.m_iX, &aiHex.m_iY );
	hexFound.X( aiHex.m_iX );
	hexFound.Y( aiHex.m_iY );
	return TRUE;
}
#endif

//
// find the hex that is closest to the base hex passed
// that will support the building passed
//
void CAIMapUtil::FindCentralHex( CAIHex *pBaseHex, 
	int iWidthX, int iWidthY, CHexCoord& hexFound )
{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nFindCentralHex() player %d for w=%d h=%d ", 
    	m_iPlayer, iWidthX, iWidthY );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"closest to %d,%d \n", pBaseHex->m_iX, pBaseHex->m_iY );
#endif

	ClearWorkMap();

	// create an array of the hexes that are width x 4
	// from the base hex in all directions considering
	// block boundaries

	// create AI copy to use for hexes
	CAIHex aiHex( 0, 0 );
	int iCnt = 0;
	CHexCoord hFrom;
	CHexCoord hTo( pBaseHex->m_iX, pBaseHex->m_iY );
	// set starting hex of the array
	CHexCoord hcStart,hcEnd,hex;

	if( hTo.X() < pGameData->m_iHexPerBlk )
	{
		hcStart.X(0);
		hcEnd.X(pGameData->m_iHexPerBlk);
	}
	else
	{
		hcStart.X( (hTo.X()/pGameData->m_iHexPerBlk) * pGameData->m_iHexPerBlk );
		hcEnd.X( hcStart.X() + pGameData->m_iHexPerBlk );
	}
	if( hTo.Y() < pGameData->m_iHexPerBlk )
	{
		hcStart.Y(0);
		hcEnd.Y(pGameData->m_iHexPerBlk);
	}
	else
	{
		hcStart.Y( (hTo.Y()/pGameData->m_iHexPerBlk) * pGameData->m_iHexPerBlk );
		hcEnd.Y( hcStart.Y() + pGameData->m_iHexPerBlk );
	}

	int iDeltaX = hex.Wrap(hcEnd.X() - hcStart.X());
	int iDeltaY = hex.Wrap(hcEnd.Y() - hcStart.Y());

	int iNearDist = m_iMapSize;
	int iNearHex = m_iMapSize;

	CMinerals *pmn;

	for( int iy=0; iy<iDeltaY; iy++ )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iy) );

		for( int ix=0; ix<iDeltaX; ix++ )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			hex.X( hex.Wrap(hcStart.X() + ix) );

			// do not allow placing on 0,0
			if( !hex.X() && !hex.Y() )
				continue;

			aiHex.m_iY=hex.Y();
			aiHex.m_iX=hex.X();

			int i = GetMapOffset( aiHex.m_iX, aiHex.m_iY );
			if( i < 0 || i >= m_iMapSize )
				continue;

			if( m_pMap[i] & MSW_RESOURCE )
				continue;

			// check hex for material
			if( theMinerals.Lookup(hex, pmn) )
				continue;

			// use the game's opinion of building this building
			// int CGameMap::FoundationCost (CHexCoord const & hex, int iType, int iDir, 
			// CVehicle const * pVeh, int * piAlt, int * piWhy) const
			if( theMap.FoundationCost(hex, 
				CStructureData::rocket, 0, (CVehicle const *)-1) < 0 )
				continue;

			// make sure hex is buffersize away from other buildings
			if( NearBuildings( hex, iWidthX, iWidthY, FALSE ) )
				continue;
			
			// check each hex and its width group
			// for known, building, vehicle, terrain
			// and exclude any hex if its width group failed
			if( AreHexesOpen( MAX_ADJACENT, hex, 
				iWidthX, iWidthY ) )
			{
				iCnt++;

				// convert from CAIHex to CHexCoord
				//hFrom.X( aiHex.m_iX );
				//hFrom.Y( aiHex.m_iY );
				hFrom = hex;
				// rate using distance from base + building multiplier
				int iDist = pGameData->GetRangeDistance( hFrom, hTo );
				// stay at least buffersize away
				if( iDist < m_iBufferSize )
					iDist = 0;

				// store distance to base
				
				// store rating 
				if( i < m_iMapSize && i >= 0 )
				{
					m_pwaWork[i] = iDist;
			
					if( iDist && iDist < iNearDist )
					{
						iNearDist = iDist;
						iNearHex = i;
					}
				}
			}
		}
	}

	// if no hexes were open, then return now
	if( !iCnt )
		return;

	// double the iNearDist
	iNearDist <<= 1;

	// reprocess limited array, this time clearing
	// hexes with > nearest distance 
	for( iy=0; iy<iDeltaY; iy++ )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iy) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
		myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( int ix=0; ix<iDeltaX; ix++ )
		{
			hex.X( hex.Wrap(hcStart.X() + ix) );

			// do not allow placing on 0,0
			if( !hex.X() && !hex.Y() )
				continue;

			int i = GetMapOffset( hex.X(), hex.Y() );
			if( i < 0 || i >= m_iMapSize )
				continue;

			if( !m_pwaWork[i] )
				continue;

			// reset those hexes with higher ratings
			if( m_pwaWork[i] > iNearDist )
				m_pwaWork[i] = 0;
			else
			{
				// get build multiplier
				m_pwaWork[i] = 
				GetMultiplier( hex, iWidthX, iWidthY, CStructureData::city );
			}
		}
	}

	iCnt = 0;
	iNearDist = m_iMapSize;
	iNearHex = m_iMapSize;
	// then process for the lowest building multiplier
	for( iy=0; iy<iDeltaY; iy++ )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iy) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
		myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( int ix=0; ix<iDeltaX; ix++ )
		{
			hex.X( hex.Wrap(hcStart.X() + ix) );

			// do not allow placing on 0,0
			if( !hex.X() && !hex.Y() )
				continue;

			int i = GetMapOffset( hex.X(), hex.Y() );
			if( i < 0 || i >= m_iMapSize )
				continue;

			if( !m_pwaWork[i] )
				continue;

			// looking for lowest building multiplier
			if( m_pwaWork[i] > 0 && m_pwaWork[i] < iNearDist )
				iNearDist = m_pwaWork[i];
		}
	}

	// reprocess limited array, this time clearing
	// hexes with > best building multiplier
	for( iy=0; iy<iDeltaY; iy++ )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iy) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
		myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( int ix=0; ix<iDeltaX; ix++ )
		{
			hex.X( hex.Wrap(hcStart.X() + ix) );

			// do not allow placing on 0,0
			if( !hex.X() && !hex.Y() )
				continue;

			int i = GetMapOffset( hex.X(), hex.Y() );
			if( i < 0 || i >= m_iMapSize )
				continue;

			if( !m_pwaWork[i] )
				continue;

			// reset those hexes with higher ratings
			if( m_pwaWork[i] > iNearDist )
				m_pwaWork[i] = 0;
		}
	}

	// reset to count the nearest open hexes
	iCnt = 0;

	// count those who are <= nearest distance
	for( iy=0; iy<iDeltaY; iy++ )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iy) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
		myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( int ix=0; ix<iDeltaX; ix++ )
		{
			hex.X( hex.Wrap(hcStart.X() + ix) );

			// do not allow placing on 0,0
			if( !hex.X() && !hex.Y() )
				continue;

			int i = GetMapOffset( hex.X(), hex.Y() );
			if( i < 0 || i >= m_iMapSize )
				continue;

			if( m_pwaWork[i] )
			{
				iCnt++;
				iNearHex = i;
			}
		}
	}

	// if more than one hex was nearest, then pick one
	if( iCnt > 1 )
	{
		iNearHex = pGameData->GetRandom( iCnt );
		iNearHex = FindNth( iNearHex );
	}
	
	// if hex is found, return
	if( iCnt )
	{
		OffsetToXY( iNearHex, &aiHex.m_iX, &aiHex.m_iY );
		hexFound.X( aiHex.m_iX );
		hexFound.Y( aiHex.m_iY );
		return;
	}

	// if still here then use brute force,
	// and check the entire m_pwaWork[]
	// check each hex and its width group
	// for known, building, vehicle, terrain
	// and exclude any hex if its width group failed
	// rate using distance from base + building multiplier
	iCnt = 0;
	iNearDist = m_iMapSize;
	iNearHex = m_iMapSize;

	for( int i=0; i<m_iMapSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		OffsetToXY( i, &aiHex.m_iX, &aiHex.m_iY );

		// use the game's opinion of building this building
		hex.X( aiHex.m_iX );
		hex.Y( aiHex.m_iY );

		m_pwaWork[i] = 0;

		// do not allow placing on 0,0
		if( !hex.X() && !hex.Y() )
			continue;

		if( theMap.FoundationCost(hex, 
			CStructureData::rocket, 0, (CVehicle const *)-1) < 0 )
			continue;

		// make sure hex is buffersize away from other buildings
		if( NearBuildings( hex, iWidthX, iWidthY, FALSE ) )
			continue;
			
		// check each hex and its width group
		// for known, building, vehicle, terrain
		// and exclude any hex if its width group failed
		if( AreHexesOpen( MAX_ADJACENT, hex, 
			iWidthX, iWidthY ) )
		{
			iCnt++;

			// convert from CAIHex to CHexCoord
			//hFrom.X(aiHex.m_iX);
			//hFrom.Y(aiHex.m_iY);
			hFrom = hex;
			// rate using distance from base + building multiplier
			int iDist = 
				pGameData->GetRangeDistance( hFrom, hTo )
				+ GetMultiplier( hFrom, iWidthX, iWidthY, CStructureData::city );

			// store distance to base + build multplier
			if( iDist && iDist < iNearDist )
			{
				iNearDist = iDist;
				iNearHex = i;
			}

			// store rating 
			if( i < m_iMapSize )
				m_pwaWork[i] = iDist;
		}
	}

	// if no hexes were open, then return now
	if( !iCnt )
		return;

	// reset to count the nearest open hexes
	iCnt = 0;

	for( i=0; i<m_iMapSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		// count those hexes with best rating
		if( m_pwaWork[i] == iNearDist )
			++iCnt;
		else
			m_pwaWork[i] = 0; // reset others for FindNth()
	}
	// nothing found
	if( !iCnt )
		return;

	// if more than one hex was nearest, then pick one
	if( iCnt > 1 )
	{
		iNearHex = pGameData->GetRandom( iCnt );
		iNearHex = FindNth( iNearHex );
	}
	
	OffsetToXY( iNearHex, &aiHex.m_iX, &aiHex.m_iY );
	hexFound.X(aiHex.m_iX);
	hexFound.Y(aiHex.m_iY);
}

//
// NOTE: this is a general building placement process
//
void CAIMapUtil::FindSectionHex( int iBldg, //CAIHex *pBaseHex, 
	int iWidthX, int iWidthY, CHexCoord& hexFound )
{
	//ASSERT_VALID( m_pwaWork );

/*
		enum { city,								// this is returned by GetType ()
				rocket,
				apartment_1_1,			// #=frontier/estab/city, #=choice
				apartment_1_2,
				apartment_2_1,
				apartment_2_2,
				apartment_2_3,
				apartment_2_4,
				apartment_3_1,
				apartment_3_2,
				office_2_1,
				office_2_2,
				office_2_3,
				office_2_4,
				office_3_1,
				office_3_2,
				// above are only placed by computer
				barracks_2,
				barracks_3, // obsolete
				command_center,
				embassy,
				farm,
				fort_1,
				fort_2,
				fort_3,
				heavy,
				light_1,
				light_2,
				lumber,
				oil_well,
				refinery,
				coal,
				iron,
				copper,
				power_1,
				power_2,
				power_3,
				research,
				repair,
				seaport,
				shipyard_1,
				shipyard_3,
				smelter,
				warehouse,
				light_0,
				num_types };
	enum { first_show = barracks_2 };
*/
	// BUGBUG now building entrance orientation needs to be
	// considered in placement selection!

	// BUGBUG need to test on something other than the building
	// type because they are all now level specific for a given
	// type of building (ie. light_1, light_2, light_3 instead
	// of light for a light weapons factory).
	//
	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData == NULL )
		return;

	switch( pBldgData->GetBldgType() )
	{
		case CStructureData::farm:	// closest known, lowest GetBuildMult()
		case CStructureData::lumber:	// and lowest GetFarmMult() for farm
								// CHex::GetType() == CHex::forest 
								// for lumber
			FindHexOnTerrain( iBldg, iWidthX, iWidthY, hexFound );
			break;
		case CStructureData::oil_well:
		case CStructureData::coal:	// closest known, lowest GetBuildMult()
		case CStructureData::iron:	// with appropriate CMaterialTypes::
		case CStructureData::copper:
			FindHexOnMaterial( iBldg, iWidthX, iWidthY, hexFound );
			break;
		case CStructureData::seaport: // closest known, lowest GetBuildMult()
		case CStructureData::shipyard:// with CHex::ocean adjacent
			FindHexByWater( iBldg, iWidthX, iWidthY, hexFound );
			break;
		case CStructureData::fort:   // on route from OPFOR buildings
								//
		case CStructureData::embassy:// adjacent OPFOR command_center
			FindSpecialHex( iBldg, iWidthX, iWidthY, hexFound );
			break;
		default:
			// handle all other types of buildings based on
			// the building to be built and make sure it is
			// within the bounds of the city (planned roads)
			FindHexInCity( iBldg, iWidthX, iWidthY, hexFound );
			break;
	}
}

//
// pick 4 ocean hexes, relative to iGoal (IDG_SHORES, IDG_SEAWAR)
// and return a CHexCoord array that contains them
//
CAIHex *CAIMapUtil::FindWaterPatrol( int iGoal )
{
	CHexCoord hcBase,hcStart,hcEnd,hcRay;
	//hcBase.X( m_iBaseX );
	//hcBase.Y( m_iBaseY );
	hcBase = m_RocketHex;

	int i, iDist, iCnt=0, iBest=m_iMapSize, iY, iX;

	hcStart.X( hcRay.Wrap(hcBase.X() - pGameData->m_iHexPerBlk) );
	hcStart.Y( hcRay.Wrap(hcBase.Y() - pGameData->m_iHexPerBlk) );
	hcEnd.X( hcRay.Wrap(hcBase.X() + pGameData->m_iHexPerBlk) );
	hcEnd.Y( hcRay.Wrap(hcBase.Y() + pGameData->m_iHexPerBlk) );

	int iDeltaX = hcRay.Wrap(hcEnd.X() - hcStart.X());
	int iDeltaY = hcRay.Wrap(hcEnd.Y() - hcStart.Y());

	for( iY=0; iY<iDeltaY; iY++)
	{
		hcRay.Y( hcRay.Wrap(hcStart.Y() + iY) );

		for( iX=0; iX<iDeltaX; iX++)
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			hcRay.X( hcRay.Wrap(hcStart.X() + iX) );
			
			i = GetMapOffset( hcRay.X(), hcRay.Y() );
			if( i >= m_iMapSize )
				continue;

			m_pwaWork[i] = 0; // rating goes in here

			// get terrain from game data
			CHex *pGameHex = theMap.GetHex( hcRay );
			if( pGameHex == NULL )
				continue;
			// and if it is ocean, save hex and stop this direction
			if( pGameHex->GetType() != CHex::ocean &&
				pGameHex->GetType() != CHex::lake )
				continue;

			// find ocean with altitude > 11
			if( iGoal == IDG_SHORES )
			{
				if( pGameHex->GetAlt() > 11 )
				{
					iDist = pGameData->GetRangeDistance(hcBase,hcRay);
					m_pwaWork[i] = iDist;
					//iCnt++;
				}
#if 1
				if( iDist && iDist < iBest ) // get closer hexes
				{
					iBest = iDist;
					iCnt = 0;
				}
				if( iBest == iDist )
					iCnt++;
#endif
			}
			// find ocean with altitude < 8
			else if( iGoal == IDG_SEAWAR )
			{
				if( pGameHex->GetAlt() < 8 )
				{
					iDist = pGameData->GetRangeDistance(hcBase,hcRay);
					m_pwaWork[i] = iDist;
					//iCnt++;
				}
#if 1
				if( iDist && iDist > iBest ) // get farther away hexes
				{
					iBest = iDist;
					iCnt = 0;
				}
				if( iBest == iDist )
					iCnt++;
#endif
			}
		}
	}

	// now this section of the work map has zero values in non-passible
	// ocean or land terrain and non-zero values representing distance
	// from the current base hex
	//
	// IDG_SHORES - want patroling closest to base hexes - iBest is low
	// IDG_SEAWAR - want patroling away from base hexes - iBest is high
	// 

	// need to reprocess with a lower threshhold
	if( iCnt < NUM_PATROL_POINTS )
	{
		iCnt = 0;

		if( iGoal == IDG_SHORES )
			iBest += iBest/2; // increase by half
		else if( iGoal == IDG_SEAWAR )
			iBest = iBest/2; // lower by half

		for( iY=0; iY<iDeltaY; iY++)
		{
			hcRay.Y( hcRay.Wrap(hcStart.Y() + iY) );

			for( iX=0; iX<iDeltaX; iX++)
			{
#if THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				hcRay.X( hcRay.Wrap(hcStart.X() + iX) );
			
				i = GetMapOffset( hcRay.X(), hcRay.Y() );
				if( i >= m_iMapSize )
					continue;

				if( !m_pwaWork[i] )
					continue;

				// want hexes closer, but threshhold has be increased
				// and we will now add hexes that were farther away
				if( iGoal == IDG_SHORES )
				{
					if( m_pwaWork[i] > iBest )
					{
					m_pwaWork[i] = 0;
					continue;
					}
				}
				// want hexes farther away, but threshhold has been reduced
				// and we will now add hexes that are closer in
				else if( iGoal == IDG_SEAWAR )
				{
					if( m_pwaWork[i] < iBest )
					{
					m_pwaWork[i] = 0;
					continue;
					}
				}
				iCnt++;
			}
		}
	}

	// pick NUM_PATROL_POINTS number of points by finding those
	// points at the most extreme edges of the collection
	// or figure out a way to pick from them
	if( iCnt > NUM_PATROL_POINTS )
	{
		// create a hex array with no points
		CAIHex *pHexPath = CreatePathPoints( 0,0,0,0 );
		if( pHexPath == NULL )
			return( NULL );
		
		// take'em and run and worry about getting there later
		for( iY=0; iY<iDeltaY; iY++)
		{
			hcRay.Y( hcRay.Wrap(hcStart.Y() + iY) );

			for( iX=0; iX<iDeltaX; iX++)
			{
#if THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				hcRay.X( hcRay.Wrap(hcStart.X() + iX) );
			
				i = GetMapOffset( hcRay.X(), hcRay.Y() );
				if( i >= m_iMapSize )
					continue;

				if( !m_pwaWork[i] )
					continue;

				if( iGoal == IDG_SHORES )
				{
					// looking for closest hexes
					if( m_pwaWork[i] > iBest )
						continue;
				}
				else if( iGoal == IDG_SEAWAR )
				{
					// looking for hexes farther away
					if( m_pwaWork[i] < iBest )
						continue;
				}
				else
					continue;

				// now run thru the patrol points array and
				// find the qualifying point at the most extreme
				// edge to the area corners
				CHexCoord hcTest;
				int iDist;
				for( int j=0; j<NUM_PATROL_POINTS; ++j )
				{
#if THREADS_ENABLED
					// BUGBUG this function must yield
					myYieldThread();
					//if( myYieldThread() == TM_QUIT )
					//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
					CAIHex *pHex = &pHexPath[j];

					switch(j)
					{
						case 0:
							hcTest.X( hcStart.X() );
							hcTest.Y( hcStart.Y() );
							break;
						case 1:
							hcTest.X( hcEnd.X() );
							hcTest.Y( hcStart.Y() );
							break;
						case 2:
							hcTest.X( hcEnd.X() );
							hcTest.Y( hcEnd.Y() );
							break;
						case 3:
							hcTest.X( hcStart.X() );
							hcTest.Y( hcEnd.Y() );
							break;
						default:
							delete [] pHexPath;
							return( NULL );
					}
					iDist =
						pGameData->GetRangeDistance( hcRay, hcTest );

					// since we are testing against the edges of the area
					// we want hexes that are closest to the edge tested
					if( iDist && iDist < pHex->m_iUnit )
					{
						pHex->m_iUnit = iDist;
						pHex->m_iX = hcRay.X();
						pHex->m_iY = hcRay.Y();
					}
				}
			}
		}
		return( pHexPath );
	}
	else if( !iCnt )	// Can't patrol here
	{
		return( NULL );
	}
	// iCnt is non-zero  and <= NUM_PATROL_POINTS
	else if( iCnt <= NUM_PATROL_POINTS )
	{
		// create a hex array with no points
		CAIHex *pHexPath = CreatePathPoints(0, 0, 0, 0 );
		if( pHexPath == NULL )
			return( NULL );
		
		// take'em and run and worry about getting there later
		int j=0;
		for( iY=0; iY<iDeltaY; iY++)
		{
			hcRay.Y( hcRay.Wrap(hcStart.Y() + iY) );

			for( iX=0; iX<iDeltaX; iX++)
			{
#if THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				hcRay.X( hcRay.Wrap(hcStart.X() + iX) );
			
				i = GetMapOffset( hcRay.X(), hcRay.Y() );
				if( i >= m_iMapSize )
					continue;

				if( !m_pwaWork[i] )
					continue;

				if( iGoal == IDG_SHORES )
				{
					// looking for closest hexes
					if( m_pwaWork[i] > iBest )
						continue;
				}
				else if( iGoal == IDG_SEAWAR )
				{
					// looking for hexes farther away
					if( m_pwaWork[i] < iBest )
						continue;
				}
				else
					continue;

				// this is what we came for
				if( iCnt-- )
				{
					CAIHex *pHex = &pHexPath[j++];
					pHex->m_iX = hcRay.X();
					pHex->m_iY = hcRay.Y();
				}
			}
		}
		// there were iBest qualifying hexes, something is wrong
		if( !j )
		{
			delete [] pHexPath;
			return( NULL );
		}
		return( pHexPath );
	}
	return( NULL );
}

//
// pick 4 patrol points that protect the hexAt and can be reached
// by the ptdShip
//
CAIHex *CAIMapUtil::FindWaterPatrol(CHexCoord& hexAt, 
	CTransportData const *ptdShip )
{
	// determine reference point
	CHexCoord hcBase,hcStart,hcEnd,hcRay;
	int iCnt=0;

	// create area around hexAt that is equal to city width/height
	int iDeltaY = abs( hcStart.Diff( m_iCityEY-m_iCitySY ));
	int iDeltaX = abs( hcStart.Diff( m_iCityEX-m_iCitySX ));
	int i,j,iDist,iBest=0,iX,iY;

	// set initial size of patrol area by using delta in all directions
	hcStart.X( hcRay.Wrap((hexAt.X() - (iDeltaX/2))) );
	hcStart.Y( hcRay.Wrap((hexAt.Y() - (iDeltaY/2))) );
	hcEnd.X( hcRay.Wrap((hexAt.X() + (iDeltaX/2))) );
	hcEnd.Y( hcRay.Wrap((hexAt.Y() + (iDeltaY/2))) );

	iDeltaX = abs( hcRay.Diff( hcEnd.X() - hcStart.X() ));
	iDeltaY = abs( hcRay.Diff( hcEnd.Y() - hcStart.Y() ));

	for( iY=0; iY<iDeltaY; iY++)
	{
		hcRay.Y( hcRay.Wrap(hcStart.Y() + iY) );

		for( iX=0; iX<iDeltaX; iX++)
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			hcRay.X( hcRay.Wrap(hcStart.X() + iX) );
			
			i = GetMapOffset( hcRay.X(), hcRay.Y() );
			if( i >= m_iMapSize )
				continue;

			m_pwaWork[i] = 0; // rating goes in here

			if( !hcRay.X() && !hcRay.Y() )
				continue;

			// get terrain from game data
			CHex *pGameHex = theMap.GetHex( hcRay );
			if( pGameHex == NULL )
				continue;
			// and if it is not ocean, skip it
			if( pGameHex->GetType() != CHex::ocean &&
				pGameHex->GetType() != CHex::lake )
				continue;
			// and if ship can't travel it, skip it
			if( !ptdShip->CanTravelHex(pGameHex) )
				continue;

			iDist = pGameData->GetRangeDistance( hcRay, hexAt );
			m_pwaWork[i] = iDist;

			if( iDist && iDist > iBest ) // get farther away hexes
			{
				iBest = iDist;
				iCnt = 0;
			}
			if( iBest == iDist )
				iCnt++;
		}
	}

	// need to reprocess with a lower threshhold
	if( iCnt < NUM_PATROL_POINTS )
	{
		iCnt = 0;
		iBest /= 2; // decrease by half

		for( iY=0; iY<iDeltaY; iY++)
		{
			hcRay.Y( hcRay.Wrap(hcStart.Y() + iY) );

			for( iX=0; iX<iDeltaX; iX++)
			{
#if THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				hcRay.X( hcRay.Wrap(hcStart.X() + iX) );
			
				i = GetMapOffset( hcRay.X(), hcRay.Y() );
				if( i >= m_iMapSize )
					continue;

				if( !m_pwaWork[i] )
					continue;

				if( m_pwaWork[i] < iBest )
				{
					m_pwaWork[i] = 0;
					continue;
				}
				iCnt++; // counting hexes above new threshhold
			}
		}
	}

	// can't patrol this point
	if( !iCnt )
		return( NULL );

	// create a hex array with no points
	CAIHex *pHexPath = CreatePathPoints(0, 0, 0, 0 );
	if( pHexPath == NULL )
		return( NULL );

	// there are more qualifying points than we need
	if( iCnt > NUM_PATROL_POINTS )
	{
		for( iY=0; iY<iDeltaY; iY++)
		{
			hcRay.Y( hcRay.Wrap(hcStart.Y() + iY) );

			for( iX=0; iX<iDeltaX; iX++)
			{
#if THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				hcRay.X( hcRay.Wrap(hcStart.X() + iX) );
			
				i = GetMapOffset( hcRay.X(), hcRay.Y() );
				if( i >= m_iMapSize )
					continue;

				if( !m_pwaWork[i] )
					continue;

				// now run thru the patrol points array and
				// find the qualifying point at the most extreme
				// edge to the area corners
				CHexCoord hcTest;
				int iDist;
				for( j=0; j<NUM_PATROL_POINTS; ++j )
				{
#if THREADS_ENABLED
					// BUGBUG this function must yield
					myYieldThread();
					//if( myYieldThread() == TM_QUIT )
					//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
					CAIHex *pHex = &pHexPath[j];

					switch(j)
					{
						case 0:
							hcTest.X( hcStart.X() );
							hcTest.Y( hcStart.Y() );
							break;
						case 1:
							hcTest.X( hcEnd.X() );
							hcTest.Y( hcStart.Y() );
							break;
						case 2:
							hcTest.X( hcEnd.X() );
							hcTest.Y( hcEnd.Y() );
							break;
						case 3:
							hcTest.X( hcStart.X() );
							hcTest.Y( hcEnd.Y() );
							break;
						default:
							delete [] pHexPath;
							return( NULL );
					}
					iDist =
						pGameData->GetRangeDistance( hcRay, hcTest );
					if( iDist && iDist < pHex->m_iUnit )
					{
						pHex->m_iUnit = iDist;
						pHex->m_iX = hcRay.X();
						pHex->m_iY = hcRay.Y();
					}
				}
			}
		}
	}
	// iCnt is non-zero  and <= NUM_PATROL_POINTS
	else if( iCnt <= NUM_PATROL_POINTS )
	{
		// take'em and run and worry about getting there later
		j=0;
		for( iY=0; iY<iDeltaY; iY++)
		{
			hcRay.Y( hcRay.Wrap(hcStart.Y() + iY) );

			for( iX=0; iX<iDeltaX; iX++)
			{
#if THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				hcRay.X( hcRay.Wrap(hcStart.X() + iX) );
			
				i = GetMapOffset( hcRay.X(), hcRay.Y() );
				if( i >= m_iMapSize )
					continue;

				if( !m_pwaWork[i] )
					continue;

				// this is what we came for
				if( iCnt-- )
				{
					CAIHex *pHex = &pHexPath[j++];
					pHex->m_iX = hcRay.X();
					pHex->m_iY = hcRay.Y();
				}
			}
		}
		// there were iBest qualifying hexes, something is wrong
		if( !j )
		{
			delete [] pHexPath;
			return( NULL );
		}
	}

	// now a path should exist between all points in hcStart,hcEnd
	// so create pHexPath to return
	return( pHexPath );
}

//
// find the hex, nearest to hcFrom, that has CHex::ocean adjacent to it
// and return it in hcFrom
//
void CAIMapUtil::FindNearestWater( CHexCoord& hcFrom, BOOL bReally /*=TRUE*/ )
{
	CHexCoord hexFrom = hcFrom;
	int iCnt, iDist, iDistBest = m_iMapSize;

	// for each direction
	for( int i=0; i<MAX_ADJACENT; ++i )
	{
		// cast in a straight line from the base hex
		// until either water is encountered or blocksize 
		// number of hexes are checked
		CHexCoord hcRay = hexFrom;
		iCnt = 0;

		while( iCnt < pGameData->m_iHexPerBlk )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			iCnt++;

			// move ray based on direction i
			switch(i)
			{
				case 0:	// north
					hcRay.Ydec();
					break;
				case 1:	// northeast
					hcRay.Ydec();
					hcRay.Xinc();
					break;
				case 2:
					hcRay.Xinc();
					break;
				case 3:
					hcRay.Yinc();
					hcRay.Xinc();
					break;
				case 4:
					hcRay.Yinc();
					break;
				case 5:
					hcRay.Yinc();
					hcRay.Xdec();
					break;
				case 6:
					hcRay.Xdec();
					break;
				case 7:
					hcRay.Xdec();
					hcRay.Ydec();
					break;
				default:
					return;
			}
			
			// get terrain from game data
			CHex *pGameHex = theMap.GetHex( hcRay );

			// and if it is ocean, save hex and stop this direction
			if( bReally )
			{
				if( pGameHex->GetType() == CHex::ocean )
				{
					if( !IsWaterAdjacent(hcRay,bReally) )
						continue;

					iDist = pGameData->GetRangeDistance( hexFrom, hcRay );
					if( iDist < iDistBest )
					{
					hcFrom = hcRay;
					iDistBest = iDist;
					break;
					}
					else
						break;
				}
				else 
				{
					// we have passed best distance to water, 
					// and not yet found any in this direction
					iDist = pGameData->GetRangeDistance( hexFrom, hcRay );
					if( iDist > iDistBest )
						break;
				}
			}
			else // no, we really don't want any water
			{
				if( pGameHex->GetType() != CHex::ocean &&
					pGameHex->GetType() != CHex::river &&
					pGameHex->GetType() != CHex::lake )
				{
					// this actually is looking for all land
					// because to be here, bReally must be FALSE
					if( !IsWaterAdjacent(hcRay,bReally) )
						continue;

					iDist = pGameData->GetRangeDistance( hexFrom, hcRay );
					if( iDist < iDistBest )
					{
						hcFrom = hcRay;
						iDistBest = iDist;
						break;
					}
					else
						break;
				}
				else 
				{
					iDist = pGameData->GetRangeDistance( hexFrom, hcRay );
					// we have passed best distance to non-water, 
					// and not yet found any in this direction
					if( iDist > iDistBest )
						break;
				}
			}
		}
	}
}

void CAIMapUtil::FindSpecialHex( int iBldg,
	int iWidthX, int iWidthY, CHexCoord& hexFound )
{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nFindSpecialHex() player %d for a %d ", m_iPlayer, iBldg );
#endif
	// CStructureData::fort_1, _2, _3
	// look for farms, oilwells and mines and refineries
	// always nearest the known enemies
	//
	if( iBldg == CStructureData::fort_1 ||
		iBldg == CStructureData::fort_2 ||
		iBldg == CStructureData::fort_3 )
	{
		// find farms, oilwells, mines and refineries that
		// don't have a fort recorded and take the highest
		// rated one with the fewest forts recorded and then
		// spiral out from cowbuffer distance from it to the
		// buildable hex closest to known enemies rocket
		// otherwise fartherest from player's rocket
		CHexCoord hexRocket;
		if( m_bUseOpFor )
			hexRocket = m_hexOpFor;
		else
		{
			//hexRocket.X( m_iBaseX );
			//hexRocket.Y( m_iBaseY );
			hexRocket = m_RocketHex;
		}

#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

		int iRating = 0;
		int iBestRating = 0;
		int iBestForts = CAI_SIZE;
		CAIUnit *pBestBldg = NULL;
		DWORD dwEnd = theGame.GettimeGetTime () + 20 * 1000;

		// go through the buildings of this map, and of the buildings
		// interested in, find the highest rated one with the lowest
		// number of forts
		POSITION pos = m_plUnits->GetHeadPosition();
		while( pos != NULL )
		{
        	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        	if( pUnit != NULL )
        	{
          		ASSERT_VALID( pUnit );
				if( pUnit->GetOwner() != m_iPlayer )
					continue;

				// if this is a building we are interested in
				switch( pUnit->GetTypeUnit() )
				{
					case CStructureData::oil_well:
					case CStructureData::refinery:
					case CStructureData::smelter:
					case CStructureData::lumber:
					case CStructureData::research:
					case CStructureData::farm:
					case CStructureData::coal:
					case CStructureData::iron:
					case CStructureData::copper:
						//
						// get number of forts assigned to this building
						//
						if( pUnit->GetParam(CAI_ACCURACY) < iBestForts )
						{
							iBestForts = pUnit->GetParam(CAI_ACCURACY);
							iBestRating = 0;	
						}
						//
						// get rating of this building if it has equal forts
						//
						if( iBestForts == pUnit->GetParam(CAI_ACCURACY) )
						{
							iRating = 0;
							EnterCriticalSection (&cs);

							CBuilding *pBuilding =
							theBuildingMap.GetBldg( pUnit->GetID() );
							if( pBuilding != NULL )
								iRating = AssessTarget(pBuilding,0);
							LeaveCriticalSection (&cs);

							// is this building higher rated?
							if( iRating > iBestRating )
							{
								iBestRating = iRating;
								pBestBldg = pUnit;
							}
						}
					default:
						break;
				}
			}
		}

		
		//CHexCoord hexBase( m_iBaseX, m_iBaseY );
		CHexCoord hexBase = m_RocketHex;
		// pBestBldg should be the building to use
		if( pBestBldg != NULL )
		{
			EnterCriticalSection (&cs);
			CBuilding *pBuilding =
				theBuildingMap.GetBldg( pBestBldg->GetID() );
			if( pBuilding != NULL )
				hexBase = pBuilding->GetExitHex();
			LeaveCriticalSection (&cs);

			// increase number of forts assigned to this building
			pBestBldg->SetParam( CAI_ACCURACY,
				pBestBldg->GetParam(CAI_ACCURACY) + 1 );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"FindSpecialHex() based from a %d building at %d,%d ", 
		pBestBldg->GetTypeUnit(), hexBase.X(), hexBase.Y() );
#endif
		}

		// now spiral out
		CHexCoord hcFrom,hcTo,hcAt,hexFort;
		CAIHex aiHex;

		// if enemy is used then best is closest to hexRocket
		if( m_bUseOpFor )
		{
			iBestForts = pGameData->m_iHexPerBlk;
		}
		else // else using own rocket, means best is far from hexRocket
		{
			iBestForts = 0;
		}

		int iMaxSteps = (pGameData->m_iHexPerBlk/2);
		// pGameData->m_iStructureSize + m_iBufferSize away from exit
		// to avoid the building in the 0 & 6 direction
		int iBldgBuffer = pGameData->m_iStructureSize + m_iBufferSize;
		int iStep = 1;
		while( iStep < iMaxSteps )
		{
			hcFrom.X( hexBase.Wrap(hexBase.X()-(iStep+iBldgBuffer)) );
			hcFrom.Y( hexBase.Wrap(hexBase.Y()-(iStep+iBldgBuffer)) );
			hcTo.X( hexBase.Wrap(hexBase.X()+(iStep+m_iBufferSize)) );
			hcTo.Y( hexBase.Wrap(hexBase.Y()+(iStep+m_iBufferSize)) );

			int iDeltax = hexBase.Wrap(hcTo.X()-hcFrom.X());
			int iDeltay = hexBase.Wrap(hcTo.Y()-hcFrom.Y());
			//int iDeltax = abs( hexBase.Diff(hcTo.X()-hcFrom.X()) ) + 1;
			//int iDeltay = abs( hexBase.Diff(hcTo.Y()-hcFrom.Y()) ) + 1;

			myYieldThread();

			for( int iY=0; iY<iDeltay; ++iY )
			{

				hcAt.Y( hexBase.Wrap(hcFrom.Y()+iY) );

				for( int iX=0; iX<iDeltax; ++iX )
				{

				// only get 20 seconds to find it
				if ( theGame.GettimeGetTime () > dwEnd )
					return;

					hcAt.X( hexBase.Wrap(hcFrom.X()+iX) );

					// do not allow placing on 0,0
					if( !hcAt.X() && !hcAt.Y() )
						continue;

					// just want the borders of the area, which is 
					// the newly expanded to hexes on the edge of area
					if( hcAt.X() != hcFrom.X() &&
						hcAt.X() != hcTo.X() && 
						hcAt.Y() != hcFrom.Y() &&
						hcAt.Y() != hcTo.Y() )
						continue;

					//aiHex.m_iX = hcAt.X();
					//aiHex.m_iY = hcAt.Y();

					// use the game's opinion of building this building
					if( theMap.FoundationCost(hcAt, 
						iBldg, 0, (CVehicle const *)-1) < 0 )
						continue;

					// consider if this is buildable
					if( !AreHexesOpen( MAX_ADJACENT, hcAt, 
						iWidthX, iWidthY ) )
						continue;

					if( NearBuildings( hcAt, iWidthX, iWidthY, FALSE ) )
						continue;

					// make sure crane can get there
					if( !GetPathRating( hexFound, hcAt ) )
						continue;

					iRating = pGameData->GetRangeDistance(
						hexRocket, hcAt );

					// if enemy is used then best is closest to hexRocket
					if( m_bUseOpFor )
					{
						if( iRating < iBestForts )
						{
							iBestForts = iRating;
							hexFort = hcAt;
						}
					}
					else // else using own rocket, means best is far from hexRocket
					{
						if( iRating > iBestForts )
						{
							iBestForts = iRating;
							hexFound = hexFort = hcAt;
						}
					}
				}
			}

			// if a site was found then return with it
			if( m_bUseOpFor )
			{
				if( iBestForts < pGameData->m_iHexPerBlk )
				{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"FindSpecialHex() found at %d,%d \n", hexFort.X(), hexFort.Y() );
#endif
					hexFound = hexFort;
					return;
				}
			}
			else
			{
				if( iBestForts )
				{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"FindSpecialHex() found at %d,%d \n", hexFort.X(), hexFort.Y() );
#endif
					hexFound = hexFort;
					return;
				}
			}
			iStep++;
		}

		// unable to find a site, reduce count of forts in building
		if( pBestBldg != NULL )
		{
			iStep = pBestBldg->GetParam(CAI_ACCURACY) - 1;
			if( iStep < 0 )
				iStep = 0;

			pBestBldg->SetParam( CAI_ACCURACY,(WORD)iStep );
		}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"FindSpecialHex() not found \n" );
#endif
	}

	// CStructureData::embassy
	// find nearest adjacent hex to an opfor command center
	if( iBldg == CStructureData::embassy )
	{
		return;
	}
}

//
// return the rating of this vehicle as a value target
//
int CAIMapUtil::AssessTarget(CVehicle *pVeh, int iKindOf)
{
	int iAssessment = 0;

	// vehicles are more desirable so give bonus for land vehicle
	// targets and naval targets
	if( iKindOf == CAI_SOFTATTACK ||
		iKindOf == CAI_NAVALATTACK )
		iAssessment = 100;

	switch( pVeh->GetData()->GetType() )
	{
		case CTransportData::light_scout:
		case CTransportData::infantry_carrier:
		case CTransportData::landing_craft:
		case CTransportData::infantry:
		case CTransportData::rangers:
		case CTransportData::marines:
			iAssessment += 1 ;
			break;
		case CTransportData::med_scout:
		case CTransportData::light_tank:
		case CTransportData::light_art:
		case CTransportData::gun_boat:
			iAssessment += 2 ;
			break;
		case CTransportData::heavy_scout:
		case CTransportData::med_tank:
		case CTransportData::med_art:
		case CTransportData::destroyer:
			iAssessment += 4 ;
			break;
		case CTransportData::heavy_tank:
		case CTransportData::heavy_art:
		case CTransportData::cruiser:
			iAssessment += 6 ;
			break;
		case CTransportData::heavy_truck:
		case CTransportData::construction:
			if( pGameData->m_iSmart )
			{
				iAssessment += pGameData->m_iSmart;
				iAssessment += pGameData->GetRandom(
					(pGameData->m_iSmart * 2) );
			}
			else
				iAssessment += 1 ;
		default:
			break;
	}
	return( iAssessment );
}

//
// return the rating of this building as a value target
//
int CAIMapUtil::AssessTarget( CBuilding *pBldg, int iKindOf)
{
	int iAssessment = 0;

	// buildings are more desirable
	if( iKindOf == CAI_HARDATTACK )
		iAssessment = 100;

	switch( pBldg->GetData()->GetType() )
	{
		case CStructureData::city:
			iAssessment = 0;
			break;
		case CStructureData::rocket:
			if( pBldg->GetOwner()->GetPlyrNum() == m_iPlayer )
				iAssessment += 2;
			else
			{
				if( iAssessment )
					iAssessment += 2;
			}
			break;
		case CStructureData::apartment_1_1:
		case CStructureData::apartment_1_2:
		case CStructureData::apartment_2_1:
		case CStructureData::apartment_2_2:
		case CStructureData::apartment_2_3:
		case CStructureData::apartment_2_4:
		case CStructureData::apartment_3_1:
		case CStructureData::apartment_3_2:
			if( iAssessment )
				iAssessment += 1;
			break;
		case CStructureData::office_2_1:
		case CStructureData::office_2_2:
		case CStructureData::office_2_3:
		case CStructureData::office_2_4:
		case CStructureData::office_3_1:
		case CStructureData::office_3_2:
			if( iAssessment )
				iAssessment += 1;
			break;
		case CStructureData::barracks_2:
		case CStructureData::command_center:
		case CStructureData::fort_1:
		case CStructureData::light_0:
			iAssessment += 4;
			break;
		//case CStructureData::barracks_3:
		case CStructureData::fort_2:
		case CStructureData::light_1:
		case CStructureData::shipyard_1:
		case CStructureData::seaport:
			iAssessment += 6;
			break;
		case CStructureData::fort_3:
		case CStructureData::light_2:
		case CStructureData::repair:
		case CStructureData::shipyard_3:
		case CStructureData::heavy:
			iAssessment += 8;
			break;
		case CStructureData::farm:
		case CStructureData::research:
		case CStructureData::power_1:
		case CStructureData::refinery:
			iAssessment += 10;
			break;
		case CStructureData::power_2:
		case CStructureData::copper:
		case CStructureData::iron:
		case CStructureData::coal:
			iAssessment += 14;
			break;
		case CStructureData::oil_well:
		case CStructureData::lumber:
		case CStructureData::smelter:
		case CStructureData::power_3:
			iAssessment += 20;
			break;
		default:
			break;
	}
	return( iAssessment );
}

//
// find the hex, relative to the material needed for the
// iBldg passed, of the width passed, and return the hex
//
void CAIMapUtil::FindHexOnMaterial( int iBldg,
	int iWidthX, int iWidthY, CHexCoord& hexFound )
{
	// based on the building type, determine
	// the material to find in the map and
	// the various users of that material
	int iMaterial = CMaterialTypes::num_types;
	int iUser1 = CStructureData::num_types;
	int iUser2 = CStructureData::num_types;
	int iUser3 = CStructureData::num_types;

	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData == NULL )
		return;

	CString sMat;

	// use the base type rather than specific type
	switch( pBldgData->GetBldgType() )
	{
		case CStructureData::copper:
			iMaterial = CMaterialTypes::copper;
			iUser1 = CStructureData::light;
			iUser2 = CStructureData::heavy;
			iUser3 = CStructureData::power;
			sMat = "copper";
			break;
		case CStructureData::oil_well:
			iMaterial = CMaterialTypes::oil;
			iUser1 = CStructureData::refinery;
			iUser2 = CStructureData::power;
			sMat = "oil";
			break;
		case CStructureData::coal:
			iMaterial = CMaterialTypes::coal;
			iUser1 = CStructureData::smelter;
			iUser2 = CStructureData::power;
			sMat = "coal";
			break;
		case CStructureData::iron:
			iMaterial = CMaterialTypes::iron;
			iUser1 = CStructureData::smelter;
			iUser2 = CStructureData::power;
			sMat = "iron";
			break;
		default:
			return;
	}

	CMinerals *pmn;
	CHexCoord hex;
	CAIHex aiHex(0,0);
	int iRating = 0;
	int iBestRating = 0;
	m_bMinerals = TRUE;
	
	// consider the cells closest to the base of the map
	

	// this considers the map to be the same size as the world
	// so for a first try, only consider the area within the 
	// pGameData->m_iHexPerBlk size distance around the base
	// pGameData->m_iBlkPerSide = 2 for small world
	CHexCoord hcStart,hcEnd;
	//int iArea = 0;
	int iArea = 4;

	// set density sought for to be 1/2 the best found on the map
	int iDenseFactor = (MAX_MINERAL_DENSITY / 2);
	CAIHex *paiHex = &pGameData->m_paihDensity[iMaterial];
	if( (int)paiHex->m_cTerrain )
		iDenseFactor = (int)((int)paiHex->m_cTerrain / 2);

	int iBestHex = m_iMapSize;
	int iCnt = 0,i,iY,iX;
	WORD wStatus, wTest;
	int iDeltaX,iDeltaY,iRocketRating;

	// setup test to filter out invalid cells
	wTest = 0;
	wTest = MSW_AI_BUILDING | MSW_OPFOR_BUILDING | MSW_STAGING;

	TryTryAgain:

	ClearWorkMap();
	iBestHex = m_iMapSize;
	iRocketRating = 0;
	iCnt = 0;
	CHexCoord hexBase = m_RocketHex;

	if( iArea ) // first try 1/4, then 1/2, then 1 (full block size)
	{
	hcStart.X( hex.Wrap(hexBase.X() - (pGameData->m_iHexPerBlk/iArea)) );
	hcStart.Y( hex.Wrap(hexBase.Y() - (pGameData->m_iHexPerBlk/iArea)) );
	hcEnd.X( hex.Wrap(hexBase.X() + (pGameData->m_iHexPerBlk/iArea)) );
	hcEnd.Y( hex.Wrap(hexBase.Y() + (pGameData->m_iHexPerBlk/iArea)) );
	}
	else	// do entire map
	{
	hcStart.X( m_wBaseCol );
	hcStart.Y( m_wBaseRow );
	hcEnd.X( m_wEndCol );
	hcEnd.Y( m_wEndRow );
	}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nFindHexOnMaterial() player %d for %s (%d) ", 
    	m_iPlayer, (const char *)sMat, iMaterial );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"from %d,%d to %d,%d ", 
		hcStart.X(), hcStart.Y(), hcEnd.X(), hcEnd.Y() );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"best density %d  wTest=%d \n", iDenseFactor, wTest );
#endif

	// run through the cells to consider
	if( iArea )
	{
		iDeltaX = hex.Wrap(hcEnd.X() - hcStart.X());
		iDeltaY = hex.Wrap(hcEnd.Y() - hcStart.Y());
	}
	else
	{
		iDeltaX = (hcEnd.X() - hcStart.X()) + 1;
		iDeltaY = (hcEnd.Y() - hcStart.Y()) + 1;
	}
	

	for( iY=0; iY<iDeltaY; ++iY )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iY) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
		myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

		for( iX=0; iX<iDeltaX; ++iX )
		{
			
			hex.X( hex.Wrap(hcStart.X() + iX) );

			// do not allow placing on 0,0
			if( !hex.X() && !hex.Y() )
				continue;

			aiHex.m_iY=hex.Y();
			aiHex.m_iX=hex.X();

			i = GetMapOffset( aiHex.m_iX, aiHex.m_iY );
			if( i >= m_iMapSize )
				continue;

			wStatus = m_pMap[i];

			//  and something is already there
			if( wStatus & wTest )
				continue;

			// check hex for material
			if( theMinerals.Lookup(hex, pmn) )
			{
				// of the type we are interested in
				if( pmn->GetType() == iMaterial )
				{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d found %d at %d,%d ", 
		m_iPlayer, iMaterial, hex.X(), hex.Y() );
#endif
					
					// make sure its dense enough for our use
					if( pmn->GetDensity() < iDenseFactor )
					{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"Insufficient density of %d with iDenseFactor=%d ",
		pmn->GetDensity(), iDenseFactor );
#endif
						continue;
					}

					// consider if this is buildable
					if( AreHexesOpen( MAX_ADJACENT, hex, 
						iWidthX, iWidthY ) )
					{
						// make sure hex is buffersize away from other buildings
						if( NearBuildings( hex, iWidthX, iWidthY, TRUE ) )
						{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"NearBuildings() failed " );
#endif
							continue;
						}

						// use the game's opinion of building this building
						if( theMap.FoundationCost(hex, 
							iBldg, 0, (CVehicle const *)-1) < 0 )
						{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"FoundationCost() failed " );
#endif
							continue;
						}

						// and we want out of the range of opfor units
						if( !IsOutOpforRange(hex) )
							continue;

						// we want sites far away from opfor rockets
						if( !GetRocketRating(hex,TRUE) )
							continue;

						//iRating = GetRocketRating( hex );
						//if( iRocketRating > iRating )
						//	continue;

						// make sure the crane can get there,
						// and if it can't then that might be
						// because it is on an island, and we
						// will need to use a cargo ship to get
						// over to it and to get stuff back
						//
						//CHexCoord hexTo( aiHex.m_iX, aiHex.m_iY );
						if( !GetPathRating( hexFound, hex ) )
						{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"GetPathRating() failed " );
#endif
							continue;
						}

						// current best combined distance from opfor
						iRocketRating = iRating;

						// get quantity of material
						//iRating = pmn->GetQuantity();

						iRating = GetMineralRating( hex, iWidthX, iWidthY, iMaterial );

						// add distance to all opfor rockets - distance this rocket
						//iRating += GetRocketRating( hex );

						// and save result
						m_pwaWork[i] = iRating;

						if( iRating > iBestRating )
						{
							iBestRating = iRating;
							iBestHex = i;
							iCnt = 0;
						}
						// count how many are best
						if( iRating == iBestRating )
							iCnt++;
					}
					else
					{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"AreHexesOpen() failed " );
#endif
						continue;
					}
				}
			}
		}
	}

	// none found
	if( !iCnt )
	{
		// can we expand the search space?
		if( iArea > 1 )
		{
			iArea /= 2;
			iDenseFactor /= 2; // reduce density required

			// BUGBUG this depends on m_wRows == m_wCols
			if( iArea == 1 )
			{
				// map is only as width/high as 2 blocks
				iRating = (pGameData->m_iHexPerBlk * 2);
				// should we force searching the entire map?
				if( iRating == (int)m_wRows )
				{
					iArea = 0;
				}
			}
			goto TryTryAgain;
		}
		// one more time, let any density work
		if( iDenseFactor )
		{
			iDenseFactor = 0;
			goto TryTryAgain;
		}

		sMat.Empty();

		return;
	}

	sMat.Empty();

	// only one found
	if( iCnt == 1 )
	{
		OffsetToXY( iBestHex, &aiHex.m_iX, &aiHex.m_iY );

		// make sure the crane can get there
		CHexCoord hexTo( aiHex.m_iX, aiHex.m_iY );
		if( GetPathRating( hexFound, hexTo ) )
		{
			hexFound.X(aiHex.m_iX);
			hexFound.Y(aiHex.m_iY);
			return;
		}
		// if still here then clear best hex and lower rating
		m_pwaWork[iBestHex] = 0;
		iRating = (iBestRating / 4) * 3;
		iBestRating = iRating;
	}

	// if still here then there many best values
	// so clear all offsets but those
	for( i=0; i<m_iMapSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		// hex has material we are interested in
		if( m_pwaWork[i] >= (WORD)iBestRating )
			continue;

		m_pwaWork[i] = 0;
	}

	// now the only offsets that are non-zero are
	// those with the best value so select the one 
	// closest to a factory that uses this material
	iBestRating = m_iMapSize * 4;
	iBestHex = m_iMapSize;
	iCnt = 0;

	for( i=0; i<m_iMapSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		// a best hex that has material we are interested in
		if( m_pwaWork[i] )
		{
			// convert offset to coordinates
			OffsetToXY( i, &aiHex.m_iX, &aiHex.m_iY );

			CHexCoord hexTo( aiHex.m_iX, aiHex.m_iY );
			if( !GetPathRating( hexFound, hexTo ) )
			{
				m_pwaWork[i] = 0;
				continue;
			}

			// sum the distances from this hex to those
			// other buildings that use this material
			iRating = GetClosestTo( hexTo, iUser1, iUser2, iUser3 );

			// and the building multiplier
			//iRating += GetMultiplier( &aiHex, 
			//	iWidthX, iWidthY, iBldg );

			// and save it
			m_pwaWork[i] = iRating;

			if( iRating && iRating < iBestRating )
			{
				iCnt = 0;
				iBestRating = iRating;
				iBestHex = i;
			}
			if( iRating == iBestRating )
				iCnt++;
		}
	}
	// none found
	if( !iCnt )
		return;

	// only one found
	if( iCnt == 1 )
	{
		OffsetToXY( iBestHex, &aiHex.m_iX, &aiHex.m_iY );
		hexFound.X(aiHex.m_iX);
		hexFound.Y(aiHex.m_iY);
		return;
	}

	// if still here then there many best values
	// so clear all offsets but those
	iCnt = 0;
	for( i=0; i<m_iMapSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		// hex has material we are interested in
		//if( m_pwaWork->GetAt(i) == iBestRating )
		if( m_pwaWork[i] == iBestRating )
			iCnt++;
		else
			m_pwaWork[i] = 0;
	}

	// randomly pick one
	iBestHex = pGameData->GetRandom( iCnt );
	iBestHex = FindNth( iBestHex );
	OffsetToXY( iBestHex, &aiHex.m_iX, &aiHex.m_iY );
	hexFound.X(aiHex.m_iX);
	hexFound.Y(aiHex.m_iY);
	return;
}


//
// find the hex, relative to the terrain required by the
// iBldg, of the width passed, and allowing for a buffer
// zone of 3 hexes from edge to other buildings and return it
//
void CAIMapUtil::FindHexOnTerrain( int iBldg,
	int iWidthX, int iWidthY, CHexCoord& hexFound )
{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"\nFindHexOnTerrain() player %d for building %d from %d,%d", 
   	m_iPlayer, iBldg, hexFound.X(), hexFound.Y() );
#endif

	//CHexCoord hexBase( m_iBaseX, m_iBaseY );
	CHexCoord hexBase = m_RocketHex;
	CHexCoord hcStart,hcEnd,hex, hexBest(0,0);
	CAIHex aiHex( 0, 0 );
	int iRating,iBestHex=m_iMapSize,iBestRating=0,iX,iY,iStep=1,i;
	int iDeltax, iDeltay;
	//int iForests = 0;

    WORD wTest = MSW_ROAD | MSW_RESOURCE 
		| MSW_OPFOR_VEHICLE | MSW_OPFOR_BUILDING
		| MSW_AI_VEHICLE | MSW_AI_BUILDING | MSW_STAGING;

	// conduct spiral search outward
	while( iStep < pGameData->m_iHexPerBlk )
	{
		hcStart.X( hex.Wrap(hexBase.X()-iStep) );
		hcStart.Y( hex.Wrap(hexBase.Y()-iStep) );
		hcEnd.X( hex.Wrap(hexBase.X()+iStep) );
		hcEnd.Y( hex.Wrap(hexBase.Y()+iStep) );

		iDeltax = hex.Wrap(hcEnd.X() - hcStart.X());
		iDeltay = hex.Wrap(hcEnd.Y() - hcStart.Y());

#if THREADS_ENABLED
				// BUGBUG this function must yield
			myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( iY=0; iY<iDeltay; ++iY )
		{
			hex.Y( hex.Wrap(hcStart.Y()+iY) );

			for( iX=0; iX<iDeltax; ++iX )
			{
				hex.X( hex.Wrap(hcStart.X()+iX) );

				// do not allow placing on 0,0
				if( !hex.X() && !hex.Y() )
					continue;

				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hex.X() != hcStart.X() &&
					hex.X() != hcEnd.X() && 
					hex.Y() != hcStart.Y() &&
					hex.Y() != hcEnd.Y() )
					continue;

				i = GetMapOffset( hex.X(), hex.Y() );
				if( i >= m_iMapSize )
					continue;

				if( m_pMap[i] & wTest )
					continue;

				// get game data, if sloped, it returns non-zero
				// can't build there either
				CHex *pGameHex = theMap.GetHex( hex );
				// known hex means return is not NULL
				if( pGameHex == NULL )
					continue;

				// pre-screen as these buildings must build
				// on certain terrain types
				if( iBldg == CStructureData::farm )
				{
					if( pGameHex->GetType() != CHex::plain )
						continue;
				}

				if( iBldg == CStructureData::lumber )
				{
					if( pGameHex->GetType() != CHex::forest )
						continue;
				}
/*
				if( iBldg == CStructureData::lumber &&
					(int)aiHex.m_cTerrain == CHex::forest )
					iForests++;
*/

				// use the game's opinion of building this building
				if( theMap.FoundationCost(hex, 
					iBldg, 0, (CVehicle const *)-1) < 0 )
					continue;

				// make sure farm hex is buffersize away from other buildings
				// with a little extra on (by doubling m_iBufferSize size)
				if( iBldg == CStructureData::farm )
				{
					iRating = 1;

					// check +y, +x, +x,+y for plains
					CHexCoord hexPlain = hex;
					hexPlain.Yinc();
					pGameHex = theMap.GetHex( hexPlain );
					if( pGameHex->GetType() == CHex::plain )
						iRating++;

					hexPlain.Xinc();
					pGameHex = theMap.GetHex( hexPlain );
					if( pGameHex->GetType() == CHex::plain )
						iRating++;

					hexPlain.Ydec();
					pGameHex = theMap.GetHex( hexPlain );
					if( pGameHex->GetType() == CHex::plain )
						iRating++;

					if( iRating < 4 )
						continue;

					// double buffersize for just this test
					//m_iBufferSize *= 2;
					//if( pGameData->m_iHexPerBlk > 32 )
						m_iBufferSize <<= 1;
					if( NearBuildings( hex,
						iWidthX, iWidthY, FALSE ) )
					{
						//m_iBufferSize /= 2;
						//if( pGameData->m_iHexPerBlk > 32 )
							m_iBufferSize >>= 1;
						continue;
					}
					//m_iBufferSize /= 2;
					//if( pGameData->m_iHexPerBlk > 32 )
						m_iBufferSize >>= 1;
				}

				if( iBldg == CStructureData::lumber )
				{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIMapUtil::FindHexOnTerrain() testing %d,%d which is %d terrain ", 
hex.X(), hex.Y(), (int)pGameHex->GetType() );
#endif
					iRating = 0;

					// check +y, +x, +x,+y for forest
					CHexCoord hexForest = hex;
					hexForest.Yinc();
					pGameHex = theMap.GetHex( hexForest );
					if( pGameHex->GetType() == CHex::forest )
						iRating++;

					hexForest.Xinc();
					pGameHex = theMap.GetHex( hexForest );
					if( pGameHex->GetType() == CHex::forest )
						iRating++;

					hexForest.Ydec();
					pGameHex = theMap.GetHex( hexForest );
					if( pGameHex->GetType() == CHex::forest )
						iRating++;

					// med and large map considerations
					if( pGameData->m_iHexPerBlk > 32 )
					{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIMapUtil::FindHexOnTerrain() testing %d,%d has rating %d terrain ", 
hex.X(), hex.Y(), iRating );
#endif
						if( iRating < 3 )
							continue;
						if( NearBuildings( hex,
							iWidthX, iWidthY, FALSE ) )
						{
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIMapUtil::FindHexOnTerrain() testing %d,%d too near buildings ", 
hex.X(), hex.Y() );
#endif
							continue;
						}
					}
					else // small world map considerations
					{
						if( pGameData->m_iSmart )
						{
							if( iRating < 2 )
								continue;
						}

						// reduce buffersize
						m_iBufferSize--;
						if( NearBuildings( hex,
							iWidthX, iWidthY, FALSE ) )
						{
							m_iBufferSize++;
							continue;
						}
						m_iBufferSize++;
					}
				}

				if( !AreHexesOpen( MAX_ADJACENT, hex, 
					iWidthX, iWidthY ) )
				{
#ifdef _LOGOUT
if( iBldg == CStructureData::lumber )
{
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexOnTerrain() testing %d,%d AreHexesOpen failed ", 
	hex.X(), hex.Y() );
}
#endif
					continue;
				}

				// we want out of the range of opfor units
				if( pGameData->m_iSmart &&
					pGameData->m_iHexPerBlk > 32 )
				{
					if( !IsOutOpforRange(hex) )
						continue;
				}
				// we want sites far away from opfor rockets,
				// except on easy
				// but on difficult and med/large worlds
				if( pGameData->m_iSmart &&
					pGameData->m_iHexPerBlk > 32 )
				{
					if( !GetRocketRating(hex,TRUE) )
						continue;
				}

				// make sure the crane can get there
				if( !GetPathRating( hexFound, hex ) )
				{
#ifdef _LOGOUT
if( iBldg == CStructureData::lumber )
{
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexOnTerrain() testing %d,%d GetPathRating failed ", 
	hex.X(), hex.Y() );
}
#endif
					continue;
				}

				// rate this hex for the buffer zone hexes
				// that would surround it
				iRating = GetBufferRating( iBldg, hex,
					iWidthX, iWidthY );

				if( iRating > iBestRating )
				{
					iBestRating = iRating;
					iBestHex = i;
					//hexBest = hex;
				}
			}
		}

		// before next step out in spiral, test for best rating at that range
		if( iBestHex < m_iMapSize )
		{
			OffsetToXY( iBestHex, &aiHex.m_iX, &aiHex.m_iY );
			hexFound.X(aiHex.m_iX);
			hexFound.Y(aiHex.m_iY);
			//hexFound = hexBest;

#ifdef _LOGOUT
CHex *pGameHex = theMap.GetHex( hexFound );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIMapUtil::FindHexOnTerrain() player %d found %d,%d  which is %d terrain", 
m_iPlayer, hexFound.X(), hexFound.Y(), (int)pGameHex->GetType() );
#endif
			return;
		}

		iStep++;
		iBestHex = m_iMapSize;
		iBestRating = 0;

/*
		if( iBldg == CStructureData::lumber )
		{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIMapUtil::FindHexOnTerrain() player %d forests %d at step %d ", 
		m_iPlayer, iForests, (iStep-1) );

#endif
		iForests = 0;
		}
*/
	}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"FindHexOnTerrain() player %d for building %d ", 
   	m_iPlayer, iBldg );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"second try with iDelta=%d,%d iStep=%d ", 
   	iDeltax,iDeltay,iStep );
#endif

	// if here, we've exceeded a block size in all directions and
	// have not found a site so use 1/2 m_wRows
	// conduct spiral search outward without opfor considerations
	// and proximity as good terrain considerations
	iStep = 1;;
	iBestHex = m_iMapSize;
	iBestRating = 0;
	int iMaxSteps = m_wRows / 2;
	while( iStep < iMaxSteps )
	{
		hcStart.X( hex.Wrap(hexBase.X()-iStep) );
		hcStart.Y( hex.Wrap(hexBase.Y()-iStep) );
		hcEnd.X( hex.Wrap(hexBase.X()+iStep) );
		hcEnd.Y( hex.Wrap(hexBase.Y()+iStep) );

		iDeltax = hex.Wrap(hcEnd.X() - hcStart.X());
		iDeltay = hex.Wrap(hcEnd.Y() - hcStart.Y());

#if THREADS_ENABLED
				// BUGBUG this function must yield
			myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( iY=0; iY<iDeltay; ++iY )
		{
			hex.Y( hex.Wrap(hcStart.Y()+iY) );

			for( iX=0; iX<iDeltax; ++iX )
			{
				hex.X( hex.Wrap(hcStart.X()+iX) );

				// do not allow placing on 0,0
				if( !hex.X() && !hex.Y() )
					continue;

				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hex.X() != hcStart.X() &&
					hex.X() != hcEnd.X() && 
					hex.Y() != hcStart.Y() &&
					hex.Y() != hcEnd.Y() )
					continue;

				i = GetMapOffset( hex.X(), hex.Y() );
				if( i >= m_iMapSize )
					continue;

				// if anything is here, skip it
				if( m_pMap[i] )
					continue;

				// get game data, if sloped, it returns non-zero
				// can't build there either
				CHex *pGameHex = theMap.GetHex( hex );
				// known hex means return is not NULL
				if( pGameHex == NULL )
					continue;

				// pre-screen as these buildings must build
				// on certain terrain types
				if( iBldg == CStructureData::farm )
				{
					if( pGameHex->GetType() != CHex::plain )
						continue;
				}
				if( iBldg == CStructureData::lumber )
				{
					if( pGameHex->GetType() != CHex::forest )
						continue;
				}

				// use the game's opinion of building this building
				if( theMap.FoundationCost(hex, 
					iBldg, 0, (CVehicle const *)-1) < 0 )
					continue;

				if( iBldg == CStructureData::farm )
				{
					// double buffersize for just this test
					//m_iBufferSize *= 2;
					if( pGameData->m_iHexPerBlk > 32 )
						m_iBufferSize <<= 1;
					if( NearBuildings( hex, iWidthX, iWidthY, FALSE ) )
					{
						//m_iBufferSize /= 2;
						if( pGameData->m_iHexPerBlk > 32 )
							m_iBufferSize >>= 1;
						continue;
					}
					//m_iBufferSize /= 2;
					if( pGameData->m_iHexPerBlk > 32 )
						m_iBufferSize <<= 1;
				}

				if( iBldg == CStructureData::lumber )
				{
					iRating = 0;
					// check +y, +x, +x,+y for forest to make lumber
					// end up on four hexes of forest, except small maps
					CHexCoord hexForest = hex;
					hexForest.Yinc();
					pGameHex = theMap.GetHex( hexForest );
					if( pGameHex->GetType() == CHex::forest )
						iRating++;
					hexForest.Xinc();
					pGameHex = theMap.GetHex( hexForest );
					if( pGameHex->GetType() == CHex::forest )
						iRating++;
					hexForest.Ydec();
					pGameHex = theMap.GetHex( hexForest );
					if( pGameHex->GetType() == CHex::forest )
						iRating++;

					// medium and large worlds
					if( pGameData->m_iHexPerBlk > 32 )
					{
						if( iRating < 3 )
							continue;
					}
					// small world can build on 1 or more forest
					// and bufferzone is not considered
				}

				if( !AreHexesOpen( MAX_ADJACENT, hex, 
					iWidthX, iWidthY ) )
					continue;

				// make sure the crane can get there
				if( !GetPathRating( hexFound, hex ) )
					continue;

				// rate this hex for the buffer zone hexes
				// that would surround it
				//iRating = GetBufferRating( iBldg, &aiHex,
				//	iWidthX, iWidthY );
				iRating = GetBufferRating( iBldg, hex,
					iWidthX, iWidthY );

				if( iRating > iBestRating )
				{
					iBestRating = iRating;
					iBestHex = i;
				}
			}
		}

		// before next step out in spiral, test for best rating at that range
		if( iBestHex < m_iMapSize )
		{
			OffsetToXY( iBestHex, &aiHex.m_iX, &aiHex.m_iY );
			hexFound.X(aiHex.m_iX);
			hexFound.Y(aiHex.m_iY);

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexOnTerrain() player %d finally found %d,%d ", 
	m_iPlayer, hexFound.X(), hexFound.Y() );

#endif
			return;
		}

		iStep++;
		iBestHex = m_iMapSize;
		iBestRating = 0;
	}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexOnTerrain() player %d no hex found ", 
	m_iPlayer );
#endif
}

//
// find first hex, spiraling out from hexTarget, of the terrain passed
//
void CAIMapUtil::FindAssaultHex( CHexCoord& hexTarget, int iTerrain )
{
	CHexCoord hexBase = hexTarget;
	CHexCoord hcStart,hcEnd,hex;
	int iX,iY,iStep=1,i;
	int iDeltax, iDeltay;

    WORD wTest = MSW_OPFOR_BUILDING | MSW_AI_BUILDING;

	// conduct spiral search outward
	while( iStep < pGameData->m_iHexPerBlk )
	{
		hcStart.X( hex.Wrap(hexBase.X()-iStep) );
		hcStart.Y( hex.Wrap(hexBase.Y()-iStep) );
		hcEnd.X( hex.Wrap(hexBase.X()+iStep) );
		hcEnd.Y( hex.Wrap(hexBase.Y()+iStep) );

		iDeltax = hex.Wrap(hcEnd.X() - hcStart.X());
		iDeltay = hex.Wrap(hcEnd.Y() - hcStart.Y());

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindAssaultHex() player %d from %d,%d to %d,%d ", 
	m_iPlayer, hcStart.X(), hcStart.Y(), hcEnd.X(), hcEnd.Y() );
#endif

#if THREADS_ENABLED
				// BUGBUG this function must yield
			myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( iY=0; iY<iDeltay; ++iY )
		{
			hex.Y( hex.Wrap(hcStart.Y()+iY) );

			for( iX=0; iX<iDeltax; ++iX )
			{
				hex.X( hex.Wrap(hcStart.X()+iX) );

				// do not allow placing on 0,0
				if( !hex.X() && !hex.Y() )
					continue;

				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hex.X() != hcStart.X() &&
					hex.X() != hcEnd.X() && 
					hex.Y() != hcStart.Y() &&
					hex.Y() != hcEnd.Y() )
					continue;

				i = GetMapOffset( hex.X(), hex.Y() );
				if( i >= m_iMapSize )
					continue;

				if( m_pMap[i] & wTest )
					continue;

				// get game data, if sloped, it returns non-zero
				// can't build there either
				CHex *pGameHex = theMap.GetHex( hex );
				// known hex means return is not NULL
				if( pGameHex == NULL )
					continue;

				// need an ocean or lake hex
				if( iTerrain == CHex::ocean )
				{
					if( pGameHex->GetType() != CHex::ocean &&
						pGameHex->GetType() != CHex::lake )
						continue;
				}
				else if( iTerrain == CHex::coastline )
				{
					if( pGameHex->GetType() != CHex::coastline )
						continue;
				}
				else // need a land hex
				{
					if( pGameHex->GetType() == CHex::ocean ||
						pGameHex->GetType() == CHex::lake ||
						pGameHex->GetType() == CHex::coastline )
						continue;
				}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindAssaultHex() player %d found hex %d,%d for %d terrain ", 
	m_iPlayer, hex.X(), hex.Y(), iTerrain );
#endif
				hexTarget = hex;
				return;
			}
		}
		iStep++;
	}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindAssaultHex() player %d no hex found for %d terrain from %d,%d ", 
	m_iPlayer, iTerrain, hexTarget.X(), hexTarget.Y() );
#endif
}

//
// find the nearest bridge candiate to the rocket and return
// the start hex location in hexSite and in pUnit->GetParam(CAI_PREV_X/Y)
// and the end hex location in pUnit->GetParam(CAI_DEST_X/Y),
// unless none are available and then leave hexSite unchanged
//
void CAIMapUtil::FindBridgeHex( CHexCoord& hexSite, CAIUnit *pUnit )
{
	CHex *pGameHex;
	CHexCoord hexBase;
	// use rocket exit for base hex
	pGameData->FindBuilding( CStructureData::rocket, m_iPlayer, hexBase );
	CHexCoord hexBridge,hcFrom,hcTo;

	int iStep = 1,i,iX,iY;
	while( iStep < pGameData->m_iHexPerBlk )
	{
		hcFrom.X( hexBridge.Wrap(hexBase.X()-iStep) );
		hcFrom.Y( hexBridge.Wrap(hexBase.Y()-iStep) );
		hcTo.X( hexBridge.Wrap(hexBase.X()+iStep) );
		hcTo.Y( hexBridge.Wrap(hexBase.Y()+iStep) );

		int iDeltax = hexBridge.Wrap(hcTo.X()-hcFrom.X());
		int iDeltay = hexBridge.Wrap(hcTo.Y()-hcFrom.Y());

		for( iY=0; iY<iDeltay; ++iY )
		{
			hexBridge.Y( hexBridge.Wrap(hcFrom.Y()+iY) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

			for( iX=0; iX<iDeltax; ++iX )
			{
				hexBridge.X( hexBridge.Wrap(hcFrom.X()+iX) );

				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hexBridge.X() != hcFrom.X() &&
					hexBridge.X() != hcTo.X() && 
					hexBridge.Y() != hcFrom.Y() &&
					hexBridge.Y() != hcTo.Y() )
					continue;

				i = GetMapOffset( hexBridge.X(), hexBridge.Y() );
				if( i >= m_iMapSize )
					continue;

				// a planned road
				if( (m_pMap[i] & MSW_PLANNED_ROAD) )
				{
					// on river terrain indicates a bridge candidate
					pGameHex = theMap.GetHex( hexBridge );
					if( pGameHex == NULL )
						continue;
					if( pGameHex->GetType() != CHex::river )
						continue;

					// found a river hex with a planned road so check
					// it for hort/vert spanning river within MAX_SPAN
					// and if so, update pUnit->GetParam() with start/end
					// and hexBridge with start hex and return
					if( IsBridgeSpan( hexBridge, pUnit ) )
					{
						hexSite = hexBridge;
						return;
					}
				}
			}
		}
	}
}

//
// the hexStart is a river with MSW_PLANNED_ROAD set, indicating it
// is a candidate bridge location,  so check 0,2,4,6 sides for another
// river with MSW_PLANNED_ROAD set until either land or MAX_SPAN has
// been reached, returning TRUE and updating pUnit->GetParam() with
// start/end of span (must be land hexes)
//
BOOL CAIMapUtil::IsBridgeSpan( CHexCoord& hexRiverRoad, CAIUnit *pUnit )
{
	CHex *pGameHex;
	int i;
	CHexCoord hexStart(0,0),hexBridge;
	hexBridge = hexRiverRoad;

	// first find the land hex that will be used as the start hex
	// which will be the land hex in the 0,2,4,6 direction with
	// MSW_PLANNED_ROAD || MSW_ROAD set
	//
	// try 0
	hexBridge.Ydec();
	GetStartSpan( hexStart, hexBridge );

	if( !hexStart.X() && !hexStart.Y() )
	{
		// try 2
		hexBridge = hexRiverRoad;
		hexBridge.Xinc();
		GetStartSpan( hexStart, hexBridge );
	}
	if( !hexStart.X() && !hexStart.Y() )
	{
		// try 4
		hexBridge = hexRiverRoad;
		hexBridge.Yinc();
		GetStartSpan( hexStart, hexBridge );
	}
	if( !hexStart.X() && !hexStart.Y() )
	{
		// try 6
		hexBridge = hexRiverRoad;
		hexBridge.Xdec();
		GetStartSpan( hexStart, hexBridge );
	}
	// could not find the land hex to start the span
	if( !hexStart.X() && !hexStart.Y() )
		return FALSE;

	// if still here, then hexStart contains the hex found with both
	// land and a type of road set and hexRiverRoad is adjacent to it
	// so the 0,2,4,6 direction that hexRiverRoad is from hexStart
	// should indicate the direction of the span of the bridge
	int iDir = MAX_ADJACENT;
	hexBridge = hexStart;
	// try 0
	hexBridge.Ydec();
	if( hexBridge == hexRiverRoad )
		iDir = 0;
	if( iDir == MAX_ADJACENT )
	{
		// try 2
		hexBridge = hexStart;
		hexBridge.Xinc();
		if( hexBridge == hexRiverRoad )
			iDir = 2;
	}
	if( iDir == MAX_ADJACENT )
	{
		// try 4
		hexBridge = hexStart;
		hexBridge.Yinc();
		if( hexBridge == hexRiverRoad )
			iDir = 4;
	}
	if( iDir == MAX_ADJACENT )
	{
		// try 6
		hexBridge = hexStart;
		hexBridge.Xdec();
		if( hexBridge == hexRiverRoad )
			iDir = 6;
	}
	// could not determin bridge direction
	if( iDir == MAX_ADJACENT )
		return FALSE;


	// while count of span < MAX_SPAN
	//      move the bridge hex in the direction of the span
	//      if land encountered, then end span, save hexBridge
	//       as the end of the span, save hexStart as hexRiverRoad
	//       and return
	//      else if not river/road then return as a failure
	//      else if river/road then keep moving
	//
	hexBridge = hexRiverRoad;
	int iSpan = 2;
	while( iSpan <= MAX_SPAN )
	{
		switch(iDir)
		{
			case 0:
				hexBridge.Ydec();
				break;
			case 2:
				hexBridge.Xinc();
				break;
			case 4:
				hexBridge.Yinc();
				break;
			case 6:
				hexBridge.Xdec();
				break;
			default:
				return FALSE;
		}
		// invalid hex conversion to offset
		i = GetMapOffset( hexBridge.X(), hexBridge.Y() );
		if( i >= m_iMapSize )
			return FALSE;
		// there MUST be a road type here to be a bridge
		if( !(m_pMap[i] & MSW_PLANNED_ROAD) &&
			!(m_pMap[i] & MSW_ROAD) )
			return FALSE;

		pGameHex = theMap.GetHex( hexBridge );
		// if not river then we assume land, this is the end
		if( pGameHex->GetType() != CHex::river )
		{
			hexRiverRoad = hexStart;
			pUnit->SetParam(CAI_PREV_X,hexStart.X());
			pUnit->SetParam(CAI_PREV_Y,hexStart.Y());
			pUnit->SetParam(CAI_DEST_X,hexBridge.X());
			pUnit->SetParam(CAI_DEST_Y,hexBridge.Y());
			return TRUE;
		}

		// if still here that means the hex was river/road
		// thus will be part of the span
		iSpan++;
	}
	return FALSE;
}

//
// check the hexBridge for a type of road, and if found and terrain
// is not river, we assume (?) land so make hexStart == hexBridge
// to indicate it is found
//
void CAIMapUtil::GetStartSpan( CHexCoord& hexStart, CHexCoord& hexBridge )
{
	int i = GetMapOffset( hexBridge.X(), hexBridge.Y() );
	if( i < m_iMapSize )
	{
		if( (m_pMap[i] & MSW_PLANNED_ROAD) ||
			(m_pMap[i] & MSW_ROAD) )
		{
			CHex *pGameHex = theMap.GetHex( hexBridge );
			if( pGameHex->GetType() != CHex::river )
				hexStart = hexBridge;
		}
	}
}

//
// find the nearest planned road location to the base location
//
void CAIMapUtil::FindRoadHex( CHexCoord& hexFound )
{
	CHexCoord hexBase;
	// use rocket exit for base hex
	pGameData->FindBuilding( CStructureData::rocket, m_iPlayer, hexBase );
	CHexCoord hexRoad,hcFrom,hcTo;

	// spiral search out from base hex
	CHex *pGameHex;
	BOOL bRoads = FALSE;
	int iStep = 1, iBestRoad = m_iMapSize, i,iX,iY;

	while( iStep < pGameData->m_iHexPerBlk )
	{
		hcFrom.X( hexRoad.Wrap(hexBase.X()-iStep) );
		hcFrom.Y( hexRoad.Wrap(hexBase.Y()-iStep) );
		hcTo.X( hexRoad.Wrap(hexBase.X()+iStep) );
		hcTo.Y( hexRoad.Wrap(hexBase.Y()+iStep) );

		int iDeltax = hexRoad.Wrap(hcTo.X()-hcFrom.X()) + 1;
		int iDeltay = hexRoad.Wrap(hcTo.Y()-hcFrom.Y()) + 1;
		//int iDeltax = abs( hexRoad.Diff(hcTo.X()-hcFrom.X()) ) + 1;
		//int iDeltay = abs( hexRoad.Diff(hcTo.Y()-hcFrom.Y()) ) + 1;

		for( iY=0; iY<iDeltay; ++iY )
		{
			hexRoad.Y( hexRoad.Wrap(hcFrom.Y()+iY) );

			for( iX=0; iX<iDeltax; ++iX )
			{
				hexRoad.X( hexRoad.Wrap(hcFrom.X()+iX) );

				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hexRoad.X() != hcFrom.X() &&
					hexRoad.X() != hcTo.X() && 
					hexRoad.Y() != hcFrom.Y() &&
					hexRoad.Y() != hcTo.Y() )
					continue;

				i = GetMapOffset( hexRoad.X(), hexRoad.Y() );
				if( i >= m_iMapSize )
					continue;

				// a planned road
				if( (m_pMap[i] & MSW_PLANNED_ROAD) )
				{
					if( (m_pMap[i] & MSW_AI_BUILDING) ||
						(m_pMap[i] & MSW_OPFOR_BUILDING) )
						continue;

#if THREADS_ENABLED
				// BUGBUG this function must yield
					myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

					// skip planned road hexes on river terrain
					// as they are there to help find candidate
					// bridge locations later when the taskmgr
					// tries to assign a crane to bridge building
					pGameHex = theMap.GetHex( hexRoad );
					if( pGameHex == NULL )
						continue;
					if( pGameHex->GetType() == CHex::river )
						continue;

					BYTE bUnits = pGameHex->GetUnits();
					// skip buildings
					if( (bUnits & CHex::bldg) )
						continue;
					// and vehicles on a planned road
					if( (bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr)) )
						continue;

					// only accept planned roads with an
					// actual road at 0 or 2 or 4 or 6
						
					CHexCoord hexTestRoad = hexRoad;
					// is there a road or building at 0	?
					hexTestRoad.Ydec();
					int j = GetMapOffset( hexTestRoad.X(), hexTestRoad.Y() );
					if( m_pMap[j] & MSW_ROAD ||
						m_pMap[j] & MSW_AI_BUILDING )
						bRoads = TRUE;

					// is there a road or building at 2 ?
					hexTestRoad = hexRoad;
					hexTestRoad.Xinc();
					j = GetMapOffset( hexTestRoad.X(), hexTestRoad.Y() );
					if( m_pMap[j] & MSW_ROAD ||
						m_pMap[j] & MSW_AI_BUILDING )
						bRoads = TRUE;

					// is there a road or building at 4 ?
					hexTestRoad = hexRoad;
					hexTestRoad.Yinc();
					j = GetMapOffset( hexTestRoad.X(), hexTestRoad.Y() );
					if( m_pMap[j] & MSW_ROAD ||
						m_pMap[j] & MSW_AI_BUILDING )
						bRoads = TRUE;

					// is there a road or building at 6 ?
					hexTestRoad = hexRoad;
					hexTestRoad.Xdec();
					j = GetMapOffset( hexTestRoad.X(), hexTestRoad.Y() );
					if( m_pMap[j] & MSW_ROAD ||
						m_pMap[j] & MSW_AI_BUILDING )
						bRoads = TRUE;

					if( bRoads )
					{
						iBestRoad = i;
						goto BuildRoad;
					}
				}
			}
		}
		iStep++;
	}

	BuildRoad:
	if( bRoads )
	{
		OffsetToXY( iBestRoad, &iX, &iY );
		hexFound.X( iX );
		hexFound.Y( iY );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"Found road site player %d at %d,%d ", 
m_iPlayer, hexFound.X(), hexFound.Y() );
#endif

	}
}


//
// check locations within buffer size range of passed
// paiHex, and if any building exists, return TRUE
//
BOOL CAIMapUtil::InBufferSizeRange( CAIHex *paiHex )
{
	CHexCoord hcStart, hcEnd, hex;
	hcStart.X( hex.Wrap(paiHex->m_iX - m_iBufferSize) );
	hcStart.Y( hex.Wrap(paiHex->m_iY - m_iBufferSize) );
	hcEnd.X( hex.Wrap(paiHex->m_iX + m_iBufferSize) );
	hcEnd.Y( hex.Wrap(paiHex->m_iY + m_iBufferSize) );
	int iDeltaX = hex.Wrap(hcEnd.X() - hcStart.X());
	int iDeltaY = hex.Wrap(hcEnd.Y() - hcStart.Y());
	//int iDeltaX = abs( hex.Diff( hcEnd.X() - hcStart.X()));
	//int iDeltaY = abs( hex.Diff( hcEnd.Y() - hcStart.Y()));

	WORD wTest = 0;
	wTest = MSW_AI_BUILDING | MSW_OPFOR_BUILDING;
	CAIHex aiHex;


	for( int iY=0; iY<iDeltaY; ++iY )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iY) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
		myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( int iX=0; iX<iDeltaX; ++iX )
		{
			hex.X( hex.Wrap(hcStart.X() + iX) );

			//aiHex.m_iY = hex.Y();
			//aiHex.m_iX = hex.X();

			// check AI's opinion of the map
			//int i = GetMapOffset( aiHex.m_iX, aiHex.m_iY );
			int i = GetMapOffset( hex.X(), hex.Y() );
			if( m_pMap[i] & wTest )
				return TRUE;

			// check game's opinion of the map
			//pGameData->GetCHexData(&aiHex);
			//if( aiHex.m_dwUnitID )

			// get game data for this hex
			CHex *pGameHex = theMap.GetHex( hex );
			// known hex means return is not NULL
			if( pGameHex == NULL )
				continue;
		
			// a building or vehicle is in this hex
			if( pGameHex->GetUnits() & CHex::bldg )
				return TRUE;
		}
	}
	return FALSE;
}

//
// find the distance to the closest rocket
//
int CAIMapUtil::GetClosestRocket( CHexCoord& hexNear )
{
	CHexCoord hexRocket;
	int iRating = m_iMapSize, iDist;

	POSITION pos = m_plUnits->GetHeadPosition();
	while( pos != NULL )
	{
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );

			if( pUnit->GetOwner() == m_iPlayer )
				continue;

			// if this is a rocket, it is an opfor rocket
			if( pUnit->GetTypeUnit() == CStructureData::rocket )
			{
				EnterCriticalSection (&cs);
				CBuilding *pBldg = 
					theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
					hexRocket = pBldg->GetExitHex();
				LeaveCriticalSection (&cs);

				iDist = 
					pGameData->GetRangeDistance( hexNear, hexRocket );

				if( iDist && iDist < iRating )
					iRating = iDist;
			}
		}
	}
	return( iRating );
}

//
// go thru the units list or the map, and add up the 
// distance from the hex passed to the hex of the opfor
// rockets found, and subtract the distance to the hex
// of the player's rocket
//
int CAIMapUtil::GetRocketRating( CHexCoord& hex, BOOL bIsCloser/*=FALSE*/ )
{
	CHexCoord hexRocket;
	int iRating = 0, iDist;
	int iCloser = pGameData->GetRangeDistance( hex, m_RocketHex );

	if( bIsCloser )
		iRating = m_iMapSize;

//#ifdef _LOGOUT
//	BOOL bOpforFound = FALSE;
//#endif


	POSITION pos = m_plUnits->GetHeadPosition();
	while( pos != NULL )
	{
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );

			// if this is a rocket
			if( pUnit->GetTypeUnit() == CStructureData::rocket )
			{
				EnterCriticalSection (&cs);
				CBuilding *pBldg = 
					theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
					hexRocket = pBldg->GetExitHex();
				LeaveCriticalSection (&cs);

				iDist = pGameData->GetRangeDistance( hex, hexRocket );

//#ifdef _LOGOUT
//				if( pUnit->GetOwner() != m_iPlayer )
//					bOpforFound = TRUE;
//#endif
				if( bIsCloser )
				{
					// look for closest opfor rocket
					if( pUnit->GetOwner() != m_iPlayer )
					{
						if( iDist < iRating )
							iRating = iDist;
					}
					//else
					//	iCloser = iDist; // save player's rocket dist
				}
				else
				{
					// accumulate ranges to opfor rockets
					if( pUnit->GetOwner() != m_iPlayer )
						iRating += iDist;
					else
						iRating -= iDist;
				}
				// large iDist from player's rocket lowers rating
				// to encourage proximity to player's rocket

#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			}
		}
	}

/*
#ifdef _LOGOUT
	if( bOpforFound )
	{
#if _LOGGING
	if( pLog != NULL )
	{
    	wsprintf( szLogMsg, 
    	"\nCAIMapUtil::GetRocketRating() player %d opfor rocket found \n", 
    	m_iPlayer );
    	pLog->Write( szLogMsg );
	}
#endif
	}
#endif
*/

	// just looking for hexes that are close to rocket
	if( bIsCloser )
	{
		// hex is closer to player rocket
		if( iCloser < iRating )
			return(1);
		else
			return(0); // hex is closer to opfor rocket
	}

	// don't allow negative
	if( iRating < 0 )
		iRating = 0;
	// higher rating means hex is farther away from all opfor rockets
	return( iRating );
}

//
// rate the hexes surrounding paiHex, for buffer-zone
// distance from the edge of the width sized group
// based on the building passed (only supporting the
// CStructureData::farm and CStructureData::lumber types
//
int CAIMapUtil::GetBufferRating( int iBldg, CHexCoord& hexRate,
	int iWidthX, int iWidthY )
{
	CHexCoord hcStart, hcEnd, hex;
	hcStart.X( hex.Wrap(hexRate.X() - m_iBufferSize) );
	hcStart.Y( hex.Wrap(hexRate.Y() - m_iBufferSize) );
	hcEnd.X( hex.Wrap((hexRate.X() + iWidthX) + m_iBufferSize) );
	hcEnd.Y( hex.Wrap((hexRate.Y() + iWidthY) + m_iBufferSize) );
	int iDeltaX = hex.Wrap(hcEnd.X() - hcStart.X());
	int iDeltaY = hex.Wrap(hcEnd.Y() - hcStart.Y());
	//int iDeltaX = abs( hex.Diff( hcEnd.X() - hcStart.X()));
	//int iDeltaY = abs( hex.Diff( hcEnd.Y() - hcStart.Y()));

	// now go thru the area bounded and consider: existing
	// buildings (other than this one), roads and planned
	// roads and sum multiplier if farm or add 1 if lumber
	int iRating = 0;
	//CAIHex aiHex;
	int iTerrain;

	for( int iY=0; iY<iDeltaY; ++iY )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iY) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
		myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( int iX=0; iX<iDeltaX; ++iX )
		{
			hex.X( hex.Wrap(hcStart.X() + iX) );

			if( !hex.X() && !hex.Y() )
				continue;

			// get game data for this hex
			CHex *pGameHex = theMap.GetHex( hex );
			// known hex means return is not NULL
			if( pGameHex == NULL )
				continue;
		
			// a building or vehicle is in this hex
			if( pGameHex->GetUnits() )
				continue;

			iTerrain = pGameHex->GetType();
			// consider the type of terrain from the map
			if( iTerrain == CHex::ocean ||
				iTerrain == CHex::river ||
				iTerrain == CHex::lake ||
				iTerrain == CHex::road ||
				iTerrain == (BYTE)0xFF )
				continue;
			
			// some terrain just discourages farming
			if( iTerrain == CHex::desert ||
				iTerrain == CHex::hill ||
				iTerrain == CHex::mountain ||
				iTerrain == CHex::swamp )
			{
				if( iRating )
					iRating--;
				continue;
			}

			// get terrain from game data, and build multiplier
			CTerrainData const *pTerrain = 
				pGameData->GetTerrainData( iTerrain );
			if( pTerrain == NULL )
				continue;

			// accumulate multiplier based on type indicated
			if( iBldg == CStructureData::lumber &&
				iTerrain == CHex::forest )
				iRating++;
			else if( iBldg == CStructureData::lumber &&
				iTerrain != CHex::forest )
				iRating += 0;
			// get farm multiplier
			else if( iBldg == CStructureData::farm )
				iRating += pTerrain->GetFarmMult();
		}
	}
	return( iRating );
}

//
// calculate the distence to the closest instance of each
// type of CStructureData::GetBldgType() passed (up to 3 
// are supported), add then together and return it
// 
// the passed iBldg* are what is returned by GetBldgType()
// and not what is the specific type of building
//
int CAIMapUtil::GetClosestTo( CHexCoord hexFrom, 
	int iBldg1, int iBldg2, int iBldg3 )
{
	//CHexCoord hexFrom( paiHex->m_iX, paiHex->m_iY );
	CHexCoord hexTo;
	CAIHex aiHex(0,0);
	int iDist = 0;
	int iBestDist = 0;
	int iBldgType;

	// load the building family types into an array for processing
	int aiBldg[3];
	int aiDist[3];
	for( int i=0; i<3; ++i )
	{
		aiBldg[i]=0;
		aiDist[i]=m_iMapSize;
	}
	aiBldg[0] = iBldg1;
	aiBldg[1] = iBldg2;
	aiBldg[2] = iBldg3;

	// go through the buildings of this map
	int iYield = 0;
	POSITION pos = m_plUnits->GetHeadPosition();
	while( pos != NULL )
	{
		if (iYield > 25)
			{
			myYieldThread();
			iYield = 0;
			}

        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// if this is a building
			if( pUnit->GetType() == CUnit::building )
			{

				// BUGBUG change to use a value in CAIUnit
				// get access to CBuilding copied data for building?
				CAICopy *pCopyCBuilding = pUnit->GetCopyData( 
					CAICopy::CBuilding );
				if( pCopyCBuilding != NULL )
				{
					// get location of building
					hexTo.X(pCopyCBuilding->m_aiDataOut[CAI_LOC_X]);
					hexTo.Y(pCopyCBuilding->m_aiDataOut[CAI_LOC_Y]);
					// and family type
					iBldgType = 
						pCopyCBuilding->m_aiDataOut[CAI_BUILDTYPE];

					// go thru possible building types for testing
					for( i=0; i<3; ++i )
					{
						// is this a valid type for testing?
						if( aiBldg[i] > 0 && 
							aiBldg[i] < CStructureData::num_types )
						{
							// is this a member of the group of type of
							// the building we want?
							if( iBldgType == aiBldg[i] )
							{
								iDist =
									pGameData->GetRangeDistance( 
									hexFrom, hexTo );
							
								// farms need 2 times bufferzone
								if( aiBldg[i] == CStructureData::farm )
								{
									if( iDist < (2 * m_iBufferSize) )
										iDist = m_iMapSize;
								}
								else if( iDist < m_iBufferSize )
									iDist = m_iMapSize;

								if( iDist && iDist < aiDist[i] )
									aiDist[i] = iDist;
							}
						}
					}	// end of for each building type to test for
				}
			}
		}
	}
	// now combine the closest distances
	for( i=0; i<3; ++i )
	{
		// only if there was a valid distance
		if( aiDist[i] < m_iMapSize )
			iBestDist += aiDist[i];
	}
	// there were no other buildings to compare to
	// so rate distance to base hex from passed hex
	//if( iBestDist == m_iMapSize )
	if( !iBestDist )
	{
		// if a zero distance, that means there
		// are no desired buildings for this building
		// to be compared, so get distance to base hex
		//hexTo.X(m_iBaseX);
		//hexTo.Y(m_iBaseY);
		hexTo = m_RocketHex;
		iBestDist = pGameData->GetRangeDistance( hexFrom, hexTo );
	}
	return( iBestDist );
}

//
// find a direct path to the paiHex from the hexFrom
// for the passed vehicle type
//
BOOL CAIMapUtil::GetPathRating( CHexCoord& hexFrom, CHexCoord& hexTo,
	int iVehType /*= CTransportData::construction*/ )
{
	// 0,0 means no truck involved, this is a placement
	if( !hexFrom.X() && !hexFrom.Y() )
		return TRUE;
	if( hexFrom.X() > m_wEndCol ||
		hexFrom.Y() > m_wEndRow )
		return FALSE;
	if( hexTo.X() > m_wEndCol ||
		hexTo.Y() > m_wEndRow )
		return FALSE;

	return( thePathMap.GetPath(hexFrom, hexTo, 
		0, 0, m_pMap, iVehType) );
		//m_wBaseCol, m_wBaseRow, m_pMap, iVehType) );
}

//
// count the hexes in the passed area that can be entered
// by land vehicles (CTransportData const *) stored in this object
//
int CAIMapUtil::GetEnterableHexes( CHexCoord hexStart, CHexCoord hexEnd )
{
	CHexCoord hex;
	int iDeltaX = hex.Wrap(hexEnd.X() - hexStart.X());
	int iDeltaY = hex.Wrap(hexEnd.Y() - hexStart.Y());
	//int iDeltaX = abs( hex.Diff( hexEnd.X() - hexStart.X() ));
	//int iDeltaY = abs( hex.Diff( hexEnd.Y() - hexStart.Y() ));

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nGetEnterableHexes() player %d ", m_iPlayer );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"in area of %d,%d to %d,%d ", 
		hexStart.X(), hexStart.Y(), hexEnd.X(), hexEnd.Y() );
#endif

	int iCnt = 0;
	CHex *pGameHex;
	for( int iy=0; iy<iDeltaY; iy++ )
	{
		hex.Y( hex.Wrap(hexStart.Y() + iy) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
		myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

		for( int ix=0; ix<iDeltaX; ix++ )
		{
			hex.X( hex.Wrap(hexStart.X() + ix) );

			pGameHex = theMap.GetHex( hex );
			if( pGameHex == NULL )
				continue;

			//if( pGameHex->GetType() == CHex::city ||
			//	pGameHex->GetType() == CHex::desert ||
			//	pGameHex->GetType() == CHex::forest ||
			//	pGameHex->GetType() == CHex::plain ||
			//	pGameHex->GetType() == CHex::road ||
			//	pGameHex->GetType() == CHex::swamp )
/*
		city,		0
		desert,		1
		forest,		2
		lake,		3
		hill,		4
		mountain,	5
		ocean,		6
		plain,		7
		river,		8
		road,		9
		rough,		10
		swamp,		11
*/
			if( !m_tdWheel->CanTravelHex(pGameHex) )
			{
				TRACE( "Wheel can't travel %d,%d with %d terrain \n", 
					hex.X(), hex.Y(), (int)pGameHex->GetType() );
				continue;
			}
			if( !m_tdTrack->CanTravelHex(pGameHex) )
			{
				TRACE( "Tracked can't travel %d,%d with %d terrain \n", 
					hex.X(), hex.Y(), (int)pGameHex->GetType() );
				continue;
			}
			if( !m_tdHover->CanTravelHex(pGameHex) )
			{
				TRACE( "Hover can't travel %d,%d with %d terrain \n", 
					hex.X(), hex.Y(), (int)pGameHex->GetType() );
				continue;
			}
#if 0
			if( !m_tdFoot->CanTravelHex(pGameHex) )
			{
				TRACE( "Foot can't travel %d,%d with %d terrain \n", 
					hex.X(), hex.Y(), (int)pGameHex->GetType() );
				continue;
			}
#endif
			iCnt++;
		}
	}
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"count=%d \n", iCnt );
#endif

	return( iCnt );
}

//
// cast rays out from the base hex, in 8 directions until
// water is encountered or blocksize number of hexes checked,
// and save the closest water location encountered.  then 
// use the base hex and the closest water hex to define a
// search space for water
//
BOOL CAIMapUtil::GetClosestWaterHex( CHexCoord& hcFrom, CHexCoord& hcTo )
{
	int iDistBest = pGameData->m_iHexPerBlk;
	int iDist, iCnt;
	CHexCoord hcWater(0,0);
	//CHexCoord hcBase( m_iBaseX, m_iBaseY );
	CHexCoord hcBase = m_RocketHex;

	// for each direction
	for( int i=0; i<MAX_ADJACENT; ++i )
	{
		// cast in a straight line from the base hex
		// until either water is encountered or blocksize 
		// number of hexes are checked
		iCnt = 0;
		//CHexCoord hcRay( m_iBaseX, m_iBaseY );
		CHexCoord hcRay = m_RocketHex;

		int iYield = 0;

		while( iCnt < pGameData->m_iHexPerBlk )
		{
			if (iYield > 50)
				{
				myYieldThread();
				iYield = 0;
				}

			iCnt++;

			// move ray based on direction i
			switch(i)
			{
				case 0:	// north
					hcRay.Ydec();
					break;
				case 1:	// northeast
					hcRay.Ydec();
					hcRay.Xinc();
					break;
				case 2:
					hcRay.Xinc();
					break;
				case 3:
					hcRay.Yinc();
					hcRay.Xinc();
					break;
				case 4:
					hcRay.Yinc();
					break;
				case 5:
					hcRay.Yinc();
					hcRay.Xdec();
					break;
				case 6:
					hcRay.Xdec();
					break;
				case 7:
					hcRay.Xdec();
					hcRay.Ydec();
					break;
				default:
					return FALSE;
			}

			iDist = pGameData->GetRangeDistance( hcBase, hcRay );
			// has this ray gone far enough
			if( iDist >= iDistBest )
				break;
			
			// get terrain from game data
			CHex *pGameHex = theMap.GetHex( hcRay );
			// and if it is ocean, save hex and stop this direction
			if( pGameHex->GetType() == CHex::ocean ||
				pGameHex->GetType() == CHex::lake )
			{
				if( iDist < iDistBest )
				{
					hcWater = hcRay;
					iDistBest = iDist;
					break;
				}
			}
		}
	}

	// no water found
	if( !hcWater.X() && !hcWater.Y() )
		return FALSE;

	// now hcWater should contain the water location that is
	// closest to the base hex, so define a rect containing
	// the base hex and iDist past the water location so
	// that the rect contains both hexes, and extends out iDist
	// from the hcWater and away from hcBase
	iDist = pGameData->m_iHexPerBlk / 4;
	int iHalf = (int)(m_wCols/2);

	if( hcBase.X() < iHalf &&
		hcWater.X() >= iHalf )
	{
		hcFrom.X( hcWater.Wrap(hcWater.X()-iDist) );
		hcTo.X( hcBase.X() );
	}
	else if( hcBase.X() >= iHalf &&
		hcWater.X() < iHalf )
	{
		hcFrom.X( hcBase.X() );
		hcTo.X( hcWater.Wrap(hcWater.X()+iDist) );
	}
	else if( (hcBase.X() >= iHalf &&
		hcWater.X() >= iHalf) ||
		(hcBase.X() <= iHalf &&
		hcWater.X() <= iHalf) )
	{
		if( hcWater.X() < hcBase.X() )
		{
		hcFrom.X( hcWater.X() );
		hcTo.X( hcWater.Wrap(hcBase.X()+iDist) ); // ?
		}
		else
		{
		hcFrom.X( hcBase.X() ); // ?
		hcTo.X( hcWater.Wrap(hcWater.X()+iDist) );
		}
	}

	iHalf = (int)(m_wRows/2);
	if( hcBase.Y() < iHalf &&
		hcWater.Y() >= iHalf )
	{
		hcFrom.Y( hcWater.Wrap(hcWater.Y()-iDist) );
		hcTo.Y( hcBase.Y() );
	}
	else if( hcBase.Y() >= iHalf &&
		hcWater.Y() < iHalf )
	{
		hcFrom.Y( hcBase.Y() );
		hcTo.Y( hcWater.Wrap(hcWater.Y()+iDist) );
	}
	else if( (hcBase.Y() >= iHalf &&
		hcWater.Y() >= iHalf) ||
		(hcBase.Y() <= iHalf &&
		hcWater.Y() <= iHalf) )
	{
		if( hcWater.Y() < hcBase.Y() )
		{
		hcFrom.Y( hcWater.Y() );
		hcTo.Y( hcWater.Wrap(hcBase.Y()+iDist) ); // ?
		}
		else
		{
		hcFrom.Y( hcBase.Y() ); // ?
		hcTo.Y( hcWater.Wrap(hcWater.Y()+iDist) );
		}
	}

	return TRUE;
}

#if 1
//
// new version - performs minimum checks and accepts 1st ocean hex found
//
void CAIMapUtil::FindHexByWater( int iBldg,
	int iWidthX, int iWidthY, CHexCoord& hexFound )
{
	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData == NULL )
		return;

	CHexCoord hcFrom, hcTo, hcAt;
	CHexCoord hexBase = m_RocketHex;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nFindHexByWater() player %d for a %d \n", m_iPlayer, iBldg );
#endif

	// only looking one block in all directions
	int iMaxSteps = pGameData->m_iHexPerBlk;		
	int iStep = 1, i, iTerrain;
	int iRating = 0;
	int iBestRating = INT_MAX;
	DWORD dwEnd = theGame.GettimeGetTime () + 30 * 1000;

	while( iStep < iMaxSteps )
	{
		hcFrom.X( hexBase.Wrap(hexBase.X()-iStep) );
		hcFrom.Y( hexBase.Wrap(hexBase.Y()-iStep) );
		hcTo.X( hexBase.Wrap(hexBase.X()+iStep) );
		hcTo.Y( hexBase.Wrap(hexBase.Y()+iStep) );

		int iDeltax = hexBase.Wrap(hcTo.X()-hcFrom.X());
		int iDeltay = hexBase.Wrap(hcTo.Y()-hcFrom.Y());

		int iYield = 0;
		for( int iY=0; iY<iDeltay; ++iY )
		{
			if (iYield > 25)
				{
				myYieldThread();
				iYield = 0;
				}
			hcAt.Y( hexBase.Wrap(hcFrom.Y()+iY) );

			for( int iX=0; iX<iDeltax; ++iX )
			{
				// only get 30 seconds
				if ( theGame.GettimeGetTime () > dwEnd )
					return;

				hcAt.X( hexBase.Wrap(hcFrom.X()+iX) );

				// do not allow placing on 0,0
				if( !hcAt.X() && !hcAt.Y() )
					continue;

				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hcAt.X() != hcFrom.X() &&
					hcAt.X() != hcTo.X() && 
					hcAt.Y() != hcFrom.Y() &&
					hcAt.Y() != hcTo.Y() )
					continue;

				i = GetMapOffset( hcAt.X(), hcAt.Y() );
				if( i >= m_iMapSize )
					continue;

				// zero in map indicates nothing is there
				if( !m_pMap[i] )
				{
					CHex *pGameHex = theMap.GetHex( hcAt );
					if( pGameHex == NULL )
						continue;
				
					// consider the type of terrain from the map
					iTerrain = pGameHex->GetType();
					if( iTerrain != CHex::coastline )
						continue;

					// make sure we have a buffer zone around the hex
					if( NearBuildings( hcAt, iWidthX, iWidthY, FALSE ) )
						continue;

					// make sure there is some ocean adjacent
					if( !IsWaterAdjacent( hcTo, iWidthX, iWidthY ) )
						continue;

					// use the game's opinion of building this building
					// and check for the first passible exit orientation
					//
					iRating = 1;
					for( int j=0; j<4; ++j )
					{
						if( theMap.FoundationCost(hcTo, 
							iBldg, j, (CVehicle const *)-1) < 0 )
							continue;

						// at least one exit orientation is acceptable
						iRating = 0;
						break;
					}
					// could not find an exit orientation
					if( iRating )
					{
#if 0 //DEBUG_OUTPUT_MAPUTL
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"FindHexByWater() FoundationCost failed at %d,%d ", hcTo.X(), hcTo.Y() );
#endif
						continue;
					}

					// make sure the crane can get there
					if( !GetPathRating( hexFound, hcAt ) )
						continue;
					
					// rate distance from stuff
					if( pBldgData->GetBldgType() 
						== CStructureData::shipyard )
					{
					iRating = GetClosestTo( hcTo, 
							CStructureData::seaport, 
							CStructureData::power, 
							CStructureData::num_types);
					}
					else // CStructureData::seaport
					iRating = GetClosestTo( hcTo, 
						CStructureData::power, 
						CStructureData::seaport, 
						CStructureData::num_types) ;

					if( iRating && (iRating < iBestRating) )
					{
						iBestRating = iRating;
						hexFound = hcAt;
					}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nFindHexByWater() player %d found at %d,%d \n", m_iPlayer, hexFound.X(), hexFound.Y() );
#endif
					return;
				}
			}
		}
		iStep++;
	}
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nFindHexByWater() player %d for a %d FAILED \n", m_iPlayer, iBldg );
#endif

}
#else
//
// find the hex, adjacent to water, for the iBldg passed,
// of the width passed, and return the hex
//
void CAIMapUtil::FindHexByWater( int iBldg,
	int iWidthX, int iWidthY, CHexCoord& hexFound )
{
	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData == NULL )
		return;

	ClearWorkMap();

	CAIHex aiHex( 0, 0 );

	// consider the cells closest to the base of the map

	// this considers the map to be the same size as the world
	// so for a first try, only consider the area within the 
	// pGameData->m_iHexPerBlk size distance around the base
	BOOL bTried = FALSE;

	TryTryAgain:
	int iRating = 0;
	int iBestRating = m_iMapSize;
	int iBestHex = m_iMapSize, iCnt = 0;
	CHexCoord hcFrom, hcTo;
	int iDeltaY = 0, iDeltaX = 0;

	if( !bTried )
	{
		// get search area for water
		if( !GetClosestWaterHex( hcFrom, hcTo ) )
		{
			bTried = TRUE;
			goto TryTryAgain;
		}
	}
	else
	{
		hcFrom.X( m_wBaseCol );
		hcFrom.Y( m_wBaseRow );
		hcTo.X( m_wEndCol );
		hcTo.Y( m_wEndRow );
		iDeltaX = m_wCols;
		iDeltaY = m_wRows;
	}


#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nFindHexByWater() player %d for a %d ", m_iPlayer, iBldg );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"from %d,%d to %d,%d \n", 
		hcFrom.X(), hcFrom.Y(), hcTo.X(), hcTo.Y() );
#endif

	// need to calculate a positive delta
	// between the hcFrom/hcTo values for X,Y
	if( !iDeltaX && !iDeltaY )
	{
	iDeltaX = hcTo.Wrap(hcTo.X() - hcFrom.X());
	iDeltaY = hcTo.Wrap(hcTo.Y() - hcFrom.Y());
	//iDeltaX = abs( hcTo.Diff( hcTo.X() - hcFrom.X() ));
	//iDeltaY = abs( hcTo.Diff( hcTo.Y() - hcFrom.Y() ));
	}

	// now go thru search space defined by hcFrom/hcTo
	int i = 0;
	int iTerrain;
	for( int iY=0; iY<iDeltaY; ++iY )
	{
		// cause iY to wrap
		hcTo.Y( hcTo.Wrap(hcFrom.Y()+iY) );

		for( int iX=0; iX<iDeltaX; ++iX )
		{

			// cause iX to wrap
			hcTo.X( hcTo.Wrap(hcFrom.X()+iX) );
			
			// do not allow placing on 0,0
			if( !hcTo.X() && !hcTo.Y() )
				continue;

			//aiHex.m_iY=hcTo.Y();
			//aiHex.m_iX=hcTo.X();

			//i = GetMapOffset( aiHex.m_iX, aiHex.m_iY );
			i = GetMapOffset( hcTo.X(), hcTo.Y() );
			if( i >= m_iMapSize )
				continue;

			// zero in map indicates nothing is there
			if( !m_pMap[i] )
			{
				// don't consider sloped terrain
				//if( pGameData->GetCHexData(&aiHex) )
				//	continue;
				CHex *pGameHex = theMap.GetHex( hcTo );
				if( pGameHex == NULL )
					continue;
				
				iTerrain = pGameHex->GetType();

				// consider the type of terrain from the map
				if( iTerrain == CHex::ocean ||
					iTerrain == CHex::river ||
					iTerrain == CHex::lake ||
					iTerrain == CHex::road )
					continue;

#if THREADS_ENABLED
		// BUGBUG this function must yield
				myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				// can build here
				// is next to water?
				if( IsWaterAdjacent( hcTo, iWidthX, iWidthY ) )
				{
#if 0 //DEBUG_OUTPUT_MAPUTL
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nFindHexByWater() AI found hex by water at %d,%d ", hcTo.X(), hcTo.Y() );
#endif
					// BUGBUG now check to make sure the orientation
					// of the entrance/exit for land and ocean vehicles
					// is appropriate and if not, toss out this candidate

					// use the game's opinion of building this building
					// and check for the first passible exit orientation
					//
					iRating = 1;
					for( int j=0; j<4; ++j )
					{
						if( theMap.FoundationCost(hcTo, 
							iBldg, j, (CVehicle const *)-1) < 0 )
							continue;

						// at least one exit orientation is acceptable
						iRating = 0;
						break;
					}
					// could not find an exit orientation
					if( iRating )
					{
#if 0 //DEBUG_OUTPUT_MAPUTL
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"FindHexByWater() FoundationCost failed at %d,%d ", hcTo.X(), hcTo.Y() );
#endif
						continue;
					}

					// and we want out of the range of opfor units
					if( !IsOutOpforRange(hcTo) )
						continue;

					if( NearBuildings( hcTo, iWidthX, iWidthY, FALSE ) )
						continue;

					// make sure the crane can get there
					if( !GetPathRating( hexFound, hcTo ) )
						continue;

					// rate distance from stuff
					if( pBldgData->GetBldgType() 
						== CStructureData::shipyard )
					{
					iRating += GetClosestTo( hcTo, 
							CStructureData::seaport, 
							CStructureData::power, 
							CStructureData::num_types);
					}
					else // CStructureData::seaport
					iRating += GetClosestTo( hcTo, 
						CStructureData::power, 
						CStructureData::seaport, 
						CStructureData::num_types) ;

					if( iRating && iRating < iBestRating )
					{
						iBestRating = iRating;
						iBestHex = i;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"FindHexByWater() FoundationCost direction %d at %d,%d passed", 
j, hcTo.X(), hcTo.Y() );
#endif
					}
					// store rating 
					m_pwaWork[i] = iRating;
					iCnt++;
				}
			}
		}
	}

	if( !iCnt && bTried )
	{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nFindHexByWater() player %d for a %d FAILED \n", m_iPlayer, iBldg );
#endif
		return;
	}
	if( !iCnt )
	{
#if 0 //DEBUG_OUTPUT_MAPUTL
		ReportPavedRoads();
#endif
		bTried = TRUE;
		goto TryTryAgain;
	}
	

	// only one candidate, so take it
	if( iCnt == 1 )
	{
		OffsetToXY( iBestHex, &aiHex.m_iX, &aiHex.m_iY );
		hexFound.X(aiHex.m_iX);
		hexFound.Y(aiHex.m_iY);
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nFindHexByWater() player first found at %d,%d  with iBestHex=%d \n", 
hexFound.X(), hexFound.Y(), iBestHex );
#endif
		return;
	}

	iBestHex = m_iMapSize;
	iCnt = 0;

#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	for( i=0; i<m_iMapSize; ++i )
	{
		// count ones with the lowest rating
		if( m_pwaWork[i] == iBestRating )
		{
			iCnt++;
			iBestHex = i;
		}
		else
			m_pwaWork[i] = 0;
	}

	// could not find an appropriate hex
	if( !iCnt && bTried )
	{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nFindHexByWater() player %d for a %d FAILED \n", m_iPlayer, iBldg );
#endif
		return;
	}

	if( !iCnt )
	{
		bTried = TRUE;
		goto TryTryAgain;
	}

	// if more than one hex was nearest, then pick one
	if( iCnt > 1 )
	{
		iBestHex = pGameData->GetRandom( iCnt );
		iBestHex = FindNth( iBestHex );
	}
	
	OffsetToXY( iBestHex, &aiHex.m_iX, &aiHex.m_iY );
	hexFound.X(aiHex.m_iX);
	hexFound.Y(aiHex.m_iY);

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nFindHexByWater() player last found at %d,%d  with iBestHex=%d \n", 
hexFound.X(), hexFound.Y(), iBestHex );
#endif
}
#endif

//
// select a different base hex based on the type
// of building that is passed
//
void CAIMapUtil::AdjustInCityHex( int iBldg, CHexCoord& hexBase )
{
/*
	barracks_2,		use smelter as base
	barracks_3,     // obsolete
	heavy,
	light_0,
	light_1,
	light_2,

	power_1,		use coal as base
	power_2,		use oil_well as base
	refinery,		use oil_well as base

	smelter,		use iron or coal as base

*/
	int iNearBldg = 0;
	int iThisBldg = 0;
	int iBestDist = m_iMapSize, iDist;
	CHexCoord hexClose = m_RocketHex;

	switch(iBldg)
	{
		case CStructureData::light_0 :
		case CStructureData::light_1 :
		case CStructureData::light_2 :
		case CStructureData::heavy :
		//case CStructureData::barracks_3 :
		case CStructureData::barracks_2 :
			iNearBldg = CStructureData::smelter;
			iThisBldg = iBldg;
			break;
		case CStructureData::power_1 :
			iNearBldg = CStructureData::coal;
			iThisBldg = iBldg;
			break;
		case CStructureData::power_2 :
		case CStructureData::refinery :
			iNearBldg = CStructureData::oil_well;
			iThisBldg = iBldg;
			break;
		case CStructureData::smelter :
			iNearBldg = CStructureData::iron;
			iThisBldg = CStructureData::coal;
			break;
		default:
			return;
	}

	// not a building type we are interested in
	if( !iNearBldg )
		return;

	CHexCoord hexBldg(0,0);
	POSITION pos = m_plUnits->GetHeadPosition();
	while( pos != NULL )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// if this is a building
			if( pUnit->GetType() == CUnit::building )
			{
				// look for specific related buildings
				// or another building of this type
				if( pUnit->GetTypeUnit() == iNearBldg ||
					pUnit->GetTypeUnit() == iThisBldg )
				{
					EnterCriticalSection (&cs);
					CBuilding *pBldg = 
						theBuildingMap.GetBldg( pUnit->GetID() );
					if( pBldg != NULL )
						hexBldg = pBldg->GetExitHex();
					LeaveCriticalSection (&cs);
				}
				else
					continue;

				// a building desired, was found
				if( !hexBldg.X() && !hexBldg.Y() )
					continue;
				// find the one closest to the rocket
				iDist = pGameData->GetRangeDistance( hexBldg, m_RocketHex );
				if( iDist && iDist < iBestDist )
				{
					iBestDist = iDist;
					hexClose = hexBldg;
				}
			}
		}
	}
	hexBase = hexClose;
}

// this version determines the actual hexes in the spiral
//
#if 0
//
// pick best hex, for building passed, within a section of map
// that is non-road and non-OPFOR building areas that is located
// within the scope of the city
//
void CAIMapUtil::FindHexInCity( int iBldg,
	int iWidthX, int iWidthY, CHexCoord& hexFound )
{

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIMapUtil::FindHexInCity() player %d for a %d ", 
		m_iPlayer, iBldg );
#endif

	// for vehicle factories, need to check access for all
	// wheel types
	BOOL bVehFactory = FALSE;
	if( iBldg == CStructureData::heavy ||
		iBldg == CStructureData::light_0 ||
		iBldg == CStructureData::light_1 ||
		iBldg == CStructureData::light_2 )
		bVehFactory = TRUE;

	// check for these damn, worthless buildings
	BOOL bCitizen = FALSE;
	if( iBldg >= CStructureData::apartment_1_1 &&
		iBldg <= CStructureData::office_3_2 )
		bCitizen = TRUE;
			
	
	//CHexCoord hexBase( m_iBaseX, m_iBaseY );
	CHexCoord hexBase = m_RocketHex;

	// considering the type of building to build, make
	// an adjustment to the base location to begin from
	AdjustInCityHex( iBldg, hexBase );

	CHexCoord hex;
	WORD wTest = MSW_STAGING |
		MSW_ROAD | MSW_RESOURCE | MSW_AI_BUILDING | MSW_OPFOR_BUILDING;

	// initialize member variable with incoming data
	m_iBldg = iBldg;
	m_bVehFactory = bVehFactory;
	m_bCitizen = bCitizen;
	m_iWidth = iWidthX;
	m_iHeight = iWidthY;
	m_hexFound.X( hexFound.X() );
	m_hexFound.Y( hexFound.Y() );
	m_wTest = wTest;

	// use a spiral search outward from base hex

	int iStep = 1, iEnd;
	while( iStep < pGameData->m_iHexPerBlk )
	{
		// do row across the top left to right
		hex.Y( hex.Wrap(hexBase.Y()-iStep) );
		for( int x=(iStep * -1); x<=iStep; ++x )
		{
			hex.X( hex.Wrap(hexBase.X()+x) );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexInCity() testing %d,%d iStep=%d", hex.X(), hex.Y(), iStep );
#endif
			if( IsHexInCity(hex) )
			{
				hexFound = hex;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexInCity() player %d found %d,%d \n", 
	m_iPlayer, hexFound.X(), hexFound.Y() );
#endif
				return;
			}
		}

		// do col down right top to bottom
		hex.X( hex.Wrap(hexBase.X()+iStep) );
		iEnd = (iStep-1);
		for( int y=((iStep-1) * -1); y<=iEnd; ++y )
		{
			hex.Y( hex.Wrap(hexBase.Y()+y) );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexInCity() testing %d,%d iStep=%d", hex.X(), hex.Y(), iStep );
#endif
			if( IsHexInCity(hex) )
			{
				hexFound = hex;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexInCity() player %d found %d,%d \n", 
	m_iPlayer, hexFound.X(), hexFound.Y() );
#endif
				return;
			}
		}

		// do row across the bottom right to left
		hex.Y( hex.Wrap(hexBase.Y()+iStep) );
		iEnd = (iStep * -1);
		for( x=iStep; x>=iEnd; --x )
		{
			hex.X( hex.Wrap(hexBase.X()+x) );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexInCity() testing %d,%d iStep=%d", hex.X(), hex.Y(), iStep );
#endif
			if( IsHexInCity(hex) )
			{
				hexFound = hex;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexInCity() player %d found %d,%d \n", 
	m_iPlayer, hexFound.X(), hexFound.Y() );
#endif
				return;
			}
		}

		// do col down left from bottom to top
		hex.X( hex.Wrap(hexBase.X()-iStep) );
		iEnd = ((iStep-1) * -1);
		for( y=(iStep-1); y>=iEnd; --y )
		{
			hex.Y( hex.Wrap(hexBase.Y()+y) );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexInCity() testing %d,%d iStep=%d", hex.X(), hex.Y(), iStep );
#endif
			if( IsHexInCity(hex) )
			{
				hexFound = hex;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexInCity() player %d found %d,%d \n", 
	m_iPlayer, hexFound.X(), hexFound.Y() );
#endif
				return;
			}
		}

		iStep++;
	}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"CAIMapUtil::FindHexInCity() player %d failed to find hex \n", m_iPlayer );
#endif
}
#endif

//
// called by new version of FindHexInCity() to perform same
// tests on hex passed as done in the old version
//
BOOL CAIMapUtil::IsHexInCity( CHexCoord& hexTest )
{
	int i = GetMapOffset( hexTest.X(), hexTest.Y() );
	if( i < m_iMapSize )
	{
		// skip hex if buildings,roads,minerals are there
		if( m_pMap[i] & m_wTest )
			return FALSE;

		// put apts and offices in poor terrain
		if( m_bCitizen )
		{
			// check terrain at hex, and make sure its poor terrain
			if( !IsCitizenTerrain(hexTest, m_iWidth, m_iHeight) )
				return FALSE;
		}

		// get AI's opinion of building here
		if( !AreHexesOpen( MAX_ADJACENT, hexTest, 
			m_iWidth, m_iHeight ) )
			return FALSE;

		// make sure hex is buffersize away from other buildings
		if( NearBuildings( hexTest, m_iWidth, m_iHeight, FALSE ) )
			return FALSE;
			
		// use the game's opinion of building this building
		if( theMap.FoundationCost(hexTest, 
			m_iBldg, 0, (CVehicle const *)-1) < 0 )
			return FALSE;

		// create the hex to be tested as the destination
		if( !GetPathRating( m_hexFound, hexTest ) )
			return FALSE;

		// and we want out of the range of opfor units
		if( !IsOutOpforRange(hexTest) )
			return FALSE;

		// additional testing for vehicle factories
		if( m_bVehFactory )
		{
			// convert test hex to possible exit hex
			//
			// this may now be wrong considering the 
			// way the game has been redesigned because
			// the exit may be in a different place
			CHexCoord hexExit;
			hexExit.X( hexTest.Wrap( hexTest.X()+m_iWidth ));
			hexExit.Y( hexTest.Wrap( hexTest.Y()+m_iHeight ));

			// check all vehicle types
			if( !GetPathRating(m_hexFound, hexExit, -1) )
				return FALSE;

			// force factories out of rough,hill,mountain
			CHex *pHex = theMap.GetHex( hexTest );
			if( !m_tdTrack->CanTravelHex(pHex) )
				return FALSE;
			if( !m_tdHover->CanTravelHex(pHex) )
				return FALSE;
			if( !m_tdFoot->CanTravelHex(pHex) )
				return FALSE;
		}

		// test hex if it is closer to this player's
		// rocket than an opfor's rocket
		if( !GetRocketRating(hexTest,TRUE) )
			return FALSE;

		// if we are still here, take it!
		return TRUE;
	}
	return FALSE;
}

// this version just skips hexes not on the spiral boundary
//
#if 1
//
// pick best hex, for building passed, within a section of map
// that is non-road and non-OPFOR building areas that is located
// within the scope of the city
//
void CAIMapUtil::FindHexInCity( int iBldg,
	int iWidthX, int iWidthY, CHexCoord& hexFound )
{

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIMapUtil::FindHexInCity() player %d for a %d ", 
		m_iPlayer, iBldg );
#endif

	// for vehicle factories, need to check access for all
	// wheel types
	BOOL bVehFactory = FALSE;
	if( iBldg == CStructureData::heavy ||
		iBldg == CStructureData::light_0 ||
		iBldg == CStructureData::light_1 ||
		iBldg == CStructureData::light_2 )
		bVehFactory = TRUE;

	// check for these damn, worthless buildings
	BOOL bCitizen = FALSE;
	if( iBldg >= CStructureData::apartment_1_1 &&
		iBldg <= CStructureData::office_3_2 )
		bCitizen = TRUE;
			

	//CHexCoord hexBase( m_iBaseX, m_iBaseY );
	CHexCoord hexBase = m_RocketHex;

	// considering the type of building to build, make
	// an adjustment to the base location to begin from
	AdjustInCityHex( iBldg, hexBase );

	CHexCoord hcFrom, hcTo, hex;
	WORD wTest = MSW_STAGING |
		MSW_ROAD | MSW_RESOURCE | MSW_AI_BUILDING | MSW_OPFOR_BUILDING;

	// use a spiral search outward from base hex
	CAIHex aiHex;
	int iStep = 1;

	while( iStep < pGameData->m_iHexPerBlk )
	{
		hcFrom.X( hex.Wrap(hexBase.X()-iStep) );
		hcFrom.Y( hex.Wrap(hexBase.Y()-iStep) );
		hcTo.X( hex.Wrap(hexBase.X()+iStep) );
		hcTo.Y( hex.Wrap(hexBase.Y()+iStep) );

		int iDeltax = hex.Wrap(hcTo.X()-hcFrom.X());
		int iDeltay = hex.Wrap(hcTo.Y()-hcFrom.Y());
		//int iDeltax = abs( hex.Diff(hcTo.X()-hcFrom.X()) ) + 1;
		//int iDeltay = abs( hex.Diff(hcTo.Y()-hcFrom.Y()) ) + 1;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIMapUtil::FindHexInCity() player %d from %d,%d to %d,%d  iStep=%d ", 
m_iPlayer, hcFrom.X(), hcFrom.Y(), hcTo.X(), hcTo.Y(), iStep );
#endif

		// allow apts/ofcs anywhere after 1/2
		if( iStep > (pGameData->m_iHexPerBlk/2) )
			bCitizen = FALSE;


		// otherwise, my original spiral algorithm is used
		for( int iY=0; iY<iDeltay; ++iY )
		{
			hex.Y( hex.Wrap(hcFrom.Y()+iY) );

			for( int iX=0; iX<iDeltax; ++iX )
			{
				hex.X( hex.Wrap(hcFrom.X()+iX) );

				// do not allow placing on 0,0
				if( !hex.X() && !hex.Y() )
					continue;

				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hex.X() != hcFrom.X() &&
					hex.X() != hcTo.X() && 
					hex.Y() != hcFrom.Y() &&
					hex.Y() != hcTo.Y() )
					continue;

				myYieldThread();

				int i = GetMapOffset( hex.X(), hex.Y() );
				if( i < m_iMapSize )
				{
					// skip hex if buildings,roads,minerals are there
					if( m_pMap[i] & wTest )
						continue;

					// put apts and offices in poor terrain
					if( bCitizen )
					{
						// check terrain at hex, and make sure its poor terrain
						if( !IsCitizenTerrain(hex, iWidthX, iWidthY) )
							continue;
					}

					// get AI's opinion of building here
					if( !AreHexesOpen( MAX_ADJACENT, hex, 
						iWidthX, iWidthY ) )
						continue;

					// make sure hex is buffersize away from other buildings
					if( NearBuildings( hex, iWidthX, iWidthY, FALSE ) )
						continue;
			
					// use the game's opinion of building this building
					if( theMap.FoundationCost(hex, 
						iBldg, 0, (CVehicle const *)-1) < 0 )
						continue;

					// create the hex to be tested as the destination
					if( !GetPathRating( hexFound, hex ) )
						continue;

					// and we want out of the range of opfor units
					if( !IsOutOpforRange(hex) )
						continue;

					// additional testing for vehicle factories
					if( bVehFactory )
					{
						// convert test hex to possible exit hex
						//
						// this may now be wrong considering the 
						// way the game has been redesigned because
						// the exit may be in a different place
						CHexCoord hexExit;
						hexExit.X( hex.Wrap( hex.X()+iWidthX ));
						hexExit.Y( hex.Wrap( hex.Y()+iWidthY ));

						// check all vehicle types
						if( !GetPathRating(hexFound, hexExit, -1) )
							continue;

						// force factories out of rough,hill,mountain
						CHex *pHex = theMap.GetHex( hex );
						if( !m_tdTrack->CanTravelHex(pHex) )
							continue;
						if( !m_tdHover->CanTravelHex(pHex) )
							continue;
						if( !m_tdFoot->CanTravelHex(pHex) )
							continue;
					}

					// test hex if it is closer to this player's
					// rocket than an opfor's rocket
					if( !GetRocketRating(hex,TRUE) )
						continue;

					// if we are still here, take it!
					//hexFound.X(aiHex.m_iX);
					//hexFound.Y(aiHex.m_iY);
					hexFound = hex;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIMapUtil::FindHexInCity() player %d found %d,%d \n", 
		m_iPlayer, hexFound.X(), hexFound.Y() );
#endif
					return;
				}
			}
		}
		// take another step on away from the start hex
		iStep++;
	}

	// if still here then a hex was not found
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"CAIMapUtil::FindHexInCity() not found \n" );
#endif
}
#endif

//
// this will select the nearest unknown hex, 
// to which the unit can travel, 
// and return it in hexFound
//
void CAIMapUtil::GetExploreHex( CAIUnit *pUnit, 
	CHexCoord& hexFound, CHexCoord& hcNow )
{
	// get pointer to vehicle type data for CanTravelHex()
	CTransportData const *pVehData = 
		pGameData->GetTransportData( pUnit->GetTypeUnit() );
	if( pVehData == NULL )
		return;

	int iStep = pVehData->_GetSpottingRange();
	int iLimit = iStep;
	
	CHexCoord hcStart,hcEnd,hex;
	hcStart.X( hcNow.Wrap(hcNow.X() - iStep) );
	hcStart.Y( hcNow.Wrap(hcNow.Y() - iStep) );
	hcEnd.X( hcNow.Wrap(hcNow.X() + iStep) );
	hcEnd.Y( hcNow.Wrap(hcNow.Y() + iStep) );

	//iStep *= 2;
	iStep <<= 1;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nGetExploreHex() player %d unit %ld considering %d,%d to %d,%d ", 
m_iPlayer,pUnit->GetID(),hcStart.X(),hcStart.Y(),hcEnd.X(), hcEnd.Y() );
#endif

	while( iLimit )
	{
		--iLimit;

		hex.X(hex.Wrap(hcStart.X() + pGameData->GetRandom(iStep)));
		hex.Y(hex.Wrap(hcStart.Y() + pGameData->GetRandom(iStep)));

		if( !hex.X() && !hex.Y() )
			continue;

		if( hex == hcNow )
			continue;

		// can this vehicle travel this hex?
		CHex *pHex = theMap.GetHex( hex );
		if( !pVehData->CanTravelHex(pHex) )
			continue;

		BYTE bUnits = pHex->GetUnits();
		// skip buildings
		if( (bUnits & CHex::bldg) )
			continue;
		// and other vehicles
		if( (bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr)) )
			continue;

		// is it known yet?
		int i = GetMapOffset( hex.X(), hex.Y() );
		if( i >= m_iMapSize )
			continue;

		if( m_pMap[i] & MSW_KNOWN )
			continue;

		// now can the vehicle get there?
		if( !GetPathRating(hcNow, hex, pVehData->GetType()) )
			continue;

		hexFound = hex;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"GetExploreHex() player %d unit %ld going to %d,%d \n", 
m_iPlayer, pUnit->GetID(), hexFound.X(), hexFound.Y() );
#endif
		return;
	}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nGetExploreHex() player %d unit %ld failed, trying to stage nearby", 
m_iPlayer, pUnit->GetID() );
#endif

	// failed, need stage nearby
	hexFound = hcNow;
	FindStagingHex( hcNow, iStep, iStep, 
		pVehData->GetType(), hexFound, FALSE );
}

//
// return a 0 if adjacent to a road, else iRating if not
//
int CAIMapUtil::RateNearRoad( CAIHex *paiHex, int iRating,
	int iWidthX, int iWidthY )
{
	int iCells[MAX_ADJACENT];
	for( int i=0; i<MAX_ADJACENT; ++i )
		iCells[i] = 0;

	// multi-hex building then only look at hexes that
	// are at postion 5,6,7,0,1
	if( iWidthX > 1 || iWidthY > 1 )
	{
		iCells[5] = 1;
		iCells[6] = 1;
		iCells[7] = 1;
		iCells[0] = 1;
		iCells[1] = 1;
	}
	else // single hex building so look at all adjacent hexes
	{
		for( i=0; i<MAX_ADJACENT; ++i )
			iCells[i] = 1;
	}

	CHexCoord hex;
	// now for cells flagged check adjacent cell in that direction
	for( i=0; i<MAX_ADJACENT; ++i )
	{
		hex.X( paiHex->m_iX );
		hex.Y( paiHex->m_iY );

		if( iCells[i] )
		{
			switch(i)
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
					hex.Xinc();
					hex.Yinc();
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
					return( iRating );
			}

			int iAt = GetMapOffset( hex.X(), hex.Y() );
			if( iAt >= m_iMapSize )
				continue;

			WORD wStatus = m_pMap[iAt];

			// if a road exists in adjacent cell then return 0
			if( (wStatus & MSW_ROAD) ||
				(wStatus & MSW_PLANNED_ROAD) )
				return(0);
		}
	}
	return( iRating );
}

// BUGBUG this function does not handle all building types!
//
// based on building type, determine other type
// of building to be closer to, and find closest
// based on their occurances and return distance
//
int CAIMapUtil::RateBuilding( int iBldg, CAIHex *paiHex )
{
	CHexCoord hFrom( paiHex->m_iX, paiHex->m_iY );
	CHexCoord hTo;
	// the building type to be adjacent to and
	// the type when there is a between need
	int iNext, iOther;
	iOther = CStructureData::num_types;

/*
		enum { city,
			rocket,
			apartment_1_1,			// #=frontier/estab/city, #=choice
			apartment_1_2,
			apartment_2_1,
			apartment_2_2,
			apartment_2_3,
			apartment_2_4,
			apartment_3_1,
			apartment_3_2,
			office_2_1,
			office_2_2,
			office_2_3,
			office_2_4,
			office_3_1,
			office_3_2,
			// above are only placed by computer
			barracks_2,
			barracks_3,	// obsolete
			command_center,
			embassy,
			farm,
			fort_1,
			fort_2,
			fort_3,
			goods_1,
			goods_2,
			heavy,
			light_1,
			light_2,
			lumber,
			oil_well,
			refinery,
			coal,
			iron,
			copper_1,
			copper_2,
			copper_3,
			moly,
			power_1,
			power_2,
			power_3,
			research,
			repair,
			seaport,
			shipyard_1,
			shipyard_3,
			smelter,
			warehouse_1,
			warehouse_2,
			warehouse_3 };
	enum { first_show = barracks_2 };
*/

	// BUGBUG new building types mean multiple levels
	// of each building type so we either test on all the
	// new types or we have a family value

	// determine those types of buildings this building
	// type wants to be closest to
	// BUGBUG the following is a kludge until actual
	// buildings can be made a part of the CStructureData

	// BUGBUG the building types are not correct
	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData == NULL )
		return(m_iMapSize);

	switch( pBldgData->GetBldgType() )
	{
		case CStructureData::farm:		// these buildings need
		case CStructureData::lumber:		// to be placed using other
		case CStructureData::coal:		// considerations
		case CStructureData::iron:
		case CStructureData::copper:
		case CStructureData::power:
		case CStructureData::seaport:
		case CStructureData::fort:
		case CStructureData::embassy:
			return( m_iMapSize );
		case CStructureData::apartment:
			iNext = CStructureData::apartment;
			iOther = CStructureData::power;
			break;
		case CStructureData::warehouse:
			iNext = CStructureData::warehouse;
			iOther = CStructureData::power;
			break;
		case CStructureData::research:
			iNext = CStructureData::power;
			break;
		case CStructureData::smelter:
			iNext = CStructureData::power;
			iOther = CStructureData::iron;
			break;
		case CStructureData::light:
			iNext = CStructureData::power;
			iOther = CStructureData::heavy;
			break;
		case CStructureData::heavy:
			iNext = CStructureData::power;
			iOther = CStructureData::light;
			break;
		//case CStructureData::goods:
		//	iNext = CStructureData::smelter;
		//	break;
		case CStructureData::shipyard:
			iNext = CStructureData::seaport;
			iOther = CStructureData::power;
			break;
		case CStructureData::barracks:
			iNext = CStructureData::light;
			iOther = CStructureData::power;
			break;
		case CStructureData::command_center:
		case CStructureData::repair:
			iNext = CStructureData::barracks;
			iOther = CStructureData::power;
			break;
		default:
			return(m_iMapSize);
	}

	// process the m_pMap looking for those types of
	// desired building and store location in hTo
	int iBestDist = 0;
	//int iBestHex = m_iMapSize;
	//int ix, iy;
	int iNextDist = m_iMapSize;
	int iOtherDist = m_iMapSize;
	int iDist;

	// for each unit of this map
	//    if this building is one of the desired types
	//    then get distance from test hex and check for
	//         lowest range (that is greater than buffersize)
	// 
	POSITION pos = m_plUnits->GetHeadPosition();
	while( pos != NULL )
	{
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// if this is a building
			if( pUnit->GetType() == CUnit::building )
			{

#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				// BUGBUG change to use a value in CAIUnit
				// get access to CBuilding copied data for building?
				CAICopy *pCopyCBuilding = pUnit->GetCopyData( 
					CAICopy::CBuilding );
				if( pCopyCBuilding != NULL )
				{
					// get location of building
					hTo.X(pCopyCBuilding->m_aiDataOut[CAI_LOC_X]);
					hTo.Y(pCopyCBuilding->m_aiDataOut[CAI_LOC_Y]);

					// is this a member of the group of type of
					// the building we want?
					if( pCopyCBuilding->m_aiDataOut[CAI_BUILDTYPE]
						== iNext )
					{
						iDist =
							pGameData->GetRangeDistance( hFrom, hTo );
						if( iDist < m_iBufferSize )
							continue;
						if( iDist < iNextDist )
							iNextDist = iDist;
					}
					else if( pCopyCBuilding->m_aiDataOut[CAI_BUILDTYPE]
						== iOther )
					{
						iDist =
							pGameData->GetRangeDistance( hFrom, hTo );
						if( iDist < m_iBufferSize )
							continue;
						if( iDist < iOtherDist )
							iOtherDist = iDist;
					}
				}
			}
		}
	}
	// now combine the closest distances
	if( iNextDist < m_iMapSize )
		iBestDist += iNextDist;
	if( iOtherDist < m_iMapSize )
		iBestDist += iOtherDist;


	// there were no other buildings to compare to
	// so rate distance to base hex from passed hex
	//if( iBestDist == m_iMapSize )
	if( !iBestDist )
	{
		// if a zero distance, that means there
		// are no desired buildings for this building
		// to be compared, so get distance to base hex
		//hTo.X(m_iBaseX);
		//hTo.Y(m_iBaseY);
		hTo = m_RocketHex;
		iBestDist = pGameData->GetRangeDistance( hFrom, hTo );
	}

	return( iBestDist );
}

//
// consider the buildings that this iBldg should be close
// to and form an area (scope) that is made of the lowest
// and highest location that includes those buildings
//
BOOL CAIMapUtil::FindBldgScope( CRect *prScope, int iBldg )
{
	// load the building family types into an array for processing
	int aiBldg[3];
	for( int i=0; i<3; ++i )
		aiBldg[i]=CStructureData::num_types;

	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData == NULL )
		return FALSE;

	switch( pBldgData->GetBldgType() )
	{
		case CStructureData::apartment:
			aiBldg[0] = CStructureData::apartment;
			aiBldg[1] = CStructureData::office;
			aiBldg[2] = CStructureData::power;
			break;
		case CStructureData::research:
			aiBldg[0] = CStructureData::command_center;
			aiBldg[1] = CStructureData::rocket;
			aiBldg[2] = CStructureData::power;
			break;
		case CStructureData::smelter:
			aiBldg[0] = CStructureData::iron;
			aiBldg[1] = CStructureData::coal;
			aiBldg[2] = CStructureData::power;
			break;
		case CStructureData::light:
			aiBldg[0] = CStructureData::smelter;
			aiBldg[1] = CStructureData::heavy;
			aiBldg[2] = CStructureData::power;
			break;
		case CStructureData::heavy:
			aiBldg[0] = CStructureData::smelter;
			aiBldg[1] = CStructureData::light;
			aiBldg[2] = CStructureData::power;
			break;
		case CStructureData::barracks:
			aiBldg[0] = CStructureData::light;
			aiBldg[1] = CStructureData::heavy;
			aiBldg[2] = CStructureData::command_center;
			break;
		case CStructureData::command_center:
			aiBldg[0] = CStructureData::light;
			aiBldg[1] = CStructureData::heavy;
			aiBldg[2] = CStructureData::barracks;
			break;
		case CStructureData::repair:
			aiBldg[0] = CStructureData::smelter;
			aiBldg[1] = CStructureData::command_center;
			aiBldg[2] = CStructureData::power;
			break;
		default:
			return FALSE;
	}

	// set initial values
	prScope->top = m_wEndRow;
	prScope->left = m_wEndCol;
	prScope->bottom = m_wBaseRow;
	prScope->right = m_wBaseCol;

	int ix, iy, iSX, iSY, iEX, iEY;
	int iWidth=0, iHeight=0;
	//CHexCoord hcStart, hcEnd, hex;

	// go through the buildings of this map
	POSITION pos = m_plUnits->GetHeadPosition();
	while( pos != NULL )
	{
        CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
        if( pUnit != NULL )
        {
          	ASSERT_VALID( pUnit );
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// if this is a building
			if( pUnit->GetType() == CUnit::building )
			{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				// BUGBUG change to use a value in CAIUnit
				// get access to CBuilding copied data for building?
				CAICopy *pCopyCBuilding = pUnit->GetCopyData( 
					CAICopy::CBuilding );
				if( pCopyCBuilding != NULL )
				{
					// go thru possible building types for testing
					for( i=0; i<3; ++i )
					{
						// is this a valid type for testing?
						if( aiBldg[i] < CStructureData::num_types )
						{
							// is this a member of the group of type of
							// the building we want?
							if( aiBldg[i] ==
								pCopyCBuilding->m_aiDataOut[CAI_BUILDTYPE] )
							{
								ix = pCopyCBuilding->m_aiDataOut[CAI_LOC_X] ;
								iy = pCopyCBuilding->m_aiDataOut[CAI_LOC_Y] ;

								// NOTE: this cannot use CHexCoord because
								// what is less than something else will
								// not be calculatable in Wrap()
								
								// adjust potential scope boundary
								// based on size of building and buffer
								iSX = (ix - pBldgData->GetCX()) 
									- m_iBufferSize;
								//if( iSX < m_wBaseCol )
								//	iSX = m_wBaseCol;

								// *could be negative* and that is ok
								if( iSX < prScope->left )
								{
									// if less than base, a wrap occurred
									if( iSX < m_wBaseCol )
										iSX += m_wCols;
									prScope->left = iSX;
								}

								iSY = (iy - pBldgData->GetCY()) 
									- m_iBufferSize;
								//if( iSY < m_wBaseRow )
								//	iSY = m_wBaseRow;

								// *could be negative* and that is ok
								if( iSY < prScope->top )
								{
									if( iSY < m_wBaseRow )
										iSY += m_wRows;
									prScope->top = iSY;
								}


								iEX = (ix + pBldgData->GetCX()) 
									+ m_iBufferSize;
								//if( iEX > m_wEndCol )
								//	iEX = m_wEndCol;

								// probably always positive
								if( iEX > prScope->right )
								{
									// if more than end, a wrap occurred
									if( iEX > m_wEndCol )
										iEX -= m_wCols;
									prScope->right = iEX;
								}


								iEY = (iy + pBldgData->GetCY()) 
									+ m_iBufferSize;
								//if( iEY > m_wEndRow )
								//	iEY = m_wEndRow;

								if( iEY > prScope->bottom )
								{
									if( iEY > m_wEndRow )
										iEY -= m_wRows;
									prScope->bottom = iEY;
								}
							}
						}
					}
				}
			}
		}
	}
	// were any of this type found?
	if( prScope->top == m_wEndRow )
		return FALSE;
	
// BUGBUG this is reporting only
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nBuiding Scope is: " );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"From %d,%d to %d,%d \n", 
		prScope->left, prScope->top,
		prScope->right, prScope->bottom );
#endif

	return TRUE;
}

// BUGBUG this fails to consider wrapping
//
// update the boundaries of the city based on the location
// passed having higher or lower values
//
void CAIMapUtil::UpdateCityBounds( int iX, int iY )
{
	CHexCoord hcStart( m_iCitySX, m_iCitySY );
	CHexCoord hcEnd( m_iCityEX, m_iCityEY );
	CHexCoord hex( iX, iY );

	ExpandAreaWithHex( hcStart, hcEnd, hex );

	m_iCitySX = hcStart.X();
	m_iCitySY = hcStart.Y();
	m_iCityEX = hcEnd.X();
	m_iCityEY = hcEnd.Y();
}
//
// determine the upper-left and lower-right hexes of
// the area of the map defined by the planned and actual
// roads, and return them in the rect
//
void CAIMapUtil::FindCityScope( CRect *prScope )
{
	// if the base hex is used to define a city
	// then these values must be recalculated
	// with a flag, or on each call
	prScope->top = m_iCitySY;
	prScope->left = m_iCitySX;
	prScope->bottom = m_iCityEY;
	prScope->right = m_iCityEX;

// BUGBUG this is reporting only
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nPlayer %d City Scope is: ",m_iPlayer );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"From %d,%d to %d,%d \n", 
		prScope->left, prScope->top,
		prScope->right, prScope->bottom );
#endif

}

//
// find the nth occurance of a value in 
// in m_pwaWork[], when all the values to be 
// checked are
// the same.  iAt is the occurance we want
// and the offset of that occurance is
// what we are returning
//
int CAIMapUtil::FindNth( int iAt )
{
	int iCnt = 0;
	int iNth = m_iMapSize;

	// reprocess limited array, this time
	// until offset sequence == iNearHex

	for( int i=0; i<m_iMapSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		// only non-zero values should be 
		// the ones we want
		if( m_pwaWork[i] )
		{
			// when iCnt reaches iAt, we
			// have found the nth occurance
			if( iCnt >= iAt )
			{
				iNth = i;
				break;
			}
			else
				iCnt++;
		}
	}
	return( iNth );
}


//
// finds staging area close to shipyards for ships
//
void CAIMapUtil::GetWaterStagingArea( int iShipType,
	CHexCoord& hcStart, CHexCoord& hcEnd, int iWidth, int iHeight )
{
	CTransportData const *pShipData = 
		pGameData->GetTransportData( iShipType );
	if( pShipData == NULL )
		return;

	int iBest=m_iMapSize;
	int iDist;
	POSITION pos = NULL;
	
	CHexCoord hcShipYard,hcRocket(0,0),hex;
	pGameData->FindBuilding( CStructureData::rocket,m_iPlayer,hcRocket );
	if( !hcRocket.X() && !hcRocket.Y() )
		goto NoShipYard;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nGetWaterStagingArea() player %d for iWidth=%d iHeight=%d iShipType=%d  ", 
m_iPlayer, iWidth, iHeight, iShipType );
#endif

	// find the closest shipyard, to the rocket
	pos = m_plUnits->GetHeadPosition();
	while( pos != NULL )
	{
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// if this is a building
			if( pUnit->GetType() == CUnit::building )
			{
				// use location of seaport/shipyards to define starting area
				if( pUnit->GetTypeUnit() == CStructureData::shipyard_1 ||
					pUnit->GetTypeUnit() == CStructureData::shipyard_3 ||
					pUnit->GetTypeUnit() == CStructureData::seaport )
				{
					EnterCriticalSection (&cs);
					CBuilding *pBldg = 
						theBuildingMap.GetBldg( pUnit->GetID() );
					if( pBldg == NULL )
					{
						LeaveCriticalSection (&cs);
						continue;
					}
					hex = pBldg->GetExitHex();
					LeaveCriticalSection (&cs);

#if THREADS_ENABLED
		// BUGBUG this function must yield
					myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
					iDist = pGameData->GetRangeDistance( hex, hcRocket );
					if( iDist && iDist < iBest )
					{
						iBest = iDist;
						hcShipYard = hex;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"GetWaterStagingArea() player %d shipyard %ld at %d,%d ", 
m_iPlayer, pUnit->GetID(), hcShipYard.X(), hcShipYard.Y() );
#endif

					}
				}
			}
		}
	}

	// there were no shipyards, so can't find area
	NoShipYard:
	if( iBest == m_iMapSize )
	{
		hcStart.X(0);
		hcStart.Y(0);
		hcEnd.X(0);
		hcEnd.Y(0);
		return;
	}

	// using shipyard hex as a center, set staging bounds
	hcStart.X( hcShipYard.Wrap(hcShipYard.X() - iWidth) );
	hcStart.Y( hcShipYard.Wrap(hcShipYard.Y() - iHeight) );
	hcEnd.X( hcShipYard.Wrap(hcShipYard.X() + iWidth) );
	hcEnd.Y( hcShipYard.Wrap(hcShipYard.Y() + iHeight) );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"GetWaterStagingArea() player %d from %d,%d to %d,%d \n", 
m_iPlayer, hcStart.X(), hcStart.Y(), hcEnd.X(), hcEnd.Y() );
#endif
}

void CAIMapUtil::ExpandAreaWithHex( CHexCoord& hcStart, CHexCoord& hcEnd, 
	CHexCoord& hex )
{
	// first make sure that expansion has started already
	if( hcStart.X() == (int)m_wEndCol )
		hcStart.X( hex.X() );
	if( hcStart.Y() == (int)m_wEndRow )
		hcStart.Y( hex.Y() );
	if( hcEnd.X() == (int)m_wBaseCol )
		hcEnd.X( hex.X() );
	if( hcEnd.Y() == (int)m_wBaseRow )
		hcEnd.Y( hex.Y() );

	// now deal with if hex is outside area
	// it has wrapped
	if( hcStart.X() > hcEnd.X() )
	{
		if( hex.X() < hcStart.X() &&
			hex.X() > hcEnd.X() )
		{
			if( abs(hex.Diff(hcStart.X()-hex.X())) <
				abs(hex.Diff(hex.X()-hcEnd.X())) )
				hcStart.X( hex.X() );
			else
				hcEnd.X( hex.X() );
		}
		// else hex.X() is within the area
	}
	else // not wrapped, yet
	{
		if( hex.X() < hcStart.X() ||
			hex.X() > hcEnd.X() )
		{
			if( abs(hex.Diff(hcStart.X()-hex.X())) <
				abs(hex.Diff(hex.X()-hcEnd.X())) )
				hcStart.X( hex.X() );
			else
				hcEnd.X( hex.X() );
		}
		// else hex.X() is within the area
	}

	if( hcStart.Y() > hcEnd.Y() )
	{
		if( hex.Y() < hcStart.Y() &&
			hex.Y() > hcEnd.Y() )
		{
			if( abs(hex.Diff(hcStart.Y()-hex.Y())) <
				abs(hex.Diff(hex.Y()-hcEnd.Y())) )
				hcStart.Y( hex.Y() );
			else
				hcEnd.Y( hex.Y() );
		}
		// else hex.Y() is within the area
	}
	else
	{
		if( hex.Y() < hcStart.Y() ||
			hex.Y() > hcEnd.Y() )
		{
			if( abs(hex.Diff(hcStart.Y()-hex.Y())) <
				abs(hex.Diff(hex.Y()-hcEnd.Y())) )
				hcStart.Y( hex.Y() );
			else
				hcEnd.Y( hex.Y() );
		}
		// else hex.Y() is within the area
	}
}
//
// by calculating width/height already in hexes
// applying the width to location hexBldg, to find -/+
// location that is fartherest from base hex location
// which becomes anchor if it is not another building
//
void CAIMapUtil::FindInfStagingArea( CHexCoord& hexBldg, 
	CHexCoord& hcFrom, CHexCoord& hcTo )
{
	//int iHeight = abs( hcTo.Diff( hcTo.Y() - hcFrom.Y() ));
	//int iWidth = abs( hcTo.Diff( hcTo.X() - hcFrom.X() ));
	int iWidth = hcTo.Wrap(hcTo.X() - hcFrom.X());
	int iHeight = hcTo.Wrap(hcTo.Y() - hcFrom.Y());

	//CHexCoord hexBase( m_iBaseX, m_iBaseY );
	CHexCoord hexBase = m_RocketHex;
	CHexCoord hex,hexBest;

	// now using hexBldg and the iHeight/iWidth find the
	// hex location -/+ the width/height that is fartherest
	// from hexBase and that hex becomes the corner anchor
	// a staging area based on which direction is best
	int iDeltaX = 0;
	int iDeltaY = 0;
	int iRange,iBest=0;
	int iDir = 4; // 0=UL,1=UR,2=LR,3=LL

	hex.X( hex.Wrap(hexBldg.X() - iWidth) );
	hex.Y( hex.Wrap(hexBldg.Y() - iHeight) );
	iRange = pGameData->GetRangeDistance( hex, hexBase );
	if( iRange > iBest )
	{
		iRange = iBest;
		iDir = 0;
		hexBest = hex;
	}

	hex.X( hex.Wrap(hexBldg.X() + iWidth) );
	hex.Y( hex.Wrap(hexBldg.Y() - iHeight) );
	iRange = pGameData->GetRangeDistance( hex, hexBase );
	if( iRange > iBest )
	{
		iRange = iBest;
		iDir = 1;
		hexBest = hex;
	}

	hex.X( hex.Wrap(hexBldg.X() + iWidth) );
	hex.Y( hex.Wrap(hexBldg.Y() + iHeight) );
	iRange = pGameData->GetRangeDistance( hex, hexBase );
	if( iRange > iBest )
	{
		iRange = iBest;
		iDir = 2;
		hexBest = hex;
	}

	hex.X( hex.Wrap(hexBldg.X() - iWidth) );
	hex.Y( hex.Wrap(hexBldg.Y() + iHeight) );
	iRange = pGameData->GetRangeDistance( hex, hexBase );
	if( iRange > iBest )
	{
		iRange = iBest;
		iDir = 3;
		hexBest = hex;
	}

	if( iDir == 4 )
		return;

	// set anchor for staging area
	hexBase = hexBest;
	iBest = 0;
	// now go around new anchor in the same way and count entry cells
	// and record the best count in hcFrom->hcTo
	hex.X( hex.Wrap(hexBase.X() - iWidth) );
	hex.Y( hex.Wrap(hexBase.Y() - iHeight) );
	iRange = GetEnterableHexes( hex, hexBase );
	if( iRange > iBest )
	{
		iBest = iRange;
		hcFrom = hex;
		hcTo = hexBase;
	}

	hex.X( hexBase.X() );
	hex.Y( hex.Wrap(hexBase.Y() - iHeight) );
	hexBest.X( hexBase.Wrap(hex.X() + iWidth) );
	hexBest.Y( hexBase.Y() );
	iRange = GetEnterableHexes( hex, hexBest );
	if( iRange > iBest )
	{
		iBest = iRange;
		hcFrom = hex;
		hcTo = hexBest;
	}

	hex = hexBase;
	hexBest.X( hexBase.Wrap(hexBase.X() + iWidth) );
	hexBest.Y( hexBase.Wrap(hexBase.Y() + iHeight) );
	iRange = GetEnterableHexes( hex, hexBest );
	if( iRange > iBest )
	{
		iBest = iRange;
		hcFrom = hex;
		hcTo = hexBest;
	}

	hex.X( hexBase.Wrap(hexBase.X() - iWidth) );
	hex.Y( hexBase.Y() );
	hexBest.X( hexBase.X() );
	hexBest.Y( hexBase.Wrap(hexBase.Y() + iHeight) );
	iRange = GetEnterableHexes( hex, hexBest );
	if( iRange > iBest )
	{
		iBest = iRange;
		hcFrom = hex;
		hcTo = hexBest;
	}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nFindInfStagingArea() player %d ",m_iPlayer );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"based on %d,%d with barracks at %d,%d ", 
		hexBase.X(), hexBase.Y(), hexBldg.X(), hexBldg.Y() );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"and staging from %d,%d to %d,%d ", 
		hcFrom.X(), hcFrom.Y(), hcTo.X(), hcTo.Y() );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"city bounds are %d,%d to %d,%d \n", 
		m_iCitySX,m_iCitySY,m_iCityEX,m_iCityEY );
#endif

}

//
// spiral out from the hexNear, starting at width and height
// from it, and for each hex on the wave spiral, count the
// non-used hexes (no building, vehicles, resources, forests)
// that exist in a width x height area from that hex, that 
// can be travelled, and select the one with the most (at least
// (iWidth + iHeight) hexes in it for each step, and return TRUE
// on the first one found, or FALSE if none found by the time
// the number of steps == 1/2 the distance to opfor rockets.
//
BOOL CAIMapUtil::GetStagingAreaNear( CHexCoord& hexNear,
	CHexCoord& hcStart, CHexCoord& hcEnd, int iWidth, int iHeight, 
	int iType /*= CTransportData::combat*/ )
{
	/*
		enum TRANS_BASE_TYPE {	non_combat,
						artillery,
						troops,
						ship,
						combat };
	*/
	// first get closest opfor rocket distance
	int iMaxStep = GetClosestRocket( hexNear ) / 2;
	int iStep = max( iWidth, iHeight );

	// protect from there being no opfor rockets known
	if( iMaxStep < iStep ||
		iMaxStep >= pGameData->m_iHexPerBlk )
		iMaxStep = (iStep * 3);

	int iDeltaX, iDeltaY, iX, iY;
	int iBestCnt = iWidth + iHeight;
	if( iType != CTransportData::combat )
		iBestCnt /= 2;
	int iCnt;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nGetStagingAreaNear() player %d staging near %d,%d ", 
m_iPlayer, hexNear.X(), hexNear.Y() );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"GetStagingAreaNear() for iWidth=%d iHeight=%d iType=%d ", 
iWidth, iHeight, iType );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"GetStagingAreaNear() for iStep=%d iMaxStep=%d iBestCnt=%d ", 
iStep, iMaxStep, iBestCnt );
#endif

	CHexCoord hexStart,hexEnd,hex,hexBest(0,0);

	while( iStep < iMaxStep )
	{
		hexStart.X( hex.Wrap(hexNear.X() - iStep) );
		hexStart.Y( hex.Wrap(hexNear.Y() - iStep) );
		hexEnd.X( hex.Wrap(hexNear.X() + iStep) );
		hexEnd.Y( hex.Wrap(hexNear.Y() + iStep) );

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"GetStagingAreaNear() player %d considering %d,%d to %d,%d ", 
m_iPlayer, hexStart.X(), hexStart.Y(), hexEnd.X(), hexEnd.Y() );
#endif

		iDeltaX = hex.Wrap(hexEnd.X() - hexStart.X());
		iDeltaY = hex.Wrap(hexEnd.Y() - hexStart.Y());

		for( iY=0; iY<iDeltaY; ++iY )
		{
			hex.Y( hex.Wrap(hexStart.Y() + iY) );

			for( iX=0; iX<iDeltaX; ++iX )
			{
				hex.X( hex.Wrap(hexStart.X() + iX) );

				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hex.X() != hexStart.X() &&
					hex.X() != hexEnd.X() && 
					hex.Y() != hexStart.Y() &&
					hex.Y() != hexEnd.Y() )
					continue;

				if( !hex.X() && !hex.Y() )
					continue;


#if THREADS_ENABLED
				// BUGBUG this function must yield
				myYieldThread();
				//if( myYieldThread() == TM_QUIT )
				//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
				// now create an area from hex that is 
				// iWidth x iHeight in size and count
				// the number of staging locations
				iCnt = GetStagingCount( hex, iWidth, iHeight, iType );
				if( iCnt > iBestCnt )
				{
					hexBest = hex;
					iBestCnt = iCnt;
				}
			}
		}

		// did we find a winner?
		if( hexBest.X() || hexBest.Y() )
		{
			hcStart = hexBest;
			hcEnd.X( hex.Wrap(hcStart.X() + iWidth));
			hcEnd.Y( hex.Wrap(hcStart.Y() + iHeight));

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d selected area of %d,%d to %d,%d \n", 
		m_iPlayer, hcStart.X(), hcStart.Y(), hcEnd.X(), hcEnd.Y() );
#endif
			return( TRUE );
		}

		// before next step
		iStep++;
		hexBest.X(0);
		hexBest.Y(0);
	}
	return( FALSE );
}

//
// count the hexes, in the area defined by the passed
// hex, for the size passed, that are open for staging
// and return the count
//
int CAIMapUtil::GetStagingCount( CHexCoord& hexStart, 
	int iWidth, int iHeight, int iType )
{
	int iCnt = 0;
	CHexCoord hexEnd,hex;

	hexEnd.X( hex.Wrap(hexStart.X() + iWidth) );
	hexEnd.Y( hex.Wrap(hexStart.Y() + iHeight) );

	int iDeltaX = hex.Wrap(hexEnd.X() - hexStart.X());
	int iDeltaY = hex.Wrap(hexEnd.Y() - hexStart.Y());
	int iY, iX;

	WORD wTest = MSW_AI_VEHICLE | MSW_OPFOR_VEHICLE | MSW_STAGING |
		MSW_RESOURCE | MSW_AI_BUILDING | MSW_OPFOR_BUILDING;

	for( iY=0; iY<iDeltaY; ++iY )
	{
		hex.Y( hex.Wrap(hexStart.Y() + iY) );

		for( iX=0; iX<iDeltaX; ++iX )
		{
			hex.X( hex.Wrap(hexStart.X() + iX) );

			// don't allow 0,0 to have an effect
			if( !hex.X() && !hex.Y() )
				continue;

			int i = GetMapOffset( hex.X(), hex.Y() );
			if( i < m_iMapSize )
			{
				// skip hex if buildings,roads,minerals are there
				if( m_pMap[i] & wTest )
					continue;

				// skip hex if any of the combat vehicles can't
				CHex *pHex = theMap.GetHex( hex );

				if( iType == CTransportData::combat )
				{
					if( !m_tdTrack->CanTravelHex(pHex) )
						continue;
					if( !m_tdHover->CanTravelHex(pHex) )
						continue;
					if( !m_tdFoot->CanTravelHex(pHex) )
						continue;
				}
				else
				{
					if( !m_tdShip->CanTravelHex(pHex) )
						continue;
				}

				// count this hex in the area
				iCnt++;
			}
		}
	}
	return( iCnt );
}

//
// using the current city borders, pick a hex that is near to the city but
// not in the way, and that has the most traversable, and fewest buildable
// hexes in a group the size of iWidth x iHeight and return in prArea
//
void CAIMapUtil::GetStagingArea( CHexCoord& hcStart, CHexCoord& hcEnd,
	int iWidth, int iHeight )
{
	// rate the 4 sides of the city, via the legs of the borders
	//  by first expanding borders by 1/2 opposite side
	//  then count the CHex::ocean hexes on each leg
	//  pick the leg with the fewest 
	int iBestCnt, iBestSide;
	CHexCoord hcCity;
	
	// get current sizes of city area, and double it to get outside
	int iDeltaX = hcStart.Wrap(m_iCityEY-m_iCitySY) * 2;
	int iDeltaY = hcStart.Wrap(m_iCityEX-m_iCitySX) * 2;
	//int iDeltaY = (abs( hcStart.Diff( m_iCityEY-m_iCitySY )) * 2);
	//int iDeltaX = (abs( hcStart.Diff( m_iCityEX-m_iCitySX )) * 2);

	// now expand outward
	int ix = m_iCitySX-(iDeltaX/2);
	int iy = m_iCitySY-(iDeltaY/2);
	hcStart.X( hcCity.Wrap(ix) );
	hcStart.Y( hcCity.Wrap(iy) );
	ix = m_iCityEX+(iDeltaX/2);
	iy = m_iCityEY+(iDeltaY/2);
	hcEnd.X( hcCity.Wrap(ix) );
	hcEnd.Y( hcCity.Wrap(iy) );

	// get new length of a side after expansion
	iDeltaX = hcStart.Wrap(hcEnd.X() - hcStart.X());
	iDeltaY = hcStart.Wrap(hcEnd.Y() - hcStart.Y());
	//iDeltaY = abs( hcStart.Diff( hcEnd.Y() - hcStart.Y()) );
	//iDeltaX = abs( hcStart.Diff( hcEnd.X() - hcStart.X()) );

	// now count the CHex::ocean hexes on the new side
	// NO, instead, count the number of hexes that can
	// be entered by wheel types 2/3

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nGetStagingArea() player %d ",m_iPlayer );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"starting area of %d,%d to %d,%d ", 
		hcStart.X(), hcStart.Y(), hcEnd.X(), hcEnd.Y() );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"city bounds are %d,%d to %d,%d \n", 
		m_iCitySX,m_iCitySY,m_iCityEX,m_iCityEY );
#endif

	//iBestCnt = iDeltaX + iDeltaY;
	iBestCnt = 0;
	iBestSide = 4;
	int iCnt = 0, j;
	CHexCoord hexStart,hexEnd;
	for( int i=0; i<4; ++i )
	{
		if( i == 0 )
		{
		// get midpoint of this side
		j = abs( hcCity.Diff( hcEnd.X() - hcStart.X() )) / 2;
		hcCity.X( hcCity.Wrap(hcStart.X() + j) );
		hcCity.Y( hcStart.Y() );

		// adjust start and end by passed iWidth and iHeight
		hexStart.X( hcCity.Wrap(hcCity.X() - (iWidth/2)) );
		hexStart.Y( hcCity.Wrap(hcCity.Y() - (iHeight/2)) );
		hexEnd.X( hcCity.Wrap(hcCity.X() + (iWidth/2)) );
		hexEnd.Y( hcCity.Wrap(hcCity.Y() + (iHeight/2)) );
		}
		else if( i == 1 )
		{
		// get midpoint of this side
		j = abs(hcCity.Diff( hcEnd.Y() - hcStart.Y() )) / 2;
		hcCity.X( hcEnd.X() );
		hcCity.Y( hcCity.Wrap(hcStart.Y() + j) );

		// adjust start and end by passed iWidth and iHeight
		hexStart.X( hcCity.Wrap(hcCity.X() - (iWidth/2)) );
		hexStart.Y( hcCity.Wrap(hcCity.Y() - (iHeight/2)) );
		hexEnd.X( hcCity.Wrap(hcCity.X() + (iWidth/2)) );
		hexEnd.Y( hcCity.Wrap(hcCity.Y() + (iHeight/2)) );
		}
		else if( i == 2 )
		{
		// get midpoint of this side
		j = abs(hcCity.Diff( hcEnd.X() - hcStart.X() )) / 2;
		hcCity.X( hcCity.Wrap(hcStart.X() + j) );
		hcCity.Y( hcEnd.Y() );

		// adjust start and end by passed iWidth and iHeight
		hexStart.X( hcCity.Wrap(hcCity.X() - (iWidth/2)) );
		hexStart.Y( hcCity.Wrap(hcCity.Y() - (iHeight/2)) );
		hexEnd.X( hcCity.Wrap(hcCity.X() + (iWidth/2)) );
		hexEnd.Y( hcCity.Wrap(hcCity.Y() + (iHeight/2)) );
		}
		else if( i == 3 )
		{
		// get midpoint of this side
		j = (hcCity.Diff( hcEnd.Y() - hcStart.Y() )) / 2;
		hcCity.X( hcStart.X() );
		hcCity.Y( hcCity.Wrap(hcStart.Y() + j) );

		// adjust start and end by passed iWidth and iHeight
		hexStart.X( hcCity.Wrap(hcCity.X() - (iWidth/2)) );
		hexStart.Y( hcCity.Wrap(hcCity.Y() - (iHeight/2)) );
		hexEnd.X( hcCity.Wrap(hcCity.X() + (iWidth/2)) );
		hexEnd.Y( hcCity.Wrap(hcCity.Y() + (iHeight/2)) );
		}

		iCnt = GetEnterableHexes( hexStart, hexEnd );

		if( iCnt > iBestCnt )
		{
			iBestCnt = iCnt;
			iBestSide = i;
		}
	}


	// side with iBestSide is the one most likely to be towards
	// the open land end of the city
	if( iBestSide == 0 )
	{
		// get midpoint of this side
		i = abs( hcCity.Diff( hcEnd.X() - hcStart.X() )) / 2;
		hcCity.X( hcCity.Wrap(hcStart.X() + i) );
		hcCity.Y( hcStart.Y() );

		// adjust start and end by passed iWidth and iHeight
		hcStart.X( hcCity.Wrap(hcCity.X() - (iWidth/2)) );
		hcStart.Y( hcCity.Wrap(hcCity.Y() - (iHeight/2)) );
		hcEnd.X( hcCity.Wrap(hcCity.X() + (iWidth/2)) );
		hcEnd.Y( hcCity.Wrap(hcCity.Y() + (iHeight/2)) );
	}
	else if( iBestSide == 1 )
	{
		// get midpoint of this side
		i = abs(hcCity.Diff( hcEnd.Y() - hcStart.Y() )) / 2;
		hcCity.X( hcEnd.X() );
		hcCity.Y( hcCity.Wrap(hcStart.Y() + i) );

		// adjust start and end by passed iWidth and iHeight
		hcStart.X( hcCity.Wrap(hcCity.X() - (iWidth/2)) );
		hcStart.Y( hcCity.Wrap(hcCity.Y() - (iHeight/2)) );
		hcEnd.X( hcCity.Wrap(hcCity.X() + (iWidth/2)) );
		hcEnd.Y( hcCity.Wrap(hcCity.Y() + (iHeight/2)) );
	}
	else if( iBestSide == 2 )
	{
		// get midpoint of this side
		i = abs(hcCity.Diff( hcEnd.X() - hcStart.X() )) / 2;
		hcCity.X( hcCity.Wrap(hcStart.X() + i) );
		hcCity.Y( hcEnd.Y() );

		// adjust start and end by passed iWidth and iHeight
		hcStart.X( hcCity.Wrap(hcCity.X() - (iWidth/2)) );
		hcStart.Y( hcCity.Wrap(hcCity.Y() - (iHeight/2)) );
		hcEnd.X( hcCity.Wrap(hcCity.X() + (iWidth/2)) );
		hcEnd.Y( hcCity.Wrap(hcCity.Y() + (iHeight/2)) );
	}
	else if( iBestSide == 3 )
	{
		// get midpoint of this side
		i = (hcCity.Diff( hcEnd.Y() - hcStart.Y() )) / 2;
		hcCity.X( hcStart.X() );
		hcCity.Y( hcCity.Wrap(hcStart.Y() + i) );

		// adjust start and end by passed iWidth and iHeight
		hcStart.X( hcCity.Wrap(hcCity.X() - (iWidth/2)) );
		hcStart.Y( hcCity.Wrap(hcCity.Y() - (iHeight/2)) );
		hcEnd.X( hcCity.Wrap(hcCity.X() + (iWidth/2)) );
		hcEnd.Y( hcCity.Wrap(hcCity.Y() + (iHeight/2)) );
	}

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"player %d selected area of %d,%d to %d,%d \n", 
m_iPlayer, hcStart.X(), hcStart.Y(), hcEnd.X(), hcEnd.Y() );
#endif
}

//
// set or unset the MSW_STAGING flag to indicate that an area of the
// map has been set aside as a assault staging area which will then
// keep buildings from being placed there
//
void CAIMapUtil::FlagStagingArea( BOOL bFlag,
	int iSX, int iSY, int iEX, int iEY )
{
	CHexCoord hexStart( iSX, iSY );
	CHexCoord hexEnd( iEX, iEY );
	CHexCoord hex;
	int iDeltaX = hexStart.Wrap(hexEnd.X() - hexStart.X());
	int iDeltaY = hexStart.Wrap(hexEnd.Y() - hexStart.Y());

	// 
	WORD wStatus;
	int x,y,i;
	for( y=0; y<iDeltaY; ++y )
	{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( x=0; x<iDeltaX; ++x )
		{
			hex.X( hex.Wrap( (iSX + x) ) );
			hex.Y( hex.Wrap( (iSY + y) ) );

			i = GetMapOffset( hex.X(), hex.Y() );
			if( i >= m_iMapSize )
				continue;

			wStatus = m_pMap[i];
			if( bFlag )
			{
				if( !(wStatus & MSW_STAGING) )
					wStatus |= MSW_STAGING;
			}
			else
			{
				if( wStatus & MSW_STAGING )
					wStatus ^= MSW_STAGING;
			}
			m_pMap[i] = wStatus;
		}
	}
}

//
// used to stage a taskforce, but only on land!
//
void CAIMapUtil::FindStagingHex( 
	int iSX, int iSY, int iEX, int iEY, 
	CHexCoord& hexDest, int iVehType, BOOL bFindPath /*=FALSE*/ )
{
	// get pointer to vehicle type data
	CTransportData const *pVehData = 
		pGameData->GetTransportData( iVehType );
	if( pVehData == NULL )
		return;

	CHexCoord hexVeh = hexDest;
	CHexCoord hexStart( iSX, iSY );
	CHexCoord hexEnd( iEX, iEY );
	int iDeltaX = hexStart.Wrap(hexEnd.X() - hexStart.X());
	int iDeltaY = hexStart.Wrap(hexEnd.Y() - hexStart.Y());
	
	// compensate for landing_craft and IFVs which need to be inside
	if( iVehType == CTransportData::landing_craft ||
		iVehType == CTransportData::infantry_carrier )
	{
		// so reduce the size of the staging area, which will
		// ensure the LC/IFV is staged one hex in on all sides
		if( iDeltaX > 2 )
		{
			hexStart.Xinc();
			hexEnd.Xdec();
		}
		if( iDeltaY > 2 )
		{
			hexStart.Yinc();
			hexEnd.Ydec();
		}
		iDeltaX = hexStart.Wrap(hexEnd.X() - hexStart.X());
		iDeltaY = hexStart.Wrap(hexEnd.Y() - hexStart.Y());
	}


	CHexCoord hex;

#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"\nFindStagingHex() player %d for a %d vehicle", m_iPlayer, iVehType );
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
	"within area of %d,%d to %d,%d ", 
	hexStart.X(), hexStart.Y(), hexEnd.X(), hexEnd.Y() );
#endif

	WORD wTest = MSW_ROAD | MSW_PLANNED_ROAD | MSW_RESOURCE
		| MSW_AI_VEHICLE | MSW_AI_BUILDING;

	// set a limit on trying
	int i, iCnt = (iDeltaX + iDeltaY) * 4;
	while( iCnt > 0 )
	{
		--iCnt;

		hex.X( hex.Wrap(hexStart.X() + pGameData->GetRandom(iDeltaX)));
		hex.Y( hex.Wrap(hexStart.Y() + pGameData->GetRandom(iDeltaY)));

		if( !hex.X() && !hex.Y() )
			continue;

#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		
		i = GetMapOffset( hex.X(), hex.Y() );
		if( i >= m_iMapSize )
			continue;
		if( m_pMap[i] & wTest )
			continue;

		CHex *pGameHex = theMap.GetHex( hex );
		if( pGameHex == NULL )
			continue;
		if( !pVehData->CanTravelHex(pGameHex) )
			continue;

		if( iVehType ==
			CTransportData::landing_craft )
		{
			if( pGameHex->GetType() != CHex::coastline )
				continue;
			if( !IsLandingArea(hex) )
				continue;
		}
		if( bFindPath )
		{
			if( !GetPathRating( hexVeh, hex, iVehType ) )
				continue;
		}

		// now take it and return
		hexDest = hex;

#ifdef _LOGOUT
dwEnd = timeGetTime();
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"player %d staging at %d,%d  took %ld ticks \n", 
m_iPlayer, hexDest.X(), hexDest.Y(), (dwEnd - dwStart) );
#endif
		return;
	}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"FindStagingHex() trying all hexes " );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"iDeltaX=%d iDeltaY=%d  starting from %d,%d ", 
		iDeltaX, iDeltaY, iSX, iSY );
#endif

	// should we try to process the area cell by cell?
	
	int x,y;
	for( y=0; y<iDeltaY; ++y )
	{
		for( x=0; x<iDeltaX; ++x )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			hex.X( hex.Wrap( (hexStart.X() + x) ) );
			hex.Y( hex.Wrap( (hexStart.Y() + y) ) );

			if( !hex.X() && !hex.Y() )
				continue;


			CHex *pGameHex = theMap.GetHex( hex );
			if( pGameHex == NULL )
				continue;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"at %d,%d terrain is %d  GetUnits() is %d ", 
	hex.X(), hex.Y(), pGameHex->GetType(), (int)pGameHex->GetUnits() );
#endif

			i = GetMapOffset( hex.X(), hex.Y() );
			if( i >= m_iMapSize )
			{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"invalid offset %d ", i );
#endif
				continue;
			}

			if( m_pMap[i] & wTest )
			{
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"m_pMap[i] = %d ", m_pMap[i] );
#endif
				continue;
			}

			if( !pVehData->CanTravelHex(pGameHex) )
				continue;

			if( iVehType ==
				CTransportData::landing_craft )
			{
				if( pGameHex->GetType() != CHex::coastline )
					continue;
				if( !IsLandingArea(hex) )
					continue;
			}

			if( bFindPath )
			{
				if( !GetPathRating( hexVeh, hex, iVehType ) )
					continue;
			}

			// now take it and return
			hexDest = hex;

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"m_pMap[i] = %d ", m_pMap[i] );
#endif
			return;
		}
	}

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"staging failed after %ld ticks \n", (dwEnd - dwStart) );
#endif
}
	
//
// find an unoccupied hex, at least iWidth,iHeight distance
// from hexNearBy (considering size) and occupyable by iVehType
// and no farther than 2xSize away, and return in hexDest
//
// with the bExclude set to FALSE, the excluded area can be 
// staged into if the vehicle is able
//
void CAIMapUtil::FindStagingHex( CHexCoord& hexNearBy, 
	int iWidth, int iHeight, int iVehType, CHexCoord& hexDest, 
	BOOL bExclude /*=TRUE*/ )
{
	// get pointer to vehicle type data
	CTransportData const *pVehData = 
		pGameData->GetTransportData( iVehType );
	if( pVehData == NULL )
		return;

#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif

	int iWidthX, iWidthY;
	if( iWidth == 1 && iHeight == 1 )
	{
		iWidthX = pGameData->m_iStructureSize;
		iWidthY = pGameData->m_iStructureSize;
	}
	else
	{
		iWidthX = iWidth;
		iWidthY = iHeight;
	}

    WORD wTest = MSW_ROAD | MSW_PLANNED_ROAD | MSW_RESOURCE 
		| MSW_AI_VEHICLE | MSW_AI_BUILDING;
	BOOL bTried = FALSE;
	int iRadi = 1;	// most buildings are 2x2
	int iCnt = 0;
	int iDeltaX, iDeltaY;
	CHexCoord hex;

	// ocean only will stage out a bit
	if( pVehData->IsBoat() )
		iRadi = 2;

	// the staging area is expanded by iRadi so that it
	// is like a wave front in this case
	// 
	TryTryAgain:

	CHexCoord hcStartArea,hcEndArea,hcStartExclude,hcEndExclude;
	hcStartArea.X( hexNearBy.Wrap(hexNearBy.X() - (iRadi*iWidthX)) );
	hcStartArea.Y( hexNearBy.Wrap(hexNearBy.Y() - (iRadi*iWidthY)) );
	hcEndArea.X( hexNearBy.Wrap(hexNearBy.X() + ((iRadi+1)*iWidthX)) );
	hcEndArea.Y( hexNearBy.Wrap(hexNearBy.Y() + ((iRadi+1)*iWidthY)) );

	hcStartExclude.X( hexNearBy.Wrap(hexNearBy.X() - iWidthX) );
	hcStartExclude.Y( hexNearBy.Wrap(hexNearBy.Y() - iWidthY) );
	hcEndExclude.X( hexNearBy.Wrap(hexNearBy.X() + (2*iWidthX)) );
	hcEndExclude.Y( hexNearBy.Wrap(hexNearBy.Y() + (2*iWidthY)) );

	// no, try many times
	CHexCoord hcDelta;
	hcDelta.X( hcDelta.Wrap(hcEndArea.X() - hcStartArea.X()) );
	hcDelta.Y( hcDelta.Wrap(hcEndArea.Y() - hcStartArea.Y()) );
	iDeltaX = hcDelta.X();
	iDeltaY = hcDelta.Y();
	if( iDeltaX > (int)m_wCols )
		iDeltaX = m_wCols;
	if( iDeltaY > (int)m_wRows )
		iDeltaY = m_wRows;
	if( iDeltaX == m_wCols ||
		iDeltaY == m_wRows )
		iRadi = 10;

	iCnt = (iDeltaX+iDeltaY) * 2;

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nFindStagingHex() player %d for a %s from nearby %d,%d  iRadi=%d ", 
		m_iPlayer, (const char *)pVehData->GetDesc(), 
		hexNearBy.X(), hexNearBy.Y(), iRadi );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"within area of %d,%d to %d,%d  deltaX=%d deltaY=%d ", 
		hcStartArea.X(), hcStartArea.Y(), hcEndArea.X(), hcEndArea.Y(),
		hcDelta.X(), hcDelta.Y() );
	if( bExclude )
	{
		logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"excluding area of %d,%d to %d,%d ", 
		hcStartExclude.X(), hcStartExclude.Y(), 
		hcEndExclude.X(), hcEndExclude.Y() );
	}
#endif

	

	while( iCnt )
	{
		--iCnt;

		hex.X(hex.Wrap(hcStartArea.X() + pGameData->GetRandom(iDeltaX)));
		hex.Y(hex.Wrap(hcStartArea.Y() + pGameData->GetRandom(iDeltaY)));

		if( !hex.X() && !hex.Y() )
			continue;

		// not in staging area at all
		if( !IsHexInArea(hcStartArea, hcEndArea, hex) )
			continue;

		if( bExclude )
		{
			// or is in excluded area
			if( IsHexInArea(hcStartExclude, hcEndExclude, hex) )
				continue;
		}

		CHex *pGameHex = theMap.GetHex( hex );
		if( pGameHex == NULL )
			continue;

		BYTE bUnits = pGameHex->GetUnits();
		// skip buildings
		if( (bUnits & CHex::bldg) )
			continue;
		// and vehicles
		if( (bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr)) )
			continue;

		// don't block roads with staging vehicles
		if( pGameHex->GetType() == CHex::road )
			continue;

		// check the game to see if vehicle can travel
		if( !pVehData->CanTravelHex( pGameHex ) )
			continue;

		// convert coordinates to an offset
		int i = GetMapOffset( hex.X(), hex.Y() );
		if( i < 0 || i >= m_iMapSize )
		    continue;

		// AI says something is there, so continue
		if( m_pMap[i] & wTest )
			continue;

		// and we want these out of the range of opfor units
		if( iVehType == CTransportData::construction ||
			iVehType == CTransportData::heavy_truck )
		{
			if( !IsOutOpforRange(hex) )
				continue;
		}

#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

		// can the vehicle get there?
		if( hexNearBy.X() || hexNearBy.Y() )
		{
			if( !GetPathRating( hexNearBy, hex, iVehType ) )
				continue;
		}

		// if still here, take it and return
		hexDest = hex;

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d for a %s staging at %d,%d  took %ld ticks \n", 
		m_iPlayer, (const char *)pVehData->GetDesc(), 
		hexDest.X(), hexDest.Y(), (dwEnd - dwStart) );
#endif
		return;
	}

	// hard code so many tries, damn if by the 10th
	// try, something should give
	if( iRadi < 10 )
	{
		iRadi++;
		goto TryTryAgain;
	}


	// spiral out from hexNearBy, and select the first
	// hex that qualifies so the unit can move!
	int iY,iX;
	iCnt = max( iWidthX, iWidthY );
	while( iCnt < pGameData->m_iHexPerBlk )
	{
		hcStartArea.X( hex.Wrap(hexNearBy.X() - iCnt) );
		hcStartArea.Y( hex.Wrap(hexNearBy.Y() - iCnt) );
		hcEndArea.X( hex.Wrap(hexNearBy.X() + iCnt) );
		hcEndArea.Y( hex.Wrap(hexNearBy.Y() + iCnt) );

		iDeltaX = hex.Wrap(hcEndArea.X() - hcStartArea.X());
		iDeltaY = hex.Wrap(hcEndArea.Y() - hcStartArea.Y());

		for( iY=0; iY<iDeltaY; ++iY )
		{
			hex.Y( hex.Wrap(hcStartArea.Y() + iY) );

			for( iX=0; iX<iDeltaX; ++iX )
			{
				hex.X( hex.Wrap(hcStartArea.X() + iX) );

				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hex.X() != hcStartArea.X() &&
					hex.X() != hcEndArea.X() && 
					hex.Y() != hcStartArea.Y() &&
					hex.Y() != hcEndArea.Y() )
					continue;

				if( !hex.X() && !hex.Y() )
					continue;

				CHex *pGameHex = theMap.GetHex( hex );
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

				// convert coordinates to an offset
				int i = GetMapOffset( hex.X(), hex.Y() );
				if( i < 0 || i >= m_iMapSize )
					continue;

				// AI says something is there, so continue
				if( m_pMap[i] & wTest )
					continue;

				// and we want these out of the range of opfor units
				if( iVehType == CTransportData::construction ||
					iVehType == CTransportData::heavy_truck )
				{
					if( !IsOutOpforRange(hex) )
						continue;
				}

#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

				// can the vehicle get there?
				if( hexNearBy.X() || hexNearBy.Y() )
				{
					if( !GetPathRating( hexNearBy, hex, iVehType ) )
						continue;
				}

				// if still here, take it and return
				hexDest = hex;

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d for a %s SPIRAL staging at %d,%d  took %ld ticks \n", 
		m_iPlayer, (const char *)pVehData->GetDesc(), 
		hexDest.X(), hexDest.Y(), (dwEnd - dwStart) );
#endif
				return;
			}
		}
		iCnt++;
	}


	hexDest = hexNearBy;

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d for a %s could not stage from %d,%d  took %ld ticks \n", 
		m_iPlayer, (const char *)pVehData->GetDesc(), 
		hexDest.X(), hexDest.Y(), (dwEnd - dwStart) );
#endif
}

//
// find a suitable hex to return, that has land/water combination next to
// it and can be reached by a landing craft from hexHead
//
void CAIMapUtil::FindLandingHex(CHexCoord& hexHead)
{
	CHexCoord hexVeh = hexHead;

	// get pointer to vehicle type data
	CTransportData const *pVehData = 
		pGameData->GetTransportData( CTransportData::landing_craft );
	if( pVehData == NULL )
		return;

#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif

    WORD wTest = MSW_ROAD | MSW_PLANNED_ROAD | MSW_RESOURCE 
		| MSW_AI_VEHICLE | MSW_AI_BUILDING;

	CHexCoord hcStartArea,hcEndArea,hex;
	int iCnt = 1;
	int iLimit = pVehData->_GetSpottingRange();
	int iY,iX,iDeltaX,iDeltaY;
	// spiral out from hexVeh and select the first hex that can be
	// landed onto
	while( iCnt < iLimit )
	{
		hcStartArea.X( hex.Wrap(hexVeh.X() - iCnt) );
		hcStartArea.Y( hex.Wrap(hexVeh.Y() - iCnt) );
		hcEndArea.X( hex.Wrap(hexVeh.X() + iCnt) );
		hcEndArea.Y( hex.Wrap(hexVeh.Y() + iCnt) );
		iDeltaX = hex.Wrap(hcEndArea.X() - hcStartArea.X());
		iDeltaY = hex.Wrap(hcEndArea.Y() - hcStartArea.Y());

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nFindLandingHex() player %d for a %s from nearby %d,%d ", 
		m_iPlayer, (const char *)pVehData->GetDesc(), 
		hexVeh.X(), hexVeh.Y() );
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"within area of %d,%d to %d,%d  deltaX=%d deltaY=%d ", 
		hcStartArea.X(), hcStartArea.Y(), hcEndArea.X(), hcEndArea.Y(),
		iDeltaX, iDeltaY );
#endif

		for( iY=0; iY<iDeltaY; ++iY )
		{
			hex.Y( hex.Wrap(hcStartArea.Y() + iY) );

			for( iX=0; iX<iDeltaX; ++iX )
			{
				hex.X( hex.Wrap(hcStartArea.X() + iX) );

				// just want the borders of the area, which is 
				// the newly expanded to hexes on the edge of area
				if( hex.X() != hcStartArea.X() &&
					hex.X() != hcEndArea.X() && 
					hex.Y() != hcStartArea.Y() &&
					hex.Y() != hcEndArea.Y() )
					continue;

				if( !hex.X() && !hex.Y() )
					continue;

				CHex *pGameHex = theMap.GetHex( hex );
				if( pGameHex == NULL )
					continue;

				BYTE bUnits = pGameHex->GetUnits();
				// skip buildings
				if( (bUnits & CHex::bldg) )
					continue;
				// and vehicles
				if( (bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr)) )
					continue;

				// convert coordinates to an offset
				int i = GetMapOffset( hex.X(), hex.Y() );
				if( i < 0 || i >= m_iMapSize )
					continue;

				// AI says something is there, so continue
				if( m_pMap[i] & wTest )
					continue;

				// check the game to see if vehicle can travel
				if( !pVehData->CanTravelHex( pGameHex ) )
					continue;

				// check for land/water combination adjacent
				if( !IsLandingArea(hex) )
					continue;

				// can the LC get there?
				if( !GetPathRating( hexVeh, hex, 
					CTransportData::landing_craft ) )
					continue;

				// take it
				hexHead = hex;

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d for a %s unloading at %d,%d  took %ld ticks \n", 
		m_iPlayer, (const char *)pVehData->GetDesc(), 
		hex.X(), hex.Y(), (dwEnd - dwStart) );
#endif
				return;
			}
		}
		iCnt++;
	}

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"player %d for a %s could find landing site from %d,%d  took %ld ticks \n", 
		m_iPlayer, (const char *)pVehData->GetDesc(), 
		hexVeh.X(), hexVeh.Y(), (dwEnd - dwStart) );
#endif
}

//
// determine if the hex passed is within the bounds of the area
// formed by hcStart -> hcEnd, inclusive and considering wrapping
//
BOOL CAIMapUtil::IsHexInArea( CHexCoord& hcStart, CHexCoord& hcEnd, 
	CHexCoord& hex )
{
	int iDeltaX = hcStart.Wrap(hcEnd.X() - hcStart.X());
	int iDeltaY = hcStart.Wrap(hcEnd.Y() - hcStart.Y());
	//int iDeltaY = abs( hcStart.Diff( hcEnd.Y() - hcStart.Y()) );
	//int iDeltaX = abs( hcStart.Diff( hcEnd.X() - hcStart.X()) );

	// force inclusion at edges by increasing delta
	iDeltaY++;
	iDeltaX++;

	CHexCoord hexIn;

	int x,y;
	for( y=0; y<iDeltaY; ++y )
	{
		hexIn.Y( hexIn.Wrap( (hcStart.Y() + y) ) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
		myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( x=0; x<iDeltaX; ++x )
		{
			hexIn.X( hexIn.Wrap( (hcStart.X() + x) ) );
			
			if( hexIn == hex )
				return( TRUE );
		}
	}
	return( FALSE );
}

//
// using the location of the attacking unit, find a location for the
// targeted unit to move to, that is at spotting+range of the attacker,
// and in the best defensive terrain, and as far from other opfors as
// is possible
//
void CAIMapUtil::FindRetreatHex( CAIUnit *pFleeUnit, CHexCoord& hexFlee,
	CAIUnit *pAttacker, CHexCoord& hexAttacking )
{
	// should not have been called if pFleeUnit is NULL
	if( pFleeUnit == NULL )
		return;

	// get pointer to vehicle type data for CanTravelHex()
	CTransportData const *pVehData = 
		pGameData->GetTransportData( pFleeUnit->GetTypeUnit() );
	if( pVehData == NULL )
		return;

	int iRange,iSpotting;

	// if no attacker, then go with minimum
	if( pAttacker == NULL )
	{
		iRange = MIN_RANGE;
		iSpotting = MIN_RANGE;
	}
	else // get spotting range and attack range of attacker
	{
		if( pAttacker->GetType() == CUnit::vehicle )
		{
			EnterCriticalSection (&cs);
			CVehicle *pVehicle = 
			theVehicleMap.GetVehicle( pAttacker->GetID() );
			if( pVehicle != NULL )
			{
			iSpotting = pVehicle->GetSpottingRange();
			iRange = pVehicle->GetRange();
			}
			else
				iRange = 0;
			LeaveCriticalSection (&cs);
		}
		else if( pAttacker->GetType() == CUnit::building )
		{
			EnterCriticalSection (&cs);
			CBuilding *pBldg = 
			theBuildingMap.GetBldg( pAttacker->GetID() );
			if( pBldg != NULL )
			{
			iSpotting = pBldg->GetSpottingRange();
			iRange = pBldg->GetRange();
			}
			else
				iRange = 0;
			LeaveCriticalSection (&cs);
		}
		else
			return;
	}

	// something is wrong, so bail out
	if( !iRange )
		return;

	// set up search area spotting+attack from hexAttacking
	int iArea = iRange + iSpotting;
	CHexCoord hcStart, hcEnd, hex;

	hcStart.X( hex.Wrap(hexAttacking.X() - iArea) );
	hcStart.Y( hex.Wrap(hexAttacking.Y() - iArea) );
	hcEnd.X( hex.Wrap(hexAttacking.X() + iArea) );
	hcEnd.Y( hex.Wrap(hexAttacking.Y() + iArea) );

	// find hex on edge of search area, that is closest to hexFlee
	// with the best defensive value, that is also as far from opfor
	// units and as close to friendly units
	int iDeltaX = hex.Wrap(hcEnd.X() - hcStart.X());
	int iDeltaY = hex.Wrap(hcEnd.Y() - hcStart.Y());
	//int iDeltaX = abs( hex.Diff( hcEnd.X()-hcStart.X() )) + 1;
	//int iDeltaY = abs( hex.Diff( hcEnd.Y()-hcStart.Y() )) + 1;

	int iY,iX,i,iSupport;

	// first clear work area, and set basic ratings
	for( iY=0; iY<iDeltaY; ++iY )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iY) );

		for( iX=0; iX<iDeltaX; ++iX )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			hex.X( hex.Wrap(hcStart.X() + iX) );

			// just want the borders of the area, which is 
			// the newly expanded to hexes on the edge of area
			if( hex.X() != hcStart.X() &&
				hex.X() != hcEnd.X() && 
				hex.Y() != hcStart.Y() &&
				hex.Y() != hcEnd.Y() )
				continue;

			if( !hex.X() && !hex.Y() )
				continue;

			i = GetMapOffset( hex.X(), hex.Y() );
			if( i >= m_iMapSize )
				continue;

			m_pwaWork[i] = 0;

			// no moving back allowed
			if( hex == hexFlee )
				continue;

			// can this vehicle travel this hex?
			CHex *pHex = theMap.GetHex( hex );
			if( !pVehData->CanTravelHex(pHex) )
				continue;

			// can't flee to an occupied hex
			if( HasOpforUnits(pHex) )
				continue;

			// can the fleeing vehicle get there?
			if( !GetPathRating(hexFlee, hex, pVehData->GetType()) )
				continue;

			// should flag location if out of range from 
			// other opfor units, 1 is out of range, 0 in range
			m_pwaWork[i] = IsOutOpforRange( hex );
		}
	}

	for( iY=0; iY<iDeltaY; ++iY )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iY) );

		for( iX=0; iX<iDeltaX; ++iX )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			hex.X( hex.Wrap(hcStart.X() + iX) );

			// just want the borders of the area, which is 
			// the newly expanded to hexes on the edge of area
			if( hex.X() != hcStart.X() &&
				hex.X() != hcEnd.X() && 
				hex.Y() != hcStart.Y() &&
				hex.Y() != hcEnd.Y() )
				continue;

			if( !hex.X() && !hex.Y() )
				continue;

			i = GetMapOffset( hex.X(), hex.Y() );
			if( i >= m_iMapSize )
				continue;
			if( !m_pwaWork[i] )
				continue;

			iRange = 0;
			// rate location for distance from all this
			// player's combat units, with closer being better
			iSupport = GetSupportRange(hex, pFleeUnit);
			if( iSupport < m_iMapSize )
				iRange = iSupport;
			else
				// rate for closeness to fleeing unit location,
				iRange = pGameData->GetRangeDistance(hexFlee,hex);


			// get location defense rating, high being better
			CHex *pHex = theMap.GetHex( hex );
			CTerrainData const *pTerrain = 
				pGameData->GetTerrainData( (int)pHex->GetType() );
			if( pTerrain == NULL )
				continue;

			// come up with a 'flee' value rating for this location
			if( iRange )
			{
				m_pwaWork[i] = 
					(pTerrain->GetDefenseMult(pVehData->GetWheelType())
					* iArea) / iRange;
			}
		}
	}

	// pick the highest value
	int iBestHex = m_iMapSize;
	int iBestRating = 0;
	for( iY=0; iY<iDeltaY; ++iY )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iY) );

		for( iX=0; iX<iDeltaX; ++iX )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			hex.X( hex.Wrap(hcStart.X() + iX) );

			// just want the borders of the area, which is 
			// the newly expanded to hexes on the edge of area
			if( hex.X() != hcStart.X() &&
				hex.X() != hcEnd.X() && 
				hex.Y() != hcStart.Y() &&
				hex.Y() != hcEnd.Y() )
				continue;

			if( !hex.X() && !hex.Y() )
				continue;

			i = GetMapOffset( hex.X(), hex.Y() );
			if( i >= m_iMapSize )
				continue;
			if( !m_pwaWork[i] )
				continue;

			if( m_pwaWork[i] > iBestRating )
			{
				iBestHex = i;
				iBestRating = m_pwaWork[i];
			}
		}
	}

	// the best hex to retreat to
	if( iBestHex < m_iMapSize )
	{
		OffsetToXY( iBestHex, &iX, &iY );
		hexFlee.X( iX );
		hexFlee.Y( iY );
	}
}
//
// check this CHex for units, and if opfor unit (not m_iPlayer)
// then return TRUE, or if more than one AI unit return TRUE,
// else return FALSE
//
BOOL CAIMapUtil::HasOpforUnits( CHex *pHex )
{
	// nothing here
	BYTE bUnits = pHex->GetUnits();
	if( !bUnits )
		return FALSE;
	// don't retreat to a building
	if( bUnits & CHex::bldg )
		return TRUE;
	// somebody's units are here
	if( bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr) )
		return TRUE;

	return FALSE;
}
//
// for each opfor unit (not equal m_iPlayer)
// if in range of hex, return 0
// else if none, return 1
//
int CAIMapUtil::IsOutOpforRange( CHexCoord& hex, int iSpotting /* =0 */ )
{
	CHexCoord hexUnit;
	int iRange = 0;

	// access the m_plUnits list of known units to this
	// AI player, and then for each opfor unit by type, access the 
	// game data and then make the in-range determination
	//
	POSITION pos = m_plUnits->GetHeadPosition();
	int iYield = 0;
	while( pos != NULL )
	{
			if (iYield > 25)
				{
				myYieldThread();
				iYield = 0;
				}

       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );

			// only opfor units are desired
			if( pUnit->GetOwner() == m_iPlayer )
				continue;

			// if this is a building
			if( pUnit->GetType() == CUnit::building )
			{
				EnterCriticalSection (&cs);
				CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					// BUGBUG dave may want range calculated from
					// every hex occupied by the building
					if( !iSpotting )
					{
						iRange = pBldg->GetRange();
						if( !iRange )
						{
							LeaveCriticalSection (&cs);
							continue;
						}
					}
					else
						iRange = iSpotting;

					hexUnit = pBldg->GetHex();
					if( pGameData->GetRangeDistance(hexUnit,hex) <= iRange )
					{
						LeaveCriticalSection (&cs);
						return(0);
					}

					hexUnit = pBldg->GetExitHex();
					if( pGameData->GetRangeDistance(hexUnit,hex) <= iRange )
					{
						LeaveCriticalSection (&cs);
						return(0);
					}
				}
				LeaveCriticalSection (&cs);
			}
			else if( pUnit->GetType() == CUnit::vehicle )
			{
				// get location
				EnterCriticalSection (&cs);
				CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
				if( pVehicle != NULL )
				{
					hexUnit = pVehicle->GetHexHead();
					if( !iSpotting )
						iRange = pVehicle->GetRange();
					else
						iRange = iSpotting;

					if( pGameData->GetRangeDistance(hexUnit,hex) <= iRange )
					{
						LeaveCriticalSection (&cs);
						return(0);
					}
				}
				LeaveCriticalSection (&cs);
			}
			else
				continue;

		}
	}
	return(1);
}


//
// for each combat unit of m_iPlayer, get closest range to hex
// and return distance
//
int CAIMapUtil::GetSupportRange(CHexCoord& hex, CAIUnit *pFleeing)
{
	int iBestRange = m_iMapSize,iRange;
	CHexCoord hexUnit;

	POSITION pos = m_plUnits->GetHeadPosition();
	while( pos != NULL )
	{
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );
			if( pUnit->GetOwner() != m_iPlayer )
				continue;

			// skip the fleeing unit
			if( pFleeing->GetID() == pUnit->GetID() )
				continue;

			// if this is a building
			if( pUnit->GetType() == CUnit::building )
			{
				EnterCriticalSection (&cs);
				CBuilding *pBldg = theBuildingMap.GetBldg( pUnit->GetID() );
				if( pBldg != NULL )
				{
					// BUGBUG may need to check every hex of the building
					// for range
					hexUnit = pBldg->GetExitHex();
					// range is in CHexCoords per DT 1/23/96 phone call
					iRange = pBldg->GetRange();
				}
				else
					iRange = 0;
				LeaveCriticalSection (&cs);
			}
			else if( pUnit->GetType() == CUnit::vehicle )
			{
				// get location
				EnterCriticalSection (&cs);
				CVehicle *pVehicle = theVehicleMap.GetVehicle( pUnit->GetID() );
				if( pVehicle != NULL )
				{
					hexUnit = pVehicle->GetHexHead();
					// range is in CHexCoords per DT 1/23/96 phone call
					iRange = pVehicle->GetRange();
				}
				else
					iRange = 0;
				LeaveCriticalSection (&cs);
			}
			else
				continue;

			if( !iRange )
				continue;

			iRange = pGameData->GetRangeDistance( hexUnit, hex );
			if( iRange && iRange < iBestRange )
				iBestRange = iRange;

#if THREADS_ENABLED
		// BUGBUG this function must yield
			myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		}
	}
	return( iBestRange );
}

// pick a random defensive hex between unit and target
// not more than spotting range out from unit
//
void CAIMapUtil::FindApproachHex( CHexCoord& hexTarget, 
	CAIUnit *pUnit, CHexCoord& hexDest )
{
	// using spotting range to set wave distance, find
	// the best defensive hex that is closest to target
	// and if more than one exists, then pick one randomly
	CTransportData const *pVehData = 
		pGameData->GetTransportData( pUnit->GetTypeUnit() );
	if( pVehData == NULL )
		return;

	int iSpotting = pVehData->_GetSpottingRange();
	int iRangeToTarget = 
		pGameData->GetRangeDistance( hexDest, hexTarget ) - (iSpotting/2);

	CHexCoord hexStart,hexEnd,hexAt=hexDest;

	hexStart.X( hexAt.Wrap(hexAt.X() - iSpotting) );
	hexStart.Y( hexAt.Wrap(hexAt.Y() - iSpotting) );
	hexEnd.X( hexAt.Wrap(hexAt.X() + iSpotting) );
	hexEnd.Y( hexAt.Wrap(hexAt.Y() + iSpotting) );

	int iDeltaX = hexStart.Wrap(hexEnd.Y() - hexStart.Y());
	int iDeltaY = hexStart.Wrap(hexEnd.Y() - hexStart.Y());
	//int iDeltaY = abs( hexStart.Diff( hexEnd.Y() - hexStart.Y()) );
	//int iDeltaX = abs( hexStart.Diff( hexEnd.X() - hexStart.X()) );
	int iy,ix,iBest=m_iMapSize,iBestDist=m_iMapSize;
	int iDist,iCnt = 0;

	for( iy=0; iy<iDeltaY; iy++ )
	{
		hexAt.Y( hexAt.Wrap(hexStart.Y() + iy) );

		for( ix=0; ix<iDeltaX; ix++ )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			hexAt.X( hexAt.Wrap(hexStart.X() + ix) );

			if( hexAt.X() != hexStart.X() &&
				hexAt.X() != hexEnd.X() && 
				hexAt.Y() != hexStart.Y() &&
				hexAt.Y() != hexEnd.Y() )
				continue;

			// convert coordinates to an offset
			int i = GetMapOffset( hexAt.X(),hexAt.Y() );
			m_pwaWork[i]=0;

			if( !hexAt.X() && !hexAt.Y() )
				continue;

			// start with range to target for this location
			iDist = pGameData->GetRangeDistance( hexAt, hexTarget );
			// make sure we are getting closer
			if( iDist > iRangeToTarget )
				continue;

			// get game data info on the candidate location
			CHex *pGameHex = theMap.GetHex( hexAt );
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
			// can the vehicle get there from where it is currently
			if( !GetPathRating(hexDest, hexAt, pVehData->GetType()) )
				continue;

			// now consider we want to determine the closest candidates
			if( iDist && iDist < iBestDist )
			{
				iBestDist = iDist;
				m_pwaWork[i] = iDist;
				iCnt = 0;
			}
			// count the hexes with best range
			if( iDist == iBestDist )
				iCnt++;
		}
	}
	// if no count then there is no place to go yet
	if( !iCnt )
		return;


	// if still here then there are at least one and
	// maybe several places to go to, so rate the hexes
	// with the best defensive score  and then pick the
	// one of them that is as close to target as possible

	//
	// adjust iBestDist with game difficulty+1 and then
	// take the one with the best defensive score
	iBestDist += (pGameData->m_iSmart + 1);
	int iBestDef = 0;;
	iCnt = 0;

	// go back through the test hexes
	for( iy=0; iy<iDeltaY; iy++ )
	{
		hexAt.Y( hexAt.Wrap(hexStart.Y() + iy) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
		myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( ix=0; ix<iDeltaX; ix++ )
		{
			hexAt.X( hexAt.Wrap(hexStart.X() + ix) );

			if( hexAt.X() != hexStart.X() &&
				hexAt.X() != hexEnd.X() && 
				hexAt.Y() != hexStart.Y() &&
				hexAt.Y() != hexEnd.Y() )
				continue;

			// convert coordinates to an offset
			int i = GetMapOffset( hexAt.X(),hexAt.Y() );
			// not a candidate
			if( m_pwaWork[i] > iBestDist )
			{
				m_pwaWork[i] = 0;
				continue;
			}

			// start off
			m_pwaWork[i] = 0;

			if( !hexAt.X() && !hexAt.Y() )
				continue;

			// get game data info on the hex
			CHex *pGameHex = theMap.GetHex( hexAt );
			if( pGameHex == NULL )
				continue;

			CTerrainData const *pTerrain = 
				pGameData->GetTerrainData( (int)pGameHex->GetType() );
			if( pTerrain == NULL )
				continue;

			m_pwaWork[i] = 
				pTerrain->GetDefenseMult(pVehData->GetWheelType());
			if( m_pwaWork[i] > iBestDef )
			{
				iBestDef = m_pwaWork[i];
				iCnt = 0;
				iBest = i;
			}
			if( m_pwaWork[i] == iBestDef )
				iCnt++;
		}
	}
	// so now I got a count of those places that are the best def
	if( iBest < m_iMapSize )
	{
		// found one
		int iX,iY;
		OffsetToXY( iBest, &iX, &iY );
		hexDest.X( iX );
		hexDest.Y( iY );
	}
}

//
// find the nearest, defense hex, to the hexAttacking, from the
// hexDefending (and return in hexDefending) within the area of
// iWidth x iHeight around hexAttacking, for the vehicle type
//
void CAIMapUtil::FindDefenseHex( CHexCoord& hexAttacking, 
	CHexCoord& hexDefending, int iWidth, int iHeight, 
	CTransportData const *pVehData )
{
	CHexCoord hcStart, hcEnd, hcAt;
	// use iWidth and iHeight +/- hex to form boundaries of an area 
	// around hexAttacking to consider moving the pUnit to to get into range
	hcStart.X( hcAt.Wrap(hexAttacking.X() - iWidth) );
	hcStart.Y( hcAt.Wrap(hexAttacking.Y() - iHeight) );
	hcEnd.X( hcAt.Wrap(hexAttacking.X() + iWidth) );
	hcEnd.Y( hcAt.Wrap(hexAttacking.Y() + iHeight) );

	// BUGBUG - any changes to CHexCoord implementation will break this!

	// now go through the area and rate each hex location as 0
	// for those with other units or cannot travel into
	// and for terrain defense if enterable
	int iBest=0, iPriorBest=0, iy, ix;
	int iDeltaX = hcStart.Wrap(hcEnd.X() - hcStart.X());
	int iDeltaY = hcStart.Wrap(hcEnd.Y() - hcStart.Y());
	//int iDeltaY = abs( hcStart.Diff( hcEnd.Y() - hcStart.Y()) );
	//int iDeltaX = abs( hcStart.Diff( hcEnd.X() - hcStart.X()) );

	for( iy=0; iy<iDeltaY; iy++ )
	{
		hcAt.Y( hcAt.Wrap(hcStart.Y() + iy) );

		for( ix=0; ix<iDeltaX; ix++ )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			hcAt.X( hcAt.Wrap(hcStart.X() + ix) );

			// convert coordinates to an offset
			int i = GetMapOffset( hcAt.X(),hcAt.Y() );
			m_pwaWork[i]=0;

			if( !hcAt.X() && !hcAt.Y() )
				continue;

			// consider that artillery only wants to be on the
			// edge of the range and no closer
			if( pGameData->IsArtillery(pVehData->GetType()) )
			{
				if( hcAt.X() != hcStart.X() &&
					hcAt.X() != hcEnd.X() && 
					hcAt.Y() != hcStart.Y() &&
					hcAt.Y() != hcEnd.Y() )
					continue;
			}

			// get game data info on the hex
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

			// get terrain at the hex we are considering
			CTerrainData const *pTerrain = 
				pGameData->GetTerrainData( (int)pGameHex->GetType() );
			if( pTerrain == NULL )
				continue;
			// save off its defense value
			m_pwaWork[i] = 
				pTerrain->GetDefenseMult(pVehData->GetWheelType());
			if( m_pwaWork[i] > iBest )
			{
				iBest = m_pwaWork[i];
			}
		}
	}

	TryTryAgain:

	// start with best defense value in iBest
	iPriorBest = iBest;
	iBest = 0;
	int iRange, iBestRange = m_iMapSize, iBestHex = m_iMapSize;
	int iCnt = 0;

	// BUGBUG - any changes to CHexCoord implementation will break this!

	// now for each location with a value of iPriorBest
	for( iy=0; iy<iDeltaY; iy++ )
	{
		hcAt.Y( hcAt.Wrap(hcStart.Y() + iy) );

		for( ix=0; ix<iDeltaX; ix++ )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			hcAt.X( hcAt.Wrap(hcStart.X() + ix) );

			// convert coordinates to an offset
			int i = GetMapOffset( hcAt.X(),hcAt.Y() );
			if( !m_pwaWork[i] )
				continue;

			// hang protection
			iCnt++;

			if( m_pwaWork[i] == iPriorBest )
			{
				// can we get there?
				if( !GetPathRating(hexDefending, hcAt, pVehData->GetType()) )
				{
					m_pwaWork[i]=0;
					continue;
				}
				iRange = pGameData->GetRangeDistance( hexDefending, hcAt );
				// convert distance to sub-hexes
				//iRange *= 2; 
				iRange <<= 1; 
				if( iRange && iRange < iBestRange )
				{
					iBestRange = iRange;
					iBestHex = i;
					// this hex is done
					m_pwaWork[i]=0;
				}
			}
			else // looking for the next best defense hex
			{
				if( m_pwaWork[i] > iBest )
					iBest = m_pwaWork[i];
			}
		}
	}

	// consider that none of the current iPriorBest would do
	// so reset and try again, since iBest has been changed
	if( iBestHex == m_iMapSize )
	{
		// hang protection
		if( !iCnt )
			return;

		goto TryTryAgain;
	}
	// otherwise, we found a good place to move to
	int iX,iY;
	OffsetToXY( iBestHex, &iX, &iY );
	hexDefending.X( iX );
	hexDefending.Y( iY );
}

//
// this function takes the passed hex, then using the
// largest possible building/vehicle size, determines
// the adjacent group hexes in all directions, then
// gets current game data for each hex of each group
// and updates the m_pMap with status
//
void CAIMapUtil::UpdateAdjacentHexes( CAIHex *paiHex )
{
	// size of group of hexes that make up largest
	// possible multi-hex CUnit
	int iWidth = pGameData->m_iStructureSize;

	// for each adjacent direction, from the passed
	// hex and the passed hex itself, get the latest
	// game data for that hex group and do an update of
	// m_pMap for each hex of the group
	//int isy,iey,isx,iex;
	CHexCoord hcStart;

	for( int i=0; i<=MAX_ADJACENT; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		ClearGroupHexes();
		// first load group hex arrays based on direction
		// from the adjacent hex paiHex
		switch( i )
		{
		// NORTH
		case 0:
			hcStart.X( paiHex->m_iX );
			hcStart.Y( hcStart.Wrap(paiHex->m_iY - iWidth) );
			break;
		// NORTHEAST
		case 1:
			hcStart.X( hcStart.Wrap(paiHex->m_iX + iWidth) );
			hcStart.Y( hcStart.Wrap(paiHex->m_iY - iWidth) );
			break;
		// EAST
		case 2:
			hcStart.X( hcStart.Wrap(paiHex->m_iX + iWidth) );
			hcStart.Y( paiHex->m_iY );
			break;
		// SOUTHEAST
		case 3:
			hcStart.X( hcStart.Wrap(paiHex->m_iX + iWidth) );
			hcStart.Y( hcStart.Wrap(paiHex->m_iY + iWidth) );
			break;
		// SOUTH
		case 4:
			hcStart.X( paiHex->m_iX );
			hcStart.Y( hcStart.Wrap(paiHex->m_iY + iWidth) );
			break;
		// SOUTHWEST
		case 5:
			hcStart.X( hcStart.Wrap(paiHex->m_iX - iWidth) );
			hcStart.Y( hcStart.Wrap(paiHex->m_iY + iWidth) );
			break;
		// WEST
		case 6:
			hcStart.X( hcStart.Wrap(paiHex->m_iX - iWidth) );
			hcStart.Y( paiHex->m_iY );
			break;
		// NORTHWEST
		case 7:
			hcStart.X( hcStart.Wrap(paiHex->m_iX - iWidth) );
			hcStart.Y( hcStart.Wrap(paiHex->m_iY - iWidth) );
			break;
		case MAX_ADJACENT:
			hcStart.X( paiHex->m_iX );
			hcStart.Y( paiHex->m_iY );
			break;
		default:
			continue;
		}

		LoadGroupHexes( hcStart.X(), hcStart.Y(), iWidth, iWidth );

		// now for each hex in the group
		CAIHex *paiHex = NULL;
		for( int j=0; j<m_iGrpSize; ++j )
		{
#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
			paiHex = &m_pgrpHexs[j];

			// inactive hex in group
			if( !paiHex->m_iUnit )
				continue;

			// skip hexes not on this map
			//if( !IsThisMap( 
			//	(WORD)paiHex->m_iX, (WORD)paiHex->m_iY ) )
			//	continue;

			int k = GetMapOffset( paiHex->m_iX, paiHex->m_iY );
			if( k >= m_iMapSize )
				continue;
			// get game data for hex
			pGameData->GetCHexData(paiHex);
			// determine status word for that location
			//WORD wStatus = m_pMap->GetAt(k);
			WORD wStatus = m_pMap[k];
			// examine location data and update status
			wStatus = ConvertStatus( paiHex, wStatus );
			// update map array with status
			m_pMap[k] = wStatus;
		}
	}
}

//
// looking for CHex::ocean adjacent in all directions from hex
// except when bReally is set TRUE, then ocean adjacent fails
//
BOOL CAIMapUtil::IsWaterAdjacent( CHexCoord hex, BOOL bReally )
{
	// for each direction
	int iYield=0;
	for( int i=0; i<MAX_ADJACENT; ++i )
	{
		CHexCoord hcRay = hex;

		if (iYield > 50)
			{
			myYieldThread();
			iYield = 0;
			}

		// move ray based on direction i
		switch(i)
		{
			case 0:	// north
				hcRay.Ydec();
				break;
			case 1:	// northeast
				hcRay.Ydec();
				hcRay.Xinc();
				break;
			case 2:
				hcRay.Xinc();
				break;
			case 3:
				hcRay.Yinc();
				hcRay.Xinc();
				break;
			case 4:
				hcRay.Yinc();
				break;
			case 5:
				hcRay.Yinc();
				hcRay.Xdec();
				break;
			case 6:
				hcRay.Xdec();
				break;
			case 7:
				hcRay.Xdec();
				hcRay.Ydec();
				break;
			default:
				return FALSE;
		}
			
		// get terrain from game data
		CHex *pGameHex = theMap.GetHex( hcRay );
		// and if it is not ocean, return
		if( bReally )
		{
			if( m_bOceanWorld )
			{
				if( pGameHex->GetType() != CHex::ocean )
					return FALSE;
			}
			else
			{
				if( pGameHex->GetType() != CHex::ocean &&
					pGameHex->GetType() != CHex::lake )
					return FALSE;
			}
		}
		else // really want all land instead
		{
			if( pGameHex->GetType() == CHex::ocean ||
				pGameHex->GetType() == CHex::lake )
				return FALSE;
		}
	}
	return TRUE; // hex is completely surrounded by CHex::ocean
}

//
// consider the passed hex for landing craft to un/load at, which
// means that there needs to be land or coastal next to it
//
BOOL CAIMapUtil::IsLandingArea( CHexCoord& hexLC )
{
	WORD wCntWater = 0;
	WORD wCntLand = 0;
	CHexCoord hex, hexStart;

	// adjust start to upper left corner of area (7 position)
	hexStart = hexLC;
	hexStart.Xdec();
	hexStart.Ydec();

	// now go thru hexes adjacent and around
	for( int iY=0; iY<3; ++iY )
	{
		hex.Y( hex.Wrap(hexStart.Y() + iY) );

		for( int iX=0; iX<3; ++iX )
		{
			hex.X( hex.Wrap(hexStart.X() + iX) );

			// skip the hex that is in the middle
			if( hex == hexLC )
				continue;

			CHex *pGameHex = theMap.GetHex( hex );
			if( pGameHex == NULL )
				continue;

			// if an Ocean exists, count only ocean
			if( m_bOceanWorld )
			{
				if( pGameHex->GetType() == CHex::ocean )
					wCntWater++;
				else if( pGameHex->GetType() == CHex::coastline )
					wCntLand++;
			}
			else // no ocean, but it may be a lake world
			{
				if( pGameHex->GetType() == CHex::ocean ||
					pGameHex->GetType() == CHex::lake )
					wCntWater++;
				else if( pGameHex->GetType() == CHex::coastline )
					wCntLand++;
			}
		}
	}

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"CAIMapUtil::IsLandingArea() at %d,%d water=%d land=%d ",
		hexLC.X(), hexLC.Y(), wCntWater, wCntLand );
#endif

	if( wCntWater >= 1 &&
		wCntLand >= 1 )
		return TRUE;

	return FALSE;
}

//
// this will test the width sized groups of hexes
// for the presence
// of water, and return TRUE if some is found
//
BOOL CAIMapUtil::IsWaterAdjacent( CHexCoord hexWater,
	int iWidthX, int iWidthY )
{
	ClearGroupHexes();

	LoadGroupHexes( hexWater.X(), hexWater.Y(), iWidthX, iWidthY );

	// now the group hex arrays, contain
	// the x,y values of the hexes that
	// make up the width sized group in
	// the indicated direction
	CAIHex *paiGrpHex = NULL;
	WORD wCntWater = 0;
	WORD wCntLand = 0;
	CHexCoord hex;

	//TRACE( "\n " );

	for( int i=0; i<m_iGrpSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		paiGrpHex = &m_pgrpHexs[i];

		// end of active hexes in group
		if( !paiGrpHex->m_iUnit )
			break;

		// BUGBUG need to store terrain data in AI to speed up acess
		//pGameData->GetCHexData(paiGrpHex);
		//CTerrainData const *pTerrain = 
		//	pGameData->GetTerrainData( (int)paiGrpHex->m_cTerrain );

		// get terrain from game data
		hex.X( paiGrpHex->m_iX );
		hex.Y( paiGrpHex->m_iY );
		CHex *pGameHex = theMap.GetHex( hex );
		if( pGameHex == NULL )
			continue;

		// if an Ocean exists, count only ocean
		if( m_bOceanWorld )
		{
			if( pGameHex->GetType() == CHex::ocean )
				wCntWater++;
			else if( IsHexValidToBuild( hex ) )
				wCntLand++;
		}
		else // no ocean, but it may be a lake world
		{
			if( pGameHex->GetType() == CHex::ocean ||
				pGameHex->GetType() == CHex::lake )
				wCntWater++;
			else if( IsHexValidToBuild( hex ) )
				wCntLand++;
		}
	}
	if( wCntWater >= 1 &&
		wCntLand >= 1 )
		return TRUE;

	return FALSE;
}

//
// check the terrain, for the size passed, to make sure it is all poor
//
BOOL CAIMapUtil::IsCitizenTerrain( CHexCoord& hcStart, int iWidthX, int iWidthY ) 
{
	// look for a good start
	CHex *pHex = theMap.GetHex( hcStart );
	if( pHex->GetType() != CHex::rough &&
		pHex->GetType() != CHex::hill &&
		pHex->GetType() != CHex::swamp &&
		pHex->GetType() != CHex::desert )
		return( FALSE );
	
	CHexCoord hcEnd,hex;
	hcEnd.X( hex.Wrap(hcStart.X() + iWidthX) );
	hcEnd.Y( hex.Wrap(hcStart.Y() + iWidthY) );
	int iDeltaX = hex.Wrap(hcEnd.X() - hcStart.X());
	int iDeltaY = hex.Wrap(hcEnd.Y() - hcStart.Y());

	WORD wTest = 
		MSW_AI_BUILDING | MSW_OPFOR_BUILDING | MSW_RESOURCE | MSW_STAGING;

	int iy,ix,i;
	for( iy=0; iy<iDeltaY; iy++ )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iy) );

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		for( ix=0; ix<iDeltaX; ix++ )
		{
			hex.X( hex.Wrap(hcStart.X() + ix) );

			i = GetMapOffset( hex.X(), hex.Y() );
			if( i >= m_iMapSize )
				return FALSE;

			// avoid buildings and resources
			if( m_pMap[i] & wTest )
				return FALSE;

			// avoid forests
			pHex = theMap.GetHex( hex );
			if( pHex->GetType() == CHex::forest )
				return FALSE;
		}
	}
	return( TRUE );						
}

int CAIMapUtil::GetMineralRating( CHexCoord& hexRate, 
	int iWidthX, int iWidthY, int iMaterial )
{
	CHexCoord hcStart,hcEnd,hex;
	hcStart.X( hexRate.X() );
	hcStart.Y( hexRate.Y() );
	hcEnd.X( hex.Wrap(hexRate.X() + iWidthX) );
	hcEnd.Y( hex.Wrap(hexRate.Y() + iWidthY) );
	int iDeltaX = hex.Wrap(hcEnd.X() - hcStart.X());
	int iDeltaY = hex.Wrap(hcEnd.Y() - hcStart.Y());

#if THREADS_ENABLED
	myYieldThread();
#endif

	CMinerals *pmn;
	int iRating = 0;
	int iy,ix;
	for( iy=0; iy<iDeltaY; iy++ )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iy) );

		for( ix=0; ix<iDeltaX; ix++ )
		{
			hex.X( hex.Wrap(hcStart.X() + ix) );

			// check hex for material
			if( theMinerals.Lookup(hex, pmn) )
			{
				// of the type we are interested in
				if( pmn->GetType() == iMaterial )
				{
					iRating += pmn->GetQuantity();
				}
			}
		}
	}
	return( iRating );
}

//
// this considers the size of the building passed, assuming the paiHex
// will be the upper left corner and checks for buildings that are
// nearby, returning TRUE if at least m_iBufferSize distance around
// the location+size is clear of other buildings (based on the AI's map)
// and FALSE if a building is found in the area
//
BOOL CAIMapUtil::NearBuildings( CHexCoord& hexRate, 
	int iWidthX, int iWidthY, BOOL bAllowResc )
{
	CHexCoord hcStart,hcEnd,hex;

	int iBufferSize = m_iBufferSize;
	// reduce buffer for small world
	if( pGameData->m_iHexPerBlk <= 32 )
		iBufferSize--;

	hcStart.X( hex.Wrap(hexRate.X() - iBufferSize) );
	hcStart.Y( hex.Wrap(hexRate.Y() - iBufferSize) );
	hcEnd.X( hex.Wrap(hexRate.X() + (iBufferSize + iWidthX)) );
	hcEnd.Y( hex.Wrap(hexRate.Y() + (iBufferSize + iWidthY)) );
	int iDeltaX = hex.Wrap(hcEnd.X() - hcStart.X());
	int iDeltaY = hex.Wrap(hcEnd.Y() - hcStart.Y());

	CHex *pGameHex;
	BYTE bUnits;
	WORD wTest = MSW_AI_BUILDING | MSW_OPFOR_BUILDING | MSW_RESOURCE;
	// by not testing for resources, a mine can be built here
	if( bAllowResc )
		wTest = MSW_AI_BUILDING | MSW_OPFOR_BUILDING;

#if THREADS_ENABLED
			// BUGBUG this function must yield
			myYieldThread();
			//if( myYieldThread() == TM_QUIT )
			//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	int iy,ix,i;
	for( iy=0; iy<iDeltaY; iy++ )
	{
		hex.Y( hex.Wrap(hcStart.Y() + iy) );

		for( ix=0; ix<iDeltaX; ix++ )
		{
			hex.X( hex.Wrap(hcStart.X() + ix) );

			i = GetMapOffset( hex.X(), hex.Y() );
			if( i >= m_iMapSize )
				return TRUE;
			if( m_pMap[i] & wTest )
				return TRUE;

			// what does the game say?
			pGameHex = theMap.GetHex( hex );
			if( pGameHex == NULL )
				return TRUE;
				
			bUnits = pGameHex->GetUnits();
			// buildings is here
			if( (bUnits & CHex::bldg) )
				return TRUE;
		}
	}

	return FALSE;
}

//
// consider the width size group of hexes in the 
// iDirection indicated from paiHex for being a
// valid group of hexes for building in
//
BOOL CAIMapUtil::AreHexesOpen( int iDirection, CHexCoord& hexRate, 
	int iWidthX, int iWidthY )
{
	ClearGroupHexes();

	CHexCoord hcStart;
	switch( iDirection )
	{
		// NORTH
		case 0:
			hcStart.X( hexRate.X() );
			hcStart.Y( hcStart.Wrap(hexRate.Y() - iWidthY) );
			break;
		// NORTHEAST
		case 1:
			hcStart.X( hcStart.Wrap(hexRate.X() + iWidthX) );
			hcStart.Y( hcStart.Wrap(hexRate.Y() - iWidthY) );
			break;
		// EAST
		case 2:
			hcStart.X( hcStart.Wrap(hexRate.X() + iWidthX) );
			hcStart.Y( hexRate.Y() );
			break;
		// SOUTHEAST
		case 3:
			hcStart.X( hcStart.Wrap(hexRate.X() + iWidthX) );
			hcStart.Y( hcStart.Wrap(hexRate.Y() + iWidthY) );
			break;
		// SOUTH
		case 4:
			hcStart.X( hexRate.X() );
			hcStart.Y( hcStart.Wrap(hexRate.Y() + iWidthY) );
			break;
		// SOUTHWEST
		case 5:
			hcStart.X( hcStart.Wrap(hexRate.X() - iWidthX) );
			hcStart.Y( hcStart.Wrap(hexRate.Y() + iWidthY) );
			break;
		// WEST
		case 6:
			hcStart.X( hcStart.Wrap(hexRate.X() - iWidthX) );
			hcStart.Y( hexRate.Y() );
			break;
		// NORTHWEST
		case 7:
			hcStart.X( hcStart.Wrap(hexRate.X() - iWidthX) );
			hcStart.Y( hcStart.Wrap(hexRate.Y() - iWidthY) );
			break;
		case MAX_ADJACENT:
			hcStart.X( hexRate.X() );
			hcStart.Y( hexRate.Y() );
			break;
		default:
			return FALSE;
	}

	LoadGroupHexes( hcStart.X(), hcStart.Y(), iWidthX, iWidthY );

	
	// now the group hex arrays, contain
	// the x,y values of the hexes that
	// make up the width sized group in
	// the indicated direction
	CAIHex *paiGrpHex = NULL;
	
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif

	for( int i=0; i<m_iGrpSize; ++i )
	{
		paiGrpHex = &m_pgrpHexs[i];

		// end of active hexes in group
		if( !paiGrpHex->m_iUnit )
			break;

		CHexCoord hex( paiGrpHex->m_iX, paiGrpHex->m_iY );
		// test for minerals being present
		if( !m_bMinerals )
		{
			// if a mineral is here, don't build
			CMinerals *pmn;
			if( theMinerals.Lookup(hex, pmn) )
				return FALSE;
		}
		// and if any are not, then can't build
		// a width sized building here
		if( !IsHexValidToBuild( hex ) )
			return FALSE;
	}

	// if we are still here, that means we can build here
	// so flag the m_pwaWork at the first hex of the group
	i = GetMapOffset( hexRate.X(), hexRate.Y() );
	if( i < m_iMapSize )
	{
		m_pwaWork[i] = 1;
		return TRUE;
	}
	return FALSE;
}


//
// this function calculates the build multiplier based
// on the terrain found in all hexes needed to build on
// for the building of the size passed
//
// paihex is the hex-of-the-building considered
//
int CAIMapUtil::GetMultiplier( CHexCoord hexBuild,
	int iWidthX, int iWidthY, int iType )
{
	ClearGroupHexes();
	LoadGroupHexes( hexBuild.X(), hexBuild.Y(), iWidthX, iWidthY );

	// now the group hex arrays, contain
	// the x,y values of the hexes that
	// make up the width sized group
	int iMulti = 0;
	CAIHex *paiGrpHex = NULL;
	CHexCoord hex;
	for( int i=0; i<m_iGrpSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		paiGrpHex = &m_pgrpHexs[i];

		// end of active hexes in group
		if( !paiGrpHex->m_iUnit )
			break;

		// get location from game data, and build multiplier
		//pGameData->GetCHexData(paiGrpHex);
		hex.X( paiGrpHex->m_iX );
		hex.Y( paiGrpHex->m_iY );
		CHex *pGameHex = theMap.GetHex( hex );
		if( pGameHex == NULL )
			continue;

		CTerrainData const *pTerrain = 
			pGameData->GetTerrainData( pGameHex->GetType() );

		// accumulate multiplier based on type indicated
		// get building multiplier
		if( iType == CStructureData::city )
			iMulti += pTerrain->GetBuildMult();
		// get farm multiplier
		else if( iType == CStructureData::farm )
			iMulti += pTerrain->GetFarmMult();
		else
			iMulti += pTerrain->GetBuildMult();

	}
	if( !iMulti )
		iMulti = 1;
	return( iMulti );
}

//
// consider whether the passed hex can be patroled by 
// either wheel, track or hover type wheeltype vehicles
//
BOOL CAIMapUtil::IsHexPatrolable( CHexCoord& hex )
{
	CHex *pGameHex = theMap.GetHex( hex );
	if( pGameHex == NULL )
		return FALSE;

	BYTE bUnits = pGameHex->GetUnits();
	// skip buildings
	if( (bUnits & CHex::bldg) )
		return FALSE;
	// and vehicles
	if( (bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr)) )
		return FALSE;

	if( !m_tdWheel->CanTravelHex(pGameHex) )
		return FALSE;
	if( !m_tdTrack->CanTravelHex(pGameHex) )
		return FALSE;
	if( !m_tdHover->CanTravelHex(pGameHex) )
		return FALSE;
	if( !m_tdFoot->CanTravelHex(pGameHex) )
		return FALSE;
	
	return TRUE;
}

//
// BUGBUG this routine decides whether anything can build here
//
BOOL CAIMapUtil::IsHexValidToBuild( CHexCoord& hex )
{
	// stay away from hex at 0,0
	if( !hex.X() && !hex.Y() )
		return FALSE;

	// get location from game data
	CHex *pGameHex = theMap.GetHex( hex );
	if( pGameHex == NULL )
		return FALSE;

	BYTE bUnits = pGameHex->GetUnits();

	// skip buildings
	if( (bUnits & CHex::bldg) )
		return FALSE;
	// and vehicles 
	//if( (bUnits & (CHex::ul | CHex::ur | CHex::ll | CHex::lr)) )
	//	return FALSE;

	// consider the type of terrain from the map
	int iTerrain = pGameHex->GetType();
	if( iTerrain == CHex::ocean ||
		iTerrain == CHex::river ||
		iTerrain == CHex::road ||
		iTerrain == CHex::lake )
		return FALSE;

	// get the offset of the hex to the real map
	int i = GetMapOffset( hex.X(), hex.Y() );
	if( i >= m_iMapSize )
		return FALSE;

	// consider what may be in the hex on the map
	WORD wTest = 0;
	wTest = MSW_AI_BUILDING | MSW_OPFOR_BUILDING |
			MSW_AI_VEHICLE | MSW_OPFOR_VEHICLE | MSW_STAGING;

	// get AI's version of the map
	if( m_pMap[i] & wTest )
		return FALSE;

	return TRUE;
}


//
// determine if the map is all known 
//
BOOL CAIMapUtil::AllKnown( void )
{
	if( m_bAllKnown )
		return TRUE;

	BOOL bKnown = TRUE;

	for( int i=0; i<m_iMapSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		// fail on the first non-known location
		if( !(m_pwaWork[i] & MSW_KNOWN) )
		{
			bKnown = FALSE;
			break;
		}
	}
	m_bAllKnown = bKnown;
	return( bKnown );
}

void CAIMapUtil::OffsetToXY( int iOffset, int *piX, int *piY )
{
	int iX, iY;
	// convert map array offset to game map X,Y
	if( iOffset >= (int)m_wCols )
	{
		// calculate relative to this map
		iY = iOffset / (int)m_wCols;
		iX = (iOffset - (iY * (int)m_wCols));
	}
	else
	{
		iY = 0;
		iX = iOffset;
	}
	// convert to game map
	iY += (int)m_wBaseRow;
	iX += (int)m_wBaseCol;

	*piY = iY;
	*piX = iX;
}

//
// convert the passed hex location into an offset
// to access the map status words array
//
int CAIMapUtil::GetMapOffset( int iX, int iY )
{
	// BUGBUG this calculation needs to be proved
	int i = (iY - (int)m_wBaseRow) * (int)m_wCols;
	i += (iX - (int)m_wBaseCol);
	return( i );
}

void CAIMapUtil::ClearWorkMap( void )
{
	for( int i=0; i<m_iMapSize; ++i )
	{
#if THREADS_ENABLED
		// BUGBUG this function must yield
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
		m_pwaWork[i] = 0;
	}
}

#if 0
//
// loads the m_pgrpHexs[] with the x,y values of the hexes
// that are adjacent and around the hex passed
//
void CAIMapUtil::LoadAroundHexes( int isx, int isy )
{
	// start hex of group, use for wrap and inc
	CHexCoord hexLoad( isx, isy );
	CAIHex *paiLoad = NULL;
	// counter for group hexes
	int i=0; 
	CHexCoord hexStart, hex;
	// adjust start to upper left corner of area (7 position)
	hexStart.X( hexLoad.Xdec() );
	hexStart.Y( hexLoad.Ydec() );

	// now load group hexes based on start
	for( int iY=0; iY<3; ++iY )
	{
		hex.Y( hex.Wrap(hexStart.Y() + iY) );

		for( int iX=0; iX<3; ++iX )
		{
			hex.X( hex.Wrap(hexStart.X() + iX) );

			// skip the hex that is in the middle
			if( hex == hexLoad )
				continue;

			if( i < m_iGrpSize )
			{
				paiLoad = &m_pgrpHexs[i++];
				paiLoad->m_iX = hex.X();
				paiLoad->m_iY = hex.Y();
				paiLoad->m_iUnit = 1; // indicates its active
			}
		}
	}
}
#endif

//
// starting at the start x,y and running thru
// the end x,y load the group hex arrays with
// the values of x,y that make up the group
//
void CAIMapUtil::LoadGroupHexes( int isx, int isy, int iWidth, int iHeight )
{
	// start hex of group, use for wrap and inc
	CHexCoord hexLoad( isx, isy );
	CAIHex *paiLoad = NULL;
	// counter for group hexes
	int i=0; 
	// now load group hexes based on start/end
	for( int iY=0; iY<iHeight; ++iY )
	{
		for( int iX=0; iX<iWidth; ++iX )
		{
			if( i < m_iGrpSize )
			{
				paiLoad = &m_pgrpHexs[i++];
				paiLoad->m_iX = hexLoad.X();
				paiLoad->m_iY = hexLoad.Y();
				paiLoad->m_iUnit = 1; // indicates its active

				// make sure that group does not spill over
				// to another map
				//if( paiLoad->m_iY > m_wEndRow ||
				//	paiLoad->m_iX > m_wEndCol )
				//	paiLoad->m_iUnit = 0;
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
//
// clear the arrays that are used to contain
// the hexes that make up a group
//
void CAIMapUtil::ClearGroupHexes( void )
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


// BUGBUG this function is not done yet!
//
// evaluate the data in the CHex, considering the CBuilding
// or CVehicle that may be there, and the CTerrainData
// that is found in the location, and construct a bitmap
// WORD that reflects the status
//
// Status WORD bitmap definition
//#define MSW_AI_BUILDING		0x0001	// bit 0
//#define MSW_AI_VEHICLE		0x0002	// bit 1
//#define MSW_OPFOR_BUILDING	0x0004	// bit 2
//#define MSW_OPFOR_VEHICLE	0x0008	// bit 3
//#define MSW_KNOWN		    0x0010	// bit 4
//#define MSW_ROAD			0x0020	// bit 5
//#define MSW_PLANNED_ROAD	0x0040	// bit 6
//#define MSW_RESOURCE		0x0080	// bit 7
//#define MSW_STAGING		0x0100  // bit 8
//
// bit 8 - bit 15 == id of building/vehicle
//
// all bits clear (word == 0) means location has nothing on it
//
WORD CAIMapUtil::ConvertStatus( CAIHex *pHex, WORD wOldStatus )
{
	// BUGBUG this is not right, if the status comes in showing
	// a vehicle, and it is not there anymore, then this would
	// cause it to still be there after converting
	WORD wStatus = wOldStatus;

	// so for the time being, clear it each time
	wStatus = 0;

	// if a road exists already then keep it
	if( wOldStatus & MSW_ROAD )
		wStatus |= MSW_ROAD;
	else
	{
		if( (int)pHex->m_cTerrain == CHex::road )
			wStatus |= MSW_ROAD;
	}
	// this is a staging area
	if( wOldStatus & MSW_STAGING )
	{
		wStatus |= MSW_STAGING;
	}
	

	// a planned road may NOT have been converted to a road
	if( wOldStatus & MSW_PLANNED_ROAD &&
		!(wStatus & MSW_ROAD) )
		wStatus |= MSW_PLANNED_ROAD;

	// or previously impassable terrain
	if( wOldStatus & MSW_KNOWN )
		wStatus |= MSW_KNOWN;

	// consider materials in this hex
	CHexCoord hcHex( pHex->m_iX, pHex->m_iY );
	CMinerals *pmn;
	if( theMinerals.Lookup( hcHex, pmn) )
		wStatus |= MSW_RESOURCE;

	//CAIUnit::CAIUnit( DWORD dwID, int iOwner, int iType, int iTypeUnit )
	//CAIUnit aiUnit(0,0,0,0);
	int iType = m_iNumBuildings;
	int iOwner = 0;
	if( pHex->m_dwUnitID > 0 &&
		pHex->m_iUnit == CUnit::building )
	{
		// get type of building and owner
		//iType = pGameData->GetBuilding( pHex->m_dwUnitID, &aiUnit );
		CBuilding *pBldg = theBuildingMap.GetBldg( pHex->m_dwUnitID );
		if( pBldg != NULL )
		{
			iType = pBldg->GetData()->GetBldgType();
			// determine if the unit is AI owned
			if( pBldg->GetOwner()->GetPlyrNum() == m_iPlayer )
				wStatus |= MSW_AI_BUILDING;
			else
				wStatus |= MSW_OPFOR_BUILDING;

			WORD wType = 0xFFFF;
			// store type in high byte of wStatus
			if( iType < m_iNumBuildings )
				wType = (WORD)iType;
			if( wType != 0xFFFF )
			{
			WORD wTemp = wStatus;
			wStatus = wType << 8;
			wStatus |= wTemp;
			}
		}
	}
	return( wStatus );
}


CAIMapUtil::CAIMapUtil( WORD *pMap, 
	CAIUnitList *plUnits, int iBaseX, int iBaseY,
	WORD wBaseCol, WORD wBaseRow, WORD wCols, WORD wRows, int iPlayer )
{
	m_bUnknown = TRUE;

	m_pMap = pMap;
	m_plUnits = plUnits;
		
	m_iMapSize = wRows * wCols;

	m_iBaseX = iBaseX;
	m_iBaseY = iBaseY;
	m_wBaseRow = wBaseRow;
	m_wBaseCol = wBaseCol;
	m_wRows = wRows;
	m_wCols = wCols;
	m_wEndRow = (m_wBaseRow + wRows) - 1;
	m_wEndCol = (m_wBaseCol + wCols) - 1;
	m_iPlayer = iPlayer;
	m_iBufferSize = CBuildFarm::GetCowDistance();

	m_iCitySX = (int)m_wEndCol;
	m_iCitySY = (int)m_wEndRow;
	m_iCityEX = (int)m_wBaseCol;
	m_iCityEY = (int)m_wBaseRow;

	m_iNumBuildings = CStructureData::num_types;
	m_iNumVehicles = CTransportData::num_types;

	m_bMinerals = TRUE;
	m_bAllKnown = FALSE;
	m_bOceanWorld = FALSE;
	m_bLakeWorld = FALSE;

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"\nCAIMapUtil created, player %d m_iBaseXY=%d,%d  m_wBaseCol=%d m_wBaseRow=%d m_wCols=%d m_wRows=%d",
m_iPlayer, m_iBaseX, m_iBaseY, m_wBaseCol, m_wBaseRow, m_wCols, m_wRows );
#endif

	try
	{
		m_pwaWork = new WORD[m_iMapSize];
		memset( m_pwaWork, 0, (size_t)(m_iMapSize * sizeof( WORD )) );
	}
	catch( CException e )
	{
		if( m_pwaWork != NULL )
		{
			delete [] m_pwaWork;
			m_pwaWork = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}

	// calculate the number of hexes in a group
	m_iGrpSize = pGameData->m_iStructureSize *
		pGameData->m_iStructureSize;
	try
	{
		// and create a work area that size
		m_pgrpHexs = new CAIHex[m_iGrpSize];
	}
	catch( CException e )
	{
		m_pgrpHexs = NULL;
		m_iGrpSize = 0;

		throw(ERR_CAI_BAD_NEW);
	}

	m_tdWheel = theTransports.GetData( CTransportData::construction );
	m_tdTrack = theTransports.GetData( CTransportData::infantry_carrier );
	m_tdHover = theTransports.GetData( CTransportData::light_scout );
	m_tdFoot  = theTransports.GetData( CTransportData::infantry );
	m_tdShip  = theTransports.GetData( CTransportData::landing_craft );
	
}

CAIMapUtil::~CAIMapUtil()
{
#if 0 //0 //DEBUG_OUTPUT_MAPUTL
	ReportPavedRoads();
#endif

	if( m_pwaWork != NULL )
	{
		delete [] m_pwaWork;
		m_pwaWork = NULL;
	}
	delete [] m_pgrpHexs;
}

void CAIMapUtil::Save( CFile *pFile )
{
	int iX, iY;
	iX = m_RocketHex.X();
	iY = m_RocketHex.Y();

		pFile->Write( (const void*)&m_iPlayer, sizeof(int) );
		pFile->Write( (const void*)&m_wRows, sizeof(WORD) );
		pFile->Write( (const void*)&m_wCols, sizeof(WORD) );
		pFile->Write( (const void*)&m_wBaseRow, sizeof(WORD) );
		pFile->Write( (const void*)&m_wBaseCol, sizeof(WORD) );
		pFile->Write( (const void*)&m_wEndRow, sizeof(WORD) );
		pFile->Write( (const void*)&m_wEndCol, sizeof(WORD) );
		pFile->Write( (const void*)&m_iBaseX, sizeof(int) );
		pFile->Write( (const void*)&m_iBaseY, sizeof(int) );
		pFile->Write( (const void*)&iX, sizeof(int) );
		pFile->Write( (const void*)&iY, sizeof(int) );
		pFile->Write( (const void*)&m_iMapSize, sizeof(int) );
		pFile->Write( (const void*)&m_iGrpSize, sizeof(int) );
		pFile->Write( (const void*)&m_iNumVehicles, sizeof(int) );
		pFile->Write( (const void*)&m_iNumBuildings, sizeof(int) );
		pFile->Write( (const void*)&m_iBufferSize, sizeof(int) );
		pFile->Write( (const void*)&m_iCitySX, sizeof(int) );
		pFile->Write( (const void*)&m_iCitySY, sizeof(int) );
		pFile->Write( (const void*)&m_iCityEX, sizeof(int) );
		pFile->Write( (const void*)&m_iCityEY, sizeof(int) );
		pFile->Write( (const void*)&m_bAllKnown, sizeof(BOOL) );
		pFile->Write( (const void*)&m_bOceanWorld, sizeof(BOOL) );
		pFile->Write( (const void*)&m_bLakeWorld, sizeof(BOOL) );
}

void CAIMapUtil::Load( CFile *pFile, WORD *pMap, CAIUnitList *plUnits )
{
	int iX, iY;

		pFile->Read( (void*)&m_iPlayer, sizeof(int) );
		pFile->Read( (void*)&m_wRows, sizeof(WORD) );
		pFile->Read( (void*)&m_wCols, sizeof(WORD) );
		pFile->Read( (void*)&m_wBaseRow, sizeof(WORD) );
		pFile->Read( (void*)&m_wBaseCol, sizeof(WORD) );
		pFile->Read( (void*)&m_wEndRow, sizeof(WORD) );
		pFile->Read( (void*)&m_wEndCol, sizeof(WORD) );
		pFile->Read( (void*)&m_iBaseX, sizeof(int) );
		pFile->Read( (void*)&m_iBaseY, sizeof(int) );
		pFile->Read( (void*)&iX, sizeof(int) );
		pFile->Read( (void*)&iY, sizeof(int) );
		pFile->Read( (void*)&m_iMapSize, sizeof(int) );
		pFile->Read( (void*)&m_iGrpSize, sizeof(int) );
		pFile->Read( (void*)&m_iNumVehicles, sizeof(int) );
		pFile->Read( (void*)&m_iNumBuildings, sizeof(int) );
		pFile->Read( (void*)&m_iBufferSize, sizeof(int) );
		pFile->Read( (void*)&m_iCitySX, sizeof(int) );
		pFile->Read( (void*)&m_iCitySY, sizeof(int) );
		pFile->Read( (void*)&m_iCityEX, sizeof(int) );
		pFile->Read( (void*)&m_iCityEY, sizeof(int) );
		pFile->Read( (void*)&m_bAllKnown, sizeof(BOOL) );
		pFile->Read( (void*)&m_bOceanWorld, sizeof(BOOL) );
		pFile->Read( (void*)&m_bLakeWorld, sizeof(BOOL) );

	m_RocketHex.X( iX );
	m_RocketHex.Y( iY );
	m_pMap = pMap;
	m_plUnits = plUnits;

	// map size and group size might have changed
	if( m_pwaWork != NULL )
	{
		delete [] m_pwaWork;
		m_pwaWork = NULL;
	}
	if( m_pgrpHexs != NULL )
	{
		delete [] m_pgrpHexs;
		m_pgrpHexs = NULL;
	}
	
	try
	{
		m_pwaWork = new WORD[m_iMapSize];
		memset( m_pwaWork, 0, (size_t)(m_iMapSize * sizeof( WORD )) );
	}
    catch( CException theException )
    {
		if( m_pwaWork != NULL )
		{
			delete [] m_pwaWork;
			m_pwaWork = NULL;
		}
		// how should memory errors be reported?
    	throw(ERR_CAI_BAD_NEW);
    }

	try
	{
		// and create a work area that size
		m_pgrpHexs = new CAIHex[m_iGrpSize];
	}
    catch( CException theException )
    {
		m_pgrpHexs = NULL;
		m_iGrpSize = 0;
		// how should memory errors be reported?
    	throw(ERR_CAI_BAD_NEW);
	}
}

#ifdef _LOGOUT

void CAIMapUtil::ReportPavedRoads( void )
{
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"Status of map and roads, player %d from %d,%d to %d,%d ",
	 	m_iPlayer, m_wBaseCol, m_wBaseRow, m_wEndCol, m_wEndRow );


	// BUGBUG this is a constant sized array which works
	// only for small maps, and if a larger map is used
	// it will display the first 64 columns only
	char szLine[65];
	for( int i=0; i<(65); ++i )
		szLine[i] = '\0';

	CAIHex aiHex( 0, 0 );
	for( int iY=0; iY<64; ++iY )
	{
		int y = iY + m_wBaseRow;

		for( int iX=0; iX<64; ++iX )
		{
			int x = iX + m_wBaseCol;

			// get location from game data
			aiHex.m_iX = x;
			aiHex.m_iY = y;
			pGameData->GetCHexData(&aiHex);

			// display terrain
			if( (int)aiHex.m_cTerrain == CHex::ocean )
				szLine[iX] = 'o';
			else if( (int)aiHex.m_cTerrain == CHex::lake )
				szLine[iX] = 'l';
			else if( (int)aiHex.m_cTerrain == CHex::plain )
				szLine[iX] = 'p';
			else if( (int)aiHex.m_cTerrain == CHex::forest )
				szLine[iX] = 'f';
			else if( (int)aiHex.m_cTerrain == CHex::hill )
				szLine[iX] = 'h';
			else if( (int)aiHex.m_cTerrain == CHex::rough )
				szLine[iX] = 'r';
			else if( (int)aiHex.m_cTerrain == CHex::swamp )
				szLine[iX] = 's';
			else if( (int)aiHex.m_cTerrain == CHex::mountain )
				szLine[iX] = 'm';
			else if( (int)aiHex.m_cTerrain == CHex::desert )
				szLine[iX] = 'd';
			else if( (int)aiHex.m_cTerrain == CHex::city )
				szLine[iX] = 'c';
			else if( (int)aiHex.m_cTerrain == CHex::river )
				szLine[iX] = 'v';
			else 
				szLine[iX] = '.';

			i = GetMapOffset( x, y );
			if( i >= m_iMapSize )
				continue;

			WORD wStatus = m_pMap[i];
			if( !wStatus )
				continue;

			if( wStatus & MSW_PLANNED_ROAD )
				szLine[iX] = 'R';
			if( wStatus & MSW_ROAD )
				szLine[iX] = 'A';
			
			if( wStatus & MSW_AI_BUILDING )
			{
				// get building type out of hi byte
				int iBldg = wStatus >> 8;
				if( iBldg == CStructureData::power )
					szLine[iX] = 'P';
				else if( iBldg == CStructureData::power_1 )
					szLine[iX] = 'P';
				else if( iBldg == CStructureData::power_2 )
					szLine[iX] = 'P';
				else if( iBldg == CStructureData::power_3 )
					szLine[iX] = 'P';
				else if( iBldg == CStructureData::farm )
					szLine[iX] = 'F';
				else if( iBldg == CStructureData::fort_1 )
					szLine[iX] = 'F';
				else if( iBldg == CStructureData::lumber )
					szLine[iX] = 'L';
				else if( iBldg == CStructureData::coal )
					szLine[iX] = 'M';
				else if( iBldg == CStructureData::iron )
					szLine[iX] = 'M';
				else if( iBldg == CStructureData::copper )
					szLine[iX] = 'M';
				else if( iBldg == CStructureData::smelter )
					szLine[iX] = '#';
				else if( iBldg == CStructureData::seaport )
					szLine[iX] = '$';
				else if( iBldg == CStructureData::shipyard_1 )
					szLine[iX] = '&';
				else if( iBldg == CStructureData::shipyard_3 )
					szLine[iX] = '&';
				else if( iBldg == CStructureData::barracks_2 )
					szLine[iX] = 'B';
				//else if( iBldg == CStructureData::barracks_3 )
				//	szLine[iX] = 'B';
				else if( iBldg == CStructureData::command_center )
					szLine[iX] = 'Q';
				else if( iBldg == CStructureData::heavy )
					szLine[iX] = 'W';
				else if( iBldg == CStructureData::light_1 )
					szLine[iX] = 'W';
				else if( iBldg == CStructureData::light_2 )
					szLine[iX] = 'W';
				else if( iBldg == CStructureData::oil_well )
					szLine[iX] = 'O';
				else if( iBldg == CStructureData::refinery )
					szLine[iX] = 'Y';
				else if( iBldg == CStructureData::rocket )
					szLine[iX] = '|';
				
			}
			if( wStatus & MSW_AI_VEHICLE )
			{
				int iVeh = wStatus >> 8;
				if( iVeh == CTransportData::construction )
					szLine[iX] = 'C';
				else if( iVeh == CTransportData::light_scout )
					szLine[iX] = 'S';
				else if( iVeh == CTransportData::heavy_truck )
					szLine[iX] = 'T';
			}
		}
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"%s ", szLine );

	}
}
#endif

#ifdef _LOGOUT
//
// this function reports the group hexes indicated by the
// passed hex (in the size iWidthX and iWidthY) for the
// passed building
//
void CAIMapUtil::ReportGroupHex( int iBldg, int iWidthX,
	int iWidthY, CHexCoord& hex )
{
	CString sName;
	CStructureData const *pBldgData = 
		pGameData->GetStructureData( iBldg );
	if( pBldgData != NULL )
	{
		sName = pBldgData->GetDesc();
	}
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"Player %d Placing a %s at %d,%d ", 
		m_iPlayer, (const char *)sName, hex.X(), hex.Y() );

	sName.Empty();

	ClearGroupHexes();
	int isy = hex.Y();
	int isx = hex.X();
	LoadGroupHexes( isx, isy, iWidthX, iWidthY );

	CMinerals *pmn;
	CHexCoord hexMin;
	int iMulti = 0;

	CAIHex *paiGrpHex = NULL;
	for( int i=0; i<m_iGrpSize; ++i )
	{
		paiGrpHex = &m_pgrpHexs[i];

		// end of active hexes in group
		if( !paiGrpHex->m_iUnit )
			break;

		int iAt = GetMapOffset( paiGrpHex->m_iX, paiGrpHex->m_iY );
		if( iAt >= m_iMapSize )
			continue;

		// get location from game data, and build multiplier
		pGameData->GetCHexData(paiGrpHex);
		CTerrainData const *pTerrain = 
			pGameData->GetTerrainData( (int)paiGrpHex->m_cTerrain );

		sName = pTerrain->GetDesc();

		logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"Hex %d,%d is %s map value = %d ", 
			paiGrpHex->m_iX, paiGrpHex->m_iY, 
			(const char *)sName, m_pMap[iAt] );

		sName.Empty();

		iMulti += pTerrain->GetBuildMult();

		logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"Build Multi is %d, Farm Multi is %d ", 
			pTerrain->GetBuildMult(), pTerrain->GetFarmMult() );


		hexMin.X( paiGrpHex->m_iX );
		hexMin.Y( paiGrpHex->m_iY );

		// check hex for material
		if( theMinerals.Lookup(hexMin, pmn) )
		{
			sName = pmn->GetStatus ();

			logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
			"Mineral found: %s ", (const char *)sName );

			sName.Empty();
		}

		if( paiGrpHex->m_dwUnitID )
		{
			logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
			"Unit found: %ld ", paiGrpHex->m_dwUnitID );

		}
	}
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"Total Build Multiplier is %d \n", iMulti );

}
#endif

// end of CAIMapUt.cpp
