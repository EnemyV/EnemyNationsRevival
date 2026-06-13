#ifndef __EN_GDI_AUDIT_H__
#define __EN_GDI_AUDIT_H__
//---------------------------------------------------------------------------
// en_gdi_audit.h — TEMPORARY instrumentation for Phase 6 Stage 5a.
//
// Logs the FIRST time each instrumented GDI call site fires (with the calling
// return address, for attribution) so we can measure which remaining GDI
// paths are actually live at runtime vs. dead. Each EN_GDI_PROBE() expansion
// has its own per-site atomic counter, so steady-state cost is one relaxed
// atomic add; the file/OutputDebugString write happens only on the 0th hit.
//
// Enable by defining EN_GDI_AUDIT (done below for the audit run). When the
// macro is compiled out, every probe is ((void)0). REMOVE this header +
// the EN_GDI_PROBE() call sites once Stage 5a's data is captured.
//---------------------------------------------------------------------------

// Flip this to 0 (or delete) to make every probe a no-op.
#define EN_GDI_AUDIT 1

#ifdef EN_GDI_AUDIT

#include <windows.h>
#include <intrin.h>
#include <atomic>
#include <cstdio>
#include <mutex>
#pragma intrinsic(_ReturnAddress)

inline std::mutex& EnGdiAuditMtx() { static std::mutex m; return m; }

// Called exactly once per (call site), on its first hit. Appends immediately
// so the data survives even if the process is killed rather than exited.
inline void EnGdiAuditFirst( const char* label, const void* caller )
{
    std::lock_guard<std::mutex> lk( EnGdiAuditMtx() );
    char line[256];
    _snprintf_s( line, sizeof( line ), _TRUNCATE,
                 "[GDI-AUDIT] %-32s caller=%p\n", label, caller );
    OutputDebugStringA( line );
    FILE* f = nullptr;
    if ( 0 == fopen_s( &f, "d:\\tmp\\gdi_audit.log", "a" ) && f )
    {
        fputs( line, f );
        fclose( f );
    }
}

#define EN_GDI_PROBE( lbl )                                                   \
    do {                                                                      \
        static std::atomic<long> _enGdiHit{ 0 };                              \
        if ( _enGdiHit.fetch_add( 1, std::memory_order_relaxed ) == 0 )       \
            EnGdiAuditFirst( ( lbl ), _ReturnAddress() );                     \
    } while ( 0 )

#else
#define EN_GDI_PROBE( lbl ) ( (void)0 )
#endif

#endif // __EN_GDI_AUDIT_H__
