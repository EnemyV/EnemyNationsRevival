#include "vpengine.h"
#ifdef _WIN32
#include "mmsystem.h"   // timeGetTime etc. — Windows multimedia; win32_compat provides these on POSIX
#endif
#include "smap.h"
#include "stdafx.h"
#include "tdlog.h"
#include "vpparam.h"
#include "vpnatcand.h"
#include <stdarg.h>
#include <time.h>

IMPLEMENT_CLASSNAME( CVpSession );
IMPLEMENT_CLASSNAME( CLocalSession );
IMPLEMENT_CLASSNAME( CRemoteSession );

static void LogV( CTDLogger* log, const char* fmt,
                  DWORD p1 = 0, DWORD p2 = 0, DWORD p3 = 0, DWORD p4 = 0 ) {
    char logBuf[128];

    if ( log ) {
        wsprintf( logBuf, fmt, p1, p2, p3, p4 );
        log->Log( logBuf );
    }
}

// Render a VPNETADDRESS/VPSESSIONID into a printable "ip:streamPort/dgPort" for
// join-path diagnostics. The 28-byte machineAddress overlays tcpaddress_s:
// 4-byte IP then 2-byte stream port then 2-byte datagram port, all in NETWORK
// byte order. Read byte-wise so it's self-contained (no socket headers) and
// endian-correct on win/mac/linux alike. Used only behind Log() (logger off in
// normal play => zero ship impact) to nail the iserve/TCP-discovered join leg:
// is the addr B dials the host's SESSION (:33561) or the reg server (:1707)?
static void FormatVpAddr( char* out, int outSz, LPCVPNETADDRESS a ) {
    const unsigned char* b = (const unsigned char*)a->machineAddress;
    unsigned sPort = (unsigned)( ( b[4] << 8 ) | b[5] );
    unsigned dPort = (unsigned)( ( b[6] << 8 ) | b[7] );
    snprintf( out, outSz, "%u.%u.%u.%u:%u/%u(stream/dg)", b[0], b[1], b[2], b[3], sPort, dPort );
}

static WSList* rwsPool = 0;

// Optional iserve transaction log (env EN_ISERVE_LOG=1). Lets the headless
// registration server show it actually fielded a host registration / client
// query — proving cross-subnet discovery went THROUGH iserve, not LAN broadcast
// (the client sends sEnumREQ to BOTH the server-lookup addr and broadcast, so
// only the server side can prove which served it). Cached so the env lookup is
// not paid per-message. Default off => zero impact on the game.
static int IserveLogOn() {
    static int on = -1;
    if ( on < 0 ) on = getenv( "EN_ISERVE_LOG" ) ? 1 : 0;
    return on;
}

// Join-path address diagnostics gate. Routes the two `[join-addr]` lines via
// getenv+fprintf(stderr) like the [iserve-host]/[nettrace] diags — because the
// session Log() logger is silent on a POSIX Release build (CVdmErrorLogger::Write
// only writes a gated logfile; its OutputDebugString fallback is NDEBUG-gated out
// in Release — two-node confirmed by linux1/linux2). Rides on EN_NETTRACE too so
// it surfaces in the Linux nodes' existing trace runs with no extra env. Cached;
// default off => zero ship impact.
static int JoinAddrLogOn() {
    static int on = -1;
    if ( on < 0 ) on = ( getenv( "EN_JOINADDR" ) || getenv( "EN_NETTRACE" ) ) ? 1 : 0;
    return on;
}

// NAT hole-punch gate. Gates the P1 rendezvous/probe machinery on game hosts
// and clients (iserve's stateless forward is always on — it only acts when a
// punch-enabled client asks). Default ON; kill switches: EN_NAT_PUNCH=0 env
// (wins when set) or [TCP] NatPunch=0 in vdmplay.ini.
// The ini switch is only honored when the profile layer proves it can return
// a default for an absent key: on macOS vpFetchInt returns 0 regardless of
// file content or defVal (mac 3-run evidence, board 2026-08-22), which made
// the old read report "explicitly disabled" on a healthy config. A broken
// reader falls back to default ON with env as the only kill switch.
static int NatPunchOn() {
    static int on = -1;
    if ( on < 0 ) {
        const char* e = getenv( "EN_NAT_PUNCH" );
        if ( e && *e )
            on = ( *e != '0' ) ? 1 : 0;
        else if ( vpFetchInt( "TCP", "EnIniProbeSentinel", 5 ) != 5 )
            on = 1;   // profile layer can't honor defaults - ini switch unusable
        else
            on = vpFetchInt( "TCP", "NatPunch", 1 ) ? 1 : 0;
    }
    return on;
}

// P0.1 candidate-dial gate. Default ON (inert until a session arrives with a
// stamped tail, i.e. until a stamping iserve is deployed); EN_NAT_CAND=0 is the
// kill switch back to the legacy always-dial-payload behavior.
static int NatCandOn() {
    static int on = -1;
    if ( on < 0 ) {
        const char* e = getenv( "EN_NAT_CAND" );
        on = ( e && *e == '0' ) ? 0 : 1;
    }
    return on;
}

// Host-relay gate (docs/plans/host-relay-spec.md). DEFAULT OFF for this first
// cycle: with it off nothing below is reachable and client-to-client unicast
// keeps today's direct-only behavior byte for byte. EN_HOST_RELAY env wins when
// set; otherwise [TCP] HostRelay in vdmplay.ini, but only when the profile layer
// proves it can return a default for an absent key — on macOS vpFetchInt returns
// 0 regardless of file content or defVal (the EnIniProbeSentinel lesson, see
// NatPunchOn above), and a broken reader must not read as "explicitly on/off".
static int HostRelayOn() {
    static int on = -1;
    if ( on < 0 ) {
        const char* e = getenv( "EN_HOST_RELAY" );
        if ( e && *e )
            on = ( *e != '0' ) ? 1 : 0;
        else if ( vpFetchInt( "TCP", "EnIniProbeSentinel", 5 ) != 5 )
            on = 0;   // profile layer can't honor defaults - ini switch unusable
        else
            on = vpFetchInt( "TCP", "HostRelay", 0 ) ? 1 : 0;
    }
    return on;
}

// [punch] log: always on while the punch machinery is active (the machinery is
// itself opt-in via EN_NAT_PUNCH), plus iserve's EN_ISERVE_LOG side.
static void PunchLog( const char* fmt, ... ) {
    va_list ap;
    va_start( ap, fmt );
    fprintf( stderr, "[punch] " );
    vfprintf( stderr, fmt, ap );
    fprintf( stderr, "\n" );
    va_end( ap );
}

// [relay] log: unconditional like PunchLog, because every line is downstream of
// HostRelayOn() — the machinery is itself opt-in. The engine sits on the other
// side of the vdmplay boundary from the game's EnMpDiagLog, so it cannot call
// it; this is the same fprintf(stderr) shape the other engine diags use.
static void RelayLog( const char* fmt, ... ) {
    va_list ap;
    va_start( ap, fmt );
    fprintf( stderr, "[relay] " );
    vfprintf( stderr, fmt, ap );
    fprintf( stderr, "\n" );
    va_end( ap );
}

DWORD vpMsgTime() {
    DWORD  msgTime;
#ifdef WIN32
    SYSTEMTIME st;
    GetLocalTime( &st );
    msgTime = ( st.wMinute << 16 ) | st.wSecond;
#else
    time_t t = time( NULL );
    struct tm tms = *localtime( &t );

    msgTime = ( (DWORD)tms.tm_min << 16 ) | tms.tm_sec;
#endif

    return msgTime;
}

void* CRemoteWS::operator new( size_t s ) {
    char* p;

    if ( !rwsPool ) {
        p = new char[s];
    } else {
        p = (char*)rwsPool->RemoveFirst();
        if ( !p )
            p = new char[s];
    }
    return p;
}

void CRemoteWS::operator delete( void* p ) {
    if ( !rwsPool ) {
        delete[]( char* ) p;
        return;                 // was missing: same bug as CNetAddress::operator delete.
                                // CleanPool() nulls rwsPool then `delete`s every pooled WS,
                                // so without the return this fell through to
                                // rwsPool->Insert() with rwsPool==NULL -> SEGV on session
                                // teardown (hit on the POSIX Join/enum-session cleanup path).
    }

    rwsPool->Insert( (CRemoteWS*)p );
}


void CRemoteWS::InitPool() {
    if ( !rwsPool )
        rwsPool = new WSList;
}

void CRemoteWS::CleanPool() {
    CRemoteWS* ws;
    WSList* list = rwsPool;

    rwsPool = NULL;


    while ( NULL != ( ws = (CRemoteWS*)list->RemoveFirst() ) )
        // Pool entries are ALREADY-DESTRUCTED raw blocks: a CRemoteWS only enters
        // the pool via operator delete, which (per C++) runs AFTER ~CRemoteWS. So
        // `delete ws` here would run ~CRemoteWS a SECOND time on dead memory — its
        // m_safeLink/m_unsafeLink/m_address/m_info->Unref() then derefs links that
        // the first destruct already released (use-after-free, the SEGV linux2 saw
        // at vpengine.cpp:2067 on the enum-session teardown). Free the raw buffer
        // directly (matches operator new's `new char[s]`) — no second destruct.
        delete[]( char* ) ws;
}




CVpSession::CVpSession( CTDLogger* log, CNetInterface* net,
                        CPlayerMap* players,
                        CWSMap* wsMap ): m_net( net ),
    m_log( log ), m_info( NULL ), m_players( players ), m_wsMap( wsMap ),
    m_localWS( NULL ), m_broadcastLink( NULL ), m_broadcastAddress( NULL ), m_registrationAddress( NULL ),
    m_error( 0 ), m_errInfo( 0 ), m_msgId( 0 ), m_invalid( FALSE ) {
    CRemoteWS::InitPool();
    m_keepLog = vpFetchInt( "VDMPLAY", "KeepDataLog", 0 );
}

void CVpSession::SetError( DWORD error, DWORD errInfo ) {
    m_error = error;
    m_errInfo = errInfo;

    if ( m_log )
        m_log->SetError( error, errInfo );
}

void CVpSession::FatalError( DWORD error, DWORD errInfo ) {
    m_error = error;
    m_errInfo = errInfo;

    // The latch behind the join bail: SetFatalError sets m_vdmPlay->m_fatalError
    // (vdmplay.cpp:654) and NOTHING clears it, so any fatal raised on the enum /
    // TCP-fallback path makes every later vpJoinSession return NULL. Log which
    // error code latches it (VP_ERR_NET_DOWN=fallback-link teardown is the prime
    // suspect) + timing, so one rerun pins gate + root. Gated; zero ship impact.
    if ( JoinAddrLogOn() )
        fprintf( stderr, "[join-addr] CVpSession::FatalError LATCHING m_fatalError: error=%lu errInfo=%lu (this blocks all later joins until handle reopen)\n",
                 (unsigned long)error, (unsigned long)errInfo );

    if ( m_log )
        m_log->SetFatalError( error, errInfo );

    m_invalid = TRUE;
}

void CVpSession::Log( LPCSTR msg ) {
    if ( m_log )
        m_log->Log( msg );
}


void CVpSession::SafeDataHook( CNetLink* link, LPVOID context ) {
    ( (CVpSession*)context )->OnSafeData( link );
}

void CVpSession::UnsafeDataHook( CNetLink* link, LPVOID context ) {
    ( (CVpSession*)context )->OnUnsafeData( link );
}

void CVpSession::ConnectHook( CNetLink* link, LPVOID context ) {
    ( (CVpSession*)context )->OnConnect( link );
}


void CVpSession::DisconnectHook( CNetLink* link, LPVOID context ) {
    ( (CVpSession*)context )->OnDisconnect( link );
}

void CVpSession::AcceptHook( CNetLink* link, LPVOID context ) {
    ( (CVpSession*)context )->OnAccept( link );
}



void CVpSession::InitSessionInfo( LPCVPGUID guid,
                                  DWORD version,
                                  DWORD sessionDataSize,
                                  DWORD playerDataSize ) {
    m_info = new( (size_t)sessionDataSize ) sesInfoMsg( (size_t)sessionDataSize );
//    m_info = new sesInfoMsg((size_t) sessionDataSize);

    if ( m_info ) {
        m_info->data.gameId = *guid;
        m_info->data.version = version;
        m_info->data.dataSize = sessionDataSize;
        m_info->data.playerDataSize = playerDataSize;
    }
}


BOOL CVpSession::InitNetwork( BOOL streamListen ) {
    VPASSERT( m_net );
    VPASSERT( m_info );

    m_net->SetHooks( SafeDataHook,
                     UnsafeDataHook,
                     ConnectHook,
                     DisconnectHook,
                     AcceptHook,
                     this );

    if ( !m_net->Listen( streamListen, IsServerMode() ) ) {
        Log( "CVpSession::Net->Listen failed\n" );
        if ( JoinAddrLogOn() )
            fprintf( stderr, "[join-addr] InitNetwork: Listen(streamListen=%d,serverMode=%d) FAILED -> MakeRemoteSession returns NULL -> vpJoinSession NULL (no host dial). This is the upstream-of-connect bail linux1's strace saw.\n",
                     (int)streamListen, (int)IsServerMode() );
        return FALSE;
    }

    m_broadcastAddress = m_net->MakeBroadcastAddress();

    if ( !m_broadcastAddress ) {
        Log( "CVpSession::InitNetwork Can't Get broadcast address" );
        FatalError( VP_ERR_NET_ERROR );
        return FALSE;
    }

    m_registrationAddress = m_net->MakeRegistrationAddress();

    // iserve host-register diagnostic: did the reg address survive into the session?
    // If NULL here the host CANNOT register (SendTo blocks below are skipped) even with
    // a correct [TCP]RegistrationAddress — points at SetRegistrationAddress timing/parse.
    if ( IserveLogOn() ) {
        if ( m_registrationAddress ) {
            char rb[160] = {0};
            m_registrationAddress->GetPrintForm( rb, sizeof( rb ) );
            fprintf( stderr, "[iserve-host] InitNetwork m_registrationAddress=%s (will register here)\n", rb );
        } else {
            fprintf( stderr, "[iserve-host] InitNetwork m_registrationAddress=NULL -> host will NOT register\n" );
        }
    }

    m_broadcastLink = m_net->MakeUnsafeLink();
    if ( !m_broadcastLink ) {
        Log( "CVpSession::InitNetwork Can't Get broadcast Link" );
        FatalError( VP_ERR_NET_ERROR );
        return FALSE;
    }

    // Zero the full 28-byte sessionId first: GetAddress only writes the
    // transport prefix (8 bytes for TCP), and the tail previously shipped heap
    // garbage. A deterministic zero tail is required now that vpnatcand.h uses
    // it for the observed-address candidate extension (P0.1).
    memset( &m_info->data.sessionId, 0, sizeof( m_info->data.sessionId ) );
    m_net->GetAddress( &m_info->data.sessionId );

    return TRUE;

}

void CVpSession::HandleNetDown() {
    VPENTER( CVpSession::HandleNetDown );

    CNotification* n = new CNotification( VP_NETDOWN, 0, 0, 0 );
    if ( !n ) {
        FatalError( VP_ERR_NOMEM );
    } else {
        PostNotification( n );
        FatalError( VP_ERR_NET_DOWN );
    }
    VPEXIT();
    return;
}

