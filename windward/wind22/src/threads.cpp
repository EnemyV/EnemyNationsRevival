//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
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
// `cs` was historically never initialized. A zero-initialized CRITICAL_SECTION
// happens to work on x86 for the uncontended fast path (interlocked LockCount),
// so the game ran for years. But the contended slow path
// (EnterCriticalSection -> wait) dereferences the lock's wait structures, which
// on a never-initialized CS are garbage. On x64 that slow path AVs — it bites on
// quit, when a worker thread still holds `cs` while the main thread's
// myThreadClose() tries to enter it. Initialize the lock before first use via a
// static-init object (cs is only ever touched at runtime, so no init-order race).
struct _CsAutoInit {
    _CsAutoInit()  { InitializeCriticalSection( &cs ); }
    ~_CsAutoInit() { DeleteCriticalSection( &cs ); }
} _csAutoInit;
// volatile: read in AI worker loops (ai.cpp AiThread while-condition) that the
// /O2 fast-debug build could otherwise hoist out of the loop.
volatile BOOL bEndThreads = FALSE;
// Bumped once per myThreadClose(). A worker that outlives the close (leaked
// straggler) sees the mismatch via myThreadShouldExit() and self-terminates
// even after a new game's myStartThread() has reset bEndThreads to FALSE —
// bEndThreads alone re-armed such a zombie (the second #65 mechanism).
volatile DWORD dwThreadGen = 0;
// Count of leaked stragglers still running. While nonzero, the AI teardown
// (fnExit == AiExit, which frees pGameData/plAIMgrList that the straggler is
// executing on) is deferred into fnDeferredExit instead of running in
// myThreadClose(); it runs from the main thread once the last zombie exits.
volatile int iZombies = 0;
static THREADEXITFUNC fnDeferredExit = NULL;
CObList lstThrds;
// The main (UI/window-owning) thread's id — myThreadTerminate must NEVER end
// it (BUGS #69). Captured on the main-thread entry points (myStartThread /
// myThreadClose / myThreadInit all run on the main thread).
static DWORD dwMainThreadId = 0;

DWORD myThreadGen() { return dwThreadGen; }
BOOL  myThreadShouldExit( DWORD dwGenAtStart ) {
    return bEndThreads || dwGenAtStart != dwThreadGen;
}

// Run a deferred AiExit() once the last zombie is gone. Main thread only
// (called from myThreadClose/myStartThread); the callback runs outside cs.
static void myRunDeferredThreadExit() {
    THREADEXITFUNC fn = NULL;
    EnterCriticalSection( &cs );
    if ( fnDeferredExit != NULL && iZombies == 0 ) {
        fn = fnDeferredExit;
        fnDeferredExit = NULL;
    }
    LeaveCriticalSection( &cs );
    if ( fn != NULL )
        fn();
}


extern "C"
{
    typedef void ( WINAPI* ENDTASKTHREAD_FUNC ) ( );
    typedef WORD( WINAPI* GETTHREADVERSION_FUNC ) ( );
    typedef void ( WINAPI* SETAIFUNC_FUNC ) ( AITHREAD pfn );
    typedef DWORD( WINAPI* STARTTHREAD_FUNC ) ( void* pData );
    typedef WORD( WINAPI* YIELDTHREAD_FUNC ) ( );

    static ENDTASKTHREAD_FUNC pfnEndTaskThread = NULL;
    static GETTHREADVERSION_FUNC pfnGetThrdUtlsVersion = NULL;
    static SETAIFUNC_FUNC pfnSetAiFunc = NULL;
    static STARTTHREAD_FUNC pfnStartThread = NULL;
    static YIELDTHREAD_FUNC pfnYieldThread = NULL;
}


class xThread {
public:
    xThread() { m_hLib = NULL; }
    ~xThread() { if ( m_hLib != NULL ) FreeLibrary( m_hLib ); m_hLib = NULL; }

    HINSTANCE m_hLib;
};

static xThread xt;


