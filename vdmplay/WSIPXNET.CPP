#include "stdafx.h"
#include "vpwinsk.h"
#include <wsipx.h>


#include "datagram.h"
#include "wsipxnet.h"

static int netCount = 0;
static int gMaxSends = 10;

CWSIpxNet::CIPXLink::LinkMap  CWSIpxNet::CIPXLink::linkMap;

#define cFF ((char) 0xff)
#define c00 ((char) 0)
const ipx_addr CWSIpxNet::IPXADDR_BROADCAST = { { c00, c00, c00, c00 }, { cFF, cFF, cFF, cFF, cFF, cFF } };
#undef c00
#undef cFF


inline char htob(UCHAR ch)
{
    if (ch <= 9) return ('0' + ch);             // handle decimal values
    if (ch <= 0xf) return ('A' + ch - 10);      // handle hexidecimal specific values
    return('X');                                // Someone screwed up
}

void htoa(const char * src, char * dest, int srclen)
{
    char * destptr; // temp pointers
    const UCHAR * srcptr;
        
    srcptr = (const UCHAR *)src;
    destptr = dest;

    while(srclen--)
    {
    *destptr++ = htob((UCHAR)(*srcptr >> 4));      // Convert high order nibble
    *destptr++ = htob((UCHAR)(*srcptr++ & 0x0F));  // Convert low order nibble
    }
    *destptr = 0;  // Null terminator
}


char* CWSIpxNet::ipx_ntoa(const ipx_addr& a)
{
	static char buf[64];

  	htoa(a.ipx_netnum, buf, 4);
  	buf[8]='.';
  	htoa(a.ipx_nodenum, buf+9, 6);
  	buf[8+1+12] = 0;

  	return buf;
	

}

static char tohex(char x)
{
	if (isdigit(x)) 
		return x - '0';

	if (!isxdigit(x))
		return 0;

	return 10 + toupper(x) - 'A';
}

static char packhexbyte(char hi, char lo)
{
	
	return (tohex(hi) << 4) | tohex(lo);
}	


static void packhex(LPSTR buf, size_t bufSize, LPCSTR str)
{
	size_t sourceDigits = strlen(str);
	size_t destDigits = bufSize*2;
	char* dPtr = buf;
	char srcbuf[64];
	char* sPtr = srcbuf;


	memset(buf, 0, bufSize);
   	if (sourceDigits & 1)  // odd number of source digits
	{
		srcbuf[0] = '0';
		strcpy(srcbuf+1, str);
		sourceDigits++;
	}
	else
	{
		strcpy(srcbuf, str);
	}

	if (destDigits < sourceDigits)
		return;

	if (destDigits > sourceDigits)
	{
		int diff = destDigits - sourceDigits;
		dPtr += diff/2;
		destDigits -= diff;
	}
	
	while(destDigits)
	{
		*dPtr++ = packhexbyte(sPtr[0], sPtr[1]);
		sPtr += 2;
		destDigits -= 2;
	}

}



BOOL CWSIpxNet::parseAddressString(ipxaddress_s& addr, LPCSTR addrString)
{
	char buf[64];

	strcpy(buf, addrString);

	char* dot = strchr(buf, '.');

	if (dot == NULL)
		return FALSE;

	*dot++ = 0;

	packhex(addr.m_stationAddress.ipx_netnum, sizeof(addr.m_stationAddress.ipx_netnum), buf);

	char* colon = strchr(dot, ':');

	if (colon)
		*colon++ = 0;

	packhex(addr.m_stationAddress.ipx_nodenum, sizeof(addr.m_stationAddress.ipx_nodenum), dot);


	if (!colon)
		return TRUE;

	char* comma = strchr(colon, ',');

	if (comma)
		*comma++ = 0;

	addr.m_streamPort = htons((u_short) atoi(colon));

	if (comma)
		addr.m_dgPort = htons((u_short) atoi(comma));


	return TRUE;
}






