#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include "SDL2Scrollbar.h"   // EnSb:: shared scrollbar geometry/chrome/hit-test
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

class SDL2Panel;
class GameWindow;
class CUnit;
class CVehicle;
class CBuilding;

// Native SDL2 replacement for CWndListVehicles / CWndListBuildings.
// Renders a scrollable list of units with original Enemy Nations art:
//   - Background texture from DIB_LIST_UNIT_BACK (stretched, normal/selected states)
//   - Unit type icons from DIB_LIST_UNIT_VEHICLES / DIB_LIST_UNIT_BUILDINGS
//   - Status bars using original icon sprite sheets with 3-piece backgrounds
//   - Truck route/destination text + material cargo
//   - Crane construction/road progress
//   - Carrier unit cargo sprites

class SDL2UnitList {
public:
    enum ListType { VEHICLES, BUILDINGS };

    SDL2UnitList(ListType type);
    ~SDL2UnitList();

    void Init(SDL2Panel* panel, GameWindow* gw);
    void Render();
    bool HandleEvent(SDL_Event& event, int localX, int localY);

    // List management
    void Rebuild();  // Rebuild from game data
    void AddUnit(CUnit* pUnit);
    void RemoveUnit(CUnit* pUnit);

    static const int ITEM_HT = 64;
    static const int SB_WIDTH = 14;  // Scrollbar width
    // One scrollbar-arrow click, in pixels. This list scrolls by PIXELS (a row is
    // 64px tall), so a whole row per click would be a jump; 16px is a quarter row
    // and a third of a wheel notch, which is what makes press-and-hold read as a
    // smooth scroll rather than a series of hops.
    static const int SB_ARROW_STEP = 16;

private:
    TTF_Font* GetFont(int size);
    // Scrollbar geometry (track, thumb, and the arrow buttons at the ends), derived
    // from the live panel size + item count so Render() and HandleEvent() cannot
    // disagree. Shared with SDL2Listbox/SDL2RouteWindow via SDL2Scrollbar.h.
    EnSb::Metrics SbMetrics() const;
    // The ONE place m_scrollY moves: arrows, pages, wheel, drag and auto-repeat.
    void ScrollPixels(int dy);
    void RenderItem(SDL_Surface* dst, int idx, int x, int y, int w, bool selected);
    void RenderShadowText(SDL_Surface* dst, TTF_Font* font, const char* text,
                          int x, int y, int maxW, SDL_Color fg, SDL_Color shadow);

    // Status bar rendering (using original icon sprite system)
    int  GetNumStatusBars(CUnit* pUnit);
    void RenderStatusBars(SDL_Surface* dst, CUnit* pUnit, int x, int y, int w, int numBars);
    void Render3PieceBg(SDL_Surface* dst, int iconIdx, int x, int y, int w);
    void RenderIconDone(SDL_Surface* dst, int iconIdx, int percent, int x, int y, int w, int h);
    void RenderIconHave(SDL_Surface* dst, int iconIdx, int have, int need, int x, int y, int w, int h);
    void RenderIconText(SDL_Surface* dst, int iconIdx, const char* text, int x, int y, int w, int h);
    void RenderIconBar(SDL_Surface* dst, int iconIdx, int percent, int x, int y, int w, int h);
    void RenderCarrierCargo(SDL_Surface* dst, CVehicle* pVeh, int iconIdx, int x, int y, int w, int h);
    void RenderMaterialsBar(SDL_Surface* dst, CUnit* pUnit, int iconIdx, int x, int y, int w, int h);

    void OnClick(int itemIdx, bool dblClick);
    void Select(int itemIdx, bool ctrl, bool shift);   // extended list selection -> map
    CUnit* LiveUnit(int idx);                           // re-resolve row by ID (unit may have died)

    ListType     m_type;
    SDL2Panel*   m_panel = nullptr;
    GameWindow*  m_gw = nullptr;

    // Sprites
    SDL_Surface* m_unitSprites = nullptr;   // Vehicle or building type icons
    SDL_Surface* m_bgNormal = nullptr;      // List item background (normal) - stretched
    SDL_Surface* m_bgSelected = nullptr;    // List item background (selected) - stretched

    // Icon sprite data (from theIcons) for status bar rendering
    struct IconData {
        SDL_Surface* sheet = nullptr;
        int cxIcon = 0, cyIcon = 0;
        int cxLeft = 0, cxBack = 0, cxRight = 0, cyBack = 0;
        int leftOff = 0, rightOff = 0;
        int typIcon = 0;   // CStatData::TYP_ICON
        int typBack = 0;   // CStatData::TYP_BACK
        int nNeedIcon = 0;
    };
    IconData m_iconData[15];  // ICON_* indices (up to ICON_VEHICLES=14)

    // Items
    struct ListItem {
        CUnit* pUnit;
        DWORD  dwID;        // unit ID — re-resolved against the live map each draw
        std::string name;
        int typeIndex;      // Index into sprite sheet
    };
    std::vector<ListItem> m_items;
    int m_scrollY = 0;
    int m_selectedIdx = -1;
    int m_anchorIdx = -1;   // range-select anchor for Shift+click

    // Font
    std::string m_fontPath;
    std::unordered_map<int, TTF_Font*> m_fontCache;

    // Click tracking
    DWORD m_lastClickTime = 0;
    int m_lastClickIdx = -1;

    // Scrollbar drag state
    bool m_sbDragging = false;
    int  m_sbDragOffset = 0;
    EnSb::Repeat m_sbRepeat;   // arrow held down; ticked at the top of Render()

    // Status bar height (from ICON_DAMAGE cyBack)
    int m_statBarHt = 14;

    // Repaint throttle. Render() is called every game frame, but the list shows
    // slowly-changing live data, so redrawing it (and marking the panel dirty,
    // which forces a detached-window re-present) at 60fps just steals frames from
    // the game. We refresh on a fixed interval instead, and force an immediate
    // redraw on interaction so scrolling/selection still feels instant.
    DWORD m_lastDrawMs = 0;
    bool  m_forceDraw = true;
    static const DWORD DRAW_INTERVAL_MS = 142;  // ~7 fps (forced redraw on interaction keeps it responsive)
};