void myThreadInit( AITHREAD fnThread ) {

    if ( iWinType != W32s )
        return;

    if ( xt.m_hLib == NULL ) {
        xt.m_hLib = LoadLibrary( "dave32ut.dll" );
        if ( xt.m_hLib == NULL ) {
            ::MessageBoxA( NULL, "Thread library not found", "Enemy Nations", MB_OK | MB_ICONSTOP );
            ThrowError( ERR_NO_THREAD_LIB );
        }

        pfnEndTaskThread = (ENDTASKTHREAD_FUNC)GetProcAddress( xt.m_hLib, "_ediEndTaskThread@0" );
        pfnGetThrdUtlsVersion = (GETTHREADVERSION_FUNC)GetProcAddress( xt.m_hLib, "_ediGetThrdUtlsVersion@0" );
        pfnSetAiFunc = (SETAIFUNC_FUNC)GetProcAddress( xt.m_hLib, "_ediSetAiFunc@4" );
        pfnStartThread = (STARTTHREAD_FUNC)GetProcAddress( xt.m_hLib, "_ediStartThread@4" );
        pfnYieldThread = (YIELDTHREAD_FUNC)GetProcAddress( xt.m_hLib, "_ediYieldThread@0" );

        if ( ( pfnEndTaskThread == NULL ) || ( pfnGetThrdUtlsVersion == NULL ) ||
             ( pfnSetAiFunc == NULL ) || ( pfnStartThread == NULL ) ||
             ( pfnYieldThread == NULL ) ) {
            ::MessageBoxA( NULL, "Bad thread library", "Enemy Nations", MB_OK | MB_ICONSTOP );
            ThrowError( ERR_NO_THREAD_LIB );
        }

        if ( pfnGetThrdUtlsVersion() < 256 ) {
            ::MessageBoxA( NULL, "Bad thread library version", "Enemy Nations", MB_OK | MB_ICONSTOP );
            ThrowError( ERR_NO_THREAD_LIB );
        }
    }

    pfnSetAiFunc( fnThread );
}

volatile int iThrdsLeft = 0;

