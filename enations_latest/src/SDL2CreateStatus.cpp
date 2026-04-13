#include "stdafx.h"

#include "SDL2CreateStatus.h"
#include "SDL2UI.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "SDL2Compositor.h"
#include "bitmaps.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <fstream>

#undef min
#undef max
#include <algorithm>

static void LogStatus(const std::string& msg) {
    std::ofstream log("SDL2CreateStatus.log", std::ios::app);
    if (log.is_open()) {
        log << msg << std::endl;
    }
}

// UI color constants — matching the game dialog style (dark purple title, gold bg, blue text)
namespace StatusColors {
    const SDL_Color DialogBg    = { 60,  65,  62,  255 };   // fallback bg
    const SDL_Color DialogFrame = { 103, 127, 121, 255 };   // fallback frame
    const SDL_Color DialogDark  = {  38,  22,  72,  255 };  // dark purple shadow
    const SDL_Color TitleBg     = {  42,  22,  65,  255 };  // dark purple title bar
    const SDL_Color TitleText   = { 255, 255, 255, 255 };   // white title
    const SDL_Color LabelText   = {  48,  58, 148, 255 };   // blue label text
    const SDL_Color BtnFace     = {  68,  55, 135, 255 };   // blueish-purple button
    const SDL_Color BtnLight    = { 115,  98, 195, 255 };   // lighter purple
    const SDL_Color BtnDark     = {  32,  22,  72, 255 };   // darker purple
    const SDL_Color BtnText     = { 225, 182,  55, 255 };   // gold button text
    const SDL_Color BtnPressed  = {  48,  38,  98, 255 };   // pressed button
    const SDL_Color BarBg       = { 200, 200, 200, 255 };   // light gray bar bg
    const SDL_Color BarFill     = {  48,  58, 148, 255 };   // blue progress fill
    const SDL_Color BarFrame    = {  32,  22,  72, 255 };   // dark purple bar frame
}

static void FillRectC(SDL_Surface* dst, SDL_Rect r, SDL_Color c) {
    SDL_FillRect(dst, &r, SDL_MapRGB(dst->format, c.r, c.g, c.b));
}

static void DrawBevelC(SDL_Surface* dst, SDL_Rect r, int width, SDL_Color light, SDL_Color dark) {
    for (int i = 0; i < width; i++) {
        SDL_Rect top = { r.x + i, r.y + i, r.w - 2*i, 1 };
        SDL_Rect left = { r.x + i, r.y + i, 1, r.h - 2*i };
        SDL_Rect bottom = { r.x + i, r.y + r.h - 1 - i, r.w - 2*i, 1 };
        SDL_Rect right = { r.x + r.w - 1 - i, r.y + i, 1, r.h - 2*i };
        Uint32 lc = SDL_MapRGB(dst->format, light.r, light.g, light.b);
        Uint32 dc = SDL_MapRGB(dst->format, dark.r, dark.g, dark.b);
        SDL_FillRect(dst, &top, lc);
        SDL_FillRect(dst, &left, lc);
        SDL_FillRect(dst, &bottom, dc);
        SDL_FillRect(dst, &right, dc);
    }
}

static void RenderTextC(SDL_Surface* dst, TTF_Font* font, const char* text,
                        SDL_Rect rect, SDL_Color color, bool centerH = true, bool centerV = true) {
    if (!font || !text || !text[0]) return;
    SDL_Surface* textSurf = TTF_RenderText_Blended(font, text, color);
    if (!textSurf) return;

    SDL_Rect dstRect = rect;
    if (centerH) dstRect.x += (rect.w - textSurf->w) / 2;
    if (centerV) dstRect.y += (rect.h - textSurf->h) / 2;
    dstRect.w = textSurf->w;
    dstRect.h = textSurf->h;
    SDL_BlitSurface(textSurf, nullptr, dst, &dstRect);
    SDL_FreeSurface(textSurf);
}

