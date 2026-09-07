//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "enprobes.h"
#include "ai.h"
#include "altoutput.h"
#include "edicts.h"     // EDICT_DESPERATE_MEASURES (rocket scrounge edict)
#include "aisnap.h"  // Tier-B AI world snapshot (published here, read by AI threads)
#include "area.h"
#include "building.inl"
#include "chproute.hpp"
#include "cpathmgr.h"
#include "cpathmap.h"
#include "cutscene.h"
#include "en_harness.h"   // EnHarness_ServiceMainLoop() — main-loop-safe harness ops (save)
#include "event.h"
#include "GameWindow.h"
#include "Perf.h"
#include "SDL2CreateStatus.h"
#include "SDL2Compositor.h"
#include "SDL2MFCPanel.h"
#include "lastplnt.h"
#include "minerals.inl"
#include "player.h"
#include "relation.h"
#include "research.h"
#include "scenario.h"
#include "sprite.h"
#include "stdafx.h"
#include "terrain.inl"
#include "unit.inl"
#include "vehicle.inl"



#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#define DEBUG_OUTPUT_MAINLOOP 0  // toggles debug output for blockages
#define LOG_VEH 0


extern BOOL bDoSubclass;


// Contenders for the last-player-standing win: every player still in m_lstAll
// EXCEPT declared observers. This is the 1996 predicate (GetAll().GetCount())
// minus spectators - deliberately NOT "players who landed a rocket", because a
// player who has not landed YET has not lost and must still count, or the win
// fires during initial rocket placement (1 AI: count 1 at t=0; MP: the first
// player to land sees it). Only the End-key observer is excluded: it declined
// the rocket, and the 1996 defeat-grace (m_iBuiltBldgsHave = m_bPlacedRocket?0:1)
// keeps it in m_lstAll forever, which is what stopped the AI from ever winning.
static int CountContenders( CList<CPlayer*, CPlayer*>& lst )
{
    int n = 0;
    for ( POSITION pos = lst.GetHeadPosition( ); pos != NULL; )
        if ( !lst.GetNext( pos )->m_bSpectator )
            n++;
    return n;
}

// A game that has only ever had ONE contender (observer watching a single AI,
// or no opponents at all) is decided before it starts. Declare it, but not on
// the first tick - an instant cut-scene at world-create reads as a bug.
// Sim clock, stops when paused. NOTE: it is zeroed on LOAD too, not just at
// create (newworld.cpp:1227 SetElapsedSeconds(0), the sim-clock-debt line, runs
// after deserialize), so this floor RE-ARMS on every load - measured 7950 -> 25
// by MacOpus. Harmless: loading an already-decided game declares its winner 60
// game-seconds later instead of at once. Do NOT assume "a load is past the
// floor" anywhere else.
static const DWORD DECIDED_GRACE_SECS = 60;


#ifdef _PROFILE
extern "C"
{
    void WINAPI MarkStop( );
    void WINAPI MarkStart( );
}
#endif


#define WM_KICKIDLE 0x036A  // from afxpriv.h

// AfxOleGetUserCtrl removed (Phase 4c prep, 2026-05-11) — Enemy Nations
// doesn't use OLE, so /Embedding /Automation aren't possible. The
// original guard `if (m_pMainWnd == NULL && AfxOleGetUserCtrl())` is now
// just `if (m_pMainWnd == NULL)`.

#ifdef _CHEAT
extern BOOL _bShowRate;
#endif


// MM timer data & code

static UINT uRenderTimer = 0;
HANDLE      hRenderEvent = 0;
DWORD       dwFrameCheck = 2 * 1000 / FRAME_RATE;


/////////////////////////////////////////////////////////////////////////////
#ifndef _WIN32
// POSIX MP port (step a): exported from libvdmplay_posix.so — drives the vp*
// select() pump that replaces WSAAsyncSelect. Called from BaseYield each pass.
extern "C" int vpPumpNet( int timeout_ms );
#endif

// CConquerApp::Run - main running routine until application exits

int CConquerApp::Run( )
{

    ASSERT_VALID( this );

    // Profiling harness: reads EN_PERF / EN_PERF_INTERVAL_MS. No-op unless enabled.
    Perf::Init();

    if ( m_pMainWnd == NULL )
    {
        // No main window: quit. (Phase 4c prep — original guard included
        // an AfxOleGetUserCtrl() check that's always TRUE for this app.)
        TRACE0( "Warning: m_pMainWnd is NULL in CWinApp::Run - quitting application.\n" );
        ::PostQuitMessage( 0 );  // Phase 4c prep: was AfxPostQuitMessage
    }

    // we take the critical section and only let it go when we aren't
    // processing our own data. Not the cleanest method in the world
    // but it works
    try
    {

        // Main event loop: SDL2 events take priority, Windows messages are secondary.
        // The loop structure:
        // 1. Check SDL2 events (primary event source when GameWindow exists)
        // 2. Process any pending Windows messages (fallback/housekeeping)
        // 3. Render one frame
        for ( ;; )
        {
            BOOL bQuitReceived = FALSE;

            // Service main-loop-only harness ops (e.g. `save`) here at the loop
            // top, before event-pumping/render — SaveGame re-pumps the event loop,
            // so it must not run from the render-path EnHarness_Service. All
            // platforms; returns immediately unless EN_HARNESS armed the server.
            EnHarness_ServiceMainLoop();

            // Profiling: one "frame" == one outer loop iteration. Cheap no-op
            // unless EN_PERF is set; flushes a perf.log line each interval.
            Perf::FrameMark();

#if EN_PERF_PROBES && defined(_WIN32)
            {
                // hang-regression probe (operator: transient freezes, days-old):
                // one line whenever a MAIN-loop iteration gap exceeds 400ms;
                // correlate timestamps with [SLOWPATH]/[SLOWROAD] to name the blocker
                static DWORD s_dwLastLoop = 0;
                DWORD        dwLoopNow    = timeGetTime( );
                if ( s_dwLastLoop && dwLoopNow - s_dwLastLoop > 400 )
                {
                    char szH[96];
                    sprintf( szH, "[MAINSTALL] %lu ms\n", dwLoopNow - s_dwLastLoop );
                    OutputDebugStringA( szH );
                }
                s_dwLastLoop = dwLoopNow;
            }
#endif

            uint64_t _perfPumpStart = Perf::IsEnabled() ? Perf::Now() : 0;

            // === Phase 3a: INVERTED event loop - SDL2 events first ===
            // Process SDL2 events if game window is active
            if ( m_gameWindow && m_gameWindow->PollEvents() )
            {
                // SDL_QUIT received - post WM_QUIT so shutdown is clean
                ::PostQuitMessage( 0 );
                bQuitReceived = TRUE;
            }

            // Render progress dialog directly (same logic as BaseYield)
            if ( !bQuitReceived && m_gameWindow )
            {
                SDL2CreateStatus* pStatus = m_gameWindow->GetCreateStatus();
                if ( pStatus && pStatus->IsVisible() )
                    pStatus->Render();
            }

            // === Secondary: Windows messages (for system integration, Win32 housekeeping) ===
            // Check for WM_QUIT or other Windows messages if they're pending
            if ( bQuitReceived || BaseYield( ) )
            {
                // BaseYield returned TRUE (WM_QUIT detected) or we already got SDL_QUIT
                if ( ::PeekMessage( &m_msgCur, NULL, NULL, NULL, PM_NOREMOVE ) )
                    if ( m_msgCur.message == WM_QUIT )
                    {
                        ::PeekMessage( &m_msgCur, NULL, NULL, NULL, PM_REMOVE );
#ifdef _DEBUG
                        if ( afxTraceFlags & traceAppMsg )
                            TRACE0( "CWinThread::BaseYield - Received WM_QUIT.\n" );
                        AfxGetThreadState( ) ->m_nDisablePumpCount++;  // application must die
                                                // Note: prevents calling message loop things in 'ExitInstance'
                                                // will never be decremented
#endif
                        return ExitInstance( );
                    }
            }

            if ( Perf::IsEnabled() )
                Perf::SectionEnd( Perf::SEC_PUMP, _perfPumpStart );

            // === Render one frame ===
            {
                Perf::ScopeSlot _perfSim( Perf::SEC_SIM );
                GraphicsEnginePump( );
            }
        }
    }

    catch ( int iNum )
    {
        CatchNum( iNum );
        bDoSubclass = FALSE;

        if ( theGame.GetState( ) == CGame::play )
        {
            theGame.SetState( CGame::error );
            myThreadClose( (THREADEXITFUNC)AiExit );
            if ( EnMessageBox( IDS_SAVE_ON_ERROR, MB_YESNO | MB_ICONQUESTION ) == IDYES )
            {
                TRAP( );
                try
                {
                    theGame.SaveGame( (CWnd*)NULL );
                }
                catch ( ... )
                {
                    EnMessageBox( IDS_SAVE_ON_ERROR_FAILED, MB_OK | MB_ICONSTOP );
                }
            }
        }
        bDoSubclass = TRUE;
        return ( 0 );
    }

    catch ( SE_Exception e )
    {
        CatchSE( e );
        bDoSubclass = FALSE;

        if ( theGame.GetState( ) == CGame::play )
        {
            theGame.SetState( CGame::error );
            myThreadClose( (THREADEXITFUNC)AiExit );
            if ( EnMessageBox( IDS_SAVE_ON_ERROR, MB_YESNO | MB_ICONQUESTION ) == IDYES )
            {
                try
                {
                    theGame.SaveGame( (CWnd*)NULL );
                }
                catch ( ... )
                {
                    EnMessageBox( IDS_SAVE_ON_ERROR_FAILED, MB_OK | MB_ICONSTOP );
                }
            }
        }
        bDoSubclass = TRUE;
        return ( 0 );
    }     
    catch ( ... )
    {
        TRAP( );
        CatchOther( );

#ifdef _DEBUG
        /* try
        {
            throw;
        }
        catch ( CException* e )
        {
            TCHAR szCause[255];
            e->GetErrorMessage( szCause, 255 );
            TRACE( "MFC Exception: %s\n", szCause );
            e->Delete( );
        }
        catch ( const std::exception& e )
        {
            TRACE( "Std Exception: %s\n", e.what( ) );
        }
        catch ( ... )
        {
            TRACE( "Unknown Exception\n" );
        }*/
#endif

        bDoSubclass = FALSE;

        if ( theGame.GetState( ) == CGame::play )
        {
            theGame.SetState( CGame::error );
            myThreadClose( (THREADEXITFUNC)AiExit );
            if ( EnMessageBox( IDS_SAVE_ON_ERROR, MB_YESNO | MB_ICONQUESTION ) == IDYES )
            {
                try
                {
                    theGame.SaveGame( (CWnd*)NULL );
                }
                catch ( ... )
                {
                    EnMessageBox( IDS_SAVE_ON_ERROR_FAILED, MB_OK | MB_ICONSTOP );
                }
            }
        }
        bDoSubclass = TRUE;
        return ( 0 );
    }
    

    ASSERT( FALSE );  // not reachable
}

// VTBUGBUG this probably shouldn't be here...

    // clears out the message queue and calls OnIdle once - no graphics engine stuff
// [mp-plyr] trace (netapi.cpp) — reused by the MP start-handshake watchdog below.
extern void EnMpDiagLog( const char* fmt, ... );

// Grace period the host waits for every joined client to report CNetInitDone
// (-> CPlayer::wait) before it force-drops the laggards and starts anyway. The
// host has already built its OWN world by the time it reaches wait_AI; clients
// build in parallel, so this only needs to cover a slow client's world-build.
static const DWORD MP_WAIT_AI_TIMEOUT_MS = 90000;

// MP start-handshake watchdog (host only). StartAi fires only once EVERY non-AI
// player has reported ready (CmdInitDone -> CPlayer::wait); a joined client that
// quits / fails to load / is a stale duplicate lobby link never reports, so the
// host hangs forever at "Waiting for Others". After the grace period, drop the
// non-responding players to AI (replace) and start, so one bad client can't
// deadlock the whole game. Cheap no-op unless we're a host actually in wait_AI.
static void MpStartHandshakeWatchdog( )
{
    static DWORD s_dwWaitStart = 0;
    static DWORD s_dwLastLog   = 0;

    if ( !theGame.AmServer( ) || !theGame.HaveHP( ) ||
         theGame.GetState( ) != CGame::wait_AI )
    {
        s_dwWaitStart = 0;   // reset outside the wait phase
        return;
    }

    DWORD dwNow = GetTickCount( );
    if ( s_dwWaitStart == 0 )
    {
        s_dwWaitStart = dwNow;
        s_dwLastLog   = dwNow;
        return;
    }

    DWORD dwElapsed = dwNow - s_dwWaitStart;

    // heartbeat every ~15s so the wait is observable in the log
    if ( dwNow - s_dwLastLog >= 15000 )
    {
        s_dwLastLog = dwNow;
        EnMpDiagLog( "MP-WATCHDOG: still waiting for InitDone (%ds/%ds) before force-start",
                     (int)( dwElapsed / 1000 ), (int)( MP_WAIT_AI_TIMEOUT_MS / 1000 ) );
    }

    if ( dwElapsed <= MP_WAIT_AI_TIMEOUT_MS )
        return;

    // Timed out: drop every non-AI player that never reported ready (not wait /
    // replace) to replace, so StartAi's replace loop AI-takes-them-over. Never
    // touch the host itself.
    BOOL     bDropped = FALSE;
    POSITION pos;
    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
        ASSERT_VALID( pPlr );
        if ( !pPlr->IsAI( ) && !pPlr->IsMe( ) &&
             pPlr->GetState( ) != CPlayer::wait &&
             pPlr->GetState( ) != CPlayer::replace )
        {
            EnMpDiagLog( "MP-WATCHDOG: plyr=%d name='%s' netnum=%d never reported ready in %ds -> dropping to AI (replace)",
                         pPlr->GetPlyrNum( ), pPlr->GetName( ), pPlr->GetNetNum( ),
                         (int)( MP_WAIT_AI_TIMEOUT_MS / 1000 ) );
            pPlr->SetState( CPlayer::replace );
            bDropped = TRUE;
        }
    }

    if ( !bDropped )
        return;   // nothing to drop (shouldn't happen, but don't force-start blind)

    // Re-run the readiness gate (mirror CmdInitDone's tail): if every non-AI
    // player is now wait/replace, we can start.
    for ( pos = theGame.GetAll( ).GetTailPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.GetAll( ).GetPrev( pos );
        ASSERT_VALID( pPlr );
        if ( ( !pPlr->IsAI( ) ) && ( pPlr->GetState( ) != CPlayer::wait ) &&
             ( pPlr->GetState( ) != CPlayer::replace ) )
            return;   // still someone outstanding — keep waiting
    }

    s_dwWaitStart = 0;   // consumed; StartAi leaves wait_AI so we won't re-fire
    EnMpDiagLog( "MP-WATCHDOG: laggards dropped, all remaining players ready -> StartAi (force-start)" );
    theApp.StartAi( );
}

