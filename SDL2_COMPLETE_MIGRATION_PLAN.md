# Enemy Nations: Complete SDL2 Migration Plan

## Current State Assessment

### What's Done
- **Audio**: Fully migrated from MSS32 to SDL2_mixer (tested, working)
- **SDL2 Window**: Basic SDL2 window created via `GameWindow::Create()` in `lastplnt.cpp:1498`
- **DIB Passthrough**: `RenderingAdapter::Render()` can blit the game's DIB surface to the SDL window (Phase 8C)
- **Title Bar**: Dynamic title with player/faction/mode info (Phase 9.1)
- **UI Framework**: `SDLButtonManager`, `StatusBar`, `InputHandler` classes exist but are **not wired in**
- **Rendering Subsystems**: 25+ files in `rendering/` and `input/` directories (sprites, terrain, text, fog, selection, damage, dialogs, viewports, asset loading) — all designed but **partially implemented and untested**

### What's NOT Done
- **Main loop is still MFC** — `CConquerApp::Run()` uses `PeekMessage`/`DispatchMessage`
- **All windows are MFC** — `CWndMain`, `CWndArea`, `CWndWorld`, `CWndBar`, `CDlgMain`, all dialogs
- **Input routing** — SDL events go nowhere; game input is MFC message handlers
- **Game rendering still goes through DIB/WinG** — the `_RenderScreens()` loop calls `ReRender()`/`Draw()` on MFC `CWndAnim` objects
- **No menu system in SDL2** — `CDlgMain` (11 buttons) is pure MFC
- **No dialog system in SDL2** — all game dialogs are MFC `CDialog` subclasses

### Architecture Summary
```
CURRENT:
  CConquerApp::Run() [MFC message loop]
    → PeekMessage/DispatchMessage
    → GraphicsEnginePump()
      → _RenderScreens()
        → CWndArea::ReRender() → DIB rendering
        → CWndWorld::ReRender() → DIB rendering
        → CWndArea::Draw() → BitBlt to screen
        → CWndWorld::Draw() → BitBlt to screen
    → GameWindow (SDL2) sits idle, DIB passthrough only

TARGET:
  SDL2 Main Loop
    → SDL_PollEvent() for all input
    → Game logic tick (24 FPS)
    → SDL2 rendering (game view, minimap, UI, dialogs)
    → SDL_RenderPresent() or SwapBuffers
```

---

## Migration Strategy: Incremental Replacement

The key insight: **we cannot do a big-bang replacement**. The game has 77KB of mainloop code, complex state machines, network play, AI, and dozens of interacting systems. We must replace MFC piece by piece while keeping the game running at every step.

**Core principle**: At each phase, the game must compile and run. We replace one MFC component at a time, routing its functionality through SDL2.

---

## Phase 1: Unified SDL2 Window (Replace CWndMain)

**Goal**: One SDL2 window owns the entire screen. MFC windows render into it instead of to their own HWNDs.

### 1.1 — SDL2 Takes Over the Main Window
**Current**: `CWndMain::Create()` creates a fullscreen MFC popup window. `GameWindow::Create()` creates a second SDL window.

**Change**:
- Remove the separate SDL window creation
- Instead, create the SDL window FIRST, then embed MFC rendering into it
- OR: Create the SDL window and make it the primary display, with MFC windows hidden

**Implementation**:
```
File: enations_latest/src/lastplnt.cpp
- Move GameWindow::Create() to BEFORE CWndMain::Create()
- Make GameWindow fullscreen (or match display resolution)
- CWndMain becomes a hidden coordinator (no visible HWND)
```

### 1.2 — Program State Machine in SDL2
**Current**: `CWndMain::SetProgPos()` manages states: loading → demo_license → movie → retail_license → playing → game_end → exiting

