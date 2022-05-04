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
AITHREAD      fnAiThread = NULL;


DWORD APIENTRY FromDll16 (LPVOID lpBuff, DWORD dwUserDefined)
{

	if (fnAiThread == NULL)
		return (0);
		
	switch (dwUserDefined)
	  {
		case THRDS_START_AI :
			fnAiThread (lpBuff);
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

void API_EXPORT WINAPI ediSetAiFunc (AITHREAD pfn)
{

	fnAiThread = pfn;
}

WORD API_EXPORT WINAPI ediGetThrdUtlsVersion()
{

	return ((WORD) (* pfnUTProc) (NULL, THRDS_VER, NULL));
}

void API_EXPORT WINAPI ediStartThread (void *pData)
{

	(* pfnUTProc) (pData, THRDS_START, NULL);
}

void API_EXPORT WINAPI ediEndTaskThread ()
{

	(* pfnUTProc) (NULL, THRDS_END, NULL);
}

WORD API_EXPORT WINAPI ediYieldThread ()
{

	return ((WORD) (* pfnUTProc) (NULL, THRDS_YIELD, NULL));
}


