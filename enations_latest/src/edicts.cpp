// edicts.cpp — Edicts v1 static catalog (see edicts.h).
//
// Starter "Core set" (Part A §3.1). Values are placeholders pending the operator's
// final list/effects; each is chosen so it can be toggle-tested in isolation. Fields:
//   name, desc, hostBuilding, scope,
//   fConstMult, fFortConstMult, fMineMult, fRsrchMult, fPopGrowthMult,
//   fEnergyUpkeepPct, fWorkforceUpkeepPct, fFoodUpkeepPct

#include "stdafx.h"
#include "edicts.h"

const EdictDef g_aEdicts[EDICT_COUNT] =
{
    // EDICT_FORTIFY_BORDER — Command Center, civ-wide combat policy.
    { "Fortify Border", "+50% fort construction speed; +20% energy upkeep",
      CStructureData::command_center, EDICT_CIVWIDE,
      1.0f, 1.5f, 1.0f, 1.0f, 1.0f,   /*bonus*/
      0.20f, 0.0f, 0.0f },            /*upkeep*/

    // EDICT_NUTRITION — Apartment-hosted, civ-wide population policy. Folds
    // m_fEdictPopGrowthMult into GetPopGrowth() per plan §2.1/§3.2 (a global
    // production multiplier; the earlier EDICT_BLDG_SCOPED tag left it inert —
    // RecomputeEdictMults only folds civ-wide edicts).
    { "Nutrition Program", "+30% population growth; +20% food upkeep",
      CStructureData::apartment, EDICT_CIVWIDE,
      1.0f, 1.0f, 1.0f, 1.0f, 1.30f,
      0.0f, 0.0f, 0.20f },

    // EDICT_MINING_SUBSIDY — Office-hosted, civ-wide economy policy. Folds
    // m_fEdictMineMult into GetMineProd() (the plan's vertical-slice edict, §6).
    { "Mining Subsidy", "+25% mine output; +20% energy upkeep",
      CStructureData::office, EDICT_CIVWIDE,
      1.0f, 1.0f, 1.25f, 1.0f, 1.0f,
      0.20f, 0.0f, 0.0f },

    // EDICT_RESEARCH_SUBSIDY — Office-hosted, civ-wide economy policy. Folds
    // m_fEdictRsrchMult into GetRsrchMult() (plan §2.1; pairs with the RG-1 lever).
    { "Research Subsidy", "+30% research speed; +15% workforce upkeep",
      CStructureData::office, EDICT_CIVWIDE,
      1.0f, 1.0f, 1.0f, 1.30f, 1.0f,
      0.0f, 0.15f, 0.0f },

    // EDICT_AUSTERITY — Rocket, civ-wide (lost if the rocket is destroyed, §29).
    { "Austerity Drive", "+20% construction speed; +15% workforce upkeep",
      CStructureData::rocket, EDICT_CIVWIDE,
      1.20f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.15f, 0.0f },
};

bool EdictHostHasEdicts( CStructureData::BLDG_TYPE bldgType )
{
    for ( int i = 0; i < EDICT_COUNT; ++i )
        if ( g_aEdicts[i].hostBuilding == bldgType )
            return true;
    return false;
}
