#include "stdafx.h"

#include "SDL2UnitList.h"
#include "SDL2Panel.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "area.h"
#include "player.h"
#include "bitmaps.h"
#include "icons.h"

#include "building.inl"
#include "vehicle.inl"
#include "unit.inl"
#include "terrain.inl"  // CHexCoord/CMapLoc inlines (needed at /Ob2)

#include <SDL.h>
#include <SDL_ttf.h>

#undef min
#undef max
#include <algorithm>

namespace ULColors {
    const SDL_Color Bg        = {  40,  45,  42, 255 };
    const SDL_Color Text      = { 255, 255, 255, 255 };
    const SDL_Color TextShadow= {   0,   0,   0, 255 };
    const SDL_Color TextDim   = { 180, 175, 165, 255 };
    const SDL_Color BarBg     = {  25,  30,  27, 255 };
    const SDL_Color ScrollBar = { 100, 110, 105, 255 };
}

static void FillU(SDL_Surface* s, SDL_Rect r, SDL_Color c) {
    SDL_FillRect(s, &r, SDL_MapRGB(s->format, c.r, c.g, c.b));
}

// Software StretchBlt: scale srcSurf(srcRect) to dstSurf(dstRect)
static void StretchBlit(SDL_Surface* src, SDL_Rect sr, SDL_Surface* dst, SDL_Rect dr) {
    if (!src || !dst || dr.w <= 0 || dr.h <= 0 || sr.w <= 0 || sr.h <= 0) return;
    SDL_BlitScaled(src, &sr, dst, &dr);
}

SDL2UnitList::SDL2UnitList(ListType type)
    : m_type(type)
{
    const char* fonts[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "C:\\Windows\\Fonts\\arial.ttf", nullptr};
    for (int i = 0; fonts[i]; i++) {
        FILE* f = fopen(fonts[i], "rb");
        if (f) { fclose(f); m_fontPath = fonts[i]; break; }
    }
}

SDL2UnitList::~SDL2UnitList() {
    if (m_unitSprites) SDL_FreeSurface(m_unitSprites);
    if (m_bgNormal) SDL_FreeSurface(m_bgNormal);
    if (m_bgSelected) SDL_FreeSurface(m_bgSelected);
    for (auto& id : m_iconData)
        if (id.sheet) SDL_FreeSurface(id.sheet);
    for (auto& p : m_fontCache)
        if (p.second) TTF_CloseFont(p.second);
}

TTF_Font* SDL2UnitList::GetFont(int size) {
    if (m_fontPath.empty()) return nullptr;
    auto it = m_fontCache.find(size);
    if (it != m_fontCache.end()) return it->second;
    TTF_Font* f = TTF_OpenFont(m_fontPath.c_str(), size);
    m_fontCache[size] = f;
    return f;
}

