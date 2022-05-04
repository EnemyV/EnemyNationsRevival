
#include "stdafx.h"
#include "dxfer.h"
#include "vpxfer.h"


#define PACKETSIZE (VP_MAXSENDDATA - sizeof(VPMSGHDR))

CVPTransfer::CVPTransfer(VPSESSIONHANDLE h) :
  CDataTransfer(PACKETS_IN_WINDOW*PACKETSIZE), m_session(h)
{
	  m_peerPlayer = 0;
	  m_localPlayer = 0;
	  m_startTime = 0;
	  m_endTime = 0;
	  m_timeout = DEFAULT_TIMEOUT;
	  m_buffer = NULL;


	  m_chunkSize = PACKETSIZE;

}

 CVPTransfer::~CVPTransfer()
 {
	delete [] m_buffer;
 }


void CVPTransfer::OnTimer()
{
	if (!m_error)
	{
		DWORD t = GetCurrentTime();

		if (t - m_lastPacketTime > m_timeout)
		{
			SetError(ERR_TIMEOUT);
		}
	}
}

BOOL CVPTransfer::ProcessNotification(UINT nCode, LPVPMESSAGE msg)
{
	if (m_error)
		return FALSE;

	switch(nCode)
	{
		case VP_READDATA:
			if (m_sending)
			{
				if (msg->toId == m_localPlayer && 
					msg->senderId == m_peerPlayer)
				{
					// this is an ACK message
					DWORD amount = *(LPDWORD) msg->u.data;

					TRACE("Got ack: %u", amount);
					OnAck(amount);
					FillWindow();

					RecordPacketTime();
					return TRUE;
				}

				return FALSE;
			}
			// we're receiveing data
			if (msg->toId == m_localPlayer && msg->senderId == m_peerPlayer)
			{
				OnIncomingData(msg->u.data, msg->dataLen);

				m_lastPacketTime = GetCurrentTime();
				if (m_acked == m_totalSize)
					m_endTime = m_lastPacketTime;
				return TRUE;
			}
			return FALSE;

		case VP_SESSIONCLOSE:
			SetError(ERR_SESSIONLOST);
			return FALSE;

		case VP_LEAVE:
			if (m_localPlayer == msg->u.playerInfo->playerId)
			{
 				SetError(ERR_PLAYERDEAD);
			} 
			else if (m_peerPlayer == msg->u.playerInfo->playerId)
			{
				SetError(ERR_PEERDEAD);
			}
			return FALSE;
	}

	return FALSE;
}


BOOL CVPTransfer::SendPacket(LPCSTR packet, DWORD packetSize)
{
	memcpy(m_buffer+sizeof(VPMSGHDR), packet, packetSize);

	if (!vpSendData(m_session, m_peerPlayer, m_localPlayer,
		m_buffer, packetSize+sizeof(VPMSGHDR), VP_MUSTDELIVER, 0))
	{
		SetError(ERR_SENDINGDATA);
		return FALSE;
	}
	m_lastPacketTime = GetCurrentTime();
	return TRUE;
}

BOOL CVPTransfer::SendDataTo(VPPLAYERID to, VPPLAYERID from, LPVOID data, DWORD dataSize)
{
 	m_buffer = new char[VP_MAXSENDDATA];

	
	if (!m_buffer)
	{
		 SetError(ERR_NOMEMORY);
		 return FALSE;
	}



	m_peerPlayer = to;
	m_localPlayer = from;
	m_startTime = GetCurrentTime();

	return SendData(data, dataSize, TRUE);
}

	

BOOL CVPTransfer::ReceiveDataFrom(VPPLAYERID from, VPPLAYERID to, LPVOID buf, DWORD bufSize)
{
	m_peerPlayer = from;
	m_localPlayer = to;
	m_lastPacketTime = m_startTime = GetCurrentTime();
	m_buffer = new char[VP_MAXSENDDATA];

	
	if (!m_buffer)
	{
		 SetError(ERR_NOMEMORY);
		 return FALSE;
	}

	return ReceiveData(buf, bufSize, TRUE);

}


