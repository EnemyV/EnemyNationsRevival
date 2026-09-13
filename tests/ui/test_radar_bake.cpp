// test_radar_bake.cpp -- pins the minimap / world-map background BAKE CADENCE.
//
// Compiles the PRODUCTION header enations_latest/src/radarbake.h (not a mirror of it),
// so a change to the real decision shows up here. Nothing else is linked: RadarShouldBake
// is pure arithmetic over booleans, a millisecond delta and a frame count.
//
// THE TWO REGRESSIONS THIS EXISTS TO CATCH
// ----------------------------------------
// The bake (a whole-map O(window-pixels) scattered-sample walk, 7.7-13.5ms per call in
// Debug x64) is gated by a wall clock -- 140ms while the user is moving the view, 320ms
// when the view is still -- and by a floor on this window's own render frames.
//
// 1. CONTENT CHURN PICKING THE CADENCE. world.cpp used to promote ANY change of the
//    content signature into "the view moved":
//
//        if ( !bCtrMoved && walkSig != m_qwLastWalkSig )
//            bCtrMoved = true;
//
//    and walkSig carries g_enFogVisGen, which every moving unit in the game bumps as
//    hexes flip fog state (terrain.inl:220-222) -- including AI units, with no human
//    input at all. So the radar sat on the 140ms "user is interacting" cadence
//    permanently: measured rr.bg modal 6/s where the idle design called for 3/s.
//
// 2. A WALL CLOCK THAT STOPS THROTTLING WHERE IT MATTERS. The radar opts out of the
//    per-window render throttle (world.h RendersEveryFrame() -> true), so the clock was
//    its only bound -- and on a 4.5 fps machine a frame is 221ms, which satisfies even
//    the SLOW cadence every single time. Measured there: 79% of frames carried the walk.
//    A frame floor (kRadarBakeMinFrames) caps the walk at 1-in-N frames at any fps; on a
//    33 fps host 3 frames is 90ms, inside both cadences, so the clock still decides.
//
// Guards here:
//   * a TABLE over RadarShouldBake covering both gates;
//   * 24fps and 4.5fps SIMULATIONS reproducing the observed bake rates and frame shares;
//   * a SOURCE LINT over world.cpp/world.h that neither defect has come back.
//
// The table is proved non-vacuous two ways: it is also evaluated against a verbatim
// replication of the PRE-FIX expression (LegacyShouldBake below), which must disagree
// with it -- and running this exe with --baseline evaluates the table against that legacy
// function on purpose, which must FAIL. run-ui-tests.ps1 runs both directions.
//
// Build/run: tests/ui/run-ui-tests.ps1   (no game build, no CMake, no link)

#include "radarbake.h"
#include "../ai/microtest.h"

#include <cstdio>
#include <cstring>
#include <string>

// ---------------------------------------------------------------------------
// The PRE-FIX decision, replicated verbatim from world.cpp:1874-1910 @ 4ca2e4d6.
// Kept so the table can prove it discriminates; never called by production code.
// Note what it does NOT take: a frame count. That was the second defect.
// ---------------------------------------------------------------------------
static bool LegacyShouldBake( bool bCtrMoved, bool bWalkSigChanged, unsigned dwSinceLastBake,
                              bool bNoCache, bool bIsRadar, bool bBldgHit )
{
    // world.cpp:1890-1891 -- "any input delta -> 140ms cadence". Fog churn is not input.
    if ( !bCtrMoved && bWalkSigChanged )
        bCtrMoved = true;
    const unsigned kWalkThrottle = bIsRadar ? ( bCtrMoved ? 140u : 320u )
                                            : ( bCtrMoved ? 140u : 1500u );
    return bNoCache || ( ( dwSinceLastBake >= kWalkThrottle ) &&
                         ( !bIsRadar || bWalkSigChanged || bBldgHit ) );
}

