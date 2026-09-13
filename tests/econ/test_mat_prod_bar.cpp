// test_mat_prod_bar.cpp
//
// Guards bug #13 -- "smelter production bar advances without producing".
//
// The sim (CBuilding::BuildMaterials, mainloop.cpp:2686-2690) advances
// m_iBuildDone by GetProd( ), which is driven by POWER and PEOPLE only -- there
// is no input gate. The input check happens later, at batch completion
// (mainloop.cpp:2717-2790): iNum = GetStore( i ) / GetInput( i ) truncates to
// zero batches when a required input cannot fund one, so NOTHING is emitted, the
// accumulated progress is thrown away (m_iBuildDone = 0) and the building halts
// with EVENT_MANUF_HALTED. The info-window bar reads m_iBuildDone / GetTime( )
// (CMaterialBuilding::GetProductionPer, new_unit.cpp), so an iron-starved
// smelter swept the bar 0 -> 100% while producing no steel at all.
//
// The fix is DISPLAY-SIDE: the bar reads 0 whenever a required input store cannot
// fund one batch. The sim is deliberately untouched -- reaching the completion
// branch starved is what raises the halt event and the resupply request, and it
// is what keeps an intermittently-fed converter's throughput unchanged.
//
// Three layers:
//  (1) SIM MIRROR -- a line-by-line copy of BuildMaterials' numeric core
//      (accumulate / complete / clamp to stock / consume / emit / bOut reset).
//      It is the thing being "driven" here: inputs present vs absent.
//  (2) THE SHIPPED PREDICATE -- enecon::MatBarStalled from
//      enations_latest/src/econprod.h is compiled here directly (not copied), so
//      the bar logic under test is the production one.
//  (3) SOURCE LINT -- new_unit.cpp must carry the gate and must no longer carry
//      the ungated pre-fix body.
//
// Standalone: links no game code, touches no game build. Safe to run anytime.
//
// Usage: mat_prod_bar_tests.exe [<path-to-new_unit.cpp>]
// Exit:  0 all pass, 1 a check failed, 2 cannot-open the source (skip).

#define _CRT_SECURE_NO_WARNINGS

#include "../ai/microtest.h"

// the SHIPPED predicate, compiled from the game's own header
#include "../../enations_latest/src/econprod.h"

#include <cstdio>
#include <cstring>
#include <string>

// ---------------------------------------------------------------------------
// (1) Sim mirror -- CBuilding::BuildMaterials' numeric core, bBioFuel == false.
//     Material indices match CMaterialTypes (base.h:725): steel 1, coal 8, iron 9.
// ---------------------------------------------------------------------------
namespace sim {

const int kNumTypes = 10;                 // CMaterialTypes::num_types
const int kSteel = 1, kCoal = 8, kIron = 9;

struct Conv {
    int  iTime;                           // pBm->GetTime( )
    int  aiInput[kNumTypes];              // per batch
    int  aiOutput[kNumTypes];             // per batch
    int  aiStore[kNumTypes];              // GetStore( )
    int  aiMade[kNumTypes];               // IncMaterialMade( ) running total
    int  iBuildDone;                      // m_iBuildDone
    bool bHalted;                         // SetFlag( event ) from the ran-out branch
    int  nHalts;

