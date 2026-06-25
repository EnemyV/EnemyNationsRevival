//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// player.cpp
//

#include <algorithm> // for the memory pool
#include "player.h"
#include "edicts.h"   // Edicts v1: g_aEdicts catalog for RecomputeEdictMults

#include "ai.h"
#include "Perf.h"  // ai.msg.skip counter (fan-out filter)
#include "area.h"
#include "bridge.h"
#include "GameWindow.h"
#include "en_harness.h"   // HarnessPendingLoadPath (headless load skips the browser)
#include "SDL2MFCPanel.h"
#include "SDL2FileBrowser.h"
#include "building.inl"
#include "CdLoc.h"
#include "chproute.hpp"
#include "codec.h"
#include "cpathmgr.h"
#include "error.h"
#include "event.h"
#include "help.h"
#include "lastplnt.h"
#include "minerals.h"
#include "netapi.h"
#include "relation.h"
#include "research.h"
#include "SaveCompat.h"
#include "SDL2CreateStatus.h"
#include "stdafx.h"
#include "terrain.inl"
#include "unit.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

extern CRITICAL_SECTION cs;

CGame theGame;

void ShowBuilding( int iInd, CBuilding* pBldg );


/////////////////////////////////////////////////////////////////////////////
// CPlayer - a player in the game

void CPlayer::_ctor( )
{

    ctor( );

    m_bAI          = FALSE;
    m_bLocal       = FALSE;
    m_bMe          = FALSE;
    m_piBldgExists = NULL;

    ai.dwHdl = 0;
    ai.hex = m_hexMapStart = CHexCoord( 8, 8 );
    m_iPerInit             = -1;
}

void CPlayer::ctor( )
{

    m_iPerInit = -1;
    m_bState   = created;

    m_rgbPlyr = RGB( 0, 0, 0 );

    m_iNetNum         = VP_LOCALMACHINE;
    m_iPlyrNum        = 0;
    m_iFood           = 0;
    m_iFoodNeed       = 0;
    m_iGas            = 0;
    m_iGasUsed        = 0;
    m_iGasNeed        = 0;
    m_iGasTurn        = 0;
    m_iPwrNeed        = 1;
    m_iPwrHave        = 0;
    m_iPplNeedBldg    = 1;
    m_iPplBldg        = 0;
    m_iPplVeh         = 0;
    m_fPplMult        = 1.0;
    m_fPwrMult        = 1.0;
    m_fConstProd      = 1.0;
    m_fMtrlsProd      = 1.0;
    m_fManfProd       = 1.0;
    m_fMineProd       = 1.0;
    m_fFarmProd       = 1.0;
    m_fRsrchProd      = 1.0;
    m_fPopGrowth      = 0.001;
    m_fPopDeath       = 0.0005;
    m_fEatingRate     = 0.01;
    // Edicts v1 (civ-wide): no edicts active, all mults neutral (see RecomputeEdictMults).
    m_dwEdicts            = 0;
    m_fEdictConstMult     = 1.0f;
    m_fEdictMineMult      = 1.0f;
    m_fEdictRsrchMult     = 1.0f;
    m_fEdictPopGrowthMult = 1.0f;
    m_fEdictFortBuildMult = 1.0f;
    m_fEdictEnergyUpkeepPct    = 0.0f;
    m_fEdictWorkforceUpkeepPct = 0.0f;
    m_fEdictFoodUpkeepPct      = 0.0f;
    m_fAttack         = 1.0;
    m_fDefense        = 1.0;
    m_fPopMod         = 0.0;
    m_fFoodMod        = 0.0;
    m_dwAiHdl         = 0;
    m_iTheirRelations = m_iRelations = RELATIONS_NEUTRAL;
    m_iRsrchHave                     = 0;
    m_iRsrchItem                     = 0;
    m_iLastDiscovered                = 0;

    m_iBldgsBuilt = 0;
    m_iVehsBuilt  = 0;
    for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ ) m_aiMade[iInd] = m_aiHave[iInd] = 0;
    m_iBldgsDest = 0;
    m_iVehsDest  = 0;

    m_iBldgsHave = 1;
    m_iVehsHave  = 0;

    m_bSpotting = 0;
    m_bRange    = 0;
    m_bAttack   = 0;
    m_bDefense  = 0;
    m_bAccuracy = 0;

    m_aRsrch.RemoveAll( );

    m_iAptCap = m_iOfcCap = 0;
    m_iNumTrucks = m_iNumCranes = 0;
    m_dwIDRocket                = 0;

    // colony stat history (graphs) starts empty
    m_iHistHead = m_iHistCount = 0;
    memset( m_aHistPwrHave,  0, sizeof( m_aHistPwrHave ) );
    memset( m_aHistPwrNeed,  0, sizeof( m_aHistPwrNeed ) );
    memset( m_aHistPplTotal, 0, sizeof( m_aHistPplTotal ) );
    memset( m_aHistPplBldg,  0, sizeof( m_aHistPplBldg ) );
    memset( m_aHistAptCap,   0, sizeof( m_aHistAptCap ) );
    memset( m_aHistOfcCap,   0, sizeof( m_aHistOfcCap ) );

    m_iGameSpeed     = NUM_SPEEDS / 2;
    m_iNumDiscovered = 0;

    m_pXferToClient = NULL;

    m_bPauseMsgs = FALSE;

    m_iBuiltBldgsHave = 1;
    m_bPlacedRocket   = FALSE;

    m_iNumAiGpfs = 0;
    m_bMsgDead   = FALSE;

    ASSERT_VALID( this );
}

CPlayer::CPlayer( char const* pName, int iNetNum ): m_sName( pName )
{

    _ctor( );
    m_iNetNum = iNetNum;
    ASSERT_VALID( this );
}

CPlayer::~CPlayer( )
{

    if ( theApp.m_pCreateGame != NULL )
        theApp.m_pCreateGame->RemovePlayer( this );
    delete[] m_piBldgExists;

    // make sure we are out of all lists
    if ( &theGame != NULL )
    {
        POSITION pos = theGame.GetAi( ).Find( this, NULL );
        if ( pos != NULL )
            theGame.GetAi( ).RemoveAt( pos );
        pos = theGame.GetAll( ).Find( this, NULL );
        if ( pos != NULL )
            theGame.GetAll( ).RemoveAt( pos );
        pos = theGame.m_lstDead.Find( this, NULL );
        if ( pos != NULL )
            theGame.m_lstDead.RemoveAt( pos );
        pos = theGame.m_lstLoad.Find( this, NULL );
        if ( pos != NULL )
            theGame.m_lstLoad.RemoveAt( pos );
    }
}

//---------------------------------------------------------------------------
// Edicts v1 — civ-wide policy multipliers. RecomputeEdictMults folds the active
// CIV-WIDE edicts (g_aEdicts) into the cached mult floats, which the Get*Prod()
// accessors then apply. Call on toggle (and after load once persistence lands).
// Building-scoped edicts use the AltOutput family, so only EDICT_CIVWIDE entries
// contribute here. Mults reset to 1.0 (neutral) each recompute.
//---------------------------------------------------------------------------
void CPlayer::RecomputeEdictMults( )
{
    m_fEdictConstMult     = 1.0f;
    m_fEdictMineMult      = 1.0f;
    m_fEdictRsrchMult     = 1.0f;
    m_fEdictPopGrowthMult = 1.0f;
    m_fEdictFortBuildMult = 1.0f;
    m_fEdictEnergyUpkeepPct    = 0.0f;
    m_fEdictWorkforceUpkeepPct = 0.0f;
    m_fEdictFoodUpkeepPct      = 0.0f;

    for ( int id = 0; id < EDICT_COUNT; ++id )
    {
        if ( ( m_dwEdicts & ( 1u << id ) ) == 0 )
            continue;
        if ( g_aEdicts[id].scope != EDICT_CIVWIDE )
            continue;   // building-scoped edicts live in AltOutput, not the player bitmask
        const EdictDef& e = g_aEdicts[id];
        m_fEdictConstMult     *= e.fConstMult;
        m_fEdictMineMult      *= e.fMineMult;
        m_fEdictRsrchMult     *= e.fRsrchMult;
        m_fEdictPopGrowthMult *= e.fPopGrowthMult;
        m_fEdictFortBuildMult *= e.fFortConstMult;
        // Upkeep is additive across active edicts (a pct of the relevant per-loop demand).
        m_fEdictEnergyUpkeepPct    += e.fEnergyUpkeepPct;
        m_fEdictWorkforceUpkeepPct += e.fWorkforceUpkeepPct;
        m_fEdictFoodUpkeepPct      += e.fFoodUpkeepPct;
    }
}

void CPlayer::ToggleEdict( int edictId, bool bOn )
{
    if ( edictId < 0 || edictId >= EDICT_COUNT )
        return;
    DWORD bit = ( 1u << edictId );
    if ( bOn ) m_dwEdicts |= bit;
    else       m_dwEdicts &= ~bit;
    RecomputeEdictMults( );
}

// User-initiated edict toggle (from the building info window): apply locally now, and in a
// net game broadcast so every other client applies the same change for this player (the
// edict_toggle dispatch in netapi.cpp). Keeps a single deterministic mutation per client.
void CPlayer::ToggleEdictNet( int edictId, bool bOn )
{
    ToggleEdict( edictId, bOn );
    if ( theGame.IsNetGame( ) )
    {
        CNetEdictToggle msg( this, edictId, bOn );
        theNet.Broadcast( &msg, sizeof( msg ), TRUE );
    }
}

void CPlayer::Close( )
{

    delete[] m_piBldgExists;
    m_piBldgExists = NULL;
    delete m_pXferToClient;
    m_pXferToClient = NULL;
    ctor( );
}

void CPlayer::SetAI( BYTE bAI )
{

    ASSERT_VALID( this );
    m_bAI = bAI;

    if ( ( !m_bAI ) && ( m_bLocal ) )
        m_bMe = TRUE;
    else
        m_bMe = FALSE;
}

void CPlayer::SetLocal( BYTE bLocal )
{

    ASSERT_VALID( this );
    m_bLocal = bLocal;

    if ( ( !m_bAI ) && ( m_bLocal ) )
        m_bMe = TRUE;
    else
        m_bMe = FALSE;
}

void CPlayer::SetAiHdl( DWORD_PTR dwHdl )
{

    ASSERT_STRICT_VALID( this );
    ai.dwHdl = m_dwAiHdl = dwHdl;
}

void CPlayer::SetColor( COLORREF clr )
{

    m_rgbPlyr = clr | 0x02000000;
    m_clrPlyr = thePal.GetColorValue( m_rgbPlyr, ptrthebltformat->GetBitsPerPixel( ) );
}

const int       NUM_PLYR_COLORS           = 7;
static COLORREF plyrClrs[NUM_PLYR_COLORS] = { RGB( 142, 33, 23 ), RGB( 32, 26, 151 ), RGB( 255, 227, 36 ),
                                              RGB( 11, 215, 0 ),  RGB( 0, 152, 159 ), RGB( 195, 78, 150 ),
                                              RGB( 127, 19, 190 ) };

void CPlayer::SetPlyrNum( int iNum )
{

    ASSERT_STRICT_VALID( this );
    m_iPlyrNum = iNum;

    SetColor( plyrClrs[iNum % NUM_PLYR_COLORS] );
}

void CPlayer::StartGame( )
{

    m_iPplBldg = m_InitData.GetSupplies( CRaceDef::people );
    m_iFood    = m_InitData.GetSupplies( CRaceDef::food );
    m_iGas     = m_InitData.GetSupplies( CRaceDef::gas );
    // help them build roads
    if ( IsAI( ) )
        m_iGas += m_iGas * theGame.m_iAi;

    m_fConstProd = m_InitData.GetRace( CRaceDef::build_bldgs );
    m_fMtrlsProd = m_InitData.GetRace( CRaceDef::manf_materials );
    m_fManfProd  = m_InitData.GetRace( CRaceDef::manf_vehicles );
    m_fMineProd  = m_InitData.GetRace( CRaceDef::mine_prod );
    m_fFarmProd  = m_InitData.GetRace( CRaceDef::farm_prod );
    m_fRsrchProd = m_InitData.GetRace( CRaceDef::research );
    m_fAttack    = m_InitData.GetRace( CRaceDef::attack );
    m_fDefense   = m_InitData.GetRace( CRaceDef::defense );

    m_fPopGrowth  = m_InitData.GetRace( CRaceDef::pop_grow ) * 0.0012;
    m_fPopDeath   = m_InitData.GetRace( CRaceDef::pop_die ) * 0.0008;
    m_fEatingRate = m_InitData.GetRace( CRaceDef::pop_eat ) * 0.01;

    m_iRsrchHave = 0;
    m_iRsrchItem = 0;
    m_iLastDiscovered = 0;

    if ( m_piBldgExists != NULL )
        delete[] m_piBldgExists;
    m_piBldgExists = new LONG[theStructures.GetNumBuildings( )];
    memset( m_piBldgExists, 0, theStructures.GetNumBuildings( ) * sizeof( LONG ) );

    m_aRsrch.RemoveAll( );
    m_aRsrch.SetSize( theRsrch.GetSize( ) );
    m_aRsrch.ElementAt( 0 ).m_bDiscovered = TRUE;

    // if no points required it's discovered (how we kill an item w/o removing it)
    for ( int iOn = 1; iOn < m_aRsrch.GetSize( ); iOn++ )
        if ( theRsrch[iOn].m_iPtsRequired <= 0 )
            m_aRsrch.ElementAt( iOn ).m_bDiscovered = TRUE;

#ifdef _CHEAT
    if ( EnGetProfileInt( "Cheat", "KnowItAll", 0 ) )
        for ( int iOn = 1; iOn < m_aRsrch.GetSize( ); iOn++ ) m_aRsrch.ElementAt( iOn ).m_bDiscovered = TRUE;
#endif

            // if shareware net game - hurt them
#ifdef BUGBUG  // needs to know which players are shareware...
    if ( ( theApp.IsShareware( ) ) && ( GetNetNum( ) != 0 ) )
    {
        m_fConstProd *= 0.8;
        m_fMtrlsProd *= 0.8;
        m_fManfProd *= 0.8;
        m_fMineProd *= 0.8;
        m_fFarmProd *= 0.8;
        m_fPopGrowth *= 0.8;
        m_fPopDeath *= 1.2;
        m_fEatingRate *= 1.2;
        m_fRsrchProd *= 0.8;
        TRAP( );
        m_fAttack *= 0.8;
        m_fDefense *= 0.8;
    }
#endif
}

void CPlayer::SetRelations( int iVal )
{

    m_iRelations = iVal;

    if ( !IsMe( ) )
        theGame.CheckAlliances( );
}

void CPlayer::SetTheirRelations( int iVal )
{

    m_iTheirRelations = iVal;

    if ( !IsMe( ) )
        theGame.CheckAlliances( );
}

void CPlayer::AddFood( int iNum )
{

    ASSERT_STRICT_VALID( this );
    m_iFood += iNum;

    m_aiMade[CMaterialTypes::food] += iNum;
}

