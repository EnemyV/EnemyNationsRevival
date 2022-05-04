#include "stdafx.h"  

#define NWWIN

#include <nwipxspx.h>

#include "ipx16stb.h"
#include "datagram.h"
#include "ipx16net.h"



#define CONTAINING_RECORD(address, type, field) \
    ((type *)((LPSTR)(address) - offsetof(type, field)))

static BOOL gInitialized;
static DWORD gTaskId;
const WORD MAX_ECB_COUNT = 32;



extern "C"
{
extern __pascal ECBPostRoutine();

void __pascal ECBCompletion(ECB* pEcb)
{
	CEcb* ecb = CONTAINING_RECORD(pEcb, CEcb, m_ecb);
	
	if (ecb->m_window)
	{
		PostMessage(ecb->m_window, WM_COMMNOTIFY, 0, (DWORD) ecb);
	} 
}

}

#define cFF '\xff'
#define c00 '\0'
const ipx16_addr CIpx16Net::IPX16ADDR_BROADCAST = { { c00, c00, c00, c00 }, { cFF, cFF, cFF, cFF, cFF, cFF } };													  
#undef c00
#undef cFF

inline char htob(BYTE ch)
{
    if (ch <= 9) return ('0' + ch);             // handle decimal values
    if (ch <= 0xf) return ('A' + ch - 10);      // handle hexidecimal specific values
    return('X');                                // Someone screwed up
}

static void htoa(const char * src, char * dest, int srclen)
{
    char * destptr; // temp pointers
    const BYTE * srcptr;
        
    srcptr = (const BYTE *)src;
    destptr = dest;

    while(srclen--)
    {
    *destptr++ = htob((BYTE)(*srcptr >> 4));      // Convert high order nibble
    *destptr++ = htob((BYTE)(*srcptr++ & 0x0F));  // Convert low order nibble
    }
    *destptr = 0;  // Null terminator
}


char* CIpx16Address::ipx16_ntoa(const ipx16_addr* a)
{
	static char buf[64];

  	htoa(a->ipx16_netnum, buf, 4);
  	buf[8]='.';
  	htoa(a->ipx16_nodenum, buf+9, 6);
  	buf[8+1+12] = 0;

  	return buf;
	

}


static const char* cmdName[] =
{
 "CALL", "LISTEN", "SENDDG", "RECEIVEDG", "SEND", "RECEIVE", "HANGUP" 
};

static void DumpEcb(const char* text, CEcb* ecb, BOOL shortDump = FALSE)
{
   static char shortFmt[] = "%s %08lx, cmd=%s, socket=%u,%04xH\n"; 
   static char longFmt[] = "%s %08lx cmd=%s sock=%u,%04xH  proto=%d stat=%02x conn=%d strm=%02x dataSize=%d\n";

		
	WORD sock = SwapWord(ecb->m_ecb.socketNumber); 
 
	
	VPTRACE((shortDump ? shortFmt : longFmt, 
			text, ecb, cmdName[ecb->GetCommand()], sock, sock, ecb->m_hdr.packetType, ecb->Status(), 
			ecb->GetDestConnId(), ecb->m_hdr.dataStreamType, ecb->GetDataSize()));
			
			
}


CIpx16Net::CIpx16Net(CTDLogger* log, HWND window, WORD wellKnownPort, ipx16_addr* server) : 
	  CNetInterface(log),
	  m_listenLink(NULL),	
	  m_dgLink(NULL),
	  m_broadcastAddress(NULL),
	  m_wellKnownPort(SwapWord(wellKnownPort)),
	  m_ipxErr(0),
	  m_activeListens(0),
	  m_maxActiveListens(3),
	  m_activeIpxReceives(0),
	  m_maxIpxReceives(5),
	  m_maxSpxReceives(5),
	  m_activeSpxReceives(0),
	  m_window(window),
	  m_cleaningUp(FALSE),
	  m_dgPort(0),
	  m_streamPort(0)
{
	
	
	WORD maxEcb = MAX_ECB_COUNT;
	int ret;

	SetError(0);
			
	if (gInitialized)
	{
		m_taskId = gTaskId;
	}
	else
	{	
		m_taskId = 0;
		ret = IPXInitialize(&m_taskId, maxEcb, 576);
	
		if (ret)
		{
			Log("CIpx16Net: IPXInitialize failed");
			SetError2(VPNET_ERR_INIT, ret);
			return;
		}
	}
	
	IPXGetInternetworkAddress(m_taskId, (LPBYTE) &m_address.m_stationAddress);
	
	if (server)
		m_serverAddress = *server;
	else
		m_serverAddress = IPX16ADDR_BROADCAST;
		

}