// --- NAT hole-punch shared plumbing (see vpengine.h decls + feasibility doc P1) ---

BOOL CVpSession::SendDgTo( LPCVPNETADDRESS to, LPVOID data, DWORD size ) {
    if ( !m_broadcastLink )
        return FALSE;

    CNetAddress* a = m_net->MakeAddress( to );
    if ( !a )
        return FALSE;

    BOOL r = m_broadcastLink->SendTo( *a, data, size, 0 );
    a->Unref();
    return r;
}

void CVpSession::PunchFireProbes( PunchPeer& p ) {
    natPunchMsg ping( NatPunchPING );
    ping.data.nonce = p.m_nonce;

    const unsigned char* pubIp = EnNatCandBytes( &p.m_pub );
    const unsigned char* privIp = EnNatCandBytes( &p.m_priv );

    if ( !EnNatCandIpZero( pubIp ) )
        SendDgTo( &p.m_pub, ping.Data(), ping.Size() );

    // Private candidate too — same-LAN peers and non-hairpinning home routers
    // only ever connect via it. Skip when absent or identical to public.
    if ( !EnNatCandIpZero( privIp ) &&
         !( EnNatCandIpEq( pubIp, privIp ) && pubIp[6] == privIp[6] && pubIp[7] == privIp[7] ) )
        SendDgTo( &p.m_priv, ping.Data(), ping.Size() );

    p.m_tries++;
    p.m_lastSend = GetCurrentTime();
}

BOOL CVpSession::PunchHandlePing( natPunchMsg* msg, CNetAddress* from, PunchPeer& p ) {
    // Always PONG back to the OBSERVED source: our reply is what opens (and
    // then keeps refreshing) our own NAT's outbound mapping toward the peer.
    natPunchMsg pong( NatPunchPONG );
    pong.data.nonce = msg->data.nonce;

    VPNETADDRESS src;
    memset( &src, 0, sizeof( src ) );
    from->GetNormalForm( &src );
    SendDgTo( &src, pong.Data(), pong.Size() );

    if ( p.m_state != PunchPeer::IDLE && msg->data.nonce == p.m_nonce ) {
        // The peer's probe reached us: the peer->us leg is open. Treat like a
        // PONG (our PONG above opens/confirms the us->peer leg at their end).
        if ( p.m_state != PunchPeer::CONFIRMED ) {
            p.m_state = PunchPeer::CONFIRMED;
            p.m_confirmed = src;
            char ab[64];
            EnNatCandFmt( ab, sizeof( ab ), EnNatCandBytes( &src ), EnNatCandBytes( &src ) + 6 );
            PunchLog( "CONFIRMED by peer PING from %s (tries=%d)", ab, p.m_tries );
        }
        p.m_lastAlive = GetCurrentTime();
        p.m_lastSend  = p.m_lastAlive;   // our PONG is traffic to this peer: suppresses a redundant keepalive
        return TRUE;
    }
    return FALSE;
}

// Liveness from REAL traffic. Previously m_lastAlive was refreshed ONLY by
// PING/PONG (PunchHandlePing/Pong), so a punched pair actively carrying data
// could still be declared dead — client at 60s, host at 120s (DrivePunch). That
// matters because StartNatPunch is reachable only from ConnectToServer, so a
// lost pair is never re-established for the rest of the session (board
// 2026-08-23). Any datagram from the confirmed peer proves the path is open.
// Compares the transport prefix (ip + stream/dg ports) only: the NAT-candidate
// tail is not part of peer identity, and both sides come from GetNormalForm.
void CVpSession::PunchNoteTraffic( PunchPeer& p, CNetAddress* from ) {
    if ( p.m_state != PunchPeer::CONFIRMED || !from )
        return;
    VPNETADDRESS src;
    memset( &src, 0, sizeof( src ) );
    from->GetNormalForm( &src );
    if ( memcmp( &src, &p.m_confirmed, 8 ) == 0 )
        p.m_lastAlive = GetCurrentTime();
}

BOOL CVpSession::PunchHandlePong( natPunchMsg* msg, CNetAddress* from, PunchPeer& p ) {
    if ( p.m_state == PunchPeer::IDLE || msg->data.nonce != p.m_nonce )
        return FALSE;

    VPNETADDRESS src;
    memset( &src, 0, sizeof( src ) );
    from->GetNormalForm( &src );

    if ( p.m_state != PunchPeer::CONFIRMED ) {
        p.m_state = PunchPeer::CONFIRMED;
        p.m_confirmed = src;
        char ab[64];
        EnNatCandFmt( ab, sizeof( ab ), EnNatCandBytes( &src ), EnNatCandBytes( &src ) + 6 );
        PunchLog( "CONFIRMED punched pair -> %s (tries=%d)", ab, p.m_tries );
    }
    p.m_lastAlive = GetCurrentTime();
    return TRUE;
}


void CVpSession::OnSafeData( CNetLink* link ) {
    DWORD waitingDataCount;
    DWORD count;            // how much data we've actually got form the net
    genericMsg* msg;
    VPMSGHDR  hdr;
    char buf[256];


    if ( m_net->KeepingBoundaries() ) {
        // Fine, the underlying protocol is doing the job for us,
        while ( 0 != ( waitingDataCount = link->HasData() ) ) {
            msg = new( (size_t)waitingDataCount - sizeof( VPMSGHDR ) ) genericMsg;
            if ( !msg ) {
                SetError( VP_ERR_NOMEM );
                return;
            }


            count = link->Receive( msg->Data(), waitingDataCount );


            if ( ( count < sizeof( VPMSGHDR ) ) || ( count < msg->Size() ) )  // ignore badly formatted messages
            {
                msg->Unref();
                continue;
            }

            ProcessSafeData( link, msg );
            msg->Unref();
        }
        return;
    }


    // The protocol does not keep messsage boundaries, we'll have to split reads in 2 steps
    // 1) Read the message header 
    // 2) Read the content using size from the message header

#if 0
    if ( link->HasPushbackData() ) {
        size_t pbCount = link->HasPushbackData();

        if ( pbCount < sizeof( hdr ) ) {
            count = link->HasData();
            if ( ( pbCount + count ) < sizeof( hdr ) )
                return;


        }

        count = link->ReadPushback( &hdr, sizeof( hdr ) );

    }
#endif

    // Stream reassembly (2026-07-01): the old code read the header, then did ONE
    // Receive for the body and DISCARDED the message when fewer bytes had arrived —
    // but those partial body bytes were already consumed, so the REMAINDER of the
    // body was later parsed as the next message header: permanent stream desync.
    // Downstream, the desynced pseudo-message's contents reached the app as a game
    // command (VP_READDATA dataLen comes from the header's self-reported msgSize),
    // which is exactly the deterministic garbage-veh_new SIGSEGV that killed the
    // POSIX clients in the first 3-platform MP game (mac2, 4/4 identical crashes).
    // Now: a short body is stashed on the link (m_pPartialMsg/m_partialGot) and
    // filled across data events; and we loop, so several complete messages in one
    // event are all processed instead of one-per-event.
    for ( ;; ) {

        // finish an in-progress body first
        if ( link->m_pPartialMsg ) {
            genericMsg* pm = link->m_pPartialMsg;
            DWORD body = pm->hdr.msgSize - sizeof( hdr );
            DWORD need = body - link->m_partialGot;

            if ( 0 == link->HasData() )
                return;

            count = link->Receive( (char*)pm->Contents() + link->m_partialGot, need );
            if ( count == 0 )
                return;
            link->m_partialGot += count;
            if ( link->m_partialGot < body )
                return;   // still short — wait for the next data event

            link->m_pPartialMsg = NULL;
            link->m_partialGot = 0;
            ProcessSafeData( link, pm );
            pm->Unref();
            continue;   // more messages may already be buffered
        }

        waitingDataCount = link->HasData();
        if ( waitingDataCount < sizeof( hdr ) )
            return;   // nothing consumed — a partial header stays in the socket buffer

        memset( &hdr, 0, sizeof( hdr ) );
        count = link->Receive( &hdr, sizeof( hdr ) );

        if ( count < sizeof( hdr ) ) {

            wsprintf( buf, "CVpSession::OnSafeData: Error reading message header for the link %ld", count );
            Log( buf );
            return;
        }

        if ( hdr.msgSize < sizeof( hdr ) ) {
            Log( "CVpSession::OnSafeData - too small message size in the message header" );
            OnDisconnect( link );
            return;
        }


        DWORD msgDataSize = hdr.msgSize - sizeof( hdr );

        if ( hdr.msgSize > VP_MAXSENDDATA ) {
            Log( "CVpSession::OnSafeData - message too big" );
            OnDisconnect( link );
            return;
        }

        msg = new( (size_t)msgDataSize ) genericMsg;
        if ( !msg ) {
            SetError( VP_ERR_NOMEM );
            return;
        }

        msg->hdr = hdr;

        count = ( msgDataSize > 0 ) ? link->Receive( msg->Contents(), msgDataSize ) : 0;

        if ( count < msgDataSize ) {
            // body not fully arrived — keep the message (and its ref) on the link
            link->m_pPartialMsg = msg;
            link->m_partialGot = count;
            return;
        }

        ProcessSafeData( link, msg );
        msg->Unref();
    }

}



BOOL CVpSession::ExceptEnumHelper( CWS* ws, LPVOID data ) {
    exceptEnumInfo& info = *(exceptEnumInfo*)data;

    return ( ws == info.exceptWs ) ? TRUE : info.p( ws, info.data );
}


void CVpSession::ForAllWorkstationsExcept( CWS* ws, WSEnum p, LPVOID data ) {
    exceptEnumInfo info;

    info.exceptWs = ws;
    info.p = p;
    info.data = data;

    ForAllWorkstations( ExceptEnumHelper, &info );
}


BOOL CVpSession::SendOnLink( CNetLink* link, LPVOID data ) {
    VPASSERT( link );
    VPASSERT( data );

    sendDataInfo& sdi = *(sendDataInfo*)data;

    link->Send( sdi.m_data, sdi.m_dataSize, sdi.m_sendFlags );
    return TRUE;
}

WORD CVpSession::PlayerCount() {
    return (WORD)m_players->Count();
}


BOOL CVpSession::GetSessionInfo( LPVPSESSIONINFO pInfo ) {
    VPASSERT( m_info );

    *pInfo = m_info->data;
    return TRUE;
}


CRemoteWS* CVpSession::MakeRemoteWS( CNetAddress* a, CNetLink* safeLink, CNetLink* unsafeLink ) {
    CRemoteWS* ws;
    if ( !unsafeLink ) {
        CNetLink* tmp = m_net->MakeUnsafeLink();
        ws = new CRemoteWS( a, safeLink, tmp );
        tmp->Unref();
    } else {
        ws = new CRemoteWS( a, safeLink, unsafeLink );
    }

    if ( !ws ) {
        SetError( VP_ERR_NOMEM );
    }
    return ws;
}



CPlayer* CVpSession::FindPlayer( VPPLAYERID pId, CWS* ws ) const {
    CPlayer* p = m_players->PlayerAtId( pId );

    if ( !p || ( ( ws != NULL ) && ( ws != p->m_ws ) ) )
        return NULL;

    return p;
}


// Host relay (spec 8). Read-only: no dial, no state change, no wire traffic.
// Always FALSE on the host and while the gate is off (nothing sets m_relayMode
// there), so the lobby marker cannot appear on a build that isn't relaying.
BOOL CVpSession::PeerIsRelayed( VPPLAYERID pId ) const {
    if ( !HostRelayOn() )
        return FALSE;

    CPlayer* p = FindPlayer( pId );

    if ( !p || !p->IsRemote() || !p->m_ws || !p->m_ws->IsRemote() )
        return FALSE;

    return ( (CRemoteWS*)p->m_ws )->m_relayMode;
}


CRemotePlayer* CVpSession::RemoveRemotePlayer( VPPLAYERID pId, CRemoteWS* ws ) {

    VPTRACE( ( "CVpSession::RemoveRemotePlayer id=%d\n", pId ) );

    CPlayer* p = FindPlayer( pId, ws );

    if ( !p ) {
        SetError( VP_ERR_BAD_PLAYER_ID, pId );
        return NULL;
    }

    if ( p->IsLocal() ) {
        SetError( VP_ERR_LOCAL_PLAYER, pId );
        return FALSE;
    }


    CLeaveNotification* n = new CLeaveNotification( p );
    m_players->RemovePlayer( p->PlayerId() );

    if ( !n ) {
        Log( "CVpSession::RemoveRemotePlayer - No memory for notification" );
        SetError( VP_ERR_NOMEM );
        return (CRemotePlayer*)p;
    }


    VPTRACE( ( "CVpSession::RemoveRemotePlayer Posting notification\n" ) );

    PostNotification( n );

    return (CRemotePlayer*)( p );
}


BOOL CVpSession::KnockOutPlayer( VPPLAYERID id, plrInfoMsg* msg, CRemoteWS* ws ) {
    CRemotePlayer* p = RemoveRemotePlayer( id, ws );

    if ( !p )
        return FALSE;


    // Now we're going to re-send a notification message to all Workstattions

    if ( !msg )
        msg = p->m_info;

    msg->hdr.msgTo = VP_LOCALMACHINE;
    msg->hdr.msgFrom = VP_SESSIONSERVER;
    msg->hdr.msgId = NextMessageId();
    msg->hdr.msgKind = LeaveREQ;


    sendDataInfo info( msg->Data(), msg->Size(), VP_MUSTDELIVER, NULL, this );
    ForAllWorkstationsExcept( p->m_ws, SendToWs, &info );

    p->m_ws->RemovePlayer();
    if ( p->m_ws->PlayerCount() == 0 )
        m_wsMap->Deregister( p->m_ws );

    return TRUE;

}

BOOL CVpSession::KillPlayer( VPPLAYERID id ) {
    // LogV's varargs are fixed DWORDs (wsprintf p1..p4), so a %s pointer can't
    // ride a DWORD on ANY 64-bit target (Win64 LLP64 *and* Linux LP64) — the old
    // (DWORD) cast truncated the ptr on LP64 and fails to compile on MSVC x64.
    // Fold the Local/Remote literal into the format string so only the numeric
    // id is passed. Portable, identical message on all three platforms (win's fix).
    LogV( m_log, IsLocal() ? "CVpSession::KillPlayer(Local) %u"
                           : "CVpSession::KillPlayer(Remote) %u", id );

    if ( !IsLocal() ) {
        SetError( VP_ERR_REMOTE_SESSION );
        return FALSE;
    }

    return KnockOutPlayer( id, NULL, NULL );
}






CRemotePlayer* CVpSession::AddRemotePlayer( plrInfoMsg* pInfoMsg, CRemoteWS* ws, BOOL doNotify ) {

    CRemotePlayer* player = new CRemotePlayer( pInfoMsg, ws );

    if ( !player ) {
        Log( "CVpSession::AddRemotePlayer - can't add remote player" );
        FatalError( VP_ERR_NOMEM );
        return NULL;
    }

    m_players->AddPlayer( player );
    ws->AddPlayer();

    if ( ws->PlayerCount() == 1 ) {
        // there is a chance that the station address is incomplete
        // fix this situation with the data from the player info
        CNetAddress* addr = m_net->MakeAddress( &pInfoMsg->data.playerAddress );
        ws->SetAddress( addr );
        addr->Unref();
    }


    if ( !doNotify )
        return player;

    CJoinNotification* n =
        new CJoinNotification( player, VP_ERR_OK );

    if ( !n ) {
        SetError( VP_ERR_NOMEM );
        return player;
    }

    PostNotification( n );
    return ( player );
}

