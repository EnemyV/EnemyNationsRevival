#pragma once
//---------------------------------------------------------------------------
// enterrain: the GPU terrain's WATER layer and BLEND-BAND code, shared verbatim by
// the game (SDL2Terrain::Render) and the tester (enterrain::RenderGrid).
//
// Model (unchanged from the game): land tiles go to a transparent-cleared layer
// texture (premultiplied); open water AND the coastline shore are holes there and are
// drawn in a separate live WATER layer underneath (opaque diamonds with the current
// wave frame, then water<->water seam bands and the shore's feather bands). Per hex:
//   EmitWaterHex    - water/coast diamond + water<->water seam bands
//   EmitFeatherHex  - land<->land feather bands (land layer) and the shore's bands
//                     toward land and open water (water layer)
// then once per rebuild BuildWaterGeom groups the water geometry by animation and
// BakeWater draws it with the current wave frame.
//
// Templates so the hot per-hex path inlines into the game's loop (no call overhead),
// and so the game keeps its own point type (CPoint) and band record.
//---------------------------------------------------------------------------
#include "EnTerrain.h"
#include <SDL.h>
#include <unordered_map>
#include <vector>
#include <functional>

namespace enterrain
{

// Animation frame table for one water body (type,variant), one shore facing, or a
// static tile exposed as a 1-frame "anim" (the shore's land-side feather).
struct WaterAnim { const Tile* frame[8]; int nFrames; };

// Resolves animation tables into a (persistent) anims vector. The key maps live as
// long as this object: the game makes one per rebuild (so an incremental strip may
// append duplicate tables to a kept vector, exactly as before).
class WaterAnimResolver
{
public:
    struct Miss
    {
        int  nullFrames = 0;     // frames with no art (after the fallback ladder)
        int  nFrames    = 0;
        bool computed   = false; // false = served from the key cache
        bool clamped    = false; // coast only: facing F was out of 0..38
        int  srcDir = 0, viewDir = 0;   // coast only (diagnostics)
        const char* typeName = "";      // water only (diagnostics)
    };

    explicit WaterAnimResolver( std::vector<WaterAnim>& anims ) : m_anims( anims ) {}

