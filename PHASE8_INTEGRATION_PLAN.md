# Phase 8: Game Logic Integration Plan

**Date**: March 23, 2026
**Status**: Planning Phase
**Goal**: Connect original Enemy Nations game logic to new SDL2/OpenGL rendering system

---

## Current State Analysis

### What We Have

**New Rendering System (Phase 7)** ✅
- SDL2 window and event handling
- OpenGL 3.3 GPU rendering
- SpriteRenderer with batching
- TextRenderer with SDF fonts
- TerrainRenderer for hex grids
- FogOfWarRenderer, SelectionRenderer, etc.
- AssetLoader and TextureLoader
- PerformanceMonitor

**Original Game Code** (src/enations/src/)
- Complete game logic (units, buildings, vehicles)
- AI system (Ai.cpp, CaiData.cpp, etc.)
- Player management and relationships
- Map/world system (World.cpp, Terrain.cpp)
- Combat and damage systems
- Research and technology trees
- Event system
- Networking infrastructure (for multiplayer)
- Save/load system

### Architecture Mismatch

**Old System (Windows MFC)**
```
CWinApp (Windows Application)
  ├─ CMainFrame (MFC Window)
  ├─ CDIBWnd (DIB rendering surface)
  ├─ Game Logic (Units, AI, World)
  └─ Rendering (DIB/GDI → Screen)
```

**New System (SDL2/OpenGL)**
```
GameWindow (SDL2)
  ├─ OpenGLRenderDevice (GPU)
  ├─ Rendering Systems (Sprite, Text, Terrain, Effects)
  └─ InputHandler (SDL events)

[Game Logic] ← (needs integration)
```

### Key Rendering Differences

| Aspect | Old (DIB/GDI) | New (SDL2/OpenGL) |
|--------|---------------|-------------------|
| **Window** | Windows HWND | SDL2 Window |
| **Context** | Device Context (HDC) | OpenGL Context |
| **Rendering** | Software (CPU) | Hardware (GPU) |
| **Sprites** | DIB memory buffer | GPU textures + VAO |
| **Text** | Bitmap fonts | SDF fonts |
| **Framerate** | Varies | 60 FPS target |
| **Platform** | Windows only | Cross-platform |

---

## Phase 8 Integration Strategy

### High-Level Approach

```
Step 1: Analyze Old Rendering Code
        └─ Understand sprite rendering loop
           - How sprites are queued
           - Sorting/batching logic
           - Terrain rendering

Step 2: Extract Game Logic Layer
        └─ Identify pure logic vs rendering
           - Unit movement (logic)
           - Sprite drawing (rendering) ✗ Remove
           - AI decisions (logic)
           - Screen updates (rendering) ✗ Remove

Step 3: Create Integration Layer
        └─ Map old game calls to new renderer
           - DrawSprite() → SpriteRenderer::QueueSprite()
           - DrawText() → TextRenderer::QueueText()
           - UpdateScreen() → Nothing (automatic)

Step 4: Adapt Game Loop
        └─ Integrate with GameWindow::Run()
           - Call game Update() at 24 FPS
           - Call game Render() at 60 FPS
           - Route input events

Step 5: Test & Validate
        └─ Load existing game data
           - Maps and scenarios
           - Units and buildings
           - Verify rendering matches original
```

---

## Key Files to Integrate

### Game Logic Files (Keep & Adapt)

**Core Systems**:
- `Unit.cpp/h` - Unit class definition
- `Vehicle.cpp/h` - Vehicle/building class
- `World.cpp/h` - Map/world management
- `Terrain.cpp/h` - Terrain data and heights
- `Player.cpp/h` - Player state and management
- `Ai.cpp/h` - AI decision making

**Game Systems**:
- `Event.cpp/h` - Game events system
- `Research.cpp/h` - Technology research
- `Relation.cpp/h` - Player relationships/diplomacy
- `Area.cpp/h` - Map viewport/area
- `Mainloop.cpp/h` - Main game loop (needs adaptation)

### Rendering Files (Replace)

**Old Rendering (Remove)**:
- `sprite.cpp/h` - Old DIB sprite rendering
- `terrain.cpp` (rendering part) - Old terrain drawing
- `sprtinit.cpp` - Old sprite initialization

**Will Use New Rendering**:
- SpriteRenderingSystem (new)
- TerrainRenderer (new)
- TextRenderer (new)
- FogOfWarRenderer (new)
- SelectionRenderer (new)

### Files to Bridge

