////////////////////////////////////////////////////////////////////////////
//
//  CAIInit.cpp :  CAIInitPos object implementation
//                 Divide and Conquer AI
//               
//  Last update:  10/28/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "lastplnt.h"
#include "CAIInit.hpp"
#include "CAIData.hpp"

#include "logging.h"	// dave's logging system

extern CException *pException;	// standard exception for yielding
extern CRITICAL_SECTION cs;	// used by threads
extern CAIData *pGameData;	// pointer to game data interface

#ifdef _LOGOUT
extern CConquerApp theApp;
#endif

#define new DEBUG_NEW


//
// HP number of units disgorging from rocket is controled
// in new_unit.cpp
//
//void CBuilding::DropUnits (int iVehTyp, int iWheelTyp, int iNum, 
//int iNumSkip, int *piTime, 
//CHexCoord const & hex, CHexCoord const & hexUL)


CAIInitPos::CAIInitPos( int iPlayer, 
	CAIMap *pMap, CAIUnitList *plUnits )
{
	m_iPlayer = iPlayer;
	m_pMap = pMap;
	m_plUnits = plUnits;
}

// dwID = theGame.GetID ();
//
// this function will set the player's initial position
// of buildings and vehicles as found in the 
// CPlayer::m_InitAttrib[NUM_RACE_CHAR to NUM_RACE_CHUNKS-1]
// 
// 
void CAIInitPos::DoIt( void )
{
	CAIUnit *pUnit = NULL;
	CPlayer *pPlayer = NULL;
	int iCnt,i;
	CHexCoord hex( 0,0 );


	// temporary for testing
	/*
	CSubHex subHexCargo;
	CSubHex subHexTail;
	subHexCargo.x = 2;
	subHexCargo.y = 80;
	subHexTail.x = 119;
	subHexTail.y = 82;
	i = subHexCargo.Dist( subHexTail );
    wsprintf( szLogMsg, 
    	"\nCSubHex::Dist() is %d subHexCargo %d,%d subHexTail %d,%d \n\n", 
    	i, subHexCargo.x, subHexCargo.y, subHexTail.x, subHexTail.y );
    pLog->Write( szLogMsg );
	*/


#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif

	// place rocket on initial hex
	PlaceRocket();
	m_pMap->RocketRoad();

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"\nPlaceRocket() player %d took %ld ticks at difficulty %d \n", 
		m_iPlayer, (dwEnd - dwStart), pGameData->m_iSmart );

#endif

	const int iSup [] = { CRaceDef::med_scout, CRaceDef::med_tank,
											CRaceDef::light_tank, CRaceDef::light_art,
											CRaceDef::crane, CRaceDef::heavy_truck,
											CRaceDef::light_scout, CRaceDef::infantry,
											CRaceDef::infantry_carrier };
	const int iTyp [] = { CTransportData::med_scout, CTransportData::med_tank,
											CTransportData::light_tank, CTransportData::light_art,
											CTransportData::construction, CTransportData::heavy_truck,
											CTransportData::light_scout, CTransportData::infantry,
											CTransportData::infantry_carrier };
	const int NUM_VEH = sizeof (iSup) / sizeof (int);
	for (int iInd=0; iInd<NUM_VEH; iInd++)
		{
		EnterCriticalSection (&cs);
		pPlayer = theGame.GetPlayerByPlyr( m_iPlayer );
		if( pPlayer != NULL )
			iCnt = pPlayer->m_InitData.GetSupplies ( iSup[iInd] );
		else
			iCnt = 0;
		LeaveCriticalSection (&cs);

		for( i=0; i<iCnt; ++i )
			{
			pUnit = new CAIUnit( theGame.GetID(), m_iPlayer, CUnit::vehicle, iTyp [iInd] );
			m_plUnits->AddTail( (CObject *)pUnit );

			PlaceVehicle( pUnit, iTyp [iInd] );
			}
		}

#ifdef BUGBUG
	// use the player's count of CInitAttrib::apartment to
	// contain the number of med scouts to deploy
	EnterCriticalSection (&cs);
	pPlayer = theGame.GetPlayerByPlyr( m_iPlayer );
	if( pPlayer != NULL )
		iCnt = pPlayer->m_InitData.GetSupplies(CRaceDef::med_scout);
	else
		iCnt = 0;
	LeaveCriticalSection (&cs);


#ifdef _LOGOUT
	iCnt = theApp.GetProfileInt("Cheat","AiMedScouts",iCnt);
