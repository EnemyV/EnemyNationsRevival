

class CNotification: public CActivity
{
public:
    VPMESSAGE  m_msg;

	CNotification(WPARAM msgCode, LPVOID userData, LPVOID data, DWORD dataLen);
	virtual ~CNotification();
	
	virtual void Complete();
		
}

				
		


/*
  We have Players, Sessions, Workstations and NetLinks.....
  The Players can be Local (to workstation) or Remote
  The Sessions can be too Local (which means this workstation is
  the Session Server) or Remote...
  The Workstations can be too Local (only one such is allowed) and
  Remote.  With each Remote workstation could be associated up to 2 NetLinks
  one is for non garanteeed data delivery, the second (optional) for
  garanteed data delivery
  Each player is identified by a PlayerID which is assiVPed by the 
  server... Each workstation maintains a copy of player map for the
  joined session. 
  
  Join Transaction:
	  The WS send to server JoinREQ and waits for server responce
	  The Server 
		  1) assiVPes the playerID, 
		  2) respond with JoinREP
		  3) Advertises the player to other WSs with JoinADV
      The original WS receives the JoinREP and store the player
	  info in it's player map
      Other Workstations receives the JoinADV and updates their
	  player map.
	  The REQ/REP pairs are matched uisng msgId field in the message header
   	  	  
	  
*/	  


// A network link
class CNetLink: public CRef
{
public:
    int       Unref() { return --m_refCnt; }
	int       Ref() { return ++m_refCnt; }
	
	
	virtual   BOOL Send(VPPLAYERID id, LPVOID data, DWORD dataSize, DWORD flags) = 0;
	virtual   DWORD Receive(LPVOID buf, DWORD bufSize, LPVPPLAYERID id) = 0;
    DWORD LastError()
	{
		DWORD e = m_err;
		m_err = 0;
		return e;
	}

 
	
	CNetLink();
	virtual ~CNetLink();
public:
	unsiVPed m_err;
	
};

// Workstation
class CWS: public CRef
{
public:
	virtual   BOOL Send(VPPLAYERID id, VPPLAYERID toId, LPVOID data, DWORD dataSize, DWORD flags) = 0;

};

class CRemoteWS: public CWS
{
public:
	 CNetLink*  m_safeLink;   // a link for garanteed delivery
	 CNetLink*  m_unsafeLink;  // a link for non garanteed delivery
     VPNETADDRESS m_address;   
	 DWORD      m_safeTrafficTime; // when we had the last traffic over
				                   // the safe link
 
	 virtual   BOOL Send(LPVOID data, DWORD dataSize, DWORD flags);	 
};

class CLocalWS: public CWS
{
public:

	CSession*   m_session;
};

	


class CPlayer
{
public:
   struct plrInfoMsg
   {
	   VPMSGHDR  hdr;
	   VPPLAYERINFO data;
	   
	   DWORD Size() const { return sizeof(hdr) +  VPPlayerInfoSize(&data) }; 
   };


	CPlayer(LPVPPLAYERINFO pInfo, CWS* ws);
	virtual ~CPlayer();
	VPPLAYERID  PlayerId() const { return m_info->data.playerId; }
	
	virtual BOOL IsLocal() const = 0; 
	virtual BOOL IsRemote() const = 0; 
	
public:	
	plrInfoMsg* m_info;
	CWS*        m_ws;   // player home workstation;
};

CPlayer::CPlayer(LPVPPLAYERINFO pInfo, CWS* ws) : m_ws(ws), m_info(NULL)
{
	if (pInfo)
	{
		size_t  infoSize = vpPlayerInfoSize(pInfo);
		m_info = new char[infoSize+sizeof(VPMSGHDR)];
		if (m_info)
		{
			_fmemcpy(&m_info.data, pInfo, infoSize);
		}
	}
}
CPlayer::~CPlayer()
{
	if (m_ws)
	{
		if (!m_ws->Unref())
		{
			delete m_ws;
			m_ws = NULL;
		}
	}
}

class CLocalLink: public CNetLink
{
public:

};


class CWSMap
{
	BOOL (*EnumProc)(CWS* ws, LPVOID data);
	
	void Add(CWS* ws);
	void Remove(CWS* ws);
	void Enum(EnumProc p, LPVOID data);
	
};

class CNetLinkMap
{
	BOOL (*EnumProc)(CNetLink* link, LPVOID data);
	
	void Add(CNetLink* link);
	void Remove(CNetLink* link);
	void Enum(EnumProc p, LPVOID data);
	
};




class CPlayerMap
{
public:
    typdef BOOL (*EnumProc)(VPPLAYERID, CPlayer*, LPVOID );
	virtual BOOL     AddPlayer(CPlayer* p) = 0;
	virtual CPlayer* PlayerAtId(VPPLAYERID playerId) const = 0;
	virtual CPlayer* PlayerAtAddress(LPCVPETADDRESS addr) = 0;
	virtual BOOL     RemovePlayer(VPPLAYERID id);
	virtual DWORD    Count() const = 0;
    virtual void  Enum(EnumProc p, LPVOID userData) = 0;	
	
