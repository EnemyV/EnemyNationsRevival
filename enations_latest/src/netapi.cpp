//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// netapi.cpp : implementation file
//

#include "netapi.h"

#include "SDL2GameDialogs.h"
#include "enprobes.h"
#include "ai.h"
#include "area.h"
#include "bridge.h"
#include "building.inl"
#include "chproute.hpp"
#include "creatmul.inl"
// CDlgMsg calls replaced by EnMessageBoxOnce (Phase 2a).
// Keep this include for CDlgModelessMsg which is still used.
#include "DlgMsg.h"
#include "event.h"
#include "help.h"
#include "join.h"
#include "lastplnt.h"
#include "netcmd.h"
#include "player.h"
#include "relation.h"
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


// this is the only instance of this
CNetApi theNet;

static void BldgNew( CMsgBldgNew* pMsg );

// Trace m_vpHdl's lifecycle to find WHERE it goes NULL between OpenClient and
// Join on the iserve/TCP-discovered path (linux2 @518e7da2: vpJoinSession sees
// hdl=(nil), and the fatal-latch was ruled out — FatalError never fired, so
// HandleNetDown is NOT the cause). getenv-gated fprintf (Log() is silent on a
// POSIX Release build); default off => zero ship impact.
static int JoinAddrLogOn() {
    static int on = -1;
    if ( on < 0 ) on = ( getenv( "EN_JOINADDR" ) || getenv( "EN_NETTRACE" ) ) ? 1 : 0;
    return on;
}


/////////////////////////////////////////////////////////////////////////////
// CNetApi
//   note - an error returns true

CNetApi::CNetApi( )
{

    m_vpHdl     = NULL;
    m_vpSession = NULL;
    m_hWnd      = NULL;
    m_iMode     = closed;
    m_iType     = closed;
    m_cFlags    = 0;
}

CNetApi::~CNetApi( )
{

    Close( FALSE );
}

std::string CNetApi::GetIServeAddress( )
{

    VPNETADDRESS addr;
    if ( !vpGetServerAddress( m_vpHdl, &addr ) )
        return ( "" );

    char sBuf[258];
    vpGetAddressString( m_vpHdl, &addr, sBuf, 256 );
    return ( sBuf );
}

BOOL CNetApi::OpenServer( int iProtocol, HWND hWnd, char const* pName, void const* pData, void const* pPrtcl )
{

    if ( iProtocol <= 2 )
    {
        int iNumProt = 0;
        for ( int iRad = 0; iRad < 3; iRad++ )
            if ( CNetApi::SupportsProtocol( aPr[iRad] ) )
                iNumProt++;
        if ( iNumProt > 1 )
        {
            EnMessageBoxOnce( IDS_ERROR_MULT_PROT_WARNING, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
                        "MultProtWarning" );
        }
    }

    if ( m_vpHdl != NULL )
        Close( FALSE );

    if ( ( m_vpHdl = vpStartup( VPAPI_VERSION, &tlpGUID, iMaxNameLen, iMaxNameLen, iProtocol, pPrtcl ) ) == NULL )
    {
        Close( FALSE );
        EnMessageBox( IDS_VPSTARTUP_FAILED, MB_OK | MB_ICONSTOP );
        return ( TRUE );
    }

    // create a session
    m_hWnd = hWnd;
    if ( ( m_vpSession = vpCreateSession( m_vpHdl, hWnd, pName, 0, pData ) ) == NULL )
    {
        Close( FALSE );
        EnMessageBox( IDS_VPCREATE_FAILED, MB_OK | MB_ICONSTOP );
        return ( TRUE );
    }

    m_iMode = opened;
    m_iType = server;
    return ( FALSE );
}

BOOL CNetApi::OpenClient( int iProtocol, HWND hWnd, void const* pPrtcl )
{

    if ( iProtocol <= 2 )
    {
        int iNumProt = 0;
        for ( int iRad = 0; iRad < 3; iRad++ )
            if ( CNetApi::SupportsProtocol( aPr[iRad] ) )
                iNumProt++;
        if ( iNumProt > 1 )
        {
            EnMessageBoxOnce( IDS_ERROR_MULT_PROT_WARNING, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
                        "MultProtWarning" );
        }
    }

    if ( m_vpHdl != NULL )
        Close( FALSE );

    if ( ( m_vpHdl = vpStartup( VPAPI_VERSION, &tlpGUID, iMaxNameLen, iMaxNameLen, iProtocol, pPrtcl ) ) == NULL )
    {
        Close( FALSE );
        EnMessageBox( IDS_VPSTARTUP_FAILED, MB_OK | MB_ICONSTOP );
        return ( TRUE );
    }

    if ( JoinAddrLogOn() )
        fprintf( stderr, "[join-addr] OpenClient: m_vpHdl SET = %p\n", (void*)m_vpHdl );

    // create a session
    m_hWnd = hWnd;
    if ( !vpEnumSessions( m_vpHdl, hWnd, TRUE, NULL ) )
    {
        Close( FALSE );
        EnMessageBox( IDS_VPENUM_FAILED, MB_OK | MB_ICONSTOP );
        return ( TRUE );
    }

    m_iMode = opened;
    m_iType = client;
    return ( FALSE );
}

std::string CNetApi::GetAddress( ) const
{

    VPNETADDRESS addr;
    vpGetAddress( m_vpHdl, &addr );

    char sBuf[258];
    vpGetAddressString( m_vpHdl, &addr, sBuf, 258 );
    return std::string( sBuf );
}

BOOL CNetApi::Join( LPCVPSESSIONID id, CNetJoin const* pJn )
{

    ASSERT( m_iType == client );

    if ( JoinAddrLogOn() )
        fprintf( stderr, "[join-addr] CNetApi::Join: m_vpHdl=%p m_iType=%d (if hdl=nil here, something nulled it between OpenClient and now)\n",
                 (void*)m_vpHdl, m_iType );

    m_iMode = joined;
    if ( ( m_vpSession = vpJoinSession( m_vpHdl, m_hWnd, id, (LPCSTR)pJn, 0, (LPVOID)TRUE ) ) == NULL )
    {
        TRAP( );
        EnMessageBox( IDS_VPJOIN_FAILED, MB_OK | MB_ICONSTOP );
        return ( TRUE );
    }
    return ( FALSE );
}

VPPLAYERID CNetApi::AddPlayer( CNetJoin const* pJn )
{

    ASSERT( m_iType == server );

    VPPLAYERID rtn;
    if ( !vpAddPlayer( m_vpSession, (LPCSTR)pJn, 0, (LPVOID)TRUE, &rtn ) )
    {
        TRAP( );
        EnMessageBox( IDS_VPJOIN_FAILED, MB_OK | MB_ICONSTOP );
        return ( 0 );
    }

    return ( rtn );
}

void CNetApi::DeletePlayer( VPPLAYERID idTo )
{

    vpKillPlayer( m_vpSession, idTo );
}

void CNetApi::CloseSession( BOOL bDelayClose )
{

    if ( m_vpSession != NULL )
    {
        if ( m_iType == client )
            StopEnum( );
        else
            SetSessionVisibility( FALSE );
    }

    m_iMode = closed;
    m_iType = closed;

    if ( m_vpSession == NULL )
        return;

    // yield to blast in messages
    theApp.BaseYield( );

    // close the session
    if ( bDelayClose )
        m_cFlags |= closeSession;
    else
    {
        vpCloseSession( theNet.m_vpSession, NULL );
        theNet.m_vpSession = NULL;
        m_cFlags &= ~closeSession;
    }

    // yield to blast in messages (and force a GP fault now if we blew it)
    theApp.BaseYield( );
}

void CNetApi::SessionClose( )
{

    m_vpSession = NULL;
    m_iMode     = closed;
    m_iType     = closed;
}

void CNetApi::Close( BOOL bDelayClose )
{

    if ( JoinAddrLogOn() )
        fprintf( stderr, "[join-addr] CNetApi::Close(delay=%d) ENTER: m_vpHdl=%p (will vpCleanup->NULL it unless delayed) — who called Close between OpenClient and Join?\n",
                 (int)bDelayClose, (void*)m_vpHdl );

    m_iMode = closed;
    m_iType = closed;

    if ( m_vpHdl == NULL )
        return;

    // yield to blast in messages
    theApp.BaseYield( );

    // close the session
    CloseSession( bDelayClose );

    if ( bDelayClose )
        m_cFlags |= cleanup;
    else
    {
        vpCleanup( theNet.m_vpHdl );
        theNet.m_vpHdl = NULL;
        theNet.m_hWnd  = NULL;
        m_cFlags &= ~cleanup;
    }

    // yield to blast in messages (and force a GP fault now if we blew it)
    theApp.BaseYield( );
}

BOOL CNetApi::Send( VPPLAYERID idTo, LPCVPMSGHDR pData, int iLen )
{

    ASSERT( iLen <= VP_MAXSENDDATA );
    ASSERT( idTo != 0 );
    ASSERT( theGame.GetMyNetNum( ) != 0 );

    if ( vpSendData( m_vpSession, idTo, theGame.GetMyNetNum( ), pData, iLen, VP_MUSTDELIVER, NULL ) )
        return ( FALSE );

    // ok, if idTo isn't any existing player we had a message in the queue for a killed
    // player
    if ( theGame._GetPlayer( idTo ) == NULL )
        return ( FALSE );

    EnMessageBox( IDS_VPSEND_FAILED, MB_OK | MB_ICONSTOP );
    SaveExistingGame( );
    theApp.CloseWorld( );
    ThrowError( ERR_TLP_QUIT );
    return ( TRUE );
}

BOOL CNetApi::Broadcast( LPCVPMSGHDR pData, int iLen, BOOL bLocal )
{

    // if we are single player get out of here
    if ( !theGame.IsNetGame( ) )
    {
        ASSERT( theGame.AmServer( ) );
        return ( FALSE );
    }

    ASSERT( iLen <= VP_MAXSENDDATA );

    if ( vpSendData( m_vpSession, VP_ALLPLAYERS, theGame.GetMyNetNum( ), pData, iLen, VP_MUSTDELIVER | VP_BROADCAST,
                     bLocal ? NULL : (LPVOID)TRUE ) )
        return ( FALSE );

    EnMessageBox( IDS_VPSEND_FAILED, MB_OK | MB_ICONSTOP );
    SaveExistingGame( );
    theApp.CloseWorld( );
    ThrowError( ERR_TLP_QUIT );
    return ( TRUE );
}

// [mp-plyr] diagnostics: player-identity / leave / AI-takeover tracing for the MP
// client auto-place bug (client's rocket placed without the player choosing; the
// leading suspect chain is spurious-leave -> replace -> AiTakeOverPlayer -> AI
// PlaceRocket -> bldg_new(rocket) matching the client's plyrnum). stderr for the
// POSIX clients + OutputDebugString for the Win host (dbgcatch records ODS, not
// stderr). Cheap and rare - stays on until the MP start path is stable.
// Env-gated: default OFF as the note above always intended for release — set
// EN_MPDIAG=1 to re-arm it for an MP-start hunt.
static BOOL MpDiagOn( )
{
    static int s_on = -1;
    if ( s_on < 0 ) {
        const char* e = getenv( "EN_MPDIAG" );
        s_on = ( e != NULL && *e == '1' ) ? 1 : 0;
    }
    return s_on;
}
void EnMpDiagLog( const char* fmt, ... )
{
    if ( !MpDiagOn( ) )
        return;
    char buf[512];
    va_list args;
    va_start( args, fmt );
    vsnprintf( buf, sizeof( buf ), fmt, args );
    va_end( args );
    fprintf( stderr, "[mp-plyr] %s\n", buf );
#ifdef _WIN32
    char ods[560];
    sprintf_s( ods, "[mp-plyr] %s\n", buf );
    OutputDebugStringA( ods );
#endif
}

static void OnMsgLeave( VPPLAYERID id )
{

    // will be NULL if we deleted it
    CPlayer* pPlr = theGame._GetPlayer( id );
    if ( pPlr == NULL )
        return;

    ASSERT_VALID( pPlr );
    ASSERT( ( !theGame.HaveHP( ) ) || ( id != theGame.GetMe( )->GetPlyrNum( ) ) );

    EnMpDiagLog( "OnMsgLeave: netid=%d plyr=%d name='%s' plrState=%d gameState=%d netMode=%d",
                 (int)id, pPlr->GetPlyrNum( ), pPlr->GetName( ), (int)pPlr->GetState( ),
                 (int)theGame.GetState( ), theNet.GetMode( ) );

    // if it was the server fix it
    if ( pPlr == theGame.GetServer( ) )
    {
        TRAP( );
        theGame.SetServer( NULL );
    }

    pPlr->SetNetNum( 0 );

    // take it out of the list
    if ( theApp.m_pCreateGame != NULL )
        theApp.m_pCreateGame->RemovePlayer( pPlr );

    switch ( theNet.GetMode( ) )
    {
    case CNetApi::opened:
        if ( ( theApp.m_pCreateGame != NULL ) && ( theApp.m_pCreateGame->m_iTyp == CCreateBase::load_multi ) )
        {
            std::string sMsg = strPrintf( EnLoadStdString( IDS_MSG_NET_GOODBYE ).c_str(),
                                          pPlr->GetName( ) );
            CDlgModelessMsg* pDlg = new CDlgModelessMsg( );
            pDlg->Create( sMsg.c_str() );

            // set back to AI
            pPlr->SetAI( TRUE );
            pPlr->SetLocal( TRUE );
            pPlr->SetState( CPlayer::replace );

            // tell the others
            CMsgCancelLoad msg( pPlr );
            theGame.PostToAllClients( &msg, sizeof( msg ), FALSE );
            break;
        }

        theGame.DeletePlayer( pPlr );
        if ( ( theGame.IsAllReady( ) ) && ( theGame.GetNetJoin( ) != CGame::approve ) )
        {
            try
            {
                theGame.IncTry( );
                theApp.StartCreateWorld( );
                theGame.DecTry( );
            }

            catch ( int iNum )
            {
                CatchNum( iNum );
                theApp.CloseWorld( );
                return;
            }
            catch ( SE_Exception e )
            {
                TRAP( );
                CatchSE( e );
                theApp.CloseWorld( );
                return;
            }
            catch ( ... )
            {
                TRAP( );
                CatchOther( );
                theApp.CloseWorld( );
                return;
            }
        }

        break;
    case CNetApi::joined:
        theGame.DeletePlayer( pPlr );
        break;

    default: {
        // before the AI sets the race name
        std::string sMsg = strPrintf( EnLoadStdString( IDS_MSG_NET_GOODBYE ).c_str(),
                                      pPlr->GetName( ) );

        // have the AI take over
        if ( theGame.AmServer( ) )
        {
            // if we are still installing just set it
            pPlr->SetAI( TRUE );
            pPlr->SetLocal( TRUE );
            if ( theGame.GetState( ) < CGame::AI_done )
            {
                pPlr->SetState( CPlayer::replace );
                EnMpDiagLog( "OnMsgLeave: plyr=%d name='%s' marked REPLACE during setup -> AI takes over at StartAi",
                             pPlr->GetPlyrNum( ), pPlr->GetName( ) );

                if ( theGame.GetState( ) == CGame::wait_AI )
                {
                    CNetInitDone msg( theGame.GetMe( ) );
                    theGame.PostToServer( &msg, sizeof( msg ) );
                }
            }
            else
                theGame.AiTakeOverPlayer( pPlr, TRUE );

            // tell the world
            CNetToAi msg( pPlr );
            theGame.PostToAllClients( &msg, sizeof( msg ), FALSE );

            // kill chat
            theApp.m_wndChat.KillAiChatWnd( pPlr );

            // if now single player loose the comm
            if ( theGame.GetAll( ).GetCount( ) == theGame.GetAi( ).GetCount( ) + 1 )
            {
                theApp.m_wndChat.DestroyWindow( );
                theApp.CloseDlgChat( );
            }

            // ok, if we went from no, to 1 AI player we need to make ourselves visible again
            if ( ( theGame.GetNetJoin( ) == CGame::any ) && ( theGame.GetAi( ).GetCount( ) == 1 ) )
                theNet.SetSessionVisibility( TRUE );
        }

        if ( theApp.m_wndBar.IsCreated() )
            theApp.m_wndBar.SetStatusText( 0, sMsg.c_str() );
        if ( ( theGame._GetMe( ) != NULL ) && ( pPlr != theGame.GetMe( ) ) )
            if ( !pPlr->m_bMsgDead )
            {
                pPlr->m_bMsgDead      = TRUE;
                CDlgModelessMsg* pDlg = new CDlgModelessMsg( );
                pDlg->Create( sMsg.c_str() );
            }
        break;
    }
    }
}

// the game is over for us (for everyone if we're the server)
static void OnMsgSessionClose( )
{

    extern bool g_bClientLobbyWaiting;
    extern bool g_bClientHostLost;
    // Host dropped while we're still in the client waiting room (before the game
    // started): there is no game to tear down or save. Flag it so the lobby dialog
    // shows "the host has left" and closes itself, instead of falling through to the
    // in-game teardown/save-prompt below (which assumes an active game).
    if ( g_bClientLobbyWaiting )
    {
        g_bClientHostLost = true;
        if ( theGame.GetServer( ) != NULL )
            theGame.GetServer( )->SetNetNum( 0 );
        theNet.SessionClose( );
        return;
    }

    int iMode = theNet.GetMode( );
    if ( theGame.GetServer( ) != NULL )
        theGame.GetServer( )->SetNetNum( 0 );
    theNet.SessionClose( );

    // if it's us we're done
    if ( ( theGame._GetMe( ) == NULL ) || ( theGame.GetMe( )->GetState( ) == CPlayer::dead ) ||
         ( theGame.GetState( ) == CGame::other ) )
    {
        TRAP( );
        theNet.Close( TRUE );
        return;
    }

    // if we are playing let them save
    BOOL      bTold = FALSE;
    CWndArea* pWnd  = theAreaList.GetTop( );
    if ( pWnd != NULL )
        if ( ( pWnd->GetMode( ) != CWndArea::rocket_ready ) && ( pWnd->GetMode( ) != CWndArea::rocket_pos ) &&
             ( pWnd->GetMode( ) != CWndArea::rocket_wait ) )
        {
            bTold = TRUE;
            std::string sMsg = strPrintf( EnLoadStdString( IDS_SAVE_CLOSE ).c_str(),
                                          theGame.m_sGameName.c_str() );

            if ( EnMessageBox( sMsg.c_str(), MB_YESNO | MB_ICONQUESTION ) == IDYES )
                theGame.SaveGame( (CWnd*)NULL );
        }

    // We have to tell the player (this is bad news) and
    // then we go back to the main screen
    if ( !bTold )
    {
        std::string sMsg;
        CPlayer* pPlyr = theGame.GetServer( );
        if ( pPlyr != NULL )
            sMsg = strPrintf( EnLoadStdString( IDS_JOIN_UNJOIN ).c_str(),
                              theGame.GetServer( )->GetName( ) );
        else
            sMsg = strPrintf( EnLoadStdString( IDS_JOIN_UNJOIN ).c_str(),
                              EnLoadStdString( IDS_UNKNOWN ).c_str() );
        EnMessageBox( sMsg.c_str(), MB_OK | MB_TASKMODAL );
    }

    // close it (will call CloseWorld after returning)
    theNet.Close( TRUE );
}

