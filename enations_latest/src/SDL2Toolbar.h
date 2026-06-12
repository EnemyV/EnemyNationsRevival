#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <functional>
#include <unordered_map>

class SDL2Panel;
class GameWindow;
class CUnit;
class CVehicle;

// Native SDL2 toolbar replacing CWndBar's PrintWindow capture.
// Renders directly to a panel surface each frame.
//
// Layout (66px total height):
//   Row 1 (38px): 8 buttons | 4 resource bars | clock
//   Row 2 (28px): 2 status text lines

class SDL2Toolbar {
public:
    SDL2Toolbar();
    ~SDL2Toolbar();

    // Initialize with the compositor panel. Call after game windows are created.
    void Init(SDL2Panel* panel, GameWindow* gw);

    // Render all toolbar content to the panel surface. Called each frame.
    void Render();

    // Handle SDL events routed to the toolbar panel.
    bool HandleEvent(SDL_Event& event, int localX, int localY);

    // Set status text (called by game code)
    void SetStatusText(int line, const std::string& text, int importance = 0);

    // Set a unit whose status (damage / materials / construction) should be
    // drawn as ICON BARS on the given status line, matching the original
    // _UnitShowStatus. Passing nullptr reverts the line to plain text.
    // Mutually exclusive with SetStatusText for the same line.
    void SetUnitStatus(int line, CUnit* pUnit);

    // Enable/disable a button by index
    void EnableButton(int index, bool enabled);

    // Button handlers (called on click)
    using ButtonHandler = std::function<void()>;
    void SetButtonHandler(int index, ButtonHandler handler);

    // Optional descriptive hover text (status line 1). When empty the button's
    // short label is used instead. Populated by CWndBar from the IDH_BAR_*
    // string resources so we show the MFC-original sentence-length help.
    void SetButtonHelpText(int index, const std::string& text);

    static const int NUM_BUTTONS = 8;
    static const int NUM_STATS = 4;
    static const int TOOLBAR_HT = 66;
    static const int BTN_ROW_HT = 38;
    static const int TEXT_ROW_HT = 28;

private:
    TTF_Font* GetFont(int size);
    void RenderButton(SDL_Surface* dst, int idx, int x, int y, int w, int h);
    void RenderStatBar(SDL_Surface* dst, int idx, int x, int y, int w, int h);
    void RenderClock(SDL_Surface* dst, int x, int y, int w, int h);
    void RenderTextLine(SDL_Surface* dst, int line, int x, int y, int w, int h);

    // Icon-based unit status (mirrors _UnitShowStatus / CStatInst). Used to
    // draw a hovered building's / vehicle's resources as icons on a status line.
    void RenderUnitStatus(SDL_Surface* dst, CUnit* pUnit, int x, int y, int w, int h);
    int  GetNumStatusBars(CUnit* pUnit);
    void RenderStatusBars(SDL_Surface* dst, CUnit* pUnit, int x, int y, int w, int numBars);
    void Render3PieceBg(SDL_Surface* dst, int iconIdx, int x, int y, int w);
    void RenderIconBar(SDL_Surface* dst, int iconIdx, int percent, int x, int y, int w, int h);
    void RenderIconDone(SDL_Surface* dst, int iconIdx, int percent, int x, int y, int w, int h);
    void RenderIconText(SDL_Surface* dst, int iconIdx, const char* text, int x, int y, int w, int h);
    void RenderMaterialsBar(SDL_Surface* dst, CUnit* pUnit, int iconIdx, int x, int y, int w, int h);
    void RenderCarrierCargo(SDL_Surface* dst, CVehicle* pVeh, int iconIdx, int x, int y, int w, int h);

    SDL2Panel*   m_panel = nullptr;
    GameWindow*  m_gw = nullptr;

    // Button state
    struct ButtonState {
        std::string label;
        std::string helpText;  // Hover text (status line 1) — falls back to label if empty
        bool enabled = true;
        bool pressed = false;
        ButtonHandler handler;
        int spriteIndex = 0;       // Index into button sprite sheet
        SDL_Surface* sprites = nullptr;  // 3-frame sprite strip (owned)
    };
    ButtonState m_buttons[NUM_BUTTONS];

    // Background tile surface (from DIB_TOOLBAR)
    SDL_Surface* m_bgTile = nullptr;

    // Master button sprite sheet (from theBmBtnData.m_pcDib)
    SDL_Surface* m_btnSheet = nullptr;

    // Icon sprite sheets for status bars (from theIcons)
    struct IconData {
        SDL_Surface* sheet = nullptr;   // Full sprite sheet for this icon type
        int cxIcon = 0, cyIcon = 0;     // Icon sprite dimensions
        int cxLeft = 0, cxBack = 0, cxRight = 0, cyBack = 0;  // Background piece dims
        int leftOff = 0, rightOff = 0;  // Padding
        int typIcon = 0;                // Icon type (done, have_all, text, etc.)
        int typBack = 0;                // Background type
        int nNeedIcon = 0;              // Animation frame count
    };
    IconData m_iconData[15]; // ICON_RESEARCH(0)..ICON_VEHICLES(14)
    int m_animFrame = 0;     // Animation counter
    int m_statBarHt = 0;     // status-bar row height (ICON_DAMAGE cyBack)

    // Unit whose status is drawn as icon bars on each line (nullptr = text).
    // Set by SetUnitStatus when the cursor hovers a unit on the map; cleared
    // by SetStatusText / SetUnitStatus(nullptr). The ClearStatusFunc()
    // death-path contract does NOT always hold (live AV 2026-06-11: cursor
    // parked over a unit that died in combat -> RenderUnitStatus deref'd the
    // freed CUnit), so Render() re-validates the pointer each frame via
    // _GetUnit(m_statusUnitId) and drops the line if the unit is gone.
    CUnit* m_statusUnit[2] = { nullptr, nullptr };
    DWORD  m_statusUnitId[2] = { 0, 0 };  // ID captured while alive, for re-validation

    // Status text.
    //   m_statusText[]    — what RenderTextLine actually draws each frame.
    //   m_externalText[]  — what callers wrote via SetStatusText(). The hover
    //                       poll inside Render() can override line 1 with a
    //                       toolbar-button hover string while the cursor is
    //                       on the toolbar, but as soon as the cursor leaves
    //                       we fall back to m_externalText[1] so area-map
    //                       hover info (unit / terrain descriptions sent by
    //                       CWndArea::OnMouseMove) stays visible.
    std::string m_statusText[2];
    std::string m_externalText[2];
    int         m_statusImportance[2] = {};

    // Font
    std::string m_fontPath;
    std::unordered_map<int, TTF_Font*> m_fontCache;

    int m_pressedBtn = -1;
    int m_btnSpriteW = 0;
    int m_btnSpriteH = 0;

    // Cached stat bar layout for hit-testing
    int m_statBarX = 0;
    int m_statBarW = 0;
};
