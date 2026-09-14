// test_radar_bake.cpp -- pins the minimap / world-map background BAKE CADENCE.
//
// Compiles the PRODUCTION header enations_latest/src/radarbake.h (not a mirror of it),
// so a change to the real decision shows up here. Nothing else is linked: RadarShouldBake
// is pure arithmetic over booleans, a millisecond delta and a frame count.
//
// THE REGRESSION THIS EXISTS TO CATCH
// -----------------------------------
// CWndWorld::ReRender caches the unit-free minimap background and re-walks the map on one
// of two cadences: 140ms while the user is moving the view, 320ms when the view is still.
// It used to promote ANY change of the content signature into "the view moved":
//
//     if ( !bCtrMoved && walkSig != m_qwLastWalkSig )
//         bCtrMoved = true;
//
// and that signature carries g_enFogVisGen, bumped whenever a hex flips fog state
// (CHex::IncVisible / DecVisible, terrain.inl:220-222). That happens continuously as the
// LOCAL player's own units move -- spotting only runs for owners where IsMe() holds
// (unit.cpp:1076) -- with no click, key or camera movement involved. So the radar sat on
// the 140ms "user is interacting" cadence permanently: measured rr.bg modal 6/s where the
// idle design called for 3/s. The fix splits the signature; the cadence is picked by a
// VIEW-only signature, and the full signature stays the skip-gate.
//
// Guards here:
//   * a TABLE over RadarShouldBake;
//   * frame-loop SIMULATIONS at 24fps, 33fps and 4.5fps, asserting the observed rates;
//   * a SOURCE LINT over world.cpp/world.h that the promotion has not come back.
//
// The optional FRAME FLOOR (uMinFrames) is covered too, but it ships DISABLED
// (kRadarBakeMinFrames == kWorldMapBakeMinFrames == 1) and the table pins that: a slow
// frame is still shorter than the idle cadence, so the clock already skips frames once
// the view is classified correctly, and a floor of 3 would push a MOVING camera to one
// walk per ~663ms at 4.5fps. The rows below cover the mechanism with an explicit value so
// the code stays honest if anyone re-enables it.
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
    unsigned    minFrames;     // frame floor in force for this row
    bool        noCache;
    bool        isRadar;
    bool        bldgHit;
    bool        expect;        // what the FIXED decision must return
};

// The shipped floor values. Named here so the rows read as "what production does".
static const unsigned R = kRadarBakeMinFrames;      // 1 -- floor disabled
static const unsigned W = kWorldMapBakeMinFrames;   // 1 -- floor disabled

