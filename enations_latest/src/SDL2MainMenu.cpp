#include "stdafx.h"

#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "resource.h"
#include "creatsin.h"
#include "creatmul.h"
#include "join.h"
#include "scenario.h"
#include "SDL2Options.h"
#include "SDL2Dialogs.h"
#include "SDL2Video.h"
#include "sfx.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <fstream>
#include <string>
#include <vector>

static void LogMenu(const std::string& msg) {
    std::ofstream log("SDL2MainMenu.log", std::ios::app);
    if (log.is_open()) {
        log << msg << std::endl;
        log.close();
    }
}

// Button definitions: positions in wallpaper coordinates, matching CDlgMain's _btnData.
// Order matches the MISC data file chunk order (same as CDlgMain).
// Text rects (L,T,R,B) are from CDlgMain's _btnData[].rText — relative to button bitmap.
// Labels match the original .rc resource strings exactly
SDL2MainMenu::ButtonDef SDL2MainMenu::s_buttonDefs[NUM_BTNS] = {
    //                                                                    textL textT textR textB
    { IDC_MAIN_LOAD,     784,  25,  "Load Single Player Game",             18, 10, 213, 84 },
    { IDC_MAIN_OPTIONS,  1060, 28,  "Options",                             34,  9, 214, 86 },
    { IDC_MAIN_LOAD_MUL, 776,  135, "Load Network Game",                   26, 17, 214, 87 },
    { IDC_MAIN_CREDITS,  1052, 138, "View Credits",                        30, 16, 211, 82 },
    { IDC_MAIN_CAMPAIGN, 100,  489, "Training Grounds",                    57, 12, 181, 110 },
    { IDC_MAIN_SINGLE,   293,  490, "Create Single Player Game",           22, 12, 205, 112 },
    { IDC_MAIN_CREATE,   345,  632, "Create Network Game",                 25, 11, 140, 65 },
    { IDC_MAIN_JOIN,     326,  744, "Join Network Game",                   26, 14, 149, 78 },
    { IDC_MAIN_INTRO,    868,  407, "Replay Introduction",                 38, 38, 177, 93 },
    { IDCANCEL,          1021, 604, "Exit",                                41, 40, 186, 73 },
    { IDC_MINIMIZE,      1035, 703, "Minimize",                            59, 11, 201, 46 },
};

// Font path - Book Antiqua not commonly installed, fall back to Book Old Style or serif
static const char* FindFontPath() {
    // Try common Windows font paths
    static const char* candidates[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "C:\\Windows\\Fonts\\BKANT.TTF",     // Book Antiqua
        "C:\\Windows\\Fonts\\BOOKOS.TTF",    // Book Old Style
        "C:\\Windows\\Fonts\\PALBI.TTF",     // Palatino Bold Italic (similar serif)
        "C:\\Windows\\Fonts\\times.ttf",     // Times New Roman
        "C:\\Windows\\Fonts\\georgia.ttf",   // Georgia
        nullptr
    };
    for (int i = 0; candidates[i]; i++) {
        FILE* f = fopen(candidates[i], "rb");
        if (f) { fclose(f); return candidates[i]; }
    }
    return nullptr;
}

SDL2MainMenu::SDL2MainMenu() {
}

SDL2MainMenu::~SDL2MainMenu() {
    Shutdown();
}

