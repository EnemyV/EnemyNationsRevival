#include "stdafx.h"
#include "vpparam.h"
#include "vpengine.h"
#include "tdlog.h"
#include "smap.h"
#include "mmsystem.h"	
#include <time.h>

IMPLEMENT_CLASSNAME(CVpSession);
IMPLEMENT_CLASSNAME(CLocalSession);
IMPLEMENT_CLASSNAME(CRemoteSession);

static void LogV(CTDLogger* log, const char* fmt, 
				 DWORD p1=0, DWORD p2=0, DWORD p3=0, DWORD p4=0)
{
	char logBuf[128];

	if (log)
	{
		wsprintf(logBuf, fmt, p1, p2, p3, p4);
		log->Log(logBuf);
	}
}

static WSList* rwsPool = 0;

DWORD vpMsgTime()
{
	DWORD  msgTime;
#ifdef WIN32
	SYSTEMTIME st;
	GetLocalTime ( &st );
	msgTime = (st.wMinute << 16) | st.wSecond;
#else
    time_t t = time(NULL);
	struct tm tms = *localtime(&t);

	msgTime = ((DWORD) tms.tm_min << 16) | tms.tm_sec;
#endif

   return msgTime;
}

void* CRemoteWS::operator new(size_t s)
{
	char* p;

	if (!rwsPool)
	{
		p = new char[s];
	}
	else
	{
		p = (char*) rwsPool->RemoveFirst();
		if (!p)
			p = new char[s];
	}
	return p;
}

void CRemoteWS::operator delete(void* p)
{
	if (!rwsPool)
		delete [] (char*) p;

	rwsPool->Insert((CRemoteWS*)p);
}
	

void CRemoteWS::InitPool()
{
	if (!rwsPool)
		rwsPool = new WSList;
}

void CRemoteWS::CleanPool()
{
	CRemoteWS* ws;
	WSList* list = rwsPool;

	rwsPool = NULL;


	while(NULL != (ws = (CRemoteWS*) list->RemoveFirst()))
		delete ws;
}
	



CVpSession::CVpSession(CTDLogger* log, CNetInterface* net, 
                  CPlayerMap* players,
				  CWSMap* wsMap) : m_net(net),
	m_log(log), m_info(NULL), m_players(players), m_wsMap(wsMap),  
    m_localWS(NULL), m_broadcastLink(NULL), m_broadcastAddress(NULL), m_registrationAddress(NULL),
    m_error(0), m_errInfo(0), m_msgId(0), m_invalid(FALSE)
{
	CRemoteWS::InitPool();	
	m_keepLog = vpFetchInt("VDMPLAY", "KeepDataLog", 0);
}

void CVpSession::SetError(DWORD error, DWORD errInfo)
{
	m_error = error;
	m_errInfo = errInfo;

	if (m_log)
		m_log->SetError(error, errInfo);
}

void CVpSession::FatalError(DWORD error, DWORD errInfo)
{
	m_error = error;
	m_errInfo = errInfo;

	if (m_log)
		m_log->SetFatalError(error, errInfo);

	m_invalid = TRUE;
}

void CVpSession::Log(LPCSTR msg)
{
	if (m_log)
		m_log->Log(msg);
}


void CVpSession::SafeDataHook(CNetLink* link, LPVOID context)
{
	((CVpSession* )context)->OnSafeData(link);
}

void CVpSession::UnsafeDataHook(CNetLink* link, LPVOID context)
{
	((CVpSession* )context)->OnUnsafeData(link);
}

void CVpSession::ConnectHook(CNetLink* link, LPVOID context)
{
	((CVpSession* )context)->OnConnect(link);
}


void CVpSession::DisconnectHook(CNetLink* link, LPVOID context)
{
	((CVpSession* )context)->OnDisconnect(link);
}

void CVpSession::AcceptHook(CNetLink* link, LPVOID context)
{
	((CVpSession* )context)->OnAccept(link);
}



void CVpSession::InitSessionInfo(LPCVPGUID guid, 
								DWORD version, 
								DWORD sessionDataSize, 
								DWORD playerDataSize)
{
	m_info = new((size_t)sessionDataSize) sesInfoMsg((size_t)sessionDataSize);

	if (m_info)
	{
		m_info->data.gameId = *guid;
		m_info->data.version = version;
		m_info->data.dataSize = sessionDataSize;
		m_info->data.playerDataSize = playerDataSize;
	}
}


BOOL CVpSession::InitNetwork(BOOL streamListen)
{
	VPASSERT(m_net);
	VPASSERT(m_info);

	m_net->SetHooks(SafeDataHook, 
					  UnsafeDataHook, 
					  ConnectHook,
					  DisconnectHook, 
					  AcceptHook,	
					  this);
		
	if (!m_net->Listen(streamListen, IsServerMode()))
	{
		Log("CVpSession::Net->Listen failed\n");
		return FALSE;
	}			  

	m_broadcastAddress = m_net->MakeBroadcastAddress();

	if (!m_broadcastAddress)
	{
		Log("CVpSession::InitNetwork Can't Get broadcast address");
		FatalError(VP_ERR_NET_ERROR);
		return FALSE;
	}

	m_registrationAddress = m_net->MakeRegistrationAddress();

	m_broadcastLink    = m_net->MakeUnsafeLink();
	if (!m_broadcastLink)
	{
		Log("CVpSession::InitNetwork Can't Get broadcast Link");
		FatalError(VP_ERR_NET_ERROR);
		return FALSE;
	}

	m_net->GetAddress(&m_info->data.sessionId);

	return TRUE;

}

void CVpSession::HandleNetDown()
{
	VPENTER(CVpSession::HandleNetDown);

	CNotification* n = new CNotification(VP_NETDOWN, 0, 0, 0);
	if (!n)
	{
		FatalError(VP_ERR_NOMEM);
	}
	else
	{
		PostNotification(n);
		FatalError(VP_ERR_NET_DOWN);
	}
	VPEXIT();
	return;
}		

void CVpSession::OnSafeData(CNetLink* link)
{
	DWORD waitingDataCount;   
	DWORD count;            // how much data we've actually got form the net
	genericMsg*  msg;
	VPMSGHDR  hdr;
	char buf[256];


	if (m_net->KeepingBoundaries())
	{
		// Fine, the underlying protocol is doing the job for us,
		while(0 != (waitingDataCount = link->HasData()))
		{
			msg = new((size_t)waitingDataCount - sizeof(VPMSGHDR)) genericMsg;
			if (!msg)
			{
				SetError(VP_ERR_NOMEM);
				return;
			}


			count = link->Receive(msg->Data(), waitingDataCount);


			if ((count < sizeof(VPMSGHDR)) || (count < msg->Size()))  // ignore badly formatted messages
			{
				msg->Unref();
				continue;
			}
	
			ProcessSafeData(link, msg);
			msg->Unref();
		}
		return;
	}

	
	// The protocol does not keep messsage boundaries, we'll have to split reads in 2 steps
	// 1) Read the message header 
	// 2) Read the content using size from the message header

#if 0
	if (link->HasPushbackData())
	{
		size_t pbCount = link->HasPushbackData();

		if (pbCount < sizeof(hdr))
		{
			count = link->HasData();
			if ((pbCount + count) < sizeof(hdr))
				return;


		}

		count = link->ReadPushback(&hdr, sizeof(hdr));

	}
#endif

	if (0 != (waitingDataCount = link->HasData()))
	{

		if (waitingDataCount < sizeof(hdr))
			return;

		memset(&hdr, 0, sizeof(hdr));
		count = link->Receive(&hdr, sizeof(hdr));

		if (count < sizeof(hdr))
		{

			wsprintf(buf, "CVpSession::OnSafeData: Error reading message header for the link %ld", count);
			Log(buf);
			return;
		}

		if (hdr.msgSize < sizeof(hdr))
		{
			Log("CVpSession::OnSafeData - too small message size in the message header");
			OnDisconnect(link);
			return;
		}

		
		DWORD msgDataSize = hdr.msgSize - sizeof(hdr);
		
		if (hdr.msgSize	> VP_MAXSENDDATA)
		{
			Log("CVpSession::OnSafeData - message too big");
			OnDisconnect(link);
			return;
 		}

		msg = new((size_t)msgDataSize) genericMsg;
		if (!msg)
		{
			SetError(VP_ERR_NOMEM);
			return;
		}

		msg->hdr = hdr;

		count = link->Receive(msg->Contents(), msgDataSize);

		if (count == msgDataSize)	   // Just to be sure that all is OK
			ProcessSafeData(link, msg);

		msg->Unref();

	}

}



