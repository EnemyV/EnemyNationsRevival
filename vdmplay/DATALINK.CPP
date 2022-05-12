#include "stdafx.h"

#include "tdlog.h"
#include "datagram.h"
#include "datalog.h"
#include "datalink.h"
#include "commport.h"
#include "frcoder.h"

#define USE_WINDOW

CCommPacket::CCommPacket(LPVOID data, DWORD size) : 
 m_data(NULL), m_hdr(NULL), m_ownBuffer(FALSE)
{
#ifdef USE_VPMEM
	LPSTR p = (LPSTR) vpAllocMem(size + sizeof(*m_hdr));
#else
	LPSTR p = new char[size + sizeof(*m_hdr)];
#endif

	if (!p)
		return;

	m_hdr = (pkthdr*) p;
	m_hdr->soh = SOH;
	m_hdr->ack = 0;
	m_hdr->siz = (WORD) (sizeof(*m_hdr) + size);
	m_data = (LPVOID) (m_hdr + 1);
	memcpy(m_data, data, (size_t)size);

	m_ownBuffer = TRUE;
}

CCommPacket::~CCommPacket()
{
#ifdef USE_VPMEM
	if (m_ownBuffer) 
			vpFreeMem(m_hdr, m_hdr->siz);
#else
	if (m_ownBuffer) 
			delete [] (LPSTR)m_hdr;

#endif
}



static WORD DoCheckSum(register LPCSTR buf, WORD size, WORD seed=0)
{
	register WORD sum = seed;
	
	while(size--)
		sum += *buf++;
	return sum;
}

WORD CCommPacket::DataCheckSum(WORD seed) const
{
	return DoCheckSum((LPCSTR) m_data, DataSize(), seed);
}


WORD CCommPacket::CheckSum() const
{
	return DataCheckSum(DoCheckSum((LPCSTR) m_hdr, sizeof(*m_hdr)));
}



CDataLink::CDataLink(CCommPort* port, CTDLogger* log) :
   m_errLog(log), m_dataLog(NULL),
   m_port(port),
   m_inIndex(0), m_readIndex(0), m_pktStart(0), m_linkState(BROKEN),
   m_ackDelay(TIMEOUT_WHEN_DISCONNECTED), 
   m_resends(0), m_resendLimit(10), m_timer(0),
   m_inPktNum(0), m_outPktNum(0),
   m_packetState(SEEKSOH), m_linkProc(NULL), m_eventData(NULL),
   m_encoder(NULL), m_winSize(4), m_error(0), m_hostIsOff(FALSE)
{
	
	m_inBuffer = new char[PKTBUFSIZE];

	if (!m_inBuffer)
		FatalError(VPNET_ERR_NOMEM);
	
	if (vpFetchInt("VDMPLAY", "UseEncoder", 1))
	{
		VPTRACE(("Datalink using encoder"));
		 m_encoder = new CFrameEncoder(port);
		 if (m_encoder && !m_encoder->Ok())
		 { 
			 delete m_encoder;
			 m_encoder = NULL;
		 }
	} 
	m_winSize = vpFetchInt("COMM", "WinSize", 4);
	if (!m_winSize)
		m_winSize = 4;

	m_resendLimit = vpFetchInt("COMM", "ResendLimit", 10);
	if (!m_resendLimit)
		m_resendLimit=10;
}


CDataLink::~CDataLink()
{
	FlushOutQueue();

 	delete m_inBuffer;	
	delete m_encoder;
}

void CDataLink::FlushOutQueue()
{
 	CCommPacket* p;
	
	while(NULL != (p = m_outQ.RemoveFirst()))
	{
		delete p;
	}
	
	
}

