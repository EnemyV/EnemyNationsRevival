#include "stdafx.h"
#include "GameWindow.h"
#include "framecap.h"   // #45 frame-capture debug mode
#ifndef _WIN32
#include "en_harness.h"   // EnHarness_Service() — services screenshots on the render thread
#endif
#include "SDL2UI.h"
#include "lastplnt.h"
#include "resource.h"
#include "RenderingAdapter.h"
#include "SDL2MainMenu.h"
#include "SDL2Compositor.h"
#include "SDL2CreateStatus.h"
#include "SDL2Panel.h"
#include "area.h"         // CWndArea, theAreaList (Esc deselect-vs-options decision)
#include "music.h"        // theMusicPlayer (pause/resume on app focus change)
#include "Perf.h"         // GaugeSet — correlate window state with frame cost
#include "w22_settings.h" // w22::GetProfileInt — [Advanced] Renderer flag (T0)
#include "SDL2Terrain.h"  // NotifyTargetsLost — rebuild cached RTs after GPU device-lost
#include "RenderBackend.h" // RenderBackendIsGpu() — 3-way backend selector
#include "../rendering/SDLButtonManager.h"
#include "../rendering/StatusBar.h"
#include "../input/UIButtonListener.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM
#endif

// SDL2 headers
#include <SDL.h>
#ifdef _WIN32
#include <SDL_syswm.h>
#endif

// Helper function for logging - write to current directory
static void LogToFile(const std::string& message) {
    std::ofstream log("GameWindow_Debug.log", std::ios::app);
    if (log.is_open()) {
        log << message << std::endl;
        log.close();
    }
}

GameWindow::GameWindow(const std::string& title, int width, int height)
    : m_title(title),
      m_width(width),
      m_height(height) {
}

GameWindow::~GameWindow() {
    Cleanup();
}

std::shared_ptr<GameWindow> GameWindow::Create(const std::string& title, int width, int height) {
    auto window = std::make_shared<GameWindow>(title, width, height);

    LogToFile("Creating GameWindow: " + title);

    // Initialize SDL directly on the main thread
    if (!window->InitializeSDL()) {
        LogToFile("ERROR: Failed to initialize SDL");
        return nullptr;
    }

    LogToFile("GameWindow initialized successfully");

    // Initialize RenderingAdapter with the window
    RenderingAdapter::Initialize(window.get());
    LogToFile("RenderingAdapter initialized");

    // Create compositor (Phase 1 — backplate + panel system)
    window->m_compositor = std::make_unique<SDL2Compositor>(window.get());
    LogToFile("SDL2Compositor created");

    // Initialize UI components
    if (!window->InitializeUI()) {
        LogToFile("WARNING: Failed to initialize UI components");
        // Don't fail, UI is optional
    }

    return window;
}

// ---------------------------------------------------------------------------
// Cross-platform borderless-window resize via SDL_SetWindowHitTest.
// SDL calls this callback on every mouse move; returning an SDL_HitTestResult
// other than SDL_HITTEST_NORMAL tells SDL to start an OS-level resize/drag.
// The OS then shows the correct resize cursor automatically.
// ---------------------------------------------------------------------------
static const int RESIZE_GRIP = 8;  // pixels along each edge/corner

static SDL_HitTestResult SDLCALL BorderHitTest(SDL_Window* win, const SDL_Point* pt, void*) {
    int w, h;
    SDL_GetWindowSize(win, &w, &h);

    bool left   = pt->x < RESIZE_GRIP;
    bool right  = pt->x >= w - RESIZE_GRIP;
    bool top    = pt->y < RESIZE_GRIP;
    bool bottom = pt->y >= h - RESIZE_GRIP;

    if (top && left)     return SDL_HITTEST_RESIZE_TOPLEFT;
    if (top && right)    return SDL_HITTEST_RESIZE_TOPRIGHT;
    if (bottom && left)  return SDL_HITTEST_RESIZE_BOTTOMLEFT;
    if (bottom && right) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    if (top)             return SDL_HITTEST_RESIZE_TOP;
    if (bottom)          return SDL_HITTEST_RESIZE_BOTTOM;
    if (left)            return SDL_HITTEST_RESIZE_LEFT;
    if (right)           return SDL_HITTEST_RESIZE_RIGHT;

    return SDL_HITTEST_NORMAL;
}

// ---------------------------------------------------------------------------
// Resize-cursor management.  SDL_SetWindowHitTest makes the OS perform the
// resize drag, but on some platforms the cursor doesn't change on hover alone.
// We keep a small set of system cursors and set the right one in PollEvents().
// ---------------------------------------------------------------------------
static SDL_Cursor* s_cursors[5] = {};  // arrow, sizeWE, sizeNS, sizeNWSE, sizeNESW

static void EnsureResizeCursors() {
    if (!s_cursors[0]) {
        s_cursors[0] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        s_cursors[1] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);    // left/right
        s_cursors[2] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);    // top/bottom
        s_cursors[3] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);  // NW-SE diagonal
        s_cursors[4] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);  // NE-SW diagonal
    }
}

void GameWindow::SetArrowCursor() {
    EnsureResizeCursors();
    if (s_cursors[0]) {
        SDL_ShowCursor(SDL_ENABLE);
        SDL_SetCursor(s_cursors[0]);
    }
#ifdef _WIN32
    static HCURSOR s_hArrow = ::LoadCursor(NULL, IDC_ARROW);
    if (s_hArrow)
        ::SetCursor(s_hArrow);
    EnsureCursorVisible();
#endif
}

