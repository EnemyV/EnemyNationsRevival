#include "stdafx.h"
#ifndef WIN32
typedef unsigned char UCHAR, FAR* PUCHAR;
typedef DWORD ULONG;
typedef char FAR* PCHAR;
#endif


#include "nb30.h"
#include "datagram.h"
#include "nbnet.h"


#if defined(CONTAINING_RECORD)
# undef CONTAINING_RECORD
#endif

#define CONTAINING_RECORD(address, type, field) \
    ((type *)((LPSTR)(address) - offsetof(type, field)))

#define MAX_NCBSENDS 4

int gPostFailed = 0;
int gBadCompletion = 0;
static int gNcbQueueSize = MAX_NCBSENDS;

extern "C"
{ 
extern NCB_POST NetbiosPostRoutine(PNCB);

NCB_POST  NCBCompletion(PNCB pNcb)
{
	CNcb* ncb = CONTAINING_RECORD(pNcb, CNcb, m_ncb);

	if (ncb->m_window)
	{
		if (!PostMessage(ncb->m_window, WM_COMMNOTIFY, 0, (ULONG) pNcb))
		{
			gPostFailed++;
			ncb->m_postFailed = TRUE;
		}
	} 
}

#ifndef WIN32
UCHAR
APIENTRY
Netbios(
    PNCB pncb
    )
{
   UCHAR retVal;
   extern UCHAR PASCAL NetbiosCall();
    
	__asm {
		LES bx, pncb
		call NetbiosCall
		mov retVal, al
	}
	return retVal;
}

#endif
}



void vpGetLanas(LANA_ENUM* lEnum)
{
	UCHAR ret;
	NCB ncb;

	lEnum->length = 0;
	memset(&ncb, 0, sizeof(ncb));

	ncb.ncb_command = NCBENUM;
	ncb.ncb_length = sizeof(*lEnum);
	ncb.ncb_buffer = (PUCHAR) lEnum;

	ret = Netbios(&ncb);

	if (NRC_GOODRET == ret)
	{
		return;
	}
	

	ADAPTER_STATUS* as;
			
	as = new ADAPTER_STATUS;
	
	for (int iNum=0; iNum<MAX_LANA; iNum++)
	{
				
		memset (as, 0, sizeof (*as));
		memset (&ncb, 0, sizeof (ncb));
		ncb.ncb_command = NCBASTAT;
		ncb.ncb_buffer = (PUCHAR) as;
		ncb.ncb_length = sizeof (*as);
		ncb.ncb_callname[0] = '*';
		ncb.ncb_lana_num = iNum;

		ret = Netbios (&ncb);
		if (((ret == NRC_GOODRET) || (ret == NRC_INCOMP)) && (as->rev_major >= 1))
		{
			lEnum->lana[lEnum->length++] = iNum;
		}
	}		
	delete as;
}



													  
CNbNet::CNbNet(CTDLogger* log, LPCSTR groupName, LPCSTR stationName, int lana) :
 CNetInterface(log),
 m_lana(lana), m_dgLink(NULL), m_cleaningUp(FALSE)
{
	m_groupName.SetNameText(groupName);
	m_stationName.SetNameText(stationName);	
	m_groupName.nameNumber = 0;
	m_stationName.nameNumber = 0;

	m_activeListens = 0;
	m_maxActiveListens = 2;
	m_activeReceives = 0;
	m_maxActiveReceives = 8;

	gNcbQueueSize = vpFetchInt("NETBIOS", "NCBQSize", MAX_NCBSENDS);
	if (gNcbQueueSize < 1 || gNcbQueueSize > 64)
		   gNcbQueueSize = MAX_NCBSENDS;


}


CNbNet::~CNbNet()
{
	Cleanup();
	ResetNetbios();
}

BOOL CNbNet::Supported()
{
	LANA_ENUM lEnum;

	vpGetLanas(&lEnum);

	if (lEnum.length == 0)
		return FALSE;

	
	return TRUE;
}


