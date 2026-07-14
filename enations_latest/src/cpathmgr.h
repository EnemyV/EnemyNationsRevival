#ifndef __CPATHMGR_H__
#define __CPATHMGR_H__

////////////////////////////////////////////////////////////////////////////
//
//  CPathMgr.h :  CPathMgr object declaration
//                Divide and Conquer
//               
//  Last update:   10/29/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

// Bucket bound for the path open-list's cost+distance (m_iBoth) index. Cells whose
// m_iBoth exceeds this can't live in the O(1) bucket array and fall back to the O(n)
// linear scan (xGetLowestCost). 600 was sized for 1996 map sizes: on a 1024x1024 map a
// cross-map path's cost+heuristic exceeds it ROUTINELY, so 12-AI assault pathing
// degraded to permanent linear scans (and tripped the "no bucket found" TRAP storm in
// CPathMap::GetLowestCost). 4096 covers 1024-map diagonals with terrain multipliers;
// the array is pointers (32KB/instance) and the per-search clear is a ~3us memset.
const int MAX_BOTH_INDEX = 4096;

#define CELLSAROUND 		8
#define HEADINGS			3
#define USE_HEADINGS		0

#include "stdafx.h"
#include "unit.inl"
#include "terrain.inl"

class CCell
{
public:
	int m_iX;
	int m_iY;
	int m_iCost;		// cost to enter this cell from m_pCellFrom
	int m_iDist;		// distance to destination from this cell
	int m_iBoth;		// combined values m_iCost + m_iDist

	CCell *m_pCellFrom;	// the cell from which we came

	// linked list for CCells on a given hash table index
	CCell *		m_pCellNext;

	// linked list for m_iBoth values
	CCell *		m_pcNextBoth;				// these are NULL at the ends
	CCell *		m_pcPrevBoth;
	int				m_iBothIn;
	BYTE			m_bClosed;	// expanded once; CPathMap::_GetPath never re-opens (boolean searches)

	CCell();
	CCell( int iX, int iY );
};

class CPathMgr
{
    CRITICAL_SECTION m_cs;  // internal use only

	// BUGBUG
	// these are used only if the array of cells is used
	CCell *m_paCells;	// array version

	// this is to find elements in CCell faster (pooled node allocator: fills+clears
	// thousands of nodes per A* search — pooling kills the CRT-heap churn/lock).
	CMap <DWORD, DWORD, CCell *, CCell *, EnPoolAllocator<std::pair<const DWORD, CCell*>>>		m_mapCell;

	int m_iPaths;	// count of all calls
	int m_iOrtho;	// count of x==x or y==y paths
	int m_iHP;		// count of non Vehicle paths

	int m_iPathTicks; // sum of ticks in paths
	int m_iStepCnt;   // count of steps in paths
	int m_iHangTicks; // sum of ticks in hangs
	int m_iHangCnt;   // count of hang steps

	int m_iWidth;		// size of MAP in width and height
	int m_iHeight;

	int m_iMapEX;		// either the size of the map (before wrap)
	int m_iMapEY;		// or the last hex of the map (before wrap)

	CHexCoord m_hexFrom;// starting location
	CHexCoord m_hexTo;	// destination requested

	CHexCoord m_lastFrom;	// to stop repeating path requests
	CHexCoord m_lastTo;

	BOOL m_bVehBlock;	// indicates if vehicles on path block path

	CTransportData const *m_pTD; // pointer for this vehicle type

	int m_iWheel;		// type of wheel for vehicle
	int m_iWater;		// water depth required to travel

	int m_iBestCost;	// goal cost to use for culling
	int m_iDistFactor;	// factor to apply to range for minimum cost
	int m_iMaxDist;		// max distance a path should take (2 x legs)
	int m_iMaxPath;		// the max range allowed per pathfinding attempt

	BOOL m_bAtDestination;	// a path being tested reached destination
	BOOL m_bPathCompleted;	// the destination has been reached
	BOOL m_bNoDestination;  // destination can't be reached
	
	// these are used only if the array of cells is used
	int m_iNumOfCells;	// size of array of cells
	int m_iNextSlot;	// next open slot in array
	int m_iFirst;		// first slot used in array
	int m_iLast;		// last slot used in array

	// an array indexed by m_iBoth. Any value > MAX_BOTH_INDEX not put in
	// does NOT do m_iBoth == 0 (why bother)
	CCell *		m_acBoth [MAX_BOTH_INDEX+1];	// +1 to let us walk past end
	int				m_iLowestBoth;

public:

	CPathMgr( int iMapEX, int iMapEY );
	CPathMgr( void );
	~CPathMgr();
	BOOL Init( int iMapEX, int iMapEY );
        void Close ();

	void		NewBoth ( CCell * pTest );

	//
	// GetPath() parameter usage is as follows:
	//
	// CVehicle *pVehicle - NULL means that iVehType is required
	// CHexCoord& hexFrom - current location
	// CHexCoord& hexTo - destination location
	// int& iPathLen - length of array returned
	// int iVehType - required if pVehicle is NULL, ignored otherwise
	// BOOL bVehBlock - default (FALSE) means that path goes thru
	//                  vehicles, TRUE means vehicles will block
	// BOOL bDirectPath - default (FALSE) means that GetPath() will
	//                    seek an optimum path, TRUE means that it
	//                    will return upon reaching the destination
	//                    on its first attempt
	//
    CHexCoord* GetPath( CVehicle* pVehicle, CHexCoord& hexFrom, CHexCoord& hexTo, int& iPathLen, int iVehType = 0,
                        BOOL bVehBlock = FALSE, BOOL bDirectPath = FALSE );

	CHexCoord *CreateHexPath( int& iPathLen, CCell *pDestCell );
	int GetCellDirection( CHexCoord& fromHex, CHexCoord& toHex );
	void AdjustDestination( void );
	void ChangeDestination( void );

	void GetHeadingCell( int iPos, CCell *pFromCell, int& iX, int& iY );
	void GetFromCell( CVehicle *pVeh, CCell *pFromCell );
	BOOL IsValidHeading( int iPos, CCell *pFromCell );

	// BUGBUG this is to help solve the repeating path problem

	//void ReportPath( CHexCoord *, int );
	//void ReportCounts( void );


	int GetPathCount( CCell *pDestCell );
	BOOL AtDestination( CCell *pCell );
	BOOL CanEnterBridge( CCell *pFromCell, CCell *pToCell );
	void GetCellCosts( int iPos, CCell *pFromCell, CCell *pToCell );
	void GetCellAt( int iPos, CCell *pFromCell, int& iX, int& iY );

	// BUGBUG
	// these are used only if the array of cells is used
	CCell *GetClosestCell( void );
	CCell *GetLowestCost( int& iCnt );
	CCell *xGetLowestCost( int& iCnt );
	CCell *GetCellAt( int iX, int iY );
	void ClearArray( void );
	CCell * AddCellToArray( CCell *pCell );

	// Diagnostic: exact live node count of the per-path CCell scratch map.
	int GetMapCellCount() const { return (int)m_mapCell.GetCount(); }

	private:
    CHexCoord* _GetPath( CVehicle* pVehicle, CHexCoord& hexFrom, CHexCoord& hexTo, int& iPathLen, int iVehType = 0,
                         BOOL bVehBlock = FALSE, BOOL bDirectPath = FALSE );
};

extern CPathMgr thePathMgr;

#endif // __CPATHMGR_H__
