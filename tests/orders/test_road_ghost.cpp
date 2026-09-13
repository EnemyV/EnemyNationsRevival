// test_road_ghost.cpp -- queue-road ghost's step rule, driven by the PRODUCTION body.
//
// run-road-ghost.py extracts CVehicle::RoadStepToward verbatim out of
// enations_latest/src/vehicle.cpp (tests/orders' extraction technique --
// run-order-lifecycle.py/run-drag-place.py) into roadghost_actual.inc and compiles it
// against the CHexCoord stand-in below -- terrain.inl's torus arithmetic, drag-place's
// technique: a power-of-two torus, mask = eX-1, half = eX/2.
//
// So a check here is a statement about the shipped step rule, not a mirror of it.
// ShadowStep below IS a hand-typed mirror: the body _NextRoadHex had BEFORE the
// road-ghost extraction pulled it out to RoadStepToward, kept so every case below is
// ALSO a differential check that the extraction changed nothing -- RoadStepToward and
// ShadowStep must agree at every hex of every walk.
//
// What this does NOT cover: the road-ghost draw itself (SDL2Terrain.cpp), which needs
// SDL/CAnimAtr/the live map and cannot be linked standalone -- this suite is about the
// one thing that can drift silently, the shared step arithmetic.

#include <cstdio>
#include <cstdlib>
#include <set>
#include <vector>

#include "../ai/microtest.h"

using BOOL = int;
static const BOOL TRUE = 1, FALSE = 0;

// --------------------------------------------------------------------------- the torus
// terrain.inl: CHexCoord::Xinc/Xdec/Yinc/Ydec mask against theMap.m_iHexMask, Diff
// against theMap.m_iWidthHalf/m_iHexMask -- ONE mask for both axes (wrldinit.cpp: a
// square power-of-two map). 256 keeps the seam arithmetic small enough to hand-check.
struct World
{
    int mask = 255;
    int half = 128;
} g_world;

struct CHexCoord
{
    int m_iX, m_iY;
    CHexCoord( ): m_iX( 0 ), m_iY( 0 ) {}
    CHexCoord( int x, int y ): m_iX( x ), m_iY( y ) {}
    int X( ) const { return m_iX; }
    int Y( ) const { return m_iY; }
    // terrain.inl, verbatim arithmetic
    void Xinc( ) { m_iX++; m_iX &= g_world.mask; }
    void Yinc( ) { m_iY++; m_iY &= g_world.mask; }
    void Xdec( ) { m_iX--; m_iX &= g_world.mask; }
    void Ydec( ) { m_iY--; m_iY &= g_world.mask; }
    static int Diff( int iVal )
    {
        iVal += g_world.half;
        iVal &= g_world.mask;
        iVal -= g_world.half;
        return ( iVal );
    }
    BOOL operator==( CHexCoord const& o ) const { return ( ( m_iX == o.m_iX ) && ( m_iY == o.m_iY ) ); }
    BOOL operator!=( CHexCoord const& o ) const { return ( !( *this == o ) ); }
};

// declared so "CHexCoord CVehicle::RoadStepToward(...) { ... }" below is a definition
// of a previously-declared member, exactly like the shipped header (vehicle.h).
struct CVehicle
{
    static CHexCoord RoadStepToward( CHexCoord const& hexFrom, CHexCoord const& hexEnd );
};

// the PRODUCTION body (roadghost_actual.inc, written by run-road-ghost.py)
#include "roadghost_actual.inc"

// the pre-extraction body, hand-copied here as the differential control: this is what
// _NextRoadHex's inline arithmetic was BEFORE the extraction (vehicle.cpp, read
// `_hexOn`/`m_hexEnd` as `hexFrom`/`hexEnd`).
static CHexCoord ShadowStep( CHexCoord const& hexFrom, CHexCoord const& hexEnd )
{
    if ( hexFrom == hexEnd )
        return ( hexFrom );

    int       x = CHexCoord::Diff( hexEnd.X( ) - hexFrom.X( ) );
    int       y = CHexCoord::Diff( hexEnd.Y( ) - hexFrom.Y( ) );
    CHexCoord hex( hexFrom );
    if ( abs( x ) >= abs( y ) )
    {
        if ( x > 0 ) hex.Xinc( ); else hex.Xdec( );
    }
    else
    {
        if ( y > 0 ) hex.Yinc( ); else hex.Ydec( );
    }
    return ( hex );
}