bool SDL2MainMenu::Initialize(GameWindow* gameWindow) {
    if (m_initialized) return true;
    if (!gameWindow || !gameWindow->GetWindow()) return false;

    m_gameWindow = gameWindow;

    LogMenu("Loading main menu assets from MISC data file...");

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        LogMenu("WARNING: TTF_Init failed: " + std::string(TTF_GetError()));
    } else {
        const char* fontPath = FindFontPath();
        if (fontPath) {
            m_fontPath = fontPath;
            LogMenu("Using font: " + m_fontPath);
        } else {
            LogMenu("WARNING: No suitable font found");
        }
    }

    // Load assets from the MISC data file, same way CDlgMain::OnCreate does
    CMmio* pMmio = theDataFile.OpenAsMMIO("misc", "MISC");
    if (!pMmio) {
        LogMenu("ERROR: Failed to open MISC data file");
        return false;
    }

    bool hasBitmapMenu = false;
    try {
        pMmio->DescendRiff('M', 'I', 'S', 'C');
        try {
            pMmio->DescendList('M', 'N', theApp.m_szOtherBPS[0], theApp.m_szOtherBPS[1]);
            pMmio->DescendChunk('D', 'A', 'T', 'A');
            hasBitmapMenu = true;
        } catch (...) {
            delete pMmio;
            pMmio = theDataFile.OpenAsMMIO("misc", "MISC");
            pMmio->DescendRiff('M', 'I', 'S', 'C');
            pMmio->DescendList('W', 'L', theApp.m_szOtherBPS[0], theApp.m_szOtherBPS[1]);
            pMmio->DescendChunk('D', 'A', 'T', 'A');
            hasBitmapMenu = false;
        }

        // Load wallpaper
        CDIB* pWall = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                                ptrthebltformat->GetMemDirection());
        pWall->Load(*pMmio);
        m_wallWidth = pWall->GetWidth();
        m_wallHeight = pWall->GetHeight();
        m_surfWallpaper = CreateSurfaceFromDIB(pWall);
        delete pWall;

        if (!m_surfWallpaper) {
            LogMenu("ERROR: Failed to create wallpaper surface");
            delete pMmio;
            return false;
        }

        LogMenu("Menu background loaded: " + std::to_string(m_wallWidth) + "x" + std::to_string(m_wallHeight));

        // Load button bitmaps
        if (hasBitmapMenu) {
            pMmio->AscendChunk();
            for (int i = 0; i < NUM_BTNS; i++) {
                pMmio->DescendChunk('D', 'A', 'T', 'A');
                CDIB* pBtn = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                                       CBLTFormat::DIR_TOPDOWN);
                pBtn->Load(*pMmio);
                m_btnStripWidth[i] = pBtn->GetWidth();
                m_btnStripHeight[i] = pBtn->GetHeight();
                m_surfButtons[i] = CreateSurfaceFromDIB(pBtn);
                delete pBtn;
                pMmio->AscendChunk();
            }
        }
    } catch (...) {
        LogMenu("ERROR: Exception loading menu assets");
        delete pMmio;
        return false;
    }

    delete pMmio;
    pMmio = nullptr;

    // If we loaded MN24 (the full menu screen), also load WL24 (tiled wallpaper)
    // for use behind dialogs during new game creation, matching MFC CDlgMain behavior.
    // Must be done AFTER closing pMmio — theDataFile may not support concurrent readers.
    if (hasBitmapMenu) {
        CMmio* pMmioWL = nullptr;
        try {
            pMmioWL = theDataFile.OpenAsMMIO("misc", "MISC");
            if (pMmioWL) {
                pMmioWL->DescendRiff('M', 'I', 'S', 'C');
                pMmioWL->DescendList('W', 'L', theApp.m_szOtherBPS[0], theApp.m_szOtherBPS[1]);
                pMmioWL->DescendChunk('D', 'A', 'T', 'A');

                CDIB* pTile = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                                        ptrthebltformat->GetMemDirection());
                pTile->Load(*pMmioWL);
                m_surfTileWallpaper = CreateSurfaceFromDIB(pTile);
                delete pTile;

                if (m_surfTileWallpaper) {
                    LogMenu("Tiled wallpaper loaded: " + std::to_string(m_surfTileWallpaper->w) +
                            "x" + std::to_string(m_surfTileWallpaper->h));
                } else {
                    LogMenu("WARNING: Failed to create tiled wallpaper surface");
                }
                delete pMmioWL;
                pMmioWL = nullptr;
            }
        } catch (...) {
            LogMenu("WARNING: Could not load WL tiled wallpaper");
            delete pMmioWL;
        }
    }

    // Set button enabled states (matching CDlgMain::OnInitDialog)
    for (int i = 0; i < NUM_BTNS; i++)
        m_btnEnabled[i] = true;

    if (theApp.IsShareware() || theApp.IsSecondDisk()) {
        m_btnEnabled[0] = false;  // Load Game
        m_btnEnabled[2] = false;  // Load Multi
    }
    if (theApp.IsSecondDisk()) {
        m_btnEnabled[6] = false;  // Create Game
    }
    if (!theApp.HaveIntro()) {
        m_btnEnabled[8] = false;  // Intro
    }

    m_initialized = true;
    LogMenu("SDL2MainMenu initialized successfully");
    return true;
}

