// net.cpp

#include "stdafx.h"
#include "net.h"
#include <nb30.h>
 
#include "../vpwinsk.h"
#include <wsipx.h>

static char helloMsg[] = "Hello form VPDIAG";


static BOOL winsockinit()
{
	WSADATA wsa;
	int err;

	err = WSAStartup(0x0101, &wsa);

	if (err != 0 || LOBYTE(wsa.wVersion) != 1 || HIBYTE(wsa.wVersion) != 1)
		return FALSE;

	return TRUE;
}



static BOOL WinsockProtoInit(int af, int proto)
{
    int err;
	BOOL doCleanup = FALSE;


	SOCKET s = socket(af, SOCK_DGRAM, proto);
	BOOL result = FALSE;

	if (s == INVALID_SOCKET)
	{
		err = WSAGetLastError();

		if (err == WSANOTINITIALISED)
		{
			if (winsockinit())
			{		
				doCleanup = TRUE;
				s = socket(proto, SOCK_DGRAM, 0);
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
	return result;
}



static void NbInitCallback(UINT LANA, LPVOID ctx)
{
	LPDWORD pDw	 = (LPDWORD) ctx;

	*pDw += 1;
}


BOOL Net_Init(BOOL& tcpOk, BOOL& ipxOk, BOOL& netbiosOk)
{

	if (winsockinit())
	{
		tcpOk = WinsockProtoInit(AF_INET, 0);
		ipxOk = WinsockProtoInit(AF_IPX, NSPROTO_IPX);
	}


	DWORD lanaCount = 0;

	Net_EnumLanas(NbInitCallback, &lanaCount);

	netbiosOk = lanaCount > 0;

	return TRUE;
}



CString Net_GetTcpHostName()
{
   char     Hostname[256];
  
   gethostname( Hostname, sizeof( Hostname ));
   return Hostname;
}

CString Net_GetTcpAddress()
{
   char     Hostname[256];
   HOSTENT *pHostEnt;
   int      nAdapter = 0;
 
   gethostname( Hostname, sizeof( Hostname ));

   pHostEnt = gethostbyname( Hostname );
 
   if (pHostEnt)
   {
	    struct in_addr addr;

		memcpy(&addr, pHostEnt->h_addr, pHostEnt->h_length);
		return inet_ntoa(addr);

   }
 


	return "127.0.0.1";
}

void ShowWinsockError(UINT errCode, LPCSTR msgTxt = NULL)
{
   char buf[256];

   if (!msgTxt)
	   msgTxt = "";

   wsprintf(buf, "WINSOCK Error %u %s", errCode-WSABASEERR, msgTxt); 
   AfxMessageBox(buf);


}

CNetSocket::~CNetSocket()
{
}

class CWinSocket: public CNetSocket
{
public:
	CWinSocket(SOCKET s) { m_bound = FALSE; m_socket = s; }
	virtual ~CWinSocket()
	{
		if (m_socket != INVALID_SOCKET)
			closesocket(m_socket);
	}
	virtual BOOL SendData(LPCSTR address, UINT port, LPCSTR data, UINT size, BOOL broadcast);

	virtual BOOL Ping(LPCSTR addr, UINT port, BOOL broadcast);
	virtual BOOL WaitData(UINT& port, BOOL broadcast);
	virtual BOOL ReadData(CString& data, CString& sender, UINT& port, LPARAM lParam);
	virtual BOOL Reply(LPCSTR addr, UINT port, LPCSTR data, UINT size);


protected:

	virtual BOOL MakeAddress(SOCKADDR& sAddr, UINT& addrLen, LPCSTR address, UINT port, BOOL broadcast) = 0;
	virtual BOOL MakeListenAddress(SOCKADDR& sAddr, UINT& addrLen, UINT& port) = 0;
	virtual BOOL MakeAddrStr(CString& addrStr, UINT& port, const SOCKADDR& sAddr, UINT addrLen) = 0;
	
public:
	SOCKET  m_socket;
	BOOL	m_bound;
};

BOOL   CWinSocket::SendData(LPCSTR address, UINT port, LPCSTR data, UINT size, BOOL broadcast)
{

	SOCKADDR  addr;
	UINT      addrLen;

	if (!MakeAddress(addr, addrLen, address, port, broadcast))
	{
		return FALSE;
	}

	int s = sendto(m_socket, data, size, 0, &addr, addrLen);


	if (s == SOCKET_ERROR) 
	{
		ShowWinsockError(WSAGetLastError());
		return FALSE;
	}


	return TRUE;
}

BOOL    CWinSocket::WaitData(UINT& port, BOOL broadcast)
{
	SOCKADDR  addr;
	UINT      addrLen;


	if (!m_bound)
	{
		if (!MakeListenAddress(addr, addrLen, port))
			return FALSE;

		if (bind(m_socket, &addr, addrLen))
		{
			ShowWinsockError(WSAGetLastError());
			return FALSE;
		}
		m_bound = TRUE;
	}
	return (TRUE);
}


BOOL	CWinSocket::Ping(LPCSTR address, UINT port, BOOL broadcast)
{
	return SendData(address, port, helloMsg, sizeof(helloMsg), broadcast);
}

BOOL CWinSocket::Reply(LPCSTR addr, UINT port, LPCSTR data, UINT size)
{
	return SendData(addr, port, data, size, FALSE);
}

BOOL CWinSocket::ReadData(CString& data, CString& sender, UINT& port, LPARAM lParam)
{
	SOCKADDR addr;
	int addrLen = sizeof(addr);
	int s;
	char buf[512];

	switch(WSAGETSELECTEVENT(lParam))
	{
		case FD_READ:
			s = recvfrom(m_socket, buf, sizeof(buf), 0, &addr, &addrLen);
			if (s == SOCKET_ERROR) 
			{
				UINT err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
					ShowWinsockError(err, " Reading data");
				return FALSE;
			}
			buf[s] = 0;
			data = buf;
			MakeAddrStr(sender, port, addr, addrLen);
			return TRUE;
			
	}


	return FALSE;


}
	 

class CTcpSocket: public CWinSocket
{
public:


	CTcpSocket(SOCKET s) : CWinSocket(s) { }

protected:
	virtual BOOL MakeAddress(SOCKADDR& sAddr, UINT& addrLen, LPCSTR address, UINT port, BOOL broadcast);
	virtual BOOL MakeListenAddress(SOCKADDR& sAddr, UINT& addrLen, UINT& port);

	virtual BOOL MakeAddrStr(CString& addrStr, UINT& port, const SOCKADDR& sAddr, UINT addrLen);


};

CNetSocket*  Net_MakeTcpSocket(HWND hWin, UINT msgCode)
{

	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);

	if (s == INVALID_SOCKET)
	{
		ShowWinsockError(WSAGetLastError());
		return NULL;
	}

	BOOL trueValue = TRUE;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (LPCSTR) &trueValue,  sizeof(trueValue));
	setsockopt(s, SOL_SOCKET, SO_USELOOPBACK, (LPCSTR) &trueValue,  sizeof(trueValue));

	WSAAsyncSelect(s, hWin, msgCode, FD_READ);

	return new CTcpSocket(s);
}


BOOL   CTcpSocket::MakeAddress(SOCKADDR& sAddr, UINT& addrLen, LPCSTR address, UINT port, BOOL broadcast)
{
	SOCKADDR_IN& addr = (SOCKADDR_IN&) sAddr;

	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;

	if (broadcast)
	{
		addr.sin_addr.s_addr = INADDR_BROADCAST;
	}
	else
	{
		size_t validLen;

		validLen = strspn(address, "0123456789.:,");

		if (validLen == strlen(address))
		{
			addr.sin_addr.s_addr = inet_addr(address);
		}
		else
		{
			LPHOSTENT  he = gethostbyname(address);


			if (!he)
			{
				ShowWinsockError(WSAGetLastError());
				return FALSE;
			}

			memcpy(&addr.sin_addr, he->h_addr, he->h_length);
		}
	}

	addr.sin_port = htons((u_short) port);
	addrLen = sizeof(addr);
	return TRUE;
}

BOOL CTcpSocket::MakeListenAddress(SOCKADDR& sAddr, UINT& addrLen, UINT& port)
{
	SOCKADDR_IN& addr = (SOCKADDR_IN&) sAddr;

	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;

	addr.sin_addr.s_addr = INADDR_ANY;

	if (!port)
	{
		port = DEFAULT_TCP_PORT;
	}

	addr.sin_port = htons(port);
	addrLen = sizeof(addr);

	return TRUE;
}





BOOL  CTcpSocket::MakeAddrStr(CString& str, UINT& port, const SOCKADDR& sAddr, UINT addrLen)
{
	const SOCKADDR_IN& sin = (const SOCKADDR_IN&) sAddr;

	str = inet_ntoa(sin.sin_addr);
	port = ntohs(sin.sin_port);
	return TRUE;
}



class CIpxSocket: public CWinSocket
{
public:


	CIpxSocket(SOCKET s) : CWinSocket(s) { }
	virtual BOOL SendData(LPCSTR address, UINT port, LPCSTR data, UINT size, BOOL broadcast);


protected:
	virtual BOOL MakeAddress(SOCKADDR& sAddr, UINT& addrLen, LPCSTR address, UINT port, BOOL broadcast);
	virtual BOOL MakeListenAddress(SOCKADDR& sAddr, UINT& addrLen, UINT& port);

	virtual BOOL MakeAddrStr(CString& addrStr, UINT& port, const SOCKADDR& sAddr, UINT addrLen);


};




CNetSocket*  Net_MakeIpxSocket(HWND hWin, UINT msgCode)
{
	SOCKET s = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);

	if (s == INVALID_SOCKET)
	{
		ShowWinsockError(WSAGetLastError());
		return NULL;
	}

	BOOL trueValue = TRUE;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (LPCSTR) &trueValue,  sizeof(trueValue));
	setsockopt(s, SOL_SOCKET, SO_USELOOPBACK, (LPCSTR) &trueValue,  sizeof(trueValue));

	WSAAsyncSelect(s, hWin, msgCode, FD_READ);

    

	return new CIpxSocket(s);

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