WORD CNbNet::ChooseLana()
{
	LANA_ENUM lEnum;

	vpGetLanas(&lEnum);

	if (lEnum.length == 0)
		return 0xffff;

	return lEnum.lana[0];
}


void CNbNet::OnNcbCompletion(CNcb* ncb)
{
	
	char logBuf[64];

	VPTRACE3(CNbNet, OnNcbCompletion, ("%08x, cmd = %02x, status=%02x, lsn=%02x, NameNum=%02x\n", 
										ncb, ncb->CommandCode() & 0x7f,
										ncb->Status(), ncb->GetLsn(), ncb->GetNameNum()));


	if (m_cleaningUp || (ncb->Status() == NRC_CANCEL) || ncb->IsLost())
	{

		if (ncb->IsQueued())
			m_activeNcbs.Remove(ncb);

		DropNcb(ncb);
		return;
	}

	if (ncb->Status() == NRC_PENDING)
	{
		ncb->m_ncb.ncb_cmd_cplt	= ncb->m_ncb.ncb_retcode;
	}

	VPASSERT(KnownActiveNcb(ncb));

	m_activeNcbs.Remove(ncb);
	BOOL doDrop = TRUE;

	switch(ncb->CommandCode() & 0x7f)
	{
		case NCBDGSEND:
			break;
		case NCBSEND:
			if (ncb->Status() == NRC_CMDCAN)
				break;
			if (ncb->Status() == NRC_GOODRET)
			{
				CNbLink* link = FindLink(ncb->GetLsn());

				if (link && !link->Closing())
				{
					link->m_pendingSends--;
					link->SendWaitingData();
				}
				break;
			}
			else 
			{
				wsprintf(logBuf, "NCBSEND status %02xx", ncb->Status());
				Log(logBuf);

				if (!ncb->Done())
					break;

				CNbLink* link = FindLink(ncb->GetLsn());

				if (link && !link->Closing())
				{
					
					if (m_disconnectHook)
						m_disconnectHook(link, m_hookData);
				}
			}

			break;

		case NCBDGRECV:
			if (ncb->Status() == NRC_GOODRET)
			{
				if (m_dgLink)
				{
					m_dgLink->OnReceiveComplete(ncb);
					doDrop = FALSE;
					SubmitReceiveDgram(ncb->GetNameNum());
					if (m_unsafeHook)
						m_unsafeHook(m_dgLink, m_hookData);

				}
			}

			break; 

		case NCBRECVANY:
			m_activeReceives--;
			if (ncb->Status() == NRC_GOODRET)
			{
				CNbLink*  link = FindLink(ncb->GetLsn());
				if (link)
				{
					link->OnReceiveComplete(ncb);
					doDrop = FALSE;
				}

				SubmitReceives();

				if (m_safeHook && link)
					m_safeHook(link, m_hookData);
				

			}
			else
			{

				wsprintf(logBuf, "NCBRECVANY status %02xx", ncb->Status());
				Log(logBuf);
				if (!ncb->Done())
					break;

				CNbLink* link = FindLink(ncb->GetLsn());


				if (link && !link->Closing())
				{
	
					if (m_disconnectHook)
						m_disconnectHook(link, m_hookData);

					if (ncb->Status() != NRC_CMDCAN)
						SubmitReceives();
				}
				
			}
			
			break;
				
		case NCBLISTEN:
			m_activeListens--;
			if (ncb->Status() == NRC_GOODRET)
			{
				CNbLink* link = MakeLinkObj(CNbLink::CONNECTED, NULL);
				link->m_lsn = ncb->GetLsn();
				link->SetRemoteName(ncb->GetCallName());
	
				if (m_acceptHook)
					m_acceptHook(link, m_hookData);

				link->Unref();

				SubmitListen();
				SubmitReceives();
			 }
			 	
			 break;
			 				
		case NCBCALL:
			if (ncb->Status() == NRC_GOODRET)
			{
	   			CNbLink* l = ncb->m_link;

				l->m_lsn = ncb->GetLsn();
			
				l->m_state = CNbLink::CONNECTED;
				l->SendWaitingData();
				if (m_connectHook)
					m_connectHook(l, m_hookData);
			
				SubmitReceives();

			}
			break;
		}				

	
	if (doDrop)
	{
		DropNcb(ncb);
	}
					 
}


