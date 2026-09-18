////////////////////////////////////////////////////////////////////////////
//
//  CPathMgr.cpp : CPathMgr object implementation
//                 Divide and Conquer
//
//  Last update:   10/29/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "cpathmgr.h"

#include "stdafx.h"
#include "logging.h"  // dave's logging system
#include "Perf.h"     // EN_PERF counters (mpath.* exit mix)
#include "ennavview.h" // the navigation read seam: every live-world read below goes through it
#include "enprobes.h" // EN_PATH_PROBES compile gate

#if EN_PATH_PROBES
#include "pathservice.h"  // step C: the worker pool the shadow's snapshot answer is compared against
#include <stdarg.h>   // shadow diff log
#include <stdlib.h>   // getenv (EN_PATH_SHADOW)
#include <string.h>   // strcmp (EN_PATH_SHADOW value parse)
#include <ctype.h>    // tolower (same)
#include <map>        // outstanding reference routes, keyed by requestId
#ifdef _WIN32
#include <share.h>    // _SH_DENYNO: the log must stay readable while the game runs
#endif
#endif

//#define TEST_RESULT1		// test GetAt improvement
//#define TEST_RESULT2			// test GetLowest improvement
//#define TEST_RESULT3			// anal level of testing

// BUGBUG these are here for diagnostics only
#include "caidata.hpp"
#include "caihex.hpp"

extern CAIData* pGameData;  // pointer to API object for game data


CPathMgr thePathMgr;

#if EN_PATH_PROBES
// Counter-name router for in-search probes. A shadow instance runs the SAME _GetPath
// over the same inputs, so every mpath.* emission inside it would silently double the
// production exit mix. m_bShadow is FALSE on thePathMgr, so on the production instance
// this expands to the identical string literal the tree emitted before.
// Both arms must be literals: Perf interns counter names, and a runtime-built buffer
// would burn one of the 512 name slots (and print garbage) on every call.
// The prefix is mpath.shadow.in.* - deliberately NOT mpath.shadow.*, which is the
// namespace the A/B COMPARISON counters live in (mpath.shadow.ok is "production said
// ok on a compared call", mpath.shadow.in.ok is "the shadow search itself said ok").
#define MPATH_INC( suffix ) Perf::CounterInc( m_bShadow ? "mpath.shadow.in." suffix : "mpath." suffix )
#endif

#define new DEBUG_NEW
#define MAX_PATH_RANGE 80

// lookup table of bit values that represent headings
// that are valid for a given heading (offset to table)
// used by BOOL CPathMgr::IsValidHeading(

// this table allows the current heading of the vehicle
// plus/minus 1 to each side
static unsigned char ucHeadings[] = {
    131,  // 10000011
    7,    // 00000111
    14,   // 00001110
    28,   // 00011100
    56,   // 00111000
    112,  // 01110000
    224,  // 11100000
    193   // 11000001
};

/*
// this table allows the current heading of the vehicle
// plus/minus 2 to each side
static unsigned char ucHeadings[] = {
    199, // 11000111
    143, // 10001111
    31,  // 00011111
    62,  // 00111110
    124, // 01111100
    248, // 11111000
    241, // 11110001
    227  // 11100011
};
*/

#if EN_PATH_PROBES
// With probes compiled in, GetPath() is the selector below and the production body
// becomes GetPathProd(). With probes compiled out the macro names that body
// CPathMgr::GetPath, so a probe-less build gets the ORIGINAL single function - no
// extra frame, no extra branch, nothing to reason about on the movement path.
#define EN_PM_PROD_ENTRY GetPathProd

// Where GetPathProd() drops the verdict for a shadow comparison, or NULL for an
// ordinary call. THREAD-LOCAL on purpose: the production instance is shared with the
// AI worker threads, and only the thread that armed this pointer may fill it.
static thread_local CPathMgr::VERDICT* t_pPathVerdict = NULL;

CHexCoord* CPathMgr::GetPath( CVehicle* pVehicle, CHexCoord& hexFrom, CHexCoord& hexTo, int& iPathLen, int iVehType,
                              BOOL bVehBlock, BOOL bDirectPath )
{
    // Shadow comparison runs on the PRODUCTION instance only (!m_bShadow), on the
    // MAIN thread only (the AI worker threads share this same object and nothing in
    // step A is thread-safe beyond that), and only when EN_PATH_SHADOW is set.
    // Default off: with the variable unset this is one already-cached int test.
    if ( !m_bShadow && EnPathShadowOn( ) && Perf::IsMainThread( ) )
        return ( ShadowGetPath( pVehicle, hexFrom, hexTo, iPathLen, iVehType, bVehBlock, bDirectPath ) );

    // Step C submits only from inside ShadowGetPath, because the reference a worker
    // result is diffed against IS the shadow's same-snapshot answer. With the shadow
    // off there is no such reference, and diffing against the live answer would
    // measure snapshot staleness rather than the worker. Say so out loud instead of
    // letting a zero comparison count read as a comparison that passed.
    if ( !m_bShadow && pVehicle != NULL && thePathService.IsRunning( ) )
        Perf::CounterInc( "pq.nosubmit" );

    return ( GetPathProd( pVehicle, hexFrom, hexTo, iPathLen, iVehType, bVehBlock, bDirectPath ) );
}
#else
#define EN_PM_PROD_ENTRY GetPath
#endif

CHexCoord* CPathMgr::EN_PM_PROD_ENTRY( CVehicle* pVehicle, CHexCoord& hexFrom, CHexCoord& hexTo, int& iPathLen,
                                       int iVehType, BOOL bVehBlock, BOOL bDirectPath )
{
#if EN_PATH_PROBES
    // movement-A* twin of the CPathMap path.* trio (cpathmap.cpp GetPath).
    // mpath.us includes lock wait, so contention shows up here too.
    Perf::ScopeCounter _t( "mpath.us" );
#endif
    // CONTENTION SPLIT (see the same block in cpathmap.cpp): mpath.us above lumps
    // wait and work together, which is exactly the ambiguity being resolved - a
    // MAIN-thread wait on m_cs is a frame stall, an AI-worker wait is not.
    const uint64_t _qWait = Perf::NowIfEnabled( );
    // audit (4): short-circuit so nothing runs with EN_PERF unset.
    const bool     _qMain = Perf::IsEnabled( ) && Perf::IsMainThread( );
    Perf::CounterInc( _qMain ? "mpath.calls.main" : "mpath.calls.ai" );
    EnterCriticalSection( &m_cs );
    Perf::CounterAddElapsedUs( _qMain ? "mpath.wait.main.us" : "mpath.wait.ai.us", _qWait );
    const uint64_t _qWork = Perf::NowIfEnabled( );

    // CACHE-FEASIBILITY PROBE, counting only - no behaviour change, nothing is
    // reused. Type 58 is closed: the fix is main-thread search COST, and the three
    // candidates are budget-per-frame, cache/reuse paths, or move off-thread.
    // This sizes the middle one BEFORE anyone builds it: how often does a
    // main-thread search repeat a (from -> to) pair seen recently? A ring of the
    // last 512 keys, scanned linearly - ~130 searches/s makes that free, and it is
    // main-thread only so the static ring needs no lock. Inert unless EN_PERF is set.
    // !m_bShadow: the ring is ONE function-level static shared by every instance, so a
    // second CPathMgr coming through here would interleave its keys with production's
    // and corrupt the hit-distance histogram. (Today the shadow calls _GetPath directly
    // and never reaches this line - the guard is what makes that not load-bearing.)
    if ( _qMain && Perf::IsEnabled( ) && !m_bShadow )
    {
        static uint64_t s_ring[512] = { 0 };
        static int      s_next      = 0;
        const uint64_t  key = ( (uint64_t)(uint16_t)hexFrom.X( ) )
                            | ( (uint64_t)(uint16_t)hexFrom.Y( ) << 16 )
                            | ( (uint64_t)(uint16_t)hexTo.X( )   << 32 )
                            | ( (uint64_t)(uint16_t)hexTo.Y( )   << 48 );
        int hitAt = -1;
        for ( int i = 0; i < 512; ++i )
        {
            const int idx = ( s_next - 1 - i + 1024 ) % 512;   // most recent first
            if ( s_ring[idx] == key ) { hitAt = i; break; }
        }
        if ( hitAt < 0 )        Perf::CounterInc( "mpath.cache.miss" );
        else if ( hitAt < 16 )  Perf::CounterInc( "mpath.cache.hit16" );
        else if ( hitAt < 64 )  Perf::CounterInc( "mpath.cache.hit64" );
        else                    Perf::CounterInc( "mpath.cache.hit512" );
        s_ring[s_next] = key;
        s_next = ( s_next + 1 ) % 512;
    }
#if EN_PATH_PROBES
    m_iNextSlot = 0;  // trivial rejects skip the in-search reset; don't re-count
#endif
    const CEnLiveNavView _view;   // one live view for this whole search
    CHexCoord* phcPath = ( _GetPath( _view, pVehicle, hexFrom, hexTo, iPathLen, iVehType, bVehBlock, bDirectPath ) );
#if EN_PATH_PROBES
    Perf::CounterInc( "mpath.calls" );
    Perf::CounterAdd( "mpath.nodes", m_iNextSlot );  // cells created this search
#endif
    if ( _qMain ) Perf::NoteFrameSearch( Perf::ElapsedUs( _qWork ) );
    Perf::CounterAddElapsedUs( _qMain ? "mpath.work.main.us" : "mpath.work.ai.us", _qWork );
#if EN_PATH_PROBES
    // Hand the verdict over while the lock is still held. m_iProbeOutcome and the cap
    // flags are per-instance scratch: an AI worker waiting on m_cs overwrites them as
    // soon as this Leave returns, so reading them afterwards is a race. t_pPathVerdict
    // is thread-local, so that worker's own pass through here sees NULL and writes
    // nothing.
    if ( t_pPathVerdict != NULL )
    {
        t_pPathVerdict->iClass     = m_iProbeOutcome;
        t_pPathVerdict->bCapArena  = m_bProbeCapArena;
        t_pPathVerdict->bCapIter   = m_bProbeCapIter;
        t_pPathVerdict->iPathLen   = iPathLen;
        t_pPathVerdict->bHaveClamp = ( pVehicle != NULL );
        if ( pVehicle != NULL )
            t_pPathVerdict->hexClamp = pVehicle->m_hexLastClamp;
        t_pPathVerdict = NULL;   // one call, one verdict
    }
#endif
    LeaveCriticalSection( &m_cs );
    return phcPath;
}
    //
// return the path via a CHexCoord array, passing the size
// back as the m_iX element of the first CHexCoord
//
template <class TView>
CHexCoord* CPathMgr::_GetPath( TView const& view, CVehicle* pVehicle, CHexCoord& hexFrom, CHexCoord& hexTo,
                              int& iPathLen, int iVehType, BOOL bVehBlock, BOOL bDirectPath )
{
#if PATH_TIMING
#ifdef _LOGOUT
    DWORD dwStart, dwEnd;
    dwStart = timeGetTime( );
#endif
#endif

#if EN_PATH_PROBES
    m_iProbeOutcome  = po_none;   // per-call, re-derived below at every exit
    m_bProbeCapArena = FALSE;
    m_bProbeCapIter  = FALSE;
#endif

    // BUGBUG count types of calls
    m_iPaths++;
    if ( pVehicle == NULL )
        m_iHP++;
    if ( hexFrom.X( ) == hexTo.X( ) || hexFrom.Y( ) == hexTo.Y( ) )
        m_iOrtho++;

    // BUGBUG initialization of iPathLen should be done outside
    // by the calling function of GetPath()
    iPathLen = 0;

    // prevent invalid CHexCoords from continuing
    if ( hexFrom.X( ) < 0 || hexFrom.X( ) > m_iMapEX || hexFrom.Y( ) < 0 || hexFrom.Y( ) > m_iMapEY || hexTo.X( ) < 0 ||
         hexTo.X( ) > m_iMapEX || hexTo.Y( ) < 0 || hexTo.Y( ) > m_iMapEY )
    {
#if PATH_TIMING
#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "\nFindPath from %d,%d to %d,%d has invalid CHexCoord \n",
                   hexFrom.X( ), hexFrom.Y( ), hexTo.X( ), hexTo.Y( ) );
#endif
#endif
#if EN_PATH_PROBES
        MPATH_INC( "trivial" );
        m_iProbeOutcome = po_trivial;
#endif
        return ( NULL );
    }

    // set best cost threshhold so its very high
    // and it must match what is tested in GetLowestCost()
    m_iBestCost = 0xFFFE;  //(m_iWidth * m_iHeight);

    // change to reflect new approach
    m_iFirst      = 0;
    m_iLast       = 0;
    m_iNextSlot   = 0;
    m_iLowestBoth = 0;

    // no path from start to destination because we are there
    if ( hexFrom == hexTo )
    {
#if EN_PATH_PROBES
        MPATH_INC( "trivial" );
        m_iProbeOutcome = po_trivial;
#endif
        return ( NULL );
    }

    m_hexFrom   = hexFrom;
    m_hexTo     = hexTo;
    m_bVehBlock = bVehBlock;
    // force vehicles to block at all times, to reduce jams
    //
    // no, because the client wants to ignore my advice, again,
    // and thus, let the jams begin ....
    // m_bVehBlock = TRUE;

    // get CTransportData pointer for the unit moving
    if ( pVehicle != NULL )
        m_pTD = pVehicle->GetData( );
    else
        m_pTD = view.GetTransportData( iVehType );

    // determine maximum cost of a single move based on
    // this wheel type and use that as a factor with distance
    // to destination from current test cell.
    m_iDistFactor = m_iBestCost;
    // m_iDistFactor = 0;

    for ( int t = CHex::city; t < CHex::num_types; ++t )
    {
        int iRtn = view.GetWheelMult( t, m_pTD->GetWheelType( ) );
        // x2 (1996 original). Do NOT soften to x1.5 (reverted #27): at the shipping
        // search budget both weights skip detour-roads identically, and x1.5 only
        // explores more cells. Road-following is a budget/cost fix, not a heuristic one.
        // iRtn *= 2;
        iRtn <<= 1;

        if ( iRtn && iRtn < m_iDistFactor )
            m_iDistFactor = iRtn;
    }

    // compensate for lack of differences in terrain costs over distance
    if ( m_iDistFactor == m_iBestCost )
        m_iDistFactor = 2;
    // m_iDistFactor >>= 1;

    // determine the maximum distance a path should take
    m_iMaxDist = ( abs( hexFrom.X( ) - hexTo.X( ) ) + abs( hexFrom.Y( ) - hexTo.Y( ) ) ) * m_iDistFactor;

