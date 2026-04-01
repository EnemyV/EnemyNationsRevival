#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <functional>
#include <unordered_map>

class SDL2Panel;
class GameWindow;
class CWndArea;

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

    SDL2Panel*  m_panel = nullptr;
    CWndArea*   m_pArea = nullptr;
    HWND        m_hStaticWnd = nullptr;  // HWND of CWndAreaStatic

    // Button sprite sheet (same as toolbar)
    SDL_Surface* m_btnSheet = nullptr;
    SDL_Surface* m_bgTile = nullptr;  // DIB_AREA_BAR background
    int m_btnW = 0, m_btnH = 0;

    struct BtnState {
        int spriteIndex;
        int cmdID;
        bool visible;
        bool enabled;
        bool pressed;
        bool advancePos;  // true = move to next column after this button
        std::string label;
    };
    BtnState m_btns[NUM_BUTTONS];

    // Layout: computed button positions
    struct BtnPos { int x, y, w, h; };
    BtnPos m_btnPos[NUM_BUTTONS];
    int m_totalW = 0;

    int m_pressedBtn = -1;

    std::string m_fontPath;
    std::unordered_map<int, TTF_Font*> m_fontCache;
};
