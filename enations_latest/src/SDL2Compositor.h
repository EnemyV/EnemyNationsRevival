#pragma once

#include <vector>
#include <string>
#include <memory>

struct SDL_Surface;
struct SDL_Window;
union SDL_Event;
class SDL2Panel;
class GameWindow;

// Manages compositing of all SDL2Panels onto the Background Window surface.
// Replaces CWndMain's role as the fullscreen backplate.
//
// Terminology:
//   Background Window  — the root SDL window; renders tiled wallpaper + non-detached panels.
//   Area View          — gameplay/terrain panel (CWndArea). Can be multiple. Detaches to its
//                        own SDL_Window with SDL_WINDOW_ALWAYS_ON_TOP so it floats above the
//                        Background Window. Dialogs temporarily match this to appear on top.
//   World View         — minimap panel (CWndWorld). Also detached.
//
// Each frame:
//   1. If any panel is dirty (or background needs refresh), render the tiled
//      wallpaper background behind all panels
//   2. Iterate panels in z-order, blitting visible panels to the Background Window surface
//   3. Call SDL_UpdateWindowSurface() once at the end
//
// The compositor does NOT own the GameWindow — it holds a non-owning pointer.

class SDL2Compositor {
public:
    explicit SDL2Compositor(GameWindow* window);
    ~SDL2Compositor();

    // Non-copyable
    SDL2Compositor(const SDL2Compositor&) = delete;
    SDL2Compositor& operator=(const SDL2Compositor&) = delete;

    // Load the wallpaper (DIB_GOLD) from the game's bitmap library.
    // Must be called after theBitmaps is loaded (i.e. after game data init).
    // Returns false if wallpaper could not be loaded.
    bool LoadWallpaper();

    // Set a pre-loaded wallpaper surface (e.g. from SDL2MainMenu).
    // The compositor does NOT take ownership — caller must keep it alive.
    void SetWallpaperSurface(SDL_Surface* wallpaper);

    // Panel management
    SDL2Panel* AddPanel(const std::string& name, int x, int y, int w, int h, int zOrder = 0);
    void RemovePanel(SDL2Panel* panel);
    void RemoveAllPanels();

    // Find a panel by name
    SDL2Panel* FindPanel(const std::string& name) const;

    // Composite all panels to the window surface and present.
    // Call this once per frame after all game rendering is complete.
    void Composite();

    // Force a full redraw next frame (all panels + background)
    void InvalidateAll();

    // Route an SDL event through the panel stack (top-down by z-order).
    // Returns true if a panel consumed the event.
    bool RouteEvent(SDL_Event& event);

    // Get the number of panels
    int GetPanelCount() const { return (int)m_panels.size(); }

    // Access panels (for external iteration if needed)
    SDL2Panel* GetPanel(int index) const;

    // Tile the wallpaper across a surface (public for status dialog background)
    void RenderWallpaper(SDL_Surface* windowSurface);

private:

    bool RouteEventInner(SDL_Event& event);

    // Sort panels by z-order (called when panels are added/removed)
    void SortPanels();

    // Bring a panel to front (reassign z-orders to keep them bounded)
    void BringToFront(SDL2Panel* panel);

    GameWindow*  m_window;
    SDL_Surface* m_wallpaper;       // Tiled wallpaper source (not owned)
    SDL_Surface* m_ownedWallpaper;  // Wallpaper we loaded ourselves (owned)
    bool         m_backgroundDirty; // Need to re-render wallpaper?

    // Panel currently being dragged/resized — gets events before all others
    SDL2Panel*   m_activePanel = nullptr;

    // Re-entrancy guard: when >0, RemovePanel defers the erase so that
    // iterators/indices into m_panels stay valid during RouteEvent.
    int          m_routingDepth = 0;
    std::vector<SDL2Panel*> m_pendingRemovals;
    void FlushPendingRemovals();

    // Panels sorted by z-order (ascending: lower z = drawn first = further back)
    std::vector<std::unique_ptr<SDL2Panel>> m_panels;
};
