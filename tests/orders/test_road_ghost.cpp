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

static const int kMaxChain = 256;   // matches SDL2Terrain.cpp's pre-fix kMaxRoadGhostHexes

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

// ----------------------------------------------------------------- the drawing body
// AppendRoadGhostLine (SDL2Terrain.cpp) is the routine WinAstra's review found two
// omissions in: a lone unbuilt endpoint drew zero vertices, and a fixed 256-hex cap
// truncated a legitimate long chain. Drives the PRODUCTION body (roadghost_draw_actual.inc,
// written by run-road-ghost.py) rather than a mirror, the same way the step-rule checks
// above drive RoadStepToward -- so a regression in the shipped drawing routine, not just
// its formula, fails here. Needs stand-ins for the SDL/map/CAnimAtr types the real body
// touches; none of these model real rendering, only enough shape to link and run.
struct CPoint
{
    int x, y;
    CPoint( int a = 0, int b = 0 ): x( a ), y( b ) {}
};
struct SDL_Color { unsigned char r, g, b, a; };
struct SDL_FPoint { float x, y; };
struct SDL_Vertex { SDL_FPoint position; SDL_Color color; SDL_FPoint tex_coord; };

// GetType()/road mirror terrain.h's CHex just enough for the "already built" prefix
// scan; the real enum's road value doesn't matter here, only that it's distinguishable.
struct CHex
{
    enum { road = 1 };
    int type = 0;
    int GetType( ) const { return ( type ); }
};

// theMap stand-in: a plain 256x256 grid (same world size as g_world above), plus the
// Get_eX()/Get_eY() accessors RoadGhostCap() (post-fix) reads to size its bound.
struct CTestMap
{
    CHex h[256][256];
    CHex* GetHex( CHexCoord const& c ) { return ( &h[c.X( )][c.Y( )] ); }
    int   Get_eX( ) const { return ( 256 ); }
    int   Get_eY( ) const { return ( 256 ); }
} theMap;

// CAnimAtr stand-in: projects a hex to 4 corner points 10 units apart, so consecutive
// hex centres in AppendRoadGhostLine's line are a fixed 10 units apart -- enough to
// clear its `len >= 0.5f` degenerate-segment guard without modelling real projection.
struct CAnimAtr
{
    BOOL MapToWindowHex( CHexCoord const& c, CPoint p[4] ) const
    {
        p[0] = CPoint( c.X( ) * 10, c.Y( ) * 10 );
        p[1] = CPoint( c.X( ) * 10 + 10, c.Y( ) * 10 );
        p[2] = CPoint( c.X( ) * 10 + 10, c.Y( ) * 10 + 10 );
        p[3] = CPoint( c.X( ) * 10, c.Y( ) * 10 + 10 );
        return ( TRUE );
    }
};
static CPoint FootprintSeamShift( CAnimAtr const&, CHexCoord, bool ) { return ( CPoint( ) ); }

// the ghost line colour AppendRoadGhostLine paints every vertex with (SDL2Terrain.cpp);
// the actual colour is irrelevant here, only that the symbol exists to assign.
static const SDL_Color kQueuedGhostCol = { 120, 190, 255, 120 };

// the PRODUCTION drawing body (roadghost_draw_actual.inc, written by run-road-ghost.py):
// RoadGhostCap() (post-fix) or the flat kMaxRoadGhostHexes constant (pre-fix, read via
// --baseline-ref) plus AppendRoadGhostLine itself.
#include "roadghost_draw_actual.inc"

// Resets the shared scratch hexes this call's chain touches back to unbuilt, so tests
// don't leak state into each other through the disjoint coordinate ranges they use.
static void ClearRoad( CHexCoord const& hexFrom, CHexCoord const& hexEnd )
{
    CHexCoord hex = hexFrom;
    for ( int i = 0; i < 512; ++i )
    {
        theMap.GetHex( hex )->type = 0;
        if ( hex == hexEnd )
            break;
        hex = CVehicle::RoadStepToward( hex, hexEnd );
    }
}

// #1: the LAST unbuilt hex of a multi-hex road must still draw -- the segment INTO
// it, anchored on the last already-built hex, matching what the crane physically lays.
// Pre-fix: `n - iStart < 2` returned with zero vertices the moment only the endpoint
// was left, so a real pending order looked visually identical to a finished one.
static void CheckLastHexDraws( )
{
    const CHexCoord hexFrom( 200, 200 ), hexEnd( 202, 200 );
    ClearRoad( hexFrom, hexEnd );
    theMap.GetHex( CHexCoord( 200, 200 ) )->type = CHex::road;
    theMap.GetHex( CHexCoord( 201, 200 ) )->type = CHex::road;   // only (202,200) unbuilt

    CAnimAtr                 aa;
    std::vector<SDL_Vertex>  verts;
    AppendRoadGhostLine( aa, hexFrom, hexEnd, verts );

    std::printf( "-- last-hex-draws: verts=%zu (want 6)\n", verts.size( ) );
    CHECK( !verts.empty( ) );        // the vanished-endpoint defect
    CHECK_EQ( (long long)verts.size( ), 6 );   // exactly the one segment into the last hex

    ClearRoad( hexFrom, hexEnd );
}

// #2: a chain longer than the old flat 256-hex cap must still draw in full. On this
// 256x256 stand-in world, (0,0) -> (128,128) is a 257-hex chain (matches WinAstra's
// witness) -- one more than the pre-fix cap, which silently dropped the tail.
static void CheckLongChainDraws( )
{
    const CHexCoord hexFrom( 0, 0 ), hexEnd( 128, 128 );
    const int        expectedLen =
        abs( CHexCoord::Diff( hexEnd.X( ) - hexFrom.X( ) ) ) +
        abs( CHexCoord::Diff( hexEnd.Y( ) - hexFrom.Y( ) ) ) + 1;

    CAnimAtr                aa;
    std::vector<SDL_Vertex> verts;
    AppendRoadGhostLine( aa, hexFrom, hexEnd, verts );

    const long long expectedVerts = (long long)( expectedLen - 1 ) * 6;   // one 6-vertex quad per segment
    std::printf( "-- long-chain-draws: chain len %d, verts=%zu (want %lld)\n",
                expectedLen, verts.size( ), expectedVerts );
    CHECK_EQ( (long long)verts.size( ), expectedVerts );   // pre-fix: capped short of the real end
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

    CheckLastHexDraws( );
    CheckLongChainDraws( );

    return microtest::Summary( );
}