	CPlayerMap();
	virtual ~CPlayerMap();
};
	

class CSession
{
public:
   typedef CWSMap::EnumProc WSEnum;

   enum MessageCodes
   {
	   JoinREQ = 'J', JoinREP='j', JoinNAK='N', JoinADV='A',
	   LeaveREQ = 'L', LeaveREP  = 'l',
	   PenumREQ = 'P', PenumREP = 'p',
	   UdataREQ = 'U',
	   UBDataREQ = 'B'
   };

   struct sesInfoMsg
   {
	   VPMSGHDR hdr;
	   VPSESSIONINFO data;
   };
   
   typedef plrInfoMsg CPlayer::plrInfoMsg;
   
   
   struct sendDataInfo 
   {
	   sendDataInfo(LPVOID data, DWORD dataSize, 
				   DWORD sendFlags,
		           LPVOID userData, LPVOID ctxData=NULL) :
				m_data(data), m_dataSize(dataSize), 
				m_sendFlags(sendFlags), m_userData(userData),
				m_ctxData(ctxData) { }
	

			   
		LPVOID m_data;
	    DWORD  m_dataSize; 
		DWORD  m_sendFlags;
		LPVOID m_userData;
		LPVOID m_ctxData;
	};
	   		   
   
   // Lock/Unlock the session object
   class CSessLock
   {
    public:
       CSesLock(const CSession* s) : m_lock((CMutex&) s->m_mutex) { }
	   CSesLock(const CSesion& s) : m_lock((CMutex&) s->m_mutex) { } 
    private:
       CMxLock m_lock;
   
    };

	CSession();
	virtual ~CSession();

	virtual BOOL IsLocal() const = 0; 
	virtual BOOL IsRemote() const = 0; 
	
	
	// Incoming message handling
	virtual BOOL OnJoinSession(plrInfoMsg* msg, CRemoteWS* ws);
	virtual BOOL OnLeaveSession(plrInfoMsg* msg, CRemoteWS* ws);
	virtual BOOL OnJoinReply(plrInfoMsg* msg) = 0;
	virtual BOOL OnUserData(usrDataMsg* msg) = 0;
	virtual BOOL OnUserBroadcast(usrDataMsg* msg) = 0;
	
	
	virtual CPlayer* FindPlayer(VPPLAYERID pId, CWS* ws = NULL) const;
	virtual CRemotePlayer* AddRemotePlayer(LPVPPLAYERINFO info,CRemoteWS* ws);
	virtual CRemotePlayer* RemoveRemotePlayer(VPPLAYERID id);
	

	virtual BOOL AddLocalPlayer(LPCSTR playerName, 
								DWORD  playerFlags,
								LPVOID userData) = 0;
	
	

	virtual BOOL SendData(VPPLAYERID id, LPVOID data, DWORD dataSize, 
		DWORD flags);
	
	virtual BOOL  UpdateSessionInfo(LPVOID data) = 0;
	
	virtual DWORD PlayerCount();

	virtual DWORD LastError(DWORD& errInfo)
	{
		DWORD e = m_error;
		m_error = VP_ERR_OK;
		errInfo = m_errInfo;
		return e;
    }

	virtual void SetError(DWORD error, DWORD errInfo = 0)
	{
		m_error = error;
		m_errInfo = errInfo;
    }

protected:
     BOOL BadBroadcastOptions(DWORD flags) = 0;
	 
	 static BOOL SendOnLink(CNetLink* link, LPVOID data);
	
     static BOOL SendAllPlayers(CWS* ws, LPVOID data);
	 static BOOL SendToWs(CWS* ws, LPVOID data);
	 
	 struct exceptEnumInfo
	 {
		 CWS*  exceptWs;
		 WSEnum p;
		 LPVOID data;
     };
	 
	 static BOOL ExceptEnumHelper(CWS* ws, LPVOID data);
	 void ForAllWorkstations(WSEnum p, LPVOID data)
	 {
		 CSessLock lk(this);
		 m_wsMap.Enum(p, data);
	 }

	 void ForAllWorkstationsExcept(CWS* ws, WSEnum p, LPVOID data);
	 
public:
	sesInfoMsg*     m_info;
	CPlayerMap*     m_players;
	CWSMap          m_wsMap;
	CWS*            m_localWs;  
	HWND            m_hWnd;
	
	CMutex          m_mutex;
	DWORD           m_error;
	DWORD           m_errInfo;
	
};