void myThreadClose( THREADEXITFUNC fnExit ) {
    static int iRecurse = 0;
    if ( dwMainThreadId == 0 ) dwMainThreadId = ::GetCurrentThreadId();

    // a previous close leaked stragglers and deferred its fnExit(); if they
    // are gone by now, run that teardown before starting this one
    myRunDeferredThreadExit();

    // we are NOT re-entrant here
    EnterCriticalSection( &cs );
    if ( iRecurse > 0 ) {
        LeaveCriticalSection( &cs );
        return;
    }
    iRecurse++;

    if ( iWinType == W32s )
        iThrdsLeft = 1;
    else {
        iThrdsLeft = lstThrds.GetCount();
        // if no threads, leave
        if ( iThrdsLeft == 0 ) {
            LeaveCriticalSection( &cs );
            fnExit();
            iRecurse--;
            return;
        }
    }

    bEndThreads = TRUE;
    // invalidate this generation of workers: even if a later myStartThread()
    // resets bEndThreads before a leaked straggler checks it, the generation
    // mismatch still makes the straggler exit (myThreadShouldExit)
    dwThreadGen++;

    // we have to get these guys moving
    POSITION pos;
    for ( pos = lstThrds.GetHeadPosition(); pos != NULL; ) {
        CWinThread* pThrd = (CWinThread*)lstThrds.GetNext( pos );
        if ( pThrd != NULL )
            if ( AfxIsValidAddress( pThrd, sizeof( CWinThread ) ) )
                if ( pThrd->m_hThread ) {
                    ASSERT_VALID( pThrd );
                    pThrd->SetThreadPriority( THREAD_PRIORITY_ABOVE_NORMAL );
                }
    }

    LeaveCriticalSection( &cs );

    // do this to get the threads to end
    DWORD dwEnd = timeGetTime() + 3000;
    while ( ( iThrdsLeft > 0 ) && ( timeGetTime() < dwEnd ) ) {
        ::Sleep( 0 );
        MSG msg;
        while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }

    // Straggler window: a worker deep in a long AI Manage() pass can miss the
    // 3s grace above. fnExit() below frees the structures the workers execute
    // on (pGameData / plAIMgrList), and theMap is torn down right after this
    // returns — so give stragglers one long, real chance to reach a yield
    // point and self-terminate (myYieldThread sees bEndThreads). Runs OUTSIDE
    // cs (a worker needs cs to exit via myThreadTerminate). Still never
    // TerminateThread — see the heap-lock note below.
    if ( iThrdsLeft > 0 ) {
        dwEnd = timeGetTime() + 10000;
        while ( ( iThrdsLeft > 0 ) && ( timeGetTime() < dwEnd ) ) {
            ::Sleep( 10 );
            MSG msg;
            while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }
    }

    // see if anyone didn't make it
    int iLeaked = 0;
    if ( iWinType == W32s ) {
        TRAP();
        EnterCriticalSection( &cs );
        pfnEndTaskThread();
        lstThrds.RemoveAll();
        LeaveCriticalSection( &cs );
    } else

    {
        EnterCriticalSection( &cs );
        for ( pos = lstThrds.GetHeadPosition(); pos != NULL; ) {
            CWinThread* pThrd = (CWinThread*)lstThrds.GetNext( pos );
            if ( pThrd != NULL )
                if ( AfxIsValidAddress( pThrd, sizeof( CWinThread ) ) )
                    if ( pThrd->m_hThread ) {
                        ASSERT_VALID( pThrd );
                        // Do NOT TerminateThread: it kills the thread with no cleanup,
                        // and if the thread held the process heap lock at that instant
                        // the lock is orphaned and the delete below (RtlFreeHeap)
                        // deadlocks forever. The 3s grace loop above already gave
                        // threads a clean chance to exit. Free only those that actually
                        // exited; leak any straggler rather than risk a heap deadlock.
                        // WaitForSingleObject(h,0) is a non-blocking poll, so it can't
                        // deadlock on cs (a stuck worker can't exit while we hold it).
                        if ( WaitForSingleObject( pThrd->m_hThread, 0 ) == WAIT_OBJECT_0 )
                            delete pThrd;   // confirmed gone -> safe to free
                        else {
                            iLeaked++;      // leak it (no terminate, no delete) -- a tiny
                                            // one-time leak beats a heap deadlock; counted
                                            // so the AI teardown is deferred below
                            iZombies++;     // its eventual myThreadTerminate() decrements
                                            // this (NOT iThrdsLeft -- already deducted here)
                        }
                        iThrdsLeft--;
                    }
        }
        lstThrds.RemoveAll();
        LeaveCriticalSection( &cs );
    }

    if ( iLeaked == 0 ) {
        // last call ever for the AI
        fnExit();
        bEndThreads = FALSE;
    } else {
        // A straggler is still executing on the very structures fnExit()
        // (AiExit) frees — running it now is the same use-after-free #65
        // closed, one level up (BUGS #65 follow-up). Defer the teardown; the
        // last zombie's myThreadTerminate() makes it runnable and the next
        // myThreadClose()/myStartThread() runs it from the main thread.
        // bEndThreads stays TRUE so the straggler exits at its next check;
        // even if a new game's myStartThread() resets the flag first, the
        // dwThreadGen bump above still kills it (myThreadShouldExit).
        EnterCriticalSection( &cs );
        fnDeferredExit = fnExit;
        LeaveCriticalSection( &cs );
        fprintf( stderr, "[threads] close leaked %d straggler(s); AI teardown deferred (gen=%lu)\n",
                 iLeaked, (unsigned long)dwThreadGen );
    }
    iRecurse--;
}

