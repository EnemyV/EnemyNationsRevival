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

void EnNavNewGame( void )
{
    g_enNavEpoch = 0;
    ++s_uGameGeneration;
    PathWorld::Clear( );
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

std::shared_ptr<const PathWorld> PathWorld::Build( void )
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

    pw.m_aHex.resize( (size_t)pw.m_iHeight << pw.m_iSideShift );
    for ( int y = 0; y < pw.m_iHeight; ++y )
        for ( int x = 0; x < pw.m_iWidth; ++x )
        {
            CHex const* pHex = theMap._GetHex( x, y );
            Hex&        h    = pw.m_aHex[( (size_t)y << pw.m_iSideShift ) + (size_t)x];
            h.bUnits         = pHex->GetUnits( );
            h.bType          = (BYTE)pHex->GetType( );
            h.bAlt           = (BYTE)pHex->GetAlt( );
            h.bOcc           = occ_none;
        }

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

    // Occupancy, derived hex by hex through exactly the loop
    // CEnNavView::IsHexMovingVehicle runs - the same four _GetVehicle lookups, the
    // same stopped / !IsOnTheMove test. Only hexes theVehicleHex actually has an
    // entry for are visited; everywhere else all four lookups miss, which is
    // occ_none.
    {
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

            pw.m_aHex[( (size_t)iHexY << pw.m_iSideShift ) + (size_t)iHexX].bOcc =
                (BYTE)( bBlocked ? occ_blocked : ( bAny ? occ_moving : occ_none ) );
        }
    }

    return ( ptr );
}

void PathWorld::PublishTick( void )
{
    if ( !Enabled( ) )
        return;

    const uint64_t _q = Perf::NowIfEnabled( );

    std::shared_ptr<const PathWorld> ptr = Build( );
    if ( !ptr )
        return;
    Publish( ptr );

    Perf::CounterAddElapsedUs( "pw.build.us", _q );
    Perf::CounterInc( "pw.builds" );
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
