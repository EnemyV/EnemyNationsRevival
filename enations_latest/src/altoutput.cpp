#include "stdafx.h"
#include "altoutput.h"

#include "base.h"       // CMaterialTypes
#include "building.h"   // CBuilding, CStructureData, CBuildPower
#include "player.h"     // CPlayer + Can*/Get* accessors
#include "research.h"   // CRsrchArray enum (via player.h)
#include "unit.inl"     // CUnit inline accessors (GetData/GetStore/...)
#include "building.inl" // CStructureData::GetUnionType / GetBldPower inlines
#include "chproute.hpp" // CHPRouter::MsgOutMat (SetToggle's router notification)

// ---------------------------------------------------------------------------------------
// Reusable alternate-output toggle system. See altoutput.h for the design rationale.
//
// To add a new feature, append one AltOutputDef entry to s_aDefs below: a type predicate,
// a tech-gate predicate, the input/output materials, the mode, and the per-mode parameter.
// Nothing else in the production loops or UI needs to change -- the generic toggle button
// (SDL2BuildingWindow) and the shared Convert() helper drive off the table.
// ---------------------------------------------------------------------------------------

namespace
{
    using AltOutput::AltOutputDef;

    // ---- Tunable ratios ---------------------------------------------------------------
    // BIOFUEL_FOOD_PER_OIL -- Global player FOOD consumed per 1 oil produced while a refinery
    // runs in BioFuel mode. Operator: BioFuel should be MORE EXPENSIVE to run -- raised 5 -> 8
    // (8 food -> 1 oil). Change this single value to retune the rate.
    const int BIOFUEL_FOOD_PER_OIL = 8;

    // ---- Type predicates --------------------------------------------------------------
    // The oil refinery: a materials building (UTmaterials) of the refinery type. Normally
    // converts oil -> gas; BioFuel re-hosts here (stops gas, burns global food into oil).
    bool IsRefinery( CBuilding* b )
    {
        return ( b->GetData( )->GetType( ) == CStructureData::refinery );
    }

    // A coal-burning power plant: a power building whose input fuel is coal.
    bool IsCoalPowerPlant( CBuilding* b )
    {
        if ( b->GetData( )->GetUnionType( ) != CStructureData::UTpower )
            return ( false );
        CBuildPower* pBp = b->GetData( )->GetBldPower( );
        return ( pBp && ( pBp->GetInput( ) == CMaterialTypes::coal ) );
    }

    // An oil-burning power plant: a power building whose input fuel is oil. Coal Liquefaction's
    // host since the relocation -- the coal it cracks is DELIVERED, not its own fuel.
    bool IsOilPowerPlant( CBuilding* b )
    {
        if ( b->GetData( )->GetUnionType( ) != CStructureData::UTpower )
            return ( false );
        CBuildPower* pBp = b->GetData( )->GetBldPower( );
        return ( pBp && ( pBp->GetInput( ) == CMaterialTypes::oil ) );
    }

    // A lumber mill (the sawmill): a farm building whose harvest output is lumber. Slash and
    // Burn's host (Charcoal moved off the mill to the coal power plant).
    bool IsLumberMill( CBuilding* b )
    {
        if ( b->GetData( )->GetUnionType( ) != CStructureData::UTfarm )
            return ( false );
        CBuildFarm* pBf = b->GetData( )->GetBldFarm( );
        return ( pBf && ( pBf->GetTypeFarm( ) == CMaterialTypes::lumber ) );
    }

    // An EXHAUSTED oil well: a mine (UTmine) that pumps oil whose deposit has run dry
    // (m_iMinerals <= 0, so it has SetFlag(stopped|abandoned) and would normally produce
    // nothing). Fracking revives exactly this state into a flat oil trickle. A still-
    // producing oil well is NOT matched, so normal wells are byte-identical to before.
    bool IsExhaustedOilWell( CBuilding* b )
    {
        if ( b->GetData( )->GetUnionType( ) != CStructureData::UTmine )
            return ( false );
        CMineBuilding* pMine = (CMineBuilding*)b;
        return ( pMine->IsOilWell( ) && pMine->IsExhausted( ) );
    }

    // An EXHAUSTED iron mine: Moho Mining revives it into a flat iron trickle, exactly like
    // Fracking on an exhausted oil well. GetType()==iron (not the UTmine union, which also
    // covers oil wells). Only matched once the deposit is dry, so producing mines are unchanged.
    bool IsExhaustedIronMine( CBuilding* b )
    {
        if ( b->GetData( )->GetUnionType( ) != CStructureData::UTmine )
            return ( false );
        if ( b->GetData( )->GetType( ) != CStructureData::iron )
            return ( false );
        return ( ( (CMineBuilding*)b )->IsExhausted( ) );
    }