    Conv() { std::memset( this, 0, sizeof( *this ) ); }
};

// One CBuilding::BuildMaterials( ) call with a production increment of iInc.
// Structure and arithmetic copied from mainloop.cpp:2686-2790.
inline void Step( Conv& c, int iInc )
{
    if ( iInc < 1 )
        return;

    c.iBuildDone += iInc;                               // NO input gate (the bug's source)

    if ( c.iBuildDone < c.iTime )
        return;

    int iNum = c.iBuildDone / c.iTime;
    c.iBuildDone -= c.iTime * iNum;

    // materials to build it?
    bool bOut = false;
    for ( int i = 0; i < kNumTypes; ++i )
        if ( c.aiStore[i] < c.aiInput[i] * iNum )
        {
            bOut = true;
            iNum = c.aiStore[i] / c.aiInput[i];         // integer divide -> 0 when short
        }

    if ( iNum )
    {
        for ( int i = 0; i < kNumTypes; ++i )
            c.aiStore[i] -= c.aiInput[i] * iNum;
        for ( int i = 0; i < kNumTypes; ++i )
        {
            int iAmt = c.aiOutput[i] * iNum;
            c.aiStore[i] += iAmt;
            c.aiMade[i] += iAmt;
        }
    }

    // if we ran out - stop us
    if ( bOut )
    {
        c.iBuildDone = 0;
        c.bHalted    = true;
        ++c.nHalts;
    }
}

// A smelter-shaped converter: iron + coal -> steel.
inline Conv Smelter( int iIron, int iCoal )
{
    Conv c;
    c.iTime            = 100;
    c.aiInput[kIron]   = 4;
    c.aiInput[kCoal]   = 2;
    c.aiOutput[kSteel] = 1;
    c.aiStore[kIron]   = iIron;
    c.aiStore[kCoal]   = iCoal;
    return ( c );
}

} // namespace sim

// ---------------------------------------------------------------------------
// (2) The bar. PreFix = the shipped-before expression; Fixed = the shipped-now
//     one, which calls the real enecon::MatBarStalled.
// ---------------------------------------------------------------------------
static int BarPreFix( const sim::Conv& c )
{
    if ( c.iTime <= 0 ) return ( -1 );
    int iPer = (int)( ( (long long)c.iBuildDone * 100 ) / c.iTime );
    return ( iPer > 100 ? 100 : iPer );
}

static int BarFixed( const sim::Conv& c, bool bAltActive = false )
{
    if ( c.iTime <= 0 ) return ( -1 );
    if ( !bAltActive )
        if ( enecon::MatBarStalled( sim::kNumTypes,
                                    [&c]( int i ) { return c.aiInput[i]; },
                                    [&c]( int i ) { return c.aiStore[i]; } ) )
            return ( 0 );
    int iPer = (int)( ( (long long)c.iBuildDone * 100 ) / c.iTime );
    return ( iPer > 100 ? 100 : iPer );
}

// The weaker "store is EMPTY" rule, kept only to show why the shipped gate uses
// "< one batch" instead (scenario 3).
static int BarEmptyOnlyGate( const sim::Conv& c )
{
    if ( c.iTime <= 0 ) return ( -1 );
    for ( int i = 0; i < sim::kNumTypes; ++i )
        if ( c.aiInput[i] > 0 && c.aiStore[i] <= 0 )
            return ( 0 );
    int iPer = (int)( ( (long long)c.iBuildDone * 100 ) / c.iTime );
    return ( iPer > 100 ? 100 : iPer );
}

// ---- 1. starved smelter: the bar must NOT advance, and nothing is produced ---
static void test_starved_bar_does_not_advance() {
    sim::Conv c = sim::Smelter( /*iron=*/0, /*coal=*/0 );
    int maxPreFix = 0, maxFixed = 0;
    for ( int step = 0; step < 4; ++step )      // 4 x 25 = one full cycle
    {
        sim::Step( c, 25 );
        if ( BarPreFix( c ) > maxPreFix ) maxPreFix = BarPreFix( c );
        if ( BarFixed( c ) > maxFixed )   maxFixed  = BarFixed( c );
        CHECK_EQ( BarFixed( c ), 0 );           // honest: no progress toward any steel
    }
    // the sim did exactly what the bug report says: swept the bar, produced nothing
    CHECK( maxPreFix >= 75 );                   // pre-fix bar climbed (this is the defect)
    CHECK_EQ( maxFixed, 0 );
    CHECK_EQ( c.aiMade[sim::kSteel], 0 );       // no steel at all
    CHECK( c.bHalted );                         // ...and the sim halted + warned, as before
    std::printf( "[matbar] starved: pre-fix bar peaked at %d%%, fixed bar 0%%, steel made %d\n",
                 maxPreFix, c.aiMade[sim::kSteel] );
}

