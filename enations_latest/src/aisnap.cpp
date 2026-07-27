////////////////////////////////////////////////////////////////////////////
//
//  aisnap.cpp : per-tick world snapshot for lock-free AI reads (Tier B).
//               See aisnap.h for the design and concurrency model.
//
////////////////////////////////////////////////////////////////////////////

#include "aisnap.h"

#include "stdafx.h"

#include "building.inl"  // CBuilding/theBuildingMap (+ CVehicleBuilding/CRepairBuilding)
#include "lastplnt.h"    // theGame (AI-count early-out)
#include "minerals.inl"  // theMinerals (presence bitmap)
#include "terrain.inl"   // CHexCoord ctor/Wrap inlines (needed at /Ob2: no
                         // other TU emits out-of-line COMDAT copies anymore)
#include "unit.inl"      // CUnit accessor inlines (GetOwner/GetStore/...)
#include "vehicle.inl"   // CVehicle/theVehicleMap

#include <unordered_map>

#define new DEBUG_NEW

extern CRITICAL_SECTION cs;  // the global game lock ("BGL")

namespace
{
typedef std::unordered_map<DWORD, AiBldgSnap> BldgMap;
typedef std::unordered_map<DWORD, AiVehSnap>  VehMap;

// double buffers: [s_iFront] is read by AI threads, the other is the
// main thread's build target. s_iFront only changes under s_swapLock
// (exclusive); readers hold it shared across find+copy.
BldgMap s_mapBldg[2];
VehMap  s_mapVeh[2];
int     s_iFront   = 0;
SRWLOCK s_swapLock = SRWLOCK_INIT;

DWORD s_dwLastPublish = 0;  // self-throttle (main thread only)

// publish cadence: the sim runs at 24Hz (~42ms); publishing faster than the
// tick gains nothing (the world only changes inside ticks).
const DWORD kPublishMs = 40;

// minerals presence bitmap: 1 byte per hex, 512x512 max (CHexCoord packs
// coords as (X<<16)|Y with WORD parts; current maps are <=512 per side).
// s_minReady: 0 = not built yet, 1 = valid, -1 = map too large (permanent
// legacy-Lookup fallback). Built by Publish under cs; bytes cleared by the
// single depletion site (main thread); AI reads are lock-free (benign race:
// one stale pass at worst).
const int     kMinDim = 512;
BYTE          s_minMap[kMinDim * kMinDim];
volatile LONG s_minReady = 0;

int SnapEnabled( void )
{
    // EN_AISNAP default ON; "0" disables (legacy locked path). Checked once.
    // (Default-on per project history: default-off perf flags go dormant.)
    static int s_iEnabled = -1;
    if ( s_iEnabled < 0 )
    {
        const char* p = SDL_getenv( "EN_AISNAP" );
        s_iEnabled    = ( p != NULL && atoi( p ) == 0 ) ? 0 : 1;
    }
    return s_iEnabled;
}

// single fill used by BOTH Publish (per tick) and the ReadBldg locked
// fallback, so the two paths can never drift apart.
// caller must hold `cs`.
void FillBldgSnap( CBuilding* pBldg, AiBldgSnap& s )
{
    memset( &s, 0, sizeof( s ) );

    // null-guarded: publish touches EVERY unit each tick (wider surface
    // than the legacy per-id path), so don't trust mid-death owners
    CPlayer* pOwner = pBldg->GetOwner( );
    s.iOwner        = ( pOwner != NULL ) ? pOwner->GetPlyrNum( ) : -1;

    for ( int i = 0; i < CMaterialTypes::num_types; ++i ) s.aiStore[i] = pBldg->GetStore( i );

    CHexCoord hex   = pBldg->GetExitHex( );
    s.iExitX        = hex.X( );
    s.iExitY        = hex.Y( );
    s.bConstructing = (int)pBldg->IsConstructing( );
    s.bPaused       = (int)pBldg->IsPaused( );
    s.bWaiting      = (int)pBldg->IsWaiting( );
    s.iDamagePer    = pBldg->GetDamagePer( );
    s.iType         = pBldg->GetData( )->GetType( );
    s.iUnionType    = (int)pBldg->GetData( )->GetUnionType( );
    s.iBldgType     = pBldg->GetData( )->GetBldgType( );
    s.bAbandoned    = (int)pBldg->IsFlag( CUnit::abandoned );
    s.pProducing    = NULL;
    s.pRepairing    = NULL;

    // current production / repair records (static-table pointers) — the
    // same lookups CAIUnit::GetCopyData's CBuildUnit path did under cs
    if ( s.iUnionType == CStructureData::UTvehicle )
    {
        CVehicleBuilding* pVehBldg = (CVehicleBuilding*)pBldg;
        s.pProducing               = pVehBldg->GetBldUnt( );
    }
    else if ( s.iUnionType == CStructureData::UTrepair )
    {
        CRepairBuilding* pRepBldg      = (CRepairBuilding*)pBldg;
        CVehicle*        pVehRepairing = pRepBldg->GetVehRepairing( );
        if ( pVehRepairing != NULL )
        {
            CBuildRepair const* pBuildRep = pRepBldg->GetData( );
            if ( pBuildRep != NULL )
                s.pRepairing = pBuildRep->GetRepair( pVehRepairing->GetData( )->GetType( ) );
        }
    }
}

// caller must hold `cs`.
void FillVehSnap( CVehicle* pVeh, AiVehSnap& s )
{
    memset( &s, 0, sizeof( s ) );

    CPlayer* pOwner = pVeh->GetOwner( );
    s.iOwner        = ( pOwner != NULL ) ? pOwner->GetPlyrNum( ) : -1;

    for ( int i = 0; i < CMaterialTypes::num_types; ++i ) s.aiStore[i] = pVeh->GetStore( i );

    CHexCoord hex = pVeh->GetHexHead( );
    s.iHeadX      = hex.X( );
    s.iHeadY      = hex.Y( );
    hex           = pVeh->GetHexDest( );
    s.iDestX      = hex.X( );
    s.iDestY      = hex.Y( );
    s.iDamagePer  = pVeh->GetDamagePer( );
    s.iType       = pVeh->GetData( )->GetType( );

    CSubHex pt  = pVeh->GetPtHead( );
    s.iPtHeadX  = pt.x;
    s.iPtHeadY  = pt.y;
    pt          = pVeh->GetPtTail( );
    s.iPtTailX  = pt.x;
    s.iPtTailY  = pt.y;
    s.iCargoCount = pVeh->GetCargoCount( );
    s.bCarried    = ( pVeh->GetTransport( ) != NULL ) ? 1 : 0;
    s.iSpotting   = pVeh->GetSpottingRange( );
    s.iRouteMode  = (int)pVeh->GetRouteMode( );
    s.iEvent      = (int)pVeh->GetEvent( );
}
}  // namespace