void GameWindow::EnsureCursorVisible() {
#ifdef _WIN32
    // Win32 maintains a display counter for the cursor. movie.cpp calls
    // ShowCursor(FALSE) during intro playback. If the counter never returns to
    // 0 (or worse, goes negative) the cursor stays hidden regardless of
    // ::SetCursor. Drive the counter back to >=0 without changing what cursor
    // shape is shown — game code's ::SetCursor(m_hCurReg / m_hCurMove / ...)
    // calls then take effect normally.
    int count = ::ShowCursor(TRUE);
    int guard = 0;
    while (count < 0 && guard++ < 20) {
        count = ::ShowCursor(TRUE);
    }
    // If we incremented past 0, push back to exactly 0 so we don't permanently
    // shift the counter (otherwise legitimate hide calls won't work).
    while (count > 0) {
        count = ::ShowCursor(FALSE);
    }
#endif
}

bool GameWindow::IsAreaPanelWindow(uint32_t winID) const {
    if (!m_compositor || winID == 0)
        return false;
    for (int i = 0; i < m_compositor->GetPanelCount(); i++) {
        SDL2Panel* p = m_compositor->GetPanel(i);
        if (p && p->IsDetached() && p->GetOwnWindowID() == winID &&
            p->GetName().rfind("area_", 0) == 0)
            return true;
    }
    return false;
}

#ifdef _WIN32
// Subclass the SDL window to intercept WM_SETCURSOR.
// The game changes cursors via ::SetCursor() in CWndArea::SetMouseState().
// SDL's default wndproc handles WM_SETCURSOR by resetting to the SDL cursor,
// which undoes the game's cursor. We intercept WM_SETCURSOR and return TRUE
// so the game's cursor sticks.
static WNDPROC s_sdlOrigWndProc = NULL;

static LRESULT CALLBACK SdlSubclassWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT) {
        // Returning TRUE alone preserves whatever cursor was last ::SetCursor'd —
        // which the game's CWndArea::SetMouseState() updates on every mouse move.
        // Do NOT force a system arrow here; that would override game-specific
        // cursors (move, select, attack, build, repair, ...).
        return TRUE;
    }
    // SDL2's default WM_MOUSEACTIVATE handler returns MA_ACTIVATEANDEAT when an
    // inactive window receives a click — the click activates the window but is
    // consumed, forcing users to click twice. Override to MA_ACTIVATE so the
    // first click both activates and registers.
    if (msg == WM_MOUSEACTIVATE) {
        return MA_ACTIVATE;
    }
    // Defeat SDL's OTHER first-click swallow: on WM_ACTIVATE with WA_CLICKACTIVE
    // (window activated by a mouse click), SDL arms `focus_click_pending` and
    // eats the matching button-down AND button-up — so the click that brings an
    // inactive window forward is lost, both when the whole app was in the
    // background and when switching between our own windows. SDL only arms this
    // for WA_CLICKACTIVE, so rewrite it to WA_ACTIVE before SDL sees it: the
    // window still activates, but no click is swallowed. (The MA_ACTIVATE above
    // and the SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH hint are meant to handle this,
    // but proved unreliable across this multi-window setup; this is the
    // deterministic fix.)
    if (msg == WM_ACTIVATE && LOWORD(wParam) == WA_CLICKACTIVE) {
        wParam = MAKEWPARAM(WA_ACTIVE, HIWORD(wParam));
    }
    // Each SDL window stores its original wndproc as a window property so we
    // can route through the right one. Fall back to the static (main-window)
    // copy for backwards compat with installs that predate per-window storage.
    WNDPROC orig = (WNDPROC)::GetProp(hWnd, "EN_origWndProc");
    if (!orig) orig = s_sdlOrigWndProc;
    return ::CallWindowProc(orig, hWnd, msg, wParam, lParam);
}
#endif // _WIN32

#ifdef _WIN32
// MFC installs a CBT hook that subclasses ALL windows created on its thread with
// _AfxActivationWndProc. SDL windows aren't created through MFC, so the hook stores
// garbage for the old wndproc and crashes. We temporarily install our own CBT hook
// that intercepts HCBT_CREATEWND *without* passing it to MFC's hook, so MFC never
// sees the SDL window creation.
static HHOOK s_sdlCbtHook = NULL;

static LRESULT CALLBACK SdlCbtFilterHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HCBT_CREATEWND)
        return 0;  // Allow creation, but don't pass to MFC's hook
    return ::CallNextHookEx(s_sdlCbtHook, nCode, wParam, lParam);
}
#endif // _WIN32

SDL_Window* GameWindow::CreateSDLWindow(const char* title, int x, int y, int w, int h, Uint32 flags) {
#ifdef _WIN32
    // Protect against MFC's CBT hook corrupting the new SDL window
    HHOOK hook = ::SetWindowsHookEx(WH_CBT, SdlCbtFilterHook, NULL, ::GetCurrentThreadId());
#endif
    SDL_Window* win = SDL_CreateWindow(title, x, y, w, h, flags);
#ifdef _WIN32
    if (hook) {
        ::UnhookWindowsHookEx(hook);
    }

    // Subclass every SDL window — including detached panel windows — so
    // WM_SETCURSOR forces a visible arrow. Otherwise SDL's default handler
    // leaves the cursor as whatever the game last ::SetCursor()'d, which
    // includes NULL (invisible) during rocket placement / build modes.
    if (win) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(win, &wmInfo)) {
            HWND hSDL = wmInfo.info.win.window;
            if (::GetProp(hSDL, "EN_origWndProc") == NULL) {
                WNDPROC orig = (WNDPROC)::GetWindowLongPtr(hSDL, GWLP_WNDPROC);
                ::SetProp(hSDL, "EN_origWndProc", (HANDLE)orig);
                ::SetWindowLongPtr(hSDL, GWLP_WNDPROC, (LONG_PTR)SdlSubclassWndProc);
                if (!s_sdlOrigWndProc)
                    s_sdlOrigWndProc = orig;
            }
        }
    }