BOOL CPlayer::BuildRoad( )
{

    // if not AI or AI & easy do this
    // in other words, for the ez ai and for humans, roads cost more gas
    if ( ( !IsAI( ) ) || ( theGame.m_iAi == 0 ) )
    {
        if ( m_iGas < GAS_PER_ROAD )
            return ( FALSE );
        m_iGas -= GAS_PER_ROAD;
        return ( TRUE );
    }

    // level 1 - take 1 gas
    if ( theGame.m_iAi == 1 )
    {
        if ( m_iGas < 1 )
            return ( FALSE );
        m_iGas--;
        return ( TRUE );
    }

    // level 2 - must have gas, but costs none
    if ( theGame.m_iAi == 2 )
    {
        if ( m_iGas < 1 )
            return ( FALSE );
        return ( TRUE );
    }

    // level 3+ - must have gas or refinery
    if ( m_iGas > 0 )
        return ( TRUE );
    TRAP( ); // why trap? because, realistically, the AI should have gas, 
    // and if it doesn't we want to know
    return GetExists( CStructureData::refinery );
}

void CPlayer::AddGas( int iNum )
{

    ASSERT_STRICT_VALID( this );
    m_iGas += iNum;

    m_aiMade[CMaterialTypes::gas] += iNum;
}

void CPlayer::StartLoop( )
{

    const int GAS_USUAGE   = 3;  // was 4
    const int MIN_GAS_NEED = 1;

    // Edict upkeep (Edicts v1): active civ-wide edicts add recurring energy/workforce
    // demand as a pct of the accumulated base, applied here — BEFORE the throttle below —
    // so an unaffordable edict correctly drags m_fPwrMult/m_fPplMult down (the cost half).
    if ( m_fEdictEnergyUpkeepPct    > 0.0f ) m_iPwrNeed     += (int)( m_iPwrNeed     * m_fEdictEnergyUpkeepPct );
    if ( m_fEdictWorkforceUpkeepPct > 0.0f ) m_iPplNeedBldg += (int)( m_iPplNeedBldg * m_fEdictWorkforceUpkeepPct );

    if ( m_iPplBldg < m_iPplNeedBldg )
        m_fPplMult = float( m_iPplBldg ) / float( m_iPplNeedBldg );
    else
        m_fPplMult = 1.0;
    if ( m_iPwrHave < m_iPwrNeed )
        m_fPwrMult = float( m_iPwrHave ) / float( m_iPwrNeed );
    else
        m_fPwrMult = 1.0;

    m_iGasTurn += theGame.GetOpersElapsed( );
    if ( m_iGasTurn >= 5 * 24 * AVG_SPEED_MUL )
    {
        if ( m_iGasUsed == 0 )
            m_iGasNeed = MIN_GAS_NEED;
        else
        {
            div_t dtRate = div( (int)m_iGasUsed, GAS_USUAGE );
            // fuel_efficiency research lowers how much gas the same travel burns
            // (GetFuelPct() = 100 down to ~60 at 10 levels; see CPlayer::GetFuelPct).
            int iBurn = ( dtRate.quot * GetFuelPct( ) ) / 100;
            if ( m_iGas > iBurn )
                m_iGas -= iBurn;
            else
                m_iGas = 0;
            m_iGasUsed = dtRate.rem;

            m_iGasNeed = ( iBurn * 12 * 5 * 24 * AVG_SPEED_MUL ) / m_iGasTurn;
            m_iGasNeed = __max( m_iGasNeed, MIN_GAS_NEED );
        }

        m_iGasTurn = 0;

        // This per-period boundary (~once per game-minute) is also our history
        // sampling tick. m_iPwrHave / m_iPwrNeed are still the finalized totals from
        // the just-finished accumulation cycle (they aren't cleared until below).
        SampleHistory( );
    }

    // clear for next count
    m_iPwrHave     = 0;
    m_iPwrNeed     = 0;
    m_iPplNeedBldg = 0;
    m_iFoodProd    = 0;

    m_iRsrchHave = 0;
}

// Append one colony-stat sample to the ring buffer (called once per period from
// StartLoop). Feeds the building-info windows' history graphs.
void CPlayer::SampleHistory( )
{
    m_aHistPwrHave[m_iHistHead]  = m_iPwrHave;
    m_aHistPwrNeed[m_iHistHead]  = m_iPwrNeed;
    m_aHistPplTotal[m_iHistHead] = GetPplTotal( );
    m_aHistPplBldg[m_iHistHead]  = m_iPplBldg;
    m_aHistAptCap[m_iHistHead]   = m_iAptCap;
    m_aHistOfcCap[m_iHistHead]   = m_iOfcCap;

    m_iHistHead = ( m_iHistHead + 1 ) % HIST_LEN;
    if ( m_iHistCount < HIST_LEN )
        m_iHistCount++;
}

void CPlayer::Research( int iNumSec )
{

    if ( ( m_iRsrchHave <= 0 ) || ( m_iRsrchItem <= 0 ) )
        return;

    // Upper-bound guard: m_iRsrchItem can go out of range (suspected AiNextRsrch
    // returning an OOB index), and ElementAt() is an unchecked raw-pointer index on
    // every platform (MSVC CArray + the POSIX mfc_compat shim) -> ACCESS VIOLATION at
    // the :ASSERT deref below. The pre-existing guard only checks the lower bound, so
    // cap the upper bound the same idiomatic way player.h already does (GetSize()).
    // NOTE: this converts the crash into a safe skip; it does NOT explain why the index
    // goes out of range -- that root cause stays open (repro-first, win-owned).
    if ( ( m_iRsrchItem >= theRsrch.GetSize( ) ) ||
         ( m_iRsrchItem >= m_aRsrch.GetSize( ) ) )
        return;

    BOOL          bFoundIt = FALSE;
    CRsrchItem*   pRi      = &theRsrch.ElementAt( m_iRsrchItem );
    CRsrchStatus* pRs      = &GetRsrch( GetRsrchItem( ) );
    ASSERT( !pRs->m_bDiscovered );

    int iNum = m_iRsrchHave * iNumSec * 2;
    pRs->m_iPtsDiscovered += iNum;

    // did we discover it
    if ( pRs->m_iPtsDiscovered > pRi->m_iPtsRequired * 2 )
        bFoundIt = TRUE;
    else if ( pRs->m_iPtsDiscovered > pRi->m_iPtsRequired )
        if ( RandNum( pRs->m_iPtsDiscovered * iNum ) > pRi->m_iPtsRequired * iNum )
            bFoundIt = TRUE;

    if ( !bFoundIt )
    {
        // CDlgResearch removed (Phase 2d) — SDL2ResearchDialog re-reads progress on open.
        return;
    }

    // set attributes
    UpdateRacialAttributes( m_iRsrchItem );

    // tell others
    CNetRsrchDisc msg( this, m_iRsrchItem );
    theGame.PostToAll( &msg, sizeof( msg ), FALSE );

    // ok, we discovered something
    m_iRsrchHave       = 0;
    pRs->m_bDiscovered = TRUE;
    m_iLastDiscovered  = m_iRsrchItem;  // remember for the Discovery button (persists in saves)
    if ( IsAI( ) )
    {
#ifdef _LOGOUT
        logPrintf( LOG_PRI_CRITICAL, LOG_AI_MISC, "Player %d discovered %d", GetPlyrNum( ), m_iRsrchItem );
#endif
        m_iRsrchItem = AiNextRsrch( this, m_iRsrchItem );
#ifdef _LOGOUT
        logPrintf( LOG_PRI_CRITICAL, LOG_AI_MISC, "Player %d started researching %d", GetPlyrNum( ), m_iRsrchItem );
#endif
        return;
    }

    // tell the user
    int iTmp     = m_iRsrchItem;
    m_iRsrchItem = 0;
    if ( IsMe( ) )
    {
        ResearchDiscovered( iTmp );
        pRi = &theRsrch.ElementAt( iTmp );
        std::string sMsg = strPrintf( EnLoadStdString( IDS_EVENT_RSRCH_DONE ).c_str(),
                                      pRi->m_sName.c_str() );
        theApp.m_wndBar.SetStatusText( 0, sMsg.c_str(), CStatInst::status );
        theGame.MulEvent( MEVENT_RSRCH_DONE, NULL );
        CWndComm::UpdateMail( );
    }
}

#ifdef _CHEAT
// DEV cheat (SP only): discover every research topic + apply its effect + refresh UI. Lets
// the research-gated tail (AltOutput toggles, fort/seaport/shipyard/embassy, edicts) be
// verified instantly instead of a multi-hour grind. Per topic, mirrors Research()'s
// completion path: set m_bDiscovered (opens the gates) + UpdateRacialAttributes (applies the
// effect) + ResearchDiscovered (count + live research/build-dialog refresh). _CHEAT-gated
// (Debug/Sanitize only — NOT Release); callers also opt-in via [Cheat] registry + SP-guard.
// Cross-platform: the Windows F12 hotkey and a POSIX control_socket cmd both call it.
void CPlayer::DebugDiscoverAllResearch( )
{
    for ( int iOn = 0; iOn < m_aRsrch.GetSize( ); iOn++ )
    {
        if ( !m_aRsrch.ElementAt( iOn ).m_bDiscovered )
        {
            m_aRsrch.ElementAt( iOn ).m_bDiscovered = TRUE;
            UpdateRacialAttributes( iOn );
            ResearchDiscovered( iOn );
        }
    }
}
#endif

// we may have to update some flags
void CPlayer::UpdateRacialAttributes( int iRsrch )
{

    switch ( iRsrch )
    {
    case CRsrchArray::const_1:
        m_fConstProd = m_InitData.GetRace( CRaceDef::build_bldgs ) * 1.125;
        break;
    case CRsrchArray::const_2:
        m_fConstProd = m_InitData.GetRace( CRaceDef::build_bldgs ) * 1.25;
        break;
    case CRsrchArray::const_3:
        m_fConstProd = m_InitData.GetRace( CRaceDef::build_bldgs ) * 1.5;
        break;
    case CRsrchArray::manf_1:
        m_fManfProd = m_InitData.GetRace( CRaceDef::manf_vehicles ) * 1.125;
        break;
    case CRsrchArray::manf_2:
        m_fManfProd = m_InitData.GetRace( CRaceDef::manf_vehicles ) * 1.25;
        break;
    case CRsrchArray::manf_3:
        m_fManfProd = m_InitData.GetRace( CRaceDef::manf_vehicles ) * 1.5;
        break;
    case CRsrchArray::mine_1:
        m_fMineProd = m_InitData.GetRace( CRaceDef::mine_prod ) * 1.125;
        break;
    case CRsrchArray::mine_2:
        m_fMineProd = m_InitData.GetRace( CRaceDef::mine_prod ) * 1.25;
        break;
    case CRsrchArray::farm_1:
        m_fFarmProd = m_InitData.GetRace( CRaceDef::farm_prod ) * 1.25;
        break;

    case CRsrchArray::spot_1:
    case CRsrchArray::spot_2:
    case CRsrchArray::spot_3:
        m_bSpotting = m_iRsrchItem - CRsrchArray::spot_1 + 1;
        m_bSpotting = __minmax( 0, 3, m_bSpotting );
        break;
    // spot_4/spot_5 are appended at the END of the enum (not contiguous after spot_3),
    // so map them explicitly. Sight bonus per level is in CUnit::AssignData.
    case CRsrchArray::spot_4:
        m_bSpotting = 4;
        break;
    case CRsrchArray::spot_5:
        m_bSpotting = 5;
        break;
    case CRsrchArray::range_1:
    case CRsrchArray::range_2:
    case CRsrchArray::range_3:
        m_bRange = m_iRsrchItem - CRsrchArray::range_1 + 1;
        m_bRange = __minmax( 0, 3, m_bRange );
        break;
    case CRsrchArray::atk_1:
    case CRsrchArray::atk_2:
    case CRsrchArray::atk_3:
        m_bAttack = m_iRsrchItem - CRsrchArray::atk_1 + 1;
        m_bAttack = __minmax( 0, 3, m_bAttack );
        break;
    case CRsrchArray::def_1:
    case CRsrchArray::def_2:
    case CRsrchArray::def_3:
        m_bDefense = m_iRsrchItem - CRsrchArray::def_1 + 1;
        m_bDefense = __minmax( 0, 3, m_bDefense );
        break;
    case CRsrchArray::acc_1:
    case CRsrchArray::acc_2:
    case CRsrchArray::acc_3:
        m_bAccuracy = m_iRsrchItem - CRsrchArray::acc_1 + 1;
        m_bAccuracy = __minmax( 0, 3, m_bAccuracy );
        break;
    }
}

void CPlayer::UpdateRemote( CNetSaveInfo* pMsg )
{

    m_iPplBldg   = pMsg->m_iPplBldg;
    m_iPplVeh    = pMsg->m_iPplVeh;
    m_iFood      = pMsg->m_iFood;
    m_iGas       = pMsg->m_iGas;
    m_iRsrchItem = pMsg->m_iRsrchItem;
    if ( m_iRsrchItem > 0 )
    {
        TRAP( );
        GetRsrch( m_iRsrchItem ).m_iPtsDiscovered = pMsg->m_iPtsDiscovered;
    }
}

