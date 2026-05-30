#pragma once

#include <string>
#include <functional>
#include <cstdint>

struct SDL_Surface;
struct SDL_Rect;
struct SDL_Window;
union SDL_Event;

// Lightweight rectangular region of the SDL2 window surface.
// Each game "window" (CWndArea, CWndWorld, CWndBar) becomes a panel.
// The panel owns its own backbuffer surface; the compositor blits
// all panels to the window surface in z-order each frame.
//
// Panels can optionally be movable/resizable with a title bar.

class SDL2Panel {
public:
    SDL2Panel(const std::string& name, int x, int y, int w, int h, int zOrder = 0);
    ~SDL2Panel();

    // Non-copyable
    SDL2Panel(const SDL2Panel&) = delete;
    SDL2Panel& operator=(const SDL2Panel&) = delete;

    // Blit this panel's backbuffer onto the window surface at our position.
    // If movable, also renders title bar and resize border.
    void Render(SDL_Surface* windowSurface);

    // Route an SDL event to this panel (hit-test first)
    // Returns true if the event was consumed
    bool HandleEvent(SDL_Event& event);

    // Hit-test: is the given screen-space point inside this panel?
    // If movable, includes the title bar area.
    bool HitTest(int screenX, int screenY) const;

    // Dirty tracking
    void SetDirty() { m_dirty = true; }
    bool IsDirty() const { return m_dirty; }
    void ClearDirty() { m_dirty = false; }

    void Invalidate();

    // Visibility. For a detached panel this also shows/hides its OS window.
    void SetVisible(bool visible);
    bool IsVisible() const { return m_visible; }

    // Z-order (lower = further back, higher = closer to viewer)
    void SetZOrder(int z) { m_zOrder = z; }
    int  GetZOrder() const { return m_zOrder; }

    // Position and size (content area, not including title bar)
    void SetPosition(int x, int y);
    void SetSize(int w, int h);
    void SetRect(int x, int y, int w, int h);

