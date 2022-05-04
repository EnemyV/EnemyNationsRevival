//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------



#include "stdafx.h"
#include "_windwrd.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


extern "C"
{
typedef WORD ( WINAPI * YIELDTHREAD_FUNC ) ();
extern YIELDTHREAD_FUNC pfnYieldThread;
}


CDiskCache theDiskCache;


CCacheElem::CCacheElem (HANDLE hFil, int iOff, int iLen, void * pBuf, void (* fnCall) (DWORD dwData), DWORD dwData)
{

	m_hFil = hFil;
	m_iOff = iOff;
	m_iLen = iLen;
	m_pBuf = pBuf;
	m_fnCallBack = fnCall;
	m_dwData = dwData;
}

void CDiskCache::ctor ()
{

	m_posOn = NULL;
	m_bKillMe = FALSE;
	m_dwThrd = NULL;
	m_hWnd = NULL;
}

void CDiskCache::Open (HWND hWnd)
{

	ctor ();
	m_hWnd = hWnd;
}

CDiskCache::~CDiskCache () 
{ 

	if (m_dwThrd == NULL)
		return;

	Close (); 
	
	if (m_dwThrd != NULL)
		TerminateThread (((CWinThread *) m_dwThrd)->m_hThread, 1);
}

void CDiskCache::KillAllRequests ()
{

	m_cs.Lock ();

	// remove all elements
	POSITION pos = m_lstRequests.GetHeadPosition ();
	while (pos != NULL)
		{
		POSITION posDel = pos;
		CCacheElem * pCceTst = m_lstRequests.GetNext (pos);
		// if we are not reading it we can kill it
		if (pCceTst != m_pCceOn)
			{
			TRAP ();
			delete pCceTst;
			m_lstRequests.RemoveAt (posDel);
			}
		else
			TRAP ();
		}

	int iOff;
	if (m_pCceOn != NULL)
		iOff = m_pCceOn->m_iOff;
	else
		iOff = -1;

	m_cs.Unlock ();

	if (iOff != -1)
		KillRequest (iOff);

	// grab any messages from the queue
	MSG msg;
	while (::PeekMessage (&msg, m_hWnd, MSG_CACHE, MSG_CACHE, PM_REMOVE))
		TRAP ();
}

void CDiskCache::Close ()
{

	m_cs.Lock ();

	// remove all elements
	POSITION pos = m_lstRequests.GetHeadPosition ();
	while (pos != NULL)
		{
		POSITION posDel = pos;
		CCacheElem * pCceTst = m_lstRequests.GetNext (pos);
		// if we are not reading it we can kill it
		if (pCceTst != m_pCceOn)
			{
			TRAP ();
			delete pCceTst;
			m_lstRequests.RemoveAt (posDel);
			}
		else
			TRAP ();
		}

	m_bKillMe = TRUE;

#ifdef BUGBUG
	if (m_dwThrd != NULL)
		{
		myThreadPause (m_dwThrd, FALSE);

		// boost it
		if (iWinType != W32s)
			((CWinThread *) m_dwThrd)->SetThreadPriority (THREAD_PRIORITY_ABOVE_NORMAL);
		}
#endif

	m_cs.Unlock ();
}

// does a synchronous read
void CDiskCache::SyncRequest (int hFil, int iOff, int iLen, void * pBuf)
{

	// sync read
	::SetFilePointer ((HANDLE) hFil, iOff, NULL, FILE_BEGIN);
	DWORD dwRead;
	::ReadFile ((HANDLE) hFil, pBuf, iLen, &dwRead, NULL);
	if (dwRead != (DWORD) iLen)
		ThrowError (ERR_CACHE_READ);
}

void CDiskCache::AddRequest (int hFil, int iOff, int iLen, void * pBuf, void (* fnCall) (DWORD dwData), DWORD dwData)
{

	// if we aren't going yet we do a sync read
//BUGBUG	if (m_dwThrd == NULL)
		{
		::SetFilePointer ((HANDLE) hFil, iOff, NULL, FILE_BEGIN);
		DWORD dwRead;
		::ReadFile ((HANDLE) hFil, pBuf, iLen, &dwRead, NULL);
		if (dwRead != (DWORD) iLen)
			ThrowError (ERR_CACHE_READ);

		// got it - tell the requestor
		(* fnCall) (dwData);
		return;
		}
		
#ifdef BUGBUG
	CCacheElem * pCce = new CCacheElem ((HANDLE) hFil, iOff, iLen, pBuf, fnCall, dwData);

	m_cs.Lock ();

	// add to the list sorted by offset
	POSITION pos = m_lstRequests.GetHeadPosition ();
	POSITION posLast = pos;
	while (pos != NULL)
		{
		CCacheElem * pCceTst = m_lstRequests.GetNext (pos);
		if (pCceTst->m_iOff >= iOff)
			break;
		posLast = pos;
		}
	if (posLast != NULL)
		m_lstRequests.InsertBefore (posLast, pCce);
	else
		m_lstRequests.AddTail (pCce);

	m_cs.Unlock ();

	// unblock if there were no requests
	if (m_lstRequests.GetCount () == 1)
		myThreadPause (m_dwThrd, FALSE);
#endif
}

