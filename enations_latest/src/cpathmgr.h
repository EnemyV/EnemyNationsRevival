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
#include "enprobes.h"  // EN_PATH_PROBES compile gate (shadow-instance members below)

// The navigation read seam (ennavview.h). Every live-world read a search makes goes
// through a view; the search body below is a template over the view type, so
// CEnLiveNavView and CEnSnapNavView each get their own inlined copy and neither
// pays an indirect call in the innermost A* loop. Both instantiations are used
// inside cpathmgr.cpp, so no explicit instantiation is needed here.

class PathWorld;   // SearchSnapshot's world, taken by reference only (pathworld.h)

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
    // m_cs lifetime, tracked explicitly instead of being inferred from m_paCells.
    // Close() frees the arena but deliberately LEAVES m_cs live, so "m_paCells != NULL"
    // never was the same question as "m_cs is initialised": after a Close()+Init() pair
    // the old section got re-Initialize()d with no matching Delete, and ~CPathMgr after
    // a Close() deleted nothing at all. Invisible for one process-lifetime global,
    // wrong for any second instance that is created and destroyed per game.
    BOOL m_bCsInit;

    // TRUE on a private instance that re-runs a search purely to be compared with
    // the production answer. Nothing in this class may ask "am I thePathMgr?" - it
    // asks this instead. Two things depend on it: the endpoint-repeat probe ring in
    // GetPath() (one function-level static, so it must only ever see the production
    // instance's requests) and every mpath.* emission inside _GetPath(), which is
    // renamed to mpath.shadow.in.* so no production counter can move. Always FALSE
    // for thePathMgr, and there is no path that sets it except MarkShadow().
    BOOL m_bShadow;
#if EN_PATH_PROBES
    int  m_iProbeOutcome;   // PROBE_OUTCOME of the last _GetPath() on this instance
    // The two cap forms are DIFFERENT terminations and must not be merged: the arena
    // filling means the cell budget was too small for this map, the iteration budget
    // running out means the search was too long. A shadow that hits one where
    // production hit the other has diverged even if both routes clamp to the same hex.
    BOOL m_bProbeCapArena;  // AddCellToArray failed - out of CCell arena
    BOOL m_bProbeCapIter;   // iHang reached 0 - out of iteration budget
#endif

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

	// A CRITICAL_SECTION must never be copied and the CCell arena is owned, so a
	// copied CPathMgr would alias one arena and duplicate one lock handle. Nothing
	// copies one today; make it a compile error before there is a second instance
	// around to be copied by accident.
	CPathMgr( CPathMgr const & ) = delete;
	CPathMgr & operator = ( CPathMgr const & ) = delete;

	// Mark this instance as a comparison-only shadow. One-way, called once right
	// after construction; thePathMgr never calls it.
	void MarkShadow( void ) { m_bShadow = TRUE; }
	BOOL IsShadow( void ) const { return m_bShadow; }

#if EN_PATH_PROBES
	// Exit class of the last _GetPath() on this instance, recorded at exactly the
	// points the mpath.* exit counters are emitted. A comparison needs the class of
	// ONE call; the counters are process-global and reset per perf interval, so they
	// cannot answer that.
	enum PROBE_OUTCOME
	{
		po_none = 0,  // did not run, or exited somewhere no counter classifies
		po_trivial,   // rejected before searching (off-map, or already there)
		po_ok,        // path returned and it reaches the requested destination
		po_clamped,   // path returned, but only as far as the closest reachable cell
		po_nopath,    // NULL returned
		po_blocked    // destination hex cannot be entered
	};
	// Everything a shadow comparison needs about ONE call. Filled inside the search's
	// own critical section, because the members it is copied from are per-instance
	// scratch that the next caller (an AI worker, on the production instance)
	// overwrites the moment the lock is released.
	struct VERDICT
	{
		int       iClass;      // PROBE_OUTCOME
		BOOL      bCapArena;   // arena exhausted
		BOOL      bCapIter;    // iteration budget exhausted
		int       iPathLen;    // length AS REPORTED to the caller, not normalised
		BOOL      bHaveClamp;  // a vehicle was passed, so hexClamp is meaningful
		CHexCoord hexClamp;    // that vehicle's m_hexLastClamp after the search
		VERDICT( ) : iClass( 0 ), bCapArena( FALSE ), bCapIter( FALSE ), iPathLen( 0 ),
		             bHaveClamp( FALSE ) { }
	};