**Change**: Move state machine to `GameWindow`:
```cpp
// GameWindow.h additions:
enum class ProgramState {
    Loading,      // Show loading screen
    MainMenu,     // Main menu (replaces CDlgMain)
    Playing,      // Active gameplay
    GameEnd,      // End screens
    Exiting       // Shutdown
};

void SetProgramState(ProgramState state);
ProgramState GetProgramState() const;
```

### 1.3 — Event Loop Integration
**Current**: `CConquerApp::Run()` uses a Windows message loop with `PeekMessage`.

**Change**: Add SDL event polling into the existing loop (not replace it yet — MFC dialogs still need the Windows message pump):
```cpp
// In CConquerApp::Run() or GraphicsEnginePump():
SDL_Event event;
while (SDL_PollEvent(&event)) {
    m_gameWindow->HandleEvent(event);  // Route to active state
}
// Keep existing PeekMessage loop for remaining MFC components
```

### Deliverables
- [ ] GameWindow created before CWndMain
- [ ] ProgramState enum and state machine in GameWindow
- [ ] SDL event polling added to main loop
- [ ] CWndMain hidden (coordinator only, no visible window)
- [ ] Game still runs with DIB passthrough to SDL window

---

## Phase 2: Main Menu (Replace CDlgMain)

**Goal**: Render the main menu entirely in SDL2. This is the first screen the user sees.

### 2.1 — Main Menu Screen
**Current**: `CDlgMain` is an MFC dialog with 11 owner-drawn buttons, a wallpaper background, and bitmap button rendering.

**What CDlgMain does** (from `lastplnt.cpp:1891+`):
- Background: Loads wallpaper bitmap from game data files
- 11 buttons: Scenario, Single Player, Create Game, Join Game, Load Game, Multi Load, Credits, Intro, Options, Minimize, Exit
- Owner-drawn buttons with DIB rendering
- Palette management

**SDL2 Implementation**:
```cpp
// New class: SDL2MainMenu
class SDL2MainMenu {
    SDL_Texture* m_background;     // Wallpaper texture
    std::vector<MenuButton> m_buttons;  // 11 menu buttons

    void LoadAssets();      // Load wallpaper + button images from game data
    void HandleEvent(SDL_Event& event);  // Mouse click → button action
    void Render(SDL_Renderer* renderer); // Draw background + buttons
};
```

### 2.2 — Button Assets
The original buttons use `CBmButton` (bitmap buttons) from the Wind22 library. Button sprites are in the game's data files.

**Approach**:
- Extract button images from the existing sprite/DIB system
- Convert to SDL_Texture at load time
- Implement hover/press/disabled states matching the originals

### 2.3 — Menu Actions
Each button triggers a game action:
| Button | Action | Notes |
|--------|--------|-------|
| Scenario | `OnMainScenario()` | Opens scenario selection |
| Single Player | `OnMainSingle()` | Starts single player setup |
| Create Game | `OnMainCreate()` | Network game creation |
| Join Game | `OnMainJoin()` | Join network game |
| Load Game | `OnMainLoad()` | Load saved game |
| Multi Load | `OnMainMultiLoad()` | Load multiplayer save |
| Credits | `OnMainCredits()` | Show credits screen |
| Intro | `OnMainIntro()` | Play intro movie |
| Options | `OnMainOptions()` | Game options dialog |
| Minimize | Minimize window | `ShowWindow(SW_MINIMIZE)` |
| Exit | `OnMainExit()` | Quit game |

### 2.4 — State Transition
When a menu button is clicked (e.g., "Single Player"):
1. SDL2MainMenu calls into the existing game logic (CConquerApp methods)
2. Game logic sets up the game session
3. GameWindow transitions to `ProgramState::Playing`
4. CDlgMain is never created

### Deliverables
- [ ] SDL2MainMenu class renders wallpaper background
- [ ] 11 menu buttons with click detection
- [ ] Hover highlighting and press states
- [ ] Each button triggers the correct CConquerApp method
- [ ] CDlgMain code path bypassed entirely
- [ ] Menu → Game transition works

---

## Phase 3: Game View Rendering (Replace CWndArea DIB path)

