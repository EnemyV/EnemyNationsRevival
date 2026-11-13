#pragma once
#include <cmath>

// Elapsed-time interpolation shared by the area-map presentation and its
// headless checks. Unsigned subtraction also handles SDL tick wraparound.
inline float AreaZoomScale(float from, float to, unsigned int start,
                           unsigned int duration, unsigned int now)
{
    float t = (now - start) / (float)duration;
    if (t > 1.0f) t = 1.0f;
    float eased = t * t * (3.0f - 2.0f * t);
    return from + (to - from) * eased;
}

// Apply a uniform map transform about its viewport centre.
struct AreaZoomPoint { int x, y; };
inline AreaZoomPoint AreaZoomTransform(AreaZoomPoint point, float cx, float cy, float scale)
{
    return { (int)std::lround(cx + (point.x - cx) * scale),
             (int)std::lround(cy + (point.y - cy) * scale) };
}
inline int AreaZoomCoarseLevel(float level) { return (int)std::ceil(level - 0.000001f); }
inline float AreaZoomAssetScale(float level, int coarse) { return std::pow(2.0f, coarse - level); }

// Re-entering after an outside drag starts with zero displacement.
inline AreaZoomPoint AreaZoomPanPrior(AreaZoomPoint current, AreaZoomPoint prior, bool priorValid)
{
    return priorValid ? prior : current;
}
inline float AreaZoomFallbackFraction(float pending, bool smoothAvailable)
{
    return smoothAvailable ? 0.0f : pending;
}

// Bounds for a selection anchored in map coordinates; endpoints are clipped
// to the client rectangle after normalization, matching the existing drag.
struct AreaZoomRect { int left, top, right, bottom; };
inline AreaZoomRect AreaZoomSelectionRect(AreaZoomPoint origin, AreaZoomPoint end, int width, int height)
{
    int l = origin.x < end.x ? origin.x : end.x;
    int r = origin.x < end.x ? end.x : origin.x;
    int t = origin.y < end.y ? origin.y : end.y;
    int b = origin.y < end.y ? end.y : origin.y;
    if (l < 0) l = 0; if (t < 0) t = 0;
    if (r > width) r = width; if (b > height) b = height;
    if (l >= r || t >= b) return {0, 0, 0, 0};
    return {l, t, r, b};
}

// Classify selection from physical motion, independent of the animated map.
inline bool AreaZoomSelectionDrag(AreaZoomPoint origin, AreaZoomPoint point,
                                  int thresholdX, int thresholdY)
{
    const long long dx = (long long)point.x - origin.x;
    const long long dy = (long long)point.y - origin.y;
    return dx >= thresholdX || dx <= -thresholdX ||
           dy >= thresholdY || dy <= -thresholdY;
}
