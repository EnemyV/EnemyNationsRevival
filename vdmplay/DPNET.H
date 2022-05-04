#ifndef __DPNET_H__
#define __DPNET_H__

class CDPBuffer;
class CDPBufPool;
union DPMSG;

#include <rw/tpslist.h>
typedef  RWTPtrSlist<CDPBuffer> CDPBufList;


class CDPAddress: public CNetAddress
{
public:
	DWORD m_addr;
	
	CDPAddress(int a) : m_addr(a) { }
	
	
	//+ Translate the address into VPNETADDRESS form
    virtual void GetNormalForm(LPVPNETADDRESS a)
	{
		*((LPDWORD) a->machineAddress) = m_addr;
	}

	virtual BOOL IsEqual(CNetAddress* a)
	{
		return m_addr == ((CDPAddress*) a)->m_addr;
	}
	
	//+ Get Printable form of the address object (retValue == actual size of buffer needed)
	virtual DWORD GetPrintForm(LPSTR buf, DWORD bufSize)
	{
		if (bufSize < 7)
			return 7;
		
		wsprintf(buf, "DP:%lx", m_addr);
		
		return 1+strlen(buf);
	}
			
	
	
};

struct IDirectPlay;

class CDPLink: public CNetLink, public CVPLink
{
	friend class CDPNet;

	public:
	 DWORD  m_remoteAddress;

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
			
	
	CDPLink(CDPNet* net, LPVOID userData, DWORD addr);
	virtual ~CDPLink();




protected:

	CDPNet* dpNet() const;
	CDPBufList	m_queue;

};

class CDPNet:  public CNetInterface
{
friend CDPLink;

public:


    virtual CNetLink*  MakeSafeLink(CNetAddress* addr, LPVOID userData=NULL);

	//+ Create a non-garanteed delivery NetLink
	virtual CNetLink*  MakeUnsafeLink(LPVOID userData=NULL);

	//+ Create a NetAddress object, if parameter is NULL the address
	//+ Is initialized with this interface address	
	virtual CNetAddress* MakeAddress(LPCVPNETADDRESS addr=NULL);

	
	//+ Create a broadcast NetAddress object
	virtual CNetAddress* MakeBroadcastAddress()
	{
		return MakeAddressObj(m_broadcastId);
	}
	
	virtual CNetAddress* MakeServerLookupAddress()
	{
		return NULL;
	}

	//+ Start listening for incoming connections and datagrams
    virtual BOOL   Listen(BOOL streamListen = TRUE, BOOL serverMode = FALSE);

	//+ Become deef to incoming data
	virtual void   BecomeDeef();

	//+ Get the address of this interface
    virtual void GetAddress(LPVPNETADDRESS addr);

	//+ return TRUE if underlying protocol is keeping packet boundaries on safe links
	virtual BOOL KeepingBoundaries() const { return TRUE; }


	//+ Stop all networking activity
	virtual void  Cleanup();


	void OnTimer();

	
    virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	CDPNet(CTDLogger* logger, LPCSTR transport, HWND wnd);
	virtual ~CDPNet();


    declare(VPList, CDPLink);
    typedef VPList(CDPLink) LinkMap;
  
	declare2(VPListIterator, LinkMap, CDPLink);
    typedef VPListIterator(LinkMap, CDPLink) LinkIter;
		  		

	virtual CDPAddress* MakeAddressObj(DWORD a);

	virtual BOOL AttachToSession(DWORD sid, BOOL forEnum = FALSE);
	virtual BOOL DetachFromSession();

	static void InitSessionDesc(LPDPSESSIONDESC sd);



protected:
	virtual CDPLink* MakeLinkObj(DWORD a, LPVOID userData);
	virtual CDPLink* FindLink(DWORD addr);
	virtual void OnSysDpMessage(DPMSG&);
	
	static BOOL PASCAL findServerCbk(DWORD pid, 	
						   LPSTR lpFriendlyName, 	
						   LPSTR lpFormalName,
						   DWORD dwFlags,
						   LPVOID ctx);


	static DWORD PASCAL dpThreadProc(LPVOID arg);

	CDPBuffer*  GetBuffer();
	void		FreeBuffer(CDPBuffer*);

public:
   IDirectPlay*   m_dp;
   DWORD		  m_myAddr;
   DWORD		  m_broadcastId;
   HANDLE		  m_hEvent;
   HANDLE		  m_hThread;
   LPVOID		  m_sesData;
   DWORD		  m_sesDataSize;
   DWORD		  m_selectedSession;
   DWORD		  m_serverPlayer;
   CDPLink*		  m_unsafeLnk;
   LinkMap		  m_linkMap;
   BOOL			  m_serverMode;
   BOOL			  m_forEnumeration;
   HWND			  m_hWnd;
   CDPBufPool*	  m_pool;
   int			  m_nameIndex;
   BOOL			  m_needClose;

};

inline CDPNet* CDPLink::dpNet() const { return (CDPNet*) m_net; }

CNetInterface* MakeDpNet(CTDLogger* logger, LPCSTR transportGuid, HWND wnd);
#endif
