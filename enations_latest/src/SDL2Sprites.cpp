#include "stdafx.h"
#include "SDL2Sprites.h"
#include "sprite.h"          // CSpriteDIB::DecodeToRGBA, NUM_ZOOM_LEVELS
#include "w22_settings.h"    // w22::GetProfileInt — [Advanced] GpuSprites flag
#include "Perf.h"            // perf counters (EN_PERF)

#include <SDL.h>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <climits>
#include <cmath>

// GPU sprite layer with TEXTURE-ATLAS BATCHING. All sprite frames are packed into one
// large atlas texture; the whole visible sprite layer is then drawn with a SINGLE
// SDL_RenderGeometry call (one big sorted vertex buffer) into the composite RT, which
// is re-presented as one blit. This turns ~900 per-sprite draw calls into ~1 — the
// per-draw-call overhead (huge on the Debug D3D9 backend) was what made the un-batched
// path 10–20× slower than the CPU composite. Plan: plans/gpu-sprite-plan.md.
namespace
{
    struct Sprite
    {
        int      sortX, sortY;   // engine m_ptCenter z-order key
        uint64_t seq;            // capture order — tiebreak for equal sort key
        bool     isVehicle;
        bool     isStructure;    // building/bridge/tree — sorts UNDER the mobile/effect layer at a tie
        int      ax, ay, aw, ah; // atlas sub-rect (the FULL sprite's texture)
        int      vx, vy;         // structure UL (view space); FULL size == aw,ah
        int      cx, cy, cw, ch; // sprite-LOCAL visible band (construction wipe); full = 0,0,aw,ah
        int      qx[4], qy[4];   // vehicle quad verts (view space)
        float    rot;            // structure-sprite rotation (radians, 0 = none); projectiles
    };

    // Keyed by (DIB, view-space pos) — see header rationale (dedup static re-captures;
    // moving/animating stale keys dropped by per-rect removal).
    struct SprKey
    {
        const void* dib; int x, y;
        bool operator==( const SprKey& o ) const { return dib == o.dib && x == o.x && y == o.y; }
    };
    struct SprKeyHash
    {
        size_t operator()( const SprKey& k ) const
        {
            size_t h = (size_t)k.dib;
            h = h * 1099511628211ull ^ (uint32_t)k.x;
            h = h * 1099511628211ull ^ (uint32_t)k.y;
            return h;
        }
    };
    std::unordered_map<SprKey, Sprite, SprKeyHash> g_sprites;

    // --- Atlas: one big texture, shelf-packed. (dib,zoom) → atlas sub-rect. ---
    struct AtlasLoc { int x, y, w, h; };
    std::unordered_map<uint64_t, AtlasLoc> g_atlasMap;   // key = dib^zoom (see AtlasKey)
    SDL_Texture* g_atlas = nullptr;
    int g_atlasW = 0, g_atlasH = 0;
    int g_shelfX = 0, g_shelfY = 0, g_shelfH = 0;        // shelf packer cursor
    const int kPad = 1;                                  // 1px gutter (no neighbour bleed)

    static uint64_t AtlasKey( const CSpriteDIB* dib, int zoom )
    {
        return ( (uint64_t)(uintptr_t)dib ) * 8 + (unsigned)zoom;
    }

    SDL_Renderer* g_renderer = nullptr;
    int  g_enabled = -1;
    int  g_zoom = -1, g_dir = -1;
    bool g_inFrame = false;
    uint64_t g_seq = 0;

    // Composite RT cache. The layer lives in g_rt and is re-presented as one blit.
    // It is updated INCREMENTALLY — only the accumulated dirty region is cleared and
    // its overlapping sprites redrawn (z-sorted), like the CPU m_dibSprite — so a busy
    // zoomed-out scene costs ∝ what changed, not ∝ all 6000+ visible sprites. A view
    // pan (UL change) invalidates content-space positions → one full rebuild.
    SDL_Texture* g_rt = nullptr;
    int  g_rtW = 0, g_rtH = 0;
    bool g_dirty = true;                 // a capture/removal happened since last present
    int  g_lastUlX = INT_MIN, g_lastUlY = INT_MIN;
    bool g_accumValid = false;           // g_accum holds a dirty region this present
    int  g_accumX0, g_accumY0, g_accumX1, g_accumY1;   // accumulated dirty rect (VIEW space)

    // Item 5 (dirty-rects): per-frame changed regions (moving/animating units), VIEW space.
    // g_dirtyPrev is last frame's set, carried one more frame (PAINT_BOTH) so a vacated
    // spot repaints. Combined (cur ∪ prev) = what the incremental path touches.
    std::vector<SDL_Rect> g_dirtyCur;
    std::vector<SDL_Rect> g_dirtyPrev;