void CPlayer::PeopleAndFood( int iNumSec )
{

    // time to eat
    int   iPplTotal  = m_iPplBldg + m_iPplVeh;
    float fPplTtlSec = float( iPplTotal * iNumSec ) / float( AVG_SPEED_MUL );
    m_fFoodMod += fPplTtlSec * m_fEatingRate;

    // track what we need for a minute
    m_iFoodNeed = float( iPplTotal * 60 ) * m_fEatingRate;
    // Edict upkeep (Edicts v1): active civ-wide edicts add extra food demand (pct of base).
    if ( m_fEdictFoodUpkeepPct > 0.0f )
        m_iFoodNeed += (int)( m_iFoodNeed * m_fEdictFoodUpkeepPct );

    // do we need to eat?
    if ( m_fFoodMod >= 1 )
    {
        int iFood = (int)m_fFoodMod;
        m_fFoodMod -= (int)m_fFoodMod;

        // if we don't have enough food - kill some people off
        if ( iFood > m_iFood )
        {
            iFood = __max( 1, iFood );

            // we only kill off part of the pop that can't eat
            m_fPopMod += m_fPopDeath * fPplTtlSec * ( 1.0 - float( m_iFood ) / float( iFood ) );
            if ( m_fPopMod >= 1 )
            {
                if ( (int)m_fPopMod > m_iPplBldg / 4 )
                    m_fPopMod = float( m_iPplBldg / 4 );
                m_iPplBldg -= (int)m_fPopMod;
                m_fPopMod -= (int)m_fPopMod;
                m_iPplBldg = __max( m_iPplBldg, 10 );
                if ( IsMe( ) )
                    theApp.m_wndBar.UpdatePeople( );
            }

            m_iFood = 0;
            if ( IsMe( ) )
                theApp.m_wndBar.UpdateFood( );
            return;
        }

        // eat the food
        m_iFood -= iFood;
    }

    // apartment check - no babies if over 200% of capacity or over 100% and have excess people
    if ( iPplTotal > m_iAptCap * 2 )
        goto CheckDie;
    if ( m_iPplBldg > m_iOfcCap * 2 )
        goto CheckDie;
    if ( m_iPplBldg > m_iPplNeedBldg )
    {
        if ( iPplTotal > m_iAptCap )
            goto CheckDie;
        if ( m_iPplBldg > m_iOfcCap )
            goto CheckDie;
    }

    // no babies unless we have enough food or need people
    if ( ( m_iFood > m_iFoodNeed ) || ( m_iPplNeedBldg > m_iPplBldg ) )
    {
        // slow this down a little
        if ( MyRand( ) & 0x0100 )
            return;

        // we will have babies up to 100% of our food capability + 1/16th of our overstock
        float fFullPpl = float( m_iFoodProd ) / ( m_fEatingRate * 60.0 );
        float fMaxPpl  = fFullPpl;
        if ( m_iFood > m_iFoodNeed * 4 )
            fMaxPpl += float( ( m_iFood - m_iFoodNeed * 4 ) / 16 ) / ( m_fEatingRate * 60.0 );

        // do we need more babies?
        if ( (int)fMaxPpl >= iPplTotal )
        {
            float fAdd = fPplTtlSec * m_fPopGrowth;
            if ( iPplTotal > m_iAptCap )             // slow down if crowded
                fAdd /= 2.0;
            else if ( m_iPplBldg < m_iPplNeedBldg )  // speed up if serious need
                fAdd *= 2.0;
            m_fPopMod += fAdd;
            if ( m_fPopMod >= 1 )
            {
                int iOldApt = GetPplTotal( );
                int iOldOfc = GetPplBldg( );

                int iPopMod = (int)m_fPopMod;
                m_iPplBldg += iPopMod;
                m_fPopMod -= iPopMod;
                if ( m_iPplBldg > (int)fMaxPpl )
                    m_iPplBldg = (int)fMaxPpl;
                if ( IsMe( ) )
                    theApp.m_wndBar.UpdatePeople( );

                if ( IsMe( ) )
                {
                    if ( ( iOldApt <= m_iAptCap ) && ( GetPplTotal( ) > m_iAptCap ) )
                        theGame.Event( EVENT_LOW_HOUSING, EVENT_BAD );
                    if ( ( iOldOfc <= m_iOfcCap ) && ( GetPplBldg( ) > m_iOfcCap ) )
                        theGame.Event( EVENT_LOW_OFFICE, EVENT_BAD );
                }
            }
            return;
        }
    }

CheckDie:
    // do we have more than we can feed?
    if ( ( m_iPplNeedBldg < m_iPplBldg ) && ( m_iFood < m_iFoodNeed * 2 ) )
    {
        m_fPopMod += m_fPopDeath * fPplTtlSec;
        if ( m_fPopMod >= 1 )
        {
            m_iPplBldg -= (int)m_fPopMod;
            m_fPopMod -= (int)m_fPopMod;
            m_iPplBldg = __max( m_iPplBldg, 10 );
            if ( IsMe( ) )
                theApp.m_wndBar.UpdatePeople( );
        }
    }
}

BOOL CPlayer::CanRsrch( int iIndex )
{

    // bad index - can't research
    if ( ( iIndex <= 0 ) || ( CRsrchArray::num_types <= iIndex ) )
        return ( FALSE );

    CRsrchItem* pRi = &theRsrch.ElementAt( iIndex );

    // its already discovered
    if ( GetRsrch( iIndex ).m_bDiscovered )
        return ( FALSE );

    // can we research it in this scenario
    if ( ( theGame.GetScenario( ) != -1 ) && ( theGame.GetScenario( ) < pRi->m_iScenarioReq ) )
        return ( FALSE );

    // are all precursor topics discovered
    for ( int iNum = 0, *piNum = pRi->m_piRsrchRequired; iNum < pRi->m_iNumRsrchRequired; iNum++, piNum++ )
        if ( !GetRsrch( *piNum ).m_bDiscovered )
            return ( FALSE );

    // are all precursor buildings built?
    for ( int iNum = 0, *piNum = pRi->m_piBldgsRequired; iNum < pRi->m_iNumBldgsRequired; iNum++, piNum++ )
        if ( !GetExists( *piNum ) )
            return ( FALSE );

    return ( TRUE );
}

void SerializeElements( CArchive& ar, CPlayer** ppNewPlyr, int iCount )
{

    while ( iCount-- )
    {
        ( *ppNewPlyr )->Serialize( ar );
        ppNewPlyr++;
    }
}

void CPlayer::Serialize( CArchive& ar )
{

    if ( ar.IsStoring( ) )
    {
        ASSERT_VALID( this );

        ar << m_rgbPlyr << m_clrPlyr;
        ar << m_iPwrNeed;
        ar << m_iPwrHave;
        ar << m_iPplNeedBldg;
        ar << m_iPplBldg;
        ar << m_iPplVeh;
        ar << m_iFood;
        ar << m_iFoodProd;
        ar << m_iFoodNeed;
        ar << m_iGas;
        ar << m_iGasUsed;
        ar << m_iGasNeed;
        ar << m_iGasTurn;
        ar << m_fPplMult;
        ar << m_fPwrMult;

        ar << m_iRsrchHave;
        ar << m_iRsrchItem;

        // Serialize research statuses element-by-element to avoid raw (vptr) copying.
        ar.WriteCount( m_aRsrch.GetSize( ) );
        for ( INT_PTR i = 0; i < m_aRsrch.GetSize( ); ++i ) m_aRsrch.ElementAt( i ).Serialize( ar );

        if ( m_piBldgExists == NULL )
            ar << (LONG)0;
        else
        {
            ar << (LONG)theStructures.GetNumBuildings( );
            for ( int iOn = 0; iOn < theStructures.GetNumBuildings( ); iOn++ ) ar << m_piBldgExists[iOn];
        }

        ar << m_fConstProd;
        ar << m_fMtrlsProd;
        ar << m_fManfProd;
        ar << m_fMineProd;
        ar << m_fFarmProd;
        ar << m_fPopGrowth;
        ar << m_fPopDeath;
        ar << m_fEatingRate;
        ar << m_fPopMod;
        ar << m_fFoodMod;
        ar << m_fRsrchProd;
        ar << m_fAttack;
        ar << m_fDefense;

        ar << m_iRelations;

        ar << m_iBldgsBuilt;
        ar << m_iVehsBuilt;
        for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ ) 
            ar << m_aiMade[iInd] << m_aiHave[iInd];
        ar << m_iBldgsDest;
        ar << m_iVehsDest;

        ar << m_iBldgsHave << m_iVehsHave;

        ar << m_sName;

        ar << m_iPlyrNum;
        if ( theGame.AmServer( ) )
            ar << m_bAI;
        else
            ar << (BYTE)FALSE;
        ar << m_bLocal;
        ar << m_bMe;

        ar << m_bSpotting;
        ar << m_bRange;
        ar << m_bAttack;
        ar << m_bDefense;
        ar << m_bAccuracy;

        ar << m_iAptCap << m_iOfcCap << m_dwIDRocket;
        ar << m_iNumTrucks << m_iNumCranes;

        m_InitData.Serialize( ar );

        // Save release 3+: most-recent discovery (for the research window's
        // "Discovery" button). See version.h. Always written by this build.
        ar << (LONG)m_iLastDiscovered;

        // Save release 4+: colony stat history ring buffers (graphs). Always written.
        ar << (LONG)m_iHistHead << (LONG)m_iHistCount;
        for ( int i = 0; i < HIST_LEN; i++ )
            ar << m_aHistPwrHave[i] << m_aHistPwrNeed[i] << m_aHistPplTotal[i]
               << m_aHistPplBldg[i] << m_aHistAptCap[i] << m_aHistOfcCap[i];
    }

    else
    {
        ar >> m_rgbPlyr >> m_clrPlyr;
        ar >> m_iPwrNeed;
        ar >> m_iPwrHave;
        ar >> m_iPplNeedBldg;
        ar >> m_iPplBldg;
        ar >> m_iPplVeh;
        ar >> m_iFood;
        ar >> m_iFoodProd;
        ar >> m_iFoodNeed;
        ar >> m_iGas;
        ar >> m_iGasUsed;
        ar >> m_iGasNeed;
        ar >> m_iGasTurn;
        ar >> m_fPplMult;
        ar >> m_fPwrMult;

        ar >> m_iRsrchHave;
        ar >> m_iRsrchItem;
        // Read research statuses element-by-element to preserve valid object layout.
        {
            DWORD_PTR nNewSize = ar.ReadCount( );
            m_aRsrch.SetSize( nNewSize );
            for ( INT_PTR i = 0; i < m_aRsrch.GetSize( ); ++i ) m_aRsrch.ElementAt( i ).Serialize( ar );
        }

        // in a net game saved early some players may not have the R&D initialized
        if ( m_aRsrch.GetSize( ) < theRsrch.GetSize( ) )
        {
            m_aRsrch.SetSize( theRsrch.GetSize( ) );
            m_aRsrch.ElementAt( 0 ).m_bDiscovered = TRUE;

            // if no points required it's discovered (how we kill an item w/o removing it)
            for ( int iOn = 1; iOn < m_aRsrch.GetSize( ); iOn++ )
                if ( theRsrch[iOn].m_iPtsRequired <= 0 )
                    m_aRsrch.ElementAt( iOn ).m_bDiscovered = TRUE;
        }

        LONG l;
        ar >> l;
        if ( !l )
        {
            ASSERT( l ); // why would this be null?
            m_piBldgExists = NULL;
        }
        else
        {
            // determine the arraysize before memset, to prevent writing outside of the array (if new buildings were added)
            // get which ever is longer, l or theStructures.GetNumBuildings( )
            int arraySize  = max( (int)l, theStructures.GetNumBuildings( ) );

            if ( m_piBldgExists ) // in case its somehow called twice?
                delete[] m_piBldgExists;

            m_piBldgExists = new LONG[arraySize];
            memset( m_piBldgExists, 0, arraySize * sizeof( LONG ) );

            for ( int iOn = 0; iOn < l; iOn++ ) ar >> m_piBldgExists[iOn];
        }

        ar >> m_fConstProd;
        ar >> m_fMtrlsProd;
        ar >> m_fManfProd;
        ar >> m_fMineProd;
        ar >> m_fFarmProd;
        ar >> m_fPopGrowth;
        ar >> m_fPopDeath;
        ar >> m_fEatingRate;
        ar >> m_fPopMod;
        ar >> m_fFoodMod;
        ar >> m_fRsrchProd;
        ar >> m_fAttack;
        ar >> m_fDefense;

        ar >> m_iRelations;

        ar >> m_iBldgsBuilt;
        ar >> m_iVehsBuilt;
        for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ ) ar >> m_aiMade[iInd] >> m_aiHave[iInd];
        ar >> m_iBldgsDest;
        ar >> m_iVehsDest;

        ar >> m_iBldgsHave >> m_iVehsHave;

        ar >> m_sName;

        ar >> m_iPlyrNum;
        ar >> m_bAI;
        ar >> m_bLocal;
        ar >> m_bMe;

        ar >> m_bSpotting;
        ar >> m_bRange;
        ar >> m_bAttack;
        ar >> m_bDefense;
        ar >> m_bAccuracy;

        ar >> m_iAptCap >> m_iOfcCap >> m_dwIDRocket;
        ar >> m_iNumTrucks >> m_iNumCranes;

        m_InitData.Serialize( ar );

        // Save release 3+ carries the most-recent discovery (Discovery button).
        // Older saves (release 2) predate the field — leave it default (0) and do
        // NOT read, or the stream would desync. theGame.m_dwVer holds the loaded
        // save's release (read at the top of CGame::Serialize, before players).
        if ( theGame.m_dwVer >= 3 )
        {
            LONG l;
            ar >> l;
            m_iLastDiscovered = l;
        }

        // Save release 4+ carries the colony stat history. Older saves predate it,
        // so the arrays stay at their init (empty) and we do NOT read.
        if ( theGame.m_dwVer >= 4 )
        {
            LONG lHead, lCount;
            ar >> lHead >> lCount;
            m_iHistHead  = (int)lHead;
            m_iHistCount = (int)lCount;
            for ( int i = 0; i < HIST_LEN; i++ )
                ar >> m_aHistPwrHave[i] >> m_aHistPwrNeed[i] >> m_aHistPplTotal[i]
                   >> m_aHistPplBldg[i] >> m_aHistAptCap[i] >> m_aHistOfcCap[i];
        }
        else
            m_iLastDiscovered = 0;

        m_bState  = created;
        m_dwAiHdl = 0;
        m_iNetNum = 0;

        // m_bPlacedRocket isn't serialized, so it would deserialize as FALSE (its
        // ctor value). Any saved game is post-landing (you can't save during the
        // initial rocket placement), so force it TRUE here. Without this the minimap
        // mode test (CWndWorld m_bIsRadar = has-command-center || !placed-rocket)
        // sees "rocket not placed" and forces RADAR on every loaded game, and the
        // building-count base (mainloop ConstComplete recount) is off by one.
        m_bPlacedRocket = TRUE;

        // in case color depth changes
        SetPlyrNum( m_iPlyrNum );
    }
}

BOOL CPlayer::BuildCcBldg(int iBldg) {
    return 1;
}

