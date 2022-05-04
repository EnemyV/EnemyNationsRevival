#ifndef __DATALINK_H__
#define __DATALINK_H__


enum { SOH=1, ACK='A', NAK='N', ZAK='Z' };
enum { CTL='C', CTLACK='A', CTLX = 'X' , UDATA='U', SDATA='S' };

#pragma pack(1)
struct pkthdr
{
   char  soh;   // start of header
   char  ack;   // acknowledge 
   char  typ;   // packet type
   BYTE  num;
   WORD  siz;   // size
   WORD  chk;   // checksum
};
#pragma pack()


class CFrameEncoder;

class CCommPacket: public CVPLink
{
public:	
	pkthdr* m_hdr;
	LPVOID  m_data;
	DWORD	m_time;
	BOOL    m_ownBuffer;   // when true delete the m_hdr in the destructor
	
	WORD  Size() const { return m_hdr->siz; }
    WORD  DataSize() const { return Size() - sizeof(*m_hdr); }

	// calculate the checksum over the packet data part
	WORD  DataCheckSum(WORD seed=0) const;
	// calculate the complete packet checksum
	WORD  CheckSum() const;

	void  PreparePacket()
	{
		m_hdr->chk = 0;
		m_hdr->chk = CheckSum();
	}
	
	BOOL  GoodPacket()
	{
		WORD chk = m_hdr->chk;

		m_hdr->chk = 0;
		WORD ckSum =  CheckSum();
		m_hdr->chk = chk;
		
		return ckSum == chk;
	}

	LPCSTR Data(DWORD offset=0) const
	{
		return offset + (LPCSTR) m_data;
	}

	LPCSTR WholePacket() const
	{
		return (LPCSTR) m_hdr;
	}
	
	CCommPacket(LPVOID data, DWORD dataSize);
	CCommPacket(pkthdr* hdr, LPVOID data) : 
		m_ownBuffer(FALSE),
		m_hdr(hdr), m_data(data) { }

	CCommPacket(LPVOID p) :
			m_hdr((pkthdr*)p), m_ownBuffer(FALSE)
	{
		m_data = (LPVOID) (1 + m_hdr);
	}
			
	virtual ~CCommPacket(); 
	BOOL IsControlPacket() const 
	{ 
		return m_hdr->typ == CTL || 
			   m_hdr->typ == CTLACK ||
			   m_hdr->typ == CTLX;	
	}
	
	 
	BOOL Ok() const { return m_hdr != NULL; }


	BOOL ContainsAck() const
	{
		return m_hdr->ack == ACK;
	}  

	BOOL ContainsNak() const
	{
		return m_hdr->ack == NAK;
	}  

	BOOL ContainsData() const
	{
		return m_hdr->siz > sizeof(pkthdr);
	}
		

	CCommPacket(Datagram* dg);

};	

declare(VPList, CCommPacket);
typedef VPList(CCommPacket) CPacketQueue;

class CCommPort;


class CTDLog;
class CDataLogger;

class CDataLink
{
public:
	 enum { PKTBUFSIZE=1024*4 };
	 enum { 
			TIMEOUT_WHEN_CONNECTED = 1000, 
			TIMEOUT_WHEN_DISCONNECTED = 5000
		  };
	 enum STATES {  CTLSENT, CTLASENT, CTLARCVD, CONNECTED, BROKEN };
	 enum PACKETSTATES { SEEKSOH, READINGDATA };
	 enum LINKEVENTS { LINKUP, LINKDOWN, LINKSDATA, LINKUDATA, STOPFLOW, RESUMEFLOW };

	 typedef void (*EventProc)(CDataLink*, int event, LPVOID userData);
	
     LPSTR        m_inBuffer;      // buffer for incoming data
	 int          m_inIndex;       // first free byte in the input buffer 
	 int          m_pktStart;	   // beginning of the incoming packet
	 int          m_readIndex;      // index of the next byte to get 
	 DWORD        m_timer;           // ackonwledgement timer
	 UINT		  m_resends;
	 UINT		  m_resendLimit;
	 UINT		  m_ackDelay;
	 CCommPort*   m_port;
	 CPacketQueue m_waitQ;		   // queue of packet waiting for transmission
	 CPacketQueue m_outQ;          // queue of packets waiting for ack
	 DataQueue	  m_inQ;
	 WORD		  m_inPktNum;
	 WORD		  m_outPktNum;
	 WORD		  m_firstUnacked;
	 unsigned	  m_winSize;
	 int		  m_linkState;
	 int		  m_packetState;
	 BOOL		  m_needAck;
	 DWORD		  m_error;
	 EventProc    m_linkProc;
	 LPVOID		  m_eventData;	 
	 CTDLogger*   m_errLog;	 
	 CDataLogger* m_dataLog;
	 CFrameEncoder* m_encoder;
	 BOOL		  m_hostIsOff;

	 
	void SuspendHost();
	void ResumeHost();

