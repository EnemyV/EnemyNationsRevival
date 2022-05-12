#include "stdafx.h"


#ifdef WITH_DP
#include <dplay.h>
#include "dpnet.h"

HRESULT (WINAPI *pDirectPlayCreate)( LPGUID lpGUID, LPDIRECTPLAY FAR *lplpDP, IUnknown FAR *pUnk);
HRESULT (WINAPI *pDirectPlayEnumerate)( LPDPENUMDPCALLBACK, LPVOID );
HMODULE hdpDLL;




#define WM_VPDP (WM_USER+1)

static char vpServerPrefix[] = "VPS:";
static char vpClientPrefix[] = "VPC:";
static char vpBroadcastGroup[] = "VPBROADCAST";
#define vpPfxSize (sizeof(vpServerPrefix)-1)



// {6B819D60-FD19-11cf-AB39-0020AF71E433}
static const GUID vpDPGUID = 
{ 0x6b819d60, 0xfd19, 0x11cf, { 0xab, 0x39, 0x0, 0x20, 0xaf, 0x71, 0xe4, 0x33 } };



void ConvertStringToW(LPCSTR aStr, WCHAR* wStr)
{
	while(*wStr++ = *aStr++) ;
}


GUID& GUIDFromString(GUID& guid, LPCSTR lpszA)
{
	WCHAR sz[64];


	ConvertStringToW(lpszA, sz);

	IIDFromString(sz, &guid);
	return guid;
}


BOOL StringFromGUID(LPSTR buf, GUID& guid)
{
	WCHAR wBuf[64];

	StringFromGUID2(guid, wBuf, sizeof(wBuf));

	for(int i = 0; wBuf[i]; i++)
		*buf++ = (char) wBuf[i];

	*buf = 0;
	return TRUE;
}

union DPMSG
{
	DPMSG_GENERIC	 generic;
	DPMSG_ADDPLAYER  addPlayer;
	DPMSG_GROUPADD	 groupAdd;
	DPMSG_DELETEPLAYER deletePlayer;
	DPMSG_INVITE	   invite;
	char			buf[512];
}; 			

class CDPBufPool
{
public:

	CDPBuffer*  Allocate();
	void	   Free(CDPBuffer*);


	CDPBufPool();
	~CDPBufPool();

private:
	CMutex m_mx;
	CDPBufList m_free;
	CDPBufList m_allocated;
};

class CDPBuffer
{
public:
	DPID	m_to;
	DPID	m_from;
	DWORD	m_size;
	DPMSG   m_u;

	int operator==(const CDPBuffer& bp) const
	{
		return this == &bp;
	}

	CDPBuffer() :  m_to(0), m_from(0), m_size(sizeof(DPMSG)) { }
};

CDPBufPool::CDPBufPool() 
{ 
}

CDPBufPool::~CDPBufPool() 
{ 
	CMxLock lk(m_mx);

	CDPBuffer* b;

	while(NULL != (b = m_free.removeFirst()))
	{
		delete b;
	}

	while(NULL != (b = m_allocated.removeFirst()))
	{
		delete b;
	}
}

CDPBuffer* CDPBufPool::Allocate()
{
	CMxLock lk(m_mx);

	CDPBuffer* b = m_free.removeFirst();

	if (b == NULL)
		b = new CDPBuffer;

	if (b)
	{
		m_allocated.insert(b);
	}

	return b;
}

void CDPBufPool::Free(CDPBuffer* b)
{
 	CMxLock lk(m_mx);

	m_allocated.remove(b);
	m_free.insert(b);
}

	



#if 1

void vpUnloadDp()
{
   if (hdpDLL)
   		FreeLibrary(hdpDLL);
	
   hdpDLL = NULL;
   pDirectPlayCreate = NULL;
   pDirectPlayEnumerate = NULL;
}