#ifdef _DEBUG
void CPlayer::AssertValid( ) const
{

    CObject::AssertValid( );

    // m_sName converted to std::string (Phase 5c) — no MFC validator.
    ASSERT( ( m_iNetNum & 0xFF ) == m_iNetNum );

    // it makes sense for these to be 0 when you start a new minimal game
    /*
    for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ ) ASSERT( m_aiHave[iInd] >= 0 );
    */
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CGame - a game

void CGame::_ctor( )
{

    m_bServer    = FALSE;
    m_bIsNetGame = FALSE;
    ctor( );

    // block size, block count, flags
   // m_memPoolLarge = mempool_large( );  // MEM_POOL_SERIALIZE flag used indicates this needs to be usable multithreaded.
   // m_memPoolSmall = mempool_small( );
    m_memPoolLarge.init( );
    m_memPoolSmall.init( );

    // MemPoolInitFS creates a new memory pool from which fixed-size memory blocks are to be allocated. You must create
    // a memory pool before allocating fixed-size memory blocks.
    //
    // The blockSize parameter specifies the size of fixed-size memory blocks to be allocated from this pool. You can’t
    // change the block size after the first allocation. SmartHeap may round the block size up for alignment — use
    // MemPoolInfo or MemSizePtr to determine the actual block size.
    //
    // The blockCount parameter specifies the initial number of blocks to allocate in the memory pool — the pool will
    // grow beyond this if necessary to satisfy allocation requests.
    //
    // See MemPoolInit for details on the flags parameter.
    //
    // MemPoolInitFS is equivalent to calling the combination of MemPoolInit, MemPoolSetBlockSizeFS, and
    // MemPoolPreAllocate.
}

void CGame::ctor( )
{

    m_iAi = m_iSize = m_iPos = 0;
    m_iWorldType = WORLD_DEFAULT;
    m_iRivers    = 60;  // river density slider baseline
    m_iOcean     = 50;  // ocean size slider baseline (~= current average)
    m_sFileName              = "";

    m_iTryCount = 0;
    m_iState    = main;
    m_pMe       = NULL;
    m_pServer   = NULL;
    m_bHP       = FALSE;

    m_uTimer     = 0;
    m_bUnPauseMe = m_bShouldPause = m_bShouldNetPause = m_bPauseMsgs = m_bMessages = m_bAnimate = m_bOperate = FALSE;
    m_dwElapsedTime                                                                                  = 0;
    m_dwOperTimeLast = m_dwFrameTimeLast = timeGetTime( );
    m_dwFramesElapsed = m_dwOpersElapsed = m_dwOperSecElapsed = m_dwOperSecFrames;
    m_dwFrame                                                 = 0;

    m_iNetJoin     = create;
    m_iSideSize    = 32;
    m_dwNextUnitID = 2;
    m_iNextPlyrNum = 1;
    m_iNextAINum   = 1;

    m_bHaveAlliances = FALSE;

    m_iScenarioNum = -1;
    m_iScenarioVar = 0;
    memset( m_adwScenarioUnits, 0, sizeof( m_adwScenarioUnits ) );

    m_iSpeedMul = NUM_SPEEDS / 2;

    m_pHpRtr = NULL;

    m_hexAreaCenter = CHexCoord( 0, 0 );
    memset( &m_wpArea, 0, sizeof( WINDOWPLACEMENT ) );
    memset( &m_wpWorld, 0, sizeof( WINDOWPLACEMENT ) );
    memset( &m_wpChat, 0, sizeof( WINDOWPLACEMENT ) );
    memset( &m_wpBldgs, 0, sizeof( WINDOWPLACEMENT ) );
    memset( &m_wpVehicles, 0, sizeof( WINDOWPLACEMENT ) );
    memset( &m_wpRelations, 0, sizeof( WINDOWPLACEMENT ) );
    memset( &m_wpFile, 0, sizeof( WINDOWPLACEMENT ) );
    memset( &m_wpRsrch, 0, sizeof( WINDOWPLACEMENT ) );

    _SettimeGetTime( );

    m_xScreen = theApp.m_iScrnX;
    m_yScreen = theApp.m_iScrnY;

    m_pXferFromServer = NULL;
    m_pGameFile       = NULL;
    m_iGameBufLen     = 0;
    m_iNumSends       = 0;

    m_dwMaj = VER_MAJOR;
    m_dwMin = VER_MINOR;
    m_dwVer = VER_RELEASE;
    m_wDbg  = _wDebug;
    m_wCht  = _wCheat;

    ASSERT_VALID( this );
}

void CGame::Open( BOOL bLocal )
{

    ASSERT_VALID( this );

    std::string sSaveName = theGame.m_sFileName;
    if ( ( theApp.m_pCreateGame == NULL ) || ( ( theApp.m_pCreateGame->m_iTyp != CCreateBase::load_single ) &&
                                               ( theApp.m_pCreateGame->m_iTyp != CCreateBase::load_multi ) ) )
        ctor( );
    theGame.m_sFileName = sSaveName;

    int iSpeed = EnGetProfileInt( "Game", "Speed", NUM_SPEEDS / 2 );
    iSpeed     = __minmax( 0, NUM_SPEEDS - 1, iSpeed );
    SetGameMul( iSpeed );

    if ( !bLocal )
    {
        m_pMe = m_pServer = NULL;
        m_bHP             = FALSE;
    }
    else

    {
        CPlayer* pPlyr  = new CPlayer( );
        m_pMe           = pPlyr;
        m_bHP           = TRUE;
        pPlyr->m_bLocal = pPlyr->m_bMe = TRUE;
        pPlyr->SetRelations( RELATIONS_ALLIANCE );
        pPlyr->SetTheirRelations( RELATIONS_ALLIANCE );
        AddPlayer( pPlyr );

        // is this the server or do we need to create it?
        if ( m_bServer )
            m_pServer = pPlyr;
    }

    SetState( open );
    SetShouldProcessMessages(TRUE);
}

// get the item in the list
CPlayer* CGame::_GetPlayer( int iNetNum ) const
{

    ASSERT_VALID( this );

    POSITION pos;
    for ( pos = m_lstAll.GetTailPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = m_lstAll.GetPrev( pos );
        ASSERT_VALID( pPlr );
        if ( pPlr->GetNetNum( ) == iNetNum )
            return ( pPlr );
    }

    // could be a load game
    if ( ( theApp.m_pCreateGame != NULL ) && ( ( theApp.m_pCreateGame->m_iTyp == CCreateBase::load_multi ) ||
                                               ( theApp.m_pCreateGame->m_iTyp == CCreateBase::load_join ) ) )
    {
        POSITION pos;
        for ( pos = theGame.m_lstLoad.GetTailPosition( ); pos != NULL; )
        {
            CPlayer* pPlr = theGame.m_lstLoad.GetPrev( pos );
            if ( pPlr->GetNetNum( ) == iNetNum )
                return ( pPlr );
        }
    }

    // ok - he may be dead
    for ( pos = m_lstDead.GetTailPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = m_lstDead.GetPrev( pos );
        ASSERT_VALID( pPlr );
        if ( pPlr->GetNetNum( ) == iNetNum )
        {
            TRAP( );
            return ( pPlr );
        }
    }

    return ( NULL );
}

// get the item in the list
CPlayer* CGame::GetPlayer( int iNetNum ) const
{

    CPlayer* pPlyr = _GetPlayer( iNetNum );
    if ( pPlyr != NULL )
        return ( pPlyr );

    ASSERT( FALSE );
    // BUGBUG	ThrowError (ERR_TLP_BAD_PLAYER_NET_NUM);
    return ( NULL );
}

CPlayer* CGame::_GetPlayerByPlyr( int iPlyrNum ) const
{

    ASSERT_VALID( this );

    // if == 0 -> its us
    if ( iPlyrNum == 0 )
        return ( m_pMe );

    POSITION pos;
    for ( pos = m_lstAll.GetTailPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = m_lstAll.GetPrev( pos );
        ASSERT_VALID( pPlr );
        if ( pPlr->GetPlyrNum( ) == iPlyrNum )
            return ( pPlr );
    }

    return ( NULL );  // avoid compile error
}

CPlayer* CGame::GetPlayerByPlyr( int iPlyrNum ) const
{

    CPlayer* pPlyr = _GetPlayerByPlyr( iPlyrNum );
    if ( pPlyr != NULL )
        return ( pPlyr );

    // ok - he may be dead
    POSITION pos;
    for ( pos = m_lstDead.GetTailPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = m_lstDead.GetPrev( pos );
        ASSERT_VALID( pPlr );
        if ( pPlr->GetPlyrNum( ) == iPlyrNum )
            return ( pPlr );
    }

    ASSERT( FALSE );
    return ( NULL );
}

void CGame::SetGameMul( int iSpeed )
{

    if ( iSpeed > NUM_SPEEDS / 2 )
        iSpeed *= 2;
    m_iSpeedMul = iSpeed + AVG_SPEED_MUL / 2;
}

void CGame::LoadToPlyr( CPlayer* pPlrLoad, CPlayer* pPlrAll )
{

    if ( pPlrLoad != pPlrAll )
    {
        pPlrAll->SetName( pPlrLoad->GetName( ) );
        pPlrAll->SetAI( FALSE );
        pPlrAll->SetLocal( FALSE );
    }

    if ( theApp.m_pCreateGame != NULL )
    {
        if ( pPlrAll->GetState( ) == CPlayer::load_pick )
            pPlrAll->SetState( CPlayer::ready );
        ( (CMultiBase*)theApp.m_pCreateGame )->m_wndPlyrList.RemovePlayer( pPlrLoad );
        ( (CMultiBase*)theApp.m_pCreateGame )->m_wndPlyrList.AddPlayer( pPlrAll );
    }

    // copy net connection across
    if ( pPlrLoad != pPlrAll )
    {
        pPlrAll->SetNetNum( pPlrLoad->GetNetNum( ) );
        pPlrLoad->SetNetNum( 0 );
        if ( pPlrLoad->m_pXferToClient != NULL )
        {
            pPlrAll->m_pXferToClient = pPlrLoad->m_pXferToClient;
            pPlrAll->m_pXferToClient = NULL;
        }

        // remove from load list
        POSITION pos = m_lstLoad.Find( pPlrLoad, NULL );
        if ( pos != NULL )
        {
            m_lstLoad.RemoveAt( pos );
            delete pPlrLoad;
        }
    }
}

int CGame::GetGameMul( ) const
{

    int iSpeed = m_iSpeedMul - AVG_SPEED_MUL / 2;
    if ( iSpeed > NUM_SPEEDS / 2 )
        iSpeed /= 2;
    return ( iSpeed );
}

void CGame::SetMessagesPaused(BOOL bPause )
{

    if ( bPause )
    {
        if ( m_uTimer != 0 )
            theApp.m_wndMain.KillTimer( m_uTimer );
        theApp.m_wndMain.SetTimer( m_uTimer = 119, 20 * 1000, NULL );
        m_bPauseMsgs = TRUE;
        m_bUnPauseMe = FALSE;
        return;
    }

    m_bUnPauseMe = TRUE;
    if ( m_uTimer != 0 )
    {
        theApp.m_wndMain.KillTimer( m_uTimer );
        m_uTimer = 0;
    }
    theGame.ClearNetPause();
}

void CGame::StartAllPlayers( ) const
{

    ASSERT_VALID( this );
    ASSERT( m_bServer == TRUE );

    // we send everyone a message telling them their player number
    // this is also gauranteed to be the first message when we go to
    // create the game
    POSITION pos;
    for ( pos = m_lstAll.GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = m_lstAll.GetNext( pos );
        ASSERT_VALID( pPlr );
        if ( ( !pPlr->IsLocal( ) ) && ( pPlr->GetState( ) != CPlayer::replace ) )
        {
            if ( pPlr->m_pXferToClient != NULL )
            {
                TRAP( );
                delete pPlr->m_pXferToClient;
                pPlr->m_pXferToClient = NULL;
            }
            CNetYouAre msg( pPlr->GetPlyrNum( ) );
            theNet.Send( pPlr->GetNetNum( ), &msg, sizeof( msg ) );
        }
    }

    // clean out buffer
    TRAP( m_pGameFile != NULL );
    delete m_pGameFile;
    ( (CGame*)this )->m_pGameFile = NULL;
    delete m_pXferFromServer;
    ( (CGame*)this )->m_pXferFromServer = NULL;

    // we now send init info on each player in m_lstOther order.
    //   join.cpp builds m_lstOther in the same order so they match
    for ( pos = m_lstAll.GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = m_lstAll.GetNext( pos );
        ASSERT_VALID( pPlr );
        if ( ( !pPlr->IsLocal( ) ) && ( pPlr->GetState( ) != CPlayer::replace ) )
        {
            POSITION posInfo;
            for ( posInfo = m_lstAll.GetHeadPosition( ); posInfo != NULL; )
            {
                CPlayer* pPlrInfo = m_lstAll.GetNext( posInfo );
                ASSERT_VALID( pPlrInfo );
                CNetPlayer* pMsg = CNetCmd::AllocPlayer( pPlrInfo );
                if ( pPlrInfo == pPlr )
                    pMsg->m_bLocal = TRUE;
                if ( pPlrInfo->GetState( ) == CPlayer::replace )
                    pMsg->m_bAI = TRUE;
                if ( pPlrInfo == theGame.GetServer( ) )
                    pMsg->m_bServer = TRUE;
                theNet.Send( pPlr->GetNetNum( ), pMsg, pMsg->GetLen( ) );
                delete pMsg;
            }
        }
    }
}

void CGame::StartNewWorld( unsigned uRand, int iSide, int iSideSize )
{
#ifdef LOGGINGON
    OutputDebugStringA( "StartNewWorld\n" );
#endif

    ASSERT_VALID( this );
    ASSERT( m_bServer == TRUE );

    // set the size
    m_iSideSize = iSideSize;

#ifdef LOGGINGON
    OutputDebugStringA( "CNetStart msg create\n" );
#endif
    CNetStart msg( uRand, iSide, iSideSize, theApp.m_pCreateGame->m_iAi, theApp.m_pCreateGame->m_iNumAi,
                   theGame.GetAll( ).GetCount( ) - theApp.m_pCreateGame->m_iNumAi, theApp.m_pCreateGame->m_iSize,
                   theApp.m_pCreateGame->m_iWorldType, theApp.m_pCreateGame->m_iRivers );

    POSITION pos;
    for ( pos = m_lstAll.GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = m_lstAll.GetNext( pos );
        ASSERT_VALID( pPlr );
        if ( !pPlr->IsLocal( ) )
            theNet.Send( pPlr->GetNetNum( ), &msg, sizeof( msg ) );
    }
}

void CGame::SendToServer( CNetCmd const* pMsg, int iLen )
{

    ASSERT_VALID( this );
    ASSERT_CMD( pMsg );
    ASSERT( m_bServer );
    TRAP( );

    EnterCriticalSection( &cs );

    ProcessMessage((CNetCmd *) pMsg);

    LeaveCriticalSection( &cs );
}

void CGame::PostToServer( CNetCmd const* pMsg, int iLen )
{

    ASSERT_VALID( this );
    ASSERT_CMD( pMsg );



    // if we are on the server we call directly
    if ( m_bServer )
    {
#ifdef LOGGINGON
        OutputDebugStringA( "AddToQueue\n" );
#endif
        AddToQueue( pMsg, iLen );
    }
    else
    {
#ifdef LOGGINGON
        OutputDebugStringA( "theNet.Send\n" );
#endif
        theNet.Send( GetServerNetNum( ), pMsg, iLen );
    }
}

void CGame::PostToClient( int iPlyr, CNetCmd const* pMsg, int iLen )
{

    CPlayer* pPlyr = _GetPlayerByPlyr( iPlyr );
    if ( pPlyr != NULL )
    {
        PostToClient( pPlyr, pMsg, iLen );
        return;
    }

    theNet.Send( iPlyr, pMsg, iLen );
}

void CGame::PostToClient( CPlayer* pPlyr, CNetCmd const* pMsg, int iLen )
{

    ASSERT_VALID( this );
    ASSERT_CMD( pMsg );
    ASSERT( m_bServer );

    // if we are on the server we call directly
    if ( m_bServer )
    {
        if ( pPlyr->IsAI( ) )
        {
            // redundancy filter: this is the HOT per-player AI delivery path
            // (broadcasts fan out through here per player); skip types this
            // AI provably ignores. Server-local; cannot affect client sync.
            if ( AiMessageWanted( pPlyr, pMsg ) )
                AiMessage( pPlyr->GetAiHdl( ), pMsg, iLen );
            else
                Perf::CounterInc( "ai.msg.skip" );
        }
        else
        {
            if ( pPlyr == _GetMe( ) )
                AddToQueue( pMsg, iLen );
            else
                theNet.Send( pPlyr->GetNetNum( ), pMsg, iLen );
        }
    }
    else
        PostClientToClient( pPlyr, pMsg, iLen );
}

void CGame::PostToClientByNet( int iNet, CNetCmd const* pMsg, int iLen )
{

    ASSERT_VALID( this );
    ASSERT_CMD( pMsg );
    ASSERT( m_bServer );

    // if it's us, we're there
    if ( iNet == GetMyNetNum( ) )
    {
        AddToQueue( pMsg, iLen );
        return;
    }

    TRAP( );
    theNet.Send( iNet, pMsg, iLen );
}

void CGame::PostClientToClient( CPlayer* pPlyr, CNetCmd const* pMsg, int iLen )
{

    ASSERT_VALID( this );
    ASSERT_CMD( pMsg );

    if ( pPlyr == GetMe( ) )
    {
        AddToQueue( pMsg, iLen );
        return;
    }

    // if we are on the server we call directly
    if ( m_bServer )
    {
        if ( pPlyr->IsAI( ) )
        {
            // redundancy filter (see PostToClient above)
            if ( AiMessageWanted( pPlyr, pMsg ) )
                AiMessage( pPlyr->GetAiHdl( ), pMsg, iLen );
            else
                Perf::CounterInc( "ai.msg.skip" );
        }
        else
            theNet.Send( pPlyr->GetNetNum( ), pMsg, iLen );
    }
    else

    {
        if ( !pPlyr->IsAI( ) )
            theNet.Send( pPlyr->GetNetNum( ), pMsg, iLen );
        else
        {
            CMsgAiMsg* pAiMsg = CMsgAiMsg::Alloc( pPlyr, pMsg, iLen );
            theNet.Send( theGame.GetServerNetNum( ), pAiMsg, pAiMsg->m_iAllocLen );
            delete[] (char*)pAiMsg;
        }
    }
}

void CGame::PostToAllAi( CNetCmd const* pMsg, int iLen )
{

    ASSERT_VALID( this );
    ASSERT_CMD( pMsg );

    if ( !m_bServer )
    {
        POSITION pos;
        for ( pos = m_lstAi.GetTailPosition( ); pos != NULL; )
        {
            CPlayer* pPlr = m_lstAi.GetPrev( pos );
            ASSERT_VALID( pPlr );
            ASSERT( pPlr->IsAI( ) );
            PostClientToClient( pPlr, pMsg, iLen );
        }
        return;
    }

    POSITION pos;
    for ( pos = m_lstAi.GetTailPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = m_lstAi.GetPrev( pos );
        ASSERT_VALID( pPlr );
        ASSERT( pPlr->IsAI( ) );

        // redundancy filter: skip deliveries this AI provably ignores
        // (owner-gated types; see AiMessageWanted in ai.cpp). Server-local,
        // cannot affect client sync.
        if ( !AiMessageWanted( pPlr, pMsg ) )
        {
            Perf::CounterInc( "ai.msg.skip" );
            continue;
        }

        AiMessage( pPlr->GetAiHdl( ), pMsg, iLen );
    }
}

void CGame::PostToAllClients( CNetCmd const* pMsg, int iLen, BOOL bAI )
{

    ASSERT_VALID( this );
    ASSERT_CMD( pMsg );
    ASSERT( m_bServer );

    if ( bAI )
        PostToAllAi( pMsg, iLen );
    theNet.Broadcast( pMsg, iLen, FALSE );
}

void CGame::PostToAll( CNetCmd const* pMsg, int iLen, BOOL bAI )
{

    ASSERT_VALID( this );
    ASSERT_CMD( pMsg );

    if ( bAI )
        PostToAllAi( pMsg, iLen );

    theNet.Broadcast( pMsg, iLen, TRUE );

    AddToQueue( pMsg, iLen );
}

BOOL CGame::IsAllReady( ) const
{

    ASSERT_VALID( this );

    // have to wait to tell us to go
    if ( !theGame.HaveHP( ) )
        return ( FALSE );

    if ( m_iNetJoin == approve )
        return ( FALSE );

    POSITION pos;
    for ( pos = m_lstAll.GetTailPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = m_lstAll.GetPrev( pos );
        ASSERT_VALID( pPlr );
        if ( pPlr->GetState( ) != CPlayer::ready )
            return ( FALSE );
    }

    return ( TRUE );
}

void CGame::AddPlayer( CPlayer* pPlr )
{
#ifdef LOGGINGON
    OutputDebugStringA( "AddPlayer\n" );
#endif

    ASSERT_VALID( this );
    ASSERT_VALID( pPlr );

    if ( pPlr->GetPlyrNum( ) == 0 )
        pPlr->SetPlyrNum( m_iNextPlyrNum++ );

    if ( ( theApp.m_pCreateGame != NULL ) && ( ( theApp.m_pCreateGame->m_iTyp == CCreateBase::load_join ) ||
                                               ( theApp.m_pCreateGame->m_iTyp == CCreateBase::load_multi ) ) )
        m_lstLoad.AddTail( pPlr );
    else
        m_lstAll.AddTail( pPlr );
}

void CGame::AddAiPlayer( CPlayer* pPlr )
{
#ifdef LOGGINGON
    OutputDebugStringA( "AddAiPlayer\n" );
#endif

    ASSERT_VALID( this );
    OutputDebugStringA( "this is valid; checking pPlr\n" );
    ASSERT_VALID( pPlr );

    if ( pPlr->GetPlyrNum( ) == 0 )
        pPlr->SetPlyrNum( m_iNextPlyrNum++ );

    if ( pPlr->m_sName.empty( ) )
    {
        std::string sNum = IntToStr( m_iNextAINum++ );
        std::string sName = strPrintf( EnLoadStdString( IDS_AI_NAME ).c_str(), sNum.c_str() );
        pPlr->m_sName = sName.c_str();
    }
    pPlr->m_iNetNum = 0;
    pPlr->SetAI( TRUE );
    pPlr->SetState( CPlayer::ready );

    m_lstAi.AddTail( pPlr );
}

void CGame::AiTakeOverPlayer( CPlayer* pPlr, BOOL bStartThread, BOOL bShowDlg )
{

    ASSERT_VALID( pPlr );

    pPlr->SetAI( TRUE );
    pPlr->SetLocal( m_bServer );
    pPlr->SetState( CPlayer::ready );
    pPlr->SetNetNum( 0 );

    m_lstAi.AddTail( pPlr );

    if ( m_bServer )
    {
        if ( pPlr->m_bPauseMsgs )
        {
            TRAP( );
            pPlr->m_bPauseMsgs = FALSE;
            POSITION pos;
            for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
            {
                CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
                // if any player says pause - we pause
                if ( pPlr->m_bPauseMsgs )
                {
                    TRAP( );
                    goto NoUnPause;
                }
            }
            SetMessagesPaused(FALSE);
        }
    NoUnPause:

        // tell them
        CDlgSaveMsg dlgMsg( CWnd::FromHandle( theApp.m_wndMain.m_hWnd ) );
        if ( bShowDlg )
        {
            dlgMsg.m_sText = strPrintf( EnLoadStdString( IDS_AI_TAKEOVER ).c_str(),
                                        (const char*)pPlr->GetName( ) );
            dlgMsg.Create( IDD_SAVE_MSG, CWnd::FromHandle( theApp.m_wndMain.m_hWnd ) );
        }

        ::AiTakeOverPlayer( pPlr );
        if ( bStartThread )
            myStartThread( &( pPlr->ai ), (AFX_THREADPROC)AiThread );

        // CDlgPlyrList removed (Phase 2d) — SDL2PlayerListDialog refreshes from
        // theGame state on each open, so no name-change push is needed.
        // (CDlgRelations invalidate removed)

        if ( bShowDlg )
            dlgMsg.DestroyWindow( );
    }
}

void CGame::AiReleasePlayer( CPlayer* pPlr, int iNetNum, const char* pName, BOOL bLocal, BOOL bPlaying )
{

    ASSERT_VALID( pPlr );

    pPlr->SetAI( FALSE );
    pPlr->SetLocal( bLocal );
    pPlr->SetState( CPlayer::ready );
    pPlr->SetNetNum( iNetNum );
    pPlr->SetName( pName );

    POSITION pos = m_lstAi.Find( pPlr, NULL );
    if ( pos != NULL )
        m_lstAi.RemoveAt( pos );

    if ( m_bServer )
    {
        ::AiKillPlayer( pPlr->GetAiHdl( ) );
        pPlr->m_iNumAiGpfs = 0;

        // CDlgPlyrList removed (Phase 2d) — SDL2PlayerListDialog refreshes on open.
        // (CDlgRelations invalidate removed)
    }

    if ( !bLocal )
        return;

    m_pMe = pPlr;

    if ( !bPlaying )
        return;

    // now add it's units to the windows
    // add to the listbox - only do this if its ours
    TRAP( );
    pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        ASSERT_STRICT_VALID( pBldg );
        if ( pBldg->GetOwner( )->IsMe( ) )
        {
            TRAP( );
            theApp.m_wndBldgs.AddToList( pBldg );
            pBldg->MakeBldgVisible( );
            pBldg->DetermineSpotting( );
            pBldg->IncrementSpotting( pBldg->m_hex );
        }
    }

    // same for vehicles
    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        ASSERT_STRICT_VALID( pVeh );
        if ( pVeh->GetOwner( )->IsMe( ) )
        {
            TRAP( );
            theApp.m_wndVehicles.AddToList( pVeh );

            pVeh->IncVisible( );
            if ( pVeh->GetHexOwnership( ) )
            {
                TRAP( );
                pVeh->DetermineSpotting( );
                pVeh->IncrementSpotting( pVeh->GetHexHead( ) );
            }
        }
        if ( ( pVeh->GetHexOwnership( ) ) && ( pVeh->GetOwner( )->IsLocal( ) ) )
            pVeh->OppoAndOthers( );
    }
}