BOOL AiSnap::Enabled( void )
{
    return SnapEnabled( ) ? TRUE : FALSE;
}

void AiSnap::Publish( void )
{
    if ( !SnapEnabled( ) )
        return;

    // self-throttle to ~tick cadence; cheap to call every loop iteration
    DWORD dwNow = timeGetTime( );
    if ( s_dwLastPublish && ( dwNow - s_dwLastPublish ) < kPublishMs )
        return;
    s_dwLastPublish = dwNow;

    int      iBack = 1 - s_iFront;
    BldgMap& mB    = s_mapBldg[iBack];
    VehMap&  mV    = s_mapVeh[iBack];
    mB.clear( );
    mV.clear( );

    // copy the world under the game lock — same data the legacy GetCopyData
    // path read, captured at one consistent instant per tick
    EnterCriticalSection( &cs );

    // no AI players -> no readers; don't pay the per-tick copy (checked under
    // cs because AI threads mutate the player lists via AiDeletePlayer)
    if ( theGame.GetAi( ).GetCount( ) == 0 )
    {
        LeaveCriticalSection( &cs );
        return;
    }

    // build the minerals presence bitmap once per game (we already hold cs)
    if ( s_minReady == 0 )
    {
        memset( (void*)s_minMap, 0, sizeof( s_minMap ) );
        BOOL     bFit  = TRUE;
        POSITION posMn = theMinerals.GetStartPosition( );
        while ( posMn != NULL )
        {
            DWORD      dwKey;
            CMinerals* pMn;
            theMinerals.GetNextAssoc( posMn, dwKey, pMn );
            int iX = (int)( dwKey >> 16 );
            int iY = (int)( dwKey & 0xFFFF );
            if ( iX < kMinDim && iY < kMinDim )
                s_minMap[( iX << 9 ) | iY] = 1;
            else
                bFit = FALSE;
        }
        InterlockedExchange( &s_minReady, bFit ? 1 : -1 );
    }

    POSITION pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        if ( pBldg == NULL )
            continue;

        FillBldgSnap( pBldg, mB[dwID] );
    }

    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        if ( pVeh == NULL )
            continue;

        FillVehSnap( pVeh, mV[dwID] );
    }

    LeaveCriticalSection( &cs );

    // flip front under the swap lock (exclusive): readers in flight finish
    // first; new readers see the fresh buffer
    AcquireSRWLockExclusive( &s_swapLock );
    s_iFront = iBack;
    ReleaseSRWLockExclusive( &s_swapLock );
}

