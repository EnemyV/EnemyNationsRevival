// CSockets — portable (winsock/BSD) TCP+UDP CProtocol implementation.
// See sockets.h and docs/plans/multiplayer-cross-platform.md.
//
// STATUS: TCP core increment. Lifecycle + TCP sessions are real:
//   Listen/Call/Send/Receive/HangUp over length-framed TCP, a background select()
//   net thread, and the NETMSG completion queue (replacing the dead WM_NET_COMPLETE
//   path) drained via DrainCompletions/PopCompletion. Verified by SelfTest() (loopback
//   Listen→Call→Send→Receive round-trip). UDP datagrams + LAN discovery (SendDatagram/
//   ReceiveDatagram) and the game-side net-path wiring are the following increments.
//
// Name model (mirrors CNetbios): a process-global name->port registry resolves logical
// names to a loopback endpoint for same-host play / the self-test; "a.b.c.d[:port]"
// strings resolve directly (direct-IP join). Cross-host name discovery comes with UDP.

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include "stdafx.h"
#include "sockets.h"

#include <cstring>
#include <cstdlib>
#include <chrono>
#include <utility>
#include <vector>

#ifndef _WIN32
  #include <cerrno>
  #include <fcntl.h>
#endif

// ---- platform glue ----------------------------------------------------------
static void en_closesock ( en_socket_t & s )
{
    if ( s != EN_INVALID_SOCKET )
    {
#ifdef _WIN32
        closesocket ( s );
#else
        close ( s );
#endif
        s = EN_INVALID_SOCKET;
    }
}

static void en_setnonblock ( en_socket_t s )
{
#ifdef _WIN32
    u_long m = 1;
    ioctlsocket ( s, FIONBIO, &m );
#else
    int f = fcntl ( s, F_GETFL, 0 );
    if ( f >= 0 )
        fcntl ( s, F_SETFL, f | O_NONBLOCK );
#endif
}

static bool en_would_block ()
{
#ifdef _WIN32
    return ( WSAGetLastError () == WSAEWOULDBLOCK );
#else
    return ( errno == EWOULDBLOCK || errno == EAGAIN );
#endif
}

// ---- process-global logical-name registry (name -> loopback TCP/UDP ports) --
// Lets same-host peers / the self-test resolve a name to a concrete endpoint
// without LAN broadcast discovery yet (cross-host discovery is the UDP path).
struct EnNameEntry { unsigned short tcp = 0, udp = 0; };
static std::mutex                              s_regLock;
static std::map<std::string, EnNameEntry>     s_nameReg;

static std::string en_namekey ( LPCSTR pName )
{
    std::string k;
    if ( pName )
        for ( int i = 0; i < NET_NAME_MAX && pName[i] && pName[i] != ' '; ++i )
            k.push_back ( pName[i] );
    return ( k );
}

// ---- socket library init (winsock needs WSAStartup; POSIX is a no-op) --------
BOOL CSockets::InitSocketLib ()
{
#ifdef _WIN32
    static bool s_started = false;
    if ( !s_started )
    {
        WSADATA wsa;
        if ( WSAStartup ( MAKEWORD ( 2, 2 ), &wsa ) != 0 )
            return ( FALSE );
        s_started = true;
    }
#endif
    return ( TRUE );
}

BOOL CSockets::Have ()
{
    return ( TRUE );  // sockets are always available
}

CSockets::CSockets ( HINSTANCE hInst, HWND hWnd )
{
    m_hInst = hInst;
    m_hWnd  = hWnd;
    m_ID    = NET_PROTO_TCP;
    m_listen = EN_INVALID_SOCKET;
    m_udp    = EN_INVALID_SOCKET;
}

CSockets::~CSockets ()
{
    Close ();
}

BOOL CSockets::InitOk ()
{
    return ( InitSocketLib () );
}