BOOL CVpSession::ExceptEnumHelper(CWS* ws, LPVOID data)
{
	exceptEnumInfo& info = *(exceptEnumInfo*) data;
	
	return (ws == info.exceptWs) ? TRUE : info.p(ws, info.data);
}


void CVpSession::ForAllWorkstationsExcept(CWS* ws, WSEnum p, LPVOID data)
{
	exceptEnumInfo info;
	
	info.exceptWs = ws;
	info.p = p;
	info.data = data;
	
	ForAllWorkstations(ExceptEnumHelper, &info);
}


BOOL CVpSession::SendOnLink(CNetLink* link, LPVOID data)
{
	VPASSERT(link);
	VPASSERT(data);
	
	sendDataInfo& sdi =  *(sendDataInfo*) data;
	
	link->Send(sdi.m_data, sdi.m_dataSize, sdi.m_sendFlags);
	return TRUE;
}

WORD CVpSession::PlayerCount() 
{
	return (WORD) m_players->Count();
}


BOOL CVpSession::GetSessionInfo(LPVPSESSIONINFO pInfo)
{
	VPASSERT(m_info);

	*pInfo = m_info->data;
	return TRUE;
}


CRemoteWS* CVpSession::MakeRemoteWS(CNetAddress* a, CNetLink* safeLink, CNetLink* unsafeLink)
{
	CRemoteWS* ws;
	if (!unsafeLink)
	{
		CNetLink* tmp = m_net->MakeUnsafeLink();
		ws = new CRemoteWS(a, safeLink, tmp);
		tmp->Unref();
	}
	else
	{
		ws = new CRemoteWS(a, safeLink, unsafeLink);
	}
	
	if (!ws)
	{
		SetError(VP_ERR_NOMEM);
	}
	return ws;
}



CPlayer* CVpSession::FindPlayer(VPPLAYERID pId, CWS* ws) const
{
	CPlayer* p = m_players->PlayerAtId(pId);
	
	if (!p || ((ws != NULL) && (ws != p->m_ws)))
		return NULL;
	
	return p;
}


CRemotePlayer* CVpSession::RemoveRemotePlayer(VPPLAYERID pId, CRemoteWS* ws)
{

	VPTRACE(("CVpSession::RemoveRemotePlayer id=%d\n", pId));
	
	CPlayer* p = FindPlayer(pId, ws);
	
	if (!p)
	{
		SetError(VP_ERR_BAD_PLAYER_ID, pId);
		return NULL;
	}
	
	if (p->IsLocal())
	{
		SetError(VP_ERR_LOCAL_PLAYER, pId);
		return FALSE;
	}
		
	
	CLeaveNotification* n = new CLeaveNotification(p);
	m_players->RemovePlayer(p->PlayerId());
	
	if (!n)
	{
		Log("CVpSession::RemoveRemotePlayer - No memory for notification");
		SetError(VP_ERR_NOMEM);
		return (CRemotePlayer*) p;
	}
	

	VPTRACE(("CVpSession::RemoveRemotePlayer Posting notification\n"));
	
	PostNotification(n);
	
	return (CRemotePlayer*) (p);
}


BOOL CVpSession::KnockOutPlayer(VPPLAYERID id, plrInfoMsg* msg,  CRemoteWS* ws)
{ 
	CRemotePlayer* p = RemoveRemotePlayer(id, ws);
	
	if (!p)
		return FALSE;
	
	
	// Now we're going to re-send a notification message to all Workstattions

	if (!msg)
		msg = p->m_info;

	msg->hdr.msgTo = VP_LOCALMACHINE;
	msg->hdr.msgFrom = VP_SESSIONSERVER;
	msg->hdr.msgId = NextMessageId();
	msg->hdr.msgKind = LeaveREQ;
	
	
	sendDataInfo info(msg->Data(), msg->Size(), VP_MUSTDELIVER,NULL,this);
	ForAllWorkstationsExcept(p->m_ws, SendToWs, &info);	
	
	p->m_ws->RemovePlayer();
	if (p->m_ws->PlayerCount() == 0)
		m_wsMap->Deregister(p->m_ws);
	
	return TRUE;

}

BOOL CVpSession::KillPlayer(VPPLAYERID id)
{
	LogV(m_log, "CVpSession::KillPlayer(%s) %u",
		(DWORD) (IsLocal() ? "Local" : "Remote"), id);

	if (!IsLocal())
	{
		SetError(VP_ERR_REMOTE_SESSION);
		return FALSE;
	}

	return KnockOutPlayer(id, NULL, NULL);
}



	
	

CRemotePlayer* CVpSession::AddRemotePlayer(plrInfoMsg* pInfoMsg, CRemoteWS* ws, BOOL doNotify)
{

	CRemotePlayer* player = new CRemotePlayer(pInfoMsg, ws);
	
	if (!player)
	{
		Log("CVpSession::AddRemotePlayer - can't add remote player");
		FatalError(VP_ERR_NOMEM);
		return NULL;
    }
	
	m_players->AddPlayer(player);
	ws->AddPlayer();

	if (ws->PlayerCount() == 1)
	{
		// there is a chance that the station address is incomplete
		// fix this situation with the data from the player info
		CNetAddress* addr = m_net->MakeAddress(&pInfoMsg->data.playerAddress);
		ws->SetAddress(addr);
		addr->Unref();
	}


	if (!doNotify)
		return player;

	CJoinNotification* n = 
		new CJoinNotification(player, VP_ERR_OK);
	
	if (!n)
	{
		SetError(VP_ERR_NOMEM);
		return player;
	}
	
	PostNotification(n);
	return (player);
}

CLocalPlayer* CVpSession::MakeLocalPlayer(LPCSTR playerName, 
								DWORD  playerFlags)
{
	size_t playerDataSize = 256;
	
	plrInfoMsg* pInfoMsg  = new(playerDataSize) plrInfoMsg(playerDataSize);
	m_net->GetAddress(&pInfoMsg->data.playerAddress);
	pInfoMsg->data.playerFlags = playerFlags;
	_fmemcpy(pInfoMsg->data.playerName, playerName, playerDataSize);
	
	CLocalPlayer* p = new CLocalPlayer(pInfoMsg, m_localWS);
	
	pInfoMsg->Unref();
	return p;
	
}