void CNbNet::Cleanup()
{
	VPENTER(CNbNet::Cleanup);
	m_cleaningUp = TRUE;
		
	CNcb* ncb;

	CNcb* canNcb = NewNcb(NCBCANCEL);
	canNcb->SetSync();


	while(NULL != (ncb = m_activeNcbs.RemoveFirst()))
	{
		VPASSERT(ncb->IsLost());
		canNcb->SetBuffer((LPVOID) &ncb->m_ncb);
		VPTRACE(("Cancelling Ncb: %08x, cmd = %02x, lsn=%02x, NameNum=%02x\n", ncb, ncb->CommandCode() & 0x7f,
										 ncb->GetLsn(), ncb->GetNameNum())); 
	 	SubmitNcb(canNcb);
	}

#if 0
	canNcb->SetCommand(NCBDELNAME);
	canNcb->SetLocalName(m_stationName);
	SubmitNcb(canNcb);

	if (m_groupName.nameNumber)
	{
		canNcb->SetCommand(NCBDELNAME);
		canNcb->SetLocalName(m_groupName);
		SubmitNcb(canNcb);
	}

#endif

	FreeNcb(canNcb);

	m_activeListens = 0;
	m_activeReceives = 0;
	CNetInterface::Cleanup();
	VPEXIT();
 		
}


LRESULT CNbNet::OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != WM_COMMNOTIFY)
		return FALSE;

	CNcb* ncb = CONTAINING_RECORD(lParam, CNcb, m_ncb);

	OnNcbCompletion(ncb);
	return TRUE;
}



CNetAddress* CNbNet::MakeAddress(LPCVPNETADDRESS addr)
{
	return MakeAddressObj(addr ? 
					addr->machineAddress : m_stationName.nameText);
}

CNetAddress* CNbNet::MakeBroadcastAddress()
{
	return MakeAddressObj(m_groupName.nameText);
}

void CNbNet::CheckFlow()
{
	if (HostIsBlocked())
	{
		if (m_postponedQueue.Count())
			return;

		LinkIter iter(m_allLinks);
		CNbLink* p;

		while(NULL != (p = iter()))
		{
			if (p->m_state == p->CONNECTED && p->m_outQ.Count())
				return;
		}

		ResumeHost();
	}
}



void CNbNet::OnTimer()
{
	static BOOL inside = 0;

	if (inside)
		return;

	inside = TRUE;
#if 0
	if (gBadCompletion)
	{
		MessageBox(NULL, "Fatal Nebtios Bug Detected", "VDMPLAY", MB_OK);
		FatalError(VPNET_ERR_NETBIOS);
		inside = FALSE;
		gBadCompletion = FALSE;
	}
#endif
	CheckFlow();

	inside = FALSE;
	if (!gPostFailed)
		return;


	inside = FALSE;
	Log("CbnNet::OnTimer - detected failed posts");


	gPostFailed = 0;
	CNcbQIter iter(m_activeNcbs);
	CNcb*    ncb;
	
	
	while(NULL != (ncb = iter()))
	{
		if (ncb->m_postFailed)
			OnNcbCompletion(ncb);

	}
	inside = FALSE;
}

void CNbNet::BecomeDeef()
{
	CNcbQIter iter(m_activeNcbs);
	CNcb*    ncb;
	CNcb* cancelNcb = NewNcb(NCBCANCEL);
	
	if (!cancelNcb)
		return;
	
	cancelNcb->SetSync();
	
	while(NULL != (ncb = iter()))
	{
		if (ncb->CommandCode() == NCBLISTEN)
		{
			cancelNcb->SetBuffer(&ncb->m_ncb);
				
			if (!SubmitNcb(cancelNcb))
			{
				break;
			}
		}
	}
	
	FreeNcb(cancelNcb);
	
}