void CGame::DeletePlayer( CPlayer* pPlr )
{

    RemovePlayer( pPlr );

    POSITION pos = m_lstDead.Find( pPlr, NULL );
    if ( pos != NULL )
        m_lstDead.RemoveAt( pos );

    delete pPlr;
}

// removes player from the game but does NOT delete the pointer
void CGame::RemovePlayer( CPlayer* pPlr )
{

    ASSERT_VALID( this );
    ASSERT_VALID( pPlr );

    // we're dead
    pPlr->SetState( CPlayer::dead );

    // undo pausing
    if ( m_bServer )
    {
        if ( pPlr->m_bPauseMsgs )
        {
            TRAP( );
            pPlr->m_bPauseMsgs = FALSE;
            POSITION pos;
            for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
            {
                CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
                // if any player says pause - we pause
                if ( pPlr->m_bPauseMsgs )
                {
                    TRAP( );
                    goto NoUnPause;
                }
            }
            SetMessagesPaused(FALSE);
        }
    }
NoUnPause:

    // CDlgPlyrList removed (Phase 2d) — SDL2PlayerListDialog refreshes on open.
    // (CDlgRelations RemovePlayer removed)

    // tell the AI
    DWORD_PTR dwAiID = pPlr->GetAiHdl( );   // pointer-width (was DWORD -> x64 truncation)
    pPlr->SetAiHdl( 0 );
    if ( ( m_bServer ) && ( dwAiID != 0 ) && ( pPlr->IsAI( ) ) && ( pPlr->m_iNumAiGpfs < 100 ) )
        ::AiKillPlayer( dwAiID );

    // close the connection - UNLESS to the server
    if ( pPlr == theGame.GetServer( ) )
    {
        theGame._SetServer( NULL );
        pPlr->SetNetNum( 0 );
    }
    else if ( pPlr->GetNetNum( ) != 0 )
    {
        theNet.DeletePlayer( pPlr->GetNetNum( ) );
        theApp.BaseYield( );
    }

    // we can't remove the AI player yet
    if ( ( !m_bServer ) || ( !pPlr->IsAI( ) ) )
    {
        POSITION pos = m_lstAi.Find( pPlr, NULL );
        if ( pos != NULL )
            m_lstAi.RemoveAt( pos );
        if ( ( pos = m_lstAll.Find( pPlr, NULL ) ) != NULL )
            m_lstAll.RemoveAt( pos );

        m_lstDead.AddTail( pPlr );
    }

    // no IPC if only 1 HP
    if ( theApp.m_wndChat.m_hWnd != NULL )
    {
        // kill chat
        theApp.m_wndChat.KillAiChatWnd( pPlr );

        // if now single player loose the comm
        if ( theGame.GetAll( ).GetCount( ) <= theGame.GetAi( ).GetCount( ) + 1 )
            theApp.m_wndChat.DestroyWindow( );
    }
    if ( theGame.GetAll( ).GetCount( ) <= theGame.GetAi( ).GetCount( ) + 1 )
        theApp.CloseDlgChat( );
}

// we can remove the AI player once it's thread has ended
void CGame::AiPlayerIsDead( CPlayer* pPlr )
{

    ASSERT_VALID( this );
    ASSERT_VALID( pPlr );

    // should never happen
    if ( ( !m_bServer ) || ( !pPlr->IsAI( ) ) )
        return;

    POSITION pos = m_lstAi.Find( pPlr, NULL );
    if ( pos != NULL )
        m_lstAi.RemoveAt( pos );
    if ( ( pos = m_lstAll.Find( pPlr, NULL ) ) != NULL )
        m_lstAll.RemoveAt( pos );

    m_lstDead.AddTail( pPlr );
}

void CGame::DeleteAll( )
{

    ASSERT_VALID( this );

    // these will get deleted below
    m_pMe = m_pServer = NULL;

    // delete the elements of m_lstAll
    //   from tail cause of assert
    POSITION pos = m_lstAll.GetTailPosition( );
    while ( pos != NULL )
    {
        CPlayer* pPlr = m_lstAll.GetPrev( pos );
        ASSERT_VALID( pPlr );
        delete pPlr;
    }

    m_lstAll.RemoveAll( );

    // delete dead guys
    pos = m_lstDead.GetTailPosition( );
    while ( pos != NULL )
    {
        CPlayer* pPlr = m_lstDead.GetPrev( pos );
        ASSERT_VALID( pPlr );
    }

    m_lstDead.RemoveAll( );
}

void CGame::CloseAll( )
{

    ASSERT_VALID( this );

    // first remove the AI list (they are all in lstAll too!!!)
    m_lstAi.RemoveAll( );

    DeleteAll( );
    ASSERT_VALID( this );
}

void CGame::Close( )
{

    ASSERT_VALID( this );

    SetShouldProcessMessages(FALSE);

    delete m_pXferFromServer;
    m_pXferFromServer = NULL;

    // close m_lstAll
    CloseAll( );
    ASSERT( m_lstAll.GetCount( ) == 0 );
    ASSERT( m_lstDead.GetCount( ) == 0 );

    theNet.CloseSession( TRUE );

    delete m_pHpRtr;

    // re-init vars
    SetState( close );
    ctor( );
}

void CGame::ProcessAllMessages( )
{

    if ( !m_bMessages )
        return;

    // process all messages so we have none pending
    while ( TRUE )
    {
        EnterCriticalSection( &cs );
        if (m_messagePointerList.GetCount( ) <= 0 )
        {
            LeaveCriticalSection( &cs );
            break;
        }
        char* pBuf = (char*)m_messagePointerList.RemoveHead( );
        if ( pBuf == NULL )
        {
            LeaveCriticalSection( &cs );
            break;
        }
        ProcessMessage((CNetCmd *) pBuf);
        FreeQueueElement((CNetCmd *) pBuf);

        // throttle messages back on if a net game
        if ( ( theGame.IsNetGame( ) ) && (theGame.ShouldPause() ) )
        {
            if (theGame.m_messagePointerList.GetCount( ) <= MIN_NUM_MESSAGES )
            {
                theGame.ClearShouldPause();

                LeaveCriticalSection( &cs );
                CMsgPauseMsg msg( FALSE );
                if ( theGame.AmServer( ) )
                    theGame.PostToAllClients( &msg, sizeof( msg ) );
                else
                    theGame.PostToServer( &msg, sizeof( msg ) );
                EnterCriticalSection( &cs );
            }
        }

        LeaveCriticalSection( &cs );
    }
}

