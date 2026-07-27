#include "stdafx.h"

#include "SDL2CreateStatus.h"
#include "SDL2UI.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "SDL2Compositor.h"
#include "bitmaps.h"
#include "player.h"
#include "lastplnt.h"
#include "netcmd.h"
#include "error.h"
#include "help.h"
#include "EnSettings.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_syswm.h>
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

#ifdef __APPLE__
// macOS only commits window/layer updates to the screen when the main runloop
// reaches its idle (before-waiting) phase — which a busy world-generation loop
// never does, even though it pumps SDL events. So every paint of this dialog
// (and of the main window) piled up uncommitted and the whole app appeared
// frozen until the load finished (operator: "no progress dialog, it freezes,
// then the dialog flashes at the end"). Explicitly flushing CoreAnimation's
// implicit transaction after each visual paint pushes the frames to the glass.
// The objc runtime is declared by hand: <objc/message.h> can't be included in
// this TU (its BOOL typedef collides with the Win32 shim's), and QuartzCore is
// already loaded via SDL's Metal backend so the class resolves at runtime.
extern "C" {
    void* objc_getClass(const char* name);
    void* sel_registerName(const char* name);
    void  objc_msgSend(void);
}
static void FlushCoreAnimation() {
    void* ca = objc_getClass("CATransaction");
    if (ca)
        ((void (*)(void*, void*))objc_msgSend)(ca, sel_registerName("flush"));
}
#endif

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
    m_fontPath = EnResolveFontPath();   // T-0073: one shared resolver, bundled font first
    LogStatus("SDL2CreateStatus created");
}

SDL2CreateStatus::~SDL2CreateStatus() {
    // Tear down the always-on-top SDL window if it's still up — otherwise it
    // outlives the dialog object and lingers over the game (the load flow
    // never calls SetPer(PER_DONE) → Hide(), so when LetsGo() deletes
    // m_pCreateGame the C++ object dies but the OS window persists).
    if (m_ownWindow) {
        SDL_DestroyWindow(m_ownWindow);
        m_ownWindow = nullptr;
    }
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
#if defined(__linux__)
        // Transient-for the game window: Mutter focuses it silently instead of a
        // "window is ready" toast (#33) and it stacks above the game only.
        extern void EnSetX11TransientFor(SDL_Window* panel, SDL_Window* owner);
        if (m_ownWindow && m_gameWindow->GetWindow())
            EnSetX11TransientFor(m_ownWindow, m_gameWindow->GetWindow());
#endif
    }

    m_visible = true;

    // Always bring to front — NOT just when the window is first created. In the
    // create flow the window already exists (ToWorld showed it) when our later
    // ShowDlgStatus runs *after* DestroyMain raised the game window over us. A
    // z-order-only SetWindowPos(TOPMOST) (the per-frame pin) leaves us painted
    // over by the GPU-rendered game window; an activating SDL_RaiseWindow forces
    // the window (and DWM compositing) to actually bring us to the foreground.
    if (m_ownWindow)
        SDL_RaiseWindow(m_ownWindow);

    LogStatus("Show");
    m_lastRenderMs = 0;      // ensure the first frame after Show paints (no throttle)
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
    SetPer(percent, FALSE);
}

void SDL2CreateStatus::SetPer(int percent, BOOL bYield) {
    // Match CDlgCreateStatus::SetPer(int,BOOL) semantics.
    if (bYield) {
        if (m_cancelled)
            ThrowError(ERR_TLP_QUIT);
        theApp.BaseYield();
        theGame.ProcessAllMessages();
        if (m_cancelled)
            ThrowError(ERR_TLP_QUIT);
    }

    percent = percent < -1 ? -1 : (percent > 100 ? 100 : percent);
    // Don't go backward (except resetting to 0).
    if ((m_percent >= percent) && (percent != 0))
        return;

    // Tell other players our new status.
    if (theGame.IsNetGame() && theGame.HaveHP()) {
        CNetPlyrStatus msg(theGame.GetMe()->GetNetNum(), percent);
        theGame.PostToAll(&msg, sizeof(msg), FALSE);
    }

    m_percent = (percent == -1) ? 0 : percent;

    if (m_percent >= PER_DONE) {
        Hide();
        return;
    }

    // Paint the update NOW (throttled to ~15fps inside Render). Several load
    // phases call the non-yielding SetPer from loops that never reach
    // PollEvents — StartGame's AI-start loop is one — so without this the
    // dialog was shown but never painted a single frame there: the operator
    // saw a frozen screen through the whole post-pick-player hang. Render is
    // display-only (no event pumping / message processing), so it's safe from
    // any game-logic phase on the main thread.
    if (!bYield && m_visible)
        Render();
}

