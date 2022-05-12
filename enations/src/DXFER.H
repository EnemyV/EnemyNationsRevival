#ifndef __DXFER_H__
#define __DXFER_H__

class CDataTransfer
{

public:

	CDataTransfer(DWORD windowSize);
	virtual ~CDataTransfer();



	virtual  BOOL SendData(LPVOID data, DWORD dataSize, BOOL async);
	BOOL SendMore()
	{
		return FillWindow();
	}
	virtual  BOOL ReceiveData(LPVOID data, DWORD dataSize, BOOL async);

	virtual  void OnAck(DWORD count);
	virtual  void OnIncomingData(LPVOID data, DWORD dataSize);

	DWORD	UntransferredDataAmount()  const
	{
		return m_totalSize - m_transferred;
	}

	DWORD   TransferredDataAmount() const
	{
		return m_acked;
	}

	BOOL Done() const
	{
		return m_error || (m_totalSize == m_acked);
	}

	DWORD  GetError() const
	{
		return m_error;
	}


	void SetError(DWORD err)
	{
		m_error = err;
	}

	BOOL Sending() const { return m_sending; }

protected:

	DWORD	 FreeWindowSpace() const 
	{
		return m_windowSize - (m_transferred - m_acked);
	}



	virtual  void SendAck(DWORD ackAmount) = 0;
	virtual  BOOL SendChunk(LPSTR chunk, DWORD chunkSize) = 0;

	virtual  BOOL WaitForSpace(DWORD neededSpace = 1) = 0;
	virtual  BOOL WaitForIncomingData() = 0;


	BOOL WaitForEmptyWindow()
	{
		return WaitForSpace(m_windowSize);
	}


	virtual  BOOL FillWindow();

protected:
	DWORD	 m_error;					// error code
	DWORD	 m_windowSize;				// transmission window size
	volatile DWORD	 m_transferred;		// transferred data amount
	volatile DWORD	 m_acked;			// acknowledged data amount
	DWORD	 m_totalSize;				// total transfer request size
	DWORD	 m_chunkSize;				// transfer chunk size
	LPSTR	 m_data;					// data buffer
	BOOL	 m_sending;					// are we sending ?

};


#endif
