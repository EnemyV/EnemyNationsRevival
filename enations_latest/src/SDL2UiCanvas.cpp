#ifndef EN_DPI_STANDALONE_TEST
#include "stdafx.h"
#endif
#include "SDL2UiCanvas.h"
#include <unordered_map>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace EnUiCanvas {
namespace {
State s_state = { nullptr, 1, 0, 0 };
std::unordered_map<TTF_Font*, double> s_fonts;
std::unordered_map<SDL_Surface*, double> s_text;
int Edge(double x) { return (int)std::lround(x); }
bool Fractional(double scale) { return std::fabs(scale - std::round(scale)) > 1e-6; }

// Smoothly resampled artwork. Nearest-neighbour replication is exact at integer
// scales, but at 125/150/175% it gives every sprite alternating one- and two-pixel
// columns. At those scales 1:1 art is instead resampled bilinearly in premultiplied
// alpha (so the colour key never bleeds into edges). Results are cached by source
// content, so a sheet freed and reallocated at the same address cannot serve stale art.
struct ArtKey {
    SDL_Surface* src; SDL_Rect rect; int w, h; Uint64 hash;
    bool operator==(const ArtKey& o) const {
        return src == o.src && w == o.w && h == o.h && hash == o.hash && std::memcmp(&rect, &o.rect, sizeof rect) == 0;
    }
};
struct ArtKeyHash {
    size_t operator()(const ArtKey& k) const {
        return (size_t)(k.hash ^ ((Uint64)k.w << 40) ^ ((Uint64)k.h << 20) ^ (Uint64)k.rect.x ^ ((Uint64)k.rect.y << 10));
    }
};
std::unordered_map<ArtKey, SDL_Surface*, ArtKeyHash> s_art;
size_t s_artBytes = 0;
constexpr size_t ART_CACHE_LIMIT = 32u << 20;

Uint64 HashPixels(SDL_Surface* s, const SDL_Rect& r) {
    Uint64 h = 1469598103934665603ull;
    if (SDL_MUSTLOCK(s) && SDL_LockSurface(s) != 0) return h;
    const int bpp = s->format->BytesPerPixel;
    for (int y = r.y; y < r.y + r.h; ++y) {
        const Uint8* row = (const Uint8*)s->pixels + (size_t)y * s->pitch + (size_t)r.x * bpp;
        for (int i = 0, n = r.w * bpp; i < n; ++i) { h ^= row[i]; h *= 1099511628211ull; }
    }
    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);
    return h;
}

// Copy the source rect to a straight-alpha ARGB8888 surface (keyed pixels stay
// transparent black) and premultiply it, ready for interpolation.
SDL_Surface* Premultiplied(SDL_Surface* src, SDL_Rect r) {
    SDL_Surface* crop = SDL_CreateRGBSurfaceWithFormat(0, r.w, r.h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!crop) return nullptr;
    SDL_BlendMode mode = SDL_BLENDMODE_NONE; SDL_GetSurfaceBlendMode(src, &mode);
    SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE);
    const int copied = SDL_BlitSurface(src, &r, crop, nullptr);
    SDL_SetSurfaceBlendMode(src, mode);
    if (copied != 0) { SDL_FreeSurface(crop); return nullptr; }
    for (int y = 0; y < crop->h; ++y) {
        Uint8* p = (Uint8*)crop->pixels + (size_t)y * crop->pitch;
        for (int x = 0; x < crop->w; ++x, p += 4) {
            const unsigned a = p[3];
            if (a == 255) continue;
            p[0] = (Uint8)((p[0] * a + 127) / 255); p[1] = (Uint8)((p[1] * a + 127) / 255); p[2] = (Uint8)((p[2] * a + 127) / 255);
        }
    }
    return crop;
}