CLocalPlayer* CVpSession::MakeLocalPlayer( LPCSTR playerName,
                                           DWORD  playerFlags ) {
    size_t playerDataSize = 256;

    plrInfoMsg* pInfoMsg = new( playerDataSize ) plrInfoMsg( playerDataSize );
    m_net->GetAddress( &pInfoMsg->data.playerAddress );
    pInfoMsg->data.playerFlags = playerFlags;
    _fmemcpy( pInfoMsg->data.playerName, playerName, playerDataSize );

    CLocalPlayer* p = new CLocalPlayer( pInfoMsg, m_localWS );

    pInfoMsg->Unref();
    return p;

}


WORD CVpSession::NextMessageId() {
    CSessLock lk( this );

    return ++m_msgId;
}

BOOL CVpSession::SendData( VPPLAYERID toId,
                           VPPLAYERID fromId,
                           LPVOID data,
                           DWORD dataSize,
                           DWORD flags,
                           LPVOID userData ) {
    sendDataInfo info( data, dataSize, flags & ~VP_BROADCAST, userData, this );
    LPVPMSGHDR pHdr = (LPVPMSGHDR)data;

    pHdr->msgTo = toId;
    pHdr->msgFrom = fromId;
    pHdr->msgKind = UDataREQ;
    pHdr->msgId = NextMessageId();
    pHdr->msgSize = (WORD)dataSize;
    pHdr->msgFlags = (BYTE)flags;
#if VP_TIMESTAMP
    pHdr->msgTime = vpMsgTime();
#endif

    if ( !( flags & VP_BROADCAST ) && ( toId != VP_ALLPLAYERS ) ) {
        CPlayer* player = m_players->PlayerAtId( toId );

        if ( !player ) {
            SetError( VP_ERR_BAD_PLAYER_ID, toId );
            return FALSE;
        }


        return player->SendData( info );
    }

    if ( !GoodBroadcastOptions( flags ) )
        return FALSE;

    // we have a broadcast here
    pHdr->msgKind = UBDataREQ;

#if 0 
    if ( !( flags & VP_MUSTDELIVER ) ) {
        // we simply send a broadcast message
        if ( !m_broadcastLink->SendTo( *m_broadcastAddress,
                                       data, dataSize, VP_BROADCAST ) ) {
            Log( "Session::SendData failed to send to broadcast address" );
            SetError( VP_ERR_NET_ERROR, m_broadcastLink->LastError() );
            return FALSE;
        }
        return TRUE;
    }
#endif
    ForAllWorkstations( SendAllPlayers, &info );
    return TRUE;
}






// Send the message to all palyers on the given WS
BOOL CVpSession::SendAllPlayers( CWS* ws, LPVOID data ) {
    VPASSERT( ws );
    VPASSERT( data );

    sendDataInfo& sdi = *(sendDataInfo*)data;
    LPVPMSGHDR pHdr = (LPVPMSGHDR)sdi.m_data;

    pHdr->msgTo = VP_ALLPLAYERS;

    ws->SendData( sdi );
    return TRUE;
}


BOOL CVpSession::SendToWs( CWS* ws, LPVOID data ) {
    VPASSERT( ws );
    VPASSERT( data );

    sendDataInfo& sdi = *(sendDataInfo*)data;

    ws->SendData( sdi );
    return TRUE;
}


BOOL CVpSession::OnUDataREQ( genericMsg* msg, CRemoteWS* ws ) {
    CDataNotification* n = new CDataNotification( msg );

    if ( !n ) {
        SetError( VP_ERR_NOMEM );
        return FALSE;
    }

    PostNotification( n );
    return TRUE;
}

BOOL CVpSession::OnUBDataREQ( genericMsg* msg, CRemoteWS* ws ) {
    return OnUDataREQ( msg, ws );
}


BOOL CVpSession::OnFtREQ( ftReqMsg* msg, CRemoteWS* ws ) {
    CFtNotification* n = new CFtNotification( msg, NULL );

    if ( !n ) {
        SetError( VP_ERR_NOMEM );
        return FALSE;
    }

    PostNotification( n );
    return TRUE;
}


BOOL CVpSession::OnFtACK( ftAckMsg* msg, CRemoteWS* ws ) {
    VPTRACE( ( "Got FTACK\n" ) );
    if ( ws->FtState() == VPFTINFO::FTREQSENT ) {
        ws->SetFtState( VPFTINFO::FTACKRECVD );
    }


    return TRUE;
}

BOOL CVpSession::StartFT( LPVPFTINFO ftInfo ) {
#if 0
    CPlayer* player = m_players->PlayerAtId( ftInfo->ftToId );

    if ( !player ) {
        ftInfo->ftErr = VP_ERR_BAD_PLAYER_ID;
        return FALSE;
    }

    ftReqMsg* msg = new( 0 ) ftReqMsg;


    if ( !msg ) {
        ftInfo->ftErr = VP_ERR_NOMEM;
        return FALSE;
    }

    ftInfo->ftXferCount = 0;
    ftInfo->ftState = VPFTINFO::FTREQSENT;
    ftInfo->ftErr = 0;
    ftInfo->ftTpData = 0;
    ftInfo->ftContext = 0;
    ftInfo->ftStatus2 = 0;

    msg->data = *ftInfo;

    sendDataInfo info( msg->Data(), msg->Size(), VP_MUSTDELIVER, NULL, NULL );
    player->SendData( info );

    // wait for FT ACK



    return TRUE;
#else
    return FALSE;
#endif

}

BOOL CVpSession::StopFT( LPVPFTINFO ftInfo ) {
    return FALSE;
}

// Accept incoming file transfer request
BOOL CVpSession::AcceptFT( LPVPFTINFO ftInfo ) {
    return FALSE;
}


// Send a file  data block
BOOL  CVpSession::SendBlock( LPVPFTINFO ftInfo,  // file transfer info
                             LPCVOID buf,
                             DWORD bufSize
) {
    return FALSE;
}


// Receive a file data block
BOOL CVpSession::GetBlock( LPVPFTINFO ftInfo,    // file transfer info
                           LPVOID buf,
                           DWORD bufSize
) {
    return FALSE;
}


int DeletePlayer( CPlayer* p, LPVOID data ) {
    delete p;
    return TRUE;
}

int DeleteWs( CWS* ws, LPVOID data ) {
    delete ws;
    return TRUE;
}


BOOL CVpSession::CloseSession() {

    delete m_players;
    m_players = NULL;

    delete m_wsMap;
    m_wsMap = NULL;

    // ForAllWorkstations(DeleteWs, NULL);

    if ( m_info ) {
        m_info->Unref();
        m_info = NULL;
    }

    if ( m_broadcastLink ) {
        m_broadcastLink->Unref();
        m_broadcastLink = NULL;
    }

    if ( m_broadcastAddress ) {
        m_broadcastAddress->Unref();
        m_broadcastAddress = NULL;
    }

    if ( m_registrationAddress ) {
        m_registrationAddress->Unref();
        m_registrationAddress = NULL;
    }

    if ( m_net ) {
        m_net->BecomeDeef();
        m_net->Cleanup();
    }


    return ( TRUE );
}





CVpSession::~CVpSession() {
    CloseSession();
    CRemoteWS::CleanPool();
}



CLocalSession::CLocalSession( CTDLogger* log,
                              CNetInterface* net,
                              CPlayerMap* players,
                              CWSMap* wsMap ):
    CVpSession( log, net, players, wsMap ), m_nextPlayerId( VP_FIRSTPLAYER ), m_visible( TRUE ),
    m_lastRegTime( 0 ), m_gotsEnumREQ( FALSE ) {
    for ( int i = 0; i < MAX_PUNCH_PEERS; i++ )
        m_punch[i].Reset();
}

//+ Connection establishemwent succeded
void CLocalSession::OnConnect( CNetLink* link ) {
    // Handle eventual connection failure
    DWORD err = link->LastError();

    if ( err ) {
        OnDisconnect( link );
    }

}



//+ New connection request arrived
void CLocalSession::OnAccept( CNetLink* link ) {
    CNetAddress* addr = link->GetRemoteAddress();
    CWS* ws = m_wsMap->FindByAddress( addr );

    if ( ws != NULL ) {
        // We have already a WS at this NetAddress
        ( (CRemoteWS*)ws )->SetSafeLink( link );
    } else {
        ws = MakeRemoteWS( addr, link, NULL );
        if ( !ws ) {
            addr->Unref();
            return;
        }

        m_wsMap->Register( ws );
        if ( IserveLogOn() )
            fprintf( stderr, "[iserve] host session REGISTERED -> %d session(s) now in registry\n",
                     (int)m_wsMap->Count() );
    }

    // Reply with session info
    m_info->Ref();

    sendDataInfo sdi( m_info->Data(), m_info->Size(), VP_MUSTDELIVER,
                      NULL, this );

    ws->SendData( sdi );

    m_info->Unref();
    addr->Unref();

}

struct leaveInfo {
    CWS* m_ws;
    CLocalSession* m_ses;
};

BOOL SimulateLeave( CPlayer* p, LPVOID ctx ) {
    VPASSERT( ctx );
    leaveInfo& info = *(leaveInfo*)ctx;

    if ( p->m_ws == info.m_ws ) {
        p->m_info->hdr.msgKind = LeaveREQ;
        info.m_ses->OnLeaveREQ( p->m_info, (CRemoteWS*)info.m_ws );
    }
    return TRUE;
}



void CLocalSession::OnDisconnect( CNetLink* link ) {
    LogV( m_log, "ClocalSession::OnDisconnect for link %08lx\n",
          (DWORD)(uintptr_t)link );   // LP64: cast via uintptr_t (ptr->DWORD direct is a clang error)

    if ( link->m_pPartialMsg ) {   // drop any half-reassembled stream message
        link->m_pPartialMsg->Unref();
        link->m_pPartialMsg = NULL;
        link->m_partialGot = 0;
    }

    if ( m_broadcastLink == link ) {
        HandleNetDown();
        return;
    }

    CWS* ws = m_wsMap->FindBySafeLink( link );

    if ( !ws )
        return;

    VPASSERT( ws->IsRemote() );

    ws->Ref();   // Make sure the object will not go away

    ( (CRemoteWS*)ws )->StopUsingSafeLink();

    if ( ws->PlayerCount() ) {
        leaveInfo info = { ws, this };

        m_players->Enum( SimulateLeave, &info );
    } else {
        m_wsMap->Deregister( ws );
    }

    ws->Unref();


}

void CLocalSession::ProcessSafeData( CNetLink* link, genericMsg* msg ) {
    BOOL unexpected = FALSE;
    CRemoteWS* ws = (CRemoteWS*)m_wsMap->FindBySafeLink( link );

    VPASSERT( ws );


    switch ( msg->hdr.msgKind ) {
    default:
        unexpected = TRUE;
        break;

    case SenumREQ:
        OnSenumREQ( msg, ws );
        break;


    case JoinREQ:
        OnJoinREQ( (plrInfoMsg*)msg, ws );
        break;

    case LeaveREQ:
        OnLeaveREQ( (plrInfoMsg*)msg, ws );
        break;


    case  PenumREQ:
        OnPenumREQ( msg, ws );
        break;

    case  UDataREQ:
        OnUDataREQ( msg, ws );
        break;

    case  UBDataREQ:
        OnUBDataREQ( msg, ws );
        break;

    case DummyREQ:
        break;
    }


    if ( unexpected )
        OnUnexpectedMsg( msg, link, TRUE );

}

//+ Process data coming from unsafe link
void CLocalSession::OnUnsafeData( CNetLink* link ) {
    genericMsg* msg = new( (size_t)VP_MAXSENDDATA ) genericMsg;
    const DWORD maxReadSize = VP_MAXSENDDATA + sizeof( VPMSGHDR );
    CNetAddress* addr = m_net->MakeAddress();
    DWORD count = link->ReceiveFrom( msg->Data(), maxReadSize, *addr );
    BOOL unexpected = FALSE;
    CWS* ws;

    if ( !count ) {
        msg->Unref();
        addr->Unref();
        return;
    }

    for ( int pi = 0; pi < MAX_PUNCH_PEERS; pi++ )
        PunchNoteTraffic( m_punch[pi], addr );

    switch ( msg->hdr.msgKind ) {
    case  SenumREQ:
        ws = MakeRemoteWS( addr, NULL, link );
        if ( !ws )
            break;
        OnSenumREQ( msg, (CRemoteWS*)ws );
        ws->Unref();
        break;

    case  UDataREQ:
    case  UBDataREQ:
        // we're going to ignore messages from unknown workstations
        ws = m_wsMap->FindByAddress( addr );
        if ( !ws ) break;

        // This is a message from known workstation
        if ( msg->hdr.msgKind == UDataREQ ) {
            OnUDataREQ( msg, (CRemoteWS*)ws );
        } else {
            OnUBDataREQ( msg, (CRemoteWS*)ws );
        }

        break;

    case DummyREQ:
        break;

    case NatPunchFWD:
        // iserve pushed a joiner's candidates through our warm registration
        // mapping (host role of the P1 rendezvous).
        if ( NatPunchOn() && msg->ContentSize() >= sizeof( natPunchInfo ) )
            OnNatPunchFWD( (natPunchMsg*)msg, addr );
        break;

    case NatPunchPING:
        if ( NatPunchOn() && msg->ContentSize() >= sizeof( natPunchInfo ) ) {
            // Exactly ONE PunchHandlePing call (it always PONGs the source):
            // aim it at the nonce-matching slot so that pairing confirms, or at
            // slot 0 (no match there => reply-only) when the nonce is unknown.
            int hit = 0;
            for ( int i = 0; i < MAX_PUNCH_PEERS; i++ ) {
                if ( m_punch[i].m_state != PunchPeer::IDLE &&
                     m_punch[i].m_nonce == ( (natPunchMsg*)msg )->data.nonce ) {
                    PunchHandlePing( (natPunchMsg*)msg, addr, m_punch[i] );
                    hit = 1;
                    break;
                }
            }
            if ( !hit )
                PunchHandlePing( (natPunchMsg*)msg, addr, m_punch[0] );
        }
        break;

    case NatPunchPONG:
        if ( NatPunchOn() && msg->ContentSize() >= sizeof( natPunchInfo ) ) {
            for ( int i = 0; i < MAX_PUNCH_PEERS; i++ ) {
                if ( PunchHandlePong( (natPunchMsg*)msg, addr, m_punch[i] ) )
                    break;
            }
        }
        break;

    default:
        unexpected = TRUE;
        break;
    }


    if ( unexpected )
        OnUnexpectedMsg( msg, link, FALSE );

    msg->Unref();
    addr->Unref();
}


