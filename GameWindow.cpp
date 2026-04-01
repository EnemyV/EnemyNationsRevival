#include "GameWindow.h"
#include "GameLogicWrapper.h"
#include "RenderingAdapter.h"
#include "rendering/SpriteRenderer.h"
#include "rendering/TextRenderer.h"
#include "rendering/TerrainRenderer.h"
#include "rendering/FogOfWarRenderer.h"
#include "rendering/SelectionRenderer.h"
#include "rendering/DamageDisplayRenderer.h"
#include "rendering/StatusBarRenderer.h"
#include "rendering/DialogRenderer.h"
#include "rendering/Viewport.h"
#include "rendering/AssetManager.h"
#include "rendering/OpenGLRenderDevice.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// SDL2 and OpenGL headers
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

// Windows headers for message boxes
#include <windows.h>

// Helper function for logging
static void LogToFile(const std::string& message) {
    // Try writing to temp directory first
    const char* temp = std::getenv("TEMP");
    std::string logPath = temp ? std::string(temp) + "\\GameWindow_Debug.log" : "GameWindow_Debug.log";

    std::ofstream log(logPath, std::ios::app);
    if (log.is_open()) {
        log << message << std::endl;
        log.close();
    }
    std::cerr << message << std::endl;
}

GameWindow::GameWindow(const std::string& title, int width, int height)
    : m_title(title),
      m_width(width),
      m_height(height) {
}

GameWindow::~GameWindow() {
    Cleanup();
}

std::shared_ptr<GameWindow> GameWindow::Create(const std::string& title, int width, int height) {
    auto window = std::make_shared<GameWindow>(title, width, height);

    LogToFile("Creating GameWindow: " + title);

    if (!window->InitializeSDL()) {
        LogToFile("ERROR: Failed to initialize SDL");
        return nullptr;
    }

    if (!window->InitializeOpenGL()) {
        LogToFile("ERROR: Failed to initialize OpenGL");
        return nullptr;
    }

    if (!window->InitializeRenderingSystems()) {
        LogToFile("ERROR: Failed to initialize rendering systems");
        return nullptr;
    }

    LogToFile("GameWindow initialized successfully");
    return window;
}

bool GameWindow::InitializeSDL() {
    LogToFile("Initializing SDL...");

    // Initialize SDL with video and timer subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        LogToFile("ERROR: SDL_Init failed: " + std::string(SDL_GetError()));
        return false;
    }

    LogToFile("SDL initialized");

    // Create SDL window with OpenGL context
    m_window = SDL_CreateWindow(
        m_title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        m_width, m_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    if (!m_window) {
        LogToFile("ERROR: Failed to create SDL window: " + std::string(SDL_GetError()));
        SDL_Quit();
        return false;
    }

    LogToFile("SDL window created: " + std::to_string(m_width) + "x" + std::to_string(m_height));
    return true;
}

bool GameWindow::InitializeOpenGL() {
    LogToFile("Initializing OpenGL...");

    // Set OpenGL attributes before creating context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    LogToFile("SDL OpenGL attributes set");

    // Create OpenGL context
    m_glContext = SDL_GL_CreateContext(m_window);

    if (!m_glContext) {
        LogToFile("ERROR: Failed to create OpenGL context: " + std::string(SDL_GetError()));
        return false;
    }

    LogToFile("OpenGL context created");

    // Enable vsync
    SDL_GL_SetSwapInterval(1);

    // Load OpenGL functions
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        LogToFile("ERROR: GLEW init failed: " + std::string((const char*)glewGetErrorString(err)));
        return false;
    }

    LogToFile("GLEW initialized");
    LogToFile("OpenGL Version: " + std::string((const char*)glGetString(GL_VERSION)));
    LogToFile("GLSL Version: " + std::string((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION)));
    return true;
}

