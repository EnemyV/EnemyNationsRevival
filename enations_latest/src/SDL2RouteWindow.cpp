#include "stdafx.h"
#include "SDL2RouteWindow.h"
#include "GameWindow.h"
#include "SDL2Compositor.h"
#include "SDL2MainMenu.h"
#include "lastplnt.h"
#include "vehicle.h"
#include "building.h"
#include "area.h"
#include "player.h"
#include "bitmaps.h"
#include "chproute.hpp"
#include <SDL_ttf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

SDL2RouteWindow::SDL2RouteWindow(GameWindow* gw, CVehicle* pVeh, SDL2Panel* areaPanel)
    : m_gameWindow(gw), m_pVeh(pVeh) {

    // Find a font
    const char* fonts[] = {"C:\\Windows\\Fonts\\arial.ttf",
                           "C:\\Windows\\Fonts\\tahoma.ttf", nullptr};
    for (int i = 0; fonts[i]; i++) {
        FILE* f = fopen(fonts[i], "rb");
        if (f) { fclose(f); m_fontPath = fonts[i]; break; }
    }
    if (!m_fontPath.empty())
        m_fontSmall = TTF_OpenFont(m_fontPath.c_str(), 12);

    SDL2Compositor* comp = gw->GetCompositor();
    if (!comp) return;

    // Position near the right side of the area window
    int panelX = 20;
    int panelY = 100;

    m_panel = comp->AddPanel("route", panelX, panelY, PANEL_W, PANEL_H, 35);
    if (m_panel) {
        m_panel->SetMovable(true);
        m_panel->SetTitle("Vehicle Route");
        m_panel->SetVisible(false);
        m_panel->SetClosable(true);
        m_panel->SetCloseCallback([this]() { Close(); });

        m_panel->SetEventCallback(
            [this](SDL_Event& event, int lx, int ly) -> bool {
                return HandleEvent(event, lx, ly);
            });

        // Detach to own OS window, owned by the area view so it floats above it.
        if (areaPanel && areaPanel->IsDetached() && areaPanel->GetOwnWindow())
            m_panel->Detach(areaPanel->GetOwnWindow());
        else
            m_panel->Detach(gw);
    }

    // Create buttons
    int btnW = 70, gap = 4;
    int btnY1 = PANEL_H - BTN_H * 2 - gap * 3;
    int btnY2 = PANEL_H - BTN_H - gap;
    int startX = LIST_MARGIN;

    bool isTransport = m_pVeh->GetData()->IsTransport() ? true : false;

    // Row 1: Waypoint, Unload, Load
    m_buttons.push_back({{startX, btnY1, btnW, BTN_H}, "Waypoint", true, false, [this]() { OnWaypoint(); }});
    m_buttons.push_back({{startX + btnW + gap, btnY1, btnW, BTN_H}, "Unload",
                         isTransport, false, [this]() { OnUnload(); }});
    m_buttons.push_back({{startX + 2*(btnW+gap), btnY1, btnW, BTN_H}, "Load",
                         isTransport, false, [this]() { OnLoad(); }});

    // Row 2: Goto, Delete, Start, Auto, Close
    btnW = 52;
    m_buttons.push_back({{startX, btnY2, btnW, BTN_H}, "Goto", false, false, [this]() { OnGoto(); }});
    m_buttons.push_back({{startX + (btnW+gap), btnY2, btnW, BTN_H}, "Delete", false, false, [this]() { OnDelete(); }});
    m_buttons.push_back({{startX + 2*(btnW+gap), btnY2, btnW, BTN_H}, "Start", true, false, [this]() { OnStart(); }});
    m_buttons.push_back({{startX + 3*(btnW+gap), btnY2, btnW, BTN_H}, "Auto", true, false, [this]() { OnAuto(); }});
    m_buttons.push_back({{startX + 4*(btnW+gap), btnY2, btnW, BTN_H}, "Close", true, false, [this]() { OnClose(); }});

    m_visibleRows = (btnY1 - gap - LIST_Y) / ROW_H;
}

SDL2RouteWindow::~SDL2RouteWindow() {
    if (m_fontSmall) TTF_CloseFont(m_fontSmall);
    if (m_bgGold)   SDL_FreeSurface(m_bgGold);
    if (m_borderH)  SDL_FreeSurface(m_borderH);
    if (m_borderV)  SDL_FreeSurface(m_borderV);

    if (m_panel && m_gameWindow) {
        SDL2Compositor* comp = m_gameWindow->GetCompositor();
        if (comp)
            comp->RemovePanel(m_panel);
    }
}

