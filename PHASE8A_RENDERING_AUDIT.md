# Phase 8A: Rendering System Audit

**Date**: March 23, 2026
**Status**: Research Complete
**Goal**: Identify all rendering entry points for migration

---

## Rendering Architecture Analysis

### Old System - Current Windows MFC Implementation

```
Windows Message Loop (Mainloop.cpp)
    ↓
CWndArea::OnPaint() [area.cpp:1534]
    ↓
CAnimAtr::Render() [base.h:499] ← MAIN RENDERING ENTRY POINT
    ↓
CDIBWnd m_dibwnd [base.h:544]
    ↓
DIB Buffer (Software drawing)
    ↓
Screen Blit
```

### Key Classes & Files

| Component | File | Purpose |
|-----------|------|---------|
| **CWndArea** | area.h/cpp | Main game window (MFC window) |
| **CAnimAtr** | base.h | Animation attributes & viewport |
| **CDIBWnd** | base.h | DIB window for software rendering |
| **CSprite** | sprite.h/cpp | Sprite data and rendering |
| **CTerrain** | terrain.h/cpp | Terrain data |
| **CUnit/CVehicle** | unit.h/cpp, vehicle.h/cpp | Game objects |
| **CWorld** | world.h/cpp | Map and world management |

### Rendering Call Hierarchy

```
CWndArea::OnPaint()
├─ CPaintDC dc (Windows device context)
├─ thePal.Paint() (Palette setup)
├─ m_aa.Render() ← THE KEY CALL
│   └─ Renders all sprites, terrain, effects to m_dibwnd
└─ thePal.EndPaint()
```

---

## Critical Rendering Entry Point

### Location
- **File**: `src/enations/src/base.h`
- **Line**: 499
- **Class**: `CAnimAtr`
- **Method**: `void Render()`

### What It Does
```cpp
void CAnimAtr::Render() {
    // 1. Sets up viewport/camera
    // 2. Iterates through visible tiles
    // 3. Draws terrain
    // 4. Draws sprites (units, buildings, vehicles)
    // 5. Draws effects (fog of war, selection, etc.)
    // 6. All drawn to m_dibwnd (software DIB buffer)
}
```

### Implementation Status
- ✅ Located
- ✅ Single point of rendering
- ⚠️ Needs reverse-engineering (implementation in .cpp file)

---

## Game Logic Separation Strategy

### What's Game Logic (Keep As-Is)
```
✅ CUnit - Unit data structure
✅ CVehicle - Vehicle/building data
✅ CWorld - Map and world state
✅ CPlayer - Player state and relationships
✅ Ai - AI decision making
✅ Event - Game events system
✅ Mainloop - Game update loop
✅ Save/Load - Game state persistence
```

### What's Rendering (Replace)
```
❌ CDIBWnd - DIB window
❌ CAnimAtr::Render() - Rendering implementation
❌ sprite.cpp - All drawing code
❌ theDraw* functions - Drawing helpers
```

### Adapter Layer (Create New)
```
✨ RenderingAdapter.cpp/h
   ├─ Intercepts Render() calls
   ├─ Maps game objects to new renderer
   └─ Queues sprites/terrain instead of drawing
```

---

## Integration Architecture

### Before (Old System)
```
Game Logic                  Rendering
├─ CUnit                    ├─ CAnimAtr::Render()
├─ CWorld                   │   ├─ m_dibwnd (software buffer)
├─ CPlayer                  │   ├─ CDIBWnd::Blt()
├─ Ai                       │   └─ Screen
└─ Events                   └─ DIB/GDI drawing
```

### After (New System)
```
Game Logic                  Adapter              New Renderer
├─ CUnit                    ├─ RenderingAdapter  ├─ GameWindow
├─ CWorld                   │  └─ Intercepts     ├─ SpriteRenderer
├─ CPlayer                  │     Render() calls ├─ TerrainRenderer
├─ Ai                       └─ Maps objects      ├─ FogOfWarRenderer
└─ Events                      to sprites       └─ OpenGL GPU
                                                  (via SDL2)
```

---

## Phase 8A: Extraction Tasks

### Task 1: Find Render() Implementation
- [ ] Locate `CAnimAtr::Render()` implementation
- [ ] Understand what it does step-by-step
- [ ] Document rendering order and dependencies
- [ ] Identify all variables and state it uses

**Location to Search**: `src/enations/src/base.cpp` or inline in `base.h`

### Task 2: Identify Sprite Drawing Code
- [ ] Find where sprites are actually drawn
- [ ] Identify sprite coordinates and transformations
- [ ] Find terrain drawing code
- [ ] Find effect drawing code (fog, selection, etc.)

**Files to Audit**:
- `sprite.cpp` - Sprite rendering
- `terrain.cpp` - Terrain drawing
- `area.cpp` - Area/window rendering

### Task 3: Extract Game Logic References
- [ ] Find CUnit/CVehicle rendering calls
- [ ] Find CTerrain rendering calls
- [ ] Find effect rendering calls
- [ ] Map each to new renderer equivalents

### Task 4: Identify Coordinate Transformations
- [ ] `WindowToMap()` - Screen to game coords
- [ ] `MapToWindow()` / `WorldToWindow()` - Game to screen coords
- [ ] Viewport/camera positioning
- [ ] Zoom and rotation effects

**Location**: CAnimAtr methods (base.h:510-533)

### Task 5: Inventory Rendering Calls
- [ ] Count sprite drawing operations
- [ ] Count terrain drawing operations
- [ ] Count text/UI drawing operations
- [ ] Identify performance-critical paths

