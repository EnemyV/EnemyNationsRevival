#include "stdafx.h"

#include "SDL2Panel.h"
#include "GameWindow.h"

#include <SDL.h>
#include <SDL_ttf.h>
#ifdef _WIN32
#include <SDL_syswm.h>
#endif
#include <fstream>

static void LogPanel(const std::string& msg) {
    std::ofstream log("SDL2Panel.log", std::ios::app);
    if (log.is_open()) {
        log << msg << std::endl;
    }
}

// Title bar colors
static const SDL_Color TitleBg     = { 50,  55,  52,  255 };
static const SDL_Color TitleBgActive = { 70,  80,  90,  255 };
static const SDL_Color TitleText   = { 220, 210, 200, 255 };
static const SDL_Color BorderLight = { 103, 127, 121, 255 };
static const SDL_Color BorderDark  = { 38,  46,  49,  255 };

static void FillRectP(SDL_Surface* dst, SDL_Rect r, SDL_Color c) {
    SDL_FillRect(dst, &r, SDL_MapRGB(dst->format, c.r, c.g, c.b));
}

SDL2Panel::SDL2Panel(const std::string& name, int x, int y, int w, int h, int zOrder)
    : m_name(name)
    , m_title(name)
    , m_x(x)
    , m_y(y)
    , m_width(w)
    , m_height(h)
    , m_zOrder(zOrder)
    , m_visible(true)
    , m_dirty(true)
    , m_surface(nullptr)
{
    CreateSurface();
}

SDL2Panel::~SDL2Panel() {
    DestroyOwnWindow();
    FreeSurface();
}

void SDL2Panel::CreateSurface() {
    FreeSurface();
    if (m_width <= 0 || m_height <= 0)
        return;

    m_surface = SDL_CreateRGBSurface(
        0, m_width, m_height, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0);

    if (!m_surface) {
        LogPanel("ERROR: Failed to create surface for panel '" + m_name + "': " + SDL_GetError());
    }
}

void SDL2Panel::FreeSurface() {
    if (m_surface) {
        SDL_FreeSurface(m_surface);
        m_surface = nullptr;
    }
}

void SDL2Panel::Render(SDL_Surface* windowSurface) {
    if (!m_visible || !m_surface || !windowSurface)
        return;

    // Detached panels render via RenderDetached(), not here
    if (m_ownWindow)
        return;

    // Render title bar if movable
    if (m_movable)
        RenderTitleBar(windowSurface);

    // Blit content
    SDL_Rect dstRect = { m_x, m_y, m_width, m_height };
    SDL_BlitSurface(m_surface, nullptr, windowSurface, &dstRect);

    // Draw border if movable/resizable
    if (m_movable || m_resizable) {
        int totalY = GetTotalY();
        int totalH = GetTotalHeight();
        SDL_Rect borderRect = { m_x - 1, totalY - 1, m_width + 2, totalH + 2 };
        // Top
        FillRectP(windowSurface, {borderRect.x, borderRect.y, borderRect.w, 1}, BorderLight);
        // Left
        FillRectP(windowSurface, {borderRect.x, borderRect.y, 1, borderRect.h}, BorderLight);
        // Bottom
        FillRectP(windowSurface, {borderRect.x, borderRect.y + borderRect.h - 1, borderRect.w, 1}, BorderDark);
        // Right
        FillRectP(windowSurface, {borderRect.x + borderRect.w - 1, borderRect.y, 1, borderRect.h}, BorderDark);
    }

    m_dirty = false;
}

// Lazily cached font for title bars
static TTF_Font* GetTitleFont() {
    static TTF_Font* s_font = nullptr;
    static bool s_tried = false;
    if (s_tried) return s_font;
    s_tried = true;
    const char* paths[] = {
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf",
        nullptr
    };
    for (int i = 0; paths[i]; i++) {
        s_font = TTF_OpenFont(paths[i], 12);
        if (s_font) break;
    }
    return s_font;
}

