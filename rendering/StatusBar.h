#pragma once

#include <string>
#include <memory>

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Rect;

/**
 * Status bar for displaying game information
 * Shows resource counts, unit statistics, and other game status
 */
class StatusBar {
public:
    /**
     * Constructor
     */
    StatusBar();

    /**
     * Destructor
     */
    ~StatusBar();

    /**
     * Initialize status bar
     * @param renderer SDL renderer for text rendering
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param width Status bar width
     * @param height Status bar height
     * @param fontPath Path to TTF font file
     * @param fontSize Font size in pixels
     * @return true on success
     */
    bool Initialize(SDL_Renderer* renderer, int x, int y, int width, int height,
                    const std::string& fontPath, int fontSize);

    /**
     * Shutdown status bar and free resources
     */
    void Shutdown();

    /**
     * Set lumber resource count
     * @param amount Lumber amount
     */
    void SetLumber(int amount);

    /**
     * Set steel resource count
     * @param amount Steel amount
     */
    void SetSteel(int amount);

    /**
     * Set oil resource count
     * @param amount Oil amount
     */
    void SetOil(int amount);

    /**
     * Set power resource count
     * @param amount Power amount
     */
    void SetPower(int amount);

    /**
     * Set food resource count
     * @param amount Food amount
     */
    void SetFood(int amount);

    /**
     * Set unit count
     * @param count Number of units
     */
    void SetUnitCount(int count);

    /**
     * Set player health percentage
     * @param percentage Health percentage (0-100)
     */
    void SetHealth(int percentage);

    /**
     * Set game time display
     * @param hours Game hours
     * @param minutes Game minutes
     */
    void SetGameTime(int hours, int minutes);

    /**
     * Render status bar
     * @param renderer SDL renderer
     */
    void Render(SDL_Renderer* renderer);

    /**
     * Check if initialized
     * @return true if initialized and ready
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * Get screen rectangle
     * @return Status bar bounds
     */
    SDL_Rect GetScreenRect() const;

private:
    // Position and size
    int m_x;
    int m_y;
    int m_width;
    int m_height;

    // Current game state
    int m_lumber;
    int m_steel;
    int m_oil;
    int m_power;
    int m_food;
    int m_unitCount;
    int m_health;
    int m_gameHours;
    int m_gameMinutes;

    // Rendering
    SDL_Renderer* m_renderer;
    SDL_Texture* m_backgroundTexture;
    void* m_font;  // TTF_Font* (kept as void to avoid SDL_ttf dependency in header)
    bool m_initialized;

    /**
     * Rebuild status bar display text
     */
    void UpdateDisplay();

    /**
     * Render text to texture
     * @param text Text to render
     * @param x X position in status bar
     * @param y Y position in status bar
     */
    void RenderText(const std::string& text, int x, int y);
};
