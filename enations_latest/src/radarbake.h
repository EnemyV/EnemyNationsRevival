// radarbake.h -- the minimap / world-map background BAKE CADENCE decision, extracted as a
// pure function so it can be table-tested (tests/ui/test_radar_bake.cpp) without a game.
//
// Background. CWndWorld::ReRender caches the unit-free minimap background in
// m_pdibRadarStatic and only re-walks the map (an O(window-pixels) scattered
// theMap._GetHex sampling pass) every so often. Between walks the cached background plus
// the live unit dots is a complete, correct frame, so a skipped walk costs only
// background freshness -- never correctness. That cache already exists; this header is
// only about HOW OFTEN the walk behind it runs.
//
// Two signatures, answering two different questions -- conflating them was the defect
// this header exists to prevent:
//
//   * the VIEW signature (centre, zoom, direction, mode) answers "did the camera move?"
//     -> it picks WHICH cadence applies: the fast one while the user is moving the view
//     (pan / zoom / rotate / mode buttons must feel immediate) or the slow one when the
//     view is still. Only human input changes it.
//   * the WALK signature (view terms PLUS fog generation, terrain-edit generation,
//     building count, resource-blink phase) answers "would a walk draw anything new?"
//     -> it is the skip-gate: when nothing changed, don't walk at all.
//
// Feeding content churn into the cadence question pins the radar to the fast cadence,
// because g_enFogVisGen is bumped whenever a hex flips fog state (CHex::IncVisible /
// DecVisible, terrain.inl:220-222) -- which happens continuously as the LOCAL player's
// own units move, since spotting only runs for units whose owner IsMe() (unit.cpp:1076).
// No click, no key, no camera movement is involved; the counter simply never settles
// while the player has anything moving.
//
// Header-only and dependency-free on purpose: no platform types, no game headers, no
// includes at all -- so tests/ui/test_radar_bake.cpp can compile THIS file, not a copy.

#ifndef RADARBAKE_H
#define RADARBAKE_H

// Cadences, in milliseconds since the last completed bake.
const unsigned kRadarBakeMovingMs    = 140;    // view is moving (radar and world map alike)
const unsigned kRadarBakeIdleMs      = 320;    // radar, view still
const unsigned kWorldMapBakeIdleMs   = 1500;   // world map, view still (slow-changing overview)

// Frame floor: a minimum number of the window's own render frames between walks, applied
// on top of the cadence. CURRENTLY DISABLED -- 1 means "no floor" -- on both paths.
//
// It is plumbed rather than absent because the idea recurs: the radar opts out of the
// per-window render throttle (world.h RendersEveryFrame() -> true), so the millisecond
// cadence is its only bound, and the reflex is to add a frame-relative one for slow
// machines. The arithmetic says not to. A slow frame is still SHORTER than the idle
// cadence (a 4.5fps frame is ~222ms, under kRadarBakeIdleMs), so once the view is
// classified correctly the clock already skips frames on its own. And a floor of 3 would
// push a MOVING camera to one walk per 3 frames -- ~663ms at 4.5fps, ~1.5s at 2fps --
// which is exactly the case that must not lag: the background is anchored to the view
// centre, so a stale one slides visibly under the live dots.
//
// Raise these only with a measurement showing the clock alone is not enough.
const unsigned kRadarBakeMinFrames    = 1;
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
//   uMinFrames        frame floor; 1 disables it (the shipped setting -- see above)
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
    // Both gates satisfied. The radar additionally SKIPS the walk when it would reproduce
    // the cached image exactly: its live unit dots are drawn outside the walk, so the
    // cache plus this frame's dots is already a complete, correct frame. The world map
    // has no live dots, so its cached blit is the frame and it walks once the clock allows.
    return !bIsRadar || bWalkSigChanged || bBldgHit;
}

#endif // RADARBAKE_H
