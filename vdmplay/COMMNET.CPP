#include "stdafx.h"
#include "vpnet.h"
#include "commport.h"
#include "datagram.h"
#include "datalink.h"
#include "datalog.h"
#include "commnet.h"


CCommNet::CCommNet(CTDLogger* log, CCommPort* port) : 
  CNetInterface(log), m_link(NULL), m_port(port), m_listening(FALSE), 
  m_waitingLink(FALSE), m_needCarrier(TRUE),
  m_lastCarrier(FALSE)
{
	m_dLink = new CDataLink(port, log);
	m_dLink->m_linkProc = OnLinkEvent;
	m_dLink->m_eventData = this;

}

		
CCommNet::~CCommNet()
{
	if (m_port)
		m_port->Close();
	delete m_dLink;
}


void CCommNet::OnLinkEvent(CDataLink* dLink, int event, LPVOID userData)
{
	CCommNet* net = (CCommNet*) userData;

	VPENTER(OnLinkEvent);

	switch(event)
	{
		case CDataLink::LINKUP:
			if ( net->m_listening )
			{
				VPTRACE(("OnLinkEvent - LINKUP"));
//				net->m_link->m_isSafe = TRUE;
//				net->OnAccept(net->m_link);
			}
			else if ( net->m_link->m_isSafe )
			{
				VPTRACE(("OnLinkEvent - LINKUP - CONNECTING"));
				net->OnConnect(net->m_link);
			}		

			break;

		case CDataLink::LINKDOWN:
			VPTRACE(("OnLinkEvent - LINKDOWN"));
			if (net->m_link->m_isSafe)
			{
				net->OnDisconnect(net->m_link);
				net->m_link->m_isSafe = FALSE;
			}
			break;

		case CDataLink::LINKSDATA:
			VPTRACE(("OnLinkEvent - LINKSDATA"));
			if (net->m_listening || net->m_link->m_isSafe)
			{
				if (!net->m_link->m_isSafe)
				{
					net->m_link->m_isSafe = TRUE;
					net->OnAccept(net->m_link);
				}
				else
				{
					if (net->m_link->m_connecting)
					{
						net->m_link->m_connecting = FALSE;
						net->OnConnect(net->m_link);
					}
				}
				net->OnSafeData(net->m_link);
				
			}
	   		break;

		case CDataLink::LINKUDATA:
			VPTRACE(("OnLinkEvent - LINKUDATA"));
			net->OnUnsafeData(net->m_link);
			break;

		case CDataLink::STOPFLOW:
			net->SuspendHost();
			break;

		case CDataLink::RESUMEFLOW:
			net->ResumeHost();
			break;
	}
	VPLEAVE(OnLinkEvent)
}



CCommLink* CCommNet::MakeLinkObj(char a)
{
	VPASSERT(m_port);
	
	if (!m_link)
	{
		m_link = new CCommLink(this, m_dLink, a);
		if (!m_link)
		{
			Log("CommNet: no memory for link object");
			SetError(VPNET_ERR_NOMEM);
		}

	}

	m_link->Ref();
	return m_link;
}

CNetLink*  CCommNet::MakeSafeLink(CNetAddress* addr, LPVOID userData)
{
	CCommLink* lnk = MakeLinkObj();

	lnk->m_isSafe = TRUE;
	lnk->m_connecting = TRUE;
	return lnk;
}

void CCommNet::CheckCarrier()
{
	BOOL cd = m_port->HasCarrier();
	if (m_lastCarrier != cd)
	{
		m_lastCarrier = cd;
	    if (cd)
		{
			if (!m_waitingLink && m_dLink->IsBroken())
				m_dLink->Start();
		}
		else
		{
			OnLinkDown();
		}
	}
}

void CCommNet::OnTimer() 
{
	if (m_port->IsOpen())
	{
		CheckCarrier();
		if (m_lastCarrier)
		{
			if (m_dLink) 
				m_dLink->OnTimer(); 

			if (m_link && m_link->m_connecting)
			{
				m_link->m_connecting = FALSE;
				OnConnect(m_link);
	
			}
		}
	}
}	 


