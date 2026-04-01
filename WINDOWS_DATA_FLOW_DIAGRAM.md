# Enemy Nations - Windows Data Flow & Architecture Diagram

## Complete Architecture Diagram

```
╔════════════════════════════════════════════════════════════════════════════════╗
║                          OPERATING SYSTEM                                      ║
║                     (Windows 11 / Display)                                     ║
╚════════════════════════════════════════════════════════════════════════════════╝
                                    ↑
                        SDL_UpdateWindowSurface()
                                    ↑
╔════════════════════════════════════════════════════════════════════════════════╗
║                         GameWindow (SDL2)                                      ║
║                  [SDL_Window + SDL_Renderer]                                   ║
║                                                                                ║
║  ┌──────────────────────────────────────────────────────────────────────────┐ ║
║  │                    SDL2Compositor                                        │ ║
║  │            [Panel Management & Rendering Order]                         │ ║
║  │                                                                          │ ║
║  │  Background Layer                                                       │ ║
║  │  ┌──────────────────────────────────────────────────────────────────┐  │ ║
║  │  │ Wallpaper (Tiled DIB_GOLD from game assets)                      │  │ ║
║  │  │ Re-rendered when any panel is dirty                             │  │ ║
║  │  └──────────────────────────────────────────────────────────────────┘  │ ║
║  │                                                                          │ ║
║  │  Panel Layer (Z-Ordered Rendering)                                      │ ║
║  │  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐     │ ║
║  │  │   SDL2Panel      │  │   SDL2Panel      │  │   SDL2Panel      │     │ ║
║  │  │   (CWndWorld)    │  │   (CWndArea)     │  │   (CWndBar)      │ ... │ ║
║  │  │                  │  │                  │  │                  │     │ ║
║  │  │ Z-Order: 0       │  │ Z-Order: 10      │  │ Z-Order: 5       │     │ ║
║  │  │ Minimap          │  │ Terrain View     │  │ Toolbar          │     │ ║
║  │  │ (DIB rendered)   │  │ (DIB rendered)   │  │ (Native SDL2)    │     │ ║
║  │  │                  │  │                  │  │                  │     │ ║
║  │  └──────────────────┘  └──────────────────┘  └──────────────────┘     │ ║
║  │                                                                          │ ║
║  │  Each Panel contains:                                                   │ ║
║  │  • SDL_Surface* m_surface (backbuffer)                                 │ ║
║  │  • Position (x, y)                                                      │ ║
║  │  • Size (w, h)                                                          │ ║
║  │  • Title bar (if movable)                                              │ ║
║  │  • Event handler callbacks                                             │ ║
║  │                                                                          │ ║
║  └──────────────────────────────────────────────────────────────────────────┘ ║
║                                                                                ║
║  Other UI Components:                                                        ║
║  ┌──────────────────────┐  ┌──────────────────┐                             ║
║  │  SDL2MainMenu        │  │  InputHandler    │                             ║
║  │  (Main menu overlay) │  │  (Keyboard/Mouse)│                             ║
║  └──────────────────────┘  └──────────────────┘                             ║
║                                                                                ║
║  Title Bar (Phase 9.1)                                                        ║
║  "Enemy Nations - Player Name (Faction) - Single Player"                    ║
║                                                                                ║
╚════════════════════════════════════════════════════════════════════════════════╝
                                    ↑
                   Each frame: Composite all panels
                                    ↑
╔════════════════════════════════════════════════════════════════════════════════╗
║                         Game Engine Loop                                       ║
║                        (BaseYield in mainloop.cpp)                            ║
╚════════════════════════════════════════════════════════════════════════════════╝
```

---

## Per-Frame Execution Flow