CIpx16Net::~CIpx16Net()
{
	
    VPTRACE(("~CIpx16Net\n"));
	Cleanup();
	PostCleanup();
	
	
	IPXSPXDeinit(m_taskId);
}


BOOL CIpx16Net::Supported()
{
	VPENTER(CIpx16Net::Supported);

	DWORD taskId = 0;
	int ret = FALSE;

	if (gInitialized)
	{
		ret = TRUE;
		goto done;
	}

	if (!vpLoadIpxDll())
		goto done;

	ret = IPXInitialize(&taskId, MAX_ECB_COUNT, 576);

	if (!ret)
	{
		gInitialized = TRUE;
		gTaskId = taskId;;
		ret = TRUE;
	}
	
	
	vpUnloadIpxDll();
done:
	VPEXITBOOL(ret);
	return ret;
}
	
		

		
	
CIpx16Link* CIpx16Net::MakeLinkObj(LinkState state, CIpx16Address* addr, LPVOID userData)
{
	CIpx16Link* l = new CIpx16Link(state, this, userData);

 	if (!l)
	{
		Log("CIpx16Net::MakeLinkObj no memory for link object");

		SetError2(VPNET_ERR_NOMEM);
		return l;
	}
    
    l->m_remoteAddr = addr;
   	if (addr)
    	addr->Ref();
	return l;
}




			
CNetLink* CIpx16Net::MakeUnsafeLink(LPVOID userData)
{

	if (!m_dgLink)
	{
	
	
	
		m_dgLink =  MakeLinkObj(CIpx16Link::DG, NULL, userData);
	}

	if (m_dgLink)
	{
		m_dgLink->Ref();
	}

	return m_dgLink;

}


CNetLink* CIpx16Net::MakeSafeLink(CNetAddress* addr, LPVOID userData)
{
	
	CIpx16Address& a = *(CIpx16Address*) addr;
	int err = 0;
	LinkState st = CIpx16Link::CONNECTING;
	
	CEcb*  callEcb = NewEcb(CEcb::CALL);
	if (!callEcb)
		return NULL;
	

	CIpx16Link* l = MakeLinkObj(st,  &a, userData);
	
	if (!l)
	{
		FreeEcb(callEcb);
		return NULL;
	}
	

	callEcb->u.m_link = l;

	callEcb->SetCallAddress(&a);
	callEcb->SetSpxSocket(m_streamPort);
	
	SubmitEcb(callEcb);
	err = SPXEstablishConnection(m_taskId, 0, 0, 
		&l->m_connId, &callEcb->m_ecb);
	
	
	if (err)
	{
		OnEcbCompletion(callEcb);
		l->Unref();
		Log("CPx16NNet::MakeSafeLink SPXEstablish error");
        SetError2(VPNET_ERR_IPX16, err);
		return NULL;
	}



   	return l;
}

 
BOOL CIpx16Net::IsRegistered(CIpx16Link* link)
{
	LinkIter iter(m_allLinks);
	CIpx16Link* p;

	while(NULL != (p = iter()))
	{
		if (link ==	p)
			return TRUE;
	}

	return FALSE;
}

CIpx16Link* CIpx16Net::FindLink(WORD connId)
{
	LinkIter iter(m_allLinks);
	CIpx16Link* p;

	while(NULL != (p = iter()))
	{
		if (p->m_connId == connId)
			return p;
	}

	return NULL;
}