// P1, host role: iserve forwarded a joiner's candidate pair (it can reach us
// any time — our periodic re-register keeps the NAT mapping to :1707 warm and
// the reply direction open). Start firing probes at the joiner from the same
// dg socket the session plays on.
void CLocalSession::OnNatPunchFWD( natPunchMsg* msg, CNetAddress* from ) {
    // Light origin check: only accept forwards from the configured reg server.
    if ( m_registrationAddress ) {
        VPNETADDRESS reg, src;
        memset( &reg, 0, sizeof( reg ) );
        memset( &src, 0, sizeof( src ) );
        m_registrationAddress->GetNormalForm( &reg );
        from->GetNormalForm( &src );
        if ( !EnNatCandIpEq( EnNatCandBytes( &reg ), EnNatCandBytes( &src ) ) ) {
            PunchLog( "host: dropping NatPunchFWD from non-regserver source" );
            return;
        }
    }

    // Re-forward for a nonce we already track just refreshes it; otherwise take
    // a free slot (or recycle the stalest).
    int slot = -1, stalest = 0;
    for ( int i = 0; i < MAX_PUNCH_PEERS; i++ ) {
        if ( m_punch[i].m_state != PunchPeer::IDLE && m_punch[i].m_nonce == msg->data.nonce ) {
            slot = i;
            break;
        }
        if ( m_punch[i].m_state == PunchPeer::IDLE ) {
            if ( slot < 0 || m_punch[slot].m_state != PunchPeer::IDLE )
                slot = i;
        } else if ( slot < 0 ) {
            if ( m_punch[i].m_lastSend <= m_punch[stalest].m_lastSend )
                stalest = i;
        }
    }
    if ( slot < 0 )
        slot = stalest;

    PunchPeer& p = m_punch[slot];
    if ( p.m_state == PunchPeer::IDLE || p.m_nonce != msg->data.nonce ) {
        p.Reset();
        p.m_nonce = msg->data.nonce;
        p.m_pub = msg->data.pubCand;
        p.m_priv = msg->data.privCand;
        p.m_state = PunchPeer::PROBING;

        char pb[64], vb[64];
        EnNatCandFmt( pb, sizeof( pb ), EnNatCandBytes( &p.m_pub ), EnNatCandBytes( &p.m_pub ) + 6 );
        EnNatCandFmt( vb, sizeof( vb ), EnNatCandBytes( &p.m_priv ), EnNatCandBytes( &p.m_priv ) + 6 );
        PunchLog( "host: joiner candidates pub=%s priv=%s%s -> probing", pb, vb,
                  ( msg->data.flags & EN_NATCAND_F_SAMENAT ) ? " (same public IP)" : "" );
    }

    PunchFireProbes( p );
}


// Punch retries/expiry for the host role, from CLocalSession::OnTimer.
void CLocalSession::DrivePunch() {
    if ( !NatPunchOn() )
        return;

    DWORD t = GetCurrentTime();

    for ( int i = 0; i < MAX_PUNCH_PEERS; i++ ) {
        PunchPeer& p = m_punch[i];

        switch ( p.m_state ) {
        case PunchPeer::PROBING:
            if ( p.m_tries >= 12 ) {
                PunchLog( "host: punch to joiner FAILED (12 probe rounds, no round-trip)" );
                p.Reset();
            } else if ( t - p.m_lastSend > 300 ) {
                PunchFireProbes( p );
            }
            break;

        case PunchPeer::CONFIRMED:
            // Host-side keepalive with piggyback suppression: PING only when
            // nothing else has gone to this peer in the interval (our PONGs set
            // m_lastSend, so an active pair sends nothing extra). Previously the
            // host sent nothing at all and relied entirely on the client, so a
            // stalled client aged out the host's own mapping too.
            if ( t - p.m_lastSend > 15000 ) {
                natPunchMsg ping( NatPunchPING );
                ping.data.nonce = p.m_nonce;
                SendDgTo( &p.m_confirmed, ping.Data(), ping.Size() );
                p.m_lastSend = t;
            }
            if ( t - p.m_lastAlive > 120000 ) {
                PunchLog( "host: punched pair expired (no keepalive for 120s)" );
                p.Reset();
            }
            break;

        default:
            break;
        }
    }
}




VPPLAYERID CLocalSession::NewPlayerId() {
    return m_nextPlayerId++;
}


BOOL CLocalSession::SetVisibility( BOOL v ) {
    m_visible = v;
    return TRUE;
}


CRemotePlayer* CLocalSession::AddRemotePlayer( plrInfoMsg* pInfo,
                                               CRemoteWS* ws ) {
    pInfo->data.playerId = NewPlayerId();
    return CVpSession::AddRemotePlayer( pInfo, ws, TRUE );
}

BOOL CLocalSession::AddLocalPlayer( LPCSTR playerName,
                                    DWORD  playerFlags,
                                    LPVOID userData, LPVPPLAYERID pId ) {
    // create te player objet and store it in the player map
    CLocalPlayer* p = MakeLocalPlayer( playerName, playerFlags );

    plrInfoMsg* pInfoMsg = p->m_info;

    pInfoMsg->data.playerId = NewPlayerId();

    if ( pId )
        *pId = pInfoMsg->data.playerId;


    m_players->AddPlayer( p );


    // notify all other worksations about this player
    pInfoMsg->hdr.msgId = NextMessageId();
    pInfoMsg->hdr.msgKind = JoinADV;

    sendDataInfo info( pInfoMsg->Data(), pInfoMsg->Size(), VP_MUSTDELIVER,
                       NULL, this );
    ForAllWorkstations( SendToWs, &info );

    // CLocalJoin* n = new CLocalJoin(p, pInfoMsg->hdr.msgId, userData);
    // PostNotification(n);

    return TRUE;
}



BOOL CLocalSession::OnLeaveREQ( plrInfoMsg* msg, CRemoteWS* ws ) {

#if 1
    LogV( m_log, "CLocalSession:: OnLeaveREQ Player %u", msg->data.playerId );

    return KnockOutPlayer( msg->data.playerId, msg, ws );
#else

    CRemotePlayer* p = RemoveRemotePlayer( msg->data.playerId, ws );

    if ( !p )
        return FALSE;


    // Now we're going to re-send a notification message to all Workstattions


    msg->hdr.msgTo = VP_LOCALMACHINE;
    msg->hdr.msgFrom = VP_SESSIONSERVER;
    msg->hdr.msgId = NextMessageId();


    sendDataInfo info( msg->Data(), msg->Size(), VP_MUSTDELIVER, NULL, this );
    ForAllWorkstationsExcept( p->m_ws, SendToWs, &info );

    p->m_ws->RemovePlayer();
    if ( p->m_ws->PlayerCount() == 0 )
        m_wsMap->Deregister( p->m_ws );

    return TRUE;
#endif

}


BOOL CLocalSession::OnJoinREQ( plrInfoMsg* msg, CRemoteWS* ws ) {

    CRemotePlayer* p = AddRemotePlayer( msg, ws );

    msg->hdr.msgTo = VP_LOCALMACHINE;
    msg->hdr.msgFrom = VP_SESSIONSERVER;

    sendDataInfo info( msg->Data(), msg->Size(), VP_MUSTDELIVER, NULL, this );


    if ( !p ) {
        // we must reply a NAK to the sender
        msg->hdr.msgKind = JoinNAK;
        SendToWs( ws, &info );  // reply back to the sender
        return FALSE;
    }

    // Now we're going to re-send a notification message to all Workstations

    msg->hdr.msgKind = JoinREP;
    msg->data.playerId = p->PlayerId();


    SendToWs( ws, &info );  // first reply back to the sender


    msg->hdr.msgKind = JoinADV;

    ForAllWorkstationsExcept( ws, SendToWs, &info );

    return TRUE;
}

// Host relay (docs/plans/host-relay-spec.md 4a): a unicast whose destination
// player lives on ANOTHER workstation is forwarded there instead of being
// delivered locally — the unicast mirror of the broadcast fan in OnUBDataREQ
// below. One hop, host only (clients never forward), so loops are structurally
// impossible. The wire is unchanged: msgFrom still names the original sender,
// so the receiver cannot tell a forwarded message from a direct one.
BOOL CLocalSession::OnUDataREQ( genericMsg* msg, CRemoteWS* ws ) {
    if ( HostRelayOn() && msg->hdr.msgTo != VP_ALLPLAYERS &&
         msg->hdr.msgFrom != msg->hdr.msgTo ) {         // sentinel: never a self-send
        CPlayer* dst = FindPlayer( msg->hdr.msgTo );

        if ( dst && dst->IsRemote() && dst->m_ws &&
             dst->m_ws != (CWS*)ws ) {                  // sentinel: never back at the sender
            sendDataInfo info( msg->Data(), msg->Size(),
                               msg->hdr.msgFlags & VP_MUSTDELIVER, NULL, this );

            dst->m_ws->SendData( info );                // rides the join link
            RelayLog( "host: FWD from=%d to=%d kind=%c size=%lu",
                      (int)msg->hdr.msgFrom, (int)msg->hdr.msgTo,
                      (char)msg->hdr.msgKind, (unsigned long)msg->Size() );
            return TRUE;                                // forwarded, not ours
        }
    }

    return CVpSession::OnUDataREQ( msg, ws );           // local delivery, as today
}

BOOL CLocalSession::OnUBDataREQ( genericMsg* msg, CRemoteWS* ws ) {

    OnUDataREQ( msg, ws );  // deliver this message to local destinations

    sendDataInfo info( msg->Data(), msg->Size(), msg->hdr.msgFlags & VP_MUSTDELIVER, NULL, this );

    ForAllWorkstationsExcept( ws, SendToWs, &info );
    return TRUE;

}

BOOL CLocalSession::OnSenumREQ( genericMsg* msg, CRemoteWS* ws ) {
    if ( IsVisible() ) {
        m_gotsEnumREQ = TRUE;
        m_info->hdr.msgId = msg->hdr.msgId;
        m_info->hdr.msgKind = SenumREP;

        sendDataInfo info( m_info->Data(), m_info->Size(), 0, NULL, this );
        ws->SendData( info );
        if ( m_registrationAddress && ws->m_address &&
             m_registrationAddress->IsEqual( ws->m_address ) ) {

            m_lastRegTime = GetCurrentTime();
        }

    }

    if ( !m_keepLog )
        m_net->StopDataLog();

    return TRUE;
}

struct spiContext {
    CRemoteWS* ws;
    WORD    totalPlayers;
    WORD    iterationIndex;
    WORD    msgId;
};

BOOL SendPlayerInfo( CPlayer* p, LPVOID data ) {
    spiContext& ctx = *(spiContext*)data;
    plrInfoMsg* msg = p->m_info;

    msg->hdr.msgId = ctx.msgId;
    msg->seq.index = ctx.iterationIndex++;
    msg->seq.total = ctx.totalPlayers;

    sendDataInfo info( msg->Data(), msg->Size(), VP_MUSTDELIVER, NULL, NULL );
    return ctx.ws->SendData( info );
}




BOOL CLocalSession::OnPenumREQ( genericMsg* msg, CRemoteWS* ws ) {
    spiContext ctx;

    ws->Ref();
    ctx.ws = ws;
    ctx.totalPlayers = PlayerCount();
    ctx.iterationIndex = 1;
    ctx.msgId = msg->hdr.msgId;

    m_players->Enum( SendPlayerInfo, &ctx );
    ws->Unref();

    return TRUE;
}



BOOL CLocalSession::UpdateSessionInfo( LPVOID data ) {
    CSessLock lk( this );
    _fmemcpy( m_info->data.sessionName, data, (size_t)m_info->data.dataSize );
    return BroadcastSessionData();
}


BOOL CLocalSession::BroadcastSessionData() {
    VPASSERT( m_broadcastLink );


    if ( IsVisible() ) {
        // First we send the broadcast message
        if ( !m_broadcastLink->SendTo( *m_broadcastAddress,
                                       m_info->Data(), m_info->Size(), VP_BROADCAST ) ) {
            Log( "CLocalSession failed to send broadcast address" );
            SetError( VP_ERR_NET_ERROR, m_broadcastLink->LastError() );
            return FALSE;
        }

        if ( m_registrationAddress ) {
            m_broadcastLink->SendTo( *m_registrationAddress,
                                     m_info->Data(), m_info->Size(), 0 );
            m_lastRegTime = GetCurrentTime();
            if ( IserveLogOn() )
                fprintf( stderr, "[iserve-host] BroadcastSessionData -> sent registration to reg server (msgKind=%d; reg server only registers SenumREP=%d)\n",
                         (int)m_info->hdr.msgKind, (int)SenumREP );
        } else if ( IserveLogOn() ) {
            fprintf( stderr, "[iserve-host] BroadcastSessionData: m_registrationAddress NULL -> NOT registering\n" );
        }
    }


    // Now we're sending the data to all workstations
    sendDataInfo info( m_info->Data(), m_info->Size(), VP_MUSTDELIVER, NULL, this );
    ForAllWorkstations( SendToWs, &info );
    return m_error == VP_ERR_OK;
}


void CLocalSession::OnTimer() {
    DrivePunch();   // NAT hole-punch probe retries/expiry (no-op unless EN_NAT_PUNCH)

    // Do NOT gate the reg-server heartbeat on !m_gotsEnumREQ. That flag trips on the
    // first DIRECT SenumREQ from any client — and a joiner's LookForServer sends one —
    // which permanently silenced the periodic re-register, so the reg server aged the
    // session out and later clients saw an empty list ("host vanishes from iserve once
    // the first player joins"). The flag's design assumed a reg server that RELAYS enum
    // queries to hosts (whose replies then refresh the registration — see OnSenumREQ's
    // m_lastRegTime touch); ours answers from its own registry and never relays, so the
    // heartbeat must run for as long as the session is visible and reg-configured.
    if ( m_visible && m_registrationAddress ) {
        DWORD t = GetCurrentTime();

        if ( t - m_lastRegTime > 2000 ) {
            m_broadcastLink->SendTo( *m_registrationAddress,
                                     m_info->Data(), m_info->Size(), 0 );
            m_lastRegTime = GetCurrentTime();
            if ( IserveLogOn() )
                fprintf( stderr, "[iserve-host] OnTimer -> periodic re-register sent to reg server\n" );
        }
    } else if ( IserveLogOn() ) {
        // Throttled: which gate is blocking the periodic re-register? (NULL regAddr,
        // not visible, or already got an enum REQ.) Tells us if OnTimer even runs on POSIX.
        static DWORD lastWhine = 0;
        DWORD t = GetCurrentTime();
        if ( t - lastWhine > 5000 ) {
            fprintf( stderr, "[iserve-host] OnTimer NOT registering: visible=%d regAddr=%p gotsEnumREQ=%d\n",
                     (int)m_visible, (void*)m_registrationAddress, (int)m_gotsEnumREQ );
            lastWhine = t;
        }
    }
}



CRemoteSession::CRemoteSession( CTDLogger* log,
                                CNetInterface* net,
                                CPlayerMap* players,
                                CWSMap* wsMap, DWORD maxAge ):
    CVpSession( log, net, players, wsMap ), m_serverWS( NULL ), m_pendingJoin( NULL ),
    m_initialJoin( TRUE ), m_serverEnumData( NULL ), m_maxServerAge( maxAge ), m_connected( FALSE ),
    m_tcpEnumTried( FALSE ), m_hasAltServer( FALSE ), m_altServerUserData( NULL ),
    m_redialPending( FALSE ), m_dialStart( 0 ),
    m_punchWanted( FALSE ), m_punchLastArm( 0 ) {
    memset( &m_altServerAddr, 0, sizeof( m_altServerAddr ) );
    memset( &m_punchSessionId, 0, sizeof( m_punchSessionId ) );
    m_punch.Reset();
}


