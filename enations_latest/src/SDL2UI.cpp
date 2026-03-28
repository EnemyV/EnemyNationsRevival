#include "stdafx.h"
#include "SDL2UI.h"
#include "GameWindow.h"
#include <SDL.h>
#include <SDL_ttf.h>

// Prevent Windows min/max macros from breaking std::min/std::max
#undef min
#undef max
#include <algorithm>

// Colors used by the dialog system - matching the game's style
namespace UIColors {
    const SDL_Color DialogBg    = { 60,  65,  62,  255 };  // Dark green-gray
    const SDL_Color DialogFrame = { 103, 127, 121, 255 };  // Light frame
    const SDL_Color DialogDark  = { 38,  46,  49,  255 };  // Dark frame
    const SDL_Color TitleText   = { 220, 210, 200, 255 };  // Light title
    const SDL_Color LabelText   = { 200, 190, 180, 255 };  // Normal text
    const SDL_Color BtnFace     = { 70,  86,  82,  255 };  // Button face
    const SDL_Color BtnLight    = { 103, 127, 121, 255 };  // Button highlight
    const SDL_Color BtnDark     = { 38,  46,  49,  255 };  // Button shadow
    const SDL_Color BtnText     = { 220, 210, 200, 255 };  // Button text
    const SDL_Color BtnPressed  = { 50,  60,  56,  255 };  // Pressed button
    const SDL_Color SliderTrack = { 40,  48,  45,  255 };  // Slider track
    const SDL_Color SliderThumb = { 140, 160, 150, 255 };  // Slider thumb
    const SDL_Color CheckMark   = { 200, 220, 210, 255 };  // Check/radio mark
    const SDL_Color Disabled    = { 100, 100, 100, 255 };  // Disabled text
}

// Helper: draw a filled rect
static void FillRect(SDL_Surface* dst, SDL_Rect r, SDL_Color c) {
    SDL_FillRect(dst, &r, SDL_MapRGB(dst->format, c.r, c.g, c.b));
}

// Helper: draw a bevel (raised or sunken)
static void DrawBevel(SDL_Surface* dst, SDL_Rect r, int width, SDL_Color light, SDL_Color dark) {
    for (int i = 0; i < width; i++) {
        // Top
        FillRect(dst, {r.x + i, r.y + i, r.w - 2*i, 1}, light);
        // Left
        FillRect(dst, {r.x + i, r.y + i, 1, r.h - 2*i}, light);
        // Bottom
        FillRect(dst, {r.x + i, r.y + r.h - 1 - i, r.w - 2*i, 1}, dark);
        // Right
        FillRect(dst, {r.x + r.w - 1 - i, r.y + i, 1, r.h - 2*i}, dark);
    }
}

// Helper: render text (single line, clipped to rect)
static void RenderText(SDL_Surface* dst, TTF_Font* font, const char* text,
                       SDL_Rect rect, SDL_Color color, bool centerH = true, bool centerV = true) {
    if (!font || !text || !text[0]) return;
    SDL_Surface* surf = TTF_RenderText_Blended(font, text, color);
    if (!surf) return;

    // Clip source rect to not exceed destination rect bounds
    SDL_Rect srcRect = { 0, 0, std::min(surf->w, rect.w), std::min(surf->h, rect.h) };
    SDL_Rect dstRect = rect;
    if (centerH && surf->w < rect.w) dstRect.x += (rect.w - surf->w) / 2;
    if (centerV && surf->h < rect.h) dstRect.y += (rect.h - surf->h) / 2;
    dstRect.w = srcRect.w;
    dstRect.h = srcRect.h;
    SDL_BlitSurface(surf, &srcRect, dst, &dstRect);
    SDL_FreeSurface(surf);
}