**Integration Layer (New Files)**:
- `GameLogic.cpp/h` - Main game logic orchestrator
- `RenderingAdapter.cpp/h` - Maps game calls to renderer
- `GameWorldAdapter.cpp/h` - Adapts World class to new renderer

---

## Implementation Phases

### Phase 8A: Game Logic Extraction (Days 1-2)

**Goal**: Understand and extract pure game logic

**Tasks**:
1. Analyze World.cpp - understand map representation
2. Analyze Unit.cpp - understand unit data structures
3. Analyze Ai.cpp - understand decision making
4. Identify rendering calls in game logic
5. Create pure logic version (no rendering calls)

**Deliverable**: Game logic compiles without graphics dependencies

---

### Phase 8B: Rendering Adapter (Days 3-4)

**Goal**: Create bridge from old game calls to new renderer

**Example Adapter**:
```cpp
// Old code would do:
// DrawSprite(spriteId, x, y, frame);

// New adapter intercepts and does:
class RenderingAdapter {
public:
    static void DrawSprite(int spriteId, int x, int y, int frame) {
        auto spriteRenderer = g_gameWindow->GetSpriteRenderer();
        auto sprite = g_assetLoader->GetSprite(GetSpriteAssetName(spriteId));

        SpriteRenderer::SpriteQuad quad{};
        quad.position = ConvertToWorldCoords(x, y);
        quad.spriteView = sprite;
        spriteRenderer->QueueSprite(quad);
    }
};
```

**Tasks**:
1. Create RenderingAdapter class
2. Implement DrawSprite mapping
3. Implement DrawText mapping
4. Implement DrawTerrain mapping
5. Implement UpdateScreen (no-op)

**Deliverable**: Old rendering calls work with new renderer

---

### Phase 8C: Game Loop Integration (Days 5-6)

**Goal**: Connect game logic to GameWindow main loop

**Implementation**:
```cpp
// In GameWindow::Update() [called at 24 FPS]
void GameWindow::Update(float deltaTime) {
    if (m_gameLogic) {
        m_gameLogic->Update(deltaTime);
        // Queues sprites and text for rendering
    }
}

// In GameWindow::Render() [called at 60 FPS]
void GameWindow::Render() {
    // Rendering systems flush queued operations
    m_spriteRenderer->Flush();
    m_textRenderer->Flush();
    m_terrainRenderer->Flush();
    // etc.
}
```

**Tasks**:
1. Create GameLogic class wrapper
2. Integrate with GameWindow
3. Connect input events
4. Connect update loop
5. Connect render loop

**Deliverable**: Game running at correct framerates

---

### Phase 8D: Data Adaptation (Days 7-8)

**Goal**: Convert old asset formats to new renderer formats

**Conversions Needed**:
1. Sprite data format
   - Old: DIB files with metadata
   - New: PNG files in texture atlases
2. Terrain data format
   - Old: Terrain height map (keeps heights)
   - New: Add isometric projection for rendering
3. Font data
   - Old: Bitmap fonts
   - New: SDF fonts (already have fallback)
4. Color palette
   - Old: 256-color palette
   - New: RGB/RGBA colors

**Tasks**:
1. Analyze old sprite format
2. Create sprite conversion tools
3. Adapt terrain rendering
4. Handle color/palette conversions
5. Validate asset loading

**Deliverable**: Old game data loads and displays correctly

---

### Phase 8E: Testing & Validation (Days 9-10)

**Goal**: Verify complete integration works

**Testing**:
1. Load original maps and scenarios
2. Run game simulation (no human input)
3. Verify rendering matches expected output
4. Check performance metrics
5. Validate save/load functionality
6. Test multiplayer connectivity (if applicable)

**Deliverable**: Full integration test suite passes

---

## Estimated Scope

### Code Changes
```
New Files:
- GameLogic.cpp/h              (~300 LOC)
- RenderingAdapter.cpp/h       (~400 LOC)
- GameWorldAdapter.cpp/h       (~200 LOC)
- GameIntegration utilities    (~200 LOC)

Modified Files:
- main.cpp                      (10-20 lines)
- GameWindow.cpp               (20-30 lines)

Existing Files to Remove:
- sprite.cpp/h                 (~3000 LOC old, removed)
- Old rendering code           (removed)

Net Change: ~1,100 LOC new code, ~3,000 LOC old code removed
```

### Timeline Estimate