BOOL CNbNet::IsRegistered(CNbLink* link)
{
	LinkIter iter(m_allLinks);
	CNbLink* p;

	while(NULL != (p = iter()))
	{
		if (link ==	p)
			return TRUE;
	}

	return FALSE;
}

CNbLink* CNbNet::FindLink(UCHAR lsn)
{

	LinkIter iter(m_allLinks);
	CNbLink* p;

	while(NULL != (p = iter()))
	{
		if (p->m_lsn == lsn)
			return p;
	}

	return NULL;
}


CNbLink* CNbNet::MakeLinkObj(LinkState state,LPVOID userData)
{
	CNbLink* l = new CNbLink(state, this, userData);

 	if (!l)
	{
		Log("CNbNet::MakeLinkObj - no memory for link object");
		SetError(VPNET_ERR_NOMEM);
	}

	return l;
}




			
CNetLink* CNbNet::MakeUnsafeLink(LPVOID userData)
{

	if (!m_dgLink)
	{
		m_dgLink =  MakeLinkObj(CNbLink::DG, userData);
	}

	if (m_dgLink)
	{
		m_dgLink->Ref();
	}

	return m_dgLink;

}


CNetLink* CNbNet::MakeSafeLink(CNetAddress* addr, LPVOID userData)
{
	
	CNbAddress& a = *(CNbAddress*) addr;
	DWORD err = 0;
	LinkState st = CNbLink::CONNECTING;
	
	CNcb*  callNcb = NewNcb(NCBCALL);
	if (!callNcb)
		return NULL;
	
	callNcb->SetLocalName(m_stationName);
	callNcb->SetCallName(a.m_name);

	CNbLink* l = MakeLinkObj(st, userData);
	
	if (!l)
	{
		FreeNcb(callNcb);
		return NULL;
	}
	

	callNcb->m_link = l;

	if (!SubmitNcb(callNcb))
	{
		FreeNcb(callNcb);
		l->Unref();
		return NULL;
	}



   	return l;
}



BOOL CNbNet::RegisterOneName(int command, struct nameinfo& name)
{
	CNcb* ncb = NULL;
	BOOL ret;

	ncb = NewNcb(command);
	ncb->SetSync();

	if (!ncb) 
		return FALSE;

	ncb->SetLocalName(name);

	ret = SubmitNcb(ncb);

	if (ret)
		name.nameNumber = ncb->GetNameNum();

	FreeNcb(ncb);

	return ret;


}


BOOL CNbNet::RegisterNames(BOOL serverMode)
{

	BOOL gotName;
	if (!lstrlen(m_stationName))
	{
		int tries = 5;
		char buf[32];

		VPTRACE(("Netbios: Trying to generate a name"));

		while(tries--)
		{
			
			wsprintf(buf, "VP%lx", GetCurrentTime());
			m_stationName.SetNameText(buf);
			gotName = RegisterOneName(NCBADDNAME, m_stationName);
			if (gotName)
				break;	
		}

		if (!gotName)
		{
			VPTRACE(("Netbios: Name generation failed"));
			return FALSE;
		}

		VPTRACE(("Netbios: generated name '%16s' with number %02x", (LPCSTR) m_stationName, m_stationName.nameNumber));

	}
	else if (!RegisterOneName(NCBADDNAME, m_stationName))
	{
		VPTRACE(("Netbios: Register name for '%16s' failed", (LPCSTR) m_stationName)); 
		return FALSE;
	}
	else
	{
 		VPTRACE(("Netbios: station name added '%16s' with number %02x", (LPCSTR) m_stationName, m_stationName.nameNumber));
	}

	if (!serverMode)
		return TRUE;

    if (!RegisterOneName(NCBADDGRNAME, m_groupName))
	{
		VPTRACE(("Netbios: Register group name for '%16s' failed", (LPCSTR) m_groupName)); 
	 	return FALSE;
	}
	else
	{
   		VPTRACE(("Netbios: group name added '%16s' with number %02x", (LPCSTR) m_groupName, m_groupName.nameNumber));
	}


	return TRUE;
}



