#ifndef __WSIPXNET_H__
#define __WSIPXNET_H__


//+ Address information for TCP


#pragma pack(1)
struct ipx_addr
{
    char  ipx_netnum[4];
    char  ipx_nodenum[6];

	ipx_addr& operator=(const SOCKADDR_IPX& sipx)
	{
		*this = *(const ipx_addr*) sipx.sa_netnum;
		return *this;
	}
	
	BOOL operator==(const ipx_addr& ipxa) const
	{
		return !memcmp(ipx_netnum, ipxa.ipx_netnum, sizeof(ipx_netnum)) &&
			!memcmp(ipx_nodenum, ipxa.ipx_nodenum, sizeof(ipx_nodenum));
	}

	BOOL operator!=(const ipx_addr& ipxa) const
	{
		return !(ipxa == *this);
	}
					
};
#pragma pack()

inline SOCKADDR_IPX& copy(SOCKADDR_IPX& sipx, const ipx_addr& ipx)
{
	 (*(ipx_addr*) sipx.sa_netnum) = ipx;
	 return sipx;
}


#pragma pack(1)
struct  ipxaddress_s 
{
	   ipx_addr   m_stationAddress;     // IPX address
	   u_short    m_streamPort;			// port number for the stream connections
	   u_short    m_dgPort;				// port number for datagrams
};

#pragma pack()


class CWSIpxNet:public CNetInterface
{
  public:
    enum LinkState { DG, CONNECTING, CONNECTED } ;

	static BOOL Supported();


	static const ipx_addr IPXADDR_BROADCAST;
	static char* ipx_ntoa(const ipx_addr& addr);
	static BOOL  parseAddressString(ipxaddress_s& adrr, LPCSTR adrString);
	 
	// Tcp transport staion addres is composed from 3 elements
	// IP address, port for TCP connections, port for UDP datagrams
    class IPXAddress: public CNetAddress
	{
	public:
		ipxaddress_s    m_addr;
	    
		IPXAddress(LPCVPNETADDRESS addr)
		{
			m_addr = *(const ipxaddress_s*) addr;
		}
		
		IPXAddress(const ipx_addr& stationAddress, u_short sPort=0, u_short dgPort=0) 
		{
			m_addr.m_stationAddress = stationAddress;
			m_addr.m_streamPort = sPort;
			m_addr.m_dgPort = dgPort;
		}
		


		 void GetNormalForm(LPVPNETADDRESS ap)
		 {
			 *(ipxaddress_s*)ap = m_addr;
		 }
		 
