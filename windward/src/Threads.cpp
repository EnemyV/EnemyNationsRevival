//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------



#include "stdafx.h"
#include "_windwrd.h"
#include "_res.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

CRITICAL_SECTION cs;
BOOL bEndThreads = FALSE;
CObList lstThrds;


extern "C"
{
typedef void ( WINAPI * ENDTASKTHREAD_FUNC ) ();
typedef WORD ( WINAPI * GETTHREADVERSION_FUNC ) ();
typedef void ( WINAPI * SETAIFUNC_FUNC ) (AITHREAD pfn);
typedef DWORD ( WINAPI * STARTTHREAD_FUNC ) ( void * pData );
typedef WORD ( WINAPI * YIELDTHREAD_FUNC ) ();

static ENDTASKTHREAD_FUNC pfnEndTaskThread = NULL;
static GETTHREADVERSION_FUNC pfnGetThrdUtlsVersion = NULL;
static SETAIFUNC_FUNC pfnSetAiFunc = NULL;
static STARTTHREAD_FUNC pfnStartThread = NULL;
static YIELDTHREAD_FUNC pfnYieldThread = NULL;
}


class xThread
{
public:
	xThread () 	{ m_hLib = NULL; }
	~xThread () 	{ if (m_hLib != NULL) FreeLibrary (m_hLib); m_hLib = NULL; }

	HINSTANCE m_hLib;
};

static xThread xt;


void myThreadInit (AITHREAD fnThread)
{

	if (iWinType != W32s)
		return ;
		
	if (xt.m_hLib == NULL)
		{
		xt.m_hLib = LoadLibrary ("dave32ut.dll");
		if (xt.m_hLib == NULL)
			{
			AfxMessageBox (IDS_NO_THREAD_LIB, MB_OK | MB_ICONSTOP);
			ThrowError (ERR_NO_THREAD_LIB);
			}

		pfnEndTaskThread = (ENDTASKTHREAD_FUNC) GetProcAddress (xt.m_hLib, "_ediEndTaskThread@0");
		pfnGetThrdUtlsVersion = (GETTHREADVERSION_FUNC) GetProcAddress (xt.m_hLib, "_ediGetThrdUtlsVersion@0");
		pfnSetAiFunc = (SETAIFUNC_FUNC) GetProcAddress (xt.m_hLib, "_ediSetAiFunc@4");
		pfnStartThread = (STARTTHREAD_FUNC) GetProcAddress (xt.m_hLib, "_ediStartThread@4");
		pfnYieldThread = (YIELDTHREAD_FUNC) GetProcAddress (xt.m_hLib, "_ediYieldThread@0");

		if ((pfnEndTaskThread == NULL) || (pfnGetThrdUtlsVersion == NULL) || 
										(pfnSetAiFunc == NULL) || (pfnStartThread == NULL) ||
										(pfnYieldThread == NULL))
			{
			AfxMessageBox (IDS_BAD_THREAD_LIB, MB_OK | MB_ICONSTOP);
			ThrowError (ERR_NO_THREAD_LIB);
			}

		if (pfnGetThrdUtlsVersion () < 256)
			{
			AfxMessageBox (IDS_BAD_THREAD_VER, MB_OK | MB_ICONSTOP);
			ThrowError (ERR_NO_THREAD_LIB);
			}
		}

	pfnSetAiFunc (fnThread);
}

volatile int iThrdsLeft=0;