    // The rocket / the warehouse: hosts for the emergency multi-resource "scrounge" trickle.
    // Test the exact GetType() -- UTwarehouse also covers the market, and rocket/warehouse differ.
    bool IsWarehouse( CBuilding* b ) { return ( b->GetData( )->GetType( ) == CStructureData::warehouse ); }

    // ---- Tech-gate predicates (thin adapters over the CPlayer accessors) ---------------
    bool TechBioFuel( CPlayer* p ) { return ( p->CanBioFuel( ) != FALSE ); }
    bool TechCoalLiq( CPlayer* p ) { return ( p->CanCoalLiq( ) != FALSE ); }
    bool TechCharcoal( CPlayer* p ) { return ( p->CanCharcoal( ) != FALSE ); }
    bool TechFrack( CPlayer* p ) { return ( p->CanFrack( ) != FALSE ); }
    bool TechMoho( CPlayer* p ) { return ( p->CanMoho( ) != FALSE ); }
    bool TechSlashBurn( CPlayer* p ) { return ( p->CanSlashBurn( ) != FALSE ); }
    bool TechAlways( CPlayer* ) { return ( true ); }   // Desperate Measures / Scrounging: default-enabled

    int FlatMohoIron( CPlayer* p ) { return ( p->GetMohoIronPerMin( ) ); }

    // Coal Liquefaction input ratio (coal per 1 oil) by the owner's highest tier: 3 at tier 1,
    // 2 at tier 2 (Catalytic Coal Cracking). Wired into the coal-liq def's m_pfnRatioIn.
    int CoalLiqRatio( CPlayer* p ) { return ( p->GetCoalLiqRatio( ) ); }

    // Charcoal input ratio (lumber per 1 coal) by the owner's highest tier: 4/3/3/2/2. Wired into
    // the charcoal def's m_pfnRatioIn. Replaces the old GetCharcoalPct throughput scaling, which
    // scaled a slice of a HARVEST -- there is no harvest at a power plant.
    int CharcoalRatio( CPlayer* p ) { return ( p->GetCharcoalRatio( ) ); }

    // ---- Per-tier flat-rate accessors (eFlatTrickle only) -----------------------------
    // Oil/min an exhausted, fracked well trickles by the owner's highest Fracking tier
    // (0/10/15/20/25/30). Convert() scales this per-minute rate by the opers elapsed.
    int FlatFrackOil( CPlayer* p ) { return ( p->GetFrackOilPerMin( ) ); }