#if PATH_TIMING
#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "\nFindPath from %d,%d to %d,%d wheeltype=%d ", m_hexFrom.X( ),
               m_hexFrom.Y( ), m_hexTo.X( ), m_hexTo.Y( ), m_pTD->GetWheelType( ) );
    if ( pVehicle != NULL )
        logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "for vehicle id %ld of player %d ", pVehicle->GetID( ),
                   pVehicle->GetOwner( )->GetPlyrNum( ) );
#endif
#endif

    // consider the range to the destination and make adjustment
    // if range is > m_iMaxPath
    if ( !bDirectPath )
        AdjustDestination( view );


    // set up hexFrom as first test cell
    CCell* pLastTest = NULL;
    CCell* pTest     = NULL;
    CCell* pDestCell = NULL;
    CCell  ccTest( m_hexFrom.X( ), m_hexFrom.Y( ) );

    // set special state for hexFrom
    ccTest.m_iCost = 0;
    ccTest.m_iDist = 0;
    ccTest.m_iBoth = 0;

    // used for array
    pTest = AddCellToArray( &ccTest );
#ifdef TEST_RESULT2
    TRAP( pTest != GetCellAt( ccTest.m_iX, ccTest.m_iY ) );
#endif
    CCell* pAdjCell = NULL;

    // if a vehicle exists, then determine its current heading
    // and record the appropriate cell in pTest->m_pFromCell
    int iAdjCells = 0;
#if USE_HEADINGS
    if ( pVehicle != NULL && !( m_pTD->GetVehFlags( ) & CTransportData::FL1hex ) )
    {
        GetFromCell( pVehicle, pTest );
        iAdjCells = HEADINGS;
    }
    else
#endif
        iAdjCells = CELLSAROUND;


    int iX, iY, iCnt;

    // BUGBUG hard code limit on pathing
    int iHang = ( m_iWidth + m_iHeight );  // 128
    if ( bDirectPath )
        iHang *= 2;  // 256
    else
        iHang += ( view.GetRangeDistance( m_hexFrom, m_hexTo ) * CELLSAROUND );
    iHang = ( iHang * 3 ) / 2;  // +50% search headroom (pairs with the *5 arena above)

#if EN_PATH_PROBES
    BOOL bProbeArenaFull = FALSE;  // arena exhaustion forces the iHang exit; keep the two counters distinct
#endif

    int iTicks = 0;
    int iList  = 1;

    // start looping to find a path
    while ( TRUE )
    {
        if ( AtDestination( pTest ) && pTest->m_iBoth == m_iBestCost )
        {
            break;
        }

        // for each adjacent cell to the test cell
        for ( int i = 0; i < iAdjCells; ++i )
        {
            // enact a heading criteria
            // if( !IsValidHeading(i,pTest) )
            //	continue;

            // get the adjacent cell x,y values
            // in the 'i' direction from pTest
#if USE_HEADINGS
            if ( iAdjCells == HEADINGS )
                GetHeadingCell( i, pTest, iX, iY );
            else
#endif
                GetCellAt( view, i, pTest, iX, iY );

            // consider if that cell is already in list
            pAdjCell = GetCellAt( iX, iY );

            if ( pAdjCell == NULL )
            {
                CCell aAdjCell( iX, iY );
                pAdjCell = AddCellToArray( &aAdjCell );
#ifdef TEST_RESULT2
                TRAP( pAdjCell != GetCellAt( iX, iY ) );
#endif

                iList++;
            }

            // the AddCellToArray() could fail if array has been exceeded
            if ( pAdjCell == NULL )
            {
#if EN_PATH_PROBES
                MPATH_INC( "arena_full" );
                bProbeArenaFull = TRUE;
#endif
                iHang = 1;  // cause early termination
                break;
            }

            // now get cost to enter pAdjCell from pTest,
            // and distance from pAdjCell to destination,
            // and make pAdjCell point to pTest
            GetCellCosts( view, i, pTest, pAdjCell );

            if ( AtDestination( pAdjCell ) )
            {
                pDestCell = pAdjCell;
#if PATH_TIMING
#ifdef _LOGOUT
                logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "at dest with pAdjCell %d,%d  cost=%d  dist=%d  best=%d",
                           pAdjCell->m_iX, pAdjCell->m_iY, pAdjCell->m_iCost, pAdjCell->m_iDist, m_iBestCost );
#endif
#endif
                if ( ( pAdjCell->m_iCost + pAdjCell->m_iDist ) < m_iBestCost )
                {
                    if ( ( m_iBestCost == 0xFFFE ) && ( ( pAdjCell->m_iCost + pAdjCell->m_iDist ) != 0xFFFE ) )
                    {
                        m_iBestCost  = ( pAdjCell->m_iCost + pAdjCell->m_iDist );
                        int    iEnd  = __min( m_iNextSlot, m_iNumOfCells );
                        CCell* pCell = m_paCells;
                        while ( iEnd-- ) NewBoth( pCell++ );
                    }
                    else
                        m_iBestCost = ( pAdjCell->m_iCost + pAdjCell->m_iDist );

                    // consider only a direct path is needed
                    // BUGBUG force direct paths for a while
                    if ( bDirectPath )
                    {
                        CHexCoord* phexPath = CreateHexPath( view, iPathLen, pAdjCell );

                        ClearArray( );
#if PATH_TIMING
                        dwEnd = timeGetTime( );
#ifdef _LOGOUT
                        logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH,
                                   "GetPath() direct for a %s took %ld ticks for %d steps ",
                                   m_pTD->GetDesc( ).c_str(), ( dwEnd - dwStart ), iPathLen );
                        logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "Took %d interations with %d cells in list \n", iTicks,
                                   iList );
                        if ( phexPath == NULL )
                            logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "GetPath() direct NULL path returned\n" );
#endif
                        m_iPathTicks += (int)( dwEnd - dwStart );  // sum of ticks in paths
                        m_iStepCnt += iPathLen;                    // count of steps in paths
#endif

#if EN_PATH_PROBES
                        if ( phexPath != NULL ) { MPATH_INC( "ok" );     m_iProbeOutcome = po_ok; }
                        else                    { MPATH_INC( "nopath" ); m_iProbeOutcome = po_nopath; }
#endif
                        return ( phexPath );
                    }
                    else
                    {
                        // less than 1/2 the time remaining to search
                        if ( iHang < m_iWidth )
                        {
                            CHexCoord* phexPath = CreateHexPath( view, iPathLen, pAdjCell );

                            // ReportPath(phexPath,iPathLen);

                            ClearArray( );
#if PATH_TIMING
                            dwEnd = timeGetTime( );
#ifdef _LOGOUT
                            logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH,
                                       "GetPath() early end for a %s took %ld ticks for %d steps ",
                                       m_pTD->GetDesc( ).c_str(), ( dwEnd - dwStart ), iPathLen );
                            logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "Took %d interations with %d cells in list \n",
                                       iTicks, iList );
                            if ( phexPath == NULL )
                                logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "GetPath() early end NULL path returned\n" );
#endif
                            m_iPathTicks += (int)( dwEnd - dwStart );  // sum of ticks in paths
                            m_iStepCnt += iPathLen;                    // count of steps in paths
#endif

#if EN_PATH_PROBES
                            if ( phexPath != NULL ) { MPATH_INC( "ok" );     m_iProbeOutcome = po_ok; }
                            else                    { MPATH_INC( "nopath" ); m_iProbeOutcome = po_nopath; }
#endif
                            return ( phexPath );
                        }
                    }
                }
                else  // cost is not the best
                {
                    // pAdjCell could be being entered from a bridge
                    if ( !CanEnterBridge( view, pTest, pAdjCell ) )
                        continue;

                    // no entry possible into destination cell
                    if ( pAdjCell->m_iDist == 0xFFFE )
                    {
                        // return path based on last reachable cell
                        CHexCoord* phexPath = CreateHexPath( view, iPathLen, pTest );

#if PATH_TIMING
                        m_hexTo.X( pTest->m_iX );
                        m_hexTo.Y( pTest->m_iY );
#ifdef _LOGOUT
                        ReportPath( phexPath, iPathLen );
#endif
#endif  // PATH_TIMING

                        ClearArray( );
#if PATH_TIMING
                        dwEnd = timeGetTime( );
#ifdef _LOGOUT
                        logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH,
                                   "GetPath() blocked for a %s took %ld ticks for %d steps ",
                                   m_pTD->GetDesc( ).c_str(), ( dwEnd - dwStart ), iPathLen );
                        logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "returning path to %d,%d instead of destination ",
                                   m_hexTo.X( ), m_hexTo.Y( ) );
                        logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "Took %d interations with %d cells in list \n", iTicks,
                                   iList );
#endif
                        m_iPathTicks += (int)( dwEnd - dwStart );  // sum of ticks in paths
                        m_iStepCnt += iPathLen;                    // count of steps in paths
#endif                                                             // PATH_TIMING
        // BUGBUG zero iTicks means this is an adjacent
        // cell that is both the destination and blocked
                        if ( !iTicks )
                        {
                            if ( phexPath != NULL )
                                delete[] phexPath;
                            phexPath = NULL;
                            iPathLen = 0;
                        }
#if PATH_TIMING
#ifdef _LOGOUT
                        if ( phexPath == NULL )
                            logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "GetPath() blocked a NULL path returned " );
#endif
#endif  // PATH_TIMING

#if EN_PATH_PROBES
                        MPATH_INC( "blocked" );  // dest hex unenterable; partial path or NULL
                        m_iProbeOutcome = po_blocked;
#endif
                        return ( phexPath );
                    }
                }
            }
        }

        // flag pTest so that it does not get re-picked
        pTest->m_iBoth = 0;
        NewBoth( pTest );

        // get lowest combined cost cell in list to repeat
        pLastTest = pTest;
        pTest     = GetLowestCost( iCnt );
        if ( pTest == NULL )
        {
#if EN_PATH_PROBES
            MPATH_INC( "exhausted" );  // open list empty pre-dest: unreachable-goal signature
#endif
            if ( !bDirectPath )
                pTest = GetClosestCell( );

            break;
        }

        // consider we have reached the destination
        if ( pTest->m_iBoth == m_iBestCost && iCnt == 1 )
        {
            TRAP( m_iBestCost == 0xFFFE );  // BUGBUG - problem on computing iCnt
            if ( AtDestination( pTest ) )
            {
                break;
            }
        }

        // put this in to prevent hangs
        iHang--;
        if ( !iHang )
        {
#if EN_PATH_PROBES
            // THE cap signal for this search, kept as the two DISTINCT terminations it
            // really is. Both land here - the iHang budget running out, and the arena
            // filling (which sets iHang = 1 to force this exit) - but they mean
            // different things, so they are recorded and compared separately.
            // Reusing the existing signal, not inventing one.
            if ( bProbeArenaFull )
                m_bProbeCapArena = TRUE;
            else
            {
                m_bProbeCapIter = TRUE;
                MPATH_INC( "ihang" );
            }
#endif

#if PATH_TIMING
#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "broke on HANG protection " );
            logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "pTest is at %d,%d  cost=%d  dist=%d  both=%d ", pTest->m_iX,
                       pTest->m_iY, pTest->m_iCost, pTest->m_iDist, pTest->m_iBoth );
            logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "Took %d interations with %d cells in list \n", iTicks, iList );
#endif
#endif

#if PATH_TIMING
            // what is the destination hex like?
            if ( pGameData != NULL && m_pTD != NULL )
            {
                CHex* pGameHex = theMap.GetHex( m_hexTo );
#ifdef _LOGOUT
                logPrintf(
                    LOG_PRI_ALWAYS, LOG_VEH_PATH,
                    "to hex %d,%d  CanTravelHex()=%d  CHex::GetUnits()=%d  CHex::GetType()=%d  CHex::GetAlt()=%d ",
                    m_hexTo.X( ), m_hexTo.Y( ), (int)m_pTD->CanTravelHex( pGameHex ), (int)pGameHex->GetUnits( ),
                    (int)pGameHex->GetType( ), (int)pGameHex->GetAlt( ) );
#endif
            }
#endif  // PATH_TIMING

            if ( !bDirectPath )
            {
                pTest = GetClosestCell( );
#if PATH_TIMING
#ifdef _LOGOUT
                logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "selected GetClosestCell() of %d,%d at %d \n", pTest->m_iX,
                           pTest->m_iY, pTest->m_iDist );
#endif
#endif
            }
            else
                pTest = NULL;
            break;
        }

        iTicks++;
    }

    if ( pTest == NULL )
    {
#if PATH_TIMING
#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "End of GetPath() NULL path returned " );
        logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "Took %d interations with %d cells in list \n", iTicks, iList );
#endif
#endif
        ClearArray( );
#if EN_PATH_PROBES
        MPATH_INC( "nopath" );
        m_iProbeOutcome = po_nopath;
#endif
        return ( NULL );
    }

    // a break from trying has occurred and the dest was reached
    if ( !iHang && pDestCell != NULL )
        pTest = pDestCell;

#if EN_PATH_PROBES
    // capture before ClearArray() wipes the cells
    BOOL      bProbeAtDest = AtDestination( pTest );
    CHexCoord hexProbeClamp( pTest->m_iX, pTest->m_iY );
#endif

    // now walk the m_plCells and
    // create a CHexCoord[] to return to caller
    CHexCoord* phexPath = CreateHexPath( view, iPathLen, pTest );

#if PATH_TIMING
#ifdef _LOGOUT
    if ( !iHang || iPathLen == 1 )
        // || m_pTD->GetType() == CTransportData::light_scout )
        ReportPath( phexPath, iPathLen );
#endif
#endif  // PATH_TIMING

    // remove cells used in pathfinding
    ClearArray( );

#if PATH_TIMING
    dwEnd = timeGetTime( );
#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "GetPath() for a %s took %ld ticks for %d steps ",
               m_pTD->GetDesc( ).c_str(), ( dwEnd - dwStart ), iPathLen );
    logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "from %d,%d to %d,%d ", m_hexFrom.X( ), m_hexFrom.Y( ), m_hexTo.X( ),
               m_hexTo.Y( ) );
    logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "Took %d interations with %d cells in list \n", iTicks, iList );
