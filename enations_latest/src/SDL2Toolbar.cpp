#include "stdafx.h"

#include "SDL2Toolbar.h"
#include "SDL2Panel.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "player.h"
#include "sfx.h"
#include "bitmaps.h"
#include "bmbutton.h"
#include "icons.h"
#include "SDL2MainMenu.h"

#include <SDL.h>
#include <SDL_ttf.h>

#undef min
#undef max
#include <algorithm>

// Colors matching the game's dark green-gray UI style
namespace TBColors {
    const SDL_Color Bg       = { 50,  55,  52,  255 };
    const SDL_Color BtnFace  = { 65,  75,  72,  255 };
    const SDL_Color BtnLight = { 95, 115, 108,  255 };
    const SDL_Color BtnDark  = { 35,  42,  45,  255 };
    const SDL_Color BtnPress = { 45,  55,  50,  255 };
    const SDL_Color Text     = { 210, 200, 190, 255 };
    const SDL_Color TextDim  = { 140, 130, 120, 255 };
    const SDL_Color TextWarn = { 255, 200, 80,  255 };
    const SDL_Color TextCrit = { 255, 80,  80,  255 };
    const SDL_Color BarBg    = { 30,  35,  32,  255 };
    const SDL_Color BarGreen = { 60, 140,  70,  255 };
    const SDL_Color BarRed   = { 160, 50,  50,  255 };
    const SDL_Color BarYel   = { 160, 140, 50,  255 };
    const SDL_Color Divider  = { 80,  90,  85,  255 };
}

static void FillR(SDL_Surface* s, SDL_Rect r, SDL_Color c) {
    SDL_FillRect(s, &r, SDL_MapRGB(s->format, c.r, c.g, c.b));
}

static const char* s_btnLabels[8] = {
    "Area", "World", "Chat", "Advisor",
    "Vehicles", "Buildings", "Science", "File"
};

SDL2Toolbar::SDL2Toolbar() {
    for (int i = 0; i < NUM_BUTTONS; i++)
        m_buttons[i].label = s_btnLabels[i];

    const char* fonts[] = {"C:\\Windows\\Fonts\\arial.ttf",
                           "C:\\Windows\\Fonts\\tahoma.ttf", nullptr};
    for (int i = 0; fonts[i]; i++) {
        FILE* f = fopen(fonts[i], "rb");
        if (f) { fclose(f); m_fontPath = fonts[i]; break; }
    }
}

SDL2Toolbar::~SDL2Toolbar() {
    for (auto& p : m_fontCache)
        if (p.second) TTF_CloseFont(p.second);
}

TTF_Font* SDL2Toolbar::GetFont(int size) {
    if (m_fontPath.empty()) return nullptr;
    auto it = m_fontCache.find(size);
    if (it != m_fontCache.end()) return it->second;
    TTF_Font* f = TTF_OpenFont(m_fontPath.c_str(), size);
    m_fontCache[size] = f;
    return f;
}

void SDL2Toolbar::Init(SDL2Panel* panel, GameWindow* gw) {
    m_panel = panel;
    m_gw = gw;

    // Load toolbar background tile from game data
    CDIB* pDibBg = theBitmaps.GetByIndex(DIB_TOOLBAR);
    if (pDibBg)
        m_bgTile = SDL2MainMenu::CreateSurfaceFromDIB(pDibBg);

    // Load master button sprite sheet
    static const int spriteIndices[NUM_BUTTONS] = {43, 17, 15, 31, 19, 18, 0, 27};
    m_btnSpriteW = theBmBtnData.Width();
    m_btnSpriteH = theBmBtnData.Height();

    if (theBmBtnData.m_pcDib)
        m_btnSheet = SDL2MainMenu::CreateSurfaceFromDIB(theBmBtnData.m_pcDib);

    for (int i = 0; i < NUM_BUTTONS; i++)
        m_buttons[i].spriteIndex = spriteIndices[i];

    // Load icon sprite sheets for status bars and text lines
    // Icons: 0=RESEARCH, 1=GAS, 2=POWER, 3=PEOPLE, 4=FOOD, 5=CLOCK, 6=BAR_TEXT
    for (int i = 0; i < 7; i++) {
        CStatData* pSd = theIcons.GetByIndex(i);
        if (!pSd) continue;
        m_iconData[i].cxIcon = pSd->m_cxIcon;
        m_iconData[i].cyIcon = pSd->m_cyIcon;
        m_iconData[i].cxLeft = pSd->m_cxLeft;
        m_iconData[i].cxBack = pSd->m_cxBack;
        m_iconData[i].cxRight = pSd->m_cxRight;
        m_iconData[i].cyBack = pSd->m_cyBack;
        m_iconData[i].leftOff = pSd->m_leftOff;
        m_iconData[i].rightOff = pSd->m_rightOff;
        m_iconData[i].typIcon = (int)pSd->m_iTypIcon;
        m_iconData[i].typBack = (int)pSd->m_iTypBack;
        m_iconData[i].nNeedIcon = pSd->m_nNeedIcon;
        if (pSd->m_pcDib)
            m_iconData[i].sheet = SDL2MainMenu::CreateSurfaceFromDIB(pSd->m_pcDib);
    }
}