BOOL vpLoadDp()
{
	BOOL ret = FALSE;

	VPENTER(vpLoadDp);
	if (hdpDLL)
	{
		VPEXITBOOL(TRUE);
		return TRUE;
	}

	hdpDLL = LoadLibrary("dplay.dll");
	if (hdpDLL)
	{
		*(FARPROC*)&pDirectPlayCreate = 
			GetProcAddress(hdpDLL, "DirectPlayCreate");
		*(FARPROC*)&pDirectPlayEnumerate = 
			GetProcAddress(hdpDLL, "DirectPlayEnumerate");

		ret = pDirectPlayCreate != NULL &&
			pDirectPlayEnumerate != NULL;

	}

	if (!ret)
	{
	   if (hdpDLL)
	   		FreeLibrary(hdpDLL);
	
	   hdpDLL = NULL;
	   pDirectPlayCreate = NULL;
	   pDirectPlayEnumerate = NULL;
	}

	VPEXITBOOL(ret);
	return ret;

}

BOOL IsDpSupported()
{
	return hdpDLL != NULL;
}


CNetInterface* MakeDpNet(CTDLogger* logger, LPCSTR transportGuid, HWND wnd)
{
	return new CDPNet(logger, transportGuid, wnd);
}




CDPNet::CDPNet(CTDLogger* logger, LPCSTR transportGuid, HWND wnd)	 : CNetInterface(logger)
{
	m_dp = NULL;
	m_serverMode = FALSE;
	m_serverPlayer = 0;
	m_broadcastId = 0;
	m_hEvent = INVALID_HANDLE_VALUE;
	m_hThread = INVALID_HANDLE_VALUE;
	m_hWnd = wnd;
	m_sesData = NULL;
	m_sesDataSize = 0;
	m_unsafeLnk = NULL;
	m_selectedSession = (DWORD) -1;
	m_forEnumeration = FALSE;
	m_pool = new CDPBufPool;
	m_nameIndex = 0;
	m_needClose = FALSE;
	
	GUID tGuid;

	GUIDFromString(tGuid, transportGuid);


    HRESULT hr = pDirectPlayCreate(&tGuid, &m_dp, NULL);

	if (FAILED(hr))
		SetError(VPNET_ERR_INIT, GetScode(hr));
}


void CDPNet::Cleanup()
{
	if (m_dp && m_needClose)
	{
		m_dp->Close();
		m_needClose = NULL;
	}

	if (m_hThread != INVALID_HANDLE_VALUE)
	{
		TerminateThread(m_hThread, 0);
		CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
	
	}

	CDPLink* l;

	while(NULL != (l = m_linkMap.RemoveFirst()))
	{
		delete l;
	}

	
	
	CNetInterface::Cleanup();


}

CDPNet::~CDPNet()
{
	Cleanup();

	m_dp->Release();
	delete m_pool;

}

CDPBuffer* CDPNet::GetBuffer()
{
	if (m_pool)
		return m_pool->Allocate();

	return NULL;
}

void CDPNet::FreeBuffer(CDPBuffer* bp)
{
	if (m_pool)
		m_pool->Free(bp);
}


void CDPNet::BecomeDeef()
{
	if (m_dp)
	{
		m_dp->EnableNewPlayers(FALSE);
	}
}

void CDPNet::OnTimer()
{
}

void CDPNet::GetAddress(LPVPNETADDRESS addr)
{
	*(LPDPID) addr = m_myAddr;
}



