#ifndef __THREADS_H__
#define __THREADS_H__


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "dave32ut.h"

#define TM_QUIT     0x0001         /* Thread has ended or must end */

typedef void (WINAPI * THREADEXITFUNC) (void);

WORD myGetThrdUtlsVersion();
void myStartThread (void *pData, AFX_THREADPROC fnThread);
void myThreadClose (THREADEXITFUNC fnExit);
void myThreadInit (AITHREAD fnThread);
void myYieldThread ();
void myThreadTerminate ();
void myPauseThread ( BOOL bPause );

#endif
