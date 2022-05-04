#include "stdafx.h"

#include "vpnet.h"
#include "commport.h"
#include "datagram.h"
#include "datalink.h"
#include "commnet.h"
#include "tapiutil.h"
#include "tapinet.h"
#include "datalog.h"
#include "resource.h"




CTapiNet::
  CTapiNet(CTDLogger* log, CCommPort* port,  LPCSTR callNumber, HINSTANCE hInst, LPCSTR iniFile, LPCSTR iniSection) :
     CCommNet(log, port), m_serverMode(FALSE), m_waitingConnect(FALSE),
		 m_connected(FALSE)
  {	

		m_tapiObj.m_net = this;
		m_hInst = hInst; 
		m_iniFile = strdup(iniFile ? iniFile : "");
		m_iniSection = strdup(iniSection ? iniSection : "");
		m_callNumber = strdup(callNumber ? callNumber : "");

  }


CTapiNet::~CTapiNet()
{
	free((LPSTR)m_iniFile);
	free((LPSTR)m_iniSection);
	free((LPSTR)m_callNumber);


}

void CTapiNet::OnConnect()
{
	m_connected = TRUE;
	m_waitingConnect = FALSE;

	if (m_dataLog)
		m_dataLog->Status (IDS_COMM_STAT_CONNECTED);

	m_port->AdoptPort(m_tapiObj.CommHandle());
}

void CTapiNet::OnDisconnect()
{
	m_connected = FALSE;
	OnLinkDown();
	m_port->Close();
}


void CTapiNet::OnDialFail()
{
	m_waitingConnect = FALSE;
	if (m_dataLog)
	{
		m_dataLog->Status (IDS_COMM_STAT_ERROR);
	}

}


void CVPTapiConn::OnConnect()
{
	m_net->OnConnect();
}

void CVPTapiConn::OnDisconnect()
{
	m_net->OnDisconnect();
}

void CVPTapiConn::OnDialFail()
{
	m_net->OnDialFail();
}




void CTapiNet::OnTimer()
{
	if (!m_connected)
	{   
		return;
	}
	CCommNet::OnTimer();
}
			

LRESULT CTapiNet::OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != WM_COMMNOTIFY)
		return FALSE;


	return CCommNet::OnMessage(hWnd, uMsg, wParam, lParam);
}


static void dummyProc(LPVOID ctx, LPCSTR devName, UINT devId)
{
}



BOOL CTapiNet::Listen(BOOL streamListen, BOOL serverMode)
{
	BOOL ret = FALSE;

	VPENTER(CTapiNet::Listen);
	
	m_serverMode = serverMode;
	m_streamListen = streamListen;

	if (m_connected)
	{
		VPLEAVEBOOL(CTapiNet::Listen, TRUE);
		return TRUE;
	}





	m_tapiObj.Create(m_hInst);
	m_tapiObj.EnumDevices(dummyProc, NULL);

	m_tapiObj.LoadConfig(m_iniFile, m_iniSection);
	m_tapiObj.m_cfgInfo.Edit(m_tapiObj.DeviceId());
	m_tapiObj.m_cfgInfo.Set(m_tapiObj.DeviceId());


	if (m_dataLog)
	{
		char devName[256];

		m_tapiObj.GetDeviceName(devName);
		m_dataLog->Start(devName);
		m_dataLog->SetCallback(CancelCbk, (LPVOID) this);

	}	

	if (!serverMode)
	{
		m_tapiObj.DialCall(m_callNumber);
		if (m_dataLog)
		{
			m_dataLog->Number(m_callNumber);
			m_dataLog->Status (IDS_COMM_STAT_DIAL);
		}
	}
	else
	{
		m_tapiObj.WaitForCall();
		if (m_dataLog)
			m_dataLog->Status (IDS_COMM_STAT_WAITCALL);

	}

	if (WaitModemConnection())
	{

		ret = CCommNet::Listen(serverMode, streamListen);
		if (m_dataLog)
		{
			m_dataLog->SetCallback(CancelCbk, this);
		}

	}

	VPLEAVEBOOL(CTapiNet::Listen, ret);

	return ret;
}


void CTapiNet::CancelCbk(LPVOID p)
{
	if (!p)
		return;

	CTapiNet* tNet =  (CTapiNet*) p;

	tNet->OnCancelCall();
}

void CTapiNet::OnCancelCall()
{
	m_waitingConnect = FALSE;
	m_tapiObj.HangupCall();
	vpDoAbortWait();
}


void CTapiNet::Cleanup()
{
	CCommNet::Cleanup();
//	m_tapiObj.HangupCall();
}


BOOL CTapiNet::ModemWaitTest(LPVOID context)
{
	 CTapiNet* net = (CTapiNet*) context;

	 if (net->m_waitingConnect)
		 net->m_waitingConnect = !net->m_connected;
	 
	 return !net->m_waitingConnect;
}


BOOL CTapiNet::WaitModemConnection()
{
	VPENTER(CTapiNet::WaitModemConnection);

	m_waitingConnect = TRUE;
	m_connected = FALSE;

	BOOL ret = vpWaitLoop(ModemWaitTest, this);
	
	m_waitingConnect = FALSE;
	if (ret)
		ret = m_connected;

	VPLEAVEBOOL(CTapiNet::WaitModemConnection, m_connected);
	return ret;
}


CCommNet* MakeTapiNet(CTDLogger* log, CCommPort* port,  LPCSTR callNumber, HINSTANCE hInst, LPCSTR iniFile, LPCSTR iniSection)
{
	return new CTapiNet(log, port, callNumber, hInst, iniFile, iniSection);
}