#endif
	BOOL Init( int iMapEX, int iMapEY );
        void Close ();

	// Everything one snapshot search hands back. The class and the two cap flags are
	// probe-only state, so they stay 0/FALSE with EN_PATH_PROBES compiled out.
	struct SNAPSEARCH
	{
		CHexCoord * phexPath;   // new[]ed by the search; the caller owns it
		int         iPathLen;
		int         iClass;     // PROBE_OUTCOME
		BOOL        bCapArena;
		BOOL        bCapIter;
		SNAPSEARCH ( ) : phexPath ( NULL ), iPathLen ( 0 ), iClass ( 0 ), bCapArena ( FALSE ),
		                 bCapIter ( FALSE ) { }
	};

	// The ONLY entry a worker thread uses. It reads the PathWorld it is handed and
	// nothing else: pVehicle is NULL, so no live CVehicle/CHex/CBuilding/CBridgeUnit is
	// touched, and the global `cs` is never taken. Runs under THIS instance's own m_cs,
	// which a worker's private instance never shares with anyone.
	void SearchSnapshot ( PathWorld const & pw, CHexCoord hexFrom, CHexCoord hexTo, int iVehType,
	                      BOOL bVehBlock, BOOL bDirectPath, SNAPSEARCH & out );

	// Step D verification, main thread, probes-gated and inert unless
	// EN_PATH_ASYNC_VERIFY names a value: run the same request synchronously against
	// the LIVE world on the private shadow instance and diff it against the route
	// being installed. Counts pa.verify.cmp / pa.verify.diff / pa.verify.diff.stale.
	static void AsyncVerify ( CVehicle * pVehicle, CHexCoord const & hexFrom, CHexCoord const & hexTo,
	                          BOOL bVehBlock, CHexCoord const * phexInstall, int iInstallLen, BOOL bStale );

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

	template <class TView> CHexCoord *CreateHexPath( TView const & view, int& iPathLen, CCell *pDestCell );
	int GetCellDirection( CHexCoord& fromHex, CHexCoord& toHex );
	template <class TView> void AdjustDestination( TView const & view );
	template <class TView> void ChangeDestination( TView const & view );

	void GetHeadingCell( int iPos, CCell *pFromCell, int& iX, int& iY );
	void GetFromCell( CVehicle *pVeh, CCell *pFromCell );
	BOOL IsValidHeading( int iPos, CCell *pFromCell );

	// BUGBUG this is to help solve the repeating path problem

	//void ReportPath( CHexCoord *, int );
	//void ReportCounts( void );


	int GetPathCount( CCell *pDestCell );
	BOOL AtDestination( CCell *pCell );
	template <class TView> BOOL CanEnterBridge( TView const & view, CCell *pFromCell, CCell *pToCell );
	template <class TView> void GetCellCosts( TView const & view, int iPos, CCell *pFromCell, CCell *pToCell );
	template <class TView> void GetCellAt( TView const & view, int iPos, CCell *pFromCell, int& iX, int& iY );

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
    // ONE search body, instantiated per view type. The view is passed in rather than
    // constructed here: a snapshot search must read the snapshot it was handed.
    template <class TView>
    CHexCoord* _GetPath( TView const& view, CVehicle* pVehicle, CHexCoord& hexFrom, CHexCoord& hexTo, int& iPathLen,
                         int iVehType = 0, BOOL bVehBlock = FALSE, BOOL bDirectPath = FALSE );

#if EN_PATH_PROBES
    // GetPathProd() is the body GetPath() has always had, verbatim, including its
    // mpath.us scope timer - so a shadow search started after it returns lands
    // outside EVERY production timing counter, not just outside the exit counters.
    // With EN_PATH_PROBES compiled out this split does not exist: the body is
    // CPathMgr::GetPath again (see EN_PM_PROD_ENTRY in cpathmgr.cpp).
    CHexCoord* GetPathProd  ( CVehicle* pVehicle, CHexCoord& hexFrom, CHexCoord& hexTo, int& iPathLen, int iVehType,
                              BOOL bVehBlock, BOOL bDirectPath );
    CHexCoord* ShadowGetPath( CVehicle* pVehicle, CHexCoord& hexFrom, CHexCoord& hexTo, int& iPathLen, int iVehType,
                              BOOL bVehBlock, BOOL bDirectPath );
#endif
};

extern CPathMgr thePathMgr;

#if EN_PATH_PROBES
// Ladder step A: ONE private CPathMgr that re-runs each main-thread movement search
// behind the production one and compares the two answers. Measurement only - it is
// constructed only when EN_PATH_SHADOW is set in the environment, it never feeds the
// game, nothing reads its route, and every counter it emits is namespaced. Init and
// Close mirror thePathMgr's, at the same call sites.
// EN_PATH_SHADOW must carry a VALUE: 1 / true / on / yes (any case) enable the
// shadow. Absent, empty, 0, false, off, no - or anything unrecognised - leave it
// off, so the obvious way to switch it off really does. Resolved once, then cached,
// and the effective setting is logged at Init whenever the variable was set at all.
BOOL EnPathShadowOn  ( void );
void EnPathShadowInit( int iMapEX, int iMapEY );
void EnPathShadowClose( void );

// Ladder step C, the measurement half. The worker's answer is compared against the
// SAME-SNAPSHOT reference the shadow already computed for that call - never against
// the live-world answer, which would measure snapshot staleness instead of the
// worker. The reference route is kept here, keyed by requestId, until its result
// comes back. Main thread only.
void EnPathWorkerDrain    ( void );   // at the publication point, before PublishTick
void EnPathWorkerRefsClear( void );   // world teardown: free every stored reference
#endif

#endif // __CPATHMGR_H__
