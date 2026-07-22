#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <functional>
#include <unordered_map>

class SDL2Panel;
class GameWindow;
class CWndArea;
class CUnit;
class CVehicle;
class CBuilding;

// Native SDL2 area button bar replacing CWndAreaStatic's PrintWindow capture.
// Shows zoom, rotate, and context-dependent unit command buttons.

class SDL2AreaBar {
public:
    SDL2AreaBar();
    ~SDL2AreaBar();

    void Init(SDL2Panel* panel, CWndArea* pArea, HWND hStaticWnd);
    void Render();
    bool HandleEvent(SDL_Event& event, int localX, int localY);

    // Update button visibility/enabled state based on selected units
    void UpdateButtons();

    static const int NUM_BUTTONS = 17;
    static const int ORD_OFFSET = 6;
    static const int NUM_ORD = 11;

private:
    TTF_Font* GetFont(int size);
    void RenderButton(SDL_Surface* dst, int idx, int x, int y, int w, int h);

    // Selected-unit status readout in the bar's right region (name + damage +
    // materials/cargo icon bars). Faithful port of the MFC CWndUnitStat path
    // (_UnitShowStatus / CStatInst), reusing the same approach the SDL2 toolbar
    // uses for its hover readout. Data source is CWndArea::GetStaticUnit().
    void RenderUnitStatus(SDL_Surface* dst, CUnit* pUnit, int x, int y, int w, int h);
    int  GetNumStatusBars(CUnit* pUnit);
    void RenderStatusBars(SDL_Surface* dst, CUnit* pUnit, int x, int y, int w, int numBars);
    void Render3PieceBg(SDL_Surface* dst, int iconIdx, int x, int y, int w);
    void RenderIconBar(SDL_Surface* dst, int iconIdx, int percent, int x, int y, int w, int h);
    void RenderIconDone(SDL_Surface* dst, int iconIdx, int percent, int x, int y, int w, int h);
    void RenderIconText(SDL_Surface* dst, int iconIdx, const char* text, int x, int y, int w, int h);
    void RenderMaterialsBar(SDL_Surface* dst, CUnit* pUnit, int iconIdx, int x, int y, int w, int h);
    void RenderCarrierCargo(SDL_Surface* dst, CVehicle* pVeh, int iconIdx, int x, int y, int w, int h);
    int  CountContainedUnits(CUnit* pUnit);
    void RenderContainedUnits(SDL_Surface* dst, CBuilding* pBldg, int iconIdx, int x, int y, int w, int h);

    SDL2Panel*  m_panel = nullptr;
    CWndArea*   m_pArea = nullptr;
    HWND        m_hStaticWnd = nullptr;  // HWND of CWndAreaStatic

    // Button sprite sheet (same as toolbar)
    SDL_Surface* m_btnSheet = nullptr;
    SDL_Surface* m_bgTile = nullptr;  // DIB_AREA_BAR background
    int m_btnW = 0, m_btnH = 0;

    // Icon sprite sheets for the unit-status bars (from theIcons). Same layout
    // as SDL2Toolbar::IconData — one entry per ICON_* index (0..14).
    struct IconData {
        SDL_Surface* sheet = nullptr;   // full sprite sheet for this icon type
        int cxIcon = 0, cyIcon = 0;     // icon sprite dimensions
        int cxLeft = 0, cxBack = 0, cxRight = 0, cyBack = 0;  // background piece dims
        int leftOff = 0, rightOff = 0;  // padding
        int typIcon = 0, typBack = 0;   // icon / background type
        int nNeedIcon = 0;              // animation frame count
    };
    IconData m_iconData[15];   // ICON_RESEARCH(0)..ICON_VEHICLES(14)
    int m_statBarHt = 0;       // status-bar row height (ICON_DAMAGE cyBack)

    struct BtnState {
        int spriteIndex;
        int cmdID;
        bool visible;
        bool enabled;
        bool pressed;
        bool advancePos;  // true = move to next column after this button
        std::string label;
        std::string helpText;   // IDH_* flyby help (shown in the bottom-right status line on hover)
    };
    BtnState m_btns[NUM_BUTTONS];

    // Layout: computed button positions
    struct BtnPos { int x, y, w, h; };
    BtnPos m_btnPos[NUM_BUTTONS];
    int m_totalW = 0;

    int m_pressedBtn = -1;
    int m_hoverBtn = -1;   // index under the cursor (flyby-help debounce; -1 = none)

    std::string m_fontPath;
    std::unordered_map<int, TTF_Font*> m_fontCache;
};