BOOL CConquerApp::BaseYield( )
{

    ASSERT_STRICT_VALID( this );

    MpStartHandshakeWatchdog( );

#ifndef _WIN32
    // POSIX (MP port, step a): drive the vp* select() pump before draining the
    // message queue. On Windows a hidden message window converted socket events
    // to WM_WINSOCK messages inside this same PeekMessage loop; there is no such
    // window on POSIX, so we poll the armed sockets here. Any notifications the
    // pump produces are PostMessage'd as WM_VPNOTIFY and dispatched to
    // CNetApi::OnNetMsg by the drain loop below, in this same pass. No-op (cheap
    // empty-registry check) when no MP sockets are armed, so SP/menus pay nothing.
    vpPumpNet( 0 );
#endif

    // for tracking the idle time state
    static BOOL bIdle      = TRUE;
    static LONG lIdleCount = 0;

    // test first for WM_QUIT
    while ( ::PeekMessage( &m_msgCur, NULL, NULL, NULL, PM_NOREMOVE ) )
    {
#ifdef _DEBUG
        if ( AfxGetThreadState( ) ->m_nDisablePumpCount != 0 )
        {
            TRACE0( "Error: CWinThread::BaseYield called when not permitted.\n" );
            ASSERT( FALSE );
        } 
#endif

        if ( m_msgCur.message == WM_QUIT )
        {
#ifdef _DEBUG
            if ( afxTraceFlags & traceAppMsg )
                TRACE0( "CWinThread::BaseYield - Received WM_QUIT.\n" );
            AfxGetThreadState( ) ->m_nDisablePumpCount++;  // application must die
                                    // Note: prevents calling message loop things in 'ExitInstance'
                                    // will never be decremented
#endif
            return ( TRUE );
        }

        // now remove it
        ::PeekMessage( &m_msgCur, NULL, NULL, NULL, PM_REMOVE );

        // process this message
        if ( m_msgCur.message != WM_KICKIDLE && !PreTranslateMessage( &m_msgCur ) )
        {

            // handle accelerators
            BOOL bTran = FALSE;
            if ( ( WM_KEYFIRST <= m_msgCur.message ) && ( m_msgCur.message <= WM_KEYLAST ) )
                if ( theGame.GetState( ) == CGame::play )
                {
                    CWndPrimary* pWndPrimary = (CWndPrimary*)CWndBase::GetActiveWindow( );
                    if ( pWndPrimary != NULL )
                    {
                        if ( thePrimaryMap.find( pWndPrimary ) != thePrimaryMap.end() )
                            if ( pWndPrimary->m_hAccel != NULL )
                                bTran = TranslateAccelerator( pWndPrimary->m_hWnd, pWndPrimary->m_hAccel, &m_msgCur );
                    }

                    // see if any of the global accelerators were pressed
                    if ( !bTran )
                        bTran = TranslateAccelerator( m_wndMain.m_hWnd, m_hAccel, &m_msgCur );
                }

            // process this message
            if ( !bTran )
            {
                ::TranslateMessage( &m_msgCur );
                ::DispatchMessage( &m_msgCur );
            }

            // reset "no idle" state after pumping "normal" message
            if ( IsIdleMessage( &m_msgCur ) )
            {
                bIdle      = TRUE;
                lIdleCount = 0;
            }
        }

        // call idle if appropiate
        if ( bIdle )
            if ( !OnIdle( lIdleCount++ ) )
                bIdle = FALSE;  // assume "no idle" state

        theMusicPlayer.YieldPlayer( );
    }

    // Process SDL events (also called from Run() — re-entrancy guard in PollEvents prevents double-processing)
    if ( m_gameWindow && m_gameWindow->PollEvents() )
    {
        // SDL_QUIT received - post WM_QUIT so MFC shuts down normally
        ::PostQuitMessage( 0 );
        return ( TRUE );
    }

    // Render progress dialog directly — PollEvents() has a re-entrancy guard that
    // blocks rendering when world creation is triggered from inside PollEvents()
    // (e.g. main menu button -> SDL2_RunCreateSinglePlayerFlow -> ReadyToCreate ->
    //  BaseYield -> PollEvents skipped -> Render never called).
    // Calling Render() here bypasses that guard.
    if ( m_gameWindow )
    {
        SDL2CreateStatus* pStatus = m_gameWindow->GetCreateStatus();
        if ( pStatus && pStatus->IsVisible() )
            pStatus->Render();
    }

    return FALSE;
}

// clears out the message queue and calls OnIdle once - and runs our engine
BOOL CConquerApp::FullYield( )
{

    // message queue and OnIdle
    if ( BaseYield( ) )
        return ( TRUE );

    GraphicsEnginePump( );
    return FALSE;
}

// calc yield and render only if necessary
BOOL CConquerApp::YieldAndRenderNoEvent( )
{

    int iCheck = 0;

    // see if we need to render
    iCheck++;
    if ( iCheck < 25 )
        return ( FALSE );
    iCheck = 0;

    DWORD dwNow = timeGetTime( );

    // is it time yet?
    if ( dwNow < theApp.m_dwMaxNextRender )
    {
        TRAP( );
        return ( FALSE );
    }

    theApp.BaseYield( );
    ::Sleep( 0 );
    // BUGBUG theApp._RenderScreens ();
    TRAP( );
    return ( TRUE );
}

// calc yield and render only if necessary
BOOL CConquerApp::YieldAndRenderEvent( )
{

    if ( WaitForSingleObject( hRenderEvent, 0 ) != WAIT_OBJECT_0 )
        return ( FALSE );

    uRenderTimer = 0;

    theApp.BaseYield( );
    ::Sleep( 0 );
    // BUGBUG theApp._RenderScreens ();
    return ( TRUE );
}

BOOL CConquerApp::CheckYield( )
{

    if ( m_bUseEvents == 1 )
        return ( YieldAndRenderEvent( ) );
    if ( m_bUseEvents == 0 )
        return ( YieldAndRenderNoEvent( ) );
    return ( FALSE );
}

