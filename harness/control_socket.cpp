//---------------------------------------------------------------------------
// control_socket.cpp — in-process harness server (Linux/Debug only).
// See en_harness.h. Protocol: one text command per connection, newline-terminated:
//   shot <path>            -> render a frame grab to <path> (BMP), reply "ok <w> <h>"
//   click <x> <y> [right]  -> left/right click at client px (x,y)
//   move <x> <y>           -> mouse-move to (x,y)
//   key <keycode>          -> press an SDL keycode (SDLK_*) once
//   text <string>          -> type literal text
//   quit                   -> request the game to quit
// Coordinates are window client pixels. Input uses SDL_PushEvent (thread-safe);
// screenshots are deferred to EnHarness_Service() on the render thread.
//---------------------------------------------------------------------------
#ifdef _WIN32
#error "control_socket.cpp is Linux-only"
#endif

#include "en_harness.h"

#include <SDL.h>

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace {

SDL_Window*   g_window   = nullptr;
SDL_Renderer* g_renderer = nullptr;
SDL_Surface*  g_mainSurface = nullptr;   // main window CPU back-buffer (see EnHarness_SetMainSurface)
std::mutex    g_winSurfMutex;
std::map<Uint32, SDL_Surface*> g_winSurfaces;   // detached panel window id -> CPU back-surface

// The active window changes as modal dialogs open/close. Target whatever has
// keyboard/mouse focus so screenshots and input follow the on-top window.
// In-game the UI spawns several child SDL windows (toolbars, status strips,
// some as small as 1x24); focus can land on one of those, which is useless for
// a screenshot. So prefer the focused window only when it's reasonably sized,
// otherwise fall back to the LARGEST visible window (the map/main view). SDL has
// no window enumeration, but window IDs are small ints — scan a low range.
// IDs are monotonic and never reused, so after many game reloads (each spawns
// and destroys child windows) live IDs climb; keep the cap well above the
// handful of windows ever shown at once.
static const Uint32 kMaxWindowId = 256;
SDL_Window* largest_window() {
    SDL_Window* best = g_window; int bestArea = 0;
    if (g_window) { int w,h; SDL_GetWindowSize(g_window,&w,&h); bestArea = w*h; }
    for (Uint32 id = 1; id <= kMaxWindowId; ++id) {
        SDL_Window* win = SDL_GetWindowFromID(id);
        if (!win) continue;
        if ((SDL_GetWindowFlags(win) & SDL_WINDOW_SHOWN) == 0) continue;
        int w = 0, h = 0; SDL_GetWindowSize(win, &w, &h);
        if (w * h > bestArea) { bestArea = w * h; best = win; }
    }
    return best;
}
SDL_Window* active_window() {
    SDL_Window* f = SDL_GetKeyboardFocus();
    if (!f) f = SDL_GetMouseFocus();
    if (f) {
        int w = 0, h = 0; SDL_GetWindowSize(f, &w, &h);
        if (w >= 200 && h >= 200) return f;   // a real dialog/view, not a tiny strip
    }
    return largest_window();
}
Uint32 active_window_id() {
    SDL_Window* w = active_window();
    return w ? SDL_GetWindowID(w) : 0;
}

// Pending `units` request (HarnessDumpUnits), serviced on the render thread —
// it reads live game state (theVehicleMap/theAreaList), which must not race the
// game loop. Same defer-to-render-thread handshake as screenshots.
std::mutex              g_unitsMutex;
std::string            g_unitsResult;
std::atomic<bool>      g_unitsPending{false};
std::atomic<bool>      g_unitsDone{false};

// Pending screenshot request, serviced on the render thread.
std::mutex              g_shotMutex;
std::string            g_shotPath;
std::atomic<Uint32>    g_shotWinId{0};   // 0 = use active_window(); else this window id
std::atomic<bool>      g_shotPending{false};
std::atomic<bool>      g_shotDone{false};
std::atomic<bool>      g_shotOK{false};
// Written by the render thread in EnHarness_Service, read by the socket thread;
// atomic to avoid a data race. g_shotDone (stored last / read first) is the
// release/acquire fence that publishes these together with g_shotOK.
std::atomic<int>       g_shotW{0}, g_shotH{0};
// GPU-panel readback (RenderReadPixels) can race the present and return a blank
// (cleared) frame — a GPU-terrain detached panel has no CPU back-surface to dump.
// Instead of making the client retry, retry server-side: on a blank read, re-arm
// and re-read on the next compositor frame (the panel redraws between frames), up
// to a cap. One `shotid` => one good frame for the client.
std::atomic<int>       g_shotRetry{0};
static const int       kMaxShotRetry = 24;   // ~0.4s @ 60fps; client waits up to ~2s

// True if a freshly read-back surface is essentially blank (all near-black) — the
// signature of a cleared GPU buffer grabbed mid-present. Samples a sparse grid so
// it stays cheap on a 1400x1000 surface.
static bool surface_is_blank(SDL_Surface* s) {
    if (!s || !s->pixels) return false;
    if (SDL_MUSTLOCK(s)) SDL_LockSurface(s);
    bool blank = true;
    const int step = 16;
    for (int y = 0; y < s->h && blank; y += step) {
        const Uint8* row = (const Uint8*)s->pixels + (size_t)y * s->pitch;
        for (int x = 0; x < s->w; x += step) {
            Uint32 px = *(const Uint32*)(row + (size_t)x * s->format->BytesPerPixel);
            Uint8 r, g, b, a; SDL_GetRGBA(px, s->format, &r, &g, &b, &a);
            if (r > 16 || g > 16 || b > 16) { blank = false; break; }
        }
    }
    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);
    return blank;
}

