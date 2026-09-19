// edicts.h — Edicts v1 data model (civ-wide + building-scoped policy toggles).
//
// An "edict" is a player-toggled policy that trades a recurring upkeep cost for a
// production/combat/population bonus. The engine levers already exist (global
// production multipliers, per-loop energy/workforce demand, food stockpile drain) —
// edicts just fold a cached multiplier into the existing Get*Prod() chokepoints and
// add upkeep at the per-loop demand-clear points. See docs/plans/edicts-feature-plan.md
// (Part A §3 architecture + Part E operator design).
//
// Scope (operator req #2, §26):
//   EDICT_CIVWIDE      — one active flag per civilization (CPlayer::m_dwEdicts bitmask).
//                        Hosted at the Rocket / Embassy (and some Command Center).
//   EDICT_BLDG_SCOPED  — per-building-instance policy (CBuilding::m_dwBldgPolicy, Part D).
//                        Hosted at Apartments / Offices.
//
// This header is the static definition table only (no game state). CPlayer state +
// RecomputeEdictMults() live in player.h/.cpp; the toggle UI is a section inside
// SDL2BuildingWindow (§27).

#ifndef ENATIONS_EDICTS_H
#define ENATIONS_EDICTS_H

#include "building.h"   // CStructureData::BLDG_TYPE (host buildings)
#include "altoutput.h"  // AltOutput::AltMat (Desperate Measures' per-draft scrounge rates)

enum EdictScope { EDICT_CIVWIDE, EDICT_BLDG_SCOPED };

// Stable ids — also the bit positions in CPlayer::m_dwEdicts (civ-wide) /
// CBuilding::m_dwBldgPolicy (building-scoped). Keep ≤ 32 for the DWORD bitmask
// (widen to a 64-bit mask if the catalog grows past that). Append-only: never
// renumber existing ids (they're serialized as bit positions).
enum EdictId
{
    EDICT_FORTIFY_BORDER = 0,   // Command Center: +fort construction speed, +energy upkeep
    EDICT_NUTRITION,            // Apartment: +population growth, +food drain
    EDICT_MINING_SUBSIDY,       // Office: +mine output, +energy upkeep
    EDICT_RESEARCH_SUBSIDY,     // Office: +research speed, +workforce upkeep, + a cut of the
                                //   spare workforce for more (surplus family, see below)
    EDICT_AUSTERITY,            // Rocket (civ-wide): +construction speed, +workforce upkeep
    EDICT_AGRICULTURAL,         // Office: +farm output, +25% workers required at farms
    EDICT_OVERCLOCKED_GRID,     // Rocket (tech-gated): +all production, +global power upkeep
    EDICT_TURBOCHARGERS,        // Command Center: +move speed / +fuel use (fuel-consuming units)
    EDICT_TOTAL_SURVEILLANCE,   // Command Center: +unit vision, +energy upkeep
    EDICT_DRAFT,                // Command Center: +infantry build speed, +infantry population cost
    EDICT_PRECISION_MINING,     // Office: +mine output, +energy & +worker requirement at mines
    EDICT_MEAT_SHIELD,          // Command Center: buildings take less damage, +worker requirement
    EDICT_AUTO_RESEARCH,        // Office: auto-researches the next-cheapest available tech (behavior flag)
    EDICT_DESPERATE_MEASURES,   // Rocket (civ-wide): scrounge a multi-resource trickle for +100 workers
    // --- surplus-scaled family (see SURPLUS_DRAFT_PCT / CPlayer::ApplySurplusEdicts) ----------
    EDICT_PUBLIC_WORKS,         // Rocket: flat workers + surplus workers -> construction crews
    EDICT_CIVIL_DEFENCE,        // Rocket: flat workers + surplus POWER -> shelters & forts
    EDICT_WAR_FOOTING,          // Command Center: flat POWER + surplus workers -> infantry
    EDICT_COUNT
};

// m_dwEdicts / m_dwBldgPolicy are DWORD bitmasks and the ids ARE the bit positions, so the
// catalog cannot grow past 32 without widening both (and the save field with them).
static_assert( EDICT_COUNT <= 32, "EdictId is a bit position in a DWORD mask - widen m_dwEdicts first" );

struct EdictDef
{
    const char*           name;          // short label for the UI checkbox
    const char*           desc;          // one-line tooltip / effect summary
    CStructureData::BLDG_TYPE   hostBuilding;  // which building's info window hosts the toggle (§25)
    EdictScope            scope;         // civ-wide vs building-scoped (§26)
    int                   researchTopic; // CRsrchArray topic gating this edict (#2, §10): the
                                         // edict row is hidden until GetRsrch(researchTopic).m_bDiscovered

