#ifndef __IPX16NET_H__
#define __IPX16NET_H__




//+ Address information for TCP

class CEcb;

#pragma pack(1)
struct ipx16_addr
{
    char  ipx16_netnum[4];
    char  ipx16_nodenum[6];

	
	BOOL operator==(const ipx16_addr& ipxa) const
	{
		return !memcmp(ipx16_netnum, ipxa.ipx16_netnum, sizeof(ipx16_netnum)) &&
			!memcmp(ipx16_nodenum, ipxa.ipx16_nodenum, sizeof(ipx16_nodenum));
	}

	BOOL operator!=(const ipx16_addr& ipxa) const
	{
		return !(ipxa == *this);
	}
					
};
#pragma pack()

inline WORD SwapWord(WORD w)
{
	return (w >> 8) | (w << 8);
}


#pragma pack(1)
struct  ipx16address_s 
{
	   ipx16_addr   m_stationAddress;     // IPX address
	   WORD    m_streamPort;			// port number for the stream connections
	   WORD    m_dgPort;				// port number for datagrams
};




#pragma pack()

class CIpx16Net;
class CIpx16Address;
class CIpx16Link;


class CIpxBufAlloc
{
public:

	CIpxBufAlloc();
	~CIpxBufAlloc();
	
	LPVOID  New(DWORD size);
	void    Free(LPVOID);
};


class CIpx16Address: public CNetAddress
{
public:
	ipx16address_s    m_addr;

	CIpx16Address(LPCVPNETADDRESS addr)
	{
		m_addr = *(const ipx16address_s*) addr;
	}

	CIpx16Address(const ipx16_addr& stationAddress, WORD sPort=0, WORD dgPort=0) 
	{
		m_addr.m_stationAddress = stationAddress;
		m_addr.m_streamPort = sPort;
		m_addr.m_dgPort = dgPort;
	}

	 void GetNormalForm(LPVPNETADDRESS ap)
	 {
		 *(ipx16address_s*)ap = m_addr;
	 }

	 BOOL IsEqual(CNetAddress* ap)
	 {
		 VPASSERT(ap);

		 ipx16address_s&  ta = ((CIpx16Address*)ap)->m_addr;

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

	static int declen(WORD v)
	{
		if (v < 10u) return 1;
		if (v < 100u) return 2;
		if (v < 1000u) return 3;
		if (v < 10000u) return 4;
		return 5;
	}
    
    static char* ipx16_ntoa(const ipx16_addr*);
    

	//+ Get Printable form of the address object (retValue == actual size of buffer needed)
	//+ The string returned is a.b.c.d:streamPort,datagramPort
	virtual DWORD GetPrintForm(LPSTR buf, DWORD bufSize)
	{
		char* p = ipx16_ntoa(&m_addr.m_stationAddress);
		size_t aLen = strlen(p);
		size_t p1Len = declen(SwapWord(m_addr.m_streamPort));
		size_t p2Len = declen(SwapWord(m_addr.m_dgPort));
		const DWORD neededLen = aLen + 1 + p1Len + 1 + p2Len + 1;

		if (bufSize < (aLen+1))
		{
			_fmemcpy(buf, p, (size_t)bufSize);

			return neededLen;
		} 

		_fmemcpy(buf, p, (size_t)aLen+1);

		bufSize -= aLen + 1;
		if (bufSize < p1Len + 1)
		{
			return neededLen;
		}

		buf += aLen;
		*buf++ = ':';

		ultoa(SwapWord(m_addr.m_streamPort), buf, 10);
		if (bufSize < p2Len + 1)
		{
			return neededLen;
		}

		buf += p1Len;
		*buf++ = ',';

		ultoa(SwapWord(m_addr.m_dgPort), buf, 10);


		return neededLen;

	}

   void  SetDgAddress(const IPXAddress* a)
   {
   		_fmemcpy(&m_addr.m_stationAddress, a, sizeof(m_addr.m_stationAddress));
     	m_addr.m_dgPort = *(LPWORD) a->socket;
   }	
   

};


class CEcb: public CVPLink
{

	
public:
    
    void* operator new(size_t s);
    void operator delete(void* p);
    
    enum Commands { CALL, LISTEN, SENDDG, RECEIVEDG, SEND, RECEIVE, HANGUP };
    
    BOOL    IsLost() const { return !IsQueued(); }
	BOOL    IsQueued() const { return m_onQueue; }
	void    SetQueued(BOOL f) { m_onQueue = f; }

