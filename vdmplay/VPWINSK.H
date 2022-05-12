#ifndef __VPWINSK_H__
#define __VPWINSK_H__

#include <winsock.h>

#define USE_VPWINSOCK
#ifdef __cplusplus
extern "C" {
#endif


#define InitWsThunks()	 (TRUE)
#define CleanupWsThunks()  vpUnloadWinsockDll()


struct VP_SOCK_CALL
{
	SOCKET (PASCAL * pfnaccept[2])(SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen);
	u_short (PASCAL * pfnhtons[2])(u_short hostshort);
	unsigned long (PASCAL * pfninet_addr[2]) (const char FAR * cp);
	int (PASCAL * pfnclosesocket[2])(SOCKET s);
	int (PASCAL * pfngethostname[2]) (char FAR * name, int namelen);
	int (PASCAL * pfngetsockname[2])(SOCKET s, struct sockaddr FAR *name, int FAR * namelen);
	int (PASCAL * pfngetpeername[2])(SOCKET s, struct sockaddr FAR *name, int FAR * namelen);
	u_short (PASCAL * pfnntohs[2])(u_short netshort);
	char* (PASCAL * pfninet_ntoa[2])(struct in_addr in);
	int (PASCAL * pfnWSAGetLastError[2])(void);
	void (PASCAL * pfnWSASetLastError[2])(int iError);
	int (PASCAL * pfnWSAStartup[2])(WORD wVersionRequired, LPWSADATA lpWSAData);
	int (PASCAL * pfnWSACleanup[2])(void);
	u_long (PASCAL * pfnhtonl[2])(u_long hostlong);
	SOCKET (PASCAL * pfnsocket[2])(int af, int type, int protocol);
	struct hostent* (PASCAL * pfngethostbyname[2])(const char FAR * name);
	int (PASCAL * pfnrecv[2])(SOCKET s, char FAR * buf, int len, int flags);
	int (PASCAL * pfnsend[2])(SOCKET s, const char FAR * buf, int len, int flags);
	int (PASCAL * pfnWSAAsyncSelect[2])(SOCKET s, HWND hWnd, u_int wMsg, long lEvent);
	int (PASCAL * pfnrecvfrom[2])(SOCKET s, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen);
	int (PASCAL * pfnsendto[2])(SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR *to, int tolen);
	int (PASCAL * pfnconnect[2])(SOCKET s, const struct sockaddr FAR *name, int namelen);
	int (PASCAL * pfnbind[2])(SOCKET s, const struct sockaddr FAR *addr, int namelen);
	int (PASCAL * pfnsetsockopt[2])(SOCKET s, int level, int optname, const char FAR * optval, int optlen);
	int (PASCAL * pfngetsockopt[2])(SOCKET s, int level, int optname, char FAR * optval, int FAR *optlen);
	int (PASCAL * pfnioctlsocket[2])(SOCKET s, long cmd, u_long FAR *argp);
	int (PASCAL * pfnlisten[2])(SOCKET s, int backlog);
	int (PASCAL * pfnshutdown[2])(SOCKET s, int how);

};

extern VP_SOCK_CALL _vpSOCK;

#ifdef USE_VPWINSOCK

#define accept              _vpSOCK.pfnaccept[0]
#define htons               _vpSOCK.pfnhtons[0]
#define inet_addr           _vpSOCK.pfninet_addr[0]
#define closesocket         _vpSOCK.pfnclosesocket[0]
#define gethostname         _vpSOCK.pfngethostname[0]
#define getsockname         _vpSOCK.pfngetsockname[0]
#define getpeername         _vpSOCK.pfngetpeername[0]
#define ntohs               _vpSOCK.pfnntohs[0]
#define inet_ntoa           _vpSOCK.pfninet_ntoa[0]
#define WSAGetLastError     _vpSOCK.pfnWSAGetLastError[0]
#define WSASetLastError     _vpSOCK.pfnWSASetLastError[0]
#define WSAStartup          _vpSOCK.pfnWSAStartup[0]
#define WSACleanup          _vpSOCK.pfnWSACleanup[0]
#define htonl               _vpSOCK.pfnhtonl[0]
#define socket              _vpSOCK.pfnsocket[0]
#define gethostbyname       _vpSOCK.pfngethostbyname[0]
#define recv                _vpSOCK.pfnrecv[0]
#define send                _vpSOCK.pfnsend[0]
#define WSAAsyncSelect      _vpSOCK.pfnWSAAsyncSelect[0]
#define recvfrom            _vpSOCK.pfnrecvfrom[0]
#define sendto              _vpSOCK.pfnsendto[0]
#define connect             _vpSOCK.pfnconnect[0]
#define bind                _vpSOCK.pfnbind[0]
#define setsockopt          _vpSOCK.pfnsetsockopt[0]
#define getsockopt          _vpSOCK.pfngetsockopt[0]
#define ioctlsocket         _vpSOCK.pfnioctlsocket[0]
#define listen              _vpSOCK.pfnlisten[0]
#define shutdown            _vpSOCK.pfnshutdown[0]
#define inet_addr			_vpSOCK.pfninet_addr[0]

#endif 

#ifdef __cplusplus
}
#endif


class _VP_SOCK_STATE
{
public:
	HINSTANCE m_hInstSOCK;      // handle of WSOCK32.DLL
	_VP_SOCK_STATE() : m_hInstSOCK(NULL) {  }
	virtual ~_VP_SOCK_STATE();
};




#endif
