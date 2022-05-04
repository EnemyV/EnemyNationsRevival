#ifndef __NBNET_H__
#define __NBNET_H__

class CNbLink;
class CNbNet;


class CNcb: public CVPLink
{
public:
	BOOL    IsLost() const { return !IsQueued(); }
	BOOL    IsQueued() const { return m_onQueue; }
	void    SetQueued(BOOL f) { m_onQueue = f; }

	void    SetCommand(int command)
	{
		m_ncb.ncb_command = (UCHAR) command;
	}

	void    SetLsn(int lsn)
	{
		m_ncb.ncb_lsn = (UCHAR) lsn;
	}
		
	void    SetBuffer(LPVOID buf=NULL, WORD size = 0)
	{
		m_ncb.ncb_buffer = (PUCHAR) buf;
		m_ncb.ncb_length = size;
	}
	
	void    SetLocalName(LPCSTR n)
	{
		_fmemcpy(m_ncb.ncb_name, n, sizeof(m_ncb.ncb_name));
	}
	
	void    SetCallName(LPCSTR n)
	{
		_fmemcpy(m_ncb.ncb_callname, n, sizeof(m_ncb.ncb_callname));
	}
	
				 
	void    SetSync()
	{
		m_ncb.ncb_command &= ~ASYNCH;
	}	
		
	void    SetAsync()
	{
		m_ncb.ncb_command |= ASYNCH;
	}
	
	void    SetNameNum(int index)
	{
		m_ncb.ncb_num = (UCHAR) index;
	}

	void    SetLana(int lana)
	{
		m_ncb.ncb_lana_num = (UCHAR) lana;
	}

	typedef  void (CALLBACK *NcbPost)(PNCB ); /* POST routine address        */


	void    SetCompletion(NcbPost proc )
	{
		m_ncb.ncb_post = proc;
	}

	
	BOOL    IsAsync() const { return 0 != (CommandCode() & ASYNCH); }
	int	    CommandCode() const
	{
		return m_ncb.ncb_command;
	}
	
	WORD    GetSize() const { return m_ncb.ncb_length; }
	
	LPVOID  GetBuffer() const { return (LPVOID) m_ncb.ncb_buffer; }
	int     GetLsn() const { return m_ncb.ncb_lsn; }
	
	LPCSTR  GetCallName() const { return (LPCSTR) m_ncb.ncb_callname; }
	UCHAR   Status() const { return m_ncb.ncb_cmd_cplt; }
	UCHAR   GetNameNum() const { return m_ncb.ncb_num; }
	
	BOOL    Done() const {  return Status() != 0xff; }	

	
	
	NCB		m_ncb;
	HWND    m_window;
	CNbNet*  m_net;
	CNbLink* m_link;
	BOOL    m_onQueue;
	BOOL    m_postFailed;


};

declare(VPList, CNcb);
typedef VPList(CNcb) CNcbList;

declare2(VPListIterator, CNcbList, CNcb);
typedef VPListIterator(CNcbList, CNcb) CNcbLiter;	

class CNcbQIter;
class CNcbQueue
{
public:
	friend CNcbQIter;

	size_t Count() const { return m_list.Count(); }
	void Append(CNcb* ncb) 
	{
		VPASSERT(!ncb->IsQueued());
		ncb->SetQueued(TRUE);
		m_list.Append(ncb);
	}

	void Insert(CNcb* ncb) 
	{
		VPASSERT(!ncb->IsQueued());
		ncb->SetQueued(TRUE);
		m_list.Insert(ncb);
	}

	void Remove(CNcb* ncb) 
	{
		VPASSERT(ncb->IsQueued());
		m_list.Remove(ncb);
		ncb->SetQueued(FALSE);
	}

	CNcb* RemoveFirst()
	{
		CNcb* ncb = m_list.RemoveFirst();
		
		if (ncb)
			ncb->SetQueued(FALSE);
		return ncb;
	}
protected:
	CNcbList  m_list;
};

				 
class CNcbQIter
{

public:
	CNcbQIter(CNcbQueue& q) : m_liter(q.m_list) { }

	CNcb* operator()() { return m_liter(); }

protected:
	CNcbLiter m_liter;
};


class CNbAddress: public CNetAddress
{
public:
	virtual void GetNormalForm(LPVPNETADDRESS a)
    {
		memcpy(a->machineAddress, m_name, sizeof(m_name));
	}
	