void CSockets::Close ()
{
    // stop the net thread first so it stops touching the session table
    m_run = false;
    if ( m_netThread.joinable () )
        m_netThread.join ();

    {
        std::lock_guard<std::mutex> lk ( m_sessLock );
        en_closesock ( m_listen );
        en_closesock ( m_udp );
        for ( int i = 0; i < SOCK_MAX_SESSIONS; ++i )
        {
            en_closesock ( m_sess[i].sock );
            m_sess[i].inUse = false;
            m_sess[i].rxbuf.clear ();
            m_sess[i].rxmsgs.clear ();
            m_sess[i].recvPending = false;
        }
        m_listens.clear ();
        m_dgPending = false;
        m_dgUser    = nullptr;
        m_dgQueue.clear ();
        m_listenPort = 0;
        m_udpPort    = 0;
    }

    // drop our registry name so a re-Listen rebinds cleanly
    if ( !m_localName.empty () )
    {
        std::lock_guard<std::mutex> lk ( s_regLock );
        s_nameReg.erase ( m_localName );
    }

    {
        std::lock_guard<std::mutex> lk ( m_qLock );
        m_completions.clear ();
    }
}

// ---- net-thread lifecycle ---------------------------------------------------
void CSockets::EnsureNetThread ()
{
    if ( m_run )
        return;
    m_run = true;
    m_netThread = std::thread ( &CSockets::NetThreadProc, this );
}

int CSockets::AllocSession ( en_socket_t s )
{
    std::lock_guard<std::mutex> lk ( m_sessLock );
    for ( int i = 0; i < SOCK_MAX_SESSIONS; ++i )
        if ( !m_sess[i].inUse )
        {
            en_setnonblock ( s );
            m_sess[i].sock        = s;
            m_sess[i].inUse       = true;
            m_sess[i].rxbuf.clear ();
            m_sess[i].rxmsgs.clear ();
            m_sess[i].recvPending = false;
            m_sess[i].recvUser    = nullptr;
            return ( i );
        }
    return ( -1 );
}

BOOL CSockets::EnsureListenSocket ()
{
    if ( m_listen != EN_INVALID_SOCKET )
        return ( TRUE );

    en_socket_t s = socket ( AF_INET, SOCK_STREAM, 0 );
    if ( s == EN_INVALID_SOCKET )
        return ( FALSE );

    int yes = 1;
    setsockopt ( s, SOL_SOCKET, SO_REUSEADDR, (const char *) &yes, sizeof ( yes ) );

    sockaddr_in addr;
    memset ( &addr, 0, sizeof ( addr ) );
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl ( INADDR_ANY );
    addr.sin_port        = 0;                       // ephemeral
    if ( bind ( s, (sockaddr *) &addr, sizeof ( addr ) ) != 0 ) { en_closesock ( s ); return ( FALSE ); }
    if ( listen ( s, 8 ) != 0 )                    { en_closesock ( s ); return ( FALSE ); }

    socklen_t len = sizeof ( addr );
    if ( getsockname ( s, (sockaddr *) &addr, &len ) == 0 )
        m_listenPort = ntohs ( addr.sin_port );

    en_setnonblock ( s );
    m_listen = s;
    return ( TRUE );
}

BOOL CSockets::ResolveName ( LPCSTR pName, sockaddr_in * pAddr, bool bUdp )
{
    if ( !pName || !pAddr )
        return ( FALSE );

    memset ( pAddr, 0, sizeof ( *pAddr ) );
    pAddr->sin_family = AF_INET;

    std::string key = en_namekey ( pName );

    // "*" / "BROADCAST" -> UDP LAN broadcast to the well-known discovery port (host advertise).
    if ( bUdp && ( key == "*" || key == "BROADCAST" || key.empty () ) )
    {
        pAddr->sin_addr.s_addr = htonl ( INADDR_BROADCAST );
        pAddr->sin_port        = htons ( SOCK_DISCOVERY_PORT );
        return ( TRUE );
    }

    // 1) logical name registered by a local Listen / ReceiveDatagram (same-host / self-test).
    {
        std::lock_guard<std::mutex> lk ( s_regLock );
        auto it = s_nameReg.find ( key );
        if ( it != s_nameReg.end () )
        {
            unsigned short port = bUdp ? it->second.udp : it->second.tcp;
            if ( port != 0 )
            {
                pAddr->sin_port        = htons ( port );
                pAddr->sin_addr.s_addr = htonl ( INADDR_LOOPBACK );
                return ( TRUE );
            }
        }
    }

    // 2) "a.b.c.d[:port]" direct-IP form (default port = discovery port).
    std::string host = key;
    unsigned short port = SOCK_DISCOVERY_PORT;
    size_t colon = host.find ( ':' );
    if ( colon != std::string::npos )
    {
        port = (unsigned short) atoi ( host.c_str () + colon + 1 );
        host = host.substr ( 0, colon );
    }
    in_addr ia;
    if ( inet_pton ( AF_INET, host.c_str (), &ia ) == 1 )
    {
        pAddr->sin_addr   = ia;
        pAddr->sin_port   = htons ( port );
        return ( TRUE );
    }

    return ( FALSE );
}

