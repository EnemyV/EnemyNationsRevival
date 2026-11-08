#ifndef __THREADS_H__
#define __THREADS_H__


//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
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

// Worker-generation support: myThreadClose() bumps the generation, so a
// straggler leaked by the close (BUGS #65) still self-terminates at its next
// check even after myStartThread() re-arms bEndThreads=FALSE for a new game.
// Workers capture myThreadGen() at start and poll myThreadShouldExit() with it.
DWORD myThreadGen ();
BOOL  myThreadShouldExit (DWORD dwGenAtStart);

// AIRTIGHT #65 tail (linux2's 3/3 start->quit->start crash; win's second-entry
// window death): block until every leaked straggler from the previous close has
// actually exited AND the deferred AI teardown has run, so a new game's AiInit
// can never delete/recreate pGameData/plAIMgrList under a live zombie.
// Pumps messages while waiting (call from the main thread during a loading
// phase). Returns the number of zombies still alive at return (0 = clean;
// nonzero only if dwMaxWaitMs expired on a truly wedged worker).
int myThreadDrainZombies (DWORD dwMaxWaitMs);

#endif
