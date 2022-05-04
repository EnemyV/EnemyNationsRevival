#include "stdafx.h"
#include "vpwinsk.h"
#include "datagram.h"
#include "tcpnet.h"
#include "tdlog.h"
#include <ctype.h>

static int netCount = 0;
static int gSimulateFrags = FALSE;
static int gMaxSends = 5;
static int gSimulateFlow = FALSE;
static int gLogSocketFull = FALSE;

class SockBuf
{
public:
	char m_buf[1024];
	unsigned m_size;
	DWORD    m_err;
	DWORD    m_lastDataTime;
	
	DWORD  HasData(SOCKET s)
	{

		unsigned pktLen;

		if (m_err)
			return 0;

		if (m_size < 2)
		{
			// read the packet length
			FillBuf(s, 2 - m_size);
		}

		if (m_err)
			return 0;

		// first word of packet contains it's size... get it
		if (m_size >= 2)
		{
			pktLen = *(WORD*) m_buf;
			
		}
		else
			return 0;

		if (m_size < pktLen)
		{
			FillBuf(s, pktLen - m_size);
		}
		
		if (m_err)
			return 0;

		if (m_size == pktLen)
			return m_size;
		else
			return 0;

	}

	DWORD GetData(void* buf, DWORD size)
	{

		if (m_err)
			return 0;

		if (m_size < (unsigned) size)
			size = m_size;

		if (size > 0)
		{
			memcpy(buf, m_buf, size);
			m_size = 0;
		}

		return size;
	}

	void FillBuf(SOCKET sock, unsigned size)
	{
		int s = recv(sock, m_buf + m_size, size, 0);
		DWORD err;

		if (s == SOCKET_ERROR) 
		{
			err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
				return;

			m_err = err;
		}

		if (s)
			m_lastDataTime = GetCurrentTime();
		
		m_size += s;
	}

	SockBuf() 
	{
		m_err = 0; m_size = 0; m_lastDataTime = 0;
	}
};




CTcpNet::CTCPLink::LinkMap  CTcpNet::CTCPLink::linkMap;


static BOOL winsockinit()
{
	WSADATA wsa;
	int err;

	err = WSAStartup(0x0101, &wsa);

	if (err != 0 || LOBYTE(wsa.wVersion) != 1 || HIBYTE(wsa.wVersion) != 1)
		return FALSE;

	return TRUE;
}

static unsigned short calcsum(LPCVOID data, DWORD dataSize)
{
	unsigned char* s = (unsigned char*) data;
	unsigned short chksum = 0;

	while(dataSize--)
		chksum += *s++;

	return chksum;
}



