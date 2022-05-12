#ifndef __MDMNET_H__
#define __MDMNET_H__


class Dialer;
class CDataLogger;

class CModemNet: public CCommNet
{

public:
	
	CModemNet(CTDLogger* log, CCommPort* port,  LPCSTR listenInit, LPCSTR callInit, LPCSTR callNumber, LPCSTR pfx, LPCSTR sfx);
	~CModemNet();


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
	virtual void  OnDialerEvent();

	static BOOL ModemWaitTest(LPVOID context);

	virtual BOOL WaitModemConnection();

public:
    LPCSTR m_listenInit;
	LPCSTR m_mdmInit;
	LPCSTR m_callNumber;
	LPSTR  m_dialPrefix;
	LPSTR  m_dialSuffix;

	Dialer*  volatile m_dialer;
	BOOL	m_serverMode;
	volatile BOOL	m_waitingConnect;
};

#endif