#endif


	for( i=0; i<iCnt; ++i )
	{
		try
		{
			// CAIUnit( DWORD dwID, int iOwner );
			pUnit = new CAIUnit( theGame.GetID(), 
				m_iPlayer, CUnit::vehicle, CTransportData::med_scout );
			ASSERT_VALID( pUnit );
			m_plUnits->AddTail( (CObject *)pUnit );
		}
		catch( CException e )
		{
			// BUGBUG need to report this error occurred
			throw(ERR_CAI_BAD_NEW);
		}


#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif

		PlaceVehicle( pUnit, CTransportData::med_scout );

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"PlaceVehicle() CTransportData::med_scout %ld took %ld ticks \n", 
    	pUnit->GetID(), (dwEnd - dwStart) );
#endif

	}



	// contain the number of med tanks to deploy
	EnterCriticalSection (&cs);
	pPlayer = theGame.GetPlayerByPlyr( m_iPlayer );
	if( pPlayer != NULL )
		iCnt = pPlayer->m_InitData.GetSupplies(CRaceDef::med_tank);
	else
		iCnt = 0;
	LeaveCriticalSection (&cs);


#ifdef _LOGOUT
	iCnt = theApp.GetProfileInt("Cheat","AiMedTanks",iCnt);
#endif


	for( i=0; i<iCnt; ++i )
	{
		try
		{
			// CAIUnit( DWORD dwID, int iOwner );
			pUnit = new CAIUnit( theGame.GetID(), 
				m_iPlayer, CUnit::vehicle, CTransportData::med_tank );
			ASSERT_VALID( pUnit );
			m_plUnits->AddTail( (CObject *)pUnit );
		}
		catch( CException e )
		{
			// BUGBUG need to report this error occurred
			throw(ERR_CAI_BAD_NEW);
		}


#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif

		PlaceVehicle( pUnit, CTransportData::med_tank );

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"PlaceVehicle() CTransportData::med_tank %ld took %ld ticks \n", 
    	pUnit->GetID(), (dwEnd - dwStart) );
#endif

	}



	// use the player's count of CInitAttrib::farm to
	// contain the number of light tanks to deploy
	EnterCriticalSection (&cs);
	pPlayer = theGame.GetPlayerByPlyr( m_iPlayer );
	if( pPlayer != NULL )
		iCnt = pPlayer->m_InitData.GetSupplies(CRaceDef::light_tank);
	else
		iCnt = 0;
	LeaveCriticalSection (&cs);


#ifdef _LOGOUT
	iCnt = theApp.GetProfileInt("Cheat","AiLightTanks",iCnt);
#endif


	for( i=0; i<iCnt; ++i )
	{
		try
		{
			// CAIUnit( DWORD dwID, int iOwner );
			pUnit = new CAIUnit( theGame.GetID(), 
				m_iPlayer, CUnit::vehicle, CTransportData::light_tank );
			ASSERT_VALID( pUnit );
			m_plUnits->AddTail( (CObject *)pUnit );
		}
		catch( CException e )
		{
			// BUGBUG need to report this error occurred
			throw(ERR_CAI_BAD_NEW);
		}


#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif

		PlaceVehicle( pUnit, CTransportData::light_tank );

#ifdef _LOGOUT
dwEnd = timeGetTime();
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"PlaceVehicle() CTransportData::light_tank %ld took %ld ticks \n", 
pUnit->GetID(), (dwEnd - dwStart) );
#endif

	}


	// use the player's count of CInitAttrib::power to
	// contain the number of light artillery to deploy
	EnterCriticalSection (&cs);
	pPlayer = theGame.GetPlayerByPlyr( m_iPlayer );
	if( pPlayer != NULL )
		iCnt = pPlayer->m_InitData.GetSupplies(CRaceDef::light_art);
	else
		iCnt = 0;
	LeaveCriticalSection (&cs);


#ifdef _LOGOUT
	iCnt = theApp.GetProfileInt("Cheat","AiLightArts",iCnt);
