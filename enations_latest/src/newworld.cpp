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
#include "player.h"
#include "racedata.h"
#include "new_game.h"
#include "relation.h"
#include "ai.h"
#include "ipccomm.h"
#include "CPathMgr.h"
#include "help.h"
#include "research.h"
#include "chproute.hpp"
#include "bitmaps.h"
#include "sfx.h"
#include "cpathmap.h"
#include "area.h"
#include "bridge.h"
#include "plyrlist.h"
#include "cdloc.h"

#include "terrain.inl"
#include "ui.inl"
#include "unit.inl"
#include "minerals.inl"

extern CPathMgr thePathMgr;

void InitColors();

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


#ifdef _CHEAT
CDlgAiPos dlgAiPos;
#endif

extern DWORD dwFrameCheck;


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

    m_pCreateGame->GetDlgStatus()->UpdateWindow();
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
    CString sName;
    sName.LoadString(IDS_AI_PLAYER);

    for (int iPlyrOn = 0; iPlyrOn < m_pCreateGame->m_iNumAi; iPlyrOn++) {
        CString sBuf(sName);
        CString sNum = IntToCString(iPlyrOn + 1);
        csPrintf(&sBuf, (const char *) sNum);
        CPlayer *pPlr = new CPlayer(sBuf, 0);
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

#ifdef _CHEAT
        // seed override, for testing i assume
        if (theApp.GetProfileInt ("Debug", "SetRand", 0))
            {
            CDlgRandNum dlg;
            dlg.m_sNum = IntToCString (uRand);
            if (dlg.DoModal () == IDOK)
                uRand = atoi (dlg.m_sNum);
            }
#endif

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

    ASSERT_VALID (this);
    ASSERT (theGame.GetMe()->GetNetNum() > 0);
    ASSERT (strlen(theGame.GetMe()->GetName()) > 0);

    // set the status box
    m_pCreateGame->GetDlgStatus()->SetStatus();
    m_pCreateGame->GetDlgStatus()->UpdateWindow();

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

#ifdef _CHEAT
    if (theApp.GetProfileInt ("Debug", "SetRand", 0))
        {
        CDlgRandNum dlg;
        dlg.m_sNum = IntToCString (uRand);
        if (dlg.DoModal () == IDOK)
            uRand = atoi (dlg.m_sNum);
        }
#endif

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

    ASSERT_VALID (m_pCreateGame->GetDlgStatus());
    ASSERT (m_pCreateGame->GetDlgStatus()->m_hWnd != NULL);

#ifdef _DEBUG
    theDataFile.EnableNegativeSeekChecking ();
#endif

    dwFrameCheck = 2 * 1000 / FRAME_RATE;

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

    CUnit::m_sDamage.LoadString(IDS_DAMAGE);
    InitColors( );

    // put up a creating window and hourglass
    m_wndMain.UpdateWindow();
    m_pCreateGame->GetDlgStatus()->SetPer(PER_START);

    // get R&D data
    theRsrch.Open();

    theApp.Log( "Loading terrain data" );
    // load the data (needed by AI)
    theTerrain.InitData();
    theStructures.InitData();
    theEffects.InitData();
    theTransports.InitData();
    theTurrets.InitData();
    theFlashes.InitData();
    theExplGrp.InitData();

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
    }

    // set rand
    MySrand(uRand);

    theApp.Log( "Call StartGame on players" );
    // get the players data ready
    POSITION pos;
    for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL;) {
        CPlayer *pPlr = theGame.GetAll().GetNext(pos);
        ASSERT_VALID (pPlr);
        pPlr->StartGame();
    }

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

    theApp.Log("Init pathmap");
    // set up thePathMap
    int iSize = iSideSize * iSide;
    theApp.BaseYield();
    thePathMap.Init(iSize, iSize);
    theApp.BaseYield();

    try {
        theApp.Log("Load sprites");
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

    // This is where we create the actual map / world / terrain
    // create the map
    theApp.Log("Create the map");
    theMap.Init(iSide, iSideSize, m_pCreateGame->m_iTyp == CCreateBase::scenario);
    theApp.Log("Map created");

    // compare final seeds
    theGame.m_dwFinalRand = MyRand();

    // create path manager
    if (!thePathMgr.Init((iSide * iSideSize), (iSide * iSideSize))) {
        CloseWorld();
        return;
    }

    // create all the windows (in reverse order of importance)
    m_pCreateGame->GetDlgStatus()->SetMsg(IDS_CREATE_WINDOWS);
    m_pCreateGame->GetDlgStatus()->SetPer(PER_CREATE_WINDOWS);

    if (theGame.HaveHP()) {
        m_wndBar.Create();    // must be first to set row3
        m_wndVehicles.Create();
        m_wndBldgs.Create();
        if (theGame.GetAll().GetCount() > theGame.GetAi().GetCount() + 1)
            m_wndChat.Create();
        if (theGame.GetMe()->GetExists(CStructureData::research)) {
            if (m_pdlgRsrch == NULL)
                m_pdlgRsrch = new CDlgResearch(&m_wndMain);
            if (m_pdlgRsrch->m_hWnd == NULL)
                m_pdlgRsrch->Create(IDD_RESEARCH, &m_wndMain);
        }
        CWndArea *pWndArea = new CWndArea();
        pWndArea->Create(theGame.GetMe()->m_hexMapStart, NULL, TRUE);

        // create world for new game (loaded is in player.cpp)
        m_wndWorld.Create(TRUE);        // world must come after area
    }

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
}