LRESULT CDPNet::OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_VPDP)
	{
		CDPBuffer* bp = (CDPBuffer*) lParam;

		if (bp->m_from == 0)
		{
			OnSysDpMessage(bp->m_u);
			FreeBuffer(bp);
			return TRUE;

		}

		if (bp->m_from == m_myAddr)
		{
			// ignore messages from myself
			FreeBuffer(bp);
			return TRUE;
		}
			
		CDPLink*  lnk = FindLink(bp->m_from);

		if (lnk == NULL)
		{
			m_unsafeLnk->m_queue.append(bp);
			OnUnsafeData(m_unsafeLnk);
			return TRUE;
				
		}

		lnk->m_queue.append(bp);
		OnSafeData(lnk);
		return TRUE;
	}

	return FALSE;
}
#if 0
		DPID dpFrom = 0;
		DPID dpTo = m_myAddr;
		DPMSG msgBuf;
		DWORD bufSize = sizeof(msgBuf);
		HRESULT hr = m_dp->Receive(&dpFrom, &dpTo, 
			DPRECEIVE_TOPLAYER|DPRECEIVE_PEEK, &msgBuf, &bufSize);

		 
		if (!FAILED(hr))
		{
			if (dpFrom == 0) // System Message
			{
				hr = m_dp->Receive(&dpFrom, &dpTo, 
				DPRECEIVE_TOPLAYER, &msgBuf, &bufSize);

			    if (FAILED(hr))
				{
					SetError(VPNET_ERR_DP, GetScode(hr));
					return TRUE;
				}

				OnSysDpMessage(msgBuf);

				return TRUE;
			}

			if (dpFrom == m_myAddr)
			{
				hr = m_dp->Receive(&dpFrom, &dpTo, 
				DPRECEIVE_TOPLAYER, &msgBuf, &bufSize);

				return TRUE;			// consume and ignore messages from myself
			}

			// do we have already a link from this source?
			CDPLink*  lnk = FindLink(dpFrom);

			if (lnk == NULL)
			{
				OnUnsafeData(m_unsafeLnk);
				return TRUE;
				
			}

			OnSafeData(lnk);
			return TRUE;
		}

		SetError(VPNET_ERR_DP, hr);
		return TRUE;
	}

	return TRUE;
}
#endif


void CDPNet::OnSysDpMessage(DPMSG& msg)
{
	BOOL ignoring = TRUE;

	switch(msg.generic.dwType)
	{
		case DPSYS_ADDPLAYER:
			VPTRACE(("DPSYS_ADDPLAYER %d %s\n", 
				msg.addPlayer.dpId,
				msg.addPlayer.szShortName));

			break;

		case DPSYS_DELETEPLAYER:
		{
			CDPLink* lnk = FindLink(msg.deletePlayer.dpId);
			if (lnk != NULL)
			{
				ignoring = FALSE;
				OnDisconnect(lnk);
			}
			VPTRACE(("DPSYS_DELETEPLAYER %d %s\n", 
				msg.deletePlayer.dpId,
				ignoring ? "ignored" : ""));
				
			break;

		}

		case DPSYS_ADDPLAYERTOGROUP:
		{

			if (m_serverMode)
			{
				if (msg.groupAdd.dpIdPlayer != m_myAddr)
				{
					CDPLink* lnk = MakeLinkObj(
						msg.groupAdd.dpIdPlayer, 
						NULL);
					if (lnk != NULL)
					{
						ignoring = FALSE;
						OnAccept(lnk);
					}
				}

			}
				
			VPTRACE(("DPSYS_ADDPLAYERTOGROUP group %d player %d %s\n",
				msg.groupAdd.dpIdGroup,
				msg.groupAdd.dpIdPlayer,
				ignoring ? "ignored" : ""));
		}
		break;

		case DPSYS_INVITE:
			VPTRACE(("DPSYS_INVITE\n"));
			break;

		case DPSYS_DELETEGROUP:
			VPTRACE(("DPSYS_DELETEGROUP %d\n", 
				msg.deletePlayer.dpId));
				
			break;

		case DPSYS_DELETEPLAYERFROMGRP:
			VPTRACE(("DPSYS_DELETEPLAYERFROMGRP group %d player %d\n",
				msg.groupAdd.dpIdGroup,
				msg.groupAdd.dpIdPlayer));
			break;
		
		case DPSYS_SESSIONLOST:
		{
			VPTRACE(("DPSYS_SESSIONLOST\n"));

			if (!m_serverMode)
			{
				CDPLink*  lnk = FindLink(m_serverPlayer);

				if (lnk)
					OnDisconnect(lnk);
			}

			break;
		}

		case DPSYS_CONNECT:
			VPTRACE(("DPSYS_CONNECT\n"));
			break;



	}


}


CNetLink* CDPNet::MakeSafeLink(CNetAddress* addr, LPVOID userData)
{
	DPID id = ((CDPAddress*) addr)->m_addr;

	CDPLink* lnk = FindLink(id);
	
	if (!lnk)
		lnk = MakeLinkObj(id, userData);
	else
		lnk->Ref();

	return lnk;
}