SDL2CreateStatus::SDL2CreateStatus(GameWindow* gameWindow)
    : m_gameWindow(gameWindow)
    , m_visible(false)
    , m_cancelled(false)
    , m_percent(0)
    , m_dlgW(400)
    , m_dlgH(160)
    , m_btnCancelRect{0, 0, 0, 0}
    , m_btnPressed(false)
{
    // Cancel button in own-window local coords
    int btnW = 90, btnH = 28;
    m_btnCancelRect = { (m_dlgW - btnW) / 2,
                        m_dlgH - btnH - 15,
                        btnW, btnH };

    // Find a font
    const char* candidates[] = {
        "C:\\Windows\\Fonts\\BKANT.TTF",
        "C:\\Windows\\Fonts\\times.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        nullptr
    };
    for (int i = 0; candidates[i]; i++) {
        FILE* f = fopen(candidates[i], "rb");
        if (f) { fclose(f); m_fontPath = candidates[i]; break; }
    }

    LogStatus("SDL2CreateStatus created");
}

SDL2CreateStatus::~SDL2CreateStatus() {
    for (auto& pair : m_fontCache)
        if (pair.second) TTF_CloseFont(pair.second);
    LogStatus("SDL2CreateStatus destroyed");
}

TTF_Font* SDL2CreateStatus::GetFont(int size) {
    if (m_fontPath.empty()) return nullptr;
    auto it = m_fontCache.find(size);
    if (it != m_fontCache.end()) return it->second;
    TTF_Font* font = TTF_OpenFont(m_fontPath.c_str(), size);
    m_fontCache[size] = font;
    return font;
}

void SDL2CreateStatus::Show() {
    m_cancelled = false;

    if (!m_ownWindow) {
        // Centre on the main window
        int mainX = 0, mainY = 0, mainW = 800, mainH = 600;
        if (m_gameWindow->GetWindow()) {
            SDL_GetWindowPosition(m_gameWindow->GetWindow(), &mainX, &mainY);
            SDL_GetWindowSize(m_gameWindow->GetWindow(), &mainW, &mainH);
        }
        int wx = mainX + (mainW - m_dlgW) / 2;
        int wy = mainY + (mainH - m_dlgH) / 2;

        m_ownWindow = GameWindow::CreateSDLWindow(
            "Enemy Nations",
            wx, wy, m_dlgW, m_dlgH,
            SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP
                | SDL_WINDOW_SKIP_TASKBAR);

        LogStatus(m_ownWindow ? "Created own window" : "Failed to create own window");
        if (m_ownWindow)
            SDL_RaiseWindow(m_ownWindow);
    }

    m_visible = true;
    LogStatus("Show");
}

void SDL2CreateStatus::Hide() {
    m_visible = false;
    if (m_ownWindow) {
        SDL_DestroyWindow(m_ownWindow);
        m_ownWindow = nullptr;
    }
    LogStatus("Hide");
}

void SDL2CreateStatus::SetMsg(const std::string& text) {
    m_message = text;
}

void SDL2CreateStatus::SetMsg(int stringResourceID) {
    std::string str = EnLoadStdString(stringResourceID);
    if (!str.empty())
        m_message = str;
    else
        m_message = "Loading...";
}

void SDL2CreateStatus::SetPer(int percent) {
    if (percent < -1) percent = -1;
    if (percent > 100) percent = 100;
    if (percent == -1) percent = 0;
    m_percent = percent;
}