void SDL2UnitList::Init(SDL2Panel* panel, GameWindow* gw) {
    m_panel = panel;
    m_gw = gw;

    // Load unit type sprites (vehicle or building icon sheet)
    int dibIdx = (m_type == VEHICLES) ? DIB_LIST_UNIT_VEHICLES : DIB_LIST_UNIT_BUILDINGS;
    CDIB* pSprites = theBitmaps.GetByIndex(dibIdx);
    if (pSprites)
        m_unitSprites = SDL2MainMenu::CreateSurfaceFromDIB(pSprites);

    // Load background texture ??? top half is normal, bottom half is selected
    // Original uses StretchBlt to stretch this to fit each item
    CDIB* pBg = theBitmaps.GetByIndex(DIB_LIST_UNIT_BACK);
    if (pBg) {
        SDL_Surface* full = SDL2MainMenu::CreateSurfaceFromDIB(pBg);
        if (full) {
            int halfH = full->h / 2;
            m_bgNormal = SDL_CreateRGBSurface(0, full->w, halfH,
                full->format->BitsPerPixel, full->format->Rmask,
                full->format->Gmask, full->format->Bmask, full->format->Amask);
            m_bgSelected = SDL_CreateRGBSurface(0, full->w, halfH,
                full->format->BitsPerPixel, full->format->Rmask,
                full->format->Gmask, full->format->Bmask, full->format->Amask);
            if (m_bgNormal) {
                SDL_Rect sr = {0, 0, full->w, halfH};
                SDL_BlitSurface(full, &sr, m_bgNormal, nullptr);
            }
            if (m_bgSelected) {
                SDL_Rect sr = {0, halfH, full->w, halfH};
                SDL_Rect dr = {0, 0, full->w, halfH};
                SDL_BlitSurface(full, &sr, m_bgSelected, &dr);
            }
            SDL_FreeSurface(full);
        }
    }

    // Load icon sprite data for status bars (same pattern as SDL2Toolbar)
    int iconIndices[] = { ICON_BAR_TEXT, ICON_MATERIALS, ICON_DAMAGE,
                          ICON_CONSTRUCTION, ICON_BUILD_VEH, ICON_REPAIR_VEH,
                          ICON_BUILD_ROAD, ICON_VEHICLES };
    for (int idx : iconIndices) {
        if (idx < 0 || idx >= 15) continue;
        CStatData* pSd = theIcons.GetByIndex(idx);
        if (!pSd) continue;
        m_iconData[idx].cxIcon = pSd->m_cxIcon;
        m_iconData[idx].cyIcon = pSd->m_cyIcon;
        m_iconData[idx].cxLeft = pSd->m_cxLeft;
        m_iconData[idx].cxBack = pSd->m_cxBack;
        m_iconData[idx].cxRight = pSd->m_cxRight;
        m_iconData[idx].cyBack = pSd->m_cyBack;
        m_iconData[idx].leftOff = pSd->m_leftOff;
        m_iconData[idx].rightOff = pSd->m_rightOff;
        m_iconData[idx].typIcon = (int)pSd->m_iTypIcon;
        m_iconData[idx].typBack = (int)pSd->m_iTypBack;
        m_iconData[idx].nNeedIcon = pSd->m_nNeedIcon;
        if (pSd->m_pcDib)
            m_iconData[idx].sheet = SDL2MainMenu::CreateSurfaceFromDIB(pSd->m_pcDib);
    }

    // Use ICON_DAMAGE height as the status bar row height (matches original m_iStatHt)
    if (m_iconData[ICON_DAMAGE].cyBack > 0)
        m_statBarHt = m_iconData[ICON_DAMAGE].cyBack;

    Rebuild();
}

void SDL2UnitList::Rebuild() {
    m_items.clear();

    if (m_type == VEHICLES) {
        POSITION pos = theVehicleMap.GetStartPosition();
        while (pos != NULL) {
            DWORD dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
            if (pVeh->GetOwner()->IsMe())
                AddUnit(pVeh);
        }
    } else {
        POSITION pos = theBuildingMap.GetStartPosition();
        while (pos != NULL) {
            DWORD dwID;
            CBuilding* pBldg;
            theBuildingMap.GetNextAssoc(pos, dwID, pBldg);
            if (pBldg->GetOwner()->IsMe())
                AddUnit(pBldg);
        }
    }

    // Sort by type then name (matches original CWndListUnits::OnCompareItem)
    std::sort(m_items.begin(), m_items.end(), [](const ListItem& a, const ListItem& b) {
        if (a.typeIndex != b.typeIndex) return a.typeIndex < b.typeIndex;
        return a.name < b.name;
    });
}

void SDL2UnitList::AddUnit(CUnit* pUnit) {
    ListItem item;
    item.pUnit = pUnit;
    item.dwID  = pUnit->GetID();
    item.name = pUnit->GetData()->GetDesc().c_str();

    if (m_type == VEHICLES) {
        CVehicle* pVeh = (CVehicle*)pUnit;
        item.typeIndex = pVeh->GetData()->GetType();
    } else {
        CBuilding* pBldg = (CBuilding*)pUnit;
        item.typeIndex = pBldg->GetData()->GetType();
    }

    m_items.push_back(item);
}

void SDL2UnitList::RemoveUnit(CUnit* pUnit) {
    m_items.erase(
        std::remove_if(m_items.begin(), m_items.end(),
            [pUnit](const ListItem& it) { return it.pUnit == pUnit; }),
        m_items.end());
}