SDL_Surface* Resample(SDL_Surface* pre, int w, int h) {
    SDL_Surface* out = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!out) return nullptr;
    const int sw = pre->w, sh = pre->h;
    for (int oy = 0; oy < h; ++oy) {
        double sy = std::max(0.0, std::min((double)(sh - 1), (oy + 0.5) * sh / h - 0.5));
        const int y0 = (int)sy, y1 = std::min(y0 + 1, sh - 1); const double fy = sy - y0;
        const Uint8* r0 = (const Uint8*)pre->pixels + (size_t)y0 * pre->pitch;
        const Uint8* r1 = (const Uint8*)pre->pixels + (size_t)y1 * pre->pitch;
        Uint8* o = (Uint8*)out->pixels + (size_t)oy * out->pitch;
        for (int ox = 0; ox < w; ++ox, o += 4) {
            double sx = std::max(0.0, std::min((double)(sw - 1), (ox + 0.5) * sw / w - 0.5));
            const int x0 = (int)sx, x1 = std::min(x0 + 1, sw - 1); const double fx = sx - x0;
            double c[4];
            for (int i = 0; i < 4; ++i) {
                const double top = r0[x0 * 4 + i] * (1 - fx) + r0[x1 * 4 + i] * fx;
                const double bottom = r1[x0 * 4 + i] * (1 - fx) + r1[x1 * 4 + i] * fx;
                c[i] = top * (1 - fy) + bottom * fy;
            }
            const double a = c[3];
            o[3] = (Uint8)std::lround(a);
            for (int i = 0; i < 3; ++i) o[i] = a > 0.5 ? (Uint8)std::min<long>(255, std::lround(c[i] * 255 / a)) : 0;
        }
    }
    SDL_SetSurfaceBlendMode(out, SDL_BLENDMODE_BLEND);
    return out;
}

SDL_Surface* SmoothArt(SDL_Surface* src, SDL_Rect source, int w, int h) {
    const ArtKey key = { src, source, w, h, HashPixels(src, source) };
    auto it = s_art.find(key);
    if (it != s_art.end()) return it->second;
    SDL_Surface* pre = Premultiplied(src, source);
    SDL_Surface* out = pre ? Resample(pre, w, h) : nullptr;
    SDL_FreeSurface(pre);
    if (!out) return nullptr;
    if (s_artBytes + (size_t)out->pitch * out->h > ART_CACHE_LIMIT) {
        for (auto& e : s_art) SDL_FreeSurface(e.second);
        s_art.clear(); s_artBytes = 0;
    }
    s_art[key] = out; s_artBytes += (size_t)out->pitch * out->h;
    return out;
}