void SDL2Panel::RenderTitleBar(SDL_Surface* windowSurface) {
    int tbY = m_y - TITLE_BAR_HT;
    SDL_Rect tbRect = { m_x, tbY, m_width, TITLE_BAR_HT };
    FillRectP(windowSurface, tbRect, m_dragging ? TitleBgActive : TitleBg);

    // Render title text
    TTF_Font* font = GetTitleFont();
    if (font && !m_title.empty()) {
        SDL_Surface* textSurf = TTF_RenderText_Blended(font, m_title.c_str(), TitleText);
        if (textSurf) {
            SDL_Rect textDst = { m_x + 4, tbY + (TITLE_BAR_HT - textSurf->h) / 2,
                                 textSurf->w, textSurf->h };
            // Clip to title bar width (leave room for close button)
            int maxTextW = m_width - (m_closable ? CLOSE_BTN_SIZE + 12 : 8);
            if (textDst.w > maxTextW) textDst.w = maxTextW;
            SDL_Rect textSrc = { 0, 0, textDst.w, textDst.h };
            SDL_BlitSurface(textSurf, &textSrc, windowSurface, &textDst);
            SDL_FreeSurface(textSurf);
        }
    }

    // Close button [X]
    if (m_closable) {
        int btnX = m_x + m_width - CLOSE_BTN_SIZE - 2;
        int btnY = tbY + (TITLE_BAR_HT - CLOSE_BTN_SIZE) / 2;
        SDL_Rect btnRect = { btnX, btnY, CLOSE_BTN_SIZE, CLOSE_BTN_SIZE };
        FillRectP(windowSurface, btnRect, {80, 40, 40, 255});
        // Draw X
        for (int i = 3; i < CLOSE_BTN_SIZE - 3; i++) {
            Uint32 white = SDL_MapRGB(windowSurface->format, 220, 210, 200);
            SDL_Rect px1 = { btnX + i, btnY + i, 1, 1 };
            SDL_Rect px2 = { btnX + CLOSE_BTN_SIZE - 1 - i, btnY + i, 1, 1 };
            SDL_FillRect(windowSurface, &px1, white);
            SDL_FillRect(windowSurface, &px2, white);
        }
    }
}

int SDL2Panel::HitTestResize(int screenX, int screenY) const {
    if (!m_resizable) return 0;

    int totalY = GetTotalY();
    int right = m_x + m_width;
    int bottom = m_y + m_height;

    // Resize zones straddle the panel edge: half inside, half outside.
    // This keeps them reachable even when the panel is against a screen edge.
    int halfIn  = RESIZE_BORDER / 2;
    int halfOut = RESIZE_BORDER - halfIn;

    bool onLeft   = (screenX >= m_x    - halfOut && screenX < m_x    + halfIn);
    bool onRight  = (screenX >= right  - halfIn  && screenX < right  + halfOut);
    bool onTop    = (screenY >= totalY - halfOut && screenY < totalY + halfIn);
    bool onBottom = (screenY >= bottom - halfIn  && screenY < bottom + halfOut);

    if (onTop && onLeft)   return 8;  // NW
    if (onTop && onRight)  return 2;  // NE
    if (onBottom && onLeft)  return 6;  // SW
    if (onBottom && onRight) return 4;  // SE
    if (onTop)    return 1;  // N
    if (onRight)  return 3;  // E
    if (onBottom) return 5;  // S
    if (onLeft)   return 7;  // W
    return 0;
}

