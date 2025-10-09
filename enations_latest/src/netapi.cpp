//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// netapi.cpp : implementation file
//

#include "netapi.h"

#include "ai.h"
#include "area.h"
#include "bridge.h"
#include "building.inl"
#include "chproute.hpp"
#include "creatmul.inl"
#include "dlgmsg.h"
#include "event.h"
#include "help.h"
#include "join.h"
#include "lastplnt.h"
#include "netcmd.h"
#include "player.h"
#include "relation.h"
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

CString CNetApi::GetIServeAddress( )
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
            CDlgMsg dlg;
            dlg.MsgBox( IDS_ERROR_MULT_PROT_WARNING, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
                        "MultProtWarning" );
        }
    }

    if ( m_vpHdl != NULL )
        Close( FALSE );

    if ( ( m_vpHdl = vpStartup( VPAPI_VERSION, &tlpGUID, iMaxNameLen, iMaxNameLen, iProtocol, pPrtcl ) ) == NULL )
    {
        Close( FALSE );
        AfxMessageBox( IDS_VPSTARTUP_FAILED, MB_OK | MB_ICONSTOP );
        return ( TRUE );
    }

    // create a session
    m_hWnd = hWnd;
    if ( ( m_vpSession = vpCreateSession( m_vpHdl, hWnd, pName, 0, pData ) ) == NULL )
    {
        Close( FALSE );
        AfxMessageBox( IDS_VPCREATE_FAILED, MB_OK | MB_ICONSTOP );
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
            CDlgMsg dlg;
            dlg.MsgBox( IDS_ERROR_MULT_PROT_WARNING, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
                        "MultProtWarning" );
        }
    }

    if ( m_vpHdl != NULL )
        Close( FALSE );

    if ( ( m_vpHdl = vpStartup( VPAPI_VERSION, &tlpGUID, iMaxNameLen, iMaxNameLen, iProtocol, pPrtcl ) ) == NULL )
    {
        Close( FALSE );
        AfxMessageBox( IDS_VPSTARTUP_FAILED, MB_OK | MB_ICONSTOP );
        return ( TRUE );
    }

    // create a session
    m_hWnd = hWnd;
    if ( !vpEnumSessions( m_vpHdl, hWnd, TRUE, NULL ) )
    {
        Close( FALSE );
        AfxMessageBox( IDS_VPENUM_FAILED, MB_OK | MB_ICONSTOP );
        return ( TRUE );
    }

    m_iMode = opened;
    m_iType = client;
    return ( FALSE );
}

CString CNetApi::GetAddress( ) const
{

    VPNETADDRESS addr;
    vpGetAddress( m_vpHdl, &addr );

    CString sRtn;
    vpGetAddressString( m_vpHdl, &addr, sRtn.GetBuffer( 256 ), 258 );
    sRtn.ReleaseBuffer( -1 );

    return ( sRtn );
}

