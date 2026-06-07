#include "stdafx.h"

#include "SDL2Panel.h"
#include "GameWindow.h"
#include "SDL2CreateStatus.h"  // GetCreateStatus()->IsVisible() — defer panel show during load
#include "SDL2MainMenu.h"  // CreateSurfaceFromDIB
#include "bmbutton.h"      // must precede bitmaps.h (provides CBmBtnData)
#include "bitmaps.h"       // theBitmaps, DIB_GOLD, DIB_BORDER_HORZ/VERT
#include "w22_settings.h"  // w22::GetProfileInt — [Advanced] Renderer flag (T0b)
#include "RenderBackend.h"  // RenderBackendIsGpu() — 3-way backend selector
#include "SDL2Sprites.h"   // GPU sprite layer (S1: trees)
#include "SDL2Terrain.h"    // T2: load terrain tile textures on the area renderer
#include "Perf.h"           // present sub-phase profiling counters
#include "base.h"           // T2.3: CAnimAtr::GetSpriteLayerSurface for compositing

#include <SDL.h>
#include <SDL_ttf.h>
#ifdef _WIN32
#include <SDL_syswm.h>
#endif
#include <fstream>
#include <unordered_map>

// Remembered detached-window placements, keyed by panel name. A game window
// (area/world/vehicles) that is closed and reopened — or, like an area map,
// destroyed and later recreated — comes back where the user last dragged and
// sized it. Stored as the global OS-window rect (top-left includes the title
// bar). Process-lifetime only; not persisted to disk.
static std::unordered_map<std::string, SDL_Rect> g_savedPlacements;

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
static SDL_Surface* s_winBtn   = nullptr;  // winbtn.d24  (BM24 idx 12): 5col x 2row of 14x14 system-button glyphs
static SDL_Surface* s_caption  = nullptr;  // caption2.d24 (BM24 idx 14): 913x24 active title-bar gradient
static bool         s_goldTried = false;

// BM24 library indices for the window-chrome art (see enations data MISC misc.mif).
static const int BMIDX_WINBTN   = 12;  // system buttons (minimize/maximize/restore/close)
static const int BMIDX_CAPTION  = 14;  // active title bar background (caption2)
// winbtn.d24 cell geometry: each glyph is 14x14, laid out on a 16px pitch with a
// 1px magenta separator. Row 0 = normal, row 1 = pressed.
static const int WINBTN_PITCH = 16;
static const int WINBTN_GLYPH = 14;

static void EnsureGoldArt() {
    if (s_goldTried) return;
    s_goldTried = true;
    CDIB* pGold = theBitmaps.GetByIndex(DIB_GOLD);
    if (pGold) s_goldBg = SDL2MainMenu::CreateSurfaceFromDIB(pGold);
    CDIB* pH = theBitmaps.GetByIndex(DIB_BORDER_HORZ);
    if (pH) s_borderH = SDL2MainMenu::CreateSurfaceFromDIB(pH);
    CDIB* pV = theBitmaps.GetByIndex(DIB_BORDER_VERT);
    if (pV) s_borderV = SDL2MainMenu::CreateSurfaceFromDIB(pV);
    CDIB* pBtn = theBitmaps.GetByIndex(BMIDX_WINBTN);
    if (pBtn) s_winBtn = SDL2MainMenu::CreateSurfaceFromDIB(pBtn);
    CDIB* pCap = theBitmaps.GetByIndex(BMIDX_CAPTION);
    if (pCap) s_caption = SDL2MainMenu::CreateSurfaceFromDIB(pCap);
}

static inline int IMin(int a, int b) { return a < b ? a : b; }

// Frame the rectangle with the carved-gold Enemy Nations border art. Delegates
// to the shared, corner-correct SDL2MainMenu::DrawGoldBorder so every framed
// window/dialog draws an identical border from one implementation.
static int DrawGoldFrame(SDL_Surface* dst, int x, int y, int w, int h) {
    EnsureGoldArt();
    if (!s_borderH && !s_borderV)
        return 0;
    SDL2MainMenu::DrawGoldBorder(dst, x, y, w, h, s_borderH, s_borderV);
    return IMin(s_borderH ? s_borderH->h : 4, s_borderV ? s_borderV->w : 4);
}

