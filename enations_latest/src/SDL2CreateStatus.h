#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <unordered_map>

class GameWindow;

// Non-modal SDL2 replacement for CDlgCreateStatus.
// Displays "Creating world..." progress during world generation.
//
// Unlike SDL2Dialog (which runs its own modal event loop), this class
// is driven externally: CCreateBase calls SetMsg()/SetPer(), and
// Render() is called from PollEvents() each frame.
//
// Cancel is detected by checking WasCancelled() after BaseYield().

class SDL2CreateStatus {
public:
    SDL2CreateStatus(GameWindow* gameWindow);
    ~SDL2CreateStatus();

    // Show/hide the dialog
    void Show();
    void Hide();
    bool IsVisible() const { return m_visible; }

    // Update message text (e.g., "Creating terrain...")
    void SetMsg(const std::string& text);
    void SetMsg(int stringResourceID);

    // Update progress percentage (0-100, or -1 to reset)
    void SetPer(int percent);
    int  GetPer() const { return m_percent; }

    // Cancel support
    bool WasCancelled() const { return m_cancelled; }
    void ResetCancel() { m_cancelled = false; }

    // Render the dialog onto the window surface.
    // Called from GameWindow::PollEvents() each frame when visible.
    void Render();

    // Handle SDL events (for cancel button clicks).
    // Returns true if event was consumed.
    bool HandleEvent(const SDL_Event& event);

private:
    TTF_Font* GetFont(int size);
    void RenderProgressBar(SDL_Surface* dst, SDL_Rect rect, int percent);

    GameWindow* m_gameWindow;
    bool        m_visible;
    bool        m_cancelled;
    std::string m_message;
    int         m_percent;

    // Dialog size (the window is exactly this size, positioned centered on screen)
    int m_dlgW, m_dlgH;

    // Cancel button rect in own-window coords (always starts at 0,0)
    SDL_Rect m_btnCancelRect;
    bool     m_btnPressed;

    // Own ALWAYS_ON_TOP window — created in Show(), destroyed in Hide()
    SDL_Window* m_ownWindow = nullptr;

    // Font cache
    std::string m_fontPath;
    std::unordered_map<int, TTF_Font*> m_fontCache;
};
