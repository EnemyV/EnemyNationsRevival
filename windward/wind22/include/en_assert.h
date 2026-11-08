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
#include <cstddef>
#include <type_traits>
#ifndef _WIN32
#include <dlfcn.h>
#endif

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

// Is `vptr` a real C++ vtable pointer (i.e. does it point into a loaded image's
// read-only data, where vtables live — never the heap/stack)? Used to keep the
// AssertValid() integrity sweep (TestEverything) non-fatal when it walks a
// dangling/freed object: the port's simplified ASSERT_VALID dispatched the
// virtual AssertValid() straight through the object's vtable, so a freed object
// (zeroed or garbage vtable) faulted — observed as SIGSEGV at 0x18 (the
// AssertValid vtable slot) and, on arm64e, as a pointer-authentication failure,
// via CUnit::AssertValid <- CMaterialBuilding/CWarehouseBuilding from
// DestroyWorld/TestEverything on a building with a garbage m_pUnitData.
//
// dladdr() answers this precisely (mincore() only proves the page is *mapped*,
// which a freed heap object still is), but dladdr walks the loaded images and is
// FAR too slow to call on every ASSERT_VALID — worldgen's CTerrain::AssertValid
// validates every terrain tile, and a dladdr per call pinned the main thread at
// 100% CPU. So cache validated vtables: there is one per class, so the working
// set is tiny and bounded, and the hot loops reuse the same handful. Garbage
// vtables are all different and never in an image, so they never cache — they
// always fall through to the dladdr reject. Windows keeps the historical
// non-null behavior (== AfxIsValidAddress) so the MSVC build is unchanged.
inline bool EnVtableInImage( const void* vptr )
{
    if ( vptr == nullptr )
        return false;
#ifdef _WIN32
    return true;
#else
    static const void* s_lastGood = nullptr;   // hottest path: same vtable repeated
    if ( vptr == s_lastGood )
        return true;

    enum { NCACHE = 1024 };
    static const void* s_good[NCACHE] = {};
    static int         s_count = 0;
    for ( int i = 0; i < s_count; ++i )
        if ( s_good[i] == vptr )
        {
            s_lastGood = vptr;
            return true;
        }

    Dl_info dli;
    if ( ::dladdr( vptr, &dli ) == 0 )
        return false;                          // not in any loaded image => garbage

    if ( s_count < NCACHE )
        s_good[s_count++] = vptr;
    s_lastGood = vptr;
    return true;
#endif
}

// Non-fatal ASSERT_VALID core: for polymorphic types, validate the object's
// vtable points into a loaded image before the virtual AssertValid() dispatch.
// A null / dangling / freed object is LOGGED via EnAssertFire instead of crashing
// — mirrors this header's "Ignore, don't kill" philosophy and original MFC's
// AfxAssertValidObject. The trap signal is preserved (logged with the call site),
// so a real lifetime bug still shows up in the log; it just no longer hard-kills.
template <class T>
inline void EnAssertValidObj( T* pOb, const char* file, int line )
{
    if ( pOb == nullptr )
    {
        EnAssertFire( "ASSERT_VALID: null", file, line );
        return;
    }
    if ( std::is_polymorphic<typename std::remove_const<T>::type>::value )
    {
        // vtable pointer lives at offset 0 (Itanium / MSVC single-inheritance).
        const void* vptr = *reinterpret_cast<const void* const*>( pOb );
        if ( !EnVtableInImage( vptr ) )
        {
            EnAssertFire( "ASSERT_VALID: freed/garbage object (bad vtable)", file, line );
            return;
        }
    }
    pOb->AssertValid();
}

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