void SDL2CreateStatus::SetStatus() {
    // Matches CDlgCreateStatus::SetStatus(): reset to "Creating World..." 0%
    SetPer(0);
    SetMsg(IDS_CREATE_WORLD);
}

void SDL2CreateStatus::Render() {
    if (!m_visible || !m_gameWindow || !m_gameWindow->GetWindow())
        return;

    // Maintain z-order EVERY frame — BEFORE the visual throttle below. The
    // drawing is throttled to ~15fps so it doesn't slow world creation, but the
    // throttle must NOT gate z-order: during the multi-second load the game
    // creates/raises other windows (detached panels, dialogs) ~60x/sec, and if we
    // only re-pin on the throttled redraws, a window wedges between the background
    // and this loading dialog in the gaps — a visible flicker, with the
    // background appearing "sent far back". Keep this loading window topmost and
    // the main background pinned directly beneath it on every single frame.
    //
    // WITHOUT stealing OS focus: the old per-frame SDL_RaiseWindow() called
    // SetForegroundWindow() ~60x/sec (couldn't alt-tab away during gen). Instead
    // bump z-order with SWP_NOACTIVATE, and only while our own process is
    // foreground (so we never fight another app the user switched to mid-load).
#ifdef _WIN32
    // Rate-limit to ~30Hz: "every frame" was designed against the ~60fps frame
    // loop, but SetPer() now calls Render() directly from tight load loops
    // (unbounded call rate), and unthrottled GetForegroundWindow + 2x
    // SetWindowPos at thousands/sec slows world creation and hammers the WM.
    // 33ms still re-pins as fast as windows are created/raised (~60Hz worst
    // case sees at most one 1-frame gap). The mac block below has its own 2Hz
    // limit already; the VISUAL throttle further down stays at 66ms.
    static DWORD s_lastZOrderMs = 0;
    DWORD nowZ = ::timeGetTime();
    bool bZOrderDue = (nowZ - s_lastZOrderMs >= 33);
    if (bZOrderDue) s_lastZOrderMs = nowZ;
    if (m_ownWindow && bZOrderDue) {
        SDL_SysWMinfo wm; SDL_VERSION(&wm.version);
        if (SDL_GetWindowWMInfo(m_ownWindow, &wm)) {
            HWND h = wm.info.win.window;
            DWORD forePid = 0;
            ::GetWindowThreadProcessId(::GetForegroundWindow(), &forePid);
            bool mineFg = (forePid == ::GetCurrentProcessId());

            if (mineFg) {
                // Keep this dialog topmost...
                ::SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0,
                               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                // ...and pin the main background to the top of the NON-topmost
                // band with HWND_TOP: forward enough that a foreign window can't
                // sandwich between it and this dialog (the load-flow symptom when
                // this was removed), but below this topmost dialog. The main window
                // is non-topmost, so HWND_TOP doesn't change its topmost state — no
                // per-frame topmost bounce, no flicker.
                SDL_Window* mainWin = m_gameWindow->GetWindow();
                SDL_SysWMinfo mw; SDL_VERSION(&mw.version);
                if (mainWin && SDL_GetWindowWMInfo(mainWin, &mw))
                    ::SetWindowPos(mw.info.win.window, HWND_TOP, 0, 0, 0, 0,
                                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
    }
#elif defined(__APPLE__)
    // macOS needs the same z-order maintenance the Windows block above does:
    // the in-game windows (Area Map, Radar, panels) are ALWAYS_ON_TOP on mac and
    // anything created/raised during the load would order above this (equally
    // always-on-top but earlier) dialog. SDL has no raise-without-activate, so
    // re-raise at a gentle ~2Hz instead of per frame (SDL_RaiseWindow takes key
    // focus on macOS), and only while THIS app holds input focus so we never
    // fight a foreground app the user switched to mid-load. (Linux panels are
    // WM_TRANSIENT_FOR-owned, not always-on-top — it needs none of this.)
    if (m_ownWindow && SDL_GetKeyboardFocus() != nullptr) {
        DWORD nowRaise = ::timeGetTime();
        if (nowRaise - m_lastRaiseMs >= 500) {
            m_lastRaiseMs = nowRaise;
            SDL_RaiseWindow(m_ownWindow);
        }
    }
#endif

    // Throttle the VISUAL redraw to ~15fps to avoid slowing down world creation.
    // BaseYield() calls this hundreds of times during loading. PER-INSTANCE timer
    // (reset in Show) so a newly shown window paints its first frame immediately.
    DWORD now = ::timeGetTime();
    if (m_lastRenderMs != 0 && now - m_lastRenderMs < 66)  // ~15fps
        return;
    m_lastRenderMs = now;

    // Render into our own always-on-top window
    SDL_Surface* winSurface = m_ownWindow ? SDL_GetWindowSurface(m_ownWindow) : nullptr;
    if (!winSurface) return;

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
    // Carved-gold frame via the shared, corner-correct routine (the old per-edge
    // loops left the same bottom-corner gaps fixed elsewhere).
    SDL2MainMenu::DrawGoldBorder(winSurface, dlgRect.x, dlgRect.y,
                                 dlgRect.w, dlgRect.h, s_bdrH, s_bdrV);
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

    // ...and also onto the game window itself, so it's visible on a GPU host where
    // the own-window above is hidden by the airspace problem.
    RenderToGameWindow();

#ifdef __APPLE__
    FlushCoreAnimation();   // push this frame to the screen despite the busy loop
#endif
}

void SDL2CreateStatus::RenderToGameWindow() {
    if (!m_gameWindow) return;
    SDL2Compositor* comp = m_gameWindow->GetCompositor();
    SDL_Surface*    surf = m_gameWindow->GetPresentSurface();
    if (!comp || !surf) return;

    // Clean background each frame (the wallpaper) so changing messages don't smear.
    comp->RenderWallpaper(surf);

    int winW = surf->w, winH = surf->h;
    SDL_Color gold  = { 225, 182, 55, 255 };
    SDL_Color shade = {  20,  16, 40, 255 };

    TTF_Font* big = GetFont(34);
    if (big && !m_message.empty()) {
        // 1px drop-shadow for readability over the tiled wallpaper.
        SDL_Rect sh = { winW / 4 + 2, winH / 2 - 44 + 2, winW / 2, 44 };
        RenderTextC(surf, big, m_message.c_str(), sh, shade, true, true);
        SDL_Rect r  = { winW / 4,     winH / 2 - 44,     winW / 2, 44 };
        RenderTextC(surf, big, m_message.c_str(), r, gold, true, true);
    }
    TTF_Font* med = GetFont(24);
    if (med && m_percent >= 0) {
        char buf[16]; sprintf_s(buf, "%d%%", m_percent);
        SDL_Rect r = { winW / 4, winH / 2 + 10, winW / 2, 32 };
        RenderTextC(surf, med, buf, r, gold, true, true);
    }

    m_gameWindow->PresentSurface();
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
                // The dialog now outlives game-start into the LetsGo tail
                // (progress covers the whole start, incl. the post-pick AI
                // spin-up) — by then the state machine is already at play and
                // a cancel would CloseWorld a LIVE game. Swallow it; the
                // dialog is torn down moments later anyway.
                if (theGame.GetState() == CGame::play)
                    return true;
                // Match CDlgCreateStatus::OnCancel — confirm before quitting.
                int idsQuit;
                if (theGame._GetMe() == NULL)
                    idsQuit = IDS_SERVER_QUIT;
                else if (theGame.AmServer())
                    idsQuit = theGame.IsNetGame() ? IDS_SERVER_QUIT : IDS_SINGLE_QUIT;
                else
                    idsQuit = IDS_CLIENT_QUIT;

                if (EnMessageBox(idsQuit, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL) != IDYES)
                    return true;

                m_cancelled = true;
                LogStatus("Cancel clicked (confirmed)");
                if (!theGame.GetTry())
                    theApp.CloseWorld();
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
