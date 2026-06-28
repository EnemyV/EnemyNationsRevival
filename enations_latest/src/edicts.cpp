// edicts.cpp — Edicts v1 static catalog (see edicts.h).
//
// Starter "Core set" (Part A §3.1). Values are placeholders pending the operator's
// final list/effects; each is chosen so it can be toggle-tested in isolation. Fields:
//   name, desc, hostBuilding, scope, researchTopic,
//   fConstMult, fFortConstMult, fMineMult, fRsrchMult, fPopGrowthMult,
//   fEnergyUpkeepPct, fWorkforceUpkeepPct, fFoodUpkeepPct
//
// researchTopic (#2, feature-plan §10 path-A): each edict is gated behind an EXISTING
// CRsrchArray topic (no ENATIONS.DAT change) — the edict row stays hidden until the owner
// has discovered that topic. Thematic map (fortification is the doc's explicit entry; the
// core-4 assigned to their nearest existing topic per win [05:06Z] — adjust constants if the
// operator wants different topics).

#include "stdafx.h"
#include "edicts.h"
#include "research.h"   // CRsrchArray topic enum (research-gating, #2)

const EdictDef g_aEdicts[EDICT_COUNT] =
{
    // EDICT_FORTIFY_BORDER — Command Center, civ-wide combat policy.
    { "Fortify Border", "Civ-wide: +50% fortification build speed.\nCost: +20% power use, +15% more workers.",
      CStructureData::command_center, EDICT_CIVWIDE, CRsrchArray::fortification,
      1.0f, 1.5f, 1.0f, 1.0f, 1.0f,   /*bonus*/
      0.20f, 0.15f, 0.0f },           /*upkeep: energy, workforce, food*/

    // EDICT_NUTRITION — Apartment-hosted, civ-wide population policy. Folds
    // m_fEdictPopGrowthMult into GetPopGrowth() per plan §2.1/§3.2 (a global
    // production multiplier; the earlier EDICT_BLDG_SCOPED tag left it inert —
    // RecomputeEdictMults only folds civ-wide edicts).
    { "Nutrition Program", "Civ-wide: +30% population growth.\nCost: +75% food consumption.",
      CStructureData::apartment, EDICT_CIVWIDE, CRsrchArray::farm_1,
      1.0f, 1.0f, 1.0f, 1.0f, 1.30f,
      0.0f, 0.0f, 0.75f },

    // EDICT_MINING_SUBSIDY — Office-hosted, civ-wide economy policy. Folds
    // m_fEdictMineMult into GetMineProd() (the plan's vertical-slice edict, §6).
    { "Mining Subsidy", "Civ-wide: +25% output from all mines.\nCost: +25% power use, +15% more workers.",
      CStructureData::office, EDICT_CIVWIDE, CRsrchArray::mine_1,
      1.0f, 1.0f, 1.25f, 1.0f, 1.0f,
      0.25f, 0.15f, 0.0f },

    // EDICT_RESEARCH_SUBSIDY — Office-hosted, civ-wide economy policy. Folds
    // m_fEdictRsrchMult into GetRsrchMult() (plan §2.1; pairs with the RG-1 lever).
    { "Research Subsidy", "Civ-wide: +30% research speed.\nCost: +25% power use, +15% more workers.",
      CStructureData::office, EDICT_CIVWIDE, CRsrchArray::advanced_facilities,
      1.0f, 1.0f, 1.0f, 1.30f, 1.0f,
      0.25f, 0.15f, 0.0f },   // energy magnitude operator-tunable (catalog values are placeholders)

    // EDICT_AUSTERITY — Rocket, civ-wide (lost if the rocket is destroyed, §29).
    { "Austerity Drive", "Civ-wide: +20% build speed (all buildings).\nCost: every building needs +30% more workers.\nLost if rocket destroyed.",
      CStructureData::rocket, EDICT_CIVWIDE, CRsrchArray::const_1,
      1.20f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.30f, 0.0f },
};

bool EdictHostHasEdicts( CStructureData::BLDG_TYPE bldgType )
{
    for ( int i = 0; i < EDICT_COUNT; ++i )
        if ( g_aEdicts[i].hostBuilding == bldgType )
            return true;
    return false;
}