CNetLink* CDPNet::MakeUnsafeLink(LPVOID userData)
{
	if (!m_unsafeLnk)
	{
		m_unsafeLnk = MakeLinkObj(0, NULL);
	}

	m_unsafeLnk->Ref();
	return m_unsafeLnk;
	
}


CDPLink*  CDPNet::MakeLinkObj(DWORD addr, LPVOID userData)
{
	return new CDPLink(this, userData, addr);
}

CDPAddress* CDPNet::MakeAddressObj(DWORD addr)
{
	return new CDPAddress(addr);
}


CNetAddress* CDPNet::MakeAddress(LPCVPNETADDRESS addr)
{
	return MakeAddressObj(addr ? *(LPDPID) addr : 0);
}

CDPLink* CDPNet::FindLink(DWORD addr)
{
	LinkIter iter(m_linkMap);
	CDPLink*  l;

	while(NULL != (l = iter()))
	{
		if (l->m_remoteAddress == addr)
			return l;
	}
    return NULL;
}



//  find the player id representing the game server
//	and the group id representing the broadcast address
BOOL CDPNet::findServerCbk(DWORD pid, 	
						   LPSTR lpFriendlyName, 	
						   LPSTR lpFormalName,
						   DWORD dwFlags,
						   LPVOID ctx)
{
	CDPNet* dpNet = (CDPNet*)  ctx;
	if (dwFlags & DPENUMPLAYERS_GROUP)
	{
		if (!strcmp(lpFriendlyName, vpBroadcastGroup))
			dpNet->m_broadcastId = pid;
	}
	else if (!strncmp(lpFriendlyName, vpServerPrefix, vpPfxSize))
	{
		dpNet->m_serverPlayer = pid;
	}

	return TRUE;
}



void CDPNet::InitSessionDesc(LPDPSESSIONDESC desc)
{
	memset(desc, 0, sizeof(*desc));

	desc->dwSize = sizeof(*desc);
	desc->guidSession = vpDPGUID;
}

BOOL CDPNet::AttachToSession(DWORD sid, BOOL forEnum)
{
	m_selectedSession = sid;
	m_forEnumeration = forEnum;
	return Listen(FALSE, FALSE);
}

BOOL CDPNet::DetachFromSession()
{
	m_forEnumeration = FALSE;
	
	if (m_needClose)
	{
		HRESULT hr = m_dp->Close();

		if (FAILED(hr))
		{
			VPTRACE(("DP: Close failed\n"));
			Log("DP: Close failed\n");
			SetError(VPNET_ERR_DP, GetScode(hr));
			return FALSE;
		}
		m_needClose = TRUE;
	}
	return TRUE;
}


