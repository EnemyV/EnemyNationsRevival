//---------------------------------------------------------------------------
// WinMain.cpp — Phase 4c entry point replacing MFC's AfxWinMain.
//
// MFC's CMAKE_MFC_FLAG=2 used to pull in an auto-generated WinMain via
// AfxWinMain in mfcsXX.lib. AfxWinMain calls AfxGetApp() which assumes a
// CWinApp-derived static instance; CConquerApp now inherits CWinAppStub
// (no MFC), so AfxGetApp would return garbage and crash.
//
// Defining our own WinMain here makes the linker prefer this entry over
// the MFC one. We then directly drive the CConquerApp lifecycle:
//   theApp.m_hInstance = hInstance
//   theApp.InitInstance()
//   theApp.Run()
//   theApp.ExitInstance()
//---------------------------------------------------------------------------

#include "stdafx.h"

#include "lastplnt.h"

#include <dbghelp.h>
#include <time.h>
#if defined( _DEBUG )
#include <crtdbg.h>
#endif
#pragma comment( lib, "dbghelp.lib" )

extern CConquerApp theApp;

// ---------------------------------------------------------------------------
// Full-memory crash dumps. Writes a complete .dmp (MiniDumpWithFullMemory, so
// stacks are never truncated) on any unhandled exception. No admin / WER
// LocalDumps needed. Captures the AI worker threads' stacks too — important
// since the slowdown/crash bug lives in the AI sim. Dumps go to
// %EN_DUMP_DIR% (default d:\tmp\crashdumps).
// ---------------------------------------------------------------------------
#ifdef _WIN32
static LONG WINAPI EnWriteFullDump( EXCEPTION_POINTERS* ep )
{
    const char* dir = getenv( "EN_DUMP_DIR" );
    char path[MAX_PATH];
    char folder[MAX_PATH];
    lstrcpynA( folder, dir && dir[0] ? dir : "d:\\tmp\\crashdumps", MAX_PATH );
    CreateDirectoryA( folder, NULL );

    SYSTEMTIME st; GetLocalTime( &st );
    wsprintfA( path, "%s\\enations_full_%04d%02d%02d_%02d%02d%02d_pid%lu.dmp",
               folder, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
               GetCurrentProcessId() );

    HANDLE hf = CreateFileA( path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, NULL );
    if ( hf != INVALID_HANDLE_VALUE )
    {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId          = GetCurrentThreadId();
        mei.ExceptionPointers = ep;
        mei.ClientPointers    = FALSE;

        MINIDUMP_TYPE type = (MINIDUMP_TYPE)(
            MiniDumpWithFullMemory | MiniDumpWithHandleData |
            MiniDumpWithThreadInfo | MiniDumpWithFullMemoryInfo );

        MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hf,
                           type, ep ? &mei : NULL, NULL, NULL );
        CloseHandle( hf );

        char msg[MAX_PATH + 32];
        wsprintfA( msg, "ENDUMP: wrote full dump %s\n", path );
        OutputDebugStringA( msg );
    }
    return EXCEPTION_EXECUTE_HANDLER;   // let the process terminate after dumping
}
#endif // _WIN32 (Windows minidump crash handler; no dbghelp on Linux)

extern "C" int APIENTRY WinMain( HINSTANCE hInstance,
                                 HINSTANCE hPrevInstance,
                                 LPSTR     lpCmdLine,
                                 int       nCmdShow )
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Install the full-memory crash dumper as early as possible so any later
    // fault (incl. assert/STATUS_BREAKPOINT and AI-thread faults) is captured
    // with complete stacks.
#ifdef _WIN32
    SetUnhandledExceptionFilter( EnWriteFullDump );
#endif

#if defined( _DEBUG ) && defined( _WIN32 )
    // CRT debug asserts (checked-iterator "list iterators incompatible", _ASSERTE,
    // heap checks) default to a MODAL Abort/Retry/Ignore dialog — which silently
    // freezes unattended test sessions until someone clicks it (and blocks ALL
    // game input, looking like a hang). Route them to OutputDebugString instead
    // (dbgcatch.ps1 captures it) and CONTINUE — the same "log + keep going" policy
    // en_assert.h applies to the game's own ASSERTs. Real faults still crash and
    // produce a full dump via the filter above.
    _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );
    _CrtSetReportMode( _CRT_ERROR,  _CRTDBG_MODE_DEBUG );
#endif

#if defined( _DEBUG ) && defined( _WIN32 )
    // Heap-corruption hunt. Two combat crashes landed in unrelated places
    // (recalloc_dbg via CVehicle::DeletePath, and msctf via the msg pump) — the
    // classic signature of a wild write that detonates later somewhere random.
    // With EN_HEAPCHECK=1 the CRT validates the ENTIRE heap on every alloc/free
    // (_CRTDBG_CHECK_ALWAYS_DF), so corruption faults right next to the operation
    // that caused it instead of much later — turning "random crash" into a stack
    // pointing at the culprit (captured by EnWriteFullDump above). Very slow, so
    // it's opt-in. EN_HEAPFREQ=N uses _CRTDBG_CHECK_EVERY_N (cheaper) instead.
    {
        const char* hc = getenv( "EN_HEAPCHECK" );
        const char* hf = getenv( "EN_HEAPFREQ" );
        if ( ( hc && hc[0] && hc[0] != '0' ) || ( hf && hf[0] ) )
        {
            int flags = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
            flags |= _CRTDBG_ALLOC_MEM_DF;
            if ( hf && hf[0] )
            {
                // Check every N allocations. The CRT encodes the frequency in the
                // HIGH 16 bits of the flag word; the predefined _CRTDBG_CHECK_EVERY_N_DF
                // constants are literally (N<<16). So we set the count directly and
                // must NOT also OR one of those constants (that would change N).
                int n = atoi( hf ); if ( n < 1 ) n = 1024;
                flags = ( flags & 0x0000FFFF ) | ( n << 16 );
            }
            else
            {
                flags |= _CRTDBG_CHECK_ALWAYS_DF;
            }
            _CrtSetDbgFlag( flags );
            // Send CRT asserts/heap reports to the debugger (OutputDebugString)
            // so dbgcatch.ps1 can capture them, and don't pop a modal dialog.
            _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
            _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );
            _CrtSetReportMode( _CRT_WARN,  _CRTDBG_MODE_DEBUG );
            OutputDebugStringA( "ENHEAP: full heap validation ENABLED\n" );
        }
    }
#endif

    // Bootstrap CWinAppStub members the app expects.
    theApp.m_hInstance = hInstance;

    // App lifecycle.
    int exitCode = 0;
    if ( theApp.InitInstance() )
    {
        exitCode = theApp.Run();
    }
    theApp.ExitInstance();

    return exitCode;
}
