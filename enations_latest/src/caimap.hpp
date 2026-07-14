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

#include "caimsg.hpp"
#include "caimaput.hpp"
#include <vector>			// planned-road index
#include <map>				// denied-bridge banks
#include <set>				// impromptu span marks (discriminator)

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

	// planned-road index helpers (replicate FindRoadHex eligibility)
	BOOL IsRoadHexEligible( int iOff, CHexCoord& hexRoad );
	BOOL IsBridgeCandidateHex( int iOff, CHexCoord& hexRoad );	// frontier-adjacent WATER plan hex = crossing reached
	BOOL NeighborHasRoadOrBldg( CHexCoord& hex );
	// ROAD AVOIDANCE: TRUE if a neighbor of hex holds an own farm/lumber bldg
	BOOL NeighborIsFarmLumber( CHexCoord& hex );

public:
	CAIMapUtil *m_pMapUtil;

	int m_iRoadCount;	// count of MSW_PLANNED_ROAD locations left
	int m_iBridgeSpanFails;	// span-fail bridge attempts (research-nudge signal; transient)
	BOOL m_bPendingBridge;	// a planned crossing awaits a crane (checked by BuildRoad; transient)
	// Exact index of planned-road hexes (offsets into m_pwaMap). Runtime-only,
	// NOT serialized -- rebuilt from the map on load. Replaces the radius spiral
	// in FindRoadHex so a distant crane can't "miss" a plan it has hexes for.
	std::vector<int> m_aPlannedRoad;
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
	void PlanWarRoad( CHexCoord& hexTo );	// road from colony toward an assault staging area
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
		CHexCoord& hexPlace, int iVehType, CHexCoord* phexRocket=NULL );

	void GetStagingHex( CHexCoord& hexNearBy, 
		int iWidth, int iHeight, int iVehType, 
		CHexCoord& hexDest, BOOL bExclude=TRUE );
	
	void GetStagingHex( CAIUnit *pUnitToStage, 
		CAIUnit *pUnitNearby, CHexCoord& hexDest );

	void GetBuildHex( int iBldg, CHexCoord& hex );
	void GetBridgingHexes( CHexCoord& hexSite, CAIUnit *pUnit );
	// bridge-site search over the WHOLE planned-road index (nearest river hex to crane)
	void FindBridgeOnPlan( CHexCoord& hexSite, CAIUnit *pUnit );
	// clamped-crane assist: plan a validated river crossing toward an unreachable site
	BOOL PlanBridgeToward( CHexCoord const& hexAt, CHexCoord const& hexSite );
	// dispatch-time claim: crane sent to this crossing - scanners skip the
	// bank until the claim expires (3 min) or the server accept marks the span
	void ClaimBridge( CHexCoord const& hexStart, CHexCoord const& hexEnd );
	// server rejected the span: unmark its planned hexes + deny the bank 30 min
	void DenyBridge( CHexCoord const& hexStart, CHexCoord const& hexEnd );
	std::map<int, DWORD> m_mBridgeDeny;	// bank map-offset -> denied-until ms (transient)
	// offsets whose MSW_PLANNED_ROAD mark came from PlanBridgeToward (impromptu),
	// NOT the road planner - the two were indistinguishable, which broke every
	// planner-vs-impromptu measurement and let impromptu marks masquerade as
	// planner crossings. Runtime-only (rebuilt per game), like m_mBridgeDeny.
	std::set<int> m_setImpromptuSpan;
	// phexBridgeCand (optional): nearest frontier-adjacent WATER plan hex - the
	// "plan frontier reached a river" event the pre-index radius spiral used to
	// surface via local exhaustion (regressed when the pick went global)
	void GetRoadHex( CHexCoord& hexSite, CHexCoord* phexBridgeCand = NULL );
	// planned-road index: nearest eligible planned hex to hexCrane -> hexOut
	BOOL GetPlannedRoadNear( CHexCoord& hexCrane, CHexCoord& hexOut, CHexCoord* phexBridgeCand = NULL );
	int  GetPlannedCount( void );		// live index entries (prunes stale)
	void RebuildPlannedIndex( void );	// rescan m_pwaMap after load
	// batch road: extend a picked road hex into a straight cardinal run of
	// contiguous planned-road hexes (marks them ROAD); returns hex count.
	int GetRoadRun( const CHexCoord& hexStart, CHexCoord& hexEnd, int iMaxHexes );
	void GetCraneHex( CHexCoord& hexSite, CHexCoord& hexCrane );

	void Save( CArchive& ar );
    void Load( CArchive& ar, CAIUnitList* plUnits );
};

#endif // __CAIMAP_HPP__
