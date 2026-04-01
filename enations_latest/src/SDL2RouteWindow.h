#pragma once

#include "SDL2Panel.h"
#include <string>
#include <vector>
#include <functional>

struct _TTF_Font;
typedef struct _TTF_Font TTF_Font;

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

private:
    void RebuildList();
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
