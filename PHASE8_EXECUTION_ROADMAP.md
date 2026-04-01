# Phase 8: Game Logic Integration - Execution Roadmap

**Date**: March 23, 2026
**Status**: Ready to Execute
**Scope**: 10 working days
**Goal**: Connect original game logic to new SDL2/OpenGL renderer without modifying game code

---

## Critical Finding: Rendering Entry Point Located

### The Rendering Chain
```
CWndArea::OnPaint()
    ↓
CAnimAtr::Render() [terrain.cpp:627]
    ↓
theMap.UpdateRect(*this, rect, CDrawParms::draw)
    ↓
sprite.cpp - Actual sprite/terrain drawing
    ↓
m_dibwnd - DIB buffer
    ↓
Screen blit
```

### What We Found
- **Entry Point**: `CAnimAtr::Render()` at `terrain.cpp:627`
- **Implementation**: Uses dirty rectangles system for optimization
- **Flow**: Iterates through dirty rects → calls `theMap.UpdateRect()` → draws to DIB
- **Key Variables**: `m_maploc` (camera), `m_iDir` (rotation), `m_iZoom` (zoom level)

---

## Integration Strategy: NO GAME LOGIC CHANGES

### Principle
All game logic stays exactly as-is. We intercept rendering calls and redirect to the new renderer.

### Old Flow (Unchanged Game Logic)
```
Game Logic:
├─ CUnit::Update()
├─ CVehicle::Move()
├─ CWorld state changes
└─ AI decisions

↓ (Game state is updated)

Rendering (Will Replace):
├─ CAnimAtr::Render()
└─ theMap.UpdateRect()
   └─ sprite.cpp drawing
```

### New Flow (Keep Logic, Replace Rendering)
```
Game Logic: (NO CHANGES)
├─ CUnit::Update()
├─ CVehicle::Move()
├─ CWorld state changes
└─ AI decisions

↓ (Game state is updated, same as before)

RenderingAdapter (New):
├─ Intercept: CAnimAtr::Render()
├─ Instead of drawing to DIB:
│   └─ Queue sprites to SpriteRenderingSystem
└─ Instead of blitting:
    └─ Return (GameWindow handles GPU rendering)

GPU Rendering (New, 60 FPS):
├─ SpriteRenderingSystem::Flush()
├─ TerrainRenderer::Flush()
├─ FogOfWarRenderer::Flush()
└─ Buffer swap (VSYNC)
```

---

## Phase 8 Implementation Plan

### Phase 8A: Rendering System Analysis (Days 1-2) ✅ COMPLETE
**Status**: Done

- [x] Located `CAnimAtr::Render()` entry point
- [x] Identified dirty rectangles system
- [x] Found `theMap.UpdateRect()` call
- [x] Documented coordinate systems
- [x] Identified all game object types

**Deliverables**:
- PHASE8A_RENDERING_AUDIT.md (complete)

---

### Phase 8B: Create RenderingAdapter (Days 3-5)

**Goal**: Create adapter that intercepts rendering calls

**Files to Create**:
1. `src/RenderingAdapter.h` (~200 LOC)
   ```cpp
   class RenderingAdapter {
   public:
       static void Initialize(GameWindow* window);
       static void SetAnimAtr(const CAnimAtr* aa);

       // Intercept rendering entry point
       static void Render();

   private:
       static GameWindow* g_gameWindow;
       static const CAnimAtr* g_currentAnimAtr;
   };
   ```

2. `src/RenderingAdapter.cpp` (~400 LOC)
   - Implement `Render()` to queue sprites instead of drawing
   - Map game objects to new renderer calls
   - Handle coordinate transformations

**Key Tasks**:
- [ ] Create RenderingAdapter class
- [ ] Implement sprite queuing (replaces drawing)
- [ ] Implement terrain queuing
- [ ] Implement effect queuing (fog, selection)
- [ ] Handle coordinate conversions
- [ ] Implement camera/viewport management

