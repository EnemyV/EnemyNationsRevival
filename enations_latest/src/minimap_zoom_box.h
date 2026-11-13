#pragma once
#include <cmath>

// Pixel span of the minimap view box. Unsupported/invalid scale is 1x.
inline int MinimapZoomBoxSpan(int overview, int viewport, int mapExtent, float scale)
{
    if (overview <= 0 || viewport <= 0 || mapExtent <= 0) return 0;
    if (!std::isfinite(scale) || scale < 1.0f) scale = 1.0f;
    const double span = (double)overview * viewport / ((double)mapExtent * scale) + 2.0;
    return span >= overview ? overview : (int)span;
}

// Invoke the writer only for pixels inside the centered, clipped box.
template<class Writer>
inline void MinimapZoomBoxBorder(int canvasWidth, int canvasHeight, int width, int height, Writer write)
{
    if (canvasWidth <= 0 || canvasHeight <= 0 || width <= 0 || height <= 0) return;
    if (width > canvasWidth) width = canvasWidth;
    if (height > canvasHeight) height = canvasHeight;
    const int x = (canvasWidth - width) / 2, y = (canvasHeight - height) / 2;
    for (int col = 0; col < width; ++col) {
        write(x + col, y);
        if (height > 1) write(x + col, y + height - 1);
    }
    for (int row = 1; row + 1 < height; ++row) {
        write(x, y + row);
        if (width > 1) write(x + width - 1, y + row);
    }
}
