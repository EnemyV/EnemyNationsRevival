#include "stdafx.h"

#include "SDL2AreaBar.h"
#include "SDL2Panel.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "player.h"       // CPlayer (housing/occupancy bars), theGame.GetMe
#include "area.h"
#include "building.h"
#include "building.inl"   // CBuilding::IsConstructing, CVehicleBuilding::GetBldUnt (inline)
#include "vehicle.inl"    // CVehicle inlines + theTransports / theVehicleMap
#include "unit.inl"       // CUnit::GetUnitType/IsPaused inlines (needed at /Ob2)
#include "terrain.inl"    // CHexCoord/CMapLoc inlines for the building-hex lookups (/Ob2)
#include "bitmaps.h"
#include "bmbutton.h"
#include "icons.h"
#include "sfx.h"        // theMusicPlayer, SOUNDS, SFXPRIORITY (button click sound)
#include "EnSettings.h" // EnLoadStdString (load the IDH_* flyby help strings)
#include "resource.h"   // IDH_AREA_* / IDH_UNIT_* help-string IDs

#include <SDL.h>

#undef min
#undef max
#include <algorithm>

// Button sprite indices (from area.cpp abBtn[])
static const int s_spriteIdx[17] = {54, 37, 36, 25, 26, 14, 39, 34, 4, 22, 47, 40, 35, 5, 23, 32, 45};

// Command IDs (from area.cpp abID[])
static const int s_cmdID[17] = {
    IDC_AREA_COMBAT, IDC_AREA_CLOCK, IDC_AREA_COUNTER,
    IDC_AREA_ZOOM_IN, IDC_AREA_ZOOM_OUT, IDC_AREA_RES,
    IDC_UNIT_STOP, IDC_UNIT_RESUME,
    IDC_UNIT_BUILD, IDC_UNIT_CANCEL_BUILD,
    IDC_UNIT_ROUTE, IDC_UNIT_UNLOAD, IDC_UNIT_RETREAT,
    IDC_UNIT_ROAD, IDC_UNIT_CANCEL_ROAD,
    IDC_UNIT_REPAIR, IDC_UNIT_CANCEL_REPAIR
};

// Position advance flags (1 = move to next column after this button)
static const int s_advPos[17] = {1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1};

// Labels for tooltips
static const char* s_labels[17] = {
    "Last Combat", "Rotate CW", "Rotate CCW", "Zoom In", "Zoom Out", "Resources",
    "Stop", "Resume", "Build", "Cancel Build",
    "Route", "Unload", "Retreat", "Road", "Cancel Road",
    "Repair", "Cancel Repair"
};

// IDH_* flyby help-string IDs (from area.cpp abHelp[], same button order) — the
// sentence-length descriptions the original showed in the status line on hover.
static const int s_helpID[17] = {
    IDH_AREA_COMBAT, IDH_AREA_CLOCK, IDH_AREA_COUNTER, IDH_AREA_ZOOM_IN, IDH_AREA_ZOOM_OUT, IDH_AREA_RES,
    IDH_UNIT_STOP, IDH_UNIT_RESUME, IDH_UNIT_BUILD, IDH_UNIT_CANCEL_BUILD,
    IDH_UNIT_ROUTE, IDH_UNIT_UNLOAD, IDH_UNIT_RETREAT, IDH_UNIT_ROAD, IDH_UNIT_CANCEL_ROAD,
    IDH_UNIT_REPAIR, IDH_UNIT_CANCEL
};

// Keyboard shortcuts (same button order) — from the original area-map accelerator
// table (IDA_OPPO/BUILD/ROUTE/UNLOAD/RETREAT, enlang17.rc) and the live key handler
// in area.cpp's SDL callback (O/B/R/U/X). Shown in the help text like the main toolbar
// does ("(Ctrl+A)  ..."). Empty = the command has no accelerator. The rotate keys
// are , and . but display as < and > (reads as the turn direction).
static const char* s_shortcut[17] = {
    "O", ">", "<", "+", "-", "",   // Last Combat, Rotate CW(.), Rotate CCW(,), Zoom In(+), Zoom Out(-), Resources
    "", "", "B", "",             // Stop, Resume, Build, Cancel Build
    "R", "U", "X", "R", "",      // Route, Unload, Retreat, Road, Cancel Road  (R is context: crane=Road, else=Route)
    "", ""                       // Repair, Cancel Repair
};