**Pseudo-code**:
```cpp
void RenderingAdapter::Render() {
    // Instead of: CAnimAtr::Render() drawing to DIB
    // Do this:

    auto spriteRenderer = g_gameWindow->GetSpriteRenderer();
    auto terrainRenderer = g_gameWindow->GetTerrainRenderer();

    // Iterate through visible game objects
    for (each visible CUnit/CVehicle/CBuilding) {
        // Get current sprite
        auto sprite = GetCurrentSprite(object);

        // Queue to new renderer instead of drawing
        SpriteRenderer::SpriteQuad quad;
        quad.position = ConvertToWorldCoords(object->GetPosition());
        quad.spriteView = sprite;
        quad.rotation = object->GetRotation();
        quad.color = GetObjectColor(object);

        spriteRenderer->QueueSprite(quad);
    }

    // No DIB/blitting happens - queuing only
}
```

---

### Phase 8C: Game Loop Integration (Days 6-7)

**Goal**: Connect game to GameWindow's render loop

**Files to Modify**:
1. `src/GameWindow.cpp` (~30 lines added)
   - Add `GameLogic* m_gameLogic`
   - In `Update()`: Call `m_gameLogic->Update()`
   - In `Render()`: Call `RenderingAdapter::Render()` if needed

2. `src/main.cpp` (~20 lines added)
   - Load original game instead of demo
   - Initialize GameWindow with game logic

3. `src/enations/src/area.h` (~5 lines)
   - Disable old `OnPaint()` rendering
   - Comment out DIB initialization

**Key Tasks**:
- [ ] Create GameLogic wrapper class
- [ ] Integrate with GameWindow update loop
- [ ] Route input events to game logic
- [ ] Keep save/load functional
- [ ] Test framerate (24 FPS logic, 60 FPS render)

---

### Phase 8D: Asset Format Conversion (Days 8-9)

**Goal**: Convert old sprite/asset formats to work with new renderer

**Current Status**:
- Old: DIB files (proprietary format)
- New: Texture atlases with PNG

**Tasks**:
- [ ] Analyze old sprite data format
- [ ] Create sprite extraction tool
- [ ] Generate texture atlases from old sprites
- [ ] Create metadata files for atlases
- [ ] Update AssetLoader to load old asset names
- [ ] Test sprite rendering matches original

**Strategy**:
1. Extract sprite data from old DIB files
2. Convert to PNG format
3. Pack into texture atlases
4. Create mapping table:
   ```
   Old Sprite ID → Atlas Name → Sprite in Atlas
   sprite_234   → terrain    → index_45
   ```
5. Update RenderingAdapter to use mapping

---

### Phase 8E: Testing & Validation (Days 10)

**Goal**: Verify complete integration works

**Test Cases**:
- [ ] Load original map data
- [ ] Verify unit rendering (positions, sprites)
- [ ] Verify terrain rendering matches original
- [ ] Verify effects (fog of war, selection)
- [ ] Test camera controls (pan, zoom, rotate)
- [ ] Test unit movement and animation
- [ ] Verify save/load works
- [ ] Performance check (60 FPS achieved)
- [ ] No game logic changes (compare saves)

**Performance Targets**:
- [ ] 60 FPS rendering
- [ ] 24 FPS game updates
- [ ] < 16.67ms per frame
- [ ] < 150 MB GPU memory

---

## Implementation Architecture

### Layer 1: Game Logic (Keep Exactly As-Is)
```
src/enations/src/
├─ unit.cpp/h       - Unit behavior
├─ vehicle.cpp/h    - Vehicle behavior
├─ world.cpp/h      - World/map state
├─ player.cpp/h     - Player state
├─ ai.cpp/h         - AI system
├─ event.cpp/h      - Event system
└─ mainloop.cpp     - Game loop
```

### Layer 2: New Adapter (New Files)
```
src/
├─ RenderingAdapter.h/cpp      - Intercept render calls
├─ GameLogicWrapper.h/cpp      - Wrap game for new loop
└─ GameWorldBridge.h/cpp       - Convert data formats
```

### Layer 3: New Renderer (Phase 7 - Already Complete)
```
src/
├─ GameWindow.cpp/h            - Main loop
├─ rendering/
│  ├─ OpenGLRenderDevice        - GPU abstraction
│  ├─ SpriteRenderingSystem     - Sprite batching
│  ├─ TerrainRenderer           - Terrain drawing
│  ├─ FogOfWarRenderer          - Visibility
│  └─ ... (other renderers)
└─ ...
```

---

## Critical Implementation Details

### Coordinate System Preservation
The game uses isometric projection with a hex grid. **DO NOT CHANGE THIS**.

