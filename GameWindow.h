// NOTE: This file is NOT compiled into the game. It was created during Phase 9
// planning as a design prototype for a full GPU rendering pipeline. The actual
// working GameWindow is in enations_latest/src/GameWindow.h/cpp.
// Some of the infrastructure here (OpenGL setup, batching, coordinate math) may
// be useful later when we replace DIB rendering with GPU rendering.
#pragma once

#include "input/InputHandler.h"
#include <memory>
#include <string>
#include <vector>

// Forward declarations
class SpriteRenderer;
class TextRenderer;
class TerrainRenderer;
class FogOfWarRenderer;
class SelectionRenderer;
class DamageDisplayRenderer;
class StatusBarRenderer;
class DialogRenderer;
class AssetManager;
class Viewport;
class OpenGLRenderDevice;
class GameLogicWrapper;
struct SDL_Window;
struct SDL_GLContext;

/**
 * Game window and main application
 * Manages SDL window, OpenGL context, rendering systems, input routing, and main game loop
 */
class GameWindow : public InputHandler::InputListener, public std::enable_shared_from_this<GameWindow> {
public:
    /**
     * Create and initialize game window
     * @param title Window title
     * @param width Window width in pixels
     * @param height Window height in pixels
     * @return Shared pointer to GameWindow instance, or nullptr on failure
     */
    static std::shared_ptr<GameWindow> Create(const std::string& title, int width, int height);

    /**
     * Destructor - cleans up SDL and OpenGL resources
     */
    ~GameWindow();

    /**
     * Check if window is valid and ready for use
     * @return True if initialized successfully, false otherwise
     */
    bool IsValid() const { return m_window != nullptr && m_glContext != nullptr; }

    /**
     * Get window width in pixels
     * @return Width
     */
    int GetWidth() const { return m_width; }

    /**
     * Get window height in pixels
     * @return Height
     */
    int GetHeight() const { return m_height; }

    /**
     * Get window title
     * @return Title string
     */
    const std::string& GetTitle() const { return m_title; }

    // === Rendering Systems ===

    /**
     * Get sprite renderer
     * @return SpriteRenderer pointer
     */
    SpriteRenderer* GetSpriteRenderer() const { return m_spriteRenderer.get(); }

    /**
     * Get text renderer
     * @return TextRenderer pointer
     */
    TextRenderer* GetTextRenderer() const { return m_textRenderer.get(); }

    /**
     * Get terrain renderer
     * @return TerrainRenderer pointer
     */
    TerrainRenderer* GetTerrainRenderer() const { return m_terrainRenderer.get(); }

    /**
     * Get fog of war renderer
     * @return FogOfWarRenderer pointer
     */
    FogOfWarRenderer* GetFogOfWarRenderer() const { return m_fogOfWarRenderer.get(); }

    /**
     * Get selection renderer
     * @return SelectionRenderer pointer
     */
    SelectionRenderer* GetSelectionRenderer() const { return m_selectionRenderer.get(); }

    /**
     * Get damage display renderer
     * @return DamageDisplayRenderer pointer
     */
    DamageDisplayRenderer* GetDamageDisplayRenderer() const { return m_damageDisplayRenderer.get(); }

    /**
     * Get status bar renderer
     * @return StatusBarRenderer pointer
     */
    StatusBarRenderer* GetStatusBarRenderer() const { return m_statusBarRenderer.get(); }

    /**
     * Get dialog renderer
     * @return DialogRenderer pointer
     */
    DialogRenderer* GetDialogRenderer() const { return m_dialogRenderer.get(); }

    /**
     * Get asset manager
     * @return AssetManager pointer
     */
    AssetManager* GetAssetManager() const { return m_assetManager.get(); }

    /**
     * Get viewport
     * @return Viewport pointer
     */
    Viewport* GetViewport() const { return m_viewport.get(); }

    /**
     * Get input handler
     * @return InputHandler pointer
     */
    InputHandler* GetInputHandler() const { return m_inputHandler.get(); }

    // === Game Logic Integration ===

    /**
     * Set game logic wrapper for integrated game
     * When set, game logic will be called from main loop
     * @param gameLogic Shared pointer to GameLogicWrapper
     */
    void SetGameLogic(std::shared_ptr<GameLogicWrapper> gameLogic);

    /**
     * Get game logic wrapper
     * @return GameLogicWrapper pointer or nullptr if not set
     */
    GameLogicWrapper* GetGameLogic() const { return m_gameLogic.get(); }

