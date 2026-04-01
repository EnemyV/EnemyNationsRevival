#include "GameLogicWrapper.h"
#include "RenderingAdapter.h"
#include <iostream>
#include <cassert>

/**
 * Create and initialize game logic wrapper
 */
std::shared_ptr<GameLogicWrapper> GameLogicWrapper::Create() {
    auto wrapper = std::shared_ptr<GameLogicWrapper>(new GameLogicWrapper());
    return wrapper;
}

/**
 * Constructor
 */
GameLogicWrapper::GameLogicWrapper()
    : m_initialized(false),
      m_running(false),
      m_paused(false),
      m_gameLoaded(false),
      m_gameSpeed(1.0f),
      m_accumulator(0.0f),
      m_app(nullptr),
      m_gameWindow(nullptr) {
}

/**
 * Destructor
 */
GameLogicWrapper::~GameLogicWrapper() {
    if (m_initialized) {
        Shutdown();
    }
}

/**
 * Initialize game logic systems
 */
bool GameLogicWrapper::Initialize() {
    if (m_initialized) {
        std::cerr << "GameLogicWrapper already initialized" << std::endl;
        return false;
    }

    std::cout << "Initializing GameLogicWrapper..." << std::endl;

    // Initialize original game systems
    if (!InitializeGameSystems()) {
        std::cerr << "Failed to initialize game systems" << std::endl;
        return false;
    }

    m_running = true;
    m_initialized = true;

    std::cout << "GameLogicWrapper initialized successfully" << std::endl;
    std::cout << "  - Game speed: " << m_gameSpeed << "x" << std::endl;
    std::cout << "  - Update frequency: 24 FPS (fixed)" << std::endl;

    return true;
}

/**
 * Initialize original game systems
 * This would set up the old game logic without the MFC window
 */
bool GameLogicWrapper::InitializeGameSystems() {
    // TODO: Initialize CConquerApp and CWndArea
    // This is where we would:
    // 1. Create CConquerApp instance
    // 2. Create CWndArea (game view)
    // 3. Initialize game world/units/buildings
    // 4. Register with RenderingAdapter
    //
    // For now, this is a placeholder that will be filled in
    // when we integrate with the actual game code

    std::cout << "  Initializing game systems (TODO: full integration)" << std::endl;

    // Set up rendering adapter integration
    // This tells the adapter which rendering functions to intercept
    // RenderingAdapter::SetAnimAtr(m_gameWindow->GetAnimAtr());

    return true;
}

/**
 * Shutdown game logic systems
 */
void GameLogicWrapper::Shutdown() {
    if (!m_initialized) {
        return;
    }

    std::cout << "Shutting down GameLogicWrapper..." << std::endl;

    ShutdownGameSystems();

    m_running = false;
    m_initialized = false;

    std::cout << "GameLogicWrapper shutdown complete" << std::endl;
}

/**
 * Shutdown original game systems
 */
void GameLogicWrapper::ShutdownGameSystems() {
    // TODO: Clean up CConquerApp and CWndArea
    // Ensure all game resources are properly released
    m_app.reset();
    m_gameWindow = nullptr;
}

/**
 * Update game logic (24 FPS)
 */
void GameLogicWrapper::Update(float deltaTime) {
    if (!m_initialized || !m_running) {
        return;
    }

    // Apply game speed multiplier
    deltaTime *= m_gameSpeed;

    // Accumulate time for fixed update rate
    m_accumulator += deltaTime;

    // Process all accumulated game updates
    ProcessAccumulatedTime(deltaTime);
}

/**
 * Process accumulated time for fixed 24 FPS updates
 */
void GameLogicWrapper::ProcessAccumulatedTime(float deltaTime) {
    if (m_paused) {
        m_accumulator = 0.0f;
        return;
    }

    // Dispatch game updates at fixed 24 FPS rate
    while (m_accumulator >= FIXED_UPDATE_TIME) {
        DispatchGameUpdate();
        m_accumulator -= FIXED_UPDATE_TIME;
    }
}

/**
 * Dispatch a single game update
 * This is called at fixed 24 FPS rate
 */
void GameLogicWrapper::DispatchGameUpdate() {
    // TODO: Call game's update functions:
    // - CWorld::Update() or equivalent
    // - CUnit::Update() for each unit
    // - CAi::Update() for AI decisions
    // - Update animations, etc.
    //
    // This is the core game logic tick
}

/**
 * Render game view
 * Queues all visible objects to GPU renderer
 *
 * Strategy: Iterate theMap directly and queue sprites
 * No need to call old MFC rendering code - just read game state and queue
 */
void GameLogicWrapper::Render() {
    if (!m_initialized || !m_running) {
        return;
    }

    // TODO: Implement sprite queuing by iterating theMap
    //
    // This is the straightforward approach:
    // 1. Iterate all hexes in visible viewport region
    // 2. For each hex with a building: call RenderingAdapter::QueueBuildingSprite(building)
    // 3. For each hex with units: call RenderingAdapter::QueueUnitSprite(unit)
    // 4. Call RenderingAdapter::Render() to flush queues
    //
    // Requires:
    // - Access to theMap (CGameMap global) - AVAILABLE
    // - Access to theBuildingHex (CBuildingHex global) - AVAILABLE
    // - Access to theVehicleHex (CVehicleHex global) - AVAILABLE
    // - Viewport bounds from CAnimAtr (for culling) - NEED TO SETUP
    //
    // For now, this is stubbed. Will implement once we have CAnimAtr or camera system.

    // Delegate to RenderingAdapter for now (no-op until above is implemented)
    RenderingAdapter::Render();
}

