//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------


// rand.cpp - random numbers
//

#include "stdafx.h"
#include "_windwrd.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


// --- Cross-platform deterministic rand (MP world-gen parity) ---------------
// MyRand feeds ALL deterministic game randomness (world-gen, placement, combat
// rolls). It used to ride the platform libc rand(): MSVC = 15-bit LCG
// (RAND_MAX 0x7FFF), glibc = 31-bit additive-feedback (RAND_MAX 0x7FFFFFFF) —
// completely different streams AND ranges from the same seed, so a Windows
// host and a POSIX client could NEVER generate the same world: CmdPlay's
// m_dwFinalRand handshake dropped every cross-platform joiner (live-confirmed
// Win↔Linux: client=56d99f39 vs host=0000091a). glibc's 31-bit range also
// overflowed the shuffle index below (iRtn*97) — the 1996 "BUGBUG platform
// dependent?" comment was right. All platforms now use the documented MSVC CRT
// LCG, bit-identical to a Windows host; MSVC builds are unchanged in behavior.
#define EN_RAND_MAX 0x7FFF
// unsigned int, NOT unsigned long: long is 64-bit on LP64 (Linux/mac) but
// 32-bit on Windows, so a `long` state silently accumulated 64 bits on POSIX.
// Outputs only read bits 16-30 of the low 32 so it happened to match — but
// any future reader of the wider state (fingerprint, wider rand) would
// diverge cross-platform. 32-bit state == the MSVC CRT's, exactly.
static unsigned int g_enRandState = 1;
static void en_srand( unsigned int uSeed ) { g_enRandState = uSeed; }
static int  en_rand() {
    g_enRandState = g_enRandState * 214013U + 2531011U;
    return (int)( ( g_enRandState >> 16 ) & 0x7FFF );
}

int RandNum( int iMax ) {

    ASSERT( iMax >= 0 );
    if ( iMax <= 0 )
        return ( 0 );

    // can be BIG
    if ( iMax >= EN_RAND_MAX ) {
        if ( iMax < INT_MAX / EN_RAND_MAX ) {
            int iRtn = ( MyRand() * iMax ) / EN_RAND_MAX;
            ASSERT( iRtn <= iMax );
            return ( iRtn );
        }
        int iRtn = MyRand() * ( iMax / EN_RAND_MAX );
        ASSERT( iRtn <= iMax );
        return ( iRtn );
    }

    int iRtn = MyRand() / ( EN_RAND_MAX / ( iMax + 1 ) );
    if ( iRtn > iMax )
        return ( iMax );
    return ( iRtn );
}


class xRand {
public:
    xRand() { MySrand( 0 ); }
};

static xRand needTheCtor;
static int aRnd[98];
static int iRtn = 0;

DWORD MySeed() {

    SYSTEMTIME _st;
    GetSystemTime( &_st );
    unsigned uRand = ( (unsigned)_st.wDay << 28 ) | ( (unsigned)( _st.wMinute & 31 ) << 23 ) |
        ( (unsigned)( _st.wSecond & 31 ) << 18 ) | ( (unsigned)( _st.wMonth & 3 ) << 16 ) |
        ( timeGetTime() & 0xFFFF );

    return ( uRand );
}

void MySrand( DWORD dwSeed ) {

    en_srand( dwSeed );

    for ( int iOn = 0; iOn < 98; iOn++ )
        aRnd[iOn] = en_rand();
    iRtn = en_rand();
}

// --- MP world-gen parity trace (env EN_RANDTRACE) --------------------------
// Every deterministic draw goes through MyRand(), so a monotonic call count +
// rolling checksum of the returned stream is a compact fingerprint of "how far
// the PRNG has been walked and with what results". A Windows host and a POSIX
// client that generate IDENTICAL worlds must have identical (calls,sum) at every
// MyRandTrace mark; the FIRST mark where they differ localizes the cross-platform
// desync that trips CmdPlay's m_dwFinalRand handshake (the client-kicked-to-menu
// RAND MISMATCH). Both counters use fixed-width, platform-identical arithmetic
// (unsigned long long / unsigned), so they compare byte-for-byte across Win/Linux/
// mac. The counter maths are unconditional (2 trivial int ops on top of MyRand's
// existing divide+array work — negligible); only the stderr dump is env-gated.
unsigned long long g_myRandCalls = 0;
unsigned           g_myRandSum   = 0;

static int RandTraceOn() {
    static int s_on = -1;
    if ( s_on < 0 ) {
        const char* e = getenv( "EN_RANDTRACE" );
        s_on = ( e != NULL && *e != '\0' && *e != '0' ) ? 1 : 0;
    }
    return s_on;
}

void MyRandTrace( const char* pszLabel ) {
    if ( !RandTraceOn() )
        return;
    fprintf( stderr, "[randtrace] %-30s calls=%llu sum=%08x\n",
             pszLabel ? pszLabel : "(mark)",
             (unsigned long long)g_myRandCalls, g_myRandSum );
}

int MyRand() {

    // 15-bit values: iRtn*97 tops out at ~3.2M — no overflow, iInd in [0,97].
    int iInd = ( iRtn * 97 ) / EN_RAND_MAX;
    ASSERT( ( 0 <= iInd ) && ( iInd < 98 ) );

    iRtn = aRnd[iInd];
    aRnd[iInd] = en_rand();

    // MP-parity fingerprint (see MyRandTrace). Rolling 32-bit hash of the stream;
    // wraps identically on every platform.
    ++g_myRandCalls;
    g_myRandSum = g_myRandSum * 1000003u + (unsigned)iRtn;

    return ( iRtn );
}