void ForgetArt(SDL_Surface* src) {
    for (auto it = s_art.begin(); it != s_art.end();) {
        if (it->first.src == src) { s_artBytes -= (size_t)it->second->pitch * it->second->h; SDL_FreeSurface(it->second); it = s_art.erase(it); }
        else ++it;
    }
}
double FontScale(TTF_Font* f) { auto it = s_fonts.find(f); return it == s_fonts.end() ? 1 : it->second; }
double SourceScale(SDL_Surface* s) { auto it = s_text.find(s); return it == s_text.end() ? 1 : it->second; }
SDL_Rect ScaleRect(SDL_Rect r, double scale, int ox = 0, int oy = 0) {
    const int x = Edge((r.x - ox) * scale), y = Edge((r.y - oy) * scale);
    return { x, y, Edge((r.x + r.w - ox) * scale) - x, Edge((r.y + r.h - oy) * scale) - y };
}
SDL_Rect Destination(SDL_Rect r) { return ScaleRect(r, s_state.scale, s_state.originX, s_state.originY); }
SDL_Surface* Register(SDL_Surface* s, TTF_Font* f) { if (s) s_text[s] = FontScale(f); return s; }
}
Scope::Scope(SDL_Surface* target, double scale, int originX, int originY) : previous(s_state) {
    s_state = { target, scale, originX, originY };
}
Scope::~Scope() { s_state = previous; }
double Scale() { return s_state.scale; }
int RasterSize(int n) { return std::max(1, std::min(512, Edge(n * Scale()))); }
TTF_Font* Font(TTF_Font* f) { if (f) s_fonts[f] = Scale(); return f; }
void ForgetFont(TTF_Font* f) { s_fonts.erase(f); }
int Width(SDL_Surface* s) { return s ? Edge(s->w / SourceScale(s)) : 0; }
int Height(SDL_Surface* s) { return s ? Edge(s->h / SourceScale(s)) : 0; }
int FillRect(SDL_Surface* dst, const SDL_Rect* rect, Uint32 color) {
    if (dst != s_state.target || !rect) return SDL_FillRect(dst, rect, color);
    SDL_Rect r = Destination(*rect); return SDL_FillRect(dst, &r, color);
}
int BlitScaled(SDL_Surface* src, const SDL_Rect* sr, SDL_Surface* dst, SDL_Rect* dr) {
    if (!src || !dst) return SDL_SetError("UI blit requires source and destination surfaces");
    SDL_Rect source = sr ? ScaleRect(*sr, SourceScale(src)) : SDL_Rect{0, 0, src->w, src->h};
    SDL_Rect dest = dr ? *dr : SDL_Rect{0, 0, Width(src), Height(src)};
    if (dst == s_state.target) dest = Destination(dest);
    return SDL_BlitScaled(src, &source, dst, &dest);
}
int BlitArt(SDL_Surface* src, const SDL_Rect* sr, SDL_Surface* dst, SDL_Rect* dr) {
    if (!src || !dst) return SDL_SetError("UI blit requires source and destination surfaces");
    if (dst != s_state.target || !Fractional(Scale()) || SourceScale(src) != 1) return BlitScaled(src, sr, dst, dr);
    SDL_Rect source = sr ? *sr : SDL_Rect{0, 0, src->w, src->h};
    SDL_Rect bounds = {0, 0, src->w, src->h};
    if (!SDL_IntersectRect(&source, &bounds, &source)) return 0;
    SDL_Rect dest = Destination(dr ? *dr : SDL_Rect{0, 0, source.w, source.h});
    if (dest.w <= 0 || dest.h <= 0) return 0;
    if (dest.w == source.w && dest.h == source.h) return SDL_BlitSurface(src, &source, dst, &dest);
    SDL_Surface* smooth = SmoothArt(src, source, dest.w, dest.h);
    if (!smooth) return SDL_BlitScaled(src, &source, dst, &dest);
    return SDL_BlitSurface(smooth, nullptr, dst, &dest);
}
int BlitSurface(SDL_Surface* src, const SDL_Rect* sr, SDL_Surface* dst, SDL_Rect* dr) {
    if (dst != s_state.target && SourceScale(src) == 1) return SDL_BlitSurface(src, sr, dst, dr);
    SDL_Rect dest = dr ? *dr : SDL_Rect{0, 0, 0, 0};
    dest.w = sr ? sr->w : Width(src); dest.h = sr ? sr->h : Height(src);
    return BlitArt(src, sr, dst, &dest);
}
SDL_bool SetClipRect(SDL_Surface* dst, const SDL_Rect* rect) {
    if (dst != s_state.target || !rect) return SDL_SetClipRect(dst, rect);
    SDL_Rect r = Destination(*rect); return SDL_SetClipRect(dst, &r);
}
void GetClipRect(SDL_Surface* dst, SDL_Rect* rect) {
    SDL_GetClipRect(dst, rect);
    if (dst == s_state.target) {
        const int right = Edge((rect->x + rect->w) / Scale()), bottom = Edge((rect->y + rect->h) / Scale());
        rect->x = Edge(rect->x / Scale()); rect->y = Edge(rect->y / Scale());
        rect->w = right - rect->x; rect->h = bottom - rect->y;
        rect->x += s_state.originX; rect->y += s_state.originY;
    }
}
void FreeSurface(SDL_Surface* s) { s_text.erase(s); ForgetArt(s); SDL_FreeSurface(s); }
int SizeUTF8(TTF_Font* f, const char* t, int* w, int* h) {
    int rw = 0, rh = 0; int result = TTF_SizeUTF8(f, t, &rw, &rh);
    if (w) *w = Edge(rw / FontScale(f)); if (h) *h = Edge(rh / FontScale(f)); return result;
}
int SizeText(TTF_Font* f, const char* t, int* w, int* h) { return SizeUTF8(f, t, w, h); }
int FontHeight(TTF_Font* f) { return Edge(TTF_FontHeight(f) / FontScale(f)); }
int FontLineSkip(TTF_Font* f) { return Edge(TTF_FontLineSkip(f) / FontScale(f)); }
SDL_Surface* Text(TTF_Font* f, const char* t, SDL_Color c) { return Register(TTF_RenderUTF8_Blended(f, t, c), f); }
SDL_Surface* Wrapped(TTF_Font* f, const char* t, SDL_Color c, Uint32 w) {
    return Register(TTF_RenderUTF8_Blended_Wrapped(f, t, c, w ? std::max(1, Edge(w * FontScale(f))) : 0), f);
}
}