    // ---- The config table -------------------------------------------------------------
    // Add Charcoal and Fracking here exactly like these two (see notes at the bottom).
    const AltOutputDef s_aDefs[] =
    {
        // 1) BioFuel -- the INTENDED design (RELEASE fix): hosted on the REFINERY, not farms.
        //    The refinery normally converts oil -> gas; with Bio Oil ON it STOPS producing gas
        //    and instead burns the player's GLOBAL food into oil. Same mode-switch shape as
        //    Coal Liquefaction (the coal plant stops power, makes oil). eGlobalConsume: pulls
        //    BIOFUEL_FOOD_PER_OIL food from the owner's global food pool per 1 oil. The
        //    gas-suppression lives in the production hook (CBuilding::BuildMaterials).
        {
            "Bio Oil",
            "Stops gas production; converts food into oil",
            &IsRefinery,
            &TechBioFuel,
            CMaterialTypes::food,        // GLOBAL player resource consumed (eGlobalConsume)
            CMaterialTypes::oil,
            AltOutput::eGlobalConsume,
            nullptr,                     // m_pfnPct
            nullptr,                     // m_pfnFlat
            BIOFUEL_FOOD_PER_OIL,        // food per 1 oil (operator-tunable, default 5)
            1.0f,
            0                            // m_iWorkforceAdd (no extra labor)
        },

        // 2) Coal Liquefaction -- hosted on the OIL power plant. Toggled ON, the plant stops
        //    generating power and cracks DELIVERED coal into oil at 3:1 (2:1 at Catalytic Coal
        //    Cracking, via m_pfnRatioIn). eTimeDriven: the conversion runs off elapsed production
        //    time and burns NO fuel -- see BuildPower's time-driven branch. It was previously
        //    hosted on the COAL plant, where the coal it consumed was also its own fuel; the
        //    move makes the produced oil exportable (an oil plant's own fuel is oil, which the
        //    router refuses to source, so the delivery rules in EffInputMat/EffOutputMat and the
        //    human router's source rule are what make this work at all).
        //    SAVE HAZARD, knowingly unguarded (no VER_RELEASE bump this phase): an OLD save with
        //    a liquefying COAL plant reloads with alt_oil set on a plant that now resolves to
        //    CHARCOAL -- it silently demands lumber it has never been sent and its oil income
        //    stops. Test on fresh saves only until the migration lands.
        {
            "Coal Liquefaction",
            "Stops power generation; cracks delivered coal into oil at 3:1 (2:1 with Catalytic Coal Cracking)",
            &IsOilPowerPlant,
            &TechCoalLiq,
            CMaterialTypes::coal,
            CMaterialTypes::oil,
            AltOutput::eRatioConsume,
            nullptr,                     // m_pfnPct
            nullptr,                     // m_pfnFlat
            3,                            // 3 coal per 1 oil at tier 1 (m_pfnRatioIn drops it to 2 at tier 2)
            1.0f,
            0,                           // m_iWorkforceAdd (no extra labor)
            0,                           // m_iPowerMultAdd (no extra power)
            {},                          // m_aMulti (unused)
            0,                           // m_nMulti
            &CoalLiqRatio,               // per-tier input ratio (3 -> 2 at Catalytic Coal Cracking)
            AltOutput::EDrive::eTimeDriven       // m_eDrive: conversion runs off TIME, no fuel burned
        },

        // 3) Charcoal -- hosted on the COAL power plant, which runs as a KILN. Toggled ON, the
        //    plant stops generating power and chars DELIVERED lumber into coal. eTimeDriven: the
        //    conversion runs off elapsed production time and burns NO fuel, so the coal it makes
        //    LEAVES the plant instead of feeding its own furnace -- that is the whole point, the
        //    chain is trees -> lumber -> charcoal -> liquefaction -> oil -> gas.
        //    It was previously hosted on the lumber MILL, where a tier-scaled slice of the
        //    harvest was diverted into the kiln (CPlayer::GetCharcoalPct). There is no harvest at
        //    a power plant, so the tier ladder now scales the RATIO instead
        //    (CPlayer::GetCharcoalRatio, 4/3/3/2/2 lumber per coal, via m_pfnRatioIn).
        //    SAVE HAZARD, knowingly unguarded (no VER_RELEASE bump this phase): an OLD save with
        //    a charcoal-burning LUMBER MILL reloads with alt_oil set on a mill that no longer
        //    matches any def, so the toggle silently does nothing there. Fresh saves only.
        {
            "Charcoal",
            "Stops power generation; chars delivered lumber into coal at 4:1, improving to 2:1 with Charcoal research, at +2 workers",
            &IsCoalPowerPlant,
            &TechCharcoal,
            CMaterialTypes::lumber,
            CMaterialTypes::coal,
            AltOutput::eRatioConsume,
            nullptr,                     // m_pfnPct
            nullptr,                     // m_pfnFlat
            4,                            // 4 lumber per 1 coal at tier 1 (m_pfnRatioIn scales it to 2 by T4)
            1.0f,
            2,                           // m_iWorkforceAdd: 20% of power_1's base GetPeople() (8) -- operator:
                                         // "the plant's workers plus a small percentage". ABSOLUTE, see the field doc.
            0,                           // m_iPowerMultAdd (no extra power)
            {},                          // m_aMulti (unused)
            0,                           // m_nMulti
            &CharcoalRatio,              // per-tier input ratio (4 -> 2 up the Charcoal ladder)
            AltOutput::EDrive::eTimeDriven       // m_eDrive: conversion runs off TIME, no fuel burned
        },

        // 4) Fracking (NEW) -- an EXHAUSTED oil well (its deposit run dry, so it is
        //    stopped/abandoned and normally idle) trickles a flat oil rate when toggled
        //    ON. eFlatTrickle: credit FlatFrackOil() units/min (per-tier 5/7/9/11/13),
        //    no input consumed -- it is neither a pct-of-production (the well produces
        //    nothing) nor an input-consume, so it uses the third mode added for this
        //    feature. The +50% well energy is applied at the production hook
        //    (CMineBuilding::FrackTick), not here.
        {
            "Fracking",
            "Revives this exhausted oil well to trickle oil, at triple power cost",
            &IsExhaustedOilWell,
            &TechFrack,
            CMaterialTypes::oil,         // notional input (not consumed in eFlatTrickle)
            CMaterialTypes::oil,
            AltOutput::eFlatTrickle,
            nullptr,                     // m_pfnPct
            &FlatFrackOil,               // m_pfnFlat (oil/min by tier)
            0,                           // m_iRatioIn
            1.0f,
            0                            // m_iWorkforceAdd (no extra labor)
        },

        // 5) Moho Mining (NEW) -- an EXHAUSTED iron mine trickles a flat iron rate when toggled,
        //    exactly like Fracking on an oil well (shares the generic UTmine FrackTick path +
        //    its +50% power surcharge). eFlatTrickle, 10 iron/min at the mine_2-granted base
        //    tier, rising +1 per moho_2..moho_6 upgrade to 15 (CPlayer::GetMohoIronPerMin).
        {
            "Moho Mining",
            "Revives this exhausted iron mine to trickle iron, at a heavy flat power cost",
            &IsExhaustedIronMine,
            &TechMoho,
            CMaterialTypes::iron,        // notional input (not consumed in eFlatTrickle)
            CMaterialTypes::iron,
            AltOutput::eFlatTrickle,
            nullptr,                     // m_pfnPct
            &FlatMohoIron,               // m_pfnFlat (iron/min)
            0,                           // m_iRatioIn
            1.0f,
            0,                           // m_iWorkforceAdd
            0,                           // m_iPowerMultAdd (no extra power)
            {},                          // m_aMulti (unused)
            0                            // m_nMulti
        },

        // 6) Scrounging -- the WAREHOUSE scrounges a small multi-resource trickle when toggled:
        //    emergency income at a cost. eMultiTrickle, no tech gate. Costs 30 WORKERS and nothing
        //    else (m_iPowerMultAdd=0).
        //    It used to double the warehouse's power draw (m_iPowerMultAdd=1). Dropped as an
        //    operator design call, NOT because power is inert -- power is a real cost, and an
        //    earlier note here claimed otherwise. Correcting the record, because the claim is easy
        //    to re-derive wrongly: CPlayer::m_fPwrMult IS consumed, by five production functions in
        //    building.inl (GetProd, GetProdNoPeople, GetProdNoDamage, GetFrameProd,
        //    GetFrameProdNoPeople), each gating on
        //        if ( GetOwner()->GetPwrMult() < 1 )
        //            fInc *= GetNoPower() + ( 1 - GetNoPower() ) * GetPwrMult();
        //    i.e. a colony in power DEFICIT has every building's output dragged toward that
        //    building type's GetNoPower() floor. The trap that produced the wrong conclusion: a
        //    grep over *.cpp/*.h misses .inl entirely, and caimgr.cpp:5399 declares an unrelated
        //    GetPwrMult() on the AI manager that a search lands on first.
        //    So the cost is real but CONDITIONAL: it is free while the colony runs a power
        //    surplus, and bites colony-wide only once need passes have -- the same shape as the
        //    workforce cost via m_fPplMult. Restoring it is a legitimate balance option (set
        //    m_iPowerMultAdd back to 1); raising m_iWorkforceAdd is the other lever, and the one
        //    that competes in the same currency as the rocket's Desperate Measures.
        //    TERRAIN-SCALED (m_bTerrainScaled=true): EVERY line below is the yield at a PERFECT
        //    site. MultiLinesFor scales each by this warehouse's own ground -- lumber =
        //    3 * forestMult / 10, food = 2 * soilMult / 10, iron and coal = 3 * scrapMult / 10.
        //    ForestMultAt and the scrounge soil scan are 0..10; the two scrap scales run 0..20
        //    because mountain rock counts double, so solid rock scrounges ~6 iron + ~5 coal where
        //    ordinary broken ground caps at 3 + 3 and forest (half weight) at 2 + 1. Iron and coal
        //    are SEPARATE scales because city ground yields only iron and roadbed only coal.
        //    The scaling ROUNDS (see MultiLinesFor), so a middling site pays one unit rather than
        //    nothing: ~17% forest cover buys the first lumber, fertility 3 (hill, marsh, water)
        //    the first food, ~15% of a mountain ring the first iron.
        //    Every terrain in the game now yields SOMETHING, so "Nothing here to scrounge" is
        //    effectively unreachable -- it is kept as an honest fallback, not a live case. What
        //    varies is how much and of what: 8/min on solid rock down to 1/min on bare roadbed.
        //    Worst case (rock/road): 4 units/min for 30 workers = 0.13 per worker. A realistic
        //    forest site (forestMult 8) runs ~7/min = 0.23. That is now ABOVE Desperate Measures
        //    per worker (25/min per DESPERATE_RATE_PER=200 workers = 0.125) -- deliberate: the two
        //    are no longer competing on efficiency. Scrounging is capped per warehouse (30 workers
        //    each, and you must have built the warehouse where the trees are); Desperate Measures
        //    is uncapped, conscripting HALF the colony's idle labour at once. Efficiency vs scale.
        //    (The rocket's richer version, Desperate Measures, is a civ-wide edict, not AltOutput.)
        {
            "Scrounging",
            "Scrounge from the land: what this warehouse yields is decided by the ground around it --\n"
            "lumber from forest, food from soil, marsh and water, iron and coal from rough country\n"
            "and richest on bare mountain rock, plus salvage off city ground and roadbeds.\n"
            "Costs 30 workers while on.",
            &IsWarehouse,
            &TechAlways,
            CMaterialTypes::lumber,
            CMaterialTypes::lumber,
            AltOutput::eMultiTrickle,
            nullptr,
            nullptr,
            0,
            1.0f,
            30,                          // m_iWorkforceAdd (30 workers)
            0,                           // m_iPowerMultAdd (no extra power -- see the note above)
            // every line is the PERFECT-site yield, terrain-scaled per warehouse (MultiLinesFor);
            // iron/coal DOUBLE these on solid mountain rock, which scores 20 on a 0..10 scale
            { { CMaterialTypes::lumber, 3 }, { CMaterialTypes::iron, 3 }, { CMaterialTypes::food, 2 }, { CMaterialTypes::coal, 3 } },
            4,
            nullptr,                     // m_pfnRatioIn (unused)
            AltOutput::EDrive::eFuelDriven, // m_eDrive: spelled out, NOT omitted. m_eDrive now sits
                                         // ahead of m_bTerrainScaled in the struct tail, so a bare
                                         // trailing `true` would bind to it -- see the EDrive note
                                         // in altoutput.h. Scrounging is a trickle and burns fuel.
            true                         // m_bTerrainScaled: lumber + food scale with the site
        },

        // 7) Slash and Burn -- the LUMBER MILL cuts at 250% while the toggle is ON, and
        //    permanently destroys the forest around it as it does. eModifier: this def produces
        //    NO secondary material at all -- it exists only to carry the per-building toggle,
        //    and Convert( ) early-returns for it. The 250% itself lives in
        //    CFarmBuilding::BuildFarm (AltOutput::SLASH_BURN_MULT), gated by
        //    CFarmBuilding::SlashBurnActive( ); the two UI rate readouts apply the same
        //    multiplier through the same predicate so the displayed rate matches the sim.
        //    NOT YET IMPLEMENTED: the deforestation half. Until it lands, the toggle is a pure
        //    250% harvest bonus and the tooltip below promises a cost the sim does not charge.
        {
            "Slash and Burn",
            "Cuts at 250% of the normal rate -- but PERMANENTLY destroys the forest around this mill, until there is nothing left to cut. Cannot be undone.",
            &IsLumberMill,
            &TechSlashBurn,
            CMaterialTypes::lumber,      // unused: eModifier consumes nothing
            CMaterialTypes::lumber,      // unused: eModifier produces nothing
            AltOutput::eModifier,
            nullptr,                     // m_pfnPct
            nullptr,                     // m_pfnFlat
            0,                           // m_iRatioIn (nothing is consumed)
            1.0f,
            0,                           // m_iWorkforceAdd: NONE. Operator 2026-09-06, "no upkeep
                                         // change from regular operation for slash and burn" --
                                         // the deforestation IS the cost. Do NOT add a labour or
                                         // power penalty later without asking.
            0,                           // m_iPowerMultAdd (no extra power, same decision)
            {},                          // m_aMulti (unused)
            0,                           // m_nMulti
            nullptr,                     // m_pfnRatioIn (no per-tier ratio)
            AltOutput::EDrive::eFuelDriven       // m_eDrive: meaningless for eModifier (nothing converts);
                                         // spelled out rather than omitted so the tail is explicit
        },
    };