// create + bind the UDP socket (ephemeral port), register our name->udp for resolution
BOOL CSockets::EnsureUdpSocket ()
{
    if ( m_udp != EN_INVALID_SOCKET )
        return ( TRUE );

    en_socket_t s = socket ( AF_INET, SOCK_DGRAM, 0 );
    if ( s == EN_INVALID_SOCKET )
        return ( FALSE );

    int yes = 1;
    setsockopt ( s, SOL_SOCKET, SO_REUSEADDR, (const char *) &yes, sizeof ( yes ) );
    setsockopt ( s, SOL_SOCKET, SO_BROADCAST, (const char *) &yes, sizeof ( yes ) ); // for LAN discovery sends

    sockaddr_in addr;
    memset ( &addr, 0, sizeof ( addr ) );
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl ( INADDR_ANY );
    addr.sin_port        = 0;                       // ephemeral (fixed discovery-port bind comes with LAN wiring)
    if ( bind ( s, (sockaddr *) &addr, sizeof ( addr ) ) != 0 ) { en_closesock ( s ); return ( FALSE ); }

    socklen_t len = sizeof ( addr );
    if ( getsockname ( s, (sockaddr *) &addr, &len ) == 0 )
        m_udpPort = ntohs ( addr.sin_port );

    en_setnonblock ( s );
    m_udp = s;

    if ( !m_localName.empty () )
    {
        std::lock_guard<std::mutex> lk ( s_regLock );
        s_nameReg[m_localName].udp = m_udpPort;
    }
    return ( TRUE );
}

// ---- completion delivery (drained by the main loop; replaces WM_NET_COMPLETE) ----
void CSockets::PushCompletion ( const NETMSG & msg )
{
    std::lock_guard<std::mutex> lk ( m_qLock );
    m_completions.push_back ( msg );
}

BOOL CSockets::PopCompletion ( NETMSG * pOut )
{
    std::lock_guard<std::mutex> lk ( m_qLock );
    if ( m_completions.empty () )
        return ( FALSE );
    if ( pOut )
        *pOut = m_completions.front ();
    m_completions.pop_front ();
    return ( TRUE );
}

int CSockets::DrainCompletions ()
{
    std::deque<NETMSG> ready;
    {
        std::lock_guard<std::mutex> lk ( m_qLock );
        ready.swap ( m_completions );
    }
    // TODO (next increment): dispatch each NETMSG to the game's net handler — recreate
    // what the original WM_NET_COMPLETE handler did (PostToServer/PostToClient routing).
    // For RECEIVE/RECEIVE_DATAGRAM the handler must free(msg.pData) after copying.
    return ( (int) ready.size () );
}