SDL2AreaBar::SDL2AreaBar() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        m_btns[i].spriteIndex = s_spriteIdx[i];
        m_btns[i].cmdID = s_cmdID[i];
        m_btns[i].visible = (i < ORD_OFFSET);  // First 6 always visible
        m_btns[i].enabled = (i < ORD_OFFSET);
        m_btns[i].pressed = false;
        m_btns[i].advancePos = (s_advPos[i] != 0);
        m_btns[i].label = s_labels[i];
    }

    const char* fonts[] = {"/System/Library/Fonts/Supplemental/Arial.ttf", "/System/Library/Fonts/Supplemental/Times New Roman.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "C:\\Windows\\Fonts\\arial.ttf", nullptr};
    for (int i = 0; fonts[i]; i++) {
        FILE* f = fopen(fonts[i], "rb");
        if (f) { fclose(f); m_fontPath = fonts[i]; break; }
    }
}

SDL2AreaBar::~SDL2AreaBar() {
    if (m_btnSheet) SDL_FreeSurface(m_btnSheet);
    if (m_bgTile) SDL_FreeSurface(m_bgTile);
    for (auto& icon : m_iconData)
        if (icon.sheet) SDL_FreeSurface(icon.sheet);
    for (auto& p : m_fontCache)
        if (p.second) TTF_CloseFont(p.second);
}

TTF_Font* SDL2AreaBar::GetFont(int size) {
    if (m_fontPath.empty()) return nullptr;
    auto it = m_fontCache.find(size);
    if (it != m_fontCache.end()) return it->second;
    TTF_Font* f = TTF_OpenFont(m_fontPath.c_str(), size);
    m_fontCache[size] = f;
    return f;
}

void SDL2AreaBar::Init(SDL2Panel* panel, CWndArea* pArea, HWND hStaticWnd) {
    m_panel = panel;
    m_pArea = pArea;
    m_hStaticWnd = hStaticWnd;

    // Load button sprite sheet
    m_btnW = theBmBtnData.Width();
    m_btnH = theBmBtnData.Height();
    if (theBmBtnData.m_pcDib)
        m_btnSheet = SDL2MainMenu::CreateSurfaceFromDIB(theBmBtnData.m_pcDib);

    // Load area bar background
    CDIB* pBg = theBitmaps.GetByIndex(DIB_AREA_BAR);
    if (pBg)
        m_bgTile = SDL2MainMenu::CreateSurfaceFromDIB(pBg);

    // Load icon sprite sheets for the selected-unit status readout (ICON_* 0..14),
    // exactly as SDL2Toolbar does — these feed RenderUnitStatus / RenderStatusBars.
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
    // Status-bar row height = ICON_DAMAGE background height (matches the original
    // CWndUnitStat m_iStatHt).
    if (m_iconData[ICON_DAMAGE].cyBack > 0)
        m_statBarHt = m_iconData[ICON_DAMAGE].cyBack;

    // Compute button positions (matching CWndAreaStatic layout)
    int x = AREA_BTN_X_SKIP;
    int y = AREA_BTN_Y_START;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        m_btnPos[i] = {x, y, m_btnW, m_btnH};
        if (s_advPos[i])
            x += m_btnW + AREA_BTN_X_SKIP;
    }
    m_totalW = x;

    // Load the flyby help strings now that the string table is available (in-game).
    // These are the same IDH_* descriptions the original CMyButton showed on hover,
    // prefixed with the keyboard shortcut like the main toolbar ("(B)  Build ...").
    for (int i = 0; i < NUM_BUTTONS; i++) {
        std::string h = EnLoadStdString(s_helpID[i]);
        if (h.empty()) h = m_btns[i].label;
        if (s_shortcut[i][0])
            h = std::string("(") + s_shortcut[i] + ")  " + h;
        m_btns[i].helpText = h;
    }
}

