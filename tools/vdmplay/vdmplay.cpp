#ifndef VPSYSTEM
#define VPSYSTEM
#endif

#if !defined(NOMFC) && defined(_WIN32)
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#if defined(_AFXDLL) && !defined(_USRDLL)
#include <afxdllx.h>
#endif
#else

#define TRACE0(x) VPTRACE((x))
#define BASED_CODE

#endif

#include "base.h"
#include "datalog.h"
#include "stdafx.h"
#include "tdlog.h"
#include "vpengine.h"
#include "vpparam.h"
#include "vpwinsk.h"
#include "wnotque.h"
#ifdef _WIN32
#include <mmsystem.h>   // timeGetTime etc. — win32_compat provides these on POSIX
#endif
#include <stdio.h>

#ifndef WIN32
typedef unsigned char UCHAR, FAR* PUCHAR;
typedef DWORD ULONG;
#include <time.h>
#endif

#include <nb30.h>


#ifdef WIN32
# include <wsipx.h>
#endif

#ifndef WIN32 
extern "C"
{
# define W32SUT_16
# include "w32sut.h" 
    LPVOID( WINAPI* pUTLinearToSelectorOffset )( LPBYTE lpByte );
    LPVOID( WINAPI* pUTSelectorOffsetToLinear )( LPBYTE lpByte );
    HMODULE hUtLib;
}
#endif

#include "datagram.h"
#include "tcpnet.h"

#ifdef _WIN32
# include "wsipxnet.h"
#elif defined(_WIN16)
#define NWWIN
#ifdef socket
#undef socket
#endif
#include <nwipxspx.h>
# include "ipx16net.h"
#endif
// POSIX (§7b): IPX/SPX transport dropped — TCP only. No IPX includes.

#include "nbnet.h"

#ifdef _WIN32
#define WITH_COMM    // COMM/modem/TAPI transport — Windows only (dropped on POSIX, §7b TCP-only)
#endif
#ifdef WITH_COMM
#include "commnet.h"
#include "commport.h"
#include "mdmnet.h"
#ifdef WIN32
#include "w32comm.h"
#define COMMPORT_CLASS CW32CommPort
#else
#include "w16comm.h"
#define COMMPORT_CLASS CW16CommPort
#endif
#endif


#include "smap.h"
#include "version.h"

#include "vpint.h"

#ifndef _WIN32
#include "vp_netpump_posix.h"   // step 5: select() pump replacing WSAAsyncSelect
#endif

#ifdef WIN32
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#endif


#define VDMPLAYCLASS "VdmPlayClass"
#define WM_WINSOCK (WM_USER+1000)
#define WM_ABORTWAIT (WM_USER+1001)

#ifndef _WIN32
// Bridge the select() pump's FD_* callback back to the engine's existing
// dispatch: CNetInterface::OnMessage(hWnd, WM_WINSOCK, socket, MAKELONG(ev,err)).
// Stored ctx is the CNetInterface* (the net object) registered by ConfigureSocket.
static long vpTcpSelectThunk( void* ctx, SOCKET s, void* hWnd, long lParam ) {
    return (long)( (CNetInterface*)ctx )->OnMessage(
        (HWND)hWnd, WM_WINSOCK, (WPARAM)s, (LPARAM)lParam );
}
#endif

HINSTANCE vphInst;
HWND   vphWnd;


void vpassertion( LPCSTR text, LPCSTR file, int line );
void vptracemsg( LPCSTR text );
CDataLogger* vpMakeDataLogger();

void ( *_vpassert )( LPCSTR text, LPCSTR file, int line ) = vpassertion;
void ( *_vptracemsg )( LPCSTR text ) = vptracemsg;

CTDLogger* gErrLogger = NULL;
BOOL  gTraceLocation = FALSE;  // When set to FALSE the tracer suppress the
          // location info
BOOL   gLogWinsock;
extern BOOL gBreakOnAssert;
extern BOOL gLocalIni;
extern BOOL gUseLogfile;

extern void doAdvDialog( HWND hWnd, int iProtocol, BOOL bServer );
extern BOOL IsTapiSupported();



class CDbDataLogger: public CDataLogger {
    char m_buf[1024];


    void OutgoingData( LPCSTR data, size_t s ) {
        size_t l;

        strcpy( m_buf, "Out: " );
        strncat( m_buf, data, s );
        l = strlen( m_buf );
        m_buf[l + s] = 0;
        OutputDebugString( m_buf );
    }

    void IncomingData( LPCSTR data, size_t s ) {
        size_t l;

        strcpy( m_buf, "Inp: " );
        strncat( m_buf, data, s );
        l = strlen( m_buf );
        m_buf[l + s] = 0;
        OutputDebugString( m_buf );
    }

};

CDataLogger* vpMakeDataLogger() {
#ifdef _WIN32
    return new CDialogLogger();
#else
    // POSIX: CDialogLogger is an MFC GUI comm-log window (datalog.cpp, not built
    // here). The engine treats a NULL data logger as "logging off" — every
    // m_dataLog dereference (CNetInterface/CDataLink) is null-guarded, and the
    // TCP path (tcpnet.cpp) never touches it. So no UI logger on POSIX.
    return NULL;
#endif
}

#if 0
CDataLogger* vpMakeDataLogger() {
    return NULL;
}
#endif


CDataLogger* ( *_vpMakeDataLogger )( ) = vpMakeDataLogger;



void CTDLogger::Log( LPCSTR msg ) {
#ifdef WIN32
    SYSTEMTIME tm;

    GetLocalTime( &tm );

    wsprintf( m_buf, "%2d:%2d:%2d:%4d - %s\n", tm.wHour, tm.wMinute,
              tm.wSecond, tm.wMilliseconds, msg );
#else  
    time_t t = time( NULL );
    struct tm* tms = localtime( &t );

    wsprintf( m_buf, "%2d:%2d:%2d:%4d - %s\n", tms->tm_hour, tms->tm_min,
              tms->tm_sec, (int)( GetCurrentTime() % 1000 ), msg );


#endif
    Write( m_buf );
}


void CTDLogger::SetError( DWORD e1, DWORD e2, DWORD e3 ) {
#ifdef WIN32
    SYSTEMTIME tm;

    GetLocalTime( &tm );

    wsprintf( m_buf, "%2d:%2d:%2d:%4d - Error: %ld:%ld:%ld\n",
              tm.wHour, tm.wMinute,
              tm.wSecond, tm.wMilliseconds,
              e1, e2, e3 );
#else
    time_t t = time( NULL );
    struct tm* tms = localtime( &t );

    wsprintf( m_buf, "%2d:%2d:%2d:%4d  - Error: %ld:%ld:%ld\n", tms->tm_hour, tms->tm_min,
              tms->tm_sec, (int)( GetCurrentTime() % 1000 ), e1, e2, e3 );

#endif

    m_time = GetCurrentTime();

    Write( m_buf );
}

void CTDLogger::SetFatalError( DWORD e1, DWORD e2, DWORD e3 ) {
    DWORD t = GetCurrentTime();
#ifdef WIN32
    SYSTEMTIME tm;

    GetLocalTime( &tm );

    wsprintf( m_buf, "%2d:%2d:%2d:%4d - Fatal Error: %ld:%ld:%ld\n",
              tm.wHour, tm.wMinute,
              tm.wSecond, tm.wMilliseconds,
              e1, e2, e3 );
#else
    time_t tt = time( NULL );
    struct tm* tms = localtime( &tt );

    wsprintf( m_buf, "%2d:%2d:%2d:%4d  - Fatal Error: %ld:%ld:%ld\n", tms->tm_hour, tms->tm_min,
              tms->tm_sec, (int)( GetCurrentTime() % 1000 ), e1, e2, e3 );

#endif

    m_time = t;
    Write( m_buf );
}









class CWinTcpNet: public CTcpNet {


public:

    static CTCPLink* LookupLink( SOCKET s ) { return CTCPLink::LookupLink( s ); }

    void KeekNotifications( SOCKET s );

    void ConfigureSocket( SOCKET s, DWORD flags ) {   // DWORD not u_long: on LP64 they differ, so
        u_long on = 1;                                 // u_long broke the override of the base pure-virtual (POSIX)
        int bufSize = 2048 * 4;
        int bvLen = sizeof( int );

        ioctlsocket( s, FIONBIO, &on );

        //  setsockopt(s, SOL_SOCKET, SO_RCVBUF, (LPCSTR) &bufSize, bvLen);

        bufSize = (int)m_sockBufSize;
        setsockopt( s, SOL_SOCKET, SO_SNDBUF, (LPCSTR)&bufSize, bvLen );

#ifdef _WIN32
        WSAAsyncSelect( s, m_window, WM_WINSOCK, flags );
#else
        // POSIX (step 5): record the requested FD_* mask for the select() pump,
        // which synthesizes the same WM_WINSOCK/OnMessage dispatch. `this` is the
        // CNetInterface* the thunk calls OnMessage on. flags==0 deregisters.
        vpNetSelectSet( s, vpTcpSelectThunk, (CNetInterface*)this, m_window, (long)flags );
#endif

    }


