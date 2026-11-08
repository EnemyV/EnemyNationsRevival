#pragma once

#include "InputHandler.h"
#include <memory>
#include <functional>

class SDLButtonManager;

/**
 * Input listener for UI button events
 * Routes mouse input to button manager with highest priority
 * Consumes events if handled by buttons, passes through otherwise
 */
class UIButtonListener : public InputHandler::InputListener {
public:
    /**
     * Constructor
     * @param buttonManager Pointer to button manager (not owned)
     */
    explicit UIButtonListener(SDLButtonManager* buttonManager);

    /**
     * Destructor
     */
    ~UIButtonListener() override = default;

    /**
     * Handle mouse click
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param button Mouse button
     * @return true if click was handled by a button
     */
    bool OnMouseClick(float x, float y, InputHandler::MouseButton button) override;

    /**
     * Handle mouse release
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param button Mouse button
     * @return true if release was handled
     */
    bool OnMouseRelease(float x, float y, InputHandler::MouseButton button) override;

    /**
     * Handle mouse move
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param deltaX Change in X
     * @param deltaY Change in Y
     * @return true if handled
     */
    bool OnMouseMove(float x, float y, float deltaX, float deltaY) override;

    /**
     * Set callback for button clicks
     * @param callback Function to call when button clicked
     */
    void SetClickCallback(std::function<void(int gameID)> callback);

private:
    SDLButtonManager* m_buttonManager;  // Not owned
    std::function<void(int)> m_clickCallback;
};