// Draw one caption button. Prefers the original Enemy Nations system-button art
// (winbtn.d24): a strip of 14x14 blue glyph cells. glyph 1=minimize, 2=maximize,
// 3=close map to winbtn columns 1, 2 and 4; `pressed` selects the lower (row 1)
// state. Falls back to a Win98 3D double-bevel button if the art is unavailable.
static void DrawCaptionButton(SDL_Surface* dst, SDL_Rect b, int glyph, bool pressed = false) {
    EnsureGoldArt();
    if (s_winBtn) {
        int col;
        switch (glyph) {
            case 1:  col = 1; break;   // minimize (bottom bar)
            case 2:  col = 2; break;   // maximize (single box)
            case 3:  col = 4; break;   // close (X)
            default: col = 0; break;
        }
        int row = pressed ? 1 : 0;
        SDL_Rect src = { col * WINBTN_PITCH + 1, row * WINBTN_PITCH + 1,
                         WINBTN_GLYPH, WINBTN_GLYPH };
        SDL_Rect d   = { b.x + (b.w - WINBTN_GLYPH) / 2,
                         b.y + (b.h - WINBTN_GLYPH) / 2,
                         WINBTN_GLYPH, WINBTN_GLYPH };
        SDL_BlitSurface(s_winBtn, &src, dst, &d);
        return;
    }

    // --- Fallback: classic Win98 double-bevel button drawn by hand ---
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
    if (glyph == 1) {            // minimize: short bar at the bottom (Win95)
        SDL_Rect bar = { cx - 3, cy + 3, 7, 2 };
        SDL_FillRect(dst, &bar, g);
    } else if (glyph == 2) {     // maximize: window box with a thick title edge
        int x0 = cx - 5, y0 = cy - 4, x1 = cx + 5, y1 = cy + 4;
        SDL_Rect top = { x0, y0, x1 - x0, 2 };               // title bar (thick)
        SDL_Rect lft = { x0, y0, 1, y1 - y0 };
        SDL_Rect rgt = { x1 - 1, y0, 1, y1 - y0 };
        SDL_Rect bot = { x0, y1 - 1, x1 - x0, 1 };
        SDL_FillRect(dst, &top, g); SDL_FillRect(dst, &lft, g);
        SDL_FillRect(dst, &rgt, g); SDL_FillRect(dst, &bot, g);
    } else if (glyph == 3) {     // close: an X (Win95 — two clean diagonals)
        for (int i = -3; i <= 3; i++) {
            SDL_Rect p1 = { cx - 3 + (i + 3), cy + i, 2, 1 };
            SDL_Rect p2 = { cx - 3 + (i + 3), cy - i, 2, 1 };
            SDL_FillRect(dst, &p1, g);
            SDL_FillRect(dst, &p2, g);
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
    if (m_width <= 0 || m_height <= 0) {
        FreeSurface();
        return;
    }

    SDL_Surface* fresh = SDL_CreateRGBSurface(
        0, m_width, m_height, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0);

    if (!fresh) {
        LogPanel("ERROR: Failed to create surface for panel '" + m_name + "': " + SDL_GetError());
        return;  // keep the old surface rather than dropping to nothing
    }

    // Carry the previous content into the resized surface. A live resize fires
    // SIZE_CHANGED ~10x/sec; each one lands here, and a freshly-allocated (black)
    // surface presented before the owner repaints made the window flicker. Blitting
    // the old pixels in (clipped to the overlap) shows the prior frame until the
    // next real repaint, so the resize looks smooth instead of strobing black.
    if (m_surface) {
        SDL_SetSurfaceBlendMode(m_surface, SDL_BLENDMODE_NONE);
        SDL_BlitSurface(m_surface, nullptr, fresh, nullptr);
        SDL_FreeSurface(m_surface);
    }
    m_surface = fresh;
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
    // Title-bar background: the original purple caption art (caption2.d24),
    // stretched across the window width. Falls back to a hand-drawn violet
    // gradient if the art is unavailable.
    SDL_Rect tbRect = { x, y, w, TITLE_BAR_HT };
    EnsureGoldArt();
    if (s_caption) {
        SDL_Rect capSrc = { 0, 0, s_caption->w, s_caption->h };
        SDL_BlitScaled(s_caption, &capSrc, dst, &tbRect);
    } else {
        FillGradientH(dst, tbRect, TitleGradL, TitleGradR);
        // thin highlight along the very top of the caption
        FillRectP(dst, { x, y, w, 1 }, BorderLight);
    }

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
            // Reopen where the user last left it (no-op the first time, or a
            // no-op if the window never moved).
            RestoreSavedPlacement();
            SDL_ShowWindow(m_ownWindow);
            SDL_RaiseWindow(m_ownWindow);
        } else {
            // Capture the position before hiding so the next open restores it,
            // even if a stray MOVED event was missed.
            RememberPlacement();
            SDL_HideWindow(m_ownWindow);
        }
    }
    SetDirty();
}