#endif


	for( i=0; i<iCnt; ++i )
	{
		try
		{
			// CAIUnit( DWORD dwID, int iOwner );
			pUnit = new CAIUnit( theGame.GetID(), 
				m_iPlayer, CUnit::vehicle, CTransportData::light_art );
			ASSERT_VALID( pUnit );
			m_plUnits->AddTail( (CObject *)pUnit );
		}
		catch( CException e )
		{
			// BUGBUG need to report this error occurred
			throw(ERR_CAI_BAD_NEW);
		}

#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif
		PlaceVehicle( pUnit, CTransportData::light_art );

#ifdef _LOGOUT
dwEnd = timeGetTime();
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"PlaceVehicle() CTransportData::light_art %ld player %d took %ld ticks \n", 
pUnit->GetID(), m_iPlayer, (dwEnd - dwStart) );
#endif

	}

	// at highest difficulty level add super tanks
	if( pGameData->m_iSmart >= (NUM_DIFFICUTY_LEVELS-1) )
	{
		for( i=0; i<iCnt; ++i )
		{
			try
			{
			// CAIUnit( DWORD dwID, int iOwner );
			pUnit = new CAIUnit( theGame.GetID(), 
				m_iPlayer, CUnit::vehicle, CTransportData::heavy_tank );
			ASSERT_VALID( pUnit );
			m_plUnits->AddTail( (CObject *)pUnit );
			}
			catch( CException e )
			{
			// BUGBUG need to report this error occurred
			throw(ERR_CAI_BAD_NEW);
			}

#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif
			PlaceVehicle( pUnit, CTransportData::heavy_tank );

#ifdef _LOGOUT
	dwEnd = timeGetTime();
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"PlaceVehicle() CTransportData::heavy_tank %ld player %d took %ld ticks \n", 
    	pUnit->GetID(), m_iPlayer, (dwEnd - dwStart) );
#endif

		}
	}

	//
	// get count of construction trucks
	//
	EnterCriticalSection (&cs);
	pPlayer = theGame.GetPlayerByPlyr( m_iPlayer );
	if( pPlayer != NULL )
		iCnt = pPlayer->m_InitData.GetSupplies(CRaceDef::crane);
	else
		iCnt = 0;
	LeaveCriticalSection (&cs);


#ifdef _LOGOUT
	iCnt = theApp.GetProfileInt("Cheat","AiCranes",iCnt);
#endif


	for( i=0; i<iCnt; ++i )
	{
		try
		{
			// CAIUnit( DWORD dwID, int iOwner );
			pUnit = new CAIUnit( theGame.GetID(), 
				m_iPlayer, CUnit::vehicle, CTransportData::construction );
			ASSERT_VALID( pUnit );
			m_plUnits->AddTail( (CObject *)pUnit );
		}
		catch( CException e )
		{
			// BUGBUG need to report this error occurred
			throw(ERR_CAI_BAD_NEW);
		}

#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif
		//PlaceVehicle( pMap, pUnit, CTransportData::construction );
		PlaceVehicle( pUnit, CTransportData::construction );

#ifdef _LOGOUT
dwEnd = timeGetTime();
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"PlaceVehicle() CTransportData::construction %ld took %ld ticks \n", 
pUnit->GetID(), (dwEnd - dwStart) );
#endif

	}

	// get count of trucks
	EnterCriticalSection (&cs);
	pPlayer = theGame.GetPlayerByPlyr( m_iPlayer );
	if( pPlayer != NULL )
		iCnt = pPlayer->m_InitData.GetSupplies(CRaceDef::heavy_truck);
	else
		iCnt = 0;
	LeaveCriticalSection (&cs);


#ifdef _LOGOUT
	iCnt = theApp.GetProfileInt("Cheat","AiTrucks",iCnt);
#endif


	for( i=0; i<iCnt; ++i )
	{
		try
		{
			// CAIUnit( DWORD dwID, int iOwner );
			pUnit = new CAIUnit( theGame.GetID(), 
				m_iPlayer, CUnit::vehicle, CTransportData::heavy_truck );
				
			ASSERT_VALID( pUnit );
			m_plUnits->AddTail( (CObject *)pUnit );
		}
		catch( CException e )
		{
			// BUGBUG need to report this error occurred
			throw(ERR_CAI_BAD_NEW);
		}

#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif
		PlaceVehicle( pUnit, CTransportData::heavy_truck );
		

#ifdef _LOGOUT
dwEnd = timeGetTime();
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"PlaceVehicle() CTransportData::heavy_truck %ld took %ld ticks \n", 
pUnit->GetID(), (dwEnd - dwStart) );
#endif
	}

	// get count of scouts
	EnterCriticalSection (&cs);
	pPlayer = theGame.GetPlayerByPlyr( m_iPlayer );
	if( pPlayer != NULL )
		iCnt = pPlayer->m_InitData.GetSupplies(CRaceDef::light_scout);
	else
		iCnt = 0;
	LeaveCriticalSection (&cs);


