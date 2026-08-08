#include "stdafx.h"
#include "en_logpath.h"   // EnLogPath - logs to the launch dir, not the exe dir
#include "GameWindow.h"
#include "framecap.h"   // #45 frame-capture debug mode
#include "en_harness.h"   // EnHarness_Service() — services harness requests on the render thread
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
#include "SDL2Sprites.h"  // NotifyTargetsLost — sprite atlas/RT recovery after device-lost
#include "RenderBackend.h" // RenderBackendIsGpu() — 3-way backend selector
#include "../rendering/SDLButtonManager.h"
#include "../rendering/StatusBar.h"
#include "../input/UIButtonListener.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>         // lround — trackpad pan delta rounding
#ifndef _WIN32
#include "appicon_data.h"  // embedded 32x32 RGBA taskbar/dock icon
#endif
#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM
#endif

// SDL2 headers
#include <SDL.h>
// Windows needs it for the HWND subclassing; macOS for the NSWindow handle used to
// turn off the child-window drop shadows. Deliberately NOT unconditional: on Linux
// this header drags in the X11 headers, whose macros collide with game symbols.
#if defined(_WIN32) || defined(__APPLE__)
#include <SDL_syswm.h>
#endif

// Helper function for logging - write to current directory
static void LogToFile(const std::string& message) {
    std::ofstream log(EnLogPath("GameWindow_Debug.log").c_str(), std::ios::app);
    if (log.is_open()) {
        log << message << std::endl;
        log.close();
    }
}

// EnResolveFontPath() now lives in wind22 (windward/wind22/src/mfc_compat_text.cpp):
// the compat text renderer needs it too, and keeping a second copy up here is what
// left PickFontPath() Debian-only after the first T-0073 fix. Declared in GameWindow.h.