static void OnMsgSessionEnum( LPCVPSESSIONINFO pSi )
{
    if ( memcmp( &( pSi->gameId ), &( tlpGUID ), sizeof( VPGUID ) ) )
        return;
    if ( theApp.m_pCreateGame->m_iTyp != CCreateBase::join_net )
    {
        ASSERT( FALSE );
        return;
    }

    theApp.m_pCreateGame->OnSessionEnum( pSi );
}

static void OnMsgJoin( LPCVPPLAYERINFO pPi, BOOL bLocal, BYTE bErr )
{

    CPlayer* pPlyr;

    if ( bLocal )
    {
        if ( bErr )
        {
            TRAP( );
            std::string sNum = IntToStr( bErr );
            std::string sMsg = strPrintf( EnLoadStdString( IDS_MSG_JOIN_FAILED ).c_str(),
                                          theGame.GetServer( )->GetName( ), sNum.c_str() );
            EnMessageBox( sMsg.c_str(), MB_OK | MB_ICONSTOP );
            return;
        }

        theGame.GetMe( )->SetNetNum( pPi->playerId );
        pPlyr = theGame.GetMe( );

        // CDlgChatAll excluded from build (Phase 2d).
    }

    else
    {
        if ( bErr )
        {
            TRAP( );
            return;
        }

        if ( theGame.AmServer( ) )
            if ( theNet.GetMode( ) != CNetApi::opened )
                return;

        // if we're not the server & this is the server then it already exists
        CNetJoin* pJn = (CNetJoin*)pPi->playerName;
        if ( ( !theGame.AmServer( ) ) && ( pJn->m_bServer ) && ( theGame.GetServer( ) != NULL ) )
        {
            pPlyr = theGame.GetServer( );
            theGame.GetServer( )->SetName( pJn->m_sName );
            theGame.GetServer( )->SetNetNum( pPi->playerId );
        }
        else if ( ( theApp.m_pCreateGame->m_iTyp != CCreateBase::load_join ) &&
                  ( theApp.m_pCreateGame->m_iTyp != CCreateBase::load_multi ) )
        {
            pPlyr = new CPlayer( pJn->m_sName, pPi->playerId );
            theGame.AddPlayer( pPlyr );
        }
        else

        {
            // add this player
            pPlyr = new CPlayer( pJn->m_sName, pPi->playerId );
            pPlyr->SetAI( FALSE );
            pPlyr->SetLocal( FALSE );
            theGame.m_lstLoad.AddTail( pPlyr );
        }

        if ( pJn->m_bServer )
            theGame._SetServer( pPlyr );
    }

    // if this is the person, not the loaded player
    if ( ( theApp.m_pCreateGame->m_iTyp == CCreateBase::load_multi ) ||
         ( ( theApp.m_pCreateGame->m_iTyp == CCreateBase::load_join ) && ( pPlyr == theGame.GetMe( ) ) ) )
        pPlyr->SetState( CPlayer::load_pick );

    // add it to the player box
    theApp.m_pCreateGame->AddPlayer( pPlyr );

    // may have to ask for players now
    if ( ( bLocal ) && ( theApp.m_pCreateGame->m_iTyp == CCreateBase::load_join ) && ( !theGame.AmServer( ) ) )
    {
        CNetEnumPlyrs msg( theGame.GetMe( )->GetNetNum( ) );
        theGame.PostToServer( &msg, sizeof( msg ) );
    }
}

// start the send
static void StartFile( CMsgStartFile* pMsg )
{

    theGame.m_iNumSends++;
    CPlayer* pPlyr = theGame.GetPlayer( pMsg->m_idTo );

    pPlyr->SetState( CPlayer::load_file );
    pPlyr->m_pXferToClient = new CVPTransfer( theNet._GetSessionHandle( ) );
    pPlyr->m_pXferToClient->SendDataTo( pMsg->m_idTo, pMsg->m_idFrom, theGame.m_pGameFile, theGame.m_iGameBufLen );
}

static void OnMsgServerDown( LPCVPSESSIONINFO pSi )
{

    if ( ( theApp.m_pCreateGame == NULL ) || ( theApp.m_pCreateGame->m_iTyp != CCreateBase::join_net ) )
    {
        ::OnMsgSessionClose( );
        return;
    }

    theApp.m_pCreateGame->OnSessionClose( pSi );
}

#if defined( _DEBUG ) && defined( _WIN32 )
// SEH wrapper: AssertMsgValid on a garbage message can FAULT while validating
// through garbage fields (2026-07-01 MP test: killed the Windows HOST too — AV at
// netcmd.cpp:1023 — after the size and m_iType guards both passed, so the guards
// can't enumerate every hostile shape). Capture the bytes and drop instead of dying.
static BOOL SafeAssertMsgValid( CNetCmd const* pCmd, int iLen )
{
    __try
    {
        pCmd->AssertMsgValid( );
        return TRUE;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        char sLine[256] = { 0 };
        const unsigned char* pb = (const unsigned char*)pCmd;
        int nDump = ( iLen < 32 ) ? iLen : 32;
        int nOff = sprintf( sLine, "[net-guard] AssertMsgValid FAULTED type=%d len=%d bytes: ",
                            pCmd->GetType( ), iLen );
        for ( int i = 0; i < nDump; i++ )
            nOff += sprintf( sLine + nOff, "%02X ", pb[i] );
        sLine[nOff] = '\n';
        fprintf( stderr, "%s", sLine );
        OutputDebugStringA( sLine );   // dbgcatch records ODS, not stderr — hex must ride here
        return FALSE;
    }
}
#endif

void CGame::AddToQueue( CNetCmd const* pCmd, int iLen )
{
#ifdef LOGGINGON
    if ( iLen == 0 )
    {
        OutputDebugStringA( "0 length pMsg in AddToQueue?!\n" );
    }
    char str[128];
    snprintf( str, sizeof(str), "AddingToQueue type %d, %d, %d", pCmd->GetType( ), iLen, sizeof( pCmd ) );
    OutputDebugStringA( str );
    OutputDebugStringA( "\n" );
#endif

    ASSERT_VALID( this );

    // Sanity BEFORE any typed read of the buffer (AssertMsgValid casts to the
    // concrete msg struct — on a short/misrouted datagram that's an OOB read,
    // the SIGSEGV that killed the POSIX clients in the 2026-07-01 MP test).
    int msgType = pCmd->GetType( );
    if ( msgType < 0 || msgType >= CNetCmd::last_message )
    {
        TRACE( "ProcessMessage: Invalid message type %d (corrupted message?)\n", msgType );
        return;  // Skip corrupted messages instead of asserting
    }
    if ( !pCmd->FitsBuffer( iLen ) )
    {
        fprintf( stderr, "[net-guard] dropped %d-byte buffer decoding as msg type %d - too short, corrupt/misrouted datagram\n",
                 iLen, msgType );
        return;
    }

#ifdef _DEBUG
#ifdef _WIN32
    if ( !SafeAssertMsgValid( pCmd, iLen ) )
        return;   // hostile/garbage message — bytes captured to stderr, do not queue
#else
    pCmd->AssertMsgValid( );
#endif
#endif
    // can't do - previous messages may need to be processed first	ASSERT_CMD (pCmd);

#ifdef _LOG_LAG
    ( (CNetCmd*)pCmd )->dwPostTime = timeGetTime( );
#endif

#ifdef bugbug_TRAP
    if ( pCmd->GetType( ) == CNetCmd::build_bldg )
    {
        CMsgBuildBldg*        pMsg = (CMsgBuildBldg*)pCmd;
        CPlayer*              pPlr = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );
        const CStructureData* pSd  = theStructures.GetData( pMsg->m_iType );
        TRAP( !pSd->PlyrIsDiscovered( pPlr ) );
    }
#endif

    EnterCriticalSection( &cs );

    void* pBuf;
    TRAP( iLen > VP_MAXSENDDATA );

    if ( iLen <= MSG_POOL_SIZE )
        pBuf = m_memPoolSmall.alloc();
    else
        pBuf = m_memPoolLarge.alloc();

    
#ifdef LOGGINGON
     str.Format( "iLen=%d, sizeof(CNetCmd)=%d, type=%d, MSG_POOL_SIZE=%d, pbuf=%d", 
        iLen, sizeof( CNetCmd ), 
        pCmd->GetType( ), 
        MSG_POOL_SIZE,
        pBuf);
    OutputDebugStringA( str );
    OutputDebugStringA( "\n" );
#endif

    // Zero out the memory
    // (should be done in the memory_pool strat now)
   // if ( iLen <= MSG_POOL_SIZE )
   //     memset( pBuf, 0, MSG_POOL_SIZE );  // harcoded from memory pool sizes
   // else
   //     memset( pBuf, 0, VP_MAXSENDDATA );

    // Validate buffer before memcpy**
    ASSERT( pBuf != NULL );
    ASSERT( iLen > 0 );
    ASSERT( pCmd != NULL );

    memcpy( pBuf, pCmd, iLen );  // crashes here sometimes, pBuf is null? 
    // once happened when building a lumber mill
    // didn't happen when idling, but maybe the ai was too?

    if ( iLen <= MSG_POOL_SIZE )
        ( (CNetCmd*)pBuf )->m_bMemPool = 1;
    else
        ( (CNetCmd*)pBuf )->m_bMemPool = 0;

    m_messagePointerList.AddTail(pBuf );
    
#ifdef LOGGINGON
    char*    pBufChar  = (char*)theGame.m_messagePointerList.GetTail( );

    str;
    if ( theGame.m_messagePointerList.GetCount( ) >= 2 )
    {
        POSITION posTail   = theGame.m_messagePointerList.GetTailPosition( );
        char*    pBufChar2 = (char*)theGame.m_messagePointerList.GetAt( posTail );

        str.Format( "Verifying type %d (old is %d, origi is %d, orig cast is %d [list count %d]), previous type %d\n",
                    ( (CNetCmd*)pBufChar )->GetType( ), ( (CNetCmd*)pBuf )->GetType( ), pCmd->GetType( ),
                    ( (CNetCmd*)pCmd )->GetType( ), theGame.m_messagePointerList.GetCount( ),
                    ( (CNetCmd*)pBufChar2 )->GetType( ) );
    }
    else
    {
        str.Format( "Verifying type %d (old is %d, origi is %d, orig cast is %d [list count %d])\n",
                    ( (CNetCmd*)pBufChar )->GetType( ), ( (CNetCmd*)pBuf )->GetType( ), pCmd->GetType( ),
                    ( (CNetCmd*)pCmd )->GetType( ), theGame.m_messagePointerList.GetCount( ) );
    
    }
    OutputDebugStringA( str );
 //   OutputDebugStringA( pBufChar );
    OutputDebugStringA( "\n" );

    
#endif


    // throttle messages off if a net game
    if ( ( theGame.IsNetGame( ) ) && ( !theGame.ShouldPause() ) )
    {
        if (theGame.m_messagePointerList.GetCount( ) >= MAX_NUM_MESSAGES )
        {
            theGame.SetShouldPause();

            LeaveCriticalSection( &cs );
            CMsgPauseMsg msg( TRUE );
            if ( theGame.AmServer( ) )
                theGame.PostToAllClients( &msg, sizeof( msg ) );
            else
                theGame.PostToServer( &msg, sizeof( msg ) );

            return;  // already left critical section
        }
    }

    LeaveCriticalSection( &cs );
}

void CGame::EmptyQueue( )
{

    SetShouldProcessMessages(FALSE);

    EnterCriticalSection( &cs );

    while (theGame.m_messagePointerList.GetCount( ) > 0 )
        FreeQueueElement((CNetCmd *) theGame.m_messagePointerList.RemoveHead());

//    MemPoolShrink( m_memPoolLarge );
//    MemPoolShrink( m_memPoolSmall );

    LeaveCriticalSection( &cs );
}

void CNetApi::OnNetFlowOff( )
{

    if ( !theGame.ShouldNetPause() )
    {
        theGame.SetNetPause( );

        if ( theApp.m_pLogFile != NULL )
        {
            SYSTEMTIME st;
            char       sBuf[200];
            GetLocalTime( &st );
            sprintf( sBuf, "Net Flow Off at %d:%d", st.wMinute, st.wSecond );
            theApp.Log( sBuf );
        }

        theGame.SetMessagesPaused(TRUE);
    }
}

void CNetApi::OnNetFlowOn( )
{

    if ( theApp.m_pLogFile != NULL )
    {
        SYSTEMTIME st;
        char       sBuf[200];
        GetLocalTime( &st );
        sprintf( sBuf, "Net Flow On at %d:%d", st.wMinute, st.wSecond );
        theApp.Log( sBuf );
    }

    theGame.SetMessagesPaused(FALSE);
}

