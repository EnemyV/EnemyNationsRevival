
#ifndef __MDMDRVR_H__
#define __MDMDRVR_H__


class CCommPort;
class CDataLogger;


class ModemDriver
{

public:
	typedef void (*Callback)(void*);

	virtual void setCallback(Callback cbk, void* data); 

	
	enum ResponceCode { Nomatch, OK, Error, Nocarrier, Connect, Busy, Noanswer };

	struct ModemResponce
	{
		int code;
		const char* string;
	};
	
	ModemDriver(CCommPort* p);
	ModemDriver(CCommPort *p, const AStrList& modemProgram);
	virtual ~ModemDriver();

	void setLogger(CDataLogger* l) { logger = l; }	
	
	void setProgram(const AStrList& modemProgram);
	void setResponces(const ModemResponce* r, int count)
	{
		responces = r; responceCount = count;
	}
	
	bool done() const { return progPhase >= (int) prog.entries(); }
	bool stillWaiting() const { return waiting; }
	bool getResult() const { return result; } 

	void start();
	
	virtual void commEvent();
	virtual void timerEvent();

	
protected:

	virtual void timeout();
	
	
	void startTiming(unsigned long timeout) 
	{ 
		endTime = currentTime() + timeout;
	}

	
	static void  portCallback(void* );
	virtual unsigned long currentTime() = 0;

	virtual bool sendNextProgramString();
	
	virtual bool readAndMatch();

	virtual void readFromPort(AString& s);
	virtual void sendToPort(const char* p, int s);

	void send(const char* s) { sendToPort(s, strlen(s)); }
	void send(const AString& s) { sendToPort(s, s.length()); }
	
	void pushBack();


	void notifyClient();
		
protected:

	
	CCommPort*  port;
	AStrList     prog;
	int			 progPhase;
	AString 	 incomingData;
	bool         waiting;
	bool		 result;
	unsigned long endTime;
	const ModemResponce* responces;
	int          responceCount;
	Callback callBack;
	void*     cbData;
	CDataLogger* logger;
		
};

#endif

