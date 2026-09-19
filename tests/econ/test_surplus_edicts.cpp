// test_surplus_edicts.cpp -- standalone fixture for the surplus-scaled edict family.
//
// The shipped arithmetic lives in two places:
//   * SurplusShare / SurplusScale                -- enations_latest/src/edicts.h
//   * the sequential-share walk + the spare calc -- CPlayer::ApplySurplusEdicts (player.cpp)
//
// Neither can be linked here (edicts.h drags in building.h, and the walk is a CPlayer method
// touching game globals), so this file MIRRORS them as pure functions. A hand mirror can drift
// from the production text, so each mirror's header comment cites the exact production lines it
// stands in for -- read them side by side whenever either is touched.
//
// What it pins:
//   1. SurplusShare is a floor-division cut that never goes negative and never exceeds its pool.
//   2. SurplusScale is clamped to [0,1] and monotone.
//   3. The sequential walk is BOUNDED: however many surplus edicts are active, they can never
//      between them draft more than the spare they were handed (the property that keeps the
//      family from pushing a colony into a workforce deficit it did not choose).
//   4. Desperate Measures' flat base comes out of the spare FIRST, so a colony with no slack
//      pays exactly DESPERATE_BASE_DRAFT -- what the edict has always cost.
//   5. The fixed point: fed back its own previous draw (the m_iSurplusPplLast add-back), the
//      walk computes the SAME answer every pump while the economy holds, instead of the
//      oscillation a naive pct of "have - need" produces. The naive form is run alongside as a
//      positive control, to show this fixture can actually see the oscillation it claims is gone.
//
// Exit: 0 all pass, 1 a check failed.

#include "../ai/microtest.h"

#include <cstdio>

// --- constants, mirroring enations_latest/src/edicts.h ---------------------------------------
static const int SURPLUS_DRAFT_PCT    = 50;
static const int SURPLUS_POWER_PCT    = 50;
static const int DESPERATE_BASE_DRAFT = 100;

// ---------------------------------------------------------------------------
// Mirrors edicts.h:
//
//     inline int SurplusShare( int iRemaining, int iPct )
//     {
//         if ( iRemaining <= 0 ) return ( 0 );
//         return ( ( iRemaining * iPct ) / 100 );
//     }
// ---------------------------------------------------------------------------
static int SurplusShare( int iRemaining, int iPct )
{
    if ( iRemaining <= 0 )
        return ( 0 );
    return ( ( iRemaining * iPct ) / 100 );
}

// ---------------------------------------------------------------------------
// Mirrors edicts.h:
//
//     inline float SurplusScale( int iHave, int iFull )
//     {
//         if ( iFull <= 0 )  return ( 1.0f );
//         if ( iHave >= iFull ) return ( 1.0f );
//         if ( iHave <= 0 )  return ( 0.0f );
//         return ( (float)iHave / (float)iFull );
//     }
// ---------------------------------------------------------------------------
static float SurplusScale( int iHave, int iFull )
{
    if ( iFull <= 0 )     return ( 1.0f );
    if ( iHave >= iFull ) return ( 1.0f );
    if ( iHave <= 0 )     return ( 0.0f );
    return ( (float)iHave / (float)iFull );
}

// What one pump of the walk charged.
struct WalkOut
{
    int desp;      // Desperate Measures draft (base + cut)
    int others;    // everything the other people-drafting edicts took
    int totalPpl;  // desp + others  == what goes into m_iSurplusPplTick
    int totalPwr;  // the surplus-power draws (Civil Defence / War Footing)
};