// ---- 2. fed smelter: the fixed bar is IDENTICAL to the pre-fix arithmetic ----
static void test_fed_bar_is_unchanged() {
    sim::Conv cA = sim::Smelter( 1000, 1000 );
    sim::Conv cB = sim::Smelter( 1000, 1000 );
    for ( int step = 0; step < 40; ++step )
    {
        sim::Step( cA, 25 );
        sim::Step( cB, 25 );
        CHECK_EQ( BarFixed( cA ), BarPreFix( cB ) );    // byte-identical while fed
    }
    CHECK( cA.aiMade[sim::kSteel] > 0 );
    CHECK( !cA.bHalted );
    // every ordinary increment size, still identical
    for ( int iInc = 1; iInc <= 60; ++iInc )
    {
        sim::Conv c = sim::Smelter( 100000, 100000 );
        for ( int step = 0; step < 25; ++step )
        {
            sim::Step( c, iInc );
            CHECK_EQ( BarFixed( c ), BarPreFix( c ) );
        }
    }
    std::printf( "[matbar] fed: steel made %d, bar identical to pre-fix on every step\n",
                 cA.aiMade[sim::kSteel] );
}

// ---- 3. partial stock that cannot fund ONE batch: still produces nothing ----
static void test_partial_stock_under_one_batch() {
    // 3 iron against a 4-iron recipe: iNum = 3/4 = 0 -> no steel, bOut, progress dropped
    sim::Conv c = sim::Smelter( /*iron=*/3, /*coal=*/1000 );
    int maxEmptyOnly = 0;
    for ( int step = 0; step < 4; ++step )
    {
        sim::Step( c, 25 );
        CHECK_EQ( BarFixed( c ), 0 );                   // shipped gate: honest
        if ( BarEmptyOnlyGate( c ) > maxEmptyOnly ) maxEmptyOnly = BarEmptyOnlyGate( c );
    }
    CHECK_EQ( c.aiMade[sim::kSteel], 0 );               // nothing produced...
    CHECK( maxEmptyOnly >= 75 );                        // ...so an "empty store" rule would lie too
}

// ---- 4. mid-run starvation: invisible while the batch can complete ----------
static void test_starves_only_after_stock_runs_out() {
    sim::Conv c = sim::Smelter( /*iron=*/4, /*coal=*/2 );   // exactly one batch
    for ( int step = 0; step < 3; ++step )                  // first cycle: fully funded
    {
        sim::Step( c, 25 );
        CHECK_EQ( BarFixed( c ), BarPreFix( c ) );          // fix invisible -- it CAN produce
    }
    sim::Step( c, 25 );                                     // completes: steel out, stock gone
    CHECK_EQ( c.aiMade[sim::kSteel], 1 );
    CHECK( !c.bHalted );                                    // funded batch -> no halt
    // now the stock is empty; the sim would keep advancing, the bar must not
    for ( int step = 0; step < 3; ++step )
    {
        sim::Step( c, 25 );
        CHECK_EQ( BarFixed( c ), 0 );
        CHECK( BarPreFix( c ) > 0 );                        // the pre-fix lie, measured
    }
}

// ---- 5. multi-batch overshoot, fed: unchanged -------------------------------
static void test_overshoot_batches_unchanged() {
    sim::Conv c = sim::Smelter( 100000, 100000 );
    for ( int step = 0; step < 10; ++step )
    {
        sim::Step( c, 250 );                                // 2.5 batches per call
        CHECK_EQ( BarFixed( c ), BarPreFix( c ) );
    }
    CHECK( c.aiMade[sim::kSteel] >= 20 );
}

// ---- 6. alt-output conversion: the gate must NOT apply ----------------------
// A BioFuel refinery consumes no local input at all (mainloop.cpp:2677 skips the
// clamp and the consumption pass), so its bar legitimately advances on an empty
// input store.
static void test_alt_output_mode_is_not_gated() {
    sim::Conv c = sim::Smelter( /*iron=*/0, /*coal=*/0 );   // shaped like a starved converter
    for ( int step = 0; step < 3; ++step )
    {
        sim::Step( c, 25 );
        CHECK_EQ( BarFixed( c, /*bAltActive=*/true ), BarPreFix( c ) );  // advances
        CHECK_EQ( BarFixed( c, /*bAltActive=*/false ), 0 );              // gated without alt
    }
}