static BOOL winsockinit()
{
	WSADATA wsa;
	int err;

	err = WSAStartup(0x0101, &wsa);

	if (err != 0 || LOBYTE(wsa.wVersion) != 1 || HIBYTE(wsa.wVersion) != 1)
		return FALSE;

	return TRUE;
}


BOOL  CWSIpxNet::Supported()
{
    int err;
	BOOL doCleanup = FALSE;

	if (!vpLoadWinsockDll())
		return FALSE;


	SOCKET s = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
	BOOL result = FALSE;

	if (s == INVALID_SOCKET)
	{
		err = WSAGetLastError();

		if (err == WSANOTINITIALISED)
		{
			if (winsockinit())
			{		
				doCleanup = TRUE;
				s = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
			}
		}

	}
	
	
	if (s != INVALID_SOCKET)
	{
		result = TRUE;
		closesocket(s);
	}

	if (doCleanup)
		WSACleanup();

	vpUnloadWinsockDll();
	return result;
}


CNetAddress* CWSIpxNet::MakeAddressFromString(LPCSTR addrString)
{
	ipxaddress_s  addr;

	if (addrString && parseAddressString(addr, addrString))
	 	return MakeAddressObject(addr.m_stationAddress, addr.m_streamPort, addr.m_dgPort);
	else
		return NULL;


}





CWSIpxNet::CWSIpxNet(CTDLogger* log, u_short streamPort, u_short dgPort, u_short wellKnownPort, ipx_addr* server, 
 LPCSTR srvAddrString) :
 CNetInterface(log),
 m_wellKnownPort(htons(wellKnownPort)), m_listenLink(NULL), m_dgLink(NULL), m_regAddr(NULL)
{
	m_address.m_streamPort = htons(streamPort);
	m_address.m_dgPort = htons(dgPort);
	
	if (!server)
		m_serverAddress = IPXADDR_BROADCAST;
	else
		m_serverAddress = *server;

	m_serverPort = m_wellKnownPort;

	gMaxSends = vpFetchInt("IPX", "MaxSends", 10);

	netCount++;

	if (netCount == 1)
	{
		WSADATA wsa;
	    int err;
		err = WSAStartup(0x0101, &wsa);

		if (err != 0 || LOBYTE(wsa.wVersion) != 1 || HIBYTE(wsa.wVersion) != 1)
			SetError(VPNET_ERR_INIT);
		else
			SetError(0);			
	}



	if (!server && srvAddrString)
	{
		ipxaddress_s addr;

		memset(&addr, 0, sizeof(addr));
		if (parseAddressString(addr, srvAddrString))
		{
			m_serverAddress = addr.m_stationAddress;
		 	m_serverPort = addr.m_dgPort;
			if (!m_serverPort)
				m_serverPort = m_wellKnownPort;
		}



	}



	sockaddr_ipx sipx;
	int namelen = sizeof(sipx);

	memset(&sipx, 0, sizeof(sipx));

	SOCKET s = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
	if (s == INVALID_SOCKET)
	{
		SetError(VPNET_ERR_INIT);
		return;
	}

	
	sipx.sa_family = AF_IPX;
	
	if (bind(s, (sockaddr*) &sipx, sizeof(sipx)))
	{
		DWORD err = WSAGetLastError();
		SetError(VPNET_ERR_INIT);
		return;
	}	
		
	if (getsockname(s, (sockaddr*) &sipx, &namelen))
	{
		DWORD err = WSAGetLastError();
		SetError(VPNET_ERR_INIT);
		return;
	}	
	
	m_address.m_stationAddress = sipx;

	closesocket(s);
	
	char* p = ipx_ntoa(m_address.m_stationAddress);
}