// Forget every remembered placement (process-wide). After this, windows open
// at their caller-provided default position until moved again.
void SDL2Panel::ResetAllPlacements() {
    g_savedPlacements.clear();
}

// Record this detached window's current OS rect under its name so a later
// (re)open can restore it. No-op for non-detached panels.
void SDL2Panel::RememberPlacement() {
    if (!m_ownWindow)
        return;
    int wx = 0, wy = 0, ww = 0, wh = 0;
    SDL_GetWindowPosition(m_ownWindow, &wx, &wy);
    SDL_GetWindowSize(m_ownWindow, &ww, &wh);
    if (ww > 0 && wh > 0)
        g_savedPlacements[m_name] = SDL_Rect{ wx, wy, ww, wh };
}

// Re-apply a remembered rect (position + size) to this detached window. Called
// from Detach (recreated windows) and SetVisible(true) (hidden→shown windows).
void SDL2Panel::RestoreSavedPlacement() {
    if (!m_ownWindow)
        return;
    auto it = g_savedPlacements.find(m_name);
    if (it == g_savedPlacements.end())
        return;
    const SDL_Rect& r = it->second;
    int tbH = GetTitleBarHeight();
    int cw = r.w;
    int ch = r.h - tbH;
    if (cw < MIN_WIDTH)  cw = MIN_WIDTH;
    if (ch < MIN_HEIGHT) ch = MIN_HEIGHT;
    if (cw != m_width || ch != m_height) {
        m_width  = cw;
        m_height = ch;
        CreateSurface();
        SDL_SetWindowSize(m_ownWindow, cw, ch + tbH);
        InvokeResizeCallback(m_width, m_height);
    }
    SDL_SetWindowPosition(m_ownWindow, r.x, r.y);
    int gx = r.x, gy = r.y;
    SDL_GetWindowPosition(m_ownWindow, &gx, &gy);  // OS may clamp to a monitor
    m_x = gx;
    m_y = gy + tbH;
    InvokeMoveCallback();
}