// Helper: render wrapped text (multi-line, clipped to rect)
static void RenderTextWrapped(SDL_Surface* dst, TTF_Font* font, const char* text,
                              SDL_Rect rect, SDL_Color color) {
    if (!font || !text || !text[0]) return;
    SDL_Surface* surf = TTF_RenderText_Blended_Wrapped(font, text, color, rect.w);
    if (!surf) return;

    // Clip to the available rect height
    SDL_Rect srcRect = { 0, 0, std::min(surf->w, rect.w), std::min(surf->h, rect.h) };
    SDL_Rect dstRect = rect;
    dstRect.w = srcRect.w;
    dstRect.h = srcRect.h;
    SDL_BlitSurface(surf, &srcRect, dst, &dstRect);
    SDL_FreeSurface(surf);
}

static bool PointInRect(int x, int y, SDL_Rect r) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

// ============================================================================
// SDL2Label
// ============================================================================
SDL2Label::SDL2Label(int x, int y, int w, int h, const std::string& text, SDL_Color color)
    : SDL2Widget(x, y, w, h), m_text(text), m_color(color) {}

void SDL2Label::Render(SDL_Surface* dst, TTF_Font* font) {
    if (!m_visible || m_text.empty()) return;
    if (m_wrapped) {
        RenderTextWrapped(dst, font, m_text.c_str(), m_rect,
                          m_enabled ? m_color : UIColors::Disabled);
    } else {
        RenderText(dst, font, m_text.c_str(), m_rect,
                   m_enabled ? m_color : UIColors::Disabled, m_centered, true);
    }
}

// ============================================================================
// SDL2Button
// ============================================================================
SDL2Button::SDL2Button(int x, int y, int w, int h, const std::string& text, ClickCallback onClick)
    : SDL2Widget(x, y, w, h), m_text(text), m_onClick(onClick) {}

void SDL2Button::Render(SDL_Surface* dst, TTF_Font* font) {
    if (!m_visible) return;

    SDL_Color face = m_pressed ? UIColors::BtnPressed : UIColors::BtnFace;
    FillRect(dst, m_rect, face);

    if (m_pressed)
        DrawBevel(dst, m_rect, 2, UIColors::BtnDark, UIColors::BtnLight);
    else
        DrawBevel(dst, m_rect, 2, UIColors::BtnLight, UIColors::BtnDark);

    SDL_Color textColor = m_enabled ? UIColors::BtnText : UIColors::Disabled;
    SDL_Rect textRect = { m_rect.x + 4, m_rect.y + 2, m_rect.w - 8, m_rect.h - 4 };
    RenderText(dst, font, m_text.c_str(), textRect, textColor);
}

bool SDL2Button::HandleEvent(const SDL_Event& event) {
    if (!m_visible || !m_enabled) return false;

    // Keyboard activation when focused
    if (m_focused && event.type == SDL_KEYDOWN &&
        (event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_RETURN)) {
        if (m_onClick) m_onClick();
        return true;
    }

    if (event.type == SDL_MOUSEMOTION) {
        m_hovered = PointInRect(event.motion.x, event.motion.y, m_rect);
        return false;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (PointInRect(event.button.x, event.button.y, m_rect)) {
            m_pressed = true;
            return true;
        }
    }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_pressed) {
            m_pressed = false;
            if (PointInRect(event.button.x, event.button.y, m_rect) && m_onClick)
                m_onClick();
            return true;
        }
    }
    return false;
}

// ============================================================================
// SDL2Slider
// ============================================================================
SDL2Slider::SDL2Slider(int x, int y, int w, int h, int minVal, int maxVal, int value,
                       ChangeCallback onChange)
    : SDL2Widget(x, y, w, h), m_minVal(minVal), m_maxVal(maxVal), m_value(value),
      m_onChange(onChange) {}

void SDL2Slider::SetValue(int v) {
    m_value = std::max(m_minVal, std::min(m_maxVal, v));
}

int SDL2Slider::ThumbX() const {
    if (m_maxVal <= m_minVal) return m_rect.x;
    int trackW = m_rect.w - 16;  // thumb width = 16
    return m_rect.x + (m_value - m_minVal) * trackW / (m_maxVal - m_minVal);
}

int SDL2Slider::ValueFromX(int x) const {
    int trackW = m_rect.w - 16;
    if (trackW <= 0) return m_minVal;
    int rel = x - m_rect.x;
    rel = std::max(0, std::min(trackW, rel));
    return m_minVal + rel * (m_maxVal - m_minVal) / trackW;
}

