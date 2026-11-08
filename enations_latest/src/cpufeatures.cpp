//---------------------------------------------------------------------------
// cpufeatures.cpp — see cpufeatures.h for why this exists (GH #8: the /arch:AVX2
// build would not start on pre-2013 CPUs, silently).
//
// Self-contained on purpose: this runs during very early init, before the app
// object and the window exist, so it must not depend on either. Logs to
// GameWindow_Debug.log AND stderr, matching the FONT: diagnostic — a player
// launching from a desktop icon has no terminal, so stderr alone reaches nobody.
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "en_logpath.h"   // EnLogPath - logs to the launch dir, not the exe dir
#include "cpufeatures.h"

#include <stdio.h>
#include <string.h>

// x86 only. macOS ships clang/ARM64, where CPUID does not exist — every query
// there answers false and the baseline path is used, which is correct.
#if defined( _M_X64 ) || defined( _M_IX86 ) || defined( __x86_64__ ) || defined( __i386__ )
    #define EN_CPU_X86 1
    #if defined( _MSC_VER )
        #include <intrin.h>
        #include <immintrin.h>
    #else
        #include <cpuid.h>
    #endif
#else
    #define EN_CPU_X86 0
#endif

namespace
{

struct Features
{
    bool sse42 = false;
    bool avx   = false;
    bool avx2  = false;
    bool fma   = false;
    char text[ 64 ] = { 0 };
};

#if EN_CPU_X86
// NB: the intrinsic is called ::-qualified. Unqualified lookup from inside this
// anonymous namespace picked up a same-named candidate and failed to resolve.
inline void EnCpuIdEx( int leaf, int sub, unsigned int out[ 4 ] )
{
    #if defined( _MSC_VER )
    int regs[ 4 ];
    ::__cpuidex( regs, leaf, sub );
    out[ 0 ] = (unsigned)regs[ 0 ]; out[ 1 ] = (unsigned)regs[ 1 ];
    out[ 2 ] = (unsigned)regs[ 2 ]; out[ 3 ] = (unsigned)regs[ 3 ];
    #else
    unsigned int a = 0, b = 0, c = 0, d = 0;
    // __cpuid_count is a MACRO in glibc's <cpuid.h> (expands to inline asm),
    // not a function — a leading "::" breaks the substitution on gcc (the
    // "unqualified lookup picked up a same-named candidate" note above is
    // about the MSVC/clang intrinsics only; this branch has no such name to
    // disambiguate from, and gcc/clang's own __cpuid_count needs none).
    __cpuid_count( leaf, sub, a, b, c, d );
    out[ 0 ] = a; out[ 1 ] = b; out[ 2 ] = c; out[ 3 ] = d;
    #endif
}

// XCR0 — the OS must have ENABLED ymm state saving, not merely the CPU support
// it. Skipping this check is the classic AVX-detection bug: the instruction
// still faults on an OS that doesn't preserve the upper halves across a context
// switch. Only legal once CPUID.1:ECX.OSXSAVE[27] is set.
inline unsigned long long XGetBv0( )
{
    #if defined( _MSC_VER )
    return _xgetbv( 0 );
    #else
    unsigned int eax, edx;
    __asm__ __volatile__( "xgetbv" : "=a"( eax ), "=d"( edx ) : "c"( 0 ) );
    return ( (unsigned long long)edx << 32 ) | eax;
    #endif
}
#endif  // EN_CPU_X86

Features Detect( )
{
    Features f;
#if EN_CPU_X86
    unsigned int r[ 4 ] = { 0, 0, 0, 0 };
    EnCpuIdEx( 0, 0, r );
    const unsigned int maxLeaf = r[ 0 ];

    if ( maxLeaf >= 1 )
    {
        EnCpuIdEx( 1, 0, r );
        const unsigned int ecx = r[ 2 ];
        f.sse42 = ( ecx & ( 1u << 20 ) ) != 0;
        const bool osxsave  = ( ecx & ( 1u << 27 ) ) != 0;
        const bool cpuAvx   = ( ecx & ( 1u << 28 ) ) != 0;
        const bool cpuFma   = ( ecx & ( 1u << 12 ) ) != 0;

        bool osAvx = false;
        if ( osxsave )
        {
            const unsigned long long xcr0 = XGetBv0( );
            osAvx = ( xcr0 & 0x6ull ) == 0x6ull;   // XMM (bit1) + YMM (bit2)
        }
        f.avx = cpuAvx && osAvx;
        f.fma = cpuFma && f.avx;

        if ( f.avx && maxLeaf >= 7 )
        {
            EnCpuIdEx( 7, 0, r );
            f.avx2 = ( r[ 1 ] & ( 1u << 5 ) ) != 0;   // CPUID.7.0:EBX.AVX2[5]
        }
    }
#endif

    f.text[ 0 ] = 0;
    if ( !f.sse42 && !f.avx && !f.avx2 && !f.fma )
    {
        strcpy( f.text, "baseline(SSE2)" );
    }
    else
    {
        if ( f.sse42 ) strcat( f.text, "SSE4.2 " );
        if ( f.avx )   strcat( f.text, "AVX " );
        if ( f.avx2 )  strcat( f.text, "AVX2 " );
        if ( f.fma )   strcat( f.text, "FMA " );
        size_t n = strlen( f.text );
        if ( n && f.text[ n - 1 ] == ' ' ) f.text[ n - 1 ] = 0;
    }
    return f;
}

const Features& Get( )
{
    static const Features f = Detect( );   // CPUID runs exactly once
    return f;
}

}  // namespace

namespace EnCpu
{

bool HasSSE42( ) { return Get( ).sse42; }
bool HasAVX( )   { return Get( ).avx; }
bool HasAVX2( )  { return Get( ).avx2; }
bool HasFMA( )   { return Get( ).fma; }

const char* Summary( ) { return Get( ).text; }

void LogFeatures( )
{
    char line[ 128 ];
    _snprintf( line, sizeof( line ) - 1, "CPU: %s", Summary( ) );
    line[ sizeof( line ) - 1 ] = 0;

    // Both sinks: a desktop-icon launch has no terminal, so stderr alone is
    // invisible; the log file is what a bug report can actually attach.
    fprintf( stderr, "%s\n", line );
    FILE* fp = fopen( EnLogPath( "GameWindow_Debug.log" ).c_str(), "a" );
    if ( fp )
    {
        fprintf( fp, "%s\n", line );
        fclose( fp );
    }
}

}  // namespace EnCpu
