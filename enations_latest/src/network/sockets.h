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
#include <map>

const int SOCK_MAX_SESSIONS = 62;     // match NUM_NCBS so callers' assumptions hold
const int SOCK_NAME_MAX     = NET_NAME_MAX; // davenet.h NET_NAME_MAX (14)
const unsigned short SOCK_DISCOVERY_PORT = 0xE4E4; // 58596 — UDP LAN host-discovery (next increment)

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
        // net handler. Returns the number dispatched. NOTE: for NET_MSG_RECEIVE /
        // NET_MSG_RECEIVE_DATAGRAM completions, NETMSG.pData is a malloc'd buffer of
        // iLen bytes that the consumer must free() after handling (mirrors the original
        // NCB buffer lifetime, but heap-owned per message for the queued model).
        int  DrainCompletions ();

        // Pop a single completed NETMSG (FALSE if none). Used by DrainCompletions and the
        // loopback self-test; lets callers drive completions without a game handler wired yet.
        BOOL PopCompletion ( NETMSG * pOut );

        // Loopback smoke test (plan P1 gate "unit-smoke: loopback send/recv"). Spins up a
        // server+client CSockets pair on 127.0.0.1, exercises Listen/Call/Send/Receive AND
        // the UDP SendDatagram/ReceiveDatagram path, draining completions to verify both
        // round-trips. Returns TRUE on success. Self-contained (no game wiring); safe to
        // call from a test driver on any platform.
        static BOOL SelfTest ();

protected:
        struct Session {
            en_socket_t              sock        = EN_INVALID_SOCKET;
            bool                     inUse       = false;
            std::string              rxbuf;                     // raw bytes accumulated off the stream
            std::deque<std::string>  rxmsgs;                    // complete length-framed messages awaiting Receive()
            bool                     recvPending = false;       // a Receive() is posted for this session
            void *                   recvUser    = nullptr;     // pUser to echo back on the Receive completion
        };

        struct PendingListen {                                  // a posted Listen() awaiting an inbound connection
            std::string local, remote;
            void *      user = nullptr;
        };

        en_socket_t              m_listen = EN_INVALID_SOCKET;   // TCP accept socket
        en_socket_t              m_udp    = EN_INVALID_SOCKET;   // UDP datagram/discovery
        unsigned short           m_listenPort = 0;               // bound TCP port (ephemeral)
        unsigned short           m_udpPort    = 0;               // bound UDP port (ephemeral)
        Session                  m_sess[SOCK_MAX_SESSIONS];
        std::deque<PendingListen> m_listens;                     // posted listens awaiting accept
        std::string              m_localName;                    // our AddName

        // UDP datagram receive state (datagrams aren't session-bound; one logical inbox).
        // A datagram on the wire is [NET_NAME_MAX src-name][payload]; the name is delivered
        // back in NETMSG.sName and the payload in pData (mirrors NetBIOS DGRECV).
        bool                     m_dgPending = false;            // a ReceiveDatagram() is posted
        void *                   m_dgUser    = nullptr;          // pUser to echo on the datagram completion
        std::deque<std::pair<std::string,std::string>> m_dgQueue; // (srcName, payload) awaiting ReceiveDatagram

        // background net thread + thread-safe completion queue
        std::thread              m_netThread;
        std::atomic<bool>        m_run{false};
        std::mutex               m_qLock;                        // guards m_completions
        std::mutex               m_sessLock;                     // guards m_sess / m_listens (net thread vs caller)
        std::deque<NETMSG>       m_completions;                  // filled by net thread, drained by main loop

        void NetThreadProc ();                                   // select() loop
        void PushCompletion ( const NETMSG & msg );             // net thread -> queue
        void EnsureNetThread ();                                 // start the net thread on first use
        int  AllocSession ( en_socket_t s );                    // claim a session slot for socket s (-1 if full)
        BOOL EnsureListenSocket ();                             // create+bind+listen m_listen (ephemeral port)
        BOOL EnsureUdpSocket ();                                // create+bind m_udp (ephemeral port) for datagrams
        BOOL ResolveName ( LPCSTR pName, struct sockaddr_in * pAddr, bool bUdp ); // name/registry/"ip[:port]" -> address
        void HandleReadable ( int iSess );                      // net thread: drain a session socket, frame, deliver
        void HandleUdpReadable ();                             // net thread: recvfrom the UDP socket, deliver datagram
        static BOOL InitSocketLib ();                            // WSAStartup on win, no-op POSIX
};

#endif // __SOCKETS_H__