void push_mouse_button(int x, int y, Uint8 button, bool down, Uint32 winId = 0, Uint8 clicks = 1) {
    SDL_Event e; SDL_zero(e);
    e.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    e.button.button = button;
    e.button.state  = down ? SDL_PRESSED : SDL_RELEASED;
    e.button.clicks = clicks;
    e.button.x = x; e.button.y = y;
    e.button.windowID = winId ? winId : active_window_id();
    SDL_PushEvent(&e);
}
void push_mouse_move(int x, int y, Uint32 winId = 0) {
    SDL_Event e; SDL_zero(e);
    e.type = SDL_MOUSEMOTION;
    e.motion.x = x; e.motion.y = y;
    e.motion.windowID = winId ? winId : active_window_id();
    SDL_PushEvent(&e);
}
void push_mouse_wheel(int dy, Uint32 winId = 0) {
    // Synthesize a vertical mouse-wheel notch. The Area Map reads event.wheel.y
    // (area.cpp: zDelta = wheel.y * WHEEL_DELTA) and needs 2*WHEEL_DELTA to step
    // zoom — so dy=+2 zooms in one level, dy=-2 zooms out. Routed by windowID
    // (GameWindow dispatches SDL_MOUSEWHEEL by event.wheel.windowID), so target
    // the Area Map (window 5) directly — the untargeted active window is the
    // larger main Game View, which ignores the wheel.
    SDL_Event e; SDL_zero(e);
    e.type = SDL_MOUSEWHEEL;
    e.wheel.y = dy;
    e.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
    e.wheel.windowID = winId ? winId : active_window_id();
    SDL_PushEvent(&e);
}
void push_key(SDL_Keycode kc, bool down, Uint32 winId = 0, Uint16 mod = 0) {
    SDL_Event e; SDL_zero(e);
    e.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    e.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    e.key.keysym.sym = kc;
    e.key.keysym.scancode = SDL_GetScancodeFromKey(kc);
    e.key.keysym.mod = mod;   // pass modifiers (e.g. KMOD_CTRL=0xC0) so Ctrl+key accelerators fire
    // Route to a specific window when asked. In-game hotkeys (e.g. 'R' build-road)
    // are handled by the Area Map child window, but active_window_id() picks the
    // LARGEST window — the main Game View is bigger than the map — so an untargeted
    // key never reaches the map. keyid lets the driver name the target window.
    e.key.windowID = winId ? winId : active_window_id();
    SDL_PushEvent(&e);
}

