// test_ai_claim.cpp
//
// Guards BUGS #69: "one idle truck silences an AI construction site
// indefinitely". CAIRouter::TrucksAreEnroute proved "a delivery is in flight"
// by NAME MATCH ONLY (truck alive + GetDataDW() == bldg id). FindTransport
// returned TRUE on that and FillPriorities only re-adds a site when
// FindTransport is FALSE, so one truck that went idle while assigned dropped
// its site out of m_plBldgsNeed with no path back in.
//
// Three layers, on purpose:
//  (1) PREDICATE MIRROR -- ai_claim_liveness.h copies ClaimIsLive /
//      NoteClaimStill / DropClaim and decides the cases that matter: live
//      en-route, idle past the bound, inside a traffic hold, job changed, gone.
//  (2) ROUTER SCENE -- the same mirror driving the FillPriorities list
//      discipline, so the LIST outcome is tested and not just the predicate:
//      the site comes back exactly once and the idle truck cannot re-claim it.
//  (3) SOURCE LINT -- parses cairoute.cpp / caiunit.hpp / vehicle.h and asserts
//      the shipped forms are present, the pre-fix forms are GONE, and the
//      traffic-hold constants the 90s bound was DERIVED from still hold those
//      values. Whitespace is squeezed first, so reformatting cannot break it.
//
// Standalone: links no game code, touches no game build. Safe to run anytime.
//
// Usage: ai_claim_tests.exe [<cairoute.cpp> [<caiunit.hpp> [<vehicle.h>]]]
// Exit:  0 all pass, 1 a check failed, 2 a given source cannot be read.

#define _CRT_SECURE_NO_WARNINGS

#include "ai_claim_liveness.h"
#include "microtest.h"

#include <cstdio>
#include <string>

using namespace aiclaim;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------
static std::string ReadAll( const char* pszPath )
{
    std::string s;
    FILE*       f = std::fopen( pszPath, "rb" );
    if ( f == 0 ) return s;
    char   buf[8192];
    size_t n;
    while ( ( n = std::fread( buf, 1, sizeof( buf ), f ) ) > 0 ) s.append( buf, n );
    std::fclose( f );
    return s;
}
static std::string NoWs( const std::string& in )
{
    std::string out;
    out.reserve( in.size( ) );
    for ( size_t i = 0; i < in.size( ); ++i )
        if ( in[i] != ' ' && in[i] != '\t' && in[i] != '\r' && in[i] != '\n' ) out += in[i];
    return out;
}
static bool Has( const std::string& hay, const char* needle ) { return hay.find( needle ) != std::string::npos; }
// comparing two named constants inside CHECK is a constant expression (C4127 at
// /W4); route the ordering assertions through a call so the intent survives
static bool Lt( DWORD_ a, DWORD_ b ) { return a < b; }

// a site that wants one material, with that one claim held by dwTruck
static void Claim( Router& r, DWORD_ dwTruck )
{
    r.site.aiWant[0]   = 12;
    r.site.adwClaim[0] = dwTruck;
    Truck* p           = r.Find( dwTruck );
    if ( p != 0 )
    {
        p->dwDataDW = r.site.dwID;
        p->bInUse   = true;
    }
}

// ===========================================================================
// (1) PREDICATE
// ===========================================================================

// A truck that is actually driving keeps its claim for as long as it drives --
// including far past every rescuer clock, which is the whole point: a live
// delivery must never be interrupted by the liveness test.
static void test_enroute_truck_keeps_claim( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );
    Claim( r, 7 );

    for ( int iPass = 0; iPass < 200; ++iPass )
    {
        r.dwNow += 3000;                   // 3s per router pass -> 600s total
        r.trucks[0].iHexX += ( iPass & 1 ); // ...and it changes hex as it goes
        r.trucks[0].iHexY += ( iPass & 1 ) ^ 1;
        CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );
    }
    CHECK( r.dwNow > RESEND_MS );          // outlived the 5-min resend
    CHECK_EQ( r.iDropped, 0 );
}