void CConquerApp::StartAi() {

#ifdef LOGGINGON
    OutputDebugStringA( "StartAi\n" );
#endif
   // TRACE( "StartAi\n");


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

        POSITION pos;
        // we tell the AI to set itself up.
        for (pos = theGame.GetAi().GetHeadPosition(); pos != NULL;) {
            CPlayer *pPlr = theGame.GetAi().GetNext(pos);
            ASSERT_VALID (pPlr);
            ASSERT (pPlr->IsAI());

#ifdef LOGGINGON
            OutputDebugStringA( "AiSetup\n" );
#endif
#ifdef _CHEAT
           if (!theApp.GetProfileInt ("Debug", "NoThreads", 0))
#endif
            AiSetup(pPlr);

            m_pCreateGame->GetDlgStatus()->SetPer(PER_START_AI + (iOn * PER_NUM_START_AI) / iNum);
            iOn++;
        }

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

        // get rid of the creation dialogs
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
}

static void _OrderWin(CWnd *pWnd, CWnd *pBefore) {

    if ((pWnd == NULL) || (pWnd->m_hWnd == NULL))
        return;

    // set the order
    if (pBefore != NULL)
        pWnd->SetWindowPos(pBefore, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
}

static void _ShowWin(CWnd *pWnd, WINDOWPLACEMENT *pWp) {

    if ((pWnd == NULL) || (pWnd->m_hWnd == NULL))
        return;

    pWnd->InvalidateRect(NULL);

//BUGBUG	if ( ( wp != NULL ) && ( pWp->length >= sizeof (WINDOWPLACEMENT) ) )
//		pWnd->SetWindowPlacement ( pWp );
//	else
    pWnd->ShowWindow(SW_SHOW);
}

static void _UpdateWin(CWnd *pWnd) {

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

    delete m_pCreateGame;
    m_pCreateGame = NULL;
    DestroyExceptMain();

    // chat off if not multi-HP
    if ((theGame.GetMyNetNum() == 0) ||
        (theGame.GetAll().GetCount() <= theGame.GetAi().GetCount() + 1))
        theApp.CloseDlgChat();

    // eric needs AI locations
#ifdef _CHEAT
    if (theGame.AmServer ())
        if (theApp.GetProfileInt ("Debug", "ShowAIStart", 0))
            dlgAiPos.Create (CDlgAiPos::IDD, &m_wndMain);
#endif

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

    // we turn the game on, process all messages, sleep, etc so the windows are ready to GO
    // when we activate them.
    theGame.ProcessAllMessages();
    ::Sleep(500 * theGame.GetAi().GetCount());
    theGame.ProcessAllMessages();
    ::Sleep(500 * theGame.GetAi().GetCount());
    theGame.ProcessAllMessages();

    // if we allow join in process restart enums
    if ((theGame.AmServer()) && (theGame.GetNetJoin() == CGame::any) &&
        (theGame.GetAi().GetCount() > 0))
        theNet.SetSessionVisibility(TRUE);

    // show them
    if (!theGame.HaveHP())
        theApp.ShowPlayerList();
    else {
        if (theGame.GetMe()->GetExists(CStructureData::research)) {
            if (m_pdlgRsrch == NULL)
                m_pdlgRsrch = new CDlgResearch(&m_wndMain);
            if (m_pdlgRsrch->m_hWnd == NULL)
                m_pdlgRsrch->Create(IDD_RESEARCH, &m_wndMain);
        }

        CWnd *pWndChat = (theGame.GetAll().GetCount() > theGame.GetAi().GetCount() + 1) ? &m_wndChat : NULL;
        CWnd *pWndArea = theAreaList.GetTop();
        _OrderWin(&m_wndVehicles, NULL);
        _OrderWin(&m_wndBldgs, &m_wndVehicles);
        _OrderWin(pWndChat, &m_wndBldgs);
        _OrderWin(m_pdlgRsrch, pWndChat != NULL ? pWndChat : &m_wndBldgs);
        _OrderWin(&m_wndWorld, m_pdlgRsrch != NULL ? m_pdlgRsrch : (pWndChat != NULL ? pWndChat : &m_wndBldgs));
        _OrderWin(pWndArea, &m_wndWorld);
        _OrderWin(&m_wndBar, pWndArea);

        _ShowWin(&m_wndBldgs, &(theGame.m_wpBldgs));
        _ShowWin(&m_wndVehicles, &(theGame.m_wpVehicles));
        _ShowWin(pWndChat, &(theGame.m_wpChat));
        _ShowWin(m_pdlgRsrch, &(theGame.m_wpRsrch));
        _ShowWin(&m_wndWorld, &(theGame.m_wpWorld));
        _ShowWin(pWndArea, &(theGame.m_wpArea));
        _ShowWin(&m_wndBar, NULL);

        ((CWndArea *) pWndArea)->Draw();    // GGTESTING

        _UpdateWin(&m_wndBar);
        _UpdateWin(pWndArea);
        _UpdateWin(&m_wndWorld);
        _UpdateWin(m_pdlgRsrch);
        _UpdateWin(pWndChat);
        _UpdateWin(&m_wndVehicles);
        _UpdateWin(&m_wndBldgs);
    }

    theGame.SetElapsedSeconds(0);

    // tell everyone speed we are at
    if (theGame.AmServer() && theGame.IsNetGame()) {
        CMsgGameSpeed msg(theGame.GetGameMul());
        theGame.PostToAll(&msg, sizeof(msg), FALSE);

        // and make us force frames less often so we process messages
        dwFrameCheck = 4 * 1000 / FRAME_RATE;
    }

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
    if (dlgAiPos.m_hWnd != NULL)
        dlgAiPos.DestroyWindow ();
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
    if (m_pdlgRelations != NULL) {
        m_pdlgRelations->DestroyWindow();
        m_pdlgRelations = NULL;
    }
    if (m_pdlgRsrch != NULL) {
        m_pdlgRsrch->DestroyWindow();
        m_pdlgRsrch = NULL;
    }
    if ((m_pdlgFile != NULL) && (m_pdlgFile->m_hWnd != NULL)) {
        m_pdlgFile->DestroyWindow();
        m_pdlgFile = NULL;
    }
    if (m_pdlgPlyrList != NULL) {
        m_pdlgPlyrList->DestroyWindow();
        m_pdlgPlyrList = NULL;
    }

    // no more animating
    theAnimList.RemoveAll();

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
    if (dlgAiPos.m_hWnd != NULL)
        dlgAiPos.DestroyWindow ();
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

    theApp.m_wndMain.SetProgPos(CWndMain::game_end);
    theApp.m_wndMain.InvalidateRect(NULL);
    theApp.m_wndMain.UpdateWindow();

    DestroyWorld();
    CreateMain();
}

#ifdef _CHEAT
/////////////////////////////////////////////////////////////////////////////
// CDlgRandNum dialog


CDlgRandNum::CDlgRandNum(CWnd* pParent /*=NULL*/)
    : CDialog(CDlgRandNum::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDlgRandNum)
    m_sNum = _T("");
    //}}AFX_DATA_INIT
}