void myThreadTerminate() {

    // NEVER end the MAIN thread (BUGS #69 ROOT CAUSE, 2026-07-04). AI code
    // paths (AiSetup/AssignUnits/cmd_play dispatch) also run ON THE MAIN
    // THREAD during game creation. Between a quit-to-menu's myThreadClose()
    // (bEndThreads=TRUE, and #65's fix keeps it armed) and the next game's
    // myStartThread() (re-arms FALSE), any main-thread myYieldThread() hit a
    // TRUE bEndThreads and fell through to AfxEndThread() below — SILENTLY
    // EXITING THE MAIN THREAD. Windows then destroys every window it owns
    // (main + all owned panels + the thread's IME windows) with no WM_DESTROY
    // through subclasses and SDL's flag cache left stale-SHOWN, while the
    // process lives on via worker threads: the operator's "all my windows
    // disappear when I create a second game" (captured live: 12 windows ->
    // 0 in one 0.9s step incl. 'Default IME'; dbgstack then showed WinMain's
    // thread GONE). The main thread is never a worker: ignore the terminate.
    if ( ::GetCurrentThreadId() == dwMainThreadId ) {
        // Log ONCE per session: this fires per yield call in tight main-thread
        // AI loops (CAIMap ctor at 15p-large = thousands of hits), and under a
        // debugger every OutputDebugString is a process suspend — the per-hit
        // logging alone stalled a 15-player create at 0% (operator 2026-07-04).
        static LONG lLogged = 0;
        if ( InterlockedExchange( &lLogged, 1 ) == 0 ) {
            fprintf( stderr, "[threads] myThreadTerminate on the MAIN thread ignored (bEndThreads armed between games; logged once)\n" );
            OutputDebugStringA( "[threads] MAIN-thread terminate IGNORED (#69 guard; logged once)\n" );
        }
        return;
    }

    // do this with a call to the AI
    if ( iWinType != W32s ) {
        EnterCriticalSection( &cs );
        // Is this a live worker (still in lstThrds -- entries are only ever
        // removed wholesale by myThreadClose) or a zombie leaked by an earlier
        // close (list rebuilt without it)? NOTE: the old check compared against
        // AfxGetThread(), which the compat shim hardwires to NULL, so it never
        // matched -- match by thread id instead. A zombie must NOT decrement
        // iThrdsLeft: it was already deducted at leak time, and a second
        // decrement corrupts the NEXT game's close accounting (the wait loop
        // exits early and leaks a healthy worker).
        BOOL bListed = FALSE;
        DWORD dwSelf = ::GetCurrentThreadId();
        POSITION pos;
        for ( pos = lstThrds.GetHeadPosition(); pos != NULL; ) {
            CWinThread* pThrd = (CWinThread*)lstThrds.GetNext( pos );
            if ( pThrd != NULL && pThrd->m_nThreadID == dwSelf ) {
                bListed = TRUE;
                break;
            }
        }
        if ( bListed )
            iThrdsLeft--;       // normal exit; myThreadClose still owns the entry
        else if ( iZombies > 0 ) {
            iZombies--;         // leaked straggler finally exiting; the deferred
                                // AiExit becomes runnable when this hits 0
            fprintf( stderr, "[threads] zombie worker exited (%d left)\n", iZombies );
        }
        LeaveCriticalSection( &cs );
        AfxEndThread( 0 );
    } else {
        TRAP();
        pfnEndTaskThread();
        iThrdsLeft--;
    }
}

// AIRTIGHT #65 tail — see threads.h. Blocks (pumping messages) until every
// leaked straggler has exited and the deferred AiExit has run. linux2 proved
// the 2s myStartThread grace is NOT enough on big games (a 15-player Manage()
// pass runs minutes): start->quit->start then hit AiInit's unconditional
// delete of pGameData/plAIMgrList under a live zombie = UAF — SIGABRT on
// gcc (forced_unwind swallowed, 3/3 repro), silent state corruption on
// Windows (the second-entry window death, BUGS #69).
int myThreadDrainZombies( DWORD dwMaxWaitMs ) {
    const DWORD dwEnd = timeGetTime() + dwMaxWaitMs;
    BOOL bWarned = FALSE;
    while ( iZombies > 0 && timeGetTime() < dwEnd ) {
        if ( !bWarned ) {
            fprintf( stderr, "[threads] draining %d zombie AI worker(s) before new game (max %lums)\n",
                     iZombies, (unsigned long)dwMaxWaitMs );
            bWarned = TRUE;
        }
        ::Sleep( 25 );
        MSG msg;
        while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }
    myRunDeferredThreadExit();
    if ( iZombies > 0 )
        fprintf( stderr, "[threads] WARNING: %d zombie(s) still live after %lums drain\n",
                 iZombies, (unsigned long)dwMaxWaitMs );
    return iZombies;
}

