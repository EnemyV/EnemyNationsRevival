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
static unsigned long g_enRandState = 1;
static void en_srand( unsigned int uSeed ) { g_enRandState = uSeed; }
static int  en_rand() {
    g_enRandState = g_enRandState * 214013UL + 2531011UL;
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

int MyRand() {

    // 15-bit values: iRtn*97 tops out at ~3.2M — no overflow, iInd in [0,97].
    int iInd = ( iRtn * 97 ) / EN_RAND_MAX;
    ASSERT( ( 0 <= iInd ) && ( iInd < 98 ) );

    iRtn = aRnd[iInd];
    aRnd[iInd] = en_rand();

    return ( iRtn );
}

// Full-state fingerprint for the [wg] world-gen parity trace: FNV-1a over the
// LCG state + the Bays-Durham shuffle table, so two processes report equal FPs
// iff their generators are bit-identical (same position in the same stream).
// Read-only — consumes nothing from the stream.
DWORD MyRandFP() {

    DWORD h = 2166136261UL;
    h = ( h ^ (DWORD)g_enRandState ) * 16777619UL;
    h = ( h ^ (DWORD)iRtn ) * 16777619UL;
    for ( int iOn = 0; iOn < 98; iOn++ )
        h = ( h ^ (DWORD)aRnd[iOn] ) * 16777619UL;
    return ( h );
}
