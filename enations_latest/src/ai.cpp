////////////////////////////////////////////////////////////////////////////
//
//  ai.cpp :  AI interface implementation
//            Divide and Conquer AI
//
//  Last update:    10/30/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "ai.h"

#include "aisnap.h"  // Tier-B AI world snapshot (cleared on game exit)
#include "caidata.hpp"
#include "caimgr.hpp"
#include "caisavld.hpp"
#include "cpathmap.h"
#include "lastplnt.h"
#include "player.h"
#include "racedata.h"
#include "stdafx.h"
#include "threads.h"

extern CRITICAL_SECTION cs;
extern CPathMap         thePathMap;  // the map pathfinding object (no yield)

// standard exception for yielding
CException* pException = NULL;

CAITaskList* plTaskList = NULL;  // standard CAITask list
CAIGoalList* plGoalList = NULL;  // standard CAIGoal list

CWordArray* pwaIG = NULL;  // initial goals

// external global reference for game data interface
CAIData* pGameData = NULL;

// the AI Manager List, a list of CAIMgr objects
// which is the central point for AI access by the game
CAIMgrList* plAIMgrList = NULL;


static int iPlyrNum = 1;

void AiCityCenter( CHexCoord& /* _hex */ )
{
}

// called by server each time a topic completes
int AiNextRsrch( CPlayer* pPlyr, int /*iCompleted*/ )
{
    CAIMgr* pAIMgr = (CAIMgr*)pPlyr->GetAiHdl( );
    if ( pAIMgr == NULL )
        return ( 0 );

    return ( pAIMgr->NextTopic( pPlyr ) );
}

//
// a human player has droped out of a multi-player game
// so now the AI must take over and manage in the HP's place
//
BOOL AiTakeOverPlayer( CPlayer* pPlr )
{
    // create an CAIMgr for the new player
    if ( AiNewPlayer( pPlr ) )
        return ( FALSE );

    int iID = pPlr->GetPlyrNum( );

    CAIMgr* pAIMgr = plAIMgrList->GetManager( iID );
    if ( pAIMgr == NULL )
        return ( FALSE );

    // get the initial location of the HP Player
    int iX = pPlr->m_hexMapStart.X( );
    int iY = pPlr->m_hexMapStart.Y( );

    // initialize AI units
    // initialize AI maps
    // initialize AI managers
    pAIMgr->AssumeControl( iX, iY );

    return ( TRUE );
}

// This is the first call to the AI when creating a game
// iSmart == 0 (Easy to beat) - 3 (Impossible) - how smart the AI is
//   You also get a multiplier on initial points of .8, .95, 1.05, 1.2 (this may
//   change)
// iNumAi - number of AI players in game
// iNumHuman - number of human players in the game
// return TRUE if you can't start.
BOOL AiInit( int iSmart, int iNumAi, int iNumHuman, int iStartPos )
{
    if ( iSmart < 0 || iNumAi < 0 || iNumHuman < 0 || iStartPos < 0 )
        return TRUE;

    if ( pGameData != NULL )
        delete pGameData;

    // attempt to create the game data interface object
    try
    {
        // BUGBUG the CAIData constructor should be changed
        // as int iHexPerBlk and int iBlkPerSide are not
        // being passed and are not available now
        pGameData              = new CAIData( iSmart, iNumAi, iNumHuman );
        pGameData->m_iStartPos = iStartPos;
    }
    catch ( CException* e )
    {
        if ( e != NULL )
        {
            e->Delete( );
            e = NULL;
        }

        if ( pGameData != NULL )
        {
            delete pGameData;
            pGameData = NULL;
        }

        return TRUE;
    }

    if ( plAIMgrList != NULL )
        delete plAIMgrList;

    // attempt to create an CAIMgr list
    try
    {
        plAIMgrList = new CAIMgrList( );
    }
    catch ( CException* e )
    {
        if ( plAIMgrList != NULL )
        {
            delete plAIMgrList;
            plAIMgrList = NULL;
        }

        if ( pGameData != NULL )
        {
            delete pGameData;
            pGameData = NULL;
        }

        if ( e != NULL )
        {
            e->Delete( );
            e = NULL;
        }

        return TRUE;
    }

    iPlyrNum = 1;
    return FALSE;
}