void CDiskCache::KillRequest (int iOff)
{

	m_cs.Lock ();

	// find it
	POSITION pos = m_lstRequests.GetHeadPosition ();
	while (pos != NULL)
		{
		POSITION posDel = pos;
		CCacheElem * pCceTst = m_lstRequests.GetNext (pos);
		if (pCceTst->m_iOff == iOff)
			{
			TRAP ();
			// if we are not reading it we can kill it
			if (pCceTst != m_pCceOn)
				{
				TRAP ();
				delete pCceTst;
				m_lstRequests.RemoveAt (posDel);
				pos = NULL;
				}
			else
				TRAP ();
			break;
			}
		}

	m_cs.Unlock ();

	// if it wasn't the one in process we're done
	if (pos == NULL)
		return;

	// ok - we need to wait till it reads (when we return the buf may be deleted)
	while (TRUE)
		{
		m_cs.Lock ();

		if (m_pCceOn == NULL)
			{
			TRAP ();
			m_cs.Unlock ();
			return;
			}
		if (m_pCceOn->m_iOff != iOff)
			{
			TRAP ();
			m_cs.Unlock ();
			return;
			}

		m_cs.Unlock ();
		::Sleep (100);
		myYieldThread ();
		}
}

UINT CDiskCache::ThreadFunc (void * pData)
{

	( (CDiskCache *) pData )->_ThreadFunc ();

	return (0);
}

void CDiskCache::_ThreadFunc ()
{

//BUGBUG	if ( ( m_dwThrd = myGetThreadHdl () ) == 0 )
		ThrowError ( ERR_NO_THREAD_LIB );

#ifdef BUGBUG
	while (! m_bKillMe)
		{
		m_cs.Lock ();

		// get the next element
		if (m_posOn == NULL)
			m_posOn = m_lstRequests.GetHeadPosition ();

		// if nothing we block
		if (m_posOn == NULL)
			{
			m_cs.Unlock ();
			myThreadPause (m_dwThrd, TRUE);
			continue;
			}

		// we've got one
		m_pCceOn = m_lstRequests.GetAt (m_posOn);
		m_cs.Unlock ();

		// read it
		::SetFilePointer (m_pCceOn->m_hFil, m_pCceOn->m_iOff, NULL, FILE_BEGIN);
		DWORD dwRead;
		::ReadFile (m_pCceOn->m_hFil, m_pCceOn->m_pBuf, m_pCceOn->m_iLen, &dwRead, NULL);
		if (dwRead != (DWORD) m_pCceOn->m_iLen)
			ThrowError (ERR_CACHE_READ);

		// got it - tell the requestor
		::PostMessage ( m_hWnd, MSG_CACHE, (DWORD) m_pCceOn, 0 );

		// Win32s
		if (iWinType == W32s)
			if (pfnYieldThread () == TM_QUIT)
				break;

		// take it out of the list
		m_cs.Lock ();
		POSITION posDel = m_posOn;
		m_lstRequests.GetNext (m_posOn);
		m_lstRequests.RemoveAt (posDel);
		m_pCceOn = NULL;
		m_cs.Unlock ();
		}

	// all done - clean it out then kill the thread
	m_cs.Lock ();

	// remove all elements
	POSITION pos = m_lstRequests.GetHeadPosition ();
	while (pos != NULL)
		{
		POSITION posDel = pos;
		CCacheElem * pCceTst = m_lstRequests.GetNext (pos);
		delete pCceTst;
		m_lstRequests.RemoveAt (posDel);
		}
	m_lstRequests.RemoveAll ();

	m_cs.Unlock ();

	m_dwThrd = NULL;
	myThreadTerminate ();
#endif
}

// call back in the context of the main thread
void CDiskCache::ProcessMessage ( CCacheElem * pCce )
{

	(pCce->m_fnCallBack) (pCce->m_dwData);
	delete pCce;
}