WORD CVpSession::NextMessageId()
{
	CSessLock lk(this);

	return ++m_msgId;
}

BOOL CVpSession::SendData(VPPLAYERID toId,
                        VPPLAYERID fromId,
                        LPVOID data, 
						DWORD dataSize, 
						DWORD flags,
						LPVOID userData)
{
	sendDataInfo info(data, dataSize, flags & ~VP_BROADCAST, userData, this);
	LPVPMSGHDR pHdr = (LPVPMSGHDR) data;
	
	pHdr->msgTo = toId;
	pHdr->msgFrom = fromId;
	pHdr->msgKind = UDataREQ;
	pHdr->msgId = NextMessageId();
	pHdr->msgSize = (WORD) dataSize;
	pHdr->msgFlags = (BYTE) flags;
#if VP_TIMESTAMP
	pHdr->msgTime = vpMsgTime();
#endif

	if (!(flags & VP_BROADCAST) && (toId != VP_ALLPLAYERS))
	{
		CPlayer* player = m_players->PlayerAtId(toId);
		
		if (!player)
		{
			SetError(VP_ERR_BAD_PLAYER_ID, toId);
			return FALSE;
	    }
				

		return player->SendData(info);
	}
	
	if (!GoodBroadcastOptions(flags))
		return FALSE;
	
	// we have a broadcast here
	pHdr->msgKind = UBDataREQ;

#if 0	
	if (!(flags & VP_MUSTDELIVER))
	{
		// we simply send a broadcast message
		if (!m_broadcastLink->SendTo(*m_broadcastAddress,
	    	                     data, dataSize, VP_BROADCAST))
		{
			Log("Session::SendData failed to send to broadcast address");	
			SetError(VP_ERR_NET_ERROR, m_broadcastLink->LastError());	
			return FALSE;
		}
		return TRUE;
	}
#endif
	ForAllWorkstations(SendAllPlayers, &info);
	return TRUE;
}




	
	
// Send the message to all palyers on the given WS
BOOL CVpSession::SendAllPlayers(CWS* ws, LPVOID data)
{
	VPASSERT(ws);
	VPASSERT(data);
	
	sendDataInfo& sdi =  *(sendDataInfo*) data;
	LPVPMSGHDR pHdr = (LPVPMSGHDR) sdi.m_data;
	
	pHdr->msgTo = VP_ALLPLAYERS;
	
	ws->SendData(sdi);
	return TRUE;
}


BOOL CVpSession::SendToWs(CWS* ws, LPVOID data)
{
	VPASSERT(ws);
	VPASSERT(data);
	
	sendDataInfo& sdi =  *(sendDataInfo*) data;
	
	ws->SendData(sdi);
	return TRUE;
}


BOOL CVpSession::OnUDataREQ(genericMsg* msg, CRemoteWS* ws)
{
	CDataNotification* n = new CDataNotification(msg);
	
	if (!n)
	{
		SetError(VP_ERR_NOMEM);
		return FALSE;
	}
	
	PostNotification(n);
	return TRUE;
}

BOOL CVpSession::OnUBDataREQ(genericMsg* msg, CRemoteWS* ws)
{
	return OnUDataREQ(msg, ws);
}


BOOL CVpSession::OnFtREQ(ftReqMsg* msg, CRemoteWS* ws)
{
	CFtNotification* n = new CFtNotification(msg, NULL);
	
	if (!n)
	{
		SetError(VP_ERR_NOMEM);
		return FALSE;
	}
	
	PostNotification(n);
	return TRUE;
}


BOOL CVpSession::OnFtACK(ftAckMsg* msg, CRemoteWS* ws)
{
	VPTRACE(("Got FTACK\n"));
    if (ws->FtState() == VPFTINFO::FTREQSENT)
	{
		ws->SetFtState(VPFTINFO::FTACKRECVD);
	}


	return TRUE;
}

BOOL CVpSession::StartFT(LPVPFTINFO ftInfo)
{
#if 0
	CPlayer* player = m_players->PlayerAtId(ftInfo->ftToId);

	if (!player)
	{
		ftInfo->ftErr = VP_ERR_BAD_PLAYER_ID;
		return FALSE;
	}

	ftReqMsg* msg = new(0) ftReqMsg;


	if (!msg)
	{
		ftInfo->ftErr = VP_ERR_NOMEM;
		return FALSE;
	}

	ftInfo->ftXferCount = 0;
	ftInfo->ftState = VPFTINFO::FTREQSENT;
	ftInfo->ftErr = 0;
	ftInfo->ftTpData = 0;
	ftInfo->ftContext = 0;
	ftInfo->ftStatus2 = 0;

	msg->data = *ftInfo;

	sendDataInfo info(msg->Data(), msg->Size(), VP_MUSTDELIVER,NULL, NULL);
	player->SendData(info);

	// wait for FT ACK



	return TRUE;
#else
	return FALSE;
#endif

}

BOOL CVpSession::StopFT(LPVPFTINFO ftInfo)
{
   return FALSE;
}
    
// Accept incoming file transfer request
BOOL CVpSession::AcceptFT(LPVPFTINFO ftInfo)
{
	return FALSE;
}
	 

// Send a file  data block
BOOL  CVpSession::SendBlock(LPVPFTINFO ftInfo,  // file transfer info
					   LPCVOID buf, 
					   DWORD bufSize
					   )
{
	return FALSE;
}


// Receive	a file data block
BOOL CVpSession::GetBlock(LPVPFTINFO ftInfo, 	  // file transfer info
					   LPVOID buf, 
					   DWORD bufSize
					   )
{
	return FALSE;
}


int DeletePlayer(CPlayer* p, LPVOID data)
{
	delete p;
	return TRUE;
}

int DeleteWs(CWS* ws, LPVOID data)
{
	delete ws;
	return TRUE;
}


BOOL CVpSession::CloseSession()
{
	
	delete m_players;
	m_players = NULL;

	delete m_wsMap;
	m_wsMap = NULL;

//	ForAllWorkstations(DeleteWs, NULL);

	if (m_info)
	{
		m_info->Unref();
		m_info = NULL;
	}

	if (m_broadcastLink)
	{
		m_broadcastLink->Unref();
		m_broadcastLink = NULL;
	}
	
	if (m_broadcastAddress)
	{
		m_broadcastAddress->Unref();
		m_broadcastAddress = NULL;
	}

	if (m_registrationAddress)
	{
		m_registrationAddress->Unref();
		m_registrationAddress = NULL;
	}

	if (m_net)
	{
		m_net->BecomeDeef();
		m_net->Cleanup();
	}


	return (TRUE);
}





CVpSession::~CVpSession()
{
	CloseSession();
	CRemoteWS::CleanPool();
}



CLocalSession::CLocalSession(CTDLogger* log,
							CNetInterface* net, 
							CPlayerMap* players, 
						    CWSMap* wsMap) : 
   CVpSession(log, net, players, wsMap), m_nextPlayerId(VP_FIRSTPLAYER), m_visible(TRUE), 
	   m_lastRegTime(0), m_gotsEnumREQ(FALSE)
{
}

//+ Connection establishemwent succeded
void CLocalSession::OnConnect(CNetLink* link)
{
	// Handle eventual connection failure
	DWORD err = link->LastError();

	if (err)
	{
		OnDisconnect(link);
	}
	
}



