#pragma once

#include <vector>

struct SDL_Surface;
union SDL_Event;
class SDL2Panel;
class GameWindow;
class CWnd;

// Helper that wraps any MFC CWnd as an SDL2 panel.
// Captures the MFC window's GDI content via PrintWindow each frame
// and routes SDL mouse/keyboard events back to the MFC window.

class SDL2MFCPanel {
public:
    // Attach an MFC window to an SDL panel. Makes the MFC window transparent
    // and creates a panel at the same screen position.
    static SDL2Panel* Attach(CWnd* pWnd, const char* name, int zOrder);

    // Detach: remove the panel and restore the MFC window.
    static void Detach(CWnd* pWnd);

    // Capture all attached MFC windows to their panels.
    static void CaptureAll();

    // Route an SDL event to the MFC window behind a panel.
    static bool RouteEvent(CWnd* pWnd, SDL2Panel* panel,
                           SDL_Event& event, int localX, int localY);

    struct Entry {
        CWnd*      pWnd;
        SDL2Panel* panel;
    };

private:
    static std::vector<Entry>& GetEntries();
    static void CaptureOne(CWnd* pWnd, SDL2Panel* panel);
};