LRESULT CNetApi::OnNetMsg( WPARAM wParam, LPARAM lParam )
{

    LPVPMESSAGE pVpMsg = (LPVPMESSAGE)lParam;

    // EN_VPNQ=1 lifecycle trace (see wnotque.h): dispatch leg. A vpmsg pointer
    // appearing here AFTER its [vpnq] ack-delete line = the UAF we're hunting.
    {
        static int s_vpnq = -1;
        if ( s_vpnq < 0 ) { const char* e = getenv( "EN_VPNQ" ); s_vpnq = ( e && *e && *e != '0' ) ? 1 : 0; }
        if ( s_vpnq )
            fprintf( stderr, "[vpnq] dispatch vpmsg=%p code=%u u.data=%p\n",
                     (void*)pVpMsg, (unsigned)wParam, (void*)pVpMsg->u.data );
    }

    DWORD dwProc = timeGetTime( );

    // see if receiving a file
    if ( theGame.m_pXferFromServer != NULL )
    {
        BOOL bProcessed = theGame.m_pXferFromServer->ProcessNotification( wParam, pVpMsg );
        theGame.m_pXferFromServer->OnTimer( );

        // error?
        if ( theGame.m_pXferFromServer->GetError( ) )
        {
            delete theGame.m_pXferFromServer;
            theGame.m_pXferFromServer = NULL;
            delete[] theGame.m_pGameFile;
            theGame.m_pGameFile = NULL;
            EnMessageBox( IDS_JOIN_FILE_ERROR, MB_OK | MB_ICONSTOP );

            theGame.Close( );
            theNet.Close( TRUE );
            theApp.CreateMain( );
        }

        else
        {
            // see if done
            if ( theGame.m_pXferFromServer->Done( ) )
            {
                delete theGame.m_pXferFromServer;
                theGame.m_pXferFromServer = NULL;
                ( (CJoinMulti*)theApp.m_pCreateGame )->GameLoaded( theGame.m_pGameFile, theGame.m_iGameBufLen );
                theGame.m_pGameFile = NULL;
            }

            // update status bar
            else if ( bProcessed )
            {
                SDL2CreateStatus* pDlg = theApp.m_pCreateGame->GetDlgStatus( );
                if ( pDlg != NULL )
                    pDlg->SetPer( ( theGame.m_pXferFromServer->TransferredDataAmount( ) * 100 ) /
                                  theGame.m_iGameBufLen );
            }
        }

        if ( bProcessed )
        {
            vpAcknowledge( theNet.m_vpHdl, pVpMsg );
            return ( 0 );
        }
    }

    // if we're the server are we sending files?
    if ( theGame.m_iNumSends > 0 )
    {
        POSITION pos;
        for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
        {
            CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
            if ( pPlr->m_pXferToClient != NULL )
            {
                BOOL bProcessed = pPlr->m_pXferToClient->ProcessNotification( wParam, pVpMsg );
                pPlr->m_pXferToClient->OnTimer( );

                // error? - drop them
                if ( pPlr->m_pXferToClient->GetError( ) )
                {
                    delete pPlr->m_pXferToClient;
                    pPlr->m_pXferToClient = NULL;
                    theGame.m_iNumSends--;
                    theNet.DeletePlayer( pPlr->GetNetNum( ) );
                    pPlr->SetNetNum( 0 );
                    pPlr->SetState( CPlayer::ready );
                }

                else
                {
                    // see if done
                    if ( pPlr->m_pXferToClient->Done( ) )
                    {
                        delete pPlr->m_pXferToClient;
                        pPlr->m_pXferToClient = NULL;
                        pPlr->SetState( CPlayer::ready );
                    }
                }

                if ( bProcessed )
                {
                    vpAcknowledge( theNet.m_vpHdl, pVpMsg );
                    return ( 0 );
                }
            }
        }
    }

    switch ( wParam )
    {
    case VP_SESSIONENUM:
        ::OnMsgSessionEnum( pVpMsg->u.sessionInfo );
        break;
    case VP_PLAYERENUM:
        TRAP( );
        break;
    case VP_JOIN:
        ::OnMsgJoin( pVpMsg->u.playerInfo, (BOOL)(INT_PTR)pVpMsg->userData, VPGETERRORCODE( pVpMsg->notificationCode ) );   // BOOL smuggled through LPVOID
        break;

    case VP_LEAVE:
        ::OnMsgLeave( pVpMsg->u.playerInfo->playerId );
        break;

    case VP_READDATA: {
        if ( pVpMsg->userData == NULL )
        {
            CNetCmd* pCmd = (CNetCmd*)( ( (char*)pVpMsg->u.data ) - sizeof( VPMsgHdr ) );
            int cbTotal = (int)( pVpMsg->dataLen + sizeof( VPMsgHdr ) );
            // Guard the immediate-chat read the same way AddToQueue guards the
            // queued path: a short/misrouted datagram must not be read as a
            // full message struct (see FitsBuffer in netcmd.cpp).
            if ( !pCmd->FitsBuffer( cbTotal ) )
            {
                char sHex[3 * 24 + 1] = { 0 };
                const unsigned char* pb = (const unsigned char*)pCmd;
                int nDump = ( cbTotal < 24 ) ? cbTotal : 24;
                if ( nDump < 0 ) nDump = 0;
                for ( int i = 0; i < nDump; i++ )
                    sprintf( sHex + 3 * i, "%02X ", pb[i] );
                fprintf( stderr, "[net-guard] dropped %d-byte VP_READDATA decoding as msg type %d sender=%u - too short, corrupt/misrouted; bytes: %s\n",
                         cbTotal, pCmd->GetType( ), (unsigned)pVpMsg->senderId, sHex );
            }
            // Content guard + provenance capture for the cross-platform garbage
            // veh_new (mac2 5/5 SIGSEGV: size-plausible message, out-of-range
            // m_iType, always right after an OnSenumREP). Plain field reads within
            // the FitsBuffer-verified span are safe; validating here (instead of
            // letting AssertValid index arrays with garbage) turns the crash into
            // a hex capture that identifies the sender and the actual bytes.
            else if ( pCmd->GetType( ) == CNetCmd::veh_new &&
                      ( ( (_CMsgVeh*)pCmd )->m_iType < 0 ||
                        ( (_CMsgVeh*)pCmd )->m_iType >= theTransports.GetNumTransports( ) ) )
            {
                char sHex[3 * 24 + 1] = { 0 };
                const unsigned char* pb = (const unsigned char*)pCmd;
                int nDump = ( cbTotal < 24 ) ? cbTotal : 24;
                for ( int i = 0; i < nDump; i++ )
                    sprintf( sHex + 3 * i, "%02X ", pb[i] );
                fprintf( stderr, "[net-guard] dropped veh_new with garbage m_iType=%d (max %d) sender=%u len=%d bytes: %s\n",
                         ( (_CMsgVeh*)pCmd )->m_iType, theTransports.GetNumTransports( ),
                         (unsigned)pVpMsg->senderId, cbTotal, sHex );
            }
            // Chat is handled IMMEDIATELY (not queued) so it also works in the
            // pre-game lobby, where the message queue isn't being drained. (The
            // queued path still exists in ProcessMessage for safety.)
            else if ( pCmd->GetType( ) == CNetCmd::cmd_chat )
            {
                const CNetChat* pChat = (const CNetChat*)pCmd;
                CPlayer* pSender = theGame._GetPlayer( pChat->m_iPlyrNetNum );
                std::string from = pSender ? (const char*)pSender->GetName( ) : "?";
                SDL2Chat_AddMessage( from + ": " + pChat->m_sMsg );
            }
            else
                theGame.AddToQueue( pCmd, cbTotal );
        }
        else
            TRAP( );

#ifdef _LOG_LAG
        static int   aiMsgBin[1000];
        static BOOL  bShoot = FALSE;
        static DWORD dwLastTime;

        if ( theApp.m_pLogFile != NULL )
        {
            CNetCmd* pCmd = (CNetCmd*)( ( (char*)pVpMsg->u.data ) - sizeof( VPMsgHdr ) );
            aiMsgBin[pCmd->GetType( )] += 1;
            if ( ( !bShoot ) && ( pCmd->GetType( ) == CNetCmd::shoot_gun ) )
            {
                bShoot = TRUE;
                theApp.Log( "shooting started" );
                goto show_it;
            }
            if ( ( pCmd->GetType( ) == CNetCmd::cmd_resume ) || ( timeGetTime( ) - dwLastTime > 10 * 60 * 1000 ) )
            {
            show_it:
                dwLastTime = timeGetTime( );
                SYSTEMTIME st;
                char       sBuf[200];
                GetLocalTime( &st );
                sprintf( sBuf, "Time %d:%d", st.wMinute, st.wSecond );
                theApp.Log( sBuf );
                for ( int iInd = 0; iInd < 1000; iInd++ )
                    if ( aiMsgBin[iInd] > 0 )
                    {
                        sprintf( sBuf, "Msg: %d received %d times", iInd, aiMsgBin[iInd] );
                        theApp.Log( sBuf );
                    }
                memset( aiMsgBin, 0, sizeof( aiMsgBin ) );
            }
        }
#endif

        break;
    }

    case VP_NETDOWN:
        if ( theNet.m_vpHdl == NULL )
        {
            TRAP( );
            return ( 0 );
        }
        theNet.m_cFlags |= cleanup;
    case VP_SESSIONCLOSE:
        ::OnMsgSessionClose( );
        break;

    case VP_SERVERDOWN:
        ::OnMsgServerDown( pVpMsg->u.sessionInfo );
        break;

#ifdef _DEBUG
    case VP_SENDDATA:
        TRAP( );
        break;
    default:
        ASSERT( FALSE );
        break;
#endif
    }

#ifdef _LOG_LAG
    // log the messages
    if ( theApp.m_pLogFile != NULL )
    {
        char       sBuf[200];
        SYSTEMTIME st;
        GetLocalTime( &st );
        DWORD dwNow = timeGetTime( );
        int   iDif  = abs( ( st.wMinute * 60 + st.wSecond ) -
                           ( ( pVpMsg->creationTime >> 16 ) * 60 + ( pVpMsg->creationTime & 0xFFFF ) ) );
        if ( ( iDif > 7 ) || ( dwNow - pVpMsg->postTime > 500 ) )
        {
            sprintf( sBuf, "Sent %d:%d, post: -%d, arv: -%d, proc: %d:%d", pVpMsg->creationTime >> 16,
                     pVpMsg->creationTime & 0xFFFF, dwNow - pVpMsg->postTime, dwNow - dwProc, st.wMinute, st.wSecond );
            theApp.Log( sBuf );
        }
    }
#endif

    if ( theNet.m_vpHdl != NULL )
    {
        vpAcknowledge( theNet.m_vpHdl, pVpMsg );

        if ( theNet.m_cFlags & ( closeSession | cleanup ) )
        {
            // close the session
            if ( theNet.m_cFlags & closeSession )
            {
                vpCloseSession( theNet.m_vpSession, NULL );
                theNet.m_vpSession = NULL;
            }

            // kill the connection
            if ( theNet.m_cFlags & cleanup )
            {
                if ( JoinAddrLogOn() )
                    fprintf( stderr, "[join-addr] OnNetMsg cleanup-flag path: vpCleanup DESTROYING m_vpHdl=%p (this is the VP_NETDOWN/cleanup route that nulls the handle mid-join)\n",
                             (void*)theNet.m_vpHdl );
                vpCleanup( theNet.m_vpHdl );
                theNet.m_vpHdl = NULL;
                theNet.m_hWnd  = NULL;
            }

            // if we're playing/loading - undo it
            theNet.m_cFlags &= ~( closeSession | cleanup );
            switch ( theGame.GetState( ) )
            {
            case CGame::play:
            case CGame::save:
            case CGame::error:
            case CGame::other:
                theApp.CloseWorld( );
                break;

            case CGame::init:
            case CGame::init_AI:
            case CGame::AI_done:
                ThrowError( ERR_TLP_QUIT );
                break;

            default:
                if ( theApp.m_pCreateGame != NULL )
                {
                    delete theApp.m_pCreateGame;
                    theApp.m_pCreateGame = NULL;
                    theApp.DestroyExceptMain( );
                    theApp.CreateMain( );
                }
                break;
            }
        }
    }

    return ( 0 );
}


/////////////////////////////////////////////////////////////////////////////
// from here down its handling messages from READDATA

static void CmdReady( CNetReady* pMsg )
{

    ASSERT( theGame.AmServer( ) );
    ASSERT_CMD( pMsg );

    CPlayer* pPlr = theGame.GetPlayer( pMsg->m_iPlyrNum );
    if ( pPlr == NULL )
    {
        // [mp-plyr] the joined player's race arrives here (CNetReady.m_InitData).
        // If we can't find the player by netnum, the race is DROPPED -> the host
        // lobby shows the default (Human) race for that player. Log the miss.
        EnMpDiagLog( "CmdReady: NO PLAYER for netnum=%d -> race DROPPED (lobby will show default/Human)",
                     pMsg->m_iPlyrNum );
        ASSERT( FALSE );
        return;
    }
    ASSERT_VALID( pPlr );
    pPlr->m_InitData = pMsg->m_InitData;
    pPlr->SetState( CPlayer::ready );
    EnMpDiagLog( "CmdReady: applied race to plyr=%d name='%s' netnum=%d (race[0]=%.3f)",
                 pPlr->GetPlyrNum( ), pPlr->GetName( ), pMsg->m_iPlyrNum,
                 pMsg->m_InitData.GetRace( 0 ) );
    if ( theApp.m_pCreateGame != NULL )
        theApp.m_pCreateGame->UpdateBtns( );

    if ( ( theGame.IsAllReady( ) ) && ( theGame.GetNetJoin( ) != CGame::approve ) )
    {
        try
        {
            theGame.IncTry( );
            theApp.StartCreateWorld( );
            theGame.DecTry( );
        }

        catch ( int iNum )
        {
            TRAP( );
            CatchNum( iNum );
            theApp.CloseWorld( );
            return;
        }
        catch ( SE_Exception e )
        {
            TRAP( );
            CatchSE( e );
            theApp.CloseWorld( );
            return;
        }
        catch ( ... )
        {
            TRAP( );
            CatchOther( );
            theApp.CloseWorld( );
            return;
        }
    }
}

static void CmdEnumPlyrs( int iNetNum )
{

    // figure out who asked
    CPlayer* pPlrSend = theGame._GetPlayer( iNetNum );

    // could be dead by now
    if ( pPlrSend == NULL )
    {
        TRAP( );
        return;
    }

    ASSERT_VALID( pPlrSend );

    // send the players (can be loaded or in process)
    POSITION pos;
    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
        ASSERT_VALID( pPlr );
        if ( pPlr->GetState( ) != CPlayer::load_pick )
        {
            CNetPlyrJoin* pData = CNetPlyrJoin::Alloc( pPlr );
            theGame.PostToClient( pPlrSend, pData, pData->m_iLen );
            delete[] ( (char*)pData );
        }
    }
}

static void CmdPlyrJoin( CNetPlyrJoin* /*pMsg*/ )
{
    // CDlgPickPlayer removed (Phase 2d). The MFC load-join path that received
    // these messages is dead; the SDL2 SDL2PickPlayerDialog populates its own
    // listbox from theGame state via CNetPlyrJoin sent earlier in the handshake.
    if ( ( theApp.m_pCreateGame == NULL ) || ( theGame.AmServer( ) ) ||
         ( theApp.m_pCreateGame->m_iTyp != CCreateBase::load_join ) )
    {
        TRAP( );
        return;
    }
}

static void CmdSelectPlyr( CNetSelectPlyr* pMsg )
{

    CPlayer* pPlrWasMe = theGame.GetPlayer( pMsg->m_iNetNum );
    CPlayer* pPlr      = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );
    if ( pPlr->GetState( ) != CPlayer::ready )
    {
        theGame.LoadToPlyr( pPlrWasMe, pPlr );
        pPlr->SetState( CPlayer::ready );
        theApp.m_pCreateGame->UpdateBtns( );

        pMsg->ToOk( );
        theGame.PostToClient( pPlr, pMsg, sizeof( CNetSelectPlyr ) );
        pMsg->ToTaken( );
        theGame.PostToAllClients( pMsg, sizeof( CNetSelectPlyr ), FALSE );

        // we need to give this guy the game file
        // read in the file
        if ( theGame.m_pGameFile == NULL )
        {
            CFile fil( theGame.m_sFileName.c_str(), CFile::modeRead | CFile::shareExclusive | CFile::typeBinary );
            theGame.m_iGameBufLen = fil.GetLength( );
            theGame.m_pGameFile   = new char[theGame.m_iGameBufLen];
            fil.Read( theGame.m_pGameFile, theGame.m_iGameBufLen );
            fil.Close( );
        }

        // tell the player to ask for it
        CNetGetFile msg( pPlr, theGame.GetServer( ), theGame.m_iGameBufLen );
        theGame.PostToClient( pPlr, &msg, sizeof( msg ) );

        return;
    }

    TRAP( );
    pMsg->ToNotOk( );
    theGame.PostToClient( pMsg->m_iNetNum, pMsg, sizeof( CNetSelectPlyr ) );
}

static void CmdSelectOk( CNetSelectPlyr* )
{

    theApp.m_pCreateGame->ClosePick( );
    SDL2CreateStatus* pDlg = theApp.m_pCreateGame->GetDlgStatus( );
    if ( pDlg != NULL )
    {
        pDlg->SetPer( 0 );
        pDlg->SetMsg( IDS_JOIN_LOAD_FILE );
        theApp.m_pCreateGame->ShowDlgStatus( );
    }
}

static void CmdSelectNotOk( CNetSelectPlyr* )
{
    TRAP( );

    if ( theApp.m_pCreateGame->m_iTyp != CCreateBase::load_join )
        return;

    // CDlgPickPlayer removed (Phase 2d) — the MFC m_dlgWait
    // (CDlgPickWait) modal that gated re-entry is gone; SDL2 join
    // flow re-enables itself when its DoModal loop resumes.
}

static void CmdPlayerTaken( CNetSelectPlyr* /*pMsg*/ )
{
    // CDlgPickPlayer removed (Phase 2d). The MFC handler walked the dialog's
    // listbox to mark the taken player and refresh button state; the SDL2
    // join flow re-queries theGame state from its own DoModal loop.
    if ( theApp.m_pCreateGame == NULL ) return;
    TRAP( );
}

// sending the game file to this player
static void CmdGetFile( CNetGetFile* pCmd )
{

    // update the player numbers
    theGame.GetMe( )->SetPlyrNum( pCmd->m_iPlyrNum );
    theGame.GetServer( )->SetPlyrNum( pCmd->m_iServerNum );
    theGame.GetServer( )->SetNetNum( pCmd->m_iServerNetNum );

    theGame.m_pXferFromServer = new CVPTransfer( theNet._GetSessionHandle( ) );
    theGame.m_pGameFile       = new char[pCmd->m_iBufLen];
    theGame.m_iGameBufLen     = pCmd->m_iBufLen;

    CMsgStartFile msg( theGame.GetServerNetNum( ), theGame.GetMyNetNum( ) );
    theGame.m_pXferFromServer->ReceiveDataFrom( theGame.GetServerNetNum( ), theGame.GetMyNetNum( ), theGame.m_pGameFile,
                                                theGame.m_iGameBufLen );

    // show loading
    theApp.m_pCreateGame->ShowDlgStatus( );
    SDL2CreateStatus* pDlg = theApp.m_pCreateGame->GetDlgStatus( );
    if ( pDlg != NULL )
        pDlg->SetMsg( IDS_JOIN_LOAD_FILE );

    // tell server to send
    theGame.PostToServer( &msg, sizeof( msg ) );
}

static void CmdYouAre( int iPlyrNum, int iSrvrNum )
{

    EnMpDiagLog( "cmd_you_are: host says I am plyr=%d (server plyr=%d); my plyr was %d",
                 iPlyrNum, iSrvrNum,
                 ( theGame._GetMe( ) != NULL ) ? theGame.GetMe( )->GetPlyrNum( ) : -1 );

    ASSERT( !theGame.AmServer( ) );
    ASSERT( theGame.GetAll( ).GetCount( ) == 2 );
    ASSERT( theGame.GetMe( )->GetNetNum( ) > 0 );
    ASSERT( strlen( theGame.GetMe( )->GetName( ) ) > 0 );

    // set the dialog box
    try
    {
        theGame.IncTry( );
        theApp.m_pCreateGame->GetDlgStatus( )->SetStatus( );
        theGame.DecTry( );
    }

    catch ( int iNum )
    {
        TRAP( );
        EnMpDiagLog( "cmd_you_are: EXCEPTION (num %d) in status update - CloseWorld, my plyrnum NOT set!", iNum );
        CatchNum( iNum );
        theApp.CloseWorld( );
        return;
    }
    catch ( SE_Exception e )
    {
        TRAP( );
        EnMpDiagLog( "cmd_you_are: SEH EXCEPTION in status update - CloseWorld, my plyrnum NOT set!" );
        CatchSE( e );
        theApp.CloseWorld( );
        return;
    }
    catch ( ... )
    {
        TRAP( );
        EnMpDiagLog( "cmd_you_are: EXCEPTION in status update - CloseWorld, my plyrnum NOT set!" );
        CatchOther( );
        theApp.CloseWorld( );
        return;
    }

    theGame.GetMe( )->SetPlyrNum( iPlyrNum );
    if ( iSrvrNum != 0 )
        if ( theGame.GetServer( ) != NULL )
            theGame.GetServer( )->SetPlyrNum( iSrvrNum );

    // clean up from receiving file
    TRAP( theGame.m_pXferFromServer != NULL );
    delete theGame.m_pXferFromServer;
    theGame.m_pXferFromServer = NULL;
    delete theGame.m_pGameFile;
    theGame.m_pGameFile = NULL;
}

static void CmdPlayer( CNetPlayer* pNp )
{

#ifdef LOGGINGON
    OutputDebugStringA( "CmdPlayer\n" );
#endif

    ASSERT( !theGame.AmServer( ) );

    // if we already have this one - move it to the end
    CPlayer* pPlr = theGame._GetPlayer( pNp->m_iNetNum );
    if ( ( pPlr != NULL ) && ( pNp->m_iNetNum != 0 ) )
    {
        POSITION pos = theGame.GetAll( ).Find( pPlr );
        theGame.GetAll( ).RemoveAt( pos );
    }
    else
    {
        if ( ( theApp.m_pCreateGame != NULL ) && ( ( theApp.m_pCreateGame->m_iTyp == CCreateBase::load_multi ) ||
                                                   ( theApp.m_pCreateGame->m_iTyp == CCreateBase::load_join ) ) )
        {
            if ( ( pPlr = theGame.GetPlayerByPlyr( pNp->m_iPlyrNum ) ) == NULL )
            {
                TRAP( );
                pPlr = new CPlayer( );
            }
            else
            {
                POSITION pos = theGame.GetAll( ).Find( pPlr );
                if ( pos != NULL )
                    theGame.GetAll( ).RemoveAt( pos );
                if ( ( pos = theGame.GetAi( ).Find( pPlr ) ) != NULL )
                    theGame.GetAi( ).RemoveAt( pos );
            }
        }
        else
            pPlr = new CPlayer( );
    }

    pPlr->SetNetNum( pNp->m_iNetNum );
    pPlr->SetPlyrNum( pNp->m_iPlyrNum );
    pPlr->SetAI( pNp->m_bAI );
    pPlr->SetLocal( pNp->m_bLocal );
    pPlr->m_InitData = pNp->m_InitData;
    pPlr->SetName( pNp->m_sName );
    theGame._SetMaxPlyrNum( __max( theGame.GetMaxPlyrNum( ), pPlr->GetPlyrNum( ) + 1 ) );

    theGame.GetAll( ).AddTail( pPlr );

    if ( pPlr->IsAI( ) )
        theGame.GetAi( ).AddTail( pPlr );

    if ( pNp->m_bServer )
        theGame._SetServer( pPlr );

    EnMpDiagLog( "cmd_player: netnum=%d plyr=%d local=%d ai=%d server=%d name='%s'%s",
                 pNp->m_iNetNum, pNp->m_iPlyrNum, (int)pNp->m_bLocal, (int)pNp->m_bAI,
                 (int)pNp->m_bServer, pPlr->GetName( ),
                 ( theGame._GetMe( ) == pPlr ) ? " (THIS IS ME)" : "" );
}