BOOL  CTcpNet::Supported()
{
    int err;
	BOOL doCleanup = FALSE;

	if (!vpLoadWinsockDll())
		return FALSE;


	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	BOOL result = FALSE;

	if (s == INVALID_SOCKET)
	{
		err = WSAGetLastError();

		if (err == WSANOTINITIALISED)
		{
			if (winsockinit())
			{		
				doCleanup = TRUE;
				s = socket(AF_INET, SOCK_DGRAM, 0);
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

// given hostent structure and desired address index, copy the corresponding
// address into 'a'. If address index is GT length of the address list 
// use the first address in the list
static void selectAddress(LPVOID a, LPHOSTENT he, int aIndex)
{
	int aCount;

	// find the address list length
	for(aCount = 0; he->h_addr_list[aCount]; aCount++) ;

	if (aIndex >= aCount)
		aIndex = 0;

	_fmemcpy(a, he->h_addr_list[aIndex], he->h_length);
}



CTcpNet::CTcpNet(CTDLogger* log, u_short streamPort, u_short dgPort, u_short wellKnownPort, u_long serverAddr, LPCSTR srvAddrStr) :
 CNetInterface(log),
 m_wellKnownPort(htons(wellKnownPort)), m_listenLink(NULL), m_dgLink(NULL), 
 m_serverAddress(serverAddr), m_regAddr(NULL) 
{
	Log("CTcpNet::CTcpNet Entry");

	m_address.m_streamPort = htons(streamPort);
	m_address.m_dgPort = htons(dgPort);
	m_serverPort = m_wellKnownPort;

    m_sockBufSize = vpFetchInt("TCP", "OBUFSIZE", 8)* 1024;
	gSimulateFrags = vpFetchInt("TCP", "SimFrag", 0);
	gMaxSends = vpFetchInt("TCP", "MaxSends", 10);
	gSimulateFlow = vpFetchInt("TCP", "SimFlow", 0);
	gLogSocketFull = vpFetchInt("TCP", "LogSocketFull", 0);
	m_lastKeekTime = 0;
	
	netCount++;

	if (netCount == 1)
	{
		WSADATA wsa;
	    int err;
		err = WSAStartup(0x0101, &wsa);

		if (err != 0 || LOBYTE(wsa.wVersion) != 1 || HIBYTE(wsa.wVersion) != 1)
		{
			VPTRACE(("Wrong WINSOCK Version"));
			Log("CTcpNet:: CTcpNet - error initializing winsock"); 
			SetError(VPNET_ERR_INIT, err);
		}
		else
			SetError(0);			
	}

	if (!m_serverAddress && srvAddrStr)
	{
		tcpaddress_s addr;

		m_serverAddress = 0;
	
		Log("CTcpNet::CTcpNet Before TranslateStringAddress");

		if (TCPAddress::TranslateAddressString(addr, srvAddrStr))
		{
			m_serverAddress = addr.m_stationAddress.s_addr;
		 	m_serverPort = addr.m_dgPort;
			if (!m_serverPort)
				m_serverPort = m_wellKnownPort;
		}

		Log("CTcpNet::CTcpNet After TranslateStringAddress");


	}

	if (!m_serverAddress)
		m_serverAddress = INADDR_BROADCAST;

#if 1
	char hname[256];


	if (SOCKET_ERROR == gethostname(hname, sizeof(hname)))
	{
		DWORD err = WSAGetLastError();
		VPTRACE(("gethostname returns error %d", err));		
		Log("CTcpNet::CTcpNet gethostname returns error");
		SetError(VPNET_ERR_INIT,err);
		return;
	}			


	LPHOSTENT  he = gethostbyname(hname);
	
	Log("CTcpNet::CTcpNet After gethostbyname");
	
	if (!he)
	{
		DWORD wsErr = WSAGetLastError();
		Log("CTcpNet::CTcpNet gethostbyname returns error");

		SetError(VPNET_ERR_INIT, wsErr);
		return;
	}

	int aIndex = vpFetchInt("TCP", "AddressIndex", 0);

	selectAddress(&m_address.m_stationAddress, he, aIndex);



#else
	m_address.m_stationAddress.s_addr = INADDR_ANY;
#endif

	char* p = inet_ntoa(m_address.m_stationAddress);

}




// address string has a folowing format:  hostpart:streamPort,datagramPort
BOOL CTcpNet::TCPAddress::extractHostPart(LPSTR hostPart, size_t len, LPCSTR addrString)
{
 	LPCSTR sep = strchr(addrString, ':');

	if (!sep)
	{
	 	if (len >= (strlen(addrString)+1))
		{
			strcpy(hostPart, addrString);
		}
		else
			return FALSE;
	}
	else
	{
		size_t sLen = sep - addrString;
	  
		if (len >= (sLen+1))
		{
			strncpy(hostPart, addrString, sLen);
			hostPart[len] = 0;
		}
		else
			return FALSE;
	}

	return TRUE;

}

// address string has a folowing format:  hostpart:streamPort,datagramPort
void CTcpNet::TCPAddress::extractPortPart(tcpaddress_s &addr, LPCSTR addrString)
{
 	LPCSTR sep = strchr(addrString, ':');
	char digits[16];
	int i;
	
	if (sep)
	{
		sep++;
		i = 0;

		while(isdigit(*sep))
		{
			digits[i++] = *sep++;
		}
		digits[i] = 0;
		addr.m_streamPort = htons(atoi(digits));

		if (*sep == ',')
		{

 			sep++;
			i = 0;

			while(isdigit(*sep))
			{
				digits[i++] = *sep++;
			}

			digits[i] = 0;
			addr.m_dgPort = htons(atoi(digits));
		}

	}

	
}

// Parse the address of the type  hostpart:streamport,datagramport
// Where hostpart can be either dotted decimal IP address  or DNS name
// and the :streamport,datagramport are optional
BOOL CTcpNet::TCPAddress::TranslateAddressString(tcpaddress_s &addr, LPCSTR addrString)
{
	size_t validLen;
	char hostpart[128];

	addr.m_stationAddress.s_addr = 0;
	addr.m_streamPort = 0;
	addr.m_dgPort = 0;  

	if (!extractHostPart(hostpart, sizeof(hostpart), addrString))
		return FALSE;


	validLen = strspn(addrString, "0123456789.:,");

	if (validLen == strlen(addrString))
	{
		addr.m_stationAddress.s_addr = inet_addr(hostpart);
	}
	else
	{
		LPHOSTENT  he = gethostbyname(hostpart);

		if (!he)
		{
			VPTRACE(("Can't find host: %s\n", hostpart));
			return FALSE;
		}

		_fmemcpy(&addr.m_stationAddress, he->h_addr, he->h_length);
	}

	extractPortPart(addr, addrString);
	return TRUE;

}

CNetAddress* CTcpNet::MakeAddressFromString(LPCSTR addrString)
{
	tcpaddress_s  addr;

	if (addrString && TCPAddress::TranslateAddressString(addr, addrString))
	 	return MakeAddressObject(addr.m_stationAddress.s_addr, addr.m_streamPort, addr.m_dgPort);
	else
		return NULL;


}

void CTcpNet::SetRegistrationAddress(LPCSTR addr)
{
	if (m_regAddr)
	{						 
		m_regAddr->Unref();
	}
	TCPAddress* ta = (TCPAddress*) MakeAddressFromString(addr);
	m_regAddr = ta;
	if (ta && !ta->m_addr.m_dgPort)
		ta->m_addr.m_dgPort = m_wellKnownPort;

	if (ta)
	{
		char buf[128];

		strcpy(buf, "CTCPNet::RegAddr ");

		int len = strlen(buf);
		ta->GetPrintForm(buf+len, sizeof(buf)-len);
		Log(buf);
	}
	else
	{
	   Log("CTCPNet	 NULL reg addr");
	}

}


CNetLink* CTcpNet::MakeSafeLink(CNetAddress* addr, LPVOID userData)
{
	
	TCPAddress& a = *(TCPAddress*) addr;
	DWORD err = 0;

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

	if (s == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		Log("CTcpNet::MakesfeLink - error creating socket");
		SetError(VPNET_ERR_WSOCK, err);
		VPTRACE(("MakeSafeLink socket creation error %d", err));		
		
		return NULL;
	}


	SOCKADDR_IN  sin;

	sin.sin_family = AF_INET;
	sin.sin_port = a.m_addr.m_streamPort;
	sin.sin_addr = a.m_addr.m_stationAddress;


	BOOL on = TRUE;
#if 0
	err = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (LPSTR) &on, sizeof(on)); 
#endif
	ConfigureSocket(s, FD_CONNECT|FD_CLOSE|FD_READ|FD_WRITE);


	int r = connect(s, (sockaddr*) &sin, sizeof(sin));

	LinkState st = CONNECTED;
	
	if (r == SOCKET_ERROR)
	{
		
		if ((err = WSAGetLastError()) != WSAEWOULDBLOCK)
		{
			VPTRACE(("connect error %d on %08x", err, s)); 
			Log("CTcpNet::MakesafeLink - error connecting socket");
			SetError(VPNET_ERR_WSOCK, err);
			closesocket(s);
			return NULL;
		}
		
		st = CONNECTING;

	} 

#if USE_LOCALHOST
	// This call uses to overcome IntraTask test case problem on Windows NT
	ConfigureSocket(s, FD_CLOSE|FD_READ|FD_WRITE);
#endif

	CTCPLink* l = new CTCPLink(s, st, this, userData);

	if (!l)
	{
		Log("CtcpNet::MakeSafeLiink - No memory for the link object");
		SetError(VPNET_ERR_NOMEM);
		VPTRACE(("MakeSafeLink:: Failed to create CTCPLink Object"));
		closesocket(s);
	}
	
   	return l;
}


CNetLink* CTcpNet::MakeUnsafeLink(LPVOID userData)
{

	if (m_dgLink)
	{
		m_dgLink->Ref();
		return m_dgLink;
	}

	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);


	if (s == INVALID_SOCKET)
	{
		Log("CTcpNet::MakeunsafeLink - error creating socket");
		SetError(VPNET_ERR_WSOCK, WSAGetLastError());
		return NULL;
	}


	BOOL trueValue = TRUE;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (LPCSTR) &trueValue,  sizeof(trueValue));

	ConfigureSocket(s, FD_READ|FD_WRITE);


	CTCPLink* l = new CTCPLink(s, DG, this, userData);

	if (!l)
	{
		Log("CtcpNet::MakeUnsafeLiink - No memory for the link object");
		SetError(VPNET_ERR_NOMEM);
		VPTRACE(("MakeUnsafeLink:: Failed to create CTCPLink Object"));
		closesocket(s);
	}
		

	
   	return l;
}


CTcpNet::CTCPLink*  CTcpNet::AcceptLink()
{

	SOCKET s = accept(m_listenLink->m_socket, NULL, NULL);


	if (s != INVALID_SOCKET)
	{
		CTCPLink* l = new CTCPLink(s, CONNECTED, this, NULL);

		if (!l)
		{
			Log("CtcpNet::AcceptLink - No memory for the link object");
			SetError(VPNET_ERR_NOMEM);
		}

		BOOL on = TRUE;
#if 0
		DWORD err = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (LPSTR) &on, sizeof(on));
#endif

		ConfigureSocket(s, FD_READ|FD_WRITE|FD_CLOSE);

		return l;
	 }

	 DWORD err = WSAGetLastError();
	 if (err == WSAEWOULDBLOCK)
	 	return NULL;
	 
	 Log("CtcpNet::AcceptLink - error accepting link");
	 SetError(VPNET_ERR_WSOCK, err);

	 return NULL;
}

	 


