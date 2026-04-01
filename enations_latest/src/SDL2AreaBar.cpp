#include "stdafx.h"

#include "SDL2AreaBar.h"
#include "SDL2Panel.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "area.h"
#include "bitmaps.h"
#include "bmbutton.h"
#include "icons.h"

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

    const char* fonts[] = {"C:\\Windows\\Fonts\\arial.ttf", nullptr};
    for (int i = 0; fonts[i]; i++) {
        FILE* f = fopen(fonts[i], "rb");
        if (f) { fclose(f); m_fontPath = fonts[i]; break; }
    }
}

SDL2AreaBar::~SDL2AreaBar() {
    if (m_bgTile) SDL_FreeSurface(m_bgTile);
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

    // Compute button positions (matching CWndAreaStatic layout)
    int x = AREA_BTN_X_SKIP;
    int y = AREA_BTN_Y_START;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        m_btnPos[i] = {x, y, m_btnW, m_btnH};
        if (s_advPos[i])
            x += m_btnW + AREA_BTN_X_SKIP;
    }
    m_totalW = x;
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
        // Build/CancelBuild toggle for factory
        bool buildReady = (iMode == CWndArea::build_ready);
        m_btns[ORD_OFFSET + 2].visible = !buildReady;
        m_btns[ORD_OFFSET + 2].enabled = !buildReady;
        m_btns[ORD_OFFSET + 3].visible =  buildReady;
        m_btns[ORD_OFFSET + 3].enabled =  buildReady;

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

    // Tile background
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
        SDL_FillRect(dst, nullptr, SDL_MapRGB(dst->format, 50, 55, 52));
    }

    // Render buttons
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (!m_btns[i].visible) continue;
        RenderButton(dst, i, m_btnPos[i].x, m_btnPos[i].y, m_btnPos[i].w, m_btnPos[i].h);
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

    case SDL_MOUSEMOTION:
        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
        return true;
    }
    return false;
}
