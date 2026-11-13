#ifdef NDEBUG
#undef NDEBUG
#endif
#define SDL_MAIN_HANDLED
#include "../enations_latest/src/dib_surface.h"
#include <cassert>

static void color(SDL_Surface* s, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    const Uint32 p = *(const Uint32*)((const Uint8*)s->pixels + y * s->pitch + x * 4);
    Uint8 ar, ag, ab, aa;
    SDL_GetRGBA(p, s->format, &ar, &ag, &ab, &aa);
    assert(ar == r && ag == g && ab == b);
}
int main()
{
    assert(SDL_Init(0) == 0);
    std::vector<Uint8> scratch;
    SDL_Color palette[256] = {};
    palette[1] = {255, 0, 0, 255}; palette[2] = {0, 255, 0, 255};
    palette[3] = {0, 0, 255, 255}; palette[4] = {255, 255, 0, 255};
    // Two padded rows: the bottom row is first in native storage.
    Uint8 bottom8[] = {3, 4, 0, 0, 1, 2, 0, 0};
    SDL_Surface* src = EnWrapNativeDib(bottom8, 2, 2, 8, 4, false, palette, 256, 0, sizeof(bottom8), scratch);
    SDL_Surface* out = SDL_CreateRGBSurfaceWithFormat(0, 2, 2, 32, SDL_PIXELFORMAT_ARGB8888);
    assert(src && out && SDL_BlitSurface(src, nullptr, out, nullptr) == 0);
    color(out, 0, 0, 255, 0, 0); color(out, 1, 0, 0, 255, 0);
    color(out, 0, 1, 0, 0, 255); color(out, 1, 1, 255, 255, 0);
    SDL_FreeSurface(src);
    Uint8 top8[] = {1, 2, 0, 0, 3, 4, 0, 0};
    src = EnWrapNativeDib(top8, 2, 2, 8, 4, true, palette, 256, 0, sizeof(top8), scratch);
    assert(src && SDL_BlitSurface(src, nullptr, out, nullptr) == 0);
    color(out, 0, 0, 255, 0, 0); color(out, 1, 1, 255, 255, 0);
    SDL_FreeSurface(src);
    // 24-bit BGR rows, bottom-up, with two padding bytes per row.
    Uint8 bottom24[] = {255,0,0, 0,255,255, 0,0, 0,0,255, 0,255,0, 0,0};
    src = EnWrapNativeDib(bottom24, 2, 2, 24, 8, false, nullptr, 0, 0, sizeof(bottom24), scratch);
    assert(src && SDL_BlitSurface(src, nullptr, out, nullptr) == 0);
    color(out, 0, 0, 255, 0, 0); color(out, 1, 0, 0, 255, 0);
    color(out, 0, 1, 0, 0, 255); color(out, 1, 1, 255, 255, 0);
    SDL_FreeSurface(src);
    // Partial palette: only declared colors are read; the rest are black.
    Uint8 partial[] = {1, 3, 0, 0};
    src = EnWrapNativeDib(partial, 2, 1, 8, 4, true, palette, 2, 0, sizeof(partial), scratch);
    assert(src && SDL_BlitSurface(src, nullptr, out, nullptr) == 0);
    color(out, 0, 0, 255, 0, 0); color(out, 1, 0, 0, 0, 0);
    SDL_FreeSurface(src);
    assert(!EnWrapNativeDib(bottom8, 2, 2, 8, 1, false, palette, 256, 0, sizeof(bottom8), scratch));
    assert(!EnWrapNativeDib(bottom8, 2, 2, 8, 4, false, nullptr, 0, 0, sizeof(bottom8), scratch));
    assert(!EnWrapNativeDib(bottom8, 2, 2, 8, 4, false, palette, 257, 0, sizeof(bottom8), scratch));
    assert(!EnWrapNativeDib(bottom8, 2, 2, 8, 4, false, palette, 256, 1, sizeof(bottom8), scratch));
    assert(!EnWrapNativeDib(bottom8, 2, 2, 1, 4, false, palette, 2, 0, sizeof(bottom8), scratch));
    assert(!EnWrapNativeDib(bottom8, 2, 2, 4, 4, false, palette, 16, 0, sizeof(bottom8), scratch));
    assert(!EnWrapNativeDib(bottom8, 2, 2, 8, 4, false, palette, 256, 0, 7, scratch));
    assert(!EnWrapNativeDib(bottom8, 1, 2147483647, 8, 4, true, palette, 256, 0, sizeof(bottom8), scratch));
    SDL_FreeSurface(out); SDL_Quit();
}
