#if !defined ( __THREADAPI)
#define __THREADAPI

#if !defined ( __WINDOWS_H ) && !defined ( _INC_WINDOWS_H)
#include <windows.h>
#endif /* __WINDOWS_H */

/********************************************************************
 *                   Threads Utilities For Windows                  *
 *                              API Unit                            *
 ********************************************************************
 *       Copyright 1992 Robert Salesas, All Rights Reserved         *
 ********************************************************************
 *      Version: 1.00             Author:  Robert Salesas           *
 *      Date:    30-Jan-1992      Changes: Original                 *
 *                                                                  *
 *      Version: 1.00             Author:  Sylvain Tremblay         *
 *      Date:    24-Feb-1992      Changes: Pascal to C convertion   *
 *                                                                  *
 ********************************************************************/

extern "C" {

typedef BYTE FAR * PThreadRec;
typedef FARPROC PThreadFunc;

#define TM_USER     0x0100         /* Starting user message to pass to a thread */
#define TM_RSV00    0x0000         /* RESERVED: Used to allow CATCH one pass only */
#define TM_QUIT     0x0001         /* Thread has ended or must end */
#define TM_CONTINUE 0x0002         /* Thread can continue or is continuing */
#define TM_PAUSED   0x0003         /* Thread is currently paused */
#define TM_ERROR    0x0004         /* ERROR: Wrong thread? */

#define TS_DEFTIMESLICE  50        /* Default time slice */
#define TS_DEFPRIORITY  100        /* Default thread priority */


WORD FAR PASCAL GetThrdUtlsVersion();
  /* Returns major revision in high byte, minor revision in low byte. */
  /* Index 100 */

WORD FAR PASCAL GetNumThreads(VOID);
  /* Index 110; */

VOID FAR PASCAL SetThrdUtlsTimeSlice(WORD ATimeSlice);
  /* Index 120; */

PThreadRec FAR PASCAL GetCurrentThread(VOID)
;
  /* Index 130; */

PThreadRec FAR PASCAL CreateThread(PThreadFunc ThreadFunc, WORD StackSize,
               HWND Wnd, WORD wParam, LONG lParam);
  /* Index 200; */

VOID FAR PASCAL DisposeThread(PThreadRec *Thread);
  /* Index 210; */

WORD FAR PASCAL ExecThread(PThreadRec Thread);
  /* Index 220; */

WORD FAR PASCAL YieldThread(VOID);
  /* Index 230; */

VOID FAR PASCAL ExitThread(VOID);
  /* Index 240; */

VOID FAR PASCAL TerminateThread(PThreadRec Thread);
  /* Index 250; */

VOID FAR PASCAL SetThreadPriority(PThreadRec Thread, WORD Priority);
  /* Index 260; */

VOID FAR PASCAL SetThreadPause(PThreadRec Thread, BOOL Paused);
  /* Index 270; */

BOOL FAR PASCAL IsThreadPaused(PThreadRec Thread);
  /* Index 280; */

BOOL FAR PASCAL IsThreadFinished(PThreadRec Thread);
  /* Index 290; */



BOOL FAR PASCAL AddThread(PThreadRec Thread);
  /* Index 300; */

VOID FAR PASCAL RemoveThread(PThreadRec Thread);
  /* Index 310; */

PThreadRec FAR PASCAL StartThread(PThreadFunc ThreadFunc, WORD StackSize,
                 HWND Wnd, WORD wParam, LONG lParam);
  /* Index 320; */

VOID FAR PASCAL EndThread(PThreadRec *Thread);
  /* Index 330; */

VOID FAR PASCAL ExecTaskThreads(HANDLE Task);
  /* Index 340; */

VOID FAR PASCAL EndTaskThreads(HANDLE Task);
  /* Index 350; */
}

#endif