    LRESULT OnMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );


    CWinTcpNet( CTDLogger* log, HWND window, WORD wellKnownPort, u_long serverAddress, LPCSTR srvAddrStr ):
        CTcpNet( log, 0, 0, wellKnownPort, serverAddress, srvAddrStr ), m_window( window ) {}

public:
    HWND  m_window;

};



//static LPCSTR protoName[] = { "TCP", "IPX", "NETBIOS", "COMM", "MODEM", "TAPI", "DP" };

void LoadDefault( UINT proto, LPCSTR keyName, LPCSTR defVal, LPSTR buf, UINT size ) {
    char fName[256];

    if ( lstrlen( buf ) )
        return;

    vpMakeIniFile( fName );

    GetPrivateProfileString(
        protoName[proto],
        keyName,
        defVal,
        buf, size, fName );
}

UINT LoadDefaultInt( UINT proto, LPCSTR keyName, UINT defVal ) {
    char fName[256];

    vpMakeIniFile( fName );

    return GetPrivateProfileInt(
        protoName[proto],
        keyName,
        defVal,
        fName );
}


void logWinsockEvent( WPARAM wParam, LPARAM lParam, CNetLink* l ) {
    char logBuf[128];
    const char* evt = "UNKNOWN";

    switch ( WSAGETSELECTEVENT( lParam ) ) {
    case FD_READ:
        evt = "FD_READ";
        break;

    case FD_WRITE:
        evt = "FD_WRITE";
        break;

    case FD_CONNECT:
        evt = "FD_CONNECT";
        break;

    case FD_ACCEPT:
        evt = "FD_ACCEPT";
        break;

    case FD_CLOSE:
        evt = "FD_CLOSE";
        break;
    }


    wsprintf( logBuf,
              "WM_WINSOCK %s on socket %u link=%08x", evt, wParam, l );

    if ( gErrLogger )
        gErrLogger->Log( logBuf );


}

// Reset WINSOCK Notification machinery
void    CWinTcpNet::KeekNotifications( SOCKET s ) {
    WSAAsyncSelect( s, m_window, WM_WINSOCK, 0 );
    WSAAsyncSelect( s, m_window, WM_WINSOCK, FD_READ | FD_WRITE | FD_CLOSE );
}

LRESULT CWinTcpNet::OnMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
    if ( uMsg != WM_WINSOCK )
        return FALSE;

    CTCPLink* l = LookupLink( (SOCKET)wParam );

    if ( gLogWinsock )
        logWinsockEvent( wParam, lParam, l );

    if ( !l )
        return TRUE;


    switch ( WSAGETSELECTEVENT( lParam ) ) {
    case FD_READ:
        VPTRACE( ( "FD_READ on %08x", wParam ) );
        if ( l->m_state == CONNECTED ) {


            if ( m_safeHook ) {

                WSAAsyncSelect( (SOCKET)wParam, m_window, WM_WINSOCK, 0 );

                m_safeHook( l, m_hookData );

                WSAAsyncSelect( (SOCKET)wParam, m_window, WM_WINSOCK, FD_READ | FD_WRITE | FD_CLOSE );
                m_lastKeekTime = GetCurrentTime();
            }
        } else if ( l->m_state == DG ) {
            if ( m_unsafeHook )
                m_unsafeHook( l, m_hookData );
        } else {
            // We're in CONNECTING state...
            // Either we've lost the FD_CONNECT or
            // the FD_READ came before the FD_CONNECT
            VPTRACE( ( "Premature FD_READ" ) );


            l->m_state = CONNECTED;
            l->SendWaitingData();
            if ( m_connectHook )
                m_connectHook( l, m_hookData );

            if ( m_safeHook )
                m_safeHook( l, m_hookData );
        }

        return TRUE;

    case FD_WRITE:
        VPTRACE( ( "FD_WRITE on %08x", wParam ) );
        l->SendWaitingData();
        return TRUE;

    case FD_CONNECT:
        VPTRACE( ( "FD_CONNECT on %08x", wParam ) );
        if ( l->m_state == CONNECTED ) {
            // Apparently the first FD_READ came before the FD_CONNECT
            // So we simply ignoring this one
            VPTRACE( ( "FD_CONNECT on CONNECTED socket" ) );
            return TRUE;
        }

        l->m_state = CONNECTED;
        l->SendWaitingData();
        if ( m_connectHook )
            m_connectHook( l, m_hookData );
        return TRUE;

    case FD_ACCEPT:
    {
        VPTRACE( ( "FD_ACCEPT on %08x", wParam ) );
        CTCPLink* nl = AcceptLink();

        if ( m_acceptHook )
            m_acceptHook( nl, m_hookData );

        nl->Unref();

        return TRUE;
    }

    case FD_CLOSE:

        VPTRACE( ( "FD_CLOSE on %08x", wParam ) );
        if ( m_disconnectHook )
            m_disconnectHook( l, m_hookData );

        return TRUE;
    }

    return TRUE;
}


#ifdef _WIN32
class CWinIpxNet: public CWSIpxNet {


public:

    static CIPXLink* LookupLink( SOCKET s ) { return CIPXLink::LookupLink( s ); }




    void ConfigureSocket( SOCKET s, DWORD flags ) {   // DWORD not u_long: on LP64 they differ, so
        u_long on = 1;                                 // u_long broke the override of the base pure-virtual (POSIX)
        int bufSize = 2048 * 4;
        int bvLen = sizeof( int );

        ioctlsocket( s, FIONBIO, &on );

        setsockopt( s, SOL_SOCKET, SO_RCVBUF, (LPCSTR)&bufSize, bvLen );
        setsockopt( s, SOL_SOCKET, SO_SNDBUF, (LPCSTR)&bufSize, bvLen );

        WSAAsyncSelect( s, m_window, WM_WINSOCK, flags );

    }


    LRESULT OnMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );


    CWinIpxNet( CTDLogger* log, HWND window, WORD wellKnownPort, ipx_addr* server = NULL, LPCSTR srvAddrStr = NULL ):
        CWSIpxNet( log, 0, 0, wellKnownPort, server, srvAddrStr ), m_window( window ) {}

public:
    HWND  m_window;

};


LRESULT CWinIpxNet::OnMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
    if ( uMsg != WM_WINSOCK )
        return FALSE;

    CIPXLink* l = LookupLink( (SOCKET)wParam );

    if ( gLogWinsock )
        logWinsockEvent( wParam, lParam, l );

    if ( !l )
        return TRUE;


    switch ( WSAGETSELECTEVENT( lParam ) ) {
    case FD_READ:
        if ( l->m_state == CONNECTED ) {
            if ( m_safeHook )
                m_safeHook( l, m_hookData );
        } else {
            if ( m_unsafeHook )
                m_unsafeHook( l, m_hookData );
        }
        return TRUE;

    case FD_WRITE:
        l->SendWaitingData();
        return TRUE;

    case FD_CONNECT:
        l->m_state = CONNECTED;
        l->SendWaitingData();
        if ( m_connectHook )
            m_connectHook( l, m_hookData );
        return TRUE;

    case FD_ACCEPT:
    {
        CIPXLink* nl = AcceptLink();

        if ( m_acceptHook )
            m_acceptHook( nl, m_hookData );

        nl->Unref();

        return TRUE;
    }

    case FD_CLOSE:

        if ( m_disconnectHook )
            m_disconnectHook( l, m_hookData );

        return TRUE;
    }

    return TRUE;
}

#elif defined(_WIN16)   // POSIX: IPX dropped (§7b TCP-only) — build neither IPX class

class CWinIpx16Net: public CIpx16Net {


public:


    LRESULT OnMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );


    CWinIpx16Net( CTDLogger* log, HWND window, WORD wellKnownPort, ipx16_addr* server ): CIpx16Net( log, window, wellKnownPort, server ) {}

public:

};


LRESULT CWinIpx16Net::OnMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
    if ( uMsg != WM_COMMNOTIFY )
        return FALSE;

    OnEcbCompletion( (CEcb*)lParam );
    return TRUE;
}


#endif 


// class CVdmPlay


class CVdmErrorLogger:public CTDLogger {

public:
    CVdmErrorLogger( CVdmPlay* vdmplay, BOOL visible = FALSE ): m_vdmPlay( vdmplay ), m_file( NULL ) {
        if ( gUseLogfile ) {
            // Per-process filename so two instances on the same machine (host +
            // client localhost test) don't truncate each other's log.
            char logName[64];
            wsprintf( logName, "vdmplay_%lu.log", (unsigned long)GetCurrentProcessId() );
            m_file = fopen( logName, "wt" );
        }

    }

    ~CVdmErrorLogger() {
        if ( m_file ) {
            fclose( m_file );
            m_file = NULL;
        }
    }

    virtual void Write( LPCSTR msg ) {
        if ( m_file ) {
            fputs( msg, m_file );
            fflush( m_file );
        }
#ifndef NDEBUG
        OutputDebugString( msg );
#endif
    }

    void SetError( DWORD e1, DWORD e2, DWORD e3 ) {
        CTDLogger::SetError( e1, e2, e3 );
        m_vdmPlay->RecordError( e1, e2, e3 );
    }