**Goal**: Route the existing DIB-based game rendering through SDL2 instead of BitBlt-to-HWND.

This is the biggest phase. The game view has 8 rendering layers (terrain, buildings, vehicles, bridges, roads, effects, UI, selection).

### 3.1 — Keep DIB Rendering, Display via SDL2
**Strategy**: Don't rewrite the rendering engine yet. The game already renders perfectly to DIB buffers. Just display those DIBs on the SDL window instead of using BitBlt/WinG.

This is essentially what Phase 8C's `RenderingAdapter::Render()` already does, but we need to:
- Make it the ONLY rendering path (remove HWND display)
- Handle the area window's portion of the screen correctly
- Support the correct screen layout (area + minimap + toolbar)

**Implementation**:
```cpp
// In _RenderScreens(), after ReRender()/Draw() loops:
// Instead of each CWndAnim::Draw() calling BitBlt to its HWND,
// route all DIBs to RenderingAdapter which composites them onto the SDL window

void CWndArea::Draw() {
    // Existing: renders to m_dibwnd
    // Change: instead of BitBlt to HWND, hand DIB to RenderingAdapter
    RenderingAdapter::BlitAreaDIB(m_aa.m_dibwnd, screenRect);
}
```

### 3.2 — Screen Layout Compositing
The SDL window must composite multiple game views:
```
┌──────────────────────────────┐
│                              │
│   CWndArea DIB               │
│   (main game viewport)       │
│                              │
│            ┌────────┐        │
│            │Minimap │        │
│            │CWndWorld│       │
│            └────────┘        │
├──────────────────────────────┤
│ CWndBar (toolbar, 66px)      │
└──────────────────────────────┘
```

**RenderingAdapter must**:
- Accept DIBs from CWndArea, CWndWorld, CWndBar
- Composite them at correct positions on the SDL surface
- Call `SDL_UpdateWindowSurface()` once per frame

### 3.3 — Remove HWND-based Display
Once DIBs render through SDL:
- Remove `CDIBWnd::Draw()` BitBlt calls
- Remove WinG dependency
- CWndArea/CWndWorld/CWndBar become headless (no visible HWND)

### Deliverables
- [ ] RenderingAdapter composites CWndArea DIB onto SDL surface
- [ ] RenderingAdapter composites CWndWorld DIB onto SDL surface
- [ ] RenderingAdapter composites CWndBar DIB onto SDL surface
- [ ] Correct screen layout (area fills main space, minimap overlay, toolbar bottom)
- [ ] BitBlt/WinG calls removed
- [ ] Game renders correctly through SDL2

---

## Phase 4: Input System (Replace MFC Message Handlers)

**Goal**: Route all keyboard and mouse input from SDL2 events to the game logic.

### 4.1 — SDL Event → Game Input Translation
**Current MFC handlers** (in CWndArea, CWndWorld, CWndBar):
- `OnLButtonDown`, `OnLButtonUp`, `OnLButtonDblClk`
- `OnRButtonDown`, `OnRButtonUp`
- `OnMouseMove`, `OnMouseWheel`
- `OnKeyDown`, `OnKeyUp`
- `OnSetCursor`

**SDL2 Approach**:
```cpp
void GameWindow::HandleEvent(SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            // Determine which game region was clicked based on coordinates
            if (InAreaRect(x, y))
                RouteToArea(event);  // Calls CWndArea's logic
            else if (InWorldRect(x, y))
                RouteToWorld(event); // Calls CWndWorld's logic
            else if (InBarRect(x, y))
                RouteToBar(event);   // Calls CWndBar's logic
            break;
        case SDL_KEYDOWN:
            RouteKeyboard(event);    // Accelerators + game shortcuts
            break;
    }
}
```

