#ifndef __TCPNET_H__
#define __TCPNET_H__


#define USE_SOCKBUF	1
class SockBuf;

//+ Address information for TCP
struct  tcpaddress_s 
{
	   in_addr    m_stationAddress;     // Internet address
	   u_short    m_streamPort;			// port number for the stream connections
	   u_short    m_dgPort;				// port number for datagrams
};


class CTcpNet:public CNetInterface
{
  public:

     
    enum LinkState { DG, CONNECTING, CONNECTED } ;

	static BOOL Supported();

	// Tcp transport staion addres is composed from 3 elements
	// IP address, port for TCP connections, port for UDP datagrams
    class TCPAddress: public CNetAddress
	{
	public:
		tcpaddress_s    m_addr;
	    
		TCPAddress(LPCVPNETADDRESS addr)
		{
			m_addr = *(const tcpaddress_s*) addr;
		}
		
		TCPAddress(u_long stationAddress, u_short sPort=0, u_short dgPort=0) 
		{
			m_addr.m_stationAddress.s_addr = stationAddress;
			m_addr.m_streamPort = sPort;
			m_addr.m_dgPort = dgPort;
		}
		
		 void GetNormalForm(LPVPNETADDRESS ap)
		 {
			 *(tcpaddress_s*)ap = m_addr;
		 }
		 
		 BOOL IsEqual(CNetAddress* ap)
		 {
			 VPASSERT(ap);
			 
			 tcpaddress_s&  ta = ((TCPAddress*)ap)->m_addr;
			 
			 if (m_addr.m_stationAddress.s_addr != ta.m_stationAddress.s_addr)
				 return FALSE;
			 
			 if (m_addr.m_streamPort != 0 && ta.m_streamPort != 0)
			 {
				 if (m_addr.m_streamPort != ta.m_streamPort)
					 return FALSE;
			 }
			 
			 if (m_addr.m_dgPort != 0 && ta.m_dgPort != 0)
			 {
				 if (m_addr.m_dgPort != ta.m_dgPort)
					 return FALSE;
			 }
			 return TRUE;
		 }

		static int declen(u_short v)
		{
			if (v < 10u) return 1;
			if (v < 100u) return 2;
			if (v < 1000u) return 3;
			if (v < 10000u) return 4;
			return 5;
		}


		//+ Get Printable form of the address object (retValue == actual size of buffer needed)
		//+ The string returned is a.b.c.d:streamPort,datagramPort
		virtual DWORD GetPrintForm(LPSTR buf, DWORD bufSize)
		{
			char* p = inet_ntoa(m_addr.m_stationAddress);
			size_t aLen = strlen(p);
			size_t p1Len = declen(ntohs(m_addr.m_streamPort));
			size_t p2Len = declen(ntohs(m_addr.m_dgPort));
			const DWORD neededLen = aLen + 1 + p1Len + 1 + p2Len + 1;
	
			if (bufSize < (aLen+1))
			{
				_fmemcpy(buf, p, (size_t) bufSize);

				return neededLen;
			} 

			_fmemcpy(buf, p, aLen+1);

			bufSize -= aLen + 1;
			if (bufSize < p1Len + 1)
			{
				return neededLen;
			}

			buf += aLen;
			*buf++ = ':';
			
			ultoa(ntohs(m_addr.m_streamPort), buf, 10);
			if (bufSize < p2Len + 1)
			{
				return neededLen;
			}
			
			buf += p1Len;
			*buf++ = ',';

			ultoa(ntohs(m_addr.m_dgPort), buf, 10);


			return neededLen;

		}
						 

   	    static BOOL extractHostPart(LPSTR hostPart, size_t len, LPCSTR addrString);
   	    static void extractPortPart(tcpaddress_s& addr, LPCSTR addrString);

 		static BOOL TranslateAddressString(tcpaddress_s& addr, LPCSTR addrString);

	 };
		 
	class CTCPLink: public CNetLink,public CVPLink 
	{
	  public:
		  
		  SOCKET  m_socket;
		  DataQueue m_queue;
		  int       m_state;
#if USE_SOCKBUF
		  SockBuf*	m_inBuffer;
#else
		  LPSTR		m_inBuffer;
#endif
		  size_t	m_inIndex;
		  DWORD	    m_nextDgramSeqNum;
		  DWORD     m_nextDgramToSend;

//		  CTcpNet* m_net;	
				

		  declare(VPList, CTCPLink);
		  typedef VPList(CTCPLink) LinkMap;