void SDL2AreaBar::UpdateButtons() {
    if (!m_pArea) return;

    // First 6 buttons (zoom, rotate, etc.) — always visible
    for (int i = 0; i < ORD_OFFSET; i++) {
        m_btns[i].visible = true;
        m_btns[i].enabled = true;
    }

    // Order button indices (relative to ORD_OFFSET):
    // 0=Stop, 1=Resume, 2=Build, 3=CancelBuild, 4=Route,
    // 5=Unload, 6=Retreat, 7=Road, 8=CancelRoad, 9=Repair, 10=CancelRepair

    // Default: all hidden
    for (int i = ORD_OFFSET; i < NUM_BUTTONS; i++) {
        m_btns[i].visible = false;
        m_btns[i].enabled = false;
    }

    unsigned flags = m_pArea->m_uFlags;
    if (flags == 0) return;  // nothing selected

    int iMode = m_pArea->GetMode();

    // --- Stop/Resume toggle (shared by all unit types that can_stop) ---
    // Mirrors SetButtonState stop/resume logic: check if any unit is paused
    auto SetStopResume = [&](bool canStop) {
        if (!canStop) return;  // both hidden (default)
        bool anyPaused = false;
        POSITION pos = m_pArea->m_lstUnits.GetHeadPosition();
        while (pos) {
            CUnit* pUnit = m_pArea->m_lstUnits.GetNext(pos);
            if (pUnit->IsPaused()) { anyPaused = true; break; }
        }
        if (anyPaused) {
            // Resume visible, Stop hidden
            m_btns[ORD_OFFSET + 1].visible = true;
            m_btns[ORD_OFFSET + 1].enabled = true;
        } else {
            // Stop visible, Resume hidden
            m_btns[ORD_OFFSET + 0].visible = true;
            m_btns[ORD_OFFSET + 0].enabled = true;
        }
    };

    bool isCraneOnly  = (flags & (CWndArea::bldg | CWndArea::non_crane))   == 0;
    bool isTruckOnly  = (flags & (CWndArea::bldg | CWndArea::non_truck))   == 0;
    bool isUnloadable = (flags & CWndArea::loaded) ||
                        ((flags & (CWndArea::bldg | CWndArea::non_carrier)) == 0);
    bool isVehOnly    = !(flags & CWndArea::bldg);
    bool isFac        = (flags & CWndArea::fac) && (m_pArea->m_lstUnits.GetCount() == 1);
    bool isBldgOnly   = !(flags & CWndArea::veh);

    if (isCraneOnly) {
        // Stop/Resume toggle
        SetStopResume(true);

        // Build/CancelBuild toggle — only for single crane
        if (m_pArea->m_lstUnits.GetCount() == 1) {
            bool buildReady = (iMode == CWndArea::build_ready);
            m_btns[ORD_OFFSET + 2].visible = !buildReady;
            m_btns[ORD_OFFSET + 2].enabled = !buildReady;
            m_btns[ORD_OFFSET + 3].visible =  buildReady;
            m_btns[ORD_OFFSET + 3].enabled =  buildReady;
        } else {
            // Multiple cranes: Build visible but disabled, CancelBuild hidden
            m_btns[ORD_OFFSET + 2].visible = true;
            m_btns[ORD_OFFSET + 2].enabled = false;
        }

        // Road/CancelRoad toggle
        bool roadActive = (iMode == CWndArea::road_begin || iMode == CWndArea::road_set);
        m_btns[ORD_OFFSET + 7].visible = !roadActive;
        m_btns[ORD_OFFSET + 7].enabled = !roadActive;
        m_btns[ORD_OFFSET + 8].visible =  roadActive;
        m_btns[ORD_OFFSET + 8].enabled =  roadActive;

        // Repair/CancelRepair toggle
        bool repairActive = (iMode == CWndArea::repair_bldg);
        m_btns[ORD_OFFSET + 9].visible  = !repairActive;
        m_btns[ORD_OFFSET + 9].enabled  = !repairActive;
        m_btns[ORD_OFFSET + 10].visible =  repairActive;
        m_btns[ORD_OFFSET + 10].enabled =  repairActive;

    } else if (isTruckOnly) {
        SetStopResume(true);
        m_btns[ORD_OFFSET + 4].visible = true;
        m_btns[ORD_OFFSET + 4].enabled = true;

    } else if (isUnloadable) {
        SetStopResume(true);
        m_btns[ORD_OFFSET + 5].visible = true;
        m_btns[ORD_OFFSET + 5].enabled = (flags & CWndArea::loaded) != 0;

    } else if (isVehOnly) {
        SetStopResume(true);
        m_btns[ORD_OFFSET + 4].visible = true;
        m_btns[ORD_OFFSET + 4].enabled = true;

    } else if (isFac) {
        SetStopResume((flags & CWndArea::can_stop) != 0);
        // Build/CancelBuild toggle for factory. Mirrors the "build -> cancel"
        // branch of the original CWndArea::SetButtonState: a vehicle factory or
        // shipyard that has finished constructing itself and currently has a unit
        // queued (GetBldUnt() != NULL) shows Cancel Build instead of Build. (The
        // crane-only path keys off build_ready; a producing factory does not, so
        // checking iMode alone never flipped the icon here.)
        bool bBuilding = (iMode == CWndArea::build_ready);
        if (!bBuilding) {
            POSITION pos = m_pArea->m_lstUnits.GetHeadPosition();
            CUnit* pUnit = pos ? m_pArea->m_lstUnits.GetNext(pos) : nullptr;
            if (pUnit && pUnit->GetUnitType() == CUnit::building) {
                CBuilding* pBldg = (CBuilding*)pUnit;
                CStructureData::BLDG_UNION_TYPE ut = pBldg->GetData()->GetUnionType();
                if (!pBldg->IsConstructing() &&
                    (ut == CStructureData::UTvehicle || ut == CStructureData::UTshipyard) &&
                    ((CVehicleBuilding*)pBldg)->GetBldUnt() != NULL)
                    bBuilding = true;
            }
        }
        m_btns[ORD_OFFSET + 2].visible = !bBuilding;
        m_btns[ORD_OFFSET + 2].enabled = !bBuilding;
        m_btns[ORD_OFFSET + 3].visible =  bBuilding;
        m_btns[ORD_OFFSET + 3].enabled =  bBuilding;

    } else if (isBldgOnly) {
        SetStopResume((flags & CWndArea::can_stop) != 0);

    } else {
        // Mixed bldg + veh
        SetStopResume((flags & CWndArea::can_stop) != 0);
    }
}