void SDL2RouteWindow::LoadArt() {
    if (m_artLoaded) return;
    m_artLoaded = true;

    CDIB* pGold = theBitmaps.GetByIndex(DIB_GOLD);
    if (pGold) m_bgGold = SDL2MainMenu::CreateSurfaceFromDIB(pGold);

    CDIB* pH = theBitmaps.GetByIndex(DIB_BORDER_HORZ);
    if (pH) m_borderH = SDL2MainMenu::CreateSurfaceFromDIB(pH);

    CDIB* pV = theBitmaps.GetByIndex(DIB_BORDER_VERT);
    if (pV) m_borderV = SDL2MainMenu::CreateSurfaceFromDIB(pV);
}

void SDL2RouteWindow::RenderBackground(SDL_Surface* dst) {
    int w = dst->w, h = dst->h;
    int brdH = m_borderH ? m_borderH->h : 3;
    int brdV = m_borderV ? m_borderV->w : 3;

    // Gold background (scaled to fill)
    if (m_bgGold) {
        SDL_Rect sr = {0, 0, m_bgGold->w, m_bgGold->h};
        SDL_Rect dr = {0, 0, w, h};
        SDL_BlitScaled(m_bgGold, &sr, dst, &dr);
    } else {
        SDL_FillRect(dst, nullptr, SDL_MapRGB(dst->format, 160, 140, 90));
    }

    // Horizontal borders top/bottom
    if (m_borderH) {
        for (int tx = 0; tx < w; tx += m_borderH->w) {
            int bw = __min(m_borderH->w, w - tx);
            SDL_Rect sr = {0, 0, bw, m_borderH->h};
            SDL_Rect dr = {tx, 0, bw, m_borderH->h};
            SDL_BlitSurface(m_borderH, &sr, dst, &dr);
            dr.y = h - m_borderH->h;
            SDL_BlitSurface(m_borderH, &sr, dst, &dr);
        }
    }

    // Vertical borders left/right (beveled)
    if (m_borderV) {
        for (int ix = 0; ix < m_borderV->w && ix < w / 2; ix++) {
            int top = brdH + ix;
            int bot = h - brdH - ix;
            if (top >= bot) break;
            for (int ty = top; ty < bot; ty += m_borderV->h) {
                int bh = __min(m_borderV->h, bot - ty);
                SDL_Rect sr = {ix, ix, 1, bh};
                SDL_Rect drL = {ix, ty, 1, bh};
                SDL_Rect drR = {w - 1 - ix, ty, 1, bh};
                SDL_BlitSurface(m_borderV, &sr, dst, &drL);
                SDL_BlitSurface(m_borderV, &sr, dst, &drR);
            }
        }
    }
}

TTF_Font* SDL2RouteWindow::GetFont(int size) {
    if (size == 12) return m_fontSmall;
    return m_fontSmall;  // fallback
}

void SDL2RouteWindow::Show() {
    if (m_panel) {
        m_panel->SetVisible(true);
        std::string sTitle = strPrintf(EnLoadStdString(IDS_ROUTE_TITLE).c_str(),
                                       m_pVeh->GetData()->GetDesc().c_str());
        m_panel->SetTitle(sTitle);
    }
    RebuildList();
    Render();
}

void SDL2RouteWindow::Hide() {
    if (m_panel) m_panel->SetVisible(false);
}

void SDL2RouteWindow::Close() {
    m_pVeh->m_pSdlRoute = nullptr;
    Hide();
    delete this;
}

bool SDL2RouteWindow::IsVisible() const {
    return m_panel && m_panel->IsVisible();
}

void SDL2RouteWindow::RefreshRoute() {
    RebuildList();
    Render();
}