CTcpNet::CTCPLink* CTcpNet::MakeListenLink()
{

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

	if (s == INVALID_SOCKET)
	{
		Log("CtcpNet::MakeListenLink - cannt't create socket");

		SetError(VPNET_ERR_WSOCK, WSAGetLastError());
		return NULL;
	}

	ConfigureSocket(s, FD_ACCEPT);


	CTCPLink* l = new CTCPLink(s, CONNECTING, this, NULL);

	if (!l)
	{
		Log("CtcpNet::MakeListenLink - No memory for the link object");
		VPTRACE(("MakeListenLink:: Failed to create CTCPLink Object"));		
		closesocket(s);
	}

	
   	return l;
}


BOOL CTcpNet::Listen(BOOL streamListen, BOOL serverMode)
{

	m_dgLink = (CTCPLink*) MakeUnsafeLink();
	
	if (!m_dgLink)
		return FALSE;


	sockaddr_in sin;
	int namelen = sizeof(sin);

	if (serverMode)
		m_address.m_dgPort = m_wellKnownPort;

	sin.sin_family = AF_INET;
	sin.sin_addr = m_address.m_stationAddress;
	sin.sin_port = m_address.m_dgPort;


	if (bind(m_dgLink->m_socket, (sockaddr*) &sin, sizeof(sin)))
		goto handleerr;

//	ConfigureSocket(m_dgLink->m_socket, FD_READ|FD_WRITE);

	if (streamListen)
	{
		m_listenLink = MakeListenLink();

		if (!m_listenLink)
			goto nonwsaerr;

		sin.sin_port = 0;		
		if (bind(m_listenLink->m_socket, (sockaddr*) &sin, sizeof(sin)))
			goto handleerr;

		if (listen(m_listenLink->m_socket, 5))
			goto handleerr;

		if (getsockname(m_listenLink->m_socket, (sockaddr*) &sin, &namelen))
			goto handleerr;

  		m_address.m_streamPort = sin.sin_port;
	 }

	if (getsockname(m_dgLink->m_socket, (sockaddr*) &sin, &namelen))
		goto handleerr;

  	m_address.m_dgPort = sin.sin_port;
	m_address.m_stationAddress = sin.sin_addr;


	return TRUE;

handleerr:

	SetError(VPNET_ERR_WSOCK, WSAGetLastError());

nonwsaerr:
	if(m_dgLink)
		m_dgLink->Unref();
	
	m_dgLink = NULL;
	
	if (m_listenLink)
		m_listenLink->Unref();
	
	m_listenLink = NULL;
	return FALSE;


}



