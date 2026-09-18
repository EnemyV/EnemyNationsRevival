////////////////////////////////////////////////////////////////////////////
//
//  pathworld.cpp : building and publishing the movement-A* world snapshot.
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "pathworld.h"

#include "stdafx.h"
#include "Perf.h"
#include "terrain.inl"
#include "building.inl"
#include "vehicle.inl"
#include "bridge.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unordered_map>

/////////////////////////////////////////////////////////////////////////////
// CPwIndex

void CPwIndex::Reserve( size_t nEntries )
{
    size_t nCap = 16;
    while ( nCap < ( nEntries * 2 + 1 ) ) nCap <<= 1;
    m_aKey.assign( nCap, (DWORD)none );
    m_aVal.assign( nCap, 0 );
    m_dwMask = (DWORD)( nCap - 1 );
}

void CPwIndex::Insert( DWORD dwKey, DWORD dwVal )
{
    DWORD dwAt = ( dwKey * 2654435761u ) & m_dwMask;
    while ( m_aKey[dwAt] != (DWORD)none )
    {
        if ( m_aKey[dwAt] == dwKey )
        {
            m_aVal[dwAt] = dwVal;
            return;
        }
        dwAt = ( dwAt + 1 ) & m_dwMask;
    }
    m_aKey[dwAt] = dwKey;
    m_aVal[dwAt] = dwVal;
}

DWORD CPwIndex::Find( DWORD dwKey ) const
{
    if ( m_aKey.empty( ) )
        return ( (DWORD)none );

    DWORD dwAt = ( dwKey * 2654435761u ) & m_dwMask;
    while ( m_aKey[dwAt] != (DWORD)none )
    {
        if ( m_aKey[dwAt] == dwKey )
            return ( m_aVal[dwAt] );
        dwAt = ( dwAt + 1 ) & m_dwMask;
    }
    return ( (DWORD)none );
}

/////////////////////////////////////////////////////////////////////////////
// generation

static uint32_t s_uGameGeneration = 1;

/////////////////////////////////////////////////////////////////////////////
// the dirty set
//
// Every mutation site that bumps g_enNavEpoch calls one of the four EnNavTouch*
// below (terrain.h); there is no other writer of the epoch. That is what makes the
// incremental build safe to reason about: an epoch bump and a dirty record are the
// same statement, so a bump can never arrive without the row it belongs to.

static PwDirty s_dirty;

// Hex indices the LAST build wrote a non-occ_none occupancy into. A full build starts
// from an all-occ_none array; an incremental one starts from a memcpy of the previous
// snapshot, which still carries that snapshot's occupancy, so these have to be cleared
// back to occ_none before the occupancy pass runs or a vehicle that has left a hex
// keeps blocking it. The list is one entry per OCCUPIED hex, so it is vehicle-sized.
static std::vector<uint32_t> s_aPrevOcc;

void EnNavNewGame( void )
{
    g_enNavEpoch = 0;
    ++s_uGameGeneration;
    s_dirty.All( );
    s_aPrevOcc.clear( );
    PathWorld::Clear( );
}

void EnNavTouchAll( void )
{
    ++g_enNavEpoch;
    s_dirty.All( );
}

void EnNavTouchTables( void )
{
    ++g_enNavEpoch;
    s_dirty.Tables( );
}

// Resolve the hex's row exactly the way CHex::GetHex resolves its coordinate, and fall
// back to All() whenever the pointer is not one of the live map's hexes - a stack
// temporary, or a write made before the map has hexes at all. An out-of-range row is
// the "unsure" case and is spelled as a full rebuild, never guessed at.
static void PwTouchRow( CHex const* pHex )
{
    // Already a full build - nothing a further row could add. This is also what keeps
    // world generation, which runs SetType/SetAlt over the whole map, from paying the
    // address arithmetic a million times.
    if ( s_dirty.IsAll( ) )
        return;

    if ( ( pHex == NULL ) || !theMap.HaveHexes( ) )
    {
        s_dirty.All( );
        return;
    }

    const ptrdiff_t iNum  = pHex - theMap._GetHex( 0, 0 );
    const ptrdiff_t nHex  = (ptrdiff_t)theMap.Get_eY( ) << theMap.GetSideShift( );
    if ( ( iNum < 0 ) || ( iNum >= nHex ) )
    {
        s_dirty.All( );
        return;
    }
    s_dirty.Row( (int)( iNum >> theMap.GetSideShift( ) ) );
}

