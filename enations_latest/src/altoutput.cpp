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

    // ---- Type predicates --------------------------------------------------------------
    bool IsFarm( CBuilding* b )
    {
        return ( b->GetData( )->GetUnionType( ) == CStructureData::UTfarm );
    }

    // A coal-burning power plant: a power building whose input fuel is coal.
    bool IsCoalPowerPlant( CBuilding* b )
    {
        if ( b->GetData( )->GetUnionType( ) != CStructureData::UTpower )
            return ( false );
        CBuildPower* pBp = b->GetData( )->GetBldPower( );
        return ( pBp && ( pBp->GetInput( ) == CMaterialTypes::coal ) );
    }

    // ---- Tech-gate predicates (thin adapters over the CPlayer accessors) ---------------
    bool TechBioFuel( CPlayer* p ) { return ( p->CanBioFuel( ) != FALSE ); }
    bool TechCoalLiq( CPlayer* p ) { return ( p->CanCoalLiq( ) != FALSE ); }

    // ---- Per-tier percent accessors (ePctAdditive only) -------------------------------
    int PctBioOil( CPlayer* p ) { return ( p->GetBioOilPct( ) ); }

    // ---- The config table -------------------------------------------------------------
    // Add Charcoal and Fracking here exactly like these two (see notes at the bottom).
    const AltOutputDef s_aDefs[] =
    {
        // 1) BioFuel -- food farm also makes oil ("Bio Oil"), output = pct% of food
        //    harvested, input NOT consumed. Migrated verbatim from the #33 one-off:
        //    PctBioOil drives the exact same 10/12/.../20% tiers, and ePctAdditive uses
        //    the same integer (amount*pct)/100 math (no remainder carry), so behaviour is
        //    identical to the original farm hook.
        {
            "Bio Oil",
            &IsFarm,
            &TechBioFuel,
            CMaterialTypes::food,        // notional input (not consumed in ePctAdditive)
            CMaterialTypes::oil,
            AltOutput::ePctAdditive,
            &PctBioOil,
            0,
            1.0f
        },

        // 2) Coal Liquefaction (NEW) -- a coal power plant converts 2 coal -> 1 oil when
        //    toggled. eRatioConsume: pulls coal from the plant's own store and credits oil.
        {
            "Coal Liquefaction",
            &IsCoalPowerPlant,
            &TechCoalLiq,
            CMaterialTypes::coal,
            CMaterialTypes::oil,
            AltOutput::eRatioConsume,
            nullptr,
            2,                            // 2 coal per 1 oil
            1.0f
        },

        // --- FUTURE config entries (slot in the same way; do not need new mechanism) ----
        // 3) Charcoal -- a lumber mill (UTfarm with lumber output) also makes coal from
        //    wood. Add a type predicate (UTfarm && GetBldFarm()->GetTypeFarm()==lumber), a
        //    CanCharcoal() gate + charcoal_* research tiers, input=lumber, output=coal,
        //    eRatioConsume with the chosen ratio (or ePctAdditive + a PctCharcoal accessor).
        // 4) Fracking -- an EXHAUSTED oil mine trickles oil. Add an IsExhaustedOilWell
        //    predicate, the existing CanFrack() gate, and either ePctAdditive over the
        //    well's production or eRatioConsume; the flat-per-min #23 numbers map to a
        //    pct/ratio. Both reuse this table + Convert() + the generic toggle button.
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
        else // eRatioConsume
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

        if ( iOut <= 0 )
            return;

        pBldg->AddToStore( pDef->m_iOutputMat, iOut );
        pOwner->IncMaterialMade( pDef->m_iOutputMat, iOut );
        pOwner->IncMaterialHave( pDef->m_iOutputMat, iOut );
    }
}