static const Row kTable[] = {
    // ---- radar, view still, content churning (the defect): the 320ms idle cadence ----
    { "idle+fog @0ms",            false, true,      0,  1, R, false, true,  false, false },
    { "idle+fog @139ms",          false, true,    139,  4, R, false, true,  false, false },
    { "idle+fog @200ms",          false, true,    200,  5, R, false, true,  false, false },
    { "idle+fog @250ms",          false, true,    250,  6, R, false, true,  false, false },
    { "idle+fog @319ms",          false, true,    319,  7, R, false, true,  false, false },
    { "idle+fog @320ms",          false, true,    320,  8, R, false, true,  false, true  },
    { "idle+fog @350ms",          false, true,    350,  8, R, false, true,  false, true  },

    // ---- radar, view moving: the 140ms cadence, unchanged by the fix ----
    { "view moved @0ms",          true,  true,      0,  1, R, false, true,  false, false },
    { "view moved @139ms",        true,  true,    139,  3, R, false, true,  false, false },
    { "view moved @140ms",        true,  true,    140,  4, R, false, true,  false, true  },
    { "view moved @150ms",        true,  true,    150,  4, R, false, true,  false, true  },
    // a pan whose walk would draw the same pixels still skips: the cache is already right
    { "view moved, no content",   true,  false,   150,  4, R, false, true,  false, false },

    // ---- the skip-gate: nothing changed at all, never walk, however long it has been ----
    { "quiet @320ms",             false, false,   320,  8, R, false, true,  false, false },
    { "quiet @5s",                false, false,  5000, 99, R, false, true,  false, false },

    // ---- hit flash keeps re-baking, but still respects the cadence ----
    { "bldgHit @200ms",           false, false,   200,  5, R, false, true,  true,  false },
    { "bldgHit @320ms",           false, false,   320,  8, R, false, true,  true,  true  },

    // ---- no cached background yet: walk regardless of clock, frames or signatures ----
    { "no cache @0ms",            false, false,     0,  0, R, true,  true,  false, true  },
    { "no cache, quiet",          false, false,     0,  0, R, true,  true,  true,  true  },

    // ---- a 4.5fps VM, 222ms a frame. A slow frame is still SHORTER than the idle
    //      cadence, so with the view classified correctly the clock alone already skips
    //      alternate frames -- no frame floor needed. A MOVING camera still follows every
    //      frame, which is the point: the background is anchored to the view centre ----
    { "vm idle frame 1 @222ms",   false, true,    222,  1, R, false, true,  false, false },
    { "vm idle frame 2 @444ms",   false, true,    444,  2, R, false, true,  false, true  },
    { "vm drag frame 1 @222ms",   true,  true,    222,  1, R, false, true,  false, true  },

    // ---- a 33fps host, 30ms a frame: the wall clock decides, as it always did ----
    { "host drag frame 1 @30ms",  true,  true,     30,  1, R, false, true,  false, false },
    { "host drag frame 5 @151ms", true,  true,    151,  5, R, false, true,  false, true  },
    { "host idle frame 8 @242ms", false, true,    242,  8, R, false, true,  false, false },
    { "host idle frame 11 @333",  false, true,    333, 11, R, false, true,  false, true  },

    // ---- world map: no live dots, so its cached blit IS the frame; it walks on the
    //      timer alone, at the 1500ms idle cadence and 140ms while panning ----
    { "world idle+fog @800ms",    false, true,    800,  2, W, false, false, false, false },
    { "world idle+fog @1499ms",   false, true,   1499,  4, W, false, false, false, false },
    { "world idle+fog @1500ms",   false, true,   1500,  5, W, false, false, false, true  },
    { "world quiet @1500ms",      false, false,  1500,  5, W, false, false, false, true  },
    { "world moved @139ms",       true,  false,   139,  1, W, false, false, false, false },
    { "world moved @140ms",       true,  false,   140,  1, W, false, false, false, true  },

    // ---- the OPTIONAL frame floor, exercised with an explicit 3. Not the shipped
    //      setting (see the constants pinned below); these rows keep the mechanism
    //      correct for anyone who re-enables it ----
    { "floor(3) frame 1",         false, true,    666,  1, 3, false, true,  false, false },
    { "floor(3) frame 2",         false, true,    666,  2, 3, false, true,  false, false },
    { "floor(3) frame 3",         false, true,    666,  3, 3, false, true,  false, true  },
    { "floor(3) vs no cache",     false, false,     0,  0, 3, true,  true,  false, true  },
    { "floor(3), clock not due",  true,  true,    100,  9, 3, false, true,  false, false },
};
static const int kRows = (int)( sizeof( kTable ) / sizeof( kTable[0] ) );

