#include "SDLButton.h"
#include <SDL.h>
#include <cstring>

SDLButton::SDLButton(int buttonIndex, int spriteIndex, const std::string& label, int gameID)
    : m_buttonIndex(buttonIndex),
      m_spriteIndex(spriteIndex),
      m_label(label),
      m_gameID(gameID),
      m_spriteSheet(nullptr),
      m_renderer(nullptr),
      m_width(0),
      m_height(0),
      m_numStates(2),
      m_currentState(UP),
      m_screenX(0),
      m_screenY(0) {
    // Clear source rects
    std::memset(m_sourceRects, 0, sizeof(m_sourceRects));
}

bool SDLButton::Initialize(SDL_Renderer* renderer, SDL_Texture* spriteSheet,
                            int buttonWidth, int buttonHeight, int numStates) {
    if (!renderer || !spriteSheet || buttonWidth <= 0 || buttonHeight <= 0) {
        return false;
    }

    m_renderer = renderer;
    m_spriteSheet = spriteSheet;
    m_width = buttonWidth;
    m_height = buttonHeight;
    m_numStates = (numStates > 3) ? 3 : numStates;  // Max 3 states

    // Calculate source rectangles for each state
    // Layout: Each button takes one row, states are in columns
    // Button Y = m_spriteIndex * height
    // State X = state * width
    for (int i = 0; i < m_numStates; ++i) {
        m_sourceRects[i].x = i * m_width;
        m_sourceRects[i].y = m_spriteIndex * m_height;
        m_sourceRects[i].w = m_width;
        m_sourceRects[i].h = m_height;
    }

    return true;
}

void SDLButton::Render(SDL_Renderer* renderer, int screenX, int screenY) {
    if (!m_spriteSheet || !renderer) {
        return;
    }

    // Update position if different
    if (screenX != m_screenX || screenY != m_screenY) {
        m_screenX = screenX;
        m_screenY = screenY;
    }

    // Create destination rectangle
    SDL_Rect destRect = {
        screenX, screenY,
        m_width, m_height
    };

    // Get source rect for current state
    int stateIndex = static_cast<int>(m_currentState);
    if (stateIndex >= m_numStates) {
        stateIndex = m_numStates - 1;  // Fallback to last state
    }

    const SDL_Rect& srcRect = m_sourceRects[stateIndex];

    // Render sprite from sheet to screen
    SDL_RenderCopy(renderer, m_spriteSheet, &srcRect, &destRect);
}

void SDLButton::SetState(ButtonState state) {
    m_currentState = state;
}

bool SDLButton::IsPointInside(int screenX, int screenY) const {
    return (screenX >= m_screenX && screenX < m_screenX + m_width &&
            screenY >= m_screenY && screenY < m_screenY + m_height);
}

void SDLButton::SetPosition(int x, int y) {
    m_screenX = x;
    m_screenY = y;
}

SDL_Rect SDLButton::GetScreenRect() const {
    SDL_Rect rect = { m_screenX, m_screenY, m_width, m_height };
    return rect;
}