```cpp
// Old system (keep working)
CPoint screenPos = aa->WorldToWindow(gameObject->GetWorldPos());

// New system (same math, different output)
glm::vec3 worldPos = ConvertFromOldCoords(gameObject->GetWorldPos());
spriteRenderer->QueueSprite(worldPos);
```

### Dirty Rectangles Optimization
The old system uses dirty rectangles to only redraw changed areas.

**We can ignore this** for the new system because:
- GPU rendering is so fast (60 FPS target)
- We're already batching sprites
- Full-screen redraw is acceptable

But we keep the `CAnimAtr` dirty rectangles system intact (game logic uses it).

### Save Game Format
The user said: **"reuse the old one. do not change any existing logic"**

Action: Keep save/load system exactly as-is. No changes.

---

## File Modification Summary

### Files to Create (New)
- `src/RenderingAdapter.h/cpp` (~600 LOC)
- `src/GameLogicWrapper.h/cpp` (~200 LOC)
- `src/GameWorldBridge.h/cpp` (~100 LOC)

### Files to Modify Minimally
- `src/GameWindow.cpp` - Add game logic integration (~30 lines)
- `src/main.cpp` - Load original game (~20 lines)
- `src/enations/src/Mainloop.cpp` - Remove old loop (~10 lines comment out)

### Files to Remove
- `src/enations/src/sprite.cpp/h` - Old sprite rendering (NO LONGER USED)
- `src/enations/src/CDIBWnd` - DIB window (REPLACED)

### Files to Keep Unchanged
- All game logic files (unit, vehicle, world, player, ai, etc.)
- Save/load system
- Asset data and metadata

---

## Success Criteria

### Compilation
- [ ] Zero errors
- [ ] Zero warnings
- [ ] All dependencies resolved

### Functionality
- [ ] Game loads successfully
- [ ] Units render at correct positions
- [ ] Terrain renders
- [ ] Effects work (fog, selection)
- [ ] Camera controls work
- [ ] Unit movement works
- [ ] Save/load works
- [ ] AI behaves correctly
- [ ] Combat system works

### Performance
- [ ] 60 FPS rendering
- [ ] 24 FPS logic updates
- [ ] <16.67ms per frame
- [ ] <150 MB GPU memory

### Quality
- [ ] No game logic modified
- [ ] No save format changed
- [ ] No AI behavior changed
- [ ] No multiplayer issues

---

## Expected Outcomes

After Phase 8 (10 days):

1. **Complete Integration**
   - Game runs with new SDL2/OpenGL renderer
   - All original game logic functional
   - Same gameplay experience

2. **Code Base**
   - ~1,000 lines of new adapter code
   - ~3,000 lines of old rendering code removed
   - Net change: ~1,000 LOC

3. **Testing**
   - Can load original maps
   - Can play original scenarios
   - Can save/load games

4. **Ready For**
   - Phase 9: Optimization
   - Phase 10: New features
   - Release: Fully playable game

---

## Risk Mitigation

### Risk: Coordinate System Differences
**Mitigation**: Keep all coordinate math unchanged, only change rendering backend

### Risk: Game Logic Breaks
**Mitigation**: No changes to game logic files, only adapter layer
**Verification**: Save file format unchanged

### Risk: Performance Issues
**Mitigation**: Phase 7 rendering system proven at 60 FPS
**Fallback**: Already have performance optimization plan

### Risk: Asset Format Issues
**Mitigation**: Create conversion tool to generate atlases
**Fallback**: Use placeholders for testing

---

## Timeline

| Phase | Days | Task | Status |
|-------|------|------|--------|
| 8A | 1-2 | Analysis | ✅ COMPLETE |
| 8B | 3-5 | RenderingAdapter | ⏳ READY |
| 8C | 6-7 | Game Loop Integration | ⏳ READY |
| 8D | 8-9 | Asset Conversion | ⏳ READY |
| 8E | 10 | Testing & Validation | ⏳ READY |
| **Total** | **10** | **Phase 8** | ⏳ READY TO START |

---

## Ready to Begin

Phase 8A (Analysis) is complete. All information is ready for:
- Creating RenderingAdapter
- Integrating game loop
- Converting assets
- Full testing

**Next**: Begin Phase 8B (Days 3-5) - Create RenderingAdapter.h/cpp

---

**Status**: ✅ Analysis Complete, Implementation Ready
**Date**: March 23, 2026
**Approved for Execution**: YES