    const int s_nDefs = (int)( sizeof( s_aDefs ) / sizeof( s_aDefs[0] ) );
}

namespace AltOutput
{
    const AltOutputDef* DefFor( CBuilding* pBldg )
    {
        if ( !pBldg )
            return ( nullptr );
        for ( int i = 0; i < s_nDefs; i++ )
            if ( s_aDefs[i].m_pfnMatches( pBldg ) )
                return ( &s_aDefs[i] );
        return ( nullptr );
    }

    const AltOutputDef* Available( CBuilding* pBldg )
    {
        const AltOutputDef* pDef = DefFor( pBldg );
        if ( !pDef )
            return ( nullptr );
        CPlayer* pOwner = pBldg->GetOwner( );
        if ( !pOwner )
            return ( nullptr );
        if ( !pDef->m_pfnHasTech( pOwner ) )
            return ( nullptr );
        return ( pDef );
    }

    bool StopsPower( CBuilding* pBldg, const AltOutputDef* pDef )
    {
        // DERIVED, not stored (see altoutput.h): a store-consuming conversion def hosted on a
        // power plant. Reproduces the old inline bCoalLiq exactly -- today's only UTpower def is
        // Coal Liquefaction (eRatioConsume), which already suppressed power before this existed.
        if ( !pBldg || !pDef )
            return ( false );
        if ( pBldg->GetData( )->GetUnionType( ) != CStructureData::UTpower )
            return ( false );
        return ( pDef->m_eMode == eRatioConsume );
    }

