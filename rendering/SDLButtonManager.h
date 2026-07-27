#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>

class SDLButton;

struct SDL_Renderer;
struct SDL_Texture;

/**
 * Manages all game area buttons
 * Loads button sprite sheet, creates 17 button instances, handles input routing
 */
class SDLButtonManager {
public:
    /**
     * Constructor
     */
    SDLButtonManager();

    /**
     * Destructor - cleans up textures and buttons
     */
    ~SDLButtonManager();

    /**
     * Initialize button manager with sprite sheet
     * @param renderer SDL renderer for button rendering
     * @param spriteSheetPath Path to buttons.tga or alternative
     * @return true on success
     */
    bool Initialize(SDL_Renderer* renderer, const std::string& spriteSheetPath);

    /**
     * Shutdown button manager
     */
    void Shutdown();

    /**
     * Add a button at screen position
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param buttonIndex Original button index (0-16)
     * @param spriteIndex Index in sprite sheet
     * @param label Display label
     * @param gameID Game command ID
     */
    void AddButton(int x, int y, int buttonIndex, int spriteIndex,
                   const std::string& label, int gameID);

    /**
     * Get button at screen position (hit testing)
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @return Pointer to button if hit, nullptr otherwise
     */
    SDLButton* GetButtonAt(int screenX, int screenY);

    /**
     * Get button by game ID
     * @param gameID Game command ID
     * @return Pointer to button if found, nullptr otherwise
     */
    SDLButton* GetButtonByID(int gameID);

    /**
     * Get button by index
     * @param index Button index (0-16)
     * @return Pointer to button if found, nullptr otherwise
     */
    SDLButton* GetButtonByIndex(int index);

    /**
     * Render all buttons
     * @param renderer SDL renderer
     */
    void RenderAll(SDL_Renderer* renderer);

    /**
     * Handle mouse click event
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @param button Mouse button (1=left, 2=middle, 3=right)
     * @return true if click was handled by a button
     */
    bool OnMouseClick(int screenX, int screenY, int button);

    /**
     * Handle mouse release event
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @param button Mouse button
     */
    void OnMouseRelease(int screenX, int screenY, int button);

    /**
     * Handle mouse move event
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     */
    void OnMouseMove(int screenX, int screenY);

    /**
     * Set callback for button clicks
     * @param callback Function called with game ID when button clicked
     */
    void SetClickCallback(std::function<void(int gameID)> callback);

    /**
     * Get number of buttons
     * @return Button count
     */
    size_t GetButtonCount() const { return m_buttons.size(); }

    /**
     * Check if initialized
     * @return true if initialized and ready
     */
    bool IsInitialized() const { return m_initialized; }

private:
    std::vector<std::unique_ptr<SDLButton>> m_buttons;
    SDL_Texture* m_spriteSheet;
    SDL_Renderer* m_renderer;
    int m_buttonWidth;
    int m_buttonHeight;
    int m_numStates;
    SDLButton* m_lastClickedButton;  // Track currently pressed button
    bool m_initialized;
    std::function<void(int)> m_clickCallback;

    /**
     * Create placeholder sprite sheet
     * @param renderer SDL renderer
     * @return Placeholder texture for button rendering
     */
    SDL_Texture* CreatePlaceholderSpriteSheet(SDL_Renderer* renderer);
};
