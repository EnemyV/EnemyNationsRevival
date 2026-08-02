#include "stdafx.h"
#include "en_logpath.h"   // EnLogPath - logs to the launch dir, not the exe dir
#include "enprobes.h"   // EN_GAMEPLAY_PROBES
#include "SDL2Terrain.h"
#include "base.h"      // CAnimAtr, CViewHexCoord, CHexCoord
#include "terrain.h"   // CHex, theMap
#include "terrain.inl"
#include "player.h"    // theGame.GetFrame() — water wave animation clock
#include "Perf.h"      // terrain sub-phase profiling counters

#include <SDL.h>
#include <vector>
#include <map>             // s_rctx — per-renderer terrain context registry
#include <utility>         // std::swap (incl. the array overload)
#include <unordered_set>   // g_enMeshEditSet — dedup the retained-mesh edit list
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

// PERF: this TU is the GPU-terrain hot path — the per-hex mesh rebuild dominates zoom-out time,
// and in a Debug build (/Od /RTC1) it is CALL-OVERHEAD bound (per-hex helper calls + ~1M STL
// push_backs), not memory-bandwidth bound. So optimize JUST this file; the rest of the game keeps
// /Od /RTC1 for debuggability. (runtime_checks must be off to pair with optimization under /RTC1.)
// Release/Profile are already /O2, where these pragmas are harmless no-ops.
#if defined( _MSC_VER )
#pragma runtime_checks( "", off )
#pragma optimize( "gt", on )
#endif

std::unordered_map<std::string, SDL2Terrain::Tile> SDL2Terrain::s_tiles;
std::unordered_map<std::string, const SDL2Terrain::Tile*> SDL2Terrain::s_byType;
std::unordered_map<std::string, const SDL2Terrain::Tile*> SDL2Terrain::s_byTypeVar;
SDL_Renderer* SDL2Terrain::s_renderer = nullptr;
bool          SDL2Terrain::s_loaded   = false;
bool          SDL2Terrain::s_anyLoaded = false;

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

// Item 5 (incremental terrain patch): a LOG of edited hexes, packed (x<<16)|y (map is
// square, coords < 65536), recorded by CHex::SetAlt/SetVisibleType via g_enEditHex.
// MULTI-READER: entry i holds the edit with gen (g_enEditBaseGen + i + 1); each area
// map keeps its own cursor (its RCtx builtEditGen) and slices only its pending tail.
// The old clear-on-consume model let the FIRST map to rebuild eat the list, so a second
// open map's tile memo never invalidated the edited hexes and its terrain went
// permanently stale. Nobody clears on consume; the render thread trims the entries
// every live map has consumed (end of SDL2Terrain::Render). Recording stops when the
// log hits kEditLogCap (worldgen floods) — the gap makes lagging readers take a full
// rebuild, and the trim re-anchors the log empty at the current gen afterwards.
std::vector<unsigned> g_enEditedHexes;
unsigned              g_enEditBaseGen = 0;   // gen of the (dropped) entry before g_enEditedHexes[0]
// Retained-mesh edit list: like g_enEditedHexes, but accumulates edits since the last FULL
// retained-mesh CAPTURE (not consumed by the s_rt edit-patch, not cleared on a zoom replay)
// so a zoom REPLAY — whose cells are at the capture's edit state — can re-apply every
// since-capture edit over the stale cells. Cleared only on a capturing rebuild. Past the cap
// we can't track them all, so g_enMeshEditOverflow forces a full rebuild instead of a replay.
std::vector<unsigned> g_enMeshEdits;
std::unordered_set<unsigned> g_enMeshEditSet;   // DEDUP: a hex re-edited many times (e.g. crop
                                                // grow stages, repeated alt sets) counts ONCE, so
                                                // the list stays bounded by UNIQUE edited hexes.
bool                  g_enMeshEditOverflow = false;
// Which context's capture the mesh-edit list is anchored to (compare-only, never deref):
// the list is cleared on a retained-mesh CAPTURE, so a replay may re-apply it only in the
// SAME context that captured — any other context treats it as overflowed (full rebuild).
static const void*    g_enMeshEditAnchor = nullptr;
std::mutex            g_enEditMutex;   // sim thread pushes; render thread reads/slices
const size_t          kEditPatchCap = 1024;    // max pending edits a ctx will PATCH (vs rebuild)
const size_t          kEditLogCap   = 4096;    // log ceiling — past it the log gaps (see above)
const size_t          kMeshEditCap  = 16384;   // unique-hex ceiling for the replay re-apply list
void g_enEditHex( int x, int y )
{
    // Bump the gen AND record under the same lock so the render can read them atomically
    // (a gen ahead of the list looked like a non-recorded edit → spurious full rebuild).
    std::lock_guard<std::mutex> lk( g_enEditMutex );
    ++g_enTerrainEditGen;
    unsigned packed = ( (unsigned)( x & 0xFFFF ) << 16 ) | (unsigned)( y & 0xFFFF );
    // Append only while the log is CONTIGUOUS (covers gens base+1 .. gen-1) and under
    // its cap. Once it gaps, readers below the gap detect incompleteness and take a
    // full rebuild; the trim (end of Render) re-anchors the log once everyone catches up.
    if ( g_enEditBaseGen + g_enEditedHexes.size( ) == g_enTerrainEditGen - 1
         && g_enEditedHexes.size( ) < kEditLogCap )
        g_enEditedHexes.push_back( packed );
    // g_enMeshEdits: only push a hex the FIRST time it's edited since the last capture. Without
    // this, repeated edits to the same hexes (crop stages, terraform passes) blew past the cap
    // in seconds → g_enMeshEditOverflow → every zoom forced a full ~11s capture instead of a
    // fast replay. Dedup keeps the list ~= the count of distinct changed hexes (small).
    if ( g_enMeshEditSet.insert( packed ).second )
    {
        if ( g_enMeshEdits.size( ) < kMeshEditCap )
            g_enMeshEdits.push_back( packed );
        else
            g_enMeshEditOverflow = true;   // too many DISTINCT edited hexes → next zoom rebuilds
    }
}

// Persistent terrain TILE memo: TileForHex(hex,dir) result per map hex, PER CAMERA DIRECTION
// (4 slices). TileForHex depends only on hex content + camera dir (NOT zoom/pan), so each slice
// PERSISTS across zoom/pan rebuilds — a zoom-out re-hits cached tiles instead of recomputing ~48k
// (the largest single zoom-out cost) — and ROTATE just switches slices instead of wiping (each
// direction stays warm once visited; ~8MB total on a 512x512 map). File scope so BOTH the full
// rebuild and the fast edit-patch can invalidate it. Whole-memo invalidation = load/overflow;
// per-hex in ALL 4 slices = terrain edit.
static std::vector<const SDL2Terrain::Tile*> s_tileCache;     // eX*eY*4
static std::vector<uint32_t>                 s_tileGen;       // eX*eY*4
static uint32_t                              s_tileGenCur[4] = { 0, 0, 0, 0 };
static int                                   s_tileCacheEX = 0, s_tileCacheEY = 0;
static inline void TileMemoInvalidate( int x, int y )   // drop hex (x,y) + its 8 neighbours, all 4 dirs
{
    if ( s_tileGen.empty( ) ) return;
    const size_t plane = (size_t)s_tileCacheEX * s_tileCacheEY;
    for ( int dy = -1; dy <= 1; ++dy )
      for ( int dx = -1; dx <= 1; ++dx )
      { int nx = x + dx, ny = y + dy;
        if ( (unsigned)nx < (unsigned)s_tileCacheEX && (unsigned)ny < (unsigned)s_tileCacheEY )
            for ( int d = 0; d < 4; ++d )
                s_tileGen[ plane * d + (size_t)ny * s_tileCacheEX + nx ] = 0u; }   // 0 != gen(>=1) → recompute
}

// Shared static index buffer for QUAD batches (4 verts/quad -> 2 tris). Emitting 4 unique corner
// vertices + 6 indices instead of 6 duplicated vertices cuts the per-hex vertex VOLUME (and
// push_back count) by a third. The rebuild is memory-bandwidth bound under multi-AI load, so this
// reduces both the work AND the bandwidth it contends for with the AI threads — helps Debug AND
// Release. Index pattern {0,1,3, 1,2,3} matches the L,T,R,B corner order (tris L-T-B and T-R-B).
// Vertex-buffer capacity pool. The per-row batch maps (rowBase/rowFeather) and s_cache are
// LOCALS rebuilt from empty every mesh rebuild — in a Debug build (/MDd heap) the realloc
// churn of thousands of tiny growing vectors dominated the zoomed-in rebuild (rb.asm was
// ~27x more per hex at z0 than z3, where long same-texture runs amortize the growth).
// Recycle the buffers' CAPACITY across rebuilds instead.
static std::vector<std::vector<SDL_Vertex>> s_vbPool;
static inline void VbAcquire( std::vector<SDL_Vertex>& vb, size_t reserveN )
{
    if ( vb.capacity( ) ) return;
    if ( !s_vbPool.empty( ) ) { vb.swap( s_vbPool.back( ) ); s_vbPool.pop_back( ); vb.clear( ); }
    else vb.reserve( reserveN );
}
static inline void VbRecycle( std::vector<SDL_Vertex>& vb )
{
    if ( s_vbPool.size( ) < 96 && vb.capacity( ) ) { vb.clear( ); s_vbPool.push_back( std::move( vb ) ); }
}

// Missing-mip fallback: the diamond UVs are relative (0 / 0.5 / 1), so ANY zoom level
// of the same art renders correctly — SDL just scales it. A missing/failed baked PNG at
// one zoom must not drop the hex: that was the "black diamond" bug (the hex was culled
// from BOTH the cached terrain texture and the live water layer, leaving the backdrop —
// black — showing through a diamond-shaped hole).
static inline SDL_Texture* TileTexAnyZoom( const SDL2Terrain::Tile* t, int zoom )
{
    if ( !t ) return nullptr;
    SDL_Texture* tex = t->tex[zoom];
    for ( int z = 0; !tex && z < SDL2Terrain::NUM_ZOOMS; ++z ) tex = t->tex[z];
    return tex;
}

static std::vector<int> s_quadIdx;
static const int* QuadIndices( int nQuads )
{
    int have = (int)s_quadIdx.size( ) / 6;
    if ( have < nQuads )
        for ( int q = have; q < nQuads; ++q )
        { int b = q * 4;
          s_quadIdx.push_back( b + 0 ); s_quadIdx.push_back( b + 1 ); s_quadIdx.push_back( b + 3 );
          s_quadIdx.push_back( b + 1 ); s_quadIdx.push_back( b + 2 ); s_quadIdx.push_back( b + 3 ); }
    return s_quadIdx.data( );
}

// GPU device-lost (SDL_RENDER_TARGETS_RESET / SDL_RENDER_DEVICE_RESET, set from the
// event pump via SDL2Terrain::NotifyTargetsLost): target textures survive as objects but
// their CONTENTS are gone — every cached layer must re-render or it presents black.
// A GENERATION counter, not a one-shot bool: the bool was consumed by whichever area
// map rendered first, so a SECOND open area map never invalidated and kept presenting
// its dead (black) caches. Each context compares its own seen-gen in Render.
static unsigned s_targetsLostGen = 0;
void SDL2Terrain::NotifyTargetsLost() { ++s_targetsLostGen; }

// FAR SNAPSHOT: flattened copy of the terrain composite from the last wide (zoom>=2) full
// rebuild. During a zoom-OUT gesture preview the crisp texture only covers the old (narrower)
// view; this snapshot underlays the rest of the window — slightly stale, blurrier terrain
// instead of black "uninitialized" border. Valid only for the direction it was captured at.
static SDL_Texture* s_farRT = nullptr;
static int       s_farZoom = -1, s_farDir = -1, s_farMargin = 0, s_farW = 0, s_farH = 0;
static unsigned  s_farLoadGen = ~0u;
static CHexCoord s_farRefHex;          // anchor hex + its window px at snapshot time
static CPoint    s_farRefPx( 0, 0 );

// Set by SDL2Terrain::Render each frame: TRUE when the visible view scrolled since last
// frame (the cached terrain blits at a different pan offset). The GPU sprite layer reads it
// to force a FULL re-emit on pan — its render target (g_rt) is screen-space and persistent,
// so a pan invalidates ALL of it; the incremental per-dirty-rect clear (which uses the new
// UL) otherwise misses old sprite positions and leaves ghosts. Authoritative because it's the
// terrain's actual blit delta, not the panel UL (which can lag the visible scroll).
bool g_enViewScrolled = false;

// Bumped by the sim when a STATIC sprite is removed from a hex (e.g. a road built over a
// forest hex clears its tree). Static sprites (trees/bridges) persist across incremental
// sprite captures, so nothing would otherwise drop the now-gone tree until a zoom/dir
// change. DiscoverSpritesGpu forces ONE full capture per VIEW when its seen-gen lags
// (aa.m_capStaticDirtySeen) — a one-shot bool was consumed by whichever area map captured
// first, leaving the ghost tree on every other open map. Reload is unaffected — the hex
// type is saved as `road`, so a fresh capture never emits the tree.
unsigned g_enStaticDirtyGen = 0;

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

// GESTURE PREVIEW + DEFERRED REBUILD (EN_PREVIEW, default ON). A zoom/rotate no longer
// rebuilds the mesh synchronously on the frame it happens: the existing composite is presented
// SCALED to the new zoom (instant response), and the real rebuild runs once the gesture has
// settled (no view change for kSettleMs). Rapid multi-notch gestures thus COALESCE into one
// rebuild instead of one per notch. Set EN_PREVIEW=0 to restore synchronous rebuilds.
static bool TerrainPreviewEnabled( )
{
    static int s_on = -1;
    if ( s_on < 0 ) { const char* e = SDL_getenv( "EN_PREVIEW" ); s_on = ( e && *e && *e == '0' ) ? 0 : 1; }
    return s_on != 0;
}

// RETAINED CONTENT-SPACE MESH (EN_RETAIN, default OFF until proven at parity). When on,
// a full rebuild also captures each hex's tile + CONTENT-space corners; a later ZOOM-only
// change then re-emits from those cells (screen = (content >> zoom) - UL, tex = tile->tex
// [zoom]) instead of running the ~15s per-hex loop. Built up layer by layer (base first).
static bool TerrainRetainEnabled( )
{
    static int s_on = -1;
    if ( s_on < 0 ) { const char* e = SDL_getenv( "EN_RETAIN" ); s_on = ( e && *e && *e == '1' ) ? 1 : 0; }
    return s_on != 0;
}

