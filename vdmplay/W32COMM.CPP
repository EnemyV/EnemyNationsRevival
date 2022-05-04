#include "stdafx.h"
#include "vpnet.h" 
#include "datagram.h"
#include "commport.h"
#include "w32comm.h"


DWORD GetCommError(HCOMM dev, LPCOMSTAT s)
{
	DWORD err;
	
	ClearCommError(dev, &err, s);
	return err;
}


HCOMM OpenComm(LPCSTR n, unsigned i, unsigned o) 
{	HANDLE dev = CreateFile(n, 
					GENERIC_READ|GENERIC_WRITE,
					0,  // NO SHARING
					NULL,  // NO security attributes
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
					NULL);
					
	if (dev != INVALID_HANDLE_VALUE)
	{
		SetupComm(dev, i, o);
	}
	else
	{
		DWORD err = GetLastError();

		VPTRACE(("Error %d opening %s port\n", err, n));

		dev = (HANDLE) -1;
	}
	return dev;
}

void FlushComm(HCOMM dev, int which)
{
 	VPTRACE(("FlushComm"));
	PurgeComm(dev, (!which) ? PURGE_RXABORT|PURGE_RXCLEAR : PURGE_TXABORT|PURGE_TXCLEAR);

}

void CloseComm(HCOMM dev)
{
	FlushComm(dev, 0);
	FlushComm(dev, 1);
	CloseHandle(dev);
}


unsigned WriteComm(HCOMM dev, LPCVOID buf, unsigned count)
{
   DWORD written = 0;

   if (WriteFile(dev, buf, count, &written, NULL))
   {
   		return written;
   }

   return (unsigned)-1;
 }

unsigned ReadComm(HCOMM dev, LPVOID buf, unsigned count)
{
   DWORD wasread = 0;

   if (ReadFile(dev, buf, count, &wasread, NULL))
   {
   		return wasread;
   }

   return 0;
}

void CW32CommPort::Flush()
{
	FlushComm(m_commH, 0);
	FlushComm(m_commH, 1);
}


DWORD __stdcall CW32CommPort::EventThreadProc(LPVOID arg)
{
	CW32CommPort* port = (CW32CommPort*) arg;
	DWORD evtMask;

	SetCommMask(port->m_commH, EV_BREAK|EV_RING|EV_RXCHAR|EV_RLSD|EV_TXEMPTY);

	while(1)
	{
		evtMask = 0;
		if (!WaitCommEvent(port->m_commH, &evtMask, &port->m_mo))
		{
			DWORD err = GetLastError();

			if (err != ERROR_IO_PENDING)
			{
				VPTRACE(("EventThreadProc: WaitCommEvent: err = %ld", err));
				break;
			}

			if (!GetOverlappedResult(port->m_commH, &port->m_mo, &err, TRUE))
			{
				err = GetLastError();
				VPTRACE(("EventThreadProc: GetOverlappedResult: err = %ld", err));
				break;
			}
		}

		if (evtMask != 0 && port->m_wnd != NULL)
		{
			PostMessage(port->m_wnd, WM_COMMNOTIFY, 0, evtMask);
		}
	}
		 
	VPTRACE(("EventThreadProc: Exiting"));
	return (0);
}

BOOL CW32CommPort::StartEventThread()
{
	VPENTER(W2CommPort::StartEventThread)
	if (!IsOpen())
	{
		VPTRACE(("StartEventThread: Port is closed"));
		return FALSE;
	}

	
	if (m_eventThread != INVALID_HANDLE_VALUE)
	{
		VPTRACE(("StartEventThread: thread is already running"));
		VPEXITBOOL(TRUE);
		return TRUE;
	}

	DWORD id;


	m_eventThread = CreateThread(NULL, 0, EventThreadProc, (LPVOID) this, 0, &id);
		
	if (m_eventThread == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
		VPTRACE(("Error %d creating EventThread", err));
		SetError(err);
		Close();
		VPEXITBOOL(FALSE);
		return (FALSE);
	}

	VPTRACE(("Created EventThread: id %x handle %x", id, m_eventThread));
	VPEXITBOOL(TRUE);
	return TRUE;	 
}


inline void MaybeClose(HANDLE& h)
{
	if (h != INVALID_HANDLE_VALUE)
		CloseHandle(h);
	h = INVALID_HANDLE_VALUE;
}


