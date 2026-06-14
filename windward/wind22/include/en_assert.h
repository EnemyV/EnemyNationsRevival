//---------------------------------------------------------------------------
//
//  en_assert.h — non-fatal, logged ASSERT for the SDL2/x64 port.
//
//  The original MFC ASSERT popped an Abort/Retry/Ignore dialog: the game
//  could continue past a tripped assert (the user confirms the original did
//  trip some of these in normal play). The straight CRT assert() this port
//  was using instead HARD-KILLS the process (STATUS_BREAKPOINT 0x80000003)
//  on the first firing — stricter than the original and a poor fit for the
//  many vestigial 1996 sanity checks that fire on harmless edge cases.
//
//  EnAssertFire() mirrors the original's "Ignore": it records the firing
//  (deduped per call-site, plus a global counter g_enAssertFires) and lets
//  execution continue. Nothing is silently swallowed — every distinct site
//  is logged via OutputDebugString so real invariant breaks are still
//  visible for triage; they just no longer crash the game.
//
//---------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <cstdio>

// Total number of assert firings this run (exposed as a perf gauge / for triage).
// selectany folds the duplicate header definitions to one; gcc uses weak.
#ifdef _WIN32
extern "C" __declspec( selectany ) volatile long g_enAssertFires = 0;
#else
extern "C" __attribute__(( weak )) volatile long g_enAssertFires = 0;
#endif

inline void EnAssertFire( const char* expr, const char* file, int line )
{
    InterlockedIncrement( &g_enAssertFires );

    // Dedup per call-site so an assert inside a per-frame loop doesn't flood
    // the log. Function-local statics in an inline function are ODR-merged to
    // a single shared instance across all translation units. The scan/append
    // race is benign for diagnostics (worst case: one duplicate log line).
    enum { MAXSITES = 1024 };
    static const char* seenFile[MAXSITES] = {};
    static int         seenLine[MAXSITES] = {};
    static long        seenCount = 0;

    long n = seenCount;
    if ( n > MAXSITES )
        n = MAXSITES;
    for ( long i = 0; i < n; ++i )
        if ( seenLine[i] == line && seenFile[i] == file )
            return;  // already reported this site

    if ( seenCount < MAXSITES )
    {
        seenFile[seenCount] = file;
        seenLine[seenCount] = line;
        seenCount           = seenCount + 1;
    }

    char buf[600];
    _snprintf_s( buf, sizeof( buf ), _TRUNCATE, "[ASSERT-IGNORED] %s  (%s:%d)\n", expr ? expr : "?", file, line );
    OutputDebugStringA( buf );
}

// Evaluates expr exactly once; non-fatal. Usable in statement context.
#define EN_ASSERT_NONFATAL( expr ) ( ( expr ) ? (void)0 : EnAssertFire( #expr, __FILE__, __LINE__ ) )

// Replacement for a TRAP() we've DELIBERATELY removed after confirming the
// guarded condition is benign and already handled (e.g. a normal RTS outcome).
// Never halts; logs ONCE per call-site (deduped) via OutputDebugString so the
// condition stays visible in the debugger / dbgcatch — we don't lose the signal
// the TRAP gave us, we just stop crashing on it. Use ONLY where a TRAP was
// removed on purpose; real invariant checks should stay TRAP/ASSERT.
inline void EnTrapRemovedLog( const char* msg, const char* file, int line )
{
    InterlockedIncrement( &g_enAssertFires );

    enum { MAXSITES = 1024 };
    static const char* seenFile[MAXSITES] = {};
    static int         seenLine[MAXSITES] = {};
    static long        seenCount = 0;

    long n = seenCount;
    if ( n > MAXSITES )
        n = MAXSITES;
    for ( long i = 0; i < n; ++i )
        if ( seenLine[i] == line && seenFile[i] == file )
            return;  // already reported this site

    if ( seenCount < MAXSITES )
    {
        seenFile[seenCount] = file;
        seenLine[seenCount] = line;
        seenCount           = seenCount + 1;
    }

    char buf[600];
    _snprintf_s( buf, sizeof( buf ), _TRUNCATE, "[TRAP-REMOVED] %s  (%s:%d)\n", msg ? msg : "", file, line );
    OutputDebugStringA( buf );
}

#define EN_TRAP_REMOVED( msg ) EnTrapRemovedLog( ( msg ), __FILE__, __LINE__ )
