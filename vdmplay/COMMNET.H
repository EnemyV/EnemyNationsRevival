#ifndef __COMMNET_H__
#define __COMMNET_H__

class CCommAddress: public CNetAddress
{
public:
	char    m_addr;
	
	CCommAddress(char a) : m_addr(a) { }
	
	
	//+ Translate the address into VPNETADDRESS form
    virtual void GetNormalForm(LPVPNETADDRESS a)
	{
		a->machineAddress[0] = m_addr;
	}

	virtual BOOL IsEqual(CNetAddress* a)
	{
		return m_addr == ((CCommAddress*) a)->m_addr;
	}
	
	//+ Get Printable form of the address object (retValue == actual size of buffer needed)
	virtual DWORD GetPrintForm(LPSTR buf, DWORD bufSize)
	{
		if (bufSize < 4)
			return 4;
		
	    strcpy(buf, "COM");
		return 1+strlen(buf);
	}
			
	
	
};


class CDataLink;
class CCommPort;

class CCommLink: public CNetLink
{
	friend class CCommNet;

	public:
//	 CCommNet*	  m_net;
	 CDataLink*   m_dLink;
	 BOOL 		  m_isSafe;	
	 BOOL		  m_connecting;	
	 char		  m_myAddr;
	 char		  m_remoteAddr;

	 virtual   BOOL Send(LPVOID data, DWORD dataSize, DWORD flags);
    
    //+ Read the data form the link, return actual count of bytes read
	virtual   DWORD Receive(LPVOID buf, DWORD bufSize);
    virtual   BOOL SendTo(CNetAddress& to, 
			              LPVOID data, 
						  DWORD dataSize, 
						  DWORD flags);

	//+ retValue = size of the datagram				  
	virtual	  DWORD ReceiveFrom(LPVOID buf, 
		            DWORD bufSize, 
			        CNetAddress& from);


	//+ How much data is waiting to be read?
	virtual DWORD HasData();
	


	//+ Get the adress of the peer
	virtual  CNetAddress* GetRemoteAddress();
			
	
	CCommLink(CCommNet* net, CDataLink* dLink, char a);
	virtual ~CCommLink();




	protected:

};


class CCommNet:public CNetInterface
{
friend CCommLink;

public:

    virtual CNetLink*  MakeSafeLink(CNetAddress* addr, LPVOID userData=NULL);

	//+ Create a non-garanteed delivery NetLink
	virtual CNetLink*  MakeUnsafeLink(LPVOID userData=NULL)
	{
		return MakeLinkObj();
	}

	//+ Create a NetAddress object, if parameter is NULL the address
	//+ Is initialized with this interface address	
	virtual CNetAddress* MakeAddress(LPCVPNETADDRESS addr=NULL);

	
	//+ Create a broadcast NetAddress object
	virtual CNetAddress* MakeBroadcastAddress()
	{
		return MakeAddress();
	}
	
	virtual CNetAddress* MakeServerLookupAddress()
	{
		return MakeAddress();
	}

	//+ Start listening for incoming connections and datagrams
    virtual BOOL   Listen(BOOL streamListen = TRUE, BOOL serverMode = FALSE);

	//+ Become deef to incoming data
	virtual void   BecomeDeef();

	//+ Get the address of this interface
    virtual void GetAddress(LPVPNETADDRESS addr);

	//+ return TRUE if underlying protocol is keeping packet boundaries on safe links
	virtual BOOL KeepingBoundaries() const { return TRUE; }


	//+ return TRUE for slow networks
	virtual BOOL IsSlowNet() { return TRUE; }


	//+ Stop all networking activity
	virtual void  Cleanup();


    CCommNet(CTDLogger* log, CCommPort* port);
	~CCommNet();

	void OnTimer();

	virtual void StartDataLog(CDataLogger* l);
	virtual void StopDataLog();	

	
    virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
protected:

	void   CheckCarrier();
	static BOOL DataLinkTest(LPVOID context);
	static BOOL CarrierTest(LPVOID context);


	virtual BOOL WaitDataLink();
	virtual BOOL WaitCarrier();
	virtual void StartDataLink();
	void    OnLinkDown();

	virtual CCommLink* MakeLinkObj(char a=0);
	virtual CCommAddress* MakeAddressObj(char ch);
 	
	static void OnLinkEvent(CDataLink* dl, int event, LPVOID userData);
    static void CancelCbk(LPVOID p);
	virtual void OnCancel();

	
public:
   CCommLink*   m_link;
   CCommPort*   m_port;
   CDataLink*   m_dLink;
   BOOL			m_listening;
   BOOL			m_waitingLink;
   BOOL			m_needCarrier;	
   BOOL			m_lastCarrier;
};



#endif /* */