// The defect case: alive, still names the building, never moves.
static void test_idle_truck_loses_claim_at_the_bound( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );
    Claim( r, 7 );

    // first observation arms the standstill clock and must NOT convict
    CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );

    // one millisecond short of the bound it is still a delivery
    r.dwNow += AI_CLAIM_IDLE_MS - 1;
    CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );

    // at the bound it is not
    r.dwNow += 1;
    CHECK( !r.ClaimIsLive( &r.trucks[0], &r.site ) );
}

// Every stationary episode the traffic layer legitimately puts a truck through
// while it still holds its job must survive. These are the real clocks from
// vehicle.h (the lint below pins them).
static void test_traffic_holds_keep_the_claim( void )
{
    const DWORD_ adwHolds[] = { FramesToMs( HOLD_FRAMES ),
                                FramesToMs( HOLD_FRAMES + HOLD_STAGGER_FRAMES ),
                                FramesToMs( GIVEUP_HOLD_FRAMES ),
                                WorstParkedHoldMs( ),
                                WorstLegitStandstillMs( ) };
    for ( int i = 0; i < 5; ++i )
    {
        Router r;
        r.trucks.push_back( Truck( 7, 10, 10 ) );
        Claim( r, 7 );
        CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );  // arm the clock

        // the whole hold elapses in one-second observations, never moving
        for ( DWORD_ dwT = 0; dwT < adwHolds[i]; dwT += 1000 )
        {
            r.dwNow += 1000;
            CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );
        }
        // ...and the truck rejoins the haul, so the clock resets
        r.dwNow += 1000;
        r.trucks[0].iHexX += 1;
        CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );
        CHECK_EQ( r.iDropped, 0 );
    }
}

// The bound has to sit between the longest legitimate hold and the rescuers
// that were MASKING the bug -- otherwise the fix either fights the traffic
// layer or never fires before a sweep re-tasks the truck. This is the check
// the bound cannot game: raise a traffic hold past 90s and it fails.
static void test_bound_is_between_holds_and_rescuers( void )
{
    CHECK( Lt( WorstLegitStandstillMs( ), AI_CLAIM_IDLE_MS ) );
    CHECK( Lt( WorstParkedHoldMs( ), AI_CLAIM_IDLE_MS ) );
    CHECK( Lt( AI_CLAIM_IDLE_MS, SWEEP_MS ) );
    CHECK( Lt( AI_CLAIM_IDLE_MS, RESEND_MS ) );
    CHECK_EQ( WorstLegitStandstillMs( ), 68000 );  // 30s + 30s + 8s of jam recovery
    CHECK_EQ( WorstParkedHoldMs( ), 30000 );       // GIVEUP_HOLD_FRAMES
    CHECK_EQ( AI_CLAIM_IDLE_MS, 90000 );
}

// A truck re-tasked to another job, or dead, is no claim at all -- immediately,
// with no dwell. (The pre-fix reaper already got these two right; they must not
// regress while the idle case is added.)
static void test_job_change_and_death_lose_the_claim( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );
    Claim( r, 7 );
    CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );

    r.trucks[0].dwDataDW = 999;  // carrying a different job now
    CHECK( !r.ClaimIsLive( &r.trucks[0], &r.site ) );

    r.trucks[0].dwDataDW = r.site.dwID;
    r.trucks[0].bAlive   = false;  // ReadVeh FALSE
    CHECK( !r.ClaimIsLive( &r.trucks[0], &r.site ) );
}

