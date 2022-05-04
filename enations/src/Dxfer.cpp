#include "stdafx.h"
#include "dxfer.h"

CDataTransfer::CDataTransfer(DWORD windowSize)
: m_windowSize(windowSize), m_transferred(0), m_acked(0), 
  m_totalSize(0), m_error(FALSE),   m_data(NULL), m_sending(FALSE)
{
}

CDataTransfer::~CDataTransfer()
{
}


// Fill the transmission window
BOOL CDataTransfer::FillWindow()
{
	DWORD moreData = UntransferredDataAmount();
	DWORD freeSpace = FreeWindowSpace();
	
	while(freeSpace && moreData)
	{
		DWORD chunkSize = m_chunkSize;

		if (chunkSize > moreData)
			chunkSize = moreData;

		if (chunkSize > freeSpace)
			chunkSize = freeSpace;

		if (!SendChunk(m_data, chunkSize))
			return FALSE;

		m_data += chunkSize;
		m_transferred += chunkSize;

		moreData = 	UntransferredDataAmount();
		freeSpace = FreeWindowSpace();
	}

	return TRUE;
}


void CDataTransfer::OnAck(DWORD amount)
{
	m_acked = amount;
	TRACE("Got Ack %lu\n", amount, m_acked); 
}


BOOL CDataTransfer::SendData(LPVOID data, DWORD dataSize, BOOL async)
{

	m_data = (LPSTR) data;
	m_totalSize = dataSize;
	m_sending = TRUE;

	if (!FillWindow())
		return FALSE;

	if (async)
		return TRUE;

	while(UntransferredDataAmount())
	{
		if (!WaitForSpace())
			return FALSE;

		if (!FillWindow())
			return FALSE;

	}

	return WaitForEmptyWindow();
}


BOOL CDataTransfer::ReceiveData(LPVOID data, DWORD dataSize, BOOL async)
{

	m_data = (LPSTR) data;
	m_totalSize = dataSize;
	m_sending = FALSE;

	if (async)
		return TRUE;

	while(UntransferredDataAmount())
	{
		if (!WaitForIncomingData())
			return FALSE;
	}

	return TRUE;
}


void CDataTransfer::OnIncomingData(LPVOID data, DWORD dataSize)
{
	TRACE("Got data %ul\n", dataSize);
	memcpy(m_data, data, dataSize);
	m_transferred += dataSize;
	m_data += dataSize;
	if ((FreeWindowSpace() < m_windowSize/2) || !UntransferredDataAmount())
	{
		m_acked = m_transferred;
		TRACE("ACKING %lu\n", m_acked);
		SendAck(m_transferred);
	}
}

