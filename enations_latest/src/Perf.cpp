//---------------------------------------------------------------------------
//
//  Perf.cpp - Lightweight runtime profiling / observability harness.
//
//  See Perf.h for usage. Implementation goals:
//    * near-zero cost when disabled (single bool branch)
//    * cheap when enabled (no per-frame allocation, no per-frame file I/O;
//      accumulate in fixed arrays, touch perf.log once per interval)
//    * no engine dependencies — only Win32 + CRT.
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "Perf.h"

#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// CRT debug-heap leak tracking. In a _DEBUG build every allocation is tracked by
// the CRT debug heap, so we can read live heap bytes/blocks each interval. Gives
// the leak RATE but not the call site.
#if defined( _DEBUG )
#include <crtdbg.h>
#endif

// In-process sampling allocation profiler (EN_PERF_ALLOC=N): overrides global
// operator new/delete, captures a call stack on 1/N allocations, tracks LIVE
// bytes per stack (so a real leak is distinguished from transient churn), and on
// trigger symbolizes the top leaking stacks against the PDB -> function+file:line
// written to leakstacks.txt. No external tools.
#include <new>
#include <dbghelp.h>

#pragma comment( lib, "psapi.lib" )
#pragma comment( lib, "dbghelp.lib" )

extern "C" __declspec(dllimport) USHORT WINAPI RtlCaptureStackBackTrace(
    ULONG FramesToSkip, ULONG FramesToCapture, PVOID* BackTrace, PULONG BackTraceHash );

namespace
{
    // ---- gate / config -----------------------------------------------------
    volatile bool g_enabled    = false;
    bool          g_inited     = false;
    DWORD         g_intervalMs = 1000;     // sample cadence (EN_PERF_INTERVAL_MS)

    // ---- timing ------------------------------------------------------------
    double   g_perfFreq      = 1.0;        // ticks per second
    LONGLONG g_intervalStart = 0;          // QPC at start of current interval
    DWORD    g_startTickMs   = 0;          // GetTickCount at Init (match time)

    unsigned g_frames        = 0;
    double   g_frameSumMs    = 0.0;
    double   g_frameMaxMs    = 0.0;
    LONGLONG g_lastFrameQpc  = 0;

    LONGLONG g_secTicks[Perf::SEC_COUNT] = { 0 };

    // match-second at which to dump the leak attribution (EN_PERF_LEAKDUMP).
    long         g_leakDumpAtSec = 0;

    // ---- named counters ----------------------------------------------------
    const int  MAX_COUNTERS = 160;
    struct Counter
    {
        const char* name;
        int64_t     value;
        bool        isGauge;   // gauges aren't reset each interval
    };
    Counter g_counters[MAX_COUNTERS];
    int     g_numCounters = 0;

    CRITICAL_SECTION g_counterCs;
    bool             g_csInited = false;

    FILE* g_log = NULL;

    Counter* FindOrAdd( const char* name, bool gauge )
    {
        for ( int i = 0; i < g_numCounters; ++i )
            if ( g_counters[i].name == name ||
                 ( g_counters[i].name && strcmp( g_counters[i].name, name ) == 0 ) )
                return &g_counters[i];
        if ( g_numCounters >= MAX_COUNTERS )
            return NULL;
        Counter* c = &g_counters[g_numCounters++];
        c->name    = name;
        c->value   = 0;
        c->isGauge = gauge;
        return c;
    }

    // ===== sampling allocation profiler ====================================
    // Enable with EN_PERF_ALLOC=N (sample 1 of every N allocations; 0/unset =
    // off). Dump top leaking stacks at EN_PERF_LEAKDUMP=<matchSec>.
    //
    // Design for low overhead + correctness:
    //  * Only 1/N allocations are tracked (sampling) -> small constant cost.
    //  * Each tracked allocation records a short call-stack signature (hash) and
    //    its pointer+size in an open-addressed table. On free we look the pointer
    //    up and subtract -> we track LIVE bytes per call stack, so a hot path that
    //    allocates and frees (churn) nets ~0, while a true leak grows without bound.
    //  * No STL, no allocations inside the hooks (would recurse): fixed arrays.
    //  * A thread-local reentrancy guard stops our own bookkeeping from sampling
    //    itself.
    const int   STACK_DEPTH    = 12;     // frames captured per sample
    const int   MAX_SITES      = 8192;   // distinct call stacks tracked
    const int   PTR_TABLE_BITS = 19;     // 512K-slot open-addressed pointer map
    const size_t PTR_TABLE_SZ  = (size_t)1 << PTR_TABLE_BITS;