//+ New connection request arrived
void CLocalSession::OnAccept(CNetLink* link)
{
	CNetAddress* addr = link->GetRemoteAddress();
	CWS* ws = m_wsMap->FindByAddress(addr);
	
	if (ws != NULL)
	{
		// We have already a WS at this NetAddress
		((CRemoteWS*) ws)->SetSafeLink(link);
	}
	else
	{
		ws = MakeRemoteWS(addr, link, NULL);
		if (!ws)
		{
			addr->Unref();
			return;
		}	
			
		m_wsMap->Register(ws);
	}

	// Reply with session info
	m_info->Ref();
		
	sendDataInfo sdi(m_info->Data(), m_info->Size(), VP_MUSTDELIVER,
						NULL, this);
	
	ws->SendData(sdi);				
	
	m_info->Unref();
	addr->Unref();
	
}

struct leaveInfo
{
	CWS*  m_ws;
	CLocalSession* m_ses;
};

BOOL SimulateLeave(CPlayer* p, LPVOID ctx)
{
	VPASSERT(ctx);
	leaveInfo& info = *(leaveInfo*) ctx;
	
	if (p->m_ws == info.m_ws)
	{
		p->m_info->hdr.msgKind = LeaveREQ;
		info.m_ses->OnLeaveREQ(p->m_info, (CRemoteWS*) info.m_ws);
	}
	return TRUE;
}



void CLocalSession::OnDisconnect(CNetLink* link)
{
	LogV(m_log, "ClocalSession::OnDisconnect for link %08lx\n", 
		(DWORD) link);

	if (m_broadcastLink == link)
	{
		HandleNetDown();
		return;
	}
	
	CWS* ws = m_wsMap->FindBySafeLink(link);

	if (!ws)
		return;	

	VPASSERT(ws->IsRemote());
	
	ws->Ref();   // Make sure the object will not go away
	
	((CRemoteWS*)ws)->StopUsingSafeLink();

	if (ws->PlayerCount())
	{
		leaveInfo info = { ws, this };
		
		m_players->Enum(SimulateLeave, &info); 
	}
	else
	{
		m_wsMap->Deregister(ws);
	}
	
	ws->Unref();
	

}

void CLocalSession::ProcessSafeData(CNetLink* link, genericMsg* msg)
{
	BOOL unexpected = FALSE;
	CRemoteWS* ws = (CRemoteWS*) m_wsMap->FindBySafeLink(link);
	
	VPASSERT(ws);
	
	
	switch(msg->hdr.msgKind)
	{
	   default:
		   unexpected = TRUE;
		   break;

	   case SenumREQ:
	  	 	OnSenumREQ(msg, ws);
			break;
			
		  
	   case JoinREQ:
		   OnJoinREQ((plrInfoMsg*) msg, ws);
		   break;
		   
	   case LeaveREQ:
		   OnLeaveREQ((plrInfoMsg*) msg, ws);
		   break;
		   

       case  PenumREQ:
		   OnPenumREQ(msg, ws);
		   break;

	   case  UDataREQ:
		   OnUDataREQ(msg, ws);
		   break;
		   
	   case  UBDataREQ:
		   OnUBDataREQ(msg, ws);
		   break;

	   case DummyREQ:
		   break;
	   }
	   
	   
	if (unexpected)
		OnUnexpectedMsg(msg, link, TRUE);
	
}

//+ Process data coming from unsafe link
void CLocalSession::OnUnsafeData(CNetLink* link)
{
	genericMsg* msg = new((size_t)VP_MAXSENDDATA) genericMsg;
	const DWORD maxReadSize = VP_MAXSENDDATA+sizeof(VPMSGHDR);
	CNetAddress* addr = m_net->MakeAddress();
	DWORD count = link->ReceiveFrom(msg->Data(), maxReadSize, *addr);
	BOOL unexpected = FALSE;
	CWS* ws;
	
	if (!count)
	{
		msg->Unref();
		addr->Unref();
		return;
	}
	
	switch(msg->hdr.msgKind)
	{
		case  SenumREQ:
			ws = MakeRemoteWS(addr, NULL, link);
			if (!ws)
				break;
			OnSenumREQ(msg, (CRemoteWS*) ws);
			ws->Unref();
			break;
			
		case  UDataREQ:
		case  UBDataREQ: 	
           // we're going to ignore messages from unknown workstations
		   ws = m_wsMap->FindByAddress(addr); 
		   if (!ws)	break;
		   
	       // This is a message from known workstation
		   if (msg->hdr.msgKind == UDataREQ)
		   {
			   OnUDataREQ(msg, (CRemoteWS*)ws);
		   }
		   else
		   {
			   OnUBDataREQ(msg, (CRemoteWS*)ws);
		   }

		   break;
	
		case DummyREQ:
			break;

	   default:
		   unexpected = TRUE;
		   break;
	   }
	   
	   
	if (unexpected)
		OnUnexpectedMsg(msg, link, FALSE);
	
	msg->Unref();
	addr->Unref();
}






VPPLAYERID CLocalSession::NewPlayerId()
{
	return m_nextPlayerId++;
}


BOOL CLocalSession::SetVisibility(BOOL v)
{
	m_visible = v;
	return TRUE;
}


CRemotePlayer* CLocalSession::AddRemotePlayer(plrInfoMsg* pInfo, 
 CRemoteWS* ws)
{
	pInfo->data.playerId = NewPlayerId();
	return CVpSession::AddRemotePlayer(pInfo, ws, TRUE);
}

BOOL CLocalSession::AddLocalPlayer(LPCSTR playerName, 
								DWORD  playerFlags,
								LPVOID userData, LPVPPLAYERID pId)
{
	// create te player objet and store it in the player map
	CLocalPlayer* p = MakeLocalPlayer(playerName, playerFlags);

	plrInfoMsg* pInfoMsg = p->m_info;
	
	pInfoMsg->data.playerId = NewPlayerId();
	
	if (pId)
		*pId = pInfoMsg->data.playerId;


	m_players->AddPlayer(p);
	
	
	// notify all other worksations about this player
	pInfoMsg->hdr.msgId = NextMessageId();
	pInfoMsg->hdr.msgKind = JoinADV;
	
	sendDataInfo info(pInfoMsg->Data(), pInfoMsg->Size(), VP_MUSTDELIVER,
						NULL, this);
	ForAllWorkstations(SendToWs, &info);
	
//	CLocalJoin* n = new CLocalJoin(p, pInfoMsg->hdr.msgId, userData);
//	PostNotification(n);
	
	return TRUE;
}



BOOL CLocalSession::OnLeaveREQ(plrInfoMsg* msg, CRemoteWS* ws)
{

#if 1
	LogV(m_log, "CLocalSession:: OnLeaveREQ Player %u", msg->data.playerId);
	
	return KnockOutPlayer(msg->data.playerId, msg, ws);
#else

	CRemotePlayer* p = RemoveRemotePlayer(msg->data.playerId, ws);
	
	if (!p)
		return FALSE;
	
	
	// Now we're going to re-send a notification message to all Workstattions

	
	msg->hdr.msgTo = VP_LOCALMACHINE;
	msg->hdr.msgFrom = VP_SESSIONSERVER;
	msg->hdr.msgId = NextMessageId();
	
	
	sendDataInfo info(msg->Data(), msg->Size(), VP_MUSTDELIVER,NULL,this);
	ForAllWorkstationsExcept(p->m_ws, SendToWs, &info);	
	
	p->m_ws->RemovePlayer();
	if (p->m_ws->PlayerCount() == 0)
		m_wsMap->Deregister(p->m_ws);
	
	return TRUE;
#endif

}