void SDL2Toolbar::SetStatusText(int line, const std::string& text, int importance) {
    if (line < 0 || line > 1) return;
    // Persist the caller's value. For line 1 the hover poll in Render() may
    // override this while the cursor sits on the toolbar, but the override
    // is transient — the moment the cursor leaves the toolbar we fall back
    // to this value so the area-map hover info from CWndArea stays visible.
    m_externalText[line] = text;
    m_statusText[line]   = text;
    m_statusImportance[line] = importance;
}

void SDL2Toolbar::EnableButton(int index, bool enabled) {
    if (index >= 0 && index < NUM_BUTTONS)
        m_buttons[index].enabled = enabled;
}

void SDL2Toolbar::SetButtonHandler(int index, ButtonHandler handler) {
    if (index >= 0 && index < NUM_BUTTONS)
        m_buttons[index].handler = std::move(handler);
}

void SDL2Toolbar::SetButtonHelpText(int index, const std::string& text) {
    if (index >= 0 && index < NUM_BUTTONS)
        m_buttons[index].helpText = text;
}

void SDL2Toolbar::Render() {
    if (!m_panel) return;
    SDL_Surface* dst = m_panel->GetSurface();
    if (!dst) return;

    int w = m_panel->GetWidth();
    int h = m_panel->GetHeight();

    // Tile background from DIB_TOOLBAR
    if (m_bgTile) {
        for (int ty = 0; ty < h; ty += m_bgTile->h) {
            for (int tx = 0; tx < w; tx += m_bgTile->w) {
                SDL_Rect sr = {0, 0, m_bgTile->w, m_bgTile->h};
                SDL_Rect dr = {tx, ty, m_bgTile->w, m_bgTile->h};
                if (tx + sr.w > w) sr.w = w - tx;
                if (ty + sr.h > h) sr.h = h - ty;
                dr.w = sr.w; dr.h = sr.h;
                SDL_BlitSurface(m_bgTile, &sr, dst, &dr);
            }
        }
    } else {
        FillR(dst, {0, 0, w, h}, TBColors::Bg);
    }

    // Layout using actual button sprite sizes (matching CWndBar::OnCreate)
    int btnW = (m_btnSpriteW > 0) ? m_btnSpriteW : 50;
    int btnH = (m_btnSpriteH > 0) ? m_btnSpriteH : 30;
    int btnGap = 4, btnY = 4;

    // Buttons
    for (int i = 0; i < NUM_BUTTONS; i++) {
        int x = btnGap + i * (btnW + btnGap);
        RenderButton(dst, i, x, btnY, btnW, btnH);
    }
    int afterBtns = btnGap + NUM_BUTTONS * (btnW + btnGap) + btnGap;

    // Clock — fixed width on far right
    int clockW = 75;
    int clockX = w - clockW - btnGap;
    RenderClock(dst, clockX, 2, clockW, BTN_ROW_HT - 4);

    // Resource bars — fill space between buttons and clock, divided into 4
    int statSpace = clockX - afterBtns;
    int statW = statSpace / NUM_STATS;

    // Store bar positions for hit-testing
    m_statBarX = afterBtns;
    m_statBarW = statW;

    for (int i = 0; i < NUM_STATS; i++) {
        RenderStatBar(dst, i, afterBtns + i * statW, 2, statW - 4, BTN_ROW_HT - 4);
    }

    // Poll mouse position BEFORE drawing text so hover is immediate.
    // While the cursor is on the toolbar we override line 1 with the
    // toolbar's own hover string (resource bar / button). As soon as the
    // cursor leaves the toolbar we restore line 1 to whatever the game
    // last set via SetStatusText(1, ...) — that's the area-map hover info
    // (unit / terrain descriptions) sent from CWndArea::OnMouseMove.
    {
        // Use GetCursorPos (Windows API) instead of SDL_GetMouseState
        // because the MFC toolbar at alpha=1 intercepts mouse messages,
        // preventing SDL from updating its internal mouse state.
        POINT cursorPos;
        ::GetCursorPos(&cursorPos);
        int lx = cursorPos.x - m_panel->GetX();
        int ly = cursorPos.y - m_panel->GetY();

        // Default: mirror what the world last set
        m_statusText[1] = m_externalText[1];

        if (lx >= 0 && lx < w && ly >= 0 && ly < h) {
            // Cursor is on the toolbar — temporarily clear so the resource
            // bar / button hover blocks below can populate. If neither
            // matches, the empty line is appropriate (cursor over dead toolbar
            // space, not over the world).
            m_statusText[1].clear();
            if (ly < BTN_ROW_HT) {
                // Check resource bars
                if (m_statBarW > 0 && lx >= m_statBarX) {
                    int si = (lx - m_statBarX) / m_statBarW;
                    CPlayer* pMe2 = theGame.GetMe();
                    if (pMe2 && si >= 0 && si < NUM_STATS) {
                        char buf[96];
                        switch (si) {
                        case 0: sprintf_s(buf, "Gas: Have %d, Need %d", pMe2->GetGasHave(), pMe2->GetGasNeed()); break;
                        case 1: sprintf_s(buf, "Power: Have %d, Need %d", pMe2->GetPwrHave(), pMe2->GetPwrNeed()); break;
                        case 2: sprintf_s(buf, "People: %d, %d Working, Need %d", pMe2->GetPplTotal(), pMe2->GetPplBldg(), pMe2->GetPplNeedBldg()); break;
                        case 3: sprintf_s(buf, "Food: Have %d, Need %d", pMe2->GetFood(), pMe2->GetFoodNeed()); break;
                        }
                        m_statusText[1] = buf;
                    }
                }
                // Check button hover — prefer the long helpText (the MFC
                // sentence-length IDH_BAR_* string) and fall back to the
                // short label only if no help text was provided.
                if (m_statusText[1].empty()) {
                    int btnW2 = (m_btnSpriteW > 0) ? m_btnSpriteW : 50;
                    int btnGap2 = 4;
                    for (int i = 0; i < NUM_BUTTONS; i++) {
                        int bx = btnGap2 + i * (btnW2 + btnGap2);
                        if (lx >= bx && lx < bx + btnW2) {
                            m_statusText[1] = m_buttons[i].helpText.empty()
                                ? m_buttons[i].label
                                : m_buttons[i].helpText;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Status text lines (row 2)
    int textY = BTN_ROW_HT + 2;
    int halfW = w / 2;
    RenderTextLine(dst, 0, 4, textY, halfW - 8, TEXT_ROW_HT - 4);
    RenderTextLine(dst, 1, halfW + 4, textY, halfW - 8, TEXT_ROW_HT - 4);

    // Increment animation frame ~once per second (matching MFC IncIcon rate)
    // rather than every render frame, so resource-low icons don't flicker too fast.
    static DWORD s_lastAnimTick = 0;
    DWORD now = ::timeGetTime();
    if (now - s_lastAnimTick >= 1000) {
        s_lastAnimTick = now;
        m_animFrame++;
    }
    m_panel->SetDirty();
}

void SDL2Toolbar::RenderButton(SDL_Surface* dst, int idx, int x, int y, int w, int h) {
    ButtonState& btn = m_buttons[idx];

    if (m_btnSheet && m_btnSpriteW > 0 && m_btnSpriteH > 0) {
        // Use actual game button sprites from the master sheet
        // 3 columns: normal(x=0), pressed(x=W), disabled(x=2*W)
        int srcX = 0;
        if (!btn.enabled) srcX = m_btnSpriteW * 2;
        else if (btn.pressed) srcX = m_btnSpriteW;

        int srcY = m_btnSpriteH * btn.spriteIndex;

        SDL_Rect srcRect = {srcX, srcY, m_btnSpriteW, m_btnSpriteH};
        SDL_Rect dstRect = {x, y, m_btnSpriteW, m_btnSpriteH};
        SDL_BlitSurface(m_btnSheet, &srcRect, dst, &dstRect);
    } else {
        // Fallback: solid color button with text
        SDL_Color face = btn.pressed ? TBColors::BtnPress : TBColors::BtnFace;
        if (!btn.enabled) face = TBColors::BarBg;
        FillR(dst, {x, y, w, h}, face);

        SDL_Color lt = btn.pressed ? TBColors::BtnDark : TBColors::BtnLight;
        SDL_Color dk = btn.pressed ? TBColors::BtnLight : TBColors::BtnDark;
        FillR(dst, {x, y, w, 1}, lt);
        FillR(dst, {x, y, 1, h}, lt);
        FillR(dst, {x, y + h - 1, w, 1}, dk);
        FillR(dst, {x + w - 1, y, 1, h}, dk);

        TTF_Font* font = GetFont(10);
        if (font) {
            SDL_Color tc = btn.enabled ? TBColors::Text : TBColors::TextDim;
            SDL_Surface* ts = TTF_RenderText_Blended(font, btn.label.c_str(), tc);
            if (ts) {
                SDL_Rect dr = {x + (w - ts->w) / 2, y + (h - ts->h) / 2, ts->w, ts->h};
                SDL_BlitSurface(ts, nullptr, dst, &dr);
                SDL_FreeSurface(ts);
            }
        }
    }
}

void SDL2Toolbar::RenderStatBar(SDL_Surface* dst, int idx, int x, int y, int w, int h) {
    CPlayer* pMe = theGame.GetMe();
    if (!pMe) return;

    // Icon indices: gas=1, power=2, people=3, food=4
    int iconIdx = idx + 1;
    IconData& icon = m_iconData[iconIdx];

    int have = 0, need = 0;
    switch (idx) {
    case 0: have = pMe->GetGasHave(); need = pMe->GetGasNeed(); break;
    case 1: have = pMe->GetPwrHave(); need = pMe->GetPwrNeed(); break;
    case 2: have = pMe->GetPplBldg(); need = pMe->GetPplNeedBldg(); break;
    case 3: have = pMe->GetFood();    need = pMe->GetFoodNeed(); break;
    }

    // Render 3-piece background from icon sprite sheet
    if (icon.sheet && icon.cyBack > 0) {
        int bgSrcY = icon.cyIcon;  // Background pieces are below icons in sprite sheet

        // Left cap
        if (icon.cxLeft > 0) {
            SDL_Rect sr = {0, bgSrcY, icon.cxLeft, icon.cyBack};
            SDL_Rect dr = {x, y, icon.cxLeft, icon.cyBack};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }

        // Middle tile
        if (icon.cxBack > 0) {
            int midX = x + icon.cxLeft;
            int midW = w - icon.cxLeft - icon.cxRight;
            for (int tx = 0; tx < midW; tx += icon.cxBack) {
                int bw = std::min(icon.cxBack, midW - tx);
                SDL_Rect sr = {icon.cxLeft, bgSrcY, bw, icon.cyBack};
                SDL_Rect dr = {midX + tx, y, bw, icon.cyBack};
                SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
            }
        }

        // Right cap
        if (icon.cxRight > 0) {
            SDL_Rect sr = {icon.cxLeft + icon.cxBack, bgSrcY, icon.cxRight, icon.cyBack};
            SDL_Rect dr = {x + w - icon.cxRight, y, icon.cxRight, icon.cyBack};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
    } else {
        FillR(dst, {x, y, w, h}, TBColors::BarBg);
    }

    // Render repeating have/need icons (matching CStatInst::DrawStatHave)
    if (icon.sheet && icon.cxIcon > 0 && icon.cyIcon > 0 && (have > 0 || need > 0)) {
        int iconY = y + (h - icon.cyIcon) / 2;
        int barLeft = x + icon.leftOff;
        int barRight = x + w - icon.rightOff;
        int barW = barRight - barLeft;

        // Calculate the split point (iDone% of bar width)
        int iDone;
        int needSrcX;
        if (have < need && need > 0) {
            // Not enough — have icons fill (have/need)%, need icons fill the rest
            iDone = (have * 100) / need;
            // Animated "need" icon (flashing warning)
            int animFrame = (icon.nNeedIcon > 0) ? (m_animFrame % icon.nNeedIcon) : 0;
            needSrcX = icon.cxIcon * (2 + animFrame);
        } else {
            // Enough or excess — need icons fill (need/have)%, have icons fill the rest
            if (have <= 0 || need <= 0)
                iDone = 100;
            else {
                iDone = (need * 100) / have;
                if (iDone == 0 && need > 0) iDone = 1;
            }
            // "Excess" icon (second frame)
            needSrcX = icon.cxIcon;
        }

        int splitX = barLeft + (barW * iDone) / 100;
        int iStep = icon.cxIcon / 2;
        if (iStep < 1) iStep = 1;

        // Fill with "have" icons (green, sprite at x=0)
        for (int ix = barLeft; ix < splitX; ix += iStep) {
            if (ix + icon.cxIcon > barRight) break;
            SDL_Rect sr = {0, 0, icon.cxIcon, icon.cyIcon};
            SDL_Rect dr = {ix, iconY, icon.cxIcon, icon.cyIcon};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }

        // Fill remainder with "need" icons
        for (int ix = std::max(splitX, barLeft); ix < barRight; ix += iStep) {
            if (ix + icon.cxIcon > barRight) break;
            SDL_Rect sr = {needSrcX, 0, icon.cxIcon, icon.cyIcon};
            SDL_Rect dr = {ix, iconY, icon.cxIcon, icon.cyIcon};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
    }

}

void SDL2Toolbar::RenderClock(SDL_Surface* dst, int x, int y, int w, int h) {
    IconData& icon = m_iconData[ICON_CLOCK];

    // Render 3-piece background
    if (icon.sheet && icon.cyBack > 0) {
        int bgSrcY = icon.cyIcon;
        if (icon.cxLeft > 0) {
            SDL_Rect sr = {0, bgSrcY, icon.cxLeft, icon.cyBack};
            SDL_Rect dr = {x, y, icon.cxLeft, icon.cyBack};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
        if (icon.cxBack > 0) {
            int midX = x + icon.cxLeft;
            int midW = w - icon.cxLeft - icon.cxRight;
            for (int tx = 0; tx < midW; tx += icon.cxBack) {
                int bw = std::min(icon.cxBack, midW - tx);
                SDL_Rect sr = {icon.cxLeft, bgSrcY, bw, icon.cyBack};
                SDL_Rect dr = {midX + tx, y, bw, icon.cyBack};
                SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
            }
        }
        if (icon.cxRight > 0) {
            SDL_Rect sr = {icon.cxLeft + icon.cxBack, bgSrcY, icon.cxRight, icon.cyBack};
            SDL_Rect dr = {x + w - icon.cxRight, y, icon.cxRight, icon.cyBack};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
    } else {
        FillR(dst, {x, y, w, h}, TBColors::BarBg);
    }

    // Clock text
    int elapsed = theGame.GetElapsedSeconds();
    int hours = elapsed / 3600;
    int mins = (elapsed % 3600) / 60;
    int secs = elapsed % 60;
    char buf[16];
    sprintf_s(buf, "%d:%02d:%02d", hours, mins, secs);

    TTF_Font* clockFont = GetFont(11);
    if (clockFont) {
        SDL_Surface* ts = TTF_RenderText_Blended(clockFont, buf, TBColors::Text);
        if (ts) {
            SDL_Rect dr = {x + (w - ts->w) / 2, y + (h - ts->h) / 2, ts->w, ts->h};
            SDL_BlitSurface(ts, nullptr, dst, &dr);
            SDL_FreeSurface(ts);
        }
    }
}

void SDL2Toolbar::RenderTextLine(SDL_Surface* dst, int line, int x, int y, int w, int h) {
    if (line < 0 || line >= 2) return;

    IconData& icon = m_iconData[ICON_BAR_TEXT];

    // Render 3-piece background from ICON_BAR_TEXT sprite
    if (icon.sheet && icon.cyBack > 0) {
        int bgSrcY = icon.cyIcon;
        int bgH = icon.cyBack;
        int bgY = y;

        if (icon.cxLeft > 0) {
            SDL_Rect sr = {0, bgSrcY, icon.cxLeft, bgH};
            SDL_Rect dr = {x, bgY, icon.cxLeft, bgH};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
        if (icon.cxBack > 0) {
            int midX = x + icon.cxLeft;
            int midW = w - icon.cxLeft - icon.cxRight;
            for (int tx = 0; tx < midW; tx += icon.cxBack) {
                int bw = std::min(icon.cxBack, midW - tx);
                SDL_Rect sr = {icon.cxLeft, bgSrcY, bw, bgH};
                SDL_Rect dr = {midX + tx, bgY, bw, bgH};
                SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
            }
        }
        if (icon.cxRight > 0) {
            SDL_Rect sr = {icon.cxLeft + icon.cxBack, bgSrcY, icon.cxRight, bgH};
            SDL_Rect dr = {x + w - icon.cxRight, bgY, icon.cxRight, bgH};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
    }

    // Text
    const std::string& text = m_statusText[line];
    if (text.empty()) return;

    SDL_Color color = {255, 255, 240, 255};  // Bright white-yellow for readability
    switch (m_statusImportance[line]) {
    case 1: color = {255, 220, 80, 255}; break;   // Bright yellow warning
    case 2: color = {255, 80, 80, 255}; break;     // Bright red critical
    }

    TTF_Font* font = GetFont(15);  // Large font for readability
    if (font) {
        SDL_Surface* ts = TTF_RenderText_Blended(font, text.c_str(), color);
        if (ts) {
            int maxW = w - icon.leftOff - icon.rightOff;
            SDL_Rect sr = {0, 0, std::min(ts->w, maxW), std::min(ts->h, h)};
            SDL_Rect dr = {x + icon.leftOff, y + (h - sr.h) / 2, sr.w, sr.h};
            SDL_BlitSurface(ts, &sr, dst, &dr);
            SDL_FreeSurface(ts);
        }
    }
}

bool SDL2Toolbar::HandleEvent(SDL_Event& event, int localX, int localY) {
    int btnW = (m_btnSpriteW > 0) ? m_btnSpriteW : 50;
    int btnGap = 4;

    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT && localY < BTN_ROW_HT) {
            for (int i = 0; i < NUM_BUTTONS; i++) {
                int bx = 4 + i * (btnW + btnGap);
                if (localX >= bx && localX < bx + btnW && m_buttons[i].enabled) {
                    m_buttons[i].pressed = true;
                    m_pressedBtn = i;
                    return true;
                }
            }
        }
        return true;

    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT && m_pressedBtn >= 0) {
            int i = m_pressedBtn;
            m_buttons[i].pressed = false;
            m_pressedBtn = -1;
            // Check if still over the button
            int bx = 4 + i * (btnW + btnGap);
            if (localX >= bx && localX < bx + btnW && localY < BTN_ROW_HT) {
                if (m_buttons[i].handler)
                    m_buttons[i].handler();
            }
            return true;
        }
        return true;

    case SDL_MOUSEMOTION:
        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
        // Hover text is handled by the poll in Render() — no duplicate logic here
        return true;
    }
    return false;
}
