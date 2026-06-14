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
#include "area.h"          // theBuildingHex (transport destination lookup)

#include "building.inl"
#include "vehicle.inl"
#include "unit.inl"
#include "terrain.inl"  // CHexCoord/CMapLoc inlines (needed at /Ob2)

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

    const char* fonts[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "C:\\Windows\\Fonts\\arial.ttf",
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

    // Load icon sprite sheets for status bars and text lines.
    // Icons 0..14 (ICON_RESEARCH..ICON_VEHICLES). Indices 7+ (MATERIALS, DAMAGE,
    // CONSTRUCTION, BUILD_ROAD, VEHICLES) are used by RenderUnitStatus to draw a
    // hovered unit's resources as icon bars (matching _UnitShowStatus).
    for (int i = 0; i < 15; i++) {
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

    // Status-bar row height = ICON_DAMAGE background height (matches the
    // original CWndUnitStat m_iStatHt).
    if (m_iconData[ICON_DAMAGE].cyBack > 0)
        m_statBarHt = m_iconData[ICON_DAMAGE].cyBack;
}

void SDL2Toolbar::SetStatusText(int line, const std::string& text, int importance) {
    if (line < 0 || line > 1) return;
    m_statusUnit[line] = nullptr;  // setting text reverts the line from icons
    m_statusUnitId[line] = 0;
    // Persist the caller's value. For line 1 the hover poll in Render() may
    // override this while the cursor sits on the toolbar, but the override
    // is transient ??? the moment the cursor leaves the toolbar we fall back
    // to this value so the area-map hover info from CWndArea stays visible.
    m_externalText[line] = text;
    m_statusText[line]   = text;
    m_statusImportance[line] = importance;
}

void SDL2Toolbar::SetUnitStatus(int line, CUnit* pUnit) {
    if (line < 0 || line > 1) return;
    m_statusUnit[line] = pUnit;
    // capture the ID while the unit is alive — Render() re-validates with it
    m_statusUnitId[line] = pUnit ? pUnit->GetID() : 0;
    if (pUnit) {
        // Icon bars replace any text previously set on this line.
        m_externalText[line].clear();
        m_statusText[line].clear();
    }
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

    // Clock ??? fixed width on far right
    int clockW = 75;
    int clockX = w - clockW - btnGap;
    RenderClock(dst, clockX, 2, clockW, BTN_ROW_HT - 4);

    // Resource bars ??? fill space between buttons and clock, divided into 4
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
    // last set via SetStatusText(1, ...) ??? that's the area-map hover info
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
            // Cursor is on the toolbar ??? temporarily clear so the resource
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
                // Check button hover ??? prefer the long helpText (the MFC
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

    // Line 1: when the cursor hovers a unit on the map (and not over the toolbar
    // itself), draw the unit's status as ICON BARS (damage gradient, materials
    // icons, construction progress) like the MFC _UnitShowStatus. Otherwise fall
    // back to text (terrain hover, toolbar button/resource hover).
    bool cursorOnToolbar = false;
    {
        POINT cp; ::GetCursorPos(&cp);
        int lx2 = cp.x - m_panel->GetX();
        int ly2 = cp.y - m_panel->GetY();
        cursorOnToolbar = (lx2 >= 0 && lx2 < w && ly2 >= 0 && ly2 < h);
    }
    // Liveness check: m_statusUnit is a raw hover-time pointer and the unit
    // can die while the cursor sits still (combat). Re-resolve by ID and
    // require the SAME object back (guards against ID reuse) before deref.
    if (m_statusUnit[1] && ::_GetUnit(m_statusUnitId[1]) != m_statusUnit[1]) {
        m_statusUnit[1]   = nullptr;
        m_statusUnitId[1] = 0;
    }
    if (m_statusUnit[1] && !cursorOnToolbar)
        RenderUnitStatus(dst, m_statusUnit[1], halfW + 4, textY, halfW - 8, TEXT_ROW_HT - 4);
    else
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
            // Not enough ??? have icons fill (have/need)%, need icons fill the rest
            iDone = (have * 100) / need;
            // Animated "need" icon (flashing warning)
            int animFrame = (icon.nNeedIcon > 0) ? (m_animFrame % icon.nNeedIcon) : 0;
            needSrcX = icon.cxIcon * (2 + animFrame);
        } else {
            // Enough or excess ??? need icons fill (need/have)%, have icons fill the rest
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

        // ONE cursor advances across the whole bar by iStep; the "need" (red) run
        // CONTINUES from where the "have" (green) run left off, staying on the same
        // half-icon grid ??? exactly like the original CStatInst::DrawStatHave single
        // rIcon cursor. (Restarting the red run at splitX, an off-grid x, shifted
        // the red icons off the green grid and overlapped the seam icon, which read
        // as the reported misalignment + doubled icon.)
        int ix = barLeft;
        for (; ix < splitX; ix += iStep) {
            if (ix + icon.cxIcon > barRight) break;
            SDL_Rect sr = {0, 0, icon.cxIcon, icon.cyIcon};
            SDL_Rect dr = {ix, iconY, icon.cxIcon, icon.cyIcon};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
        for (; ix < barRight; ix += iStep) {
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
                    // Match the original CMyButton::OnLButtonDown click feedback.
                    theMusicPlayer.PlayForegroundSound(
                        SOUNDS::GetID(SOUNDS::button), SFXPRIORITY::selected_pri);
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
        // Hover text is handled by the poll in Render() ??? no duplicate logic here
        return true;
    }
    return false;
}


/////////////////////////////////////////////////////////////////////////////
// Icon-based unit status (mirrors CStatInst / _UnitShowStatus). Ported from
// SDL2UnitList so a hovered building's / vehicle's resources draw as icons in
// the status line instead of plain text.

// Software StretchBlt: scale srcSurf(srcRect) to dstSurf(dstRect)
static void TB_StretchBlit(SDL_Surface* src, SDL_Rect sr, SDL_Surface* dst, SDL_Rect dr) {
    if (!src || !dst || dr.w <= 0 || dr.h <= 0 || sr.w <= 0 || sr.h <= 0) return;
    SDL_BlitScaled(src, &sr, dst, &dr);
}

int SDL2Toolbar::GetNumStatusBars(CUnit* pUnit) {
    if (!pUnit) return 0;

    if (pUnit->GetUnitType() == CUnit::vehicle) {
        CVehicle* pVeh = (CVehicle*)pUnit;
        if (pVeh->GetData()->IsTransport())
            return 3;  // damage + route text + materials/cargo
        if (pVeh->GetData()->IsCrane() || pVeh->GetData()->IsCarrier())
            return 2;  // damage + construction/cargo
        return 1;      // damage only
    }

    if (pUnit->GetUnitType() == CUnit::building) {
        CBuilding* pBldg = (CBuilding*)pUnit;
        if (pBldg->IsConstructing())
            return 3;  // damage + materials + construction progress
        // Apartments/offices show damage + a population (occupancy) bar, matching
        // CHousingBuilding::GetNumStatusBars/PaintStatusBars in the original. This
        // takes priority over the materials bar.
        int bt = pBldg->GetData()->GetBldgType();
        if (bt == CStructureData::apartment || bt == CStructureData::office)
            return 2;  // damage + population
        // Completed vehicle plants (factory/barracks/shipyard) always show 3 bars:
        // damage + materials + the vehicle being built. CVehicleBuilding::
        // GetNumStatusBars returns 3 unconditionally. Gate on UNION TYPE ??? that's
        // what CBuilding::Create dispatches the class on (UTvehicle/UTshipyard ->
        // CVehicleBuilding/CShipyardBuilding), so the cast in RenderStatusBars is
        // guaranteed safe even if data gives another building a vehicle list.
        int ut = pBldg->GetData()->GetUnionType();
        int base;
        if (ut == CStructureData::UTvehicle || ut == CStructureData::UTshipyard)
            base = 3;
        else if (ut == CStructureData::UTfarm)
            // Mirror CFarmBuilding::GetNumStatusBars: a farm shows damage + fertility
            // (the green ICON_DENSITY "X" bar); a lumber mill adds a materials bar.
            base = (pBldg->GetData()->GetType() == CStructureData::farm) ? 2 : 3;
        else if (pBldg->GetTotalStore() > 0)
            base = 2;  // damage + materials
        else
            base = 1;  // damage only
        // One extra bar when units are parked inside (e.g. ships in a seaport),
        // showing their icons — see RenderContainedUnits.
        if (CountContainedUnits(pBldg) > 0)
            base++;
        return base;
    }

    return 1;
}

// Render the 3-piece background for a status bar icon
void SDL2Toolbar::Render3PieceBg(SDL_Surface* dst, int iconIdx, int x, int y, int w) {
    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || icon.cyBack <= 0) return;

    int bgSrcY = icon.cyIcon;  // Background row is below icon row in sprite sheet

    if (icon.typBack == 1) {  // back_3: left cap + tiled middle + right cap
        if (icon.cxLeft > 0) {
            SDL_Rect sr = {0, bgSrcY, icon.cxLeft, icon.cyBack};
            SDL_Rect dr = {x, y, icon.cxLeft, icon.cyBack};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
        if (icon.cxBack > 0) {
            int midX = x + icon.cxLeft;
            int midEnd = x + w - icon.cxRight;
            for (int tx = midX; tx < midEnd; tx += icon.cxBack) {
                int bw = std::min(icon.cxBack, midEnd - tx);
                SDL_Rect sr = {icon.cxLeft, bgSrcY, bw, icon.cyBack};
                SDL_Rect dr = {tx, y, bw, icon.cyBack};
                SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
            }
        }
        if (icon.cxRight > 0) {
            SDL_Rect sr = {icon.cxLeft + icon.cxBack, bgSrcY, icon.cxRight, icon.cyBack};
            SDL_Rect dr = {x + w - icon.cxRight, y, icon.cxRight, icon.cyBack};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
    } else if (icon.typBack == 0) {  // full_back: stretch entire background
        SDL_Rect sr = {0, bgSrcY, icon.cxBack, icon.cyBack};
        SDL_Rect dr = {x, y, w, icon.cyBack};
        TB_StretchBlit(icon.sheet, sr, dst, dr);
    } else {  // tile
        for (int tx = 0; tx < w; tx += icon.cxBack) {
            int bw = std::min(icon.cxBack, w - tx);
            SDL_Rect sr = {0, bgSrcY, bw, icon.cyBack};
            SDL_Rect dr = {x + tx, y, bw, icon.cyBack};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
    }
}

// DrawStatBar: continuous gradient progress bar (damage/health)
void SDL2Toolbar::RenderIconBar(SDL_Surface* dst, int iconIdx, int percent,
                                int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || percent <= 0) return;

    int barLeft = x + icon.leftOff;
    int barRight = x + w - icon.rightOff;
    int barW = barRight - barLeft;
    if (barW <= 0) return;

    int fillW = (barW * percent) / 100;
    if (fillW <= 0) return;

    int srcW = (icon.cxIcon * percent) / 100;
    if (srcW <= 0) srcW = 1;
    SDL_Rect sr = {0, 0, srcW, icon.cyIcon};
    SDL_Rect dr = {barLeft, y, fillW, h};
    TB_StretchBlit(icon.sheet, sr, dst, dr);
}

// DrawStatDone: tiles the stat icon sprite across `percent`% of the bar
void SDL2Toolbar::RenderIconDone(SDL_Surface* dst, int iconIdx, int percent,
                                 int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    if (percent <= 0) return;

    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || icon.cxIcon <= 0 || icon.cyIcon <= 0) return;

    int iconY  = y + (h - icon.cyIcon) / 2;
    int left   = x + icon.leftOff;
    int right  = x + w - icon.rightOff;
    int width  = right - left;
    if (width <= 0) return;

    int iEnd = right;
    if (percent < 100) iEnd -= icon.cxIcon / 2;
    int iRight = left + (width * percent) / 100;
    iRight = std::max(left + 1, iRight);
    int iStep = std::max(1, icon.cxIcon / 2);

    SDL_SetSurfaceBlendMode(icon.sheet, SDL_BLENDMODE_BLEND);
    for (int ix = left; ix < iRight; ix += iStep) {
        if (ix + icon.cxIcon > iEnd) break;
        SDL_Rect sr = {0, 0, icon.cxIcon, icon.cyIcon};
        SDL_Rect dr = {ix, iconY, icon.cxIcon, icon.cyIcon};
        SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
    }
}

// DrawStatText: text inside status bar background
void SDL2Toolbar::RenderIconText(SDL_Surface* dst, int iconIdx, const char* text,
                                 int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    IconData& icon = m_iconData[iconIdx];
    if (!text || !text[0]) return;

    // 16pt for the hovered unit's name / route text ??? slightly larger than the
    // main status line (15pt) so it reads clearly in the status bar.
    TTF_Font* font = GetFont(16);
    if (!font) return;

    int textX = x + icon.leftOff;
    int textW = w - icon.leftOff - icon.rightOff;
    if (textW <= 0) return;

    int th = 0;
    TTF_SizeText(font, text, nullptr, &th);
    int textY = y + (h - th) / 2;

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* ts = TTF_RenderText_Blended(font, text, white);
    if (ts) {
        SDL_Rect sr = {0, 0, std::min(ts->w, textW), ts->h};
        SDL_Rect dr = {textX, textY, sr.w, sr.h};
        SDL_BlitSurface(ts, &sr, dst, &dr);
        SDL_FreeSurface(ts);
    }
}

// Render carried vehicle sprites
void SDL2Toolbar::RenderCarrierCargo(SDL_Surface* dst, CVehicle* pVeh, int iconIdx,
                                     int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || icon.cxIcon <= 0 || icon.cyIcon <= 0) return;

    int iconY = y + (h - icon.cyIcon) / 2;
    int drawX = x + icon.leftOff;
    int rightLimit = x + w - icon.rightOff;

    // Use auto so `pos` adopts whatever type POSITION resolves to in vehicle.h
    // for this translation unit (avoids an x64 POSITION/CNode* mismatch).
    auto pos = pVeh->GetCargoHeadPosition();
    while (pos != NULL) {
        CVehicle* pCargo = pVeh->GetCargoNext(pos);
        if (drawX + icon.cxIcon > rightLimit) break;

        int srcX = pCargo->GetData()->GetType() * icon.cxIcon;
        if (srcX + icon.cxIcon <= icon.sheet->w) {
            SDL_Rect sr = {srcX, 0, icon.cxIcon, icon.cyIcon};
            SDL_Rect dr = {drawX, iconY, icon.cxIcon, icon.cyIcon};
            SDL_SetSurfaceBlendMode(icon.sheet, SDL_BLENDMODE_BLEND);
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
        drawX += icon.cxIcon;
    }
}

// Count the friendly vehicles currently parked inside a building. A vehicle is
// "inside" when it owns no hex (it's tucked in the building, not on the map) and
// the building under its head hex is this one. Mirrors the same test the unit-info
// popup uses (SDL2UnitInfoPanel::BuildContent). 0 for non-buildings.
int SDL2Toolbar::CountContainedUnits(CUnit* pUnit) {
    if (!pUnit || pUnit->GetUnitType() != CUnit::building) return 0;
    int n = 0;
    auto pos = theVehicleMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID; CVehicle* pVeh;
        theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
        if (pVeh->GetOwner()->IsMe() && !pVeh->GetHexOwnership() &&
            theBuildingHex._GetBuilding(pVeh->GetPtHead()) == pUnit)
            n++;
    }
    return n;
}

// Draw the icons of the vehicles parked inside a building, left to right, the same
// way RenderCarrierCargo draws a carrier's cargo (ICON_VEHICLES sheet, one tile per
// vehicle type at srcX = GetType()*cxIcon). This is what reveals, on hover, what's
// sitting in a seaport / factory.
void SDL2Toolbar::RenderContainedUnits(SDL_Surface* dst, CBuilding* pBldg, int iconIdx,
                                       int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || icon.cxIcon <= 0 || icon.cyIcon <= 0) return;

    int iconY = y + (h - icon.cyIcon) / 2;
    int drawX = x + icon.leftOff;
    int rightLimit = x + w - icon.rightOff;

    auto pos = theVehicleMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID; CVehicle* pVeh;
        theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
        if (!pVeh->GetOwner()->IsMe() || pVeh->GetHexOwnership()) continue;
        if (theBuildingHex._GetBuilding(pVeh->GetPtHead()) != pBldg) continue;
        if (drawX + icon.cxIcon > rightLimit) break;

        int srcX = pVeh->GetData()->GetType() * icon.cxIcon;
        if (srcX + icon.cxIcon <= icon.sheet->w) {
            SDL_Rect sr = {srcX, 0, icon.cxIcon, icon.cyIcon};
            SDL_Rect dr = {drawX, iconY, icon.cxIcon, icon.cyIcon};
            SDL_SetSurfaceBlendMode(icon.sheet, SDL_BLENDMODE_BLEND);
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
        drawX += icon.cxIcon;
    }
}

