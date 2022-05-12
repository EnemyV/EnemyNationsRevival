#ifndef __VPINT_H__
#define __VPINT_H__

class CVdmPlay;

#ifndef WIN32
// Notifocation queue used tp end notifications to the Win32 client calling us through UT
// The original notification messages are copied to the structures allocated from GlobalFixed pool
// The message address and containing pointers are translated to the FLAT form before posting them
// into the message queue.  The copy structure contains also pointer to the original notification
// so it can be recovered by vpAcknowledge routine
class CUtNotifyQueue: public CWinNotifyQueue
{

public:
   struct UTMSG
    {
    	VPMESSAGE  msg;
    	union
    	{
    		struct UTMSG* next;
    		LPVPMESSAGE original;
    	} u;
    };
    
    typedef UTMSG FAR* LPUTMSG;
    
 	enum { POOLSIZE = 128 };
	
	virtual CNotification* RecoverNotification(LPCVPMESSAGE msg)
	{
		LPUTMSG utMsg = (LPUTMSG) msg;
		LPVPMESSAGE oMsg = utMsg->u.original;
        
        if (oMsg->u.data)
        {
        	GlobalUnfix((HGLOBAL) SELECTOROF(oMsg->u.data));
        }
        // put this structure to the free list
		utMsg->u.next = m_freeList;
		m_freeList = utMsg;
				
		return CNotification::ContainingObject(oMsg);
	}
	
	virtual BOOL  PostMessage(WPARAM wParam, LPARAM lParam);

	CUtNotifyQueue(UINT msg, HWND wnd = NULL) : CWinNotifyQueue(msg, wnd)
	{ 
	    
	    
		m_hBuff = GlobalAlloc(GPTR, POOLSIZE*sizeof(UTMSG));
		GlobalFix(m_hBuff);
		m_freeList = (LPUTMSG) GlobalLock(m_hBuff);
		
		LPUTMSG p = m_freeList;
		for(int i = POOLSIZE-1; i--;  p = p+1)
			p->u.next = p+1;	
		
		p->u.next = NULL;
		
	}
	BOOL Ok() const { return m_hBuff != NULL; }
	
	~CUtNotifyQueue()
	{
		GlobalUnlock(m_hBuff);
		GlobalUnfix(m_hBuff);
		GlobalFree(m_hBuff);
			
		
	}
  	
public:
	HGLOBAL  m_hBuff;
	LPUTMSG  m_freeList;
};
		
#endif


class CWinSession
{
public:

	void SetClientWindow(HWND w) { m_notifyQueue->m_window = w; }
	
	CWinSession(CWinNotifyQueue*  queue) : 
	 m_notifyQueue(queue), m_vdmPlay(NULL), m_sesObj(NULL) 
	 {
	 }

	virtual ~CWinSession();


	CWinNotifyQueue*  m_notifyQueue;
	CVdmPlay*        m_vdmPlay;
	CSession*        m_sesObj;
};



class CWinRemoteSession: public CRemoteSession, public CWinSession
{
public:



    void OnUnexpectedMsg(genericMsg* msg, CNetLink* link, BOOL isSafe)
	{
		VPTRACE(("CWinRemoteSession:: OnUnexpectedMessage %d", msg->hdr.msgKind));
	}
	

    void ShouldNotBeHere(LPCSTR aClass, LPCSTR aMethod, LPCSTR txt)
	{
		VPTRACE(("Should not be here: %s %s %s\n", aClass, aMethod, txt));
		VPASSERT( 0 == 1);
	}		

    virtual BOOL GoodBroadcastOptions(DWORD flags)
	{
		return TRUE;
    }
	
						
	void PostNotification(CNotification* n)
	{
		m_notifyQueue->PostNotification(n);
	}
	
	CWinRemoteSession(CTDLogger* log, CNetInterface* net, CPlayerMap* pMap, CWSMap* wsMap, DWORD maxAge, 
					  CWinNotifyQueue* queue) :
		CRemoteSession(log, net, pMap, wsMap, maxAge), CWinSession(queue)
	{
		m_sesObj = this;
	}
														     
	
	~CWinRemoteSession() { }

	

};


class CWinRegSession: public CRegisterySession, public CWinSession
{
public:



    void OnUnexpectedMsg(genericMsg* msg, CNetLink* link, BOOL isSafe)
	{
	}
	

    void ShouldNotBeHere(LPCSTR aClass, LPCSTR aMethod, LPCSTR txt)
	{
		VPASSERT(0 == 1);
	}		

    virtual BOOL GoodBroadcastOptions(DWORD flags)
	{
		return TRUE;
    }
	
						
	void PostNotification(CNotification* n)
	{
		m_notifyQueue->PostNotification(n);
	}
	
	CWinRegSession(CTDLogger* log, CNetInterface* net, CPlayerMap* pMap, CWSMap* wsMap, DWORD maxAge, 
					  CWinNotifyQueue* queue) :
		CRegisterySession(log, net, pMap, wsMap, maxAge), CWinSession(queue)
	{
		m_sesObj = this;
	}

	
	~CWinRegSession() { }


};


class CWinLocalSession: public CLocalSession, public CWinSession
{
public:

	void SetClientWindow(HWND w) { m_notifyQueue->m_window = w; }

    void OnUnexpectedMsg(genericMsg* msg, CNetLink* link, BOOL isSafe)
	{
		
	}
	

    void ShouldNotBeHere(LPCSTR aClass, LPCSTR aMethod, LPCSTR txt)
	{
		VPASSERT(0 == 1);
	}

    virtual BOOL GoodBroadcastOptions(DWORD flags)
	{
		return TRUE;
    }
	
						
	void PostNotification(CNotification* n)
	{
		m_notifyQueue->PostNotification(n);
	}
	
	CWinLocalSession(CTDLogger* log, CNetInterface* net, CPlayerMap* pMap, CWSMap* wsMap, 
					  CWinNotifyQueue* queue ) :
		CLocalSession(log, net, pMap, wsMap), CWinSession(queue)
	{
		m_sesObj = this;
	}

	~CWinLocalSession() { }


};









class CSessionEnum
{
	public:

	void SetWindow(HWND hWnd) { wses->SetClientWindow(hWnd); };
	
	void Begin()
	{
		if (rses)
		{
			rses->LookForServer(userData);
			lastRequestTime = GetCurrentTime();
		}
	}
		
	void Stop()  
	{
		if (rses)
			rses->CloseSession(); 
	}
	
	void SetGuid(LPCVPGUID) { }
	void SetUserData(LPVOID d) { userData = d; }
	
	void OnTimer()
	{
		if (!rses)
			return;

		DWORD ct = GetCurrentTime();

		if ((ct - lastRequestTime) > period)
		{
			rses->LookForServer(userData);
			lastRequestTime = ct;
		}
   }

	void OnSessionDeath(CSession* s)
	{
		if (rses == s)
		{
			wses = NULL;
			rses = NULL;
		}
	}



   CSessionEnum(CWinSession* ws, CRemoteSession* rs, DWORD p = 5000) : 
   		wses(ws), rses(rs), period(p), lastRequestTime(0), userData(0) { }
   ~CSessionEnum()
   { 
   		if (rses) 
		{
			delete rses;
   		}
	}
public:
	DWORD  lastRequestTime;
	CWinSession* wses;
	CRemoteSession* rses;
	LPVOID userData;
	DWORD period;

};

class CPlayerEnum
{
	public:

	void SetWindow(HWND hWnd) { }
	void Begin() { }
	void Stop()  { }
	void SetSessionId(LPCVPSESSIONID) { }
	void SetUserData(LPVOID ) { }
	void OnTimer() { }

	void OnSessionDeath(CSession* s) { }


};



class CVdmPlay
{
public:

	virtual BOOL Ok() const { return m_window != NULL && NULL != m_net; }

	BOOL GotFatalError() const { return m_fatalError; }
	virtual BOOL Startup(IN DWORD version, 
                   IN LPCVPGUID guid, 
				   IN DWORD sessionDataSize,
				   IN DWORD playerDataSize,
				   IN UINT protocol, LPCVOID protocolData);

	virtual void Cleanup();


	virtual BOOL EnumSessions(IN HWND hWnd,
					IN BOOL dontAutoStop, // if FALSE will AutoStop enum
					IN LPVOID userData);


