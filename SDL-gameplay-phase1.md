# Phase 1: SDL2 Backplate and Compositing Framework

## Goal
Replace CWndMain's role as the fullscreen "backplate" window with an SDL2 equivalent. Build a compositing system that renders sub-regions (panels) onto the single SDL2 window surface.

## What to Build

### SDL2Panel
Lightweight class representing a rectangular region of the SDL2 window surface.
- Position and size (x, y, w, h) relative to the window
- Own SDL_Surface* backbuffer
- `Render(SDL_Surface* windowSurface)` blits backbuffer to window at position
- `HandleEvent(SDL_Event&)` with hit-testing; routes events to panel
- Visibility flag, z-order index
- `SetDirty()` / `IsDirty()` for selective recomposition

### SDL2Compositor
Owned by GameWindow; manages an ordered list of SDL2Panel* objects. Each frame:
1. Renders the wallpaper/background texture first (the "backplate")
2. Iterates panels in z-order, calling Render() for dirty panels
3. Calls SDL_UpdateWindowSurface() once at the end

### Backplate Background
- Load CWndMain's wallpaper CDIB (theBitmaps.GetByIndex(DIB_GOLD)) and tile it as background
- This replaces CWndMain::OnPaint() / DrawScreen()
- Must work during all game states: main menu, game creation, gameplay

### RenderingAdapter Changes
- Remove the current "mirror entire window" behavior
- Add SetTargetPanel(SDL2Panel*) so CWndArea rendering goes to a specific panel
- Keep the DIB-to-SDL-surface conversion logic

### Game Loop Integration
In _RenderScreens() (mainloop.cpp:452), after the existing:
```
for each CWndAnim: ReRender()
for each CWndAnim: Draw()
ClearInvalidated()
```
Add: SDL2Compositor composites all panels to window surface; calls SDL_UpdateWindowSurface(). This replaces CDIBWnd::Update()'s GDI BitBlt.

## Key Risks
- SDL_GetWindowSurface() and SDL_Renderer cannot coexist; stay with surface-based rendering
- The game's global rendering state (xiZoom, xiDir, xpdibwnd) is set per-CAnimAtr context; the panel system must not interfere with when these are set
- CDIBWnd::Update() currently does GDI BitBlt to MFC windows; must be redirected to panel surfaces without breaking the rendering sequence

## Reuse
- GameWindow (extend, don't replace)
- RenderingAdapter (modify target)
- SDL2MainMenu::CreateSurfaceFromDIB() pattern for CDIB conversion

## Testable Milestone
When entering gameplay, the SDL2 window stays visible (no Hide()), renders the tiled wallpaper background, and the game's CAnimAtr output appears composited at the correct position within the SDL2 window. MFC windows can still exist alongside for comparison during development.

## Complexity: LARGE