// ---- net thread: select() over listen + sessions ----------------------------
void CSockets::HandleReadable ( int i )
{
    // recv whatever is available, frame it, deliver if a Receive is pending.
    std::lock_guard<std::mutex> lk ( m_sessLock );
    Session & se = m_sess[i];
    if ( !se.inUse )
        return;

    char buf[2048];
    for ( ;; )
    {
        int n = (int) recv ( se.sock, buf, sizeof ( buf ), 0 );
        if ( n > 0 )
        {
            se.rxbuf.append ( buf, n );
            continue;
        }
        if ( n == 0 )
        {
            // peer closed: tear down the session and report it
            en_closesock ( se.sock );
            se.inUse = false;
            NETMSG msg; memset ( &msg, 0, sizeof ( msg ) );
            msg.bInUse  = TRUE;
            msg.bCmd    = NET_MSG_RECEIVE;
            msg.bErr    = NET_ERR_SESSION_CLOSED;
            msg.bNetNum = (BYTE) i;
            msg.pUser   = se.recvUser;
            se.recvPending = false;
            PushCompletion ( msg );
            return;
        }
        if ( en_would_block () )
            break;
        return; // hard error; leave session, Close()/HangUp() will clean up
    }

    // parse all complete length-prefixed frames out of rxbuf
    for ( ;; )
    {
        if ( se.rxbuf.size () < 2 )
            break;
        unsigned short netlen;
        memcpy ( &netlen, se.rxbuf.data (), 2 );
        unsigned short plen = ntohs ( netlen );
        if ( se.rxbuf.size () < (size_t) ( 2 + plen ) )
            break;
        se.rxmsgs.emplace_back ( se.rxbuf.data () + 2, plen );
        se.rxbuf.erase ( 0, 2 + plen );
    }

    // deliver one framed message if the game has a Receive posted
    if ( se.recvPending && !se.rxmsgs.empty () )
    {
        std::string s = std::move ( se.rxmsgs.front () );
        se.rxmsgs.pop_front ();
        void * p = malloc ( s.size () ? s.size () : 1 );
        if ( p )
            memcpy ( p, s.data (), s.size () );
        NETMSG msg; memset ( &msg, 0, sizeof ( msg ) );
        msg.bInUse  = TRUE;
        msg.bCmd    = NET_MSG_RECEIVE;
        msg.bErr    = NET_ERR_NONE;
        msg.bNetNum = (BYTE) i;
        msg.pUser   = se.recvUser;
        msg.pData   = p;
        msg.iLen    = (short int) s.size ();
        se.recvPending = false;
        PushCompletion ( msg );
    }
}

void CSockets::HandleUdpReadable ()
{
    // drain all queued datagrams; each = [NET_NAME_MAX src-name][payload]
    char buf[2048];
    std::vector<std::pair<std::string,std::string>> got;
    for ( ;; )
    {
        sockaddr_in from;
        socklen_t   flen = sizeof ( from );
        int n = (int) recvfrom ( m_udp, buf, sizeof ( buf ), 0, (sockaddr *) &from, &flen );
        if ( n < 0 )
            break;                       // EWOULDBLOCK or error -> done
        std::string src, payload;
        if ( n >= NET_NAME_MAX )
        {
            int nl = 0;
            while ( nl < NET_NAME_MAX && buf[nl] && buf[nl] != ' ' )
                ++nl;
            src.assign ( buf, nl );
            payload.assign ( buf + NET_NAME_MAX, n - NET_NAME_MAX );
        }
        else
        {
            payload.assign ( buf, n );   // unframed (shouldn't happen from us)
        }
        got.emplace_back ( std::move ( src ), std::move ( payload ) );
    }
    if ( got.empty () )
        return;

    NETMSG ready; memset ( &ready, 0, sizeof ( ready ) );
    bool deliver = false;
    {
        std::lock_guard<std::mutex> lk ( m_sessLock );
        for ( auto & g : got )
            m_dgQueue.push_back ( std::move ( g ) );
        if ( m_dgPending && !m_dgQueue.empty () )
        {
            std::pair<std::string,std::string> d = std::move ( m_dgQueue.front () );
            m_dgQueue.pop_front ();
            void * p = malloc ( d.second.size () ? d.second.size () : 1 );
            if ( p )
                memcpy ( p, d.second.data (), d.second.size () );
            ready.bInUse = TRUE;
            ready.bCmd   = NET_MSG_RECEIVE_DATAGRAM;
            ready.bErr   = NET_ERR_NONE;
            ready.pUser  = m_dgUser;
            ready.pData  = p;
            ready.iLen   = (short int) d.second.size ();
            size_t cn = d.first.size () > NET_NAME_MAX ? NET_NAME_MAX : d.first.size ();
            memcpy ( ready.sName, d.first.data (), cn );
            m_dgPending = false;
            deliver = true;
        }
    }
    if ( deliver )
        PushCompletion ( ready );
}

