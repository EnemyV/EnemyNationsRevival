#pragma once

#include "SDL2Panel.h"
#include "SDL2Scrollbar.h"   // EnSb:: shared scrollbar geometry/chrome/hit-test
#include <string>
#include <vector>
#include <functional>

// Include SDL_ttf for the TTF_Font type. (Older SDL_ttf used an opaque
// `struct _TTF_Font`; SDL_ttf 2.24+ renamed the tag to `TTF_Font`, so a manual
// forward declaration conflicts depending on the installed version.)
#include <SDL_ttf.h>

class CVehicle;
class CWndArea;
class GameWindow;
class SDL2Compositor;

// Native SDL2 replacement for CWndRoute.
// Non-modal panel that shows vehicle route waypoints with action buttons.

class SDL2RouteWindow {
public:
    SDL2RouteWindow(GameWindow* gw, CVehicle* pVeh, SDL2Panel* areaPanel = nullptr);
    ~SDL2RouteWindow();

    void Show();
    void Hide();
    void Close();
    bool IsVisible() const;

    // Called when route changes externally (waypoint added from map click)
    void RefreshRoute();

    // Render to the panel surface
    void Render();

    CVehicle* GetVehicle() const { return m_pVeh; }

    // Advance press-and-hold auto-repeat on the scrollbar arrows for every open
    // route window. Unlike SDL2Listbox (whose Render runs every frame) this window
    // only repaints on demand, so it has no per-frame hook of its own; GameWindow
    // calls this once per frame from PollEvents. No-op when nothing is held.
    static void TickScrollRepeats();

private:
    void RebuildList();
    void Layout();   // (re)position buttons + list rows for the current panel size
    // The ONE place m_scrollOffset moves: arrows, pages, wheel and auto-repeat all
    // call this, so the clamp and the repaint cannot be forgotten at one site.
    void ScrollRows(int rows);
    bool HandleEvent(SDL_Event& event, int localX, int localY);
    TTF_Font* GetFont(int size);

    // Button actions
    void OnWaypoint();
    void OnLoad();
    void OnUnload();
    void OnGoto();
    void OnDelete();
    void OnStart();
    void OnAuto();
    void OnClose();

    struct RouteEntry {
        std::string text;
        bool isCurrent = false;
    };

    struct Button {
        SDL_Rect rect;
        std::string label;
        bool enabled = true;
        bool pressed = false;
        std::function<void()> onClick;
    };

    void LoadArt();
    void RenderBackground(SDL_Surface* dst);

    GameWindow* m_gameWindow;
    CVehicle* m_pVeh;
    SDL2Panel* m_panel = nullptr;
    std::string m_fontPath;
    TTF_Font* m_fontSmall = nullptr;

    std::vector<RouteEntry> m_entries;
    std::vector<Button> m_buttons;
    int m_selectedIndex = -1;
    int m_scrollOffset = 0;
    int m_visibleRows = 8;

    // Geometry captured by Render() so HandleEvent hit-tests exactly match what's drawn
    // (the list inset is border-art-relative, not the fixed LIST_MARGIN).
    SDL_Rect m_listRect    = { 0, 0, 0, 0 };  // drawn list area (full width)
    int      m_listInnerW  = 0;               // list width excluding the scrollbar column
    bool     m_hasScrollbar = false;
    // Scrollbar geometry (track, thumb, and the up/down arrow buttons at the ends),
    // captured by Render() from the SAME EnSb::Layout call that paints it so the
    // hit-test cannot disagree with the chrome. Shared with SDL2Listbox/SDL2UnitList.
    EnSb::Metrics m_sbMetrics;
    EnSb::Repeat  m_sbRepeat;                 // arrow held down; ticked per frame
    bool     m_sbDragging  = false;
    int      m_sbDragOffset = 0;
    // Scrollbar column width. Was 8 (with a 6px thumb painted inside it); the arrow
    // buttons are squares as wide as the column, and an 8px arrow is too small to
    // hit or to read, so the gutter is 12 like the dialog list boxes. Only the
    // gutter widened - rows, row height and every button keep their geometry.
    static const int SB_COL_W = 12;
    SDL_Rect m_loopRect    = { 0, 0, 0, 0 };  // "Loop" checkbox hit-rect (F1)

    // Game art surfaces
    SDL_Surface* m_bgGold = nullptr;
    SDL_Surface* m_borderH = nullptr;
    SDL_Surface* m_borderV = nullptr;
    bool m_artLoaded = false;

    static const int PANEL_W = 320;
    static const int PANEL_H = 300;
    static const int BTN_H = 26;
    static const int ROW_H = 18;
    static const int LIST_Y = 24;
    static const int LIST_MARGIN = 8;
};