#endif
    return win;
}

bool GameWindow::InitializeSDL() {
    LogToFile("Initializing SDL...");

    // Deliver the click that focuses an unfocused window as a normal mouse
    // event (instead of swallowing it for focus only). This lets the FIRST
    // click on a detached map window select a unit / start a resize, without a
    // separate "click to activate" first.
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    // Initialize SDL with video and timer subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        LogToFile(std::string("ERROR: SDL_Init failed: ") + SDL_GetError());
        return false;
    }

    LogToFile("SDL initialized");

    std::string sizeStr = std::string("Window size: ") + std::to_string(m_width) + "x" + std::to_string(m_height);
    LogToFile(sizeStr);

#ifdef _WIN32
    // Install temporary CBT hook to prevent MFC from subclassing the SDL window.
    // Hooks are LIFO, so ours fires first and blocks MFC from seeing the creation.
    s_sdlCbtHook = ::SetWindowsHookEx(WH_CBT, SdlCbtFilterHook, NULL, ::GetCurrentThreadId());
#else
    // macOS: run fullscreen-desktop by default (EN_FULLSCREEN=0 to stay windowed).
    // The engine already renders at the desktop resolution (en_SetScreenMetrics
    // from linux_main), so fullscreen-desktop matches the back-buffer with no
    // scaling. Disable macOS fullscreen "Spaces" so the detached map/radar/panel
    // windows (ALWAYS_ON_TOP) still overlay the main window instead of being
    // hidden behind a separate fullscreen Space — essential for the multi-window
    // / multi-monitor model.
    const char* enFs = getenv("EN_FULLSCREEN");
    bool wantFullscreen = !(enFs && enFs[0] == '0');
    if (wantFullscreen)
        SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "0");
#endif

    // Create SDL window — borderless, positioned at the primary-display origin so
    // it fills the screen (and is a known anchor for the floating panels).
    Uint32 winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS;
    int winX = 0, winY = 0;
#ifndef _WIN32
#ifdef __APPLE__
    // #47 symptom-5 (opt-in, default OFF): EN_MAC_USABLE_FULLSCREEN makes the game a
    // borderless window sized+positioned to the display's USABLE bounds (which exclude
    // the Dock + menu bar) instead of a true FULLSCREEN_DESKTOP window. A borderless
    // fullscreen-desktop window with Spaces disabled covers the Dock's screen rect, but
    // the Dock (a higher-level system element) draws ON TOP of the game's bottom edge,
    // and the fullscreen style can swallow Cmd-Tab. A usable-bounds window leaves the
    // Dock/menu-bar untouched (no overlap), stays a normal window (Cmd-Tab works), and —
    // not being a fullscreen Space — still lets the ALWAYS_ON_TOP panels overlay. The
    // matching engine render size is set in linux_main.cpp under the SAME env flag, so
    // metrics/window/back-buffer stay consistent (no terrain-rasterizer mismatch).
    const char* usableFs = getenv("EN_MAC_USABLE_FULLSCREEN");
    const bool usableMode = wantFullscreen && usableFs && usableFs[0] && usableFs[0] != '0';
    if (usableMode) {
        SDL_Rect usable;
        if (SDL_GetDisplayUsableBounds(0, &usable) == 0 && usable.w > 0 && usable.h > 0) {
            winX = usable.x;
            winY = usable.y;
        }
    }
#else
    const bool usableMode = false;
#endif
    if (wantFullscreen && !usableMode) winFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
    m_window = SDL_CreateWindow(
        m_title.c_str(),
        winX, winY,
        m_width, m_height,
        winFlags
    );

#ifdef _WIN32
    // Remove our temporary hook — MFC hooks resume normal operation
    if (s_sdlCbtHook) {
        ::UnhookWindowsHookEx(s_sdlCbtHook);
        s_sdlCbtHook = NULL;
    }
#endif

    if (!m_window) {
        LogToFile(std::string("ERROR: SDL_CreateWindow failed: ") + SDL_GetError());
        SDL_Quit();
        return false;
    }

    LogToFile("SDL window created successfully");

    // Cross-platform: register hit-test callback so SDL handles resize drags
    // on all platforms.  The OS shows the resize cursor during the drag itself.
    SDL_SetWindowHitTest(m_window, BorderHitTest, nullptr);
    EnsureResizeCursors();

#ifdef _WIN32
    // Subclass main window for WM_SETCURSOR / WM_MOUSEACTIVATE handling.
    // Detached panel windows get subclassed automatically in CreateSDLWindow.
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(m_window, &wmInfo)) {
        HWND hSDL = wmInfo.info.win.window;
        if (::GetProp(hSDL, "EN_origWndProc") == NULL) {
            WNDPROC orig = (WNDPROC)::GetWindowLongPtr(hSDL, GWLP_WNDPROC);
            ::SetProp(hSDL, "EN_origWndProc", (HANDLE)orig);
            ::SetWindowLongPtr(hSDL, GWLP_WNDPROC, (LONG_PTR)SdlSubclassWndProc);
            s_sdlOrigWndProc = orig;
        }
    }