#endif
    if ( iHang )
    {
        m_iPathTicks += (int)( dwEnd - dwStart );  // sum of ticks in paths
        m_iStepCnt += iPathLen;                    // count of steps in paths
    }
    else
    {
        m_iHangTicks += (int)( dwEnd - dwStart );  // sum of ticks in hangs
        m_iHangCnt += iPathLen;                    // count of hang steps
    }
#endif

    // this may be a case of a 1 step path with the
    // destination occupied, so do one last check
    // on the single step and return NULL if blocked
    if ( /*!iHang || */ iPathLen == 1 )
    {
        CHexCoord*        pHex  = &phexPath[0];
        CHexCoord         hexDest( pHex->X( ), pHex->Y( ) );
        CEnHexFacts const fDest = view.GetHex( hexDest );
        if ( fDest.IsValid( ) )
        {
            // consider that a vehicle occupies the dest hex or
            // it cannot be entered
            BYTE bUnits = fDest.GetUnits( );
            if ( ( bUnits & ( CHex::ul | CHex::ur | CHex::ll | CHex::lr ) ) || !m_pTD->CanTravelHex( view, hexDest ) )
            {
                delete[] phexPath;
                phexPath = NULL;
                iPathLen = 0;
            }
        }
    }

#if PATH_TIMING
#ifdef _LOGOUT
    if ( phexPath == NULL )
        logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "last NULL path returned " );
#endif
#endif

#if EN_PATH_PROBES
    if ( phexPath == NULL )
    {
        MPATH_INC( "nopath" );
        m_iProbeOutcome = po_nopath;
    }
    else if ( bProbeAtDest )
    {
        MPATH_INC( "ok" );
        m_iProbeOutcome = po_ok;
        if ( pVehicle != NULL )
            pVehicle->m_hexLastClamp = CHexCoord( -1, -1 );  // success clears the re-clamp watch
    }
    else
    {
        MPATH_INC( "clamped" );  // GetClosestCell fallback path
        m_iProbeOutcome = po_clamped;
        if ( pVehicle != NULL )
        {
            if ( pVehicle->m_hexLastClamp == hexProbeClamp )
                MPATH_INC( "reclamp" );  // clamped at the SAME hex again: churn signature
            pVehicle->m_hexLastClamp = hexProbeClamp;
        }
    }
#endif

    return ( phexPath );
}

//
// while the range to the destination is greater than what is
// acceptable, make an adjustment by finding the mid point between
// m_hexFrom and the current m_hexTo and put the just found mid
// point into m_hexTo in lieu of the original destination
//
template <class TView>
void CPathMgr::AdjustDestination( TView const& view )
{
    int iRange = view.GetRangeDistance( m_hexFrom, m_hexTo );

    // #25 long-haul fix: the flat MAX_PATH_RANGE (80) clamp below halved any order
    // beyond ~80 hexes down to a near midpoint, so long hauls only pathed partway
    // ("short-path breakout"). Scale the clamp to THIS order's straight-line range
    // (m_hexTo is still the original destination here, pre-ChangeDestination), capped
    // at the map span (m_iWidth+m_iHeight) so it stays bounded. SHORT orders
    // (iRange <= m_iMaxPath) keep today's early return unchanged -> the common case is
    // unaffected (operator's no-regress gate). Long orders run the full search; iHang
    // (FindPath L215-219) already scales the node budget with range and early-exits, so
    // an over-budget search still terminates with best-so-far heading the right way.
    int iLimit = m_iMaxPath;
    if ( iRange > iLimit )
    {
        int iCap = m_iWidth + m_iHeight;            // map span = max meaningful range
        iLimit   = ( iRange < iCap ) ? iRange : iCap;
    }

    if ( iRange <= iLimit )
        return;

    while ( iRange > iLimit )
    {
        ChangeDestination( view );

        // get new range from the game
        iRange = view.GetRangeDistance( m_hexFrom, m_hexTo );
        if ( iRange < 0 )
        {
#if PATH_TIMING
#ifdef _LOGOUT
            logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "CPathMgr::range problem from %d,%d to %d,%d   range=%d ",
                       m_hexFrom.X( ), m_hexFrom.Y( ), m_hexTo.X( ), m_hexTo.Y( ), iRange );
#endif
#endif
            iRange = iLimit;
            continue;
        }

        // make sure we can test with CTransportData
        if ( m_pTD == NULL )
            continue;

        // make sure new destination is passible
        if ( !view.GetHex( m_hexTo ).IsValid( ) )
            continue;
        // new destination is not passible so stay in the loop
        if ( !m_pTD->CanTravelHex( view, m_hexTo ) )
            iRange = iLimit;
    }

#if 0  // PATH_TIMING
#ifdef _LOGOUT
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"CPathMgr::AdjustDestination() from %d,%d to %d,%d   range=%d ",
		m_hexFrom.X(), m_hexFrom.Y(), m_hexTo.X(), m_hexTo.Y(), iRange );
#endif
#endif
}
//
// find the mid point between
// m_hexFrom and the current m_hexTo and put the just found mid
// point into m_hexTo in lieu of the original destination
//
template <class TView>
void CPathMgr::ChangeDestination( TView const& view )
{
    int iNewX, iNewY;

    // get 1/2 the distance on each axis
    int iDeltaX = abs( view.Diff( m_hexFrom.X( ) - m_hexTo.X( ) ) ) / 2;
    int iDeltaY = abs( view.Diff( m_hexFrom.Y( ) - m_hexTo.Y( ) ) ) / 2;

    if ( view.Diff( m_hexFrom.X( ) - m_hexTo.X( ) ) < 0 )
        iNewX = view.Wrap( ( m_hexFrom.X( ) + iDeltaX ) );
    else
        iNewX = view.Wrap( ( m_hexFrom.X( ) - iDeltaX ) );

    if ( view.Diff( m_hexFrom.Y( ) - m_hexTo.Y( ) ) < 0 )
        iNewY = view.Wrap( ( m_hexFrom.Y( ) + iDeltaY ) );
    else
        iNewY = view.Wrap( ( m_hexFrom.Y( ) - iDeltaY ) );

    if ( !iNewX && !iNewY )
    {
        iNewX = 1;
    }
    // the masking CHexCoord::X(int)/Y(int) setters read the map width mask; both
    // values are already wrapped through the view here.
    m_hexTo = CHexCoord( view.Wrap( iNewX ), view.Wrap( iNewY ) );

#if PATH_TIMING
#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "GetPath() adjusted destination to %d,%d ", m_hexTo.X( ), m_hexTo.Y( ) );
#endif
#endif
}

#if 0  //_LOGOUT
// BUGBUG this is for solving the repeating path problem
void CPathMgr::ReportPath( CHexCoord *phexPath, int iPathLen )
{
	if( m_pTD == NULL )
		return;

	if( phexPath == NULL )
	{
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() path returned is NULL " );
		return;
	}

	CHex *pGameHex = theMap.GetHex( m_hexFrom );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() from-hex is occupied with %d ",
		(int)pGameHex->GetUnits() );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"GetPath() path returned is " );

	for( int i=0; i<iPathLen; ++i )
	{
		CHexCoord *pHex = &phexPath[i];
		
		CHexCoord hexDest( pHex->X(), pHex->Y() );
		pGameHex = theMap.GetHex( hexDest );
		if( pGameHex == NULL )
			return;

		CCell *pCell = GetCellAt(pHex->X(), pHex->Y());

	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"path step %d is %d,%d ", i, pHex->X(), pHex->Y() );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		" CanTravelHex()=%d  CHex::GetUnits()=%d  CHex::GetType()=%d  CHex::GetAlt()=%d ",
		(int)m_pTD->CanTravelHex( pGameHex ), (int)pGameHex->GetUnits(), 
		(int)pGameHex->GetType(), (int)pGameHex->GetAlt() );
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		" dist %d   cost %d   both %d ", 
		pCell->m_iDist, pCell->m_iCost, pCell->m_iBoth );

	}
}
#endif

//
// create arrays of appropriate objects, based on whether in use
// by the game or using fake map for testing, and return the pointer
// to the array, with the size of the array in iPathLen
//
template <class TView>
CHexCoord* CPathMgr::CreateHexPath( TView const& view, int& iPathLen, CCell* pDestCell )
{
    if ( pDestCell == NULL )
        return ( NULL );

#if USE_HEADINGS
    // first NULL out m_pCellFrom for the CCell representing m_hexFrom
    // which may have been set in order to find heading for m_hexFrom
    CCell* pFromCell       = GetCellAt( m_hexFrom.X( ), m_hexFrom.Y( ) );
    pFromCell->m_pCellFrom = NULL;
#endif

    // now count the number of cells in the path, by
    // following the CCell::m_pCellFrom to the next cell
    int i = GetPathCount( pDestCell );
    if ( !i )
        return ( NULL );

    // create a hex path array that size to return
    CHexCoord* pHexPath = NULL;
    pHexPath            = new CHexCoord[i];
    iPathLen            = i;


    // now go thru path, reversing step order
    CCell *pNext, *pThis;
    pThis = pDestCell;
    while ( TRUE )
    {
        if ( pThis == NULL )
            break;

        CHexCoord* pHex = &pHexPath[--i];
        *pHex           = CHexCoord( view.Wrap( pThis->m_iX ), view.Wrap( pThis->m_iY ) );

        // at the end
        if ( !i )
            break;

        // move to next cell in path
        pNext = pThis->m_pCellFrom;
        pThis = pNext;
    }


#if DEBUG_OUTPUT_PATH
    for ( i = 0; i < iPathLen; ++i )
    {
        CHexCoord* pHex = &pHexPath[i];
#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "path step %d is %d,%d ", i, pHex->X( ), pHex->Y( ) );
#endif
    }
#endif

#if DEBUG_OUTPUT_PATH
#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "total path steps %d ", iPathLen );
#endif
#endif

    return ( pHexPath );
}

//
// count the number of steps in the completed best path
//
int CPathMgr::GetPathCount( CCell* pDestCell )
{
    // now go thru path, reversing step order
    int    iCnt = 0;
    CCell *pNext, *pThis;
    pThis = pDestCell;
    while ( TRUE )
    {
        if ( pThis->m_pCellFrom == NULL )
            break;

        // move to next cell in path
        pNext = pThis->m_pCellFrom;
        pThis = pNext;
        iCnt++;
    }
    return ( iCnt );
}

//
// consider if the cell passed is the destination cell
//
BOOL CPathMgr::AtDestination( CCell* pCell )
{
    if ( pCell->m_iX == m_hexTo.X( ) && pCell->m_iY == m_hexTo.Y( ) )
        return TRUE;

    return FALSE;
}

//
// test for the destination being entered from a bridge
//
template <class TView>
BOOL CPathMgr::CanEnterBridge( TView const& view, CCell* pFromCell, CCell* pToCell )
{
    // need this because multiple threads call thePathMgr and it is global
    if ( m_pTD == NULL )
        return FALSE;

    // get game data for this cell
    CHexCoord         hexDest( pToCell->m_iX, pToCell->m_iY );
    CEnHexFacts const fDest = view.GetHex( hexDest );
    if ( !fDest.IsValid( ) )
        return FALSE;

    // create hex for from cell
    CHexCoord         hexFrom( pFromCell->m_iX, pFromCell->m_iY );
    CEnHexFacts const fFrom = view.GetHex( hexFrom );
    if ( !fFrom.IsValid( ) )
        return FALSE;

    // if there is bridge hex involved, we must do another test
    if ( fDest.GetUnits( ) & CHex::bridge || fFrom.GetUnits( ) & CHex::bridge )
    {
        // A WATER vehicle is ALWAYS on the water — it passes UNDER bridges, never on
        // the deck — so force on-water for it; otherwise exiting a bridge-over-water
        // hex hits CanEnterHex's bridge-deck direction checks and a boat (e.g. cargo
        // ship) can't traverse under the span. Keep this in sync with GetCellCosts,
        // the other A* gate, which already has this fix.
        BOOL bOnWater = ( m_pTD->GetWheelType( ) == CWheelTypes::water ) ||
                        ( fFrom.IsWater( ) & ( ( fFrom.GetUnits( ) & CHex::bridge ) == 0 ) );
        if ( !m_pTD->CanEnterHex( view, hexFrom, hexDest, bOnWater ) )
            return FALSE;
    }
    return TRUE;
}