// ---------------------------------------------------------------------------
// 1. The table.
// ---------------------------------------------------------------------------
struct Row {
    const char* name;
    bool        viewMoved;     // VIEW signature differs from the last bake's
    bool        walkChanged;   // content signature differs from the last bake's
    unsigned    since;         // ms since the last completed bake
    unsigned    frames;        // render frames since the last completed bake
    bool        noCache;
    bool        isRadar;
    bool        bldgHit;
    bool        expect;        // what the FIXED decision must return
};

static const Row kTable[] = {
    // ---- radar, view still, content churning (defect 1): the 320ms idle cadence ----
    { "idle+fog @0ms",            false, true,      0,  0, false, true,  false, false },
    { "idle+fog @139ms",          false, true,    139,  4, false, true,  false, false },
    { "idle+fog @200ms",          false, true,    200,  5, false, true,  false, false },
    { "idle+fog @250ms",          false, true,    250,  6, false, true,  false, false },
    { "idle+fog @319ms",          false, true,    319,  7, false, true,  false, false },
    { "idle+fog @320ms",          false, true,    320,  8, false, true,  false, true  },
    { "idle+fog @350ms",          false, true,    350,  8, false, true,  false, true  },

    // ---- radar, view moving: the 140ms cadence, unchanged by the fix ----
    { "view moved @0ms",          true,  true,      0,  0, false, true,  false, false },
    { "view moved @139ms",        true,  true,    139,  3, false, true,  false, false },
    { "view moved @140ms",        true,  true,    140,  4, false, true,  false, true  },
    { "view moved @150ms",        true,  true,    150,  4, false, true,  false, true  },
    // a pan whose bake would draw the same pixels still skips: the cache is already right
    { "view moved, no content",   true,  false,   150,  4, false, true,  false, false },

    // ---- the skip-gate: nothing changed at all, never bake, however long it has been ----
    { "quiet @320ms",             false, false,   320,  8, false, true,  false, false },
    { "quiet @5s",                false, false,  5000, 99, false, true,  false, false },

    // ---- hit flash keeps re-baking, but still respects BOTH gates ----
    { "bldgHit @200ms",           false, false,   200,  5, false, true,  true,  false },
    { "bldgHit @320ms",           false, false,   320,  8, false, true,  true,  true  },
    { "bldgHit @320ms, 2 frames", false, false,   320,  2, false, true,  true,  false },

    // ---- no cached background yet: bake regardless of clock, frames or signatures ----
    { "no cache @0ms",            false, false,     0,  0, true,  true,  false, true  },
    { "no cache, quiet",          false, false,     0,  0, true,  true,  true,  true  },

    // ---- defect 2: a 4.5fps VM, 222ms a frame. The clock alone lets EVERY frame walk;
    //      the frame floor holds it to 1-in-3, in BOTH cadences ----
    { "vm idle frame 1 @222ms",   false, true,    222,  1, false, true,  false, false },
    { "vm idle frame 2 @444ms",   false, true,    444,  2, false, true,  false, false },
    { "vm idle frame 3 @666ms",   false, true,    666,  3, false, true,  false, true  },
    { "vm drag frame 1 @222ms",   true,  true,    222,  1, false, true,  false, false },
    { "vm drag frame 2 @444ms",   true,  true,    444,  2, false, true,  false, false },
    { "vm drag frame 3 @666ms",   true,  true,    666,  3, false, true,  false, true  },

    // ---- a 33fps host, 30ms a frame: 3 frames is 90ms, inside both cadences, so the
    //      WALL CLOCK is what decides and host behaviour is unchanged ----
    { "host drag frame 1 @30ms",  true,  true,     30,  1, false, true,  false, false },
    { "host drag frame 3 @90ms",  true,  true,     90,  3, false, true,  false, false },
    { "host drag frame 5 @151ms", true,  true,    151,  5, false, true,  false, true  },
    { "host idle frame 8 @242ms", false, true,    242,  8, false, true,  false, false },
    { "host idle frame 11 @333",  false, true,    333, 11, false, true,  false, true  },

    // ---- world map: no live dots, so its cached blit IS the frame; it bakes on the
    //      timer alone, at the 1500ms idle cadence and 140ms while panning, and is
    //      EXEMPT from the frame floor (MinRenderIntervalMs already caps it at ~3/s) ----
    { "world idle+fog @800ms",    false, true,    800,  2, false, false, false, false },
    { "world idle+fog @1499ms",   false, true,   1499,  4, false, false, false, false },
    { "world idle+fog @1500ms",   false, true,   1500,  5, false, false, false, true  },
    { "world quiet @1500ms",      false, false,  1500,  5, false, false, false, true  },
    { "world moved @139ms",       true,  false,   139,  1, false, false, false, false },
    { "world moved @140ms, 1 fr", true,  false,   140,  1, false, false, false, true  },
};
static const int kRows = (int)( sizeof( kTable ) / sizeof( kTable[0] ) );