#endif

    // T0: GPU present path. Read once at init — must happen BEFORE anything calls
    // SDL_GetWindowSurface on m_window, since SDL forbids a renderer on a window
    // that already has a framebuffer surface.
    m_useRenderer = RenderBackendIsGpu();
    if (m_useRenderer) {
        // NOTE: no PRESENT_VSYNC yet. Today's loop presents from several sites
        // per frame (RenderingAdapter + Compositor), so a vblank-blocking present
        // on each call would cap the main-thread loop and throttle the sim —
        // violating the decoupled-loop rule. VSync gets enabled once present is
        // funneled to a single per-frame chokepoint (decoupled-loop follow-up).
        // For T0 this matches the non-VSync software present (parity, no throttle).
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
        if (!m_renderer) {
            // Fall back to software present rather than failing the whole window.
            LogToFile(std::string("WARN: SDL_CreateRenderer failed, falling back to "
                                  "software present: ") + SDL_GetError());
            m_useRenderer = false;
        } else {
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");  // linear when scaling
            EnsureBackBuffer();
            SDL_RendererInfo info;
            if (SDL_GetRendererInfo(m_renderer, &info) == 0)
                LogToFile(std::string("T0 renderer present path active: ") + info.name);
        }
    }

    return true;
}

bool GameWindow::EnsureBackBuffer() {
    if (!m_useRenderer || !m_renderer)
        return false;

    int w = 0, h = 0;
    SDL_GetRendererOutputSize(m_renderer, &w, &h);  // != window size on HiDPI
    if (w <= 0 || h <= 0)
        return false;
    if (m_backBuffer && m_backW == w && m_backH == h)
        return true;  // already the right size

    if (m_backTex) { SDL_DestroyTexture(m_backTex); m_backTex = nullptr; }
    if (m_backBuffer) { SDL_FreeSurface(m_backBuffer); m_backBuffer = nullptr; }

    // ARGB8888 matches the window's typical 32-bit BGRX framebuffer and the
    // CDIB 32-bit BGRX blit format used across the renderer (phase-6 finding).
    m_backBuffer = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    m_backTex = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!m_backBuffer || !m_backTex) {
        LogToFile(std::string("ERROR: back-buffer alloc failed: ") + SDL_GetError());
        return false;
    }
    m_backW = w; m_backH = h;
    m_backBufferFresh = true;   // texture is uninitialized → force a full upload next present
    return true;
}

SDL_Surface* GameWindow::GetPresentSurface() {
    if (m_useRenderer) {
        EnsureBackBuffer();
#ifndef _WIN32
        // Expose the CPU back-buffer to the harness so `shot` can dump the real
        // composited frame even when GPU read-back is blank / there is no display.
        EnHarness_SetMainSurface(m_backBuffer);
#endif
        return m_backBuffer;
    }
#ifndef _WIN32
    SDL_Surface* ws = m_window ? SDL_GetWindowSurface(m_window) : nullptr;
    EnHarness_SetMainSurface(ws);
    return ws;
#else
    return m_window ? SDL_GetWindowSurface(m_window) : nullptr;
#endif
}

void GameWindow::PresentSurface(const SDL_Rect* dirty) {
    if (!m_window)
        return;

    // A freshly (re)created back-buffer's texture holds garbage everywhere except a
    // dirty rect, so the first present after a (re)alloc must upload the whole surface.
    bool fullUpload = m_backBufferFresh || !dirty || dirty->w <= 0 || dirty->h <= 0;

    if (m_useRenderer) {
        if (!m_backBuffer || !m_backTex)
            return;
        if (fullUpload) {
            SDL_UpdateTexture(m_backTex, nullptr, m_backBuffer->pixels, m_backBuffer->pitch);
        } else {
            // Clamp the dirty rect to the surface and upload only that sub-rect. The
            // persistent texture keeps the rest; RenderCopy below still draws it all.
            SDL_Rect r = *dirty, full = { 0, 0, m_backBuffer->w, m_backBuffer->h };
            SDL_IntersectRect(&r, &full, &r);
            if (r.w > 0 && r.h > 0) {
                const Uint8* px = (const Uint8*)m_backBuffer->pixels
                                + (size_t)r.y * m_backBuffer->pitch
                                + (size_t)r.x * m_backBuffer->format->BytesPerPixel;
                SDL_UpdateTexture(m_backTex, &r, px, m_backBuffer->pitch);
            }
        }
        SDL_RenderClear(m_renderer);
        SDL_RenderCopy(m_renderer, m_backTex, nullptr, nullptr);
        FrameCap::Capture(m_renderer, "main");   // #45: no-op unless EN_FRAMECAP/toggle on
        SDL_RenderPresent(m_renderer);
    } else {
        if (fullUpload)
            SDL_UpdateWindowSurface(m_window);
        else
            SDL_UpdateWindowSurfaceRects(m_window, dirty, 1);
    }
    m_backBufferFresh = false;
}

void GameWindow::Cleanup() {
    LogToFile("Cleaning up GameWindow...");

    // Destroy compositor before window (it holds SDL surfaces)
    m_compositor.reset();

    m_buttonManager.reset();
    m_statusBar.reset();
    m_uiInputListener.reset();
    m_inputHandler.reset();

    if (m_backTex) { SDL_DestroyTexture(m_backTex); m_backTex = nullptr; }
    if (m_backBuffer) { SDL_FreeSurface(m_backBuffer); m_backBuffer = nullptr; }
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    // Note: Don't call SDL_Quit() here - SDL audio subsystem may still be in use
    // SDL_Quit() should only be called at final application shutdown
}