BOOL CRemoteSession::LookForServer( LPVOID data ) {
    BOOL ret = TRUE;
    VPASSERT( m_broadcastLink );
    VPASSERT( m_broadcastAddress );

    if ( m_connected )
        return TRUE;

    m_serverEnumData = data;
    genericMsg* msg = new( 0 ) genericMsg( SenumREQ, 0 );
    CNetAddress* lookupA;

    if ( !m_net->IsSlowNet() ) {
        // First we send point to pint message to the rehistery server
        lookupA = m_net->MakeServerLookupAddress();

        if ( !lookupA )
            return FALSE;

        // TCP-discovery diagnostic (env EN_NETTRACE=1): show the DIRECTED server-lookup
        // address the client sends its sEnumREQ to — confirms whether the directed query
        // actually targets the registration server (vs falling back to broadcast).
        { static int nt=-1; if(nt<0) nt=getenv("EN_NETTRACE")?1:0;
          if(nt){ char ab[128]={0}; lookupA->GetPrintForm(ab,sizeof(ab));
                  fprintf(stderr,"[nettrace] enum sEnumREQ -> server-lookup addr=%s\n", ab); } }

        if ( !m_broadcastLink->SendTo( *lookupA,
                                       msg->Data(), msg->Size(), 0 ) ) {
            Log( "CremoteSession failed to send to lookup address" );
            SetError( VP_ERR_NET_ERROR, m_broadcastLink->LastError() );
            ret = FALSE;
        }

        lookupA->Unref();
    }

    // Now we send broadcast broadcast message
    lookupA = m_net->MakeBroadcastAddress();
    if ( !m_broadcastLink->SendTo( *lookupA,
                                   msg->Data(), msg->Size(), 0 ) ) {
        Log( "CremoteSession failed to send to brodcast address" );
        SetError( VP_ERR_NET_ERROR, m_broadcastLink->LastError() );
        ret = FALSE;
    }

    lookupA->Unref();
    msg->Unref();
    AgeServerList();

    // TCP-enum (phase-3): UDP is always tried first (above). Then, for a DIRECTED reg
    // server, also fire a one-shot TCP query in PARALLEL ("happy-eyeballs", per win) —
    // because the enum's WM_TIMER re-poll does NOT fire on POSIX, so a Search yields only
    // ONE LookForServer call (linux2 [08:10Z]) — we can't wait for "no reply after N
    // polls". The reg server answers whichever (UDP or TCP) reaches it; replies dedup in
    // m_wsMap. When UDP :1707 is blocked, only the TCP probe gets through -> discovery
    // still works. Additive: the UDP path is untouched; fires at most once per enum
    // session; no-op on broadcast-only LANs (MakeServerStreamAddress == NULL).
    TryTcpEnumFallback();

    return TRUE;
}


// TCP-enum (phase-3, mac 2026-06-26): the client's UDP->TCP enum auto-fallback. Called
// from LookForServer once the directed UDP query has gone unanswered for ~2 poll periods.
// Trigger is clean vs "serving 0": iserve replies DummyREQ even with 0 hosts, which sets
// m_serverSeen (MatchAddress) — so !IsServerSeen() means UDP genuinely didn't reach the
// reg server, NOT that it's empty. Connects TCP to the reg server's well-known port and
// sends a SenumREQ over the stream (queued + flushed on FD_CONNECT via SendWaitingData,
// exactly like the join flow); the reply arrives via ProcessSafeData's SenumREP case.
// Reuses m_serverWS — safe because the ENUM session is a distinct object from the JOIN
// session (MakeSessionEnum vs JoinSession) and is torn down at StopEnumSessions.
void CRemoteSession::TryTcpEnumFallback() {
    if ( m_tcpEnumTried || m_connected || m_serverWS )
        return;

    // Already heard from the directed reg server over UDP? Then UDP works — don't fall back.
    VPNETADDRESS seen;
    memset( &seen, 0, sizeof( seen ) );
    if ( m_net->IsServerSeen( &seen ) )
        return;

    // No directed reg server configured (broadcast-only LAN) / non-TCP net => no fallback.
    CNetAddress* sa = m_net->MakeServerStreamAddress();
    if ( !sa )
        return;

    m_tcpEnumTried = TRUE;

    { static int nt = -1; if ( nt < 0 ) nt = getenv( "EN_NETTRACE" ) ? 1 : 0;
      if ( nt ) {
          char ab[128] = {0}; sa->GetPrintForm( ab, sizeof( ab ) );
          fprintf( stderr, "[nettrace] enum: no UDP reply from reg server -> TCP-enum fallback connect to %s\n", ab ); } }

    CNetLink* link = m_net->MakeSafeLink( sa, NULL );
    if ( !link ) {
        sa->Unref();
        return;
    }

    m_serverWS = MakeRemoteWS( sa, link, NULL );
    if ( m_serverWS ) {
        m_wsMap->Register( m_serverWS );
        link->Unref();

        // Send the SenumREQ over the (async-connecting) TCP link; it queues and flushes
        // on FD_CONNECT (SendWaitingData), same as the join flow's JoinREQ.
        genericMsg* req = new( 0 ) genericMsg( SenumREQ, 0 );
        sendDataInfo info( req->Data(), req->Size(), VP_MUSTDELIVER, NULL, this );
        m_serverWS->SendData( info );
        req->Unref();
    }

    sa->Unref();
}





BOOL CRemoteSession::SendData( VPPLAYERID toId,
                               VPPLAYERID fromId,
                               LPVOID data,
                               DWORD dataSize,
                               DWORD flags,
                               LPVOID userData ) {
    if ( !m_connected ) {
        Log( "CRemoteSession::SendData when disconnetced" );
        VPTRACE( ( "CremoteSession::SendData when disconnected" ) );
        //  return FALSE;
    }
    // Use standard method for sending UNICAST data
    if ( !( flags & VP_BROADCAST ) && ( toId != VP_SESSIONSERVER ) && ( toId != VP_ALLPLAYERS ) ) {
        return CVpSession::SendData( toId, fromId, data,
                                     dataSize, flags, userData );
    }

    // Use server to send BROADCAST data
    sendDataInfo info( data, dataSize, flags, userData, this );

    LPVPMSGHDR pHdr = (LPVPMSGHDR)data;
    if ( toId != VP_SESSIONSERVER )
        pHdr->msgTo = VP_ALLPLAYERS;

    pHdr->msgFrom = fromId;
    pHdr->msgKind = UBDataREQ;
    pHdr->msgId = NextMessageId();
    pHdr->msgSize = (WORD)dataSize;
#if VP_TIMESTAMP
    pHdr->msgTime = vpMsgTime();
#endif
    if ( toId == VP_SESSIONSERVER ) {
        pHdr->msgTo = toId;
        pHdr->msgKind = UDataREQ;
    }

    // Guard: a send to the server while m_serverWS is NULL (server WS not yet
    // established, or torn down) would deref NULL here — the post-join SEGV linux2
    // hit at vpengine.cpp:1337 on the first player-data send. Fail the send cleanly
    // instead of crashing. (CNetApi::Send only treats a send failure as fatal if
    // the target player is already registered; during the early join handshake the
    // server player isn't, so this returns gracefully rather than aborting.)
    if ( !m_serverWS ) {
        Log( "CRemoteSession::SendData - no server WS (send to server before connect WS ready)" );
        VPTRACE( ( "CRemoteSession::SendData: m_serverWS NULL, dropping send to server" ) );
        return FALSE;
    }

    return m_serverWS->SendData( info );

}


BOOL CRemoteSession::OnLeaveREQ( plrInfoMsg* msg, CRemoteWS* ws ) {
    // we know that this message comes from the server, so we pass NULL pointer as
    // to the RemoveRemotePlayer to cause it to ignore WS object during player Lookup
    CRemotePlayer* p = RemoveRemotePlayer( msg->data.playerId, NULL );

    if ( !p )
        return FALSE;

    CWS* pws = p->m_ws;
    pws->RemovePlayer();
    if ( pws->PlayerCount() == 0 ) {
        // The player we're going to remove is the only one
        // on this worksation
        m_wsMap->Deregister( pws );
    }

    return TRUE;
}


BOOL CRemoteSession::OnJoinREP( plrInfoMsg* msg, CRemoteWS* ws ) {
    VPASSERT( m_serverWS );

    // the reply id must match request id
    if ( m_pendingJoin && ( m_pendingJoin->MsgId() == msg->hdr.msgId ) ) {

        CLocalPlayer* p = m_pendingJoin->Player();
        p->FixId( msg->data.playerId );
        m_players->AddPlayer( p );
        PostNotification( m_pendingJoin );
        m_pendingJoin = NULL;

        if ( m_initialJoin ) {
            // this was the initial join - send a PenumREQ message

            genericMsg* msg = new( 0 ) genericMsg( PenumREQ, 0 );
            msg->hdr.msgId = NextMessageId();

            m_initialJoin = FALSE;

            sendDataInfo info( msg->Data(), msg->Size(), VP_MUSTDELIVER, NULL, NULL );
            return m_serverWS->SendData( info );
        }


        return TRUE;
    }

    return TRUE;
}


CRemoteWS* CRemoteSession::RegisterPlayerWS( plrInfoMsg* msg ) {

    CNetAddress* addr = m_net->MakeAddress( &msg->data.playerAddress );

    CWS* ws = m_wsMap->FindByAddress( addr );
    if ( ws == NULL ) {
        ws = MakeRemoteWS( addr, NULL, NULL );
        if ( !ws ) {
            addr->Unref();
            return NULL;
        }
        m_wsMap->Register( ws );
        ws->Unref();
    }

    addr->Unref();
    return (CRemoteWS*)ws;
}


// Host relay (spec 8): client↔client links are made lazily on FIRST SEND, so in
// the lobby a client does not yet know whether a peer is direct-reachable and
// the (r) marker would never appear there. This kicks that same lazy dial once,
// early — no payload, no new message kind, nothing on the wire the first real
// unicast would not already have sent. Success pre-warms a real p2p link;
// failure lands in OnDisconnect above and arms relay mode, which is what makes
// the marker show. One dial per peer per session, gate-off = no-op.
BOOL CRemoteSession::ProbePeerLink( VPPLAYERID pId ) {
    if ( !HostRelayOn() )
        return FALSE;

    CPlayer* p = FindPlayer( pId );

    if ( !p || !p->IsRemote() || !p->m_ws || !p->m_ws->IsRemote() )
        return FALSE;

    CRemoteWS* ws = (CRemoteWS*)p->m_ws;

    // The host is reached over the join link by construction — never probed,
    // never relayed. An already-linked or already-probed peer is done.
    if ( ws == m_serverWS || ws->m_relayProbed || ws->m_safeLink || ws->m_relayMode )
        return FALSE;

    ws->m_relayProbed = TRUE;
    ws->m_safeLink = MakeSafeLink( ws->m_address );   // async connect; owns the ref

    if ( !ws->m_safeLink ) {
        ws->m_relayMode = TRUE;
        RelayLog( "client: probe dial to peer %d could not start -> RELAY MODE (sticky)", (int)pId );
        return FALSE;
    }

    RelayLog( "client: probing direct link to peer %d", (int)pId );
    return TRUE;
}


BOOL CRemoteSession::OnJoinADV( plrInfoMsg* msg, CRemoteWS* ) {
    if ( m_players->PlayerAtId( msg->data.playerId ) )  // we're simply ignoring duplicates
        return TRUE;


    CRemoteWS* ws = RegisterPlayerWS( msg );
    if ( !ws )
        return FALSE;

    CRemotePlayer* p = AddRemotePlayer( msg, ws, TRUE );

    if ( !p ) {
        return FALSE;
    }

    // ws->Unref();
    return TRUE;
}

BOOL CRemoteSession::OnJoinNAK( plrInfoMsg* msg, CRemoteWS* ws ) {
    if ( !m_pendingJoin )
        return TRUE;

    PostNotification( m_pendingJoin );
    m_pendingJoin = 0;
    return TRUE;
}



BOOL CRemoteSession::AddLocalPlayer( LPCSTR playerName,
                                     DWORD  playerFlags,
                                     LPVOID userData, LPVPPLAYERID ) {
    if ( m_pendingJoin ) {
        SetError( VP_ERR_BUSY );
        return FALSE;

    }


    CLocalPlayer* p = MakeLocalPlayer( playerName, playerFlags );

    if ( JoinAddrLogOn() )
        fprintf( stderr, "[join-addr] AddLocalPlayer: m_serverWS=%s -> %s\n",
                 m_serverWS ? "ALREADY-SET" : "NULL",
                 m_serverWS ? "SKIP ConnectToServer (reuse existing ws — if it's the enum's :1707 reg-server ws, the host session is never dialed)"
                            : "will ConnectToServer(host session)" );

    if ( !m_serverWS ) {
        m_initialJoin = TRUE;
        if ( !ConnectToServer( &m_info->data.sessionId, NULL ) ) {
            delete p;
            return FALSE;
        }
    }


    plrInfoMsg* pInfoMsg = p->m_info;


    pInfoMsg->hdr.msgKind = JoinREQ;
    pInfoMsg->hdr.msgId = NextMessageId();
    m_pendingJoin = new CLocalJoin( p, pInfoMsg->hdr.msgId, userData );



    sendDataInfo info( pInfoMsg->Data(), pInfoMsg->Size(), VP_MUSTDELIVER, NULL, NULL );
    return m_serverWS->SendData( info );

}


#if 0
BOOL CRemoteSession::OnUDataREQ( genericMsg* msg, CRemoteWS* ws ) {
    IMPOSSIBLE( OnUDataREQ );
    return TRUE;
}


BOOL CRemoteSession::OnUBDataREQ( genericMsg* msg, CRemoteWS* ws ) {
    IMPOSSIBLE( OnUBDataREQ );
    return TRUE;
}

#endif

BOOL CRemoteSession::OnPenumREP( plrInfoMsg* msg, CRemoteWS* ws ) {
    // This is the reply on the request we've sent to server just after joining it
    // we're treating it like JOIN ADVERTISEMENT messages while ignoring info
    // about local players

    CPlayer* p = m_players->PlayerAtId( msg->data.playerId );
    if ( !p->IsLocal() )
        return OnJoinADV( msg, ws );

    return TRUE;

#if 0
    CWS* ws = RegisterPlayerWS( msg );
    if ( !ws )
        return FALSE;

    CRemotePlayer* p = AddRemotePlayer( msg, ws, FALSE );

    if ( !p ) {
        return FALSE;
    }

    ws->Unref();
    return TRUE;
#endif
}

struct AgeParams {
    DWORD curTime;
    WSXList list;
    DWORD maxAge;
};


int CRemoteSession::CheckForAge( CWS* ws, LPVOID data ) {
    AgeParams* p = (AgeParams*)data;


    if ( ws->IsRemote() && ( p->curTime - ws->LastSeen() > p->maxAge ) )
        p->list.Append( new WSLink( ws ) );
    return TRUE;
}

