//---------------------------------------------------------------------------
// en_hangdump.cpp — hang watchdog + the dump-file helper shared with the
// crash dumper in WinMain.cpp. See en_hangdump.h for the rationale and gates.
//
// Windows-only: dbghelp/MiniDumpWriteDump do not exist on Linux/macOS, so
// everything below is inside #ifdef _WIN32 and the exported functions become
// no-ops elsewhere (the call sites stay unconditional).
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "en_hangdump.h"

#ifdef _WIN32

#include <dbghelp.h>
#include <stdio.h>
#include <stdlib.h>
#include <atomic>
#include <thread>
#pragma comment( lib, "dbghelp.lib" )

static std::atomic<unsigned long long> g_enHangBeat( 0 );   // 0 == main loop has not ticked yet
static std::atomic<bool>               g_enHangStopping( false );
static bool                            g_enHangStarted = false;

// ---------------------------------------------------------------------------
// Dump folder/path, shared by the crash dumper and the hang dumper so both
// land in the same place: %EN_DUMP_DIR%, else %LOCALAPPDATA%\EnemyNations\
// crashdumps (the old hardcoded d:\tmp path silently wrote nothing on a tester
// box with no D: drive), else d:\tmp\crashdumps. Falls back to %TEMP% when the
// chosen folder is unwritable (read-only Program Files, missing drive).
//
// Every string is built BOUNDED (_snprintf_s + _TRUNCATE returns -1 instead of
// overrunning). An overlong folder or file name is skipped, never truncated: a
// cut-off path could name some other file. This runs inside the crash handler
// and the watchdog, so an overflow here would corrupt the very stack being dumped.
// ---------------------------------------------------------------------------
static HANDLE EnDumpCreate( char* szPath, unsigned cbPath, const char* szFolder, const char* szSep,
                            const char* szPrefix, const SYSTEMTIME& st )
{
    if ( _snprintf_s( szPath, cbPath, _TRUNCATE, "%s%senations_%s_%04d%02d%02d_%02d%02d%02d_pid%lu.dmp",
                      szFolder, szSep, szPrefix, st.wYear, st.wMonth, st.wDay,
                      st.wHour, st.wMinute, st.wSecond, GetCurrentProcessId() ) < 0 )
    {
        szPath[0] = '\0';
        return INVALID_HANDLE_VALUE;
    }
    return CreateFileA( szPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
}

void* EnDumpOpenFile( const char* szPrefix, char* szPath, unsigned cbPath )
{
    if ( ( szPath == NULL ) || ( cbPath == 0 ) )
        return (void*)INVALID_HANDLE_VALUE;
    szPath[0] = '\0';

    const char* dir = getenv( "EN_DUMP_DIR" );
    const char* lad = getenv( "LOCALAPPDATA" );
    char folder[MAX_PATH];
    int  n;
    if ( dir && dir[0] )
        n = _snprintf_s( folder, sizeof( folder ), _TRUNCATE, "%s", dir );
    else if ( lad && lad[0] )
    {
        n = _snprintf_s( folder, sizeof( folder ), _TRUNCATE, "%s\\EnemyNations", lad );
        if ( n >= 0 )
        {
            CreateDirectoryA( folder, NULL );      // parent must exist first
            n = _snprintf_s( folder, sizeof( folder ), _TRUNCATE, "%s\\EnemyNations\\crashdumps", lad );
        }
    }
    else    // reachable only with LOCALAPPDATA unset (a service/stripped environment)
        n = _snprintf_s( folder, sizeof( folder ), _TRUNCATE, "%s", "d:\\tmp\\crashdumps" );

    SYSTEMTIME st; GetLocalTime( &st );

    HANDLE hf = INVALID_HANDLE_VALUE;
    if ( n >= 0 )
    {
        CreateDirectoryA( folder, NULL );
        hf = EnDumpCreate( szPath, cbPath, folder, "\\", szPrefix, st );
    }
    if ( hf == INVALID_HANDLE_VALUE )
    {
        // GetTempPathA returns the length WITHOUT the NUL when it fits, else the size it needs.
        char  tmp[MAX_PATH];
        DWORD cch = GetTempPathA( MAX_PATH, tmp );
        if ( ( cch > 0 ) && ( cch < MAX_PATH ) )
            hf = EnDumpCreate( szPath, cbPath, tmp, "", szPrefix, st );
    }
    if ( hf == INVALID_HANDLE_VALUE )
        szPath[0] = '\0';   // callers log szPath as "wrote"; nothing was opened
    return (void*)hf;
}

void EnHangHeartbeat( void )
{
    g_enHangBeat.fetch_add( 1, std::memory_order_relaxed );
}

void EnHangWatchdogStop( void )
{
    g_enHangStopping.store( true, std::memory_order_relaxed );
}

// ---------------------------------------------------------------------------
// The watchdog body. Polls the heartbeat once a second; when it has not moved
// for the threshold it writes ONE full-memory dump for the process lifetime
// and keeps running. It never suspends, resumes or kills anything -
// MiniDumpWriteDump on GetCurrentProcess() does its own thread suspension
// internally and restores it, so the hung process is left exactly as found.
// ---------------------------------------------------------------------------
static void EnHangWatchdogBody( unsigned nSecs )
{
    unsigned long long ullSeen  = 0;
    ULONGLONG          ullStamp = 0;
    bool               bFired   = false;

    for ( ;; )
    {
        ::Sleep( 1000 );
        if ( g_enHangStopping.load( std::memory_order_relaxed ) )
            return;                                  // normal shutdown: never dump on the way out

        unsigned long long ullNow = g_enHangBeat.load( std::memory_order_relaxed );
        if ( ullNow == 0 )
            continue;                                // main loop has not ticked once yet

        ULONGLONG ullTick = GetTickCount64();
        if ( ullNow != ullSeen )                     // progress
        {
            ullSeen  = ullNow;
            ullStamp = ullTick;
            continue;
        }
        if ( ullStamp == 0 ) { ullStamp = ullTick; continue; }
        if ( bFired )
            continue;
        if ( IsDebuggerPresent() )                   // a debugger halting us is not a hang
        {
            ullStamp = ullTick;
            continue;
        }

        ULONGLONG ullSilent = ( ullTick - ullStamp ) / 1000;
        if ( ullSilent < (ULONGLONG)nSecs )
            continue;

        char szPath[MAX_PATH]; szPath[0] = '\0';
        HANDLE hf = (HANDLE)EnDumpOpenFile( "hang", szPath, MAX_PATH );
        BOOL   bWrote = FALSE;
        DWORD  dwErr  = 0;
        if ( hf != INVALID_HANDLE_VALUE )
        {
            MINIDUMP_TYPE type = (MINIDUMP_TYPE)(
                MiniDumpWithFullMemory | MiniDumpWithHandleData |
                MiniDumpWithThreadInfo | MiniDumpWithFullMemoryInfo );
            bWrote = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hf,
                                        type, NULL, NULL, NULL );
            if ( !bWrote )
                dwErr = GetLastError();
            CloseHandle( hf );
            if ( !bWrote )
                DeleteFileA( szPath );               // a partial dump is not a dump
        }
        bFired = true;                               // one dump per process lifetime

        char szMsg[MAX_PATH + 96];
        if ( bWrote )
            _snprintf_s( szMsg, sizeof( szMsg ), _TRUNCATE, "[HANGDUMP] main thread silent for %lu s, wrote %s\n",
                         (unsigned long)ullSilent, szPath );
        else if ( szPath[0] )
            _snprintf_s( szMsg, sizeof( szMsg ), _TRUNCATE,
                         "[HANGDUMP] main thread silent for %lu s, MiniDumpWriteDump failed (0x%08lx), deleted %s\n",
                         (unsigned long)ullSilent, (unsigned long)dwErr, szPath );
        else
            _snprintf_s( szMsg, sizeof( szMsg ), _TRUNCATE,
                         "[HANGDUMP] main thread silent for %lu s, wrote nothing - could not open dump file\n",
                         (unsigned long)ullSilent );
        fputs( szMsg, stderr );
        fflush( stderr );
        OutputDebugStringA( szMsg );
    }
}