void SDL2CreateStatus::Render() {
    if (!m_visible || !m_gameWindow || !m_gameWindow->GetWindow())
        return;

    // Throttle rendering to ~15fps to avoid slowing down world creation.
    // BaseYield() calls this hundreds of times during loading.
    static DWORD s_lastRender = 0;
    DWORD now = ::timeGetTime();
    if (now - s_lastRender < 66)  // ~15fps
        return;
    s_lastRender = now;

    // Render into our own ALWAYS_ON_TOP window
    SDL_Surface* winSurface = m_ownWindow ? SDL_GetWindowSurface(m_ownWindow) : nullptr;
    if (!winSurface) return;

    // Re-raise each frame to stay on top of other topmost windows (detached panels, etc.)
    SDL_RaiseWindow(m_ownWindow);

    // Ensure dialog art is loaded
    SDL2Dialog::LoadDialogArt();

    // Dialog background using game art (matching SDL2Dialog::Render)
    SDL_Rect dlgRect = { 0, 0, m_dlgW, m_dlgH };

    // Draw game art border + interior background
    // Gold background fill for border area
    static SDL_Surface* s_gold = nullptr;
    static SDL_Surface* s_bkgnd = nullptr;
    static SDL_Surface* s_bdrH = nullptr;
    static SDL_Surface* s_bdrV = nullptr;
    static bool s_loaded = false;
    if (!s_loaded) {
        s_loaded = true;
        auto convert = [](int idx) -> SDL_Surface* {
            CDIB* p = theBitmaps.GetByIndex(idx);
            return p ? SDL2MainMenu::CreateSurfaceFromDIB(p) : nullptr;
        };
        s_gold  = convert(DIB_GOLD);
        s_bkgnd = convert(CBitmapLib::DLG_BKGND);
        s_bdrH  = convert(DIB_BORDER_HORZ);
        s_bdrV  = convert(DIB_BORDER_VERT);
    }

    int borderTop = s_bdrH ? s_bdrH->h : 3;
    int borderSide = s_bdrV ? s_bdrV->w : 3;

    if (s_gold) {
        SDL_BlitScaled(s_gold, nullptr, winSurface, &dlgRect);
    }
    if (s_bdrH) {
        for (int x = dlgRect.x; x < dlgRect.x + dlgRect.w; x += s_bdrH->w) {
            int w = std::min(s_bdrH->w, dlgRect.x + dlgRect.w - x);
            SDL_Rect src = { 0, 0, w, s_bdrH->h };
            SDL_Rect dTop = { x, dlgRect.y, w, s_bdrH->h };
            SDL_BlitSurface(s_bdrH, &src, winSurface, &dTop);
            SDL_Rect dBot = { x, dlgRect.y + dlgRect.h - s_bdrH->h, w, s_bdrH->h };
            SDL_BlitSurface(s_bdrH, &src, winSurface, &dBot);
        }
    }
    if (s_bdrV) {
        for (int col = 0; col < s_bdrV->w; col++) {
            SDL_Rect src = { col, col, 1, s_bdrV->h - 2 * col };
            int stripH = dlgRect.h - 2 * borderTop - 2 * col;
            if (stripH <= 0) continue;
            SDL_Rect dL = { dlgRect.x + col, dlgRect.y + borderTop + col, 1, stripH };
            SDL_BlitScaled(s_bdrV, &src, winSurface, &dL);
            SDL_Rect dR = { dlgRect.x + dlgRect.w - 1 - col, dlgRect.y + borderTop + col, 1, stripH };
            SDL_BlitScaled(s_bdrV, &src, winSurface, &dR);
        }
    }
    // Interior: stretch gold texture (matching SDL2Dialog style)
    {
        SDL_Rect interior = { dlgRect.x + borderSide, dlgRect.y + borderTop,
                              dlgRect.w - 2 * borderSide, dlgRect.h - 2 * borderTop };
        if (s_gold)
            SDL_BlitScaled(s_gold, nullptr, winSurface, &interior);
    }

    // Title bar: dark purple strip at top of interior
    const int titleBarH = 26;
    SDL_Rect titleBarRect = { 0 + borderSide, 0 + borderTop,
                              m_dlgW - 2 * borderSide, titleBarH };
    FillRectC(winSurface, titleBarRect, StatusColors::TitleBg);

    TTF_Font* titleFont = GetFont(14);
    if (titleFont) {
        SDL_Rect titleTextRect = { titleBarRect.x + 6, titleBarRect.y,
                                   titleBarRect.w - 12, titleBarRect.h };
        RenderTextC(winSurface, titleFont, "Enemy Nations", titleTextRect,
                    StatusColors::TitleText, false, true);
    }

    // Message text (below title bar)
    int contentTop = 0 + borderTop + titleBarH + 6;
    TTF_Font* msgFont = GetFont(14);
    if (msgFont && !m_message.empty()) {
        SDL_Rect msgRect = { 0 + 20, contentTop, m_dlgW - 40, 24 };
        RenderTextC(winSurface, msgFont, m_message.c_str(), msgRect,
                    StatusColors::LabelText, true, true);
    }

    // Progress bar
    SDL_Rect barRect = { 0 + 20, contentTop + 30, m_dlgW - 40, 22 };
    RenderProgressBar(winSurface, barRect, m_percent);

    // Percentage text on the bar
    if (msgFont) {
        char perText[16];
        sprintf_s(perText, "%d%%", m_percent);
        RenderTextC(winSurface, msgFont, perText, barRect,
                    StatusColors::TitleText, true, true);
    }

    // Cancel button — horizontal gradient matching dialog button style
    {
        SDL_Color edge   = StatusColors::BtnDark;
        SDL_Color center = m_btnPressed ? StatusColors::BtnPressed : StatusColors::BtnLight;
        float halfW = m_btnCancelRect.w / 2.0f;
        for (int i = 0; i < m_btnCancelRect.w; i++) {
            float t = 1.0f - std::abs(i - halfW + 0.5f) / halfW;
            t = t * t;
            Uint8 rc = (Uint8)(edge.r + (center.r - edge.r) * t);
            Uint8 gc = (Uint8)(edge.g + (center.g - edge.g) * t);
            Uint8 bc = (Uint8)(edge.b + (center.b - edge.b) * t);
            SDL_Rect col = { m_btnCancelRect.x + i, m_btnCancelRect.y, 1, m_btnCancelRect.h };
            SDL_FillRect(winSurface, &col, SDL_MapRGB(winSurface->format, rc, gc, bc));
        }
        if (m_btnPressed)
            DrawBevelC(winSurface, m_btnCancelRect, 2, StatusColors::BtnDark, StatusColors::BtnLight);
        else
            DrawBevelC(winSurface, m_btnCancelRect, 1, StatusColors::BtnLight, StatusColors::BtnDark);
    }
    if (msgFont) {
        RenderTextC(winSurface, msgFont, "Cancel", m_btnCancelRect,
                    StatusColors::BtnText, true, true);
    }

    SDL_UpdateWindowSurface(m_ownWindow);
}