void CConquerApp::ProcessAllMessages( DWORD dwBudgetMs )
{

    if ( !theGame.ShouldProcessMessages() )
        return;

    // Time-box support (see lastplnt.h): game-start unit-creation storms
    // (veh_new x per-AI fan-out; 5,670 msgs/s at 9 AIs, worse at 15) held a
    // single frame 30-45s in ONE drain (EN_PERF msg=32,298ms) — input pumped
    // via CheckYield so clicks/sounds worked while the display froze AND the
    // hidden new-game windows never got their render-loop reveal (operator's
    // "crash": live process, zero visible windows). Operator-approved fix.
    const DWORD dwDrainStart = dwBudgetMs ? timeGetTime( ) : 0;

    // process all messages so we have none pending
    while ( TRUE )
    {
        if ( dwBudgetMs && ( timeGetTime( ) - dwDrainStart >= dwBudgetMs ) )
            break;                      // budget spent: render a frame, resume next pump

        EnterCriticalSection( &cs );
        if (theGame.m_messagePointerList.GetCount( ) <= 0 )
        {
            LeaveCriticalSection( &cs );
            break;
        }
        char* pBuf = (char*)theGame.m_messagePointerList.RemoveHead( );

#ifdef LOGGINGON
        char dbgBuf[80];
        _snprintf_s( dbgBuf, sizeof(dbgBuf), _TRUNCATE, "Processing type %d\n", ( (CNetCmd*)pBuf )->GetType( ) );
        OutputDebugStringA( dbgBuf );
        OutputDebugStringA( "\n" );
#endif

        if ( pBuf == NULL )
        {
            LeaveCriticalSection( &cs );
            break;
        }
#if EN_PERF_PROBES && defined(_WIN32)
        {
            // hang-regression probe: the drain budget checks BETWEEN messages,
            // so one slow handler = one multi-second frame (t=632: 9,958ms in
            // ProcessAllMessages under a 400ms budget). Name the message type.
            DWORD dwMsgT0  = timeGetTime( );
            int   iMsgType = (int)( (CNetCmd*)pBuf )->GetType( );
            theGame.ProcessMessage( (CNetCmd*)pBuf );
            DWORD dwMsgMs = timeGetTime( ) - dwMsgT0;
            if ( dwMsgMs > 250 )
            {
                char szM[80];
                sprintf( szM, "[SLOWMSG] type %d took %lu ms\n", iMsgType, dwMsgMs );
                OutputDebugStringA( szM );
            }
        }
#else
        theGame.ProcessMessage((CNetCmd *) pBuf);
#endif
        theGame.FreeQueueElement((CNetCmd *) pBuf);

        // throttle messages back on if a net game
        if ( ( theGame.IsNetGame( ) ) && ( theGame.ShouldPause() ) ) {
            if (theGame.m_messagePointerList.GetCount( ) <= MIN_NUM_MESSAGES ) {
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

        // see if we need to render
        if ( CheckYield( ) )
            if ( !theGame.ShouldProcessMessages() )
                return;
    }
}

void CConquerApp::RenderScreens( )
{

    DWORD dwNow = timeGetTime( );

    // is it time yet?
    if ( dwNow < m_dwNextRender )
        return;

    _RenderScreens( );
}

void CConquerApp::_RenderScreens( )
{

    DWORD dwNow               = timeGetTime( );
    div_t dtFrame             = div( dwNow - theGame.m_dwFrameTimeLast, 1000 / FRAME_RATE );
    theGame.m_dwFramesElapsed = dtFrame.quot;
    theGame.m_dwFrameTimeLast = dwNow + dtFrame.rem;

    if ( !theGame.ShouldAnimate() )
    {
        m_dwMaxNextRender = theGame.m_dwFrameTimeLast + dwFrameCheck;
        return;
    }

    theGame.m_dwFrame++;

    {
        Perf::ScopeSlot _perfRender( Perf::SEC_RENDER );

        // animate the screen. Most game windows repaint at a reduced rate
        // (ANIM_THROTTLE_MS) so they don't steal frames from the simulation;
        // the area map and radar override RendersEveryFrame() to stay smooth.
        // DecideRenderFrame() (called in the ReRender pass) records the choice
        // so the Draw pass skips the same windows.
        DWORD ANIM_THROTTLE_MS = 1000 / 7;  // 7 fps for throttled windows
        // While the user is dragging or resizing a window, render every window at
        // the full frame rate so the move/resize tracks the cursor smoothly
        // instead of stepping at the throttled ~7 fps.
        if ( theApp.m_gameWindow )
        {
            SDL2Compositor* pc = theApp.m_gameWindow->GetCompositor();
            if ( pc && pc->AnyPanelInteracting() )
                ANIM_THROTTLE_MS = 0;
        }
        DWORD dwAnimNow = timeGetTime( );
        {
            Perf::ScopeCounter _ci( "r.inval" );   // invalidate pass (theMap.Update)
            for ( CWndAnim* pWnd : theAnimList )
            {
                // SPLIT (2026-06-11): r.inval read 118-224ms/s but the per-window
                // ReRender counters (rr.area + rr.radar) only accounted for ~40ms —
                // attribute the rest: the decide step vs the un-counted ReRender
                // prologues (everything before the rr.* scopes inside ReRender).
                bool bGo;
                { Perf::ScopeCounter _cd( "ri.decide" );
                  bGo = pWnd->DecideRenderFrame( dwAnimNow, ANIM_THROTTLE_MS ); }
                if ( bGo )
                { Perf::ScopeCounter _cr( "ri.rerender" );
                  pWnd->ReRender( ); }
            }
        }
        {
            Perf::ScopeCounter _cd( "r.draw" );    // draw pass (UpdateRect walk + capture)
            for ( CWndAnim* pWnd : theAnimList )
                if ( pWnd->RenderingThisFrame( ) )
                    pWnd->Draw( );
        }

        // Item 5 (dirty-rects) de-risk probe: how many hexes were invalidated this
        // frame (sim moves + render-time marks). If this is O(moving-units) and not
        // O(visible-hexes), the push-based dirty-rect source is viable.
        Perf::CounterAdd( "inval.hexes", theMap.GetHexValidMatrix( )->GetDirtyCount( ) );

        CHexCoord::ClearInvalidated( );  // Set terrain invalidated flags to FALSE

        if ( theApp.m_wndBar.m_sdlPanel &&
             theApp.m_wndBar.DecideRenderFrame( dwAnimNow, ANIM_THROTTLE_MS ) )
            theApp.m_wndBar.Draw();
    }

    // SDL2 compositor: composite all panels to window surface and present.
    // This replaces CDIBWnd::Update()'s GDI BitBlt for the SDL2 path.
    if ( theApp.m_gameWindow )
    {
        Perf::ScopeSlot _perfPresent( Perf::SEC_PRESENT );
        SDL2Compositor* pCompositor = theApp.m_gameWindow->GetCompositor();
        if ( pCompositor )
            pCompositor->Composite();
    }

// show the frame rate
#ifdef _CHEAT
    {
        static DWORD dwLastTime = 0;

        if ( _bShowRate )
            if ( !theAnimList.empty( ) )
            {
                DWORD dwTemp;
                if ( dwNow > dwLastTime )
                    dwTemp = 10000L / ( dwNow - dwLastTime );
                else
                {
                    TRAP( );
                    dwTemp = 10000L;
                }
                div_t   dtRate = div( dwTemp, 10 );
                char sText[128];
                _snprintf_s( sText, sizeof(sText), _TRUNCATE, "_  %d.%d fps", dtRate.quot, dtRate.rem );
                std::string dbg = sText;
                if ( theGame.AreMessagesPaused( ) )
                    dbg += "  Msgs PAUSED";
                if ( theGame.ShouldPause( ) )
                    dbg += "  Should PAUSED";
                theApp.m_wndBar.SetDebugText( 0, dbg.c_str() );
                dwLastTime = dwNow;
            }
    }
#endif

    // for when to come in next
    m_dwMaxNextRender = timeGetTime( ) + dwFrameCheck;

    if ( m_bUseEvents == 1 )
    {
        if ( uRenderTimer != 0 )
            timeKillEvent( uRenderTimer );
        ResetEvent( hRenderEvent );
        uRenderTimer = timeSetEvent( dwFrameCheck, dwFrameCheck / 4, (LPTIMECALLBACK)hRenderEvent, 0,
                                     TIME_ONESHOT | TIME_CALLBACK_EVENT_SET );
    }
}

// runs a frame of our graphics engine (and also does a bunch of logic..)
void CConquerApp::GraphicsEnginePump( )
{

#ifdef _PROFILE
    DWORD dwMarkStart = timeGetTime( );
    MarkStart( );
#endif

    // process messages — time-boxed so a storm can't starve rendering, but
    // ADAPTIVE: when the backlog is deep (game-start unit flood) spend more of
    // each pump draining so the player's own commands (e.g. the rocket-place
    // message queued BEHIND the AI traffic — operator: "2 min before my rocket
    // lands, AI units crossing my screen") catch up in seconds, while still
    // rendering ≥2-3 fps. Shallow queue = 100ms (normal ~7-10fps worst case).
    {
        Perf::ScopeSlot _perfMsg( Perf::SEC_MSG );
        const int   iBacklog = theGame.m_messagePointerList.GetCount( );
        const DWORD dwBudget = iBacklog > 2000 ? 400 : ( iBacklog > 500 ? 200 : 100 );
        ProcessAllMessages( dwBudget );
    }

    theGame._SettimeGetTime( );

    // OP-CLOCK SANITY: m_dwOperTimeLast/m_dwFrameTimeLast are SERIALIZED into saves as
    // raw timeGetTime() values — milliseconds since the SAVING session's boot. Loading a
    // save from a different boot epoch (machine rebooted since, or just a different
    // uptime) can restore a value in the FUTURE of this session's clock; the tick gate
    // below then rejects every frame: rendering runs normally while units never move
    // (user repro on Save13-Player, 2026-06-11). In-session writers only ever move the
    // clocks backward-or-equal to "now", so future values are always stale loads (or the
    // 49.7-day timeGetTime wrap) — clamp them to now and the sim resumes immediately.
    if ( theGame.m_dwOperTimeLast > theGame.GettimeGetTime( ) )
        theGame.m_dwOperTimeLast = theGame.GettimeGetTime( );
    if ( theGame.m_dwFrameTimeLast > theGame.GettimeGetTime( ) )
        theGame.m_dwFrameTimeLast = theGame.GettimeGetTime( );

    // if we haven't used up 1/24 of a second - leave
    if ( theGame.GettimeGetTime( ) < theGame.m_dwOperTimeLast + 1000 / FRAME_RATE )
    {
#ifdef _PROFILE
        if ( timeGetTime( ) > dwMarkStart + 2000 )
            MarkStop( );
#endif

        // do we need to render
        if ( theGame.GettimeGetTime( ) >= m_dwNextRender )
            _RenderScreens( );

        // sleep if we're playing
        if (theGame.ShouldOperate() )
        {
            int dwNow        = timeGetTime( );
            int dwOperSleep  = (int)theGame.m_dwOperTimeLast + 1000 / FRAME_RATE - dwNow;
            int dwFrameSleep = (int)m_dwMaxNextRender - dwNow;
            int dwSleep      = __min( dwOperSleep, dwFrameSleep );

            // was stopping here? why?
           // TRAP( dwSleep > 0 );

            // Render/sim decouple: only PARK the thread when we're genuinely ahead of the
            // next sim tick (dwSleep > 0) — then a short sleep paces us without spinning.
            // When a single render overran the 1/24s sim period (dwSleep <= 0, i.e. we're
            // render-bound and already behind), the old code still forced a 10ms floor here,
            // throwing away ~10ms/frame for nothing and delaying the next render/sim. In that
            // case just yield (Sleep(0)) so AI/network worker threads still get scheduled but
            // we immediately loop back to render again. (The sim-tick path keeps its own
            // explicit AI time slice below, so AI is not starved.)
            Perf::ScopeSlot _perfSleep( Perf::SEC_SLEEP );
            if ( dwSleep > 0 )
                ::Sleep( __minmax( 1, 1000 / FRAME_RATE, dwSleep ) );
            else
                ::Sleep( 0 );   // render-bound: yield without the 10ms penalty
        }
        return;
    }

    // give network & AI a chance
    if ( theGame.HaveAI( ) )
    {
        // if 1/12 of a second or better - give the AI half of it
        int iExtra =
            ( (int)( 2 * 1000 / FRAME_RATE ) - (int)( theGame.GettimeGetTime( ) - theGame.m_dwOperTimeLast ) ) / 2;
        Perf::ScopeSlot _perfSleep( Perf::SEC_SLEEP );
        ::Sleep( __minmax( 10, 2 * 1000 / FRAME_RATE, iExtra ) );
    }
    else
        { Perf::ScopeSlot _perfSleep( Perf::SEC_SLEEP ); ::Sleep( 10 ); }  // give network some time

    // animate if 1/24 of a second has passed
    div_t dtFrame             = div( theGame.GettimeGetTime( ) - theGame.m_dwOperTimeLast, 1000 / FRAME_RATE );
    theGame.m_dwFramesElapsed = dtFrame.quot;
    theGame.m_dwOperTimeLast  = theGame.GettimeGetTime( ) - dtFrame.rem;
    theGame.m_dwOpersElapsed  = theGame.m_dwFramesElapsed * theGame.m_iSpeedMul;

    if (theGame.ShouldOperate() )
    {
        // time played
        theGame.m_dwElapsedTime += theGame.m_dwOpersElapsed;

        // every 15 seconds we check for number of buildings
        static int iFifteen = 0;

        // we enter this code once a second (note - we enter once a real second regardless
        // of the game speed but we have more frames at higher speeds)
        theGame.m_dwOperSecFrames += theGame.m_dwOpersElapsed;
        if ( theGame.m_dwOperSecFrames >= (DWORD)( FRAME_RATE * theGame.m_iSpeedMul ) )
        {
            div_t dtNum                = div( theGame.m_dwOperSecFrames, FRAME_RATE );
            theGame.m_dwOperSecElapsed = dtNum.quot;
            theGame.m_dwOperSecFrames  = dtNum.rem;

            // every 15 seconds count the building
            if ( theGame.AmServer( ) )
            {
                iFifteen++;
                if ( iFifteen >= 15 )
                {
                    iFifteen = 0;

                    // zero it out
                    POSITION pos;
                    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
                    {
                        CPlayer* pPlr           = theGame.GetAll( ).GetNext( pos );
                        pPlr->m_iBuiltBldgsHave = pPlr->m_bPlacedRocket ? 0 : 1;
                    }

                    // count them
                    pos = theBuildingMap.GetStartPosition( );
                    while ( pos != NULL )
                    {
                        DWORD      dwID;
                        CBuilding* pBldg;
                        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
                        if ( !pBldg->IsConstructing( ) )
                            pBldg->GetOwner( )->m_iBuiltBldgsHave++;
                    }
                }
            }

            // unstick the HP material router's truck/ship rendezvous
            // (event-driven state machine; blocked arrivals need a retry pass)
            theGame.m_pHpRtr->Pump( );

            // update the time
            if ( theGame.IsNetGame( ) && theGame.AmServer( ) )
            {
                static DWORD dwLast = 0;
                DWORD        dwNow  = theGame.GetElapsedSeconds( );
                if ( ( dwLast & ~31 ) != ( dwNow & ~31 ) )
                {
                    dwLast = dwNow;
                    CMsgSetTime msg( dwNow );
                    theGame.PostToAllClients( &msg, sizeof( msg ) );
                }
            }

            EnterCriticalSection( &cs );

            // game over?
            if ( !theGame.HaveHP( ) )
            {
                // if only AI players left - it's over
                if ( theGame.GetAll( ).GetCount( ) <= theGame.GetAi( ).GetCount( ) )
                {
                    LeaveCriticalSection( &cs );
                    // CDlgPlyrList removed (Phase 2d) — SDL2PlayerListDialog is modal so
                    // no game-state hide hook is needed.
                    EnMessageBox( IDS_GAME_OVER, MB_OK | MB_ICONSTOP );
                    theApp.CloseWorld( );
                    return;
                }
            }
            else

            {
                // if shareware they only get a 90 minutes
                if ( m_bTimeLimit )
                    if ( theGame.GetElapsedSeconds( ) > DEMO_SINGLE_TIME_LIMIT )
                    {
                        LeaveCriticalSection( &cs );
                        TRAP( );
                        if ( EnMessageBox( IDS_TIME_OUT, MB_YESNO | MB_ICONQUESTION ) == IDYES )
                            theGame.SaveGame( &m_wndMain );
                        theApp.CloseWorld( );
                        return;
                    }

                if ( ( theGame.GetMe( )->GetBldgsHave( ) <= 0 ) || ( theGame.GetMe( )->m_iBuiltBldgsHave == 0 ) )
                {
                    theGame.SetState( CGame::other );
                    LeaveCriticalSection( &cs );

                    // special mode if this is the server for a net game
                    if ( theGame.IsNetGame( ) && theGame.AmServer( ) &&
                         ( theGame.GetAll( ).GetCount( ) > theGame.GetAi( ).GetCount( ) + 1 ) )
                    {
                        // we're gone
                        theGame.SetHP( FALSE );

                        // put up the game control list
                        theApp.ShowPlayerList( );

                        // show the lose scene
                        theCutScene.PlayEnd( CWndCutScene::lose, TRUE );

                        // close the game windows
                        m_wndVehicles.DestroyWindow( );
                        m_wndBldgs.DestroyWindow( );
                        m_wndChat.DestroyWindow( );
                        m_wndWorld.DestroyWindow( );
                        theAreaList.DestroyAllWindows( );
                        m_wndBar.DestroyWindow( );
                        // CDlgResearch removed (Phase 2d) — modal SDL2 dialog.
                        return;
                    }

                    theCutScene.PlayEnd( CWndCutScene::lose );
                    theApp.CloseWorld( );
                    return;
                }

                if ( ( CountContenders( theGame.GetAll( ) ) <= 1 ) &&
                     ( theGame.GetElapsedSeconds( ) >= DECIDED_GRACE_SECS ) )
                {
                    LeaveCriticalSection( &cs );
                    theCutScene.PlayEnd( CWndCutScene::win );
                    theApp.CloseWorld( );
                    return;
                }

                // scenario end conditions
                if ( theGame.GetScenario( ) >= 0 )
                {
                    int iRtn = ScenarioEnd( );
                    if ( iRtn < 0 )
                    {
                        LeaveCriticalSection( &cs );
                        theCutScene.PlayEnd( CWndCutScene::lose );
                        theApp.CloseWorld( );
                        return;
                    }

#ifdef _CHEAT
                    if ( theGame.GetScenario( ) < _iScenarioOn )
                    {
                        CWndArea* pWnd = theAreaList.GetTop( );
                        if ( pWnd != NULL )
                            if ( ( pWnd->GetMode( ) != CWndArea::rocket_ready ) &&
                                 ( pWnd->GetMode( ) != CWndArea::rocket_pos ) &&
                                 ( pWnd->GetMode( ) != CWndArea::rocket_wait ) )
                                iRtn = 1;
                    }
#endif

                    if ( iRtn > 0 )
                    {
                        // turn off while doing this
                        theGame.SetShouldOperate(FALSE);
                        theGame.SetShouldAnimate(FALSE);
                        LeaveCriticalSection( &cs );

                        theCutScene.PlayEnd( CWndCutScene::scenario_end );

                        // next scenario
                        theGame.IncScenario( );

                        // only 5 scenarios
                        if ( ( ( theApp.IsShareware( ) ) || ( theApp.IsSecondDisk( ) ) ) &&
                             ( theGame.GetScenario( ) > 4 ) )
                        {
                            TRAP( );
                            if ( EnMessageBox( IDS_TIME_OUT, MB_YESNO | MB_ICONQUESTION ) == IDYES )
                                theGame.SaveGame( &m_wndMain );
                            theApp.m_wndCutScene.DestroyWindow( );
                            theApp.CloseWorld( );
                            return;
                        }

                        if ( theGame.GetScenario( ) < NUM_SCENARIOS )
                        {
                            ScenarioStart( );
                            if ( theCutScene.PlayCutScene( theGame.GetScenario( ), FALSE ) != IDOK )
                            {
                                theApp.CloseWorld( );
                                return;
                            }
                        }
                        else
                        {
                            // we play a special one telling them about the net
                            theCutScene.PlayEnd( CWndCutScene::win );
                            theCutScene.PlayCutScene( NUM_SCENARIOS, TRUE );
                            theApp.CloseWorld( );
                            return;
                        }

                        theGame.SetShouldAnimate(TRUE);
                        theGame.SetShouldOperate(TRUE);
                        EnterCriticalSection( &cs );
                    }
                }
            }

            // (CDlgStats removed; was a _CHEAT-only diagnostic dialog)

            // grow the population
            // eat the food
            // research tech
            POSITION pos;
            for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
            {
                CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
                ASSERT_STRICT_VALID( pPlr );
                if ( pPlr->IsLocal( ) )
                {
                    pPlr->PeopleAndFood( theGame.GetOperSecElapsed( ) );
                    pPlr->Research( theGame.GetOperSecElapsed( ) );
                }

                // if we're the server decide if a player is dead
                if ( theGame.AmServer( ) )
                    if ( ( pPlr->GetBldgsHave( ) <= 0 ) || ( pPlr->m_iBuiltBldgsHave == 0 ) )
                    {
                        // kill all their buildings & vehicles (if they have them)
                        pos = theBuildingMap.GetStartPosition( );
                        while ( pos != NULL )
                        {
                            DWORD      dwID;
                            CBuilding* pBldg;
                            theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
                            ASSERT_STRICT_VALID( pBldg );
                            if ( pBldg->GetOwner( ) == pPlr )
                            {
                                ASSERT( FALSE );
                                pBldg->PrepareToDie( NULL );
                            }
                        }
                        pos = theVehicleMap.GetStartPosition( );
                        while ( pos != NULL )
                        {
                            DWORD     dwID;
                            CVehicle* pVeh;
                            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
                            ASSERT_STRICT_VALID( pVeh );
                            if ( pVeh->GetOwner( ) == pPlr )
                                pVeh->PrepareToDie( NULL );
                        }

                        // now tell them to delete the player (will follow above delete messages)
                        CMsgPlyrDying msg( pPlr );
                        theGame.PostToAll( &msg, sizeof( msg ) );
                    }
            }

            LeaveCriticalSection( &cs );

            // Profiling: sample key object counts once per game-second so we
            // can correlate slowdown with unbounded growth. Uses the same
            // position-walk idiom as the operate loops above; only runs when
            // EN_PERF is set.
            if ( Perf::IsEnabled() )
            {
                int64_t nBldg = 0, nVeh = 0, nProj = 0;
                {
                    POSITION p = theBuildingMap.GetStartPosition( );
                    while ( p != NULL )
                    {
                        DWORD dwID; CBuilding* pB;
                        theBuildingMap.GetNextAssoc( p, dwID, pB );
                        ++nBldg;
                    }
                }
                {
                    POSITION p = theVehicleMap.GetStartPosition( );
                    while ( p != NULL )
                    {
                        DWORD dwID; CVehicle* pV;
                        theVehicleMap.GetNextAssoc( p, dwID, pV );
                        ++nVeh;
                    }
                }
                {
                    POSITION p = theProjMap.GetStartPosition( );
                    while ( p != NULL )
                    {
                        DWORD dwID; CProjBase* pP;
                        theProjMap.GetNextAssoc( p, dwID, pP );
                        for ( ; pP != NULL; pP = theProjMap.GetNext( pP ) )
                            ++nProj;
                    }
                }
                Perf::GaugeSet( "obj.bldgs", nBldg );
                Perf::GaugeSet( "obj.vehs",  nVeh );
                Perf::GaugeSet( "obj.projs", nProj );
                Perf::GaugeSet( "obj.anims", (int64_t)theAnimList.size() );

                // Exact (un-sampled) leak probes:
                //  path.cells  = live nodes in the two pathfinders' CCell scratch
                //                maps (should sit near 0; rising = a clear is missed)
                //  mfc.iterpos = net-live CMap/CList iterator wrappers (rising =
                //                the per-advance iterator leak)
                extern CPathMap thePathMap;
                extern CPathMgr thePathMgr;
                extern long     g_mfcIterPosLive;
                extern int      AiTotalPathCells( );
                // Include the per-AI path maps: AI pathing now runs on per-AI
                // CPathMap instances, so the two globals no longer see it.
                Perf::GaugeSet( "path.cells",  (int64_t)( thePathMap.GetMapCellCount() + thePathMgr.GetMapCellCount() + AiTotalPathCells() ) );
                Perf::GaugeSet( "mfc.iterpos", (int64_t)g_mfcIterPosLive );
            }

            // the status bars
            if ( theGame.HaveHP( ) )
            {
                m_wndBar.UpdateTime( );

                // update if the gas needs have changed
                static int iLastGasNeed, iLastGasHave;
                BOOL       bLow = theGame.GetMe( )->GetGasNeed( ) > theGame.GetMe( )->GetGasHave( );
                if ( bLow || ( iLastGasNeed != theGame.GetMe( )->GetGasNeed( ) ) ||
                     ( iLastGasHave != theGame.GetMe( )->GetGasHave( ) ) )
                {
                    iLastGasNeed = theGame.GetMe( )->GetGasNeed( );
                    iLastGasHave = theGame.GetMe( )->GetGasHave( );
                    if ( bLow )
                        m_wndBar.FlashLowIcon( CWndBar::gas );
                    m_wndBar.UpdateGas( );
                }

#ifdef _CHEAT
                if ( _bMaxPower )
                    theGame.GetMe( )->AddPwrHave( 64000 );
#endif

                // update if the power needs have changed || it's low
                static int iLastPwrNeed, iLastPwrHave;
                bLow = theGame.GetMe( )->GetPwrNeed( ) > theGame.GetMe( )->GetPwrHave( );
                if ( bLow || ( iLastPwrNeed != theGame.GetMe( )->GetPwrNeed( ) ) ||
                     ( iLastPwrHave != theGame.GetMe( )->GetPwrHave( ) ) )
                {
                    iLastPwrNeed = theGame.GetMe( )->GetPwrNeed( );
                    iLastPwrHave = theGame.GetMe( )->GetPwrHave( );
                    if ( bLow )
                        m_wndBar.FlashLowIcon( CWndBar::power );
                    m_wndBar.UpdatePower( );
                }

                // update if the people needs have changed (or flashing red)
                static int iLastPplNeed, iLastPplHave;
                BOOL       bPpl = FALSE;
                bLow            = theGame.GetMe( )->GetPplNeedBldg( ) > theGame.GetMe( )->GetPplBldg( );
                if ( bLow || ( iLastPplNeed != theGame.GetMe( )->GetPplNeedBldg( ) ) ||
                     ( iLastPplHave != theGame.GetMe( )->GetPplBldg( ) ) )
                {
                    iLastPplNeed = theGame.GetMe( )->GetPplNeedBldg( );
                    iLastPplHave = theGame.GetMe( )->GetPplBldg( );
                    if ( bLow )
                        m_wndBar.FlashLowIcon( CWndBar::people );
                    m_wndBar.UpdatePeople( );
                    bPpl = TRUE;
                }

                // update if the food supply has changed
                static int iLastFood;
                bLow = theGame.GetMe( )->GetFoodNeed( ) > theGame.GetMe( )->GetFood( );
                if ( bLow || bPpl || ( iLastFood != theGame.GetMe( )->GetFood( ) ) )
                {
                    iLastFood = theGame.GetMe( )->GetFood( );
                    if ( bLow )
                        m_wndBar.FlashLowIcon( CWndBar::food );
                    m_wndBar.UpdateFood( );
                }
            }  // HaveHP

            if ( CheckYield( ) )
                if ( !theGame.ShouldOperate() )
                    goto NoOper;
        }  // 1 second code

        // got stuck waiting here?
        // take the critical section while we do our thing
        EnterCriticalSection( &cs );

        // figure the multipliers, etc
        POSITION pos;
        for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
        {
            CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
            ASSERT_STRICT_VALID( pPlr );
            pPlr->StartLoop( );
        }

        // operate the buildings
        {
        Perf::ScopeSlot _perfOperB( Perf::SEC_OPER_B );
        pos = theBuildingMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD      dwID;
            CBuilding* pBldg;
            theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
            ASSERT_STRICT_VALID( pBldg );
#if EN_PERF_PROBES && defined(_WIN32)
            {
                // lag probe: perf.log shows the recurring ~10s frames spend
                // ~10,000ms in THIS loop (operB) - name the building
                DWORD dwB0 = timeGetTime( );
                pBldg->Operate( );
                DWORD dwBMs = timeGetTime( ) - dwB0;
                if ( dwBMs > 500 )
                {
                    char szB[96];
                    sprintf( szB, "[SLOWBLDG] plyr %d bldg %lu type %d took %lu ms\n",
                             pBldg->GetOwner( ) ? pBldg->GetOwner( )->GetPlyrNum( ) : -1,
                             (unsigned long)dwID, pBldg->GetData( ) ? (int)pBldg->GetData( )->GetType( ) : -1,
                             dwBMs );
                    OutputDebugStringA( szB );
                }
            }
#else
            pBldg->Operate( );
#endif
        }
        }

        // operate the vehicles
        {
        Perf::ScopeSlot _perfOperV( Perf::SEC_OPER_V );   // dtor runs even on the goto below
        pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            ASSERT_STRICT_VALID( pVeh );
            pVeh->Operate( );

            // see if we need to render
            if ( CheckYield( ) )
            {
                if ( !theGame.ShouldOperate() )
                {
                    LeaveCriticalSection( &cs );
                    goto NoOper;
                }
                // get our position again (may have changed)
                CVehicle* pVehWasOn = pVeh;
                pos                 = theVehicleMap.GetStartPosition( );
                while ( pos != NULL )
                {
                    theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
                    if ( pVeh == pVehWasOn )
                        break;
                }
            }
        }
        }

        // operate the projectiles
        {
        Perf::ScopeSlot _perfOperP( Perf::SEC_OPER_P );
        pos = theProjMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD      dwID;
            CProjBase* pPb;
            theProjMap.GetNextAssoc( pos, dwID, pPb );
            ASSERT_STRICT_VALID( pPb );
            while ( pPb != NULL )
            {
                ASSERT_STRICT_VALID( pPb );
                pPb->Operate( );
                pPb = theProjMap.GetNext( pPb );
            }
        }
        }

        // see if we need to render
        if ( CheckYield( ) )
            if ( !theGame.ShouldOperate() )
            {
                TRAP( );
                LeaveCriticalSection( &cs );
                goto NoOper;
            }

        // post built up messages
        if ( !theGame.CheckAreMessagesPaused() )
        {
            //   buildings
            CMsgCompUnitDamage    msgDam;
            CMsgCompUnitSetDamage msgSetDam;
            pos = theBuildingMap.GetStartPosition( );
            while ( pos != NULL )
            {
                DWORD      dwID;
                CBuilding* pBldg;
                theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
                ASSERT_STRICT_VALID( pBldg );

                // damage
                int iDam = pBldg->GetDamageThisTurn( );
                if ( iDam != 0 )
                {
                    int iNum = msgDam.AddUnit( pBldg, iDam );
                    if ( iNum >= NUM_UNIT_DAMAGE_ELEM )
                    {
                        TRAP( );
                        theGame.PostToServer( &msgDam, sizeof( msgDam ) );
                        msgDam.Reset( );
                    }
                }
                if ( pBldg->IsFlag( CUnit::unit_set_damage ) )
                {
                    TRAP( !theGame.AmServer( ) );
                    pBldg->ClrUnitSetDamage( );
                    int iNum = msgSetDam.AddUnit( pBldg );
                    if ( iNum >= NUM_UNIT_SET_DAMAGE_ELEM )
                    {
                        TRAP( );
                        theGame.PostToAllClients( &msgSetDam, sizeof( msgSetDam ), FALSE );
                        msgSetDam.Reset( );
                    }
                }
            }

            // vehicles
            CMsgVehCompLoc msgLoc;
            pos = theVehicleMap.GetStartPosition( );
            while ( pos != NULL )
            {
                DWORD     dwID;
                CVehicle* pVeh;
                theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
                ASSERT_STRICT_VALID( pVeh );

                // movement
                if ( ( pVeh->IsNewLoc( ) ) && ( !( pVeh->GetFlags( ) & CUnit::dying ) ) )
                {
                    pVeh->NewLocOff( );
                    int iNum = msgLoc.AddVeh( pVeh );
                    if ( iNum >= NUM_LOC_ELEM )
                    {
                        theGame.PostToAllClients( &msgLoc, sizeof( msgLoc ), FALSE );
                        msgLoc.Reset( );
                    }
                }

                // damage
                int iDam = pVeh->GetDamageThisTurn( );
                if ( iDam != 0 )
                {
                    int iNum = msgDam.AddUnit( pVeh, iDam );
                    if ( iNum >= NUM_UNIT_DAMAGE_ELEM )
                    {
                        TRAP( );
                        theGame.PostToServer( &msgDam, sizeof( msgDam ) );
                        msgDam.Reset( );
                    }
                }
                if ( pVeh->IsFlag( CUnit::unit_set_damage ) )
                {
                    TRAP( !theGame.AmServer( ) );
                    pVeh->ClrUnitSetDamage( );
                    int iNum = msgSetDam.AddUnit( pVeh );
                    if ( iNum >= NUM_UNIT_SET_DAMAGE_ELEM )
                    {
                        TRAP( );
                        theGame.PostToAllClients( &msgSetDam, sizeof( msgSetDam ), FALSE );
                        msgSetDam.Reset( );
                    }
                }
            }

            // may have units left over
            if ( msgLoc.m_iNumMsgs > 0 )
                theGame.PostToAllClients( &msgLoc, msgLoc.SendSize( ), FALSE );
            if ( msgDam.m_iNumMsgs > 0 )
                theGame.PostToServer( &msgDam, msgDam.SendSize( ) );
            if ( msgSetDam.m_iNumMsgs > 0 )
                theGame.PostToAllClients( &msgSetDam, msgSetDam.SendSize( ), FALSE );
            if ( theGame.m_msgShoot.m_iNumMsgs > 0 )
            {
                theGame.PostToAll( &( theGame.m_msgShoot ), theGame.m_msgShoot.SendSize( ), FALSE );
                theGame.m_msgShoot.Reset( );
            }
        }

        LeaveCriticalSection( &cs );

        // process messages from Operate calls (same time-box as the pump head)
        ProcessAllMessages( 100 );
    }  // if operate