void SDL2Slider::Render(SDL_Surface* dst, TTF_Font* font) {
    if (!m_visible) return;

    // Track
    SDL_Rect trackRect = { m_rect.x, m_rect.y + m_rect.h/2 - 3, m_rect.w, 6 };
    FillRect(dst, trackRect, UIColors::SliderTrack);
    DrawBevel(dst, trackRect, 1, UIColors::BtnDark, UIColors::BtnLight);

    // Thumb
    int tx = ThumbX();
    SDL_Rect thumbRect = { tx, m_rect.y + 2, 16, m_rect.h - 4 };
    FillRect(dst, thumbRect, UIColors::SliderThumb);
    DrawBevel(dst, thumbRect, 2, UIColors::BtnLight, UIColors::BtnDark);

    // Value text
    std::string valStr = std::to_string(m_value);
    SDL_Rect valRect = { m_rect.x + m_rect.w + 8, m_rect.y, 40, m_rect.h };
    RenderText(dst, font, valStr.c_str(), valRect,
               m_enabled ? UIColors::LabelText : UIColors::Disabled, false, true);
}

bool SDL2Slider::HandleEvent(const SDL_Event& event) {
    if (!m_visible || !m_enabled) return false;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (PointInRect(event.button.x, event.button.y, m_rect)) {
            m_dragging = true;
            int newVal = ValueFromX(event.button.x);
            if (newVal != m_value) {
                m_value = newVal;
                if (m_onChange) m_onChange(m_value);
            }
            return true;
        }
    }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_dragging) {
            m_dragging = false;
            return true;
        }
    }
    if (event.type == SDL_MOUSEMOTION && m_dragging) {
        int newVal = ValueFromX(event.motion.x);
        newVal = std::max(m_minVal, std::min(m_maxVal, newVal));
        if (newVal != m_value) {
            m_value = newVal;
            if (m_onChange) m_onChange(m_value);
        }
        return true;
    }
    return false;
}

// ============================================================================
// SDL2Checkbox
// ============================================================================
SDL2Checkbox::SDL2Checkbox(int x, int y, int w, int h, const std::string& text,
                           bool checked, ChangeCallback onChange)
    : SDL2Widget(x, y, w, h), m_text(text), m_checked(checked), m_onChange(onChange) {}

void SDL2Checkbox::Render(SDL_Surface* dst, TTF_Font* font) {
    if (!m_visible) return;

    // Checkbox square
    int boxSize = std::min(m_rect.h - 4, 18);
    SDL_Rect boxRect = { m_rect.x + 2, m_rect.y + (m_rect.h - boxSize)/2, boxSize, boxSize };
    FillRect(dst, boxRect, UIColors::SliderTrack);
    DrawBevel(dst, boxRect, 1, UIColors::BtnDark, UIColors::BtnLight);

    // Check mark
    if (m_checked) {
        SDL_Rect checkRect = { boxRect.x + 3, boxRect.y + 3, boxSize - 6, boxSize - 6 };
        FillRect(dst, checkRect, UIColors::CheckMark);
    }

    // Label text
    SDL_Rect textRect = { m_rect.x + boxSize + 8, m_rect.y, m_rect.w - boxSize - 10, m_rect.h };
    RenderText(dst, font, m_text.c_str(), textRect,
               m_enabled ? UIColors::LabelText : UIColors::Disabled, false, true);
}

bool SDL2Checkbox::HandleEvent(const SDL_Event& event) {
    if (!m_visible || !m_enabled) return false;

    if (m_focused && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
        m_checked = !m_checked;
        if (m_onChange) m_onChange(m_checked);
        return true;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (PointInRect(event.button.x, event.button.y, m_rect)) {
            m_checked = !m_checked;
            if (m_onChange) m_onChange(m_checked);
            return true;
        }
    }
    return false;
}

// ============================================================================
// SDL2RadioGroup
// ============================================================================
SDL2RadioGroup::SDL2RadioGroup(int x, int y, int w, int h,
                               const std::vector<std::string>& options,
                               int selected, ChangeCallback onChange)
    : SDL2Widget(x, y, w, h), m_options(options), m_selected(selected), m_onChange(onChange) {
    m_optEnabled.resize(options.size(), true);
}