BOOL CNbNet::ResetNetbios()
{
#ifdef WIN32
	CNcb* ncb = NULL;
	BOOL ret;

	ncb = NewNcb(NCBRESET);

	if (!ncb)
		return FALSE;

	ncb->SetSync();
	ncb->SetNameNum(0);
	ncb->SetLsn(0);

	ret = SubmitNcb(ncb);
	FreeNcb(ncb);

	m_cleaningUp = FALSE;

	return ret;
#else
	return TRUE;
#endif 
}


BOOL CNbNet::Init()
{

  return TRUE;

}


BOOL CNbNet::SubmitReceiveCode(int command, int nameIndex)
{
	CNcb* ncb = NULL;
	LPVOID buf = NULL;
	
	ncb = NewNcb(command);
	if (!ncb) goto error;

	buf = AllocBuffer(MAXDGRAMSIZE);
	if (!buf) goto error;
	
	ncb->SetBuffer(buf, MAXDGRAMSIZE);
	ncb->SetNameNum(nameIndex);
	if (!SubmitNcb(ncb)) goto error;
	
	return TRUE;
	
error:
	if (buf)
		FreeBuffer(buf);
	if (ncb)
		FreeNcb(ncb);
	
	return FALSE;
	
}


BOOL CNbNet::SubmitReceiveDgram(int nameIndex)
{
	return SubmitReceiveCode(NCBDGRECV, nameIndex);
}


BOOL CNbNet::SubmitOneReceive()
{
	BOOL ret = SubmitReceiveCode(NCBRECVANY, m_stationName.nameNumber);

	if (ret)
		m_activeReceives++;
	
	return ret;

}


BOOL   CNbLink::EnqueueData(LPVOID data, DWORD size)
{
	Datagram* d = new Datagram(data, (WORD) size);

	if (!d)
	{
		Log("CNbLink::EnqueueData no memory for Datagram bject");

		SetError(VPNET_ERR_NOMEM);
		return FALSE;
	}

	if (!d->Ok())
	{
		delete d;
		Log("CNbLink::EnqueueData Bad Datagram object");
		SetError(VPNET_ERR_NOMEM);
		return FALSE;
	}

	m_outQ.Add(d);
	if (m_state == CONNECTED)
		m_net->SuspendHost();

	return TRUE;
  }


BOOL   CNbLink::SendWaitingData()
{
	BOOL ret = TRUE;
	m_outQ.Lock();
	
	Datagram* d;
	
	while((m_pendingSends < MAX_NCBSENDS) && (NULL != (d = m_outQ.Get())))
	{
		LPVOID data = d->MoreData();
		WORD   size = d->DataSize();
		DWORD  t = GetCurrentTime();

		if (!NonQSend(data, size))
		{
			ret = FALSE;
			Log("CnbLink::SendWaiting data - error sending data");
			VPTRACE(("Netbios Send error"));			

			break;
		}

		if (t - d->m_time  > 2)
		{
			char logBuf[128];

			wsprintf(logBuf, "CNbNet::SendWaitingData delay %ld ms "
				      "in data transmission", t - d->m_time);
			Log(logBuf);
		}
		delete d;
	}
	
	m_outQ.Unlock();

	return ret;
}




BOOL CNbNet::SubmitReceives()
{
	while((m_activeReceives < m_maxActiveReceives) && 
		SubmitOneReceive())
			;
		
	return m_activeReceives >=  m_maxActiveReceives;
}