    // Item 5 S2.3 (incremental capture): the keys of the DYNAMIC sprites captured this
    // frame (buildings/vehicles/projectiles — anything that can move or change). On an
    // incremental frame BeginIncremental() removes last frame's set (so a moved unit's
    // old-position entry doesn't linger) and the dynamic objects are recaptured fresh,
    // while static trees/bridges keep their persistent g_sprites entries (skipped).
    std::vector<SprKey> g_dynKeys;
    bool                g_captureDynamic = false;   // mark captures into g_dynKeys
    bool                g_captureWasFull = true;     // S2.4: full vs incremental EMIT

    // Projectile tracer streaks (eye-candy, client-local). A plain list rebuilt every
    // capture pass and drawn as one additive batch in Submit, then cleared — they are
    // NOT keyed/persisted like sprites (no atlas entry, no z-sort, drawn on top).
    struct Trail
    {
        float hx, hy, tx, ty;   // head (bullet) → tail, VIEW space
        float halfW;            // half-width (px)
        Uint8 r, g, b, aHead;   // colour + head opacity (tail fades to 0)
    };
    std::vector<Trail> g_trails;

    // Soft radial discs (a triangle fan: bright/dark centre -> alpha-0 rim, no texture),
    // VIEW space. Two per-frame lists like trails: g_flashes draw ADDITIVE on top (impact
    // pops), g_shadows draw alpha-BLEND dark UNDER the sprite layer (drop shadows). Both
    // refilled every capture pass and cleared after drawing.
    struct Disc
    {
        float cx, cy, rx, ry;
        Uint8 r, g, b, aCenter;
    };
    std::vector<Disc> g_flashes;
    std::vector<Disc> g_shadows;

    // One-shot shadow arm: set true before a unit's draw; the FIRST sprite captured then
    // emits a ground shadow and disarms it (so multi-piece/multi-layer units get one).
    bool g_captureShadow = false;

    // Rotation (radians) applied to the next captured structure sprite — projectiles set
    // this to their screen travel angle so the bullet faces where it flies. 0 = axis-aligned.
    float g_captureRot = 0.0f;

    void AccumDirty( int x, int y, int w, int h )
    {
        if ( w <= 0 || h <= 0 ) return;
        if ( !g_accumValid )
        {
            g_accumX0 = x; g_accumY0 = y; g_accumX1 = x + w; g_accumY1 = y + h;
            g_accumValid = true;
        }
        else
        {
            g_accumX0 = __min( g_accumX0, x );     g_accumY0 = __min( g_accumY0, y );
            g_accumX1 = __max( g_accumX1, x + w );  g_accumY1 = __max( g_accumY1, y + h );
        }
    }

    bool RectsOverlap( int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh )
    {
        return !( ax + aw <= bx || bx + bw <= ax || ay + ah <= by || by + bh <= ay );
    }

    bool EnsureAtlas( )
    {
        if ( g_atlas )
            return true;
        SDL_RendererInfo info;
        int dim = 4096;
        if ( SDL_GetRendererInfo( g_renderer, &info ) == 0 )
        {
            int mx = __min( info.max_texture_width, info.max_texture_height );
            if ( mx > 0 ) dim = __min( dim, mx );
        }
        g_atlas = SDL_CreateTexture( g_renderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STATIC, dim, dim );
        if ( !g_atlas )
            return false;
        SDL_SetTextureBlendMode( g_atlas, SDL_BLENDMODE_BLEND );
        SDL_SetTextureScaleMode( g_atlas, SDL_ScaleModeNearest );  // no neighbour bleed
        g_atlasW = g_atlasH = dim;
        g_shelfX = g_shelfY = g_shelfH = 0;
        return true;
    }

    // Pack + upload a sprite frame into the atlas; return its sub-rect (or {0,0,0,0} if
    // the atlas is full → caller falls back to the CPU path for that sprite).
    AtlasLoc GetAtlasLoc( const CSpriteDIB* dib, int zoom, int w, int h )
    {
        if ( !g_renderer || w <= 0 || h <= 0 || !EnsureAtlas( ) )
            return { 0, 0, 0, 0 };

        uint64_t key = AtlasKey( dib, zoom );
        auto it = g_atlasMap.find( key );
        if ( it != g_atlasMap.end( ) )
            return it->second;

        if ( w > g_atlasW || h > g_atlasH )
            return { 0, 0, 0, 0 };

        // Shelf-pack with a 1px gutter.
        if ( g_shelfX + w + kPad > g_atlasW )          // new shelf
        {
            g_shelfX = 0;
            g_shelfY += g_shelfH + kPad;
            g_shelfH = 0;
        }
        if ( g_shelfY + h + kPad > g_atlasH )          // atlas full → CPU fallback
            return { 0, 0, 0, 0 };

        AtlasLoc loc { g_shelfX, g_shelfY, w, h };
        g_shelfX += w + kPad;
        if ( h > g_shelfH ) g_shelfH = h;

        std::vector<unsigned> rgba( (size_t)w * h );
        if ( !dib->DecodeToRGBA( rgba.data( ) ) )      // structure-RLE vs vehicle-flat
            return { 0, 0, 0, 0 };
        SDL_Rect dst = { loc.x, loc.y, w, h };
        SDL_UpdateTexture( g_atlas, &dst, rgba.data( ), w * 4 );

        g_atlasMap[ key ] = loc;
        return loc;
    }
}