BOOL CW32CommPort::Open()
{
	if (IsOpen())
		return TRUE;
			
	m_commH = OpenComm(m_deviceName, 4096, 4096);

	if (IsOpen())
	{		
	
		SetCommMask(m_commH, EV_RLSD|EV_DSR|EV_RXCHAR);
		memset(&m_ro, 0, sizeof(m_mo));
		memset(&m_wo, 0, sizeof(m_mo));
		memset(&m_mo, 0, sizeof(m_mo));

		m_ro.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_wo.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_mo.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	return IsOpen();
}

BOOL CW32CommPort::UseTapiPort(HCOMM hComm)
{
	CCommPort::AdoptPort(hComm);

	m_usingTapi = TRUE;

	memset(&m_ro, 0, sizeof(m_mo));
	memset(&m_wo, 0, sizeof(m_mo));
	memset(&m_mo, 0, sizeof(m_mo));

	m_ro.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_wo.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_mo.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	return TRUE;
}

BOOL CW32CommPort::Close()
{
	VPENTER(CW32CommPort::Close);

	if (m_eventThread != INVALID_HANDLE_VALUE)
	{
		TerminateThread(m_eventThread, 0);
		CloseHandle(m_eventThread);
		m_eventThread = INVALID_HANDLE_VALUE;
	
	}

	if (m_commH != INVALID_PORT)
		CloseComm(m_commH);
	
	m_commH = INVALID_PORT;

	MaybeClose(m_ro.hEvent);
	MaybeClose(m_wo.hEvent);
	MaybeClose(m_mo.hEvent);

	VPEXITBOOL(TRUE);
	return TRUE;
}


DWORD CW32CommPort::Write(LPCVOID buf, DWORD bufSize)
{

	if (!IsOpen())
	{
		VPTRACE(("Write: Port is closed"));
		return 0;
	}


	DWORD written;

	if (WriteFile(m_commH, buf, bufSize, &written, &m_wo))
   	{
   		return written;
   	}

	DWORD err = GetLastError();

	if (err != ERROR_IO_PENDING)
	{
		VPTRACE(("Write: WriteFile: err = %ld", err));
		SetError(err);
		return (DWORD) -1;
	}


	if (!GetOverlappedResult(m_commH, &m_wo, &written, TRUE))
	{
		err = GetLastError();
		VPTRACE(("Write: GetOverlappedResult: err = %ld", err));
		SetError(err);
	
	   	return (DWORD) -1;
	}


	if (written != bufSize)
	{
		VPTRACE(("CW32CommPort::Writee written %u bytes instead of %u", 
			written, bufSize));
	}
	return written;
	
}



DWORD CW32CommPort::Read(LPVOID buf, DWORD bufSize)
{
	DWORD err = 0;
	COMSTAT  stat;

	if (!IsOpen())
	{
		VPTRACE(("Read: Port is closed"));
		return 0;
	}
	
	ClearCommError(m_commH, &err, &stat);

	if (err & CE_BREAK)
		return (DWORD)-1;

	if (bufSize > stat.cbInQue)
		bufSize = stat.cbInQue;

	if (!bufSize)
		return (0);
			

	DWORD wasRead;

   	if (ReadFile(m_commH, buf, bufSize, &wasRead, &m_ro))
   	{
   		return wasRead;
   	}


	err = GetLastError();

	if (err != ERROR_IO_PENDING)
	{
		VPTRACE(("Read: ReadFile: err = %ld", err));
		SetError(err);
		return (DWORD) -1;
	}


	if (!GetOverlappedResult(m_commH, &m_ro, &wasRead, TRUE))
	{
		err = GetLastError();
		VPTRACE(("Read: GetOverlappedResult: err = %ld", err));
		SetError(err);
	
	   	return (DWORD) -1;
	}

	
	return wasRead;



	
}



BOOL CW32CommPort::HasCarrier()
{
	DWORD mdmStat;

	if (!IsOpen())
		return FALSE;
	
	if (!GetCommModemStatus(m_commH, &mdmStat))
	{
		DWORD err;

		err = GetLastError();
		VPTRACE(("W32CommPort:HasCarrier err = %lu", err));
		SetError(err);
		return FALSE;

	}


	return 0 != (mdmStat & MS_RLSD_ON);
	
}

BOOL CW32CommPort::HasDsr()
{
	DWORD mdmStat;

	if (!IsOpen())
		return FALSE;
	
	GetCommModemStatus(m_commH, &mdmStat);

	return 0 != (mdmStat & MS_DSR_ON);
	
}



BOOL CW32CommPort::Setup(DWORD baudRate)
{

	DCB dcb;

	VPENTER(W32CommPort::Setup);

    dcb.DCBlength = sizeof(dcb);
	
	if (!GetCommState(m_commH, &dcb))
	{
		VPTRACE(("GetCommState error")); 
		SetError(GetLastError());
		VPEXITBOOL(FALSE);
		return FALSE;
	}


	if (!m_usingTapi)
	{

#if 1    
		dcb.BaudRate = baudRate;
		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;
		dcb.ByteSize = 8;
#else
		char buf[256];

		wsprintf(buf,"baud=%ld parity=%c data=%d stop=%d", //  xon=off rts=hs" octs=on rts=hs" 
        				baudRate, (char) 'N', 8, 1);


		if (!BuildCommDCB(buf, &dcb))
		{
			VPTRACE(("GetCommState error")); 
			SetError(GetLastError());
			VPEXITBOOL(FALSE);
			return FALSE;
		}
#endif
	}

    dcb.fBinary = TRUE;

	// we don't need software flow ctl
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;


	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	dcb.fOutxCtsFlow = TRUE;
	dcb.fErrorChar = 0;

    if (!SetCommState(m_commH, &dcb))
	{
		VPTRACE(("SetCommState error")); 
		SetError(GetLastError());
		VPEXITBOOL(FALSE);
		return FALSE;
	}


    if (!StartEventThread())
   	{
		VPEXITBOOL(FALSE);
		return FALSE;
	}


	VPEXITBOOL(TRUE);
	return TRUE;
	
	
}


CW32CommPort::~CW32CommPort()
{
	if (m_commH != INVALID_PORT)
	{
		PurgeComm(m_commH,
				  PURGE_RXABORT|PURGE_RXCLEAR|PURGE_TXABORT|PURGE_TXCLEAR);	
		Close();
	}
	
}

	
