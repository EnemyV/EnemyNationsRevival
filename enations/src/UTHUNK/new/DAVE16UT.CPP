/*++

Copyright (c) 1995 - Windward Studios

Module Name:

    thread16.c

--*/

#ifndef APIENTRY
#define APIENTRY
#endif
#define W32SUT_16

#include <windows.h>
#include <w32sut.h>
#include "thread.h"
#include "thrdapi.h"

extern "C" {
int FAR PASCAL _export WEP (int bSystemExit);
DWORD FAR PASCAL _export UTInit (UT16CBPROC, LPVOID);
DWORD FAR PASCAL _export UTProc (LPVOID lpBuf, DWORD dwFunc);
DWORD FAR PASCAL _export DllInit16 (UT16CBPROC pCallback, LPVOID);
}

UT16CBPROC pfnUT32CallBack = NULL;


DWORD FAR PASCAL _export DllInit16 (UT16CBPROC pCallback, LPVOID)
{

	pfnUT32CallBack = pCallback;
	return (TRUE);
}

int FAR PASCAL _export WEP (int bSystemExit)
{

  return (1);
}

int FAR PASCAL LibMain (HANDLE, WORD, WORD, LPSTR)
{

  return (1);
}

DWORD FAR PASCAL _export UTInit (UT16CBPROC, LPVOID)
{

  return(1);
}

void FAR PASCAL __export thrd32Func (PThreadRec Thread, HWND hWnd, WORD wParam, LONG lParam)
{

	(*pfnUT32CallBack) ((LPVOID) lParam, THRDS_START_AI, NULL);
	ExitThread ();
}

DWORD FAR PASCAL _export UTProc (LPVOID lpBuf, DWORD dwFunc)
{
static FARPROC funcProc = NULL;

	switch (dwFunc)
		{
		case THRDS_VER :
			return (GetThrdUtlsVersion());

		case THRDS_START : {
 			funcProc = MakeProcInstance ((FARPROC) thrd32Func, GetWindowWord (GetActiveWindow (), GWW_HINSTANCE));
			PThreadRec thrd = StartThread (funcProc, 4000, NULL, 0, (LONG) lpBuf);
			return ((DWORD) thrd); }

		// set the priority
		case THRDS_PRI_LOW :
			SetThreadPriority ((PThreadRec) lpBuf, 80);
			return (0);
		case THRDS_PRI_NORMAL :
			SetThreadPriority ((PThreadRec) lpBuf, 100);
			return (0);
		case THRDS_PRI_HIGH :
			SetThreadPriority ((PThreadRec) lpBuf, 120);
			return (0);

		case THRDS_END :
			EndTaskThreads (GetCurrentTask ());
			FreeProcInstance (funcProc);
			return (0);

		case THRDS_END_HDL :
			RemoveThread ((PThreadRec) lpBuf);
			TerminateThread ((PThreadRec) lpBuf);
			FreeProcInstance (funcProc);
			return (0);

		case THRDS_YIELD :
			return (YieldThread ());

		case THRDS_ID :
			return ((DWORD) GetCurrentThread ());

		case THRDS_PAUSE :
			SetThreadPause ((PThreadRec) lpBuf, TRUE);
			return (0);
		case THRDS_RESUME :
			SetThreadPause ((PThreadRec) lpBuf, FALSE);
			return (0);
		}

  return ((DWORD) -1L);
}