// Compositor hook: the OS dragged our borderless window. Keep the stored
// content origin in sync (so GetX/GetY never go stale after a drag) and
// remember the new placement.
void SDL2Panel::OnOwnWindowMoved() {
    if (!m_ownWindow)
        return;
    int wx = 0, wy = 0;
    SDL_GetWindowPosition(m_ownWindow, &wx, &wy);
    m_x = wx;
    m_y = wy + GetTitleBarHeight();
    RememberPlacement();
    InvokeMoveCallback();
    m_dirty = true;
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
    if (m_ownWindow && !m_suppressSync) {
        SDL_SetWindowSize(m_ownWindow, w, h + GetTitleBarHeight());
        RememberPlacement();  // remember the new size for reopen
    }
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
// we draw our own purple title bar. We return NORMAL everywhere so every press
// reaches our own event handlers: edges/corners drive HandleDetachedResize and
// the title bar drives HandleDetachedDrag. We deliberately do NOT return
// SDL_HITTEST_DRAGGABLE for the title bar — that hands the move to the Windows
// modal move loop, which blocks the app's main loop and freezes rendering until
// the drag ends. Driving the drag ourselves keeps the game rendering.
static SDL_HitTestResult SDLCALL DetachedPanelHitTest(SDL_Window* /*win*/, const SDL_Point* /*pt*/, void* /*data*/) {
    return SDL_HITTEST_NORMAL;
}

void SDL2Panel::Detach(GameWindow* mainWin) {
    m_detachOwner = mainWin;  // remembered so RenderDetached can see when the load finishes
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

    // If a load is in progress (the topmost loading dialog is up), create this
    // window HIDDEN and reveal it on the first RenderDetached after the load
    // finishes — otherwise the spawning window flashes over the background for a
    // frame mid-load. Outside a load, create it shown as before.
    m_deferShow = (m_detachOwner && m_detachOwner->GetCreateStatus()
                   && m_detachOwner->GetCreateStatus()->IsVisible());

    // Borderless: we draw the Enemy-Nations purple chrome ourselves; the OS
    // only performs drag/resize via the hit-test callback below. SKIP_TASKBAR +
    // the owner-window relationship keep it floating above the background.
    Uint32 winFlags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SKIP_TASKBAR;
    if (!m_deferShow) winFlags |= SDL_WINDOW_SHOWN;
    m_ownWindow = GameWindow::CreateSDLWindow(
        m_title.c_str(), globalX, globalY - tbH, m_width, m_height + tbH, winFlags);

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

    // If this window has been opened (and moved/sized) before, reopen it in the
    // same spot. Done before the on-monitor clamp below so a restored rect is
    // still validated against the current display bounds.
    RestoreSavedPlacement();

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

    MaybeCreateOwnRenderer();  // T0b: GPU present for this window if the flag is on

    // Don't raise a window we created hidden (deferred until load completes) —
    // raising it would SetForegroundWindow and defeat the point.
    if (!m_deferShow)
        SDL_RaiseWindow(m_ownWindow);
    LogPanel("Detached panel '" + m_name + "' to own window (ID=" + std::to_string(m_ownWindowID) + ")");
    m_dirty = true;
    InvokeMoveCallback();  // sync the backing MFC window to the new screen pos
}

// ---- T0b: optional GPU present path for the detached window ----
// Mirrors GameWindow's main-window present abstraction. When [Advanced] Renderer
// is on, the whole detached frame (content + chrome) is composited into m_ownBack
// and presented through m_ownRenderer. This is the window/surface the GPU terrain
// mesh (T2) will render into; for T0b it just re-presents today's composite, so
// the result is pixel-parity with the software path.
void SDL2Panel::MaybeCreateOwnRenderer() {
    if (m_ownRenderer || !m_ownWindow)
        return;
    if (!RenderBackendIsGpu())
        return;  // software present (default) — unchanged behavior
    // No PRESENT_VSYNC (same rationale as the main window: present is called
    // per content-change, not from a single per-frame chokepoint yet).
    m_ownRenderer = SDL_CreateRenderer(m_ownWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!m_ownRenderer) {
        LogPanel("WARN: own-window SDL_CreateRenderer failed for '" + m_name +
                 "', staying software: " + SDL_GetError());
        return;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    EnsureOwnBack();
    LogPanel("T0b: GPU present active for detached panel '" + m_name + "'");

    // T2: the area-map panel is the terrain-bearing window — load the baked
    // terrain tile textures onto its renderer so the GPU mesh (T2.3) can draw.
    if (m_name.rfind("area", 0) == 0) {
        int n = SDL2Terrain::Load(m_ownRenderer);
        char rp[64]; sprintf(rp, "%p", (void*)m_ownRenderer);
        LogPanel("[REN] area renderer CREATED " + std::string(rp) + " loaded " +
                 std::to_string(n) + " tiles");
        // GPU sprite layer (S1): trees draw on this same renderer.
        SDL2Sprites::SetRenderer(m_ownRenderer);
    }
}

// Item 5 (GPU dirty-rects): runtime kill-switch. Default OFF — the staged dirty-rect
// path is opt-in via env EN_DIRTY=1 until validated, then it becomes the default. Read
// once; cached for the process.
bool SDL2Panel::GpuDirtyEnabled() {
    // Default ON: enables incremental GPU sprite capture (the static forest is captured
    // once and kept; only dynamic objects are re-captured each frame) + the persistent
    // content RT + dirty-rect harvest. With it OFF, DiscoverSpritesGpu re-scans the whole
    // ~27k-tree forest and re-captures every sprite EVERY frame (spr.full every frame),
    // which is the dominant zoomed-out CPU cost (~35ms in r.draw). Set EN_DIRTY=0 to disable.
    static int s_on = -1;
    if (s_on < 0) {
        const char* e = SDL_getenv("EN_DIRTY");
        s_on = (e && *e && *e == '0') ? 0 : 1;
    }
    return s_on != 0;
}

void SDL2Panel::DestroyOwnRenderer() {
    // T2/S1: the GPU terrain tiles and sprite atlas are GLOBAL singletons bound to one
    // renderer. During an in-game load the NEW area panel is created (and binds the
    // global state to ITS renderer) BEFORE this OLD panel is destroyed — so only tear
    // the global state down if it is still bound to THIS panel's renderer. Otherwise we
    // would Unload the new panel's tiles / null its sprite renderer → black terrain +
    // flickering trees after the load. (Main-menu load avoids the overlap; this is the
    // in-game-load / new-game path.)
    if (m_name.rfind("area", 0) == 0) {
        char rp[64]; sprintf(rp, "%p", (void*)m_ownRenderer);
        bool ownsTerrain = (SDL2Terrain::CurrentRenderer() == m_ownRenderer);
        bool ownsSprites = (SDL2Sprites::CurrentRenderer() == m_ownRenderer);
        LogPanel("[REN] area renderer DESTROYED " + std::string(rp) +
                 " ownsTerrain=" + (ownsTerrain ? "1" : "0") +
                 " ownsSprites=" + (ownsSprites ? "1" : "0"));
        if (ownsTerrain && SDL2Terrain::IsLoaded())
            SDL2Terrain::Unload();   // free terrain tiles bound to THIS renderer
        if (ownsSprites) {
            SDL2Sprites::InvalidateTextures();
            SDL2Sprites::SetRenderer(nullptr);
        }
    }
    if (m_contentRT)  { SDL_DestroyTexture(m_contentRT);  m_contentRT = nullptr; }
    if (m_ownBackTex) { SDL_DestroyTexture(m_ownBackTex); m_ownBackTex = nullptr; }
    if (m_ownBack)    { SDL_FreeSurface(m_ownBack);       m_ownBack = nullptr; }
    if (m_ownRenderer){ SDL_DestroyRenderer(m_ownRenderer); m_ownRenderer = nullptr; }
    m_ownBackW = m_ownBackH = 0;
}

SDL_Surface* SDL2Panel::EnsureOwnBack() {
    if (!m_ownRenderer)
        return nullptr;
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(m_ownRenderer, &w, &h);
    if (w <= 0 || h <= 0)
        return nullptr;
    if (m_ownBack && m_ownBackW == w && m_ownBackH == h)
        return m_ownBack;
    if (m_ownBackTex) { SDL_DestroyTexture(m_ownBackTex); m_ownBackTex = nullptr; }
    if (m_ownBack)    { SDL_FreeSurface(m_ownBack);       m_ownBack = nullptr; }
    if (m_contentRT)  { SDL_DestroyTexture(m_contentRT);  m_contentRT = nullptr; }
    m_ownBack = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    m_ownBackTex = SDL_CreateTexture(m_ownRenderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!m_ownBack || !m_ownBackTex) {
        LogPanel("ERROR: own-window back-buffer alloc failed for '" + m_name + "'");
        return nullptr;
    }
    // BLEND so the T2.3 overlay's transparent content lets GPU terrain show through.
    SDL_SetTextureBlendMode(m_ownBackTex, SDL_BLENDMODE_BLEND);
    // Item 5: persistent content RT (terrain+sprites). Lazily created only when the
    // dirty-rect path is enabled; opaque (the terrain clears it each (re)build).
    if (GpuDirtyEnabled()) {
        m_contentRT = SDL_CreateTexture(m_ownRenderer, SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_TARGET, w, h);
        if (m_contentRT)
            SDL_SetTextureBlendMode(m_contentRT, SDL_BLENDMODE_NONE);
    }
    m_ownBackW = w; m_ownBackH = h;
    return m_ownBack;
}

void SDL2Panel::PresentOwn() {
    if (!m_ownRenderer || !m_ownBack || !m_ownBackTex)
        return;

    // T2.3 layered composite (terrain window): GPU terrain mesh on the bottom,
    // then the transparent overlay (color-keyed sprite layer + chrome) on top.
    if (m_terrainAA && SDL2Terrain::IsLoaded()) {
        int tbH = GetTitleBarHeight();

        // Item 5 Stage 0: route the terrain+sprite composite through a persistent RT
        // instead of straight to the window backbuffer. Behaviour is identical here (we
        // still full-redraw each frame, +1 RenderCopy), but it establishes the surface
        // that later stages update incrementally (static content persists across frames).
        bool useRT = GpuDirtyEnabled() && m_contentRT != nullptr;
        SDL_Texture* prevTarget = nullptr;
        if (useRT) {
            prevTarget = SDL_GetRenderTarget(m_ownRenderer);
            SDL_SetRenderTarget(m_ownRenderer, m_contentRT);
        }

        SDL_SetRenderDrawColor(m_ownRenderer, 0, 0, 0, 255);
        SDL_RenderClear(m_ownRenderer);

        // Terrain mesh into the content area (below the title bar). The mesh
        // coords are content-relative, so a viewport offset by tbH places them.
        // (When rendering to m_contentRT the same offset keeps content aligned.)
        SDL_Rect vp = { 0, tbH, m_width, m_height };
        SDL_RenderSetViewport(m_ownRenderer, &vp);
        { Perf::ScopeCounter _ct( "p.terrain" );
          SDL2Terrain::Render(m_ownRenderer, *m_terrainAA); }
        // GPU sprite layer (S1): trees over terrain, under the CPU sprite/chrome
        // overlay. Re-project the view-space tree list to the current UL so they
        // track the panning terrain; drawn in this content viewport (screen = pos-UL).
        CPoint ul = m_terrainAA->GetUL();
        { Perf::ScopeCounter _cs( "p.sprites" );
          SDL2Sprites::Submit(m_ownRenderer, ul.x, ul.y, m_width, m_height); }
        SDL_RenderSetViewport(m_ownRenderer, nullptr);

        // Copy the persistent content RT to the window backbuffer before the overlay.
        if (useRT) {
            SDL_SetRenderTarget(m_ownRenderer, prevTarget);
            SDL_RenderCopy(m_ownRenderer, m_contentRT, nullptr, nullptr);
        }

        // Overlay: m_ownBack = sprites (opaque) + chrome (opaque), content
        // transparent → terrain shows through. m_ownBackTex is BLEND mode.
        { Perf::ScopeCounter _co( "p.overlay" );
          SDL_UpdateTexture(m_ownBackTex, nullptr, m_ownBack->pixels, m_ownBack->pitch);
          SDL_RenderCopy(m_ownRenderer, m_ownBackTex, nullptr, nullptr); }
        { Perf::ScopeCounter _cp( "p.present" );
          SDL_RenderPresent(m_ownRenderer); }
        return;
    }

    SDL_UpdateTexture(m_ownBackTex, nullptr, m_ownBack->pixels, m_ownBack->pitch);
    SDL_RenderClear(m_ownRenderer);
    SDL_RenderCopy(m_ownRenderer, m_ownBackTex, nullptr, nullptr);
    SDL_RenderPresent(m_ownRenderer);
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
        // Capture the final placement so a window that is destroyed and later
        // recreated under the same name (e.g. an area map closed then reopened
        // from the status bar) comes back in the same spot.
        RememberPlacement();
        DestroyOwnRenderer();  // T0b: free renderer/back-buffer before the window
        SDL_DestroyWindow(m_ownWindow);
        m_ownWindow = nullptr;
        m_ownWindowID = 0;
    }
}

void SDL2Panel::RenderDetached() {
    if (!m_ownWindow || !m_visible || !m_surface)
        return;

    // T0b: in renderer mode, composite into the offscreen back-buffer (sized to
    // the renderer output, which tracks the window) instead of the window surface
    // — SDL forbids SDL_GetWindowSurface on a window that has a renderer.
    SDL_Surface* winSurf;
    if (m_ownRenderer) {
        winSurf = EnsureOwnBack();
        if (!winSurf)
            return;
    } else {
        winSurf = SDL_GetWindowSurface(m_ownWindow);
        if (!winSurf)
            return;

        // After a manual resize SDL can hand back a window surface still sized to
        // the OLD dimensions; clearing/blitting into it leaves the newly-exposed
        // strip showing stale backbuffer pixels. Force a fresh surface bound to the
        // current window size whenever they disagree.
        int wW = 0, wH = 0;
        SDL_GetWindowSize(m_ownWindow, &wW, &wH);
        if (winSurf->w != wW || winSurf->h != wH) {
            SDL_DestroyWindowSurface(m_ownWindow);
            winSurf = SDL_GetWindowSurface(m_ownWindow);
            if (!winSurf) return;
        }
    }

    int tbH = GetTitleBarHeight();
    int W = winSurf->w, H = winSurf->h;

    // T2.3: GPU-terrain window. Build winSurf (m_ownBack) as a transparent
    // OVERLAY — only the color-keyed sprite layer + chrome are opaque; the
    // content area stays transparent so the GPU terrain mesh (drawn in
    // PresentOwn, underneath) shows through.
    bool bGpuTerrain = m_ownRenderer && m_terrainAA && SDL2Terrain::IsLoaded();

    SDL_Rect dstRect = { 0, tbH, m_width, m_height };
    if (bGpuTerrain) {
        Perf::ScopeCounter _cof( "p.ovlfill" );        // MEASURE: full-window CPU overlay fill+blit
        SDL_FillRect(winSurf, nullptr, 0x00000000);   // transparent
        SDL_Surface* spriteSurf = m_terrainAA->GetSpriteLayerSurface();
        if (spriteSurf) {
            SDL_SetColorKey(spriteSurf, SDL_TRUE,
                            SDL_MapRGB(spriteSurf->format, 255, 0, 255));
            SDL_BlitSurface(spriteSurf, nullptr, winSurf, &dstRect);  // sprites only
        }

        // Area button bar lives in the area panel's m_surface (it's blitted there
        // in CWndArea::OnPaint), which the GPU path skips — so composite it here as
        // opaque chrome along the bottom of the content. Without this the bar (build
        // building / build road / zoom / rotate) vanishes once the area window is
        // using the GPU terrain renderer.
        if (m_bottomChromePanel) {
            SDL_Surface* barSurf = m_bottomChromePanel->GetSurface();
            if (barSurf) {
                int barOffY = tbH + m_height - barSurf->h;
                if (barOffY >= tbH) {
                    SDL_Rect bd = { 0, barOffY, barSurf->w, barSurf->h };
                    SDL_BlitSurface(barSurf, nullptr, winSurf, &bd);
                }
            }
        }
    } else {
        // Clear background, then the full CPU composite (terrain+sprites).
        SDL_FillRect(winSurf, nullptr, SDL_MapRGB(winSurf->format, 0, 0, 0));
        SDL_BlitSurface(m_surface, nullptr, winSurf, &dstRect);
    }

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

    if (m_ownRenderer)
        PresentOwn();                       // T0b: upload back-buffer + present via GPU
    else
        SDL_UpdateWindowSurface(m_ownWindow);
    m_dirty = false;
    m_lastRenderMs = GetTickCount();        // for the secondary-window present throttle

    // Reveal a deferred (load-time) window now that its content is drawn — but
    // only once the load has actually finished, so it still can't flash mid-load.
    // We're past the present, so the first visible frame already has full content.
    if (m_deferShow) {
        bool stillLoading = (m_detachOwner && m_detachOwner->GetCreateStatus()
                             && m_detachOwner->GetCreateStatus()->IsVisible());
        if (!stillLoading) {
            SDL_ShowWindow(m_ownWindow);
            SDL_RaiseWindow(m_ownWindow);
            m_deferShow = false;
        }
    }
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
            // Remember the latest target so mouse-up can apply the exact final
            // size, then push to SDL at ~10 Hz. Each push fires SIZE_CHANGED,
            // which rebuilds the map DIB — too slow to run on every one of the
            // ~125 motion events/sec: doing so floods the event queue so the
            // main loop never drains it and never renders, so the window only
            // visibly resizes on mouse-up. Throttling lets the queue drain and
            // the loop repaint the content between steps for live feedback.
            m_dPendWX = nx; m_dPendWY = ny; m_dPendWW = nw; m_dPendWH = nh;
            m_dPendValid = true;
            uint32_t now = SDL_GetTicks();
            if (now - m_dLastApply >= 100) {
                m_dLastApply = now;
                SDL_SetWindowPosition(m_ownWindow, nx, ny);
                SDL_SetWindowSize(m_ownWindow, nw, nh);
                m_dPendValid = false;
            }
            return true;
        }
        if (event.type == SDL_MOUSEBUTTONUP) {
            // Apply the exact final size even if the last motion was throttled.
            if (m_dPendValid) {
                SDL_SetWindowPosition(m_ownWindow, m_dPendWX, m_dPendWY);
                SDL_SetWindowSize(m_ownWindow, m_dPendWW, m_dPendWH);
                m_dPendValid = false;
            }
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
        m_dLastApply = 0;       // first motion applies immediately
        m_dPendValid = false;
        SDL_CaptureMouse(SDL_TRUE);
        return true;
    }
    return false;
}