void SDL2UnitList::Render() {
    if (!m_panel) return;

    // Throttle: only actually redraw on a fixed interval (or when forced by an
    // interaction). On skipped frames we leave the panel's surface and dirty flag
    // untouched, so the compositor's present-gate skips re-presenting our detached
    // window ??? that's what keeps an open list from dragging the game's framerate.
    DWORD nowTick = ::timeGetTime();
    if (!m_forceDraw && (nowTick - m_lastDrawMs) < DRAW_INTERVAL_MS)
        return;
    m_lastDrawMs = nowTick;
    m_forceDraw = false;

    SDL_Surface* dst = m_panel->GetSurface();
    if (!dst) return;

    int w = m_panel->GetWidth();
    int h = m_panel->GetHeight();

    // Background
    FillU(dst, {0, 0, w, h}, ULColors::Bg);

    // Rebuild list periodically to catch new units
    static DWORD s_lastRebuild = 0;
    DWORD now = ::timeGetTime();
    if (now - s_lastRebuild > 2000) {
        Rebuild();
        s_lastRebuild = now;
    }

    // Content width excludes scrollbar (always reserved)
    int contentW = w - SB_WIDTH;

    // Highlight whichever item matches the area map's current selection, rather
    // than a stored index. This keeps the list in sync with the map: selecting a
    // unit on the map highlights it here, and ??? importantly ??? when the map clears
    // its selection (e.g. after a crane is given a build order, area.cpp build_loc
    // does RemoveAllUnits/SelectOff) the highlight clears here too instead of
    // lingering.
    CUnit* pSelUnit = nullptr;
    if (CWndArea* pTop = theAreaList.GetTop())
        pSelUnit = pTop->GetUnit();

    // Render visible items (within content area, not overlapping scrollbar)
    int y = -m_scrollY;
    for (int i = 0; i < (int)m_items.size(); i++) {
        if (y + ITEM_HT > 0 && y < h) {
            bool selected = (pSelUnit != nullptr && m_items[i].pUnit == pSelUnit);
            RenderItem(dst, i, 0, y, contentW, selected);
        }
        y += ITEM_HT;
    }

    // Scrollbar (always visible)
    int totalH = (int)m_items.size() * ITEM_HT;
    // Track background
    FillU(dst, {contentW, 0, SB_WIDTH, h}, {30, 35, 32, 255});
    if (totalH > h) {
        // Thumb
        int sbH = std::max(24, (h * h) / totalH);
        int sbY = (m_scrollY * (h - sbH)) / (totalH - h);
        FillU(dst, {contentW, sbY, SB_WIDTH, sbH}, ULColors::ScrollBar);
        // Thumb border highlight
        FillU(dst, {contentW, sbY, SB_WIDTH, 1}, {130, 140, 135, 255});
        FillU(dst, {contentW, sbY + sbH - 1, SB_WIDTH, 1}, {60, 65, 62, 255});
    } else {
        // Content fits ??? show full-height disabled thumb
        FillU(dst, {contentW, 0, SB_WIDTH, h}, {50, 55, 52, 255});
    }

    m_panel->SetDirty();
}

// Render shadow text (black at +1,+1, white on top ??? matching original)
void SDL2UnitList::RenderShadowText(SDL_Surface* dst, TTF_Font* font, const char* text,
                                     int x, int y, int maxW, SDL_Color fg, SDL_Color shadow) {
    if (!font || !text || !text[0]) return;

    // Truncate text with "..." if too long (matching original)
    std::string sText = text;
    int tw = 0, th = 0;
    TTF_SizeText(font, sText.c_str(), &tw, &th);
    if (tw > maxW && sText.length() > 4) {
        while (tw > maxW && sText.length() > 4) {
            sText.resize(sText.length() - 1);
            TTF_SizeText(font, (sText + "...").c_str(), &tw, &th);
        }
        sText += "...";
    }

    // Shadow (offset +1,+1)
    SDL_Surface* ts = TTF_RenderText_Blended(font, sText.c_str(), shadow);
    if (ts) {
        SDL_Rect sr = {0, 0, std::min(ts->w, maxW), ts->h};
        SDL_Rect dr = {x + 1, y + 1, sr.w, sr.h};
        SDL_BlitSurface(ts, &sr, dst, &dr);
        SDL_FreeSurface(ts);
    }
    // Foreground
    ts = TTF_RenderText_Blended(font, sText.c_str(), fg);
    if (ts) {
        SDL_Rect sr = {0, 0, std::min(ts->w, maxW), ts->h};
        SDL_Rect dr = {x, y, sr.w, sr.h};
        SDL_BlitSurface(ts, &sr, dst, &dr);
        SDL_FreeSurface(ts);
    }
}

