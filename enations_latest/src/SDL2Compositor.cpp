#include "stdafx.h"

#include "SDL2Compositor.h"
#include "SDL2Panel.h"
#include "GameWindow.h"
#include "SDL2MainMenu.h"  // For CreateSurfaceFromDIB
#include "lastplnt.h"      // For theDataFile, ptrthebltformat
#include "bmbutton.h"      // Must precede bitmaps.h (provides CBmBtnData)
#include "bitmaps.h"       // For DIB_GOLD, theBitmaps
#include "player.h"        // theGame.GetFrame() — water-animation re-render tick
#include "terrain.h"       // theMap.HaveBldgCur() — keep the live build-cursor overlay animating
#include "Perf.h"          // MEASURE: main-window composite cost

#include <SDL.h>
#include <algorithm>
#include <fstream>

static void LogCompositor(const std::string& msg) {
    std::ofstream log("SDL2Compositor.log", std::ios::app);
    if (log.is_open()) {
        log << msg << std::endl;
    }
}

SDL2Compositor::SDL2Compositor(GameWindow* window)
    : m_window(window)
    , m_wallpaper(nullptr)
    , m_ownedWallpaper(nullptr)
    , m_backgroundDirty(true)
{
    LogCompositor("SDL2Compositor created");
}

SDL2Compositor::~SDL2Compositor() {
    RemoveAllPanels();
    if (m_ownedWallpaper) {
        SDL_FreeSurface(m_ownedWallpaper);
        m_ownedWallpaper = nullptr;
    }
    LogCompositor("SDL2Compositor destroyed");
}

bool SDL2Compositor::LoadWallpaper() {
    // Load the CWndMain wallpaper from MISC data file.
    // This is the purple tiled background behind all game windows —
    // the same bitmap CWndMain::m_pcdibWall uses.
    try {
        CMmio* pMmio = theDataFile.OpenAsMMIO("misc", "MISC");
        if (!pMmio) {
            LogCompositor("ERROR: Could not open MISC data file");
            return false;
        }

        pMmio->DescendRiff('M', 'I', 'S', 'C');
        pMmio->DescendList('W', 'L', theApp.m_szOtherBPS[0], theApp.m_szOtherBPS[1]);
        pMmio->DescendChunk('D', 'A', 'T', 'A');

        CDIB* pWall = new CDIB(ptrthebltformat->GetColorFormat(),
                                CBLTFormat::DIB_MEMORY,
                                ptrthebltformat->GetMemDirection());
        pWall->Load(*pMmio);
        delete pMmio;

        // Free previous owned wallpaper if any
        if (m_ownedWallpaper) {
            SDL_FreeSurface(m_ownedWallpaper);
            m_ownedWallpaper = nullptr;
        }

        m_ownedWallpaper = SDL2MainMenu::CreateSurfaceFromDIB(pWall);
        delete pWall;

        if (!m_ownedWallpaper) {
            LogCompositor("ERROR: Failed to create wallpaper surface from MISC");
            return false;
        }

        m_wallpaper = m_ownedWallpaper;
        m_backgroundDirty = true;

        LogCompositor("Loaded MISC wallpaper: " + std::to_string(m_wallpaper->w) + "x" +
                      std::to_string(m_wallpaper->h));
        return true;
    }
    catch (...) {
        LogCompositor("ERROR: Exception loading MISC wallpaper");
        return false;
    }
}

void SDL2Compositor::SetWallpaperSurface(SDL_Surface* wallpaper) {
    // If we owned a previous wallpaper, free it
    if (m_ownedWallpaper) {
        SDL_FreeSurface(m_ownedWallpaper);
        m_ownedWallpaper = nullptr;
    }
    m_ownedWallpaper = wallpaper;  // Take ownership so it's freed on destruction
    m_wallpaper = wallpaper;
    m_backgroundDirty = true;
}

SDL2Panel* SDL2Compositor::AddPanel(const std::string& name, int x, int y, int w, int h, int zOrder) {
    auto panel = std::make_unique<SDL2Panel>(name, x, y, w, h, zOrder);
    SDL2Panel* raw = panel.get();
    m_panels.push_back(std::move(panel));
    SortPanels();
    m_backgroundDirty = true;
    LogCompositor("Added panel '" + name + "' (total: " + std::to_string(m_panels.size()) + ")");
    return raw;
}

