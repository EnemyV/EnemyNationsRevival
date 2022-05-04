#include "stdafx.h"

#include "astring.h"
#include "astrlist.h"
#include "mdmdrvr.h"
#include "commport.h"
#include "datalog.h"


#define LENGTH(x) sizeof(x)/sizeof(x[0])


static ModemDriver::ModemResponce responceTable[] =
{
	{ ModemDriver::OK, "OK" },
	{ ModemDriver::Error, "ERROR"},
	{ ModemDriver::Nocarrier, "NO CARRIER" },
	{ ModemDriver::Connect, "CONNECT" },
	{ ModemDriver::Busy, "BUSY" },
	{ ModemDriver::Noanswer, "NO ANSWER" } 
};
	
					


ModemDriver::ModemDriver(CCommPort* p) :  port(p),
  progPhase(0), responces(responceTable),
  responceCount(LENGTH(responceTable)), 
  waiting(FALSE), result(FALSE), endTime(0), callBack(0)
{
//!!	p->setCallback(portCallback, this);
}



ModemDriver::ModemDriver(CCommPort* p, const AStrList& modemProg) :  port(p),
  prog(modemProg), progPhase(0), responces(responceTable),
  responceCount(LENGTH(responceTable)), 
  waiting(FALSE), result(FALSE), endTime(0), callBack(0)
{
//!!	p->setCallback(portCallback, this);
}

ModemDriver::~ModemDriver()
{
#if 0
	if (port)
		port->setCallback(NULL, NULL);
#endif
}

void ModemDriver::setCallback(Callback cbk, void* data)
{ 
	callBack = cbk; cbData = data; 
}

void ModemDriver::portCallback(void* p)
{
	((ModemDriver*)p)->commEvent();
}

void ModemDriver::pushBack()
{
//	port->pushBack(incomingData.data(), incomingData.length());
}

int ModemDriver::readAndMatch()
{
	AString tmp;
	
	readFromPort(tmp);
	if (tmp.length())
	{

		incomingData += tmp;
		for(int i = 0; i < responceCount; i++)
		{
			int matchIndex = incomingData.find(responces[i].string);
			if ( -1 != matchIndex)
			{
				incomingData.cutFrom(matchIndex + 
					strlen(responces[i].string));
					
				return responces[i].code;
			}
		}	
		// we are here if no match found
		incomingData.cutLine();
	}
	incomingData.cutLine();
	return (Nomatch);
}




bool ModemDriver::sendNextProgramString()
{
	if (!done())
	{
		
		send(prog[progPhase++]);
		send("\r");
		return waiting = TRUE;
	}
	return (waiting = FALSE);
}


void ModemDriver::commEvent()
{
	switch(readAndMatch())
	{
		case OK:
			if (!sendNextProgramString())
			{
				result = TRUE;
				notifyClient();
			}
			break;
		case Nomatch:
			break;
		default:
			result = FALSE;
			progPhase = prog.entries();
			notifyClient();
			break;
	}

}

void ModemDriver::timerEvent()
{
	if (currentTime() > endTime)
		timeout();
	
}


void ModemDriver::timeout()
{
	result = FALSE;
	progPhase = prog.entries();
	notifyClient();
}

void ModemDriver::notifyClient()
{
	pushBack();
	if (callBack)
		callBack(cbData);
}	

void ModemDriver::start()
{
	startTiming(30*1000L);

	
	AString dummy;
	readFromPort(dummy);  // flush the pending input

	sendNextProgramString();
}



void ModemDriver::readFromPort(AString& s)
{
	
	s.resize(128);

	DWORD len = port->Read((void*) (const char*) s, 128);
	s.resize((size_t) len);
	if (logger)
		logger->IncomingData(s, (size_t) s.length());

}

void ModemDriver::sendToPort(const char* p, int s)
{
	DWORD t = currentTime();
	if (s)
	{
		while((currentTime() - t) < 50)
			  ;
		port->Write(p, s);
		if (logger)
			logger->OutgoingData(p, (size_t) s);
		
		t = currentTime();
		while((currentTime() - t) < 50)
			  ;
	}


}