BOOL CNbNet::SubmitOneListen()
{
	CNcb* ncb = NULL;
	LPVOID buf = NULL;
	static char callName[NCBNAMSZ+1] = "*";

	ncb = NewNcb(NCBLISTEN);
	if (!ncb) 
		return FALSE;
	
	ncb->SetLocalName(m_stationName);
	ncb->SetCallName(callName);

	if (!SubmitNcb(ncb))
	{
		FreeNcb(ncb);
		return FALSE;
	}

	m_activeListens++;
	return TRUE;
}

BOOL CNbNet::SubmitListen()
{
	while((m_activeListens < m_maxActiveListens) && 
		SubmitOneListen())
			;
		
	return m_activeListens >=  m_maxActiveListens;
}





BOOL CNbNet::Listen(BOOL streamListen, BOOL serverMode)
{


	if (!ResetNetbios())
	{
		Log("CNbNet::Listen - Failed to reset Netbios");
		VPTRACE(("Failed to reset Netbios"));
		return FALSE;
	} 
	

	if (!RegisterNames(serverMode))
	{
		Log("CNbNet::Listen - Failed to register Netbios names");
		VPTRACE(("Failed to register Netbios Names"));
		return FALSE;
	}




	MakeUnsafeLink();
	
	if (!m_dgLink)
		return FALSE;

    // The reference count of m_dgLink is == 2
	VPASSERT(m_dgLink->Count() == 2);
	m_dgLink->Unref();




	if (!SubmitReceiveDgram(m_stationName.nameNumber))
		return FALSE;
	
	if (serverMode)
	{
		if (!SubmitReceiveDgram(m_groupName.nameNumber))
			return FALSE;
	}
	
	if (streamListen)
	{
		if (!SubmitListen())
			return FALSE;
	}
	
	return TRUE;


}

void CNbNet::SetCompletionHook(CNcb* ncb)
{
#ifndef WIN32

	if (ncb->IsAsync())
		ncb->SetCompletion(NetbiosPostRoutine); 
#else
	if (ncb->IsAsync())
		ncb->SetCompletion(NCBCompletion); 
#endif
}


BOOL CNbNet::KnownActiveNcb(CNcb* ncb)
{
	CNcbQIter iter(m_activeNcbs);
	CNcb* p;

	while(NULL != (p = iter()))
	{
		if (ncb == p)
			return TRUE;
	}

	VPTRACE(("UnKnown Ncb: %08x, cmd = %02x, status=%02x, lsn=%02x, NameNum=%02x\n", 
										ncb, ncb->CommandCode() & 0x7f,
										ncb->Status(), ncb->GetLsn(), ncb->GetNameNum()));


	return FALSE;
}


void CNbNet::DropNcb(CNcb* ncb)
{
	LPVOID buf = ncb->GetBuffer();


	VPASSERT(ncb->IsLost());

	FreeBuffer(buf);
	FreeNcb(ncb);
}



