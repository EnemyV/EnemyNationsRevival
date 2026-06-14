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
    const char* fonts[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "C:\\Windows\\Fonts\\arial.ttf",
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
        m_panel->SetResizable(true);   // CWndRoute was resizable (WS_THICKFRAME, ui.h)
        m_panel->SetTitle("Vehicle Route");
        m_panel->SetVisible(false);
        m_panel->SetClosable(true);
        m_panel->SetCloseCallback([this]() { Close(); });

        m_panel->SetEventCallback(
            [this](SDL_Event& event, int lx, int ly) -> bool {
                return HandleEvent(event, lx, ly);
            });

        // Re-flow buttons + list rows when the user resizes the window.
        m_panel->SetResizeCallback([this](int, int) { Layout(); Render(); });

        // When dragged, repaint the background the window vacated — otherwise the
        // compositor's incremental redraw leaves a trail of the old positions (the
        // "snaking cascade"). Marks only the background dirty (not detached panels).
        m_panel->SetMoveCallback([this](int, int, int, int) {
            if (m_gameWindow)
                if (SDL2Compositor* c = m_gameWindow->GetCompositor())
                    c->MarkBackgroundDirty();
        });

        // Give the route window its OWN top-level OS window (same mechanism the Area Map
        // uses: SDL2Panel::Detach). A COMPOSITED panel lives inside the main game window and
        // therefore renders BEHIND the detached Area Map / Radar OS windows — that's why it
        // could never come to the front. Detached, it's its own always-raisable window.
        // RenderDetached blits our m_surface into the own window; the old "double-wrapped
        // frame" concern is handled in RenderBackground (we skip our own gold border when
        // detached, since the detached chrome draws the title bar + frame).
        m_panel->Detach( m_gameWindow );
        (void)areaPanel;
    }

    // Create buttons (label / enabled / onClick); their rects are positioned by Layout()
    // so they re-flow on resize. Row 1 = Waypoint/Unload/Load, Row 2 = Goto/Delete/Start/Auto/Close.
    bool isTransport = m_pVeh->GetData()->IsTransport() ? true : false;
    SDL_Rect z = { 0, 0, 0, 0 };
    m_buttons.push_back({ z, "Waypoint", true,        false, [this]() { OnWaypoint(); } });
    m_buttons.push_back({ z, "Unload",   isTransport, false, [this]() { OnUnload();   } });
    m_buttons.push_back({ z, "Load",     isTransport, false, [this]() { OnLoad();     } });
    m_buttons.push_back({ z, "Goto",     false,       false, [this]() { OnGoto();     } });
    m_buttons.push_back({ z, "Delete",   false,       false, [this]() { OnDelete();   } });
    m_buttons.push_back({ z, "Start",    true,        false, [this]() { OnStart();    } });
    m_buttons.push_back({ z, "Auto",     true,        false, [this]() { OnAuto();     } });
    m_buttons.push_back({ z, "Close",    true,        false, [this]() { OnClose();    } });

    Layout();
}

// (Re)position the two button rows and recompute how many list rows fit, for the CURRENT
// panel size. Called once at construction and again from the resize callback.
void SDL2RouteWindow::Layout() {
    int w = m_panel ? m_panel->GetWidth()  : PANEL_W;
    int h = m_panel ? m_panel->GetHeight() : PANEL_H;
    const int gap = 4;
    int btnY1 = h - BTN_H * 2 - gap * 3;
    int btnY2 = h - BTN_H - gap;
    int startX = LIST_MARGIN;

    const int btnW1 = 70;   // row 1: 3 wide buttons
    for (int i = 0; i < 3 && i < (int)m_buttons.size(); ++i)
        m_buttons[i].rect = { startX + i * (btnW1 + gap), btnY1, btnW1, BTN_H };

    const int btnW2 = 52;   // row 2: 5 narrower buttons
    for (int i = 3; i < 8 && i < (int)m_buttons.size(); ++i)
        m_buttons[i].rect = { startX + (i - 3) * (btnW2 + gap), btnY2, btnW2, BTN_H };

    m_visibleRows = (btnY1 - gap - LIST_Y) / ROW_H;
    if (m_visibleRows < 1) m_visibleRows = 1;
    (void)w;
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

    // Carved-gold frame (shared corner-correct routine — fills the corners). Skipped when
    // detached: SDL2Panel::RenderDetached already draws the title bar + carved-gold frame
    // around the whole OS window, so drawing our own here would double-wrap it.
    if ( !( m_panel && m_panel->IsDetached() ) )
        SDL2MainMenu::DrawGoldBorder(dst, 0, 0, w, h, m_borderH, m_borderV);
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
        // Raise above the other composited panels on open — the compositor only
        // brings a panel to front on a click, so a freshly shown window would
        // otherwise keep its add-order z and could sit behind another panel.
        SDL2Compositor* comp = m_gameWindow ? m_gameWindow->GetCompositor() : nullptr;
        if (comp)
            comp->BringToFront(m_panel);
    }
    RebuildList();
    Render();
}

