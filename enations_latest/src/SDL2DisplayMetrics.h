#pragma once

#include <SDL.h>
#include <cmath>
#include <algorithm>

// All layouts use baseline units. Density describes SDL's coordinate system;
// pixelScale describes the size of the UI. Neither is a map zoom setting.
struct SDL2DisplayMetrics {
    int windowW = 0, windowH = 0, drawableW = 0, drawableH = 0;
    int display = -1;
    double densityX = 1, densityY = 1, pixelScale = 1;
    bool valid = false;

    static bool AllowedScale(int percent) {
        return percent == 0 || percent == 100 || percent == 125 ||
               percent == 150 || percent == 175 || percent == 200;
    }
    static double SelectScale(int percent, double desktopScale) {
        if (!AllowedScale(percent)) percent = 0;
        const double scale = percent ? percent / 100.0 : desktopScale;
        return std::isfinite(scale) && scale > 0 ? std::max(0.5, std::min(4.0, scale)) : 1.0;
    }
    double WindowScaleX() const { return pixelScale / densityX; }
    double WindowScaleY() const { return pixelScale / densityY; }
    int Pixels(double units) const { return (int)std::lround(units * pixelScale); }
    int WindowX(double units) const { return (int)std::lround(units * WindowScaleX()); }
    int WindowY(double units) const { return (int)std::lround(units * WindowScaleY()); }
    // Map a raster sample to the unit whose rounded left edge contains it.
    // This is the inverse of Rect(), including fractional-scale boundaries.
    static int InverseEdge(double pixel, double scale) {
        int unit = (int)std::floor(pixel / scale);
        while (std::lround((unit + 1) * scale) <= pixel) ++unit;
        while (std::lround(unit * scale) > pixel) --unit;
        return unit;
    }
    int LayoutX(double windowX) const { return InverseEdge(windowX * densityX, pixelScale); }
    int LayoutY(double windowY) const { return InverseEdge(windowY * densityY, pixelScale); }
    // Canvas origins are whole layout units. Use the same quantized placement
    // for painting and input when a dialog is centered in the main drawable.
    int CenteredOrigin(int drawableExtent, int layoutExtent) const {
        const int view = std::min(layoutExtent, (int)std::floor(drawableExtent / pixelScale));
        return (int)std::lround(((drawableExtent - Pixels(view)) / 2) / pixelScale);
    }
    SDL_Rect Rect(const SDL_Rect& r, int originX = 0, int originY = 0) const {
        const int x = Pixels(r.x - originX), y = Pixels(r.y - originY);
        return { x, y, Pixels(r.x + r.w - originX) - x,
                       Pixels(r.y + r.h - originY) - y };
    }
    bool Refresh(SDL_Window* window, SDL_Renderer* renderer = nullptr);
};

namespace EnDpi {
    void InitializeProcess();              // before the first HWND / SDL video init
    int Setting();                        // Game/UIScale; 0 means Auto
    void CommitSetting(int percent);      // main-thread, local profile only
    unsigned Generation();
    Uint32 EventWindowID(const SDL_Event& event);
    void ClampWindow(SDL_Window* window);  // keeps chrome reachable on small monitors
    bool IsWayland();
    Uint32 MouseButtons();
}