void SDL2Compositor::RemovePanel(SDL2Panel* panel) {
    if (m_routingDepth > 0) {
        // Defer removal — vector is being iterated by RouteEvent.
        // Hide the panel immediately so it stops rendering/receiving events.
        panel->SetVisible(false);
        m_pendingRemovals.push_back(panel);
        return;
    }
    auto it = std::find_if(m_panels.begin(), m_panels.end(),
        [panel](const std::unique_ptr<SDL2Panel>& p) { return p.get() == panel; });
    if (it != m_panels.end()) {
        LogCompositor("Removed panel '" + (*it)->GetName() + "'");
        m_panels.erase(it);
        m_backgroundDirty = true;
    }
}

void SDL2Compositor::FlushPendingRemovals() {
    for (SDL2Panel* panel : m_pendingRemovals) {
        auto it = std::find_if(m_panels.begin(), m_panels.end(),
            [panel](const std::unique_ptr<SDL2Panel>& p) { return p.get() == panel; });
        if (it != m_panels.end()) {
            LogCompositor("Deferred removal of panel '" + (*it)->GetName() + "'");
            m_panels.erase(it);
        }
    }
    if (!m_pendingRemovals.empty())
        m_backgroundDirty = true;
    m_pendingRemovals.clear();
}

void SDL2Compositor::RemoveAllPanels() {
    m_panels.clear();
    m_backgroundDirty = true;
}

SDL2Panel* SDL2Compositor::FindPanel(const std::string& name) const {
    for (auto& p : m_panels) {
        if (p->GetName() == name)
            return p.get();
    }
    return nullptr;
}

bool SDL2Compositor::AnyPanelInteracting() const {
    for (auto& p : m_panels)
        if (p && p->IsInteracting())
            return true;
    return false;
}

void SDL2Compositor::ResetWindowLayout() {
    // Forget every remembered placement so closed/recreated windows no longer
    // snap back to wherever they last were.
    SDL2Panel::ResetAllPlacements();

    const int W = m_window ? m_window->GetWidth()  : 1280;
    const int H = m_window ? m_window->GetHeight() : 720;
    const int tb = SDL2Panel::TITLE_BAR_HT;
    const int margin = 8;

    // Default geometry mirrors the original Enemy Nations startup arrangement:
    // a narrow LEFT column holds the world/radar minimap (top) with the unit
    // lists stacked below it, and the area map fills the remaining width on the
    // RIGHT. (The shipped game put the world map at x=0 over the left ~1/5 of the
    // screen and the area map from there to the right edge.)
    const int leftW    = 320;
    const int leftX    = margin;
    const int topY     = tb + margin;
    const int mapH     = 300;
    const int listY    = topY + mapH + margin;
    const int listSpan = (H - listY - margin);
    const int vehH     = (listSpan - margin) / 2;   // split the column below the
    const int bldgY    = listY + vehH + margin;     // minimap between the two lists
    const int bldgH    = (H - bldgY - margin);

    const int areaX    = leftX + leftW + margin;     // area map starts right of the column

    int areaCascade = 0;  // offset successive area maps so they don't fully overlap

    for (const auto& up : m_panels) {
        SDL2Panel* p = up.get();
        const std::string& n = p->GetName();

        int nx, ny, nw, nh;
        if (n == "area_bar") {
            // Repositioned automatically by the area's resize callback below.
            continue;
        } else if (n.rfind("area", 0) == 0) {
            nx = areaX + areaCascade;
            ny = topY  + areaCascade;
            nw = (W - areaX - margin) - areaCascade;
            nh = (H - ny - margin);
            areaCascade += 30;
        } else if (n == "world" || n == "radar") {
            nx = leftX; ny = topY;  nw = leftW; nh = mapH;
        } else if (n == "vehicles") {
            nx = leftX; ny = listY; nw = leftW; nh = vehH;
        } else if (n == "buildings") {
            nx = leftX; ny = bldgY; nw = leftW; nh = bldgH;
        } else {
            continue;  // toolbar/status bar and anything else: leave alone
        }

        if (nw < SDL2Panel::MIN_WIDTH)  nw = SDL2Panel::MIN_WIDTH;
        if (nh < SDL2Panel::MIN_HEIGHT) nh = SDL2Panel::MIN_HEIGHT;

        // Size first (rebuilds the backing surface; the detached-window
        // SIZE_CHANGED event fires the resize callback that rebuilds the map
        // DIB), then move into place.
        p->SetSize(nw, nh);
        p->SetPosition(nx, ny);
    }

    InvalidateAll();
}

