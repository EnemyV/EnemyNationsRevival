#include "stdafx.h"
#include "altoutput.h"

#include "base.h"       // CMaterialTypes
#include "building.h"   // CBuilding, CStructureData, CBuildPower
#include "player.h"     // CPlayer + Can*/Get* accessors
#include "research.h"   // CRsrchArray enum (via player.h)
#include "unit.inl"     // CUnit inline accessors (GetData/GetStore/...)
#include "building.inl" // CStructureData::GetUnionType / GetBldPower inlines

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

    // A lumber mill (the sawmill): a farm building whose harvest output is lumber.
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

    // ---- Tech-gate predicates (thin adapters over the CPlayer accessors) ---------------
    bool TechBioFuel( CPlayer* p ) { return ( p->CanBioFuel( ) != FALSE ); }
    bool TechCoalLiq( CPlayer* p ) { return ( p->CanCoalLiq( ) != FALSE ); }
    bool TechCharcoal( CPlayer* p ) { return ( p->CanCharcoal( ) != FALSE ); }
    bool TechFrack( CPlayer* p ) { return ( p->CanFrack( ) != FALSE ); }

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

        // 2) Coal Liquefaction (NEW) -- a coal power plant converts 2 coal -> 1 oil when
        //    toggled. eRatioConsume: pulls coal from the plant's own store and credits oil.
        {
            "Coal Liquefaction",
            "Stops power generation; converts 3 coal -> 1 oil",
            &IsCoalPowerPlant,
            &TechCoalLiq,
            CMaterialTypes::coal,
            CMaterialTypes::oil,
            AltOutput::eRatioConsume,
            nullptr,                     // m_pfnPct
            nullptr,                     // m_pfnFlat
            3,                            // 3 coal per 1 oil (nerfed from 2 per operator)
            1.0f,
            0                            // m_iWorkforceAdd (no extra labor)
        },

        // 3) Charcoal (NEW) -- a lumber mill (the sawmill: UTfarm with lumber output) runs a
        //    kiln that converts harvested lumber into coal ("Charcoal" label only). The
        //    Convert() slice-ratio below is a fixed 2 lumber -> 1 coal, BUT the production hook
        //    only feeds a tier-scaled 6/8/10/12% of the harvest into the kiln and discards the
        //    rest, so the player-VISIBLE effective rate is ~1 coal per 17-33 lumber (stingy by
        //    design). Do NOT describe this as "2 lumber -> 1 coal" in the player desc -- that is
        //    the internal slice ratio, not what the player experiences. eRatioConsume: pulls
        //    lumber from the mill's own store and
        //    credits coal. MODE-SWITCH: the production hook (CFarmBuilding::BuildFarm lumber
        //    branch) diverts the harvest into the kiln instead of crediting player lumber, and
        //    feeds Convert() a TIER-SCALED amount (CPlayer::GetCharcoalPct; T1 very low) so the
        //    2:1 ratio stays fixed while throughput scales with research. No energy cost.
        {
            "Charcoal",
            "Stops lumber output; burns the mill's whole harvest into a small coal trickle (yield scales with Charcoal research)",
            &IsLumberMill,
            &TechCharcoal,
            CMaterialTypes::lumber,
            CMaterialTypes::coal,
            AltOutput::eRatioConsume,
            nullptr,                     // m_pfnPct
            nullptr,                     // m_pfnFlat
            2,                            // 2 lumber per 1 coal
            1.0f,
            15                           // m_iWorkforceAdd (#2: kiln draws +15 workers ABSOLUTE; operator-tunable, approved linux1)
        },

        // 4) Fracking (NEW) -- an EXHAUSTED oil well (its deposit run dry, so it is
        //    stopped/abandoned and normally idle) trickles a flat oil rate when toggled
        //    ON. eFlatTrickle: credit FlatFrackOil() units/min (per-tier 10/15/20/25/30),
        //    no input consumed -- it is neither a pct-of-production (the well produces
        //    nothing) nor an input-consume, so it uses the third mode added for this
        //    feature. The +50% well energy is applied at the production hook
        //    (CMineBuilding::FrackTick), not here.
        {
            "Fracking",
            "Revives this exhausted oil well to trickle oil, at +50% power cost",
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

    void Convert( CBuilding* pBldg, int iAmount, float& fAccum )
    {
        if ( iAmount <= 0 )
            return;

        // Toggle must be ON; def must apply and be researched for this owner.
        if ( !pBldg->IsFlag( CUnit::alt_oil ) )
            return;
        const AltOutputDef* pDef = Available( pBldg );
        if ( !pDef )
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

            fAccum += ( (float)iRate * (float)iAmount ) / OPERS_PER_MINUTE;
            int iWantOut = (int)fAccum;
            if ( iWantOut <= 0 )
                return;

            fAccum -= (float)iWantOut;   // keep the un-emitted fraction for next call
            iOut = iWantOut;
        }
        else if ( pDef->m_eMode == eRatioConsume )
        {
            // Consume m_iRatioIn input units from the building's store per 1 output unit,
            // scaled by the per-call amount. A runtime fractional accumulator carries the
            // sub-unit remainder between calls so small yields aren't repeatedly lost.
            if ( pDef->m_iRatioIn <= 0 )
                return;

            float fWant = ( (float)iAmount * pDef->m_fEnergyMult ) / (float)pDef->m_iRatioIn;
            fAccum += fWant;
            int iWantOut = (int)fAccum;
            if ( iWantOut <= 0 )
                return;

            // Clamp to the coal actually on hand (whole-unit conversions only).
            int iHave    = pBldg->GetStore( pDef->m_iInputMat );
            int iMaxOut  = iHave / pDef->m_iRatioIn;
            if ( iWantOut > iMaxOut )
                iWantOut = iMaxOut;
            if ( iWantOut <= 0 )
                return;

            int iConsume = iWantOut * pDef->m_iRatioIn;
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
}