void EnNavTouchHexAt( CHex const* pHex )
{
    ++g_enNavEpoch;
    PwTouchRow( pHex );
}

void EnNavTouchUnits( CHex const* pHex )
{
    ++g_enNavEpoch;
    PwTouchRow( pHex );
    // theBuildingHex/theBridgeHex are grabbed and released in the same statement as the
    // bldg/bridge bit (building.inl _GrabHex/_ReleaseHex, bridge.cpp GrabHex), so a bit
    // write is exactly when the fact tables can have moved.
    s_dirty.Tables( );
}

uint32_t EnNavGameGeneration( void )
{
    return ( s_uGameGeneration );
}

/////////////////////////////////////////////////////////////////////////////
// the published snapshot

static std::shared_ptr<const PathWorld> s_ptrCurrent;

std::shared_ptr<const PathWorld> PathWorld::Current( void )
{
    return ( std::atomic_load( &s_ptrCurrent ) );
}

void PathWorld::Publish( std::shared_ptr<const PathWorld> const& ptr )
{
    std::atomic_store( &s_ptrCurrent, ptr );
}

void PathWorld::Clear( void )
{
    std::atomic_store( &s_ptrCurrent, std::shared_ptr<const PathWorld>( ) );
}

// EN_PATH_SNAP must carry a VALUE: 1 / true / on / yes (any case) turn the
// per-tick build on. Absent, empty, or anything unrecognised leaves it off, so
// the obvious way to switch it off really does. Resolved once, then cached.
static int s_iSnapOn = -1;

BOOL PathWorld::Enabled( void )
{
    if ( s_iSnapOn < 0 )
    {
        s_iSnapOn              = 0;
        const char* pszEnv = getenv( "EN_PATH_SNAP" );
        if ( pszEnv != NULL && pszEnv[0] != '\0' )
        {
            char sz[8];
            int  i = 0;
            for ( ; i < (int)sizeof( sz ) - 1 && pszEnv[i] != '\0'; ++i )
                sz[i] = (char)tolower( (unsigned char)pszEnv[i] );
            sz[i] = '\0';
            if ( strcmp( sz, "1" ) == 0 || strcmp( sz, "true" ) == 0 || strcmp( sz, "on" ) == 0 ||
                 strcmp( sz, "yes" ) == 0 )
                s_iSnapOn = 1;
        }
    }
    return ( s_iSnapOn ? TRUE : FALSE );
}

/////////////////////////////////////////////////////////////////////////////
// Build

size_t PathWorld::Bytes( ) const
{
    return ( m_aHex.size( ) * sizeof( Hex ) + m_aBldg.size( ) * sizeof( Bldg ) +
             m_aBridge.size( ) * sizeof( Bridge ) + m_idxBldg.Bytes( ) + m_idxBridge.Bytes( ) + sizeof( PathWorld ) );
}

// One row of hexes, encoded from the live map. The ONLY place a snapshot hex record is
// written from a CHex, so a full build and an incremental one cannot encode differently.
static void PwEncodeRow( PathWorld::Hex* pOut, CHex const* pRow, int iWidth )
{
    for ( int x = 0; x < iWidth; ++x )
    {
        CHex const& hx = pRow[x];
        pOut[x].bUnits = hx.GetUnits( );
        pOut[x].bType  = (BYTE)hx.GetType( );
        pOut[x].bAlt   = (BYTE)hx.GetAlt( );
        pOut[x].bOcc   = PathWorld::occ_none;
    }
}

