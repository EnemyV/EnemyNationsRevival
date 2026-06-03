#include "stdafx.h"
#include "SDL2Terrain.h"
#include "base.h"      // CAnimAtr, CViewHexCoord, CHexCoord
#include "terrain.h"   // CHex, theMap
#include "terrain.inl"

#include <SDL.h>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "thirdparty/stb_image.h"

#include <fstream>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>   // FindFirstFile / GetFileAttributes (project isn't C++17)
#endif

std::unordered_map<std::string, SDL2Terrain::Tile> SDL2Terrain::s_tiles;
std::unordered_map<std::string, const SDL2Terrain::Tile*> SDL2Terrain::s_byType;
std::unordered_map<std::string, const SDL2Terrain::Tile*> SDL2Terrain::s_byTypeVar;
SDL_Renderer* SDL2Terrain::s_renderer = nullptr;
bool          SDL2Terrain::s_loaded   = false;

// CHex terrain-type enum (terrain.h) → baked tile type name. forest has no
// terrain art (trees are sprites) → empty = no tile.
static const char* const kTypeName[] = {
    "city", "desert", "", "lake", "hill", "mountain", "ocean", "plain",
    "river", "road", "rough", "swamp", "coastline", "fields", "resources"
};
static const int kNumTypeNames = (int)( sizeof( kTypeName ) / sizeof( kTypeName[0] ) );

// Road facing (CHex::r_*) → (base source-dir, rotation). MakeRotated permutes
// the 4 direction views, so a rotated facing = the base road art shown from
// view (m_iDir - rot) mod 4. Base road sprites load at engine indices 0,2,6,10,11
// ↔ source road dirs 0,1,2,3,4 (sprtinit.cpp:114-120, ChangeToRoad:2574).
static const int kRoadSrcDir[12] = { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 4 };
// Corner facings (6-9 = r_l_*) come out 90° rotated vs straights/T (the corner
// base art orientation differs), so they carry an extra +1 quarter-turn.
static const int kRoadRot[12]    = { 0, 1, 0, 1, 2, 3, 1, 2, 3, 0, 0, 0 };
static const char* const kDirPrefix[4] = { "aa", "ac", "ae", "ag" };

static void LogTerrain( const std::string& msg )
{
    std::ofstream log( "SDL2Terrain.log", std::ios::app );
    if ( log.is_open() )
        log << msg << std::endl;
}

// terrain type render flags (mirror tools/terrainbake + terrain.cpp CHex::Draw).
static bool TypeShade( const std::string& t )
{
    return !( t == "river" || t == "coastline" || t == "swamp" ||
              t == "ocean" || t == "resources" || t == "lake" );
}
static bool TypeDrawVert( const std::string& t ) { return t == "road" || t == "resources"; }
static bool TypeTransparent( const std::string& t )
{
    return t == "road" || t == "coastline" || t == "river" || t == "resources";
}

std::string SDL2Terrain::MakeKey( const std::string& type, int variant,
                                  const std::string& stem )
{
    return type + "_" + std::to_string( variant ) + "_" + stem;
}

// Candidate locations for the baked PNG set, relative to the run dir (the game's
// CWD is d:\Enemy Nations) and the dev tree. First hit wins.
static bool FileExists( const std::string& p )
{
    return GetFileAttributesA( p.c_str() ) != INVALID_FILE_ATTRIBUTES;
}

static std::string FindAssetDir()
{
    const char* candidates[] = {
        "data/terrain_gpu",
        "src/enations_latest/data/terrain_gpu",
        "enations_latest/data/terrain_gpu",
        "../src/enations_latest/data/terrain_gpu",
    };
    for ( const char* c : candidates )
        if ( FileExists( std::string( c ) + "/plain_0_aa010000_z0.png" ) )
            return c;
    return std::string();
}