bool SDL2Panel::HandleEvent(SDL_Event& event) {
    if (!m_visible)
        return false;

    int screenX = 0, screenY = 0;

    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        screenX = event.button.x;
        screenY = event.button.y;
        break;
    case SDL_MOUSEMOTION:
        screenX = event.motion.x;
        screenY = event.motion.y;
        break;
    case SDL_MOUSEWHEEL:
        SDL_GetMouseState(&screenX, &screenY);
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        if (m_eventCallback)
            return m_eventCallback(event, 0, 0);
        return false;
    default:
        return false;
    }

    // Handle ongoing drag/resize — consume ALL mouse events
    if (m_dragging) {
        if (event.type == SDL_MOUSEMOTION) {
            SetPosition(screenX - m_dragOffX, screenY - m_dragOffY);
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            m_dragging = false;
        }
        return true;  // Always consume during drag
    }

    if (m_resizing) {
        if (event.type == SDL_MOUSEMOTION) {
            int dx = screenX - m_resizeStartX;
            int dy = screenY - m_resizeStartY;
            int newX = m_resizeOrigX, newY = m_resizeOrigY;
            int newW = m_resizeOrigW, newH = m_resizeOrigH;

            if (m_resizeEdge == 3 || m_resizeEdge == 4 || m_resizeEdge == 2)  // E, SE, NE
                newW = m_resizeOrigW + dx;
            if (m_resizeEdge == 7 || m_resizeEdge == 6 || m_resizeEdge == 8) { // W, SW, NW
                newW = m_resizeOrigW - dx;
                newX = m_resizeOrigX + dx;
            }
            if (m_resizeEdge == 5 || m_resizeEdge == 4 || m_resizeEdge == 6)  // S, SE, SW
                newH = m_resizeOrigH + dy;
            if (m_resizeEdge == 1 || m_resizeEdge == 2 || m_resizeEdge == 8) { // N, NE, NW
                newH = m_resizeOrigH - dy;
                newY = m_resizeOrigY + dy;
            }

            if (newW < MIN_WIDTH) newW = MIN_WIDTH;
            if (newH < MIN_HEIGHT) newH = MIN_HEIGHT;

            SetRect(newX, newY, newW, newH);
            if (m_resizeCallback)
                m_resizeCallback(newW, newH);
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            m_resizing = false;
        }
        return true;  // Always consume during resize
    }

    // Hit-test for mouse events (include title bar and resize borders)
    bool inContent = (screenX >= m_x && screenX < m_x + m_width &&
                      screenY >= m_y && screenY < m_y + m_height);
    bool inTitleBar = false;
    if (m_movable) {
        int tbY = m_y - TITLE_BAR_HT;
        inTitleBar = (screenX >= m_x && screenX < m_x + m_width &&
                      screenY >= tbY && screenY < m_y);
    }
    int resizeEdge = HitTestResize(screenX, screenY);

    if (!inContent && !inTitleBar && resizeEdge == 0)
        return false;

    // Close button click
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT && m_closable && inTitleBar) {
        int btnX = m_x + m_width - CLOSE_BTN_SIZE - 2;
        int btnY = (m_y - TITLE_BAR_HT) + (TITLE_BAR_HT - CLOSE_BTN_SIZE) / 2;
        if (screenX >= btnX && screenX < btnX + CLOSE_BTN_SIZE &&
            screenY >= btnY && screenY < btnY + CLOSE_BTN_SIZE) {
            if (m_closeCallback)
                m_closeCallback();
            else
                SetVisible(false);
            return true;
        }
    }

    // Show resize cursor when hovering over a resize edge,
    // but not when inside the title bar (title bar wins for dragging).
    if (event.type == SDL_MOUSEMOTION && resizeEdge > 0 && !inTitleBar) {
        static SDL_Cursor* s_sizeWE   = nullptr;
        static SDL_Cursor* s_sizeNS   = nullptr;
        static SDL_Cursor* s_sizeNWSE = nullptr;
        static SDL_Cursor* s_sizeNESW = nullptr;
        if (!s_sizeWE) {
            s_sizeWE   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
            s_sizeNS   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
            s_sizeNWSE = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
            s_sizeNESW = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
        }
        switch (resizeEdge) {
            case 7: case 3: SDL_SetCursor(s_sizeWE);   break;  // W, E
            case 1: case 5: SDL_SetCursor(s_sizeNS);   break;  // N, S
            case 8: case 4: SDL_SetCursor(s_sizeNWSE); break;  // NW, SE
            case 2: case 6: SDL_SetCursor(s_sizeNESW); break;  // NE, SW
        }
        return true;  // consume — don't send resize-edge hover to content
    }

    // Start drag on title bar click
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (resizeEdge > 0 && !inTitleBar) {
            m_resizing = true;
            m_resizeEdge = resizeEdge;
            m_resizeStartX = screenX;
            m_resizeStartY = screenY;
            m_resizeOrigX = m_x;
            m_resizeOrigY = m_y;
            m_resizeOrigW = m_width;
            m_resizeOrigH = m_height;
            return true;
        }
        if (inTitleBar) {
            m_dragging = true;
            m_dragOffX = screenX - m_x;
            m_dragOffY = screenY - m_y;
            return true;
        }
    }

    // Content area events go to the callback
    if (inContent) {
        int localX = screenX - m_x;
        int localY = screenY - m_y;

        if (m_eventCallback)
            return m_eventCallback(event, localX, localY);
    }

    return true;  // Consumed by hit-test
}

bool SDL2Panel::HitTest(int screenX, int screenY) const {
    if (!m_visible) return false;

    // Content area
    if (screenX >= m_x && screenX < m_x + m_width &&
        screenY >= m_y && screenY < m_y + m_height)
        return true;

    // Title bar
    if (m_movable) {
        int tbY = m_y - TITLE_BAR_HT;
        if (screenX >= m_x && screenX < m_x + m_width &&
            screenY >= tbY && screenY < m_y)
            return true;
    }

    // Resize borders
    if (m_resizable && HitTestResize(screenX, screenY) > 0)
        return true;

    return false;
}

void SDL2Panel::Invalidate() {
    if (m_surface) {
        SDL_FillRect(m_surface, nullptr, SDL_MapRGB(m_surface->format, 0, 0, 0));
    }
    m_dirty = true;
}