// --- Deferred client start (multiplayer waiting-room lobby) ----------------
// While a joining client sits in its waiting-room lobby, a CNetStart from the
// host must NOT build the world immediately: that would run CreateNewWorld
// nested inside the lobby's modal message loop and re-enter BaseYield. Instead
// we stash the start params, let the lobby close, then build at the flow level.
bool g_bClientLobbyWaiting  = false;
bool g_bClientStartReceived = false;
// Set by OnMsgSessionClose when the host drops WHILE we're still in the client
// waiting room (pre-start). The lobby dialog polls it to show "the host has left"
// instead of waiting on "Waiting for the host..." forever. Reset at lobby entry.
bool g_bClientHostLost      = false;
static char g_clientStartBuf[ sizeof( CNetStart ) ];

static void CmdStart( CNetStart* pStrt );   // fwd

void RunDeferredClientStart( )
{
    fprintf( stderr, "[mp-start] RunDeferredClientStart (startReceived=%d) -> building world\n",
             (int)g_bClientStartReceived );
    if ( g_bClientStartReceived )
    {
        g_bClientStartReceived = false;
        CmdStart( (CNetStart*)g_clientStartBuf );
        fprintf( stderr, "[mp-start] deferred CmdStart returned (world build done or caught)\n" );
    }
}

static void CmdStart( CNetStart* pStrt )
{
    // [mp-start] stderr breadcrumbs: the 3-client join test (2026-07-01) had POSIX
    // clients silently bounce to the main menu when the host started; these mark
    // exactly how far the client start path got. Keep until MP start is stable.
    fprintf( stderr, "[mp-start] CNetStart received (lobbyWaiting=%d)\n", (int)g_bClientLobbyWaiting );
    if ( g_bClientLobbyWaiting )
    {
        memcpy( g_clientStartBuf, pStrt, sizeof( CNetStart ) );
        g_bClientStartReceived = true;
        return;   // lobby will close, then RunDeferredClientStart() builds the world
    }

    try
    {
        // create the world
        theGame.IncTry( );
        // adopt the host's world-generation preset + river/ocean sliders so our
        // seed-deterministic generator paints the identical map (synced via CNetStart).
        theGame.m_iWorldType = pStrt->m_iWorldType;
        theGame.m_iRivers    = pStrt->m_iRivers;
        theGame.m_iOcean     = pStrt->m_iOcean;
        if ( theApp.m_pCreateGame != NULL )
        {
            theApp.m_pCreateGame->m_iWorldType = pStrt->m_iWorldType;
            theApp.m_pCreateGame->m_iRivers    = pStrt->m_iRivers;
            theApp.m_pCreateGame->m_iOcean     = pStrt->m_iOcean;
        }
        // MP world-gen parity (Bug 2): freeze the count world-gen will use to the
        // HOST's authoritative roster size (numHp+numAi from CNetStart), NOT this
        // client's live list count — which can differ while the roster is still
        // settling (auto-start / late AI) and would shift the RNG stream -> RAND
        // MISMATCH -> uniform client kick. Set BEFORE CreateNewWorld (the client
        // path calls SetSideSize, not StartNewWorld, so nothing else sets it).
        theGame.m_iWorldGenCount = pStrt->m_iNumHp + pStrt->m_iNumAi;
        AIinit aiData( pStrt->m_iAi, pStrt->m_iNumAi, pStrt->m_iNumHp, pStrt->m_iStart );
        theApp.CreateNewWorld( pStrt->m_uRand, &aiData, pStrt->m_iSide, pStrt->m_iSideSize );
        theGame.DecTry( );
    }

    catch ( int iNum )
    {
        CatchNum( iNum );
        theApp.CloseWorld( );
        return;
    }
    catch ( SE_Exception e )
    {
        TRAP( );
        CatchSE( e );
        theApp.CloseWorld( );
        return;
    }
    catch ( ... )
    {
        TRAP( );
        CatchOther( );
        theApp.CloseWorld( );
        return;
    }
}

static void CmdPlyrStatus( CNetPlyrStatus* pMsg )
{

    if ( theApp.m_pCreateGame == NULL )
        return;

    try
    {
        theApp.m_pCreateGame->UpdatePlyrStatus( theGame.GetPlayer( pMsg->m_iNetNum ), pMsg->m_iStatus );
    }
    catch ( ... )
    {
        EnMessageBox( IDS_BAD_PLAYER_NUM, MB_OK | MB_ICONSTOP );
        theApp.CloseWorld( );
        return;
    }
}

static void CmdInitDone( CNetInitDone* pMsg )
{

#ifdef LOGGINGON
    OutputDebugStringA( "CmdInitDone" );
#endif

    CPlayer* pPlyr = theGame.GetPlayer( pMsg->m_iPlyrNum );
    if ( pPlyr == NULL )
    {
        ASSERT( FALSE );
        return;
    }
    if ( theApp.m_pCreateGame->m_iTyp != CCreateBase::load_multi )
        pPlyr->SetState( CPlayer::wait );
    else
        pPlyr->SetState( CPlayer::ready );

    POSITION pos;
    for ( pos = theGame.GetAll( ).GetTailPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.GetAll( ).GetPrev( pos );
        ASSERT_VALID( pPlr );
        if ( ( !pPlr->IsAI( ) ) && ( pPlr->GetState( ) != CPlayer::wait ) && ( pPlr->GetState( ) != CPlayer::replace ) )
            return;
    }

    if ( theApp.m_pCreateGame->m_iTyp == CCreateBase::load_multi )
    {
#ifdef LOGGINGON
        OutputDebugStringA( "LoadMulti\n" );
#endif
        TRAP( );
        theGame.StartGame( FALSE );
        return;
    }

    // we can start the AI
    theApp.StartAi( );
}

static void CmdPlay( CNetPlay* pMsg )
{

    // if our rand doesn't match we drop out
    if ( theGame.m_dwFinalRand != pMsg->m_uRand )
    {
        // Always log: this is the cross-platform world-gen desync gate (the
        // operator-visible "client disconnects when the host starts"), and the
        // two values are the only evidence of HOW far the PRNG streams diverged.
        fprintf( stderr, "[mp-start] RAND MISMATCH: client m_dwFinalRand=%08lx host m_uRand=%08lx -> world-gen diverged, dropping out (CmdPlay)\n",
                 (unsigned long)theGame.m_dwFinalRand, (unsigned long)pMsg->m_uRand );
        ASSERT( !theGame.AmServer( ) );
        theGame.Close( );
        theNet.Close( TRUE );
        EnMessageBox( IDS_RAND_MISMATCH, MB_OK | MB_ICONSTOP );
        theApp.CloseWorld( );
        return;
    }

    // enable the windows
    theApp.LetsGo( );
}


static void PlaceBldg( CMsgPlaceBldg* pMsg )
{

    ASSERT_CMD( pMsg );
    ASSERT( theGame.AmServer( ) );

    int iWhy;
    int iRtn = theMap.FoundationCost( pMsg->m_hexBldg, pMsg->m_iType, pMsg->m_iDir,
                                      theVehicleMap.GetVehicle( pMsg->m_dwIDVeh ), NULL, &iWhy );

    if ( theApp.m_pLogFile != NULL )
    {
        char sBuf[80];
        sprintf( sBuf, "Place building %d at %d,%d = cost: %d, why: %d", pMsg->m_iType, pMsg->m_hexBldg.X( ),
                 pMsg->m_hexBldg.Y( ), iRtn, iWhy );
        theApp.Log( sBuf );
    }

    pMsg->m_iWhy = (signed char)iWhy;
    if ( iRtn < 0 )
    {
        pMsg->ToErr( );
        theGame.PostToClient( pMsg->m_iPlyrNum, pMsg, sizeof( *pMsg ) );
        return;
    }

    // and tell the clients
    pMsg->ToNew( );
    if ( pMsg->m_dwIDBldg == 0 )
        pMsg->m_dwIDBldg = theGame.GetID( );

    // we call ourselves here so we own the hex
    BldgNew( (CMsgBldgNew*)pMsg );

    // tell the clients (if we didn't end on the rocket above)
    theGame.PostToAllClients( pMsg, sizeof( *pMsg ) );
}

static void ErrPlaceBldg( CMsgPlaceBldg* pMsg )
{

    if ( !theGame.GetPlayer( pMsg->m_iPlyrNum )->IsMe( ) )
        return;
    ASSERT_CMD( pMsg );

    TRAP( );  // BUGBUG - correct owner?
    // tell the user
    theGame.Event( EVENT_CONST_CANT, EVENT_WARN );
    CWndArea* pWnd = theAreaList.GetTop( );
    if ( pWnd != NULL )
        pWnd->SetupStart( );
}

static void BuildBldg( CMsgBuildBldg* pMsg )
{

    ASSERT_CMD( pMsg );
    ASSERT( theGame.AmServer( ) );
    ASSERT( pMsg->m_dwIDBldg == 0 );

    int iWhy;
    int iRtn = theMap.FoundationCost( pMsg->m_hexBldg, pMsg->m_iType, pMsg->m_iDir,
                                      theVehicleMap.GetVehicle( pMsg->m_dwIDVeh ), NULL, &iWhy );

    if ( theApp.m_pLogFile != NULL )
    {
        char sBuf[80];
        sprintf( sBuf, "Build building %d at %d,%d = cost: %d, why: %d", pMsg->m_iType, pMsg->m_hexBldg.X( ),
                 pMsg->m_hexBldg.Y( ), iRtn, iWhy );
        theApp.Log( sBuf );
    }

    pMsg->m_iWhy = (signed char)iWhy;
    if ( iRtn < 0 )
    {
        pMsg->ToErr( );
        theGame.PostToClient( pMsg->m_iPlyrNum, pMsg, sizeof( *pMsg ) );
        return;
    }

    CPlayer*              pPlr = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );
    const CStructureData* pSd  = theStructures.GetData( pMsg->m_iType );
    if ( !pSd->PlyrIsDiscovered( pPlr ) )
    {
        pMsg->ToErr( );
        theGame.PostToClient( pMsg->m_iPlyrNum, pMsg, sizeof( *pMsg ) );
        return;
    }

    // and tell the clients
    pMsg->ToNew( );
    if ( pMsg->m_dwIDBldg == 0 )
        pMsg->m_dwIDBldg = theGame.GetID( );

    // we call ourselves here so we own the hex
    BldgNew( (CMsgBldgNew*)pMsg );

    theGame.PostToAllClients( pMsg, sizeof( *pMsg ) );
}

static void ErrBuildBldg( CMsgBuildBldg* pMsg )
{

    // tell the user
    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwIDVeh );
    if ( pVeh == NULL )
        return;
    ASSERT_CMD( pMsg );
    ASSERT_VALID( pVeh );
    ASSERT( pVeh->GetOwner( )->IsLocal( ) );

    if ( pVeh->GetOwner( )->IsAI( ) )
    {
        TRAP( );
        AiMessage( pVeh->GetOwner( )->GetAiHdl( ), pMsg, sizeof( CMsgBuildBldg ) );
    }
    else
        theGame.Event( EVENT_CONST_CANT, EVENT_WARN, pVeh );
}

#if EN_GAMEPLAY_PROBES
extern void EnBridgeDbgLog( const char* pszMsg );   // vehicle.cpp — file sink, not DBWIN
#endif

static void BuildRoad( CMsgBuildRoad* pMsg )
{

    ASSERT( theGame.AmServer( ) );
    ASSERT_CMD( pMsg );

    CHex* pHex = theMap._GetHex( pMsg->m_hexBuild );

    if ( ( !pHex->CanRoad( ) ) || ( pHex->GetUnits( ) & CHex::bldg ) )
    {
#if EN_GAMEPLAY_PROBES
        {
            char szR[176];
            sprintf( szR, "[ROADSRV] REJECT plyr %d hex %d,%d type %d units 0x%x canroad %d\n",
                     pMsg->m_iPlyrNum, pMsg->m_hexBuild.X( ), pMsg->m_hexBuild.Y( ),
                     (int)pHex->GetType( ), (unsigned)pHex->GetUnits( ), pHex->CanRoad( ) ? 1 : 0 );
            EnBridgeDbgLog( szR );
        }
#endif
        pMsg->ToErr( );
        theGame.PostToClient( pMsg->m_iPlyrNum, pMsg, sizeof( *pMsg ) );
        return;
    }

    // and tell the clients
    pMsg->ToNew( );
    theGame.PostToAll( pMsg, sizeof( *pMsg ) );
}

// CBBData/fnEnumBaseBridge (the 3x3 base-regrade test) moved to terrain.cpp:
// the acceptance rule is CGameMap::BridgeSpanDeny, shared with the AI
// planner/dispatcher so client==server by construction.

static void BuildBridge( CMsgBuildBridge* pMsg )
{

    ASSERT( theGame.AmServer( ) );
    ASSERT_CMD( pMsg );

    // is it a legit path?
    CHexCoord _hexOn( pMsg->m_hexStart );
    int       xAdd = CHexCoord::Diff( pMsg->m_hexEnd.X( ) - _hexOn.X( ) );
    int       yAdd = CHexCoord::Diff( pMsg->m_hexEnd.Y( ) - _hexOn.Y( ) );
    xAdd           = __minmax( -1, 1, xAdd );
    yAdd           = __minmax( -1, 1, yAdd );

    // test params
    if ( ( ( xAdd != 0 ) && ( yAdd != 0 ) ) || ( pMsg->m_hexStart == pMsg->m_hexEnd ) )
    {
#if defined( _WIN32 ) && EN_GAMEPLAY_PROBES
        { char szB[112]; sprintf( szB, "[BRIDGESRV] REJECT params plyr %d span %d,%d -> %d,%d\n", pMsg->m_iPlyrNum,
                  pMsg->m_hexStart.X( ), pMsg->m_hexStart.Y( ), pMsg->m_hexEnd.X( ), pMsg->m_hexEnd.Y( ) );
          OutputDebugStringA( szB ); }
#endif
        pMsg->ToErr( );
        theGame.PostToClient( pMsg->m_iPlyrNum, pMsg, sizeof( *pMsg ) );
        return;
    }

    // span limit depends on the requesting player's bridge research tier.
    // m_iPlyrNum is a PLAYER number (senders fill it from GetPlyrNum /
    // CAIUnit::GetOwner) so look up by plyr num — GetPlayer matches NET
    // numbers and returns NULL on a miss (caught 2026-06-11: NULL->GetMaxSpan
    // AV in a 13-player game). The player can also be gone by the time a
    // queued command is processed, so a miss is survivable: drop the command.
    CPlayer* pPlyrSpan = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );
    if ( pPlyrSpan == NULL )
        return;
    int const iMaxSpan = pPlyrSpan->GetMaxSpan( );

    // shared server/AI acceptance rule - CGameMap::BridgeSpanDeny (the AI
    // planner/dispatcher call the SAME code, client==server by construction);
    // sets m_iAlt (the deck altitude) once the span walk passes
    int       iLen = 0;
    CHexCoord hexAt( pMsg->m_hexStart );
    int iDeny = theMap.BridgeSpanDeny( pMsg->m_hexStart, pMsg->m_hexEnd, iMaxSpan, &pMsg->m_iAlt, &hexAt, &iLen );
    if ( iDeny != CGameMap::bridge_ok )
    {
        ASSERT( iLen <= iMaxSpan );
#if defined( _WIN32 ) && EN_GAMEPLAY_PROBES
        if ( ( iDeny == CGameMap::bridge_too_long ) || ( iDeny == CGameMap::bridge_obstacle ) )
        { char szB[128]; sprintf( szB, "[BRIDGESRV] REJECT %s plyr %d span %d,%d -> %d,%d at %d,%d len %d max %d\n",
                  ( iDeny == CGameMap::bridge_too_long ) ? "span-too-long" : "obstacle-in-path", pMsg->m_iPlyrNum,
                  pMsg->m_hexStart.X( ), pMsg->m_hexStart.Y( ), pMsg->m_hexEnd.X( ), pMsg->m_hexEnd.Y( ),
                  hexAt.X( ), hexAt.Y( ), iLen, iMaxSpan );
          OutputDebugStringA( szB ); }
        else if ( iDeny == CGameMap::bridge_start_base )
        { char szB[112]; sprintf( szB, "[BRIDGESRV] REJECT start-base plyr %d at %d,%d\n", pMsg->m_iPlyrNum,
                  pMsg->m_hexStart.X( ), pMsg->m_hexStart.Y( ) );
          OutputDebugStringA( szB ); }
#endif
        // end-base: reachable at runtime via AI-planned spans (bldg/bridge
        // beside the landing at another altitude) -- was TRAP(), which killed
        // the game on the FIRST AI bridge order ever sent; answer the error
#ifdef _WIN32
        if ( iDeny == CGameMap::bridge_end_base )
        {
            char szB[96];
            sprintf( szB, "[BRIDGEDENY] plyr %d end-base fail at %d,%d\n", pMsg->m_iPlyrNum, pMsg->m_hexEnd.X( ),
                     pMsg->m_hexEnd.Y( ) );
            OutputDebugStringA( szB );
        }
#endif
        pMsg->ToErr( );
        theGame.PostToClient( pMsg->m_iPlyrNum, pMsg, sizeof( *pMsg ) );
        return;
    }

    /////////////////////////////
    // we can build it

    // set the alt at the ends
    theMap.GetHex( pMsg->m_hexStart.X( ) + ( xAdd == -1 ? 1 : 0 ), pMsg->m_hexStart.Y( ) + ( yAdd == 1 ? -1 : 0 ) )
        ->SetAlt( pMsg->m_iAlt );
    theMap.GetHex( pMsg->m_hexStart.X( ) + ( xAdd == 1 ? 0 : 1 ), pMsg->m_hexStart.Y( ) + ( yAdd == -1 ? 0 : -1 ) )
        ->SetAlt( pMsg->m_iAlt );
    theMap.GetHex( pMsg->m_hexEnd.X( ) + ( xAdd == 1 ? 1 : 0 ), pMsg->m_hexEnd.Y( ) + ( yAdd == -1 ? -1 : 0 ) )
        ->SetAlt( pMsg->m_iAlt );
    theMap.GetHex( pMsg->m_hexEnd.X( ) + ( xAdd == -1 ? 0 : 1 ), pMsg->m_hexEnd.Y( ) + ( yAdd == 1 ? 0 : -1 ) )
        ->SetAlt( pMsg->m_iAlt );

    // mark it
    _hexOn = pMsg->m_hexStart;
    goto StartMark;
    while ( _hexOn != pMsg->m_hexEnd )
    {
        _hexOn.X( ) += xAdd;
        _hexOn.Y( ) += yAdd;
        _hexOn.Wrap( );
    StartMark:
        theMap._GetHex( _hexOn )->OrUnits( CHex::bridge );
    }

    // it's ok - tell the clients
    pMsg->ToNew( );
    if ( pMsg->m_dwIDBrdg == 0 )
        pMsg->m_dwIDBrdg = theGame.GetID( );

