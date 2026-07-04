//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// newworld.cpp : create a new world
//

#include "stdafx.h"
#include "lastplnt.h"
#include "GameWindow.h"
#include "SDL2MFCPanel.h"
#include "SDL2Compositor.h"
#include "SDL2Panel.h"
#include "SDL2CreateStatus.h"
#include "player.h"
#include "racedata.h"
#include "new_game.h"
#include "relation.h"
#include "ai.h"
#include "ipccomm.h"
#include "cpathmgr.h"
#include "help.h"
#include "research.h"
#include "chproute.hpp"
#include "bitmaps.h"
#include "sfx.h"
#include "cpathmap.h"
#include "area.h"
#include "bridge.h"
#include "CdLoc.h"

#include "terrain.inl"
#include "ui.inl"
#include "unit.inl"
#include "minerals.inl"

#include <vector>
#include <thread>
#include <atomic>

extern CPathMgr thePathMgr;

void InitColors();

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


// CDlgAiPos removed (CHEAT-only AI position dialog)

extern DWORD dwFrameCheck;

//---------------------------------------------------------------------------
// New-game create-flow profiler.
//
// One-shot high-resolution timing of the create phases, written to
// CreateProfile.log in the run dir. Negligible cost (a QPC + fprintf per
// phase boundary, only during world creation). CpReset() opens the log and
// starts the clock; CpMark("phase") records the elapsed time since the last
// mark (the phase that just finished) plus a running total.
//---------------------------------------------------------------------------
namespace
{
    LARGE_INTEGER g_cpFreq{ };
    LARGE_INTEGER g_cpPhaseStart{ };
    LARGE_INTEGER g_cpTotalStart{ };
    FILE*         g_cpLog = nullptr;

    void CpReset( const char* pszTitle )
    {
        QueryPerformanceFrequency( &g_cpFreq );
        QueryPerformanceCounter( &g_cpTotalStart );
        g_cpPhaseStart = g_cpTotalStart;
        if ( g_cpLog )
            fclose( g_cpLog );
        g_cpLog = nullptr;
        // Off by default; set EN_CREATEPROF=1 to write the create-phase timing log
        // (CreateProfile.log in the run dir). All CpMark() calls no-op when null.
        if ( !getenv( "EN_CREATEPROF" ) )
            return;
        g_cpLog = fopen( "CreateProfile.log", "w" );
        if ( g_cpLog )
        {
            SYSTEMTIME st;
            GetLocalTime( &st );
            fprintf( g_cpLog, "=== %s  %02d:%02d:%02d ===\n", pszTitle, st.wHour, st.wMinute, st.wSecond );
            fflush( g_cpLog );
        }
    }

    double CpMsBetween( const LARGE_INTEGER& a, const LARGE_INTEGER& b )
    {
        if ( g_cpFreq.QuadPart == 0 )
            return 0.0;
        return (double)( b.QuadPart - a.QuadPart ) * 1000.0 / (double)g_cpFreq.QuadPart;
    }

    void CpMark( const char* pszName )
    {
        if ( !g_cpLog )
            return;
        LARGE_INTEGER now;
        QueryPerformanceCounter( &now );
        double dMs    = CpMsBetween( g_cpPhaseStart, now );
        double dTotal = CpMsBetween( g_cpTotalStart, now );
        fprintf( g_cpLog, "  %-30s %9.1f ms   (t+%9.1f ms)\n", pszName, dMs, dTotal );
        fflush( g_cpLog );
        g_cpPhaseStart = now;
    }
}


BOOL CConquerApp::LoadData() {

    theMusicPlayer.SoundsOff();

    // put up a creating window and hourglass
    BOOL bDel = m_pCreateGame->GetDlgStatus() == NULL;
    if (bDel)
        m_pCreateGame->CreateDlgStatus();

    m_pCreateGame->GetDlgStatus()->SetMsg("");
    m_pCreateGame->GetDlgStatus()->SetPer(PER_START);
    m_pCreateGame->ShowDlgStatus();
    m_wndMain.UpdateWindow();

    try {
        theApp.Log("Load sprites");
        theGame.IncTry();

        // get R&D data
        theRsrch.Open();

        // load the data (needed by AI)
        theTerrain.InitData();
        theStructures.InitData();
        theEffects.InitData();
        theTransports.InitData();
        theTurrets.InitData();
        theFlashes.InitData();
        theExplGrp.InitData();

        // we now load the sprites themselves (after getting the others started)
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_LOAD_TERRAIN);
        theTerrain.InitSprites();
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_LOAD_STRUCTURES);
        theStructures.InitSprites();
        theEffects.InitSprites();
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_LOAD_TRANSPORTS);
        theTransports.InitSprites();

        theApp.BaseYield();
        theFlashes.InitSprites();
        theApp.BaseYield();
        theTurrets.InitSprites();
        theApp.BaseYield();

        theExplGrp.PostSpriteInit();

        CMaterialTypes::ctor();

        theApp.BaseYield();
        theTerrain.InitLang();
        theApp.BaseYield();
        theStructures.InitLang();
        theApp.BaseYield();
        theTransports.InitLang();

        ASSERT_VALID (&theStructures);
        ASSERT_VALID (&theTransports);

        // load music
        theApp.Log("Load music");
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_LOAD_MUSIC);
        theMusicPlayer.LoadGroup(SFXGROUP::play);
        theGame.DecTry();
    }

    catch (int iNum) {
        CatchNum(iNum);
        return FALSE;
    }
    catch (SE_Exception e) {
        CatchSE(e);
        return FALSE;
    }
    catch (...) {
        TRAP();
        CatchOther();
        return FALSE;
    }

    if (bDel)
        m_pCreateGame->HideDlgStatus();

    return TRUE;
}

//---------------------------------------------------------------------------
// CZoomData::CZoomData
//---------------------------------------------------------------------------
CZoomData::CZoomData(
        int nDataZooms)
        :
        m_nDataZooms(nDataZooms) {
    ASSERT(0 < m_nDataZooms && m_nDataZooms <= NUM_ZOOM_LEVELS);

    m_iFirstZoom = theApp.UseZoom0() ? 0 : 1;
    m_iFirstZoom = max(m_iFirstZoom, NUM_ZOOM_LEVELS - m_nDataZooms);

    ASSERT(0 <= m_iFirstZoom && m_iFirstZoom < NUM_ZOOM_LEVELS);
    ASSERT(m_iFirstZoom >= NUM_ZOOM_LEVELS - m_nDataZooms);
}

void CConquerApp::ReadyToJoin() {

    
#ifdef LOGGINGON
    OutputDebugStringA( "ReadyToJoin\n" );
#endif

    ASSERT_VALID (this);
    ASSERT (m_pCreateGame != NULL);

    // tell everyone we're ready
    if (theGame.HaveHP()) {

#ifdef LOGGINGON
        OutputDebugStringA( "HaveHP; CNetPlyrStatus\n" );
#endif

        CNetPlyrStatus msg(theGame.GetMe()->GetNetNum(), 0);
        ASSERT( sizeof( CNetPlyrStatus ) == sizeof( msg ) );
        theGame.PostToAll(&msg, sizeof(msg), FALSE);
    }

    m_pCreateGame->ToWorld();

    // if its single player change the text
    // or - if everyone else is ready
    if (theGame.HaveHP())
        theGame.GetMe()->SetState(CPlayer::ready);
    theGame.SetState(CGame::wait_start);
    if ((m_pCreateGame->m_iNet < 0) || ((theGame.IsAllReady()) &&
                                        (theGame.GetNetJoin() != CGame::approve)))
        m_pCreateGame->GetDlgStatus()->SetStatus();

    m_pCreateGame->UpdateBtns();

    theMusicPlayer.SoundsOff();
//BUGBUG	theMusicPlayer.PlayExclusiveMusic (MUSIC::GetID (MUSIC::create_game));

    theApp.DestroyMain();
}