void myThreadClose (THREADEXITFUNC fnExit)
{
static int iRecurse = 0;

	// we are NOT re-entrant here
	EnterCriticalSection (&cs);
	if (iRecurse > 0)
		{
		LeaveCriticalSection (&cs);
		return;
		}
	iRecurse++;

	if (iWinType == W32s)
		iThrdsLeft = 1;
	else
		{
		iThrdsLeft = lstThrds.GetCount ();
		// if no threads, leave
		if (iThrdsLeft == 0)
			{
			LeaveCriticalSection (&cs);
			fnExit ();
			iRecurse--;
			return;
			}
		}

	bEndThreads = TRUE;

	// we have to get these guys moving
	POSITION pos;
	for (pos = lstThrds.GetHeadPosition(); pos != NULL; )
		{
		CWinThread * pThrd = (CWinThread *) lstThrds.GetNext (pos);
		if (pThrd != NULL)
			if (AfxIsValidAddress (pThrd, sizeof (CWinThread)))
				if (pThrd->m_hThread)
					{
					ASSERT_VALID (pThrd);
					pThrd->SetThreadPriority (THREAD_PRIORITY_ABOVE_NORMAL);
					}
		}

	LeaveCriticalSection (&cs);

	// do this to get the threads to end
	DWORD dwEnd = timeGetTime () + 3000;
	while ((iThrdsLeft > 0) && (timeGetTime () < dwEnd))
		{
		::Sleep (0);
		MSG msg;
		while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
			{
			TranslateMessage (&msg);
			DispatchMessage (&msg);
			}
		}

	// see if anyone didn't make it
	if (iWinType == W32s)
	  {
		TRAP ();
		EnterCriticalSection (&cs);
		pfnEndTaskThread ();
		lstThrds.RemoveAll ();
		LeaveCriticalSection (&cs);
	  }
	else
	  
		{
		EnterCriticalSection (&cs);
		for (pos = lstThrds.GetHeadPosition(); pos != NULL; )
			{
			CWinThread * pThrd = (CWinThread *) lstThrds.GetNext (pos);
			if (pThrd != NULL)
				if (AfxIsValidAddress (pThrd, sizeof (CWinThread)))
					if (pThrd->m_hThread)
						{
						ASSERT_VALID (pThrd);
						TerminateThread (pThrd->m_hThread, 1);
						delete pThrd;
						iThrdsLeft--;
						}
			}
		lstThrds.RemoveAll ();
		LeaveCriticalSection (&cs);
		}

	// last call ever for the AI
	fnExit ();
	bEndThreads = FALSE;
	iRecurse--;
}

void myThreadTerminate ()
{

	// do this with a call to the AI
	if (iWinType != W32s)
		{
		// remove from our list
		EnterCriticalSection (&cs);
		iThrdsLeft--;
		CWinThread *pCurrent = AfxGetThread ();
		POSITION pos;
		for (pos = lstThrds.GetHeadPosition(); pos != NULL; )
			{
			POSITION posOn = pos;
			CWinThread * pThrd = (CWinThread *) lstThrds.GetNext (pos);
			if (pThrd == pCurrent)
				lstThrds.RemoveAt (posOn);
			}
		LeaveCriticalSection (&cs);
		AfxEndThread (0);
		}
	else
	  {
		TRAP ();
		pfnEndTaskThread ();
		iThrdsLeft--;
	  }
}

WORD myGetThrdUtlsVersion()
{

	if (iWinType != W32s)
		return (0);
		
	return (pfnGetThrdUtlsVersion ());
}

void myStartThread (void *pData, AFX_THREADPROC fnThread)
{

	bEndThreads = FALSE;

	if (iWinType != W32s)
		{
		CWinThread *pThrd = AfxBeginThread (fnThread, pData, THREAD_PRIORITY_BELOW_NORMAL);
		lstThrds.AddTail (pThrd);
		return;
		}
	
	pfnStartThread ( pData );
}

void myYieldThread ()
{

	// is it time to end it?
	if (bEndThreads)
		{
		myThreadTerminate ();
		TRAP ();
		}
		
	if (iWinType == W32s)
		if (pfnYieldThread () == TM_QUIT)
			{
			TRAP ();
			myThreadTerminate ();
			TRAP ();
			}
}

void myPauseThread ( BOOL bPause )
{

	if (iWinType == W32s)
		return;

	// we have to get these guys moving
	POSITION pos;
	for (pos = lstThrds.GetHeadPosition(); pos != NULL; )
		{
		CWinThread * pThrd = (CWinThread *) lstThrds.GetNext (pos);
		if (pThrd != NULL)
			if (AfxIsValidAddress (pThrd, sizeof (CWinThread)))
				if (pThrd->m_hThread)
					{
					ASSERT_VALID (pThrd);
					if ( bPause )
						pThrd->SuspendThread ();
					else
						pThrd->ResumeThread ();
					}
		}
}