    void SetFatalError( DWORD e1, DWORD e2, DWORD e3 ) {
        CTDLogger::SetFatalError( e1, e2, e3 );
        m_vdmPlay->RecordError( e1, e2, e3 );
        m_vdmPlay->m_fatalError = TRUE;
    }

protected:
    CVdmPlay* m_vdmPlay;
    FILE* m_file;
};


#ifdef _WIN16   // Universal-Thunk CVdmPlay subclass — Win16/Win32s only (n/a on Win32/POSIX)
class CUtVdmPlay: public CVdmPlay {
public:

    CUtVdmPlay() {}

    virtual CWinNotifyQueue* MakeNotifyQueue( HWND wnd, UINT msg );
};

#endif

BOOL CVdmPlay::Startup( IN DWORD version,
                        IN LPCVPGUID guid,
                        IN DWORD sessionDataSize,
                        IN DWORD playerDataSize,
                        IN UINT protocol, IN LPCVOID protocolData )

{
    m_version = version;
    m_guid = *guid;
    m_sessionDataSize = sessionDataSize;
    m_playerDataSize = playerDataSize;

    if ( !InitWindowsStuff() )
        return FALSE;

    InitNetworkProtocol( protocol, protocolData );



    if ( !m_net ) {

        return FALSE;
    }

    return TRUE;


}



BOOL CVdmPlay::EnumSessions( IN HWND hWnd,
                             IN BOOL dontAutoStop, // if FALSE will AutoStop enum
                             IN LPVOID userData ) {

    if ( m_session || m_sEnum ) {
        return FALSE;
    }

    m_sEnum = MakeSessionEnum( hWnd, userData );
    if ( !m_sEnum )
        return FALSE;

    m_sEnum->Begin();

    return TRUE;

}


BOOL CVdmPlay::StartRegistrationServer( IN HWND hWnd,
                                        IN LPVOID userData ) {

    if ( m_session || m_sEnum ) {
        return FALSE;
    }

    m_sEnum = MakeSessionRegistery( hWnd, userData );
    if ( !m_sEnum )
        return FALSE;

    m_sEnum->Begin();

    return TRUE;

}


BOOL CVdmPlay::StopEnumSessions() {

    if ( !m_sEnum )
        return FALSE;

    m_sEnum->Stop();
    delete m_sEnum;
    m_sEnum = NULL;
    return TRUE;
}


BOOL CVdmPlay::StopEnumPlayers() {
    if ( m_pEnum ) {
        m_pEnum->Stop();
        delete m_pEnum;
        m_pEnum = NULL;
        return TRUE;
    }

    return FALSE;
}





BOOL CVdmPlay::EnumPlayers( IN HWND hWnd,
                            IN LPCVPSESSIONID sessionId,
                            IN LPVOID userData ) {

    if ( m_pEnum || m_sEnum || m_session )
        return FALSE;

    m_pEnum = MakePlayerEnum();

    m_pEnum->SetWindow( hWnd );
    m_pEnum->SetUserData( userData );
    m_pEnum->SetSessionId( sessionId );
    m_pEnum->Begin();

    return TRUE;

}



void CVdmPlay::Cleanup() {

    StopEnumSessions();
    StopEnumPlayers();
    CleanupWindowsStuff();

    if ( m_session ) {
        m_session->CloseSession();
        delete m_session;
        m_session = NULL;
    }

    delete m_net;
    m_net = NULL;
}


VPSESSIONHANDLE CVdmPlay::CreateSession( IN HWND hWnd,
                                         IN LPCSTR sessionName,
                                         IN DWORD sessionFlags,
                                         IN LPVOID userData ) {

    m_session = MakeLocalSession( hWnd, sessionName, sessionFlags, userData );

    if ( !m_session )
        return NULL;

    return m_session;
}


VPSESSIONHANDLE CVdmPlay::JoinSession( IN HWND hWnd,
                                       IN LPCVPSESSIONID sessionId,
                                       IN LPCSTR playerName,
                                       IN DWORD  playerFlags,
                                       IN LPVOID userData ) {

    StopEnumSessions();

    m_session = MakeRemoteSession( hWnd );



    if ( !m_session )
        return NULL;


    m_session->SetSessionId( sessionId );

    m_session->AddLocalPlayer( playerName, playerFlags, userData );

    return m_session;


}

DWORD CVdmPlay::GetAddressString( IN LPCVPNETADDRESS addr, LPSTR buf, DWORD bufSize ) {
    if ( !m_net )
        return ~0u;

    CNetAddress* a = m_net->MakeAddress( addr );
    if ( !a )
        return ~0u;

    DWORD ret = a->GetPrintForm( buf, bufSize );
    a->Unref();

    return ret;
}




BOOL  CVdmPlay::GetAddress( IN LPVPNETADDRESS addr ) {

    if ( !m_net )
        return FALSE;

    m_net->GetAddress( addr );

    return TRUE;
}

BOOL CVdmPlay::GetServerAddress( OUT LPVPNETADDRESS addr ) {
    if ( !m_net )
        return FALSE;

    return m_net->IsServerSeen( addr );

}






LRESULT APIENTRY CVdmPlay::WinProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
    CVdmPlay* vp = (CVdmPlay*)GetWindowLongPtr( hWnd, GWLP_USERDATA );

    if ( !vp )
        return DefWindowProc( hWnd, uMsg, wParam, lParam );


    if ( uMsg == WM_TIMER ) {
        vp->DriveTimers();
        return TRUE;
    }

    if ( uMsg == WM_ABORTWAIT ) {
        vpDoAbortWait();
        return TRUE;
    }

    LRESULT ret = FALSE;
    if ( vp->m_net )
        ret = vp->m_net->OnMessage( hWnd, uMsg, wParam, lParam );

    if ( !ret )
        ret = DefWindowProc( hWnd, uMsg, wParam, lParam );

    return ret;

}


BOOL CVdmPlay::InitWindowsStuff() {
#ifdef _WIN32
    static BOOL classReady = FALSE;
    WNDCLASS vdmPlayClass = { 0, WinProc, 0, sizeof( void* ),
             NULL, NULL, NULL, NULL,
             NULL, VDMPLAYCLASS };



    vdmPlayClass.hInstance = vphInst;
    if ( !classReady ) {
        ATOM a = RegisterClass( &vdmPlayClass );

        if ( !a )
            return FALSE;

        classReady = TRUE;
    }


    m_window = CreateWindow(
        VDMPLAYCLASS,                   /* See RegisterClass() call.          */
        "Hidden VdmPlay Window",        /* Text for window title bar.         */
        WS_OVERLAPPEDWINDOW,            /* Window style.                      */
        CW_USEDEFAULT,                  /* Default horizontal position.       */
        CW_USEDEFAULT,                  /* Default vertical position.         */
        CW_USEDEFAULT,                  /* Default width.                     */
        CW_USEDEFAULT,                  /* Default height.                    */
        NULL,                           /* Overlapped windows have no parent. */
        NULL,                           /* Use the window class menu.         */
        vphInst,                      /* This instance owns this window.    */
        NULL                            /* Pointer not needed.                */
    );


    if ( !m_window ) {
        return FALSE;
    }


    SetWindowLongPtr( m_window, GWLP_USERDATA, (LONG_PTR)this );

    m_timer = SetTimer( m_window, 0, 250L, NULL );

    if ( !m_timer ) {
        DestroyWindow( m_window );
        m_window = NULL;
        return FALSE;
    }

    return TRUE;
#else
    // POSIX: no hidden WSAAsyncSelect msg-window. The step-5 select() loop drives socket
    // events and the engine timer (OnTimer) runs from the main-loop drain (step 6).
    m_window = NULL;
    m_timer  = 0;
    return TRUE;
#endif
}