	virtual BOOL StartRegistrationServer(IN HWND hWnd,
					IN LPVOID userData);

	virtual BOOL StopEnumSessions();


	virtual VPSESSIONHANDLE CreateSession(IN HWND hWnd, 
                                IN LPCSTR sessionName,
								IN DWORD sessionFlags,
							    IN LPVOID userData);



	virtual BOOL EnumPlayers(IN HWND hWnd, 
	               IN LPCVPSESSIONID sessionId,
				   IN LPVOID userData);

	virtual BOOL StopEnumPlayers();




	// Use it to get an info about a session that is not joined/created by you
	virtual BOOL GetSessionInfoById(IN VPHANDLE pHdl, 
	                      IN HWND hWnd, 
                          IN LPCVPSESSIONID sessionId,
						  LPVOID userData) { return FALSE; }


					  
	virtual VPSESSIONHANDLE JoinSession(IN HWND hWnd,
						      IN LPCVPSESSIONID sessionId, 
				              IN LPCSTR playerName,
							  IN DWORD  playerFlags,
							  IN LPVOID userData);



	virtual DWORD GetAddressString(IN LPCVPNETADDRESS addr,  LPSTR buf, DWORD bufSize);
	virtual BOOL  GetAddress(IN LPVPNETADDRESS addr);
	virtual BOOL  GetServerAddress(OUT LPVPNETADDRESS addr);


	virtual BOOL InitWindowsStuff();
	virtual BOOL InitNetworkProtocol(int protocol, LPCVOID protocolData);
	virtual void CleanupWindowsStuff();


	virtual CWinLocalSession*   MakeLocalSession(HWND wnd, LPCSTR sessionName, DWORD sessionFlags, LPVOID userData);
	virtual CWinRemoteSession*  MakeRemoteSession(HWND wnd, BOOL forEnum=FALSE);
	virtual CWinRegSession*  MakeRegSession(HWND wnd);
	
	virtual CPlayerEnum*  MakePlayerEnum(...) { return NULL; }
	virtual CSessionEnum* MakeSessionEnum(HWND wnd, LPVOID userData);
	virtual CSessionEnum* MakeSessionRegistery(HWND wnd, LPVOID userData);
	

	CVdmPlay() : m_sEnum(NULL), m_pEnum(NULL), m_session(NULL), m_net(NULL), 
	           m_sessionDataSize(0), m_playerDataSize(0), m_version(0), m_timer(0), m_window(0), 
	           m_queue(NULL), m_log(NULL), m_fatalError(FALSE)
	{ 
	}

	virtual ~CVdmPlay() 
	{
		DestroyWindow(m_window);
		delete m_queue;
		delete m_log;
	}
    
    virtual CWinNotifyQueue* MakeNotifyQueue(HWND wnd, UINT msg);
    
	static LRESULT APIENTRY WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnSessionDeath(CSession* s)
	{
		if (m_session == s)
		{
			m_session = NULL;
		}
		else
		{
			 if (m_sEnum)
			 	m_sEnum->OnSessionDeath(s);

			 if (m_pEnum)
			 	m_pEnum->OnSessionDeath(s);
		}
	}

	virtual BOOL InitTcp(LPCVOID);
	virtual BOOL InitComm(LPCVOID, BOOL modem );
	virtual BOOL InitNetbios(LPCVOID);
	virtual BOOL InitIpx(LPCVOID );
	virtual BOOL InitTapi(LPCVOID );
	virtual BOOL InitDP(LPCVOID );

	void RecordError(DWORD e1, DWORD e2, DWORD e3)
	{
		m_err1 = e1; m_err2= e2; m_err3 = e3;
	}


public:
		CTDLogger*    m_log;
		CSessionEnum* m_sEnum;
		CPlayerEnum*  m_pEnum;
		CSession* m_session;
		CWinNotifyQueue* m_queue;
		CNetInterface* m_net;
		VPGUID     m_guid;
		DWORD      m_sessionDataSize;
		DWORD      m_playerDataSize;
		DWORD      m_version;
		UINT       m_timer;
		HWND       m_window;
		DWORD      m_err1, m_err2, m_err3;
		BOOL       m_fatalError;
		int		   m_protocol;

};

#endif