void SDL2AreaBar::Render() {
    if (!m_panel) return;
    SDL_Surface* dst = m_panel->GetSurface();
    if (!dst) return;

    int w = m_panel->GetWidth();
    int h = m_panel->GetHeight();

    // Update button states from MFC
    UpdateButtons();

    // Stretch the brushed-gold background to fill the bar. (It was tiled before,
    // but the strip isn't seamless, so the repeat seam was visible — stretching
    // matches the original single-piece look.)
    if (m_bgTile) {
        SDL_Rect dr = {0, 0, w, h};
        SDL_BlitScaled(m_bgTile, nullptr, dst, &dr);
    } else {
        SDL_FillRect(dst, nullptr, SDL_MapRGB(dst->format, 50, 55, 52));
    }

    // Render buttons
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (!m_btns[i].visible) continue;
        RenderButton(dst, i, m_btnPos[i].x, m_btnPos[i].y, m_btnPos[i].w, m_btnPos[i].h);
    }

    // Selected-unit status readout in the right region — restores the 1996 local
    // status bar (name + damage + materials/cargo). Data source is the existing
    // CWndUnitStat feed via GetStaticUnit(); nothing selected -> region stays as-is.
    if (m_pArea) {
        CUnit* pUnit = m_pArea->GetStaticUnit();
        if (pUnit) {
            // Status start = the same two fixed anchors CWndAreaStatic::OnCreate set:
            // a crane shows the extra Road/Repair buttons, so its readout starts
            // past ALL order buttons (m_totalW); otherwise it starts at the Road
            // button's column (index 13). Both + one AREA_BTN_X_SKIP, matching the
            // original m_iStatusCraneStrt / m_iStatusNoCraneStrt (area.cpp:1012,1015).
            unsigned flags = m_pArea->m_uFlags;
            bool isCrane = flags != 0 &&
                           ((flags & (CWndArea::bldg | CWndArea::non_crane)) == 0);
            int statX  = (isCrane ? m_totalW : m_btnPos[13].x) + AREA_BTN_X_SKIP;
            int statW  = w - statX - AREA_BTN_X_SKIP;
            int statY  = (h - AREA_TEXT_HT) / 2;   // vertically centered (SizeStatus)
            if (statW > 0)
                RenderUnitStatus(dst, pUnit, statX, statY, statW, AREA_TEXT_HT);
        }
    }

    m_panel->SetDirty();
}