void CConquerApp::ReadyToCreate() {

#ifdef LOGGINGON
    OutputDebugStringA( "ReadyToCreate\n" );
#endif

    ReadyToJoin();

    // create the AI players
    //   intermix with the human players
    POSITION pos = theGame.GetAll().GetHeadPosition();
    float fInc = (float) (theGame.GetAll().GetCount()) / (float) m_pCreateGame->m_iNumAi;
    float fOn = 0.0;
    std::string sName = EnLoadStdString(IDS_AI_PLAYER);

    for (int iPlyrOn = 0; iPlyrOn < m_pCreateGame->m_iNumAi; iPlyrOn++) {
        std::string sNum = IntToStr(iPlyrOn + 1);
        std::string sBuf = strPrintf(sName.c_str(), sNum.c_str());
        CPlayer *pPlr = new CPlayer(sBuf.c_str(), 0);
        pPlr->SetLocal(TRUE);
        theGame.AddAiPlayer(pPlr);

        // mix in to the list
        if (pos == NULL)
            theGame.GetAll().AddTail(pPlr);
        else {
            theGame.GetAll().InsertBefore(pos, pPlr);
            fOn += fInc;
            while ((fOn >= 1.0) && (pos != NULL)) {
                theGame.GetAll().GetNext(pos);
                fOn -= 1.0;
            }
        }
    }

    // if its single player just create the world
    if (m_pCreateGame->m_iNet < 0) {
        theApp.BaseYield();

        unsigned uRand = MySeed();

        // (cheat seed-override dialog removed — Debug:SetRand was its only use)

        // TEST HOOK (env-guarded, SP-only): EN_WG_SEED forces the world-gen seed so
        // run-to-run and cross-platform [wg] traces are directly comparable — a
        // determinism check for the ocean-OOB / cross-platform RAND MISMATCH hunt.
        // No effect unless EN_WG_SEED is set; does not touch the MP (host-seeded) path.
        if ( const char* szSeed = getenv( "EN_WG_SEED" ) )
            uRand = (unsigned) strtoul( szSeed, nullptr, 0 );

        AIinit aiData(m_pCreateGame->m_iAi, m_pCreateGame->m_iNumAi,
                      theGame.GetAll().GetCount() - m_pCreateGame->m_iNumAi, m_pCreateGame->m_iSize);
        theApp.CreateNewWorld(uRand, &aiData, 2, 32);
        return;
    }

    // if everyone else was ready - go do it
    theApp.BaseYield();
    if ((theGame.IsAllReady()) && (theGame.GetNetJoin() != CGame::approve))
        StartCreateWorld();
}

void CConquerApp::StartCreateWorld() {

    // Guard against DOUBLE world creation. Both the host's Start (ReadyToCreate)
    // and the client-ready auto-start (CmdReady) call StartCreateWorld; the second
    // fires RE-ENTRANTLY from inside the first CreateNewWorld's BaseYield message
    // pump, building the world twice -> AV in World::Create (duplicate area/world
    // windows). The first call flips the net mode to 'starting' (below), so any
    // later/nested call bails here.
    if (theNet.GetMode() == CNetApi::starting)
        return;

    ASSERT_VALID (this);
    ASSERT (theGame.GetMe()->GetNetNum() > 0);
    ASSERT (strlen(theGame.GetMe()->GetName()) > 0);

    // set the status box
    m_pCreateGame->GetDlgStatus()->SetStatus();

    // stop the net till we are running
    theNet.SetMode(CNetApi::starting);
    if (theGame.AmServer())
        theNet.SetSessionVisibility(FALSE);

    // drop lstLoad
    POSITION pos;
    for (pos = theGame.m_lstLoad.GetTailPosition(); pos != NULL;) {
        TRAP();
        CPlayer *pPlr = theGame.m_lstLoad.GetPrev(pos);
        m_pCreateGame->RemovePlayer(pPlr);
        theNet.DeletePlayer(pPlr->GetNetNum());
        delete pPlr;
    }
    theGame.m_lstLoad.RemoveAll();

    // kill anyone not ready
    for (pos = theGame.GetAll().GetTailPosition(); pos != NULL;) {
        CPlayer *pPlr = theGame.GetAll().GetPrev(pos);
        ASSERT_VALID (pPlr);
        if (pPlr->GetState() != CPlayer::ready) {
            m_pCreateGame->RemovePlayer(pPlr);
            theNet.DeletePlayer(pPlr->GetNetNum());
            theGame.DeletePlayer(pPlr);
        }
    }

    // lets start the game
    unsigned uRand = MySeed();

    // (cheat seed-override dialog removed)

    AIinit aiData(m_pCreateGame->m_iAi, m_pCreateGame->m_iNumAi,
                  theGame.GetAll().GetCount() - m_pCreateGame->m_iNumAi, m_pCreateGame->m_iSize);
    theApp.CreateNewWorld(uRand, &aiData, 2, 32);
}

int GetPrime(int iMin) {

    if (iMin <= 3)
        return (iMin);

    // never even, do a +2
    iMin = (iMin - 2) | 1;

    BOOL bFoundIt;
    do {
        bFoundIt = TRUE;
        iMin += 2;
        int iMax = (int) sqrt((float) iMin);

        for (int iTry = 2; iTry < iMax; iTry++)
            if (iMin % iTry == 0) {
                bFoundIt = FALSE;
                break;
            }

    } while (!bFoundIt);

    return (iMin);
}

