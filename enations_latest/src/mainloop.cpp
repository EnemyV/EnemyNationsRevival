//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "ai.h"
#include "area.h"
#include "building.inl"
#include "chproute.hpp"
#include "cpathmgr.h"
#include "cutscene.h"
#include "event.h"
#include "lastplnt.h"
#include "minerals.inl"
#include "player.h"
#include "plyrlist.h"
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


#ifdef _PROFILE
extern "C"
{
    void WINAPI MarkStop( );
    void WINAPI MarkStart( );
}
#endif


#define WM_KICKIDLE 0x036A  // from afxpriv.h

// for ::Run
BOOL AFXAPI AfxOleGetUserCtrl( );

#ifdef _CHEAT
extern BOOL _bShowRate;
#endif


// MM timer data & code

static UINT uRenderTimer = 0;
HANDLE      hRenderEvent = 0;
DWORD       dwFrameCheck = 2 * 1000 / FRAME_RATE;


/////////////////////////////////////////////////////////////////////////////
// CConquerApp::Run - main running routine until application exits

int CConquerApp::Run( )
{

    ASSERT_VALID( this );

    if ( m_pMainWnd == NULL && AfxOleGetUserCtrl( ) )
    {
        // Not launched /Embedding or /Automation, but has no main window!
        TRACE0( "Warning: m_pMainWnd is NULL in CWinApp::Run - quitting application.\n" );
        AfxPostQuitMessage( 0 );
    }

    // we take the critical section and only let it go when we aren't
    // processing our own data. Not the cleanest method in the world
    // but it works
    try
    {

        // acquire and dispatch messages until a WM_QUIT message is received.
        for ( ;; )
        {
            // process all pending messages
            if ( BaseYield( ) )

                // it's a QUIT
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

            // run a frame
            GraphicsEnginePump( );
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
            if ( AfxMessageBox( IDS_SAVE_ON_ERROR, MB_YESNO | MB_ICONQUESTION ) == IDYES )
            {
                TRAP( );
                try
                {
                    theGame.SaveGame( NULL );
                }
                catch ( ... )
                {
                    AfxMessageBox( IDS_SAVE_ON_ERROR_FAILED, MB_OK | MB_ICONSTOP );
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
            if ( AfxMessageBox( IDS_SAVE_ON_ERROR, MB_YESNO | MB_ICONQUESTION ) == IDYES )
            {
                try
                {
                    theGame.SaveGame( NULL );
                }
                catch ( ... )
                {
                    AfxMessageBox( IDS_SAVE_ON_ERROR_FAILED, MB_OK | MB_ICONSTOP );
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
        bDoSubclass = FALSE;

        if ( theGame.GetState( ) == CGame::play )
        {
            theGame.SetState( CGame::error );
            myThreadClose( (THREADEXITFUNC)AiExit );
            if ( AfxMessageBox( IDS_SAVE_ON_ERROR, MB_YESNO | MB_ICONQUESTION ) == IDYES )
            {
                try
                {
                    theGame.SaveGame( NULL );
                }
                catch ( ... )
                {
                    AfxMessageBox( IDS_SAVE_ON_ERROR_FAILED, MB_OK | MB_ICONSTOP );
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
BOOL CConquerApp::BaseYield( )
{

    ASSERT_STRICT_VALID( this );

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
                        CWndPrimary* pTmp;
                        if ( thePrimaryMap.Lookup( pWndPrimary, pTmp ) )
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

void CConquerApp::ProcessAllMessages( )
{

    if ( !theGame.ShouldProcessMessages() )
        return;

    // process all messages so we have none pending
    while ( TRUE )
    {
        EnterCriticalSection( &cs );
        if (theGame.m_messagePointerList.GetCount( ) <= 0 )
        {
            LeaveCriticalSection( &cs );
            break;
        }
        char* pBuf = (char*)theGame.m_messagePointerList.RemoveHead( );

#ifdef LOGGINGON
        CString str;
        str.Format( "Processing type %d\n", ( (CNetCmd*)pBuf )->GetType( ) );
        OutputDebugStringA( str );
      //  OutputDebugStringA( pBuf );
        OutputDebugStringA( "\n" );
#endif

        if ( pBuf == NULL )
        {
            LeaveCriticalSection( &cs );
            break;
        }
        theGame.ProcessMessage((CNetCmd *) pBuf);
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

    // animate the screen
    POSITION pos = theAnimList.GetHeadPosition( );
    while ( pos != NULL )
    {
        CWndAnim* pWnd = theAnimList.GetNext( pos );
        pWnd->ReRender( );
    }
    pos = theAnimList.GetHeadPosition( );
    while ( pos != NULL )
    {
        CWndAnim* pWnd = theAnimList.GetNext( pos );
        pWnd->Draw( );
    }

    CHexCoord::ClearInvalidated( );  // Set terrain invalidated flags to FALSE

// show the frame rate
#ifdef _CHEAT
    {
        static DWORD dwLastTime = 0;

        if ( _bShowRate )
            if ( !theAnimList.IsEmpty( ) )
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
                CString sText;
                sprintf( sText.GetBuffer( 80 ), "_  %d.%d fps", dtRate.quot, dtRate.rem );
                sText.ReleaseBuffer( -1 );
                    sText += "  Msgs PAUSED";
                if ( theGame.AreMessagesPaused( ) )
                    sText += "  Msgs PAUSED";
                if ( theGame.ShouldPause( ) )
                    sText += "  Should PAUSED";
                theApp.m_wndBar.SetDebugText( 0, sText );
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

// runs a frame of our graphics engine
void CConquerApp::GraphicsEnginePump( )
{

#ifdef _PROFILE
    DWORD dwMarkStart = timeGetTime( );
    MarkStart( );
#endif

    // process messages
    ProcessAllMessages( );

    theGame._SettimeGetTime( );

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

            ::Sleep( __minmax( 10, 1000 / FRAME_RATE, dwSleep ) );
        }
        return;
    }

    // give network & AI a chance
    if ( theGame.HaveAI( ) )
    {
        // if 1/12 of a second or better - give the AI half of it
        int iExtra =
            ( (int)( 2 * 1000 / FRAME_RATE ) - (int)( theGame.GettimeGetTime( ) - theGame.m_dwOperTimeLast ) ) / 2;
        ::Sleep( __minmax( 10, 2 * 1000 / FRAME_RATE, iExtra ) );
    }
    else
        ::Sleep( 10 );  // give network some time

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
                    if ( m_pdlgPlyrList != NULL )
                        m_pdlgPlyrList->ShowWindow( SW_HIDE );
                    AfxMessageBox( IDS_GAME_OVER, MB_OK | MB_ICONSTOP );
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
                        if ( AfxMessageBox( IDS_TIME_OUT, MB_YESNO | MB_ICONQUESTION ) == IDYES )
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
                        if ( m_pdlgRelations != NULL )
                        {
                            m_pdlgRelations->DestroyWindow( );
                            m_pdlgRelations = NULL;
                        }
                        if ( m_pdlgRsrch != NULL )
                        {
                            m_pdlgRsrch->DestroyWindow( );
                            m_pdlgRsrch = NULL;
                        }
                        return;
                    }

                    theCutScene.PlayEnd( CWndCutScene::lose );
                    theApp.CloseWorld( );
                    return;
                }

                if ( theGame.GetAll( ).GetCount( ) <= 1 )
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
                            CString sMsg, sName;
                            if ( AfxMessageBox( IDS_TIME_OUT, MB_YESNO | MB_ICONQUESTION ) == IDYES )
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

#ifdef _CHEAT
            if ( pDlgStats != NULL )
                pDlgStats->Update( );
#endif

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
                        m_wndBar.m_wndStat[CWndBar::gas].IncIcon( );
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
                        m_wndBar.m_wndStat[CWndBar::power].IncIcon( );
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
                        m_wndBar.m_wndStat[CWndBar::people].IncIcon( );
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
                        m_wndBar.m_wndStat[CWndBar::food].IncIcon( );
                    m_wndBar.UpdateFood( );
                }
            }  // HaveHP

            if ( CheckYield( ) )
                if ( !theGame.ShouldOperate() )
                    goto NoOper;
        }  // 1 second code

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
        pos = theBuildingMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD      dwID;
            CBuilding* pBldg;
            theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
            ASSERT_STRICT_VALID( pBldg );
            pBldg->Operate( );
        }

        // operate the vehicles
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

        // operate the projectiles
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

        // process messages from Operate calls
        ProcessAllMessages( );
    }  // if operate

NoOper:

    // where we should render (if the above was fast)
    RenderScreens( );

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
        if ( ( m_iConstDone != -1 ) || ( m_unitFlags & stopped ) )
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
        // if stopped we only need half the people & power
        if ( ( m_iConstDone == -1 ) && ( !( m_unitFlags & abandoned ) ) )
        {
            GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) / 2 );
            GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) / 2 );
        }
        m_fOperMod = 0;
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
        CMsgUnitRepair msg( this, dtFix.quot );
        theGame.PostToServer( &msg, sizeof( msg ) );
    }
RepairDone:;

    // if waiting on an event, return
    if ( m_unitFlags & event )
    {
        // if stopped we only need half the people & power
        if ( m_iConstDone == -1 )
        {
            GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) / 2 );
            GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) / 2 );
        }
        m_fOperMod = 0;
        return;
    }

    // are we still building?
    if ( m_iConstDone != -1 )
    {
        Construct( );
        return;
    }

    // first we handle combat stuff
    HandleCombat( );

    // special case - rockets generate power
    if ( GetData( )->GetType( ) == CStructureData::rocket )
        GetOwner( )->AddPwrHave( (int)( 15.0 * GetFrameProd( 1 ) ) );

    // ok its build - now it has to operate
    switch ( GetData( )->GetUnionType( ) )
    {
    case CStructureData::UTmaterials:
        BuildMaterials( );
        break;
    case CStructureData::UTvehicle:
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
        float           fTmp = pBr->GetRate( ) * m_fDamPerfMult * GetOwner( )->GetPplMult( );
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
            theApp.m_wndBar._GotoScience( );

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
            return;

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
        CDlgBuildTransport* pDlg = QueryDlgBuild( );
        if ( pDlg != NULL )
            pDlg->UpdateStatus( m_iNum > 0 ? 1 : 0 );
        return;
    }

    // update the %
    int iPer = ( m_iBuildDone * 100 ) / m_pBldUnt->GetTime( );
    if ( iPer == m_iLastPer )
        return;
    m_iLastPer = iPer;

    // update the status
    MaterialChange( );

    // update the dialog
    CDlgBuildTransport* pDlg = QueryDlgBuild( );
    if ( pDlg != NULL )
        pDlg->UpdateStatus( iPer );
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

    if ( pBp->GetInput( ) < 0 )
    {
        GetOwner( )->AddPwrHave( (int)( (float)pBp->GetPower( ) * fPower ) );
        return;
    }

    // if we have nothing to burn there is nothing to do
    if ( GetStore( pBp->GetInput( ) ) <= 0 )
        return;

    // add in our power if we have any input materials left
    GetOwner( )->AddPwrHave( (int)( (float)pBp->GetPower( ) * fPower ) );

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

    // add in its power & people usuage
    GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) );
    GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) );

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