// TRUE if have or others have with me
void CGame::CheckAlliances( )
{

    m_bHaveAlliances = FALSE;
    POSITION pos;
    for ( pos = m_lstAll.GetTailPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = m_lstAll.GetPrev( pos );
        if ( ( !pPlr->IsMe( ) ) && ( ( pPlr->GetRelations( ) == RELATIONS_ALLIANCE ) ||
                                     ( pPlr->GetTheirRelations( ) == RELATIONS_ALLIANCE ) ) )
        {
            m_bHaveAlliances = TRUE;
            return;
        }
    }
}

int CGame::LoadGame( CWnd* pPar, BOOL bReplace )
{
#ifdef LOGGINGON
    OutputDebugStringA( "CGame::LoadGame\n" );
#endif

    EnableAllWindows( NULL, FALSE );

    // Headless harness load (HarnessLoadGame): the .en path is pre-supplied, so skip
    // the file-browser modal entirely (the POSIX SDL2FileBrowser isn't harness-
    // drivable). Mirrors SaveGame's pre-set-filename skip. nullptr for any normal
    // menu-driven load, so that path is unchanged.
    if ( HarnessPendingLoadPath( ) )
    {
        theGame.m_sFileName = HarnessPendingLoadPath( );
    }
    // Use SDL2 file browser if the SDL2 window is active, else fall back to MFC
    else if ( theApp.m_gameWindow )
    {
        SDL2FileBrowser browser( theApp.m_gameWindow.get(), SDL2FileBrowser::Open,
                                 "Load Game", "", "", ".en" );
        if ( browser.DoModal() != 1 || !browser.WasConfirmed() )
        {
            EnableAllWindows( NULL, TRUE );
            return ( IDCANCEL );
        }
        theGame.m_sFileName = browser.GetSelectedPath().c_str();
    }
    else
    {
        std::string sFilters = EnLoadStdString( IDS_SAVE_FILTERS );
        std::string sExt     = EnLoadStdString( IDS_SAVE_EXT );
        CFileDialog dlg( TRUE, sExt.c_str(), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                         sFilters.c_str(), pPar );
        if ( dlg.DoModal( ) != IDOK )
        {
            EnableAllWindows( NULL, TRUE );
            return ( IDCANCEL );
        }
        theGame.m_sFileName = (LPCSTR)dlg.GetPathName( );
    }

    // Extract just the filename for the status message
    std::string sFileTitle = theGame.m_sFileName;
    size_t iSlash = sFileTitle.find_last_of( '\\' );
    if ( iSlash != std::string::npos ) sFileTitle = sFileTitle.substr( iSlash + 1 );

    // put up a message to say we are loading
    theApp.m_pCreateGame->CreateDlgStatus( );
    std::string sText = strPrintf( EnLoadStdString( IDS_LOAD_NAME ).c_str(), sFileTitle.c_str() );
    theApp.m_pCreateGame->GetDlgStatus( )->SetMsg( sText );
    theApp.m_pCreateGame->GetDlgStatus( )->SetPer( 0 );
    theApp.m_pCreateGame->ShowDlgStatus( );

    theApp.BaseYield( );

    try
    {
        theGame.IncTry( );
        // we throw out everything we have
        if ( bReplace )
            theApp.ClearWorld( );
        else
            theApp.NewWorld( );

        theApp.m_pCreateGame->GetDlgStatus( )->SetPer( PER_WORLD_START );
        theApp.m_pCreateGame->GetDlgStatus( )->SetMsg( IDS_LOAD_FILE );

        // game file - read into memory
        std::string sSaveName = theGame.m_sFileName;
        CFile   fil( m_sFileName.c_str(), CFile::modeRead | CFile::shareExclusive | CFile::typeBinary );
        int     iLen = fil.GetLength( );
        char*   pBuf = (char*)malloc( iLen );
        if ( pBuf == NULL )
            ThrowError( ERR_OUT_OF_MEMORY );
        fil.Read( pBuf, iLen );
        fil.Close( );

        // decompress it
        int   iDecompLen;
        void* pDeComp = CoDec::Decompress( pBuf, iLen, iDecompLen );
        free( pBuf );

        // put it in a CMemFile
        CMemFile filMem;
        filMem.Attach( (BYTE*)pDeComp, iDecompLen );

        CArchive ar( &filMem, CArchive::load );
        Serialize( ar );
        ar.Close( );

        filMem.Detach( );
        CoDec::FreeBuf( pDeComp );
        filMem.Close( );
        theGame.m_sFileName = sSaveName;
        theGame.DecTry( );
    }

    catch ( int iNum )
    {
        theGame.DecTry( );
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        CatchNum( iNum );
        theApp.CloseWorld( );
        // LoadGame disabled all windows at entry (EnableAllWindows FALSE); the
        // dialog-cancel paths re-enable, but the load-failure catches forgot to —
        // leaving the main menu dead after e.g. a save version-mismatch. Re-enable.
        EnableAllWindows( NULL, TRUE );
        return ( IDCANCEL );
    }
    catch ( SE_Exception e )
    {
        TRAP( );
        theGame.DecTry( );
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        CatchSE( e );
        theApp.CloseWorld( );
        EnableAllWindows( NULL, TRUE );
        return ( IDCANCEL );
    }
    catch ( ... )
    {
        TRAP( );
        theGame.DecTry( );
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        CatchOther( );
        theApp.CloseWorld( );
        EnableAllWindows( NULL, TRUE );
        return ( IDCANCEL );
    }

    // pick player
    return ( IDOK );
}

int CGame::StartGame( BOOL bReplace )
{
#ifdef LOGGINGON
    OutputDebugStringA( "CGame::StartGame\n" );
#endif

    // set relations based on who we picked
    POSITION pos;
    for ( pos = theGame.m_lstAll.GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.m_lstAll.GetNext( pos );
        if ( pPlr == GetMe( ) )
        {
            pPlr->SetRelations( RELATIONS_ALLIANCE );
            pPlr->SetTheirRelations( RELATIONS_ALLIANCE );
        }
        else
        {
            pPlr->SetRelations( RELATIONS_NEUTRAL );
            pPlr->SetTheirRelations( RELATIONS_NEUTRAL );
        }
    }

    if ( AmServer( ) )
    {
        if ( IsNetGame( ) )
        {
            // drop lstLoad players
            POSITION pos;
            for ( pos = theGame.m_lstLoad.GetTailPosition( ); pos != NULL; )
            {
                TRAP( );
                CPlayer* pPlr = theGame.m_lstLoad.GetPrev( pos );
                if ( theApp.m_pCreateGame != NULL )
                    theApp.m_pCreateGame->RemovePlayer( pPlr );
                theNet.DeletePlayer( pPlr->GetNetNum( ) );
                delete pPlr;
            }
            theGame.m_lstLoad.RemoveAll( );
        }

        // we need to swap out the AI where necessary
        POSITION pos = theGame.GetAll( ).GetHeadPosition( );
        while ( pos != NULL )
        {
            CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
            ASSERT_STRICT_VALID( pPlr );

            // if ! ready -> needs to be AI
            if ( ( pPlr->GetState( ) != CPlayer::ready ) && ( !pPlr->IsAI( ) ) )
                theGame.AiTakeOverPlayer( pPlr, FALSE );
            else
                // if ready -> can't be AI
                if ( ( pPlr->GetState( ) == CPlayer::ready ) && ( pPlr->GetAiHdl( ) != 0 ) )
                    theGame.AiReleasePlayer( pPlr, pPlr->GetNetNum( ), pPlr->GetName( ), pPlr->IsLocal( ), FALSE );
        }

        if ( IsNetGame( ) )
        {
            // give them the final status of each player
            for ( pos = m_lstAll.GetHeadPosition( ); pos != NULL; )
            {
                CPlayer* pPlr = m_lstAll.GetNext( pos );
                ASSERT_VALID( pPlr );
                if ( !pPlr->IsLocal( ) )
                {
                    POSITION posInfo;
                    for ( posInfo = m_lstAll.GetHeadPosition( ); posInfo != NULL; )
                    {
                        CPlayer* pPlrInfo = m_lstAll.GetNext( posInfo );
                        ASSERT_VALID( pPlrInfo );
                        CNetPlayer* pMsg = CNetCmd::AllocPlayer( pPlrInfo );
                        if ( pPlrInfo == pPlr )
                            pMsg->m_bLocal = TRUE;
                        if ( pPlrInfo == theGame.GetServer( ) )
                            pMsg->m_bServer = TRUE;
                        theNet.Send( pPlr->GetNetNum( ), pMsg, pMsg->GetLen( ) );
                        delete pMsg;
                    }
                }
            }

            // tell others to start
            CMsgStartLoadedGame msg;
            PostToAllClients( &msg, sizeof( msg ) );
        }
    }

    // do this here because we needed bAI above for switching
    if ( HaveHP( ) )
    {
        GetMe( )->SetLocal( TRUE );
        m_pHpRtr = new CHPRouter( theGame.GetMe( )->GetPlyrNum( ) );
        m_pHpRtr->Init( );
    }

    ASSERT( TestEverything( ) );

    // we have no remembered spotting of units if multi-player OR we changed who we are
    BOOL bChangeSpotting = FALSE;
    if ( GetScenario( ) == -1 )
        if ( IsNetGame( ) || ( GetMe( )->GetPlyrNum( ) != m_iSavedPlyrNum ) )
        {
            bChangeSpotting = TRUE;
            // all roads and city tiles are invisible
            int   iNum   = theMap.Get_eX( ) * theMap.Get_eY( ) - 2;
            CHex* pHexOn = theMap._GetHex( 0, 0 ) + 1;
            while ( iNum-- )
            {
                if ( ( pHexOn->GetVisibleType( ) == CHex::city ) || ( pHexOn->GetVisibleType( ) == CHex::road ) )
                {
                    BOOL bIsCoast;
                    if ( ( pHexOn - 1 )->IsWater( ) || ( pHexOn + 1 )->IsWater( ) )
                        bIsCoast = TRUE;
                    else
                    {
                        CHexCoord _hex( pHexOn->GetHex( ) );
                        if ( theMap.GetHex( _hex.X( ), _hex.Y( ) - 1 )->IsWater( ) ||
                             theMap.GetHex( _hex.X( ), _hex.Y( ) + 1 )->IsWater( ) )
                            bIsCoast = TRUE;
                        else
                            bIsCoast = FALSE;
                    }

                    if ( bIsCoast )
                    {
                        pHexOn->SetVisibleType( CHex::coastline );
                        pHexOn->m_psprite = theTerrain.GetSprite( CHex::coastline, CHex::island );
                    }
                    else
                    {
                        int iType = ( pHexOn - 1 )->GetVisibleType( );
                        pHexOn->SetVisibleType( iType );
                        int iNum          = theTerrain.GetCount( iType );
                        pHexOn->m_psprite = theTerrain.GetSprite( iType, iNum <= 1 ? 0 : RandNum( iNum - 1 ) );
                    }
                }

                // farm plots are "hidden until seen" like roads: the save carries the
                // SAVER's view, where their own plots are always painted. Re-disguise
                // as the underlying soil (mirrors CFarmBuilding::RevertFields); the
                // farm's next BuildFarm tick repaints any plot we can currently see.
                else if ( pHexOn->GetVisibleType( ) == CHex::fields )
                {
                    int iSoil = pHexOn->GetType( );
                    pHexOn->SetVisibleType( iSoil );
                    pHexOn->SetGrowStage( 0 );
                    CHexCoord _hex( pHexOn->GetHex( ) );
                    int iCount        = theTerrain.GetCount( iSoil );
                    pHexOn->m_psprite = theTerrain.GetSprite(
                        iSoil, iCount <= 1 ? 0 : ( ( _hex.X( ) * 2 + _hex.Y( ) ) % iCount ) );
                }
                pHexOn++;
            }
        }

    // we now need to update some data now that players are set
    pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        ASSERT_VALID( pBldg );
        if ( bChangeSpotting )
        {
            pBldg->SetConstPer( );
            pBldg->m_iVisible = pBldg->m_pOwner->IsMe( ) ? 1 : 0;
        }
        pBldg->FixForPlayer( );
    }
    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        ASSERT_VALID( pVeh );
        pVeh->FixForPlayer( );
    }
    ASSERT( TestEverything( ) );

    // create path manager
    if ( !thePathMgr.Init( theMap.Get_eX( ), theMap.Get_eY( ) ) )
    {
        EnableAllWindows( NULL, TRUE );
        theApp.CloseWorld( );
        return ( IDCANCEL );
    }

    // center on our rocket
    m_maploc = CMapLoc( 0, 0 );
    pos      = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        ASSERT_STRICT_VALID( pBldg );
        if ( pBldg->GetOwner( )->IsMe( ) )
        {
            m_maploc = pBldg->GetWorldPixels( );
            if ( pBldg->GetData( )->GetType( ) == CStructureData::rocket )
                break;
        }
    }

    if ( bReplace )
    {
        // put it where it was
        CWndArea* pWnd = theAreaList.BringToTop( );
        if ( pWnd != NULL )
        {
            pWnd->GetAA( ).Set( m_maploc, m_iDir, m_iZoom );
            pWnd->CheckZoomBtns( );
        }
    }
    else
    {
        theApp.m_wndBar.Create( );  // first to set row3
        if ( theGame.IsNetGame( ) && !theApp.m_gameWindow )
            theApp.m_wndChat.Create( );
        // CDlgResearch removed (Phase 2d) — SDL2ResearchDialog is modal.

        // Player load game?

#ifdef LOGGINGON
        OutputDebugStringA( "Creating CWndArea and setup\n" );
#endif

        CWndArea* pWndArea = new CWndArea( );
        pWndArea->Create( m_maploc, NULL, FALSE );
        pWndArea->SetupDone( );
        pWndArea->GetAA( ).Set( m_maploc, m_iDir, m_iZoom );
        pWndArea->CheckZoomBtns( );

        // but it was already done, maybe? ISSUE? crash?
        // 
        // create world for loaded game
        // because only do it if its null?
        if ( theApp.m_wndWorld.m_hWnd == NULL )
        {
            theApp.m_wndWorld.Create( );  // world must come after area
        }

        pWndArea->SetFocus( );

        // Make MFC main window transparent, attach others as SDL panels
        if ( theApp.m_gameWindow )
        {
            ::SetWindowLong( theApp.m_wndMain.m_hWnd, GWL_EXSTYLE,
                ::GetWindowLong( theApp.m_wndMain.m_hWnd, GWL_EXSTYLE ) | WS_EX_LAYERED );
            ::SetLayeredWindowAttributes( theApp.m_wndMain.m_hWnd, 0, 0, LWA_ALPHA );

            // Vehicle/building lists are native SDL2 now — just hide MFC
            auto hideW = []( auto& w ) {
                if ( !w.m_hWnd ) return;
                ::SetWindowLong( w.m_hWnd, GWL_EXSTYLE,
                    ::GetWindowLong( w.m_hWnd, GWL_EXSTYLE ) | WS_EX_LAYERED );
                ::SetLayeredWindowAttributes( w.m_hWnd, 0, 0, LWA_ALPHA );
            };
            hideW( theApp.m_wndVehicles );
            hideW( theApp.m_wndBldgs );
            theApp.m_gameWindow->Raise();
        }
    }
    EnableAllWindows( NULL, TRUE );

    // now we start each thread
    if ( AmServer( ) )
    {
        int iNum = theGame.GetAi( ).GetCount( );
        int iOn  = 0;
        for ( pos = theGame.GetAi( ).GetHeadPosition( ); pos != NULL; )
        {
            CPlayer* pPlr = theGame.GetAi( ).GetNext( pos );
            ASSERT_VALID( pPlr );
            ASSERT( pPlr->IsAI( ) );
            pPlr->ai.dwHdl = pPlr->GetAiHdl( );
            pPlr->ai.hex   = pPlr->m_hexMapStart;

#ifdef _CHEAT
            if ( EnGetProfileInt( "Debug", "NoThreads", 0 ) == 0 )
#endif
                myStartThread( &( pPlr->ai ), (AFX_THREADPROC)AiThread );

            theApp.m_pCreateGame->GetDlgStatus( )->SetPer( PER_START_AI + ( iOn * PER_NUM_START_AI ) / iNum );
            iOn++;
        }
    }

    // show all visible roads & buildings
    if ( bChangeSpotting )
    {
        int   iNum   = theMap.Get_eX( ) * theMap.Get_eY( );
        CHex* pHexOn = theMap._GetHex( 0, 0 );
        while ( iNum-- )
        {
            if ( pHexOn->GetVisibility( ) )
            {
                switch ( pHexOn->GetType( ) )
                {
                case CHex::city: {
                    CBuilding* pBldg = theBuildingHex.GetBuilding( pHexOn->GetHex( ) );
                    if ( pBldg != NULL )
                        pBldg->MakeBldgVisible( );
                    else
                        pHexOn->SetVisibleType( CHex::city );
                    break;
                }
                case CHex::road:
                    CHexCoord _hex( pHexOn->GetHex( ) );
                    pHexOn->ChangeToRoad( _hex, TRUE, TRUE );
                    break;
                }
            }
            pHexOn++;
        }
    }

    MySrand( m_uSeed );
    theApp.RestartWorld( );

    // display the new world
    theApp.LetsGo( );

    return ( IDOK );
}