// Taskbar/alt-tab/dock icon (_NET_WM_ICON on X11): without it a minimized game
// window shows the generic SDL icon on Linux (operator-reported). Embedded RGBA
// pixels — no image-loading dependency; Windows keeps its .rc resource icon.
void GameWindow::ApplyAppIcon(SDL_Window* win) {
#ifndef _WIN32
    if (!win)
        return;
    static SDL_Surface* s_icon = nullptr;
    if (!s_icon)
        s_icon = SDL_CreateRGBSurfaceWithFormatFrom(
            (void*)kAppIconRGBA, kAppIconW, kAppIconH, 32, kAppIconW * 4,
            SDL_PIXELFORMAT_RGBA32);
    if (s_icon)
        SDL_SetWindowIcon(win, s_icon);
#else
    (void)win;
#endif
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
    // Display-only overlays (the Shift+RMB unit-info panel) tag themselves click-through:
    // they open UNDER the cursor, so without this they swallow the player's next map click
    // whole — the global dismiss hook closed the panel and the intended action never fired.
    if (msg == WM_NCHITTEST && ::GetProp(hWnd, "EN_clickThrough"))
        return HTTRANSPARENT;
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

#ifdef __APPLE__
// Defined below, next to the other hand-rolled objc helpers (the objc headers' BOOL
// collides with the win32 shim, so this file talks to the runtime by hand).
static void DisableMacWindowShadow(SDL_Window* win);
#endif

SDL_Window* GameWindow::CreateSDLWindow(const char* title, int x, int y, int w, int h, Uint32 flags) {
#ifdef _WIN32
    // Protect against MFC's CBT hook corrupting the new SDL window
    HHOOK hook = ::SetWindowsHookEx(WH_CBT, SdlCbtFilterHook, NULL, ::GetCurrentThreadId());
#endif
    SDL_Window* win = SDL_CreateWindow(title, x, y, w, h, flags);
#if defined(__linux__)
    // Every game window is opened by a user action; stamp _NET_WM_USER_TIME so
    // Mutter focuses it instead of toasting "<title> is ready" (BUGS #33).
    // Windows whose X11 window gets RECREATED later (GPU panels) re-stamp after.
    extern void EnSetX11UserTimeNow(SDL_Window* win);
    if (win)
        EnSetX11UserTimeNow(win);
#endif
#ifdef __APPLE__
    // Every child window here (area map, radar, unit/building lists, dialogs, the
    // Shift+RMB tooltip) is BORDERLESS, and macOS gives a borderless NSWindow a full
    // system drop shadow — measured hasShadow=YES on all of them. On Windows these were
    // child windows inside ONE frame and cast no shadows at all, so on mac the in-game
    // UI ends up ringed with heavy shadows that were never part of the game's look
    // (operator: "the shadows of the windows was too strong"). Drop them for our own
    // child windows; the main window is created directly in Create() and is unaffected.
    // EN_MAC_WINDOW_SHADOWS=1 restores them.
    if (win)
        DisableMacWindowShadow(win);
#endif
    ApplyAppIcon(win);
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

#ifdef __APPLE__
// Set the Dock icon at runtime: the game ships as a bare Mach-O binary (no
// .app bundle), so macOS shows the generic executable icon when running or
// minimized (operator-reported). Loads the classic 32x32 game icon staged at
// assets/appicon.png (converted from res/main.ico). objc runtime declared by
// hand — the objc headers' BOOL collides with the Win32 shim's — and AppKit
// is already loaded via SDL.
extern "C" {
    void* objc_getClass(const char* name);
    void* sel_registerName(const char* name);
    void  objc_msgSend(void);
}
// Turn off the macOS system drop shadow on one of our borderless child windows.
// Shadows are composited by the window server OUTSIDE the window surface, so no
// harness capture can show them — this was found by querying hasShadow directly
// (measured YES for BORDERLESS, BORDERLESS|ALWAYS_ON_TOP and the tooltip's exact
// flag set on SDL 2.32.10). Opt back in with EN_MAC_WINDOW_SHADOWS=1.
static void DisableMacWindowShadow(SDL_Window* win) {
    const char* keep = getenv("EN_MAC_WINDOW_SHADOWS");
    if (keep && keep[0] == '1') return;
    SDL_SysWMinfo wm; SDL_VERSION(&wm.version);
    if (!SDL_GetWindowWMInfo(win, &wm)) return;
    void* nswin = wm.info.cocoa.window;
    if (!nswin) return;
    // Exact-prototype cast — objc_msgSend must NOT be called through a variadic
    // signature on arm64 (different argument-passing convention). Same discipline
    // as SetMacDockIcon / HideMacDockBar below.
    auto sendB = (void (*)(void*, void*, bool))objc_msgSend;
    sendB(nswin, sel_registerName("setHasShadow:"), false);
}

static void SetMacDockIcon() {
    // Resolve like PlayVideo: CWD first, then the exe's directory.
    std::string path = "assets/appicon.png";
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string dir(exePath);
        size_t slash = dir.find_last_of("\\/");
        if (slash == std::string::npos) return;
        path = dir.substr(0, slash + 1) + "assets/appicon.png";
        f = fopen(path.c_str(), "rb");
        if (!f) return;
    }
    fclose(f);

    // Exact-prototype casts — objc_msgSend must NOT be called through a
    // variadic signature on arm64 (different argument-passing convention).
    auto cls   = [](const char* n) { return objc_getClass(n); };
    auto sel   = [](const char* n) { return sel_registerName(n); };
    auto send0 = (void* (*)(void*, void*))objc_msgSend;
    auto send1 = (void* (*)(void*, void*, const void*))objc_msgSend;

    void* nsPath = send1(cls("NSString"), sel("stringWithUTF8String:"), path.c_str());
    if (!nsPath) return;
    void* img = send1(send0(cls("NSImage"), sel("alloc")), sel("initWithContentsOfFile:"), nsPath);
    if (!img) return;
    void* app = send0(cls("NSApplication"), sel("sharedApplication"));
    send1(app, sel("setApplicationIconImage:"), img);
}

// Hide the macOS Dock + menu bar while the game is frontmost. The game runs as a
// borderless fullscreen-desktop window with Spaces DISABLED (so the detached
// map/radar panels can overlay it) — but a non-Space fullscreen window does NOT
// auto-hide the Dock, so the always-on Dock (and menu bar) draw over the game's
// edges (operator release blocker: "the launcher bar draws on top of the game").
// NSApplicationPresentationAutoHideDock | AutoHideMenuBar hides both WITHOUT a
// fullscreen Space, so the multi-window panels keep working. Re-applied on focus
// gain (macOS clears presentation options when the app resigns active).
static void HideMacDockBar() {
    auto cls    = [](const char* n) { return objc_getClass(n); };
    auto sel    = [](const char* n) { return sel_registerName(n); };
    auto send0  = (void* (*)(void*, void*))objc_msgSend;
    auto sendUL = (void  (*)(void*, void*, unsigned long))objc_msgSend;
    auto sendL  = (void  (*)(void*, void*, long))objc_msgSend;
    auto sendB  = (void  (*)(void*, void*, bool))objc_msgSend;
    void* app = send0(cls("NSApplication"), sel("sharedApplication"));
    if (!app) return;
    // NSApplicationPresentation* options only take effect while the app is the ACTIVE,
    // regular-policy application. A binary launched from Terminal (not a .app bundle)
    // can come up as an accessory/background process on some machines (operator's
    // MacBook host: menu bar never auto-hid, so the full-desktop window's bottom UI row
    // fell off-screen) — there setPresentationOptions: silently no-ops. Force a regular,
    // frontmost app FIRST so the menu-bar/Dock auto-hide actually applies. Idempotent, so
    // the focus-gain re-apply path is safe too. (Works already on the VM where the app
    // came up active; this makes it deterministic everywhere.)
    sendL(app, sel("setActivationPolicy:"), 0L);                 // NSApplicationActivationPolicyRegular
    sendB(app, sel("activateIgnoringOtherApps:"), true);         // become frontmost
    // HideDock (1<<1) | HideMenuBar (1<<3) — NOT the AutoHide variants.
    //
    // This used to be AutoHideDock|AutoHideMenuBar, and auto-hide is exactly wrong here:
    // an auto-hidden Dock REAPPEARS when the pointer reaches the screen edge, and the
    // game's bottom button bar occupies that very edge. So moving the mouse toward the
    // bar summoned the Dock on top of it — the operator's "sometimes the bottom bar when
    // in game isn't visible", which is intermittent precisely because it depends on where
    // the pointer is. Hiding them outright removes the reveal-on-hover behaviour, so the
    // bar can never be covered and the game gets the full screen.
    //
    // AppKit requires HideMenuBar to be accompanied by HideDock; both are set here.
    // Deliberately still NOT a fullscreen Space (SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES=0
    // at the call site), so the detached ALWAYS_ON_TOP panels still overlay and other
    // displays stay usable — hiding the bars must not cost multi-monitor support.
    const unsigned long opts = (1UL << 1) | (1UL << 3);
    sendUL(app, sel("setPresentationOptions:"), opts);
}
#endif

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
#ifdef __APPLE__
    SetMacDockIcon();
#endif
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
    // BACK TO DEFAULT OFF (mac, 2026-08-08). I briefly made this the default to stop the
    // Dock covering the bottom bar; that was the wrong trade. Sizing to the usable bounds
    // means the menu bar and Dock KEEP their strips, so the game silently loses ~106px of
    // screen and stops being fullscreen — operator: "the bar at the bottom is still
    // visible... the bar at the top is still visible... they are taking up space". The
    // right fix was in HideMacDockBar (hide the bars outright instead of AUTO-hiding them,
    // which let them reappear on hover over the game's own bottom edge). This flag stays
    // available as an opt-in for anyone who wants the bars left alone.
    // Must stay in lock-step with linux_main.cpp, which seeds the engine metrics from the
    // SAME flag: if the two disagree the layout is computed against a size the window does
    // not have, which is precisely the T-0072 failure (bar laid out below the window).
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

#ifndef _WIN32
    // T-0072: tell the win32 shim the size the WM actually GRANTED.
    //
    // The shim's window record is sized from what CreateWindowEx was ASKED for
    // (win32_compat.cpp:1137-1142), and the main window is requested at the engine
    // metric size (lastplnt.cpp:1461, from the desktop). When the WM grants less --
    // measured on a mac host: requested 1024x1200, granted 1024x768 -- the record
    // keeps the unclamped value, and GetClientRect() reports it. CWndBar::Create()
    // positions the bar from exactly that (toolbar.cpp:163-165,
    // m_iRow3 = rect.Height() - TOOLBAR_HT), so the bar is laid out below the visible
    // window and vanishes. Correcting the g_enScreenW/H metrics instead does NOT work:
    // GetClientRect only falls back to them when no window record exists.
    // The shim's SetWindowPos updates cx/cy and fires WM_SIZE, so no new mechanism.
    {
        int aw = 0, ah = 0;
        SDL_GetWindowSize( m_window, &aw, &ah );
        if ( aw > 0 && ah > 0 && theApp.m_wndMain.m_hWnd )
            ::SetWindowPos( theApp.m_wndMain.m_hWnd, NULL, 0, 0, aw, ah,
                            SWP_NOMOVE | SWP_NOZORDER );
    }
#endif

#ifdef __APPLE__
    m_macUsableFullscreen = usableMode;
    // Hide the Dock + menu bar now that the game window exists (fullscreen game;
    // the non-Space fullscreen-desktop window won't auto-hide them on its own).
    // NOT in usable-bounds mode: there the window is deliberately sized to exclude the
    // Dock/menu-bar strips, so auto-hiding them would just leave dead space where the
    // game does not draw. Leaving them alone is the whole point of that mode.
    if (wantFullscreen && !usableMode)
        HideMacDockBar();
#endif

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
        // Renderer flag: ACCELERATED (GL) by DEFAULT — this is the proper SDL2 render
        // path with full blending / fog / move-preview-overlay support, matching the
        // Windows and macOS builds. EN_RENDER_SW opts into SDL's SOFTWARE renderer:
        // it's faster on a box with NO hardware GL (a VM where GL is CPU-emulated via
        // Mesa llvmpipe / VMware SVGA3D-LLVM), BUT its render-target compositing does
        // NOT reproduce the GL backend's blending faithfully — fog, alpha blends, and
        // the Shift move-preview overlay render wrong/missing — so it is OPT-IN, never
        // the default. (The real fix for software-GL lag is the VM's 3D acceleration.)
        Uint32 rflags = getenv("EN_RENDER_SW") ? SDL_RENDERER_SOFTWARE : SDL_RENDERER_ACCELERATED;
        m_renderer = SDL_CreateRenderer(m_window, -1, rflags);
        if (!m_renderer) {
            // Fall back to software present rather than failing the whole window.
            LogToFile(std::string("WARN: SDL_CreateRenderer failed, falling back to "
                                  "software present: ") + SDL_GetError());
            m_useRenderer = false;
        } else {
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");  // linear when scaling
#if defined(__linux__)
            // Creating the renderer can RECREATE the X11 window (GL visual),
            // wiping the _NET_WM_USER_TIME stamped in CreateSDLWindow — without
            // it a later Raise() gets denied by Mutter and toasts "window is
            // ready" (BUGS #33). Re-stamp the (possibly new) X11 window.
            extern void EnSetX11UserTimeNow(SDL_Window* win);
            EnSetX11UserTimeNow(m_window);
#endif
            ApplyAppIcon(m_window);   // recreation also wiped _NET_WM_ICON
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
        // Expose the CPU back-buffer to the harness so `shot` can dump the real
        // composited frame even when GPU read-back is blank / there is no display.
        // Early-outs to a single atomic load when EN_HARNESS is not set.
        EnHarness_SetMainSurface(m_backBuffer);
        return m_backBuffer;
    }
    SDL_Surface* ws = m_window ? SDL_GetWindowSurface(m_window) : nullptr;
    EnHarness_SetMainSurface(ws);
    return ws;
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

#if defined(__APPLE__)
// macOS trackpad multi-finger drag -> pan the area map, mirroring the Windows
// middle-mouse "grab" pan. Accepts a 2- OR 3-finger drag (numFingers >= 2): SDL
// synthesizes SDL_MULTIGESTURE from the trackpad, where mgesture.x/y is the
// normalized (0..1) finger centroid and numFingers the count. We track the
// centroid frame-to-frame and scroll the map by the delta. A finger up/down
// re-baselines the next sample so adding/lifting a finger (2<->3) never jumps the
// view. Gesture events carry no windowID, so the compositor's per-window routing
// drops them -> we handle them here, ahead of that routing. macOS-only;
// Windows/Linux are untouched (keeps the cross-platform golden rule).
// NOTE: macOS may reserve 3-finger swipes for Mission Control / app-spaces at the
// WindowServer level; if so those never reach SDL. 2-finger is always delivered.
static bool HandleTrackpadPan(SDL_Event& event, SDL_Window* win)
{
    static bool  s_active = false;
    static float s_lastX  = 0.0f, s_lastY = 0.0f;
    static int   s_logged = 0;

    // A change in finger count invalidates the running centroid baseline.
    if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP) {
        s_active = false;
        return false;   // don't consume — no other touch handling, but stay neutral
    }

    if (event.type != SDL_MULTIGESTURE)
        return false;

    if (s_logged < 8) {   // one-time visibility that gestures actually arrive in this build/VM
        LogToFile("trackpad MULTIGESTURE fingers=" + std::to_string(event.mgesture.numFingers) +
                  " x=" + std::to_string(event.mgesture.x) +
                  " y=" + std::to_string(event.mgesture.y));
        ++s_logged;
    }

    if (event.mgesture.numFingers < 2)
        return false;

    float cx = event.mgesture.x, cy = event.mgesture.y;
    if (!s_active) {          // first sample of a gesture only sets the baseline
        s_active = true;
        s_lastX  = cx;
        s_lastY  = cy;
        return true;
    }

    float ndx = cx - s_lastX;
    float ndy = cy - s_lastY;
    s_lastX = cx;
    s_lastY = cy;

    CWndArea* pTop = theAreaList.GetTop();
    if (!pTop)
        return false;        // not in-game / no area map to pan

    int w = 1024, h = 768;
    if (win)
        SDL_GetWindowSize(win, &w, &h);

    // Normalized centroid delta -> pixels. kGain tunes pan speed; a comfortable
    // trackpad swipe should move the map a proportional distance.
    const float kGain = 2.0f;
    int dxPix = (int)lround((double)ndx * w * kGain);
    int dyPix = (int)lround((double)ndy * h * kGain);

    // Grab-style, like the middle-mouse pan: the map follows the fingers, so the
    // view center moves opposite to the finger motion.
    pTop->PanByPixels(-dxPix, -dyPix);
    return true;
}
#endif  // __APPLE__

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

    EnHarness_Service();   // service any pending harness request on this (render) thread

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
            SDL2Sprites::NotifyTargetsLost();
        }

        // Any mouse click dismisses the Shift+RMB unit-info tooltip, in ANY window.
        // This runs before routing, so the open gesture (Shift+RMB on a unit, handled
        // in the area's OnRButtonDown below) re-shows it — every other click just closes it.
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            CWndArea* pArea = theAreaList.GetTop();
            if (pArea) pArea->HideUnitInfo();
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
                bool dlgConsumed = dlg->ProcessEventNonModal(event);
                // ARROW keys the dialog did NOT consume (no focused widget wanted
                // them) flow on to the game-hotkey fallback → topmost area map, so
                // the map keeps arrow-panning while an info/build/research window
                // has keyboard focus (matches the original's app-wide accelerator;
                // arrow-pan fix, 2026-08-07). ARROWS ONLY: a focused edit box
                // consumes arrows (caret) so dialog text entry keeps priority, but
                // it deliberately does NOT consume letter KEYDOWNs (text arrives
                // via SDL_TEXTINPUT) — forwarding those would fire bare-letter map
                // hotkeys while typing. Still break: only this dialog gets a shot.
                bool arrowKey = ( event.type == SDL_KEYDOWN || event.type == SDL_KEYUP ) &&
                                ( event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT ||
                                  event.key.keysym.sym == SDLK_UP   || event.key.keysym.sym == SDLK_DOWN );
                consumedByDialog = dlgConsumed || !arrowKey;
                break;
            }
        }
        if (consumedByDialog) continue;

        // Global keyboard shortcuts (Ctrl+letter, F1, Esc). Checked before the
        // compositor so the area map's bare-letter handlers don't shadow the
        // Ctrl-modified accelerators. Only consumes keys it actually maps.
        if (HandleGlobalShortcut(event))
            continue;