void EnHangWatchdogStart( void )
{
    if ( g_enHangStarted )
        return;

    // Gate: on by default in a Debug build; in a non-debug build only when
    // EN_HANGDUMP is set to something other than "0". EN_HANGDUMP=0 always off.
    const char* szOn = getenv( "EN_HANGDUMP" );
#if defined( _DEBUG )
    bool bEnabled = !( szOn && szOn[0] == '0' && szOn[1] == '\0' );
#else
    bool bEnabled = ( szOn && szOn[0] && !( szOn[0] == '0' && szOn[1] == '\0' ) );
#endif
    if ( !bEnabled )
        return;

    unsigned nSecs = 40;                             // a world load is ONE 25s frame - never go under 40
    const char* szSecs = getenv( "EN_HANGDUMP_SECS" );
    if ( szSecs && szSecs[0] )
    {
        int n = atoi( szSecs );
        nSecs = ( n < 10 ) ? 10 : (unsigned)n;       // clamped: a short threshold dumps on normal stalls
    }

    g_enHangStarted = true;
    std::thread( EnHangWatchdogBody, nSecs ).detach();
}

#else   // !_WIN32 - no dbghelp on Linux/macOS

void EnHangHeartbeat( void )     { }
void EnHangWatchdogStart( void ) { }
void EnHangWatchdogStop( void )  { }

#endif  // _WIN32