BOOL CNetApi::Join( LPCVPSESSIONID id, CNetJoin const* pJn )
{

    ASSERT( m_iType == client );

    m_iMode = joined;
    if ( ( m_vpSession = vpJoinSession( m_vpHdl, m_hWnd, id, (LPCSTR)pJn, 0, (LPVOID)TRUE ) ) == NULL )
    {
        TRAP( );
        AfxMessageBox( IDS_VPJOIN_FAILED, MB_OK | MB_ICONSTOP );
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
        AfxMessageBox( IDS_VPJOIN_FAILED, MB_OK | MB_ICONSTOP );
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

    AfxMessageBox( IDS_VPSEND_FAILED, MB_OK | MB_ICONSTOP );
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

    AfxMessageBox( IDS_VPSEND_FAILED, MB_OK | MB_ICONSTOP );
    SaveExistingGame( );
    theApp.CloseWorld( );
    ThrowError( ERR_TLP_QUIT );
    return ( TRUE );
}

static void OnMsgLeave( VPPLAYERID id )
{

    // will be NULL if we deleted it
    CPlayer* pPlr = theGame._GetPlayer( id );
    if ( pPlr == NULL )
        return;

    ASSERT_VALID( pPlr );
    ASSERT( ( !theGame.HaveHP( ) ) || ( id != theGame.GetMe( )->GetPlyrNum( ) ) );

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
            CString sMsg;
            sMsg.LoadString( IDS_MSG_NET_GOODBYE );
            csPrintf( &sMsg, (const char*)pPlr->GetName( ) );
            CDlgModelessMsg* pDlg = new CDlgModelessMsg( );
            pDlg->Create( sMsg );

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
        CString sMsg;
        sMsg.LoadString( IDS_MSG_NET_GOODBYE );
        csPrintf( &sMsg, (const char*)pPlr->GetName( ) );

        // have the AI take over
        if ( theGame.AmServer( ) )
        {
            // if we are still installing just set it
            pPlr->SetAI( TRUE );
            pPlr->SetLocal( TRUE );
            if ( theGame.GetState( ) < CGame::AI_done )
            {
                pPlr->SetState( CPlayer::replace );

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

        if ( theApp.m_wndBar.m_hWnd != NULL )
            theApp.m_wndBar.SetStatusText( 0, sMsg );
        if ( ( theGame._GetMe( ) != NULL ) && ( pPlr != theGame.GetMe( ) ) )
            if ( !pPlr->m_bMsgDead )
            {
                pPlr->m_bMsgDead      = TRUE;
                CDlgModelessMsg* pDlg = new CDlgModelessMsg( );
                pDlg->Create( sMsg );
            }
        break;
    }
    }
}

// the game is over for us (for everyone if we're the server)
static void OnMsgSessionClose( )
{

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
            CString sMsg;
            sMsg.LoadString( IDS_SAVE_CLOSE );
            csPrintf( &sMsg, (const char*)theGame.m_sGameName );

            if ( AfxMessageBox( sMsg, MB_YESNO | MB_ICONQUESTION ) == IDYES )
                theGame.SaveGame( NULL );
        }

    // We have to tell the player (this is bad news) and
    // then we go back to the main screen
    if ( !bTold )
    {
        CString sMsg;
        sMsg.LoadString( IDS_JOIN_UNJOIN );
        CPlayer* pPlyr = theGame.GetServer( );
        if ( pPlyr != NULL )
            csPrintf( &sMsg, (const char*)theGame.GetServer( )->GetName( ) );
        else
        {
            CString sTxt;
            sTxt.LoadString( IDS_UNKNOWN );
            csPrintf( &sMsg, (const char*)sTxt );
        }
        AfxMessageBox( sMsg, MB_OK | MB_TASKMODAL );
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
            CString sMsg;
            sMsg.LoadString( IDS_MSG_JOIN_FAILED );
            CString sNum = IntToCString( bErr );
            csPrintf( &sMsg, (char const*)theGame.GetServer( )->GetName( ), (char const*)sNum );
            AfxMessageBox( sMsg, MB_OK | MB_ICONSTOP );
            return;
        }

        theGame.GetMe( )->SetNetNum( pPi->playerId );
        pPlyr = theGame.GetMe( );

        // enable the chat window
        theApp.GetDlgChat( )->EnableWindow( TRUE );
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

void CGame::AddToQueue( CNetCmd const* pCmd, int iLen )
{
#ifdef LOGGINGON
    if ( iLen == 0 )
    {
        OutputDebugStringA( "0 length pMsg in AddToQueue?!\n" );
    }
    CString str;

    str.Format( "AddingToQueue type %d, %d, %d", pCmd->GetType( ), iLen, sizeof( pCmd ) );
    OutputDebugStringA( str );
    OutputDebugStringA( "\n" );
#endif

    ASSERT_VALID( this );

#ifdef _DEBUG
    pCmd->AssertMsgValid( );
#endif
    // can't do - previous messages may need to be processed first	ASSERT_CMD (pCmd);

    // check if the command id is too high:
    int msgType = pCmd->GetType( );
    if ( msgType < 0 || msgType >= CNetCmd::last_message )
    {
        TRACE( "ProcessMessage: Invalid message type %d (corrupted message?)\n", msgType );
        return;  // Skip corrupted messages instead of asserting
    }

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

    memcpy( pBuf, pCmd, iLen ); 

    if ( iLen <= MSG_POOL_SIZE )
        ( (CNetCmd*)pBuf )->m_bMemPool = 1;
    else
        ( (CNetCmd*)pBuf )->m_bMemPool = 0;

    EnterCriticalSection( &cs );

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
            AfxMessageBox( IDS_JOIN_FILE_ERROR, MB_OK | MB_ICONSTOP );

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
                CDlgCreateStatus* pDlg = theApp.m_pCreateGame->GetDlgStatus( );
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
        ::OnMsgJoin( pVpMsg->u.playerInfo, (BOOL)pVpMsg->userData, VPGETERRORCODE( pVpMsg->notificationCode ) );
        break;

    case VP_LEAVE:
        ::OnMsgLeave( pVpMsg->u.playerInfo->playerId );
        break;

    case VP_READDATA: {
        if ( pVpMsg->userData == NULL )
            theGame.AddToQueue( (CNetCmd*)( ( (char*)pVpMsg->u.data ) - sizeof( VPMsgHdr ) ),
                                pVpMsg->dataLen + sizeof( VPMsgHdr ) );
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
        ASSERT( FALSE );
        return;
    }
    ASSERT_VALID( pPlr );
    pPlr->m_InitData = pMsg->m_InitData;
    pPlr->SetState( CPlayer::ready );
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

static void CmdPlyrJoin( CNetPlyrJoin* pMsg )
{

    if ( ( theApp.m_pCreateGame == NULL ) || ( theGame.AmServer( ) ) ||
         ( theApp.m_pCreateGame->m_iTyp != CCreateBase::load_join ) ||
         ( ( (CJoinMulti*)theApp.m_pCreateGame )->m_dlgPickPlayer.m_hWnd == NULL ) )
    {
        TRAP( );
        ASSERT( FALSE );
        return;
    }

    CNetPlyrJoin* pData = CNetPlyrJoin::Alloc( pMsg );

    int iInd = ( (CJoinMulti*)theApp.m_pCreateGame )->m_dlgPickPlayer.m_lstRace.AddString( pData->m_sName );
    ( (CJoinMulti*)theApp.m_pCreateGame )->m_dlgPickPlayer.m_lstRace.SetItemDataPtr( iInd, pData );
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
            CFile fil( theGame.m_sFileName, CFile::modeRead | CFile::shareExclusive | CFile::typeBinary );
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
    CDlgCreateStatus* pDlg = theApp.m_pCreateGame->GetDlgStatus( );
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

    ( (CJoinMulti*)theApp.m_pCreateGame )->m_dlgPickPlayer.m_dlgWait.DestroyWindow( );
    ( (CJoinMulti*)theApp.m_pCreateGame )->m_dlgPickPlayer.EnableWindow( TRUE );
}

static void CmdPlayerTaken( CNetSelectPlyr* pMsg )
{

    // we have a pPlyr for the person who joined (iNetNum) & for the
    // player in the loaded game (iPlyrNum). We keep the joined player.
    if ( ( theApp.m_pCreateGame != NULL ) || ( NULL == ( (CJoinMulti*)theApp.m_pCreateGame )->m_dlgPickPlayer ) ||
         ( ( (CJoinMulti*)theApp.m_pCreateGame )->m_dlgPickPlayer.m_hWnd == NULL ) )
        return;
    TRAP( );

    // update the taken player
    CListBox* pLb  = &( ( (CJoinMulti*)theApp.m_pCreateGame )->m_dlgPickPlayer.m_lstRace );
    int       iMax = pLb->GetCount( );
    for ( int iOn = 0; iOn < iMax; iOn++ )
    {
        CNetPlyrJoin* pData = (CNetPlyrJoin*)pLb->GetItemDataPtr( iOn );
        if ( pData->m_iPlyrNum == pMsg->m_iPlyrNum )
        {
            TRAP( );
            pData->m_bAvail = FALSE;
            ( (CJoinMulti*)theApp.m_pCreateGame )->m_dlgPickPlayer.UpdateBtns( );
            break;
        }
    }
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
    CDlgCreateStatus* pDlg = theApp.m_pCreateGame->GetDlgStatus( );
    if ( pDlg != NULL )
        pDlg->SetMsg( IDS_JOIN_LOAD_FILE );

    // tell server to send
    theGame.PostToServer( &msg, sizeof( msg ) );
}

static void CmdYouAre( int iPlyrNum, int iSrvrNum )
{

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
}

static void CmdStart( CNetStart* pStrt )
{

    try
    {
        // create the world
        theGame.IncTry( );
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
        AfxMessageBox( IDS_BAD_PLAYER_NUM, MB_OK | MB_ICONSTOP );
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
        ASSERT( !theGame.AmServer( ) );
        theGame.Close( );
        theNet.Close( TRUE );
        AfxMessageBox( IDS_RAND_MISMATCH, MB_OK | MB_ICONSTOP );
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

static void BuildRoad( CMsgBuildRoad* pMsg )
{

    ASSERT( theGame.AmServer( ) );
    ASSERT_CMD( pMsg );

    CHex* pHex = theMap._GetHex( pMsg->m_hexBuild );

    if ( ( !pHex->CanRoad( ) ) || ( pHex->GetUnits( ) & CHex::bldg ) )
    {
        pMsg->ToErr( );
        theGame.PostToClient( pMsg->m_iPlyrNum, pMsg, sizeof( *pMsg ) );
        return;
    }

    // and tell the clients
    pMsg->ToNew( );
    theGame.PostToAll( pMsg, sizeof( *pMsg ) );
}

class CBBData
{
  public:
    CBBData( ) {}

    int       m_iAlt;
    int       m_iLen;
    BOOL      m_bOK;
    CHexCoord m_hexOn;
    CHexCoord m_hexEnd;
};

static int fnEnumBaseBridge( CHex* pHex, CHexCoord hex, void* pData )
{

    CBBData* pBbData = (CBBData*)pData;

    // if already there ok
    if ( ( ( pHex->GetAlt( ) + 2 ) & ~0x03 ) == pBbData->m_iAlt )
        return FALSE;

    // if this is closer to end it's not relevant
    int xDif = abs( CHexCoord::Diff( pBbData->m_hexEnd.X( ) - hex.X( ) ) );
    int yDif = abs( CHexCoord::Diff( pBbData->m_hexEnd.Y( ) - hex.Y( ) ) );
    if ( xDif + yDif <= pBbData->m_iLen )
        return FALSE;

    // if there is a building or bridge here we can't change it
    if ( pHex->GetUnits( ) & ( CHex::bldg | CHex::bridge ) )
    {
        pBbData->m_bOK = FALSE;
        return TRUE;
    }

    return FALSE;
}

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
        pMsg->ToErr( );
        theGame.PostToClient( pMsg->m_iPlyrNum, pMsg, sizeof( *pMsg ) );
        return;
    }

    int   iLen = 0;
    CHex* pHex;
    goto StartIt;

    while ( _hexOn != pMsg->m_hexEnd )
    {
        _hexOn.X( ) += xAdd;
        _hexOn.Y( ) += yAdd;
        _hexOn.Wrap( );
    StartIt:
        pHex = theMap._GetHex( _hexOn );
        if ( pHex->IsWater( ) )
            iLen++;

        if ( ( iLen > MAX_SPAN ) || ( pHex->GetUnits( ) & ( CHex::bldg | CHex::bridge ) ) )
        {
            ASSERT( iLen <= MAX_SPAN );
        EndIt:
            pMsg->ToErr( );
            theGame.PostToClient( pMsg->m_iPlyrNum, pMsg, sizeof( *pMsg ) );
            return;
        }
    }

    // set the altitude - see if we can do it
    pMsg->m_iAlt =
        ( ( theMap.GetHex( pMsg->m_hexStart )->GetAlt( ) + theMap.GetHex( pMsg->m_hexEnd )->GetAlt( ) + 2 ) / 2 ) &
        ~0x03;
    int xDif = CHexCoord::Diff( pMsg->m_hexEnd.X( ) - pMsg->m_hexStart.X( ) );
    int yDif = CHexCoord::Diff( pMsg->m_hexEnd.Y( ) - pMsg->m_hexStart.Y( ) );

    // check start hex
    CHexCoord _hexUL( pMsg->m_hexStart.X( ) - 1, pMsg->m_hexStart.Y( ) - 1 );
    _hexUL.Wrap( );
    CBBData bbData;
    bbData.m_iAlt   = pMsg->m_iAlt;
    bbData.m_hexOn  = pMsg->m_hexStart;
    bbData.m_hexEnd = pMsg->m_hexEnd;
    bbData.m_iLen   = abs( xDif ) + abs( yDif );
    bbData.m_bOK    = TRUE;
    theMap._EnumHexes( _hexUL, 3, 3, fnEnumBaseBridge, &bbData );
    if ( !bbData.m_bOK )
        goto EndIt;

    // check end hex
    _hexUL = CHexCoord( pMsg->m_hexEnd.X( ) - 1, pMsg->m_hexEnd.Y( ) - 1 );
    _hexUL.Wrap( );
    bbData.m_hexOn  = pMsg->m_hexEnd;
    bbData.m_hexEnd = pMsg->m_hexStart;
    theMap._EnumHexes( _hexUL, 3, 3, fnEnumBaseBridge, &bbData );
    if ( !bbData.m_bOK )
    {
        TRAP( );
        goto EndIt;
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
        theGame.Event( EVENT_ROAD_HALTED, EVENT_WARN, pVeh );
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
        theGame.Event( EVENT_ROAD_HALTED, EVENT_WARN, pVeh );
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
        return;
    ASSERT_CMD( pMsg );

    // AI
    if ( pMsg->m_iMode == _CMsgRoad::one_hex )
    {
        TRAP( );  // eric - if it builds a bridge and then stops - tell me to delete this
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
        theBridgeHex.GetBridge( pMsg->m_hexBuild )->GetParent( )->BridgeBuilt( );
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

    pVeh->SetEvent( CVehicle::none );
    if ( pMsg->m_iSub == CVehicle::sub )
        pVeh->SetDest( pMsg->m_sub );
    else
        pVeh->SetDestAndMode( pMsg->m_hex, (CVehicle::VEH_POS)pMsg->m_iSub );

    ASSERT( ( pVeh->m_hexDest == pMsg->m_hex ) ||
            ( ( theBuildingHex.GetBuilding( pVeh->m_hexDest ) != NULL ) &&
              ( theBuildingHex.GetBuilding( pVeh->m_hexDest ) == theBuildingHex.GetBuilding( pMsg->m_hex ) ) ) );

#ifdef _DEBUG
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
         ( pVehCarrier->m_iCargoSize + iAdd > pVehCarrier->GetData( )->GetPeopleCarry( ) ) )
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
            ( pVehCarrier->m_iCargoSize <= pVehCarrier->GetData( )->GetPeopleCarry( ) ) );

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
                CDlgRelations::NewRelations( pPlyrAtk, RELATIONS_WAR );
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
        if ( theApp.m_pdlgFile != NULL )
            theApp.m_pdlgFile->SetSpeed( );
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
            if ( theApp.m_pdlgFile != NULL )
                theApp.m_pdlgFile->SetSpeed( );
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

    CString str;
   // str.Format( "ProcessMessage (PM1) type %d\n", pCmd->GetType( ) );
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
        CDlgChatAll* pDlg = theApp.GetDlgChat( );
        if ( pDlg != NULL )
            pDlg->NewMessage( ( (CNetChat*)pCmd )->m_sMsg );
        break;
    }

    case CNetCmd::cmd_to_ai: {
        CPlayer* pPlr = theGame.GetPlayer( ( (CNetToAi*)pCmd )->m_iPlyrNum );
        if ( pPlr == NULL )
        {
            TRAP( );
            break;
        }

        // if it was the server fix it
        if ( pPlr == theGame.GetServer( ) )
        {
            TRAP( );
            theGame._SetServer( NULL );
        }

        // if it was the server player we have to handle the LEAVE stuff here
        CString sMsg;
        BOOL    bMsg;
        if ( ( !theGame.AmServer( ) ) && ( pPlr->GetNetNum( ) != 0 ) )
        {
            sMsg.LoadString( IDS_MSG_NET_GOODBYE );
            csPrintf( &sMsg, (const char*)pPlr->GetName( ) );

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

        // update relations
        if ( ( theApp.m_pdlgRelations != NULL ) && ( theApp.m_pdlgRelations->m_hWnd != NULL ) )
            theApp.m_pdlgRelations->ChangedIfAi( );

        // tell the player
        if ( bMsg )
        {
            if ( theApp.m_wndBar.m_hWnd != NULL )
                theApp.m_wndBar.SetStatusText( 0, sMsg );
            if ( theGame.GetState( ) == CGame::play )
            {
                CDlgModelessMsg* pDlg = new CDlgModelessMsg( );
                pDlg->Create( sMsg );
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

        // update relations
        if ( ( theApp.m_pdlgRelations != NULL ) && ( theApp.m_pdlgRelations->m_hWnd != NULL ) )
            theApp.m_pdlgRelations->ChangedIfAi( );
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
        theApp.m_wndChat.IncomingMessage( (CMsgIPC*)pCmd );
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
            CString sMsg;
            sMsg.LoadString( IDS_EVENT_DEAD );
            csPrintf( &sMsg, pPlr->GetName( ) );
            CDlgModelessMsg* pDlg = new CDlgModelessMsg( );
            pDlg->Create( sMsg );
        }
        break;
    }

    case CNetCmd::set_rsrch: {

        CMsgRsrch* pMsg = (CMsgRsrch*)pCmd;
        CPlayer*   pPlr = theGame.GetPlayerByPlyr( pMsg->m_iPlyrNum );
        if (!pPlr)
        {
            
            OutputDebugStringA("set_rsrch: player not found\n");
            break;
        }
        int        iOn  = pPlr->GetRsrchItem( );
        if ( iOn > 0 )
        {
            CRsrchStatus* pRs = &( pPlr->GetRsrch( pPlr->GetRsrchItem( ) ) );
            pRs->m_iPtsDiscovered -= pRs->m_iPtsDiscovered / 10;
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
                TRAP( );
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

    int iLen = sizeof( CNetPublish ) + 2 + pCm->m_sName.GetLength( ) + pCm->m_sPw.GetLength( ) +
               pCm->m_sGameName.GetLength( ) + pCm->m_sGameDesc.GetLength( );
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

    strcpy( pMsg->m_sPlyrName, pCm->m_sName );
    char* pBuf = pMsg->m_sPlyrName + strlen( pMsg->m_sPlyrName ) + 1;
    strcpy( pBuf, pCm->m_sPw );
    pBuf = pBuf + strlen( pBuf ) + 1;
    strcpy( pBuf, pCm->m_sGameName );
    pBuf = pBuf + strlen( pBuf ) + 1;
    strcpy( pBuf, pCm->m_sGameDesc );

    return ( pMsg );
}

CNetPublish* CNetPublish::Alloc( CGame* pGame )
{

    CString sName;
    if ( pGame->HaveHP( ) )
        sName = pGame->GetMe( )->GetName( );
    else
        sName = "";

    int iLen = sizeof( CNetPublish ) + 2 + sName.GetLength( ) + pGame->m_sPwJoin.GetLength( ) +
               pGame->m_sGameName.GetLength( ) + pGame->m_sGameDesc.GetLength( );
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

    strcpy( pMsg->m_sPlyrName, sName );
    char* pBuf = pMsg->m_sPlyrName + strlen( pMsg->m_sPlyrName ) + 1;
    strcpy( pBuf, pGame->m_sPwJoin );
    pBuf = pBuf + strlen( pBuf ) + 1;
    strcpy( pBuf, pGame->m_sGameName );
    pBuf = pBuf + strlen( pBuf ) + 1;
    strcpy( pBuf, pGame->m_sGameDesc );

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