void SDL2RouteWindow::RebuildList() {
    m_entries.clear();
    if (!m_pVeh) return;

    POSITION pos = m_pVeh->GetRouteList().GetHeadPosition();
    while (pos != NULL) {
        POSITION curPos = pos;
        CRoute* pR = m_pVeh->GetRouteList().GetNext(pos);

        RouteEntry entry;
        entry.isCurrent = (curPos == m_pVeh->GetRoutePos());

        // Location name
        std::string loc;
        std::string sName;
        CBuilding* pBldg = theBuildingHex._GetBuilding(pR->GetCoord());
        if (pBldg != NULL)
            pBldg->GetDesc(sName);

        if (!sName.empty())
            loc = sName;
        else
            loc = std::to_string(pR->GetCoord().X()) + "," + std::to_string(pR->GetCoord().Y());

        // Route type
        const char* typeStr = "Waypoint";
        switch (pR->GetRouteType()) {
            case 1: typeStr = "Unload"; break;
            case 2: typeStr = "Load"; break;
        }

        entry.text = loc + " - " + typeStr;
        if (entry.isCurrent)
            entry.text = ">> " + entry.text;

        m_entries.push_back(entry);
    }

    // Update Goto/Delete button state
    bool hasSel = (m_selectedIndex >= 0 && m_selectedIndex < (int)m_entries.size());
    if (m_buttons.size() > 4) {
        m_buttons[3].enabled = hasSel;  // Goto
        m_buttons[4].enabled = hasSel;  // Delete
    }
}

void SDL2RouteWindow::Render() {
    if (!m_panel) return;

    SDL_Surface* surf = m_panel->GetSurface();
    if (!surf) return;

    LoadArt();
    TTF_Font* font = GetFont(12);

    int brdH = m_borderH ? m_borderH->h : 3;
    int brdV = m_borderV ? m_borderV->w : 3;

    // Gold background with beveled borders
    RenderBackground(surf);

    if (!font) { m_panel->SetDirty(); return; }

    int insetX = brdV + 4;
    int insetW = surf->w - insetX * 2;

    // List area — dark inset panel for the route list
    int listX = insetX;
    int listW = insetW;
    int listH = m_visibleRows * ROW_H;
    SDL_Rect listRect = {listX, LIST_Y, listW, listH};
    SDL_FillRect(surf, &listRect, SDL_MapRGB(surf->format, 30, 25, 18));

    // Beveled border around list (dark top-left, light bottom-right)
    Uint32 darkBrd  = SDL_MapRGB(surf->format, 60, 50, 35);
    Uint32 lightBrd = SDL_MapRGB(surf->format, 190, 170, 120);
    SDL_Rect bdr;
    bdr = {listX, LIST_Y, listW, 1}; SDL_FillRect(surf, &bdr, darkBrd);
    bdr = {listX, LIST_Y, 1, listH}; SDL_FillRect(surf, &bdr, darkBrd);
    bdr = {listX, LIST_Y + listH - 1, listW, 1}; SDL_FillRect(surf, &bdr, lightBrd);
    bdr = {listX + listW - 1, LIST_Y, 1, listH}; SDL_FillRect(surf, &bdr, lightBrd);

    // Draw entries
    for (int i = 0; i < m_visibleRows && (i + m_scrollOffset) < (int)m_entries.size(); i++) {
        int idx = i + m_scrollOffset;
        const auto& entry = m_entries[idx];
        int rowY = LIST_Y + i * ROW_H;

        // Highlight selected
        if (idx == m_selectedIndex) {
            SDL_Rect selRect = {listX + 1, rowY, listW - 2, ROW_H};
            SDL_FillRect(surf, &selRect, SDL_MapRGB(surf->format, 80, 65, 40));
        }

        // Current route: gold, others: dark text
        SDL_Color color = entry.isCurrent ?
            SDL_Color{255, 220, 100, 255} : SDL_Color{220, 210, 180, 255};

        SDL_Surface* text = TTF_RenderText_Blended(font, entry.text.c_str(), color);
        if (text) {
            SDL_Rect dst = {listX + 4, rowY + 1, 0, 0};
            SDL_BlitSurface(text, NULL, surf, &dst);
            SDL_FreeSurface(text);
        }
    }

    // Scrollbar if needed
    int entryCount = (int)m_entries.size();
    if (entryCount > m_visibleRows) {
        int sbX = listX + listW - 8;
        int maxScroll = entryCount - m_visibleRows;
        int sbH = listH * m_visibleRows / entryCount;
        if (sbH < 10) sbH = 10;
        int sbY = LIST_Y;
        if (maxScroll > 0)
            sbY = LIST_Y + (listH - sbH) * m_scrollOffset / maxScroll;
        SDL_Rect sbRect = {sbX, sbY, 6, sbH};
        SDL_FillRect(surf, &sbRect, SDL_MapRGB(surf->format, 140, 120, 80));
    }

    // Draw buttons — beveled style matching game aesthetic
    for (const auto& btn : m_buttons) {
        Uint32 bgColor;
        if (!btn.enabled)
            bgColor = SDL_MapRGB(surf->format, 120, 110, 80);
        else if (btn.pressed)
            bgColor = SDL_MapRGB(surf->format, 100, 85, 55);
        else
            bgColor = SDL_MapRGB(surf->format, 150, 135, 95);

        SDL_FillRect(surf, &btn.rect, bgColor);

        // Beveled button border (light top-left, dark bottom-right)
        Uint32 light = SDL_MapRGB(surf->format, 200, 185, 140);
        Uint32 dark  = SDL_MapRGB(surf->format, 80, 70, 45);
        if (btn.pressed) { Uint32 tmp = light; light = dark; dark = tmp; }

        bdr = {btn.rect.x, btn.rect.y, btn.rect.w, 1}; SDL_FillRect(surf, &bdr, light);
        bdr = {btn.rect.x, btn.rect.y, 1, btn.rect.h}; SDL_FillRect(surf, &bdr, light);
        bdr = {btn.rect.x, btn.rect.y + btn.rect.h - 1, btn.rect.w, 1}; SDL_FillRect(surf, &bdr, dark);
        bdr = {btn.rect.x + btn.rect.w - 1, btn.rect.y, 1, btn.rect.h}; SDL_FillRect(surf, &bdr, dark);

        // Button label — dark text on gold button
        SDL_Color txtColor = btn.enabled ?
            SDL_Color{30, 25, 15, 255} : SDL_Color{90, 85, 70, 255};
        SDL_Surface* text = TTF_RenderText_Blended(font, btn.label.c_str(), txtColor);
        if (text) {
            SDL_Rect dst = {
                btn.rect.x + (btn.rect.w - text->w) / 2,
                btn.rect.y + (btn.rect.h - text->h) / 2,
                0, 0
            };
            SDL_BlitSurface(text, NULL, surf, &dst);
            SDL_FreeSurface(text);
        }
    }

    m_panel->SetDirty();
}