BOOL CNbNet::SubmitNcb(CNcb* ncb)
{
	UCHAR ret;
	BOOL fromQueue = FALSE;
	char logBuf[32];

	ncb->m_postFailed = FALSE;
	ncb->SetLana((BYTE)m_lana);
	SetCompletionHook(ncb);

	if (ncb->IsAsync())
	{
		if (m_postponedQueue.Count())
		{
			m_postponedQueue.Append(ncb);
			ncb = m_postponedQueue.RemoveFirst();
			fromQueue = TRUE;
		}
		
		m_activeNcbs.Append(ncb);

		VPTRACE(("Submitting Async Ncb: %08x, cmd = %02x, lsn=%02x, NameNum=%02x\n", ncb, ncb->CommandCode() & 0x7f,
										 ncb->GetLsn(), ncb->GetNameNum())); 
		ret = Netbios(&ncb->m_ncb);
		if (NRC_GOODRET == ret || NRC_PENDING == ret)
			return TRUE;
		if (ret == NRC_TOOMANY)
		{
			if (fromQueue)
				m_postponedQueue.Insert(ncb);
			else
				m_postponedQueue.Append(ncb);
			return TRUE;
		}

		VPTRACE3(CNbNet, SubmitNcb, ("Async Submit failed: %08x, cmd = %02x, status=%02x, lsn=%02x, NameNum=%02x\n", 
										ncb, ncb->CommandCode() & 0x7f,
										ncb->Status(), ncb->GetLsn(), ncb->GetNameNum()));

		
		wsprintf(logBuf, "AsyncNcb %02xx status %02xx\n", 
			ncb->CommandCode(), ncb->Status());

		Log(logBuf);
		SetError(VPNET_ERR_NETBIOS, ret);
		return FALSE;
	}
	ncb->SetCompletion(NULL);
	ret = Netbios(&ncb->m_ncb);
	if (ret != NRC_GOODRET)
	{
		VPTRACE3(CNbNet, SubmitNcb, ("Sync Submit failed: %08x, cmd = %02x, status=%02x, lsn=%02x, NameNum=%02x\n", 
										ncb, ncb->CommandCode() & 0x7f,
										ncb->Status(), ncb->GetLsn(), ncb->GetNameNum()));

		SetError(VPNET_ERR_NETBIOS, ret);
		wsprintf(logBuf, "SyncNcb %02xx status %02xx\n", 
			ncb->CommandCode(), ncb->Status());

		Log(logBuf);
	}
	return ret == NRC_GOODRET;
}

CNbLink::CNbLink(LinkState st, CNbNet* net, LPVOID userData) :
   CNetLink(net, userData), m_state(st), m_remoteName(NULL),
   m_pendingSends(0)
{
	VPASSERT(net);
	net->RegisterLink(this);

}

	
CNbLink::~CNbLink()
{
	m_state = CLOSING;
	
	CleanDataQueue();
	CleanNcbQueue();
	
	if (m_net)
		((CNbNet*)m_net)->UnregisterLink(this);

	m_net = NULL;
}


void CNbLink::CleanDataQueue()
{
}


void CNbLink::CleanNcbQueue()
{
	CNcb* ncb;

	while(NULL != (ncb = m_inQ.RemoveFirst()))
	{
		DropNcb(ncb);
	}
}


void CNbLink::SetRemoteName(LPCSTR name)
{
 	if (!m_remoteName)
 	{
		VPASSERT(m_net);
 		m_remoteName = nbNet()->MakeAddressObj(name);
 	}
	else
	{
		m_remoteName->SetName(name);
	}
 }



CNcb* CNbLink::MakeNcb(int cmd, LPVOID data, DWORD size)
{
	CNcb* ncb = nbNet()->NewNcb(cmd);
	if (!ncb)
	{
		Log("CNbLink:: MakeNcb -- no memory for Ncb object");
		SetError(VPNET_ERR_NOMEM);
		return NULL;
	}

	if (size)
	{
		LPVOID buf = nbNet()->AllocBuffer(size, data);
		if (!buf)
		{
			Log("CNbLink:: MakeNcb -- no memory for buffer object");
			SetError(VPNET_ERR_NOMEM);			
			nbNet()->FreeNcb(ncb);
			return NULL;
		}

		ncb->SetBuffer(buf, (USHORT) size);
	}

	return ncb;
}

	
BOOL CNbLink::Send(LPVOID data, DWORD size, DWORD flags)
{
	VPASSERT(m_state != DG);
	
	if (m_state == CONNECTING)
		return EnqueueData(data, size);

	if (m_pendingSends >= MAX_NCBSENDS || m_outQ.Count())
		return EnqueueData(data, size);

	return NonQSend(data, size);
}

BOOL CNbLink::NonQSend(LPVOID data, DWORD size)
{
	CNcb* ncb = MakeNcb(NCBSEND, data, size);
	if (!ncb)
		return FALSE;

	ncb->SetLsn(m_lsn);
//	ncb->SetSync();

	m_pendingSends++;
	if (!nbNet()->SubmitNcb(ncb))
	{
		m_pendingSends--;
		DropNcb(ncb);
		return FALSE;
	}
	else
	{
//		DropNcb(ncb);
	}

	return TRUE;
}