void CConquerApp::CreateNewWorld(unsigned uRand, AIinit *pAiData, int iSide, int iSideSize) {
    OutputDebugStringA( "[REN] CreateNewWorld ENTRY\n" );
    { FILE* _f = fopen( "SDL2Panel.log", "a" ); if ( _f ) { fputs( "[REN] CreateNewWorld ENTRY\n", _f ); fclose( _f ); } }

    ASSERT (m_pCreateGame->GetDlgStatus() != NULL);
    // ASSERT_VALID + m_hWnd check removed — SDL2CreateStatus is the replacement (Phase 2d) and is not a CObject/CWnd

#ifdef _DEBUG
    theDataFile.EnableNegativeSeekChecking ();
#endif

    dwFrameCheck = 2 * 1000 / FRAME_RATE;

    CpReset( "CreateNewWorld" );

    theApp.Log("Create world");

    // no player, no music/sound
    if (!theGame.HaveHP()) {
        theMusicPlayer.SoundsOff();
        theMusicPlayer.SetSoundVolume(0);
        theMusicPlayer.SetMusicVolume(0);
    }

    // we ignore all requests to join from now till the game is going.
    theNet.SetMode(CNetApi::starting);
    theGame.SetState(CGame::init);
    if (m_pCreateGame->m_iTyp == CCreateBase::create_net)
        m_pCreateGame->UpdateBtns();

    // make sure cleared out
    theBridgeMap.RemoveAll();
    theBridgeHex.RemoveAll();
    theBuildingMap.RemoveAll();
    theBuildingHex.RemoveAll();
    theProjMap.RemoveAll();
    theVehicleMap.RemoveAll();
    theVehicleHex.RemoveAll();

    // set up hash tables
    int iNumPlayers = theGame.GetAll().GetCount() + 1;
    theBridgeMap.InitHashTable(GetPrime(iNumPlayers * 20));
    theBridgeHex.InitHashTable(GetPrime(iNumPlayers * 100));
    theBuildingMap.InitHashTable(GetPrime(iNumPlayers * 200));
    theBuildingHex.InitHashTable(GetPrime(iNumPlayers * 200 * 9));
    theProjMap.InitHashTable(GetPrime(iNumPlayers * 2000));
    theVehicleMap.InitHashTable(GetPrime(iNumPlayers * 2000));
    theVehicleHex.InitHashTable(GetPrime(iNumPlayers * 2000 * 2));

    // we all have to build from the same seed
    theGame.SetSeed(uRand);

    CpMark( "hash tables + seed" );

    theApp.Log( "Set seed " );

    // we do it here because of the initial paint
    theGame.SetElapsedSeconds(0);

    // init the global vars
    ASSERT ((m_bInGame == FALSE) && (!theGame.ShouldOperate()) && (!theGame.ShouldAnimate()));
    m_bInGame = TRUE;
    bForceDraw = FALSE;
    bInvAmb = FALSE;
    xiZoom = 0;
    xiDir = 0;
    xpdibwnd = NULL;

    CUnit::m_sDamage = EnLoadStdString(IDS_DAMAGE);
    InitColors( );

    // put up a creating window and hourglass
    m_wndMain.UpdateWindow();
    // Make the progress window actually visible. LoadData() shows it on first
    // data load, but the single-player create path comes straight here (skipping
    // LoadData) and would otherwise SetPer() an invisible window — no progress
    // bar after picking your race. ShowDlgStatus() creates/shows it if needed.
    m_pCreateGame->ShowDlgStatus();
    m_pCreateGame->GetDlgStatus()->SetPer(PER_START);

    // get R&D data
    theRsrch.Open();

    CpMark( "globals + InitColors + Rsrch.Open" );

    theApp.Log( "Loading terrain data" );
    // load the data (needed by AI)
    theTerrain.InitData();
    theStructures.InitData();
    theEffects.InitData();
    theTransports.InitData();
    theTurrets.InitData();
    theFlashes.InitData();
    theExplGrp.InitData();

    CpMark( "InitData (terrain/struct/etc)" );

    theApp.Log( "Setup AI" );
    // setup the AI
    // tell the AI about the game (needed for client to for save)
    if (AiInit(pAiData->m_iDiff, pAiData->m_iNumAI, pAiData->m_iNumHp, m_pCreateGame->m_iPos))
        return;

    if ( theGame.AmServer( ) )
    {
        theApp.Log( "Send " + IDS_START_AI1 );
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_START_AI1);

        // Get the attributes for each AI player
        POSITION pos;
        for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL;) {
            CPlayer *pPlr = theGame.GetAll().GetNext(pos);
            ASSERT_VALID (pPlr);
            if (pPlr->IsAI())
                if (AiNewPlayer(pPlr))
                    return;
        }

        // SP-start fix (revert @c3540c06 + @8de2cff2): do NOT route non-AI (human)
        // players through the AI init path. Those commits added a non-AI AiNewPlayer
        // loop here on the false premise "the human needs a CAIMgr or box-select /
        // HOME-center / crane-Build fail". Verified false on gcc: kept out of the AI
        // path the human starts with 0 placed units and lands its rocket
        // (CWndArea::SetupStart), which disgorges its chosen loadout — and box-select
        // / HOME / crane-Build all work without a human CAIMgr, as in the original
        // game. The loop above stays AI-only (pre-regression behaviour).
    }

    // set rand
    MySrand(uRand);

    CpMark( "AiInit + AiNewPlayer loop" );

    theApp.Log( "Call StartGame on players" );
    // get the players data ready
    POSITION pos;
    for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL;) {
        CPlayer *pPlr = theGame.GetAll().GetNext(pos);
        ASSERT_VALID (pPlr);
        pPlr->StartGame();
    }

    CpMark( "StartGame on players" );

    // we save and reset the rand in case the below does something (it's server only)
    int iSaveRand = MyRand();

    // set up the world
    if (theGame.AmServer()) {
        // get the map size
        theMap.GetWorldSize(pAiData->m_iSize, iSide, iSideSize);
        if (AiWorldSize(iSideSize, iSide))
            return;

        theApp.Log("Tell other players to start");
        // send everyone all the players so they can set up their direct links
        if (theGame.IsNetGame())
            theGame.StartAllPlayers();

        // we now have the size - tell the others (if any)
        theGame.StartNewWorld(uRand, iSide, iSideSize);
    } else {
        theGame.SetSideSize(iSideSize);

        // needed for save game AI data
        if (AiWorldSize(iSideSize, iSide))
            return;
    }

    CpMark( "world size + StartNewWorld" );

    theApp.Log("Init pathmap");
    // set up thePathMap
    int iSize = iSideSize * iSide;
    theApp.BaseYield();
    thePathMap.Init(iSize, iSize);
    theApp.BaseYield();

    CpMark( "thePathMap.Init" );

    try {
        theApp.Log("Load sprites");
        // we now load the sprites themselves (after getting the others started)
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_LOAD_TERRAIN);
        theTerrain.InitSprites();
        CpMark( "  theTerrain.InitSprites" );
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_LOAD_STRUCTURES);
        theStructures.InitSprites();
        CpMark( "  theStructures.InitSprites" );
        theEffects.InitSprites();
        CpMark( "  theEffects.InitSprites" );
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_LOAD_TRANSPORTS);
        theTransports.InitSprites();
        CpMark( "  theTransports.InitSprites" );

        theApp.BaseYield();
        theFlashes.InitSprites();
        CpMark( "  theFlashes.InitSprites" );
        theApp.BaseYield();
        theTurrets.InitSprites();
        CpMark( "  theTurrets.InitSprites" );
        theApp.BaseYield();

        theExplGrp.PostSpriteInit();
        CpMark( "  theExplGrp.PostSpriteInit" );

        CMaterialTypes::ctor();

        theApp.BaseYield();
        theTerrain.InitLang();
        theApp.BaseYield();
        theStructures.InitLang();
        theApp.BaseYield();
        theTransports.InitLang();

        ASSERT_VALID (&theStructures);
        ASSERT_VALID (&theTransports);

        CpMark( "  InitLang (3x)" );

        // load music
        theApp.Log("Load music");
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_LOAD_MUSIC);
        theMusicPlayer.LoadGroup(SFXGROUP::play);

        CpMark( "LoadGroup (music/sfx)" );
    }

    catch (int iNum) {
        CatchNum(iNum);
        theApp.CloseWorld();
        return;
    }
    catch (SE_Exception e) {
        CatchSE(e);
        theApp.CloseWorld();
        return;
    }
    catch (...) {
        TRAP();
        CatchOther();
        theApp.CloseWorld();
        return;
    }

    // restore rand
    MySrand(iSaveRand);

    // MP world-gen parity: baseline fingerprint right after the seed is set, BEFORE
    // any world-gen draw. Host and client feed the SAME seed here (CNetStart m_uRand),
    // so these two lines must match across platforms; if they already differ the
    // divergence is upstream of world-gen. See MyRandTrace / EN_RANDTRACE.
    MyRandTrace( "worldgen: seed set (pre-theMap.Init)" );

    // Progress-bar smoothing for the SECOND+ game (operator, minor): the 0..85
    // band is sprite/data loading (PER_*_INIT), which drives the bar smoothly on
    // the FIRST create. On a reload the assets are already resident, so every
    // InitSprites/InitData above is a fast no-op that emits NO SetPer — leaving
    // the bar stuck at 0 until world-gen's first SetPer(PER_WORLD_START=86)
    // snaps it to ~90. Advance to PER_MUZZLE_FLASHES_INIT (85) here: on the
    // first create the bar is already there (SetPer is no-backward, so no-op);
    // on a reload it jumps 0->85, which is honest (85% = all assets loaded) and
    // turns "stuck at 0 then jump to 90" into "quick to 85 then smooth to 100".
    m_pCreateGame->GetDlgStatus()->SetPer(PER_MUZZLE_FLASHES_INIT);

    // This is where we create the actual map / world / terrain
    // create the map
    theApp.Log("Create the map");
    theMap.Init(iSide, iSideSize, m_pCreateGame->m_iTyp == CCreateBase::scenario);
    theApp.Log("Map created");

    CpMark( "theMap.Init (WORLDGEN)" );

    // MP world-gen parity: fingerprint the FULL world-gen draw sequence. This is the
    // decisive line — compare it host-vs-client (both with EN_RANDTRACE=1) against the
    // baseline above. calls DIFFER => a control-flow / unsequenced-MyRand()-arg-order
    // divergence inside theMap.Init; calls SAME but sum DIFFERS => a value divergence
    // (a float/data input feeding the same number of draws). Either way this is what
    // makes m_dwFinalRand mismatch and kicks the client back to the menu at CmdPlay.
    MyRandTrace( "worldgen: theMap.Init done (pre-finalrand)" );

    // compare final seeds
    theGame.m_dwFinalRand = MyRand();

    // create path manager
    if (!thePathMgr.Init((iSide * iSideSize), (iSide * iSideSize))) {
        CloseWorld();
        return;
    }

    CpMark( "thePathMgr.Init" );

    // create all the windows (in reverse order of importance)
    m_pCreateGame->GetDlgStatus()->SetMsg(IDS_CREATE_WINDOWS);
    m_pCreateGame->GetDlgStatus()->SetPer(PER_CREATE_WINDOWS);

    {
        char b[128];
        sprintf_s( b, "[REN] NewGame window-create reached: HaveHP=%d\n", theGame.HaveHP() ? 1 : 0 );
        OutputDebugStringA( b );
    }
    if (theGame.HaveHP()) {
        m_wndBar.Create();    // must be first to set row3
        m_wndVehicles.Create();
        m_wndBldgs.Create();
        if (theGame.GetAll().GetCount() > theGame.GetAi().GetCount() + 1)
            if ( !m_gameWindow )  // SDL2 path: chat not yet implemented
                m_wndChat.Create();
        // CDlgResearch removed (Phase 2d) — SDL2ResearchDialog is modal,
        // launched from the toolbar Research button on demand.
        CWndArea *pWndArea = new CWndArea();
        pWndArea->Create(theGame.GetMe()->m_hexMapStart, NULL, TRUE);

        // create world for new game (loaded is in player.cpp)
        m_wndWorld.Create(TRUE);        // world must come after area

        // Make MFC main window transparent (area, world, toolbar handle themselves).
        // Attach vehicle/building list windows as SDL panels.
        if ( m_gameWindow )
        {
            // Main window just needs to be transparent
            ::SetWindowLong( m_wndMain.m_hWnd, GWL_EXSTYLE,
                ::GetWindowLong( m_wndMain.m_hWnd, GWL_EXSTYLE ) | WS_EX_LAYERED );
            ::SetLayeredWindowAttributes( m_wndMain.m_hWnd, 0, 0, LWA_ALPHA );

            // Vehicle/building lists are now native SDL2 (opened via toolbar buttons).
            // Just make MFC windows transparent so they don't show.
            auto hideWnd = []( auto& w ) {
                if ( !w.m_hWnd ) return;
                ::SetWindowLong( w.m_hWnd, GWL_EXSTYLE,
                    ::GetWindowLong( w.m_hWnd, GWL_EXSTYLE ) | WS_EX_LAYERED );
                ::SetLayeredWindowAttributes( w.m_hWnd, 0, 0, LWA_ALPHA );
            };
            hideWnd( m_wndVehicles );
            hideWnd( m_wndBldgs );
            // m_wndChat is now ChatStub (Phase 2d-cont) — never has an HWND, never attaches.

            m_gameWindow->Raise();
        }
    }

    CpMark( "Create windows" );

    // set up our routing
    if (theGame.HaveHP()) {
        theApp.Log("Create the HP router");
        theGame.m_pHpRtr = new CHPRouter(theGame.GetMe()->GetPlyrNum());
        if (!theGame.m_pHpRtr->Init()) {
            TRAP();
            CloseWorld();
            return;
        }
        theApp.Log("HP router created");
    }

    CpMark( "HP router" );

    // get rid of the creation dialogs
    if (!theGame.AmServer())
        m_pCreateGame->GetDlgStatus()->SetPer(PER_DONE, FALSE);

    theApp.Log( "SetState wait_AI" );
    // tell everyone we are done
    theGame.SetState(CGame::wait_AI);
    if (theGame.HaveHP()) {
        CNetInitDone msg( theGame.GetMe( ) );
        theApp.Log( "PostToServer" );
        theGame.PostToServer(&msg, sizeof(msg));
    }
    else
    {
        theApp.Log( "Not HP" );
    
    }

