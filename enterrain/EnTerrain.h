#pragma once
//---------------------------------------------------------------------------
// enterrain: the Enemy Nations GPU terrain core, as a standalone static library.
//
// Shared by the game (SDL2Terrain.cpp adapts CHex/theMap/CAnimAtr onto it) and
// by tools/terraintester (renders hand-authored maps with no game running).
// No dependence on theApp/theGame/theMap or any game header: inputs are plain
// ints (terrain type, sprite index, altitude, camera dir/zoom) plus an
// SDL_Renderer.
//
// What lives here:
//   - TileSet: the baked tile textures (tools/terrainbake PNG set, data/terrain_gpu).
//   - SelectTile: which baked tile a hex draws (road/coast rotation, fields, water frames).
//   - Shade/feather rules + constants, slope brightness.
//   - Projection: world -> content space, view-hex <-> map-hex, per-dir corner order.
//   - RenderGrid: the tester's per-frame version of the game's pass over a HexGrid,
//     built on the shared layer code (EnTerrainLayers.h: live water layer, water/coast
//     blend bands, feather). The game keeps its cached pipeline (fog, retained mesh,
//     pan/zoom caches, underlay) in SDL2Terrain.cpp around the same pieces.
//   - Shoreline + road facing rules: EnTerrainShore.h.
//---------------------------------------------------------------------------
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

struct SDL_Renderer;
struct SDL_Texture;

namespace enterrain
{

static const int NUM_ZOOMS = 4;   // z0=128x64 .. z3=16x8 (per terrainbake)

// Terrain types: MUST match CHex's enum (terrain.h); SDL2Terrain.cpp static_asserts it.
enum TerrainType
{
    TerrCity = 0, TerrDesert, TerrForest, TerrLake, TerrHill, TerrMountain, TerrOcean,
    TerrPlain, TerrRiver, TerrRoad, TerrRough, TerrSwamp, TerrCoastline, TerrFields,
    TerrResources, TerrNumTypes
};

// Road facings (CHex::r_*) needed by the tile rules.
static const int RoadX    = 10;   // r_x: 4-way junction
static const int RoadPath = 11;   // r_path: under-construction preview

// Altitudes (CHex enum): a hex draws at max(alt, SeaLevel).
static const int SeaLevel = 16;

// Engine geometry constants (base.h).
static const int HexHt          = 64;   // MAX_HEX_HT: map units per hex edge
static const int HexHtPwr       = 6;    // HEX_HT_PWR
static const int TerrainHtShift = 3;    // TERRAIN_HT_SHIFT

// Baked tile type name for a TerrainType ("" for forest: no tile art, trees are sprites).
const char* TypeName( int type );

// One baked tile at all zoom sizes, plus the render flags from its type.
struct Tile
{
    SDL_Texture* tex[NUM_ZOOMS] = { nullptr, nullptr, nullptr, nullptr };
    int          w = 0, h = 0;    // native (z0) size
    bool         shade       = false;
    bool         drawVert    = false;
    bool         transparent = false;
};

// Any zoom's texture (requested zoom first): a missing PNG at one zoom must not
// cull the hex (UVs are relative, so another zoom's art scales fine).
SDL_Texture* TileTexAnyZoom( const Tile* t, int zoom );

// The baked tile set for ONE SDL_Renderer (textures cannot cross renderers).
// Not copyable (lookup maps hold pointers into `tiles`); swap() keeps them valid.
class TileSet
{
public:
    TileSet() {}
    TileSet( const TileSet& ) = delete;
    TileSet& operator=( const TileSet& ) = delete;
    void swap( TileSet& o );

    // Load every <type>_<variant>_<stem>_z<n>.png in `dir` into textures on `renderer`.
    // `warn` (optional) receives per-file failures. Returns PNG files loaded.
    int  LoadDir( SDL_Renderer* renderer, const std::string& dir,
                  const std::function<void( const std::string& )>& warn = nullptr );

    // Destroy all textures and forget every tile.
    void Unload();

    size_t Size() const { return tiles.size(); }

