# Enemy Nations - Gameplay Windows Architecture

## Overview

The Enemy Nations SDL2 migration replaces the original MFC+DirectDraw window system with a modern SDL2-based compositor architecture. The key insight is that **each gameplay "window" is now a panel** that composites onto a single SDL2 window surface.

## Master Architecture

### The Component Hierarchy

```
GameWindow (Main SDL2 Window)
│
├─ SDL2Compositor (Compositing Engine)
│  ├─ Background (Tiled Wallpaper)
│  └─ SDL2Panel[] (Z-ordered panels)
│     ├─ CWndArea (Gameplay/Terrain View)
│     ├─ CWndWorld (Minimap)
│     ├─ CWndBar (Toolbar/Status Bar)
│     ├─ CWndAreaStatic (Area Button Bar)
│     └─ Other UI panels...
│
├─ SDL2MainMenu (Main menu overlay)
├─ InputHandler (Keyboard/Mouse)
└─ Title Bar System (Player/Faction/Mode info)
```

### The Rendering Pipeline

```
Each Frame:
1. Game logic updates (units, buildings, state changes)
2. RenderingAdapter::SetTargetPanel() selects a panel
3. Game renders to CAnimAtr::m_dibwnd (DIB surface)
4. RenderingAdapter::RenderToPanel() blits DIB → SDL2Panel surface
5. Compositor iterates panels in Z-order, blits each to window surface
6. SDL_UpdateWindowSurface() presents the frame
```

---

## The Core Components

### 1. GameWindow (SDL2 Window Container)

**File**: `enations_latest/src/GameWindow.h/cpp`

The root SDL2 window that contains everything. Created once per game session.

**Key Methods**:
- `Create()` - Creates SDL window and initializes all subsystems
- `PollEvents()` - Pumps SDL events (called from BaseYield() each frame)
- `SwapBuffers()` - Presents the frame (calls SDL_UpdateWindowSurface)
- `SetGameInfo()` / `SetPlayerName()` / `SetFactionName()` / `SetGameMode()` - Update title bar
- `Render()` - Renders UI (buttons and status bar)

**Owns**:
- SDL_Window* - The native window
- SDL_Renderer* - For UI rendering
- SDL2Compositor - Manages panel compositing
- SDLButtonManager - 17 game buttons
- StatusBar - Resource/unit info display
- InputHandler - Keyboard/mouse input

**Render Loop Integration**:
```cpp
// In BaseYield() mainloop
if (gameWindow) {
    gameWindow->PollEvents();           // Process SDL events
    // ... game logic ...
    gameWindow->Render();               // Render UI
    gameWindow->SwapBuffers();          // Present frame
}
```

---

### 2. SDL2Compositor (Panel Management & Compositing)

**File**: `enations_latest/src/SDL2Compositor.h/cpp`

Manages the compositor pattern: all panels render to their own backbuffer surfaces, then the compositor blits them all to the main window in Z-order.

**Key Concepts**:

**Background Window**
- The root SDL window surface
- Has a tiled wallpaper background (DIB_GOLD from game assets)
- Each frame, the wallpaper is re-rendered if any panel is dirty

**Area View**
- The main gameplay/terrain window (CWndArea)
- Can be detached to its own OS window with SDL_WINDOW_ALWAYS_ON_TOP
- Floats above the background window on other monitors

**Panel Z-Order**
- Lower Z = drawn first (further back)
- Higher Z = drawn last (closer to viewer)
- Example: wallpaper (Z=-1), minimap (Z=0), terrain (Z=1), dialogs (Z=10)

**Key Methods**:
- `LoadWallpaper()` - Load tiled background from game assets
- `AddPanel()` - Create a new panel
- `RemovePanel()` - Remove a panel
- `FindPanel(name)` - Look up a panel by name
- `Composite()` - Render all panels to window (called once per frame)
- `InvalidateAll()` - Force full redraw next frame
- `RouteEvent()` - Pass SDL events to panels in reverse Z-order (top panel first)

