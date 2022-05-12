#ifndef __VPNET_H__
#define __VPNET_H__

class CDataLogger;
class CNetAddress;
class CTDLogger;
class CNetInterface;
enum 
{ 
	VPNET_ERR_LINK_LOST = 1,
	VPNET_ERR_NOMEM,
	VPNET_ERR_INIT,
	VPNET_ERR_NETBIOS,
	VPNET_ERR_IPX16,
	VPNET_ERR_WSOCK,
	VPNET_ERR_COMM,
	VPNET_ERR_DP,		

};
	
//+ A network link
//+ Send/Recieve data
class CNetLink: public CRef
{
public:

	//+ Send the data to the link
	virtual   BOOL Send(LPVOID data, DWORD dataSize, DWORD flags) = 0;
    
    //+ Read the data form the link, return actual count of bytes read
	virtual   DWORD Receive(LPVOID buf, DWORD bufSize) = 0;
    virtual   BOOL SendTo(CNetAddress& to, 
			              LPVOID data, 
						  DWORD dataSize, 
						  DWORD flags) = 0;

	//+ retValue = size of the datagram				  
	virtual	  DWORD ReceiveFrom(LPVOID buf, 
		            DWORD bufSize, 
			        CNetAddress& from) = 0;


	//+ How much data is waiting to be read?
	virtual DWORD HasData() = 0;
					
	DWORD LastError()
	{
		DWORD e = m_err;
		m_err = 0;
		return e;
	}


	//+ Get the adress of the peer
	virtual  CNetAddress* GetRemoteAddress() = 0;
	
	CNetLink(CNetInterface* net, LPVOID userData) : m_err(0), m_userData(0), m_net(net) { }
	virtual ~CNetLink() { }
	
	LPVOID UserData() { return m_userData; }

	void Log(LPCSTR);	

protected:
     void SetError(DWORD e, DWORD xe = 0);
	 
public:
	DWORD m_err;
	CNetInterface* m_net;
    LPVOID   m_userData;
	
};



//+ Network address
//+ Allow for Net Independent way to manage and compare netwokr identity information
class CNetAddress:public CVPLink, public CRef
{
public:
	//+ Translate the address into VPNETADDRESS form
    virtual void GetNormalForm(LPVPNETADDRESS a) = 0;

	virtual BOOL IsEqual(CNetAddress* a) = 0;

	//+ Get Printable form of the address object (retValue == actual size of buffer needed)
	virtual DWORD GetPrintForm(LPSTR buf, DWORD bufSize) = 0;
	virtual ~CNetAddress() { }

	void* operator new(size_t s);
	void operator delete(void*);
    unsigned       Unref() 
	{ 
		unsigned ret = --m_refCnt;
		if (!ret)
			delete this;
		return ret;
	}


	static void  ResetPool();



protected:
	declare(VPList, CNetAddress);
	typedef VPList(CNetAddress) AddrList;

	static AddrList* m_pool;



};

//+ Netwokr interface, 
//+ Create and Manage NetLink objects,
//+ Notifies client on significiant network events (Data/Connection arrival, Connection close)  
class CNetInterface
{
friend CNetLink;
public:
    typedef void (*NetHook)(CNetLink* lnk, LPVOID data);
	
    //+ Create a garanteed delivery NetLink to the given destination
    virtual CNetLink*  MakeSafeLink(CNetAddress* addr, LPVOID userData=NULL) = 0;

	//+ Create a non-garanteed delivery NetLink
	virtual CNetLink*  MakeUnsafeLink(LPVOID userData=NULL) = 0;

	//+ Create a NetAddress object, if parameter is NULL the address
	//+ Is initialized with this interface address	
	virtual CNetAddress* MakeAddress(LPCVPNETADDRESS addr=NULL) = 0;

	//+ Create a broadcast NetAddress object
	virtual CNetAddress* MakeBroadcastAddress() = 0;

