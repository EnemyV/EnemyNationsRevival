#include "stdafx.h"

#include "SDL2Panel.h"
#include "GameWindow.h"
#include "SDL2MainMenu.h"  // CreateSurfaceFromDIB
#include "bmbutton.h"      // must precede bitmaps.h (provides CBmBtnData)
#include "bitmaps.h"       // theBitmaps, DIB_GOLD, DIB_BORDER_HORZ/VERT

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

// ---------------------------------------------------------------------------
// Enemy Nations purple title-bar palette (matches the original 1996 chrome:
// a horizontal violet gradient caption with white text and Win95 caption
// buttons, framed by a thin beveled border).
// ---------------------------------------------------------------------------
static const SDL_Color TitleGradL  = {  58,  46, 124, 255 };  // caption gradient, left  (blue-violet)
static const SDL_Color TitleGradR  = { 120, 100, 178, 255 };  // caption gradient, right (periwinkle violet)
static const SDL_Color TitleText   = { 248, 246, 252, 255 };  // caption text (near-white)
static const SDL_Color BorderLight = { 178, 156, 210, 255 };  // (composited) bevel highlight
static const SDL_Color BorderDark  = {  36,  22,  60, 255 };  // (composited) bevel shadow
// Raised 3D window frame (Win9x silver sizing border) for detached windows.
static const SDL_Color FrameFace   = { 212, 208, 200, 255 };
static const SDL_Color FrameHi     = { 255, 255, 255, 255 };
static const SDL_Color FrameSh     = { 128, 128, 128, 255 };
static const SDL_Color FrameDk     = {  64,  64,  64, 255 };
// Win98 caption-button colors (classic 3D double bevel)
static const SDL_Color BtnFace     = { 212, 208, 200, 255 };
static const SDL_Color BtnHi       = { 255, 255, 255, 255 };  // outer top/left
static const SDL_Color BtnLt       = { 223, 223, 223, 255 };  // inner top/left
static const SDL_Color BtnSh       = { 128, 128, 128, 255 };  // inner bottom/right
static const SDL_Color BtnDk       = {  64,  64,  64, 255 };  // outer bottom/right
static const SDL_Color BtnGlyph    = {   0,   0,   0, 255 };

static void FillRectP(SDL_Surface* dst, SDL_Rect r, SDL_Color c) {
    SDL_FillRect(dst, &r, SDL_MapRGB(dst->format, c.r, c.g, c.b));
}

// Horizontal left→right gradient fill (one 1px column per step).
static void FillGradientH(SDL_Surface* dst, SDL_Rect r, SDL_Color l, SDL_Color rgt) {
    if (r.w <= 0 || r.h <= 0) return;
    for (int i = 0; i < r.w; i++) {
        float t = (r.w > 1) ? (float)i / (float)(r.w - 1) : 0.0f;
        Uint8 cr = (Uint8)(l.r + (rgt.r - l.r) * t);
        Uint8 cg = (Uint8)(l.g + (rgt.g - l.g) * t);
        Uint8 cb = (Uint8)(l.b + (rgt.b - l.b) * t);
        SDL_Rect col = { r.x + i, r.y, 1, r.h };
        SDL_FillRect(dst, &col, SDL_MapRGB(dst->format, cr, cg, cb));
    }
}

// Raised 3D window frame, `th` px thick, drawn as an overlay on the window edge:
// a silver face band with an outer light/dark bevel and an inner sunken edge —
// matching the chunky border around the original Enemy Nations windows.
static void DrawRaisedBorder(SDL_Surface* dst, int x, int y, int w, int h, int th) {
    if (w <= 2 * th || h <= 2 * th) return;
    // Frame ring (face)
    FillRectP(dst, { x, y, w, th }, FrameFace);
    FillRectP(dst, { x, y + h - th, w, th }, FrameFace);
    FillRectP(dst, { x, y, th, h }, FrameFace);
    FillRectP(dst, { x + w - th, y, th, h }, FrameFace);
    // Outer bevel: highlight top/left, shadow bottom/right
    FillRectP(dst, { x, y, w, 1 }, FrameHi);
    FillRectP(dst, { x, y, 1, h }, FrameHi);
    FillRectP(dst, { x + w - 1, y, 1, h }, FrameDk);
    FillRectP(dst, { x, y + h - 1, w, 1 }, FrameDk);
    // Inner sunken edge: shadow top/left, highlight bottom/right (recessed look)
    int ix = x + th - 1, iy = y + th - 1, iw = w - 2 * (th - 1), ih = h - 2 * (th - 1);
    FillRectP(dst, { ix, iy, iw, 1 }, FrameSh);
    FillRectP(dst, { ix, iy, 1, ih }, FrameSh);
    FillRectP(dst, { ix + iw - 1, iy, 1, ih }, FrameHi);
    FillRectP(dst, { ix, iy + ih - 1, iw, 1 }, FrameHi);
}