// Both the enroute test and the ghost-claim reaper observe the same truck in
// the same pass. Observing twice must not inflate the dwell, and the verdict
// must not depend on how often the router looks.
static void test_observation_is_idempotent( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );
    Claim( r, 7 );

    for ( int i = 0; i < 50; ++i ) CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );  // one pass, 50 looks
    r.dwNow += AI_CLAIM_IDLE_MS - 1;
    for ( int i = 0; i < 50; ++i ) CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );
    r.dwNow += 1;
    CHECK( !r.ClaimIsLive( &r.trucks[0], &r.site ) );

    // a dense sampler and a sparse one agree on the same timeline
    Router a, b;
    a.trucks.push_back( Truck( 7, 10, 10 ) );
    b.trucks.push_back( Truck( 7, 10, 10 ) );
    Claim( a, 7 );
    Claim( b, 7 );
    CHECK( a.ClaimIsLive( &a.trucks[0], &a.site ) );
    CHECK( b.ClaimIsLive( &b.trucks[0], &b.site ) );
    for ( int i = 0; i < 90; ++i )
    {
        a.dwNow += 1000;
        a.ClaimIsLive( &a.trucks[0], &a.site );  // sampled every second
    }
    b.dwNow += 90000;                            // sampled once
    CHECK( !a.ClaimIsLive( &a.trucks[0], &a.site ) );
    CHECK( !b.ClaimIsLive( &b.trucks[0], &b.site ) );
}

// A re-assignment must hand out a FULL fresh window, or a truck that just lost
// a claim for standing still would lose the next one on sight (churn).
static void test_assignment_resets_the_window( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );
    Claim( r, 7 );
    CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );
    r.dwNow += AI_CLAIM_IDLE_MS;
    CHECK( !r.ClaimIsLive( &r.trucks[0], &r.site ) );
    r.DropClaim( &r.site, 7 );

    // re-assigned where it stands, without having moved a hex
    r.plTrucksAvailable.push_back( 7 );
    CHECK( r.FindTransport( &r.site ) );
    CHECK_EQ( r.trucks[0].dwDataDW, r.site.dwID );
    CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );
    r.dwNow += AI_CLAIM_IDLE_MS - 1;
    CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );  // the new window, in full
}

// ===========================================================================
// (2) ROUTER SCENE -- the list outcome, which is what the bug actually was
// ===========================================================================

// The defect, end to end, under the PRE-FIX predicate: the site leaves
// m_plBldgsNeed and the out_mat re-signal cannot get it back in.
static void test_legacy_predicate_strands_the_site( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );  // the idle claimer
    r.trucks.push_back( Truck( 8, 40, 40 ) );  // a perfectly good spare
    Claim( r, 7 );
    r.plBldgsNeed.push_back( r.site.dwID );
    r.plTrucksAvailable.push_back( 8 );

    // hand-rolled FillPriorities using the legacy test. out_mat fires every pass
    // and is immediately undone by the stale claim, which is the defect.
    for ( int iPass = 0; iPass < 50; ++iPass )
    {
        r.dwNow += 5000;
        r.SignalNeedsMaterials( );
        CHECK_EQ( r.NeedCount( r.site.dwID ), 1 );  // the re-signal does get it in...
        int iCnt = (int)r.plBldgsNeed.size( );
        while ( iCnt-- )
        {
            r.plBldgsNeed.erase( r.plBldgsNeed.begin( ) );
            if ( !LegacyClaimIsLive( &r.trucks[0], &r.site ) ) r.plBldgsNeed.push_back( r.site.dwID );
        }
        CHECK_EQ( r.NeedCount( r.site.dwID ), 0 );  // ...and the claim drops it again
    }
    CHECK_EQ( r.plBldgsNeed.size( ), 0u );          // stranded: off the needs list
    CHECK_EQ( r.site.adwClaim[0], 7u );             // the fossil claim still stands
    CHECK_EQ( r.site.aiWant[0], 12 );               // and the site still wants material
    CHECK( r.InPool( 8 ) );                         // with a spare truck sitting idle
}