// This call is made once for each AI player to create it.
// m_iPlyrNum is set on entry
// m_attrib must be set before exit
// m_AiHdl must be set before exit
// return TRUE if you can't start.
BOOL AiNewPlayer( CPlayer* pPlr )
{
    if ( plAIMgrList == NULL )
        return TRUE;

    ASSERT_VALID( plAIMgrList );

    // BUGBUG will the CPlayer have valid data by this time?  Yes!
    int iID = pPlr->GetPlyrNum( );

    CAIMgr* pAIMgr = plAIMgrList->GetManager( iID );
    if ( pAIMgr == NULL )
    {
        try
        {
            // CAIMgr( int iPlayer )
            pAIMgr = new CAIMgr( iID );
            plAIMgrList->AddTail( (CObject*)pAIMgr );
        }
        catch ( CException* e )
        {
            if ( pAIMgr != NULL )
                delete pAIMgr;

            return TRUE;
        }
    }

    // make pointer to this CAIMgr available to the game
    pPlr->SetAiHdl( (DWORD_PTR)pAIMgr );

    // BUGBUG consider if the player is human and flag CAIMgr
    pAIMgr->SetAI( pPlr->IsAI( ) );

    // considering pGameData->m_iSmart, make adjustments to
    // CPlayer attribs to reflect "dumbness"
    // make legal adjustments to player racial characteristics
    pAIMgr->AdjustAttribs( pPlr );

    return ( FALSE );
}

// Sum the live CCell scratch across every AI's per-AI path map. Diagnostic only
// (EN_PERF "path.cells" gauge): AI pathing moved off the global thePathMap to
// per-AI instances, so the global's count no longer reflects AI activity. Called
// once/sec on the main thread; the per-AI size() reads are a benign race (an
// aligned word read returns old-or-new, never torn) and only run while profiling.
int AiTotalPathCells( )
{
    if ( plAIMgrList == NULL )
        return 0;
    int iTotal = 0;
    POSITION pos = plAIMgrList->GetHeadPosition( );
    while ( pos != NULL )
    {
        CAIMgr* pMgr = (CAIMgr*)plAIMgrList->GetNext( pos );
        if ( pMgr != NULL )
            iTotal += pMgr->GetPathMapCellCount( );
    }
    return iTotal;
}

// iHexPerBlk - the number of hex's a block is wide or high (a block is ALWAYS
// square).
//   This is presently 64, 128, or 256
// iBlkPerSide - the number of blocks a map is wide or high (the map is always
// square).
//   Therefore the number of hexes wide (or high) a map is is iHexPerBlk *
//   iBlkPerSide
BOOL AiWorldSize( int iHexPerBlk, int iBlkPerSide )
{
    if ( iHexPerBlk < 0 || iBlkPerSide < 0 )
        return TRUE;

    ASSERT_VALID( pGameData );

    // handle global game data object and exception for yielding
    if ( pGameData == NULL )
        pGameData = new CAIData( 0, 0, 0 );

    // BUGBUG this is a different way to set these values
    // than anticipated.  See AiThread()
    pGameData->SetWorldSize( iHexPerBlk, iBlkPerSide );
    return ( FALSE );
}

// Serial (RNG-sensitive) part: PickStartHex draws from the shared game RNG, so
// this must run in deterministic player order on the main thread.
void AiSetupPre( CPlayer* pPlr )
{
    CAIMgr* pAIMgr = (CAIMgr*)pPlr->GetAiHdl( );
    if ( pAIMgr == NULL )
        return;

    // by this time, the pAiI->hex should have
    // the initial location of the AI Player
    int iX = pPlr->m_hexMapStart.X( );
    int iY = pPlr->m_hexMapStart.Y( );

    try
    {
        pAIMgr->CreatePre( iX, iY );
    }
    catch ( CException* e )
    {
        throw( ERR_CAI_BAD_NEW );
    }
}

// Heavy part: builds this AI's private map + managers and places its initial
// units. Must follow AiSetupPre. CreateHeavy's CAIMap build copies the shared
// base map (see AiHexCacheBuild) rather than re-scanning the locked game map.
void AiSetupHeavy( CPlayer* pPlr )
{
    CAIMgr* pAIMgr = (CAIMgr*)pPlr->GetAiHdl( );
    if ( pAIMgr == NULL )
        return;

    // now the pAIMgr must create its CAIMap map, and
    // CAIUnits list and CAIGoalMgr and CAITaskMgr objects
    try
    {
        pAIMgr->CreateHeavy( );
        theApp.BaseYield( );

        pAIMgr->SetInitialPos( );
        theApp.BaseYield( );
    }
    catch ( CException* e )
    {
        throw( ERR_CAI_BAD_NEW );
    }
}

void AiSetup( CPlayer* pPlr )
{
    // Serial path = the original behaviour (Pre then Heavy).
    AiSetupPre( pPlr );
    AiSetupHeavy( pPlr );
}