BOOL CDPNet::Listen(BOOL streamListen, BOOL serverMode)
{
	DPSESSIONDESC desc;

	m_serverMode = serverMode;

	if (!serverMode && m_selectedSession == (DWORD)-1)
	{
		// no DP session is still selected we need to postpone the connection
		// until one is really selected
		return TRUE;
	}


	InitSessionDesc(&desc);

	if (serverMode)
	{
		desc.dwMaxPlayers = 256;
		desc.dwFlags = DPOPEN_CREATESESSION;
		strcpy(desc.szSessionName, 	"vdmplay");
	}
	else
	{
		desc.dwFlags = DPOPEN_OPENSESSION;
		desc.dwSession = m_selectedSession;
   	}

	HRESULT hr = m_dp->Open(&desc);

	if (FAILED(hr))
	{
		VPTRACE(("DP: Open failed\n"));
		Log("DP::Open Failed\n");
		SetError(VPNET_ERR_DP, GetScode(hr));
		return FALSE;
	}

	if (m_serverMode)
	{
		hr = m_dp->CreateGroup(&m_broadcastId, vpBroadcastGroup, "");
		if (FAILED(hr))
		{
			VPTRACE(("DP: CreateGroup failed\n"));
			Log("DP::CreateGroup Failed\n");
			SetError(VPNET_ERR_DP, GetScode(hr));
			m_dp->Close();
			return FALSE;
		}
	}
	else
	{
		Sleep(250); 
		hr = m_dp->EnumPlayers(0, findServerCbk, this, 0);

		if (FAILED(hr))
		{
			VPTRACE(("DP: EnumPlayers failed\n"));
			Log("DP::EnumPlayers Failed\n");
			SetError(VPNET_ERR_DP, GetScode(hr));
			m_dp->Close();
			return FALSE;
		}
		hr = m_dp->EnumGroups(0, findServerCbk, this, 0);
		if (FAILED(hr))
		{
			VPTRACE(("DP: EnumGroups failed\n"));
			Log("DP::EnumGroups Failed\n");
			SetError(VPNET_ERR_DP, GetScode(hr));
			m_dp->Close();
			return FALSE;
		}
		
		if (m_broadcastId == 0 || m_serverPlayer == 0)
		{
			VPTRACE(("DP: m_broadcastId=%d, m_serverPlayer=%d\n",
					 m_broadcastId, m_serverPlayer));

			Log("Can't locate server player or broadcast group\n");
			SetError(VPNET_ERR_INIT, 0);
			m_dp->Close();
			return FALSE;
		}
	}


	char nmBuf[64];
	strcpy(nmBuf, m_serverMode ? vpServerPrefix : vpClientPrefix);

	DWORD nmSize = sizeof(nmBuf) - vpPfxSize;

	GetComputerName(nmBuf+vpPfxSize, &nmSize);

	// The DP DOC says it doesn't check for the same
	// names but ith reality is different
	// so we're adding an index to the generated name
	nmSize = strlen(nmBuf);
	wsprintf(nmBuf+nmSize, "_%d", m_nameIndex++);
	
	hr = m_dp->CreatePlayer(&m_myAddr, nmBuf, "", &m_hEvent);
	if (FAILED(hr))
	{
		VPTRACE(("DP: CreatePlayer failed\n"));
		SetError(VPNET_ERR_DP, GetScode(hr));
		m_dp->Close();
		return FALSE;
	}

	if (!m_forEnumeration)
	{
		hr = m_dp->AddPlayerToGroup(m_broadcastId, m_myAddr);
		if (FAILED(hr))
		{
			VPTRACE(("DP: AddPlayerToGroup failed\n"));
			Log("DP::AddPlayerToGroup Failed\n");
			SetError(VPNET_ERR_DP, GetScode(hr));
			m_dp->Close();
			return FALSE;
		}
	}


	DWORD id;

	m_hThread = CreateThread(NULL, 0, dpThreadProc, (LPVOID) this, 0, &id);
		
	if (m_hThread == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
		VPTRACE(("Error %d creating dpThread", err));
		Log("DP::Can't create thread\n");
		SetError(VPNET_ERR_INIT, err);
		m_dp->Close();
		return (FALSE);
	}


	MakeUnsafeLink();
	m_unsafeLnk->Unref();
	m_needClose = TRUE;

	return TRUE;

}

DWORD PASCAL CDPNet::dpThreadProc(LPVOID arg)
{
	CDPNet* dpNet = (CDPNet*) arg;
	

	while(dpNet->m_hEvent)
	{
		DWORD ret = WaitForSingleObject(dpNet->m_hEvent, INFINITE);
		DWORD msgCount = 0;

		if (ret != WAIT_OBJECT_0)
			break;

		CDPBuffer* bp = dpNet->GetBuffer();
		if (bp == NULL)
		{
			VPTRACE(("dpThreadProc:: Can't allocate buffer"));
			return 0;
		}

		bp->m_from  = 0;
		bp->m_to = dpNet->m_myAddr;

		HRESULT hr = dpNet->m_dp->Receive(&bp->m_from, &bp->m_to, 
			DPRECEIVE_TOPLAYER, &bp->m_u, &bp->m_size);

		if (!FAILED(hr))
		{
			PostMessage(dpNet->m_hWnd, WM_VPDP, (WPARAM) 0, (LPARAM) bp);
		}

	}

	return (0);
}


