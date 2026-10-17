#include "stdafx.h"
#include "SDL2DisplayMetrics.h"
#include "w22_settings.h"
#include "en_logpath.h"
#include <SDL_syswm.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace {
int s_setting = -1;
unsigned s_generation = 0;
double DesktopScale(SDL_Window* window, double density) {
#ifdef _WIN32
    SDL_SysWMinfo wm; SDL_VERSION(&wm.version);
    if (SDL_GetWindowWMInfo(window, &wm)) {
        typedef UINT (WINAPI *GetDpiFn)(HWND);
        auto getDpi = (GetDpiFn)GetProcAddress(GetModuleHandleA("user32.dll"), "GetDpiForWindow");
        if (getDpi) { UINT dpi = getDpi(wm.info.win.window); if (dpi) return dpi / 96.0; }
        HMODULE shcore = LoadLibraryA("shcore.dll");
        if (shcore) {
            typedef HRESULT (WINAPI *GetMonitorDpiFn)(HMONITOR, int, UINT*, UINT*);
            auto getMonitorDpi = (GetMonitorDpiFn)GetProcAddress(shcore, "GetDpiForMonitor");
            UINT x = 96, y = 96;
            const bool ok = getMonitorDpi && SUCCEEDED(getMonitorDpi(
                MonitorFromWindow(wm.info.win.window, MONITOR_DEFAULTTONEAREST), 0, &x, &y));
            FreeLibrary(shcore);
            if (ok && y) return y / 96.0;
        }
        HDC dc = GetDC(wm.info.win.window);
        if (dc) { int dpi = GetDeviceCaps(dc, LOGPIXELSY); ReleaseDC(wm.info.win.window, dc); if (dpi > 0) return dpi / 96.0; }
    }
#elif defined(__APPLE__)
    return density;
#else
    // SDL's native high-DPI backend supplies a backing ratio where supported.
    // X11 has no universal per-monitor UI policy. Accept explicit toolkit scale
    // hints, otherwise use 1x; physical monitor DPI is intentionally not used.
    if (density > 1.01) return density;
    const char* env = SDL_getenv("GDK_SCALE");
    if (!env || !*env) env = SDL_getenv("QT_SCALE_FACTOR");
    if (env && *env) {
        char* end = nullptr; double value = std::strtod(env, &end);
        if (end != env && *end == '\0' && std::isfinite(value) && value > 0) return value;
    }
#endif
    return 1;
}
}

void EnDpi::InitializeProcess() {
#ifdef _WIN32
    // Keep SDL coordinates in native pixels: legacy HWND cursor bridges rely on
    // this. Per-monitor-v2 removes OS bitmap virtualization without SDL points.
    typedef BOOL (WINAPI *SetContextFn)(HANDLE);
    auto setContext = (SetContextFn)GetProcAddress(GetModuleHandleA("user32.dll"), "SetProcessDpiAwarenessContext");
    if (setContext) setContext((HANDLE)(INT_PTR)-4); // PER_MONITOR_AWARE_V2
    else {
        HMODULE shcore = LoadLibraryA("shcore.dll");
        if (shcore) {
            typedef HRESULT (WINAPI *SetAwarenessFn)(int);
            auto setAwareness = (SetAwarenessFn)GetProcAddress(shcore, "SetProcessDpiAwareness");
            if (setAwareness) setAwareness(2);
            FreeLibrary(shcore);
        } else SetProcessDPIAware();
    }
    SDL_SetHint("SDL_WINDOWS_DPI_SCALING", "0");
#endif
}