// The fix, same scene: the claim dies at the bound, the site returns to the
// needs list EXACTLY ONCE, and the spare truck takes the delivery.
static void test_idle_claim_releases_site_exactly_once( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );
    r.trucks.push_back( Truck( 8, 40, 40 ) );
    Claim( r, 7 );
    r.plBldgsNeed.push_back( r.site.dwID );
    r.plTrucksAvailable.push_back( 8 );

    // inside the bound nothing happens: the site stays out of the list, held by
    // the claim, and no truck is re-assigned
    r.dwNow += 1000;
    r.FillPriorities( );
    CHECK_EQ( r.NeedCount( r.site.dwID ), 0 );
    CHECK_EQ( r.iDropped, 0 );
    CHECK_EQ( r.iAssigned, 0 );
    r.SignalNeedsMaterials( );
    CHECK_EQ( r.NeedCount( r.site.dwID ), 1 );

    // past the bound: the claim is released and the SPARE truck is assigned in
    // the same pass, so the site is served rather than merely re-listed
    r.dwNow += AI_CLAIM_IDLE_MS;
    r.FillPriorities( );
    CHECK_EQ( r.iDropped, 1 );
    CHECK_EQ( r.iAssigned, 1 );
    CHECK_EQ( r.site.adwClaim[0], 8u );
    CHECK_EQ( r.trucks[1].dwDataDW, r.site.dwID );
    CHECK_EQ( r.NeedCount( r.site.dwID ), 0 );  // claimed by a LIVE truck now

    // no duplicate node anywhere in the run
    CHECK( r.NeedCount( r.site.dwID ) <= 1 );
}

// With no spare truck the site must come back into the list exactly once -- not
// zero times (the bug) and not twice (a double re-add from releasing inside a
// pass that also re-adds on FALSE).
static void test_site_readded_once_with_no_spare( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );
    Claim( r, 7 );
    r.plBldgsNeed.push_back( r.site.dwID );
    r.plTrucksAvailable.push_back( 99 );  // a phantom id: pool non-empty, no truck

    // one pass arms the standstill clock (a single sample never convicts), and
    // leaves the site out of the list on the live claim
    r.dwNow += 1000;
    r.FillPriorities( );
    CHECK_EQ( r.iDropped, 0 );
    CHECK_EQ( r.NeedCount( r.site.dwID ), 0 );
    r.SignalNeedsMaterials( );

    r.dwNow += AI_CLAIM_IDLE_MS + 1;
    r.FillPriorities( );
    CHECK_EQ( r.iDropped, 1 );
    CHECK_EQ( r.NeedCount( r.site.dwID ), 1 );
    CHECK_EQ( r.site.adwClaim[0], 0u );
    CHECK_EQ( r.trucks[0].dwDataDW, 0u );  // binding CLEARED, not just ignored
    CHECK( !r.trucks[0].bInUse );
}

// The released truck must not be the one that takes the site again on the spot:
// the release does not pool it, so GetNearestTruck cannot see it in this pass.
static void test_released_truck_cannot_reclaim_in_the_same_pass( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );  // the only truck in the colony
    Claim( r, 7 );
    r.plBldgsNeed.push_back( r.site.dwID );
    // a claimed truck is NOT in the pool (GetTrucksAvailable skips CAI_IN_USE);
    // the phantom id only keeps the pool non-empty, as production requires
    r.plTrucksAvailable.push_back( 99 );

    r.dwNow += 1000;
    r.FillPriorities( );  // arm the clock
    r.SignalNeedsMaterials( );

    r.dwNow += AI_CLAIM_IDLE_MS + 1;
    r.FillPriorities( );
    CHECK( !r.InPool( 7 ) );               // not re-pooled by the release
    CHECK_EQ( r.site.adwClaim[0], 0u );    // and so not re-claimed
    CHECK_EQ( r.NeedCount( r.site.dwID ), 1 );

    // it rejoins the pool only on the periodic re-scan, bounded by 10 passes,
    // and then gets a FULL fresh window rather than dying on sight
    for ( int i = 0; i < 10; ++i )
    {
        r.dwNow += 1000;
        r.FillPriorities( );
    }
    CHECK_EQ( r.trucks[0].dwDataDW, r.site.dwID );  // re-assigned
    CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );
    CHECK_EQ( r.iDropped, 1 );  // exactly one release in the whole run
}