void CIpx16Net::Cleanup()
{

	m_cleaningUp = TRUE;
	

	VPTRACE(("CIpx16Net::Cleanup\n"));
	

#if 0		
	CEcb* ecb;
	while(NULL != (ecb = m_activeEcbs.RemoveFirst()))
	{
		VPASSERT(ecb->IsLost());
		DumpEcb("CleaningUp", ecb, TRUE);
	 	IPXCancelEvent(m_taskId, &ecb->m_ecb);
	}

#endif

	m_activeListens = 0;
	m_activeIpxReceives = 0;
	m_activeSpxReceives = 0;

	if (m_streamPort)
	{
		IPXCloseSocket(m_taskId, m_streamPort);
		m_streamPort = 0;
	}	


	if (m_dgPort)
	{
		IPXCloseSocket(m_taskId, m_dgPort);
		m_dgPort = 0;
	}	
    
    delete m_dgLink;
    delete m_listenLink;

	m_dgLink = NULL;
	m_listenLink = NULL;		



}

void CIpx16Net::PostCleanup()
{

	CEcb* ecb;

	WaitEcbCompletion();
		
	while(NULL != (ecb = m_activeEcbs.RemoveFirst()))
	{
		DropEcb(ecb);
	}
}
	


BOOL CIpx16Net::Listen(BOOL streamListen, BOOL serverMode)
{
  WORD port = m_dgPort;	
  int ret;
 
  if (m_cleaningUp)
  {
  	  PostCleanup();
	  m_cleaningUp = FALSE;
  }
     
  VPTRACE(("Ipx16Net::Listen(%d, %d)\n", streamListen, serverMode));
   
   if (!m_dgLink)
   {
	  if (serverMode)
		  m_dgPort = m_wellKnownPort;
	
 	  ret = IPXOpenSocket(m_taskId, &m_dgPort, 0xff);

	  VPTRACE(("IpxOpenSocket(%ld, %u=%u) = %d\n", m_taskId, port, m_dgPort, ret));
	  if (ret)
	  {
	  	 SetError2(VPNET_ERR_IPX16, ret);
		 return NULL;
	  }
   }
    
	if (streamListen)
	{   
		port = m_streamPort;
  		ret = IPXOpenSocket(m_taskId, &m_streamPort, 0xff);
		VPTRACE(("IpxOpenSocket(%ld, %u=%u) = %d\n", m_taskId, port, m_streamPort, ret));
	
		if (ret)
		{
			Log("Cipx16Net::Listen - IpxOpenSocket error");
			SetError2(VPNET_ERR_IPX16, ret);

			IPXCloseSocket(m_taskId, m_dgPort);
			return NULL;
		}
	}
	
    m_address.m_streamPort = m_streamPort;
    m_address.m_dgPort = m_dgPort;
    
    if (!m_wellKnownPort)
    	m_wellKnownPort = m_dgPort;
    	
    
    if (!m_broadcastAddress)
    {
    	MakeBroadcastAddress();
    	m_broadcastAddress->Unref();
    }
    
	if (!m_dgLink)
	{
		m_dgLink = MakeLinkObj(CIpx16Link::DG, NULL, NULL);
	
		if (!m_dgLink)
			goto error;
    }
  
	
	if (!SubmitIpxReceives())
		goto error;
	
	if (streamListen)
	{
		SubmitSpxReceives();
		if (!SubmitListen())
			goto error;
	}
	 
	return TRUE;

error:
	IPXCloseSocket(m_taskId, m_dgPort);
	IPXCloseSocket(m_taskId, m_streamPort);  
	return FALSE;
	
}  

BOOL CIpx16Net::SubmitOneListen()
{
	CEcb* ecb = NULL;

	ecb = NewEcb(CEcb::LISTEN);
	if (!ecb) 
		return FALSE;
	
	ecb->SetSpxSocket(m_streamPort);
	
	SubmitEcb(ecb);
	VPTRACE(("Calling SPXListen for socket %u\n", m_streamPort));
	SPXListenForConnection(m_taskId, 0, 0, &ecb->m_ecb);
	m_activeListens++;
	return TRUE;
}

