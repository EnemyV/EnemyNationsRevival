#ifndef __DIALER_H__
#define __DIALER_H__



class CCommPort;

class Dialer: public ModemDriver
{
  public:
	
	typedef ModemDriver::Callback Callback;
	
	void setCallback(Callback cbk, void* data);

	
	
	enum DialState { 
		             IDLE, INITMODEM, DIALING,  LISTENING, WAITCARRIER, 
						 CONNECTED, FAILED,
	                 
	               };

	Dialer(CCommPort* p, const AStrList& initList);
	virtual ~Dialer();
	
	virtual void setup(const char* pfx, const char* sfx);
	virtual void dial(const char* number, unsigned timeout=60);
	virtual void waitForCall(unsigned noYes);
	virtual bool abortCall();
	
	virtual int status() const { return (int) state; }

	bool done() const { return status() >= CONNECTED; }
	bool running() const { return !done(); }

	DWORD currentTime();
		
	virtual bool hasCarrier();
	virtual void commEvent();

protected:

	bool initDone() const
	{
		return ModemDriver::done() && !stillWaiting();
	}
	
	virtual void initModem(DialState targetState);
	
	virtual void sendDialString();

	void notifyClient();
	
	static void callbackHook(void* );
	
	

	virtual void changeState(DialState nextState);

	virtual void doInit();
	virtual void doDial();
	virtual void doListen();
	virtual void doWaitCarrier();
	
	virtual void timeout();

	
	virtual void openPort();
	virtual void closePort();



	

  protected:
	Callback callBack;
	void*     cbData;

	DialState targetState;
	DialState state;
	AString callNumber;
	AString dialPrefix;
	AString dialSuffix;
};



#endif