```
╔═══════════════════════════════════════════════════════════════════════════════╗
║  BaseYield() - Called each frame from game main loop                          ║
╚═══════════════════════════════════════════════════════════════════════════════╝
                                    ↓
┌───────────────────────────────────────────────────────────────────────────────┐
│  1. EVENT PROCESSING                                                          │
├───────────────────────────────────────────────────────────────────────────────┤
│                                                                               │
│  GameWindow::PollEvents()                                                    │
│      ↓                                                                        │
│  while (SDL_PollEvent(&event)) {                                            │
│      if (event.type == SDL_QUIT) {                                          │
│          PostQuitMessage(0);                                                │
│      } else {                                                                │
│          InputHandler::ProcessEvent(event)                                  │
│              ↓                                                               │
│          SDL2Compositor::RouteEvent(event)                                 │
│              ↓                                                               │
│          Iterate panels in REVERSE Z-order (top-to-bottom)                │
│              ↓                                                               │
│          For each panel:                                                    │
│              ├─ if (!HitTest(event.x, event.y)) continue;                 │
│              ├─ if (EventCallback(event, localX, localY))                 │
│              │    return true;  // Event consumed                         │
│              └─ (continue to next panel if not consumed)                  │
│      }                                                                       │
│  }                                                                            │
│                                                                               │
└───────────────────────────────────────────────────────────────────────────────┘
                                    ↓
┌───────────────────────────────────────────────────────────────────────────────┐
│  2. GAME LOGIC UPDATE                                                         │
├───────────────────────────────────────────────────────────────────────────────┤
│                                                                               │
│  • Process unit movement & combat                                           │
│  • Handle building construction                                             │
│  • Update world state                                                       │
│  • Calculate visibility/fog of war                                          │
│  • AI processing                                                            │
│  ↓                                                                           │
│  → Mark panels dirty (SetDirty())                                          │
│                                                                               │
└───────────────────────────────────────────────────────────────────────────────┘
                                    ↓
┌───────────────────────────────────────────────────────────────────────────────┐
│  3. PANEL RENDERING (Game → DIB → SDL2Panel)                                 │
├───────────────────────────────────────────────────────────────────────────────┤
│                                                                               │
│  For each dirty panel:                                                      │
│                                                                               │
│  CWndArea::ReRender()                                                       │
│      ├─ Render terrain to m_dibwnd (DIB surface)                           │
│      ├─ Render units overlay to m_dibwnd                                   │
│      ├─ Render buildings to m_dibwnd                                       │
│      └─ Render selection indicators to m_dibwnd                            │
│                                                                               │
│  CWndArea::Draw()                                                           │
│      ├─ RenderingAdapter::SetTargetPanel(m_sdlPanel)                       │
│      ├─ RenderingAdapter::RenderToPanel(&m_dibwnd)                         │
│      │   └─ BlitDIBToSurface()                                             │
│      │       └─ Copy pixel data: DIB → SDL2Panel surface                   │
│      └─ m_sdlPanel→SetDirty()                                             │
│                                                                               │
│  CWndWorld::ReRender() (Same pattern)                                       │
│      ├─ Render terrain to m_dibwnd                                         │
│      ├─ Render resources/buildings overlay                                 │
│      └─ Render radar frame                                                 │
│                                                                               │
│  CWndWorld::Draw()                                                          │
│      └─ RenderingAdapter::RenderToPanel(...)                               │
│          └─ m_sdlPanel→SetDirty()                                         │
│                                                                               │
│  CWndBar::Render() [Native SDL2, not DIB-based]                            │
│      └─ SDL2Toolbar::Render()                                              │
│          ├─ Draw buttons (sprite sheets)                                   │
│          ├─ Draw resource bars (TTF text)                                  │
│          ├─ Draw clock                                                      │
│          └─ Draw status text (TTF rendering)                               │
│                                                                               │
│  Other panels... (same pattern)                                            │
│                                                                               │
└───────────────────────────────────────────────────────────────────────────────┘
                                    ↓
┌───────────────────────────────────────────────────────────────────────────────┐
│  4. COMPOSITION & PRESENTATION                                               │
├───────────────────────────────────────────────────────────────────────────────┤
│                                                                               │
│  SDL2Compositor::Composite()                                                │
│      ↓                                                                       │
│  // 1. Render background if any panel dirty                                │
│  if (m_backgroundDirty || anyPanelDirty) {                                 │
│      RenderWallpaper(windowSurface);                                       │
│      m_backgroundDirty = false;                                            │
│  }                                                                            │
│      ↓                                                                       │
│  // 2. Iterate panels in Z-order (ascending)                               │
│  for (auto& panel : m_panels) {  // Sorted: Z=0, Z=5, Z=10, Z=20...      │
│      if (!panel→IsVisible()) continue;                                     │
│      panel→Render(windowSurface);                                          │
│          ├─ Blit panel→m_surface to windowSurface at (m_x, m_y)          │
│          ├─ If movable: draw title bar                                    │
│          ├─ If resizable: draw resize borders                            │
│          └─ panel→ClearDirty()                                           │
│  }                                                                            │
│      ↓                                                                       │
│  // 3. Render detached panels to their own windows                         │
│  for (auto& panel : m_panels) {                                            │
│      if (panel→IsDetached()) {                                             │
│          panel→RenderDetached();                                           │
│      }                                                                       │
│  }                                                                            │
│      ↓                                                                       │
│  // 4. Present to screen (FINAL STEP)                                      │
│  SDL_UpdateWindowSurface(m_window);                                        │
│                                                                               │
│  → Now visible on monitor                                                   │
│                                                                               │
└───────────────────────────────────────────────────────────────────────────────┘
                                    ↓
╔═══════════════════════════════════════════════════════════════════════════════╗
║  Frame Complete - Repeat next iteration                                       ║
╚═══════════════════════════════════════════════════════════════════════════════╝
```