void SDL2AreaBar::RenderButton(SDL_Surface* dst, int idx, int x, int y, int w, int h) {
    if (m_btnSheet && m_btnW > 0 && m_btnH > 0) {
        int srcX = 0;
        if (!m_btns[idx].enabled) srcX = m_btnW * 2;
        else if (m_btns[idx].pressed) srcX = m_btnW;

        int srcY = m_btnH * m_btns[idx].spriteIndex;
        SDL_Rect sr = {srcX, srcY, m_btnW, m_btnH};
        SDL_Rect dr = {x, y, m_btnW, m_btnH};
        SDL_BlitSurface(m_btnSheet, &sr, dst, &dr);
    }
}

bool SDL2AreaBar::HandleEvent(SDL_Event& event, int localX, int localY) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            for (int i = 0; i < NUM_BUTTONS; i++) {
                if (!m_btns[i].visible || !m_btns[i].enabled) continue;
                BtnPos& bp = m_btnPos[i];
                if (localX >= bp.x && localX < bp.x + bp.w &&
                    localY >= bp.y && localY < bp.y + bp.h) {
                    m_btns[i].pressed = true;
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
            m_btns[i].pressed = false;
            m_pressedBtn = -1;
            BtnPos& bp = m_btnPos[i];
            if (localX >= bp.x && localX < bp.x + bp.w &&
                localY >= bp.y && localY < bp.y + bp.h) {
                // Post command to CWndArea via WM_COMMAND.
                // PostMessage (not SendMessage) avoids re-entrant PollEvents
                // when the command opens a DoModal dialog.
                if (m_pArea && m_pArea->m_hWnd)
                    m_pArea->PostMessage(WM_COMMAND,
                        MAKEWPARAM(m_btns[i].cmdID, BN_CLICKED), 0);
            }
            return true;
        }
        return true;

    case SDL_MOUSEMOTION: {
        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
        // Flyby help: show the hovered button's help text in the bottom-right status
        // line (line 1) — the same line the main toolbar buttons use. Mirrors the
        // original CMyButton -> WM_BUTTONMOUSEMOVE -> SetStatusText(1, GetHelp()).
        int hovered = -1;
        for (int i = 0; i < NUM_BUTTONS; i++) {
            if (!m_btns[i].visible) continue;
            BtnPos& bp = m_btnPos[i];
            if (localX >= bp.x && localX < bp.x + bp.w &&
                localY >= bp.y && localY < bp.y + bp.h) { hovered = i; break; }
        }
        if (hovered != m_hoverBtn) {
            m_hoverBtn = hovered;
            theApp.m_wndBar.SetStatusText(1, hovered >= 0 ? m_btns[hovered].helpText.c_str() : "");
        }
        return true;
    }
    }
    return false;
}


/////////////////////////////////////////////////////////////////////////////
// Selected-unit status readout (name + damage + materials/cargo icon bars).
// Faithful port of the MFC CWndUnitStat path (_UnitShowStatus / CStatInst /
// CUnit::PaintStatusBars), copied from SDL2Toolbar's proven hover-readout so the
// area bar and the main toolbar render unit status identically. Data comes from
// CWndArea::GetStaticUnit() (the existing StatUnit -> SetUnit feed).

