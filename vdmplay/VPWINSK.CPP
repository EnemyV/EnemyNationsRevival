#include "stdafx.h"
#include "vputil.h"
#include "vpwinsk.h"

#ifdef USE_VPWINSOCK

#ifdef WIN32
static char sockdll[] = "WSOCK32.DLL";
#else
static char sockdll[] = "WINSOCK.DLL";
#endif


static BOOL bWin31 = -1; // unknown right now

_VP_SOCK_STATE _vpSOCKState;
extern const VP_SOCK_CALL _vpSOCKInit;
static HINSTANCE winsockDLL = NULL;

void vpUnloadWinsockDll()
{
	VPENTER(vpUnloadWinsockDll);
	if (winsockDLL != NULL)
		FreeLibrary(winsockDLL);
	
	VPEXIT();
}

BOOL vpLoadWinsockDll()
{
	BOOL ret;
	VPENTER(vpLoadWinsockDll);
    winsockDLL = LoadLibrary(sockdll);
#ifdef WIN32
	ret = (winsockDLL != NULL);
#else
	ret = (UINT) winsockDLL > 32;
#endif

	VPEXITBOOL(ret);
	return ret;

}


HINSTANCE VPLoadDll(HINSTANCE& hInst, LPCSTR lpszDLL, FARPROC* pProcPtrs, LPCSTR lpszProcName)
{
	if (hInst == NULL)
	{
		hInst = LoadLibrary(lpszDLL);
		VPASSERT(hInst != NULL);
	}

	// cache the procedure pointer if cache memory is provided
	if (pProcPtrs != NULL && pProcPtrs[1] == NULL)
	{
#ifndef WIN32
		char nmBuf[64];
	 	strcpy(nmBuf, lpszProcName);
		AnsiUpper(nmBuf);
		lpszProcName = nmBuf;
#endif
		pProcPtrs[1] = GetProcAddress(hInst, lpszProcName);
		VPASSERT(pProcPtrs[1] != NULL);

#ifdef WIN32

		if (bWin31 == -1)
		{
			DWORD dwVersion = GetVersion();
			bWin31 = (dwVersion & 0x80000000) && (BYTE)dwVersion < 4;
		}
		// only substitute the thunk on real Win32 -- not Win32s
		if (!bWin31)
#endif
			pProcPtrs[0] = pProcPtrs[1];
	}
	return hInst;
}





inline HINSTANCE VPLoadSock(FARPROC* proc, LPCSTR lpsz)
	{ return VPLoadDll(_vpSOCKState.m_hInstSOCK,  sockdll , proc, lpsz); }