static bool RunRow( const Row& r, bool bLegacy )
{
    const unsigned idle = r.isRadar ? kRadarBakeIdleMs : kWorldMapBakeIdleMs;
    return bLegacy
        ? LegacyShouldBake( r.viewMoved, r.walkChanged, r.since, r.noCache, r.isRadar, r.bldgHit )
        : RadarShouldBake( r.viewMoved, r.walkChanged, r.since, r.frames,
                           kRadarBakeMovingMs, idle, r.minFrames,
                           r.noCache, r.isRadar, r.bldgHit );
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

// The shipped frame floor is OFF. This is the whole of the deferral: if someone turns it
// on, this fails and they have to come back to the arithmetic in radarbake.h.
static void TestFrameFloorShipsDisabled()
{
    CHECK_EQ( kRadarBakeMinFrames, 1 );
    CHECK_EQ( kWorldMapBakeMinFrames, 1 );
    // With the floor at 1 the frame count cannot change any decision.
    for ( unsigned f = 1; f <= 20; f++ ) {
        CHECK_EQ( RadarShouldBake( false, true, 400, f, kRadarBakeMovingMs, kRadarBakeIdleMs,
                                   kRadarBakeMinFrames, false, true, false ),
                  RadarShouldBake( false, true, 400, 1, kRadarBakeMovingMs, kRadarBakeIdleMs,
                                   kRadarBakeMinFrames, false, true, false ) );
    }
}

// The table must actually separate the fixed decision from the pre-fix one, or it is
// pinning nothing.
static void TestTableDiscriminates( int disagree )
{
    CHECK( disagree >= 8 );
    // Name the discriminating cases explicitly so a future edit cannot quietly drop them.
    // Fog churn below the idle cadence, on a healthy frame rate...
    CHECK( !RadarShouldBake( false, true, 200, 9, kRadarBakeMovingMs, kRadarBakeIdleMs,
                             kRadarBakeMinFrames, false, true, false ) );
    CHECK(  LegacyShouldBake( false, true, 200, false, true, false ) );
    CHECK( !RadarShouldBake( false, true, 319, 9, kRadarBakeMovingMs, kRadarBakeIdleMs,
                             kRadarBakeMinFrames, false, true, false ) );
    CHECK(  LegacyShouldBake( false, true, 319, false, true, false ) );
    // ...and on a 4.5fps VM, where the pre-fix classification made a 222ms frame clear the
    // 140ms cadence every time. Correctly classified as idle, the 320ms cadence skips it.
    CHECK( !RadarShouldBake( false, true, 222, 1, kRadarBakeMovingMs, kRadarBakeIdleMs,
                             kRadarBakeMinFrames, false, true, false ) );
    CHECK(  LegacyShouldBake( false, true, 222, false, true, false ) );

    // The clock behaves exactly as it always did for a genuinely moving view, at every
    // elapsed time. This is the preservation guarantee for pan / zoom / rotate / buttons.
    for ( unsigned t = 0; t <= 400; t++ ) {
        CHECK_EQ( RadarShouldBake( true, true, t, 1, kRadarBakeMovingMs, kRadarBakeIdleMs,
                                   kRadarBakeMinFrames, false, true, false ),
                  LegacyShouldBake( true, true, t, false, true, false ) );
    }
}

// ---------------------------------------------------------------------------
// 2. Simulations -- reproduce the measured bake RATES (rr.bg) and frame shares.
// ---------------------------------------------------------------------------
// One radar ReRender per frame (the radar renders every frame by design); fog churn every
// frame (the local player has units moving); the view either dragging or still.
// `frameMs10` is the frame period in TENTHS of a millisecond: 24fps -> 417,
// 33fps -> 303, 4.5fps -> 2222.
struct Sim { int bakes; int frames; };

static Sim Simulate( int frames, int frameMs10, bool bViewMovingEveryFrame, bool bLegacy,
                     unsigned uMinFrames = kRadarBakeMinFrames )
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
                               kRadarBakeMovingMs, kRadarBakeIdleMs, uMinFrames,
                               false, true, false );
        if ( go ) { lastBake = t; framesSince = 0; s.bakes++; }
    }
    return s;
}

// 24fps: the rates WinOpus measured in the two Debug x64 captures.
static void TestObservedRates()
{
    const int kFrames24 = 240, kPeriod24 = 417;   // 10 s at 24fps, 41.7ms a frame

    // Pre-fix, view STILL, local units churning fog: 140ms cadence => the measured modal
    // rr.bg of 6/s (271 of 495 and 299 of 595 intervals).
    const Sim legacyIdle = Simulate( kFrames24, kPeriod24, false, true );
    CHECK( legacyIdle.bakes >= 57 && legacyIdle.bakes <= 62 );

    // Post-fix, same conditions: 320ms cadence => rr.bg 3/s, the documented idle design.
    const Sim fixedIdle = Simulate( kFrames24, kPeriod24, false, false );
    CHECK( fixedIdle.bakes >= 28 && fixedIdle.bakes <= 32 );

    // ~2x fewer whole-map walks -- and the walk is what rr.radar is made of.
    CHECK( legacyIdle.bakes >= fixedIdle.bakes * 19 / 10 );

    // PRESERVATION: while the view is dragging the fixed decision is identical to pre-fix.
    // If this drops, the view signature is missing a term.
    const Sim fixedDrag  = Simulate( kFrames24, kPeriod24, true, false );
    const Sim legacyDrag = Simulate( kFrames24, kPeriod24, true, true );
    CHECK_EQ( fixedDrag.bakes, legacyDrag.bakes );
    CHECK( fixedDrag.bakes >= 57 && fixedDrag.bakes <= 62 );

    // 33fps host: same story, the clock decides. Dragging reaches the full 140ms cadence.
    const int kFrames33 = 330, kPeriod33 = 303;   // 10 s at 33fps, 30.3ms a frame
    const Sim hostDrag       = Simulate( kFrames33, kPeriod33, true, false );
    const Sim hostDragLegacy = Simulate( kFrames33, kPeriod33, true, true );
    CHECK_EQ( hostDrag.bakes, hostDragLegacy.bakes );
    CHECK( hostDrag.bakes >= 66 && hostDrag.bakes <= 72 );        // ~7/s = the 140ms cadence
    const Sim hostIdle = Simulate( kFrames33, kPeriod33, false, false );
    CHECK( hostIdle.bakes >= 28 && hostIdle.bakes <= 32 );        // ~3/s = the 320ms cadence
}