    const Tile* Get( const std::string& type, int variant, const std::string& stem ) const;
    const Tile* GetDefaultForType( const std::string& type ) const;   // first-loaded of the type
    const Tile* GetTypeVar( int type, int variant ) const;            // (type,variant) representative

    static std::string MakeKey( const std::string& type, int variant, const std::string& stem );

    std::unordered_map<std::string, Tile>        tiles;      // key: type_variant_stem
    std::unordered_map<std::string, const Tile*> byType;     // type -> representative
    std::unordered_map<std::string, const Tile*> byTypeVar;  // "type_variant" -> tile (smallest stem)
    std::vector<const Tile*>                     typeVarPtr[TerrNumTypes];   // integer mirror of byTypeVar
};

// Water wave frame letter for the game clock: one frame every 6 game frames
// (the engine's water Time()=6), cycling nFrames letters from 'a'.
char WaterFrameLetter( unsigned gameFrame, int nFrames );

// Everything about a hex that decides its tile.
struct HexTileQuery
{
    int type      = -1;  // TerrainType
    int index     = 0;   // sprite index (variant / road facing 0-11 / coast facing 0-38)
    int growStage = 0;   // fields only (CHex::GetGrowStage)
    int hx = 0, hy = 0;  // map hex coords (fields furrow rotation)
};

// The baked tile a hex draws at camera dir `iDir` and game clock `gameFrame`.
// (forest -> plain, road facing + rotation, coast facing + foam frame, fields stage +
// coord rotation, open water frame, else the (type,variant) representative.)
const Tile* SelectTile( const TileSet& ts, const HexTileQuery& q, int iDir, unsigned gameFrame );

// Coastline facing F (0-38) -> source dir and per-camera view dir; and its tile for
// one foam frame letter (with the engine's fallback order).
int         CoastSrcDir( int F );
int         CoastViewDir( int F, int iDir );
const Tile* CoastFrameTile( const TileSet& ts, int F, int iDir, char frameLetter );
// Open-water (ocean/lake/river/swamp) tile for one frame letter.
const Tile* WaterFrameTile( const TileSet& ts, const char* typeName, int variant, char frameLetter );

// ---- render rules ----
bool Featherable( int type );      // everything but road/city/resources
bool IsOpenWater( int type );      // ocean/lake/river/swamp
bool IsAnimatedTile( int type );   // open water + coastline (live wave layer in the game)
// Does a hex of (type,index) bleed a feather band toward neighbour (ntype,nindex)?
// Pre-tile-lookup part of the decision; the caller still skips when the resolved
// neighbour tile is null or equals this hex's tile.
bool FeatherToward( int type, int index, int ntype, int nindex );

static const unsigned char kFeatherA = 135;   // feather band edge alpha
static const float         kBand     = 0.38f; // band depth: edge -> 38% toward centre

// Map-neighbour e (0..3) offsets: the hex sharing world edge (corner e, e+1).
static const int kNbrDX[4] = { 0, 1, 0, -1 };
static const int kNbrDY[4] = { -1, 0, 1, 0 };
// Camera-rotation corner permutation: world corner i -> screen slot kCornerSlot[dir][i]
// (screen slots L,T,R,B). Same table as CAnimAtr::MapToContentHex's xaaiIndex.
extern const int kCornerSlot[4][4];

// Slope brightness for one triangle of a hex (engine TerrainGetShadeIndex curve,
// continuous). z0..z3 = corner altitudes (world corner order), left = L,T,B tri.
float TriBrightness( int z0, int z1, int z2, int z3, bool left );

// ---- projection (pure; mirrors CAnimAtr / CHexCoord / CViewHexCoord) ----
// World (map units, x/y) + rounded height px (fixZ << TERRAIN_HT_SHIFT, rounded) ->
// CONTENT space (zoom-0, un-panned view px) for camera dir 0..3.
inline void WorldToContent( int x, int y, int zPx, int iDir, int& outX, int& outY )
{
    int iNewX = 0, iNewY = 0;
    switch ( iDir )
    {
    case 0: iNewX =  x + y; iNewY = ( -x + y ) >> 1; break;
    case 1: iNewX = -x + y; iNewY = ( -x - y ) >> 1; break;
    case 2: iNewX = -x - y; iNewY = (  x - y ) >> 1; break;
    case 3: iNewX =  x - y; iNewY = (  x + y ) >> 1; break;
    default: break;
    }
    outX = iNewX;
    outY = iNewY - zPx;
}

// Map hex -> view hex (dir-dependent). Inverse of ViewHexToMap.
inline void MapToViewHex( int hx, int hy, int iDir, int& vx, int& vy )
{
    switch ( iDir )
    {
    case 0: vx =  hx + hy; vy = -hx + hy; break;
    case 1: vx = -hx + hy; vy = -hx - hy; break;
    case 2: vx = -hx - hy; vy =  hx - hy; break;
    case 3: vx =  hx - hy; vy =  hx + hy; break;
    default: vx = vy = 0; break;
    }
    vx >>= 1;
}

// View hex -> map hex. bRight = jog right (vs left) from the centre hex.
inline void ViewHexToMap( int vx, int vy, bool bRight, int iDir, int& hx, int& hy )
{
    int iY1 = ( vy + !bRight ) >> 1;
    int iY2 = ( vy + bRight ) >> 1;
    switch ( iDir )
    {
    case 0: hx =  vx - iY1; hy =  vx + iY2; break;
    case 1: hx = -vx - iY2; hy =  vx - iY1; break;
    case 2: hx = -vx + iY1; hy = -vx - iY2; break;
    case 3: hx =  vx + iY2; hy = -vx + iY1; break;
    default: hx = hy = 0; break;
    }
}

struct Pt { int x, y; };

// The 4 CONTENT-space corners of hex (hx,hy) in screen-slot order (L,T,R,B), from its
// 4 world-corner draw altitudes (alt of hexes (x,y),(x+1,y),(x+1,y+1),(x,y+1)).
void HexContentCorners( int hx, int hy, const int altDraw[4], int iDir, Pt out[4] );

// ---- reference renderer (terraintester) ----
// A hex grid the renderer reads. Coordinates wrap (the game map is a torus).
class HexGrid
{
public:
    virtual ~HexGrid() {}
    virtual int  Width() const = 0;
    virtual int  Height() const = 0;
    // Hex at (x,y), already wrapped into [0,W)x[0,H).
    virtual int  Type( int x, int y ) const = 0;
    virtual int  Index( int x, int y ) const = 0;
    virtual int  GrowStage( int x, int y ) const { (void)x; (void)y; return 0; }
    virtual int  Alt( int x, int y ) const = 0;    // raw altitude (draws at max(alt, SeaLevel))
};

struct Camera
{
    int dir  = 0;          // 0..3
    int zoom = 0;          // 0 (closest) .. 3
    int ulX = 0, ulY = 0;  // window UL in zoomed view px: screen = (content >> zoom) - ul
};

struct RenderOptions
{
    bool     shade   = true;   // slope-shade overlay (blurred, multiplied), as the game
    bool     feather = true;   // land feather + the shore's bands (water seam bands always on)
    unsigned frame   = 0;      // game clock (water/coast foam frame)
};

// Render-target scratch: land layer, water layer, shade + blur (sized to the output;
// Release() before destroying the renderer).
struct RenderScratch
{
    SDL_Texture* shade = nullptr;
    SDL_Texture* half  = nullptr;
    SDL_Texture* land  = nullptr;
    SDL_Texture* water = nullptr;
    int          w = 0, h = 0;
    void Release();
};

// Draw the whole grid (no wrap copies) back-to-front, row by row, composited into the
// current target of `r`. Caller clears and presents. Returns hexes drawn.
int RenderGrid( SDL_Renderer* r, const TileSet& ts, const HexGrid& g,
                const Camera& cam, const RenderOptions& opt, RenderScratch& scratch );

// Content-space bounding box of the grid at `dir` (for centring the camera).
void GridContentBounds( const HexGrid& g, int dir, int& minX, int& minY, int& maxX, int& maxY );

} // namespace enterrain