// Render materials using ICON_MATERIALS sprites
void SDL2Toolbar::RenderMaterialsBar(SDL_Surface* dst, CUnit* pUnit, int iconIdx,
                                     int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || icon.cxIcon <= 0 || icon.cyIcon <= 0) return;

    int total = pUnit->GetTotalStore();
    if (total <= 0) return;

    int maxStore = total;
    if (pUnit->GetUnitType() == CUnit::vehicle) {
        int maxMat = ((CVehicle*)pUnit)->GetMaxMaterials();
        if (maxMat > total) maxStore = maxMat;
    } else {
        int iStep = std::max(1, icon.cxIcon / 2);
        int barWi = w - icon.leftOff - icon.rightOff;
        int numIcons = barWi / iStep;
        maxStore = std::max(total, 500 * numIcons);
    }

    int iconY = y + (h - icon.cyIcon) / 2;
    int barLeft = x + icon.leftOff;
    int barRight = x + w - icon.rightOff;
    int barW = barRight - barLeft;
    if (barW <= 0) return;

    // Faithful port of CUnit::PaintStatusMaterials: a single cursor advances by
    // cxIcon/2 across ALL material types (no per-type reset), and each type's run
    // reserves room for one icon of every remaining type. Carrying the cursor
    // continuously is what prevents the last icon of one type overlapping the
    // first of the next (the bug from the old per-segment drawX=segEnd reset).
    int iIconAdd = std::max(1, icon.cxIcon / 2);
    int iLen   = barW - icon.cxIcon;     // drawable run length (minus one icon)
    int iRight = barRight - icon.cxIcon; // rightmost legal icon left-edge

    int iNumTypes = 0;
    for (int i = 0; i < CMaterialTypes::GetNumTypes(); i++)
        if (pUnit->GetStore(i) > 0) iNumTypes++;

    int iTotal = maxStore;
    int cursor = barLeft;

    for (int i = 0; i < CMaterialTypes::GetNumTypes(); i++) {
        int stored = pUnit->GetStore(i);
        if (stored <= 0) continue;

        int iWid = (iTotal > 0) ? (iLen * stored) / iTotal : iLen;
        iWid = std::max(1, iWid);
        iNumTypes--;
        iWid = std::min(iWid, iLen - iNumTypes * iIconAdd);  // room for 1 of each remaining
        iTotal -= stored;
        iWid += cursor;                                       // absolute stop x

        // Each material type draws from its own sprite column (srcX = i*cxIcon),
        // matching the original's `CPoint pt(iOn*cxIcon, 0)`.
        int srcX = i * icon.cxIcon;
        if (srcX + icon.cxIcon > icon.sheet->w)
            srcX = 0;  // out-of-range type: fall back to first icon

        SDL_SetSurfaceBlendMode(icon.sheet, SDL_BLENDMODE_BLEND);
        for (; cursor < iWid; cursor += iIconAdd) {
            if (cursor > iRight) break;
            SDL_Rect sr = {srcX, 0, icon.cxIcon, icon.cyIcon};
            SDL_Rect dr = {cursor, iconY, icon.cxIcon, icon.cyIcon};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
            iLen -= iIconAdd;
        }
    }
}

