#ifndef EN_DPI_STANDALONE_TEST
#include "stdafx.h"
#endif
#include "SDL2UiCanvas.h"
#include <unordered_map>
#include <cmath>
#include <algorithm>

namespace EnUiCanvas {
namespace {
State s_state = { nullptr, 1, 0, 0 };
std::unordered_map<TTF_Font*, double> s_fonts;
std::unordered_map<SDL_Surface*, double> s_text;
int Edge(double x) { return (int)std::lround(x); }
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
int BlitSurface(SDL_Surface* src, const SDL_Rect* sr, SDL_Surface* dst, SDL_Rect* dr) {
    if (dst != s_state.target && SourceScale(src) == 1) return SDL_BlitSurface(src, sr, dst, dr);
    SDL_Rect dest = dr ? *dr : SDL_Rect{0, 0, 0, 0};
    dest.w = sr ? sr->w : Width(src); dest.h = sr ? sr->h : Height(src);
    return BlitScaled(src, sr, dst, &dest);
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
void FreeSurface(SDL_Surface* s) { s_text.erase(s); SDL_FreeSurface(s); }
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