	size_t HasData() { return (size_t) m_inQ.FirstItemSize(); }
	  
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
	
	void BecomeConnected() 
	{ 
		m_linkState = CONNECTED;
		m_ackDelay =  TIMEOUT_WHEN_CONNECTED;
	}
	void BreakLink();

protected:

	void FillOutputWindow();
	static BOOL between(BYTE a, BYTE b, BYTE c)
	{
		return ((a <= b) && (b < c)) ||
			   ((c < a) && (a <= b)) ||
			   ((b < c) && (c < a));
	}

	void BecomeBroken()    
	{ 
		m_linkState = BROKEN;
		m_readIndex = m_inIndex = m_pktStart = 0;
		m_packetState = SEEKSOH;
		m_inPktNum = m_outPktNum = 0;
		m_ackDelay = TIMEOUT_WHEN_DISCONNECTED;
	}
public:
	
	BOOL IsBroken() const { return m_linkState == BROKEN; }

	// get the ponter to the header of the incoming packet
	pkthdr* InPacket() const
	{
		return (pkthdr*) (m_inBuffer + m_pktStart);
	}

	// return TRUE if we've got complete packet header
	BOOL CompleteHeader() const
	{
			
		return (m_inIndex - m_pktStart) >= sizeof(pkthdr);
	}


	static BOOL ValidHeader(const pkthdr* hdr)
	{
			
		return (hdr->soh == SOH) &&
			( NAK == hdr->ack || ACK == hdr->ack || ZAK == hdr->ack) &&
				(hdr->siz <=  512+sizeof(*hdr));
	}

	BOOL IncomingPacketComplete() const
	{
		return (CompleteHeader() && ValidHeader(InPacket()) &&
			((m_inIndex - m_pktStart) >= (int) InPacket()->siz));
	}

	BOOL IncomingPacketContainsAck() const
	{
		return InPacket()->ack == ACK;
	}  

	BOOL IncomingPacketContainsNak() const
	{
		return InPacket()->ack == NAK;
	}  

	BOOL IncomingPacketContainsData() const
	{
		return InPacket()->siz > sizeof(pkthdr);
	}
		
	BOOL CheckIncomingPacket() const
	{
		VPASSERT(IncomingPacketComplete());
		
		CCommPacket pkt(InPacket(), (LPVOID) (InPacket()+1));
			
		return pkt.GoodPacket();
	}

	void DiscardIncomingPacket()
	{
		m_inIndex = m_pktStart;
	}
	

	LPVOID  FreeBufferPosition()
	{
		return m_inBuffer + m_inIndex;
	}
	
	DWORD   FreeBufSize()
	{
		return PKTBUFSIZE - m_inIndex;
	}
	
	void AdvanceFreeBufferPosition(DWORD s)
	{
		m_inIndex += (int) s;
	}


	BOOL AckTimerExpired()
	{
		return (m_timer != 0) && ((GetCurrentTime() - m_timer) > m_ackDelay);
	}
	

	virtual DWORD ReadLinkData(LPVOID buf, DWORD bufSize);
	
	virtual BOOL FeedBuffer();
	virtual BOOL TransmitPacket(CCommPacket* d);
	virtual void SendAck(char ack, char pktNum);
	virtual void PiggyBackAck(char ack);
	virtual void StartAckTimer();
	virtual void StopAckTimer();

	virtual BOOL SeekSoh();

	virtual void OnDamagedInputPacket();
	virtual void OnValidInputPacket();
	virtual void OnControlPacket(CCommPacket& pkt);	
	virtual	BOOL ResendPacket();

	virtual BOOL BuildAndSendPacket(LPVOID data, 
						  DWORD dataSize, 
						  char pktType);
		

	BOOL   DataLinkReady() const { return m_linkState == CONNECTED; } 

	void AdjustReadIndex();
	
	virtual void OnTimer();

	virtual BOOL OnIncomingData();

	void Start();
	
	void FlushOutQueue();

	void SendControlPacket(char pktType, unsigned pktNum);

	CDataLink(CCommPort* port, CTDLogger* logger);
	virtual ~CDataLink();

	BOOL SendData(LPCVOID data, DWORD size, BOOL safe)
	{
		OnIncomingData();
		return BuildAndSendPacket((LPVOID) data, size, safe ? SDATA : UDATA);
	}


};

#endif
