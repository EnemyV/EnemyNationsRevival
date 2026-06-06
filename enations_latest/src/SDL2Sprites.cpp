#include "stdafx.h"
#include "SDL2Sprites.h"
#include "sprite.h"          // CSpriteDIB::DecodeToRGBA, NUM_ZOOM_LEVELS
#include "w22_settings.h"    // w22::GetProfileInt — [Advanced] GpuSprites flag

#include <SDL.h>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <climits>

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
        int      ax, ay, aw, ah; // atlas sub-rect (the FULL sprite's texture)
        int      vx, vy;         // structure UL (view space); FULL size == aw,ah
        int      cx, cy, cw, ch; // sprite-LOCAL visible band (construction wipe); full = 0,0,aw,ah
        int      qx[4], qy[4];   // vehicle quad verts (view space)
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
        Sprite s { };
        s.sortX = sortX; s.sortY = sortY; s.seq = ++g_seq; s.isVehicle = false;
        s.ax = loc.x; s.ay = loc.y; s.aw = loc.w; s.ah = loc.h;
        s.vx = vx; s.vy = vy;
        s.cx = clx; s.cy = cly; s.cw = clw; s.ch = clh;
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
        Sprite s { };
        s.sortX = sortX; s.sortY = sortY; s.seq = ++g_seq; s.isVehicle = true;
        s.ax = loc.x; s.ay = loc.y; s.aw = loc.w; s.ah = loc.h;
        for ( int i = 0; i < 4; ++i ) { s.qx[i] = vx[i]; s.qy[i] = vy[i]; }
        SprKey key { dib, vx[0], vy[0] };
        g_sprites[ key ] = s;
        if ( g_captureDynamic ) g_dynKeys.push_back( key );
        g_dirty = true;
        return true;
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

    static bool ZLess( const Sprite* a, const Sprite* b )
    {
        if ( a->sortY != b->sortY ) return a->sortY < b->sortY;
        if ( a->sortX != b->sortX ) return a->sortX < b->sortX;
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
            EmitOrder( r, order, ulX, ulY );
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

            for ( const SDL_Rect& R : rects )
            {
                SDL_RenderSetClipRect( r, &R );
                SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_NONE );
                SDL_SetRenderDrawColor( r, 0, 0, 0, 0 );
                SDL_RenderFillRect( r, &R );          // clear the region (transparent)
                // Re-emit ALL sprites overlapping R (z-sorted) so the local depth order
                // (a unit between a tree's rear & front) stays exactly correct.
                order.clear( );
                for ( auto& kv : g_sprites )
                {
                    int bx, by, bw, bh; SprBox( kv.second, bx, by, bw, bh );
                    if ( RectsOverlap( bx - ulX, by - ulY, bw, bh, R.x, R.y, R.w, R.h ) )
                        order.push_back( &kv.second );
                }
                std::sort( order.begin( ), order.end( ), ZLess );
                SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
                EmitOrder( r, order, ulX, ulY );      // GPU clips to R
            }
            SDL_RenderSetClipRect( r, nullptr );
        }

        SDL_SetRenderTarget( r, prevTarget );
        SDL_RenderSetViewport( r, &savedVp );
        SDL_RenderCopy( r, g_rt, nullptr, nullptr );   // composite the layer onto the target

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
    }
}