void CTcpNet::BecomeDeef()
{
	if (m_listenLink)
	{
		delete m_listenLink;
		m_listenLink = NULL;

	}

	
	if (m_dgLink)
	   ConfigureSocket(m_dgLink->m_socket, FD_WRITE);
  	

}

BOOL CTcpNet::IsServerSeen(LPVPNETADDRESS addr)
{
	if (m_serverSeen)
	{
		tcpaddress_s* ta = (tcpaddress_s*) addr;
		
		if (ta)
		{
			ta->m_stationAddress.s_addr = m_matchedAddress;
			ta->m_dgPort = m_matchedPort;
			ta->m_streamPort = 0;
		}
		return TRUE;
	}

	return FALSE;
}


void CTcpNet::Cleanup()
{
	delete m_listenLink;
	delete m_dgLink;


	m_listenLink = 0;
	m_dgLink = 0;
	m_address.m_dgPort = 0;
	m_address.m_streamPort = 0;
	CNetInterface::Cleanup();
}


CTcpNet::~CTcpNet()
{

	delete m_listenLink;
	delete m_dgLink;
	delete m_regAddr;  

	if (netCount-- == 1)
	{
		WSACleanup();
	}

}

CTcpNet::CTCPLink::CTCPLink(SOCKET sock, 
				   LinkState state, 
				   CTcpNet* net, 
				   LPVOID userData) : 