bool GameWindow::PollEvents() {
    // Guard against re-entrancy: DoModal's PeekMessage pump can trigger
    // BaseYield() → PollEvents() while a dialog event loop is already active.
    // If we drain SDL events here, the dialog never sees them and hangs.
    if (m_pollingEvents) {
        // Re-entrant call (e.g. SaveGame's BaseYield loop while the in-game menu's
        // DoModal is still on the stack). Don't drain events — the active modal
        // loop needs them — but DO render active non-modal dialogs so the
        // save-progress window (and any other modeless dialog) still paints.
        // Without this the save dialog never showed when saving from the menu.
        for (SDL2Dialog* dlg : m_activeDialogs)
            if (dlg->IsNonModalActive())
                dlg->RenderFrameNonModal();
        return false;
    }
    m_pollingEvents = true;

    // #47 / Cluster-A global capture safety net (mac2) — complements the per-drag-site
    // local heal in SDL2UI.cpp/SDL2Panel.cpp (@c49873f7). A missed SDL_MOUSEBUTTONUP can
    // leave an OS mouse-capture (SDL_CaptureMouse(TRUE), set by a title-bar/panel drag)
    // latched ON. If that happens with NO follow-up mouse motion — e.g. the user Cmd-Tabs
    // away the instant the drag ends — the motion-driven local heal never fires and the
    // stuck capture routes all input to the game (Cmd-Tab is swallowed; the macOS focus
    // bug, symptom #47-4). Frame-level invariant: a drag requires a button held, so if NO
    // mouse button is physically down there can be no live drag → the OS capture must not
    // outlive it. This releases ONLY the OS capture; it does NOT touch the per-widget drag
    // flags (m_dlgDragging/m_dResizing/m_dDragging) — those are owned by the drag-site
    // local heal, so the two patches stay disjoint (no double-patch; mac [00:42Z] split).
    // Portable by design: on Windows/Linux button-up is delivered reliably, so during a
    // real drag a button is down (net skips) and when idle nothing is captured (no-op) —
    // it also hardens those platforms against a focus-loss/alt-tab missed-up.
    {
        Uint32 mouseButtons = SDL_GetGlobalMouseState(nullptr, nullptr);
        if (!(mouseButtons & (SDL_BUTTON_LMASK | SDL_BUTTON_MMASK | SDL_BUTTON_RMASK)))
            SDL_CaptureMouse(SDL_FALSE);
    }

#ifndef _WIN32
    EnHarness_Service();   // service any pending harness screenshot on this (render) thread
#endif

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            LogToFile("SDL_QUIT received");
            m_pollingEvents = false;
            return true;
        }

        // GPU device-lost (monitor power-off/on, driver reset, fullscreen transitions):
        // render-TARGET textures lose their CONTENTS while remaining valid objects, so
        // the terrain's cached composite/water/fog/underlay textures go black while the
        // per-frame sprite layer keeps working (user-reported: "turned my screen off
        // then on, terrain is black, sprites still work"). Tell the terrain to rebuild
        // everything it caches.
        if (event.type == SDL_RENDER_TARGETS_RESET || event.type == SDL_RENDER_DEVICE_RESET) {
            LogToFile(event.type == SDL_RENDER_DEVICE_RESET ? "SDL_RENDER_DEVICE_RESET"
                                                            : "SDL_RENDER_TARGETS_RESET");
            SDL2Terrain::NotifyTargetsLost();
        }

        // The area map hides the OS cursor while placing a building/rocket (it
        // draws its own footprint). SDL_ShowCursor is application-global, so that
        // hide otherwise leaks into the toolbar / world map / dialogs. Re-show the
        // cursor whenever the pointer enters any non-area window; the area map
        // re-hides it on its own mouse-move while it still owns the pointer.
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_ENTER &&
            !IsAreaPanelWindow(event.window.windowID)) {
            SDL_ShowCursor(SDL_ENABLE);
        }

        // Reset resize cursor before routing — if a dialog/menu consumes the event,
        // HandleEvent() never runs and the cursor would stay stuck as a resize arrow.
        if (event.type == SDL_MOUSEMOTION) {
            SDL_Point pt = { event.motion.x, event.motion.y };
            SDL_HitTestResult ht = BorderHitTest(m_window, &pt, nullptr);
            if (ht == SDL_HITTEST_NORMAL)
                SDL_SetCursor(s_cursors[0]);
        }

        // Route to active menu first
        if (m_mainMenu && m_mainMenu->HandleEvent(event))
            continue;

        // Route to create-status dialog (cancel button)
        if (m_createStatus && m_createStatus->HandleEvent(event))
            continue;

        // Route to active non-modal dialogs by window ID
        bool consumedByDialog = false;
        for (SDL2Dialog* dlg : m_activeDialogs) {
            if (!dlg->IsNonModalActive()) continue;
            uint32_t dlgWinID = dlg->GetSDLWindowID();
            if (dlgWinID == 0) continue;
            // Check if event targets this dialog's window
            uint32_t evWinID = 0;
            switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:  evWinID = event.button.windowID;  break;
            case SDL_MOUSEMOTION:    evWinID = event.motion.windowID;  break;
            case SDL_MOUSEWHEEL:     evWinID = event.wheel.windowID;   break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:          evWinID = event.key.windowID;     break;
            // Text input/editing carry their own windowID. Without these the
            // edit boxes in non-modal in-game dialogs (e.g. Build Vehicle's
            // count field) never receive typed characters.
            case SDL_TEXTINPUT:      evWinID = event.text.windowID;    break;
            case SDL_TEXTEDITING:    evWinID = event.edit.windowID;    break;
            case SDL_WINDOWEVENT:    evWinID = event.window.windowID;  break;
            default:                 evWinID = 0;                      break;
            }
            if (evWinID == dlgWinID) {
                dlg->ProcessEventNonModal(event);
                consumedByDialog = true;
                break;
            }
        }
        if (consumedByDialog) continue;

        // Global keyboard shortcuts (Ctrl+letter, F1, Esc). Checked before the
        // compositor so the area map's bare-letter handlers don't shadow the
        // Ctrl-modified accelerators. Only consumes keys it actually maps.
        if (HandleGlobalShortcut(event))
            continue;

        // Route through compositor panels (top-down z-order)
        if (m_compositor && m_compositor->RouteEvent(event))
            continue;

        HandleEvent(event);
    }

    // App-level focus → pause/resume music. SDL_GetKeyboardFocus() is non-NULL
    // whenever ANY of our windows (main, detached maps, dialogs) holds focus, and
    // NULL only when another application is in front. Tracking that transition
    // (rather than per-window FOCUS_LOST/GAINED) means switching between our own
    // windows doesn't stop the music, but alt-tabbing to another app does — and
    // tabbing back resumes it. (The MFC WM_ACTIVATEAPP path is unreliable here
    // because the MFC main window is hidden.)
    {
        // Focus can drop to NULL for a frame or two while WE create and raise one
        // of our own borderless windows (a build dialog, a detached map): the old
        // window loses focus before the new one gains it. Reacting to that single
        // null reading would pause the music every time an in-game window opens
        // (e.g. double-clicking a crane to open its build dialog). So require the
        // focus loss to *persist* past a short grace period before treating it as a
        // real app-deactivation; resume immediately when focus returns.
        const Uint32 kFocusLossGraceMs = 400;
        bool hasFocus = (SDL_GetKeyboardFocus() != nullptr);
        Uint32 now = SDL_GetTicks();
        if (hasFocus) {
            m_focusLostAt = 0;
            if (!m_appActive) {
                m_appActive = true;
                theMusicPlayer.OnActivate(TRUE);
            }
        } else {
            if (m_focusLostAt == 0)
                m_focusLostAt = now;
            if (m_appActive && (now - m_focusLostAt) >= kFocusLossGraceMs) {
                m_appActive = false;
                theMusicPlayer.OnActivate(FALSE);
            }
        }
    }

    // Correlate frame cost with how many of our own extra windows are open. If the
    // SEC_RENDER spike (area map redraw) tracks ui.dialogs going 0→1, the overlapping
    // research/build window is the trigger; if it doesn't, the cost is elsewhere
    // (e.g. full-viewport redraws while scrolling). Near-zero cost when EN_PERF off.
    if (Perf::IsEnabled()) {
        int nDlg = 0;
        for (SDL2Dialog* dlg : m_activeDialogs)
            if (dlg->IsNonModalActive()) nDlg++;
        Perf::GaugeSet("ui.dialogs", nDlg);
    }

    // Render active non-modal dialogs
    for (SDL2Dialog* dlg : m_activeDialogs) {
        if (dlg->IsNonModalActive())
            dlg->RenderFrameNonModal();
    }

    // Clean up dialogs that have finished (EndDialog was called)
    for (auto it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ) {
        if (!(*it)->IsNonModalActive()) {
            delete *it;
            it = m_activeDialogs.erase(it);
        } else {
            ++it;
        }
    }

    // Render the create-status dialog if visible (takes over the full window)
    if (m_createStatus && m_createStatus->IsVisible()) {
        m_createStatus->Render();
    }
    // Otherwise render the active menu if present
    else if (m_mainMenu && m_mainMenu->IsInitialized()) {
        m_mainMenu->Render();
    }

    m_pollingEvents = false;
    return false;
}

