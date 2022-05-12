#include "stdafx.h"

#include "resource.h"
#include "astring.h"
#include "astrlist.h"
#include "mdmdrvr.h"
#include "dialer.h"
#include "commport.h"
#include "datalog.h"

#define LENGTH(x) sizeof(x)/sizeof(x[0])

#define CARRIER_TIMEOUT	10000L




Dialer::Dialer(CCommPort* p, const AStrList& strings) : 
  state(IDLE),
  ModemDriver(p, strings),
  callBack(NULL), cbData(NULL)
{
//!!	p->setCallback(portCallback, this);

	dialPrefix = "ATD";
	dialSuffix = "\r";
	ModemDriver::setCallback(callbackHook, NULL);
}


void Dialer::setCallback(Callback cbk, void* data)
{ 
	callBack = cbk; cbData = data; 
}


Dialer::~Dialer()
{
}


void Dialer::callbackHook(void* p)
{
}



void Dialer::changeState(DialState newState)
{
	state = newState;
	if (!logger)
		return;

	switch(state)
	{
		case INITMODEM:
			logger->Status (IDS_COMM_STAT_INIT);
			break;
		case DIALING:
			logger->Number (callNumber);
			logger->Status (IDS_COMM_STAT_DIAL);
			break;
		case LISTENING:
			logger->Status (IDS_COMM_STAT_WAITCALL);
			break;
		case WAITCARRIER:
			logger->Status (IDS_COMM_STAT_CONNECT);
			break;
		case CONNECTED:
			logger->Status (IDS_COMM_STAT_CONNECTED);
			break;
		case FAILED:
			logger->Status (IDS_COMM_STAT_ERROR);
			break;
		default:
			break;
	}


}

void Dialer::commEvent()
{
	switch(state)
	{
		case INITMODEM:
			doInit();
			break;
		case DIALING:
			doDial();
			break;
		case LISTENING:
			doListen();
			break;
		case WAITCARRIER:
			doWaitCarrier();
			break;
		case CONNECTED:
			break;
		case FAILED:
			break;
		default:
			break;
	}
}


void Dialer::initModem( DialState tState)
{
	changeState(INITMODEM);
	targetState = tState;
	
	start();
	if (initDone()){
		if (targetState == DIALING)
		{
			sendDialString();
		}
		changeState(targetState);
		targetState = IDLE;
	}

}


void Dialer::notifyClient()
{
	pushBack();
	if (state == CONNECTED)
	{
		// following data are not for dialer
//		port->setCallback(NULL, NULL);
	}
	if (callBack)
	{
		callBack(cbData);
	}

}
		

void Dialer::doDial()
{
	int code = readAndMatch();
	int logCode = 0;
	
	switch(code)
	{
		case ModemDriver::Connect:
			changeState(WAITCARRIER);
			startTiming(CARRIER_TIMEOUT);			
			break;
		case ModemDriver::Nomatch:
			break;

		case ModemDriver::Error:
			logCode = IDS_COMM_STAT_ERROR;
			break;

		case ModemDriver::Nocarrier:
			logCode = IDS_COMM_STAT_NO_CONNECT;
			break;
		case ModemDriver::Busy:
			logCode = IDS_COMM_STAT_BUSY;
			break;

		case ModemDriver::Noanswer: 
			logCode = IDS_COMM_STAT_NO_ANSWER;
			break;

		default:
			break;
	}

  	
  	if (logCode)
	{
		changeState(FAILED);
		notifyClient();

		if (logger)
			logger->Status(logCode);

	}

}

void Dialer::doWaitCarrier()
{
	if (hasCarrier())
	{
		changeState(CONNECTED);
		notifyClient();
	}
}


void Dialer::setup(const char* pfx, const char* sfx)
{
	if (pfx && strlen(pfx))
		dialPrefix = pfx;

	if (sfx && strlen(sfx))
		dialSuffix = sfx;
}
	

void Dialer::doListen()
{
	switch(readAndMatch())
	{
		case ModemDriver::Connect:
			changeState(WAITCARRIER);
			startTiming(CARRIER_TIMEOUT);
			break;
		case ModemDriver::Nomatch:
			break;
		default:
			changeState(FAILED);
			notifyClient();
			break;
	}
}
		

void Dialer::doInit()
{

	ModemDriver::commEvent();
	if (initDone())
	{
		if (getResult())
		{
			if (targetState == DIALING)
			{
				sendDialString();
			}
			changeState(targetState);
			targetState = IDLE;
		}
		else
		{
			changeState(FAILED);
			notifyClient();
		}
	}

}


		

void Dialer::sendDialString()
{
	AString tmp;

	if (logger)
		logger->Number (callNumber);
	
	tmp = dialPrefix;
	tmp += callNumber;
	tmp += dialSuffix;

	if (~0u == tmp.findChar('\r'))
		tmp += "\r";

	send(tmp);

			
	changeState(DIALING);
}
	



void Dialer::waitForCall( unsigned noYes)
{
	if (noYes)
	{
		initModem(LISTENING);
	}
	else
	{
		changeState(IDLE);
	}
}


void Dialer::dial(const char* number, unsigned timeout)
{
	openPort();
	if (state == FAILED)
		return;
	
	AString tmp = number;	
	callNumber = tmp;
	startTiming(timeout*1000L);
	initModem(DIALING);
};
		

bool Dialer::abortCall()
{
	if (!hasCarrier())
	{
		send("\r");
		changeState(FAILED);
		notifyClient();
		return TRUE;
	}
	return FALSE;
}



void Dialer::timeout()
{
	if (INITMODEM == state || state == DIALING)
	{
		
		changeState(FAILED);


		if (INITMODEM == state)
		  ModemDriver::timeout();

		
		notifyClient();
	}
		
}

void Dialer::openPort()
{
}

void Dialer::closePort()
{
}



bool Dialer::hasCarrier()
{

	return (port->HasCarrier());
}

DWORD Dialer::currentTime()
{
	return GetCurrentTime();
}
