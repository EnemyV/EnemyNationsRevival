//---------------------------------------------------------------------------
// enterrain reference renderer: the game's GPU terrain pass (SDL2Terrain::Render)
// rebuilt every frame over a plain HexGrid, using the SAME shared code for
// everything that decides pixels:
//   tile selection (SelectTile), projection (HexContentCorners), row order,
//   the land layer (transparent target, premultiplied, 4-vertex quads),
//   the live WATER layer + water/water seam bands + the shore's feather bands
//   (EmitWaterHex / EmitFeatherHex / BuildWaterGeom / BakeWater),
//   the slope-shade target + half-res blur, and the composite order
//   (water, land, shade MOD).
// Not reproduced (game-only): fog of war, the retained-mesh / pan / zoom caches and
// gesture preview, the far underlay, build ghosts, tree sprites, the per-direction tile
// memo (the tester resolves tiles at the current frame; the game memoizes them).
//---------------------------------------------------------------------------
#include "EnTerrain.h"
#include "EnTerrainLayers.h"

#include <SDL.h>
#include <algorithm>

namespace enterrain
{

void RenderScratch::Release()
{
    SDL_Texture* all[] = { shade, half, land, water };
    for ( SDL_Texture* t : all ) if ( t ) SDL_DestroyTexture( t );
    shade = half = land = water = nullptr;
    w = h = 0;
}

static inline int Wrap( int v, int n ) { return ( ( v % n ) + n ) % n; }

static int AltDraw( const HexGrid& g, int x, int y )
{
    int a = g.Alt( Wrap( x, g.Width() ), Wrap( y, g.Height() ) );
    return a > SeaLevel ? a : SeaLevel;
}

static void CornerAlts( const HexGrid& g, int hx, int hy, int alt[4] )
{
    alt[0] = AltDraw( g, hx,     hy );
    alt[1] = AltDraw( g, hx + 1, hy );
    alt[2] = AltDraw( g, hx + 1, hy + 1 );
    alt[3] = AltDraw( g, hx,     hy + 1 );
}

static HexTileQuery Query( const HexGrid& g, int hx, int hy )
{
    HexTileQuery q;
    int x = Wrap( hx, g.Width() ), y = Wrap( hy, g.Height() );
    q.type = g.Type( x, y );
    q.index = g.Index( x, y );
    q.growStage = g.GrowStage( x, y );
    q.hx = x; q.hy = y;
    return q;
}

void GridContentBounds( const HexGrid& g, int dir, int& minX, int& minY, int& maxX, int& maxY )
{
    minX = minY = 1 << 30; maxX = maxY = -( 1 << 30 );
    for ( int y = 0; y < g.Height(); ++y )
        for ( int x = 0; x < g.Width(); ++x )
        {
            int alt[4]; Pt c[4];
            CornerAlts( g, x, y, alt );
            HexContentCorners( x, y, alt, dir, c );
            for ( int k = 0; k < 4; ++k )
            {
                minX = std::min( minX, c[k].x ); maxX = std::max( maxX, c[k].x );
                minY = std::min( minY, c[k].y ); maxY = std::max( maxY, c[k].y );
            }
        }
}

typedef std::vector<std::pair<SDL_Texture*, std::vector<SDL_Vertex>>> Batches;

static std::vector<SDL_Vertex>& Bucket( Batches& b, SDL_Texture* t )
{
    for ( auto& e : b ) if ( e.first == t ) return e.second;
    b.emplace_back( t, std::vector<SDL_Vertex>() );
    return b.back().second;
}

// Draw 4-vertex quads with the game's index pattern {0,1,3, 1,2,3}.
static void DrawQuads( SDL_Renderer* r, SDL_Texture* t, const std::vector<SDL_Vertex>& v, std::vector<int>& idx )
{
    int nq = (int)v.size() / 4;
    if ( nq <= 0 ) return;
    while ( (int)idx.size() < nq * 6 )
    {
        int b = (int)idx.size() / 6 * 4;
        for ( int k = 0; k < 6; ++k ) idx.push_back( b + kQuadIdx[k] );
    }
    SDL_RenderGeometry( r, t, v.data(), nq * 4, idx.data(), nq * 6 );
}

namespace
{
// Water-layer band record (same layout the game keeps: e0,e1,n0,n1).
struct Band { int animIdx; Pt p[4]; SDL_FPoint uv[4]; };

struct GridNbr
{
    const HexGrid& g; const TileSet& ts; int hx, hy, dir; unsigned frame;
    HexTileQuery q[4];
    bool Get( int e, int& t, int& i )
    {
        q[e] = Query( g, hx + kNbrDX[e], hy + kNbrDY[e] );
        t = q[e].type; i = q[e].index;
        return true;
    }
    const Tile* TileOf( int e ) { return SelectTile( ts, q[e], dir, frame ); }
};

struct Sink
{
    const Pt* pts;
    std::vector<Pt>& waterPos; std::vector<int>& waterAnimOf; std::vector<Band>& bands;
    Batches& rowFeather;
    void WaterDiamond( int animIdx )
    {
        for ( int k = 0; k < 4; ++k ) waterPos.push_back( pts[k] );
        waterAnimOf.push_back( animIdx );
    }
    void WaterBand( int animIdx, const EdgeBand& b )
    {
        Band wb; wb.animIdx = animIdx;
        wb.p[0] = pts[b.c0];                              wb.uv[0] = b.u0;
        wb.p[1] = pts[b.c1];                              wb.uv[1] = b.u1;
        wb.p[2] = Pt{ (int)b.i0x, (int)b.i0y };           wb.uv[2] = b.iu0;
        wb.p[3] = Pt{ (int)b.i1x, (int)b.i1y };           wb.uv[3] = b.iu1;
        bands.push_back( wb );
    }
    void LandBand( const Tile*, SDL_Texture* ntex, const EdgeBand& b )
    {
        SDL_Vertex v[4];
        LandBandVerts( pts, b, v );
        std::vector<SDL_Vertex>& fvb = Bucket( rowFeather, ntex );
        for ( int k = 0; k < 4; ++k ) fvb.push_back( v[k] );
    }
};
} // namespace

static SDL_Texture* MakeTarget( SDL_Renderer* r, int w, int h, bool premul )
{
    SDL_Texture* t = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, std::max( 1, w ), std::max( 1, h ) );
    if ( !t ) return nullptr;
    if ( premul ) SetPremulBlend( t ); else SDL_SetTextureBlendMode( t, SDL_BLENDMODE_NONE );
    SDL_SetTextureScaleMode( t, SDL_ScaleModeLinear );
    return t;
}