// Incremental pan ("strip rebuild"): when the view pans past the cached-texture margin
// with the SAME zoom/dir, scroll the cached textures by the pan delta and mesh only the
// newly-revealed edge strip instead of re-meshing the whole ~56k-hex view. Opt-in until
// validated (default OFF). Set EN_PANSTRIP=1 to enable.
static bool TerrainPanStripEnabled( )
{
    // Default ON: when a pan crosses the cached-texture margin (same zoom/dir), scroll the
    // cached terrain/shade textures by the pan delta and re-mesh only the newly-revealed edge
    // strip instead of the whole ~56k-hex view — a small pan costs ~90ms vs ~640ms full.
    // Set EN_PANSTRIP=0 to disable. KNOWN LIMITATION: AI terrain edits (roads) in the already-
    // scrolled region are briefly stale until the next full rebuild (zoom/dir/large pan).
    static int s_on = -1;
    if ( s_on < 0 ) { const char* e = SDL_getenv( "EN_PANSTRIP" ); s_on = ( e && *e && *e == '0' ) ? 0 : 1; }
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
#if EN_GAMEPLAY_PROBES
    std::ofstream log( EnLogPath( "SDL2Terrain.log" ).c_str(), std::ios::app );
    if ( log.is_open() )
        log << msg << std::endl;
#else
    (void)msg;   // release: terrain/renderer lifecycle chatter compiled out
#endif
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
    const char* kProbe = "/plain_0_aa010000_z0.png";

    // 1) Working-directory-relative (original behaviour: cwd = d:\Enemy Nations).
    for ( const char* c : candidates )
        if ( FileExists( std::string( c ) + kProbe ) )
            return c;

    // 2) EXE-relative — robust to the launch working directory. The game is often
    // started with cwd = the Debug\ exe folder (not the run dir), where none of the
    // cwd-relative candidates resolve, so the baked terrain set wasn't found and the
    // area map rendered black. Walk up from the exe directory trying the same
    // candidates at each level so the assets are found regardless of cwd.
    char exePath[MAX_PATH];
    DWORD n = GetModuleFileNameA( NULL, exePath, MAX_PATH );
    if ( n > 0 && n < MAX_PATH )
    {
        std::string dir( exePath, n );
        size_t slash = dir.find_last_of( "\\/" );
        if ( slash != std::string::npos )
            dir = dir.substr( 0, slash );   // exe directory

        for ( int up = 0; up < 8 && !dir.empty(); ++up )
        {
            for ( const char* c : candidates )
            {
                std::string cand = dir + "/" + c;
                if ( FileExists( cand + kProbe ) )
                    return cand;
            }
            size_t s = dir.find_last_of( "\\/" );
            if ( s == std::string::npos )
                break;
            dir = dir.substr( 0, s );        // go up one directory
        }
    }

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
        // The under-construction PREVIEW road (r_path) is a non-directional site overlay:
        // only the dir-0 ("aa") variant is baked. Applying camera rotation made the rotated
        // views miss it and fall back to GetDefaultForType — a COMPLETED road. Pin it to the
        // dir-0 variant so the preview shows the construction graphic at every rotation.
        if ( F == CHex::r_path ) viewDir = 0;
        const Tile* t = Get( "road", srcDir, std::string( kDirPrefix[viewDir] ) + "010000" );
        // Rotation-symmetric road shapes — the 4-way junction (r_x, srcDir 3) and the
        // r_path preview (srcDir 4) — bake ONLY the "aa" view. A rotated camera
        // (viewDir 1-3) finds no ac/ae/ag art and would fall through to the generic
        // default road tile (the dotted-preview look) — so the 4-way junction only
        // rendered correctly at camera dir 0. Fall back to this shape's own "aa" view
        // first (same pattern as the coastline lookup below).
        if ( !t ) t = Get( "road", srcDir, "aa010000" );
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
        // ROTATION-SYMMETRIC variants (3/7/11 — single F index, no MakeRotated gaps in
        // the table above) ship only the "aa" view: the original draws that one view at
        // every camera dir. Without this fallback a rotated camera (viewDir != 0) found
        // no art at all and fell to the variant-0 default — a wrong-shaped shore.
        if ( !t ) t = Get( "coastline", srcDir, std::string( "aa00" ) + fr + "000" );
        if ( !t ) t = Get( "coastline", srcDir, "aa00a000" );
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

// ---- WHOLE-MAP FAR UNDERLAY ---------------------------------------------------------
// A low-res render of the ENTIRE map (z3 art, down-scaled to fit a <=4096px texture),
// one texture per camera direction, per-hex fog-of-war dim baked into the vertex colour.
// Drawn UNDER the zoom-gesture preview: a fast zoom-out's uncovered ring then shows
// blurry-but-real terrain EVERYWHERE instead of black "uninitialized" squares — even on
// the very first gesture after load. The first build after a load runs in ONE burst
// (lands inside the load reveal — "prewarm during loading is fine"); a build for a newly
// visited camera direction is TIME-SLICED (kMapRowsSlice rows/frame) so rotates stay
// <300ms. Terrain edits after the build leave it slightly stale — acceptable for a
// transient backdrop; it refreshes on the next load/dir (re)build.
static SDL_Texture* s_mapRT[4]      = { nullptr, nullptr, nullptr, nullptr };
static unsigned     s_mapLoadGen[4] = { ~0u, ~0u, ~0u, ~0u };
static unsigned     s_mapFogGen[4]  = { ~0u, ~0u, ~0u, ~0u };   // g_enFogVisGen baked into this dir's underlay
static int          s_mapRow[4]     = { 0, 0, 0, 0 };   // next map row to mesh; >= eY = done
static CPoint       s_mapOrg[4];                        // content(z0) bbox origin (after pad)
static float        s_mapScale[4]   = { 0, 0, 0, 0 };   // content-z3 px -> texture px
static int          s_mapTexW[4]    = { 0, 0, 0, 0 };
static int          s_mapTexH[4]    = { 0, 0, 0, 0 };
static CPoint       s_mapPerX[4];                       // content shift of one full map X-wrap
static CPoint       s_mapPerY[4];                       // content shift of one full map Y-wrap

// Persistent-tile-memo lookup usable outside the rebuild loop (same slices/gens as the
// rebuild's cachedTile lambda — so the underlay build also WARMS the memo for its dir,
// making a later rotate's full rebuild cheaper).
static const SDL2Terrain::Tile* MemoTile( CHex* ph, int dir )
{
    if ( s_tileGen.empty( ) || s_tileGenCur[dir & 3] == 0 )
        return SDL2Terrain::TileForHex( ph, dir );
    CHexCoord hc = ph->GetHex( );
    size_t k = (size_t)s_tileCacheEX * s_tileCacheEY * ( dir & 3 )
             + (size_t)hc.Y( ) * s_tileCacheEX + hc.X( );
    if ( s_tileGen[k] == s_tileGenCur[dir & 3] ) return s_tileCache[k];
    const SDL2Terrain::Tile* t = SDL2Terrain::TileForHex( ph, dir );
    s_tileCache[k] = t; s_tileGen[k] = s_tileGenCur[dir & 3];
    return t;
}

// Tiles whose foam is ANIMATED and so must live in the live wave layer (a HOLE in
// the cached s_rt, re-drawn into s_waterRT on each wave-tick) instead of being baked
// statically. Open water plus the coastline (shore) tile — the latter is an opaque
// directional tile whose foam cycles a-h just like the sea; baking it froze the foam
// at one frame while the open ocean beside it kept moving.
static bool IsAnimatedTile( int type )
{
    return IsOpenWater( type ) || type == CHex::coastline;
}

// Build (or continue building) the whole-map far underlay for the CURRENT camera dir.
// burst=true sweeps every remaining row this call (load-time prewarm); else only
// kMapRowsSlice rows per frame. Safe to call every frame — returns immediately once the
// dir's texture is complete for this load generation.
static void BuildMapUnderlay( SDL_Renderer* r, const CAnimAtr& aa, unsigned loadGen, unsigned fogGen, bool burst )
{
    const int dir = aa.m_iDir & 3;
    const int eX = theMap.Get_eX( ), eY = theMap.Get_eY( );
    if ( !r || !eX || !eY ) return;
    // Freshness key = (loadGen, fogGen). loadGen covers texture (re)creation; fogGen
    // covers the per-hex GetVisibility() dim baked into the vertex colour below. The
    // underlay bakes fog (unlike the live s_fogRT overlay, which self-heals on the fog
    // dirty-gen), so a reveal — e.g. rocket-land disgorging units → CHex::IncVisible →
    // ++g_enFogVisGen — must force a rebuild here too, else the zoom-out backdrop shows
    // stale fog over the newly-revealed base until the next load/rotate (bug #9).
    if ( s_mapLoadGen[dir] == loadGen && s_mapFogGen[dir] == fogGen && s_mapRow[dir] >= eY ) return;   // complete

    Perf::ScopeCounter _cm( "map.build" );

    // LOAD-gen (re)start ONLY: size the texture from the map's content bbox + realloc +
    // transparent-clear. The iso projection is affine in hex coords, so the 4 corner hexes
    // bound the parallelogram; a one-tile pad absorbs altitude offsets and tile overhang.
    // (A FOG-only change does NOT come through here — see the else-branch below: it re-meshes
    // in place so a fast zoom-out mid-rebuild shows last-good terrain, not a cleared/black one.)
    if ( s_mapLoadGen[dir] != loadGen )
    {
        long mnx = LONG_MAX, mny = LONG_MAX, mxx = LONG_MIN, mxy = LONG_MIN;
        const int cxs[4] = { 0, eX - 1, 0, eX - 1 }, cys[4] = { 0, 0, eY - 1, eY - 1 };
        for ( int i = 0; i < 4; ++i )
        {
            CPoint c[4];
            aa.MapToContentHex( CHexCoord( cxs[i], cys[i] ), c );
            for ( int k = 0; k < 4; ++k )
            { mnx = __min( mnx, (long)c[k].x ); mxx = __max( mxx, (long)c[k].x );
              mny = __min( mny, (long)c[k].y ); mxy = __max( mxy, (long)c[k].y ); }
        }
        const long pad = 512;   // content px: max altitude lift + one 128px tile overhang
        mnx -= pad; mny -= pad; mxx += pad; mxy += pad;
        int   w3 = (int)( ( mxx - mnx ) >> 3 ), h3 = (int)( ( mxy - mny ) >> 3 );  // z3 px
        float sc = 1.0f;                       // fit within 4096 (33MB ARGB worst case)
        if ( w3 * sc > 4096.0f ) sc = 4096.0f / w3;
        if ( h3 * sc > 4096.0f ) sc = 4096.0f / h3;
        int tw = __max( 1, (int)( w3 * sc ) ), th = __max( 1, (int)( h3 * sc ) );
        if ( s_mapRT[dir] ) { SDL_DestroyTexture( s_mapRT[dir] ); s_mapRT[dir] = nullptr; }
        s_mapRT[dir] = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, tw, th );
        if ( !s_mapRT[dir] ) return;           // alloc failed — retry next frame
        SDL_SetTextureBlendMode( s_mapRT[dir], SDL_BLENDMODE_BLEND );
        SDL_SetTextureScaleMode( s_mapRT[dir], SDL_ScaleModeLinear );
        s_mapOrg[dir]   = CPoint( (int)mnx, (int)mny );
        s_mapScale[dir] = sc; s_mapTexW[dir] = tw; s_mapTexH[dir] = th;
        s_mapRow[dir] = 0; s_mapLoadGen[dir] = loadGen; s_mapFogGen[dir] = fogGen;
        // TORUS WRAP periods: the map repeats; stepping all of hex-X (or hex-Y) shifts the
        // content image by a constant vector (the projection is affine and CHexCoord is NOT
        // wrapped here — GetWorldHex extends linearly). The preview tiles the underlay at
        // these offsets so a viewport hanging past the map seam still has cover (the
        // canonical image alone left a BLACK band — user: "it was there, at the top").
        {
            CPoint o0[4], oX[4], oY[4];
            aa.MapToContentHex( CHexCoord( 0, 0 ), o0 );
            aa.MapToContentHex( CHexCoord( eX, 0 ), oX );
            aa.MapToContentHex( CHexCoord( 0, eY ), oY );
            s_mapPerX[dir] = CPoint( oX[0].x - o0[0].x, oX[0].y - o0[0].y );
            s_mapPerY[dir] = CPoint( oY[0].x - o0[0].x, oY[0].y - o0[0].y );
        }

        SDL_Texture* pt = SDL_GetRenderTarget( r ); SDL_Rect vp; SDL_RenderGetViewport( r, &vp );
        SDL_SetRenderTarget( r, s_mapRT[dir] );
        SDL_RenderSetViewport( r, nullptr );
        SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_NONE );
        // TRANSPARENT clear, NOT opaque black: the map is a DIAMOND inside this rect, and
        // the 3x3 wrap copies draw over each other — an opaque corner from one copy was
        // painting black OVER the neighbouring copy's terrain, which is exactly the black
        // the underlay exists to remove (user: "still happening when you zoom out rapidly").
        SDL_SetRenderDrawColor( r, 0, 0, 0, 0 );
        SDL_RenderClear( r );
        SDL_SetRenderTarget( r, pt ); SDL_RenderSetViewport( r, &vp );
    }
    else if ( s_mapFogGen[dir] != fogGen )
    {
        // FOG-only change (same map + geometry, a visibility reveal bumped g_enFogVisGen):
        // re-mesh the EXISTING underlay IN PLACE — do NOT realloc or transparent-clear it.
        // The row sweep below repaints every hex with the new fog dim over the last-good
        // bake (geometry unchanged → the in-place repaint is exact), so a FAST zoom-out
        // caught mid-rebuild shows the previous terrain (briefly with mixed old/new fog)
        // instead of an empty/BLACK underlay (operator Note 1/2 → BUGS #9 regression). Keeps
        // #9's stale-fog fix (the dim still re-bakes); only the clear-induced black flash is
        // removed. realloc + clear stay gated on a loadGen change above.
        s_mapRow[dir] = 0; s_mapFogGen[dir] = fogGen;
    }

    const int kMapRowsSlice = 16;              // ~8k hexes/frame on a 512-wide map
    const int rowStart = s_mapRow[dir];
    int rowEnd = burst ? eY : __min( eY, rowStart + kMapRowsSlice );
    if ( rowStart >= rowEnd ) return;

    SDL_Texture* pt = SDL_GetRenderTarget( r ); SDL_Rect vp; SDL_RenderGetViewport( r, &vp );
    SDL_SetRenderTarget( r, s_mapRT[dir] );
    SDL_RenderSetViewport( r, nullptr );
    SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );

    const float sc   = s_mapScale[dir];
    const float orgX = (float)s_mapOrg[dir].x, orgY = (float)s_mapOrg[dir].y;
    const Uint8 gFog = (Uint8)( 255.0f * kFogDim );
    static const SDL_FPoint uvD[4] = { {0.f,0.5f}, {0.5f,0.f}, {1.f,0.5f}, {0.5f,1.f} };
    // Row-major sweep = back-to-front painter's order (tall tiles occlude correctly).
    std::vector<std::pair<SDL_Texture*, std::vector<SDL_Vertex>>> buckets;
    for ( int y = rowStart; y < rowEnd; ++y )
    {
        for ( auto& b : buckets ) b.second.clear( );
        SDL_Texture* lastTex = nullptr; std::vector<SDL_Vertex>* lastVb = nullptr;
        for ( int x = 0; x < eX; ++x )
        {
            CHexCoord hc( x, y );
            CHex* ph = theMap.GetHex( hc );
            if ( !ph ) continue;
            const SDL2Terrain::Tile* tile = MemoTile( ph, dir );
            SDL_Texture* tex = TileTexAnyZoom( tile, 3 );
            if ( !tex ) continue;
            CPoint c[4];
            aa.MapToContentHex( hc, c );
            Uint8 g = ph->GetVisibility( ) ? 255 : gFog;   // bake fog-of-war dim
            SDL_Color col = { g, g, g, 255 };
            if ( tex != lastTex )
            {
                lastVb = nullptr;
                for ( auto& b : buckets ) if ( b.first == tex ) { lastVb = &b.second; break; }
                if ( !lastVb ) { buckets.emplace_back( tex, std::vector<SDL_Vertex>( ) ); lastVb = &buckets.back( ).second; }
                lastTex = tex;
            }
            for ( int k = 0; k < 4; ++k )
            {
                SDL_Vertex v;
                v.position  = { ( (float)c[k].x - orgX ) * 0.125f * sc,
                                ( (float)c[k].y - orgY ) * 0.125f * sc };
                v.tex_coord = uvD[k];
                v.color     = col;
                lastVb->push_back( v );
            }
        }
        for ( auto& b : buckets )
            if ( !b.second.empty( ) )
                SDL_RenderGeometry( r, b.first, b.second.data( ), (int)b.second.size( ),
                                    QuadIndices( (int)b.second.size( ) / 4 ), (int)b.second.size( ) / 4 * 6 );
    }
    s_mapRow[dir] = rowEnd;
    Perf::CounterAdd( "map.rows", rowEnd - rowStart );

    SDL_SetRenderTarget( r, pt ); SDL_RenderSetViewport( r, &vp );
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

    // Emit the diamond clipped to the horizontal slab [y0,y1] (fan-triangulated).
    auto emitSlab = [&]( float y0, float y1 ) {
        if ( y1 <= y0 ) return;
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
    };

    bool any = false;
    for ( int yb = ( yTop / band ) * band; yb < yBot; yb += band )
    {
        if ( ( ( yb + phase ) & band ) != 0 )   // "off" band → skip (matches (y+iFrame)&0x04)
            continue;
        float y0 = (float)__max( yb, yTop );
        float y1 = (float)__min( yb + band, yBot );
        if ( y1 <= y0 ) continue;
        emitSlab( y0, y1 );
        any = true;
    }

    // VISIBILITY GUARANTEE: a hex shorter than the 4px stripe — or one whose single band
    // lands on the "off" parity (which shifts with altitude/pan/zoom) — would otherwise
    // emit NOTHING and the cursor would silently vanish on that hex (the "disappears over
    // water / on hills / when zoomed out" bug). If the parity walk produced no band, draw
    // one guaranteed slab so every cursor hex always shows. Big hexes are unaffected.
    if ( !any )
        emitSlab( (float)yTop, (float)__min( yTop + band, yBot ) );
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

    // Hatch animation phase — the original formula (sprite.cpp): advance one step per
    // game frame so the stripe parity flips every 4 frames. (An earlier attempt to slow
    // this down backfired: a slower phase lengthens the "off" parity window, so a small
    // or off-aligned hex stays INVISIBLE for longer instead of blinking imperceptibly.
    // AppendHatchBands now guarantees a visible band regardless of parity, so speed and
    // visibility are independent again.)
    const int               phase = (int)theGame.GetFrame( );
    std::vector<SDL_Vertex> verts;

    // --- TORUS-SEAM wrap correction --------------------------------------------------
    // The footprint can straddle the map's wrap seam (the SW-NE line where the world
    // repeats). Two failure modes, both fixed here:
    //   (1) projecting a hex's CANONICAL (wrapped) coord throws a seam-crossing hex a full
    //       world-width away -> off-screen -> the hatch is cut along the seam. FIX: project
    //       the UN-wrapped coord (hcProj below). GetWorldHex extends LINEARLY for out-of-
    //       range coords (see BuildMapUnderlay's CHexCoord(eX,0) wrap-period probe), so a
    //       seam-crossing footprint stays contiguous.
    //   (2) when the mouse is over a hex shown via the wrapped (torus-tiled) copy, the
    //       anchor's OWN canonical projection is a full map off-screen and the whole
    //       footprint vanishes. FIX: if the anchor projects well outside the window, shift
    //       the entire footprint by an integer number of world-wrap PERIODS onto the
    //       visible copy. Away from the seam the anchor is on-screen -> shift == 0 ->
    //       behaviour is byte-identical to before.
    const CSize wsz = aa.m_dibwnd.GetWinSize( );
    CPoint shift( 0, 0 );
    {
        CPoint pa[4]; aa.MapToWindowHex( hexUL, pa );
        bool anchorOff = pa[0].x < -wsz.cx || pa[0].x > 2 * wsz.cx ||
                         pa[0].y < -wsz.cy || pa[0].y > 2 * wsz.cy;
        if ( anchorOff )
        {
            CPoint p0[4], pX[4], pY[4];
            aa.MapToWindowHex( CHexCoord( 0, 0 ),                p0 );
            aa.MapToWindowHex( CHexCoord( theMap.Get_eX( ), 0 ), pX );  // one full X-wrap (extends linearly)
            aa.MapToWindowHex( CHexCoord( 0, theMap.Get_eY( ) ), pY );  // one full Y-wrap
            double ex = pX[0].x - p0[0].x, ey = pX[0].y - p0[0].y;
            double fx = pY[0].x - p0[0].x, fy = pY[0].y - p0[0].y;
            double det = ex * fy - ey * fx;
            if ( det > 1e-6 || det < -1e-6 )
            {
                double dx = wsz.cx * 0.5 - pa[0].x;
                double dy = wsz.cy * 0.5 - pa[0].y;
                double a  = ( dx * fy - dy * fx ) / det;     // periods to shift in X / Y
                double b  = ( ex * dy - ey * dx ) / det;
                long   ai = (long)( a >= 0 ? a + 0.5 : a - 0.5 );
                long   bi = (long)( b >= 0 ? b + 0.5 : b - 0.5 );
                shift.x = (int)( ai * ex + bi * fx );
                shift.y = (int)( ai * ey + bi * fy );
            }
        }
    }

    // `hcProj` is the PROJECTION coord (NOT torus-wrapped, per the note above); `phex`
    // (the wrapped hex) supplies the terrain data / cursor mode.
    auto addHex = [&]( CHex* phex, CHexCoord hcProj ) {
        if ( !phex ) return;
        int cm = phex->GetCursorMode( );
        if ( cm == CHex::no_cur ) return;
        SDL_Color col;
        switch ( cm )
        {
            case CHex::ok_cur:        col = { 255, 255, 255, 215 }; break;  // white = buildable
            case CHex::land_exit_cur: col = {  30,  30,  30, 215 }; break;  // dark = land exit
            // BUGS #24: the SDL2 port gave the SEA exit the land exit's dark, so a water
            // entrance drew like a black road and you couldn't tell the two exits apart.
            // 1996 distinguished them — sprite.cpp maps land_exit_cur to palette 0 (black)
            // and sea_exit_cur to 252 — hence the operator's "should be BLUE again". The
            // palette lives in the data file, not source, so 252's exact RGB isn't
            // recoverable here; this is a blue in the same saturated family as the other
            // hatch colours below (alpha 215 to match).
            case CHex::sea_exit_cur:  col = {  45, 120, 235, 215 }; break;  // blue = sea exit
            case CHex::warn_cur:      col = { 235, 220,  45, 215 }; break;  // yellow
            case CHex::bad_cur:       col = { 225,  45,  45, 225 }; break;  // red
            case CHex::lousy_cur:     col = { 235, 140,  45, 220 }; break;  // orange
            default:                  col = { 255, 255, 255, 215 }; break;
        }
        CPoint pts[4];
        // Project the UN-wrapped coord, then apply the seam shift. Corners are used
        // unconditionally — MapToWindowHex's RETURN is only a front/back-FACING test
        // (false on steep hexes); the 4 corners are valid and AppendHatchBands clips any order.
        aa.MapToWindowHex( hcProj, pts );
        for ( int k = 0; k < 4; ++k ) { pts[k].x += shift.x; pts[k].y += shift.y; }
        AppendHatchBands( pts, col, phase, verts );
    };

    for ( int j = 0; j < cy; ++j )
        for ( int i = 0; i < cx; ++i )
        {
            CHexCoord hcProj( hexUL.X( ) + i, hexUL.Y( ) + j );   // un-wrapped: seam-continuous projection
            CHexCoord hcData = hcProj; hcData.Wrap( );            // wrapped: terrain-data lookup
            addHex( theMap.GetHex( hcData ), hcProj );
        }
    // Exits (the dark entrance tile) must also project from a footprint-contiguous,
    // UN-wrapped coord, or they drop at the seam just like the footprint did. CHexCoord::Diff
    // gives the shortest signed delta across the wrap, so hexUL + delta is the exit's
    // un-wrapped position next to the building.
    auto exitProj = [&]( CHex* pExit ) -> CHexCoord {
        CHexCoord ec = pExit->GetHex( );
        return CHexCoord( hexUL.X( ) + CHexCoord::Diff( ec.X( ) - hexUL.X( ) ),
                          hexUL.Y( ) + CHexCoord::Diff( ec.Y( ) - hexUL.Y( ) ) );
    };
    if ( theMap.m_pLandExit ) addHex( theMap.m_pLandExit, exitProj( theMap.m_pLandExit ) );
    if ( theMap.m_pShipExit ) addHex( theMap.m_pShipExit, exitProj( theMap.m_pShipExit ) );

    if ( verts.empty( ) )
        return;
    SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
    SDL_RenderGeometry( r, nullptr, verts.data( ), (int)verts.size( ), nullptr, 0 );
}

//==========================================================================
// PER-RENDERER TERRAIN CONTEXT
//
// Each detached area-map window has its OWN SDL_Renderer (SDL2Panel), and GPU
// textures can't be shared across renderers. Originally all of SDL2Terrain's
// state — the tile asset set AND Render()'s ~40 cache statics — was a single
// global instance, so two area maps fought over one set: the second showed only
// untextured foundations and both thrashed the cache into a full rebuild every
// frame (the "VERY laggy" multi-area map). RCtx gives each renderer an
// independent copy.
//
// Two storage strategies, by who needs the state:
//  * FILE-STATE (assets, tile memo, whole-map underlay, far snapshot) is read by
//    file-scope helpers OUTSIDE Render (Load, TileForHex, MemoTile,
//    BuildMapUnderlay). Those keep using the existing globals, so we SWAP the
//    active renderer's copy into the globals around each Render/Load call
//    (swapFileState + TerrainCtxScope) and swap it back out on return.
//  * RENDER-STATE (the render-target textures + mesh/water/fog caches) is touched
//    only inside Render. Those former `static` locals are now ALIASED by
//    reference to RCtx members (see the top of Render) — no swap needed; the
//    alias just retargets each name at the current renderer's storage.
//==========================================================================

// Retained-mesh capture record types (were local to Render; hoisted so RCtx can
// hold std::vector<> of them).
struct BaseCell { const SDL2Terrain::Tile* tile; CPoint c[4]; bool transpose;
                  bool shade; float bL, bR;       // + slope-shade (per-triangle brightness)
                  int fStart, fCount; };          // [fStart,fStart+fCount) into featherBands
struct FeatherBand { const SDL2Terrain::Tile* tile; CPoint c[4]; SDL_FPoint uv[4]; };
struct WaterCell     { CHex* hex; CPoint c[4]; int animIdx; };
struct WaterBandCell { int animIdx; CPoint c[4]; SDL_FPoint uv[4]; };
struct FogCell { CHex* hex; CPoint c[4]; };

struct SDL2TerrainRCtx
{
    // ---- FILE-STATE (swapped with the globals around Render/Load) ----
    std::unordered_map<std::string, SDL2Terrain::Tile>        tiles;
    std::unordered_map<std::string, const SDL2Terrain::Tile*> byType;
    std::unordered_map<std::string, const SDL2Terrain::Tile*> byTypeVar;
    std::vector<const SDL2Terrain::Tile*> typeVarPtr[kNumTypeNames];
    SDL_Renderer* renderer = nullptr;
    bool          loaded   = false;
    unsigned      loadGen  = 0;
    std::vector<const SDL2Terrain::Tile*> tileCache;
    std::vector<uint32_t>                 tileGen;
    uint32_t      tileGenCur[4] = { 0, 0, 0, 0 };
    int           tileCacheEX = 0, tileCacheEY = 0;
    SDL_Texture*  farRT = nullptr;
    int           farZoom = -1, farDir = -1, farMargin = 0, farW = 0, farH = 0;
    unsigned      farLoadGen = ~0u;
    CHexCoord     farRefHex;
    CPoint        farRefPx{ 0, 0 };
    SDL_Texture*  mapRT[4]      = { nullptr, nullptr, nullptr, nullptr };
    unsigned      mapLoadGen[4] = { ~0u, ~0u, ~0u, ~0u };
    unsigned      mapFogGen[4]  = { ~0u, ~0u, ~0u, ~0u };   // was file-static only: 2nd map kept stale fog
    int           mapRow[4]     = { 0, 0, 0, 0 };
    CPoint        mapOrg[4];
    float         mapScale[4]   = { 0, 0, 0, 0 };
    int           mapTexW[4]    = { 0, 0, 0, 0 };
    int           mapTexH[4]    = { 0, 0, 0, 0 };
    CPoint        mapPerX[4];
    CPoint        mapPerY[4];

