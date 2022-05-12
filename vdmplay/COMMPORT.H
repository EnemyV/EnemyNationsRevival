#ifndef __COMMPORT_H__
#define __COMMPORT_H__

#ifdef WIN32
typedef HANDLE HCOMM;
#define INVALID_PORT INVALID_HANDLE_VALUE
#else 
typedef int HCOMM;
#define INVALID_PORT -1
#endif

class CCommPort
{
public:
	virtual BOOL Open() = 0;
	virtual DWORD Read(LPVOID buf, DWORD bufSize) = 0;
	virtual DWORD Write(LPCVOID buf, DWORD bufSize) = 0;
	virtual BOOL Close() = 0;
	virtual BOOL Setup(DWORD baudRate) = 0;
	virtual BOOL HasCarrier() = 0;
	virtual BOOL HasDsr() = 0;
	virtual UINT ClearEvent() = 0;
	virtual void Flush() = 0;
	virtual ~CCommPort() { }
	CCommPort(LPCSTR devName=NULL, HWND wnd = NULL) :
			m_error(0), m_wnd(wnd), m_commH(INVALID_PORT)
	{
		strcpy(m_deviceName, devName? devName : "");
	}

	BOOL IsOpen() const { return m_commH != INVALID_PORT; }

	void SetError(DWORD e) {  m_error = e; }
	DWORD GetError()
	{
		DWORD e = m_error;

		m_error = 0;
		return e;
	}
	
	virtual BOOL AdoptPort(HCOMM hComm)
	{
		m_commH = hComm;
		return TRUE;
	}

public:
	HCOMM   m_commH;  
	HWND    m_wnd;		// associated window
	DWORD   m_error;
	char	m_deviceName[256];
};

#endif 