int RenderGrid( SDL_Renderer* r, const TileSet& ts, const HexGrid& g,
                const Camera& cam, const RenderOptions& opt, RenderScratch& sc )
{
    if ( !r || g.Width() <= 0 || g.Height() <= 0 )
        return 0;
    const int dir = cam.dir & 3, zoom = std::max( 0, std::min( NUM_ZOOMS - 1, cam.zoom ) );
    int outW = 0, outH = 0;
    SDL_GetRendererOutputSize( r, &outW, &outH );
    if ( outW <= 0 || outH <= 0 )
        return 0;

    if ( sc.w != outW || sc.h != outH || !sc.land )
    {
        sc.Release();
        sc.land  = MakeTarget( r, outW, outH, true );
        sc.water = MakeTarget( r, outW, outH, true );
        sc.shade = MakeTarget( r, outW, outH, false );
        sc.half  = MakeTarget( r, outW / 2, outH / 2, false );
        sc.w = outW; sc.h = outH;
        if ( !sc.land || !sc.water || !sc.shade || !sc.half ) { sc.Release(); return 0; }
    }

    // View-hex range covering the whole grid (map corners, padded).
    int vx0 = 1 << 30, vx1 = -( 1 << 30 ), vy0 = 1 << 30, vy1 = -( 1 << 30 );
    const int cx[4] = { 0, g.Width() - 1, 0, g.Width() - 1 };
    const int cy[4] = { 0, 0, g.Height() - 1, g.Height() - 1 };
    for ( int i = 0; i < 4; ++i )
    {
        int vx, vy; MapToViewHex( cx[i], cy[i], dir, vx, vy );
        vx0 = std::min( vx0, vx ); vx1 = std::max( vx1, vx );
        vy0 = std::min( vy0, vy ); vy1 = std::max( vy1, vy );
    }
    vx0 -= 2; vx1 += 2; vy0 -= 2; vy1 += 2;

    std::vector<WaterAnim> anims;
    WaterAnimResolver res( anims );
    auto resolveWater  = [&]( int t, int v ) { return res.Water( ts, t, v ); };
    auto resolveCoast  = [&]( int F ) { return res.Coast( ts, F, dir ); };
    auto resolveStatic = [&]( const Tile* t ) { return res.Static( t ); };

    std::vector<Pt> waterPos; std::vector<int> waterAnimOf; std::vector<Band> bands;
    std::vector<SDL_Vertex> shadeVerts;
    std::vector<int> idx;
    Batches rowBase, rowFeather;
    const SDL_Color white = { 255, 255, 255, 255 };
    int drawn = 0;

    SDL_Texture* prevTarget = SDL_GetRenderTarget( r );
    SDL_SetRenderTarget( r, sc.land );
    SDL_SetRenderDrawColor( r, 0, 0, 0, 0 );    // TRANSPARENT: water/coast are holes here
    SDL_RenderClear( r );
    SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );

    for ( int vy = vy0; vy <= vy1; ++vy )
    {
        for ( int vx = vx0; vx <= vx1; ++vx )
        {
            int hx, hy;
            ViewHexToMap( vx, vy, true, dir, hx, hy );
            if ( hx < 0 || hy < 0 || hx >= g.Width() || hy >= g.Height() )
                continue;

            int alt[4]; Pt cpts[4], pts[4];
            CornerAlts( g, hx, hy, alt );
            HexContentCorners( hx, hy, alt, dir, cpts );
            int minX = 1 << 30, maxX = -( 1 << 30 ), minY = 1 << 30, maxY = -( 1 << 30 );
            for ( int k = 0; k < 4; ++k )
            {
                pts[k].x = ( cpts[k].x >> zoom ) - cam.ulX;
                pts[k].y = ( cpts[k].y >> zoom ) - cam.ulY;
                minX = std::min( minX, pts[k].x ); maxX = std::max( maxX, pts[k].x );
                minY = std::min( minY, pts[k].y ); maxY = std::max( maxY, pts[k].y );
            }
            if ( maxX < 0 || minX > outW || maxY < 0 || minY > outH )
                continue;

            HexTileQuery q = Query( g, hx, hy );
            const int type = q.type;
            if ( type < 0 || type >= TerrNumTypes )
                continue;
            const Tile*  tile = SelectTile( ts, q, dir, opt.frame );
            SDL_Texture* tex  = TileTexAnyZoom( tile, zoom );
            if ( !tex && !IsAnimatedTile( type ) )
                continue;   // land hex with no art at any zoom (animated water/coast proceed)
            ++drawn;

            float bL = 1.0f, bR = 1.0f;
            if ( tile && tile->shade )
            {
                bL = TriBrightness( alt[0], alt[1], alt[2], alt[3], true );
                bR = TriBrightness( alt[0], alt[1], alt[2], alt[3], false );
            }

            // Land layer base quad (L,T,R,B); road transposes U/V (bDrawVert).
            bool bTranspose = tile && tile->drawVert && type == TerrRoad;
            SDL_FPoint uv[4];
            if ( bTranspose ) { uv[0] = { 0.5f, 0.0f }; uv[1] = { 0.0f, 0.5f }; uv[2] = { 0.5f, 1.0f }; uv[3] = { 1.0f, 0.5f }; }
            else              { uv[0] = { 0.0f, 0.5f }; uv[1] = { 0.5f, 0.0f }; uv[2] = { 1.0f, 0.5f }; uv[3] = { 0.5f, 1.0f }; }
            SDL_Vertex v[4];
            for ( int k = 0; k < 4; ++k )
            {
                v[k].position = { (float)pts[k].x, (float)pts[k].y };
                v[k].tex_coord = uv[k]; v[k].color = white;
            }
            if ( !IsAnimatedTile( type ) )
            {
                std::vector<SDL_Vertex>& vb = Bucket( rowBase, tex );
                for ( int k = 0; k < 4; ++k ) vb.push_back( v[k] );
            }

            GridNbr nbr{ g, ts, hx, hy, dir, opt.frame, {} };
            Sink    sink{ pts, waterPos, waterAnimOf, bands, rowFeather };

            // Water layer: open-water / shore diamonds + water<->water seam bands.
            EmitWaterHex( type, q.index, pts, dir, resolveWater, resolveCoast, nbr, sink );

            if ( opt.shade && tile && tile->shade )
            {
                auto sv = [&]( int k, float b ) {
                    SDL_Vertex s; s.position = v[k].position; s.tex_coord = { 0, 0 };
                    Uint8 gr = (Uint8)std::min( 255, (int)( b * 255.0f ) ); s.color = { gr, gr, gr, 255 };
                    shadeVerts.push_back( s );
                };
                sv( 0, bL ); sv( 1, bL ); sv( 3, bL );
                sv( 1, bR ); sv( 2, bR ); sv( 3, bR );
            }

            if ( opt.feather )
                EmitFeatherHex( type, q.index, tile, pts, dir, zoom, resolveWater, resolveStatic, nbr, sink );
        }
        // this row's base, then its feather (back-to-front)
        for ( auto& b : rowBase )    DrawQuads( r, b.first, b.second, idx );
        for ( auto& b : rowFeather ) DrawQuads( r, b.first, b.second, idx );
        rowBase.clear(); rowFeather.clear();
    }

    // Slope shade: white = no darkening; overwrite (front wins), then the half-res blur.
    SDL_SetRenderTarget( r, sc.shade );
    SDL_SetRenderDrawColor( r, 255, 255, 255, 255 );
    SDL_RenderClear( r );
    SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_NONE );
    if ( !shadeVerts.empty() )
        SDL_RenderGeometry( r, nullptr, shadeVerts.data(), (int)shadeVerts.size(), nullptr, 0 );
    SDL_SetRenderTarget( r, sc.half );
    SDL_RenderCopy( r, sc.shade, nullptr, nullptr );
    SDL_SetRenderTarget( r, sc.shade );
    SDL_RenderCopy( r, sc.half, nullptr, nullptr );
    SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );

    // Water layer: grouped by anim, baked at the current wave frame (game frame / 6).
    std::vector<std::vector<SDL_Vertex>> diamGeom, bandGeom;
    BuildWaterGeom( anims.size(), waterPos.data(), waterAnimOf.data(), waterAnimOf.size(), bands, diamGeom, bandGeom );
    const bool haveWater = !waterAnimOf.empty();
    if ( haveWater )
        BakeWater( r, sc.water, anims, diamGeom, bandGeom, opt.frame / 6, zoom );

    // Composite (game order): water UNDER the land layer, then shade (MOD).
    SDL_SetRenderTarget( r, prevTarget );
    if ( haveWater ) SDL_RenderCopy( r, sc.water, nullptr, nullptr );
    SDL_RenderCopy( r, sc.land, nullptr, nullptr );
    if ( opt.shade )
    {
        SDL_SetTextureBlendMode( sc.shade, SDL_BLENDMODE_MOD );
        SDL_RenderCopy( r, sc.shade, nullptr, nullptr );
        SDL_SetTextureBlendMode( sc.shade, SDL_BLENDMODE_NONE );
    }
    return drawn;
}

} // namespace enterrain