void SDL2UnitList::RenderItem(SDL_Surface* dst, int idx, int x, int y, int w, bool selected) {
    ListItem& item = m_items[idx];

    // The list is only rebuilt every ~2s (see Render), so a unit can die in
    // combat and leave this row pointing at freed memory until the next rebuild.
    // The original kept rows in sync by removing them on death; here we instead
    // re-resolve the cached pointer against the live map by ID every draw and
    // skip the row if the unit is gone. (Refresh item.pUnit so the rest of this
    // function ??? GetData(), status bars, etc. ??? uses the live object.)
    CUnit* pLive = (m_type == VEHICLES)
                       ? (CUnit*)theVehicleMap.GetVehicle( item.dwID )
                       : (CUnit*)theBuildingMap.GetBldg( item.dwID );
    if ( pLive == NULL )
        return;            // unit died since the last Rebuild() ??? don't draw a stale row
    item.pUnit = pLive;

    // --- Background: STRETCH to fill entire item (matching original StretchBlt) ---
    SDL_Surface* bg = selected ? m_bgSelected : m_bgNormal;
    if (bg) {
        SDL_Rect sr = {0, 0, bg->w, bg->h};
        SDL_Rect dr = {x, y, w, ITEM_HT};
        SDL_BlitScaled(bg, &sr, dst, &dr);
    } else {
        SDL_Color bgColor = selected ? SDL_Color{70, 85, 95, 255} : SDL_Color{50, 55, 52, 255};
        FillU(dst, {x, y, w, ITEM_HT}, bgColor);
    }

    // --- Unit icon from sprite sheet ---
    // Original: CRect rDest(4, 0, min(88, spriteW), 64), CPoint ptSrc(20, 64*iTyp)
    // So source starts at x=20, dest starts at x=4, max dest width = 84-4=80
    // Clamp to fit within item and not overlap text area
    if (m_unitSprites && item.typeIndex >= 0) {
        int spriteW = m_unitSprites->w;
        int srcY = ITEM_HT * item.typeIndex;
        if (srcY + ITEM_HT <= m_unitSprites->h) {
            int srcX = 20;
            int iconW = std::min(spriteW - srcX, std::min(68, w - 8));
            if (iconW > 0) {
                SDL_Rect sr = {srcX, srcY, iconW, ITEM_HT};
                SDL_Rect dr = {x + 4, y, iconW, ITEM_HT};
                SDL_BlitSurface(m_unitSprites, &sr, dst, &dr);
            }
        }
    }

    // Text area starts after icon
    int textLeft = x + 72;
    int textRight = w - 4;
    int textW = textRight - textLeft;
    if (textW <= 0) return;

    // --- Unit name with status text (shadow text like original) ---
    TTF_Font* font = GetFont(11);
    if (font) {
        // Build display text matching original unit_wnd.cpp OnDrawItem
        std::string sText = item.name;

        if (m_type == VEHICLES) {
            CVehicle* pVeh = (CVehicle*)item.pUnit;
            sText += " ";

            // Transport: show AUTO/ROUTE prefix
            if (pVeh->GetData()->IsTransport()) {
                if (!pVeh->IsHpControl())
                    sText += CTransportData::m_sAuto;
                else if (pVeh->GetEvent() == CVehicle::route)
                    sText += CTransportData::m_sRoute;
            }

            // Show destination building name
            CBuilding* pBldg = theBuildingHex.GetBuilding(pVeh->GetPtHead());
            if (pBldg == NULL || pVeh->GetHexOwnership())
                pBldg = theBuildingHex.GetBuilding(pVeh->GetHexDest());
            if (pBldg != NULL && pBldg->GetOwner()->IsMe())
                sText += std::string("[") + pBldg->GetData()->GetDesc().c_str() + "]";
            else if (pVeh->GetData()->IsTransport()) {
                if (pVeh->GetRouteMode() == CVehicle::stop)
                    sText += CTransportData::m_sIdle;
                else
                    sText += CTransportData::m_sTravel;
            }
        } else {
            CBuilding* pBldg = (CBuilding*)item.pUnit;
            if (pBldg->IsConstructing())
                sText += " [Building]";
        }

        // Text is drawn above the status bars (original: rect.top=4, rect.bottom=60-statHt-4)
        int textTop = y + 4;
        int textBotLimit = y + ITEM_HT - m_statBarHt - 4;
        int textAreaH = textBotLimit - textTop;
        int th = 0;
        TTF_SizeText(font, "Ay", nullptr, &th);
        int textY = textTop + (textAreaH - th) / 2;

        RenderShadowText(dst, font, sText.c_str(), textLeft, textY, textW,
                         ULColors::Text, ULColors::TextShadow);
    }

    // --- Status bars at bottom (original: rect at y=60-statHt to y=60) ---
    int numBars = GetNumStatusBars(item.pUnit);
    if (numBars > 0) {
        int barY = y + ITEM_HT - m_statBarHt;
        RenderStatusBars(dst, item.pUnit, textLeft, barY, textW, numBars);
    }
}

