#pragma once

// ============================================================================
// SDL2Scrollbar - ONE definition of a vertical scrollbar's geometry, chrome and
// hit-test, shared by every scrolling widget in the SDL2 UI.
//
// Before this existed each widget carried its own copy of the thumb arithmetic
// (SDL2Listbox, SDL2RouteWindow, SDL2UnitList), none of them had arrow buttons,
// and the copies had already drifted: the route window painted a 6px thumb in an
// 8px trough, and the unit list hit-tested its gutter only while overflowing, so
// a click in the bar selected a row whenever the content fit. A scrollbar is pure
// arithmetic over four numbers, so it belongs in one place that everybody derives
// from - Render() and HandleEvent() of each owner call the SAME Layout(), which is
// what makes "the thing I clicked is the thing that was drawn" structural instead
// of a coincidence that two copies of the math happen to agree.
//
// SCROLL UNITS. Everything here is in the owner's own unit, whatever that is:
//   SDL2Listbox / SDL2RouteWindow  -> rows   (content = item count, view = visible rows)
//   SDL2UnitList                   -> pixels (content = total height, view = panel height)
// The only requirement is that content, view and offset share one unit.
//
// The geometry half (MaxOffset / Clamp / Layout / HitTest / OffsetFromThumbTop)
// is pure: a rect plus three ints in, rects and an enum out - no SDL calls, no
// state. tests/ui/test_scrollbar_geom.cpp pins it directly.
// ============================================================================

#include <SDL.h>