NoOper:

    // Tier-B AI snapshot: publish the world copy the AI threads read lock-free
    // (self-throttled to ~tick cadence; takes `cs` briefly inside). Sits after
    // both the operate and skip paths so it runs every loop iteration.
    AiSnap::Publish( );

    // where we should render (if the above was fast)
    RenderScreens( );

    // TODO: SDL UI rendering disabled - need to integrate with game's DirectDraw system
    // if (m_gameWindow) {
    //     m_gameWindow->UpdateStatusBar();
    //     m_gameWindow->Render();
    // }

#ifdef _PROFILE
    if ( timeGetTime( ) > dwMarkStart + 2000 )
        MarkStop( );
#endif
}

struct tagDROP
{
    CHexCoord hex;
    int       iMov;
    int       iWheel;
};

static int fnEnumDrop( CHex* pHex, CHexCoord _hex, void* pData )
{

    // can't/must be water
    struct tagDROP* pD = (struct tagDROP*)pData;
    if ( ( ( pD->iWheel == CWheelTypes::water ) && ( !pHex->IsWater( ) ) ) ||
         ( ( pD->iWheel != CWheelTypes::water ) && ( pHex->IsWater( ) ) ) )
        return ( FALSE );

    // can't be a building
    if ( pHex->GetUnits( ) & CHex::bldg )
        return ( FALSE );

    CTerrainData const* pTd = &theTerrain.GetData( pHex->GetType( ) );
    ASSERT_STRICT_VALID( pTd );

    if ( ( pTd->GetWheelMult( pD->iWheel ) > 0 ) && ( pTd->GetWheelMult( pD->iWheel ) < pD->iMov ) )
    {
        pD->iMov = pTd->GetWheelMult( pD->iWheel );
        pD->hex  = _hex;
    }

    return ( FALSE );
}

CHexCoord CBuilding::GetExit( int iWheelTyp )
{

    CHexCoord hex( ( iWheelTyp == CWheelTypes::water ) ? GetShipHex( ) : GetExitHex( ) );

    switch ( ( iWheelTyp == CWheelTypes::water ) ? GetShipDir( ) : GetExitDir( ) )
    {
    case 0:
        hex.Ydec( );
        break;
    case 1:
        hex.Xinc( );
        break;
    case 2:
        hex.Yinc( );
        break;
    case 3:
        hex.Xdec( );
        break;
    }

    return ( hex );
}