CNetLink(net, userData),  m_socket(sock), m_state(state),
m_inBuffer(NULL), m_inIndex(0), m_nextDgramSeqNum(0), m_nextDgramToSend(0)
{
  	 linkMap.Insert(this);

#if USE_SOCKBUF
	 if (m_state != DG)
	 {
		 m_inBuffer = new SockBuf;
	 }
#endif
}

		 
CTcpNet::CTCPLink::~CTCPLink() 
{
	char logBuf[128];

	wsprintf(logBuf, "CTcpLink::Closing link %08lx socket %u",
		this, m_socket);
	Log(logBuf);
 	closesocket(m_socket); 
	linkMap.Remove(this);
#if USE_SOCKBUF
	delete m_inBuffer;
#else
	delete [] m_inBuffer;
#endif
 }


BOOL CTcpNet::CTCPLink::EnqueueData(LPVOID data, DWORD dataSize, BOOL pfx)
{
	Datagram* d = new Datagram(data, (WORD) dataSize, pfx);

	if (!d)
	{
		Log("CTcpLink::Enqueue data - no memory for datagram object");
		SetError(VPNET_ERR_NOMEM);
		return FALSE;
	}

	if (!d->Ok())
	{
		delete d;
		Log("CTcpLink::Enqueue data - no memory for datagram object");
		SetError(VPNET_ERR_NOMEM);
		return FALSE;
	}

	d->SetSeq(m_nextDgramSeqNum++);
	d->SetSum(calcsum(data, dataSize));
	m_queue.Add(d);
	if (m_state == CONNECTED)
		m_net->SuspendHost();

	return TRUE;
}






