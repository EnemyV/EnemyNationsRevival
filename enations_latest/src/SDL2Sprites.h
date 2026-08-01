// SDL2Sprites — GPU sprite layer (follow-on to the GPU terrain path).
//
// Moves the WHOLE world sprite layer (buildings, bridges, trees, vehicles,
// projectiles, explosions) off the per-frame CPU rasterizer: each sprite frame is
// cached once as an SDL_Texture and drawn over the GPU terrain through the area
// panel's renderer. Structures draw as axis-aligned quads (SDL_RenderCopy); vehicles
// are affine-warped quads (SDL_RenderGeometry, 4 view-space verts).
//
// Key design points:
//  - VIEW-SPACE storage (screen + UL) is scroll-invariant → re-projected to the
//    current UL each present, so sprites track the panning terrain (no flicker).
//  - Keyed by view-space position so a re-captured (static) sprite OVERWRITES instead
//    of appending — the engine repaints in partial dirty rects + extra rows, so a
//    plain list would grow without bound. Moving sprites' stale positions are dropped
//    by the per-dirty-rect removal in BeginFrame.
//  - Each sprite carries the engine's z-order key (projected m_ptCenter); the list is
//    y-sorted every present so list order = draw order regardless of capture order.
//
// Gated by [Advanced] GpuSprites (sub-flag under [Advanced] Renderer).
// Plan: plans/gpu-sprite-plan.md.
#pragma once

struct SDL_Renderer;
struct CSpriteDIB;

namespace SDL2Sprites
{
    bool Enabled( );

    // Activate the per-renderer sprite context for `r` (cheap state swap; each area-map
    // renderer keeps its own atlas/RT/cache). Called at panel creation, at the start of
    // each capture pass (DiscoverSpritesGpu, from the view's panel) and by Submit.
    void SetRenderer( SDL_Renderer* r );

    // Free a renderer's sprite context. Call from the area panel teardown BEFORE
    // SDL_DestroyRenderer; other live area maps' contexts are untouched.
    void ReleaseRenderer( SDL_Renderer* r );

    // The renderer the sprite atlas/layer is currently bound to (null if none). The
    // panel teardown checks this so an OLD area panel's destroy doesn't reset the
    // global sprite renderer that a NEWER area panel already owns (in-game load).
    SDL_Renderer* CurrentRenderer( );

    // Begin a capture pass for one world paint. zoom/dir detect a projection change
    // (clears the list). The dirty rect (VIEW coords = window rect + UL) drops the
    // entries it covers so moving sprites' stale positions don't linger.
    void BeginFrame( int zoom, int dir,
                     int dirtyViewX, int dirtyViewY, int dirtyW, int dirtyH );

    // Axis-aligned sprite (structure/tree/effect/projectile) at VIEW-space UL (vx,vy),
    // size (w,h), with z-order key (sortX,sortY). (clx,cly,clw,clh) is the sprite-LOCAL
    // sub-rect actually visible after clipping — for the building-construction "swype"
    // the engine pushes the clip top down so only the bottom band of the sprite shows,
    // and that band grows frame by frame. Pass (0,0,w,h) for an unclipped full draw.
    // Returns TRUE → caller skips CPU blit.
    bool CaptureStructure( const CSpriteDIB* dib, int zoom, int vx, int vy, int w, int h,
                           int clx, int cly, int clw, int clh,
                           int sortX, int sortY );

    // Affine-warped sprite (vehicle): 4 VIEW-space quad verts (engine order: 0=origin,
    // 1=+U, 2=opposite, 3=+V) + z-order key. Returns TRUE → caller skips CPU warp.
    bool CaptureVehicle( const CSpriteDIB* dib, int zoom, const int vx[4], const int vy[4],
                         int sortX, int sortY );