		 BOOL IsEqual(CNetAddress* ap)
		 {
			 VPASSERT(ap);
			 
			 ipxaddress_s&  ta = ((IPXAddress*)ap)->m_addr;
			 
			 if (m_addr.m_stationAddress != ta.m_stationAddress)
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
			char* p = ipx_ntoa(m_addr.m_stationAddress);
			size_t aLen = strlen(p);
			size_t p1Len = declen(ntohs(m_addr.m_streamPort));
			size_t p2Len = declen(ntohs(m_addr.m_dgPort));
			const DWORD neededLen = aLen + 1 + p1Len + 1 + p2Len + 1;
	
			if (bufSize < (aLen+1))
			{
				_fmemcpy(buf, p, bufSize);

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
						 


	 };
		 
	class CIPXLink: public CNetLink,public CVPLink 
	{
	  public:
		  
		  SOCKET  m_socket;
		  DataQueue m_queue;
		  int       m_state;
//		  CWSIpxNet* m_net;	
				

		  declare(VPList, CIPXLink);
		  typedef VPList(CIPXLink) LinkMap;

		  declare2(VPListIterator, LinkMap, CIPXLink);

		  typedef VPListIterator(LinkMap, CIPXLink) LinkIter;
		  		
			  
		  CIPXLink(SOCKET sock, 
				   LinkState state, 
				   CWSIpxNet* net, 
				   LPVOID userData) : 
			CNetLink(net, userData), 
			  m_socket(sock), m_state(state) // , m_net(net)
		  {
		  	 linkMap.Insert(this);
		  }

		 
		 ~CIPXLink() 
		 { 
		 	closesocket(m_socket); 
			linkMap.Remove(this);
		 }
		 
		 CNetAddress* GetRemoteAddress();

		 BOOL Send(LPVOID data, DWORD dataSize, DWORD flags);
		 DWORD Receive(LPVOID buf, DWORD bufSize);
		 DWORD ReceiveFrom(LPVOID buf, DWORD bufSize, CNetAddress& from);
		 BOOL SendTo(CNetAddress& to, 
					  LPVOID data, 
					  DWORD dataSize, 
					  DWORD flags);


		 virtual DWORD HasData();

		 static CIPXLink* LookupLink(SOCKET s);

		void SendWaitingData();
		
	public:
	       static LinkMap   linkMap;
	           
	protected:
		BOOL EnqueueData(LPVOID data, DWORD dataSize);

	};



	virtual void GetAddress(LPVPNETADDRESS addr)
	{
			*(ipxaddress_s*)addr = m_address;
	}
		
	CNetLink* MakeSafeLink(CNetAddress* a, LPVOID userData=NULL);
	CNetLink* MakeUnsafeLink(LPVOID userData = NULL);
		
	CNetAddress* MakeAddress(LPCVPNETADDRESS a)
	{
		return new IPXAddress(a ? a : (LPCVPNETADDRESS) &m_address);
	}

	virtual CNetAddress* MakeBroadcastAddress()
	{
		return new IPXAddress(IPXADDR_BROADCAST, 0, m_wellKnownPort);
	}
 	
 	//+ Create a NetAddress object used for server lookup
	virtual CNetAddress* MakeServerLookupAddress() 
	{
		return new IPXAddress(m_serverAddress, 0, m_serverPort);
	}


	virtual CNetAddress* MakeRegistrationAddress()
	{
		if (m_regAddr)
		{
			m_regAddr->Ref();
		}

		return m_regAddr;

	}

	virtual void SetRegistrationAddress(LPCSTR addr)
	{
		if (m_regAddr)
		{
			m_regAddr->Unref();
		}

		IPXAddress* ipxa = (IPXAddress*) MakeAddressFromString(addr);
		m_regAddr = ipxa;
		if (ipxa && !ipxa->m_addr.m_dgPort)
			ipxa->m_addr.m_dgPort = m_wellKnownPort;

	}

	
	virtual CNetAddress* MakeAddressFromString(LPCSTR addrString);


	BOOL  Listen(BOOL streamListen = TRUE, BOOL serverMode = FALSE);
	void  BecomeDeef();

	//+ Stop all networking activity
	void  Cleanup();


		
	IPXAddress* MakeAddressObject(const ipx_addr& stationAddr, u_short strmPort=0, u_short dgPort=0)
	{
		return new IPXAddress(stationAddr, strmPort, dgPort);
	}

	//+ TCP doesn't keep packet boundaries
	virtual BOOL KeepingBoundaries() const
	{
		return FALSE;
	}
		

	virtual CIPXLink* MakeListenLink();

    virtual void ConfigureSocket(SOCKET s, DWORD flags) = 0;


    virtual CIPXLink*  AcceptLink();

	void OnTimer();
	void CheckFlow();

	CWSIpxNet(CTDLogger* log, u_short streamPort, u_short dgPort, u_short wellKnownPort, ipx_addr* serverAddr=NULL,
	  LPCSTR srvAddrString=NULL);
	~CWSIpxNet();


protected:



public:
		
	   CIPXLink*   m_listenLink;
	   CIPXLink*   m_dgLink;
	   ipxaddress_s   m_address;
	   u_short     m_wellKnownPort;
	   ipx_addr    m_serverAddress;
	   u_short	   m_serverPort;	
  	   CNetAddress* m_regAddr;


};






#endif
