#pragma once

#include <string>
#include <memory>

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Rect;

/**
 * Individual button rendered with SDL2
 * Handles rendering, state management, and hit-testing for a single button
 */
class SDLButton {
public:
    /**
     * Button visual states
     */
    enum ButtonState {
        UP = 0,      // Normal/unpressed state
        DOWN = 1,    // Pressed state
        HOVER = 2    // Mouse hover state (if available in sprite sheet)
    };

    /**
     * Constructor
     * @param buttonIndex Index of button in original array (0-16)
     * @param spriteIndex Index in sprite sheet
     * @param label Display label for button
     * @param gameID Windows message ID or game command ID
     */
    SDLButton(int buttonIndex, int spriteIndex, const std::string& label, int gameID);

    /**
     * Destructor
     */
    ~SDLButton() = default;

    /**
     * Initialize button with sprite sheet texture
     * @param renderer SDL renderer for rendering
     * @param spriteSheet Texture containing all button sprites
     * @param buttonWidth Width of each button sprite in pixels
     * @param buttonHeight Height of each button sprite in pixels
     * @param numStates Number of button states in sprite sheet (2 or 3)
     * @return true on success
     */
    bool Initialize(SDL_Renderer* renderer, SDL_Texture* spriteSheet,
                    int buttonWidth, int buttonHeight, int numStates);

    /**
     * Render button at screen position
     * @param renderer SDL renderer
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     */
    void Render(SDL_Renderer* renderer, int screenX, int screenY);

    /**
     * Set button visual state
     * @param state New state (up/down/hover)
     */
    void SetState(ButtonState state);

    /**
     * Get current button state
     * @return Current state
     */
    ButtonState GetState() const { return m_currentState; }

    /**
     * Check if point is within button bounds
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @return true if point is inside button
     */
    bool IsPointInside(int screenX, int screenY) const;

    /**
     * Get button width in pixels
     * @return Width
     */
    int GetWidth() const { return m_width; }

    /**
     * Get button height in pixels
     * @return Height
     */
    int GetHeight() const { return m_height; }

    /**
     * Get button label
     * @return Display label
     */
    const std::string& GetLabel() const { return m_label; }

    /**
     * Get game command ID for this button
     * @return Game ID
     */
    int GetGameID() const { return m_gameID; }

    /**
     * Get button index (0-16)
     * @return Button index
     */
    int GetButtonIndex() const { return m_buttonIndex; }

    /**
     * Set screen position
     * @param x Screen X
     * @param y Screen Y
     */
    void SetPosition(int x, int y);

    /**
     * Get screen rectangle
     * @return Button bounds on screen
     */
    SDL_Rect GetScreenRect() const;

private:
    // Button identity
    int m_buttonIndex;          // Index in original 17-button array
    int m_spriteIndex;          // Index in sprite sheet
    std::string m_label;        // Display text
    int m_gameID;               // Game command ID

    // Rendering
    SDL_Texture* m_spriteSheet; // Pointer to sprite sheet (not owned)
    SDL_Renderer* m_renderer;   // Pointer to renderer (not owned)
    int m_width;                // Button width in pixels
    int m_height;               // Button height in pixels
    int m_numStates;            // Number of states in sprite sheet (2 or 3)

    // State
    ButtonState m_currentState; // Current visual state
    int m_screenX;              // Screen X position
    int m_screenY;              // Screen Y position

    // Source rectangles for each state in sprite sheet
    SDL_Rect m_sourceRects[3];  // Up, Down, Hover source rects
};