void SDL2Toolbar::RenderStatusBars(SDL_Surface* dst, CUnit* pUnit, int x, int y, int w, int numBars) {
    if (!pUnit || numBars <= 0 || w <= 0) return;

    int usedW = 0;
    int barH = (m_statBarHt > 0) ? m_statBarHt
             : (m_iconData[ICON_DAMAGE].cyBack > 0 ? m_iconData[ICON_DAMAGE].cyBack : 16);

    // When a building has units parked inside, GetNumStatusBars added one trailing
    // bar for them — this is its slot index (last bar). -1 when there's none.
    int containedSlot = -1;
    if (pUnit->GetUnitType() == CUnit::building && CountContainedUnits(pUnit) > 0)
        containedSlot = numBars - 1;

    for (int iOn = 0; iOn < numBars; iOn++) {
        int barW = (w - usedW) / (numBars - iOn);
        int barX = x + usedW;
        usedW += barW;

        int renderX = barX + 1;
        int renderW = barW - 1;

        if (pUnit->GetUnitType() == CUnit::vehicle) {
            CVehicle* pVeh = (CVehicle*)pUnit;

            if (iOn == 0) {
                int dmg = std::max(1, pVeh->GetDamagePer());
                RenderIconBar(dst, ICON_DAMAGE, dmg, renderX, y, renderW, barH);
            } else if (pVeh->GetData()->IsCarrier() &&
                       ((pVeh->GetData()->IsTransport() && iOn == 2) ||
                        (!pVeh->GetData()->IsTransport() && iOn == 1))) {
                RenderCarrierCargo(dst, pVeh, ICON_VEHICLES, renderX, y, renderW, barH);
            } else if (pVeh->GetData()->IsTransport() && iOn == 1) {
                std::string routeText;
                if (!pVeh->IsHpControl())
                    routeText = CTransportData::m_sAuto;
                else if (pVeh->GetEvent() == CVehicle::route)
                    routeText = CTransportData::m_sRoute;

                CBuilding* pBldg = theBuildingHex.GetBuilding(pVeh->GetPtHead());
                if (pBldg == NULL || pVeh->GetHexOwnership())
                    pBldg = theBuildingHex.GetBuilding(pVeh->GetHexDest());
                if (pBldg != NULL && pBldg->GetOwner()->IsMe())
                    routeText += pBldg->GetData()->GetDesc().c_str();
                else if (pVeh->GetRouteMode() == CVehicle::stop)
                    routeText += CTransportData::m_sIdle;
                else
                    routeText += CTransportData::m_sTravel;

                RenderIconText(dst, ICON_BAR_TEXT, routeText.c_str(), renderX, y, renderW, barH);
            } else if (pVeh->GetData()->IsCrane() && iOn == 1) {
                int per = 0;
                int craneIcon = ICON_CONSTRUCTION;
                if (pVeh->GetRouteMode() == CVehicle::run) {
                    if (pVeh->GetEvent() == CVehicle::build ||
                        pVeh->GetEvent() == CVehicle::repair_bldg) {
                        CBuilding* pConst = pVeh->GetConst();
                        if (pConst)
                            per = std::max(1, pConst->GetBuildPer());
                    } else if (pVeh->GetEvent() == CVehicle::build_road) {
                        craneIcon = ICON_BUILD_ROAD;
                        per = std::max(1, pVeh->GetRoadPer());
                    }
                }
                RenderIconDone(dst, craneIcon, per, renderX, y, renderW, barH);
            } else {
                RenderMaterialsBar(dst, pVeh, ICON_MATERIALS, renderX, y, renderW, barH);
            }
        } else if (pUnit->GetUnitType() == CUnit::building) {
            CBuilding* pBldg = (CBuilding*)pUnit;
            int bt = pBldg->GetData()->GetBldgType();
            bool isHousing = !pBldg->IsConstructing() &&
                             (bt == CStructureData::apartment || bt == CStructureData::office);

            if (iOn == containedSlot) {
                // Trailing bar: the units currently parked inside this building.
                RenderContainedUnits(dst, pBldg, ICON_VEHICLES, renderX, y, renderW, barH);
            } else if (iOn == 0) {
                int dmg = std::max(1, pBldg->GetDamagePer());
                RenderIconBar(dst, ICON_DAMAGE, dmg, renderX, y, renderW, barH);
            } else if (isHousing && iOn == 1) {
                // Population/occupancy bar (mirrors CHousingBuilding::PaintStatusBars):
                // apartments show the player's total population vs apartment capacity,
                // offices show working population vs office capacity. Drawn with the
                // "done" fill style (RenderIconDone), like the original.
                CPlayer* pMe = theGame.GetMe();
                int per = 0;
                if (pMe) {
                    if (bt == CStructureData::apartment) {
                        if (pMe->m_iAptCap > 0)
                            per = (int)((pMe->GetPplTotal() * 100) / pMe->m_iAptCap);
                    } else {
                        if (pMe->m_iOfcCap > 0)
                            per = (int)((pMe->GetPplBldg() * 100) / pMe->m_iOfcCap);
                    }
                }
                RenderIconDone(dst, ICON_PEOPLE, std::min(100, std::max(0, per)),
                               renderX, y, renderW, barH);
            } else if (iOn == 1) {
                // A farm's second bar is fertility (green ICON_DENSITY "X"s), not
                // materials — mirrors CFarmBuilding::PaintStatusBars. Lumber mills
                // keep the materials bar here (their fertility is the 3rd bar).
                if (pBldg->GetData()->GetUnionType() == CStructureData::UTfarm &&
                    pBldg->GetData()->GetType() == CStructureData::farm) {
                    int per = std::min(100, std::max(0, ((CFarmBuilding*)pBldg)->GetTerMult() * 10));
                    RenderIconDone(dst, ICON_DENSITY, per, renderX, y, renderW, barH);
                } else {
                    RenderMaterialsBar(dst, pBldg, ICON_MATERIALS, renderX, y, renderW, barH);
                }
            } else if (iOn == 2) {
                int ut = pBldg->GetData()->GetUnionType();
                if (pBldg->IsConstructing()) {
                    int per = std::max(1, pBldg->GetBuildPer());
                    RenderIconDone(dst, ICON_CONSTRUCTION, per, renderX, y, renderW, barH);
                } else if (ut == CStructureData::UTfarm) {
                    // Lumber mill's productivity bar (forest density), same green art.
                    int per = std::min(100, std::max(0, ((CFarmBuilding*)pBldg)->GetTerMult() * 10));
                    RenderIconDone(dst, ICON_DENSITY, per, renderX, y, renderW, barH);
                } else if (ut == CStructureData::UTvehicle || ut == CStructureData::UTshipyard) {
                    // Completed vehicle plant: show the vehicle being built and its
                    // progress ??? mirrors CVehicleBuilding::PaintStatusBars (the
                    // ICON_BUILD_VEH strip with the vehicle's name centered over it).
                    // Gate on UNION TYPE, not GetBldVehicle(): the union type is what
                    // CBuilding::Create dispatches the class on (UTvehicle/UTshipyard
                    // -> CVehicleBuilding/CShipyardBuilding), so this cast is safe
                    // even if data gives another building type a vehicle list.
                    CVehicleBuilding* pVb = (CVehicleBuilding*)pBldg;
                    CBuildUnit const* pBu = pVb->GetBldUnt();
                    if (pBu == NULL) {
                        // idle plant: empty strip background (SetPer(0) + DrawIcon)
                        RenderIconDone(dst, ICON_BUILD_VEH, 0, renderX, y, renderW, barH);
                    } else {
                        RenderIconDone(dst, ICON_BUILD_VEH, std::max(1, pVb->GetBuildPer()),
                                       renderX, y, renderW, barH);
                        // Vehicle name on top (original drew CLR_CONST white text).
                        // Validate the type FIRST: the sim thread can finish/cancel the
                        // build between GetBldUnt() and here, leaving GetVehType() == -1
                        // ??? the unguarded GetData(-1)->GetDesc() was a crash (AV reading
                        // a garbage string in RenderStatusBars).
                        int vehType = pBu->GetVehType();
                        std::string name;
                        if (vehType >= 0 && vehType < theTransports.GetNumTransports())
                            name = theTransports.GetData(vehType)->GetDesc();
                        TTF_Font* font = GetFont(14);
                        if (font && !name.empty()) {
                            SDL_Color white = {255, 255, 255, 255};
                            SDL_Surface* ts = TTF_RenderText_Blended(font, name.c_str(), white);
                            if (ts) {
                                // Clip to the bar width (long names would otherwise
                                // bleed into the neighbouring bar / off the toolbar).
                                SDL_Rect sr = { 0, 0, std::min(ts->w, renderW), ts->h };
                                SDL_Rect dr = { renderX + std::max(0, (renderW - ts->w) / 2),
                                                y + (barH - ts->h) / 2, sr.w, sr.h };
                                SDL_BlitSurface(ts, &sr, dst, &dr);
                                SDL_FreeSurface(ts);
                            }
                        }
                    }
                }
            }
        }
    }
}

