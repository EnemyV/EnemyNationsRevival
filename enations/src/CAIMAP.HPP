////////////////////////////////////////////////////////////////////////////
//
//  CAIMap.hpp : CAIMap object declaration
//               Divide and Conquer AI
//               
//  Last update:    09/13/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAIMsg.hpp"
#include "CAIMapUt.hpp"

#ifndef __CAIMAP_HPP__
#define __CAIMAP_HPP__

//
// this class maintains a block of locations, in start block size chunks
//
class CAIMap : public CObject
{
friend class CAIMapUtil;

protected:
	int m_iPlayer;			// AI Player for whom the map serves
	WORD m_wRows;			// the blocksize of the map in rows/cols
	WORD m_wCols;
	WORD m_wBaseRow;		// the starting row/col of the map
	WORD m_wBaseCol;

	BYTE m_cMainRoads;	// predetermined number of main roads
		
	WORD *m_pwaMap;		// the actual map
	int m_iMapSize;

	WORD m_wStatus;			// general purpose status word

public:
	CAIMapUtil *m_pMapUtil;

	int m_iRoadCount;	// count of MSW_PLANNED_ROAD locations left
	int m_iOcean;		// count of terrain == ocean
	int m_iLake;		// count of terrain == lake
	int m_iLand;		// count of all other terrain

	int m_iBaseX;	// the base hex location originally made
	int m_iBaseY;	// available to the AI
	//CHexCoord m_RocketHex; // the hex where this player starts

	CAIMap() {};
	~CAIMap();
	CAIMap( int iPlayer, CAIUnitList *pUnits,
		WORD wBaseCol, WORD wBaseRow, 
		WORD wCols, WORD wRows );
	
	WORD GetRows( void );
	WORD GetCols( void );
	int GetPlayer( void );

	void ConfirmPlacement( CHexCoord& hex );

	void SetMainRoad( BYTE cLayout );
	BYTE GetMainRoad( void );

	// BUGBUG these are temporary routine
#ifdef _LOGOUT
	void ReportFakeMap( void );
#endif

	// these do an AI map update before the real update
	// so that the AI can know that it is there before
	// the game gets around to putting it there
	void PlaceFakeVeh( CHexCoord& hex, int iVeh );
	void PlaceFakeBldg( CHexCoord& hex, int iBldg );


	void RocketRoad( void );
	void PlanRoads( CAIMsg *pMsg );
	void PlanRoad( DWORD dwID );
	void PlanRoad( CAIHex *paiHex );
	BOOL ConnectRoad( CHexCoord& hexFrom, CHexCoord& hexTo );
	
	void Initialize( void );
	void UpdateLoc(	CAIMsg *pMsg );
	void UpdateMap( CAIMsg *pMsg );	
	void UpdateHex( int iX, int iY );


	WORD GetLocation( WORD wCol, WORD wRow );
	void SetLocation( WORD wCol, WORD wRow, WORD wStatus );

	void PlaceRocket( CHexCoord& hex );
	void PlacePowerPlant(CHexCoord& hex, int iBldg);
	void PlaceBuilding( CAIMsg *pMsg, CAIUnitList *plUnits );
	void PlaceProducer( int iBldg, CHexCoord& hex);
	void PlaceScout(CHexCoord& hex);
	void PlaceVehicleNextTo( int iBldg, CHexCoord& hex);

	//CAIHex *GetPatrolPoints( CHexCoord& hexNearBy );

	void GetStartHex( CHexCoord& hexStart, CHexCoord& hexEnd, 
		CHexCoord& hexPlace, int iVehType );

	void GetStagingHex( CHexCoord& hexNearBy, 
		int iWidth, int iHeight, int iVehType, 
		CHexCoord& hexDest, BOOL bExclude=TRUE );
	
	void GetStagingHex( CAIUnit *pUnitToStage, 
		CAIUnit *pUnitNearby, CHexCoord& hexDest );

	void GetBuildHex( int iBldg, CHexCoord& hex );
	void GetBridgingHexes( CHexCoord& hexSite, CAIUnit *pUnit );
	void GetRoadHex( CHexCoord& hexSite );
	void GetCraneHex( CHexCoord& hexSite, CHexCoord& hexCrane );

	void Save( CFile *pFile );
	void Load( CFile *pFile, CAIUnitList *plUnits );
};

#endif // __CAIMAP_HPP__