void SDL2RadioGroup::SetEnabled(int index, bool enabled) {
    if (index >= 0 && index < (int)m_optEnabled.size())
        m_optEnabled[index] = enabled;
}

void SDL2RadioGroup::Render(SDL_Surface* dst, TTF_Font* font) {
    if (!m_visible || m_options.empty()) return;

    int itemH = m_rect.h / (int)m_options.size();
    int radioSize = std::min(itemH - 4, 16);

    for (int i = 0; i < (int)m_options.size(); i++) {
        int iy = m_rect.y + i * itemH;
        bool enabled = m_enabled && m_optEnabled[i];

        // Radio circle (approximated as rect)
        SDL_Rect radioRect = { m_rect.x + 2, iy + (itemH - radioSize)/2, radioSize, radioSize };
        FillRect(dst, radioRect, UIColors::SliderTrack);
        DrawBevel(dst, radioRect, 1, UIColors::BtnDark, UIColors::BtnLight);

        // Selected indicator
        if (i == m_selected) {
            SDL_Rect dotRect = { radioRect.x + 3, radioRect.y + 3, radioSize - 6, radioSize - 6 };
            FillRect(dst, dotRect, UIColors::CheckMark);
        }

        // Label
        SDL_Rect textRect = { m_rect.x + radioSize + 8, iy, m_rect.w - radioSize - 10, itemH };
        RenderText(dst, font, m_options[i].c_str(), textRect,
                   enabled ? UIColors::LabelText : UIColors::Disabled, false, true);
    }
}

bool SDL2RadioGroup::HandleEvent(const SDL_Event& event) {
    if (!m_visible || !m_enabled) return false;

    // Keyboard navigation when focused
    if (m_focused && event.type == SDL_KEYDOWN) {
        int count = (int)m_options.size();
        switch (event.key.keysym.sym) {
            case SDLK_UP: {
                int next = m_selected - 1;
                while (next >= 0 && !m_optEnabled[next]) next--;
                if (next >= 0) { m_selected = next; if (m_onChange) m_onChange(m_selected); }
                return true;
            }
            case SDLK_DOWN: {
                int next = m_selected + 1;
                while (next < count && !m_optEnabled[next]) next++;
                if (next < count) { m_selected = next; if (m_onChange) m_onChange(m_selected); }
                return true;
            }
        }
    }

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (PointInRect(event.button.x, event.button.y, m_rect)) {
            int itemH = m_rect.h / (int)m_options.size();
            int index = (event.button.y - m_rect.y) / itemH;
            if (index >= 0 && index < (int)m_options.size() && m_optEnabled[index]) {
                if (index != m_selected) {
                    m_selected = index;
                    if (m_onChange) m_onChange(m_selected);
                }
                return true;
            }
        }
    }
    return false;
}

// ============================================================================
// SDL2Listbox
// ============================================================================
SDL2Listbox::SDL2Listbox(int x, int y, int w, int h, SelectCallback onSelect, DblClickCallback onDblClick)
    : SDL2Widget(x, y, w, h), m_onSelect(onSelect), m_onDblClick(onDblClick) {}

void SDL2Listbox::AddItem(const std::string& text, void* data) {
    m_items.push_back({text, data});
}

void SDL2Listbox::Clear() {
    m_items.clear();
    m_selected = -1;
    m_scrollOffset = 0;
}

void* SDL2Listbox::GetItemData(int index) const {
    if (index < 0 || index >= (int)m_items.size()) return nullptr;
    return m_items[index].data;
}

const std::string& SDL2Listbox::GetItemText(int index) const {
    static std::string empty;
    if (index < 0 || index >= (int)m_items.size()) return empty;
    return m_items[index].text;
}

