#include "stdafx.h"
#include "vpdemo.h"
#include "vpdemdlg.h"
#include "dxfer.h"
#include "vpxfer.h"
#include "loadtest.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

void vpFetchString (LPCSTR section, LPCSTR key, LPCSTR defVal, LPSTR pStr, int iLen);
UINT vpFetchInt(LPCSTR section, LPCSTR key, UINT defVal);
UINT vpFetchIntR(LPCSTR section, LPCSTR key, UINT defVal, UINT minVal, UINT maxVal);
void vpStoreInt(LPCSTR section, LPCSTR key, UINT val);
void vpStoreString(LPCSTR section, LPCSTR key, LPCSTR val);
void vpMakeIniFile(LPSTR fName);


CLoadTest::CLoadTest(VPSESSIONHANDLE ses, VPPLAYERID myId) :
m_ses(ses), m_done(FALSE), m_recvSeqNum(1), m_sendSeqNum(1), m_myId(myId)
{ 
	m_stopFlow = FALSE;
	srand((int) GetCurrentTime());

	m_dlg = (CVpdemoDlg*) AfxGetApp()->m_pMainWnd;
	m_packetSize = vpFetchIntR("VPDEMO", "PacketSize", 64, 64, VP_MAXSENDDATA);
	
}


void CLoadTest::Log(LPCSTR fmt, DWORD p1, DWORD p2)
{
	char buf[256];
	SYSTEMTIME tm;

	GetLocalTime(&tm);

	wsprintf(buf, "%2d:%2d:%2d:%4d - ", tm.wHour, tm.wMinute, 
		tm.wSecond, tm.wMilliseconds);
	wsprintf(buf+strlen(buf), fmt, p1, p2);

	m_dlg->Log(buf);
}

void CLoadTest::InitMsg(testMsg* msg)
{
	msg->i.seqnum = m_sendSeqNum++;
	msg->i.msgtime = GetCurrentTime();
	msg->i.type = TST_MSG;
	msg->i.reqtime = 0;
	msg->i.reqseq = 0;
	strcpy(msg->i.text,"TST");
}

void CLoadTest::SendMsg(testMsg* msg, VPPLAYERID to)
{
	int flags = VP_MUSTDELIVER;
	if (to == VP_ALLPLAYERS)
		flags |= VP_BROADCAST;

	if (!vpSendData(m_ses, to, m_myId,
		msg, m_packetSize, flags, 0))
	{
		Log("Errio sending msg seq: %ld", msg->i.seqnum);
		m_done = TRUE;
	}
}


void CLoadTest::OnFlowOff()
{
	m_stopFlow = TRUE;

	Log("LoadTest Flow OFF");
}

void CLoadTest::OnFlowOn()
{
	m_stopFlow = FALSE;

	Log("LoadTest Flow ON");
}

BOOL CLoadTest::FlowIsBlocked()
{
	MSG msg;

	if (m_stopFlow)
		return TRUE;

	if (PeekMessage(&msg, m_dlg->m_hWnd, WM_VPFLOWOFF, WM_VPFLOWOFF, PM_REMOVE))
		OnFlowOff();

	return m_stopFlow;
}





MasterLoadTest::MasterLoadTest(VPSESSIONHANDLE ses, VPPLAYERID id, int pCount) :
CLoadTest(ses, id)
{

	m_sendsPerTick = vpFetchInt("VPDEMO", "sendsPerTick", 1); 
	m_sendsPerBurst = vpFetchInt("VPDEMO", "sendsPerBurst", 25); 

	m_waitingAck = 0;
	m_reqTime = 0;
	m_nonAckCount = 0;
	m_playerCount = pCount;
	m_ackTimeout = vpFetchIntR("VPDEMO", "ackTimeout", 25000, 10, 60000);
	m_tickFreq = vpFetchIntR("VPDEMO", "LoadTestTimer", 100, 50, 1000);
	
	m_tickDivisor = vpFetchIntR("VPDEMO", "TickDivisor", 1, 1, 50) - 1;
	m_skippedTicks = 0;

}


static char packetBuf[1024];

