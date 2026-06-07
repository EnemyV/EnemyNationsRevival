#include "stdafx.h"
#include "SDL2Terrain.h"
#include "base.h"      // CAnimAtr, CViewHexCoord, CHexCoord
#include "terrain.h"   // CHex, theMap
#include "terrain.inl"
#include "player.h"    // theGame.GetFrame() — water wave animation clock
#include "Perf.h"      // terrain sub-phase profiling counters

#include <SDL.h>
#include <vector>
#include <mutex>      // g_enEditedHexes is touched by sim AND render threads
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "thirdparty/stb_image.h"

#include <fstream>
#include <cstdlib>
#include <climits>
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

// Integer-indexed [type][variant] → representative tile, mirroring s_byTypeVar but
// without the per-lookup std::string construction + hash. The general (non-road/
// coast/water) path in TileForHex is the hot loop (called for every hex AND its 4
// feather neighbours); the string-keyed map made it ~15 us/hex. Built in Load().
static std::vector<const SDL2Terrain::Tile*> s_typeVarPtr[kNumTypeNames];

// Bumped on every (re)Load so the terrain mesh cache (keyed partly by this) is
// invalidated when textures are recreated — otherwise a stale cache could
// re-submit freed SDL_Texture* after loading a different game.
static unsigned s_loadGen = 0;

// Terrain-EDIT generation: bumped by the game whenever a hex's terrain changes at
// runtime (roads, city footprints, terraform, farm crop plots). Folded into the
// rebuild key so a genuine terrain edit forces ONE texture rebuild — without the
// periodic time-refresh hitch that was removed. Global so low-level game code
// (CHex::SetType, CFarmBuilding::UpdateFieldStage) can bump it via a local extern.
unsigned g_enTerrainEditGen = 0;

// Item 5 (incremental terrain patch): the hexes edited since the last mesh build, so the
// render can re-mesh ONLY those into the cached s_rt instead of a full ~50k-hex rebuild.
// Packed (x<<16)|y (map is square, coords < 65536). Capped — past the cap a full rebuild
// is cheaper (e.g. worldgen sets the whole map). Recorded by CHex::SetAlt/SetVisibleType.
std::vector<unsigned> g_enEditedHexes;
std::mutex            g_enEditMutex;   // sim thread pushes; render thread reads/swaps
const size_t          kEditPatchCap = 1024;
void g_enEditHex( int x, int y )
{
    // Bump the gen AND record under the same lock so the render can read them atomically
    // (a gen ahead of the list looked like a non-recorded edit → spurious full rebuild).
    std::lock_guard<std::mutex> lk( g_enEditMutex );
    ++g_enTerrainEditGen;
    if ( g_enEditedHexes.size( ) < kEditPatchCap )
        g_enEditedHexes.push_back( ( (unsigned)( x & 0xFFFF ) << 16 ) | (unsigned)( y & 0xFFFF ) );
}

// Item 5 (incremental terrain patch) kill-switch — opt-in until verified, then default on.
static bool TerrainPatchEnabled( )
{
    // Default ON: incrementally re-mesh only the few edited hexes instead of a full
    // ~1.6s rebuild. Without this, AI road-building edits ~10-50 hexes/frame and each
    // edit forces a whole-map rebuild → ~0.5fps zoomed out. Set EN_TPATCH=0 to disable.
    static int s_on = -1;
    if ( s_on < 0 ) { const char* e = SDL_getenv( "EN_TPATCH" ); s_on = ( e && *e && *e == '0' ) ? 0 : 1; }
    return s_on != 0;
}

static int TypeNameToInt( const std::string& n )
{
    for ( int i = 0; i < kNumTypeNames; ++i )
        if ( n == kTypeName[i] ) return i;
    return -1;
}

// Road facing (CHex::r_*) → (base source-dir, rotation). MakeRotated permutes
// the 4 direction views, so a rotated facing = the base road art shown from
// view (m_iDir - rot) mod 4. Base road sprites load at engine indices 0,2,6,10,11
// ↔ source road dirs 0,1,2,3,4 (sprtinit.cpp:114-120, ChangeToRoad:2574).
static const int kRoadSrcDir[12] = { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 4 };
// Corner facings (6-9 = r_l_*) come out 90° rotated vs straights/T (the corner
// base art orientation differs), so they carry an extra +1 quarter-turn.
static const int kRoadRot[12]    = { 0, 1, 0, 1, 2, 3, 1, 2, 3, 0, 0, 0 };
static const char* const kDirPrefix[4] = { "aa", "ac", "ae", "ag" };

// Coastline facing (0-38) → (source coastlne dir 0-11, rotation). Base sprites
// load at indices 0,4,8,12,13,17,21,25,26,30,34,38 ← coastlne/0..11 (TERRAIN.MIF),
// with MakeRotated filling the gaps (sprtinit.cpp:123-151). Coastline tiles are
// single-shape per dir at damage "a"; not bDrawVert, not shaded.
static const int kCoastSrcDir[39] = {
    0, 0, 0, 0,  1, 1, 1, 1,  2, 2, 2, 2,  3,  4, 4, 4, 4,  5, 5, 5, 5,
    6, 6, 6, 6,  7,  8, 8, 8, 8,  9, 9, 9, 9,  10, 10, 10, 10,  11 };
static const int kCoastRot[39] = {
    0, 1, 2, 3,  0, 1, 2, 3,  0, 1, 2, 3,  0,  0, 1, 2, 3,  0, 1, 2, 3,
    0, 1, 2, 3,  0,  0, 1, 2, 3,  0, 1, 2, 3,  0, 1, 2, 3,  0 };

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
        // NEAREST sampling: the tile art is an iso DIAMOND inscribed in a square, so
        // the square's corners hold non-tile pixels. Bilinear filtering at the diamond
        // EDGES samples those corner pixels and bleeds them in → the visible seams
        // between tiles. Nearest samples only the exact texel (no cross-edge bleed),
        // matching the original 1996 look, which abutted tiles seamlessly.
        SDL_SetTextureScaleMode( tex, SDL_ScaleModeNearest );
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

    // Tracks the stem currently chosen for each "type_variant" representative, so
    // the choice is deterministic (lexicographically smallest = "aa00a000" = dir
    // aa, letter a) rather than dependent on FindFirstFile enumeration order. The
    // engine always draws view (xiDir, damage 0) for non-field terrain
    // (terrain.cpp CHex::Draw), i.e. letter 'a'; multi-letter types reaching the
    // general s_byTypeVar path (river/swamp) must resolve to that 'a' tile.
    std::unordered_map<std::string, std::string> tvStem;

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
            // Per-(type,variant) representative: deterministically the tile with
            // the lexicographically smallest stem ("aa00a000" — direction aa,
            // letter a = the engine's damage-0 view). For single-tile types this
            // is the exact tile; for multi-letter types on the general path
            // (river/swamp) it pins the letter to 'a' regardless of scan order.
            if ( zoom == 0 )
            {
                std::string tvKey = type + "_" + std::to_string( variant );
                auto        stemIt = tvStem.find( tvKey );
                if ( stemIt == tvStem.end() || tstem < stemIt->second )
                {
                    s_byTypeVar[tvKey] = &tile;
                    tvStem[tvKey]      = tstem;
                    int ti = TypeNameToInt( type );          // integer-indexed mirror
                    if ( ti >= 0 )
                    {
                        if ( (int)s_typeVarPtr[ti].size() <= variant )
                            s_typeVarPtr[ti].resize( variant + 1, nullptr );
                        s_typeVarPtr[ti][variant] = &tile;
                    }
                }
            }
            ++files;
        } while ( FindNextFileA( h, &fd ) );
        FindClose( h );
    }

    s_loaded = true;
    ++s_loadGen;   // invalidate the terrain mesh cache (textures were (re)created)
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
    for ( int i = 0; i < kNumTypeNames; ++i ) s_typeVarPtr[i].clear();
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

// Camera-rotation corner permutation for the fog overlay. The per-corner fog alphas are
// computed in WORLD-corner order (the map-neighbour averages are dir-independent), but
// the verts are screen slots L/T/R/B (pts[]). This is the INVERSE of terrain.cpp's
// kCornerSlot (world→screen): kFogSlotInv[dir][screenSlot] = world corner that lands
// there. Without it the Gouraud blend disagrees across shared edges at dirs 1/2/3 and
// the soft fog collapses to hard diamond steps (the feather code already does this).
static const int kFogSlotInv[4][4] =
    { { 0, 1, 2, 3 }, { 1, 2, 3, 0 }, { 2, 3, 0, 1 }, { 3, 0, 1, 2 } };

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

// Pre-resolved water animation: the frame textures for one (type,variant), looked up
// ONCE per rebuild so the per-frame live draw is pure array indexing (no string build
// / hash lookup per hex — that was a ~1s/frame Debug-STL hot spot).
struct WaterAnim { const SDL2Terrain::Tile* frame[8]; int nFrames; };

// One water<->water feather band, computed once per rebuild and re-drawn LIVE each
// frame with the NEIGHBOUR's current wave-frame texture — so lake/ocean/river seams
// blend AND stay in sync, and the band is never clobbered. Positions are in s_rt
// texture space; offset by the current pan at draw time. animIdx → s_waterAnims.
struct WaterBlendBand { int animIdx; CPoint p[4]; SDL_FPoint uv[4]; };  // p/uv order: e0,e1,n0,n1
static const Uint8 kWaterFeatherA = 135;   // match the land feather strength

