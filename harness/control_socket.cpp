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
void push_key(SDL_Keycode kc, bool down) {
    SDL_Event e; SDL_zero(e);
    e.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    e.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    e.key.keysym.sym = kc;
    e.key.keysym.scancode = SDL_GetScancodeFromKey(kc);
    e.key.windowID = active_window_id();
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
        g_shotDone = false; g_shotOK = false; g_shotPending = true;
        // wait (up to ~2s) for the render thread to service it
        for (int i = 0; i < 400 && !g_shotDone.load(); ++i) { struct timespec ts={0,5000000}; nanosleep(&ts,nullptr); }
        snprintf(reply, sizeof(reply), g_shotOK.load() ? "ok %d %d %s\n" : "err shot failed\n", g_shotW.load(), g_shotH.load(), path);
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
    } else if (strcmp(cmd, "moveid") == 0) {
        unsigned id=0; int x=0,y=0; sscanf(line.c_str(), "%*s %u %d %d", &id, &x, &y); push_mouse_move(x,y,id);
    } else if (strcmp(cmd, "move") == 0) {
        int x=0,y=0; sscanf(line.c_str(), "%*s %d %d", &x, &y); push_mouse_move(x,y);
    } else if (strcmp(cmd, "key") == 0) {
        long kc=0; sscanf(line.c_str(), "%*s %ld", &kc); push_key((SDL_Keycode)kc,true); push_key((SDL_Keycode)kc,false);
    } else if (strcmp(cmd, "text") == 0) {
        char txt[512]={0}; sscanf(line.c_str(), "%*s %511[^\n]", txt);
        SDL_Event e; SDL_zero(e); e.type=SDL_TEXTINPUT;
        std::strncpy(e.text.text, txt, sizeof(e.text.text)-1);
        if (g_window) e.text.windowID = SDL_GetWindowID(g_window);
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

void EnHarness_Service() {
    if (!g_shotPending.exchange(false)) return;
    std::string path; { std::lock_guard<std::mutex> lk(g_shotMutex); path = g_shotPath; }
    bool ok = false; int w = 0, h = 0;
    Uint32 wantId = g_shotWinId.load();
    SDL_Window* win = wantId ? SDL_GetWindowFromID(wantId) : active_window();
    if (!win) win = active_window();
    SDL_Renderer* rend = (win == g_window) ? g_renderer : SDL_GetRenderer(win);
    if (win) {
        SDL_GetWindowSize(win, &w, &h);
        if (rend) {
            // GPU path: read pixels from the renderer.
            int rw = w, rh = h; SDL_GetRendererOutputSize(rend, &rw, &rh);
            SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, rw, rh, 32, SDL_PIXELFORMAT_ARGB8888);
            if (surf && SDL_RenderReadPixels(rend, nullptr, SDL_PIXELFORMAT_ARGB8888, surf->pixels, surf->pitch) == 0) {
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