### 4.2 — Coordinate Translation
MFC handlers receive coordinates relative to their HWND. SDL gives coordinates relative to the full window. We need a translation layer:
```cpp
// Convert SDL window coords to CWndArea-local coords
CPoint ScreenToAreaLocal(int sdlX, int sdlY) {
    return CPoint(sdlX - areaRect.left, sdlY - areaRect.top);
}
```

### 4.3 — Cursor Management
The game uses context-sensitive cursors (move, attack, build road, etc.). Replace `SetCursor(HCURSOR)` with `SDL_SetCursor()`:
```cpp
// Create SDL cursors from the original cursor resources
// Map game cursor states to SDL cursors
```

### 4.4 — Keyboard Accelerators
The game uses `IDR_ACCEL` accelerator table. Map these to SDL keycodes:
- Ctrl+S → Save
- F1 → Help
- Space → Pause
- +/- → Zoom in/out
- etc.

### Deliverables
- [ ] SDL mouse events routed to CWndArea/CWndWorld/CWndBar logic
- [ ] Coordinate translation (SDL window → game window local)
- [ ] SDL keyboard events mapped to game accelerators
- [ ] Context-sensitive cursors via SDL
- [ ] MFC message handlers bypassed

---

## Phase 5: Toolbar and Status Bar (Replace CWndBar)

**Goal**: Render the toolbar natively in SDL2 instead of as a DIB.

### 5.1 — Toolbar Layout
**CWndBar** contains:
- 8 navigation buttons: Area, World, Chat, Vehicles, Buildings, Relations, Science, File
- 4 resource status bars: Gas, Power, People, Food
- Time display
- 2 text message lines
- Total height: 66px (38px buttons + 28px status text)

### 5.2 — SDL2 Toolbar
Use the existing `SDLButtonManager` and `StatusBar` classes (already created but not wired):
```cpp
class SDL2Toolbar {
    SDLButtonManager m_buttons;  // 8 toolbar buttons
    StatusBar m_statusBar;       // Resource display

    void Render(SDL_Renderer* renderer, int y, int width);
    void HandleClick(int x, int y);
};
```

### 5.3 — Connect to Game State
The toolbar displays live game data:
- Resources from `theGame` global
- Time from game clock
- Messages from the event system

### Deliverables
- [ ] 8 toolbar buttons rendered in SDL2
- [ ] Resource status bars showing live game data
- [ ] Time display
- [ ] Message text lines
- [ ] CWndBar MFC window removed

---

## Phase 6: Minimap (Replace CWndWorld)

**Goal**: Render the minimap/radar view in SDL2.

### 6.1 — Minimap Rendering
**CWndWorld** renders:
- Full world map as colored pixels (terrain → color)
- Resource deposits (blinking colored dots)
- Building locations (player colors)
- Unit positions (radar mode)
- Visibility overlay
- 4 toggle buttons: Resources, Visible, Mine, Units

### 6.2 — SDL2 Minimap
The minimap is essentially a bitmap that gets updated:
```cpp
class SDL2Minimap {
    SDL_Texture* m_mapTexture;     // The minimap image
    SDL_Rect m_viewportIndicator;  // Rectangle showing current view

    void UpdateMap();              // Regenerate from game state
    void Render(SDL_Renderer* renderer, SDL_Rect destRect);
    void HandleClick(int x, int y);  // Click to scroll main view
};
```

### 6.3 — Reuse Existing Logic
The minimap generation logic in `CWndWorld::_NewDir()` creates a DIB. We can:
1. Keep that logic running (it generates pixel data)
2. Upload the resulting bitmap to an SDL_Texture
3. Render the texture at the minimap position

### Deliverables
- [ ] Minimap texture generated from game state
- [ ] Minimap rendered at correct position in SDL window
- [ ] 4 toggle buttons (Resources, Visible, Mine, Units)
- [ ] Click-to-scroll-main-view working
- [ ] Viewport indicator rectangle on minimap
- [ ] CWndWorld MFC window removed

---

## Phase 7: Dialog System (Replace MFC Dialogs)

**Goal**: All game dialogs render as SDL2 overlays.