//
// this call is made by the game to tell the AI player
// thread to end itself and send a message back indicating
// the player is dead
//
// dwID is the ID returned from AiNewPlayer
//
void AiKillPlayer( DWORD_PTR dwID )
{
    // dwID is a CAIMgr* — pointer-width. It was DWORD, which TRUNCATED the
    // pointer on x64 so SetDead() wrote m_bIsDead through a garbage `this`
    // (silent corruption when the low-32 address was mapped, AV when not —
    // e.g. crashed on player release). Null-guard too so a double-release or
    // cleared handle is harmless (a double-click confirming a Pick must not crash).
    CAIMgr* pAIMgr = (CAIMgr*)dwID;
    if ( pAIMgr != NULL )
        pAIMgr->SetDead( );
}

//
// this call is made by the game to actually delete the
// CAIMgr object
//
// dwID is the ID returned from AiNewPlayer
//
void AiDeletePlayer( DWORD_PTR dwID )
{
    CAIMgr* pAIMgr = (CAIMgr*)dwID;   // pointer-width (was DWORD -> x64 truncation)
    if ( pAIMgr == NULL )
        return;
    ASSERT_VALID( pAIMgr );

    if ( plAIMgrList != NULL )
    {
        ASSERT_VALID( plAIMgrList );
        EnterCriticalSection( &cs );
        theGame.AiPlayerIsDead( theGame.GetPlayerByPlyr( pAIMgr->GetPlayer( ) ) );
        plAIMgrList->RemoveManager( pAIMgr->GetPlayer( ) );
        LeaveCriticalSection( &cs );
    }
}

// last call on game exit - shut everything down (program still running)
void AiExit( )
{
    // drop the world snapshot: it holds pointers into the static type tables,
    // which must not be served once the world is gone
    AiSnap::Reset( );

    try
    {
        // delete standard data
        if ( plTaskList != NULL )
        {
            delete plTaskList;
            plTaskList = NULL;
        }
        if ( plGoalList != NULL )
        {
            delete plGoalList;
            plGoalList = NULL;
        }

        if ( pwaIG != NULL )
        {
            pwaIG->RemoveAll( );
            delete pwaIG;
            pwaIG = NULL;
        }

        // cleans up everything the AI had created
        if ( plAIMgrList != NULL )
        {
            delete plAIMgrList;
            plAIMgrList = NULL;
        }
        // cleans up the game data object
        if ( pGameData != NULL )
        {
            delete pGameData;
            pGameData = NULL;
        }
        if ( pException != NULL )
        {
            pException->Delete( );
            pException = NULL;
        }
    }

    catch ( ... )
    {
        plTaskList  = NULL;
        plGoalList  = NULL;
        pwaIG       = NULL;
        plAIMgrList = NULL;
        pGameData   = NULL;
        pException  = NULL;
    }
}

// BUGBUG is this still necessary?
//
LRESULT CALLBACK AiWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return ( DefWindowProc( hWnd, uMsg, wParam, lParam ) );
}

// called once for each AI once the game starts. Return when the game ends
// dwID is the ID returned from AiNewPlayer
// hex is the location player will start at
void WINAPI AiThread( AI_INIT* pAiI )
{
    CAIMgr* pAIMgr = (CAIMgr*)pAiI->dwHdl;

    ASSERT_VALID( pAIMgr );

    try
    {
        // Event-driven loop (Phase 1): block until a message arrives for this
        // AI or the idle slice elapses, then run one Manage() pass. Replaces
        // the old hot spin (Manage() called continuously with a Sleep(0)),
        // which at N AIs burned N cores' worth of CPU mostly doing nothing.
        // 100ms timeout bounds: idle-function cadence, SetDead/bEndThreads
        // notice latency, and exit-path responsiveness.
        while ( 1 )
        {
            pAIMgr->WaitForWork( 100 );  // AI_IDLE_SLICE_MS (caimgr.cpp)
            pAIMgr->Manage( );
            myYieldThread( );
        }
#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "AiThread() player %d terminated ", pAIMgr->GetPlayer( ) );
#endif
    }
    catch ( ... )
    {

#ifdef _LOGOUT
        logPrintf( LOG_PRI_ALWAYS, LOG_AI_MISC, "AiThread() player %d terminated ", pAIMgr->GetPlayer( ) );
#endif
        TRAP( );

        // ok, on the first two GPFs we start it up again
        CPlayer* pPlr = NULL;
        if ( !pAIMgr->m_bIsDead ) // remove condition: pAIMgr != NULL, always true
            pPlr = theGame._GetPlayerByPlyr( pAIMgr->GetPlayer( ) );
        if ( pPlr != NULL )
        {
            if ( pPlr->m_iNumAiGpfs < 2 )
            {
                pPlr->m_iNumAiGpfs += 1;
                // send a msg to the main thread
                CNetAiGpf msg( pPlr );
                theGame.PostToServer( &msg, sizeof( msg ) );
            }
            else
            {
                // make it dead
                pPlr->m_iNumAiGpfs = 100;
                CMsgPlyrDying msg( pPlr );
                theGame.PostToAll( &msg, sizeof( msg ) );
            }
        }
    }

    myThreadTerminate( );
}

