// edicts.cpp — Edicts v1 static catalog (see edicts.h).
//
// Starter "Core set" (Part A §3.1). Values are placeholders pending the operator's
// final list/effects; each is chosen so it can be toggle-tested in isolation. Fields:
//   name, desc, hostBuilding, scope, researchTopic,
//   fConstMult, fFortConstMult, fMineMult, fRsrchMult, fPopGrowthMult, fFarmMult,
//     fGlobalProdMult, fMoveMult, fVisionMult, fInfBuildMult,
//   fEnergyUpkeepPct, fWorkforceUpkeepPct, fFoodUpkeepPct,
//   fFarmWorkerMult, fFuelMult, fInfPopMult, fMineEnergyMult, fMineWorkerMult
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
    // Field order (positional):
    //   bonus : const, fort, mine, rsrch, pop, farm, global, move, vision, infBuild, bldgDmg
    //   upkeep: energy, workforce, food
    //   scoped: farmWorker, fuel, infPop, mineEnergy, mineWorker
    // All bonus/scoped mults default 1.0 (neutral); upkeep pcts default 0.0.

    // EDICT_FORTIFY_BORDER — Command Center, civ-wide combat policy.
    { "Fortify Border", "Civ-wide: +50% fortification build speed.\nCost: +20% power use, +15% more workers.",
      CStructureData::command_center, EDICT_CIVWIDE, CRsrchArray::fortification,
      1.0f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.20f, 0.15f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_NUTRITION — Apartment-hosted, civ-wide population policy. Folds
    // m_fEdictPopGrowthMult into GetPopGrowth() per plan §2.1/§3.2 (a global
    // production multiplier; the earlier EDICT_BLDG_SCOPED tag left it inert —
    // RecomputeEdictMults only folds civ-wide edicts).
    { "Nutrition Program", "Civ-wide: +20% population growth.\nCost: +75% food consumption.",
      CStructureData::apartment, EDICT_CIVWIDE, CRsrchArray::farm_1,
      1.0f, 1.0f, 1.0f, 1.0f, 1.20f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.75f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_MINING_SUBSIDY — Office-hosted, civ-wide economy policy. Folds
    // m_fEdictMineMult into GetMineProd() (the plan's vertical-slice edict, §6).
    { "Mining Subsidy", "Civ-wide: +25% output from all mines.\nCost: +25% power use, +15% more workers.",
      CStructureData::office, EDICT_CIVWIDE, CRsrchArray::mine_1,
      1.0f, 1.0f, 1.25f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.25f, 0.15f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_RESEARCH_SUBSIDY — Office-hosted, civ-wide economy policy. Folds
    // m_fEdictRsrchMult into GetRsrchMult() (plan §2.1; pairs with the RG-1 lever).
    { "Research Subsidy", "Civ-wide: +30% research speed.\nCost: +25% civ-wide power use, +15% civ-wide workers.",
      CStructureData::office, EDICT_CIVWIDE, CRsrchArray::advanced_facilities,
      1.0f, 1.0f, 1.0f, 1.30f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.25f, 0.15f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_AUSTERITY — Rocket, civ-wide (lost if the rocket is destroyed, §29).
    { "Austerity Drive", "Civ-wide: +20% build speed (all buildings).\nCost: every building needs +30% more workers.\nLost if rocket destroyed.",
      CStructureData::rocket, EDICT_CIVWIDE, CRsrchArray::const_1,
      1.20f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.30f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_AGRICULTURAL — Office-hosted, civ-wide economy policy. Folds m_fEdictFarmMult
    // into GetFarmProd() (+10% farm/food output) and bumps only the farm's own worker
    // requirement via fFarmWorkerMult (BuildFarm, +25% → a 4-worker farm needs 5). Lumber
    // mills share UTfarm but their base GetPeople() ≈ 0, so the worker cost lands on food farms.
    { "Agricultural Subsidy", "Civ-wide: +10% output from all farms.\nCost: farms need +25% more workers.",
      CStructureData::office, EDICT_CIVWIDE, CRsrchArray::farm_1,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.10f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.25f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_OVERCLOCKED_GRID — Rocket, civ-wide signature (lost if rocket destroyed, §29).
    // fGlobalProdMult folds into EVERY Get*Prod accessor (+15% all production); the cost is
    // a big global power-demand upkeep. Tech-gated on nuclear (endgame power).
    { "Overclocked Grid", "Civ-wide: +15% to ALL production.\nCost: +30% power use (empire-wide).\nLost if rocket destroyed.",
      CStructureData::rocket, EDICT_CIVWIDE, CRsrchArray::nuclear,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.15f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.30f, 0.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_TURBOCHARGERS — Command Center, civ-wide. fMoveMult scales fuel-consuming units'
    // move speed (vehmove.cpp, scoped != walk); fFuelMult raises their gas burn (FuelVehicle,
    // already scoped to != walk at the call site). Infantry (walk) unaffected. Gate: gas_turbine.
    { "Turbochargers", "Civ-wide: +20% movement speed (fuel-consuming units).\nCost: +50% fuel use for those units.",
      CStructureData::command_center, EDICT_CIVWIDE, CRsrchArray::gas_turbine,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.20f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.0f, 1.50f, 1.0f, 1.0f, 1.0f },

    // EDICT_TOTAL_SURVEILLANCE — Command Center, civ-wide. fVisionMult scales unit spotting
    // range (baked in AssignData + re-derived on toggle; re-clamped to MAX_SPOTTING). Gate: spot_3.
    { "Total Surveillance", "Civ-wide: +20% unit & building vision.\nCost: +30% power use.",
      CStructureData::command_center, EDICT_CIVWIDE, CRsrchArray::spot_3,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.20f, 1.0f, 1.0f,
      0.30f, 0.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_DRAFT — Command Center, civ-wide. fInfBuildMult speeds infantry production
    // (CVehicleBuilding::BuildVehicle, scoped to walk units); fInfPopMult burns extra
    // population when a drafted infantry is built (PplBldgToVeh site). Gate: atk_1.
    { "The Draft", "Civ-wide: +100% infantry build speed.\nCost: drafted infantry cost +200% population.",
      CStructureData::command_center, EDICT_CIVWIDE, CRsrchArray::atk_1,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.0f, 1.0f, 3.0f, 1.0f, 1.0f },

    // EDICT_PRECISION_MINING — Office-hosted, civ-wide economy policy. +5% mine output via the
    // shared fMineMult lever; cost is a FLAT +1 power & +1 worker per producing mine, applied by
    // IsEdictActive in BuildMine (a % surcharge rounded away on a mine's tiny base). The
    // fMineEnergyMult/fMineWorkerMult scoped fields are left neutral (1.0) — now unused. Gate: mine_2.
    { "Precision Mining", "Civ-wide: +5% output from all mines.\nCost: each mine uses +1 power and +1 worker.",
      CStructureData::office, EDICT_CIVWIDE, CRsrchArray::mine_2,
      1.0f, 1.0f, 1.05f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_MEAT_SHIELD — Command Center, civ-wide combat policy. fBldgDmgMult reduces damage
    // TAKEN by buildings (0.90 = 10% less, applied at projbase.cpp hit site — live/toggleable,
    // no stored HP change). Cost is the existing global workforce upkeep (+30% workers). Gate: fortification.
    { "Meat Shield", "Civ-wide: your buildings take 10% less damage.\nCost: +30% more workers (all buildings).",
      CStructureData::command_center, EDICT_CIVWIDE, CRsrchArray::fortification,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.90f,
      0.0f, 0.30f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_AUTO_RESEARCH — Office-hosted, civ-wide. Pure BEHAVIOR flag (all mults neutral):
    // CPlayer::Research() auto-starts the cheapest available topic when idle (see the hook there).
    // No downside cost specified by the operator (flagged). Gate: medium_facilities.
    { "AutoResearch", "Civ-wide: automatically researches the next-cheapest available technology.",
      CStructureData::office, EDICT_CIVWIDE, CRsrchArray::medium_facilities,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_DESPERATE_MEASURES — Rocket, civ-wide BEHAVIOR edict (all mults neutral). The production
    // (10 lumber/5 iron/5 food/5 coal per min + 100 workers) is hardcoded in CBuilding::Operate's
    // UTwarehouse case, gated on IsEdictActive. Default-available (gate: nothing, always discovered).
    // Net-synced via ToggleEdictNet; revoked on rocket death via EdictHostLost (rocket host).
    { "Desperate Measures", "Frantically scrounge base resources: +10 lumber, +5 iron, +5 food, +5 coal / min.\nCost: 100 workers. Lost if rocket destroyed.",
      CStructureData::rocket, EDICT_CIVWIDE, CRsrchArray::nothing,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
};

bool EdictHostHasEdicts( CStructureData::BLDG_TYPE bldgType )
{
    for ( int i = 0; i < EDICT_COUNT; ++i )
        if ( g_aEdicts[i].hostBuilding == bldgType )
            return true;
    return false;
}