void GameWindow::RegisterDialog(SDL2Dialog* dlg) {
    if (dlg)
        m_activeDialogs.push_back(dlg);
}

void GameWindow::UnregisterDialog(SDL2Dialog* dlg) {
    // No immediate action needed — cleanup pass in PollEvents deletes dead dialogs.
    // This is a no-op hook in case callers want to force early removal.
    (void)dlg;
}

void GameWindow::CloseActiveDialogs() {
    // EndDialog(0) tears down each dialog's OS window immediately and fires its
    // onDone (which clears the owner's pointer, e.g. CWndBar::m_pSdlRelations).
    // We then delete and clear right here rather than waiting for the PollEvents
    // cleanup pass, because the caller (DestroyWorld) is about to free the game
    // state these dialogs reference. Swap first so an onDone callback that somehow
    // re-enters can't mutate the vector we're iterating.
    std::vector<SDL2Dialog*> dialogs;
    dialogs.swap(m_activeDialogs);
    for (SDL2Dialog* dlg : dialogs) {
        if (dlg->IsNonModalActive())
            dlg->EndDialog(0);
        delete dlg;
    }
}

void GameWindow::HandleEvent(SDL_Event& event) {
    switch (event.type) {
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                LogToFile("Window close event — routing through OnCloseApp");
                // Route through the MFC main window's close handler, which calls
                // SaveGame() (now SDL2-based) for quit confirmation + save prompt.
#ifdef _WIN32
                ::PostMessage(theApp.m_wndMain.m_hWnd, WM_COMMAND, IDA_CLOSE_APP, 0);
#endif
            }
            else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                m_width  = event.window.data1;
                m_height = event.window.data2;
                LogToFile("Window resized to " + std::to_string(m_width) + "x" + std::to_string(m_height));
            }
            else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                // The detached map/radar windows are borderless + SKIP_TASKBAR, so
                // after Alt-Tabbing away they can get buried with no way back. When
                // any of our windows regains focus, raise the whole window group so
                // they come forward together. Throttled because SDL_RaiseWindow can
                // itself emit FOCUS_GAINED — without this it would raise-storm.
                static DWORD s_lastRaise = 0;
                DWORD now = timeGetTime();
                if (now - s_lastRaise > 400) {
                    s_lastRaise = now;
                    // id order ascends: main (lowest id) first, detached panels last
                    // → panels end up on top of the main game-view window.
                    for (Uint32 id = 1; id <= 64; ++id) {
                        SDL_Window* w = SDL_GetWindowFromID(id);
                        if (w && (SDL_GetWindowFlags(w) & SDL_WINDOW_SHOWN))
                            SDL_RaiseWindow(w);
                    }
                }
            }
            break;

        case SDL_MOUSEMOTION: {
            // Show resize cursor when hovering over the window border.
            // Only override the cursor for resize edges; leave it alone
            // when inside the client area so the game's own cursors work.
            SDL_Point pt = { event.motion.x, event.motion.y };
            SDL_HitTestResult ht = BorderHitTest(m_window, &pt, nullptr);
            switch (ht) {
                case SDL_HITTEST_RESIZE_LEFT:
                case SDL_HITTEST_RESIZE_RIGHT:
                    SDL_SetCursor(s_cursors[1]);  // sizeWE
                    break;
                case SDL_HITTEST_RESIZE_TOP:
                case SDL_HITTEST_RESIZE_BOTTOM:
                    SDL_SetCursor(s_cursors[2]);  // sizeNS
                    break;
                case SDL_HITTEST_RESIZE_TOPLEFT:
                case SDL_HITTEST_RESIZE_BOTTOMRIGHT:
                    SDL_SetCursor(s_cursors[3]);  // NW-SE diagonal
                    break;
                case SDL_HITTEST_RESIZE_TOPRIGHT:
                case SDL_HITTEST_RESIZE_BOTTOMLEFT:
                    SDL_SetCursor(s_cursors[4]);  // NE-SW diagonal
                    break;
                default:
                    // Inside client area — restore default and let the game
                    // manage the cursor via its own SetMouseState() path.
                    SDL_SetCursor(s_cursors[0]);
                    break;
            }
            break;
        }

        default:
            break;
    }
}