// Full window-space footprint a non-detached panel's Render() touches: content rect
// plus its optional title bar (above m_y) and 1px raised border, clamped to the window.
// Used to upload only the changed strip instead of the whole 2560x1440 surface.
static SDL_Rect PanelFootprint(SDL2Panel* p, SDL_Surface* ws) {
    SDL_Rect r    = { p->GetX() - 1, p->GetTotalY() - 1,
                      p->GetWidth() + 2, p->GetTotalHeight() + 2 };
    SDL_Rect full = { 0, 0, ws->w, ws->h };
    SDL_Rect out;
    if (SDL_IntersectRect(&r, &full, &out) != SDL_TRUE)
        out.x = out.y = out.w = out.h = 0;
    return out;
}

void SDL2Compositor::Composite() {
    if (!m_window || !m_window->GetWindow())
        return;

    SDL_Surface* windowSurface = m_window->GetPresentSurface();  // T0: software surface or renderer back-buffer
    if (!windowSurface)
        return;

    // Check if any panel is dirty
    bool anyDirty = m_backgroundDirty;
    if (!anyDirty) {
        for (auto& p : m_panels) {
            if (p->IsVisible() && p->IsDirty()) {
                anyDirty = true;
                break;
            }
        }
    }

    if (!anyDirty)
        return;  // Nothing changed, skip composition

    // Step 1: Re-sort panels (z-orders may have changed during Draw)
    SortPanels();

#ifndef _WIN32
    if (getenv("EN_DIAG")) {
        static int s_diagN = 0;
        if (s_diagN++ < 3) {
            fprintf(stderr, "[DIAG] Composite win=%p surf=%dx%d panels=%zu bgDirty=%d\n",
                    (void*)m_window, windowSurface->w, windowSurface->h, m_panels.size(), (int)m_backgroundDirty);
            for (auto& p : m_panels)
                fprintf(stderr, "[DIAG]   panel '%s' xywh=%d,%d,%d,%d vis=%d det=%d dirty=%d surf=%p\n",
                        p->GetName().c_str(), p->GetX(), p->GetY(), p->GetWidth(), p->GetHeight(),
                        (int)p->IsVisible(), (int)p->IsDetached(), (int)p->IsDirty(), (void*)p->GetSurface());
        }
    }
#endif

    // Steps 2-4: composite the non-detached panels (toolbar/status chrome) onto the main
    // window. The wallpaper + static panels rarely change; only the toolbar repaints every
    // frame (it marks itself dirty for its live resource/status text). So when only panels
    // changed (background NOT dirty), skip the full-window wallpaper re-tile + the full
    // 2560x1440 GPU upload: re-blit only the dirty panels and upload just their bounding
    // rect. The persistent back-texture keeps the rest. (Was ~12ms/frame, p.mainwin.)
    if (m_backgroundDirty) {
        Perf::ScopeCounter _cm( "p.mainwin" );
        RenderWallpaper(windowSurface);
        m_backgroundDirty = false;
        for (auto& p : m_panels)
            if (p->IsVisible() && !p->IsDetached())
                p->Render(windowSurface);
        m_window->PresentSurface();   // full upload (back-buffer may also be fresh)
    } else {
        // Incremental: only the dirty non-detached panels need re-blitting; the rest of the
        // main window (wallpaper + unchanged panels) is already in the back-buffer/texture.
        // Non-detached chrome (toolbar, status bar) doesn't overlap, so re-blitting only the
        // dirty ones is safe.
        SDL_Rect uni = { 0, 0, 0, 0 };
        bool have = false;
        for (auto& p : m_panels) {
            if (p->IsVisible() && !p->IsDetached() && p->IsDirty()) {
                SDL_Rect f = PanelFootprint(p.get(), windowSurface);
                if (f.w > 0 && f.h > 0) {
                    if (!have) { uni = f; have = true; }
                    else SDL_UnionRect(&uni, &f, &uni);
                }
                p->Render(windowSurface);   // opaque blit over its own rect (clears m_dirty)
            }
        }
        // If only a DETACHED panel changed (area map/radar — handled in step 5 below), the
        // main window is unchanged, so skip its upload + present entirely.
        if (have) {
            Perf::ScopeCounter _cm( "p.mainwin" );
            m_window->PresentSurface( &uni );
        }
    }

    // Step 5: Render detached panels to their own windows — but only the ones
    // whose own content actually changed. A detached panel lives in its own OS
    // window, so it has no reason to re-present just because a *different* panel
    // (e.g. the always-dirty toolbar) changed. Without this gate, opening the
    // vehicle/building list re-blits + presents its window every game frame,
    // which visibly drags the framerate down. SetDirty() is raised on content
    // change, resize, move, and EXPOSED/FOCUS_GAINED, so an uncovered window
    // still repaints correctly. RenderDetached() clears the flag.
    // Animated water needs the GPU-terrain window re-rendered when its WAVE FRAME
    // changes — but only THEN, not every frame. The wave advances every 6 game-
    // frames (the engine's water Time()=6; see SDL2Terrain::WaterFrameLetter), so
    // forcing a full mesh rebuild every frame (esp. zoomed out, ~20k hexes) tanked
    // the FPS for no visible gain. Re-render at the wave rate (~4×/s) instead.
    static unsigned s_lastWaterTick = ~0u;
    const unsigned  kWaterHold = 6;
    unsigned        waterTick  = (unsigned)( theGame.GetFrame() / kWaterHold );
    bool            waterChanged = ( waterTick != s_lastWaterTick );
    s_lastWaterTick = waterTick;

    DWORD nowMs = GetTickCount();
    for (auto& p : m_panels) {
        if (!p->IsVisible() || !p->IsDetached())
            continue;
        if (p->HasGpuTerrain()) {
            // Gameplay view: re-render on content change, a water wave-tick, OR while a
            // build/rocket placement cursor is active. That cursor's striped hatch is a
            // LIVE per-frame overlay (DrawBuildCursorOverlay) — if we stop re-rendering
            // while the mouse is held still, it freezes on whatever animation phase the
            // last frame happened to draw. For a small / zoomed-out footprint that phase
            // can be an all-"off" band parity, so the cursor "disappears until you move
            // the mouse" (and winks out as altitude/pan shift the footprint's screen Y).
            // The 1996 game redrew the cursor area every frame; mirror that here. Cheap:
            // a static view takes the cached-blit path, only the overlay is re-emitted.
            if (p->IsDirty() || waterChanged || theMap.HaveBldgCur())
                p->RenderDetached();
        } else if (p->IsDirty() && ( nowMs - p->GetLastRenderMs() ) >= 100) {
            // Secondary windows (World Map/radar, vehicle/building lists) don't need
            // a fast refresh — each owns a separate GPU window whose present is
            // ~5-7 ms, so re-presenting every frame is pure overhead. Cap at ~10 fps.
            p->RenderDetached();
        }
    }
}

