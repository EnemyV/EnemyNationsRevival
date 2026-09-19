// test_surplus_edicts.cpp -- standalone fixture for the surplus-scaled edict family.
//
// The shipped arithmetic lives in two places:
//   * SurplusShare / SurplusScale                -- enations_latest/src/edicts.h
//   * the flat-cost + cut walk and the spare calc -- CPlayer::ApplySurplusEdicts (player.cpp)
//
// Neither can be linked here (edicts.h drags in building.h, and the walk is a CPlayer method
// touching game globals), so this file MIRRORS them as pure functions. A hand mirror can drift
// from the production text, so each mirror's header comment cites the exact production lines it
// stands in for -- read them side by side whenever either is touched.
//
// THE RULE BEING PINNED (operator: "nothing should be JUST surplus, it should always be base +
// something"): every surplus edict is a FLAT cost in one resource PLUS a cut of the surplus of
// one resource, and the two need not be the same resource. Per edict, in walk order:
//
//    3 Research Subsidy     static pct upkeeps + 25% of the spare workers (it is the LOWEST id,
//                           so it takes its cut first, before Desperate Measures)
//   13 Desperate Measures   flat 100 workers + 50% of the remaining spare workers
//   14 Public Works         flat 100 workers + 50% of the remaining spare workers
//   15 Civil Defence        flat 100 workers + 50% of the remaining surplus POWER
//   16 War Footing          flat  30 power   + 50% of the remaining spare workers
//
// Research Subsidy's flat half is not a number this walk books: it is the static +30% research /
// +25% power / +15% workers in its g_aEdicts row, charged by RecomputeEdictMults + StartLoop. So
// it contributes nothing to FlatPpl/FlatPwr here, only a cut.
//
// Accounting, uniform: only the SURPLUS-DERIVED cuts are added back when the next pump measures
// the spare; a flat cost stays inside the need. So the spare handed to a pump has already had
// every flat cost deducted, and the walk must not deduct them a second time.
//
// What it pins:
//   1. SurplusShare is a floor-division cut that never goes negative and never exceeds its pool.
//   2. SurplusScale is clamped to [0,1] and monotone.
//   3. With ZERO spare of either resource, every active edict still charges exactly its flat
//      part -- and nothing more. That is the commitment the operator asked for.
//   4. With spare S, each edict charges flat_i + its pct of what remains after the earlier cuts
//      (25% for Research Subsidy, 50% for the rest), and the cuts together never exceed S (the
//      flat parts are on top, by design). 4a pins the walk ORDER that "what remains" depends on.
//   5a. Research Subsidy's extra research multiplier: 1.0 + 20% * SurplusScale(draft, 300).
//   5. The fixed point, for BOTH pools with all five edicts on: fed back its own previous cuts,
//      the walk computes the SAME answer every pump while the economy holds. The naive form
//      (no add-back) is run alongside as a positive control, to show this fixture can actually
//      see the oscillation it claims is gone.
//
// Exit: 0 all pass, 1 a check failed.

#include "../ai/microtest.h"

#include <cstdio>

// --- constants, mirroring enations_latest/src/edicts.h ---------------------------------------
static const int SURPLUS_DRAFT_PCT      = 50;
static const int SURPLUS_POWER_PCT      = 50;
static const int DESPERATE_BASE_DRAFT   = 100;
static const int PUBLIC_WORKS_BASE_DRAFT = 100;
static const int PUBLIC_WORKS_FULL_DRAFT = 300;
static const int CIVDEF_BASE_DRAFT      = 100;
static const int CIVDEF_FULL_POWER      = 150;
static const int WARFOOT_BASE_POWER     = 30;
static const int WARFOOT_FULL_DRAFT     = 400;
static const int RSRCH_SUBSIDY_DRAFT_PCT  = 25;
static const int RSRCH_SUBSIDY_FULL_DRAFT = 300;
static const int RSRCH_SUBSIDY_MAX_PCT    = 20;

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

