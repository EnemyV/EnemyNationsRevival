// Keep assertions active in Release CTest builds as well.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../enations_latest/src/minimap_zoom_box.h"
#include "../enations_latest/src/zoom_interpolation.h"
#include "../enations_latest/src/PerfHarnessZoom.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <initializer_list>
#include <vector>

int main()
{
    assert(MinimapZoomBoxSpan(200, 800, 1600, 1.0f) == 102);
    assert(MinimapZoomBoxSpan(200, 800, 1600, 2.0f) == 52);
    assert(MinimapZoomBoxSpan(200, 800, 1600, 1.5f) == 68);
    assert(MinimapZoomBoxSpan(40, 800, 100, 1.0f) == 40);
    assert(MinimapZoomBoxSpan(200, 800, 1600, 0.0f) == 102);
    assert(MinimapZoomBoxSpan(200, 800, 1600, 0.5f) == 102);
    assert(MinimapZoomBoxSpan(200, 800, 1600, std::numeric_limits<float>::quiet_NaN()) == 102);
    assert(MinimapZoomBoxSpan(200, 800, 1600, std::numeric_limits<float>::infinity()) == 102);
    assert(MinimapZoomBoxSpan(200, 800, 0, 1.0f) == 0);
    assert(MinimapZoomBoxSpan(200, 800, -1, 1.0f) == 0);
    assert(MinimapZoomBoxSpan(0, 800, 1600, 1.0f) == 0);
    assert(MinimapZoomBoxSpan(200, -1, 1600, 1.0f) == 0);
    // Guard bytes surround the whole canvas: full-size and degenerate boxes
    // must never write outside it and must use its last valid row/column.
    for (int w : {0, 1, 4, 9}) for (int h : {0, 1, 3, 9}) {
        std::vector<int> pixels(14, -7);
        MinimapZoomBoxBorder(4, 3, w, h, [&](int x, int y) {
            assert(x >= 0 && x < 4 && y >= 0 && y < 3);
            pixels[1 + y * 4 + x] = 1;
        });
        assert(pixels.front() == -7 && pixels.back() == -7);
        if (w == 0 || h == 0) for (int value : pixels) assert(value == -7);
        if (w >= 4 && h >= 3) { assert(pixels[4] == 1); assert(pixels[12] == 1); }
    }
    const float at60 = AreaZoomScale(1.0f, 2.0f, 100, 180, 190);
    assert(std::fabs(at60 - 1.5f) < 0.00001f);
    // Intermediate samples must not affect the value at a given timestamp.
    for (unsigned int now = 100; now < 190; now += 7)
        (void)AreaZoomScale(1.0f, 2.0f, 100, 180, now);
    assert(AreaZoomScale(1.0f, 2.0f, 100, 180, 190) == at60);
    // Reversing starts exactly where the old motion was, and moves back.
    assert(AreaZoomScale(at60, 1.0f, 190, 90, 190) == at60);
    assert(AreaZoomScale(at60, 1.0f, 190, 90, 235) < at60);
    assert(AreaZoomScale(at60, 1.0f, 190, 90, 280) == 1.0f);
    assert(AreaZoomScale(2.0f, 1.0f, 0xfffffff0u, 32, 0u) == 1.5f);
    // A stationary pan pointer cannot create motion when both positions use the
    // current transform, even when the zoom changed since the prior event.
    AreaZoomPoint raw = { 720, 200 };
    AreaZoomPoint previous = AreaZoomTransform(raw, 500, 400, 0.7f);
    AreaZoomPoint current = AreaZoomTransform(raw, 500, 400, 0.7f);
    assert(previous.x == current.x && previous.y == current.y);
    AreaZoomPoint next = AreaZoomTransform({740, 200}, 500, 400, 0.7f);
    assert(next.x - previous.x == 14);
    // Leaving capture invalidates the previous point; re-entry causes no pan jump.
    AreaZoomPoint reentry = AreaZoomPanPrior({80, 90}, {5, 8}, false);
    assert(reentry.x == 80 && reentry.y == 90);
    AreaZoomPoint continuing = AreaZoomPanPrior({80, 90}, {5, 8}, true);
    assert(continuing.x == 5 && continuing.y == 8);
    // A stationary physical endpoint maps to a new client endpoint as zoom
    // changes; a release without motion must select the refreshed rectangle.
    AreaZoomPoint selectAnchor = {200, 200};
    AreaZoomPoint physicalEnd = {800, 500};
    AreaZoomPoint zoomEnd = AreaZoomTransform(physicalEnd, 500, 400, 0.5f);
    AreaZoomRect beforeZoom = AreaZoomSelectionRect(selectAnchor, physicalEnd, 1000, 800);
    AreaZoomRect duringZoom = AreaZoomSelectionRect(selectAnchor, zoomEnd, 1000, 800);
    AreaZoomRect onRelease = AreaZoomSelectionRect(selectAnchor, zoomEnd, 1000, 800);
    assert(beforeZoom.right == 800 && duringZoom.right == 650);
    assert(duringZoom.bottom == 450 && onRelease.right == duringZoom.right && onRelease.bottom == duringZoom.bottom);
    // Supported presentation magnifies the asset (scale >= 1), so inverse
    // mapping keeps each physical viewport edge within the backing surface.
    for (float level : {0.0f, 0.25f, 0.5f, 0.999f}) {
        float scale = AreaZoomAssetScale(level, AreaZoomCoarseLevel(level));
        assert(scale >= 1.0f);
        AreaZoomPoint topLeft = AreaZoomTransform({0, 0}, 500, 400, 1.0f / scale);
        AreaZoomPoint bottomRight = AreaZoomTransform({999, 799}, 500, 400, 1.0f / scale);
        assert(topLeft.x >= 0 && topLeft.y >= 0);
        assert(bottomRight.x < 1000 && bottomRight.y < 800);
    }
    AreaZoomRect clipped = AreaZoomSelectionRect({1200, 900}, {-20, -30}, 1000, 800);
    assert(clipped.left == 0 && clipped.top == 0 && clipped.right == 1000 && clipped.bottom == 800);
    AreaZoomRect click = AreaZoomSelectionRect({100, 100}, {100, 100}, 1000, 800);
    assert(click.left == 0 && click.right == 0); // threshold still decides click vs drag
    // Zoom may move map coordinates, but only physical pointer motion starts a drag.
    AreaZoomPoint rawOrigin = {100, 100};
    assert(!AreaZoomSelectionDrag(rawOrigin, {100, 100}, 20, 10));
    assert(!AreaZoomSelectionDrag(rawOrigin, {119, 109}, 20, 10));
    assert(AreaZoomSelectionDrag(rawOrigin, {120, 100}, 20, 10));
    assert(AreaZoomSelectionDrag(rawOrigin, {100, 90}, 20, 10));
    // Returning inside the threshold restores click classification.
    assert(!AreaZoomSelectionDrag(rawOrigin, rawOrigin, 20, 10));
    assert(!AreaZoomSelectionDrag(rawOrigin, {101, 100}, 20, 10));
    // Pending fallback input cannot survive a successful smooth-mode recovery.
    assert(AreaZoomFallbackFraction(0.75f, false) == 0.75f);
    assert(AreaZoomFallbackFraction(0.75f, true) == 0.0f);
    assert(AreaZoomFallbackFraction(-0.75f, true) == 0.0f);
    // Adjacent asset levels meet at the same projected scale at their boundary.
    assert(AreaZoomCoarseLevel(2.0f) == 2);
    assert(AreaZoomCoarseLevel(2.01f) == 3);
    assert(std::fabs(AreaZoomAssetScale(2.0f, 3) - 2.0f) < 0.00001f);
    assert(std::fabs(AreaZoomAssetScale(2.0f, 2) - 1.0f) < 0.00001f);
    // Fractional targets produce intermediate scale rather than a notch deadband.
    assert(AreaZoomAssetScale(2.9f, AreaZoomCoarseLevel(2.9f)) > 1.0f);
    assert(PerfHarnessZoomReached(true, 2.0f, 2.0f));
    // A previously successful settle must be checked again at MARK END.
    bool settled = PerfHarnessZoomReached(true, 2.0f, 2.0f);
    assert(settled && !PerfHarnessZoomReached(true, 2.1f, 2.0f));
    assert(settled && !PerfHarnessZoomReached(false, 2.0f, 2.0f));
    assert(!PerfHarnessZoomReached(true, 2.1f, 2.0f));
    assert(!PerfHarnessZoomReached(false, 2.0f, 2.0f));
    assert(!PerfHarnessZoomReached(true, NAN, 2.0f));
    std::string id; float delta;
    assert(PerfHarnessParseZoom("ZOOM trial_1 -0.25", id, delta) && id == "trial_1" && delta == -0.25f);
    assert(!PerfHarnessParseZoom("ZOOM trial 0", id, delta));
    assert(!PerfHarnessParseZoom("ZOOM trial 2", id, delta));
    assert(!PerfHarnessParseZoom("ZOOM trial 0.5 extra", id, delta));
    assert(!PerfHarnessParseZoom("ZOOM bad/id 0.5", id, delta));
}