#define SOCKLOAD(x) \
	VPLoadSock((FARPROC*)_vpSOCK.pfn##x, #x);

SOCKET PASCAL VPThunkaccept(SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen)
{
	SOCKLOAD(accept);
	return _vpSOCK.pfnaccept[1](s, addr, addrlen);
}

u_short PASCAL VPThunkhtons(u_short hostshort)
{
	SOCKLOAD(htons);
	return _vpSOCK.pfnhtons[1](hostshort);
}

unsigned long PASCAL VPThunkinet_addr(const char FAR * cp)
{
	SOCKLOAD(inet_addr);
	return _vpSOCK.pfninet_addr[1](cp);
}

int PASCAL VPThunkclosesocket(SOCKET s)
{
	SOCKLOAD(closesocket);
	return _vpSOCK.pfnclosesocket[1](s);
}

int PASCAL FAR VPThunkgethostname (char FAR * name, int namelen)
{
	SOCKLOAD(gethostname);
	return _vpSOCK.pfngethostname[1](name, namelen);
}	

int PASCAL VPThunkgetsockname(SOCKET s, struct sockaddr FAR *name, int FAR * namelen)
{
	SOCKLOAD(getsockname);
	return _vpSOCK.pfngetsockname[1](s, name, namelen);
}


int PASCAL VPThunkgetpeername(SOCKET s, struct sockaddr FAR *name, int FAR * namelen)
{
	SOCKLOAD(getpeername);
	return _vpSOCK.pfngetpeername[1](s, name, namelen);
}

u_short PASCAL VPThunkntohs(u_short netshort)
{
	SOCKLOAD(ntohs);
	return _vpSOCK.pfnntohs[1](netshort);
}

char* PASCAL VPThunkinet_ntoa(struct in_addr in)
{
	SOCKLOAD(inet_ntoa);
	return _vpSOCK.pfninet_ntoa[1](in);
}

int PASCAL VPThunkWSAGetLastError(void)
{
	SOCKLOAD(WSAGetLastError);
	return _vpSOCK.pfnWSAGetLastError[1]();
}

void PASCAL VPThunkWSASetLastError(int iError)
{
	SOCKLOAD(WSASetLastError);
	_vpSOCK.pfnWSASetLastError[1](iError);
}

int PASCAL VPThunkWSAStartup(WORD wVersionRequired, LPWSADATA lpWSAData)
{
	SOCKLOAD(WSAStartup);
	return _vpSOCK.pfnWSAStartup[1](wVersionRequired, lpWSAData);
}

int PASCAL VPThunkWSACleanup(void)
{
	SOCKLOAD(WSACleanup);
	int ret = _vpSOCK.pfnWSACleanup[1]();

	_vpSOCK = _vpSOCKInit;
	FreeLibrary(_vpSOCKState.m_hInstSOCK);
	_vpSOCKState.m_hInstSOCK = NULL; 
	return ret;

}
	


u_long PASCAL VPThunkhtonl(u_long hostlong)
{
	SOCKLOAD(htonl);
	return _vpSOCK.pfnhtonl[1](hostlong);
}

SOCKET PASCAL VPThunksocket(int af, int type, int protocol)
{
	SOCKLOAD(socket);
	return _vpSOCK.pfnsocket[1](af, type, protocol);
}

struct hostent* PASCAL VPThunkgethostbyname(const char FAR * name)
{
	SOCKLOAD(gethostbyname);
	return _vpSOCK.pfngethostbyname[1](name);
}

int PASCAL VPThunkrecv(SOCKET s, char FAR * buf, int len, int flags)
{
	SOCKLOAD(recv);
	return _vpSOCK.pfnrecv[1](s, buf, len, flags);
}

int PASCAL VPThunksend(SOCKET s, const char FAR * buf, int len, int flags)
{
	SOCKLOAD(send);
	return _vpSOCK.pfnsend[1](s, buf, len, flags);
}

int PASCAL VPThunkWSAAsyncSelect(SOCKET s, HWND hWnd, u_int wMsg, long lEvent)
{
	SOCKLOAD(WSAAsyncSelect);
	return _vpSOCK.pfnWSAAsyncSelect[1](s, hWnd, wMsg, lEvent);
}

int PASCAL VPThunkrecvfrom(SOCKET s, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen)
{
	SOCKLOAD(recvfrom);
	return _vpSOCK.pfnrecvfrom[1](s, buf, len, flags, from, fromlen);
}

int PASCAL VPThunksendto(SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR *to, int tolen)
{
	SOCKLOAD(sendto);
	return _vpSOCK.pfnsendto[1](s, buf, len, flags, to, tolen);
}

int PASCAL VPThunkconnect(SOCKET s, const struct sockaddr FAR *name, int namelen)
{
	SOCKLOAD(connect);
	return _vpSOCK.pfnconnect[1](s, name, namelen);
}

int PASCAL VPThunkbind(SOCKET s, const struct sockaddr FAR *addr, int namelen)
{
	SOCKLOAD(bind);
	return _vpSOCK.pfnbind[1](s, addr, namelen);
}

int PASCAL VPThunksetsockopt(SOCKET s, int level, int optname, const char FAR * optval, int optlen)
{
	SOCKLOAD(setsockopt);
	return _vpSOCK.pfnsetsockopt[1](s, level, optname, optval, optlen);
}

int PASCAL VPThunkgetsockopt(SOCKET s, int level, int optname, char FAR * optval, int FAR *optlen)
{
	SOCKLOAD(getsockopt);
	return _vpSOCK.pfngetsockopt[1](s, level, optname, optval, optlen);
}

int PASCAL VPThunkioctlsocket(SOCKET s, long cmd, u_long FAR *argp)
{
	SOCKLOAD(ioctlsocket);
	return _vpSOCK.pfnioctlsocket[1](s, cmd, argp);
}

int PASCAL VPThunklisten(SOCKET s, int backlog)
{
	SOCKLOAD(listen);
	return _vpSOCK.pfnlisten[1](s, backlog);
}

int PASCAL VPThunkshutdown(SOCKET s, int how)
{
	SOCKLOAD(shutdown);
	return _vpSOCK.pfnshutdown[1](s, how);
}




_VP_SOCK_STATE::~_VP_SOCK_STATE()
{
	if (m_hInstSOCK != NULL)
		FreeLibrary(m_hInstSOCK);       // handle of WSOCK32.DLL
}



VP_SOCK_CALL _vpSOCK =
{
	{ VPThunkaccept, },
	{ VPThunkhtons, },
	{ VPThunkinet_addr, },
	{ VPThunkclosesocket, },
	{ VPThunkgethostname, },
	{ VPThunkgetsockname, },
	{ VPThunkgetpeername, },
	{ VPThunkntohs, },
	{ VPThunkinet_ntoa, },
	{ VPThunkWSAGetLastError, },
	{ VPThunkWSASetLastError, },
	{ VPThunkWSAStartup, },
	{ VPThunkWSACleanup, },
	{ VPThunkhtonl, },
	{ VPThunksocket, },
	{ VPThunkgethostbyname, },
	{ VPThunkrecv, },
	{ VPThunksend, },
	{ VPThunkWSAAsyncSelect, },
	{ VPThunkrecvfrom, },
	{ VPThunksendto, },
	{ VPThunkconnect, },
	{ VPThunkbind, },
	{ VPThunksetsockopt, },
	{ VPThunkgetsockopt, },
	{ VPThunkioctlsocket, },
	{ VPThunklisten, },
	{ VPThunkshutdown, }
};

const VP_SOCK_CALL _vpSOCKInit =
{
	{ VPThunkaccept, },
	{ VPThunkhtons, },
	{ VPThunkinet_addr, },
	{ VPThunkclosesocket, },
	{ VPThunkgethostname, },
	{ VPThunkgetsockname, },
	{ VPThunkgetpeername, },
	{ VPThunkntohs, },
	{ VPThunkinet_ntoa, },
	{ VPThunkWSAGetLastError, },
	{ VPThunkWSASetLastError, },
	{ VPThunkWSAStartup, },
	{ VPThunkWSACleanup, },
	{ VPThunkhtonl, },
	{ VPThunksocket, },
	{ VPThunkgethostbyname, },
	{ VPThunkrecv, },
	{ VPThunksend, },
	{ VPThunkWSAAsyncSelect, },
	{ VPThunkrecvfrom, },
	{ VPThunksendto, },
	{ VPThunkconnect, },
	{ VPThunkbind, },
	{ VPThunksetsockopt, },
	{ VPThunkgetsockopt, },
	{ VPThunkioctlsocket, },
	{ VPThunklisten, },
	{ VPThunkshutdown, }

};



#endif