// Which edicts are on, in walk order (EdictId 3, then 13..16).
struct Active
{
    bool rsub, desp, pubworks, civdef, warfoot;
};

// What one pump of the walk charged. ppl*/pwr* are the TOTALS booked into m_iPplNeedBldg /
// m_iPwrNeed (flat + cut); cutPpl/cutPwr are the surplus-derived halves only -- what goes into
// m_iSurplusPplTick / m_iSurplusPwrTick for next pump's add-back.
struct WalkOut
{
    int rsubPpl, despPpl, pubPpl, civPpl, civPwr, warPpl, warPwr;
    int ppl, pwr;        // totals booked
    int cutPpl, cutPwr;  // surplus-derived halves
    float rsubMult;      // m_fSurplusRsrchMult (1.0 when the edict is off)
};

// ---------------------------------------------------------------------------
// Mirrors the walk in CPlayer::ApplySurplusEdicts (player.cpp), edict by edict:
//
//     if ( IsEdictActive( EDICT_RESEARCH_SUBSIDY ) ) {
//         int iCut = SurplusShare( (int)lSparePpl, RSRCH_SUBSIDY_DRAFT_PCT );
//         lSparePpl -= iCut;
//         m_iRsrchSubDraft = iCut;  AddPplNeedBldg( iCut );  m_iSurplusPplTick += iCut;
//         m_fSurplusRsrchMult = 1.0f + ( RSRCH_SUBSIDY_MAX_PCT / 100.0f ) *
//                                          SurplusScale( iCut, RSRCH_SUBSIDY_FULL_DRAFT ); }
//     if ( IsEdictActive( EDICT_DESPERATE_MEASURES ) ) {
//         int iCut = SurplusShare( (int)lSparePpl, SURPLUS_DRAFT_PCT );
//         lSparePpl -= iCut;
//         m_iDespDraft = DESPERATE_BASE_DRAFT + iCut;
//         AddPplNeedBldg( m_iDespDraft );  m_iSurplusPplTick += iCut; }
//     ... Public Works the same shape with PUBLIC_WORKS_BASE_DRAFT ...
//     if ( IsEdictActive( EDICT_CIVIL_DEFENCE ) ) {
//         int iCutPwr = SurplusShare( (int)lSparePwr, SURPLUS_POWER_PCT );  lSparePwr -= iCutPwr;
//         m_iCivDefDraft = CIVDEF_BASE_DRAFT;  m_iCivDefPower = iCutPwr; ... }
//     if ( IsEdictActive( EDICT_WAR_FOOTING ) ) {
//         int iCutPpl = SurplusShare( (int)lSparePpl, SURPLUS_DRAFT_PCT );  lSparePpl -= iCutPpl;
//         m_iWarFootDraft = iCutPpl;  m_iWarFootPower = WARFOOT_BASE_POWER; ... }
// ---------------------------------------------------------------------------
static WalkOut Walk( int sparePpl, int sparePwr, const Active& a )
{
    WalkOut w = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0f };
    if ( sparePpl < 0 ) sparePpl = 0;
    if ( sparePwr < 0 ) sparePwr = 0;

    if ( a.rsub )   // EdictId 3 -- FIRST in the walk, cuts the untouched pool
    {
        int cut = SurplusShare( sparePpl, RSRCH_SUBSIDY_DRAFT_PCT );
        sparePpl -= cut;
        w.rsubPpl = cut;            // no flat draft: its flat half is the static pct upkeeps
        w.cutPpl += cut;
        w.rsubMult = 1.0f + ( RSRCH_SUBSIDY_MAX_PCT / 100.0f ) *
                                SurplusScale( cut, RSRCH_SUBSIDY_FULL_DRAFT );
    }
    if ( a.desp )
    {
        int cut = SurplusShare( sparePpl, SURPLUS_DRAFT_PCT );
        sparePpl -= cut;
        w.despPpl = DESPERATE_BASE_DRAFT + cut;
        w.cutPpl += cut;
    }
    if ( a.pubworks )
    {
        int cut = SurplusShare( sparePpl, SURPLUS_DRAFT_PCT );
        sparePpl -= cut;
        w.pubPpl = PUBLIC_WORKS_BASE_DRAFT + cut;
        w.cutPpl += cut;
    }
    if ( a.civdef )
    {
        int cut = SurplusShare( sparePwr, SURPLUS_POWER_PCT );
        sparePwr -= cut;
        w.civPpl = CIVDEF_BASE_DRAFT;   // flat, no people cut
        w.civPwr = cut;
        w.cutPwr += cut;
    }
    if ( a.warfoot )
    {
        int cut = SurplusShare( sparePpl, SURPLUS_DRAFT_PCT );
        sparePpl -= cut;
        w.warPpl = cut;
        w.warPwr = WARFOOT_BASE_POWER;  // flat, no power cut
        w.cutPpl += cut;
    }
    w.ppl = w.rsubPpl + w.despPpl + w.pubPpl + w.civPpl + w.warPpl;
    w.pwr = w.civPwr + w.warPwr;
    return w;
}

