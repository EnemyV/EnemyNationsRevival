#include "stdafx.h"

#include "vpnet.h"
#include "datalog.h"


CNetAddress::AddrList* CNetAddress::m_pool = NULL;

void* CNetAddress::operator new(size_t s)
{
	char* p;

	if (!m_pool)
	{
		p = new char[s];
	}
	else
	{
		p = (char*) m_pool->RemoveFirst();
		if (!p)
			p = new char[s];
	}
	return p;
}

void CNetAddress::operator delete(void* p)
{
	if (!m_pool)
		delete [] (char*) p;

	m_pool->Insert((CNetAddress*)p);
}
	


void CNetAddress::ResetPool()
{

	if (!m_pool)
	{
		m_pool = new AddrList;
		return;
	}

	CNetAddress* a;

	AddrList* list = m_pool;
	m_pool = NULL;

	while(NULL != (a = list->RemoveFirst()))
		delete a;

	m_pool = list;
}



void CNetLink::SetError(DWORD e, DWORD xe)
{
	m_err = e;
	if (e)
		m_net->m_errLog->SetError(VP_ERR_NET_ERROR, e, xe);
}

CNetInterface::CNetInterface(CTDLogger* log) : 
		m_safeHook(NULL), m_unsafeHook(NULL), 
		m_connectHook(NULL), m_disconnectHook(NULL), m_acceptHook(NULL) , m_errLog(log), m_err(0),
		m_dataLog(NULL), m_hostWin(NULL), m_lastStopTime(0), m_serverSeen(FALSE)
{
			CNetAddress::ResetPool();
}

CNetInterface::~CNetInterface()
{
	StopDataLog();
	CNetAddress::ResetPool();
}



LRESULT CNetInterface::OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

void CNetInterface::OnTimer() 
{
}



void CNetInterface::StartDataLog(CDataLogger* l) 
{
	m_dataLog = l;
}

void CNetInterface::StopDataLog() 
{
	if (m_dataLog)
	{
		m_dataLog->Stop();
		delete m_dataLog;
		m_dataLog = NULL;
	}
}


CNetAddress* CNetInterface::MakeAddressFromString(LPCSTR addr)
{
	return NULL;
} 

CNetAddress* CNetInterface::MakeRegistrationAddress()
{
	return NULL;
}

void CNetInterface::SetRegistrationAddress(LPCSTR addr)
{
}


void CNetInterface::SetError(DWORD e, DWORD xe)
{
	 m_err = e;
	 m_xerr = xe;
	 if (e && (m_errLog != NULL))
		m_errLog->SetError(VP_ERR_NET_ERROR, e, xe);
}

	
 void CNetInterface::FatalError(DWORD e, DWORD xe)
 {
 	m_err = e;
 	m_xerr = xe;

	if (m_errLog)
		m_errLog->SetFatalError(VP_ERR_NET_ERROR, e, xe);
 }


 void CNetInterface::Log(LPCSTR m)
 {
	if (m_errLog)
		m_errLog->Log(m);
 }



 void CNetInterface::SuspendHost()
 {
	if (m_hostWin)
	{
		DWORD ct = GetCurrentTime();

		if (ct - m_lastStopTime > 500)
		{
			Log("CNetInterface::SuspendHost");
			PostMessage(m_hostWin, WM_VPFLOWOFF, 0, 0);
			m_lastStopTime = GetCurrentTime();
			if (!m_lastStopTime)
				m_lastStopTime = 1;
		}
	}
 }


  void CNetInterface::ResumeHost()
 {
	if ((NULL != m_hostWin) && ( 0 != m_lastStopTime))
	{
 		Log("CNetInterface::ResumeHost");
		PostMessage(m_hostWin, WM_VPFLOWON, 0, 0);
		m_lastStopTime = 0;
	}
 }