    /**
     * Check if game logic is integrated
     * @return True if game logic is active
     */
    bool HasGameLogic() const { return m_gameLogic != nullptr; }

    // === Main Loop ===

    /**
     * Run the main game loop
     * Processes events, updates, and renders until window closes or error occurs
     * @return Exit code (0 on success)
     */
    int Run();

    /**
     * Request window close (stops main loop)
     */
    void RequestClose() { m_running = false; }

    /**
     * Check if main loop is running
     * @return True if running, false otherwise
     */
    bool IsRunning() const { return m_running; }

    /**
     * Get current frame rate
     * @return Frames per second
     */
    float GetFrameRate() const { return m_currentFrameRate; }

    /**
     * Get target frame rate
     * @return Target frames per second (24 for game updates)
     */
    int GetTargetFrameRate() const { return m_targetFrameRate; }

    /**
     * Set target frame rate
     * @param fps Target frames per second
     */
    void SetTargetFrameRate(int fps) { m_targetFrameRate = fps; }

    // === Window Control ===

    /**
     * Set window visibility
     * @param visible True to show, false to hide
     */
    void SetVisible(bool visible);

    /**
     * Swap buffers and present frame
     */
    void SwapBuffers();

    /**
     * Clear screen with color
     * @param r Red component (0-1)
     * @param g Green component (0-1)
     * @param b Blue component (0-1)
     * @param a Alpha component (0-1)
     */
    void ClearScreen(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);

    /**
     * Update game logic (for MFC integration)
     * Called by main loop at variable rate
     * @param deltaTime Time since last update in seconds
     */
    void Update(float deltaTime);

    /**
     * Render one frame (for MFC integration)
     * Called by main loop at 60 FPS
     */
    void Render();

protected:
    // === InputListener Implementation ===

    bool OnMouseClick(float x, float y, InputHandler::MouseButton button) override;
    bool OnMouseRelease(float x, float y, InputHandler::MouseButton button) override;
    bool OnMouseMove(float x, float y, float deltaX, float deltaY) override;
    bool OnMouseWheel(float x, float y, int deltaY) override;
    bool OnKeyPress(int keyCode, int modifiers) override;
    bool OnKeyRelease(int keyCode, int modifiers) override;
    bool OnTextInput(const std::string& text) override;

private:
    // Constructor is private - use Create() factory method
    GameWindow(const std::string& title, int width, int height);

    // === Initialization ===

    /**
     * Initialize SDL
     * @return True on success, false on failure
     */
    bool InitializeSDL();

    /**
     * Initialize OpenGL
     * @return True on success, false on failure
     */
    bool InitializeOpenGL();

    /**
     * Initialize all rendering systems
     * @return True on success, false on failure
     */
    bool InitializeRenderingSystems();

    /**
     * Cleanup all resources
     */
    void Cleanup();

    // === Main Loop Helpers ===

    /**
     * Handle one SDL event
     * @param event SDL event pointer
     */
    void HandleEvent(void* event);

    // === State ===

    std::string m_title;
    int m_width;
    int m_height;

    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

    // Render device for GPU operations
    class OpenGLRenderDevice* m_renderDevice = nullptr;

    bool m_running = false;

    // Frame timing
    int m_targetFrameRate = 24;      // 24 FPS game updates
    float m_currentFrameRate = 0.0f;
    uint64_t m_lastFrameTime = 0;
    float m_frameTimeRemainder = 0.0f;  // Carries over for 24 FPS precision

    // Rendering systems
    std::unique_ptr<SpriteRenderer> m_spriteRenderer;
    std::unique_ptr<TextRenderer> m_textRenderer;
    std::unique_ptr<TerrainRenderer> m_terrainRenderer;
    std::unique_ptr<FogOfWarRenderer> m_fogOfWarRenderer;
    std::unique_ptr<SelectionRenderer> m_selectionRenderer;
    std::unique_ptr<DamageDisplayRenderer> m_damageDisplayRenderer;
    std::unique_ptr<StatusBarRenderer> m_statusBarRenderer;
    std::unique_ptr<DialogRenderer> m_dialogRenderer;

    // Core systems
    std::unique_ptr<AssetManager> m_assetManager;
    std::unique_ptr<Viewport> m_viewport;
    std::unique_ptr<InputHandler> m_inputHandler;

    // Game logic integration (optional)
    std::shared_ptr<GameLogicWrapper> m_gameLogic;

    // Camera/viewport state
    float m_viewportX = 0.0f;
    float m_viewportY = 0.0f;
    float m_viewportZoom = 1.0f;
};