CNetAddress* CTcpNet::CTCPLink::GetRemoteAddress()
{
	if (m_state != CONNECTED)
		return NULL;

    sockaddr_in   sin;
	int  namelen = sizeof(sin);
	
	int err = getpeername(m_socket, (sockaddr*) &sin, &namelen);

	if (!err)
	{
		
		TCPAddress* a = new TCPAddress(sin.sin_addr.s_addr, sin.sin_port, 0);
		if (!a)
		{
			Log("CTCPLink::GetRemoteAddress - no memory for address object");
			SetError(VPNET_ERR_NOMEM);
			return NULL;
		}
		return a;
	}


	Log("CTCPLink::GetRemoteAddress - getpeername gives error");
	SetError(VPNET_ERR_WSOCK, err = WSAGetLastError());
				  
	VPTRACE(("getpeername error %d on %08x", err, m_socket));		
	

	return NULL;
}

#if USE_SOCKBUF
DWORD CTcpNet::CTCPLink::LastDataTime() const
{
	return m_inBuffer->m_lastDataTime;
}
#endif
	

CTcpNet::CTCPLink* CTcpNet::CTCPLink::LookupLink(SOCKET s)
{
	LinkIter iter(linkMap);
	CTCPLink*  l;

	while(NULL != (l = iter()))
	{
		if (l->m_socket == s)
			return l;
	}
    return NULL;
}

void CTcpNet::OnTimer()
{
	CTCPLink*  l;
	CTCPLink::LinkIter iter(CTCPLink::linkMap);
	BOOL allClear = TRUE;
	DWORD t = GetCurrentTime();
	BOOL needKeek = t - m_lastKeekTime > 1000;

	while(NULL != (l = iter()))
	{
		if ((l->m_state == CONNECTED) && l->m_queue.Count())
		{
			l->SendWaitingData();
			if (allClear)
				allClear = l->m_queue.Count() == 0;

		}
		if (needKeek)
		{
			m_lastKeekTime = t;
			if ((l->m_state == CONNECTED) && (l->LastDataTime() - t > 1000))
				KeekNotifications(l->m_socket);
		}
	}

	if (allClear && HostIsBlocked())
		ResumeHost();
}


void CTcpNet::CheckFlow()
{

	if (!HostIsBlocked())
		return ;

	CTCPLink*  l;
	CTCPLink::LinkIter iter(CTCPLink::linkMap);

	while(NULL != (l = iter()))
	{
		if (l->m_queue.Count())
			return;
	}

	ResumeHost();
}


