#include "RenderingAdapter.h"
#include "GameWindow.h"
#include "SDL2Panel.h"
#include "base.h"  // For CAnimAtr and CDIBWnd

#include <SDL.h>
#include <cstring>
#include <fstream>

static void LogRender(const std::string& msg) {
    std::ofstream log("RenderingAdapter.log", std::ios::app);
    if (log.is_open()) {
        log << msg << std::endl;
    }
}

// Static member variables
GameWindow*    RenderingAdapter::s_gameWindow   = nullptr;
const CAnimAtr* RenderingAdapter::s_animAtr     = nullptr;
SDL2Panel*     RenderingAdapter::s_targetPanel  = nullptr;

void RenderingAdapter::Initialize(GameWindow* gameWindow) {
    s_gameWindow = gameWindow;
}

// --- Panel-based rendering ---

void RenderingAdapter::SetTargetPanel(SDL2Panel* panel) {
    s_targetPanel = panel;
}

bool RenderingAdapter::RenderToPanel(const CAnimAtr* aa) {
    if (!s_targetPanel)
        return false;
    return RenderToPanel(aa, s_targetPanel);
}

bool RenderingAdapter::RenderToPanel(const CAnimAtr* aa, SDL2Panel* panel) {
    if (!aa || !panel)
        return false;

    SDL_Surface* panelSurface = panel->GetSurface();
    if (!panelSurface)
        return false;

    if (!BlitDIBToSurface(aa, panelSurface, 0, 0))
        return false;

    panel->SetDirty();
    return true;
}

// --- Legacy whole-window rendering ---

void RenderingAdapter::SetAnimAtr(const CAnimAtr* aa) {
    s_animAtr = aa;
}

void RenderingAdapter::Render() {
    if (!s_gameWindow || !s_animAtr)
        return;

    SDL_Surface* windowSurface = SDL_GetWindowSurface(s_gameWindow->GetWindow());
    if (!windowSurface)
        return;

    if (BlitDIBToSurface(s_animAtr, windowSurface, 0, 0)) {
        SDL_UpdateWindowSurface(s_gameWindow->GetWindow());
    }
}

void RenderingAdapter::FlushRenderQueue() {
}

// --- Shared DIB-to-surface blitting ---

bool RenderingAdapter::BlitDIBToSurface(const CAnimAtr* aa, SDL_Surface* dst,
                                         int dstX, int dstY) {
    if (!aa || !dst)
        return false;

    const CDIBWnd& dibwnd = aa->m_dibwnd;
    CDIB* pDib = dibwnd.GetDIB();
    if (!pDib)
        return false;

    int dibWidth  = pDib->GetWidth();
    int dibHeight = pDib->GetHeight();
    int bytesPerPixel = pDib->GetBytesPerPixel();
    int pitch     = pDib->GetPitch();
    int bitsPerPixel  = pDib->GetBitsPerPixel();

    if (dibWidth <= 0 || dibHeight <= 0 || pitch <= 0)
        return false;

    // Lock the DIB bits — CDIBits is RAII; unlocks when it goes out of scope
    CDIBits dibits = pDib->GetBits();
    BYTE* pDibPixels = (BYTE*)(dibits);
    if (!pDibPixels)
        return false;

    // Create a temporary SDL surface wrapping the DIB pixel data
    SDL_Surface* dibSurface = nullptr;

    if (bytesPerPixel == 3 || bytesPerPixel == 4) {
        // Windows DIB: BGR(X) format
        Uint32 bmask = 0x000000FF;
        Uint32 gmask = 0x0000FF00;
        Uint32 rmask = 0x00FF0000;
        Uint32 amask = 0;  // No alpha — opaque
        dibSurface = SDL_CreateRGBSurfaceFrom(
            pDibPixels, dibWidth, dibHeight, bitsPerPixel, pitch,
            rmask, gmask, bmask, amask);
    } else {
        dibSurface = SDL_CreateRGBSurfaceFrom(
            pDibPixels, dibWidth, dibHeight, bitsPerPixel, pitch,
            0, 0, 0, 0);
    }

    if (!dibSurface)
        return false;

    // Blit to destination surface
    SDL_Rect dstRect = { dstX, dstY, dibWidth, dibHeight };

    // If DIB size matches destination, direct blit; otherwise scale
    if (dibWidth == dst->w && dibHeight == dst->h && dstX == 0 && dstY == 0) {
        SDL_BlitSurface(dibSurface, nullptr, dst, nullptr);
    } else if (dibWidth != dst->w - dstX || dibHeight != dst->h - dstY) {
        // Scale to fit the destination area (panel may be different size)
        SDL_Rect fitRect = { dstX, dstY, dst->w - dstX, dst->h - dstY };
        SDL_BlitScaled(dibSurface, nullptr, dst, &fitRect);
    } else {
        SDL_BlitSurface(dibSurface, nullptr, dst, &dstRect);
    }

    SDL_FreeSurface(dibSurface);
    return true;
}