BOOL CVdmPlay::InitTcp( LPCVOID data ) {
    LPCVPTCPDATA tcpData;
    u_long   serverAddress = INADDR_BROADCAST;
    u_short   tcpPort = 0;
    BOOL needAddrString = TRUE;

    if ( data ) {
        tcpData = (LPCVPTCPDATA)data;
        serverAddress = tcpData->serverAddress;
        tcpPort = tcpData->wellKnownPort;

        if ( serverAddress )
            needAddrString = FALSE;
    }


    if ( !tcpPort ) {

        tcpPort = LoadDefaultInt( VPT_TCP, "WellKnownPort", DEF_TCP_PORT );
    }

    if ( !InitWsThunks() )
        return FALSE;

    char srvAddrStr[64];
    LPCSTR pSrvAddr = NULL;

    srvAddrStr[0] = 0;

    if ( needAddrString ) {
        char nt_sa[160]={0}, nt_ra[160]={0}, nt_path[256]={0}; int nt_src=0;
        LoadDefault( VPT_TCP, "ServerAddress", "", srvAddrStr, sizeof( srvAddrStr ) );
        strncpy(nt_sa, srvAddrStr, sizeof(nt_sa)-1);
        if ( lstrlen( srvAddrStr ) ) {
            pSrvAddr = srvAddrStr;
            serverAddress = 0;
            nt_src=1;
        } else {
            LoadDefault( VPT_TCP, "RegistrationAddress", "", srvAddrStr, sizeof( srvAddrStr ) );
            strncpy(nt_ra, srvAddrStr, sizeof(nt_ra)-1);
            if ( lstrlen( srvAddrStr ) ) {
                pSrvAddr = srvAddrStr;
                serverAddress = 0;
                nt_src=2;
            } else {
                strcpy( srvAddrStr, DEF_IP_REG_SERVER );
                pSrvAddr = srvAddrStr;
                serverAddress = 0;
                nt_src=3;
            }

        }
        { static int nt=-1; if(nt<0) nt=getenv("EN_NETTRACE")?1:0;
          if(nt){ vpMakeIniFile(nt_path);
                  fprintf(stderr,"[nettrace] InitTcp read iniPath='%s' ServerAddress='%s' RegistrationAddress='%s' -> pSrvAddr='%s' src=%s\n",
                          nt_path, nt_sa, nt_ra, pSrvAddr?pSrvAddr:"(null)",
                          nt_src==1?"ServerAddress":nt_src==2?"RegistrationAddress":"DEF_IP_REG_SERVER(dead)"); } }
    }


    CWinTcpNet* net = new CWinTcpNet( m_log, m_window, tcpPort, serverAddress, pSrvAddr );


    if ( !net ) {
        return FALSE;
    }

    DWORD err = net->LastError();

    if ( err ) {
        delete net;
        return FALSE;
    }

    m_net = net;
    LoadDefault( VPT_TCP, "RegistrationAddress", "", srvAddrStr, sizeof( srvAddrStr ) );

    if ( lstrlen( srvAddrStr ) )
        net->SetRegistrationAddress( srvAddrStr );

    // iserve host-register diagnostic (env EN_ISERVE_LOG=1; off => zero impact).
    // Logs the [TCP]RegistrationAddress the HOST read here (distinct from the client's
    // ServerAddress read above) + whether SetRegistrationAddress fired — the first link
    // in the host->iserve register chain. Pairs with the [iserve-host] traces in vpengine.
    { static int il=-1; if(il<0) il=getenv("EN_ISERVE_LOG")?1:0;
      if(il) fprintf(stderr,"[iserve-host] InitTcp [TCP]RegistrationAddress read='%s' -> SetRegistrationAddress %s\n",
                     srvAddrStr, lstrlen(srvAddrStr)?"CALLED":"SKIPPED(empty)"); }

    return TRUE;
}

BOOL CVdmPlay::InitIpx( LPCVOID data ) {
#ifdef _WIN32
    UINT ipxPort = 0;
    LPCVPIPXDATA ipxData = (LPCVPIPXDATA)data;
#ifdef WIN32
    typedef ipx_addr* PIpxAddr;
#else
    typedef ipx16_addr* PIpxAddr;
#endif

    PIpxAddr server = NULL;
    if ( data ) {
        ipxPort = ipxData->wellKnownPort;
        server = (PIpxAddr)ipxData->netNum;
    }

    if ( !ipxPort ) {
        ipxPort = LoadDefaultInt( VPT_IPX, "WellKnownPort", DEF_IPX_PORT );
    }

    if ( !InitWsThunks() )
        return FALSE;

    char srvAddrStr[64];
    LPCSTR pSrvAddr = NULL;

    srvAddrStr[0] = 0;

    if ( !server ) {
        char srvNode[32];
        char srvNet[32];

        srvNode[0] = srvNet[0] = 0;

        LoadDefault( VPT_IPX, "ServerNode", "", srvNode, sizeof( srvNode ) );
        LoadDefault( VPT_IPX, "ServerNet", "", srvNet, sizeof( srvNet ) );
        if ( lstrlen( srvNode ) && lstrlen( srvNet ) ) {
            strcpy( srvAddrStr, srvNet );
            strcat( srvAddrStr, "." );
            strcat( srvAddrStr, srvNode );
            pSrvAddr = srvAddrStr;
        } else {
            LoadDefault( VPT_IPX, "RegistrationAddress", "", srvAddrStr, sizeof( srvAddrStr ) );
            if ( lstrlen( srvAddrStr ) ) {
                pSrvAddr = srvAddrStr;
            }
        }


    }


#ifdef WIN32     
    m_net = new CWinIpxNet( m_log, m_window, ipxPort, server, pSrvAddr );
#else
    m_net = new CWinIpx16Net( m_log, m_window, ipxPort, server );
#endif

    if ( !m_net ) {
        return FALSE;
    }

    DWORD err = m_net->LastError();
    if ( err ) {
        delete m_net;
        m_net = NULL;
        return FALSE;
    }

    srvAddrStr[0] = 0;
    LoadDefault( VPT_IPX, "RegistrationAddress", "", srvAddrStr, sizeof( srvAddrStr ) );

    if ( lstrlen( srvAddrStr ) )
        m_net->SetRegistrationAddress( srvAddrStr );

    return TRUE;
#else
    (void)data; return FALSE;   // §7b: IPX transport dropped on POSIX (TCP only)
#endif
}


BOOL CVdmPlay::InitComm( LPCVOID data, BOOL modem ) {
#if defined(WITH_COMM)
    VPCOMMDATA commData;

    if ( data ) {
        commData = *(LPCVPCOMMDATA)data;
    } else {
        memset( &commData, 0, sizeof( commData ) );
    }

    LoadDefault( modem ? VPT_MODEM : VPT_COMM, "Port", "",
                 commData.deviceName, sizeof( commData.deviceName ) );



    if ( !lstrlen( commData.deviceName ) ) {
        //  SetError(VP_ERR_BAD_CONFIG);
        if ( !vpFindFreePort( commData.deviceName ) )
            return FALSE;
        vpStoreString( protoName[modem ? VPT_MODEM : VPT_COMM], "Port",
                       commData.deviceName );
    }

    if ( modem ) {
        LoadDefault( VPT_MODEM, "CALL_INIT", "",
                     commData.callInit, sizeof( commData.callInit ) );

        LoadDefault( VPT_MODEM, "LISTEN_INIT", "",
                     commData.listenInit, sizeof( commData.listenInit ) );


        LoadDefault( VPT_MODEM, "DIAL_NUMBER", "",
                     commData.callNumber, sizeof( commData.callNumber ) );

        LoadDefault( VPT_MODEM, "DIAL_PREFIX", "",
                     commData.dialPrefix, sizeof( commData.dialPrefix ) );

        LoadDefault( VPT_MODEM, "DIAL_SUFFIX", "",
                     commData.dialSuffix, sizeof( commData.dialSuffix ) );


    }




    CCommPort* p = new COMMPORT_CLASS( commData.deviceName, m_window );

    if ( !p ) {
        //  SetError(VP_ERR_NOMEM);
        return FALSE;
    }

    if ( !modem ) {
        m_net = new CCommNet( m_log, p );
    } else {
        CModemNet* mdmNet = new CModemNet( m_log, p, commData.listenInit, commData.callInit,
                                           commData.callNumber, commData.dialPrefix, commData.dialSuffix );


        m_net = mdmNet;
    }


    if ( !m_net ) {
        //  SetError(VP_ERR_NOMEM);
        delete p;
        return FALSE;
    }

    m_net->StartDataLog( _vpMakeDataLogger() );

    DWORD err = m_net->LastError();

    if ( err ) {
        delete m_net;
        delete p;

        m_net = NULL;
        return FALSE;
    }

    return TRUE;
#else
    return FALSE;
#endif
}

BOOL CVdmPlay::InitTapi( LPCVOID data ) {
#if defined(WIN32)
    CCommNet* MakeTapiNet( CTDLogger * log, CCommPort * port, LPCSTR callNumber, HINSTANCE hInst, LPCSTR iniFile, LPCSTR iniSection );
    char number[32] = "";
    char fName[128];

    vpMakeIniFile( fName );

    LoadDefault( VPT_TAPI, "DIAL_NUMBER", "",
                 number, sizeof( number ) );




    CCommPort* p = new COMMPORT_CLASS( "", m_window );

    if ( !p ) {
        //  SetError(VP_ERR_NOMEM);
        return FALSE;
    }


    m_net = MakeTapiNet( m_log, p, number, vphInst, fName, protoName[VPT_TAPI] );



    if ( !m_net ) {
        //  SetError(VP_ERR_NOMEM);
        delete p;
        return FALSE;
    }

    m_net->StartDataLog( _vpMakeDataLogger() );

    DWORD err = m_net->LastError();

    if ( err ) {
        delete m_net;
        delete p;

        m_net = NULL;
        return FALSE;
    }

    return TRUE;
#else
    return FALSE;
#endif
}