bool GameWindow::InitializeRenderingSystems() {
    LogToFile("Initializing rendering systems...");

    // Create core systems
    LogToFile("Creating InputHandler...");
    m_inputHandler = std::make_unique<InputHandler>();
    LogToFile("Creating AssetManager...");
    m_assetManager = std::make_unique<AssetManager>();
    LogToFile("Creating Viewport...");
    m_viewport = std::make_unique<Viewport>(m_width, m_height);

    // Create OpenGL render device for GPU operations
    LogToFile("Creating OpenGL render device...");
    m_renderDevice = new OpenGLRenderDevice();
    if (!m_renderDevice) {
        LogToFile("ERROR: Failed to create OpenGL render device");
        return false;
    }

    // Create rendering systems in dependency order
    LogToFile("Creating SpriteRenderer...");
    m_spriteRenderer = std::make_unique<SpriteRenderer>(m_renderDevice);
    if (!m_spriteRenderer) {
        LogToFile("ERROR: Failed to create SpriteRenderer");
        return false;
    }

    // Initialize sprite renderer (compiles shaders, creates buffers)
    LogToFile("Initializing SpriteRenderer...");
    if (!m_spriteRenderer->Initialize()) {
        LogToFile("ERROR: Failed to initialize SpriteRenderer");
        return false;
    }

    LogToFile("Creating TextRenderer...");
    m_textRenderer = std::make_unique<TextRenderer>();
    if (!m_textRenderer) {
        LogToFile("ERROR: Failed to create TextRenderer");
        return false;
    }

    LogToFile("Creating remaining renderers...");
    m_terrainRenderer = std::make_unique<TerrainRenderer>(m_spriteRenderer.get());
    m_fogOfWarRenderer = std::make_unique<FogOfWarRenderer>(m_spriteRenderer.get());
    m_selectionRenderer = std::make_unique<SelectionRenderer>(m_spriteRenderer.get());
    m_damageDisplayRenderer = std::make_unique<DamageDisplayRenderer>(m_textRenderer.get());
    m_statusBarRenderer = std::make_unique<StatusBarRenderer>(m_spriteRenderer.get(), m_textRenderer.get());
    m_dialogRenderer = std::make_unique<DialogRenderer>(m_spriteRenderer.get(), m_textRenderer.get());

    // Load standard asset atlases
    LogToFile("Loading standard asset atlases...");
    if (!m_assetManager->LoadStandardAtlases()) {
        LogToFile("Warning: Failed to load standard asset atlases (may be acceptable)");
        // Don't fail - may be acceptable in dev environment
    }

    // Initialize rendering adapter for game logic integration
    LogToFile("Initializing RenderingAdapter...");
    RenderingAdapter::Initialize(this);

    // Register this window as input listener (gets priority after dialogs)
    LogToFile("Registering input listener...");
    m_inputHandler->RegisterListener(shared_from_this());

    LogToFile("Rendering systems initialized successfully");
    return true;
}

void GameWindow::Cleanup() {
    std::cout << "Cleaning up GameWindow..." << std::endl;

    // Clear all input listeners
    if (m_inputHandler) {
        m_inputHandler->ClearListeners();
    }

    // Clear rendering systems
    m_dialogRenderer.reset();
    m_statusBarRenderer.reset();
    m_damageDisplayRenderer.reset();
    m_selectionRenderer.reset();
    m_fogOfWarRenderer.reset();
    m_terrainRenderer.reset();
    m_textRenderer.reset();
    m_spriteRenderer.reset();

    // Clear core systems
    m_viewport.reset();
    m_assetManager.reset();
    m_inputHandler.reset();

    // Clean up render device
    if (m_renderDevice) {
        m_renderDevice->Shutdown();
        delete m_renderDevice;
        m_renderDevice = nullptr;
    }

    // Clean up OpenGL
    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }

    // Clean up SDL window
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    // Quit SDL
    SDL_Quit();

    std::cout << "GameWindow cleanup complete" << std::endl;
}

