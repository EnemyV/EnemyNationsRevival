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

        // Building gate is ONE-OF light factory / refinery / heavy factory,
        // special-cased in CPlayer::CanRsrch (the prereq array is AND-semantics
        // only). Mid-sized Buildings' prereq (light factory only) starved
        // river-split AIs that had a refinery but no factory (operator).
        pRi->m_iNumBldgsRequired = 0;

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

    // In-code research topics: the Fuel Efficiency line (not in the DAT file). A 23-level
    // line unlocked after Gas Turbines; level 1 requires gas_turbine, each later the prev.
    // Cost DOUBLES to 32*B at level 6, then flat +16*B (L7=48B ... L23=304B; B = gas_turbine
    // cost). Gas saving diminishes 5/4/4/3/3/3/2/2/2/2 to 30% at level 10, then +1% per level
    // to 38% at level 18, then +2% per level to 48% at level 23 (see CPlayer::GetFuelPct).
    // The late levels are deliberately mundane garage tweaks (additives, thinner oil, valve
    // timing), not sci-fi. NOTE: no level's text may claim to be the "last" or "final" one --
    // this line has been extended four times and every such claim had to be walked back.
    // Levels 1-10 are contiguous; 11-12, 13-16, 17-18 and 19-23 were appended at the enum end
    // for save parity, so the setup loop maps the index through aiIdx[] rather than a
    // running offset.
    {
        static char const* aszName[23] = {
            "Fuel Injection",      "Lean-Burn Tuning",    "Turbo Compounding",
            "Regenerative Braking","Waste-Heat Recovery", "Better Spark Timing",
            "Exhaust Reclamation", "Synthetic Lubricants","Low-Friction Bearings",
            "Reduced Rolling Resistance","Fuel Additives", "Tighter Tolerances",
            "Low-Viscosity Oil",   "Cleaner Fuel Filters","Lightweight Flywheels",
            "Idle Cutoff",         "Coasting Governor",   "Fuel Preheating",
            "Variable Valve Timing","Cylinder Deactivation","Ceramic Cylinder Liners",
            "Two-Stage Turbocharging","Hydraulic Hybrid Drive" };
        static char const* aszDesc[23] = {
            "Our engines still gulp fuel through crude carburetors. We should be able to meter each drop with proper injection and burn a good 5% less gas.",
            "We think we can tune the engines to run leaner, coaxing more travel out of every tank for another 4%.",
            "All that hot exhaust just blows away. If we feed it back through a turbine we should recover another 4% of the fuel.",
            "Every time a vehicle slows down we throw away good energy as heat. We should be able to catch some of it back, worth about 3%.",
            "Our engines run hot enough to cook dinner on. We think we can scavenge that waste heat for a further 3%.",
            "Our ignition timing is a guess at best. Dialing in the spark should burn the charge more completely for another 3%.",
            "There is still unburned fuel going out the tailpipe. We should be able to catch and re-burn it for another 2%.",
            "The local oils gum up in this climate. A proper synthetic lubricant should cut friction across the drivetrain for 2%.",
            "Our bearings are rougher than we would like. Polishing them to a low-friction finish should be good for another 2%.",
            "Our wheels and tracks fight the ground the whole way. Trimming that rolling resistance should save another 2%, a full 30% by now.",
            "The local crude is full of grit. A dose of the right additives should keep our engines from gumming up and save another 1%.",
            "If our machinists shave the tolerances a little finer, the engines will leak a bit less power, worth about 1%.",
            "A thinner oil would let everything spin easier once the engine warms up. We think that is good for another 1%.",
            "Half the dirt on this planet ends up in our fuel lines. Finer filters should keep the injectors happy for another 1%.",
            "Our flywheels are heavier than they need to be. Shaving them down should free up about 1%.",
            "Vehicles sitting idle just drink fuel for nothing. A cutoff that stops the engine when they wait should save 1%.",
            "On a downhill our engines keep pulling when they could just coast. A governor to ease off should be worth another 1%.",
            "Cold fuel burns poorly in this thin air. Warming it before it hits the cylinder should wring out another 1%.",
            "Our valves open and shut on a fixed cam whether the engine is crawling or flat out. We should be able to let the timing shift with the load and save another 2%.",
            "On an empty run half our cylinders are just along for the ride. We think we can shut them down when the load is light for another 2%.",
            "Our cylinder walls bleed heat straight into the coolant. Lining them with ceramic should keep that heat in the burn, and it's good for another 2%.",
            "One turbo is always either too small or too big. However, if we stage a small one behind a large one we should cover the whole range and save another 2%.",
            "We should be able to store the energy of a stop in a pressure accumulator and spend it pulling away again. It's heavy and it's a lot of plumbing, but it should be worth another 2%." };
        static char const* aszRslt[23] = {
            "Fuel injection is working. Our vehicles stop dumping gas down the intake and burn about 5% less of it.",
            "The engines run lean and clean now. Our vehicles squeeze another 4% out of every tank.",
            "Turbo compounding is fitted. The exhaust that used to blow away now helps drive the wheels, saving another 4%.",
            "Regenerative braking is installed. Our vehicles claw back the energy they used to burn off stopping, about 3% less gas.",
            "Waste-heat recovery is running. The heat that poured off our engines now does useful work, 3% less gas.",
            "The spark timing is dialed in. A cleaner burn means our vehicles use 3% less gas.",
            "Exhaust reclamation is online. What used to go out the pipe now goes back in the tank, 2% saved.",
            "Synthetic lubricants are in service. Nothing drags the way it used to, and we burn 2% less gas.",
            "Low-friction bearings are fitted. The wheels turn that much easier, saving another 2%.",
            "Rolling resistance is down. Our vehicles roll freer than ever, a full 30% less gas than when we landed.",
            "Fuel additives are in the mix. Our engines run cleaner on the local muck and burn 1% less (31% total).",
            "The parts fit tighter now. A little less slop, a little less waste, 1% saved (32% total).",
            "We switched to a lighter oil. Everything spins a touch freer, 1% less gas (33% total).",
            "Finer fuel filters are fitted. Cleaner fuel, happier injectors, 1% saved (34% total).",
            "Lighter flywheels are installed. Less dead weight to spin up, 1% less gas (35% total).",
            "Idle cutoff is fielded. Our vehicles stop guzzling while they sit around, 1% saved (36% total).",
            "The coasting governor works. Our vehicles freewheel where they can instead of burning gas, 1% saved (37% total).",
            "Fuel preheating is running. Even in the cold our engines burn every drop, 1% saved (38% total).",
            "We can now shift the valve timing with the load. The engines breathe right at every speed and burn 2% less gas (40% total).",
            "Cylinder deactivation is working. Our engines drop to half their cylinders on the easy stretches, 2% saved (42% total).",
            "The ceramic liners are in service. The heat stays where it does work instead of going out the radiator, 2% less gas (44% total).",
            "The two-stage turbos are fitted. There's boost from idle to redline now and we burn 2% less gas (46% total).",
            "We now have hydraulic hybrid drive. Our vehicles launch on stored pressure instead of fuel, 2% saved (48% total)." };

        // Extra (cross-line) prereq per level, on top of the previous level. -1 = none.
        // Turbo compounding leans on manufacturing; better spark timing needs nuclear-era
        // physics. Levels above each gate inherit it through the chain, so we only pin it
        // once where it first becomes necessary. The late mundane levels add no new gate.
        static const int aiExtra[23] = {
            -1, -1, (int)manf_1, -1, -1, (int)nuclear, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1 };

        // Level (0-based) -> enum id. Non-contiguous because 11-12, 13-16 and 17-18 were
        // appended at the enum end for save parity.
        static const int aiIdx[23] = {
            fuel_efficiency_1,  fuel_efficiency_2,  fuel_efficiency_3,  fuel_efficiency_4,  fuel_efficiency_5,
            fuel_efficiency_6,  fuel_efficiency_7,  fuel_efficiency_8,  fuel_efficiency_9,  fuel_efficiency_10,
            fuel_efficiency_11, fuel_efficiency_12, fuel_efficiency_13, fuel_efficiency_14,
            fuel_efficiency_15, fuel_efficiency_16, fuel_efficiency_17, fuel_efficiency_18,
            fuel_efficiency_19, fuel_efficiency_20, fuel_efficiency_21, fuel_efficiency_22,
            fuel_efficiency_23 };

        int iBase = ElementAt( gas_turbine ).m_iPtsRequired;   // B = gas_turbine cost
        int iPts  = iBase;                                     // level 1 = B
        for ( int iOn = 0; iOn < 23; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( aiIdx[iOn] );

            pRi->m_iPtsRequired       = iPts;   // level 1 = gas_turbine cost; doubles each level
            pRi->m_iScenarioReq       = ElementAt( gas_turbine ).m_iScenarioReq;

            int iChain = ( 0 == iOn ) ? (int)gas_turbine : aiIdx[iOn - 1];
            int nReq   = 1 + ( aiExtra[iOn] >= 0 ? 1 : 0 );
            pRi->m_iNumRsrchRequired  = nReq;
            pRi->m_piRsrchRequired    = new int[nReq];
            pRi->m_piRsrchRequired[0] = iChain;
            if ( aiExtra[iOn] >= 0 )
                pRi->m_piRsrchRequired[1] = aiExtra[iOn];

            pRi->m_sName              = aszName[iOn];
            pRi->m_sDesc              = aszDesc[iOn];
            pRi->m_sResult            = aszRslt[iOn];

            // Cost curve: double each level up to 32*B at level 6, then switch to a flat
            // +16*B per level (L6=32B -> L7=48B -> ... -> L18=224B). Keeps the top of the
            // line expensive but LINEAR, not the runaway 2x doubling.
            if ( iOn < 5 )
                iPts *= 2;            // levels 1->6 still double
            else
                iPts += 16 * iBase;   // level 6 onward: flat +16*B per level
        }
    }

    // In-code research topics: Vehicle Speed 1-10 (not in the DAT file). Each level
    // adds +2% vehicle movement speed (see CPlayer::GetSpeedPct). Gated off the Fuel
    // Efficiency line: level 1 requires the first TWO fuel-efficiency techs, and each
    // later level requires the previous speed level plus the NEXT fuel-efficiency level
    // (so it climbs in lock-step with fuel economy). Level 10 has no higher fuel level
    // to gate on, so it just requires speed level 9. The AI reaches these via its
    // randomized research fallback. COST: because the speed line unlocks LATE (each tier is
    // gated behind the matching fuel-efficiency tier), it is priced as a premium line that
    // KEEPS DOUBLING the whole way — B * 2^(tier-1) with NO flat cap (B = gas_turbine cost):
    // B, 2B, 4B ... 512B at tier 10, 1024B/2048B at 11/12. Unlike Fuel Efficiency (which
    // caps its doubling at 32*B then goes flat +16*B, staying affordable), speed escalates
    // continuously so the high tiers are appropriately expensive for how deep they unlock.
    // The extended tiers 11-12 continue this one curve (see the block below).
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
            "Inertial dampeners shrug off acceleration losses, adding another 2% speed." };
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
            "Inertial dampeners are installed. Our vehicles gain still more speed." };

        // Extra (cross-line) prereq per level, on top of the chain + fuel prereqs.
        // -1 = none. Aerodynamic Profiling (level 6) also needs Fuel-Air Explosive
        // (atk_3, the top shell-damage tech) — aerodynamics + warhead crossover.
        static const int aiExtra[10] = {
            -1, -1, -1, -1, -1, (int)atk_3, -1, -1, -1, -1 };

        int iBase = ElementAt( gas_turbine ).m_iPtsRequired;   // B, same base as Fuel Efficiency
        for ( int iOn = 0; iOn < 10; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( vehicle_speed_1 + iOn );

            // Premium curve: pure doubling, no flat cap — B * 2^(tier-1) (t10 = 512B).
            int iTier = iOn + 1;                                                  // 1..10
            pRi->m_iPtsRequired      = iBase << ( iTier - 1 );
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

    // In-code research topics: Vehicle Speed 11-12 (not in the DAT file). Two MORE speed
    // tiers, each +1% (vs +2% for 1-10). Chain off the previous speed tier; no fuel gate.
    // Cost continues the pure-doubling curve: tier 11 = 1024*B, tier 12 = 2048*B.
    {
        static char const* aszName[2] = {
            "Fluidic Drives", "Gyroscopic Stabilizers" };
        static char const* aszDesc[2] = {
            "Fluidic drivetrains smooth every power stroke, adding 1% vehicle speed.",
            "Gyroscopic stabilizers hold vehicles steady at pace, adding a final 1% speed." };
        static char const* aszRslt[2] = {
            "Fluidic drives are fielded. Our vehicles move 1% faster.",
            "Gyroscopic stabilizers are perfected. Our vehicles reach their top speed." };

        int iBase = ElementAt( gas_turbine ).m_iPtsRequired;   // B, same base as Fuel Efficiency
        for ( int iOn = 0; iOn < 2; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( vehicle_speed_11 + iOn );

            // Pure doubling, no cap: B * 2^(tier-1). Tier 11 = 1024*B, tier 12 = 2048*B.
            int iTier = 11 + iOn;                                                // 11..12
            pRi->m_iPtsRequired      = iBase << ( iTier - 1 );
            pRi->m_iScenarioReq      = ElementAt( gas_turbine ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired = 0;

            int iChain = ( 0 == iOn ) ? (int)vehicle_speed_10 : (int)( vehicle_speed_11 + iOn - 1 );
            pRi->m_iNumRsrchRequired  = 1;
            pRi->m_piRsrchRequired    = new int[1];
            pRi->m_piRsrchRequired[0] = iChain;

            pRi->m_sName   = aszName[iOn];
            pRi->m_sDesc   = aszDesc[iOn];
            pRi->m_sResult = aszRslt[iOn];
        }
    }

    // Radar/Spotting tiers 4-7 (in-code) — extend the DAT spot_1..3 line with four
    // diminishing-return levels. Each tier costs 2x the previous tier's points and chains
    // off it (spot_4<-spot_3, spot_5<-spot_4, spot_6<-spot_5, spot_7<-spot_6). Per-level
    // sight bonus (diminishing) is in CUnit::AssignData; level lookup in CPlayer::SetRsrch.
    // The AI's frozen research path doesn't pursue these (optional human tiers). NOTE:
    // spot_4/5 are contiguous, but spot_6/7 were appended at the END of the enum for save
    // parity, so the current + previous index are mapped non-contiguously below.
    {
        static const char* aszSpotName[4] = {
            "Enhanced Sensors", "Deep-Scan Array", "Quantum Radar", "Orbital Uplink" };
        static const char* aszSpotDesc[4] = {
            "Refined sensor arrays extend our units' sight a little further.",
            "Deep-scanning sensors push our sight range further still.",
            "Quantum radar teases faint returns from the noise, extending sight a little more.",
            "An orbital uplink relays a top-down view, pushing sight to its practical limit." };
        static const char* aszSpotRslt[4] = {
            "Enhanced Sensors online. Our units see a bit further.",
            "Deep-Scan Array online. Our units see further still.",
            "Quantum Radar online. Our units pick out targets further out.",
            "Orbital Uplink online. Our units see as far as the hardware allows." };
        for ( int iOn = 0; iOn < 4; iOn++ )
        {
            // spot_4/5 contiguous; spot_6/7 at the enum end. Map current + previous index.
            int iIdx  = ( iOn < 2 ) ? (int)( spot_4 + iOn ) : (int)( spot_6 + ( iOn - 2 ) );
            int iPrev = ( iOn == 0 ) ? (int)spot_3
                      : ( iOn - 1 < 2 ) ? (int)( spot_4 + iOn - 1 )
                                        : (int)( spot_6 + ( iOn - 1 - 2 ) );
            CRsrchItem* pRi = &ElementAt( iIdx );
            pRi->m_iPtsRequired       = ElementAt( iPrev ).m_iPtsRequired * 2;   // 2x the previous tier
            pRi->m_iScenarioReq       = ElementAt( spot_3 ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired  = 0;
            pRi->m_iNumRsrchRequired  = 1;
            pRi->m_piRsrchRequired    = new int[1];
            pRi->m_piRsrchRequired[0] = iPrev;
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

    // In-code research topics: Fracking 1-7 (#23, not in the DAT file). Exhausted oil
    // wells trickle oil when fracking is toggled ON (consumed in the mine production hook
    // via CPlayer::GetFrackOilPerMin), at +50% well energy. Each tier costs DOUBLE the
    // previous and chains the prior tier; T1 needs gas_turbine, later tiers also a
    // Fuel-Efficiency level. The AI's frozen research path doesn't pursue these (optional
    // human tiers). Point/gate values are easy to retune (operator balances in-game).
    {
        static const char* aszFrName[7] = {
            "Hydraulic Fracturing", "Horizontal Drilling", "Proppant Injection",
            "Microseismic Mapping", "Supercritical Extraction", "Thermal Flooding",
            "Electrokinetic Recovery" };
        static const char* aszFrDesc[7] = {
            "Our spent wells still hold oil we can't reach. If we fracture the rock with high pressure fluid we should be able to draw off 5 units a minute.",
            "A vertical bore only touches what lies straight beneath it. We should be able to drill sideways into the pockets it misses and take 7.",
            "Our fractures close again as soon as the pressure comes off. Engineered proppant should hold them open for 9.",
            "We are fracturing blind and wasting half the effort on dead rock. Listening to the seismic echoes should find us the better seams and 11.",
            "Some of the oil is bound to the rock and no pressure will shift it. A supercritical solvent should strip it loose for 13.",
            "The oil that is left clings too tightly to flow. However, if we flood the seam with steam we should be able to drive it out for 15.",
            "Even steam leaves oil behind in the tightest rock. We think we can walk it to the bore with a direct current and take 17." };
        static const char* aszFrRslt[7] = {
            "We can now fracture our spent wells. Turn one back on and an exhausted well gives up 5 units of oil a minute.",
            "We are now drilling horizontally. Our fracked wells reach what the old bores missed and give 7.",
            "The new proppants hold our fractures open. Our fracked wells now give 9.",
            "We can now map a seam by its own echoes. Our bores go where the oil is and the wells give 11.",
            "We have supercritical extraction working. It strips out oil that no pressure could move and the wells give 13.",
            "We have the steam flood operational. Even the most spent wells give up 15 now.",
            "We are now running a current through the seam. It draws out the oil the steam left behind and the wells give 17." };
        // Extra (cross-line) prereq per tier, on top of the previous tier. -1 = none.
        static const int aiFrExtra[7] = {
            -1, (int)fuel_efficiency_1, (int)fuel_efficiency_3, (int)fuel_efficiency_5, (int)fuel_efficiency_8, (int)fuel_efficiency_10,
            (int)fuel_efficiency_12 };

        // Level (0-based) -> enum id. Tiers 1-5 are contiguous; tier 6 was appended at the
        // enum end for save parity, so it is NOT fracking_5+1 -- map it explicitly.
        static const int aiFrIdx[7] = {
            fracking_1, fracking_2, fracking_3, fracking_4, fracking_5, fracking_6, fracking_7 };

        int iPts = ElementAt( gas_turbine ).m_iPtsRequired;
        for ( int iOn = 0; iOn < 7; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( aiFrIdx[iOn] );

            iPts *= 2;
            pRi->m_iPtsRequired       = iPts;
            pRi->m_iScenarioReq       = ElementAt( gas_turbine ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired  = 0;

            // #23 (operator Note 17): fracking should be LATE-game ("especially fracking").
            // T1 prereq gas_turbine -> nuclear (a late gate, later than coal-liq's Adv-Mfg).
            // mac1's pick; @linux2/@win adjust the exact tech if balance wants different.
            int iChain = ( 0 == iOn ) ? (int)nuclear : aiFrIdx[iOn - 1];
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
    // converts global FOOD into oil (~8 food -> 1 oil), consumed in the refinery production
    // hook. Each tier costs DOUBLE the previous and chains the prior tier; T1 is a heavy
    // multi-line gate (farming + gas turbines + some fuel efficiency + vehicle speed + ADVANCED
    // MANUFACTURING) and the cost basis is gas_turbine (not farm_1) so it lands late and costs
    // more, no energy cost. The AI's frozen research path doesn't pursue these.
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

        // Operator: BioFuel should be gated behind a HIGHER / more expensive tech. Cost basis
        // raised from farm_1 (an early ag tech) to gas_turbine so every tier costs more, and the
        // T1 entry gate now also requires ADVANCED MANUFACTURING (manf_3) -- the same high gate
        // coal-liquefaction sits behind -- so Biomass Digestion can't be reached early.
        int iPts = ElementAt( gas_turbine ).m_iPtsRequired;
        for ( int iOn = 0; iOn < 6; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( biofuel_1 + iOn );

            iPts *= 2;
            pRi->m_iPtsRequired       = iPts;
            pRi->m_iScenarioReq       = ElementAt( farm_1 ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired  = 0;

            if ( 0 == iOn )
            {
                // Heavy multi-line entry gate + manf_3 (advanced manufacturing) so it lands late.
                static const int aiBf1[5] = {
                    (int)farm_1, (int)gas_turbine, (int)fuel_efficiency_2, (int)vehicle_speed_2, (int)manf_3 };
                pRi->m_iNumRsrchRequired  = 5;
                pRi->m_piRsrchRequired    = new int[5];
                for ( int k = 0; k < 5; k++ ) pRi->m_piRsrchRequired[k] = aiBf1[k];
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

    // In-code research topics: Coal Liquefaction (2 tiers, not in the DAT file). A coal
    // POWER PLANT, once tier 1 is researched and its per-building alt-output toggle is ON,
    // also converts coal into oil via the shared AltOutput system (eRatioConsume). Tier 1
    // runs the recipe at 3 coal -> 1 oil; tier 2 improves it to 2 coal -> 1 oil (the ratio
    // is read per-tier by CPlayer::GetCoalLiqRatio and wired into the def's m_pfnRatioIn).
    // Tier 1 chained off Advanced Manufacturing. Tier 2 is a deliberate MEGA-EXPENSIVE
    // endgame tech: a flat 2,000,000-point cost, gated behind fuel_efficiency_5 (so a few
    // fuel-efficiency techs are researched first) as well as tier 1. Appended LAST in the
    // enum so save indices don't shift.
    {
        CRsrchItem* pRi = &ElementAt( coal_liquefaction );

        pRi->m_iPtsRequired      = ElementAt( gas_turbine ).m_iPtsRequired * 4;
        pRi->m_iScenarioReq      = ElementAt( gas_turbine ).m_iScenarioReq;
        pRi->m_iNumBldgsRequired = 0;

        pRi->m_iNumRsrchRequired = 1;
        pRi->m_piRsrchRequired   = new int[1];
        // #28 (operator Note 23): gate Coal Liquefaction behind ADVANCED MANUFACTURING
        // (manf_3) since it appeared too early off gas_turbine. (Cost basis left as-is;
        // operator retunes in-game.)
        pRi->m_piRsrchRequired[0] = (int)manf_3;

        pRi->m_sName   = "Coal Liquefaction";
        pRi->m_sDesc   = "Fischer-Tropsch synthesis cracks coal into liquid fuel: a toggled coal power plant turns 3 coal into 1 oil.";
        pRi->m_sResult = "Coal liquefaction online. Coal power plants can convert coal to oil (toggle per plant).";

        // Tier 2: better catalysts wring more oil from the same coal (3:1 -> 2:1). Priced as a
        // super-expensive endgame prize (2 million points) and gated behind a chunk of the fuel-
        // efficiency line, not just tier 1. Both values are trivially retunable here.
        CRsrchItem* pRi2 = &ElementAt( coal_liquefaction_2 );

        pRi2->m_iPtsRequired      = 2000000;   // millions of points: a very late, very costly tech
        pRi2->m_iScenarioReq      = ElementAt( gas_turbine ).m_iScenarioReq;
        pRi2->m_iNumBldgsRequired = 0;

        // Requires Coal Liquefaction (tier 1) AND fuel_efficiency_5 (which chains 1-5, so a few
        // fuel-efficiency techs are already done before this unlocks).
        pRi2->m_iNumRsrchRequired = 2;
        pRi2->m_piRsrchRequired   = new int[2];
        pRi2->m_piRsrchRequired[0] = (int)coal_liquefaction;
        pRi2->m_piRsrchRequired[1] = (int)fuel_efficiency_5;

        pRi2->m_sName   = "Catalytic Coal Cracking";
        pRi2->m_sDesc   = "A better catalyst bed should let our plants squeeze the same oil from less coal, dropping the recipe to 2 coal for 1 oil. It will take a fortune in research to perfect.";
        pRi2->m_sResult = "Catalytic cracking is dialed in. Our coal plants now make 1 oil from just 2 coal instead of 3.";
    }

    // In-code research topic: Charcoal (5 tiers, not in the DAT file). A lumber MILL (the
    // sawmill -- UTfarm whose GetTypeFarm() == lumber), once a Charcoal tier is researched
    // and its per-building alt-output toggle is ON, runs a kiln: it converts harvested
    // lumber into coal ("Charcoal" label only) at a fixed 2 lumber -> 1 coal via the shared
    // AltOutput system (eRatioConsume), MODE-SWITCH (lumber output stops while the kiln
    // runs). The 2:1 ratio is fixed; the THROUGHPUT is tier-scaled by CPlayer::GetCharcoalPct
    // (T1 = VERY LOW per operator spec, T2-5 raise it). No energy cost. T1 chained off Gas
    // Turbines; T2-4 chain the prior tier (mirrors the BioFuel line). Cost doubles each tier.
    // Appended LAST in the enum so save indices don't shift. The AI's frozen research path
    // doesn't pursue these.
    {
        static const char* aszChName[5] = {
            "Charcoal Kiln", "Retort Kiln", "Continuous Carbonization", "Pyrolysis Refinery",
            "Fluidized-Bed Reactor" };
        static const char* aszChDesc[5] = {
            "A simple wood kiln chars lumber into coal: a toggled sawmill converts 2 lumber into 1 coal at a very low rate.",
            "Sealed retort kilns char lumber more efficiently, raising the sawmill's charcoal output.",
            "Continuous carbonization lines keep the kiln running, raising charcoal output again.",
            "A full pyrolysis refinery wrings still more charcoal from every log.",
            "A fluidized-bed reactor chars every scrap at once, squeezing the most coal yet from each log." };
        static const char* aszChRslt[5] = {
            "Charcoal kiln online. Sawmills can convert lumber into coal (toggle per mill).",
            "Retort kilns fielded. Sawmill charcoal output rises.",
            "Continuous carbonization in service. More charcoal per log.",
            "Pyrolysis refinery fielded. Charcoal output climbs again.",
            "Fluidized-bed reactor perfected. Maximum charcoal from every sawmill." };

        // Level (0-based) -> enum id. Tiers 1-4 are contiguous; tier 5 was appended at the
        // enum end for save parity, so it is NOT charcoal_4+1 -- map it explicitly.
        static const int aiChIdx[5] = {
            charcoal_1, charcoal_2, charcoal_3, charcoal_4, charcoal_5 };

        int iPts = ElementAt( gas_turbine ).m_iPtsRequired;
        for ( int iOn = 0; iOn < 5; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( aiChIdx[iOn] );

            iPts *= 2;
            pRi->m_iPtsRequired       = iPts;
            pRi->m_iScenarioReq       = ElementAt( gas_turbine ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired  = 0;

            pRi->m_iNumRsrchRequired  = 1;
            pRi->m_piRsrchRequired    = new int[1];
            pRi->m_piRsrchRequired[0] = ( 0 == iOn ) ? (int)gas_turbine : aiChIdx[iOn - 1];

            pRi->m_sName   = aszChName[iOn];
            pRi->m_sDesc   = aszChDesc[iOn];
            pRi->m_sResult = aszChRslt[iOn];
        }
    }

    // ---- Research Speed 1-5 (in-code) ----------------------------------------
    // Each level makes RESEARCH ITSELF 10% faster: the points earned per tick go
    // 100% -> 110% -> 120% -> 130% -> 140% -> 150% (CPlayer::GetRsrchSpeedPct,
    // consumed in CPlayer::Research). Costs are ABSOLUTE (not a multiple of a DAT
    // topic, the way the fuel/charcoal lines are): 100k doubling to 1.6M. That puts
    // the whole line between acc_3 (320k, priciest DAT topic) and coal_liquefaction_2
    // (2M), so unlike the late Fuel Efficiency tiers these stay reachable in a normal
    // game. T1 has no precursor topic: it unlocks once the colony has completed
    // RSRCH_SPEED_MIN_TOPICS paid topics (gate in CPlayer::CanRsrch), so it is earned
    // by research experience rather than by one arbitrary tech. T2-5 chain off the
    // previous tier. The scenario gate is still borrowed from telephone.
    //
    // Text follows the ORIGINAL DAT voice, measured off the "New/Improved/Adv.
    // Construction Techniques" productivity line (the game's own analogue of this
    // one): no "%" anywhere (the original never uses it), no semicolons or dashes,
    // single space after a period, "However," / "In fact," as the connectors, and the
    // a little -> significantly -> a lot -> much -> drastically ladder (every rung of
    // which is attested in the original text). It calls the building a "Research
    // Institute", which is the name the original uses -- never "laboratory".
    // NOTE: no level's text may claim to be the "last" or "final" one -- the fuel
    // line was extended four times and every such claim had to be walked back.
    {
        static const char* aszRsName[5] = {
            "New Research Methods",
            "Precision Instruments",
            "Scientific Computing",
            "Automated Test Benches",
            "Parallel Research Teams" };
        static const char* aszRsDesc[5] = {
            "If we spent some time learning how to run our Research Institutes more efficiently instead of just working harder it could pay off for us in increased productivity.",
            "Our Research Institutes are still working with the instruments we brought with us. We should be able to build proper ones here and learn more from every experiment.",
            "Our researchers spend more of their time working through their figures than running experiments. If we put our computers on the numbers they could get on with the science.",
            "Our test benches stand idle every night when the shift goes home. We should be able to run them unattended and keep our experiments going around the clock.",
            "We have our researchers all working the same problem one at a time. If we split them into teams working in parallel we could chase several answers at once." };
        static const char* aszRsRslt[5] = {
            "With what we have learned we should be able to complete our research a little faster. However, there is quite a bit left to learn.",
            "We have the new instruments in place. Our researchers get a clean reading the first time and we complete our research significantly faster. However, there is still a good bit left to learn.",
            "We can now put our computers on the numbers. Our researchers are free of their figures and we complete our research a lot faster. However, there is a bit left to learn.",
            "We have the test benches running unattended. The work doesn't stop when the shift goes home and we complete our research much faster. However, there is not much left to learn.",
            "We are now running our researchers in parallel teams. Chasing several answers at once has drastically increased the pace of our research. In fact, our Research Institutes now spend more time writing up results than getting them." };

        // Absolute costs, doubling: 100k / 200k / 400k / 800k / 1.6M.
        static const int aiRsPts[5] = { 100000, 200000, 400000, 800000, 1600000 };

        // Contiguous at the enum end, but map explicitly so a future append cannot
        // silently shift the line (the fuel line learned this the hard way).
        static const int aiRsIdx[5] = {
            rsrch_speed_1, rsrch_speed_2, rsrch_speed_3, rsrch_speed_4, rsrch_speed_5 };

        for ( int iOn = 0; iOn < 5; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( aiRsIdx[iOn] );

            pRi->m_iPtsRequired       = aiRsPts[iOn];
            pRi->m_iScenarioReq       = ElementAt( telephone ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired  = 0;

            // T1 has NO precursor topic: its gate is the RSRCH_SPEED_MIN_TOPICS count
            // check in CPlayer::CanRsrch. Leaving the count at 0 keeps m_piRsrchRequired
            // NULL (set by the ctor), which is what CRsrchArray::Close expects to delete.
            // T2-5 chain off the previous tier as usual.
            if ( 0 == iOn )
                pRi->m_iNumRsrchRequired = 0;
            else
            {
                pRi->m_iNumRsrchRequired  = 1;
                pRi->m_piRsrchRequired    = new int[1];
                pRi->m_piRsrchRequired[0] = aiRsIdx[iOn - 1];
            }

            pRi->m_sName   = aszRsName[iOn];
            pRi->m_sDesc   = aszRsDesc[iOn];
            pRi->m_sResult = aszRsRslt[iOn];
        }
    }


    // ---- Moho Mining upgrade line 2-6 (in-code) ------------------------------
    // The IRON twin of Fracking: an EXHAUSTED iron mine trickles iron when the per-
    // building toggle is on (AltOutput "Moho Mining", +50% mine power). Until now this
    // was a single flat 10 iron/min granted free by the DAT mine_2 topic and had no
    // upgrade path at all -- these five tiers are that path. mine_2 stays the free base
    // tier (still 10/min, NOT repriced, so nobody loses a capability they already have)
    // and each paid tier adds +1 iron/min, reaching 15 at moho_6.
    //
    // Cost doubles per tier off mine_2 as the basis (296k .. 4.736M), mirroring how the
    // Fracking line doubles off gas_turbine. Gating is deliberately NOT the Fuel
    // Efficiency line that Fracking leans on -- deep rock is a CONSTRUCTION problem, so
    // the cross-line prereqs walk the const_1..3 ladder, with nuclear up front for the
    // power a Moho bore needs. Rates/gates are easy to retune (operator balances in-game).
    {
        // The BASE Moho capability rides on the DAT mine_2 topic, whose own result text talks
        // only about mining faster -- so nothing ever told the player the toggle exists and it
        // was found by accident. Append one sentence announcing it. Safe to do here: Open()
        // asserts GetSize()==0 on entry, so the DAT text is re-read fresh every load and this
        // cannot append twice.
        ElementAt( mine_2 ).m_sResult += " We can also work an exhausted iron mine again by boring past the crust.";

        static const char* aszMoName[5] = {
            "Mantle Boreholes", "Diamond Drill Strings", "Magma-Assisted Smelting",
            "Seismic Ore Imaging", "Continuous Deep Extraction" };
        static const char* aszMoDesc[5] = {
            "Our revived mines only ever scratched the crust. If we bore down past it we should be able to reach ore the shafts never touched and take 11 units a minute.",
            "Our drill strings blunt themselves on mantle rock and don't last a week before re-tipping. Diamond strings should keep them cutting for 12.",
            "There is heat enough at the bore face to work the ore where it lies. We should be able to smelt it down there and bring up 13.",
            "We are boring into whatever lies beneath us and hoping. However, seismic imaging should steer us into the better ore bodies for 14.",
            "Our bores stop every time the cutting head comes up. A continuous head should keep them advancing around the clock for 15." };
        static const char* aszMoRslt[5] = {
            "We can now bore past the crust. Our exhausted iron mines give up 11 units of iron a minute.",
            "The diamond strings are in service. Our bores cut mantle rock for days without stopping and the mines give 12.",
            "We can now smelt the ore at the bore face. The heat down there does the work for us and the mines give 13.",
            "We have seismic ore imaging working. Our bores find the ore instead of hunting for it and the mines give 14.",
            "We are now cutting around the clock. The deep bores don't stop any more and the mines give 15." };

        // Extra (cross-line) prereq per tier, on top of the previous tier. -1 = none.
        static const int aiMoExtra[5] = {
            (int)nuclear, (int)const_1, (int)const_2, (int)const_3, (int)advanced_facilities };

        // Level (0-based) -> enum id. Contiguous at the enum end, but mapped explicitly
        // so a future append cannot silently shift the line.
        static const int aiMoIdx[5] = { moho_2, moho_3, moho_4, moho_5, moho_6 };

        int iPts = ElementAt( mine_2 ).m_iPtsRequired;   // basis: the topic that grants base Moho
        for ( int iOn = 0; iOn < 5; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( aiMoIdx[iOn] );

            iPts *= 2;
            pRi->m_iPtsRequired       = iPts;
            pRi->m_iScenarioReq       = ElementAt( mine_2 ).m_iScenarioReq;
            pRi->m_iNumBldgsRequired  = 0;

            // T1 of this line chains off mine_2 itself (the free base tier); T2-5 off the
            // previous tier. Every tier also carries one cross-line prereq.
            int iChain = ( 0 == iOn ) ? (int)mine_2 : aiMoIdx[iOn - 1];
            int nReq   = 1 + ( aiMoExtra[iOn] >= 0 ? 1 : 0 );
            pRi->m_iNumRsrchRequired  = nReq;
            pRi->m_piRsrchRequired    = new int[nReq];
            pRi->m_piRsrchRequired[0] = iChain;
            if ( aiMoExtra[iOn] >= 0 )
                pRi->m_piRsrchRequired[1] = aiMoExtra[iOn];

            pRi->m_sName   = aszMoName[iOn];
            pRi->m_sDesc   = aszMoDesc[iOn];
            pRi->m_sResult = aszMoRslt[iOn];
        }
    }
    // Late-game combat/structure tier (in-code): one more Range level, one more Attack
    // level, and the first Building Armor topic. All three are END-GAME purchases -- each
    // costs 8x its 248,000-point DAT parent (1,984,000, ~6x the dearest DAT topic) and each
    // is gated behind the top of two or three other lines, so none of them can be reached
    // before the rest of the tree is well along. The AI's frozen research path doesn't
    // pursue them (optional human tiers). Effects: range_4/atk_4 in CUnit::AssignData via
    // CPlayer::SetRsrch (m_bRange / m_bAttack level 4, diminishing step); bldg_armor in
    // CUnit::DecDamagePoints via CPlayer::GetBldgArmorMult.
    {
        static const int aiLtIdx[3]  = { range_4, atk_4, bldg_armor };

        // Points basis per topic (all three DAT parents cost 248,000) and the multiplier.
        static const int aiLtBasis[3] = { (int)range_3, (int)atk_3, (int)const_3 };
        static const int LT_COST_MULT = 8;

        // Prereqs per topic, -1 padded. Every one of these is a top-of-line DAT topic.
        static const int aiLtReq[3][3] = {
            { (int)range_3, (int)acc_3,   -1                        },
            { (int)atk_3,   (int)manf_3,  (int)nuclear              },
            { (int)fortification, (int)const_3, (int)advanced_facilities } };

        static const char* aszLtName[3] = {
            "Base-Bleed Shells", "Tandem Warheads", "Blast Shielding" };
        static const char* aszLtDesc[3] = {
            "We think we can fit a small gas generator into the base of a shell to fill in the drag behind it. That should buy our guns a little more distance than the turbo ignitors alone.",
            "If we set a second charge behind the first our shells should defeat armor that stops a single warhead. It should give our units a little more punch.",
            "With some work we should be able to hang spaced plate on our buildings so a shell breaks up before it reaches the wall behind it. Our structures would take less damage from enemy fire." };
        static const char* aszLtRslt[3] = {
            "The base-bleed shells carry a little further than anything we have fired so far. We will stock all new units with them.",
            "The tandem warheads give our units a little more offensive strength. We are loading these shells on all new units.",
            "The blast shielding is up. Every building we own takes significantly less damage from enemy fire, not just the ones we build from here on." };

        for ( int iOn = 0; iOn < 3; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( aiLtIdx[iOn] );

            pRi->m_iPtsRequired      = ElementAt( aiLtBasis[iOn] ).m_iPtsRequired * LT_COST_MULT;
            pRi->m_iNumBldgsRequired = 0;

            // Count the real prereqs, and take the LATEST scenario any of them needs -- the
            // topic cannot be started before every gate is itself reachable.
            int nReq  = 0;
            int iScen = 0;
            for ( int iReq = 0; iReq < 3; iReq++ )
                if ( aiLtReq[iOn][iReq] >= 0 )
                {
                    nReq++;
                    if ( ElementAt( aiLtReq[iOn][iReq] ).m_iScenarioReq > iScen )
                        iScen = ElementAt( aiLtReq[iOn][iReq] ).m_iScenarioReq;
                }
            pRi->m_iScenarioReq      = iScen;
            pRi->m_iNumRsrchRequired = nReq;
            pRi->m_piRsrchRequired   = new int[nReq];
            for ( int iReq = 0, iPut = 0; iReq < 3; iReq++ )
                if ( aiLtReq[iOn][iReq] >= 0 )
                    pRi->m_piRsrchRequired[iPut++] = aiLtReq[iOn][iReq];

            pRi->m_sName   = aszLtName[iOn];
            pRi->m_sDesc   = aszLtDesc[iOn];
            pRi->m_sResult = aszLtRslt[iOn];
        }
    }
    // Nuclear Uprate 1-5 (in-code): each level adds 10 percent to the output of this
    // player's Nuclear Power Plants. Cost starts at 2x the DAT nuclear topic (296,000) and
    // DOUBLES per level to 4,736,000. T1 chains nuclear itself; T2-5 chain the previous tier,
    // and every tier carries one cross-line manufacturing/construction gate, because a
    // reactor uprate is a materials problem before it is a physics one. The AI's frozen
    // research path doesn't pursue these (optional human tiers).
    {
        static const int aiNkIdx[5] = { nuke_power_1, nuke_power_2, nuke_power_3,
                                        nuke_power_4, nuke_power_5 };
        static const int aiNkExtra[5] = {
            (int)advanced_facilities, (int)manf_1, (int)manf_2, (int)manf_3, (int)const_3 };

        static const char* aszNkName[5] = {
            "Reactor Uprate", "Improved Control Rods", "Breeder Cycle",
            "Fast Neutron Core", "Closed Fuel Cycle" };
        static const char* aszNkDesc[5] = {
            "Our reactors are run well inside their margins because we did not trust the first cores we cast. Now that we do, we should be able to open them up and draw more power from every plant.",
            "The control rods we started with are crude and we hold the pile back to stay safe. Finer rods would let us run the reactors hotter without losing the margin.",
            "The spent fuel we pull out still has most of its energy in it. If we breed it back into fuel we can keep the piles running harder for longer.",
            "A fast neutron core burns the heavier waste our thermal piles leave behind. It is harder to hold steady but there is a lot more power in it.",
            "If we close the fuel cycle nothing leaves the plant but electricity. Everything we dig up gets burned, and the piles run harder than we built them to." };
        static const char* aszNkRslt[5] = {
            "The uprate is done. Every nuclear plant we own puts out more power than it did, and the ones we build from here on start that way.",
            "The new control rods are in. Our reactors run hotter and give us more power for the same fuel.",
            "The breeder cycle is running. We are making fuel faster than we burn it and our nuclear plants give more power again.",
            "The fast neutron cores are online. They burn what the old piles threw away and our nuclear plants give more power still.",
            "The fuel cycle is closed. Nothing leaves our nuclear plants but electricity, and they give more power than we thought those piles had in them." };

        int iPts = ElementAt( nuclear ).m_iPtsRequired;   // basis: the topic that unlocks the plant
        for ( int iOn = 0; iOn < 5; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( aiNkIdx[iOn] );

            iPts *= 2;                                    // 296,000 .. 4,736,000
            pRi->m_iPtsRequired      = iPts;
            pRi->m_iNumBldgsRequired = 0;

            int iChain = ( 0 == iOn ) ? (int)nuclear : aiNkIdx[iOn - 1];
            pRi->m_iNumRsrchRequired  = 2;
            pRi->m_piRsrchRequired    = new int[2];
            pRi->m_piRsrchRequired[0] = iChain;
            pRi->m_piRsrchRequired[1] = aiNkExtra[iOn];

            int iScen = ElementAt( iChain ).m_iScenarioReq;
            if ( ElementAt( aiNkExtra[iOn] ).m_iScenarioReq > iScen )
                iScen = ElementAt( aiNkExtra[iOn] ).m_iScenarioReq;
            pRi->m_iScenarioReq = iScen;

            pRi->m_sName   = aszNkName[iOn];
            pRi->m_sDesc   = aszNkDesc[iOn];
            pRi->m_sResult = aszNkRslt[iOn];
        }
    }
    // Late attack tiers 5-8 and Building Armor tiers 2-3 (in-code). These are the deep end of
    // the tree: every one carries THREE prerequisites that reach across other lines (accuracy,
    // range, defense, construction, manufacturing, spotting and the Nuclear Uprate line above),
    // so they cannot be opened early no matter how the points are spent. Because the benefit
    // per tier is shrinking, the cost ramp is FLAT rather than doubling -- each tier adds
    // another 8x the 248,000-point basis, exactly the way Fuel Efficiency behaves past its cap.
    // Effects: attack in CUnit::AssignData (via CPlayer::SetRsrch, m_bAttack levels 5-8);
    // building armor in CUnit::DecDamagePoints (via CPlayer::GetBldgArmorMult).
    // NOTE bldg_armor_3 requires atk_5: we learn what stops a tandem warhead by building one.
    // That is a one-way link (no attack tier requires a building-armor tier), so no cycle.
    {
        static const int aiDpIdx[6]   = { atk_5, atk_6, atk_7, atk_8, bldg_armor_2, bldg_armor_3 };
        static const int aiDpBasis[6] = { (int)atk_3, (int)atk_3, (int)atk_3, (int)atk_3,
                                          (int)const_3, (int)const_3 };
        // Flat cost ramp, in multiples of the 248,000 basis: the attack line continues from
        // atk_4 (8x) and the armor line continues from bldg_armor (8x).
        static const int aiDpMult[6]  = { 16, 24, 32, 40, 16, 24 };

        static const int aiDpReq[6][3] = {
            { (int)atk_4,        (int)acc_3,        (int)advanced_facilities },
            { (int)atk_5,        (int)range_4,      (int)def_3               },
            { (int)atk_6,        (int)nuke_power_2, (int)const_3             },
            { (int)atk_7,        (int)nuke_power_4, (int)spot_5              },
            { (int)bldg_armor,   (int)def_3,        (int)manf_3              },
            { (int)bldg_armor_2, (int)nuke_power_3, (int)atk_5               } };

        static const char* aszDpName[6] = {
            "Shaped Liners", "Kinetic Penetrators", "Thermobaric Cores", "Guided Shells",
            "Spall Liners", "Reactive Facing" };
        static const char* aszDpDesc[6] = {
            "A copper liner pressed into the right cone turns the charge into a jet instead of a blast. It is a small gain on top of the tandem rounds but it is a real one.",
            "A dense dart carries its energy further into the plate than any explosive we pack behind it. We would be trading blast for penetration and coming out a little ahead.",
            "A thermobaric core keeps burning after the case opens instead of spending itself at once. The gain is getting small now, but our gunners will take it.",
            "We can put the radar sights on the shell itself and let it correct on the way in. There is not much left to win in a shell this size, but a round that steers itself will take what there is.",
            "Shells that fail to hole a wall still knock plate off the inside of it, and that is what hurts the people working in there. A liner catches the fragments.",
            "Plate that fires back into the jet as it forms will blunt a shaped charge before it reaches the wall. It is heavy and awkward and we would only do it now." };
        static const char* aszDpRslt[6] = {
            "The shaped liners are in production. Our shells hit a little harder than the tandem rounds did. We are loading them on all new units.",
            "The kinetic penetrators work. Our units hit a little harder again, and against armor rather better than the numbers suggest. All new units carry them.",
            "The thermobaric cores are loaded. The gain is small now but our shells do hit harder. All new units are stocked with them.",
            "The guided shells are in service. They correct themselves on the way in and our units hit a little harder for it. We are loading them on all new units.",
            "The spall liners are fitted. Every building we own takes less damage again, and the crews inside come through a shelling in better shape.",
            "The reactive facing is up on every building we own. Our structures take less damage still, and a shaped charge does very little to them now." };

        for ( int iOn = 0; iOn < 6; iOn++ )
        {
            CRsrchItem* pRi = &ElementAt( aiDpIdx[iOn] );

            pRi->m_iPtsRequired      = ElementAt( aiDpBasis[iOn] ).m_iPtsRequired * aiDpMult[iOn];
            pRi->m_iNumBldgsRequired = 0;

            // All three prereq slots are real here (no -1 padding), but take the LATEST
            // scenario any of them needs all the same.
            int iScen = 0;
            pRi->m_iNumRsrchRequired = 3;
            pRi->m_piRsrchRequired   = new int[3];
            for ( int iReq = 0; iReq < 3; iReq++ )
            {
                pRi->m_piRsrchRequired[iReq] = aiDpReq[iOn][iReq];
                if ( ElementAt( aiDpReq[iOn][iReq] ).m_iScenarioReq > iScen )
                    iScen = ElementAt( aiDpReq[iOn][iReq] ).m_iScenarioReq;
            }
            pRi->m_iScenarioReq = iScen;

            pRi->m_sName   = aszDpName[iOn];
            pRi->m_sDesc   = aszDpDesc[iOn];
            pRi->m_sResult = aszDpRslt[iOn];
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