namespace SDL2Sprites
{
    bool Enabled( )
    {
        if ( g_enabled < 0 )
            g_enabled = w22::GetProfileInt( "Advanced", "GpuSprites", 1 ) ? 1 : 0;
        return g_enabled != 0;
    }

    void SetRenderer( SDL_Renderer* r )
    {
        if ( r != g_renderer )
            InvalidateTextures( );
        g_renderer = r;
    }

    void BeginFrame( int zoom, int dir, int dvx, int dvy, int dw, int dh )
    {
        g_inFrame = false;
        if ( !Enabled( ) || !g_renderer )
            return;

        if ( zoom != g_zoom || dir != g_dir )   // projection change invalidates positions
        {
            g_sprites.clear( );
            g_zoom = zoom; g_dir = dir;
            g_dirty = true;
        }

        g_dynKeys.clear( );   // full walk recaptures dynamic objects → repopulates this
        g_captureWasFull = true;

        AccumDirty( dvx, dvy, dw, dh );   // this repaint's dirty region (view space)

        if ( dw > 0 && dh > 0 )
            for ( auto it = g_sprites.begin( ); it != g_sprites.end( ); )
            {
                const Sprite& s = it->second;
                int bx = s.vx, by = s.vy, bw = s.aw, bh = s.ah;
                if ( s.isVehicle )
                {
                    bx = bw = s.qx[0]; by = bh = s.qy[0];
                    for ( int i = 1; i < 4; ++i )
                    {
                        bx = __min( bx, s.qx[i] ); bw = __max( bw, s.qx[i] );
                        by = __min( by, s.qy[i] ); bh = __max( bh, s.qy[i] );
                    }
                    bw -= bx; bh -= by;
                }
                if ( RectsOverlap( bx, by, bw, bh, dvx, dvy, dw, dh ) )
                    { it = g_sprites.erase( it ); g_dirty = true; }
                else
                    ++it;
            }

        g_inFrame = true;
    }

    // S2.3 incremental capture: begin a frame that REUSES the persistent g_sprites
    // (static trees/bridges stay) and only refreshes the dynamic objects. Remove last
    // frame's dynamic entries (a moved unit's old position would otherwise linger), then
    // the caller recaptures buildings/vehicles/projectiles with SetCaptureDynamic(true).
    // A projection change still forces a full clear (positions are invalid).
    void BeginIncremental( int zoom, int dir )
    {
        g_inFrame = false;
        if ( !Enabled( ) || !g_renderer )
            return;
        if ( zoom != g_zoom || dir != g_dir )
        {
            g_sprites.clear( );
            g_zoom = zoom; g_dir = dir;
            g_dirty = true;
        }
        for ( const SprKey& k : g_dynKeys )
            g_sprites.erase( k );
        g_dynKeys.clear( );
        g_dirty = true;     // dynamic objects will be re-captured + re-emitted
        g_captureWasFull = false;
        g_inFrame = true;
    }

    void SetCaptureDynamic( bool b ) { g_captureDynamic = b; }
    void SetCaptureShadow( bool b ) { g_captureShadow = b; }
    void SetCaptureRotation( float radians ) { g_captureRot = radians; }
}

namespace
{
    // Push a ground shadow ellipse under a sprite's view-space bounding box, then disarm
    // the one-shot. Anchored at the bottom-centre, sized from the sprite width.
    void EmitShadowForBox( int vx, int vy, int w, int h )
    {
        if ( !g_captureShadow )
            return;
        g_captureShadow = false;   // one shadow per unit (first piece wins)
        Disc d;
        d.rx = w * 0.42f;
        d.ry = d.rx * 0.40f;
        d.cx = vx + w * 0.5f;
        d.cy = vy + h - d.ry * 0.8f;     // sit just above the sprite's bottom edge
        d.r = d.g = d.b = 0;             // black
        d.aCenter = 90;                  // soft; alpha-blended
        g_shadows.push_back( d );
    }
}

extern int g_enSprIsStruct;   // set by the engine draw path (terrain.cpp): 1=structure