BOOL CCommNet::Listen(BOOL streamListen, BOOL serverMode)
{
	VPENTER(CCommNet::Listen);
	VPASSERT(m_port);

	BOOL ret = FALSE;

	if (m_dataLog)
	{
		 m_dataLog->Start(m_port->m_deviceName);
	}

	if (!m_port->IsOpen())
	{
		if (!m_port->Open())
		{
			Log("CCommNet::Listen Can't open specified port");
			FatalError(VPNET_ERR_INIT);
			VPLEAVEBOOL(CCommNet::Listen, FALSE);
			return FALSE;
		}
	}
	
	DWORD brt = 100L*vpFetchInt("COMM", "BaudRate", 192);
	if (!m_port->Setup(brt))
    {
		Log("CCommNet::Listen Port setup error");
		FatalError(VPNET_ERR_INIT);
		VPLEAVEBOOL(CCommNet::Listen, FALSE);
		return FALSE;
    }

	MakeUnsafeLink();

	if (!m_link)
	{
		VPLEAVEBOOL(CCommNet::Listen, FALSE);

		return FALSE;
	}

	m_link->Unref();

	if (streamListen)
	{
		m_listening	= TRUE;
#if 0
		if (m_dLink->DataLinkReady())
		{
			m_link->m_isSafe = TRUE;
			OnAccept(m_link);
		}
#endif
	}

	if (m_dataLog)
	{
		 m_dataLog->SetCallback(CancelCbk, this);
	}

	if (!m_needCarrier)
	{
		VPLEAVEBOOL(CCommNet::Listen, TRUE);
		return TRUE;
	}

	if (!WaitCarrier())
	{
		VPLEAVEBOOL(CCommNet::Listen, FALSE);
		return FALSE;
	}

	if (m_port->HasCarrier())
	{
		if (!m_dLink->DataLinkReady())
		{
			m_port->Flush();
			StartDataLink();
		}

		ret = WaitDataLink();
	}
					


//	m_link->Unref();
	VPLEAVEBOOL(CCommNet::Listen, ret);

	return ret;
}

void CCommNet::CancelCbk(LPVOID p)
{
	if (!p)
		return;


	CCommNet* mNet =  (CCommNet*) p;

	mNet->OnCancel();
}

void CCommNet::OnCancel()
{
	vpDoAbortWait();
	m_waitingLink = FALSE;
}


void CCommNet::StartDataLink()
{
	m_dLink->Start();
}


BOOL CCommNet::DataLinkTest(LPVOID context)
{
	CCommNet* net = (CCommNet*) context;

  if (!net->m_port->IsOpen())
	 net->m_waitingLink = FALSE;

	return !net->m_waitingLink || net->m_dLink->DataLinkReady();
}

BOOL CCommNet::CarrierTest(LPVOID context)
{
	CCommNet* net = (CCommNet*) context;

    if (!net->m_port->IsOpen())
	   net->m_waitingLink = FALSE;

	return !net->m_waitingLink || net->m_port->HasCarrier();
}

BOOL CCommNet::WaitDataLink()
{
	VPENTER(CCommNet::WaitDataLink);
	m_waitingLink = TRUE;

	BOOL ret = vpWaitLoop(DataLinkTest, this);

	if (ret)
		ret = m_dLink && m_dLink->DataLinkReady();

	VPLEAVEBOOL(CCommNet::WaitDataLink, ret);
	return ret;
}

BOOL CCommNet::WaitCarrier()
{
	VPENTER(CCommNet::WaitCarrier);

	if (!m_needCarrier)
	{
		VPLEAVEBOOL(CCommNet::WaitCarrier, TRUE);
		return TRUE;
	}

	m_waitingLink = TRUE;

	BOOL ret = vpWaitLoop(CarrierTest, this);

	if (ret)
		ret = m_port && m_port->HasCarrier();

	// make little delay
	DWORD ct = GetCurrentTime();
	while((GetCurrentTime() - ct) < 100)
		;

	VPLEAVEBOOL(CCommNet::WaitCarrier, ret);
	return ret;
}
  
  
void CCommNet::BecomeDeef()
{
	m_listening = FALSE;
}