void MasterLoadTest::GenerateMessages(int i)
{
	testMsg& msg = *(testMsg*) packetBuf;

	while(i--)
	{
		InitMsg(&msg);

		if (!m_waitingAck)
		{
			int rVal = rand();

			if (rVal < RAND_MAX/100)  
			{
				// we need 1/100 probabilty to generate
				// a request for ack
				msg.i.type = REQ_MSG;
				m_waitingAck = msg.i.seqnum;
				m_nonAckCount = m_playerCount;
				Log("Generating Req %ld", m_waitingAck);
				m_reqTime = GetCurrentTime();
			}
		}
		SendMsg(&msg);
	}

}

void MasterLoadTest::OnTimer()
{
	int rVal = rand();

	if (m_skippedTicks++ == m_tickDivisor)
	{
		m_skippedTicks = 0;

		if (rVal < RAND_MAX/300)
		{
			if (!FlowIsBlocked())
			{
				Log("Generating burst");
				GenerateMessages(m_sendsPerBurst);
			}

		}
		else
		{
			if (! FlowIsBlocked() )
				GenerateMessages(m_sendsPerTick);
		}
	}

	if (m_waitingAck)
	{
		if (GetCurrentTime() - m_reqTime  > m_ackTimeout)
		{
			Log("Timeout waiting for ack=%lu, nonAckPlayers=%ld",
				m_waitingAck, m_nonAckCount);
			m_waitingAck = 0;
		}
	}

}


BOOL MasterLoadTest::ProcessNotification(UINT nCode, LPVPMESSAGE msg)
{

	switch(nCode)
	{
		case VP_READDATA:
		{
			testInfo* i = (testInfo*) msg->u.data;
			
			if (i->type == ACK_MSG)
			{
				if (m_waitingAck == i->reqseq)
				{
					Log("Got ack to req %ld from player %ld", i->reqseq, msg->senderId);
					Log("Delay is %d ms", GetCurrentTime() - m_reqTime);
					if (!--m_nonAckCount)
					{
						m_waitingAck = 0;
						Log("Request completed");
					}
				}
				else
				{
					Log("Invalid ack to req %ld from player %ld", i->reqseq, msg->senderId);
				}

			}
			else if (i->type == END_MSG)
			{
				m_done = TRUE;
			}
			return TRUE;
		}
		case VP_SESSIONCLOSE:
			m_done = TRUE;
			return FALSE;

		case VP_LEAVE:
			Log("Player Gone");
			m_playerCount--;
			if (m_waitingAck)
			{
				if (!--m_nonAckCount)
				{
					m_waitingAck = 0;
					Log("Request %ld completed", m_waitingAck);
				}
			}
			return FALSE;
	}

	return FALSE;
}


SlaveLoadTest::SlaveLoadTest(VPSESSIONHANDLE ses, VPPLAYERID id)
: CLoadTest(ses, id)
{
}

BOOL SlaveLoadTest::ProcessNotification(UINT nCode, LPVPMESSAGE msg)
{

	switch(nCode)
	{
		case VP_READDATA:
		{
			testInfo* i = (testInfo*) msg->u.data;
			
			if (m_recvSeqNum != i->seqnum)
			{
				Log("Got invalid sequence %ld when expecting %ld",
					i->seqnum, m_recvSeqNum);
				m_done = TRUE;
				return TRUE;
			}

			if (0 == m_recvSeqNum % 100)
			{
			   Log("Got msg seq = %ld", m_recvSeqNum);
			}

			m_recvSeqNum++;

			if (i->type == REQ_MSG)
			{
				testMsg rpmsg;

				InitMsg(&rpmsg);
				rpmsg.i.type = ACK_MSG;
				rpmsg.i.reqseq = i->seqnum;
				rpmsg.i.reqtime = i->msgtime;
				SendMsg(&rpmsg, msg->senderId);
			}
			else if (i->type == END_MSG)
			{
				Log("Got END_MSG");
				m_done = TRUE;
			}
			return TRUE;
		}
		case VP_SESSIONCLOSE:
			m_done = TRUE;
			return FALSE;

		case VP_LEAVE:
			Log("Player Gone");
			return FALSE;
	}

	return FALSE;
}