void SDL2Listbox::Render(SDL_Surface* dst, TTF_Font* font) {
    if (!m_visible) return;

    // Background
    FillRect(dst, m_rect, UIColors::SliderTrack);
    DrawBevel(dst, m_rect, 1, UIColors::BtnDark, UIColors::BtnLight);

    int visibleItems = m_rect.h / m_itemHeight;
    for (int i = 0; i < visibleItems && (i + m_scrollOffset) < (int)m_items.size(); i++) {
        int idx = i + m_scrollOffset;
        int iy = m_rect.y + i * m_itemHeight;

        // Highlight selected
        if (idx == m_selected) {
            SDL_Rect hlRect = { m_rect.x + 1, iy, m_rect.w - 2, m_itemHeight };
            FillRect(dst, hlRect, UIColors::BtnFace);
        }

        // Item text
        SDL_Rect textRect = { m_rect.x + 6, iy, m_rect.w - 12, m_itemHeight };
        RenderText(dst, font, m_items[idx].text.c_str(), textRect,
                   m_enabled ? UIColors::LabelText : UIColors::Disabled, false, true);
    }
}

bool SDL2Listbox::HandleEvent(const SDL_Event& event) {
    if (!m_visible || !m_enabled) return false;

    // Keyboard navigation when focused
    if (m_focused && event.type == SDL_KEYDOWN) {
        int count = (int)m_items.size();
        if (count == 0) return false;
        switch (event.key.keysym.sym) {
            case SDLK_UP:
                if (m_selected > 0) {
                    m_selected--;
                    // Scroll to keep selection visible
                    if (m_selected < m_scrollOffset) m_scrollOffset = m_selected;
                    if (m_onSelect) m_onSelect(m_selected);
                }
                return true;
            case SDLK_DOWN:
                if (m_selected < count - 1) {
                    m_selected++;
                    int visibleItems = m_rect.h / m_itemHeight;
                    if (m_selected >= m_scrollOffset + visibleItems)
                        m_scrollOffset = m_selected - visibleItems + 1;
                    if (m_onSelect) m_onSelect(m_selected);
                }
                return true;
            case SDLK_HOME:
                m_selected = 0; m_scrollOffset = 0;
                if (m_onSelect) m_onSelect(m_selected);
                return true;
            case SDLK_END:
                m_selected = count - 1;
                { int vis = m_rect.h / m_itemHeight;
                  m_scrollOffset = std::max(0, count - vis); }
                if (m_onSelect) m_onSelect(m_selected);
                return true;
            case SDLK_RETURN:
                if (m_selected >= 0 && m_onDblClick) m_onDblClick(m_selected);
                return true;
        }
    }

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (PointInRect(event.button.x, event.button.y, m_rect)) {
            int idx = (event.button.y - m_rect.y) / m_itemHeight + m_scrollOffset;
            if (idx >= 0 && idx < (int)m_items.size()) {
                // Double-click detection
                Uint32 now = SDL_GetTicks();
                if (idx == m_lastClickIndex && (now - m_lastClickTime) < 400) {
                    if (m_onDblClick) m_onDblClick(idx);
                    m_lastClickTime = 0;
                    return true;
                }
                m_lastClickIndex = idx;
                m_lastClickTime = now;

                m_selected = idx;
                if (m_onSelect) m_onSelect(idx);
            }
            return true;
        }
    }

    // Mouse wheel scrolling
    if (event.type == SDL_MOUSEWHEEL) {
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        if (PointInRect(mx, my, m_rect)) {
            m_scrollOffset -= event.wheel.y * 2;
            int maxScroll = std::max(0, (int)m_items.size() - m_rect.h / m_itemHeight);
            m_scrollOffset = std::max(0, std::min(maxScroll, m_scrollOffset));
            return true;
        }
    }

    return false;
}

// ============================================================================
// SDL2EditBox
// ============================================================================
SDL2EditBox::SDL2EditBox(int x, int y, int w, int h, const std::string& text, ChangeCallback onChange)
    : SDL2Widget(x, y, w, h), m_text(text), m_onChange(onChange), m_cursorPos((int)text.size()) {}