	virtual BOOL IsEqual(CNetAddress* a)
	{
		return 0 == memcmp(m_name, ((CNbAddress*) a)->m_name, sizeof(m_name));
    }

	//+ Get Printable form of the address object (retValue == actual size of buffer needed)
	virtual DWORD GetPrintForm(LPSTR buf, DWORD bufSize)
	{
		const unsigned needed = sizeof(m_name)+1;
		if (bufSize < needed)
			return needed;
		memcpy(buf, m_name, needed-1);
		buf[needed-1] = 0;
		return needed;
	}

    CNbAddress(LPCSTR name=NULL) 
	{
		if (name)
		{
			memcpy(m_name, name, sizeof(m_name));
		}
		else
		{
			memset(m_name, ' ', sizeof(m_name));
		}
		
	}

	void SetName(LPCSTR name)
	{
		_fmemcpy(m_name, name, sizeof(m_name));
	}
	
    virtual ~CNbAddress() { }

public:
	char m_name[NCBNAMSZ];
	
};


	

class CNcbAlloc
{
public:
	CNcbAlloc();
	~CNcbAlloc();
	
	CNcb*  New();
	void   Free(CNcb*);
	
protected:

	CNcbList  m_freeList;
	
};

class CNbBufAlloc
{
public:

	CNbBufAlloc();
	~CNbBufAlloc();
	
	LPVOID  New(DWORD size);
	void    Free(LPVOID);
};


	
class CNbLink: public CNetLink, public CVPLink
{
public:
    enum LinkState { DG, CONNECTING, CONNECTED, CLOSING } ;
	friend CNbNet;
	
	//+ Send the data to the link
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
	virtual DWORD HasData() { return m_inQ.Count() ? m_inQ.First()->GetSize() : 0; }
					

	//+ Get the adress of the peer
	virtual  CNetAddress* GetRemoteAddress();

	
	CNbLink(LinkState st, CNbNet* net, LPVOID userData);
	virtual ~CNbLink();
	
	LPVOID UserData() { return m_userData; }

	BOOL Closing() const { return m_state == CLOSING; }	

protected:
 	 
	 CNcb*  MakeNcb(int cmdCode, LPVOID buf, DWORD size);
	 void   DropNcb(CNcb* ncb);

	 BOOL   EnqueueData(LPVOID data, DWORD size);
	 BOOL   SendWaitingData();
	 
	 void   CleanDataQueue();
	 void   CleanNcbQueue();
	 void   OnReceiveComplete(CNcb* ncb)
	 {
	 	m_inQ.Append(ncb);
	 }

	 void   SetRemoteName(LPCSTR name);

	 BOOL    NonQSend(LPVOID data, DWORD size);

	 CNbNet* nbNet();

public:
    
//	CNbNet*		m_net;
	CNcbList	m_inQ;
	DataQueue   m_outQ;
	int			m_state;
	int			m_lsn;
	int			m_pendingSends;
	CNbAddress* m_remoteName;

	
};


class CNbNet: public CNetInterface
{
public:
    enum { MAXDGRAMSIZE = 512 };
	friend CNbLink;
	
	declare(VPList, CNbLink);
	typedef VPList(CNbLink) LinkMap;

	declare2(VPListIterator, LinkMap, CNbLink);
    typedef VPListIterator(LinkMap, CNbLink) LinkIter;	

	typedef CNbLink::LinkState LinkState;
	
	 struct nameinfo
	 {
		 int    nameNumber;
		 char   nameText[NCBNAMSZ];
		 nameinfo() : nameNumber(-1) { }
		 
		 operator LPCSTR() const { return nameText; }
		 void SetNameText(LPCSTR s)
		 {
			 _fmemcpy(nameText, s, sizeof(nameText));
		 }
		 
		 void GetNameText(LPVOID buf)
		 {
			 _fmemcpy(buf, nameText, sizeof(nameText));
		 }
     };
		 

	static BOOL Supported();

	static WORD ChooseLana();
    //+ Create a garanteed delivery NetLink to the given destination
    virtual CNetLink*  MakeSafeLink(CNetAddress* addr, LPVOID userData=NULL);

	//+ Create a non-garanteed delivery NetLink
	virtual CNetLink*  MakeUnsafeLink(LPVOID userData=NULL);