void CTcpNet::CTCPLink::SendWaitingData()
{
	char logBuf[128];
	int  maxSends = gMaxSends;

	if (m_socket == INVALID_SOCKET)
		return;

	m_queue.Lock();
	
	Datagram* d;

	if (gLogSocketFull)
	{
		wsprintf(logBuf, 
			"CTcpLink::SendWaitingData(%d) %u packets pending",
			m_socket, m_queue.Count());
		Log(logBuf);
	}

		
	while(maxSends-- && (NULL != (d = m_queue.Get())))
	{
		LPVOID data = d->MoreData();
		WORD   size = d->DataSize();
		DWORD t = GetCurrentTime();

#ifdef WIN32
		if (IsBadWritePtr(data, size))
		{
			wsprintf(logBuf, "CtcpLink::SendWaitingData BadWritePtr %08lx size=%lu", 
				data, size);

			Log(logBuf);
			break;
		}
#endif
		
		if (d->GetSeq() != m_nextDgramToSend)
		{
			wsprintf(logBuf, 
				"CtcpLink::SendWaitingData(%d): Seq error: d->seq = %lu, expected=%lu",
				d->GetSeq(), m_nextDgramToSend);
			Log(logBuf);
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			SetError(VPNET_ERR_LINK_LOST);
//			m_net->OnDisconnect(this);
			break;
		}

		WORD chksum = calcsum(data, size);
		if (d->GetSum() != chksum)
		{
			wsprintf(logBuf, 
				"CtcpLink::SendWaitingData(%d): chksum error: seq = %lu, d->sum = %lu, expected=%lu",
				m_socket, d->GetSeq(), d->GetSum(), chksum);
			Log(logBuf);
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			SetError(VPNET_ERR_LINK_LOST);

//			m_net->OnDisconnect(this);
			break;
		}
		int s = send(m_socket, (LPSTR) data, size, 0);

		if (s == SOCKET_ERROR) 
		{
			DWORD err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				if (gLogSocketFull)
				{
					wsprintf(logBuf, 
					"CTcpLink::SendWaitingData(%d) socket FULL",
						m_socket);
					Log(logBuf);
				}
				m_queue.Insert(d);
				break;
			}

			wsprintf(logBuf, 
				"CTcpLink::SendWaitingData(%d) send data - gives error",
				m_socket);
			Log(logBuf);
			SetError(VPNET_ERR_WSOCK, err);
			VPTRACE(("send error %d on %08x", err, m_socket));			
			// Check for connection loss
			// ....
			//
			break;
		}


		d->AddOffset((WORD) s);
		if (!d->Done())
		{
			wsprintf(logBuf, 
				"CTcpLink::SendWaitingData(%d) partial send", m_socket); 
			Log(logBuf);
			m_queue.Insert(d);
			break;
		}

		if (gLogSocketFull || ((t - d->m_time) > 50))
		{

			wsprintf(logBuf, "CTcpNet::SendWaitingData(%d) delay %ld ms "
			      "in data transmission", m_socket, t - d->m_time);
			Log(logBuf);
		}

		delete d;
		m_nextDgramToSend++;

	}
	
	m_queue.Unlock();
}
		

static int doSend(SOCKET sock, LPSTR data, size_t size, int flags)
{
	int s;
	static size_t fragSize = 1;
	static BOOL doFrag = TRUE;

	if (gSimulateFrags)
	{
		if (doFrag)
		{
			if (fragSize < size)
			{
				size = fragSize;
				fragSize *= 2;
			}
			else
				fragSize = 1;
		}

		s = send(sock,  data, size, 0);
		doFrag = !doFrag;
	}
	else
	{
		s = send(sock,  data, size, 0);
	}

	return s;
}

BOOL CTcpNet::CTCPLink::Send(LPVOID data, DWORD dataSize, DWORD flags)
{
	VPASSERT(m_state != DG);
	char logBuf[128];
	
	if (m_state == CONNECTING)
		return EnqueueData(data, dataSize, FALSE);

	if (m_queue.Count())
	{
		if (EnqueueData(data, dataSize, FALSE))
		{
		//	SendWaitingData();
		}
		return TRUE;
	}

	int s;
#if 0	
	WORD  ds = (WORD) dataSize;
	s = send(m_socket, (LPSTR) &ds, sizeof(ds), 0);
	DWORD err;

	if (s == SOCKET_ERROR) 
	{
		err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			return EnqueueData(data, dataSize, TRUE);

		VPTRACE(("CTCPLink::Send data size  on %08x returns error %d", m_socket, err));
		Log("CTCPLink::Send returns error");
		SetError(VPNET_ERR_WSOCK, err);
		return FALSE;
	}
#endif

	
	s = doSend(m_socket, (LPSTR) data, (size_t) dataSize, 0);
	DWORD err;

	if (s == SOCKET_ERROR) 
	{
		err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
		{
			wsprintf(logBuf, 
					"CTcpLink::Send(%d) socket FULL",
						m_socket);
			Log(logBuf);
			return EnqueueData(data, dataSize, FALSE);
		}

		VPTRACE(("CTCPLink::Send  on %08x returns error %d", m_socket, err));
		Log("CTCPLink::Send returns error");
		SetError(VPNET_ERR_WSOCK, err);
		return FALSE;
	}

	if (s < (int) (dataSize))
	{
		if (gSimulateFlow)
		   return EnqueueData(s + (LPSTR) data, dataSize - (DWORD) s, FALSE);
		else
		   return Send(s + (LPSTR) data, dataSize - (DWORD) s, flags);
	}

	return TRUE;
}