BOOL CIpx16Net::SubmitListen()
{
	while((m_activeListens < m_maxActiveListens) && 
		SubmitOneListen())
			;
		
	return m_activeListens >=  m_maxActiveListens;
}



void CIpx16Net::SetCompletionHook(CEcb* ecb)
{
	ecb->SetCompletion((CEcb::CompletionProc)ECBPostRoutine);
}	

 
 
BOOL CIpx16Net::KnownActiveEcb(CEcb* ecb)
{
	CEcbQIter iter(m_activeEcbs);
	CEcb* p;

	while(NULL != (p = iter()))
	{
		if (ecb == p)
			return TRUE;
	}

	DumpEcb("UnKnown Ecb", ecb, TRUE);

	return FALSE;
}

BOOL CIpx16Net::HasActiveEcbs()
{
	CEcbQIter iter(m_activeEcbs);
	CEcb* p;

	while(NULL != (p = iter()))
	{
		if (!p->Completed())
			return TRUE;
	}
	
	return FALSE;
}
	
 
void CIpx16Net::DropEcb(CEcb* ecb)
{   
	LPVOID buf = ecb->GetBuffer();
	
	FreeBuffer(buf);
	FreeEcb(ecb);
}


BOOL CIpx16Net::SubmitSpxReceives()
{
	VPTRACE(("SubmitSpxRecives\n"));
	while((m_activeSpxReceives < m_maxSpxReceives) && 
		SubmitOneSpxReceive())
			;
		
	return m_activeSpxReceives >=  m_maxSpxReceives;
}


BOOL CIpx16Net::SubmitIpxReceives()
{
	VPTRACE(("SubmitIpxRecives\n"));
	
	while((m_activeIpxReceives < m_maxIpxReceives) && 
		SubmitReceiveDgram())
			;
		
	return m_activeIpxReceives >=  m_maxIpxReceives;
}




BOOL CIpx16Net::SubmitReceiveDgram()
{ 
	CEcb* ecb = NULL;
	LPVOID buf = NULL;

	VPTRACE(("SubmitIpxRecieveDgram\n"));
	
	ecb = NewEcb(CEcb::RECEIVEDG);
	if (!ecb) goto error;

	buf = AllocBuffer(MAXDGRAMSIZE);
	if (!buf) goto error;
	 
	 
	ecb->SetBuffer(buf, MAXDGRAMSIZE);
	ecb->SetIpxSocket(m_dgPort);
	
	SubmitEcb(ecb);
	IPXListenForPacket(m_taskId, &ecb->m_ecb);

	m_activeIpxReceives++;
	
	return TRUE;
	
error:
	VPTRACE(("SubmitIpxReceive: error label\n"));

	if (buf)
		FreeBuffer(buf);
	if (ecb)
		FreeEcb(ecb);
	
	return FALSE;
	
}



BOOL CIpx16Net::SubmitOneSpxReceive()
{
	VPTRACE(("SubmitOneSpxReceive\n"));
 
	CEcb* ecb = NULL;
	LPVOID buf = NULL;
	
	ecb = NewEcb(CEcb::RECEIVE);
	if (!ecb) goto error;

	buf = AllocBuffer(MAXDGRAMSIZE);
	if (!buf) goto error;
	 
	 
	ecb->SetBuffer(buf, MAXDGRAMSIZE);
	ecb->SetSpxSocket(m_streamPort);
	
	SubmitEcb(ecb);
	m_activeSpxReceives++;

	SPXListenForSequencedPacket(m_taskId, &ecb->m_ecb);

	
	return TRUE;
	
error:
	VPTRACE(("SubmitOneSpxReceive: error label\n"));
	if (buf)
		FreeBuffer(buf);
	if (ecb)
		FreeEcb(ecb);
	
	return FALSE;
	
}

