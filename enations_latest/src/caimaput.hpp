////////////////////////////////////////////////////////////////////////////					 /
//  CAIMapUt.hpp : CAIMapUtil object declaration
//                 The Last Planet - AI
//               
//  Last update:    09/13/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAIUnit.hpp"
#include "CAIHex.hpp"

#ifndef __CAIMAPUT_HPP__
#define __CAIMAPUT_HPP__

//
// this class provides utilities for CAIMap
//
class CAIMapUtil
{
	WORD *m_pMap;		// map for whom this utility serves
	WORD *m_pwaWork; // working version of this map

	int m_iMapSize;

	CAIHex *m_pgrpHexs;	// pointer to work area for group of hexes
	int m_iGrpSize;		// count of hexes in group

	WORD m_wBaseRow;
	WORD m_wBaseCol;
	WORD m_wRows;
	WORD m_wCols;
	WORD m_wEndRow;
	WORD m_wEndCol;
	int m_iPlayer;
	
	int m_iBaseX;	// base hex of the map
	int m_iBaseY;
	int m_iNumVehicles;
	int m_iNumBuildings;
	int m_iBufferSize;
	BOOL m_bAllKnown;

	int m_iBldg;			// variables used by spiral search eval routines
	BOOL m_bVehFactory;
	BOOL m_bCitizen;
	int m_iWidth;
	int m_iHeight;
	CHexCoord m_hexFound;
	WORD m_wTest;

public:

	// store off local copies for speed
	CTransportData const *m_tdWheel; // CTransportData::construction
	CTransportData const *m_tdTrack; // CTransportData::infantry_carrier
	CTransportData const *m_tdHover; // CTransportData::light_scout
	CTransportData const *m_tdFoot; // CTransportData::infantry
	CTransportData const *m_tdShip; // CTransportData::gun_boat

	int m_iCitySX;
	int m_iCitySY;
	int m_iCityEX;
	int m_iCityEY;

						// this is dynamic
	BOOL m_bUseOpFor;	// indicates that an opfor is known and
	CHexCoord m_hexOpFor; // its rocket was/is located here
	CHexCoord m_RocketHex; // the hex where this player starts

	BOOL m_bUnknown;	// indicates if the map this utility
						// belongs to is known or not
	BOOL m_bMinerals;	// indicates minerals can be built on

	BOOL m_bOceanWorld;	// flags from goalmgr influencing type of water
	BOOL m_bLakeWorld;

	CAIUnitList *m_plUnits;	// list of units of this AI player

	CAIMapUtil( WORD *pMap, CAIUnitList *plUnits,
		int iBaseX, int iBaseY,
		WORD wBaseCol, WORD wBaseRow, WORD wCols, WORD wRows, 
		int iPlayer );
	~CAIMapUtil();

	CAIHex *CreatePathPoints( int iSX, int iSY, int iEX, int iEY );

	void FindCentralHex( CAIHex *pBaseHex, 
		int iWidthX, int iWidthY, CHexCoord& hexFound );
	
	void FindSectionHex( int iBldg, 
		int iWidthX, int iWidthY, CHexCoord& hexFound );
	void FindNearestWater( CHexCoord& hcFrom, BOOL bReally=TRUE );
	void FindHexByWater( int iBldg, 
		int iWidthX, int iWidthY, CHexCoord& hexFound );
	void FindHexOnTerrain( int iBldg, 
		int iWidthX, int iWidthY, CHexCoord& hexFound );
	void FindHexOnMaterial( int iBldg, 
		int iWidthX, int iWidthY, CHexCoord& hexFound );
	void FindSpecialHex( int iBldg, 
		int iWidthX, int iWidthY, CHexCoord& hexFound );
	void FindHexInCity( int iBldg, 
		int iWidthX, int iWidthY, CHexCoord& hexFound );
	BOOL IsHexInCity( CHexCoord& hexTest );

	void GetExploreHex( CAIUnit *pUnit, CHexCoord& hexFound,
		/*int& iDirectionFromBase, */CHexCoord& hcNow );

