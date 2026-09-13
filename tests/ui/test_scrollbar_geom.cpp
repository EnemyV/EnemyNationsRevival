// test_scrollbar_geom.cpp -- pins the shared scrollbar geometry in
// enations_latest/src/SDL2Scrollbar.h. Compiles the PRODUCTION header (not a mirror
// of it), so a change to the real arithmetic shows up here.
//
// What is nailed down:
//   1. arrow squares sit exactly at the two ends of the bar, as wide as the gutter
//   2. the track is the bar minus the two arrows, and the thumb never leaves the track
//      (so it can never overlap an arrow button, at any offset)
//   3. HitTest partitions the WHOLE bar with no gaps and no overlaps: sweeping every
//      y in the bar yields arrow-up / page-up / thumb / page-down / arrow-down in that
//      order, and every point outside the bar yields HitNone
//   4. clamping at both ends, and Layout/HitTest/OffsetFromThumbTop all agreeing about
//      where the thumb is
//   5. the auto-repeat clock: nothing before the delay, then one step per interval
//
// Build/run: tests/ui/run-ui-tests.ps1   (no game build, no CMake, no link to SDL)

#define SDL_MAIN_HANDLED   // we supply main(), so SDL_main must not rename it
#include "SDL2Scrollbar.h"
#include "../ai/microtest.h"

#include <cstdio>
#include <vector>

using namespace EnSb;