static void MessageLoop()
{
 	BOOL gotMessage = FALSE;
	MSG msg;
	 	
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
    	gotMessage = TRUE;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	if (!gotMessage)    
         WaitMessage(); // Will not return until a message is posted.
} 

// Wait for completion of the given ECB... If the parameter is NULL wait for completion of ALL Ecbs
void CIpx16Net::WaitEcbCompletion(CEcb* ecb)
{
	while(ecb ? ecb->Completed() : HasActiveEcbs())
	{
		IPXRelinquishControl();
		
		MessageLoop();
	}
}
		
		
		



void CIpx16Net::BecomeDeef()
{
	CEcbQIter iter(m_activeEcbs);
	CEcb*    ecb;   
	
	VPTRACE(("CIpx16Net::BecomeDeef\n"));
	
	while(NULL != (ecb = iter()))
	{   
		int cmd = ecb->GetCommand();
		
		if (cmd == CEcb::LISTEN || CEcb::RECEIVEDG == cmd || cmd == CEcb::RECEIVE)
		{  
			DumpEcb("BecomeDeef: ", ecb, TRUE);
		 	IPXCancelEvent(m_taskId, &ecb->m_ecb); 
		}
		
	}
  
	m_activeListens = 0;
	m_activeIpxReceives = 0;
	m_activeSpxReceives = 0;
	
	if (m_streamPort)
	{
		IPXCloseSocket(m_taskId, m_streamPort);
		m_streamPort = 0;
	}	

}


void CIpx16Net::OnEcbCompletion(CEcb* ecb)
{

	DumpEcb("OnEcbCompetion", ecb);
    
    if (m_cleaningUp)
    {
    	VPTRACE(("OnEcbCompletion -- cleaning up\n"));
    	return;
    }
	
	if ((ecb->Status() == ECB_CANCELLED) || ecb->IsLost())
	{
        
        VPTRACE(("OnEcbCompeltion: ignoring ecb %08lx m_cleaningUp=%d isLost=%d\n", 
        		ecb, m_cleaningUp, ecb->IsLost()));
        		
		if (ecb->IsQueued())
			m_activeEcbs.Remove(ecb);

		DropEcb(ecb);
		return;
	}

	VPASSERT(KnownActiveEcb(ecb));

	m_activeEcbs.Remove(ecb);
	BOOL doDrop = TRUE;

	switch(ecb->GetCommand())
	{
		case CEcb::SENDDG:
			break;
		case CEcb::SEND:
			if (ecb->Status() == SUCCESSFUL || ecb->Status() == ECB_CANCELLED)
			{
				break;
			}
			else 
			{
				CIpx16Link* link = ecb->u.m_link;

				if (link && !link->Closing())
				{
					link->m_state = link->CLOSING;
					if (m_disconnectHook)
						m_disconnectHook(link, m_hookData);
				}
			}

			break;

		case CEcb::RECEIVEDG:
			VPTRACE(("RECIEEDG: activeIpxReceives = %d\n", m_activeIpxReceives)); 
			if (m_activeIpxReceives)
				m_activeIpxReceives--;
			SubmitIpxReceives();
			if (ecb->Status() == SUCCESSFUL)
			{
				if (m_dgLink)
				{
					m_dgLink->OnReceiveComplete(ecb);
					doDrop = FALSE;
					if (m_unsafeHook)
						m_unsafeHook(m_dgLink, m_hookData);

				}
			}

			break; 

		case CEcb::RECEIVE:
			VPTRACE(("RECIEVE: activeSpxReceives = %d\n", m_activeSpxReceives)); 
			if (m_activeSpxReceives);
				m_activeSpxReceives--;
			SubmitSpxReceives();
			if ((ecb->Status() == SUCCESSFUL) && !ecb->SpxCloseNotification())
			{
				CIpx16Link*  link = FindLink(ecb->GetDestConnId());
				VPASSERT(link);
				link->OnReceiveComplete(ecb);
				if (m_safeHook)
					m_safeHook(link, m_hookData);
				doDrop = FALSE;

			}
			else
			{
				CIpx16Link*  link = FindLink(ecb->GetDestConnId());
				VPTRACE(("RECIEVE: breaking link %08lx %04x\n", link, ecb->GetDestConnId())); 

				if (link && !link->Closing())
				{
	
					link->m_state = link->CLOSING;
					if (m_disconnectHook)
						m_disconnectHook(link, m_hookData);

				}
				
			}
			
			break;
				
		case CEcb::LISTEN:
			if (m_activeListens)
				m_activeListens--;
			SubmitListen();	
			SubmitSpxReceives();
			if (ecb->Status() ==  SUCCESSFUL)
			{
				CIpx16Address* addr = MakeAddressObject((ipx16_addr&) *(ecb->GetDgAddress())
					, ecb->GetSourceSocket(), 0);
				CIpx16Link* link = MakeLinkObj(CIpx16Link::CONNECTED, addr, NULL);

				link->m_connId = ecb->GetConnNum();
	            addr->Unref();
	            
				if (m_acceptHook)
					m_acceptHook(link, m_hookData);

				link->Unref();
			 }
			 	
			 break;
			 				
		case CEcb::CALL:
			if (ecb->Status() == SUCCESSFUL)
			{
	   			CIpx16Link* l = ecb->u.m_link;

				l->m_state = CIpx16Link::CONNECTED;
			
				l->SendWaitingData();
				if (m_connectHook)
					m_connectHook(l, m_hookData);
			
				SubmitSpxReceives();

			}
			break;
		}				

	
	if (doDrop)
	{
		DropEcb(ecb);
	}
					 
}