int GameWindow::Run() {
    if (!IsValid()) {
        std::cerr << "GameWindow is not valid, cannot run" << std::endl;
        return 1;
    }

    std::cout << "Starting main game loop" << std::endl;
    m_running = true;

    // Initialize frame timing
    uint64_t startTicks = SDL_GetTicks64();
    m_lastFrameTime = startTicks;

    int frameCount = 0;
    uint64_t lastSecond = startTicks;

    // Main game loop
    while (m_running) {
        // Get current time for this frame
        uint64_t currentTicks = SDL_GetTicks64();

        // Process SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            HandleEvent(&event);
        }

        // Calculate delta time
        float deltaTime = (currentTicks - m_lastFrameTime) / 1000.0f;
        m_lastFrameTime = currentTicks;

        // Accumulate frame time for 24 FPS updates with remainder carryover
        m_frameTimeRemainder += deltaTime;
        float updateDeltaTime = 1.0f / m_targetFrameRate;

        // Update game logic at target frame rate
        while (m_frameTimeRemainder >= updateDeltaTime) {
            Update(updateDeltaTime);
            m_frameTimeRemainder -= updateDeltaTime;
        }

        // Render frame
        Render();

        // Calculate frame rate every second
        frameCount++;
        if (currentTicks - lastSecond >= 1000) {
            m_currentFrameRate = (float)frameCount;
            frameCount = 0;
            lastSecond = currentTicks;
        }

        // Frame rate limiting
        uint64_t frameEndTicks = SDL_GetTicks64();
        uint64_t frameTimeMs = frameEndTicks - currentTicks;
        int targetFrameTimeMs = 1000 / 60;  // Target 60 FPS rendering

        if (frameTimeMs < targetFrameTimeMs) {
            SDL_Delay(targetFrameTimeMs - frameTimeMs);
        }
    }

    std::cout << "Main game loop ended" << std::endl;
    return 0;
}

void GameWindow::Update(float deltaTime) {
    // Update game logic if integrated
    if (m_gameLogic) {
        m_gameLogic->Update(deltaTime);
    }

    // Update all systems
    if (m_spriteRenderer) {
        m_spriteRenderer->Update(deltaTime);
    }

    if (m_terrainRenderer) {
        m_terrainRenderer->Update(deltaTime);
    }

    if (m_damageDisplayRenderer) {
        m_damageDisplayRenderer->Update(deltaTime);
    }

    if (m_dialogRenderer) {
        m_dialogRenderer->Update(deltaTime);
    }

    // Clear frame input
    if (m_inputHandler) {
        m_inputHandler->ClearFrameInput();
    }
}

void GameWindow::Render() {
    // Clear screen
    ClearScreen(0.2f, 0.2f, 0.2f, 1.0f);

    // Set up projection matrix (orthographic for 2D)
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_width),
                                       static_cast<float>(m_height), 0.0f,
                                       -1.0f, 1.0f);
    m_renderDevice->SetProjection(projection);

    // Set up view matrix (identity with camera offset)
    glm::mat4 view = glm::mat4(1.0f);
    m_renderDevice->SetView(view);

    // Get viewport bounds for culling
    float viewX = m_viewportX;
    float viewY = m_viewportY;
    float viewWidth = m_width / m_viewportZoom;
    float viewHeight = m_height / m_viewportZoom;

    // Render game logic (queues sprites to renderers)
    if (m_gameLogic) {
        m_gameLogic->Render();
    }

    // Render game world
    if (m_spriteRenderer && m_terrainRenderer) {
        // Terrain first
        m_terrainRenderer->Render(viewX, viewY, viewX + viewWidth, viewY + viewHeight,
                                  64.0f, 64.0f, *m_viewport);

        // Fog of war
        if (m_fogOfWarRenderer) {
            m_fogOfWarRenderer->Render(viewX, viewY, viewX + viewWidth, viewY + viewHeight,
                                      64.0f, 64.0f, *m_viewport);
        }

        // Selection highlights
        if (m_selectionRenderer) {
            m_selectionRenderer->RenderHexSelections(viewX, viewY, viewX + viewWidth, viewY + viewHeight,
                                                    64.0f, 64.0f, *m_viewport);
        }

        // Render batched sprites
        m_spriteRenderer->Render();
    }

    // Render floating damage/text
    if (m_damageDisplayRenderer) {
        m_damageDisplayRenderer->Render(*m_viewport);
    }

    // Render UI dialogs
    if (m_dialogRenderer) {
        m_dialogRenderer->Render();
    }

    SwapBuffers();
}