**Frame Rendering**:
```cpp
void SDL2Compositor::Composite() {
    // 1. If any panel dirty, render wallpaper background
    if (anyPanelDirty()) {
        RenderWallpaper(windowSurface);
    }

    // 2. Iterate panels in Z-order, blit each to window
    for (auto& panel : m_panels) {  // Sorted by z-order
        if (panel->IsVisible()) {
            panel->Render(windowSurface);
        }
    }

    // 3. Present the frame
    SDL_UpdateWindowSurface(window);
}
```

---

### 3. SDL2Panel (Individual Window)

**File**: `enations_latest/src/SDL2Panel.h/cpp`

A rectangular region with its own SDL_Surface backbuffer. Each MFC "window" (CWndArea, CWndWorld, CWndBar) becomes a panel.

**Key Features**:

**Window Management**
- Movable (draggable title bar) with optional OS window drag
- Resizable with resize border feedback
- Closable with title bar button
- Detachable to own SDL_Window (for multi-monitor support)

**Rendering**
- `m_surface` - Backbuffer SDL_Surface for this panel
- `Render()` - Blit backbuffer to window surface + title bar + resize border
- `SetDirty()` - Mark for re-render next frame

**Event Handling**
- `HandleEvent()` - Process SDL events (hit-test first)
- `HitTest()` - Check if point is inside this panel
- `SetEventCallback()` - Route events to game code

**Title Bar** (if movable)
- Height: 20px
- Close button: 16x16 in top-right
- Resize border: 6px on edges
- Drawn by `RenderTitleBar()`

**Detached Windows** (Multi-monitor support)
- `Detach()` - Create own SDL_Window, becomes independent
- `Attach()` - Destroy own window, return to compositor
- `RenderDetached()` - Render to own window surface

---

### 4. RenderingAdapter (DIB → SDL2 Bridge)

**File**: `enations_latest/src/RenderingAdapter.h/cpp`

**The Critical Bridge**: Converts game rendering from DIB (old MFC/DirectDraw) to SDL2 panels.

**How It Works**:

```
1. Game code creates CAnimAtr (Animation Attribute)
   - Contains m_dibwnd (DIB surface with rendered content)

2. RenderingAdapter::SetTargetPanel(panel)
   - Selects which SDL2Panel to render into

3. Game renders frame to CAnimAtr::m_dibwnd

4. RenderingAdapter::RenderToPanel(animAtr)
   - Blits DIB pixel data to selected panel's SDL_Surface
   - Marks panel dirty
   - Compositor picks it up and renders

5. Alternative: RenderingAdapter::Render()
   - Legacy whole-window rendering (for backward compatibility)
   - Blits to full window surface
```

**Key Methods**:
- `Initialize(gameWindow)` - Set up adapter
- `SetTargetPanel(panel)` - Select rendering target
- `RenderToPanel(animAtr)` - Blit DIB to panel
- `RenderToPanel(animAtr, panel)` - Blit to specific panel
- `BlitDIBToSurface()` - Internal helper for pixel blitting

**Example Usage** (in area.cpp):
```cpp
// Render game world to CWndArea panel
if (m_sdlPanel) {
    RenderingAdapter::SetTargetPanel(m_sdlPanel);
    // ... game renders to CAnimAtr ...
    RenderingAdapter::RenderToPanel(animAtr);
}
```

---

## The Gameplay Windows

### CWndArea - Main Gameplay/Terrain Window

**File**: `enations_latest/src/area.h/cpp`

The core gameplay view showing terrain, units, buildings, and user interaction.

**Structure**:
```
CWndArea (main content area)
├─ m_dibwnd (DIB surface with terrain/units/buildings)
├─ m_sdlPanel (SDL2Panel for compositing)
└─ m_sdl2Bar (native SDL2 button bar - Phase 9.2)

CWndAreaStatic (static elements - buttons, status)
├─ m_btns[17] (17 gameplay buttons: Build, Move, Attack, etc.)
├─ m_wndStat (unit status bar)
└─ m_sdlPanel (SDL2Panel)
└─ m_sdl2Bar (native SDL2 area button bar)
```

**What It Displays**:
1. **Terrain Grid** - Hexagonal map with resources, buildings, terrain features
2. **Units** - Player and enemy units with selection highlights
3. **Buildings** - Factories, refineries, labs, walls, etc.
4. **Overlay Indicators** - Movement paths, build zones, attack ranges
5. **UI Elements** - 17 context buttons, status text, unit info

