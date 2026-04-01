#include "stdafx.h"

#include "SDL2Compositor.h"
#include "SDL2Panel.h"
#include "GameWindow.h"
#include "SDL2MainMenu.h"  // For CreateSurfaceFromDIB
#include "lastplnt.h"      // For theDataFile, ptrthebltformat
#include "bmbutton.h"      // Must precede bitmaps.h (provides CBmBtnData)
#include "bitmaps.h"       // For DIB_GOLD, theBitmaps

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

void SDL2Compositor::Composite() {
    if (!m_window || !m_window->GetWindow())
        return;

    SDL_Surface* windowSurface = SDL_GetWindowSurface(m_window->GetWindow());
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

    // Step 2: Render tiled wallpaper background
    RenderWallpaper(windowSurface);
    m_backgroundDirty = false;

    // Step 3: Blit non-detached panels in z-order (sorted ascending)
    for (auto& p : m_panels) {
        if (p->IsVisible() && !p->IsDetached()) {
            p->Render(windowSurface);
        }
    }

    // Step 4: Present the main window
    SDL_UpdateWindowSurface(m_window->GetWindow());

    // Step 5: Render detached panels to their own windows
    for (auto& p : m_panels) {
        if (p->IsVisible() && p->IsDetached()) {
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

    if (eventWindowID) {
        for (auto& p : m_panels) {
            if (p->IsDetached() && p->GetOwnWindowID() == eventWindowID) {
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
                    else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        p->Attach();  // re-dock on close
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
