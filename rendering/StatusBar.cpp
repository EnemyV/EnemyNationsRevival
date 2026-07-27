#include "StatusBar.h"
#include <SDL.h>
#include <iostream>

StatusBar::StatusBar()
    : m_x(0), m_y(0), m_width(0), m_height(0),
      m_lumber(0), m_steel(0), m_oil(0), m_power(0), m_food(0),
      m_unitCount(0), m_health(100),
      m_gameHours(0), m_gameMinutes(0),
      m_renderer(nullptr),
      m_backgroundTexture(nullptr),
      m_font(nullptr),
      m_initialized(false) {
}

StatusBar::~StatusBar() {
    Shutdown();
}

bool StatusBar::Initialize(SDL_Renderer* renderer, int x, int y, int width, int height,
                           const std::string& fontPath, int fontSize) {
    if (!renderer || width <= 0 || height <= 0) {
        return false;
    }

    m_renderer = renderer;
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;

    // Create background texture (dark background with border)
    SDL_Surface* bgSurface = SDL_CreateRGBSurface(0, width, height, 32,
                                                   0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    if (bgSurface) {
        // Fill with dark background
        SDL_FillRect(bgSurface, nullptr, SDL_MapRGB(bgSurface->format, 32, 32, 48));

        // Draw border
        SDL_Rect border = { 0, 0, width, height };
        SDL_FillRect(bgSurface, &border, SDL_MapRGB(bgSurface->format, 64, 64, 80));

        SDL_Rect inner = { 1, 1, width - 2, height - 2 };
        SDL_FillRect(bgSurface, &inner, SDL_MapRGB(bgSurface->format, 32, 32, 48));

        m_backgroundTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
        SDL_FreeSurface(bgSurface);
    }

    std::cout << "StatusBar initialized at " << x << "," << y << " size " << width << "x" << height << std::endl;
    m_initialized = true;
    return true;
}

void StatusBar::Shutdown() {
    if (m_backgroundTexture) {
        SDL_DestroyTexture(m_backgroundTexture);
        m_backgroundTexture = nullptr;
    }

    m_initialized = false;
}

void StatusBar::SetLumber(int amount) {
    m_lumber = amount;
}

void StatusBar::SetSteel(int amount) {
    m_steel = amount;
}

void StatusBar::SetOil(int amount) {
    m_oil = amount;
}

void StatusBar::SetPower(int amount) {
    m_power = amount;
}

void StatusBar::SetFood(int amount) {
    m_food = amount;
}

void StatusBar::SetUnitCount(int count) {
    m_unitCount = count;
}

void StatusBar::SetHealth(int percentage) {
    m_health = (percentage < 0) ? 0 : (percentage > 100) ? 100 : percentage;
}

void StatusBar::SetGameTime(int hours, int minutes) {
    m_gameHours = hours;
    m_gameMinutes = minutes;
}

void StatusBar::UpdateDisplay() {
    // Placeholder for future text rendering
}

void StatusBar::RenderText(const std::string& text, int x, int y) {
    // Text rendering requires SDL_ttf - placeholder for now
}

void StatusBar::Render(SDL_Renderer* renderer) {
    if (!m_initialized || !renderer) {
        return;
    }

    // Render background
    if (m_backgroundTexture) {
        SDL_Rect destRect = { m_x, m_y, m_width, m_height };
        SDL_RenderCopy(renderer, m_backgroundTexture, nullptr, &destRect);
    }

    // Draw visual indicators for resources (colored rectangles)
    int sectionWidth = m_width / 7;  // 7 sections: lumber, steel, oil, power, food, units, health

    // Lumber (red)
    SDL_Rect lumberRect = { m_x + 5, m_y + 10, 15, 20 };
    SDL_SetRenderDrawColor(renderer, 200, 100, 50, 255);
    SDL_RenderFillRect(renderer, &lumberRect);

    // Steel (gray)
    SDL_Rect steelRect = { m_x + 5 + sectionWidth, m_y + 10, 15, 20 };
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderFillRect(renderer, &steelRect);

    // Oil (black)
    SDL_Rect oilRect = { m_x + 5 + sectionWidth * 2, m_y + 10, 15, 20 };
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &oilRect);

    // Power (yellow)
    SDL_Rect powerRect = { m_x + 5 + sectionWidth * 3, m_y + 10, 15, 20 };
    SDL_SetRenderDrawColor(renderer, 200, 200, 100, 255);
    SDL_RenderFillRect(renderer, &powerRect);

    // Food (green)
    SDL_Rect foodRect = { m_x + 5 + sectionWidth * 4, m_y + 10, 15, 20 };
    SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
    SDL_RenderFillRect(renderer, &foodRect);

    // Units (blue)
    SDL_Rect unitsRect = { m_x + 5 + sectionWidth * 5, m_y + 10, 15, 20 };
    SDL_SetRenderDrawColor(renderer, 100, 150, 200, 255);
    SDL_RenderFillRect(renderer, &unitsRect);

    // Health (red gradient based on percentage)
    int healthColorIntensity = (m_health * 255) / 100;
    SDL_Rect healthRect = { m_x + 5 + sectionWidth * 6, m_y + 10, 15, 20 };
    SDL_SetRenderDrawColor(renderer, healthColorIntensity, 255 - healthColorIntensity, 50, 255);
    SDL_RenderFillRect(renderer, &healthRect);
}

SDL_Rect StatusBar::GetScreenRect() const {
    return SDL_Rect{ m_x, m_y, m_width, m_height };
}