#if defined( _WIN32 ) && EN_GAMEPLAY_PROBES
    { char szB[112]; sprintf( szB, "[BRIDGESRV] ACCEPT plyr %d span %d,%d -> %d,%d id %lu\n", pMsg->m_iPlyrNum,
              pMsg->m_hexStart.X( ), pMsg->m_hexStart.Y( ), pMsg->m_hexEnd.X( ), pMsg->m_hexEnd.Y( ),
              (unsigned long)pMsg->m_dwIDBrdg );
      OutputDebugStringA( szB ); }
#endif

    theGame.PostToAll( pMsg, sizeof( *pMsg ) );
}

static void ErrBuildRoad( CMsgBuildRoad* pMsg )
{

    // tell the user
    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwID );
    if ( pVeh == NULL )
        return;
    ASSERT_CMD( pMsg );
    ASSERT_VALID( pVeh );
    ASSERT( pVeh->GetOwner( )->IsLocal( ) );

    if ( pVeh->GetOwner( )->IsAI( ) )
    {
        TRAP( );
        AiMessage( pVeh->GetOwner( )->GetAiHdl( ), pMsg, sizeof( CMsgBuildRoad ) );
    }
    else
    {
#if EN_GAMEPLAY_PROBES
        { char szE[144]; sprintf( szE, "[HALT-SITE C: server refused ROAD] veh %lu hex %d,%d\n",
                  (unsigned long)pMsg->m_dwID, pMsg->m_hexBuild.X( ), pMsg->m_hexBuild.Y( ) );
          EnBridgeDbgLog( szE ); }
#endif
        theGame.Event( EVENT_ROAD_HALTED, EVENT_WARN, pVeh );
    }
}

static void ErrBuildBridge( CMsgBuildBridge* pMsg )
{

    // tell the user
    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwIDVeh );
    if ( pVeh == NULL )
        return;
    ASSERT_CMD( pMsg );
    ASSERT_VALID( pVeh );
    ASSERT( pVeh->GetOwner( )->IsLocal( ) );

    if ( pVeh->GetOwner( )->IsAI( ) )
    {
        TRAP( );
        AiMessage( pVeh->GetOwner( )->GetAiHdl( ), pMsg, sizeof( CMsgBuildBridge ) );
    }
    else
    {
#if EN_GAMEPLAY_PROBES
        { char szE[160]; sprintf( szE, "[HALT-SITE D: server refused BRIDGE] veh %lu span %d,%d -> %d,%d\n",
                  (unsigned long)pMsg->m_dwIDVeh, pMsg->m_hexStart.X( ), pMsg->m_hexStart.Y( ),
                  pMsg->m_hexEnd.X( ), pMsg->m_hexEnd.Y( ) );
          EnBridgeDbgLog( szE ); }
#endif
        // A refused BRIDGE said "Construction of a road has halted" — the operator spent a
        // session chasing that, since it names the wrong structure and no cause. The server's
        // refusals here are the bank-regrade rules (a building/bridge stands on ground the
        // deck approach would have to level) plus obstacle/span, i.e. "this ground won't take
        // a bridge". Uses one of the 1996 spare event slots; no wire change.
        theGame.Event( EVENT_BRIDGE_HALTED, EVENT_WARN, pVeh );
    }
}

static void RoadNew( CMsgRoadNew* pMsg )
{

    // get the pVeh
    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwID );
    if ( pVeh == NULL )
        return;
    ASSERT_CMD( pMsg );

    // start it
    if ( pMsg->m_iMode == _CMsgRoad::one_hex )
        pVeh->SetRoadHex( pMsg->m_hexBuild );

    pVeh->SetEventAndRoute( CVehicle::build_road, CVehicle::run );
}

static void BridgeNew( CMsgBridgeNew* pMsg )
{

    // get the pVeh
    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwIDVeh );
    if ( pVeh == NULL )
    {
        // crane died between order and here: BuildBridge already marked the
        // span - unmark or the orphaned bits poison every path probe (AV)
        CHexCoord _hexOn( pMsg->m_hexStart );
        int xAdd = __minmax( -1, 1, CHexCoord::Diff( pMsg->m_hexEnd.X( ) - _hexOn.X( ) ) );
        int yAdd = __minmax( -1, 1, CHexCoord::Diff( pMsg->m_hexEnd.Y( ) - _hexOn.Y( ) ) );
        for ( ;; )
        {
            if ( theBridgeHex.GetBridge( _hexOn ) == NULL )
                theMap._GetHex( _hexOn )->NandUnits( CHex::bridge );
            if ( _hexOn == pMsg->m_hexEnd || ( xAdd == 0 && yAdd == 0 ) )
                break;
            _hexOn.X( ) += xAdd;
            _hexOn.Y( ) += yAdd;
            _hexOn.Wrap( );
        }
#ifdef _WIN32
        {
            char szO[96];
            sprintf( szO, "[BRIDGEORPHAN] crane %lu gone, unmarked span %d,%d-%d,%d\n",
                     (unsigned long)pMsg->m_dwIDVeh, pMsg->m_hexStart.X( ), pMsg->m_hexStart.Y( ),
                     pMsg->m_hexEnd.X( ), pMsg->m_hexEnd.Y( ) );
            OutputDebugStringA( szO );
        }
#endif
        return;
    }
    ASSERT_CMD( pMsg );

    // AI
    if ( pMsg->m_iMode == _CMsgRoad::one_hex )
    {
        // eric's 1996 note said "if it builds a bridge and then stops - tell me
        // to delete this". It happened (2026-07-12, first AI pontoon). Deleted.
        pVeh->SetRoadHex( pMsg->m_hexStart, pMsg->m_hexEnd );
    }

    // start it
    pVeh->SetBridgeHex( pMsg->m_hexStart, pMsg->m_hexEnd, pMsg->m_dwIDBrdg, pMsg->m_iAlt );
    pVeh->SetEventAndRoute( CVehicle::build_road, CVehicle::run );
}

// change the tile
static void RoadDone( CMsgRoadDone* pMsg )
{

    CHex* pHex = theMap.GetHex( pMsg->m_hexBuild );

    // mark the bridge as completed
    if ( pHex->GetUnits( ) & CHex::bridge )
    {
        CBridgeUnit* pBu = theBridgeHex.GetBridge( pMsg->m_hexBuild );
        if ( pBu != NULL && pBu->GetParent( ) != NULL )
            pBu->GetParent( )->BridgeBuilt( );
    }
    else
        pHex->ChangeToRoad( pMsg->m_hexBuild );
}

static void PlaceVeh( CMsgPlaceVeh* pMsg )
{

    ASSERT_CMD( pMsg );
    ASSERT( theGame.AmServer( ) );

    CPlayer* pPlr = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );

    // tell everyone
    pMsg->ToNew( );
    if ( pMsg->m_dwID == 0 )
        pMsg->m_dwID = theGame.GetID( );

    theGame.PostToAll( pMsg, sizeof( *pMsg ) );
}

static void BldgNew( CMsgBldgNew* pMsg )
{

    ASSERT_CMD( pMsg );

    // get the pVeh if there is one
    CVehicle* pVeh = NULL;
    if ( pMsg->m_dwIDVeh != 0 )
        pVeh = theVehicleMap.GetVehicle( pMsg->m_dwIDVeh );

    // create it
    CBuilding* pBldg = CBuilding::Create( pMsg->m_hexBldg, pMsg->m_iType, pMsg->m_iDir, pVeh, pMsg->m_iPlyrNum,
                                          pMsg->m_dwIDBldg, pMsg->m_bShow );
    if ( pBldg == NULL )
        return;

    if ( pVeh != NULL )
        theGame.Event( EVENT_CONST_START, EVENT_NOTIFY, pBldg );

    // check for all done
    if ( theGame.HaveHP( ) && ( pMsg->m_iType == CStructureData::rocket ) )
        EnMpDiagLog( "bldg_new ROCKET: plyr=%d me=%d%s", pMsg->m_iPlyrNum,
                     theGame.GetMe( )->GetPlyrNum( ),
                     ( pMsg->m_iPlyrNum == theGame.GetMe( )->GetPlyrNum( ) )
                         ? " -> completes MY placement (SetupDone)" : "" );
    if ( theGame.HaveHP( ) )
        if ( ( pMsg->m_iType == CStructureData::rocket ) && ( pMsg->m_iPlyrNum == theGame.GetMe( )->GetPlyrNum( ) ) )
        {
            CWndArea* pWnd = theAreaList.GetTop( );
            if ( pWnd != NULL )
                pWnd->SetupDone( );
        }
}

static void BldgStat( CMsgBldgStat* pMsg )
{

    // nothing if doesn't exist or it's local (sent this message)
    CBuilding* pBldg = theBuildingMap.GetBldg( pMsg->m_dwID );
    if ( ( pBldg == NULL ) || ( pBldg->GetOwner( )->IsLocal( ) ) )
        return;
    ASSERT_CMD( pMsg );

    if ( pMsg->m_iFlags & CMsgBldgStat::built )
    {
        if ( ( pBldg->GetOwner( )->GetTheirRelations( ) == RELATIONS_ALLIANCE ) && ( pBldg->IsConstructing( ) ) &&
             ( pMsg->m_iConstDone == -1 ) )
            if ( ( pBldg->GetData( )->GetUnionType( ) == CStructureData::UTmine ) ||
                 ( pBldg->GetData( )->GetType( ) == CStructureData::lumber ) )
                theGame.m_pHpRtr->MsgGiveBldg( pBldg );
        pBldg->UpdateConst( pMsg );
    }

    if ( pMsg->m_iFlags & CMsgBldgStat::paused )
    {
        pBldg->SetFlag( CUnit::stopped );
        if ( pBldg->IsLive( ) )
            pBldg->EnableAnimations( FALSE );
    }

    if ( pMsg->m_iFlags & CMsgBldgStat::resumed )
    {
        pBldg->ClrFlag( CUnit::stopped );
        if ( pBldg->IsLive( ) )
            pBldg->EnableAnimations( TRUE );
    }
}

static void VehNew( CMsgVehNew* pMsg )
{

    ASSERT_CMD( pMsg );

    // veh_new carrying an EXISTING id is a re-place (the AI 10-min stuck
    // teleport posts CMsgPlaceVeh with the unit's id). Create has no relocate
    // path: it minted a duplicate object on the same id and orphaned the old
    // one on its hexes (ghost unit; soak38 14:18 wire TRAP). Relocate instead.
    if ( pMsg->m_dwID != 0 )
    {
        CVehicle* pExisting = theVehicleMap._GetVehicle( pMsg->m_dwID );
        if ( pExisting != NULL )
        {
            pExisting->RelocateTo( pMsg->m_ptHead, pMsg->m_hexDest );
            return;
        }
    }

    // create it
    CVehicle::Create( pMsg->m_ptHead, pMsg->m_ptTail, pMsg->m_iType, pMsg->m_iPlyrNum, pMsg->m_dwID,
                      (CVehicle::VEH_MODE)pMsg->m_iRouteMode, pMsg->m_hexDest, pMsg->m_dwIDBldg, pMsg->m_iDelay );
}

static void VehGoto( CMsgVehGoto* pMsg )
{

    ASSERT( theGame.AmServer( ) );

    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwID );
    if ( pVeh == NULL )
        return;
    ASSERT_CMD( pMsg );

    ASSERT_VALID_LOC( pVeh );
    if ( !pVeh->GetOwner( )->IsLocal( ) )
    {
        TRAP( );
        return;
    }

    // lets see if ptNext is taken
    CVehicle* pVehOwner;
    if ( pMsg->m_iOwn )
        pVehOwner = theVehicleHex._GetVehicle( pMsg->m_ptNext );
    else
        pVehOwner = NULL;

    if ( ( pVehOwner == NULL ) || ( pVehOwner == pVeh ) )
    {
        // its free - grab it
        if ( ( pVehOwner == NULL ) && ( pMsg->m_iOwn ) && ( pVeh->GetHexOwnership( ) ) )
            theVehicleHex.GrabHex( pMsg->m_ptNext, pVeh );

        pVeh->NewLocOn( );
        return;
    }

#ifdef _LOGOUT
    logPrintf( LOG_PRI_CRITICAL, LOG_VEH_MOVE, "Vehicle %d VehGotoErr sub (%d,%d))", pVeh->GetID( ), pMsg->m_ptNext.x,
               pMsg->m_ptNext.y );
#endif

    pMsg->ToErr( pVehOwner );
    theGame.PostToClient( pMsg->m_iPlyrNum, pMsg, sizeof( *pMsg ) );
}

static void VehLoc( CMsgVehLoc* pMsg )
{

    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwID );
    if ( pVeh == NULL )
    {
#ifdef _LOGOUT
        logPrintf( LOG_PRI_CRITICAL, LOG_VEH_MOVE, "VehicleLoc %d NULL", pMsg->m_dwID );
#endif
        return;
    }
    ASSERT_CMD( pMsg );

    // set our next dest
    pVeh->MsgSetNextHex( pMsg );

    // tell the AI
    if ( ( theGame.AmServer( ) ) && ( pVeh->GetOwner( )->IsAI( ) ) )
        AiMessage( pVeh->GetOwner( )->GetAiHdl( ), pMsg, sizeof( CMsgVehLoc ) );
}

static void ErrVehGoto( CMsgVehGoto* pMsg )
{

    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwID );
    if ( pVeh == NULL )
        return;
    ASSERT_CMD( pMsg );
    ASSERT( pVeh->GetOwner( )->IsLocal( ) );

    ASSERT( ( pVeh->GetPtHead( ) == pVeh->GetPtNext( ) ) || ( theVehicleHex._GetVehicle( pMsg->m_ptNext ) != pVeh ) );

    // if we are in a building go to cant_deploy
    CBuilding* pBldg = theBuildingHex._GetBuilding( pVeh->GetPtHead( ) );
    if ( ( pBldg != NULL ) && ( theBuildingHex._GetBuilding( pVeh->GetPtTail( ) ) != NULL ) )
    {
        TRAP( );
#ifdef _LOGOUT
        logPrintf( LOG_PRI_CRITICAL, LOG_VEH_MOVE, "VehicleLoc %d CantInBldg", pMsg->m_dwID );
#endif
        pVeh->CantInBldg( pBldg );
    }
    else
    {
        ASSERT( pMsg->m_iMode != CVehicle::moving );
        pVeh->_SetRouteMode( (CVehicle::VEH_MODE)pMsg->m_iMode );
    }
}

static void BuildVeh( CMsgBuildVeh* pMsg )
{

    CVehicleBuilding* pBldg = (CVehicleBuilding*)theBuildingMap.GetBldg( pMsg->m_dwID );
    if ( pBldg == NULL )
        return;
    ASSERT_CMD( pMsg );

    // tell the factory to start building the vehicle
    pBldg->StartVehicle( pMsg->m_iVehType, pMsg->m_iNum );
}

static void SetVehDest( CMsgVehSetDest* pMsg )
{

    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwID );
    if ( pVeh == NULL )
    {
#ifdef _LOGOUT
        logPrintf( LOG_PRI_CRITICAL, LOG_VEH_MOVE, "Vehicle %d doesn't exist (goto sub (%d,%d))", pMsg->m_dwID,
                   pMsg->m_sub.x, pMsg->m_sub.y );
#endif
        return;
    }

    ASSERT_CMD( pMsg );
    ASSERT_VALID( pVeh );
    ASSERT( pVeh->GetOwner( )->IsLocal( ) );
#ifdef _LOGOUT
    logPrintf( LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d SetVehDest to sub (%d,%d)", pVeh->GetID( ), pMsg->m_sub.x,
               pMsg->m_sub.y );
#endif

#if EN_AI_PROBES_ECON && defined(_WIN32)
    if ( pVeh->GetOwner( )->IsAI( ) )
    {
        char szE[112];
        sprintf( szE, "[ENGSET] plyr %d veh %lu vtype %d mode %d to %d,%d submode %d\n",
                 pVeh->GetOwner( )->GetPlyrNum( ), (unsigned long)pMsg->m_dwID,
                 pVeh->GetData( )->GetType( ), (int)pVeh->m_cMode,
                 pMsg->m_hex.X( ), pMsg->m_hex.Y( ), (int)pMsg->m_iSub );
        OutputDebugStringA( szE );
    }
#endif
    pVeh->SetEvent( CVehicle::none );
    if ( pMsg->m_iSub == CVehicle::sub )
        pVeh->SetDest( pMsg->m_sub );
    else
        pVeh->SetDestAndMode( pMsg->m_hex, (CVehicle::VEH_POS)pMsg->m_iSub );

    // this can happen if the unit needs to change its destination because its going to a building and 
    // needs to get to the entrance
#ifdef STRICTER_ASSERTS2
    ASSERT( ( pVeh->m_hexDest == pMsg->m_hex) ||
            ( ( theBuildingHex.GetBuilding( pVeh->m_hexDest ) != NULL ) &&
              ( theBuildingHex.GetBuilding( pVeh->m_hexDest ) == theBuildingHex.GetBuilding( pMsg->m_hex ) ) ) );
#endif

#ifdef _LOGOUT
    if ( pVeh->m_hexDest != pMsg->m_hex )
        logPrintf( LOG_PRI_CRITICAL, LOG_VEH_MOVE, "Vehicle %d SetVehDest (%d,%d) changed to (%d,%d)", pMsg->m_dwID,
                   pMsg->m_hex.X( ), pMsg->m_hex.Y( ), pVeh->GetHexDest( ).X( ), pVeh->GetHexDest( ).Y( ) );
#endif
}

static void UnitDamage( CMsgUnitDamage* pMsg )
{

    // if our target is dead - stop
    CUnit* pDamage = GetUnit( pMsg->m_dwIDDamage );
    if ( pDamage == NULL )
        return;

    // assess it here
    pDamage->DecDamagePoints( pMsg->m_iDamageShot, pMsg->m_dwIDShoot );

    // tell the AI (so AI doesn't have to switch over to using unit_set_damage)
    if ( pDamage->GetOwner( )->IsAI( ) )
        theGame.PostToClient( pDamage->GetOwner( ), pMsg, sizeof( CMsgUnitDamage ) );

    // tell everyone
    pDamage->SetUnitSetDamage( ::GetUnit( pMsg->m_dwIDShoot ) );
}

static void UnitSetDamage( CMsgUnitSetDamage* pMsg )
{

    // if our target is dead - stop
    CUnit* pDamage = GetUnit( pMsg->m_dwIDDamage );
    if ( pDamage == NULL )
        return;

    // assess it here
    pDamage->DecDamagePoints( pDamage->GetDamagePoints( ) - pMsg->m_iDamageLevel, pMsg->m_dwIDShoot );
}