class CJoinNotification: public CNotification
{
	CJoinNotification(LPPLAYERINFO pInfo, DWORD errCode, LPVOID userData=NULL)
	:	CAsyncTask((WPARAM) ((errCode << 8) | VP_JOIN), pInfo,
			sizeof(*pInfo), userData) { }
	void Complete() { }
};


class CLeaveNotification: public CAsyncTask
{
public:
	CLeaveNotification(CPlayer* player, DWORD errCode, LPVOID userData=NULL);
	
	virtual void FreeBuffer()
	{
		delete m_player;
		m_player = NULL;
	}
	
   CPlayer* m_player;
};



BOOL CSession::ExceptEnumHelper(CWS* ws, LPVOID data)
{
	exceptEnumInfo& info = *(exceptEnumInfo*) data;
	
	return (ws == info.exceptWs) ? TRUE : info.p(ws, data);
}


void CSession::ForAllWorkstationsExcept(CWS* ws, WSEnum p, LPVOID data)
{
	exceptEnumInfo info;
	
	info.excepWs = ws;
	info.p = p;
	info.data = data;
	
	ForAllWorkstations(ExceptEnumHelper, &info);
}


BOOL CSession::SendOnLink(CNetLink* link, LPVOID data)
{
	ASSERT(link);
	ASSERT(data);
	
	sendDataIngo& sdi =  *(sendInfoData*) data;
	
	link->Send(sdi.m_data, sdi.m_dataSize, sdi.m_sendFlags);
	return TRUE;
}