CNetLink* CWSIpxNet::MakeSafeLink(CNetAddress* addr, LPVOID userData)
  {
	
	IPXAddress& a = *(IPXAddress*) addr;
	DWORD err = 0;

	SOCKET s = socket(AF_IPX, SOCK_STREAM, NSPROTO_SPX);

	if (s == INVALID_SOCKET)
	{
		SetError(WSAGetLastError());
		return NULL;
	}


	SOCKADDR_IPX  sipx;

	sipx.sa_family = AF_IPX;
	sipx.sa_socket = a.m_addr.m_streamPort;
	copy(sipx, a.m_addr.m_stationAddress);


	ConfigureSocket(s, FD_CONNECT|FD_CLOSE|FD_READ|FD_WRITE);


	int r = connect(s, (sockaddr*) &sipx, sizeof(sipx));

	LinkState st = CONNECTED;
	
	if (r == SOCKET_ERROR)
	{
		
		if ((err = WSAGetLastError()) != WSAEWOULDBLOCK)
		{
			SetError(err);
			closesocket(s);
			return NULL;
		}
		
		st = CONNECTING;

	} 

#if USE_LOCALHOST
	// This call uses to overcome IntraTask test case problem on Windows NT
	ConfigureSocket(s, FD_CLOSE|FD_READ|FD_WRITE);
#endif

	CIPXLink* l = new CIPXLink(s, st, this, userData);

	if (!l)
		closesocket(s);
	
   	return l;
}


CNetLink* CWSIpxNet::MakeUnsafeLink(LPVOID userData)
{

	if (m_dgLink)
	{
		m_dgLink->Ref();
		return m_dgLink;
	}

	SOCKET s = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);


	if (s == INVALID_SOCKET)
	{
		SetError(WSAGetLastError());
		return NULL;
	}


	BOOL trueValue = TRUE;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (LPCSTR) &trueValue,  sizeof(trueValue));
	setsockopt(s, SOL_SOCKET, SO_USELOOPBACK, (LPCSTR) &trueValue,  sizeof(trueValue));

	ConfigureSocket(s, FD_READ|FD_WRITE);


	CIPXLink* l = new CIPXLink(s, DG, this, userData);

	if (!l)
		closesocket(s);

   	return l;
}


CWSIpxNet::CIPXLink*  CWSIpxNet::AcceptLink()
{

	SOCKET s = accept(m_listenLink->m_socket, NULL, NULL);


	if (s != INVALID_SOCKET)
	{
		CIPXLink* l = new CIPXLink(s, CONNECTED, this, NULL);

		if (!l)
		{
			SetError(VPNET_ERR_NOMEM);
		}

		ConfigureSocket(s, FD_READ|FD_WRITE|FD_CLOSE);

		return l;
	 }

	 DWORD err = WSAGetLastError();
	 if (err == WSAEWOULDBLOCK)
	 	return NULL;
	 	
	 SetError(err);

	 return NULL;
}

	 


CWSIpxNet::CIPXLink* CWSIpxNet::MakeListenLink()
{

	SOCKET s = socket(AF_IPX, SOCK_STREAM, NSPROTO_SPX);

	if (s == INVALID_SOCKET)
	{
		SetError(WSAGetLastError());
		return NULL;
	}

	ConfigureSocket(s, FD_ACCEPT);


	CIPXLink* l = new CIPXLink(s, CONNECTING, this, NULL);

	if (!l)
		closesocket(s);

	
   	return l;
}