// A truck inside a traffic hold must not lose the job mid-hold, and the site
// must stay out of the needs list while the hold runs.
static void test_scene_traffic_hold_keeps_the_job( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );
    r.trucks.push_back( Truck( 8, 40, 40 ) );
    Claim( r, 7 );
    r.plBldgsNeed.push_back( r.site.dwID );
    r.plTrucksAvailable.push_back( 8 );

    const DWORD_ dwHold = WorstLegitStandstillMs( );  // 68s, the worst of them
    for ( DWORD_ dwT = 0; dwT < dwHold; dwT += 1000 )
    {
        r.dwNow += 1000;
        r.FillPriorities( );
        r.SignalNeedsMaterials( );
    }
    CHECK_EQ( r.iDropped, 0 );
    CHECK_EQ( r.site.adwClaim[0], 7u );
    CHECK_EQ( r.trucks[0].dwDataDW, r.site.dwID );
    CHECK( r.InPool( 8 ) );  // the spare was never needed

    // the hold ends, the truck resumes the haul and is never convicted
    for ( int i = 0; i < 20; ++i )
    {
        r.dwNow += 1000;
        r.trucks[0].iHexX += 1;
        r.FillPriorities( );
    }
    CHECK_EQ( r.iDropped, 0 );
    CHECK_EQ( r.site.adwClaim[0], 7u );
}

// A truck re-tasked away from the site loses the claim at once (no dwell), and
// the site is re-listed in the same pass.
static void test_scene_job_change_releases_at_once( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );
    Claim( r, 7 );
    r.plBldgsNeed.push_back( r.site.dwID );
    r.plTrucksAvailable.push_back( 99 );

    r.dwNow += 1000;
    r.trucks[0].dwDataDW = 555;  // a sweep re-tasked it
    r.FillPriorities( );
    CHECK_EQ( r.iDropped, 1 );
    CHECK_EQ( r.site.adwClaim[0], 0u );
    CHECK_EQ( r.NeedCount( r.site.dwID ), 1 );
    // and the release must NOT have cancelled the job it moved on to
    CHECK_EQ( r.trucks[0].dwDataDW, 555u );
    CHECK( r.trucks[0].bInUse );
}

// The fossil case in isolation: a stale claim on site A held by a truck that is
// now delivering to B. Clearing A's claim must leave B's delivery alone - the
// release unbinds the truck only where the truck still names the site. (The
// pre-fix reaper cleared only the building side, and that part was RIGHT.)
static void test_fossil_claim_does_not_cancel_another_job( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );
    Claim( r, 7 );
    r.trucks[0].dwDataDW = 555;  // really working for building 555 now

    r.DropClaim( &r.site, 7 );
    CHECK_EQ( r.site.adwClaim[0], 0u );       // fossil cleared off the site
    CHECK_EQ( r.trucks[0].dwDataDW, 555u );   // other job intact
    CHECK( r.trucks[0].bInUse );
    CHECK( !r.InPool( 7 ) );                  // and it was not pooled out from under 555
}

// A second material claimed by a DIFFERENT, live truck still speaks for the
// site: dropping one dead claim must not drop the live one or strand the site.
static void test_one_dead_claim_does_not_drop_a_live_one( void )
{
    Router r;
    r.trucks.push_back( Truck( 7, 10, 10 ) );  // idle, claims material 0
    r.trucks.push_back( Truck( 8, 20, 20 ) );  // driving, claims material 1
    r.site.aiWant[0] = 5;
    r.site.aiWant[1] = 5;
    r.site.adwClaim[0] = 7;
    r.site.adwClaim[1] = 8;
    r.trucks[0].dwDataDW = r.site.dwID;
    r.trucks[1].dwDataDW = r.site.dwID;
    r.trucks[0].bInUse = r.trucks[1].bInUse = true;
    r.plBldgsNeed.push_back( r.site.dwID );
    r.plTrucksAvailable.push_back( 99 );

    CHECK( r.ClaimIsLive( &r.trucks[0], &r.site ) );  // arm both clocks
    CHECK( r.ClaimIsLive( &r.trucks[1], &r.site ) );
    r.dwNow += AI_CLAIM_IDLE_MS + 1;
    r.trucks[1].iHexX += 3;  // truck 8 has been driving all along

    CHECK( r.TrucksAreEnroute( &r.site ) );  // truck 8 still speaks for the site
    CHECK_EQ( r.site.adwClaim[0], 0u );      // truck 7's dead claim released
    CHECK_EQ( r.site.adwClaim[1], 8u );      // truck 8's kept
    CHECK_EQ( r.trucks[1].dwDataDW, r.site.dwID );
    CHECK_EQ( r.iDropped, 1 );
}