	void    SetCommand(int cmd) 
	{
			m_cmd = cmd;
	}
	
	void    ClearFragCount() 
	{
	   m_ecb.fragmentCount = 0;
	} 
	
	void    SetBuffer(LPVOID buf, size_t s)
	{   
		m_ecb.fragmentCount = (buf == NULL) ? 1 : 2;
		m_ecb.fragmentDescriptor[1].size = s;
		m_ecb.fragmentDescriptor[1].address = buf;
	}	
	
	int     GetCommand() const
	{
		return m_cmd;
	}
    
    int     Status() const
    {
    	return m_ecb.completionCode;
    }
    
    BOOL    Completed() const
    {
    	return m_ecb.inUseFlag == 0;
    }
    
    WORD	GetConnNum() const
    {
    	return *(LPWORD) &m_ecb.IPXWorkspace;
    } 
    
#if 0   
    WORD    GetSize() const
    {
    	return m_ecb.fragmentDescriptor[1].size;
    }
#endif

    WORD    GetDataSize() const
    {
    	return SwapWord(m_hdr.length) - m_ecb.fragmentDescriptor[0].size;
    }
    
    LPVOID  GetBuffer() const
    {
    	return m_ecb.fragmentDescriptor[1].address;
    }
    
    void  SetSpxSocket(int port)
    {
    	m_ecb.socketNumber = port;
    	if (m_ecb.fragmentCount > 0)
    	{
    		m_ecb.fragmentDescriptor[0].size = sizeof(SPXHeader);
      		m_hdr.packetType = 5;
  		}
    	
    } 
    
    void  SetIpxSocket(int port)
    {
    	m_ecb.socketNumber = port;
    	if (m_ecb.fragmentCount > 0)
    	{
    		m_ecb.fragmentDescriptor[0].size = sizeof(IPXHeader);  
    		m_hdr.packetType = 4;
    	}
    	
    }
    

	BOOL SpxCloseNotification() const
	{
		return (m_hdr.dataStreamType >= 0xFE);
	}	
    
    WORD GetDestConnId() const
    {
    	return m_hdr.destConnectionID;
    }
    
	ECB		m_ecb;
	SPXHeader m_hdr;

	BOOL    m_onQueue;
	HWND	m_window;
	int     m_cmd;
	union   
	{
		CIpx16Net* m_net; 
		CIpx16Link* m_link;
	}  u;
	

	void SetCallAddress(CIpx16Address* a)
	{
		_fmemcpy(&m_hdr.destination,&a->m_addr.m_stationAddress, sizeof(a->m_addr.m_stationAddress)); 
		*(LPWORD)m_hdr.destination.socket = a->m_addr.m_streamPort;
	}
		
	void SetDgAddress(CIpx16Address* a)
	{
		_fmemcpy(&m_hdr.destination,&a->m_addr.m_stationAddress, sizeof(a->m_addr.m_stationAddress)); 
		*(LPWORD)m_hdr.destination.socket = a->m_addr.m_dgPort;
	}
	
	void SetBroadcastAddress(CIpx16Address* a)
	{
		SetDgAddress(a);
		_fmemset(m_ecb.immediateAddress, 0xff, sizeof(m_ecb.immediateAddress));
	} 	
		
	IPXAddress*  GetDgAddress()
	{
		return &m_hdr.source;
	}
	
	WORD GetSourceSocket()
	{
		return  *(LPWORD) m_hdr.source.socket;
	}
	
    
    typedef void (far* CompletionProc)();
    
	void SetCompletion(CompletionProc p)
	{
		m_ecb.ESRAddress = p;
	}
	
};


declare(VPList, CEcb);
typedef VPList(CEcb) CEcbList;

declare2(VPListIterator, CEcbList, CEcb);
typedef VPListIterator(CEcbList, CEcb) CEcbLiter;	

class CEcbQIter;
class CEcbQueue
{
public:
	friend CEcbQIter;

	size_t Count() const { return m_list.Count(); }
	void Append(CEcb* ecb) 
	{
		VPASSERT(!ecb->IsQueued());
		ecb->SetQueued(TRUE);
		m_list.Append(ecb);
	}

	void Insert(CEcb* ecb) 
	{
		VPASSERT(!ecb->IsQueued());
		ecb->SetQueued(TRUE);
		m_list.Insert(ecb);
	}

	void Remove(CEcb* ecb) 
	{
		VPASSERT(ecb->IsQueued());
		m_list.Remove(ecb);
		ecb->SetQueued(FALSE);
	}

