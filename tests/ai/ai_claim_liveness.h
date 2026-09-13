// ai_claim_liveness.h -- pure model of the AI router's material-claim liveness
// test (BUGS #69) and of the minimal router scene it decides.
//
// MIRRORS production; links nothing. The two mirrored bodies are:
//   CAIUnit::NoteClaimStill / ClearClaimProgress   (caiunit.hpp)
//   CAIRouter::ClaimIsLive / DropClaim /
//   CAIRouter::TrucksAreEnroute / FillPriorities   (cairoute.cpp)
// test_ai_claim.cpp lints the shipped sources so these copies cannot drift.
//
// The scene is deliberately tiny: ONE construction site that still wants one
// material, a handful of trucks, the needs list and the truck pool. It models
// only what the bug lives in -- pop the head, ask FindTransport, re-add on
// FALSE -- not the real router's sourcing, pathing or messaging.

#ifndef AI_CLAIM_LIVENESS_H
#define AI_CLAIM_LIVENESS_H

#include <vector>

namespace aiclaim {

typedef unsigned long DWORD_;

// ---------------------------------------------------------------------------
// The bound under test. MIRROR of AI_CLAIM_IDLE_MS (cairoute.cpp).
// ---------------------------------------------------------------------------
const DWORD_ AI_CLAIM_IDLE_MS = 90000;

// Traffic-layer holds the bound is DERIVED from (vehicle.h). Game frames,
// 24 to the second. test_ai_claim.cpp lints vehicle.h for these exact values,
// so a later traffic change that would invalidate the derivation fails here.
const int FRAMES_PER_SEC      = 24;
const int HOLD_FRAMES         = 240;            // retreat hold
const int HOLD_STAGGER_FRAMES = 3 * 24;         // + (GetID() % 4) seconds
const int GIVEUP_HOLD_FRAMES  = 24 * 30;        // parked off-road after giving up
const int JAM_STUCK_FRAMES    = 24 * 30;        // stagnation before asking for clearance
const int JAM_WINDOW_FRAMES   = 24 * 30;        // how long that clearance lasts
const int JAM_STAGGER_FRAMES  = 24 * 8;         // spread of per-truck expiry

// The masking rescuers the fix must fire INSIDE of (caimgr.cpp:340 / :2147).
const DWORD_ SWEEP_MS  = 240000;
const DWORD_ RESEND_MS = 300000;

inline DWORD_ FramesToMs( int iFrames ) { return (DWORD_)iFrames * 1000 / FRAMES_PER_SEC; }

// Longest episode in which a truck legitimately keeps its job while standing
// still: a jam it is being unwound from. The parked holds are shorter.
inline DWORD_ WorstLegitStandstillMs( void )
{
    return FramesToMs( JAM_STUCK_FRAMES + JAM_WINDOW_FRAMES + JAM_STAGGER_FRAMES );
}
inline DWORD_ WorstParkedHoldMs( void )
{
    return FramesToMs( GIVEUP_HOLD_FRAMES > HOLD_FRAMES + HOLD_STAGGER_FRAMES
                           ? GIVEUP_HOLD_FRAMES
                           : HOLD_FRAMES + HOLD_STAGGER_FRAMES );
}

// ---------------------------------------------------------------------------
// MIRROR of CAIUnit's two transient claim-liveness members + NoteClaimStill.
// ---------------------------------------------------------------------------
struct ClaimStamp
{
    DWORD_ dwClaimHex;
    DWORD_ dwClaimStill;

    ClaimStamp( ) : dwClaimHex( 0 ), dwClaimStill( 0 ) {}

