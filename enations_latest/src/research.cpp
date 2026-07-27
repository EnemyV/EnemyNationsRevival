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

    // In-code research topics: the Fuel Efficiency line (not in the DAT file). An 18-level
    // line unlocked after Gas Turbines; level 1 requires gas_turbine, each later the prev.
    // Cost DOUBLES to 32*B at level 6, then flat +16*B (L7=48B ... L18=224B; B = gas_turbine
    // cost). Gas saving diminishes 5/4/4/3/3/3/2/2/2/2 to 30% at level 10, then +1% per level
    // to 38% at level 18 (see CPlayer::GetFuelPct). The late levels are deliberately mundane
    // garage tweaks (additives, thinner oil, cleaner filters), not sci-fi. Levels 1-10 are
    // contiguous; 11-12, 13-16, and 17-18 were appended at the enum end for save parity, so
    // the setup loop maps the index through aiIdx[] rather than a running offset.
    {
        static char const* aszName[18] = {
            "Fuel Injection",      "Lean-Burn Tuning",    "Turbo Compounding",
            "Regenerative Braking","Waste-Heat Recovery", "Better Spark Timing",
            "Exhaust Reclamation", "Synthetic Lubricants","Low-Friction Bearings",
            "Reduced Rolling Resistance","Fuel Additives", "Tighter Tolerances",
            "Low-Viscosity Oil",   "Cleaner Fuel Filters","Lightweight Flywheels",
            "Idle Cutoff",         "Coasting Governor",   "Fuel Preheating" };
        static char const* aszDesc[18] = {
            "Our engines still gulp fuel through crude carburetors. We should be able to meter each drop with proper injection and burn a good 5% less gas.",
            "We think we can tune the engines to run leaner, coaxing more travel out of every tank for another 4%.",
            "All that hot exhaust just blows away. If we feed it back through a turbine we should recover another 4% of the fuel.",
            "Every time a vehicle slows down we throw away good energy as heat. We should be able to catch some of it back, worth about 3%.",
            "Our engines run hot enough to cook dinner on. We think we can scavenge that waste heat for a further 3%.",
            "Our ignition timing is a guess at best. Dialing in the spark should burn the charge more completely for another 3%.",
            "There is still unburned fuel going out the tailpipe. We should be able to catch and re-burn it for another 2%.",
            "The local oils gum up in this climate. A proper synthetic lubricant should cut friction across the drivetrain for 2%.",
            "Our bearings are rougher than we would like. Polishing them to a low-friction finish should be good for another 2%.",
            "Our wheels and tracks fight the ground the whole way. Trimming that rolling resistance should save a final 2%, a full 30% by now.",
            "The local crude is full of grit. A dose of the right additives should keep our engines from gumming up and save another 1%.",
            "If our machinists shave the tolerances a little finer, the engines will leak a bit less power, worth about 1%.",
            "A thinner oil would let everything spin easier once the engine warms up. We think that is good for another 1%.",
            "Half the dirt on this planet ends up in our fuel lines. Finer filters should keep the injectors happy for another 1%.",
            "Our flywheels are heavier than they need to be. Shaving them down should free up about 1%.",
            "Vehicles sitting idle just drink fuel for nothing. A cutoff that stops the engine when they wait should save 1%.",
            "On a downhill our engines keep pulling when they could just coast. A governor to ease off should be worth a last 1%.",
            "Cold fuel burns poorly in this thin air. Warming it before it hits the cylinder should wring out one final 1%." };
        static char const* aszRslt[18] = {
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
            "Fuel preheating is running. Even in the cold our engines burn every drop, the last 1% we are going to get (38% total)." };

        // Extra (cross-line) prereq per level, on top of the previous level. -1 = none.
        // Turbo compounding leans on manufacturing; better spark timing needs nuclear-era
        // physics. Levels above each gate inherit it through the chain, so we only pin it
        // once where it first becomes necessary. The late mundane levels add no new gate.
        static const int aiExtra[18] = {
            -1, -1, (int)manf_1, -1, -1, (int)nuclear, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

        // Level (0-based) -> enum id. Non-contiguous because 11-12, 13-16 and 17-18 were
        // appended at the enum end for save parity.
        static const int aiIdx[18] = {
            fuel_efficiency_1,  fuel_efficiency_2,  fuel_efficiency_3,  fuel_efficiency_4,  fuel_efficiency_5,
            fuel_efficiency_6,  fuel_efficiency_7,  fuel_efficiency_8,  fuel_efficiency_9,  fuel_efficiency_10,
            fuel_efficiency_11, fuel_efficiency_12, fuel_efficiency_13, fuel_efficiency_14,
            fuel_efficiency_15, fuel_efficiency_16, fuel_efficiency_17, fuel_efficiency_18 };

        int iBase = ElementAt( gas_turbine ).m_iPtsRequired;   // B = gas_turbine cost
        int iPts  = iBase;                                     // level 1 = B
        for ( int iOn = 0; iOn < 18; iOn++ )
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

    // In-code research topics: Fracking 1-6 (#23, not in the DAT file). Exhausted oil
    // wells trickle oil when fracking is toggled ON (consumed in the mine production hook
    // via CPlayer::GetFrackOilPerMin), at +50% well energy. Each tier costs DOUBLE the
    // previous and chains the prior tier; T1 needs gas_turbine, later tiers also a
    // Fuel-Efficiency level. The AI's frozen research path doesn't pursue these (optional
    // human tiers). Point/gate values are easy to retune (operator balances in-game).
    {
        static const char* aszFrName[6] = {
            "Hydraulic Fracturing", "Horizontal Drilling", "Proppant Injection",
            "Microseismic Mapping", "Supercritical Extraction", "Thermal Flooding" };
        static const char* aszFrDesc[6] = {
            "High-pressure fluid fractures spent rock, coaxing a 5/min oil trickle from exhausted wells.",
            "Horizontal bores reach untapped pockets, lifting the trickle to 7/min.",
            "Engineered proppants hold fractures open longer, raising recovery to 9/min.",
            "Microseismic mapping targets the richest seams, yielding 11/min.",
            "Supercritical solvents strip the last bound oil from dead rock, 13/min.",
            "Pumped steam drives the last clinging oil out of dead rock, lifting the trickle to 15/min." };
        static const char* aszFrRslt[6] = {
            "Hydraulic fracturing online. Exhausted wells now trickle oil (toggle per well).",
            "Horizontal drilling fielded. Fracked wells yield more oil.",
            "Proppant injection in service. Fracked-well oil rises again.",
            "Microseismic mapping operational. Fracked wells reach deeper pockets.",
            "Supercritical extraction perfected. Maximum oil from spent wells.",
            "Thermal flooding operational. Even the most spent wells give up a little more oil." };
        // Extra (cross-line) prereq per tier, on top of the previous tier. -1 = none.
        static const int aiFrExtra[6] = {
            -1, (int)fuel_efficiency_1, (int)fuel_efficiency_3, (int)fuel_efficiency_5, (int)fuel_efficiency_8, (int)fuel_efficiency_10 };

        // Level (0-based) -> enum id. Tiers 1-5 are contiguous; tier 6 was appended at the
        // enum end for save parity, so it is NOT fracking_5+1 -- map it explicitly.
        static const int aiFrIdx[6] = {
            fracking_1, fracking_2, fracking_3, fracking_4, fracking_5, fracking_6 };

        int iPts = ElementAt( gas_turbine ).m_iPtsRequired;
        for ( int iOn = 0; iOn < 6; iOn++ )
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