BOOL CWSIpxNet::Listen(BOOL streamListen, BOOL serverMode)
{

	m_dgLink = (CIPXLink*) MakeUnsafeLink();
	

	if (!m_dgLink)
		return FALSE;


	sockaddr_ipx sipx;
	int namelen = sizeof(sipx);

	if (serverMode)
		m_address.m_dgPort = m_wellKnownPort;

	sipx.sa_family = AF_IPX;
	copy(sipx, m_address.m_stationAddress);
	sipx.sa_socket = m_address.m_dgPort;


	if (bind(m_dgLink->m_socket, (sockaddr*) &sipx, sizeof(sipx)))
		goto handleerr;

//	ConfigureSocket(m_dgLink->m_socket, FD_READ|FD_WRITE);

	if (streamListen)
	{
		m_listenLink = MakeListenLink();

		if (!m_listenLink)
			goto nonwsaerr;

		sipx.sa_socket = 0;		
		if (bind(m_listenLink->m_socket, (sockaddr*) &sipx, sizeof(sipx)))
			goto handleerr;

		if (listen(m_listenLink->m_socket, 5))
			goto handleerr;

		if (getsockname(m_listenLink->m_socket, (sockaddr*) &sipx, &namelen))
			goto handleerr;

  		m_address.m_streamPort = sipx.sa_socket;
	 }

	if (getsockname(m_dgLink->m_socket, (sockaddr*) &sipx, &namelen))
		goto handleerr;

  	m_address.m_dgPort = sipx.sa_socket;
	m_address.m_stationAddress = sipx;


	return TRUE;

handleerr:

	SetError(WSAGetLastError());

nonwsaerr:
	delete m_dgLink;
	m_dgLink = NULL;
	
	delete m_listenLink;
	
	m_listenLink = NULL;
	return FALSE;


}



void CWSIpxNet::BecomeDeef()
{
	if (m_listenLink)
	{
		delete m_listenLink;
		m_listenLink = NULL;

	}

	
	if (m_dgLink)
	   ConfigureSocket(m_dgLink->m_socket, FD_WRITE);
  	

}
	

void CWSIpxNet::Cleanup()
{
	delete m_listenLink;
	delete m_dgLink;

	m_listenLink = 0;
	m_dgLink = 0;
	m_address.m_dgPort = 0;
	m_address.m_streamPort = 0;
	CNetInterface::Cleanup();
}


CWSIpxNet::~CWSIpxNet()
{
	Cleanup();

	if (netCount-- == 1)
	{
		WSACleanup();
	}

}



BOOL CWSIpxNet::CIPXLink::EnqueueData(LPVOID data, DWORD dataSize)
{
	Datagram* d = new Datagram(data, (WORD) dataSize);

	if (!d)
	{
		SetError(VPNET_ERR_NOMEM);
		return FALSE;
	}

	if (!d->Ok())
	{
		delete d;
		SetError(VPNET_ERR_NOMEM);
		return FALSE;
	}

	m_queue.Add(d);

	if (m_state == CONNECTED)
		m_net->SuspendHost();

	return TRUE;
}






CNetAddress* CWSIpxNet::CIPXLink::GetRemoteAddress()
{
	if (m_state != CONNECTED)
		return NULL;

    sockaddr   sipx;
	int  namelen = sizeof(sipx);
	
	int err = getpeername(m_socket, (sockaddr*) &sipx, &namelen);

	if (!err)
	{
		ipx_addr ipxa;

		ipxa = (sockaddr_ipx&) sipx;

		IPXAddress* a = new IPXAddress(ipxa, ((sockaddr_ipx&)sipx).sa_socket, 0);
		if (!a)
		{
			SetError(VPNET_ERR_NOMEM);
			return NULL;
		}
		return a;
	}

	SetError(WSAGetLastError());

	return NULL;
}


CWSIpxNet::CIPXLink* CWSIpxNet::CIPXLink::LookupLink(SOCKET s)
{
	LinkIter iter(linkMap);
	CIPXLink*  l;

	while(NULL != (l = iter()))
	{
		if (l->m_socket == s)
			return l;
	}
    return NULL;
}


void CWSIpxNet::OnTimer()
{
	CheckFlow();
}

void CWSIpxNet::CheckFlow()
{

	if (!HostIsBlocked())
		return ;

	CIPXLink*  l;
	CIPXLink::LinkIter iter(CIPXLink::linkMap);

	while(NULL != (l = iter()))
	{
		if (l->m_state == CONNECTED && (0 != l->m_queue.Count()))
			return;
	}

	ResumeHost();
}