static void TransMat( CMsgTransMat* pMsg )
{

    ASSERT( theGame.AmServer( ) );

    CUnit* pSrc  = ::GetUnit( pMsg->m_dwIDSrc );
    CUnit* pDest = ::GetUnit( pMsg->m_dwIDDest );
    if ( ( pSrc == NULL ) || ( pDest == NULL ) )
        return;
    ASSERT_CMD( pMsg );

    for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ )
    {
        // if the AI asks for too much we'll do it (less likely to confuse it)
        if ( pMsg->m_aiMat[iOn] > pSrc->GetStore( iOn ) )
            pMsg->m_aiMat[iOn] = pSrc->GetStore( iOn );

        // kill negative numbers
        if ( pMsg->m_aiMat[iOn] < 0 )
        {
            ASSERT( FALSE );
            pMsg->m_aiMat[iOn] = 0;
        }

        pSrc->AddToStore( iOn, -pMsg->m_aiMat[iOn] );
        pDest->AddToStore( iOn, pMsg->m_aiMat[iOn] );
    }

    // turn back on if paused
    if ( pSrc->GetUnitType( ) == CUnit::building )
        ( (CBuilding*)pSrc )->EventOff( );

    // check for scenario 7 copper unload
    if ( pDest->GetUnitType( ) == CUnit::building )
    {
        ( (CBuilding*)pDest )->EventOff( );
        if ( pDest->GetOwner( )->IsMe( ) )
            if ( ( theGame.GetScenario( ) == 7 ) && ( pDest->GetStore( CMaterialTypes::copper ) > 0 ) )
                theGame.m_iScenarioVar++;
    }

    // update the status
    pSrc->MaterialChange( );
    pDest->MaterialChange( );

#ifdef _DEBUG
    // make sure we didn't overfill a truck
    if ( pDest->GetUnitType( ) == CUnit::vehicle )
        ASSERT( pDest->GetTotalStore( ) <= ( (CVehicle*)pDest )->GetData( )->GetMaxMaterials( ) );
#endif
}

static void UnitDestroying( CMsgDestroyUnit* pCmd )
{

    CUnit* pUnit = ::GetUnit( pCmd->m_dwID );
    if ( pUnit == NULL )
        return;
    ASSERT_CMD( pCmd );

    if ( ( theApp.m_pLogFile != NULL ) && ( pUnit->GetUnitType( ) == CUnit::building ) )
    {
        TRAP( );
        CBuilding* pBldg = (CBuilding*)pUnit;
        char       sBuf[80];
        sprintf( sBuf, "SetDestroy building %d at %d,%d", pBldg->GetID( ), pBldg->GetHex( ).X( ),
                 pBldg->GetHex( ).Y( ) );
        theApp.Log( sBuf );
    }

    pUnit->SetDestroyUnit( );
}

static void StopUnitDestroying( CMsgDestroyUnit* pCmd )
{

    CUnit* pUnit = ::GetUnit( pCmd->m_dwID );
    if ( pUnit == NULL )
        return;
    ASSERT_CMD( pCmd );

    pUnit->StopDestroyUnit( );
}

static void DeleteUnit( CMsgDeleteUnit* pCmd )
{

    CUnit* pUnit = ::_GetUnit( pCmd->m_dwID );
    if ( pUnit == NULL )
        return;
    ASSERT_CMD( pCmd );
    ASSERT( pUnit->GetFlags( ) & CUnit::dying );

    CPlayer* pPlr;
    if ( pCmd->m_iPlyrKiller >= 0 )
        pPlr = theGame._GetPlayerByPlyr( pCmd->m_iPlyrKiller );
    else
        pPlr = NULL;

    if ( ( theApp.m_pLogFile != NULL ) && ( pUnit->GetUnitType( ) == CUnit::building ) )
    {
        CBuilding* pBldg = (CBuilding*)pUnit;
        char       sBuf[80];
        sprintf( sBuf, "DeleteUnit building %d at %d,%d", pBldg->GetID( ), pBldg->GetHex( ).X( ),
                 pBldg->GetHex( ).Y( ) );
        theApp.Log( sBuf );
    }

    // track the kill
    if ( ( pPlr != NULL ) && ( pPlr != pUnit->GetOwner( ) ) )
        switch ( pUnit->GetUnitType( ) )
        {
        case CUnit::building:
            pPlr->IncBldgsDest( );
            break;
        case CUnit::vehicle:
            pPlr->IncVehsDest( );
            break;
        }

    // kill if visible or not a building
    //   OR we are shooting at it (artillery can shoot further than it can spot)
    if ( ( pUnit->GetUnitType( ) != CUnit::building ) || ( pUnit->GetOwner( )->IsMe( ) ) || ( !theGame.HaveHP( ) ) ||
         ( ( (CBuilding*)pUnit )->IsLive( ) ) )
    {
        delete pUnit;
        return;
    }

    // mark building to kill when seen (dying flag will cause us to leave it alone)
    pUnit->SetFlag( CUnit::dead );
    pUnit->GetOwner( )->AddBldgsHave( -1 );
}

void Attack( CMsgAttack* pCmd )
{

    CUnit* pUnitSrc = GetUnit( pCmd->m_dwShooter );
    if ( pUnitSrc == NULL )
        return;

    CUnit* pUnitDest = GetUnit( pCmd->m_dwTarget );
    if ( pUnitDest == NULL )
        return;

    ASSERT_CMD( pCmd );

    // if a vehicle and we don't own the hex look for our covering
    if ( ( pUnitDest->GetUnitType( ) == CUnit::vehicle ) && ( !( (CVehicle*)pUnitDest )->GetHexOwnership( ) ) )
    {
        // if in a building they have to hit that instead
        CUnit* pTest = theBuildingHex._GetBuilding( ( (CVehicle*)pUnitDest )->GetPtHead( ) );
        if ( pTest != NULL )
            pUnitDest = pTest;
        else
            // if in a transporter hit the carrier
            if ( ( (CVehicle*)pUnitDest )->GetTransport( ) != NULL )
                pUnitDest = ( (CVehicle*)pUnitDest )->GetTransport( );
    }

#ifdef _LOGOUT
    logPrintf( LOG_PRI_USEFUL, LOG_ATTACK, "Unit %d SetTarget (%d)", pUnitSrc->GetID( ), pUnitDest->GetID( ) );
#endif

    pUnitSrc->_SetTarget( pUnitDest );
}

static void RepairVeh( CMsgRepairVeh* pMsg )
{

    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwIDVeh );
    if ( pVeh == NULL )
        return;
    ASSERT_CMD( pMsg );

    CBuilding* pBldg = theBuildingHex._GetBuilding( pVeh->GetPtHead( ) );
    if ( ( pBldg == NULL ) || ( pBldg->GetData( )->GetUnionType( ) != CStructureData::UTrepair ) )
    {
        ASSERT( FALSE );
        return;
    }

    ( (CRepairBuilding*)pBldg )->RepairVehicle( pVeh );
}

static void RepairBldg( CMsgRepairBldg* pMsg )
{

    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwIDVeh );
    if ( pVeh == NULL )
        return;

    CBuilding* pBldg = theBuildingMap.GetBldg( pMsg->m_dwIDBldg );
    if ( pBldg == NULL )
        return;

    ASSERT_CMD( pMsg );
    pVeh->SetEvent( CVehicle::repair_bldg );
    // If the crane is already standing on the target building's footprint, weld
    // in place. SetDest(building hex) on an inside-the-building vehicle triggers
    // ExitBuilding (unit.cpp:3138), which kicks it 3 hexes away and fizzles the
    // armed repair event -> the crane oscillates in/out and never welds. Small
    // buildings whose exit hex is inside the footprint hit this; larger ones
    // repair from an outside exit hex (guard false -> unchanged SetDest path).
    // StartConst here mirrors the on-arrival weld (vehmove.cpp:408-414).
    if ( theBuildingHex._GetBuilding( pVeh->GetPtHead( ) ) == pBldg )
        pVeh->StartConst( pBldg );
    else
        pVeh->SetDest( pBldg->GetHex( ) );
}

static void LoadCarrier( CMsgLoadCarrier* pMsg )
{

    CVehicle* pVehCargo = theVehicleMap.GetVehicle( pMsg->m_dwIDCargo );
    if ( pVehCargo == NULL )
        return;

    CVehicle* pVehCarrier = theVehicleMap.GetVehicle( pMsg->m_dwIDCarrier );
    if ( pVehCarrier == NULL )
        return;

    ASSERT_CMD( pMsg );

    int iAdd = pVehCargo->GetData( )->IsPeople( ) ? 1 : MAX_CARGO;

    // no loading on trucks
    if ( ( pVehCarrier->GetData( )->IsTransport( ) ) && ( !pVehCarrier->GetData( )->IsBoat( ) ) )
    {
        ASSERT( FALSE );
        return;
    }

    // can we do it
    if ( ( !pVehCarrier->GetData( )->IsCarrier( ) ) ||
         ( pVehCarrier->m_iCargoSize + iAdd > pVehCarrier->GetEffPeopleCarry( ) ) )
    {
        ASSERT( FALSE );
        return;
    }
    if ( pVehCarrier->GetData( )->IsBoat( ) )
    {
        if ( ( !( pVehCargo->GetData( )->GetVehFlags( ) & CTransportData::FLlc_carryable ) ) &&
             ( !pVehCargo->GetData( )->IsCarryable( ) ) )
        {
            ASSERT( FALSE );
            return;
        }
    }
    else if ( !( pVehCargo->GetData( )->IsCarryable( ) ) )
    {
        ASSERT( FALSE );
        return;
    }

#ifdef _LOGOUT
    logPrintf( LOG_PRI_USEFUL, LOG_VEH_MOVE, "Loaded vehicle %d on vehicle %d at sub (%d,%d)", pVehCargo->GetID( ),
               pVehCarrier->GetID( ), pVehCarrier->m_ptHead.x, pVehCarrier->m_ptHead.y );
#endif

    pVehCargo->ReleaseOwnership( );
    pVehCargo->SetTransport( pVehCarrier );

    ASSERT( ( pVehCarrier->GetData( )->IsBoat( ) && pVehCargo->GetData( )->IsTransport( ) ) ||
            ( pVehCarrier->m_iCargoSize <= pVehCarrier->GetEffPeopleCarry( ) ) );

    if ( pVehCargo->GetOwner( )->IsLocal( ) )
    {
        if ( pVehCargo->GetOwner( )->IsAI( ) )
        {
            pVehCargo->ToldAiStopOn( );
            CMsgLoaded msg( pVehCarrier, pVehCargo );
            theGame.PostToClient( pVehCargo->GetOwner( ), &msg, sizeof( msg ) );
        }
        else
            theAreaList.MaterialChange( pVehCarrier );
    }
}

static void UnloadCarrier( CMsgUnloadCarrier* pMsg )
{

    CVehicle* pVeh = theVehicleMap.GetVehicle( pMsg->m_dwID );
    if ( pVeh == NULL )
    {
        TRAP( );
        return;
    }
    ASSERT_CMD( pMsg );
    ASSERT_VALID( pVeh );

    pVeh->UnloadCarrier( );
}

static void UnitControl( CMsgUnitControl* pMsg )
{

    CUnit* pUnit = GetUnit( pMsg->m_dwID );
    if ( pUnit == NULL )
    {
        TRAP( );
        return;
    }
    ASSERT_CMD( pMsg );

    switch ( pMsg->m_cCmd )
    {
    case CMsgUnitControl::cancel:
        TRAP( );
        pUnit->CancelUnit( );
        break;
    case CMsgUnitControl::stop:
        pUnit->StopUnit( );
        break;
    case CMsgUnitControl::resume:
        pUnit->ResumeUnit( );
        break;
#ifdef _DEBUG
    default:
        ASSERT( FALSE );
        break;
#endif
    }
}

static void UnitAttacked( CMsgUnitAttacked* pMsg )
{

    CUnit* pTarget   = GetUnit( pMsg->m_dwIDtarget );
    CUnit* pAttacker = GetUnit( pMsg->m_dwIDme );
    if ( ( pTarget == NULL ) || ( pAttacker == NULL ) )
        return;
    ASSERT_CMD( pMsg );
    ASSERT( pTarget->GetOwner( )->IsLocal( ) );

    CPlayer* pPlyrTrgt = pTarget->GetOwner( );
    CPlayer* pPlyrAtk  = pAttacker->GetOwner( );

    // if it's me we have to do some stuff
    if ( pPlyrTrgt->IsMe( ) )
    {
        // if we are at neutral change or relations
        if ( !pPlyrAtk->IsMe( ) )
        {
            if ( pPlyrAtk->GetRelations( ) <= RELATIONS_NEUTRAL )
            {
                NewRelations( pPlyrAtk, RELATIONS_WAR );
                // I can't find why but pAttacker can be bad when this returns
                theGame.Event( EVENT_NEW_RELATIONS, EVENT_NOTIFY, pPlyrAtk );

                // so we exit here if that's the case
                pTarget   = GetUnit( pMsg->m_dwIDtarget );
                pAttacker = GetUnit( pMsg->m_dwIDme );
                if ( ( pTarget == NULL ) || ( pAttacker == NULL ) )
                    return;
            }
            if ( pTarget->GetUnitType( ) == CUnit::building )
                theGame.Event( EVENT_BLDG_UNDER_ATK, EVENT_NOTIFY, pTarget );
        }

        if ( pTarget->GetUnitType( ) == CUnit::vehicle )
        {
            CVehicle* pVeh = (CVehicle*)pTarget;
            if ( ( pVeh->GetRouteMode( ) == CVehicle::stop ) && ( pVeh->GetEvent( ) == CVehicle::none ) )
            {
                // if we have no oppo - and its' not friendly fire - shoot back
                if ( ( pVeh->GetFireRate( ) != 0 ) && ( pVeh->GetOppo( ) == NULL ) &&
                     ( !pAttacker->GetOwner( )->IsMe( ) ) &&
                     ( pAttacker->GetOwner( )->GetRelations( ) > RELATIONS_PEACE ) )
                    pVeh->SetOppo( pAttacker );

                // if we are a truck or crane and stopped - run away - HP only
                if ( pVeh->GetData( )->GetVehFlags( ) & CTransportData::FLcivilian )
                {
                    int xDif = CMapLoc::Diff( pTarget->GetMapLoc( ).x - pAttacker->GetMapLoc( ).x );
                    xDif     = __minmax( -1, 1, xDif );
                    int yDif = CMapLoc::Diff( pTarget->GetMapLoc( ).y - pAttacker->GetMapLoc( ).y );
                    yDif     = __minmax( -1, 1, yDif );
                    CSubHex _dest( ( (CVehicle*)pTarget )->GetPtHead( ).x + xDif * ( 4 + RandNum( 16 ) ),
                                   ( (CVehicle*)pTarget )->GetPtHead( ).y + yDif * ( 4 + RandNum( 16 ) ) );
                    _dest.Wrap( );

                    // make sure not a building
                    int iNum = 5;
                    while ( ( iNum > 0 ) && ( theBuildingHex._GetBuilding( _dest ) != NULL ) )
                    {
                        _dest.x += RandNum( 8 ) - 4;
                        _dest.y += RandNum( 8 ) - 4;
                        _dest.Wrap( );
                        iNum--;
                    }

                    // RUN AWAY!!
                    ( (CVehicle*)pTarget )->SetDest( _dest );
                    theGame.Event( EVENT_CONST_UNDER_ATK, EVENT_WARN, pTarget );
                }
            }
        }
    }

    if ( pTarget->GetFireRate( ) == 0 )
        return;

    // fire back if not firing at assigned target and not oppo firing at someone shooting at us
    if ( ( pTarget->GetOppo( ) != NULL ) && ( pTarget->GetOppo( ) != pTarget->GetTarget( ) ) )
        return;

    if ( ( pTarget->GetOppo( ) == NULL ) || ( pTarget->GetOppo( )->GetOppo( ) != pTarget ) )
    {
        CUnit* pUnitOppo   = pTarget->GetOppo( );
        int    iDamageOppo = 0;
        // we call check oppo because we may not have LOS back
        pTarget->CheckOppo( pAttacker, iDamageOppo, &pUnitOppo );
        pTarget->SetOppo( pUnitOppo );
    }
}

static void GameSpeed( CMsgGameSpeed* pMsg )
{

    // if not the server - this is the game speed
    if ( !theGame.AmServer( ) )
    {
        theGame.SetGameMul( pMsg->m_iSpeed );
        // CDlgFile removed (Phase 2d) — SDL2FileDialog re-reads speed on next open.
        return;
    }

    // set this player to this speed
    CPlayer* pPlr = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );
    if ( pPlr == NULL )
        return;
    pPlr->m_iGameSpeed = pMsg->m_iSpeed;

    // do we have a new speed?
    POSITION pos;
    int      iMin, iMax;
    iMax = 0;
    iMin = NUM_SPEEDS - 1;
    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
        ASSERT_STRICT_VALID( pPlr );
        if ( !pPlr->IsAI( ) )
        {
            iMin = __min( iMin, pPlr->m_iGameSpeed );
            iMax = __max( iMax, pPlr->m_iGameSpeed );
        }
    }

    // we tend toward the middle
    if ( iMax < NUM_SPEEDS / 2 )
    {
        if ( iMax != theGame.GetGameMul( ) )
        {
            CMsgGameSpeed msg( iMax );
            theGame.PostToAllClients( &msg, sizeof( msg ), FALSE );
            theGame.SetGameMul( iMax );
        }
    }
    else if ( iMin > NUM_SPEEDS / 2 )
    {
        if ( iMin != theGame.GetGameMul( ) )
        {
            CMsgGameSpeed msg( iMin );
            theGame.PostToAllClients( &msg, sizeof( msg ), FALSE );
            theGame.SetGameMul( iMin );
        }
    }
    else
    {
        if ( theGame.GetGameMul( ) != NUM_SPEEDS / 2 )
        {
            CMsgGameSpeed msg( NUM_SPEEDS / 2 );
            theGame.PostToAllClients( &msg, sizeof( msg ), FALSE );
            theGame.SetGameMul( NUM_SPEEDS / 2 );
            // CDlgFile removed (Phase 2d) — SDL2FileDialog re-reads speed on next open.
        }
    }
}