    int InputRatio( CBuilding* pBldg, const AltOutputDef* pDef )
    {
        // Same expression Convert()'s eRatioConsume branch computes inline; Convert keeps its own
        // copy deliberately (it must stay byte-identical for the shipped callers).
        if ( !pBldg || !pDef )
            return ( 0 );
        CPlayer* pOwner = pBldg->GetOwner( );
        if ( !pOwner )
            return ( 0 );
        return ( pDef->m_pfnRatioIn ? pDef->m_pfnRatioIn( pOwner ) : pDef->m_iRatioIn );
    }

    void SetToggle( CBuilding* pBldg, bool bOn )
    {
        if ( !pBldg )
            return;
        if ( ( pBldg->IsFlag( CUnit::alt_oil ) != FALSE ) == bOn )
            return;                     // already in the requested state -- nothing changed

        if ( bOn )
            pBldg->SetFlag( CUnit::alt_oil );
        else
            pBldg->ClrFlag( CUnit::alt_oil );

        // A mode change alters what this building wants DELIVERED, and nothing else tells the
        // router: CHPRouter::NeedsCommodities( NULL ) only revisits buildings already in its need
        // list, so a stocked plant that needed nothing can never be discovered. Fire the existing
        // per-building entry point on both ON and OFF. Scoped to the INPUT-CONSUMING modes so the
        // four shipped trickle/modifier features are untouched. (The AI writes the flag directly
        // for its trickle defs and so bypasses this -- harmless, they consume no input.)
        const AltOutputDef* pDef = Available( pBldg );
        if ( !pDef )
            return;
        if ( ( pDef->m_eMode != eRatioConsume ) && ( pDef->m_eMode != eGlobalConsume ) )
            return;
        CPlayer* pOwner = pBldg->GetOwner( );
        if ( !pOwner || !pOwner->IsMe( ) )
            return;
        if ( theGame.m_pHpRtr )
            theGame.m_pHpRtr->MsgOutMat( pBldg );
    }