void SDL2MainMenu::Shutdown() {
    for (int i = 0; i < NUM_BTNS; i++) {
        if (m_surfButtons[i]) {
            SDL_FreeSurface(m_surfButtons[i]);
            m_surfButtons[i] = nullptr;
        }
    }
    if (m_surfWallpaper) {
        SDL_FreeSurface(m_surfWallpaper);
        m_surfWallpaper = nullptr;
    }
    if (m_surfTileWallpaper) {
        SDL_FreeSurface(m_surfTileWallpaper);
        m_surfTileWallpaper = nullptr;
    }
    for (auto& pair : m_fontCache) {
        if (pair.second) TTF_CloseFont(pair.second);
    }
    m_fontCache.clear();
    // Don't call TTF_Quit here - might be reinitialized later
    m_gameWindow = nullptr;
    m_initialized = false;
}

void SDL2MainMenu::DrawGoldBorder(SDL_Surface* dst, int x, int y, int w, int h,
                                 SDL_Surface* borderH, SDL_Surface* borderV) {
    if (!dst || (!borderH && !borderV) || w <= 0 || h <= 0)
        return;
    int brdH = borderH ? borderH->h : 4;

    // Top & bottom horizontal strips, tiled across the full width.
    if (borderH) {
        for (int tx = 0; tx < w; tx += borderH->w) {
            int bw = (borderH->w < w - tx) ? borderH->w : (w - tx);
            SDL_Rect sr = { 0, 0, bw, brdH };
            SDL_Rect dr = { x + tx, y, bw, brdH };
            SDL_BlitSurface(borderH, &sr, dst, &dr);
            dr.y = y + h - brdH;
            SDL_BlitSurface(borderH, &sr, dst, &dr);
        }
    }

    // Left & right vertical strips. Each column steps inward only 1px
    // (top+1+ix .. bottom-1-ix), so the outer columns run nearly full height
    // and overlap the top/bottom strips — filling the corners (no gap) — while
    // the inner columns form the 45-degree miter. Matches the original
    // PaintBorder (bmbutton.cpp).
    if (borderV) {
        for (int ix = 0; ix < borderV->w && ix < w / 2; ix++) {
            int top = y + 1 + ix;
            int bot = y + h - 1 - ix;
            if (top >= bot) break;
            for (int ty = top; ty < bot; ty += borderV->h) {
                int bh = (borderV->h < bot - ty) ? borderV->h : (bot - ty);
                SDL_Rect sr  = { ix, ix, 1, bh };
                SDL_Rect drL = { x + ix, ty, 1, bh };
                SDL_Rect drR = { x + w - 1 - ix, ty, 1, bh };
                SDL_BlitSurface(borderV, &sr, dst, &drL);
                SDL_BlitSurface(borderV, &sr, dst, &drR);
            }
        }
    }
}

SDL_Surface* SDL2MainMenu::CreateSurfaceFromDIB(CDIB* pDib) {
    if (!pDib) return nullptr;

    int width = pDib->GetWidth();
    int height = pDib->GetHeight();
    int bytesPerPixel = pDib->GetBytesPerPixel();
    int bitsPerPixel = pDib->GetBitsPerPixel();
    int pitch = pDib->GetPitch();

    if (width <= 0 || height <= 0 || pitch <= 0) return nullptr;

    CDIBits dibits = pDib->GetBits();
    BYTE* pPixels = (BYTE*)(dibits);
    if (!pPixels) return nullptr;

    SDL_Surface* tmpSurface = nullptr;
    if (bytesPerPixel == 3 || bytesPerPixel == 4) {
        Uint32 bmask = 0x000000FF;
        Uint32 gmask = 0x0000FF00;
        Uint32 rmask = 0x00FF0000;
        Uint32 amask = 0;
        tmpSurface = SDL_CreateRGBSurfaceFrom(pPixels, width, height, bitsPerPixel, pitch,
                                               rmask, gmask, bmask, amask);
    } else {
        tmpSurface = SDL_CreateRGBSurfaceFrom(pPixels, width, height, bitsPerPixel, pitch,
                                               0, 0, 0, 0);
    }

    if (!tmpSurface) return nullptr;

    // Copy to owned surface (DIB will be deleted after this)
    SDL_Surface* ownedSurface = SDL_ConvertSurfaceFormat(tmpSurface, SDL_PIXELFORMAT_BGR888, 0);
    SDL_FreeSurface(tmpSurface);

    if (!ownedSurface) return nullptr;

    // Magenta (255, 0, 255) = transparency, matching palette index 253
    SDL_SetColorKey(ownedSurface, SDL_TRUE,
                    SDL_MapRGB(ownedSurface->format, 255, 0, 255));

    return ownedSurface;
}