// Software StretchBlt: scale src(sr) to dst(dr).
static void AB_StretchBlit(SDL_Surface* src, SDL_Rect sr, SDL_Surface* dst, SDL_Rect dr) {
    if (!src || !dst || dr.w <= 0 || dr.h <= 0 || sr.w <= 0 || sr.h <= 0) return;
    SDL_BlitScaled(src, &sr, dst, &dr);
}

int SDL2AreaBar::GetNumStatusBars(CUnit* pUnit) {
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
        int bt = pBldg->GetData()->GetBldgType();
        if (bt == CStructureData::apartment || bt == CStructureData::office)
            return 2;  // damage + population
        int ut = pBldg->GetData()->GetUnionType();
        int base;
        if (ut == CStructureData::UTvehicle || ut == CStructureData::UTshipyard)
            base = 3;
        else if (ut == CStructureData::UTfarm)
            base = (pBldg->GetData()->GetType() == CStructureData::farm) ? 2 : 3;
        else if (pBldg->GetTotalStore() > 0)
            base = 2;  // damage + materials
        else
            base = 1;  // damage only
        if (CountContainedUnits(pBldg) > 0)
            base++;    // trailing bar for units parked inside
        return base;
    }

    return 1;
}

// Render the 3-piece background for a status bar icon.
void SDL2AreaBar::Render3PieceBg(SDL_Surface* dst, int iconIdx, int x, int y, int w) {
    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || icon.cyBack <= 0) return;

    int bgSrcY = icon.cyIcon;  // background row is below the icon row

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
        AB_StretchBlit(icon.sheet, sr, dst, dr);
    } else {  // tile
        for (int tx = 0; tx < w; tx += icon.cxBack) {
            int bw = std::min(icon.cxBack, w - tx);
            SDL_Rect sr = {0, bgSrcY, bw, icon.cyBack};
            SDL_Rect dr = {x + tx, y, bw, icon.cyBack};
            SDL_BlitSurface(icon.sheet, &sr, dst, &dr);
        }
    }
}

// Continuous gradient progress bar (damage/health).
void SDL2AreaBar::RenderIconBar(SDL_Surface* dst, int iconIdx, int percent,
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
    AB_StretchBlit(icon.sheet, sr, dst, dr);
}

