//---------------------------------------------------------------------------
//
//  Perf.h - Lightweight runtime profiling / observability harness.
//
//  Purpose: empirically watch the game degrade over a long match and
//  localize the cost (suspected memory / thread / event leak or unbounded
//  growth). Designed to be near-zero overhead and gated at runtime.
//
//  Enable at runtime (no recompile) via either:
//      - environment variable:  set EN_PERF=1   (before launching)
//      - or call Perf::SetEnabled(true) from code / a debugger.
//  Optional:  set EN_PERF_INTERVAL_MS=1000  (sampling cadence, 50..600000).
//
//  Output: appends to  perf.log  in the process working directory
//  (d:\Enemy Nations\ at runtime) once per sampling interval (default 1s).
//
//  Cost when DISABLED: one bool load per macro hit. The scoped timers and
//  frame marker early-out before touching any clocks.
//
//  How other investigators plug in counters:
//      Perf::CounterInc("alloc.sprite");        // ++ a named accumulator
//      Perf::CounterAdd("net.bytes", n);        // += a named accumulator
//      Perf::GaugeSet("ai.queue.depth", n);     // set a named gauge
//  Accumulators are dumped and reset each interval; gauges hold their last
//  value. Names should be string literals (interned by pointer).
//
//---------------------------------------------------------------------------

#ifndef PERF_H
#define PERF_H

#include <stdint.h>

namespace Perf
{
    // ---- master gate -------------------------------------------------------
    bool IsEnabled();
    void SetEnabled( bool b );

    // Initialise once (reads EN_PERF / EN_PERF_INTERVAL_MS). Safe to call
    // repeatedly; perf.log is opened lazily on first emitted sample.
    void Init();

    // Call exactly once per outer main-loop iteration ("frame"). Updates
    // rolling frame-time stats and, when the interval elapses, flushes one
    // line to perf.log (timing breakdown + memory + counters). Near-zero
    // cost when disabled.
    void FrameMark();

    // ---- section timers ----------------------------------------------------
    uint64_t Now();                              // raw performance counter ticks
    // GATED clock read. Perf::Now() always calls QueryPerformanceCounter, so every
    // inline `const uint64_t t0 = Perf::Now();` pays a syscall even with EN_PERF unset.
    // Use this at probe sites: returns 0 and reads nothing when profiling is off.
    inline uint64_t NowIfEnabled() { return IsEnabled() ? Now() : 0; }
    void     SectionEnd( int slot, uint64_t startTicks );

    // Fixed slot ids for the hot phases (avoids string lookups on hot path).
    enum Section
    {
        SEC_PUMP = 0,     // message pump / input
        SEC_SIM,          // AI / sim tick (GraphicsEnginePump)
        SEC_RENDER,       // animate + draw
        SEC_PRESENT,      // compositor composite/present
        // ---- GraphicsEnginePump sub-breakdown (all nested inside SEC_SIM) ----
        // Decompose the opaque "logic" bucket (sim - render - present) so we can
        // tell real per-unit work from the deliberate frame-pacing Sleep()s. The
        // remainder (logic - msg - sleep - operB - operV - operP) is the once-a-
        // second housekeeping + message-posting scans + animate.
        SEC_MSG,          // ProcessAllMessages
        SEC_SLEEP,        // deliberate frame-pacing Sleep() (NOT cpu work)
        SEC_OPER_B,       // per-building Operate() loop
        SEC_OPER_V,       // per-vehicle Operate() loop
        SEC_OPER_P,       // per-projectile Operate() loop
        SEC_COUNT
    };

    // TRUE only on the thread that called Init() (the main loop). Lock-wait on the
    // MAIN thread is a frame stall; the same wait on an AI worker is not.
    bool IsMainThread();

    // ---- named counters ----------------------------------------------------
    void CounterInc( const char* name, int64_t by = 1 );  // accumulator
    void CounterAdd( const char* name, int64_t v );       // accumulator
    void GaugeSet  ( const char* name, int64_t v );       // gauge (kept)

    // Add elapsed MICROSECONDS (Now()-startTicks) into a named accumulator — for
    // ad-hoc sub-phase profiling finer than the fixed Section slots. The counter
    // then reads as µs/interval (÷1000 ≈ the ms/s render/present columns).
    void CounterAddElapsedUs( const char* name, uint64_t startTicks );
    // Microseconds since a NowIfEnabled() stamp, for probes that keep their own
    // accumulators instead of a named counter. Returns 0 when profiling is off.
    uint64_t ElapsedUs( uint64_t startTicks );

    // RAII scoped timer for a NAMED counter. (Audit note: this is functionally the
    // same shape as ScopeCounter; kept separate only because it early-outs on
    // IsEnabled() at construction. Collapse into ScopeCounter when that is confirmed
    // equivalent at every call site - not done blind.) Use where a fixed Section slot is
    // overkill but an early `goto`/`return` must still be recorded - a bare
    // CounterAddElapsedUs at the end of a block silently loses those paths.
    struct ScopeNamed
    {
        const char* m_name;
        uint64_t    m_start;
        bool        m_active;
        ScopeNamed( const char* name )
        {
            m_active = IsEnabled();
            if ( m_active ) { m_name = name; m_start = Now(); }
        }
        ~ScopeNamed()
        {
            if ( m_active ) CounterAddElapsedUs( m_name, m_start );
        }
    };

    // RAII scoped timer for a fixed slot. Early-outs when disabled.
    struct ScopeSlot
    {
        int      m_slot;
        uint64_t m_start;
        bool     m_active;
        ScopeSlot( int slot )
        {
            m_active = IsEnabled();
            if ( m_active ) { m_slot = slot; m_start = Now(); }
        }
        ~ScopeSlot()
        {
            if ( m_active ) SectionEnd( m_slot, m_start );
        }
    };

    // RAII timer that accumulates µs into a NAMED counter (sub-phase profiling).
    struct ScopeCounter
    {
        const char* m_name;
        uint64_t    m_start;
        bool        m_active;
        ScopeCounter( const char* name )
        {
            m_active = IsEnabled();
            if ( m_active ) { m_name = name; m_start = Now(); }
        }
        ~ScopeCounter()
        {
            if ( m_active ) CounterAddElapsedUs( m_name, m_start );
        }
    };
}

#endif // PERF_H