namespace SDL2Sprites
{
    bool CaptureStructure( const CSpriteDIB* dib, int zoom, int vx, int vy, int w, int h,
                           int clx, int cly, int clw, int clh,
                           int sortX, int sortY )
    {
        if ( !g_inFrame || !g_renderer )
            return false;
        AtlasLoc loc = GetAtlasLoc( dib, zoom, w, h );
        if ( loc.w == 0 )
            return false;   // atlas full → CPU fallback
        // Clamp the visible band to the sprite (defensive).
        if ( clx < 0 ) { clw += clx; clx = 0; }
        if ( cly < 0 ) { clh += cly; cly = 0; }
        if ( clx + clw > w ) clw = w - clx;
        if ( cly + clh > h ) clh = h - cly;
        if ( clw <= 0 || clh <= 0 )
            return true;    // fully clipped away — nothing to draw, but skip CPU blit
        EmitShadowForBox( vx, vy, w, h );   // one-shot; no-op unless armed for this unit
        Sprite s { };
        s.sortX = sortX; s.sortY = sortY; s.seq = ++g_seq; s.isVehicle = false;
        s.isStructure = ( g_enSprIsStruct != 0 );   // building/bridge/tree vs effect/projectile
        s.ax = loc.x; s.ay = loc.y; s.aw = loc.w; s.ah = loc.h;
        s.vx = vx; s.vy = vy;
        s.cx = clx; s.cy = cly; s.cw = clw; s.ch = clh;
        s.rot = g_captureRot;
        // Key on the FULL sprite UL so the construction wipe (same UL, growing band)
        // overwrites in place each frame instead of leaving stale partial bands.
        SprKey key { dib, vx, vy };
        g_sprites[ key ] = s;
        if ( g_captureDynamic ) g_dynKeys.push_back( key );
        g_dirty = true;
        return true;
    }

    bool CaptureVehicle( const CSpriteDIB* dib, int zoom, const int vx[4], const int vy[4],
                         int sortX, int sortY )
    {
        if ( !g_inFrame || !g_renderer )
            return false;
        int w = dib->Width( ), h = dib->Height( );
        AtlasLoc loc = GetAtlasLoc( dib, zoom, w, h );
        if ( loc.w == 0 )
            return false;
        if ( g_captureShadow )
        {
            int x0 = vx[0], x1 = vx[0], y0 = vy[0], y1 = vy[0];
            for ( int i = 1; i < 4; ++i )
            {
                x0 = __min( x0, vx[i] ); x1 = __max( x1, vx[i] );
                y0 = __min( y0, vy[i] ); y1 = __max( y1, vy[i] );
            }
            EmitShadowForBox( x0, y0, x1 - x0, y1 - y0 );
        }
        Sprite s { };
        s.sortX = sortX; s.sortY = sortY; s.seq = ++g_seq; s.isVehicle = true;
        s.isStructure = false;   // vehicles are the mobile layer (on top of structures at a tie)
        s.ax = loc.x; s.ay = loc.y; s.aw = loc.w; s.ah = loc.h;
        for ( int i = 0; i < 4; ++i ) { s.qx[i] = vx[i]; s.qy[i] = vy[i]; }
        SprKey key { dib, vx[0], vy[0] };
        g_sprites[ key ] = s;
        if ( g_captureDynamic ) g_dynKeys.push_back( key );
        g_dirty = true;
        return true;
    }

    void CaptureTrail( float headX, float headY, float tailX, float tailY,
                       float halfWidth, int r, int g, int b, int headAlpha )
    {
        if ( !g_inFrame || !g_renderer )
            return;
        Trail t;
        t.hx = headX; t.hy = headY; t.tx = tailX; t.ty = tailY;
        t.halfW = halfWidth;
        t.r = (Uint8)r; t.g = (Uint8)g; t.b = (Uint8)b; t.aHead = (Uint8)headAlpha;
        g_trails.push_back( t );
    }

    void CaptureFlash( float cx, float cy, float radius, int r, int g, int b, int centerAlpha )
    {
        if ( !g_inFrame || !g_renderer || radius < 1.0f || centerAlpha <= 0 )
            return;
        Disc d;
        d.cx = cx; d.cy = cy; d.rx = radius; d.ry = radius;
        d.r = (Uint8)r; d.g = (Uint8)g; d.b = (Uint8)b; d.aCenter = (Uint8)centerAlpha;
        g_flashes.push_back( d );
    }

    void EndFrame( ) { g_inFrame = false; }

    // Sprite view-space bounding box.
    static void SprBox( const Sprite& s, int& x, int& y, int& w, int& h )
    {
        if ( !s.isVehicle ) { x = s.vx; y = s.vy; w = s.aw; h = s.ah; return; }
        x = s.qx[0]; y = s.qy[0]; int x1 = x, y1 = y;
        for ( int i = 1; i < 4; ++i )
        {
            x = __min( x, s.qx[i] ); y = __min( y, s.qy[i] );
            x1 = __max( x1, s.qx[i] ); y1 = __max( y1, s.qy[i] );
        }
        w = x1 - x; h = y1 - y;
    }

