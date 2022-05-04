#ifndef __VPXFER_H__
#define __VPXFER_H__

#include "dxfer.h"


const int MSG_FILE_XFER = 1283;


// CVPTranfer class implements sliding window data transfer on top of VDMPLAY
// Usage:  
//  The sender advertises the receiver by some means how much data it will send
//  The sender creates a CVPTransfer object
//  The sender calls SendDataTo method 
//  The reciever gets the sender advertisment of amount of data to receive
//  It allocates a buffer to receive,
//    creates a CVPTransfer the data and calls RecieveDataFrom method
//  Both sides call periodically the OnTimer method
//  Both sides call ProcessNotification method with EACH VDMPLAY notification
//  if the method return TRUE it means that we did handle this notification and 
//  the only thing left do is to call vpAcknowledge, else normal notification processing
//  should be done
//  The Done method return TRUE when the transfer is terminated either because of
//  error or because all data was transferred. It should be called ofter the
//  calls to OnTimer and ProcessNotification
//  The TransferredDataAmount  method can be used to monitor the progress of the transfer
//  when the transfer is Done the CVPTranfser object should be deleted
//  GetError method returns error code when the tranfre was abnormally aborted 
//  Multiple transfers can be active at the same time, the only limitation is
//  that between any two players only one transfer may exist
class CVPTransfer: protected CDataTransfer
{
public:
	CDataTransfer::Done;
	CDataTransfer::GetError;
	CDataTransfer::SetError;
	CDataTransfer::SendMore;
	CDataTransfer::Sending;	   
	CDataTransfer::TransferredDataAmount;

	enum { PACKETS_IN_WINDOW = 6 };
	enum { DEFAULT_TIMEOUT = 5000 };
	
	enum { 
		ERR_TIMEOUT=1, 		 // timeout witing data from the peer
		ERR_SESSIONLOST, 	 // connection to server is lost
		ERR_PEERDEAD, 		 // Peer played dead deleted 
		ERR_PLAYERDEAD,		 // local player removed
		ERR_NOMEMORY,
		ERR_SENDINGDATA		 // Error while sending data
	};

	CVPTransfer(VPSESSIONHANDLE h);
	~CVPTransfer();

	BOOL SendDataTo(VPPLAYERID to, VPPLAYERID from, LPVOID data, DWORD dataSize);
	BOOL ReceiveDataFrom(VPPLAYERID from, VPPLAYERID to, LPVOID buf, DWORD bufSize);



 	// Should be called periodically (the timer resolution 1000 mSec is OK)
	virtual void OnTimer();

	// return TRUE if we handled this notification message 
	virtual BOOL ProcessNotification(UINT nCode, LPVPMESSAGE msg);


protected:

	virtual void SendAck(DWORD amount)
	{
		SendPacket((LPCSTR) &amount, sizeof(amount));
	}

	virtual BOOL SendChunk(LPSTR chunk, DWORD chunkSize)
	{
		return SendPacket(chunk, chunkSize);
	}


	virtual BOOL SendPacket(LPCSTR packet, DWORD packetSize);


	virtual  BOOL WaitForSpace(DWORD neededSpace = 1)
	{
		return FALSE;
	}


	virtual  BOOL WaitForIncomingData()
	{
		return FALSE;
	}


	void RecordPacketTime()
	{
		m_lastPacketTime = GetCurrentTime();
		if (m_acked == m_totalSize)
			m_endTime = m_lastPacketTime;
	}

protected:
	VPPLAYERID m_peerPlayer;
	VPPLAYERID m_localPlayer;
	DWORD	   m_lastPacketTime;
	DWORD	   m_startTime;
	DWORD	   m_endTime;
	LPSTR	   m_buffer;
	VPSESSIONHANDLE m_session;
	DWORD	   m_timeout;

};



#endif
