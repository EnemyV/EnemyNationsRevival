//---------------------------------------------------------------------------
// x11_transient_linux.cpp — set the X11 WM_TRANSIENT_FOR owner relationship for
// the game's detached panel windows (area map / radar / unit windows) on Linux.
//
// Why a separate translation unit: the system SDL2 headers on this build do NOT
// define SDL_VIDEO_DRIVER_X11, so <SDL_syswm.h> hides its `info.x11` union member
// and the SDL_SYSWM_X11 enum. We define the macro here to expose them (the runtime
// SDL .so IS an X11 build, so the struct layout matches), and pull <X11/Xlib.h> for
// XSetTransientForHint(). Both bring in X11 macros (None, Window, Bool, Status, ...)
// that collide with game code, so they are confined to THIS one small file and we
// expose a plain entry point instead.
//
// This replaces SDL_WINDOW_ALWAYS_ON_TOP on Linux (see SDL2Panel.cpp): that flag
// pins a window above the WHOLE X11 desktop — it floated the area map over the user's
// other applications (e.g. their terminal). WM_TRANSIENT_FOR instead keeps the panel
// above only its OWNER (the main game window) — the X11 analogue of the Win32
// GWLP_HWNDPARENT owner relationship — so other apps can still be raised over it.
//---------------------------------------------------------------------------
// X11-only: guard on __linux__, NOT !_WIN32 — macOS is also !_WIN32 but has no
// libX11, so building this TU there fails to link (XFlush/XSetTransientForHint).
// On macOS this compiles to an empty translation unit (panels use ALWAYS_ON_TOP;
// see SDL2Panel.cpp). The call site is likewise #if defined(__linux__).
#if defined(__linux__)

// X11 is the LINUX windowing system here; macOS (Aqua) has no X11 and doesn't need
// WM_TRANSIENT_FOR. Confine the X11 impl to __linux__ and stub it elsewhere (POSIX-but-
// not-Linux = macOS) so EnSetX11TransientFor still links. (Was guarded only by
// `#ifndef _WIN32`, which pulled the X11 code into the mac/clang build → the X11 header/
// symbols aren't there → `Undefined symbols: EnSetX11TransientFor` link failure on arm64.)
#ifdef __linux__

#ifndef SDL_VIDEO_DRIVER_X11
#define SDL_VIDEO_DRIVER_X11 1
#endif

#include <SDL.h>
#include <SDL_syswm.h>
#include <X11/Xlib.h>

// Declared (extern) at the call site in SDL2Panel.cpp.
void EnSetX11TransientFor(SDL_Window* panel, SDL_Window* owner)
{
    if (panel == NULL || owner == NULL)
        return;

    SDL_SysWMinfo panelInfo, ownerInfo;
    SDL_VERSION(&panelInfo.version);
    SDL_VERSION(&ownerInfo.version);

    if (SDL_GetWindowWMInfo(panel, &panelInfo) &&
        SDL_GetWindowWMInfo(owner, &ownerInfo) &&
        panelInfo.subsystem == SDL_SYSWM_X11 &&
        ownerInfo.subsystem == SDL_SYSWM_X11)
    {
        XSetTransientForHint(panelInfo.info.x11.display,
                             panelInfo.info.x11.window,
                             ownerInfo.info.x11.window);
        XFlush(panelInfo.info.x11.display);   // ensure the WM sees the hint promptly
    }
}

#else  // POSIX but not Linux (macOS): no X11 → no-op stub so the symbol links

#include <SDL.h>
// macOS uses native (Aqua) window management; there is no X11 WM_TRANSIENT_FOR to set.
void EnSetX11TransientFor(SDL_Window*, SDL_Window*) { }

#endif // __linux__

#endif // !_WIN32
