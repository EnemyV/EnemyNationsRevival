////////////////////////////////////////////////////////////////////////////
//
//  CAIData.hpp :  CAIData object declaration
//                 Divide and Conquer AI
//               
//  Last update:    09/17/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "terrain.inl"
#include "minerals.inl"
#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"
#include "netapi.h"
#include "netcmd.h"
#include "player.h"
#include "threads.h"

#include "cai.h"
#include "caihex.hpp"
#include "caiunit.hpp"

#ifndef __CAIDATA_HPP__
#define __CAIDATA_HPP__


//
// this class provides an interface to the game database for the AI
//
class CAIData : public CObject
{
public:

	int m_iStartPos; // the type of assortment of units
	int m_iSmart; // 0 (Easy to beat) - 3 (Impossible) - how smart the AI is
	int m_iNumAi; // number of AI players in game
	int m_iNumHuman; // number of human players in the game
	int m_iHexPerBlk; // the number of hex's a block is wide or high 
	                  // (a block is ALWAYS square).
	                  // This is presently 64, 128, or 256
	int m_iBlkPerSide; // the number of blocks a map is wide or high 
	                   // (the map is always square).
					   // Therefore the number of hexes wide (or high) 
					   // a map is is iHexPerBlk * iBlkPerSide
	int m_iStructureSize; // largest size in width and height of
						  // the structures that can be built

	int m_iNumTransports;	// stored here to reduce access time
	int m_iNumBuildings;

	CAIHex *m_paihDensity;	// store best mineral density here

	CAIData( int iSmart, int iNumAi, int iNumHuman );
	~CAIData();

	void SetWorldSize( int iHexPerBlk, int iBlkPerSide );

	// **** Accessing types of data

	// vehicle type
	CTransportData const * GetTransportData (int iIndex) const;

	// inerate thru vehicle type data
	// passing iIndex == NULL, return first instance;
	//         iIndex != NULL, return instance following iIndex;
	CTransportData const * GetNextTransport (int iIndex) const;

	// building type
	CStructureData const * GetStructureData (int iIndex) const;

	// inerate thru building type data
	// passing iIndex == NULL, return first instance;
	//         iIndex != NULL, return instance following iIndex;
	CStructureData const * GetNextStructure (int iIndex) const;

	// terrain type
	CTerrainData const * GetTerrainData (int iIndex) const;

	// inerate thru terrain type data
	// passing iIndex == NULL, return first instance;
	//         iIndex != NULL, return instance following iIndex;
	CTerrainData const & GetNextTerrain (int iIndex) const;

	// **** Accessing instances of vehicle, building and player data

	// specific unit or building
	void GetBldgExit( DWORD dwID, CHexCoord& hex );
	CBuilding *GetBuildingData( int iPlayer, DWORD dwID);
	int GetBuilding(DWORD wdID, CAIUnit *paiUnit );
	CVehicle *GetVehicleData( int iPlayer, DWORD dwID);
	int GetVehicle(DWORD wdID, CAIUnit *paiUnit );
	void GetVehicleHex(DWORD wdID, CHexCoord& hex );

	void FindBuilding( int iBldg, int iPlayer, CHexCoord& hexAt );

	int GetMaterialCapacity( CAIUnit *paiUnit );
	int GetQtyMaterialAt( CAIUnit *paiUnit, int iMaterial );

	void ClearUnitEvent( CAIUnit *paiUnit );

	void BuildBridgeAt( CAIUnit *paiUnit, CHexCoord& hexStart, CHexCoord& hexEnd );
	void BuildRoadAt( CHexCoord& hexSite, CAIUnit *paiUnit );
	void BuildAt( CHexCoord& hexSite, int iBldg, int iDir, CAIUnit *paiUnit );

	BOOL IsArtillery( int iVehType );
	BOOL IsCombatVehicle( int iVehType );
	BOOL IsTruck( DWORD dwID );
	BOOL IsLoaded( DWORD dwID );
	BOOL IsHexOfUnit( CAIHex *pHex );

	// inerate thru unit or building data
	// passing dwID == NULL, return first instance known;
	//         dwID != NULL, return instance known following dwID;
	// always return id of next known; NULL to end
	CBuilding const* GetNextBuilding( int iPlayer, DWORD dwID) const;
	CVehicle const* GetNextVehicle( int iPlayer, DWORD dwID) const;

	// specific player
	CPlayer* GetPlayerData (int iPlyrNum) const;
	int GetOwnerID( DWORD dwID );

	// inerate thru player data
	// passing iPlyrNum == NULL, return first instance known;
	//         iPlyrNum != NULL, return instance known following iPlyrNum;
	CPlayer const* GetNextPlayer (int iPlyrNum) const;


	// specific location from the map
	int GetCHexData( CAIHex *pHex );
	int GetCHexTerrain( CAIHex *pHex );

	// get range in distance between locations
	int GetRangeDistance( CHexCoord& hFrom, CHexCoord& hTo );
	// get range in time between locations
	int GetRangeTime( CHexCoord& hFrom, CHexCoord& hTo, int iType );
	// get a random number <= number passed
	int GetRandom( int iDice );

//#if _LOGOUT
	 const char* GetMsgString( int iMsg );
//#endif
};

#endif // __CAIDATA_HPP__