---

## Input Event Routing

```
┌──────────────────────────────────────────────────────────────────────────────┐
│  Operating System                                                            │
│  ├─ User clicks at screen position (100, 200)                              │
│  └─ OS delivers mouse event to SDL_Window                                  │
└──────────────────────────────────────────────────────────────────────────────┘
                                    ↓
                        SDL_PollEvent(&event)
                        event.type = SDL_MOUSEBUTTONDOWN
                        event.button.x = 100
                        event.button.y = 200
                                    ↓
┌──────────────────────────────────────────────────────────────────────────────┐
│  InputHandler::ProcessEvent(event)                                          │
│  • Convert SDL event to game format                                         │
│  • Translate SDL keycodes to game keycodes                                  │
│  ↓                                                                           │
│  SDL2Compositor::RouteEvent(event)                                         │
│  ├─ event.button.x = 100                                                   │
│  └─ event.button.y = 200                                                   │
└──────────────────────────────────────────────────────────────────────────────┘
                                    ↓
        Iterate panels in REVERSE Z-order (top panel first)
                    [Z=20] [Z=10] [Z=5] [Z=0] [Z=-1]
                                    ↓
┌──────────────────────────────────────────────────────────────────────────────┐
│  Panel Z=20 (Dialog, if any)                                                │
│  ├─ HitTest(100, 200)?                                                      │
│  │  ├─ No → Skip this panel                                                │
│  │  └─ Yes → Panel occupies (0,0) to (400,300)                            │
│  ├─ Convert screen coords to local coords                                  │
│  │  ├─ screenX = 100 → localX = 100 - panelX = 100 - 0 = 100           │
│  │  └─ screenY = 200 → localY = 200 - panelY = 200 - 0 = 200           │
│  └─ HandleEvent(event, 100, 200)                                          │
│     ├─ Check if click is on close button                                 │
│     ├─ Check if click is on resize border                                │
│     ├─ Check if click is on draggable area                               │
│     ├─ Invoke m_eventCallback(event, 100, 200)                           │
│     │  └─ Game code handles click (e.g., click button)                   │
│     ├─ Return true (event consumed - stop routing)                       │
│     └─ Back to Compositor: "Event handled" → Done                        │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## Panel Rendering Detail - DIB to SDL2

```
CWndArea Rendering Pipeline
═══════════════════════════════════════════════════════════════════════════════

