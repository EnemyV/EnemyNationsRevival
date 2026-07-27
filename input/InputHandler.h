#pragma once

#include <functional>
#include <vector>
#include <memory>

/**
 * Input handler
 * Routes SDL input events to appropriate game subsystems
 */
class InputHandler {
public:
    /**
     * Mouse button types
     */
    enum class MouseButton {
        LEFT = 1,
        MIDDLE = 2,
        RIGHT = 3,
    };

    /**
     * Input listener interface
     * Implementing classes receive input events
     */
    class InputListener {
    public:
        virtual ~InputListener() = default;

        /**
         * Handle mouse click event
         * @param x Screen X coordinate
         * @param y Screen Y coordinate
         * @param button Mouse button
         * @return True if event was handled, false to pass to next listener
         */
        virtual bool OnMouseClick(float x, float y, MouseButton button) { return false; }

        /**
         * Handle mouse release event
         * @param x Screen X coordinate
         * @param y Screen Y coordinate
         * @param button Mouse button
         * @return True if event was handled, false to pass to next listener
         */
        virtual bool OnMouseRelease(float x, float y, MouseButton button) { return false; }

        /**
         * Handle mouse move event
         * @param x Screen X coordinate
         * @param y Screen Y coordinate
         * @param deltaX Change in X since last move
         * @param deltaY Change in Y since last move
         * @return True if event was handled, false to pass to next listener
         */
        virtual bool OnMouseMove(float x, float y, float deltaX, float deltaY) { return false; }

        /**
         * Handle mouse wheel event
         * @param x Screen X coordinate
         * @param y Screen Y coordinate
         * @param deltaY Wheel delta (positive = up, negative = down)
         * @return True if event was handled, false to pass to next listener
         */
        virtual bool OnMouseWheel(float x, float y, int deltaY) { return false; }

        /**
         * Handle keyboard key press
         * @param keyCode SDL key code
         * @param modifiers Key modifiers (Ctrl, Shift, Alt flags)
         * @return True if event was handled, false to pass to next listener
         */
        virtual bool OnKeyPress(int keyCode, int modifiers) { return false; }

        /**
         * Handle keyboard key release
         * @param keyCode SDL key code
         * @param modifiers Key modifiers
         * @return True if event was handled, false to pass to next listener
         */
        virtual bool OnKeyRelease(int keyCode, int modifiers) { return false; }

        /**
         * Handle text input (for typing in text fields)
         * @param text Unicode text input
         * @return True if event was handled, false to pass to next listener
         */
        virtual bool OnTextInput(const std::string& text) { return false; }
    };

    InputHandler();
    ~InputHandler() = default;

    // === Event Listener Management ===

    /**
     * Register an input listener (listeners are checked in reverse order)
     * @param listener Shared pointer to listener
     */
    void RegisterListener(std::shared_ptr<InputListener> listener);

    /**
     * Unregister an input listener
     * @param listener Shared pointer to listener
     */
    void UnregisterListener(std::shared_ptr<InputListener> listener);

    /**
     * Clear all registered listeners
     */
    void ClearListeners();

    /**
     * Get number of registered listeners
     * @return Count of listeners
     */
    size_t GetListenerCount() const { return m_listeners.size(); }

    // === Mouse State ===

    /**
     * Get current mouse position
     * @param x Output mouse X
     * @param y Output mouse Y
     */
    void GetMousePosition(float& x, float& y) const {
        x = m_mouseX;
        y = m_mouseY;
    }

    /**
     * Check if a mouse button is currently pressed
     * @param button Mouse button
     * @return True if pressed, false otherwise
     */
    bool IsMouseButtonPressed(MouseButton button) const {
        return m_mouseButtonPressed[(int)button];
    }

    /**
     * Get mouse movement since last update
     * @param deltaX Output X movement
     * @param deltaY Output Y movement
     */
    void GetMouseDelta(float& deltaX, float& deltaY) const {
        deltaX = m_mouseDeltaX;
        deltaY = m_mouseDeltaY;
    }

    // === Keyboard State ===

    /**
     * Check if a key is currently pressed
     * @param keyCode SDL key code
     * @return True if pressed, false otherwise
     */
    bool IsKeyPressed(int keyCode) const;

    /**
     * Get current modifier state (Ctrl, Shift, Alt)
     * @return Modifier flags (KMOD_CTRL, KMOD_SHIFT, KMOD_ALT, etc.)
     */
    int GetKeyModifiers() const { return m_keyModifiers; }

    // === Event Processing ===

    /**
     * Process SDL event
     * Routes event to registered input listeners
     * @param event SDL event pointer
     */
    void ProcessEvent(void* event);  // void* is SDL_Event*

    /**
     * Clear frame-based input state (called once per frame)
     * Clears mouse delta and one-shot key states
     */
    void ClearFrameInput();

    /**
     * Update keyboard state from SDL (call once per frame)
     */
    void UpdateKeyboardState();

private:
    // Listeners (checked in reverse order for priority)
    std::vector<std::shared_ptr<InputListener>> m_listeners;

    // Mouse state
    float m_mouseX = 0.0f;
    float m_mouseY = 0.0f;
    float m_mouseDeltaX = 0.0f;
    float m_mouseDeltaY = 0.0f;
    bool m_mouseButtonPressed[4] = { false };  // Indexed by MouseButton enum

    // Keyboard state
    const uint8_t* m_keyboardState = nullptr;
    int m_numKeys = 0;
    int m_keyModifiers = 0;

    // === Helper Methods ===

    /**
     * Route mouse click event to listeners
     * @param x Screen X
     * @param y Screen Y
     * @param button Mouse button
     */
    void RouteMouseClick(float x, float y, MouseButton button);

    /**
     * Route mouse release event to listeners
     * @param x Screen X
     * @param y Screen Y
     * @param button Mouse button
     */
    void RouteMouseRelease(float x, float y, MouseButton button);

    /**
     * Route mouse move event to listeners
     * @param x Screen X
     * @param y Screen Y
     * @param deltaX X movement
     * @param deltaY Y movement
     */
    void RouteMouseMove(float x, float y, float deltaX, float deltaY);

    /**
     * Route mouse wheel event to listeners
     * @param x Screen X
     * @param y Screen Y
     * @param deltaY Wheel delta
     */
    void RouteMouseWheel(float x, float y, int deltaY);

    /**
     * Route key press event to listeners
     * @param keyCode SDL key code
     * @param modifiers Key modifiers
     */
    void RouteKeyPress(int keyCode, int modifiers);

    /**
     * Route key release event to listeners
     * @param keyCode SDL key code
     * @param modifiers Key modifiers
     */
    void RouteKeyRelease(int keyCode, int modifiers);

    /**
     * Route text input event to listeners
     * @param text Unicode text
     */
    void RouteTextInput(const std::string& text);
};
