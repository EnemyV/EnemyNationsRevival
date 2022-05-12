#ifndef __NDLINK_H__
#define	__NDLINK_H__




class CNDataLink: public CDLinkInt
{


public:

	typedef int SeqNr;

	
	struct framehdr
	{
		unsigned char	kind;
		unsigned char   seq;
		unsigned char	ack;
		unsigned short  siz;
	};

	class CDataFrame
	{
		public:

			framehdr m_hdr;
			LPVOID	 m_info;
			size_t	 m_infoSize;

			CDataFrame()
			{
				m_info = NULL;
				m_infoSize = 0;
			}

			int Ack() const { return m_hdr.ack; }
			int Seq() const { return m_hdr.seq; }
			unsigned char Kind() const { return m_hdr.kind; }
 
			size_t InfoSize() const { return m_infoSize; }
			LPVOID Info() const { return  m_info; }

			void SetAck(SeqNr v) { m_hdr.ack = (unsigned char) v; }
			void SetSeq(SeqNr v) { m_hdr.seq = (unsigned char) v; }
			void SetKind(char v) { m_hdr.kind = (unsigned char) v; }
			void SetInfo(LPVOID i, size_t s)
			{
				m_info = i;
				m_infoSize = s;
				m_hdr.siz = sizeof(m_hdr)+s;
			}
	};


	class IDataFrame
	{
	public:
		framehdr& m_hdr;
		
		IDataFrame(LPVOID data) : m_hdr(*(framehdr*) data)
		{
		}

		int Ack() const { return m_hdr.ack; }
		int Seq() const { return m_hdr.seq; }
		unsigned char Kind() const { return m_hdr.kind; }
	};


	enum { DATAFRAME = 'd', ACKFRAME='a', NAKFRAME='n' };

	void inc(SeqNr& ctr)
	{
		ctr = (ctr + 1) % (m_maxSeq+1);
	}

	static BOOL between(SeqNr a, SeqNr b, SeqNr c)
	{
		return ((a <= b) && (b < c)) ||
			   ((c < a) && (a <= b)) ||
			   ((b < c) && (c < a));
	}


	virtual void OnTimer();
	virtual void OnIncomingData();
	virtual void BecomeBroken();
	

protected:




	void	SendDatagram(Datagram* d);
	void    OnIncomingFrame(CDataFrame& fr);


	void DisableHost();
	void EnableHost();


	void StartCtlTimer()
	{
		m_ctlAckTimer = GetCurrentTime();
	}

	void StopCtlTimer()
	{
		m_ctlAckTimer = 0;
	}

	void StartFrameTimer(SeqNr fNumber);
	void StopFrameTimer(SeqNr fNumber);
	
	void StartHostTimer()
	{
		m_hostTimer = GetCurrentTime();
	}

	void StopHostTimer()
	{
		m_hostTimer = 0;
	}


	void SendFrame(int fKind,  SeqNr fNumber);

	
	void  OnFrameTimeout();
	void  OnHostTimeout();
	void  OnCtlFrameTimeout();

	void  TransmitFrame(CDataFrame& fr);
	void  TransmitData(LPVOID data, size_t s);
	void  ToHost(Datagram* d);

protected:
	int			m_maxSeq;
	int			m_nBufs;
	SeqNr 		m_ackExpected;
	SeqNr		m_nextFrameToSend;
	SeqNr		m_frameExpected;
	SeqNr		m_tooFar;
	SeqNr		m_oldestFrame;
	Datagram**	m_outBuf;
	Datagram**  m_inBuf;
	SeqNr*		m_outSeq;
	SeqNr		m_nBuffered;
	BOOL		m_noNak;
	DWORD		m_hostTimer;
	DWORD		m_ctlAckTimer;
	DWORD		m_frameTimer;
	DWORD		m_hostDelay;
	DWORD		m_frameAckDelay;
	DWORD		m_ctlAckDelay;
	
	DataQueue   m_fromHost;
	DataQueue	m_toHost;
	BOOL		m_hostEnabled;
	int			m_resends;
	int			m_resendLimit;


};


#endif