#if defined(__APPLE__)
        // macOS trackpad two-finger drag pans the area map (Windows middle-mouse
        // "grab" equivalent). Handled before compositor routing because gesture
        // events carry no windowID for the compositor to route on.
        if (HandleTrackpadPan(event, m_window))
            continue;
#endif

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

void GameWindow::MinimizeAll() {
    // In-game the visible screen is mostly the detached ALWAYS_ON_TOP panel
    // windows (Area Map, Radar, unit lists), not the main window — minimizing
    // only the main window leaves them all up, which made the options-dialog
    // Minimize button look dead (operator-reported on mac). Hide the panel
    // windows first (their logical IsVisible() state is untouched), then
    // minimize the main window. Restoring is free: un-minimizing fires
    // FOCUS_GAINED, whose group-restore path below re-shows and raises every
    // logically open panel (the same recovery used for alt-tab).
    int hidden = 0;
    if (m_compositor) {
        for (int i = 0; i < m_compositor->GetPanelCount(); ++i) {
            SDL2Panel* panel = m_compositor->GetPanel(i);
            if (panel && panel->IsDetached() && panel->GetOwnWindow()) {
                SDL_HideWindow(panel->GetOwnWindow());
                ++hidden;
            }
        }
    }
    // Stand the FOCUS_GAINED group-restore down long enough for the options
    // dialog's teardown focus shuffle to pass (it would de-miniaturize us).
    m_suppressGroupRestoreMs = timeGetTime() + 2000;
    if (m_window)
        SDL_MinimizeWindow(m_window);
    LogToFile("MinimizeAll: hid " + std::to_string(hidden) + " panels");
}

