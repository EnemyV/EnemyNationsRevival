#ifndef __DAVE32_H__
#define __DAVE32_H__


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#define TM_QUIT     0x0001         /* Thread has ended or must end */

typedef void (WINAPI * THREADFUNC) (void *pData);

#define DllExport	__declspec( dllexport )

enum { pri_low, pri_normal, pri_high };


extern "C"
{
DllExport void WINAPI ediEndTaskThread ();
DllExport WORD WINAPI ediGetThrdUtlsVersion();
DllExport void WINAPI ediSetAiFunc (THREADFUNC pfn);
DllExport DWORD WINAPI ediStartThread (void *pData);
DllExport WORD WINAPI ediYieldThread ();
DllExport void ediThreadPause (DWORD hdl, BOOL bPause);
}


#endif
