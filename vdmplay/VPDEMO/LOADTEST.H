#ifndef __LOADTEST_H__
#define __LOADTEST_H__

class CLoadTest
{

public:
	struct testInfo
	{
		char	text[8];
		char    type;
		DWORD	seqnum;
		DWORD   msgtime; 
		DWORD   reqtime;
		DWORD	reqseq;
		char    data[400];
	};
	
#pragma pack(1)
	struct testMsg
	{
		VPMSGHDR  hdr;
		testInfo  i;
	};
#pragma pack()

	enum { TST_MSG='T', REQ_MSG='R', ACK_MSG='A', END_MSG='E' };

	virtual BOOL  Done() { return m_done; }
 	// Should be called periodically (the timer resolution 100 mSec is OK)
	virtual void OnTimer() = 0;

	// return TRUE if we handled this notification message 
	virtual BOOL ProcessNotification(UINT nCode, LPVPMESSAGE msg) = 0;

	CLoadTest(VPSESSIONHANDLE ses, VPPLAYERID myId);

	virtual void SendMsg(testMsg* msg, VPPLAYERID to=VP_ALLPLAYERS);
	virtual void InitMsg(testMsg* msg);
	void    Log(LPCSTR fmt, DWORD p1=0, DWORD p2=0);

	void OnFlowOff();
	void OnFlowOn();

	BOOL FlowIsBlocked();

public:
	VPSESSIONHANDLE m_ses;
	BOOL m_done;
	DWORD	m_sendSeqNum;
	DWORD	m_recvSeqNum;
	VPPLAYERID m_myId; 
	CVpdemoDlg* m_dlg;
	BOOL        m_stopFlow;
	UINT	    m_packetSize;
};


class SlaveLoadTest: public CLoadTest
{
public:

 	// Should be called periodically (the timer resolution 100 mSec is OK)
	virtual void OnTimer() { }

	// return TRUE if we handled this notification message 
	virtual BOOL ProcessNotification(UINT nCode, LPVPMESSAGE msg);

	SlaveLoadTest(VPSESSIONHANDLE ses, VPPLAYERID id);
};

class MasterLoadTest: public CLoadTest
{
public:

 	// Should be called periodically (the timer resolution 100 mSec is OK)
	virtual void OnTimer();

	// return TRUE if we handled this notification message 
	virtual BOOL ProcessNotification(UINT nCode, LPVPMESSAGE msg);

	MasterLoadTest(VPSESSIONHANDLE ses, VPPLAYERID id, int pCount=1); 

	void GenerateMessages(int i);

public:
	int m_sendsPerTick;
	int m_sendsPerBurst;
	DWORD  m_waitingAck;
	DWORD  m_reqTime;
	int    m_nonAckCount;   // How much players still not acked 
	int	m_playerCount;
	DWORD  m_ackTimeout;
	UINT   m_tickDivisor;

	UINT   m_skippedTicks;
	DWORD  m_tickFreq;
};

#endif