Step 1: Game Rendering to DIB
┌──────────────────────────────────────────────────────────────────────────────┐
│  CAnimAtr::m_dibwnd (DIB Surface)                                           │
│  • 24-bit color: RGB format                                                 │
│  • Created once, reused each frame                                          │
│                                                                              │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  Pixel Buffer (e.g., 1024x768x3 = 2.36 MB)                             │ │
│  │  ╔═════════════════════════════════════════╗                           │ │
│  │  ║ Terrain tiles (hex grid)                ║                           │ │
│  │  ║ + Unit sprites                          ║                           │ │
│  │  ║ + Building sprites                      ║                           │ │
│  │  ║ + Overlay indicators                    ║                           │ │
│  │  ╚═════════════════════════════════════════╝                           │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│                                                                              │
│  Game code renders into this each frame (DirectDraw legacy flow)           │
└──────────────────────────────────────────────────────────────────────────────┘

Step 2: RenderingAdapter Blits DIB to SDL2Panel
┌──────────────────────────────────────────────────────────────────────────────┐
│  RenderingAdapter::RenderToPanel(animAtr)                                   │
│  ├─ Get target panel (set by SetTargetPanel())                             │
│  ├─ Get source DIB (CAnimAtr::m_dibwnd)                                    │
│  ├─ Get destination: panel→GetSurface()                                    │
│  └─ BlitDIBToSurface(source_dib, dest_surface, 0, 0)                       │
│      ├─ For each pixel in source:                                         │
│      │  ├─ Read RGB triplet from DIB                                      │
│      │  ├─ Write to SDL_Surface at same position                          │
│      │  └─ (if SDL2 uses same pixel format, can memcpy chunks)           │
│      └─ Blit complete                                                      │
│                                                                              │
│  Result: SDL2Panel now contains copy of DIB                               │
│  ↓                                                                           │
│  panel→SetDirty()  // Mark for compositor                                 │
└──────────────────────────────────────────────────────────────────────────────┘

Step 3: Compositor Blits SDL2Panel to Window
┌──────────────────────────────────────────────────────────────────────────────┐
│  SDL2Compositor::Composite()                                                │
│  ├─ For each panel in z-order:                                            │
│  │  ├─ if (panel→IsDirty()) {                                             │
│  │  │  ├─ Get source: panel→GetSurface()                                  │
│  │  │  ├─ Get destination: windowSurface                                  │
│  │  │  ├─ SDL_Rect srcRect = {0, 0, w, h};                               │
│  │  │  ├─ SDL_Rect dstRect = {panel→m_x, panel→m_y, w, h};              │
│  │  │  ├─ SDL_BlitSurface(panelSurface, &srcRect,                        │
│  │  │  │                   windowSurface, &dstRect);                      │
│  │  │  ├─ RenderTitleBar() if movable                                    │
│  │  │  └─ panel→ClearDirty()                                             │
│  │  └─ }                                                                   │
│  └─                                                                         │
│  SDL_UpdateWindowSurface(window)  // PRESENT TO SCREEN                    │
└──────────────────────────────────────────────────────────────────────────────┘

Result: Frame now visible on monitor
```

---

## Memory Layout - Single Frame

```
RAM Memory Organization
═══════════════════════════════════════════════════════════════════════════════

Persistent Surfaces (allocated once, reused):
┌─────────────────────────────────────────┐
│ CAnimAtr::m_dibwnd (DIB)                │  ~2.36 MB (1024x768x3)
│ Game terrain/units/buildings rendering  │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│ SDL2Panel::m_surface (CWndArea)         │  ~2.36 MB (1024x768)
│ Copy of DIB after RenderToPanel()      │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│ SDL2Panel::m_surface (CWndWorld)        │  ~256 KB (256x256)
│ Minimap rendering                       │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│ SDL2Panel::m_surface (CWndBar)          │  ~128 KB (800x66)
│ Toolbar/status bar rendering           │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│ windowSurface (SDL window backing)      │  ~3 MB (1280x1024)
│ Composite of all panels                 │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│ Wallpaper background (tiled)            │  ~128 KB (cached)
│ DIB_GOLD from game assets               │
└─────────────────────────────────────────┘