    // Emit one atlas-batched SDL_RenderGeometry for a sorted sprite list.
    static void EmitOrder( SDL_Renderer* r, const std::vector<const Sprite*>& order,
                           int ulX, int ulY )
    {
        static std::vector<SDL_Vertex> verts; static std::vector<int> idx;
        verts.clear( ); idx.clear( );
        verts.reserve( order.size( ) * 4 ); idx.reserve( order.size( ) * 6 );
        const float iaw = 1.0f / (float)g_atlasW, iah = 1.0f / (float)g_atlasH;
        const SDL_Color white = { 255, 255, 255, 255 };
        for ( const Sprite* s : order )
        {
            int base = (int)verts.size( );
            SDL_Vertex v[4];
            if ( !s->isVehicle )
            {
                // Draw only the visible band (sprite-local cx,cy,cw,ch) so the
                // construction wipe reveals the building bottom-up. UVs map the same
                // band within the atlas sub-rect; dest is offset by the band origin.
                float uL = ( s->ax + s->cx ) * iaw,         uT = ( s->ay + s->cy ) * iah;
                float uR = ( s->ax + s->cx + s->cw ) * iaw, uB = ( s->ay + s->cy + s->ch ) * iah;
                float x0 = (float)( s->vx + s->cx - ulX ), y0 = (float)( s->vy + s->cy - ulY );
                float x1 = x0 + s->cw, y1 = y0 + s->ch;
                v[0].position = { x0, y0 }; v[0].tex_coord = { uL, uT };
                v[1].position = { x1, y0 }; v[1].tex_coord = { uR, uT };
                v[2].position = { x1, y1 }; v[2].tex_coord = { uR, uB };
                v[3].position = { x0, y1 }; v[3].tex_coord = { uL, uB };
                // Optional rotation about the sprite centre (projectiles face their travel
                // direction). Rotate the dest corners only; UVs stay put.
                if ( s->rot != 0.0f )
                {
                    float ccx = (float)( s->vx + s->aw * 0.5f - ulX );
                    float ccy = (float)( s->vy + s->ah * 0.5f - ulY );
                    float sn = std::sin( s->rot ), cs = std::cos( s->rot );
                    for ( int i = 0; i < 4; ++i )
                    {
                        float dx = v[i].position.x - ccx, dy = v[i].position.y - ccy;
                        v[i].position.x = ccx + dx * cs - dy * sn;
                        v[i].position.y = ccy + dx * sn + dy * cs;
                    }
                }
            }
            else
            {
                float uL = s->ax * iaw, uT = s->ay * iah;
                float uR = ( s->ax + s->aw ) * iaw, uB = ( s->ay + s->ah ) * iah;
                static const float fu[4] = { 0, 1, 1, 0 }, fv[4] = { 0, 0, 1, 1 };
                for ( int i = 0; i < 4; ++i )
                {
                    v[i].position = { (float)( s->qx[i] - ulX ), (float)( s->qy[i] - ulY ) };
                    v[i].tex_coord = { fu[i] ? uR : uL, fv[i] ? uB : uT };
                }
            }
            for ( int i = 0; i < 4; ++i ) { v[i].color = white; verts.push_back( v[i] ); }
            idx.push_back( base + 0 ); idx.push_back( base + 1 ); idx.push_back( base + 2 );
            idx.push_back( base + 0 ); idx.push_back( base + 2 ); idx.push_back( base + 3 );
        }
        if ( !verts.empty( ) )
            SDL_RenderGeometry( r, g_atlas, verts.data( ), (int)verts.size( ),
                                idx.data( ), (int)idx.size( ) );
    }

    // Draw the accumulated tracer streaks as ONE additive, untextured RenderGeometry
    // batch onto the current target (called after the sprite layer is composited, so
    // tracers glow on top). Each streak is a quad: full colour + opaque at the head
    // (the bullet), same colour + zero alpha at the tail → an additive fade. Clears the
    // list. ulX/ulY map VIEW space → the panel viewport (same as the sprite layer).
    static void DrawTrails( SDL_Renderer* r, int ulX, int ulY )
    {
        if ( g_trails.empty( ) )
            return;

        static std::vector<SDL_Vertex> verts; static std::vector<int> idx;
        verts.clear( ); idx.clear( );
        verts.reserve( g_trails.size( ) * 4 ); idx.reserve( g_trails.size( ) * 6 );

        for ( const Trail& t : g_trails )
        {
            float hx = t.hx - ulX, hy = t.hy - ulY;
            float tx = t.tx - ulX, ty = t.ty - ulY;
            float dx = tx - hx, dy = ty - hy;
            float len = std::sqrt( dx * dx + dy * dy );
            if ( len < 0.5f )
                continue;
            // Unit perpendicular × half-width gives the streak's two sides.
            float px = -dy / len * t.halfW, py = dx / len * t.halfW;

            int            base = (int)verts.size( );
            // Head alpha (< 255) keeps the additive tracer from blowing out to white at
            // the bullet; tail fades to fully transparent.
            const SDL_Color head = { t.r, t.g, t.b, t.aHead };
            const SDL_Color tail = { t.r, t.g, t.b, 0 };
            SDL_Vertex      v[4];
            v[0].position = { hx + px, hy + py }; v[0].color = head;
            v[1].position = { hx - px, hy - py }; v[1].color = head;
            v[2].position = { tx - px, ty - py }; v[2].color = tail;
            v[3].position = { tx + px, ty + py }; v[3].color = tail;
            for ( int i = 0; i < 4; ++i ) { v[i].tex_coord = { 0, 0 }; verts.push_back( v[i] ); }
            idx.push_back( base + 0 ); idx.push_back( base + 1 ); idx.push_back( base + 2 );
            idx.push_back( base + 0 ); idx.push_back( base + 2 ); idx.push_back( base + 3 );
        }

        if ( !verts.empty( ) )
        {
            SDL_BlendMode prev = SDL_BLENDMODE_BLEND;
            SDL_GetRenderDrawBlendMode( r, &prev );
            SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_ADD );   // glow; untextured geometry
            SDL_RenderGeometry( r, nullptr, verts.data( ), (int)verts.size( ),
                                idx.data( ), (int)idx.size( ) );
            SDL_SetRenderDrawBlendMode( r, prev );
        }