BOOL AiSnap::CopyBldg( DWORD dwID, AiBldgSnap& out )
{
    if ( !SnapEnabled( ) )
        return FALSE;

    BOOL bFound = FALSE;
    AcquireSRWLockShared( &s_swapLock );
    const BldgMap&          m  = s_mapBldg[s_iFront];
    BldgMap::const_iterator it = m.find( dwID );
    if ( it != m.end( ) )
    {
        out    = it->second;
        bFound = TRUE;
    }
    ReleaseSRWLockShared( &s_swapLock );
    return bFound;
}

BOOL AiSnap::CopyVeh( DWORD dwID, AiVehSnap& out )
{
    if ( !SnapEnabled( ) )
        return FALSE;

    BOOL bFound = FALSE;
    AcquireSRWLockShared( &s_swapLock );
    const VehMap&          m  = s_mapVeh[s_iFront];
    VehMap::const_iterator it = m.find( dwID );
    if ( it != m.end( ) )
    {
        out    = it->second;
        bFound = TRUE;
    }
    ReleaseSRWLockShared( &s_swapLock );
    return bFound;
}

BOOL AiSnap::ReadVeh( DWORD dwID, AiVehSnap& out )
{
    // snapshot first (lock-free)
    if ( CopyVeh( dwID, out ) )
        return TRUE;

    // miss (new unit, dead unit, snapshot disabled/not yet published):
    // read the live unit under cs — exactly what the legacy site did
    BOOL bFound = FALSE;
    EnterCriticalSection( &cs );
    CVehicle* pVeh = theVehicleMap.GetVehicle( dwID );
    if ( pVeh != NULL )
    {
        FillVehSnap( pVeh, out );
        bFound = TRUE;
    }
    LeaveCriticalSection( &cs );
    return bFound;
}

BOOL AiSnap::ReadBldg( DWORD dwID, AiBldgSnap& out )
{
    if ( CopyBldg( dwID, out ) )
        return TRUE;

    BOOL bFound = FALSE;
    EnterCriticalSection( &cs );
    CBuilding* pBldg = theBuildingMap.GetBldg( dwID );
    if ( pBldg != NULL )
    {
        FillBldgSnap( pBldg, out );
        bFound = TRUE;
    }
    LeaveCriticalSection( &cs );
    return bFound;
}

BOOL AiSnap::MineralsReady( void )
{
    return ( s_minReady == 1 ) ? TRUE : FALSE;
}

BOOL AiSnap::MineralsHas( int iX, int iY )
{
    if ( s_minReady != 1 )
        return FALSE;
    if ( iX < 0 || iX >= kMinDim || iY < 0 || iY >= kMinDim )
        return FALSE;
    return s_minMap[( iX << 9 ) | iY] ? TRUE : FALSE;
}

void AiSnap::MineralsRemoved( int iX, int iY )
{
    if ( iX >= 0 && iX < kMinDim && iY >= 0 && iY < kMinDim )
        s_minMap[( iX << 9 ) | iY] = 0;
}

void AiSnap::Reset( void )
{
    AcquireSRWLockExclusive( &s_swapLock );
    s_mapBldg[0].clear( );
    s_mapBldg[1].clear( );
    s_mapVeh[0].clear( );
    s_mapVeh[1].clear( );
    s_dwLastPublish = 0;
    InterlockedExchange( &s_minReady, 0 );  // rebuilt on next Publish
    ReleaseSRWLockExclusive( &s_swapLock );
}