bool SDL2RouteWindow::HandleEvent(SDL_Event& event, int localX, int localY) {
    if (!m_panel || !m_panel->IsVisible()) return false;

    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                // Check list click
                int listW = PANEL_W - LIST_MARGIN * 2;
                int listH = m_visibleRows * ROW_H;
                if (localX >= LIST_MARGIN && localX < LIST_MARGIN + listW &&
                    localY >= LIST_Y && localY < LIST_Y + listH) {
                    int row = (localY - LIST_Y) / ROW_H + m_scrollOffset;
                    if (row >= 0 && row < (int)m_entries.size()) {
                        m_selectedIndex = row;
                        if (m_buttons.size() > 4) {
                            m_buttons[3].enabled = true;
                            m_buttons[4].enabled = true;
                        }
                        Render();
                    }
                    return true;
                }

                // Check button clicks
                for (auto& btn : m_buttons) {
                    if (btn.enabled &&
                        localX >= btn.rect.x && localX < btn.rect.x + btn.rect.w &&
                        localY >= btn.rect.y && localY < btn.rect.y + btn.rect.h) {
                        btn.pressed = true;
                        Render();
                        return true;
                    }
                }
            }
            return true;

        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                for (auto& btn : m_buttons) {
                    if (btn.pressed) {
                        btn.pressed = false;
                        if (btn.enabled &&
                            localX >= btn.rect.x && localX < btn.rect.x + btn.rect.w &&
                            localY >= btn.rect.y && localY < btn.rect.y + btn.rect.h) {
                            if (btn.onClick) btn.onClick();
                            return true;
                        }
                        Render();
                        return true;
                    }
                }
            }
            return true;

        case SDL_MOUSEWHEEL:
            if (event.wheel.y > 0 && m_scrollOffset > 0) {
                m_scrollOffset--;
                Render();
            } else if (event.wheel.y < 0 &&
                       m_scrollOffset < (int)m_entries.size() - m_visibleRows) {
                m_scrollOffset++;
                Render();
            }
            return true;

        case SDL_MOUSEMOTION:
            return true;
    }

    return false;
}

void SDL2RouteWindow::OnWaypoint() {
    CWndArea* pArea = NULL;
    POSITION aPos = theAreaList.GetHeadPosition();
    if (aPos) pArea = theAreaList.GetNext(aPos);

    if (pArea) {
        POSITION pos = NULL;
        if (m_selectedIndex >= 0) {
            pos = m_pVeh->GetRouteList().GetHeadPosition();
            int n = m_selectedIndex;
            while (n > 0 && pos) { m_pVeh->GetRouteList().GetNext(pos); n--; }
        }
        pArea->GotoOn(m_pVeh, CWndArea::veh_route, CRoute::waypoint, pos);
        pArea->SetButtonState();
    }
}