// ---------------------------------------------------------------------------
// Mirrors the people/power walk of CPlayer::ApplySurplusEdicts (player.cpp): each active edict
// in EdictId order takes SURPLUS_DRAFT_PCT / SURPLUS_POWER_PCT of what is LEFT, and Desperate
// Measures' flat base comes out of the spare before its percentage:
//
//     lSparePpl -= DESPERATE_BASE_DRAFT;  if ( lSparePpl < 0 ) lSparePpl = 0;
//     int iCut = SurplusShare( (int)lSparePpl, SURPLUS_DRAFT_PCT );
//     lSparePpl -= iCut;
//     m_iDespDraft = DESPERATE_BASE_DRAFT + iCut;
//     ...
//     int iDraft = SurplusShare( (int)lSparePpl, SURPLUS_DRAFT_PCT );  lSparePpl -= iDraft;
//
// nOtherPpl = how many of the four people-drafting followers are active (0..4);
// nPwr      = how many of the two surplus-power drawers are active (0..2).
// ---------------------------------------------------------------------------
static WalkOut Walk( int sparePpl, int sparePwr, bool bDesperate, int nOtherPpl, int nPwr )
{
    WalkOut w = { 0, 0, 0, 0 };
    if ( sparePpl < 0 ) sparePpl = 0;
    if ( sparePwr < 0 ) sparePwr = 0;

    if ( bDesperate )
    {
        sparePpl -= DESPERATE_BASE_DRAFT;
        if ( sparePpl < 0 ) sparePpl = 0;
        int iCut = SurplusShare( sparePpl, SURPLUS_DRAFT_PCT );
        sparePpl -= iCut;
        w.desp = DESPERATE_BASE_DRAFT + iCut;
    }
    for ( int i = 0; i < nOtherPpl; i++ )
    {
        int iDraft = SurplusShare( sparePpl, SURPLUS_DRAFT_PCT );
        sparePpl -= iDraft;
        w.others += iDraft;
    }
    for ( int i = 0; i < nPwr; i++ )
    {
        int iDraw = SurplusShare( sparePwr, SURPLUS_POWER_PCT );
        sparePwr -= iDraw;
        w.totalPwr += iDraw;
    }
    w.totalPpl = w.desp + w.others;
    return w;
}

// ---------------------------------------------------------------------------
// The spare, "as if the surplus edicts were not running" -- mirrors ApplySurplusEdicts:
//
//     lSparePpl = m_iPplBldg - ( m_iPplNeedLast - m_iSurplusPplLast );
//
// needLast < 0 (no finished pump yet) => 0 spare.
// ---------------------------------------------------------------------------
static int SpareAsIfNotRunning( int have, int needLast, int ownDrawLast )
{
    if ( needLast < 0 )
        return ( 0 );
    int spare = have - ( needLast - ownDrawLast );
    return ( spare < 0 ) ? 0 : spare;
}