static bool RunRow( const Row& r, bool bLegacy )
{
    const unsigned idle  = r.isRadar ? kRadarBakeIdleMs    : kWorldMapBakeIdleMs;
    const unsigned minFr = r.isRadar ? kRadarBakeMinFrames : kWorldMapBakeMinFrames;
    return bLegacy
        ? LegacyShouldBake( r.viewMoved, r.walkChanged, r.since, r.noCache, r.isRadar, r.bldgHit )
        : RadarShouldBake( r.viewMoved, r.walkChanged, r.since, r.frames,
                           kRadarBakeMovingMs, idle, minFr, r.noCache, r.isRadar, r.bldgHit );
}

// Returns the number of rows on which the pre-fix decision disagrees with the table.
static int TestTable( bool bLegacy )
{
    int disagree = 0;
    for ( int i = 0; i < kRows; i++ ) {
        const Row& r = kTable[i];
        const bool got = RunRow( r, bLegacy );
        ++microtest::Checks();
        if ( got != r.expect ) {
            ++microtest::Fails();
            std::printf( "FAIL %s:%d  row \"%s\": got %s, want %s%s\n",
                         __FILE__, __LINE__, r.name, got ? "bake" : "skip",
                         r.expect ? "bake" : "skip", bLegacy ? "  (--baseline)" : "" );
        }
        if ( RunRow( r, true ) != r.expect )
            disagree++;
    }
    return disagree;
}

// The table must actually separate the fixed decision from the pre-fix one, or it is
// pinning nothing.
static void TestTableDiscriminates( int disagree )
{
    CHECK( disagree >= 8 );
    // Name the discriminating cases explicitly so a future edit cannot quietly drop them.
    // Defect 1: fog churn below the idle cadence.
    CHECK( !RadarShouldBake( false, true, 200, 9, kRadarBakeMovingMs, kRadarBakeIdleMs,
                             kRadarBakeMinFrames, false, true, false ) );
    CHECK(  LegacyShouldBake( false, true, 200, false, true, false ) );
    CHECK( !RadarShouldBake( false, true, 319, 9, kRadarBakeMovingMs, kRadarBakeIdleMs,
                             kRadarBakeMinFrames, false, true, false ) );
    CHECK(  LegacyShouldBake( false, true, 319, false, true, false ) );
    // Defect 2: a slow machine, where every frame clears the clock on its own.
    CHECK( !RadarShouldBake( true, true, 222, 1, kRadarBakeMovingMs, kRadarBakeIdleMs,
                             kRadarBakeMinFrames, false, true, false ) );
    CHECK(  LegacyShouldBake( true, true, 222, false, true, false ) );

    // ...and with the frame floor satisfied, the clock behaves exactly as it always did,
    // at every elapsed time, for a genuinely moving view.
    for ( unsigned t = 0; t <= 400; t++ ) {
        CHECK_EQ( RadarShouldBake( true, true, t, 99, kRadarBakeMovingMs, kRadarBakeIdleMs,
                                   kRadarBakeMinFrames, false, true, false ),
                  LegacyShouldBake( true, true, t, false, true, false ) );
    }
}

