#include "stdafx.h"

#include "vpnet.h"
#include "commport.h"
#include "datagram.h"
#include "datalink.h"
#include "commnet.h"
#include "mdmnet.h"
#include "datalog.h"


#include "astring.h"
#include "astrlist.h"
#include "mdmdrvr.h"
#include "dialer.h"



CModemNet::
  CModemNet(CTDLogger* log, CCommPort* port,  LPCSTR listenInit, LPCSTR mdmInit, LPCSTR callNumber, LPCSTR pfx, LPCSTR sfx) :
     CCommNet(log, port), m_serverMode(FALSE)
  {	
		m_needCarrier = FALSE;

		m_dialPrefix = NULL;
		m_dialSuffix = NULL;

		m_listenInit = strdup(listenInit ? listenInit : "");
		m_mdmInit = strdup(mdmInit ? mdmInit : "");
		m_callNumber = strdup(callNumber ? callNumber : "");
		if (pfx)
			m_dialPrefix = strdup(pfx);

		if (sfx) 
			m_dialSuffix = strdup(sfx);

		m_dialer = NULL;
		m_waitingConnect = FALSE;
  }


CModemNet::~CModemNet()
{
	free((LPSTR)m_listenInit);
	free((LPSTR)m_mdmInit);
	free((LPSTR)m_callNumber);
	if (m_dialPrefix)
	  free(m_dialPrefix);

	if (m_dialSuffix)
	  free(m_dialSuffix);


	delete m_dialer;


}

void CModemNet::DialerCbk(LPVOID p)
{
	CModemNet* mNet = (CModemNet*) p;

	mNet->OnDialerEvent();
}

void CModemNet::OnDialerEvent()
{
	if (m_dialer->status() != Dialer::CONNECTED)
	{
		Log("CModemNet::OnDialerEvent - connection failed");
		SetError(VPNET_ERR_INIT);
	}
	else
	{
		Log("CModemNet::OnDialerEvent - connected");
		m_lastCarrier = TRUE;
	}
	m_waitingConnect = FALSE;

}

void CModemNet::OnTimer()
{
	if (m_dialer && !m_dialer->done())
	{   
#ifndef WIN32
		if (m_port->ClearEvent() & EV_RXCHAR)
			m_dialer->commEvent();
#endif		
		m_dialer->timerEvent();
#if 0
		if (m_dialer->done() && !m_dialer->hasCarrier())
		{
			SetError(VPNET_ERR_INIT);
		}
#endif
		return;
	}
	CCommNet::OnTimer();
}
			

LRESULT CModemNet::OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != WM_COMMNOTIFY)
		return FALSE;

	if (m_dialer)
	{  
		if (!m_dialer->done())
		{
			m_port->ClearEvent();
			m_dialer->commEvent();
#if 0
			if (!m_dialer->done())
				return TRUE;
#endif
		}
		else if (m_lastCarrier) // m_dialer->hasCarrier())
		{
			return CCommNet::OnMessage(hWnd, uMsg, wParam, lParam);
		}
	}
	else
	{
		return CCommNet::OnMessage(hWnd, uMsg, wParam, lParam);
	}
	return TRUE;
}




BOOL CModemNet::Listen(BOOL streamListen, BOOL serverMode)
{
	VPENTER(CModemNet::Listen);
	BOOL ret = FALSE;
	m_serverMode = serverMode;

	if (!m_port->HasCarrier())
	{
		if (!CCommNet::Listen(streamListen, serverMode))
		{
			goto done;
		}

		AString initCmd;
		AStrList initList;
		
		initCmd = m_mdmInit;
		   
		if (initCmd.length())
			initList.append(initCmd);

		if (m_serverMode)
		{
			initCmd = m_listenInit;
			if (initCmd.length())
				initList.append(initCmd);
		}


		m_dialer = new Dialer(m_port, initList);

		m_dialer->setLogger(m_dataLog);
		m_dialer->setCallback(DialerCbk, this);

		m_dialer->setup(m_dialPrefix, m_dialSuffix);
		if (m_dataLog)
		{
			m_dataLog->SetCallback(CancelCbk, (LPVOID) this);
		}	

		if (m_serverMode)
		{
			m_dialer->waitForCall(TRUE);
		}
		else
		{
			m_dialer->dial(m_callNumber);
		}

		if (!WaitModemConnection())
		{
			delete m_dialer;
			m_dialer = NULL;
			goto done;
		}

		StartDataLink();
		ret = WaitDataLink();

	}
	else
	{	// we already have the carrier... 
		ret = TRUE;
	}

done:
	VPLEAVEBOOL(CModemNet::Listen, ret);

	return ret;

}



BOOL CModemNet::ModemWaitTest(LPVOID context)
{
	 CModemNet* net = (CModemNet*) context;

	 if (!net->m_port->IsOpen())
		 net->m_waitingConnect = FALSE;
	
	 if (net->m_waitingConnect)
		 net->m_waitingConnect = net->m_dialer && !net->m_dialer->done();
	 
	 return !net->m_waitingConnect;
}



BOOL CModemNet::WaitModemConnection()
{
	VPENTER(WaitModemConnection);
	m_waitingConnect = TRUE;

	BOOL ret = vpWaitLoop(ModemWaitTest, this);

	m_waitingConnect = FALSE;

	if (ret)
		ret = (m_dialer && (m_dialer->status() == Dialer::CONNECTED));

	VPLEAVEBOOL(WaitModemConnection, ret);
	return ret;

}


void CModemNet::CancelCbk(LPVOID p)
{
	if (!p)
		return;

	CModemNet* mNet =  (CModemNet*) p;

	mNet->OnCancelCall();
}

void CModemNet::OnCancelCall()
{
	m_waitingConnect = FALSE;

#if 0
	if (m_serverMode)
	{
		Log("Cancel has no meaning in server mode\n");
		return;
	}
#endif

	if (m_dialer)
		m_dialer->abortCall();

	CCommNet::OnCancel();
}


void CModemNet::Cleanup()
{
	delete m_dialer;
	m_dialer = NULL;
	CCommNet::Cleanup();

}