BOOL CLocalSession::OnJoinREQ(plrInfoMsg* msg, CRemoteWS* ws)
{

	CRemotePlayer* p = AddRemotePlayer(msg, ws);

	msg->hdr.msgTo = VP_LOCALMACHINE;
	msg->hdr.msgFrom = VP_SESSIONSERVER;

	sendDataInfo info(msg->Data(), msg->Size(),VP_MUSTDELIVER,NULL,this);

	
	if (!p)
	{
		// we must reply a NAK to the sender
		msg->hdr.msgKind = JoinNAK;
		SendToWs(ws, &info);  // reply back to the sender
		return FALSE;
   }
	
	// Now we're going to re-send a notification message to all Workstations

	msg->hdr.msgKind = JoinREP;
	msg->data.playerId = p->PlayerId();
	
	
	SendToWs(ws, &info);  // first reply back to the sender


	msg->hdr.msgKind = JoinADV;
		
	ForAllWorkstationsExcept(ws, SendToWs, &info);	
	
	return TRUE;
}

BOOL CLocalSession::OnUBDataREQ(genericMsg* msg, CRemoteWS* ws)
{
	
	OnUDataREQ(msg, ws);  // deliver this message to local destinations
		
	sendDataInfo info(msg->Data(), msg->Size(), msg->hdr.msgFlags & VP_MUSTDELIVER, NULL,this);

    ForAllWorkstationsExcept(ws, SendToWs, &info);		
	return TRUE;
	
}

BOOL CLocalSession::OnSenumREQ(genericMsg* msg, CRemoteWS* ws)
{
	if (IsVisible())
	{
		m_gotsEnumREQ = TRUE;
		m_info->hdr.msgId = msg->hdr.msgId;
		m_info->hdr.msgKind = SenumREP;
	
		sendDataInfo info(m_info->Data(), m_info->Size(), 0,NULL,this);	
		ws->SendData(info);
		if (m_registrationAddress && ws->m_address &&
			m_registrationAddress->IsEqual(ws->m_address))
		{
			
			m_lastRegTime = GetCurrentTime();
		}

	}

	if (!m_keepLog)
		m_net->StopDataLog();
	
	return TRUE;
}

struct spiContext
{
	CRemoteWS*     ws;
	WORD    totalPlayers;
	WORD    iterationIndex;
	WORD    msgId;
};

BOOL SendPlayerInfo(CPlayer* p, LPVOID data)
{
	spiContext& ctx  = *(spiContext*) data;
	plrInfoMsg*  msg = p->m_info;
	
	msg->hdr.msgId = ctx.msgId;
	msg->seq.index = ctx.iterationIndex++;
	msg->seq.total = ctx.totalPlayers;
	
	sendDataInfo info(msg->Data(), msg->Size(), VP_MUSTDELIVER, NULL, NULL);
	return ctx.ws->SendData(info);
}

	
	
	
BOOL CLocalSession::OnPenumREQ(genericMsg* msg, CRemoteWS* ws)
{
	spiContext ctx;
	
	ws->Ref();
	ctx.ws = ws;  
	ctx.totalPlayers = PlayerCount();
	ctx.iterationIndex = 1;
	ctx.msgId = msg->hdr.msgId;
	
	m_players->Enum(SendPlayerInfo, &ctx);
	ws->Unref();
	
	return TRUE;
}

	

BOOL CLocalSession::UpdateSessionInfo(LPVOID data)
{
	 CSessLock lk(this);
	 _fmemcpy(m_info->data.sessionName, data, (size_t)m_info->data.dataSize);
	 return BroadcastSessionData();
}


BOOL CLocalSession::BroadcastSessionData()
{
	VPASSERT(m_broadcastLink);
	
	
	if (IsVisible())
	{
		// First we send the broadcast message
		if (!m_broadcastLink->SendTo(*m_broadcastAddress,
	    	                     m_info->Data(), m_info->Size(), VP_BROADCAST))
		{
			Log("CLocalSession failed to send broadcast address");	
			SetError(VP_ERR_NET_ERROR, m_broadcastLink->LastError());	
			return FALSE;
		}

		if (m_registrationAddress)
		{
			m_broadcastLink->SendTo(*m_registrationAddress,
	    	                     m_info->Data(), m_info->Size(), 0);
			m_lastRegTime = GetCurrentTime();
	    }	
	}
	
	
	// Now we're sending the data to all workstations
	sendDataInfo info(m_info->Data(), m_info->Size(), VP_MUSTDELIVER,NULL,this);
	ForAllWorkstations(SendToWs, &info);	
	return m_error == VP_ERR_OK;
}


void CLocalSession::OnTimer()
{
	if (m_visible && m_registrationAddress && !m_gotsEnumREQ)
	{
		DWORD t = GetCurrentTime();

		if (t - m_lastRegTime > 2000)	
		{
			m_broadcastLink->SendTo(*m_registrationAddress,
	    	                     m_info->Data(), m_info->Size(), 0);
			m_lastRegTime = GetCurrentTime();
	    }	
	}
}



CRemoteSession::CRemoteSession(CTDLogger* log, 
							CNetInterface* net, 
							CPlayerMap* players, 
						    CWSMap* wsMap, DWORD maxAge) : 
   CVpSession(log, net, players, wsMap), m_serverWS(NULL), m_pendingJoin(NULL), 
   m_initialJoin(TRUE), m_serverEnumData(NULL), m_maxServerAge(maxAge), m_connected(FALSE)
{
}


BOOL CRemoteSession::LookForServer(LPVOID data)
{
	BOOL ret = TRUE;
	VPASSERT(m_broadcastLink);
	VPASSERT(m_broadcastAddress);

	if (m_connected)
		return TRUE;

	m_serverEnumData = data;
	genericMsg*  msg = new(0) genericMsg(SenumREQ, 0);
	CNetAddress*  lookupA;

	if (!m_net->IsSlowNet())
	{
		// First we send point to pint message to the rehistery server
		lookupA = m_net->MakeServerLookupAddress();

		if (!lookupA)
			return FALSE;

		if (!m_broadcastLink->SendTo(*lookupA,
	                         msg->Data(), msg->Size(), 0))
		{
			Log("CremoteSession failed to send to lookup address");	
			SetError(VP_ERR_NET_ERROR, m_broadcastLink->LastError());	
			ret = FALSE;
		}
	
		lookupA->Unref();
	}
	
	// Now we send broadcast broadcast message
	lookupA = m_net->MakeBroadcastAddress();
	if (!m_broadcastLink->SendTo(*lookupA,
	                         msg->Data(), msg->Size(), 0))
	{
		Log("CremoteSession failed to send to brodcast address");	
		SetError(VP_ERR_NET_ERROR, m_broadcastLink->LastError());	
		ret = FALSE;
	}

	lookupA->Unref();
	msg->Unref();
	AgeServerList();

	return TRUE;
}
	




