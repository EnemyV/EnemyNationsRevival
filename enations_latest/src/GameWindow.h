#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../input/InputHandler.h"

union SDL_Event;
struct SDL_Window;
struct SDL_Renderer;
class SDLButtonManager;
class StatusBar;
class UIButtonListener;
class SDL2MainMenu;
class SDL2Compositor;
class SDL2Panel;
class SDL2Dialog;

/**
 * Minimal SDL2 Game Window
 * Creates and manages an SDL2 window for game rendering.
 * Must be created and used on the main thread (same thread as the game loop).
 * Call PollEvents() each frame from BaseYield() to process SDL events.
 */
class GameWindow {
public:
    /**
     * Create and initialize game window (on the calling thread)
     */
    static std::shared_ptr<GameWindow> Create(const std::string& title, int width, int height);

    /**
     * Destructor
     */
    ~GameWindow();

    /**
     * Check if window is valid
     */
    bool IsValid() const { return m_window != nullptr; }

    /**
     * Process all pending SDL events.
     * Called each frame from the main game loop (BaseYield).
     * Returns true if SDL_QUIT was received (app should exit).
     */
    bool PollEvents();

    /**
     * Get window width
     */
    int GetWidth() const { return m_width; }

    /**
     * Get window height
     */
    int GetHeight() const { return m_height; }

    /**
     * Get SDL window pointer
     */
    SDL_Window* GetWindow() const { return m_window; }

    /**
     * Swap buffers and present frame
     */
    void SwapBuffers();

    // ---- T0 GPU present path (gated by [Advanced] Renderer profile int) ----
    // The main game window is presented in one of two ways:
    //   software (Renderer=0, default): SDL_GetWindowSurface / SDL_UpdateWindowSurface
    //   renderer (Renderer=1): draw into an offscreen back-buffer surface, then
    //     upload it to a streaming texture and present via SDL_Renderer (VSync'd,
    //     and the layer GPU terrain composites onto).
    // Every main-window present site funnels through these two calls so the two
    // paths are byte-identical at Renderer=0. Secondary/own windows are unaffected.
    bool IsRendererMode() const { return m_useRenderer; }

    // Back-buffer to draw the main window's frame into. Software mode returns the
    // window surface (unchanged behavior); renderer mode returns the persistent
    // offscreen surface (recreated to match output size on resize). May be null.
    SDL_Surface* GetPresentSurface();

    // Present whatever was drawn into GetPresentSurface() to the main window.
    // Software: SDL_UpdateWindowSurface. Renderer: upload + RenderCopy + Present.
    // `dirty` (optional): only the given rect of the back-buffer changed since the
    // last present, so upload just that rect (the rest of the persistent back-texture
    // is still valid) instead of the whole 2560x1440 surface. The GPU RenderCopy still
    // draws the full texture, so the unchanged area is preserved. Ignored (full upload)
    // right after the back-buffer was (re)created, since the texture is then garbage.
    void PresentSurface(const SDL_Rect* dirty = nullptr);

    /**
     * Clear screen with color
     */
    void ClearScreen(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);

    /**
     * Update (placeholder for now)
     */
    void Update(float deltaTime) {}

    /**
     * Render UI (buttons and status bar)
     */
    void Render();

    /**
     * Constructor (public for make_shared)
     */
    GameWindow(const std::string& title, int width, int height);

    /**
     * Update window title with game information
     * Builds title as: "Enemy Nations - {PlayerName} ({Faction}) - {Mode}"
     * @param playerName Player's name (empty string = not set)
     * @param factionName Faction name (empty string = not set)
     * @param gameMode Game mode (e.g., "Single Player", "Multiplayer")
     */
    void SetGameInfo(const std::string& playerName, const std::string& factionName, const std::string& gameMode);

    /**
     * Update just the player name in title
     * @param playerName Player's name
     */
    void SetPlayerName(const std::string& playerName);