#ifdef _LOGOUT
	iCnt = theApp.GetProfileInt("Cheat","AiLightScouts",iCnt);
#endif

/*
	// bump up scouts to tanks for higher difficulty
	int iVeh = 0;
	if( pGameData->m_iSmart >= (NUM_DIFFICUTY_LEVELS-1) )
		iVeh = CTransportData::med_tank;
	else
		iVeh = CTransportData::light_scout;
*/

	for( i=0; i<iCnt; ++i )
	{
		try
		{
			// CAIUnit( DWORD dwID, int iOwner );
			pUnit = new CAIUnit( theGame.GetID(), 
				//m_iPlayer, CUnit::vehicle, iVeh );
				m_iPlayer, CUnit::vehicle, CTransportData::light_scout );
				
			ASSERT_VALID( pUnit );
			m_plUnits->AddTail( (CObject *)pUnit );
		}
		catch( CException e )
		{
			// BUGBUG need to report this error occurred
			throw(ERR_CAI_BAD_NEW);
		}

	//
	// place each IDIP_SCOUT in farthest known corner of the quads
	// relative to the location of the IDIP_POWERPLANT such that
	// one IDIP_SCOUT is in each quad
	//
#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif
		PlaceVehicle( pUnit, CTransportData::light_scout );
		//PlaceVehicle( pUnit, iVeh );

#ifdef _LOGOUT
dwEnd = timeGetTime();
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"Place Scout %ld took %ld ticks \n", 
pUnit->GetID(), (dwEnd - dwStart) );
#endif

	}

	// get count of infantry
	EnterCriticalSection (&cs);
	pPlayer = theGame.GetPlayerByPlyr( m_iPlayer );
	if( pPlayer != NULL )
		iCnt = pPlayer->m_InitData.GetSupplies(CRaceDef::infantry);
	else
		iCnt = 0;
	LeaveCriticalSection (&cs);


#ifdef _LOGOUT
	iCnt = theApp.GetProfileInt("Cheat","AiInfantry",iCnt);
#endif


	for( i=0; i<iCnt; ++i )
	{
		try
		{
			// CAIUnit( DWORD dwID, int iOwner );
			pUnit = new CAIUnit( theGame.GetID(), 
				m_iPlayer, CUnit::vehicle, CTransportData::infantry );
				
			ASSERT_VALID( pUnit );
			m_plUnits->AddTail( (CObject *)pUnit );
		}
		catch( CException e )
		{
			// BUGBUG need to report this error occurred
			throw(ERR_CAI_BAD_NEW);
		}

#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif
		PlaceVehicle( pUnit, CTransportData::infantry );
		
#ifdef _LOGOUT
dwEnd = timeGetTime();
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"PlaceVehicle() CTransportData::infantry %ld took %ld ticks \n", 
pUnit->GetID(), (dwEnd - dwStart) );
#endif
	}

	// get count of infantry_carrier
	EnterCriticalSection (&cs);
	pPlayer = theGame.GetPlayerByPlyr( m_iPlayer );
	if( pPlayer != NULL )
		iCnt = pPlayer->m_InitData.GetSupplies(CRaceDef::infantry_carrier);
	else
		iCnt = 0;
	LeaveCriticalSection (&cs);


#ifdef _LOGOUT
	iCnt = theApp.GetProfileInt("Cheat","AiIFV",iCnt);
#endif


	for( i=0; i<iCnt; ++i )
	{
		try
		{
			// CAIUnit( DWORD dwID, int iOwner );
			pUnit = new CAIUnit( theGame.GetID(), 
				m_iPlayer, CUnit::vehicle, CTransportData::infantry_carrier );
				
			ASSERT_VALID( pUnit );
			m_plUnits->AddTail( (CObject *)pUnit );
		}
		catch( CException e )
		{
			// BUGBUG need to report this error occurred
			throw(ERR_CAI_BAD_NEW);
		}

#ifdef _LOGOUT
	DWORD dwStart, dwEnd;
	dwStart = timeGetTime(); 
#endif
		PlaceVehicle( pUnit, CTransportData::infantry_carrier );
		
#ifdef _LOGOUT
dwEnd = timeGetTime();
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"PlaceVehicle() CTransportData::infantry_carrier %ld took %ld ticks \n", 
pUnit->GetID(), (dwEnd - dwStart) );
#endif
	}