// ===========================================================================
// (3) SOURCE LINT
// ===========================================================================
static void test_cairoute_carries_the_fix( const std::string& sq )
{
    // the bound, and the predicate both callers consult
    CHECK( Has( sq, "#defineAI_CLAIM_IDLE_MS90000" ) );
    CHECK( Has( sq, "BOOLCAIRouter::ClaimIsLive(CAIUnit*pTruck,CAIUnit*pBldg)" ) );
    CHECK( Has( sq, "if(pTruck->GetDataDW()!=pBldg->GetID())" ) );
    CHECK( Has( sq, "if(!AiSnap::ReadVeh(pTruck->GetID(),snapTruck))" ) );
    CHECK( Has( sq, "pTruck->NoteClaimStill(MAKELPARAM(snapTruck.iHeadX,snapTruck.iHeadY),theGame.GettimeGetTime())" ) );
    CHECK( Has( sq, "return(dwStill<AI_CLAIM_IDLE_MS);" ) );

    // the release, and both call sites going through it
    CHECK( Has( sq, "voidCAIRouter::DropClaim(CAIUnit*pBldg,DWORDdwTruckID)" ) );
    CHECK( Has( sq, "if(ClaimIsLive(pTruck,pBldg))returnTRUE;" ) );
    CHECK( Has( sq, "DropClaim(pBldg,dwTruck);" ) );
    CHECK( Has( sq, "if(!ClaimIsLive(pClaimT,pCAIBldg))" ) );
    CHECK( Has( sq, "DropClaim(pCAIBldg,dwClaim);" ) );

    // every assignment starts its own window
    CHECK( Has( sq, "pTruck->ClearClaimProgress();" ) );
}

// The pre-fix forms must be GONE -- catches a revert that would leave the
// mirror passing while the shipped predicate drops back to the name match.
static void test_cairoute_has_no_frozen_forms( const std::string& sq )
{
    CHECK( !Has( sq, "if(pTruck->GetDataDW()==pBldg->GetID())returnTRUE;" ) );
    CHECK( !Has( sq, "if(pClaimT==NULL||pClaimT->GetDataDW()!=pCAIBldg->GetID())" ) );
}

// DropClaim runs with the site popped off m_plBldgsNeed (FillPriorities) AND
// under a live walk of it (SetPriorities), and must not be able to re-list the
// site or re-pool the truck it just released. Proven on the shipped body text,
// because the mirror cannot prove the production function touches no list.
static void test_dropclaim_touches_no_list( const std::string& sq )
{
    size_t iBeg = sq.find( "voidCAIRouter::DropClaim(CAIUnit*pBldg,DWORDdwTruckID)" );
    CHECK( iBeg != std::string::npos );
    if ( iBeg == std::string::npos ) return;
    size_t iEnd = sq.find( "BOOLCAIRouter::TrucksAreEnroute", iBeg );
    CHECK( iEnd != std::string::npos );
    if ( iEnd == std::string::npos ) return;
    std::string body = sq.substr( iBeg, iEnd - iBeg );
    CHECK( !Has( body.c_str( ), "AddTail" ) );
    CHECK( !Has( body.c_str( ), "m_plBldgsNeed" ) );
    CHECK( !Has( body.c_str( ), "m_plTrucksAvailable" ) );
    // ...and it DOES unbind both ends
    CHECK( Has( body.c_str( ), "pBldg->SetParamDW(i,0);" ) );
    CHECK( Has( body.c_str( ), "pTruck->SetDataDW(0);" ) );
    CHECK( Has( body.c_str( ), "pTruck->ClearClaimProgress();" ) );
    // ...but the truck side ONLY where the truck still names this building, or a
    // fossil claim would cancel that truck's current delivery somewhere else
    CHECK( Has( body.c_str( ), "if(pTruck!=NULL&&pTruck->GetDataDW()==pBldg->GetID())" ) );
}

