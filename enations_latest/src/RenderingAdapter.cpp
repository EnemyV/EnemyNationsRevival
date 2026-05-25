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

    if (dibWidth <= 0 || dibHeight <= 0)
        return false;

    // Phase 6 Stage 2: if the CDIB is SDL-backed, blit its surface directly.
    // This skips the per-frame SDL_CreateRGBSurfaceFrom wrap and the CDIBits
    // lock — SDL_BlitSurface locks the surface internally. All three size
    // branches preserved (exact-match, scaled, offset) — collapsing to a
    // single blit breaks differently-sized panels.
    SDL_Surface* sdlBacking = pDib->GetSDLSurface();
    if (sdlBacking) {
        SDL_Rect dstRect = { dstX, dstY, dibWidth, dibHeight };
        if (dibWidth == dst->w && dibHeight == dst->h && dstX == 0 && dstY == 0) {
            SDL_BlitSurface(sdlBacking, nullptr, dst, nullptr);
        } else if (dibWidth != dst->w - dstX || dibHeight != dst->h - dstY) {
            SDL_Rect fitRect = { dstX, dstY, dst->w - dstX, dst->h - dstY };
            SDL_BlitScaled(sdlBacking, nullptr, dst, &fitRect);
        } else {
            SDL_BlitSurface(sdlBacking, nullptr, dst, &dstRect);
        }
        return true;
    }

    // Fallback path: non-SDL-backed CDIB (DDraw etc.). Lock bits, wrap in a
    // throwaway SDL surface, blit, free. Live default until Stage 3 flips
    // CalcBltMethod; after Stage 3 + 4 this fallback should rarely run.
    int bytesPerPixel = pDib->GetBytesPerPixel();
    int pitch         = pDib->GetPitch();
    int bitsPerPixel  = pDib->GetBitsPerPixel();

    if (pitch <= 0)
        return false;

    CDIBits dibits = pDib->GetBits();
    BYTE* pDibPixels = (BYTE*)(dibits);
    if (!pDibPixels)
        return false;

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

    SDL_Rect dstRect = { dstX, dstY, dibWidth, dibHeight };

    if (dibWidth == dst->w && dibHeight == dst->h && dstX == 0 && dstY == 0) {
        SDL_BlitSurface(dibSurface, nullptr, dst, nullptr);
    } else if (dibWidth != dst->w - dstX || dibHeight != dst->h - dstY) {
        SDL_Rect fitRect = { dstX, dstY, dst->w - dstX, dst->h - dstY };
        SDL_BlitScaled(dibSurface, nullptr, dst, &fitRect);
    } else {
        SDL_BlitSurface(dibSurface, nullptr, dst, &dstRect);
    }

    SDL_FreeSurface(dibSurface);
    return true;
}