TTF_Font* SDL2MainMenu::GetCachedFont(int size) {
    if (m_fontPath.empty()) return nullptr;
    auto it = m_fontCache.find(size);
    if (it != m_fontCache.end()) return it->second;

    TTF_Font* font = TTF_OpenFont(m_fontPath.c_str(), size);
    m_fontCache[size] = font;
    return font;
}

void SDL2MainMenu::RenderTextShadowed(SDL_Surface* dst, TTF_Font* font, const char* text,
                                       SDL_Rect dstRect, SDL_Color fg, SDL_Color shadow,
                                       bool center) {
    if (!font || !text || !text[0] || !dst) return;

    // Greedy word-wrap into lines that fit dstRect.w. We do the wrapping here
    // (rather than via TTF_RenderText_Blended_Wrapped) so we can measure the
    // true block width — the widest line — and center the whole block while the
    // individual lines stay LEFT-aligned. The built-in wrapped renderer pads its
    // surface toward the wrap width, which defeats horizontal centering and made
    // the 2-line "Load Single Player Game" label hug the left edge.
    std::vector<std::string> lines;
    {
        std::string src(text);
        std::string line;
        size_t pos = 0;
        while (pos < src.size()) {
            // Pull the next word (including any leading spaces collapsed to one).
            size_t sp = src.find(' ', pos);
            std::string word = (sp == std::string::npos)
                                   ? src.substr(pos)
                                   : src.substr(pos, sp - pos);
            std::string candidate = line.empty() ? word : line + " " + word;
            int w = 0, h = 0;
            TTF_SizeUTF8(font, candidate.c_str(), &w, &h);
            if (w > dstRect.w && !line.empty()) {
                lines.push_back(line);
                line = word;
            } else {
                line = candidate;
            }
            pos = (sp == std::string::npos) ? src.size() : sp + 1;
        }
        if (!line.empty()) lines.push_back(line);
    }
    if (lines.empty()) return;

    // Measure the block: width = widest line, height = lineSkip per line.
    int lineSkip = TTF_FontLineSkip(font);
    int blockW = 0;
    std::vector<int> lineW(lines.size(), 0);
    for (size_t i = 0; i < lines.size(); i++) {
        int w = 0, h = 0;
        TTF_SizeUTF8(font, lines[i].c_str(), &w, &h);
        lineW[i] = w;
        if (w > blockW) blockW = w;
    }
    int blockH = lineSkip * (int)lines.size();

    // Origin of the block, centered within dstRect when requested.
    int originX = dstRect.x;
    int originY = dstRect.y;
    if (center) {
        if (blockW < dstRect.w) originX += (dstRect.w - blockW) / 2;
        if (blockH < dstRect.h) originY += (dstRect.h - blockH) / 2;
        // Nudge the block down a hair — pure vertical centering sat a touch high.
        originY += dstRect.h / 16;
    }

    // Render each line left-aligned at the block's left edge (originX).
    for (size_t i = 0; i < lines.size(); i++) {
        int y = originY + (int)i * lineSkip;

        SDL_Surface* shadowSurf = TTF_RenderUTF8_Blended(font, lines[i].c_str(), shadow);
        if (shadowSurf) {
            SDL_Rect shadowRect = { originX + 2, y + 2, 0, 0 };
            SDL_BlitSurface(shadowSurf, nullptr, dst, &shadowRect);
            SDL_FreeSurface(shadowSurf);
        }

        SDL_Surface* lineSurf = TTF_RenderUTF8_Blended(font, lines[i].c_str(), fg);
        if (lineSurf) {
            SDL_Rect lineRect = { originX, y, 0, 0 };
            SDL_BlitSurface(lineSurf, nullptr, dst, &lineRect);
            SDL_FreeSurface(lineSurf);
        }
    }
}