#ifdef _DEBUG
  //  theDataFile.DisableNegativeSeekChecking ();
#endif

    theApp.Log( "IDS_READY_TO_GO" ); // Waiting for Others
    // tell player we are waiting on others
    m_pCreateGame->GetDlgStatus()->SetMsg(IDS_READY_TO_GO);

    CpMark( "wait_AI / post-init" );
    if ( g_cpLog )
        fprintf( g_cpLog, "  --- CreateNewWorld returned; waiting for StartAi ---\n" );
}

void CConquerApp::StartAi() {

#ifdef LOGGINGON
    OutputDebugStringA( "StartAi\n" );
#endif
   // TRACE( "StartAi\n");


    if ( g_cpLog )
        fprintf( g_cpLog, "  --- StartAi entered ---\n" );
    CpMark( "gap (CreateNewWorld -> StartAi)" );

    try {
        theGame.SetAI(TRUE);
        theGame.IncTry();

#ifdef LOGGINGON
//        OutputDebugStringA( "ASSERT\n" );
#endif
        ASSERT (theGame.AmServer());
        theGame.SetState(CGame::init_AI);

#ifdef LOGGINGON
        OutputDebugStringA( "SetMsg\n" );
#endif
        // start the AI
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_START_AI2);
        m_pCreateGame->GetDlgStatus()->SetPer(PER_START_AI);

        int iNum = theGame.GetAi().GetCount() * 2;
        int iOn = 0;

        // Snapshot the player-independent per-hex map data ONCE so every AI's
        // map build reads it lock-free instead of re-scanning the locked game
        // map. Built on the main thread before any worker/AI thread starts.
        AiHexCacheBuild();

        // Tell each AI to set itself up (serial). Each AI's CreateMap copies the
        // shared base map the first AI builds, instead of re-scanning the locked
        // game map — see AiHexCacheBuild / the base-map cache in caimap.cpp.
        POSITION posS;
        for (posS = theGame.GetAi().GetHeadPosition(); posS != NULL;) {
            CPlayer *pPlr = theGame.GetAi().GetNext(posS);
            ASSERT_VALID (pPlr);
            ASSERT (pPlr->IsAI());

#ifdef _CHEAT
           if (!EnGetProfileInt("Debug", "NoThreads", 0))
#endif
            AiSetup(pPlr);

            m_pCreateGame->GetDlgStatus()->SetPer(PER_START_AI + (iOn * PER_NUM_START_AI) / iNum);
            iOn++;
        }

        // SP-start fix (paired with the AiNewPlayer revert above): do NOT AiSetup
        // non-AI players. @c3540c06/@8de2cff2 ran the human through AiSetup ->
        // CAIInitPos::DoIt -> PlaceRocket()+disgorge, which pre-empted the human's
        // real start (land-your-rocket) by auto-landing the rocket with a placed
        // loadout. AI-only loop above is the pre-regression behaviour.

        // Done building every AI's map — drop the snapshot so gameplay (the AI
        // threads launched just below) uses live, locked game-map reads.
        AiHexCacheFree();

        if ( g_cpLog )
            fprintf( g_cpLog, "  (AI count = %d)\n", theGame.GetAi().GetCount() );
        CpMark( "AI setup (all AIs, serial)" );

        POSITION pos;
        // now we start each thread
#ifdef LOGGINGON
        theApp.Log( "Start AI threads" );
#endif
        for (pos = theGame.GetAi().GetHeadPosition(); pos != NULL;) {
            CPlayer *pPlr = theGame.GetAi().GetNext(pos);
            ASSERT_VALID (pPlr);
            ASSERT (pPlr->IsAI());
            pPlr->ai.dwHdl = pPlr->GetAiHdl();

            // start it (if not dead)
#ifdef _CHEAT
            if (GetProfileInt ("Debug", "NoThreads", 0) == 0)
#endif
            if (pPlr->GetBldgsHave() > 0)
                myStartThread(&(pPlr->ai), (AFX_THREADPROC) AiThread);

            m_pCreateGame->GetDlgStatus()->SetPer(PER_START_AI + (iOn * PER_NUM_START_AI) / iNum);
            iOn++;

            // mark it as ready to go
            pPlr->SetState(CPlayer::wait);
        }
#ifdef LOGGINGON
        theApp.Log( "AI threads started" );
#endif

        CpMark( "launch AI threads" );
        if ( g_cpLog )
        {
            LARGE_INTEGER now;
            QueryPerformanceCounter( &now );
            fprintf( g_cpLog, "=== TOTAL create+AI: %.1f ms ===\n",
                     CpMsBetween( g_cpTotalStart, now ) );
            fflush( g_cpLog );
        }

        // get rid of the creation dialogs — NET GAMES ONLY. The early hide is
        // deliberate there (it reveals the create_net lobby while the host
        // waits for clients). In single-player there is no lobby underneath:
        // PER_DONE here (a) Hide()s the dialog and (b) latches m_percent=100,
        // whose don't-go-backward rule turns LetsGo's 97/98/99 milestones into
        // no-ops — so the whole LetsGo tail (2x Sleep(500*AI-count) + window
        // ordering, ~8-10s at 8+ AIs, main thread blocked) ran against a bare
        // frozen game view: the operator's "loading got fast but the game view
        // lags before you can play" regression. Keep the dialog up in SP; it
        // paints through the tail and DestroyExceptMain (end of LetsGo) tears
        // it down — the dtor handles a never-Hidden window (SDL2CreateStatus).
        if (theGame.IsNetGame())
            m_pCreateGame->GetDlgStatus()->SetPer(PER_DONE, FALSE);

        // tell the AI
        // BUGBUG - need % update here
        CNetCmd cmd(CNetCmd::cmd_play);
        for (pos = theGame.GetAi().GetHeadPosition(); pos != NULL;) {
            CPlayer *pPlr = theGame.GetAi().GetNext(pos);
            ASSERT_VALID (pPlr);
            ASSERT (pPlr->IsAI());
            AiMessage(pPlr->GetAiHdl(), &cmd, sizeof(cmd));
        }

        // put here so can't delete anymore
        theGame.SetState(CGame::AI_done);
        if (m_pCreateGame->m_iTyp == CCreateBase::create_net)
            m_pCreateGame->UpdateBtns();

        // if we need to switch any to AI do it here
        for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL;) {
            CPlayer *pPlr = theGame.GetAll().GetNext(pos);
            ASSERT_VALID (pPlr);
            if (pPlr->GetState() == CPlayer::replace) {
                TRAP(); // check number of threads in 20 seconds
                {
                    extern void EnMpDiagLog( const char* fmt, ... );   // [mp-plyr] trace (netapi.cpp)
                    EnMpDiagLog( "StartAi: plyr=%d name='%s' state=replace -> AI takeover + cmd_to_ai broadcast",
                                 pPlr->GetPlyrNum(), pPlr->GetName() );
                }
                theGame.AiTakeOverPlayer(pPlr, TRUE);

                // tell the world
                CNetToAi msg(pPlr);
                theGame.PostToAllClients(&msg, sizeof(msg), FALSE);
            }
        }

        // lets play
        CNetPlay msg(theGame.m_dwFinalRand);
        theGame.PostToAll(&msg, sizeof(msg), FALSE);
        theGame.DecTry();
    }

    catch (int iNum) {
        AiHexCacheFree();   // never leave the setup snapshot live past setup
        CatchNum(iNum);
        theApp.CloseWorld();
        return;
    }
    catch (SE_Exception e) {
        AiHexCacheFree();
        TRAP();
        CatchSE(e);
        theApp.CloseWorld();
        return;
    }
    catch (...) {
        AiHexCacheFree();
        TRAP();
        CatchOther();
        theApp.CloseWorld();
        return;
    }
}