//
// get cost to enter 'from' cell into 'to' cell, and if less than
// the current cost in 'to' then record cost, and make 'to' cell
// point to 'from' cell also get range to destination for 'to' cell
// and record distance and combine the two into 'to' both
//
template <class TView>
void CPathMgr::GetCellCosts( TView const& view, int iPos, CCell* pFromCell, CCell* pToCell )
{
    // this cell is the start cell and should not be costed
    if ( !pToCell->m_iCost )
        return;
    // hex at 0,0 is never passable
    if ( !pToCell->m_iX && !pToCell->m_iY )
        return;

    // need this because multiple threads call thePathMgr and it is global
    if ( m_pTD == NULL )
        return;

    // get game data for this cell
    CHexCoord         hexDest( pToCell->m_iX, pToCell->m_iY );
    CEnHexFacts const fDest = view.GetHex( hexDest );
    if ( !fDest.IsValid( ) )
        return;

    // test to enter hex
    if ( !m_pTD->CanTravelHex( view, hexDest ) )
        return;

    // since DT will not change the terrain cost data for coastlines
    // to make them more expensive to travel, we will do the same
    // thing in code, which slows down the pathing process
    //
    // if terrain coastline
    // then let's skip it
    // unless its the dest
    if ( fDest.GetType( ) == CHex::coastline )
    {
        // except of course, inf/outriders can travel coastline anytime — and small
        // boats (motorboat/gun_boat, landing craft) must cross coastline shores to
        // reach rivers / lakes through their mouths.
        if ( m_pTD->GetWheelType( ) != CWheelTypes::walk && m_pTD->GetWheelType( ) != CWheelTypes::hover &&
             m_pTD->GetType( ) != CTransportData::gun_boat && m_pTD->GetType( ) != CTransportData::landing_craft &&
             hexDest != m_hexTo )
        {
            // but any vehicle on a bridge
            if ( !( fDest.GetUnits( ) & CHex::bridge ) )
                return;
        }
    }
    // BRIDGEBUG will need to create (CHex) pFromHex
    // and test for pFromHex->GetUnits() & CHex::bridge and
    // if TRUE then use m_pTD->CanEnterHex( hexFrom, hexDest )
    // to determine if the hex can be entered and then not use
    // the m_pTD->CanTravelHex( pDestHex )

    // create hex for from cell
    CHexCoord         hexFrom( pFromCell->m_iX, pFromCell->m_iY );
    CEnHexFacts const fFrom = view.GetHex( hexFrom );
    if ( !fFrom.IsValid( ) )
        return;

    // if there is bridge hex involved, we must do another test
    if ( fDest.GetUnits( ) & CHex::bridge || fFrom.GetUnits( ) & CHex::bridge )
    {
        // figure if we're on the water (vs land/bridge-deck): a from-hex that is water
        // & not a bridge means on water. A WATER vehicle is ALWAYS on the water — it
        // passes UNDER bridges, never on the deck — so force on-water for it; otherwise
        // exiting a bridge-over-water hex hits CanEnterHex's bridge-deck direction
        // checks and a boat can't traverse the span.
        BOOL bOnWater = ( m_pTD->GetWheelType( ) == CWheelTypes::water ) ||
                        ( fFrom.IsWater( ) & ( ( fFrom.GetUnits( ) & CHex::bridge ) == 0 ) );
        if ( !m_pTD->CanEnterHex( view, hexFrom, hexDest, bOnWater ) )
            return;
        // if( !m_pTD->CanEnterHex(hexFrom, hexDest, pFromHex->IsWater()) )
        //	return;
    }

    // get cost from the game
    // int CGameMap::GetTerrainCost (CHexCoord const & hex,
    // CHexCoord const & hexNext, int iDir, int iWheel)
    int iCost = view.GetTerrainCost( hexFrom, hexDest, iPos, m_pTD->GetWheelType( ) );

    // do not allow ZERO cost terrain to proceed
    if ( !iCost )
        return;

    // since DT will not change the terrain cost data to solve the
    // problem in using roads, then we must to do it with code,
    // so we need to multiply the cost of all non-road terrain
    // to increase the diff, which of course, slows down the process
    // of finding a path.  Oh, well I did try to tell him, but as
    // usual he would not listen.
    if ( fDest.GetType( ) != CHex::road )
        iCost <<= 1;

    // if this cost + cost to this point < what we already have
    // then save the value and the pointer to where we came from
    if ( ( iCost + pFromCell->m_iCost ) < pToCell->m_iCost )
    {
        pToCell->m_iCost     = ( iCost + pFromCell->m_iCost );
        pToCell->m_pCellFrom = pFromCell;
    }
    else
        return;

    // prepare for occupation test
    pToCell->m_iDist = 0;

    // consider if hex is occupied
    //
    // might only be part of a unit if multi-hex units are allowed
    // or parts of other multi-hex units occupying hexes occupied
    // by this multi-hex unit
    //
    // CUnit *pUnit = pGameHex->GetUnit();
    BYTE bUnits = fDest.GetUnits( );

    // force arrival at destinations, regardless of if occupied
    // if( pUnit != NULL && hexDest != m_hexTo )
    if ( bUnits != 0 && hexDest != m_hexTo )
    {
        // unit is a building, consider if this entry hex
        // BUGBUG - but who owns it?
        //
        // if( pUnit->GetUnitType() == CUnit::building )
        // int iType = pUnit->GetUnitType();
        // BUGBUG dave needs a temp change when we update/sync
        // and he uses new hash system
        // if( iType != CUnit::vehicle )
        // if( iType == CUnit::building )
        if ( bUnits & CHex::bldg )
        {
            // this is a hex of a building
            // and is not the destination hex for vehicle
            pToCell->m_iDist = 0xFFFE;  // no entry
        }
        else if ( bUnits & ( CHex::ul | CHex::ur | CHex::ll | CHex::lr ) )
        {
            // vehicle hexes are considered open unless block set
            //
            // ...and even then, a MOVING vehicle is not an obstacle: it will have
            // driven on long before we reach its hex, so planning around it is what
            // turns a queue into a detour into the oncoming lane. Only a vehicle
            // that is actually sitting there blocks the route.
            if ( m_bVehBlock && ( ( !( view.TrafficRules( ) & 32 ) ) || ( !view.IsHexMovingVehicle( hexDest ) ) ) )
                pToCell->m_iDist = 0xFFFE;  // no entry
        }
    }


    // get distance to destination for enterable cells
    // and cost to enter is already in pCell->m_iCost
    if ( !pToCell->m_iDist )
        pToCell->m_iDist = ( view.GetRangeDistance( hexDest, m_hexTo ) * m_iDistFactor );

    // else if distance is left alone then recalculate m_iBoth
    if ( pToCell->m_iDist != 0xFFFE )
    {
        pToCell->m_iBoth = pToCell->m_iCost + pToCell->m_iDist;
    }
    else
        pToCell->m_iBoth = pToCell->m_iDist;
    NewBoth( pToCell );
}

//
// using the actual sub-hex head and tail of the passed vehicle
// determine the vehicle's current heading, and then initialize
// a CCell for the hex that was left to reach the current from
// cell and secure the pFromCell->m_pCellFrom to point to it
//
void CPathMgr::GetFromCell( CVehicle* pVeh, CCell* pFromCell )
{
    int       iHeading = pVeh->CalcBaseDir( );
    CHexCoord hex( pFromCell->m_iX, pFromCell->m_iY );
    switch ( iHeading )
    {
    case 1:
        hex.Yinc( );
        hex.Xdec( );
        break;
    case 2:
        hex.Xdec( );
        break;
    case 3:
        hex.Ydec( );
        hex.Xdec( );
        break;
    case 4:
        hex.Ydec( );
        break;
    case 5:
        hex.Xinc( );
        hex.Ydec( );
        break;
    case 6:
        hex.Xinc( );
        break;
    case 7:
        hex.Xinc( );
        hex.Yinc( );
        break;
    case 0:
    default:
        hex.Ydec( );
        break;
    }
    // create a CCell to point back to for heading
    CCell  cc( hex.X( ), hex.Y( ) );
    CCell* pCell = AddCellToArray( &cc );
    TRAP( );
#ifdef TEST_RESULT2
    TRAP( pCell != GetCellAt( hex.X( ), hex.Y( ) ) );
#endif
    pFromCell->m_pCellFrom = pCell;
}

//
// determine if iPos passed is a valid heading out of pFromCell based
// on the heading used to enter pFromCell from pFromCell->m_pCellFrom
// and return TRUE if it is valid, or FALSE if not
//
BOOL CPathMgr::IsValidHeading( int iPos, CCell* pFromCell )
{
    // first cell, without a heading, will have NULL
    if ( pFromCell->m_pCellFrom == NULL )
        return TRUE;

    // test if this is a single sub-hex vehicle?
    if ( ( m_pTD->GetVehFlags( ) & CTransportData::FL1hex ) )
        return TRUE;

    // passed pFromCell was entered on a heading from m_pCellFrom
    CHexCoord hexTo( pFromCell->m_iX, pFromCell->m_iY );
    CHexCoord hexFrom( pFromCell->m_pCellFrom->m_iX, pFromCell->m_pCellFrom->m_iY );

    // find the direction of the heading that was used to move
    // FROM pFromCell->m_pCellFrom TO pFromCell
    int i = 0;
    for ( ; i < CELLSAROUND; ++i )
    {
        CHexCoord hex = hexFrom;
        switch ( i )
        {
        case 0:
            hex.Ydec( );
            break;
        case 1:
            hex.Ydec( );
            hex.Xinc( );
            break;
        case 2:
            hex.Xinc( );
            break;
        case 3:
            hex.Yinc( );
            hex.Xinc( );
            break;
        case 4:
            hex.Yinc( );
            break;
        case 5:
            hex.Xdec( );
            hex.Yinc( );
            break;
        case 6:
            hex.Xdec( );
            break;
        case 7:
            hex.Xdec( );
            hex.Ydec( );
            break;
        }
        if ( hex == hexTo )
            break;
    }
    // 'i' now represents heading used to enter pFromCell
    // from pFromCell->m_pCellFrom, so use it to access
    // lookup table of candidate directions off of that
    // heading in which each byte is a bit array with 1
    // in the bit that represents a candidate direction
    // for that heading
    unsigned char ucDirections = ucHeadings[i];
    unsigned char ucTest       = 0;
    ucTest                     = 1 << iPos;
    // if iPos represents a valid heading return TRUE
    if ( ( ucDirections & ucTest ) )
        return TRUE;

    return FALSE;
}

/*
>x == newHex.x - oldHex.x
>y == newHex.y - oldHex.y
>
>you then try:
>
>newHex.x + x, newHex.y + y	// straight line
>newHex.x + __minmax (-1, 1, x - y), newHex.y + __minmax (-1, 1, x + y)	// soft right turn
>newHex.x + __minmax (-1, 1, y + x), newHex.y + __minmax (-1, 1, y - x)	// soft left turn
>
>#define __minmax(min,max,val)  (val < min ? min : (val > max ? max : val))
>
*/
void CPathMgr::GetHeadingCell( int iPos, CCell* pFromCell, int& iX, int& iY )
{
    // passed pFromCell was entered on a heading from m_pCellFrom
    CHexCoord hexTo( pFromCell->m_iX, pFromCell->m_iY );
    CHexCoord hexFrom( pFromCell->m_pCellFrom->m_iX, pFromCell->m_pCellFrom->m_iY );

    // what if hexFrom.X() == 127 and hexTo.X() == 0 ?
    int x = hexTo.X( ) - hexFrom.X( );
    x     = x > 1 ? 1 : ( x < -1 ? -1 : x );
    int y = hexTo.Y( ) - hexFrom.Y( );
    y     = y > 1 ? 1 : ( y < -1 ? -1 : y );

    if ( iPos == 0 )  // on same heading
    {
        iX = hexTo.Wrap( hexTo.X( ) + x );
        iY = hexTo.Wrap( hexTo.Y( ) + y );
    }
    else if ( iPos == 1 )  // soft right turn from heading
    {
        iX = hexTo.Wrap( hexTo.X( ) + __minmax( -1, 1, x - y ) );
        iY = hexTo.Wrap( hexTo.Y( ) + __minmax( -1, 1, x + y ) );
    }
    else if ( iPos == 2 )  // soft left turn from heading
    {
        iX = hexTo.Wrap( hexTo.X( ) + __minmax( -1, 1, y + x ) );
        iY = hexTo.Wrap( hexTo.Y( ) + __minmax( -1, 1, y - x ) );
    }
}


//
// get the adjacent cell x,y values
// in the iPos direction from pFromCell
//
template <class TView>
void CPathMgr::GetCellAt( TView const& view, int iPos, CCell* pFromCell, int& iX, int& iY )
{
    int x = pFromCell->m_iX;
    int y = pFromCell->m_iY;
    switch ( iPos )
    {
    case 0:
        y--;
        break;
    case 1:
        y--;
        x++;
        break;
    case 2:
        x++;
        break;
    case 3:
        y++;
        x++;
        break;
    case 4:
        y++;
        break;
    case 5:
        x--;
        y++;
        break;
    case 6:
        x--;
        break;
    case 7:
        x--;
        y--;
        break;
    }
    iX = view.Wrap( x );
    iY = view.Wrap( y );
}

//
// check the array for the cell with the lowest distance value
//
CCell* CPathMgr::GetClosestCell( void )
{
    CCell* pcClosest = NULL;
    int    iBestDist = INT_MAX;

    // change to reflect new approach
    int iEnd = min( m_iNextSlot, m_iNumOfCells );

    CCell* pCell = &m_paCells[0];
    for ( int i = 0; i < iEnd; ++i, pCell++ )
    {
        if ( pCell->m_iDist && pCell->m_iDist < iBestDist )
        {
            pcClosest = pCell;
            iBestDist = pCell->m_iDist;
        }
    }
    return ( pcClosest );
}

//
// considering the status of reaching the path as a guide,
// find the cell with the lowest cost and return it
//
CCell* CPathMgr::xGetLowestCost( int& iCnt )
{
    CCell* pcLowest = NULL;
    int    iCost    = 0xFFFE;
    iCnt            = 0;

    // change to reflect new approach
    int iEnd = min( m_iNextSlot, m_iNumOfCells );

    CCell* pCell = &m_paCells[0];
    CCell* pEnd  = pCell + iEnd;

    // consider combined cost after dest reached 1st time
    if ( m_iBestCost != 0xFFFE )
    {
        for ( ; pCell < pEnd; pCell++ )
        {
            if ( pCell->m_iBoth < iCost && pCell->m_iBoth )
            {
                pcLowest = pCell;
                iCost    = pCell->m_iBoth;
                iCnt     = 0;
            }
            else if ( pCell->m_iBoth == iCost )
                iCnt++;
        }
        return ( pcLowest );
    }

    // until the destination is reached once,
    // attempt direct path considering only distance
    for ( ; pCell < pEnd; pCell++ )
    {
        if ( pCell->m_iDist && pCell->m_iBoth && pCell->m_iDist < iCost )
        {
            pcLowest = pCell;
            iCost    = pCell->m_iDist;
            iCnt     = 0;
        }
        // BUGBUG - eric had this but it makes no sense		if( pCell->m_iBoth == iCost )
        //  also eric did not have the else
        else if ( pCell->m_iBoth && ( pCell->m_iDist == iCost ) )
            iCnt++;
    }

    return ( pcLowest );
}