void GameWindow::HandleEvent(SDL_Event& event) {
    switch (event.type) {
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                LogToFile("Window close event — routing through OnCloseApp");
                // Direct, portable call: SaveGame()-backed quit confirm + save prompt.
                // The old PostMessage(WM_COMMAND) route died with the MFC message map.
                theApp.m_wndMain.OnCloseApp();
            }
            else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                // ONLY the main game window's resize updates the recorded client size.
                // Child windows (dialogs, the unit-info tooltip, detached panels) each
                // fire SIZE_CHANGED too; without this filter a small tooltip (240xN)
                // would clobber m_width/m_height and every dialog would then center
                // off-screen at the top-left. The T-0072 shim push lives INSIDE the
                // same filter for the same reason: pushing a tooltip's size into the
                // main window's shim record is exactly the divergence T-0072 fixes.
                if (m_window && event.window.windowID == SDL_GetWindowID(m_window)) {
                    m_width  = event.window.data1;
                    m_height = event.window.data2;
                    LogToFile("Window resized to " + std::to_string(m_width) + "x" + std::to_string(m_height));
#ifndef _WIN32
                    // T-0072: keep the shim's record in step with the real window, or a
                    // later resize re-introduces the same stale-size divergence that hides
                    // the bottom bar. Sufficient on its own here (no re-layout needed)
                    // because CWndBar::Create() reads the record at GAME START, i.e. after
                    // the window exists; the shim's SetWindowPos also fires WM_SIZE.
                    if ( m_width > 0 && m_height > 0 && theApp.m_wndMain.m_hWnd )
                        ::SetWindowPos( theApp.m_wndMain.m_hWnd, NULL, 0, 0, m_width, m_height,
                                        SWP_NOMOVE | SWP_NOZORDER );
#endif
                }
            }
            else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