void SDL2EditBox::Render(SDL_Surface* dst, TTF_Font* font) {
    if (!m_visible) return;

    // Background
    FillRect(dst, m_rect, { 30, 35, 32, 255 });
    DrawBevel(dst, m_rect, 1, UIColors::BtnDark, UIColors::BtnLight);

    // Text
    SDL_Rect textRect = { m_rect.x + 4, m_rect.y + 2, m_rect.w - 8, m_rect.h - 4 };
    if (!m_text.empty()) {
        RenderText(dst, font, m_text.c_str(), textRect,
                   m_enabled ? UIColors::LabelText : UIColors::Disabled, false, true);
    }

    // Cursor blink
    if (m_focused && ((SDL_GetTicks() / 500) % 2 == 0)) {
        int cursorX = m_rect.x + 4;
        if (font && !m_text.empty()) {
            int tw, th;
            std::string beforeCursor = m_text.substr(0, m_cursorPos);
            TTF_SizeText(font, beforeCursor.c_str(), &tw, &th);
            cursorX += tw;
        }
        SDL_Rect cursorRect = { cursorX, m_rect.y + 3, 1, m_rect.h - 6 };
        FillRect(dst, cursorRect, UIColors::LabelText);
    }
}

bool SDL2EditBox::HandleEvent(const SDL_Event& event) {
    if (!m_visible || !m_enabled) return false;

    // Manage SDL text input based on focus state
    static SDL2EditBox* s_activeEdit = nullptr;
    if (m_focused && s_activeEdit != this) {
        SDL_StartTextInput();
        s_activeEdit = this;
        m_cursorPos = (int)m_text.size();
    } else if (!m_focused && s_activeEdit == this) {
        SDL_StopTextInput();
        s_activeEdit = nullptr;
    }

    // Click to focus
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (PointInRect(event.button.x, event.button.y, m_rect)) {
            m_cursorPos = (int)m_text.size();
            return true;
        }
    }

    if (!m_focused) return false;

    // Text input
    if (event.type == SDL_TEXTINPUT) {
        m_text.insert(m_cursorPos, event.text.text);
        m_cursorPos += (int)strlen(event.text.text);
        if (m_onChange) m_onChange(m_text);
        return true;
    }

    // Key handling
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_BACKSPACE:
                if (m_cursorPos > 0) {
                    m_text.erase(m_cursorPos - 1, 1);
                    m_cursorPos--;
                    if (m_onChange) m_onChange(m_text);
                }
                return true;
            case SDLK_DELETE:
                if (m_cursorPos < (int)m_text.size()) {
                    m_text.erase(m_cursorPos, 1);
                    if (m_onChange) m_onChange(m_text);
                }
                return true;
            case SDLK_LEFT:
                if (m_cursorPos > 0) m_cursorPos--;
                return true;
            case SDLK_RIGHT:
                if (m_cursorPos < (int)m_text.size()) m_cursorPos++;
                return true;
            case SDLK_HOME:
                m_cursorPos = 0;
                return true;
            case SDLK_END:
                m_cursorPos = (int)m_text.size();
                return true;
        }
    }

    return false;
}

// ============================================================================
// SDL2Image
// ============================================================================
SDL2Image::SDL2Image(int x, int y, int w, int h)
    : SDL2Widget(x, y, w, h) {}

SDL2Image::~SDL2Image() {
    if (m_owned && m_surface) SDL_FreeSurface(m_surface);
}

void SDL2Image::SetSurface(SDL_Surface* surf, bool takeOwnership) {
    if (m_owned && m_surface) SDL_FreeSurface(m_surface);
    m_surface = surf;
    m_owned = takeOwnership;
}

void SDL2Image::Clear() {
    if (m_owned && m_surface) SDL_FreeSurface(m_surface);
    m_surface = nullptr;
    m_owned = false;
}

void SDL2Image::Render(SDL_Surface* dst, TTF_Font* font) {
    if (!m_visible || !m_surface) return;

    // Scale the image to fit the widget rect, preserving aspect ratio
    int imgW = m_surface->w;
    int imgH = m_surface->h;
    float scaleX = (float)m_rect.w / imgW;
    float scaleY = (float)m_rect.h / imgH;
    float scale = std::min(scaleX, scaleY);

    int dstW = (int)(imgW * scale);
    int dstH = (int)(imgH * scale);
    int dstX = m_rect.x + (m_rect.w - dstW) / 2;
    int dstY = m_rect.y + (m_rect.h - dstH) / 2;

    SDL_Rect dstRect = { dstX, dstY, dstW, dstH };
    SDL_BlitScaled(m_surface, nullptr, dst, &dstRect);
}