void CRemoteSession::AgeServerList() {

    if ( m_connected )
        return;

    AgeParams p;

    p.curTime = GetCurrentTime();
    p.maxAge = m_maxServerAge;

    m_wsMap->Enum( CheckForAge, &p );

    WSLink* wsl;

    while ( NULL != ( wsl = p.list.RemoveFirst() ) ) {

        CRemoteWS* aged = (CRemoteWS*)( wsl->m_data );

        // Only post a "server down" for a ws that actually reported a discovered server,
        // i.e. has session Info(). A ws with NULL Info() never got a SenumREP — e.g. a
        // TCP-enum probe connection (TryTcpEnumFallback) to a reg server that didn't reply,
        // or any ws aged before its first reply — and CServerDownNotification's ctor derefs
        // ws->Info()->Contents()/ContentSize() -> SIGSEGV on NULL. (Surfaced only once the
        // POSIX OnTimer-drive @7ed73b9b started actually running AgeServerList.) Just
        // deregister the info-less ws; there's no discovered server to notify "down".
        if ( aged->Info() ) {
            CServerDownNotification* n = new CServerDownNotification( aged, m_serverEnumData );

            if ( !n ) {
                SetError( VP_ERR_NOMEM );
                return;
            }

            PostNotification( n );
        }

        m_wsMap->Deregister( wsl->m_data );
        delete wsl;
    }
}


BOOL CRemoteSession::OnSenumREP( sesInfoMsg* msg, CRemoteWS* ws ) {
    VPENTER( CRemoteSession::OnSenumREP );

    if ( msg && JoinAddrLogOn() ) {
        char abuf[64];
        FormatVpAddr( abuf, sizeof( abuf ), &msg->data.sessionId );
        fprintf( stderr, "[join-addr] OnSenumREP relayed sessionId=%s\n", abuf );
    }

    if ( m_connected ) {
        // we've just established the connection to the server
        // and the server responds with SenumREP

        Log( "Got SenumREP in connected state" );

        if ( m_info )
            m_info->Unref();
        m_info = msg;
        m_info->Ref();
        VPLEAVEBOOL( CRemoteSession::OnSenumREP, TRUE );
        return TRUE;
    }


    Log( "Got SenumREP in non-connected state" );


    CWS* knownWs = m_wsMap->Find( ws );

    if ( knownWs )   // we've already seen this workstation and notifyed the client app about it
    {
        ws->Touch();
        VPLEAVEBOOL( CRemoteSession::OnSenumREP, FALSE );
        return FALSE;
    }

    CSenumNotification* n = new CSenumNotification( msg, m_serverEnumData );
    if ( !n ) {
        Log( "OnSenumRep: no moemory for notification object" );
        SetError( VP_ERR_NOMEM );
        VPLEAVEBOOL( CRemoteSession::OnSenumREP, FALSE );
        return FALSE;
    }

    ws->KeepInfo( msg );
    m_wsMap->Register( ws );
    ws->Touch();

    Log( "OnSenumRep: posting notification" );
    PostNotification( n );

    if ( !m_keepLog )
        m_net->StopDataLog();

    VPLEAVEBOOL( CRemoteSession::OnSenumREP, TRUE );
    return TRUE;
}


BOOL CRemoteSession::SetVisibility( BOOL v ) {
    SetError( VP_ERR_REMOTE_SESSION );
    return FALSE;
}



BOOL CRemoteSession::UpdateSessionInfo( LPVOID data ) {
    SetError( VP_ERR_REMOTE_SESSION );
    return FALSE;
}


void CRemoteSession::OnConnect( CNetLink* link ) {
    VPENTER( CRemoteSession::OnConnect );
    DWORD err = link->LastError();

    if ( err ) {
        Log( "CRemoteSession::OnConnect got connection error" );
        if ( m_pendingJoin ) {

            m_pendingJoin->SetError( VP_ERR_NET_ERROR );
            PostNotification( m_pendingJoin );
            m_pendingJoin = NULL;
        }

        OnDisconnect( link );
        VPEXIT();
        return;
    }

    if ( m_serverWS && m_serverWS->m_safeLink == link ) {
        m_connected = TRUE;
        m_hasAltServer = FALSE;   // primary candidate connected — no fallback needed/wanted
        m_dialStart = 0;          // dial completed — disarm the OnTimer connect deadline
    }

    VPEXIT();

}

void CRemoteSession::OnAccept( CNetLink* link ) {
    CNetAddress* addr = link->GetRemoteAddress();
    CWS* ws = m_wsMap->FindByAddress( addr );


    if ( ws != NULL ) {
        // We have already a WS at this NetAddress
        ( (CRemoteWS*)ws )->SetSafeLink( link );
    } else {
        ws = MakeRemoteWS( addr, link, NULL );
        if ( ws ) {
            m_wsMap->Register( ws );
            ws->Unref();
        }
    }

    addr->Unref();

}


void CRemoteSession::OnDisconnect( CNetLink* link ) {
    VPENTER( CRemoteSession::OnDisconnect );
    LogV( m_log, "RemoteSession::OnDisconnect link %08lx", (DWORD)(uintptr_t)link );   // LP64-safe cast

    if ( link->m_pPartialMsg ) {   // drop any half-reassembled stream message
        link->m_pPartialMsg->Unref();
        link->m_pPartialMsg = NULL;
        link->m_partialGot = 0;
    }

    if ( m_broadcastLink == link ) {
        m_connected = FALSE;
        HandleNetDown();
        VPEXIT();
        return;
    }

    // Dial-both fallback: the PRIMARY candidate's connect dropped BEFORE we joined
    // (m_connected still FALSE — the async EINPROGRESS-then-fail that lands here for an
    // unreachable candidate). If ConnectToServer stashed an alternate, re-dial it ONCE
    // rather than stalling the join back to the menu. Never fires once connected (the flag
    // is cleared on connect) so it cannot disturb a live game's disconnect handling.
    if ( !m_connected && m_hasAltServer && m_serverWS && m_serverWS->m_safeLink == link ) {
        if ( JoinAddrLogOn() )
            fprintf( stderr, "[natcand] primary candidate connect dropped pre-join -> DEFERRING re-dial of stashed ALTERNATE candidate to next OnTimer\n" );
        m_hasAltServer  = FALSE;                // fall back only once
        m_redialPending = TRUE;                 // serviced on the next pump pass (OnTimer)
        // Tear down the failed primary WS now (same as the normal drop path below);
        // the actual ConnectToServer runs from OnTimer, NOT re-entrantly here — the
        // pump may still hold `link`, so dialing a fresh link inline risks UAF.
        CWS* dead = m_wsMap->FindBySafeLink( link );
        if ( dead )
            ( (CRemoteWS*)dead )->StopUsingSafeLink();
        m_serverWS = NULL;
        VPEXIT();
        return;
    }

    if ( m_connected && ( m_serverWS->m_safeLink == link ) ) {
        VPTRACE( ( "CRemoteSession::OnDisconnect - link to server broken" ) );
        m_connected = FALSE;

        // ROOT FIX (iserve/TCP-discovered join bail): m_tcpEnumTried means THIS is the
        // ENUM/discovery session and m_serverWS is the TCP-enum fallback probe to the
        // REG SERVER (TryTcpEnumFallback) — NOT a joined game host. When that probe link
        // drops (iserve exits after serving, or StopEnum tears the enum down), posting
        // VP_SESSIONCLOSE makes the app's OnMsgSessionClose (netapi.cpp:490) think the
        // joined game closed; since "me" is NULL mid-browse it calls theNet.Close(TRUE),
        // whose deferred vpCleanup DESTROYS the SHARED vp handle the subsequent Join needs
        // -> vpJoinSession(NULL) -> IDS_VPJOIN_FAILED. Functionally pinned by linux2's
        // m_vpHdl lifecycle trace @028bc7fc (Close(delay=1)->cleanup nulled the handle,
        // FatalError tracer 0 => VP_SESSIONCLOSE path, not VP_NETDOWN). A discovery-probe
        // teardown is benign — drop it quietly; never post a session-close for it. Real
        // join sessions (m_tcpEnumTried==FALSE; separate object) keep the notification.
        if ( m_tcpEnumTried ) {
            if ( JoinAddrLogOn() )
                fprintf( stderr, "[join-addr] OnDisconnect: enum TCP-fallback probe link dropped — suppressing VP_SESSIONCLOSE (it was destroying the shared join handle)\n" );
            VPEXIT();
            return;
        }

        CNotification* n = new CNotification( VP_SESSIONCLOSE, 0, 0, 0 );
        if ( !n ) {
            FatalError( VP_ERR_NOMEM );
        } else {
            PostNotification( n );
        }

        VPEXIT();
        return;
    }

    CWS* ws = m_wsMap->FindBySafeLink( link );
    if ( ws ) {
        // Host relay (spec 4b): this is a PEER link (the server link is handled
        // above), so its loss - a failed dial or a dropped p2p link - is the
        // direct-path failure that arms relay mode. Sticky: no retry, no
        // flapping. A peer that never fails never enters relay mode, which is
        // what makes the change a provable no-op on direct-capable pairs.
        if ( HostRelayOn() && ws != (CWS*)m_serverWS &&
             !( (CRemoteWS*)ws )->m_relayMode ) {
            ( (CRemoteWS*)ws )->m_relayMode = TRUE;
            RelayLog( "client: direct peer link lost -> RELAY MODE (sticky)" );
        }
        ( (CRemoteWS*)ws )->StopUsingSafeLink();
    }

    VPEXIT();
}



void CRemoteSession::ProcessSafeData( CNetLink* link, genericMsg* msg ) {
    BOOL unexpected = FALSE;

    switch ( msg->hdr.msgKind ) {
    default:
        unexpected = TRUE;
        break;

    case SenumREP:
        VPASSERT( m_serverWS->m_safeLink == link );
        OnSenumREP( (sesInfoMsg*)msg, m_serverWS );
        break;

    case JoinREP:
        VPASSERT( m_serverWS->m_safeLink == link );
        OnJoinREP( (plrInfoMsg*)msg, m_serverWS );
        break;

    case JoinNAK:
        VPASSERT( m_serverWS->m_safeLink == link );
        OnJoinNAK( (plrInfoMsg*)msg, m_serverWS );
        break;

    case JoinADV:
        VPASSERT( m_serverWS->m_safeLink == link );
        OnJoinADV( (plrInfoMsg*)msg, m_serverWS );
        break;


    case LeaveREQ:
        VPASSERT( m_serverWS->m_safeLink == link );
        OnLeaveREQ( (plrInfoMsg*)msg, m_serverWS );
        break;


    case LeaveREP:
        // we're silently ignoring this message
        break;


    case  PenumREP:
        VPASSERT( m_serverWS->m_safeLink == link );
        OnPenumREP( (plrInfoMsg*)msg, m_serverWS );
        break;

    case  UDataREQ:
        OnUDataREQ( msg, NULL );
        break;

    case  UBDataREQ:
        OnUBDataREQ( msg, NULL );
        break;

    case  FtREQ:
    {
        CRemoteWS* ws = (CRemoteWS*)m_wsMap->FindBySafeLink( link );

        if ( ws )
            OnFtREQ( (ftReqMsg*)msg, ws );
        break;
    }

    case  FtACK:
    {
        CRemoteWS* ws = (CRemoteWS*)m_wsMap->FindBySafeLink( link );

        if ( ws )
            OnFtACK( (ftAckMsg*)msg, ws );
        break;
    }
    case DummyREQ:
        break;

    }


    if ( unexpected )
        OnUnexpectedMsg( msg, link, TRUE );

}

//+ Process data coming from unsafe link
void CRemoteSession::OnUnsafeData( CNetLink* link ) {
    const DWORD maxReadSize = link->HasData();
    genericMsg* msg = new( (size_t)maxReadSize - sizeof( VPMSGHDR ) ) genericMsg;
    CNetAddress* addr = m_net->MakeAddress();
    DWORD count = link->ReceiveFrom( msg->Data(), maxReadSize, *addr );
    BOOL unexpected = FALSE;

    // iserve receive diagnostic (EN_ISERVE_LOG): proves the POSIX pump is actually
    // delivering datagram FD_READ events to the reg server's handler at all. If this
    // never logs while a datagram is sent to the bound :1707 socket, the socket isn't
    // being selected-for-read (pump/registration), NOT a parse/dispatch issue downstream.
    if ( IserveLogOn() )
        fprintf( stderr, "[iserve] OnUnsafeData ENTER (pump delivered a datagram FD_READ) count=%lu\n",
                 (unsigned long)count );

    if ( count < sizeof( VPMSGHDR ) ) {
        msg->Unref();
        return;
    }


    if ( count < msg->Size() )  // ignore badly formatted messages
    {
        msg->Unref();
        return;
    }

    PunchNoteTraffic( m_punch, addr );

    // iserve receive diagnostic (EN_ISERVE_LOG): the reg server uses this handler. Shows
    // whether the host's registration datagram ARRIVES here and with what msgKind — the reg
    // server only registers SenumREP. If this never logs, the datagram isn't reaching the
    // reg server's UDP link (bind/route); if it logs a non-SenumREP kind, the host is sending
    // the wrong message type for directed registration.
    if ( IserveLogOn() )
        fprintf( stderr, "[iserve] OnUnsafeData RX datagram msgKind=%d size=%lu (SenumREP=%d SenumREQ=%d)\n",
                 (int)msg->hdr.msgKind, (unsigned long)count, (int)SenumREP, (int)SenumREQ );

    switch ( msg->hdr.msgKind ) {
    case  UDataREQ:
    case  UBDataREQ:
    {
        // we're going to ignore messages from unknown workstations
        CWS* ws = m_wsMap->FindByAddress( addr );
        if ( !ws ) break;

        // This is a message from known workstation
        if ( msg->hdr.msgKind == UDataREQ ) {
            OnUDataREQ( msg, (CRemoteWS*)ws );
        } else {
            OnUBDataREQ( msg, (CRemoteWS*)ws );
        }

        break;
    }

    case SenumREP:
    {
        sesInfoMsg* siMsg = (sesInfoMsg*)msg;

        // P0.1: the registration server stamps the datagram's OBSERVED source
        // (the host's public NAT mapping for its game dg socket) into the
        // sessionId tail before storing/serving it. No-op for a game client
        // receiving enum replies (base impl is empty).
        StampRegistration( siMsg, addr );

        CNetAddress* addr2 = m_net->MakeAddress( &siMsg->data.sessionId );

        if ( !addr2 )
            break;
        CRemoteWS* ws = MakeRemoteWS( addr2, NULL, NULL );
        addr2->Unref();
        if ( !ws )
            break;



        OnSenumREP( siMsg, ws );
        if ( IserveLogOn() )
            fprintf( stderr, "[iserve] SenumREP processed (host registration) -> registry now %d session(s)\n",
                     (int)m_wsMap->Count() );
        ws->Unref();
        break;
    }

    case  SenumREQ:
    {
        CRemoteWS* ws = MakeRemoteWS( addr, NULL, link );
        if ( !ws )
            break;
        OnSenumREQ( msg, (CRemoteWS*)ws );
        ws->Unref();
        break;
    }

    case DummyREQ:
        break;

    case NatPunchREQ:
        // Only the registration server acts on this (virtual override); a game
        // client ignores stray REQs. Always dispatched (not EN_NAT_PUNCH-gated)
        // so iserve serves punch-enabled clients without its own env setup.
        if ( msg->ContentSize() >= sizeof( natPunchInfo ) )
            OnNatPunchREQ( (natPunchMsg*)msg, addr );
        break;

    case NatPunchREP:
        if ( NatPunchOn() && msg->ContentSize() >= sizeof( natPunchInfo ) )
            OnNatPunchREP( (natPunchMsg*)msg, addr );
        break;

    case NatPunchPING:
        if ( NatPunchOn() && msg->ContentSize() >= sizeof( natPunchInfo ) )
            PunchHandlePing( (natPunchMsg*)msg, addr, m_punch );
        break;

    case NatPunchPONG:
        if ( NatPunchOn() && msg->ContentSize() >= sizeof( natPunchInfo ) )
            PunchHandlePong( (natPunchMsg*)msg, addr, m_punch );
        break;

    default:
        unexpected = TRUE;
        break;
    }


    if ( unexpected )
        OnUnexpectedMsg( msg, link, FALSE );

    msg->Unref();
    addr->Unref();
}