void CBuilding::Operate( )
{

    ASSERT_STRICT_VALID( this );

    // check for material changes
    if ( m_iUpdateMat != 0 )
        UpdateStore( FALSE );

    // only if we are local
    if ( !GetOwner( )->IsLocal( ) )
        return;

    if ( m_iFrameHit > 0 )
    {
        m_iFrameHit -= theGame.GetFramesElapsed( );
        if ( m_iFrameHit <= 0 )
        {
            m_iFrameHit = 0;
            theApp.m_wndWorld.SetBldgHit( );
        }
    }

    if ( ( m_unitFlags & dying ) || ( GetOwner( ) == NULL ) )
        return;

    // we handle building fire rates here because it depends on power & people
    if ( GetData( )->_GetFireRate( ) > 0 )
    {
        // #60: a FINISHED armed building still defends even if production is `stopped` (operator:
        // enemy buildings should fire when visible + in-range + at-war, same as the player's).
        // Under-construction (m_iConstDone != -1) buildings still can't fire. The `stopped` term
        // was silencing 15 finished enemy camps (barracks_2) persisted `stopped` in saved games.
        // Reads only network-synced state (no RandNum/time) → MP-deterministic. newwin-concurred.
        if ( m_iConstDone != -1 )
            m_iFireRate = 0;
        else
        {
            float fRate = GetFrameProd( 1.0 );
            if ( fRate < 0.05 )
                m_iFireRate = 0;
            else
                m_iFireRate = (int)( (float)GetData( )->_GetFireRate( ) / fRate );
        }
    }

    // are we destroying?
    if ( m_unitFlags & destroying )
    {
        int iInc = GetProdNoDamage( 8 * GetOwner( )->GetConstProd( ) );
        iInc     = __max( 1, iInc );
        if ( iInc > 0 )
            AddDamageThisTurn( this, iInc );
    }

    // if paused, return
    if ( m_unitFlags & ( stopped | abandoned ) )
    {
#if EN_AI_PROBES_ECON && defined(_WIN32)
        // AI vehicle producers stuck paused (persisted `stopped` from the save,
        // same phenomenon as #60; the AI never sets nor clears this flag)
        if ( GetOwner( ) != NULL && GetOwner( )->IsAI( ) &&
             GetData( )->GetUnionType( ) == CStructureData::UTvehicle &&
             ( ( theGame.GettimeGetTime( ) / 5000 + GetID( ) ) % 24 ) == 0 )
        {
            char szP[112];
            sprintf( szP, "[FACTSTOPPED] plyr %d bldg %lu type %d constdone %d flags %x\n",
                     GetOwner( )->GetPlyrNum( ), (unsigned long)GetID( ), GetData( )->GetType( ),
                     m_iConstDone, (unsigned)m_unitFlags );
            OutputDebugStringA( szP );
        }
#endif
        // Fracking (#23): an EXHAUSTED oil well is stopped+abandoned and so would idle
        // here. If its per-well alt-output toggle (alt_oil) is ON and the owner has
        // researched Fracking, instead of idling it draws power and trickles oil. The
        // AltOutput "Fracking" def (eFlatTrickle) matches exactly this state; FrackTick()
        // applies the +50% well energy and credits the trickle via AltOutput::Convert.
        // Toggle OFF / no tech / non-oil-well => this is a no-op and the well idles
        // exactly as before (byte-identical), falling through to the normal stopped path.
        if ( ( m_iConstDone == -1 )
             && IsFlag( CUnit::alt_oil )
             && ( GetData( )->GetUnionType( ) == CStructureData::UTmine )
             && AltOutput::Available( this ) )
        {
            ( (CMineBuilding*)this )->FrackTick( );
            AnimateOperating( TRUE );        // fracking/moho: keep the well animating...
            SetAmbientHalfSpeed( TRUE );     // ...at half speed
            return;
        }

        // an exhausted well not fracking shows no animation (undo a prior frack-enable)
        if ( ( m_unitFlags & abandoned ) && ( GetData( )->GetUnionType( ) == CStructureData::UTmine ) )
        {
            SetAmbientHalfSpeed( FALSE );
            AnimateOperating( FALSE );
        }

        // if stopped we only need half the people & power
        if ( ( m_iConstDone == -1 ) && ( !( m_unitFlags & abandoned ) ) )
        {
            GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) / 2 );
            GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) / 2 );
        }
        m_fOperMod = 0;

        // #60 (dispatch half; pairs with the fire-rate fix above): a FINISHED, armed,
        // non-abandoned building still DEFENDS while production is `stopped` — this
        // return used to skip HandleCombat() entirely, so enemy camps persisted
        // `stopped` in saved games computed fr>0 but never dispatched a shot.
        // Combat only; production stays paused. Abandoned buildings don't fight,
        // and `event` (out-of-materials wait) is excluded for exact parity with the
        // running path, which returns before HandleCombat when event is set (:1634).
        if ( ( m_iConstDone == -1 )
             && ( !( m_unitFlags & ( abandoned | event ) ) )
             && ( GetData( )->_GetFireRate( ) > 0 ) )
            HandleCombat( );
        return;
    }

    // are we repairing
    if ( ( GetOwner( )->IsLocal( ) ) && ( m_iRepairWork > 0 ) && ( !IsFlag( repair_stop ) ) )
    {
        TRAP( m_unitFlags & event );
        // can we use it?
        for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
            if ( GetStore( iInd ) < NeedToRepair( iInd, m_aiRepair[iInd] ) )
            {
#if EN_AI_PROBES_ECON && defined(_WIN32)
                {
                    static DWORD s_dwNextRepStopLog = 0;
                    if ( GetOwner( )->IsAI( ) && theGame.GettimeGetTime( ) >= s_dwNextRepStopLog )
                    {
                        s_dwNextRepStopLog = theGame.GettimeGetTime( ) + 5000;
                        char szRs[144];
                        sprintf( szRs, "[REPSTOP] plyr %d bldg %lu mat %d store %d need %d work %d dmg %d\n",
                                 GetOwner( )->GetPlyrNum( ), (unsigned long)GetID( ), iInd, GetStore( iInd ),
                                 NeedToRepair( iInd, m_aiRepair[iInd] ), m_iRepairWork, GetDamagePer( ) );
                        OutputDebugStringA( szRs );
                    }
                }
#endif
                SetFlag( repair_stop );
                memset( m_aiRepair, 0, sizeof( m_aiRepair ) );

                MaterialChange( );
                // tell the router/AI
                if ( GetOwner( )->IsMe( ) )
                    theGame.m_pHpRtr->MsgOutMat( this );
                else
                    MaterialMessage( );
                m_iRepairWork = 0;
                m_iRepairMod  = 0;
                memset( &m_aiRepair[0], 0, sizeof( m_aiRepair ) );
                goto RepairDone;
            }

        // use the materials
        for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
        {
            int iMat = NeedToRepair( iInd, m_aiRepair[iInd] );
            if ( iMat > 0 )
            {
                AddToStore( iInd, -iMat );
                GetOwner( )->IncMaterialHave( iInd, -iMat );
                m_aiRepair[iInd] -= PointsRepaired( iInd, iMat );
            }
        }

        // need to convert from time repairing to points repaired
        div_t dtFix = div(
            (int)( m_iRepairMod + m_iRepairWork * GetData( )->GetDamagePoints( ) * 2 ),
            GetData( )->GetTimeBuild( ) );
        m_iRepairMod  = dtFix.rem;
        m_iRepairWork = 0;
#if EN_AI_PROBES_ECON && defined(_WIN32)
        if ( GetOwner( )->IsAI( ) && dtFix.quot > 0 )
        {
            static DWORD s_dwNextRepAppLog = 0;
            if ( theGame.GettimeGetTime( ) >= s_dwNextRepAppLog )
            {
                s_dwNextRepAppLog = theGame.GettimeGetTime( ) + 5000;
                char szRa[112];
                sprintf( szRa, "[REPAPPLY] plyr %d bldg %lu pts %d dmg %d\n", GetOwner( )->GetPlyrNum( ),
                         (unsigned long)GetID( ), dtFix.quot, GetDamagePer( ) );
                OutputDebugStringA( szRa );
            }
        }
#endif
        CMsgUnitRepair msg( this, dtFix.quot );
        theGame.PostToServer( &msg, sizeof( msg ) );
    }
RepairDone:;

    // if waiting on an event, return -- UNLESS this is a refinery running BioFuel (Bio Oil).
    // A refinery out of OIL gets SetFlag(event) (see BuildMaterials, "if we ran out - stop us")
    // and would idle here every tick, BEFORE BuildMaterials -- so the BioFuel food->oil branch
    // (which lives IN BuildMaterials and needs no oil) never ran. That's the operator's bug:
    // "turned Bio Oil on, plenty of food, but it's not producing" -- you enable Bio Oil exactly
    // because you're out of oil, i.e. exactly when the well is stuck in `event`. Clear the
    // oil-wait for a BioFuel-active refinery and fall through so it operates off global food.
    if ( m_unitFlags & event )
    {
        const bool bBioFuelActive = ( m_iConstDone == -1 )
                                 && IsFlag( CUnit::alt_oil )
                                 && ( GetData( )->GetUnionType( ) == CStructureData::UTmaterials )
                                 && ( AltOutput::Available( this ) != nullptr );
        if ( bBioFuelActive )
        {
            ClrFlag( event );   // no longer waiting on oil -- BioFuel operates off global food
        }
        else
        {
#if EN_AI_PROBES_ECON && defined(_WIN32)
            // which factories sit event-wedged (halt saved into the game; the
            // material request that would clear it did not survive the load)
            static DWORD s_dwNextWedgeLog = 0;
            if ( m_iConstDone == -1 && GetOwner( )->IsAI( ) &&
                 GetData( )->GetUnionType( ) == CStructureData::UTvehicle &&
                 theGame.GettimeGetTime( ) >= s_dwNextWedgeLog )
            {
                s_dwNextWedgeLog = theGame.GettimeGetTime( ) + 15000;
                char szW[128];
                sprintf( szW, "[FACTWEDGE] plyr %d bldg %lu type %d stores %d/%d/%d\n",
                         GetOwner( )->GetPlyrNum( ), (unsigned long)GetID( ), GetData( )->GetType( ),
                         GetStore( 0 ), GetStore( 1 ), GetStore( 2 ) );
                OutputDebugStringA( szW );
            }
#endif
            // if stopped we only need half the people & power
            if ( m_iConstDone == -1 )
            {
                GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) / 2 );
                GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) / 2 );
            }
            m_fOperMod = 0;
            return;
        }
    }

    // are we still building?
    if ( m_iConstDone != -1 )
    {
        Construct( );
        return;
    }

    // first we handle combat stuff
    HandleCombat( );

    // special case - rockets generate free power
    if ( GetData( )->GetType( ) == CStructureData::rocket )
        GetOwner( )->AddPwrHave( (int)( 15.0 * GetFrameProd( 1 ) ) );

    // ok its built - now it has to operate
    switch ( GetData( )->GetUnionType( ) ) // union is like building class
    {
    case CStructureData::UTmaterials: // generates minerals
        BuildMaterials( );
        break;
    case CStructureData::UTvehicle: // builds vehicles
        ( (CVehicleBuilding*)this )->BuildVehicle( );
        break;
    case CStructureData::UTmine:
        ( (CMineBuilding*)this )->BuildMine( );
        break;
    case CStructureData::UTfarm:
        ( (CFarmBuilding*)this )->BuildFarm( );
        break;
    case CStructureData::UThousing:
        break;
    case CStructureData::UTwarehouse:
        // A warehouse/rocket normally just draws power + people. Two scrounge modes add a small
        // multi-resource trickle: the ROCKET via the Desperate Measures civ-wide EDICT (net-synced,
        // lost on rocket death); the WAREHOUSE via its per-building AltOutput toggle (Scrounging).
        GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) );
        if ( ( GetData( )->GetType( ) == CStructureData::rocket )
             && GetOwner( )->IsEdictActive( EDICT_DESPERATE_MEASURES ) )
        {
            // +100 draft workers; fixed 10 lumber / 5 iron / 5 food / 5 coal per minute.
            // Keep in sync with the "Cost: 100 workers" line in g_aEdicts (edicts.cpp).
            GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) + 100 );
            static const AltOutput::AltMat aDesperate[4] =
                { { CMaterialTypes::lumber, 10 }, { CMaterialTypes::iron, 5 },
                  { CMaterialTypes::food, 5 },    { CMaterialTypes::coal, 5 } };
            AltOutput::CreditTrickle( this, (int)theGame.GetOpersElapsed( ), m_afAltAccum, aDesperate, 4 );
        }
        else
        {
            GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) );
            if ( IsFlag( CUnit::alt_oil ) && ( AltOutput::Available( this ) != nullptr ) )
                AltOutput::ConvertMulti( this, (int)theGame.GetOpersElapsed( ), m_afAltAccum );
        }
        break;
    case CStructureData::UTrepair:
        ( (CRepairBuilding*)this )->BuildRepair( );
        break;
    case CStructureData::UTshipyard:
        ( (CShipyardBuilding*)this )->BuildShipyard( );
        break;

    // R&D from this building
    case CStructureData::UTresearch: {
        if ( GetOwner( )->GetRsrchItem( ) != 0 )
        {
            GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) );
            GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) );
        }
        else
        {
            GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) / 2 );
            GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) / 2 );
        }

        CBuildResearch* pBr  = GetData( )->GetBldResearch( );
        // Apply the per-player research productivity multiplier (m_fRsrchProd, from
        // CRaceDef::research + the AI difficulty 0.8 scale). This was set/serialized but
        // never consumed â€” the lone race attribute with no live consumer â€” so per-race
        // research bonuses and the AI research handicap silently did nothing. Honoring it
        // here (the productivity point) matches how mines/farms/etc. apply their own
        // GetMineProd/GetFarmProd multipliers.
        float           fTmp = pBr->GetRate( ) * m_fDamPerfMult * GetOwner( )->GetPplMult( ) *
                               GetOwner( )->GetRsrchMult( );
        GetOwner( )->AddRsrch( (int)fTmp );
        break;
    }

    // determine the power from this building
    case CStructureData::UTpower:
        ( (CPowerBuilding*)this )->BuildPower( );
        break;

    default:
        // add in its power & people usuage
        GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) );
        GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) );
        break;
    }

    // #2 (operator): an AltOutput mode more labor-intensive than the base building (the
    // charcoal kiln vs. just cutting) draws EXTRA workforce while ON -- an ABSOLUTE per-def
    // amount (m_iWorkforceAdd), since a lumber mill's base GetPeople() is ~0 so a percent
    // added nothing (verified via pstats). Applied centrally after the per-type tick.
    if ( IsFlag( CUnit::alt_oil ) )
    {
        const AltOutput::AltOutputDef* pAo = AltOutput::Available( this );
        if ( pAo && pAo->m_iWorkforceAdd > 0 )
            GetOwner( )->AddPplNeedBldg( pAo->m_iWorkforceAdd );
        // Extra power (Scrounging doubles the warehouse's draw): m_iPowerMultAdd EXTRA copies of
        // base power. Base power was already added by the per-type tick, so add only the delta.
        if ( pAo && pAo->m_iPowerMultAdd > 0 )
            GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) * pAo->m_iPowerMultAdd );
    }

    ASSERT_STRICT_VALID( this );
}

void CBuilding::SetConstPer( )
{

    // update spotting if alliance
    if ( ( !GetOwner( )->IsMe( ) ) && ( GetOwner( )->GetTheirRelations( ) == RELATIONS_ALLIANCE ) &&
         ( m_iVisConstDone != -1 ) && ( m_iConstDone == -1 ) )
    {
        m_iVisConstDone = -1;
        DecrementSpotting( );
        DetermineSpotting( );
        IncrementSpotting( m_hex );
    }

    m_iVisConstDone = m_iConstDone;
    m_iVisFoundPer  = m_iFoundPer;
    m_iVisSkltnPer  = m_iSkltnPer;
    m_iVisFinalPer  = m_iFinalPer;
}

