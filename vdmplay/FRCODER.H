#ifndef __FRCODER_H__
#define __FRCODER_H__

class CCommPort;

class CFrameEncoder
{
public:
	enum { STX=0x7E, ETX=0x7F, DLE=0x7D, MASK=0x70 };

	
	BOOL Ok() const
	{
		return m_inBuf != NULL && NULL != m_outBuf;
	}

	int GetChar()
	{
		int ch;

		ch = NextChar();

		if (ch == -1)
			return ch;

		if (m_inDLE)
		{
			m_inDLE = FALSE;
			ch ^= MASK;
		}
		else if (ch == ETX)
		{
			return -1;
		}
		else if (ch == DLE)
		{
			ch = NextChar();
			if (ch == -1)
				m_inDLE = TRUE;
			else
				ch ^= MASK;
		}
		else if (ch == STX)
		{
			ch = NextChar();
		}
		

		return ch;
	}

	int NextChar()
	{
		if (m_inIndex >= m_inCount)
			FillInBuffer();

		if (m_inIndex < m_inCount)
		{
			return m_inBuf[m_inIndex++] & 0x0FF;
		}
		return -1;
	}

	virtual BOOL SendData(LPCSTR data, size_t s);

	size_t DecodeFrame(LPVOID outBuf, size_t bufSize);


	CFrameEncoder(CCommPort* p);
	virtual ~CFrameEncoder();

#ifdef NEW_DATALINK
	Datagram* InPacket() {  m_dataQ.RemoveFirst(); }
	void DecodeData();
#endif



protected:
	enum { ENCODE_BUFSIZE = 2048 };

	virtual void FillInBuffer();


protected:
	CCommPort*  m_port;
	LPSTR		m_inBuf;
	LPSTR		m_outBuf;
	LPSTR		m_frameBuf;  // decoded frame
	int			m_inIndex;
	int			m_inCount;
	BOOL		m_inDLE;
	size_t	    m_frameLen;  // decoded frame szie


#ifdef NEW_DATALINK
	DataQueue   m_inQ;
#endif


};

#endif