### 7.1 — Dialog Framework
Create a general-purpose SDL2 dialog system:
```cpp
class SDL2Dialog {
    SDL_Rect m_rect;           // Position and size
    bool m_modal;              // Block input to game?
    std::string m_title;

    virtual void Render(SDL_Renderer* renderer) = 0;
    virtual void HandleEvent(SDL_Event& event) = 0;
    virtual void OnClose() = 0;
};

class SDL2DialogManager {
    std::vector<std::unique_ptr<SDL2Dialog>> m_activeDialogs;

    void ShowDialog(std::unique_ptr<SDL2Dialog> dialog);
    void CloseDialog(SDL2Dialog* dialog);
    void RenderAll(SDL_Renderer* renderer);
    bool HandleEvent(SDL_Event& event);  // Returns true if dialog consumed event
};
```

### 7.2 — Dialogs to Implement (Priority Order)

**Critical (needed for basic gameplay)**:
1. **Save/Load Dialog** — File browser with save game list
2. **Options Dialog** — Graphics, sound, gameplay settings
3. **Pause Dialog** — Simple overlay when game paused

**Important (needed for full gameplay)**:
4. **Build Structure Dialog** — Building selection grid
5. **Build Transport Dialog** — Vehicle production selection
6. **Research Dialog** — Technology tree
7. **Relations Dialog** — Diplomacy controls

**Nice to have**:
8. **Chat Dialog** — Multiplayer chat
9. **Player List Dialog** — Server management
10. **Credits Screen** — Scrolling credits

### 7.3 — SDL2 UI Widgets
Each dialog needs basic UI controls. Create minimal widgets:
- **Button** — Already have `SDLButton`
- **Label** — Text display (use `SDL_ttf` or game fonts)
- **List** — Scrollable list for file selection, unit lists
- **Slider** — Volume controls, game speed
- **Checkbox** — Toggle options
- **Text Input** — Player name, save file name

### Deliverables
- [ ] SDL2Dialog base class with modal/modeless support
- [ ] SDL2DialogManager for dialog lifecycle
- [ ] Save/Load dialog
- [ ] Options dialog
- [ ] Pause dialog
- [ ] Build Structure/Transport dialogs
- [ ] Research dialog
- [ ] Relations dialog
- [ ] All MFC CDialog subclasses removed

---

## Phase 8: Native SDL2 Rendering (Replace DIB Pipeline)

**Goal**: Replace the DIB-based software renderer with SDL2 hardware-accelerated rendering.

This is the final major phase. Up to this point, the game engine still renders to DIBs — we just display them via SDL. Now we replace the rendering itself.

### 8.1 — SDL_Renderer for 2D
Use `SDL_Renderer` (hardware-accelerated 2D) rather than raw OpenGL. This is simpler and sufficient for a 2D sprite-based game.

```cpp
// Convert sprites from DIB format to SDL_Texture at load time
class SpriteCache {
    std::unordered_map<int, SDL_Texture*> m_textures;

    SDL_Texture* GetSprite(int spriteId, int zoom, int dir, int frame);
    void LoadFromGameData();  // Convert CSpriteDIB → SDL_Texture
};
```

### 8.2 — Terrain Rendering
Replace `CSpriteDIB::TerrainDraw()` with SDL texture blitting:
- Load terrain sprites as SDL_Textures (one per terrain type × zoom level × shade)
- Render hexes using `SDL_RenderCopy()` with appropriate source rects
- Feathering between terrain types via alpha blending

### 8.3 — Sprite Rendering (Buildings, Vehicles, Effects)
Replace `CSpriteDIB::StructureDraw()` and `VehicleDraw()`:
- Load all building/vehicle sprites as SDL_Textures
- Handle rotation via `SDL_RenderCopyEx()` (hardware rotation)
- Handle damage states and build stages via sprite sheet selection