    // ---- RENDER-STATE (aliased by reference inside Render; not swapped) ----
    SDL_Texture *rt = nullptr, *fogRT = nullptr, *shadeRT = nullptr,
                *shadeHalf = nullptr, *rtB = nullptr, *shadeB = nullptr, *waterRT = nullptr;
    int rtW = 0, rtH = 0;
    std::vector<SDL_Vertex>     shadeVerts;
    std::vector<CHex*>          waterHex;
    std::vector<CPoint>         waterPos;
    std::vector<int>            waterAnimOf;
    std::vector<WaterAnim>      waterAnims;
    std::vector<WaterBlendBand> waterBlend;
    unsigned waterTick = ~0u;
    std::vector<std::vector<SDL_Vertex>> waterDiamGeom, waterBandGeom;
    unsigned waterRTtick = ~0u;
    uint64_t sig = ~0ull;
    unsigned builtEditGen = 0;
    std::vector<BaseCell>      baseCells;
    std::vector<FeatherBand>   featherBands;
    std::vector<WaterCell>     waterCells;
    std::vector<WaterBandCell> waterBandCells;
    std::vector<FogCell>       fogCells;
    int mirZoom = -1, mirDir = -1, capMaxZoom = -1;
    int capCMinX = 0, capCMaxX = -1, capCMinY = 0, capCMaxY = -1;
    unsigned mirEditGen = ~0u, mirLoadGen = ~0u;
    bool mirValid = false;
    CHexCoord refHex;
    CPoint    refPx{ 0, 0 };
    CPoint    builtUL{ 0, 0 };
    std::vector<SDL_Vertex> fogVerts;
    std::vector<CHex*>      fogHex;
    std::vector<int>        fogNbr;
    std::vector<float>      fogVis;
    DWORD fogUpdAt = 0;
    SDL_Renderer* renderRenderer = nullptr;
    int builtZoom = -1, builtDir = -1, builtMargin = 0;
    unsigned builtLoadGen = ~0u;
    DWORD viewChangedAt = 0;
    uint64_t lastViewSig = ~0ull;
    bool deferActive = false;
    int zoutBudget = -1;
    std::vector<unsigned> memoEditQ;
    bool memoEditOverflow = false;
    unsigned tileMemoLoadGen = 0xFFFFFFFFu;
    unsigned fogVisGenSeen = ~0u;
    unsigned targetsLostSeen = 0;   // s_targetsLostGen this ctx has recovered from
    int prevDstX = INT_MIN, prevDstY = INT_MIN;
    int lastDx = INT_MIN, lastDy = INT_MIN, lastCx = INT_MIN, lastCy = INT_MIN;

    void swapFileState();   // exchange the file-state members above with the globals
};

void SDL2TerrainRCtx::swapFileState()
{
    using std::swap;
    swap( tiles,       SDL2Terrain::s_tiles );
    swap( byType,      SDL2Terrain::s_byType );
    swap( byTypeVar,   SDL2Terrain::s_byTypeVar );
    for ( int i = 0; i < kNumTypeNames; ++i ) swap( typeVarPtr[i], s_typeVarPtr[i] );
    swap( renderer,    SDL2Terrain::s_renderer );
    swap( loaded,      SDL2Terrain::s_loaded );
    swap( loadGen,     s_loadGen );
    swap( tileCache,   s_tileCache );
    swap( tileGen,     s_tileGen );
    swap( tileGenCur,  s_tileGenCur );          // array overload
    swap( tileCacheEX, s_tileCacheEX );
    swap( tileCacheEY, s_tileCacheEY );
    swap( farRT,       s_farRT );
    swap( farZoom,     s_farZoom );
    swap( farDir,      s_farDir );
    swap( farMargin,   s_farMargin );
    swap( farW,        s_farW );
    swap( farH,        s_farH );
    swap( farLoadGen,  s_farLoadGen );
    swap( farRefHex,   s_farRefHex );
    swap( farRefPx,    s_farRefPx );
    swap( mapRT,       s_mapRT );
    swap( mapLoadGen,  s_mapLoadGen );
    swap( mapFogGen,   s_mapFogGen );
    swap( mapRow,      s_mapRow );
    swap( mapOrg,      s_mapOrg );
    swap( mapScale,    s_mapScale );
    swap( mapTexW,     s_mapTexW );
    swap( mapTexH,     s_mapTexH );
    swap( mapPerX,     s_mapPerX );
    swap( mapPerY,     s_mapPerY );
}

// Per-renderer registry + which context's file-state currently sits in the globals.
static std::map<SDL_Renderer*, SDL2TerrainRCtx> s_rctx;
static SDL2TerrainRCtx* s_activeFileCtx = nullptr;

// RAII: swap a context's file-state into the globals for the duration of a call,
// and restore on exit. Nesting the SAME context (Render → Load) is a no-op so the
// file-state isn't double-swapped.
struct TerrainCtxScope
{
    SDL2TerrainRCtx* prev; SDL2TerrainRCtx* mine; bool did;
    explicit TerrainCtxScope( SDL2TerrainRCtx& c )
    {
        mine = &c;
        if ( s_activeFileCtx == mine ) { did = false; prev = mine; return; }
        prev = s_activeFileCtx;
        if ( prev ) prev->swapFileState();   // current context out
        mine->swapFileState();               // this context in
        s_activeFileCtx = mine; did = true;
    }
    ~TerrainCtxScope()
    {
        if ( !did ) return;
        mine->swapFileState();               // this context out
        if ( prev ) prev->swapFileState();   // previous context back in
        s_activeFileCtx = prev;
    }
};

void SDL2Terrain::ReleaseRenderer( SDL_Renderer* r )
{
    auto it = s_rctx.find( r );
    if ( it == s_rctx.end() ) return;
    SDL2TerrainRCtx& C = it->second;
    // If this context is currently swapped into the globals, pull its data back
    // into the members first so the frees below see it (defensive — teardown
    // normally runs with no Render on the stack).
    if ( s_activeFileCtx == &C ) { C.swapFileState(); s_activeFileCtx = nullptr; }
    // Render-target textures (always live in the members — never swapped).
    SDL_Texture** rts[] = { &C.rt, &C.fogRT, &C.shadeRT, &C.shadeHalf,
                            &C.rtB, &C.shadeB, &C.waterRT, &C.farRT };
    for ( SDL_Texture** pp : rts ) if ( *pp ) { SDL_DestroyTexture( *pp ); *pp = nullptr; }
    for ( int d = 0; d < 4; ++d ) if ( C.mapRT[d] ) { SDL_DestroyTexture( C.mapRT[d] ); C.mapRT[d] = nullptr; }
    // Tile asset textures bound to this renderer.
    for ( auto& kv : C.tiles )
        for ( SDL_Texture* t : kv.second.tex )
            if ( t ) SDL_DestroyTexture( t );
    s_rctx.erase( it );
#if EN_GAMEPLAY_PROBES
    char m[96]; sprintf( m, "[REN] terrain context released for renderer %p (now %d live)",
                         (void*)r, (int)s_rctx.size() );
    LogTerrain( m );
#endif
}

int SDL2Terrain::Load( SDL_Renderer* renderer )
{
    if ( !renderer )
        return 0;
    // Per-renderer assets: swap THIS renderer's context into the globals for the
    // load. Each renderer gets its own tile-texture set (textures can't cross
    // renderers); a second area-map window no longer Unload()s the first one's tiles.
    SDL2TerrainRCtx& C = s_rctx[renderer];
    TerrainCtxScope _ctxScope( C );
    if ( s_loaded && s_renderer == renderer )
        return (int)s_tiles.size();
    if ( s_loaded )
        Unload();  // (per-ctx this can't happen — each context owns one renderer)

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
            // stb_image's stbi_load() calls fopen() DIRECTLY (not through the Win32
            // file shim that translates '\'→'/'), so a backslash join is a literal
            // filename char on Linux and every tile load fails → 0 tiles → black GPU
            // terrain. Use '/' off-Windows (Windows accepts either separator).
#ifdef _WIN32
            SDL_Texture* tex = LoadPng( renderer, dir + "\\" + name, tw, th );
#else
            SDL_Texture* tex = LoadPng( renderer, dir + "/" + name, tw, th );
#endif
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
    s_anyLoaded = true;   // renderer-agnostic gate (IsLoaded)
    ++s_loadGen;   // invalidate the terrain mesh cache (textures were (re)created)
    LogTerrain( "Loaded " + std::to_string( s_tiles.size() ) + " tiles (" +
                std::to_string( files ) + " PNGs)." );
    return (int)s_tiles.size();
}