CCell* CPathMgr::GetLowestCost( int& iCnt )
{

#ifdef TEST_RESULT3
    {
        CCell** ppCell = m_acBoth;
        for ( int iOn = 0; iOn < MAX_BOTH_INDEX; iOn++ )
        {
            if ( *ppCell != NULL )
            {
                TRAP( iOn < m_iLowestBoth );
                TRAP( ( *ppCell )->m_iBothIn != iOn );
            }
            ppCell++;
        }
    }
#endif

    // use old method if out of range
    if ( ( m_iLowestBoth <= 0 ) || ( MAX_BOTH_INDEX <= m_iLowestBoth ) )
        return xGetLowestCost( iCnt );

    // special case
    if ( ( m_acBoth[m_iLowestBoth] != NULL ) && ( m_acBoth[m_iLowestBoth]->m_pcNextBoth == NULL ) )
    {
        iCnt = 0;
#ifdef TEST_RESULT2
        int iTest;
        TRAP( m_acBoth[m_iLowestBoth] != xGetLowestCost( iTest ) );
        TRAP( iTest != iCnt );
#endif
        return m_acBoth[m_iLowestBoth];
    }

    // find lowest entry
    CCell** ppCellOn = &m_acBoth[m_iLowestBoth];
    int     iNum     = MAX_BOTH_INDEX - m_iLowestBoth;
    while ( ( *ppCellOn == NULL ) && ( iNum-- > 0 ) ) ppCellOn++;

    // no entries
    if ( iNum <= 0 )
        return xGetLowestCost( iCnt );

    // we now walk the linked list returning the lowest one
    CCell *pcLowest, *pCellOn;
    pcLowest = pCellOn = *ppCellOn;
    iCnt               = 0;
    while ( pCellOn->m_pcNextBoth != NULL )
    {
        pCellOn  = pCellOn->m_pcNextBoth;
        pcLowest = __min( pcLowest, pCellOn );
        iCnt++;
    }

#ifdef TEST_RESULT2
    int iTest;
    TRAP( pcLowest != xGetLowestCost( iTest ) );
    TRAP( iTest != iCnt );
#endif
    return ( pcLowest );
}

void CPathMgr::NewBoth( CCell* pTest )
{

#ifdef TEST_RESULT3
    {
        CCell** ppCell = m_acBoth;
        for ( int iOn = 0; iOn < MAX_BOTH_INDEX; iOn++ )
        {
            if ( *ppCell != NULL )
            {
                TRAP( iOn < m_iLowestBoth );
                TRAP( ( *ppCell )->m_iBothIn != iOn );
            }
            ppCell++;
        }
    }
#endif

    // first remove old
    if ( pTest->m_iBothIn != 0 )
    {
        // remove it
        if ( pTest->m_pcNextBoth != NULL )
            pTest->m_pcNextBoth->m_pcPrevBoth = pTest->m_pcPrevBoth;
        if ( pTest->m_pcPrevBoth == NULL )
        {
#ifdef TEST_RESULT2
            TRAP( m_acBoth[pTest->m_iBothIn] != pTest );
#endif
            m_acBoth[pTest->m_iBothIn] = pTest->m_pcNextBoth;
        }
        else
            pTest->m_pcPrevBoth->m_pcNextBoth = pTest->m_pcNextBoth;
    }

    // common - get out fast
    if ( pTest->m_iBoth == 0 )
    {
        pTest->m_pcNextBoth = pTest->m_pcPrevBoth = NULL;
        pTest->m_iBothIn                          = 0;
#ifdef TEST_RESULT3
        {
            CCell** ppCell = m_acBoth;
            for ( int iOn = 0; iOn < MAX_BOTH_INDEX; iOn++ )
            {
                if ( *ppCell != NULL )
                {
                    TRAP( iOn < m_iLowestBoth );
                    TRAP( ( *ppCell )->m_iBothIn != iOn );
                }
                ppCell++;
            }
        }
#endif

        return;
    }

    // figure the cost
    int iCost;
    if ( m_iBestCost != 0xFFFE )
        iCost = pTest->m_iBoth;
    else
        iCost = pTest->m_iDist;

    // add new
    pTest->m_pcPrevBoth = NULL;
    if ( ( 0 < iCost ) && ( iCost < MAX_BOTH_INDEX ) )
    {
        CCell* pCelOn       = m_acBoth[iCost];
        pTest->m_pcNextBoth = pCelOn;
        if ( pCelOn != NULL )
            pCelOn->m_pcPrevBoth = pTest;
        m_acBoth[iCost]  = pTest;
        pTest->m_iBothIn = iCost;
        if ( m_iLowestBoth == 0 )
            m_iLowestBoth = iCost;
        else
            m_iLowestBoth = __min( m_iLowestBoth, iCost );
    }

    else
    {
        pTest->m_pcNextBoth = NULL;
        pTest->m_iBothIn    = 0;
    }

#ifdef TEST_RESULT3
    {
        CCell** ppCell = m_acBoth;
        for ( int iOn = 0; iOn < MAX_BOTH_INDEX; iOn++ )
        {
            if ( *ppCell != NULL )
            {
                TRAP( iOn < m_iLowestBoth );
                TRAP( ( *ppCell )->m_iBothIn != iOn );
            }
            ppCell++;
        }
    }
#endif
}

CCell* CPathMgr::GetCellAt( int iX, int iY )
{

    DWORD  dwKey = ( iX & 0xFFFF ) | ( ( iY & 0xFFFF ) << 16 );
    CCell* pCellFind;
    // do we have it?
    if ( m_mapCell.Lookup( dwKey, pCellFind ) == 0 )
        pCellFind = NULL;
    else
    {
        // only need to look if more than 1
        while ( pCellFind->m_pCellNext != NULL )
        {
            TRAP( );
            if ( ( pCellFind->m_iX == iX ) && ( pCellFind->m_iY == iY ) )
            {
                TRAP( );
                break;
            }
            TRAP( );
            pCellFind = pCellFind->m_pCellNext;
        }
#ifdef TEST_RESULT1
        TRAP( ( pCellFind->m_iX != iX ) || ( pCellFind->m_iY != iY ) );
#endif
    }

#ifdef TEST_RESULT1
    CCell* pNewCell = &m_paCells[0];
    for ( int j = 0; j < m_iNextSlot; ++j, pNewCell++ )
    {
        if ( pNewCell->m_iX == iX && pNewCell->m_iY == iY )
        {
            TRAP( pNewCell != pCellFind );
            goto found;
        }
    }
    TRAP( pCellFind != NULL );
found:
#endif

    return ( pCellFind );
}

// BUGBUG: identical implementation to CPathMap::AddCellToArray! 
CCell* CPathMgr::AddCellToArray( CCell* pCell )
{
    if ( m_iNextSlot >= m_iNumOfCells )
        return NULL;

    CCell* pNewCell       = &m_paCells[m_iNextSlot++];
    pNewCell->m_iX        = pCell->m_iX;
    pNewCell->m_iY        = pCell->m_iY;
    pNewCell->m_iCost     = pCell->m_iCost;
    pNewCell->m_iDist     = pCell->m_iDist;
    pNewCell->m_iBoth     = pCell->m_iBoth;
    pNewCell->m_pCellFrom = pCell->m_pCellFrom;
    pNewCell->m_bClosed   = 0;  // CPathMgr never closes; keep recycled slots clean
    NewBoth( pNewCell );

    DWORD  dwKey = ( pCell->m_iX & 0xFFFF ) | ( ( pCell->m_iY & 0xFFFF ) << 16 );
    CCell* pCellFind;
    // add first element to hash table
    if ( m_mapCell.Lookup( dwKey, pCellFind ) == 0 )
    {
        m_mapCell.SetAt( dwKey, pNewCell );
        pNewCell->m_pCellNext = NULL;
    }

    // add another element to a hash element
    else
    {
        TRAP( );
        while ( pCellFind->m_pCellNext != NULL )
        {
            TRAP( );
            pCellFind = pCellFind->m_pCellNext;
        }
        TRAP( );
        pCellFind->m_pCellNext = pNewCell;
    }

    return pNewCell;
}

//
// clear the contents of the array
//
void CPathMgr::ClearArray( void )
{
    const int iVal  = 0xFFFE;
    const int iZero = 0;

    int iEnd = min( m_iNextSlot, m_iNumOfCells );

    m_mapCell.RemoveAll( );

    CCell* pCell = &m_paCells[0];
    for ( int i = 0; i < iEnd; ++i, pCell++ )
    {
        pCell->m_iX         = iZero;
        pCell->m_iY         = iZero;
        pCell->m_iCost      = iVal;
        pCell->m_iDist      = iVal;
        pCell->m_iBoth      = iVal;
        pCell->m_pCellFrom  = NULL;
        pCell->m_pCellNext  = NULL;
        pCell->m_pcNextBoth = NULL;
        pCell->m_pcPrevBoth = NULL;
        pCell->m_iBothIn    = 0;
    }

    m_iLowestBoth = 0;
    memset( m_acBoth, 0, sizeof( m_acBoth ) );
}

////////////////////////////////////////////////////////////////////////
//
// constructor with initialization
//
CPathMgr::CPathMgr( int iMapEX, int iMapEY )
{
    // NOTE: unused today (thePathMgr and the shadow instance both use the default
    // ctor + Init()). It does NOT set m_iMaxPath/m_pTD, so Init() is still required
    // before a search - but it must at least leave m_cs in a legal state, because it
    // allocates the arena and ~CPathMgr used to read "arena != NULL" as "section live".
    m_bCsInit   = FALSE;
    m_bShadow   = FALSE;
    m_iWidth    = iMapEX;
    m_iHeight   = iMapEY;
    m_iMapEX    = iMapEX - 1;
    m_iMapEY    = iMapEY - 1;
    m_bVehBlock = TRUE;

    m_iNumOfCells = ( m_iWidth + m_iHeight ) * 5;  // was *2 (1996); *5 = 2.5x search headroom for big obstacle-cut maps
    m_iNextSlot   = 0;
    m_iLowestBoth = 0;
    memset( m_acBoth, 0, sizeof( m_acBoth ) );
    m_iFirst = m_iNumOfCells - 1;
    m_iLast  = 0;

    m_paCells = new CCell[m_iNumOfCells];

    m_mapCell.RemoveAll( );
    m_mapCell.InitHashTable( GetPrime( m_iNumOfCells * 2 ) );

    m_iNextSlot = m_iFirst;
    ClearArray( );
    m_iNextSlot   = 0;
    m_iLowestBoth = 0;

    memset( &m_cs, 0, sizeof( m_cs ) );
    InitializeCriticalSection( &m_cs );
    m_bCsInit = TRUE;

    return;
}


void CPathMgr::Close( )
{
    // ReportCounts();

    delete[] m_paCells;
    m_paCells = NULL;

    m_mapCell.RemoveAll( );
}


//
// separate initialization process
//
BOOL CPathMgr::Init( int iMapEX, int iMapEY )
{
    m_iPaths     = 0;  // count of all calls
    m_iOrtho     = 0;  // count of x==x or y==y paths
    m_iHP        = 0;  // count of non Vehicle paths
    m_iPathTicks = 0;  // sum of ticks in paths
    m_iStepCnt   = 0;  // count of steps in paths
    m_iHangTicks = 0;  // sum of ticks in hangs
    m_iHangCnt   = 0;  // count of hang steps

    m_iWidth  = iMapEX;
    m_iHeight = iMapEY;
    m_iMapEX  = iMapEX - 1;
    m_iMapEY  = iMapEY - 1;

    m_iNumOfCells = ( m_iWidth + m_iHeight ) * 5;  // was *2 (1996); *5 = 2.5x search headroom for big obstacle-cut maps
    m_iNextSlot   = 0;
    m_iLowestBoth = 0;
    memset( m_acBoth, 0, sizeof( m_acBoth ) );
    m_iFirst   = m_iNumOfCells - 1;
    m_iLast    = 0;
    m_iMaxPath = MAX_PATH_RANGE;

    // make adjustment if using large or medium worlds
    /*
    if( theGame.GetSideSize() > 64 )
        m_iMaxPath /= 3;
    else if( theGame.GetSideSize() >= 32 )
        m_iMaxPath /= 2;
    */

    if ( m_paCells != NULL )
        delete[] m_paCells;
    // Null it immediately: if the new[] below throws, the dtor must not delete the freed arena again.
    m_paCells = NULL;

    // Tear the OLD section down whenever one exists. Keying this off m_paCells meant a
    // Close()+Init() pair (new game after a game) re-Initialize()d a live section with
    // no matching Delete. See m_bCsInit in cpathmgr.h.
    if ( m_bCsInit )
    {
        DeleteCriticalSection( &m_cs );
        m_bCsInit = FALSE;
    }

    m_paCells = new CCell[m_iNumOfCells];

    m_mapCell.RemoveAll( );
    m_mapCell.InitHashTable( GetPrime( m_iNumOfCells + 1 ) );

    m_iNextSlot = m_iFirst;
    ClearArray( );
    m_iNextSlot   = 0;
    m_iLowestBoth = 0;

#if 1
    // BUGBUG used for solving repeating path problem
    m_lastFrom.X( 0 );
    m_lastFrom.Y( 0 );
    m_lastTo = m_lastFrom;
#endif

#if PATH_TIMING
#ifdef _LOGOUT
    logPrintf( LOG_PRI_ALWAYS, LOG_VEH_PATH, "\nCPathMgr::Init() for %d,%d  m_iNumOfCells=%d ", iMapEX, iMapEX,
               m_iNumOfCells );
#endif
#endif

    // private critical section
    memset( &m_cs, 0, sizeof( m_cs ) );
    InitializeCriticalSection( &m_cs );
    m_bCsInit = TRUE;

    return TRUE;
}

//
// constructor does not initialize
//
CPathMgr::CPathMgr( void )
{
    m_iWidth      = 0;
    m_iHeight     = 0;
    m_iMapEX      = 0;
    m_iMapEY      = 0;
    m_iDistFactor = 0;
    m_bVehBlock   = TRUE;

    m_iLowestBoth = 0;
    memset( m_acBoth, 0, sizeof( m_acBoth ) );
    m_paCells = NULL;
    m_bCsInit = FALSE;  // Init() creates m_cs; nothing may enter it before that
    m_bShadow = FALSE;  // only MarkShadow() ever changes this
}

CPathMgr::~CPathMgr( )
{
    // Close() nulls m_paCells but leaves m_cs live, so the old "arena != NULL" test
    // leaked the section on every Close()d instance.
    if ( m_bCsInit )
    {
        DeleteCriticalSection( &m_cs );
        m_bCsInit = FALSE;
    }

    delete[] m_paCells;
    m_paCells = NULL;

    m_mapCell.RemoveAll( );
}

