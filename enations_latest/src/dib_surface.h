#pragma once
#include <SDL.h>
#include <cstddef>
#include <cstring>
#include <limits>
#include <vector>

// Wrap native DIB bytes in visual top-down order. Bottom-up DIBs are copied to
// reusable storage because SDL surfaces cannot express a negative pitch. The
// caller retains both the DIB lock and scratch until the returned surface dies.
inline SDL_Surface* EnWrapNativeDib(const Uint8* pixels, int width, int height,
                                   int bpp, int pitch, bool topDown,
                                   const SDL_Color* palette, unsigned int paletteCount,
                                   unsigned int compression, size_t pixelBytes,
                                   std::vector<Uint8>& scratch)
{
    if (!pixels || width <= 0 || height <= 0 || pitch <= 0 ||
        compression != 0 ||
        (bpp != 8 && bpp != 15 && bpp != 16 && bpp != 24 && bpp != 32)) return nullptr;
    if (bpp == 8 && (!palette || paletteCount == 0 || paletteCount > 256)) return nullptr;
    const size_t bytesPerPixel = (bpp + 7) / 8;
    if ((size_t)width > (size_t)pitch / bytesPerPixel ||
        (size_t)height > (std::numeric_limits<size_t>::max)() / (size_t)pitch) return nullptr;
    const size_t requiredBytes = (size_t)height * (size_t)pitch;
    if (requiredBytes > pixelBytes || requiredBytes > (size_t)(std::numeric_limits<std::ptrdiff_t>::max)())
        return nullptr;
    const Uint8* surfacePixels = pixels;
    if (!topDown) {
        size_t bytes = requiredBytes;
        if (bytes > scratch.max_size()) return nullptr;
        try { scratch.resize(bytes); } catch (...) { return nullptr; }
        for (int y = 0; y < height; ++y)
            std::memcpy(scratch.data() + (size_t)y * pitch,
                        pixels + (size_t)(height - 1 - y) * pitch, (size_t)pitch);
        surfacePixels = scratch.data();
    }
    Uint32 r = 0, g = 0, b = 0;
    if (bpp == 24 || bpp == 32) { r = 0x00ff0000; g = 0x0000ff00; b = 0x000000ff; }
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(const_cast<Uint8*>(surfacePixels),
                                                    width, height, bpp, pitch, r, g, b, 0);
    if (!surface) return nullptr;
    if (bpp == 8) {
        // Undefined entries remain black; do not read past biClrUsed.
        SDL_Color complete[256] = {};
        for (unsigned int i = 0; i < 256; ++i) complete[i].a = 255;
        for (unsigned int i = 0; i < paletteCount; ++i) complete[i] = palette[i];
        if (!surface->format->palette || SDL_SetPaletteColors(surface->format->palette, complete, 0, 256) != 0) {
            SDL_FreeSurface(surface);
            return nullptr;
        }
    }
    return surface;
}