void handle_command(const std::string& line, int conn) {
    char reply[256] = "ok\n";
    char cmd[32] = {0};
    if (sscanf(line.c_str(), "%31s", cmd) != 1) { write(conn, "err\n", 4); return; }

    if (strcmp(cmd, "shot") == 0 || strcmp(cmd, "shotid") == 0) {
        char path[1024] = {0};
        Uint32 winId = 0;
        if (strcmp(cmd, "shotid") == 0) {
            unsigned id = 0;
            sscanf(line.c_str(), "%*s %u %1023[^\n]", &id, path);
            winId = id;
        } else {
            sscanf(line.c_str(), "%*s %1023[^\n]", path);
        }
        if (!path[0]) std::strcpy(path, "/tmp/enshot.bmp");
        { std::lock_guard<std::mutex> lk(g_shotMutex); g_shotPath = path; }
        g_shotWinId = winId;
        g_shotDone = false; g_shotOK = false; g_shotRetry = 0; g_shotPending = true;
        // wait (up to ~2s) for the render thread to service it
        for (int i = 0; i < 400 && !g_shotDone.load(); ++i) { struct timespec ts={0,5000000}; nanosleep(&ts,nullptr); }
        snprintf(reply, sizeof(reply), g_shotOK.load() ? "ok %d %d %s\n" : "err shot failed\n", g_shotW.load(), g_shotH.load(), path);
    } else if (strcmp(cmd, "units") == 0) {
        // Enumerate the local player's units (HarnessDumpUnits) on the render
        // thread, then return the lines. Deterministic crane/unit location.
        g_unitsDone = false; g_unitsPending = true;
        for (int i = 0; i < 400 && !g_unitsDone.load(); ++i) { struct timespec ts={0,5000000}; nanosleep(&ts,nullptr); }
        std::string out;
        { std::lock_guard<std::mutex> lk(g_unitsMutex); out = g_unitsResult; }
        if (!g_unitsDone.load()) out = "err units timeout (not in-game?)\n";
        write(conn, out.c_str(), out.size());
        return;
    } else if (strcmp(cmd, "click") == 0) {
        int x=0,y=0; char rb[16]={0};
        sscanf(line.c_str(), "%*s %d %d %15s", &x, &y, rb);
        Uint8 btn = (rb[0]=='r') ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
        push_mouse_move(x,y); push_mouse_button(x,y,btn,true); push_mouse_button(x,y,btn,false);
    } else if (strcmp(cmd, "clickid") == 0) {
        // clickid <winId> <x> <y> [right] — click a SPECIFIC window (the in-game map
        // is window 5, but it's smaller than the main window so active_window() picks
        // the main one; this routes the click to the intended window directly).
        unsigned id=0; int x=0,y=0; char rb[16]={0};
        sscanf(line.c_str(), "%*s %u %d %d %15s", &id, &x, &y, rb);
        Uint8 btn = (rb[0]=='r') ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
        push_mouse_move(x,y,id); push_mouse_button(x,y,btn,true,id); push_mouse_button(x,y,btn,false,id);
    } else if (strcmp(cmd, "dblclickid") == 0) {
        // dblclickid <winId> <x> <y> — proper double-click: a select-click (clicks=1)
        // then a clicks=2 press (the game's OnLButtonDblClk needs button.clicks>=2).
        unsigned id=0; int x=0,y=0;
        sscanf(line.c_str(), "%*s %u %d %d", &id, &x, &y);
        push_mouse_move(x,y,id);
        push_mouse_button(x,y,SDL_BUTTON_LEFT,true,id,1);  push_mouse_button(x,y,SDL_BUTTON_LEFT,false,id,1);
        push_mouse_button(x,y,SDL_BUTTON_LEFT,true,id,2);  push_mouse_button(x,y,SDL_BUTTON_LEFT,false,id,2);
    } else if (strcmp(cmd, "dragid") == 0) {
        // dragid <winId> <x1> <y1> <x2> <y2> [right] — press at (x1,y1), drag to
        // (x2,y2), release. Needed for gestures the game reads as a drag: crane
        // road-build (press 'R', then drag start->end), box-select, and the
        // right-drag line-move. The game CaptureMouse()s on press, so the move +
        // release route to the captured window regardless of the move's target.
        unsigned id=0; int x1=0,y1=0,x2=0,y2=0; char rb[16]={0};
        sscanf(line.c_str(), "%*s %u %d %d %d %d %15s", &id, &x1,&y1,&x2,&y2, rb);
        Uint8 btn = (rb[0]=='r') ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
        push_mouse_move(x1,y1,id);
        push_mouse_button(x1,y1,btn,true,id);
        for (int s=1; s<=4; ++s)   // interpolate so the game tracks the drag path
            push_mouse_move(x1+(x2-x1)*s/4, y1+(y2-y1)*s/4, id);
        push_mouse_button(x2,y2,btn,false,id);
    } else if (strcmp(cmd, "moveid") == 0) {
        unsigned id=0; int x=0,y=0; sscanf(line.c_str(), "%*s %u %d %d", &id, &x, &y); push_mouse_move(x,y,id);
    } else if (strcmp(cmd, "move") == 0) {
        int x=0,y=0; sscanf(line.c_str(), "%*s %d %d", &x, &y); push_mouse_move(x,y);
    } else if (strcmp(cmd, "scroll") == 0 || strcmp(cmd, "wheel") == 0) {
        // scroll <winId> <dy> [x y] — synthesize a mouse-wheel notch at the named
        // window. The Area Map (window 5) zooms with the wheel: dy=+2 zooms IN one
        // level, dy=-2 zooms OUT (needs 2*WHEEL_DELTA per step). This makes units
        // big enough to click — at the default zoom (z0) starting units are
        // sub-click-precision and missed clicks recenter the view.
        //
        // KEY DETAIL: the detached Area Map panel routes wheel events by the REAL
        // cursor position (SDL2Panel::HandleEvent uses SDL_GetMouseState for
        // SDL_MOUSEWHEEL, then bounds-checks "inContent"). Headless, the OS cursor
        // isn't over the panel, so the wheel is dropped. So we first warp the SDL
        // mouse into the target window's content (default = window center, which is
        // below the title bar) — that updates SDL's internal mouse state so the
        // wheel lands in-content and reaches CWndArea::OnMouseWheel -> ZoomIn/Out.
        unsigned id=0; int dy=0, wx=-1, wy=-1;
        sscanf(line.c_str(), "%*s %u %d %d %d", &id, &dy, &wx, &wy);
        if (dy == 0) dy = 2;   // default: one zoom-in step
        SDL_Window* w = id ? SDL_GetWindowFromID(id) : nullptr;
        if (w) {
            if (wx < 0 || wy < 0) { int ww=0,wh=0; SDL_GetWindowSize(w,&ww,&wh); wx=ww/2; wy=wh/2; }
            SDL_WarpMouseInWindow(w, wx, wy);
        }
        push_mouse_wheel(dy, id);
    } else if (strcmp(cmd, "key") == 0) {
        // key <keycode> [mod] — optional 2nd arg = modifier mask (KMOD_CTRL=192) so Ctrl+ accelerators fire.
        long kc=0, mod=0; sscanf(line.c_str(), "%*s %ld %ld", &kc, &mod);
        push_key((SDL_Keycode)kc,true,0,(Uint16)mod); push_key((SDL_Keycode)kc,false,0,(Uint16)mod);
    } else if (strcmp(cmd, "keyid") == 0) {
        // keyid <winId> <keycode> [mod] — press a key targeted at a SPECIFIC window
        // (in-game hotkeys like 'R' build-road must reach the Area Map window,
        // which active_window_id() won't pick since the main view is larger).
        unsigned id=0; long kc=0, mod=0; sscanf(line.c_str(), "%*s %u %ld %ld", &id, &kc, &mod);
        push_key((SDL_Keycode)kc,true,id,(Uint16)mod); push_key((SDL_Keycode)kc,false,id,(Uint16)mod);
    } else if (strcmp(cmd, "text") == 0) {
        char txt[512]={0}; sscanf(line.c_str(), "%*s %511[^\n]", txt);
        SDL_Event e; SDL_zero(e); e.type=SDL_TEXTINPUT;
        std::strncpy(e.text.text, txt, sizeof(e.text.text)-1);
        if (g_window) e.text.windowID = SDL_GetWindowID(g_window);
        SDL_PushEvent(&e);
    } else if (strcmp(cmd, "textid") == 0) {
        // textid <winId> <string> — SDL_TEXTINPUT targeted at a SPECIFIC window's
        // focused editbox. Modal child dialogs (Join Network Game server-address,
        // Pick-Your-Race name, ...) are their own SDL windows that plain `text`
        // (hardcoded to g_window/main) can't reach. Click the field first to focus
        // it, then textid the string. Mirrors clickid/keyid.
        unsigned id=0; char txt[512]={0};
        sscanf(line.c_str(), "%*s %u %511[^\n]", &id, txt);
        SDL_Event e; SDL_zero(e); e.type=SDL_TEXTINPUT;
        std::strncpy(e.text.text, txt, sizeof(e.text.text)-1);
        e.text.windowID = id ? id : (g_window ? SDL_GetWindowID(g_window) : 0);
        SDL_PushEvent(&e);
    } else if (strcmp(cmd, "raise") == 0) {
        // Bring all app windows forward (un-bury borderless detached panels).
        for (Uint32 id = 1; id <= kMaxWindowId; ++id) {
            SDL_Window* w = SDL_GetWindowFromID(id);
            if (w && (SDL_GetWindowFlags(w) & SDL_WINDOW_SHOWN)) SDL_RaiseWindow(w);
        }
    } else if (strcmp(cmd, "quit") == 0) {
        SDL_Event e; SDL_zero(e); e.type=SDL_QUIT; SDL_PushEvent(&e);
    } else if (strcmp(cmd, "wins") == 0) {
        // Enumerate windows (id:WxH shown title) so the driver can pick the view.
        std::string out;
        for (Uint32 id = 1; id <= kMaxWindowId; ++id) {
            SDL_Window* win = SDL_GetWindowFromID(id);
            if (!win) continue;
            int w=0,h=0; SDL_GetWindowSize(win,&w,&h);
            bool shown = (SDL_GetWindowFlags(win) & SDL_WINDOW_SHOWN) != 0;
            const char* title = SDL_GetWindowTitle(win);
            char buf[256];
            snprintf(buf,sizeof(buf),"%u:%dx%d %s \"%s\"\n", id, w, h, shown?"shown":"hidden", title?title:"");
            out += buf;
        }
        if (out.empty()) out = "(no windows)\n";
        write(conn, out.c_str(), out.size());
        return;
    } else {
        std::strcpy(reply, "err unknown\n");
    }
    write(conn, reply, std::strlen(reply));
}

