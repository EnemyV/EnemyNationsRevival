# TODO Implementation Specification

## Complete List of SDL2/OpenGL Functions Requiring Implementation

This document lists all ~25 TODO: placeholders in the codebase with exact specifications for implementation.

---

## GameWindow.cpp TODO: Items (14 functions)

### 1. InitializeSDL() - Line ~54

**Current Code:**
```cpp
// TODO: SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)
// TODO: m_window = SDL_CreateWindow(...)
```

**Implementation:**
```cpp
bool GameWindow::InitializeSDL() {
    // Initialize SDL with video and timer subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create SDL window with OpenGL support
    m_window = SDL_CreateWindow(
        m_title.c_str(),                    // Window title
        SDL_WINDOWPOS_CENTERED,             // X position (centered)
        SDL_WINDOWPOS_CENTERED,             // Y position (centered)
        m_width,                            // Window width
        m_height,                           // Window height
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN  // Flags: OpenGL + visible
    );

    if (!m_window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    return true;
}
```

**Dependencies:** SDL2 library

---

### 2. InitializeOpenGL() - Line ~72

**Current Code:**
```cpp
// TODO: SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
// TODO: SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
// TODO: SDL_GL_CreateContext(m_window);
// TODO: glewInit();
```

**Implementation:**
```cpp
bool GameWindow::InitializeOpenGL() {
    // Set OpenGL context attributes BEFORE creating context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Create OpenGL context
    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Enable vsync (1 = wait for vsync, 0 = no vsync)
    SDL_GL_SetSwapInterval(1);

    // Initialize GLEW to load OpenGL function pointers
    glewExperimental = GL_TRUE;  // Needed for core profile
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "glewInit failed: " << glewGetErrorString(glewErr) << std::endl;
        return false;
    }

    // Print OpenGL version for debugging
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    return true;
}
```

**Dependencies:** SDL2, GLEW

---

### 3. Cleanup() - Line ~120

**Current Code:**
```cpp
// TODO: SDL_GL_DeleteContext(m_glContext);
// TODO: SDL_DestroyWindow(m_window);
// TODO: SDL_Quit();
```

**Implementation:**
```cpp
void GameWindow::Cleanup() {
    // ... (code before the TODOs remains)

    // Delete OpenGL context
    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }

    // Destroy SDL window
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    // Quit SDL
    SDL_Quit();
}
```

**Dependencies:** SDL2

---

### 4. Run() - Frame Timing (Line ~220)

**Current Code:**
```cpp
// TODO: uint64_t currentTicks = SDL_GetTicks64();
// TODO: uint64_t startTicks = SDL_GetTicks64();
```

**Implementation:**
```cpp
// At start of Run():
uint64_t startTicks = SDL_GetTicks64();
m_lastFrameTime = startTicks;

// In main loop (inside while(m_running)):
uint64_t currentTicks = SDL_GetTicks64();
float deltaTime = (currentTicks - m_lastFrameTime) / 1000.0f;  // Convert to seconds
m_lastFrameTime = currentTicks;
```

**Dependencies:** SDL2

---

### 5. Run() - Event Processing (Line ~235)

**Current Code:**
```cpp
// TODO: SDL_Event event;
// TODO: while (SDL_PollEvent(&event)) {
// TODO:     HandleEvent(&event);
// TODO: }
```

**Implementation:**
```cpp
SDL_Event event;
while (SDL_PollEvent(&event)) {
    HandleEvent(&event);
}
```

**Dependencies:** SDL2

---

### 6. Run() - Frame Limiting (Line ~285)

**Current Code:**
```cpp
// TODO: uint64_t frameTimeMs = SDL_GetTicks64() - currentTicks;
// TODO: if (frameTimeMs < (1000 / m_targetFrameRate)) {
// TODO:     SDL_Delay((1000 / m_targetFrameRate) - frameTimeMs);
// TODO: }
```

**Implementation:**
```cpp
// At end of frame loop, before incrementing frameCount
uint64_t frameEndTicks = SDL_GetTicks64();
uint64_t frameTimeMs = frameEndTicks - currentTicks;
int targetFrameTimeMs = 1000 / 60;  // Target 60 FPS rendering

if (frameTimeMs < targetFrameTimeMs) {
    SDL_Delay(targetFrameTimeMs - frameTimeMs);
}
```

**Dependencies:** SDL2

---

### 7. SetVisible() - Line ~320

**Current Code:**
```cpp
// TODO: if (visible) {
// TODO:     SDL_ShowWindow(m_window);
// TODO: } else {
// TODO:     SDL_HideWindow(m_window);
// TODO: }
```

**Implementation:**
```cpp
void GameWindow::SetVisible(bool visible) {
    if (!m_window) return;

    if (visible) {
        SDL_ShowWindow(m_window);
    } else {
        SDL_HideWindow(m_window);
    }
}
```

**Dependencies:** SDL2

---

### 8. SwapBuffers() - Line ~330