// Filename convention from terrainbake: <type>_<variant>_<stem>_z<n>.png
// e.g. "plain_0_aa010000_z0.png", "coastline_0_ac00h000_z3.png". type and stem
// contain no underscores, so 4 underscore fields split cleanly.
static bool ParseName( const std::string& name, std::string& type, int& variant,
                       std::string& stem, int& zoom )
{
    size_t p1 = name.find( '_' );
    size_t p2 = ( p1 == std::string::npos ) ? p1 : name.find( '_', p1 + 1 );
    size_t p3 = ( p2 == std::string::npos ) ? p2 : name.find( '_', p2 + 1 );
    if ( p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos )
        return false;

    type           = name.substr( 0, p1 );
    std::string sV = name.substr( p1 + 1, p2 - p1 - 1 );
    stem           = name.substr( p2 + 1, p3 - p2 - 1 );  // full source stem
    std::string sZ = name.substr( p3 + 1 );               // "z<n>"

    if ( stem.empty() || sZ.size() < 2 || sZ[0] != 'z' )
        return false;

    variant = atoi( sV.c_str() );
    zoom    = atoi( sZ.c_str() + 1 );
    return zoom >= 0 && zoom < SDL2Terrain::NUM_ZOOMS;
}

static SDL_Texture* LoadPng( SDL_Renderer* r, const std::string& path, int& outW, int& outH )
{
    int w = 0, h = 0, comp = 0;
    stbi_uc* px = stbi_load( path.c_str(), &w, &h, &comp, 4 );  // force RGBA
    if ( !px )
        return nullptr;

    // stb gives tightly-packed RGBA bytes → SDL_PIXELFORMAT_ABGR8888 (LE byte
    // order R,G,B,A). Upload to a static texture; alpha-blend for the keyed
    // overlay types; nearest sampling for T2 parity (bilinear comes at T3).
    SDL_Texture* tex = SDL_CreateTexture( r, SDL_PIXELFORMAT_ABGR8888,
                                          SDL_TEXTUREACCESS_STATIC, w, h );
    if ( tex )
    {
        SDL_UpdateTexture( tex, nullptr, px, w * 4 );
        SDL_SetTextureBlendMode( tex, SDL_BLENDMODE_BLEND );
        // T3: bilinear sampling (was nearest) — smooths the 1996 blockiness. The
        // Square corner-fill keeps the inscribed-diamond UVs from bleeding the
        // colour key, and per-zoom LOD picks the right base size before filtering.
        SDL_SetTextureScaleMode( tex, SDL_ScaleModeLinear );
        outW = w; outH = h;
    }
    stbi_image_free( px );
    return tex;
}

int SDL2Terrain::Load( SDL_Renderer* renderer )
{
    if ( !renderer )
        return 0;
    if ( s_loaded && s_renderer == renderer )
        return (int)s_tiles.size();
    if ( s_loaded )
        Unload();  // renderer changed — rebuild

    std::string dir = FindAssetDir();
    if ( dir.empty() )
    {
        LogTerrain( "ERROR: baked terrain PNG set not found (looked for data/terrain_gpu etc.)" );
        return 0;
    }
    LogTerrain( "Loading terrain tiles from: " + dir );

    s_renderer = renderer;
    int files = 0;

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA( ( dir + "\\*.png" ).c_str(), &fd );
    if ( h != INVALID_HANDLE_VALUE )
    {
        do
        {
            if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                continue;

            std::string name     = fd.cFileName;    // e.g. coastline_0_ac00h000_z2.png
            std::string baseName = name.substr( 0, name.find_last_of( '.' ) );

            std::string type, tstem;
            int variant = 0, zoom = 0;
            if ( !ParseName( baseName, type, variant, tstem, zoom ) )
                continue;

            int tw = 0, th = 0;
            SDL_Texture* tex = LoadPng( renderer, dir + "\\" + name, tw, th );
            if ( !tex )
            {
                LogTerrain( "WARN: failed to load " + name );
                continue;
            }

            Tile& tile = s_tiles[MakeKey( type, variant, tstem )];
            tile.tex[zoom] = tex;
            if ( zoom == 0 ) { tile.w = tw; tile.h = th; }
            tile.shade       = TypeShade( type );
            tile.drawVert    = TypeDrawVert( type );
            tile.transparent = TypeTransparent( type );
            if ( s_byType.find( type ) == s_byType.end() )
                s_byType[type] = &tile;   // first-seen representative for first-light
            // Per-(type,variant): prefer the "aa" base-direction tile (first shape).
            // For single-tile-per-variant types this is the exact tile; for
            // multi-shape types (coastline/road/river/swamp) it's the base shape.
            if ( zoom == 0 )
            {
                std::string tvKey = type + "_" + std::to_string( variant );
                auto        tvIt  = s_byTypeVar.find( tvKey );
                if ( tvIt == s_byTypeVar.end() ||
                     ( tstem.rfind( "aa", 0 ) == 0 && tvIt->second->tex[0] &&
                       /* prefer aa over a previously-stored non-aa */ true ) )
                {
                    // store on first sight, or upgrade to an aa-dir tile
                    if ( tvIt == s_byTypeVar.end() || tstem.rfind( "aa", 0 ) == 0 )
                        s_byTypeVar[tvKey] = &tile;
                }
            }
            ++files;
        } while ( FindNextFileA( h, &fd ) );
        FindClose( h );
    }

    s_loaded = true;
    LogTerrain( "Loaded " + std::to_string( s_tiles.size() ) + " tiles (" +
                std::to_string( files ) + " PNGs)." );
    return (int)s_tiles.size();
}