// Given a string of hex characterc pack it into trhe¨PACKED BCD form
static void packhex(LPSTR buf, size_t bufSize, LPCSTR str)
{
	size_t sourceDigits = strlen(str);
	size_t destDigits = bufSize*2;
	char* dPtr = buf;
	char srcbuf[64];
	char* sPtr = srcbuf;


	memset(buf, 0, bufSize);
   	if (sourceDigits & 1)  // odd number of source digits, simply add leading 0
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


BOOL   CIpxSocket::MakeAddress(SOCKADDR& sAddr, UINT& addrLen, LPCSTR address, UINT port, BOOL broadcast)
{
	SOCKADDR_IPX& addr = (SOCKADDR_IPX&) sAddr;

	memset(&addr, 0, sizeof(addr));

	addr.sa_family = AF_IPX;

	if (broadcast)
	{
		memset(addr.sa_netnum, 0, sizeof(addr.sa_netnum));
		memset(addr.sa_nodenum, 0xFF, sizeof(addr.sa_nodenum));
	}
	else
	{
		char buf[64];

		strcpy(buf, address);

		char* dot = strchr(buf, '.');

		if (dot)
		{
			*dot++ = 0;

			packhex(addr.sa_netnum, sizeof(addr.sa_netnum), buf);
		}
		else
		{
			dot = buf;
		}

		char* colon = strchr(dot, ':');

		if (colon)
			*colon++ = 0;

		packhex(addr.sa_nodenum, sizeof(addr.sa_nodenum), dot);

	}

	addr.sa_socket = htons((u_short) port);
	addrLen = sizeof(addr);
	return TRUE;
}

BOOL CIpxSocket::MakeListenAddress(SOCKADDR& sAddr, UINT& addrLen, UINT& port)
{
	SOCKADDR_IPX& addr = (SOCKADDR_IPX&) sAddr;

	memset(&addr, 0, sizeof(addr));

	addr.sa_family = AF_IPX;

	if (!port)
	{
		port = DEFAULT_IPX_PORT;
	}

	addr.sa_socket = htons(port);
	addrLen = sizeof(addr);

	return TRUE;
}



inline char htob(unsigned char ch)
{
    if (ch <= 9) return ('0' + ch);             // handle decimal values
    if (ch <= 0xf) return ('A' + ch - 10);      // handle hexidecimal specific values
    return('X');                                // Someone screwed up
}


static void htoa(const char * src, char * dest, int srclen)
{
    char * destptr; // temp pointers
    const unsigned char * srcptr;
        
    srcptr = (const UCHAR *)src;
    destptr = dest;

    while(srclen--)
    {
    *destptr++ = htob((UCHAR)(*srcptr >> 4));      // Convert high order nibble
    *destptr++ = htob((UCHAR)(*srcptr++ & 0x0F));  // Convert low order nibble
    }
    *destptr = 0;  // Null terminator
}


char* ipx_ntoa(LPSTR buf, const SOCKADDR_IPX& a)
{

  	htoa(a.sa_netnum, buf, 4);
  	buf[8]='.';
  	htoa(a.sa_nodenum, buf+9, 6);
  	buf[8+1+12] = 0;

  	return buf;
	

}

BOOL CIpxSocket::SendData(LPCSTR address, UINT port, LPCSTR data, UINT size, BOOL broadcast)
{
	if (!m_bound)
	{
		sockaddr_ipx sipx;

		memset(&sipx, 0, sizeof(sipx));
 		sipx.sa_family = AF_IPX;
	
		if (bind(m_socket, (sockaddr*) &sipx, sizeof(sipx)))
		{
			ShowWinsockError(WSAGetLastError());
			return FALSE;
		}	
		m_bound = TRUE;
	}
	return CWinSocket::SendData(address, port, data, size, broadcast);
}


CString Net_GetIpxAddress()
{

	sockaddr_ipx sipx;
	int namelen = sizeof(sipx);

	memset(&sipx, 0, sizeof(sipx));

	SOCKET s = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
	if (s == INVALID_SOCKET)
	{
		ShowWinsockError(WSAGetLastError());
		return "";
	}

	
	sipx.sa_family = AF_IPX;
	
	if (bind(s, (sockaddr*) &sipx, sizeof(sipx)))
	{
		ShowWinsockError(WSAGetLastError());
		return "";
	}	
		
	if (getsockname(s, (sockaddr*) &sipx, &namelen))
	{
		ShowWinsockError(WSAGetLastError());
		return "";
	}	
	

	closesocket(s);

	char buf[64]; 
	return ipx_ntoa(buf, sipx);

}




BOOL  CIpxSocket::MakeAddrStr(CString& str, UINT& port, const SOCKADDR& sAddr, UINT addrLen)
{
	const SOCKADDR_IPX& sipx = (const SOCKADDR_IPX&) sAddr;
	char buf[64];

	str = ipx_ntoa(buf, sipx);
	port = ntohs(sipx.sa_socket);
	return TRUE;
}






// Netbios Handling routines

void MakeNetbiosName (LPSTR achDest, LPCSTR szSrc);
BOOL NBAddName (int nLana, LPCSTR szName, int& myName);
BOOL NBReset (int nLana, int nSessions, int nNames);
BOOL NBListNames (int nLana, LPCSTR szName, void (*cbk)(UINT, UINT, LPVOID), LPVOID ctx);
BOOL NBAdapterStatus (int nLana, LPVOID pBuffer, int cbBuffer,
                      LPCSTR szName);
BOOL NBPostRead(int nLana, int nNum);
BOOL NBAddClientName(int nLana, int& myName);
BOOL NBDeleteName(int nLana, int myName);
BOOL NBSendDG(int nLana, int myName, LPCSTR callName, LPCSTR msg);

HWND nbhWnd;

static const char NB_serverName[]="VPDIAG";

void Net_SetWindow(HWND hWnd)
{ 
	nbhWnd = hWnd;
}

void NBGetLanas(LANA_ENUM* lEnum)
{
	UCHAR ret;
	NCB ncb;

	lEnum->length = 0;
	memset(&ncb, 0, sizeof(ncb));

	ncb.ncb_command = NCBENUM;
	ncb.ncb_length = sizeof(*lEnum);
	ncb.ncb_buffer = (PUCHAR) lEnum;

	ret = Netbios(&ncb);

	if (NRC_GOODRET == ret)
	{
		return;
	}
	

	ADAPTER_STATUS* as;
			
	as = new ADAPTER_STATUS;
	
	for (int iNum=0; iNum<MAX_LANA; iNum++)
	{
				
		memset (as, 0, sizeof (*as));
		memset (&ncb, 0, sizeof (ncb));
		ncb.ncb_command = NCBASTAT;
		ncb.ncb_buffer = (PUCHAR) as;
		ncb.ncb_length = sizeof (*as);
		ncb.ncb_callname[0] = '*';
		ncb.ncb_lana_num = iNum;

		ret = Netbios (&ncb);
		if (((ret == NRC_GOODRET) || (ret == NRC_INCOMP)) && (as->rev_major >= 1))
		{
			lEnum->lana[lEnum->length++] = iNum;
		}
	}		
	delete as;
}



void Net_EnumLanas(void (*callback)(UINT, LPVOID), LPVOID context)
{
	LANA_ENUM lEnum;

	NBGetLanas(&lEnum);
	for(int i = 0; i < lEnum.length; i++)
	{
		int lana = lEnum.lana[i];

		callback(lana, context);
	}
}



BOOL Net_InitServerLana(UINT lana)
{
	if (!NBReset (lana, 20, 30))
		return FALSE;

	char xName[32];
	int myName;

	if (NBAddName(lana, NB_serverName, myName))
	{
		wsprintf(xName,"%s_%d", NB_serverName, lana);

		if (!NBAddName(lana, xName, myName))
			return FALSE;

		BOOL ret = NBPostRead(lana, myName);

		return ret;
	}

	return FALSE;
}

void Net_ResetLanas()
{
	LANA_ENUM lEnum;

	NBGetLanas(&lEnum);

	for(int i = 0; i < lEnum.length; i++)
	{
		int lana = lEnum.lana[i];

		NBReset(lana, 16, 16);
	}

}


void Net_LookForServerOnLana(UINT  lana, void (*FoundLana)(UINT, UINT, LPVOID), LPVOID ctx)
{

	if (!NBReset (lana, 20, 30))
		return;

	NBListNames (lana, NB_serverName, FoundLana, ctx);
}

		





BOOL
NBReset (int nLana, int nSessions, int nNames)
{
    NCB ncb;
 
    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBRESET;
    ncb.ncb_lsn = 0;            /* Allocate new lana_num resources */
    ncb.ncb_lana_num = nLana;
    ncb.ncb_callname[0] = nSessions;  /* max sessions */
    ncb.ncb_callname[2] = nNames;  /* max names */
 
    Netbios (&ncb);
  
    return (NRC_GOODRET == ncb.ncb_retcode);
}
 
BOOL
NBAddName (int nLana, LPCSTR szName, int& nNum)
{
    NCB ncb;
 
    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBADDNAME;
    ncb.ncb_lana_num = nLana;
 
    MakeNetbiosName ((LPSTR) ncb.ncb_name, szName);
 
    Netbios (&ncb);
 	nNum = ncb.ncb_num;
 
    return (NRC_GOODRET == ncb.ncb_retcode);
}

void CALLBACK NBCallBack(PNCB ncb)
{
	if (ncb->ncb_retcode == NRC_GOODRET)
		PostMessage(nbhWnd, WM_NBNOTIFY, 0, (LPARAM) ncb->ncb_lana_num);

	delete [] (LPSTR) ncb->ncb_buffer;
	delete ncb;

}


BOOL NBPostRead(int nLana, int nNum)
{
    NCB* ncb = new NCB;
	char* buf = new char[512];

	if (!ncb)
		return NULL;
	
	memset(ncb, 0, sizeof(*ncb));
	ncb->ncb_command=NCBDGRECV|ASYNCH;
	ncb->ncb_num = nNum;
	ncb->ncb_length = 512;
	ncb->ncb_buffer = (PUCHAR) buf;
	ncb->ncb_lana_num = nLana;
	ncb->ncb_post = NBCallBack;
	char ret;

	ret = Netbios(ncb);
	return ret == NRC_GOODRET;
}

	
/*
 * MakeNetbiosName - Builds a name padded with spaces up to
 * the length of a NetBIOS name (NCBNAMSZ).
 */
void
MakeNetbiosName (LPSTR achDest, LPCSTR szSrc)
{
    int cchSrc;
 
    cchSrc = lstrlen (szSrc);
    if (cchSrc > NCBNAMSZ)
        cchSrc = NCBNAMSZ;
 
    memset (achDest, ' ', NCBNAMSZ);
    memcpy (achDest, szSrc, cchSrc);
}
 
BOOL
NBListNames(int nLana, LPCSTR szName, void (*cbk)(UINT, UINT, LPVOID), LPVOID ctx)
{
    int cbBuffer;
    ADAPTER_STATUS *pStatus;
    NAME_BUFFER *pNames;
	HGLOBAL hNames;
    int i;
 
    // Allocate the largest buffer we might need
    cbBuffer = sizeof (ADAPTER_STATUS) + 255 * sizeof (NAME_BUFFER);
	hNames = GlobalAlloc(GPTR, cbBuffer);
  
	if (NULL == hNames)
        return FALSE;


    pStatus = (ADAPTER_STATUS *) GlobalLock(hNames);
 
	// Get the list of the netios names from the machine specifed by szName
    if (!NBAdapterStatus (nLana, (PVOID) pStatus, cbBuffer, szName))
    {
        GlobalFree(hNames);
        return FALSE;
    }
 
    // The list of names immediately follows the adapter status
    // structure.
    pNames = (NAME_BUFFER *) (pStatus + 1);
 
	// is the our magic name is present in the list
	int srvNameLen = strlen(NB_serverName);
    for (i = 0; i < pStatus->name_count; i++)
	{
		LPCSTR nType = (pNames[i].name_flags & GROUP_NAME) ? "Group" : "Unique";
		LPCSTR pNm =  (LPCSTR) pNames[i].name;
       // printf ("\t%.*s\t%s\n", NCBNAMSZ, pNm, nType);
		if (!memicmp(NB_serverName, pNm, srvNameLen))
		{
			
		    if (pNm[srvNameLen] == '_')
			{
				// ok, the target machine is running VPDIAG in the server mode
				// extract the adapter number form the name
				int remoteLana = atoi(pNm+srvNameLen+1);
				char reply[64];
				
				// Update our window with the adpter number through which
				// we see the server
				PostMessage(nbhWnd, WM_NBNOTIFY, 0, nLana);

				int myName;

				// Send a datagram to the server saying it it has been seen
				if (NBAddClientName(nLana, myName))
				{
					wsprintf(reply, "Seeing you on my LANA %d", nLana);
					NBSendDG(nLana, myName, pNm, reply);
					cbk(nLana, remoteLana, ctx);
					NBDeleteName(nLana, myName);
				}
				break;
			}
		}


	}
 
    GlobalFree(hNames);
 
    return TRUE;
}
 
BOOL
NBAdapterStatus (int nLana, PVOID pBuffer, int cbBuffer, LPCSTR szName)
{
    NCB ncb;
 
    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBASTAT;
    ncb.ncb_lana_num = nLana;
 
    ncb.ncb_buffer = (PUCHAR) pBuffer;
    ncb.ncb_length = cbBuffer;
 
    MakeNetbiosName ((LPSTR) ncb.ncb_callname, szName);
 
    Netbios (&ncb);
 
    return (NRC_GOODRET == ncb.ncb_retcode);
}

BOOL NBSendDG(int nLana, int myName, LPCSTR name, LPCSTR data)
{
    NCB ncb;
 
    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBDGSEND;
    ncb.ncb_lana_num = nLana;
	ncb.ncb_num = myName;
    MakeNetbiosName ((LPSTR) ncb.ncb_callname, name);
 
    ncb.ncb_buffer = (PUCHAR) data;
    ncb.ncb_length = strlen(data)+1;

	UCHAR ret = Netbios(&ncb);
	return ret == NRC_GOODRET;
}

BOOL NBDeleteName(int nLana, int myName)
{
    NCB ncb;
 
    memset (&ncb, 0, sizeof (ncb));
    ncb.ncb_command = NCBDELNAME;
    ncb.ncb_lana_num = nLana;
	ncb.ncb_num = myName;

	UCHAR ret = Netbios(&ncb);
	return ret == NRC_GOODRET;
}


BOOL NBAddClientName(int nLana, int& nNum)
{
	int tries = 5;
	char buf[32];
	NCB ncb;
	
	memset(&ncb, 0, sizeof(ncb));

	TRACE("Netbios: Trying to generate a name");

	while(tries--)
	{
		wsprintf(buf, "VPDIAG%lx", GetCurrentTime());
		if (NBAddName(nLana, buf, nNum))
			return TRUE;
	}

	return FALSE;
}



void vpassertion(LPCSTR text, LPCSTR file, int line);

void (*_vpassert)(LPCSTR text, LPCSTR file, int line) = vpassertion;

void vpassertion(LPCSTR text, LPCSTR file, int line)
{
	char buf[512];


	wsprintf(buf, "Assertion (%s) in file %s at line %d", text, file, line);
	int iRet = AfxMessageBox(buf, MB_RETRYCANCEL);
	if (iRet == IDRETRY)
	{
		AfxDebugBreak();
	}
	else
		abort();
}
		