#ifdef __APPLE__
                // Re-hide the Dock/menu bar: macOS clears an app's presentation
                // options when it resigns active (Cmd-Tab away), so they must be
                // re-applied each time the game regains focus, or the launcher bar
                // reappears over the game after an alt-tab.
                {
                    const char* enFs = getenv("EN_FULLSCREEN");
                    // Same exclusion as at creation: in usable-bounds mode we never
                    // covered the Dock, so we must not hide it on focus either.
                    if (!(enFs && enFs[0] == '0') && !m_macUsableFullscreen)
                        HideMacDockBar();
                }
#endif
                // The detached map/radar/unit windows are borderless + SKIP_TASKBAR
                // and (Linux) WM_TRANSIENT_FOR-owned by the main window, so Alt-Tabbing
                // away can leave them buried OR — on WMs that minimize/unmap transient
                // children with their parent — HIDDEN, with no way back (operator-
                // reported: "alt-tab → windows disappear"). On regaining focus, bring
                // the whole group back: raise the main window, then for every LOGICALLY
                // OPEN detached panel (IsVisible()), re-SHOW it if the WM cleared its
                // SHOWN flag, and raise it above the main window. Re-showing only
                // IsVisible() panels avoids un-hiding ones the user/game deliberately
                // closed. Throttled — Show/RaiseWindow can re-emit FOCUS_GAINED.
                // Not while the create-status (loading) dialog is up: raising the
                // main window here buries that dialog — and the dialog's own
                // periodic raise re-triggers THIS handler via the focus flip, so
                // the two fight and the dialog only ever flashes (the mac
                // "no creating-world dialog" bug). During a load the dialog owns
                // the screen; the group-restore resumes once it hides.
                if (m_createStatus && m_createStatus->IsVisible())
                    break;
                // Not while the main window is deliberately minimized either:
                // MinimizeAll runs from the modal options dialog, whose teardown
                // fires a focus event — SDL_RaiseWindow here would de-miniaturize
                // the game we just minimized and re-show every panel (operator:
                // "in-game minimize didn't work, game stayed up"). The MINIMIZED
                // flag alone races (it's set by a queued event that may arrive
                // AFTER the teardown focus event), so MinimizeAll also arms a
                // short stand-down deadline. When the user restores from the
                // Dock, macOS clears MINIMIZED first, so the next focus gain
                // (past the deadline) runs the group-restore and brings the
                // panels back as intended.
                if (m_window && (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED))
                    break;
                if (timeGetTime() < m_suppressGroupRestoreMs)
                    break;
                static DWORD s_lastRaise = 0;
                DWORD now = timeGetTime();
                if (now - s_lastRaise > 400) {
                    s_lastRaise = now;
                    if (m_window) SDL_RaiseWindow(m_window);
                    if (m_compositor) {
                        for (int i = 0; i < m_compositor->GetPanelCount(); ++i) {
                            SDL2Panel* panel = m_compositor->GetPanel(i);
                            if (!panel || !panel->IsDetached() || !panel->IsVisible())
                                continue;
                            SDL_Window* w = panel->GetOwnWindow();
                            if (!w) continue;
                            if (!(SDL_GetWindowFlags(w) & SDL_WINDOW_SHOWN))
                                SDL_ShowWindow(w);   // WM hid it on alt-tab → restore
                            SDL_RaiseWindow(w);
                        }
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
    if (event.type != SDL_KEYDOWN)
        return false;

    // Only while actually in a game (toolbar exists). At the main menu the menu
    // handles its own keys.
    if (!theApp.m_wndBar.IsCreated())
        return false;

    // Every case dispatches through a portable direct call: the window-switch
    // commands hit the SAME public CWndBar handlers the toolbar buttons use, and
    // pause/save/boss hit CWndMain's handlers. The old path posted WM_COMMAND to
    // m_wndMain, but that MFC message map is dead in the SDL port so nothing was
    // dispatched — hence this whole routine used to be _WIN32-only and dead
    // everywhere.
    CWndBar&    bar  = theApp.m_wndBar;
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
        theApp.m_wndMain.OnBoss();   // minimize (portable via MinimizeAll)
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
        // Pause = server-side pause logic; Save = SaveGame (both SDL2-safe).
        case SDLK_p: theApp.m_wndMain.OnPause(); return true;
        case SDLK_s: theApp.m_wndMain.OnSave();  return true;
        default: break;
        }
    }
    return false;
}

void GameWindow::Raise() {
    if (m_window) {
        SDL_ShowWindow(m_window);
#if defined(__linux__)
        // Restack WITHOUT activating: SDL_RaiseWindow sends _NET_ACTIVE_WINDOW
        // with this window's (stale) last-input timestamp; once a detached panel
        // holds newer focus Mutter denies the activation, flags the main window
        // DEMANDS_ATTENTION and gnome-shell toasts "window is ready" (BUGS #33,
        // last case — fired on every game start). Raise() is about z-order, not
        // focus (the panels are re-raised above us right after, and the area map
        // keeping keyboard focus is desired), so a plain XRaiseWindow does the
        // job with no activation request for Mutter to refuse.
        extern void EnRaiseX11WindowNoActivate(SDL_Window* win);
        EnRaiseX11WindowNoActivate(m_window);
#else
        SDL_RaiseWindow(m_window);
#endif
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