    // Open water (ocean/lake/river/swamp): 8 frames (swamp 5), key (type,variant).
    int Water( const TileSet& ts, int wtype, int wvar, Miss* miss = nullptr );
    // Coastline shore facing F at camera dir: 8 foam frames, key F (dir fixed per rebuild).
    int Coast( const TileSet& ts, int F, int dir, Miss* miss = nullptr );
    // A static tile as a 1-frame anim.
    int Static( const Tile* t );

private:
    std::vector<WaterAnim>&             m_anims;
    std::unordered_map<int, int>         m_waterKey;
    std::unordered_map<int, int>         m_coastKey;
    std::unordered_map<const Tile*, int> m_staticKey;
};

static const unsigned char kWaterFeatherA = 135;   // water-layer band edge alpha (= land feather)

// Diamond-vertex UVs in screen-slot order L,T,R,B (non-transposed).
static const SDL_FPoint kDiamondUV[4] = { {0.f,0.5f}, {0.5f,0.f}, {1.f,0.5f}, {0.5f,1.f} };
// Index pattern for 4-corner quads (L,T,R,B or e0,e1,n1,n0): tris {0,1,3} {1,2,3}.
static const int kQuadIdx[6] = { 0, 1, 3, 1, 2, 3 };

// Band toward the diamond edge (c0,c1): the edge endpoints plus inset points kBand of
// the way toward the centre, and their UVs. P = any point type with int .x/.y.
struct EdgeBand
{
    int        c0, c1;
    float      i0x, i0y, i1x, i1y;   // inset points (float; the water layer truncates to int)
    SDL_FPoint u0, u1, iu0, iu1;
};

template <class P>
inline EdgeBand MakeEdgeBand( const P pts[4], int dir, int e )
{
    EdgeBand b;
    b.c0 = kCornerSlot[dir & 3][e];
    b.c1 = kCornerSlot[dir & 3][( e + 1 ) & 3];
    float cx = ( pts[0].x + pts[1].x + pts[2].x + pts[3].x ) * 0.25f;
    float cy = ( pts[0].y + pts[1].y + pts[2].y + pts[3].y ) * 0.25f;
    b.i0x = pts[b.c0].x + kBand * ( cx - pts[b.c0].x );
    b.i0y = pts[b.c0].y + kBand * ( cy - pts[b.c0].y );
    b.i1x = pts[b.c1].x + kBand * ( cx - pts[b.c1].x );
    b.i1y = pts[b.c1].y + kBand * ( cy - pts[b.c1].y );
    b.u0 = kDiamondUV[b.c0]; b.u1 = kDiamondUV[b.c1];
    b.iu0 = { b.u0.x + kBand*(0.5f-b.u0.x), b.u0.y + kBand*(0.5f-b.u0.y) };
    b.iu1 = { b.u1.x + kBand*(0.5f-b.u1.x), b.u1.y + kBand*(0.5f-b.u1.y) };
    return b;
}

// Land feather band quad (land layer): e0,e1,n1,n0, drawn with kQuadIdx.
template <class P>
inline void LandBandVerts( const P pts[4], const EdgeBand& b, SDL_Vertex out[4] )
{
    const Uint8 g = 255;   // full bright; fog dim is the overlay
    out[0].position = { (float)pts[b.c0].x, (float)pts[b.c0].y }; out[0].tex_coord = b.u0;  out[0].color = { g, g, g, kFeatherA };
    out[1].position = { (float)pts[b.c1].x, (float)pts[b.c1].y }; out[1].tex_coord = b.u1;  out[1].color = { g, g, g, kFeatherA };
    out[2].position = { b.i1x, b.i1y };                          out[2].tex_coord = b.iu1; out[2].color = { g, g, g, 0 };
    out[3].position = { b.i0x, b.i0y };                          out[3].tex_coord = b.iu0; out[3].color = { g, g, g, 0 };
}

// Per-hex neighbour access (duck-typed), e = 0..3 map neighbour (kNbrDX/kNbrDY):
//   bool Get( int e, int& type, int& index )   false = no hex
//   const Tile* TileOf( int e )                 resolved tile (caller memoizes)
// Sink (duck-typed):
//   void WaterDiamond( int animIdx )
//   void WaterBand( int animIdx, const EdgeBand& b )   (water layer; ints = (int)inset)
//   void LandBand( const Tile* ntile, SDL_Texture* ntex, const EdgeBand& b )

// Open water: its diamond + a seam band toward each DIFFERENT open-water body (anim).
// Coastline: its diamond (directional shore anim). Other types: nothing.
template <class P, class Nbr, class Sink, class ResolveWater, class ResolveCoast>
inline void EmitWaterHex( int type, int index, const P pts[4], int dir,
                          ResolveWater&& resolveWater, ResolveCoast&& resolveCoast,
                          Nbr& nbr, Sink& sink )
{
    if ( IsOpenWater( type ) )
    {
        int thisAnim = resolveWater( type, index );
        sink.WaterDiamond( thisAnim );
        for ( int e = 0; e < 4; ++e )
        {
            int ntype, nindex;
            if ( !nbr.Get( e, ntype, nindex ) ) continue;
            if ( !IsOpenWater( ntype ) ) continue;            // water<->water only
            int nAnim = resolveWater( ntype, nindex );
            if ( nAnim == thisAnim ) continue;                // same water body -> no band
            sink.WaterBand( nAnim, MakeEdgeBand( pts, dir, e ) );
        }
    }
    if ( type == TerrCoastline )
        sink.WaterDiamond( resolveCoast( index ) );
}

// Feather bands of a featherable, non-open-water hex (see FeatherToward). The shore's
// bands ride the WATER layer (over the opaque animated coast; the land layer has a
// transparent hole there that would darken a partial-alpha band into a black fringe):
// toward open water with the LIVE neighbour frames, toward land via a static anim.
template <class P, class Nbr, class Sink, class ResolveWater, class ResolveStatic>
inline void EmitFeatherHex( int type, int index, const Tile* tile, const P pts[4], int dir, int zoom,
                            ResolveWater&& resolveWater, ResolveStatic&& resolveStatic,
                            Nbr& nbr, Sink& sink )
{
    if ( !( Featherable( type ) && !IsOpenWater( type ) ) )
        return;
    for ( int e = 0; e < 4; ++e )
    {
        int ntype, nindex;
        if ( !nbr.Get( e, ntype, nindex ) ) continue;
        if ( !FeatherToward( type, index, ntype, nindex ) ) continue;
        const Tile*  ntile = nbr.TileOf( e );
        SDL_Texture* nftex = TileTexAnyZoom( ntile, zoom );
        if ( !ntile || !nftex || ntile == tile ) continue;
        EdgeBand b = MakeEdgeBand( pts, dir, e );
        if ( type == TerrCoastline )
            sink.WaterBand( IsOpenWater( ntype ) ? resolveWater( ntype, nindex ) : resolveStatic( ntile ), b );
        else
            sink.LandBand( ntile, nftex, b );
    }
}

// Water-layer geometry grouped by anim index (positions fixed; only the frame texture
// changes per wave tick). diamonds: n hexes, 4 corners each (L,T,R,B) at pos[i*4..];
// bands: records with .animIdx, .p[4] (e0,e1,n0,n1) and .uv[4].
template <class P, class Band>
void BuildWaterGeom( size_t nAnims, const P* pos, const int* animOf, size_t nHex,
                     const std::vector<Band>& bands,
                     std::vector<std::vector<SDL_Vertex>>& diamGeom,
                     std::vector<std::vector<SDL_Vertex>>& bandGeom )
{
    diamGeom.assign( nAnims, {} );
    bandGeom.assign( nAnims, {} );
    for ( size_t i = 0; i < nHex; ++i )
    {
        int ai = animOf[i];
        // Anim resolve can fail for odd variants (missing art / unmapped index). Skipping
        // would leave a PERMANENT transparent hole; fall back to anim 0 (open sea).
        if ( ai < 0 || ai >= (int)diamGeom.size( ) )
            ai = diamGeom.empty( ) ? -1 : 0;
        if ( ai < 0 ) continue;
        std::vector<SDL_Vertex>& wv = diamGeom[ai];
        const P* p = &pos[i * 4];
        SDL_Vertex a, b, c, d;
        a.position = { (float)p[0].x, (float)p[0].y }; a.tex_coord = kDiamondUV[0]; a.color = { 255,255,255,255 };
        b.position = { (float)p[1].x, (float)p[1].y }; b.tex_coord = kDiamondUV[1]; b.color = { 255,255,255,255 };
        c.position = { (float)p[2].x, (float)p[2].y }; c.tex_coord = kDiamondUV[2]; c.color = { 255,255,255,255 };
        d.position = { (float)p[3].x, (float)p[3].y }; d.tex_coord = kDiamondUV[3]; d.color = { 255,255,255,255 };
        wv.push_back( a ); wv.push_back( b ); wv.push_back( d );
        wv.push_back( b ); wv.push_back( c ); wv.push_back( d );
    }
    for ( const Band& wb : bands )
    {
        if ( wb.animIdx < 0 || wb.animIdx >= (int)bandGeom.size( ) ) continue;
        std::vector<SDL_Vertex>& wv = bandGeom[wb.animIdx];
        SDL_Vertex e0, e1, n0, n1;
        e0.position = { (float)wb.p[0].x, (float)wb.p[0].y }; e0.tex_coord = wb.uv[0]; e0.color = { 255,255,255,kWaterFeatherA };
        e1.position = { (float)wb.p[1].x, (float)wb.p[1].y }; e1.tex_coord = wb.uv[1]; e1.color = { 255,255,255,kWaterFeatherA };
        n0.position = { (float)wb.p[2].x, (float)wb.p[2].y }; n0.tex_coord = wb.uv[2]; n0.color = { 255,255,255,0 };
        n1.position = { (float)wb.p[3].x, (float)wb.p[3].y }; n1.tex_coord = wb.uv[3]; n1.color = { 255,255,255,0 };
        wv.push_back( e0 ); wv.push_back( e1 ); wv.push_back( n1 );
        wv.push_back( e0 ); wv.push_back( n1 ); wv.push_back( n0 );
    }
}

// Bake the water layer into `target` (cleared transparent) at wave frame `wbase`
// (= game frame / 6) with zoom `wz`'s art: opaque diamonds, then the bands. Restores
// the previous render target and viewport. onMiss(ai, anim, quads) is called when a
// diamond group has no frame art and draws with the global fallback frame.
void BakeWater( SDL_Renderer* r, SDL_Texture* target, const std::vector<WaterAnim>& anims,
                const std::vector<std::vector<SDL_Vertex>>& diamGeom,
                const std::vector<std::vector<SDL_Vertex>>& bandGeom,
                unsigned wbase, int wz,
                const std::function<void( int, const WaterAnim&, int )>& onMiss = nullptr );

// Premultiplied-alpha blend for the cached layer textures (falls back to BLEND).
void SetPremulBlend( SDL_Texture* t );

} // namespace enterrain