void SDL2RouteWindow::OnUnload() {
    CWndArea* pArea = NULL;
    POSITION aPos = theAreaList.GetHeadPosition();
    if (aPos) pArea = theAreaList.GetNext(aPos);

    if (pArea) {
        POSITION pos = NULL;
        if (m_selectedIndex >= 0) {
            pos = m_pVeh->GetRouteList().GetHeadPosition();
            int n = m_selectedIndex;
            while (n > 0 && pos) { m_pVeh->GetRouteList().GetNext(pos); n--; }
        }
        pArea->GotoOn(m_pVeh, CWndArea::veh_route, CRoute::unload, pos);
        pArea->SetButtonState();
    }
}

void SDL2RouteWindow::OnLoad() {
    CWndArea* pArea = NULL;
    POSITION aPos = theAreaList.GetHeadPosition();
    if (aPos) pArea = theAreaList.GetNext(aPos);

    if (pArea) {
        POSITION pos = NULL;
        if (m_selectedIndex >= 0) {
            pos = m_pVeh->GetRouteList().GetHeadPosition();
            int n = m_selectedIndex;
            while (n > 0 && pos) { m_pVeh->GetRouteList().GetNext(pos); n--; }
        }
        pArea->GotoOn(m_pVeh, CWndArea::veh_route, CRoute::load, pos);
        pArea->SetButtonState();
    }
}

void SDL2RouteWindow::OnGoto() {
    if (m_selectedIndex < 0) return;

    int n = m_selectedIndex;
    POSITION pos = m_pVeh->GetRouteList().GetHeadPosition();
    while (n > 0 && pos) { m_pVeh->GetRouteList().GetNext(pos); n--; }
    m_pVeh->SetRoutePos(pos);
    RebuildList();
    Render();
}

void SDL2RouteWindow::OnDelete() {
    if (m_selectedIndex < 0) return;

    int n = m_selectedIndex;
    POSITION pos = m_pVeh->GetRouteList().GetHeadPosition();
    while (n > 0 && pos) { m_pVeh->GetRouteList().GetNext(pos); n--; }

    if (pos) {
        if (pos == m_pVeh->GetRoutePos()) {
            POSITION pos_next = pos;
            m_pVeh->GetRouteList().GetNext(pos_next);
            if (!pos_next)
                pos_next = m_pVeh->GetRouteList().GetHeadPosition();
            m_pVeh->SetRoutePos(pos_next);
        }
        CRoute* pR = m_pVeh->GetRouteList().GetAt(pos);
        m_pVeh->GetRouteList().RemoveAt(pos);
        delete pR;
    }

    if (m_selectedIndex >= (int)m_pVeh->GetRouteList().GetCount())
        m_selectedIndex = (int)m_pVeh->GetRouteList().GetCount() - 1;

    RebuildList();
    Render();
}

void SDL2RouteWindow::OnStart() {
    if (m_pVeh->GetRouteList().GetCount() <= 0) {
        OnAuto();
        return;
    }

    m_pVeh->SetEvent(CVehicle::route);

    CWndArea* pArea = NULL;
    POSITION aPos = theAreaList.GetHeadPosition();
    if (aPos) pArea = theAreaList.GetNext(aPos);
    if (pArea) pArea->SetButtonState();

    // Take from router and set hp_controls
    theGame.m_pHpRtr->MsgTakeVeh(m_pVeh);
    m_pVeh->HpControlOn();

    // Get it going
    POSITION routePos = m_pVeh->GetRoutePos();
    if (routePos) {
        CRoute* pR = m_pVeh->GetRouteList().GetAt(routePos);
        if (!pR) {
            m_pVeh->SetRoutePos(m_pVeh->GetRouteList().GetHeadPosition());
            routePos = m_pVeh->GetRoutePos();
            if (routePos)
                pR = m_pVeh->GetRouteList().GetAt(routePos);
        }
        if (pR)
            m_pVeh->SetDest(pR->GetCoord());
    }

    Close();
}

void SDL2RouteWindow::OnAuto() {
    POSITION pos = m_pVeh->GetRouteList().GetHeadPosition();
    while (pos) {
        CRoute* pR = m_pVeh->GetRouteList().GetNext(pos);
        delete pR;
    }
    m_pVeh->GetRouteList().RemoveAll();
    m_pVeh->SetRoutePos(NULL);
    m_pVeh->DumpContents();
    Close();
}

void SDL2RouteWindow::OnClose() {
    Close();
}