// Water wave animation: ocean/lake/coastline/river bake 8 frames (letters a-h),
// swamp 5 (a-e). The engine animates water by drawing its terrain sprite as a
// CSpriteView animation (DrawSimpleAnimation) that cycles those frames off the
// game clock; the GPU path was rendering only frame 'a' (frozen). Cycle the
// frame letter off theGame.GetFrame() so waves move again. kWaterHold = game
// frames per wave frame (24 Hz sim → hold 3 ≈ one 8-frame cycle per second).
static char WaterFrameLetter( int nFrames )
{
    // Match the engine: water sprites advance one frame every Time()=6 game-frames
    // (read live from the ocean sprite's ANIM_FRONT_1; DrawSimpleAnimation uses
    // nHold = GetAnim(FRONT_1,0)->Time()). 8 frames * 6 = ~2 s per ripple cycle.
    const int kWaterHold = 6;
    int f = ( (int)theGame.GetFrame( ) / kWaterHold ) % nFrames;
    return (char)( 'a' + f );
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

    if ( type == CHex::coastline )
    {
        int F = psprite->GetIndex( );
        if ( F < 0 || F > 38 ) F = 0;
        int  srcDir  = kCoastSrcDir[F];
        // The CAMERA term must SUBTRACT iDir (the camera rotates the world the
        // opposite way the sprite view index advances). With (+iDir) the shore came
        // out 180° wrong at camera dirs 1 and 3 (0/2 are self-symmetric under iDir
        // negation, so they looked fine) — i.e. dirs 1 and 3 were swapped. Coastline
        // is not 180°-symmetric (water on one side), so unlike straight roads the
        // camera sign is visible. kCoastRot keeps its (+) sign (tile-rotation term).
        int  viewDir = ( ( kCoastRot[F] - iDir ) % 4 + 4 ) % 4;
        char fr      = WaterFrameLetter( 8 );                       // animate foam
        std::string stem = std::string( kDirPrefix[viewDir] ) + "00" + fr + "000";
        const Tile* t = Get( "coastline", srcDir, stem );
        if ( !t ) t = Get( "coastline", srcDir, std::string( kDirPrefix[viewDir] ) + "00a000" );
        return t ? t : GetDefaultForType( "coastline" );
    }

    // Farm fields: the stored sprite is always (fields, variant 0); the visible
    // furrow ROTATION and crop GROW STAGE are NOT in the sprite index — the engine
    // derives them at draw time (CHex::Draw, terrain.cpp ~2517): furrow rotation =
    // (X*2+Y)&3 (coord-based patchwork, NOT camera dir), grow stage = GetGrowStage().
    // So the general integer-indexed path would freeze every plot at stage 0 / rot 0;
    // rebuild the stem the same way the engine picks GetView(iViewDir, iViewDamage).
    // Stem encoding (verified against source TGAs): first letter = stage (a/d/g =
    // 0/1/2), second = direction (a/c/e/g = 0/1/2/3) — same dir convention as kDirPrefix.
    if ( type == CHex::fields )
    {
        static const char kStageLetter[3] = { 'a', 'd', 'g' };
        static const char kRotLetter[4]   = { 'a', 'c', 'e', 'g' };
        int stage = phex->GetGrowStage( );
        if ( stage < 0 ) stage = 0; else if ( stage > 2 ) stage = 2;
        long off = theMap.GetHexOffPub( phex );
        int  eX  = theMap.Get_eX( );
        int  hx  = eX ? (int)( off % eX ) : 0;
        int  hy  = eX ? (int)( off / eX ) : 0;
        int  rot = ( hx * 2 + hy ) & 3;
        int  variant = psprite->GetIndex( );
        std::string stem; stem += kStageLetter[stage]; stem += kRotLetter[rot]; stem += "010000";
        const Tile* t = Get( "fields", variant, stem );
        if ( !t ) t = Get( "fields", variant, "aa010000" );    // fallback: stage 0 / rot 0
        return t ? t : GetDefaultForType( "fields" );
    }

    // Open-water types: single direction (aa), animated frames a-h / a-e.
    if ( type == CHex::ocean || type == CHex::lake || type == CHex::river || type == CHex::swamp )
    {
        int  variant = psprite->GetIndex( );
        int  nFrames = ( type == CHex::swamp ) ? 5 : 8;
        char fr      = WaterFrameLetter( nFrames );
        const Tile* t = Get( typeName, variant, std::string( "aa00" ) + fr + "000" );
        if ( !t ) t = Get( typeName, variant, "aa00a000" );        // static fallback
        return t ? t : GetDefaultForType( typeName );
    }

    int variant = psprite->GetIndex( );
    if ( type >= 0 && type < kNumTypeNames && variant >= 0 &&
         variant < (int)s_typeVarPtr[type].size() && s_typeVarPtr[type][variant] )
        return s_typeVarPtr[type][variant];            // integer-indexed (no string alloc)
    return GetDefaultForType( typeName );
}

// T5: which terrain types feather (soft-blend) at their boundaries. Matches the
// engine's CHex::Draw exclusions EXACTLY: road / city / resources never feather.
// Everything else (incl. fields) feathers — verified against the original; the
// original also feathers same-type/different-variant (INOUT) and coastline IN/OUT.
static bool Featherable( int type )
{
    return type >= 0 && type != CHex::road && type != CHex::city && type != CHex::resources;
}

// OPEN water (ocean/lake/river/swamp) never participates in the edge feather:
// the coast should blend only on its TERRAIN side, not bleed into the water.
// NOTE coastline is NOT open water — it's the land-side transition tile, so it
// DOES feather toward its land neighbours (just not toward the ocean edge, which
// this predicate excludes as a neighbour). Open water itself stays crisp.
static bool IsOpenWater( int type )
{
    return type == CHex::ocean || type == CHex::lake ||
           type == CHex::river || type == CHex::swamp;
}

// --- Build-placement footprint: striped hatch (matches the original) -------
// The original (sprite.cpp TerrainDrawQuad bHatch) fills each cursor hex with the
// cursor COLOUR on 4px horizontal bands — a striped hatch on the ground, NOT a solid
// fill. Reproduce the stripes by emitting only the "on" bands of the diamond.
// Drawn as a LIVE per-frame overlay (DrawBuildCursorOverlay), NOT baked into the
// cached terrain texture — so it animates every frame and shows on a static view
// (e.g. the rocket landing site), exactly as the 1996 per-frame software draw did.

// Clip a convex polygon `in` (n verts, boundary order) to the horizontal slab
// [yLo,yHi] (Sutherland–Hodgman against the two horizontal lines). Result in `out`,
// vertex count returned (≤ n+2). Robust for ANY quad shape including the pointy top/
// bottom tips — the tip vertex is simply kept or clipped naturally, so no wedge is
// dropped (the earlier per-scanline span version lost the tips, where the span
// collapses to a zero-width point).
static int ClipQuadToSlab( const SDL_FPoint* in, int n, float yLo, float yHi, SDL_FPoint* out )
{
    SDL_FPoint tmp[8]; int m = 0;
    for ( int i = 0; i < n; ++i )                 // keep y >= yLo
    {
        SDL_FPoint a = in[i], b = in[( i + 1 ) % n];
        bool ia = a.y >= yLo, ib = b.y >= yLo;
        if ( ia ) tmp[m++] = a;
        if ( ia != ib ) { float t = ( yLo - a.y ) / ( b.y - a.y ); tmp[m++] = { a.x + t * ( b.x - a.x ), yLo }; }
    }
    int o = 0;
    for ( int i = 0; i < m; ++i )                 // keep y <= yHi
    {
        SDL_FPoint a = tmp[i], b = tmp[( i + 1 ) % m];
        bool ia = a.y <= yHi, ib = b.y <= yHi;
        if ( ia ) out[o++] = a;
        if ( ia != ib ) { float t = ( yHi - a.y ) / ( b.y - a.y ); out[o++] = { a.x + t * ( b.x - a.x ), yHi }; }
    }
    return o;
}

// Append the cursor-colour band hatch for one diamond to `verts`. Matches the
// original's (y & 0x04) 4px bands; `phase` scrolls them. Each "on" 4px band is the
// diamond clipped to that horizontal slab → a convex polygon, fan-triangulated. The
// grid (yb) and phase are global (absolute window-Y), so bands align across tiles.
static void AppendHatchBands( const CPoint pts[4], SDL_Color col, int phase,
                              std::vector<SDL_Vertex>& verts )
{
    // Full vertical extent over ALL 4 corners — altitude can lift L/R above T or below B.
    const int yTop = __min( __min( pts[0].y, pts[1].y ), __min( pts[2].y, pts[3].y ) );
    const int yBot = __max( __max( pts[0].y, pts[1].y ), __max( pts[2].y, pts[3].y ) );
    if ( yBot <= yTop ) return;

    const SDL_FPoint quad[4] = {
        { (float)pts[0].x, (float)pts[0].y }, { (float)pts[1].x, (float)pts[1].y },
        { (float)pts[2].x, (float)pts[2].y }, { (float)pts[3].x, (float)pts[3].y } };
    const int band = 4;

    for ( int yb = ( yTop / band ) * band; yb < yBot; yb += band )
    {
        if ( ( ( yb + phase ) & band ) != 0 )   // "off" band → skip (matches (y+iFrame)&0x04)
            continue;
        float y0 = (float)__max( yb, yTop );
        float y1 = (float)__min( yb + band, yBot );
        if ( y1 <= y0 ) continue;

        SDL_FPoint poly[8];
        int np = ClipQuadToSlab( quad, 4, y0, y1, poly );
        for ( int i = 1; i + 1 < np; ++i )      // fan-triangulate the convex slice
        {
            SDL_Vertex a, b, c;
            a.position = poly[0];     a.tex_coord = { 0, 0 }; a.color = col;
            b.position = poly[i];     b.tex_coord = { 0, 0 }; b.color = col;
            c.position = poly[i + 1]; c.tex_coord = { 0, 0 }; c.color = col;
            verts.push_back( a ); verts.push_back( b ); verts.push_back( c );
        }
    }
}