### 8.4 — World Map Rendering
Replace pixel-by-pixel world map generation:
- Generate minimap as SDL_Texture directly
- Use `SDL_SetRenderTarget()` to render to minimap texture
- Update incrementally (dirty rects)

### 8.5 — Remove Global Rendering State
The current renderer uses dangerous globals (`xiDir`, `xiZoom`, `xpdibwnd`, `prVp`). In the SDL2 path:
- Pass rendering context explicitly to each draw call
- No more global state corruption risk
- Enable future multi-threaded rendering possibility

### Deliverables
- [ ] Sprite loading pipeline: game data → SDL_Texture
- [ ] Terrain rendering via SDL_RenderCopy
- [ ] Building/vehicle rendering via SDL_RenderCopy/Ex
- [ ] Effects rendering (smoke, fire, explosions)
- [ ] Selection highlighting
- [ ] Fog of war overlay
- [ ] Remove DIB/WinG rendering path entirely
- [ ] Remove DirectDraw dependency

---

## Phase 9: Cleanup and MFC Removal

**Goal**: Remove all MFC and Win32 GUI dependencies.

### 9.1 — Remove MFC Window Classes
- Delete `CWndMain`, `CWndArea`, `CWndWorld`, `CWndBar` (or hollow them out to logic-only)
- Delete `CDlgMain` and all `CDialog` subclasses
- Delete `CDIBWnd`, `CBmButton` MFC wrappers

### 9.2 — Remove Win32 Dependencies
- Remove `ddraw.lib` link
- Remove `wing32.lib` link
- Remove `vfw32.lib` link (unless needed for video playback)
- Remove MFC library dependencies

### 9.3 — Remove Global Rendering State
- Remove `xiDir`, `xiZoom`, `xpdibwnd`, `prVp` globals
- Remove `bShowAmb`, `bForceDraw`, `bInvAmb` globals
- Pass all state through explicit parameters

### 9.4 — Clean Up Build System
- Remove MFC from CMakeLists.txt
- Remove unused resource files (.rc, .rc2)
- Update include paths
- Verify clean build with only SDL2 dependencies

### Deliverables
- [ ] No MFC includes or dependencies
- [ ] No DirectDraw/WinG dependencies
- [ ] Clean build with SDL2 only
- [ ] All rendering through SDL2
- [ ] All input through SDL2
- [ ] All UI through SDL2

---

## Implementation Order and Dependencies

```
Phase 1: Unified SDL2 Window ──┐
                                ├── Phase 2: Main Menu
                                │
                                ├── Phase 3: Game View (DIB→SDL) ──┐
                                │                                   │
                                └── Phase 4: Input System ──────────┤
                                                                    │
                                    Phase 5: Toolbar ───────────────┤
                                                                    │
                                    Phase 6: Minimap ───────────────┤
                                                                    │
                                    Phase 7: Dialog System ─────────┤
                                                                    │
                                    Phase 8: Native SDL2 Rendering ─┤
                                                                    │
                                    Phase 9: MFC Removal ───────────┘
```

**Phases 1-4 are sequential** (each depends on the previous).
**Phases 5-7 can be done in parallel** after Phase 4.
**Phase 8 requires Phases 3-7 complete**.
**Phase 9 requires everything else complete**.

---

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| Global rendering state (`xiDir`, `xiZoom`) corruption | Game renders wrong sprites | Keep single-threaded rendering; only replace display path first |
| MFC message pump still needed for dialogs | Can't remove MFC until all dialogs migrated | Phase 7 must complete before Phase 9 |
| Sprite format conversion (DIB → SDL_Texture) | Wrong colors, transparency issues | Test with a few sprites first; handle 8-bit palette + 24/32-bit |
| Coordinate system mismatch | Clicks land in wrong place | Build coordinate translation layer early (Phase 4); extensive testing |
| Network play uses Windows messages (`WM_VPNOTIFY`) | Multiplayer breaks | Keep a minimal Windows message pump for network; or migrate to SDL_net |
| Performance regression | Slower rendering | Profile DIB→SDL blit path; SDL_Renderer is hardware-accelerated so should be faster |
| Thread safety (SDL window on worker thread) | Race conditions | Move to single-thread SDL; process SDL events on main thread |