CPlayer* CSession::FindPlayer(VPPLAYERID pId, CWS* ws)
{
	CPlayer* p = m_players->PlayerAtIt(pid);
	
	if (!p || ((ws != NULL) && (ws != p->m_ws))
		return NULL;
	
	return p;
}


CRemotePlayer* CSession::RemoveRemotePlayer(LPPLAYERINFO pInfo, CRemoteWS* ws)
{
	CPlayer* p = FindPlayer(pInfo->playerId, ws);
	
	if (!p)
	{
		SetError(VP_ERR_BAD_PLAYER_ID, pInfo->playerId);
		return NULL;
	}
	
	if (p->IsLocal())
	{
		SetError(VP_ERR_LOCAL_PLAYER, pInfo->playerId);
		return FALSE;
	}
		
	
	CLeaveNotification* n = new CLeaveNotification(p);
	m_players->Remove(p);
	
	if (!n)
	{
		SetError(VP_ERR_NOMEM);
		return p;
	}
	
	
	PostNotification(n);
	
	return (p);
}

	
	

CRemotePlayer* CSession::AddRemotePlayer(LPPLAYERINFO pInfo, CRemoteWS* ws)
{
	CRemotePlayer* player = new CRemotePlayer(pInfo, ws);
	
	if (!player)
	{
		SetError(VP_ERR_NOMEM);
		return FALSE;
    }
	
	m_players->Add(player, pinfo->playerId);

	CJoinNotification* n = new CJoinNotification(player->m_info, VP_ERR_OK);
	if (!n)
	{
		SetError(VP_ERR_NOMEM);
		return player;
	}
	
	PostNotification(n);
	return (player);
}


	
	

BOOL CSession::SendAllPlayers(CWS* ws, LPVOID data)
{
	ASSERT(ws);
	ASSERT(data);
	
	sendDataInfo& sdi =  *(sendInfoData*) data;
	LPVPMSGHDR pHdr = (LPVPMSGHDR) sdi.m_data;
	
	pHdr->msgTo = VP_ALLPLAYERS;
	
	ws->Send(sdi.m_data, sdi.m_dataSize, sdi.m_sendFlags);
	return TRUE;
}


BOOL CSession::SendToWs(CWS* ws, LPVOID data)
{
	ASSERT(ws);
	ASSERT(data);
	
	sendDataInfo& sdi =  *(sendInfoData*) data;
	
	ws->Send(sdi.m_data, sdi.m_dataSize, sdi.m_sendFlags);
	return TRUE;
}


	
class CLocalSession: public CSession
{
public:

	virtual BOOL IsLocal() const { return  TRUE; }
	virtual BOOL IsRemote() const { return FALSE; }
	
	virtual BOOL OnLeaveSession(plrInfoMsg* msg, CRemoteWS* ws);
	virtual BOOL OnJoinSession(plrInfoMsg* msg, CRemoteWS* ws);
	
protected:
    
};

BOOL CLocalSession::OnLeaveSession(plrInfoMsg* msg, CRemoteWS* ws)
{
	CRemotePlayer* p = RemoveRemotePlayer(&msg->data.playerId, ws);
	
	if (!p)
		return FALSE;
	
	
	// Now we're going to re-send a notification message to all Workstattions
	msg->hdr.msgTo = VP_LOCALMACHINE;
	msg->hdr.msgFrom = VP_SESSIONSEVRER;

	sendDataInfo info(msg, msg->Size(), VP_MUSTDELIVER,NULL,this);
	ForAllWorkstationsExcept(ws, SendToWs, &info);	
	
	delete p;
	
	
	return TRUE;
}


BOOL CLocalSession::OnJoinSession(plrInfoMsg* msg, CRemoteWS* ws)
{
	CRemotePlayer* p = AddRemotePlayer(&msg->data, ws);

	msg->hdr.msgTo = VP_LOCALMACHINE;
	msg->hdr.msgFrom = VP_SESSIONSERVER;

	sendDataInfo info(msg, msg->Size(), VP_MUSTDELIVER,NULL,this);

	
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



		
class CRemoteSession: public CSession
{
public:

	virtual BOOL IsLocal() const { return  TRUE; }
	virtual BOOL IsRemote() const { return FALSE; }

	virtual BOOL OnLeaveSession(plrInfoMsg* msg, CRemoteWS* ws);
	virtual BOOL OnJoinSession(plrInfoMsg* msg, CRemoteWS* ws);

	
	CRemoteWS*    m_serverWs;   // game server WS
	
};

BOOL CRemoteSession::OnLeaveSession(plrInfoMsg* msg, CRemoteWS* ws)
{
	CRemotePlayer* p = RemoveRemotePlayer(&msg->data.playerId, ws);
	
	if (!p)
		return FALSE;
	
	delete p;
	
	
	return TRUE;
}


BOOL CRemoteSession::OnJoinSession(plrInfoMsg* msg, CRemoteWS* ws)
{
	CRemotePlayer* p = AddRemotePlayer(&msg->data, ws);
	
	if (!p)
		return FALSE;
	
	return TRUE;
}


BOOL CLocalSession::UpdateSessionData(LPVOID data)
{
	 CSesLock lk(this);
	 _fmemcpy(1+&m_info, data, m_info.dataSize);
	 return BroadcastSessionData();
}


void CLocalSession::BroadcastSessionData()
{
	ASSERT(m_broadcastLink);
	
	DWORD dataSize = sizeof(*m_info) + m_info->data.dataSize;
	
	
	// First we send the broadcast message
	if (!m_broadcastLink->Send(m_info, 
							   dataSize, 
							   VP_BROADCAST))
	{
		
		SetError(VP_ERR_NET_ERROR, m_broadcastLink->LastError());	
		return FALSE;
	}
	
	
	// Now we're sending the data to all workstations
	sendDataInfo info(data,dataSize,VP_MUSTDELIVER,NULL,this);
	ForAllWorkstations(SendToWs, &info);	
	return m_err == VP_ERR_OK;
}





class CLocalPlayer: public CPlayer
{
public:
	virtual BOOL IsLocal() const { return  TRUE; }
	virtual BOOL IsRemote() const { return FALSE; }

	
};

class CRemotePlayer: public CPlayer
{
public:
    typedef CSession::sendDataIfo sendDataInfo;
	
	virtual BOOL IsLocal() const { return  FALSE; }
	virtual BOOL IsRemote() const { return TRUE; }

	BOOL    SendData(sendDataInfo& );

	VPPLAYERID m_remoteId;
	BOOL       m_accepted;

};

BOOL CRemotePlayer::SendData(sendDataInfo& sid)
{
	 CSession& ses = *(CSession*) sid.ctx;
		 
     BOOL r=m_ws->Send(sid.m_data, sid.m_dataSize,sid.m_flags);
	 if (!r)
	 {
		 ses.SetError(VP_ERR_NET_ERROR, m_ws->GetLastError());
	     return FALSE;
     };
}
		 
		

BOOL CRemoteSession::UpdateSessionData(LPVOID data)
{
	 SetError(VP_ERR_REMOTE_SESSION);
	 return FALSE;
}



	

BOOL CSession::SendData(VPPLAYERID toId,
                        VPPLAYERID fromId
                        LPVOID data, 
						DWORD dataSize, 
						DWORD flags,
						LPVOID userData)
{
	sendDataInfo info(data, dataSize, flags, userData, this);
	
	LPVPMSGHDR pHdr = (LPVPMSGHDR) data;
	
	pHdr->msgTo = toId;
	pHdr->msgFrom = fromId;
	pHdr->msgKind = UDataREQ;
	phdr->msgId = NextMessageId();
	
	if (!(flags & VP_BROADCAST))
	{
		m_mutex.Lock();
		CPlayer* player = m_players->GetAt(id);
		m_mutex.Unlock();
		
		if (!player)
		{
			SetError(VP_ERR_BAD_PLAYER_ID);
			return FALSE;
	    }
				

		return player->Send(info);
	}
	
	if (!GoodBroadcastOptions(flags))
		return FALSE;
	
	// we have a broadcast here

	ForAllWorkstations(SendAllPlayers, &info);
	return TRUE;
}



			
			