    // Bonus multipliers — 1.0 == no change. Folded into the matching Get*Prod()/
    // construction chokepoints when the edict is active (Phase 2).
    float fConstMult;       // global construction speed
    float fFortConstMult;   // fort-only construction speed (vehicle.cpp ConstructBuilding)
    float fMineMult;        // mine output
    float fRsrchMult;       // research speed (wires the latent m_fRsrchProd lever, RG-1)
    float fPopGrowthMult;   // population growth
    float fFarmMult;        // farm/food output (GetFarmProd — all UTfarm buildings)
    float fGlobalProdMult;  // ALL production (folded into every Get*Prod — Overclocked Grid)
    float fMoveMult;        // movement speed of fuel-consuming units (vehmove.cpp)
    float fVisionMult;      // unit spotting range (CUnit::AssignData; re-clamped to MAX_SPOTTING)
    float fInfBuildMult;    // infantry build speed (CVehicleBuilding::BuildVehicle)
    float fBldgDmgMult;     // building damage TAKEN (Meat Shield; 0.90 = 10% less damage, applied at projbase.cpp)

    // Upkeep — recurring cost while active. Expressed as a fraction of the relevant
    // per-loop base demand (so "+20% energy" scales with empire size) or a flat amount.
    float fEnergyUpkeepPct;     // added to m_iPwrNeed as pct of current need
    float fWorkforceUpkeepPct;  // added to m_iPplNeedBldg as pct of current need
    float fFoodUpkeepPct;       // extra drain in PeopleAndFood as pct of current food use

    // Scoped costs — a multiplier on ONE building/unit's requirement (1.0 == no change).
    // Unlike the upkeep pcts above (which tax the whole empire's demand), these bump only
    // the named requirement at its own site.
    float fFarmWorkerMult;      // farm worker requirement (BuildFarm AddPplNeedBldg; 1.25 = +25%)
    float fFuelMult;            // fuel-consuming unit gas burn (FuelVehicle; 1.5 = +50%)
    float fInfPopMult;          // infantry population cost at build (PplBldgToVeh; 3.0 = +200%)
    float fMineEnergyMult;      // mine power requirement (BuildMine AddPwrNeed; 1.10 = +10%)
    float fMineWorkerMult;      // mine worker requirement (BuildMine AddPplNeedBldg; 1.10 = +10%)
};

// The catalog. Definition in edicts.cpp. Indexed by EdictId; size == EDICT_COUNT.
// Starter "Core set" (Part A §3.1) — the operator is still refining the final list/effects,
// so these values are placeholders chosen to be individually toggle-testable.
extern const EdictDef g_aEdicts[EDICT_COUNT];

// Convenience: is this building type an edict host (so SDL2BuildingWindow shows the section)?
bool EdictHostHasEdicts( CStructureData::BLDG_TYPE bldgType );

// --- Surplus-scaled edicts: the shared cut ---------------------------------------------------
// A "surplus" edict puts the resources the colony is NOT using to work -- but NONE of them is
// purely opportunistic (operator: "nothing should be JUST surplus, it should always be base +
// something"). Every one is a FLAT cost in one resource PLUS a cut of the SURPLUS of a resource,
// and the two need not be the same resource:
//
//    3 Research Subsidy   pct upkeeps + RSRCH_SUBSIDY_DRAFT_PCT% of the spare workers
//   13 Desperate Measures  flat workers + SURPLUS_DRAFT_PCT% of the remaining spare workers
//   14 Public Works        flat workers + SURPLUS_DRAFT_PCT% of the remaining spare workers
//   15 Civil Defence       flat workers + SURPLUS_POWER_PCT% of the remaining surplus POWER
//   16 War Footing         flat POWER   + SURPLUS_DRAFT_PCT% of the remaining spare workers
//
// Research Subsidy is the one whose flat half is not a number but the static pct upkeeps in its
// g_aEdicts row (energy + workforce), charged by RecomputeEdictMults/StartLoop like any other
// edict's; only its surplus half is priced here. It is also the LOWEST id, so it takes its cut
// first (see the walk in ApplySurplusEdicts).
//
// Exactly ONE surplus input each, so an edict's effect scales with that one quantity -- there is
// no min() of two ratios to reason about. The flat cost is charged whether or not there is any
// slack, which means switching an edict on is always a real commitment and CAN push the colony
// into workforce or power deficit; that is the point, not an oversight. The edicts are visited
// in EdictId order and each cut shrinks the pool for the next, so the family together can never
// take more surplus than the colony actually has idle.
//
// Accounting (uniform across the family -- see CPlayer::ApplySurplusEdicts): only the SURPLUS-
// DERIVED cuts are added back when next pump measures the spare. A flat cost stays inside the
// need, because it is a bill the colony is really paying, not idle capacity being borrowed.
const int SURPLUS_DRAFT_PCT = 50;   // pct of the REMAINING spare workforce one edict drafts
const int SURPLUS_POWER_PCT = 50;   // pct of the REMAINING surplus power one edict draws

// The one place a surplus share is computed. Integer, so the sim, the UI readout and the harness
// dump all land on the identical number (a float here would let a readout round the other way).
inline int SurplusShare( int iRemaining, int iPct )
{
    if ( iRemaining <= 0 )
        return ( 0 );
    return ( ( iRemaining * iPct ) / 100 );
}