// Manual title-bar drag of the borderless own-window. Mirrors HandleDetachedResize:
// capture the mouse and drive SDL_SetWindowPosition from the global cursor delta,
// so we never enter the OS modal move loop (which would freeze the render loop).
// Resize is checked BEFORE this (edges win), so by here an edge press has already
// been claimed. Returns true if the event was consumed.
bool SDL2Panel::HandleDetachedDrag(SDL_Event& event) {
    if (!m_ownWindow) return false;

    if (m_dDragging) {
        if (event.type == SDL_MOUSEMOTION) {
            int gx = 0, gy = 0;
            SDL_GetGlobalMouseState(&gx, &gy);
            SDL_SetWindowPosition(m_ownWindow,
                                  m_dDragStartWX + (gx - m_dDragStartMX),
                                  m_dDragStartWY + (gy - m_dDragStartMY));
            return true;
        }
        if (event.type == SDL_MOUSEBUTTONUP) {
            m_dDragging = false;
            SDL_CaptureMouse(SDL_FALSE);
            return true;
        }
        return true;  // swallow other events mid-drag
    }

    if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT)
        return false;

    int w = 0, h = 0;
    SDL_GetWindowSize(m_ownWindow, &w, &h);
    int tbH  = GetTitleBarHeight();
    int grip = SDL2Panel::RESIZE_BORDER;
    int px = event.button.x, py = event.button.y;

    // Title-bar band only, excluding the resize grips (handled earlier) and the
    // caption buttons (min/max/close act on their own mouse-up — starting a drag
    // here would swallow that mouse-up and the button would look dead). The
    // button geometry matches the old DetachedPanelHitTest / DrawTitleBar layout.
    if (tbH <= 0 || py < grip || py >= tbH || px < grip || px >= w - grip)
        return false;
    {
        int byTop  = (tbH - SDL2Panel::TITLE_BTN_H) / 2;
        int closeX = w - 2 - SDL2Panel::TITLE_BTN_W;
        int minX   = closeX - 2 * (SDL2Panel::TITLE_BTN_W + 1);
        if (py >= byTop && py < byTop + SDL2Panel::TITLE_BTN_H &&
            px >= minX && px < closeX + SDL2Panel::TITLE_BTN_W)
            return false;  // on a caption button — let the click reach HandleEvent
    }

    SDL_GetGlobalMouseState(&m_dDragStartMX, &m_dDragStartMY);
    SDL_GetWindowPosition(m_ownWindow, &m_dDragStartWX, &m_dDragStartWY);
    m_dDragging = true;
    SDL_CaptureMouse(SDL_TRUE);
    return true;
}