// ---------------------------------------------------------------------------
// 2. Simulations -- reproduce the measured bake RATES (rr.bg) and frame shares.
// ---------------------------------------------------------------------------
// One radar ReRender per frame (the radar renders every frame by design); fog churn
// every frame (700-800 moving vehicles); the view either dragging or still.
// `frameMs10` is the frame period in TENTHS of a millisecond, so 4.5fps (222.2ms) is
// expressible exactly enough: 24fps -> 4167, 33fps -> 3030, 4.5fps -> 2222.
struct Sim { int bakes; int frames; };

static Sim Simulate( int frames, int frameMs10, bool bViewMovingEveryFrame, bool bLegacy )
{
    Sim      s           = { 0, frames };
    unsigned lastBake    = 0;
    unsigned framesSince = 0;
    for ( int i = 1; i <= frames; i++ ) {
        const unsigned t = (unsigned)( (long long)i * frameMs10 / 10 );
        framesSince++;
        const bool go = bLegacy
            ? LegacyShouldBake( bViewMovingEveryFrame, true, t - lastBake, false, true, false )
            : RadarShouldBake( bViewMovingEveryFrame, true, t - lastBake, framesSince,
                               kRadarBakeMovingMs, kRadarBakeIdleMs, kRadarBakeMinFrames,
                               false, true, false );
        if ( go ) { lastBake = t; framesSince = 0; s.bakes++; }
    }
    return s;
}

// 24fps host: the rates WinOpus measured in the two Debug x64 captures.
static void TestObservedRates()
{
    const int kFrames24 = 240, kPeriod24 = 417;   // 10 s at 24fps, 41.7ms a frame

    // Pre-fix, view STILL, AI units churning fog: 140ms cadence => the measured modal
    // rr.bg of 6/s (271 of 495 and 299 of 595 intervals).
    const Sim legacyIdle = Simulate( kFrames24, kPeriod24, false, true );
    CHECK( legacyIdle.bakes >= 57 && legacyIdle.bakes <= 62 );

    // Post-fix, same conditions: 320ms cadence => rr.bg 3/s, the documented idle design.
    const Sim fixedIdle = Simulate( kFrames24, kPeriod24, false, false );
    CHECK( fixedIdle.bakes >= 28 && fixedIdle.bakes <= 32 );

    // ~2x fewer whole-map walks, which at ~7.7ms per bake is the ~23ms/s the fix recovers.
    CHECK( legacyIdle.bakes >= fixedIdle.bakes * 19 / 10 );

    // PRESERVATION: while the view is dragging at 24fps the fixed decision is still 6-7/s,
    // identical to pre-fix -- the 140ms cadence is 3.4 frames, so the frame floor does not
    // bind here. If this drops, either the view signature is missing a term or the frame
    // floor has been raised too far.
    const Sim fixedDrag  = Simulate( kFrames24, kPeriod24, true, false );
    const Sim legacyDrag = Simulate( kFrames24, kPeriod24, true, true );
    CHECK_EQ( fixedDrag.bakes, legacyDrag.bakes );
    CHECK( fixedDrag.bakes >= 57 && fixedDrag.bakes <= 62 );
}