    DWORD_ NoteClaimStill( DWORD_ dwHex, DWORD_ dwNow )
    {
        if ( dwHex != dwClaimHex || dwClaimStill == 0 )
        {
            dwClaimHex   = dwHex;
            dwClaimStill = dwNow ? dwNow : 1;
            return 0;
        }
        return dwNow - dwClaimStill;
    }
    void ClearClaimProgress( void )
    {
        dwClaimHex   = 0;
        dwClaimStill = 0;
    }
};

inline DWORD_ PackHex( int iX, int iY ) { return ( (DWORD_)(unsigned short)iX ) | ( ( (DWORD_)(unsigned short)iY ) << 16 ); }

// ---------------------------------------------------------------------------
// Scene units. A truck carries exactly the state the predicate reads.
// ---------------------------------------------------------------------------
struct Truck
{
    DWORD_     dwID;
    DWORD_     dwDataDW;   // the building it names as its job (0 = none)
    int        iHexX, iHexY;
    bool       bAlive;     // false models "the unit is gone" (ReadVeh FALSE)
    bool       bInUse;     // CAI_IN_USE
    ClaimStamp stamp;

    Truck( DWORD_ id, int x, int y ) : dwID( id ), dwDataDW( 0 ), iHexX( x ), iHexY( y ), bAlive( true ), bInUse( false ) {}
};

const int NUM_MATS = 10;  // CMaterialTypes::num_types

struct Site
{
    DWORD_ dwID;
    int    aiWant[NUM_MATS];    // GetParam(i)   -- material still needed
    DWORD_ adwClaim[NUM_MATS];  // GetParamDW(i) -- the truck claiming to bring it

    Site( DWORD_ id ) : dwID( id )
    {
        for ( int i = 0; i < NUM_MATS; ++i ) { aiWant[i] = 0; adwClaim[i] = 0; }
    }
};

// ---------------------------------------------------------------------------
// The router scene.
// ---------------------------------------------------------------------------
struct Router
{
    Site                site;
    std::vector<Truck>  trucks;
    std::vector<DWORD_> plBldgsNeed;        // ids, in list order
    std::vector<DWORD_> plTrucksAvailable;  // ids, in list order
    DWORD_              dwNow;
    int                 iPasses;            // FillPriorities call counter (m_iReserveSweep)
    int                 iDropped;           // claims released by the liveness test
    int                 iAssigned;          // claims handed out

    Router( ) : site( 100 ), dwNow( 1000 ), iPasses( 0 ), iDropped( 0 ), iAssigned( 0 ) {}

    Truck* Find( DWORD_ dwID )
    {
        for ( size_t i = 0; i < trucks.size( ); ++i )
            if ( trucks[i].dwID == dwID && trucks[i].bAlive ) return &trucks[i];
        return 0;
    }
    int NeedCount( DWORD_ dwID ) const
    {
        int n = 0;
        for ( size_t i = 0; i < plBldgsNeed.size( ); ++i )
            if ( plBldgsNeed[i] == dwID ) ++n;
        return n;
    }
    bool InPool( DWORD_ dwID ) const
    {
        for ( size_t i = 0; i < plTrucksAvailable.size( ); ++i )
            if ( plTrucksAvailable[i] == dwID ) return true;
        return false;
    }

    // MIRROR of CAIRouter::ClaimIsLive. Mutates the truck's stamp, exactly like
    // production: what is stored is the START of the standstill.
    bool ClaimIsLive( Truck* pTruck, Site* pBldg )
    {
        if ( pTruck == 0 || pBldg == 0 ) return false;
        if ( pTruck->dwDataDW != pBldg->dwID ) return false;
        if ( !pTruck->bAlive ) return false;  // ReadVeh FALSE
        DWORD_ dwStill = pTruck->stamp.NoteClaimStill( PackHex( pTruck->iHexX, pTruck->iHexY ), dwNow );
        return dwStill < AI_CLAIM_IDLE_MS;
    }

    // MIRROR of CAIRouter::DropClaim. Touches neither list.
    void DropClaim( Site* pBldg, DWORD_ dwTruckID )
    {
        if ( pBldg == 0 || dwTruckID == 0 ) return;
        for ( int i = 0; i < NUM_MATS; ++i )
            if ( pBldg->adwClaim[i] == dwTruckID ) pBldg->adwClaim[i] = 0;
        // truck side ONLY for a truck that still names this building: a fossil
        // left by a re-tasked truck must not cancel that truck's current job
        Truck* pTruck = Find( dwTruckID );
        if ( pTruck != 0 && pTruck->dwDataDW == pBldg->dwID )
        {
            pTruck->dwDataDW = 0;
            pTruck->bInUse   = false;
            pTruck->stamp.ClearClaimProgress( );
        }
        ++iDropped;
    }

