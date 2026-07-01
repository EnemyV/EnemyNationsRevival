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

#ifndef SDL_VIDEO_DRIVER_X11
#define SDL_VIDEO_DRIVER_X11 1
#endif

#include <SDL.h>
#include <SDL_syswm.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>   // XA_CARDINAL
#include <string.h>   // memset
#include <stdio.h>    // fprintf diagnostics

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

//---------------------------------------------------------------------------
// EnSetX11FakeTopExtent — let a detached panel be dragged ALL THE WAY to the top
// of the screen (under the gnome-shell top bar), closing the last gap in #55.
//
// Mutter constrains even an INTERACTIVE move (_NET_WM_MOVERESIZE below) so the
// window's top edge stays below the work-area top (the 32px gnome-shell panel
// strut) — native GNOME apps share the limit. But the constraint applies to the
// window's VISIBLE rect: for client-decorated windows that is the frame minus
// _GTK_FRAME_EXTENTS (margins the client declares as invisible shadow). Declaring
// a fake top extent equal to the strut height makes Mutter's math allow the real
// top edge to ride up to y=0, matching Windows where panels reach the screen top.
// Verified live on GNOME 48/Wayland (XWayland): extent 36 → area map dragged to
// y=6, impossible before. Visually inert — the WM draws nothing for borderless
// windows, and the game draws no shadow; only the constraint arithmetic changes.
//
// Trade-off (accepted): dragged to the extreme, the panel's own title bar sits
// UNDER the shell bar (clicks there go to gnome-shell), so it can't be grabbed
// again until moved programmatically (layout reset / restart restore). The strut
// height is read from _NET_WORKAREA; no top strut → property not set → no-op.
void EnSetX11FakeTopExtent(SDL_Window* panel)
{
    if (panel == NULL)
        return;

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(panel, &info) || info.subsystem != SDL_SYSWM_X11) {
        fprintf(stderr, "[TOPEXT] WMInfo failed or not X11 (subsystem=%d) err=%s\n",
                (int)info.subsystem, SDL_GetError());
        return;
    }

    Display* dpy = info.info.x11.display;

    long lft = 0;   // work-area left = width of the shell's dock strut
    long top = 0;   // work-area top  = height of the shell's top-bar strut
    {
        Atom workarea = XInternAtom(dpy, "_NET_WORKAREA", False);
        Atom type; int fmt; unsigned long n, left; unsigned char* data = NULL;
        if (workarea != None &&
            XGetWindowProperty(dpy, DefaultRootWindow(dpy), workarea, 0, 4, False,
                               XA_CARDINAL, &type, &fmt, &n, &left, &data) == Success &&
            data != NULL)
        {
            if (type == XA_CARDINAL && fmt == 32 && n >= 2) {
                lft = ((long*)data)[0];   // workarea = x, y, w, h — x/y ARE the struts
                top = ((long*)data)[1];
            }
            XFree(data);
        }
    }
    if (top <= 0 && lft <= 0) {
        fprintf(stderr, "[TOPEXT] no struts (workarea x=%ld y=%ld) — skipping\n", lft, top);
        return;
    }
    if (lft < 0) lft = 0;
    if (top < 0) top = 0;

    Atom extents = XInternAtom(dpy, "_GTK_FRAME_EXTENTS", False);
    if (extents == None)
        return;

    long ext[4] = { lft, 0, top, 0 };   // LEFT, right, TOP, bottom
    XChangeProperty(dpy, info.info.x11.window, extents, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char*)ext, 4);
    XFlush(dpy);
    fprintf(stderr, "[TOPEXT] set _GTK_FRAME_EXTENTS left=%ld top=%ld on X11 win 0x%lx\n",
            lft, top, (unsigned long)info.info.x11.window);
}

//---------------------------------------------------------------------------
// EnStartX11WindowMove — hand an in-progress title-bar drag to the window manager
// via the EWMH _NET_WM_MOVERESIZE protocol (the WM's own interactive move).
//
// Why: our detached panels are BORDERLESS SDL windows (no server-side titlebar), so
// the user can't WM-drag them; we drive the move ourselves with SDL_SetWindowPosition.
// But Mutter (GNOME/Wayland's XWayland WM) CLAMPS programmatic moves (ConfigureRequest)
// to the work area — a near-fullscreen area map then can't be dragged off-screen, to the
// top-left strut, or onto another monitor (operator-reported #55: "windows can't be put
// into the top / dragged right / off screen"). An INTERACTIVE move via _NET_WM_MOVERESIZE
// is positioned by Mutter without that clamp (it follows the pointer anywhere, incl.
// off-screen / cross-monitor) — matching how the windows behave on Windows.
//
// Unlike the Win32 modal SC_MOVE loop (which froze our rendering, the reason we drive the
// move manually elsewhere), _NET_WM_MOVERESIZE on X11 is ASYNC: Mutter grabs the pointer
// and moves the window while our render loop keeps running. On button-release Mutter ends
// the move and SDL delivers SDL_WINDOWEVENT_MOVED, which OnOwnWindowMoved() resyncs.
//
// Returns true if the WM move was initiated (caller then skips its manual drag).
bool EnStartX11WindowMove(SDL_Window* win, int xRoot, int yRoot)
{
    if (win == NULL)
        return false;

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(win, &info) || info.subsystem != SDL_SYSWM_X11)
        return false;

    Display* dpy  = info.info.x11.display;
    Window   w    = info.info.x11.window;
    Window   root = DefaultRootWindow(dpy);

    Atom moveresize = XInternAtom(dpy, "_NET_WM_MOVERESIZE", False);
    if (moveresize == None)
        return false;

    // Release the implicit pointer grab from the ButtonPress so Mutter can take the
    // pointer for the interactive move (harmless if there is none).
    XUngrabPointer(dpy, CurrentTime);
    XFlush(dpy);

    XClientMessageEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.type         = ClientMessage;
    ev.display      = dpy;
    ev.window       = w;
    ev.message_type = moveresize;
    ev.format       = 32;
    ev.data.l[0]    = (long)xRoot;   // pointer absolute (root) X at drag start
    ev.data.l[1]    = (long)yRoot;   // pointer absolute (root) Y at drag start
    ev.data.l[2]    = 8;             // _NET_WM_MOVERESIZE_MOVE
    ev.data.l[3]    = Button1;       // the button driving the move
    ev.data.l[4]    = 1;             // source indication: normal application

    XSendEvent(dpy, root, False,
               SubstructureRedirectMask | SubstructureNotifyMask,
               (XEvent*)&ev);
    XFlush(dpy);
    return true;
}

#endif // !_WIN32