// Tiles the stat icon sprite across `percent`% of the bar (construction/road/etc).
void SDL2AreaBar::RenderIconDone(SDL_Surface* dst, int iconIdx, int percent,
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

// Text inside a status bar background (unit name / route text).
void SDL2AreaBar::RenderIconText(SDL_Surface* dst, int iconIdx, const char* text,
                                 int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    IconData& icon = m_iconData[iconIdx];
    if (!text || !text[0]) return;

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

// Carried vehicle sprites (a carrier's cargo).
void SDL2AreaBar::RenderCarrierCargo(SDL_Surface* dst, CVehicle* pVeh, int iconIdx,
                                     int x, int y, int w, int h) {
    Render3PieceBg(dst, iconIdx, x, y, w);

    IconData& icon = m_iconData[iconIdx];
    if (!icon.sheet || icon.cxIcon <= 0 || icon.cyIcon <= 0) return;

    int iconY = y + (h - icon.cyIcon) / 2;
    int drawX = x + icon.leftOff;
    int rightLimit = x + w - icon.rightOff;

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

// Count friendly vehicles parked inside a building (ships in a seaport, etc).
int SDL2AreaBar::CountContainedUnits(CUnit* pUnit) {
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

// Draw the icons of the vehicles parked inside a building (reveals seaport/factory contents).
void SDL2AreaBar::RenderContainedUnits(SDL_Surface* dst, CBuilding* pBldg, int iconIdx,
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

// Materials icon bars — faithful port of CUnit::PaintStatusMaterials.
void SDL2AreaBar::RenderMaterialsBar(SDL_Surface* dst, CUnit* pUnit, int iconIdx,
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

    // Single cursor advances by cxIcon/2 across ALL material types (no per-type
    // reset), each type reserving room for one icon of every remaining type —
    // mirrors CUnit::PaintStatusMaterials.
    int iIconAdd = std::max(1, icon.cxIcon / 2);
    int iLen   = barW - icon.cxIcon;
    int iRight = barRight - icon.cxIcon;

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
        iWid = std::min(iWid, iLen - iNumTypes * iIconAdd);
        iTotal -= stored;
        iWid += cursor;

        int srcX = i * icon.cxIcon;
        if (srcX + icon.cxIcon > icon.sheet->w)
            srcX = 0;

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

void SDL2AreaBar::RenderStatusBars(SDL_Surface* dst, CUnit* pUnit, int x, int y, int w, int numBars) {
    if (!pUnit || numBars <= 0 || w <= 0) return;

    int usedW = 0;
    int barH = (m_statBarHt > 0) ? m_statBarHt
             : (m_iconData[ICON_DAMAGE].cyBack > 0 ? m_iconData[ICON_DAMAGE].cyBack : 16);

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
                RenderContainedUnits(dst, pBldg, ICON_VEHICLES, renderX, y, renderW, barH);
            } else if (iOn == 0) {
                int dmg = std::max(1, pBldg->GetDamagePer());
                RenderIconBar(dst, ICON_DAMAGE, dmg, renderX, y, renderW, barH);
            } else if (isHousing && iOn == 1) {
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
                    int per = std::min(100, std::max(0, ((CFarmBuilding*)pBldg)->GetTerMult() * 10));
                    RenderIconDone(dst, ICON_DENSITY, per, renderX, y, renderW, barH);
                } else if (ut == CStructureData::UTvehicle || ut == CStructureData::UTshipyard) {
                    CVehicleBuilding* pVb = (CVehicleBuilding*)pBldg;
                    CBuildUnit const* pBu = pVb->GetBldUnt();
                    if (pBu == NULL) {
                        RenderIconDone(dst, ICON_BUILD_VEH, 0, renderX, y, renderW, barH);
                    } else {
                        RenderIconDone(dst, ICON_BUILD_VEH, std::max(1, pVb->GetBuildPer()),
                                       renderX, y, renderW, barH);
                        // Validate the vehicle type first: the sim thread can finish/cancel
                        // the build between GetBldUnt() and here (GetVehType() == -1).
                        int vehType = pBu->GetVehType();
                        std::string name;
                        if (vehType >= 0 && vehType < theTransports.GetNumTransports())
                            name = theTransports.GetData(vehType)->GetDesc();
                        TTF_Font* font = GetFont(14);
                        if (font && !name.empty()) {
                            SDL_Color white = {255, 255, 255, 255};
                            SDL_Surface* ts = TTF_RenderText_Blended(font, name.c_str(), white);
                            if (ts) {
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

void SDL2AreaBar::RenderUnitStatus(SDL_Surface* dst, CUnit* pUnit, int x, int y, int w, int h) {
    if (!pUnit || w <= 0) return;

    int barH = (m_statBarHt > 0 && m_statBarHt <= h) ? m_statBarHt : h;
    int barY = y + (h - barH) / 2;

    // Description (unit name) in an ICON_BAR_TEXT bar — left portion, capped at
    // ~14 chars, matching the original _UnitShowStatus(bText=TRUE).
    IconData& bt = m_iconData[ICON_BAR_TEXT];
    int descMax = 14 * 8 + bt.leftOff + bt.rightOff;
    int descW = std::min(w / 2, descMax);
    if (descW < 1) descW = w / 2;
    RenderIconText(dst, ICON_BAR_TEXT, pUnit->GetData()->GetDesc().c_str(), x, y, descW, h);

    int sx = x + descW + 1;
    int sw = w - descW - 1;
    if (sw <= 0) return;

    // Non-owned unit: owner name + damage only (matches _UnitShowStatus). The area
    // bar only feeds owned units, but keep the branch faithful to the original.
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