#if 0
void CPathMgr::ReportCounts( void )
{
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"\nTotal path calls %d  ortho %d  HP %d", 
		m_iPaths,m_iOrtho,m_iHP );
	if( !m_iStepCnt )
		m_iStepCnt = 1;
	if( m_iPathTicks > m_iStepCnt )
	{
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"path ticks %d  steps %d  ticks/step %d", 
		m_iPathTicks,m_iStepCnt,m_iPathTicks/m_iStepCnt );
	}
	else if( m_iPathTicks )
	{
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"path ticks %d  steps %d  steps/tick %d", 
		m_iPathTicks,m_iStepCnt,m_iStepCnt/m_iPathTicks );
	}
	if( !m_iHangCnt )
		m_iHangCnt = 1;
	logPrintf(LOG_PRI_ALWAYS, LOG_VEH_PATH, 
		"hang ticks %d  steps %d  ticks/step %d \n", 
		m_iHangTicks,m_iHangCnt,m_iHangTicks/m_iHangCnt );
}
#endif

//
// one of Dave's CGameMap object member functions
//
int CGameMap::GetTravelTime( CHexCoord const& hexSrc, CHexCoord const& hexDest, int iVehType )
{
    CHexCoord hcFrom = hexSrc;
    CHexCoord hcTo   = hexDest;
    // get the path
    int        iPathLen;
    CHexCoord* pHexPath = thePathMgr.GetPath( NULL, hcFrom, hcTo, iPathLen, iVehType );

    // get the wheel type again
    CTransportData const* pTD = theTransports.GetData( iVehType );
    if ( pTD == NULL )
        return ( 0 );

    int iWheel = pTD->GetWheelType( );

    // now cost it out
    int       iPathCost = 0;
    CHexCoord fromHex   = hexSrc;
    CHexCoord toHex;
    for ( int i = 0; i < iPathLen; ++i )
    {
        toHex    = pHexPath[i];
        int iPos = thePathMgr.GetCellDirection( fromHex, toHex );

        // bad position returned
        if ( iPos == CELLSAROUND )
            break;
        // add up the final costs
        iPathCost += theMap.GetTerrainCost( fromHex, toHex, iPos, iWheel );
        // move along the path
        fromHex = toHex;
    }

    delete[] pHexPath;
    return ( iPathCost );
}

//
// return the cells around direction used to go fromHex toHex
//
int CPathMgr::GetCellDirection( CHexCoord& fromHex, CHexCoord& toHex )
{
    // consider 0
    CHexCoord nextHex = fromHex;
    nextHex.Ydec( );
    if ( nextHex.m_iY == toHex.m_iY && nextHex.m_iX == toHex.m_iX )
        return ( 0 );

    // consider 1
    nextHex = fromHex;
    nextHex.Ydec( );
    nextHex.Xinc( );
    if ( nextHex.m_iY == toHex.m_iY && nextHex.m_iX == toHex.m_iX )
        return ( 1 );

    // consider 2
    nextHex = fromHex;
    nextHex.Xinc( );
    if ( nextHex.m_iY == toHex.m_iY && nextHex.m_iX == toHex.m_iX )
        return ( 2 );

    // consider 3
    nextHex = fromHex;
    nextHex.Yinc( );
    nextHex.Xinc( );
    if ( nextHex.m_iY == toHex.m_iY && nextHex.m_iX == toHex.m_iX )
        return ( 3 );

    // consider 4
    nextHex = fromHex;
    nextHex.Yinc( );
    if ( nextHex.m_iY == toHex.m_iY && nextHex.m_iX == toHex.m_iX )
        return ( 4 );

    // consider 5
    nextHex = fromHex;
    nextHex.Yinc( );
    nextHex.Xdec( );
    if ( nextHex.m_iY == toHex.m_iY && nextHex.m_iX == toHex.m_iX )
        return ( 5 );

    // consider 6
    nextHex = fromHex;
    nextHex.Xdec( );
    if ( nextHex.m_iY == toHex.m_iY && nextHex.m_iX == toHex.m_iX )
        return ( 6 );

    // consider 7
    nextHex = fromHex;
    nextHex.Ydec( );
    nextHex.Xdec( );
    if ( nextHex.m_iY == toHex.m_iY && nextHex.m_iX == toHex.m_iX )
        return ( 7 );

    return ( CELLSAROUND );
}

/////////////////////////////////////////////////////////////////////////
//
// CCell
//
// the basic cell object
//
/////////////////////////////////////////////////////////////////////////

CCell::CCell( )
{
    m_iX         = 0;
    m_iY         = 0;
    m_iCost      = 0xFFFE;
    m_iDist      = 0xFFFE;
    m_iBoth      = 0xFFFE;
    m_pCellFrom  = NULL;
    m_pCellNext  = NULL;
    m_pcNextBoth = NULL;
    m_pcPrevBoth = NULL;
    m_iBothIn    = 0;
    m_bClosed    = 0;
}

