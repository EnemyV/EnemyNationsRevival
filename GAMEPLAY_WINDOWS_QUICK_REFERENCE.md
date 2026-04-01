# Gameplay Windows - Quick Reference

## 5-Second Overview

```
┌────────────────────────────────────────────┐
│         GameWindow (SDL2)                  │
│  ┌──────────────────────────────────────┐  │
│  │   SDL2Compositor (Renders Panels)    │  │
│  │  ┌──────────────────────────────────┐│  │
│  │  │   Wallpaper Background (tiled)  ││  │
│  │  ├──────────────────────────────────┤│  │
│  │  │  [CWndWorld]  [CWndArea]  [Bar]  ││  │
│  │  │  (Minimap)    (Terrain)  (Btns)  ││  │
│  │  └──────────────────────────────────┘│  │
│  └──────────────────────────────────────┘  │
│                                            │
│  Each "window" is a SDL2Panel with:       │
│  • Its own SDL_Surface (backbuffer)       │
│  • Title bar (if movable)                 │
│  • Resize borders                         │
│  • Event handling (clicks, drags)         │
└────────────────────────────────────────────┘
```

---

## The 4 Main Windows

### 1. **CWndArea** - Terrain View
```
┌─────────────────────────────┐
│ Terrain View (Movable)      │◄── Title Bar
├─────────────────────────────┤
│  ┌──────────────────────┐   │
│  │  Hex Grid Terrain    │   │
│  │  + Units             │   │
│  │  + Buildings         │   │
│  │  + Selections        │   │
│  └──────────────────────┘   │
└─────────────────────────────┘

Key:
• 24-bit DIB rendering (game logic)
• RenderingAdapter converts DIB → SDL2Panel
• User clicks/drags select units and pan view
• SDL2AreaBar shows 17 context buttons
• Status text at bottom (unit info, build cost, etc.)
```

### 2. **CWndWorld** - Minimap
```
┌──────────────────┐
│ Minimap (Toggle) │
├──────────────────┤
│ ┌──────────────┐ │
│ │ Terrain Grid │ │  • Toggle: Units
│ │ + Resources  │ │  • Toggle: Resources
│ │ + Buildings  │ │  • Toggle: Fog of War
│ │ + Magenta    │ │  • Toggle: Radar/Map Mode
│ │   Frame      │ │
│ └──────────────┘ │
│ [U] [R] [V] [M]  │  ◄── 4 buttons
└──────────────────┘

Key:
• Scaled-down version of full map
• Shows what's visible in main viewport (magenta frame)
• Click to jump viewport to that location
• Resources blink when ready
```

### 3. **CWndBar** - Toolbar
```
┌─────────────────────────────────────────┐
│ [Build] [Units] [Defense] [Special] ... │  Row 1 (38px buttons)
│ Ore: ███░  Wood: ████░  Coal: ██░       │  Row 2 (resource bars)
│ Status: "5 tanks ready, all at base"    │  Row 3 (text status)
└─────────────────────────────────────────┘

Key:
• Bottom of screen (usually fixed)
• 8 quick-access buttons
• 4 resource bars with animation
• 2 lines of status text
• Native SDL2 rendering (not DIB)
```

### 4. **CWndAreaStatic** - Button Bar
```
┌─────────────────┐
│ [B] [U] [D] [S] │
│ [M] [A] [T] [S] │  4 columns × 5 rows
│ [R] [R] [U] [S] │
│ [Ro] [D] [R] [S]│
│ [Mi] [O] [M] [S]│
│ [...] [S] [...] │
└─────────────────┘

Key:
• 17 buttons total
• Context-sensitive (changes based on selected unit)
• Enable/disable based on game state
• Click to trigger game command
• Hovering shows tooltip
```

---

## What Happens When You Click

### In Terrain View (CWndArea)

```
1. User Clicks
   └─→ OS generates mouse event
       └─→ SDL captures event
           └─→ InputHandler::ProcessEvent()
               └─→ SDL2Compositor::RouteEvent()
                   └─→ CWndArea::HandleEvent()
                       └─→ GameWindow::OnLButtonDown()
                           └─→ Select unit or trigger command
                               └─→ Game state updates
                                   └─→ Panel marked dirty
                                       └─→ Next frame: re-render
```

### In Button Bar

