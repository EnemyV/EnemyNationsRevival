#include "stdafx.h"

#include "commport.h"
#include "frcoder.h"

CFrameEncoder::CFrameEncoder(CCommPort* p)
{
	m_inDLE = FALSE;
	m_inIndex = 0;
	m_inCount = 0;
	m_frameLen = 0;
	m_port = p;
	m_inBuf = new char[ENCODE_BUFSIZE];
	m_outBuf = new char[ENCODE_BUFSIZE];
	m_frameBuf = new char[ENCODE_BUFSIZE];
}

CFrameEncoder::~CFrameEncoder()
{
	delete [] m_outBuf;
	delete [] m_inBuf;
	delete [] m_frameBuf;
}



BOOL CFrameEncoder::SendData(LPCSTR data, size_t s)
{
	size_t count = 0;
	char*  d = m_outBuf;

	*d++ = STX;
	while(s--)
	{
		char ch = *data++;

		if (ch == STX || ETX == ch || ch == DLE)
		{
			*d++ = DLE;
			*d++ = ch	^ MASK;
		}
		else
			*d++ = ch;
	}
	*d++ = ETX;

	count = d - m_outBuf;

	return m_port->Write(m_outBuf, count);
}

void CFrameEncoder::FillInBuffer()
{
	m_inCount = m_port->Read(m_inBuf, ENCODE_BUFSIZE);
	m_inIndex = 0;
}

size_t CFrameEncoder::DecodeFrame(LPVOID outBuf, size_t bufSize)
{
	int ch;
	char* frame = m_frameBuf + m_frameLen;
	size_t ret = 0;
	size_t maxFrame = bufSize+1;

	while(1)
	{
		if (m_frameLen > maxFrame) // we've missed ETX
		{
			// reset the frame
			m_frameLen = 0;
			frame = m_frameBuf;
			m_inDLE = FALSE;
		}

		ch = NextChar();

		if (ch == -1)
			return 0;  // no more data

		if (ch == STX) // We've got a start of frame
		{
			m_frameLen = 1;
			frame  = m_frameBuf;
			*frame++ = STX;
			m_inDLE = FALSE;
			continue;
		}

		if (!m_frameLen)  // we look for STX
			continue;

		if (ch == ETX)
		{
			if (m_inDLE)
			{
				// we'be missed character just after the DLE
				m_frameLen = 0;
				frame = m_frameBuf;
				m_inDLE = FALSE;
				continue;
			}

			ret = m_frameLen-1;
			m_frameLen = 0;
			m_inDLE = FALSE;
			if (ret)
			{
				// copy the frame while skipping the STX char in the beginning
				memcpy(outBuf, m_frameBuf+1, ret);
				return ret;
			}
			// the frame was really too short
			continue;
		}
		else if (ch == DLE)
		{
			if (m_inDLE)
			{
				// we'be missed character just after the DLE
				m_frameLen = 0;
				frame = m_frameBuf;
				m_inDLE = FALSE;
				continue;
			}

			ch = NextChar();
			if (ch == -1)
			{
				m_inDLE = TRUE;
				return 0;
			}
			ch ^= MASK;
		}

		if (m_inDLE)
		{
			m_inDLE = FALSE;
			ch ^=  MASK;
		}


		*frame++ = (char) ch;
		m_frameLen++;
	}

}
