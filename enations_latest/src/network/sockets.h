#ifndef __SOCKETS_H__
#define __SOCKETS_H__

// CSockets — portable (winsock/BSD) TCP+UDP implementation of CProtocol, the
// cross-platform replacement for CNetbios. Wired as the NET_PROTO_TCP path in
// naInit (davenet.cpp). See docs/plans/multiplayer-cross-platform.md.
//
// Model (mirrors CNetbios's async NCB semantics):
//  - NetBIOS "names" -> a logical name table; Call/Listen -> TCP connect/accept;
//    Send/Receive -> length-prefixed framed I/O over a TCP session; SendDatagram/
//    ReceiveDatagram -> UDP (LAN broadcast for host discovery). Session "numbers"
//    (the NETMSG bNetNum / NetBIOS lsn) index a session table.
//  - Every op is asynchronous: it's queued to a background net thread that runs
//    select() over the listen/session/UDP sockets. On completion the thread fills a
//    NETMSG (bCmd/bErr/bNetNum/iLen/pData/pUser/sName) and pushes it to a thread-safe
//    completion queue. The main loop drains that queue each frame and dispatches each
//    NETMSG to the game's net handler — this REPLACES the dead WM_NET_COMPLETE/HWND
//    path (no Win32 message pump dependency; works on SDL2/POSIX). See DrainCompletions().

#include "_davenet.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET en_socket_t;
  #define EN_INVALID_SOCKET INVALID_SOCKET
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int en_socket_t;
  #define EN_INVALID_SOCKET (-1)
#endif

#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <vector>
#include <string>

const int SOCK_MAX_SESSIONS = 62;     // match NUM_NCBS so callers' assumptions hold
const int SOCK_NAME_MAX     = NAME_MAX; // davenet.h NAME_MAX (14)

class CSockets : public CProtocol {
public:
        CSockets (HINSTANCE hInst, HWND hWnd);
        ~CSockets ();

        static BOOL Have ();          // sockets are always available
        BOOL InitOk ();

        void ErrMsgBox (NETMSG * pMsg);
        void Close ();
        BOOL AddName (LPCSTR pName, LPCVOID pData);
        BOOL AddGroupName (LPCSTR pName, LPCVOID pData);
        BOOL Call (LPCSTR pLocal, LPCSTR pRemote, LPCVOID pUser);
        void CancelReceive (int iNum);
        void CancelReceiveDatagram (int iNum);
        BOOL DeleteName (LPCSTR pName, LPCVOID pData);
        BOOL HangUp (int iNum, LPCVOID pUser);
        BOOL Listen (LPCSTR pLocal, LPCSTR pRemote, LPCVOID pUser);
        BOOL Receive (int iNum, LPCVOID pUser);
        BOOL ReceiveDatagram (int iNum, LPCVOID pUser);
        BOOL Send (int iNum, LPCVOID pData, int iLen, LPCVOID pUser);
        BOOL SendDatagram (int iNum, LPCSTR pName, LPCVOID pData, int iLen, LPCVOID pUser);

        // Completion delivery (the rebuilt WM_NET_COMPLETE path). The main loop calls
        // this each frame; it pops completed NETMSGs and dispatches them to the game's
        // net handler. Returns the number dispatched.
        int  DrainCompletions ();

protected:
        struct Session {
            en_socket_t sock = EN_INVALID_SOCKET;
            bool        inUse = false;
            // length-prefixed receive framing state, pending Receive() user ptr, etc.
        };

        en_socket_t              m_listen = EN_INVALID_SOCKET;   // TCP accept socket
        en_socket_t              m_udp    = EN_INVALID_SOCKET;   // UDP datagram/discovery
        Session                  m_sess[SOCK_MAX_SESSIONS];
        std::string              m_localName;                    // our AddName

        // background net thread + thread-safe completion queue
        std::thread              m_netThread;
        std::atomic<bool>        m_run{false};
        std::mutex               m_qLock;
        std::deque<NETMSG>       m_completions;                  // filled by net thread, drained by main loop

        void NetThreadProc ();                                   // select() loop
        void PushCompletion (const NETMSG & msg);                // net thread -> queue
        static BOOL InitSocketLib ();                            // WSAStartup on win, no-op POSIX
};

#endif // __SOCKETS_H__