typedef CWndStub _OrderShowUpdateWin;

static void _OrderWin(_OrderShowUpdateWin *pWnd, _OrderShowUpdateWin *pBefore) {

    if ((pWnd == NULL) || (pWnd->m_hWnd == NULL))
        return;

    // set the order
    if (pBefore != NULL)
        pWnd->SetWindowPos(pBefore->m_hWnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
}

static void _ShowWin(_OrderShowUpdateWin *pWnd, WINDOWPLACEMENT *pWp) {

    if ((pWnd == NULL) || (pWnd->m_hWnd == NULL))
        return;

    pWnd->InvalidateRect(NULL);

//BUGBUG	if ( ( wp != NULL ) && ( pWp->length >= sizeof (WINDOWPLACEMENT) ) )
//		pWnd->SetWindowPlacement ( pWp );
//	else
    pWnd->ShowWindow(SW_SHOW);
}

static void _UpdateWin(_OrderShowUpdateWin *pWnd) {

    if ((pWnd == NULL) || (pWnd->m_hWnd == NULL))
        return;

    pWnd->EnableWindow(TRUE);
    pWnd->UpdateWindow();
}

// star the game?
void CConquerApp::LetsGo() {

    // do we have a CD?
    if (!CheckForCD()) {
        CloseWorld();
        return;
    }

    // tell server speed we want
    if (!theGame.AmServer()) {
        CMsgGameSpeed msg(theGame.GetGameMul());
        theGame.PostToServer(&msg, sizeof(msg));
    }

    // do we have a time limit?
    // never on scenarios (limited to scenario 5)
    if (theGame.GetScenario() != -1)
        m_bTimeLimit = FALSE;
    else
        // shareware yes
    if (IsShareware())
        m_bTimeLimit = TRUE;
    else if (!IsSecondDisk())
        m_bTimeLimit = FALSE;
        // second disk - not multi-player
    else if (theGame.IsNetGame())
        m_bTimeLimit = FALSE;
    else
        m_bTimeLimit = TRUE;

    LoadStandardCursor(IDC_WAIT);

    // we need this now because we delete m_pCreateGame
    // we do this for all multi-player in case something hits it
    if ((theGame.AmServer()) && (theGame.GetMyNetNum() != 0)) {
        CNetPublish *pMsg = CNetPublish::Alloc(m_pCreateGame);
        pMsg->m_cFlags |= CNetPublish::finprogress;
        pMsg->m_iNumPlayers = theGame.GetAi().GetCount();
        theNet.UpdateSessionData(pMsg);
        delete[] pMsg;
    }

    // NOTE: DestroyExceptMain (which deletes m_pCreateGame and with it the
    // loading dialog) now runs at the END of LetsGo, not here — the tail below
    // (message pumps + Sleep(500*AI count) TWICE + window ordering + the area
    // map's first draw) is the multi-second "hangs at the game screen" stretch
    // the operator kept reporting: the dialog died here, so the whole tail ran
    // against a frozen wallpaper. Kept visible (with the % milestones below
    // painting via SetPer's direct render), it covers the tail, and the
    // deferred detached panels bake their first frame while still hidden and
    // reveal fully-formed after it closes.

    // chat off if not multi-HP
    if ((theGame.GetMyNetNum() == 0) ||
        (theGame.GetAll().GetCount() <= theGame.GetAi().GetCount() + 1))
        theApp.CloseDlgChat();

    // (CDlgAiPos removed; was a CHEAT-only dialog gated on Debug:ShowAIStart)

    // for loaded games - have all IsMe buildings say out_mat
    POSITION pos = theBuildingMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CBuilding *pBldg;
        theBuildingMap.GetNextAssoc(pos, dwID, pBldg);
        if (pBldg->GetOwner()->IsMe())
            for (int i = 0; i < CMaterialTypes::num_build_types; ++i)
                if (pBldg->GetNextMinuteMat(i) > 0) {
                    theGame.m_pHpRtr->MsgOutMat(pBldg);
                    break;
                }
    }

    // time to switch music over to game play mode
    if (theGame.HaveHP()) {
        theMusicPlayer.EndExclusiveMusic();
        theMusicPlayer.StartMidiMusic();
        theMusicPlayer.PlayMusicGroup(MUSIC::GetID(MUSIC::play_game), MUSIC::num_play_game);
    }

    // enable windows, bring area to the top
    theGame.SetShouldProcessMessages(TRUE);
    theGame.SetShouldAnimate(TRUE);

    // we want each generating different random numbers in case of vehicle contention
    MySrand(MySeed());

    theNet.SetMode(CNetApi::playing);
    theGame.SetState(CGame::play);
    theGame.SetShouldOperate(TRUE);

    // We turn the game on and give the AI threads their setup head start —
    // UNDER the loading dialog. Originally this was two blind
    // ::Sleep(500 × AI-count) pauses (1996 cooperative threading: sleeping the
    // main thread WAS how the AI threads got CPU to place their starting units).
    // On preemptive threads the AIs run concurrently, so instead: slice the wait
    // 100ms at a time and drain their message traffic each slice, so the setup
    // flood is consumed here — on the loading screen — rather than stalling the
    // first game-view frames (operator: "it needs to be loaded before the game";
    // 35-player games are fine taking a long LOAD, never a frozen game view).
    // Budget per operator spec: 250ms × AI-count, capped at 10s; after the
    // budget, keep draining until the queue has been quiet for 3 consecutive
    // slices (the AIs have stopped talking), with a hard +20s ceiling so a
    // chatty AI can never wedge game-start.
    {
        const DWORD dwBudget  = __min( (DWORD)( 250 * theGame.GetAi().GetCount() ),
                                       (DWORD)10000 );
        const DWORD dwCeiling = dwBudget + 20000;
        DWORD       dwWaited  = 0;
        int         iQuiet    = 0;
        while ( TRUE )
        {
            theGame.ProcessAllMessages();
            iQuiet = ( theGame.m_messagePointerList.GetCount() <= 0 ) ? iQuiet + 1 : 0;
            if ( ( dwWaited >= dwBudget ) && ( iQuiet >= 3 ) )
                break;                      // head start served AND traffic settled
            if ( dwWaited >= dwCeiling )
                break;                      // safety: never wedge the start
            if ( m_pCreateGame && m_pCreateGame->GetDlgStatus() )
                m_pCreateGame->GetDlgStatus()->SetPer(
                    __min( 99, 97 + (int)( ( 2 * dwWaited ) / __max( dwBudget, 1 ) ) ) );
            ::Sleep( 100 );
            dwWaited += 100;
        }
    }
    if (m_pCreateGame && m_pCreateGame->GetDlgStatus())
        m_pCreateGame->GetDlgStatus()->SetPer(99);

    // if we allow join in process restart enums
    if ((theGame.AmServer()) && (theGame.GetNetJoin() == CGame::any) &&
        (theGame.GetAi().GetCount() > 0))
        theNet.SetSessionVisibility(TRUE);

    // show them
    if (!theGame.HaveHP())
        theApp.ShowPlayerList();
    else {
        // CDlgResearch removed (Phase 2d) — SDL2ResearchDialog is modal,
        // launched from the toolbar Research button on demand.

        // m_wndChat is now ChatStub (Phase 2d-cont) — never a CWnd, always NULL.
        _OrderShowUpdateWin *pWndChat = NULL;
        _OrderShowUpdateWin *pWndArea = theAreaList.GetTop();
        _OrderWin(&m_wndVehicles, NULL);
        _OrderWin(&m_wndBldgs, &m_wndVehicles);
        _OrderWin(pWndChat, &m_wndBldgs);
        // CDlgResearch removed (Phase 2d).
        _OrderWin(&m_wndWorld, pWndChat != NULL ? pWndChat : &m_wndBldgs);
        _OrderWin(pWndArea, &m_wndWorld);
        _OrderWin(&m_wndBar, pWndArea);

        _ShowWin(&m_wndBldgs, &(theGame.m_wpBldgs));
        _ShowWin(&m_wndVehicles, &(theGame.m_wpVehicles));
        _ShowWin(pWndChat, &(theGame.m_wpChat));
        _ShowWin(&m_wndWorld, &(theGame.m_wpWorld));
        _ShowWin(pWndArea, &(theGame.m_wpArea));
        _ShowWin(&m_wndBar, NULL);

        ((CWndArea *) pWndArea)->Draw();    // GGTESTING

        _UpdateWin(&m_wndBar);
        _UpdateWin(pWndArea);
        _UpdateWin(&m_wndWorld);
        _UpdateWin(pWndChat);
        _UpdateWin(&m_wndVehicles);
        _UpdateWin(&m_wndBldgs);
    }

    theGame.SetElapsedSeconds(0);

    // ZERO THE SIM-CLOCK DEBT (operator's "one frame holds 20-30s at game
    // start"). ShouldOperate turned on near the TOP of LetsGo, so every second
    // spent under the loading dialog since (AI settle, window ordering — and
    // historically the 500ms×AI sleeps) accrues as unsimulated time. The first
    // GraphicsEnginePump after the dialog closes then computes
    // m_dwOpersElapsed = (now - m_dwOperTimeLast)/42ms = HUNDREDS of sim steps
    // and fast-forwards them all in ONE iteration — with 9 AIs × 70 starting
    // units that single catch-up frame ran 8-16s (EN_PERF: fps=0.1,
    // avg=8030ms, path.nodes=4.9M/interval) while the display held the last
    // frame (input still pumps mid-iteration, so clicks/sounds worked). The
    // game must START at T=0, not repay the loading time: stamp both pacing
    // clocks to NOW so the first frame simulates 1/24s like every other frame.
    // (Local pacing only — lockstep is message-driven; nothing here is synced.)
    theGame._SettimeGetTime();
    theGame.m_dwOperTimeLast  = theGame.GettimeGetTime();
    theGame.m_dwFrameTimeLast = theGame.GettimeGetTime();

    // tell everyone speed we are at
    if (theGame.AmServer() && theGame.IsNetGame()) {
        CMsgGameSpeed msg(theGame.GetGameMul());
        theGame.PostToAll(&msg, sizeof(msg), FALSE);

        // and make us force frames less often so we process messages
        dwFrameCheck = 4 * 1000 / FRAME_RATE;
    }

    // NOW the load is really over — DestroyExceptMain deletes m_pCreateGame,
    // which hides + destroys the loading dialog: that is the deferred detached
    // panels' signal to reveal (first RenderDetached after this shows them).
    // Also tears down the pause/cutscene leftovers it always handled.
    DestroyExceptMain();

    ASSERT (TestEverything());
}