// How fully an edict's draft/draw meets what it wants: 0.0 at nothing, 1.0 at iFull and above.
// The effect multipliers below are all "max bonus * this", so an edict ramps in smoothly with
// the colony's slack instead of snapping on.
inline float SurplusScale( int iHave, int iFull )
{
    if ( iFull <= 0 )  return ( 1.0f );
    if ( iHave >= iFull ) return ( 1.0f );
    if ( iHave <= 0 )  return ( 0.0f );
    return ( (float)iHave / (float)iFull );
}

// Is this edict priced by CPlayer::ApplySurplusEdicts? The behaviour is keyed on the ID here
// rather than on a new EdictDef field: the surplus edicts each have their OWN formula (not a
// single shared multiplier), so a data field could only ever have been a flag anyway, and the
// UI/harness need the same predicate. Keep in step with the walk in ApplySurplusEdicts.
inline bool EdictIsSurplus( int id )
{
    return ( ( id == EDICT_RESEARCH_SUBSIDY ) || ( id == EDICT_DESPERATE_MEASURES ) ||
             ( id == EDICT_PUBLIC_WORKS ) || ( id == EDICT_CIVIL_DEFENCE ) ||
             ( id == EDICT_WAR_FOOTING ) );
}

// --- Desperate Measures tuning (EDICT_DESPERATE_MEASURES) -----------------------------------
// The rocket conscripts a flat base draft PLUS a cut of the workforce the colony is not using,
// and scrounges proportionally harder for it: a civ sitting on idle population gets more out of
// the edict than one already running flat out, at the SAME resources-per-worker exchange rate.
const int DESPERATE_BASE_DRAFT  = 100;  // workers drafted even with zero spare population
const int DESPERATE_RATE_PER    = 200;  // workers that buy one helping of DESPERATE_BASE_RATES.
                                        // Deliberately NOT the base draft: the base is the floor
                                        // of the CONSCRIPTION, this is the EXCHANGE RATE, and the
                                        // two are tuned against each other. Raising this alone
                                        // makes the edict weaker per worker without changing how
                                        // many workers it takes.

// What the edict scrounges per DESPERATE_RATE_PER workers drafted. The sim scales these by the
// live draft (CBuilding::Operate) and the rocket's info window quotes them scaled the same way,
// so the number on screen and the number credited come from this one table. Def in edicts.cpp.
const int DESPERATE_RATE_LINES = 4;
extern const AltOutput::AltMat DESPERATE_BASE_RATES[DESPERATE_RATE_LINES];

// --- Public Works tuning (EDICT_PUBLIC_WORKS) ------------------------------------------------
// Flat workers + surplus workers; the bonus scales with the TOTAL of the two.
const int PUBLIC_WORKS_BASE_DRAFT = 100;  // workers conscripted even with zero spare population
const int PUBLIC_WORKS_FULL_DRAFT = 300;  // total drafted workers that buy the FULL bonus
const int PUBLIC_WORKS_MAX_PCT    = 30;   // max +pct build speed (= +1% per 10 drafted at full)

// --- Civil Defence tuning (EDICT_CIVIL_DEFENCE) ----------------------------------------------
// Population + surplus ENERGY: the flat cost is workers, the scaling input is spare power, so
// the effect is set by the POWER drawn (drafted workers buy nothing on their own here).
const int CIVDEF_BASE_DRAFT   = 100;  // workers manning the shelters, spare population or not
const int CIVDEF_FULL_POWER   = 150;  // surplus power drawn for the full effect
const int CIVDEF_MAX_DMG_PCT  = 20;   // max pct of building damage TAKEN removed
const int CIVDEF_MAX_FORT_PCT = 30;   // max +pct fortification build speed

// --- War Footing tuning (EDICT_WAR_FOOTING) --------------------------------------------------
// Energy + surplus PEOPLE: the mirror image of Civil Defence -- flat power bill, effect set by
// the workers drafted.
const int WARFOOT_BASE_POWER = 30;    // power the war effort burns, surplus power or not
const int WARFOOT_FULL_DRAFT = 400;   // drafted workers for the full effect
const int WARFOOT_MAX_INF_PCT = 100;  // max +pct infantry build speed

// --- Research Subsidy surplus half (EDICT_RESEARCH_SUBSIDY) ----------------------------------
// Its flat half is the static fRsrchMult/upkeep pcts in its g_aEdicts row (+30% research for
// +25% power and +15% workers civ-wide). On top of that it seconds a cut of the idle workforce
// to the labs for up to RSRCH_SUBSIDY_MAX_PCT more. The draft pct is deliberately smaller than
// SURPLUS_DRAFT_PCT: this edict is cheap, always available early, and must not be the one that
// eats the whole idle pool before the rocket/command-center edicts get a look at it.
const int RSRCH_SUBSIDY_DRAFT_PCT = 25;   // pct of the REMAINING spare workforce it drafts
const int RSRCH_SUBSIDY_FULL_DRAFT = 300; // drafted workers for the full EXTRA bonus
const int RSRCH_SUBSIDY_MAX_PCT    = 20;  // max EXTRA research pct, on top of the static +30%

#endif // ENATIONS_EDICTS_H