void CFarmBuilding::BuildFarm( )
{

    ASSERT_STRICT( GetData( )->GetUnionType( ) == CStructureData::UTfarm );
    ASSERT_STRICT( ( m_unitFlags & ( stop | event ) ) == 0 );
    ASSERT_STRICT( m_iConstDone == -1 );

    // add in its power & people usuage
    GetOwner( )->AddPwrNeed( GetData( )->GetPower( ) );
    GetOwner( )->AddPplNeedBldg( GetData( )->GetPeople( ) );

    // BUGBUG - pull this from the adjoining hexes!!!
    float fMul = GetOwner( )->GetFarmProd( ) * m_iTerMult;

    CBuildFarm* pBf = GetData( )->GetBldFarm( );
    // get the productivity of this farm and add it to our total
    if ( pBf->GetTypeFarm( ) == CMaterialTypes::food )
        GetOwner( )->AddFoodProd(
            GetFrameProdNoPeople( fMul * float( 24 * 60 * pBf->GetQuantity( ) ) / float( pBf->GetTimeToFarm( ) ) ) );

    // get change based on everything
    // farms are special - no people degradation
    int iInc = GetProdNoPeople( fMul );
    if ( iInc <= 0 )
        return;

    m_iBuildDone += iInc;

    if ( m_iBuildDone < pBf->GetTimeToFarm( ) )
        return;

    div_t dtRate = div( (int)m_iBuildDone, pBf->GetTimeToFarm( ) );
    m_iBuildDone = dtRate.rem;
    dtRate.quot *= pBf->GetQuantity( );

    // we farmed some stuff - store it
    if ( pBf->GetTypeFarm( ) == CMaterialTypes::food )
        GetOwner( )->AddFood( dtRate.quot );
    else
    {
        AddToStore( pBf->GetTypeFarm( ), dtRate.quot );
        GetOwner( )->IncMaterialMade( pBf->GetTypeFarm( ), dtRate.quot );
        GetOwner( )->IncMaterialHave( pBf->GetTypeFarm( ), dtRate.quot );
    }

    // update the %
    MaterialChange( );
}