	//+ Create a NetAddress object, if parameter is NULL the address
	//+ Is initialized with this interface address	
	virtual CNetAddress* MakeAddress(LPCVPNETADDRESS addr=NULL);

	//+ Create a broadcast NetAddress object
	virtual CNetAddress* MakeBroadcastAddress();

	//+ Create a NetAddress object used for server lookup
	virtual CNetAddress* MakeServerLookupAddress()
	{
		return MakeBroadcastAddress();
	}

	//+ Start listening for incoming connections and datagrams
    virtual BOOL   Listen(BOOL streamListen=TRUE, BOOL serverMode = FALSE);

	//+ Become deef to incoming data
	virtual void   BecomeDeef();

	//+ Get the address of this interface
    virtual void GetAddress(LPVPNETADDRESS addr)
	{
		m_stationName.GetNameText(addr->machineAddress);
	}

	//+ return TRUE if underlying protocol is keeping packet boundaries on safe links
	virtual BOOL KeepingBoundaries() const { return TRUE; }


	//+ Stop all networking activity
	virtual void  Cleanup();

	virtual BOOL Init();

	CNbNet(CTDLogger* log, LPCSTR groupName, LPCSTR stationName, int lana);
	~CNbNet();
	

	 virtual void OnNcbCompletion(CNcb* ncb);

	 void SetWindow(HWND w) { m_window = w; }

	 LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	 virtual void OnTimer();	

	 virtual void CheckFlow();

protected:

	 virtual CNbAddress* MakeAddressObj(LPCSTR n) 
	 {
		 return new CNbAddress(n);
	 }
	 

	 virtual BOOL SubmitNcb(CNcb*);
	 virtual BOOL SubmitReceiveCode(int command, int nameIndex);	 
	 virtual BOOL SubmitReceiveDgram(int nameIndex);
	 virtual BOOL SubmitOneListen();
	 virtual BOOL SubmitListen();
	 virtual BOOL SubmitOneReceive();
	 virtual BOOL SubmitReceives();



	 virtual void SetCompletionHook(CNcb*);	
	 

	 
	 CNcb*  NewNcb(int command)
	 {
		 CNcb* ncb = m_ncbAlloc.New();
		 if (ncb)
		 {
			 ncb->SetCommand(command|ASYNCH);
			 ncb->m_net = this;
			 ncb->m_window = m_window;
			 ncb->SetQueued(FALSE);
			 return ncb;
		 }
		 SetError(VPNET_ERR_NOMEM);
		 return FALSE;
	 }

	 void DropNcb(CNcb* ncb);
	 	 
	 void FreeNcb(CNcb* n) { m_ncbAlloc.Free(n); }

	 BOOL KnownActiveNcb(CNcb* );

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
	 
	 virtual BOOL  RegisterOneName(int command, nameinfo& name);
	 virtual BOOL  RegisterNames(BOOL serverMode);
	 virtual BOOL  ResetNetbios();


	 //
	 //	  Link Object management methods
	 //

	 virtual CNbLink* MakeLinkObj(LinkState st, LPVOID userData);

	 void 	RegisterLink(CNbLink* l) { m_allLinks.Append(l); }
	 void   UnregisterLink(CNbLink* l) 
	 {
	 	VPASSERT(IsRegistered(l));
	 	m_allLinks.Remove(l);
	 }
	 	 
	 CNbLink*  FindLink(UCHAR lsn); 
	 BOOL IsRegistered(CNbLink* l);

protected:
	 CNcbQueue  m_activeNcbs;
	 CNcbList   m_postponedQueue;
	 
	 CNcbAlloc  m_ncbAlloc;
	 CNbBufAlloc m_bufAlloc;
	 CNbLink*    m_dgLink;
	 
	 LinkMap    m_allLinks;
	 
	 nameinfo   m_groupName;
	 nameinfo   m_stationName;
	 int        m_activeListens;
	 int        m_maxActiveListens;
	 int        m_activeReceives;
	 int        m_maxActiveReceives;
	 
	 DWORD		m_lana;
	 HWND       m_window;	 
	 BOOL       m_cleaningUp;	 
};


inline	 CNbNet*   CNbLink::nbNet() { return (CNbNet*) m_net; }
inline	 void   CNbLink::DropNcb(CNcb* ncb) { nbNet()->DropNcb(ncb); }



#endif 
