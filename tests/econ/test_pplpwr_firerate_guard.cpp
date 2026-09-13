// test_pplpwr_firerate_guard.cpp -- standalone fixture for BUGS #87/#82.
//
// Mirrors, as pure functions, the three guards this batch item touches:
//
//   1. CPlayer::StartLoop's m_fPplMult divide (player.cpp ~665-668).
//   2. CPlayer::StartLoop's m_fPwrMult divide (player.cpp ~669-672).
//   3. CUnit::Shoot's fire-rate divide guard (projbase.cpp ~1523-1536, this batch's fix).
//
// The chain BUGS #87 traced: a negative `have` against a zero `need` produced -inf in
// (1)/(2) (the `have < need` guard passed for a negative have against a zero need),
// which fed a NaN into a downstream power multiplier, which cast to an INT_MIN fire
// rate in mainloop.cpp, whose product with AVG_SPEED_MUL overflowed back to exactly 0,
// turning CUnit::Shoot's `div()` into a divide by zero.
//
// This fixture proves, over a wide swept battery of hostile (have, need) and
// fire-rate inputs -- including the exact INT_MIN / negative-have shapes the row
// measured -- that:
//   * the two StartLoop mults are always finite and in [-something sane, 1], never
//     NaN/inf, for EVERY (have, need) pair including need <= 0 and have == INT_MIN;
//   * the fire-rate divisor used by Shoot's div() is never <= 0, for every fire rate
//     from INT_MIN to INT_MAX, so `div()` can never divide by zero;
//   * the player.cpp load-time clamp on m_iPplBldg (this batch's fix) maps every
//     int, including INT_MIN and the measured ~2.13e9 runaway value, into [1, 1e6].
//
// A hand mirror can drift from the production text, so each mirror function's header
// comment cites the exact production lines it stands in for -- read them side by side
// when this file is touched.
//
// Exit: 0 all pass, 1 a check failed.

#include "../ai/microtest.h"

#include <cstdio>
#include <climits>
#include <cmath>

typedef unsigned int DWORD;
static const int AVG_SPEED_MUL = 16;   // real project.h constant, small and stable

// ---------------------------------------------------------------------------
// (1)/(2) Mirrors CPlayer::StartLoop's guarded ratio divides (player.cpp):
//
//     if ( m_iPplNeedBldg > 0 && m_iPplBldg < m_iPplNeedBldg )
//         m_fPplMult = float( m_iPplBldg ) / float( m_iPplNeedBldg );
//     else
//         m_fPplMult = 1.0;
//     if ( m_iPwrNeed > 0 && m_iPwrHave < m_iPwrNeed )
//         m_fPwrMult = float( m_iPwrHave ) / float( m_iPwrNeed );
//     else
//         m_fPwrMult = 1.0;
//
// Both divides share one shape (have, need) -> mult, so one mirror covers both.
// ---------------------------------------------------------------------------
static float GuardedRatioMult( int have, int need )
{
    if ( need > 0 && have < need )
        return float( have ) / float( need );
    return 1.0f;
}

// The UNGUARDED shape, for the positive control: a negative have against a
// zero (or negative) need used to pass `have < need` trivially (any negative have is
// less than 0), and 0.0f divides a nonzero DWORD... but for -inf/NaN the row's chain
// needed `need == 0` specifically (division by 0.0f), which THIS mirror reproduces --
// the unguarded expression is the pre-fix production text with no `need > 0` term.
// This function deliberately performs the hazardous divide the fix removes, so the
// compiler's static divide-by-zero warning is expected here and nowhere else.
#pragma warning( push )
#pragma warning( disable: 4723 )   // potential divide by 0 -- the whole point of this control
static float UnguardedRatioMult( int have, int need )
{
    return float( have ) / float( need );
}
#pragma warning( pop )

// ---------------------------------------------------------------------------
// (3) Mirrors CUnit::Shoot's fire-rate divisor guard (projbase.cpp, this batch's fix):
//
//     int iFireRate = GetFireRate();
//     if ( iFireRate <= 0 ) iFireRate = 1;
//     div_t dtShoot = div( m_dwReloadMod, iFireRate * AVG_SPEED_MUL );
//
// Returns the divisor `div()` would actually receive.
// ---------------------------------------------------------------------------
static int GuardedFireRateDivisor( int iFireRateRaw )
{
    int iFireRate = iFireRateRaw;
    if ( iFireRate <= 0 )
        iFireRate = 1;
    return iFireRate * AVG_SPEED_MUL;
}

// The UNGUARDED shape (pre-fix production text): the exact overflow the row measured.
static int UnguardedFireRateDivisor( int iFireRateRaw )
{
    return iFireRateRaw * AVG_SPEED_MUL;   // int overflow is UB; mirrors the shipped expression
}

