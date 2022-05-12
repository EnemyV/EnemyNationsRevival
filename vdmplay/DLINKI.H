#ifndef __DLINKI_H__
#define __DLINKI_H__


class CFrameEncoder;
class CCommPort;
class CTDLog;
class CDataLogger;

enum { SOH=1, ACK='A', NAK='N', ZAK='Z' };
enum { CTL='C', CTLACK='A', CTLX = 'X' , UDATA='U', SDATA='S' };

class CDLinkInt
{

public:
	 enum { PKTBUFSIZE=1024*4 };
	 enum { 
			TIMEOUT_WHEN_CONNECTED = 1000, 
			TIMEOUT_WHEN_DISCONNECTED = 5000
		  };
	 enum STATES {  CTLSENT, CTLASENT, CTLARCVD, CONNECTED, BROKEN };
	 enum PACKETSTATES { SEEKSOH, READINGDATA };
	 enum LINKEVENTS { LINKUP, LINKDOWN, LINKSDATA, LINKUDATA };

	 typedef void (*EventProc)(CDLinkInt*, int event, LPVOID userData);


public:

	CDLinkInt(CCommPort* port, CTDLogger* logger);
	virtual ~CDLinkInt();

	virtual size_t HasData() = 0;
	virtual void BreakLink() = 0;
	virtual void OnTimer() = 0;
	virtual void OnIncomingData() = 0;
	virtual BOOL BuildAndSendPacket(LPVOID data, 
						  DWORD dataSize, 
						  char pktType) = 0;


	BOOL SendData(LPCVOID data, DWORD size, BOOL safe)
	{
		OnIncomingData();
		return BuildAndSendPacket((LPVOID) data, size, safe ? SDATA : UDATA);
	}



	BOOL   DataLinkReady() const { return m_linkState == CONNECTED; } 

	  
    void SetError(DWORD err)
	{
		m_error = err;

		if (m_errLog)
			m_errLog->SetError(VP_ERR_NET_ERROR, err, 0);

	}

    void FatalError(DWORD err)
	{
		m_error = err;

		if (m_errLog)
			m_errLog->SetFatalError(VP_ERR_NET_ERROR, err, 0);

	}
	
	void Log(LPCSTR m)
	{
		if (m_errLog)
			m_errLog->Log(m);
	}

	void LogIncomingPacket(char c, UINT num, UINT siz);
	void LogOutgoingPacket(char c, UINT num, UINT siz);



protected:

     LPSTR        m_inBuffer;      // buffer for incoming data
	 int          m_inIndex;       // first free byte in the input buffer 
	 int          m_pktStart;	   // beginning of the incoming packet
	 int          m_readIndex;      // index of the next byte to get 
	 CCommPort*   m_port;
	 DWORD		  m_error;
	 EventProc    m_linkProc;
	 LPVOID		  m_eventData;	 
	 CTDLogger*   m_errLog;	 
	 CDataLogger* m_dataLog;
	 CFrameEncoder* m_encoder;
	 int		  m_linkState;
	 int		  m_packetState;
	 
};


#endif