BOOL CRemoteSession::SendData(VPPLAYERID toId,
                        VPPLAYERID fromId,
                        LPVOID data, 
						DWORD dataSize, 
						DWORD flags,
						LPVOID userData)
{
	if (!m_connected)
	{
		Log("CRemoteSession::SendData when disconnetced");
		VPTRACE(("CremoteSession::SendData when disconnected"));
//		return FALSE;
	}
	// Use standard method for sending UNICAST data
	if (!(flags & VP_BROADCAST) && (toId != VP_SESSIONSERVER) && (toId != VP_ALLPLAYERS))
	{
		return CVpSession::SendData(toId, fromId, data, 
								  dataSize, flags, userData);
    }

	// Use server to send BROADCAST data
	sendDataInfo info(data, dataSize, flags, userData, this);
	
	LPVPMSGHDR pHdr = (LPVPMSGHDR) data;
	if (toId != VP_SESSIONSERVER)
		pHdr->msgTo = VP_ALLPLAYERS;

	pHdr->msgFrom = fromId;
	pHdr->msgKind = UBDataREQ;
	pHdr->msgId = NextMessageId();
	pHdr->msgSize = (WORD) dataSize;
#if VP_TIMESTAMP
	pHdr->msgTime = vpMsgTime();
#endif
	if (toId == VP_SESSIONSERVER)
	{
		pHdr->msgTo = toId;
		pHdr->msgKind = UDataREQ;
	}      
	return m_serverWS->SendData(info);

}


BOOL CRemoteSession::OnLeaveREQ(plrInfoMsg* msg, CRemoteWS* ws)
{
	// we know that this message comes from the server, so we pass NULL pointer as
	// to the RemoveRemotePlayer to cause it to ignore WS object during player Lookup
	CRemotePlayer* p = RemoveRemotePlayer(msg->data.playerId, NULL);

	if (!p)
		return FALSE;

	CWS*  pws = p->m_ws;
	pws->RemovePlayer();
	if (pws->PlayerCount() == 0)
	{   
		// The player we're going to remove is the only one
		// on this worksation
		m_wsMap->Deregister(pws);
	}
	
	return TRUE;
}


BOOL CRemoteSession::OnJoinREP(plrInfoMsg* msg, CRemoteWS* ws)
{
	VPASSERT(m_serverWS);
	
    // the reply id must match request id
	if (m_pendingJoin && (m_pendingJoin->MsgId() == msg->hdr.msgId))
	{

		CLocalPlayer* p = m_pendingJoin->Player();
		p->FixId(msg->data.playerId);
		m_players->AddPlayer(p);
		PostNotification(m_pendingJoin);
		m_pendingJoin = NULL;
		
		if (m_initialJoin)
		{
			// this was the initial join - send a PenumREQ message
			
			genericMsg* msg = new(0) genericMsg(PenumREQ, 0);
			msg->hdr.msgId = NextMessageId();

			m_initialJoin = FALSE;
			
			sendDataInfo info(msg->Data(), msg->Size(), VP_MUSTDELIVER,NULL, NULL);
			return m_serverWS->SendData(info);
		}
			
			
		return TRUE;
	}
	
	return TRUE;
}


CRemoteWS* CRemoteSession::RegisterPlayerWS(plrInfoMsg* msg)
{

	CNetAddress* addr = m_net->MakeAddress(&msg->data.playerAddress);
	
	CWS*  ws = m_wsMap->FindByAddress(addr);
	if (ws == NULL)
	{
		ws = MakeRemoteWS(addr, NULL, NULL);
		if (!ws)
		{
			addr->Unref();
			return NULL;
		}
		m_wsMap->Register(ws);
		ws->Unref();
	}
	
	addr->Unref();
	return (CRemoteWS*) ws;
}


BOOL CRemoteSession::OnJoinADV(plrInfoMsg* msg, CRemoteWS* )
{
	if (m_players->PlayerAtId(msg->data.playerId))  // we're simply ignoring duplicates
		return TRUE;

	
	CRemoteWS* ws = RegisterPlayerWS(msg);
	if (!ws)
		return FALSE;

	CRemotePlayer* p = AddRemotePlayer(msg, ws, TRUE);
	
	if (!p)
	{
		return FALSE;
    }
	
//	ws->Unref();
	return TRUE;
}

BOOL CRemoteSession::OnJoinNAK(plrInfoMsg* msg, CRemoteWS* ws)
{
	if (!m_pendingJoin)
		return TRUE;
	
	PostNotification(m_pendingJoin);
	m_pendingJoin = 0;
	return TRUE;
}



BOOL CRemoteSession::AddLocalPlayer(LPCSTR playerName, 
								DWORD  playerFlags,
								LPVOID userData, LPVPPLAYERID)
{
	if (m_pendingJoin)
	{
		SetError(VP_ERR_BUSY);
		return FALSE;

	}
	
	
	CLocalPlayer* p = MakeLocalPlayer(playerName, playerFlags);
	
	if (!m_serverWS)
	{
		m_initialJoin = TRUE;
		if (!ConnectToServer(&m_info->data.sessionId, NULL))
		{
			delete p;
			return FALSE;
		}
	}
	
	
	plrInfoMsg* pInfoMsg = p->m_info;

	
	pInfoMsg->hdr.msgKind = JoinREQ;
	pInfoMsg->hdr.msgId = NextMessageId();
	m_pendingJoin = new CLocalJoin(p, pInfoMsg->hdr.msgId, userData);
	
	

	sendDataInfo info(pInfoMsg->Data(), pInfoMsg->Size(), VP_MUSTDELIVER,NULL, NULL);
	return m_serverWS->SendData(info);

}


#if 0
BOOL CRemoteSession::OnUDataREQ(genericMsg* msg, CRemoteWS* ws)
{
	IMPOSSIBLE(OnUDataREQ);
	return TRUE;
}


BOOL CRemoteSession::OnUBDataREQ(genericMsg* msg, CRemoteWS* ws)
{
	IMPOSSIBLE(OnUBDataREQ);
	return TRUE;
}

#endif

BOOL CRemoteSession::OnPenumREP(plrInfoMsg* msg, CRemoteWS* ws)
{
	// This is the reply on the request we've sent to server just after joining it
	// we're treating it like JOIN ADVERTISEMENT messages while ignoring info
	// about local players

	CPlayer* p = m_players->PlayerAtId(msg->data.playerId);
	if (!p->IsLocal())
		return OnJoinADV(msg, ws);

	return TRUE;
	
#if 0
		CWS* ws = RegisterPlayerWS(msg);
		if (!ws)
			return FALSE;

		CRemotePlayer* p = AddRemotePlayer(msg, ws, FALSE);
	
		if (!p)
		{
			return FALSE;
    	}
	
		ws->Unref();
		return TRUE;
#endif
}

struct AgeParams
{
	DWORD curTime;	   
	WSXList list;
	DWORD maxAge;
};


int CRemoteSession::CheckForAge(CWS* ws, LPVOID data)
{
	AgeParams* p = (AgeParams*) data;


	if (ws->IsRemote() && (p->curTime - ws->LastSeen() > p->maxAge))
		p->list.Append(new WSLink(ws));
	return TRUE;
}

void CRemoteSession::AgeServerList()
{
  
  if (m_connected)
  	  return;

  AgeParams p;

  p.curTime = GetCurrentTime();
  p.maxAge = m_maxServerAge;

  m_wsMap->Enum(CheckForAge, &p);

  WSLink* wsl;
  
  while(NULL != (wsl = p.list.RemoveFirst()))
  {
	
	  CServerDownNotification* n = new CServerDownNotification(
	    (CRemoteWS*) (wsl->m_data), 
	     m_serverEnumData);

	  if (!n)											  
	  {
		 SetError(VP_ERR_NOMEM);
		 return;
	  }

	  
	  PostNotification(n);
	  m_wsMap->Deregister(wsl->m_data);
	  delete wsl;
  }
}
	  	