static const int kMaxChain = 256;   // matches SDL2Terrain.cpp's kMaxRoadGhostHexes

// Walks RoadStepToward and ShadowStep in lockstep from hexFrom to hexEnd, checking:
//   - the chain reaches hexEnd
//   - no hex is visited twice
//   - RoadStepToward agrees with ShadowStep at every step (the extraction is a no-op)
//   - the length matches |Diff(dx)| + |Diff(dy)| + 1 -- exactly one axis moves one hex
//     closer per step, so the walk's length is the Manhattan distance to the target,
//     regardless of which axis is chosen at a tie (an independent formula check, not
//     just "the two implementations agree with each other").
static void CheckChain( char const* name, CHexCoord hexFrom, CHexCoord hexEnd )
{
    const int expectedLen =
        abs( CHexCoord::Diff( hexEnd.X( ) - hexFrom.X( ) ) ) +
        abs( CHexCoord::Diff( hexEnd.Y( ) - hexFrom.Y( ) ) ) + 1;

    std::vector<CHexCoord> chain;
    std::set<long long>    seen;
    CHexCoord              hex = hexFrom;
    bool                   reached = false, revisit = false, agree = true;

    for ( int i = 0; i < kMaxChain; ++i )
    {
        const long long key = ( (long long)hex.X( ) << 32 ) | (unsigned)hex.Y( );
        if ( seen.count( key ) ) { revisit = true; break; }
        seen.insert( key );
        chain.push_back( hex );
        if ( hex == hexEnd ) { reached = true; break; }

        const CHexCoord viaShared = CVehicle::RoadStepToward( hex, hexEnd );
        const CHexCoord viaShadow = ShadowStep( hex, hexEnd );
        if ( viaShared != viaShadow )
            agree = false;
        hex = viaShared;
    }

    std::printf( "-- %s: (%d,%d) -> (%d,%d), chain len %d (expected %d)\n", name,
                hexFrom.X( ), hexFrom.Y( ), hexEnd.X( ), hexEnd.Y( ), (int)chain.size( ), expectedLen );
    CHECK( reached );
    CHECK( !revisit );
    CHECK( agree );
    CHECK_EQ( (int)chain.size( ), expectedLen );
}

int main( )
{
    const CHexCoord c( 128, 128 );   // map centre, well clear of the seam

    // the eight compass directions, 5 hexes out
    struct
    {
        char const* name;
        int         dx, dy;
    } compass[] = {
        { "N", 0, -5 }, { "NE", 5, -5 }, { "E", 5, 0 }, { "SE", 5, 5 },
        { "S", 0, 5 },  { "SW", -5, 5 }, { "W", -5, 0 }, { "NW", -5, -5 },
    };
    for ( auto& d : compass )
        CheckChain( d.name, c, CHexCoord( c.X( ) + d.dx, c.Y( ) + d.dy ) );

    // two ASYMMETRIC diagonals (|dx| != |dy|) -- exercise the "move on the currently
    // longer axis" branch flipping mid-walk, not just a steady 45-degree run
    CheckChain( "diag-9x4", c, CHexCoord( c.X( ) + 9, c.Y( ) + 4 ) );
    CheckChain( "diag-3x11", c, CHexCoord( c.X( ) - 3, c.Y( ) - 11 ) );

    // wraps across the seam: world is 256 wide/tall (mask 255) -- the SHORT way from
    // near the high edge to near the low edge is across the wrap, not the long way round
    CheckChain( "seam-x", CHexCoord( 254, 128 ), CHexCoord( 2, 128 ) );
    CheckChain( "seam-y", CHexCoord( 128, 254 ), CHexCoord( 128, 2 ) );
    CheckChain( "seam-diag", CHexCoord( 254, 254 ), CHexCoord( 3, 1 ) );

    // zero-length: already at the destination
    CheckChain( "zero-length", c, c );

    return microtest::Summary( );
}