**Current Code:**
```cpp
// TODO: SDL_GL_SwapWindow(m_window);
```

**Implementation:**
```cpp
void GameWindow::SwapBuffers() {
    if (!m_window) return;
    SDL_GL_SwapWindow(m_window);
}
```

**Dependencies:** SDL2

---

### 9. ClearScreen() - Line ~340

**Current Code:**
```cpp
// TODO: glClearColor(r, g, b, a);
// TODO: glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

**Implementation:**
```cpp
void GameWindow::ClearScreen(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
```

**Dependencies:** OpenGL (GLEW)

---

## InputHandler.cpp TODO: Items (8 functions)

### 10-17. ProcessEvent() - Event Structure Parsing

**File:** src/input/InputHandler.cpp - Lines 100-200

**Current Code:**
```cpp
// SDL event type constants defined in code
#define SDL_MOUSEMOTION 4
#define SDL_MOUSEBUTTONDOWN 5
// ... etc

// Manual SDL_Event field extraction
uint32_t eventType = *(uint32_t*)eventData;
int x = *(int*)(eventData + 8);
```

**Implementation Notes:**
The event structure parsing is already correctly implemented using manual offset calculations. However, you may want to replace this with proper SDL types for clarity:

**Optional Improvement (Requires SDL header):**
```cpp
void InputHandler::ProcessEvent(void* eventPtr) {
    SDL_Event* event = static_cast<SDL_Event*>(eventPtr);

    switch (event->type) {
        case SDL_MOUSEMOTION: {
            float deltaX = static_cast<float>(event->motion.xrel);
            float deltaY = static_cast<float>(event->motion.yrel);
            m_mouseX = static_cast<float>(event->motion.x);
            m_mouseY = static_cast<float>(event->motion.y);
            m_mouseDeltaX += deltaX;
            m_mouseDeltaY += deltaY;
            RouteMouseMove(m_mouseX, m_mouseY, deltaX, deltaY);
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            MouseButton button = static_cast<MouseButton>(event->button.button);
            m_mouseX = static_cast<float>(event->button.x);
            m_mouseY = static_cast<float>(event->button.y);
            m_mouseButtonPressed[(int)button] = true;
            RouteMouseClick(m_mouseX, m_mouseY, button);
            break;
        }
        // ... similar for other event types
    }
}
```

**Dependencies:** SDL2 (if using proper types)

---

### 18. UpdateKeyboardState() - Line ~240

**Current Code:**
```cpp
void InputHandler::UpdateKeyboardState() {
    // This would be called from the main game loop using SDL_GetKeyboardState
    // For now, this is a placeholder
}
```

**Implementation:**
```cpp
void InputHandler::UpdateKeyboardState() {
    // Get keyboard state from SDL
    m_keyboardState = SDL_GetKeyboardState(&m_numKeys);

    // Get current modifier state (Ctrl, Shift, Alt)
    m_keyModifiers = SDL_GetModState();
}
```

**Usage in GameWindow::Run():**
```cpp
// Call once per frame after handling events
inputHandler->UpdateKeyboardState();
```

**Dependencies:** SDL2

---

## SpriteRenderer.cpp TODO: Items (varies)

The SpriteRenderer will need OpenGL state management:

### 19. Initialize GPU Resources

**Location:** SpriteRenderer constructor or initialization method

```cpp
void SpriteRenderer::Initialize() {
    // Create vertex array object (VAO)
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Create vertex buffer object (VBO)
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Reserve space for max sprites
    size_t maxVertices = MAX_SPRITES * 6;  // 6 vertices per quad
    glBufferData(GL_ARRAY_BUFFER,
                 maxVertices * sizeof(Vertex),
                 nullptr,
                 GL_DYNAMIC_DRAW);

    // Setup vertex attributes
    // Position (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                         sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // TexCoord (2 floats)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                         sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(1);

    // Color (4 floats)
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
                         sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}
```

**Dependencies:** OpenGL

---

### 20. Render Batched Sprites

**Location:** SpriteRenderer::Render()

```cpp
void SpriteRenderer::Render() {
    if (m_spriteQuads.empty()) return;

    glBindVertexArray(m_vao);

    Vertex* vertices = new Vertex[m_spriteQuads.size() * 6];
    size_t vertexCount = 0;

    // Build vertex data for all queued sprites
    for (const auto& quad : m_spriteQuads) {
        // Generate quad vertices (6 per quad for 2 triangles)
        // Fill in position, texture coordinates, and color
        vertexCount += 6;
    }

    // Upload to GPU
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                   vertexCount * sizeof(Vertex), vertices);

    // Render
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);

    delete[] vertices;
}
```

**Dependencies:** OpenGL

---

## TextRenderer.cpp TODO: Items (varies)

### 21. Load SDF Font

**Location:** TextRenderer::LoadFont()

```cpp
bool TextRenderer::LoadFont(const std::string& fontPath, int size) {
    // Initialize font rendering system
    // Load SDF font data
    // Create texture atlases for glyphs
    // Generate vertex data for text rendering

    // This requires either:
    // 1. A pre-baked SDF texture atlas
    // 2. Using a library like freetype to generate SDFs
    // 3. Loading pre-computed glyph metrics

    return true;
}
```

**Dependencies:** Font file format (custom or standardized)

---

## TerrainRenderer.cpp TODO: Items (varies)

### 22. Render Hex Grid

**Location:** TerrainRenderer::Render()

```cpp
void TerrainRenderer::Render(float minX, float minY, float maxX, float maxY,
                             float hexWidth, float hexHeight, Viewport& viewport) {
    // For each visible hex in the map:
    // 1. Calculate world position
    // 2. Convert to screen coordinates
    // 3. Calculate terrain shading
    // 4. Queue sprite to SpriteRenderer with shading color

    for (int y = minHexY; y < maxHexY; ++y) {
        for (int x = minHexX; x < maxHexX; ++x) {
            // Calculate shade based on corner heights
            int shade = CalculateShade(GetCornerHeights(x, y));

            // Queue terrain hex sprite
            SpriteRenderer::SpriteQuad quad;
            quad.position = glm::vec3(screenX, screenY, -1.0f);  // Terrain is background
            quad.width = hexWidth;
            quad.height = hexHeight;
            quad.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) * (shade / 7.0f);  // Darken by shade
            // ... set other quad properties

            m_spriteRenderer->QueueSprite(quad);
        }
    }
}
```

**Dependencies:** AssetManager for terrain sprites

---

## Summary of Implementation Order (Phase 7)

### Week 1: Core Integration
1. **InitializeSDL()** - Get window running
2. **InitializeOpenGL()** - Get graphics context
3. **SwapBuffers()** / **ClearScreen()** - Basic rendering
4. **Event Processing** - Mouse/keyboard input
5. **Frame Timing** - 24 FPS game loop

### Week 2: Rendering Systems
6. **SpriteRenderer GPU setup** - Vertex buffers, VAOs
7. **SpriteRenderer Rendering** - Draw batched sprites
8. **TerrainRenderer** - Render hex grid
9. **TextRenderer** - Render text with SDF

### Week 3: Polish
10. **Performance optimization** - Profile and tune
11. **Cross-platform testing** - Windows/Linux/macOS
12. **Documentation** - API docs and guides

---

## Build & Linking Verification Checklist

- [ ] SDL2 development libraries installed
- [ ] OpenGL headers (Windows: included in SDK, Linux: libgl1-mesa-dev)
- [ ] GLEW development libraries installed
- [ ] glm headers installed
- [ ] CMakeLists.txt finds all dependencies
- [ ] Project compiles without errors
- [ ] Project links without errors
- [ ] No undefined reference errors for SDL/GL functions
- [ ] Application runs and creates window
- [ ] OpenGL context is active (glClear works)
- [ ] Input events are processed

---

## Common Implementation Mistakes to Avoid

1. **Creating OpenGL context before SDL window** - Must create window first
2. **Not setting OpenGL attributes before creating context** - Set before CreateContext()
3. **Not calling glewInit()** - Required to load GL function pointers
4. **Using OpenGL 4.x features with 3.3 context** - Stick to GL 3.3 Core
5. **Not binding VAO before drawing** - VAO stores vertex state
6. **Buffer overflow in sprite batching** - Check max sprite count
7. **Not handling SDL event types correctly** - Use SDL_EventType constants
8. **Memory leaks in frame loop** - Don't allocate per-frame in render
9. **Blocking on input** - Use SDL_PollEvent, not SDL_WaitEvent
10. **Not checking for errors** - Always check SDL_GetError() and glGetError()

---

## Testing Each Component

### 1. Test SDL Initialization
```cpp
// Should see window appear
auto window = GameWindow::Create("Test", 800, 600);
assert(window && window->IsValid());
```

### 2. Test OpenGL Context
```cpp
// Should print OpenGL version
// Check console output for GL_VERSION string
```

### 3. Test Rendering
```cpp
// Render a simple quad
// Should see shape on screen
```

### 4. Test Input
```cpp
// Mouse movement should pan viewport
// Keyboard should work in dialogs
// Check console for input events
```

### 5. Test Frame Timing
```cpp
// Game should update at exactly 24 FPS
// Monitor remainder carryover
// Check for smooth animations
```

---

## Reference Documentation

- **SDL2 Documentation**: https://wiki.libsdl.org/
- **OpenGL 3.3 Reference**: https://www.khronos.org/registry/OpenGL/specs/gl/glspec33.core.pdf
- **GLEW Documentation**: http://glew.sourceforge.net/
- **GLM Documentation**: https://glm.g-truc.net/0.9.8/index.html

---

**Total TODO: Functions: ~25**
**Estimated Implementation Time: 14 working days**
**Complexity: Medium (straightforward SDL/GL calls)**