    // Projectile TRACER STREAK (eye-candy, client-local — NOT a sim object). A stretched,
    // additive, alpha-faded quad from (headX,headY) [the bullet, full colour] to
    // (tailX,tailY) [transparent], halfWidth px to each side. Coords are VIEW space
    // (window + UL) so they track the panning terrain like sprites. Recorded during the
    // capture pass and drawn as ONE batch in Submit, over the composited sprite layer.
    // Colour is 0-255 RGB; additive blending makes it read as a glow. headAlpha (0-255)
    // is the opacity at the bullet end (tail always fades to 0) — lets the caller dim
    // weaker shots. No texture/atlas entry — pure coloured geometry, so it never touches
    // the persistent sprite store.
    // outlineR/G/B = the shooter's TEAM colour, drawn as a wider additive halo under the core
    // tracer so trails are player-distinguishable (operator).
    void CaptureTrail( float headX, float headY, float tailX, float tailY,
                       float halfWidth, int r, int g, int b, int headAlpha,
                       int outlineR, int outlineG, int outlineB );

    // Impact flash (explosions): an additive soft disc of `radius` px at VIEW-space
    // (cx,cy), `centerAlpha` (0-255) at the centre fading to 0 at the rim. Drawn on top
    // of the sprite layer, fresh each present. Keep centerAlpha modest — it's additive.
    void CaptureFlash( float cx, float cy, float radius, int r, int g, int b, int centerAlpha );

    // Drop-shadow control (units). SetCaptureShadow(true) arms a ONE-SHOT: the next sprite
    // captured emits a ground shadow under it, then disarms (so a multi-piece unit gets
    // one shadow). Wrap a single unit's draw with true/false.
    void SetCaptureShadow( bool on );

    // Rotation (radians) for the NEXT captured structure sprite — projectiles set their
    // screen travel angle so the bullet faces where it flies. Set before the draw, reset
    // to 0 after. 0 = axis-aligned (the default for everything else).
    void SetCaptureRotation( float radians );

    void EndFrame( );

    // Draw the whole layer over the terrain, y-sorted, re-projected to the current UL.
    // vpW/vpH = content viewport size (off-screen entries are pruned).
    void Submit( SDL_Renderer* r, int ulX, int ulY, int vpW, int vpH );

    void InvalidateTextures( );

    // GPU device-lost (SDL_RENDER_TARGETS_RESET / SDL_RENDER_DEVICE_RESET, from the event
    // pump): bumps a generation every per-renderer context checks. TakeTargetsLost() is the
    // per-context one-shot consumer (DiscoverSpritesGpu) — true once after an event, with
    // the view's context active; caller reacts with InvalidateTextures() + a full capture.
    void NotifyTargetsLost( );
    bool TakeTargetsLost( );

    // True once a capture pass has populated the ACTIVE context's sprite store (cleared
    // by InvalidateTextures). DiscoverSpritesGpu's incremental-capture gate — replaces a
    // function static that was shared across views/renderer recreations.
    bool HasStore( );

    // Atlas overflow self-heal: the append-only packer can fill over a long session.
    // GetAtlasLoc flags it; the render orchestrator consumes the flag once per frame and,
    // when set, does a full InvalidateTextures()+repack so the atlas holds only the current
    // on-screen working set rather than every (frame×zoom) ever seen. Returns+clears the flag.
    bool TakeAtlasOverflow( );

    // --- Item 5 (dirty-rects) ---
    // Per-frame list of the regions that actually changed (moving/animating units), in
    // VIEW space. DirtyNewFrame() rotates this frame's list to "previous" (the PAINT_BOTH
    // carryover: a vacated spot must repaint one more frame) and starts a fresh list;
    // DirtyAddRect() records a unit's view-space bbox. The combined (this+previous) set is
    // what the incremental capture/render will touch — everything else persists.
    void DirtyNewFrame( );
    void DirtyAddRect( int vx, int vy, int w, int h );
    int  DirtyRectCount( );   // combined this+previous (probe / coalescing input)
    int  SpriteCount( );      // live g_sprites size (leak probe)

    // S2.3 incremental capture. BeginIncremental() reuses the persistent sprite store
    // (static trees/bridges kept) and drops last frame's dynamic entries; the caller then
    // recaptures only the dynamic objects (buildings/vehicles/projectiles) wrapped in
    // SetCaptureDynamic(true) so their keys are tracked for next frame's removal.
    void BeginIncremental( int zoom, int dir );
    void SetCaptureDynamic( bool dynamic );
}