```
1. User Clicks Button
   └─→ SDL2AreaBar::HandleEvent()
       └─→ Hit-test button region
           └─→ Invoke callback
               └─→ Game command executes
                   └─→ Button state updates
                       └─→ m_sdlPanel marked dirty
                           └─→ Next frame: re-render button bar
```

---

## The Rendering Process

### From Game Logic to Screen

```
Game Update Phase:
├─ Units move (game state changes)
├─ Buildings constructed
├─ Battles occur
└─ Dirty flags set

Rendering Phase:
├─ CWndArea::ReRender()
│  ├─ Render terrain → DIB
│  └─ Render units → DIB
├─ CWndArea::Draw()
│  ├─ RenderingAdapter::SetTargetPanel(m_sdlPanel)
│  ├─ RenderingAdapter::RenderToPanel(animAtr)
│  └─ Blit DIB pixel data → SDL2Panel surface
└─ (repeat for CWndWorld, CWndBar, etc.)

Compositing Phase:
├─ SDL2Compositor::Composite()
│  ├─ Render wallpaper background
│  ├─ For each panel (in z-order):
│  │  └─ Blit panel surface → window surface
│  └─ SDL_UpdateWindowSurface() ← PRESENT FRAME
└─ Frame visible on screen

Next Frame:
└─ Repeat...
```

---

## Window States

### Position & Visibility

| State | What | How to Change |
|-------|------|---------------|
| **Visible** | Panel appears on screen | `SetVisible(true)` |
| **Hidden** | Panel doesn't render | `SetVisible(false)` |
| **Movable** | User can drag title bar | `SetMovable(true)` |
| **Fixed** | User can't move | `SetMovable(false)` |
| **Resizable** | User can resize edges | `SetResizable(true)` |
| **Fixed Size** | User can't resize | `SetResizable(false)` |
| **Detached** | Has own SDL_Window | `Detach()` |
| **Attached** | In compositor | `Attach()` |
| **On Top** | Always above others | `SetZOrder(999)` |
| **Behind** | Further back | `SetZOrder(-1)` |

### Typical Layouts

**Standard Gameplay**:
```
┌──────────────────────────┐
│      CWndArea            │  Z=10 (Active)
│      (Terrain View)      │
├──┬────────────────────┬──┤
│CI│    CWndWorld       │CB│  Z=0 (Minimap & Bar)
└──┴────────────────────┴──┘
```

**With Detached Minimap**:
```
┌──────────────────────────┐  ┌──────────────┐
│      CWndArea            │  │  CWndWorld   │
│      (Terrain)           │  │  (Detached)  │
│      Z=10                │  │  Z=999       │
├──────────────────────────┤  └──────────────┘
│   CWndBar (Toolbar)      │
│   Z=0                    │
└──────────────────────────┘
```

---

## Key Methods by Component

### GameWindow
```cpp
Create()                 // Initialize SDL window + panels
PollEvents()            // Process SDL events each frame
Render()                // Render UI (buttons, status)
SwapBuffers()           // Present frame to screen
SetGameInfo()           // Update title bar
GetCompositor()         // Access compositor
```

### SDL2Compositor
```cpp
AddPanel()              // Create new panel
RemovePanel()           // Remove panel
FindPanel(name)         // Look up panel by name
Composite()             // Render all panels (called each frame)
InvalidateAll()         // Force full redraw
RouteEvent()            // Pass SDL event to panels
```

### SDL2Panel
```cpp
SetPosition()           // Move panel
SetSize()               // Resize panel
SetVisible()            // Show/hide panel
SetZOrder()             // Change depth order
Render()                // Blit to window surface
HandleEvent()           // Process click/drag/key
HitTest()               // Check if point is inside
Detach()                // Create own SDL_Window
Attach()                // Return to compositor
```

### RenderingAdapter
```cpp
SetTargetPanel()        // Select rendering destination
RenderToPanel()         // Blit DIB to panel
```

---

## File Map

```
enations_latest/src/

Core Infrastructure:
├─ GameWindow.h/cpp         ← Main SDL window
├─ SDL2Compositor.h/cpp      ← Panel management & rendering
└─ SDL2Panel.h/cpp           ← Individual window surface

Rendering:
├─ RenderingAdapter.h/cpp    ← DIB → SDL2 bridge
├─ area.h/cpp                ← Main gameplay window
├─ world.h/cpp               ← Minimap
├─ SDL2Toolbar.h/cpp         ← Status bar
└─ SDL2AreaBar.h/cpp         ← Context buttons

UI Components:
├─ GameWindow.h              ← Title bar (Phase 9.1)
├─ SDLButton / SDLButtonManager ← Button system (Phase 9.2)
└─ StatusBar                 ← Status display (Phase 9.2)

Input:
├─ input/InputHandler.h/cpp  ← Keyboard/mouse
└─ UIButtonListener          ← Event routing
```