	//+ Create a NetAddress object used for server lookup
	virtual CNetAddress* MakeServerLookupAddress() = 0;

	//+ Start listening for incoming connections and datagrams
    virtual BOOL   Listen(BOOL streamListen = TRUE, BOOL serverMode = FALSE) = 0;

	//+ Become deef to incoming data
	virtual void   BecomeDeef() = 0;

	//+ Get the address of this interface
    virtual void GetAddress(LPVPNETADDRESS addr) = 0;



	virtual void StartDataLog(CDataLogger* l); 

	virtual void StopDataLog();


	virtual CNetAddress* MakeAddressFromString(LPCSTR addr);

	virtual CNetAddress* MakeRegistrationAddress();

	virtual void SetRegistrationAddress(LPCSTR addr);


	//+ return TRUE if underlyiong protocol is keeping packet boundaries on safe links
	virtual BOOL KeepingBoundaries() const = 0;

	//+ return TRUE for slow networks
	virtual BOOL IsSlowNet() { return FALSE; }

	//+ Stop all networking activity
	virtual void  Cleanup()
	{
		SetHooks(NULL, NULL, NULL, NULL, NULL, NULL);
	}


    CNetInterface(CTDLogger* log); 


    virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual void OnTimer();


    virtual ~CNetInterface();

	void SetHooks(NetHook safeProc, 
					 NetHook unsafeProc,
					 NetHook connectProc,
					 NetHook disconnectProc,
				     NetHook acceptProc,
							LPVOID data)
	{
		m_safeHook = safeProc;
		m_unsafeHook = unsafeProc;
		m_connectHook = connectProc;
		m_disconnectHook = disconnectProc;
		m_acceptHook = acceptProc;
		m_hookData = data;
	}
	

	DWORD LastError()
	{
		DWORD e = m_err;
		m_err = 0;
		return e;
	}

	virtual void CheckFlow() { }

	void SuspendHost();
	void ResumeHost();
	BOOL HostIsBlocked() const
	{
		return m_lastStopTime != 0;
	}
		
	void SetHostWindow(HWND h)
	{
		m_hostWin = h;
	}

	virtual BOOL IsServerSeen(LPVPNETADDRESS addr)
	{
		return FALSE;
	}

protected:

    void SetError(DWORD e, DWORD xe=0);
 	void FatalError(DWORD e, DWORD xe = 0);
	void Log(LPCSTR m);


	void OnConnect(CNetLink* l)
	{
		if (m_connectHook)
			m_connectHook(l, m_hookData);
	}

	void OnAccept(CNetLink* l)
	{
		if (m_acceptHook)
			m_acceptHook(l, m_hookData);
	}


	void OnDisconnect(CNetLink* l)
	{
		if (m_disconnectHook)
			m_disconnectHook(l, m_hookData);
	}


	void OnSafeData(CNetLink* l)
	{
		if (m_safeHook)
			m_safeHook(l, m_hookData);
	}

	void OnUnsafeData(CNetLink* l)
	{
		if (m_unsafeHook)
			m_unsafeHook(l, m_hookData);
	}


protected:
	 NetHook m_safeHook;          // hook to call when safe data arrives
	 NetHook m_unsafeHook;		  // hook to call when unsafe data arrives
	 NetHook m_connectHook;		  // hook to call when connection complets
	 NetHook m_disconnectHook;	  // hook to call when connection is closed by the peer
	 NetHook m_acceptHook;        // hook to call when connection arrives

  	 LPVOID       m_hookData;	  // data parameter for the hook procedures
	 DWORD   	  m_err;
	 DWORD		  m_xerr;
	 CTDLogger*   m_errLog;
	 CDataLogger* m_dataLog;
	 DWORD		  m_lastStopTime;
	 HWND		  m_hostWin;
	 BOOL		  m_serverSeen;   // set to TRUE if we soaw packet from server or Iserve
};


inline void CNetLink::Log(LPCSTR m) { m_net->Log(m); }


#endif /* */