void SDL2RouteWindow::Hide() {
    if (m_panel) m_panel->SetVisible(false);
}

void SDL2RouteWindow::Close() {
    m_pVeh->m_pSdlRoute = nullptr;

    // If any area window still has us in veh_route mode, take it out — this
    // resets m_iMode back to normal so the route cursor doesn't stay stuck on
    // after Start / Auto / Close. Mirrors CWndRoute::OnDestroy (unit_wnd.cpp).
    POSITION pos;
    for (pos = theAreaList.GetHeadPosition(); pos != NULL;) {
        CWndArea* pWndArea = theAreaList.GetNext(pos);
        if (pWndArea && pWndArea->GetUnit() == m_pVeh &&
            pWndArea->GetMode() == CWndArea::veh_route)
            pWndArea->SelectOff();
    }

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

    // F1: "Loop" checkbox in the top margin (above the list). Checked = the route loops
    // (legacy behaviour); unchecked = one-shot (stop at the last stop).
    {
        const int cbSz = 13, cbY = 5;
        m_loopRect = { insetX, cbY, cbSz, cbSz };
        SDL_FillRect(surf, &m_loopRect, SDL_MapRGB(surf->format, 30, 25, 18));
        Uint32 dk = SDL_MapRGB(surf->format, 60, 50, 35);
        Uint32 lt = SDL_MapRGB(surf->format, 190, 170, 120);
        SDL_Rect e;
        e = { m_loopRect.x, m_loopRect.y, cbSz, 1 };               SDL_FillRect(surf, &e, dk);
        e = { m_loopRect.x, m_loopRect.y, 1, cbSz };               SDL_FillRect(surf, &e, dk);
        e = { m_loopRect.x, m_loopRect.y + cbSz - 1, cbSz, 1 };    SDL_FillRect(surf, &e, lt);
        e = { m_loopRect.x + cbSz - 1, m_loopRect.y, 1, cbSz };    SDL_FillRect(surf, &e, lt);
        if (m_pVeh && m_pVeh->GetRouteLoop()) {
            SDL_Rect chk = { m_loopRect.x + 3, m_loopRect.y + 3, cbSz - 6, cbSz - 6 };
            SDL_FillRect(surf, &chk, SDL_MapRGB(surf->format, 255, 220, 100));
        }
        SDL_Surface* lbl = TTF_RenderText_Blended(font, "Loop", SDL_Color{ 220, 210, 180, 255 });
        if (lbl) {
            SDL_Rect d = { insetX + cbSz + 5, cbY - 1, 0, 0 };
            SDL_BlitSurface(lbl, NULL, surf, &d);
            SDL_FreeSurface(lbl);
        }
    }

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

    // Capture geometry for hit-testing (the inset is border-art-relative, not LIST_MARGIN)
    // and clamp the scroll offset to the current contents.
    m_listRect = { listX, LIST_Y, listW, listH };
    int entryCount = (int)m_entries.size();
    int maxScroll  = (entryCount > m_visibleRows) ? (entryCount - m_visibleRows) : 0;
    if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
    if (m_scrollOffset < 0)         m_scrollOffset = 0;

    // Draw entries
    for (int i = 0; i < m_visibleRows && (i + m_scrollOffset) < entryCount; i++) {
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

    // Scrollbar (interactive — see HandleEvent). Reserve a fixed column on the right;
    // m_listInnerW excludes it so row hit-testing doesn't overlap the bar.
    m_hasScrollbar = (entryCount > m_visibleRows);
    m_listInnerW   = m_hasScrollbar ? (listW - SB_COL_W) : listW;
    if (m_hasScrollbar) {
        int sbX = listX + listW - SB_COL_W;
        int sbH = listH * m_visibleRows / entryCount;
        if (sbH < 10) sbH = 10;
        int sbY = LIST_Y;
        if (maxScroll > 0)
            sbY = LIST_Y + (listH - sbH) * m_scrollOffset / maxScroll;
        m_sbThumb = { sbX, sbY, 6, sbH };
        SDL_Rect trough = { sbX, LIST_Y, SB_COL_W, listH };
        SDL_FillRect(surf, &trough,   SDL_MapRGB(surf->format, 50, 42, 28));
        SDL_FillRect(surf, &m_sbThumb, SDL_MapRGB(surf->format, 140, 120, 80));
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

    int maxScroll = (int)m_entries.size() - m_visibleRows;
    if (maxScroll < 0) maxScroll = 0;

    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                // F1: Loop checkbox (box + "Loop" label hit area).
                if (m_pVeh &&
                    localX >= m_loopRect.x && localX < m_loopRect.x + m_loopRect.w + 42 &&
                    localY >= m_loopRect.y && localY < m_loopRect.y + m_loopRect.h) {
                    m_pVeh->SetRouteLoop(!m_pVeh->GetRouteLoop());
                    Render();
                    return true;
                }

                // Scrollbar column: drag the thumb, or page on a trough click.
                if (m_hasScrollbar &&
                    localX >= m_listRect.x + m_listInnerW &&
                    localX <  m_listRect.x + m_listRect.w &&
                    localY >= m_listRect.y && localY < m_listRect.y + m_listRect.h) {
                    if (localY >= m_sbThumb.y && localY < m_sbThumb.y + m_sbThumb.h) {
                        m_sbDragging   = true;
                        m_sbDragOffset = localY - m_sbThumb.y;
                    } else {
                        m_scrollOffset += (localY < m_sbThumb.y) ? -m_visibleRows : m_visibleRows;
                        if (m_scrollOffset < 0)         m_scrollOffset = 0;
                        if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
                        Render();
                    }
                    return true;
                }

                // List rows: use the rendered list rect (minus the scrollbar column),
                // so the clickable area matches exactly what's drawn.
                if (localX >= m_listRect.x && localX < m_listRect.x + m_listInnerW &&
                    localY >= m_listRect.y && localY < m_listRect.y + m_listRect.h) {
                    int row = (localY - m_listRect.y) / ROW_H + m_scrollOffset;
                    if (row >= 0 && row < (int)m_entries.size()) {
                        m_selectedIndex = row;
                        if (m_buttons.size() > 4) {
                            m_buttons[3].enabled = true;   // Goto
                            m_buttons[4].enabled = true;   // Delete
                        }
                        Render();
                    }
                    return true;
                }

                // Buttons.
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
                m_sbDragging = false;
                for (auto& btn : m_buttons) {
                    if (btn.pressed) {
                        btn.pressed = false;
                        if (btn.enabled &&
                            localX >= btn.rect.x && localX < btn.rect.x + btn.rect.w &&
                            localY >= btn.rect.y && localY < btn.rect.y + btn.rect.h) {
                            if (btn.onClick) btn.onClick();   // NB: Close/Start/Auto delete `this`
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
            } else if (event.wheel.y < 0 && m_scrollOffset < maxScroll) {
                m_scrollOffset++;
                Render();
            }
            return true;

        case SDL_MOUSEMOTION:
            if (m_sbDragging && m_hasScrollbar) {
                int trackH = m_listRect.h - m_sbThumb.h;
                if (trackH > 0 && maxScroll > 0) {
                    int newY = localY - m_sbDragOffset;
                    m_scrollOffset = (newY - m_listRect.y) * maxScroll / trackH;
                    if (m_scrollOffset < 0)         m_scrollOffset = 0;
                    if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
                    Render();
                }
            }
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