// Clamp position so the title bar and resize borders stay on-screen.
// Called from SetPosition/SetRect to enforce this regardless of caller.
void SDL2Panel::ClampPosition(int& x, int& y) const {
    if (m_ownWindow)
        return;  // detached panels are positioned by the OS
    if (!m_movable && !m_resizable)
        return;  // fixed panels don't need clamping
    if (x < RESIZE_BORDER)
        x = RESIZE_BORDER;
    int minY = (m_movable ? TITLE_BAR_HT : 0) + RESIZE_BORDER;
    if (y < minY)
        y = minY;
}

void SDL2Panel::SetPosition(int x, int y) {
    ClampPosition(x, y);
    m_x = x;
    m_y = y;
    if (m_ownWindow && !m_suppressSync)
        SDL_SetWindowPosition(m_ownWindow, x, y - GetTitleBarHeight());
    m_dirty = true;
}

void SDL2Panel::SetSize(int w, int h) {
    if (w == m_width && h == m_height)
        return;
    m_width = w;
    m_height = h;
    CreateSurface();
    if (m_ownWindow && !m_suppressSync)
        SDL_SetWindowSize(m_ownWindow, w, h + GetTitleBarHeight());
    m_dirty = true;
}

void SDL2Panel::SetRect(int x, int y, int w, int h) {
    ClampPosition(x, y);
    m_x = x;
    m_y = y;
    if (w != m_width || h != m_height) {
        m_width = w;
        m_height = h;
        CreateSurface();
    }
    if (m_ownWindow && !m_suppressSync) {
        SDL_SetWindowPosition(m_ownWindow, x, y - GetTitleBarHeight());
        SDL_SetWindowSize(m_ownWindow, w, h + GetTitleBarHeight());
    }
    m_dirty = true;
}

// ---------------------------------------------------------------------------
// Detached window support
// ---------------------------------------------------------------------------

// Hit-test callback for detached panel windows — enables OS-level resize.
// The title bar area returns SDL_HITTEST_DRAGGABLE so the OS handles the drag.
static SDL_HitTestResult SDLCALL DetachedPanelHitTest(SDL_Window* win, const SDL_Point* pt, void* data) {
    SDL2Panel* panel = static_cast<SDL2Panel*>(data);
    int w, h;
    SDL_GetWindowSize(win, &w, &h);
    int tbH = panel->GetTitleBarHeight();
    int grip = SDL2Panel::RESIZE_BORDER;

    bool left   = pt->x < grip;
    bool right  = pt->x >= w - grip;
    bool top    = pt->y < grip;
    bool bottom = pt->y >= h - grip;

    if (top && left)     return SDL_HITTEST_RESIZE_TOPLEFT;
    if (top && right)    return SDL_HITTEST_RESIZE_TOPRIGHT;
    if (bottom && left)  return SDL_HITTEST_RESIZE_BOTTOMLEFT;
    if (bottom && right) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    if (top)             return SDL_HITTEST_RESIZE_TOP;
    if (bottom)          return SDL_HITTEST_RESIZE_BOTTOM;
    if (left)            return SDL_HITTEST_RESIZE_LEFT;
    if (right)           return SDL_HITTEST_RESIZE_RIGHT;

    // Title bar area — OS handles the drag
    if (tbH > 0 && pt->y < tbH)
        return SDL_HITTEST_DRAGGABLE;

    return SDL_HITTEST_NORMAL;
}

void SDL2Panel::Detach(GameWindow* mainWin) {
    if (m_ownWindow)
        return;  // already detached

    // Convert panel position (within main window) to global screen coords.
    // No custom title bar when using OS chrome.
    int globalX = m_x, globalY = m_y;
    if (mainWin && mainWin->GetWindow()) {
        int wx, wy;
        SDL_GetWindowPosition(mainWin->GetWindow(), &wx, &wy);
        globalX += wx;
        globalY += wy;
    }

    // Use OS window chrome (not borderless) — OS provides title bar, borders,
    // resize handles. No ALWAYS_ON_TOP; we use the Win32 owner window
    // relationship so the panel always stays above the background window.
    m_ownWindow = GameWindow::CreateSDLWindow(
        m_title.c_str(), globalX, globalY, m_width, m_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SKIP_TASKBAR);

    if (!m_ownWindow) {
        LogPanel("ERROR: Failed to create detached window for '" + m_name + "': " + SDL_GetError());
        return;
    }

    m_ownWindowID = SDL_GetWindowID(m_ownWindow);

#ifdef _WIN32
    // Set the background window as the owner of this panel so it always
    // stays above it without needing ALWAYS_ON_TOP.
    if (mainWin && mainWin->GetWindow()) {
        SDL_SysWMinfo mainInfo, panelInfo;
        SDL_VERSION(&mainInfo.version);
        SDL_VERSION(&panelInfo.version);
        if (SDL_GetWindowWMInfo(mainWin->GetWindow(), &mainInfo) &&
            SDL_GetWindowWMInfo(m_ownWindow, &panelInfo)) {
            ::SetWindowLongPtr(panelInfo.info.win.window, GWLP_HWNDPARENT,
                               (LONG_PTR)mainInfo.info.win.window);
        }
    }
#endif

    // Raise above the main window
    SDL_RaiseWindow(m_ownWindow);

    LogPanel("Detached panel '" + m_name + "' to own window (ID=" + std::to_string(m_ownWindowID) + ")");
    m_dirty = true;
}