void CConquerApp::NewWorld() {

#ifdef _DEBUG
    theDataFile.EnableNegativeSeekChecking ();
#endif

    // we ignore all requests to join from now till the game is going.
    theNet.SetMode(CNetApi::starting);
    theGame.SetState(CGame::init);

    theMusicPlayer.SoundsOff();
//BUGBUG	theMusicPlayer.PlayExclusiveMusic (MUSIC::GetID (MUSIC::create_game));

    // LOAD-while-in-game CRASH FIX (mac2 2/2 repro; newwin root-cause): if we are loading a
    // save while ALREADY in a game, the AI worker threads are still scanning the live world
    // (CAIMap::UpdateMap / CAIMapUtil::OffsetToXY / pathing). The RemoveAll() teardown below
    // frees that map state out from under them -> AI thread derefs freed memory -> SIGSEGV.
    // Stop+join the AI threads FIRST, exactly as ClearWorld() (the bReplace=TRUE path,
    // newworld.cpp:1409) and ExitInstance() (the quit path) already do. Guarded on m_bInGame
    // because the normal from-menu NewWorld (m_bInGame==FALSE) has no AI threads running yet;
    // the subsequent load restarts them via StartAi(), same as the ClearWorld path.
    if ( m_bInGame )
        myThreadClose( (THREADEXITFUNC)AiExit );

    // LOAD-crash UI-half (newwin's option-B; mac2 loadcrash3 + 4th-path audit): the open game-data
    // dialogs (SDL2BuildingWindow, SDL2BuildTransport, SDL2BuildStructure, unit/building-info dialogs)
    // each hold a CBuilding*/CVehicle*/CUnit* and Refresh it every frame. The RemoveAll() below frees
    // those units WITHOUT nulling the dialogs' pointers -> their next OnFrame derefs a DANGLING ptr ->
    // SIGSEGV (mac2's OnFrame->GetPplTotal stack + the unguarded SDL2BuildTransport::OnFrame). Close
    // them FIRST, exactly as DestroyWorld() already does (newworld.cpp:1260, GameWindow::
    // CloseActiveDialogs). This is signal-INDEPENDENT (no AmInGame timing assumption — verified that
    // AmInGame()==TRUE here) and covers the WHOLE dialog family in one place. The load/status dialog
    // (SDL2CreateStatus) is NOT a registered SDL2Dialog, so it is untouched and keeps driving the load.
    if ( m_bInGame && m_gameWindow )
        m_gameWindow->CloseActiveDialogs();

    // make sure cleared out
    theBridgeMap.RemoveAll();
    theBridgeHex.RemoveAll();
    theBuildingMap.RemoveAll();
    theBuildingHex.RemoveAll();
    theProjMap.RemoveAll();
    theVehicleMap.RemoveAll();
    theVehicleHex.RemoveAll();

    // set up hash tables
    int iNumPlayers = theGame.GetAll().GetCount() + 1;
    theBridgeMap.InitHashTable(GetPrime(iNumPlayers * 20));
    theBridgeHex.InitHashTable(GetPrime(iNumPlayers * 100));
    theBuildingMap.InitHashTable(GetPrime(iNumPlayers * 100));
    theBuildingHex.InitHashTable(GetPrime(iNumPlayers * 100 * 9));
    theProjMap.InitHashTable(GetPrime(iNumPlayers * 2000));
    theVehicleMap.InitHashTable(GetPrime(iNumPlayers * 1000));
    theVehicleHex.InitHashTable(GetPrime(iNumPlayers * 1000 * 2));

    // init the global vars
    ASSERT (m_bInGame == FALSE);
    m_bInGame = TRUE;
    bForceDraw = FALSE;
    bInvAmb = FALSE;
    xiZoom = 0;
    xiDir = 0;
    xpdibwnd = NULL;

    CMaterialTypes::ctor();
    InitColors();

    // put up a creating window and hourglass
    theApp.BaseYield();
    m_pCreateGame->GetDlgStatus()->SetPer(PER_START);

    try {
        // load the data (needed by AI)
        theTerrain.InitData();
        theStructures.InitData();
        theEffects.InitData();
        theTransports.InitData();
        theFlashes.InitData();
        theTurrets.InitData();
        theExplGrp.InitData();

        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_LOAD_TERRAIN);
        theTerrain.InitSprites();
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_LOAD_STRUCTURES);
        theStructures.InitSprites();
        theEffects.InitSprites();
        m_pCreateGame->GetDlgStatus()->SetMsg(IDS_LOAD_TRANSPORTS);
        theTransports.InitSprites();
        theFlashes.InitSprites();
        theTurrets.InitSprites();

        theExplGrp.PostSpriteInit();

        // music
        theMusicPlayer.LoadGroup(SFXGROUP::play);

        theTerrain.InitLang();
        theStructures.InitLang();
        theTransports.InitLang();

        theRsrch.Open();
    }

    catch (int iNum) {
        CatchNum(iNum);
        theApp.CloseWorld();
        return;
    }
    catch (SE_Exception e) {
        TRAP();
        CatchSE(e);
        theApp.CloseWorld();
        return;
    }
    catch (...) {
        TRAP();
        CatchOther();
        theApp.CloseWorld();
        return;
    }