        g_trails.clear( );
    }

    // Append a soft radial disc as a triangle fan: centre vertex at aCenter alpha, rim
    // vertices at alpha 0 → a gradient falloff with no texture (same trick as the trail).
    static void AppendDiscFan( std::vector<SDL_Vertex>& verts, std::vector<int>& idx,
                               float cx, float cy, float rx, float ry,
                               Uint8 r, Uint8 g, Uint8 b, Uint8 aCenter )
    {
        const int   SEG = 16;
        const float TWO_PI = 6.2831853f;
        int         base = (int)verts.size( );

        SDL_Vertex c; c.position = { cx, cy }; c.tex_coord = { 0, 0 };
        c.color = { r, g, b, aCenter };
        verts.push_back( c );
        for ( int i = 0; i < SEG; ++i )
        {
            float a = TWO_PI * i / SEG;
            SDL_Vertex v; v.position = { cx + std::cos( a ) * rx, cy + std::sin( a ) * ry };
            v.tex_coord = { 0, 0 }; v.color = { r, g, b, 0 };
            verts.push_back( v );
        }
        for ( int i = 0; i < SEG; ++i )
        {
            idx.push_back( base );
            idx.push_back( base + 1 + i );
            idx.push_back( base + 1 + ( i + 1 ) % SEG );
        }
    }

    // Impact flashes: additive soft discs drawn on top (over the sprite layer).
    static void DrawFlashes( SDL_Renderer* r, int ulX, int ulY )
    {
        if ( g_flashes.empty( ) )
            return;
        static std::vector<SDL_Vertex> verts; static std::vector<int> idx;
        verts.clear( ); idx.clear( );
        for ( const Disc& d : g_flashes )
            AppendDiscFan( verts, idx, d.cx - ulX, d.cy - ulY, d.rx, d.ry,
                           d.r, d.g, d.b, d.aCenter );
        if ( !verts.empty( ) )
        {
            SDL_BlendMode prev = SDL_BLENDMODE_BLEND;
            SDL_GetRenderDrawBlendMode( r, &prev );
            SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_ADD );
            SDL_RenderGeometry( r, nullptr, verts.data( ), (int)verts.size( ),
                                idx.data( ), (int)idx.size( ) );
            SDL_SetRenderDrawBlendMode( r, prev );
        }
        g_flashes.clear( );
    }

    // Drop shadows: alpha-blended dark soft discs drawn UNDER the sprite layer (over terrain).
    static void DrawShadows( SDL_Renderer* r, int ulX, int ulY )
    {
        if ( g_shadows.empty( ) )
            return;
        static std::vector<SDL_Vertex> verts; static std::vector<int> idx;
        verts.clear( ); idx.clear( );
        for ( const Disc& d : g_shadows )
            AppendDiscFan( verts, idx, d.cx - ulX, d.cy - ulY, d.rx, d.ry,
                           d.r, d.g, d.b, d.aCenter );
        if ( !verts.empty( ) )
        {
            SDL_BlendMode prev = SDL_BLENDMODE_BLEND;
            SDL_GetRenderDrawBlendMode( r, &prev );
            SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
            SDL_RenderGeometry( r, nullptr, verts.data( ), (int)verts.size( ),
                                idx.data( ), (int)idx.size( ) );
            SDL_SetRenderDrawBlendMode( r, prev );
        }
        g_shadows.clear( );
    }

    static bool ZLess( const Sprite* a, const Sprite* b )
    {
        if ( a->sortY != b->sortY ) return a->sortY < b->sortY;
        if ( a->sortX != b->sortX ) return a->sortX < b->sortX;
        // Same z-order key: draw STRUCTURES (building/bridge/tree) UNDER the mobile/effect
        // layer (vehicle/projectile/fire/smoke), so an animated effect on a re-captured
        // (growing) building stays on top instead of z-fighting it via the unstable seq.
        if ( a->isStructure != b->isStructure ) return a->isStructure;
        return a->seq < b->seq;
    }

    // Build the z-sorted list of ALL on-screen sprites (and prune fully off-screen
    // entries from the store). Used by the full-emit path.
    static void BuildOrderAll( std::vector<const Sprite*>& order, int ulX, int ulY, int vpW, int vpH )
    {
        order.clear( );
        order.reserve( g_sprites.size( ) );
        for ( auto it = g_sprites.begin( ); it != g_sprites.end( ); )
        {
            int bx, by, bw, bh; SprBox( it->second, bx, by, bw, bh );
            int sx = bx - ulX, sy = by - ulY;
            if ( sx + bw < -64 || sx > vpW + 64 || sy + bh < -64 || sy > vpH + 64 )
                { it = g_sprites.erase( it ); continue; }
            order.push_back( &it->second ); ++it;
        }
        std::sort( order.begin( ), order.end( ), ZLess );
    }

    void Submit( SDL_Renderer* r, int ulX, int ulY, int vpW, int vpH )
    {
        if ( !r || vpW <= 0 || vpH <= 0 )
            return;

        // S2.4: render the sprite layer into a PERSISTENT RT (g_rt) and composite it onto
        // the caller's target. A FULL capture (or a pan/resize) rebuilds the whole layer;
        // an INCREMENTAL capture only clears + re-emits the dirty unit regions (cur ∪ prev),
        // so the thousands of static trees are not re-sorted/re-emitted every frame — the
        // rest of g_rt persists. g_sprites always holds every sprite (trees persist across
        // incremental frames), so a full emit is always complete.
        bool needNew = ( !g_rt || g_rtW != vpW || g_rtH != vpH );
        if ( needNew )
        {
            if ( g_rt ) SDL_DestroyTexture( g_rt );
            g_rt = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, vpW, vpH );
            if ( g_rt ) SDL_SetTextureBlendMode( g_rt, SDL_BLENDMODE_BLEND );
            g_rtW = vpW; g_rtH = vpH;
        }

        static std::vector<const Sprite*> order;
        if ( !g_rt )   // alloc failed → fall back to direct emit onto the target
        {
            BuildOrderAll( order, ulX, ulY, vpW, vpH );
            DrawShadows( r, ulX, ulY );  // under the sprites (over terrain)
            EmitOrder( r, order, ulX, ulY );
            DrawFlashes( r, ulX, ulY );  // impact pops, additive, on top
            DrawTrails( r, ulX, ulY );   // tracers on top of the sprite layer
            g_dirty = false; g_lastUlX = ulX; g_lastUlY = ulY; g_accumValid = false;
            return;
        }

        bool full = g_captureWasFull || needNew || ulX != g_lastUlX || ulY != g_lastUlY;

        SDL_Texture* prevTarget = SDL_GetRenderTarget( r );
        SDL_Rect     savedVp; SDL_RenderGetViewport( r, &savedVp );
        SDL_SetRenderTarget( r, g_rt );
        SDL_RenderSetViewport( r, nullptr );

        if ( full )
        {
            SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_NONE );
            SDL_SetRenderDrawColor( r, 0, 0, 0, 0 );
            SDL_RenderClear( r );
            BuildOrderAll( order, ulX, ulY, vpW, vpH );
            SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
            EmitOrder( r, order, ulX, ulY );
        }
        else
        {
            // Coalesce the dirty unit regions (view → screen = -UL) into a few rects.
            static std::vector<SDL_Rect> rects; rects.clear( );
            auto addRect = [&]( const SDL_Rect& vr ) {
                SDL_Rect sr = { vr.x - ulX, vr.y - ulY, vr.w, vr.h };
                for ( SDL_Rect& e : rects )
                    if ( RectsOverlap( sr.x, sr.y, sr.w, sr.h, e.x - 8, e.y - 8, e.w + 16, e.h + 16 ) )
                    {
                        int x1 = __max( e.x + e.w, sr.x + sr.w ), y1 = __max( e.y + e.h, sr.y + sr.h );
                        e.x = __min( e.x, sr.x ); e.y = __min( e.y, sr.y );
                        e.w = x1 - e.x; e.h = y1 - e.y;
                        return;
                    }
                rects.push_back( sr );
            };
            for ( const SDL_Rect& vr : g_dirtyCur )  addRect( vr );
            for ( const SDL_Rect& vr : g_dirtyPrev ) addRect( vr );

            // Tight dirty rects multiply the rect count, so a per-rect all-sprites scan would
            // blow up. Bin every sprite into a screen-space uniform grid ONCE, then each rect
            // queries only its overlapping cells. Result is identical to the brute scan (same
            // overlapping set, z-sorted) — pure speedup, no visual change.
            const int kCell = 96, kGM = 256;            // cell size / off-screen sprite margin
            const int gnx = ( vpW + 2 * kGM ) / kCell + 1;
            const int gny = ( vpH + 2 * kGM ) / kCell + 1;
            static std::vector<std::vector<const Sprite*>> grid;
            if ( (int)grid.size( ) < gnx * gny ) grid.resize( gnx * gny );
            for ( int i = 0; i < gnx * gny; ++i ) grid[ i ].clear( );
            {
                Perf::ScopeCounter _cg( "spr.t.grid" );
                for ( auto& kv : g_sprites )
                {
                    int bx, by, bw, bh; SprBox( kv.second, bx, by, bw, bh );
                    int sx = bx - ulX + kGM, sy = by - ulY + kGM;
                    int cx0 = sx / kCell, cx1 = ( sx + bw ) / kCell;
                    int cy0 = sy / kCell, cy1 = ( sy + bh ) / kCell;
                    if ( cx1 < 0 || cy1 < 0 || cx0 >= gnx || cy0 >= gny ) continue;
                    cx0 = __max( 0, cx0 ); cy0 = __max( 0, cy0 );
                    cx1 = __min( gnx - 1, cx1 ); cy1 = __min( gny - 1, cy1 );
                    for ( int cy = cy0; cy <= cy1; ++cy )
                        for ( int cx = cx0; cx <= cx1; ++cx )
                            grid[ cy * gnx + cx ].push_back( &kv.second );
                }
            }

            Perf::CounterAdd( "spr.full", 0 );
            Perf::CounterAdd( "spr.rects", (int)rects.size( ) );
            int dbgScan = 0, dbgEmit = 0;
            for ( const SDL_Rect& R : rects )
            {
                SDL_RenderSetClipRect( r, &R );
                SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_NONE );
                SDL_SetRenderDrawColor( r, 0, 0, 0, 0 );
                SDL_RenderFillRect( r, &R );          // clear the region (transparent)
                // Gather sprites overlapping R from the grid cells it spans (z-sorted) so the
                // local depth order (a unit between a tree's rear & front) stays exact.
                order.clear( );
                {
                    Perf::ScopeCounter _cs( "spr.t.scan" );
                    int cx0 = ( R.x + kGM ) / kCell, cx1 = ( R.x + R.w + kGM ) / kCell;
                    int cy0 = ( R.y + kGM ) / kCell, cy1 = ( R.y + R.h + kGM ) / kCell;
                    cx0 = __max( 0, cx0 ); cy0 = __max( 0, cy0 );
                    cx1 = __min( gnx - 1, cx1 ); cy1 = __min( gny - 1, cy1 );
                    for ( int cy = cy0; cy <= cy1; ++cy )
                        for ( int cx = cx0; cx <= cx1; ++cx )
                        {
                            for ( const Sprite* s : grid[ cy * gnx + cx ] )
                            {
                                int bx, by, bw, bh; SprBox( *s, bx, by, bw, bh );
                                if ( RectsOverlap( bx - ulX, by - ulY, bw, bh, R.x, R.y, R.w, R.h ) )
                                    order.push_back( s );
                            }
                            dbgScan += (int)grid[ cy * gnx + cx ].size( );
                        }
                    std::sort( order.begin( ), order.end( ), ZLess );
                    order.erase( std::unique( order.begin( ), order.end( ) ), order.end( ) );  // dedup multi-cell
                }
                dbgEmit += (int)order.size( );
                SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
                { Perf::ScopeCounter _ce( "spr.t.emit" );
                  EmitOrder( r, order, ulX, ulY ); }      // GPU clips to R
            }
            SDL_RenderSetClipRect( r, nullptr );
            Perf::CounterAdd( "spr.scan", dbgScan );
            Perf::CounterAdd( "spr.emit", dbgEmit );
        }
        if ( full ) Perf::CounterAdd( "spr.full", 1 );

        SDL_SetRenderTarget( r, prevTarget );
        SDL_RenderSetViewport( r, &savedVp );

        DrawShadows( r, ulX, ulY );   // under the sprite layer, over terrain (fresh each present)
        SDL_RenderCopy( r, g_rt, nullptr, nullptr );   // composite the layer onto the target

        DrawFlashes( r, ulX, ulY );   // impact pops, additive, on top
        DrawTrails( r, ulX, ulY );    // tracers drawn fresh on top, never cached in g_rt

        g_dirty = false; g_lastUlX = ulX; g_lastUlY = ulY; g_accumValid = false;
    }

    void DirtyNewFrame( )
    {
        g_dirtyPrev.swap( g_dirtyCur );   // last frame's rects carry one more frame
        g_dirtyCur.clear( );
    }

    int SpriteCount( ) { return (int)g_sprites.size( ); }

    void DirtyAddRect( int vx, int vy, int w, int h )
    {
        if ( w > 0 && h > 0 )
            g_dirtyCur.push_back( SDL_Rect{ vx, vy, w, h } );
    }

    int DirtyRectCount( )
    {
        return (int)( g_dirtyCur.size( ) + g_dirtyPrev.size( ) );
    }

    void InvalidateTextures( )
    {
        if ( g_atlas ) { SDL_DestroyTexture( g_atlas ); g_atlas = nullptr; }
        g_atlasMap.clear( );
        g_atlasW = g_atlasH = 0;
        g_shelfX = g_shelfY = g_shelfH = 0;
        g_sprites.clear( );
        if ( g_rt ) { SDL_DestroyTexture( g_rt ); g_rt = nullptr; g_rtW = g_rtH = 0; }
        g_dirty = true; g_lastUlX = g_lastUlY = INT_MIN;
        g_accumValid = false;
        g_inFrame = false;
        g_trails.clear( );
        g_flashes.clear( );
        g_shadows.clear( );
    }
}