// 4.5fps VM: 222ms a frame. Pre-fix, misclassifying idle as "moving" put the gate at
// 140ms, which a 222ms frame clears every time -- the throttle had disengaged entirely.
static void TestSlowMachine()
{
    const int kFramesVm = 45, kPeriodVm = 2222;   // 10 s at 4.5fps, 222.2ms a frame

    const Sim legacyIdle = Simulate( kFramesVm, kPeriodVm, false, true );
    const Sim fixedIdle  = Simulate( kFramesVm, kPeriodVm, false, false );

    // Pre-fix: a walk on essentially every frame.
    CHECK( legacyIdle.bakes * 100 / legacyIdle.frames >= 95 );
    // Post-fix: the 320ms cadence exceeds one 222ms frame, so the clock alone drops it to
    // every OTHER frame. No frame floor involved -- this is why the floor was deferred.
    CHECK( fixedIdle.bakes * 100 / fixedIdle.frames <= 55 );
    CHECK( fixedIdle.bakes * 100 / fixedIdle.frames >= 45 );

    // Dragging on the VM still walks every frame, and that is correct: the background is
    // anchored to the view centre, so a stale one slides under the live dots.
    const Sim fixedDrag  = Simulate( kFramesVm, kPeriodVm, true, false );
    const Sim legacyDrag = Simulate( kFramesVm, kPeriodVm, true, true );
    CHECK_EQ( fixedDrag.bakes, legacyDrag.bakes );
    CHECK( fixedDrag.bakes * 100 / fixedDrag.frames >= 95 );

    // ...and this is what a 3-frame floor WOULD do to that dragging camera: one walk per
    // ~666ms. Documented as an executable reason the floor ships disabled.
    const Sim withFloor = Simulate( kFramesVm, kPeriodVm, true, false, 3 );
    CHECK( withFloor.bakes * 100 / withFloor.frames <= 34 );
    CHECK( withFloor.bakes * 3 <= fixedDrag.bakes + 2 );
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
    //     resets and the radar walks on every frame instead.
    CHECK( worldH.find( "m_qwLastViewSig" ) != std::string::npos );
    CHECK_EQ( CountOf( body, "m_qwLastViewSig   = viewSig" ), 1 );
    CHECK_EQ( CountOf( body, "m_qwLastWalkSig   = walkSig" ), 1 );

    // (g) the frame-count input is fed a real counter, advanced once per render and
    //     cleared on the bake -- even though the floor it feeds ships disabled.
    CHECK( worldH.find( "m_uFramesSinceBake" ) != std::string::npos );
    CHECK_EQ( CountOf( body, "m_uFramesSinceBake++" ), 1 );
    CHECK_EQ( CountOf( body, "m_uFramesSinceBake = 0" ), 1 );
    CHECK( body.find( "kRadarBakeMinFrames" ) != std::string::npos );

    // (h) the render-rate levers the investigation ruled out must stay untouched:
    //     rate-limiting ReRender itself stretches the hit flash, which world.cpp:2411-2414
    //     decays once per radar render.
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

    TestFrameFloorShipsDisabled();
    TestTableDiscriminates( disagree );
    TestObservedRates();
    TestSlowMachine();
    TestMovedThenIdleReturnsToIdle();

    CHECK( !srcRoot.empty() );   // --src=<enations_latest/src> is mandatory: no silent skip
    if ( !srcRoot.empty() ) TestSourceLint( srcRoot );

    const int rc = microtest::Summary();
    std::printf( "[ui_radar_bake] %s\n", rc == 0 ? "PASS" : "FAIL" );
    return rc;
}