void SDL2Toolbar::RenderUnitStatus(SDL_Surface* dst, CUnit* pUnit, int x, int y, int w, int h) {
    if (!pUnit || w <= 0) return;

    int barH = (m_statBarHt > 0 && m_statBarHt <= h) ? m_statBarHt : h;
    int barY = y + (h - barH) / 2;

    // Description (unit name) in an ICON_BAR_TEXT bar ??? left portion, capped at
    // ~14 chars, matching the original _UnitShowStatus.
    IconData& bt = m_iconData[ICON_BAR_TEXT];
    int descMax = 14 * 8 + bt.leftOff + bt.rightOff;
    int descW = std::min(w / 2, descMax);
    if (descW < 1) descW = w / 2;
    // Pass the full row height (y, h) so the 14pt name centers in the whole row
    // rather than the shorter stat-bar height ??? the background sprite draws at
    // its own native height either way.
    RenderIconText(dst, ICON_BAR_TEXT, pUnit->GetData()->GetDesc().c_str(), x, y, descW, h);

    int sx = x + descW + 1;
    int sw = w - descW - 1;
    if (sw <= 0) return;

    // Non-owned unit: owner name + damage only (matches _UnitShowStatus).
    if (!pUnit->GetOwner()->IsMe()) {
        int nameW = sw / 2;
        RenderIconText(dst, ICON_BAR_TEXT, (const char*)pUnit->GetOwner()->GetName(),
                       sx, barY, nameW, barH);
        int dmg = std::max(1, pUnit->GetLastShowDamagePer());
        RenderIconBar(dst, ICON_DAMAGE, dmg, sx + nameW + 1, barY, sw - nameW - 1, barH);
        return;
    }

    RenderStatusBars(dst, pUnit, sx, barY, sw, GetNumStatusBars(pUnit));
}