static void test_caiunit_carries_the_stamp( const std::string& sq )
{
    CHECK( Has( sq, "DWORDNoteClaimStill(DWORDdwHex,DWORDdwNow)" ) );
    CHECK( Has( sq, "if(dwHex!=m_dwClaimHex||m_dwClaimStill==0)" ) );
    CHECK( Has( sq, "m_dwClaimStill=dwNow?dwNow:1;" ) );
    CHECK( Has( sq, "returndwNow-m_dwClaimStill;" ) );
    CHECK( Has( sq, "voidClearClaimProgress(void){m_dwClaimHex=0;m_dwClaimStill=0;}" ) );
    // transient, and initialised on both construction paths
    CHECK( Has( sq, "m_dwClaimHex;" ) );
    CHECK( Has( sq, "m_dwClaimStill;" ) );
    CHECK( Has( sq, "m_dwClaimHex(0)" ) );
    CHECK( Has( sq, "m_dwClaimStill(0)" ) );
}

// The 90s bound is DERIVED from these. If a traffic change moves one of them
// past the bound, the derivation is void and this fails -- which is the point.
static void test_vehicle_h_pins_the_derivation( const std::string& sq )
{
    CHECK( Has( sq, "constintHOLD_FRAMES=240;" ) );
    CHECK( Has( sq, "constintGIVEUP_HOLD_FRAMES=24*30;" ) );
    CHECK( Has( sq, "constintJAM_STUCK_FRAMES=24*30;" ) );
    CHECK( Has( sq, "constintJAM_WINDOW_FRAMES=24*30;" ) );
    CHECK( Has( sq, "constintJAM_STAGGER_FRAMES=24*8;" ) );
}

int main( int argc, char** argv )
{
    // predicate + scene -- always run, need no external input
    test_enroute_truck_keeps_claim( );
    test_idle_truck_loses_claim_at_the_bound( );
    test_traffic_holds_keep_the_claim( );
    test_bound_is_between_holds_and_rescuers( );
    test_job_change_and_death_lose_the_claim( );
    test_observation_is_idempotent( );
    test_assignment_resets_the_window( );

    test_legacy_predicate_strands_the_site( );
    test_idle_claim_releases_site_exactly_once( );
    test_site_readded_once_with_no_spare( );
    test_released_truck_cannot_reclaim_in_the_same_pass( );
    test_scene_traffic_hold_keeps_the_job( );
    test_scene_job_change_releases_at_once( );
    test_fossil_claim_does_not_cancel_another_job( );
    test_one_dead_claim_does_not_drop_a_live_one( );

    // source lint -- each path is optional and skips cleanly
    for ( int i = 1; i < argc && i <= 3; ++i )
    {
        std::string src = ReadAll( argv[i] );
        if ( src.empty( ) )
        {
            std::printf( "[ai_claim] SKIP: cannot read %s\n", argv[i] );
            return 2;
        }
        std::string sq = NoWs( src );
        std::printf( "[ai_claim] linting %s (%d bytes)\n", argv[i], (int)src.size( ) );
        if ( i == 1 )
        {
            test_cairoute_carries_the_fix( sq );
            test_cairoute_has_no_frozen_forms( sq );
            test_dropclaim_touches_no_list( sq );
        }
        else if ( i == 2 )
            test_caiunit_carries_the_stamp( sq );
        else
            test_vehicle_h_pins_the_derivation( sq );
    }
    if ( argc < 2 ) std::printf( "[ai_claim] no source paths given -- lint SKIPPED\n" );

    return microtest::Summary( );
}