BOOL CDataLink::BuildAndSendPacket(LPVOID data, 
						  DWORD dataSize, 
						  char pktType)
{

	if (m_error)
		return FALSE;

	// if we're are not detected the other side and it's an unsfae data simply ignore it
	BOOL connected = DataLinkReady();

	if (!connected && (pktType != CTL))
		return TRUE;

	CCommPacket*  pkt = new CCommPacket(data, dataSize);
	BOOL ret = TRUE;
	
	if  (!pkt || !pkt->Ok())
	{
		Log("CDataLink::BuildAndSendPacket - packet creation failure");
		SetError(VPNET_ERR_NOMEM);
		return FALSE;
	}


	pkt->m_hdr->num = (BYTE) m_outPktNum++;
	pkt->m_hdr->ack = ZAK;
	pkt->m_hdr->typ = pktType;
	pkt->PreparePacket();
#ifndef USE_WINDOW
	m_outQ.Append(pkt);

	if (m_outQ.Count() == 1)  
	{
		// if it is the only packet we can transmit it and wait for ack
		if (TransmitPacket(pkt))
		{
			m_resends = 0;
			StartAckTimer();
			ret = TRUE;
		}

	}
#else
	ret = TRUE;
	m_waitQ.Append(pkt);
	if (m_waitQ.Count() > m_winSize)
		SuspendHost();

	FillOutputWindow();
#endif

	return ret;
}

void CDataLink::FillOutputWindow()
{
	CCommPacket* p;

	while(m_waitQ.Count() && (m_outQ.Count() < m_winSize))
	{
		p = m_waitQ.RemoveFirst();
		m_outQ.Append(p);
		TransmitPacket(p);
	}

	if (m_outQ.Count())
	{
		p = m_outQ.First();
		m_firstUnacked = p->m_hdr->num;
		m_timer = p->m_time;
	}

	if (m_waitQ.Count() < m_winSize/2)
		ResumeHost();
}



BOOL CDataLink::ResendPacket()
{

	if (!m_outQ.Count())
		return TRUE;




	if (m_resends < m_resendLimit)
	{
		m_resends++;
		TransmitPacket(m_outQ.First());
		StartAckTimer();
		OnIncomingData();   // Poll for incoming data
		return TRUE;
	}

	VPTRACE(("CDataLink::ResendPacket - Calling Become Broken\n"));	
	BecomeBroken();

	if (m_linkProc)
	{
		m_linkProc(this, LINKDOWN, m_eventData);
	}

	return FALSE;
}


