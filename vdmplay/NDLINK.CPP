#include "stdafx.h"

#include "tdlog.h"
#include "datagram.h"
#include "datalog.h"
#include "dlinki.h"
#include "commport.h"
#include "frcoder.h"

#include "ndlink.h"


void CNDataLink::OnTimer()
{
	if (m_frameTimer)
	{
		if ((GetCurrentTime() - m_frameTimer) > m_frameAckDelay)
			OnFrameTimeout();
	}

	if (m_hostTimer)
	{
		if ((GetCurrentTime() - m_hostTimer) > m_hostDelay)
			OnHostTimeout();
	}

}




void CNDataLink::StopFrameTimer(SeqNr frame)
{
	if (!m_nBuffered)
	{
		m_frameTimer = 0;
	}
	else  
	{
		// we have some more frames buffered, 
		// see if they need timeout too
		inc(frame);
		int x = frame % m_nBufs;
		m_oldestFrame = m_outSeq[x];
		m_frameTimer = m_outBuf[x]->m_time;
	}
}

void CNDataLink::StartFrameTimer(SeqNr frame)
{
	m_outBuf[frame]->m_time = GetCurrentTime();
	if (!m_frameTimer)
	{
		m_frameTimer = 	m_outBuf[frame]->m_time;
		m_oldestFrame = m_outSeq[frame];
	}
}


		
	

void CNDataLink::SendDatagram(Datagram* d)
{

	m_nBuffered++;
	m_outBuf[m_nextFrameToSend % m_nBufs] = d;
	SendFrame(DATAFRAME, m_nextFrameToSend);
	inc(m_nextFrameToSend);

	if (m_nBuffered >= m_nBufs)
		DisableHost();

}


void CNDataLink::OnHostTimeout()
{
	SendFrame(ACKFRAME, 0);
}


void CNDataLink::OnFrameTimeout()
{

	if (m_resends < m_resendLimit)
	{
		m_resends++;
		SendFrame(DATAFRAME, m_oldestFrame);
		OnIncomingData();   // Poll for incoming data
	}

	VPTRACE(("CNDataLink::OnFrameTimout - Calling Become Broken\n"));	
	BecomeBroken();

	if (m_linkProc)
	{
		m_linkProc(this, LINKDOWN, m_eventData);
	}

}

void CNDataLink::OnIncomingFrame(CDatagram* d)
{

	if (d->MoreSize() < sizeof(frameHeader))
	{
		Log("Frame to short");
		delete dg;
	}

	IDataFrame fr(d->MoreData());

	d->AddOfset(sizeof(frameHeader));

	if (fr.Size() != d->MoreSize())
		return;
	
	SeqNr  x;
	if (fr.Kind() == DATAFRAME)
	{
		if ((fr.Seq() != m_frameExpected) && m_noNak)
			SendFrame(NAKFRAME, 0);


		x = fr.Seq() % m_nBufs;

		if (between(m_frameExpected, fr.Seq() , m_tooFar) &&
			!m_inBuf[x])
		{
			
			m_inBuf[x] = d;
			x = m_frameExpected % m_nBufs;
			while(m_inBuf[x])
			{
				ToHost(m_inBuf[x]);
				m_inBuf[x] = NULL;
				m_noNak = TRUE;
				inc(m_frameExpected);
				inc(m_tooFar);
				StartHostTimer();
			}
		}
	}

	x = (fr.Ack()+1) % (m_maxSeq + 1);
	if ((fr.Kind() == NAKFRAME) &&
		between(m_ackExpected, x, m_nextFrameToSend))
	{
		SendFrame(DATAFRAME, x);
	}

	while(between(m_ackExpected, fr.Ack(), m_nextFrameToSend))
	{
		x = m_ackExpected % m_nBufs;
		m_nBuffered--;
		StopFrameTimer(x);
		delete m_outBuf[x];
		m_outBuf[x] = NULL;
		inc(m_ackExpected);
	}

	if (m_nBuffered < m_nBufs)
		EnableHost();

}


void CNDataLink::SendFrame(int fKind, SeqNr fNumber)
{
	CDataFrame fr;
	int x = fNumber % m_nBufs;
	
	fr.SetKind(fKind);
	
	if (fKind == DATAFRAME)
	{
		Datagram* d = m_outBuf[x];
		fr.SetInfo(d->MoreData(), d->DataSize());
	}

	fr.SetSeq(fNumber);
	fr.SetAck((m_frameExpected + m_maxSeq) % (m_maxSeq+1));
	if (fKind == NAKFRAME)
		m_noNak = FALSE;

	TransmitFrame(fr);

	if (fKind == DATAFRAME)
		StartFrameTimer(x);

	StopHostTimer();
}

#define DL_DELIM  0x7F
#define DL_ESC    0x7E
#define DL_MASK   0x10
#define DL_FIN    0x7D

CNDataLink::TransmitFrame(CDataFrame& fr)
{
	unsigned char* p = m_outBuf;
	
	*p++ = DL_DELIM;
	p += EncodeChunk(p, &fr.m_hdr, sizeof(m_hdr));
	p += EncodeChunk(p, fr.m_info, m_infoSize));
	*p++ = DL_END;

	m_port->Write(m_outBuf, m_outBuf-p);
}

size_t CNDataLink::EncodeChunk(unsigned char* p, LPVOID data, size_t dataSize)
{
	size_t len=0;
	unsigned char* sp = (unsigned char* ) data;

	while(dataSize--)
	{
		if (*sp == DL_ESC || *sp == DL_DELIM || *sp == DL_FIN)
		{
			*p++ = DL_ESC;
			*p++ = *sp++ ^ DL_MASK;
			len += 2;
		}
		else
		{
			*p++ = *sp++;
			len++;
		}
	}
	return len;
}
		

void CNDataLink::DecodeInput()
{


	inData = m_inDataQueue.First();


	while(inData)
	{
		if (!m_curentDg)
		{
			m_currentDg = new Datagram(1024);
		}
		if (!m_currentDg)
			return NULL;
		
		unsigned char* ip = (unsigned char*) inData->MoreData();
		size_t iSize = inData->DataSize();
		size_t oCount = m_currentDg->m_size;
		unsigned char* op = oCount + (unsigned char*) m_currentDg->m_buf;

		while(iSize--)
		{
			if (oCount > m_currentDg->m_bufSize)
			{
				Log("Frame to long");
				m_currentDg->m_size = 0;
				break;
			}

			if (oCount == 0)
			{
				// we need to look for frame start
				if (*ip++ == DL_DELIM)
				{
					*op++ = DL_DELIM;
					m_currentDg->m_size++;
					m_decodeEsc = FALSE;
					continue;
				}
			}

			if (m_decodeEsc)
			{
				*op++ = *ip++ ^ DL_MASK;
				m_decodeEsc = FALSE;
				m_currentDg->m_size++;
				continue;
			}

			switch(*ip)
			{
				case DL_ESC:
					m_decodeState = TRUE;
					ip++;
					continue;
				case DL_DELIM:
					// we've missed something
					Log("Unxpected start of frame");
					m_currentDg->m_size = 0;
					goto nextDg;				
					break;
				case DL_FIN:
					m_currentDg->AddOffset(1);    // skip the DL_DELIM
					m_inDgQueue.Append(m_currentDg);

					m_currentDg = NULL;
					goto nextDg;
				default:
					*op++ = *ip++;
					oCount++;
					break;
			}
		}
nextDg:
		inData->AddOffset(inData->DataSize() - iSize);
		if (!inData->MoreData())
		{
			inDataQueue.RemoveFirst();
			delete inData;
		}
		inData = inDataQueue.First();
	}
}