// the vehicles actually inc the %, this controls materials usuage, sprite %, etc.
void CBuilding::Construct( )
{

    ASSERT_STRICT( ( m_unitFlags & ( stop | event ) ) == 0 );
    ASSERT_STRICT( m_iConstDone != -1 );

    // see if we are completely done
    //   the >= 100 is so all the minerals are taken
    if ( ( m_iLastMaterialTake >= 100 ) && ( m_iConstDone >= m_iFoundTime + GetData( )->GetTimeBuild( ) ) )
    {
        // stop all vehicles from building
        CVehicle::StopConstruction( this );
        if ( GetOwner( )->IsMe( ) )
            theGame.Event( EVENT_CONST_DONE, EVENT_NOTIFY, this );

        m_iLastPer   = 100;
        m_iConstDone = -1;
        EventOff( );

        // redraw if we can see it
        if ( ( GetOwner( )->IsMe( ) ) || ( theMap.GetHex( m_hex )->GetVisible( ) ) )
        {
            PauseAnimations( FALSE );
            SetInvalidated( );
            SetConstPer( );
        }

        // tell everyone
        CMsgBldgStat msg( this );
        msg.m_iFlags = CMsgBldgStat::built;
        theGame.PostToAllClients( &msg, sizeof( msg ) );
        MaterialChange( );

        if ( GetOwner( )->IsMe( ) )
        {
            theGame.m_pHpRtr->MsgBuiltBldg( this );
            if ( ( GetData( )->GetUnionType( ) == CStructureData::UTmaterials ) ||
                 ( GetData( )->GetUnionType( ) == CStructureData::UTpower ) )
                theGame.m_pHpRtr->MsgOutMat( this );
        }

        // if it's AI tell it it needs materials
        if ( ( GetData( )->GetUnionType( ) == CStructureData::UTmaterials ) ||
             ( GetData( )->GetUnionType( ) == CStructureData::UTpower ) )
            MaterialMessage( );

        // done - they can move in
        if ( GetData( )->GetUnionType( ) == CStructureData::UThousing )
        {
            if ( GetData( )->GetBldgType( ) == CStructureData::apartment )
                GetOwner( )->m_iAptCap += GetOwner( )->GetHousingCap( GetData( )->GetPopHoused( ) );
            else if ( GetData( )->GetBldgType( ) == CStructureData::office )
                GetOwner( )->m_iOfcCap += GetOwner( )->GetHousingCap( GetData( )->GetPopHoused( ) );
        }

        // if first research facility
        BOOL bRsrch;
        if ( GetData( )->GetType( ) == CStructureData::research )
            bRsrch = !GetOwner( )->GetExists( CStructureData::research );
        else
            bRsrch = FALSE;

        // set as built
        GetOwner( )->AddExists( GetData( )->GetType( ), 1 );

        // get building ready to go
        m_iBuildDone = 0;
        m_fOperMod   = 0;
        ConstComplete( );

        // if first research facility - put R&D window up
        if ( GetOwner( )->IsMe( ) )
            ResearchDiscovered( 0 );
        if ( ( bRsrch ) && ( GetOwner( )->IsMe( ) ) )
            theApp.m_wndBar._GotoScience( TRUE );   // alert: pop above map + focus

        // if command center turn on the radar
        if ( ( GetOwner( )->IsMe( ) ) && ( GetData( )->GetType( ) == CStructureData::command_center ) &&
             ( GetOwner( )->GetExists( CStructureData::command_center ) == 1 ) )
        {
            theApp.m_wndWorld.CommandCenterChange( );
            theGame.Event( EVENT_HAVE_RADAR, EVENT_NOTIFY );
        }

        // if embassy bring up relations
        if ( GetData( )->GetType( ) == CStructureData::embassy )
            theApp.m_wndBar.GotoRelations( );

        // set it's visibility
        if ( GetOwner( )->IsMe( ) )
        {
            DecrementSpotting( );
            DetermineSpotting( );
            IncrementSpotting( m_hex );

            CWndArea* pAreaWnd = theAreaList.GetTop( );
            if ( pAreaWnd != NULL )
                pAreaWnd->InvalidateSound( );
        }

        // get it going
        if ( !( GetData( )->GetBldgFlags( ) & CStructureData::FlOperAmb1 ) )
            if ( IsLive( ) )
                GetAmbient( CSpriteView::ANIM_FRONT_1 )->Enable( TRUE );

        if ( !( GetData( )->GetBldgFlags( ) & CStructureData::FlOperAmb2 ) )
            if ( IsLive( ) )
                GetAmbient( CSpriteView::ANIM_FRONT_2 )->Enable( TRUE );

        EventOff( );

        // update oppo fire
        OppoAndOthers( );
        return;
    }

    // set the sprite drawing %
    DetermineConstPer( );

    // get the new percentage
    if ( m_iConstDone >= GetData( )->GetTimeBuild( ) + m_iFoundTime )
        m_iConstDone = GetData( )->GetTimeBuild( ) + m_iFoundTime;
    int iPer = ( m_iConstDone * 100 ) / ( GetData( )->GetTimeBuild( ) + m_iFoundTime );
    ASSERT_STRICT( ( 0 <= iPer ) && ( iPer <= 100 ) );

    if ( ( iPer == m_iLastPer ) && ( m_iLastPer < 100 ) )
        return;
    int iOldPer = m_iLastPer;
    m_iLastPer  = iPer;

    // tell the world
    if ( !theGame.AreMessagesPaused() )
    {
        CMsgBldgStat msg( this );
        msg.m_iFlags = CMsgBldgStat::built;
        theGame.PostToAllClients( &msg, sizeof( msg ), FALSE );
    }

    MaterialChange( );

    // we don't take materials till we get the foundation done
    if ( m_iConstDone < m_iFoundTime )
        return;

    // we only do materials if it's local
    if ( GetOwner( )->IsLocal( ) )
    {
        // ok, take materials ONLY if all
        // are available. Otherwise we suspend and list the status
        // note: if this changes, same for MaterialChange

        // first we check
        for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
            if ( GetStore( iInd ) < NeedToBuild( iInd, iPer ) )
            {
                m_iLastPer = iOldPer;
                // BUGBUG				m_iConstDone = ((GetData()->GetTimeBuild () + m_iFoundTime) * m_iLastPer) / 100;
                SetFlag( event );
                DetermineConstPer( );
                theGame.Event( EVENT_CONST_HALTED, EVENT_WARN, this );
                MaterialChange( );

                // tell the router/AI
                if ( GetOwner( )->IsMe( ) )
                    theGame.m_pHpRtr->MsgOutMat( this );
                else
                    MaterialMessage( );
                return;
            }

        // use the materials
        for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
        {
            AddToStore( iInd, -NeedToBuild( iInd, iPer ) );
            GetOwner( )->IncMaterialHave( iInd, -NeedToBuild( iInd, iPer ) );
        }
    }

    m_iLastMaterialTake = iPer;

    MaterialChange( );
    ASSERT_STRICT_VALID( this );
}

void CBuilding::DetermineConstPer( )
{

    // set the sprite drawing %
    if ( ( m_iFoundPer != -1 ) && ( m_iConstDone < m_iFoundTime ) )
        m_iFoundPer = ( m_iConstDone * 100 ) / m_iFoundTime;
    else

    {
        if ( m_iFoundPer != -1 )
        {
            if ( GetOwner( )->IsMe( ) )
            {
                CWndArea* pAreaWnd = theAreaList.GetTop( );
                if ( pAreaWnd != NULL )
                {
                    CRect rect;
                    pAreaWnd->GetClientRect( &rect );
                    CPoint ptBldg = pAreaWnd->GetAA( ).WrapWorldToWindow( CMapLoc3D( GetWorldPixels( ) ) );
                    if ( rect.PtInRect( ptBldg ) )
                        pAreaWnd->InvalidateSound( );
                }
            }
            SetInvalidated( );
            m_iFoundPer = -1;
        }

        int iRem  = m_iConstDone - m_iFoundTime;
        int iPart = GetData( )->GetTimeBuild( ) / 2;
        if ( ( m_iSkltnPer != -1 ) && ( iRem < iPart ) )
            m_iSkltnPer = ( iRem * 100 ) / iPart;
        else

        {
            if ( m_iSkltnPer != -1 )
            {
                if ( GetOwner( )->IsMe( ) )
                {
                    CWndArea* pAreaWnd = theAreaList.GetTop( );
                    if ( pAreaWnd != NULL )
                    {
                        CRect rect;
                        pAreaWnd->GetClientRect( &rect );
                        CPoint ptBldg = pAreaWnd->GetAA( ).WrapWorldToWindow( CMapLoc3D( GetWorldPixels( ) ) );
                        if ( rect.PtInRect( ptBldg ) )
                            pAreaWnd->InvalidateSound( );
                    }
                }
                SetInvalidated( );
                m_iSkltnPer = -1;
            }
            m_iFinalPer = ( ( iRem - iPart ) * 100 ) / iPart;
            m_iFinalPer = __min( m_iFinalPer, 99 );
        }
    }

    // show the differences if we can see it
    if ( ( GetOwner( )->IsMe( ) ) || ( theMap.GetHex( m_hex )->GetVisible( ) ) )
        SetConstPer( );
}

void CVehicleBuilding::BuildVehicle( )
{

    ASSERT( GetOwner( )->IsLocal( ) );
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT( ( m_unitFlags & ( stop | event ) ) == 0 );
    ASSERT_STRICT( m_iConstDone == -1 );

    if ( m_pBldUnt == NULL )
    {
        // add in its power & people usuage
        GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) / 2 );
        GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) / 2 );
        return;
    }
    ASSERT_STRICT_VALID( m_pBldUnt );

    // add in its power & people usuage
    GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) );
    GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) );

    int iOldTime = m_iBuildDone;
    // get production based on everything
    int iInc = GetProd( GetOwner( )->GetManfProd( ) );

    // The Draft edict: infantry (walk units) build faster. Mult is 1.0 when the edict is off.
    if ( theTransports.GetData( m_pBldUnt->GetVehType( ) )->GetWheelType( ) == CWheelTypes::walk )
        iInc = (int)( iInc * GetOwner( )->GetEdictInfBuildMult( ) + 0.5f );

    if ( iInc < 1 )
        return;
    m_iBuildDone += iInc;
    if ( m_iBuildDone >= m_pBldUnt->GetTime( ) )
        m_iBuildDone = m_pBldUnt->GetTime( );

    // we use materials as we go
    // note: also in MaterialChange
    for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
    {
        int iAmount = ( m_pBldUnt->GetInput( iInd ) * m_iBuildDone ) / m_pBldUnt->GetTime( );
        if ( iAmount > m_aiUsed[iInd] )
        {
            if ( GetStore( iInd ) < iAmount - m_aiUsed[iInd] )
            {
#if EN_AI_PROBES_ECON && defined(_WIN32)
                if ( m_dwNextPplLog <= timeGetTime( ) )
                {
                    m_dwNextPplLog = timeGetTime( ) + 30000;
                    char szS[128];
                    sprintf( szS, "[VEHSTALL] plyr %d bldg %lu veh %d at %d%% needs mat %d store %d\n",
                             GetOwner( )->GetPlyrNum( ), (unsigned long)GetID( ), m_pBldUnt->GetVehType( ),
                             ( m_iBuildDone * 100 ) / m_pBldUnt->GetTime( ), iInd, (int)GetStore( iInd ) );
                    OutputDebugStringA( szS );
                }
#endif
                m_iBuildDone = iOldTime;
                m_fOperMod   = 0;
                SetFlag( event );
                MaterialChange( );
                AnimateOperating( FALSE );

                // let the AI/router know we are out
                if ( GetOwner( )->IsMe( ) )
                    theGame.m_pHpRtr->MsgOutMat( this );
                else
                    MaterialMessage( );
                theGame.Event( EVENT_BUILD_HALTED, EVENT_WARN, this );

                return;
            }

            AddToStore( iInd, -( iAmount - m_aiUsed[iInd] ) );
            m_aiUsed[iInd] = iAmount;
            GetOwner( )->IncMaterialHave( iInd, -( iAmount - m_aiUsed[iInd] ) );
        }
    }

    // see if done
    if ( m_iBuildDone >= m_pBldUnt->GetTime( ) )
    {
        // must have enough people
        CTransportData const* pData = theTransports.GetData( m_pBldUnt->GetVehType( ) );
        ASSERT_VALID( pData );
        if ( pData->GetPeople( ) >= GetOwner( )->GetPplBldg( ) )
        {
#if EN_AI_PROBES_ECON && defined(_WIN32)
            // observation: finished vehicle can't spawn for lack of CREW -- the
            // suspected 100%-wedge (throttled: one line per building per ~30s)
            if ( m_dwNextPplLog <= timeGetTime( ) )
            {
                m_dwNextPplLog = timeGetTime( ) + 30000;
                char szW[112];
                sprintf( szW, "[VEHPPL] plyr %d bldg %lu veh %d done100 needppl %d haveppl %d\n",
                         GetOwner( )->GetPlyrNum( ), (unsigned long)GetID( ), m_pBldUnt->GetVehType( ),
                         pData->GetPeople( ), GetOwner( )->GetPplBldg( ) );
                OutputDebugStringA( szW );
            }
#endif
            return;
        }

        // AI cheat - we give it materials for more units
        if ( ( GetOwner( )->IsAI( ) ) && ( GetOwner( )->IsLocal( ) ) )
            for ( int iOn = 0; iOn < CMaterialTypes::GetNumBuildTypes( ); iOn++ )
                AddToStore( iOn, ( m_pBldUnt->GetInput( iOn ) ) / ( 5 - theGame.m_iAi ) );

        m_iBuildDone = -1;
        m_iLastPer   = 100;
        m_fOperMod   = 0;

        theGame.Event( EVENT_BUILD_DONE, EVENT_NOTIFY, this );

        // lets find a hex to dump it out at
        CHexCoord hex = GetExitDest( pData, TRUE );

        CMsgPlaceVeh msg( this, hex, m_pOwner->GetPlyrNum( ), m_pBldUnt->GetVehType( ) );
        theGame.PostToServer( &msg, sizeof( msg ) );

        m_iNum--;
        m_iBuildDone = 0;
        m_iLastPer   = 0;
        memset( m_aiUsed, 0, sizeof( m_aiUsed ) );

        if ( m_iNum <= 0 )
        {
            AnimateOperating( FALSE );
            m_pBldUnt = NULL;
        }

        // update the status
        MaterialChange( );
        theAreaList.MaterialChange( this );
        // CDlgBuildTransport excluded from build (Phase 2d) — SDL2BuildTransport
        // re-reads status from the building on each open.
        return;
    }

    // update the %
    int iPer = ( m_iBuildDone * 100 ) / m_pBldUnt->GetTime( );
    if ( iPer == m_iLastPer )
        return;
    m_iLastPer = iPer;

    // update the status
    MaterialChange( );

    // CDlgBuildTransport excluded from build (Phase 2d) — SDL2BuildTransport
    // re-reads progress from the building on each open.
}

void CVehicleBuilding::CancelUnit( )
{

    ASSERT_STRICT_VALID( this );

    // kill it
    m_pBldUnt = NULL;
    MaterialChange( );

    // update area list buttons
    theAreaList.MaterialChange( this );
}