void CWSIpxNet::CIPXLink::SendWaitingData()
{
	int maxSends = gMaxSends;

	m_queue.Lock();
	
	Datagram* d;
	
	while(maxSends-- && NULL != (d = m_queue.Get()))
	{
		LPVOID data = d->MoreData();
		WORD   size = d->DataSize();
		
		int s = send(m_socket, (LPSTR) data, size, 0);

		if (s == SOCKET_ERROR) 
		{
			DWORD err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				m_queue.Insert(d);
				break;
			}
			
			// Check for connection loss
			// ....
			//
			SetError(err);
			break;
		}


		d->AddOffset((WORD) s);
		if (!d->Done())
		{
			m_queue.Insert(d);
			break;
		}
		delete d;
	}
	
	m_queue.Unlock();
}
		


BOOL CWSIpxNet::CIPXLink::Send(LPVOID data, DWORD dataSize, DWORD flags)
{
	VPASSERT(m_state != DG);

	
	if (m_state == CONNECTING)
		return EnqueueData(data, dataSize);

	if (m_queue.Count())
	{
		if (EnqueueData(data, dataSize))
			SendWaitingData();

		return TRUE;
	}

	
	int s = send(m_socket, (LPSTR) data, dataSize, 0);
	DWORD err;

	if (s == SOCKET_ERROR) 
	{
		err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			return EnqueueData(data, dataSize);

		// Check for connection loss
		// ....
		//
		SetError(err);
		return FALSE;
	}

	if (s < (int) (dataSize))
	{
		return Send(s + (LPSTR) data, dataSize - (DWORD) s, flags);
	}

	return TRUE;
}


BOOL CWSIpxNet::CIPXLink::SendTo(CNetAddress& na, LPVOID data, DWORD dataSize, DWORD flags)
{
	VPASSERT(m_state == DG);
	ipxaddress_s& ta = ((IPXAddress&)na).m_addr;
	sockaddr_ipx  sipx;

	sipx.sa_family = AF_IPX;
	sipx.sa_socket = ta.m_dgPort;
	copy(sipx, ta.m_stationAddress);
	
	if (flags & VP_BROADCAST)
		copy(sipx,IPXADDR_BROADCAST);
		 


	int s = sendto(m_socket, (LPSTR) data, dataSize, 0, (sockaddr*) &sipx, sizeof(sipx));

	DWORD err;

	if (s == SOCKET_ERROR) 
	{
		err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			return TRUE;
		SetError(err);
		return FALSE;
	}


	return TRUE;
}


DWORD CWSIpxNet::CIPXLink::Receive(LPVOID data, DWORD dataSize)
{
	VPASSERT(m_state != DG);
	
	if (m_state == CONNECTING)
		return 0;

	int s = recv(m_socket, (LPSTR) data, dataSize, 0);
	DWORD err;

	if (s == SOCKET_ERROR) 
	{
		err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			return 0;

		SetError(err);
		return 0;
	}

#if 0
	if (s == 0)
	{
		
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		SetError(VPNET_ERR_LINK_LOST);
	}
#endif
	return s;
}



DWORD CWSIpxNet::CIPXLink::ReceiveFrom(LPVOID data, DWORD dataSize, CNetAddress& na)
{
	VPASSERT(m_state == DG);
   	sockaddr_ipx sipx;
	int namelen = sizeof(sipx);

	int s = recvfrom(m_socket, (LPSTR) data, dataSize, 0, (sockaddr*) &sipx, &namelen);
	DWORD err;

	if (s == SOCKET_ERROR) 
	{
		err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			return 0;

		SetError(err);
		return 0;
	}

	if (s != 0)
	{
		IPXAddress& ta = (IPXAddress&) na;
		ta.m_addr.m_stationAddress = sipx;
		ta.m_addr.m_dgPort = sipx.sa_socket;
		ta.m_addr.m_streamPort = 0;

	}

	return s;
}

DWORD CWSIpxNet::CIPXLink::HasData()
{
	
	DWORD byteCount;

	if (!ioctlsocket(m_socket, FIONREAD, &byteCount))
		return byteCount;

	return 0;
}