void CSockets::NetThreadProc ()
{
    while ( m_run )
    {
        // snapshot the sockets to watch (don't hold m_sessLock across select)
        en_socket_t listenSock;
        en_socket_t udpSock;
        en_socket_t socks[SOCK_MAX_SESSIONS];
        int         idx[SOCK_MAX_SESSIONS];
        int         nSock = 0;
        {
            std::lock_guard<std::mutex> lk ( m_sessLock );
            listenSock = m_listen;
            udpSock    = m_udp;
            for ( int i = 0; i < SOCK_MAX_SESSIONS; ++i )
                if ( m_sess[i].inUse && m_sess[i].sock != EN_INVALID_SOCKET )
                {
                    socks[nSock] = m_sess[i].sock;
                    idx[nSock]   = i;
                    ++nSock;
                }
        }

        fd_set rfds;
        FD_ZERO ( &rfds );
        int maxfd = 0;
        if ( listenSock != EN_INVALID_SOCKET )
        {
            FD_SET ( listenSock, &rfds );
            maxfd = (int) listenSock;
        }
        if ( udpSock != EN_INVALID_SOCKET )
        {
            FD_SET ( udpSock, &rfds );
            if ( (int) udpSock > maxfd )
                maxfd = (int) udpSock;
        }
        for ( int k = 0; k < nSock; ++k )
        {
            FD_SET ( socks[k], &rfds );
            if ( (int) socks[k] > maxfd )
                maxfd = (int) socks[k];
        }

        timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = 50 * 1000;   // 50ms tick
        int r = select ( maxfd + 1, &rfds, NULL, NULL, &tv );
        if ( r <= 0 )
            continue;

        // inbound UDP datagram
        if ( udpSock != EN_INVALID_SOCKET && FD_ISSET ( udpSock, &rfds ) )
            HandleUdpReadable ();

        // inbound connection -> new session, complete a pending Listen
        if ( listenSock != EN_INVALID_SOCKET && FD_ISSET ( listenSock, &rfds ) )
        {
            sockaddr_in from;
            socklen_t   flen = sizeof ( from );
            en_socket_t c = accept ( listenSock, (sockaddr *) &from, &flen );
            if ( c != EN_INVALID_SOCKET )
            {
                PendingListen pl;
                bool havePending = false;
                {
                    std::lock_guard<std::mutex> lk ( m_sessLock );
                    if ( !m_listens.empty () )
                    {
                        pl = m_listens.front ();
                        m_listens.pop_front ();
                        havePending = true;
                    }
                }
                if ( !havePending )
                {
                    en_closesock ( c );   // nobody listening for it yet
                }
                else
                {
                    int sess = AllocSession ( c );
                    if ( sess < 0 )
                        en_closesock ( c );
                    else
                    {
                        NETMSG msg; memset ( &msg, 0, sizeof ( msg ) );
                        msg.bInUse  = TRUE;
                        msg.bCmd    = NET_MSG_LISTEN;
                        msg.bErr    = NET_ERR_NONE;
                        msg.bNetNum = (BYTE) sess;
                        msg.pUser   = pl.user;
                        PushCompletion ( msg );
                    }
                }
            }
        }

        // readable sessions
        for ( int k = 0; k < nSock; ++k )
            if ( FD_ISSET ( socks[k], &rfds ) )
                HandleReadable ( idx[k] );
    }
}

// ---- CProtocol API ----------------------------------------------------------
void CSockets::ErrMsgBox ( NETMSG * /*pMsg*/ )            {}
BOOL CSockets::AddName ( LPCSTR pName, LPCVOID )          { if ( pName ) m_localName = en_namekey ( pName ); return ( TRUE ); }
BOOL CSockets::AddGroupName ( LPCSTR, LPCVOID )           { return ( TRUE ); }
BOOL CSockets::DeleteName ( LPCSTR, LPCVOID )             { return ( TRUE ); }

void CSockets::CancelReceiveDatagram ( int )
{
    std::lock_guard<std::mutex> lk ( m_sessLock );
    m_dgPending = false;
}

void CSockets::CancelReceive ( int iNum )
{
    if ( iNum < 0 || iNum >= SOCK_MAX_SESSIONS )
        return;
    std::lock_guard<std::mutex> lk ( m_sessLock );
    m_sess[iNum].recvPending = false;
}