static void SetRelations( CMsgSetRelations* pMsg )
{

    if ( !theGame.HaveHP( ) )
        return;

    if ( pMsg->m_iPlyrNumGet != theGame.GetMe( )->GetPlyrNum( ) )
    {
        TRAP( );
        return;
    }
    CPlayer* pPlyr = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNumSet );
    if ( pPlyr == NULL )
    {
        TRAP( );
        ASSERT( FALSE );
        return;
    }
    if ( pPlyr->IsMe( ) )
    {
        TRAP( );
        return;
    }

    int iOld = pPlyr->GetTheirRelations( );

    // set the relations
    pPlyr->SetTheirRelations( pMsg->m_iLevel );
    theGame.CheckAlliances( );

    // something is very wrong
    if ( pPlyr->IsLocal( ) )
    {
        TRAP( );
        return;
    }

    // if old was alliance and new isn't - zero out the building supplies, spotting
    if ( ( iOld == RELATIONS_ALLIANCE ) && ( pMsg->m_iLevel != RELATIONS_ALLIANCE ) )
    {
        POSITION pos = theBuildingMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD      dwID;
            CBuilding* pBldg;
            theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
            if ( pBldg->GetOwner( ) == pPlyr )
            {
                if ( pBldg->SpottingOn( ) )
                    pBldg->DecrementSpotting( );

                if ( ( pBldg->GetData( )->GetUnionType( ) == CStructureData::UTmine ) ||
                     ( pBldg->GetData( )->GetType( ) == CStructureData::lumber ) )
                {
                    if ( !pBldg->GetOwner( )->IsLocal( ) )
                        pBldg->ZeroStore( );
                    if ( !pBldg->IsConstructing( ) )
                        theGame.m_pHpRtr->MsgTakeBldg( pBldg );
                }
            }
        }

        // no scout spotting (faster to test for SpottingOn)
        pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            if ( ( pVeh->GetOwner( ) == pPlyr ) && ( pVeh->SpottingOn( ) ) )
            {
                pVeh->DecrementSpotting( );
                pVeh->DoSpottingOff( );
            }
        }
    }

    // is now alliance - add to the router, spotting
    if ( ( iOld != RELATIONS_ALLIANCE ) && ( pMsg->m_iLevel == RELATIONS_ALLIANCE ) )
    {
        POSITION pos = theBuildingMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD      dwID;
            CBuilding* pBldg;
            theBuildingMap.GetNextAssoc( pos, dwID, pBldg );

            // only mines & lumber mill
            if ( pBldg->GetOwner( ) == pPlyr )
            {
                if ( pBldg->IsFlag( CUnit::dead ) )
                    delete pBldg;
                else
                {
                    pBldg->MakeBldgVisible( );
                    pBldg->DetermineSpotting( );
                    pBldg->IncrementSpotting( pBldg->GetHex( ) );

                    if ( ( pBldg->GetData( )->GetUnionType( ) == CStructureData::UTmine ) ||
                         ( pBldg->GetData( )->GetType( ) == CStructureData::lumber ) )
                        if ( !pBldg->IsConstructing( ) )
                            theGame.m_pHpRtr->MsgGiveBldg( pBldg );
                }
            }
        }

        // scouts can spot!!!
        pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            if ( ( pVeh->GetOwner( ) == pPlyr ) && ( pVeh->GetHexOwnership( ) ) &&
                 ( ( pVeh->GetData( )->GetType( ) == CTransportData::light_scout ) ||
                   ( pVeh->GetData( )->GetType( ) == CTransportData::med_scout ) ) )
            {
                pVeh->DoSpottingOn( );
                pVeh->DetermineSpotting( );
                pVeh->IncrementSpotting( pVeh->GetHexHead( ) );
            }
        }
    }
}

static void BldgMat( CMsgBldgMat* pMsg )
{

    CUnit* pUnit = GetUnit( pMsg->m_dwID );
    if ( pUnit == NULL )
        return;

    pUnit->StoreMsg( pMsg );
}

// this guy needs info on all local players
static void NeedSaveInfo( CNetNeedSaveInfo* pMsg )
{

    CPlayer* pPlr = theGame._GetPlayerByPlyr( pMsg->m_iPlyrNum );
    if ( ( pPlr == NULL ) || ( pPlr->IsLocal( ) ) )
        return;

    // send the base player info
    POSITION pos;
    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlrOn = theGame.GetAll( ).GetNext( pos );
        if ( pPlrOn->IsLocal( ) )
        {
            CNetSaveInfo msg( pPlrOn );
            theGame.PostClientToClient( pPlr, &msg, sizeof( msg ) );
        }
    }

    // send materials in local buildings
    pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        if ( ( pBldg->GetOwner( )->IsLocal( ) ) && ( pBldg->GetTotalStore( ) > 0 ) )
        {
            CMsgBldgMat msg( pBldg, TRUE );
            theGame.PostClientToClient( pPlr, &msg, sizeof( msg ) );
        }
    }

    // and local vehicles
    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        if ( ( pVeh->GetOwner( )->IsLocal( ) ) && ( pVeh->GetTotalStore( ) > 0 ) )
        {
            CMsgBldgMat msg( pVeh, TRUE );
            theGame.PostClientToClient( pPlr, &msg, sizeof( msg ) );
        }
    }
}