BOOL CRemoteSession::OnSenumREP(sesInfoMsg* msg, CRemoteWS* ws)
{
	VPENTER(CRemoteSession::OnSenumREP);

	if (m_connected)
	{
		// we've just established the connection to the server
		// and the server responds with SenumREP

		Log("Got SenumREP in connected state");
		
		if (m_info)
			m_info->Unref();
		m_info = msg;
		m_info->Ref();
		VPLEAVEBOOL(CRemoteSession::OnSenumREP, TRUE);
		return TRUE;
	}


	Log("Got SenumREP in non-connected state");


	CWS* knownWs = m_wsMap->Find(ws);

	if (knownWs)   // we've already seen this workstation and notifyed the client app about it
	{
		ws->Touch();
		VPLEAVEBOOL(CRemoteSession::OnSenumREP, FALSE);
		return FALSE;
	}

	CSenumNotification* n = new CSenumNotification(msg, m_serverEnumData);
	if (!n)
	{
		Log("OnSenumRep: no moemory for notification object");
		SetError(VP_ERR_NOMEM);
		VPLEAVEBOOL(CRemoteSession::OnSenumREP, FALSE);
		return FALSE;
	}

	ws->KeepInfo(msg);
	m_wsMap->Register(ws);
	ws->Touch();

	Log("OnSenumRep: posting notification");
	PostNotification(n);

	if (!m_keepLog)
		m_net->StopDataLog();

	VPLEAVEBOOL(CRemoteSession::OnSenumREP, TRUE);
	return TRUE;
}


BOOL CRemoteSession::SetVisibility(BOOL v)
{
	 SetError(VP_ERR_REMOTE_SESSION);
	 return FALSE;
}



BOOL CRemoteSession::UpdateSessionInfo(LPVOID data)
{
	 SetError(VP_ERR_REMOTE_SESSION);
	 return FALSE;
}


void CRemoteSession::OnConnect(CNetLink* link)
{
	VPENTER(CRemoteSession::OnConnect);
	DWORD err = link->LastError();

	if (err)
	{
		Log("CRemoteSession::OnConnect got connection error");
		if (m_pendingJoin)
		{
			
			m_pendingJoin->SetError(VP_ERR_NET_ERROR);
			PostNotification(m_pendingJoin);
			m_pendingJoin = NULL;
		}

		OnDisconnect(link);
		VPEXIT();
		return;
	}

	if (m_serverWS && m_serverWS->m_safeLink == link)
	{
		m_connected = TRUE;
	}	

	VPEXIT();

}

void CRemoteSession::OnAccept(CNetLink* link)
{
	CNetAddress* addr = link->GetRemoteAddress();
	CWS* ws = m_wsMap->FindByAddress(addr);
	

	if (ws != NULL)
	{
		// We have already a WS at this NetAddress
		((CRemoteWS*) ws)->SetSafeLink(link);
	}
	else
	{
		ws = MakeRemoteWS(addr, link, NULL);
		if (ws)
		{
			m_wsMap->Register(ws);
			ws->Unref();
		}
	}
	
	addr->Unref();

}


void CRemoteSession::OnDisconnect(CNetLink* link)
{
	VPENTER(CRemoteSession::OnDisconnect);
	LogV(m_log, "RemoteSession::OnDisconnect link %08lx", (DWORD) link);

	if (m_broadcastLink == link)
	{
		m_connected = FALSE;
		HandleNetDown();
		VPEXIT();
		return;
	}

	if (m_connected && (m_serverWS->m_safeLink == link))
	{
		VPTRACE(("CRemoteSession::OnDisconnect - link to server broken"));
		m_connected = FALSE;
		
		CNotification* n = new CNotification(VP_SESSIONCLOSE, 0, 0, 0);
		if (!n)
		{
			FatalError(VP_ERR_NOMEM);
		}
		else
		{
			PostNotification(n);
		}

		VPEXIT();
		return;
	}

	CWS* ws = m_wsMap->FindBySafeLink(link);
	if (ws)
	{
		((CRemoteWS*)ws)->StopUsingSafeLink();
	}
	
	VPEXIT();
}



void CRemoteSession::ProcessSafeData(CNetLink* link, genericMsg* msg)
{
	BOOL unexpected = FALSE;

	switch(msg->hdr.msgKind)
	{
	   default:
		   unexpected = TRUE;
		   break;
		
	   case SenumREP:
		   VPASSERT(m_serverWS->m_safeLink == link);
		   OnSenumREP((sesInfoMsg*) msg, m_serverWS);
		   break;

	   case JoinREP:
		   VPASSERT(m_serverWS->m_safeLink == link);
		   OnJoinREP((plrInfoMsg*) msg, m_serverWS);
		   break;
		   
	   case JoinNAK:
		   VPASSERT(m_serverWS->m_safeLink == link);
		   OnJoinNAK((plrInfoMsg*) msg, m_serverWS);
		   break;
	
	   case JoinADV:
		   VPASSERT(m_serverWS->m_safeLink == link);
		   OnJoinADV((plrInfoMsg*) msg, m_serverWS);
		   break;
		   		   

	   case LeaveREQ:
		   VPASSERT(m_serverWS->m_safeLink == link);
		   OnLeaveREQ((plrInfoMsg*) msg, m_serverWS);
		   break;
    
	
	   case LeaveREP:
		   // we're silently ignoring this message
		   break;
		   

	   case  PenumREP:
		   VPASSERT(m_serverWS->m_safeLink == link);
		   OnPenumREP((plrInfoMsg*) msg, m_serverWS);
		   break;

	   case  UDataREQ:
		   OnUDataREQ(msg, NULL);
		   break;
		   
	   case  UBDataREQ:
		   OnUBDataREQ(msg, NULL);
		   break;

	   case  FtREQ:
		   {
		   CRemoteWS* ws = (CRemoteWS*) m_wsMap->FindBySafeLink(link);

		   if (ws)
				OnFtREQ((ftReqMsg*) msg, ws);
		   break;
		   }

	   case  FtACK:
		   {
		   CRemoteWS* ws = (CRemoteWS*) m_wsMap->FindBySafeLink(link);

		   if (ws)
				OnFtACK((ftAckMsg*) msg, ws);
		   break;
		   }
	   case DummyREQ:
		   break;
		   
	}
	   
	   
	if (unexpected)
		OnUnexpectedMsg(msg, link, TRUE);
	
}