BOOL CSockets::Call ( LPCSTR pLocal, LPCSTR pRemote, LPCVOID pUser )
{
    EnsureNetThread ();
    if ( pLocal && m_localName.empty () )
        m_localName = en_namekey ( pLocal );

    sockaddr_in addr;
    if ( !ResolveName ( pRemote, &addr, false ) )
        return ( FALSE );

    en_socket_t s = socket ( AF_INET, SOCK_STREAM, 0 );
    if ( s == EN_INVALID_SOCKET )
        return ( FALSE );

    // blocking connect (loopback/LAN is fast; async connect is a later refinement)
    if ( connect ( s, (sockaddr *) &addr, sizeof ( addr ) ) != 0 )
    {
        en_closesock ( s );
        return ( FALSE );
    }

    int sess = AllocSession ( s );
    if ( sess < 0 )
    {
        en_closesock ( s );
        return ( FALSE );
    }

    NETMSG msg; memset ( &msg, 0, sizeof ( msg ) );
    msg.bInUse  = TRUE;
    msg.bCmd    = NET_MSG_CALL;
    msg.bErr    = NET_ERR_NONE;
    msg.bNetNum = (BYTE) sess;
    msg.pUser   = (void *) pUser;
    PushCompletion ( msg );
    return ( TRUE );
}

BOOL CSockets::Listen ( LPCSTR pLocal, LPCSTR pRemote, LPCVOID pUser )
{
    EnsureNetThread ();
    if ( !EnsureListenSocket () )
        return ( FALSE );

    std::string key = en_namekey ( pLocal );
    if ( !key.empty () )
    {
        m_localName = key;
        std::lock_guard<std::mutex> lk ( s_regLock );
        s_nameReg[key].tcp = m_listenPort;
    }

    PendingListen pl;
    pl.local  = key;
    pl.remote = en_namekey ( pRemote );
    pl.user   = (void *) pUser;
    {
        std::lock_guard<std::mutex> lk ( m_sessLock );
        m_listens.push_back ( pl );
    }
    return ( TRUE );
}

BOOL CSockets::Receive ( int iNum, LPCVOID pUser )
{
    if ( iNum < 0 || iNum >= SOCK_MAX_SESSIONS )
        return ( FALSE );

    NETMSG ready; memset ( &ready, 0, sizeof ( ready ) );
    bool deliver = false;
    {
        std::lock_guard<std::mutex> lk ( m_sessLock );
        Session & se = m_sess[iNum];
        if ( !se.inUse )
            return ( FALSE );
        se.recvPending = true;
        se.recvUser    = (void *) pUser;

        // a frame may already be buffered (data arrived before this Receive)
        if ( !se.rxmsgs.empty () )
        {
            std::string s = std::move ( se.rxmsgs.front () );
            se.rxmsgs.pop_front ();
            void * p = malloc ( s.size () ? s.size () : 1 );
            if ( p )
                memcpy ( p, s.data (), s.size () );
            ready.bInUse  = TRUE;
            ready.bCmd    = NET_MSG_RECEIVE;
            ready.bErr    = NET_ERR_NONE;
            ready.bNetNum = (BYTE) iNum;
            ready.pUser   = (void *) pUser;
            ready.pData   = p;
            ready.iLen    = (short int) s.size ();
            se.recvPending = false;
            deliver = true;
        }
    }
    if ( deliver )
        PushCompletion ( ready );
    return ( TRUE );
}

BOOL CSockets::Send ( int iNum, LPCVOID pData, int iLen, LPCVOID pUser )
{
    if ( iNum < 0 || iNum >= SOCK_MAX_SESSIONS || iLen < 0 )
        return ( FALSE );
    if ( iLen > 0xFFFF )
        iLen = 0xFFFF;   // 2-byte length frame ceiling

    en_socket_t sock;
    {
        std::lock_guard<std::mutex> lk ( m_sessLock );
        if ( !m_sess[iNum].inUse )
            return ( FALSE );
        sock = m_sess[iNum].sock;
    }

    // frame = [2-byte big-endian length][payload], sent as one logical message
    unsigned short netlen = htons ( (unsigned short) iLen );
    std::string frame;
    frame.append ( (const char *) &netlen, 2 );
    if ( iLen > 0 && pData )
        frame.append ( (const char *) pData, iLen );

    size_t off = 0;
    while ( off < frame.size () )
    {
        int n = (int) send ( sock, frame.data () + off, (int) ( frame.size () - off ), 0 );
        if ( n > 0 ) { off += n; continue; }
        if ( n < 0 && en_would_block () ) continue;   // socket buffer full; retry
        return ( FALSE );
    }

    NETMSG msg; memset ( &msg, 0, sizeof ( msg ) );
    msg.bInUse  = TRUE;
    msg.bCmd    = NET_MSG_SEND;
    msg.bErr    = NET_ERR_NONE;
    msg.bNetNum = (BYTE) iNum;
    msg.pUser   = (void *) pUser;
    msg.iLen    = (short int) iLen;
    PushCompletion ( msg );
    return ( TRUE );
}