    void Convert( CBuilding* pBldg, int iAmount, float& fAccum, float fThrottle )
    {
        if ( iAmount <= 0 )
            return;

        // Toggle must be ON; def must apply and be researched for this owner.
        if ( !pBldg->IsFlag( CUnit::alt_oil ) )
            return;
        const AltOutputDef* pDef = Available( pBldg );
        if ( !pDef )
            return;

        // eModifier defs produce NO secondary material at all -- they exist only to carry the
        // per-building toggle plus modifier fields, and their effect lives in the feature's own
        // production hook. This return MUST be explicit: the if/else chain below ends in an
        // unguarded `else` that IS eGlobalConsume, so an eModifier def would otherwise fall into
        // the global-food accounting. (No def uses eModifier yet, so this is inert today.)
        if ( pDef->m_eMode == eModifier )
            return;

        CPlayer* pOwner = pBldg->GetOwner( );
        int iOut = 0;

        if ( pDef->m_eMode == ePctAdditive )
        {
            // Output = pct% of the primary production amount; input not consumed. Integer
            // math identical to the original BioFuel hook (no remainder carry) for exact
            // regression parity. (fAccum intentionally unused in this mode.)
            int iPct = pDef->m_pfnPct ? pDef->m_pfnPct( pOwner ) : 0;
            if ( iPct <= 0 )
                return;
            // Integer math when there's no energy multiplier -- bit-for-bit identical to the
            // original BioFuel hook ( (amount * pct) / 100 ). Only fall back to float when a
            // def actually sets a non-unit multiplier.
            if ( pDef->m_fEnergyMult == 1.0f )
                iOut = ( iAmount * iPct ) / 100;
            else
                iOut = (int)( ( (float)iAmount * (float)iPct * pDef->m_fEnergyMult ) / 100.0f );
        }
        else if ( pDef->m_eMode == eFlatTrickle )
        {
            // Flat trickle: credit m_pfnFlat(owner) units PER IN-GAME MINUTE, scaled by the
            // opers elapsed this call (passed as iAmount). No input is consumed. The float
            // accumulator carries the sub-unit remainder so a slow trickle isn't lost to
            // truncation. Game time runs at OPERS_PER_MINUTE opers/min (m_dwElapsedTime is
            // in 24ths-of-a-second << 4 => 24*16=384 opers/sec => 23040 opers/min; see
            // CPlayer::GetElapsedSeconds).
            const float OPERS_PER_MINUTE = 384.0f * 60.0f;   // = 23040
            int iRate = pDef->m_pfnFlat ? pDef->m_pfnFlat( pOwner ) : 0;
            if ( iRate <= 0 )
                return;

            // fThrottle is the caller's production multiplier (damage / workforce / power).
            // Applied here, in float, so a partial tick keeps its remainder in fAccum.
            if ( fThrottle < 0.0f ) fThrottle = 0.0f;
            fAccum += ( (float)iRate * (float)iAmount * fThrottle ) / OPERS_PER_MINUTE;
            int iWantOut = (int)fAccum;
            if ( iWantOut <= 0 )
                return;

            fAccum -= (float)iWantOut;   // keep the un-emitted fraction for next call
            iOut = iWantOut;
        }
        else if ( pDef->m_eMode == eRatioConsume )
        {
            // Consume iRatio input units from the building's store per 1 output unit, scaled by
            // the per-call amount. iRatio is the def's fixed m_iRatioIn unless the def supplies a
            // per-tier override (m_pfnRatioIn) -- Coal Liquefaction drops 3:1 -> 2:1 at tier 2.
            // A runtime fractional accumulator carries the sub-unit remainder between calls so
            // small yields aren't repeatedly lost.
            int iRatio = pDef->m_pfnRatioIn ? pDef->m_pfnRatioIn( pOwner ) : pDef->m_iRatioIn;
            if ( iRatio <= 0 )
                return;

            float fWant = ( (float)iAmount * pDef->m_fEnergyMult ) / (float)iRatio;
            fAccum += fWant;
            int iWantOut = (int)fAccum;
            if ( iWantOut <= 0 )
                return;

            // Clamp to the coal actually on hand (whole-unit conversions only).
            int iHave    = pBldg->GetStore( pDef->m_iInputMat );
            int iMaxOut  = iHave / iRatio;
            if ( iWantOut > iMaxOut )
                iWantOut = iMaxOut;
            if ( iWantOut <= 0 )
                return;

            int iConsume = iWantOut * iRatio;
            pBldg->AddToStore( pDef->m_iInputMat, -iConsume );
            pOwner->IncMaterialHave( pDef->m_iInputMat, -iConsume );

            fAccum -= (float)iWantOut;   // keep the un-emitted fraction for next call
            iOut = iWantOut;
        }
        else // eGlobalConsume (BioFuel: refinery burns GLOBAL player food -> oil)
        {
            // Like eRatioConsume but the input (food) is the player's GLOBAL pool, not the
            // building's store. Consume m_iRatioIn food per 1 oil, scaled by the per-call
            // amount; the accumulator carries the sub-unit remainder. The caller has already
            // suppressed the refinery's normal gas output for this batch.
            if ( pDef->m_iRatioIn <= 0 )
                return;

            float fWant = ( (float)iAmount * pDef->m_fEnergyMult ) / (float)pDef->m_iRatioIn;
            fAccum += fWant;
            int iWantOut = (int)fAccum;
            if ( iWantOut <= 0 )
                return;

            // Clamp to the GLOBAL food actually on hand (whole-unit conversions only).
            int iHave   = pOwner->GetFood( );
            int iMaxOut = iHave / pDef->m_iRatioIn;
            if ( iWantOut > iMaxOut )
                iWantOut = iMaxOut;
            if ( iWantOut <= 0 )
                return;

            int iConsume = iWantOut * pDef->m_iRatioIn;
            pOwner->AddFood( -iConsume );   // decrement the global food pool (see new_unit.cpp)

            fAccum -= (float)iWantOut;   // keep the un-emitted fraction for next call
            iOut = iWantOut;
        }

        if ( iOut <= 0 )
            return;

        pBldg->AddToStore( pDef->m_iOutputMat, iOut );
        pOwner->IncMaterialMade( pDef->m_iOutputMat, iOut );
        pOwner->IncMaterialHave( pDef->m_iOutputMat, iOut );
    }