static bool SameRect(SDL_Rect a, SDL_Rect b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

// ---------------------------------------------------------------------------
// 1 + 2. Arrow placement, track extent, thumb containment.
// ---------------------------------------------------------------------------
static void TestArrowsAndTrack() {
    const SDL_Rect bar = { 200, 50, 12, 198 };   // a dialog list box gutter
    const int content = 40, view = 9;

    Metrics m = Layout(bar, content, view, 0);
    CHECK(m.hasArrows);
    // Arrows are squares as wide as the gutter, flush with the two ends.
    CHECK(SameRect(m.up,   SDL_Rect{ 200, 50, 12, 12 }));
    CHECK(SameRect(m.down, SDL_Rect{ 200, 50 + 198 - 12, 12, 12 }));
    // Track = bar minus the two arrow squares, full gutter width.
    CHECK(SameRect(m.track, SDL_Rect{ 200, 62, 12, 198 - 24 }));
    // bar is partitioned exactly: up + track + down, no gap, no overlap.
    CHECK_EQ(m.up.h + m.track.h + m.down.h, bar.h);
    CHECK_EQ(m.up.y + m.up.h, m.track.y);
    CHECK_EQ(m.track.y + m.track.h, m.down.y);

    // The thumb stays inside the track at EVERY legal offset, so it can never be
    // drawn over an arrow button.
    for (int off = -5; off <= MaxOffset(content, view) + 5; off++) {
        Metrics mm = Layout(bar, content, view, off);
        CHECK(mm.thumb.y >= mm.track.y);
        CHECK(mm.thumb.y + mm.thumb.h <= mm.track.y + mm.track.h);
        CHECK(mm.thumb.y >= mm.up.y + mm.up.h);            // below the up arrow
        CHECK(mm.thumb.y + mm.thumb.h <= mm.down.y);       // above the down arrow
        CHECK(mm.thumb.h >= kMinThumb);
    }

    // At offset 0 the thumb is at the top of the track; at max it is at the bottom.
    Metrics top = Layout(bar, content, view, 0);
    Metrics bot = Layout(bar, content, view, MaxOffset(content, view));
    CHECK_EQ(top.thumb.y, top.track.y);
    CHECK_EQ(bot.thumb.y + bot.thumb.h, bot.track.y + bot.track.h);

    // A bar too short for two arrows plus a usable thumb drops the arrows rather
    // than drawing slivers, and then the track is the whole bar.
    Metrics tiny = Layout(SDL_Rect{ 0, 0, 12, 30 }, content, view, 0);
    CHECK(!tiny.hasArrows);
    CHECK(SameRect(tiny.track, SDL_Rect{ 0, 0, 12, 30 }));
    // 12*2 + kMinThumb = 36 is the threshold.
    CHECK(!Layout(SDL_Rect{ 0, 0, 12, 35 }, content, view, 0).hasArrows);
    CHECK(Layout(SDL_Rect{ 0, 0, 12, 36 }, content, view, 0).hasArrows);

    // A degenerate gutter produces nothing at all (no div-by-zero, no stray rects).
    Metrics none = Layout(SDL_Rect{ 0, 0, 0, 100 }, content, view, 0);
    CHECK(!none.hasArrows);
    CHECK(!none.canScroll);
    CHECK_EQ(none.thumb.w, 0);
}

// ---------------------------------------------------------------------------
// 3. The hit-test partition: every y in the bar, in order, nothing missed.
// ---------------------------------------------------------------------------
static void TestHitPartition() {
    const SDL_Rect bar = { 200, 50, 12, 198 };
    const int content = 40, view = 9;

    for (int off = 0; off <= MaxOffset(content, view); off++) {
        Metrics m = Layout(bar, content, view, off);

        // Walk the bar top to bottom. The sequence of regions must be exactly
        // ArrowUp, [PageUp], Thumb, [PageDown], ArrowDown -- each contiguous, none
        // skipped, nothing else, and never HitNone anywhere inside the bar.
        std::vector<Hit> seq;
        int misses = 0, thumbRows = 0;
        for (int y = bar.y; y < bar.y + bar.h; y++) {
            Hit h = HitTest(m, bar.x + bar.w / 2, y);
            if (h == HitNone) misses++;
            if (h == HitThumb) thumbRows++;
            if (seq.empty() || seq.back() != h) seq.push_back(h);
        }
        CHECK_EQ(misses, 0);                    // no gap in the partition
        CHECK_EQ(thumbRows, m.thumb.h);         // the thumb hit-area IS the thumb
        CHECK(seq.front() == HitArrowUp);
        CHECK(seq.back()  == HitArrowDown);
        // No region appears twice (that would mean the walk left and re-entered it).
        for (size_t i = 0; i < seq.size(); i++)
            for (size_t j = i + 1; j < seq.size(); j++)
                CHECK(seq[i] != seq[j]);
        // The thumb is present, and the pages sit on the correct side of it.
        bool sawThumb = false, orderOk = true;
        for (size_t i = 0; i < seq.size(); i++) {
            if (seq[i] == HitThumb) sawThumb = true;
            if (seq[i] == HitPageUp   && sawThumb) orderOk = false;
            if (seq[i] == HitPageDown && !sawThumb) orderOk = false;
        }
        CHECK(sawThumb);
        CHECK(orderOk);
    }

    // Outside the bar is always HitNone -- a click beside the gutter must fall
    // through to the rows, not page the list.
    Metrics m = Layout(bar, content, view, 3);
    CHECK(HitTest(m, bar.x - 1,         bar.y + 20) == HitNone);
    CHECK(HitTest(m, bar.x + bar.w,     bar.y + 20) == HitNone);
    CHECK(HitTest(m, bar.x + 2, bar.y - 1)          == HitNone);
    CHECK(HitTest(m, bar.x + 2, bar.y + bar.h)      == HitNone);
    // The arrow squares win over the trough they abut (corner pixels included).
    CHECK(HitTest(m, bar.x,             bar.y)                  == HitArrowUp);
    CHECK(HitTest(m, bar.x + bar.w - 1, bar.y + m.up.h - 1)      == HitArrowUp);
    CHECK(HitTest(m, bar.x,             bar.y + bar.h - 1)       == HitArrowDown);
    CHECK(HitTest(m, bar.x + bar.w - 1, m.down.y)                == HitArrowDown);

    // Without arrows the partition is just page/thumb/page, still with no gaps.
    Metrics na = Layout(SDL_Rect{ 0, 0, 12, 30 }, 40, 9, 2);
    int misses = 0;
    for (int y = 0; y < 30; y++)
        if (HitTest(na, 6, y) == HitNone) misses++;
    CHECK_EQ(misses, 0);
    CHECK(HitTest(na, 6, 0) != HitArrowUp);
}

// ---------------------------------------------------------------------------
// 4. Clamping, and the drag inverse agreeing with Layout.
// ---------------------------------------------------------------------------
static void TestClampAndDrag() {
    CHECK_EQ(MaxOffset(40, 9), 31);
    CHECK_EQ(MaxOffset(9, 9), 0);        // exactly fits -> nothing to scroll
    CHECK_EQ(MaxOffset(3, 9), 0);        // fewer items than rows
    CHECK_EQ(MaxOffset(0, 9), 0);        // empty list
    CHECK_EQ(Clamp(-100, 40, 9), 0);
    CHECK_EQ(Clamp(999, 40, 9), 31);
    CHECK_EQ(Clamp(7, 40, 9), 7);
    CHECK_EQ(Clamp(5, 9, 9), 0);         // no range at all -> pinned to 0

    const SDL_Rect bar = { 0, 0, 12, 198 };
    const int content = 40, view = 9, maxOff = MaxOffset(content, view);

    // Dragging the thumb to either extreme lands exactly on the extremes, and
    // dragging past them clamps instead of wrapping (integer division toward zero
    // would otherwise make a negative drag read as 0 only by luck).
    Metrics m = Layout(bar, content, view, 0);
    CHECK_EQ(OffsetFromThumbTop(m, m.track.y,                          content, view), 0);
    CHECK_EQ(OffsetFromThumbTop(m, m.track.y - 500,                    content, view), 0);
    CHECK_EQ(OffsetFromThumbTop(m, m.track.y + m.track.h - m.thumb.h,  content, view), maxOff);
    CHECK_EQ(OffsetFromThumbTop(m, m.track.y + 5000,                   content, view), maxOff);

    // Round trip: the offset Layout drew the thumb at is the offset a drag to that
    // same pixel reports. With integer pixels the recovered offset can land one unit
    // short of the original (several offsets share a thumb pixel), never more, and it
    // must be monotone -- dragging down never scrolls up.
    int prev = -1;
    for (int off = 0; off <= maxOff; off++) {
        Metrics mm = Layout(bar, content, view, off);
        int back = OffsetFromThumbTop(mm, mm.thumb.y, content, view);
        CHECK(back <= off);
        CHECK(off - back <= 1);
        Metrics sweep = Layout(bar, content, view, 0);
        int at = OffsetFromThumbTop(sweep, sweep.track.y + off, content, view);
        CHECK(at >= prev);
        prev = at;
    }

    // A non-overflowing list: no range, so a drag cannot move it off 0.
    Metrics fit = Layout(bar, 4, 9, 0);
    CHECK(!fit.canScroll);
    CHECK_EQ(fit.thumb.h, fit.track.h);     // full-height inert thumb
    CHECK_EQ(OffsetFromThumbTop(fit, fit.track.y + 50, 4, 9), 0);

    // Pixel-scrolled owner (SDL2UnitList): same arithmetic, unit = pixels.
    const int totalH = 12 * 64, panelH = 300;
    Metrics px = Layout(SDL_Rect{ 0, 0, 14, panelH }, totalH, panelH, 0);
    CHECK(px.canScroll);
    CHECK(px.hasArrows);
    CHECK_EQ(MaxOffset(totalH, panelH), 468);
    Metrics pxEnd = Layout(SDL_Rect{ 0, 0, 14, panelH }, totalH, panelH, 468);
    CHECK_EQ(pxEnd.thumb.y + pxEnd.thumb.h, pxEnd.track.y + pxEnd.track.h);
}

// ---------------------------------------------------------------------------
// 5. SDL2Listbox's gutter rect (SDL2UI.cpp ScrollbarRect) reaches the box's
//    full inner height, not just the painted rows. m_rect.h is rarely a whole
//    multiple of m_itemHeight (Pick Your Player: h=366, row=24 -> a 6px
//    remainder left over), and the pre-fix bar was VisibleRows()*m_itemHeight
//    tall, leaving that remainder as a strip of m_colBg background showing
//    under the down arrow -- QA: "white box at the bottom of the Pick Your
//    Player box". This fixture links only the header (never SDL2UI.cpp), so
//    the bar-rect formula below is kept in sync by hand with the production
//    one in SDL2UI.cpp::ScrollbarRect.
// ---------------------------------------------------------------------------
static SDL_Rect ListboxGutterRect(SDL_Rect box, int scrollbarW) {
    // Mirrors SDL2UI.cpp::ScrollbarRect: spans the box's full inner height,
    // inside the 1px bevel on each side -- NOT VisibleRows()*itemHeight.
    return SDL_Rect{ box.x + box.w - scrollbarW, box.y + 1,
                     scrollbarW - 1, box.h - 2 };
}

static void CheckGutterReachesBottomBevel(SDL_Rect box, int itemHeight, int items) {
    const int scrollbarW = 12;
    const int view = box.h / itemHeight;              // VisibleRows(): whole rows only
    const int bevelRow = box.y + box.h - 1;            // the box's bottom 1px bevel row
    const int barBottomPx = bevelRow - 1;              // bottom-most gutter pixel: flush against the bevel

    SDL_Rect bar = ListboxGutterRect(box, scrollbarW);
    CHECK_EQ(bar.y + bar.h - 1, barBottomPx);          // gutter reaches the bevel, no gap below it

    Metrics m = Layout(bar, items, view, 0);
    CHECK_EQ(m.down.y + m.down.h - 1, barBottomPx);    // down arrow flush with the bevel
    CHECK(HitTest(m, bar.x + bar.w / 2, barBottomPx) == HitArrowDown);  // not HitNone
}

static void TestListboxGutterFullHeight() {
    // Pick Your Player list box: h=366 over a 24px row -- 15 whole rows, 6px
    // left over.
    CheckGutterReachesBottomBevel(SDL_Rect{ 100, 50, 200, 366 }, 24, 40);
    // A second, unrelated remainder: h=100 over a 22px row -- 4 whole rows,
    // 12px left over. Different box, different font metrics, same invariant.
    CheckGutterReachesBottomBevel(SDL_Rect{ 100, 50, 200, 100 }, 22, 10);
}

// ---------------------------------------------------------------------------
// 6. Press-and-hold auto-repeat clock.
// ---------------------------------------------------------------------------
static void TestRepeat() {
    Repeat r;
    CHECK(!r.Active());
    CHECK_EQ(r.Steps(1000), 0);          // idle never yields a step

    r.Begin(-1, 1000);
    CHECK(r.Active());
    CHECK_EQ(r.dir, -1);
    // Nothing at all until the initial delay expires: a single click must scroll
    // exactly one row (the caller does that step itself), not two.
    CHECK_EQ(r.Steps(1000), 0);
    CHECK_EQ(r.Steps(1000 + kRepeatDelayMs - 1), 0);
    CHECK_EQ(r.Steps(1000 + kRepeatDelayMs), 1);
    CHECK_EQ(r.Steps(1000 + kRepeatDelayMs), 0);                   // same instant, no double
    CHECK_EQ(r.Steps(1000 + kRepeatDelayMs + kRepeatRateMs - 1), 0);
    CHECK_EQ(r.Steps(1000 + kRepeatDelayMs + kRepeatRateMs), 1);
    // A skipped frame does not lose steps: the count is a function of elapsed time.
    CHECK_EQ(r.Steps(1000 + kRepeatDelayMs + kRepeatRateMs * 4), 3);
    // A long stall cannot fling the list: the catch-up is capped.
    CHECK_EQ(r.Steps(1000 + kRepeatDelayMs + kRepeatRateMs * 1000), kRepeatMaxBurst);

    r.End();
    CHECK(!r.Active());
    CHECK_EQ(r.Steps(9999999), 0);

    // Re-pressing restarts the delay, so a second click is also a single row.
    r.Begin(1, 5000);
    CHECK_EQ(r.dir, 1);
    CHECK_EQ(r.Steps(5000 + kRepeatDelayMs - 1), 0);
}

int main() {
    TestArrowsAndTrack();
    TestHitPartition();
    TestClampAndDrag();
    TestListboxGutterFullHeight();
    TestRepeat();
    int rc = microtest::Summary();
    std::printf("[ui_scrollbar] %s\n", rc == 0 ? "PASS" : "FAIL");
    return rc;
}
