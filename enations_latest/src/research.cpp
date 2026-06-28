//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// research.cpp : implementation file
//

#include "research.h"

#include "area.h"
#include "SDL2GameDialogs.h"
#include "SDL2MFCPanel.h"
#include "bitmaps.h"
#include "building.inl"
#include "icons.h"
#include "lastplnt.h"
#include "stdafx.h"
#include "unit.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CRsrchArray theRsrch;

void ResearchDiscovered( int iItem )
{

    if ( iItem > 0 )
        theGame.GetMe( )->m_iNumDiscovered++;

    // Live-refresh the research window if it's open (non-modal). Mirrors the
    // original ResearchDiscovered -> CDlgResearch::UpdateChoices( TRUE ): the
    // just-discovered topic drops off the list, newly unlocked topics appear, and
    // the stale "current research" marker clears. Without this the open window's
    // list stays frozen until reopened, and arms the "Discovery" button to re-show
    // this item's result text.
    theApp.m_wndBar.RefreshResearch( iItem );

    // check and update the unit build dialogs
    POSITION pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        ASSERT_STRICT_VALID( pBldg );
        if ( pBldg->GetOwner( )->IsMe( ) )
            pBldg->UpdateChoices( );
    }

    // check and update the vehicle build dialogs
    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        ASSERT_STRICT_VALID( pVeh );
        if ( pVeh->GetOwner( )->IsMe( ) )
            pVeh->UpdateChoices( );
    }

    // copper discovered
    if ( iItem == CRsrchArray::copper )
        theAreaList.XilDiscovered( );
}


/////////////////////////////////////////////////////////////////////////////
// CRsrchStatus

CRsrchStatus::CRsrchStatus( )
{

    m_bDiscovered    = FALSE;
    m_iPtsDiscovered = 0;
}

void CRsrchStatus::Serialize( CArchive& ar )
{

    if ( ar.IsStoring( ) )
    {
        ASSERT_VALID( this );
        //TRAP( );

        ar << m_bDiscovered << m_iPtsDiscovered;
    }
    else
    {
        //TRAP( );
        ar >> m_bDiscovered >> m_iPtsDiscovered;
    }
}

void ConstructElements( CRsrchStatus* pNewElem, int iCount )
{

    for ( int i = 0; i < iCount; i++, pNewElem++ )
#ifdef _WIN32
        pNewElem->CRsrchStatus::CRsrchStatus( );
#else
        new ( pNewElem ) CRsrchStatus( );
#endif
}

void DestructElements( CRsrchStatus* pNewElem, int iCount )
{

    for ( int i = 0; i < iCount; i++, pNewElem++ ) pNewElem->CRsrchStatus::~CRsrchStatus( );
}

void SerializeElements( CArchive& ar, CRsrchStatus* pData, int iCount )
{
    for ( int i = 0; i < iCount; i++ ) pData[i].Serialize( ar );
}


/////////////////////////////////////////////////////////////////////////////
// CRsrchItem

CRsrchItem::CRsrchItem( )
{

    m_iPtsRequired      = 0;
    m_iNumRsrchRequired = 0;
    m_piRsrchRequired   = NULL;
    m_iNumBldgsRequired = 0;
    m_piBldgsRequired   = NULL;
    m_iScenarioReq      = 0;
}

CRsrchItem::~CRsrchItem( )
{

    delete[] m_piRsrchRequired;
    delete[] m_piBldgsRequired;
}

void ConstructElements( CRsrchItem* pNewElem, int iCount )
{

    for ( int i = 0; i < iCount; i++, pNewElem++ )
#ifdef _WIN32
        pNewElem->CRsrchItem::CRsrchItem( );
#else
        new ( pNewElem ) CRsrchItem( );
#endif
}

void DestructElements( CRsrchItem* pNewElem, int iCount )
{

    for ( int i = 0; i < iCount; i++, pNewElem++ ) pNewElem->CRsrchItem::~CRsrchItem( );
}