    /**
     * Update just the faction name in title
     * @param factionName Faction name
     */
    void SetFactionName(const std::string& factionName);

    /**
     * Update just the game mode in title
     * @param gameMode Game mode string
     */
    void SetGameMode(const std::string& gameMode);

    /**
     * Get current player name
     * @return Player name
     */
    const std::string& GetPlayerName() const { return m_playerName; }

    /**
     * Get current faction name
     * @return Faction name
     */
    const std::string& GetFactionName() const { return m_factionName; }

    /**
     * Get current game mode
     * @return Game mode
     */
    const std::string& GetGameMode() const { return m_gameMode; }

    /**
     * Get button manager for UI interaction
     * @return Pointer to button manager, or nullptr if not initialized
     */
    SDLButtonManager* GetButtonManager() const { return m_buttonManager.get(); }

    /**
     * Get status bar for info display
     * @return Pointer to status bar, or nullptr if not initialized
     */
    StatusBar* GetStatusBar() const { return m_statusBar.get(); }

    /**
     * Get SDL renderer for UI rendering
     * @return Pointer to renderer, or nullptr if not initialized
     */
    SDL_Renderer* GetRenderer() const { return m_renderer; }

    /**
     * Update status bar with game state
     * Called each frame from game loop
     */
    void UpdateStatusBar();

    /**
     * Get the input handler
     * @return Pointer to InputHandler
     */
    InputHandler* GetInputHandler() const { return m_inputHandler.get(); }

    /**
     * Set the active main menu (for event routing and rendering).
     * Pass nullptr to deactivate.
     */
    void SetMainMenu(SDL2MainMenu* menu) { m_mainMenu = menu; }
    SDL2MainMenu* GetMainMenu() const { return m_mainMenu; }

    // Compositor (Phase 1 — backplate + panel compositing)
    SDL2Compositor* GetCompositor() const { return m_compositor.get(); }

    // Active create-status dialog (Phase 2 — non-modal, not owned)
    void SetCreateStatus(class SDL2CreateStatus* status) { m_createStatus = status; }
    class SDL2CreateStatus* GetCreateStatus() const { return m_createStatus; }

    // Native SDL2 toolbar (replaces CWndBar PrintWindow capture)
    class SDL2Toolbar* GetSDL2Toolbar() const { return m_sdl2Toolbar; }
    void SetSDL2Toolbar(class SDL2Toolbar* tb) { m_sdl2Toolbar = tb; }

    // Force the SDL arrow cursor visible. Used by detached SDL panels (e.g.
    // CWndArea's map window) where SDL owns the cursor and Win32 ::SetCursor
    // calls from game code get overridden by SDL's WM_SETCURSOR handler.
    void SetArrowCursor();

    // Drive the Win32 ShowCursor display counter back to 0 without changing
    // the cursor shape. Game code's ::SetCursor(m_hCurReg / m_hCurMove / ...)
    // calls are then visible. Call this each frame from cursor-aware hot paths
    // (e.g. CWndArea's SDL panel MOUSEMOTION callback) to compensate for
    // legacy ShowCursor(FALSE) calls (intro movie etc.) that left the counter
    // negative.
    void EnsureCursorVisible();

    // True if winID belongs to a detached Area Map panel window. The area map
    // hides the OS cursor while placing a building/rocket; this lets PollEvents
    // re-show it when the pointer crosses into any other (non-area) window, since
    // SDL_ShowCursor is application-global rather than per-window.
    bool IsAreaPanelWindow(uint32_t winID) const;

    /**
     * Raise the SDL window to the foreground (above MFC windows)
     */
    void Raise();

    /**
     * Hide the SDL window (let MFC windows show through)
     */
    void Hide();

    /**
     * Show the SDL window
     */
    void Show();