**Rendering Path**:
```
1. CWndArea::ReRender()
   - Calls game rendering logic
   - Draws terrain, units, buildings to m_dibwnd (DIB surface)

2. CWndArea::Draw()
   - Calls RenderingAdapter::SetTargetPanel(m_sdlPanel)
   - RenderingAdapter::RenderToPanel() blits DIB to panel
   - m_sdlPanel marked dirty

3. Compositor::Composite()
   - Blits m_sdlPanel to window surface
   - Calls SDL2AreaBar::Render() for button overlay
```

**User Interaction**:
- Click → Routes through panel → CWndArea message handlers
- Drag → Pans view or selects units
- Buttons → Trigger build/move/attack commands
- Right-click → Context menu

**Status Text**:
- "Unit Status: 5 tanks selected, all ready"
- "Build: Refinery at (3,4) costs 500 ore"
- Multi-line with importance levels (normal, warning, urgent)

---

### CWndWorld - Minimap

**File**: `enations_latest/src/world.h/cpp`

Overview map showing terrain, resources, units, and building locations.

**Structure**:
```
CWndWorld
├─ m_dibwnd (DIB surface with minimap)
├─ m_sdlPanel (SDL2Panel for compositing)
├─ m_pdibGround0 (terrain at 0,0)
├─ m_pdibBase (terrain + resources + buildings)
├─ m_pdibRadar (radar overlay with magenta frame)
└─ m_pdibButtons (4 toggle buttons: Units, Resources, Visible, Mode)
```

**What It Displays**:
1. **Terrain Grid** - Scaled-down version of full map
2. **Resources** - Ore, wood, coal deposits (toggleable)
3. **Buildings** - All faction buildings (color-coded, toggleable)
4. **Units** - Your units vs enemy units (toggleable)
5. **Radar Frame** - Magenta border showing current viewport in main window
6. **4 Buttons**:
   - Units (toggle visibility)
   - Resources (toggle visibility)
   - Visible (toggle fog of war)
   - Mode (switch radar ↔ map mode)

**Rendering Path**:
```
1. CWndWorld::_NewLocation()
   - Rebuilds m_pdibBase from terrain/units/buildings

2. CWndWorld::ReRender()
   - Composites m_pdibBase, overlays radar frame
   - Renders to m_dibwnd

3. CWndWorld::Draw()
   - Blits m_dibwnd to m_sdlPanel via RenderingAdapter
   - m_sdlPanel marked dirty

4. Compositor::Composite()
   - Blits m_sdlPanel to window surface
```

**User Interaction**:
- Left-click → Jump viewport to that location in main window
- Right-click → Pan minimap
- Buttons → Toggle resource/unit/fog visibility
- Can be detached to separate window

---

### CWndBar - Toolbar (Status Bar)

**File**: `enations_latest/src/toolbar.h/cpp` (old MFC)
**New**: `enations_latest/src/SDL2Toolbar.h/cpp` (SDL2 native)

Bottom status bar showing game information and quick-access buttons.

**Layout** (66px total height):
```
Row 1 (38px):
├─ 8 Quick Buttons (build, research, etc.)
├─ 4 Resource Bars (ore, wood, coal, food)
└─ Clock (game time)

Row 2 (28px):
├─ Status Text Line 1
└─ Status Text Line 2
```

**What It Displays**:
- **Buttons**: Build, Research, Diplomacy, Options, Load, Save, etc.
- **Resources**: Current ore, wood, coal, food with bar graphs
- **Clock**: Game turn number or real-time
- **Status**: Selected unit info, current order, alerts

**Rendering**:
- Native SDL2 (NOT DIB-based like area/world)
- `SDL2Toolbar::Render()` draws directly to panel surface
- TTF font for text
- Sprite sheets for buttons and resource icons

---

### CWndAreaStatic - Area Button Bar

**File**: `enations_latest/src/area.h/cpp` (static child)

17 context buttons in a vertical bar next to the main gameplay window.