BOOL CVdmPlay::InitNetbios( LPCVOID data ) {
#ifdef _WIN32
    char stationName[NCBNAMSZ + 1];
    static char groupName[NCBNAMSZ + 1] = "VDMPLAY         ";
    WORD  lana = 255;
    LPCVPNETBIOSDATA nbData = (LPCVPNETBIOSDATA)data;


    stationName[0] = 0;
    if ( nbData ) {
        memcpy( stationName, nbData->stationName, NCBNAMSZ );
        lana = nbData->lana;
    }


    if ( !stationName[0] ) {
        LoadDefault(
            VPT_NETBIOS, "StationName", "",
            stationName, sizeof( stationName ) );
    }

    if ( lana == 255 )
        lana = (WORD)LoadDefaultInt( VPT_NETBIOS, "LANA", 255 );


    if ( lana == 255 ) {
        lana = CNbNet::ChooseLana();
    }


    CNbNet* net = new CNbNet( m_log, groupName, stationName, lana );

    net->SetWindow( m_window );

    if ( !net->Init() ) {
        delete net;
        return FALSE;
    }


    m_net = net;

    return TRUE;
#else
    (void)data; return FALSE;   // §7b: NetBIOS transport dropped on POSIX (TCP only)
#endif
}



BOOL CVdmPlay::InitDP( LPCVOID data ) {
#if defined(WIN32) && defined(WITH_DP)
    CNetInterface* MakeDpNet( CTDLogger * log, LPCSTR protocol, HWND wnd );
    char buf[64];

    buf[0] = 0;

    LoadDefault( VPT_DP, "Provider", "", buf, sizeof( buf ) );
    if ( strlen( buf ) ) {
        m_net = MakeDpNet( m_log, buf, m_window );
    }

    return m_net ? TRUE : FALSE;
#else
    return FALSE;
#endif

}



BOOL CVdmPlay::InitNetworkProtocol( int protocol, LPCVOID data ) {
    m_protocol = protocol;

    switch ( protocol ) {
    case VPT_TCP:
        return InitTcp( data );

    case VPT_IPX:
        return InitIpx( data );

    case VPT_COMM:
        return InitComm( data, FALSE );

    case VPT_MODEM:
        return InitComm( data, TRUE );


    case VPT_NETBIOS:
        return InitNetbios( data );

#ifdef WIN32  
    case VPT_TAPI:
        return InitTapi( data );
#ifdef  WITH_DP
    case VPT_DP:
        return InitDP( data );
#endif
#endif
    default:
        return FALSE;
    }




}



void CVdmPlay::CleanupWindowsStuff() {
    if ( m_timer ) {

        KillTimer( m_window, m_timer );
        m_timer = 0;
    }

    if ( m_window ) {
        DestroyWindow( m_window );
        m_window = 0;

    }
}

CWinNotifyQueue* CVdmPlay::MakeNotifyQueue( HWND wnd, UINT msg ) {
    CWinNotifyQueue* q = new CWinNotifyQueue( msg, wnd );

    if ( !q ) {
        //  SetError(VP_ERR_NOMEM);
    }

    return q;
}

#ifdef _WIN16   // Universal-Thunk notify-queue factory — Win16/Win32s only (n/a on Win32/POSIX)
CWinNotifyQueue* CUtVdmPlay::MakeNotifyQueue( HWND wnd, UINT msg ) {
    CUtNotifyQueue* q = new CUtNotifyQueue( msg, wnd );

    if ( !q ) {
        //  SetError(VP_ERR_NOMEM);
        VPTRACE( ( "CutVdmPlay::MakeNotiffyQueue new failed\n" ) );
        return NULL;
    }

    if ( !q->Ok() ) {
        VPTRACE( ( "CutVdmPlay::MakeNotiffyQueue q->Ok failed\n" ) );
        delete q;
        return NULL;
    }

    return q;
}

#endif 




CWinSession::~CWinSession() {
    if ( m_vdmPlay )
        m_vdmPlay->OnSessionDeath( m_sesObj );
}



CWinRemoteSession* CVdmPlay::MakeRemoteSession( HWND wnd, BOOL forEnum ) {
    CWinRemoteSession* MakeDpRemoteSession( CTDLogger * log, CNetInterface * net, CPlayerMap * pMap, CWSMap * wsMap, DWORD maxAge,
                                            CWinNotifyQueue * queue );

    m_queue = MakeNotifyQueue( wnd, WM_VPNOTIFY );

    if ( !m_queue )
        return NULL;

    DWORD maxAge;

    char fName[256];

    vpMakeIniFile( fName );


    maxAge = GetPrivateProfileInt( "VDMPLAY", "MaxServerAge", 30, fName );



    CWinRemoteSession* ses = NULL;

    if ( m_protocol == VPT_DP ) {
#ifdef WITH_DP
        ses = MakeDpRemoteSession( m_log, m_net,
                                   new CSimplePlayerMap,
                                   new CSimpleWSMap,
                                   maxAge * 1000,
                                   m_queue );
#endif

    } else {
        ses = new CWinRemoteSession( m_log, m_net,
                                     new CSimplePlayerMap,
                                     new CSimpleWSMap,
                                     maxAge * 1000,
                                     m_queue );
    }

    if ( !ses ) {
        delete m_queue;
        m_queue = NULL;
        return NULL;
    }

    ses->InitSessionInfo( &m_guid, m_version, m_sessionDataSize, m_playerDataSize );

    if ( !ses->InitNetwork( !forEnum ) ) {
        ses->CloseSession();
        delete ses;
        return NULL;
    }

    if ( !forEnum )
        m_net->SetHostWindow( wnd );

    ses->m_vdmPlay = this;

    return ses;
}

CWinRegSession* CVdmPlay::MakeRegSession( HWND wnd ) {
    m_queue = MakeNotifyQueue( wnd, WM_VPNOTIFY );

    if ( !m_queue )
        return NULL;

    DWORD maxAge;

    char fName[256];

    vpMakeIniFile( fName );

    maxAge = GetPrivateProfileInt( "VDMPLAY", "MaxServerAge", 30, fName );



    CWinRegSession* ses = new CWinRegSession( m_log, m_net,
                                              new CSimplePlayerMap,
                                              new CSimpleWSMap,
                                              maxAge * 1000,
                                              m_queue );

    if ( !ses )
        return NULL;

    ses->InitSessionInfo( &m_guid, m_version, m_sessionDataSize, m_playerDataSize );

    if ( !ses->InitNetwork( FALSE ) ) {
        ses->CloseSession();
        delete ses;
        return NULL;
    }

    // TCP-enum (phase-1): ALSO accept enum/queries over TCP on the well-known port,
    // so clients behind UDP-blocking routers / tunnels can reach the reg server.
    // Best-effort + additive — the UDP datagram enum (set up in InitNetwork) stays the
    // default; if the TCP listener can't bind we log and continue (UDP still serves).
    if ( !m_net->EnableStreamEnumListener() ) {
        if ( m_log )
            m_log->Log( "MakeRegSession: TCP-enum listener not enabled (UDP enum still active)" );
    }

    ses->m_vdmPlay = this;

    return ses;
}


// Periodic engine-timer drive (see vpint.h). Called from the 250ms WM_TIMER on Windows
// and from vpPumpNet on POSIX (which has no WM_TIMER). Same sequence either way.
void CVdmPlay::DriveTimers() {
    if ( m_queue )
        m_queue->RetryPosting();
    if ( m_net )
        m_net->OnTimer();
    if ( m_sEnum )
        m_sEnum->OnTimer();
    if ( m_pEnum )
        m_pEnum->OnTimer();
    if ( m_session )
        m_session->OnTimer();
}


CSessionEnum* CVdmPlay::MakeSessionEnum( HWND wnd, LPVOID userData ) {
    CWinRemoteSession* ses = MakeRemoteSession( wnd, TRUE );


    if ( !ses )
        return NULL;

    DWORD period = VP_SERVER_ENUM_PERIOD;


    CSessionEnum* senum = new CSessionEnum( ses, ses, period );

    if ( !senum ) {
        ses->CloseSession();
        delete ses;
        return NULL;
    }


    senum->SetUserData( userData );
    return senum;
}

CSessionEnum* CVdmPlay::MakeSessionRegistery( HWND wnd, LPVOID userData ) {
    CWinRegSession* ses = MakeRegSession( wnd );

    if ( !ses )
        return NULL;


    CSessionEnum* senum = new CSessionEnum( ses, ses );

    if ( !senum ) {
        ses->CloseSession();
        delete ses;
        return NULL;
    }


    senum->SetUserData( userData );
    return senum;
}






CWinLocalSession* CVdmPlay::MakeLocalSession( HWND wnd, LPCSTR sessionName, DWORD sessioNFlags, LPVOID userData ) {
    m_queue = MakeNotifyQueue( wnd, WM_VPNOTIFY );

    if ( !m_queue )
        return NULL;

    CWinLocalSession* ses = new CWinLocalSession( m_log, m_net,
                                                  new CSimplePlayerMap,
                                                  new CSimpleWSMap,
                                                  m_queue );

    if ( !ses )
        return NULL;

    ses->InitSessionInfo( &m_guid, m_version, m_sessionDataSize, m_playerDataSize );

    if ( !ses->InitNetwork( TRUE ) ) {
        ses->CloseSession();
        delete ses;
        return NULL;
    }

    m_net->SetHostWindow( wnd );
    ses->UpdateSessionInfo( (LPVOID)sessionName );
    ses->m_vdmPlay = this;


    return ses;
}