void CGame::ProcessMessage(CNetCmd* pCmd )
{
    // Add validation for corrupted messages**
    if ( pCmd == NULL )
    {
        TRACE( "ProcessMessage: NULL command pointer\n" );
        return;
    }

    int msgType = pCmd->GetType( );
    if ( msgType < 0 || msgType >= CNetCmd::last_message )
    {
        TRACE( "ProcessMessage: Invalid message type %d (corrupted message?)\n", msgType );
        return;  // Skip corrupted messages instead of asserting
    }

#ifdef _LOG_LAG
    if ( theApp.m_pLogFile != NULL )
    {
        DWORD dwNow = timeGetTime( );
        if ( dwNow - pCmd->dwPostTime > 2 * 1000 )
        {
            char       sBuf[80];
            SYSTEMTIME st;
            GetLocalTime( &st );
            sprintf( sBuf, "%d:%d - Msg %d(%d) lag of %d seconds AddQueue to ProcessMsg", st.wMinute, st.wSecond,
                     pCmd->GetType( ), pCmd->m_bMemPool, ( dwNow - pCmd->dwPostTime + 500 ) / 1000 );
            theApp.Log( sBuf );
        }
    }
#endif

    ASSERT(theGame.ShouldProcessMessages() );
    ASSERT_VALID( this );

#ifdef LOGGINGON
   // char str[128];
   // snprintf( str, sizeof(str), "ProcessMessage (PM1) type %d\n", pCmd->GetType( ) );
   // OutputDebugStringA( str );
#endif

    switch ( pCmd->GetType( ) )
    {
    case CNetCmd::cmd_ready:
        CmdReady( (CNetReady*)pCmd );
        break;
    case CNetCmd::cmd_you_are:
        CmdYouAre( ( (CNetYouAre*)pCmd )->m_iPlyrNum, ( (CNetYouAre*)pCmd )->m_iServerNum );
        break;
    case CNetCmd::cmd_player:
        CmdPlayer( (CNetPlayer*)pCmd );
        break;
    case CNetCmd::cmd_start:
        CmdStart( (CNetStart*)pCmd );
        break;

    case CNetCmd::cmd_plyr_status:
        CmdPlyrStatus( (CNetPlyrStatus*)pCmd );
        break;
    case CNetCmd::cmd_init_done:

#ifdef LOGGINGON
        str.Format( "cmd_init_done!\n" );
        OutputDebugStringA( str );
#endif

        CmdInitDone( (CNetInitDone*)pCmd );
        break;
    case CNetCmd::cmd_play:
        CmdPlay( (CNetPlay*)pCmd );
        break;
    case CNetCmd::cmd_chat: {
        const CNetChat* pChat = (const CNetChat*)pCmd;
        // Look up sender name from net number
        CPlayer* pSender = theGame._GetPlayer( pChat->m_iPlyrNetNum );
        std::string from = pSender ? pSender->GetName() : "?";
        SDL2Chat_AddMessage( from + ": " + pChat->m_sMsg );
        break;
    }

    case CNetCmd::cmd_to_ai: {
        CPlayer* pPlr = theGame.GetPlayer( ( (CNetToAi*)pCmd )->m_iPlyrNum );
        if ( pPlr == NULL )
        {
            TRAP( );
            break;
        }

        EnMpDiagLog( "cmd_to_ai: plyr=%d name='%s' netnum=%d isMe=%d amServer=%d",
                     pPlr->GetPlyrNum( ), pPlr->GetName( ), pPlr->GetNetNum( ),
                     ( theGame._GetMe( ) == pPlr ) ? 1 : 0, (int)theGame.AmServer( ) );

        // if it was the server fix it
        if ( pPlr == theGame.GetServer( ) )
        {
            TRAP( );
            theGame._SetServer( NULL );
        }

        // if it was the server player we have to handle the LEAVE stuff here
        std::string sMsg;
        BOOL    bMsg;
        if ( ( !theGame.AmServer( ) ) && ( pPlr->GetNetNum( ) != 0 ) )
        {
            sMsg = strPrintf( EnLoadStdString( IDS_MSG_NET_GOODBYE ).c_str(),
                              pPlr->GetName( ) );

            theGame.AiTakeOverPlayer( pPlr, TRUE );

            // if now single player loose the comm
            if ( theGame.GetAll( ).GetCount( ) == theGame.GetAi( ).GetCount( ) + 1 )
            {
                theApp.m_wndChat.DestroyWindow( );
                theApp.CloseDlgChat( );
            }

            bMsg = TRUE;
        }
        else
            bMsg = FALSE;

        // close down chat session (if one)
        if ( theGame.GetAll( ).GetCount( ) == theGame.GetAi( ).GetCount( ) + 1 )
            theApp.m_wndChat.KillAiChatWnd( pPlr );

        // if we can, allow others to come claim this
        if ( ( theGame.AmServer( ) ) && ( theGame.GetNetJoin( ) == CGame::any ) &&
             ( theGame.GetAi( ).GetCount( ) > 0 ) )
        {
            TRAP( );
            theNet.SetSessionVisibility( TRUE );
        }

        // (CDlgRelations relations refresh removed; SDL2RelationsDialog is modal)

        // tell the player
        if ( bMsg )
        {
            if ( theApp.m_wndBar.IsCreated() )
                theApp.m_wndBar.SetStatusText( 0, sMsg.c_str() );
            if ( theGame.GetState( ) == CGame::play )
            {
                CDlgModelessMsg* pDlg = new CDlgModelessMsg( );
                pDlg->Create( sMsg.c_str() );
            }
        }
        break;
    }

    case CNetCmd::cmd_to_hp: {
        TRAP( );
        CNetToHp* pMsg = (CNetToHp*)pCmd;
        CPlayer*  pPlr = theGame.GetPlayer( pMsg->m_iPlyrNum );
        pPlr->SetAI( FALSE );
        pPlr->SetNetNum( pMsg->m_iNetNum );
        pPlr->SetName( pMsg->m_sName );

        // if there is nothing left stop enumerating
        if ( ( theGame.GetNetJoin( ) == CGame::any ) && ( theGame.GetAi( ).GetCount( ) <= 0 ) )
        {
            TRAP( );
            theNet.SetSessionVisibility( FALSE );
        }

        // (CDlgRelations relations refresh removed; SDL2RelationsDialog is modal)
        break;
    }

    case CNetCmd::cmd_enum_plyrs:
        CmdEnumPlyrs( ( (CNetEnumPlyrs*)pCmd )->m_iNetNum );
        break;
    case CNetCmd::cmd_plyr_join:
        CmdPlyrJoin( (CNetPlyrJoin*)pCmd );
        break;
    case CNetCmd::cmd_select_plyr:
        CmdSelectPlyr( (CNetSelectPlyr*)pCmd );
        break;
    case CNetCmd::cmd_select_ok:
        CmdSelectOk( (CNetSelectPlyr*)pCmd );
        break;
    case CNetCmd::cmd_select_not_ok:
        CmdSelectNotOk( (CNetSelectPlyr*)pCmd );
        break;
    case CNetCmd::cmd_plyr_taken:
        CmdPlayerTaken( (CNetSelectPlyr*)pCmd );
        break;
    case CNetCmd::cmd_get_file:
        CmdGetFile( (CNetGetFile*)pCmd );
        break;

    case CNetCmd::place_bldg:  // place original bldgs
        PlaceBldg( (CMsgPlaceBldg*)pCmd );
        break;
    case CNetCmd::bldg_new:
        BldgNew( (CMsgBldgNew*)pCmd );
        break;
    case CNetCmd::err_place_bldg:
        ErrPlaceBldg( (CMsgPlaceBldg*)pCmd );
        break;

    case CNetCmd::place_veh:  // place original vehicles
        PlaceVeh( (CMsgPlaceVeh*)pCmd );
        break;
    case CNetCmd::veh_new:
        VehNew( (CMsgVehNew*)pCmd );
        break;

    case CNetCmd::veh_goto:  // vehicle to next hex
        VehGoto( (CMsgVehGoto*)pCmd );
        break;
    case CNetCmd::veh_loc:
        VehLoc( (CMsgVehLoc*)pCmd );
        break;
    case CNetCmd::err_veh_goto:
        ErrVehGoto( (CMsgVehGoto*)pCmd );
        break;

    case CNetCmd::trans_mat:  // transfer material
        TransMat( (CMsgTransMat*)pCmd );
        break;

    case CNetCmd::build_veh:
        BuildVeh( (CMsgBuildVeh*)pCmd );
        break;
    case CNetCmd::build_bldg:
        BuildBldg( (CMsgBuildBldg*)pCmd );
        break;
    case CNetCmd::err_build_bldg:
        ErrBuildBldg( (CMsgBuildBldg*)pCmd );
        break;
    case CNetCmd::bldg_stat:
        BldgStat( (CMsgBldgStat*)pCmd );
        break;

    case CNetCmd::build_road:
        BuildRoad( (CMsgBuildRoad*)pCmd );
        break;
    case CNetCmd::err_build_road:
        ErrBuildRoad( (CMsgBuildRoad*)pCmd );
        break;
    case CNetCmd::road_new:
        RoadNew( (CMsgRoadNew*)pCmd );
        break;
    case CNetCmd::road_done:
        RoadDone( (CMsgRoadDone*)pCmd );
        break;

    case CNetCmd::build_bridge:
        BuildBridge( (CMsgBuildBridge*)pCmd );
        break;
    case CNetCmd::err_build_bridge:
        ErrBuildBridge( (CMsgBuildBridge*)pCmd );
        break;
    case CNetCmd::bridge_new:
        BridgeNew( (CMsgBridgeNew*)pCmd );
        break;

    case CNetCmd::veh_set_dest:
        SetVehDest( (CMsgVehSetDest*)pCmd );
        break;

    case CNetCmd::shoot_gun: {
        CMsgShoot*     pMsg  = (CMsgShoot*)pCmd;
        CMsgShootElem* pElem = &( pMsg->m_aMSE[0] );
        for ( int iInd = 0; iInd < pMsg->m_iNumMsgs; iInd++ )
        {
            CUnit* pShoot = GetUnit( pElem->m_dwID );
            if ( pShoot != NULL )
                pShoot->MsgSetFire( pElem );
            pElem++;
        }
        break;
    }

    case CNetCmd::unit_damage:
        TRAP( );
        UnitDamage( (CMsgUnitDamage*)pCmd );
        break;

    case CNetCmd::unit_set_damage:
        TRAP( );
        UnitSetDamage( (CMsgUnitSetDamage*)pCmd );
        break;

    case CNetCmd::unit_repair: {
        CMsgUnitRepair* pMsg = (CMsgUnitRepair*)pCmd;
        pMsg->ToSetRepair( );

        // if our target is dead - stop
        CUnit* pUnit = GetUnit( pMsg->m_dwID );
        if ( pUnit == NULL )
            break;

        pUnit->DecDamagePoints( -pMsg->m_iRepair );
        pMsg->m_iDamageLevel = pUnit->GetDamagePoints( );
        PostToAllClients( pMsg, sizeof( *pMsg ) );
        break;
    }

    case CNetCmd::unit_set_repair: {
        CMsgUnitRepair* pMsg  = (CMsgUnitRepair*)pCmd;
        CUnit*          pUnit = GetUnit( pMsg->m_dwID );
        if ( pUnit == NULL )
            break;

        pUnit->DecDamagePoints( -pMsg->m_iRepair );
        break;
    }

    case CNetCmd::destroy_unit: {
        CMsgDestroyUnit* pMsg = (CMsgDestroyUnit*)pCmd;
        pMsg->m_bMsg          = CNetCmd::unit_destroying;
        PostToAll( pCmd, sizeof( CMsgDestroyUnit ) );
        break;
    }
    case CNetCmd::unit_destroying: {
        UnitDestroying( (CMsgDestroyUnit*)pCmd );
        break;
    }
    case CNetCmd::stop_destroy_unit: {
        CMsgDestroyUnit* pMsg = (CMsgDestroyUnit*)pCmd;
        pMsg->m_bMsg          = CNetCmd::stop_unit_destroying;
        PostToAll( pCmd, sizeof( CMsgDestroyUnit ) );
        break;
    }
    case CNetCmd::stop_unit_destroying: {
        StopUnitDestroying( (CMsgDestroyUnit*)pCmd );
        break;
    }

    case CNetCmd::delete_unit: {
        DeleteUnit( (CMsgDeleteUnit*)pCmd );
        break;
    }

    case CNetCmd::ipc_msg:
        // Email/chat over the network. The old CWndComm::IncomingMessage was a
        // stub (no-op) — route to the SDL2 mail subsystem instead so email
        // actually lands in the inbox. (Modern chat uses CNetChat above.)
        SDL2Mail_HandleIncoming( (CMsgIPC*)pCmd );
        break;

    case CNetCmd::attack:
        Attack( (CMsgAttack*)pCmd );
        break;

    case CNetCmd::deploy_it: {
        CVehicle* pVeh = theVehicleMap.GetVehicle( ( (CMsgDeployIt*)pCmd )->m_dwID );
        if ( pVeh == NULL )
            break;
        if ( !theGame.AmServer( ) )
            pVeh->TakeOwnership( );
        ASSERT( pVeh->GetOwner( )->IsLocal( ) );
        pVeh->_SetRouteMode( CVehicle::deploy_it );
        break;
    }

    case CNetCmd::plyr_dying: {
        // remove from list of players & then post again
        CMsgPlyrDying* pMsg = (CMsgPlyrDying*)pCmd;
        CPlayer*       pPlr = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );

        // ok - remove it
        theGame.RemovePlayer( pPlr );

        // tell the player - ONCE
        if ( ( !pPlr->m_bMsgDead ) && ( pPlr != theGame._GetMe( ) ) )
        {
            pPlr->m_bMsgDead = TRUE;
            theGame.Event( EVENT_PLAYER_DEAD, EVENT_NOTIFY, pPlr );
            std::string sMsg = strPrintf( EnLoadStdString( IDS_EVENT_DEAD ).c_str(),
                                          pPlr->GetName( ) );
            CDlgModelessMsg* pDlg = new CDlgModelessMsg( );
            pDlg->Create( sMsg.c_str() );
        }
        break;
    }

    case CNetCmd::set_rsrch: {

        CMsgRsrch* pMsg = (CMsgRsrch*)pCmd;
        CPlayer*   pPlr = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );
        if (!pPlr)
        {
            
#if EN_AI_PROBES_ECON && defined(_WIN32)
            OutputDebugStringA("set_rsrch: player not found\n");
#endif
            break;
        }
        int        iSize = pPlr->GetRsrchSize( );
        int        iOn   = pPlr->GetRsrchItem( );
        if ( iOn > 0 && iOn < iSize )
        {
            CRsrchStatus* pRs = &( pPlr->GetRsrch( iOn ) );
            pRs->m_iPtsDiscovered -= pRs->m_iPtsDiscovered / 10;
        }
        // out-of-range topic (garbage from a stale/reused message buffer): drop
        // it before ANY GetRsrch deref - the lower-bound-only guard below read
        // ~30GB past the 114-entry array (v48 crash, full dump 2026-07-10)
        if ( pMsg->m_iTopic < 0 || pMsg->m_iTopic >= iSize )
        {
#if EN_AI_PROBES_ECON && defined(_WIN32)
            OutputDebugStringA( "set_rsrch: out-of-range topic dropped\n" );
#endif
            break;
        }
        // Root-cause guard for the CPlayer::Research ASSERT( !m_bDiscovered )
        // crash: set_rsrch is posted asynchronously (e.g. AI CheckResearch ->
        // NextResearchTopic -> PostToServer). Between the topic being chosen and
        // this message being processed, that topic can already become discovered
        // — the AI is handed techs for free when other players research them
        // (research_disc handler), and Research() can complete it independently.
        // Pointing m_iRsrchItem at an already-discovered topic is exactly the
        // stale state the assert catches. Drop the stale request instead of
        // entering it; the AI's CheckResearch will pick a fresh topic next pass.
        if ( pMsg->m_iTopic > 0 &&
             pPlr->GetRsrch( pMsg->m_iTopic ).m_bDiscovered )
        {
#if EN_AI_PROBES_ECON && defined(_WIN32)
            OutputDebugStringA( "set_rsrch: ignoring already-discovered topic (stale request)\n" );
#endif
            pPlr->SetRsrchItem( 0 );
            break;
        }
        pPlr->SetRsrchItem( pMsg->m_iTopic );
        break;
    }

    case CNetCmd::repair_veh:
        RepairVeh( (CMsgRepairVeh*)pCmd );
        break;

    case CNetCmd::repair_bldg:
        RepairBldg( (CMsgRepairBldg*)pCmd );
        break;

    case CNetCmd::load_carrier:
        LoadCarrier( (CMsgLoadCarrier*)pCmd );
        break;
    case CNetCmd::unload_carrier:
        UnloadCarrier( (CMsgUnloadCarrier*)pCmd );
        break;

    case CNetCmd::unit_control:
        UnitControl( (CMsgUnitControl*)pCmd );
        break;

    case CNetCmd::ai_msg: {
        CMsgAiMsg* pMsg = (CMsgAiMsg*)pCmd;
        CPlayer*   pPlr = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );
        AiMessage( pPlr->GetAiHdl( ), pMsg + 1, pMsg->m_iLen );
        break;
    }

    case CNetCmd::cmd_pause:
        if ( !theGame.AmServer( ) )
        {
            theApp.m_wndMain._EnableGameWindows( FALSE );
            theApp.GetDlgPause( )->Show( CDlgPause::client );
            theGame.SetShouldOperate(FALSE);
        }
        break;

    case CNetCmd::cmd_resume:
        if ( !theGame.AmServer( ) )
        {
            theApp.m_wndMain._EnableGameWindows( TRUE );
            theApp.GetDlgPause( )->Show( CDlgPause::off );
            theGame.SetShouldOperate(TRUE);
        }
        break;

    case CNetCmd::unit_attacked:
        UnitAttacked( (CMsgUnitAttacked*)pCmd );
        break;

    case CNetCmd::set_relations:
        SetRelations( (CMsgSetRelations*)pCmd );
        break;

    case CNetCmd::game_speed:
        GameSpeed( (CMsgGameSpeed*)pCmd );
        break;

    case CNetCmd::bldg_materials:
        BldgMat( (CMsgBldgMat*)pCmd );
        break;

    case CNetCmd::give_unit: {
        CMsgGiveUnit* pMsg  = (CMsgGiveUnit*)pCmd;
        CUnit*        pUnit = ::GetUnit( pMsg->m_dwID );
        if ( pUnit == NULL )
            break;
        CPlayer* pPlr = theGame._GetPlayerByPlyr( pMsg->m_iPlyrNum );
        if ( ( pPlr == NULL ) || ( pPlr == pUnit->GetOwner( ) ) )
            break;
        // update stores
        for ( int iInd = 0; iInd < CMaterialTypes::num_types; iInd++ ) pUnit->SetStore( iInd, pMsg->m_aiStore[iInd] );
        pUnit->m_iUpdateMat = 0;
        pUnit->_SetOwner( pPlr );
        break;
    }

    case CNetCmd::set_time: {
        if ( theGame.AmServer( ) )
            break;
        CMsgSetTime* pMsg = (CMsgSetTime*)pCmd;
        theGame.SetElapsedSeconds( pMsg->m_dwTime );
        break;
    }

    case CNetCmd::start_file:
        StartFile( (CMsgStartFile*)pCmd );
        break;

    case CNetCmd::start_loaded_game:
        theGame.StartGame( FALSE );
        break;

    case CNetCmd::cancel_load: {
        TRAP( );
        CPlayer* pPlyr = theGame._GetPlayer( ( (CMsgCancelLoad*)pCmd )->m_iNetNum );
        if ( ( pPlyr != NULL ) && ( theApp.m_pCreateGame != NULL ) )
            theApp.m_pCreateGame->RemovePlayer( pPlyr );
        break;
    }

    case CNetCmd::veh_comp_loc: {
        CMsgVehCompLoc*     pMsg  = (CMsgVehCompLoc*)pCmd;
        CMsgVehCompLocElem* pElem = &( pMsg->m_aMVCLE[0] );
        for ( int iInd = 0; iInd < pMsg->m_iNumMsgs; iInd++ )
        {
            CVehicle* pVeh = theVehicleMap.GetVehicle( pElem->m_dwID );
            if ( ( pVeh != NULL ) && ( !( pVeh->GetFlags( ) & CUnit::dying ) ) )
            {
                CMsgVehLoc msg( pElem );
                VehLoc( &msg );
            }
            pElem++;
        }
        break;
    }

    case CNetCmd::comp_unit_damage: {
        CMsgCompUnitDamage*     pMsg  = (CMsgCompUnitDamage*)pCmd;
        CMsgCompUnitDamageElem* pElem = &( pMsg->m_aMCUDE[0] );
        for ( int iInd = 0; iInd < pMsg->m_iNumMsgs; iInd++ )
        {
            if ( GetUnit( pElem->m_dwID ) != NULL )
            {
                CMsgUnitDamage msg( pElem );
                UnitDamage( &msg );
            }
            pElem++;
        }
        break;
    }

    case CNetCmd::comp_unit_set_damage: {
        CMsgCompUnitSetDamage*  pMsg  = (CMsgCompUnitSetDamage*)pCmd;
        CMsgCompUnitDamageElem* pElem = &( pMsg->m_aMCUDE[0] );
        for ( int iInd = 0; iInd < pMsg->m_iNumMsgs; iInd++ )
        {
            if ( GetUnit( pElem->m_dwID ) != NULL )
            {
                CMsgUnitSetDamage msg( pElem );
                UnitSetDamage( &msg );
            }
            pElem++;
        }
        break;
    }

    case CNetCmd::pause_messages: {
        CMsgPauseMsg* pMsg = (CMsgPauseMsg*)pCmd;
        // if not the server - just do as it says
        if ( !theGame.AmServer( ) )
        {
            if ( theApp.m_pLogFile != NULL )
            {
                SYSTEMTIME st;
                char       sBuf[200];
                GetLocalTime( &st );
                sprintf( sBuf, "Pause: %s at %d:%d", pMsg->m_bPause ? "On" : "Off", st.wMinute, st.wSecond );
                theApp.Log( sBuf );
            }

            SetMessagesPaused(pMsg->m_bPause);
            break;
        }

        CPlayer* pPlr = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );
        // if pausing us, set that player to paused and pause
        if ( pMsg->m_bPause )
        {
            if ( theApp.m_pLogFile != NULL )
            {
                SYSTEMTIME st;
                char       sBuf[200];
                GetLocalTime( &st );
                sprintf( sBuf, "Pause: On at %d:%d", st.wMinute, st.wSecond );
                theApp.Log( sBuf );
            }
            pPlr->m_bPauseMsgs = TRUE;
            SetMessagesPaused(TRUE);
            break;
        }

        // mark that player as un-paused and then check all to see if can restart
        pPlr->m_bPauseMsgs = FALSE;
        POSITION pos;
        for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
        {
            CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
            // if any player says pause - we pause
            if ( pPlr->m_bPauseMsgs )
            {
                // NOT a fault (was TRAP): with several clients, pause windows overlap
                // routinely — e.g. game start, where every joiner pauses while it builds
                // the world and unpauses as it finishes. First unpause arriving while a
                // slower client is still paused lands here by design: stay paused and
                // wait for the rest. (The TRAP froze the host in the debugger mid-start
                // of the first 3-client MP game, 2026-07-01.)
                if ( theApp.m_pLogFile != NULL )
                    theApp.Log( "Pause: staying paused, another player still paused" );
                return;
            }
        }

        // everyone says ok
        if ( theApp.m_pLogFile != NULL )
        {
            SYSTEMTIME st;
            char       sBuf[200];
            GetLocalTime( &st );
            sprintf( sBuf, "Pause: Off at %d:%d", st.wMinute, st.wSecond );
            theApp.Log( sBuf );
        }
        SetMessagesPaused(FALSE);
        break;
    }

    case CNetCmd::ai_gpf_takeover: {
        TRAP( );
        CNetAiGpf* pMsg = (CNetAiGpf*)pCmd;
        CPlayer*   pPlr = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );
        theGame.AiTakeOverPlayer( pPlr, TRUE, FALSE );
        break;
    }

    case CNetCmd::need_save_info:
        NeedSaveInfo( (CNetNeedSaveInfo*)pCmd );
        break;

    // update population, food, gas, research on
    case CNetCmd::save_info: {
        CNetSaveInfo* pMsg = (CNetSaveInfo*)pCmd;
        CPlayer*      pPlr = theGame._GetPlayerByPlyr( pMsg->m_iPlyrNum );
        if ( ( pPlr != NULL ) && ( !pPlr->IsLocal( ) ) )
            pPlr->UpdateRemote( pMsg );
        break;
    }

    // update research status
    case CNetCmd::research_disc: {
        CNetRsrchDisc* pMsg = (CNetRsrchDisc*)pCmd;
        CPlayer*       pPlr = theGame._GetPlayerByPlyr( pMsg->m_iPlyrNum );
        if ( pPlr == NULL )
            break;
        // out-of-range topic: drop before ANY GetRsrch/ElementAt deref -
        // unfixed twin of the set_rsrch v48 OOB (same pooled-buffer garbage)
        if ( pMsg->m_iRsrch < 0 || pMsg->m_iRsrch >= pPlr->GetRsrchSize( ) )
        {
#if EN_AI_PROBES_ECON && defined(_WIN32)
            OutputDebugStringA( "research_disc: out-of-range topic dropped\n" );
#endif
            break;
        }
        if ( !pPlr->IsLocal( ) )
        {
            TRAP( );
            pPlr->UpdateRacialAttributes( pMsg->m_iRsrch );
            ( pPlr->GetRsrch( pMsg->m_iRsrch ) ).m_bDiscovered = TRUE;
        }
        // cheat - give it to the AI (wait, does this mean ai gets all tecks anybody researches??)
        if ( !pPlr->IsAI( ) )
            for ( POSITION pos = m_lstAi.GetHeadPosition( ); pos != NULL; )
            {
                CPlayer*      pPlrAi = m_lstAi.GetNext( pos );
                CRsrchStatus* pRs    = &( pPlrAi->GetRsrch( pMsg->m_iRsrch ) );
                if ( !pRs->m_bDiscovered )
                {
                    CRsrchItem* pRi = &theRsrch.ElementAt( pMsg->m_iRsrch );
                    pRs->m_iPtsDiscovered += ( pRi->m_iPtsRequired * theGame.m_iAi ) / 2;
                }
            }
        break;
    }

    // Edicts v1: replicate a civ-wide edict toggle. The originating client already applied
    // it locally (UI), so apply only on the OTHER clients for that player → all converge.
    case CNetCmd::edict_toggle: {
        CNetEdictToggle* pMsg = (CNetEdictToggle*)pCmd;
        CPlayer*         pPlr = theGame._GetPlayerByPlyr( pMsg->m_iPlyrNum );
        if ( pPlr == NULL )
            break;
        if ( !pPlr->IsLocal( ) )
            pPlr->ToggleEdict( pMsg->m_iEdict, pMsg->m_bOn != 0 );
        // Optional MP-sync runtime-test log (env EN_EDICT_LOG=1, default off → no
        // impact). Confirms a remote client RECEIVED + APPLIED a peer's civ-edict
        // toggle (the CNetEdictToggle wire path) and shows the resulting per-player
        // edict bitmask — the edict-MP-sync gate (per linux2's recipe).
        { static int el=-1; if(el<0) el=getenv("EN_EDICT_LOG")?1:0;
          if(el) fprintf(stderr,"[edict-mp] RX edict_toggle plyr=%d edict=%d on=%d -> %s m_dwEdicts=0x%lx\n",
                         (int)pMsg->m_iPlyrNum,(int)pMsg->m_iEdict,(int)pMsg->m_bOn,
                         pPlr->IsLocal()?"local-skip":"APPLIED",(unsigned long)pPlr->GetEdicts()); }
        break;
    }

#ifdef _DEBUG

    // we couldn't handle the message!!
    default:
        // WE OFTEN GET A "221" msgKind, not sure where it's from..
        // and every time it comes, as the game gets later, it seems to be more of them
        ASSERT( FALSE ); 
        break;
#endif
    }
}

CNetPublish* CNetPublish::Alloc( CCreateBase* pCm )
{

    ASSERT_VALID( pCm );

    int iLen = sizeof( CNetPublish ) + 2 + (int)pCm->m_sName.length( ) + (int)pCm->m_sPw.length( ) +
               (int)pCm->m_sGameName.length( ) + (int)pCm->m_sGameDesc.length( );
    CNetPublish* pMsg     = (CNetPublish*)new char[__max( 516, iLen )];
    pMsg->m_iLen          = iLen;
    pMsg->m_iNumOpponents = pCm->m_iNumAi;
    pMsg->m_iAIlevel      = pCm->m_iAi;
    pMsg->m_iWorldSize    = pCm->m_iSize;
    pMsg->m_iPos          = pCm->m_iPos;
    pMsg->m_iNumPlayers   = pCm->m_iNumPlayers;

    pMsg->m_iGameID     = TLP_GAME_ID;
    pMsg->m_cVerMajor   = VER_MAJOR;
    pMsg->m_cVerMinor   = VER_MINOR;
    pMsg->m_cVerRelease = VER_RELEASE;

    pMsg->m_cFlags = 0;
#ifdef _DEBUG
    pMsg->m_cFlags |= fdebug;
#endif
#ifdef _CHEAT
    pMsg->m_cFlags |= fcheat;
#endif

    strcpy( pMsg->m_sPlyrName, pCm->m_sName.c_str() );
    char* pBuf = pMsg->m_sPlyrName + strlen( pMsg->m_sPlyrName ) + 1;
    strcpy( pBuf, pCm->m_sPw.c_str() );
    pBuf = pBuf + strlen( pBuf ) + 1;
    strcpy( pBuf, pCm->m_sGameName.c_str() );
    pBuf = pBuf + strlen( pBuf ) + 1;
    strcpy( pBuf, pCm->m_sGameDesc.c_str() );

    return ( pMsg );
}

CNetPublish* CNetPublish::Alloc( CGame* pGame )
{

    std::string sName;
    if ( pGame->HaveHP( ) )
        sName = pGame->GetMe( )->GetName( );

    int iLen = sizeof( CNetPublish ) + 2 + (int)sName.size( ) + (int)pGame->m_sPwJoin.length( ) +
               (int)pGame->m_sGameName.length( ) + (int)pGame->m_sGameDesc.length( );
    CNetPublish* pMsg     = (CNetPublish*)new char[__max( 516, iLen )];
    pMsg->m_iLen          = iLen;
    pMsg->m_iNumOpponents = pGame->GetAi( ).GetCount( );
    pMsg->m_iAIlevel      = pGame->m_iAi;
    pMsg->m_iWorldSize    = pGame->m_iSize;
    pMsg->m_iPos          = pGame->m_iPos;
    pMsg->m_iNumPlayers   = pGame->GetAll( ).GetCount( );

    pMsg->m_iGameID     = TLP_GAME_ID;
    pMsg->m_cVerMajor   = VER_MAJOR;
    pMsg->m_cVerMinor   = VER_MINOR;
    pMsg->m_cVerRelease = VER_RELEASE;

    pMsg->m_cFlags = 0;
#ifdef _DEBUG
    pMsg->m_cFlags |= fdebug;
#endif
#ifdef _CHEAT
    pMsg->m_cFlags |= fcheat;
#endif

    strcpy( pMsg->m_sPlyrName, sName.c_str( ) );
    char* pBuf = pMsg->m_sPlyrName + strlen( pMsg->m_sPlyrName ) + 1;
    strcpy( pBuf, pGame->m_sPwJoin.c_str() );
    pBuf = pBuf + strlen( pBuf ) + 1;
    strcpy( pBuf, pGame->m_sGameName.c_str() );
    pBuf = pBuf + strlen( pBuf ) + 1;
    strcpy( pBuf, pGame->m_sGameDesc.c_str() );

    return ( pMsg );
}

CNetJoin* CNetJoin::Alloc( CPlayer const* pPlyr, BOOL bSrvr )
{

    ASSERT_VALID( pPlyr );

    int       iLen   = sizeof( CNetJoin ) + 2 + strlen( pPlyr->GetName( ) );
    CNetJoin* pMsg   = (CNetJoin*)new char[__max( 516, iLen )];
    pMsg->m_iLen     = iLen;
    pMsg->m_iPlyrNum = pPlyr->GetPlyrNum( );
    pMsg->m_bServer  = bSrvr;
    strcpy( pMsg->m_sName, pPlyr->GetName( ) );

    return ( pMsg );
}