void  CIpx16Net::SubmitEcb(CEcb* ecb)
{
	SetCompletionHook(ecb);
	DumpEcb("SubmitEcb:", ecb);  
	m_activeEcbs.Append(ecb);
	
	
	
}



CIpx16Link::CIpx16Link(LinkState st, CIpx16Net* net, LPVOID userData) :
   CNetLink(net, userData), m_state(st), m_net(net), m_remoteAddr(NULL)
{
	VPASSERT(m_net);
	m_net->RegisterLink(this);

}

	
CIpx16Link::~CIpx16Link()
{   
 
 	VPTRACE(("~CIpx16Link %08lx %s\n", this, (m_state == CONNECTED) ? "CONNECTED" : "DG"));
 	
	if (m_state == CONNECTED)
	{
		CEcb* ecb = MakeEcb(CEcb::HANGUP, NULL, 0);
		
		SubmitSpxEcb(ecb);
		ecb->SetCompletion(NULL);
		SPXTerminateConnection(m_net->m_taskId, m_connId, &ecb->m_ecb);
		m_net->WaitEcbCompletion(ecb);
		m_net->OnEcbCompletion(ecb);
	}
	
	
	 	
	m_state = CLOSING;
	
	CleanDataQueue();
	CleanEcbQueue();
	
	if (m_net)
		m_net->UnregisterLink(this);

	m_net = NULL;
}


void CIpx16Link::CleanDataQueue()
{
}


void CIpx16Link::CleanEcbQueue()
{
	CEcb* ecb;

	while(NULL != (ecb = m_inQ.RemoveFirst()))
	{
		DropEcb(ecb);
	}
}

void CIpx16Net::OnTimer()
{
	CheckFlow();
}


void CIpx16Net::CheckFlow()
{
	if (!HostIsBlocked())
		return;
	
	LinkIter iter(m_allLinks);
	CIpx16Link* p;

	while(NULL != (p = iter()))
	{
		if (p->m_state == p->CONNECTED && p->m_outQ.Count() != 0)
			return;
	}

	ResumeHost();
}
	



