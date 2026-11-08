#include "UIButtonListener.h"
#include "../rendering/SDLButtonManager.h"
#include <iostream>

UIButtonListener::UIButtonListener(SDLButtonManager* buttonManager)
    : m_buttonManager(buttonManager) {
    if (!m_buttonManager) {
        std::cerr << "UIButtonListener - Warning: Created with null button manager" << std::endl;
    }
}

bool UIButtonListener::OnMouseClick(float x, float y, InputHandler::MouseButton button) {
    if (!m_buttonManager) {
        return false;
    }

    // Only handle left mouse button
    if (button != InputHandler::MouseButton::LEFT) {
        return false;
    }

    // Check if click hit a button
    bool handled = m_buttonManager->OnMouseClick((int)x, (int)y, (int)button);

    if (handled && m_clickCallback) {
        // Note: SDLButtonManager will trigger its own callback
        // This is just for any additional processing
        // m_clickCallback will be called by button manager
    }

    return handled;
}

bool UIButtonListener::OnMouseRelease(float x, float y, InputHandler::MouseButton button) {
    if (!m_buttonManager) {
        return false;
    }

    // Only handle left mouse button
    if (button != InputHandler::MouseButton::LEFT) {
        return false;
    }

    m_buttonManager->OnMouseRelease((int)x, (int)y, (int)button);

    // Mouse release is informational, doesn't block other listeners
    return false;
}

bool UIButtonListener::OnMouseMove(float x, float y, float deltaX, float deltaY) {
    if (!m_buttonManager) {
        return false;
    }

    m_buttonManager->OnMouseMove((int)x, (int)y);

    // Mouse move is informational, doesn't block other listeners
    return false;
}

void UIButtonListener::SetClickCallback(std::function<void(int gameID)> callback) {
    m_clickCallback = callback;

    // Also set callback on button manager
    if (m_buttonManager) {
        m_buttonManager->SetClickCallback(callback);
    }
}