void CDlgRandNum::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgRandNum)
    DDX_Text(pDX, IDC_RAND_EDIT, m_sNum);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgRandNum, CDialog)
    //{{AFX_MSG_MAP(CDlgRandNum)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgRandNum message handlers

/////////////////////////////////////////////////////////////////////////////
// CDlgAiPos dialog


CDlgAiPos::CDlgAiPos(CWnd* pParent /*=NULL*/)
    : CDialog(CDlgAiPos::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDlgAiPos)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CDlgAiPos::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgAiPos)
    DDX_Control(pDX, IDOK, m_btnGoto);
    DDX_Control(pDX, IDC_LIST_AI, m_lstBox);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgAiPos, CDialog)
    //{{AFX_MSG_MAP(CDlgAiPos)
    ON_LBN_DBLCLK(IDC_LIST_AI, OnDblclkListAi)
    ON_LBN_SELCHANGE(IDC_LIST_AI, OnSelchangeListAi)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgAiPos message handlers

void CDlgAiPos::OnOK() 
{

    // goto that location - no kill dialog
    OnDblclkListAi ();
}

void CDlgAiPos::OnDblclkListAi() 
{

    int iInd = m_lstBox.GetCurSel ();
    if (iInd < 0)
        return;

    CString sLine;
    m_lstBox.GetText (iInd, sLine);
    CPlayer * pPlyr = theGame.GetPlayerByPlyr (atoi (sLine));
    if (pPlyr == NULL)
        return;

    CWndArea * pWnd = theAreaList.BringToTop ();
    if (pWnd != NULL)
        pWnd->GetAA().SetCenter( CMapLoc( pPlyr->m_hexMapStart ), CAnimAtr::SET_CENTER_SCROLL );
}

void CDlgAiPos::OnSelchangeListAi() 
{

    // goto only enabled if something selected
    if (m_lstBox.GetCurSel () >= 0)
        m_btnGoto.EnableWindow (TRUE);
    else
        m_btnGoto.EnableWindow (FALSE);
}

BOOL CDlgAiPos::OnInitDialog() 
{

    CDialog::OnInitDialog();

    // init list box
    POSITION pos;
    for (pos = theGame.GetAi().GetHeadPosition(); pos != NULL; )
        {
        CPlayer *pPlr = theGame.GetAi().GetNext (pos);
        ASSERT_VALID (pPlr);
        ASSERT (pPlr->IsAI ());
        CString sLine = IntToCString (pPlr->GetPlyrNum ()) + "  " +
                                        IntToCString (pPlr->m_hexMapStart.X()) + "," +
                                        IntToCString (pPlr->m_hexMapStart.Y());
        m_lstBox.AddString (sLine);
        }

    OnSelchangeListAi();

    return TRUE;  // return TRUE unless you set the focus to a control
}
#endif