bool SDL2CreateStatus::HandleEvent(const SDL_Event& event) {
    if (!m_visible || !m_ownWindow) return false;

    uint32_t ownID = SDL_GetWindowID(m_ownWindow);
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.windowID != ownID) return false;
        if (event.button.button == SDL_BUTTON_LEFT) {
            int mx = event.button.x, my = event.button.y;
            if (mx >= m_btnCancelRect.x && mx < m_btnCancelRect.x + m_btnCancelRect.w &&
                my >= m_btnCancelRect.y && my < m_btnCancelRect.y + m_btnCancelRect.h) {
                m_btnPressed = true;
                return true;
            }
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (event.button.windowID != ownID) return false;
        if (event.button.button == SDL_BUTTON_LEFT && m_btnPressed) {
            m_btnPressed = false;
            int mx = event.button.x, my = event.button.y;
            if (mx >= m_btnCancelRect.x && mx < m_btnCancelRect.x + m_btnCancelRect.w &&
                my >= m_btnCancelRect.y && my < m_btnCancelRect.y + m_btnCancelRect.h) {
                m_cancelled = true;
                LogStatus("Cancel clicked");
                return true;
            }
        }
        break;
    }
    return false;
}

void SDL2CreateStatus::RenderProgressBar(SDL_Surface* dst, SDL_Rect rect, int percent) {
    // Background
    FillRectC(dst, rect, StatusColors::BarBg);

    // Fill
    if (percent > 0) {
        int fillW = (rect.w - 4) * std::min(percent, 100) / 100;
        SDL_Rect fillRect = { rect.x + 2, rect.y + 2, fillW, rect.h - 4 };
        FillRectC(dst, fillRect, StatusColors::BarFill);
    }

    // Frame
    DrawBevelC(dst, rect, 1, StatusColors::DialogDark, StatusColors::BarFrame);
}
