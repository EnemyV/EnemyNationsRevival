#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <functional>
#include <unordered_map>

class SDL2Panel;
class GameWindow;

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
    IconData m_iconData[7];  // ICON_RESEARCH(0)..ICON_BAR_TEXT(6)
    int m_animFrame = 0;     // Animation counter

    // Status text
    std::string m_statusText[2];
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
