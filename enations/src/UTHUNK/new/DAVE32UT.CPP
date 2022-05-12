/*++

Copyright (c) 1995 - Windward Studios

Module Name:

    thread32.c

--*/


#define W32SUT_32

#include <windows.h>
#include <w32sut.h>
#include "thread.h"
#include "dave32ut.h"

extern "C" {
BOOL APIENTRY DllInit (HANDLE hInst, DWORD fdwReason, LPVOID lpReserved);
}

typedef BOOL (WINAPI * PUTREGISTER) ( HANDLE     hModule,
                                      LPCSTR     lpsz16BitDLL,
                                      LPCSTR     lpszInitName,
                                      LPCSTR     lpszProcName,
                                      UT32PROC * ppfn32Thunk,
                                      FARPROC    pfnUT32Callback,
                                      LPVOID     lpBuff
                                    );


typedef VOID (WINAPI * PUTUNREGISTER) (HANDLE hModule);

UT32PROC      pfnUTProc = NULL;
PUTREGISTER   pUTRegister = NULL;
PUTUNREGISTER pUTUnRegister = NULL;
int           cProcessesAttached = 0;
BOOL          fWin32s = FALSE;
HANDLE        hKernel32 = 0;
THREADFUNC    fnAiThread = NULL;


DWORD APIENTRY FromDll16 (LPVOID lpBuff, DWORD dwUserDefined)
{

	if (fnAiThread == NULL)
		return (0);
		
	switch (dwUserDefined)
	  {
		case THRDS_START_AI :
	MessageBox (NULL, "before call thread", "test", MB_OK);
			fnAiThread (lpBuff);
	MessageBox (NULL, "after call thread", "test", MB_OK);
			break;
	  }

	return (0);
}

BOOL APIENTRY DllInit (HANDLE hInst, DWORD fdwReason, LPVOID lpReserved)
{

	switch (fdwReason)
	  {
	  case DLL_PROCESS_ATTACH: {
	    if ( cProcessesAttached++ )
  	    return (TRUE);         // Not the first initialization.

			OSVERSIONINFO ovi;
			memset (&ovi, 0, sizeof (ovi));
			ovi.dwOSVersionInfoSize = sizeof (ovi);
			if (! GetVersionEx (&ovi))
	      return (FALSE);        // Error
			if (ovi.dwPlatformId != VER_PLATFORM_WIN32s)
	      return (TRUE);        // its not Win32s

	    hKernel32 = LoadLibrary ("Kernel32.Dll"); // Get Handle to Kernel32.Dll
	    pUTRegister = (PUTREGISTER) GetProcAddress (hKernel32, "UTRegister");
	    if (! pUTRegister )
  	    return (FALSE);        // Error - On Win32s, can't find UTRegister
	    pUTUnRegister = (PUTUNREGISTER) GetProcAddress (hKernel32, "UTUnRegister");
	    if (! pUTUnRegister)
  	    return (FALSE);        // Error - On Win32s, can't find UTUnRegister

	    if ((*pUTRegister) (hInst, "dave16ut.DLL", "DllInit16", "UTProc", 
                           &pfnUTProc, (FARPROC) FromDll16, NULL))
				if (pfnUTProc != NULL)
					return (TRUE);

			MessageBox (NULL, "Error loading dave16ut.dll", "DAVE32UT", MB_OK);
			return (FALSE);
	  	break; }

		case DLL_PROCESS_DETACH :
			if ((--cProcessesAttached > 0) || (pUTUnRegister == NULL))
  	    return(TRUE);
      (*pUTUnRegister) (hInst);
      FreeLibrary (hKernel32);
			pUTUnRegister = NULL;
			hKernel32 = NULL;
			break;
	  }

	return (TRUE);
}

DllExport void WINAPI ediSetAiFunc (THREADFUNC pfn)
{

	fnAiThread = pfn;
}

DllExport WORD WINAPI ediGetThrdUtlsVersion()
{

	if (pfnUTProc == NULL)
		return (0);
		
	return ((WORD) (* pfnUTProc) (NULL, THRDS_VER, NULL));
}

DllExport DWORD WINAPI ediStartThread (THREADFUNC fnThread, void * pData, int iPri)
{

	MessageBox (NULL, "before StartThread", "test", MB_OK);
	fnAiThread = fnThread;
	DWORD dwRtn = (* pfnUTProc) (pData, THRDS_START, NULL);
	MessageBox (NULL, "after StartThread", "test", MB_OK);

	switch (iPri)
	  {
		case pri_low :
			iPri = THRDS_PRI_LOW;
			break;
		case pri_high :
			iPri = THRDS_PRI_HIGH;
			break;
		default:
			iPri = THRDS_PRI_NORMAL;
			break;
	  }

	(* pfnUTProc) ((void *) dwRtn, iPri, NULL);
	return dwRtn;
}

DllExport void WINAPI ediEndTaskThread ()
{

	(* pfnUTProc) (NULL, THRDS_END, NULL);
}

DllExport WORD WINAPI ediYieldThread ()
{

	return ((WORD) (* pfnUTProc) (NULL, THRDS_YIELD, NULL));
}

DllExport void WINAPI ediPauseThread (DWORD hdl, BOOL bPause)
{

	MessageBox (NULL, "before PauseThread", "test", MB_OK);
	if (bPause)
		(* pfnUTProc) ((void *) hdl, THRDS_PAUSE, NULL);
	else
		(* pfnUTProc) ((void *) hdl, THRDS_RESUME, NULL);
	MessageBox (NULL, "after PauseThread", "test", MB_OK);
}

DllExport DWORD WINAPI ediGetThreadId ()
{

	MessageBox (NULL, "before GetThreadId", "test", MB_OK);
	DWORD dwRtn = (* pfnUTProc) (NULL, THRDS_ID, NULL);
	MessageBox (NULL, "after GetThreadId", "test", MB_OK);
	return dwRtn;
}

DllExport void WINAPI ediEndThread (DWORD hdl)
{

	MessageBox (NULL, "before EndThread", "test", MB_OK);
	(* pfnUTProc) ((void *) hdl, THRDS_END_HDL, NULL);
	MessageBox (NULL, "after EndThread", "test", MB_OK);
}
