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
    void SetRenderer( SDL_Renderer* r );

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

    void EndFrame( );

    // Draw the whole layer over the terrain, y-sorted, re-projected to the current UL.
    // vpW/vpH = content viewport size (off-screen entries are pruned).
    void Submit( SDL_Renderer* r, int ulX, int ulY, int vpW, int vpH );

    void InvalidateTextures( );

    // --- Item 5 (dirty-rects) ---
    // Per-frame list of the regions that actually changed (moving/animating units), in
    // VIEW space. DirtyNewFrame() rotates this frame's list to "previous" (the PAINT_BOTH
    // carryover: a vacated spot must repaint one more frame) and starts a fresh list;
    // DirtyAddRect() records a unit's view-space bbox. The combined (this+previous) set is
    // what the incremental capture/render will touch — everything else persists.
    void DirtyNewFrame( );
    void DirtyAddRect( int vx, int vy, int w, int h );
    int  DirtyRectCount( );   // combined this+previous (probe / coalescing input)
}