    void CreditTrickle( CBuilding* pBldg, int iAmount, float* afAccum, const AltMat* pMats, int nMats )
    {
        if ( ( iAmount <= 0 ) || !afAccum || !pMats )
            return;

        CPlayer*    pOwner = pBldg->GetOwner( );
        const float OPERS_PER_MINUTE = 384.0f * 60.0f;   // = 23040 (matches Convert)

        // Each output line trickles independently, carried by its own accumulator.
        for ( int i = 0; ( i < nMats ) && ( i < kMaxMulti ); i++ )
        {
            int iRate = pMats[i].m_iPerMin;
            if ( iRate <= 0 )
                continue;
            afAccum[i] += ( (float)iRate * (float)iAmount ) / OPERS_PER_MINUTE;
            int iOut = (int)afAccum[i];
            if ( iOut <= 0 )
                continue;
            afAccum[i] -= (float)iOut;

            int iMat = pMats[i].m_iMat;
            if ( iMat == CMaterialTypes::food )
                pOwner->AddFood( iOut );                 // food is a GLOBAL pool, not a store
            else if ( iMat == CMaterialTypes::gas )
                pOwner->AddGas( iOut );                  // gas is global too
            else
            {
                pBldg->AddToStore( iMat, iOut );         // lumber / iron / coal -> building store
                pOwner->IncMaterialMade( iMat, iOut );
                pOwner->IncMaterialHave( iMat, iOut );
            }
        }
    }