| Phase | Days | Tasks | Notes |
|-------|------|-------|-------|
| 8A | 1-2 | Logic extraction | Analysis and understanding |
| 8B | 3-4 | Adapter creation | Mapping old calls to new |
| 8C | 5-6 | Loop integration | Connect systems |
| 8D | 7-8 | Data conversion | Format adaptation |
| 8E | 9-10 | Testing | Validation and fixes |
| **Total** | **10 days** | **5 phases** | **Complete integration** |

---

## Key Integration Points

### Input Integration
```cpp
// SDL2 events come into InputHandler
// Route to game logic
class GameLogic : public InputHandler::InputListener {
    bool OnMouseClick(float x, float y, MouseButton btn) override {
        // Convert screen coords to game world coords
        // Call game's click handler
        return m_world->HandleClick(worldX, worldY);
    }

    bool OnKeyPress(int keyCode) override {
        // Route to game's keyboard handler
        return m_gameController->OnKeyPress(keyCode);
    }
};
```

### Update Loop
```cpp
// GameWindow calls game logic at 24 FPS
void GameWindow::Update(float deltaTime) {
    m_gameLogic->Update(deltaTime);
    // This queues all rendering operations
}

// Game logic queues rendering:
void GameLogic::Update(float dt) {
    m_world->Update(dt);  // Updates units, AI, etc.
    m_world->QueueRender();  // Queues sprites to render
}
```

### Rendering Pipeline
```cpp
// GameWindow renders at 60 FPS
void GameWindow::Render() {
    // All queued operations execute
    m_spriteRenderer->Flush();
    m_textRenderer->Flush();
    m_terrainRenderer->Flush();
    m_fogOfWarRenderer->Flush();
}
```

---

## Challenges & Solutions

### Challenge 1: DIB to GPU Texture Conversion
**Problem**: Old code uses DIB (Device-Independent Bitmap) format
**Solution**:
1. Extract sprite data from DIBs
2. Convert to PNG format
3. Load through new TextureLoader
4. Use SpriteRenderingSystem for rendering

### Challenge 2: Coordinate System Differences
**Problem**: Old code uses different coordinate system
**Solution**:
1. Old: Screen coordinates (0,0 = top-left)
2. New: Isometric world coordinates
3. Create adapter functions for conversion

### Challenge 3: Framerate Differences
**Problem**: Old code renders on-demand; new renders at fixed 60 FPS
**Solution**:
1. Decouple game logic (24 FPS) from rendering (60 FPS)
2. Queue rendering operations each update
3. Use PerformanceMonitor to validate timing

### Challenge 4: Platform Dependencies
**Problem**: Old code uses Windows MFC and Win32 APIs
**Solution**:
1. Remove Windows-specific code
2. Use SDL2 for window/input
3. Use OpenGL for rendering
4. Test on multiple platforms

### Challenge 5: Asset Format Changes
**Problem**: Old uses proprietary formats; new uses standard formats
**Solution**:
1. Create conversion tools
2. Build asset pipeline
3. Validate conversion quality

---

## Success Criteria

### Compilation
- [ ] All code compiles without errors
- [ ] No compilation warnings
- [ ] All dependencies resolved

### Functionality
- [ ] Game logic runs correctly
- [ ] Units move and behave as expected
- [ ] AI makes reasonable decisions
- [ ] Combat system works
- [ ] Save/load functionality preserved
- [ ] Rendering displays game correctly

### Performance
- [ ] 60 FPS rendering achieved
- [ ] 24 FPS game updates stable
- [ ] CPU time < 16.67ms per frame
- [ ] GPU memory < 150 MB
- [ ] Sprite count > 1000 at 60 FPS

### Quality
- [ ] Zero memory leaks
- [ ] Error handling comprehensive
- [ ] Code well documented
- [ ] Test coverage > 90%

---

## Next Steps

1. **Review** this plan for accuracy and completeness
2. **Clarify** any ambiguities about the original game logic
3. **Confirm** which features are essential for Phase 8
4. **Begin** with Phase 8A (Game Logic Extraction)

---

## Questions to Answer Before Starting

1. **Maps & Assets**: Where are the original game maps and sprite data stored?
2. **Feature Scope**: Do we need multiplayer/networking in Phase 8, or single-player only?
3. **Audio**: Should we integrate audio, or leave that for Phase 9?
4. **Scenarios**: Which scenarios/maps are priorities to test with?
5. **Save Format**: Should we preserve old save game format or create new one?

---

**Status**: Plan complete, ready for user review and approval.
Phase 8 can begin immediately upon confirmation.
