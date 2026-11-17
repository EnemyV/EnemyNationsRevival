//---------------------------------------------------------------------------
// enterrain: baked tile set, tile-selection rules, shade/feather rules, projection.
// Moved verbatim (logic-wise) out of enations_latest/src/SDL2Terrain.cpp and
// CAnimAtr (terrain.cpp); the game now calls these.
//---------------------------------------------------------------------------
#include "EnTerrain.h"

#include <SDL.h>
#include <cstdlib>
#include <cstring>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "thirdparty/stb_image.h"

#ifdef _WIN32
#include <windows.h>   // FindFirstFile (project isn't C++17 on MSVC: no <filesystem>)
#else
#include <dirent.h>
#include <fnmatch.h>
#endif

// Same per-TU optimisation the game applied to SDL2Terrain.cpp (hot path under Debug).
#if defined( _MSC_VER )
#pragma runtime_checks( "", off )
#pragma optimize( "gt", on )
#endif

namespace enterrain
{

// CHex terrain-type enum (terrain.h) -> baked tile type name. forest has no
// terrain art (trees are sprites) -> empty = no tile.
static const char* const kTypeName[] = {
    "city", "desert", "", "lake", "hill", "mountain", "ocean", "plain",
    "river", "road", "rough", "swamp", "coastline", "fields", "resources"
};
static const int kNumTypeNames = (int)( sizeof( kTypeName ) / sizeof( kTypeName[0] ) );
static_assert( kNumTypeNames == TerrNumTypes, "kTypeName must cover every TerrainType" );

const int kCornerSlot[4][4] = { { 0, 1, 2, 3 }, { 3, 0, 1, 2 }, { 2, 3, 0, 1 }, { 1, 2, 3, 0 } };

const char* TypeName( int type )
{
    return ( type >= 0 && type < kNumTypeNames ) ? kTypeName[type] : "";
}

static int TypeNameToInt( const std::string& n )
{
    for ( int i = 0; i < kNumTypeNames; ++i )
        if ( n == kTypeName[i] ) return i;
    return -1;
}

SDL_Texture* TileTexAnyZoom( const Tile* t, int zoom )
{
    if ( !t ) return nullptr;
    SDL_Texture* tex = ( zoom >= 0 && zoom < NUM_ZOOMS ) ? t->tex[zoom] : nullptr;
    for ( int z = 0; !tex && z < NUM_ZOOMS; ++z ) tex = t->tex[z];
    return tex;
}

// Road facing (CHex::r_*) -> (base source-dir, rotation). MakeRotated permutes
// the 4 direction views, so a rotated facing = the base road art shown from
// view (m_iDir - rot) mod 4. Base road sprites load at engine indices 0,2,6,10,11
// <-> source road dirs 0,1,2,3,4 (sprtinit.cpp:114-120, ChangeToRoad:2574).
static const int kRoadSrcDir[12] = { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 4 };
// Corner facings (6-9 = r_l_*) come out 90 deg rotated vs straights/T (the corner
// base art orientation differs), so they carry an extra +1 quarter-turn.
static const int kRoadRot[12]    = { 0, 1, 0, 1, 2, 3, 1, 2, 3, 0, 0, 0 };
static const char* const kDirPrefix[4] = { "aa", "ac", "ae", "ag" };

// Coastline facing (0-38) -> (source coastlne dir 0-11, rotation). Base sprites
// load at indices 0,4,8,12,13,17,21,25,26,30,34,38 <- coastlne/0..11 (TERRAIN.MIF),
// with MakeRotated filling the gaps (sprtinit.cpp:123-151). Coastline tiles are
// single-shape per dir at damage "a"; not bDrawVert, not shaded.
static const int kCoastSrcDir[39] = {
    0, 0, 0, 0,  1, 1, 1, 1,  2, 2, 2, 2,  3,  4, 4, 4, 4,  5, 5, 5, 5,
    6, 6, 6, 6,  7,  8, 8, 8, 8,  9, 9, 9, 9,  10, 10, 10, 10,  11 };
static const int kCoastRot[39] = {
    0, 1, 2, 3,  0, 1, 2, 3,  0, 1, 2, 3,  0,  0, 1, 2, 3,  0, 1, 2, 3,
    0, 1, 2, 3,  0,  0, 1, 2, 3,  0, 1, 2, 3,  0, 1, 2, 3,  0 };

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

std::string TileSet::MakeKey( const std::string& type, int variant, const std::string& stem )
{
    return type + "_" + std::to_string( variant ) + "_" + stem;
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
    return zoom >= 0 && zoom < NUM_ZOOMS;
}

static SDL_Texture* LoadPng( SDL_Renderer* r, const std::string& path, int& outW, int& outH )
{
    int w = 0, h = 0, comp = 0;
    stbi_uc* px = stbi_load( path.c_str(), &w, &h, &comp, 4 );  // force RGBA
    if ( !px )
        return nullptr;

    // stb gives tightly-packed RGBA bytes -> SDL_PIXELFORMAT_ABGR8888 (LE byte
    // order R,G,B,A). Upload to a static texture; alpha-blend for the keyed
    // overlay types; nearest sampling for T2 parity.
    SDL_Texture* tex = SDL_CreateTexture( r, SDL_PIXELFORMAT_ABGR8888,
                                          SDL_TEXTUREACCESS_STATIC, w, h );
    if ( tex )
    {
        SDL_UpdateTexture( tex, nullptr, px, w * 4 );
        SDL_SetTextureBlendMode( tex, SDL_BLENDMODE_BLEND );
        // NEAREST sampling: the tile art is an iso DIAMOND inscribed in a square, so
        // the square's corners hold non-tile pixels. Bilinear filtering at the diamond
        // EDGES samples those corner pixels and bleeds them in -> visible seams
        // between tiles. Nearest samples only the exact texel (no cross-edge bleed),
        // matching the original 1996 look, which abutted tiles seamlessly.
        SDL_SetTextureScaleMode( tex, SDL_ScaleModeNearest );
        outW = w; outH = h;
    }
    stbi_image_free( px );
    return tex;
}

// "*.png" in `dir`, in the platform's enumeration order (the game's FindFirstFile order:
// Win32 on Windows; readdir + case-insensitive fnmatch, as win32_compat's shim, on POSIX).
// Order matters: byType keeps the FIRST-seen tile of each type.
static void ListPngs( const std::string& dir, std::vector<std::string>& out )
{
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA( ( dir + "\\*.png" ).c_str(), &fd );
    if ( h == INVALID_HANDLE_VALUE )
        return;
    do
    {
        if ( !( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
            out.push_back( fd.cFileName );
    } while ( FindNextFileA( h, &fd ) );
    FindClose( h );
#else
#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD 0
#endif
    DIR* dp = opendir( dir.c_str() );
    if ( !dp )
        return;
    while ( struct dirent* de = readdir( dp ) )
    {
        if ( fnmatch( "*.png", de->d_name, FNM_CASEFOLD ) != 0 )
            continue;
        if ( de->d_type == DT_DIR )
            continue;
        out.push_back( de->d_name );
    }
    closedir( dp );
#endif
}

void TileSet::swap( TileSet& o )
{
    using std::swap;
    swap( tiles, o.tiles );
    swap( byType, o.byType );
    swap( byTypeVar, o.byTypeVar );
    for ( int i = 0; i < TerrNumTypes; ++i ) swap( typeVarPtr[i], o.typeVarPtr[i] );
}

int TileSet::LoadDir( SDL_Renderer* renderer, const std::string& dir,
                      const std::function<void( const std::string& )>& warn )
{
    if ( !renderer || dir.empty() )
        return 0;
    int files = 0;

    // Tracks the stem currently chosen for each "type_variant" representative, so
    // the choice is deterministic (lexicographically smallest = "aa00a000" = dir
    // aa, letter a) rather than dependent on enumeration order. The engine always
    // draws view (xiDir, damage 0) for non-field terrain (terrain.cpp CHex::Draw),
    // i.e. letter 'a'; multi-letter types reaching the general byTypeVar path
    // (river/swamp) must resolve to that 'a' tile.
    std::unordered_map<std::string, std::string> tvStem;

    std::vector<std::string> names;
    ListPngs( dir, names );
    for ( const std::string& name : names )
    {
        std::string baseName = name.substr( 0, name.find_last_of( '.' ) );

        std::string type, tstem;
        int variant = 0, zoom = 0;
        if ( !ParseName( baseName, type, variant, tstem, zoom ) )
            continue;

        int tw = 0, th = 0;
        // '/' join on every platform: stbi_load fopen()s directly, and Windows accepts it.
        SDL_Texture* tex = LoadPng( renderer, dir + "/" + name, tw, th );
        if ( !tex )
        {
            if ( warn ) warn( "WARN: failed to load " + name );
            continue;
        }

        Tile& tile = tiles[MakeKey( type, variant, tstem )];
        tile.tex[zoom] = tex;
        if ( zoom == 0 ) { tile.w = tw; tile.h = th; }
        tile.shade       = TypeShade( type );
        tile.drawVert    = TypeDrawVert( type );
        tile.transparent = TypeTransparent( type );
        if ( byType.find( type ) == byType.end() )
            byType[type] = &tile;   // first-seen representative
        // Per-(type,variant) representative: deterministically the tile with the
        // lexicographically smallest stem ("aa00a000" = direction aa, letter a = the
        // engine's damage-0 view).
        if ( zoom == 0 )
        {
            std::string tvKey = type + "_" + std::to_string( variant );
            auto        stemIt = tvStem.find( tvKey );
            if ( stemIt == tvStem.end() || tstem < stemIt->second )
            {
                byTypeVar[tvKey] = &tile;
                tvStem[tvKey]    = tstem;
                int ti = TypeNameToInt( type );          // integer-indexed mirror
                if ( ti >= 0 )
                {
                    if ( (int)typeVarPtr[ti].size() <= variant )
                        typeVarPtr[ti].resize( variant + 1, nullptr );
                    typeVarPtr[ti][variant] = &tile;
                }
            }
        }
        ++files;
    }
    return files;
}

void TileSet::Unload()
{
    for ( auto& kv : tiles )
        for ( SDL_Texture* t : kv.second.tex )
            if ( t ) SDL_DestroyTexture( t );
    tiles.clear();
    byType.clear();
    byTypeVar.clear();
    for ( int i = 0; i < TerrNumTypes; ++i ) typeVarPtr[i].clear();
}

const Tile* TileSet::GetDefaultForType( const std::string& type ) const
{
    auto it = byType.find( type );
    return it == byType.end() ? nullptr : it->second;
}

const Tile* TileSet::Get( const std::string& type, int variant, const std::string& stem ) const
{
    auto it = tiles.find( MakeKey( type, variant, stem ) );
    return it == tiles.end() ? nullptr : &it->second;
}

const Tile* TileSet::GetTypeVar( int type, int variant ) const
{
    if ( type >= 0 && type < TerrNumTypes && variant >= 0 &&
         variant < (int)typeVarPtr[type].size() )
        return typeVarPtr[type][variant];
    return nullptr;
}

// Water wave animation: ocean/lake/coastline/river bake 8 frames (letters a-h),
// swamp 5 (a-e). Match the engine: water sprites advance one frame every Time()=6
// game-frames (DrawSimpleAnimation nHold = GetAnim(FRONT_1,0)->Time()).
char WaterFrameLetter( unsigned gameFrame, int nFrames )
{
    const int kWaterHold = 6;
    int f = ( (int)gameFrame / kWaterHold ) % nFrames;
    return (char)( 'a' + f );
}

int CoastSrcDir( int F )
{
    if ( F < 0 || F > 38 ) F = 0;
    return kCoastSrcDir[F];
}

// The CAMERA term must SUBTRACT iDir (the camera rotates the world the opposite way
// the sprite view index advances). With (+iDir) the shore came out 180 deg wrong at
// camera dirs 1 and 3. kCoastRot keeps its (+) sign (tile-rotation term).
int CoastViewDir( int F, int iDir )
{
    if ( F < 0 || F > 38 ) F = 0;
    return ( ( kCoastRot[F] - iDir ) % 4 + 4 ) % 4;
}

const Tile* CoastFrameTile( const TileSet& ts, int F, int iDir, char fr )
{
    int srcDir  = CoastSrcDir( F );
    int viewDir = CoastViewDir( F, iDir );
    std::string stem = std::string( kDirPrefix[viewDir] ) + "00" + fr + "000";
    const Tile* t = ts.Get( "coastline", srcDir, stem );
    if ( !t ) t = ts.Get( "coastline", srcDir, std::string( kDirPrefix[viewDir] ) + "00a000" );
    // ROTATION-SYMMETRIC variants (3/7/11 - single F index, no MakeRotated gaps)
    // ship only the "aa" view: the original draws that one view at every camera dir.
    if ( !t ) t = ts.Get( "coastline", srcDir, std::string( "aa00" ) + fr + "000" );
    if ( !t ) t = ts.Get( "coastline", srcDir, "aa00a000" );
    return t;
}

const Tile* WaterFrameTile( const TileSet& ts, const char* typeName, int variant, char fr )
{
    const Tile* t = ts.Get( typeName, variant, std::string( "aa00" ) + fr + "000" );
    if ( !t ) t = ts.Get( typeName, variant, "aa00a000" );        // static fallback
    return t;
}

const Tile* SelectTile( const TileSet& ts, const HexTileQuery& q, int iDir, unsigned gameFrame )
{
    const int type = q.type;
    if ( type < 0 || type >= kNumTypeNames )
        return nullptr;

    const char* typeName = kTypeName[type];
    if ( !typeName[0] )                                   // forest -> plain ground
        return ts.GetDefaultForType( "plain" );

    if ( type == TerrRoad )
    {
        int F = q.index;
        if ( F < 0 || F > 11 ) F = RoadX;
        int srcDir  = kRoadSrcDir[F];
        int viewDir = ( ( iDir - kRoadRot[F] ) % 4 + 4 ) % 4;
        // The under-construction PREVIEW road (r_path) is a non-directional site overlay:
        // only the dir-0 ("aa") variant is baked. Pin it to the dir-0 variant so the
        // preview shows the construction graphic at every rotation.
        if ( F == RoadPath ) viewDir = 0;
        const Tile* t = ts.Get( "road", srcDir, std::string( kDirPrefix[viewDir] ) + "010000" );
        // Rotation-symmetric road shapes (4-way junction r_x, srcDir 3; r_path, srcDir 4)
        // bake ONLY the "aa" view: fall back to this shape's own "aa" view first.
        if ( !t ) t = ts.Get( "road", srcDir, "aa010000" );
        return t ? t : ts.GetDefaultForType( "road" );
    }

    if ( type == TerrCoastline )
    {
        const Tile* t = CoastFrameTile( ts, q.index, iDir, WaterFrameLetter( gameFrame, 8 ) );  // animate foam
        return t ? t : ts.GetDefaultForType( "coastline" );
    }

    // Farm fields: the stored sprite is always (fields, variant 0); the visible
    // furrow ROTATION and crop GROW STAGE are NOT in the sprite index - the engine
    // derives them at draw time (CHex::Draw, terrain.cpp): furrow rotation =
    // (X*2+Y)&3 (coord-based patchwork, NOT camera dir), grow stage = GetGrowStage().
    // Stem encoding: first letter = stage (a/d/g = 0/1/2), second = direction
    // (a/c/e/g = 0/1/2/3).
    if ( type == TerrFields )
    {
        static const char kStageLetter[3] = { 'a', 'd', 'g' };
        static const char kRotLetter[4]   = { 'a', 'c', 'e', 'g' };
        int stage = q.growStage;
        if ( stage < 0 ) stage = 0; else if ( stage > 2 ) stage = 2;
        int rot = ( q.hx * 2 + q.hy ) & 3;
        std::string stem; stem += kStageLetter[stage]; stem += kRotLetter[rot]; stem += "010000";
        const Tile* t = ts.Get( "fields", q.index, stem );
        if ( !t ) t = ts.Get( "fields", q.index, "aa010000" );    // fallback: stage 0 / rot 0
        return t ? t : ts.GetDefaultForType( "fields" );
    }

    // Open-water types: single direction (aa), animated frames a-h / a-e.
    if ( IsOpenWater( type ) )
    {
        int  nFrames = ( type == TerrSwamp ) ? 5 : 8;
        const Tile* t = WaterFrameTile( ts, typeName, q.index, WaterFrameLetter( gameFrame, nFrames ) );
        return t ? t : ts.GetDefaultForType( typeName );
    }

    const Tile* t = ts.GetTypeVar( type, q.index );        // integer-indexed (no string alloc)
    return t ? t : ts.GetDefaultForType( typeName );
}

// Which terrain types feather (soft-blend) at their boundaries. Matches the
// engine's CHex::Draw exclusions EXACTLY: road / city / resources never feather.
bool Featherable( int type )
{
    return type >= 0 && type != TerrRoad && type != TerrCity && type != TerrResources;
}

// OPEN water (ocean/lake/river/swamp) never participates in the land edge feather.
// Coastline is NOT open water: it's the land-side transition tile.
bool IsOpenWater( int type )
{
    return type == TerrOcean || type == TerrLake || type == TerrRiver || type == TerrSwamp;
}

// Tiles whose foam is ANIMATED (open water + the coastline shore).
bool IsAnimatedTile( int type )
{
    return IsOpenWater( type ) || type == TerrCoastline;
}

bool FeatherToward( int type, int index, int ntype, int nindex )
{
    if ( !Featherable( ntype ) )
        return false;
    // Same type+variant = same tile = no band. fields excluded: its drawn tile also
    // depends on grow stage + coord rotation.
    if ( ntype == type && ntype != TerrFields && nindex == index )
        return false;
    // The COASTLINE feathers toward all its non-coast neighbours (land AND open water).
    // A NON-coast tile feathers land<->land only: never toward open water and never
    // ONTO a coastline neighbour (the coast feathers itself).
    if ( type == TerrCoastline )
        return ntype != TerrCoastline;
    return !( IsOpenWater( ntype ) || ntype == TerrCoastline );
}

// Slope brightness for one triangle, matching the engine's
// CSpriteDIB::TerrainGetShadeIndex (sprite.cpp) but WITHOUT the 8-level
// quantization: light from the right, SHADE_CONTRAST=14. Neutral ~1.06, a higher
// floor (less aggressive dark dips). Fog dim is applied separately.
float TriBrightness( int z0, int z1, int z2, int z3, bool left )
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

void HexContentCorners( int hx, int hy, const int altDraw[4], int iDir, Pt out[4] )
{
    // CHexCoord::GetWorldHex corner order (x,y world axes): 0=(x,y) 1=(x+1,y)
    // 2=(x+1,y+1) 3=(x,y+1); then CAnimAtr::MapToContentHex's per-dir slot reorder.
    const int mx = hx << HexHtPwr, my = hy << HexHtPwr;
    const int wx[4] = { mx, mx + HexHt, mx + HexHt, mx };
    const int wy[4] = { my, my, my + HexHt, my + HexHt };
    const int* slot = kCornerSlot[iDir & 3];
    for ( int i = 0; i < 4; ++i )
    {
        int x, y;
        WorldToContent( wx[i], wy[i], altDraw[i] << TerrainHtShift, iDir & 3, x, y );
        out[slot[i]].x = x;
        out[slot[i]].y = y;
    }
}

} // namespace enterrain