#if 0 //DEBUG_OUTPUT_IP
	pMap->ReportFakeMap();
#endif

#endif

	// make sure units start clear after being placed
   	POSITION pos = m_plUnits->GetHeadPosition();
   	while( pos != NULL )
   	{   
       	CAIUnit *pUnit = (CAIUnit *)m_plUnits->GetNext( pos );
       	if( pUnit != NULL )
       	{
       		ASSERT_VALID( pUnit );
			pUnit->SetDataDW(0);
			pUnit->ClearParam();
			pUnit->SetStatus(0);
		}
	}
}

// 
// places a fully constructed instance of the building
// type passed at the hex passed
//
void CAIInitPos::PlaceBuilding( CAIUnit *pUnit, CHexCoord& hex, int iBldg)
{
	// BUGBUG how should no hex found be reported?
	if( !hex.Y() && !hex.X() )
		return;

	// place fake building in map array
	m_pMap->PlaceFakeBldg( hex, iBldg );

	// only placement updates the unit's params
	pUnit->SetParam( CAI_TYPEBUILD, iBldg );
	pUnit->SetParam( CAI_LOC_X, hex.X() );
	pUnit->SetParam( CAI_LOC_Y, hex.Y() );

	//CMsgPlaceBldg (CHexCoord const & hex, int iDir, int iBldg);
	CMsgPlaceBldg msg( hex, 0, iBldg );
	msg.m_dwIDBldg = pUnit->GetID();
	msg.m_iPlyrNum = m_iPlayer;

	theGame.PostToServer( (CNetCmd *)&msg, sizeof(CMsgPlaceBldg) );

	// send message to CAIMgr to build planned roads to placed buildings
	CAIMsg aiMsg( (CNetCmd const *)&msg );
	m_pMap->PlanRoads( &aiMsg );
}

// 
// places a fully produced instance of the vehicle
// type based on type of vehicle and current other
// vehicles locations
//
void CAIInitPos::PlaceVehicle( CAIUnit *pUnit, int iVeh )
{
	// yield for the music's sake
	theApp.BaseYield();

	CHexCoord hexPlace(0,0);
	GetVehInitPosHex( iVeh, hexPlace );
	if( !hexPlace.Y() && !hexPlace.X() )
		return;

	// BUGBUG place fake vehicle in map array
	m_pMap->PlaceFakeVeh( hexPlace, iVeh );

	// only placement updates the unit's params
	pUnit->SetParam( CAI_TYPEVEHICLE, iVeh );
	pUnit->SetParam( CAI_LOC_X, hexPlace.X() );
	pUnit->SetParam( CAI_LOC_Y, hexPlace.Y() );

#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
		"PlaceVehicle:: id=%ld type=%d at %d,%d ", 
		pUnit->GetID(), iVeh, hexPlace.X(), hexPlace.Y() );
#endif

	// message to place unit and cause CVehicle to be created
	//CMsgPlaceVeh (int iType, CHexCoord const & hex, CHexCoord const & hexDest);
	//CMsgPlaceVeh (CHexCoord const & hexVeh, CHexCoord const & hexDest, 
	//int iPlyr, int iType);
	CMsgPlaceVeh msg( hexPlace, hexPlace, m_iPlayer, iVeh );
	msg.m_dwID = pUnit->GetID();
	theGame.PostToServer( (CNetCmd *)&msg, sizeof(CMsgPlaceVeh) );

	// send message to self that this is a new vehicle
}