Per-Frame Operations (temporary):
┌─────────────────────────────────────────┐
│ PixelBuffer temp (blitting work area)   │  Variable
│ Used during SDL_BlitSurface()          │
└─────────────────────────────────────────┘

Total VRAM: ~8-10 MB (typical)
Performance: Single memcpy per dirty panel per frame
```

---

## State Synchronization

```
Game State Updates
═══════════════════════════════════════════════════════════════════════════════

1. Unit Moves
   ├─ Game updates CUnit position
   ├─ Sets dirty flag: "terrain changed"
   ├─ Next frame: CWndArea::ReRender() called
   ├─ Terrain re-rendered with unit at new position
   ├─ DIB updated
   ├─ RenderingAdapter blits to SDL2Panel
   └─ Panel rendered to window → user sees change

2. Building Constructed
   ├─ Game updates CBuilding in world
   ├─ Sets dirty flag
   ├─ CWndArea and CWndWorld both marked dirty
   ├─ Both panels re-render
   ├─ Both DIBs updated
   ├─ Both panels composited to window
   └─ User sees building in both views

3. Status Text Changed
   ├─ Game calls CWndArea::SetStatusText("5 tanks ready")
   ├─ Text stored in m_wndStat
   ├─ CWndAreaStatic marked dirty
   ├─ Status bar re-rendered
   ├─ Panel composited to window
   └─ User sees new status text

4. Title Bar Updated
   ├─ Game calls gameWindow→SetPlayerName("John")
   ├─ Title updated: "Enemy Nations - John (Terrans) - Single Player"
   ├─ SDL_SetWindowTitle() called
   ├─ OS updates window title
   └─ User sees change immediately (not frame-synced)
```

---

## Event Consumption Example

```
User clicks at screen position (300, 150)
┌─────────────────────────────────────────────────────────────────────────────┐
│  SDL2Compositor::RouteEvent(event)                                         │
│  event.button.x = 300                                                      │
│  event.button.y = 150                                                      │
│                                                                             │
│  Panels (in reverse Z-order): [Z=20] [Z=10] [Z=5] [Z=0]                  │
└─────────────────────────────────────────────────────────────────────────────┘

Panel Z=20 (Dialog if present):
  ├─ HitTest(300, 150)?
  └─ No (dialog at 0,0 to 400,100, point is at 300,150 which is outside)
     └─ Skip this panel

Panel Z=10 (CWndArea - Terrain):
  ├─ HitTest(300, 150)?
  ├─ Yes (terrain window at 50,30 to 950,700)
  ├─ localX = 300 - 50 = 250
  ├─ localY = 150 - 30 = 120
  ├─ HandleEvent(event, 250, 120)
  │  ├─ Not on title bar (title bar is at y=-20 to y=0)
  │  ├─ Not on resize border
  │  ├─ m_eventCallback called:
  │  │  └─ CWndArea::OnLButtonDown(250, 120)
  │  │     ├─ Convert to map coordinates (click_hex = (15, 8))
  │  │     ├─ Check if unit at that hex
  │  │     ├─ If yes: select unit
  │  │     ├─ If no: clear selection
  │  │     └─ Mark panel dirty (will re-render next frame)
  │  └─ Return true (event consumed)
  └─ Back to Compositor: Event was consumed, stop routing

Panels Z=5, Z=0: Not checked (event already consumed)

Result: Unit selected, will be redrawn next frame with selection highlight
```

---

## Window Detach/Attach Flow

```
Attached State:
┌──────────────────────────────────────────┐
│ SDL2Compositor                           │
│ ├─ SDL2Panel (CWndArea)                │
│ │  ├─ m_ownWindow = nullptr            │
│ │  ├─ m_surface = backbuffer           │
│ │  └─ Rendered by Compositor           │
│ ├─ SDL2Panel (CWndWorld)               │
│ └─ SDL2Panel (CWndBar)                 │
│                                         │
│ Single SDL_UpdateWindowSurface() call   │
└──────────────────────────────────────────┘
                    ↓ User double-clicks title bar
                 panel→Detach()
                    ↓
