#include "SDLButtonManager.h"
#include "SDLButton.h"
#include <SDL.h>
#include <iostream>

SDLButtonManager::SDLButtonManager()
    : m_spriteSheet(nullptr),
      m_renderer(nullptr),
      m_buttonWidth(40),
      m_buttonHeight(40),
      m_numStates(2),
      m_lastClickedButton(nullptr),
      m_initialized(false) {
}

SDLButtonManager::~SDLButtonManager() {
    Shutdown();
}

bool SDLButtonManager::Initialize(SDL_Renderer* renderer, const std::string& spriteSheetPath) {
    if (!renderer) {
        std::cerr << "SDLButtonManager::Initialize - No renderer provided" << std::endl;
        return false;
    }

    m_renderer = renderer;

    // Create placeholder sprite sheet (white surface with grid)
    m_spriteSheet = CreatePlaceholderSpriteSheet(renderer);
    if (!m_spriteSheet) {
        std::cerr << "SDLButtonManager::Initialize - Failed to create placeholder sprite sheet" << std::endl;
        return false;
    }

    std::cout << "SDLButtonManager initialized: " << m_buttonWidth << "x" << m_buttonHeight
              << " buttons, " << m_numStates << " states" << std::endl;

    m_initialized = true;
    return true;
}

void SDLButtonManager::Shutdown() {
    m_buttons.clear();

    if (m_spriteSheet) {
        SDL_DestroyTexture(m_spriteSheet);
        m_spriteSheet = nullptr;
    }

    m_initialized = false;
}

void SDLButtonManager::AddButton(int x, int y, int buttonIndex, int spriteIndex,
                                  const std::string& label, int gameID) {
    if (!m_initialized || !m_spriteSheet) {
        return;
    }

    auto button = std::make_unique<SDLButton>(buttonIndex, spriteIndex, label, gameID);
    if (button->Initialize(m_renderer, m_spriteSheet, m_buttonWidth, m_buttonHeight, m_numStates)) {
        button->SetPosition(x, y);
        m_buttons.push_back(std::move(button));
    }
}

SDLButton* SDLButtonManager::GetButtonAt(int screenX, int screenY) {
    // Check buttons in reverse order (last registered has highest priority)
    for (auto it = m_buttons.rbegin(); it != m_buttons.rend(); ++it) {
        if ((*it)->IsPointInside(screenX, screenY)) {
            return it->get();
        }
    }
    return nullptr;
}

SDLButton* SDLButtonManager::GetButtonByID(int gameID) {
    for (auto& button : m_buttons) {
        if (button->GetGameID() == gameID) {
            return button.get();
        }
    }
    return nullptr;
}

SDLButton* SDLButtonManager::GetButtonByIndex(int index) {
    for (auto& button : m_buttons) {
        if (button->GetButtonIndex() == index) {
            return button.get();
        }
    }
    return nullptr;
}

void SDLButtonManager::RenderAll(SDL_Renderer* renderer) {
    if (!m_spriteSheet) {
        return;
    }

    for (auto& button : m_buttons) {
        auto rect = button->GetScreenRect();
        button->Render(renderer, rect.x, rect.y);
    }
}

bool SDLButtonManager::OnMouseClick(int screenX, int screenY, int button) {
    SDLButton* clicked = GetButtonAt(screenX, screenY);
    if (clicked) {
        m_lastClickedButton = clicked;
        clicked->SetState(SDLButton::DOWN);

        // Trigger callback
        if (m_clickCallback) {
            m_clickCallback(clicked->GetGameID());
        }

        return true;
    }
    return false;
}

void SDLButtonManager::OnMouseRelease(int screenX, int screenY, int button) {
    if (m_lastClickedButton) {
        m_lastClickedButton->SetState(SDLButton::UP);
        m_lastClickedButton = nullptr;
    }
}

void SDLButtonManager::OnMouseMove(int screenX, int screenY) {
    // Update hover state for all buttons
    for (auto& button : m_buttons) {
        if (button->IsPointInside(screenX, screenY)) {
            // Only change to HOVER if not currently pressed
            if (button->GetState() != SDLButton::DOWN && m_numStates > 2) {
                button->SetState(SDLButton::HOVER);
            }
        } else {
            // Only change to UP if not currently pressed
            if (button->GetState() != SDLButton::DOWN) {
                button->SetState(SDLButton::UP);
            }
        }
    }
}

void SDLButtonManager::SetClickCallback(std::function<void(int gameID)> callback) {
    m_clickCallback = callback;
}

SDL_Texture* SDLButtonManager::CreatePlaceholderSpriteSheet(SDL_Renderer* renderer) {
    // Create a placeholder sprite sheet
    // 40x40 buttons, 3 states wide (UP, DOWN, HOVER), 17 buttons tall = 120x680
    const int buttonSize = 40;
    const int numStates = 3;
    const int numButtons = 17;

    int sheetWidth = buttonSize * numStates;
    int sheetHeight = buttonSize * numButtons;

    SDL_Surface* surface = SDL_CreateRGBSurface(0, sheetWidth, sheetHeight, 32,
                                                0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    if (!surface) {
        std::cerr << "SDLButtonManager - Failed to create surface" << std::endl;
        return nullptr;
    }

    // Fill with light gray background
    SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 200, 200, 200));

    // Draw buttons in grid - each row is one button, columns are states
    for (int buttonIdx = 0; buttonIdx < numButtons; ++buttonIdx) {
        // UP state (col 0) - light blue
        SDL_Rect upRect = { 0, buttonIdx * buttonSize, buttonSize, buttonSize };
        SDL_FillRect(surface, &upRect, SDL_MapRGB(surface->format, 100, 150, 200));

        // Draw border
        SDL_Rect upBorder = { 1, buttonIdx * buttonSize + 1, buttonSize - 2, buttonSize - 2 };
        SDL_FillRect(surface, &upBorder, SDL_MapRGB(surface->format, 120, 170, 220));

        // DOWN state (col 1) - darker blue
        SDL_Rect downRect = { buttonSize, buttonIdx * buttonSize, buttonSize, buttonSize };
        SDL_FillRect(surface, &downRect, SDL_MapRGB(surface->format, 70, 120, 170));

        SDL_Rect downBorder = { buttonSize + 1, buttonIdx * buttonSize + 1, buttonSize - 2, buttonSize - 2 };
        SDL_FillRect(surface, &downBorder, SDL_MapRGB(surface->format, 80, 130, 180));

        // HOVER state (col 2) - bright blue
        SDL_Rect hoverRect = { buttonSize * 2, buttonIdx * buttonSize, buttonSize, buttonSize };
        SDL_FillRect(surface, &hoverRect, SDL_MapRGB(surface->format, 150, 200, 255));

        SDL_Rect hoverBorder = { buttonSize * 2 + 1, buttonIdx * buttonSize + 1, buttonSize - 2, buttonSize - 2 };
        SDL_FillRect(surface, &hoverBorder, SDL_MapRGB(surface->format, 170, 220, 255));
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return texture;
}