bool GameWindow::HandleGlobalShortcut(SDL_Event& event) {
#ifdef _WIN32
    if (event.type != SDL_KEYDOWN)
        return false;

    // Only while actually in a game (toolbar exists). At the main menu the menu
    // handles its own keys.
    if (!theApp.m_wndBar.IsCreated())
        return false;

    // The window-switch commands call the SAME public CWndBar handlers the toolbar
    // buttons use (CWndMain::On* just forward to these). The old path posted
    // WM_COMMAND to m_wndMain, but that MFC window is stubbed in the SDL port so the
    // messages were never dispatched — call the handlers directly instead.
    CWndBar&    bar  = theApp.m_wndBar;
    HWND        hMain = theApp.m_wndMain.m_hWnd;   // may be NULL (stub) in the SDL port
    SDL_Keycode key  = event.key.keysym.sym;
    SDL_Keymod  mod  = (SDL_Keymod)event.key.keysym.mod;
    bool        ctrl = (mod & KMOD_CTRL) != 0;

    // Esc: back out of the current action if there is one, otherwise open the
    // file/options dialog. Falling through (return false) when an area has a
    // selection / non-normal mode preserves the original Esc = deselect.
    if (key == SDLK_ESCAPE && !ctrl) {
        CWndArea* pArea = theAreaList.GetTop();
        if (pArea && (pArea->GetMode() != CWndArea::normal || pArea->NumSelected() > 0))
            return false;  // area handler will deselect / cancel the mode
        bar.GotoFile();
        return true;
    }

    // F1 → Help. The original opened the .hlp file (gone); send players to the
    // revival project's issue tracker instead.
    if (key == SDLK_F1) {
        SDL_OpenURL("https://github.com/EnemyV/EnemyNationsRevival/issues");
        return true;
    }
    if (key == SDLK_F2) {
        if (hMain) ::PostMessage(hMain, WM_COMMAND, (WPARAM)IDA_BOSS, 0);
        return true;
    }

    // Ctrl+letter accelerators (the IDR_ACCEL table).
    if (ctrl) {
        switch (key) {
        case SDLK_a: bar.GotoArea();              return true;
        case SDLK_w: bar.GotoWorld();             return true;
        case SDLK_v: bar.GotoVehicles();          return true;
        case SDLK_b: bar.GotoBuildings();         return true;
        case SDLK_m: bar.GotoChat();              return true;  // IDA_MAIL -> chat
        case SDLK_o: bar.GotoFile();              return true;  // IDA_OPTIONS -> file/options
        case SDLK_r: bar.GotoScience();           return true;
        case SDLK_d: bar.GotoRelations();         return true;
        case SDLK_h: bar.ShowWindow(SW_HIDE);     return true;  // hide toolbar
        case SDLK_u: bar.ShowWindow(SW_SHOW);     return true;  // unhide toolbar
        // Pause/save keep the MFC command route (server-side pause logic / SaveGame);
        // no-op if the stubbed main window has no HWND.
        case SDLK_p: if (hMain) ::PostMessage(hMain, WM_COMMAND, (WPARAM)IDA_PAUSE, 0); return true;
        case SDLK_s: if (hMain) ::PostMessage(hMain, WM_COMMAND, (WPARAM)IDA_SAVE, 0);  return true;
        default: break;
        }
    }
#else
    (void)event;
#endif
    return false;
}