BOOL CTcpNet::CTCPLink::SendTo(CNetAddress& na, LPVOID data, DWORD dataSize, DWORD flags)
{
	VPASSERT(m_state == DG);
	tcpaddress_s& ta = ((TCPAddress&)na).m_addr;
	sockaddr_in  sin;

	sin.sin_family = AF_INET;
	sin.sin_port = ta.m_dgPort;
	sin.sin_addr = ta.m_stationAddress;
	
	if (flags & VP_BROADCAST)
		sin.sin_addr.s_addr = INADDR_BROADCAST;
		 

	

	int s = sendto(m_socket, (LPSTR) data, (size_t)dataSize, 0, (sockaddr*) &sin, sizeof(sin));

	DWORD err;

	if (s == SOCKET_ERROR) 
	{
		err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			return TRUE;
		Log("CTcpLink::SendTo - sendto gives error");
		SetError(VPNET_ERR_WSOCK, err);
		return FALSE;
	}


	return TRUE;
}



DWORD CTcpNet::CTCPLink::Receive(LPVOID data, DWORD dataSize)
{
	VPASSERT(m_state != DG);
	
	if (m_state == CONNECTING)
		return 0;

#if USE_SOCKBUF
	if (m_err)
		return 0;

	return m_inBuffer->GetData(data, dataSize);

#else
	int s = recv(m_socket, (LPSTR) data, (size_t) dataSize, 0);
	DWORD err;

	if (s == SOCKET_ERROR) 
	{
		err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			return 0;

		Log("CTcpLink::Receive -- recv - gives error");
		SetError(VPNET_ERR_WSOCK, err);
		return 0;
	}

	if (s == 0)
	{

		VPTRACE(("CTCPLink::Receive -- recv ZERO Bytes from %08x", m_socket));	
		Log("CTcpLink::Receive -- got ZERO bytes from socket");
		return s;
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		SetError(VPNET_ERR_LINK_LOST);
	}

	return s;
#endif
}



DWORD CTcpNet::CTCPLink::ReceiveFrom(LPVOID data, DWORD dataSize, CNetAddress& na)
{
	VPASSERT(m_state == DG);
   	sockaddr_in sin;
	int namelen = sizeof(sin);

	int s = recvfrom(m_socket, (LPSTR) data, (size_t)dataSize, 0, (sockaddr*) &sin, &namelen);
	DWORD err;

	if (s == SOCKET_ERROR) 
	{
		err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			return 0;

		SetError(VPNET_ERR_WSOCK, err);
		return 0;
	}

	if (s != 0)
	{
		TCPAddress& ta = (TCPAddress&) na;
		ta.m_addr.m_stationAddress = sin.sin_addr;
		ta.m_addr.m_dgPort = sin.sin_port;
		ta.m_addr.m_streamPort = 0;
		((CTcpNet*)m_net)->MatchAddress(sin.sin_addr.s_addr, sin.sin_port);

	}

	return s;
}

DWORD CTcpNet::CTCPLink::HasData()
{
	DWORD byteCount = 0;

#if USE_SOCKBUF
	if (m_state == DG)
	{
		if (!ioctlsocket(m_socket, FIONREAD, &byteCount))
			return byteCount;

		return 0;
	}

	byteCount = m_inBuffer->HasData(m_socket);
	
	if (!byteCount && m_inBuffer->m_err)
	{
		SetError(VPNET_ERR_WSOCK, m_inBuffer->m_err);
	}

	return byteCount;
#else

	if (!ioctlsocket(m_socket, FIONREAD, &byteCount))
		return byteCount;

	return 0;
#endif
}