	void GetWaterStagingArea( int iShipType,
		CHexCoord& hcStart, CHexCoord& hcEnd,
		int iWidth, int iHeight );
	BOOL GetStagingAreaNear( CHexCoord& hexNear,
		CHexCoord& hcStart, CHexCoord& hcEnd, 
		int iWidth, int iHeight, int iType = CTransportData::combat );
	int GetStagingCount( CHexCoord& hexStart, 
		int iWidth, int iHeight, int iType );
	void FindInfStagingArea( CHexCoord& hexBldg, 
		CHexCoord& hcFrom, CHexCoord& hcTo );
	void GetStagingArea( CHexCoord& hcStart, CHexCoord& hcEnd,
		int iWidth, int iHeight );
	void FlagStagingArea( BOOL bFlag,
		int iSX, int iSY, int iEX, int iEY );
	void FindStagingHex( int iSX, int iSY, 
		int iEX, int iEY, CHexCoord& hexDest, int iVehType, 
		BOOL bFindPath=FALSE );
	void FindStagingHex( CHexCoord& hexNearBy, 
		int iWidth, int iHeight, int iVehType, 
		CHexCoord& hexDest, BOOL bExclude=TRUE );
	void FindLandingHex(CHexCoord& hexHead);
	BOOL IsHexInArea( CHexCoord& hcStart, CHexCoord& hcEnd, 
		CHexCoord& hex );
	void ExpandAreaWithHex( CHexCoord& hcStart, CHexCoord& hcEnd, 
		CHexCoord& hex );

	void FindRetreatHex( CAIUnit *pFleeUnit, CHexCoord& hexFlee,
		CAIUnit *pAttacker, CHexCoord& hexAttacking );
	void FindApproachHex( CHexCoord& hexTarget, 
		CAIUnit *pUnit, CHexCoord& hexDest );
	void FindDefenseHex( CHexCoord& hexAttacking, 
		CHexCoord& hexDefending, int iWidth, int iHeight, 
		CTransportData const *pVehData );
	CAIHex *FindWaterPatrol( int iGoal );
	CAIHex *FindWaterPatrol( CHexCoord& hexAt, CTransportData const *ptdShip );

	void UpdateAdjacentHexes( CAIHex *paiHex );
	BOOL IsWaterAdjacent( CHexCoord hex, BOOL bReally );
	BOOL IsWaterAdjacent( CHexCoord hexWater, int iWidthX, int iWidthY );
	BOOL IsLandingArea( CHexCoord& hexLC );
	BOOL IsCitizenTerrain( CHexCoord& hcStart, int iWidthX, int iWidthY ) ;
	int GetMineralRating( CHexCoord& hexRate, 
		int iWidthX, int iWidthY, int iMaterial );
	BOOL NearBuildings( CHexCoord& hexRate, int iWidthX, int iWidthY, BOOL bAllowResc );
	
	//BOOL NearMapEdge( CAIHex *paiHex, int iWidthX, int iWidthY );
	//BOOL AreHexesOpen( int iDirection, 
	//	CAIHex *paiHex, int iWidthX, int iWidthY );
	BOOL AreHexesOpen( int iDirection, 
		CHexCoord& hexRate, int iWidthX, int iWidthY );
	

	BOOL HasOpforUnits( CHex *pHex );
	int AssessTarget( CVehicle *pVeh, int iKindOf);
	int AssessTarget( CBuilding *pBldg, int iKindOf);
			
	int IsOutOpforRange( CHexCoord& hex, int iSpotting=0 );
	int GetSupportRange(CHexCoord& hex, CAIUnit *pFleeing);
	
	int GetMultiplier( CHexCoord hexBuild,
		int iWidthX, int iWidthY, int iType );
	//int GetMultiplier( CAIHex *paiHex, 
	//	int iWidthX, int iWidthY, int iType );
	void OffsetToXY( int iOffset, int *piX, int *piY );
	int GetMapOffset( int iX, int iY );