void CRepairBuilding::BuildRepair( )
{

    if ( !GetOwner( )->IsLocal( ) )
        return;

    ASSERT_VALID( this );
    ASSERT( ( m_unitFlags & ( stopped | event ) ) == 0 );
    ASSERT( m_iConstDone == -1 );

    if ( ( m_pVehRepairing == NULL ) || ( m_pBldUnt == NULL ) )
    {
        // add in its power & people usuage
        GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) / 2 );
        GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) / 2 );
        return;
    }
    ASSERT_VALID( m_pVehRepairing );

    // add in its power & people usuage
    GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) );
    GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) );

    int iOldLevel = m_pVehRepairing->GetDamagePoints( );
    // get change based on everything
    int iInc = GetProd( GetOwner( )->GetManfProd( ) * (float)m_pVehRepairing->GetData( )->GetDamagePoints( ) /
                        (float)m_pBldUnt->GetTime( ) );
    if ( iInc < 1 )
        return;

    // new damage level
    if ( iInc + m_pVehRepairing->GetDamagePoints( ) > m_pVehRepairing->GetData( )->GetDamagePoints( ) )
        iInc = m_pVehRepairing->GetData( )->GetDamagePoints( ) - m_pVehRepairing->GetDamagePoints( );
    int iNewLevel = iInc + m_pVehRepairing->GetDamagePoints( );

    // we use materials as we go
    // note: also in MaterialChange
    for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
    {
        int iTotal = m_pBldUnt->GetInput( iInd );
        if ( iTotal > 0 )
        {
            int iLastAmount = ( iTotal * iOldLevel ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iNewAmount  = ( iTotal * iNewLevel ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iDiff       = iNewAmount - iLastAmount;
            if ( iDiff > GetStore( iInd ) )
            {
                m_fOperMod = 0;
                SetFlag( event );
                MaterialChange( );
                AnimateOperating( FALSE );

                // tell the AI
                if ( GetOwner( )->IsMe( ) )
                    theGame.m_pHpRtr->MsgOutMat( this );
                else
                    MaterialMessage( );
                return;
            }
        }
    }

    for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
    {
        int iTotal = m_pBldUnt->GetInput( iInd );
        if ( iTotal > 0 )
        {
            int iLastAmount = ( iTotal * iOldLevel ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iNewAmount  = ( iTotal * iNewLevel ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iDiff       = iNewAmount - iLastAmount;
            if ( iDiff > 0 )
            {
                AddToStore( iInd, -iDiff );
                GetOwner( )->IncMaterialHave( iInd, -iDiff );
            }
        }
    }

    // see if done

    CMsgUnitRepair msg( m_pVehRepairing, iInc );
    theGame.PostToServer( &msg, sizeof( msg ) );
    if ( iInc + m_pVehRepairing->GetDamagePoints( ) >= m_pVehRepairing->GetData( )->GetDamagePoints( ) )
    {
        // tell the AI
        if ( ( GetOwner( )->IsLocal( ) ) && ( GetOwner( )->IsAI( ) ) )
        {
            CMsgRepaired msg( m_pVehRepairing );
            theGame.PostToClient( GetOwner( ), &msg, sizeof( msg ) );
        }

        // scenario 6 test
        if ( theGame.GetScenario( ) == 6 )
        {
            for ( int iInd = 0; iInd < 5; iInd++ )
                if ( theGame.m_adwScenarioUnits[iInd] == m_pVehRepairing->GetID( ) )
                {
                    theGame.m_iScenarioVar++;
                    break;
                }
        }

        // lets find a hex to push it out at
        // will also add next in list to be repaired
        m_pVehRepairing->ExitBuilding( );
        return;
    }

    // update the %
    int iPer = ( m_pVehRepairing->GetDamagePoints( ) * 100 ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
    if ( iPer == m_iLastPer )
        return;
    m_iLastPer = iPer;

    // update the status
    MaterialChange( );
}

void CShipyardBuilding::BuildShipyard( )
{

    // figure out the mode we should be in (hack)
    // they started construction
    if ( ( m_iMode == repair ) && ( m_iBuildDone != -1 ) )
    {
        m_iMode = build;
        m_lstNext.AddHead( m_pVehRepairing );
        m_pVehRepairing = NULL;
    }

    // we are building
    if ( m_iBuildDone != -1 )
        m_iMode = build;

    // if we were building and have nothing to build - we are done
    if ( ( m_iMode == build ) && ( m_iBuildDone == -1 ) )
        m_iMode = nothing;

    // if we were repairing and have nothing to repair - we are done
    if ( ( m_iMode == repair ) && ( m_pVehRepairing == NULL ) && ( m_lstNext.GetCount( ) <= 0 ) )
        m_iMode = nothing;

    // if we are on nothing and have a list to repair - repair it
    if ( ( m_iMode == nothing ) && ( m_lstNext.GetCount( ) > 0 ) )
    {
        m_pVehRepairing = NULL;
        while ( ( m_lstNext.GetCount( ) > 0 ) && ( m_pVehRepairing == NULL ) )
        {
            m_pVehRepairing = m_lstNext.RemoveHead( );
            // is it still here?
            if ( ( ( m_pVehRepairing->GetRouteMode( ) != CVehicle::stop ) &&
                   ( m_pVehRepairing->GetRouteMode( ) != CVehicle::repair_self ) ) ||
                 ( theBuildingHex._GetBuilding( m_pVehRepairing->GetPtHead( ) ) != this ) )
            {
                TRAP( );
                m_pVehRepairing = NULL;
            }
            else
            {
                ASSERT_VALID( m_pVehRepairing );
                AssignBldUnit( m_pVehRepairing->GetData( )->GetType( ) );
                if ( m_pBldUnt == NULL )
                    m_pVehRepairing = NULL;
                else
                    m_iLastPer =
                        ( m_pVehRepairing->GetDamagePoints( ) * 100 ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            }
        }

        // we've got one, repair it
        if ( m_pVehRepairing != NULL )
        {
            m_iMode = repair;
            MaterialChange( );
            theAreaList.MaterialChange( this );
        }
    }

    if ( m_iMode != repair )
    {
        CVehicleBuilding::BuildVehicle( );
        return;
    }

    if ( !GetOwner( )->IsLocal( ) )
        return;

    ASSERT_VALID( this );
    ASSERT( ( m_unitFlags & ( stopped | event ) ) == 0 );
    ASSERT( m_iConstDone == -1 );

    if ( m_pVehRepairing == NULL )
    {
        // add in its power & people usuage
        GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) / 2 );
        GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) / 2 );
        return;
    }
    ASSERT_VALID( m_pVehRepairing );

    // add in its power & people usuage
    GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) );
    GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) );

    int iOldLevel = m_pVehRepairing->GetDamagePoints( );
    // get change based on everything
    int iInc = GetProd( GetOwner( )->GetManfProd( ) * 2.0 * (float)m_pVehRepairing->GetData( )->GetDamagePoints( ) /
                        (float)m_pBldUnt->GetTime( ) );
    if ( iInc < 1 )
        return;

    if ( iInc + m_pVehRepairing->GetDamagePoints( ) > m_pVehRepairing->GetData( )->GetDamagePoints( ) )
        iInc = m_pVehRepairing->GetData( )->GetDamagePoints( ) - m_pVehRepairing->GetDamagePoints( );
    int iNewLevel = iInc + m_pVehRepairing->GetDamagePoints( );

    // we use materials as we go
    // note: also in MaterialChange
    for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
    {
        int iTotal = m_pBldUnt->GetInput( iInd ) / 2;
        if ( iTotal > 0 )
        {
            int iLastAmount = ( iTotal * iOldLevel ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iNewAmount  = ( iTotal * iNewLevel ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iDiff       = iNewAmount - iLastAmount;
            if ( iDiff > GetStore( iInd ) )
            {
                m_fOperMod = 0;
                SetFlag( event );
                MaterialChange( );
                AnimateOperating( FALSE );

                // tell the AI/router
                theGame.Event( EVENT_BUILD_HALTED, EVENT_WARN, this );
                if ( GetOwner( )->IsMe( ) )
                    theGame.m_pHpRtr->MsgOutMat( this );
                else
                    MaterialMessage( );
                return;
            }
        }
    }

    for ( int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes( ); iInd++ )
    {
        int iTotal = m_pBldUnt->GetInput( iInd ) / 2;
        if ( iTotal > 0 )
        {
            int iLastAmount = ( iTotal * iOldLevel ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iNewAmount  = ( iTotal * iNewLevel ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
            int iDiff       = iNewAmount - iLastAmount;
            AddToStore( iInd, -iDiff );
            GetOwner( )->IncMaterialHave( iInd, -iDiff );
        }
    }

    // see if done
    CMsgUnitRepair msg( m_pVehRepairing, iInc );
    theGame.PostToServer( &msg, sizeof( msg ) );
    if ( iInc + m_pVehRepairing->GetDamagePoints( ) >= m_pVehRepairing->GetData( )->GetDamagePoints( ) )
    {
        // lets find a hex to push it out at
        // will also start on next one
        m_pVehRepairing->ExitBuilding( );
        return;
    }

    // update the %
    int iPer = ( m_pVehRepairing->GetDamagePoints( ) * 100 ) / m_pVehRepairing->GetData( )->GetDamagePoints( );
    if ( iPer == m_iLastPer )
        return;
    m_iLastPer = iPer;

    // update the status
    MaterialChange( );
}

void CBuilding::BuildMaterials( )
{

    ASSERT_STRICT( GetData( )->GetUnionType( ) == CStructureData::UTmaterials );
    ASSERT_STRICT( ( m_unitFlags & ( stop | event ) ) == 0 );
    ASSERT_STRICT( m_iConstDone == -1 );

    // add in its power & people usuage
    GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) );
    GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) );

    // BioFuel mode (the INTENDED design): when this refinery's alt-output toggle is ON and the
    // BioFuel tech is researched, it STOPS producing gas and instead burns the player's GLOBAL
    // food into oil (the conversion runs below via AltOutput::Convert, eGlobalConsume). So while
    // the toggle is active we suppress the gas output -- the refinery makes oil, not gas. Mirrors
    // the coal plant's Coal-Liquefaction stop-power/make-oil mode. (No-op for non-refineries /
    // toggle OFF / un-researched: bBioFuel stays false and gas production is byte-identical.)
    const bool bBioFuel = IsFlag( CUnit::alt_oil ) && ( AltOutput::Available( this ) != nullptr );

    // BioFuel runs continuously off global food, so keep the refinery animating the whole time the
    // toggle is on. Do this BEFORE the batch-complete early-returns below (which otherwise leave the
    // operating animation in whatever state it had when toggled on). Idempotent + mirrors fracking.
    if ( bBioFuel )
        AnimateOperating( TRUE );

    // get change based on everything
    int iInc = GetProd( GetOwner( )->GetMtrlsProd( ) );
    if ( iInc < 1 )
        return;

    m_iBuildDone += iInc;

    CBuildMaterials const* pBm = GetData( )->GetBldMaterials( );

    // if not done, leave
    if ( m_iBuildDone < pBm->GetTime( ) )
        return;

    int iNum = m_iBuildDone / pBm->GetTime( );
    m_iBuildDone -= pBm->GetTime( ) * iNum;

    // BioFuel mode-switch: a refinery in BioFuel mode does NOT consume its normal inputs (oil)
    // and is NOT gated by them -- it burns the player's GLOBAL food into oil instead. So when
    // bBioFuel, skip the input-availability clamp and the input-consumption pass below, and run
    // the food->oil conversion off the raw per-batch rate. (Normal materials buildings are
    // byte-identical: bBioFuel is false for every non-refinery and for a refinery toggled OFF.)
    if ( bBioFuel )
    {
        // Convert player food -> oil at the def's ratio (eGlobalConsume), scaled by this batch.
        // m_fAltAccum carries the sub-unit remainder. No gas, no oil-input consumption.
        AltOutput::Convert( this, iNum, m_fAltAccum );

        // update the % and leave -- the gas-production path below is intentionally skipped.
        MaterialChange( );
        return;
    }

    // materials to build it?
    BOOL bOut = FALSE;
    for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ )
        if ( GetStore( iInd ) < pBm->GetInput( iInd ) * iNum )
        {
            bOut = TRUE;
            iNum = GetStore( iInd ) / pBm->GetInput( iInd );
        }

    // if we can build some/all, change materials
    if ( iNum )
    {
        BOOL bAsked = FALSE;
        for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ )
        {
            // if less than a minute left - ask for more
            int i1Min, iBefore;
            if ( !bAsked )
            {
                i1Min   = GetNextMinuteMat( iInd );
                iBefore = GetStore( iInd );
            }

            AddToStore( iInd, -pBm->GetInput( iInd ) * iNum );
            GetOwner( )->IncMaterialHave( iInd, -pBm->GetInput( iInd ) * iNum );

            if ( !bAsked )
            {
                int iAfter = GetStore( iInd );
                if ( ( iBefore >= i1Min ) && ( iAfter < i1Min ) )
                    if ( GetOwner( )->IsMe( ) )
                    {
                        theGame.m_pHpRtr->MsgOutMat( this );
                        bAsked = TRUE;
                    }
            }
        }

        for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ )
            if ( iInd == CMaterialTypes::gas )
            {
                // the AI burns gas a LOT faster so we help it
                if ( GetOwner( )->IsAI( ) )
                    iNum += iNum * theGame.m_iAi;
                GetOwner( )->AddGas( pBm->GetOutput( iInd ) * iNum );
            }
            else
            {
                int iAmt = pBm->GetOutput( iInd ) * iNum;
                AddToStore( iInd, iAmt );
                GetOwner( )->IncMaterialMade( iInd, iAmt );
                GetOwner( )->IncMaterialHave( iInd, iAmt );
            }
    }

    // if we ran out - stop us
    if ( bOut )
    {
        m_iBuildDone = 0;
        m_fOperMod   = 0;
        SetFlag( event );
        m_iLastPer = 0;
        MaterialChange( );
        AnimateOperating( FALSE );

        // tell the user
        if ( GetOwner( )->IsMe( ) )
            for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ )
                if ( pBm->GetOutput( iInd ) > 0 )
                    if ( GetStore( iInd ) <= 0 )
                    {
                        theGame.Event( EVENT_MANUF_HALTED, EVENT_WARN, this );
                        break;
                    }

        // tell the AI/router
        if ( GetOwner( )->IsMe( ) )
            theGame.m_pHpRtr->MsgOutMat( this );
        else
            MaterialMessage( );
    }

    // update the %
    MaterialChange( );
}