int SDL2UnitList::GetNumStatusBars(CUnit* pUnit) {
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
        if (pBldg->GetTotalStore() > 0)
            return 2;  // damage + materials
        return 1;      // damage only
    }

    return 1;
}

// Render the 3-piece background for a status bar icon
void SDL2UnitList::Render3PieceBg(SDL_Surface* dst, int iconIdx, int x, int y, int w) {
    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || icon.cyBack <= 0) return;

    int bgSrcY = icon.cyIcon;  // Background row is below icon row in sprite sheet

    if (icon.typBack == 1) {  // back_3: left cap + tiled middle + right cap
        // Left cap
        if (icon.cxLeft > 0) {
            SDL_Rect sr = {0, bgSrcY, icon.cxLeft, icon.cyBack};
            SDL_Rect dr = {x, y, icon.cxLeft, icon.cyBack};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
        // Middle tile
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
        // Right cap
        if (icon.cxRight > 0) {
            SDL_Rect sr = {icon.cxLeft + icon.cxBack, bgSrcY, icon.cxRight, icon.cyBack};
            SDL_Rect dr = {x + w - icon.cxRight, y, icon.cxRight, icon.cyBack};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
    } else if (icon.typBack == 0) {  // full_back: stretch entire background
        SDL_Rect sr = {0, bgSrcY, icon.cxBack, icon.cyBack};
        SDL_Rect dr = {x, y, w, icon.cyBack};
        StretchBlit(icon.sheet, sr, dst, dr);
    } else {  // tile
        for (int tx = 0; tx < w; tx += icon.cxBack) {
            int bw = std::min(icon.cxBack, w - tx);
            SDL_Rect sr = {0, bgSrcY, bw, icon.cyBack};
            SDL_Rect dr = {x + tx, y, bw, icon.cyBack};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
    }
}

// DrawStatDone: tiles the stat icon sprite (e.g. the construction wrench / road /
// build-vehicle tool) across `percent`% of the bar ??? matching the original
// CStatInst::DrawStatDone. The previous version drew a flat colour block, which
// is why crane/build progress showed a solid bar instead of tool icons.
void SDL2UnitList::RenderIconDone(SDL_Surface* dst, int iconIdx, int percent,
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
    if (percent < 100) iEnd -= icon.cxIcon / 2;   // leave the last icon for 100%
    int iRight = left + (width * percent) / 100;
    iRight = std::max(left + 1, iRight);           // at least one icon when > 0
    int iStep = std::max(1, icon.cxIcon / 2);      // overlap by half (like the original)

    SDL_SetSurfaceBlendMode(icon.sheet, SDL_BLENDMODE_BLEND);
    for (int ix = left; ix < iRight; ix += iStep) {
        if (ix + icon.cxIcon > iEnd) break;
        SDL_Rect sr = {0, 0, icon.cxIcon, icon.cyIcon};   // base icon (frame 0)
        SDL_Rect dr = {ix, iconY, icon.cxIcon, icon.cyIcon};
        SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
    }
}

// DrawStatHave: have icons (green) then need icons (red/animated)
void SDL2UnitList::RenderIconHave(SDL_Surface* dst, int iconIdx, int have, int need,
                                   int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || icon.cxIcon <= 0 || icon.cyIcon <= 0) return;
    if (have <= 0 && need <= 0) return;

    int iconY = y + (h - icon.cyIcon) / 2;
    int barLeft = x + icon.leftOff;
    int barRight = x + w - icon.rightOff;
    int barW = barRight - barLeft;
    if (barW <= 0) return;

    int iDone;
    int needSrcX;
    if (have < need && need > 0) {
        iDone = (have * 100) / need;
        needSrcX = icon.cxIcon * 2;  // "need" icon (frame 2+)
    } else {
        if (have <= 0 || need <= 0)
            iDone = 100;
        else {
            iDone = (need * 100) / have;
            if (iDone == 0 && need > 0) iDone = 1;
        }
        needSrcX = icon.cxIcon;  // "excess" icon
    }

    int splitX = barLeft + (barW * iDone) / 100;
    int iStep = std::max(1, icon.cxIcon / 2);

    // ONE cursor advances by iStep across the bar; the "need" run continues from
    // where the "have" run stopped, on the same half-icon grid (matching the
    // original CStatInst::DrawStatHave). Restarting at the off-grid splitX shifted
    // the red icons and overlapped the seam icon (misaligned + doubled look).
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

// DrawStatText: text inside status bar background
void SDL2UnitList::RenderIconText(SDL_Surface* dst, int iconIdx, const char* text,
                                   int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    IconData& icon = m_iconData[iconIdx];
    if (!text || !text[0]) return;

    // Status text (e.g. "Auto: Idle") ??? 9pt was too small to read; 11 matches the
    // item-name font and still fits the bar height.
    TTF_Font* font = GetFont(11);
    if (!font) return;

    int textX = x + icon.leftOff;
    int textW = w - icon.leftOff - icon.rightOff;
    if (textW <= 0) return;

    int th = 0;
    TTF_SizeText(font, text, nullptr, &th);
    int textY = y + (h - th) / 2;

    SDL_Surface* ts = TTF_RenderText_Blended(font, text, ULColors::Text);
    if (ts) {
        SDL_Rect sr = {0, 0, std::min(ts->w, textW), ts->h};
        SDL_Rect dr = {textX, textY, sr.w, sr.h};
        SDL_BlitSurface(ts, &sr, dst, &dr);
        SDL_FreeSurface(ts);
    }
}

// DrawStatBar: continuous progress bar (for construction)
void SDL2UnitList::RenderIconBar(SDL_Surface* dst, int iconIdx, int percent,
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

    // Draw the gradient at its NATIVE height (cyIcon), vertically centered in the
    // box ??? exactly CStatInst::DrawStatBar (icons.cpp): rDest.top += (H - cyIcon)/2;
    // rDest.bottom = top + cyIcon. The 3-piece background drawn above is taller
    // (cyBack), so its carved gold frame shows as a clean border above/below the
    // bar. The previous fabricated vInset (cyBack/8) made the fill SHORTER than
    // cyIcon, so the dark box interior peeked out as black strips top and bottom.
    int fillH = icon.cyIcon;
    int fillY = y + (h - icon.cyIcon) / 2;
    if (fillH < 1) fillH = 1;

    int srcW = (icon.cxIcon * percent) / 100;
    if (srcW <= 0) srcW = 1;
    SDL_Rect sr = {0, 0, srcW, icon.cyIcon};
    SDL_Rect dr = {barLeft, fillY, fillW, fillH};
    StretchBlit(icon.sheet, sr, dst, dr);
}

// Render carried vehicle sprites
void SDL2UnitList::RenderCarrierCargo(SDL_Surface* dst, CVehicle* pVeh, int iconIdx,
                                       int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || icon.cxIcon <= 0 || icon.cyIcon <= 0) return;

    int iconY = y + (h - icon.cyIcon) / 2;
    int drawX = x + icon.leftOff;
    int rightLimit = x + w - icon.rightOff;

    POSITION pos = pVeh->GetCargoHeadPosition();
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

// Render materials using ICON_MATERIALS sprites
void SDL2UnitList::RenderMaterialsBar(SDL_Surface* dst, CUnit* pUnit, int iconIdx,
                                       int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || icon.cxIcon <= 0 || icon.cyIcon <= 0) return;

    int total = pUnit->GetTotalStore();
    if (total <= 0) return;

    // Match original: for vehicles use max capacity, for buildings use at least 500 per icon
    int maxStore = total;
    if (pUnit->GetUnitType() == CUnit::vehicle) {
        int maxMat = ((CVehicle*)pUnit)->GetMaxMaterials();
        if (maxMat > total) maxStore = maxMat;
    } else {
        int iStep = std::max(1, icon.cxIcon / 2);
        int barW = w - icon.leftOff - icon.rightOff;
        int numIcons = barW / iStep;
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
    // first of the next. Each type uses its own sprite column (srcX = i*cxIcon),
    // matching the original's `CPoint pt(iOn*cxIcon, 0)`.
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

void SDL2UnitList::RenderStatusBars(SDL_Surface* dst, CUnit* pUnit, int x, int y, int w, int numBars) {
    if (!pUnit || numBars <= 0 || w <= 0) return;

    // Divide available width equally among status bars (matching original)
    // Original: rect.right += (rDraw.right - rect.left) / (iNum - iOn)
    int usedW = 0;
    int barH = m_statBarHt;

    for (int iOn = 0; iOn < numBars; iOn++) {
        int barW = (w - usedW) / (numBars - iOn);
        int barX = x + usedW;
        usedW += barW;

        // Leave 1px gap between bars (matching original rect.left = rect.right + 1)
        int renderX = barX + 1;
        int renderW = barW - 1;

        if (pUnit->GetUnitType() == CUnit::vehicle) {
            CVehicle* pVeh = (CVehicle*)pUnit;

            if (iOn == 0) {
                // Bar 0: Damage/health. ICON_DAMAGE is a TYP_ICON 'bar' (its art is
                // a 1000px red->yellow->green gradient), so it draws with DrawStatBar:
                // the gradient is scaled by percent into the fill. Use RenderIconBar,
                // NOT RenderIconDone (which is for 'done'-type icons and was drawing a
                // flat colour that didn't match the original).
                int dmg = std::max(1, pVeh->GetDamagePer());
                RenderIconBar(dst, ICON_DAMAGE, dmg, renderX, y, renderW, barH);
            } else if (pVeh->GetData()->IsCarrier() &&
                       ((pVeh->GetData()->IsTransport() && iOn == 2) ||
                        (!pVeh->GetData()->IsTransport() && iOn == 1))) {
                // Carrier cargo (ICON_VEHICLES)
                RenderCarrierCargo(dst, pVeh, ICON_VEHICLES, renderX, y, renderW, barH);
            } else if (pVeh->GetData()->IsTransport() && iOn == 1) {
                // Route text (ICON_BAR_TEXT)
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
                // Crane construction progress
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
                // Materials (ICON_MATERIALS, DrawStatHave-style)
                RenderMaterialsBar(dst, pVeh, ICON_MATERIALS, renderX, y, renderW, barH);
            }
        } else if (pUnit->GetUnitType() == CUnit::building) {
            CBuilding* pBldg = (CBuilding*)pUnit;

            if (iOn == 0) {
                // Damage/health ??? gradient 'bar' icon (see vehicle note above).
                int dmg = std::max(1, pBldg->GetDamagePer());
                RenderIconBar(dst, ICON_DAMAGE, dmg, renderX, y, renderW, barH);
            } else if (iOn == 1) {
                // Materials
                RenderMaterialsBar(dst, pBldg, ICON_MATERIALS, renderX, y, renderW, barH);
            } else if (iOn == 2) {
                // Construction progress
                int per = std::max(1, pBldg->GetBuildPer());
                RenderIconDone(dst, ICON_CONSTRUCTION, per, renderX, y, renderW, barH);
            }
        }
    }
}

void SDL2UnitList::OnClick(int itemIdx, bool dblClick) {
    if (itemIdx < 0 || itemIdx >= (int)m_items.size()) return;

    CUnit* pUnit = m_items[itemIdx].pUnit;
    CWndArea* pArea = theAreaList.GetTop();

    if (dblClick) {
        // ShowWindow() brings the relevant area map to the top of the list and
        // centers it on the unit. (Multiple area maps are supported ??? BringToTop
        // picks the right one.)
        if (pUnit->GetUnitType() == CUnit::vehicle)
            ((CVehicle*)pUnit)->ShowWindow();
        else if (pUnit->GetUnitType() == CUnit::building)
            ((CBuilding*)pUnit)->ShowWindow();

        // Re-fetch the top map (ShowWindow may have raised a different one).
        CWndArea* pTop = theAreaList.GetTop();
        if (pTop) {
            // Hand keyboard focus back to the map window FIRST, THEN select.
            // Clicking in this detached unit-list window moved OS focus here, so
            // we raise/focus the map's SDL window to restore arrow-key scrolling.
            // But that changes OS foreground, which sends WM_ACTIVATE(WA_INACTIVE)
            // to the *separate* MFC area window ??? and CWndArea::OnActivate clears
            // every unit's selected flag on deactivation (legacy single-window
            // behavior). If we selected before this, that deselect would silently
            // drop the selection box while leaving m_pUnit set (the unit still
            // looks selected on the toolbar, but loses its rectangle on the map).
            // Selecting AFTER the focus churn makes the selection stick.
            SDL_Window* mapWin = nullptr;
            if (SDL2Panel* p = pTop->GetAA().m_sdlPanel)
                if (p->IsDetached())
                    mapWin = p->GetOwnWindow();
            if (!mapWin && theApp.m_gameWindow)
                mapWin = theApp.m_gameWindow->GetWindow();
            if (mapWin) {
                SDL_RaiseWindow(mapWin);
                SDL_SetWindowInputFocus(mapWin);
            }

            pTop->OnlySelectUnit(pUnit);
            pTop->InvalidateWindow();
        }
    } else {
        if (pArea) {
            pArea->OnlySelectUnit(pUnit);
            pArea->InvalidateWindow();

            if (pUnit->GetUnitType() == CUnit::vehicle) {
                CHexCoord hex = ((CVehicle*)pUnit)->GetPtHead();
                pArea->Center(CMapLoc(hex));
            } else if (pUnit->GetUnitType() == CUnit::building) {
                pArea->Center(CMapLoc(((CBuilding*)pUnit)->GetHex()));
            }
        }
    }
}

bool SDL2UnitList::HandleEvent(SDL_Event& event, int localX, int localY) {
    if (!m_panel) return false;
    int w = m_panel->GetWidth();
    int h = m_panel->GetHeight();
    int totalH = (int)m_items.size() * ITEM_HT;
    int maxScroll = std::max(0, totalH - h);

    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            // Check if clicking on scrollbar area
            if (localX >= w - SB_WIDTH && totalH > h) {
                int sbH = std::max(24, (h * h) / totalH);
                int sbY = (maxScroll > 0) ? (m_scrollY * (h - sbH)) / maxScroll : 0;

                if (localY >= sbY && localY < sbY + sbH) {
                    // Drag the thumb
                    m_sbDragging = true;
                    m_sbDragOffset = localY - sbY;
                } else {
                    // Click above/below thumb ??? page up/down
                    if (localY < sbY)
                        m_scrollY = std::max(0, m_scrollY - h);
                    else
                        m_scrollY = std::min(maxScroll, m_scrollY + h);
                }
            } else {
                // Click on list item
                int itemIdx = (localY + m_scrollY) / ITEM_HT;

                DWORD now = ::timeGetTime();
                bool dblClick = (itemIdx == m_lastClickIdx && now - m_lastClickTime < 400);
                m_lastClickTime = now;
                m_lastClickIdx = itemIdx;

                m_selectedIdx = itemIdx;
                OnClick(itemIdx, dblClick);
            }
            m_forceDraw = true;  // repaint immediately so selection/scroll feels instant
        }
        return true;

    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT)
            m_sbDragging = false;
        return true;

    case SDL_MOUSEWHEEL:
        m_scrollY -= event.wheel.y * 48;
        if (m_scrollY < 0) m_scrollY = 0;
        if (m_scrollY > maxScroll) m_scrollY = maxScroll;
        m_forceDraw = true;
        return true;

    case SDL_MOUSEMOTION:
        if (m_sbDragging && totalH > h) {
            int sbH = std::max(24, (h * h) / totalH);
            int trackH = h - sbH;
            if (trackH > 0) {
                int newSbY = localY - m_sbDragOffset;
                m_scrollY = (newSbY * maxScroll) / trackH;
                if (m_scrollY < 0) m_scrollY = 0;
                if (m_scrollY > maxScroll) m_scrollY = maxScroll;
            }
            m_forceDraw = true;
        }
        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
        return true;
    }
    return false;
}