    int GetX() const { return m_x; }
    int GetY() const { return m_y; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    // Access the backbuffer surface
    SDL_Surface* GetSurface() const { return m_surface; }

    // Name/title
    const std::string& GetName() const { return m_name; }
    void SetTitle(const std::string& title) { m_title = title; }

    // Window management
    void SetMovable(bool m) { m_movable = m; }
    bool IsMovable() const { return m_movable; }
    void SetResizable(bool r) { m_resizable = r; }
    bool IsResizable() const { return m_resizable; }
    bool IsDragging() const { return m_dragging; }
    bool IsResizing() const { return m_resizing; }

    // Title bar height. Movable panels draw their own purple title bar in BOTH
    // composited and detached (own borderless OS window) modes — detached game
    // windows are borderless so we render the chrome ourselves, the OS only
    // handles the drag/resize via SDL_SetWindowHitTest.
    int GetTitleBarHeight() const { return m_movable ? TITLE_BAR_HT : 0; }

    // Total rect including title bar
    int GetTotalY() const { return m_y - GetTitleBarHeight(); }
    int GetTotalHeight() const { return m_height + GetTitleBarHeight(); }

    // Event handler callback
    using EventCallback = std::function<bool(SDL_Event& event, int localX, int localY)>;
    void SetEventCallback(EventCallback cb) { m_eventCallback = std::move(cb); }

    // Resize callback (called when user resizes the panel)
    using ResizeCallback = std::function<void(int newW, int newH)>;
    void SetResizeCallback(ResizeCallback cb) { m_resizeCallback = std::move(cb); }
    void InvokeResizeCallback(int w, int h) { if (m_resizeCallback) m_resizeCallback(w, h); }

    // Move callback (called whenever the panel's position or size changes).
    // Used to keep the backing hidden MFC window glued under the visible panel
    // so ScreenToClient-based selection/hit-testing stays aligned wherever the
    // panel is dragged (including onto other monitors when detached).
    // Args are the panel's CONTENT rect in panel-local (compositor) coords.
    using MoveCallback = std::function<void(int x, int y, int w, int h)>;
    void SetMoveCallback(MoveCallback cb) { m_moveCallback = std::move(cb); }
    void InvokeMoveCallback() { if (m_moveCallback) m_moveCallback(m_x, m_y, m_width, m_height); }

    // Close callback (called when user clicks title bar close button)
    using CloseCallback = std::function<void()>;
    void SetCloseCallback(CloseCallback cb) { m_closeCallback = std::move(cb); }
    void SetClosable(bool c) { m_closable = c; }

    // Detached window support — panel gets its own OS-level SDL_Window
    // so it can be dragged to other monitors.
    void Detach(class GameWindow* mainWin);  // Create own SDL_Window, owned by main
    void Detach(SDL_Window* ownerWindow);    // Create own SDL_Window, owned by given window
    void Attach(class GameWindow* mainWin = nullptr); // Destroy own window, return to compositor
    bool IsDetached() const { return m_ownWindow != nullptr; }
    SDL_Window* GetOwnWindow() const { return m_ownWindow; }
    uint32_t    GetOwnWindowID() const { return m_ownWindowID; }

    // Render to own detached window (called by compositor for detached panels)
    void RenderDetached();

    // Suppress syncing position/size to OS window (used during temp coordinate swaps)
    void SuppressWindowSync(bool s) { m_suppressSync = s; }

    // Handle event for detached panel (coords already in content-local space)
    bool HandleDetachedEvent(SDL_Event& event, int localX, int localY);

    // Manual edge/corner resize for the borderless own-window (SDL won't OS-resize
    // a borderless window reliably). Returns true if the event was consumed.
    bool HandleDetachedResize(SDL_Event& event);
    bool IsDetachedResizing() const { return m_dResizing; }

    static const int TITLE_BAR_HT = 20;
    static const int CLOSE_BTN_SIZE = 16;
    static const int RESIZE_BORDER = 6;
    static const int MIN_WIDTH = 80;
    static const int MIN_HEIGHT = 60;
    // Win95-style caption buttons (minimize / maximize / close), right-aligned.
    static const int TITLE_BTN_W = 18;
    static const int TITLE_BTN_H = 14;

    // Title-bar caption button ids (return of HitTestTitleButton).
    enum { TB_BTN_NONE = 0, TB_BTN_MIN = 1, TB_BTN_MAX = 2, TB_BTN_CLOSE = 3 };

private:
    void CreateSurface();
    void FreeSurface();
    // Draw the purple Enemy-Nations-style title bar (gradient + caption text +
    // min/max/close buttons) into `dst` with its top-left at (x, y), width w.
    void DrawTitleBar(SDL_Surface* dst, int x, int y, int w);
    void DestroyOwnWindow();

    // Which caption button (if any) is at screen/window point (sx, sy), using
    // the panel's current (m_x, m_y) title-bar frame. Returns a TB_BTN_* id.
    int HitTestTitleButton(int sx, int sy) const;

    // Determine resize edge at screen point (0=none, 1-8 = N,NE,E,SE,S,SW,W,NW)
    int HitTestResize(int screenX, int screenY) const;

    // Enforce minimum position so title bar / resize borders stay on-screen
    void ClampPosition(int& x, int& y) const;

    std::string  m_name;
    std::string  m_title;
    int          m_x;
    int          m_y;
    int          m_width;
    int          m_height;
    int          m_zOrder;
    bool         m_visible;
    bool         m_dirty;
    SDL_Surface* m_surface;
    EventCallback m_eventCallback;
    ResizeCallback m_resizeCallback;
    CloseCallback m_closeCallback;
    MoveCallback m_moveCallback;

    // Window management state
    bool m_closable = false;
    bool m_movable = false;
    bool m_resizable = false;
    bool m_dragging = false;
    bool m_resizing = false;
    int  m_dragOffX = 0;
    int  m_dragOffY = 0;
    int  m_resizeEdge = 0;
    int  m_resizeOrigX = 0, m_resizeOrigY = 0;
    int  m_resizeOrigW = 0, m_resizeOrigH = 0;
    int  m_resizeStartX = 0, m_resizeStartY = 0;

    // Detached window state
    SDL_Window* m_ownWindow   = nullptr;
    uint32_t    m_ownWindowID = 0;
    bool        m_suppressSync = false;  // skip SDL_SetWindowPos/Size during temp swaps

    // Manual detached-resize state
    bool m_dResizing   = false;
    int  m_dResizeEdge = 0;            // bitmask: 1=L 2=R 4=T 8=B
    int  m_dStartMX = 0, m_dStartMY = 0;   // global mouse at gesture start
    int  m_dStartWX = 0, m_dStartWY = 0;   // window pos at gesture start
    int  m_dStartWW = 0, m_dStartWH = 0;   // window size at gesture start
};