CCell::CCell( int iX, int iY )
{
    m_iX         = iX;
    m_iY         = iY;
    m_iCost      = 0xFFFE;
    m_iDist      = 0xFFFE;
    m_iBoth      = 0xFFFE;
    m_pCellFrom  = NULL;
    m_pCellNext  = NULL;
    m_pcNextBoth = NULL;
    m_pcPrevBoth = NULL;
    m_iBothIn    = 0;
    m_bClosed    = 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  Pathfinding ladder, step C: the one entry a worker thread uses.
//
//  Defined here, after the template bodies, and taking no vehicle: with
//  USE_HEADINGS == 0 the only thing a search does with pVehicle is take
//  m_pTD from it, and view.GetTransportData( iVehType ) returns the very same
//  CTransportData* for the index theTransports.GetIndex( veh->GetData() )
//  hands back. The static_assert makes enabling headings a build failure here
//  rather than a vehicle routed with no heading.
//
////////////////////////////////////////////////////////////////////////////

void CPathMgr::SearchSnapshot( PathWorld const& pw, CHexCoord hexFrom, CHexCoord hexTo, int iVehType,
                               BOOL bVehBlock, BOOL bDirectPath, SNAPSEARCH& out )
{
    static_assert( USE_HEADINGS == 0, "a headings search needs the vehicle; a worker has no live vehicle" );

    out = SNAPSEARCH( );
    if ( m_paCells == NULL || !m_bCsInit )
        return;

    // _GetPath takes non-const references and adjusts them (AdjustDestination), so it
    // gets its own copies and the caller's request is left as it was made.
    CHexCoord hexF( hexFrom );
    CHexCoord hexT( hexTo );
    int       iLen = 0;

    EnterCriticalSection( &m_cs );
    try
    {
        const CEnSnapNavView view( pw );
        out.phexPath = _GetPath( view, NULL, hexF, hexT, iLen, iVehType, bVehBlock, bDirectPath );
        out.iPathLen = iLen;
#if EN_PATH_PROBES
        out.iClass    = m_iProbeOutcome;
        out.bCapArena = m_bProbeCapArena;
        out.bCapIter  = m_bProbeCapIter;
#endif
    }
    catch ( ... )
    {
        // CreateHexPath's new CHexCoord[] is the one allocation in here. A throw must
        // not escape onto a worker thread, where it would take the process down.
        delete[] out.phexPath;
        out.phexPath = NULL;
        out.iPathLen = 0;
    }
    LeaveCriticalSection( &m_cs );
}

#if EN_PATH_PROBES

////////////////////////////////////////////////////////////////////////////
//
//  Pathfinding ladder, step A: the private shadow instance.
//
//  Nothing here changes what the game does. One extra CPathMgr runs the same
//  search over the same inputs right after the production one and the two
//  answers are compared. It exists to answer, with numbers rather than
//  argument, whether CPathMgr's search is self-contained enough to be moved
//  off the main thread - i.e. whether a second instance, fed identical
//  inputs, produces an identical route.
//
//  Everything below is inside EN_PATH_PROBES, and inside that, inert unless
//  EN_PATH_SHADOW names something.
//
////////////////////////////////////////////////////////////////////////////

// A pointer, not a second file-scope object. Three reasons: with EN_PATH_SHADOW
// unset nothing is constructed at all (a global would cost the m_acBoth array -
// 32 KB - in every build with probes compiled in, plus a second CCell arena's
// worth of bookkeeping); there is no static-initialisation-order question against
// thePathMgr; and "created at Init, destroyed at Close" is then literal instead of
// implied by a flag.
static CPathMgr* g_pPathShadow = NULL;

// Effective setting, resolved once. A VALUE is required, not just presence:
// EN_PATH_SHADOW=0 (and =false, =off, =no, or anything unrecognised) means OFF.
// Treating any non-empty string as ON made the obvious way to turn the shadow off
// turn it on instead.
static int s_iShadowOn      = -1;   // -1 = not resolved yet, 0 = off, 1 = on
static int s_bShadowAsked   = 0;    // EN_PATH_SHADOW was present in the environment

static void ResolveShadowSwitch( void )
{
    if ( s_iShadowOn >= 0 )
        return;

    const char* pszEnv = getenv( "EN_PATH_SHADOW" );
    s_bShadowAsked     = ( pszEnv != NULL && pszEnv[0] != '\0' ) ? 1 : 0;
    s_iShadowOn        = 0;
    if ( !s_bShadowAsked )
        return;

    // Accept 1 / true / on / yes, case-insensitively. Everything else is off.
    char sz[8];
    int  i = 0;
    for ( ; i < (int)sizeof( sz ) - 1 && pszEnv[i] != '\0'; ++i )
        sz[i] = (char)tolower( (unsigned char)pszEnv[i] );
    sz[i] = '\0';

    if ( strcmp( sz, "1" ) == 0 || strcmp( sz, "true" ) == 0 || strcmp( sz, "on" ) == 0 ||
         strcmp( sz, "yes" ) == 0 )
        s_iShadowOn = 1;
}

BOOL EnPathShadowOn( void )
{
    ResolveShadowSwitch( );
    return ( s_iShadowOn ? TRUE : FALSE );
}

// One line per shadow event, in the process working directory beside perf.log.
// Opened SHARED: EN_WAIT_LOG learned that an exclusive open makes a probe log
// unreadable while the game is running, which is exactly when it is wanted.
// EN_PATH_SHADOW_LOG renames the file.
static FILE* s_pShadowLog      = NULL;
static int   s_iShadowLogTried = 0;

static void ShadowLog( const char* pszFmt, ... )
{
    if ( !s_iShadowLogTried )
    {
        s_iShadowLogTried   = 1;
        const char* pszPath = getenv( "EN_PATH_SHADOW_LOG" );
        if ( pszPath == NULL || pszPath[0] == '\0' )
            pszPath = "pathshadow.log";
#ifdef _WIN32
        s_pShadowLog = _fsopen( pszPath, "w", _SH_DENYNO );
#else
        s_pShadowLog = fopen( pszPath, "w" );
#endif
    }
    if ( s_pShadowLog == NULL )
        return;

    va_list va;
    va_start( va, pszFmt );
    vfprintf( s_pShadowLog, pszFmt, va );
    va_end( va );
    fputc( '\n', s_pShadowLog );
    fflush( s_pShadowLog );
}

static const char* ShadowClassName( int iOutcome )
{
    switch ( iOutcome )
    {
    case CPathMgr::po_trivial: return ( "trivial" );
    case CPathMgr::po_ok:      return ( "ok" );
    case CPathMgr::po_clamped: return ( "clamped" );
    case CPathMgr::po_nopath:  return ( "nopath" );
    case CPathMgr::po_blocked: return ( "blocked" );
    default:                   return ( "none" );
    }
}

// "none", "arena", "iter", or "arena+iter" if a search ever managed both.
static const char* ShadowCapName( BOOL bArena, BOOL bIter )
{
    if ( bArena && bIter ) return ( "arena+iter" );
    if ( bArena )          return ( "arena" );
    if ( bIter )           return ( "iter" );
    return ( "none" );
}

// A route as "x,y x,y ...", capped so one pathological path cannot fill the log. The
// cap is reported in the line itself (+N more) rather than silently truncating.
enum { kShadowRouteCap = 128 };

static void ShadowRouteStr( char* pszOut, size_t cbOut, CHexCoord const* pHex, int iLen )
{
    if ( cbOut == 0 )
        return;
    pszOut[0] = '\0';
    if ( pHex == NULL )
    {
        strncpy( pszOut, "(null)", cbOut - 1 );
        pszOut[cbOut - 1] = '\0';
        return;
    }

    size_t    cb    = 0;
    const int iShow = ( iLen > (int)kShadowRouteCap ) ? (int)kShadowRouteCap : iLen;
    for ( int i = 0; i < iShow; ++i )
    {
        char sz[32];
        sprintf( sz, "%s%d,%d", i ? " " : "", pHex[i].X( ), pHex[i].Y( ) );
        const size_t cbAdd = strlen( sz );
        if ( cb + cbAdd + 1 >= cbOut )
            break;
        memcpy( pszOut + cb, sz, cbAdd + 1 );
        cb += cbAdd;
    }
    if ( iShow < iLen )
    {
        char sz[32];
        sprintf( sz, " +%d more", iLen - iShow );
        const size_t cbAdd = strlen( sz );
        if ( cb + cbAdd + 1 < cbOut )
            memcpy( pszOut + cb, sz, cbAdd + 1 );
    }
}

void EnPathShadowInit( int iMapEX, int iMapEY )
{
    ResolveShadowSwitch( );

    // Say which arm ran, once, so a tester never has to infer it from counter
    // presence. Only when the variable was actually SET: an untouched environment
    // must not cause a log file to appear at all.
    if ( s_bShadowAsked )
        ShadowLog( "mpath.shadow: EN_PATH_SHADOW effective=%s", s_iShadowOn ? "on" : "off" );

    if ( !s_iShadowOn )
        return;

    // A failure to build the shadow must never cost the game a path. Construction and
    // Init() both allocate (CCell arena, hash table); on any failure the instance is
    // dropped and every later call counts mpath.shadow.skipped instead of comparing.
    try
    {
        if ( g_pPathShadow == NULL )
        {
            g_pPathShadow = new CPathMgr( );
            g_pPathShadow->MarkShadow( );   // before its first search, and never unset
        }

        // Same dimensions as thePathMgr, from the same call site, so the arena, the
        // iHang budget and m_iMapEX/m_iMapEY all match. A different Init() would make
        // any route difference meaningless.
        g_pPathShadow->Init( iMapEX, iMapEY );
    }
    catch ( ... )
    {
        delete g_pPathShadow;
        g_pPathShadow = NULL;
        ShadowLog( "[shadow] init FAILED to allocate for map=%dx%d - shadow disabled for this world, "
                   "production unaffected",
                   iMapEX, iMapEY );
        return;
    }

    // SUBTOTAL, not the instance's full retained footprint. It covers the two fixed
    // allocations Init() makes that are sized by the map - the CCell arena and the
    // m_iBoth index array. It does NOT include the CMap hash table's own bucket array,
    // its per-node CCell* entries, or the EnPoolAllocator free list those nodes are
    // recycled through (that pool is per-THREAD and shared with the production
    // instance, so it cannot be attributed to one instance anyway). Bucket count is
    // printed so the bucket array can be sized separately if step B needs the total.
    const int    iCells   = ( iMapEX + iMapEY ) * 5;
    const int    iBuckets = GetPrime( iCells + 1 );
    const double dArenaKB = (double)( (size_t)iCells * sizeof( CCell ) ) / 1024.0;
    const double dBothKB  = (double)( ( MAX_BOTH_INDEX + 1 ) * sizeof( CCell* ) ) / 1024.0;
    ShadowLog( "[shadow] init map=%dx%d  cells=%d  cellSize=%d  arena=%.1f KB  "
               "bothIndex=%d entries (%.1f KB)  hashBuckets=%d (bucket array, map nodes and the "
               "per-thread node pool NOT included)  subtotal=%.1f KB",
               iMapEX, iMapEY, iCells, (int)sizeof( CCell ), dArenaKB,
               MAX_BOTH_INDEX + 1, dBothKB, iBuckets, dArenaKB + dBothKB );
}

void EnPathShadowClose( void )
{
    if ( g_pPathShadow == NULL )
        return;

    g_pPathShadow->Close( );
    delete g_pPathShadow;
    g_pPathShadow = NULL;
}

////////////////////////////////////////////////////////////////////////////
//
//  Pathfinding ladder, step C: the worker comparison.
//
//  A worker's answer is compared against the SAME-SNAPSHOT reference the
//  shadow computed for the same request on the main thread. Comparing it
//  against the live-world answer instead would fold snapshot staleness into
//  the number and make a clean result unprovable.
//
//  Everything here is main-thread only: the submit runs inside ShadowGetPath,
//  the drain at the snapshot publication point in mainloop.cpp. The workers
//  touch none of it.
//
////////////////////////////////////////////////////////////////////////////

namespace
{
// The reference answer for one outstanding request. The route is OUR copy: the
// shadow frees its own the moment ShadowGetPath returns.
struct WORKERREF
{
    uint32_t   uGameGen;
    CHexCoord  hexFrom;
    CHexCoord  hexTo;
    int        iVehType;
    BOOL       bVehBlock;
    BOOL       bDirect;
    uint64_t   uEpoch;
    CHexCoord* phexPath;
    int        iPathLen;
    int        iClass;
    BOOL       bCapArena;
    BOOL       bCapIter;

    WORKERREF( )
        : uGameGen( 0 ), hexFrom( 0, 0 ), hexTo( 0, 0 ), iVehType( 0 ), bVehBlock( FALSE ), bDirect( FALSE ),
          uEpoch( 0 ), phexPath( NULL ), iPathLen( 0 ), iClass( 0 ), bCapArena( FALSE ), bCapIter( FALSE )
    {
    }
};
}  // namespace

// Ordered by requestId, so evicting "the oldest" on overflow is begin() and needs no
// second structure.
static std::map<uint64_t, WORKERREF> g_mapWorkerRef;

enum { kWorkerRefCap = 4096 };

static void WorkerRefFree( WORKERREF& ref )
{
    delete[] ref.phexPath;
    ref.phexPath = NULL;
    ref.iPathLen = 0;
}

void EnPathWorkerRefsClear( void )
{
    for ( std::map<uint64_t, WORKERREF>::iterator it = g_mapWorkerRef.begin( ); it != g_mapWorkerRef.end( ); ++it )
        WorkerRefFree( it->second );
    g_mapWorkerRef.clear( );
}

static void EnPathWorkerSubmit( CVehicle* pVehicle, CHexCoord const& hexFrom, CHexCoord const& hexTo,
                                BOOL bVehBlock, BOOL bDirectPath,
                                std::shared_ptr<const PathWorld> const& ptrSnap, CHexCoord const* phexRef,
                                CPathMgr::VERDICT const& vRef )
{
    if ( !thePathService.IsRunning( ) )
        return;

    // Step D owns the queue when it is on: it submits from the mover itself and
    // installs the answer. A shadow submit beside it would put results in the same
    // queue that the install drain would reject and free.
    if ( PathService::AsyncEnabled( ) )
        return;

    // The mover, on a snapshot, with occupancy out of scope - the v1 eligibility of
    // plan section 1.2 D1/D3. Everything else keeps its synchronous answer and is
    // counted so the submit rate is never inferred from a silence.
    if ( pVehicle == NULL || bVehBlock || !ptrSnap )
    {
        Perf::CounterInc( "pq.nosubmit" );
        return;
    }

    PathRequest req;
    req.gameGeneration = EnNavGameGeneration( );
    req.vehicleId      = (uint32_t)pVehicle->GetID( );
    req.orderGeneration = (uint32_t)pVehicle->GetOrderGen( );
    req.from           = hexFrom;
    req.to             = hexTo;
    // NOT GetData()->GetType(): TRANS_TYPE is the unit's kind, the index is its slot in
    // theTransports, and GetTransportData() takes the slot. tests/path/run-pathservice.py
    // pins GetData( GetIndex( p ) ) == p for every transport.
    req.iVehType    = theTransports.GetIndex( pVehicle->GetData( ) );
    req.bVehBlock   = bVehBlock;
    req.bDirectPath = bDirectPath;
    req.tSubmit     = Perf::NowIfEnabled( );
    req.world       = ptrSnap;

    const uint64_t uId = thePathService.Submit( req );
    if ( uId == 0 )
    {
        Perf::CounterInc( "pq.nosubmit" );
        return;
    }

    WORKERREF ref;
    ref.uGameGen  = req.gameGeneration;
    ref.hexFrom   = hexFrom;
    ref.hexTo     = hexTo;
    ref.iVehType  = req.iVehType;
    ref.bVehBlock = bVehBlock;
    ref.bDirect   = bDirectPath;
    ref.uEpoch    = ptrSnap->Epoch( );
    ref.iPathLen  = vRef.iPathLen;
    ref.iClass    = vRef.iClass;
    ref.bCapArena = vRef.bCapArena;
    ref.bCapIter  = vRef.bCapIter;
    if ( phexRef != NULL && vRef.iPathLen > 0 )
    {
        ref.phexPath = new CHexCoord[vRef.iPathLen];
        for ( int i = 0; i < vRef.iPathLen; ++i )
            ref.phexPath[i] = phexRef[i];
    }
    else if ( phexRef != NULL )
    {
        // A non-NULL route of reported length 0 is itself a difference worth keeping,
        // so record that it was non-NULL without copying anything.
        ref.phexPath = new CHexCoord[1];
    }

    // Never block the main thread and never grow without bound: the oldest outstanding
    // reference goes, and the result that eventually arrives for it counts as dropped.
    while ( g_mapWorkerRef.size( ) >= (size_t)kWorkerRefCap )
    {
        WorkerRefFree( g_mapWorkerRef.begin( )->second );
        g_mapWorkerRef.erase( g_mapWorkerRef.begin( ) );
        Perf::CounterInc( "pq.overflow" );
    }

    g_mapWorkerRef[uId] = ref;
}

void EnPathWorkerDrain( void )
{
    PathResult res;
    while ( thePathService.PopResult( res ) )
    {
        std::map<uint64_t, WORKERREF>::iterator it = g_mapWorkerRef.find( res.requestId );
        if ( it == g_mapWorkerRef.end( ) || res.gameGeneration != EnNavGameGeneration( ) )
        {
            // Evicted by the cap, or answered for a world that no longer exists.
            Perf::CounterInc( "pq.dropped" );
            if ( it != g_mapWorkerRef.end( ) )
            {
                WorkerRefFree( it->second );
                g_mapWorkerRef.erase( it );
            }
            PathService::FreeResult( res );
            continue;
        }

        WORKERREF& ref = it->second;
        Perf::CounterInc( "pq.cmp" );

        // Same equality as the shadow's, MINUS the clamp hex: the clamp is written
        // through pVehicle, the worker has no vehicle, and the clamped destination is
        // the last hex of a clamped route anyway - so the route comparison already
        // carries it.
        BOOL bEqual     = TRUE;
        int  iFirstDiff = -1;
        if ( ( ref.phexPath == NULL ) != ( res.path == NULL ) )
            bEqual = FALSE;
        if ( ref.iPathLen != res.pathLen )
            bEqual = FALSE;
        if ( ref.phexPath != NULL && res.path != NULL )
        {
            const int iCommon = __min( ref.iPathLen, res.pathLen );
            for ( int i = 0; i < iCommon; ++i )
                if ( ref.phexPath[i] != res.path[i] )
                {
                    bEqual     = FALSE;
                    iFirstDiff = i;
                    break;
                }
        }
        if ( ref.iClass != res.exitClass )
            bEqual = FALSE;
        if ( ref.bCapArena != res.bCapArena || ref.bCapIter != res.bCapIter )
            bEqual = FALSE;

        if ( !bEqual )
        {
            Perf::CounterInc( "pq.diff" );

            static int s_iWorkerDiffLogged = 0;
            if ( s_iWorkerDiffLogged < 64 )
            {
                ++s_iWorkerDiffLogged;
                static char s_szRef[1400];
                static char s_szWrk[1400];
                ShadowRouteStr( s_szRef, sizeof( s_szRef ), ref.phexPath, ref.iPathLen );
                ShadowRouteStr( s_szWrk, sizeof( s_szWrk ), res.path, res.pathLen );

                ShadowLog( "[worker] DIFF #%d  req=%llu veh=%lu ordGen=%lu  from=%d,%d to=%d,%d  vehType=%d  "
                           "refLen=%d wrkLen=%d  refNull=%d wrkNull=%d  refClass=%s wrkClass=%s  "
                           "refCap=%s wrkCap=%s  firstDiff=%d  direct=%d vehBlock=%d  "
                           "refEpoch=%llu wrkEpoch=%llu%s%s%s%s",
                           s_iWorkerDiffLogged, (unsigned long long)res.requestId,
                           (unsigned long)res.vehicleId, (unsigned long)res.orderGeneration, ref.hexFrom.X( ),
                           ref.hexFrom.Y( ), ref.hexTo.X( ), ref.hexTo.Y( ), ref.iVehType, ref.iPathLen,
                           res.pathLen, ref.phexPath == NULL ? 1 : 0, res.path == NULL ? 1 : 0,
                           ShadowClassName( ref.iClass ), ShadowClassName( res.exitClass ),
                           ShadowCapName( ref.bCapArena, ref.bCapIter ),
                           ShadowCapName( res.bCapArena, res.bCapIter ), iFirstDiff, ref.bDirect ? 1 : 0,
                           ref.bVehBlock ? 1 : 0, (unsigned long long)ref.uEpoch,
                           (unsigned long long)res.worldEpoch, "\n  ref:    ", s_szRef, "\n  worker: ", s_szWrk );
            }
        }

        WorkerRefFree( ref );
        g_mapWorkerRef.erase( it );
        PathService::FreeResult( res );
    }

    Perf::GaugeSet( "pq.depth", thePathService.QueueDepth( ) );
}

//
// Run the production search, then the shadow search, compare, and hand the caller
// the PRODUCTION answer and nothing else.
//
CHexCoord* CPathMgr::ShadowGetPath( CVehicle* pVehicle, CHexCoord& hexFrom, CHexCoord& hexTo, int& iPathLen,
                                    int iVehType, BOOL bVehBlock, BOOL bDirectPath )
{
    // Snapshot the request as it was MADE. _GetPath does not write through these
    // references today (it copies into m_hexFrom/m_hexTo and adjusts those), but the
    // shadow must be fed the original request, not whatever production left behind.
    CHexCoord hexShFrom = hexFrom;
    CHexCoord hexShTo   = hexTo;

    // @WinAstra's pre-call state rule. The one piece of per-vehicle state _GetPath
    // writes is the re-clamp watch (vehicle.h m_hexLastClamp, probe-only). Save it
    // BEFORE production, rewind to it for the shadow so the shadow sees the vehicle
    // production saw, then put production's result back. The vehicle must end this
    // call exactly as it ends it with the shadow switched off.
    const BOOL bVeh = ( pVehicle != NULL );
    CHexCoord  hexClampPre;
    if ( bVeh )
        hexClampPre = pVehicle->m_hexLastClamp;

    // Arm the verdict slot. GetPathProd fills it INSIDE m_cs and clears the pointer;
    // reading m_iProbeOutcome / the cap flags after the lock is released would race an
    // AI worker's next search on this same instance.
    VERDICT vProd;
    t_pPathVerdict = &vProd;

    CHexCoord* phcProd = GetPathProd( pVehicle, hexFrom, hexTo, iPathLen, iVehType, bVehBlock, bDirectPath );

    t_pPathVerdict = NULL;   // belt: GetPathProd always clears it, but never leave it armed

    if ( bVeh )
        pVehicle->m_hexLastClamp = hexClampPre;   // rewind for the shadow

    if ( g_pPathShadow == NULL || g_pPathShadow->m_paCells == NULL )
    {
        // The switch is on but there is no usable shadow (allocation failed, or the
        // world was Closed under us). Say so out loud - a missing comparison must not
        // read as a comparison that passed.
        Perf::CounterInc( "mpath.shadow.skipped" );
    }
    else
    {
        int        iShLen    = 0;
        CHexCoord* phcShadow = NULL;
        VERDICT    vSh;
        BOOL       bRan      = FALSE;

        // Which world the shadow reads. v1 scope: only bVehBlock == FALSE searches go
        // through the snapshot - those are the ones that never consult vehicle
        // occupancy except in the one-step destination check at the very end. A
        // snapshot from a previous world is not a snapshot of this one, whatever its
        // epoch says, so the generation must match too.
        std::shared_ptr<const PathWorld> ptrSnap;
        if ( PathWorld::Enabled( ) )
        {
            if ( bVehBlock )
                Perf::CounterInc( "mpath.shadow.snap.skipveh" );
            else
            {
                ptrSnap = PathWorld::Current( );
                if ( ptrSnap && ( ptrSnap->Generation( ) != EnNavGameGeneration( ) ) )
                    ptrSnap.reset( );
            }
        }

        // The two epochs AS OF THIS SEARCH, read once, before the shadow runs. epochAge
        // is how many world writes landed since the snapshot was built - 0 means the
        // snapshot IS this world and a diff cannot be blamed on its age.
        const uint64_t uWorldEpoch = g_enNavEpoch;
        const uint64_t uSnapEpoch  = ptrSnap ? ptrSnap->Epoch( ) : 0;
        const uint64_t uEpochAge   = ptrSnap ? ( uWorldEpoch - uSnapEpoch ) : 0;
        const BOOL     bStaleSnap  = ( ptrSnap && ( uSnapEpoch != uWorldEpoch ) ) ? TRUE : FALSE;

        // The SHADOW's own section - never this instance's, which GetPathProd has
        // already taken and released. Held across the whole private search so the
        // second instance is serialised exactly as the first one is, and its verdict
        // is read before the lock drops. Lock order is only ever production-then-
        // shadow, so there is no cycle.
        EnterCriticalSection( &g_pPathShadow->m_cs );
        try
        {
            if ( ptrSnap )
            {
                const CEnSnapNavView viewSnap( *ptrSnap );
                phcShadow = g_pPathShadow->_GetPath( viewSnap, pVehicle, hexShFrom, hexShTo, iShLen, iVehType,
                                                     bVehBlock, bDirectPath );
            }
            else
            {
                const CEnLiveNavView viewLive;
                phcShadow = g_pPathShadow->_GetPath( viewLive, pVehicle, hexShFrom, hexShTo, iShLen, iVehType,
                                                     bVehBlock, bDirectPath );
            }
            vSh.iClass     = g_pPathShadow->m_iProbeOutcome;
            vSh.bCapArena  = g_pPathShadow->m_bProbeCapArena;
            vSh.bCapIter   = g_pPathShadow->m_bProbeCapIter;
            vSh.iPathLen   = iShLen;
            vSh.bHaveClamp = bVeh;
            if ( bVeh )
                vSh.hexClamp = pVehicle->m_hexLastClamp;
            bRan = TRUE;
        }
        catch ( ... )
        {
            // The shadow's own allocation (CreateHexPath's new CHexCoord[]) or anything
            // else it throws must not reach the caller: production already has its
            // answer and it is still valid.
            phcShadow = NULL;
        }
        LeaveCriticalSection( &g_pPathShadow->m_cs );

        if ( !bRan )
        {
            Perf::CounterInc( "mpath.shadow.skipped" );
        }
        else
        {
            Perf::CounterInc( "mpath.shadow.cmp" );
            if ( ptrSnap )
            {
                Perf::CounterInc( "mpath.shadow.snap" );
                // The world moved between the snapshot and this search, so a diff on
                // this comparison may be the age and not the view. Counted, never
                // excused: the diff still lands in mpath.shadow.diff.
                if ( bStaleSnap )
                    Perf::CounterInc( "mpath.shadow.stale" );
            }

            // Coverage, by the PRODUCTION class: a diff of 0 means nothing until these
            // show the comparison actually visited each exit.
            switch ( vProd.iClass )
            {
            case po_ok:      Perf::CounterInc( "mpath.shadow.ok" );      break;
            case po_clamped: Perf::CounterInc( "mpath.shadow.clamped" ); break;
            case po_nopath:  Perf::CounterInc( "mpath.shadow.nopath" );  break;
            case po_trivial: Perf::CounterInc( "mpath.shadow.trivial" ); break;
            case po_blocked: Perf::CounterInc( "mpath.shadow.blocked" ); break;
            default:         Perf::CounterInc( "mpath.shadow.unclassed" ); break;
            }
            if ( vProd.bCapArena ) Perf::CounterInc( "mpath.shadow.cap.arena" );
            if ( vProd.bCapIter )  Perf::CounterInc( "mpath.shadow.cap.iter" );

            // Equality is EVERY promised output, not just the hexes:
            //   - both NULL or both non-NULL
            //   - the reported iPathLen, raw, exactly as each handed it back (NOT
            //     normalised to 0 on a NULL route: a NULL route that leaves a stale
            //     non-zero length behind is itself a difference worth seeing)
            //   - every hex, for all iPathLen entries
            //   - the exit class
            //   - the termination reason (arena cap vs iteration cap vs neither)
            //   - the vehicle's clamp post-state
            BOOL bEqual     = TRUE;
            int  iFirstDiff = -1;
            if ( ( phcProd == NULL ) != ( phcShadow == NULL ) )
                bEqual = FALSE;
            if ( vProd.iPathLen != vSh.iPathLen )
                bEqual = FALSE;
            // firstDiff is over the COMMON PREFIX, so two routes of different length
            // still say where they parted; -1 now means "the common prefix is
            // identical" and nothing else.
            if ( phcProd != NULL && phcShadow != NULL )
            {
                const int iCommon = __min( vProd.iPathLen, vSh.iPathLen );
                for ( int i = 0; i < iCommon; ++i )
                    if ( phcProd[i] != phcShadow[i] )
                    {
                        bEqual     = FALSE;
                        iFirstDiff = i;
                        break;
                    }
            }
            if ( vProd.iClass != vSh.iClass )
                bEqual = FALSE;
            if ( vProd.bCapArena != vSh.bCapArena || vProd.bCapIter != vSh.bCapIter )
                bEqual = FALSE;
            if ( vProd.bHaveClamp && vSh.bHaveClamp && vProd.hexClamp != vSh.hexClamp )
                bEqual = FALSE;

            if ( !bEqual )
            {
                // The one-step occupied-destination check at the foot of _GetPath is
                // the one place a bVehBlock == FALSE search reads vehicle bits, so on
                // the snapshot it reads snapshot bits. Its whole effect is turning a
                // length-1 route into NULL, which is exactly this shape. The plan's
                // install-time rule re-checks that hex live anyway, so it is counted
                // apart rather than mixed into the view-equivalence number.
                const BOOL bOneStep = ( ( vProd.iPathLen == 1 && phcProd != NULL && vSh.iPathLen == 0 &&
                                          phcShadow == NULL ) ||
                                        ( vSh.iPathLen == 1 && phcShadow != NULL && vProd.iPathLen == 0 &&
                                          phcProd == NULL ) ) &&
                                      ( vProd.bCapArena == vSh.bCapArena ) && ( vProd.bCapIter == vSh.bCapIter );
                Perf::CounterInc( bOneStep ? "mpath.shadow.diff.onestep" : "mpath.shadow.diff" );

                static int s_iDiffLogged = 0;   // main-thread only, like the rest of this
                if ( s_iDiffLogged < 64 )
                {
                    ++s_iDiffLogged;
                    const int iType = ( bVeh && pVehicle->GetData( ) != NULL )
                                          ? (int)pVehicle->GetData( )->GetType( )
                                          : iVehType;
                    // Only a real diff gets the two routes; the onestep shape is already
                    // understood and would only pad the log.
                    static char s_szProd[1400];
                    static char s_szSh[1400];
                    if ( bOneStep )
                    {
                        s_szProd[0] = '\0';
                        s_szSh[0]   = '\0';
                    }
                    else
                    {
                        ShadowRouteStr( s_szProd, sizeof( s_szProd ), phcProd, vProd.iPathLen );
                        ShadowRouteStr( s_szSh, sizeof( s_szSh ), phcShadow, vSh.iPathLen );
                    }

                    ShadowLog( "[shadow] DIFF #%d  from=%d,%d to=%d,%d  vehType=%d  "
                               "prodLen=%d shLen=%d  prodNull=%d shNull=%d  prodClass=%s shClass=%s  "
                               "prodCap=%s shCap=%s  prodClamp=%d,%d shClamp=%d,%d  "
                               "firstDiff=%d  direct=%d vehBlock=%d  "
                               "snap=%d stale=%d epochAge=%llu snapEpoch=%llu worldEpoch=%llu%s%s%s%s",
                               s_iDiffLogged, hexShFrom.X( ), hexShFrom.Y( ), hexShTo.X( ), hexShTo.Y( ), iType,
                               vProd.iPathLen, vSh.iPathLen, phcProd == NULL ? 1 : 0, phcShadow == NULL ? 1 : 0,
                               ShadowClassName( vProd.iClass ), ShadowClassName( vSh.iClass ),
                               ShadowCapName( vProd.bCapArena, vProd.bCapIter ),
                               ShadowCapName( vSh.bCapArena, vSh.bCapIter ),
                               vProd.bHaveClamp ? vProd.hexClamp.X( ) : -1,
                               vProd.bHaveClamp ? vProd.hexClamp.Y( ) : -1,
                               vSh.bHaveClamp ? vSh.hexClamp.X( ) : -1,
                               vSh.bHaveClamp ? vSh.hexClamp.Y( ) : -1,
                               iFirstDiff, bDirectPath ? 1 : 0, bVehBlock ? 1 : 0,
                               ptrSnap ? 1 : 0, bStaleSnap ? 1 : 0,
                               (unsigned long long)uEpochAge, (unsigned long long)uSnapEpoch,
                               (unsigned long long)uWorldEpoch,
                               bOneStep ? "" : "\n  prod:   ", bOneStep ? "" : s_szProd,
                               bOneStep ? "" : "\n  shadow: ", bOneStep ? "" : s_szSh );
                }
            }

            // Step C. The shadow has just answered THIS request on THIS snapshot, on
            // the main thread; that answer is the reference. Hand the worker the same
            // shared_ptr, so the two searches read one identical world and a diff can
            // only be the thread, never the age of the world.
            EnPathWorkerSubmit( pVehicle, hexShFrom, hexShTo, bVehBlock, bDirectPath, ptrSnap, phcShadow, vSh );
        }

        // The shadow route is ours and nobody else's; free it here, exactly once.
        // The production route belongs to the caller and is never touched.
        if ( phcShadow != NULL )
            delete[] phcShadow;
    }

    // Put the vehicle back the way production left it, on every path out of here.
    if ( bVeh )
        pVehicle->m_hexLastClamp = vProd.bHaveClamp ? vProd.hexClamp : hexClampPre;

    return ( phcProd );
}

//
// Step D verification. At install time, run the SAME request synchronously against
// the LIVE world on the private shadow instance and compare it with the route about
// to be installed. This is what turns "the worker answers quickly" into "the worker
// answers correctly": an installed route must equal the route the synchronous search
// would have produced on identical inputs.
//
void CPathMgr::AsyncVerify( CVehicle* pVehicle, CHexCoord const& hexFrom, CHexCoord const& hexTo, BOOL bVehBlock,
                            CHexCoord const* phexInstall, int iInstallLen, BOOL bStale )
{
    if ( !PathService::AsyncVerifyEnabled( ) || pVehicle == NULL )
        return;
    if ( g_pPathShadow == NULL || g_pPathShadow->m_paCells == NULL )
        return;

    // The worker had no vehicle (USE_HEADINGS == 0), so neither does the reference:
    // a pVehicle here would also write the re-clamp watch this call must not disturb.
    const int  iVehType = theTransports.GetIndex( pVehicle->GetData( ) );
    CHexCoord  hexF( hexFrom );
    CHexCoord  hexT( hexTo );
    int        iRefLen  = 0;
    CHexCoord* phexRef  = NULL;

    EnterCriticalSection( &g_pPathShadow->m_cs );
    try
    {
        const CEnLiveNavView viewLive;
        phexRef = g_pPathShadow->_GetPath( viewLive, NULL, hexF, hexT, iRefLen, iVehType, bVehBlock, FALSE );
    }
    catch ( ... )
    {
        delete[] phexRef;
        phexRef = NULL;
        iRefLen = 0;
    }
    LeaveCriticalSection( &g_pPathShadow->m_cs );

    Perf::CounterInc( "pa.verify.cmp" );

    BOOL bEqual     = TRUE;
    int  iFirstDiff = -1;
    if ( ( phexRef == NULL ) != ( phexInstall == NULL ) )
        bEqual = FALSE;
    if ( iRefLen != iInstallLen )
        bEqual = FALSE;
    if ( phexRef != NULL && phexInstall != NULL )
    {
        const int iCommon = __min( iRefLen, iInstallLen );
        for ( int i = 0; i < iCommon; ++i )
            if ( phexRef[i] != phexInstall[i] )
            {
                bEqual     = FALSE;
                iFirstDiff = i;
                break;
            }
    }

    if ( !bEqual )
    {
        Perf::CounterInc( "pa.verify.diff" );
        if ( bStale )
            Perf::CounterInc( "pa.verify.diff.stale" );

        static int s_iAsyncDiffLogged = 0;
        if ( s_iAsyncDiffLogged < 64 )
        {
            ++s_iAsyncDiffLogged;
            static char s_szRef[1400];
            static char s_szIns[1400];
            ShadowRouteStr( s_szRef, sizeof( s_szRef ), phexRef, iRefLen );
            ShadowRouteStr( s_szIns, sizeof( s_szIns ), phexInstall, iInstallLen );

            ShadowLog( "[async] DIFF #%d  veh=%lu ordGen=%lu  from=%d,%d to=%d,%d  vehType=%d  "
                       "refLen=%d insLen=%d  refNull=%d insNull=%d  firstDiff=%d  vehBlock=%d  stale=%d%s%s%s%s",
                       s_iAsyncDiffLogged, (unsigned long)pVehicle->GetID( ),
                       (unsigned long)pVehicle->GetOrderGen( ), hexFrom.X( ), hexFrom.Y( ), hexTo.X( ), hexTo.Y( ),
                       iVehType, iRefLen, iInstallLen, phexRef == NULL ? 1 : 0, phexInstall == NULL ? 1 : 0,
                       iFirstDiff, bVehBlock ? 1 : 0, bStale ? 1 : 0, "\n  ref:       ", s_szRef,
                       "\n  installed: ", s_szIns );
        }
    }

    delete[] phexRef;
}

#endif  // EN_PATH_PROBES

#if !EN_PATH_PROBES
void CPathMgr::AsyncVerify( CVehicle*, CHexCoord const&, CHexCoord const&, BOOL, CHexCoord const*, int, BOOL )
{
}
#endif

// end of CPathMgr.cpp