BOOL CSockets::HangUp ( int iNum, LPCVOID pUser )
{
    if ( iNum < 0 || iNum >= SOCK_MAX_SESSIONS )
        return ( FALSE );
    {
        std::lock_guard<std::mutex> lk ( m_sessLock );
        Session & se = m_sess[iNum];
        if ( !se.inUse )
            return ( FALSE );
        en_closesock ( se.sock );
        se.inUse       = false;
        se.recvPending = false;
        se.rxbuf.clear ();
        se.rxmsgs.clear ();
    }
    NETMSG msg; memset ( &msg, 0, sizeof ( msg ) );
    msg.bInUse  = TRUE;
    msg.bCmd    = NET_MSG_HANG_UP;
    msg.bErr    = NET_ERR_NONE;
    msg.bNetNum = (BYTE) iNum;
    msg.pUser   = (void *) pUser;
    PushCompletion ( msg );
    return ( TRUE );
}

// ---- UDP datagrams (+ LAN broadcast for host discovery) ---------------------
BOOL CSockets::ReceiveDatagram ( int /*iNum*/, LPCVOID pUser )
{
    EnsureNetThread ();
    if ( !EnsureUdpSocket () )
        return ( FALSE );

    NETMSG ready; memset ( &ready, 0, sizeof ( ready ) );
    bool deliver = false;
    {
        std::lock_guard<std::mutex> lk ( m_sessLock );
        m_dgPending = true;
        m_dgUser    = (void *) pUser;
        if ( !m_dgQueue.empty () )      // a datagram already arrived before this post
        {
            std::pair<std::string,std::string> d = std::move ( m_dgQueue.front () );
            m_dgQueue.pop_front ();
            void * p = malloc ( d.second.size () ? d.second.size () : 1 );
            if ( p )
                memcpy ( p, d.second.data (), d.second.size () );
            ready.bInUse = TRUE;
            ready.bCmd   = NET_MSG_RECEIVE_DATAGRAM;
            ready.bErr   = NET_ERR_NONE;
            ready.pUser  = (void *) pUser;
            ready.pData  = p;
            ready.iLen   = (short int) d.second.size ();
            size_t cn = d.first.size () > NET_NAME_MAX ? NET_NAME_MAX : d.first.size ();
            memcpy ( ready.sName, d.first.data (), cn );
            m_dgPending = false;
            deliver = true;
        }
    }
    if ( deliver )
        PushCompletion ( ready );
    return ( TRUE );
}

BOOL CSockets::SendDatagram ( int /*iNum*/, LPCSTR pName, LPCVOID pData, int iLen, LPCVOID pUser )
{
    if ( iLen < 0 )
        return ( FALSE );
    if ( iLen > 1024 )
        iLen = 1024;                    // keep datagrams comfortably under the UDP MTU

    EnsureNetThread ();
    if ( !EnsureUdpSocket () )
        return ( FALSE );

    sockaddr_in addr;
    if ( !ResolveName ( pName, &addr, true ) )   // "*"/"BROADCAST" -> LAN broadcast
        return ( FALSE );

    // frame = [NET_NAME_MAX src-name (space-padded)][payload] so the receiver gets sName
    char nm[NET_NAME_MAX];
    memset ( nm, ' ', NET_NAME_MAX );
    {
        size_t cn = m_localName.size () > NET_NAME_MAX ? NET_NAME_MAX : m_localName.size ();
        memcpy ( nm, m_localName.data (), cn );
    }
    std::string frame;
    frame.append ( nm, NET_NAME_MAX );
    if ( iLen > 0 && pData )
        frame.append ( (const char *) pData, iLen );

    int s = (int) sendto ( m_udp, frame.data (), (int) frame.size (), 0,
                           (sockaddr *) &addr, sizeof ( addr ) );
    if ( s < 0 )
        return ( FALSE );

    NETMSG msg; memset ( &msg, 0, sizeof ( msg ) );
    msg.bInUse = TRUE;
    msg.bCmd   = NET_MSG_SEND_DATAGRAM;
    msg.bErr   = NET_ERR_NONE;
    msg.pUser  = (void *) pUser;
    msg.iLen   = (short int) iLen;
    PushCompletion ( msg );
    return ( TRUE );
}