// ---- 7. a producer with no inputs is never gated ---------------------------
static void test_inputless_producer_unaffected() {
    sim::Conv c;
    c.iTime             = 100;
    c.aiOutput[sim::kSteel] = 1;                            // no aiInput at all
    for ( int step = 0; step < 8; ++step )
    {
        sim::Step( c, 25 );
        CHECK_EQ( BarFixed( c ), BarPreFix( c ) );
    }
    CHECK( c.aiMade[sim::kSteel] > 0 );
}

// ---- the predicate itself ---------------------------------------------------
static void test_predicate_contract() {
    int aiIn[4]  = { 0, 4, 0, 2 };
    int aiHave[4] = { 0, 4, 99, 2 };
    CHECK( !enecon::MatBarStalled( 4, [&]( int i ) { return aiIn[i]; },
                                      [&]( int i ) { return aiHave[i]; } ) );
    aiHave[1] = 3;                                  // one short of a batch
    CHECK( enecon::MatBarStalled( 4, [&]( int i ) { return aiIn[i]; },
                                     [&]( int i ) { return aiHave[i]; } ) );
    aiHave[1] = 4; aiHave[3] = 0;                   // the OTHER input empty
    CHECK( enecon::MatBarStalled( 4, [&]( int i ) { return aiIn[i]; },
                                     [&]( int i ) { return aiHave[i]; } ) );
    // a material the recipe does not use can be empty without stalling anything
    aiHave[3] = 2; aiHave[0] = 0; aiHave[2] = 0;
    CHECK( !enecon::MatBarStalled( 4, [&]( int i ) { return aiIn[i]; },
                                      [&]( int i ) { return aiHave[i]; } ) );
    // nothing to check -> never stalled
    CHECK( !enecon::MatBarStalled( 0, [&]( int i ) { return aiIn[i]; },
                                      [&]( int i ) { return aiHave[i]; } ) );
}

// ---------------------------------------------------------------------------
// (3) Source lint
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

static void test_source_carries_the_gate( const std::string& sq ) {
    CHECK( Has( sq, "#include\"econprod.h\"" ) );
    CHECK( Has( sq, "enecon::MatBarStalled(CMaterialTypes::GetNumTypes()," ) );
    CHECK( Has( sq, "[pBm](intiIn){returnpBm->GetInput(iIn);}" ) );
    CHECK( Has( sq, "[this](intiIn){returnGetStore(iIn);}" ) );
    // the alt-output escape hatch must be part of the gate
    CHECK( Has( sq, "if(!(self->IsFlag(CUnit::alt_oil)&&AltOutput::Available(self)))" ) );
}

static void test_source_has_no_ungated_body( const std::string& sq ) {
    // the pre-fix first line of CMaterialBuilding::GetProductionPer
    CHECK( !Has( sq, "intiTime=GetData()->GetBldMaterials()->GetTime();" ) );
}

int main( int argc, char** argv ) {
    test_starved_bar_does_not_advance();
    test_fed_bar_is_unchanged();
    test_partial_stock_under_one_batch();
    test_starves_only_after_stock_runs_out();
    test_overshoot_batches_unchanged();
    test_alt_output_mode_is_not_gated();
    test_inputless_producer_unaffected();
    test_predicate_contract();

    if ( argc < 2 ) {
        std::printf( "[matbar] no source path given -- lint SKIPPED\n" );
        return microtest::Summary();
    }
    std::string src = ReadAll( argv[1] );
    if ( src.empty() ) {
        std::printf( "[matbar] SKIP: cannot read %s\n", argv[1] );
        return 2;
    }
    std::string sq = NoWs( src );
    std::printf( "[matbar] linting %s (%d bytes)\n", argv[1], (int)src.size() );
    test_source_carries_the_gate( sq );
    test_source_has_no_ungated_body( sq );

    return microtest::Summary();
}