void SDL2MainMenu::ScreenToWallpaper(int screenX, int screenY, int& wallX, int& wallY) {
    if (!m_gameWindow || m_wallWidth <= 0 || m_wallHeight <= 0) {
        wallX = screenX;
        wallY = screenY;
        return;
    }
    int winW = m_gameWindow->GetWidth();
    int winH = m_gameWindow->GetHeight();
    wallX = (screenX * m_wallWidth) / winW;
    wallY = (screenY * m_wallHeight) / winH;
}

int SDL2MainMenu::HitTestButton(int screenX, int screenY) {
    int wallX, wallY;
    ScreenToWallpaper(screenX, screenY, wallX, wallY);

    for (int i = 0; i < NUM_BTNS; i++) {
        if (!m_surfButtons[i] || !m_btnEnabled[i]) continue;

        int btnW = m_btnStripWidth[i] / 3;
        int btnH = m_btnStripHeight[i];
        int btnX = s_buttonDefs[i].srcX;
        int btnY = s_buttonDefs[i].srcY;

        if (wallX >= btnX && wallX < btnX + btnW &&
            wallY >= btnY && wallY < btnY + btnH) {
            return i;
        }
    }
    return -1;
}

void SDL2MainMenu::TileWallpaper(SDL_Surface* dst) {
    // Use the WL24 tile surface; fall back to main wallpaper if WL24 wasn't loaded
    // (which happens when MN24 wasn't available and m_surfWallpaper IS the WL24 tile)
    SDL_Surface* tile = m_surfTileWallpaper ? m_surfTileWallpaper : m_surfWallpaper;
    if (!tile || !dst) return;
    int wallW = tile->w;
    int wallH = tile->h;
    if (wallW <= 0 || wallH <= 0) return;

    for (int y = 0; y < dst->h; y += wallH) {
        for (int x = 0; x < dst->w; x += wallW) {
            SDL_Rect srcRect = { 0, 0, wallW, wallH };
            SDL_Rect dstRect = { x, y, wallW, wallH };
            if (x + wallW > dst->w) srcRect.w = dst->w - x;
            if (y + wallH > dst->h) srcRect.h = dst->h - y;
            dstRect.w = srcRect.w;
            dstRect.h = srcRect.h;
            SDL_BlitSurface(tile, &srcRect, dst, &dstRect);
        }
    }
}

void SDL2MainMenu::RenderWallpaperOnly() {
    if (!m_initialized || !m_gameWindow) return;

    SDL_Surface* winSurface = m_gameWindow->GetPresentSurface();  // T0: software surface or renderer back-buffer
    if (!winSurface) return;

    // Tile the WL24 wallpaper, matching MFC CDlgMain behavior when m_bTile is TRUE.
    // This is what shows behind dialogs during new game creation.
    TileWallpaper(winSurface);

    m_gameWindow->PresentSurface();
}

void SDL2MainMenu::RenderWallpaperWithMessage(const char* msg) {
    if (!m_initialized || !m_gameWindow) return;

    SDL_Surface* winSurface = m_gameWindow->GetPresentSurface();
    if (!winSurface) return;

    TileWallpaper(winSurface);

    if (msg && msg[0]) {
        int winW = m_gameWindow->GetWidth();
        int winH = m_gameWindow->GetHeight();
        // Size the status font to the window so it reads at any resolution.
        TTF_Font* font = GetCachedFont(__max(18, winH / 28));
        if (font) {
            SDL_Color fg     = { 230, 215, 170, 255 };  // warm gold, matches menu text
            SDL_Color shadow = {  20,  20,  20, 255 };
            SDL_Rect box = { winW / 4, (winH - winH / 12) / 2, winW / 2, winH / 12 };
            RenderTextShadowed(winSurface, font, msg, box, fg, shadow, true);
        }
    }

    // Pin the main window forward (same reason as RenderWallpaperOnly's raise).
    if (m_gameWindow->GetWindow())
        SDL_RaiseWindow(m_gameWindow->GetWindow());

    m_gameWindow->PresentSurface();
}