BOOL  CDataLink::FeedBuffer()
{

  	VPASSERT(m_port);

#ifdef NEW_DATALINK

	m_encoder->Decode(d);

#else

	
	if (m_encoder)
	{
#if 1
		m_inIndex = m_readIndex = 0;
		size_t count = m_encoder->DecodeFrame(m_inBuffer, 1024);

		if (count)
		{
			AdvanceFreeBufferPosition(count);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
#else

		size_t bs = FreeBufSize();
		char*  d = (char*) FreeBufferPosition();
		int  ch;
		int count = 0;

		while(bs)
		{
			ch = m_encoder->GetChar();
			if (ch == -1)
			{
				break;
			}
			*d++ = ch;
			count++;
			bs--;
		}

		AdvanceFreeBufferPosition(count);
		return count? TRUE : FALSE;
#endif
	}


	DWORD s = m_port->Read(FreeBufferPosition(), FreeBufSize());

	if (s == 0)
		return FALSE;

	AdvanceFreeBufferPosition(s);
#endif

	return TRUE;
}


DWORD CDataLink::ReadLinkData(LPVOID buf, DWORD bufSize)
{
	DWORD size;
	
	if (!m_inQ.Count())
		return 0;


	Datagram* d = m_inQ.Get();
	size = d->DataSize();
	if (bufSize > size)
		bufSize = size;

	if (bufSize < size)
	{
		VPTRACE(("CDataLink::ReadLinkData reading %d bytes into %ld buffer", 
			size, bufSize));
	}
	memcpy(buf, d->MoreData(), (size_t) bufSize);


	delete d;

	return bufSize;
}



BOOL CDataLink::SeekSoh()
{

	while(m_readIndex < m_inIndex)
	{
		if (m_inBuffer[m_readIndex] == SOH)
		{
			m_pktStart = m_readIndex;
			return TRUE;
		}

		m_readIndex++;
	}

	return FALSE;
}

	
void CDataLink::StartAckTimer()
{
	m_timer = GetCurrentTime();
}

void CDataLink::StopAckTimer()
{
	m_timer = NULL;
}




void CDataLink::OnTimer()
{
	
	if (AckTimerExpired())
	{
		switch(m_linkState)
		{
			case CTLARCVD:
			case CTLSENT:
				SendControlPacket(CTL, m_outPktNum++);
				StartAckTimer();
				break;

			case CTLASENT:
				SendControlPacket(CTL, m_outPktNum++);
				StartAckTimer();
				m_linkState = CTLSENT;
				break;
				
			case CONNECTED:
				ResendPacket();
				break;
			default:
				break;
		}
	}
	
}


BOOL CDataLink::OnIncomingData()
{
 	BOOL ret = FALSE;

	VPENTER(OnIncomingData);
	// Read the data in our buffer
	if (!FeedBuffer())
	{
		VPLEAVEBOOL(OnIcomingData, FALSE);
		return FALSE;
	}



	while( m_readIndex < m_inIndex)
	{	
		switch(m_packetState)
		{
			
			case SEEKSOH:   // looking for beginning of the packet
				if (!SeekSoh())
				{
					VPTRACE(("Couldn't find SOH"));
					DiscardIncomingPacket();
					break;
				}
		    // fall down
			case READINGDATA:  // got packet start, waiting for the rest of data
				if (IncomingPacketComplete())
				{
					if (!CheckIncomingPacket())
					{
						VPTRACE(("Damaged Input Packet"));
						DiscardIncomingPacket();
						OnDamagedInputPacket();
						m_packetState = SEEKSOH;
						break;
					}
					// we've got a valid packet
					ret = TRUE;
					m_packetState = SEEKSOH;
					VPTRACE(("Valid Input Packet"));
					OnValidInputPacket();
				
				}
				else
				{
					VPTRACE(("Input Packet incomplete"));
					m_readIndex = m_inIndex;
					m_packetState = READINGDATA;
				}

				break;
			default:
				VPASSERT(0 == "Invalid instate in CCommLink");
				break;
		}
	}

	AdjustReadIndex();
	VPLEAVEBOOL(OnIcomingData, ret);
	return ret;
}


void CDataLink::OnDamagedInputPacket()
{
//	SendAck(NAK, 0);
}

void CDataLink::AdjustReadIndex()
{
	if (m_readIndex >= m_inIndex)
	{
		m_pktStart = m_readIndex = m_inIndex = 0;
	}
}


void CDataLink::SuspendHost()
{
	m_hostIsOff = TRUE;
	m_linkProc(this, STOPFLOW, m_eventData);
}

void CDataLink::ResumeHost()
{
	if (m_hostIsOff)
		m_linkProc(this, RESUMEFLOW, m_eventData);
	m_hostIsOff  = FALSE;

}


static char* stnames[] = { "CTLSENT", "CTLASENT", "CTLARCVD", "CONNECTED", "BROKEN" };

void CDataLink::BreakLink()
{
	SendControlPacket(CTLX, m_outPktNum++);
	BecomeBroken();
	FlushOutQueue();
    if (m_linkProc)
	{
		m_linkProc(this, LINKDOWN, m_eventData);
	}
}

void CDataLink::OnControlPacket(CCommPacket& pkt)
{

	m_inPktNum = pkt.m_hdr->num;

#ifdef USE_WINDOW
	m_inPktNum++;
	m_inPktNum &= 0xff;
#endif

	if (pkt.m_hdr->typ == CTLX)
	{
		BreakLink();
		return;
	}


	switch(m_linkState)
	{
		case CTLSENT:
		  if (pkt.m_hdr->typ == CTL)
		  {
				SendControlPacket(CTLACK, m_outPktNum++);
				m_linkState = CTLASENT;
				break;
		  }

		  if (pkt.m_hdr->typ == CTLACK)
		  {
			  VPTRACE(("OnCtlPacket:: Becoming Connected"));  	
 			  SendControlPacket(CTLACK, m_outPktNum++);
		   	  StopAckTimer();
			  BecomeConnected();
			  if (m_linkProc)
			  {
				m_linkProc(this, LINKUP, m_eventData);
			  }

		  	  break;
		  }

		  			
		  break;

		case CTLASENT:
		  if (pkt.m_hdr->typ == CTL)
		  {
				SendControlPacket(CTLACK, m_outPktNum++);
				m_linkState = CTLASENT;
				break;
		  }

		  if (pkt.m_hdr->typ == CTLACK)
		  {
			  VPTRACE(("OnCtlPacket:: Becoming Connected"));  	
		   	  StopAckTimer();
			  BecomeConnected();
			  if (m_linkProc)
			  {
				m_linkProc(this, LINKUP, m_eventData);
			  }
		  }
	  	  break;

	    case CTLARCVD:
		    if (pkt.m_hdr->typ == CTL)
		    {
				SendControlPacket(CTLACK, m_outPktNum++);
 				m_linkState = CTLASENT;
				break;
#if 0
		        VPTRACE(("OnCtlPacket:: Becoming Connected")); 
		   	    StopAckTimer();
			    m_linkState = CONNECTED;
			    if (m_linkProc)
			    {
					  m_linkProc(this, LINKUP, m_eventData);
			    }
#endif
		    }
		    break;

		case BROKEN:
	  		break;

		case CONNECTED:
		    if (pkt.m_hdr->typ == CTL)
		    {
				SendControlPacket(CTLACK, m_outPktNum++);
				StopAckTimer();
				break;
			}


			
			break;
	}

}

#ifdef NEW_DATALINK
void CDataLink::OnValidInputPacket()
{
	char ackCode = 0;
	BOOL ignoringPacket = FALSE;

	CDatagram* dg = m_encoder->InPacket();
	
	if (dg->MoreSize() < sizeof(pkthdr))
	{
		VPTRACE(("OnValidInput packet: packet too short"));
		delete dg;
		return;
	}
	
	CCommPacket pkt(dg);
	dg->AddOffset(sizeof(pkthdr));

	VPTRACE(("GotPacket: %c num = %d size=%d, in state %s", pkt.m_hdr->typ, pkt.m_hdr->num, pkt.m_hdr->siz, stnames[m_linkState]));
	LogIncomingPacket(pkt.m_hdr->typ, pkt.m_hdr->num, pkt.m_hdr->siz);

	if (pkt.IsControlPacket())
	{
		OnControlPacket(pkt);
		delete dg;
		return;
	}

	if (m_linkState != CONNECTED)
	{
		return;
	}
	
	BOOL hasData = pkt.ContainsData();
	BOOL inQueue = FALSE;
	
	if (hasData)
	{
		ackCode = ACK;

		if (pkt.m_hdr->num == (BYTE) m_inPktNum)
		{
			m_inPktNum++;
			m_inQ.Add(dg);
			inQueue = TRUE;
		}
		else
		{
			ignoringPacket = TRUE;
			// the packet is out of sequence.
			// we send ACK with the SEQ number of last good packet we've got
			pkt.m_hdr->num = (BYTE) (m_inPktNum-1);
		}
	}

	if (m_timer)
	{
		if (pkt.ContainsAck())
		{
			
			if (between((BYTE) m_firstUnacked, pkt.m_hdr->num, (BYTE)
				(m_firstUnacked+m_winSize)))
			{

				CCommPacket* d = m_outQ.RemoveFirst();
				while(d)
				{
					delete d;
					if (d->m_hdr->num == pkt.m_hdr->num)
						break;
					d = m_outQ.RemoveFirst();
				}

				m_resends = 0;
				if (m_outQ.Count())
				{
					d = m_outQ.First();
					m_firstUnacked = d->m_hdr->num;
					m_timer = d->m_time;
				}
				else
				{
					m_firstUnacked = 0;
					StopAckTimer();
				}

			}

			
		}
		else if (hasData && pkt.ContainsNak())
		{
			ackCode = ZAK;
		}
			// we need retransmit
	}
	
	
	if (ackCode)
	{

#if 0	
		if (m_outQ.Count())
		{
			PiggyBackAck(ackCode);
		}
		else
#endif
		{
			SendAck(ackCode, pkt.m_hdr->num);
		}
	}

	FillOutputWindow();
		


	if (!ignoringPacket)
	{
		if (m_linkProc)
			m_linkProc(this, pkt.m_hdr->typ == SDATA ? LINKSDATA : LINKUDATA, m_eventData);
	}

	if (!inQueue)
		delete dg;

}



#else

void CDataLink::OnValidInputPacket()
{
	char ackCode = 0;
	BOOL ignoringPacket = FALSE;

	CCommPacket pkt(InPacket());

	VPTRACE(("GotPacket: %c num = %d size=%d, in state %s", pkt.m_hdr->typ, pkt.m_hdr->num, pkt.m_hdr->siz, stnames[m_linkState]));
	LogIncomingPacket(pkt.m_hdr->typ, pkt.m_hdr->num, pkt.m_hdr->siz);

	if (pkt.IsControlPacket())
	{
		OnControlPacket(pkt);
		m_readIndex = m_pktStart + pkt.Size();
		AdjustReadIndex();

		return;
	}

	if (m_linkState != CONNECTED)
	{
		return;
	}
	
	BOOL hasData = IncomingPacketContainsData();
	
	if (hasData)
	{
		ackCode = ACK;

#ifndef USE_WINDOW
		if (pkt.m_hdr->num != (BYTE) m_inPktNum)
		{
			m_inPktNum = pkt.m_hdr->num;
#else
		if (pkt.m_hdr->num == (BYTE) m_inPktNum)
		{
			m_inPktNum++;
#endif
			Datagram* d = new Datagram((char*) pkt.Data(), pkt.DataSize());
			if (!d)
			{
				// we have no space for the packet, simply drop it, in the hope that the application
				// will consume the incoming data and relase some space
				m_readIndex = m_pktStart + pkt.Size();
				AdjustReadIndex();
				return;
			}

		
			m_inQ.Add(d);
		}
		else
		{
			ignoringPacket = TRUE;
#ifdef USE_WINDOW
			// the packet is out of sequence.
			// we send ACK with the SEQ number of last good packet we've got
			pkt.m_hdr->num = (BYTE) (m_inPktNum-1);
#endif
		}
	}

	if (m_timer)
	{
		if (IncomingPacketContainsAck())
		{
			
#ifndef USE_WINDOW
			
			// the first packet in the out queue has been acknowledged
			// destroy its copy waiting for eventual retransmission
			CCommPacket* d = m_outQ.RemoveFirst();
			if (d->m_hdr->num == pkt.m_hdr->num)
			{
				delete d;
				StopAckTimer();
			}
			else
				m_outQ.Insert(d);
#else
			if (between((BYTE) m_firstUnacked, pkt.m_hdr->num, (BYTE)
				(m_firstUnacked+m_winSize)))
			{

				CCommPacket* d = m_outQ.RemoveFirst();
				while(d)
				{
					delete d;
					if (d->m_hdr->num == pkt.m_hdr->num)
						break;
					d = m_outQ.RemoveFirst();
				}

				m_resends = 0;
				if (m_outQ.Count())
				{
					d = m_outQ.First();
					m_firstUnacked = d->m_hdr->num;
					m_timer = d->m_time;
				}
				else
				{
					m_firstUnacked = 0;
					StopAckTimer();
				}

			}
#endif

			
		}
		else if (hasData && IncomingPacketContainsNak())
		{
			ackCode = ZAK;
		}
			// we need retransmit
	}
	
	
	if (ackCode)
	{

#if 0	
		if (m_outQ.Count())
		{
			PiggyBackAck(ackCode);
		}
		else
#endif
		{
			SendAck(ackCode, pkt.m_hdr->num);
		}
	}

#ifndef USE_WINDOW
	if (!m_timer && m_outQ.Count())
	{
		TransmitPacket(m_outQ.First());
		m_resends = 0;
		StartAckTimer();
	}
#else
	FillOutputWindow();
#endif
		


	m_readIndex = m_pktStart + pkt.Size();
	AdjustReadIndex();

	if (!ignoringPacket)
	{
		if (m_linkProc)
			m_linkProc(this, pkt.m_hdr->typ == SDATA ? LINKSDATA : LINKUDATA, m_eventData);
	}

}
#endif


void CDataLink::SendAck(char ack, char pktNum)
{
	pkthdr hdr;


	hdr.soh = SOH;
	hdr.ack = ack;
	hdr.typ = UDATA;
	hdr.siz = sizeof(hdr);
	

	CCommPacket pkt(&hdr, NULL);

	pkt.m_hdr->num = pktNum;
	pkt.PreparePacket();
	

	TransmitPacket(&pkt);
	
}

void CDataLink::SendControlPacket(char typ, unsigned pktNum)
{
	pkthdr hdr;


	hdr.soh = SOH;
	hdr.ack = ZAK;
	hdr.typ = typ;
	hdr.siz = sizeof(hdr);
	

	CCommPacket pkt(&hdr, NULL);

	pkt.m_hdr->num = (char) pktNum;
	pkt.PreparePacket();
	

	TransmitPacket(&pkt);
	StartAckTimer();
	
}

void CDataLink::Start()
{
	VPENTER(CDataLink::Start);	

	if (m_linkState == BROKEN)
	{
		m_outPktNum = 0;
		m_inPktNum = 0;
		SendControlPacket(CTL, m_outPktNum++);
		m_linkState = CTLSENT;
		StartAckTimer();
	}

	VPLEAVE(CDataLink::Start);	

}

void CDataLink::LogOutgoingPacket(char c, UINT num, UINT size)
{
	char buf[128];

 	if (m_dataLog)
	{
		wsprintf(buf, "OUT: Type=%c Num=%d size=%d state=%s\n", c, num, size, stnames[m_linkState]);

		m_dataLog->OutgoingData(buf, strlen(buf));
	}
}

void CDataLink::LogIncomingPacket(char c, UINT num, UINT size)
{
	char buf[128];

 	if (m_dataLog)
	{
		wsprintf(buf, "IN : Type=%c Num=%d size=%d state=%s\n", c, num, size, stnames[m_linkState]);

		m_dataLog->IncomingData(buf, strlen(buf));
	}
}

BOOL CDataLink::TransmitPacket(CCommPacket* pkt)
{
	VPASSERT(m_port);

	VPTRACE(("TransmitPacket: %c num = %d size=%d, in state %s", pkt->m_hdr->typ, pkt->m_hdr->num, pkt->m_hdr->siz, stnames[m_linkState]));
	LogOutgoingPacket(pkt->m_hdr->typ, pkt->m_hdr->num, pkt->m_hdr->siz);

	pkt->m_time = GetCurrentTime();
	if (m_encoder)
	{ 
		m_encoder->SendData(pkt->WholePacket(), pkt->Size());
		return TRUE;
	} 

	m_port->Write(pkt->WholePacket(), pkt->Size());

	return TRUE;
}


void CDataLink::PiggyBackAck(char ack)
{
	VPASSERT(m_outQ.Count());


	CCommPacket* pkt = m_outQ.First();

	if (pkt->m_hdr->ack != ack)
	{
		// we're changing the packet header so we need to recalc the checksum
		pkt->m_hdr->ack = ack;
		pkt->PreparePacket();
	}

	if (TransmitPacket(pkt))
	{
		StartAckTimer();
	}
	
}


 
