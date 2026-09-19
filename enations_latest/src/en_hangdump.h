//---------------------------------------------------------------------------
// en_hangdump.h — hang watchdog (diagnostic instrument only).
//
// The crash dumper in WinMain.cpp (SetUnhandledExceptionFilter ->
// EnWriteFullDump) only fires on an unhandled exception. A hard FREEZE (main
// loop stops, perf.log stops ticking, no exception) produces nothing at all,
// so the operator's frozen session had to be killed by hand with zero
// evidence. This adds the missing half: a watchdog thread that notices the
// main thread has stopped making progress and writes the SAME kind of
// full-memory minidump, to the same folder, so the freeze is diagnosable
// offline with cdb.
//
// It changes NO game state: it never kills, resumes or signals anything. The
// process is left exactly as it was found.
//
//   EnHangHeartbeat()      - called from every main-thread event-pump site.
//   EnHangWatchdogStart()  - starts the watchdog thread (from WinMain).
//   EnHangWatchdogStop()   - normal shutdown; stops the watchdog firing on exit.
//
// Gates: on by default in _DEBUG; in a non-debug build only when EN_HANGDUMP
// is set to something other than "0". EN_HANGDUMP=0 disables it everywhere.
// EN_HANGDUMP_SECS=<n> overrides the silence threshold (default 40s, min 10s)
// — a world load has been measured as a single 25s frame, so nothing under
// 40s may fire. A dump is never written while IsDebuggerPresent() (a debugger
// suspending the process is not a hang), never before the first heartbeat,
// and at most ONCE per process lifetime.
//
// Non-Windows builds have no dbghelp: the heartbeat is a no-op and the
// watchdog never starts, so the calls stay unconditional at the call sites.
//---------------------------------------------------------------------------

#ifndef EN_HANGDUMP_H
#define EN_HANGDUMP_H

void EnHangHeartbeat( void );
void EnHangWatchdogStart( void );
void EnHangWatchdogStop( void );

#ifdef _WIN32
// Shared by the crash dumper (WinMain.cpp) and the hang dumper so both land in
// the same folder: %EN_DUMP_DIR%, else %LOCALAPPDATA%\EnemyNations\crashdumps,
// else d:\tmp\crashdumps. Builds "<folder>\enations_<prefix>_<date>_<time>_pid<n>.dmp"
// into szPath (MAX_PATH bytes) and opens it; on failure retries in %TEMP%.
// Returns the open write handle, or INVALID_HANDLE_VALUE.
void* EnDumpOpenFile( const char* szPrefix, char* szPath, unsigned cbPath );
#endif

#endif // EN_HANGDUMP_H