void vpassertion( LPCSTR text, LPCSTR file, int line ) {
    char buf[512];
#ifndef NDEBUG
    wsprintf( buf, "Assertion (%s) in file %s at line %d", text, file, line );
    MessageBox( NULL, buf, "VDMPLAY", MB_OK );
#endif
    if ( gBreakOnAssert ) {
        __debugbreak();   // was `__asm int 3` — x64 has no inline asm
    }

#if defined (WIN32)
    throw ( 1 << 28 );
#else
    abort();
#endif
}

void vptracemsg( LPCSTR text ) {
    OutputDebugString( text );
}


#ifndef NDEBUG
int VPTracer::DoTrace( LPCSTR fmt, ... ) {
    va_list ap;
    char buf[1024];
    int offset = 0;

    buf[0] = 0;

    if ( gTraceLocation && m_file ) {
        wsprintf( buf, "%s:%d - ", m_file, m_line );
    }

    va_start( ap, fmt );
    wvsprintf( buf + strlen( buf ), fmt, ap );
    strcat( buf, "\n" );

    if ( _vptracemsg )
        _vptracemsg( buf );

    return ( 0 );

}

#endif







extern "C"
{

    DWORD VPAPI vpGetVersion() {
        return ( MAKELONG( VER_RELEASE, MAKEWORD( VER_MINOR, VER_MAJOR ) ) );
    }

#ifndef _WIN32
    // POSIX MP pump (step a): the game's main loop calls this each idle pass to
    // drive the select() pump that replaces WSAAsyncSelect (on Windows the hidden
    // message window did this implicitly). FD_* events become OnMessage calls
    // whose notifications are PostMessage'd as WM_VPNOTIFY and drained by the same
    // loop -> CNetApi::OnNetMsg. Returns the number of events dispatched; a no-op
    // (returns 0) when no MP sockets are armed, so single-player pays nothing.
    // POSIX engine-timer drive. On Windows the engine OnTimers (enum re-poll, host
    // periodic re-registration, server-list aging, notify-queue retry that delivers enum
    // replies to the app) run off a 250ms WM_TIMER. POSIX has no message window / WM_TIMER
    // (SetTimer is a no-op stub, m_window==NULL), so those NEVER fired on Linux/mac —
    // breaking enum re-poll, re-register, aging, AND enum-reply delivery (linux1 root-cause
    // 2026-06-26). Fix: a tiny registry of live CVdmPlay handles, throttle-driven from
    // vpPumpNet (the game's net-service entry) on the same ~250ms cadence. Additive.
    static CVdmPlay* g_vpActive[8] = { 0 };
    static void vpRegisterActive( CVdmPlay* vp ) {
        for ( int i = 0; i < 8; ++i ) if ( !g_vpActive[i] ) { g_vpActive[i] = vp; return; }
    }
    static void vpUnregisterActive( CVdmPlay* vp ) {
        for ( int i = 0; i < 8; ++i ) if ( g_vpActive[i] == vp ) { g_vpActive[i] = 0; return; }
    }

    int VPAPI vpPumpNet( int timeout_ms ) {
        int n = vpNetPumpPosix( timeout_ms );

        // Drive the engine timers on a ~250ms cadence (POSIX has no WM_TIMER to do it).
        static DWORD lastDrive = 0;
        DWORD now = GetCurrentTime();
        if ( now - lastDrive >= 250 ) {
            lastDrive = now;
            for ( int i = 0; i < 8; ++i )
                if ( g_vpActive[i] )
                    g_vpActive[i]->DriveTimers();
        }
        return n;
    }
#endif



    DWORD VPAPI vpSupportedTransports() {
        DWORD result = 0;

        // TCP/IP ONLY. We deliberately report only the TCP transport:
        //  - Multiplayer is TCP/IP only by design.
        //  - The NetBIOS probe (CNbNet::Supported -> GetLanas -> NCBASTAT) corrupts
        //    the stack on x64 (the custom NCB struct in nbnet doesn't match the
        //    Windows SDK NCB the OS Netbios() writes back), which fired the /RTC
        //    "Debug Error" crash when hosting/joining a network game.
        //  - Reporting a single transport also removes the spurious "multiple
        //    protocols will cause issues" warning.
        // The old IPX / TAPI / DirectPlay / COMM / NetBIOS probes are left here
        // (disabled) for reference.
        if ( CWinTcpNet::Supported() )
            result |= 1 << VPT_TCP;

#if 0  // non-TCP transports disabled — see note above
#ifdef WIN32
        if ( CWinIpxNet::Supported() )
            result |= 1 << VPT_IPX;

        if ( IsTapiSupported() )
            result |= ( 1 << VPT_TAPI );
#ifdef WITH_DP
        if ( IsDpSupported() )
            result |= ( 1 << VPT_DP );
#endif

#else
        if ( CIpx16Net::Supported() )
            result |= 1 << VPT_IPX;
#endif

#ifdef WITH_COMM
        result |= 1 << VPT_COMM;
        result |= 1 << VPT_MODEM;
#endif

        if ( CNbNet::Supported() )
            result |= 1 << VPT_NETBIOS;
#endif  // 0

        return result;
    }





    VPHANDLE VPAPI vpStartup( IN DWORD version,
                              IN LPCVPGUID guid,
                              IN DWORD sessionDataSize,
                              IN DWORD playerDataSize,
                              IN UINT protocol, IN LPCVOID protocolData )

    {

        if ( vpReentrancyCounter )
            return NULL;

        vpMemPoolInit();
        CVdmPlay* vp = new CVdmPlay;

        if ( !vp ) {
            vpMemPoolCleanup();
            return NULL;
        }


        gErrLogger = vp->m_log = new CVdmErrorLogger( vp );



        vp->Startup( version, guid, sessionDataSize, playerDataSize, protocol, protocolData );


        if ( !vp->Ok() ) {
            delete vp;
            vpMemPoolCleanup();
            return NULL;
        }

#ifndef _WIN32
        vpRegisterActive( vp );   // so vpPumpNet drives its engine timers (no WM_TIMER on POSIX)
#endif

        return (VPHANDLE)vp;
    }


#ifdef _WIN16   // Win16/Win32s Universal-Thunk machinery (vpUtStartup + CUtNotifyQueue) — n/a on Win32/POSIX

    CUtNotifyQueue::PostMessage( WPARAM wParam, LPARAM lParam ) {
        LPUTMSG utMsg = m_freeList;

        if ( !utMsg )
            return NULL;

        m_freeList = utMsg->u.next;

        LPVPMESSAGE oMsg = (LPVPMESSAGE)lParam;
        utMsg->u.original = oMsg;
        utMsg->msg = *oMsg;

        LPVOID data = utMsg->msg.u.data;


        if ( data ) {
            GlobalFix( (HGLOBAL)SELECTOROF( data ) );
            utMsg->msg.u.data = pUTSelectorOffsetToLinear( (LPBYTE)data );
        }

        BOOL ret = CWinNotifyQueue::PostMessage( wParam, (LPARAM)pUTSelectorOffsetToLinear( (LPBYTE)utMsg ) );

        VPTRACE( ( "CUtNotifyQueue::PostMessage(%ld)=%d\n",
                   VPGETNOTIFICATION( utMsg->msg.notificationCode ), ret ) );


        if ( !ret ) {
            GlobalUnfix( (HGLOBAL)SELECTOROF( data ) );
            m_freeList = utMsg;
            utMsg->u.next = m_freeList;
        }

        return ret;
    }


    // startup for UT
    VPHANDLE vpUtStartup( IN DWORD version,
                          IN LPCVPGUID guid,
                          IN DWORD sessionDataSize,
                          IN DWORD playerDataSize,
                          IN UINT protocol, IN LPCVOID protocolData )

    {

        if ( vpReentrancyCounter )
            return NULL;

        hUtLib = LoadLibrary( "WIN32S16.DLL" );
        if ( !hUtLib ) {
            MessageBox( NULL, "VDMPLAY", "WIN32s is not installed on this machine", MB_OK );
            return FALSE;
        }

        *( (FARPROC*)&pUTLinearToSelectorOffset ) =
            GetProcAddress( hUtLib, "UTLINEARTOSELECTOROFFSET" );
        *( (FARPROC*)&pUTSelectorOffsetToLinear ) =
            GetProcAddress( hUtLib, "UTSELECTOROFFSETTOLINEAR" );

        if ( !pUTLinearToSelectorOffset || !pUTSelectorOffsetToLinear ) {
            MessageBox( NULL, "VDMPLAY", "Cannot bind to 16 WIN32 helper routines", MB_OK );
            FreeLibrary( hUtLib );
            hUtLib = NULL;
            return FALSE;
        }

        CVdmPlay* vp = new CUtVdmPlay;


        if ( !vp )
            return NULL;

        vp->m_log = new CVdmErrorLogger( vp );

#ifdef _WIN16
        SetMessageQueue( 64 );   // Win16 only — enlarge msg queue (n/a on Win32/POSIX)
#endif

        vp->Startup( version, guid, sessionDataSize, playerDataSize, protocol, protocolData );


        if ( !vp->Ok() ) {
            delete vp;
            return NULL;
        }


        return (VPHANDLE)vp;
    }

#endif



    BOOL VPAPI vpCleanup( IN VPHANDLE pHdl ) {
        if ( vpReentrancyCounter ) {
            vpDoAbortWait();
            return FALSE;
        }


        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
#ifndef _WIN32
            vpUnregisterActive( vp );
#endif
            vp->Cleanup();
            delete vp;
        }
        vpMemPoolCleanup();
#ifndef WIN32
        if ( hUtLib ) {
            FreeLibrary( hUtLib );
            hUtLib = NULL;
        }
#endif 
        return TRUE;
    }


    BOOL VPAPI vpEnumSessions( IN VPHANDLE pHdl,
                               IN HWND hWnd,
                               IN BOOL dontAutoStop, // if FALSE will AutoStop enum
                               IN LPCVOID userData ) {
        if ( vpReentrancyCounter )
            return FALSE;

        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            if ( !vp->GotFatalError() )
                return vp->EnumSessions( hWnd, dontAutoStop, (LPVOID)userData );
        }

        return FALSE;
    }



    // Must be called if dontAutoStop was specified in the eEnumSessions call
    BOOL VPAPI vpStopEnumSessions( IN VPHANDLE pHdl ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            return vp->StopEnumSessions();
        }

        return FALSE;
    }

    BOOL VPAPI vpStartRegistrationServer( IN VPHANDLE pHdl,
                                          IN HWND hWnd,
                                          IN LPCVOID userData ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            if ( !vp->GotFatalError() )
                return vp->StartRegistrationServer( hWnd, (LPVOID)userData );
        }

        return FALSE;
    }



    // Must be called if dontAutoStop was specified in the eEnumSessions call
    BOOL VPAPI vpStopRegistrationServer( IN VPHANDLE pHdl ) {
        if ( vpReentrancyCounter )
            return FALSE;

        return vpStopEnumSessions( pHdl );
    }





    BOOL VPAPI vpEnumPlayers( IN VPHANDLE pHdl,
                              IN HWND hWnd,
                              IN LPCVPSESSIONID sessionId,
                              IN LPCVOID userData ) {
        if ( vpReentrancyCounter )
            return FALSE;

        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            if ( !vp->GotFatalError() )
                return vp->EnumPlayers( hWnd, sessionId, (LPVOID)userData );
        }

        return FALSE;
    }


    VPSESSIONHANDLE VPAPI vpCreateSession( IN VPHANDLE pHdl,
                                           IN HWND hWnd,
                                           IN LPCSTR sessionName,
                                           IN DWORD sessionFlags,
                                           IN LPCVOID userData ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            if ( !vp->GotFatalError() )
                return vp->CreateSession( hWnd, sessionName, sessionFlags, (LPVOID)userData );
        }

        return FALSE;

    }

    // Use it to get info on the session created/joined  by you     
    BOOL VPAPI vpGetSessionInfo( IN VPSESSIONHANDLE pSesHdl, OUT LPVPSESSIONINFO sInfo ) {
        if ( vpReentrancyCounter )
            return FALSE;

        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            if ( ses->IsValid() )
                return ses->GetSessionInfo( sInfo );
        }

        return FALSE;

    }



    // Use it to get an info about a session that is not joined/created by you
    BOOL VPAPI vpGetSessionInfoById( IN VPHANDLE pHdl,
                                     IN HWND hWnd,
                                     IN LPCVPSESSIONID sessionId,
                                     LPCVOID userData ) {
        if ( vpReentrancyCounter )
            return FALSE;
#if 0
        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            return vp->GetSessionInfoById( hWnd, sessionId, (LPVOID)userData );
        }