---

## Common Tasks

### Add a New Panel

```cpp
// In initialization
SDL2Panel* newPanel = compositor->AddPanel(
    "MyPanel",                  // name
    100,                        // x
    100,                        // y
    300,                        // width
    200,                        // height
    50                          // z-order
);

// Configure it
newPanel->SetMovable(true);
newPanel->SetResizable(true);
newPanel->SetTitle("My Window");
newPanel->SetEventCallback([](SDL_Event& e, int lx, int ly) {
    // Handle clicks/drags/keys
    return false;  // event not consumed
});

// Render each frame
void MyPanel::Render() {
    SDL_Surface* surf = m_panel->GetSurface();
    // Draw on surf...
    m_panel->SetDirty();
}
```

### Render Game Content to Panel

```cpp
// Before rendering
RenderingAdapter::SetTargetPanel(panelPtr);

// Do normal game rendering to DIB
// (existing code unchanged)

// After rendering
RenderingAdapter::RenderToPanel(animAtrPtr);
// Panel is now marked dirty and will be composited
```

### Handle Window Detach

```cpp
// User double-clicks title bar
if (eventIsDoubleClick) {
    if (m_panel->IsDetached()) {
        m_panel->Attach(gameWindow);
    } else {
        m_panel->Detach(gameWindow);
    }
}
```

### Update Status Text

```cpp
gameWindow->GetStatusBar()->SetText(0, "5 tanks selected");
gameWindow->GetStatusBar()->SetResources(500, 100, 50, 200);
// Automatically re-renders next frame
```

---

## Performance Tips

1. **Only Mark Dirty When Changed**
   - `panel->SetDirty()` only after content changes
   - Reduces unnecessary rendering

2. **Reuse Backbuffers**
   - Panel's SDL_Surface persists between frames
   - Only re-blit if marked dirty

3. **Cache Wallpaper**
   - Compositor caches tiled background
   - Only re-render if panel moves/resizes

4. **Single Update Call**
   - `SDL_UpdateWindowSurface()` called once per frame
   - Not once per panel

5. **Detach Heavy Panels**
   - If panel renders lots of content, detach it
   - Reduces compositor overhead
   - Each window has its own buffer

---

## Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| Panel not showing | `SetVisible(false)` | Call `SetVisible(true)` |
| Panel flickering | Not marked dirty | Call `SetDirty()` after render |
| Rendering wrong | Target panel not set | Call `SetTargetPanel()` before rendering |
| Old content visible | DIB not copied to panel | Call `RenderToPanel()` |
| Window title wrong | Game info not set | Call `SetGameInfo()` or `SetPlayerName()` |
| Buttons not responding | Event not routed | Check event callback |
| Minimap misaligned | Coordinate mismatch | Verify viewport → minimap transform |

---

## Architecture Summary

```
        OS Events
            ↓
    InputHandler::ProcessEvent()
            ↓
    SDL2Compositor::RouteEvent()
            ↓
    SDL2Panel::HandleEvent() [reverse Z-order]
            ↓
        Game Logic
            ↓
        Update State
            ↓
    Mark Panels Dirty
            ↓
    Game Rendering (DIB)
            ↓
    RenderingAdapter::RenderToPanel()
            ↓
    SDL2Panel Surface Updated
            ↓
    SDL2Compositor::Composite()
            ↓
    Blit All Panels to Window
            ↓
    SDL_UpdateWindowSurface()
            ↓
        Screen
```

---

## Next Steps

Phase 9.2+ Integration tasks:
1. ✅ **Phase 9.1** - Title bar with player/faction/mode
2. ✅ **Phase 9.2 Core** - 17 buttons + status bar classes
3. ⏳ **Phase 9.2 Integration** - Wire buttons to game commands
4. ⏳ **Phase 9.3+** - Remaining dialogs (file, build, options)
5. ⏳ **Phase 10+** - Full SDL2 menus + dialogs

See GAMEPLAY_WINDOWS_ARCHITECTURE.md for detailed documentation.