void* server_thread(void*) {
    // A client that disconnects mid-reply would otherwise SIGPIPE-kill the game;
    // ignore it so a stray write() just fails with EPIPE.
    signal(SIGPIPE, SIG_IGN);
    int port = 7070;
    if (const char* p = getenv("EN_HARNESS_PORT")) { int v = atoi(p); if (v > 0) port = v; }
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) return nullptr;
    int one = 1; setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_in addr; std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); addr.sin_port = htons(port);
    if (bind(srv, (sockaddr*)&addr, sizeof(addr)) < 0) { fprintf(stderr,"[harness] bind :%d failed\n",port); close(srv); return nullptr; }
    listen(srv, 4);
    fprintf(stderr, "[harness] listening on 127.0.0.1:%d\n", port);
    for (;;) {
        int conn = accept(srv, nullptr, nullptr);
        if (conn < 0) {
            if (errno == EINTR) continue;
            struct timespec ts = {0, 10000000}; nanosleep(&ts, nullptr);  // avoid busy-spin on persistent error
            continue;
        }
        // Read one newline-terminated command. Loop because a command can arrive
        // split across TCP segments; cap total size to bound a misbehaving client.
        std::string line; char buf[2048];
        for (;;) {
            ssize_t n = read(conn, buf, sizeof(buf));
            if (n <= 0) break;
            line.append(buf, (size_t)n);
            if (line.find('\n') != std::string::npos || line.size() > 64 * 1024) break;
        }
        if (!line.empty()) handle_command(line, conn);
        close(conn);
    }
    return nullptr;
}

} // namespace

