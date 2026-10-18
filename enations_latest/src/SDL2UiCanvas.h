#pragma once
#include <SDL.h>
#include <SDL_ttf.h>

// Scoped, main-thread raster adapter. Layout and widget state remain in baseline
// units; only paint calls targeting this canvas are converted. Text is rasterized
// at output resolution, never enlarged from a baseline glyph bitmap.
namespace EnUiCanvas {
struct State { SDL_Surface* target; double scale; int originX, originY; };
class Scope {
    State previous;
public:
    Scope(SDL_Surface* target, double scale, int originX = 0, int originY = 0);
    ~Scope();
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};
double Scale();
int RasterSize(int baselineSize);
TTF_Font* Font(TTF_Font* font);
void ForgetFont(TTF_Font* font);
int Width(SDL_Surface* surface);
int Height(SDL_Surface* surface);
int FillRect(SDL_Surface* dst, const SDL_Rect* rect, Uint32 color);
int BlitSurface(SDL_Surface* src, const SDL_Rect* srcRect, SDL_Surface* dst, SDL_Rect* dstRect);
int BlitScaled(SDL_Surface* src, const SDL_Rect* srcRect, SDL_Surface* dst, SDL_Rect* dstRect);
// Artwork (sprites, icons, button faces): nearest at integer scales, bilinear at fractional ones.
int BlitArt(SDL_Surface* src, const SDL_Rect* srcRect, SDL_Surface* dst, SDL_Rect* dstRect);
SDL_bool SetClipRect(SDL_Surface* dst, const SDL_Rect* rect);
void GetClipRect(SDL_Surface* dst, SDL_Rect* rect);
void FreeSurface(SDL_Surface* surface);
int SizeText(TTF_Font* font, const char* text, int* w, int* h);
int SizeUTF8(TTF_Font* font, const char* text, int* w, int* h);
int FontHeight(TTF_Font* font);
int FontLineSkip(TTF_Font* font);
SDL_Surface* Text(TTF_Font* font, const char* text, SDL_Color color);
SDL_Surface* Wrapped(TTF_Font* font, const char* text, SDL_Color color, Uint32 width);
}
