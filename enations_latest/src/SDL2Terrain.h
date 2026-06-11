#pragma once
//---------------------------------------------------------------------------
// SDL2Terrain — GPU terrain path (T2).
//
// Owns the baked terrain tile textures (loaded from the PNG set produced by
// tools/terrainbake) for a given SDL_Renderer, and (later phases) builds the
// hex vertex mesh and draws it with SDL_RenderGeometry. Created against the
// detached area-map panel's own renderer (see SDL2Panel / T0b).
//---------------------------------------------------------------------------
#include <string>
#include <unordered_map>
#include <cstdint>

struct SDL_Renderer;
struct SDL_Texture;
class  CAnimAtr;
class  CHex;

class SDL2Terrain
{
public:
    static const int NUM_ZOOMS = 4;   // z0=128x64 .. z3=16x8 (per terrainbake)

    // One baked tile at all zoom sizes, plus the render flags from its type.
    struct Tile
    {
        SDL_Texture* tex[NUM_ZOOMS] = { nullptr, nullptr, nullptr, nullptr };
        int          w = 0, h = 0;    // native (z0) size
        bool         shade       = false;
        bool         drawVert    = false;
        bool         transparent = false;
    };

    // Load every baked PNG into textures owned by this renderer. Searches the
    // known asset locations (see .cpp). Idempotent; returns tiles loaded (0 =
    // assets not found / renderer null). Safe to call once per renderer.
    static int  Load( SDL_Renderer* renderer );

    // Free all textures (call before destroying the renderer).
    static void Unload();

    static bool IsLoaded() { return s_loaded; }

    // The renderer the tile textures are currently bound to (null if unloaded).
    // Used by the panel teardown to avoid one area panel's destroy clobbering the
    // global terrain state that a DIFFERENT (newer) area panel now owns — during an
    // in-game load the new panel is created (and Load()s tiles) BEFORE the old panel
    // is destroyed, so the old panel must not Unload() the new panel's tiles.
    static SDL_Renderer* CurrentRenderer() { return s_renderer; }

    // Look up a tile by engine identity. type = CHex type name (plain, road,
    // coastline, ...); variant = CHex sprite index; stem = the source tile stem
    // (e.g. "aa00c000") whose first 2 chars are the view direction (aa/ac/ae/ag)
    // and char 4 the sub-shape. Returns nullptr if absent (caller falls back).
    static const Tile* Get( const std::string& type, int variant,
                            const std::string& stem );

    // T2.2a first-light: draw the visible terrain hexes as textured GPU quads
    // into `renderer`, using the engine's own MapToWindowHex transform (so it
    // aligns with the software render) and a per-type representative tile
    // (exact (type,index,dir) tile parity is T2.2b). Caller clears/presents.
    static void Render( SDL_Renderer* renderer, const CAnimAtr& aa );

    // A representative tile for a CHex type name (first one loaded). Used by
    // the first-light render until the exact MakeRotated mapping lands.
    static const Tile* GetDefaultForType( const std::string& type );

    // GPU device-lost (SDL_RENDER_TARGETS_RESET / SDL_RENDER_DEVICE_RESET): render-
    // target textures keep their handles but lose their CONTENTS. Marks every cached
    // texture (terrain composite, water, shade, fog, far snapshot, whole-map underlay)
    // for a full re-render on the next frame. Called from the event pump.
    static void NotifyTargetsLost();

    // Resolve the tile a hex should draw (forest→plain, road facing+rotation,
    // else variant). Shared by the main pass, the T5 edge-feather pass, and the
    // whole-map far-underlay builder (a file-scope helper in the .cpp — which is
    // why this is public).
    static const Tile* TileForHex( CHex* phex, int iDir );

private:
    static std::unordered_map<std::string, Tile> s_tiles;  // key: type_variant_stem
    static std::unordered_map<std::string, const Tile*> s_byType;       // type → representative
    static std::unordered_map<std::string, const Tile*> s_byTypeVar;    // "type_variant" → tile (prefers aa dir)
    static SDL_Renderer*                          s_renderer;
    static bool                                   s_loaded;

    static std::string MakeKey( const std::string& type, int variant,
                                const std::string& stem );
};