// 4.5fps VM: 222ms a frame, so the wall clock alone permits a walk on EVERY frame and
// the throttle has effectively disengaged. This is the case the frame floor is for.
static void TestSlowMachineFrameShare()
{
    const int kFramesVm = 45, kPeriodVm = 2222;   // 10 s at 4.5fps, 222.2ms a frame

    const Sim legacyIdle = Simulate( kFramesVm, kPeriodVm, false, true );
    const Sim fixedIdle  = Simulate( kFramesVm, kPeriodVm, false, false );
    const Sim fixedDrag  = Simulate( kFramesVm, kPeriodVm, true,  false );

    // Pre-fix: a walk on essentially every frame (the VM capture measured 79%).
    CHECK( legacyIdle.bakes * 100 / legacyIdle.frames >= 95 );
    // Post-fix: capped at 1-in-kRadarBakeMinFrames, i.e. at most a third of frames...
    CHECK( fixedIdle.bakes * 100 / fixedIdle.frames <= 34 );
    CHECK( fixedIdle.bakes * 100 / fixedIdle.frames >= 28 );
    // ...and the cap holds while DRAGGING too: the floor applies to both cadences.
    CHECK( fixedDrag.bakes * 100 / fixedDrag.frames <= 34 );

    // 33fps host: 3 frames is 90ms, inside both cadences, so the frame floor never binds
    // and the fixed decision is exactly the clock -- host behaviour is unchanged except
    // for the fog-signature fix. Dragging still reaches the full 140ms cadence.
    const int kFrames33 = 330, kPeriod33 = 303;   // 10 s at 33fps, 30.3ms a frame
    const Sim hostDrag       = Simulate( kFrames33, kPeriod33, true, false );
    const Sim hostDragLegacy = Simulate( kFrames33, kPeriod33, true, true );
    CHECK_EQ( hostDrag.bakes, hostDragLegacy.bakes );
    CHECK( hostDrag.bakes >= 66 && hostDrag.bakes <= 72 );        // ~7/s = the 140ms cadence
    const Sim hostIdle = Simulate( kFrames33, kPeriod33, false, false );
    CHECK( hostIdle.bakes >= 28 && hostIdle.bakes <= 32 );        // ~3/s = the 320ms cadence
}

// Drag, then let go: the cadence must return to idle, not stay latched fast.
static void TestMovedThenIdleReturnsToIdle()
{
    const int dragMs = 2000, idleMs = 4000;
    unsigned  lastBake = 0, framesSince = 0;
    int       dragBakes = 0, idleBakes = 0;
    const int frames = ( dragMs + idleMs ) * 24 / 1000;
    for ( int i = 1; i <= frames; i++ ) {
        const unsigned t = (unsigned)( 1000LL * i / 24 );
        const bool moving = ( (int)t < dragMs );
        framesSince++;
        if ( RadarShouldBake( moving, true, t - lastBake, framesSince, kRadarBakeMovingMs,
                              kRadarBakeIdleMs, kRadarBakeMinFrames, false, true, false ) ) {
            lastBake = t; framesSince = 0;
            if ( moving ) dragBakes++; else idleBakes++;
        }
    }
    CHECK( dragBakes >= 11 && dragBakes <= 13 );      // 2 s at ~6/s
    CHECK( idleBakes >= 11 && idleBakes <= 13 );      // 4 s at ~3/s
    // ...and the rate really did halve within the one run: no latching.
    CHECK( dragBakes * 1000 / dragMs > idleBakes * 1000 / idleMs );
}

// ---------------------------------------------------------------------------
// 3. Source lint over the production call site.
// ---------------------------------------------------------------------------
static std::string Slurp( const std::string& path )
{
    std::FILE* f = std::fopen( path.c_str(), "rb" );
    if ( !f ) return std::string();
    std::string out;
    char        buf[65536];
    size_t      n;
    while ( ( n = std::fread( buf, 1, sizeof buf, f ) ) > 0 ) out.append( buf, n );
    std::fclose( f );
    return out;
}

static int CountOf( const std::string& hay, const char* needle )
{
    int n = 0;
    for ( size_t p = hay.find( needle ); p != std::string::npos;
          p = hay.find( needle, p + 1 ) ) n++;
    return n;
}

// The LONGEST `<lhs> = ...;` in `body` -- i.e. the real assignment, not the `= 0`
// declaration that precedes it. Returns the expression text, without the semicolon.
static std::string AssignExpr( const std::string& body, const char* lhs )
{
    static const char* kWs = " \t\r\n";
    const size_t len = std::strlen( lhs );
    std::string  best;
    for ( size_t p = body.find( lhs ); p != std::string::npos; p = body.find( lhs, p + 1 ) ) {
        const size_t q = body.find_first_not_of( kWs, p + len );
        if ( q == std::string::npos || q + 1 >= body.size() ) continue;
        if ( body[q] != '=' || body[q + 1] == '=' ) continue;        // skip ==, !=, >=, <=
        const size_t e = body.find( ';', q );
        if ( e == std::string::npos ) continue;
        const std::string expr = body.substr( q + 1, e - q - 1 );
        if ( expr.size() > best.size() ) best = expr;
    }
    return best;
}