**Layout** (4 columns × 5 rows):
```
Column 1      Column 2      Column 3      Column 4
├─ Build      ├─ Units      ├─ Defense    ├─ Special1
├─ Move       ├─ Attack     ├─ Transport  ├─ Special2
├─ Rocket     ├─ Research   ├─ Upgrade    ├─ Special3
├─ Road       ├─ Diplomacy  ├─ Repair     └─ Special4
└─ Mine       ├─ Options    └─ More
              └─ Save
```

**What It Shows**:
- **Current State**: Which buttons are enabled/disabled/highlighted
- **Selected Units**: Context changes based on unit type
- **Building Mode**: Different buttons during construction
- **Tooltips**: Hover shows command name and hotkey

**Rendering**:
- **Old (MFC)**: `CWndAreaStatic::PrintWindow()` → capture to DIB → blit
- **New (SDL2)**: `SDL2AreaBar` draws directly to panel with sprite sheets

**User Interaction**:
- Click → Trigger game command
- Hover → Show tooltip + highlight
- Enable/disable based on game state

---

## The Complete Rendering Flow

### Per-Frame Sequence

```
┌─────────────────────────────────────────────┐
│ BaseYield() - Main Game Loop                │
└─────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────┐
│ GameWindow::PollEvents()                    │
│ - Dispatch SDL events to panels/dialogs     │
└─────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────┐
│ Game Logic Update                           │
│ - Process unit movement, building, combat   │
│ - Update world state                        │
└─────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────┐
│ Panel Rendering (for each visible panel)    │
├─────────────────────────────────────────────┤
│ 1. CWndArea::ReRender()                    │
│    ├─ Render terrain/units/buildings      │
│    └─ to CAnimAtr::m_dibwnd               │
│ 2. CWndArea::Draw()                       │
│    ├─ RenderingAdapter::SetTargetPanel()  │
│    ├─ RenderingAdapter::RenderToPanel()   │
│    └─ Blit DIB → m_sdlPanel               │
│                                            │
│ 3. CWndWorld::ReRender() (same flow)      │
│ 4. CWndBar rendering (if MFC-based)       │
│ 5. Other panels...                        │
└─────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────┐
│ SDL2Compositor::Composite()                 │
│                                             │
│ 1. For each panel in z-order:              │
│    ├─ If dirty/visible: Render wallpaper  │
│    └─ Blit panel surface to window        │
│ 2. For detached panels: render to own win │
│ 3. SDL_UpdateWindowSurface() - PRESENT    │
└─────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────┐
│ Frame Complete                              │
│ (ready for next iteration)                  │
└─────────────────────────────────────────────┘
```

### Input Event Flow

```
SDL_Event from OS
        ↓
GameWindow::PollEvents()
        ↓
InputHandler::ProcessEvent()
        ↓
SDL2Compositor::RouteEvent()
├─ Panels in reverse Z-order (top first)
│  ├─ SDL2Panel::HandleEvent()
│  │  ├─ Hit-test (is point in this panel?)
│  │  └─ Route to m_eventCallback (game code)
│  └─ If consumed: return true
├─ Dialogs (if any)
│  └─ SDL2Dialog::HandleEvent()
└─ Background wallpaper click → game command
```

---

## Key Design Patterns

### 1. **Compositor Pattern**
- Each panel owns its backbuffer (SDL_Surface)
- Compositor blits all panels to single window surface
- Decouples rendering from window management

### 2. **Dirty Flag Tracking**
- Panels only re-render when content changes
- Wallpaper only re-renders if any panel dirty
- Reduces redundant rendering

### 3. **Z-Order Layering**
- Simple integer: lower = further back
- Compositor sorts panels once on add/remove
- Events routed in reverse Z-order (top panel first)

### 4. **Event Consumption**
- Panels can consume events (prevent propagation)
- Useful for dialog overlays + buttons

### 5. **Callback-Based Game Logic**
- Panels don't know about game state
- Game code sets callbacks on panels
- Clean separation of concerns

---

## Window States & Transitions

### Detached Windows
When a panel is detached to its own SDL_Window:

```
ATTACHED (in compositor)
        ↓ Detach()
DETACHED (own SDL_Window with SDL_WINDOW_ALWAYS_ON_TOP)
├─ Rendered independently each frame
├─ Own title bar and window chrome
├─ Can move to other monitors
└─ Still receives events from InputHandler
        ↓ Attach()
ATTACHED (back in compositor)
```

### Title Bar Modes

**CWndArea** (if detached):
- OS provides title bar + window chrome
- User can drag/resize with OS window manager

**CWndArea** (if attached):
- SDL2Panel provides custom title bar
- User can drag panel within main window
- Can close, minimize, etc. via callbacks

---

## Current Migration Status (as of 2026-03-26)

### ✅ Phase 1 - SDL Window Infrastructure
- GameWindow creates SDL window on main thread
- Event polling in BaseYield()
- SDL_QUIT → clean shutdown

### ✅ Phase 8C - Game Rendering Integration
- RenderingAdapter bridges DIB → SDL2Panel
- CWndArea renders terrain/units correctly
- CWndWorld renders minimap
- Compositor blits all panels to window

### ✅ Phase 9.1 - Title Bar System
- Dynamic player/faction/mode in window title
- Real-time updates during gameplay

### ⏳ Phase 9.2 - Full Button System
- SDLButtonManager: 17 buttons
- StatusBar: Resources, units, health, time
- UIButtonListener: Input routing
- Integration with game loop (in progress)

### ⏳ Phase 10+ - Remaining MFC Dialogs
- File dialogs
- Build/unit creation dialogs
- Diplomacy/trade dialogs
- Options dialogs

---

## Performance Considerations

### Dirty Tracking
- Only re-render changed panels
- Wallpaper cached unless panels move/resize
- Single SDL_UpdateWindowSurface() per frame

### Resolution Scaling
- CWndWorld scaled to fit on-screen
- CWndArea fits viewport + decorations
- Can be resized at runtime

### Multi-Monitor Support
- Detachable panels create own SDL_Window
- Each window rendered independently
- No composition overhead for detached windows

---

## Debugging & Development

### Key Files to Watch

| Component | File | Purpose |
|-----------|------|---------|
| Core Window | GameWindow.h/cpp | SDL window creation, title bar, UI |
| Compositing | SDL2Compositor.h/cpp | Panel management, rendering order |
| Panels | SDL2Panel.h/cpp | Individual window surfaces, dragging |
| Rendering Bridge | RenderingAdapter.h/cpp | DIB → SDL2 conversion |
| Area View | area.h/cpp | Main gameplay window, terrain |
| Minimap | world.h/cpp | Overview map, radar |
| Toolbar | SDL2Toolbar.h/cpp | Status bar, quick buttons |
| Area Bar | SDL2AreaBar.h/cpp | Context buttons |

### Enable Debug Output

In `GameWindow.h`, set:
```cpp
#define DEBUG_COMPOSITOR 1      // Panel compositing
#define DEBUG_RENDERING 1       // DIB → SDL2 blitting
#define DEBUG_EVENTS 1          // Event routing
#define DEBUG_DETACH 1          // Window detach/attach
```

### Test Scenarios

1. **Panel Dragging** - Click title bar, drag panel
2. **Detach/Attach** - Double-click title bar to detach
3. **Dirty Tracking** - Move units → should only re-render terrain
4. **Multi-Monitor** - Detach panel, move to second monitor
5. **Button Interaction** - Click game buttons, verify commands execute
6. **Viewport Panning** - Right-click in area view, drag to pan
7. **Minimap Click** - Click minimap to jump viewport

---

## Summary

The SDL2 migration transforms Enemy Nations from rigid MFC windows to a **flexible, composited UI system** where:

- **GameWindow** is the SDL2 container
- **SDL2Compositor** manages rendering order
- **SDL2Panels** are independent rendering surfaces
- **RenderingAdapter** bridges old DIB rendering to new SDL2
- Each **game window** (area, minimap, toolbar) is a independent, **movable, resizable, detachable panel**
- The **rendering pipeline** is: game logic → DIB rendering → RenderingAdapter → SDL2Panel → Compositor → window surface → screen

This architecture provides the foundation for modern UI features: resizable windows, multi-monitor support, independent dialog panels, and clean separation of rendering concerns.