	void AdjustInCityHex( int iBldg, CHexCoord& hexBase );
	int GetEnterableHexes( CHexCoord hexStart, CHexCoord hexEnd );
	BOOL GetClosestWaterHex( CHexCoord& hcFrom, CHexCoord& hcTo );
	BOOL GetPathRating( CHexCoord& hexFrom, CHexCoord& hexTo,
		int iVehType = CTransportData::construction );
	BOOL InBufferSizeRange( CAIHex *paiHex );
	int GetClosestRocket( CHexCoord& hexNear );
	int GetRocketRating( CHexCoord& hex, BOOL bIsCloser=FALSE );
	int GetBufferRating( int iBldg, CHexCoord& hexRate,
		int iWidthX, int iWidthY );
//	int GetBufferRating( int iBldg, CAIHex *paiHex,
//		int iWidthX, int iWidthY );
	
	int GetClosestTo( CHexCoord hexFrom, 
		int iBldg1, int iBldg2, int iBldg3 );
	//int GetClosestTo( CAIHex *paiHex, 
	//	int iBldg1, int iBldg2, int iBldg3 );
	//void RateAdjacentCells( CAIHex *pHexBldg, int iBldgW, int iBldgH, 
	//	int iAdjW, int iAdjH, CAIHex *pBaseHex );
	int RateNearRoad( CAIHex *paiHex, 
		int iRating, int iWidthX, int iWidthY );
	int RateBuilding( int iBldg, CAIHex *paiHex );

	void FindAssaultHex( CHexCoord& hexTarget, int iTerrain );
	void FindBridgeHex( CHexCoord& hexSite, CAIUnit *pUnit );
	BOOL IsBridgeSpan( CHexCoord& hexRiverRoad, CAIUnit *pUnit );
	void GetStartSpan( CHexCoord& hexStart, CHexCoord& hexBridge );

	void FindRoadHex( CHexCoord& hexFound );
	void UpdateCityBounds( int iX, int iY );
	BOOL FindBldgScope( CRect *prScope, int iBldg );
	void FindCityScope( CRect *prScope );
	int FindNth( int iAt );

	BOOL AllKnown( void );
	BOOL IsHexPatrolable( CHexCoord& hex );
	//BOOL IsHexValidToBuild( CAIHex *paiHex );
	BOOL IsHexValidToBuild( CHexCoord& hex );
	void LoadGroupHexes( int isx, int isy, 
		int iWidth, int iHeight );
	void ClearGroupHexes( void );
	void ClearWorkMap( void );

	WORD GetStatus( int iAt );
	WORD ConvertStatus( CAIHex *pHex, WORD wOldStatus );

	void Save( CFile *pFile );
	void Load( CFile *pFile, WORD *pMap, CAIUnitList *plUnits );

#ifdef _LOGOUT
	void ReportPavedRoads( void );

	void ReportGroupHex( int iBldg, int iWidthX,
		int iWidthY, CHexCoord& hex );
#endif
  private:
    // Memoization cache for path results
    struct PathCacheEntry
    {
        CHexCoord hexFrom;
        CHexCoord hexTo;
        int       iVehType;
        BOOL      bResult;
        DWORD     dwTimestamp;
        int       iFailureCount;  // Track repeated failures
    };

    static const int   MAX_CACHE_SIZE    = 1000;
    static const DWORD CACHE_EXPIRE_MS   = 35000;  // Clear the cache because the map changes
    static const int   MAX_FAILURE_COUNT = 2;      // Max failures before temporary ban
    static const DWORD FAILURE_BAN_MS    = 55000;  // just tempban it after repeated failures

    PathCacheEntry m_pathCache[MAX_CACHE_SIZE];
    int            m_iCacheSize;
    DWORD          m_dwLastCacheClear;

    // Methods for cache management
    int  FindCacheEntry( const CHexCoord& hexFrom, const CHexCoord& hexTo, int iVehType );
    void AddCacheEntry( const CHexCoord& hexFrom, const CHexCoord& hexTo, int iVehType, BOOL bResult );
    void ClearExpiredCache( );
    void InvalidatePathCache( );
    BOOL IsPathBanned( const CHexCoord& hexFrom, const CHexCoord& hexTo, int iVehType );

};

#endif // __CAIMAPUT_HPP__