void SDL2Terrain::Unload()
{
    for ( auto& kv : s_tiles )
        for ( SDL_Texture* t : kv.second.tex )
            if ( t ) SDL_DestroyTexture( t );
    s_tiles.clear();
    s_byType.clear();
    s_byTypeVar.clear();
    s_renderer = nullptr;
    s_loaded   = false;
}

const SDL2Terrain::Tile* SDL2Terrain::GetDefaultForType( const std::string& type )
{
    auto it = s_byType.find( type );
    return it == s_byType.end() ? nullptr : it->second;
}

const SDL2Terrain::Tile* SDL2Terrain::Get( const std::string& type, int variant,
                                           const std::string& stem )
{
    auto it = s_tiles.find( MakeKey( type, variant, stem ) );
    return it == s_tiles.end() ? nullptr : &it->second;
}

// T4: continuous slope brightness for one triangle, matching the engine's
// CSpriteDIB::TerrainGetShadeIndex (sprite.cpp:378) but WITHOUT the 8-level
// quantization — light from the right, SHADE_CONTRAST=14, range ~[0.6,1.3]
// (the original Shade() lightness span). Flat ground → 1.0 (neutral).
// Slope brightness for one triangle (engine TerrainGetShadeIndex curve, but
// continuous). Tuned a touch brighter than the original: neutral ~1.06, a
// higher floor (less aggressive dark dips) — user wanted brighter terrain.
// Fog dim is applied separately by the caller.
static const float kFogDim = 0.58f;   // invisible (out-of-sight) hexes — dim, but
                                      // between the old too-light and too-dark (darker side)

static float TriBrightness( int z0, int z1, int z2, int z3, bool left )
{
    const int SC = 14;
    int iDY1, iDY2, kk;
    if ( left )  { iDY1 = SC * ( z3 - z0 ); iDY2 = SC * ( z1 - z0 ); kk = -iDY1 - iDY2 + 128; }
    else         { iDY1 = SC * ( z1 - z2 ); iDY2 = SC * ( z3 - z2 ); kk =  iDY1 + iDY2 + 128; }

    float shadeF;
    if ( kk < 0 )
        shadeF = 0.0f;
    else
    {
        float den = (float)( iDY1 * iDY1 + iDY2 * iDY2 + 128 * 64 );
        shadeF = den > 0.0f ? ( (float)kk * (float)kk * 2.0f / den ) : 4.0f;
        if ( shadeF > 7.0f ) shadeF = 7.0f;
    }
    float b = 1.06f + 0.09f * ( shadeF - 4.0f );    // neutral 1.06 (brighter)
    return b < 0.80f ? 0.80f : ( b > 1.40f ? 1.40f : b );
}