// --- Enemy Nations gold ornate border art (shared across all detached panels) -
static SDL_Surface* s_goldBg   = nullptr;
static SDL_Surface* s_borderH  = nullptr;  // horizontal border strip (top/bottom)
static SDL_Surface* s_borderV  = nullptr;  // vertical border strip (left/right)
static bool         s_goldTried = false;

static void EnsureGoldArt() {
    if (s_goldTried) return;
    s_goldTried = true;
    CDIB* pGold = theBitmaps.GetByIndex(DIB_GOLD);
    if (pGold) s_goldBg = SDL2MainMenu::CreateSurfaceFromDIB(pGold);
    CDIB* pH = theBitmaps.GetByIndex(DIB_BORDER_HORZ);
    if (pH) s_borderH = SDL2MainMenu::CreateSurfaceFromDIB(pH);
    CDIB* pV = theBitmaps.GetByIndex(DIB_BORDER_VERT);
    if (pV) s_borderV = SDL2MainMenu::CreateSurfaceFromDIB(pV);
}

static inline int IMin(int a, int b) { return a < b ? a : b; }

// Frame the rectangle (x,y,w,h) with the carved-gold Enemy Nations border art,
// mitering the corners exactly as the original PaintBorder() (bmbutton.cpp):
// the horizontal strip runs full width top & bottom; the vertical strips step
// inward 1px per column (top+ix, bottom-ix) and sample the bitmap diagonally
// (source x=ix, y=ix), so the verticals meet the horizontals in a clean 45°
// corner instead of butting/overlapping. Returns the border thickness.
static int DrawGoldFrame(SDL_Surface* dst, int x, int y, int w, int h) {
    EnsureGoldArt();
    if (!s_borderH && !s_borderV)
        return 0;
    int brdH = s_borderH ? s_borderH->h : 4;
    int brdV = s_borderV ? s_borderV->w : 4;

    // Top & bottom horizontal strips (tiled across the full width).
    if (s_borderH) {
        for (int tx = 0; tx < w; tx += s_borderH->w) {
            int bw = IMin(s_borderH->w, w - tx);
            SDL_Rect sr = { 0, 0, bw, brdH };
            SDL_Rect dr = { x + tx, y, bw, brdH };
            SDL_BlitSurface(s_borderH, &sr, dst, &dr);
            dr.y = y + h - brdH;
            SDL_BlitSurface(s_borderH, &sr, dst, &dr);
        }
    }

    // Left & right vertical strips, mitered into the corners. Each column steps
    // inward by only 1px (top+1+ix .. bottom-1-ix) — matching the original
    // PaintBorder — so the OUTER columns run nearly full height and overlap the
    // top/bottom strips, filling the corners (no gap), while the inner columns
    // form the 45° miter.
    if (s_borderV) {
        for (int ix = 0; ix < s_borderV->w && ix < w / 2; ix++) {
            int top = y + 1 + ix;
            int bot = y + h - 1 - ix;
            if (top >= bot) break;
            for (int ty = top; ty < bot; ty += s_borderV->h) {
                int bh = IMin(s_borderV->h, bot - ty);
                SDL_Rect sr  = { ix, ix, 1, bh };
                SDL_Rect drL = { x + ix, ty, 1, bh };
                SDL_Rect drR = { x + w - 1 - ix, ty, 1, bh };
                SDL_BlitSurface(s_borderV, &sr, dst, &drL);
                SDL_BlitSurface(s_borderV, &sr, dst, &drR);
            }
        }
    }
    return IMin(brdH, brdV);
}