    struct Site {
        ULONG  hash;
        int    used;
        long long liveBytes;    // currently-live sampled bytes from this stack
        long long totalBytes;   // cumulative sampled bytes ever (alloc rate)
        long long liveCount;
        void*  frames[STACK_DEPTH];
        int    nFrames;
    };

    // Live allocation record keyed by the CRT request number (stable from the
    // alloc hook's _HOOK_ALLOC through to _HOOK_FREE), so we never have to
    // replace the allocator or touch real pointers.
    struct PtrRec { long reqNum; int siteIdx; size_t size; };

    Site*    g_sites    = NULL;          // [MAX_SITES]
    PtrRec*  g_ptrTable = NULL;          // [PTR_TABLE_SZ]
    int      g_numSites = 0;
    int      g_allocSampleN = 0;         // EN_PERF_ALLOC (0 = disabled)
    volatile LONG g_allocSeq = 0;        // for sampling cadence
    CRITICAL_SECTION g_allocCs;
    bool     g_allocInited = false;
    bool     g_allocDumped = false;
    bool     g_dumpFileLatched = false;     // edge-detect the DUMP_NOW file
    __declspec(thread) int t_inAlloc = 0;   // reentrancy guard

#if defined( _DEBUG )
    _CRT_ALLOC_HOOK g_prevAllocHook = NULL;
    int AllocHook( int nAllocType, void* pvData, size_t nSize, int nBlockUse,
                   long lRequest, const unsigned char* szFile, int nLine );
#endif