BOOL CRemoteSession::ConnectToServer( LPCVPNETADDRESS addr, LPVOID userData ) {
    // P0.1 candidate selection: when the sessionId carries iserve's stamped
    // observed-address tail (vpnatcand.h), pick which candidate to dial.
    //   - stamped public IP == payload private IP  -> host isn't NAT'd: private.
    //   - SAMENAT flag (iserve saw us and the host from the SAME public IP)
    //                                              -> same household: private.
    //   - otherwise -> the private address is meaningless from here; dial the
    //     PUBLIC candidate {observed IP, claimed stream port, observed dg port}
    //     and (EN_NAT_PUNCH) start the UDP rendezvous in parallel.
    // No tail (old iserve / LAN broadcast discovery) -> exactly the old path.
    // Dial-both fallback: when the stamped session offers TWO distinct candidates
    // (observed PUBLIC + advertised PRIVATE), dial the heuristic primary now and stash
    // the OTHER as m_altServerAddr; OnDisconnect re-dials it if the primary connect drops
    // before we join. Stored alternates have the NAT tail stripped so the re-dial goes
    // straight to the transport address (no re-selection / no loop). Reset any stale alt.
    m_hasAltServer = FALSE;
    VPNETADDRESS dial;
    if ( addr && m_net->IsInetTransport() && NatCandOn() && EnNatCandPresent( addr ) ) {
        const unsigned char* pubIp = EnNatCandPubIp( addr );
        const unsigned char* privIp = EnNatCandPrivIp( addr );
        BOOL sameNat = ( EnNatCandFlags( addr ) & EN_NATCAND_F_SAMENAT ) != 0;
        BOOL twoCand = !EnNatCandIpZero( pubIp ) && !EnNatCandIpEq( pubIp, privIp );

        if ( twoCand && !sameNat ) {
            // Host is NAT'd: primary = PUBLIC; alternate = the advertised PRIVATE (raw addr,
            // tail stripped) in case the observed mapping is stale but we share a LAN.
            EnNatCandPublicAddr( addr, &dial );
            m_altServerAddr = *addr;
            memset( EnNatCandBytes( &m_altServerAddr ) + EN_NATCAND_OFF, 0, 12 );
            m_altServerUserData = userData;
            m_hasAltServer = TRUE;

            if ( JoinAddrLogOn() ) {
                char pb[64];
                EnNatCandFmt( pb, sizeof( pb ), pubIp, EnNatCandPubDgPort( addr ) );
                fprintf( stderr, "[natcand] host is NAT'd (observed %s != advertised private): dialing PUBLIC candidate (PRIVATE stashed as fallback)\n", pb );
            }

            if ( NatPunchOn() )
                StartNatPunch( addr );

            addr = &dial;
        } else if ( twoCand ) {
            // sameNat (or host-not-NAT'd but a distinct public exists): primary = PRIVATE
            // (hairpin/LAN), alternate = the observed PUBLIC. A DOUBLE-NAT'd joiner that
            // shares the host's public IP still cannot reach the host's private LAN address,
            // so keep PUBLIC as the fallback rather than dead-ending at the menu.
            EnNatCandPublicAddr( addr, &m_altServerAddr );   // already tail-free
            m_altServerUserData = userData;
            m_hasAltServer = TRUE;

            if ( JoinAddrLogOn() )
                fprintf( stderr, "[natcand] dialing PRIVATE candidate (%s) - PUBLIC stashed as fallback\n",
                         sameNat ? "same public IP as host" : "host not NAT'd" );

            if ( NatPunchOn() )
                StartNatPunch( addr );
        } else if ( JoinAddrLogOn() ) {
            fprintf( stderr, "[natcand] dialing PRIVATE candidate (no distinct public candidate)\n" );
        }
    }

    if ( addr && JoinAddrLogOn() ) {
        char abuf[64];
        FormatVpAddr( abuf, sizeof( abuf ), addr );
        fprintf( stderr, "[join-addr] ConnectToServer dialing %s\n", abuf );
    }
    CNetAddress* nA = m_net->MakeAddress( addr );
    CNetLink* link = m_net->MakeSafeLink( nA, userData );

    // MakeSafeLink returns NULL when the TCP socket/connect setup fails (an
    // unreachable or bad server address). The original code then still built a WS
    // around the NULL safe link and dereferenced it at `link->Unref()` below ->
    // SEGV (the join crash linux2 hit at vpengine.cpp:1893). Bail cleanly instead.
    if ( !link ) {
        Log( "CRemoteSession::ConnectToServer - MakeSafeLink failed (bad/unreachable addr)" );
        if ( JoinAddrLogOn() )
            fprintf( stderr, "[join-addr] ConnectToServer - MakeSafeLink FAILED (bad/unreachable addr) -> IDS_VPJOIN_FAILED\n" );
        if ( nA )
            nA->Unref();
        m_serverWS = NULL;
        return FALSE;
    }

    m_serverWS = MakeRemoteWS( nA, link, NULL );

    if ( m_serverWS ) {
        m_dialStart = GetCurrentTime();   // arm the OnTimer connect deadline
        if ( !m_dialStart )
            m_dialStart = 1;
        m_wsMap->Register( m_serverWS );

        link->Unref();
        nA->Unref();
    }
    return m_serverWS != NULL;
}


// P1, client role: kick the rendezvous. The REQ goes to iserve FROM our dg
// socket — the same socket the session will play on, so the mapping iserve
// observes for us is the one the host must punch. Runs in parallel with the
// TCP dial; retried from DrivePunch until REP arrives.
void CRemoteSession::StartNatPunch( LPCVPNETADDRESS sessionId ) {
    // Remember what we are punching for OUTSIDE m_punch: Reset() memsets it, so
    // DrivePunch could not otherwise re-arm a dead pair (board ruling 2026-08-23).
    m_punchSessionId = *sessionId;
    m_punchWanted    = TRUE;
    m_punchLastArm   = GetCurrentTime();

    m_punch.Reset();
    m_punch.m_sessionId = *sessionId;
    m_punch.m_nonce = ( GetCurrentTime() * 2654435761u ) ^ (DWORD)(size_t)this;
    if ( !m_punch.m_nonce )
        m_punch.m_nonce = 1;
    m_punch.m_state = PunchPeer::WAIT_REP;

    natPunchMsg req( NatPunchREQ );
    req.data.sessionId = *sessionId;
    req.data.nonce = m_punch.m_nonce;
    // Private candidate: our own station address + bound ports, same source the
    // host advertises about itself (GetAddress only fills the transport prefix
    // of the zeroed overlay).
    m_net->GetAddress( &req.data.privCand );
    // pubCand stays zero — iserve fills it with what it OBSERVES from us.

    CNetAddress* reg = m_net->MakeServerLookupAddress();
    if ( !reg ) {
        PunchLog( "client: no reg-server address - punch unavailable" );
        m_punch.Reset();
        return;
    }

    VPNETADDRESS regA;
    memset( &regA, 0, sizeof( regA ) );
    reg->GetNormalForm( &regA );
    reg->Unref();

    m_punch.m_lastSend = GetCurrentTime();
    m_punch.m_tries = 1;
    SendDgTo( &regA, req.Data(), req.Size() );
    PunchLog( "client: NatPunchREQ sent to reg server (nonce=%08lx)", (unsigned long)m_punch.m_nonce );
}


// P1, client role: iserve answered with the host's candidate pair — probe both.
void CRemoteSession::OnNatPunchREP( natPunchMsg* msg, CNetAddress* from ) {
    if ( m_punch.m_state != PunchPeer::WAIT_REP || msg->data.nonce != m_punch.m_nonce )
        return;

    m_punch.m_pub = msg->data.pubCand;
    m_punch.m_priv = msg->data.privCand;
    m_punch.m_state = PunchPeer::PROBING;
    m_punch.m_tries = 0;

    char pb[64], vb[64];
    EnNatCandFmt( pb, sizeof( pb ), EnNatCandBytes( &m_punch.m_pub ), EnNatCandBytes( &m_punch.m_pub ) + 6 );
    EnNatCandFmt( vb, sizeof( vb ), EnNatCandBytes( &m_punch.m_priv ), EnNatCandBytes( &m_punch.m_priv ) + 6 );
    PunchLog( "client: host candidates pub=%s priv=%s%s -> probing", pb, vb,
              ( msg->data.flags & EN_NATCAND_F_SAMENAT ) ? " (same public IP)" : "" );

    PunchFireProbes( m_punch );
}


// Client-side punch pacing: REQ resends, probe retries, then keepalives that
// hold both NATs' mappings open (host answers each PING with a PONG).
void CRemoteSession::DrivePunch() {
    if ( !NatPunchOn() )
        return;

    DWORD t = GetCurrentTime();

    if ( m_punch.m_state == PunchPeer::IDLE ) {
        // Re-punch ladder. Once a pair has ever been wanted, keep trying for the
        // life of the session: one REQ every 30s, no backoff machinery. Bounded
        // chatter and nothing to lose - without this a pair lost to a transient
        // outage stayed lost, since StartNatPunch is otherwise only reachable
        // from ConnectToServer (board ruling 2026-08-23).
        if ( m_punchWanted && t - m_punchLastArm > 30000 ) {
            VPSESSIONID sid = m_punchSessionId;   // copy: StartNatPunch resets state
            PunchLog( "client: re-arming punch (30s ladder)" );
            StartNatPunch( &sid );
        }
        return;
    }

    switch ( m_punch.m_state ) {
    case PunchPeer::WAIT_REP:
        if ( m_punch.m_tries >= 5 ) {
            PunchLog( "client: no NatPunchREP from reg server after 5 tries - giving up" );
            m_punch.Reset();
        } else if ( t - m_punch.m_lastSend > 1000 ) {
            // Resend the REQ with the SAME nonce (StartNatPunch would reset the
            // try counter and never hit the give-up above).
            natPunchMsg req( NatPunchREQ );
            req.data.sessionId = m_punch.m_sessionId;
            req.data.nonce = m_punch.m_nonce;
            m_net->GetAddress( &req.data.privCand );

            CNetAddress* reg = m_net->MakeServerLookupAddress();
            if ( reg ) {
                VPNETADDRESS regA;
                memset( &regA, 0, sizeof( regA ) );
                reg->GetNormalForm( &regA );
                reg->Unref();
                SendDgTo( &regA, req.Data(), req.Size() );
            }
            m_punch.m_tries++;
            m_punch.m_lastSend = t;
        }
        break;

    case PunchPeer::PROBING:
        if ( m_punch.m_tries >= 12 ) {
            PunchLog( "client: punch FAILED (12 probe rounds, no round-trip) - symmetric NAT or filtered path" );
            m_punch.Reset();
        } else if ( t - m_punch.m_lastSend > 300 ) {
            PunchFireProbes( m_punch );
        }
        break;

    case PunchPeer::CONFIRMED:
        if ( t - m_punch.m_lastSend > 15000 ) {
            natPunchMsg ping( NatPunchPING );
            ping.data.nonce = m_punch.m_nonce;
            SendDgTo( &m_punch.m_confirmed, ping.Data(), ping.Size() );
            m_punch.m_lastSend = t;
        }
        // Three keepalive intervals of silence => the pair is dead. Re-arm with a
        // FRESH nonce rather than Reset()ing terminally; the 5x1s WAIT_REP ladder
        // and then the 30s IDLE ladder above take it from here.
        if ( t - m_punch.m_lastAlive > 45000 ) {
            VPSESSIONID sid = m_punchSessionId;   // copy: StartNatPunch resets state
            PunchLog( "client: punched pair silent 45s -> re-arming with a fresh nonce" );
            StartNatPunch( &sid );
        }
        break;
    }
}


void CRemoteSession::OnTimer() {
    // Bounded pre-join dial: a candidate behind a DROP firewall/NAT hangs in
    // SYN retries for the OS budget (21s Windows / up to 127s Linux) and never
    // errors the socket inside the UI's 8s join guard, so the OnDisconnect
    // dial-both fallback below was dead code for dropped ports (UbuntuOpus
    // root cause, board 2026-08-22). Expire the pending dial here and route it
    // through OnDisconnect on this clean pump pass: with an alternate stashed
    // it re-dials, without one it tears down honestly instead of hanging.
    enum { EN_DIAL_DEADLINE_MS = 3500 };
    if ( !m_connected && m_dialStart && m_serverWS && m_serverWS->m_safeLink &&
         GetCurrentTime() - m_dialStart > EN_DIAL_DEADLINE_MS ) {
        m_dialStart = 0;
        if ( JoinAddrLogOn() )
            fprintf( stderr, "[natcand] pending connect exceeded %ums app deadline -> forcing failure (OS SYN budget would outlive the join window)\n",
                     (unsigned)EN_DIAL_DEADLINE_MS );
        OnDisconnect( m_serverWS->m_safeLink );
    }

    // Deferred dial-both fallback: a pre-join primary-candidate drop stashed an
    // alternate in OnDisconnect; dial it here, on a clean pump pass (never inline
    // from the disconnect callback). Single-shot — m_redialPending is one-way false.
    if ( m_redialPending ) {
        m_redialPending = FALSE;
        VPNETADDRESS alt = m_altServerAddr;
        LPVOID       ud  = m_altServerUserData;
        BOOL ok = ConnectToServer( &alt, ud );
        if ( !ok && JoinAddrLogOn() )
            fprintf( stderr, "[natcand] deferred re-dial of ALTERNATE candidate FAILED synchronously (no reachable candidate left) -> join will dead-end\n" );
    }
    DrivePunch();
}


CRegisterySession::CRegisterySession( CTDLogger* log, CNetInterface* net, CPlayerMap* players, CWSMap* wsMap, DWORD maxAge ):
    CRemoteSession( log, net, players, wsMap, maxAge ), m_queryAge( maxAge / 2 ) {}


BOOL CRegisterySession::LookForServer( LPVOID data ) {

    AgeParams p;

    m_serverEnumData = data;
    genericMsg* msg = new( 0 ) genericMsg( SenumREQ, 0 );

    if ( !msg )
        return FALSE;

    p.curTime = GetCurrentTime();
    p.maxAge = m_queryAge;




    m_wsMap->Enum( CheckForAge, &p );


    sendDataInfo info( msg->Data(), msg->Size(), 0, NULL, this );

    WSLink* wsl;

    while ( NULL != ( wsl = p.list.RemoveFirst() ) ) {
        CRemoteWS* ws = (CRemoteWS*)( wsl->m_data );

        ws->SendData( info );

        delete wsl;
    }


    msg->Unref();

    AgeServerList();
    return TRUE;

}


struct EnumRepParams {
    CVpSession* session;
    CRemoteWS* ws;
    WORD       msgId;
};