	CEcb* RemoveFirst()
	{
		CEcb* ecb = m_list.RemoveFirst();
		
		if (ecb)
			ecb->SetQueued(FALSE);
		return ecb;
	}
protected:
	CEcbList  m_list;
};

				 
class CEcbQIter
{

public:
	CEcbQIter(CEcbQueue& q) : m_liter(q.m_list) { }

	CEcb* operator()() { return m_liter(); }

protected:
	CEcbLiter m_liter;
};

class CEcbAlloc
{
public:
	CEcbAlloc();
	~CEcbAlloc();
	
	CEcb*  New();
	void   Free(CEcb*);
	
protected:

	CEcbList  m_freeList;
	
};


class CIpx16Net;

class CIpx16Link: public CNetLink,public CVPLink 
{
  friend CIpx16Net;
  public:
     enum LinkState { DG, CONNECTING, CONNECTED, CLOSING } ;

	  CIpx16Net*	m_net;
	  CEcbList		m_inQ;
	  DataQueue		m_outQ;
	  int			m_state;
	  int			m_ipxErr;
	  CIpx16Address* m_remoteAddr;
	  WORD	  	m_connId;
   
	  DataQueue m_queue;




	  CIpx16Link(LinkState state, 
			   CIpx16Net* net, 
			   LPVOID userData);


	 ~CIpx16Link(); 
     
     BOOL Closing() const { return m_state == CLOSING; }
     
	 CNetAddress* GetRemoteAddress();

	 BOOL Send(LPVOID data, DWORD dataSize, DWORD flags);
	 DWORD Receive(LPVOID buf, DWORD bufSize);
	 DWORD ReceiveFrom(LPVOID buf, DWORD bufSize, CNetAddress& from);
	 BOOL SendTo(CNetAddress& to, 
				  LPVOID data, 
				  DWORD dataSize, 
				  DWORD flags);


     virtual DWORD HasData() { return m_inQ.Count() ? m_inQ.First()->GetDataSize() : 0; }
  
     void SetError2(int errCode, int ipxErr=0)
     {
     	m_ipxErr = ipxErr;
     	SetError(errCode);
     }
      

protected:
	void SubmitSpxEcb(CEcb* );
	void SubmitIpxEcb(CEcb* );
	
	CEcb*  MakeEcb(int cmdCode, LPVOID buf, DWORD size);
	void   DropEcb(CEcb* ecb);

	 BOOL   EnqueueData(LPVOID data, DWORD size);
	 BOOL   SendWaitingData();
	 
	 void   CleanDataQueue();
	 void   CleanEcbQueue();
	 void   OnReceiveComplete(CEcb* ecb);

	

};






class CIpx16Net:public CNetInterface
{
  public:
    enum { MAXDGRAMSIZE = 512 };
    
    typedef CIpx16Link::LinkState LinkState;

	declare(VPList, CIpx16Link);
	typedef VPList(CIpx16Link) LinkMap;

	declare2(VPListIterator, LinkMap, CIpx16Link);
    typedef VPListIterator(LinkMap, CIpx16Link) LinkIter;	

    
	static BOOL Supported();


	static const ipx16_addr IPX16ADDR_BROADCAST;
	static char* ipx16_ntoa(const ipx16_addr& addr)
	{
		return CIpx16Address::ipx16_ntoa(&addr);
	}

	 
	virtual void GetAddress(LPVPNETADDRESS addr)
	{
			*(ipx16address_s*)addr = m_address;
	}
		
	CNetLink* MakeSafeLink(CNetAddress* a, LPVOID userData=NULL);
	CNetLink* MakeUnsafeLink(LPVOID userData = NULL);
		
	CNetAddress* MakeAddress(LPCVPNETADDRESS a)
	{
		return new CIpx16Address(a ? a : (LPCVPNETADDRESS) &m_address);
	}

	virtual CNetAddress* MakeBroadcastAddress()
	{  
		if (!m_broadcastAddress)
			m_broadcastAddress = MakeAddressObject(IPX16ADDR_BROADCAST, 0, m_wellKnownPort);
		
		if (!m_broadcastAddress)
			return NULL;
				
		m_broadcastAddress->Ref();
		return m_broadcastAddress;	
	}

	//+ Create a NetAddress object used for server lookup
	virtual CNetAddress* MakeServerLookupAddress()
	{
		return MakeAddressObject(m_serverAddress, 0, m_wellKnownPort);
	}