#endif
        return FALSE;
    }


    // This will work only for the session created by the caller
    BOOL VPAPI vpUpdateSessionData( IN VPSESSIONHANDLE pSesHdl, IN LPCVOID sessionData ) {
        if ( vpReentrancyCounter )
            return FALSE;

        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            if ( ses->IsValid() )
                return ses->UpdateSessionInfo( (LPVOID)sessionData );
        }

        return FALSE;
    }



    VPSESSIONHANDLE VPAPI vpJoinSession( IN VPHANDLE pHdl,
                                         IN HWND hWnd,
                                         IN LPCVPSESSIONID sessionId,
                                         IN LPCSTR playerName,
                                         IN DWORD  playerFlags,
                                         IN LPCVOID userData ) {
        if ( vpReentrancyCounter )
            return NULL;
        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            if ( !vp->GotFatalError() )
                return vp->JoinSession( hWnd, sessionId, playerName, playerFlags, (LPVOID)userData );
        }

        return NULL;
    }



    // Add a player to the session, this will work only for 
    // the session created on this machine
    BOOL VPAPI vpAddPlayer( IN VPSESSIONHANDLE pSesHdl,
                            IN LPCSTR playerName,
                            IN DWORD  playerFlags,
                            IN LPCVOID userData,
                            OUT LPVPPLAYERID playerId ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            if ( ses->IsValid() )
                return ses->AddLocalPlayer( playerName, playerFlags, (LPVOID)userData, playerId );
        }

        return FALSE;
    }



    BOOL VPAPI vpSendData( IN VPSESSIONHANDLE pSesHdl,
                           IN VPPLAYERID toPlayerId,
                           IN VPPLAYERID fromPlayerId,
                           IN LPCVOID data, IN DWORD dataLen,
                           IN DWORD sendFlags,   // these are from enum VPSENDFLAGS
                           LPCVOID userData ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            if ( ses->IsValid() )
                return ses->SendData( toPlayerId, fromPlayerId, (LPVOID)data, dataLen, sendFlags, (LPVOID)userData );
        }

        return FALSE;
    }



    BOOL VPAPI vpCloseSession( IN VPSESSIONHANDLE pSesHdl, IN LPCVOID userData ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            BOOL ret = ses->CloseSession();
            delete ses;
            return ret;
        }

        return FALSE;
    }


    DWORD VPAPI vpGetAddressString( IN VPHANDLE pHdl, IN LPCVPNETADDRESS addr, LPSTR buf, DWORD bufSize ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            if ( !vp->GotFatalError() )
                return vp->GetAddressString( addr, buf, bufSize );
        }

        return ~0u;
    }



    BOOL VPAPI vpGetAddress( IN VPHANDLE pHdl, OUT LPVPNETADDRESS addr ) {
        if ( vpReentrancyCounter )
            return FALSE;

        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            if ( !vp->GotFatalError() )
                return vp->GetAddress( addr );
        }


        return FALSE;
    }


    // this should be called after processing the WM_VPNOTIFY message 
    BOOL VPAPI vpAcknowledge( IN VPHANDLE pHdl, LPCVPMESSAGE pMsg ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pHdl && pMsg ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            CNotification* notification = vp->m_queue->RecoverNotification( pMsg );
            notification->Complete();


            if ( vp->m_log ) {
                DWORD ct = timeGetTime();
                DWORD tDiff = ct - notification->m_vpmsg.postTime;
                char logBuf[64];
                char* p = NULL;

                if ( tDiff > 5000 ) {
                    wsprintf( logBuf, "Late acked notification (%lu,%lu)",
                              ct - notification->m_vpmsg.createMsTime, tDiff );

                    p = logBuf;
                } else if ( notification->m_vpmsg.postTime - notification->m_vpmsg.createMsTime > 250 ) {
                    wsprintf( logBuf, "Postponed notification: %lu msec",
                              notification->m_vpmsg.postTime - notification->m_vpmsg.createMsTime );
                    p = logBuf;

                }

                if ( p )
                    vp->m_log->Log( p );
            }

            delete notification;
            return  TRUE;
        }

        return FALSE;
    }







    // One of these MUST be called in responce to VP_JOIN 
    // for NOAUTOJOIN sessions
    BOOL VPAPI vpInvitePlayer( IN VPSESSIONHANDLE pSesHdl, VPPLAYERID playrId ) {
        if ( vpReentrancyCounter )
            return FALSE;
        return FALSE;
    }

    BOOL VPAPI vpRejectPlayer( IN VPSESSIONHANDLE pSesHdl, VPPLAYERID playrId ) {
        if ( vpReentrancyCounter )
            return FALSE;
        return FALSE;
    }

    BOOL VPAPI vpKillPlayer( IN VPSESSIONHANDLE pSesHdl, IN VPPLAYERID playerId ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            if ( ses->IsValid() )
                return ses->KillPlayer( playerId );
        }
        return FALSE;
    }

    BOOL VPAPI vpSetSessionVisibility(
        IN VPSESSIONHANDLE pSesHdl,  // Handle returned by vpCreateSession
        IN BOOL visibility   //  If set to FALSE the server stops responding to to server enumeration requests
    ) {
        if ( vpReentrancyCounter )
            return FALSE;

        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            if ( ses->IsValid() )
                return ses->SetVisibility( visibility );
        }
        return FALSE;
    }



    // Start File Transfer
    BOOL VPAPI vpStartFT( IN VPSESSIONHANDLE pSesHdl, // session handle 
                          INOUT LPVPFTINFO ftInfo     // file transfer info
    ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            if ( ses->IsValid() )
                return ses->StartFT( ftInfo );
        }
        return FALSE;
    }



    // Accept incoming file transfer request
    BOOL vpAcceptFT( IN VPSESSIONHANDLE pSesHdl, // session handle
                     INOUT LPVPFTINFO ftInfo  // file transfer info
    ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            if ( ses->IsValid() )
                return ses->AcceptFT( ftInfo );
        }
        return FALSE;
    }


    // Send a file  data block
    BOOL VPAPI vpSendBlock( IN VPSESSIONHANDLE pSesHdl, // session handle 
                            IN LPVPFTINFO ftInfo,  // file transfer info
                            IN LPCVOID buf,
                            IN DWORD bufSize
    ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            if ( ses->IsValid() )
                return ses->SendBlock( ftInfo, buf, bufSize );
        }
        return FALSE;
    }

    // Receive a file data block
    BOOL VPAPI vpGetBlock( IN VPSESSIONHANDLE pSesHdl,  // session handle 
                           INOUT LPVPFTINFO ftInfo,    // file transfer info
                           OUT LPVOID buf,
                           IN  DWORD bufSize
    ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            if ( ses->IsValid() )
                return ses->GetBlock( ftInfo, buf, bufSize );
        }
        return FALSE;
    }

    // Terminate a File Transfer
    BOOL VPAPI vpStopFT( IN VPSESSIONHANDLE pSesHdl, // session handle 
                         INOUT LPVPFTINFO ftInfo // file transfer info
    ) {
        if ( vpReentrancyCounter )
            return FALSE;
        if ( pSesHdl ) {
            CSession* ses = (CSession*)pSesHdl;
            if ( ses->IsValid() )
                return ses->StopFT( ftInfo );
        }
        return FALSE;
    }

    void VPAPI vpAbortWait( VPHANDLE pHdl ) {
        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            PostMessage( vp->m_window, WM_ABORTWAIT, 0, 0 );
        }
    }



    void InitMfcStuff();

    void VPAPI vpAdvDialog( HWND hWnd, int iProtocol, BOOL bServer ) {
#ifdef _WIN32
        InitMfcStuff();
        doAdvDialog( hWnd, iProtocol, bServer );
#else
        // POSIX MP port: the advanced TCP/IP-settings dialog is MFC GUI
        // (advdlg.cpp/advanced.cpp, not built on POSIX). The SDL2 UI drives MP
        // setup, so this exported entry is a no-op here.
        (void)hWnd; (void)iProtocol; (void)bServer;
#endif
    }


    BOOL VPAPI vpGetServerAddress(
        IN VPHANDLE pHdl,    // Handle returned by vpStartup
        OUT LPVPNETADDRESS addr      // Where to store the addreess
    ) {
        if ( vpReentrancyCounter )
            return FALSE;

        if ( pHdl ) {
            CVdmPlay* vp = (CVdmPlay*)pHdl;
            if ( !vp->GotFatalError() )
                return vp->GetServerAddress( addr );
        }


        return FALSE;
    }