static void TestSourceLint( const std::string& srcRoot )
{
    const std::string world  = Slurp( srcRoot + "/world.cpp" );
    const std::string worldH = Slurp( srcRoot + "/world.h" );
    const std::string gate   = Slurp( srcRoot + "/radarbake.h" );
    CHECK( world.size()  > 10000 );   // the lint must not pass by reading nothing
    CHECK( worldH.size() > 1000 );
    CHECK( gate.size()   > 500 );
    if ( world.empty() || worldH.empty() || gate.empty() ) return;

    // Isolate CWndWorld::ReRender: from its definition to the next brace in column 1.
    const size_t fn = world.find( "void CWndWorld::ReRender" );
    CHECK( fn != std::string::npos );
    if ( fn == std::string::npos ) return;
    const size_t end = world.find( "\n}", fn );
    CHECK( end != std::string::npos );
    if ( end == std::string::npos ) return;
    const std::string body = world.substr( fn, end - fn );
    CHECK( body.find( "rr.radar" ) != std::string::npos );   // we grabbed the right function

    // (a) THE REGRESSION LINE. No promotion of anything into "the view moved" survives.
    CHECK_EQ( CountOf( body, "bCtrMoved = true" ), 0 );
    CHECK_EQ( CountOf( body, "bCtrMoved  = true" ), 0 );
    CHECK_EQ( CountOf( body, "bCtrMoved = TRUE" ), 0 );
    // Exactly two writes to bCtrMoved in the whole function: its declaration, and the one
    // view-signature comparison. Any third is a promotion by another name.
    CHECK_EQ( CountOf( body, "bCtrMoved =" ), 2 );
    CHECK( body.find( "bool bCtrMoved = false" ) != std::string::npos );

    // (b) that single comparison is derived from the VIEW signature and nothing else.
    const std::string ctrReal = AssignExpr( body, "bCtrMoved" );
    CHECK( ctrReal.find( "viewSig" )         != std::string::npos );
    CHECK( ctrReal.find( "m_qwLastViewSig" ) != std::string::npos );
    CHECK( ctrReal.find( "walkSig" )         == std::string::npos );
    CHECK( ctrReal.find( "g_enFogVisGen" )   == std::string::npos );

    // (c) the VIEW signature must contain the view terms and NONE of the content terms.
    const std::string viewExpr = AssignExpr( body, "viewSig" );
    CHECK( viewExpr.size() > 40 );
    CHECK( viewExpr.find( "ctr.x" )   != std::string::npos );   // pan / scroll-follow
    CHECK( viewExpr.find( "ctr.y" )   != std::string::npos );
    CHECK( viewExpr.find( "m_iZoom" ) != std::string::npos );   // zoom = viewbox size
    CHECK( viewExpr.find( "m_iDir" )  != std::string::npos );   // rotation
    CHECK( viewExpr.find( "m_iMode" ) != std::string::npos );   // mode buttons (human input)
    // The whole point of the fix: content churn is NOT a view change.
    CHECK( viewExpr.find( "g_enFogVisGen" )      == std::string::npos );
    CHECK( viewExpr.find( "g_enTerrainEditGen" ) == std::string::npos );
    CHECK( viewExpr.find( "theBuildingMap" )     == std::string::npos );
    CHECK( viewExpr.find( "m_iResOn" )           == std::string::npos );

    // (d) ...and the WALK signature (the skip-gate) still carries all of them.
    const std::string walkExpr = AssignExpr( body, "walkSig" );
    CHECK( walkExpr.find( "g_enFogVisGen" )      != std::string::npos );
    CHECK( walkExpr.find( "g_enTerrainEditGen" ) != std::string::npos );
    CHECK( walkExpr.find( "theBuildingMap" )     != std::string::npos );
    CHECK( walkExpr.find( "m_iResOn" )           != std::string::npos );
    CHECK( walkExpr.find( "viewSig" )            != std::string::npos );   // view terms folded in
    CHECK( body.find( "walkSig != m_qwLastWalkSig" ) != std::string::npos );  // still the gate

    // (e) the decision is the shared pure helper, not a re-inlined copy.
    CHECK( world.find( "#include \"radarbake.h\"" ) != std::string::npos );
    CHECK_EQ( CountOf( body, "RadarShouldBake(" ), 1 );
    CHECK_EQ( CountOf( body, "kWalkThrottle" ), 0 );              // old inline cadence gone
    CHECK( gate.find( "inline bool RadarShouldBake(" ) != std::string::npos );
    CHECK_EQ( CountOf( gate, "#include" ), 0 );   // helper stays dependency-free / testable

    // (f) the new bake state is declared and stamped at bake time, or the cadence never
    //     resets and the radar bakes on every frame instead.
    CHECK( worldH.find( "m_qwLastViewSig" ) != std::string::npos );
    CHECK_EQ( CountOf( body, "m_qwLastViewSig   = viewSig" ), 1 );
    CHECK_EQ( CountOf( body, "m_qwLastWalkSig   = walkSig" ), 1 );

    // (g) the FRAME floor: a counter advanced once per render and cleared on the bake,
    //     and the radar cadence actually passing the floor constant.
    CHECK( gate.find( "kRadarBakeMinFrames" )    != std::string::npos );
    CHECK( gate.find( "kWorldMapBakeMinFrames" ) != std::string::npos );
    CHECK( worldH.find( "m_uFramesSinceBake" )   != std::string::npos );
    CHECK_EQ( CountOf( body, "m_uFramesSinceBake++" ), 1 );
    CHECK_EQ( CountOf( body, "m_uFramesSinceBake = 0" ), 1 );
    CHECK( body.find( "kRadarBakeMinFrames" ) != std::string::npos );

    // (h) the render-rate levers the investigation ruled out must stay untouched:
    //     rate-limiting ReRender itself stretches the hit flash, which world.cpp:2411-2414
    //     decays once per radar render. The frame floor skips the WALK, not the render.
    CHECK( worldH.find( "RendersEveryFrame () const override { return m_bIsRadar != FALSE; }" )
           != std::string::npos );
    CHECK( worldH.find( "MinRenderIntervalMs () const override { return m_bIsRadar ? 0 : 333; }" )
           != std::string::npos );
    CHECK( body.find( "GetFramesElapsed" ) != std::string::npos );
}

