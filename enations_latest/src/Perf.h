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
    void     SectionEnd( int slot, uint64_t startTicks );

    // Fixed slot ids for the hot phases (avoids string lookups on hot path).
    enum Section
    {
        SEC_PUMP = 0,     // message pump / input
        SEC_SIM,          // AI / sim tick (GraphicsEnginePump)
        SEC_RENDER,       // animate + draw
        SEC_PRESENT,      // compositor composite/present
        SEC_COUNT
    };

    // ---- named counters ----------------------------------------------------
    void CounterInc( const char* name, int64_t by = 1 );  // accumulator
    void CounterAdd( const char* name, int64_t v );       // accumulator
    void GaugeSet  ( const char* name, int64_t v );       // gauge (kept)

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
}

#endif // PERF_H
