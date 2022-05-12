#include "stdafx.h" 
#include "commport.h"
#include "w16comm.h"

#define MSR_CTS              0x10  /* Absolute CTS state in MSR  */ 
#define MSR_DSR              0x20  /* Absolute DSR state in MSR  */
#define MSR_RI               0x40  /* Absolute RI state in MSR   */
#define MSR_RLSD             0x80  /* Absolute RLSD state in MSR */
 
#define COMM_MSRSHADOW 35


BOOL CW16CommPort::Open()
{
	VPENTER(CW16CommPort::Open);
	BOOL ret;

	if (IsOpen())
	{
		VPEXITBOOL(TRUE);
		return TRUE;
	}

	m_commH = OpenComm(m_deviceName, 4096, 4096);
	ret = (int) m_commH  >= 0;
	if (ret)
	{
		m_msr = COMM_MSRSHADOW + (LPBYTE) SetCommEventMask(m_commH, 0);
	}
	else
		m_commH = INVALID_PORT;


	VPEXITBOOL(ret);
	return TRUE;
}


DWORD CW16CommPort::Read(LPVOID buf, DWORD bufSize)
{
	COMSTAT stat;
	
	GetCommError(m_commH, &stat);
	

	UINT inCount = (UINT) bufSize;
	
	
	inCount = ReadComm(m_commH, buf, inCount);
	if (!inCount)
		GetCommError(m_commH, &stat);
	
	return inCount;
	
}


void CW16CommPort::Flush()
{
	FlushComm(m_commH, 0);
	FlushComm(m_commH, 1);
}
	


DWORD CW16CommPort::Write(LPCVOID buf, DWORD bufSize)
{
  	COMSTAT stat;
	
	GetCommError(m_commH, &stat);

	return WriteComm(m_commH, buf, (UINT) bufSize);
}


BOOL CW16CommPort::Close()
{
	if (m_commH != -1)
	{
		FlushComm(m_commH, 0);
		FlushComm(m_commH, 1);
		EscapeCommFunction(m_commH, CLRDTR);
		CloseComm(m_commH);
	}
	
	m_commH = -1;
	return TRUE;

}


BOOL CW16CommPort::Setup(DWORD baudRate)
{
    DCB  dcb;
   
   	if (baudRate > 19200)
   	{
   		if (baudRate == 38400L)
   			baudRate = CBR_38400;
   		else if (baudRate == 57600L) 
   			baudRate = CBR_56000;    // I hope this will work
   		else
   			baudRate = CBR_128000;   // I hope this will work
   	}	 
    
    GetCommState(m_commH, &dcb);
    
  	dcb.Id = (BYTE) m_commH;
    dcb.fBinary = TRUE;
    dcb.BaudRate = (UINT) baudRate;
    dcb.ByteSize = 8;
    dcb.fParity = FALSE;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    
	// we don't need software flow ctl
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;

	dcb.fOutxCtsFlow =  TRUE;
	dcb.fRtsflow =  TRUE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrDisable = FALSE;
	
	dcb.fDtrflow = FALSE;
	dcb.fBinary = TRUE;
	
	
    SetCommState(&dcb);
    
    m_msr = COMM_MSRSHADOW + (LPBYTE) SetCommEventMask(m_commH, EV_BREAK|EV_RING|EV_RXCHAR|EV_RLSD|EV_TXEMPTY);
    EnableCommNotification(m_commH, m_wnd, 1, -1);
    
    
    return TRUE;



}

UINT CW16CommPort::ClearEvent()
{
	UINT e;
	
	e = GetCommEventMask(m_commH, EV_BREAK|EV_RING|EV_RXCHAR|EV_RLSD|EV_TXEMPTY); 
	return e;
}


BOOL CW16CommPort::HasCarrier()
{  
	if (!IsOpen())
		return FALSE;
		
	if (!m_msr)
		return FALSE;
  	return 0 != (*m_msr & MSR_RLSD);
}


BOOL CW16CommPort::HasDsr()
{
	if (!IsOpen())
		return FALSE;
		
	if (!m_msr)
		return FALSE;

  	return 0 != (*m_msr & MSR_DSR);
}


	
CW16CommPort::~CW16CommPort()
{
	Close();
}

	