const SDL2Terrain::Tile* SDL2Terrain::TileForHex( CHex* phex, int iDir )
{
    if ( !phex )
        return nullptr;
    CTerrainSprite* psprite = phex->GetSprite( );
    int             type    = psprite ? psprite->GetID( ) : -1;
    if ( type < 0 || type >= kNumTypeNames )
        return nullptr;

    const char* typeName = kTypeName[type];
    if ( !typeName[0] )                                   // forest → plain ground
        return GetDefaultForType( "plain" );

    if ( type == CHex::road )
    {
        int F = psprite->GetIndex( );
        if ( F < 0 || F > 11 ) F = CHex::r_x;
        int srcDir  = kRoadSrcDir[F];
        int viewDir = ( ( iDir - kRoadRot[F] ) % 4 + 4 ) % 4;
        const Tile* t = Get( "road", srcDir, std::string( kDirPrefix[viewDir] ) + "010000" );
        return t ? t : GetDefaultForType( "road" );
    }

    int  variant = psprite->GetIndex( );
    auto it      = s_byTypeVar.find( std::string( typeName ) + "_" + std::to_string( variant ) );
    return ( it != s_byTypeVar.end() ) ? it->second : GetDefaultForType( typeName );
}

// T5: which terrain types feather (soft-blend) at their boundaries. Matches the
// engine's CHex::Draw exclusions EXACTLY: road / city / resources never feather.
// Everything else (incl. fields) feathers — verified against the original; the
// original also feathers same-type/different-variant (INOUT) and coastline IN/OUT.
static bool Featherable( int type )
{
    return type >= 0 && type != CHex::road && type != CHex::city && type != CHex::resources;
}