// ---------------------------------------------------------------------------

int main( int argc, char** argv )
{
    bool        baseline = false;
    std::string srcRoot;
    for ( int i = 1; i < argc; i++ ) {
        if ( std::strcmp( argv[i], "--baseline" ) == 0 )           baseline = true;
        else if ( std::strncmp( argv[i], "--src=", 6 ) == 0 )      srcRoot  = argv[i] + 6;
    }

    const int disagree = TestTable( baseline );

    if ( baseline ) {
        // Evaluated against the PRE-FIX decision on purpose. The table must reject it;
        // a zero exit code here would mean the table pins nothing.
        std::printf( "[ui_radar_bake] --baseline: %d of %d rows disagree with pre-fix\n",
                     disagree, kRows );
        const int rc = microtest::Summary();
        std::printf( "[ui_radar_bake] baseline %s (a FAIL here is the expected result)\n",
                     rc == 0 ? "PASS" : "FAIL" );
        return rc;
    }

    TestTableDiscriminates( disagree );
    TestObservedRates();
    TestSlowMachineFrameShare();
    TestMovedThenIdleReturnsToIdle();

    CHECK( !srcRoot.empty() );   // --src=<enations_latest/src> is mandatory: no silent skip
    if ( !srcRoot.empty() ) TestSourceLint( srcRoot );

    const int rc = microtest::Summary();
    std::printf( "[ui_radar_bake] %s\n", rc == 0 ? "PASS" : "FAIL" );
    return rc;
}