void SDL2Compositor::InvalidateAll() {
    m_backgroundDirty = true;
    for (auto& p : m_panels) {
        p->SetDirty();
    }
}

bool SDL2Compositor::RouteEvent(SDL_Event& event) {
    // Guard: defer RemovePanel while we're iterating m_panels.
    m_routingDepth++;

    bool result = RouteEventInner(event);

    m_routingDepth--;
    if (m_routingDepth == 0)
        FlushPendingRemovals();
    return result;
}

bool SDL2Compositor::RouteEventInner(SDL_Event& event) {
    // Check if this event targets a detached panel's own window.
    uint32_t eventWindowID = 0;
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
        eventWindowID = event.button.windowID; break;
    case SDL_MOUSEMOTION:
        eventWindowID = event.motion.windowID; break;
    case SDL_MOUSEWHEEL:
        eventWindowID = event.wheel.windowID; break;
    case SDL_WINDOWEVENT:
        eventWindowID = event.window.windowID; break;
    case SDL_KEYDOWN: case SDL_KEYUP:
        eventWindowID = event.key.windowID; break;
    }

    // While a detached panel is being manually resized or dragged it has
    // captured the mouse — route all mouse events to it regardless of the
    // reported window ID.
    if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONUP ||
        event.type == SDL_MOUSEBUTTONDOWN) {
        for (auto& p : m_panels) {
            if (p->IsDetachedResizing() && p->HandleDetachedResize(event))
                return true;
            if (p->IsDetachedDragging() && p->HandleDetachedDrag(event))
                return true;
        }
    }

    if (eventWindowID) {
        for (auto& p : m_panels) {
            if (p->IsDetached() && p->GetOwnWindowID() == eventWindowID) {
                // Linux/XWayland: clicking a detached panel must RAISE it. Under
                // Mutter (GNOME-on-Wayland, game on XWayland) these borderless
                // SKIP_TASKBAR panels do not auto-raise on click, so a buried
                // area-map/radar could not be brought to the top (operator-reported
                // "can no longer be put into the top"). Raise on any button-press so
                // the panel comes forward BEFORE drag/content handling (also makes
                // a title-bar grab on a buried panel act on the now-top window).
                // Win (GWLP_HWNDPARENT) / mac (ALWAYS_ON_TOP) already raise via their
                // own WM owner relationship, so this is Linux-only.
#if defined(__linux__)
                if (event.type == SDL_MOUSEBUTTONDOWN && p->GetOwnWindow())
                    SDL_RaiseWindow(p->GetOwnWindow());
#endif

                // Manual edge/corner resize takes priority over content + drag.
                if ((event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN) &&
                    p->HandleDetachedResize(event))
                    return true;

                // Title-bar press (not on an edge/caption button) begins a manual
                // window drag. Checked after resize so edges win. Driving the move
                // ourselves avoids the OS modal move loop that froze rendering.
                if (event.type == SDL_MOUSEBUTTONDOWN && p->HandleDetachedDrag(event))
                    return true;

                // Handle window resize/close events from the detached window
                if (event.type == SDL_WINDOWEVENT) {
                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        int newW = event.window.data1;
                        int newH = event.window.data2 - p->GetTitleBarHeight();
                        if (newW > 0 && newH > 0) {
                            p->SetSize(newW, newH);
                            p->InvokeResizeCallback(newW, newH);
                        }
                        p->SetDirty();
                    }
                    else if (event.window.event == SDL_WINDOWEVENT_MOVED) {
                        // OS dragged the borderless window (possibly to another
                        // monitor). Sync the stored content origin, remember the
                        // placement for reopen, and re-sync the backing MFC
                        // window so selection / hit-testing stays aligned.
                        p->OnOwnWindowMoved();
                        p->SetDirty();
                    }
                    else if (event.window.event == SDL_WINDOWEVENT_EXPOSED ||
                             event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                        p->SetDirty();  // force a repaint of the detached window
                    }
                    else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        // HIDE the view (don't re-dock, don't quit): the [X] just puts
                        // it away. It reopens from its toolbar button / status bar, which
                        // already call SetVisible(true) (GotoArea / GotoWorld /
                        // ToggleUnitListPanel). Hiding keeps the detached window + its GPU
                        // renderer alive, so reopening is instant and intact — whereas
                        // Attach() destroyed the window+renderer and left a broken facade.
                        p->SetVisible(false);
                    }
                    return true;
                }

                // Route mouse events through the full HandleEvent (which has
                // resize/drag/cursor logic) by temporarily placing the panel
                // at its detached-window-local coordinates.
                // Grab the raw pointer — HandleEvent may trigger AddPanel(),
                // which can reallocate m_panels and invalidate the reference.
                SDL2Panel* panel = p.get();
                int tbH = panel->GetTitleBarHeight();
                int savedX = panel->GetX(), savedY = panel->GetY();
                panel->SuppressWindowSync(true);
                panel->SetPosition(0, tbH);
                bool consumed = panel->HandleEvent(event);
                panel->SetPosition(savedX, savedY);
                panel->SuppressWindowSync(false);
                return consumed;
            }
        }
    }

    // If a panel is actively being dragged/resized, it gets ALL mouse events
    // regardless of z-order or hit-test. This prevents other panels from
    // stealing drag events.
    if (m_activePanel) {
        if (m_activePanel->HandleEvent(event)) {
            // Check if drag/resize ended
            if (!m_activePanel->IsDragging() && !m_activePanel->IsResizing())
                m_activePanel = nullptr;
            return true;
        }
        m_activePanel = nullptr;  // Panel didn't handle it, clear
    }

    // Route events top-down (highest z-order first) for non-detached panels.
    SDL2Panel* hitPanel = nullptr;
    for (int i = (int)m_panels.size() - 1; i >= 0; i--) {
        if (m_panels[i]->IsVisible() && !m_panels[i]->IsDetached() && m_panels[i]->HandleEvent(event)) {
            hitPanel = m_panels[i].get();
            break;
        }
    }

    if (hitPanel) {
        if (hitPanel->IsDragging() || hitPanel->IsResizing())
            m_activePanel = hitPanel;

        if (event.type == SDL_MOUSEBUTTONDOWN)
            BringToFront(hitPanel);

        return true;
    }

    // GLOBAL GAME-HOTKEY FALLBACK (restores the original's app-wide accelerators).
    // SDL stamps keyboard events with its keyboard-FOCUS window — not the window the
    // Win32 message targeted — so when focus sits on a window with no key bindings
    // (radar/world map, the main Game View chrome) or on no window at all (the game
    // driven from the background by the test harness), the gameplay keys would die in
    // the windowID routing above and never reach the Area Map. Deliver any key event
    // nothing else consumed to the TOPMOST visible area panel (pan/rotate/zoom/unit
    // hotkeys). Full chain, in pump order: non-modal dialogs (GameWindow::PollEvents)
    // → app-wide accelerators (HandleGlobalShortcut: Esc/F1/F2/Ctrl+letter — the main
    // window's bottom-toolbar commands) → the focused window (routing above) → area
    // map (here). The radar's 4 buttons are mouse-only today; if they ever get
    // hotkeys, handle them in its callback BEFORE this fallback fires.
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        SDL2Panel* area = nullptr;
        for (auto& p : m_panels) {
            // Map panels are named "area_<index>" (area.cpp OnCreate). NOTE: "area_bar" (the
            // button bar INSIDE the map window, z = map+1, mouse-only) must not match, or the
            // fallback hands the key to a panel that ignores it and the map never sees it.
            const std::string& nm = p->GetName();
            if (p->IsVisible() && nm.size() >= 6 && nm.rfind("area_", 0) == 0 &&
                nm[5] >= '0' && nm[5] <= '9' &&
                (!area || p->GetZOrder() > area->GetZOrder()))
                area = p.get();
        }
        if (area && area->HandleEvent(event))   // HandleEvent forwards keys straight to the content callback
            return true;
    }
    return false;
}

