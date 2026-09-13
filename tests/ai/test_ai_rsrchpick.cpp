// test_ai_rsrchpick.cpp
//
// Guards bug #73 -- the off-by-one out-of-bounds read in the AI research
// "cheapest available topic" fallback (caigmgr.cpp, CAIGoalMgr::NextResearchTopic).
//
// The defect: the fallback fills the first nCheapest slots of a stack array
//     int aiCheapest[CRsrchArray::num_types];
// and then indexed it with  pGameData->GetRandom( nCheapest ). GetRandom is a
// passthrough to RandNum (caidata.cpp:1208-1212) and RandNum( iMax ) returns
// 0..iMax INCLUSIVE (windward/wind22/src/rand.cpp:65-68 -- the last branch
// clamps *to* iMax), so the draw can equal nCheapest and read aiCheapest
// [nCheapest]: one element PAST the last written slot, an uninitialised value
// handed to the server as a research topic id.
//
// Three layers, on purpose:
//  (1) RNG MIRROR -- an exact copy of the shipped generator (en_rand / MySrand /
//      MyRand / RandNum). Used to prove the bad draw is REAL, not hypothetical:
//      RandNum( n ) genuinely returns n on the actual stream.
//  (2) THE SHIPPED HELPER -- enaipick::PickFilled from enations_latest/src/aipick.h
//      is compiled here directly (not copied), so the fixture tests production code.
//  (3) SOURCE LINT -- caigmgr.cpp must carry the fixed call and must NOT carry the
//      pre-fix expression, so the helper can't be bypassed by a revert.
//
// Standalone: links no game code, touches no game build. Safe to run anytime.
//
// Usage: ai_rsrchpick_tests.exe [<path-to-caigmgr.cpp>]
// Exit:  0 all pass, 1 a check failed, 2 cannot-open the source (skip).

#define _CRT_SECURE_NO_WARNINGS

#include "microtest.h"

// the SHIPPED helper, compiled from the game's own header
#include "../../enations_latest/src/aipick.h"

#include <cstdio>
#include <string>

// ---------------------------------------------------------------------------
// (1) RNG mirror -- byte-for-byte the shipped generator (rand.cpp:38-68,
//     104-170). Pure ints, no platform dependency, so the numbers below are the
//     numbers the game draws.
// ---------------------------------------------------------------------------
namespace rngmirror {

#define EN_RAND_MAX 0x7FFF

static unsigned int g_enRandState = 1;
static void en_srand( unsigned int uSeed ) { g_enRandState = uSeed; }
static int  en_rand() {
    g_enRandState = g_enRandState * 214013U + 2531011U;
    return (int)( ( g_enRandState >> 16 ) & 0x7FFF );
}

static int aRnd[98];
static int iRtn = 0;

static void MySrand( unsigned long dwSeed ) {
    en_srand( (unsigned int)dwSeed );
    for ( int iOn = 0; iOn < 98; iOn++ )
        aRnd[iOn] = en_rand();
    iRtn = en_rand();
}

static int MyRand() {
    int iInd   = ( iRtn * 97 ) / EN_RAND_MAX;
    iRtn       = aRnd[iInd];
    aRnd[iInd] = en_rand();
    return ( iRtn );
}

// rand.cpp:47-68 -- the ASSERT( iRtn <= iMax ) and the final clamp TO iMax are
// the proof that the contract is 0..iMax inclusive.
static int RandNum( int iMax ) {
    if ( iMax <= 0 )
        return ( 0 );
    if ( iMax >= EN_RAND_MAX ) {
        if ( iMax < 0x7FFFFFFF / EN_RAND_MAX )
            return ( ( MyRand() * iMax ) / EN_RAND_MAX );
        return ( MyRand() * ( iMax / EN_RAND_MAX ) );
    }
    int r = MyRand() / ( EN_RAND_MAX / ( iMax + 1 ) );
    if ( r > iMax )
        return ( iMax );
    return ( r );
}

// CAIData::GetRandom is a straight passthrough (caidata.cpp:1208-1212).
static int GetRandom( int iDice ) { return ( RandNum( iDice ) ); }

} // namespace rngmirror

