// vp_sock_posix.h — POSIX (Linux/macOS) BSD-socket compat for the VDMPLAY vp* TCP engine.
//
// MP cross-platform port (Task#2, plan §7b). On POSIX we do NOT use the winsock
// function-pointer thunk table (USE_VPWINSOCK is left undefined in vpwinsk.h's #else
// branch), so the engine's plain socket()/connect()/bind()/recv()/sendto()/... calls
// bind directly to the real BSD socket functions — the names are identical. This header
// supplies only the handful of winsock-isms the engine uses that BSD spells differently.
//
// The one genuinely Windows-specific call, WSAAsyncSelect (message-based async I/O), is
// NOT emulated here — it is replaced by a select()/poll() event loop in port step 5; this
// header provides a link-satisfying stub so the TCP subset compiles meanwhile.
#ifndef __VP_SOCK_POSIX_H__
#define __VP_SOCK_POSIX_H__

#ifndef _WIN32   // POSIX only

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cstring>

// ---- winsock handle/sentinels -> BSD ---------------------------------------
typedef int SOCKET;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR   (-1)
#endif

// u_char/u_short/u_long/u_int come from <sys/types.h>.
// htons/htonl/ntohs/ntohl/inet_addr/inet_ntoa/gethostbyname/gethostname are real BSD
// functions (arpa/inet.h, netdb.h) — used by name, no redirect needed.

// ---- winsock-only calls -> BSD equivalents ---------------------------------
static inline int  vp_closesocket(SOCKET s)            { return ::close(s); }
static inline int  vp_WSAGetLastError(void)            { return errno; }
static inline void vp_WSASetLastError(int e)           { errno = e; }
// void* arg: engine passes DWORD*(=unsigned int*) for FIONREAD/FIONBIO; on LP64 that is
// not unsigned long*, and ioctl's 3rd arg is variadic, so accept any pointer.
static inline int  vp_ioctlsocket(SOCKET s, long cmd, void* argp)
                                                       { return ::ioctl(s, (unsigned long)cmd, argp); }

// WSAStartup/WSACleanup: no-op on POSIX (no library to init).
typedef struct { unsigned short wVersion; unsigned short wHighVersion; } VP_WSADATA;
typedef VP_WSADATA  WSADATA;
typedef WSADATA*    LPWSADATA;
static inline int vp_WSAStartup(unsigned short ver, void* data) { (void)ver; (void)data; return 0; }
static inline int vp_WSACleanup(void)                           { return 0; }

// ---- winsock types/error-codes used by the engine -> BSD --------------------
typedef struct hostent*    LPHOSTENT;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr    SOCKADDR;
#ifndef WSAEWOULDBLOCK
#define WSAEWOULDBLOCK   EWOULDBLOCK
#define WSAEINPROGRESS   EINPROGRESS
#define WSAEISCONN       EISCONN
#define WSAEALREADY      EALREADY
#define WSAECONNRESET    ECONNRESET
#define WSANOTINITIALISED (-1)   // no winsock-init concept on POSIX; sentinel (never matches errno)
#endif
// ultoa() is a Windows CRT-ism; win32_compat provides _ultoa.
#ifndef ultoa
#define ultoa _ultoa
#endif
// lstrcmpi (Win32 case-insensitive compare) -> strcasecmp.
#ifndef lstrcmpi
#define lstrcmpi strcasecmp
#endif

#define closesocket(s)        vp_closesocket(s)
#define WSAGetLastError()     vp_WSAGetLastError()
#define WSASetLastError(e)    vp_WSASetLastError(e)
#define ioctlsocket(s,c,a)    vp_ioctlsocket((s),(c),(a))
#define WSAStartup(v,d)       vp_WSAStartup((v),(d))
#define WSACleanup()          vp_WSACleanup()

// ---- WSAAsyncSelect: replaced by the select()/poll() loop (port step 5) ------
// Link-satisfying stub. The engine registers FD_READ/FD_CONNECT/FD_ACCEPT/FD_CLOSE
// interest via this call; on POSIX the select-loop (step 5) tracks the same fd set and
// dispatches the engine's existing FD_* handler. Until that lands, accept+record nothing.
#ifndef FD_READ
#define FD_READ    0x01
#define FD_WRITE   0x02
#define FD_OOB     0x04
#define FD_ACCEPT  0x08
#define FD_CONNECT 0x10
#define FD_CLOSE   0x20
#endif
static inline int vp_WSAAsyncSelect(SOCKET s, void* hWnd, unsigned int wMsg, long lEvent)
{ (void)s; (void)hWnd; (void)wMsg; (void)lEvent; return 0; }   // TODO step5: register in select set
#define WSAAsyncSelect(s,h,m,e)  vp_WSAAsyncSelect((s),(h),(m),(e))

#endif // !_WIN32
#endif // __VP_SOCK_POSIX_H__