void SDL2MainMenu::Render() {
    if (!m_initialized || !m_gameWindow) return;

    SDL_Surface* winSurface = m_gameWindow->GetPresentSurface();  // T0: software surface or renderer back-buffer
    if (!winSurface) return;

    int winW = m_gameWindow->GetWidth();
    int winH = m_gameWindow->GetHeight();

    // Draw wallpaper (stretched to fill)
    if (m_surfWallpaper) {
        SDL_Rect dstRect = { 0, 0, winW, winH };
        SDL_BlitScaled(m_surfWallpaper, nullptr, winSurface, &dstRect);
    }

    // Draw buttons
    for (int i = 0; i < NUM_BTNS; i++) {
        if (!m_surfButtons[i]) continue;

        int frameW = m_btnStripWidth[i] / 3;
        int frameH = m_btnStripHeight[i];

        int frameIndex = 0;
        if (!m_btnEnabled[i])
            frameIndex = 2;
        else if (m_pressedButton == i)
            frameIndex = 1;

        SDL_Rect srcRect = { frameIndex * frameW, 0, frameW, frameH };

        SDL_Rect dstRect;
        dstRect.x = (s_buttonDefs[i].srcX * winW) / m_wallWidth;
        dstRect.y = (s_buttonDefs[i].srcY * winH) / m_wallHeight;
        dstRect.w = (frameW * winW) / m_wallWidth;
        dstRect.h = (frameH * winH) / m_wallHeight;

        SDL_BlitScaled(m_surfButtons[i], &srcRect, winSurface, &dstRect);

        // Draw button text — shrink-to-fit like the original CDlgMain::OnDrawItem
        if (!m_fontPath.empty() && m_btnEnabled[i]) {
            SDL_Rect textRect;
            textRect.x = dstRect.x + (s_buttonDefs[i].textL * dstRect.w) / frameW;
            textRect.y = dstRect.y + (s_buttonDefs[i].textT * dstRect.h) / frameH;
            textRect.w = ((s_buttonDefs[i].textR - s_buttonDefs[i].textL) * dstRect.w) / frameW;
            textRect.h = ((s_buttonDefs[i].textB - s_buttonDefs[i].textT) * dstRect.h) / frameH;

            // Start at rect height and shrink until text fits within the rect.
            // Prefer a layout of at most 2 lines: with the wider DejaVu glyphs
            // (vs the original Arial) the plain "largest font that fits the box"
            // pick can land on 3-4 single-word lines (e.g. "Create Single Player
            // Game" stacked vertically). We instead choose the largest font whose
            // wrapped text fits in <= 2 lines, and only fall back to the raw
            // best-fit (any line count) if nothing fits in two.
            int fontSize = textRect.h;
            if (fontSize > 40) fontSize = 40;
            TTF_Font* btnFont = nullptr;
            TTF_Font* fallbackFont = nullptr;
            while (fontSize > 8) {
                TTF_Font* f = GetCachedFont(fontSize);
                if (f) {
                    int textW = 0, textH = 0;
                    SDL_Surface* testSurf = TTF_RenderText_Blended_Wrapped(f, s_buttonDefs[i].label,
                                                                            {255,255,255,255}, textRect.w);
                    if (testSurf) {
                        textW = testSurf->w;
                        textH = testSurf->h;
                        SDL_FreeSurface(testSurf);
                    }
                    if (textW <= textRect.w && textH <= textRect.h) {
                        if (!fallbackFont) fallbackFont = f;   // largest that fits at all
                        int lineSkip = TTF_FontLineSkip(f);
                        int lines = (lineSkip > 0) ? (textH + lineSkip - 1) / lineSkip : 1;
                        if (lines <= 2) { btnFont = f; break; }
                    }
                }
                fontSize -= 1;
            }
            if (!btnFont) btnFont = fallbackFont;

            if (btnFont) {
                SDL_Color fg = { 173, 156, 140, 255 };
                SDL_Color shadow = { 44, 53, 46, 255 };
                RenderTextShadowed(winSurface, btnFont, s_buttonDefs[i].label,
                                  textRect, fg, shadow);
            }
        }
    }

    // Draw title text - single line, sized to fit in the left portion of screen
    if (!m_fontPath.empty()) {
        const char* title = "Second Chance";
        // Target: title fits in left half of screen, single line
        int maxWidth = (winW * 4) / 8;

        // Start large and shrink until it fits on one line
        int fontSize = winH / 6;
        TTF_Font* titleFont = nullptr;
        int textW = 0, textH = 0;

        while (fontSize > 20) {
            titleFont = GetCachedFont(fontSize);
            if (titleFont) {
                TTF_SetFontStyle(titleFont, TTF_STYLE_BOLD);
                TTF_SizeText(titleFont, title, &textW, &textH);
                if (textW <= maxWidth) break;
            }
            fontSize -= 2;
            titleFont = nullptr;
        }

        if (titleFont) {
            SDL_Rect titleRect;
            titleRect.x = winW / 20;  // Close to left edge
            titleRect.y = textH / 3;
            titleRect.w = maxWidth;
            titleRect.h = textH;

            SDL_Color fg = { 90, 74, 57, 255 };
            SDL_Color shadow = { 144, 127, 116, 255 };
            RenderTextShadowed(winSurface, titleFont, title, titleRect, fg, shadow, false);
        }
    }

    m_gameWindow->PresentSurface();
}