// Size and fill a snapshot's hex array, and return the number of rows re-encoded.
//
// Full build: resize (which zero-fills) and then encode every row off the live map.
// Incremental: copy-assign the previous snapshot's array - one allocation and one linear
// 4 MB copy, no zero-fill and no million four-field re-encodes through GetUnits /
// GetType / GetAlt - put the occupancy the previous build wrote back to occ_none, then
// re-encode just the rows the dirty set names.
//
// The two paths must come out byte-identical; that is what
// tests/path/run-pathworld-snapshot.py asserts, and it is why the occupancy reset lives
// here rather than in the occupancy pass. Occupancy is written after this and is
// deliberately outside the epoch, so the copied array still carries the PREVIOUS
// publish's occupancy while a from-scratch array starts blank: pPrevOcc names exactly
// the hexes that have to be put back, and it is vehicle-sized, not map-sized.
static int PwFillHexes( std::vector<PathWorld::Hex>& aOut, std::vector<PathWorld::Hex> const* pPrev,
                        uint32_t const* pPrevOcc, size_t nPrevOcc, int iHeight, int iWidth, int iSideShift,
                        PwDirty const& dirty )
{
    if ( ( pPrev == NULL ) || dirty.IsAll( ) )
    {
        aOut.resize( (size_t)iHeight << iSideShift );
        for ( int y = 0; y < iHeight; ++y )
            PwEncodeRow( &aOut[(size_t)y << iSideShift], theMap._GetHex( 0, y ), iWidth );
        return ( iHeight );
    }

    aOut = *pPrev;
    for ( size_t i = 0; i < nPrevOcc; ++i ) aOut[pPrevOcc[i]].bOcc = PathWorld::occ_none;

    int iRows = 0;
    for ( int y = 0; y < iHeight; ++y )
        if ( dirty.IsRow( y ) )
        {
            PwEncodeRow( &aOut[(size_t)y << iSideShift], theMap._GetHex( 0, y ), iWidth );
            ++iRows;
        }
    return ( iRows );
}

std::shared_ptr<const PathWorld> PathWorld::Build( void )
{
    PwDirty dirtyAll;   // default-constructed is IsAll()
    return ( _Build( NULL, dirtyAll, NULL ) );
}

