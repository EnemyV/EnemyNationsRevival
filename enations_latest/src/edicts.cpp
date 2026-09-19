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

    // EDICT_DESPERATE_MEASURES — Rocket, civ-wide BEHAVIOR edict (all mults neutral). The DRAFT is
    // priced once per pump by CPlayer::ApplySurplusEdicts (flat DESPERATE_BASE_DRAFT plus
    // SURPLUS_DRAFT_PCT% of the spare workforce); CBuilding::Operate's UTwarehouse case, gated on
    // IsEdictActive, credits DESPERATE_BASE_RATES scaled by draft/DESPERATE_RATE_PER.
    // Default-available (gate: nothing, always discovered).
    // Net-synced via ToggleEdictNet; revoked on rocket death via EdictHostLost (rocket host).
    { "Desperate Measures", "Frantically scrounge base resources: +10 lumber, +5 iron, +5 food, +5 coal / min per 200 workers drafted.\nConscripts 100 workers plus half of your idle workforce, and scrounges proportionally harder. Lost if rocket destroyed.",
      CStructureData::rocket, EDICT_CIVWIDE, CRsrchArray::nothing,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // --- surplus-scaled family --------------------------------------------------------------
    // Every one of these has ALL its catalog mults neutral and ALL its upkeep pcts zero: their
    // bonus AND their cost are computed live each pump from the colony's spare workforce/power
    // by CPlayer::ApplySurplusEdicts, which writes the dynamic m_fSurplus*Mult fields that the
    // matching Get* accessors fold in beside these static ones. Nothing here is a placeholder to
    // be filled in later -- a value in these columns would be a SECOND, static effect on top.
    // Each is a FLAT cost in one resource plus a cut of the surplus of one resource (operator:
    // never just surplus), so switching one on always commits the colony to something.

    // EDICT_PUBLIC_WORKS — Rocket, civ-wide (lost if the rocket is destroyed, §29). People both
    // sides: a flat PUBLIC_WORKS_BASE_DRAFT plus SURPLUS_DRAFT_PCT% of the spare workforce, all
    // of it turned into construction speed (m_fSurplusConstMult -> GetConstProd), ramping to
    // PUBLIC_WORKS_MAX_PCT at PUBLIC_WORKS_FULL_DRAFT total. Gate: const_2.
    { "Public Works", "Civ-wide: puts workers on construction crews: +1% build speed per 10 drafted, up to +30%.\nCost: 100 workers plus half of your idle workforce. Lost if rocket destroyed.",
      CStructureData::rocket, EDICT_CIVWIDE, CRsrchArray::const_2,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_CIVIL_DEFENCE — Rocket, civ-wide (lost if the rocket is destroyed, §29). Population
    // + surplus ENERGY: a flat CIVDEF_BASE_DRAFT of workers mans the shelters, and the EFFECT is
    // bought with SURPLUS_POWER_PCT% of the spare power (CIVDEF_FULL_POWER = full effect).
    // Drives m_fSurplusBldgDmgMult (folded into GetEdictBldgDmgMult beside Meat Shield) and
    // m_fSurplusFortMult (GetEdictFortBuildMult, beside Fortify Border). Gate: fortification.
    { "Civil Defence", "Civ-wide: workers and surplus power go to shelters and fortifications: buildings take up to 20% less damage, forts build up to 30% faster.\nCost: 100 workers, plus half of your surplus power — which is what the effect scales with. Lost if rocket destroyed.",
      CStructureData::rocket, EDICT_CIVWIDE, CRsrchArray::fortification,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_WAR_FOOTING — Command Center, civ-wide. The mirror of Civil Defence: energy +
    // surplus PEOPLE. A flat WARFOOT_BASE_POWER bill, and the effect is bought with
    // SURPLUS_DRAFT_PCT% of the spare workforce (WARFOOT_FULL_DRAFT = full effect), spent on
    // m_fSurplusInfBuildMult (folded into GetEdictInfBuildMult beside The Draft). Gate: atk_2.
    { "War Footing", "Civ-wide: power and idle workers go to the war effort: infantry build up to 100% faster.\nCost: 30 power, plus half of your idle workforce — which is what the effect scales with.",
      CStructureData::command_center, EDICT_CIVWIDE, CRsrchArray::atk_2,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },

    // EDICT_RESEARCH_FELLOWSHIPS — Office, civ-wide. Energy + surplus PEOPLE: a flat
    // FELLOWS_BASE_POWER for the programme plus 1 more per FELLOWS_PER_POWER fellows, booked
    // into m_iPwrNeed like a building's own draw so it browns the colony out if it can't afford
    // it. Each fellow credits AddRsrch at FELLOW_RATE_PCT% of one laboratory worker's rate,
    // through the same PplMult/RsrchMult throttles the lab itself uses. Gate: medium_facilities.
    { "Research Fellowships", "Civ-wide: places idle workers on research fellowships: each fellow researches at 25% of a laboratory worker's rate.\nCost: 25 power plus 1 more per 5 fellows, and half of your idle workforce.",
      CStructureData::office, EDICT_CIVWIDE, CRsrchArray::medium_facilities,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
};

// Desperate Measures' scrounge, per DESPERATE_RATE_PER workers drafted (see edicts.h).
const AltOutput::AltMat DESPERATE_BASE_RATES[DESPERATE_RATE_LINES] =
    { { CMaterialTypes::lumber, 10 }, { CMaterialTypes::iron, 5 },
      { CMaterialTypes::food, 5 },    { CMaterialTypes::coal, 5 } };

bool EdictHostHasEdicts( CStructureData::BLDG_TYPE bldgType )
{
    for ( int i = 0; i < EDICT_COUNT; ++i )
        if ( g_aEdicts[i].hostBuilding == bldgType )
            return true;
    return false;
}