void CCommNet::GetAddress(LPVPNETADDRESS addr)
{
	addr->machineAddress[0] = 0;
}
	

CNetAddress*  CCommNet::MakeAddress(LPCVPNETADDRESS addr)
{
	char ch = addr ? addr->machineAddress[0] : '1';
	return MakeAddressObj(ch);
}

CCommAddress*  CCommNet::MakeAddressObj(char ch)
{
	return new CCommAddress(ch);
}

void CCommNet::Cleanup()
{
	VPASSERT(m_port);
	CNetInterface::Cleanup();

//	m_port->Close();
}

			
void CCommNet::StartDataLog(CDataLogger* l)
{
	CNetInterface::StartDataLog(l);
	if (m_dLink)
	{
		m_dLink->m_dataLog = l;
	}
}

void CCommNet::StopDataLog()
{
	if (m_dLink)
		m_dLink->m_dataLog = NULL;

	CNetInterface::StopDataLog();
}


void CCommNet::OnLinkDown()
{
	if (m_link)
	{
		OnDisconnect(m_link);
		m_link->m_isSafe = FALSE;
	}
}
	



LRESULT CCommNet::OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != WM_COMMNOTIFY)
		return FALSE;

	if (!m_port)
		return TRUE;

	if (!m_dLink)
		return TRUE;

	VPASSERT(m_link);	
    
	m_port->ClearEvent();



	CheckCarrier();
#ifdef WIN32
	if ((lParam & EV_RXCHAR) && m_lastCarrier)
	{
		m_dLink->OnIncomingData();
	}
#else
	if ((lParam & (CN_EVENT|CN_RECEIVE)) && m_lastCarrier)
	{
		m_dLink->OnIncomingData();
	}
#endif    
	
		

	
#if 0
	if (lParam & EV_DSR)
	{
		if (m_port->HasDsr())
		{
			if (m_connectHook)
			{
				m_connectHook(m_link, NULL);
			}
		}	
		else if (m_disconnectHook)
		{
			m_disconnectHook(m_link, NULL);
		}
	}
		
#endif
		
   return TRUE;

}





	

CCommLink::CCommLink(CCommNet* net, CDataLink* dLink, char myAddr) :
   CNetLink(net, NULL), m_dLink(dLink), m_myAddr(myAddr),
   m_remoteAddr(0),
   m_isSafe(0), m_connecting(FALSE)
{
	
}


CCommLink::~CCommLink()
{
}



CNetAddress*  CCommLink::GetRemoteAddress()
{
 	return ((CCommNet*)m_net)->MakeAddressObj(m_remoteAddr);
}

BOOL CCommLink::SendTo(CNetAddress& to, 
			              LPVOID data, 
						  DWORD dataSize, 
						  DWORD flags)
{
	if (!m_dLink->SendData(data, dataSize, FALSE))
	{
		return FALSE;
	}
	
	return TRUE;
}


BOOL CCommLink::Send(LPVOID data, DWORD dataSize, DWORD flags)
{
	if (!m_isSafe)
	{
		VPTRACE(("CCommLink::Send - link is unsafe"));
		return FALSE;
	}
	return m_dLink->SendData(data, dataSize, TRUE);
}



	
DWORD CCommLink::Receive(LPVOID data, DWORD dataSize)
{
	if (!m_isSafe)
	{
		VPTRACE(("CCommLink::Receive - link is unsafe"));
		return FALSE;
	}
	return m_dLink->ReadLinkData(data, dataSize);
}


DWORD CCommLink::ReceiveFrom(LPVOID data, DWORD dataSize, CNetAddress& na)
{
	CCommAddress& ca = 	(CCommAddress&) na;

	ca.m_addr = m_remoteAddr;

	return m_dLink->ReadLinkData(data, dataSize);
}


DWORD CCommLink::HasData()
{
	return m_dLink->HasData();
}	   