void SDL2Terrain::Render( SDL_Renderer* r, const CAnimAtr& aa )
{
    if ( !s_loaded || !r )
        return;

    const CSize ws    = aa.m_dibwnd.GetWinSize();
    const int   zoom  = ( aa.m_iZoom < 0 ) ? 0 : ( aa.m_iZoom >= NUM_ZOOMS ? NUM_ZOOMS - 1 : aa.m_iZoom );
    if ( ws.cx <= 0 || ws.cy <= 0 )
        return;

    // Visible hex window (mirror CGameMap::UpdateRect's seed), with margin.
    CHexCoord     hexTL = aa._WindowToHex( CPoint( 0, 0 ) );
    CHexCoord     hexTR = aa._WindowToHex( CPoint( ws.cx - 1, 0 ) );
    CViewHexCoord ptTL( hexTL );
    CViewHexCoord ptTR( hexTR );
    const int iTopY   = __min( ptTL.y, ptTR.y ) - 2;
    const int iLeftX  = ptTL.x - 2;
    const int iRightX = ptTR.x + 2;
    const int margin  = ( 64 << 3 ) >> zoom;           // tall-hex overscan (max alt)

    // Batch quads by texture (SDL_RenderGeometry binds one texture per call).
    std::unordered_map<SDL_Texture*, std::vector<SDL_Vertex>> batches;
    // T5: edge-feather triangles (neighbour tiles bled into shared edges),
    // drawn in a 2nd pass over the opaque base so boundaries soft-blend.
    std::unordered_map<SDL_Texture*, std::vector<SDL_Vertex>> featherBatches;

    for ( int y = iTopY;; ++y )
    {
        bool rowAny = false;
        for ( int x = iLeftX; x <= iRightX; ++x )
        {
            CHexCoord hexcoord( CViewHexCoord( x, y ), TRUE );
            CHex*     phex = theMap.GetHex( hexcoord );
            if ( !phex )
                continue;

            // Screen positions of the 4 diamond vertices in (L,T,R,B) order
            // (MapToWindowHex applies the per-m_iDir corner reorder). Plus the
            // world-corner altitudes for slope shading (T4).
            CPoint pts[4];
            aa.MapToWindowHex( hexcoord, pts );   // pts[0]=Left 1=Top 2=Right 3=Bottom
            CMapLoc3D c3d[4];
            hexcoord.GetWorldHex( c3d );

            // viewport cull (with altitude overscan upward)
            int minX = pts[0].x, maxX = pts[0].x, minY = pts[0].y, maxY = pts[0].y;
            for ( int i = 1; i < 4; ++i )
            {
                minX = __min( minX, pts[i].x ); maxX = __max( maxX, pts[i].x );
                minY = __min( minY, pts[i].y ); maxY = __max( maxY, pts[i].y );
            }
            if ( maxX < 0 || minX > ws.cx || maxY < 0 || minY > ws.cy + margin )
                continue;
            rowAny = true;

            CTerrainSprite* psprite = phex->GetSprite( );
            int             type    = psprite ? psprite->GetID( ) : -1;
            if ( type < 0 || type >= kNumTypeNames )
                continue;

            const Tile* tile = TileForHex( phex, aa.m_iDir );  // forest/road/variant
            if ( !tile || !tile->tex[zoom] )
                continue;

            SDL_Texture* tex = tile->tex[zoom];
            std::vector<SDL_Vertex>& vb = batches[tex];

            // T4 slope shading: shaded land gets per-triangle slope brightness;
            // water/coast stay full colour. T6 fog is applied per-vertex below.
            float bL = 1.0f, bR = 1.0f;
            if ( tile->shade )
            {
                int z0 = c3d[0].m_fixZ.Round(), z1 = c3d[1].m_fixZ.Round();
                int z2 = c3d[2].m_fixZ.Round(), z3 = c3d[3].m_fixZ.Round();
                bL = TriBrightness( z0, z1, z2, z3, true );
                bR = TriBrightness( z0, z1, z2, z3, false );
            }

            // T6 SOFT fog-of-war: per-corner visibility = average of the nearby
            // hexes meeting at that corner, so the dim blends smoothly across hex
            // edges (Gouraud-interpolated) instead of hard diamond steps.
            // fog[c] in [kFogDim, 1]: 1 = fully in sight, kFogDim = fully fogged.
            int  hx = hexcoord.X( ), hy = hexcoord.Y( );
            auto visAt = [&]( int dx, int dy ) -> float {
                CHexCoord hc( hx + dx, hy + dy ); hc.Wrap( );
                CHex* h = theMap.GetHex( hc );
                return ( h && h->GetVisibility( ) ) ? 1.0f : 0.0f;
            };
            float vS = visAt( 0, 0 );
            float fog[4];
            fog[0] = kFogDim + ( 1.0f - kFogDim ) * ( vS + visAt(-1,0) + visAt(0,-1) + visAt(-1,-1) ) * 0.25f;
            fog[1] = kFogDim + ( 1.0f - kFogDim ) * ( vS + visAt( 1,0) + visAt(0,-1) + visAt( 1,-1) ) * 0.25f;
            fog[2] = kFogDim + ( 1.0f - kFogDim ) * ( vS + visAt( 1,0) + visAt(0, 1) + visAt( 1, 1) ) * 0.25f;
            fog[3] = kFogDim + ( 1.0f - kFogDim ) * ( vS + visAt(-1,0) + visAt(0, 1) + visAt(-1, 1) ) * 0.25f;

            auto grayCol = []( float b ) -> SDL_Color {
                Uint8 g = (Uint8)__min( 255, (int)( b * 255.0f ) );
                SDL_Color c; c.r = c.g = c.b = g; c.a = 255; return c;
            };

            // Diamond-vertex UVs: the tile's edge-midpoints land on the 4 diamond
            // vertices (TerrainDrawQuad maps U=screen-x from Left, V=screen-y from
            // Top). road transposes U/V (bDrawVert / TerrainDrawQuadVert, for
            // vertical-seam continuity); resources are a single upright icon that
            // maps directly (transposing it rotates the cart sideways).
            bool bTranspose = tile->drawVert && type == CHex::road;
            SDL_FPoint uvL, uvT, uvR, uvB;
            if ( bTranspose )
            {
                uvL = { 0.5f, 0.0f }; uvT = { 0.0f, 0.5f };
                uvR = { 0.5f, 1.0f }; uvB = { 1.0f, 0.5f };
            }
            else
            {
                uvL = { 0.0f, 0.5f }; uvT = { 0.5f, 0.0f };
                uvR = { 1.0f, 0.5f }; uvB = { 0.5f, 1.0f };
            }

            SDL_Vertex vL, vT, vR, vB;
            vL.position = { (float)pts[0].x, (float)pts[0].y }; vL.tex_coord = uvL;  // Left
            vT.position = { (float)pts[1].x, (float)pts[1].y }; vT.tex_coord = uvT;  // Top
            vR.position = { (float)pts[2].x, (float)pts[2].y }; vR.tex_coord = uvR;  // Right
            vB.position = { (float)pts[3].x, (float)pts[3].y }; vB.tex_coord = uvB;  // Bottom

            // Left tri (corners 0,1,3) shade bL; right (1,2,3) shade bR. Per-vertex
            // colour = slope brightness * corner fog → smooth (Gouraud) fog edges.
            vL.color = grayCol( bL * fog[0] );
            vT.color = grayCol( bL * fog[1] );
            vB.color = grayCol( bL * fog[3] );
            vb.push_back( vL ); vb.push_back( vT ); vb.push_back( vB );
            vT.color = grayCol( bR * fog[1] );
            vR.color = grayCol( bR * fog[2] );
            vB.color = grayCol( bR * fog[3] );
            vb.push_back( vT ); vb.push_back( vR ); vb.push_back( vB );

            // T5 edge feather: for each diamond edge bordering a DIFFERENT
            // featherable type, bleed the neighbour's tile in from that edge with
            // a soft alpha gradient (≈half at the edge → 0 at the diamond centre).
            // Both hexes do this mutually → a smooth INOUT-style blend, replacing
            // the original checkerboard dither. road/city/resources never feather.
            if ( Featherable( type ) )
            {
                static const SDL_FPoint fuv[4] = { {0.f,0.5f}, {0.5f,0.f}, {1.f,0.5f}, {0.5f,1.f} };
                static const int nbrDX[4] = { 0, 1, 0, -1 };
                static const int nbrDY[4] = { -1, 0, 1, 0 };
                const Uint8 kFeatherA = 205;                       // ~0.8 blend at the edge (strong)
                float cxF = ( pts[0].x + pts[1].x + pts[2].x + pts[3].x ) * 0.25f;
                float cyF = ( pts[0].y + pts[1].y + pts[2].y + pts[3].y ) * 0.25f;
                Uint8 gc  = (Uint8)__min( 255, (int)( ( fog[0]+fog[1]+fog[2]+fog[3] ) * 0.25f * 255.0f ) );

                for ( int e = 0; e < 4; ++e )
                {
                    CHexCoord nhc( hx + nbrDX[e], hy + nbrDY[e] ); nhc.Wrap( );
                    CHex* pn = theMap.GetHex( nhc );
                    if ( !pn ) continue;
                    CTerrainSprite* ns = pn->GetSprite( );
                    int ntype = ns ? ns->GetID( ) : -1;
                    if ( !Featherable( ntype ) ) continue;
                    const Tile* ntile = TileForHex( pn, aa.m_iDir );
                    // Blend whenever the neighbour's TILE differs (variant OR type)
                    // — softens variant boundaries too, not just type boundaries.
                    if ( !ntile || !ntile->tex[zoom] || ntile == tile ) continue;

                    int   c0 = e, c1 = ( e + 1 ) & 3;
                    Uint8 g0 = (Uint8)__min( 255, (int)( fog[c0] * 255.0f ) );
                    Uint8 g1 = (Uint8)__min( 255, (int)( fog[c1] * 255.0f ) );

                    SDL_Vertex fa, fb, fc;
                    fa.position = { (float)pts[c0].x, (float)pts[c0].y }; fa.tex_coord = fuv[c0]; fa.color = { g0, g0, g0, kFeatherA };
                    fb.position = { (float)pts[c1].x, (float)pts[c1].y }; fb.tex_coord = fuv[c1]; fb.color = { g1, g1, g1, kFeatherA };
                    fc.position = { cxF, cyF };                          fc.tex_coord = { 0.5f, 0.5f }; fc.color = { gc, gc, gc, 0 };

                    std::vector<SDL_Vertex>& fvb = featherBatches[ntile->tex[zoom]];
                    fvb.push_back( fa ); fvb.push_back( fb ); fvb.push_back( fc );
                }
            }
        }

        // Stop once we're below the viewport and rows have gone empty.
        if ( !rowAny && y > iTopY + 3 )
            break;
        if ( y > iTopY + 4 * ( ws.cy / __max( 1, ( 16 >> zoom ) ) + 8 ) )
            break;  // hard safety bound
    }

    for ( auto& b : batches )
        SDL_RenderGeometry( r, b.first, b.second.data(), (int)b.second.size(), nullptr, 0 );
    // T5: feather pass on top (textures are BLEND mode, vertex alpha gradient).
    for ( auto& b : featherBatches )
        SDL_RenderGeometry( r, b.first, b.second.data(), (int)b.second.size(), nullptr, 0 );
}