// ---- loopback self-test (plan P1 "unit-smoke: loopback send/recv") -----------
BOOL CSockets::SelfTest ()
{
    if ( !InitSocketLib () )
        return ( FALSE );

    CSockets srv ( NULL, NULL );
    CSockets cli ( NULL, NULL );
    if ( !srv.InitOk () || !cli.InitOk () )
        return ( FALSE );

    srv.AddName ( "EN_SELFTEST_SRV", NULL );
    cli.AddName ( "EN_SELFTEST_CLI", NULL );

    if ( !srv.Listen ( "EN_SELFTEST_SRV", "*", (LPCVOID) 0x1111 ) )
        return ( FALSE );
    if ( !cli.Call ( "EN_SELFTEST_CLI", "EN_SELFTEST_SRV", (LPCVOID) 0x2222 ) )
        return ( FALSE );

    // wait for Listen (server) + Call (client) completions
    int srvSess = -1, cliSess = -1;
    NETMSG m;
    for ( int i = 0; i < 300 && ( srvSess < 0 || cliSess < 0 ); ++i )
    {
        while ( srv.PopCompletion ( &m ) )
            if ( m.bCmd == NET_MSG_LISTEN && m.bErr == NET_ERR_NONE )
                srvSess = m.bNetNum;
        while ( cli.PopCompletion ( &m ) )
            if ( m.bCmd == NET_MSG_CALL && m.bErr == NET_ERR_NONE )
                cliSess = m.bNetNum;
        std::this_thread::sleep_for ( std::chrono::milliseconds ( 5 ) );
    }
    if ( srvSess < 0 || cliSess < 0 )
        return ( FALSE );

    // server posts a Receive; client sends one message
    const char * payload = "enemy-nations-mp";
    int          plen    = (int) strlen ( payload );
    srv.Receive ( srvSess, (LPCVOID) 0x3333 );
    if ( !cli.Send ( cliSess, payload, plen, (LPCVOID) 0x4444 ) )
        return ( FALSE );

    BOOL tcpOk = FALSE;
    for ( int i = 0; i < 300 && !tcpOk; ++i )
    {
        while ( srv.PopCompletion ( &m ) )
        {
            if ( m.bCmd == NET_MSG_RECEIVE && m.bErr == NET_ERR_NONE )
            {
                if ( m.iLen == plen && m.pData && memcmp ( m.pData, payload, plen ) == 0
                     && m.pUser == (void *) 0x3333 )
                    tcpOk = TRUE;
            }
            if ( m.pData )
                free ( m.pData );
        }
        while ( cli.PopCompletion ( &m ) ) { if ( m.pData ) free ( m.pData ); }
        std::this_thread::sleep_for ( std::chrono::milliseconds ( 5 ) );
    }

    // ---- UDP datagram round-trip (host-discovery transport) ----
    // server posts a datagram receive (binds+registers its UDP port), client sends to it by name
    const char * dgram = "EN-DISCOVER?";
    int          dlen  = (int) strlen ( dgram );
    srv.ReceiveDatagram ( 0, (LPCVOID) 0x5555 );
    if ( !cli.SendDatagram ( 0, "EN_SELFTEST_SRV", dgram, dlen, (LPCVOID) 0x6666 ) )
        return ( FALSE );

    BOOL udpOk = FALSE;
    for ( int i = 0; i < 300 && !udpOk; ++i )
    {
        while ( srv.PopCompletion ( &m ) )
        {
            if ( m.bCmd == NET_MSG_RECEIVE_DATAGRAM && m.bErr == NET_ERR_NONE )
            {
                if ( m.iLen == dlen && m.pData && memcmp ( m.pData, dgram, dlen ) == 0
                     && m.pUser == (void *) 0x5555
                     && strncmp ( m.sName, "EN_SELFTEST_CLI", NET_NAME_MAX ) == 0 )
                    udpOk = TRUE;
            }
            if ( m.pData )
                free ( m.pData );
        }
        while ( cli.PopCompletion ( &m ) ) { if ( m.pData ) free ( m.pData ); }
        std::this_thread::sleep_for ( std::chrono::milliseconds ( 5 ) );
    }

    srv.Close ();
    cli.Close ();
    return ( tcpOk && udpOk );
}