void SDL2Panel::Detach(SDL_Window* ownerWindow) {
    if (m_ownWindow)
        return;  // already detached

    // Position at the panel's current compositor coordinates (global screen).
    int globalX = m_x, globalY = m_y;
    if (ownerWindow) {
        int wx, wy;
        SDL_GetWindowPosition(ownerWindow, &wx, &wy);
        globalX += wx;
        globalY += wy;
    }

    m_ownWindow = GameWindow::CreateSDLWindow(
        m_title.c_str(), globalX, globalY, m_width, m_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SKIP_TASKBAR);

    if (!m_ownWindow) {
        LogPanel("ERROR: Failed to create detached window for '" + m_name + "': " + SDL_GetError());
        return;
    }

    m_ownWindowID = SDL_GetWindowID(m_ownWindow);

#ifdef _WIN32
    // Set the given window as the owner so this panel floats above it.
    if (ownerWindow) {
        SDL_SysWMinfo ownerInfo, panelInfo;
        SDL_VERSION(&ownerInfo.version);
        SDL_VERSION(&panelInfo.version);
        if (SDL_GetWindowWMInfo(ownerWindow, &ownerInfo) &&
            SDL_GetWindowWMInfo(m_ownWindow, &panelInfo)) {
            ::SetWindowLongPtr(panelInfo.info.win.window, GWLP_HWNDPARENT,
                               (LONG_PTR)ownerInfo.info.win.window);
        }
    }
#endif

    SDL_RaiseWindow(m_ownWindow);
    LogPanel("Detached panel '" + m_name + "' owned by external window (ID=" + std::to_string(m_ownWindowID) + ")");
    m_dirty = true;
}

void SDL2Panel::Attach(GameWindow* mainWin) {
    if (!m_ownWindow)
        return;

    // Convert global screen position back to main-window-relative coords
    int wx = 0, wy = 0;
    if (mainWin && mainWin->GetWindow()) {
        SDL_GetWindowPosition(mainWin->GetWindow(), &wx, &wy);
    }
    int gx, gy;
    SDL_GetWindowPosition(m_ownWindow, &gx, &gy);
    m_x = gx - wx;
    m_y = gy - wy;  // No custom title bar offset — detached uses OS chrome

    DestroyOwnWindow();
    LogPanel("Attached panel '" + m_name + "' back to compositor");
    m_dirty = true;
}

void SDL2Panel::DestroyOwnWindow() {
    if (m_ownWindow) {
        SDL_DestroyWindow(m_ownWindow);
        m_ownWindow = nullptr;
        m_ownWindowID = 0;
    }
}

void SDL2Panel::RenderDetached() {
    if (!m_ownWindow || !m_visible || !m_surface)
        return;

    SDL_Surface* winSurf = SDL_GetWindowSurface(m_ownWindow);
    if (!winSurf)
        return;

    // Detached windows use OS chrome (title bar, borders) — no custom title bar.
    // Save panel position, temporarily set to (0,0) for rendering into own window
    int savedX = m_x, savedY = m_y;
    m_x = 0;
    m_y = 0;

    // Clear background
    SDL_FillRect(winSurf, nullptr, SDL_MapRGB(winSurf->format, 0, 0, 0));

    // Blit content directly (OS provides title bar)
    SDL_Rect dstRect = { 0, 0, m_width, m_height };
    SDL_BlitSurface(m_surface, nullptr, winSurf, &dstRect);

    // Restore position
    m_x = savedX;
    m_y = savedY;

    SDL_UpdateWindowSurface(m_ownWindow);
    m_dirty = false;
}

bool SDL2Panel::HandleDetachedEvent(SDL_Event& event, int localX, int localY) {
    if (m_eventCallback)
        return m_eventCallback(event, localX, localY);
    return false;
}