    // MIRROR of CAIRouter::TrucksAreEnroute.
    bool TrucksAreEnroute( Site* pBldg )
    {
        for ( int i = 0; i < NUM_MATS; ++i )
        {
            if ( pBldg->adwClaim[i] && pBldg->aiWant[i] )
            {
                DWORD_ dwTruck = pBldg->adwClaim[i];
                Truck* pTruck  = Find( dwTruck );
                if ( pTruck == 0 )
                {
                    pBldg->adwClaim[i] = 0;  // truck is gone
                    continue;
                }
                if ( ClaimIsLive( pTruck, pBldg ) ) return true;
                DropClaim( pBldg, dwTruck );
            }
        }
        return false;
    }

    // Model of CAIRouter::FindTransport: the enroute short-circuit, then an
    // assignment out of the pool (nearest = first, the pool is tiny).
    bool FindTransport( Site* pBldg )
    {
        if ( plTrucksAvailable.empty( ) ) return false;
        if ( TrucksAreEnroute( pBldg ) ) return true;
        if ( plTrucksAvailable.empty( ) ) return false;

        DWORD_ dwPick = plTrucksAvailable.front( );
        Truck* pTruck = Find( dwPick );
        plTrucksAvailable.erase( plTrucksAvailable.begin( ) );
        if ( pTruck == 0 ) return false;
        pTruck->dwDataDW = pBldg->dwID;
        pTruck->bInUse   = true;
        pTruck->stamp.ClearClaimProgress( );  // fresh window per assignment
        for ( int i = 0; i < NUM_MATS; ++i )
            if ( pBldg->aiWant[i] ) pBldg->adwClaim[i] = pTruck->dwID;
        ++iAssigned;
        return true;
    }

    // Model of CAIRouter::GetTrucksAvailable: re-pool idle, untasked trucks.
    void GetTrucksAvailable( void )
    {
        for ( size_t i = 0; i < trucks.size( ); ++i )
        {
            Truck& t = trucks[i];
            if ( !t.bAlive || t.bInUse ) continue;
            if ( !InPool( t.dwID ) ) plTrucksAvailable.push_back( t.dwID );
        }
    }

    // MIRROR of CAIRouter::FillPriorities' list discipline (the bug's home).
    void FillPriorities( void )
    {
        if ( ( ++iPasses % 10 ) == 0 ) GetTrucksAvailable( );

        int iCnt = (int)plBldgsNeed.size( );
        while ( iCnt-- )
        {
            DWORD_ dwBldg = plBldgsNeed.front( );
            plBldgsNeed.erase( plBldgsNeed.begin( ) );
            bool bFound = ( dwBldg == site.dwID ) ? FindTransport( &site ) : true;
            if ( !bFound ) plBldgsNeed.push_back( dwBldg );
        }
    }

    // Model of the out_mat / bldg_stat re-signal: the site says it still needs
    // materials. Guarded against duplicates, like the production handlers.
    void SignalNeedsMaterials( void )
    {
        if ( NeedCount( site.dwID ) == 0 ) plBldgsNeed.push_back( site.dwID );
    }
};

// ---------------------------------------------------------------------------
// The PRE-FIX predicate, kept so a test can show the defect it replaced:
// alive + still names the building => "enroute", with no progress test.
// ---------------------------------------------------------------------------
inline bool LegacyClaimIsLive( Truck* pTruck, Site* pBldg )
{
    if ( pTruck == 0 || pBldg == 0 ) return false;
    return pTruck->bAlive && pTruck->dwDataDW == pBldg->dwID;
}

}  // namespace aiclaim

#endif  // AI_CLAIM_LIVENESS_H