static void fnCompSave( DWORD_PTR dwData, int iBlk )
{

    CDlgSaveMsg* pDlg = (CDlgSaveMsg*)dwData;
    pDlg->UpdateData( TRUE );
    std::string sNum = IntToStr( iBlk );
    pDlg->m_sStat = strPrintf( EnLoadStdString( IDS_SAVE_COMPRESS ).c_str(), sNum.c_str() );
    pDlg->UpdateData( FALSE );

    // iBlk is the 1-based compression block index; m_iTotalBlocks was computed
    // from the uncompressed length so we can show a real percentage.
    if ( pDlg->m_iTotalBlocks > 0 )
        pDlg->SetProgress( __min( 100, ( iBlk * 100 ) / pDlg->m_iTotalBlocks ) );

    // needed for MODEM games
    theApp.BaseYield( );
    ::Sleep( 0 );
}

int CGame::SaveGame( CWnd* pPar )
{

    // do we have a CD?
    if ( !CheckForCD( ) )
        return ( IDCANCEL );

    // can't save if haven't landed
    CWndArea* pWnd = theAreaList.GetTop( );
    if ( pWnd == NULL )
        return ( IDCANCEL );

    if ( ( pWnd->GetMode( ) == CWndArea::rocket_ready ) || ( pWnd->GetMode( ) == CWndArea::rocket_pos ) ||
         ( pWnd->GetMode( ) == CWndArea::rocket_wait ) )
        return ( IDCANCEL );

    ASSERT( ( theAreaList.GetTop( )->GetMode( ) != CWndArea::rocket_ready ) &&
            ( theAreaList.GetTop( )->GetMode( ) != CWndArea::rocket_pos ) );
    ASSERT( TestEverything( ) );

    // If SDL2 window is active and filename is pre-set, skip file dialog
    if ( theApp.m_gameWindow && !m_sFileName.empty() )
    {
        // Filename already chosen by SDL2FileBrowser — proceed directly
    }
    else if ( theApp.m_gameWindow )
    {
        // Use SDL2 file browser
        std::string defaultName = m_sFileName;
        size_t lastSlash = defaultName.find_last_of("\\/");
        if (lastSlash != std::string::npos)
            defaultName = defaultName.substr(lastSlash + 1);
        if (defaultName.empty()) defaultName = "savegame";

        SDL2FileBrowser browser( theApp.m_gameWindow.get(), SDL2FileBrowser::Save,
                                 "Save Game", "", defaultName, ".en" );
        EnableAllWindows( NULL, FALSE );
        int iRtn = browser.DoModal();
        if ( iRtn != 1 || !browser.WasConfirmed() )
        {
            EnableAllWindows( NULL, TRUE );
            return ( IDCANCEL );
        }
        m_sFileName = browser.GetSelectedPath().c_str();
    }
    else
    {
        std::string sFilters = EnLoadStdString( IDS_SAVE_FILTERS );
        std::string sExt     = EnLoadStdString( IDS_SAVE_EXT );

        char const* pName;
        if ( m_sFileName.empty( ) )
            pName = "";
        else
            pName = m_sFileName.c_str();

        CFileDialog dlg( FALSE, sExt.c_str(), pName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                         sFilters.c_str(), pPar );
        EnableAllWindows( NULL, FALSE );
        int iRtn = dlg.DoModal( );
        if ( iRtn != IDOK )
        {
            EnableAllWindows( NULL, TRUE );
            return ( iRtn );
        }

        m_sFileName = (LPCSTR)dlg.GetPathName( );
    }

    // put up a message to say we are saving
    CDlgSaveMsg dlgMsg( pPar );
    dlgMsg.m_sText = strPrintf( EnLoadStdString( IDS_SAVE_NAME ).c_str(),
                                m_sFileName.c_str() );
    if ( IsNetGame( ) )
        dlgMsg.m_sStat = EnLoadStdString( IDS_SAVE_REMOTE );
    else
        dlgMsg.m_sStat = EnLoadStdString( IDS_SAVE_LOCAL );
    dlgMsg.Create( IDD_SAVE_MSG, pPar );

    // disable all other windows
    theApp.m_wndMain._EnableGameWindows( FALSE );

    // we grab the crit sec here not because we need it but because it
    // will cause all AI threads to stop at a clean location so the CPU
    // spends all it's time saving the game. It's ok if an AI keeps running,
    // just slower.
    EnterCriticalSection( &cs );

    // now we ask for updated materials on all players - some may be late
    if ( IsNetGame( ) && HaveHP( ) )
    {
        CNetNeedSaveInfo msg( GetMe( ) );
        theGame.PostToAll( &msg, sizeof( msg ), FALSE );
    }

    // we now need to pause to get the above messages back and to block all
    // the AI threads.
    int iWait = 2 + 2 * ( m_lstAll.GetCount( ) - m_lstAi.GetCount( ) );
    if ( IsNetGame( ) )
        iWait += 10;
    while ( iWait-- )
    {
        ProcessAllMessages( );
        theApp.BaseYield( );
        Sleep( 100 );
    }

    pWnd = theAreaList.GetTop( );
    if ( pWnd != NULL )
    {
        m_maploc = pWnd->GetAA( ).m_maploc;
        m_iDir   = pWnd->GetAA( ).m_iDir;
        m_iZoom  = pWnd->GetAA( ).m_iZoom;
    }
    else
    {
        m_maploc = CMapLoc( 0, 0 );
        m_iDir   = 0;
        m_iZoom  = 0;
    }

    // we save it
    try
    {
        theGame.IncTry( );
        SetState( save );

        // stop processing and clear out final messages
        theGame.SetShouldOperate(FALSE);
        theApp.BaseYield( );
        ProcessAllMessages( );

        dlgMsg.UpdateData( TRUE );
        dlgMsg.m_sStat = EnLoadStdString( IDS_SAVE_DATA );
        dlgMsg.UpdateData( FALSE );

        // CMemFile to save to
        CMemFile fil;

        CArchive ar( &fil, CArchive::store );
        Serialize( ar );
        ar.Close( );

        // compress it
        int   iLen = fil.GetLength( );
        BYTE* pBuf = fil.Detach( );
        int   iCompLen;
        // The GAME codec (BPE) compresses in fixed 5000-byte blocks
        // (BPECoDec::BLOCKSIZE); fnCompSave turns the running block index into a
        // percentage against this total.
        const int kBpeBlockSize = 5000;
        dlgMsg.m_iTotalBlocks = ( iLen + kBpeBlockSize - 1 ) / kBpeBlockSize;
        void* pComp = CoDec::Compress( CoDec::CODEC::GAME, pBuf, iLen, iCompLen, fnCompSave, (DWORD_PTR)&dlgMsg );
        free( pBuf );

        theApp.BaseYield( );

        dlgMsg.UpdateData( TRUE );
        dlgMsg.m_sStat = EnLoadStdString( IDS_SAVE_WRITE );
        dlgMsg.UpdateData( FALSE );

        // write it to disk
        CFile filDest( m_sFileName.c_str(), CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeBinary );
        filDest.Write( pComp, iCompLen );
        CoDec::FreeBuf( pComp );
        filDest.Close( );

        theGame.SetShouldOperate(TRUE);
        LeaveCriticalSection( &cs );
        theApp.m_wndMain._EnableGameWindows( TRUE );

        SetState( play );
        theGame.DecTry( );
    }

    catch ( ... )
    {
        std::string sMsg = strPrintf( EnLoadStdString( IDS_CANT_SAVE ).c_str(),
                                      m_sFileName.c_str() );
        EnMessageBox( sMsg.c_str() );

        theGame.SetShouldOperate(TRUE);
        LeaveCriticalSection( &cs );
        theApp.m_wndMain._EnableGameWindows( TRUE );

        SetState( play );
        theGame.DecTry( );
    }

    EnableAllWindows( NULL, TRUE );
    dlgMsg.DestroyWindow( );

    return ( IDOK );
}

static std::string GetVerText( DWORD dwMaj, DWORD dwMin, DWORD dwVer, WORD wDbg, WORD wCht )
{

    std::string sRtn = IntToStr( dwMaj ) + "." + IntToStr( dwMin ) + "." + IntToStr( dwVer );

    if ( wDbg && wCht )
        sRtn += " (debug, cheat)";
    else if ( wDbg )
        sRtn += " (debug)";
    else if ( wCht )
        sRtn += " (cheat)";

    return ( sRtn );
}

// iMode 0 - reg
//       1 - dialog
//       2 - not smaller
void CGame::ReadWP( CArchive& ar, WINDOWPLACEMENT& wp, int iMode )
{

    ar.Read( &wp, sizeof( WINDOWPLACEMENT ) );

    // adjust
    wp.ptMinPosition.x = ( wp.ptMinPosition.x * theApp.m_iScrnX ) / m_xScreen;
    wp.ptMinPosition.y = ( wp.ptMinPosition.y * theApp.m_iScrnY ) / m_yScreen;
    wp.ptMaxPosition.x = ( wp.ptMaxPosition.x * theApp.m_iScrnX ) / m_xScreen;
    wp.ptMaxPosition.y = ( wp.ptMaxPosition.y * theApp.m_iScrnY ) / m_yScreen;
    if ( iMode == 0 )
    {
        wp.rcNormalPosition.left   = ( wp.rcNormalPosition.left * theApp.m_iScrnX ) / m_xScreen;
        wp.rcNormalPosition.top    = ( wp.rcNormalPosition.top * theApp.m_iScrnY ) / m_yScreen;
        wp.rcNormalPosition.right  = ( wp.rcNormalPosition.right * theApp.m_iScrnX ) / m_xScreen;
        wp.rcNormalPosition.bottom = ( wp.rcNormalPosition.bottom * theApp.m_iScrnY ) / m_yScreen;
    }
    else if ( iMode == 2 )
        OffsetRect( &( wp.rcNormalPosition ),
                    wp.rcNormalPosition.left - ( wp.rcNormalPosition.left * theApp.m_iScrnX ) / m_xScreen,
                    wp.rcNormalPosition.top - ( wp.rcNormalPosition.top * theApp.m_iScrnY ) / m_yScreen );
}