BOOL CDPLink::Send(LPVOID data, DWORD dataSize, DWORD flags)
{
	HRESULT hr = dpNet()->m_dp->Send(dpNet()->m_myAddr, 
		m_remoteAddress, DPSEND_GUARANTEE, data, dataSize);
	

    if (FAILED(hr))
	{
		SetError(VPNET_ERR_DP, GetScode(hr));
		return FALSE;
	}

	return TRUE;
}

    //+ Read the data form the link, return actual count of bytes read
DWORD CDPLink::Receive(LPVOID buf, DWORD bufSize)
{
	CDPBuffer* bp = m_queue.removeFirst();

	if (!bp)
		return 0;

	bufSize = min(bufSize, bp->m_size);
	memcpy(buf, bp->m_u.buf, bufSize);
	dpNet()->FreeBuffer(bp);
	return bufSize;
}



BOOL CDPLink::SendTo(CNetAddress& to, 
			              LPVOID data, 
						  DWORD dataSize, 
						  DWORD flags)
{
	CDPAddress& dpTo = (CDPAddress&) to;
	DPID toId = dpTo.m_addr;
	DWORD dpFlags = 0;
	
	if (flags & VP_BROADCAST) 
		toId = dpNet()->m_broadcastId;

	HRESULT hr = dpNet()->m_dp->Send(dpNet()->m_myAddr, 
		 toId, 0, data, dataSize);
	

    if (FAILED(hr))
	{
		SetError(VPNET_ERR_DP, GetScode(hr));
		return FALSE;
	}

	return TRUE;
}


//+ retValue = size of the datagram				  
DWORD CDPLink::ReceiveFrom(LPVOID buf, 
		            DWORD bufSize, 
			        CNetAddress& from)
{
	CDPAddress& fromAddr	= (CDPAddress&) from;
	CDPBuffer* bp = m_queue.removeFirst();

	if (!bp)
		return 0;

	bufSize = min(bufSize, bp->m_size);
	memcpy(buf, bp->m_u.buf, bufSize);
	fromAddr.m_addr = bp->m_from;
	dpNet()->FreeBuffer(bp);
	return bufSize;

#if 0	
	DPID dpFrom;
	DPID dpTo = dpNet()->m_myAddr;

	HRESULT hr = dpNet()->m_dp->Receive(&dpFrom, &dpTo, 
		DPRECEIVE_TOPLAYER, buf, &bufSize);


    if (FAILED(hr))
	{
		SetError(VPNET_ERR_DP, GetScode(hr));
		return 0;
	}

	CDPAddress& fromAddr	= (CDPAddress&) from;

	
	return bufSize;
#endif
}



//+ How much data is waiting to be read?
DWORD CDPLink::HasData()
{
	CDPBuffer* bp = m_queue.first();

	return bp ? bp->m_size : 0;


#if 0
	char buf[4];
	DWORD bufSize = 0;
	DPID dpFrom = m_remoteAddress;
	DPID dpTo = dpNet()->m_myAddr;
	DWORD flags = DPRECEIVE_PEEK;

	if (dpFrom != 0)
		flags |= DPRECEIVE_FROMPLAYER;

	HRESULT hr = dpNet()->m_dp->Receive(&dpFrom, &dpTo, 
		flags, buf, &bufSize);


    if (FAILED(hr))
	{
		if (hr != DPERR_BUFFERTOOSMALL)
		{
			SetError(VPNET_ERR_DP, GetScode(hr));	
 			return 0;
		}
		return bufSize;
	} 
	
	return 0;
#endif
}

	


//+ Get the adress of the peer
CNetAddress* CDPLink::GetRemoteAddress()
{
	CDPAddress*  addr = dpNet()->MakeAddressObj(m_remoteAddress);
	return addr;
}
			
	
CDPLink::CDPLink(CDPNet* net,  LPVOID userData, DWORD addr)	: CNetLink(net, userData), 
m_remoteAddress(addr)
{
	net->m_linkMap.Insert(this);
}

CDPLink::~CDPLink()
{
	CDPBuffer* bp;

	while(NULL != (bp = m_queue.removeFirst()))
		dpNet()->FreeBuffer(bp);

	dpNet()->m_linkMap.Remove(this);
}

#endif

#endif