int CRegisterySession::ReplyServerInfo( CWS* ws, LPVOID data ) {
    EnumRepParams& p = *(EnumRepParams*)data;

    if ( ws->IsRemote() ) {
        CRemoteWS* rws = (CRemoteWS*)ws;
        sesInfoMsg* msg = rws->m_info;

        // NULL-guard (linux2 gdb bt 2026-06-26 — the iserve post-serve SIGSEGV at
        // vpengine.cpp:2206). m_wsMap can hold a bare transport ws with no registered
        // session info: a client's own TCP-enum query connection, or a half-open probe
        // that hasn't sent its SenumREP yet. Enum() hits it while serving a client query
        // and the msg->hdr deref below dereferences NULL -> SIGSEGV (crashes iserve after
        // it served, an iserve-on-VPS availability bug). It has nothing to relay, so skip
        // it and continue the enumeration. Parallel to the AgeServerList NULL-Info guard
        // @b4fef157; game<->game play is unaffected (already proven), this hardens iserve.
        if ( !msg )
            return TRUE;

        msg->hdr.msgId = p.msgId;

        // TCP-enum reply-leg (linux1 root-cause 2026-06-26): reply over the SAME link the
        // query arrived on. If the querying WS reached us over TCP (m_safeLink set — a
        // TCP-enum query), send the SenumREP back over that TCP stream (VP_MUSTDELIVER ->
        // CRemoteWS::SendData uses the safe link); a UDP-arriving query (m_safeLink NULL)
        // keeps the datagram reply. The old unconditional flags=0 always replied over UDP,
        // so a TCP-enum query under a UDP block never got its reply -> empty browser.
        DWORD replyFlags = ( p.ws->m_safeLink ) ? VP_MUSTDELIVER : 0;

        // P0.1 serve-time same-NAT hint: when THIS requester's observed IP
        // equals the host's stamped public IP, both sit behind the same NAT —
        // the public candidate would need router hairpinning (often broken),
        // so flag the reply to make the joiner dial the private candidate.
        // Per-request, so it must go into a COPY, never the stored m_info.
        char cbuf[VP_MAXSENDDATA];
        LPVOID sendData = msg->Data();
        if ( msg->Size() <= sizeof( cbuf ) && p.ws->m_address &&
             EnNatCandPresent( &msg->data.sessionId ) ) {
            VPNETADDRESS req;
            memset( &req, 0, sizeof( req ) );
            p.ws->m_address->GetNormalForm( &req );
            if ( EnNatCandIpEq( EnNatCandBytes( &req ), EnNatCandPubIp( &msg->data.sessionId ) ) ) {
                memcpy( cbuf, msg->Data(), (size_t)msg->Size() );
                VPSESSIONINFO* si = (VPSESSIONINFO*)( cbuf + sizeof( VPMSGHDR ) );
                EnNatCandSetFlags( &si->sessionId,
                                   (unsigned char)( EnNatCandFlags( &si->sessionId ) | EN_NATCAND_F_SAMENAT ) );
                sendData = cbuf;
            }
        }

        sendDataInfo info( sendData, msg->Size(), replyFlags, NULL, p.session );

        p.ws->SendData( info );
    }

    return TRUE;
}


// Count only genuinely REGISTERED sessions: ws entries carrying session info (an
// SenumREP landed). m_wsMap also holds bare transport entries — every enum REQUESTER
// gets a MakeRemoteWS before OnSenumREQ, plus TCP probes (see the ReplyServerInfo
// NULL-guard) — so raw Count() inflates with each browsing client, and a map holding
// only such phantoms skipped the no-sessions dummy reply below, leaving the client
// with no answer at all.
static int CountRegisteredSession( CWS* ws, LPVOID data ) {
    if ( ws->IsRemote() && ( (CRemoteWS*)ws )->m_info )
        ++*(int*)data;
    return TRUE;
}

BOOL CRegisterySession::OnSenumREQ( genericMsg* msg, CRemoteWS* ws ) {

    int nRegistered = 0;
    m_wsMap->Enum( CountRegisteredSession, &nRegistered );

    if ( IserveLogOn() )
        fprintf( stderr, "[iserve] sEnumREQ from a client -> serving %d registered session(s) (%d map entries)\n",
                 nRegistered, (int)m_wsMap->Count() );

    if ( !nRegistered ) {
        // send a dummy reply so the client will see something coming
        // from us; reply over the query's own link (TCP if it arrived over TCP, else UDP)
        // — same reason as the real SenumREP above.
        genericMsg msg( DummyREQ, 0 );
        DWORD replyFlags = ( ws->m_safeLink ) ? VP_MUSTDELIVER : 0;
        sendDataInfo info( msg.Data(), msg.Size(), replyFlags, NULL, this );
        ws->SendData( info );
        return TRUE;

    }


    EnumRepParams p;

    p.session = this;
    p.ws = ws;
    p.msgId = msg->hdr.msgId;



    m_wsMap->Enum( ReplyServerInfo, &p );

    return TRUE;
}


// TCP-enum (phase-1, mac 2026-06-26): the reg server's safe-link (TCP) dispatch.
// Today enum rides UDP (OnUnsafeData); this lets a client's directed SenumREQ arrive
// over a TCP connection instead (for UDP-blocked routers / tunnels). The reply rides
// back over the same TCP link (OnSenumREQ -> ws->SendData is link-agnostic). All other
// safe-link kinds (host-register SenumREP, etc.) delegate to the base. Additive: this
// only fires once a TCP-enum listener exists (step-2 of phase-1) — the UDP enum path
// is unchanged and remains the default.
void CRegisterySession::ProcessSafeData( CNetLink* link, genericMsg* msg ) {
    if ( msg->hdr.msgKind == SenumREQ ) {
        CRemoteWS* ws = (CRemoteWS*)m_wsMap->FindBySafeLink( link );
        if ( ws )
            OnSenumREQ( msg, ws );
        return;
    }
    CRemoteSession::ProcessSafeData( link, msg );
}


// P0.1: a registration datagram's OBSERVED UDP source is the host's public NAT
// mapping for its game dg socket (registrations are sent from that socket).
// Stamp it into the sessionId tail before the message is stored — the registry
// then serves both candidates to every joiner with no wire-format change, and
// a host can never spoof its public candidate (we always overwrite the tail).
void CRegisterySession::StampRegistration( sesInfoMsg* msg, CNetAddress* observed ) {
    if ( !m_net->IsInetTransport() || !observed )
        return;

    VPNETADDRESS obs;
    memset( &obs, 0, sizeof( obs ) );
    observed->GetNormalForm( &obs );

    // Observed overlay (tcpaddress_s): IP at [0..3], source (dg) port at [6..7].
    const unsigned char* b = EnNatCandBytes( &obs );
    EnNatCandStamp( &msg->data.sessionId, b, b + 6 );

    if ( IserveLogOn() ) {
        char ab[64];
        EnNatCandFmt( ab, sizeof( ab ), b, b + 6 );
        fprintf( stderr, "[iserve] registration stamped: observed public candidate %s\n", ab );
    }
}


// P1: stateless rendezvous. Look the target session up, tell the host about
// the joiner (through the host's warm registration mapping — no inbound
// connection needed), tell the joiner about the host. No punch state is kept
// at the registry at all.
void CRegisterySession::OnNatPunchREQ( natPunchMsg* msg, CNetAddress* from ) {
    if ( !m_net->IsInetTransport() || !from )
        return;

    // Key the lookup by the session's transport address (IsEqual ignores the
    // candidate tail).
    CNetAddress* key = m_net->MakeAddress( &msg->data.sessionId );
    if ( !key )
        return;
    CWS* found = m_wsMap->FindByAddress( key );
    key->Unref();

    if ( !found || !found->IsRemote() || !( (CRemoteWS*)found )->m_info ) {
        if ( IserveLogOn() )
            fprintf( stderr, "[iserve] NatPunchREQ for an unknown/unregistered session - dropped\n" );
        return;
    }

    const VPNETADDRESS& hostId = ( (CRemoteWS*)found )->m_info->data.sessionId;
    if ( !EnNatCandPresent( &hostId ) ) {
        if ( IserveLogOn() )
            fprintf( stderr, "[iserve] NatPunchREQ: host registration carries no observed stamp (pre-P0.1 host?) - dropped\n" );
        return;
    }

    VPNETADDRESS cliObs;
    memset( &cliObs, 0, sizeof( cliObs ) );
    from->GetNormalForm( &cliObs );
    const unsigned char* cb = EnNatCandBytes( &cliObs );

    DWORD flags = 0;
    if ( EnNatCandIpEq( cb, EnNatCandPubIp( &hostId ) ) )
        flags |= EN_NATCAND_F_SAMENAT;

    // Joiner's public candidate: observed IP + its self-claimed stream port +
    // observed (dg) source port.
    VPNETADDRESS cliPub;
    memset( &cliPub, 0, sizeof( cliPub ) );
    unsigned char* cp = EnNatCandBytes( &cliPub );
    cp[0] = cb[0]; cp[1] = cb[1]; cp[2] = cb[2]; cp[3] = cb[3];
    cp[4] = EnNatCandBytes( &msg->data.privCand )[4];
    cp[5] = EnNatCandBytes( &msg->data.privCand )[5];
    cp[6] = cb[6]; cp[7] = cb[7];

    // -> host, at its OBSERVED public endpoint, from our :1707 socket (the
    //    only source its NAT filter is guaranteed to admit).
    natPunchMsg fwd( NatPunchFWD );
    fwd.data.sessionId = msg->data.sessionId;
    fwd.data.pubCand = cliPub;
    fwd.data.privCand = msg->data.privCand;
    fwd.data.nonce = msg->data.nonce;
    fwd.data.flags = flags;

    VPNETADDRESS hostObs;
    memset( &hostObs, 0, sizeof( hostObs ) );
    unsigned char* hp = EnNatCandBytes( &hostObs );
    const unsigned char* hip = EnNatCandPubIp( &hostId );
    const unsigned char* hdp = EnNatCandPubDgPort( &hostId );
    hp[0] = hip[0]; hp[1] = hip[1]; hp[2] = hip[2]; hp[3] = hip[3];
    hp[6] = hdp[0]; hp[7] = hdp[1];
    SendDgTo( &hostObs, fwd.Data(), fwd.Size() );

    // -> joiner: the host's dialable candidate pair.
    natPunchMsg rep( NatPunchREP );
    rep.data.sessionId = hostId;
    EnNatCandPublicAddr( &hostId, &rep.data.pubCand );
    memcpy( &rep.data.privCand, &hostId, 8 );   // transport prefix = private candidate
    rep.data.nonce = msg->data.nonce;
    rep.data.flags = flags;
    SendDgTo( &cliObs, rep.Data(), rep.Size() );

    if ( IserveLogOn() ) {
        char ha[64], ca[64];
        EnNatCandFmt( ha, sizeof( ha ), hip, hdp );
        EnNatCandFmt( ca, sizeof( ca ), cb, cb + 6 );
        fprintf( stderr, "[iserve] punch rendezvous: joiner %s <-> host %s%s\n",
                 ca, ha, ( flags & EN_NATCAND_F_SAMENAT ) ? " (same public IP)" : "" );
    }
}






CPlayer::CPlayer( LPVPPLAYERINFO pInfo, CWS* ws ): m_ws( ws ), m_info( NULL ) {
    if ( pInfo ) {
        m_info = new( (size_t)pInfo->dataSize ) plrInfoMsg( (size_t)pInfo->dataSize );
        if ( m_info ) {
            _fmemcpy( &m_info->data, pInfo, vpPlayerInfoSize( pInfo ) );
        }
    }
}

CPlayer::CPlayer( plrInfoMsg* pInfoMsg, CWS* ws ): m_ws( ws ), m_info( pInfoMsg ) {
    if ( m_info )
        m_info->Ref();
}


CPlayer::~CPlayer() {
    if ( m_ws ) {
        m_ws->Unref();
        m_ws = NULL;
    }

    if ( m_info ) {
        m_info->Unref();
        m_info = NULL;
    }
}




BOOL CRemotePlayer::SendData( sendDataInfo& sdi ) {
    VPASSERT( sdi.m_ctxData );
    VPASSERT( m_ws );

    CVpSession& ses = *(CVpSession*)sdi.m_ctxData;


    BOOL r = m_ws->SendData( sdi );
    if ( !r ) {
        ses.Log( "CRemote Player: error sending data" );
        ses.SetError( VP_ERR_NET_ERROR, m_ws->GetLastError() );
        return FALSE;
    }
    return r;
}




// Host relay (spec 4b), client side. The fallback is hooked HERE rather than in
// CVpSession::SendData's unicast branch for two reasons: this is the single
// point EVERY unicast user funnels through (game messages and vpxfer file
// transfer alike), and the message header is already fully formed, so the relay
// needs no re-addressing. It cannot change host behavior or gate-off behavior:
// m_relayMode is only ever set on a CLIENT's peer WS (never on the server WS),
// and both the flag and the gate are checked before anything diverges — so the
// recursion below is one level deep at most and unreachable on a host.
BOOL CRemoteWS::SendData( sendDataInfo& sdi ) {
    CVpSession* relaySes = ( m_relayMode && HostRelayOn() ) ? (CVpSession*)sdi.m_ctxData : NULL;

    if ( relaySes && relaySes->IsRemote() ) {
        CRemoteWS* srv = ( (CRemoteSession*)relaySes )->m_serverWS;

        if ( srv && srv != this )
            return srv->SendData( sdi );
    }

    if ( sdi.m_sendFlags & VP_MUSTDELIVER ) {
        if ( !m_safeLink ) {
            CVpSession* ses = (CVpSession*)sdi.m_ctxData;

            m_safeLink = ses->MakeSafeLink( m_address );
            if ( !m_safeLink ) {
                // No direct link to this peer can even be set up. Flip to relay
                // mode (sticky for the session, no retries - spec 4b) and put
                // THIS message through the host rather than dropping it.
                if ( HostRelayOn() && ses && ses->IsRemote() && !m_relayMode ) {
                    CRemoteWS* srv = ( (CRemoteSession*)ses )->m_serverWS;

                    if ( srv && srv != this ) {
                        m_relayMode = TRUE;
                        RelayLog( "client: MakeSafeLink to peer WS failed -> RELAY MODE (sticky)" );
                        return srv->SendData( sdi );
                    }
                }
                return FALSE;
            }

        }
        return m_safeLink->Send( sdi.m_data, sdi.m_dataSize, sdi.m_sendFlags );
    }

    return m_unsafeLink->SendTo( *m_address, sdi.m_data, sdi.m_dataSize,
                                 sdi.m_sendFlags );
}


CRemoteWS::~CRemoteWS() {
    if ( m_safeLink )
        m_safeLink->Unref();

    if ( m_unsafeLink )
        m_unsafeLink->Unref();

    if ( m_address )
        m_address->Unref();

    if ( m_info )
        m_info->Unref();

}


CNotification::CNotification( WORD msgCode,
                              LPVOID data,
                              DWORD dataLen,
                              LPVOID userData ) {
    m_vpmsg.notificationCode = msgCode;
    m_vpmsg.u.data = data;
    m_vpmsg.dataLen = dataLen;
    m_vpmsg.userData = userData;
    m_vpmsg.creationTime = vpMsgTime();
    m_vpmsg.postTime = 0;
    m_vpmsg.createMsTime = timeGetTime();
}

CNotification::~CNotification() {}