// Draw one Win98-style caption button with a 3D double bevel and a crisp glyph.
// glyph: 1 = minimize (bottom bar), 2 = maximize (titled box), 3 = close (X).
static void DrawCaptionButton(SDL_Surface* dst, SDL_Rect b, int glyph) {
    FillRectP(dst, b, BtnFace);
    // Outer bevel: white top/left, near-black bottom/right.
    FillRectP(dst, { b.x, b.y, b.w, 1 }, BtnHi);
    FillRectP(dst, { b.x, b.y, 1, b.h }, BtnHi);
    FillRectP(dst, { b.x, b.y + b.h - 1, b.w, 1 }, BtnDk);
    FillRectP(dst, { b.x + b.w - 1, b.y, 1, b.h }, BtnDk);
    // Inner bevel: light-gray top/left, mid-gray bottom/right.
    FillRectP(dst, { b.x + 1, b.y + 1, b.w - 2, 1 }, BtnLt);
    FillRectP(dst, { b.x + 1, b.y + 1, 1, b.h - 2 }, BtnLt);
    FillRectP(dst, { b.x + 1, b.y + b.h - 2, b.w - 2, 1 }, BtnSh);
    FillRectP(dst, { b.x + b.w - 2, b.y + 1, 1, b.h - 2 }, BtnSh);

    Uint32 g = SDL_MapRGB(dst->format, BtnGlyph.r, BtnGlyph.g, BtnGlyph.b);
    int cx = b.x + b.w / 2;
    int cy = b.y + b.h / 2;
    if (glyph == 1) {            // minimize: short thick bar near the bottom
        SDL_Rect bar = { cx - 3, b.y + b.h - 5, 6, 2 };
        SDL_FillRect(dst, &bar, g);
    } else if (glyph == 2) {     // maximize: box with a thick (title) top edge
        int x0 = cx - 4, y0 = cy - 4, x1 = cx + 4, y1 = cy + 4;
        SDL_Rect top = { x0, y0, x1 - x0, 2 };
        SDL_Rect lft = { x0, y0, 1, y1 - y0 };
        SDL_Rect rgt = { x1 - 1, y0, 1, y1 - y0 };
        SDL_Rect bot = { x0, y1 - 1, x1 - x0, 1 };
        SDL_FillRect(dst, &top, g); SDL_FillRect(dst, &lft, g);
        SDL_FillRect(dst, &rgt, g); SDL_FillRect(dst, &bot, g);
    } else if (glyph == 3) {     // close: a bold 2px X
        for (int i = -3; i <= 3; i++) {
            for (int t = 0; t < 2; t++) {
                SDL_Rect p1 = { cx + i + t, cy + i, 1, 1 };
                SDL_Rect p2 = { cx + i + t, cy - i, 1, 1 };
                SDL_FillRect(dst, &p1, g);
                SDL_FillRect(dst, &p2, g);
            }
        }
    }
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
        DrawTitleBar(windowSurface, m_x, m_y - TITLE_BAR_HT, m_width);

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

void SDL2Panel::DrawTitleBar(SDL_Surface* dst, int x, int y, int w) {
    // Purple horizontal gradient caption
    SDL_Rect tbRect = { x, y, w, TITLE_BAR_HT };
    FillGradientH(dst, tbRect, TitleGradL, TitleGradR);
    // thin highlight along the very top of the caption
    FillRectP(dst, { x, y, w, 1 }, BorderLight);

    // Caption buttons (right-aligned): minimize, maximize, close.
    // All three are drawn for visual fidelity with the original chrome.
    int byTop = y + (TITLE_BAR_HT - TITLE_BTN_H) / 2;
    int closeX = x + w - 2 - TITLE_BTN_W;
    int maxX   = closeX - 1 - TITLE_BTN_W;
    int minX   = maxX   - 1 - TITLE_BTN_W;
    DrawCaptionButton(dst, { minX,   byTop, TITLE_BTN_W, TITLE_BTN_H }, 1);
    DrawCaptionButton(dst, { maxX,   byTop, TITLE_BTN_W, TITLE_BTN_H }, 2);
    DrawCaptionButton(dst, { closeX, byTop, TITLE_BTN_W, TITLE_BTN_H }, 3);

    // Caption text (left-aligned), clipped so it never runs under the buttons.
    // For a detached window the gold frame wraps the whole window, so inset the
    // text past the left gold border to keep it readable.
    int textLeft = x + 5;
    if (m_ownWindow) {
        EnsureGoldArt();
        textLeft += (s_borderV ? s_borderV->w : 0);
    }
    TTF_Font* font = GetTitleFont();
    if (font && !m_title.empty()) {
        SDL_Surface* textSurf = TTF_RenderText_Blended(font, m_title.c_str(), TitleText);
        if (textSurf) {
            SDL_Rect textDst = { textLeft, y + (TITLE_BAR_HT - textSurf->h) / 2,
                                 textSurf->w, textSurf->h };
            int maxTextW = (minX - 6) - textLeft;
            if (maxTextW < 0) maxTextW = 0;
            if (textDst.w > maxTextW) textDst.w = maxTextW;
            SDL_Rect textSrc = { 0, 0, textDst.w, textDst.h };
            SDL_BlitSurface(textSurf, &textSrc, dst, &textDst);
            SDL_FreeSurface(textSurf);
        }
    }
}

// Which caption button is at (sx, sy), in the same coordinate frame as m_x/m_y.
int SDL2Panel::HitTestTitleButton(int sx, int sy) const {
    if (!m_movable) return TB_BTN_NONE;
    int tbY = m_y - GetTitleBarHeight();
    int byTop = tbY + (TITLE_BAR_HT - TITLE_BTN_H) / 2;
    if (sy < byTop || sy >= byTop + TITLE_BTN_H) return TB_BTN_NONE;
    int closeX = m_x + m_width - 2 - TITLE_BTN_W;
    int maxX   = closeX - 1 - TITLE_BTN_W;
    int minX   = maxX   - 1 - TITLE_BTN_W;
    if (sx >= closeX && sx < closeX + TITLE_BTN_W) return TB_BTN_CLOSE;
    if (sx >= maxX   && sx < maxX   + TITLE_BTN_W) return TB_BTN_MAX;
    if (sx >= minX   && sx < minX   + TITLE_BTN_W) return TB_BTN_MIN;
    return TB_BTN_NONE;
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

    // Caption-button click (minimize / maximize / close) on mouse-up.
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT && inTitleBar) {
        int btn = HitTestTitleButton(screenX, screenY);
        if (btn == TB_BTN_CLOSE) {
            if (m_closeCallback)      m_closeCallback();
            else if (m_closable)      SetVisible(false);
            // A non-closable own-window (the area/world map) has no close action.
            return true;
        } else if (btn == TB_BTN_MIN && m_ownWindow) {
            SDL_MinimizeWindow(m_ownWindow);
            return true;
        } else if (btn == TB_BTN_MAX && m_ownWindow) {
            if (SDL_GetWindowFlags(m_ownWindow) & SDL_WINDOW_MAXIMIZED)
                SDL_RestoreWindow(m_ownWindow);
            else
                SDL_MaximizeWindow(m_ownWindow);
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
            // Don't start a drag if the press lands on a caption button —
            // otherwise the matching mouse-up is swallowed by the m_dragging
            // early-return above and the button would appear dead.
            if (HitTestTitleButton(screenX, screenY) == TB_BTN_NONE) {
                m_dragging = true;
                m_dragOffX = screenX - m_x;
                m_dragOffY = screenY - m_y;
            }
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

void SDL2Panel::SetVisible(bool visible) {
    m_visible = visible;
    // A detached panel owns an OS window — actually show/hide it so "close"
    // makes the window disappear (and the status-bar icon can bring it back).
    if (m_ownWindow) {
        if (visible) {
            SDL_ShowWindow(m_ownWindow);
            SDL_RaiseWindow(m_ownWindow);
        } else {
            SDL_HideWindow(m_ownWindow);
        }
    }
    SetDirty();
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
    if (!m_suppressSync)
        InvokeMoveCallback();
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
    if (!m_suppressSync)
        InvokeMoveCallback();
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
    if (!m_suppressSync)
        InvokeMoveCallback();
}

// ---------------------------------------------------------------------------
// Detached window support
// ---------------------------------------------------------------------------

// Hit-test callback for detached panel windows. The window is borderless and
// we draw our own purple title bar. SDL/Windows won't reliably OS-resize a
// borderless window (it strips WS_THICKFRAME), so we return NORMAL on the edges
// and resize them MANUALLY (HandleDetachedResize). The title bar stays OS-
// DRAGGABLE (works great, crosses monitors). Buttons/content -> NORMAL.
static SDL_HitTestResult SDLCALL DetachedPanelHitTest(SDL_Window* win, const SDL_Point* pt, void* data) {
    SDL2Panel* panel = static_cast<SDL2Panel*>(data);
    int w, h;
    SDL_GetWindowSize(win, &w, &h);
    int tbH = panel->GetTitleBarHeight();
    int grip = SDL2Panel::RESIZE_BORDER;

    // Edges/corners → NORMAL so the mouse-down reaches HandleDetachedResize.
    if (pt->x < grip || pt->x >= w - grip || pt->y < grip || pt->y >= h - grip)
        return SDL_HITTEST_NORMAL;

    if (tbH > 0 && pt->y < tbH) {
        // Caption buttons must deliver a normal click to our event handler.
        int byTop = (tbH - SDL2Panel::TITLE_BTN_H) / 2;
        int closeX = w - 2 - SDL2Panel::TITLE_BTN_W;
        int minX   = closeX - 2 * (SDL2Panel::TITLE_BTN_W + 1);
        if (pt->y >= byTop && pt->y < byTop + SDL2Panel::TITLE_BTN_H &&
            pt->x >= minX && pt->x < closeX + SDL2Panel::TITLE_BTN_W)
            return SDL_HITTEST_NORMAL;
        return SDL_HITTEST_DRAGGABLE;  // OS moves the window — crosses monitors
    }

    return SDL_HITTEST_NORMAL;
}

void SDL2Panel::Detach(GameWindow* mainWin) {
    Detach(mainWin ? mainWin->GetWindow() : nullptr);
}

void SDL2Panel::Detach(SDL_Window* ownerWindow) {
    if (m_ownWindow)
        return;  // already detached

    int tbH = GetTitleBarHeight();

    // m_x/m_y are the CONTENT top-left in compositor (main-window-local) coords.
    // Convert to global screen coords, then place the borderless OS window so
    // its custom title bar sits just above the content.
    int globalX = m_x, globalY = m_y;
    if (ownerWindow) {
        int wx, wy;
        SDL_GetWindowPosition(ownerWindow, &wx, &wy);
        globalX += wx;
        globalY += wy;
    }

    // Borderless: we draw the Enemy-Nations purple chrome ourselves; the OS
    // only performs drag/resize via the hit-test callback below. SKIP_TASKBAR +
    // the owner-window relationship keep it floating above the background.
    m_ownWindow = GameWindow::CreateSDLWindow(
        m_title.c_str(), globalX, globalY - tbH, m_width, m_height + tbH,
        SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SKIP_TASKBAR);

    if (!m_ownWindow) {
        LogPanel("ERROR: Failed to create detached window for '" + m_name + "': " + SDL_GetError());
        return;
    }

    m_ownWindowID = SDL_GetWindowID(m_ownWindow);
    SDL_SetWindowHitTest(m_ownWindow, DetachedPanelHitTest, this);

#ifdef _WIN32
    // Owner relationship: the panel floats above its owner without ALWAYS_ON_TOP.
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

    // Keep the new window fully on its monitor — a detached map can inherit a
    // near-fullscreen placement from the old composited layout and hang off the
    // screen edges otherwise.
    {
        int di = SDL_GetWindowDisplayIndex(m_ownWindow);
        SDL_Rect ub;
        if (di >= 0 && SDL_GetDisplayUsableBounds(di, &ub) == 0) {
            int wx, wy, ww, wh;
            SDL_GetWindowPosition(m_ownWindow, &wx, &wy);
            SDL_GetWindowSize(m_ownWindow, &ww, &wh);
            bool resized = false;
            if (ww > ub.w) { ww = ub.w; resized = true; }
            if (wh > ub.h) { wh = ub.h; resized = true; }
            if (wx + ww > ub.x + ub.w) wx = ub.x + ub.w - ww;
            if (wy + wh > ub.y + ub.h) wy = ub.y + ub.h - wh;
            if (wx < ub.x) wx = ub.x;
            if (wy < ub.y) wy = ub.y;
            SDL_SetWindowPosition(m_ownWindow, wx, wy);
            if (resized) {
                SDL_SetWindowSize(m_ownWindow, ww, wh);
                m_width  = ww;
                m_height = wh - tbH;
                CreateSurface();
                InvokeResizeCallback(m_width, m_height);
            }
        }
    }

    // Keep m_x/m_y as global content coords (from the final, clamped window
    // position) so SetPosition and the move-callback (MFC tracking) stay
    // consistent.
    {
        int wx, wy;
        SDL_GetWindowPosition(m_ownWindow, &wx, &wy);
        m_x = wx;
        m_y = wy + tbH;
    }

    SDL_RaiseWindow(m_ownWindow);
    LogPanel("Detached panel '" + m_name + "' to own window (ID=" + std::to_string(m_ownWindowID) + ")");
    m_dirty = true;
    InvokeMoveCallback();  // sync the backing MFC window to the new screen pos
}

void SDL2Panel::Attach(GameWindow* mainWin) {
    if (!m_ownWindow)
        return;

    int tbH = GetTitleBarHeight();

    // Convert global window position back to main-window-relative content coords.
    int wx = 0, wy = 0;
    if (mainWin && mainWin->GetWindow()) {
        SDL_GetWindowPosition(mainWin->GetWindow(), &wx, &wy);
    }
    int gx, gy;
    SDL_GetWindowPosition(m_ownWindow, &gx, &gy);
    m_x = gx - wx;
    m_y = gy - wy + tbH;  // window top is the title bar; content is tbH below

    DestroyOwnWindow();
    LogPanel("Attached panel '" + m_name + "' back to compositor");
    m_dirty = true;
    InvokeMoveCallback();
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

    int tbH = GetTitleBarHeight();
    int W = winSurf->w, H = winSurf->h;

    // Clear background
    SDL_FillRect(winSurf, nullptr, SDL_MapRGB(winSurf->format, 0, 0, 0));

    // Content below the title bar (full width; the frame overlays the outer
    // few px of the edges, so the content origin stays at (0, tbH) and
    // selection coordinates are unaffected).
    SDL_Rect dstRect = { 0, tbH, m_width, m_height };
    SDL_BlitSurface(m_surface, nullptr, winSurf, &dstRect);

    // Purple title bar across the top (full width — drawn at the same (0,0)
    // frame the button hit-testing assumes). Drawn before the gold so the
    // frame wraps its outer edge; the title text is inset past the gold below.
    if (m_movable)
        DrawTitleBar(winSurf, 0, 0, m_width);

    // Carved-gold Enemy Nations border around the WHOLE window, so the title
    // bar sits inside the frame and the top is fully bordered (matching the
    // original dialogs). Falls back to a silver raised bevel if art is missing.
    EnsureGoldArt();
    if (s_borderH || s_borderV)
        DrawGoldFrame(winSurf, 0, 0, W, H);
    else
        DrawRaisedBorder(winSurf, 0, 0, W, H, 4);

    SDL_UpdateWindowSurface(m_ownWindow);
    m_dirty = false;
}

bool SDL2Panel::HandleDetachedEvent(SDL_Event& event, int localX, int localY) {
    if (m_eventCallback)
        return m_eventCallback(event, localX, localY);
    return false;
}

// Manual edge/corner resize of the borderless own-window. We can't rely on the
// OS to resize a borderless window (SDL strips WS_THICKFRAME), so when the user
// presses an edge we capture the mouse and drive SDL_SetWindowSize/Position
// from the global cursor delta. The resulting SDL_WINDOWEVENT_SIZE_CHANGED /
// _MOVED events flow back through the compositor to resize the game surface and
// re-sync the MFC window.
bool SDL2Panel::HandleDetachedResize(SDL_Event& event) {
    if (!m_ownWindow) return false;
    const int GRIP = SDL2Panel::RESIZE_BORDER + 2;  // a touch more forgiving than the hit-test

    if (m_dResizing) {
        if (event.type == SDL_MOUSEMOTION) {
            int gx = 0, gy = 0;
            SDL_GetGlobalMouseState(&gx, &gy);
            int dx = gx - m_dStartMX, dy = gy - m_dStartMY;
            int nx = m_dStartWX, ny = m_dStartWY, nw = m_dStartWW, nh = m_dStartWH;
            if (m_dResizeEdge & 2) nw = m_dStartWW + dx;                        // right
            if (m_dResizeEdge & 1) { nw = m_dStartWW - dx; nx = m_dStartWX + dx; }  // left
            if (m_dResizeEdge & 8) nh = m_dStartWH + dy;                        // bottom
            if (m_dResizeEdge & 4) { nh = m_dStartWH - dy; ny = m_dStartWY + dy; }  // top
            int minW = MIN_WIDTH;
            int minH = MIN_HEIGHT + GetTitleBarHeight();
            if (nw < minW) { if (m_dResizeEdge & 1) nx -= (minW - nw); nw = minW; }
            if (nh < minH) { if (m_dResizeEdge & 4) ny -= (minH - nh); nh = minH; }
            SDL_SetWindowPosition(m_ownWindow, nx, ny);
            SDL_SetWindowSize(m_ownWindow, nw, nh);
            return true;
        }
        if (event.type == SDL_MOUSEBUTTONUP) {
            m_dResizing = false;
            SDL_CaptureMouse(SDL_FALSE);
            return true;
        }
        return true;  // swallow other events mid-resize
    }

    int mx, my;
    if (event.type == SDL_MOUSEMOTION) { mx = event.motion.x; my = event.motion.y; }
    else if (event.type == SDL_MOUSEBUTTONDOWN) { mx = event.button.x; my = event.button.y; }
    else return false;

    int w = 0, h = 0;
    SDL_GetWindowSize(m_ownWindow, &w, &h);
    int edge = 0;
    if (mx < GRIP)      edge |= 1;
    if (mx >= w - GRIP) edge |= 2;
    if (my < GRIP)      edge |= 4;
    if (my >= h - GRIP) edge |= 8;
    if (edge == 0) return false;

    if (event.type == SDL_MOUSEMOTION) {
        static SDL_Cursor* sWE = nullptr; static SDL_Cursor* sNS = nullptr;
        static SDL_Cursor* sNWSE = nullptr; static SDL_Cursor* sNESW = nullptr;
        if (!sWE) {
            sWE   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
            sNS   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
            sNWSE = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
            sNESW = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
        }
        bool l = edge & 1, r = edge & 2, t = edge & 4, b = edge & 8;
        if ((l && t) || (r && b))      SDL_SetCursor(sNWSE);
        else if ((r && t) || (l && b)) SDL_SetCursor(sNESW);
        else if (l || r)               SDL_SetCursor(sWE);
        else                           SDL_SetCursor(sNS);
        return true;  // consume so the edge doesn't fall through to content hover
    }

    // MOUSEBUTTONDOWN on an edge → begin manual resize.
    if (event.button.button == SDL_BUTTON_LEFT) {
        m_dResizing = true;
        m_dResizeEdge = edge;
        SDL_GetGlobalMouseState(&m_dStartMX, &m_dStartMY);
        SDL_GetWindowPosition(m_ownWindow, &m_dStartWX, &m_dStartWY);
        SDL_GetWindowSize(m_ownWindow, &m_dStartWW, &m_dStartWH);
        SDL_CaptureMouse(SDL_TRUE);
        return true;
    }
    return false;
}
