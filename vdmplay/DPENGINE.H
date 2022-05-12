#ifndef __DPENGINE_H__
#define __DPENGINE_H__

class CDPNet;

class CDpRemoteSession: public CWinRemoteSession
{

protected:


	CDPNet* dpNet() { return (CDPNet*) m_net; }
	IDirectPlay* dp() { return dpNet()->m_dp; }

	static BOOL PASCAL EnumSessionsCallback(LPDPSESSIONDESC lpGame,
				LPVOID lpContext, LPDWORD lpdwTimeOut,
				DWORD dwFlags);

    virtual BOOL OnSenumREP(sesInfoMsg* msg, CRemoteWS* ws);

	void GetSessionData(LPDPSESSIONDESC);
	BOOL WaitSessionData(DWORD sid);
	static BOOL SessionDataTest(LPVOID context);


public:

	virtual BOOL LookForServer(LPVOID userData);
	virtual void SetSessionId(LPCVPSESSIONID id);
	CDpRemoteSession(CTDLogger* log, CNetInterface* net, CPlayerMap* pMap, CWSMap* wsMap, DWORD maxAge, 
					  CWinNotifyQueue* queue);

protected:
	BOOL m_waitingSessionData;
	BOOL m_lookingForServer;
};

CWinRemoteSession* MakeDpRemoteSession(CTDLogger* log, CNetInterface* net, CPlayerMap* pMap, CWSMap* wsMap, DWORD maxAge, 
					  CWinNotifyQueue* queue);

#endif