void SDL2Terrain::Render( SDL_Renderer* r, const CAnimAtr& aa )
{
    // Per-renderer context: file-state swapped into the globals for this call;
    // render-state reached via the reference aliases declared below (s_rt → C.rt …).
    if ( !r ) return;
    SDL2TerrainRCtx& C = s_rctx[r];
    TerrainCtxScope _ctxScope( C );
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
    SDL_Texture*& s_rt        = C.rt;        // terrain tiles + feather (full bright, NO shade/fog)
    SDL_Texture*& s_fogRT      = C.fogRT;     // fog-of-war dim, decoupled overlay
    SDL_Texture*& s_shadeRT    = C.shadeRT;   // slope shading, BLURRED so tile edges feather
    SDL_Texture*& s_shadeHalf  = C.shadeHalf; // half-res scratch for the blur round-trip
    // Incremental-pan (EN_PANSTRIP) ping-pong scratch: the two texture-ACCUMULATED layers
    // (terrain + slope-shade) are scrolled by copying old→scratch at the pan offset, then
    // swapped, so a pan keeps the old content and only the edge strip is re-meshed. (Fog +
    // water re-render from their vertex lists, so they need no ping-pong.)
    SDL_Texture*& s_rtB        = C.rtB;
    SDL_Texture*& s_shadeB     = C.shadeB;
    int& s_rtW = C.rtW; int& s_rtH = C.rtH;
    // Slope-shade overlay geometry (grayscale brightness per tile), built with the
    // terrain mesh; rendered to s_shadeRT then blurred so the per-tile flat shading
    // blends across boundaries (the original feathered this; a sharp step looks bad).
    std::vector<SDL_Vertex>& s_shadeVerts = C.shadeVerts;
    // Animated open-water: captured during the build so the wave can be re-drawn IN
    // PLACE into s_rt on each wave-tick (cheap) instead of rebuilding the whole mesh —
    // that's what lets water animate at ALL zooms (it was frozen zoomed out because
    // the terrain texture only rebuilds on a view change).
    std::vector<CHex*>&  s_waterHex    = C.waterHex;    // open-water hexes (stable CHex*)
    std::vector<CPoint>& s_waterPos    = C.waterPos;    // 4 corner positions/hex (texture space)
    std::vector<int>&    s_waterAnimOf = C.waterAnimOf; // per water hex: index into s_waterAnims
    std::vector<WaterAnim>& s_waterAnims = C.waterAnims; // distinct (type,variant) frame tables
    std::vector<WaterBlendBand>& s_waterBlend = C.waterBlend; // water<->water blend bands (texture space)
    unsigned& s_waterTick = C.waterTick;
    // Water cache (perf): the live water layer used to re-submit ~30k hex diamonds EVERY
    // frame (~85ms in Debug → the zoomed-out FPS wall). Instead, pre-group the diamond/band
    // vertices by animation index ONCE per rebuild (positions are static in texture space —
    // only the frame texture changes), bake them into s_waterRT only when the wave frame
    // advances (~4Hz), and blit that texture (1 GPU copy) under the terrain each frame.
    std::vector<std::vector<SDL_Vertex>>& s_waterDiamGeom = C.waterDiamGeom;  // per animIdx: diamond verts (tex space)
    std::vector<std::vector<SDL_Vertex>>& s_waterBandGeom = C.waterBandGeom;  // per animIdx: blend-band verts (tex space)
    SDL_Texture*& s_waterRT     = C.waterRT;    // baked animated water, blitted under s_rt
    unsigned&     s_waterRTtick = C.waterRTtick; // wave-frame index baked into s_waterRT
    uint64_t& s_sig          = C.sig;
    unsigned& s_builtEditGen = C.builtEditGen;  // g_enTerrainEditGen baked into the current mesh

    // RETAINED MESH (base layer, EN_RETAIN): each visible LAND hex's tile + CONTENT-space
    // corners, captured on a full rebuild. A later zoom-only change re-emits the base from
    // these (screen = (content >> newZoom) - newUL, tex = tile->tex[newZoom]) instead of the
    // per-hex loop. s_mir* = the (zoom,dir,editGen,loadGen) the cells were captured at, to
    // detect a pure zoom change (everything else identical → cells still valid). (BaseCell /
    // FeatherBand / WaterCell / WaterBandCell / FogCell are now file-scope, above RCtx.)
    std::vector<BaseCell>& s_baseCells = C.baseCells;
    std::vector<FeatherBand>& s_featherBands = C.featherBands;
    std::vector<WaterCell>&     s_waterCells     = C.waterCells;
    std::vector<WaterBandCell>& s_waterBandCells = C.waterBandCells;
    std::vector<FogCell>& s_fogCells = C.fogCells;
    int& s_mirZoom = C.mirZoom; int& s_mirDir = C.mirDir;
    int& s_capMaxZoom = C.capMaxZoom;   // most-zoomed-OUT level the captured region covers (capZoom + zout budget)
    // CONTENT-space bbox of all captured cells (= the capture viewport + margin). A zoom replay
    // is only valid when the current (possibly panned) viewport's content lies inside this box;
    // otherwise the uncovered part re-emits as nothing (BLACK) yet caches as valid. Set on capture.
    int& s_capCMinX = C.capCMinX; int& s_capCMaxX = C.capCMaxX; int& s_capCMinY = C.capCMinY; int& s_capCMaxY = C.capCMaxY;
    unsigned& s_mirEditGen = C.mirEditGen; unsigned& s_mirLoadGen = C.mirLoadGen;
    bool& s_mirValid = C.mirValid;
    CHexCoord& s_refHex = C.refHex;       // a fixed hex captured at build time
    CPoint&    s_refPx  = C.refPx;        // its window-screen pos at build time
    CPoint&    s_builtUL = C.builtUL;     // ulSnap the cached mesh was built against (edit
                                          // patches project content->texture with THIS, not
                                          // the live UL — pan-race coherence)
    // Pan-buffer margin: the cached texture extends this far beyond the viewport so the
    // view can pan within it before a re-mesh. Shrink it when zoomed way out — there the
    // tiles are tiny (z3 = 16x8 px), so a fixed 256px margin is DOZENS of off-screen hexes
    // of pure rebuild cost (the margin band inflated the hex count ~70% at max zoom), AND
    // the whole map nearly fits on screen so there's little room to pan into the margin
    // anyway. Keep the full buffer zoomed in, where panning ranges far and hex counts are
    // low. Cuts ~25% of the hexes (hence ~25% off the rebuild) at z3 with negligible pan
    // loss. zoom is clamped to [0,NUM_ZOOMS) at the top of Render.
    // Trimmed further at z2/z3 (2026-06-07): incremental-pan (incPan) re-meshes only the
    // edge STRIP on a pan, so a large pan buffer no longer saves rebuilds — it just inflates
    // the off-screen overscan hex count on every zoom/rotate FULL rebuild (the 15 s wall).
    static const int kMarginByZoom[NUM_ZOOMS] = { 256, 160, 80, 32 };
    const int kMarginPx = kMarginByZoom[zoom];

    // Fog overlay geometry, built FREE during the terrain pass (positions only), then
    // re-coloured on a fast throttle WITHOUT rebuilding terrain — so fog tracks unit
    // vision in ~150 ms while the heavy terrain mesh rebuilds only on a view change.
    std::vector<SDL_Vertex>& s_fogVerts = C.fogVerts; // 4 per hex (indexed quad, L/T/R/B), black, per-corner alpha
    std::vector<CHex*>&      s_fogHex   = C.fogHex;   // hex per fog quad (self visibility, no GetHex)
    std::vector<int>&        s_fogNbr   = C.fogNbr;   // 8 neighbour fog-indices per hex (-1 = none)
    std::vector<float>&      s_fogVis   = C.fogVis;   // per-hex visibility, re-sampled each throttle
    DWORD& s_fogUpdAt = C.fogUpdAt;
    const DWORD kFogThrottle = 150;

    // Terrain mesh is keyed on zoom/dir/wave/loadGen — NOT scroll, and NO time
    // refresh: it rebuilds only when the view changes or a pan leaves the margin, so
    // sitting still / panning within margin never hitches. (Fog moved to the overlay;
    // terrain edits like roads show on the next view change.)
    // ONE gen read per frame (genTop): sig, the noEdit projections, and the rebuild's
    // cursor advance below must all agree on the same value. The old live re-reads let
    // an edit landing mid-rebuild bake a gen residue into s_sig, so baseSame never
    // matched again and every subsequent edit forced a FULL rebuild instead of a patch.
    const unsigned genTop = g_enTerrainEditGen;
    uint64_t sig = (uint64_t)( zoom & 7 )
                 ^ ( (uint64_t)( aa.m_iDir & 3 ) << 3 )
                 ^ ( (uint64_t)s_loadGen << 5 )
                 ^ ( (uint64_t)genTop    << 40 );  // runtime terrain edits
    // Water animation no longer rebuilds the terrain texture (it's re-drawn in place
    // on the wave-tick below), so the rebuild key omits the wave at every zoom.
    DWORD    _nowT = GetTickCount( );
    unsigned curWaterTick = (unsigned)( theGame.GetFrame( ) / 6 );   // kWaterHold = 6

    const int rtW = ws.cx + 2 * kMarginPx;
    const int rtH = ws.cy + margin + 2 * kMarginPx;

    // RENDERER-CHANGE REBIND (root-cause probe for "black terrain after a map
    // transition"): the per-frame render-target textures below are STATIC and were
    // created against a specific SDL_Renderer. A map transition that recreates the area
    // window's renderer (in-game Load, New Game) leaves them bound to a DESTROYED
    // renderer → SDL draws nothing → black terrain. They were only ever recreated on a
    // SIZE change, never a renderer change. Detect the change, drop all renderer-bound
    // statics so the size-block below rebuilds them on the new renderer, force a full
    // mesh rebuild, and resync the tile textures (Load() Unload+reloads on r change).
    SDL_Renderer*& s_renderRenderer = C.renderRenderer;
    if ( r != s_renderRenderer )
    {
        if ( s_renderRenderer != nullptr )   // not the first-ever bind — log the swap
        {
#if EN_GAMEPLAY_PROBES
            char m[128];
            sprintf( m, "[REN] terrain renderer changed %p -> %p (rebinding RTs)",
                     (void*)s_renderRenderer, (void*)r );
            LogTerrain( m );
#endif
            // NULL (do NOT SDL_DestroyTexture) the renderer-bound statics: the old
            // renderer may already have been destroyed by the panel teardown, which
            // frees all its textures — destroying them again here is a use-after-free.
            // The size-block below recreates them fresh on the new renderer.
            s_rt = s_rtB = s_fogRT = s_shadeRT = s_shadeHalf = s_shadeB = s_waterRT = nullptr;
            s_farRT = nullptr; s_farZoom = -1;   // far snapshot was bound to the dead renderer too
            for ( int d = 0; d < 4; ++d ) { s_mapRT[d] = nullptr; s_mapLoadGen[d] = ~0u; s_mapRow[d] = 0; }   // whole-map underlay too
            s_rtW = s_rtH = 0;        // also forces the size-block recreate path
            s_sig = ~0ull;            // force a full mesh rebuild on the new renderer
            s_waterRTtick = ~0u; s_waterTick = ~0u;
            SDL2Terrain::Load( r );   // ensure tile textures are bound to the new renderer
        }
        s_renderRenderer = r;
    }

    // --- GESTURE DEFER (EN_PREVIEW): when the view KEY changed (zoom/rotate — not edits, not
    // load), don't rebuild this frame. Present the existing composite scaled to the new view
    // (below) and wait for the gesture to SETTLE (kSettleMs since the last view change), so a
    // rapid 4-notch zoom does ONE rebuild at the destination instead of four along the way.
    // The texture-recreate block below is gated off during the defer — the old-size textures
    // ARE the preview source; the settle frame recreates + rebuilds as usual.
    int& s_builtZoom = C.builtZoom; int& s_builtDir = C.builtDir; int& s_builtMargin = C.builtMargin;  // view the textures were built at
    unsigned& s_builtLoadGen = C.builtLoadGen;
    DWORD&    s_viewChangedAt = C.viewChangedAt;     // last time the view key changed (gesture clock)
    uint64_t& s_lastViewSig = C.lastViewSig;
    bool&     s_deferActive = C.deferActive;
    const  DWORD    kSettleMs = 120;
    {
        uint64_t curNoEdit = sig   ^ ( (uint64_t)genTop          << 40 );
        uint64_t bltNoEdit = s_sig ^ ( (uint64_t)s_builtEditGen  << 40 );
        bool viewKeyChanged = ( s_sig != ~0ull ) && ( curNoEdit != bltNoEdit );
        if ( viewKeyChanged && s_lastViewSig != curNoEdit ) { s_viewChangedAt = _nowT; s_lastViewSig = curNoEdit; }
        // Pure ROTATE (dir change) is NOT deferred: the preview can't rotate the old texture
        // (not affine in this projection), so holding it left old-orientation terrain under
        // already-rotated sprites — the "terrain lags the buildings" artifact. Rotate taps are
        // slower than the settle window anyway (no coalescing value), and the per-direction
        // tile memo makes the synchronous rebuild cheap. Zoom-only changes keep the preview.
        s_deferActive = TerrainPreviewEnabled( ) && viewKeyChanged && s_rt != nullptr
                        && s_builtZoom >= 0 && s_builtLoadGen == s_loadGen
                        && s_builtDir == ( aa.m_iDir & 3 )
                        && ( _nowT - s_viewChangedAt ) < kSettleMs;
        if ( s_deferActive ) Perf::CounterAdd( "pv.defer", 1 );
    }

    // GPU device-lost (screen power-off/on, driver reset): the cached target textures
    // are valid objects with DEAD contents. Invalidate every cached layer so this frame
    // rebuilds them all — terrain mesh (s_sig), water bake, far snapshot, whole-map
    // underlay. Without this the composite blits black while sprites (re-emitted every
    // frame) keep working — the user-reported "terrain black after monitor off/on".
    if ( C.targetsLostSeen != s_targetsLostGen )
    {
        C.targetsLostSeen = s_targetsLostGen;
        s_sig         = ~0ull;   // full mesh re-render into s_rt/shade/fog
        s_waterRTtick = ~0u; s_waterTick = ~0u;          // water re-bake
        if ( s_farRT ) { SDL_DestroyTexture( s_farRT ); s_farRT = nullptr; }
        s_farZoom = -1;
        for ( int d = 0; d < 4; ++d ) { s_mapLoadGen[d] = ~0u; s_mapRow[d] = 0; }
#if EN_GAMEPLAY_PROBES
        LogTerrain( "[REN] render targets reset — rebuilding cached layers" );
#endif
    }

    // (Re)create the textures when the viewport size (hence rtW/rtH) changes.
    if ( !s_deferActive && s_rt && ( s_rtW != rtW || s_rtH != rtH ) )
    {
        SDL_DestroyTexture( s_rt ); s_rt = nullptr;
        if ( s_fogRT )    { SDL_DestroyTexture( s_fogRT );    s_fogRT = nullptr; }
        if ( s_shadeRT )  { SDL_DestroyTexture( s_shadeRT );  s_shadeRT = nullptr; }
        if ( s_shadeHalf ){ SDL_DestroyTexture( s_shadeHalf );s_shadeHalf = nullptr; }
        if ( s_waterRT )  { SDL_DestroyTexture( s_waterRT );  s_waterRT = nullptr; }
        if ( s_rtB )      { SDL_DestroyTexture( s_rtB );      s_rtB = nullptr; }
        if ( s_shadeB )   { SDL_DestroyTexture( s_shadeB );   s_shadeB = nullptr; }
    }
    if ( !s_rt )
    {
        s_rt      = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW, rtH );
        s_fogRT   = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW, rtH );
        s_waterRT = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW, rtH );
        if ( s_waterRT ) { SDL_SetTextureBlendMode( s_waterRT, SDL_BLENDMODE_BLEND ); s_waterRTtick = ~0u; }
        s_shadeRT = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW, rtH );
        s_shadeHalf = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW / 2, rtH / 2 );
        if ( s_rt )    { SDL_SetTextureBlendMode( s_rt, SDL_BLENDMODE_BLEND ); s_rtW = rtW; s_rtH = rtH;
                         SDL_SetTextureScaleMode( s_rt, SDL_ScaleModeLinear ); }       // smooth gesture-preview scaling (no-op at 1:1)
        if ( s_fogRT ) { SDL_SetTextureBlendMode( s_fogRT, SDL_BLENDMODE_BLEND );
                         SDL_SetTextureScaleMode( s_fogRT, SDL_ScaleModeLinear ); }
        if ( s_waterRT ) SDL_SetTextureScaleMode( s_waterRT, SDL_ScaleModeLinear );
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
    uint64_t sigNoEdit  = sig   ^ ( (uint64_t)genTop          << 40 );
    uint64_t lastNoEdit = s_sig ^ ( (uint64_t)s_builtEditGen  << 40 );
    // UL SNAPSHOT (pan-race fix, user's "line cut the screen" / black seams on fast pan):
    // the view origin (aa.m_ptUL) can move WHILE this function runs (input/sim pans the
    // map). Every texture-space computation this frame — the pan-delta measure, the mesh
    // emit, the strip shift, the edit patches — must use ONE consistent origin, or the
    // strip is meshed against a different origin than the shift applied to the old
    // content and the seam bakes into s_rt as a permanent transparent (=black) band.
    const CPoint ulSnap = aa.m_ptUL;
    int dX = 0, dY = 0; bool covered = false, ulMoved = false;
    if ( sigNoEdit == lastNoEdit )
    {
        CPoint rp[4];
        if ( aa.MapToWindowHex( s_refHex, rp ) )
        {
            dX = rp[0].x - s_refPx.x;
            dY = rp[0].y - s_refPx.y;
            covered = ( abs( dX ) <= kMarginPx && abs( dY ) <= kMarginPx );
            // COHERENCE GUARD: MapToWindowHex read the LIVE UL; re-derive the same
            // projection from ulSnap (window = (content >> zoom) - UL, integer-exact).
            // Any disagreement = the UL moved between the two reads (or a torus-wrap
            // image mismatch) → dX is NOT trustworthy for blitting or for shifting
            // cached content. Force the FULL rebuild (ulMoved also vetoes incPan —
            // !covered alone would route the bogus dX into the strip path's shift).
            CPoint rcc[4]; aa.MapToContentHex( s_refHex, rcc );
            if ( ( rcc[0].x >> zoom ) - ulSnap.x != rp[0].x ||
                 ( rcc[0].y >> zoom ) - ulSnap.y != rp[0].y )
            {
                covered = false; ulMoved = true;
                Perf::CounterAdd( "rb.ulrace", 1 );   // how often the race actually fires
            }
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
    // Snapshot under the lock. The log is MULTI-READER: this ctx's pending edits are the
    // slice from ITS cursor (s_builtEditGen); "complete" = the cursor is not below the
    // trimmed base AND the log is contiguous through the current gen (it stops recording
    // at kEditLogCap). The mesh-edit replay list is valid only for the ctx that captured
    // it (anchor) — any other ctx must treat it as overflowed.
    unsigned genSnap; size_t editCount; bool meshOverflow, editsComplete;
    { std::lock_guard<std::mutex> lk( g_enEditMutex );
      genSnap       = g_enTerrainEditGen;
      editCount     = (size_t)( genSnap - s_builtEditGen );
      editsComplete = ( s_builtEditGen >= g_enEditBaseGen ) &&
                      ( g_enEditBaseGen + g_enEditedHexes.size( ) == genSnap );
      meshOverflow  = g_enMeshEditOverflow || g_enMeshEditAnchor != &C; }
    bool editsPend = ( genSnap != s_builtEditGen );
    bool editsOK   = editsPend && editsComplete && editCount <= kEditPatchCap;
    const bool doPatch     = TerrainPatchEnabled( ) && baseSame && editsOK;
    // The gesture defer vetoes the rebuild: this frame presents the scaled preview instead.
    // The settle frame re-evaluates with s_deferActive false and rebuilds normally.
    const bool needRebuild = ( !baseSame || ( editsPend && !doPatch ) ) && !s_deferActive;
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

    // Incremental pan (strip rebuild): the rebuild was forced ONLY by a pan past the margin
    // (same zoom/dir/loadgen, no pending edits) — so instead of re-meshing the whole view,
    // SCROLL the cached textures by the pan delta and mesh just the newly-revealed edge.
    // Requires the pan to still overlap the texture (else it's a teleport → full rebuild).
    // NOTE: editsPend is NOT required false — the AI edits terrain (roads) almost every
    // frame, so requiring no-pending-edits would block incPan during all normal play. The
    // edited hexes are absorbed by the full rebuild's gen-advance below; in the covered
    // region they're briefly stale until the next full rebuild (acceptable; refine later).
    const bool panOnly = ( sigNoEdit == lastNoEdit ) && !covered && s_rt;
    const bool incPan  = TerrainPanStripEnabled( ) && panOnly && !ulMoved &&
                         abs( dX ) < rtW - 8 && abs( dY ) < rtH - 8;
    // Texture-space scroll = the pan delta (captured before dX/dY are zeroed at re-center):
    // a world hex drawn at window (Tx + dX - kMarginPx) must, after re-centering to dX'=0,
    // draw at the same window pos with new texture-local Tx' = Tx + dX. So content shifts +dX.
    const int shiftX = dX, shiftY = dY;
    if ( incPan ) Perf::CounterAdd( "rebuild.incpan", 1 );

    // Build the mesh in TEXTURE space: positions are offset by +kMarginPx so the
    // margin band (hexes left/above the viewport) lands at texture x/y >= 0.
    const int offX = kMarginPx, offY = kMarginPx;

    // Per-build batch accumulator (texture, verts), drawn into s_rt then discarded.
    std::vector<std::pair<SDL_Texture*, std::vector<SDL_Vertex>>> s_cache;

    // RETAINED-MESH decision (EN_RETAIN). zoomReplay: a pure ZOOM-IN change (dir/edit/loadgen
    // identical, only zoom, AND zooming IN so the captured viewport-cells cover the smaller
    // area) → re-emit the base from cells instead of the per-hex loop. retainCap: a normal
    // full rebuild that should (re)capture the cells. content >> zoom - _uz0 = texture pos.
    const int  _uzx0 = ulSnap.x - offX, _uzy0 = ulSnap.y - offY;   // ulSnap: ONE origin per frame (pan-race fix)
    // Zoom-out budget (EN_RETAIN_ZOUT, default 0 = zoom-IN replay only, exactly as validated).
    // N>0 makes a CAPTURE also resolve N extra levels of zoom-OUT worth of hexes, so a later
    // zoom-OUT up to s_capMaxZoom replays from those cells instead of a full rebuild. Bounded:
    // each level roughly quadruples the captured hex count.
    static int s_zoutBudget = -1;
    if ( s_zoutBudget < 0 ) { const char* _zb = SDL_getenv( "EN_RETAIN_ZOUT" );
        s_zoutBudget = _zb ? atoi( _zb ) : 0; if ( s_zoutBudget < 0 ) s_zoutBudget = 0; if ( s_zoutBudget > 3 ) s_zoutBudget = 3; }
    // Edits since the capture no longer block the replay: the replay rebuilds s_rt from the
    // (capture-state) cells, then re-applies every since-capture edit over it (g_enMeshEdits)
    // — UNLESS that list overflowed (too many to track → full rebuild is safer).
    //
    // COVERAGE: a zoom can also PAN the view (zoom-toward-cursor). If the panned viewport's
    // content slides outside the captured cells' content bbox, the uncovered part re-emits as
    // nothing (BLACK) and caches as valid → a persistent, variable-size black hole on rapid
    // zoom/pan. So require the current viewport's 4 content corners to lie inside the captured
    // bbox; otherwise fall through to a fresh capture (always correct).
    bool capCovers = false;
    if ( s_mirValid && s_capCMaxX >= s_capCMinX )
    {
        const CPoint _wc[4] = { CPoint( 0, 0 ), CPoint( ws.cx - 1, 0 ),
                                CPoint( 0, ws.cy - 1 ), CPoint( ws.cx - 1, ws.cy - 1 ) };
        int vMnX = 0x7FFFFFFF, vMxX = -0x7FFFFFFF, vMnY = 0x7FFFFFFF, vMxY = -0x7FFFFFFF;
        for ( int i = 0; i < 4; ++i )
        {
            CPoint cc[4]; aa.MapToContentHex( aa._WindowToHex( _wc[i] ), cc );
            for ( int k = 0; k < 4; ++k )
            {
                vMnX = __min( vMnX, cc[k].x ); vMxX = __max( vMxX, cc[k].x );
                vMnY = __min( vMnY, cc[k].y ); vMxY = __max( vMxY, cc[k].y );
            }
        }
        capCovers = ( vMnX >= s_capCMinX && vMxX <= s_capCMaxX &&
                      vMnY >= s_capCMinY && vMxY <= s_capCMaxY );
    }
    const bool zoomReplay = TerrainRetainEnabled( ) && needRebuild && !incPan && s_mirValid
                          && !s_baseCells.empty( ) && ( aa.m_iDir & 3 ) == s_mirDir
                          && s_loadGen == s_mirLoadGen && !meshOverflow && capCovers
                          && zoom != s_mirZoom && zoom <= s_capMaxZoom   // zoom-IN always a subset; zoom-OUT within the captured region
                          && !( zoom <= 1 && s_mirZoom >= 2 );   // zoom INTO z0/z1 from a zoomed-out capture → re-capture (cheap, few hexes) to resolve feather
    const bool retainCap = TerrainRetainEnabled( ) && needRebuild && !incPan && !zoomReplay;

    std::vector<unsigned>& s_memoEditQ = C.memoEditQ;     // hexes edited since last rebuild → per-hex tile-memo invalidation
    bool&                  s_memoEditOverflow = C.memoEditOverflow;

    if ( needRebuild )
    {
    Perf::ScopeCounter _cr( incPan ? "t.incpan" : "t.rebuild" );   // mesh (re)build cost
    s_sig = sig; dX = dY = 0;   // (re)built at the current view → zero pan
    s_builtUL = ulSnap;         // the origin this mesh is coherent with
    // A strip-pan moved the viewport beyond the captured margin. The retained
    // zoom-replay cells still exist (content-space), but the replay's coverage gate
    // tests the panned viewport against the capture's content AABB — and the REAL
    // captured region is a rotated parallelogram, so a viewport in an AABB corner
    // passes the gate with NO CELLS there → black diamond/sawtooth bands on the next
    // zoom-in replay (user-reported, persisted after the clipped-edge fix). Pans
    // invalidate the mirror; the next zoom change does a full capture (~100ms at
    // z0/z1) instead of a holed replay.
    if ( incPan )
        s_mirValid = false;
    { std::lock_guard<std::mutex> lk( g_enEditMutex );
      // Queue MY slice of the edit log (edits since THIS ctx's cursor) so the tile-memo
      // (below) can drop just those entries instead of being wiped wholesale (one AI
      // building must not invalidate ~48k cached tiles). Multi-reader: do NOT clear —
      // other area maps still need these entries; the trim at the end of Render drops
      // what everyone has consumed. Cursor below the trimmed base, or a gapped log
      // (cap hit) → edits were lost to us → wipe the memo to stay safe. Queuing past
      // genTop merely over-invalidates the memo — harmless.
      unsigned haveTop = g_enEditBaseGen + (unsigned)g_enEditedHexes.size( );
      if ( s_builtEditGen < g_enEditBaseGen || haveTop < g_enTerrainEditGen )
          s_memoEditOverflow = true;
      else if ( s_builtEditGen < haveTop )
          s_memoEditQ.insert( s_memoEditQ.end( ),
                              g_enEditedHexes.begin( ) + ( s_builtEditGen - g_enEditBaseGen ),
                              g_enEditedHexes.end( ) );
      if ( retainCap ) { g_enMeshEdits.clear( ); g_enMeshEditSet.clear( ); g_enMeshEditOverflow = false;
                         g_enMeshEditAnchor = &C; } }   // replay re-apply list now anchored to THIS ctx's capture
    // Advance MY cursor to genTop — the SAME gen s_sig encodes. (The old live re-read
    // here could bake a residue into the baseSame comparison whenever an edit landed
    // mid-rebuild; edits after genTop stay pending and patch in next frame.)
    s_builtEditGen = genTop;
    if ( retainCap ) { s_baseCells.clear( ); s_waterCells.clear( ); s_waterBandCells.clear( ); s_fogCells.clear( ); s_featherBands.clear( ); }   // capturing a fresh full mesh → drop old cells
    if ( incPan )
    {
        // --- INCREMENTAL PAN: scroll the two texture-accumulated layers (terrain + slope
        // shade) by the pan delta into ping-pong scratch, then swap. Old content is kept;
        // only the newly-revealed strip is re-meshed below (loop skips covered hexes). ---
        auto shiftTex = [&]( SDL_Texture*& tex, SDL_Texture*& scratch,
                             Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca, bool linear ) {
            if ( !scratch )
            {
                scratch = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW, rtH );
                if ( scratch ) { SDL_SetTextureBlendMode( scratch, linear ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND );
                                 if ( linear ) SDL_SetTextureScaleMode( scratch, SDL_ScaleModeLinear ); }
            }
            if ( !scratch ) return;
            SDL_SetRenderTarget( r, scratch );
            SDL_RenderSetViewport( r, nullptr );
            SDL_SetRenderDrawColor( r, cr, cg, cb, ca );
            SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_NONE );
            SDL_RenderClear( r );
            SDL_Rect dst = { shiftX, shiftY, rtW, rtH };   // old content slides by the pan delta
            SDL_RenderCopy( r, tex, nullptr, &dst );
            std::swap( tex, scratch );
        };
        SDL_Texture* _pt = SDL_GetRenderTarget( r ); SDL_Rect _vp; SDL_RenderGetViewport( r, &_vp );
        shiftTex( s_rt,      s_rtB,    0,   0,   0,   0,   false );  // terrain: transparent fill
        shiftTex( s_shadeRT, s_shadeB, 255, 255, 255, 255, true  );  // shade: white = no darkening
        SDL_SetRenderTarget( r, _pt ); SDL_RenderSetViewport( r, &_vp );

        // Compact the PERSISTENT per-hex lists (fog + water): shift each entry's positions by
        // the pan delta and DROP entries that scrolled fully off-texture (clipped → dead). The
        // loop appends the strip's new hexes below; a full rebuild later compacts any residue.
        auto onTex = [&]( float x, float y ) { return x >= 0 && x < rtW && y >= 0 && y < rtH; };
        { size_t w = 0, n = s_fogHex.size( );                       // fog: 4 verts + 1 CHex* per hex
          for ( size_t i = 0; i < n; ++i ) {
              SDL_Vertex* v = &s_fogVerts[i*4]; bool keep = false;
              for ( int k = 0; k < 4; ++k ) { v[k].position.x += shiftX; v[k].position.y += shiftY;
                                              if ( onTex( v[k].position.x, v[k].position.y ) ) keep = true; }
              if ( keep ) { if ( w != i ) { s_fogHex[w] = s_fogHex[i]; for ( int k = 0; k < 4; ++k ) s_fogVerts[w*4+k] = s_fogVerts[i*4+k]; } ++w; } }
          s_fogHex.resize( w ); s_fogVerts.resize( w*4 ); }
        { size_t w = 0, n = s_waterHex.size( );                     // water diamonds: parallel arrays
          for ( size_t i = 0; i < n; ++i ) {
              CPoint* p = &s_waterPos[i*4]; bool keep = false;
              for ( int k = 0; k < 4; ++k ) { p[k].x += shiftX; p[k].y += shiftY; if ( onTex( (float)p[k].x, (float)p[k].y ) ) keep = true; }
              if ( keep ) { if ( w != i ) { s_waterHex[w] = s_waterHex[i]; s_waterAnimOf[w] = s_waterAnimOf[i]; for ( int k = 0; k < 4; ++k ) s_waterPos[w*4+k] = s_waterPos[i*4+k]; } ++w; } }
          s_waterHex.resize( w ); s_waterAnimOf.resize( w ); s_waterPos.resize( w*4 ); }
        { size_t w = 0, n = s_waterBlend.size( );                   // water blend bands
          for ( size_t i = 0; i < n; ++i ) {
              WaterBlendBand& b = s_waterBlend[i]; bool keep = false;
              for ( int k = 0; k < 4; ++k ) { b.p[k].x += shiftX; b.p[k].y += shiftY; if ( onTex( (float)b.p[k].x, (float)b.p[k].y ) ) keep = true; }
              if ( keep ) { if ( w != i ) s_waterBlend[w] = b; ++w; } }
          s_waterBlend.resize( w ); }
        s_shadeVerts.clear(); s_fogNbr.clear();   // shade is re-emitted strip-only; fog nbrs recomputed
        // s_fogVerts/s_fogHex/s_water*/s_waterAnims kept (compacted above); strip appends to them.
    }
    else
    {
    s_fogVerts.clear(); s_fogHex.clear(); s_fogNbr.clear(); s_fogVis.clear(); s_shadeVerts.clear();
    s_waterHex.clear(); s_waterPos.clear(); s_waterBlend.clear();
    s_waterAnimOf.clear();
    if ( !zoomReplay ) s_waterAnims.clear();   // a zoom replay reuses the captured frame tables
    }
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
        int wMiss = 0;
        for ( int f = 0; f < wa.nFrames; ++f )
        {
            char fr = (char)( 'a' + f );
            const Tile* t = Get( tn, wvar, std::string( "aa00" ) + fr + "000" );
            if ( !t ) t = Get( tn, wvar, "aa00a000" );
            if ( !t ) ++wMiss;
            wa.frame[f] = t;
        }
        // DIAGNOSTIC (shore/water data bugs): a variant with NO art points at bad map data
        // (e.g. worldgen writing an out-of-range variant index). The bake falls back so it
        // still renders, but log what was asked for so the generation bug can be chased.
        if ( wMiss )
        {
            Perf::CounterAdd( "rb.watermiss", wMiss );
            static int s_wLogged = 0;
            if ( s_wLogged < 8 )
            { char b[160]; sprintf_s( b, "water anim miss: type=%s variant=%d nullFrames=%d/%d", tn, wvar, wMiss, wa.nFrames ); LogTerrain( b ); ++s_wLogged; }
        }
        int idx = (int)s_waterAnims.size( );
        s_waterAnims.push_back( wa );
        waterAnimKey[key] = idx;
        return idx;
    };

    // Coastline anim resolver — same WaterAnim table, but the shore is DIRECTIONAL
    // (per-hex facing index F 0-38) so it can't be keyed on (type,variant) like open
    // water. The camera dir is fixed for the whole rebuild, so key on F alone and bake
    // the 8 foam frames (a-h) for that orientation. Mirrors TileForHex's coastline arm
    // (kCoastSrcDir / kCoastRot / viewDir = kCoastRot[F] - cameraDir). Entries land in
    // the SAME s_waterAnims vector, so the per-tick bake animates them like the sea.
    std::unordered_map<int,int> coastAnimKey;
    auto resolveCoastAnim = [&]( int F ) -> int {
        bool clamped = ( F < 0 || F > 38 );   // out-of-range facing = bad SHORE data (worldgen/placement)
        if ( clamped ) F = 0;
        auto it = coastAnimKey.find( F );
        if ( it != coastAnimKey.end( ) ) { if ( clamped ) Perf::CounterAdd( "rb.coastclamp", 1 ); return it->second; }
        WaterAnim wa; wa.nFrames = 8;
        int srcDir  = kCoastSrcDir[F];
        int viewDir = ( ( kCoastRot[F] - ( aa.m_iDir & 3 ) ) % 4 + 4 ) % 4;
        int cMiss = 0;
        for ( int f = 0; f < 8; ++f )
        {
            char fr = (char)( 'a' + f );
            std::string stem = std::string( kDirPrefix[viewDir] ) + "00" + fr + "000";
            const Tile* t = Get( "coastline", srcDir, stem );
            if ( !t ) t = Get( "coastline", srcDir, std::string( kDirPrefix[viewDir] ) + "00a000" );
            // Rotation-symmetric variants (3/7/11) ship only the "aa" view — the original
            // draws it at every camera dir. Without this, a rotated camera resolved an
            // ALL-NULL anim for these shores → the hex's hole in s_rt had nothing under it
            // (the "black diamond in shallow water" bug; post-wFall, a wrong open-sea fill).
            if ( !t ) t = Get( "coastline", srcDir, std::string( "aa00" ) + fr + "000" );
            if ( !t ) t = Get( "coastline", srcDir, "aa00a000" );
            if ( !t ) ++cMiss;
            wa.frame[f] = t;
        }
        // DIAGNOSTIC (shore generation/placement bugs): a facing whose art doesn't exist
        // means the map data asked for a shore orientation the tile set doesn't have.
        // Rendering falls back (frame 0 → open sea), but log the request for the data fix.
        if ( cMiss || clamped )
        {
            Perf::CounterAdd( "rb.coastmiss", __max( cMiss, 1 ) );
            static int s_cLogged = 0;
            if ( s_cLogged < 8 )
            { char b[160]; sprintf_s( b, "coast anim miss: F=%d%s srcDir=%d viewDir=%d nullFrames=%d/8", F, clamped ? " (CLAMPED out-of-range)" : "", srcDir, viewDir, cMiss ); LogTerrain( b ); ++s_cLogged; }
        }
        int idx = (int)s_waterAnims.size( );
        s_waterAnims.push_back( wa );
        coastAnimKey[F] = idx;
        return idx;
    };

    // A STATIC (non-animated) neighbour tile exposed as a 1-frame "anim", so the
    // coastline's LAND-side feather can ride the SAME s_waterBlend -> s_waterRT pipeline
    // as its water-side blend. The shore lives in the live layer (a hole in s_rt), so its
    // feather must bake into s_waterRT OVER the opaque coast — not into the transparent-
    // backed s_rt, where a partial-alpha band blends toward black (the "dark fringe").
    std::unordered_map<const Tile*,int> staticAnimKey;
    auto resolveStaticAnim = [&]( const Tile* t ) -> int {
        auto it = staticAnimKey.find( t );
        if ( it != staticAnimKey.end( ) ) return it->second;
        WaterAnim wa; wa.nFrames = 1; wa.frame[0] = t;
        int idx = (int)s_waterAnims.size( );
        s_waterAnims.push_back( wa );
        staticAnimKey[t] = idx;
        return idx;
    };

    // Extend the iteration region to fill the margin band. Estimate screen px per
    // view-hex step from two probe projections, then convert kMarginPx to hexes.
    // When CAPTURING with a zoom-out budget, extend the region further so it covers the
    // hexes a zoomed-OUT (by s_zoutBudget levels) viewport would show — those extra cells
    // let a later zoom-out replay instead of rebuild.
    int capExtraRows = 0;   // extra bottom rows to sweep when capturing the zoom-out region
    int capPadX = 0, capPadY = 0;   // texture-space cull pad so the off-screen budget cells aren't culled
    {
        CPoint a[4], bx[4], by[4];
        aa.MapToWindowHex( CHexCoord( CViewHexCoord( iLeftX,     iTopY     ), TRUE ), a  );
        aa.MapToWindowHex( CHexCoord( CViewHexCoord( iLeftX + 1, iTopY     ), TRUE ), bx );
        aa.MapToWindowHex( CHexCoord( CViewHexCoord( iLeftX,     iTopY + 1 ), TRUE ), by );
        // PER-STEP advance, NOT the sum of both probes' deltas. A single view-Y step
        // advances the screen by only half a hex height (staggered iso rows), but the old
        // pxY summed the x-step's AND y-step's y-deltas (= a full hex), so the margin band
        // converted to HALF the rows it needed — the top ~half of the margin was never
        // meshed, and a blit-covered pan upward scrolled the unmeshed rows into view as a
        // BLACK SAWTOOTH band at the top (user-reported; same undershoot on left/right).
        int colStep = __max( 1, abs( bx[0].x - a[0].x ) );   // screen px per view-X step
        int rowStep = __max( 1, abs( by[0].y - a[0].y ) );   // screen px per view-Y step
        if ( retainCap && s_zoutBudget > 0 )
        {
            int f = ( 1 << s_zoutBudget ) - 1;   // a z+N viewport spans 2^N× the px; need (2^N-1)×half each side
            capPadX = f * ws.cx / 2;
            capPadY = f * ws.cy / 2;
            capExtraRows = capPadY / rowStep;
        }
        iLeftX  -= ( kMarginPx + capPadX ) / colStep + 2;
        iRightX += ( kMarginPx + capPadX ) / colStep + 2;
        iTopY   -= ( kMarginPx + capPadY ) / rowStep + 2;
    }

    // Reference hex for pan tracking + its window-screen pos (NO texture offset — pan tracking
    // is in window space). Use the hex at the WINDOW CENTRE, not the top of the visible span:
    // the top hex's projection can land just above the window and get CULLED by MapToWindowHex
    // (returns false), which left `covered` permanently false at far zoom-out → a full ~1.6s
    // mesh rebuild EVERY frame (0.5fps). A centre hex always projects inside the window.
    s_refHex = aa._WindowToHex( CPoint( ws.cx / 2, ws.cy / 2 ) );
    // refPx from ulSnap-coherent math (content >> zoom - ulSnap), NOT MapToWindowHex's
    // live-UL read: if the view moved since the snapshot, the anchor would carry a
    // constant offset into every later pan-delta measurement (pan-race coherence).
    { CPoint rc[4]; aa.MapToContentHex( s_refHex, rc );
      s_refPx = CPoint( ( rc[0].x >> zoom ) - ulSnap.x, ( rc[0].y >> zoom ) - ulSnap.y ); }
    // Record the view these textures are being built at — the gesture preview uses this to
    // scale-place the old composite when the live view's zoom departs from the built zoom.
    s_builtZoom = zoom; s_builtDir = ( aa.m_iDir & 3 ); s_builtMargin = kMarginPx; s_builtLoadGen = s_loadGen;

    // Redirect rendering into the texture (save/restore target + viewport).
    SDL_Texture* prevTarget = SDL_GetRenderTarget( r );
    SDL_Rect savedVp; SDL_RenderGetViewport( r, &savedVp );
    SDL_SetRenderTarget( r, s_rt );
    SDL_RenderSetViewport( r, nullptr );
    // TRANSPARENT clear: open water is left as holes here and filled by the live water
    // layer UNDER this texture at composite time. The torus map covers the whole
    // viewport+margin, so no visible area is left uncovered by terrain+water.
    // INCREMENTAL PAN: do NOT clear — s_rt already holds the scrolled old content; the strip
    // draws onto it. Compute the still-COVERED texture rect (where the shifted old content
    // landed) so the per-hex loop can skip hexes fully inside it (only the strip re-meshes).
    int covX0 = 0, covX1 = rtW, covY0 = 0, covY1 = rtH;
    if ( incPan )
    {
        // INSET the covered region: hexes at the OLD texture's edges were drawn CLIPPED
        // (SDL clips geometry to the target), and tall terrain near the old TOP edge had
        // its altitude-lifted upper pixels cut off entirely. Treating those border bands
        // as "covered" after the shift made the missing pixels PERMANENT — a black
        // sawtooth band slicing across mountain tops after a zoom-out + pan (user
        // screenshot, 2026-06-12). Re-mesh one tile of border all round, plus the full
        // altitude lift below the top edge (mountains draw upward from their base row).
        const int tileW   = 128 >> zoom, tileH = 64 >> zoom;
        const int altLift = ( 64 << 3 ) >> zoom;
        covX0 = __max( 0, shiftX ) + tileW;
        covX1 = rtW + __min( 0, shiftX ) - tileW;
        covY0 = __max( 0, shiftY ) + tileH + altLift;
        covY1 = rtH + __min( 0, shiftY ) - tileH;
    }
    else
    {
        SDL_SetRenderDrawColor( r, 0, 0, 0, 0 );
        SDL_RenderClear( r );
    }

    // Terrain is drawn ROW BY ROW, back-to-front, so a tall tile's altitude-raised
    // diamond correctly occludes the lower tiles in rows behind it (global texture
    // batching broke this — the ocean batch, drawn after the mountain batch, painted
    // over a mountain in front of it). WITHIN a row, base + feather are batched by
    // texture (rows don't overlap, so order inside a row is free) — that keeps the
    // SDL_RenderGeometry call count low (per-hex drawing tanked the FPS when zoomed
    // out). Each row: draw its base batches, then its feather batches, then advance.
    // Per-row batch buckets. FLAT vectors, not unordered_map: a row holds only a handful of
    // distinct textures, and at z0 the texture changes nearly EVERY hex (terrain variety), so
    // the per-switch map lookup (Debug hash+bucket walk, ~15us) dominated the zoomed-in rebuild
    // (rb.asm was ~20us/hex at z0 vs 0.2us at z3 where long same-texture runs amortized it).
    // A linear scan of <=16 entries is faster in any build and trivial in Debug.
    std::vector<std::pair<SDL_Texture*, std::vector<SDL_Vertex>>> rowBase;
    std::vector<std::pair<SDL_Texture*, std::vector<SDL_Vertex>>> rowFeather;
    auto rowBucket = []( std::vector<std::pair<SDL_Texture*, std::vector<SDL_Vertex>>>& row,
                         SDL_Texture* tex ) -> std::vector<SDL_Vertex>* {
        for ( auto& b : row ) if ( b.first == tex ) return &b.second;
        row.emplace_back( tex, std::vector<SDL_Vertex>( ) );
        return &row.back( ).second;
    };
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
    int  emptyRun    = 0;          // consecutive content-free rows (bottom-stop hysteresis)
    DWORD _gt = GetTickCount( );   // MEASURE: rebuild sub-phases (coarse ms → µs)
    LARGE_INTEGER _qpf; QueryPerformanceFrequency( &_qpf );   // PROFILE: per-hex phase split
    long long _accProj = 0, _accFeath = 0, _accTile = 0, _accWater = 0; LARGE_INTEGER _pa, _pb;
    long long _accShade = 0, _accFog = 0; LARGE_INTEGER _sa, _sb, _fa, _fb;   // MEASURE: split the "other" bucket
    long long _accAsm = 0; LARGE_INTEGER _aa0, _aa1;   // MEASURE: pure base-vertex ASSEMBLY (the
    // retained-mesh re-emit floor — what zoom-replay would still pay after skipping resolution).
    int _hexCnt = 0;   // PROFILE: count of hexes emitted (margin/zoom hex-count vs per-hex cost)

    // Per-hex TileForHex memo for THIS rebuild. TileForHex(hex,dir) depends only on the
    // hex + camera dir (constant per rebuild), but each hex is resolved once as the current
    // hex AND up to 4× as a feather neighbour — so without a cache it runs ~5× per hex.
    // Flat array keyed by map-hex coord (same indexing as the fog precompute below) with a
    // generation stamp so there's no per-rebuild clear. Computes each hex's tile exactly once.
    const int eXc = theMap.Get_eX( ), eYc = theMap.Get_eY( );
    unsigned& s_tileMemoLoadGen = C.tileMemoLoadGen; // s_loadGen the memo was built under
    if ( (int)s_tileCache.size( ) != eXc * eYc * 4 )
    { s_tileCache.assign( (size_t)eXc * eYc * 4, nullptr ); s_tileGen.assign( (size_t)eXc * eYc * 4, 0u );
      for ( int d = 0; d < 4; ++d ) s_tileGenCur[d] = 0;
      s_tileCacheEX = eXc; s_tileCacheEY = eYc; }
    const int tcDir = aa.m_iDir;
    // TileForHex(hex,dir) is INDEPENDENT of zoom and pan — and the memo keeps a slice PER CAMERA
    // DIRECTION — so zoom/pan/rotate all hit warm caches (rotate just switches slices). Only a
    // fresh map LOAD (or edit-list overflow) wipes; individual terrain EDITS invalidate just the
    // edited hexes + neighbours in all 4 slices (below), so the rest stays cached.
    if ( s_tileMemoLoadGen != s_loadGen || s_memoEditOverflow )
    {
        for ( int d = 0; d < 4; ++d )
            if ( ++s_tileGenCur[d] == 0 ) { std::fill( s_tileGen.begin( ), s_tileGen.end( ), 0u );
                                            for ( int j = 0; j < 4; ++j ) s_tileGenCur[j] = 1; break; }
        s_tileMemoLoadGen = s_loadGen;
        s_memoEditQ.clear( ); s_memoEditOverflow = false;       // full wipe supersedes pending per-hex edits
    }
    else if ( !s_memoEditQ.empty( ) )
    {
        // Drop just the edited hexes + their 8 neighbours (coast/road tiles depend on neighbour
        // types) from the memo; everything else stays cached.
        for ( unsigned packed : s_memoEditQ )
            TileMemoInvalidate( (int)( packed >> 16 ), (int)( packed & 0xFFFF ) );
        s_memoEditQ.clear( );
    }
    const size_t   tcPlane = (size_t)eXc * eYc * ( tcDir & 3 );
    const uint32_t tcGen   = s_tileGenCur[tcDir & 3];
    auto cachedTile = [&]( CHex* ph ) -> const Tile* {
        CHexCoord hc = ph->GetHex( );
        size_t k = tcPlane + (size_t)hc.Y( ) * eXc + hc.X( );
        if ( s_tileGen[k] == tcGen ) return s_tileCache[k];
        const Tile* t = TileForHex( ph, tcDir );
        s_tileCache[k] = t; s_tileGen[k] = tcGen;
        return t;
    };

    if ( zoomReplay )
    {
        // RETAINED-MESH ZOOM REPLAY (base layer): re-emit the captured land cells at the new
        // zoom (tex = tile->tex[zoom], pos = (content >> zoom) - _uz0) — skipping the whole
        // per-hex loop. Run-batch consecutive same-texture cells (cells are in row order, so
        // batching within a run keeps back-to-front occlusion). Other layers stay empty this
        // pass (feather/shade/water/fog not yet captured) → base tiles only.
        static const SDL_FPoint uvN[4] = { {0.f,0.5f}, {0.5f,0.f}, {1.f,0.5f}, {0.5f,1.f} };
        static const SDL_FPoint uvT[4] = { {0.5f,0.f}, {0.f,0.5f}, {0.5f,1.f}, {1.f,0.5f} };
        const SDL_Color white = { 255, 255, 255, 255 };
        s_shadeVerts.clear( );   // SHADE layer is now re-emitted from cells (below), not cleared
        SDL_Texture* curTex = nullptr; std::vector<SDL_Vertex>* cur = nullptr;
        for ( const BaseCell& bc : s_baseCells )
        {
            SDL_Texture* tex = TileTexAnyZoom( bc.tile, zoom );   // mip fallback (see helper)
            if ( !tex ) continue;
            const SDL_FPoint* uv = bc.transpose ? uvT : uvN;
            SDL_Vertex v[4];
            int mnx, mxx, mny, mxy;
            for ( int k = 0; k < 4; ++k )
            {
                int px = ( bc.c[k].x >> zoom ) - _uzx0;
                int py = ( bc.c[k].y >> zoom ) - _uzy0;
                v[k].position.x = (float)px; v[k].position.y = (float)py;
                v[k].tex_coord = uv[k]; v[k].color = white;
                if ( k == 0 ) { mnx = mxx = px; mny = mxy = py; }
                else { mnx = __min( mnx, px ); mxx = __max( mxx, px ); mny = __min( mny, py ); mxy = __max( mxy, py ); }
            }
            // CULL off-texture cells: the captured region can be far larger than the current
            // viewport (e.g. captured zoomed-out, now zoomed in), and re-emitting every cell
            // built a huge off-screen mesh — the multi-second zoom. Skip cells fully outside s_rt.
            if ( mxx < 0 || mnx > rtW || mxy < 0 || mny > rtH ) continue;
            if ( tex != curTex ) { s_cache.emplace_back( tex, std::vector<SDL_Vertex>( ) ); cur = &s_cache.back( ).second; curTex = tex; }
            cur->push_back( v[0] ); cur->push_back( v[1] ); cur->push_back( v[3] );
            cur->push_back( v[1] ); cur->push_back( v[2] ); cur->push_back( v[3] );
            // SHADE: re-emit the per-triangle slope brightness from the cell (same diamond,
            // L,T,B = bL / T,R,B = bR), at the new zoom. Matches the per-hex loop's shadeV.
            if ( bc.shade )
            {
                auto sv = [&]( int k, float b ) -> SDL_Vertex {
                    SDL_Vertex s; s.position = v[k].position; s.tex_coord = { 0, 0 };
                    Uint8 g = (Uint8)__min( 255, (int)( b * 255.0f ) ); s.color = { g, g, g, 255 }; return s;
                };
                s_shadeVerts.push_back( sv( 0, bc.bL ) ); s_shadeVerts.push_back( sv( 1, bc.bL ) ); s_shadeVerts.push_back( sv( 3, bc.bL ) );
                s_shadeVerts.push_back( sv( 1, bc.bR ) ); s_shadeVerts.push_back( sv( 2, bc.bR ) ); s_shadeVerts.push_back( sv( 3, bc.bR ) );
            }
            // FEATHER: emit this cell's land<->land bands right after its base+shade, then break
            // the base run (curTex=nullptr) so the bands layer over the base AND a later row's
            // base (a fresh s_cache entry) still occludes them. Each band uses the NEIGHBOUR's
            // texture; corners 0,1 = edge (opaque band), 2,3 = inset (transparent) → thin fade.
            // Skipped at z2/z3 (invisible there) to keep the zoomed-out replay light.
            for ( int fi = 0; zoom <= 1 && fi < bc.fCount; ++fi )
            {
                const FeatherBand& fb = s_featherBands[bc.fStart + fi];
                SDL_Texture* ftex = TileTexAnyZoom( fb.tile, zoom );   // mip fallback
                if ( !ftex ) continue;
                SDL_Vertex fv[4]; int fmnx, fmxx, fmny, fmxy;
                for ( int k = 0; k < 4; ++k )
                {
                    int px = ( fb.c[k].x >> zoom ) - _uzx0;
                    int py = ( fb.c[k].y >> zoom ) - _uzy0;
                    fv[k].position = { (float)px, (float)py }; fv[k].tex_coord = fb.uv[k];
                    Uint8 a = ( k < 2 ) ? (Uint8)135 : (Uint8)0;
                    fv[k].color = { 255, 255, 255, a };
                    if ( k == 0 ) { fmnx = fmxx = px; fmny = fmxy = py; }
                    else { fmnx = __min( fmnx, px ); fmxx = __max( fmxx, px ); fmny = __min( fmny, py ); fmxy = __max( fmxy, py ); }
                }
                if ( fmxx < 0 || fmnx > rtW || fmxy < 0 || fmny > rtH ) continue;
                s_cache.emplace_back( ftex, std::vector<SDL_Vertex>( ) );
                std::vector<SDL_Vertex>& fcv = s_cache.back( ).second;
                fcv.push_back( fv[0] ); fcv.push_back( fv[1] ); fcv.push_back( fv[3] );
                fcv.push_back( fv[0] ); fcv.push_back( fv[3] ); fcv.push_back( fv[2] );
                curTex = nullptr;   // bands broke the base run
            }
        }
        // FOG: re-emit each hex's overlay quad at the new zoom (black, alpha=0 placeholder —
        // the unconditional precompute below rebuilds s_fogNbr/s_fogVis from s_fogHex and the
        // forced fog re-sample fills the real per-corner alpha this same frame). Feather not
        // captured yet.
        s_fogVerts.clear( ); s_fogHex.clear( ); s_fogNbr.clear( );
        for ( const FogCell& fc : s_fogCells )
        {
            int px[4], py[4], mnx, mxx, mny, mxy;
            for ( int k = 0; k < 4; ++k )
            {
                px[k] = ( fc.c[k].x >> zoom ) - _uzx0;
                py[k] = ( fc.c[k].y >> zoom ) - _uzy0;
                if ( k == 0 ) { mnx = mxx = px[k]; mny = mxy = py[k]; }
                else { mnx = __min( mnx, px[k] ); mxx = __max( mxx, px[k] ); mny = __min( mny, py[k] ); mxy = __max( mxy, py[k] ); }
            }
            if ( mxx < 0 || mnx > rtW || mxy < 0 || mny > rtH ) continue;   // cull off-texture fog
            s_fogHex.push_back( fc.hex );
            auto fzv = [&]( int k ) -> SDL_Vertex {
                SDL_Vertex v; v.position = { (float)px[k], (float)py[k] };
                v.tex_coord = { 0, 0 }; v.color = { 0, 0, 0, 0 }; return v;
            };
            s_fogVerts.push_back( fzv( 0 ) ); s_fogVerts.push_back( fzv( 1 ) );   // 4 corners L,T,R,B —
            s_fogVerts.push_back( fzv( 2 ) ); s_fogVerts.push_back( fzv( 3 ) );  // drawn with QuadIndices()
        }
        // WATER: re-emit the captured open-water/coast diamonds + blend bands at the new
        // zoom. s_waterAnims (frame tables) is KEPT from the capture build (zoom-independent),
        // so the captured animIdx values stay valid; only positions re-project. The geom build
        // (s_waterDiamGeom/s_waterBandGeom) below the loop then bakes s_waterRT as usual.
        s_waterHex.clear( ); s_waterPos.clear( ); s_waterAnimOf.clear( ); s_waterBlend.clear( );
        for ( const WaterCell& wc : s_waterCells )
        {
            CPoint wp[4]; int mnx, mxx, mny, mxy;
            for ( int k = 0; k < 4; ++k )
            {
                wp[k] = CPoint( ( wc.c[k].x >> zoom ) - _uzx0, ( wc.c[k].y >> zoom ) - _uzy0 );
                if ( k == 0 ) { mnx = mxx = wp[k].x; mny = mxy = wp[k].y; }
                else { mnx = __min( mnx, wp[k].x ); mxx = __max( mxx, wp[k].x ); mny = __min( mny, wp[k].y ); mxy = __max( mxy, wp[k].y ); }
            }
            if ( mxx < 0 || mnx > rtW || mxy < 0 || mny > rtH ) continue;   // cull off-texture water
            s_waterHex.push_back( wc.hex );
            for ( int k = 0; k < 4; ++k ) s_waterPos.push_back( wp[k] );
            s_waterAnimOf.push_back( wc.animIdx );
        }
        for ( const WaterBandCell& bc : s_waterBandCells )
        {
            WaterBlendBand wb; wb.animIdx = bc.animIdx;
            int mnx, mxx, mny, mxy;
            for ( int k = 0; k < 4; ++k )
            {
                wb.p[k]  = CPoint( ( bc.c[k].x >> zoom ) - _uzx0, ( bc.c[k].y >> zoom ) - _uzy0 );
                wb.uv[k] = bc.uv[k];
                if ( k == 0 ) { mnx = mxx = wb.p[k].x; mny = mxy = wb.p[k].y; }
                else { mnx = __min( mnx, wb.p[k].x ); mxx = __max( mxx, wb.p[k].x ); mny = __min( mny, wb.p[k].y ); mxy = __max( mxy, wb.p[k].y ); }
            }
            if ( mxx < 0 || mnx > rtW || mxy < 0 || mny > rtH ) continue;   // cull off-texture band
            s_waterBlend.push_back( wb );
        }
        s_mirZoom = zoom;   // cells now also valid at this (zoomed-in) level
        Perf::CounterAdd( "rebuild.zoomreplay", 1 );
    }
    else
    for ( int y = iTopY;; ++y )
    {
        bool rowAny = false;
        // Per-row "last texture" cache for the base/feather vertex batches: runs of the
        // same terrain type share a tile texture, so this skips the unordered_map hash on
        // every hex/band (slow under Debug's checked iterators). unordered_map pointers to
        // elements stay valid across inserts, so the cached vector pointer is safe.
        SDL_Texture* lastBaseTex = nullptr; std::vector<SDL_Vertex>* lastBaseVb = nullptr;
        SDL_Texture* lastFeatTex = nullptr; std::vector<SDL_Vertex>* lastFeatVb = nullptr;
        for ( int x = iLeftX; x <= iRightX; ++x )
        {
            CHexCoord hexcoord( CViewHexCoord( x, y ), TRUE );
            CHex*     phex = theMap.GetHex( hexcoord );
            if ( !phex )
                continue;

            // Screen positions of the 4 diamond vertices in (L,T,R,B) order.
            // Project to CONTENT space (zoom/pan-independent) then derive the texture-space
            // screen corners: window = (content >> m_iZoom) - m_ptUL (integer-EXACT match to
            // MapToWindowHex — see CAnimAtr::MapToContentHex), then + the texture offset. The
            // content corners (cpts) are what the retained-mesh path will reuse to re-derive
            // any zoom by re-scaling instead of rebuilding.
            CPoint pts[4];
            CPoint cpts[4];
            QueryPerformanceCounter( &_pa );
            aa.MapToContentHex( hexcoord, cpts );   // cpts[0]=Left 1=Top 2=Right 3=Bottom
            QueryPerformanceCounter( &_pb ); _accProj += _pb.QuadPart - _pa.QuadPart;
            // ulSnap-derived origin (_uzx0), NOT a live aa.m_ptUL re-read: this ran PER HEX,
            // so a pan landing mid-mesh moved the origin between rows — early rows meshed
            // against one origin, later rows against another ("a line cut the screen,
            // one side initialized, the other not"). One snapshot, one origin, coherent mesh.
            for ( int k = 0; k < 4; ++k )
            {
                pts[k].x = ( cpts[k].x >> zoom ) - _uzx0;
                pts[k].y = ( cpts[k].y >> zoom ) - _uzy0;
            }

            // Cull to the texture bounds [0,rtW]x[0,rtH] (= viewport + kMarginPx all
            // round, in texture space). Hexes outside the margin band are dropped.
            // capPadX/Y widen this when CAPTURING a zoom-out budget: the budget cells
            // project off-texture at capture zoom (they belong to a zoomed-OUT view), so
            // without the pad they'd be culled here and never captured → black border on
            // zoom-out. They still render clipped (SDL drops the off-target geometry).
            int minX = pts[0].x, maxX = pts[0].x, minY = pts[0].y, maxY = pts[0].y;
            for ( int i = 1; i < 4; ++i )
            {
                minX = __min( minX, pts[i].x ); maxX = __max( maxX, pts[i].x );
                minY = __min( minY, pts[i].y ); maxY = __max( maxY, pts[i].y );
            }
            if ( maxX < -capPadX || minX > rtW + capPadX || maxY < -capPadY || minY > rtH + capPadY )
                continue;
            // INCREMENTAL PAN: skip hexes whose footprint is fully inside the still-covered
            // region (the scrolled old content already has them) — only the strip re-meshes.
            if ( incPan && minX >= covX0 && maxX <= covX1 && minY >= covY0 && maxY <= covY1 )
                continue;
            rowAny = true;
            ++_hexCnt;   // PROFILE: hexes actually emitted this rebuild

            CTerrainSprite* psprite = phex->GetSprite( );
            int             type    = psprite ? psprite->GetID( ) : -1;
            if ( type < 0 || type >= kNumTypeNames )
                continue;

            QueryPerformanceCounter( &_pa );
            const Tile* tile = cachedTile( phex );             // forest/road/variant (memoized)
            QueryPerformanceCounter( &_pb ); _accTile += _pb.QuadPart - _pa.QuadPart;
            // BLACK-DIAMOND guard. The old `!tile->tex[zoom] → continue` culled the hex from
            // EVERYTHING below — including the open-water/coast capture, which never draws the
            // static tile at all (those hexes are holes in s_rt filled by the live water layer).
            // One missing baked PNG at this zoom = a permanent black diamond. Instead:
            //  - land: fall back to any other zoom's texture (UVs are relative — scales fine);
            //  - animated water/coast: proceed regardless — the live layer is what fills them.
            SDL_Texture* tex = TileTexAnyZoom( tile, zoom );
            if ( !tex && !IsAnimatedTile( type ) )
            {
                Perf::CounterAdd( "rb.notile", 1 );   // land hex with no art at ANY zoom (diagnostic)
                continue;
            }

            // T4 slope shading: shaded land gets per-triangle slope brightness;
            // water/coast stay full colour. T6 fog is applied per-vertex below.
            // GetWorldHex (4 fixed-point corner altitudes) is only needed for the
            // shade math, so fetch it lazily here — skips it for the ~27k unshaded
            // water/coast hexes at full zoom-out (was called unconditionally).
            float bL = 1.0f, bR = 1.0f;
            if ( tile && tile->shade )
            {
                QueryPerformanceCounter( &_sa );
                CMapLoc3D c3d[4];
                hexcoord.GetWorldHex( c3d );
                int z0 = c3d[0].m_fixZ.Round(), z1 = c3d[1].m_fixZ.Round();
                int z2 = c3d[2].m_fixZ.Round(), z3 = c3d[3].m_fixZ.Round();
                bL = TriBrightness( z0, z1, z2, z3, true );
                bR = TriBrightness( z0, z1, z2, z3, false );
                QueryPerformanceCounter( &_sb ); _accShade += _sb.QuadPart - _sa.QuadPart;
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
            bool bTranspose = tile && tile->drawVert && type == CHex::road;
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

            QueryPerformanceCounter( &_aa0 );   // MEASURE: base-vertex assembly start
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
            // Animated water (open sea AND the coastline shore) is NOT baked into the
            // cached texture — it's left as a transparent HOLE and filled by the live
            // wave layer UNDER this texture (composite below). That keeps all water on
            // one shared wave frame (synced) while land in front still occludes it, and
            // — for the shore — lets its foam cycle instead of freezing at the frame the
            // last mesh rebuild happened to capture. Land IS baked.
            if ( !IsAnimatedTile( type ) )
            {
                // tex is guaranteed non-null here (null land tex bailed above) — bucket lazily
                // so water/coast hexes (which never bake their tile) don't open empty buckets.
                if ( tex != lastBaseTex ) { lastBaseVb = rowBucket( rowBase, tex ); lastBaseTex = tex; }
                std::vector<SDL_Vertex>& vb = *lastBaseVb;   // batched within this row
                if ( !vb.capacity( ) ) VbAcquire( vb, 256 ); // pooled capacity (drawRow moves the buffer out each row)
                vb.push_back( vL ); vb.push_back( vT ); vb.push_back( vR ); vb.push_back( vB );   // 4 corners (L,T,R,B) — QuadIndices() expands to 2 tris
                // RETAINED MESH: capture this land tile + its CONTENT corners so a zoom-only
                // change can re-emit it scaled without re-running this loop. (Base layer only
                // for now — feather/shade/water/fog still rebuild; built up next.)
                if ( retainCap )
                    s_baseCells.push_back( { tile, { cpts[0], cpts[1], cpts[2], cpts[3] },
                                             bTranspose, tile->shade != 0, bL, bR,
                                             (int)s_featherBands.size( ), 0 } );   // feather range filled below
            }
            QueryPerformanceCounter( &_aa1 ); _accAsm += _aa1.QuadPart - _aa0.QuadPart;   // MEASURE

            // Capture OPEN-WATER tiles for the live layer: the hex (to re-pick the
            // current frame) + its 4 corner positions (texture space). Also cache a
            // water<->water blend band toward each DIFFERING open-water neighbour, drawn
            // live (neighbour's current frame) so lake/ocean/river seams blend + sync.
            if ( IsOpenWater( type ) )
            {
                QueryPerformanceCounter( &_pa );
                s_waterHex.push_back( phex );
                s_waterPos.push_back( pts[0] ); s_waterPos.push_back( pts[1] );
                s_waterPos.push_back( pts[2] ); s_waterPos.push_back( pts[3] );
                int thisWaterAnim = resolveWaterAnim( type, psprite->GetIndex( ) );
                s_waterAnimOf.push_back( thisWaterAnim );
                if ( retainCap )
                    s_waterCells.push_back( { phex, { cpts[0], cpts[1], cpts[2], cpts[3] }, thisWaterAnim } );

                static const SDL_FPoint wfuv[4] = { {0.f,0.5f}, {0.5f,0.f}, {1.f,0.5f}, {0.5f,1.f} };
                static const int wDX[4] = { 0, 1, 0, -1 };
                static const int wDY[4] = { -1, 0, 1, 0 };
                static const int wSlot[4][4] = { {0,1,2,3}, {3,0,1,2}, {2,3,0,1}, {1,2,3,0} };
                const float kBandW = 0.38f;
                float wcx = ( pts[0].x + pts[1].x + pts[2].x + pts[3].x ) * 0.25f;
                float wcy = ( pts[0].y + pts[1].y + pts[2].y + pts[3].y ) * 0.25f;
                // Water<->water seam bands at ALL zooms (Bug #21). The earlier z0/z1-only
                // gate assumed the operator's "no blending zoomed out" complaint was about
                // LAND seams (those already feather at all zooms below) — but it was
                // re-reported (#21, operator twice) specifically about WATER: river/ocean/
                // lake seams must blend when zoomed OUT too, else the SAME spot blends in but
                // not out. The cost is the 4 GetHex neighbour walks per open-water hex at
                // REBUILD time (~250ms on a 1024 map at z3) — one-time per zoom/dir gesture,
                // NOT per-frame, so it doesn't touch the static-frame fps goal; resolveWaterAnim
                // is a cached lookup (no TileForHex re-resolve). Accepted for blend consistency.
                for ( int e = 0; e < 4; ++e )
                {
                    CHexCoord wnhc( hx + wDX[e], hy + wDY[e] ); wnhc.Wrap( );
                    CHex* wpn = theMap.GetHex( wnhc );
                    if ( !wpn ) continue;
                    CTerrainSprite* wns = wpn->GetSprite( );
                    int wntype = wns ? wns->GetID( ) : -1;
                    if ( !IsOpenWater( wntype ) ) continue;          // water<->water only
                    // Skip the expensive per-neighbour TileForHex (it was ~50% of the whole
                    // rebuild at max zoom-out): the seam-band only needs which water BODY the
                    // neighbour is, i.e. its animation index. Same anim == same type+variant
                    // == same tile == no seam to blend. resolveWaterAnim is a cached hash
                    // lookup; TileForHex re-resolved road/coast/variant strings for nothing.
                    int wnAnim = resolveWaterAnim( wntype, wns->GetIndex( ) );
                    if ( wnAnim == thisWaterAnim ) continue;   // same water body → no band
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
                    if ( retainCap )   // same band in CONTENT space (interior re-derived from content corners)
                    {
                        float wccx = ( cpts[0].x + cpts[1].x + cpts[2].x + cpts[3].x ) * 0.25f;
                        float wccy = ( cpts[0].y + cpts[1].y + cpts[2].y + cpts[3].y ) * 0.25f;
                        WaterBandCell bc; bc.animIdx = wnAnim;
                        bc.c[0] = cpts[c0]; bc.uv[0] = wbnd.uv[0];
                        bc.c[1] = cpts[c1]; bc.uv[1] = wbnd.uv[1];
                        bc.c[2] = CPoint( (int)( cpts[c0].x + kBandW*( wccx - cpts[c0].x ) ), (int)( cpts[c0].y + kBandW*( wccy - cpts[c0].y ) ) ); bc.uv[2] = wbnd.uv[2];
                        bc.c[3] = CPoint( (int)( cpts[c1].x + kBandW*( wccx - cpts[c1].x ) ), (int)( cpts[c1].y + kBandW*( wccy - cpts[c1].y ) ) ); bc.uv[3] = wbnd.uv[3];
                        s_waterBandCells.push_back( bc );
                    }
                }
                QueryPerformanceCounter( &_pb ); _accWater += _pb.QuadPart - _pa.QuadPart;
            }

            // Capture the COASTLINE shore for the live layer too (so its foam animates).
            // Reuses the open-water diamond machinery — same parallel arrays, same fixed
            // diamond UVs in the geometry build — but resolves a DIRECTIONAL coast anim
            // (keyed on the hex facing F). No water<->water blend band: the shore's
            // softening into land/sea is the FEATHER pass below (which still runs for
            // coastline, baking thin neighbour slivers into s_rt OVER this hole).
            if ( type == CHex::coastline )
            {
                QueryPerformanceCounter( &_pa );
                s_waterHex.push_back( phex );
                s_waterPos.push_back( pts[0] ); s_waterPos.push_back( pts[1] );
                s_waterPos.push_back( pts[2] ); s_waterPos.push_back( pts[3] );
                int coastAnim = resolveCoastAnim( psprite->GetIndex( ) );
                s_waterAnimOf.push_back( coastAnim );
                if ( retainCap )
                    s_waterCells.push_back( { phex, { cpts[0], cpts[1], cpts[2], cpts[3] }, coastAnim } );
                QueryPerformanceCounter( &_pb ); _accWater += _pb.QuadPart - _pa.QuadPart;
            }

            // Slope-shade overlay quad: grayscale brightness per triangle (bL/bR), same
            // diamond. Rendered (overwrite, front-wins) into s_shadeRT then blurred, so
            // adjacent tiles' flat shades blend at the seam. MOD-blitted over terrain.
            auto shadeV = []( const CPoint& p, float b ) -> SDL_Vertex {
                SDL_Vertex v; v.position = { (float)p.x, (float)p.y }; v.tex_coord = { 0, 0 };
                Uint8 g = (Uint8)__min( 255, (int)( b * 255.0f ) );
                v.color = { g, g, g, 255 }; return v;
            };
            // Only SHADED tiles need a shade diamond: s_shadeRT is cleared to white
            // (MOD = ×1 = no darkening), and unshaded tiles have bL=bR=1.0, so their quad
            // is an all-white no-op. Skipping them (water/coast, ~half the hexes at full
            // zoom-out) is pixel-identical and saves 6 vert builds + pushes per such hex.
            if ( tile && tile->shade )
            {
                s_shadeVerts.push_back( shadeV( pts[0], bL ) );  // L,T,B  (left tri)
                s_shadeVerts.push_back( shadeV( pts[1], bL ) );
                s_shadeVerts.push_back( shadeV( pts[3], bL ) );
                s_shadeVerts.push_back( shadeV( pts[1], bR ) );  // T,R,B  (right tri)
                s_shadeVerts.push_back( shadeV( pts[2], bR ) );
                s_shadeVerts.push_back( shadeV( pts[3], bR ) );
            }

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
            QueryPerformanceCounter( &_fa );
            s_fogHex.push_back( phex );   // self visibility on re-sample (no GetHex)
            s_fogVerts.push_back( fogV( pts[0], sf[0] ) );   // 4 corners L,T,R,B (was 6 duplicated
            s_fogVerts.push_back( fogV( pts[1], sf[1] ) );   // verts) — drawn with QuadIndices(),
            s_fogVerts.push_back( fogV( pts[2], sf[2] ) );   // and the throttle re-sample rewrites
            s_fogVerts.push_back( fogV( pts[3], sf[3] ) );   // alphas by direct corner slot below.
            QueryPerformanceCounter( &_fb ); _accFog += _fb.QuadPart - _fa.QuadPart;
            if ( retainCap )   // CONTENT corners for the zoom replay (alpha re-sampled each tick)
                s_fogCells.push_back( { phex, { cpts[0], cpts[1], cpts[2], cpts[3] } } );

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
            // LAND feather at ALL zooms (parity with the original renderer, which edge-blended
            // at every zoom; the user sees the hard tile seams at z2/z3). This was gated to
            // z0/z1 when the 4-neighbour resolution meant fresh TileForHex calls (~3s at max
            // zoom-out); cachedTile is now backed by the PERSISTENT per-direction memo, so the
            // neighbour lookups are warm array hits. (Re-measured 2026-06-09: see rb.feather.)
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

                const int idxSelf = psprite->GetIndex( );
                for ( int e = 0; e < 4; ++e )
                {
                    CHexCoord nhc( hx + nbrDX[e], hy + nbrDY[e] ); nhc.Wrap( );
                    CHex* pn = theMap.GetHex( nhc );
                    if ( !pn ) continue;
                    CTerrainSprite* ns = pn->GetSprite( );
                    int ntype = ns ? ns->GetID( ) : -1;
                    if ( !Featherable( ntype ) )
                        continue;
                    // FAST IDENTICAL-NEIGHBOUR SKIP: same type+variant = same tile = no band
                    // (the `ntile == tile` check below would drop it anyway, but only AFTER a
                    // cachedTile resolve). Big uniform fields are the COMMON case, so this
                    // skips the memo hit for most edges — the feather pass's main cost at
                    // zoom-out (rb.feather ~200ms of a 1.1s z3 rebuild on the 1024 map).
                    // fields excluded: its drawn tile also depends on grow stage + coord
                    // rotation, so equal (type,index) does NOT imply the same art there.
                    if ( ntype == type && ntype != CHex::fields && ns->GetIndex( ) == idxSelf )
                        continue;
                    // Coast feather model (b75d66c "coast land-side" + this session's water-side):
                    //  - The COASTLINE tile feathers toward ALL its non-coast neighbours — LAND
                    //    (its terrain-side shore blend) AND OPEN WATER (its water-side blend into
                    //    the live sea). Its bands bake into s_waterRT, over the opaque animated
                    //    coast (NOT s_rt, whose coast holes are transparent-black → dark fringe).
                    //  - A NON-coast tile feathers land<->land only: never toward open water (the
                    //    bright-land/dark-water seam reads as a muddy smear) and never ONTO a
                    //    coastline neighbour (the coast feathers itself — avoids a double band and
                    //    keeps rough/rock off the waterline).
                    if ( type == CHex::coastline )
                    {
                        if ( ntype == CHex::coastline )
                            continue;                       // coast<->coast: one continuous shore, no seam
                    }
                    else if ( IsOpenWater( ntype ) || ntype == CHex::coastline )
                        continue;
                    const Tile* ntile = cachedTile( pn );   // memoized (computed once per hex/rebuild)
                    SDL_Texture* nftex = TileTexAnyZoom( ntile, zoom );   // mip fallback (see helper)
                    if ( !ntile || !nftex || ntile == tile ) continue;

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

                    // The SHORE's feather rides the WATER layer (bakes into s_waterRT, over the
                    // opaque animated coast) — NOT s_rt, whose coast holes are transparent-black
                    // and would darken this partial-alpha band into a black fringe. Reuses the
                    // water-blend-band path: an OPEN-WATER neighbour blends with the LIVE sea
                    // frame; a LAND neighbour via a 1-frame static "anim".
                    if ( type == CHex::coastline )
                    {
                        WaterBlendBand cb;
                        cb.animIdx = IsOpenWater( ntype ) ? resolveWaterAnim( ntype, ns->GetIndex( ) )
                                                          : resolveStaticAnim( ntile );
                        cb.p[0] = pts[c0];                       cb.uv[0] = u0;
                        cb.p[1] = pts[c1];                       cb.uv[1] = u1;
                        cb.p[2] = CPoint( (int)i0x, (int)i0y );  cb.uv[2] = iu0;
                        cb.p[3] = CPoint( (int)i1x, (int)i1y );  cb.uv[3] = iu1;
                        s_waterBlend.push_back( cb );
                        if ( retainCap )   // shore feather band in CONTENT space (interior re-derived)
                        {
                            float fccx = ( cpts[0].x + cpts[1].x + cpts[2].x + cpts[3].x ) * 0.25f;
                            float fccy = ( cpts[0].y + cpts[1].y + cpts[2].y + cpts[3].y ) * 0.25f;
                            WaterBandCell bc; bc.animIdx = cb.animIdx;
                            bc.c[0] = cpts[c0]; bc.uv[0] = u0;
                            bc.c[1] = cpts[c1]; bc.uv[1] = u1;
                            bc.c[2] = CPoint( (int)( cpts[c0].x + kBand*( fccx - cpts[c0].x ) ), (int)( cpts[c0].y + kBand*( fccy - cpts[c0].y ) ) ); bc.uv[2] = iu0;
                            bc.c[3] = CPoint( (int)( cpts[c1].x + kBand*( fccx - cpts[c1].x ) ), (int)( cpts[c1].y + kBand*( fccy - cpts[c1].y ) ) ); bc.uv[3] = iu1;
                            s_waterBandCells.push_back( bc );
                        }
                    }
                    else
                    {
                        SDL_Vertex e0, e1, n0, n1;   // band quad = 2 tris
                        e0.position = { (float)pts[c0].x, (float)pts[c0].y }; e0.tex_coord = u0;  e0.color = { g0, g0, g0, kFeatherA };
                        e1.position = { (float)pts[c1].x, (float)pts[c1].y }; e1.tex_coord = u1;  e1.color = { g1, g1, g1, kFeatherA };
                        n0.position = { i0x, i0y };                          n0.tex_coord = iu0; n0.color = { g0, g0, g0, 0 };
                        n1.position = { i1x, i1y };                          n1.tex_coord = iu1; n1.color = { g1, g1, g1, 0 };

                        SDL_Texture* ftex = nftex;
                        if ( ftex != lastFeatTex ) { lastFeatVb = rowBucket( rowFeather, ftex ); lastFeatTex = ftex; }
                        std::vector<SDL_Vertex>& fvb = *lastFeatVb;   // batched within this row
                        if ( !fvb.capacity( ) ) VbAcquire( fvb, 256 );  // pooled capacity
                        fvb.push_back( e0 ); fvb.push_back( e1 ); fvb.push_back( n1 ); fvb.push_back( n0 );   // 4 corners — QuadIndices() expands to 2 tris (same filled band)
                        if ( retainCap )   // capture the band in CONTENT space, attached to THIS hex's base cell
                        {
                            float fccx = ( cpts[0].x + cpts[1].x + cpts[2].x + cpts[3].x ) * 0.25f;
                            float fccy = ( cpts[0].y + cpts[1].y + cpts[2].y + cpts[3].y ) * 0.25f;
                            FeatherBand fb; fb.tile = ntile;
                            fb.c[0] = cpts[c0]; fb.uv[0] = u0;
                            fb.c[1] = cpts[c1]; fb.uv[1] = u1;
                            fb.c[2] = CPoint( (int)( cpts[c0].x + kBand*( fccx - cpts[c0].x ) ), (int)( cpts[c0].y + kBand*( fccy - cpts[c0].y ) ) ); fb.uv[2] = iu0;
                            fb.c[3] = CPoint( (int)( cpts[c1].x + kBand*( fccx - cpts[c1].x ) ), (int)( cpts[c1].y + kBand*( fccy - cpts[c1].y ) ) ); fb.uv[3] = iu1;
                            s_featherBands.push_back( fb );
                            if ( !s_baseCells.empty( ) ) s_baseCells.back( ).fCount++;
                        }
                    }
                }
            }
            QueryPerformanceCounter( &_pb ); _accFeath += _pb.QuadPart - _pa.QuadPart;
        }

        drawRow();   // flush this row's base then feather batches (back-to-front)

        // Stop once we've passed the bottom of on-screen content — but only after
        // SEVERAL consecutive empty rows: a single flat row projecting just below the
        // texture bottom must not cut off tall terrain from rows further down whose
        // altitude lift draws back INTO the texture (missing mountains at the bottom
        // edge of the margin band, revealed by a later pan). seenContent still guards
        // the empty top band.
        if ( rowAny ) { seenContent = true; emptyRun = 0; }
        else if ( seenContent && ++emptyRun >= 8 )
            break;
        if ( y > iTopY + 4 * ( ws.cy / __max( 1, ( 16 >> zoom ) ) + 8 ) + 64 + capExtraRows )
            break;  // hard safety bound (+ the captured zoom-out region's extra rows)
    }
    Perf::CounterAdd( "reb.loop", (int)( ( GetTickCount( ) - _gt ) * 1000 ) ); _gt = GetTickCount( );
    Perf::CounterAdd( "rb.proj", (int)( _accProj * 1000000 / _qpf.QuadPart ) );      // PROFILE
    Perf::CounterAdd( "rb.feather", (int)( _accFeath * 1000000 / _qpf.QuadPart ) );  // PROFILE
    Perf::CounterAdd( "rb.shade", (int)( _accShade * 1000000 / _qpf.QuadPart ) );    // PROFILE: GetWorldHex+TriBrightness
    Perf::CounterAdd( "rb.fog",   (int)( _accFog   * 1000000 / _qpf.QuadPart ) );    // PROFILE: fog vertex build (every hex)
    Perf::CounterAdd( "rb.hexes", _hexCnt );                                         // PROFILE
    Perf::CounterAdd( "rb.tile",  (int)( _accTile  * 1000000 / _qpf.QuadPart ) );    // PROFILE: TileForHex
    Perf::CounterAdd( "rb.water", (int)( _accWater * 1000000 / _qpf.QuadPart ) );    // PROFILE: water bands
    Perf::CounterAdd( "rb.asm",   (int)( _accAsm   * 1000000 / _qpf.QuadPart ) );    // MEASURE: base-vert assembly (retained re-emit floor)

    // RETAINED MESH: a full capturing rebuild just (re)built s_baseCells for the current
    // (zoom,dir,editGen,loadGen) — mark the mirror valid so a later zoom-in can replay it.
    if ( retainCap )
    {
        s_mirZoom = zoom; s_mirDir = aa.m_iDir & 3;
        s_capMaxZoom = zoom + s_zoutBudget;   // cells now cover down to this zoomed-out level
        s_mirEditGen = genTop; s_mirLoadGen = s_loadGen;   // genTop: one gen read per frame
        s_mirValid = true;
        // CONTENT bbox of the captured region (fog cells = every rendered hex). The replay
        // coverage gate compares the viewport's content against this to avoid black holes.
        if ( s_fogCells.empty( ) ) { s_capCMinX = 0; s_capCMaxX = -1; s_capCMinY = 0; s_capCMaxY = -1; }
        else
        {
            s_capCMinX = s_capCMinY = 0x7FFFFFFF; s_capCMaxX = s_capCMaxY = -0x7FFFFFFF;
            for ( const FogCell& fc : s_fogCells )
                for ( int k = 0; k < 4; ++k )
                {
                    s_capCMinX = __min( s_capCMinX, fc.c[k].x ); s_capCMaxX = __max( s_capCMaxX, fc.c[k].x );
                    s_capCMinY = __min( s_capCMinY, fc.c[k].y ); s_capCMaxY = __max( s_capCMaxY, fc.c[k].y );
                }
        }
    }

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
    // The full-rebuild main loop emits 4-vertex QUADS (base + feather) -> draw indexed. The zoom
    // REPLAY path still emits 6 raw verts/quad, so it draws non-indexed. (s_cache is filled by one
    // path or the other per frame, never mixed: zoomReplay gates the replay, !zoomReplay the loop.)
    const bool cacheQuad = !zoomReplay;
    for ( auto& d : s_cache )
    {
        if ( cacheQuad )
        { int nq = (int)d.second.size( ) / 4;
          SDL_RenderGeometry( r, d.first, d.second.data( ), nq * 4, QuadIndices( nq ), nq * 6 ); }
        else
            SDL_RenderGeometry( r, d.first, d.second.data( ), (int)d.second.size( ), nullptr, 0 );
        VbRecycle( d.second );   // return the buffer's capacity to the pool for the next rebuild
    }

    // RETAINED-MESH zoom replay: the cells are at the CAPTURE's edit state, so re-apply every
    // edit made since the capture over the freshly-drawn (stale) base — the same base-only
    // patch doPatch uses (feather/shade for those few hexes are briefly stale, as doPatch also
    // accepts). g_enMeshEdits persists across replays (cleared only on a capture), so a busy
    // game keeps replaying on zoom instead of forcing a full rebuild each time. (Target = s_rt.)
    if ( zoomReplay )
    {
        std::vector<unsigned> meshEd;
        { std::lock_guard<std::mutex> lk( g_enEditMutex ); meshEd = g_enMeshEdits; }   // COPY: keep the list for the next replay
        if ( !meshEd.empty( ) )
        {
            const SDL_Color white = { 255, 255, 255, 255 };
            const int       pidx[6] = { 0, 1, 3, 1, 2, 3 };
            for ( unsigned packed : meshEd )
            {
                CHexCoord hc( (int)( packed >> 16 ), (int)( packed & 0xFFFF ) );
                CHex* phex = theMap.GetHex( hc );
                if ( !phex ) continue;
                const Tile* tile = TileForHex( phex, aa.m_iDir );
                SDL_Texture* ptex = TileTexAnyZoom( tile, zoom );   // mip fallback (see helper)
                if ( !tile || !ptex ) continue;
                // Project content -> texture against s_builtUL (the origin the mesh was just
                // built with) — NOT MapToWindowHex's live UL, which may have moved already
                // (pan-race: a stamp at the wrong offset bakes a misplaced tile into s_rt).
                CPoint cc[4], pts[4];
                aa.MapToContentHex( hc, cc );
                int mnx = INT_MAX, mny = INT_MAX, mxx = INT_MIN, mxy = INT_MIN;
                for ( int i = 0; i < 4; ++i )
                {
                    pts[i].x = ( cc[i].x >> zoom ) - s_builtUL.x + offX;
                    pts[i].y = ( cc[i].y >> zoom ) - s_builtUL.y + offY;
                    mnx = __min( mnx, pts[i].x ); mxx = __max( mxx, pts[i].x );
                    mny = __min( mny, pts[i].y ); mxy = __max( mxy, pts[i].y );
                }
                if ( mxx < 0 || mnx > rtW || mxy < 0 || mny > rtH ) continue;   // off-texture
                CTerrainSprite* psp = phex->GetSprite( );
                bool bTranspose = tile->drawVert && psp && psp->GetID( ) == CHex::road;
                SDL_FPoint uvL, uvT, uvR, uvB;
                if ( bTranspose ) { uvL = { 0.5f, 0.0f }; uvT = { 0.0f, 0.5f }; uvR = { 0.5f, 1.0f }; uvB = { 1.0f, 0.5f }; }
                else              { uvL = { 0.0f, 0.5f }; uvT = { 0.5f, 0.0f }; uvR = { 1.0f, 0.5f }; uvB = { 0.5f, 1.0f }; }
                SDL_Vertex v[4];
                v[0].position = { (float)pts[0].x, (float)pts[0].y }; v[0].tex_coord = uvL; v[0].color = white;
                v[1].position = { (float)pts[1].x, (float)pts[1].y }; v[1].tex_coord = uvT; v[1].color = white;
                v[2].position = { (float)pts[2].x, (float)pts[2].y }; v[2].tex_coord = uvR; v[2].color = white;
                v[3].position = { (float)pts[3].x, (float)pts[3].y }; v[3].tex_coord = uvB; v[3].color = white;
                SDL_RenderGeometry( r, ptex, v, 4, pidx, 6 );
            }
        }
    }

    // Slope-shade overlay → s_shadeRT, then BLUR it (down-/up-scale through a half-res
    // texture with bilinear filtering) so the per-tile flat shading feathers across
    // tile edges instead of stepping. Clear to white (=multiply by 1, no darkening);
    // draw shade diamonds with OVERWRITE (front-wins, like fog) so overlapping mountain
    // diamonds don't multiply-darken.
    SDL_SetRenderTarget( r, s_shadeRT );
    SDL_RenderSetViewport( r, nullptr );
    // INCREMENTAL PAN: s_shadeRT already holds the scrolled old shade (white = no darkening
    // in the newly-exposed strip), so don't clear — just draw the strip's shade onto it.
    if ( !incPan )
    {
        SDL_SetRenderDrawColor( r, 255, 255, 255, 255 );
        SDL_RenderClear( r );
    }
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
            // Anim resolve can fail for odd variants (missing art / unmapped index). Skipping
            // left the hex as a PERMANENT transparent hole in both s_rt and s_waterRT — showing
            // whatever was behind (black after a gesture-preview frame painted the backdrop).
            // Fall back to anim 0 (open sea) so every water hex gets SOME diamond.
            if ( ai < 0 || ai >= (int)s_waterDiamGeom.size( ) )
                ai = s_waterDiamGeom.empty( ) ? -1 : 0;
            if ( ai < 0 ) continue;
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
        // Copy MY slice (cursor .. log top) under the lock, then patch unlocked. COPY,
        // not swap: the log is multi-reader — other area maps still need these entries
        // (the trim at the end of Render drops what everyone has consumed). patchedTo
        // is the gen this ctx's cursor advances to below.
        std::vector<unsigned> edited;
        unsigned patchedTo = s_builtEditGen;
        { std::lock_guard<std::mutex> lk( g_enEditMutex );
          unsigned haveTop = g_enEditBaseGen + (unsigned)g_enEditedHexes.size( );
          if ( s_builtEditGen >= g_enEditBaseGen && haveTop > s_builtEditGen )
          {
              edited.assign( g_enEditedHexes.begin( ) + ( s_builtEditGen - g_enEditBaseGen ),
                             g_enEditedHexes.end( ) );
              patchedTo = haveTop;
          } }
        // Keep the persistent tile-memo coherent: this fast patch consumes the edits WITHOUT a
        // full rebuild, so drop the edited hexes + neighbours from the memo or a later rebuild
        // would re-bake their stale (pre-edit) tiles from the cache.
        for ( unsigned packed : edited )
            TileMemoInvalidate( (int)( packed >> 16 ), (int)( packed & 0xFFFF ) );
        // Torus-seam wrap periods (texture px). An edited hex on the WRAPPED side of the
        // seam projects (canonically) a full map-width off the cached texture, so its stamp
        // was skipped below and the edit (road preview / new foundation) never appeared until
        // a full rebuild. Below: if the canonical stamp lands off-texture, shift it by ±1
        // wrap period to find the copy that lies on s_rt.
        CPoint cs0[4], csX[4], csY[4];
        aa.MapToContentHex( CHexCoord( 0, 0 ),                cs0 );
        aa.MapToContentHex( CHexCoord( theMap.Get_eX( ), 0 ), csX );
        aa.MapToContentHex( CHexCoord( 0, theMap.Get_eY( ) ), csY );
        const int perXx = ( csX[0].x >> zoom ) - ( cs0[0].x >> zoom );
        const int perXy = ( csX[0].y >> zoom ) - ( cs0[0].y >> zoom );
        const int perYx = ( csY[0].x >> zoom ) - ( cs0[0].x >> zoom );
        const int perYy = ( csY[0].y >> zoom ) - ( cs0[0].y >> zoom );
        for ( unsigned packed : edited )
        {
            CHexCoord hc( (int)( packed >> 16 ), (int)( packed & 0xFFFF ) );
            CHex* phex = theMap.GetHex( hc );
            if ( !phex ) continue;
            const Tile* tile = TileForHex( phex, aa.m_iDir );
            SDL_Texture* ptex = TileTexAnyZoom( tile, zoom );   // mip fallback (see helper)
            if ( !tile || !ptex ) continue;
            // Content -> texture against s_builtUL (what s_rt was built with), replacing the
            // old MapToWindowHex(live UL) + (offX - dX) compensation — the live UL can move
            // between the dX measure and this stamp (pan-race: misplaced patched tiles).
            CPoint cc[4], pts[4];
            aa.MapToContentHex( hc, cc );
            int mnx = INT_MAX, mny = INT_MAX, mxx = INT_MIN, mxy = INT_MIN;
            for ( int i = 0; i < 4; ++i )
            {
                pts[i].x = ( cc[i].x >> zoom ) - s_builtUL.x + offX;
                pts[i].y = ( cc[i].y >> zoom ) - s_builtUL.y + offY;
                mnx = __min( mnx, pts[i].x ); mxx = __max( mxx, pts[i].x );
                mny = __min( mny, pts[i].y ); mxy = __max( mxy, pts[i].y );
            }
            if ( mxx < 0 || mnx > rtW || mxy < 0 || mny > rtH )
            {
                // Canonical stamp is off the cached texture — try the ±1 wrap copies so an
                // edit on the wrapped side of the seam still patches in (s_rt is ~one
                // viewport, so at most one period crosses it).
                bool placed = false;
                for ( int b = -1; b <= 1 && !placed; ++b )
                  for ( int a = -1; a <= 1 && !placed; ++a )
                  {
                      if ( a == 0 && b == 0 ) continue;
                      int sx = a * perXx + b * perYx, sy = a * perXy + b * perYy;
                      int qmnx = INT_MAX, qmny = INT_MAX, qmxx = INT_MIN, qmxy = INT_MIN;
                      for ( int i = 0; i < 4; ++i )
                      { int X = pts[i].x + sx, Y = pts[i].y + sy;
                        qmnx = __min( qmnx, X ); qmxx = __max( qmxx, X );
                        qmny = __min( qmny, Y ); qmxy = __max( qmxy, Y ); }
                      if ( !( qmxx < 0 || qmnx > rtW || qmxy < 0 || qmny > rtH ) )
                      { for ( int i = 0; i < 4; ++i ) { pts[i].x += sx; pts[i].y += sy; } placed = true; }
                  }
                if ( !placed ) continue;   // genuinely off-texture
            }
            // Road tiles flagged drawVert sample the diamond TRANSPOSED (U/V swapped) for
            // vertical-seam continuity — the full-rebuild base pass does this. The patch
            // previously always used the upright UVs, so a freshly built road (especially
            // L/T junctions) rendered 90° ROTATED until a rotate/zoom forced a full rebuild.
            // Match the rebuild so the patch draws it right immediately.
            CTerrainSprite* psp = phex->GetSprite( );
            bool bTranspose = tile->drawVert && psp && psp->GetID( ) == CHex::road;
            SDL_FPoint uvL, uvT, uvR, uvB;
            if ( bTranspose ) { uvL = { 0.5f, 0.0f }; uvT = { 0.0f, 0.5f }; uvR = { 0.5f, 1.0f }; uvB = { 1.0f, 0.5f }; }
            else              { uvL = { 0.0f, 0.5f }; uvT = { 0.5f, 0.0f }; uvR = { 1.0f, 0.5f }; uvB = { 0.5f, 1.0f }; }
            SDL_Vertex v[4];
            v[0].position = { (float)pts[0].x, (float)pts[0].y }; v[0].tex_coord = uvL; v[0].color = white;
            v[1].position = { (float)pts[1].x, (float)pts[1].y }; v[1].tex_coord = uvT; v[1].color = white;
            v[2].position = { (float)pts[2].x, (float)pts[2].y }; v[2].tex_coord = uvR; v[2].color = white;
            v[3].position = { (float)pts[3].x, (float)pts[3].y }; v[3].tex_coord = uvB; v[3].color = white;
            SDL_RenderGeometry( r, ptex, v, 4, idx, 6 );
        }
        SDL_SetRenderTarget( r, pt );
        SDL_RenderSetViewport( r, &vp );
        s_builtEditGen = patchedTo;   // cursor: exactly the log entries stamped above
        // CRITICAL: keep s_sig's edit-gen bits in sync with s_builtEditGen. baseSame is
        // tested next frame as ( sigNoEdit == lastNoEdit ), where
        //   lastNoEdit = s_sig ^ (s_builtEditGen << 40).
        // The rebuild path sets s_sig AND s_builtEditGen together; the patch path only
        // advanced s_builtEditGen, leaving s_sig encoding the OLD edit-gen. That made
        // lastNoEdit != sigNoEdit every frame after a patch → a spurious full ~1.6s
        // rebuild after EVERY edit (the ~1fps zoomed-out stutter). Re-encode s_sig with
        // the pure base (sigNoEdit) XOR the new built edit-gen so next frame matches.
        s_sig = sigNoEdit ^ ( (uint64_t)s_builtEditGen << 40 );
        // (no clear — the log is multi-reader and trimmed at the end of Render; edits the
        //  sim pushed after patchedTo stay pending and are handled next frame)
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
    unsigned& s_fogVisGenSeen = C.fogVisGenSeen;
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
                SDL_Vertex* v = &s_fogVerts[i * 4];   // 4 corners, screen slots L,T,R,B = 0..3
                bool changed = ( v[0].color.a != a0 || v[1].color.a != a1 ||
                                 v[2].color.a != a2 || v[3].color.a != a3 );
                v[0].color.a = a0; v[1].color.a = a1; v[2].color.a = a2; v[3].color.a = a3;
                if ( changed )
                    for ( int k = 0; k < 4; ++k )
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
                int nq = (int)s_fogVerts.size( ) / 4;
                SDL_RenderGeometry( r, nullptr, s_fogVerts.data( ), nq * 4, QuadIndices( nq ), nq * 6 );
            }
            else
            {
                int nq = (int)s_fogBatch.size( ) / 4;
                SDL_RenderGeometry( r, nullptr, s_fogBatch.data( ), nq * 4, QuadIndices( nq ), nq * 6 );
            }
            SDL_SetRenderTarget( r, pt );
            SDL_RenderSetViewport( r, &vp );
        }
    }

    // Composite at the current pan offset (one GPU copy each, ~1 µs): terrain tiles,
    // then the blurred slope-shade (MOD = multiply), then fog (BLEND = dim). The
    // shade texture is flipped to MOD here (it stays NONE for its blur round-trip).
    SDL_Rect dst = { dX - kMarginPx, dY - kMarginPx, rtW, rtH };
    // GESTURE PREVIEW: while a rebuild is deferred, place the old composite under the NEW view.
    // Anchor = the built ref hex: every built-texture pixel t maps to window (t - tRef)*f + refNow,
    // where f = 2^(builtZoom - zoom) and refNow is the anchor's projection in the current view.
    // A rotate (dir change) can't be expressed as a scale — hold the previous placement instead
    // (stale view for the settle window, then the single coalesced rebuild snaps it correct).
    int& s_prevDstX = C.prevDstX; int& s_prevDstY = C.prevDstY;
    SDL_FRect dstF = { 0, 0, 0, 0 }; bool fPreview = false;
    if ( s_deferActive )
    {
        CPoint rp[4];
        if ( s_builtDir == ( aa.m_iDir & 3 ) && aa.MapToWindowHex( s_refHex, rp ) )
        {
            float f = ldexpf( 1.0f, s_builtZoom - zoom );   // <1 zooming out, >1 zooming in
            dstF.x = (float)rp[0].x - ( (float)s_refPx.x + (float)s_builtMargin ) * f;
            dstF.y = (float)rp[0].y - ( (float)s_refPx.y + (float)s_builtMargin ) * f;
            dstF.w = (float)s_rtW * f; dstF.h = (float)s_rtH * f;
        }
        else
        {
            dstF.x = (float)( s_prevDstX != INT_MIN ? s_prevDstX : dst.x );
            dstF.y = (float)( s_prevDstY != INT_MIN ? s_prevDstY : dst.y );
            dstF.w = (float)s_rtW; dstF.h = (float)s_rtH;
        }
        fPreview = true;
        Perf::CounterAdd( "pv.frame", 1 );
    }
    if ( fPreview ) { dst.x = (int)dstF.x; dst.y = (int)dstF.y; }   // scroll signal below sees the live origin
    s_prevDstX = dst.x; s_prevDstY = dst.y;
    // Tell the GPU sprite layer whether the view moved since last frame, so it forces a full
    // sprite re-emit and avoids screen-space ghosts in g_rt (fast vehicles leaving parts on
    // pan). Two complementary signals — neither alone covers all cases:
    //   * blit offset (dst): pixel-precise; catches ZOOMED-IN pans (which blit within the big
    //     margin, so the camera centre barely moves in integer map coords). Resets on rebuild.
    //   * camera centre: catches ZOOMED-OUT pans, which re-mesh every frame (dst stays put).
    {
        CMapLoc c = aa.GetCenter( );
        int& s_lastDx = C.lastDx; int& s_lastDy = C.lastDy; int& s_lastCx = C.lastCx; int& s_lastCy = C.lastCy;
        g_enViewScrolled = ( dst.x != s_lastDx || dst.y != s_lastDy || c.x != s_lastCx || c.y != s_lastCy );
        s_lastDx = dst.x; s_lastDy = dst.y; s_lastCx = c.x; s_lastCy = c.y;
    }

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
            // Use the BUILT zoom's tile art: the cached water geometry is in built-texture space,
            // and during a gesture defer the live `zoom` has already moved on.
            const int wz = ( s_builtZoom >= 0 ) ? s_builtZoom : zoom;
            // Fallback texture for groups whose anim has no usable frame (missing art):
            // the first valid frame of any anim — drawing the WRONG water frame beats
            // leaving a permanent black hole in the bake.
            SDL_Texture* wFall = nullptr;
            for ( const WaterAnim& wa2 : s_waterAnims )
                if ( wa2.nFrames > 0 && wa2.frame[0] && TileTexAnyZoom( wa2.frame[0], wz ) ) { wFall = TileTexAnyZoom( wa2.frame[0], wz ); break; }
            for ( size_t ai = 0; ai < s_waterDiamGeom.size( ); ++ai )   // opaque water diamonds
            {
                std::vector<SDL_Vertex>& wv = s_waterDiamGeom[ai];
                if ( wv.empty( ) ) continue;
                const WaterAnim& wa = s_waterAnims[ai];
                // Frame fallback ladder: this frame (any mip) → frame 0 (any mip) → global
                // fallback. A missing/odd frame must downgrade gracefully, never to a hole.
                const Tile* wt = wa.nFrames > 0 ? wa.frame[ wbase % wa.nFrames ] : nullptr;
                SDL_Texture* wtex = TileTexAnyZoom( wt, wz );
                if ( !wtex && wa.nFrames > 0 ) wtex = TileTexAnyZoom( wa.frame[0], wz );
                if ( !wtex )
                {
                    wtex = wFall; Perf::CounterAdd( "rb.animmiss", 1 );
                    static int s_amLogged = 0;   // identify WHICH anim still draws via wFall
                    if ( s_amLogged < 8 )
                    { int nn = 0; for ( int q = 0; q < wa.nFrames; ++q ) if ( !wa.frame[q] ) ++nn;
                      char b[160]; sprintf_s( b, "animmiss: ai=%d nFrames=%d nullFrames=%d quads=%d", (int)ai, wa.nFrames, nn, (int)( wv.size( ) / 6 ) ); LogTerrain( b ); ++s_amLogged; }
                }
                if ( !wtex ) continue;
                SDL_RenderGeometry( r, wtex, wv.data( ), (int)wv.size( ), nullptr, 0 );
            }
            for ( size_t ai = 0; ai < s_waterBandGeom.size( ); ++ai )   // then water<->water blend bands
            {
                std::vector<SDL_Vertex>& wv = s_waterBandGeom[ai];
                if ( wv.empty( ) ) continue;
                const WaterAnim& wa = s_waterAnims[ai];
                const Tile* nt = wa.nFrames > 0 ? wa.frame[ wbase % wa.nFrames ] : nullptr;
                SDL_Texture* ntex = TileTexAnyZoom( nt, wz );
                if ( !ntex && wa.nFrames > 0 ) ntex = TileTexAnyZoom( wa.frame[0], wz );
                if ( !ntex ) ntex = wFall;
                if ( !ntex ) continue;
                SDL_RenderGeometry( r, ntex, wv.data( ), (int)wv.size( ), nullptr, 0 );
            }
            SDL_SetRenderTarget( r, pt );
            SDL_RenderSetViewport( r, &vp );
            s_waterRTtick = wbase;
        }
    }

    { Perf::ScopeCounter _cc( "p.compose" );   // MEASURE: 4 full-screen RT blits
    if ( fPreview )
    {
        // Transient gesture frame: black backdrop (the scaled texture may not cover the whole
        // viewport when zooming out), then the WHOLE-MAP UNDERLAY (always covers the window,
        // blurry), then the FAR SNAPSHOT (stale wide view, crisper, if one matches) under the
        // crisp layers — the uncovered ring shows real terrain instead of black.
        SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_NONE );
        SDL_SetRenderDrawColor( r, 0, 0, 0, 255 );
        SDL_RenderFillRect( r, nullptr );
        SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
        {
            // Window px of content c = (c >> zoom) - UL; texture px t = ((c-org)>>3)*sc
            // => window = t * 2^(3-zoom)/sc + (org>>zoom) - UL  (org via float, sign-safe).
            const int mdir = aa.m_iDir & 3;
            if ( s_mapRT[mdir] && s_mapLoadGen[mdir] == s_loadGen && s_mapRow[mdir] > 0 )
            {
                float invZ = ldexpf( 1.0f, -zoom );
                float Fm   = ldexpf( 1.0f, 3 - zoom ) / s_mapScale[mdir];
                SDL_FRect dstM;
                dstM.x = (float)s_mapOrg[mdir].x * invZ - (float)aa.m_ptUL.x;
                dstM.y = (float)s_mapOrg[mdir].y * invZ - (float)aa.m_ptUL.y;
                dstM.w = (float)s_mapTexW[mdir] * Fm;
                dstM.h = (float)s_mapTexH[mdir] * Fm;
                // TORUS TILING: the viewport can hang past the map seam; blit the wrapped
                // images too (3x3 at the content-period offsets, culled to the window) so
                // the seam side shows terrain instead of a black band.
                int nDrawn = 0;
                for ( int wy = -1; wy <= 1; ++wy )
                    for ( int wx = -1; wx <= 1; ++wx )
                    {
                        SDL_FRect dW = dstM;
                        dW.x += ( wx * s_mapPerX[mdir].x + wy * s_mapPerY[mdir].x ) * invZ;
                        dW.y += ( wx * s_mapPerX[mdir].y + wy * s_mapPerY[mdir].y ) * invZ;
                        if ( dW.x < (float)ws.cx && dW.y < (float)ws.cy && dW.x + dW.w > 0 && dW.y + dW.h > 0 )
                        { SDL_RenderCopyF( r, s_mapRT[mdir], nullptr, &dW ); ++nDrawn; }
                    }
                Perf::CounterAdd( "pv.mapdraw", nDrawn );
                static int s_mdLogged = 0;   // one-shot placement diagnostics
                if ( s_mdLogged < 6 )
                { char b[240]; sprintf_s( b, "underlay: zoom=%d dir=%d dst=(%.0f,%.0f %.0fx%.0f) win=%dx%d UL=(%ld,%ld) org=(%ld,%ld) sc=%.3f tex=%dx%d perX=(%ld,%ld) perY=(%ld,%ld) n=%d",
                    zoom, mdir, dstM.x, dstM.y, dstM.w, dstM.h, (int)ws.cx, (int)ws.cy, (long)aa.m_ptUL.x, (long)aa.m_ptUL.y,
                    (long)s_mapOrg[mdir].x, (long)s_mapOrg[mdir].y, s_mapScale[mdir], s_mapTexW[mdir], s_mapTexH[mdir],
                    (long)s_mapPerX[mdir].x, (long)s_mapPerX[mdir].y, (long)s_mapPerY[mdir].x, (long)s_mapPerY[mdir].y, nDrawn );
                  LogTerrain( b ); ++s_mdLogged; }
            }
            else
                Perf::CounterAdd( "pv.mapskip", 1 );   // preview frame WITHOUT underlay cover
        }
        if ( s_farRT && s_farDir == ( aa.m_iDir & 3 ) && s_farLoadGen == s_loadGen
             && s_farZoom > s_builtZoom )   // only useful when WIDER than the crisp texture
        {
            CPoint rpF[4];
            if ( aa.MapToWindowHex( s_farRefHex, rpF ) )
            {
                float f2 = ldexpf( 1.0f, s_farZoom - zoom );
                SDL_FRect dstFar;
                dstFar.x = (float)rpF[0].x - ( (float)s_farRefPx.x + (float)s_farMargin ) * f2;
                dstFar.y = (float)rpF[0].y - ( (float)s_farRefPx.y + (float)s_farMargin ) * f2;
                dstFar.w = (float)s_farW * f2; dstFar.h = (float)s_farH * f2;
                SDL_RenderCopyF( r, s_farRT, nullptr, &dstFar );
            }
        }
        if ( s_waterRT && !s_waterHex.empty( ) )
            SDL_RenderCopyF( r, s_waterRT, nullptr, &dstF );
        SDL_RenderCopyF( r, s_rt, nullptr, &dstF );
        SDL_SetTextureBlendMode( s_shadeRT, SDL_BLENDMODE_MOD );
        SDL_RenderCopyF( r, s_shadeRT, nullptr, &dstF );
        SDL_SetTextureBlendMode( s_shadeRT, SDL_BLENDMODE_NONE );
        SDL_RenderCopyF( r, s_fogRT, nullptr, &dstF );
    }
    else
    {
    if ( s_waterRT && !s_waterHex.empty( ) )   // water UNDER terrain (shows through s_rt's holes)
        SDL_RenderCopy( r, s_waterRT, nullptr, &dst );
    SDL_RenderCopy( r, s_rt, nullptr, &dst );
    SDL_SetTextureBlendMode( s_shadeRT, SDL_BLENDMODE_MOD );
    SDL_RenderCopy( r, s_shadeRT, nullptr, &dst );
    SDL_SetTextureBlendMode( s_shadeRT, SDL_BLENDMODE_NONE );
    SDL_RenderCopy( r, s_fogRT, nullptr, &dst );
    } }

    // FAR-SNAPSHOT capture: after a wide (zoom>=2) full rebuild, flatten the composite
    // (water+terrain+shade+fog, 1:1) into s_farRT for the zoom-out preview underlay.
    // Keep the WIDEST snapshot per direction (a z2 build doesn't evict a z3 one), but
    // any dir/load change or same-zoom rebuild refreshes it.
    if ( needRebuild && !incPan && zoom >= 2 &&
         ( zoom >= s_farZoom || s_farDir != ( aa.m_iDir & 3 ) || s_farLoadGen != s_loadGen ) )
    {
        if ( s_farRT && ( s_farW != rtW || s_farH != rtH ) ) { SDL_DestroyTexture( s_farRT ); s_farRT = nullptr; }
        if ( !s_farRT )
        {
            s_farRT = SDL_CreateTexture( r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rtW, rtH );
            if ( s_farRT ) { SDL_SetTextureBlendMode( s_farRT, SDL_BLENDMODE_BLEND );
                             SDL_SetTextureScaleMode( s_farRT, SDL_ScaleModeLinear ); }
        }
        if ( s_farRT )
        {
            SDL_Texture* pt = SDL_GetRenderTarget( r );
            SDL_Rect     vp; SDL_RenderGetViewport( r, &vp );
            SDL_SetRenderTarget( r, s_farRT );
            SDL_RenderSetViewport( r, nullptr );
            SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_NONE );
            SDL_SetRenderDrawColor( r, 0, 0, 0, 255 );
            SDL_RenderClear( r );
            SDL_SetRenderDrawBlendMode( r, SDL_BLENDMODE_BLEND );
            if ( s_waterRT && !s_waterHex.empty( ) ) SDL_RenderCopy( r, s_waterRT, nullptr, nullptr );
            SDL_RenderCopy( r, s_rt, nullptr, nullptr );
            SDL_SetTextureBlendMode( s_shadeRT, SDL_BLENDMODE_MOD );
            SDL_RenderCopy( r, s_shadeRT, nullptr, nullptr );
            SDL_SetTextureBlendMode( s_shadeRT, SDL_BLENDMODE_NONE );
            SDL_RenderCopy( r, s_fogRT, nullptr, nullptr );
            SDL_SetRenderTarget( r, pt );
            SDL_RenderSetViewport( r, &vp );
            s_farZoom = zoom; s_farDir = ( aa.m_iDir & 3 ); s_farMargin = kMarginPx;
            s_farW = rtW; s_farH = rtH; s_farLoadGen = s_loadGen;
            s_farRefHex = s_refHex; s_farRefPx = s_refPx;
            Perf::CounterAdd( "pv.farsnap", 1 );
        }
    }

    // DEBUG: EN_SHOWMAP=1 stretches the whole-map underlay texture over the window
    // (content check). EN_SHOWMAP=2 draws it at its COMPUTED in-world placement, 50%
    // alpha over the live terrain — placement/scale errors show as ghost-doubling.
    static int s_showMap = -1;
    if ( s_showMap < 0 ) { const char* e = SDL_getenv( "EN_SHOWMAP" ); s_showMap = e ? atoi( e ) : 0; }
    if ( s_showMap && s_mapRT[aa.m_iDir & 3] )
    {
        const int mdir = aa.m_iDir & 3;
        if ( s_showMap == 1 )
            SDL_RenderCopy( r, s_mapRT[mdir], nullptr, nullptr );
        else
        {
            float invZ = ldexpf( 1.0f, -zoom );
            float Fm   = ldexpf( 1.0f, 3 - zoom ) / s_mapScale[mdir];
            SDL_FRect dbg;
            dbg.x = (float)s_mapOrg[mdir].x * invZ - (float)aa.m_ptUL.x;
            dbg.y = (float)s_mapOrg[mdir].y * invZ - (float)aa.m_ptUL.y;
            dbg.w = (float)s_mapTexW[mdir] * Fm;
            dbg.h = (float)s_mapTexH[mdir] * Fm;
            SDL_SetTextureAlphaMod( s_mapRT[mdir], 128 );
            SDL_RenderCopyF( r, s_mapRT[mdir], nullptr, &dbg );
            SDL_SetTextureAlphaMod( s_mapRT[mdir], 255 );
            static int s_dbgLogged = 0;
            if ( s_dbgLogged < 4 )
            { char b[200]; sprintf_s( b, "underlay-dbg: zoom=%d dst=(%.0f,%.0f %.0fx%.0f) win=%ldx%ld UL=(%ld,%ld) org=(%ld,%ld) sc=%.3f",
                zoom, dbg.x, dbg.y, dbg.w, dbg.h, (long)ws.cx, (long)ws.cy, (long)aa.m_ptUL.x, (long)aa.m_ptUL.y,
                (long)s_mapOrg[mdir].x, (long)s_mapOrg[mdir].y, s_mapScale[mdir] );
              LogTerrain( b ); ++s_dbgLogged; }
        }
    }

    // WHOLE-MAP UNDERLAY build/continue for the current camera dir. BURST (all rows in
    // one call, ~0.5-1s once) only when NO dir has this load generation yet — i.e. the
    // first frames after a load, which counts as load time ("prewarm during loading is
    // fine"). A rotate to a cold dir builds TIME-SLICED instead, keeping rotates <300ms.
    // Skipped during gesture frames so the slice never inflates a preview frame.
    if ( !fPreview )
    {
        bool anyCur = false;
        for ( int d = 0; d < 4; ++d ) anyCur |= ( s_mapLoadGen[d] == s_loadGen && s_mapRow[d] > 0 );
        BuildMapUnderlay( r, aa, s_loadGen, g_enFogVisGen, !anyCur );
    }

    // EDIT-LOG TRIM (multi-reader): drop the entries every live area-map context has
    // consumed. All Renders run on the main thread; the lock is against the sim's
    // g_enEditHex. If the log GAPPED (cap hit) and everyone is past its recorded
    // entries, restart it empty at the current gen so recording resumes (a reader
    // whose cursor sits inside the discarded gap takes one full rebuild via the
    // completeness check — the gap already was the degraded path).
    { std::lock_guard<std::mutex> lk( g_enEditMutex );
      unsigned minB = g_enTerrainEditGen;
      for ( auto& kv : s_rctx )
          if ( kv.second.builtEditGen < minB )
              minB = kv.second.builtEditGen;
      // Abandon laggards past the PATCH cap. A ctx with more than kEditPatchCap pending
      // can never patch anyway (editsOK requires editCount <= kEditPatchCap → it will
      // full-rebuild, and its memo consumption already tolerates a trimmed-past cursor
      // via the overflow wipe), so holding the log for it buys nothing — while an
      // [X]-closed area map (panel HIDDEN, ctx alive but never rendering) would grow
      // the log to its cap and GAP it, putting every VISIBLE map into recurring full
      // rebuilds. Clamping here keeps the log well under kEditLogCap so live maps keep
      // patching; the hidden map takes one memo-wipe + full rebuild when re-shown.
      if ( g_enTerrainEditGen - minB > (unsigned)kEditPatchCap )
          minB = g_enTerrainEditGen - (unsigned)kEditPatchCap;
      unsigned haveTop = g_enEditBaseGen + (unsigned)g_enEditedHexes.size( );
      bool     gapped  = ( haveTop != g_enTerrainEditGen );
      if ( gapped && minB >= haveTop )
      {
          g_enEditedHexes.clear( );
          g_enEditBaseGen = g_enTerrainEditGen;
      }
      else if ( minB > g_enEditBaseGen )
      {
          unsigned dropTo = ( minB < haveTop ) ? minB : haveTop;
          g_enEditedHexes.erase( g_enEditedHexes.begin( ),
                                 g_enEditedHexes.begin( ) + ( dropTo - g_enEditBaseGen ) );
          g_enEditBaseGen = dropTo;
      }
    }

    // Cursor footprint hatch — live overlay, every frame (animated; shows on a static
    // view). Cheap no-op when not placing a building/rocket.
    DrawBuildCursorOverlay( r, aa );
}