std::shared_ptr<const PathWorld> PathWorld::_Build( PathWorld const* pPrev, PwDirty const& dirty, int* piRows )
{
    if ( !theMap.HaveHexes( ) )
        return ( std::shared_ptr<const PathWorld>( ) );

    std::shared_ptr<PathWorld> ptr( new PathWorld );
    PathWorld&                 pw = *ptr;

    pw.m_iWidth       = theMap.Get_eX( );
    pw.m_iHeight      = theMap.Get_eY( );
    pw.m_iHexMask     = theMap.GetHexMask( );
    pw.m_iWidthHalf   = theMap.GetWidthHalf( );
    pw.m_iSideShift   = theMap.GetSideShift( );
    pw.m_iTrafficRules = TrafficOpts( );
    pw.m_uEpoch       = g_enNavEpoch;
    pw.m_uGeneration  = s_uGameGeneration;

    // The terrain wheel-mult table is loaded with the gameplay data and never
    // written after that, but it is copied rather than pinned: it is 60 ints, and a
    // copy is one less global a worker thread has to be reasoned about.
    for ( int t = 0; t < CHex::num_types; ++t )
        for ( int w = 0; w < NUM_WHEEL_TYPES; ++w ) pw.m_aiWheelMult[t][w] = theTerrain.GetData( t ).GetWheelMult( w );

    // A previous snapshot of a DIFFERENT map is not a base this one can be copied from.
    if ( ( pPrev != NULL ) && ( ( pPrev->m_iWidth != pw.m_iWidth ) || ( pPrev->m_iHeight != pw.m_iHeight ) ||
                                ( pPrev->m_iSideShift != pw.m_iSideShift ) ) )
        pPrev = NULL;

    const uint64_t _qHex = Perf::NowIfEnabled( );

    // The row pitch is ( y << m_iSideShift ) + x on both sides, so a row is contiguous
    // in the live array too: PwEncodeRow takes the row base once and walks it, instead
    // of paying _GetHex's per-hex index arithmetic (and, in a Debug build, its two range
    // asserts and the strict-valid check) a million times. Same reads, same values.
    const int iRows = PwFillHexes( pw.m_aHex, ( pPrev != NULL ) ? &pPrev->m_aHex : NULL,
                                   s_aPrevOcc.empty( ) ? NULL : &s_aPrevOcc[0], s_aPrevOcc.size( ), pw.m_iHeight,
                                   pw.m_iWidth, pw.m_iSideShift, dirty );
    if ( piRows != NULL )
        *piRows = iRows;

    Perf::CounterAddElapsedUs( "pw.build.hex.us", _qHex );
    const uint64_t _qTbl = Perf::NowIfEnabled( );

    // The two fact tables move only when a bldg/bridge bit is written or a bridge's
    // exit/built state changes, all of which go through EnNavTouchUnits/EnNavTouchTables;
    // otherwise they are carried over from the previous snapshot unchanged. They are
    // value types (vectors and two open-addressed DWORD arrays), so a copy is a copy.
    if ( ( pPrev != NULL ) && !dirty.IsTables( ) )
    {
        pw.m_aBldg     = pPrev->m_aBldg;
        pw.m_aBridge   = pPrev->m_aBridge;
        pw.m_idxBldg   = pPrev->m_idxBldg;
        pw.m_idxBridge = pPrev->m_idxBridge;
    }
    else
    {   // ---- the two fact tables, rebuilt from the live maps ----

    // Buildings: one record per building, a per-hex index into it - a building
    // covers many hexes, so the shared record is the smaller shape here.
    {
        std::unordered_map<size_t, DWORD> mapSeen;
        std::vector<DWORD>                aKey;
        std::vector<DWORD>                aVal;

        POSITION pos = theBuildingHex.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD      dwKey;
            CBuilding* pBldg;
            theBuildingHex.GetNextAssoc( pos, dwKey, pBldg );
            if ( pBldg == NULL )
                continue;

            const size_t uKey = (size_t)pBldg;
            DWORD        dwAt;
            std::unordered_map<size_t, DWORD>::const_iterator it = mapSeen.find( uKey );
            if ( it != mapSeen.end( ) )
                dwAt = it->second;
            else
            {
                Bldg b;
                b.uKey      = uKey;
                b.hexExit   = pBldg->GetExitHex( );
                b.iExitDir  = pBldg->GetExitDir( );
                b.hexShip   = pBldg->GetShipHex( );
                b.iShipDir  = pBldg->GetShipDir( );
                b.bShipExit = ( pBldg->GetData( ) != NULL ) && pBldg->GetData( )->HasShipExit( );
                dwAt        = (DWORD)pw.m_aBldg.size( );
                pw.m_aBldg.push_back( b );
                mapSeen[uKey] = dwAt;
            }
            aKey.push_back( dwKey );
            aVal.push_back( dwAt );
        }

        pw.m_idxBldg.Reserve( aKey.size( ) );
        for ( size_t i = 0; i < aKey.size( ); ++i ) pw.m_idxBldg.Insert( aKey[i], aVal[i] );
    }

    // Bridges: one record per registered bridge hex (see PathWorld::Bridge).
    {
        std::vector<DWORD> aKey;

        POSITION pos = theBridgeHex.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD        dwKey;
            CBridgeUnit* pBu;
            theBridgeHex.GetNextAssoc( pos, dwKey, pBu );
            if ( pBu == NULL )
                continue;

            Bridge   b;
            CBridge* pPar  = pBu->GetParent( );
            b.uParent      = (size_t)pPar;
            b.iExit        = pBu->GetExit( );
            b.bParentBuilt = ( ( pPar != NULL ) && pPar->IsBuilt( ) );
            aKey.push_back( dwKey );
            pw.m_aBridge.push_back( b );
        }

        pw.m_idxBridge.Reserve( aKey.size( ) );
        for ( size_t i = 0; i < aKey.size( ); ++i ) pw.m_idxBridge.Insert( aKey[i], (DWORD)i );
    }

    }   // ---- end of the fact-table rebuild ----

    Perf::CounterAddElapsedUs( "pw.build.tbl.us", _qTbl );
    const uint64_t _qOcc = Perf::NowIfEnabled( );

    // Occupancy, derived hex by hex through exactly the loop
    // CEnNavView::IsHexMovingVehicle runs - the same four _GetVehicle lookups, the
    // same stopped / !IsOnTheMove test. Only hexes theVehicleHex actually has an
    // entry for are visited; everywhere else all four lookups miss, which is
    // occ_none.
    {
        std::vector<uint32_t> aOcc;

        POSITION pos = theVehicleHex.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwKey;
            CVehicle* pIgnored;
            theVehicleHex.GetNextAssoc( pos, dwKey, pIgnored );

            const int iHexX = (int)( dwKey >> 16 ) / 2;
            const int iHexY = (int)( dwKey & 0xFFFF ) / 2;
            if ( iHexX < 0 || iHexX >= pw.m_iWidth || iHexY < 0 || iHexY >= pw.m_iHeight )
                continue;

            BOOL bAny     = FALSE;
            BOOL bBlocked = FALSE;
            for ( int iX = 0; iX < 2; iX++ )
                for ( int iY = 0; iY < 2; iY++ )
                {
                    CVehicle* pVeh = theVehicleHex._GetVehicle( CSubHex( iHexX * 2 + iX, iHexY * 2 + iY ) );
                    if ( pVeh == NULL )
                        continue;
                    if ( pVeh->IsFlag( CUnit::stopped ) || !pVeh->IsOnTheMove( ) )
                        bBlocked = TRUE;
                    else
                        bAny = TRUE;
                }

            const size_t uAt = ( (size_t)iHexY << pw.m_iSideShift ) + (size_t)iHexX;
            const BYTE   bOcc = (BYTE)( bBlocked ? occ_blocked : ( bAny ? occ_moving : occ_none ) );
            pw.m_aHex[uAt].bOcc = bOcc;
            // Exactly the hexes the NEXT build has to put back to occ_none before it can
            // reuse this array. occ_none writes are not recorded - they change nothing.
            if ( bOcc != occ_none )
                aOcc.push_back( (uint32_t)uAt );
        }

        s_aPrevOcc.swap( aOcc );
    }

    Perf::CounterAddElapsedUs( "pw.build.occ.us", _qOcc );

    return ( ptr );
}