// ---------------------------------------------------------------------------
// The two selection expressions under test.
//   PreFix: what caigmgr.cpp:2155 did -- index straight with GetRandom( n ).
//   Fixed : what it does now -- draw over n-1 and clamp into the filled prefix.
// ---------------------------------------------------------------------------
static int SelectPreFix( int nFilled ) {
    return ( rngmirror::GetRandom( nFilled ) );            // can return nFilled
}

static int SelectFixed( int nFilled ) {
    if ( nFilled <= 0 )
        return ( -1 );
    return ( enaipick::PickFilled( nFilled, rngmirror::GetRandom( nFilled - 1 ) ) );
}

// ---- the engine's draw really does hit the bound (so the OOB is reachable) --
static void test_randnum_bound_is_inclusive() {
    // closed-form: the largest raw MyRand value maps onto iMax for any small iMax
    for ( int iMax = 1; iMax < 60; ++iMax ) {
        int raw   = EN_RAND_MAX;                            // MyRand()'s maximum
        int naive = raw / ( EN_RAND_MAX / ( iMax + 1 ) );
        CHECK( naive >= iMax );                             // ...so the clamp returns iMax
    }
    // and on the REAL stream: RandNum( n ) returns n for a typical tie-break size
    rngmirror::MySrand( 0 );
    const int n = 3;
    int atBound = 0;
    for ( int i = 0; i < 20000; ++i )
        if ( rngmirror::RandNum( n ) == n )
            ++atBound;
    CHECK( atBound > 0 );            // the bad draw is REAL, not theoretical
    std::printf( "[rsrchpick] RandNum(%d) returned %d on %d of 20000 draws\n", n, n, atBound );
}

// ---- the pre-fix expression indexes past the filled prefix -----------------
static void test_prefix_expression_reads_past_the_array() {
    // 53-slot stack array (CRsrchArray::num_types); only the first nFilled are
    // written, the rest is whatever was on the stack -- modelled as a canary.
    const int kSlots  = 53;
    const int kCanary = -777;
    int       aiCheapest[kSlots];
    for ( int i = 0; i < kSlots; ++i )
        aiCheapest[i] = kCanary;
    const int nFilled = 3;
    for ( int i = 0; i < nFilled; ++i )
        aiCheapest[i] = 100 + i;     // the real topic ids

    rngmirror::MySrand( 0 );
    int nGarbage = 0, nOutOfRange = 0;
    for ( int i = 0; i < 20000; ++i ) {
        int idx = SelectPreFix( nFilled );
        if ( idx >= nFilled )
            ++nOutOfRange;
        if ( aiCheapest[idx] == kCanary )
            ++nGarbage;                                     // uninitialised read
    }
    CHECK( nOutOfRange > 0 );        // would be 0 if the draw were exclusive
    CHECK_EQ( nGarbage, nOutOfRange );
    std::printf( "[rsrchpick] pre-fix expression read an UNWRITTEN slot %d / 20000 times\n", nGarbage );
}

// ---- the fixed expression never leaves the filled prefix -------------------
static void test_fixed_expression_stays_in_range() {
    rngmirror::MySrand( 0 );
    for ( int nFilled = 1; nFilled <= 53; ++nFilled )
        for ( int i = 0; i < 2000; ++i ) {
            int idx = SelectFixed( nFilled );
            CHECK( idx >= 0 && idx < nFilled );
        }
    // the same 20000-draw loop that tripped 1600-ish times pre-fix: zero now
    rngmirror::MySrand( 0 );
    const int nFilled = 3;
    int nOutOfRange = 0;
    for ( int i = 0; i < 20000; ++i )
        if ( SelectFixed( nFilled ) >= nFilled )
            ++nOutOfRange;
    CHECK_EQ( nOutOfRange, 0 );
}

// ---- the fix is a NO-OP for every draw that was already in range ----------
static void test_pick_is_identity_in_range() {
    for ( int nFilled = 1; nFilled <= 64; ++nFilled )
        for ( int r = 0; r < nFilled; ++r )
            CHECK_EQ( enaipick::PickFilled( nFilled, r ), r );   // same element as before
    // out-of-range draws are clamped INTO the prefix, never past it
    for ( int nFilled = 1; nFilled <= 64; ++nFilled ) {
        CHECK_EQ( enaipick::PickFilled( nFilled, nFilled ), nFilled - 1 );
        CHECK_EQ( enaipick::PickFilled( nFilled, 1 << 20 ), nFilled - 1 );
        CHECK_EQ( enaipick::PickFilled( nFilled, -5 ), 0 );
    }
    // nothing filled -> "no choice"; the caller must not index anything
    CHECK_EQ( enaipick::PickFilled( 0, 0 ), -1 );
    CHECK_EQ( enaipick::PickFilled( -1, 3 ), -1 );
}