void GameWindow::Raise() {
    if (m_window) {
        SDL_ShowWindow(m_window);
        SDL_RaiseWindow(m_window);
    }
    // Re-raise all detached panels (Area View, World View) so they remain
    // visible above the background window after it is raised.
    if (m_compositor) {
        for (int i = 0; i < m_compositor->GetPanelCount(); i++) {
            SDL2Panel* panel = m_compositor->GetPanel(i);
            if (panel && panel->IsDetached())
                SDL_RaiseWindow(panel->GetOwnWindow());
        }
    }
}

void GameWindow::Hide() {
    if (m_window) {
        SDL_HideWindow(m_window);
    }
}

void GameWindow::Show() {
    if (m_window) {
        SDL_ShowWindow(m_window);
    }
}

void GameWindow::RequestQuit() {
    // See header: Win32 PostQuitMessage's WM_QUIT is eaten by SDL's pump, so signal
    // shutdown through the SDL event queue that the main loop actually drains.
    SDL_Event q;
    SDL_zero(q);
    q.type = SDL_QUIT;
    SDL_PushEvent(&q);
}

void GameWindow::SwapBuffers() {
    if (m_window) {
        PresentSurface();
    }
}

void GameWindow::ClearScreen(float r, float g, float b, float a) {
    if (!m_window) return;
    SDL_Surface* surface = GetPresentSurface();
    if (!surface) return;
    Uint8 red = (Uint8)(r * 255.0f);
    Uint8 green = (Uint8)(g * 255.0f);
    Uint8 blue = (Uint8)(b * 255.0f);
    Uint32 color = SDL_MapRGB(surface->format, red, green, blue);
    SDL_FillRect(surface, nullptr, color);
    PresentSurface();
}

void GameWindow::UpdateWindowTitle() {
    if (!m_window) return;

    // Build title string: "Enemy Nations - {PlayerName} ({Faction}) - {Mode}"
    std::string fullTitle = "Enemy Nations";

    if (!m_playerName.empty() || !m_factionName.empty() || !m_gameMode.empty()) {
        fullTitle += " - ";

        if (!m_playerName.empty()) {
            fullTitle += m_playerName;
        }

        if (!m_factionName.empty()) {
            if (!m_playerName.empty()) {
                fullTitle += " ";
            }
            fullTitle += "(" + m_factionName + ")";
        }

        if (!m_gameMode.empty()) {
            fullTitle += " - " + m_gameMode;
        }
    }

    LogToFile("Updating window title to: " + fullTitle);
    SDL_SetWindowTitle(m_window, fullTitle.c_str());
}

void GameWindow::SetGameInfo(const std::string& playerName, const std::string& factionName, const std::string& gameMode) {
    m_playerName = playerName;
    m_factionName = factionName;
    m_gameMode = gameMode;
    UpdateWindowTitle();
}

void GameWindow::SetPlayerName(const std::string& playerName) {
    m_playerName = playerName;
    UpdateWindowTitle();
}

void GameWindow::SetFactionName(const std::string& factionName) {
    m_factionName = factionName;
    UpdateWindowTitle();
}

void GameWindow::SetGameMode(const std::string& gameMode) {
    m_gameMode = gameMode;
    UpdateWindowTitle();
}

bool GameWindow::InitializeUI() {
    // UI component initialization disabled - SDL_Renderer conflicts with
    // SDL_GetWindowSurface used by RenderingAdapter and SDL2MainMenu.
    // The old SDLButtonManager/StatusBar system will be replaced by
    // surface-based rendering in future phases.
    LogToFile("InitializeUI - skipped (using surface-based rendering)");
    return false;
}

void GameWindow::ConfigureGameAreaButtons() {
    if (!m_buttonManager) {
        return;
    }

    // Button indices from original game (enations_latest/src/area.cpp)
    const int NUM_AREA_BUTTONS = 17;
    const int buttonIndices[] = { 54, 37, 36, 25, 26, 14, 39, 34, 4, 22, 47, 40, 35, 5, 23, 32, 45 };

    // Button command IDs from original game
    const int buttonIDs[] = {
        1001, 1002, 1003, 1004, 1005, 1006, 1007,
        1008, 1009, 1010, 1011, 1012, 1013, 1014,
        1015, 1016
    };

    const int startX = 500;
    const int startY = 20;
    const int spacingX = 45;
    const int spacingY = 45;

    for (int i = 0; i < NUM_AREA_BUTTONS; ++i) {
        int col = i % 4;
        int row = i / 4;
        int x = startX + col * spacingX;
        int y = startY + row * spacingY;

        m_buttonManager->AddButton(x, y, i, buttonIndices[i],
                                   "Btn", buttonIDs[i]);
    }

    LogToFile("Game area buttons configured");
}

void GameWindow::UpdateStatusBar() {
    if (!m_statusBar) {
        return;
    }

    // Update with placeholder values (would get from game state in full integration)
    m_statusBar->SetLumber(500);
    m_statusBar->SetSteel(200);
    m_statusBar->SetOil(150);
    m_statusBar->SetPower(100);
    m_statusBar->SetFood(300);
    m_statusBar->SetUnitCount(12);
    m_statusBar->SetHealth(95);
    m_statusBar->SetGameTime(0, 0);
}

void GameWindow::Render() {
    // TODO: SDL rendering conflicts with game's DirectDraw rendering
    // The game uses MFC/DirectDraw (CDC, BitBlt) which conflicts with SDL
    // Rendering is disabled until we can properly integrate with the existing UI system
    // For now, input is working (button clicks are registered)
}