namespace EnSb {

// Smallest thumb we will draw, in pixels: a 2px thumb on a long list is not
// grabbable, so the thumb stops shrinking here and only its position moves.
static const int kMinThumb = 12;

// Press-and-hold auto-repeat timing (SDL tick clock, milliseconds). Windows' own
// scrollbars use roughly 400ms then 30-60ms; 400/60 matches that feel.
static const Uint32 kRepeatDelayMs = 400;
static const Uint32 kRepeatRateMs  = 60;
static const int    kRepeatMaxBurst = 8;   // caps the catch-up after a stalled frame

// The complete on-screen geometry of one scrollbar, in the owner's pixel space.
//   bar   = the whole reserved gutter
//   up / down = the two arrow buttons (present only when hasArrows)
//   track = the trough the thumb slides in (bar minus the two arrow squares)
//   thumb = the draggable block
// bar is partitioned exactly by up + track + down, with thumb inside track.
struct Metrics {
    SDL_Rect bar   = { 0, 0, 0, 0 };
    SDL_Rect up    = { 0, 0, 0, 0 };
    SDL_Rect down  = { 0, 0, 0, 0 };
    SDL_Rect track = { 0, 0, 0, 0 };
    SDL_Rect thumb = { 0, 0, 0, 0 };
    bool hasArrows = false;   // false when the bar is too short for 2 arrows + a thumb
    bool canScroll = false;   // the content actually overflows the view
};

// What a point inside the bar means. HitTest partitions the WHOLE bar: every
// point in bar returns one of these five, and points outside it return HitNone.
enum Hit {
    HitNone = 0,
    HitArrowUp,
    HitPageUp,      // trough above the thumb
    HitThumb,
    HitPageDown,    // trough below the thumb
    HitArrowDown
};

inline bool In(const SDL_Rect& r, int x, int y) {
    return r.w > 0 && r.h > 0 &&
           x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

// Largest legal scroll offset: everything past the last viewable unit.
inline int MaxOffset(int content, int view) {
    int m = content - view;
    return m > 0 ? m : 0;
}

inline int Clamp(int offset, int content, int view) {
    int m = MaxOffset(content, view);
    if (offset < 0) return 0;
    if (offset > m) return m;
    return offset;
}

// Lay the bar out for the current content/view/offset. offset is clamped inside,
// so a caller that has not clamped yet still gets a thumb inside its track.
inline Metrics Layout(SDL_Rect bar, int content, int view, int offset,
                      int minThumb = kMinThumb) {
    Metrics m;
    if (bar.w <= 0 || bar.h <= 0) return m;
    m.bar = bar;

    // Arrows are squares as wide as the gutter, so a widget picks its gutter width
    // and the arrows follow. A gutter too short to hold both plus a usable thumb
    // drops them rather than drawing two unclickable slivers.
    const int arrow = bar.w;
    m.hasArrows = (bar.h >= arrow * 2 + minThumb);
    if (m.hasArrows) {
        m.up    = { bar.x, bar.y,                 arrow, arrow };
        m.down  = { bar.x, bar.y + bar.h - arrow, arrow, arrow };
        m.track = { bar.x, bar.y + arrow, bar.w, bar.h - arrow * 2 };
    } else {
        m.track = bar;
    }

    const int maxOff = MaxOffset(content, view);
    m.canScroll = (maxOff > 0);

    // Thumb length = the visible fraction of the content, floored so it stays
    // grabbable and capped at the track.
    int th = (content > 0) ? (m.track.h * view / content) : m.track.h;
    if (th < minThumb)  th = minThumb;
    if (th > m.track.h) th = m.track.h;

    int ty = m.track.y;
    if (maxOff > 0) {
        const int off = Clamp(offset, content, view);
        ty = m.track.y + (m.track.h - th) * off / maxOff;
    }
    m.thumb = { m.track.x, ty, m.track.w, th };
    return m;
}

// Inverse of the paint mapping: which region of the bar is under (x, y). Arrows
// are checked first so an arrow square wins over the trough it abuts.
inline Hit HitTest(const Metrics& m, int x, int y) {
    if (!In(m.bar, x, y)) return HitNone;
    if (m.hasArrows) {
        if (In(m.up,   x, y)) return HitArrowUp;
        if (In(m.down, x, y)) return HitArrowDown;
    }
    if (In(m.thumb, x, y)) return HitThumb;
    return (y < m.thumb.y) ? HitPageUp : HitPageDown;
}

// Drag: the offset that puts the thumb's TOP edge at thumbTop.
inline int OffsetFromThumbTop(const Metrics& m, int thumbTop, int content, int view) {
    const int maxOff = MaxOffset(content, view);
    const int travel = m.track.h - m.thumb.h;   // pixels the thumb can actually move
    if (travel <= 0 || maxOff <= 0) return 0;
    int dy = thumbTop - m.track.y;
    if (dy < 0) dy = 0;              // integer division truncates toward zero, so the
    int off = dy * maxOff / travel;  // negative side is clamped before dividing
    return Clamp(off, content, view);
}

// ---------------------------------------------------------------------------
// Chrome
// ---------------------------------------------------------------------------
struct Colors {
    SDL_Color trough;     // the gutter behind everything
    SDL_Color face;       // thumb and arrow button face
    SDL_Color light;      // bevel highlight (top-left)
    SDL_Color dark;       // bevel shadow (bottom-right)
    SDL_Color glyph;      // arrow triangle, enabled
    SDL_Color glyphDim;   // arrow triangle, already at that end of the range
};

// The gold/brown dialog palette (SDL2Listbox, SDL2RouteWindow). Same values the
// listbox bar already used, so existing screens are unchanged apart from the arrows.
inline Colors GoldColors() {
    Colors c;
    c.trough   = {  50,  42,  28, 255 };
    c.face     = { 140, 120,  80, 255 };
    c.light    = { 196, 176, 124, 255 };
    c.dark     = {  92,  76,  46, 255 };
    c.glyph    = {  30,  25,  15, 255 };   // same dark ink the gold buttons letter with
    c.glyphDim = { 105,  92,  64, 255 };
    return c;
}

// The slate/green in-game panel palette (SDL2UnitList).
inline Colors SlateColors() {
    Colors c;
    c.trough   = {  30,  35,  32, 255 };
    c.face     = { 100, 110, 105, 255 };
    c.light    = { 150, 160, 155, 255 };
    c.dark     = {  55,  62,  58, 255 };
    c.glyph    = {  20,  24,  22, 255 };
    c.glyphDim = {  72,  80,  76, 255 };
    return c;
}

inline void SbFill(SDL_Surface* dst, const SDL_Rect& r, SDL_Color c) {
    if (!dst || r.w <= 0 || r.h <= 0) return;
    SDL_Rect rr = r;
    SDL_FillRect(dst, &rr, SDL_MapRGB(dst->format, c.r, c.g, c.b));
}

inline void SbBevel(SDL_Surface* dst, const SDL_Rect& r, SDL_Color light, SDL_Color dark) {
    if (r.w <= 0 || r.h <= 0) return;
    SbFill(dst, { r.x, r.y, r.w, 1 }, light);
    SbFill(dst, { r.x, r.y, 1, r.h }, light);
    SbFill(dst, { r.x, r.y + r.h - 1, r.w, 1 }, dark);
    SbFill(dst, { r.x + r.w - 1, r.y, 1, r.h }, dark);
}

// Solid triangle inside box, apex up or down - one 1px row per scanline, so it
// needs no renderer and matches how the rest of this UI paints its chrome.
inline void SbTriangle(SDL_Surface* dst, SDL_Rect box, bool pointUp, SDL_Color c) {
    if (box.w <= 0 || box.h <= 0) return;
    for (int i = 0; i < box.h; i++) {
        int wide = box.w * (i + 1) / box.h;
        if (wide < 1) wide = 1;
        if (((box.w - wide) & 1) != 0 && wide < box.w) wide++;   // keeps it centered
        const int x = box.x + (box.w - wide) / 2;
        const int y = pointUp ? (box.y + i) : (box.y + box.h - 1 - i);
        SbFill(dst, { x, y, wide, 1 }, c);
    }
}

// One arrow button. enabled == false draws a dimmed glyph and an all-dark outline
// (no raised bevel), so "you are already at this end" reads at a glance.
inline void SbArrow(SDL_Surface* dst, const SDL_Rect& r, bool pointUp,
                    const Colors& c, bool enabled) {
    if (r.w <= 0 || r.h <= 0) return;
    SbFill(dst, r, c.face);
    if (enabled) SbBevel(dst, r, c.light, c.dark);
    else         SbBevel(dst, r, c.dark,  c.dark);
    // Glyph box, inset so the triangle sits inside the bevel. An ODD width keeps
    // the apex on a whole pixel instead of straddling two.
    const int pad = (r.w >= 12) ? 3 : 2;
    SDL_Rect g = { r.x + pad, r.y + pad + 1, r.w - 2 * pad, r.h - 2 * pad - 2 };
    if ((g.w & 1) == 0 && g.w > 1) { g.w--; g.x++; }
    if (g.h < 2) { g.h = 2; g.y = r.y + (r.h - 2) / 2; }
    SbTriangle(dst, g, pointUp, enabled ? c.glyph : c.glyphDim);
}

// Paint the whole bar: trough, thumb, both arrows. When the content fits, the
// track is filled edge to edge with a flat face and both arrows are dimmed - the
// bar keeps its place (no layout jump) while advertising that it is inert.
inline void Draw(SDL_Surface* dst, const Metrics& m, const Colors& c,
                 int offset, int content, int view) {
    if (!dst || m.bar.w <= 0 || m.bar.h <= 0) return;
    SbFill(dst, m.bar, c.trough);

    const int maxOff = MaxOffset(content, view);
    const int off    = Clamp(offset, content, view);

    if (m.canScroll) {
        SbFill(dst, m.thumb, c.face);
        SbBevel(dst, m.thumb, c.light, c.dark);
    } else {
        SbFill(dst, m.track, c.face);
        SbBevel(dst, m.track, c.dark, c.dark);
    }

    if (m.hasArrows) {
        SbArrow(dst, m.up,   true,  c, m.canScroll && off > 0);
        SbArrow(dst, m.down, false, c, m.canScroll && off < maxOff);
    }
}

// ---------------------------------------------------------------------------
// Press-and-hold auto-repeat. The owner calls Begin() on the arrow press (and
// performs the FIRST step itself), then Steps(now) once per repaint: Steps returns
// 0 until kRepeatDelayMs has passed and then one step per kRepeatRateMs. It is a
// pure function of the press time, so a skipped frame cannot lose steps and a long
// stall cannot fling the list (kRepeatMaxBurst caps the catch-up).
// ---------------------------------------------------------------------------
struct Repeat {
    int    dir    = 0;   // -1 = toward 0, +1 = toward MaxOffset, 0 = not held
    Uint32 t0     = 0;   // press time
    Uint32 issued = 0;   // repeat steps already handed out since the delay expired

    void Begin(int d, Uint32 now) { dir = d; t0 = now; issued = 0; }
    void End()                    { dir = 0; issued = 0; }
    bool Active() const           { return dir != 0; }

    int Steps(Uint32 now) {
        if (dir == 0) return 0;
        const Uint32 held = now - t0;
        if (held < kRepeatDelayMs) return 0;
        const Uint32 due = 1 + (held - kRepeatDelayMs) / kRepeatRateMs;
        if (due <= issued) return 0;
        Uint32 n = due - issued;
        issued = due;
        if (n > (Uint32)kRepeatMaxBurst) n = (Uint32)kRepeatMaxBurst;
        return (int)n;
    }
};

} // namespace EnSb