// Draw the build/rocket placement footprint LIVE, every frame, in window space —
// the original (1996) redrew the cursor area each frame, so the hatch animated for
// free; our cached terrain texture is frozen between rebuilds, so the hatch must be
// its own per-frame overlay. Costs ~nothing: only the footprint rect + 2 exit hexes
// are visited, and only while a build cursor is active. Drawn UNDER the sprite/chrome
// overlay (composited after Render returns), so it reads as on-the-ground.
static void DrawBuildCursorOverlay( SDL_Renderer* r, const CAnimAtr& aa )
{
    if ( !theMap.HaveBldgCur( ) )
        return;
    CHexCoord hexUL; int cx = 0, cy = 0;
    if ( !theMap.GetBldgCurRect( hexUL, cx, cy ) )
        return;

    const int               phase = (int)theGame.GetFrame( );   // advances every frame → bands scroll
    std::vector<SDL_Vertex> verts;

    auto addHex = [&]( CHex* phex, CHexCoord hc ) {
        if ( !phex ) return;
        int cm = phex->GetCursorMode( );
        if ( cm == CHex::no_cur ) return;
        SDL_Color col;
        switch ( cm )
        {
            case CHex::ok_cur:        col = { 255, 255, 255, 215 }; break;  // white = buildable
            case CHex::land_exit_cur: col = {  30,  30,  30, 215 }; break;  // dark = land exit
            case CHex::sea_exit_cur:  col = {  30,  30,  30, 215 }; break;  // dark = sea exit
            case CHex::warn_cur:      col = { 235, 220,  45, 215 }; break;  // yellow
            case CHex::bad_cur:       col = { 225,  45,  45, 225 }; break;  // red
            case CHex::lousy_cur:     col = { 235, 140,  45, 220 }; break;  // orange
            default:                  col = { 255, 255, 255, 215 }; break;
        }
        CPoint pts[4];
        if ( !aa.MapToWindowHex( hc, pts ) ) return;
        AppendHatchBands( pts, col, phase, verts );
    };

    for ( int j = 0; j < cy; ++j )
        for ( int i = 0; i < cx; ++i )
        {
            CHexCoord hc( hexUL.X( ) + i, hexUL.Y( ) + j ); hc.Wrap( );
            addHex( theMap.GetHex( hc ), hc );
        }
    if ( theMap.m_pLandExit ) addHex( theMap.m_pLandExit, theMap.m_pLandExit->GetHex( ) );
    if ( theMap.m_pShipExit ) addHex( theMap.m_pShipExit, theMap.m_pShipExit->GetHex( ) );

    if ( verts.empty( ) )
        return;
    SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
    SDL_RenderGeometry( r, nullptr, verts.data( ), (int)verts.size( ), nullptr, 0 );
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
    int       iTopY   = __min( ptTL.y, ptTR.y ) - 2;
    int       iLeftX  = ptTL.x - 2;
    int       iRightX = ptTR.x + 2;
    const int margin  = ( 64 << 3 ) >> zoom;           // tall-hex overscan (max alt)

    // --- Terrain mesh CACHE → RENDER-TARGET TEXTURE + PAN-BLIT --------------
    // Rebuilding the vertex mesh is the whole cost (2 ms zoom-in .. 225 ms zoomed
    // out @ ~43k hexes); the GPU submit/present was never it. So render the terrain
    // ONCE into an off-screen texture (sized viewport + kMarginPx all round, keyed
    // on zoom/dir/wave/loadGen — NOT scroll). Scrolling iso terrain is a pure
    // screen-space shift, so a pan just BLITS that texture at an offset — one GPU
    // copy, sub-millisecond — instead of the 225 ms CPU rebuild that made zoomed-out
    // scrolling crawl. The fog/feather per-hex work runs only on a rebuild (key
    // change, pan past the margin, or a refresh interval), so smooth fog + feather
    // are affordable at all zooms.
    static SDL_Texture* s_rt = nullptr;      // terrain tiles + feather (full bright, NO shade/fog)
    static SDL_Texture* s_fogRT = nullptr;   // fog-of-war dim, decoupled overlay
    static SDL_Texture* s_shadeRT = nullptr; // slope shading, BLURRED so tile edges feather
    static SDL_Texture* s_shadeHalf = nullptr;  // half-res scratch for the blur round-trip
    static int       s_rtW = 0, s_rtH = 0;
    // Slope-shade overlay geometry (grayscale brightness per tile), built with the
    // terrain mesh; rendered to s_shadeRT then blurred so the per-tile flat shading
    // blends across boundaries (the original feathered this; a sharp step looks bad).
    static std::vector<SDL_Vertex> s_shadeVerts;
    // Animated open-water: captured during the build so the wave can be re-drawn IN
    // PLACE into s_rt on each wave-tick (cheap) instead of rebuilding the whole mesh —
    // that's what lets water animate at ALL zooms (it was frozen zoomed out because
    // the terrain texture only rebuilds on a view change).
    static std::vector<CHex*>  s_waterHex;   // open-water hexes (stable CHex*)
    static std::vector<CPoint> s_waterPos;   // 4 corner positions/hex (texture space)
    static std::vector<int>    s_waterAnimOf;   // per water hex: index into s_waterAnims
    static std::vector<WaterAnim> s_waterAnims; // distinct (type,variant) frame tables
    static std::vector<WaterBlendBand> s_waterBlend;  // water<->water blend bands (texture space)
    static unsigned s_waterTick = ~0u;
    // Water cache (perf): the live water layer used to re-submit ~30k hex diamonds EVERY
    // frame (~85ms in Debug → the zoomed-out FPS wall). Instead, pre-group the diamond/band
    // vertices by animation index ONCE per rebuild (positions are static in texture space —
    // only the frame texture changes), bake them into s_waterRT only when the wave frame
    // advances (~4Hz), and blit that texture (1 GPU copy) under the terrain each frame.
    static std::vector<std::vector<SDL_Vertex>> s_waterDiamGeom;  // per animIdx: diamond verts (tex space)
    static std::vector<std::vector<SDL_Vertex>> s_waterBandGeom;  // per animIdx: blend-band verts (tex space)
    static SDL_Texture* s_waterRT     = nullptr;  // baked animated water, blitted under s_rt
    static unsigned     s_waterRTtick = ~0u;      // wave-frame index baked into s_waterRT
    static uint64_t  s_sig = ~0ull;
    static unsigned  s_builtEditGen = 0;  // g_enTerrainEditGen baked into the current mesh
    static CHexCoord s_refHex;            // a fixed hex captured at build time
    static CPoint    s_refPx( 0, 0 );     // its window-screen pos at build time
    const int kMarginPx = 256;            // texture extends this far beyond viewport

    // Fog overlay geometry, built FREE during the terrain pass (positions only), then
    // re-coloured on a fast throttle WITHOUT rebuilding terrain — so fog tracks unit
    // vision in ~150 ms while the heavy terrain mesh rebuilds only on a view change.
    static std::vector<SDL_Vertex> s_fogVerts;   // 6 per hex (2 tris), black, per-corner alpha
    static std::vector<CHex*>      s_fogHex;     // hex per fog quad (self visibility, no GetHex)
    static std::vector<int>        s_fogNbr;     // 8 neighbour fog-indices per hex (-1 = none)
    static std::vector<float>      s_fogVis;     // per-hex visibility, re-sampled each throttle
    static DWORD s_fogUpdAt = 0;
    const DWORD kFogThrottle = 150;

    // Terrain mesh is keyed on zoom/dir/wave/loadGen — NOT scroll, and NO time
    // refresh: it rebuilds only when the view changes or a pan leaves the margin, so
    // sitting still / panning within margin never hitches. (Fog moved to the overlay;
    // terrain edits like roads show on the next view change.)
    uint64_t sig = (uint64_t)( zoom & 7 )
                 ^ ( (uint64_t)( aa.m_iDir & 3 ) << 3 )
                 ^ ( (uint64_t)s_loadGen          << 5 )
                 ^ ( (uint64_t)g_enTerrainEditGen << 40 );  // runtime terrain edits
    // Water animation no longer rebuilds the terrain texture (it's re-drawn in place
    // on the wave-tick below), so the rebuild key omits the wave at every zoom.
    DWORD    _nowT = GetTickCount( );
    unsigned curWaterTick = (unsigned)( theGame.GetFrame( ) / 6 );   // kWaterHold = 6

    const int rtW = ws.cx + 2 * kMarginPx;
    const int rtH = ws.cy + margin + 2 * kMarginPx;

    // (Re)create the textures when the viewport size (hence rtW/rtH) changes.
    if ( s_rt && ( s_rtW != rtW || s_rtH != rtH ) )
    {
        SDL_DestroyTexture( s_rt ); s_rt = nullptr;
        if ( s_fogRT )    { SDL_DestroyTexture( s_fogRT );    s_fogRT = nullptr; }
        if ( s_shadeRT )  { SDL_DestroyTexture( s_shadeRT );  s_shadeRT = nullptr; }
        if ( s_shadeHalf ){ SDL_DestroyTexture( s_shadeHalf );s_shadeHalf = nullptr; }
        if ( s_waterRT )  { SDL_DestroyTexture( s_waterRT );  s_waterRT = nullptr; }
    }
    if ( !s_rt )
    {
        s_rt      = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW, rtH );
        s_fogRT   = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW, rtH );
        s_waterRT = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW, rtH );
        if ( s_waterRT ) { SDL_SetTextureBlendMode( s_waterRT, SDL_BLENDMODE_BLEND ); s_waterRTtick = ~0u; }
        s_shadeRT = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW, rtH );
        s_shadeHalf = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW / 2, rtH / 2 );
        if ( s_rt )    { SDL_SetTextureBlendMode( s_rt, SDL_BLENDMODE_BLEND ); s_rtW = rtW; s_rtH = rtH; }
        if ( s_fogRT ) SDL_SetTextureBlendMode( s_fogRT, SDL_BLENDMODE_BLEND );
        // s_shadeRT blend is NONE here so the blur copies overwrite cleanly; it's
        // flipped to MOD only for the final composite blit over the terrain.
        if ( s_shadeRT )   { SDL_SetTextureBlendMode( s_shadeRT, SDL_BLENDMODE_NONE ); SDL_SetTextureScaleMode( s_shadeRT, SDL_ScaleModeLinear ); }
        if ( s_shadeHalf ) { SDL_SetTextureBlendMode( s_shadeHalf, SDL_BLENDMODE_NONE ); SDL_SetTextureScaleMode( s_shadeHalf, SDL_ScaleModeLinear ); }
        s_sig = ~0ull;   // force a rebuild into the fresh textures
    }
    if ( !s_rt || !s_fogRT || !s_shadeRT || !s_shadeHalf )   // alloc failed
        return;

    // Pan delta of the cached textures = how far our reference hex has moved on
    // screen. Within ±kMarginPx the textures still cover the viewport → just blit.
    // Compare the BASE signature (zoom/dir/loadgen) ignoring the terrain-edit gen, so a
    // pure terrain edit can take the cheap PATCH path instead of a full rebuild.
    uint64_t sigNoEdit  = sig   ^ ( (uint64_t)g_enTerrainEditGen << 40 );
    uint64_t lastNoEdit = s_sig ^ ( (uint64_t)s_builtEditGen     << 40 );
    int dX = 0, dY = 0; bool covered = false;
    if ( sigNoEdit == lastNoEdit )
    {
        CPoint rp[4];
        if ( aa.MapToWindowHex( s_refHex, rp ) )
        {
            dX = rp[0].x - s_refPx.x;
            dY = rp[0].y - s_refPx.y;
            covered = ( abs( dX ) <= kMarginPx && abs( dY ) <= kMarginPx );
        }
        else
            // Can't measure the pan (ref hex projected off-window / culled). Rebuilding every
            // frame because of this is catastrophic (~1.6s each → 0.5fps). The view didn't
            // change (sig is unchanged), so the cached texture is almost certainly still valid
            // — assume covered and keep blitting rather than re-meshing in a loop.
            covered = true;
    }

    // Edit-patch (Item 5): when only terrain EDITS changed (base unchanged, pan covered)
    // and every edit since the last build was recorded (gen delta == list size) and the
    // list is small, re-mesh ONLY those hexes into s_rt instead of the ~50k-hex rebuild.
    bool baseSame  = ( sigNoEdit == lastNoEdit ) && covered;
    // Read the gen AND the list size together under the lock so they're a consistent
    // snapshot (else a concurrent edit between the two reads looks non-recorded → rebuild).
    unsigned genSnap; size_t editCount;
    { std::lock_guard<std::mutex> lk( g_enEditMutex ); genSnap = g_enTerrainEditGen; editCount = g_enEditedHexes.size( ); }
    bool editsPend = ( genSnap != s_builtEditGen );
    bool editsOK   = editsPend && editCount > 0 && editCount <= kEditPatchCap
                     && ( genSnap - s_builtEditGen ) == (unsigned)editCount;
    const bool doPatch     = TerrainPatchEnabled( ) && baseSame && editsOK;
    const bool needRebuild = !baseSame || ( editsPend && !doPatch );
    if ( needRebuild )
        Perf::CounterAdd( !baseSame ? "reb.base" : "reb.editmiss", 1 );
    // DIAGNOSE reb.base: split into signature-change vs pan-not-covered, and when the
    // signature changed, record WHICH component (zoom/dir/loadgen) flipped. This tells
    // us why a full rebuild keeps firing while sitting still (the ~1.6s/rebuild = ~1fps).
    if ( needRebuild && !baseSame )
    {
        bool sigChg = ( sigNoEdit != lastNoEdit );
        Perf::CounterAdd( sigChg ? "rb.sigchg" : "rb.notcov", 1 );
        if ( sigChg )
        {
            static int s_pz = -999, s_pd = -999; static unsigned s_plg = ~0u;
            if ( zoom        != s_pz )  { Perf::CounterAdd( "rb.zoomchg", 1 ); s_pz  = zoom; }
            if ( ( aa.m_iDir & 3 ) != s_pd ) { Perf::CounterAdd( "rb.dirchg", 1 ); s_pd = aa.m_iDir & 3; }
            if ( s_loadGen   != s_plg ) { Perf::CounterAdd( "rb.loadgenchg", 1 ); s_plg = s_loadGen; }
        }
        else
        {
            Perf::CounterAdd( "rb.dx", abs( dX ) );
            Perf::CounterAdd( "rb.dy", abs( dY ) );
        }
    }
    if ( editsPend )
    {
        Perf::CounterAdd( "ed.gendelta", (int)( g_enTerrainEditGen - s_builtEditGen ) );
        Perf::CounterAdd( "ed.listsz",   (int)g_enEditedHexes.size( ) );
        Perf::CounterAdd( "ed.basesame", baseSame ? 1 : 0 );
        Perf::CounterAdd( "ed.patchen",  TerrainPatchEnabled( ) ? 1 : 0 );
    }

    // MEASURE (why is the mesh rebuilding?): count rebuilds/interval and split the cause
    // — a key change (zoom/dir/s_loadGen/terrain-edit) vs a pan past the margin. A high
    // rebuild.edit during road-building/combat = terrain edits forcing a full re-mesh.
    if ( needRebuild )
    {
        Perf::CounterAdd( "rebuild.cnt", 1 );
        if ( sig != s_sig )
        {
            Perf::CounterAdd( "rebuild.key", 1 );
            static uint64_t s_prevEditGen = 0;
            if ( g_enTerrainEditGen != s_prevEditGen ) { Perf::CounterAdd( "rebuild.edit", 1 ); s_prevEditGen = g_enTerrainEditGen; }
        }
        else
            Perf::CounterAdd( "rebuild.pan", 1 );
    }

    // Build the mesh in TEXTURE space: positions are offset by +kMarginPx so the
    // margin band (hexes left/above the viewport) lands at texture x/y >= 0.
    const int offX = kMarginPx, offY = kMarginPx;

    // Per-build batch accumulator (texture, verts), drawn into s_rt then discarded.
    std::vector<std::pair<SDL_Texture*, std::vector<SDL_Vertex>>> s_cache;

    if ( needRebuild )
    {
    Perf::ScopeCounter _cr( "t.rebuild" );   // full mesh rebuild (O visible hexes)
    s_sig = sig; dX = dY = 0;   // rebuilt at the current view → zero pan
    s_builtEditGen = g_enTerrainEditGen;   // this mesh now reflects all edits so far
    { std::lock_guard<std::mutex> lk( g_enEditMutex ); g_enEditedHexes.clear( ); }   // rebuild absorbs all pending edits
    s_fogVerts.clear(); s_fogHex.clear(); s_fogNbr.clear(); s_fogVis.clear(); s_shadeVerts.clear();
    s_waterHex.clear(); s_waterPos.clear(); s_waterBlend.clear();
    s_waterAnimOf.clear(); s_waterAnims.clear();
    // (type<<8|variant) -> index into s_waterAnims for THIS rebuild; resolves each
    // water animation's frame textures once (string build + hash lookup happens here,
    // ~19 groups, not per-hex-per-frame).
    std::unordered_map<int,int> waterAnimKey;
    auto resolveWaterAnim = [&]( int wtype, int wvar ) -> int {
        int key = ( wtype << 8 ) | ( wvar & 0xFF );
        auto it = waterAnimKey.find( key );
        if ( it != waterAnimKey.end( ) ) return it->second;
        WaterAnim wa;
        wa.nFrames = ( wtype == CHex::swamp ) ? 5 : 8;
        const char* tn = ( wtype >= 0 && wtype < kNumTypeNames ) ? kTypeName[wtype] : "ocean";
        for ( int f = 0; f < wa.nFrames; ++f )
        {
            char fr = (char)( 'a' + f );
            const Tile* t = Get( tn, wvar, std::string( "aa00" ) + fr + "000" );
            if ( !t ) t = Get( tn, wvar, "aa00a000" );
            wa.frame[f] = t;
        }
        int idx = (int)s_waterAnims.size( );
        s_waterAnims.push_back( wa );
        waterAnimKey[key] = idx;
        return idx;
    };

    // Extend the iteration region to fill the margin band. Estimate screen px per
    // view-hex step from two probe projections, then convert kMarginPx to hexes.
    {
        CPoint a[4], bx[4], by[4];
        aa.MapToWindowHex( CHexCoord( CViewHexCoord( iLeftX,     iTopY     ), TRUE ), a  );
        aa.MapToWindowHex( CHexCoord( CViewHexCoord( iLeftX + 1, iTopY     ), TRUE ), bx );
        aa.MapToWindowHex( CHexCoord( CViewHexCoord( iLeftX,     iTopY + 1 ), TRUE ), by );
        int pxX = __max( 1, abs( bx[0].x - a[0].x ) + abs( by[0].x - a[0].x ) );
        int pxY = __max( 1, abs( bx[0].y - a[0].y ) + abs( by[0].y - a[0].y ) );
        iLeftX  -= kMarginPx / pxX + 2;
        iRightX += kMarginPx / pxX + 2;
        iTopY   -= kMarginPx / pxY + 2;
    }

    // Reference hex for pan tracking + its window-screen pos (NO texture offset — pan tracking
    // is in window space). Use the hex at the WINDOW CENTRE, not the top of the visible span:
    // the top hex's projection can land just above the window and get CULLED by MapToWindowHex
    // (returns false), which left `covered` permanently false at far zoom-out → a full ~1.6s
    // mesh rebuild EVERY frame (0.5fps). A centre hex always projects inside the window.
    s_refHex = aa._WindowToHex( CPoint( ws.cx / 2, ws.cy / 2 ) );
    { CPoint rp[4]; if ( aa.MapToWindowHex( s_refHex, rp ) ) s_refPx = rp[0]; else s_refPx = CPoint( 0, 0 ); }

    // Redirect rendering into the texture (save/restore target + viewport).
    SDL_Texture* prevTarget = SDL_GetRenderTarget( r );
    SDL_Rect savedVp; SDL_RenderGetViewport( r, &savedVp );
    SDL_SetRenderTarget( r, s_rt );
    SDL_RenderSetViewport( r, nullptr );
    // TRANSPARENT clear: open water is left as holes here and filled by the live water
    // layer UNDER this texture at composite time. The torus map covers the whole
    // viewport+margin, so no visible area is left uncovered by terrain+water.
    SDL_SetRenderDrawColor( r, 0, 0, 0, 0 );
    SDL_RenderClear( r );

    // Terrain is drawn ROW BY ROW, back-to-front, so a tall tile's altitude-raised
    // diamond correctly occludes the lower tiles in rows behind it (global texture
    // batching broke this — the ocean batch, drawn after the mountain batch, painted
    // over a mountain in front of it). WITHIN a row, base + feather are batched by
    // texture (rows don't overlap, so order inside a row is free) — that keeps the
    // SDL_RenderGeometry call count low (per-hex drawing tanked the FPS when zoomed
    // out). Each row: draw its base batches, then its feather batches, then advance.
    std::unordered_map<SDL_Texture*, std::vector<SDL_Vertex>> rowBase;
    std::unordered_map<SDL_Texture*, std::vector<SDL_Vertex>> rowFeather;
    // Per-row flush keeps back-to-front order so a tall (mountain) tile occludes
    // lower tiles AND feather bands behind it. (An earlier global-batch optimization
    // flushed all base then all feather across the whole frame to cut draw calls —
    // but that drew feather OVER mountains in front of it, and zoomed out it also let
    // a back tile paint over a front mountain. Draw count only matters on a REBUILD
    // now, which is rare, so per-row ordering is affordable at every zoom.)
    auto drawRow = [&]() {
        for ( auto& b : rowBase )
            if ( !b.second.empty() ) { s_cache.emplace_back( b.first, std::move( b.second ) ); b.second.clear(); }
        for ( auto& b : rowFeather )
            if ( !b.second.empty() ) { s_cache.emplace_back( b.first, std::move( b.second ) ); b.second.clear(); }
    };
    // NOTE: the cursor footprint hatch is drawn LIVE every frame in
    // DrawBuildCursorOverlay() (end of Render), NOT baked into this cached texture —
    // so it animates and appears even on a static view (e.g. the rocket landing site).

    bool seenContent = false;
    DWORD _gt = GetTickCount( );   // MEASURE: rebuild sub-phases (coarse ms → µs)
    LARGE_INTEGER _qpf; QueryPerformanceFrequency( &_qpf );   // PROFILE: per-hex phase split
    long long _accProj = 0, _accFeath = 0; LARGE_INTEGER _pa, _pb;
    int _hexCnt = 0;   // PROFILE: count of hexes emitted (margin/zoom hex-count vs per-hex cost)
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
            // (MapToWindowHex applies the per-m_iDir corner reorder).
            CPoint pts[4];
            QueryPerformanceCounter( &_pa );
            aa.MapToWindowHex( hexcoord, pts );   // pts[0]=Left 1=Top 2=Right 3=Bottom
            QueryPerformanceCounter( &_pb ); _accProj += _pb.QuadPart - _pa.QuadPart;
            pts[0].x += offX; pts[0].y += offY;   // window → texture space
            pts[1].x += offX; pts[1].y += offY;
            pts[2].x += offX; pts[2].y += offY;
            pts[3].x += offX; pts[3].y += offY;

            // Cull to the texture bounds [0,rtW]x[0,rtH] (= viewport + kMarginPx all
            // round, in texture space). Hexes outside the margin band are dropped.
            int minX = pts[0].x, maxX = pts[0].x, minY = pts[0].y, maxY = pts[0].y;
            for ( int i = 1; i < 4; ++i )
            {
                minX = __min( minX, pts[i].x ); maxX = __max( maxX, pts[i].x );
                minY = __min( minY, pts[i].y ); maxY = __max( maxY, pts[i].y );
            }
            if ( maxX < 0 || minX > rtW || maxY < 0 || minY > rtH )
                continue;
            rowAny = true;
            ++_hexCnt;   // PROFILE: hexes actually emitted this rebuild

            CTerrainSprite* psprite = phex->GetSprite( );
            int             type    = psprite ? psprite->GetID( ) : -1;
            if ( type < 0 || type >= kNumTypeNames )
                continue;

            const Tile* tile = TileForHex( phex, aa.m_iDir );  // forest/road/variant
            if ( !tile || !tile->tex[zoom] )
                continue;

            SDL_Texture* tex = tile->tex[zoom];
            std::vector<SDL_Vertex>& vb = rowBase[tex];   // batched within this row

            // T4 slope shading: shaded land gets per-triangle slope brightness;
            // water/coast stay full colour. T6 fog is applied per-vertex below.
            // GetWorldHex (4 fixed-point corner altitudes) is only needed for the
            // shade math, so fetch it lazily here — skips it for the ~27k unshaded
            // water/coast hexes at full zoom-out (was called unconditionally).
            float bL = 1.0f, bR = 1.0f;
            if ( tile->shade )
            {
                CMapLoc3D c3d[4];
                hexcoord.GetWorldHex( c3d );
                int z0 = c3d[0].m_fixZ.Round(), z1 = c3d[1].m_fixZ.Round();
                int z2 = c3d[2].m_fixZ.Round(), z3 = c3d[3].m_fixZ.Round();
                bL = TriBrightness( z0, z1, z2, z3, true );
                bR = TriBrightness( z0, z1, z2, z3, false );
            }

            // T6 SOFT fog-of-war: per-corner visibility = average of the nearby
            // hexes meeting at that corner, so the dim blends smoothly across hex
            // edges (Gouraud-interpolated) instead of hard diamond steps.
            // fog[c] in [kFogDim, 1]: 1 = fully in sight, kFogDim = fully fogged.
            // Soft per-corner fog at ALL zooms: each corner = average of the nearby
            // hexes meeting at it, so the dim blends smoothly across hex edges
            // (Gouraud) instead of hard diamond steps. The 9 visibility samples/hex
            // run only on a mesh REBUILD now (the pan path blits the cached texture),
            // so the smooth fog blur is affordable at every zoom.
            int  hx = hexcoord.X( ), hy = hexcoord.Y( );   // (feather below uses these)
            // PERF: the per-corner soft-fog alphas are filled by the throttle re-sample
            // (fast, precomputed neighbours) on EVERY fog update — including this rebuild
            // frame (the fog block below now re-samples on rebuild too). So skip the 9
            // GetHex/hex sampling here; it was ~half the whole rebuild. Placeholder alphas
            // (overwritten before the fog is drawn this frame).
            float fog[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

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

            // Terrain texture is FULL BRIGHT (white) — neither shade nor fog baked in.
            // Slope shade is the blurred s_shadeRT overlay (multiply); fog is s_fogRT
            // (blend). Keeping them as separate overlays lets the shade be BLURRED so
            // the per-tile flat shading feathers across boundaries instead of a hard
            // step, and lets fog re-sample without rebuilding the terrain texture.
            SDL_Color white = { 255, 255, 255, 255 };
            vL.color = white; vT.color = white; vR.color = white; vB.color = white;
            // Open water is NOT baked into the cached texture — it's left as a
            // transparent HOLE and filled by the live animated water layer UNDER this
            // texture (composite below). That keeps all water on one shared wave frame
            // (synced) while land in front still occludes it. Land/coast ARE baked.
            if ( !IsOpenWater( type ) )
            {
                vb.push_back( vL ); vb.push_back( vT ); vb.push_back( vB );
                vb.push_back( vT ); vb.push_back( vR ); vb.push_back( vB );
            }

            // Capture OPEN-WATER tiles for the live layer: the hex (to re-pick the
            // current frame) + its 4 corner positions (texture space). Also cache a
            // water<->water blend band toward each DIFFERING open-water neighbour, drawn
            // live (neighbour's current frame) so lake/ocean/river seams blend + sync.
            if ( IsOpenWater( type ) )
            {
                s_waterHex.push_back( phex );
                s_waterPos.push_back( pts[0] ); s_waterPos.push_back( pts[1] );
                s_waterPos.push_back( pts[2] ); s_waterPos.push_back( pts[3] );
                s_waterAnimOf.push_back( resolveWaterAnim( type, psprite->GetIndex( ) ) );

                static const SDL_FPoint wfuv[4] = { {0.f,0.5f}, {0.5f,0.f}, {1.f,0.5f}, {0.5f,1.f} };
                static const int wDX[4] = { 0, 1, 0, -1 };
                static const int wDY[4] = { -1, 0, 1, 0 };
                static const int wSlot[4][4] = { {0,1,2,3}, {3,0,1,2}, {2,3,0,1}, {1,2,3,0} };
                const float kBandW = 0.38f;
                float wcx = ( pts[0].x + pts[1].x + pts[2].x + pts[3].x ) * 0.25f;
                float wcy = ( pts[0].y + pts[1].y + pts[2].y + pts[3].y ) * 0.25f;
                for ( int e = 0; e < 4; ++e )
                {
                    CHexCoord wnhc( hx + wDX[e], hy + wDY[e] ); wnhc.Wrap( );
                    CHex* wpn = theMap.GetHex( wnhc );
                    if ( !wpn ) continue;
                    CTerrainSprite* wns = wpn->GetSprite( );
                    int wntype = wns ? wns->GetID( ) : -1;
                    if ( !IsOpenWater( wntype ) ) continue;          // water<->water only
                    const Tile* wntile = TileForHex( wpn, aa.m_iDir );
                    if ( !wntile || !wntile->tex[zoom] || wntile == tile ) continue;
                    int wnAnim = resolveWaterAnim( wntype, wns->GetIndex( ) );
                    int c0 = wSlot[aa.m_iDir & 3][e];
                    int c1 = wSlot[aa.m_iDir & 3][( e + 1 ) & 3];
                    float i0x = pts[c0].x + kBandW * ( wcx - pts[c0].x );
                    float i0y = pts[c0].y + kBandW * ( wcy - pts[c0].y );
                    float i1x = pts[c1].x + kBandW * ( wcx - pts[c1].x );
                    float i1y = pts[c1].y + kBandW * ( wcy - pts[c1].y );
                    SDL_FPoint u0 = wfuv[c0], u1 = wfuv[c1];
                    WaterBlendBand wbnd;
                    wbnd.animIdx = wnAnim;
                    wbnd.p[0] = pts[c0];                       wbnd.uv[0] = u0;
                    wbnd.p[1] = pts[c1];                       wbnd.uv[1] = u1;
                    wbnd.p[2] = CPoint( (int)i0x, (int)i0y );  wbnd.uv[2] = { u0.x + kBandW*(0.5f-u0.x), u0.y + kBandW*(0.5f-u0.y) };
                    wbnd.p[3] = CPoint( (int)i1x, (int)i1y );  wbnd.uv[3] = { u1.x + kBandW*(0.5f-u1.x), u1.y + kBandW*(0.5f-u1.y) };
                    s_waterBlend.push_back( wbnd );
                }
            }

            // Slope-shade overlay quad: grayscale brightness per triangle (bL/bR), same
            // diamond. Rendered (overwrite, front-wins) into s_shadeRT then blurred, so
            // adjacent tiles' flat shades blend at the seam. MOD-blitted over terrain.
            auto shadeV = []( const CPoint& p, float b ) -> SDL_Vertex {
                SDL_Vertex v; v.position = { (float)p.x, (float)p.y }; v.tex_coord = { 0, 0 };
                Uint8 g = (Uint8)__min( 255, (int)( b * 255.0f ) );
                v.color = { g, g, g, 255 }; return v;
            };
            s_shadeVerts.push_back( shadeV( pts[0], bL ) );  // L,T,B  (left tri)
            s_shadeVerts.push_back( shadeV( pts[1], bL ) );
            s_shadeVerts.push_back( shadeV( pts[3], bL ) );
            s_shadeVerts.push_back( shadeV( pts[1], bR ) );  // T,R,B  (right tri)
            s_shadeVerts.push_back( shadeV( pts[2], bR ) );
            s_shadeVerts.push_back( shadeV( pts[3], bR ) );

            // Fog overlay quad (built once per terrain rebuild; re-coloured cheaply on
            // the fog throttle). Black, per-corner alpha = (1-fog) → fogged = dark.
            // Vert order [0..5] = L,T,B, T,R,B (corners 0,1,3, 1,2,3) so the throttled
            // re-sample can rewrite alpha by index without re-projecting.
            auto fogV = []( const CPoint& p, float f ) -> SDL_Vertex {
                SDL_Vertex v; v.position = { (float)p.x, (float)p.y }; v.tex_coord = { 0, 0 };
                Uint8 a = (Uint8)__min( 255, (int)( ( 1.0f - f ) * 255.0f ) );
                v.color = { 0, 0, 0, a }; return v;
            };
            // World-corner alphas → screen slots (L/T/R/B) for this camera rotation.
            const int* fperm = kFogSlotInv[aa.m_iDir & 3];
            float sf[4] = { fog[fperm[0]], fog[fperm[1]], fog[fperm[2]], fog[fperm[3]] };
            s_fogHex.push_back( phex );   // self visibility on re-sample (no GetHex)
            s_fogVerts.push_back( fogV( pts[0], sf[0] ) );
            s_fogVerts.push_back( fogV( pts[1], sf[1] ) );
            s_fogVerts.push_back( fogV( pts[3], sf[3] ) );
            s_fogVerts.push_back( fogV( pts[1], sf[1] ) );
            s_fogVerts.push_back( fogV( pts[2], sf[2] ) );
            s_fogVerts.push_back( fogV( pts[3], sf[3] ) );

            // (Cursor footprint hatch is drawn live in DrawBuildCursorOverlay, not
            // baked here — see note above.)

            // T5 edge feather: bleed a DIFFERING featherable neighbour's tile in
            // along the shared diamond edge — a THIN, low-alpha band hugging the
            // edge, approximating the original's 1-2px edge dither (terrain.cpp
            // FEATHER_IN/OUT/INOUT). Accumulated into this ROW's feather batch and
            // drawn after the row's base (back-to-front — a tall tile in a later row
            // occludes it, instead of a back tile's blend painting over a mountain in
            // front of it).
            //
            // Mirrors the ORIGINAL's model (terrain.cpp:2470): everything feathers
            // EXCEPT road/city/resources. The COASTLINE (shore) tile feathers ITSELF
            // into all its neighbours — the original's FEATHER_IN — so the shore
            // softens into BOTH the land behind it AND the open water it sits on (this
            // is the "feather the shore tile, not the ocean" goal). A non-coastline
            // tile does NOT feather toward a coastline neighbour (the original's
            // FEATHER_OUT — the coastline side already blends that seam), avoiding a
            // double band. Land↔land is symmetric (INOUT), as before.
            //
            // OPEN water: only the COASTLINE may pull it in (shore→sea blend). A land
            // tile still does NOT pull open water (bleeding ocean onto grass read as a
            // smear). Water↔water (lake/river vs ocean) is NOT done here — those tiles
            // are re-drawn in place every wave-tick, which would wipe a baked band; that
            // case needs the wave re-draw path (separate change).
            //
            // Open water itself stays a non-receiver (excluded below) for the same
            // wave-redraw reason. Feather runs at ALL zooms and only on a mesh REBUILD.

            QueryPerformanceCounter( &_pa );
            if ( Featherable( type ) && !IsOpenWater( type ) )
            {
                static const SDL_FPoint fuv[4] = { {0.f,0.5f}, {0.5f,0.f}, {1.f,0.5f}, {0.5f,1.f} };
                static const int nbrDX[4] = { 0, 1, 0, -1 };
                static const int nbrDY[4] = { -1, 0, 1, 0 };
                // Camera-rotation corner permutation — MUST mirror WorldToWindowHex's
                // xaaiIndex (terrain.cpp): world corner i → screen slot kCornerSlot[dir][i].
                // The feather bleeds along the diamond edge shared with map-neighbour e,
                // which is the WORLD edge (corner e, e+1) → screen slots kCornerSlot[dir][e]
                // and [e+1]. Omitting this made the bands attach to the wrong edges at
                // dirs 1/2/3 (the softening "broke" on rotation).
                static const int kCornerSlot[4][4] =
                    { { 0, 1, 2, 3 }, { 3, 0, 1, 2 }, { 2, 3, 0, 1 }, { 1, 2, 3, 0 } };
                const Uint8  kFeatherA = 135;    // visible but not a wash (205 was too strong, 70 too faint)
                const float  kBand     = 0.38f;  // band depth: edge → 38% toward centre
                float cxF = ( pts[0].x + pts[1].x + pts[2].x + pts[3].x ) * 0.25f;
                float cyF = ( pts[0].y + pts[1].y + pts[2].y + pts[3].y ) * 0.25f;

                for ( int e = 0; e < 4; ++e )
                {
                    CHexCoord nhc( hx + nbrDX[e], hy + nbrDY[e] ); nhc.Wrap( );
                    CHex* pn = theMap.GetHex( nhc );
                    if ( !pn ) continue;
                    CTerrainSprite* ns = pn->GetSprite( );
                    int ntype = ns ? ns->GetID( ) : -1;
                    if ( !Featherable( ntype ) )
                        continue;
                    // FEATHER_OUT coordination: a non-coastline tile does NOT feather
                    // toward a coastline neighbour — the coastline tile feathers itself
                    // into us (FEATHER_IN), so doing it from this side too would double
                    // the band at the shore seam.
                    if ( ntype == CHex::coastline && type != CHex::coastline )
                        continue;
                    // Only the coastline tile may pull OPEN water in (shore→sea blend);
                    // a land tile pulling ocean read as a smear, and water-as-receiver
                    // gets wiped by the wave re-draw.
                    if ( IsOpenWater( ntype ) && type != CHex::coastline )
                        continue;
                    const Tile* ntile = TileForHex( pn, aa.m_iDir );
                    if ( !ntile || !ntile->tex[zoom] || ntile == tile ) continue;

                    int   c0 = kCornerSlot[aa.m_iDir & 3][e];
                    int   c1 = kCornerSlot[aa.m_iDir & 3][( e + 1 ) & 3];
                    Uint8 g0 = 255, g1 = 255;   // full bright; fog dim is the overlay

                    // Inset points: a short way from each edge endpoint toward the
                    // tile centre. Edge endpoints full alpha → inset 0 → thin band.
                    float i0x = pts[c0].x + kBand * ( cxF - pts[c0].x );
                    float i0y = pts[c0].y + kBand * ( cyF - pts[c0].y );
                    float i1x = pts[c1].x + kBand * ( cxF - pts[c1].x );
                    float i1y = pts[c1].y + kBand * ( cyF - pts[c1].y );
                    SDL_FPoint u0 = fuv[c0], u1 = fuv[c1];
                    SDL_FPoint iu0 = { u0.x + kBand*(0.5f-u0.x), u0.y + kBand*(0.5f-u0.y) };
                    SDL_FPoint iu1 = { u1.x + kBand*(0.5f-u1.x), u1.y + kBand*(0.5f-u1.y) };

                    SDL_Vertex e0, e1, n0, n1;   // band quad = 2 tris
                    e0.position = { (float)pts[c0].x, (float)pts[c0].y }; e0.tex_coord = u0;  e0.color = { g0, g0, g0, kFeatherA };
                    e1.position = { (float)pts[c1].x, (float)pts[c1].y }; e1.tex_coord = u1;  e1.color = { g1, g1, g1, kFeatherA };
                    n0.position = { i0x, i0y };                          n0.tex_coord = iu0; n0.color = { g0, g0, g0, 0 };
                    n1.position = { i1x, i1y };                          n1.tex_coord = iu1; n1.color = { g1, g1, g1, 0 };

                    std::vector<SDL_Vertex>& fvb = rowFeather[ntile->tex[zoom]];   // batched within this row
                    fvb.push_back( e0 ); fvb.push_back( e1 ); fvb.push_back( n1 );
                    fvb.push_back( e0 ); fvb.push_back( n1 ); fvb.push_back( n0 );
                }
            }
            QueryPerformanceCounter( &_pb ); _accFeath += _pb.QuadPart - _pa.QuadPart;
        }

        drawRow();   // flush this row's base then feather batches (back-to-front)

        // Stop once we've passed the bottom of on-screen content. seenContent
        // guards against the empty rows in the top pan-margin band (which is now
        // off the top of the map) tripping an early break before we reach content.
        if ( rowAny ) seenContent = true;
        if ( seenContent && !rowAny )
            break;
        if ( y > iTopY + 4 * ( ws.cy / __max( 1, ( 16 >> zoom ) ) + 8 ) + 64 )
            break;  // hard safety bound (extra for the bottom pan margin)
    }
    Perf::CounterAdd( "reb.loop", (int)( ( GetTickCount( ) - _gt ) * 1000 ) ); _gt = GetTickCount( );
    Perf::CounterAdd( "rb.proj", (int)( _accProj * 1000000 / _qpf.QuadPart ) );      // PROFILE
    Perf::CounterAdd( "rb.feather", (int)( _accFeath * 1000000 / _qpf.QuadPart ) );  // PROFILE
    Perf::CounterAdd( "rb.hexes", _hexCnt );                                         // PROFILE

    // Precompute each fog hex's 8 neighbour indices (once, here), so the throttled
    // fog re-sample is 1 visibility read/hex + array lookups instead of 9 hashed
    // GetHex/hex — that was the bulk of the zoomed-out per-frame cost. Neighbour
    // order matches the corner formula below: 0:(-1,0) 1:(1,0) 2:(0,-1) 3:(0,1)
    // 4:(-1,-1) 5:(1,-1) 6:(1,1) 7:(-1,1).
    {
        const size_t nHex = s_fogHex.size( );
        s_fogVis.assign( nHex, 0.0f );
        s_fogNbr.assign( nHex * 8, -1 );
        // PERF: index hexes by map coord in a FLAT ARRAY (the map is square, hc.X()/Y()
        // are in [0,eX)) instead of a 50k-entry std::unordered_map rebuilt every time —
        // that hash map was ~22% of the whole rebuild in Debug. Reused static, cleared
        // each rebuild (eX*eY int writes, cheap).
        const int eX = theMap.Get_eX( ), eY = theMap.Get_eY( );
        static std::vector<int> s_hexIdx;
        s_hexIdx.assign( (size_t)eX * eY, -1 );
        for ( size_t i = 0; i < nHex; ++i )
        {
            CHexCoord hc = s_fogHex[i]->GetHex( );
            s_hexIdx[ (size_t)hc.Y( ) * eX + hc.X( ) ] = (int)i;
        }
        static const int ndx[8] = { -1, 1, 0, 0, -1, 1, 1, -1 };
        static const int ndy[8] = {  0, 0, -1, 1, -1, -1, 1, 1 };
        for ( size_t i = 0; i < nHex; ++i )
        {
            CHexCoord hc = s_fogHex[i]->GetHex( );
            int hx = hc.X( ), hy = hc.Y( );
            for ( int k = 0; k < 8; ++k )
            {
                CHexCoord n( hx + ndx[k], hy + ndy[k] ); n.Wrap( );
                s_fogNbr[i * 8 + k] = s_hexIdx[ (size_t)n.Y( ) * eX + n.X( ) ];
            }
        }
    }
    Perf::CounterAdd( "reb.fognbr", (int)( ( GetTickCount( ) - _gt ) * 1000 ) ); _gt = GetTickCount( );

    // Draw the freshly-built mesh INTO the texture (target is still s_rt).
    SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
    for ( auto& d : s_cache )
        SDL_RenderGeometry( r, d.first, d.second.data(), (int)d.second.size(), nullptr, 0 );

    // Slope-shade overlay → s_shadeRT, then BLUR it (down-/up-scale through a half-res
    // texture with bilinear filtering) so the per-tile flat shading feathers across
    // tile edges instead of stepping. Clear to white (=multiply by 1, no darkening);
    // draw shade diamonds with OVERWRITE (front-wins, like fog) so overlapping mountain
    // diamonds don't multiply-darken.
    SDL_SetRenderTarget( r, s_shadeRT );
    SDL_RenderSetViewport( r, nullptr );
    SDL_SetRenderDrawColor( r, 255, 255, 255, 255 );
    SDL_RenderClear( r );
    SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_NONE );
    if ( !s_shadeVerts.empty( ) )
        SDL_RenderGeometry( r, nullptr, s_shadeVerts.data( ), (int)s_shadeVerts.size( ), nullptr, 0 );
    // Blur: shadeRT → half (downscale) → shadeRT (upscale); bilinear does the blur.
    SDL_SetRenderTarget( r, s_shadeHalf );
    SDL_RenderCopy( r, s_shadeRT, nullptr, nullptr );
    SDL_SetRenderTarget( r, s_shadeRT );
    SDL_RenderCopy( r, s_shadeHalf, nullptr, nullptr );
    Perf::CounterAdd( "reb.gpu", (int)( ( GetTickCount( ) - _gt ) * 1000 ) );

    // Restore the window target/viewport. (The fog overlay is rendered in the common
    // tail below; the blit also happens there so the cached path shares it.)
    SDL_SetRenderTarget( r, prevTarget );
    SDL_RenderSetViewport( r, &savedVp );
    s_fogUpdAt = 0;       // force a fog (re)render of the fresh geometry this frame
    s_waterTick = curWaterTick;   // rebuild drew water at the current wave frame

    // Pre-group the water geometry by animation index ONCE per rebuild. Positions are
    // fixed in texture space (only the frame TEXTURE changes per wave-tick), so the
    // per-tick s_waterRT bake below just re-issues these cached arrays with the current
    // frame's texture — no per-hex CPU work. This is what removes the ~85ms/frame water
    // cost (it now runs ~4Hz as a handful of RenderGeometry calls, not 30k/frame).
    s_waterDiamGeom.assign( s_waterAnims.size( ), {} );
    s_waterBandGeom.assign( s_waterAnims.size( ), {} );
    {
        static const SDL_FPoint duv[4] = { {0.f,0.5f}, {0.5f,0.f}, {1.f,0.5f}, {0.5f,1.f} };
        const size_t nWGeom = s_waterHex.size( );
        for ( size_t i = 0; i < nWGeom; ++i )
        {
            int ai = s_waterAnimOf[i];
            if ( ai < 0 || ai >= (int)s_waterDiamGeom.size( ) ) continue;
            std::vector<SDL_Vertex>& wv = s_waterDiamGeom[ai];
            const CPoint* p = &s_waterPos[i * 4];
            SDL_Vertex a, b, c, d;
            a.position = { (float)p[0].x, (float)p[0].y }; a.tex_coord = duv[0]; a.color = { 255,255,255,255 };
            b.position = { (float)p[1].x, (float)p[1].y }; b.tex_coord = duv[1]; b.color = { 255,255,255,255 };
            c.position = { (float)p[2].x, (float)p[2].y }; c.tex_coord = duv[2]; c.color = { 255,255,255,255 };
            d.position = { (float)p[3].x, (float)p[3].y }; d.tex_coord = duv[3]; d.color = { 255,255,255,255 };
            wv.push_back( a ); wv.push_back( b ); wv.push_back( d );
            wv.push_back( b ); wv.push_back( c ); wv.push_back( d );
        }
        for ( const WaterBlendBand& wb : s_waterBlend )
        {
            if ( wb.animIdx < 0 || wb.animIdx >= (int)s_waterBandGeom.size( ) ) continue;
            std::vector<SDL_Vertex>& wv = s_waterBandGeom[wb.animIdx];
            SDL_Vertex e0, e1, n0, n1;
            e0.position = { (float)wb.p[0].x, (float)wb.p[0].y }; e0.tex_coord = wb.uv[0]; e0.color = { 255,255,255,kWaterFeatherA };
            e1.position = { (float)wb.p[1].x, (float)wb.p[1].y }; e1.tex_coord = wb.uv[1]; e1.color = { 255,255,255,kWaterFeatherA };
            n0.position = { (float)wb.p[2].x, (float)wb.p[2].y }; n0.tex_coord = wb.uv[2]; n0.color = { 255,255,255,0 };
            n1.position = { (float)wb.p[3].x, (float)wb.p[3].y }; n1.tex_coord = wb.uv[3]; n1.color = { 255,255,255,0 };
            wv.push_back( e0 ); wv.push_back( e1 ); wv.push_back( n1 );
            wv.push_back( e0 ); wv.push_back( n1 ); wv.push_back( n0 );
        }
    }
    s_waterRTtick = ~0u;   // force a re-bake of s_waterRT this frame (new geometry)
    }   // end if ( needRebuild )

    // --- Edit-patch (Item 5): re-mesh ONLY the edited hexes into s_rt instead of a full
    // ~50k-hex rebuild. v1 draws the base tile (skips feather/shade — minor edge-softening
    // / shade staleness on the few changed hexes; fog positions are stable + re-sampled
    // separately). Texture pos = window pos - pan(dX,dY) + margin(offX,offY). This is what
    // turns a combat-crater / road / building / rocket edit from an ~850ms freeze into ~ms.
    if ( doPatch )
    {
        Perf::ScopeCounter _cp( "t.patch" );
        SDL_Texture* pt = SDL_GetRenderTarget( r );
        SDL_Rect     vp; SDL_RenderGetViewport( r, &vp );
        SDL_SetRenderTarget( r, s_rt );
        SDL_RenderSetViewport( r, nullptr );
        SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
        const SDL_Color white = { 255, 255, 255, 255 };
        const int       idx[6] = { 0, 1, 3, 1, 2, 3 };
        // Swap the shared list into a local under the lock, then patch unlocked (the sim
        // thread keeps pushing new edits into the now-empty shared list meanwhile).
        std::vector<unsigned> edited;
        { std::lock_guard<std::mutex> lk( g_enEditMutex ); edited.swap( g_enEditedHexes ); }
        for ( unsigned packed : edited )
        {
            CHexCoord hc( (int)( packed >> 16 ), (int)( packed & 0xFFFF ) );
            CPoint    pts[4];
            if ( !aa.MapToWindowHex( hc, pts ) ) continue;
            CHex* phex = theMap.GetHex( hc );
            if ( !phex ) continue;
            const Tile* tile = TileForHex( phex, aa.m_iDir );
            if ( !tile || !tile->tex[zoom] ) continue;
            for ( int i = 0; i < 4; ++i ) { pts[i].x += offX - dX; pts[i].y += offY - dY; }
            SDL_Vertex v[4];
            v[0].position = { (float)pts[0].x, (float)pts[0].y }; v[0].tex_coord = { 0.0f, 0.5f }; v[0].color = white;
            v[1].position = { (float)pts[1].x, (float)pts[1].y }; v[1].tex_coord = { 0.5f, 0.0f }; v[1].color = white;
            v[2].position = { (float)pts[2].x, (float)pts[2].y }; v[2].tex_coord = { 1.0f, 0.5f }; v[2].color = white;
            v[3].position = { (float)pts[3].x, (float)pts[3].y }; v[3].tex_coord = { 0.5f, 1.0f }; v[3].color = white;
            SDL_RenderGeometry( r, tile->tex[zoom], v, 4, idx, 6 );
        }
        SDL_SetRenderTarget( r, pt );
        SDL_RenderSetViewport( r, &vp );
        s_builtEditGen += (unsigned)edited.size( );   // account for exactly the patched edits
        // CRITICAL: keep s_sig's edit-gen bits in sync with s_builtEditGen. baseSame is
        // tested next frame as ( sigNoEdit == lastNoEdit ), where
        //   lastNoEdit = s_sig ^ (s_builtEditGen << 40).
        // The rebuild path sets s_sig AND s_builtEditGen together; the patch path only
        // advanced s_builtEditGen, leaving s_sig encoding the OLD edit-gen. That made
        // lastNoEdit != sigNoEdit every frame after a patch → a spurious full ~1.6s
        // rebuild after EVERY edit (the ~1fps zoomed-out stutter). Re-encode s_sig with
        // the pure base (sigNoEdit) XOR the new built edit-gen so next frame matches.
        s_sig = sigNoEdit ^ ( (uint64_t)s_builtEditGen << 40 );
        // (no clear — the swap already emptied the shared list; edits the sim pushed during
        //  the patch stay there and are handled next frame)
    }

    // (Water is no longer re-baked into s_rt — it's drawn as a live layer UNDER the
    // cached terrain in the composite below, which keeps it synced + lets the
    // water<->water blend animate without being clobbered.)

    // --- Fog overlay: re-sample + re-render on a fast throttle (NOT tied to the
    // terrain rebuild), so unit vision tracks within ~kFogThrottle ms while the
    // heavy terrain texture stays cached. Positions were captured during the build;
    // here we only rewrite per-corner alpha and redraw the small untextured mesh.
    // Skip the whole fog pass when fog-of-war hasn't changed since the last render
    // (g_enFogVisGen): the persistent s_fogRT still composites the previous fog below,
    // so a static view pays nothing here — this is the dominant zoomed-out cost when
    // the fog mesh is ~all visible hexes. A rebuild always refreshes (new geometry); a
    // 1 s force-refresh self-heals any cross-thread missed bump. The kFogThrottle still
    // caps the rate when fog IS changing (active play).
    static unsigned s_fogVisGenSeen = ~0u;
    const DWORD     kFogForceMs     = 1000;
    bool fogChanged = needRebuild || ( g_enFogVisGen != s_fogVisGenSeen ) ||
                      ( _nowT - s_fogUpdAt ) >= kFogForceMs;
    if ( fogChanged && ( _nowT - s_fogUpdAt ) >= kFogThrottle && !s_fogVerts.empty( ) )
    {
        s_fogVisGenSeen = g_enFogVisGen;
        // S3 incremental fog: re-sample all quads but collect ONLY the ones whose alpha
        // actually CHANGED into a batch, and re-draw just those into the persistent
        // s_fogRT. OVERWRITE blend replaces each changed diamond's pixels in place (the
        // diamonds tessellate, so the unchanged neighbours' pixels stay valid) — no clear
        // needed, and the cost is ∝ the moving fog frontier, not all ~50k diamonds. This
        // works even when changes are scattered across the map (a clipped bbox would just
        // cover the whole viewport then). Tall-tile altitude overlap can leave a tiny
        // artifact at a mountain fog edge — acceptable (dim, rare) vs redrawing everything.
        static std::vector<SDL_Vertex> s_fogBatch;
        s_fogBatch.clear( );
        // Re-sample fog alphas EVERY update — on a rebuild too (the build now skips the
        // 9-GetHex/hex sampling and pushes placeholder alphas, filled here via the fast
        // precomputed neighbours). The change-batch is only consumed on the non-rebuild path.
        {
            const size_t nHex = s_fogHex.size( );
            // 1 visibility read per hex (cached CHex*, no GetHex/hash).
            for ( size_t i = 0; i < nHex; ++i )
                s_fogVis[i] = ( s_fogHex[i] && s_fogHex[i]->GetVisibility( ) ) ? 1.0f : 0.0f;
            // Per-corner fog from self + precomputed neighbours (array lookups only).
            auto A = []( float f ) -> Uint8 { return (Uint8)__min( 255, (int)( ( 1.0f - f ) * 255.0f ) ); };
            const int* fperm = kFogSlotInv[aa.m_iDir & 3];
            for ( size_t i = 0; i < nHex; ++i )
            {
                const int* nb = &s_fogNbr[i * 8];
                float s = s_fogVis[i];
                auto nv = [&]( int k ) -> float { return nb[k] >= 0 ? s_fogVis[nb[k]] : s; };
                // nb: 0:(-1,0)1:(1,0)2:(0,-1)3:(0,1)4:(-1,-1)5:(1,-1)6:(1,1)7:(-1,1)
                float f0 = kFogDim + ( 1.0f - kFogDim ) * ( s + nv(0) + nv(2) + nv(4) ) * 0.25f;
                float f1 = kFogDim + ( 1.0f - kFogDim ) * ( s + nv(1) + nv(2) + nv(5) ) * 0.25f;
                float f2 = kFogDim + ( 1.0f - kFogDim ) * ( s + nv(1) + nv(3) + nv(6) ) * 0.25f;
                float f3 = kFogDim + ( 1.0f - kFogDim ) * ( s + nv(0) + nv(3) + nv(7) ) * 0.25f;
                // World-corner alphas → screen slots (same permutation as the build path).
                float fc[4] = { f0, f1, f2, f3 };
                Uint8 a0 = A( fc[fperm[0]] ), a1 = A( fc[fperm[1]] ),
                      a2 = A( fc[fperm[2]] ), a3 = A( fc[fperm[3]] );
                SDL_Vertex* v = &s_fogVerts[i * 6];   // order L,T,B, T,R,B (slots 0,1,3,1,2,3)
                bool changed = ( v[0].color.a != a0 || v[1].color.a != a1 ||
                                 v[4].color.a != a2 || v[2].color.a != a3 );
                v[0].color.a = a0; v[1].color.a = a1; v[2].color.a = a3;
                v[3].color.a = a1; v[4].color.a = a2; v[5].color.a = a3;
                if ( changed )
                    for ( int k = 0; k < 6; ++k )
                        s_fogBatch.push_back( v[k] );
            }
        }
        s_fogUpdAt = _nowT;

        // Re-render: a rebuild redraws the whole (new) mesh; otherwise only the changed
        // diamonds (skip entirely when nothing changed — s_fogRT persists).
        if ( needRebuild || !s_fogBatch.empty( ) )
        {
            SDL_Texture* pt = SDL_GetRenderTarget( r );
            SDL_Rect     vp; SDL_RenderGetViewport( r, &vp );
            SDL_SetRenderTarget( r, s_fogRT );
            SDL_RenderSetViewport( r, nullptr );
            // OVERWRITE (no blend), NOT alpha-blend: altitude-raised diamonds OVERLAP on
            // tall mountains and blending would ACCUMULATE alpha into black wedges. Build
            // order is back-to-front so overwrite = the front tile's fog wins. The texture
            // keeps BLENDMODE_BLEND so the final dim composites over the terrain.
            SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_NONE );
            if ( needRebuild )
            {
                SDL_SetRenderDrawColor( r, 0, 0, 0, 0 );   // transparent = no dim
                SDL_RenderClear( r );
                SDL_RenderGeometry( r, nullptr, s_fogVerts.data( ), (int)s_fogVerts.size( ), nullptr, 0 );
            }
            else
            {
                SDL_RenderGeometry( r, nullptr, s_fogBatch.data( ), (int)s_fogBatch.size( ), nullptr, 0 );
            }
            SDL_SetRenderTarget( r, pt );
            SDL_RenderSetViewport( r, &vp );
        }
    }

    // Composite at the current pan offset (one GPU copy each, ~1 µs): terrain tiles,
    // then the blurred slope-shade (MOD = multiply), then fog (BLEND = dim). The
    // shade texture is flipped to MOD here (it stays NONE for its blur round-trip).
    SDL_Rect dst = { dX - kMarginPx, dY - kMarginPx, rtW, rtH };

    // --- WATER (drawn UNDER the cached terrain): bake the animated water into s_waterRT
    // (texture space, like s_rt), but ONLY when the wave frame advanced (~4Hz) or the mesh
    // was rebuilt. The diamond/band geometry is pre-grouped by anim index at rebuild, so a
    // bake is just a handful of RenderGeometry of cached arrays with the current frame's
    // texture — NOT 30k hex diamonds rebuilt every frame (that was the ~85ms/frame wall).
    // Per frame we then blit s_waterRT at the pan offset (one GPU copy), in the composite.
    if ( s_waterRT && !s_waterHex.empty( ) )
    {
        Perf::CounterAdd( "water.hexes", (int)s_waterHex.size( ) );
        const unsigned wbase = (unsigned)( theGame.GetFrame( ) / 6 );   // shared wave-frame (kWaterHold=6)
        if ( needRebuild || wbase != s_waterRTtick )
        {
            Perf::ScopeCounter _cw( "p.waterbake" );   // MEASURE: water bake (~4Hz, not per-frame)
            SDL_Texture* pt = SDL_GetRenderTarget( r );
            SDL_Rect     vp; SDL_RenderGetViewport( r, &vp );
            SDL_SetRenderTarget( r, s_waterRT );
            SDL_RenderSetViewport( r, nullptr );
            SDL_SetRenderDrawColor( r, 0, 0, 0, 0 );   // transparent: only water hexes get filled
            SDL_RenderClear( r );
            SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
            for ( size_t ai = 0; ai < s_waterDiamGeom.size( ); ++ai )   // opaque water diamonds
            {
                std::vector<SDL_Vertex>& wv = s_waterDiamGeom[ai];
                if ( wv.empty( ) ) continue;
                const WaterAnim& wa = s_waterAnims[ai];
                const Tile* wt = wa.nFrames > 0 ? wa.frame[ wbase % wa.nFrames ] : nullptr;
                if ( !wt || !wt->tex[zoom] ) continue;
                SDL_RenderGeometry( r, wt->tex[zoom], wv.data( ), (int)wv.size( ), nullptr, 0 );
            }
            for ( size_t ai = 0; ai < s_waterBandGeom.size( ); ++ai )   // then water<->water blend bands
            {
                std::vector<SDL_Vertex>& wv = s_waterBandGeom[ai];
                if ( wv.empty( ) ) continue;
                const WaterAnim& wa = s_waterAnims[ai];
                const Tile* nt = wa.nFrames > 0 ? wa.frame[ wbase % wa.nFrames ] : nullptr;
                if ( !nt || !nt->tex[zoom] ) continue;
                SDL_RenderGeometry( r, nt->tex[zoom], wv.data( ), (int)wv.size( ), nullptr, 0 );
            }
            SDL_SetRenderTarget( r, pt );
            SDL_RenderSetViewport( r, &vp );
            s_waterRTtick = wbase;
        }
    }

    { Perf::ScopeCounter _cc( "p.compose" );   // MEASURE: 4 full-screen RT blits
    if ( s_waterRT && !s_waterHex.empty( ) )   // water UNDER terrain (shows through s_rt's holes)
        SDL_RenderCopy( r, s_waterRT, nullptr, &dst );
    SDL_RenderCopy( r, s_rt, nullptr, &dst );
    SDL_SetTextureBlendMode( s_shadeRT, SDL_BLENDMODE_MOD );
    SDL_RenderCopy( r, s_shadeRT, nullptr, &dst );
    SDL_SetTextureBlendMode( s_shadeRT, SDL_BLENDMODE_NONE );
    SDL_RenderCopy( r, s_fogRT, nullptr, &dst ); }

    // Cursor footprint hatch — live overlay, every frame (animated; shows on a static
    // view). Cheap no-op when not placing a building/rocket.
    DrawBuildCursorOverlay( r, aa );
}