bool SDL2MainMenu::HandleEvent(const SDL_Event& event) {
    if (!m_initialized) return false;

    switch (event.type) {
        case SDL_MOUSEMOTION: {
            int btn = HitTestButton(event.motion.x, event.motion.y);
            m_hoverButton = btn;
            return false;
        }

        case SDL_MOUSEBUTTONDOWN: {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int btn = HitTestButton(event.button.x, event.button.y);
                if (btn >= 0) {
                    m_pressedButton = btn;
                    return true;
                }
            }
            return false;
        }

        case SDL_MOUSEBUTTONUP: {
            if (event.button.button == SDL_BUTTON_LEFT && m_pressedButton >= 0) {
                int btn = HitTestButton(event.button.x, event.button.y);
                int pressed = m_pressedButton;
                m_pressedButton = -1;
                if (btn == pressed) {
                    OnButtonClick(btn);
                    return true;
                }
            }
            return false;
        }

        default:
            return false;
    }
}

void SDL2MainMenu::OnButtonClick(int buttonIndex) {
    if (buttonIndex < 0 || buttonIndex >= NUM_BTNS) return;
    if (!m_btnEnabled[buttonIndex]) return;

    LogMenu("Button clicked: " + std::string(s_buttonDefs[buttonIndex].label));

    theMusicPlayer.PlayForegroundSound(SOUNDS::GetID(SOUNDS::button), SFXPRIORITY::selected_pri);

    switch (s_buttonDefs[buttonIndex].id) {
        case IDC_MAIN_LOAD:
            if (!theApp.IsShareware() && !theApp.IsSecondDisk()) {
                if (!SDL2_RunLoadSinglePlayerFlow(m_gameWindow)) {
                    // User cancelled or error
                }
            }
            break;

        case IDC_MAIN_OPTIONS:
        {
            SDL2OptionsDialog dlg(m_gameWindow);
            dlg.DoModal();
            break;
        }

        case IDC_MAIN_LOAD_MUL:
            if (!theApp.IsShareware() && !theApp.IsSecondDisk()) {
                SDL2_RunLoadNetworkFlow(m_gameWindow);
            }
            break;

        case IDC_MAIN_CREDITS:
            SDL2_RunCredits(m_gameWindow);
            break;

        case IDC_MAIN_CAMPAIGN:
            if (!SDL2_RunCreateScenarioFlow(m_gameWindow)) {
                // User cancelled - stay on menu
            }
            break;

        case IDC_MAIN_SINGLE:
            if (!SDL2_RunCreateSinglePlayerFlow(m_gameWindow)) {
                // User cancelled - stay on menu
            }
            break;

        case IDC_MAIN_CREATE:
            if (!theApp.IsSecondDisk()) {
                SDL2_RunCreateNetworkFlow(m_gameWindow);
            }
            break;

        case IDC_MAIN_JOIN:
            SDL2_RunJoinNetworkFlow(m_gameWindow);
            break;

        case IDC_MAIN_INTRO:
            SDL2VideoPlayer::PlayVideo(m_gameWindow, "assets\\videos\\intro.mpg");
            break;

        case IDCANCEL:
            LogMenu("Exit triggered - IDCANCEL handler");
            theApp.DestroyMain();
            theApp.CloseApp();
            break;

        case IDC_MINIMIZE:
            theApp.Minimize();
            break;
    }
}