int EnDpi::Setting() {
    if (s_setting < 0) {
        s_setting = w22::GetProfileInt("Game", "UIScale", 0);
        if (!SDL2DisplayMetrics::AllowedScale(s_setting)) s_setting = 0;
    }
    return s_setting;
}
void EnDpi::CommitSetting(int percent) {
    if (!SDL2DisplayMetrics::AllowedScale(percent)) percent = 0;
    if (Setting() != percent) ++s_generation;
    s_setting = percent;
    w22::WriteProfileInt("Game", "UIScale", percent);
}
unsigned EnDpi::Generation() { return s_generation; }
bool EnDpi::IsWayland() {
    const char* driver = SDL_GetCurrentVideoDriver();
    return driver && std::strcmp(driver, "wayland") == 0;
}
Uint32 EnDpi::MouseButtons() {
    return IsWayland() ? SDL_GetMouseState(nullptr, nullptr) : SDL_GetGlobalMouseState(nullptr, nullptr);
}

bool SDL2DisplayMetrics::Refresh(SDL_Window* window, SDL_Renderer* renderer) {
    if (!window) return false;
    SDL2DisplayMetrics next;
    next.display = SDL_GetWindowDisplayIndex(window);
    SDL_GetWindowSize(window, &next.windowW, &next.windowH);
    if (next.windowW <= 0 || next.windowH <= 0 ||
        (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)) { valid = false; return false; }
    if (renderer) {
        if (SDL_GetRendererOutputSize(renderer, &next.drawableW, &next.drawableH) != 0) { valid = false; return false; }
    } else {
        if (display >= 0 && next.display >= 0 && display != next.display)
            SDL_DestroyWindowSurface(window);
        SDL_Surface* surface = SDL_GetWindowSurface(window);
        if (!surface) { valid = false; return false; }
        next.drawableW = surface->w; next.drawableH = surface->h;
    }
    if (next.drawableW <= 0 || next.drawableH <= 0) { valid = false; return false; }
    next.densityX = (double)next.drawableW / next.windowW;
    next.densityY = (double)next.drawableH / next.windowH;
    next.pixelScale = SelectScale(EnDpi::Setting(), DesktopScale(window, next.densityY));
    next.valid = true;
    const bool changed = !valid || windowW != next.windowW || windowH != next.windowH ||
        drawableW != next.drawableW || drawableH != next.drawableH || display != next.display ||
        std::abs(pixelScale - next.pixelScale) > 0.001;
    *this = next;
    if (changed) {
        SDL_version version; SDL_GetVersion(&version);
        FILE* log = std::fopen(EnLogPath("SDL2Dpi.log").c_str(), "a");
        if (log) {
            std::fprintf(log, "window=%u backend=%s SDL=%u.%u.%u display=%d window=%dx%d drawable=%dx%d setting=%d scale=%.3f\n",
                SDL_GetWindowID(window), SDL_GetCurrentVideoDriver(), version.major, version.minor, version.patch,
                display, windowW, windowH, drawableW, drawableH, EnDpi::Setting(), pixelScale);
            std::fclose(log);
        }
    }
    return changed;
}
Uint32 EnDpi::EventWindowID(const SDL_Event& e) {
    switch (e.type) {
    case SDL_WINDOWEVENT: return e.window.windowID;
    case SDL_MOUSEMOTION: return e.motion.windowID;
    case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: return e.button.windowID;
    case SDL_MOUSEWHEEL: return e.wheel.windowID;
    case SDL_KEYDOWN: case SDL_KEYUP: return e.key.windowID;
    case SDL_TEXTINPUT: return e.text.windowID;
    case SDL_TEXTEDITING: return e.edit.windowID;
    default: return 0;
    }
}
void EnDpi::ClampWindow(SDL_Window* window) {
    SDL_Rect bounds;
    int display = SDL_GetWindowDisplayIndex(window);
    if (display < 0 || SDL_GetDisplayUsableBounds(display, &bounds) != 0) return;
    int x, y, w, h;
    SDL_GetWindowPosition(window, &x, &y); SDL_GetWindowSize(window, &w, &h);
    w = std::min(w, bounds.w); h = std::min(h, bounds.h);
    if (w <= 0 || h <= 0) return;
    SDL_SetWindowSize(window, w, h);
    SDL_SetWindowPosition(window, std::max(bounds.x, std::min(x, bounds.x + bounds.w - w)),
                                  std::max(bounds.y, std::min(y, bounds.y + bounds.h - h)));
}
