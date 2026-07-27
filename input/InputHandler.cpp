#include "InputHandler.h"
#include <iostream>
#include <algorithm>

// SDL event types (copied to avoid SDL header dependency)
#define SDL_MOUSEMOTION 4
#define SDL_MOUSEBUTTONDOWN 5
#define SDL_MOUSEBUTTONUP 6
#define SDL_MOUSEWHEEL 7
#define SDL_KEYDOWN 2
#define SDL_KEYUP 3
#define SDL_TEXTINPUT 20
#define SDL_QUIT 12

// SDL key modifiers
#define KMOD_SHIFT 0x0003
#define KMOD_CTRL 0x00C0
#define KMOD_ALT 0x0300

InputHandler::InputHandler() {
    // Initialize mouse button pressed array
    m_mouseButtonPressed[0] = false;
    m_mouseButtonPressed[1] = false;
    m_mouseButtonPressed[2] = false;
    m_mouseButtonPressed[3] = false;
}

void InputHandler::RegisterListener(std::shared_ptr<InputListener> listener) {
    if (!listener) return;

    // Check if already registered
    auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
    if (it == m_listeners.end()) {
        m_listeners.push_back(listener);
        std::cout << "Input listener registered. Total: " << m_listeners.size() << std::endl;
    }
}

void InputHandler::UnregisterListener(std::shared_ptr<InputListener> listener) {
    if (!listener) return;

    auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
    if (it != m_listeners.end()) {
        m_listeners.erase(it);
        std::cout << "Input listener unregistered. Total: " << m_listeners.size() << std::endl;
    }
}

void InputHandler::ClearListeners() {
    m_listeners.clear();
}

bool InputHandler::IsKeyPressed(int keyCode) const {
    if (!m_keyboardState || keyCode < 0 || keyCode >= m_numKeys) {
        return false;
    }
    return m_keyboardState[keyCode] != 0;
}

void InputHandler::ProcessEvent(void* eventPtr) {
    if (!eventPtr) return;

    // Cast to SDL_Event structure manually (we don't include SDL.h to avoid dependency)
    // SDL_Event is a union, we'll extract the common fields
    const uint8_t* eventData = (const uint8_t*)eventPtr;

    // First 4 bytes are the event type
    uint32_t eventType = *(uint32_t*)eventData;

    switch (eventType) {
        case SDL_MOUSEMOTION: {
            // SDL_MouseMotionEvent: type, x, y, xrel, yrel, ...
            int x = *(int*)(eventData + 8);
            int y = *(int*)(eventData + 12);
            int xrel = *(int*)(eventData + 16);
            int yrel = *(int*)(eventData + 20);

            float deltaX = static_cast<float>(xrel);
            float deltaY = static_cast<float>(yrel);

            m_mouseX = static_cast<float>(x);
            m_mouseY = static_cast<float>(y);
            m_mouseDeltaX += deltaX;
            m_mouseDeltaY += deltaY;

            RouteMouseMove(m_mouseX, m_mouseY, deltaX, deltaY);
            break;
        }

        case SDL_MOUSEBUTTONDOWN: {
            // SDL_MouseButtonEvent: type, button, state, clicks, x, y
            uint8_t button = eventData[6];
            int x = *(int*)(eventData + 12);
            int y = *(int*)(eventData + 16);

            m_mouseX = static_cast<float>(x);
            m_mouseY = static_cast<float>(y);
            m_mouseButtonPressed[button] = true;

            RouteMouseClick(m_mouseX, m_mouseY, static_cast<MouseButton>(button));
            break;
        }

        case SDL_MOUSEBUTTONUP: {
            // SDL_MouseButtonEvent
            uint8_t button = eventData[6];
            int x = *(int*)(eventData + 12);
            int y = *(int*)(eventData + 16);

            m_mouseX = static_cast<float>(x);
            m_mouseY = static_cast<float>(y);
            m_mouseButtonPressed[button] = false;

            RouteMouseRelease(m_mouseX, m_mouseY, static_cast<MouseButton>(button));
            break;
        }

        case SDL_MOUSEWHEEL: {
            // SDL_MouseWheelEvent: type, x, y, preciseX, preciseY, direction, ...
            float preciseY = *(float*)(eventData + 12);
            int x = static_cast<int>(m_mouseX);
            int y = static_cast<int>(m_mouseY);

            int deltaY = static_cast<int>(preciseY);

            RouteMouseWheel(m_mouseX, m_mouseY, deltaY);
            break;
        }

        case SDL_KEYDOWN: {
            // SDL_KeyboardEvent: type, state, repeat, keysym
            // keysym structure: scancode, sym, mod, ...
            uint16_t sym = *(uint16_t*)(eventData + 12);
            uint16_t mod = *(uint16_t*)(eventData + 14);

            m_keyModifiers = mod;

            RouteKeyPress(sym, mod);
            break;
        }

        case SDL_KEYUP: {
            // SDL_KeyboardEvent
            uint16_t sym = *(uint16_t*)(eventData + 12);
            uint16_t mod = *(uint16_t*)(eventData + 14);

            m_keyModifiers = mod;

            RouteKeyRelease(sym, mod);
            break;
        }

        case SDL_TEXTINPUT: {
            // SDL_TextInputEvent: type, text[32]
            const char* text = (const char*)(eventData + 8);
            RouteTextInput(std::string(text));
            break;
        }

        case SDL_QUIT: {
            std::cout << "Quit event received" << std::endl;
            break;
        }
    }
}