BOOL   CIpx16Link::EnqueueData(LPVOID data, DWORD size)
{
	Datagram* d = new Datagram(data, (WORD) size);

	if (!d)
	{
		Log("CIpx16Link::EnqueueData -  no memory for datagram object");
		SetError(VPNET_ERR_NOMEM);
		return FALSE;
	}

	if (!d->Ok())
	{
		delete d;
		Log("CIpx16Link::EnqueueData -  bad datagram object");
		SetError(VPNET_ERR_NOMEM);
		return FALSE;
	}

	m_outQ.Add(d);
	if (m_state == CONNECTED)
		m_net->SuspendHost();
	return TRUE;
}

CEcb* CIpx16Link::MakeEcb(int cmd, LPVOID data, DWORD size)
{
	CEcb* ecb = m_net->NewEcb(cmd);

	if (!ecb)
	{
		Log("CIpx16Link::MakeEcb no memory for ECB object");
		SetError2(VPNET_ERR_NOMEM);
		return NULL;
	}

	if (size)
	{
		LPVOID buf = m_net->AllocBuffer(size, data);
		if (!buf)
		{
			Log("CIpx16Link::MakeEcb no memory for buffer");
			SetError2(VPNET_ERR_NOMEM);			
			m_net->FreeEcb(ecb);
			return NULL;
		}

		ecb->SetBuffer(buf, (size_t) size);
	}

	return ecb;
}

	    
void CIpx16Link::DropEcb(CEcb* ecb)
{   
	m_net->DropEcb(ecb);
}

void CIpx16Link::SubmitIpxEcb(CEcb* ecb)
{
	ecb->SetIpxSocket(m_net->m_dgPort);
	ecb->m_hdr.packetType = 4;
	m_net->SubmitEcb(ecb);  
}

void CIpx16Link::SubmitSpxEcb(CEcb* ecb)
{
	ecb->SetSpxSocket(m_net->m_streamPort);
	ecb->m_hdr.packetType = 5;
	m_net->SubmitEcb(ecb);
	  
}


BOOL   CIpx16Link::SendWaitingData()
{
	BOOL ret = TRUE;
	m_outQ.Lock();
	
	Datagram* d;
	
	while(NULL != (d = m_outQ.Get()))
	{
		LPVOID data = d->MoreData();
		WORD   size = d->DataSize();
		
		if (!Send(data, size, 0))
		{

			ret = FALSE;
			Log("CIpx16Link::SendWatingData - error sending data");

			break;
		}

		delete d;
	}
	
	m_outQ.Unlock();

	return ret;
}

	    
BOOL CIpx16Link::Send(LPVOID data, DWORD size, DWORD flags)
{
	VPASSERT(m_state != DG);
	
	if (m_state == CONNECTING)
		return EnqueueData(data, size);

	CEcb* ecb = MakeEcb(CEcb::SEND, data, size);
	if (!ecb)
		return FALSE;

    SubmitSpxEcb(ecb);
	
	SPXSendSequencedPacket(m_net->m_taskId, m_connId, &ecb->m_ecb);
	return TRUE;
}

BOOL CIpx16Link::SendTo(CNetAddress& na, LPVOID data, DWORD dataSize, DWORD flags)
{
	VPASSERT(m_state == DG);
	CIpx16Address& ipxA = (CIpx16Address&)na;


	CEcb* ecb = MakeEcb(CEcb::SENDDG, data, dataSize);
	
	if (!ecb)
		return FALSE;
    
    ecb->SetIpxSocket(m_net->m_dgPort);

	if (flags & VP_BROADCAST)
	{
		ecb->SetBroadcastAddress(m_net->m_broadcastAddress);
	}
	else
	{   
		int timeout;

		ecb->SetDgAddress(&ipxA);
		IPXGetLocalTarget(m_net->m_taskId, (LPBYTE) &ecb->m_hdr.destination, ecb->m_ecb.immediateAddress, &timeout);
	}
	
	SubmitIpxEcb(ecb);
    
    IPXSendPacket(m_net->m_taskId, &ecb->m_ecb);
    
	return TRUE;

}