BOOL CNbLink::SendTo(CNetAddress& na, LPVOID data, DWORD dataSize, DWORD flags)
{
	VPASSERT(m_state == DG);
	CNbAddress& nba = (CNbAddress&)na;


	CNcb* ncb = MakeNcb(NCBDGSEND, data, dataSize);
	
	if (!ncb)
		return FALSE;

	ncb->SetNameNum(nbNet()->m_stationName.nameNumber);
	
	if (flags & VP_BROADCAST)
	{
		ncb->SetCallName(nbNet()->m_groupName);
	}
	else
	{
		ncb->SetCallName(nba.m_name);
	}
	
	ncb->SetSync();
	BOOL ret = nbNet()->SubmitNcb(ncb);

//	if (!ret)
		DropNcb(ncb);

	return ret;

}

DWORD CNbLink::Receive(LPVOID data, DWORD dataSize)
{
	VPASSERT(m_state != DG);
	
	if (m_state == CONNECTING)
		return 0;

	CNcb* ncb = m_inQ.RemoveFirst();

	if (!ncb)
		return 0;


	DWORD xSize = ncb->GetSize();
	if (xSize < dataSize)
		xSize = dataSize;


	_fmemcpy(data, ncb->GetBuffer(), (size_t) xSize);

	DropNcb(ncb);

	return xSize;
	
}

DWORD CNbLink::ReceiveFrom(LPVOID data, DWORD dataSize, CNetAddress& na)
{
	VPASSERT(m_state == DG);

	CNcb* ncb = m_inQ.RemoveFirst();

	if (!ncb)
		return 0;

	DWORD xSize = ncb->GetSize();
	if (xSize > dataSize)
		xSize = dataSize;


	_fmemcpy(data, ncb->GetBuffer(), (size_t) xSize);
	CNbAddress& nba = (CNbAddress&)na;

	nba.SetName(ncb->GetCallName());
	
	DropNcb(ncb);

	return xSize;
	
}

CNetAddress* CNbLink::GetRemoteAddress()
{
	if (m_remoteName)
	{
		m_remoteName->Ref();
	}
	return m_remoteName;
}


CNbBufAlloc::CNbBufAlloc()
{
}


CNbBufAlloc::~CNbBufAlloc()
{
}


	
LPVOID  CNbBufAlloc::New(DWORD size)
{
#ifdef USE_VPMEM
	size_t* m =(size_t*) vpAllocMem(size+sizeof(size_t));
	if (m)
		*m++ = size+sizeof(size_t);

	return m;
#else

	return (LPVOID) new char[size];
#endif
}


void CNbBufAlloc::Free(LPVOID buf)
{
	  if (buf)
	  {
#ifdef USE_VPMEM
		size_t* m = (size_t*)buf;
		vpFreeMem(m-1,m[-1]);
#else
		delete [] (LPSTR) buf;
#endif
	  }

}


CNcbAlloc::CNcbAlloc()
{
}

CNcbAlloc::~CNcbAlloc()
{
	CNcb* ncb;

	while(NULL != (ncb = m_freeList.RemoveFirst()))
	{
		delete ncb;
	}

}


	
CNcb*  CNcbAlloc::New()
{
	CNcb* ncb = m_freeList.RemoveFirst();

	if (!ncb)
	{
		ncb = new CNcb;

	 	if (!ncb)
			return NULL;
	}

	memset(&ncb->m_ncb, 0, sizeof(ncb->m_ncb));
	ncb->m_window = NULL;
	ncb->m_net = NULL;
	ncb->m_link = NULL;

	return ncb;
}


void CNcbAlloc::Free(CNcb* ncb)
{
	VPASSERT(ncb->IsLost());
	m_freeList.Append(ncb);
}	