// BUGBUG this function may change due to forthcoming
// change to initial position buildings and vehicles
//
// this function will select a hex in which to place an
// instance of the vehicle type indicated
//
void CAIInitPos::GetVehInitPosHex( int iVeh, CHexCoord& hexPlace )
{
	switch( iVeh )
	{
	// assign each IDIP_SCOUT with respective Scout BlockX tasks
	// of IDG_BASICDEFENSE, for the quad the IDIP_SCOUT is located
	//
		case CTransportData::light_scout: // VEHICLE_SCOUT:
		case CTransportData::med_scout:
			//hexPlace = m_hexRocket;
			//m_pMap->GetStagingHex( m_hexRocket, 3, 3, iVeh, hexPlace, FALSE );
			m_pMap->GetStartHex( m_hexStart, m_hexEnd, hexPlace, iVeh );
			break;
	// place a IDIP_JEEP adjacent to the IDIP_FARM location
	// place a IDIP_JEEP adjacent to the IDIP_LOGCAMP location
	// place a IDIP_JEEP adjacent to the IDIP_FOODPROC location
	// place a IDIP_JEEP adjacent to the IDIP_APARTMENT location
	// assign all IDIP_JEEP to Patrol Range task of IDG_BASICDEFENSE
	//
	// BUGBUG would like to place heavys near producers
	//
		case CTransportData::construction:  // VEHICLE_CONSTRUCT:
		case CTransportData::heavy_truck: // VEHICLE_TRUCK:
			//hexPlace = m_hexRocket;
			//m_pMap->GetStagingHex( m_hexRocket, 2, 2, iVeh, hexPlace, FALSE );
			m_pMap->GetStartHex( m_hexStart, m_hexEnd, hexPlace, iVeh );
			break;
	// place 2 IDIP_TRUCK adjacent to each IDIP_FARM and IDIP_LOGCAMP
	// place remaining IDIP_TRUCK adjacent to IDIP_FOODPROC and
	// IDIP_LUMMILL respectively
	// assign Set Transport to all IDIP_TRUCK relative to the 
	// location placed to transport production to processor
	//
	// BUGBUG would like to place heavys near producers
	//
		case CTransportData::light_tank:
		case CTransportData::light_art:
		case CTransportData::med_tank:
			// now ask map to do the work and find a place to stage
			//hexPlace = m_hexRocket;
			//m_pMap->GetStagingHex( m_hexRocket, 3, 3, iVeh, hexPlace, FALSE );
			m_pMap->GetStartHex( m_hexStart, m_hexEnd, hexPlace, iVeh );
			break;
		case CTransportData::infantry:
		case CTransportData::infantry_carrier:
		default:
			//hexPlace = m_hexRocket;
			//m_pMap->GetStagingHex( m_hexRocket, 2, 2, iVeh, hexPlace, FALSE );
			m_pMap->GetStartHex( m_hexStart, m_hexEnd, hexPlace, iVeh );
			break;
	}
}

//
// tell game to place the AI's rocket on the map at the location
// determined by the CAIStart object of CAIMgr
//
void CAIInitPos::PlaceRocket( void )
{
	// first create a CAIUnit for it
	CAIUnit *pUnit = NULL;
	try
	{
		// CAIUnit( DWORD dwID, int iOwner );
		pUnit = new CAIUnit( theGame.GetID(), 
			m_iPlayer, CUnit::building, CStructureData::rocket );
		ASSERT_VALID( pUnit );
		m_plUnits->AddTail( (CObject *)pUnit );
	}
	catch( CException e )
	{
		// BUGBUG need to report this error occurred
		throw(ERR_CAI_BAD_NEW);
	}

	// then put it on the selected hex and record it
	CHexCoord hex;
	m_pMap->PlaceRocket( hex );
	m_hexRocket = hex;

	// determine the start of the block based on the start hex
	if( m_hexRocket.X() > pGameData->m_iHexPerBlk )
		m_hexStart.X( (m_hexRocket.X() / pGameData->m_iHexPerBlk) * 
		pGameData->m_iHexPerBlk );
	else
		m_hexStart.X( 0 );

	if( m_hexRocket.Y() > pGameData->m_iHexPerBlk )
		m_hexStart.Y( (m_hexRocket.Y() / pGameData->m_iHexPerBlk) * 
		pGameData->m_iHexPerBlk );
	else
		m_hexStart.Y( 0 );

	m_hexEnd.X( m_hexEnd.Wrap( (m_hexStart.X() + pGameData->m_iHexPerBlk) ) );
	m_hexEnd.Y( m_hexEnd.Wrap( (m_hexStart.Y() + pGameData->m_iHexPerBlk) ) );
	
	// only placement updates the unit's params
	pUnit->SetParam( CAI_TYPEBUILD, CStructureData::rocket );
	pUnit->SetParam( CAI_LOC_X, hex.X() );
	pUnit->SetParam( CAI_LOC_Y, hex.Y() );

	// now tell the game
	//CMsgPlaceBldg (CHexCoord const & hex, int iDir, int iBldg);
	CMsgPlaceBldg msg( hex, 0, CStructureData::rocket );
	msg.m_dwIDBldg = pUnit->GetID();
	msg.m_iPlyrNum = m_iPlayer;

	theGame.PostToServer( (CNetCmd *)&msg, sizeof(CMsgPlaceBldg) );
}

// end of CAIInit.cpp