void PathWorld::PublishTick( void )
{
    if ( !Enabled( ) )
        return;

    // Rebuild only when the world the search reads actually moved. g_enNavEpoch counts
    // every runtime write of hex terrain / altitude / the bldg+bridge occupancy bits and
    // bridge built-state (terrain.h). Vehicle occupancy is deliberately OUTSIDE it: a
    // bVehBlock == FALSE search never reads it except in the one-step destination check
    // at the foot of _GetPath, and the plan re-checks that hex live when the route is
    // installed. Two loads and a compare - O(1) per tick, whatever the map size.
    {
        std::shared_ptr<const PathWorld> ptrCur = Current( );
        if ( ptrCur && ( ptrCur->Generation( ) == s_uGameGeneration ) && ( ptrCur->Epoch( ) == g_enNavEpoch ) )
        {
            Perf::CounterInc( "pw.builds.skipped" );
            return;
        }
    }

    const uint64_t _q = Perf::NowIfEnabled( );

    // The base for an incremental build: the snapshot that is published right now. It is
    // const and no one ever writes a published snapshot, and the shared_ptr held here
    // keeps it alive for the whole copy even if a worker retires its own reference.
    std::shared_ptr<const PathWorld> ptrPrev = Current( );
    if ( ptrPrev && ( ptrPrev->Generation( ) != s_uGameGeneration ) )
        ptrPrev.reset( );

    int                              iRows = 0;
    std::shared_ptr<const PathWorld> ptr   = _Build( ptrPrev.get( ), s_dirty, &iRows );
    if ( !ptr )
        return;
    Publish( ptr );

    // Only once the build that consumed it has been published - a build that bailed out
    // (no hexes) must not lose the rows it never read.
    s_dirty.Reset( ptr->Height( ) );

    Perf::CounterAddElapsedUs( "pw.build.us", _q );
    Perf::CounterInc( "pw.builds" );
    Perf::CounterAdd( "pw.build.rows", (int64_t)iRows );
    Perf::GaugeSet( "pw.bytes", (int64_t)ptr->Bytes( ) );

    // Sizes once per world, the way the shadow instance reports its arena.
    static uint32_t s_uSaid = 0;
    if ( s_uSaid != ptr->Generation( ) )
    {
        s_uSaid = ptr->Generation( );
        char sz[256];
        sprintf( sz, "[pathworld] init map=%dx%d  hexRec=%d bytes  hexes=%.1f KB  bldgs=%d  bridges=%d  "
                     "total=%.1f KB\n",
                 ptr->Width( ), ptr->Height( ), (int)sizeof( Hex ),
                 (double)( (size_t)ptr->Width( ) * ptr->Height( ) * sizeof( Hex ) ) / 1024.0,
                 (int)ptr->m_aBldg.size( ), (int)ptr->m_aBridge.size( ), (double)ptr->Bytes( ) / 1024.0 );
#ifdef _WIN32
        OutputDebugStringA( sz );
#endif
        FILE* pf = fopen( "pathworld.log", "a" );
        if ( pf != NULL )
        {
            fputs( sz, pf );
            fclose( pf );
        }
    }
}