    void AllocProfInit()
    {
#if !defined( _DEBUG )
        if ( getenv( "EN_PERF_ALLOC" ) )
            OutputDebugStringA( "ENPERF: EN_PERF_ALLOC needs a _DEBUG build (CRT alloc hook); ignoring.\n" );
        return;
#else
        const char* e = getenv( "EN_PERF_ALLOC" );
        if ( !e || !e[0] ) return;
        int n = atoi( e );
        if ( n <= 0 ) return;
        g_allocSampleN = n;

        // Tables via raw VirtualAlloc so our bookkeeping never re-enters the CRT
        // heap (and thus the hook).
        g_sites    = (Site*)  VirtualAlloc( NULL, sizeof(Site)   * MAX_SITES,
                                            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
        g_ptrTable = (PtrRec*)VirtualAlloc( NULL, sizeof(PtrRec) * PTR_TABLE_SZ,
                                            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
        if ( !g_sites || !g_ptrTable ) { g_allocSampleN = 0; return; }
        InitializeCriticalSection( &g_allocCs );
        g_allocInited = true;

        // OBSERVE the CRT allocator (we do not replace it) — zero corruption risk.
        g_prevAllocHook = _CrtSetAllocHook( AllocHook );
        OutputDebugStringA( "ENPERF: alloc profiler ON (CRT alloc hook, observe-only)\n" );
#endif
    }

    inline size_t PtrSlot( long reqNum )
    {
        // hash the CRT request number (monotonic) — Fibonacci hash
        unsigned long long x = (unsigned long long)(unsigned long)reqNum;
        x *= 11400714819323198485ULL;
        return (size_t)( x >> ( 64 - PTR_TABLE_BITS ) );
    }

    int FindOrAddSite( void** frames, int nFrames, ULONG hash )
    {
        // linear-probe a small zone keyed by hash; sites are few (<=8192)
        for ( int i = 0; i < g_numSites; ++i )
            if ( g_sites[i].hash == hash && g_sites[i].nFrames == nFrames )
                return i;
        if ( g_numSites >= MAX_SITES ) return -1;
        int idx = g_numSites++;
        Site& s = g_sites[idx];
        s.hash = hash; s.used = 1;
        s.liveBytes = s.totalBytes = s.liveCount = 0;
        s.nFrames = nFrames;
        for ( int i = 0; i < nFrames; ++i ) s.frames[i] = frames[i];
        return idx;
    }

    void AllocProfOnAlloc( long reqNum, size_t size )
    {
        if ( !g_allocInited ) return;
        if ( t_inAlloc ) return;
        // sample 1 in N
        LONG seq = InterlockedIncrement( &g_allocSeq );
        if ( ( seq % g_allocSampleN ) != 0 ) return;

        t_inAlloc = 1;
        void* frames[STACK_DEPTH];
        ULONG hash = 0;
        USHORT n = RtlCaptureStackBackTrace( 3, STACK_DEPTH, frames, &hash );

        EnterCriticalSection( &g_allocCs );
        int idx = FindOrAddSite( frames, n, hash );
        if ( idx >= 0 )
        {
            g_sites[idx].liveBytes  += (long long)size;
            g_sites[idx].totalBytes += (long long)size;
            g_sites[idx].liveCount  += 1;
            // record by request number so the matching free can decrement
            size_t slot = PtrSlot( reqNum );
            for ( size_t tries = 0; tries < 64; ++tries )
            {
                PtrRec& r = g_ptrTable[ ( slot + tries ) & ( PTR_TABLE_SZ - 1 ) ];
                if ( r.reqNum == 0 ) { r.reqNum = reqNum; r.siteIdx = idx; r.size = size; break; }
            }
        }
        LeaveCriticalSection( &g_allocCs );
        t_inAlloc = 0;
    }

    void AllocProfOnFree( long reqNum )
    {
        if ( !g_allocInited || reqNum == 0 ) return;
        if ( t_inAlloc ) return;
        t_inAlloc = 1;
        EnterCriticalSection( &g_allocCs );
        size_t slot = PtrSlot( reqNum );
        for ( size_t tries = 0; tries < 64; ++tries )
        {
            PtrRec& r = g_ptrTable[ ( slot + tries ) & ( PTR_TABLE_SZ - 1 ) ];
            if ( r.reqNum == reqNum )
            {
                if ( r.siteIdx >= 0 && r.siteIdx < g_numSites )
                {
                    g_sites[r.siteIdx].liveBytes -= (long long)r.size;
                    g_sites[r.siteIdx].liveCount -= 1;
                }
                r.reqNum = 0; r.siteIdx = -1; r.size = 0;
                break;
            }
            if ( r.reqNum == 0 ) break;   // not sampled
        }
        LeaveCriticalSection( &g_allocCs );
        t_inAlloc = 0;
    }

#if defined( _DEBUG )
    // CRT allocation hook. The CRT performs the actual alloc/free; we only
    // observe. Must chain to any previous hook and must return TRUE so the CRT
    // proceeds. _CRT_BLOCK allocations are the CRT's own internal bookkeeping —
    // skip them. On realloc the CRT fires a free for the old request then an
    // alloc for the new, so live tracking stays balanced.
    int AllocHook( int nAllocType, void* /*pvData*/, size_t nSize, int nBlockUse,
                   long lRequest, const unsigned char* /*szFile*/, int /*nLine*/ )
    {
        if ( nBlockUse != _CRT_BLOCK && g_allocInited && !t_inAlloc )
        {
            if ( nAllocType == _HOOK_ALLOC )
                AllocProfOnAlloc( lRequest, nSize );
            else if ( nAllocType == _HOOK_FREE )
                AllocProfOnFree( lRequest );
            // _HOOK_REALLOC: the CRT separately issues FREE(old)+ALLOC(new),
            // so nothing to do here.
        }
        if ( g_prevAllocHook )
            return g_prevAllocHook( nAllocType, NULL, nSize, nBlockUse, lRequest, NULL, 0 );
        return TRUE;
    }
#endif

    // Dumping must NOT hold g_allocCs while doing file I/O or symbolization:
    // those allocate (CRT heap), and the AI worker threads block on g_allocCs in
    // AllocHook -> classic ABBA deadlock with the CRT heap lock. So we SNAPSHOT
    // the top-N sites into a fixed local buffer under the lock, RELEASE the lock,
    // then write raw addresses lock-free. NO in-process symbolization (that was
    // the deadlock/hang source) — addresses + the module base are symbolized
    // offline with the .pdb via d:\tmp\leaksym.exe. Near-instant, deadlock-proof.
    void AllocProfDump()
    {
        if ( !g_allocInited ) return;

        const int MAXSHOW = 30;
        struct Snap { long long trueBytes, liveCount; int nFrames; void* frames[STACK_DEPTH]; };
        static Snap snap[MAXSHOW];   // static: not on the stack, not heap
        int nSnap = 0;

        // ---- under lock: rank + copy out, nothing slow ----
        EnterCriticalSection( &g_allocCs );
        static int order[MAX_SITES];
        int ns = g_numSites;
        for ( int i = 0; i < ns; ++i ) order[i] = i;
        // partial selection sort for the top MAXSHOW by liveBytes
        for ( int i = 0; i < ns && i < MAXSHOW; ++i )
        {
            int best = i;
            for ( int j = i + 1; j < ns; ++j )
                if ( g_sites[order[j]].liveBytes > g_sites[order[best]].liveBytes ) best = j;
            int t = order[i]; order[i] = order[best]; order[best] = t;
            Site& s = g_sites[ order[i] ];
            if ( s.liveBytes <= 0 ) break;
            Snap& d = snap[nSnap++];
            d.trueBytes = s.liveBytes * (long long)g_allocSampleN;
            d.liveCount = s.liveCount;
            d.nFrames   = s.nFrames;
            for ( int fr = 0; fr < s.nFrames; ++fr ) d.frames[fr] = s.frames[fr];
        }
        int sampleN = g_allocSampleN, totalSites = ns;
        LeaveCriticalSection( &g_allocCs );

        // ---- lock-free: write the snapshot ----
        HMODULE hMod = GetModuleHandleA( NULL );
        FILE* f = fopen( "leakstacks.txt", "w" );
        if ( !f ) f = fopen( "d:\\Enemy Nations\\leakstacks.txt", "w" );
        if ( f )
        {
            fprintf( f, "# leak profiler 1/%d sampled. top %d of %d sites by LIVE bytes.\n",
                     sampleN, nSnap, totalSites );
            fprintf( f, "# module_base=0x%llx  (symbolize: leaksym.exe leakstacks.txt <pdbdir>)\n\n",
                     (unsigned long long)(DWORD64)hMod );
            for ( int i = 0; i < nSnap; ++i )
            {
                Snap& d = snap[i];
                fprintf( f, "==== #%d  ~%lld true bytes  liveBlocks=%lld ====\n",
                         i + 1, d.trueBytes, d.liveCount );
                for ( int fr = 0; fr < d.nFrames; ++fr )
                    fprintf( f, "    0x%llx\n", (unsigned long long)(DWORD64)d.frames[fr] );
                fprintf( f, "\n" );
            }
            fclose( f );
            OutputDebugStringA( "ENPERF: wrote leakstacks.txt\n" );
        }
    }

    void AllocProfMaybeDump( long matchSec )
    {
        // Trigger A: fixed time (EN_PERF_LEAKDUMP=<sec>), one-shot.
        bool timeHit = ( !g_allocDumped && g_leakDumpAtSec > 0 && matchSec >= g_leakDumpAtSec );

        // Trigger B: on-demand — a "DUMP_NOW" file appears. RE-TRIGGERABLE: each
        // time the file appears we dump once, then ignore until it's removed.
        bool filePresent =
            ( GetFileAttributesA( "DUMP_NOW" ) != INVALID_FILE_ATTRIBUTES ) ||
            ( GetFileAttributesA( "d:\\Enemy Nations\\DUMP_NOW" ) != INVALID_FILE_ATTRIBUTES );
        bool fileHit = ( filePresent && !g_dumpFileLatched );
        g_dumpFileLatched = filePresent;   // reset once the file is gone

        if ( !timeHit && !fileHit ) return;

        if ( timeHit ) g_allocDumped = true;
        if ( g_allocInited ) AllocProfDump();
        else OutputDebugStringA( "ENPERF: leak dump requested but EN_PERF_ALLOC not set/compiled\n" );
    }

    void OpenLogIfNeeded()
    {
        if ( g_log )
            return;
        g_log = fopen( "perf.log", "a" );
        if ( g_log )
        {
            SYSTEMTIME st;
            GetLocalTime( &st );
            fprintf( g_log,
                     "# ===== perf session start %04d-%02d-%02d %02d:%02d:%02d  interval=%lums =====\n",
                     st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
                     (unsigned long)g_intervalMs );
            fprintf( g_log,
                     "# t=matchSec fps avg=ms max=ms | pump sim render present (ms/interval) "
                     "| ws=workingSetMB priv=privateMB | <counters>\n" );
            fflush( g_log );
        }
    }
}

namespace Perf
{

bool IsEnabled() { return g_enabled; }

void SetEnabled( bool b )
{
    Init();
    g_enabled = b;
}

void Init()
{
    if ( g_inited )
        return;
    g_inited = true;

    if ( !g_csInited )
    {
        InitializeCriticalSection( &g_counterCs );
        g_csInited = true;
    }

    LARGE_INTEGER f;
    QueryPerformanceFrequency( &f );
    g_perfFreq = (double)f.QuadPart;
    if ( g_perfFreq <= 0.0 )
        g_perfFreq = 1.0;

    LARGE_INTEGER c;
    QueryPerformanceCounter( &c );
    g_intervalStart = c.QuadPart;
    g_lastFrameQpc  = c.QuadPart;
    g_startTickMs   = GetTickCount();

    const char* env = getenv( "EN_PERF" );
    if ( env && env[0] && env[0] != '0' )
        g_enabled = true;

    const char* envI = getenv( "EN_PERF_INTERVAL_MS" );
    if ( envI && envI[0] )
    {
        long v = atol( envI );
        if ( v >= 50 && v <= 600000 )
            g_intervalMs = (DWORD)v;
    }

    const char* envL = getenv( "EN_PERF_LEAKDUMP" );
    if ( envL && envL[0] )
        g_leakDumpAtSec = atol( envL );   // match-second at which to dump leaks

    AllocProfInit();                      // arms operator new/delete if EN_PERF_ALLOC set
}

uint64_t Now()
{
    LARGE_INTEGER c;
    QueryPerformanceCounter( &c );
    return (uint64_t)c.QuadPart;
}

void SectionEnd( int slot, uint64_t startTicks )
{
    if ( slot < 0 || slot >= SEC_COUNT )
        return;
    LARGE_INTEGER c;
    QueryPerformanceCounter( &c );
    g_secTicks[slot] += (LONGLONG)( (uint64_t)c.QuadPart - startTicks );
}

void CounterInc( const char* name, int64_t by )
{
    if ( !g_enabled ) return;
    EnterCriticalSection( &g_counterCs );
    Counter* c = FindOrAdd( name, false );
    if ( c ) c->value += by;
    LeaveCriticalSection( &g_counterCs );
}

void CounterAdd( const char* name, int64_t v )
{
    if ( !g_enabled ) return;
    EnterCriticalSection( &g_counterCs );
    Counter* c = FindOrAdd( name, false );
    if ( c ) c->value += v;
    LeaveCriticalSection( &g_counterCs );
}

void CounterAddElapsedUs( const char* name, uint64_t startTicks )
{
    if ( !g_enabled ) return;
    LARGE_INTEGER c;
    QueryPerformanceCounter( &c );
    int64_t us = (int64_t)( ( (double)( (uint64_t)c.QuadPart - startTicks ) / g_perfFreq ) * 1000000.0 );
    EnterCriticalSection( &g_counterCs );
    Counter* cc = FindOrAdd( name, false );
    if ( cc ) cc->value += us;
    LeaveCriticalSection( &g_counterCs );
}

void GaugeSet( const char* name, int64_t v )
{
    if ( !g_enabled ) return;
    EnterCriticalSection( &g_counterCs );
    Counter* c = FindOrAdd( name, true );
    if ( c ) c->value = v;
    LeaveCriticalSection( &g_counterCs );
}

void FrameMark()
{
    if ( !g_enabled )
        return;
    if ( !g_inited )          // SetEnabled(true) may precede Init
        Init();

    LARGE_INTEGER c;
    QueryPerformanceCounter( &c );

    double frameMs = ( (double)( c.QuadPart - g_lastFrameQpc ) / g_perfFreq ) * 1000.0;
    g_lastFrameQpc = c.QuadPart;

    g_frames++;
    g_frameSumMs += frameMs;
    if ( frameMs > g_frameMaxMs )
        g_frameMaxMs = frameMs;

    double elapsedMs = ( (double)( c.QuadPart - g_intervalStart ) / g_perfFreq ) * 1000.0;
    if ( elapsedMs < (double)g_intervalMs )
        return;

    // ----- interval boundary: emit a line -----
    OpenLogIfNeeded();

    double avgMs = g_frames ? ( g_frameSumMs / g_frames ) : 0.0;
    double fps   = ( g_frameSumMs > 0.0 ) ? ( 1000.0 * g_frames / g_frameSumMs ) : 0.0;
    unsigned long matchSec = ( GetTickCount() - g_startTickMs ) / 1000;

    double secMs[SEC_COUNT];
    for ( int i = 0; i < SEC_COUNT; ++i )
        secMs[i] = ( (double)g_secTicks[i] / g_perfFreq ) * 1000.0;

    PROCESS_MEMORY_COUNTERS_EX pmc;
    memset( &pmc, 0, sizeof( pmc ) );
    pmc.cb = sizeof( pmc );
    double wsMB = 0.0, privMB = 0.0;
    if ( GetProcessMemoryInfo( GetCurrentProcess(),
                               (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof( pmc ) ) )
    {
        wsMB   = (double)pmc.WorkingSetSize / ( 1024.0 * 1024.0 );
        privMB = (double)pmc.PrivateUsage  / ( 1024.0 * 1024.0 );
    }

    // NOTE: a previous version sampled the CRT debug heap here via
    // _CrtMemCheckpoint to publish heap.live.blocks / heap.delta.KB. That call
    // walks the entire debug-heap block list and is NOT safe against concurrent
    // allocation: with the AI worker threads allocating ~22k blocks/sec into a
    // ~2M-block list, the list mutated mid-traversal and _CrtMemCheckpoint
    // access-violated inside ucrtbased.dll (confirmed via crash-dump stack).
    // Removed. The sampling allocation profiler below gives the attribution we
    // need without touching the CRT heap list. The OS working-set / private
    // bytes above already show the growth curve.

    // Publish alloc-profiler status so perf.log shows whether the CRT hook is
    // actually recording (init flag, distinct sites, total live sampled bytes).
    if ( g_allocInited )
    {
        long long liveSum = 0;
        for ( int i = 0; i < g_numSites; ++i ) liveSum += g_sites[i].liveBytes;
        GaugeSet( "alloc.sites", g_numSites );
        GaugeSet( "alloc.liveKB", (int64_t)( liveSum / 1024 ) );
    }

    // One-shot: dump the top leaking allocation call stacks (symbolized) once we
    // reach EN_PERF_LEAKDUMP seconds. This is the real attribution — file:line of
    // the call sites holding the most still-live bytes.
    AllocProfMaybeDump( (long)matchSec );

    if ( g_log )
    {
        fprintf( g_log,
                 "t=%-6lu fps=%6.1f avg=%6.2f max=%7.2f | pump=%6.1f sim=%7.1f render=%7.1f present=%6.1f"
                 " msg=%5.0f sleep=%6.0f operB=%5.0f operV=%6.0f operP=%5.0f | ws=%7.1f priv=%7.1f",
                 matchSec, fps, avgMs, g_frameMaxMs,
                 secMs[SEC_PUMP], secMs[SEC_SIM], secMs[SEC_RENDER], secMs[SEC_PRESENT],
                 secMs[SEC_MSG], secMs[SEC_SLEEP], secMs[SEC_OPER_B], secMs[SEC_OPER_V], secMs[SEC_OPER_P],
                 wsMB, privMB );

        EnterCriticalSection( &g_counterCs );
        for ( int i = 0; i < g_numCounters; ++i )
        {
            fprintf( g_log, " | %s=%lld", g_counters[i].name, (long long)g_counters[i].value );
            if ( !g_counters[i].isGauge )
                g_counters[i].value = 0;   // accumulators reset per interval
        }
        LeaveCriticalSection( &g_counterCs );

        fprintf( g_log, "\n" );
        fflush( g_log );
    }

    // reset interval accumulators
    g_frames     = 0;
    g_frameSumMs = 0.0;
    g_frameMaxMs = 0.0;
    for ( int i = 0; i < SEC_COUNT; ++i )
        g_secTicks[i] = 0;
    g_intervalStart = c.QuadPart;
}

} // namespace Perf

// NOTE: The allocation profiler now OBSERVES the CRT heap via _CrtSetAllocHook
// (see AllocHook above) instead of REPLACING global operator new/delete. The CRT
// still performs every allocation, so there is no new/delete-vs-DEBUG_NEW or
// aligned/placement mismatch and no heap-corruption risk — it is safe to leave
// enabled during normal play. (The previous global-operator override has been
// removed.)