    int MultiLinesFor( CBuilding* pBldg, const AltOutputDef* pDef, AltMat* pOut )
    {
        if ( !pDef || !pOut )
            return ( 0 );

        int nLines = ( pDef->m_nMulti < kMaxMulti ) ? pDef->m_nMulti : kMaxMulti;
        for ( int i = 0; i < nLines; i++ )
            pOut[i] = pDef->m_aMulti[i];

        // Not terrain-scaled (or not a warehouse host): the table lines ARE the rates.
        if ( !pDef->m_bTerrainScaled || !pBldg
             || ( pBldg->GetData( )->GetUnionType( ) != CStructureData::UTwarehouse ) )
            return ( nLines );

        // Scrounging: scale the wood and food lines by what is actually around this warehouse.
        // Both mults are 0..10 and cached on the building (the scan is a hex enumeration).
        CWarehouseBuilding* pWh     = (CWarehouseBuilding*)pBldg;
        int                 iForest = pWh->GetScroungeForestMult( );
        int                 iSoil   = pWh->GetScroungeSoilMult( );
        int                 iIron   = pWh->GetScroungeIronMult( );
        int                 iCoal   = pWh->GetScroungeCoalMult( );
        // ONE rounding, here, and nowhere else. The mults arrive in HUNDREDTHS of the 0..10
        // scale (UpdateScrounge asks the scans for 100x), so the divisor is 1000 and the
        // round-to-nearest term is half of it. Every term is non-negative, so the usual
        // signed-rounding trap does not apply.
        //
        // Two roundings is what this shipped with and it was wrong twice over. Truncating the
        // AVERAGE inside each scan threw away up to a full multiplier point before the rate was
        // computed -- a systematic downward bias, worst exactly where the weights are small. That
        // is what made an ordinary town warehouse (a MIX of city and road, each averaging ~1.8,
        // each truncating to 1) score zero on every line and report "Nothing here to scrounge".
        // Rounding once, at the end, keeps the fractions that decide these small numbers.
        const int kScale = 1000;
        for ( int i = 0; i < nLines; i++ )
        {
            if ( pOut[i].m_iMat == CMaterialTypes::lumber )
                pOut[i].m_iPerMin = ( pOut[i].m_iPerMin * iForest + kScale / 2 ) / kScale;
            else if ( pOut[i].m_iMat == CMaterialTypes::food )
                pOut[i].m_iPerMin = ( pOut[i].m_iPerMin * iSoil + kScale / 2 ) / kScale;
            else if ( pOut[i].m_iMat == CMaterialTypes::iron )
                pOut[i].m_iPerMin = ( pOut[i].m_iPerMin * iIron + kScale / 2 ) / kScale;
            else if ( pOut[i].m_iMat == CMaterialTypes::coal )
                pOut[i].m_iPerMin = ( pOut[i].m_iPerMin * iCoal + kScale / 2 ) / kScale;
        }
        return ( nLines );
    }

    void ConvertMulti( CBuilding* pBldg, int iAmount, float* afAccum )
    {
        if ( !pBldg->IsFlag( CUnit::alt_oil ) )
            return;
        const AltOutputDef* pDef = Available( pBldg );
        if ( !pDef || ( pDef->m_eMode != eMultiTrickle ) )
            return;
        // Credit what this SITE yields, not what the table says -- see MultiLinesFor.
        AltMat aLines[kMaxMulti];
        int    nLines = MultiLinesFor( pBldg, pDef, aLines );
        CreditTrickle( pBldg, iAmount, afAccum, aLines, nLines );
    }
}