void CGame::Serialize( CArchive& ar )
{

    ASSERT( this == &theGame );

    if ( ar.IsStoring( ) )
    {
        ASSERT_VALID( this );
        ASSERT( TestEverything( ) );

        m_dwMaj = VER_MAJOR;
        m_dwMin = VER_MINOR;
        m_dwVer = VER_RELEASE;
        m_wDbg  = _wDebug;
        m_wCht  = _wCheat;

        // version, cheats, & debug
        ar << m_dwMaj << m_dwMin << m_dwVer << m_wDbg << m_wCht;
        ar << m_sFileName;

        ar << m_xScreen << m_yScreen;
        ar << m_iAi;
        ar << m_iSize;
        ar << m_iPos;
        ar << m_sGameName;
        ar << m_sGameDesc;
        ar << m_iScenarioVar;
        for ( int iInd = 0; iInd < 5; iInd++ ) ar << m_adwScenarioUnits[iInd];

        ar << m_dwOperTimeLast << m_dwFrameTimeLast << m_dwFramesElapsed << m_dwOpersElapsed;
        ar << m_dwOperSecElapsed << m_dwOperSecFrames << m_dwElapsedTime << m_dwFrame;
        ar << m_iSideSize;
        ar << m_iNextPlyrNum;
        ar << m_iNextAINum;
        ar << m_dwNextUnitID;
        ar << (WORD)m_bServer;
        ar << m_iScenarioNum;
        ar << m_iSpeedMul;

        ar << m_bHaveAlliances;
        ar << m_uSeed;

        // window positions
        ar << m_hexAreaCenter;
        ar.Write( &m_wpArea, sizeof( WINDOWPLACEMENT ) );
        ar.Write( &m_wpWorld, sizeof( WINDOWPLACEMENT ) );
        ar.Write( &m_wpChat, sizeof( WINDOWPLACEMENT ) );
        ar.Write( &m_wpBldgs, sizeof( WINDOWPLACEMENT ) );
        ar.Write( &m_wpVehicles, sizeof( WINDOWPLACEMENT ) );
        ar.Write( &m_wpRelations, sizeof( WINDOWPLACEMENT ) );
        ar.Write( &m_wpFile, sizeof( WINDOWPLACEMENT ) );
        ar.Write( &m_wpRsrch, sizeof( WINDOWPLACEMENT ) );

        CWndArea* pWnd = theAreaList.GetTop( );
        if ( pWnd == NULL )
        {
            TRAP( );
            m_iZoom = max( 1, theApp.GetZoomData( )->GetFirstZoom( ) );
            m_iDir  = 0;
        }
        else
        {
            m_iZoom = pWnd->GetAA( ).m_iZoom;
            m_iDir  = pWnd->GetAA( ).m_iDir;
        }
        ar << m_maploc;
        ar << (BYTE)m_iDir;
        ar << (BYTE)m_iZoom;
        ar << (BYTE)m_iNetJoin;

        // the players
        ar << (WORD)m_lstAll.GetCount( );
        POSITION pos;
        for ( pos = m_lstAll.GetHeadPosition( ); pos != NULL; )
        {
            CPlayer* pPlr = m_lstAll.GetNext( pos );
            pPlr->Serialize( ar );
        }
        ar << (WORD)m_pMe->GetPlyrNum( );
        ar << (WORD)m_pServer->GetPlyrNum( );

        // the dead players
        ar << (WORD)m_lstDead.GetCount( );
        for ( pos = m_lstDead.GetHeadPosition( ); pos != NULL; )
        {
            CPlayer* pPlr = m_lstDead.GetNext( pos );
            pPlr->Serialize( ar );
        }

        // save the map
        theMap.Serialize( ar );
        theMinerals.Serialize( ar );

        // TEMP DEBUG: mineral flag/map round-trip diagnosis (store)
        {
            int flagged = 0;
            for ( int yy = 0; yy < theMap.Get_eY( ); yy++ )
                for ( int xx = 0; xx < theMap.Get_eX( ); xx++ )
                    if ( theMap._GetHex( xx, yy )->GetUnits( ) & CHex::minerals ) flagged++;
            char b[160];
            sprintf_s( b, sizeof( b ), "[SAVE store] minerals=%d flaggedHexes=%d eX=%d eY=%d\n",
                       (int)theMinerals.GetCount( ), flagged, theMap.Get_eX( ), theMap.Get_eY( ) );
            OutputDebugStringA( b );
            FILE* f = NULL; if ( fopen_s( &f, "d:\\tmp\\worlddbg.log", "a" ) == 0 && f ) { fputs( b, f ); fclose( f ); }
        }

        // save the bridges
        theBridgeMap.Serialize( ar );

        // save the units
        ar << (WORD)theBuildingMap.GetCount( );
        pos = theBuildingMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD      dwID;
            CBuilding* pBldg;
            theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
            ar << (WORD)pBldg->GetData( )->GetType( ) << (BYTE)pBldg->GetDir( )
               << (WORD)pBldg->GetOwner( )->GetPlyrNum( ) << (DWORD)pBldg->GetID( );
            pBldg->Serialize( ar );
        }
        ar << (WORD)theVehicleMap.GetCount( );
        pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            ar << (LONG)pVeh->GetData( )->GetType( ) << (WORD)pVeh->GetOwner( )->GetPlyrNum( ) << (DWORD)pVeh->GetID( );
            pVeh->Serialize( ar );
        }

        AiSaveGame( ar );
    }

    else
    {
        theApp.Log( "Loading CGame" );
        // these all need to be set before we start
        ASSERT_VALID( &theTerrain );
        ASSERT_VALID( &theStructures );
        ASSERT_VALID( &theTransports );
        ASSERT_VALID( &theBuildingHex );
        ASSERT_VALID( &theVehicleHex );
        ASSERT_VALID( &theStructureType );

        // version, cheats, & debug
        ar >> m_dwMaj >> m_dwMin >> m_dwVer >> m_wDbg >> m_wCht;
        if ( EnGetProfileInt( "Cheat", "DiffVer", 1 ) )
        {
            //			if ((m_dwMaj != VER_MAJOR) || (m_dwMin != VER_MINOR) ||

            BOOL wrongMajorVersion     = ( m_dwMaj != VER_MAJOR );
            BOOL wrongMinorVersion     = ( m_dwMin != VER_MINOR );
            BOOL debugCheatMissmatched = ( m_wDbg != _wDebug ) || ( m_wCht != _wCheat );

            if ( wrongMajorVersion || wrongMinorVersion || debugCheatMissmatched )
            {
                std::string sVer1 = GetVerText( m_dwMaj, m_dwMin, m_dwVer, m_wDbg, m_wCht );
                std::string sVer2 = GetVerText( VER_MAJOR, VER_MINOR, VER_RELEASE, _wDebug, _wCheat );
                std::string sMsg = strPrintf( EnLoadStdString( IDS_SAVE_VER ).c_str(),
                                              sVer1.c_str( ), sVer2.c_str( ) );
                EnMessageBox( sMsg.c_str() );
                ThrowError( ERR_RES_CREATE_WND );
            }
        }

        ar >> m_sFileName;
        ar >> m_xScreen >> m_yScreen;

        ar >> m_iAi;
        ar >> m_iSize;
        ar >> m_iPos;
        ar >> m_sGameName;
        ar >> m_sGameDesc;
        ar >> m_iScenarioVar;
        for ( int iInd = 0; iInd < 5; iInd++ ) ar >> m_adwScenarioUnits[iInd];

        ar >> m_dwOperTimeLast >> m_dwFrameTimeLast >> m_dwFramesElapsed >> m_dwOpersElapsed;
        ar >> m_dwOperSecElapsed >> m_dwOperSecFrames >> m_dwElapsedTime >> m_dwFrame;
        ar >> m_iSideSize;
        ar >> m_iNextPlyrNum;
        ar >> m_iNextAINum;
        ar >> m_dwNextUnitID;
        WORD w;
        ar >> w;
        m_bServer = TRUE;
        ar >> m_iScenarioNum;
        ar >> m_iSpeedMul;

        ar >> m_bHaveAlliances;
        ar >> m_uSeed;

        // window positions - adjust to this screen's resolution
        ar >> m_hexAreaCenter;
        ReadWP( ar, m_wpArea, 0 );
        ReadWP( ar, m_wpWorld, 0 );
        ReadWP( ar, m_wpChat, 2 );
        ReadWP( ar, m_wpBldgs, 2 );
        ReadWP( ar, m_wpVehicles, 2 );
        ReadWP( ar, m_wpRelations, 1 );
        ReadWP( ar, m_wpFile, 1 );
        ReadWP( ar, m_wpRsrch, 1 );

        ar >> m_maploc;
        BYTE b;
        ar >> b;
        m_iDir = b;
        ar >> b;
        m_iZoom = b;
        ar >> b;
        m_iNetJoin = b;

        // the players
        WORD wCount;
        ar >> wCount;
        while ( wCount-- )
        {
            CPlayer* pPlr = new CPlayer( );
            pPlr->Serialize( ar );
            m_lstAll.AddTail( pPlr );
            if ( pPlr->IsAI( ) )
                m_lstAi.AddTail( pPlr );
            m_iNextPlyrNum = __max( m_iNextPlyrNum, pPlr->GetPlyrNum( ) + 1 );
        }
        ar >> wCount;
        m_pMe           = GetPlayerByPlyr( wCount );
        m_iSavedPlyrNum = wCount;
        ar >> wCount;
        m_pServer = GetPlayerByPlyr( wCount );

        ar >> wCount;
        while ( wCount-- )
        {
            CPlayer* pPlr = new CPlayer( );
            pPlr->Serialize( ar );
            pPlr->SetState( CPlayer::dead );
            m_iNextPlyrNum = __max( m_iNextPlyrNum, pPlr->GetPlyrNum( ) + 1 );
            m_lstDead.AddTail( pPlr );
        }

        ASSERT_VALID( this );

        // load the map
        theMap.Serialize( ar );
        ASSERT_VALID( &theMap );
        theMinerals.Serialize( ar );
        ASSERT_VALID( &theMinerals );

        // TEMP DEBUG: mineral flag/map round-trip diagnosis (load)
        {
            int flagged = 0;
            for ( int yy = 0; yy < theMap.Get_eY( ); yy++ )
                for ( int xx = 0; xx < theMap.Get_eX( ); xx++ )
                    if ( theMap._GetHex( xx, yy )->GetUnits( ) & CHex::minerals ) flagged++;
            char b[160];
            sprintf_s( b, sizeof( b ), "[LOAD] minerals=%d flaggedHexes=%d eX=%d eY=%d\n",
                       (int)theMinerals.GetCount( ), flagged, theMap.Get_eX( ), theMap.Get_eY( ) );
            OutputDebugStringA( b );
            FILE* f = NULL; if ( fopen_s( &f, "d:\\tmp\\worlddbg.log", "a" ) == 0 && f ) { fputs( b, f ); fclose( f ); }
        }

        // load the bridges
        theBridgeMap.Serialize( ar );
        ASSERT_VALID( &theBridgeMap );

        // load the units
        ar >> wCount;
        while ( wCount-- )
        {
            WORD  wBldg;
            BYTE  bDir;
            WORD  wOwner;
            DWORD dwID;
            ar >> wBldg >> bDir >> wOwner >> dwID;
            CBuilding* pBldg = CBuilding::Alloc( wBldg, bDir, wOwner, dwID );
            pBldg->Serialize( ar );
        }
        ar >> wCount;
        while ( wCount-- )
        {
            LONG  lVeh;
            WORD  wOwner;
            DWORD dwID;
            ar >> lVeh >> wOwner >> dwID;
            CVehicle* pVeh = new CVehicle( lVeh, wOwner, dwID );
            pVeh->Serialize( ar );
        }

        // we now need to update some data now that everything is loaded
        POSITION pos = theBuildingMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD      dwID;
            CBuilding* pBldg;
            theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
            ASSERT_VALID( pBldg );
            pBldg->FixUp( );
        }
        pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            ASSERT_VALID( pVeh );
            pVeh->FixUp( );
        }

        // show buildings for scenarios
        if ( ( m_iScenarioNum == 4 ) || ( m_iScenarioNum == 5 ) )
            for ( int iInd = 0; iInd < 5; iInd++ )
            {
                CBuilding* pBldg = theBuildingMap.GetBldg( m_adwScenarioUnits[iInd] );
                if ( pBldg != NULL )
                    ShowBuilding( iInd, pBldg );
            }

        // see if can have R&D
        if ( HaveHP( ) )
            theApp.m_wndBar.CheckButtons( );

        // test everything
        ASSERT( TestEverything( ) );

        // if not the server then loading over the net
        BOOL bLocal =
            ( ( theApp.m_pCreateGame == NULL ) || ( theApp.m_pCreateGame->m_iTyp != CCreateBase::load_join ) );
        AiLoadGame( ar, bLocal );
        if ( bLocal )
            AiLoadComplete( );

        // set to our screen res
        m_xScreen = theApp.m_iScrnX;
        m_yScreen = theApp.m_iScrnY;
        theApp.Log( "CGame loaded" );
    }
}

#ifdef _DEBUG
void CGame::AssertValid( ) const
{
#ifdef LOGGINGON
  //  OutputDebugStringA( "CGame::AssertValid\n" );
#endif

    CObject::AssertValid( );

#ifdef LOGGINGON
   // OutputDebugStringA( "CGame::AssertValid:ASSERT_VALID_OR_NULL\n" );
#endif


    ASSERT_VALID_OR_NULL( m_pMe );
    ASSERT_VALID_OR_NULL( m_pServer );
    ASSERT_VALID( &m_lstAll );
    ASSERT_VALID( &m_lstAi );
    ASSERT_VALID( &m_lstDead );

    if ( m_pServer )
    {
        if ( m_bServer )
            ASSERT(m_pServer == m_pMe);
        else
            ASSERT(m_pServer->m_iNetNum != m_pMe->m_iNetNum);
    }
}
#endif // _DEBUG (CGame::AssertValid)

// TestEverything() is NOT Debug-only: the port's ASSERT (en_assert.h) evaluates
// its argument in Release as well, so every ASSERT( TestEverything() ) site needs
// this defined in all configs. The body's ASSERT_VALID(...) checks compile to
// no-ops in Release; the plain ASSERT(...) invariants stay live (and logged).
BOOL TestEverything( )
{
    // THROTTLE (2026-06-10): this is an O(every unit + every hex-ownership)
    // validation sweep, and the HP router ASSERTs it on EVERY routed message
    // (6 sites in chproute.cpp), including synchronously from the building
    // operate pass (BuildMaterials -> MsgOutMat). In a large game (~700 bldgs
    // + ~1700 vehs) an out-of-materials message storm turned this into
    // seconds of main-thread validation per second -- fps fell to ~0.5 with
    // sim=1.3-2.6 s/s, and dbgstack showed the operate pass parked under
    // CBuilding::AssertValid -> CBuildingHex::GetBuilding -> CMap::Lookup.
    // Run the full sweep at most once per 2s; callers ASSERT the return
    // value, so the skip must return TRUE. All call sites are main-thread
    // (HP router, init, cutscene), so a plain static is safe.
    static DWORD s_dwLastSweep = 0;
    DWORD        dwNowSweep    = timeGetTime( );
    if ( s_dwLastSweep != 0 && ( dwNowSweep - s_dwLastSweep ) < 2000 )
        return TRUE;
    s_dwLastSweep = dwNowSweep;

    ASSERT_VALID( &theTerrain );
    ASSERT_VALID( &theStructures );
    ASSERT_VALID( &theTransports );
    ASSERT_VALID( &theStructureType );
    ASSERT_VALID( &theMap );

    ASSERT_VALID( &theMinerals );

    POSITION pos = theMinerals.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CMinerals* pMn;
        theMinerals.GetNextAssoc( pos, dwID, pMn );

        // VTFIXME: this was failing on game load?
        ASSERT_VALID( pMn ); // I think this is 100% nessasary
    }

    ASSERT_VALID( &theGame );

    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
        ASSERT_VALID( pPlr );
    }

    ASSERT_VALID( &theBuildingHex );
    ASSERT_VALID( &theVehicleHex );
    ASSERT_VALID( &theBuildingMap );
    ASSERT_VALID( &theVehicleMap );

    // test the units
    pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        ASSERT_VALID( pBldg );
    }

    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        ASSERT_VALID( pVeh );
    }

    // test the ownership
    pos = theBuildingHex.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwLoc;
        CBuilding* pBldg;
        theBuildingHex.GetNextAssoc( pos, dwLoc, pBldg );
        ASSERT_VALID( pBldg );
        CHexCoord hex( dwLoc >> 16, dwLoc & 0xFFFF );

        for ( int y = 0; y < pBldg->GetCY( ); y++ )
            for ( int x = 0; x < pBldg->GetCX( ); x++ )
            {
                CHexCoord _hex( pBldg->GetHex( ).X( ) + x, pBldg->GetHex( ).Y( ) + y );
                _hex.Wrap( );
                if ( _hex == hex )
                    goto got_it;
            }
        ASSERT( FALSE );
    got_it:;
    }

    pos = theVehicleHex.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwLoc;
        CVehicle* pVeh;
        theVehicleHex.GetNextAssoc( pos, dwLoc, pVeh );
        ASSERT_VALID( pVeh );
        CSubHex sub( dwLoc >> 16, dwLoc & 0xFFFF );
        if ( pVeh->GetHexOwnership( ) )
            ASSERT( ( pVeh->GetHexOwnership( ) ) && ( ( sub == pVeh->GetPtHead( ) ) || ( sub == pVeh->GetPtTail( ) ) ||
                                                      ( sub == pVeh->GetPtNext( ) ) ) );
    }

    return ( TRUE );
}