// ============================================================================
// SDL2Dialog
// ============================================================================
SDL2Dialog::SDL2Dialog(GameWindow* gameWindow, const std::string& title, int w, int h)
    : m_gameWindow(gameWindow), m_title(title), m_width(w), m_height(h) {

    // Center on screen
    int winW = gameWindow->GetWidth();
    int winH = gameWindow->GetHeight();
    m_x = (winW - w) / 2;
    m_y = (winH - h) / 2;

    // Find font
    static const char* candidates[] = {
        "C:\\Windows\\Fonts\\BKANT.TTF",
        "C:\\Windows\\Fonts\\BOOKOS.TTF",
        "C:\\Windows\\Fonts\\times.ttf",
        "C:\\Windows\\Fonts\\georgia.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        nullptr
    };
    for (int i = 0; candidates[i]; i++) {
        FILE* f = fopen(candidates[i], "rb");
        if (f) { fclose(f); m_fontPath = candidates[i]; break; }
    }
}

SDL2Dialog::~SDL2Dialog() {
    if (m_background) SDL_FreeSurface(m_background);
    for (auto& pair : m_fontCache)
        if (pair.second) TTF_CloseFont(pair.second);
}

TTF_Font* SDL2Dialog::GetFont(int size) {
    if (m_fontPath.empty()) return nullptr;
    auto it = m_fontCache.find(size);
    if (it != m_fontCache.end()) return it->second;
    TTF_Font* font = TTF_OpenFont(m_fontPath.c_str(), size);
    m_fontCache[size] = font;
    return font;
}

void SDL2Dialog::AddOKCancelButtons() {
    int btnW = 90, btnH = 30;
    int btnY = m_y + m_height - btnH - 15;
    int centerX = m_x + m_width / 2;

    AddWidget<SDL2Button>(centerX - btnW - 10, btnY, btnW, btnH, "OK",
                          [this]() { OnOK(); });
    AddWidget<SDL2Button>(centerX + 10, btnY, btnW, btnH, "Cancel",
                          [this]() { OnCancel(); });
}

void SDL2Dialog::Render() {
    SDL_Surface* winSurface = SDL_GetWindowSurface(m_gameWindow->GetWindow());
    if (!winSurface) return;

    // Restore the captured background (main menu or whatever was behind)
    if (m_background) {
        SDL_BlitSurface(m_background, nullptr, winSurface, nullptr);
    }

    // Dialog background
    SDL_Rect dlgRect = { m_x, m_y, m_width, m_height };
    FillRect(winSurface, dlgRect, UIColors::DialogBg);
    DrawBevel(winSurface, dlgRect, 3, UIColors::DialogFrame, UIColors::DialogDark);

    // Title bar
    SDL_Rect titleRect = { m_x + 3, m_y + 3, m_width - 6, 28 };
    FillRect(winSurface, titleRect, {50, 55, 52, 255});
    TTF_Font* titleFont = GetFont(16);
    if (titleFont) {
        SDL_Rect titleTextRect = { m_x + 10, m_y + 5, m_width - 20, 24 };
        RenderText(winSurface, titleFont, m_title.c_str(), titleTextRect,
                   UIColors::TitleText, false, true);
    }

    // Render all widgets
    TTF_Font* widgetFont = GetFont(14);
    for (auto& widget : m_widgets) {
        widget->Render(winSurface, widgetFont);
        // Draw focus indicator
        if (widget->HasFocus()) {
            SDL_Rect r = widget->GetRect();
            r.x -= 2; r.y -= 2; r.w += 4; r.h += 4;
            DrawBevel(winSurface, r, 1, {180, 200, 220, 255}, {180, 200, 220, 255});
        }
    }

    SDL_UpdateWindowSurface(m_gameWindow->GetWindow());
}