//+ Process data coming from unsafe link
void CRemoteSession::OnUnsafeData(CNetLink* link)
{
	const DWORD maxReadSize = link->HasData();
	genericMsg* msg = new((size_t)maxReadSize-sizeof(VPMSGHDR)) genericMsg;
	CNetAddress* addr = m_net->MakeAddress();
	DWORD count =link->ReceiveFrom(msg->Data(), maxReadSize, *addr);
	BOOL unexpected = FALSE;
	
	if (count < sizeof(VPMSGHDR))
	{
		msg->Unref();
		return;
	}


	if (count < msg->Size())  // ignore badly formatted messages
	{
		msg->Unref();
		return;
	}

	
	switch(msg->hdr.msgKind)
	{
		case  UDataREQ:
		case  UBDataREQ:
		{
		   // we're going to ignore messages from unknown workstations
		   CWS* ws = m_wsMap->FindByAddress(addr);
		   if (!ws) break;
		   
		   // This is a message from known workstation
		   if (msg->hdr.msgKind == UDataREQ)
		   {	
			   OnUDataREQ(msg, (CRemoteWS*)ws);
	       }
		   else
		   {	
			   OnUBDataREQ(msg, (CRemoteWS*)ws);
	       }

		   break;
		}
		
	   case SenumREP:
	   	{
		   sesInfoMsg* siMsg = (sesInfoMsg*) msg;
		   CNetAddress* addr2 = m_net->MakeAddress(&siMsg->data.sessionId);

		   if (!addr2)
			   break;
	   	   CRemoteWS* ws  = MakeRemoteWS(addr2, NULL, NULL);
	   	   addr2->Unref();
		   if (!ws)
		   	 break;
	   	   

		   
		   OnSenumREP(siMsg, ws);
		   ws->Unref();
		   break;
		}   

		case  SenumREQ:
		{
			CRemoteWS* ws = MakeRemoteWS(addr, NULL, link);
			if (!ws)
				break;
			OnSenumREQ(msg, (CRemoteWS*) ws);
			ws->Unref();
			break;
		}

		case DummyREQ:
			break;

	    default:
		   unexpected = TRUE;
		   break;
	   }
	   
	   
	if (unexpected)
		OnUnexpectedMsg(msg, link, FALSE);
	
	msg->Unref();
	addr->Unref();
}


BOOL CRemoteSession::ConnectToServer(LPCVPNETADDRESS addr, LPVOID userData)
{
	CNetAddress* nA = m_net->MakeAddress(addr);
	CNetLink* link = m_net->MakeSafeLink(nA, userData);
	m_serverWS = MakeRemoteWS(nA, link, NULL);
	
	if (m_serverWS)
	{
		m_wsMap->Register(m_serverWS);
	
		link->Unref();
		nA->Unref();
	}
	return m_serverWS != NULL;
}
	
	
CRegisterySession::CRegisterySession(CTDLogger* log, CNetInterface* net, CPlayerMap* players, CWSMap* wsMap, DWORD maxAge) :
 CRemoteSession(log, net, players, wsMap, maxAge), m_queryAge(maxAge/2)
{
}
	

BOOL CRegisterySession::LookForServer(LPVOID data)
{
   
  AgeParams p;

  m_serverEnumData = data;
  genericMsg*  msg = new(0) genericMsg(SenumREQ, 0);

  if (!msg)
  	return FALSE;

  p.curTime = GetCurrentTime();
  p.maxAge = m_queryAge;




  m_wsMap->Enum(CheckForAge, &p);


  sendDataInfo info(msg->Data(), msg->Size(), 0, NULL, this);

  WSLink* wsl;
  
  while(NULL != (wsl = p.list.RemoveFirst()))
  {
	 CRemoteWS* ws = (CRemoteWS*) (wsl->m_data); 

	 ws->SendData(info);

     delete wsl;
  }


  msg->Unref();

  AgeServerList();
  return TRUE;

}


struct EnumRepParams
{
	CVpSession* 	session;
	CRemoteWS*  ws;
	WORD      	msgId;
};


int CRegisterySession::ReplyServerInfo(CWS* ws, LPVOID data)
{
	EnumRepParams& p = 	*(EnumRepParams*) data;

	if (ws->IsRemote())
	{
		CRemoteWS* rws = (CRemoteWS*) ws;
		sesInfoMsg*  msg = rws->m_info;

		msg->hdr.msgId = p.msgId;


 		sendDataInfo info(msg->Data(), msg->Size(), 0, NULL, p.session);

		p.ws->SendData(info);
	 }
	
	return TRUE;
}

 															 
BOOL CRegisterySession::OnSenumREQ(genericMsg* msg, CRemoteWS* ws)
{
	
	if (!m_wsMap->Count())
	{
		// send a dummy reply so the client will see something coming
		// from us;
		genericMsg msg(DummyREQ, 0);
		sendDataInfo info(msg.Data(), msg.Size(), 0, NULL, NULL);
		ws->SendData(info);
		return TRUE;

	}

	
	EnumRepParams p;

	p.session = this;
	p.ws = ws;
	p.msgId = msg->hdr.msgId;



	m_wsMap->Enum(ReplyServerInfo, &p);

	return TRUE;
}

 	  	




CPlayer::CPlayer(LPVPPLAYERINFO pInfo, CWS* ws) : m_ws(ws), m_info(NULL)
{
	if (pInfo)
	{
		m_info = new((size_t)pInfo->dataSize) plrInfoMsg((size_t) pInfo->dataSize);
		if (m_info)
		{
			_fmemcpy(&m_info->data, pInfo, vpPlayerInfoSize(pInfo));
		}
	}
}

CPlayer::CPlayer(plrInfoMsg* pInfoMsg, CWS* ws) : m_ws(ws), m_info(pInfoMsg)
{
	if (m_info)
		m_info->Ref();
}


CPlayer::~CPlayer()
{
	if (m_ws)
	{
		m_ws->Unref();
		m_ws = NULL;
	}

	if (m_info)
	{
		m_info->Unref();
		m_info = NULL;
	}
}
		



BOOL CRemotePlayer::SendData(sendDataInfo& sdi)
{
	 VPASSERT(sdi.m_ctxData);
	 VPASSERT(m_ws);

	 CVpSession& ses = *(CVpSession*) sdi.m_ctxData;

	 
     BOOL r=m_ws->SendData(sdi);
	 if (!r)
	 {
	 	 ses.Log("CRemote Player: error sending data");
		 ses.SetError(VP_ERR_NET_ERROR, m_ws->GetLastError());
	     return FALSE;
     }
	 return r;
}
		 

	

BOOL CRemoteWS::SendData(sendDataInfo& sdi)
{
	
	if (sdi.m_sendFlags & VP_MUSTDELIVER)
	{
		if (!m_safeLink)
		{
			CVpSession* ses = (CVpSession*) sdi.m_ctxData;
			
			m_safeLink = ses->MakeSafeLink(m_address);
			if (!m_safeLink)
				return FALSE;
			
		}
		return m_safeLink->Send(sdi.m_data, sdi.m_dataSize, sdi.m_sendFlags);
	}
	
	return m_unsafeLink->SendTo(*m_address, sdi.m_data, sdi.m_dataSize, 
		sdi.m_sendFlags);
}


CRemoteWS::~CRemoteWS()
{
	if (m_safeLink)
		m_safeLink->Unref();
	
	if (m_unsafeLink)
		m_unsafeLink->Unref();
	
	if (m_address)
		m_address->Unref();

	if (m_info)
		m_info->Unref();
	
}


CNotification::CNotification(WORD msgCode, 
							LPVOID data, 
							DWORD dataLen, 
							LPVOID userData)
{
	m_vpmsg.notificationCode = msgCode;
	m_vpmsg.u.data = data;
	m_vpmsg.dataLen = dataLen;
	m_vpmsg.userData = userData;
	m_vpmsg.creationTime = vpMsgTime();
	m_vpmsg.postTime = 0;
	m_vpmsg.createMsTime = timeGetTime();
}

CNotification::~CNotification()
{
}


