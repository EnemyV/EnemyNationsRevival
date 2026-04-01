# Enemy Nations Rendering System - API Reference

## Table of Contents

1. [Core Application](#core-application)
2. [Rendering Systems](#rendering-systems)
3. [Input System](#input-system)
4. [UI System](#ui-system)
5. [Utility Systems](#utility-systems)

---

## Core Application

### GameWindow

Main application window and orchestrator for all rendering systems.

#### Creation

```cpp
// Factory method - preferred way to create
std::shared_ptr<GameWindow> window = GameWindow::Create(
    "Enemy Nations",  // Window title
    1024,            // Width in pixels
    768              // Height in pixels
);

if (!window || !window->IsValid()) {
    // Error - check SDL_GetError()
}
```

#### Properties

```cpp
// Window dimensions
int width = window->GetWidth();      // Returns 1024
int height = window->GetHeight();    // Returns 768
std::string title = window->GetTitle();

// State checking
bool valid = window->IsValid();      // True if initialized properly
bool running = window->IsRunning();  // True during main loop
float fps = window->GetFrameRate();  // Current FPS (updated each second)

// Frame rate control
int targetFps = window->GetTargetFrameRate();  // Default 24
window->SetTargetFrameRate(30);                // Change target
```

#### System Access

```cpp
// Get pointers to all rendering systems
SpriteRenderer* sprites = window->GetSpriteRenderer();
TextRenderer* text = window->GetTextRenderer();
TerrainRenderer* terrain = window->GetTerrainRenderer();
FogOfWarRenderer* fog = window->GetFogOfWarRenderer();
SelectionRenderer* selection = window->GetSelectionRenderer();
DamageDisplayRenderer* damage = window->GetDamageDisplayRenderer();
StatusBarRenderer* status = window->GetStatusBarRenderer();
DialogRenderer* dialogs = window->GetDialogRenderer();

// Core systems
AssetManager* assets = window->GetAssetManager();
Viewport* viewport = window->GetViewport();
InputHandler* input = window->GetInputHandler();
```

#### Main Loop

```cpp
// Run the main game loop (blocking, returns when window closes)
int exitCode = window->Run();
// Processes events, updates at 24 FPS, renders at 60 FPS
// Returns 0 on success, 1 on error

// Request window close from anywhere
window->RequestClose();  // Sets m_running = false
```

#### Window Control

```cpp
// Show/hide window
window->SetVisible(true);   // Show
window->SetVisible(false);  // Hide

// Direct rendering control (called automatically by Run())
window->ClearScreen(0.2f, 0.2f, 0.2f, 1.0f);  // RGB + Alpha
window->SwapBuffers();                        // Present frame
```

#### Frame Timing

The game loop runs at different rates:
- **Rendering**: 60 FPS (or uncapped if no vsync)
- **Game Updates**: 24 FPS via accumulation
- **Remainder Carryover**: Prevents frame skip jitter

```cpp
// In main loop:
// deltaTime ≈ 16.67ms (1/60 second)
// accumulated with remainder to trigger 24 FPS updates
```

---

## Rendering Systems

### SpriteRenderer

GPU-based sprite batching with Z-order sorting.

#### Queuing Sprites

```cpp
SpriteRenderer::SpriteQuad quad;
quad.position = glm::vec3(screenX, screenY, depth);  // Screen coords + Z
quad.width = 64.0f;                                  // Pixel width
quad.height = 64.0f;                                 // Pixel height
quad.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);     // RGBA (white, opaque)
quad.rotationAngle = 0.0f;                           // Rotation in degrees
quad.objectId = unitId;                             // For hit testing
quad.screenX = screenX;                             // Cached for sorting
quad.screenY = screenY;                             // Cached for sorting

spriteRenderer->QueueSprite(quad);

// Multiple sprites queued in one frame
spriteRenderer->QueueSprite(quad1);
spriteRenderer->QueueSprite(quad2);
// ...later...
spriteRenderer->Render();  // All sorted and rendered at once
```

#### Sorting

Sprites are automatically sorted by:
1. **screenY** (primary) - Ground level determines draw order
2. **screenX** (secondary) - Left-to-right for same Y
3. **objectId** (tertiary) - Stable ordering for same position

This creates correct isometric depth ordering.

#### Rendering

```cpp
// Called once per frame
spriteRenderer->Render();
// All queued sprites are sorted and drawn in one batch

// Update any internal state
spriteRenderer->Update(deltaTime);
```

---

### TerrainRenderer

Renders hexagonal terrain grid with altitude-based shading.

#### Setup

```cpp
// Pass SpriteRenderer as dependency
TerrainRenderer* terrain = new TerrainRenderer(spriteRenderer);

// Get shader for different terrain types
TextureAtlas::Sprite grassSprite = assets->GetSprite("terrain/grass");
TextureAtlas::Sprite waterSprite = assets->GetSprite("terrain/water");
```

#### Height/Shading Map

```cpp
// Terrain shading based on corner height differences
// Light source from right (+X), so right-to-left height diff = shadow depth
// Range: 0 (light) to 7 (dark)

// Set up 128x128 hex map
float heights[128][128];  // Corner heights for each hex
terrain->SetHeightMap(heights);

// Calculate shade for specific hex (0-7)
int shade = terrain->CalculateShade({10, 11, 12, 13});  // Corner heights
// shade = 0 (no shadow), 7 (heavy shadow)
```

#### Rendering

```cpp
// Render terrain hexes in viewport
terrain->Render(
    minX, minY, maxX, maxY,    // Screen bounds to render
    hexWidth, hexHeight,        // Sprite size (64x64)
    viewport                    // For coordinate transformation
);

// Updates and queues sprites to SpriteRenderer
// Then SpriteRenderer::Render() draws them
```

---

### TextRenderer

SDF (Signed Distance Field) font rendering for scalable text.

#### Font Loading

```cpp
// Load font from file
bool loaded = textRenderer->LoadFont("assets/fonts/arial.fnt", 24);

// Font metrics
TextRenderer::FontMetrics metrics = textRenderer->GetFontMetrics();
float lineHeight = metrics.lineHeight;
float ascender = metrics.ascender;
```

#### Drawing Text

```cpp
// Draw text at screen position
textRenderer->DrawText(
    "Hello, World!",           // Text to render
    100.0f, 200.0f,            // Screen X, Y
    16.0f,                      // Font size in pixels
    glm::vec4(1, 1, 1, 1),      // Color (RGBA)
    0                           // Alignment: 0=left, 1=center, 2=right
);

// Different alignments
textRenderer->DrawText(text, x, y, size, color, 0);  // Left-aligned
textRenderer->DrawText(text, x, y, size, color, 1);  // Centered
textRenderer->DrawText(text, x, y, size, color, 2);  // Right-aligned
```

#### Text Wrapping

```cpp
// Wrap text to fit within width
std::string wrapped = textRenderer->WrapText(
    "Long text here...",  // Text to wrap
    200.0f,              // Max width in pixels
    12.0f                // Font size
);

// Returns text with newlines inserted
```

#### Text Measurement

```cpp
// Get bounding box of text
float width = textRenderer->MeasureWidth("Text", 16.0f);
float height = textRenderer->GetLineHeight(16.0f);

// Useful for UI layout
```

---

### TerrainRenderer

Already documented above.

---

### FogOfWarRenderer

Manages game world visibility with three states.

#### Visibility States

```cpp
enum class VisibilityState : uint8_t {
    UNEXPLORED = 0,  // Never seen (black, opacity 1.0)
    EXPLORED = 1,    // Seen before (dim, opacity 0.5)
    VISIBLE = 2,     // Currently visible (clear, no overlay)
};
```

#### Setting Visibility

```cpp
// Set entire map visibility
std::vector<VisibilityState> visibilityMap(128 * 128);
std::fill(visibilityMap.begin(), visibilityMap.end(),
         VisibilityState::UNEXPLORED);  // Start all hidden

fogRenderer->SetVisibilityMap(visibilityMap.data(), 128, 128);

// Update single hex
fogRenderer->SetHexVisibility(
    hexX, hexY,
    VisibilityState::VISIBLE  // Make this hex visible
);

// Clear all fog
fogRenderer->ClearFog();  // All hexes become VISIBLE

// Set all to unexplored
fogRenderer->SetAllUnexplored();
```

#### Customization

```cpp
// Set colors for visibility states
glm::vec4 unexploredColor = glm::vec4(0, 0, 0, 1);  // Black, opaque
glm::vec4 exploredColor = glm::vec4(0, 0, 0, 0.5);  // Black, 50% opacity

fogRenderer->SetColors(exploredColor, unexploredColor);

// Set custom opacity
fogRenderer->SetOpacity(0.5f, 1.0f);  // explored, unexplored
```

#### Rendering

```cpp
fogRenderer->Render(
    minX, minY, maxX, maxY,  // Screen viewport bounds
    hexWidth, hexHeight,      // Sprite dimensions
    viewport                  // For coordinate transformation
);
```

---

### SelectionRenderer

Visual highlighting for selected hexes and objects.

#### Hex Selection

```cpp
// Select single hex
selectionRenderer->SelectHex(32, 48);

// Multi-select mode
selectionRenderer->SelectHex(32, 48, true);  // Add to selection
selectionRenderer->SelectHex(33, 48, true);  // Keep adding

// Select rectangular region
selectionRenderer->SelectHexRegion(
    32, 48,      // Min X, Y
    40, 56,      // Max X, Y
    false        // Replace existing selection
);

// Query selection
bool selected = selectionRenderer->IsHexSelected(32, 48);
```

#### Object Selection

```cpp
// Select single object
selectionRenderer->SelectObject(unitId);

// Multi-select objects
selectionRenderer->SelectObjects({unit1, unit2, unit3}, false);

// Query
bool selected = selectionRenderer->IsObjectSelected(unitId);
size_t count = selectionRenderer->GetSelectedObjectCount();
std::vector<int> selected = selectionRenderer->GetSelectedObjects();
```

#### Object Positioning

```cpp
// Update object world position for rendering
selectionRenderer->UpdateObjectPosition(unitId, worldX, worldY);

// Called each frame for all selected objects:
for (int id : selectedObjects) {
    Unit* unit = GetUnit(id);
    selectionRenderer->UpdateObjectPosition(id, unit->worldX, unit->worldY);
}
```

#### Customization

```cpp
// Change highlight colors
selectionRenderer->SetHighlightColor(glm::vec4(1, 1, 0, 0.6));    // Yellow
selectionRenderer->SetSecondaryColor(glm::vec4(0, 1, 1, 0.5));    // Cyan

// Change outline thickness (as fraction of hex size)
selectionRenderer->SetOutlineThickness(0.15f);  // 15% larger
```

#### Rendering

```cpp
// Render hex selections
selectionRenderer->RenderHexSelections(
    minX, minY, maxX, maxY,
    hexWidth, hexHeight,
    viewport
);

// Render object selections
selectionRenderer->RenderObjectSelections(
    hexWidth, hexHeight,
    viewport
);
```

---

### DamageDisplayRenderer

Floating damage numbers with fade and drift animation.

#### Display Types

```cpp
enum class DisplayType {
    NORMAL_DAMAGE = 0,    // Orange
    HEALING = 1,          // Green
    CRITICAL_DAMAGE = 2,  // Red (larger)
    MISS = 3,             // Gray
    ABSORBED = 4,         // Light blue
    RESOURCE = 5,         // Yellow
};
```

#### Showing Damage

```cpp
// Show damage number
damageDisplay->ShowDamage(
    worldX, worldY,                              // World position
    25,                                          // Damage amount
    DamageDisplayRenderer::DisplayType::NORMAL_DAMAGE
);

// Show healing
damageDisplay->ShowDamage(worldX, worldY, 15,
    DamageDisplayRenderer::DisplayType::HEALING);

// Show critical hit
damageDisplay->ShowDamage(worldX, worldY, 50,
    DamageDisplayRenderer::DisplayType::CRITICAL_DAMAGE);

// Show miss
damageDisplay->ShowDamage(0, 0,  // Damage amount ignored
    DamageDisplayRenderer::DisplayType::MISS);
```

#### Custom Text

```cpp
// Display arbitrary text
damageDisplay->ShowText(
    worldX, worldY,
    "+100 Gold",                           // Text
    glm::vec4(1, 1, 0, 1),                 // Yellow
    2.0f,                                  // Duration in seconds
    -40.0f                                 // Drift speed (upward)
);
```

#### Configuration

```cpp
// Set default duration (seconds)
damageDisplay->SetDefaultDuration(1.5f);

// Set default drift speed (pixels per second, negative = up)
damageDisplay->SetDefaultDriftY(-30.0f);

// Set default font size
damageDisplay->SetDefaultFontSize(16.0f);

// Limit active displays (older ones fade out faster)
damageDisplay->SetMaxDisplayCount(50);

// Customize color for each type
damageDisplay->SetTypeColor(
    DamageDisplayRenderer::DisplayType::CRITICAL_DAMAGE,
    glm::vec4(1, 0, 0, 1)  // Red
);
```

#### Animation Details

```
Duration: 1.5 seconds
Fade: Full opacity 0-1.2s, fade out 1.2-1.5s
Drift: Move -30 pixels/second (upward)
Update: Called each frame with deltaTime
```

#### Rendering

```cpp
// Update animations (call once per frame)
damageDisplay->Update(deltaTime);

// Render visible displays
damageDisplay->Render(viewport);

// Query active count
size_t count = damageDisplay->GetDisplayCount();
```

---

### StatusBarRenderer

Health, construction, experience, morale, and shield bars for units/buildings.

#### Bar Types

```cpp
enum class BarType {
    HEALTH = 0,       // Red
    CONSTRUCTION = 1, // Orange
    EXPERIENCE = 2,   // Blue
    MORALE = 3,       // Green
    SHIELD = 4,       // Cyan
};
```

#### Drawing Bars

```cpp
// Draw unit status bar
statusBar->DrawUnitStatusBar(
    "Unit Name",
    worldX, worldY,
    {
        {BarType::HEALTH, 0.75f, glm::vec4(1, 0, 0, 1)},      // 75% health
        {BarType::SHIELD, 0.50f, glm::vec4(0, 1, 1, 0.8)},     // 50% shield
    }
);

// Draw building status bar
statusBar->DrawBuildingStatusBar(
    "Building Name",
    worldX, worldY,
    {
        {BarType::CONSTRUCTION, 0.33f, glm::vec4(1, 0.5, 0, 1)},  // 33% done
    }
);
```

---

## Input System

### InputHandler

Central event routing system for mouse, keyboard, and text input.

#### Event Listener Interface

```cpp
class MyInputHandler : public InputHandler::InputListener {
public:
    // Return true to consume event, false to pass to next listener
    bool OnMouseClick(float x, float y, InputHandler::MouseButton button) override {
        if (button == InputHandler::MouseButton::LEFT) {
            HandleLeftClick(x, y);
            return true;  // Event consumed
        }
        return false;  // Pass to next listener
    }

    bool OnMouseRelease(float x, float y, InputHandler::MouseButton button) override {
        // Handle mouse release
        return false;
    }

    bool OnMouseMove(float x, float y, float deltaX, float deltaY) override {
        // Handle mouse movement
        return false;
    }

    bool OnMouseWheel(float x, float y, int deltaY) override {
        // deltaY > 0 = scroll up, < 0 = scroll down
        return false;
    }

    bool OnKeyPress(int keyCode, int modifiers) override {
        if (keyCode == SDLK_ESCAPE) {
            OnEscape();
            return true;
        }
        return false;
    }

    bool OnKeyRelease(int keyCode, int modifiers) override {
        return false;
    }

    bool OnTextInput(const std::string& text) override {
        // Unicode text input (for text fields)
        return false;
    }
};
```

#### Registration

```cpp
auto inputHandler = gameWindow->GetInputHandler();

// Create listener
auto myHandler = std::make_shared<MyInputHandler>();

// Register (last registered = highest priority)
inputHandler->RegisterListener(myHandler);

// Unregister
inputHandler->UnregisterListener(myHandler);

// Clear all
inputHandler->ClearListeners();
```

#### Mouse State

```cpp
// Get current position
float x, y;
inputHandler->GetMousePosition(x, y);

// Check button state
bool leftDown = inputHandler->IsMouseButtonPressed(
    InputHandler::MouseButton::LEFT
);

// Get movement since last frame
float deltaX, deltaY;
inputHandler->GetMouseDelta(deltaX, deltaY);
```

#### Keyboard State

```cpp
// Check if key is pressed
bool wPressed = inputHandler->IsKeyPressed(SDLK_w);

// Get modifier state (Ctrl, Shift, Alt)
int mods = inputHandler->GetKeyModifiers();
bool ctrlPressed = (mods & KMOD_CTRL) != 0;
bool shiftPressed = (mods & KMOD_SHIFT) != 0;
bool altPressed = (mods & KMOD_ALT) != 0;
```

#### Event Routing

```
Event Flow:
1. InputHandler::ProcessEvent(sdlEvent)
2. Route to listeners in reverse registration order
3. First listener to return true consumes event
4. Other listeners don't get called
5. Used for priority: Dialogs → UI → Game Logic
```

---

## UI System

### DialogRenderer

Modal and modeless dialogs with buttons and callbacks.

#### Creating Dialogs

```cpp
// Create dialog
auto dialog = std::make_shared<UIDialog>(
    "Confirm Action",      // Title
    "Are you sure?",       // Message
    true                   // Modal (blocks input to game world)
);

// Add buttons with callbacks
dialog->AddOKButton();
dialog->AddOKCancelButtons();
dialog->AddYesNoButtons();

// Custom buttons
dialog->AddButton("Custom", 999, [](int result) {
    std::cout << "Custom button clicked!" << std::endl;
});

// Show dialog
dialogRenderer->ShowDialog(dialog);
```

#### Result Codes

```cpp
enum class UIDialog::Result {
    NONE = 0,
    OK = 1,
    CANCEL = 2,
    YES = 3,
    NO = 4,
};

// Get result
if (dialog->IsOpen() == false) {
    int result = dialog->GetResult();
    if (result == (int)UIDialog::Result::OK) {
        // User clicked OK
    }
}
```

#### Dialog Management

```cpp
auto dialogRenderer = gameWindow->GetDialogRenderer();

// Check for open dialogs
bool hasDialogs = dialogRenderer->HasOpenDialogs();
size_t count = dialogRenderer->GetDialogCount();

// Get topmost dialog
auto topDialog = dialogRenderer->GetTopDialog();

// Close dialogs
dialogRenderer->CloseTopDialog();   // Remove top
dialogRenderer->CloseAllDialogs();  // Remove all
```

#### Input Handling

```
Keyboard:
- ESC: Cancel dialog
- ENTER: Activate first button

Mouse:
- Click button: Activate that button
- Click outside modal: No effect
```

---

### UIWidget

Base class for UI elements.

#### Types

```cpp
// Button
UIButton btn("Click Me");
btn.SetCallback([](){ std::cout << "Clicked!" << std::endl; });

// Progress bar
UIProgressBar progress;
progress.SetValue(0.75f);  // 75% complete
progress.ShowLabel(true);  // Shows "75%"

// Label
UILabel label("Status: Ready");
label.SetFontSize(12.0f);
label.SetAlignment(1);  // Center

// Panel (container)
UIPanel panel;
panel.SetSize(400, 300);
panel.AddChild(button);
panel.AddChild(label);
```

#### Properties

```cpp
// Position and size
widget->SetPosition(100, 200);
widget->SetSize(200, 50);
widget->GetX();
widget->GetY();
widget->GetWidth();
widget->GetHeight();

// Colors
widget->SetBackColor(glm::vec4(0.2f, 0.2f, 0.2f, 0.9f));
widget->SetForeColor(glm::vec4(1, 1, 1, 1));

// Visibility and interaction
widget->SetVisible(true);
widget->SetEnabled(false);
```

#### Hit Testing

```cpp
// Check if point is inside widget
bool inside = widget->IsPointInside(x, y);

// For panels, find child at point
auto child = panel->GetWidgetAt(x, y);
```

---

## Utility Systems

### AssetManager

Manages sprite atlases and assets.

#### Initialization

```cpp
auto assetManager = gameWindow->GetAssetManager();

// Load standard atlases (terrain, building, unit, vehicle, effect, misc)
bool success = assetManager->LoadStandardAtlases();

// Look for atlases in:
// data/atlases/terrain.atlas
// data/atlases/building.atlas
// data/atlases/unit.atlas
// etc.
```

#### Sprite Access

```cpp
// Get sprite by name (searches all atlases)
TextureAtlas::Sprite sprite = assetManager->GetSprite("unit/soldier");

// Sprite contains:
// - Texture ID
// - Atlas coordinates (u0, v0, u1, v1)
// - Dimensions
// - Metadata
```

---

### Viewport

Screen-to-world and world-to-screen coordinate transformation.

#### Conversion

```cpp
auto viewport = gameWindow->GetViewport();

// Screen to world
float worldX, worldY;
viewport->ScreenToWorld(screenX, screenY, worldX, worldY);

// World to screen
float screenX, screenY;
viewport->WorldToScreen(worldX, worldY, screenX, screenY);

// These account for:
// - Camera position
// - Zoom level
// - Isometric projection (if applicable)
```

---

## Common Usage Patterns

### Game Loop Integration

```cpp
class Game : public InputHandler::InputListener {
    void Initialize() {
        gameWindow = GameWindow::Create("Enemy Nations", 1024, 768);
        inputHandler = gameWindow->GetInputHandler();
        inputHandler->RegisterListener(shared_from_this());

        // Load assets, create units, etc.
    }

    void Update(float deltaTime) {
        // Update game logic
        // Update unit positions
        // Update camera
    }

    void Render() {
        // Queue all visible sprites to SpriteRenderer
        // Update selection/damage displays
        // Render happens automatically
    }

    bool OnMouseClick(float x, float y, InputHandler::MouseButton button) override {
        // Convert to world coordinates
        float worldX, worldY;
        gameWindow->GetViewport()->ScreenToWorld(x, y, worldX, worldY);

        // Check what was clicked
        HandleWorldClick(worldX, worldY);
        return true;  // Consumed
    }
};

// Main:
auto game = std::make_shared<Game>();
game->Initialize();
// GameWindow::Run() calls Update() and Render() automatically
gameWindow->Run();
```

### Rendering a Scene

```cpp
void RenderScene() {
    auto sprites = gameWindow->GetSpriteRenderer();
    auto terrain = gameWindow->GetTerrainRenderer();
    auto fog = gameWindow->GetFogOfWarRenderer();
    auto selection = gameWindow->GetSelectionRenderer();
    auto damage = gameWindow->GetDamageDisplayRenderer();

    // Render terrain
    terrain->Render(viewX, viewY, viewX + viewWidth, viewY + viewHeight,
                   64, 64, *viewport);

    // Render fog
    fog->Render(viewX, viewY, viewX + viewWidth, viewY + viewHeight,
               64, 64, *viewport);

    // Render selection
    selection->RenderHexSelections(viewX, viewY, viewX + viewWidth, viewY + viewHeight,
                                  64, 64, *viewport);

    // Queue game objects (units, buildings, effects)
    for (auto& unit : units) {
        SpriteRenderer::SpriteQuad quad;
        quad.position = glm::vec3(screenX, screenY, unit.screenY);
        quad.width = 64;
        quad.height = 64;
        quad.objectId = unit.id;
        sprites->QueueSprite(quad);
    }

    // Render all queued sprites
    sprites->Render();

    // Render floating damage
    damage->Render(*viewport);

    // Render dialogs (automatically in GameWindow::Render)
}
```

---

## Performance Considerations

### GPU Optimization
- Sprite batching reduces draw calls from 1000s to 10s
- Spatial grid culls 80-95% of objects
- Scissor test optimization for dirty rectangles

### Memory
- Total GPU memory: ~107 MB (estimated)
- System RAM: ~2 MB for rendering structures
- Vertex buffer: Reused per frame

### Frame Timing
- Game logic: 24 FPS (41.67ms per update)
- Rendering: 60 FPS (16.67ms per frame)
- Remainder carryover prevents frame skip jitter

---

**API Reference Complete**
**Last Updated: March 22, 2026**
