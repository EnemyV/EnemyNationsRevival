// CSockets — portable (winsock/BSD) TCP+UDP CProtocol implementation.
// See sockets.h and docs/plans/multiplayer-cross-platform.md.
//
// STATUS: foundation increment. Lifecycle (ctor/dtor/InitOk/Close/socket-lib) is
// real; the per-op methods (Call/Listen/Send/Receive/datagrams) and the net-thread
// select() loop are stubs (TODO), fleshed out in following increments. This file
// exists so the network layer compiles + links into the build on all 3 platforms
// for the first time, with naInit(NET_PROTO_TCP) constructing a CSockets.

#include "stdafx.h"
#include "sockets.h"

#ifndef _WIN32
  #include <cerrno>
#endif

// ---- socket library init (winsock needs WSAStartup; POSIX is a no-op) ----
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
    // stop the net thread
    m_run = false;
    if ( m_netThread.joinable () )
        m_netThread.join ();

    // close all sockets
    auto closesock = []( en_socket_t & s )
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
    };
    closesock ( m_listen );
    closesock ( m_udp );
    for ( int i = 0; i < SOCK_MAX_SESSIONS; ++i )
        closesock ( m_sess[i].sock );

    {
        std::lock_guard<std::mutex> lk ( m_qLock );
        m_completions.clear ();
    }
}

// ---- completion delivery (drained by the main loop; replaces WM_NET_COMPLETE) ----
int CSockets::DrainCompletions ()
{
    std::deque<NETMSG> ready;
    {
        std::lock_guard<std::mutex> lk ( m_qLock );
        ready.swap ( m_completions );
    }
    // TODO: dispatch each NETMSG to the game's net handler (the rebuilt completion path).
    return ( (int) ready.size () );
}

void CSockets::PushCompletion ( const NETMSG & msg )
{
    std::lock_guard<std::mutex> lk ( m_qLock );
    m_completions.push_back ( msg );
}

void CSockets::NetThreadProc ()
{
    // TODO: select() over m_listen / m_udp / sessions; on events build NETMSG
    // completions (mirroring netbios.cpp's NCB->NETMSG mapping) and PushCompletion().
}

// ---- CProtocol API (TODO: implement over TCP sessions + UDP datagrams) ----
void CSockets::ErrMsgBox ( NETMSG * /*pMsg*/ )            {}
BOOL CSockets::AddName ( LPCSTR pName, LPCVOID )          { if ( pName ) m_localName = pName; return ( TRUE ); }
BOOL CSockets::AddGroupName ( LPCSTR, LPCVOID )           { return ( TRUE ); }
BOOL CSockets::Call ( LPCSTR, LPCSTR, LPCVOID )           { return ( FALSE ); }
void CSockets::CancelReceive ( int )                      {}
void CSockets::CancelReceiveDatagram ( int )             {}
BOOL CSockets::DeleteName ( LPCSTR, LPCVOID )             { return ( TRUE ); }
BOOL CSockets::HangUp ( int, LPCVOID )                    { return ( FALSE ); }
BOOL CSockets::Listen ( LPCSTR, LPCSTR, LPCVOID )         { return ( FALSE ); }
BOOL CSockets::Receive ( int, LPCVOID )                   { return ( FALSE ); }
BOOL CSockets::ReceiveDatagram ( int, LPCVOID )          { return ( FALSE ); }
BOOL CSockets::Send ( int, LPCVOID, int, LPCVOID )        { return ( FALSE ); }
BOOL CSockets::SendDatagram ( int, LPCSTR, LPCVOID, int, LPCVOID ) { return ( FALSE ); }