DWORD CIpx16Link::Receive(LPVOID data, DWORD dataSize)
{
	VPASSERT(m_state != DG);
	
	if (m_state == CONNECTING)
		return 0;

	CEcb* ecb = m_inQ.RemoveFirst();

	if (!ecb)
		return 0;


	size_t xSize = ecb->GetDataSize();
	if (xSize < dataSize)
		xSize = (size_t) dataSize;


	_fmemcpy(data, ecb->GetBuffer(), xSize);

	DropEcb(ecb);

	return xSize;
	
}

DWORD CIpx16Link::ReceiveFrom(LPVOID data, DWORD dataSize, CNetAddress& na)
{
	VPASSERT(m_state == DG);

	CEcb* ecb = m_inQ.RemoveFirst();

	if (!ecb)
		return 0;

	size_t xSize = ecb->GetDataSize();
	if (xSize > dataSize)
		xSize = (size_t) dataSize;


	_fmemcpy(data, ecb->GetBuffer(), xSize);
	CIpx16Address& ipxa = (CIpx16Address&)na;

	ipxa.SetDgAddress(ecb->GetDgAddress());
	
	DropEcb(ecb);

	return xSize;
	
}

CNetAddress* CIpx16Link::GetRemoteAddress()
{
	if (m_remoteAddr)
	{
		m_remoteAddr->Ref();
	}
	return m_remoteAddr;
}


void   CIpx16Link::OnReceiveComplete(CEcb* ecb)
{

	if (!ecb->GetDataSize())
	{
		VPTRACE(("Cixp16Link:OnReceveComplete - Dropping empty ECB %08lx", ecb));
		DropEcb(ecb);
		return;
	}	
	VPTRACE(("Cixp16Link:OnReceveComplete - Enqueing ECB %08lx", ecb));

 	m_inQ.Append(ecb);
}





CEcbAlloc::CEcbAlloc()
{
}

CEcbAlloc::~CEcbAlloc()
{
	CEcb*ecb;

	while(NULL != (ecb = m_freeList.RemoveFirst()))
	{
		delete ecb;
	}

}



LPVOID AllocPageLockedBuffer(DWORD bytes)
{
	HGLOBAL h = GlobalAlloc(GPTR, bytes);
	
	if (h)
	{
		GlobalPageLock(h);
		return GlobalLock(h);
	}
	
	return NULL;
}

void FreePageLockedBuffer(LPVOID p)
{
	HGLOBAL h = (HGLOBAL) SELECTOROF(p);
	 
	GlobalPageUnlock(h);
	GlobalUnlock(h);
	
	GlobalFree(h);
}


void* CEcb::operator new(size_t s)
{
	return AllocPageLockedBuffer(s);
}

void CEcb::operator delete(void* p)
{
    FreePageLockedBuffer(p);
}

	
CEcb*  CEcbAlloc::New()
{
	CEcb* ecb = m_freeList.RemoveFirst();

	if (!ecb)
	{
		ecb = new CEcb;

	 	if (!ecb)
			return NULL;
	}

	memset(&ecb->m_ecb, 0, sizeof(ecb->m_ecb));
	memset(&ecb->m_hdr, 0, sizeof(ecb->m_hdr));
	ecb->m_ecb.fragmentCount = 1;
	ecb->m_ecb.fragmentDescriptor[0].address = &ecb->m_hdr;
	ecb->m_ecb.fragmentDescriptor[0].size = sizeof(ecb->m_hdr);
	
	ecb->m_window = NULL;
	ecb->u.m_link = NULL;

	return ecb;
}


void CEcbAlloc::Free(CEcb* ecb)
{
	VPASSERT(ecb->IsLost());
	m_freeList.Append(ecb);
}	


CIpxBufAlloc::CIpxBufAlloc()
{
}


CIpxBufAlloc::~CIpxBufAlloc()
{
}


	
LPVOID  CIpxBufAlloc::New(DWORD size)
{
	return AllocPageLockedBuffer(size);
}


void CIpxBufAlloc::Free(LPVOID buf)
{
	  if (buf)
	  	FreePageLockedBuffer(buf);
}
	
