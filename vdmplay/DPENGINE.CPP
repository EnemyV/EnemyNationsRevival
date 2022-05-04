#ifdef WITH_DP
#include "stdafx.h"
#include "vpparam.h"
#include "vpengine.h"
#include "vpnet.h"
#include "wnotque.h"
#include "vpint.h"

#include <dplay.h>
#include "dpnet.h"

#include "dpengine.h"

#include <rw/tvslist.h>

typedef  RWTValSlist<DPSESSIONDESC> DPSessionList;


BOOL CDpRemoteSession::EnumSessionsCallback(LPDPSESSIONDESC lpGame,
    LPVOID lpContext, LPDWORD lpdwTimeOut,
    DWORD dwFlags)
{
	DPSessionList* sList = (DPSessionList*) lpContext;

	if (lpGame)
	{
		sList->insert(*lpGame);
		return TRUE;
	}

	return FALSE;
}


BOOL CDpRemoteSession::LookForServer(LPVOID userData)
{
	DPSessionList sesList;
	DPSESSIONDESC sd;

	if (m_lookingForServer)
		return TRUE;

	m_lookingForServer = TRUE;
	dpNet()->InitSessionDesc(&sd);

	HRESULT hr = dp()->EnumSessions(&sd, 
		1500, EnumSessionsCallback, &sesList, 
		DPENUMSESSIONS_AVAILABLE);


    if (FAILED(hr))
	{
		if (hr != DPERR_NOSESSIONS)  // DP didn't find any session?
		{
			SetError(VPNET_ERR_DP, GetScode(hr));
			m_lookingForServer = FALSE;
			return FALSE;
		}
		return TRUE;
	}




	while(sesList.entries())
	{
		DPSESSIONDESC sd = sesList.removeFirst();

		GetSessionData(&sd);

	}

//	m_lookingForServer = FALSE;

	return TRUE;


}

void CDpRemoteSession::GetSessionData(LPDPSESSIONDESC sd)
{

	if (!dpNet()->AttachToSession(sd->dwSession, TRUE))	
		return;
	CNetAddress* addr = dpNet()->MakeAddressObj(dpNet()->m_serverPlayer);
	CNetLink* lnk = dpNet()->MakeUnsafeLink();
	genericMsg*  msg = new(0) genericMsg(SenumREQ, 0);

	if (!lnk->SendTo(*addr, msg->Data(), msg->Size(), 0))
	{
		Log("CDpremoteSession failed to send sEnumREQ the server");	
		SetError(VP_ERR_NET_ERROR, lnk->LastError());
		goto done;
	}
	WaitSessionData(sd->dwSession);
	
done:
	msg->Unref();
	lnk->Unref();
	addr->Unref();
	dpNet()->DetachFromSession();
}


BOOL CDpRemoteSession::OnSenumREP(sesInfoMsg* msg, CRemoteWS* ws)
{
	CNetAddress* addr = dpNet()->MakeAddressObj(dpNet()->m_selectedSession);
	addr->GetNormalForm(&msg->data.sessionId);
	addr->Unref();
	if (m_waitingSessionData)
		m_waitingSessionData = FALSE;

	return CWinRemoteSession::OnSenumREP(msg, ws);
}

void CDpRemoteSession::SetSessionId(LPCVPSESSIONID id)
{
	CWinRemoteSession::SetSessionId(id);
	if (!dpNet()->AttachToSession(*(LPDWORD) id->machineAddress))
	{
		VPTRACE(("DPENGINE:: Couldn't connect to the remote session\n"));
	}
}

struct dataTestCtx
{
	CDpRemoteSession* ses;
	DWORD startTime;
	DWORD timeout;
	BOOL  ret;
};

BOOL CDpRemoteSession::SessionDataTest(LPVOID context)
{
	dataTestCtx*  ctx = (dataTestCtx*) context;

	ctx->ret = !ctx->ses->m_waitingSessionData;
	if ((GetCurrentTime() - ctx->startTime) > ctx->timeout)
		ctx->ses->m_waitingSessionData = FALSE;

	return !ctx->ses->m_waitingSessionData;

}

BOOL CDpRemoteSession::WaitSessionData(DWORD sId)
{
	const DWORD timeout = 1000*60;
	dataTestCtx ctx;

	VPENTER(CDpRemoteSession::WaitSessionData);

	ctx.startTime = GetCurrentTime();
	ctx.ses = this;
	ctx.timeout = timeout;
	m_waitingSessionData = TRUE;

	BOOL ret = vpWaitLoop(SessionDataTest, &ctx);

	if (ret)
		ret = ctx.ret;


	VPEXITBOOL(ret);
	return ret;

}


CDpRemoteSession::CDpRemoteSession(CTDLogger* log, 
								   CNetInterface* net, 
								   CPlayerMap* pMap, 
								   CWSMap* wsMap, 
								   DWORD maxAge, 
								   CWinNotifyQueue* queue) :
CWinRemoteSession(log, net, pMap, wsMap, maxAge, queue)
{
	m_waitingSessionData = FALSE;
	m_lookingForServer = FALSE;
}



CWinRemoteSession* MakeDpRemoteSession(CTDLogger* log, CNetInterface* net, CPlayerMap* pMap, CWSMap* wsMap, DWORD maxAge, 
					  CWinNotifyQueue* queue)
{
	return new CDpRemoteSession(log, net, pMap, wsMap, maxAge, queue);
}

#endif