WORD myGetThrdUtlsVersion() {

    if ( iWinType != W32s )
        return ( 0 );

    return ( pfnGetThrdUtlsVersion() );
}

void myStartThread( void* pData, AFX_THREADPROC fnThread ) {

    if ( dwMainThreadId == 0 ) dwMainThreadId = ::GetCurrentThreadId();

    // If the last close leaked a straggler, give it a beat to hit its next
    // check (the generation mismatch kills it in ~100ms unless it is wedged
    // inside one long Manage() pass), then run the deferred AiExit() so the
    // old game's AI structures are freed BEFORE the new game re-creates them
    // (ai.cpp deletes plAIMgrList unconditionally on init — doing that under
    // a live zombie is #65 again).
    if ( iZombies > 0 ) {
        DWORD dwEnd = timeGetTime() + 2000;
        while ( iZombies > 0 && timeGetTime() < dwEnd )
            ::Sleep( 10 );
        if ( iZombies > 0 )
            fprintf( stderr, "[threads] WARNING: starting new AI worker with %d zombie(s) still live; deferred AI teardown stays parked\n", iZombies );
    }
    myRunDeferredThreadExit();

    bEndThreads = FALSE;

    if ( iWinType != W32s ) {
        CWinThread* pThrd = AfxBeginThread( fnThread, pData, THREAD_PRIORITY_BELOW_NORMAL );
        lstThrds.AddTail( pThrd );
        return;
    }

    pfnStartThread( pData );
}

void myYieldThread() {

    // The MAIN thread must NEVER take the terminate path here (BUGS #69 root):
    // AI setup (AiSetupHeavy/CreateHeavy/CAIMap ctor) runs ON the main thread
    // during game-2 creation while bEndThreads is still armed from game-1's
    // quit (myThreadClose set it TRUE; game-2's myStartThread hasn't reset it
    // yet). The guard in myThreadTerminate stops the main thread from EXITING,
    // but execution then falls through to the TRAP() below = an int3 on EVERY
    // yield — thousands in the CAIMap loop = "stuck at 94%" under a debugger,
    // and an unhandled EXCEPTION_BREAKPOINT (crash) undebugged. Skip the whole
    // end-of-thread block on the main thread; it just yields.
    if ( dwMainThreadId != 0 && ::GetCurrentThreadId() == dwMainThreadId ) {
#ifdef AI_THREADS_ENABLED
        if ( SwitchToThread( ) == 0 )
            Sleep( 0 );
#endif
        return;
    }

    // is it time to end it?
    if ( bEndThreads ) {
        myThreadTerminate();
        TRAP();
    }

    if ( iWinType == W32s )
        if ( pfnYieldThread() == TM_QUIT ) {
            TRAP();
            myThreadTerminate();
            TRAP();
        }

#ifdef AI_THREADS_ENABLED
    if ( SwitchToThread( ) == 0 )
    {
        Sleep( 0 );
    }
#endif
}

void myPauseThread( BOOL bPause ) {

    if ( iWinType == W32s )
        return;

    // we have to get these guys moving
    POSITION pos;
    for ( pos = lstThrds.GetHeadPosition(); pos != NULL; ) {
        CWinThread* pThrd = (CWinThread*)lstThrds.GetNext( pos );
        if ( pThrd != NULL )
            if ( AfxIsValidAddress( pThrd, sizeof( CWinThread ) ) )
                if ( pThrd->m_hThread ) {
                    ASSERT_VALID( pThrd );
                    if ( bPause )
                        pThrd->SuspendThread();
                    else
                        pThrd->ResumeThread();
                }
    }
}