---

## Key Technical Decisions

### 1. Single Window vs Multiple Windows
**Decision**: Single SDL window. All game content renders into one window.
**Reason**: Matches original game (fullscreen popup), simpler input routing, no window management issues.

### 2. SDL_Renderer vs OpenGL
**Decision**: Start with SDL_Renderer (Phase 3-7), optionally move to OpenGL later (Phase 8).
**Reason**: SDL_Renderer is simpler, handles 2D sprite blitting efficiently, and the game is fundamentally 2D. OpenGL adds complexity for minimal gain.

### 3. Keep DIB Rendering Initially
**Decision**: Keep the existing DIB rendering engine through Phases 1-7. Only replace it in Phase 8.
**Reason**: The DIB renderer is proven and correct. Replacing it risks breaking the game. Better to get all UI/input working first, then optimize rendering last.

### 4. Font System
**Decision**: Use SDL_ttf for new UI elements. Keep existing game font system for in-game text until Phase 8.
**Reason**: Game fonts use custom sprite-based rendering that works. New UI (menus, dialogs) can use TTF fonts.

### 5. Thread Model
**Decision**: Move SDL to the main thread. Remove the current worker thread approach.
**Reason**: SDL event handling should be on the main thread. The current worker thread architecture in `GameWindow::ThreadProc()` creates synchronization issues.

---

## Files to Create/Modify

### New Files
| File | Phase | Purpose |
|------|-------|---------|
| `SDL2MainMenu.h/cpp` | 2 | Main menu screen |
| `SDL2Toolbar.h/cpp` | 5 | Toolbar replacement |
| `SDL2Minimap.h/cpp` | 6 | Minimap replacement |
| `SDL2Dialog.h/cpp` | 7 | Dialog base class and manager |
| `SDL2SaveLoadDialog.h/cpp` | 7 | Save/load file browser |
| `SDL2OptionsDialog.h/cpp` | 7 | Options screen |
| `SDL2Widgets.h/cpp` | 7 | UI widget library (buttons, sliders, lists) |
| `SpriteCache.h/cpp` | 8 | DIB-to-texture sprite cache |

### Modified Files
| File | Phase | Changes |
|------|-------|---------|
| `GameWindow.h/cpp` | 1 | Add ProgramState, remove worker thread, add event dispatch |
| `RenderingAdapter.h/cpp` | 3 | Add multi-DIB compositing, screen layout |
| `mainloop.cpp` | 1,3,4 | Add SDL event polling, route rendering through SDL |
| `lastplnt.cpp` | 1,2 | Change init order, bypass CDlgMain |
| `lastplnt.h` | 1 | Add SDL2 state to CConquerApp |
| `CMakeLists.txt` | 1 | Fix SDL2 linking (currently missing!) |

### Files to Eventually Remove (Phase 9)
- `ui.h/cpp` (CWndMain)
- MFC dialog code in `lastplnt.cpp` (CDlgMain section)
- `wing32.lib`, `ddraw.lib` dependencies
- DIB rendering paths in `sprite.cpp`, `area.cpp`, `world.cpp`

---

## Recommended Starting Point

**Start with Phase 1.3 (Event Loop Integration)** — it's the smallest change with the biggest unlock:

1. Add `SDL_PollEvent()` to the existing main loop
2. Route events to `GameWindow::HandleEvent()`
3. This immediately enables testing SDL input alongside MFC

Then proceed to Phase 2 (Main Menu) because:
- It's the first thing users see
- It's self-contained (doesn't interact with game logic much)
- It proves the SDL2 UI system works before tackling the complex game view
- Success here is visually obvious and motivating

After the menu works, tackle Phases 3+4 together (game view rendering + input) — they're tightly coupled and the game isn't playable without both.
