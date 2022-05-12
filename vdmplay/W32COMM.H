#ifndef __W32COMM_H__
#define __W32COMM_H__ 


class CW32CommPort:public CCommPort
{
public:
	virtual BOOL Open();
	virtual DWORD Read(LPVOID buf, DWORD bufSize);
	virtual DWORD Write(LPCVOID buf, DWORD bufSize);
	virtual BOOL Close();
	virtual BOOL Setup(DWORD baudRate);
	virtual BOOL HasCarrier();
	virtual BOOL HasDsr();
	virtual ~CW32CommPort();
	virtual BOOL StartEventThread();
	virtual UINT ClearEvent() { return 0; }
	virtual void Flush();

	virtual BOOL AdoptPort(HCOMM hComm)
	{
		return UseTapiPort(hComm);
	}

	virtual BOOL UseTapiPort(HCOMM hComm);

	CW32CommPort(LPCSTR portName=NULL, HWND wnd=NULL) :
	CCommPort(portName, wnd),
		m_eventThread(INVALID_HANDLE_VALUE), m_usingTapi(FALSE)
		{ 
			m_ro.hEvent = INVALID_HANDLE_VALUE;
			m_wo.hEvent = INVALID_HANDLE_VALUE;
			m_mo.hEvent = INVALID_HANDLE_VALUE;

		}
	
	static DWORD __stdcall EventThreadProc(LPVOID arg);



protected:
	HANDLE   m_eventThread;
	OVERLAPPED m_ro;
	OVERLAPPED m_wo;
	OVERLAPPED m_mo;
	BOOL  m_usingTapi;


};



#endif 
