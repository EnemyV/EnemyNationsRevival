//---------------------------------------------------------------------------
// enterrain: water-layer animation tables, bake, premultiplied blend. Moved from
// SDL2Terrain::Render (resolveWaterAnim / resolveCoastAnim / resolveStaticAnim lambdas,
// the s_waterRT bake, SetPremulBlend).
//---------------------------------------------------------------------------
#include "EnTerrainLayers.h"

#if defined( _MSC_VER )
#pragma runtime_checks( "", off )
#pragma optimize( "gt", on )
#endif

namespace enterrain
{

int WaterAnimResolver::Water( const TileSet& ts, int wtype, int wvar, Miss* miss )
{
    int key = ( wtype << 8 ) | ( wvar & 0xFF );
    auto it = m_waterKey.find( key );
    if ( it != m_waterKey.end( ) ) return it->second;
    WaterAnim wa;
    wa.nFrames = ( wtype == TerrSwamp ) ? 5 : 8;
    const char* tn = ( wtype >= 0 && wtype < TerrNumTypes ) ? TypeName( wtype ) : "ocean";
    int wMiss = 0;
    for ( int f = 0; f < wa.nFrames; ++f )
    {
        char fr = (char)( 'a' + f );
        const Tile* t = WaterFrameTile( ts, tn, wvar, fr );
        if ( !t ) ++wMiss;
        wa.frame[f] = t;
    }
    if ( miss ) { miss->computed = true; miss->nullFrames = wMiss; miss->nFrames = wa.nFrames; miss->typeName = tn; }
    int idx = (int)m_anims.size( );
    m_anims.push_back( wa );
    m_waterKey[key] = idx;
    return idx;
}

int WaterAnimResolver::Coast( const TileSet& ts, int F, int dir, Miss* miss )
{
    bool clamped = ( F < 0 || F > 38 );   // out-of-range facing = bad SHORE data (worldgen/placement)
    if ( clamped ) F = 0;
    if ( miss ) miss->clamped = clamped;
    auto it = m_coastKey.find( F );
    if ( it != m_coastKey.end( ) ) return it->second;
    WaterAnim wa; wa.nFrames = 8;
    int cMiss = 0;
    for ( int f = 0; f < 8; ++f )
    {
        char fr = (char)( 'a' + f );
        // Same lookup + fallback chain as SelectTile's coastline arm, incl. the
        // rotation-symmetric variants (3/7/11) that ship only the "aa" view.
        const Tile* t = CoastFrameTile( ts, F, dir, fr );
        if ( !t ) ++cMiss;
        wa.frame[f] = t;
    }
    if ( miss )
    {
        miss->computed = true; miss->nullFrames = cMiss; miss->nFrames = 8;
        miss->srcDir = CoastSrcDir( F ); miss->viewDir = CoastViewDir( F, dir );
    }
    int idx = (int)m_anims.size( );
    m_anims.push_back( wa );
    m_coastKey[F] = idx;
    return idx;
}

int WaterAnimResolver::Static( const Tile* t )
{
    auto it = m_staticKey.find( t );
    if ( it != m_staticKey.end( ) ) return it->second;
    WaterAnim wa; wa.nFrames = 1; wa.frame[0] = t;
    int idx = (int)m_anims.size( );
    m_anims.push_back( wa );
    m_staticKey[t] = idx;
    return idx;
}

void BakeWater( SDL_Renderer* r, SDL_Texture* target, const std::vector<WaterAnim>& anims,
                const std::vector<std::vector<SDL_Vertex>>& diamGeom,
                const std::vector<std::vector<SDL_Vertex>>& bandGeom,
                unsigned wbase, int wz,
                const std::function<void( int, const WaterAnim&, int )>& onMiss )
{
    SDL_Texture* pt = SDL_GetRenderTarget( r );
    SDL_Rect     vp; SDL_RenderGetViewport( r, &vp );
    SDL_SetRenderTarget( r, target );
    SDL_RenderSetViewport( r, nullptr );
    SDL_SetRenderDrawColor( r, 0, 0, 0, 0 );   // transparent: only water hexes get filled
    SDL_RenderClear( r );
    SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
    // Fallback texture for groups whose anim has no usable frame (missing art): the first
    // valid frame of any anim - drawing the WRONG water frame beats a permanent black hole.
    SDL_Texture* wFall = nullptr;
    for ( const WaterAnim& wa2 : anims )
        if ( wa2.nFrames > 0 && wa2.frame[0] && TileTexAnyZoom( wa2.frame[0], wz ) ) { wFall = TileTexAnyZoom( wa2.frame[0], wz ); break; }
    for ( size_t ai = 0; ai < diamGeom.size( ); ++ai )   // opaque water diamonds
    {
        const std::vector<SDL_Vertex>& wv = diamGeom[ai];
        if ( wv.empty( ) ) continue;
        const WaterAnim& wa = anims[ai];
        // Frame fallback ladder: this frame (any mip) -> frame 0 (any mip) -> global fallback.
        const Tile* wt = wa.nFrames > 0 ? wa.frame[ wbase % wa.nFrames ] : nullptr;
        SDL_Texture* wtex = TileTexAnyZoom( wt, wz );
        if ( !wtex && wa.nFrames > 0 ) wtex = TileTexAnyZoom( wa.frame[0], wz );
        if ( !wtex )
        {
            wtex = wFall;
            if ( onMiss ) onMiss( (int)ai, wa, (int)( wv.size( ) / 6 ) );
        }
        if ( !wtex ) continue;
        SDL_RenderGeometry( r, wtex, wv.data( ), (int)wv.size( ), nullptr, 0 );
    }
    for ( size_t ai = 0; ai < bandGeom.size( ); ++ai )   // then water<->water / shore bands
    {
        const std::vector<SDL_Vertex>& wv = bandGeom[ai];
        if ( wv.empty( ) ) continue;
        const WaterAnim& wa = anims[ai];
        const Tile* nt = wa.nFrames > 0 ? wa.frame[ wbase % wa.nFrames ] : nullptr;
        SDL_Texture* ntex = TileTexAnyZoom( nt, wz );
        if ( !ntex && wa.nFrames > 0 ) ntex = TileTexAnyZoom( wa.frame[0], wz );
        if ( !ntex ) ntex = wFall;
        if ( !ntex ) continue;
        SDL_RenderGeometry( r, ntex, wv.data( ), (int)wv.size( ), nullptr, 0 );
    }
    SDL_SetRenderTarget( r, pt );
    SDL_RenderSetViewport( r, &vp );
}

// PREMULTIPLIED composite: drawing with BLEND into a transparent-cleared target stores
// premultiplied pixels; blitting that with plain BLEND multiplies by alpha AGAIN and
// darkens partial-alpha texels into a black fringe along shorelines when scaled.
void SetPremulBlend( SDL_Texture* t )
{
    if ( !t ) return;
    SDL_BlendMode pm = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD );
    if ( SDL_SetTextureBlendMode( t, pm ) != 0 )
        SDL_SetTextureBlendMode( t, SDL_BLENDMODE_BLEND );
}

} // namespace enterrain