//
// the AI message function called by the game on messages
//
void AiMessage( DWORD_PTR dwID, CNetCmd const* pMsg, int )
{
    // tell the manager that a message has come in
    CAIMgr* pAIMgr = (CAIMgr*)dwID;
    if ( pAIMgr != NULL )
        pAIMgr->MessageArrived( pMsg );
}

//
// Fan-out redundancy filter (called per AI from CGame::PostToAllAi).
//
// Returns FALSE only for message types whose EVERY consumer in the AI is
// owner-gated (verified against CAIMgr::ProcessMessage's flag switch and the
// flag consumers in Manage): the non-owner AI would pay alloc + enqueue +
// semaphore wake + a Manage pass just to discard the message. With N AIs that
// is N-1 wasted deliveries per event.
//
// Deliberately NOT filtered (intel-bearing or unaudited): bldg_new, veh_new,
// delete_unit, see_unit, unit_attacked, unit_damage (the war-attitude trigger
// reads it), bldg_stat, veh_dest (sets m_bMapChanged for ALL players),
// err_veh_goto/traffic, unit_loaded, give_unit, plyr_dying, scenario,
// cmd_play. Default is deliver.
//
BOOL AiMessageWanted( CPlayer* pPlr, CNetCmd const* pMsg )
{
    int iPlyr = pPlr->GetPlyrNum( );

    switch ( pMsg->GetType( ) )
    {
    // ProcessMessage: flag set only when m_idata3 == m_iPlayer, else the
    // message is discarded after a full queue round-trip.
    case CNetCmd::veh_loc:  // m_bLocChanged -> UpdateLoc (own vehicles only)
        return ( ( (CMsgVehLoc const*)pMsg )->m_iPlyrNum == iPlyr );

    case CNetCmd::build_civ:  // m_bPlaceBldg (owner-gated)
        return ( ( (CMsgBuildCiv const*)pMsg )->m_iPlyrNum == iPlyr );

    case CNetCmd::scenario_atk:  // m_bAttackUnit (owner-gated)
        return ( ( (CMsgScenarioAtk const*)pMsg )->m_iPlyrAi == iPlyr );

    case CNetCmd::out_of_LOS:  // m_bOutLOS (gated on attacker == me)
        return ( ( (CMsgOutOfLos const*)pMsg )->m_iPlyrAttacker == iPlyr );

    case CNetCmd::err_place_bldg:  // m_bPlaceRocket (owner + rocket)
        return ( ( (CMsgPlaceBldg const*)pMsg )->m_iPlyrNum == iPlyr );

    case CNetCmd::road_done:  // m_bMapChanged (owner-gated; others' roads are
                              // learned from the map rescan, not the message)
        return ( ( (CMsgRoadDone const*)pMsg )->m_iPlyrNum == iPlyr );

    case CNetCmd::road_new:
    case CNetCmd::err_build_road:  // same struct, owner-gated
        return ( ( (CMsgRoadNew const*)pMsg )->m_iPlyrNum == iPlyr );

    default:
        return TRUE;  // conservative: deliver anything unaudited
    }
}