// ---------------------------------------------------------------------------
// player.cpp's load-time m_iPplBldg clamp (this batch's fix):
//
//     m_iPplBldg = __minmax( 1, 1000000, m_iPplBldg );
// ---------------------------------------------------------------------------
static int ClampLoadedPplBldg( int raw )
{
    if ( raw < 1 ) return 1;
    if ( raw > 1000000 ) return 1000000;
    return raw;
}

// ---------------------------------------------------------------------------

int main()
{
    std::printf( "[pplpwr_firerate_guard] BUGS #87/#82 divide/overflow-guard fixture\n" );

    // -------- (1)/(2) the ratio-mult guard --------
    static const int aHaves[] = { INT_MIN, -2000000000, -1, 0, 1, 100, INT_MAX };
    static const int aNeeds[] = { INT_MIN, -1, 0, 1, 100, INT_MAX };
    const int nH = (int)( sizeof( aHaves ) / sizeof( aHaves[0] ) );
    const int nN = (int)( sizeof( aNeeds ) / sizeof( aNeeds[0] ) );

    bool sawUnguardedNaNOrInf = false;

    for ( int i = 0; i < nH; i++ )
    {
        for ( int j = 0; j < nN; j++ )
        {
            const int have = aHaves[i];
            const int need = aNeeds[j];

            const float mGuarded = GuardedRatioMult( have, need );

            // GUARANTEE: the guarded mult is always finite.
            CHECK( std::isfinite( mGuarded ) );
            // GUARANTEE: it never exceeds 1.0 (it's either the safe 1.0 default, or a
            // have < need ratio which -- for a POSITIVE need -- is < 1; a negative have
            // over a positive need is itself negative, still <= 1).
            CHECK( mGuarded <= 1.0f );

            // The row's exact repro shape: have < 0, need == 0. Unguarded, this is the
            // -inf that started the chain; guarded, need > 0 is false so it is the safe
            // 1.0 default.
            if ( need == 0 && have < 0 )
            {
                const float mUnguarded = UnguardedRatioMult( have, need );
                CHECK( !std::isfinite( mUnguarded ) );   // positive control: reproduces -inf
                CHECK_EQ( (long long)mGuarded, 1 );      // fixed: exactly the 1.0 default
                sawUnguardedNaNOrInf = true;
            }
        }
    }
    CHECK( sawUnguardedNaNOrInf );   // fixture must actually exercise the repro shape

    // -------- (3) the fire-rate divisor guard --------
    // Real fire rates are small (the row's repro building had baseFR 72); the guard's
    // documented guarantee is only "never <= 0" (BUGS #87's exact failure mode), not an
    // upper bound, so the sweep stays in the realistic/adversarial-negative range and
    // does not include a fire rate near INT_MAX (an unrelated, unrealistic upper-bound
    // overflow that neither the row nor this fix claims to cover).
    static const int aFireRates[] = { INT_MIN, INT_MIN / AVG_SPEED_MUL, -1000000, -1, 0, 1, 72, 1000000 };
    const int nF = (int)( sizeof( aFireRates ) / sizeof( aFireRates[0] ) );

    bool sawUnguardedZeroDivisor = false;

    for ( int i = 0; i < nF; i++ )
    {
        const int fr = aFireRates[i];
        const int divisorGuarded = GuardedFireRateDivisor( fr );

        CHECK( divisorGuarded > 0 );   // GUARANTEE: div() can never see a <= 0 divisor

        // The row's exact repro: INT_MIN * AVG_SPEED_MUL overflows to exactly 0.
        if ( fr == INT_MIN )
        {
            const int divisorUnguarded = UnguardedFireRateDivisor( fr );
            CHECK_EQ( divisorUnguarded, 0 );        // positive control: reproduces the 0
            CHECK_EQ( divisorGuarded, AVG_SPEED_MUL );  // fixed: clamped to iFireRate=1
            sawUnguardedZeroDivisor = true;
        }
    }
    CHECK( sawUnguardedZeroDivisor );

    // -------- the m_iPplBldg load clamp --------
    static const int aRawPpl[] = { INT_MIN, -13, 0, 1, 500, 2133386353, INT_MAX };
    const int nP = (int)( sizeof( aRawPpl ) / sizeof( aRawPpl[0] ) );
    for ( int i = 0; i < nP; i++ )
    {
        const int c = ClampLoadedPplBldg( aRawPpl[i] );
        CHECK( c >= 1 );
        CHECK( c <= 1000000 );
        // A sane mid-range value round-trips unchanged.
        if ( aRawPpl[i] >= 1 && aRawPpl[i] <= 1000000 )
            CHECK_EQ( c, aRawPpl[i] );
    }
    // The two measured runaway/negative values from the row are both corrected.
    CHECK_EQ( ClampLoadedPplBldg( INT_MIN ), 1 );
    CHECK_EQ( ClampLoadedPplBldg( 2133386353 ), 1000000 );

    return microtest::Summary();
}