/**
 * Handle mouse click
 */
bool GameLogicWrapper::OnMouseClick(int x, int y, int button) {
    if (!m_initialized || !m_running || m_paused) {
        return false;
    }

    RouteInputToGameLogic(x, y, button);
    return true;
}

/**
 * Handle mouse move
 */
void GameLogicWrapper::OnMouseMove(int x, int y) {
    if (!m_initialized || !m_running) {
        return;
    }

    UpdateGameCamera(x, y);
}

/**
 * Handle keyboard input
 */
bool GameLogicWrapper::OnKeyPress(int keyCode, bool pressed) {
    if (!m_initialized || !m_running) {
        return false;
    }

    // TODO: Route key presses to game logic
    // This would handle:
    // - Unit selection/commands
    // - Camera controls (arrow keys, etc.)
    // - UI interactions
    // - Game commands (pause, save, etc.)

    return false;
}

/**
 * Handle window resize
 */
void GameLogicWrapper::OnWindowResize(int width, int height) {
    if (!m_initialized) {
        return;
    }

    // TODO: Recalculate viewport and camera
    // This would call CAnimAtr::Resized() or equivalent
}

/**
 * Load game from file
 */
bool GameLogicWrapper::LoadGame(const char* filePath) {
    if (!m_initialized) {
        return false;
    }

    std::cout << "Loading game from: " << filePath << std::endl;

    // TODO: Load save game file
    // This would deserialize the game state from disk
    // All original save format preserved

    m_gameLoaded = true;
    return true;
}

/**
 * Save game to file
 */
bool GameLogicWrapper::SaveGame(const char* filePath) {
    if (!m_initialized || !m_gameLoaded) {
        return false;
    }

    std::cout << "Saving game to: " << filePath << std::endl;

    // TODO: Save game state to file
    // Uses original save format, no changes

    return true;
}

/**
 * Start new game
 */
bool GameLogicWrapper::NewGame(const char* mapName) {
    if (!m_initialized) {
        return false;
    }

    std::cout << "Starting new game with map: " << mapName << std::endl;

    // TODO: Load map and initialize game
    // This would:
    // 1. Load map data
    // 2. Create units/buildings
    // 3. Initialize player state
    // 4. Register with RenderingAdapter

    m_gameLoaded = true;
    return true;
}

/**
 * Set paused state
 */
void GameLogicWrapper::SetPaused(bool paused) {
    m_paused = paused;

    if (m_paused) {
        std::cout << "Game paused" << std::endl;
    } else {
        std::cout << "Game resumed" << std::endl;
    }
}

/**
 * Set game speed multiplier
 */
void GameLogicWrapper::SetGameSpeed(float speed) {
    // Clamp between 0.1x and 4.0x
    if (speed < 0.1f) speed = 0.1f;
    if (speed > 4.0f) speed = 4.0f;

    m_gameSpeed = speed;
    std::cout << "Game speed: " << (speed * 100) << "%" << std::endl;
}

/**
 * Route input to game logic
 */
void GameLogicWrapper::RouteInputToGameLogic(int x, int y, int button) {
    // TODO: Convert screen coords to game coords using RenderingAdapter
    // Then dispatch to game's input handling
    //
    // glm::vec3 gameCoords = RenderingAdapter::ScreenToWorldCoords(glm::vec3(x, y, 0));
    // m_gameWindow->OnLButtonDown(gameCoords.x, gameCoords.y); // or similar
}

/**
 * Update game camera
 */
void GameLogicWrapper::UpdateGameCamera(int mouseX, int mouseY) {
    // TODO: Handle camera panning based on mouse position
    // This might pan the viewport when mouse is near edges
}

/**
 * Validation and debugging
 */
void GameLogicWrapper::PrintDebugInfo() const {
    std::cout << "\n=== GameLogicWrapper Debug Info ===" << std::endl;
    std::cout << "Initialized: " << (m_initialized ? "YES" : "NO") << std::endl;
    std::cout << "Running: " << (m_running ? "YES" : "NO") << std::endl;
    std::cout << "Paused: " << (m_paused ? "YES" : "NO") << std::endl;
    std::cout << "Game loaded: " << (m_gameLoaded ? "YES" : "NO") << std::endl;
    std::cout << "Game speed: " << (m_gameSpeed * 100) << "%" << std::endl;
    std::cout << "Accumulator: " << m_accumulator << "s" << std::endl;
    std::cout << "====================================\n" << std::endl;
}

/**
 * Validate wrapper state
 */
bool GameLogicWrapper::ValidateState() const {
    if (!m_initialized) {
        std::cerr << "WARNING: GameLogicWrapper not initialized" << std::endl;
        return false;
    }

    if (!m_running) {
        std::cerr << "WARNING: Game is not running" << std::endl;
        return false;
    }

    if (!m_gameLoaded) {
        std::cerr << "WARNING: No game loaded" << std::endl;
        return false;
    }

    return true;
}