void InputHandler::ClearFrameInput() {
    m_mouseDeltaX = 0.0f;
    m_mouseDeltaY = 0.0f;
}

void InputHandler::UpdateKeyboardState() {
    // This would be called from the main game loop using SDL_GetKeyboardState
    // For now, this is a placeholder
}

void InputHandler::RouteMouseClick(float x, float y, MouseButton button) {
    // Route to listeners in reverse order (last registered gets first chance)
    for (auto it = m_listeners.rbegin(); it != m_listeners.rend(); ++it) {
        if (*it && (*it)->OnMouseClick(x, y, button)) {
            return;  // Event handled
        }
    }
}

void InputHandler::RouteMouseRelease(float x, float y, MouseButton button) {
    for (auto it = m_listeners.rbegin(); it != m_listeners.rend(); ++it) {
        if (*it && (*it)->OnMouseRelease(x, y, button)) {
            return;
        }
    }
}

void InputHandler::RouteMouseMove(float x, float y, float deltaX, float deltaY) {
    for (auto it = m_listeners.rbegin(); it != m_listeners.rend(); ++it) {
        if (*it && (*it)->OnMouseMove(x, y, deltaX, deltaY)) {
            return;
        }
    }
}

void InputHandler::RouteMouseWheel(float x, float y, int deltaY) {
    for (auto it = m_listeners.rbegin(); it != m_listeners.rend(); ++it) {
        if (*it && (*it)->OnMouseWheel(x, y, deltaY)) {
            return;
        }
    }
}

void InputHandler::RouteKeyPress(int keyCode, int modifiers) {
    for (auto it = m_listeners.rbegin(); it != m_listeners.rend(); ++it) {
        if (*it && (*it)->OnKeyPress(keyCode, modifiers)) {
            return;
        }
    }
}

void InputHandler::RouteKeyRelease(int keyCode, int modifiers) {
    for (auto it = m_listeners.rbegin(); it != m_listeners.rend(); ++it) {
        if (*it && (*it)->OnKeyRelease(keyCode, modifiers)) {
            return;
        }
    }
}

void InputHandler::RouteTextInput(const std::string& text) {
    for (auto it = m_listeners.rbegin(); it != m_listeners.rend(); ++it) {
        if (*it && (*it)->OnTextInput(text)) {
            return;
        }
    }
}