#ifdef _DEBUG
void CRsrchItem::AssertValid( ) const
{

    // assert the base class
    CObject::AssertValid( );
    TRAP( );

    int iOn = 0;
    for ( iOn = 0; iOn < m_iNumRsrchRequired; iOn++ )
        ASSERT( ( 0 < m_piRsrchRequired[iOn] ) && ( m_piRsrchRequired[iOn] < theRsrch.GetSize( ) ) );
    for ( iOn = 0; iOn < m_iNumBldgsRequired; iOn++ )
        ASSERT( ( 0 < m_piBldgsRequired[iOn] ) && ( m_piBldgsRequired[iOn] < theStructures.GetNumBuildings( ) ) );
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CRsrchArray - the R&D data

void CRsrchArray::Open( )
{

    ASSERT_VALID( this );
    ASSERT( GetSize( ) == 0 );

    // read in the RIF data
    CMmio* pMmio = theDataFile.OpenAsMMIO( "research", "RSRH" );

    pMmio->DescendRiff( 'R', 'S', 'R', 'H' );
    pMmio->DescendList( 'I', 'T', 'M', 'S' );

    pMmio->DescendChunk( 'N', 'U', 'M', 'I' );
    int iSize = pMmio->ReadShort( );
    ASSERT( iSize + 1 == bridge_2 );  // DAT topics end at acc_3; bridge_2..5 are in-code
    pMmio->AscendChunk( );
    SetSize( num_types );

    // read in the per/item stuff
    // note - we make R&D level 0 discovered
    for ( int iOn = 0; iOn < iSize; iOn++ )
    {
        CRsrchItem* pRi = &ElementAt( iOn + 1 );
        pMmio->DescendChunk( 'D', 'A', 'T', 'A' );
        pRi->m_iPtsRequired = pMmio->ReadLong( );
        pRi->m_iScenarioReq = pMmio->ReadLong( );
        if ( ( pRi->m_iNumRsrchRequired = pMmio->ReadLong( ) ) > 0 )
        {
            int  iNum = pRi->m_iNumRsrchRequired;
            int* piOn = pRi->m_piRsrchRequired = new int[iNum];
            while ( iNum-- > 0 ) *piOn++ = pMmio->ReadLong( );
        }
        if ( ( pRi->m_iNumBldgsRequired = pMmio->ReadLong( ) ) > 0 )
        {
            int  iNum = pRi->m_iNumBldgsRequired;
            int* piOn = pRi->m_piBldgsRequired = new int[iNum];
            while ( iNum-- > 0 ) *piOn++ = pMmio->ReadLong( );
        }
        pMmio->AscendChunk( );
    }
    pMmio->AscendList( );

    delete pMmio;

    // get the text
    pMmio = theDataFile.OpenAsMMIO( NULL, "LANG" );

    pMmio->DescendRiff( 'L', 'A', 'N', 'G' );
    pMmio->DescendList( 'R', 'S', 'R', 'H' );

    for ( int iOn = 0; iOn < iSize; iOn++ )
    {
        CRsrchItem* pRi = &ElementAt( iOn + 1 );
        pMmio->DescendChunk( 'D', 'A', 'T', 'A' );
        pMmio->ReadString( pRi->m_sName );
        pMmio->AscendChunk( );
        pMmio->DescendChunk( 'D', 'E', 'S', 'C' );
        pMmio->ReadString( pRi->m_sDesc );
        pMmio->AscendChunk( );
        pMmio->DescendChunk( 'R', 'S', 'L', 'T' );
        pMmio->ReadString( pRi->m_sResult );
        pMmio->AscendChunk( );
    }
    pMmio->AscendList( );
    delete pMmio;

    // In-code research topic: Pontoon Bridges (not in the DAT file). An EARLY, cheap
    // bridge tech that unlocks bridge building at HALF the span of the full Bridges
    // tech (see CPlayer::GetMaxSpan). Its gate mirrors Mid-sized Buildings (the entry
    // of the building line): same scenario + same BUILDING prerequisites, and no
    // research prerequisite. The full Bridges tech is then gated BEHIND Pontoon (we
    // append it to Bridges' prereqs below). The AI can still reach Pontoon via its
    // randomized research fallback (CAIGoalMgr::NextResearchTopic), so gating Bridges
    // behind a tech the frozen RDPath can't see no longer locks the AI out.
    {
        CRsrchItem* pRi   = &ElementAt( bridge_short );
        CRsrchItem* pFull = &ElementAt( bridge );
        CRsrchItem* pMid  = &ElementAt( medium_facilities );   // "Mid-sized Buildings"

        pRi->m_iPtsRequired      = __max( 1, pFull->m_iPtsRequired / 2 );  // half of Bridge Building
        pRi->m_iScenarioReq      = pMid->m_iScenarioReq;                   // same gate as Mid-sized Buildings
        pRi->m_iNumRsrchRequired = 0;                                      // no research prereq

        // Same BUILDING prerequisites as Mid-sized Buildings.
        pRi->m_iNumBldgsRequired = pMid->m_iNumBldgsRequired;
        if ( pMid->m_iNumBldgsRequired > 0 )
        {
            pRi->m_piBldgsRequired = new int[pMid->m_iNumBldgsRequired];
            for ( int i = 0; i < pMid->m_iNumBldgsRequired; i++ )
                pRi->m_piBldgsRequired[i] = pMid->m_piBldgsRequired[i];
        }

        pRi->m_sName   = "Pontoon Bridges";
        pRi->m_sDesc   = "Light floating pontoon spans let our engineers bridge narrow water early, at half the reach of full bridge engineering.";
        pRi->m_sResult = "Pontoon bridges are ready. Our engineers can now bridge short stretches of water.";

        // Gate full Bridge Building behind Pontoon Bridges: append bridge_short to its
        // existing prerequisites (the one existing tech we modify, per the bridge
        // exception). Keeps the old prereqs (e.g. Mid-sized Buildings) and adds ours.
        int  iOldN = pFull->m_iNumRsrchRequired;
        int* piNew = new int[iOldN + 1];
        for ( int i = 0; i < iOldN; i++ )
            piNew[i] = pFull->m_piRsrchRequired[i];
        piNew[iOldN] = (int)bridge_short;
        delete[] pFull->m_piRsrchRequired;
        pFull->m_piRsrchRequired   = piNew;
        pFull->m_iNumRsrchRequired = iOldN + 1;
    }

    // In-code research topics: Bridges 2-5 (not in the DAT file). Each tier costs
    // double the previous tier's points, requires the previous tier, and extends
    // the max bridge span by +25% of the base span (see CPlayer::GetMaxSpan).
    {
        static char const* aszName[4] = { "Composite Trusses", "Tensile Spans", "Suspension Lattice", "Monofilament Spans" };
        static char const* aszDesc[4] = {
            "Lightweight composite trusses let our bridges span 25% more water than the original design.",
            "High-tension tensile members reach 50% farther across the water than the original design.",
            "A self-bracing suspension lattice carries bridges 75% farther than the original design.",
            "Monofilament cabling, stronger than steel at a fraction of the weight, lets a single bridge span twice as much water as the original design." };
        static char const* aszRslt[4] = {
            "Composite trusses approved. Our engineers can now build bridges 25% longer.",
            "Tensile spans mastered. Our engineers can now build bridges 50% longer.",
            "The suspension lattice is field-ready. Our engineers can now build bridges 75% longer.",
            "Monofilament spans perfected. Our engineers can now build bridges twice as long." };

        // Extra (cross-line) prereq per tier, on top of the previous tier. -1 = none.
        // Composites lean on manufacturing; longer spans on heavier construction;
        // monofilament on advanced (nuclear-era) materials science.
        static const int aiExtra[4] = {
            (int)manf_1, (int)const_2, (int)const_3, (int)nuclear };

        int iPts = ElementAt( bridge ).m_iPtsRequired;
        for ( int iOn = 0; iOn < 4; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( bridge_2 + iOn );

            iPts *= 2;
            pRi->m_iPtsRequired       = iPts;
            pRi->m_iScenarioReq       = ElementAt( bridge ).m_iScenarioReq;

            int iChain = ( 0 == iOn ) ? (int)bridge : (int)( bridge_2 + iOn - 1 );
            int nReq   = 1 + ( aiExtra[iOn] >= 0 ? 1 : 0 );
            pRi->m_iNumRsrchRequired  = nReq;
            pRi->m_piRsrchRequired    = new int[nReq];
            pRi->m_piRsrchRequired[0] = iChain;
            if ( aiExtra[iOn] >= 0 )
                pRi->m_piRsrchRequired[1] = aiExtra[iOn];

            pRi->m_sName              = aszName[iOn];
            pRi->m_sDesc              = aszDesc[iOn];
            pRi->m_sResult            = aszRslt[iOn];
        }
    }

    // In-code research topics: the Cargo Handling line (not in the DAT file). Mirrors
    // the bridge tiers; each costs double the previous tier's points and requires the
    // previous tier. Each level adds +10% truck cargo capacity over the base; with the
    // base cargo_handling research (+10%) the four levels run 110%..140% of stock
    // capacity (see CPlayer::GetCargoPct).
    {
        static char const* aszName[3] = { "Servo-Loaders", "Modular Cargo Pods", "Grav-Assisted Hauling" };
        static char const* aszDesc[3] = {
            "Powered servo arms load and stow freight with no wasted space, letting trucks carry 20% more than a stock vehicle.",
            "Sealed modular pods lock together and stack tighter, raising truck capacity to 30% over stock.",
            "Gravitic load compensators let trucks bear far denser cargo, 40% over stock." };
        static char const* aszRslt[3] = {
            "Servo-loaders are online. Our trucks now haul 20% more cargo.",
            "Modular cargo pods are in service. Our trucks now haul 30% more cargo.",
            "Grav-assisted hauling is operational. Our trucks now haul 40% more cargo." };

        // Extra (cross-line) prereq per tier, on top of the previous tier. -1 = none.
        // Servo-loaders and modular pods need manufacturing to build; grav-assisted
        // hauling needs advanced (nuclear-era) physics.
        static const int aiExtra[3] = {
            (int)manf_1, (int)manf_2, (int)nuclear };

        int iPts = ElementAt( cargo_handling ).m_iPtsRequired;
        for ( int iOn = 0; iOn < 3; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( cargo_handling_2 + iOn );

            iPts *= 2;
            pRi->m_iPtsRequired       = iPts;
            pRi->m_iScenarioReq       = ElementAt( cargo_handling ).m_iScenarioReq;

            int iChain = ( 0 == iOn ) ? (int)cargo_handling : (int)( cargo_handling_2 + iOn - 1 );
            int nReq   = 1 + ( aiExtra[iOn] >= 0 ? 1 : 0 );
            pRi->m_iNumRsrchRequired  = nReq;
            pRi->m_piRsrchRequired    = new int[nReq];
            pRi->m_piRsrchRequired[0] = iChain;
            if ( aiExtra[iOn] >= 0 )
                pRi->m_piRsrchRequired[1] = aiExtra[iOn];

            pRi->m_sName              = aszName[iOn];
            pRi->m_sDesc              = aszDesc[iOn];
            pRi->m_sResult            = aszRslt[iOn];
        }
    }

    // In-code research topics: the Fuel Efficiency line (not in the DAT file). A 10-
    // level repeatable line unlocked after Gas Turbines; level 1 requires gas_turbine,
    // each later level requires the previous. Each level costs DOUBLE the previous in
    // points, and cuts gas consumption by 5% of what remains (diminishing; the engine
    // applies 0.95^level via CPlayer::GetFuelPct). Names run grounded -> sci-fi.
    {
        static char const* aszName[10] = {
            "Fuel Injection",      "Lean-Burn Mapping",   "Turbo Compounding",
            "Regenerative Drive",  "Thermal Recovery",    "Plasma Ignition",
            "Catalytic Reclamation","Synthetic Lubricants","Ion Scavenging",
            "Zero-Loss Cycle" };
        static char const* aszDesc[10] = {
            "Precision fuel injection meters every drop, trimming gas burn by 5%.",
            "Lean-burn engine mapping squeezes more travel from less gas, saving another 5%.",
            "Turbo-compounding recovers exhaust energy back into the drivetrain for a further 5%.",
            "Regenerative drives recapture braking energy, cutting gas burn another 5%.",
            "Thermal recovery loops harvest waste engine heat, burning 5% less gas.",
            "Plasma ignition burns fuel cleaner and more completely, saving a further 5%.",
            "Catalytic reclamation refines spent fuel back into the tank for another 5%.",
            "Synthetic lubricants slash friction losses across the drivetrain, saving 5%.",
            "Ion scavengers strip the last usable energy from the exhaust stream, another 5%.",
            "A near-closed-loop drive cycle wastes almost nothing, shaving a final 5%." };
        static char const* aszRslt[10] = {
            "Fuel injection is fielded. Our vehicles burn 5% less gas.",
            "Lean-burn mapping is live. Our vehicles burn less gas still.",
            "Turbo compounding is online. Gas consumption drops again.",
            "Regenerative drives are installed. Our fleet sips even less gas.",
            "Thermal recovery is running. Gas burn falls further.",
            "Plasma ignition is operational. Cleaner burn, less gas used.",
            "Catalytic reclamation is online. Our vehicles stretch every tank further.",
            "Synthetic lubricants are in service. Friction losses cut, gas saved.",
            "Ion scavenging is active. Almost nothing leaves the exhaust unused.",
            "The zero-loss cycle is perfected. Our vehicles burn the least gas possible." };

        // Extra (cross-line) prereq per level, on top of the previous level. -1 = none.
        // Turbo compounding leans on manufacturing; plasma ignition needs nuclear-era
        // physics. Levels above each gate inherit it through the chain, so we only
        // pin it once where it first becomes necessary.
        static const int aiExtra[10] = {
            -1, -1, (int)manf_1, -1, -1, (int)nuclear, -1, -1, -1, -1 };

        int iPts = ElementAt( gas_turbine ).m_iPtsRequired;
        for ( int iOn = 0; iOn < 10; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( fuel_efficiency_1 + iOn );

            pRi->m_iPtsRequired       = iPts;   // level 1 = gas_turbine cost; doubles each level
            pRi->m_iScenarioReq       = ElementAt( gas_turbine ).m_iScenarioReq;

            int iChain = ( 0 == iOn ) ? (int)gas_turbine : (int)( fuel_efficiency_1 + iOn - 1 );
            int nReq   = 1 + ( aiExtra[iOn] >= 0 ? 1 : 0 );
            pRi->m_iNumRsrchRequired  = nReq;
            pRi->m_piRsrchRequired    = new int[nReq];
            pRi->m_piRsrchRequired[0] = iChain;
            if ( aiExtra[iOn] >= 0 )
                pRi->m_piRsrchRequired[1] = aiExtra[iOn];

            pRi->m_sName              = aszName[iOn];
            pRi->m_sDesc              = aszDesc[iOn];
            pRi->m_sResult            = aszRslt[iOn];

            iPts *= 2;   // each level costs twice as much as the previous
        }
    }

    // In-code research topics: Vehicle Speed 1-10 (not in the DAT file). Each level
    // adds +2% vehicle movement speed (see CPlayer::GetSpeedPct). Gated off the Fuel
    // Efficiency line: level 1 requires the first TWO fuel-efficiency techs, and each
    // later level requires the previous speed level plus the NEXT fuel-efficiency level
    // (so it climbs in lock-step with fuel economy). Level 10 has no higher fuel level
    // to gate on, so it just requires speed level 9. The AI reaches these via its
    // randomized research fallback.
    {
        static char const* aszName[10] = {
            "Tuned Drivetrains",  "High-Torque Gearing", "Lightweight Frames",
            "Active Suspension",  "Variable Transmission","Aerodynamic Profiling",
            "Magnetic Bearings",  "Composite Drivetrains","Vectored Thrust",
            "Inertial Dampeners" };
        static char const* aszDesc[10] = {
            "Tuned drivetrains deliver power more efficiently, moving every vehicle 2% faster.",
            "High-torque gearing puts more of the engine to the wheels and tracks, adding 2% speed.",
            "Lighter structural frames cut dead weight, adding another 2% to vehicle speed.",
            "Active suspension keeps wheels and tracks planted over rough ground, adding 2% speed.",
            "A variable transmission keeps engines in their power band, adding 2% speed.",
            "Aerodynamic profiling trims drag across the fleet, adding 2% speed.",
            "Frictionless magnetic bearings cut drivetrain losses, adding 2% speed.",
            "Composite drivetrains shed weight and friction together, adding 2% speed.",
            "Vectored thrust adds a push where wheels and tracks cannot, adding 2% speed.",
            "Inertial dampeners shrug off acceleration losses, adding a final 2% speed." };
        static char const* aszRslt[10] = {
            "Tuned drivetrains are fielded. Our vehicles move 2% faster.",
            "High-torque gearing is installed. Our vehicles move faster still.",
            "Lightweight frames are in service. Our vehicles pick up more speed.",
            "Active suspension is online. Our vehicles move faster over any terrain.",
            "Variable transmissions are fielded. Our vehicles gain more speed.",
            "Aerodynamic profiling is complete. Our vehicles move faster.",
            "Magnetic bearings are running. Our vehicles gain still more speed.",
            "Composite drivetrains are in service. Our vehicles move faster.",
            "Vectored thrust is operational. Our vehicles surge ahead.",
            "Inertial dampeners are perfected. Our vehicles reach their top speed." };

        // Extra (cross-line) prereq per level, on top of the chain + fuel prereqs.
        // -1 = none. Aerodynamic Profiling (level 6) also needs Fuel-Air Explosive
        // (atk_3, the top shell-damage tech) — aerodynamics + warhead crossover.
        static const int aiExtra[10] = {
            -1, -1, -1, -1, -1, (int)atk_3, -1, -1, -1, -1 };

        for ( int iOn = 0; iOn < 10; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( vehicle_speed_1 + iOn );

            pRi->m_iPtsRequired      = 50000 + 25000 * iOn;                       // modest, gating does the pacing
            pRi->m_iScenarioReq      = ElementAt( gas_turbine ).m_iScenarioReq;   // fuel line's campaign gate
            pRi->m_iNumBldgsRequired = 0;

            // Build the prereq list: level 1 = first two fuel-efficiency techs; levels
            // 2-9 = previous speed level + the next fuel-efficiency level; level 10 =
            // previous speed level only (fuel line exhausted). Plus any per-level extra.
            int aiReq[4];
            int nReq = 0;
            if ( iOn == 0 )
            {
                aiReq[nReq++] = (int)fuel_efficiency_1;
                aiReq[nReq++] = (int)fuel_efficiency_2;
            }
            else
            {
                aiReq[nReq++] = (int)( vehicle_speed_1 + iOn - 1 );
                if ( iOn < 9 )
                    aiReq[nReq++] = (int)( fuel_efficiency_1 + iOn + 1 );   // fuel level (iOn+2)
            }
            if ( aiExtra[iOn] >= 0 )
                aiReq[nReq++] = aiExtra[iOn];

            pRi->m_iNumRsrchRequired = nReq;
            pRi->m_piRsrchRequired   = new int[nReq];
            for ( int k = 0; k < nReq; k++ )
                pRi->m_piRsrchRequired[k] = aiReq[k];

            pRi->m_sName   = aszName[iOn];
            pRi->m_sDesc   = aszDesc[iOn];
            pRi->m_sResult = aszRslt[iOn];
        }
    }

    // Radar/Spotting tiers 4-5 (in-code) — extend the DAT spot_1..3 line. Each tier costs
    // 2x the previous tier's points and chains off it (spot_4<-spot_3, spot_5<-spot_4).
    // Per-level sight bonus (diminishing) is in CUnit::AssignData. The AI's frozen research
    // path doesn't pursue these (optional human tiers), like the other in-code lines.
    {
        static const char* aszSpotName[2] = { "Enhanced Sensors", "Deep-Scan Array" };
        static const char* aszSpotDesc[2] = {
            "Refined sensor arrays extend our units' sight a little further.",
            "Deep-scanning sensors push our sight range to its practical limit." };
        static const char* aszSpotRslt[2] = {
            "Enhanced Sensors online. Our units see a bit further.",
            "Deep-Scan Array online. Our units see as far as the hardware allows." };
        int aiSpotPrev[2] = { (int)spot_3, (int)spot_4 };   // spot_5 reads spot_4 after it's set below
        for ( int iOn = 0; iOn < 2; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( spot_4 + iOn );
            pRi->m_iPtsRequired       = ElementAt( aiSpotPrev[iOn] ).m_iPtsRequired * 2;   // 2x the previous tier
            pRi->m_iScenarioReq       = ElementAt( spot_3 ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired  = 0;
            pRi->m_iNumRsrchRequired  = 1;
            pRi->m_piRsrchRequired    = new int[1];
            pRi->m_piRsrchRequired[0] = aiSpotPrev[iOn];
            pRi->m_sName   = aszSpotName[iOn];
            pRi->m_sDesc   = aszSpotDesc[iOn];
            pRi->m_sResult = aszSpotRslt[iOn];
        }
    }

    // Landing Craft capacity tiers 2-3 (in-code) — each adds +1 to the landing craft's
    // unit hold (base 2 -> 3 -> 4). Fairly expensive: 4x then 8x the base landing_craft
    // tech's points. Chain off it (lc_2 <- landing_craft, lc_3 <- lc_2). Capacity bonus
    // applied in CVehicle::GetEffPeopleCarry via CPlayer::GetLandingCraftBonus. The AI's
    // frozen research path doesn't pursue these (optional human tiers).
    {
        static const char* aszLcName[2] = { "Expanded Landing Bay", "Reinforced Landing Bay" };
        static const char* aszLcDesc[2] = {
            "Reworked internal bracing lets a landing craft ferry a third unit.",
            "A fully reinforced hold lets a landing craft ferry a fourth unit." };
        static const char* aszLcRslt[2] = {
            "Expanded Landing Bay online. Landing craft now carry three units.",
            "Reinforced Landing Bay online. Landing craft now carry four units." };
        int aiLcPrev[2] = { (int)landing_craft, (int)landing_craft_2 };
        int aiLcMul[2]  = { 4, 8 };   // fairly expensive vs the base landing_craft tech
        for ( int iOn = 0; iOn < 2; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( landing_craft_2 + iOn );
            pRi->m_iPtsRequired       = ElementAt( landing_craft ).m_iPtsRequired * aiLcMul[iOn];
            pRi->m_iScenarioReq       = ElementAt( landing_craft ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired  = 0;
            pRi->m_iNumRsrchRequired  = 1;
            pRi->m_piRsrchRequired    = new int[1];
            pRi->m_piRsrchRequired[0] = aiLcPrev[iOn];
            pRi->m_sName   = aszLcName[iOn];
            pRi->m_sDesc   = aszLcDesc[iOn];
            pRi->m_sResult = aszLcRslt[iOn];
        }
    }

    // In-code research topics: Fracking 1-5 (#23, not in the DAT file). Exhausted oil
    // wells trickle oil when fracking is toggled ON (consumed in the mine production hook
    // via CPlayer::GetFrackOilPerMin), at +50% well energy. Each tier costs DOUBLE the
    // previous and chains the prior tier; T1 needs gas_turbine, later tiers also a
    // Fuel-Efficiency level. The AI's frozen research path doesn't pursue these (optional
    // human tiers). Point/gate values are easy to retune — operator to balance in-game.
    {
        static const char* aszFrName[5] = {
            "Hydraulic Fracturing", "Horizontal Drilling", "Proppant Injection",
            "Microseismic Mapping", "Supercritical Extraction" };
        static const char* aszFrDesc[5] = {
            "High-pressure fluid fractures spent rock, coaxing a 10/min oil trickle from exhausted wells.",
            "Horizontal bores reach untapped pockets, lifting the trickle to 15/min.",
            "Engineered proppants hold fractures open longer, raising recovery to 20/min.",
            "Microseismic mapping targets the richest seams, yielding 25/min.",
            "Supercritical solvents strip the last bound oil from dead rock, 30/min." };
        static const char* aszFrRslt[5] = {
            "Hydraulic fracturing online. Exhausted wells now trickle oil (toggle per well).",
            "Horizontal drilling fielded. Fracked wells yield more oil.",
            "Proppant injection in service. Fracked-well oil rises again.",
            "Microseismic mapping operational. Fracked wells reach deeper pockets.",
            "Supercritical extraction perfected. Maximum oil from spent wells." };
        // Extra (cross-line) prereq per tier, on top of the previous tier. -1 = none.
        static const int aiFrExtra[5] = {
            -1, (int)fuel_efficiency_1, (int)fuel_efficiency_3, (int)fuel_efficiency_5, (int)fuel_efficiency_8 };

        int iPts = ElementAt( gas_turbine ).m_iPtsRequired;
        for ( int iOn = 0; iOn < 5; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( fracking_1 + iOn );

            iPts *= 2;
            pRi->m_iPtsRequired       = iPts;
            pRi->m_iScenarioReq       = ElementAt( gas_turbine ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired  = 0;

            // #23 (operator Note 17): fracking should be LATE-game ("especially fracking").
            // T1 prereq gas_turbine -> nuclear (a late gate, later than coal-liq's Adv-Mfg).
            // mac1's pick; @linux2/@win adjust the exact tech if balance wants different.
            int iChain = ( 0 == iOn ) ? (int)nuclear : (int)( fracking_1 + iOn - 1 );
            int nReq   = 1 + ( aiFrExtra[iOn] >= 0 ? 1 : 0 );
            pRi->m_iNumRsrchRequired  = nReq;
            pRi->m_piRsrchRequired    = new int[nReq];
            pRi->m_piRsrchRequired[0] = iChain;
            if ( aiFrExtra[iOn] >= 0 )
                pRi->m_piRsrchRequired[1] = aiFrExtra[iOn];

            pRi->m_sName   = aszFrName[iOn];
            pRi->m_sDesc   = aszFrDesc[iOn];
            pRi->m_sResult = aszFrRslt[iOn];
        }
    }

    // In-code research topics: BioFuel 1-6 (#33, not in the DAT file). Unlocks a refinery
    // mode toggle: a refinery switched to Bio Oil stops converting oil into gas and instead
    // converts global FOOD into oil (~5 food -> 1 oil), consumed in the refinery production
    // hook. Each tier costs DOUBLE the previous and chains the prior tier; T1 is a heavy
    // multi-line gate (farming + gas turbines + some fuel efficiency + vehicle speed), no
    // energy cost. The AI's frozen research path doesn't pursue these.
    {
        static const char* aszBfName[6] = {
            "Biomass Digestion", "Algae Bioreactors", "Enzymatic Cracking",
            "Cellulosic Synthesis", "Gene-Tuned Oilseed", "Closed-Loop Biorefinery" };
        static const char* aszBfDesc[6] = {
            "Lets a refinery convert food into oil instead of oil into gas, rendering surplus food down to fuel oil.",
            "Algae bioreactors enrich the refinery's food-to-oil conversion, lifting its oil yield.",
            "Enzymatic cracking breaks food stock down more completely, raising the food-to-oil yield again.",
            "Cellulosic synthesis wrings oil from tougher food matter, improving the conversion further.",
            "Gene-tuned oilseed feedstock pushes the refinery's food-to-oil yield higher still.",
            "A closed-loop biorefinery wastes nothing, maximizing the oil drawn from each unit of food." };
        static const char* aszBfRslt[6] = {
            "Biomass digestion online. Refineries can now be toggled to convert food into oil instead of oil into gas.",
            "Algae bioreactors fielded. The refinery food-to-oil conversion yields more oil.",
            "Enzymatic cracking in service. More oil from the same food.",
            "Cellulosic synthesis operational. Refinery food-to-oil yield climbs again.",
            "Gene-tuned oilseed adopted. Refinery food-to-oil yield rises further.",
            "Closed-loop biorefinery perfected. Maximum oil from every unit of food converted." };

        int iPts = ElementAt( farm_1 ).m_iPtsRequired;
        for ( int iOn = 0; iOn < 6; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( biofuel_1 + iOn );

            iPts *= 2;
            pRi->m_iPtsRequired       = iPts;
            pRi->m_iScenarioReq       = ElementAt( farm_1 ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired  = 0;

            if ( 0 == iOn )
            {
                // Heavy multi-line entry gate.
                static const int aiBf1[4] = {
                    (int)farm_1, (int)gas_turbine, (int)fuel_efficiency_2, (int)vehicle_speed_2 };
                pRi->m_iNumRsrchRequired  = 4;
                pRi->m_piRsrchRequired    = new int[4];
                for ( int k = 0; k < 4; k++ ) pRi->m_piRsrchRequired[k] = aiBf1[k];
            }
            else
            {
                pRi->m_iNumRsrchRequired  = 1;
                pRi->m_piRsrchRequired    = new int[1];
                pRi->m_piRsrchRequired[0] = (int)( biofuel_1 + iOn - 1 );
            }

            pRi->m_sName   = aszBfName[iOn];
            pRi->m_sDesc   = aszBfDesc[iOn];
            pRi->m_sResult = aszBfRslt[iOn];
        }
    }

    // In-code research topic: Coal Liquefaction (1 tier, not in the DAT file). A coal
    // POWER PLANT, once this is researched and its per-building alt-output toggle is ON,
    // also converts coal into oil (2 coal -> 1 oil) via the shared AltOutput system
    // (eRatioConsume). Single tech, chained off Gas Turbines; cost ~4x the gas_turbine
    // tech (a few x, easy to retune -- operator balances in-game). Appended LAST in the
    // enum so save indices don't shift.
    {
        CRsrchItem* pRi = &ElementAt( coal_liquefaction );

        pRi->m_iPtsRequired      = ElementAt( gas_turbine ).m_iPtsRequired * 4;
        pRi->m_iScenarioReq      = ElementAt( gas_turbine ).m_iScenarioReq;
        pRi->m_iNumBldgsRequired = 0;

        pRi->m_iNumRsrchRequired = 1;
        pRi->m_piRsrchRequired   = new int[1];
        // #28 (operator Note 23): gate Coal Liquefaction behind ADVANCED MANUFACTURING
        // (manf_3) — it appeared too early off gas_turbine. (Cost basis left as-is;
        // operator retunes in-game.)
        pRi->m_piRsrchRequired[0] = (int)manf_3;

        pRi->m_sName   = "Coal Liquefaction";
        pRi->m_sDesc   = "Fischer-Tropsch synthesis cracks coal into liquid fuel: a toggled coal power plant turns 3 coal into 1 oil.";
        pRi->m_sResult = "Coal liquefaction online. Coal power plants can convert coal to oil (toggle per plant).";
    }

    // In-code research topic: Charcoal (4 tiers, not in the DAT file). A lumber MILL (the
    // sawmill -- UTfarm whose GetTypeFarm() == lumber), once a Charcoal tier is researched
    // and its per-building alt-output toggle is ON, runs a kiln: it converts harvested
    // lumber into coal ("Charcoal" label only) at a fixed 2 lumber -> 1 coal via the shared
    // AltOutput system (eRatioConsume), MODE-SWITCH (lumber output stops while the kiln
    // runs). The 2:1 ratio is fixed; the THROUGHPUT is tier-scaled by CPlayer::GetCharcoalPct
    // (T1 = VERY LOW per operator spec, T2-4 raise it). No energy cost. T1 chained off Gas
    // Turbines; T2-4 chain the prior tier (mirrors the BioFuel line). Cost doubles each tier.
    // Appended LAST in the enum so save indices don't shift. The AI's frozen research path
    // doesn't pursue these.
    {
        static const char* aszChName[4] = {
            "Charcoal Kiln", "Retort Kiln", "Continuous Carbonization", "Pyrolysis Refinery" };
        static const char* aszChDesc[4] = {
            "A simple wood kiln chars lumber into coal: a toggled sawmill converts 2 lumber into 1 coal at a very low rate.",
            "Sealed retort kilns char lumber more efficiently, raising the sawmill's charcoal output.",
            "Continuous carbonization lines keep the kiln running, raising charcoal output again.",
            "A full pyrolysis refinery wrings the most charcoal from every log." };
        static const char* aszChRslt[4] = {
            "Charcoal kiln online. Sawmills can convert lumber into coal (toggle per mill).",
            "Retort kilns fielded. Sawmill charcoal output rises.",
            "Continuous carbonization in service. More charcoal per log.",
            "Pyrolysis refinery perfected. Maximum charcoal from every sawmill." };

        int iPts = ElementAt( gas_turbine ).m_iPtsRequired;
        for ( int iOn = 0; iOn < 4; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( charcoal_1 + iOn );

            iPts *= 2;
            pRi->m_iPtsRequired       = iPts;
            pRi->m_iScenarioReq       = ElementAt( gas_turbine ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired  = 0;

            pRi->m_iNumRsrchRequired  = 1;
            pRi->m_piRsrchRequired    = new int[1];
            pRi->m_piRsrchRequired[0] = ( 0 == iOn ) ? (int)gas_turbine : (int)( charcoal_1 + iOn - 1 );

            pRi->m_sName   = aszChName[iOn];
            pRi->m_sDesc   = aszChDesc[iOn];
            pRi->m_sResult = aszChRslt[iOn];
        }
    }

#ifdef _DEBUG
    theDataFile.DisableNegativeSeekChecking( );
    theDataFile.EnableNegativeSeekChecking( );
#endif

    ASSERT_VALID( this );
}

void CRsrchArray::Close( )
{

    ASSERT_VALID( this );

    for ( int iOn = 0; iOn < GetSize( ); iOn++ )
    {
        CRsrchItem* pRi = &ElementAt( iOn );
        delete pRi->m_piRsrchRequired;
        delete pRi->m_piBldgsRequired;
        pRi->m_piRsrchRequired = NULL;
        pRi->m_piBldgsRequired = NULL;
    }

    RemoveAll( );
}

