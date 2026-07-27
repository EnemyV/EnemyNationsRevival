#pragma once

class GameWindow;
class CAnimAtr;
class SDL2Panel;

/**
 * RenderingAdapter - Bridges game DIB rendering to SDL2
 *
 * Phase 1 flow (panel-based):
 *   1. Game renders to CAnimAtr::m_dibwnd (DIB surface)
 *   2. SetTargetPanel() selects which SDL2Panel to render into
 *   3. RenderToPanel() copies DIB pixel data to the panel's surface
 *   4. SDL2Compositor composites all panels to the window surface
 *
 * Legacy flow (whole-window, for backward compat during transition):
 *   1. SetAnimAtr() + Render() copies DIB to the full window surface
 */
class RenderingAdapter {
public:
    static void Initialize(GameWindow* gameWindow);

    // --- Panel-based rendering (Phase 1+) ---

    // Set the target panel for subsequent RenderToPanel() calls.
    // Pass nullptr to clear the target.
    static void SetTargetPanel(SDL2Panel* panel);

    // Copy a CAnimAtr's DIB content to the current target panel.
    // The panel is marked dirty; the compositor will present it.
    // Returns false if no target panel is set or rendering fails.
    static bool RenderToPanel(const CAnimAtr* aa);

    // Copy a CAnimAtr's DIB to a specific panel (convenience overload).
    static bool RenderToPanel(const CAnimAtr* aa, SDL2Panel* panel);

    // --- Legacy whole-window rendering ---

    static void SetAnimAtr(const CAnimAtr* aa);
    static void Render();
    static void FlushRenderQueue();

private:
    // Shared helper: blit a CAnimAtr's terrain DIB (m_dibwnd) to an SDL surface.
    static bool BlitDIBToSurface(const CAnimAtr* aa, struct SDL_Surface* dst,
                                  int dstX = 0, int dstY = 0);
    // Core: blit one CDIB to an SDL surface; colorkey=true treats magenta
    // (index 253) as transparent — used for the T1 sprite layer over terrain.
    static bool BlitCDIBToSurface(class CDIB* pDib, struct SDL_Surface* dst,
                                  int dstX, int dstY, bool colorkey);

    static GameWindow*  s_gameWindow;
    static const CAnimAtr* s_animAtr;
    static SDL2Panel*   s_targetPanel;
};