#ifdef _DEBUG
    theDataFile.DisableNegativeSeekChecking ();
#endif

    m_pCreateGame->GetDlgStatus()->SetMsg(IDS_START_AI1);

    // create all the windows (in reverse order of importance)
    m_wndVehicles.Create();
    m_wndBldgs.Create();

    // put it back on top
    theApp.BaseYield();
}

void CConquerApp::RestartWorld() {

    ASSERT (m_bInGame == TRUE);

    DestroyMain();

    theGame.SetShouldProcessMessages(TRUE);
    theGame.SetShouldAnimate(TRUE);
    theGame.SetShouldOperate(TRUE);
    theNet.SetMode(CNetApi::playing);
}

void CConquerApp::DestroyWorld() {

    ASSERT (TestEverything());

    // stop outstanding cache requests
//BUGBUG	theDiskCache.KillAllRequests ();

    // clean out the message queue
    theGame.SetShouldAnimate(FALSE);
    theGame.SetShouldOperate(FALSE);
    theGame.EmptyQueue();

    // Close any open SDL2 non-modal dialogs (Relations/diplomacy, build menus, …)
    // before we tear down the windows and game state they reference. Otherwise an
    // orphaned dialog survives the level, keeps rendering + bumping itself on top
    // every frame, and dangles a pointer into freed game data.
    if (m_gameWindow)
        m_gameWindow->CloseActiveDialogs();

    // we may not be in the game yet
    if (m_pCreateGame != NULL) {
        delete m_pCreateGame;
        m_pCreateGame = NULL;
    }

    // switch the music back
    theMusicPlayer.SoundsOff();

    theNet.SetMode(CNetApi::ending);
    theGame.SetState(CGame::close);
    theGame.SetShouldProcessMessages(FALSE);

    // close down the AI
    myThreadClose((THREADEXITFUNC) AiExit);

    // this can be called when the world doesn't exist
    if (!m_bInGame) {
        theNet.Close(FALSE);
        theGame.Close();
        return;
    }

    m_bInGame = FALSE;

#ifdef _CHEAT
    // CDlgAiPos removed (Phase 2d) — was CHEAT-only AI position debug dialog
#endif

    // close down the net link
    theNet.Close(FALSE);

    // kill all the order windows first
    POSITION pos = theVehicleMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CVehicle *pVeh;
        theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
        ASSERT_VALID (pVeh);
        pVeh->DestroyAllWindows();
    }
    pos = theBuildingMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CBuilding *pBldg;
        theBuildingMap.GetNextAssoc(pos, dwID, pBldg);
        ASSERT_VALID (pBldg);
        pBldg->DestroyAllWindows();
    }

    // destroy all the windows (some point to the below data)
    CloseDlgChat();
    m_wndChat.DestroyWindow();
    if (m_wndVehicles.m_hWnd != NULL)
        m_wndVehicles.RemoveAll();
    if (m_wndBldgs.m_hWnd != NULL)
        m_wndBldgs.RemoveAll();
    m_wndVehicles.DestroyWindow();
    m_wndBldgs.DestroyWindow();
    m_wndWorld.DestroyWindow();
    theAreaList.DestroyAllWindows();
    m_wndBar.DestroyWindow();
    // CDlgResearch removed (Phase 2d) — SDL2ResearchDialog is modal.
    // CDlgFile removed (Phase 2d) — SDL2FileDialog is modal.
    // CDlgPlyrList removed (Phase 2d) — SDL2PlayerListDialog is modal.

    // no more animating
    theAnimList.clear();

    // clean out any messages from DestroyWindow
    //   process existing messages - base class call cause
    //   we don't want processing of our stuff
    BaseYield();

    // we kill again incase we had an updatesounds above
    theMusicPlayer.SoundsOff();

    // kill all the units (some use the below global data)
    // vehicles first (in case in/building buildings)
    pos = theProjMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CProjBase *pPb;
        theProjMap.GetNextAssoc(pos, dwID, pPb);
        while (pPb != NULL) {
            ASSERT_STRICT_VALID (pPb);
            CProjBase *pProj = pPb;
            pPb = theProjMap.GetNext(pPb);
            delete pProj;
        }
    }
    theProjMap.RemoveAll();

    pos = theVehicleMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CVehicle *pVeh;
        theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
        ASSERT_VALID (pVeh);
        delete pVeh;
    }
    theVehicleMap.RemoveAll();

    pos = theBuildingMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CBuilding *pBldg;
        theBuildingMap.GetNextAssoc(pos, dwID, pBldg);
        ASSERT_VALID (pBldg);
        delete pBldg;
    }
    theBuildingMap.RemoveAll();

    ASSERT (theBuildingHex.GetCount() == 0);
    ASSERT (theVehicleHex.GetCount() == 0);
    theBuildingHex.RemoveAll();
    theVehicleHex.RemoveAll();

    // the bridges
    theBridgeMap.DeleteAll();
    theBridgeHex.RemoveAll();

    // close the game
    theGame.Close();

    // free up the global data
    theRsrch.Close();
    theMinerals.Close();
    theMap.Close();

    thePathMgr.Close();

    CMaterialTypes::dtor();

    // clean out the message queue
    theGame.EmptyQueue();
}