void GameWindow::HandleEvent(void* event) {
    if (!m_inputHandler) return;

    // Route event through input handler
    m_inputHandler->ProcessEvent(event);

    // Also check for quit events directly
    SDL_Event* sdlEvent = static_cast<SDL_Event*>(event);
    if (sdlEvent->type == SDL_QUIT) {
        m_running = false;
    }
}

void GameWindow::SetVisible(bool visible) {
    if (!m_window) return;

    if (visible) {
        SDL_ShowWindow(m_window);
    } else {
        SDL_HideWindow(m_window);
    }
}

void GameWindow::SwapBuffers() {
    if (!m_window) return;

    SDL_GL_SwapWindow(m_window);
}

void GameWindow::ClearScreen(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// === InputListener Implementation ===

bool GameWindow::OnMouseClick(float x, float y, InputHandler::MouseButton button) {
    // Handle window mouse clicks (e.g., UI interactions not handled by dialogs)
    // Return true if handled, false to continue routing
    return false;
}

bool GameWindow::OnMouseRelease(float x, float y, InputHandler::MouseButton button) {
    return false;
}

bool GameWindow::OnMouseMove(float x, float y, float deltaX, float deltaY) {
    return false;
}

bool GameWindow::OnMouseWheel(float x, float y, int deltaY) {
    // Handle zoom
    if (deltaY > 0) {
        m_viewportZoom *= 1.1f;
    } else {
        m_viewportZoom /= 1.1f;
    }

    // Clamp zoom range
    if (m_viewportZoom < 0.25f) m_viewportZoom = 0.25f;
    if (m_viewportZoom > 4.0f) m_viewportZoom = 4.0f;

    return true;
}

bool GameWindow::OnKeyPress(int keyCode, int modifiers) {
    // ESC closes the window
    if (keyCode == 27) {  // SDLK_ESCAPE
        m_running = false;
        return true;
    }

    // Arrow keys for viewport movement
    if (keyCode == 1073741903) {  // SDLK_RIGHT
        m_viewportX += 32.0f;
        return true;
    } else if (keyCode == 1073741904) {  // SDLK_LEFT
        m_viewportX -= 32.0f;
        return true;
    } else if (keyCode == 1073741905) {  // SDLK_DOWN
        m_viewportY += 32.0f;
        return true;
    } else if (keyCode == 1073741906) {  // SDLK_UP
        m_viewportY -= 32.0f;
        return true;
    }

    return false;
}

bool GameWindow::OnKeyRelease(int keyCode, int modifiers) {
    return false;
}

bool GameWindow::OnTextInput(const std::string& text) {
    return false;
}

/**
 * Set game logic wrapper for integrated game
 */
void GameWindow::SetGameLogic(std::shared_ptr<GameLogicWrapper> gameLogic) {
    if (!gameLogic) {
        std::cerr << "ERROR: Attempting to set null GameLogicWrapper" << std::endl;
        return;
    }

    m_gameLogic = gameLogic;

    std::cout << "Game logic integrated with GameWindow" << std::endl;
    std::cout << "  - Game logic wrapper set" << std::endl;
    std::cout << "  - Update/Render integration active" << std::endl;
    std::cout << "  - Input events will be routed to game logic" << std::endl;
}