// The flat halves the given set charges, independent of any slack.
static int FlatPpl( const Active& a )
{
    return ( ( a.desp ? DESPERATE_BASE_DRAFT : 0 ) + ( a.pubworks ? PUBLIC_WORKS_BASE_DRAFT : 0 ) +
             ( a.civdef ? CIVDEF_BASE_DRAFT : 0 ) );
}
static int FlatPwr( const Active& a )
{
    // Research Subsidy's power cost is a PCT upkeep charged in StartLoop, not a flat bill booked
    // by this walk, so it is deliberately absent here.
    return ( a.warfoot ? WARFOOT_BASE_POWER : 0 );
}

// ---------------------------------------------------------------------------
// The spare, "as if the surplus edicts were not borrowing it" -- mirrors ApplySurplusEdicts:
//
//     lSparePpl = m_iPplBldg     - ( m_iPplNeedLast - m_iSurplusPplLast );
//     lSparePwr = m_iPwrHaveLast - ( m_iPwrNeedLast - m_iSurplusPwrLast );
//
// needLast < 0 (no finished pump yet) => 0 spare. Note only the CUTS are in *SurplusLast, so a
// flat cost stays inside needLast and is correctly absent from the spare.
// ---------------------------------------------------------------------------
static int SpareAsIfNotBorrowed( int have, int needLast, int ownCutLast )
{
    if ( needLast < 0 )
        return ( 0 );
    int spare = have - ( needLast - ownCutLast );
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

    // -------- 3. zero slack: every active edict still charges exactly its flat part --------
    // This is the operator's rule. Note it can (deliberately) put the colony into deficit: the
    // charge does not shrink just because there is nothing spare.
    for ( int mask = 0; mask < 32; mask++ )
    {
        Active a = { ( mask & 1 ) != 0, ( mask & 2 ) != 0, ( mask & 4 ) != 0,
                     ( mask & 8 ) != 0, ( mask & 16 ) != 0 };
        WalkOut w = Walk( 0, 0, a );
        CHECK_EQ( w.ppl, FlatPpl( a ) );
        CHECK_EQ( w.pwr, FlatPwr( a ) );
        CHECK_EQ( w.cutPpl, 0 );
        CHECK_EQ( w.cutPwr, 0 );
        // the individual flat parts, named
        CHECK_EQ( w.despPpl, a.desp ? DESPERATE_BASE_DRAFT : 0 );
        CHECK_EQ( w.pubPpl,  a.pubworks ? PUBLIC_WORKS_BASE_DRAFT : 0 );
        CHECK_EQ( w.civPpl,  a.civdef ? CIVDEF_BASE_DRAFT : 0 );
        CHECK_EQ( w.civPwr,  0 );                               // its surplus half is power
        CHECK_EQ( w.warPpl,  0 );                               // its surplus half is people
        CHECK_EQ( w.warPwr,  a.warfoot ? WARFOOT_BASE_POWER : 0 );
        // Research Subsidy is PURELY surplus in this walk -- with no slack it drafts nobody and
        // its extra research bonus is neutral. Its flat half (the static +30% and the pct
        // upkeeps) is charged elsewhere and is exactly what keeps it a commitment.
        CHECK_EQ( w.rsubPpl, 0 );
        CHECK( w.rsubMult == 1.0f );
    }

    // -------- 4. flat + 50% of what remains, and the cuts stay inside the spare --------
    {
        // All five on, 1000 spare people and 400 spare power. People cuts, in walk order:
        //   Research Subsidy 250 (25% of 1000), Desperate 375 (of 750), Public Works 187 (of
        //   375, floored), War Footing 94 (of 188).   Power cuts: Civil Defence 200 (of 400).
        Active all = { true, true, true, true, true };
        WalkOut w = Walk( 1000, 400, all );
        CHECK_EQ( w.rsubPpl, 250 );
        CHECK_EQ( w.despPpl, DESPERATE_BASE_DRAFT + 375 );
        CHECK_EQ( w.pubPpl,  PUBLIC_WORKS_BASE_DRAFT + 187 );
        CHECK_EQ( w.civPpl,  CIVDEF_BASE_DRAFT );
        CHECK_EQ( w.civPwr,  200 );
        CHECK_EQ( w.warPpl,  94 );
        CHECK_EQ( w.warPwr,  WARFOOT_BASE_POWER );
        CHECK_EQ( w.cutPpl, 250 + 375 + 187 + 94 );
        CHECK_EQ( w.cutPwr, 200 );
        CHECK_EQ( w.ppl, FlatPpl( all ) + w.cutPpl );
        CHECK_EQ( w.pwr, FlatPwr( all ) + w.cutPwr );   // no per-draft power term left in the family
    }

    // -------- 4a. walk ORDER: Research Subsidy (id 3) cuts before Desperate Measures (id 13) --
    // The ids are the order, so the lowest-id edict sees the untouched pool. Pinned by the
    // effect the order HAS -- Desperate's draft shrinks when Research Subsidy is also on -- not
    // just by re-asserting the number, so a walk reordered in player.cpp trips this.
    {
        Active despOnly = { false, true, false, false, false };
        Active both     = { true,  true, false, false, false };
        WalkOut wD = Walk( 1000, 0, despOnly );
        WalkOut wB = Walk( 1000, 0, both );
        CHECK_EQ( wD.despPpl, DESPERATE_BASE_DRAFT + 500 );    // 50% of the whole 1000
        CHECK_EQ( wB.rsubPpl, 250 );                           // 25% of the whole 1000, taken first
        CHECK_EQ( wB.despPpl, DESPERATE_BASE_DRAFT + 375 );    // then 50% of the 750 left
        CHECK( wB.despPpl < wD.despPpl );
        // Its cut is 25%, NOT the family's 50%: deliberately the smaller share.
        CHECK( wB.rsubPpl < SurplusShare( 1000, SURPLUS_DRAFT_PCT ) );
    }

    // -------- 5a. the extra research multiplier ramps 1.00 -> 1.20 over 300 drafted ----------
    {
        Active rs = { true, false, false, false, false };
        CHECK( Walk( 0, 0, rs ).rsubMult == 1.0f );                   // nothing spare, no extra
        // 1200 spare -> 300 drafted -> exactly the cap.
        WalkOut wFull = Walk( 1200, 0, rs );
        CHECK_EQ( wFull.rsubPpl, RSRCH_SUBSIDY_FULL_DRAFT );
        CHECK( wFull.rsubMult == 1.0f + RSRCH_SUBSIDY_MAX_PCT / 100.0f );
        // Past the cap it stays there (SurplusScale clamps) -- no runaway research.
        CHECK( Walk( 100000, 0, rs ).rsubMult == wFull.rsubMult );
        // 600 spare -> 150 drafted -> half the extra.
        CHECK( Walk( 600, 0, rs ).rsubMult == 1.0f + ( RSRCH_SUBSIDY_MAX_PCT / 100.0f ) * 0.5f );
        // Monotone and bounded across the range, and never below neutral.
        float prevM = 0.0f;
        for ( int sp = 0; sp <= 2000; sp += 11 )
        {
            WalkOut w = Walk( sp, 0, rs );
            CHECK( w.rsubMult >= 1.0f );
            CHECK( w.rsubMult <= 1.0f + RSRCH_SUBSIDY_MAX_PCT / 100.0f );
            CHECK( w.rsubMult >= prevM );
            prevM = w.rsubMult;
            CHECK_EQ( w.rsubPpl, SurplusShare( sp, RSRCH_SUBSIDY_DRAFT_PCT ) );
            CHECK_EQ( w.cutPpl, w.rsubPpl );   // its whole charge is surplus-derived: all added back
            CHECK_EQ( w.ppl, w.rsubPpl );      // and it books no flat draft at all
            CHECK_EQ( w.pwr, 0 );              // nor any flat power
        }
    }
    // Swept: the cuts can never exceed the spare they were handed, and the booked total is
    // always exactly the flat part plus the cuts.
    for ( int sp = 0; sp <= 4000; sp += 13 )
        for ( int mask = 0; mask < 32; mask++ )
        {
            Active a = { ( mask & 1 ) != 0, ( mask & 2 ) != 0, ( mask & 4 ) != 0,
                         ( mask & 8 ) != 0, ( mask & 16 ) != 0 };
            WalkOut w = Walk( sp, sp, a );
            CHECK( w.cutPpl >= 0 && w.cutPpl <= sp );
            CHECK( w.cutPwr >= 0 && w.cutPwr <= sp );
            CHECK_EQ( w.ppl, FlatPpl( a ) + w.cutPpl );
            CHECK_EQ( w.pwr, FlatPwr( a ) + w.cutPwr );
        }

    // -------- 5. the fixed point, both pools (and a positive control) --------
    // The colony: 3000 workers / 800 power on hand, against 2000 workers and 500 power of BASE
    // need (everything that is not these edicts). Each pump the totals reported back are that
    // base plus whatever the edicts booked.
    const int kHavePpl = 3000, kBasePpl = 2000;
    const int kHavePwr = 800,  kBasePwr = 500;
    Active all = { true, true, true, true, true };
    {
        int needPplLast = -1, cutPplLast = 0;
        int needPwrLast = -1, cutPwrLast = 0;
        int seenPpl[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        int seenPwr[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        for ( int pump = 0; pump < 8; pump++ )
        {
            int sparePpl = SpareAsIfNotBorrowed( kHavePpl, needPplLast, cutPplLast );
            int sparePwr = SpareAsIfNotBorrowed( kHavePwr, needPwrLast, cutPwrLast );
            WalkOut w = Walk( sparePpl, sparePwr, all );
            seenPpl[pump] = w.ppl;
            seenPwr[pump] = w.pwr;
            needPplLast = kBasePpl + w.ppl;   cutPplLast = w.cutPpl;
            needPwrLast = kBasePwr + w.pwr;   cutPwrLast = w.cutPwr;
        }
        // Pump 0 has no finished pump to read: spare 0, so only the flat parts.
        CHECK_EQ( seenPpl[0], FlatPpl( all ) );
        CHECK_EQ( seenPwr[0], FlatPwr( all ) );
        // PEOPLE settle immediately: from the first pump that has a finished total to read, the
        // answer never moves again.
        for ( int i = 2; i < 8; i++ )
            CHECK_EQ( seenPpl[i], seenPpl[1] );
        // POWER settles immediately too, now that no edict's power bill depends on the PEOPLE
        // cut: the only power terms left are War Footing's flat bill and Civil Defence's cut of
        // the spare power, so the power pool's "need minus own cut" is constant from pump 1 on.
        // (Research Subsidy's power cost is a pct upkeep charged outside this walk.)
        for ( int i = 2; i < 8; i++ )
            CHECK_EQ( seenPwr[i], seenPwr[1] );
        CHECK( seenPpl[1] > FlatPpl( all ) );   // it really is drafting the surplus
        CHECK( seenPwr[1] > FlatPwr( all ) );   // and really is drawing surplus power
    }
    // Swept: over a range of colony sizes and active sets, the walk ALWAYS reaches a fixed point
    // within 4 pumps and stays on it for the rest of the run. One pump is needed to have a
    // finished total to read at all; the rest is slack (both pools now settle at pump 1).
    for ( int havePpl = 2000; havePpl <= 6000; havePpl += 500 )
        for ( int havePwr = 600; havePwr <= 1400; havePwr += 200 )
            for ( int mask = 0; mask < 32; mask++ )
            {
                Active a = { ( mask & 1 ) != 0, ( mask & 2 ) != 0, ( mask & 4 ) != 0,
                             ( mask & 8 ) != 0, ( mask & 16 ) != 0 };
                int needPplLast = -1, cutPplLast = 0;
                int needPwrLast = -1, cutPwrLast = 0;
                int ppl[10] = { 0 }, pwr[10] = { 0 };
                for ( int pump = 0; pump < 10; pump++ )
                {
                    WalkOut w = Walk( SpareAsIfNotBorrowed( havePpl, needPplLast, cutPplLast ),
                                      SpareAsIfNotBorrowed( havePwr, needPwrLast, cutPwrLast ), a );
                    ppl[pump] = w.ppl;
                    pwr[pump] = w.pwr;
                    needPplLast = kBasePpl + w.ppl;   cutPplLast = w.cutPpl;
                    needPwrLast = kBasePwr + w.pwr;   cutPwrLast = w.cutPwr;
                }
                for ( int i = 5; i < 10; i++ )
                {
                    CHECK_EQ( ppl[i], ppl[4] );
                    CHECK_EQ( pwr[i], pwr[4] );
                }
            }

    {
        // Positive control: the naive version -- spare = have - need with NO add-back of our own
        // previous cuts. If this fixture's loop could not see an oscillation, the check above
        // would prove nothing.
        int needPplLast = -1, needPwrLast = -1;
        int seenPpl[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        int seenPwr[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        for ( int pump = 0; pump < 8; pump++ )
        {
            int sparePpl = SpareAsIfNotBorrowed( kHavePpl, needPplLast, 0 /* NO add-back */ );
            int sparePwr = SpareAsIfNotBorrowed( kHavePwr, needPwrLast, 0 /* NO add-back */ );
            WalkOut w = Walk( sparePpl, sparePwr, all );
            seenPpl[pump] = w.ppl;
            seenPwr[pump] = w.pwr;
            needPplLast = kBasePpl + w.ppl;
            needPwrLast = kBasePwr + w.pwr;
        }
        // Compared over the SAME window the settled walk is asserted constant on (pumps 3..7),
        // so this is a fair control and not just the first-pump transient.
        bool bOscPpl = false, bOscPwr = false;
        for ( int i = 3; i < 8; i++ )
        {
            if ( seenPpl[i] != seenPpl[i - 1] ) bOscPpl = true;
            if ( seenPwr[i] != seenPwr[i - 1] ) bOscPwr = true;
        }
        CHECK( bOscPpl );
        CHECK( bOscPwr );
    }

    return microtest::Summary( );
}