void SDL2Dialog::FocusNext() {
    int start = m_focusIndex;
    int count = (int)m_widgets.size();
    if (count == 0) return;

    // Clear old focus
    if (m_focusIndex >= 0 && m_focusIndex < count)
        m_widgets[m_focusIndex]->SetFocus(false);

    // Find next focusable widget
    for (int i = 1; i <= count; i++) {
        int idx = (start + i) % count;
        if (m_widgets[idx]->IsFocusable()) {
            m_focusIndex = idx;
            m_widgets[idx]->SetFocus(true);
            return;
        }
    }
    m_focusIndex = -1;
}

void SDL2Dialog::FocusPrev() {
    int start = m_focusIndex;
    int count = (int)m_widgets.size();
    if (count == 0) return;

    if (m_focusIndex >= 0 && m_focusIndex < count)
        m_widgets[m_focusIndex]->SetFocus(false);

    for (int i = 1; i <= count; i++) {
        int idx = (start - i + count) % count;
        if (m_widgets[idx]->IsFocusable()) {
            m_focusIndex = idx;
            m_widgets[idx]->SetFocus(true);
            return;
        }
    }
    m_focusIndex = -1;
}

bool SDL2Dialog::HandleEvent(SDL_Event& event) {
    // ESC = cancel
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        OnCancel();
        return true;
    }
    // ENTER = OK (unless an editbox has focus — let it handle enter)
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
        // If focused widget is a button, activate it
        if (m_focusIndex >= 0 && m_focusIndex < (int)m_widgets.size()) {
            SDL2Button* btn = dynamic_cast<SDL2Button*>(m_widgets[m_focusIndex].get());
            if (btn && btn->IsEnabled()) {
                btn->HandleEvent(event);  // let button handle it
                return true;
            }
        }
        OnOK();
        return true;
    }
    // TAB = cycle focus (Shift+TAB = reverse)
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_TAB) {
        if (event.key.keysym.mod & KMOD_SHIFT)
            FocusPrev();
        else
            FocusNext();
        return true;
    }

    // Route to focused widget first for keyboard events
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP || event.type == SDL_TEXTINPUT) {
        if (m_focusIndex >= 0 && m_focusIndex < (int)m_widgets.size()) {
            if (m_widgets[m_focusIndex]->HandleEvent(event))
                return true;
        }
    }

    // Route to all widgets for mouse events (reverse order for z-order)
    for (int i = (int)m_widgets.size() - 1; i >= 0; i--) {
        if (m_widgets[i]->HandleEvent(event)) {
            // If a widget was clicked, give it focus
            if (event.type == SDL_MOUSEBUTTONDOWN && m_widgets[i]->IsFocusable()) {
                if (m_focusIndex >= 0 && m_focusIndex < (int)m_widgets.size())
                    m_widgets[m_focusIndex]->SetFocus(false);
                m_focusIndex = i;
                m_widgets[i]->SetFocus(true);
            }
            return true;
        }
    }

    // Consume all mouse events within dialog rect
    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP ||
        event.type == SDL_MOUSEMOTION) {
        return true;
    }

    return false;
}

void SDL2Dialog::CaptureBackground() {
    SDL_Surface* winSurface = SDL_GetWindowSurface(m_gameWindow->GetWindow());
    if (!winSurface) return;

    if (m_background) SDL_FreeSurface(m_background);
    m_background = SDL_ConvertSurface(winSurface, winSurface->format, 0);
}

int SDL2Dialog::DoModal() {
    m_running = true;
    m_result = 0;

    // Capture current screen content as dialog background
    CaptureBackground();

    OnInit();

    SDL_Event event;
    while (m_running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                m_running = false;
                m_result = 0;
                ::PostQuitMessage(0);
                break;
            }
            HandleEvent(event);
        }

        Render();

        // Don't burn CPU
        SDL_Delay(16);

        // Also pump Windows messages so MFC doesn't freeze
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                m_running = false;
                m_result = 0;
                break;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    return m_result;
}

void SDL2Dialog::EndDialog(int result) {
    m_result = result;
    m_running = false;
}
