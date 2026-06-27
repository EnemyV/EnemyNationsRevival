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
    EDICT_RESEARCH_SUBSIDY,     // Office: +research speed, +workforce upkeep
    EDICT_AUSTERITY,            // Rocket (civ-wide): +construction speed, +workforce upkeep
    EDICT_COUNT
};

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

    // Upkeep — recurring cost while active. Expressed as a fraction of the relevant
    // per-loop base demand (so "+20% energy" scales with empire size) or a flat amount.
    float fEnergyUpkeepPct;     // added to m_iPwrNeed as pct of current need
    float fWorkforceUpkeepPct;  // added to m_iPplNeedBldg as pct of current need
    float fFoodUpkeepPct;       // extra drain in PeopleAndFood as pct of current food use
};

// The catalog. Definition in edicts.cpp. Indexed by EdictId; size == EDICT_COUNT.
// Starter "Core set" (Part A §3.1) — the operator is still refining the final list/effects,
// so these values are placeholders chosen to be individually toggle-testable.
extern const EdictDef g_aEdicts[EDICT_COUNT];

// Convenience: is this building type an edict host (so SDL2BuildingWindow shows the section)?
bool EdictHostHasEdicts( CStructureData::BLDG_TYPE bldgType );

#endif // ENATIONS_EDICTS_H