void SDL2Compositor::BringToFront(SDL2Panel* panel) {
    if (!panel) return;

    // Reassign z-orders: panel gets the max, others keep relative order.
    // This keeps z-orders bounded (0..N).
    int maxZ = 0;
    for (auto& p : m_panels)
        if (p->GetZOrder() > maxZ) maxZ = p->GetZOrder();

    if (panel->GetZOrder() >= maxZ)
        return;  // Already on top

    panel->SetZOrder(maxZ + 1);

    // Compact z-orders to prevent unbounded growth
    // Reassign 0..N based on current sort order
    SortPanels();
    for (int i = 0; i < (int)m_panels.size(); i++)
        m_panels[i]->SetZOrder(i);

    m_backgroundDirty = true;
}

SDL2Panel* SDL2Compositor::GetPanel(int index) const {
    if (index < 0 || index >= (int)m_panels.size())
        return nullptr;
    return m_panels[index].get();
}

void SDL2Compositor::RenderWallpaper(SDL_Surface* windowSurface) {
    if (!m_wallpaper) {
        // No wallpaper loaded — fill with dark gold as fallback
        Uint32 color = SDL_MapRGB(windowSurface->format, 128, 100, 40);
        SDL_FillRect(windowSurface, nullptr, color);
        return;
    }

    // Tile the wallpaper across the entire window surface, matching
    // the original CWndMain::OnPaint() behavior
    int wallW = m_wallpaper->w;
    int wallH = m_wallpaper->h;
    int winW = windowSurface->w;
    int winH = windowSurface->h;

    if (wallW <= 0 || wallH <= 0)
        return;

    for (int y = 0; y < winH; y += wallH) {
        for (int x = 0; x < winW; x += wallW) {
            SDL_Rect srcRect = { 0, 0, wallW, wallH };
            SDL_Rect dstRect = { x, y, wallW, wallH };

            // Clip source rect if it would extend past window edge
            if (x + wallW > winW)
                srcRect.w = winW - x;
            if (y + wallH > winH)
                srcRect.h = winH - y;

            dstRect.w = srcRect.w;
            dstRect.h = srcRect.h;

            SDL_BlitSurface(m_wallpaper, &srcRect, windowSurface, &dstRect);
        }
    }
}

void SDL2Compositor::SortPanels() {
    std::stable_sort(m_panels.begin(), m_panels.end(),
        [](const std::unique_ptr<SDL2Panel>& a, const std::unique_ptr<SDL2Panel>& b) {
            return a->GetZOrder() < b->GetZOrder();
        });
}