void CPowerBuilding::BuildPower( )
{

    ASSERT_STRICT_VALID( this );
    GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) );

    CBuildPower* pBp = GetData( )->GetBldPower( );

    float fPower = GetFrameProd( 1 );

    // AltOutput mode-switch, split into its TWO independent halves. Conflating them is what
    // the single old bCoalLiq flag did, and it is why moving a def would have changed shipped
    // behaviour two phases early:
    //   bAltStopsPower -- POWER half. DERIVED (a store-consuming conversion def on a UTpower
    //                     host), so it is TRUE for today's Coal Liquefaction exactly as
    //                     bCoalLiq was: the plant feeds no power to the grid and instead DRAWS
    //                     2 power to run the conversion.
    //   bAltTimeDriven -- PRODUCTION half. The def's own explicit m_eDrive. FALSE for every def
    //                     shipped today, so the fuel-burn path below is byte-identical.
    const AltOutput::AltOutputDef* pAlt = IsFlag( CUnit::alt_oil ) ? AltOutput::Available( this ) : nullptr;
    const bool bAltStopsPower = pAlt && AltOutput::StopsPower( this, pAlt );
    const bool bAltTimeDriven = pAlt && ( pAlt->m_eDrive == AltOutput::EDrive::eTimeDriven );

    if ( pBp->GetInput( ) < 0 )
    {
        // No-fuel plant (solar/rocket): can't liquefy coal it doesn't burn, so it always
        // generates power normally regardless of the toggle.
        GetOwner( )->AddPwrHave( (int)( (float)pBp->GetPower( ) * fPower ) );
        return;
    }

    if ( bAltTimeDriven )
    {
        // ---- TIME-DRIVEN conversion (a kiln / liquefaction plant) --------------------------
        // No fuel is burned at all. The conversion runs off the plant's own production
        // accumulator and consumes the DEF's input material; GetRate( ) is reused as the
        // CONVERSION period, so the existing per-type tuning knob still applies. Unreachable
        // until a def sets m_eDrive = eTimeDriven.
        const int iIn    = pAlt->m_iInputMat;
        const int iRatio = AltOutput::InputRatio( this, pAlt );
        if ( ( iRatio <= 0 ) || ( pBp->GetRate( ) <= 0 ) )
        {
            AnimateOperating( FALSE );
            return;
        }

        // Can't fund ONE whole batch -> not operating. Same rule as IsOperating( ) and
        // EffInputBatch( ). Note what is deliberately NOT here: the fuel path's low-fuel
        // discard branch below. On a fuel plant "burned the last of it" is correct; applied to
        // a kiln's lumber it would DELETE trucked input without converting it. Sub-batch input
        // is simply left in the store.
        if ( GetStore( iIn ) < iRatio )
        {
            AnimateOperating( FALSE );
            return;
        }

        AnimateOperating( TRUE );

        // The POWER half stays on the DERIVED flag -- unchanged from shipped Coal-Liq behaviour.
        if ( !bAltStopsPower )
            GetOwner( )->AddPwrHave( (int)( (float)pBp->GetPower( ) * fPower ) );
        else
            GetOwner( )->AddPwrNeed( 2 );

        int iInc = GetProd( 1 );
        if ( iInc <= 0 )
            return;
        m_iBuildDone += iInc;

        if ( m_iBuildDone < pBp->GetRate( ) )
            return;

        int iNum = m_iBuildDone / pBp->GetRate( );      // conversion batches elapsed this tick

        // Batch TIME is spent whether or not the store could fund it. That is the deliberate
        // half of the accumulator decision: unfunded time is DISCARDED, so a starved plant
        // banks no debt and there is no burst the moment a truck arrives, and m_iBuildDone
        // stays bounded below Rate exactly as it does on the fuel path.
        //
        // The other half is caller-side too, and it is why AltOutput::Convert( ) is NOT
        // modified (it is shared with the fuel-driven Coal-Liq and Charcoal callers, which must
        // stay byte-identical): Convert's eRatioConsume branch credits fAccum BEFORE it clamps
        // to what the store can afford, so a starved caller banks unfunded OUTPUT forever and
        // dumps it in one burst later. This caller can never trigger that, because it only ever
        // passes iDo * iRatio -- an exact whole-batch input quantity the store already holds --
        // so fWant is an integer, fAccum returns to exactly 0 after every call, and nothing is
        // ever banked.
        m_iBuildDone -= iNum * pBp->GetRate( );

        int iAfford = GetStore( iIn ) / iRatio;         // whole batches the store can fund
        int iDo     = __min( iNum, iAfford );

        // The ONE consumer of the TRUE consumption rate. Everything router-facing keeps the
        // router's own units instead (see CPowerBuilding::GetNextMinuteMat).
        int i1Min   = EffInputPerMin( );
        int iBefore = GetStore( iIn );

        // Convert( ) takes the INPUT quantity, not the batch count.
        if ( iDo > 0 )
            AltOutput::Convert( this, iDo * iRatio, m_fAltAccum );

        int iAfter = GetStore( iIn );
        if ( iAfter < iRatio )
        {
            // Ran dry this tick. Ask the router HERE, once: from the next tick the top gate
            // returns before any notification path, so this is the last chance to ask.
            AnimateOperating( FALSE );
            theGame.Event( EVENT_MANUF_HALTED, EVENT_WARN, this );
            if ( GetOwner( )->IsMe( ) )
                theGame.m_pHpRtr->MsgOutMat( this );
            else
                MaterialMessage( );
        }
        else if ( ( iBefore >= i1Min ) && ( iAfter < i1Min ) )
        {
            // Crossed the 1-minute mark this tick -- ask for more. Sampled AFTER Convert( ) for
            // the same reason the fuel path does: Convert drains the store with no router
            // notification of its own.
            if ( GetOwner( )->IsMe( ) )
                theGame.m_pHpRtr->MsgOutMat( this );
        }

        // update the %
        MaterialChange( );
        return;
    }

    // ---- FUEL-DRIVEN (every def shipped today; unchanged apart from bCoalLiq's rename) -----

    // if we have nothing to burn there is nothing to do -- and it isn't operating, so stop the
    // animation (operator: a coal-liq plant with no coal shouldn't animate).
    if ( GetStore( pBp->GetInput( ) ) <= 0 )
    {
        AnimateOperating( FALSE );
        return;
    }

    AnimateOperating( TRUE );   // has fuel -> operating (generating power, or liquefying coal)

    // add in our power if we have any input materials left -- UNLESS we're in coal-liq mode,
    // where the burned coal becomes oil instead of power AND the plant DRAWS 2 power to run the
    // conversion (operator).
    if ( !bAltStopsPower )
        GetOwner( )->AddPwrHave( (int)( (float)pBp->GetPower( ) * fPower ) );
    else
        GetOwner( )->AddPwrNeed( 2 );

    int iInc = GetProd( 1 );
    if ( iInc <= 0 )
        return;
    m_iBuildDone += iInc;

    if ( m_iBuildDone < pBp->GetRate( ) )
        return;

    int iNum = m_iBuildDone / pBp->GetRate( );
    if ( GetStore( pBp->GetInput( ) ) <= iNum )
    {
        GetOwner( )->IncMaterialHave( pBp->GetInput( ), -GetStore( pBp->GetInput( ) ) );
        SetStore( pBp->GetInput( ), 0 );
        m_iBuildDone = 0;
        m_fOperMod   = 0;
        // if we are out of materials then we don't add our power capacity
        AnimateOperating( FALSE );

        // tell the AI/router
        theGame.Event( EVENT_MANUF_HALTED, EVENT_WARN, this );
        if ( GetOwner( )->IsMe( ) )
            theGame.m_pHpRtr->MsgOutMat( this );
        else
            MaterialMessage( );
        return;
    }

    // if less than a minute left - ask for more
    int i1Min   = GetNextMinuteMat( pBp->GetInput( ) );
    int iBefore = GetStore( pBp->GetInput( ) );

    GetOwner( )->IncMaterialHave( pBp->GetInput( ), -iNum );
    AddToStore( pBp->GetInput( ), -iNum );
    m_iBuildDone -= iNum * pBp->GetRate( );

    // Coal Liquefaction (reusable AltOutput system): when this is a coal power plant whose
    // alt-output toggle is ON and the tech is researched, convert additional coal from the
    // plant's store into oil at 2:1 (eRatioConsume), scaled by the fuel burned this batch.
    // No-op for non-coal plants / toggle OFF / un-researched. (Charcoal & Fracking would
    // hook their own production loops the same way -- one shared helper.)
    // MUST run BEFORE the ask-for-more threshold check below: Convert drains up to 2x more
    // coal from the store with no router notification of its own, so sampling iAfter before
    // it missed the 1-minute crossing entirely - the router was never told, no truck was
    // ever dispatched, and once the store hit 0 the empty-plant early-return above never
    // asks either. Liquefying plants starved forever beside idle trucks (operator-reported).
    AltOutput::Convert( this, iNum, m_fAltAccum );

    int iAfter = GetStore( pBp->GetInput( ) );
    if ( ( iBefore >= i1Min ) && ( iAfter < i1Min ) )
        if ( GetOwner( )->IsMe( ) )
            theGame.m_pHpRtr->MsgOutMat( this );

    // update the %
    MaterialChange( );
}

void CMineBuilding::BuildMine( )
{

    ASSERT_STRICT( GetData( )->GetUnionType( ) == CStructureData::UTmine );
    ASSERT_STRICT( ( m_unitFlags & ( stop | event ) ) == 0 );
    ASSERT_STRICT( m_iConstDone == -1 );

    // nothing left to mine
    if ( m_iMinerals <= 0 )
    {
        m_iBuildDone = 0;
        SetFlag( stopped );
        m_iLastPer = 0;
        AnimateOperating( FALSE );
        theGame.Event( EVENT_MINE_EMPTY, EVENT_WARN, this );
        return;
    }

    // add in its power & people usuage. Precision Mining edict: flat +1 power & +1 worker per
    // producing mine (a % surcharge rounds away on a mine's tiny base, e.g. 1-power/2-worker).
    int iMinePwr = GetData( )->GetPower( );
    int iMinePpl = GetData( )->GetPeople( );
    if ( GetOwner( )->IsEdictActive( EDICT_PRECISION_MINING ) )
    {
        iMinePwr += 1;
        iMinePpl += 1;
    }
    GetOwner( )->AddPwrNeed( iMinePwr );
    GetOwner( )->AddPplNeedBldg( iMinePpl );

    // get change based on everything
    int iInc = GetProd( GetOwner( )->GetMineProd( ) );

    if ( iInc <= 0 )
        return;

    m_iBuildDone += iInc;

    CBuildMine* pBm = GetData( )->GetBldMine( );
    if ( m_iBuildDone < pBm->GetTimeToMine( ) )
        return;

    int iNum = m_iBuildDone / pBm->GetTimeToMine( );
    m_iBuildDone -= pBm->GetTimeToMine( ) * iNum;

    float fInc   = (float)( iNum * pBm->GetAmount( ) * m_iDensity ) / (float)CMinerals::DensityDiv( ) + m_fAmountMod;
    iNum         = (int)fInc;
    m_fAmountMod = fInc - (int)fInc;

    iNum = __min( iNum, m_iMinerals );
    m_iMinerals -= iNum;
    AddToStore( pBm->GetTypeMines( ), iNum );
    GetOwner( )->IncMaterialMade( pBm->GetTypeMines( ), iNum );
    GetOwner( )->IncMaterialHave( pBm->GetTypeMines( ), iNum );

    // update the ground every 64th time
    if ( ( m_iMinerals & 0x7F ) == 0 )
    {
        UpdateGround( );

        if ( m_iMinerals <= 0 )
        {
            m_iBuildDone = 0;
            SetFlag( stopped );
            SetFlag( abandoned );
            m_iLastPer = 0;
            AnimateOperating( FALSE );
            theGame.Event( EVENT_MINE_EMPTY, EVENT_WARN, this );
        }
    }

    // update the %
    MaterialChange( );
}

// Fracking (#23): production tick for an EXHAUSTED oil well whose alt-output toggle is ON
// and whose owner has researched Fracking. Called from the per-building update's
// stopped/abandoned branch (the well is normally idle there). Draws the well's full power
// plus +50% (the energy cost of fracking) and trickles a flat per-tier oil rate via the
// reusable AltOutput system (eFlatTrickle). Caller has already verified alt_oil + the tech
// gate via AltOutput::Available, so a non-oil-well / un-teched / toggle-OFF well never
// reaches here and idles exactly as before.
void CMineBuilding::FrackTick( )
{
    ASSERT_STRICT( GetData( )->GetUnionType( ) == CStructureData::UTmine );

    // Running an exhausted mine HOT. Moho Mining (iron mine) draws a flat 16 power -- a big,
    // deliberate drain (a power plant only makes 120). Fracking (oil well) draws 2*(1.5x + 1).
    // (A normal stopped building draws only half power.) Plus the building's people.
    if ( GetData( )->GetType( ) == CStructureData::iron )
        GetOwner( )->AddPwrNeed( 16 );                                                  // Moho: flat 16
    else
        GetOwner( )->AddPwrNeed( ( ( ( GetData( )->GetPower( ) * 3 ) / 2 ) + 1 ) * 2 );  // Fracking: 2*(1.5x + 1)
    GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) );

    // Credit the flat oil trickle. eFlatTrickle scales the per-minute rate by the opers
    // elapsed this call; the building's m_fAltAccum carries the sub-unit remainder.
    AltOutput::Convert( this, (int)theGame.GetOpersElapsed( ), m_fAltAccum );
}

void CFarmBuilding::BuildFarm( )
{

    ASSERT_STRICT( GetData( )->GetUnionType( ) == CStructureData::UTfarm );
    ASSERT_STRICT( ( m_unitFlags & ( stop | event ) ) == 0 );
    ASSERT_STRICT( m_iConstDone == -1 );

    // Harvest pacing: FARM_HARVEST_SLOW makes each harvest take that many times
    // LONGER and yield that many times MORE → identical net food/material rate, but the
    // harvest (and the crop-growth "season" tied to it) cycles slower with chunkier
    // payouts. 2 = harvest half as often, double the yield.
    const int FARM_HARVEST_SLOW = 2;

    // "Fields grown around farms": paint the crop plots on the first operational
    // tick (also rebuilds them after a load), then animate their growth. GrowFields
    // / UpdateFieldStage no-op for lumber mills (food farms only). Pass the SAME slowed
    // period we harvest on so the crop spans exactly one (slowed) harvest cycle.
    if ( m_fieldHexes.empty( ) )
        GrowFields( );
    UpdateFieldStage( FARM_HARVEST_SLOW * GetData( )->GetBldFarm( )->GetTimeToFarm( ) );

    // add in its power & people usuage
    GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) );
    // Agricultural Subsidy edict bumps only the farm's own worker requirement (default ×1.0).
    GetOwner( )->AddPplNeedBldg(
        (int)( GetData( )->GetPeople( ) * GetOwner( )->GetEdictFarmWorkerMult( ) + 0.5f ) );

    CBuildFarm* pBf = GetData( )->GetBldFarm( );

    // BUGBUG - pull this from the adjoining hexes!!!
    float fMul = GetOwner( )->GetFarmProd( ) * m_iTerMult;

    // Slash and Burn: this mill cuts at 250% while its toggle is on. Per-BUILDING, no CPlayer
    // state and no edict multiplier chain. Food farms are untouched BY CONSTRUCTION --
    // SlashBurnActive( ) requires a lumber mill -- so the multiplier can never leak onto food.
    if ( SlashBurnActive( ) )
        fMul *= AltOutput::SLASH_BURN_MULT;

    // get the productivity of this farm and add it to our total
    if ( pBf->GetTypeFarm( ) == CMaterialTypes::food )
        GetOwner( )->AddFoodProd(
            GetFrameProdNoPeople( fMul * float( 24 * 60 * pBf->GetQuantity( ) ) / float( pBf->GetTimeToFarm( ) ) ) );

    // Slash and Burn: destroy forest on TIME, on every tick this mill actually produces -- not
    // only on harvest ticks. The gate is the per-FRAME production rate, which is the exact
    // expression of "does this mill produce at all": GetFrameProdNoPeople is
    // m_fDamPerfMult * fMul * powerFactor with NO accumulator and NO truncation, so it is > 0
    // exactly when all three factors are. The three real self-terminating gates are therefore
    // preserved exactly:
    //   - a stopped / abandoned / event-wedged mill never reaches BuildFarm at all (Operate),
    //   - a mill wrecked to zero damage-performance has m_fDamPerfMult == 0,
    //   - a mill that has cut its own box down to fertility 0 has fMul == 0,
    //     so it stops slashing exactly when it stops yielding.
    // Do NOT gate on GetProdNoPeople( fMul ) instead. That returns the TRUNCATED per-tick
    // increment, which is 0 on many ticks for a weak mill (it carries the remainder in
    // m_fOperMod), so those ticks would contribute no accrual and SLASH_HEXES_PER_MINUTE would
    // silently run at roughly half rate at fertility 1 -- the dial would not mean what it says.
    // (Note this does NOT exclude an unpowered mill: the lumber mill has a non-zero
    // CStructureData::GetNoPower, so it keeps producing at a reduced rate without power and
    // keeps slashing. That is existing behaviour, not a choice made here.)
    if ( SlashBurnActive( ) && ( GetFrameProdNoPeople( fMul ) > 0.0f ) )
        SlashTick( );

    // get change based on everything
    // farms are special - no people degradation
    // (Must still run exactly once per tick, and after the slash gate above: it advances
    // m_fOperMod.)
    int iInc = GetProdNoPeople( fMul );
    if ( iInc <= 0 )
        return;

    m_iBuildDone += iInc;

    if ( m_iBuildDone < FARM_HARVEST_SLOW * pBf->GetTimeToFarm( ) )
        return;

    div_t dtRate = div( (int)m_iBuildDone, FARM_HARVEST_SLOW * pBf->GetTimeToFarm( ) );
    m_iBuildDone = dtRate.rem;
    dtRate.quot *= FARM_HARVEST_SLOW * pBf->GetQuantity( );

    // we farmed some stuff - store it
    if ( pBf->GetTypeFarm( ) == CMaterialTypes::food )
    {
        GetOwner( )->AddFood( dtRate.quot );

        // BioFuel was REMOVED from farms: the INTENDED design hosts Bio Oil on the REFINERY
        // (it stops gas and burns global food into oil -- see CBuilding::BuildMaterials and
        // the AltOutput table). A food farm no longer shows or runs the toggle.
    }
    else
    {
        // The lumber mill has no alt-output mode any more: Charcoal moved to the COAL POWER
        // PLANT, where the kiln is fed TRUCKED lumber instead of a slice of this mill's own
        // harvest (see the Charcoal def in altoutput.cpp). So a mill always credits its full
        // harvest, exactly as it did before Charcoal was ever hosted here.
        AddToStore( pBf->GetTypeFarm( ), dtRate.quot );
        GetOwner( )->IncMaterialMade( pBf->GetTypeFarm( ), dtRate.quot );
        GetOwner( )->IncMaterialHave( pBf->GetTypeFarm( ), dtRate.quot );
    }

    // update the %
    MaterialChange( );
}