	BOOL  Listen(BOOL streamListen = TRUE, BOOL serverMode = FALSE);
	void  BecomeDeef();

	//+ Stop all networking activity
	void  Cleanup();

	// Wait for completion of the given ECB... If the parameter is NULL wait for completion of ALL Ecbs
	virtual void WaitEcbCompletion(CEcb* ecb = NULL);

		
	CIpx16Address* MakeAddressObject(const ipx16_addr& stationAddr, WORD strmPort=0, WORD dgPort=0)
	{
		return new CIpx16Address(stationAddr, strmPort, dgPort);
	}

	//+ TCP doesn't keep packet boundaries
	virtual BOOL KeepingBoundaries() const
	{
		return TRUE;
	}
		

	virtual CIpx16Link* MakeLinkObj(LinkState state, CIpx16Address* addr, LPVOID userData);
		

	CIpx16Net(CTDLogger* log, HWND window, WORD wellKnownPort, ipx16_addr* server);
	~CIpx16Net();

	 CEcb*  NewEcb(int command)
	 {
		 CEcb* ecb = m_ecbAlloc.New();
		 if (ecb)
		 {
			 ecb->u.m_net = this;
			 ecb->m_window = m_window;
			 ecb->SetCommand(command);
			 ecb->SetQueued(FALSE);
			 return ecb;
		 }
		 SetError(VPNET_ERR_NOMEM);
		 return FALSE;
	 }

	 void DropEcb(CEcb* Ecb);
	 	 
	 void FreeEcb(CEcb* n) { m_ecbAlloc.Free(n); }

	 BOOL KnownActiveEcb(CEcb* );

	 BOOL HasActiveEcbs();
     void PostCleanup();
     
     
	 LPVOID  AllocBuffer(DWORD size,LPVOID contents=NULL)
	 {

		 LPVOID buf = m_bufAlloc.New(size);
		 if (buf && contents )
		 {
			 _fmemcpy(buf, contents, (size_t)size);
		 }
		 return buf;
	 }
	 void    FreeBuffer(LPVOID buf) {  m_bufAlloc.Free(buf); }
    
    
     
     void    SubmitEcb(CEcb* ecb);
 
	 virtual void  SetCompletionHook(CEcb* ecb);
	      
     virtual void    OnEcbCompletion(CEcb* ecb);

    
     virtual BOOL SubmitReceiveDgram();
 	 virtual BOOL SubmitSpxReceives();
 	 virtual BOOL SubmitIpxReceives();
 	 virtual BOOL SubmitListen();
 	 virtual BOOL SubmitOneListen();
 	 BOOL 	 SubmitOneSpxReceive();
	 
 	 
 	 
 	 void 	RegisterLink(CIpx16Link* l) { m_allLinks.Append(l); }
	 void   UnregisterLink(CIpx16Link* l) 
	 {
	 	VPASSERT(IsRegistered(l));
	 	m_allLinks.Remove(l);
	 	if (m_dgLink == l)
	 		m_dgLink = NULL;
	 	else if (m_listenLink == l)	
	 		m_listenLink = NULL;
	 }
	 	 
	 CIpx16Link*  FindLink(WORD connId); 
	 BOOL IsRegistered(CIpx16Link* l);    
	 void SetError2(int errCode, int ipxErr=0)
	 {
	 		m_ipxErr = ipxErr;
	 		SetError(errCode);
	 }

	 void OnTimer();
	 void CheckFlow();


protected:



public:
		
	   CIpx16Link*   	m_listenLink;
	   CIpx16Link*  	m_dgLink;
	   ipx16address_s   m_address;
	   ipx16_addr		m_serverAddress;      
	   CIpx16Address*   m_broadcastAddress;
	   WORD      		m_wellKnownPort;
	   WORD      		m_streamPort;
	   WORD      		m_dgPort;
	   DWORD	    	m_taskId;
	   int				m_ipxErr;
	   WORD				m_activeListens;
	   WORD				m_maxActiveListens;
	   WORD				m_activeIpxReceives;
	   WORD				m_maxIpxReceives;
	   WORD				m_activeSpxReceives;
	   WORD				m_maxSpxReceives;
	   CEcbAlloc    	m_ecbAlloc;
	   CIpxBufAlloc 	m_bufAlloc;
	   CEcbQueue    	m_activeEcbs;
	   HWND				m_window;
	   LinkMap     		m_allLinks;
	   BOOL       		m_cleaningUp;
	   
	   

};






#endif
