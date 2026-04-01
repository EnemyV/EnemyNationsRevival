#pragma once

#include <SDL.h>
#include <string>
#include <unordered_map>

struct _TTF_Font;
typedef struct _TTF_Font TTF_Font;

class GameWindow;
class CDIB;

// SDL2 replacement for CDlgMain - renders the main menu on the SDL window.
// Loads wallpaper and button art from the same MISC data file as CDlgMain.
// Handles mouse clicks on button regions and routes to game handlers.
//
// Uses SDL_Surface blitting (not SDL_Renderer) to stay compatible with
// RenderingAdapter's SDL_GetWindowSurface approach.
class SDL2MainMenu {
public:
    SDL2MainMenu();
    ~SDL2MainMenu();

    // Load menu assets from the MISC data file and create SDL surfaces.
    // Must be called after the game's color format is initialized.
    bool Initialize(GameWindow* gameWindow);

    // Clean up surfaces and state.
    void Shutdown();

    // Render the menu to the SDL window surface. Called each frame when menu is active.
    void Render();

    // Render only the wallpaper background (no buttons, no title) - used behind dialogs.
    void RenderWallpaperOnly();

    // Handle an SDL event. Returns true if the event was consumed.
    bool HandleEvent(const SDL_Event& event);

    bool IsInitialized() const { return m_initialized; }

    // Convert a CDIB to an SDL_Surface. Caller owns the returned surface.
    static SDL_Surface* CreateSurfaceFromDIB(CDIB* pDib);

    // Access the tiled wallpaper surface (WL24 - for dialog/game backgrounds)
    SDL_Surface* GetTileWallpaper() const { return m_surfTileWallpaper; }

    // Access the menu background surface (MN24 - for main menu)
    SDL_Surface* GetMenuWallpaper() const { return m_surfWallpaper; }

    // Tile the WL24 wallpaper across an entire surface
    void TileWallpaper(SDL_Surface* dst);

private:
    static const int NUM_BTNS = 11;

    // Button definition matching CDlgMain's _btnData layout
    struct ButtonDef {
        int id;           // Resource ID (for identification)
        int srcX, srcY;   // Position in wallpaper coordinates
        const char* label;
        // Text rect within the button bitmap (wallpaper coords, relative to button)
        int textL, textT, textR, textB;
    };

    // Render text with shadow/outline effect onto a surface
    void RenderTextShadowed(SDL_Surface* dst, TTF_Font* font, const char* text,
                            SDL_Rect dstRect, SDL_Color fg, SDL_Color shadow,
                            bool center = true);

    // Map screen coordinates to wallpaper coordinates
    void ScreenToWallpaper(int screenX, int screenY, int& wallX, int& wallY);

    // Find which button (if any) is at the given screen position. Returns -1 if none.
    int HitTestButton(int screenX, int screenY);

    // Execute a button's action
    void OnButtonClick(int buttonIndex);

    GameWindow* m_gameWindow = nullptr;

    // Surfaces (owned by us)
    SDL_Surface* m_surfWallpaper = nullptr;       // MN24 menu background (stretched)
    SDL_Surface* m_surfTileWallpaper = nullptr;   // WL24 tiled wallpaper (for behind dialogs)
    SDL_Surface* m_surfButtons[NUM_BTNS] = {};

    // Font helpers
    std::string m_fontPath;
    TTF_Font* GetCachedFont(int size);

    // Font cache (size → font)
    std::unordered_map<int, TTF_Font*> m_fontCache;

    // Wallpaper native size (buttons are positioned relative to this)
    int m_wallWidth = 0;
    int m_wallHeight = 0;

    // Per-button bitmap dimensions (full strip: 3 states wide)
    int m_btnStripWidth[NUM_BTNS] = {};
    int m_btnStripHeight[NUM_BTNS] = {};

    // Button state
    int m_hoverButton = -1;    // Button index under mouse, or -1
    int m_pressedButton = -1;  // Button being held down, or -1
    bool m_btnEnabled[NUM_BTNS] = {};

    bool m_initialized = false;

    static ButtonDef s_buttonDefs[NUM_BTNS];
};