		  declare2(VPListIterator, LinkMap, CTCPLink);

		  typedef VPListIterator(LinkMap, CTCPLink) LinkIter;
		  		
			  
		  CTCPLink(SOCKET sock, 
				   LinkState state, 
				   CTcpNet* net, 
				   LPVOID userData); 

		 
		 ~CTCPLink(); 
		 CNetAddress* GetRemoteAddress();

		 BOOL Send(LPVOID data, DWORD dataSize, DWORD flags);
		 DWORD Receive(LPVOID buf, DWORD bufSize);
		 DWORD ReceiveFrom(LPVOID buf, DWORD bufSize, CNetAddress& from);
		 BOOL SendTo(CNetAddress& to, 
					  LPVOID data, 
					  DWORD dataSize, 
					  DWORD flags);


		 virtual DWORD HasData();

		 static CTCPLink* LookupLink(SOCKET s);

		void SendWaitingData();

 		DWORD LastDataTime() const;

	public:
	       static LinkMap   linkMap;
	           
	protected:
		BOOL EnqueueData(LPVOID data, DWORD dataSize, BOOL pfx=FALSE);

	};



	virtual void GetAddress(LPVPNETADDRESS addr)
	{
			*(tcpaddress_s*)addr = m_address;
	}
		
	CNetLink* MakeSafeLink(CNetAddress* a, LPVOID userData=NULL);
	CNetLink* MakeUnsafeLink(LPVOID userData = NULL);
		
	CNetAddress* MakeAddress(LPCVPNETADDRESS a)
	{
		return new TCPAddress(a ? a : (LPCVPNETADDRESS) &m_address);
	}

	virtual CNetAddress* MakeBroadcastAddress()
	{
		return new TCPAddress(INADDR_BROADCAST, 0, m_wellKnownPort);
	}

	//+ Create a NetAddress object used for server lookup
	virtual CNetAddress* MakeServerLookupAddress()
	{
		return new TCPAddress(m_serverAddress, 0, m_serverPort);
	}


	virtual CNetAddress* MakeAddressFromString(LPCSTR addr);

	virtual CNetAddress* MakeRegistrationAddress()
	{
		if (m_regAddr)
		{
			m_regAddr->Ref();
		}

		return m_regAddr;

	}

	virtual void SetRegistrationAddress(LPCSTR addr);

	BOOL  Listen(BOOL streamListen = TRUE, BOOL serverMode = FALSE);
	void  BecomeDeef();

	//+ Stop all networking activity
	void  Cleanup();

	virtual void OnTimer();

		
	TCPAddress* MakeAddressObject(u_long stationAddr, u_short strmPort=0, u_short dgPort=0)
	{
		return new TCPAddress(stationAddr, strmPort, dgPort);
	}

	//+ TCP doesn't keep packet boundaries
	virtual BOOL KeepingBoundaries() const
	{
		return TRUE;
	}
		

	virtual CTCPLink* MakeListenLink();

    virtual void ConfigureSocket(SOCKET s, DWORD flags) = 0;


    virtual CTCPLink*  AcceptLink();

	virtual void CheckFlow();

	virtual BOOL IsServerSeen(LPVPNETADDRESS addr);

	CTcpNet(CTDLogger* log, u_short streamPort, u_short dgPort, u_short wellKnownPort, u_long serverAddress, LPCSTR srvAddrStr=NULL);
	~CTcpNet();

	 void MatchAddress(u_long addr, u_short port)
	 {
		 if (m_serverAddress == addr)
		 {
			 m_serverSeen = TRUE;
			 m_matchedAddress = addr;
			 m_matchedPort = port;
		 }
		 else if (m_regAddr && m_regAddr->m_addr.m_stationAddress.s_addr == addr)
		 {
			 m_serverSeen = TRUE;
			 m_matchedAddress = addr;
			 m_matchedPort = port;
		 }
	 }

	 virtual void KeekNotifications(SOCKET s) { }
	 
protected:



public:
		
	   CTCPLink*   m_listenLink;
	   CTCPLink*   m_dgLink;
	   tcpaddress_s   m_address;
	   u_short     m_wellKnownPort;
	   u_long      m_serverAddress;
	   u_short     m_serverPort;
	   TCPAddress* m_regAddr;
	   u_long	   m_matchedAddress;  // address of the server or Iserve 
	   DWORD	   m_sockBufSize;
	   u_short	   m_matchedPort;
	   DWORD	   m_lastKeekTime;
};

#endif
		 
