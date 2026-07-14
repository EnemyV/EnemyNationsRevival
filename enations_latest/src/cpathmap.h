#ifndef __CPATHMAP_H__
#define __CPATHMAP_H__

////////////////////////////////////////////////////////////////////////////
//
//  CPathMap.h :  CPathMap object declaration
//                Divide and Conquer
//               
//  Last update:    10/29/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "cpathmgr.h"

class CPathMap
{
	CRITICAL_SECTION m_cs; // internal use only — guards ALL shared scratch state
	BOOL m_bCsInited;      // m_cs created in ctor, deleted once in dtor/Close

	// these are used only if the array of cells is used
	CCell *m_paCells;	// array version

	// O(1) coord->cell lookup: flat per-hex grids + generation stamps. This
	// replaces the per-search hash map (m_mapCell) the profiler showed as the
	// scratch bottleneck (~1.2M cells/s created at 2-3 hash ops each, plus a
	// RemoveAll + full-arena reinit per search). A grid entry is valid only if
	// its gen stamp equals the current search's; "clearing" between searches
	// is ++m_wSearchGen (O(1)). On WORD wrap (every 65535 searches) the gen
	// grid is memset once. Slots fit a WORD because the arena is (W+H)*4
	// cells (8192 on a 1024 map); Init() clamps if that ever exceeds 0xFFFF.
	WORD *m_pwCellGen;   // per-hex generation stamp     [m_iNumOfMapCells]
	WORD *m_pwCellSlot;  // per-hex slot into m_paCells  [m_iNumOfMapCells]
	WORD m_wSearchGen;   // current search's generation

	void NewSearchGen( void );	// O(1) lookup reset (real clear on wrap)

	int m_iWidth;		// size of MAP in width and height
	int m_iHeight;

	int m_iMapEX;		// either the size of the map (before wrap)
	int m_iMapEY;		// or the last hex of the map (before wrap)

	CHexCoord m_hexFrom;// starting location
	CHexCoord m_hexTo;	// destination requested

	int m_iBestCost;	// goal cost to use for culling
	int m_iDistFactor;	// factor to apply to range for minimum cost
	int m_iMaxDist;		// max distance a path should take (2 x legs)

	BOOL m_bAtDestination;	// a path being tested reached destination
	BOOL m_bPathCompleted;	// the destination has been reached
	BOOL m_bNoDestination;  // destination can't be reached
	BOOL m_bRoadPlanning;	// flag indicates road planning is occuring
	BOOL m_bWarPlanning;	// war mode: rivers passable (planned bridge), bigger iHang
	BOOL m_bOverWater;		// flag indicates a path across water is allowed

	// these are used only if the array of cells is used
	int m_iNumOfCells;	// arena cap for normal searches (also iHang base)
	int m_iCellAlloc;	// actual allocation (2x) - war planning may fill it all
	int m_iNextSlot;	// next open slot in array
	int m_iFirst;		// first slot used in array
	int m_iLast;		// last slot used in array

	int m_iBaseX;		// relative start of map
	int m_iBaseY;
	WORD *m_pMap;		// pointer to array representing map
	int m_iNumOfMapCells; // size of m_pMap

	// an array indexed by m_iBoth. Any value > MAX_BOTH_INDEX not put in
	// does NOT do m_iBoth == 0 (why bother)
	CCell *		m_acBoth [MAX_BOTH_INDEX+1];	// +1 to let us walk past end
	int				m_iLowestBoth;

	CTransportData const *m_pTD; // pointer for this vehicle type
	// store off local copies for speed
	CTransportData const *m_tdWheel; // CTransportData::construction
	CTransportData const *m_tdTrack; // CTransportData::infantry_carrier
	CTransportData const *m_tdHover; // CTransportData::light_scout
	CTransportData const *m_tdFoot; // CTransportData::infantry

public:

	CPathMap( void );
	~CPathMap();
	BOOL Init( int iMapEX, int iMapEY );
        void Close ();
		
	//
	// GetPath() parameter usage is as follows:
	//
	// CHexCoord& hexFrom - current location
	// CHexCoord& hexTo - destination location
	// int iBaseX - base location of map    (must be 0 if pMap=NULL)
	// int iBaseY							(must be 0 if pMap=NULL)
	// WORD *pMap - pointer to word array representing map
	// BOOL bLongHang - must be TRUE to have LONG paths tried
	//
	BOOL GetPath( CHexCoord& hexFrom, CHexCoord& hexTo,
		int iBaseX, int iBaseY, WORD *pMap, int iVehType,
		BOOL bLongHang=FALSE, BOOL bWarPlanning=FALSE );
	BOOL _GetPath( CHexCoord& hexFrom, CHexCoord& hexTo,
		int iBaseX, int iBaseY, WORD *pMap, int iVehType,
		BOOL bLongHang, BOOL bWarPlanning );

	// for use in finding a road path between buildings
	// and with flag bAllowWater set
	// for use in finding a water path with land too
	// and with flag bRiverCrossing set OFF will not allow
	// road to cross river
	// 
	// WORD *pMap - map to use or when null, uses theGame.GetHex only
	//
	CHexCoord *GetRoadPath( 
		CHexCoord& hexFrom, CHexCoord& hexTo, 
		int& iPathLen, WORD *pMap, 
		BOOL bAllowWater=FALSE, BOOL bRiverCrossing=TRUE );
	CHexCoord *_GetRoadPath( 
		CHexCoord& hexFrom, CHexCoord& hexTo, 
		int& iPathLen, WORD *pMap, 
		BOOL bAllowWater, BOOL bRiverCrossing );
	
	BOOL AtDestination( CCell *pCell );
	int GetOffset( int iX, int iY );
	// ROAD AVOIDANCE: TRUE if a neighbor of iX,iY holds an own farm/lumber bldg
	BOOL FarmLumberAdjacent( int iX, int iY );
	void GetCellCosts( CCell *pFromCell, CCell *pToCell );
	void GetCellAt( int iPos, CCell *pFromCell, int& iX, int& iY );

	CCell *GetClosestCell( void );
	void NewBoth ( CCell * pTest );
	CCell *xGetLowestCost( void );
	CCell *GetLowestCost( void );
	CCell *GetCellAt( int iX, int iY );
	void ClearArray( void );
	CCell * AddCellToArray( CCell *pCell );

	CHexCoord *CreateHexPath( int& iPathLen, CCell *pDestCell );
	int GetPathCount( CCell *pDestCell );

	// Diagnostic: cells created by the current/most recent search. (The hash
	// map this used to count is gone; the arena recycles via gen stamps, so
	// there is no longer a per-node container that CAN leak.)
	int GetMapCellCount() const { return m_iNextSlot; }
};

extern CPathMap thePathMap;

#endif // __CPATHMAP_H__