void AiSaveGame( CArchive& ar )
{

    CAIHexBuff aiHexBuff;

    // no game to save
    if ( pGameData == NULL )
    {
        throw( ERR_CAI_BAD_FILE );
        return;
    }

    try
    {
        // the CGameData object stuff
        ar << pGameData->m_iSmart;
        ar << pGameData->m_iNumAi;
        ar << pGameData->m_iNumHuman;
        ar << pGameData->m_iHexPerBlk;
        ar << pGameData->m_iBlkPerSide;
        ar << pGameData->m_iStructureSize;
        ar << pGameData->m_iNumTransports;
        ar << pGameData->m_iNumBuildings;

        // mineral densities
        for ( int i = 0; i < CMaterialTypes::num_types; ++i )
        {
            CAIHex* paiHex = &pGameData->m_paihDensity[i];
            ar << paiHex->m_iX;
            ar << paiHex->m_iY;
            ar << paiHex->m_iUnit;
            ar << paiHex->m_dwUnitID;
            ar << paiHex->m_cTerrain;
        }
    }
    catch ( CFileException* theException )
    {
        // how should write errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }

    if ( plAIMgrList == NULL )
        return;

    if ( plAIMgrList->GetCount( ) )
    {
        POSITION pos = plAIMgrList->GetHeadPosition( );
        while ( pos != NULL )
        {
            CAIMgr* pMgr = (CAIMgr*)plAIMgrList->GetNext( pos );
            if ( pMgr != NULL )
            {
                ASSERT_VALID( pMgr );

                pMgr->SaveGame( ar );
            }
        }
    }
}

void AiLoadGame( CArchive& ar, BOOL bLocal )
{

    if ( plAIMgrList != NULL )
    {
        delete plAIMgrList;
        plAIMgrList = NULL;
    }

    // if not local we do just enough so we can save
    if ( bLocal )
    {
        try
        {
            plAIMgrList = new CAIMgrList( );
        }
        catch ( CException* e )
        {
            if ( plAIMgrList != NULL )
            {
                delete plAIMgrList;
                plAIMgrList = NULL;
            }
            throw( ERR_CAI_BAD_NEW );
        }
    }

    // handle global game data object and exception for yielding
    if ( pGameData == NULL )
        pGameData = new CAIData( 0, 0, 0 );
//    if ( pException == NULL )
//        pException = new CException( );

    // the CGameData stuff should be loaded separately from
    // the CAMgr stuff

    // the CGameData object stuff
    CAIHexBuff aiHexBuff;
    int        iCnt;

    try
    {
        ar >> pGameData->m_iSmart;
        ar >> pGameData->m_iNumAi;
        ar >> pGameData->m_iNumHuman;
        ar >> pGameData->m_iHexPerBlk;
        ar >> pGameData->m_iBlkPerSide;
        ar >> pGameData->m_iStructureSize;
        ar >> pGameData->m_iNumTransports;
        ar >> pGameData->m_iNumBuildings;

        // mineral densities
        if ( pGameData->m_paihDensity == NULL )
            pGameData->m_paihDensity = new CAIHex[CMaterialTypes::num_types];

        for ( int i = 0; i < CMaterialTypes::num_types; ++i )
        {
            CAIHex* paiHex = &pGameData->m_paihDensity[i];
            ar >> paiHex->m_iX;
            ar >> paiHex->m_iY;
            ar >> paiHex->m_iUnit;
            ar >> paiHex->m_dwUnitID;
            ar >> paiHex->m_cTerrain;
        }

        int i = pGameData->m_iHexPerBlk * pGameData->m_iBlkPerSide;

        // if not local we're done now
        if ( !bLocal )
            return;

        thePathMap.Init( i, i );
    }
    catch ( CFileException* theException )
    {
        // how should read errors be reported?
        throw( ERR_CAI_BAD_FILE );
    }

    // now I guess, go through all the CPlayers looking
    // for AI players and do a create for the new CAIMgr
    // and call CAIMgr::LoadGame()
    POSITION pos;
    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
        ASSERT_VALID( pPlr );

        if ( pPlr->IsAI( ) )
        {
            CAIMgr* pAIMgr = NULL;
            try
            {
                // CAIMgr( int iPlayer )
                pAIMgr = new CAIMgr( pPlr->GetPlyrNum( ) );
                plAIMgrList->AddTail( (CObject*)pAIMgr );
            }
            catch ( CException* e )
            {
                if ( pAIMgr != NULL )
                    delete pAIMgr;

                throw( ERR_CAI_BAD_NEW );
            }
            // make pointer to this CAIMgr available to the game
            pPlr->SetAiHdl( (DWORD_PTR)pAIMgr );

            pAIMgr->LoadGame( ar );
        }
    }
}

void AiLoadComplete( void )
{
}

//
// return TRUE if I can fire
// if return FALSE will do nothing (will NOT look for other)
//
BOOL AiOppoFire( CUnit* pUnit, CUnit const* pTarget )
{
    CAIMgr* pAIMgr = (CAIMgr*)pUnit->GetOwner( )->GetAiHdl( );
    if ( pAIMgr == NULL )
        return ( FALSE );

    return ( pAIMgr->AutoFire( pUnit, pTarget ) );
}

// end of ai