int main( )
{
    // -------- 1. SurplusShare --------
    CHECK_EQ( SurplusShare( 0, SURPLUS_DRAFT_PCT ), 0 );
    CHECK_EQ( SurplusShare( -1000, SURPLUS_DRAFT_PCT ), 0 );
    CHECK_EQ( SurplusShare( 1, SURPLUS_DRAFT_PCT ), 0 );     // floors, never rounds up
    CHECK_EQ( SurplusShare( 300, SURPLUS_DRAFT_PCT ), 150 );
    CHECK_EQ( SurplusShare( 301, SURPLUS_DRAFT_PCT ), 150 );
    for ( int pool = 0; pool <= 5000; pool += 7 )
    {
        int cut = SurplusShare( pool, SURPLUS_DRAFT_PCT );
        CHECK( cut >= 0 );
        CHECK( cut <= pool );
    }

    // -------- 2. SurplusScale --------
    CHECK( SurplusScale( 0, 300 ) == 0.0f );
    CHECK( SurplusScale( -50, 300 ) == 0.0f );
    CHECK( SurplusScale( 300, 300 ) == 1.0f );
    CHECK( SurplusScale( 30000, 300 ) == 1.0f );   // clamped: no runaway bonus
    CHECK( SurplusScale( 150, 300 ) == 0.5f );
    CHECK( SurplusScale( 7, 0 ) == 1.0f );         // degenerate iFull is treated as satisfied
    float prev = -1.0f;
    for ( int have = 0; have <= 600; have += 3 )
    {
        float s = SurplusScale( have, 300 );
        CHECK( s >= 0.0f && s <= 1.0f );
        CHECK( s >= prev );                        // monotone non-decreasing
        prev = s;
    }

    // -------- 3. the walk is bounded --------
    // However many are active, the family cannot draft more people than the spare it was given
    // (plus Desperate's flat base, which is the edict's own floor and deliberately outside the
    // spare). Same for power, with no flat part at all.
    for ( int spare = 0; spare <= 4000; spare += 13 )
        for ( int nOther = 0; nOther <= 4; nOther++ )
            for ( int d = 0; d <= 1; d++ )
            {
                WalkOut w = Walk( spare, spare, d != 0, nOther, 2 );
                CHECK( w.totalPpl >= 0 );
                CHECK( w.totalPpl <= spare + ( d ? DESPERATE_BASE_DRAFT : 0 ) );
                CHECK( w.totalPwr >= 0 );
                CHECK( w.totalPwr <= spare );
                if ( !d ) CHECK_EQ( w.desp, 0 );            // inactive edicts draft nothing
                if ( nOther == 0 ) CHECK_EQ( w.others, 0 );
            }

    // -------- 4. Desperate's flat base comes out of the spare first --------
    // A colony with no slack pays exactly the base, and nothing else can take a cut behind it.
    {
        WalkOut w = Walk( 0, 0, true, 4, 2 );
        CHECK_EQ( w.desp, DESPERATE_BASE_DRAFT );
        CHECK_EQ( w.others, 0 );
        CHECK_EQ( w.totalPwr, 0 );
    }
    {
        WalkOut w = Walk( DESPERATE_BASE_DRAFT, 0, true, 4, 0 );  // exactly the base, no more
        CHECK_EQ( w.desp, DESPERATE_BASE_DRAFT );
        CHECK_EQ( w.others, 0 );
    }
    {
        // 1100 spare: base 100 out first, then 50% of the remaining 1000 = 500.
        WalkOut w = Walk( 1100, 0, true, 0, 0 );
        CHECK_EQ( w.desp, 600 );
        // With Public Works behind it: 500 left, it takes 250, leaving 250.
        WalkOut w2 = Walk( 1100, 0, true, 1, 0 );
        CHECK_EQ( w2.desp, 600 );
        CHECK_EQ( w2.others, 250 );
        CHECK_EQ( w2.totalPpl, 850 );
        CHECK( w2.totalPpl <= 1100 + DESPERATE_BASE_DRAFT );
    }

    // -------- 5. the fixed point (and a positive control that shows the oscillation) --------
    // The colony: 3000 workers on hand, 2000 of BASE need (buildings, excluding our own draft).
    // Each pump the total need reported back is base + what we drafted last pump.
    const int kHave = 3000, kBase = 2000;
    {
        int needLast = -1, ownLast = 0;
        int seen[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        for ( int pump = 0; pump < 8; pump++ )
        {
            int spare = SpareAsIfNotRunning( kHave, needLast, ownLast );
            WalkOut w = Walk( spare, 0, true, 1, 0 );
            seen[pump] = w.totalPpl;
            // next pump reads this pump's FINISHED need and our own draw
            needLast = kBase + w.totalPpl;
            ownLast  = w.totalPpl;
        }
        // Pump 0 has no finished pump to read: spare 0, so only Desperate's flat base.
        CHECK_EQ( seen[0], DESPERATE_BASE_DRAFT );
        // From the first pump that HAS a finished total, the answer never moves again.
        for ( int i = 2; i < 8; i++ )
            CHECK_EQ( seen[i], seen[1] );
        CHECK( seen[1] > DESPERATE_BASE_DRAFT );   // it really is drafting the surplus
    }
    {
        // Positive control: the naive version -- spare = have - need with NO add-back of our own
        // previous draw. If this fixture's loop could not see an oscillation, the check above
        // would prove nothing.
        int needLast = -1, ownLast = 0;
        int seen[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        for ( int pump = 0; pump < 8; pump++ )
        {
            int spare = SpareAsIfNotRunning( kHave, needLast, 0 /* NO add-back */ );
            WalkOut w = Walk( spare, 0, true, 1, 0 );
            seen[pump] = w.totalPpl;
            needLast = kBase + w.totalPpl;
            ownLast  = w.totalPpl;
        }
        (void)ownLast;
        bool bOscillates = false;
        for ( int i = 2; i < 8; i++ )
            if ( seen[i] != seen[i - 1] )
                bOscillates = true;
        CHECK( bOscillates );
    }

    return microtest::Summary( );
}
