// radarbake.h -- the minimap / world-map background BAKE CADENCE decision, extracted as a
// pure function so it can be table-tested (tests/ui/test_radar_bake.cpp) without a game.
//
// Background. CWndWorld::ReRender caches the unit-free minimap background in
// m_pdibRadarStatic and only re-walks the map (an O(window-pixels) scattered
// theMap._GetHex sampling pass, measured 7.7-9.4ms per bake in Debug x64) every so often.
// Between walks the cached background plus the live unit dots is a complete, correct
// frame, so a skipped walk costs only background freshness -- never correctness.
//
// TWO gates decide, and they answer different questions:
//
//  1. WALL CLOCK -- how long since the last walk. Two cadences: a FAST one while the user
//     is moving the view (pan / zoom / rotate / mode buttons must feel immediate) and a
//     SLOW one when the view is still.
//  2. FRAME COUNT -- how many of this window's render frames since the last walk. The
//     radar deliberately opts out of the frame-rate throttle every other window gets
//     (world.h RendersEveryFrame() returns true for the radar, so wndbase.h
//     DecideRenderFrame skips the interval test), which leaves the wall-clock gate as its
//     ONLY bound. That bound inverts on a slow machine: at 4.5 fps a frame is 221ms, so
//     EVERY frame satisfies even the slow cadence and the walk runs on ~every frame --
//     the throttle disengages exactly where it is needed most. A minimum frame count
//     caps the walk at 1-in-N frames whatever the frame rate.
//
// WHICH SIGNATURE DRIVES WHICH -- the defect this header exists to prevent:
//
//   * the VIEW signature (centre, zoom, direction, mode) answers "did the camera move?"
//     -> it picks WHICH cadence applies. Only human input changes it.
//   * the WALK signature (view terms PLUS fog generation, terrain-edit generation,
//     building count, resource-blink phase) answers "would a walk draw anything new?"
//     -> it is the skip-gate: when nothing changed, don't walk at all.
//
// Feeding content churn into the cadence question pins the radar to the fast cadence
// forever, because g_enFogVisGen is bumped by every hex that flips fog state
// (terrain.inl:220-222), i.e. continuously, by every moving AI unit, with no human
// input at all. That is a ~2x multiplier on the most expensive thing the renderer does.
//
// Header-only and dependency-free on purpose: no platform types, no game headers, no
// includes at all -- so tests/ui/test_radar_bake.cpp can compile THIS file, not a copy.

#ifndef RADARBAKE_H
#define RADARBAKE_H

// Cadences, in milliseconds since the last completed bake.
const unsigned kRadarBakeMovingMs    = 140;    // view is moving (radar and world map alike)
const unsigned kRadarBakeIdleMs      = 320;    // radar, view still
const unsigned kWorldMapBakeIdleMs   = 1500;   // world map, view still (slow-changing overview)

// Frame floor, in render frames of the window since the last completed bake. Applies to
// BOTH cadences: it is a ceiling on the SHARE of frames the walk may occupy, not a
// cadence of its own. 3 = the walk can never cost more than a third of the frames.
const unsigned kRadarBakeMinFrames   = 3;
// The world map is EXEMPT (1 = no floor) because it already has the frame-relative bound
// the radar lacks: world.h MinRenderIntervalMs() returns 333 for it, so it renders -- and
// therefore walks -- at most ~3/s however fast the game runs. Imposing 3 frames on top
// would put a world-map pan at ~1 re-bake/s and bring back the 1-2s pan lag that the
// centre/zoom/dir detection in ReRender was added to fix.
const unsigned kWorldMapBakeMinFrames = 1;

// Which cadence applies right now.
inline unsigned RadarBakeCadenceMs( bool bViewMoved, unsigned dwMovingMs, unsigned dwIdleMs )
{
    return bViewMoved ? dwMovingMs : dwIdleMs;
}

// The whole decision.
//
//   bViewMoved        view signature differs from the one the last bake was made at
//                     (centre / zoom / direction / mode ONLY -- never fog or content)
//   bWalkSigChanged   full content signature differs from the last bake's
//   dwSinceLastBake   ms elapsed since the last completed bake (caller does the unsigned
//                     subtraction, so tick wraparound keeps its natural semantics)
//   uFramesSinceBake  render frames of THIS window since the last completed bake
//   dwMovingMs        cadence to use while the view is moving
//   dwIdleMs          cadence to use while the view is still
//   uMinFrames        frame floor; 1 disables it
//   bNoCache          there is no cached background yet -- must bake, whatever else says
//   bIsRadar          radar (minimap) vs the world-map overview
//   bBldgHit          a building-hit flash is animating and must keep re-baking
//
// Pure: no clock read, no globals, no state.
inline bool RadarShouldBake( bool     bViewMoved,
                             bool     bWalkSigChanged,
                             unsigned dwSinceLastBake,
                             unsigned uFramesSinceBake,
                             unsigned dwMovingMs,
                             unsigned dwIdleMs,
                             unsigned uMinFrames,
                             bool     bNoCache,
                             bool     bIsRadar,
                             bool     bBldgHit )
{
    if ( bNoCache )                                                             // nothing to show
        return true;
    if ( dwSinceLastBake < RadarBakeCadenceMs( bViewMoved, dwMovingMs, dwIdleMs ) )
        return false;                                                           // too soon (clock)
    if ( uFramesSinceBake < uMinFrames )
        return false;                                                           // too soon (frames)
    // Both gates satisfied. The radar additionally SKIPS the bake when a bake would
    // reproduce the cached image exactly: its live unit dots are drawn outside the bake,
    // so the cache plus this frame's dots is already a complete, correct frame. The
    // world map has no live dots, so its cached blit is the frame and it always bakes
    // once the clock allows.
    return !bIsRadar || bWalkSigChanged || bBldgHit;
}

#endif // RADARBAKE_H
