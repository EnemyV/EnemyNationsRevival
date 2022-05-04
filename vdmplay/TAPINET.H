#ifndef __TAPINET_H__
#define __TAPINET_H__


class Dialer;
class CDataLogger;
class CTapiNet;

class CVPTapiConn: public CTapiConnection
{
	friend class CTapiNet;

	CTapiNet* m_net;
public:

	CVPTapiConn() : m_net(NULL) { }
	virtual void OnConnect();
	virtual void OnDisconnect();
	virtual void OnDialFail();

};
	


class CTapiNet: public CCommNet
{

public:
	
	CTapiNet(CTDLogger* log, CCommPort* port,  LPCSTR callNumber, HINSTANCE hInst, LPCSTR iniFile, LPCSTR iniSection);
	~CTapiNet();


	void OnTimer();
    virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//+ Start listening for incoming connections and datagrams
    virtual BOOL   Listen(BOOL streamListen = TRUE, BOOL serverMode = FALSE);

	void Cleanup();

	// callabck for the user cancel request
	static void CancelCbk(LPVOID p);

	// callback for the dialer events
	static void DialerCbk(LPVOID p);

	virtual void  OnCancelCall();
	
	virtual void  OnConnect();
	virtual void  OnDisconnect();
	virtual void  OnDialFail();

	static BOOL ModemWaitTest(LPVOID context);
	virtual BOOL WaitModemConnection();

public:
	CVPTapiConn m_tapiObj;

    LPCSTR m_iniFile;
	LPCSTR m_iniSection;
	LPCSTR m_callNumber;
	HINSTANCE m_hInst;
	BOOL	m_serverMode;
	BOOL	m_streamListen;
	BOOL	m_waitingConnect;
	BOOL	m_connected;
};

#endif