void EnHarness_Start(SDL_Window* window, SDL_Renderer* renderer) {
    if (!getenv("EN_HARNESS")) return;
    g_window = window; g_renderer = renderer;
    pthread_t tid; pthread_create(&tid, nullptr, server_thread, nullptr); pthread_detach(tid);
}

void EnHarness_SetMainSurface(SDL_Surface* surface) {
    g_mainSurface = surface;
}

void EnHarness_RegisterWindowSurface(unsigned int windowId, SDL_Surface* surface) {
    std::lock_guard<std::mutex> lk(g_winSurfMutex);
    if (surface) g_winSurfaces[(Uint32)windowId] = surface;
    else         g_winSurfaces.erase((Uint32)windowId);
}

void EnHarness_Service() {
    // Service a pending `units` request on the render thread (reads live game
    // state safely, in sync with the game loop). One request per frame.
    if (g_unitsPending.exchange(false)) {
        std::string out;
        HarnessDumpUnits(out);
        { std::lock_guard<std::mutex> lk(g_unitsMutex); g_unitsResult = out; }
        g_unitsDone = true;
        return;
    }
    if (!g_shotPending.exchange(false)) return;
    std::string path; { std::lock_guard<std::mutex> lk(g_shotMutex); path = g_shotPath; }
    bool ok = false; int w = 0, h = 0;
    Uint32 wantId = g_shotWinId.load();
    SDL_Window* win = wantId ? SDL_GetWindowFromID(wantId) : active_window();
    if (!win) win = active_window();
    SDL_Renderer* rend = (win == g_window) ? g_renderer : SDL_GetRenderer(win);
    // Preferred path for the main window: dump its CPU back-buffer directly. The
    // compositor draws the full frame into it every present, so it holds the real
    // image even when SDL_RenderReadPixels reads back blank (macOS Metal/GL) or
    // there is no on-screen drawable (headless session). We run on the render
    // thread synchronously with compositing, so the surface is not being written.
    if (win == g_window && g_mainSurface) {
        SDL_GetWindowSize(win, &w, &h);
        ok = (SDL_SaveBMP(g_mainSurface, path.c_str()) == 0);
        if (ok) { w = g_mainSurface->w; h = g_mainSurface->h; }
        g_shotW = w; g_shotH = h; g_shotOK = ok; g_shotDone = true;
        return;
    }
    // Detached panel with a registered CPU back-surface: dump it directly
    // (reliable; GPU read-back of child windows is blank on macOS).
    if (win) {
        Uint32 wid = SDL_GetWindowID(win);
        SDL_Surface* reg = nullptr;
        { std::lock_guard<std::mutex> lk(g_winSurfMutex);
          auto it = g_winSurfaces.find(wid); if (it != g_winSurfaces.end()) reg = it->second; }
        if (reg) {
            ok = (SDL_SaveBMP(reg, path.c_str()) == 0);
            g_shotW = reg->w; g_shotH = reg->h; g_shotOK = ok; g_shotDone = true;
            return;
        }
    }
    if (win) {
        SDL_GetWindowSize(win, &w, &h);
        if (rend) {
            // GPU path: read pixels from the renderer.
            int rw = w, rh = h; SDL_GetRendererOutputSize(rend, &rw, &rh);
            SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, rw, rh, 32, SDL_PIXELFORMAT_ARGB8888);
            if (surf && SDL_RenderReadPixels(rend, nullptr, SDL_PIXELFORMAT_ARGB8888, surf->pixels, surf->pitch) == 0) {
                // Blank read = cleared GPU buffer grabbed mid-present. Re-arm and
                // re-read on the next compositor frame (the panel redraws between
                // frames), up to the cap — so a single shotid returns one good
                // frame instead of the client having to retry-til-non-black.
                if (surface_is_blank(surf) && g_shotRetry.fetch_add(1) < kMaxShotRetry) {
                    SDL_FreeSurface(surf);
                    g_shotPending = true;   // service again next frame; leave g_shotDone false
                    return;
                }
                ok = (SDL_SaveBMP(surf, path.c_str()) == 0); w = rw; h = rh;
            }
            if (surf) SDL_FreeSurface(surf);
        } else {
            // Software path: grab the window surface.
            SDL_Surface* ws = SDL_GetWindowSurface(win);
            if (ws) ok = (SDL_SaveBMP(ws, path.c_str()) == 0);
        }
    }
    g_shotW = w; g_shotH = h; g_shotOK = ok; g_shotDone = true;
}