Create own SDL_Window:
├─ SDL_CreateWindow() with SDL_WINDOW_ALWAYS_ON_TOP
├─ m_ownWindow = new window pointer
├─ m_ownWindowID = window ID
└─ Create new SDL_Surface for new window

Detached State:
┌──────────────────────────────────────────┐
│ SDL2Compositor                           │
│ ├─ SDL2Panel (CWndArea) [DETACHED]     │
│ │  ├─ m_ownWindow = SDL_Window*        │
│ │  ├─ m_surface = still exists         │
│ │  ├─ Rendered to own window each frame│
│ │  └─ Has own title bar + window chrome│
│ ├─ SDL2Panel (CWndWorld) [attached]     │
│ └─ SDL2Panel (CWndBar) [attached]      │
│                                         │
│ Each detached window has own:          │
│ • SDL_UpdateWindowSurface() call       │
│ • Title bar (OS-provided)              │
│ • Window chrome (OS-provided)          │
│ • Event routing (still through main)   │
└──────────────────────────────────────────┘
                    ↓ User double-clicks title bar again
                 panel→Attach()
                    ↓
Destroy own SDL_Window:
├─ SDL_DestroyWindow(m_ownWindow)
├─ m_ownWindow = nullptr
└─ Back to Compositor compositing

Result: Panel back in Compositor, rendered with other panels
```

---

## Summary Table

| Component | Purpose | Type | Rendered |
|-----------|---------|------|----------|
| **GameWindow** | SDL2 window container | Owns all | OS window |
| **SDL2Compositor** | Panel management | Manages | windowSurface |
| **SDL2Panel** | Individual window surface | Container | m_surface |
| **CAnimAtr::m_dibwnd** | Game DIB rendering | DIB | DIB pixels |
| **RenderingAdapter** | DIB→SDL2 bridge | Converter | Panel surface |
| **CWndArea** | Terrain view game code | Game Logic | DIB → Panel |
| **CWndWorld** | Minimap game code | Game Logic | DIB → Panel |
| **CWndBar / SDL2Toolbar** | Toolbar/status | Native SDL2 | Panel directly |
| **Wallpaper** | Background texture | Tiled | windowSurface |

---

## Performance Characteristics

```
Per-Frame Cost Breakdown
═══════════════════════════════════════════════════════════════════════════════

Event Processing:           < 1 ms (few events per frame)
Game Logic:                 5-50 ms (variable, depends on game state)

Panel Rendering:
  CWndArea DIB rendering:   2-10 ms (terrain complex)
  RenderingAdapter blit:    2-5 ms (memcpy DIB → panel)
  CWndWorld rendering:      1-2 ms (smaller, simpler)
  CWndBar rendering:        < 1 ms (native SDL2)

Compositing:
  Wallpaper rendering:      1-2 ms (cached)
  Panel blitting:           2-5 ms (each panel)
  SDL_UpdateWindowSurface:  1-10 ms (driver dependent)

Total per frame:            15-80 ms (60 FPS target = ~16.7 ms budget)

Optimization Opportunity:
  If any panel not dirty, skip its rendering entirely
  Example: Minimap not changed last frame → skip CWndWorld rendering
```

---

## Conclusion

The SDL2 window system uses a **compositor architecture** where:

1. **Each game window is a panel** with its own rendering surface
2. **Game code renders to DIB** (unchanged from DirectDraw era)
3. **RenderingAdapter bridges** DIB pixel data to SDL2 surfaces
4. **Compositor composites** all panels to the main window surface each frame
5. **Input is routed top-down** through the panel stack (highest Z-order first)
6. **Dirty tracking** minimizes redundant rendering
7. **Detachable panels** can escape to their own SDL_Windows for multi-monitor support

This design provides the flexibility of modern UI frameworks while maintaining compatibility with the existing game rendering pipeline.
