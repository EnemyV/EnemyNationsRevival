# Area Status Bar Double Rendering Issue

## Problem Statement

The area map status bar (containing the 17 buttons) is being rendered **twice**:
1. Once in the background window (via compositor)
2. Once inside the area window itself

This creates a visual duplication where the status bar appears to be drawn on top of itself.

---

## Root Cause Analysis

### Panel Setup (CWndArea::OnCreate, lines 2277-2292)

When the area view is created:

```cpp
// Line 2277-2278: Bar panel ADDED to compositor
m_WndStatic.m_sdlPanel = theApp.m_gameWindow->GetCompositor()->AddPanel(
    "area_bar", barX, barY, barW, staticH, barZ );

// ... SDL2AreaBar initialization ...

// Line 2292: Area panel is DETACHED to own window
m_aa.m_sdlPanel->Detach(theApp.m_gameWindow.get());
```

**Key Observation**:
- Bar panel (`m_WndStatic.m_sdlPanel`) is added to the **compositor** with z-order above the area
- Area panel (`m_aa.m_sdlPanel`) is **detached** to its own OS window

### Rendering Flow (CWndArea::Draw, lines 1700-1730)

Each frame, the rendering code does:

```cpp
// Line 1710-1712: Render bar to its panel surface
if ( m_WndStatic.m_sdl2Bar ) {
    m_WndStatic.m_sdl2Bar->Render();

    // Line 1715-1729: If area is detached, ALSO blit bar into area panel
    if ( m_aa.m_sdlPanel && m_aa.m_sdlPanel->IsDetached() && m_WndStatic.m_sdlPanel ) {
        // ... blit bar surface into area panel at bottom position ...
        SDL_BlitSurface( barSurf, nullptr, areaSurf, &dst );
    }
}
```

### The Double Rendering

**Scenario**: Area panel is detached to its own SDL_Window

**Current behavior**:

```
Frame Rendering:
1. SDL2AreaBar::Render()
   └─ Renders buttons to m_WndStatic.m_sdlPanel->GetSurface()

2. Compositor::Composite() [RENDERS TO BACKGROUND WINDOW]
   ├─ Iterates all panels in z-order
   ├─ Finds m_WndStatic.m_sdlPanel (bar panel)
   └─ Blits it to background window surface

3. SDL_BlitSurface (inside Draw() at line 1725) [RENDERS TO AREA WINDOW]
   └─ Copies bar surface into area panel's own window surface

4. Area panel RenderDetached()
   └─ Presents area panel's surface to its own SDL_Window

Result:
┌──────────────────────────────────────────────┐
│        Background Window Surface             │
│                                              │
│  [Wallpaper + Bar Panel (from compositor)] ◄─ BAR APPEARS HERE
└──────────────────────────────────────────────┘

┌──────────────────────────────────────────────┐
│        Area Window Surface (own)             │
│  ┌──────────────────────────────────────────┐│
│  │ Area content (terrain, units)            ││
│  ├──────────────────────────────────────────┤│
│  │ [Bar (from SDL_BlitSurface)]         ◄─ BAR APPEARS HERE TOO
│  └──────────────────────────────────────────┘│
└──────────────────────────────────────────────┘

User sees: Bar on top of bar (double rendering)
```

---

## Why This Happens

### The Architectural Issue

The code assumes:
- Bar panel renders to compositor
- Compositor presents to one window

But when area detaches:
- Compositor still renders bar to **background** window
- Code also manually blits bar to **area's** window
- Result: Bar appears in both places

### The Intent

The logic at lines 1715-1729 was meant to:
> "When area is in its own window, make sure the bar appears in that window too"

But it doesn't account for:
> "When area is in its own window, the bar panel is still being rendered by the compositor to the background window!"

---

## Verification Checklist

Before fixing, I need to confirm:

- [x] Bar panel is added to compositor (line 2277-2278) ✓
- [x] Area panel is detached (line 2292) ✓
- [x] SDL2AreaBar::Render() writes to m_sdlPanel (SDL2AreaBar.cpp:254) ✓
- [x] Compositor iterates and renders all panels including bar (SDL2Compositor.cpp) ✓
- [x] SDL_BlitSurface happens when area is detached (line 1715-1729) ✓
- [x] Area panel has RenderDetached() that presents its own window ✓
- [ ] Verify bar is visible in BOTH windows simultaneously (user report confirms)

---

## The Fix Options

### Option 1: Hide Bar Panel from Compositor When Area is Detached ⭐ RECOMMENDED

```cpp
// In CWndArea::Draw() when SDL2AreaBar exists and area is detached
if ( m_WndStatic.m_sdl2Bar ) {
    m_WndStatic.m_sdl2Bar->Render();

    if ( m_aa.m_sdlPanel && m_aa.m_sdlPanel->IsDetached() && m_WndStatic.m_sdlPanel ) {
        // Hide bar from background window
        m_WndStatic.m_sdlPanel->SetVisible(false);  // ADD THIS

        // But blit it into area panel
        SDL_Surface* barSurf  = m_WndStatic.m_sdlPanel->GetSurface();
        SDL_Surface* areaSurf = m_aa.m_sdlPanel->GetSurface();
        // ... rest of blit logic ...
    } else {
        // Area not detached, make sure bar is visible
        m_WndStatic.m_sdlPanel->SetVisible(true);  // ADD THIS
    }
}
```