#ifndef _USRDLL


#ifdef WIN32


#ifndef NOMFC
#pragma message("WIN32 MFC NOT _USRDLL ")

    static AFX_EXTENSION_MODULE vdmplayDLL = { NULL, NULL };

    void InitMfcStuff() {
        static CDynLinkLibrary* pDll = NULL;

        if ( pDll == NULL )
            pDll = new CDynLinkLibrary( vdmplayDLL );

    }
#else
#pragma message("WIN32 NOMFC")

    void InitMfcStuff() {}

#endif

    BOOL WINAPI DllMain(
        HINSTANCE  hinstDLL, // handle of DLL module 
        DWORD  fdwReason, // reason for calling function 
        LPVOID  lpvReserved  // reserved 
    ) {
        if ( fdwReason == DLL_PROCESS_ATTACH ) {

            TRACE0( "VDMPLAY.DLL Initializing!\n" );

#ifndef NOMFC  
            // Extension DLL one-time initialization
            if ( !AfxInitExtensionModule( vdmplayDLL, hinstDLL ) ) {
                TRACE0( "VDMPLAY:  AfxInitExtensionModule failed!\n" );
                return FALSE;
            }

            // Insert this DLL into the resource chain
            new CDynLinkLibrary( vdmplayDLL );
#endif

            vphInst = hinstDLL;
            char fName[256];



            // Default to local-directory ini so port/address writes from the
            // game (WritePrivateProfileString "vdmplay.ini") and VDMPLAY reads
            // land in the same file.  Overridable via [VDMPLAY] LocalIni=0 in
            // the Windows-directory VDMPLAY.INI.
            gLocalIni = TRUE;
            vpMakeIniFile( fName );

            gLocalIni = GetPrivateProfileInt( "VDMPLAY", "LocalIni", 1, fName );

            vpMakeIniFile( fName );

            gBreakOnAssert = GetPrivateProfileInt( "VDMPLAY", "BreakOnAssert", 0, fName );
            gUseLogfile = GetPrivateProfileInt( "VDMPLAY", "UseLogFile", 0, fName );
            gLogWinsock = GetPrivateProfileInt( "VDMPLAY", "LogWinsock", 0, fName );
            vpLoadWinsockDll();
#ifdef WITH_DP
            vpLoadDp();
#endif
        } else if ( fdwReason == DLL_PROCESS_DETACH ) {
            vpUnloadWinsockDll();
#ifdef WITH_DP
            vpUnloadDp();
#endif
            CleanupWsThunks();
            TRACE0( "VDMPLAY.DLL Terminating!\n" );
#ifndef NOMFC
            AfxTermExtensionModule( vdmplayDLL );
#endif

        }


        return TRUE;
    }
#else

#endif // WIN32


#else



#ifdef WIN32
#define DLLNAME "vdmplay.dll"
#else
#define DLLNAME "vdmpl16.dll"
#endif

    void InitMfcStuff() {}


    class CVdmPlayDll: public CWinApp {

    public:
        CVdmPlayDll();

        BOOL InitInstance();
        int ExitInstance();


        // Overrides
         // ClassWizard generated virtual function overrides
         //{{AFX_VIRTUAL(CVdmPlayDll)
         //}}AFX_VIRTUAL

         //{{AFX_MSG(CVdmPlayDll)
          // NOTE - the ClassWizard will add and remove member functions here.
          //    DO NOT EDIT what you see in these blocks of generated code !
         //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

    };

    BEGIN_MESSAGE_MAP( CVdmPlayDll, CWinApp )
        //{{AFX_MSG_MAP(CVdmPlayDll)
         // NOTE - the ClassWizard will add and remove mapping macros here.
         //    DO NOT EDIT what you see in these blocks of generated code!
        //}}AFX_MSG_MAP
    END_MESSAGE_MAP()


    CVdmPlayDll::CVdmPlayDll(): CWinApp( DLLNAME ) {}

    BOOL CVdmPlayDll::InitInstance() {

        TRACE0( "VDMPLAY.DLL Initializing!\n" );

        vphInst = AfxGetInstanceHandle();
        char fName[256];



        // Default to the local-directory ini so the WellKnownPort/ServerAddress
        // the game writes to ".\vdmplay.ini" are actually read (otherwise this
        // read the Windows-dir VDMPLAY.INI and silently fell back to defaults,
        // e.g. the old 1707 port -> host/client port mismatch -> no discovery).
        gLocalIni = TRUE;
        vpMakeIniFile( fName );

        gLocalIni = GetPrivateProfileInt( "VDMPLAY", "LocalIni", 1, fName );

        gBreakOnAssert = GetPrivateProfileInt( "VDMPLAY", "BreakOnAssert", 0, fName );

        gUseLogfile = GetPrivateProfileInt( "VDMPLAY", "UseLogFile", 0, fName );
        gLogWinsock = GetPrivateProfileInt( "VDMPLAY", "LogWinsock", 0, fName );
        return TRUE;

    }

    int CVdmPlayDll::ExitInstance() {
        CleanupWsThunks();
        TRACE0( "VDMPLAY.DLL Terminating!\n" );
        return CWinApp::ExitInstance();
    }

    CVdmPlayDll  theApp;


#endif    // _USRLDLL


#if defined(NOMFC) && !defined(WIN32)
#pragma message("NOMFC WIN16")

    void InitMfcStuff() {}


    int FAR PASCAL LibMain( HANDLE hLibInst, WORD wDataSeg,
                            WORD cbHeapSize, LPSTR lpszCmdLine ) {
        char fName[256];
        vphInst = (HINSTANCE)hLibInst;
        gLocalIni = FALSE;
        vpMakeIniFile( fName );

        gLocalIni = GetPrivateProfileInt( "VDMPLAY", "LocalIni", 0, fName );
        vpMakeIniFile( fName );

        gBreakOnAssert = GetPrivateProfileInt( "VDMPLAY", "BreakOnAssert", 0, fName );
        gUseLogfile = GetPrivateProfileInt( "VDMPLAY", "UseLogFile", 0, fName );
        gLogWinsock = GetPrivateProfileInt( "VDMPLAY", "LogWinsock", 0, fName );
        return ( 1 );
    } // LibMain()


#endif

}