    /**
     * Request application shutdown. The main loop (CConquerApp::Run) is driven by
     * SDL_PollEvent, but Win32 PostQuitMessage posts a NULL-hwnd WM_QUIT thread
     * message that SDL's internal pump silently discards (no SDL_QUIT is produced),
     * so the loop never sees the quit and spins forever. Push an SDL_QUIT directly —
     * PollEvents() handles it and returns true, giving a clean shutdown.
     */
    void RequestQuit();

    /**
     * Create an SDL_Window safely (with MFC CBT hook protection on Windows).
     * Use this instead of calling SDL_CreateWindow directly.
     */
    static SDL_Window* CreateSDLWindow(const char* title, int x, int y, int w, int h, Uint32 flags);

    /**
     * Register a non-modal SDL2Dialog for per-frame event routing and rendering.
     * The dialog must be heap-allocated. GameWindow deletes it when it closes.
     */
    void RegisterDialog(SDL2Dialog* dlg);

    /**
     * Unregister (but do NOT delete) a non-modal dialog.
     * Called by EndDialog — GameWindow's cleanup pass deletes the object.
     */
    void UnregisterDialog(SDL2Dialog* dlg);

private:

    bool InitializeSDL();
    void Cleanup();
    void HandleEvent(SDL_Event& event);

    // Translate a global keyboard shortcut (Ctrl+letter accelerators, F1 help,
    // Esc → game options) into the matching game command and dispatch it.
    // Returns true if the key was handled. Mirrors the original IDR_ACCEL
    // accelerator table, which is dead under SDL because SDL key events never
    // reach MFC's TranslateAccelerator.
    bool HandleGlobalShortcut(SDL_Event& event);

    void UpdateWindowTitle();  // Rebuild window title from components

    std::string m_title;
    int m_width;
    int m_height;

    // Game info tracked for title bar
    std::string m_playerName;   // "Player Name"
    std::string m_factionName;  // "Faction Name"
    std::string m_gameMode;     // "Single Player", "Multiplayer", etc.

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;  // T0: main-window GPU present (null in software mode)
    bool m_useRenderer = false;          // T0: [Advanced] Renderer flag, read once at init
    struct SDL_Surface* m_backBuffer = nullptr;  // T0: offscreen draw target (renderer mode)
    struct SDL_Texture* m_backTex = nullptr;     // T0: streaming texture for the back-buffer
    int m_backW = 0, m_backH = 0;                // current back-buffer size
    bool m_backBufferFresh = false;              // just (re)created → next present must be full upload
    // (Re)create the back-buffer surface + texture to match the renderer output
    // size. No-op if size unchanged. Renderer mode only.
    bool EnsureBackBuffer();
    bool m_pollingEvents = false;  // re-entrancy guard for PollEvents()
    bool m_appActive = true;       // app-level focus (any of our windows focused)
    Uint32 m_focusLostAt = 0;      // SDL_GetTicks() when focus first read NULL (0 = focused)

    // UI Components (Phase 9.2)
    std::unique_ptr<SDLButtonManager> m_buttonManager;
    std::unique_ptr<StatusBar> m_statusBar;
    std::shared_ptr<UIButtonListener> m_uiInputListener;

    // Input handling
    std::unique_ptr<InputHandler> m_inputHandler;

    // Active main menu (not owned - managed by CConquerApp)
    SDL2MainMenu* m_mainMenu = nullptr;

    // Compositor (Phase 1)
    std::unique_ptr<SDL2Compositor> m_compositor;

    // Active create-status dialog (Phase 2, not owned)
    class SDL2CreateStatus* m_createStatus = nullptr;

    // Native SDL2 toolbar (not owned)
    class SDL2Toolbar* m_sdl2Toolbar = nullptr;

    // Active non-modal dialogs (heap-allocated, owned by GameWindow)
    std::vector<SDL2Dialog*> m_activeDialogs;

    /**
     * Initialize UI components (button manager and status bar)
     * Called from Create() after window is set up
     */
    bool InitializeUI();

    /**
     * Configure game area button layout
     * Determines positions of all 17 buttons
     */
    void ConfigureGameAreaButtons();
};