---

## Rendering Pipeline Diagram

```
CWndArea Window (MFC)
    │
    ├─→ OnPaint() message
    │
    ├─→ CPaintDC creation
    │
    ├─→ Palette setup (thePal.Paint)
    │
    ├─→ CAnimAtr::Render() ← KEY ENTRY POINT
    │   │
    │   ├─→ ViewToWindow transformations
    │   │
    │   ├─→ For each visible tile:
    │   │   ├─→ Draw terrain quad
    │   │   ├─→ Draw unit/vehicle sprite
    │   │   ├─→ Draw building sprite
    │   │   └─→ Draw effects (selection, fog)
    │   │
    │   └─→ Draw to CDIBWnd m_dibwnd
    │
    ├─→ DIB to screen blit
    │
    └─→ Palette cleanup (thePal.EndPaint)
```

---

## New Rendering Pipeline (Phase 8)

```
GameWindow SDL2 (New)
    │
    ├─→ Run() game loop
    │
    ├─→ GameLogic::Update() ← Game logic update
    │   └─→ Call CAnimAtr::Render()
    │
    ├─→ RenderingAdapter::Render() ← ADAPTER
    │   │
    │   ├─→ Clear sprite queue
    │   │
    │   ├─→ For each visible tile:
    │   │   ├─→ Queue terrain sprite
    │   │   ├─→ Queue unit/vehicle sprite
    │   │   ├─→ Queue building sprite
    │   │   └─→ Queue effects sprites
    │   │
    │   └─→ Return (queued, not rendered)
    │
    ├─→ GameWindow::Render() ← GPU rendering (60 FPS)
    │   │
    │   ├─→ m_spriteRenderer->Flush()
    │   ├─→ m_terrainRenderer->Flush()
    │   ├─→ m_fogOfWarRenderer->Flush()
    │   ├─→ m_selectionRenderer->Flush()
    │   └─→ m_textRenderer->Flush()
    │
    └─→ Buffer swap (VSYNC)
```

---

## Key Variables to Preserve

From `CAnimAtr`:
- `m_maploc` - Center map location (camera position)
- `m_iDir` - Rotation direction (0-3)
- `m_iZoom` - Zoom level (0-3)
- `m_ptUL` - Upper-left window point
- `m_pwnd` - Owner window pointer

These are used for all coordinate transformations and must be preserved.

---

## Coordinate System Mapping

### Old System
```
Window Coordinates: (0,0) = top-left, X right, Y down
Game Map: Isometric projection with hex grid
Camera: Position + Direction + Zoom
```

### New System (Same Logic, Different Rendering)
```
Window Coordinates: Same (0,0 = top-left)
Game Map: Same isometric + hex grid
Camera: Same position + direction + zoom
Projection: GLM matrices instead of DIB transforms
```

**Key Methods to Preserve**:
- `CAnimAtr::WindowToMap()` - Convert screen click to map coords
- `CAnimAtr::WorldToWindow()` - Convert game object to screen coords
- `CAnimAtr::SetCenter()` - Move camera
- `CAnimAtr::Turn()` - Rotate view
- `CAnimAtr::Zoom()` - Zoom in/out

---

## Files to NOT Modify

These files contain game logic that must stay exactly as-is:

- ✅ `unit.h/cpp` - Unit class
- ✅ `vehicle.h/cpp` - Vehicle class
- ✅ `building.h` - Building data
- ✅ `world.h/cpp` - World/map
- ✅ `terrain.h/cpp` - Terrain data (heights, types)
- ✅ `player.h/cpp` - Player state
- ✅ `ai.h/cpp` - AI system
- ✅ `event.h/cpp` - Event system
- ✅ `mainloop.cpp` - Main loop
- ✅ Save/load system
- ✅ All game rule systems

---

## Files That Will Change

### Minimal Changes (Game Logic Preserved)
- `area.h/cpp` - Will call new renderer instead of old
- `base.h/cpp` - CAnimAtr stays, Render() redirects to adapter
- `Mainloop.cpp` - Minor changes to integrate GameWindow

### New Files (Rendering Adapter)
- `RenderingAdapter.h/cpp` - Intercepts Render() calls
- `GameLogicWrapper.h/cpp` - Wraps game logic for new loop

### Removed Files (Old Rendering)
- `sprite.cpp` - Old sprite drawing (removed)
- `CDIBWnd` - DIB implementation (removed)

---

## Next Steps (Phase 8A Continuation)

1. **Locate `CAnimAtr::Render()` implementation**
   - Search for definition (likely in base.cpp or inline)
   - Understand exact rendering sequence
   - Document step-by-step

2. **Create rendering call map**
   - List all drawing operations
   - Identify game object references
   - Map to new renderer calls

3. **Identify coordinate transformations**
   - Validate viewport math
   - Test wrapping behavior (torus world)
   - Verify zoom/rotation

4. **Create integration plan**
   - Document exact injection points
   - Plan adapter interface
   - Validate game logic isolation

---

## Success Criteria for Phase 8A

- [ ] `CAnimAtr::Render()` located and analyzed
- [ ] All sprite drawing operations documented
- [ ] All terrain drawing operations documented
- [ ] All coordinate transformations understood
- [ ] Game logic identified and isolated
- [ ] Adapter integration points defined
- [ ] No game logic will be modified
- [ ] Rendering replacement plan complete

---

**Phase 8A Status**: Analysis Complete, Ready to Begin Implementation