void CConquerApp::ClearWorld() {

    theGame.SetShouldAnimate(FALSE);
    theGame.SetShouldOperate(FALSE);

    theNet.SetMode(CNetApi::ending);
    theGame.SetState(CGame::close);

    theMusicPlayer.SoundsOff();
//BUGBUG	theMusicPlayer.PlayExclusiveMusic (MUSIC::GetID (MUSIC::create_game));

    ASSERT (m_bInGame == TRUE);
    if (!m_bInGame)
        return;

    // clean out the message queue
    theGame.SetShouldProcessMessages(FALSE);
    theGame.EmptyQueue();

    // close down the AI
    myThreadClose((THREADEXITFUNC) AiExit);

#ifdef _CHEAT
    // CDlgAiPos removed (Phase 2d) — was CHEAT-only AI position debug dialog
#endif

    // close down the net link
    theNet.Close(FALSE);

    // kill all the order windows first
    POSITION pos = theVehicleMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CVehicle *pVeh;
        theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
        ASSERT_VALID (pVeh);
        pVeh->DestroyAllWindows();
    }
    pos = theBuildingMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CBuilding *pBldg;
        theBuildingMap.GetNextAssoc(pos, dwID, pBldg);
        ASSERT_VALID (pBldg);
        pBldg->DestroyAllWindows();
    }

    // destroy all area windows except topmost
    while (theAreaList.GetCount() > 1) {
        TRAP();
        CWndArea *pWnd = theAreaList.GetTail();
        theAreaList.RemoveTail();
        pWnd->DestroyWindow();
    }
    theAreaList.GetTop()->SelectNone();

    // clean out any messages from DestroyWindow
    //   process existing messages - base class call cause
    //   we don't want processing of our stuff
    BaseYield();

    // kill all the units (some use the below global data)
    // vehicles first (in case in/building buildings)
    pos = theProjMap.GetStartPosition();
    while (pos != NULL) {
        TRAP();
        DWORD dwID;
        CProjBase *pPb;
        theProjMap.GetNextAssoc(pos, dwID, pPb);
        while (pPb != NULL) {
            TRAP();
            ASSERT_STRICT_VALID (pPb);
            CProjBase *pProj = pPb;
            pPb = theProjMap.GetNext(pPb);
            delete pProj;
        }
    }
    theProjMap.RemoveAll();

    pos = theVehicleMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CVehicle *pVeh;
        theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
        ASSERT_VALID (pVeh);
        delete pVeh;
    }
    theVehicleMap.RemoveAll();
    m_wndVehicles.RemoveAll();

    pos = theBuildingMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CBuilding *pBldg;
        theBuildingMap.GetNextAssoc(pos, dwID, pBldg);
        ASSERT_VALID (pBldg);
        delete pBldg;
    }
    theBuildingMap.RemoveAll();
    m_wndBldgs.RemoveAll();

    ASSERT (theBuildingHex.GetCount() == 0);
    ASSERT (theVehicleHex.GetCount() == 0);
    theBuildingHex.RemoveAll();
    theVehicleHex.RemoveAll();

    // the bridges
    theBridgeMap.DeleteAll();
    theBridgeHex.RemoveAll();

    // close the game
    theGame.Close();

    // free up the global data
    theMinerals.Close();
    theMap.Close();
    thePathMgr.Close();

    // clean out the message queue
    theGame.EmptyQueue();
    m_bInGame = TRUE;
}

void CConquerApp::CloseWorld() {

    ASSERT_VALID (this);

    // Restore MFC main window from transparent state before cleanup
    if ( m_wndMain.m_hWnd ) {
        LONG exStyle = ::GetWindowLong( m_wndMain.m_hWnd, GWL_EXSTYLE );
        if ( exStyle & WS_EX_LAYERED )
            ::SetLayeredWindowAttributes( m_wndMain.m_hWnd, 0, 255, LWA_ALPHA );
    }

    theApp.m_wndMain.SetProgPos(CWndMain::game_end);
    theApp.m_wndMain.InvalidateRect(NULL);
    theApp.m_wndMain.UpdateWindow();

    DestroyWorld();
    CreateMain();
}

// CDlgRandNum and CDlgAiPos removed (CHEAT-only debug dialogs)