// ---- the clamp is belt-and-braces: with the n-1 draw it never fires --------
// (so the fix changes WHICH index is drawn, not the distribution over the
//  filled slots -- every slot stays reachable and none is over-weighted by a
//  clamp.)
static void test_clamp_never_fires_and_every_slot_reachable() {
    rngmirror::MySrand( 0 );
    const int nFilled = 4;
    int hits[4] = { 0, 0, 0, 0 };
    int clamped = 0;
    for ( int i = 0; i < 20000; ++i ) {
        int raw = rngmirror::GetRandom( nFilled - 1 );
        if ( raw >= nFilled )
            ++clamped;
        int idx = enaipick::PickFilled( nFilled, raw );
        CHECK( idx >= 0 && idx < nFilled );
        ++hits[idx];
    }
    CHECK_EQ( clamped, 0 );                   // GetRandom( n-1 ) is already in range
    for ( int i = 0; i < nFilled; ++i )
        CHECK( hits[i] > 0 );                 // no slot starved
}

// ---------------------------------------------------------------------------
// (3) Source lint -- the shipped caigmgr.cpp must use the helper, and the
//     pre-fix expression must be gone.
// ---------------------------------------------------------------------------
namespace {

std::string ReadAll( const char* path ) {
    std::FILE* f = std::fopen( path, "rb" );
    if ( !f ) return std::string();
    std::fseek( f, 0, SEEK_END );
    long n = std::ftell( f );
    std::fseek( f, 0, SEEK_SET );
    std::string s( (size_t)( n > 0 ? n : 0 ), '\0' );
    if ( n > 0 && std::fread( &s[0], 1, (size_t)n, f ) != (size_t)n ) s.clear();
    std::fclose( f );
    return s;
}

std::string NoWs( const std::string& s ) {
    std::string o;
    o.reserve( s.size() );
    for ( size_t i = 0; i < s.size(); ++i ) {
        char c = s[i];
        if ( c != ' ' && c != '\t' && c != '\r' && c != '\n' ) o.push_back( c );
    }
    return o;
}

bool Has( const std::string& hay, const char* needle ) {
    return hay.find( needle ) != std::string::npos;
}

} // namespace

static void test_source_uses_the_helper( const std::string& sq ) {
    CHECK( Has( sq, "#include\"aipick.h\"" ) );
    CHECK( Has( sq, "enaipick::PickFilled(nCheapest,pGameData->GetRandom(nCheapest-1))" ) );
    CHECK( Has( sq, "intiTopic=aiCheapest[iPick];" ) );
    CHECK( Has( sq, "if(nCheapest<=0)" ) );
}

static void test_source_has_no_prefix_form( const std::string& sq ) {
    // the exact pre-fix expression -- fails on the unfixed tree
    CHECK( !Has( sq, "aiCheapest[pGameData->GetRandom(nCheapest)]" ) );
    // and no other raw "draw over the count" index into the array
    CHECK( !Has( sq, "aiCheapest[pGameData->GetRandom(nCheapest)];" ) );
}

int main( int argc, char** argv ) {
    test_randnum_bound_is_inclusive();
    test_prefix_expression_reads_past_the_array();
    test_fixed_expression_stays_in_range();
    test_pick_is_identity_in_range();
    test_clamp_never_fires_and_every_slot_reachable();

    if ( argc < 2 ) {
        std::printf( "[rsrchpick] no source path given -- lint SKIPPED\n" );
        return microtest::Summary();
    }
    std::string src = ReadAll( argv[1] );
    if ( src.empty() ) {
        std::printf( "[rsrchpick] SKIP: cannot read %s\n", argv[1] );
        return 2;
    }
    std::string sq = NoWs( src );
    std::printf( "[rsrchpick] linting %s (%d bytes)\n", argv[1], (int)src.size() );
    test_source_uses_the_helper( sq );
    test_source_has_no_prefix_form( sq );

    return microtest::Summary();
}