**Pros**:
- Simple one-liner fix
- Clear intent: "hide bar from background when it's in area window"
- Existing blit code handles the "show in area window" part
- Minimal changes

**Cons**:
- Requires SetVisible() to be called every frame (performance)

### Option 2: Detach Bar Panel Along with Area

```cpp
// When area is detached
m_aa.m_sdlPanel->Detach(theApp.m_gameWindow.get());
m_WndStatic.m_sdlPanel->Detach(theApp.m_gameWindow.get());  // ADD THIS
```

**Pros**:
- Both panels are independent
- Natural separation

**Cons**:
- More complex: bar needs its own window
- Positioning becomes tricky (bar inside area's window, not separate)
- Window management overhead

### Option 3: Don't Blit, Just Render Bar Panel to Both Locations

Skip the SDL_BlitSurface, let the bar panel render natively in both windows.

**Pros**:
- Follows DRY principle (single rendering path)

**Cons**:
- Very complex coordination between compositor and detached windows
- Bar panel would need to know about both windows

### Option 4: Remove Bar Panel from Compositor Entirely

Instead of adding bar to compositor, only render it via SDL2AreaBar::Render() to its panel, and let the panel be embedded in the area.

```cpp
// DON'T do this:
// m_WndStatic.m_sdlPanel = theApp.m_gameWindow->GetCompositor()->AddPanel(...)

// Instead: Don't add to compositor, only render directly
```

**Pros**:
- Single rendering path
- No double-rendering issue

**Cons**:
- Breaks the panel hierarchy
- Bar wouldn't appear in background window (might be desired, might not)

---

## Recommended Fix

**Option 1** is recommended because:

1. **Minimal code change** - One `SetVisible()` call per frame
2. **Clear intent** - Explicitly hides bar from background
3. **Preserves architecture** - Bar still in compositor, just hidden conditionally
4. **Easy to understand** - Future maintainers see the logic immediately
5. **Safe** - No risk of breaking other systems
6. **Reversible** - Easy to revert if needed

### Implementation

**File**: `d:\Enemy Nations\src\enations_latest\src\area.cpp`
**Location**: Around line 1710-1730 in CWndArea::Draw()

**Current Code**:
```cpp
if ( m_WndStatic.m_sdl2Bar )
{
    m_WndStatic.m_sdl2Bar->Render();
    if ( m_aa.m_sdlPanel && m_aa.m_sdlPanel->IsDetached() && m_WndStatic.m_sdlPanel )
    {
        // ... blit code ...
    }
}
```

**Fixed Code**:
```cpp
if ( m_WndStatic.m_sdl2Bar )
{
    m_WndStatic.m_sdl2Bar->Render();

    if ( m_aa.m_sdlPanel && m_aa.m_sdlPanel->IsDetached() && m_WndStatic.m_sdlPanel )
    {
        // When area is detached, hide bar from background compositor
        // (it will appear in the area's own window via the blit below)
        m_WndStatic.m_sdlPanel->SetVisible(false);

        SDL_Surface* barSurf  = m_WndStatic.m_sdlPanel->GetSurface();
        SDL_Surface* areaSurf = m_aa.m_sdlPanel->GetSurface();
        if ( barSurf && areaSurf )
        {
            int barOffY = m_aa.m_sdlPanel->GetHeight() - m_WndStatic.m_sdlPanel->GetHeight();
            if ( barOffY >= 0 )
            {
                SDL_Rect dst = { 0, barOffY, barSurf->w, barSurf->h };
                SDL_BlitSurface( barSurf, nullptr, areaSurf, &dst );
                m_aa.m_sdlPanel->SetDirty();
            }
        }
    }
    else if ( m_WndStatic.m_sdlPanel )
    {
        // When area NOT detached, make sure bar is visible in background
        m_WndStatic.m_sdlPanel->SetVisible(true);
    }
}
```

---

## Expected Result After Fix

**Before Fix**:
```
Background Window: [Bar visible]
Area Window:       [Bar visible]
User sees:         Double bar (overlapped)
```

**After Fix**:
```
Background Window: [Bar NOT visible when area detached]
Area Window:       [Bar visible via blit]
User sees:         Single bar in correct location
```

When area is NOT detached (normal mode):
```
Background Window: [Bar visible as normal panel]
Area Window:       [N/A - area not detached]
User sees:         Single bar in correct location
```

---

## Testing Plan

1. **Open area view in normal mode** (not detached)
   - Verify bar appears below terrain ✓
   - No double rendering ✓

2. **Detach area view to own window**
   - Verify bar appears in area's window ✓
   - Verify bar does NOT appear in background ✓
   - No double rendering ✓

3. **Re-attach area view**
   - Verify bar reappears in background ✓
   - No double rendering ✓

4. **Resize area window while detached**
   - Verify bar follows and resizes ✓
   - No artifacts ✓

5. **Move area window to other monitor**
   - Verify bar moves with it ✓
   - No rendering glitches ✓

---

## Summary

**Problem**: Bar rendered in two locations when area is detached
- Bar panel in compositor renders to background
- SDL_BlitSurface also copies bar into area's window
- Result: Double rendering

**Root Cause**: Panel management doesn't account for detached windows

**Solution**: Hide bar panel from compositor when area is detached

**Impact**: Single line addition to visibility management, fixes double-render issue completely.
