#include "stdafx.h"
#include "SDL2UI.h"
#include "GameWindow.h"
#include "SDL2MainMenu.h"
#include "bitmaps.h"
#include <SDL.h>
#include <SDL_ttf.h>
#ifdef _WIN32
#include <SDL_syswm.h>
#endif

// Prevent Windows min/max macros from breaking std::min/std::max
#undef min
#undef max
#include <algorithm>

// Static game art surfaces shared by all dialogs
SDL_Surface* SDL2Dialog::s_dlgBkgnd   = nullptr;
SDL_Surface* SDL2Dialog::s_dlgGold    = nullptr;
SDL_Surface* SDL2Dialog::s_borderHorz = nullptr;
SDL_Surface* SDL2Dialog::s_borderVert = nullptr;
bool SDL2Dialog::s_artLoaded = false;

// Colors used by the dialog system - matching the original Enemy Nations style
namespace UIColors {
    const SDL_Color DialogBg    = { 60,  65,  62,  255 };  // Dark green-gray (fallback)
    const SDL_Color DialogFrame = { 103, 127, 121, 255 };  // Light frame (fallback)
    const SDL_Color DialogDark  = { 38,  46,  49,  255 };  // Dark frame (fallback)
    const SDL_Color TitleBg     = { 42,  22,  65,  255 };  // Dark purple title bar
    const SDL_Color TitleText   = { 255, 255, 255, 255 };  // White title text
    const SDL_Color LabelText   = {  48,  58, 148, 255 };  // Blue label text (matching original)
    const SDL_Color BtnFace     = { 68,  55,  135, 255 };  // Blueish-purple button
    const SDL_Color BtnLight    = { 115,  98, 195, 255 };  // Lighter purple highlight
    const SDL_Color BtnDark     = { 32,  22,  72,  255 };  // Darker purple shadow
    const SDL_Color BtnText     = { 225, 182,  55, 255 };  // Gold button text
    const SDL_Color BtnPressed  = { 48,  38,  98,  255 };  // Pressed button (darker purple)
    const SDL_Color SliderTrack = { 40,  48,  45,  255 };  // Slider track
    const SDL_Color SliderThumb = { 140, 160, 150, 255 };  // Slider thumb
    const SDL_Color CheckMark   = { 225, 182,  55,  255 };  // Gold check/radio mark
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

// Helper: symmetric horizontal gradient — dark at left/right edges, lighter at center
static void FillGradientSymH(SDL_Surface* dst, SDL_Rect r, SDL_Color edge, SDL_Color center) {
    if (r.w <= 0) return;
    float halfW = r.w / 2.0f;
    for (int i = 0; i < r.w; i++) {
        float t = 1.0f - std::abs(i - halfW + 0.5f) / halfW;
        t = t * t;  // quadratic for a softer center peak
        Uint8 rc = (Uint8)(edge.r + (center.r - edge.r) * t);
        Uint8 gc = (Uint8)(edge.g + (center.g - edge.g) * t);
        Uint8 bc = (Uint8)(edge.b + (center.b - edge.b) * t);
        SDL_Rect col = { r.x + i, r.y, 1, r.h };
        SDL_FillRect(dst, &col, SDL_MapRGB(dst->format, rc, gc, bc));
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

// Helper: render wrapped text (multi-line, clipped to rect, vertically centered)
static void RenderTextWrapped(SDL_Surface* dst, TTF_Font* font, const char* text,
                              SDL_Rect rect, SDL_Color color, bool centerH = false,
                              bool topAligned = false, bool rightAligned = false) {
    if (!font || !text || !text[0]) return;

    // Use TTF_SetFontWrappedAlign for per-line alignment (SDL_ttf 2.20+).
    // rightAligned wins over centerH if both are set.
    int oldAlign = TTF_GetFontWrappedAlign(font);
    if (rightAligned)
        TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_RIGHT);
    else if (centerH)
        TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_CENTER);

    SDL_Surface* surf = TTF_RenderText_Blended_Wrapped(font, text, color, rect.w);

    // Restore original alignment
    TTF_SetFontWrappedAlign(font, oldAlign);

    if (!surf) return;

    // Clip to the available rect height
    SDL_Rect srcRect = { 0, 0, std::min(surf->w, rect.w), std::min(surf->h, rect.h) };
    SDL_Rect dstRect = rect;
    // Vertically center the text block within the rect (unless top-aligned)
    if (!topAligned && surf->h < rect.h)
        dstRect.y += (rect.h - surf->h) / 2;
    // For right-aligned single short lines, the wrapped surface still spans rect.w
    // so blitting at dstRect.x is correct — the right-align lives inside surf.
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
                          m_enabled ? m_color : UIColors::Disabled,
                          m_centered, m_topAligned, m_rightAligned);
    } else if (m_rightAligned) {
        // Single-line right-align: render at natural width, then offset to right edge.
        SDL_Surface* surf = TTF_RenderText_Blended(font, m_text.c_str(),
            m_enabled ? m_color : UIColors::Disabled);
        if (surf) {
            SDL_Rect srcRect = { 0, 0, std::min(surf->w, m_rect.w), std::min(surf->h, m_rect.h) };
            SDL_Rect dstRect = m_rect;
            if (surf->w < m_rect.w) dstRect.x += (m_rect.w - surf->w);
            if (!m_topAligned && surf->h < m_rect.h) dstRect.y += (m_rect.h - surf->h) / 2;
            dstRect.w = srcRect.w;
            dstRect.h = srcRect.h;
            SDL_BlitSurface(surf, &srcRect, dst, &dstRect);
            SDL_FreeSurface(surf);
        }
    } else {
        RenderText(dst, font, m_text.c_str(), m_rect,
                   m_enabled ? m_color : UIColors::Disabled, m_centered,
                   !m_topAligned);
    }
}

// ============================================================================
// SDL2Button
// ============================================================================
SDL2Button::SDL2Button(int x, int y, int w, int h, const std::string& text, ClickCallback onClick)
    : SDL2Widget(x, y, w, h), m_text(text), m_onClick(onClick) {}

void SDL2Button::Render(SDL_Surface* dst, TTF_Font* font) {
    if (!m_visible) return;

    // Use 3-state sprite sheet if available (matching CUnitButton::DrawItem)
    // m_toggled shows the pressed frame when selected (like original m_cState & 0x01)
    bool showPressed = m_pressed || m_toggled;
    if (m_btnSheet) {
        int frameW = m_btnSheet->w / 3;
        int frameH = m_btnSheet->h;
        int frameIdx = 0;  // normal
        if (!m_enabled)
            frameIdx = 2;  // disabled
        else if (showPressed)
            frameIdx = 1;  // pressed
        SDL_Rect src = { frameIdx * frameW, 0, frameW, frameH };
        SDL_Rect dstR = m_rect;
        SDL_BlitScaled(m_btnSheet, &src, dst, &dstR);
    } else {
        // Gradient: symmetric dark edges → lighter center (matching original game style)
        if (showPressed) {
            FillGradientSymH(dst, m_rect, UIColors::BtnDark,    UIColors::BtnPressed);
            DrawBevel(dst, m_rect, 2, UIColors::BtnDark, UIColors::BtnLight);
        } else {
            FillGradientSymH(dst, m_rect, UIColors::BtnDark,    UIColors::BtnLight);
            DrawBevel(dst, m_rect, 1, UIColors::BtnLight, UIColors::BtnDark);
        }
    }

    SDL_Color textColor = m_enabled ? UIColors::BtnText : UIColors::Disabled;
    // Inset rect: large buttons use -6 (matching CUnitButton), small buttons use -2
    int inset = (m_rect.h >= 32) ? 6 : 2;
    SDL_Rect faceRect = { m_rect.x + inset, m_rect.y + inset, m_rect.w - 2*inset, m_rect.h - 2*inset };

    // Draw icon if set — fills the ENTIRE face rect (matching original CUnitButton).
    // In the original, the icon takes the full button area and text is drawn
    // ON TOP of the icon at the bottom.
    bool hasIcon = (m_iconSheet && m_iconSrc.w > 0 && m_iconSrc.h > 0);
    if (hasIcon) {
        int shift = showPressed ? 2 : 0;
        int iconH = m_iconSrc.h;
        int iconW = m_iconSrc.w;

        // Match the original CUnitButton::DrawItem exactly: the icon spans the
        // full face HEIGHT (stretched, not aspect-preserved) and keeps its native
        // WIDTH, centered — or fills the face width if the icon is wider than the
        // face. The previous aspect-preserving fit left a square icon shrunk to a
        // small block adrift in a wide button; this fills it the way the game did.
        int dstW = (faceRect.w >= iconW) ? iconW : faceRect.w;
        int dstH = faceRect.h;
        int dstY = faceRect.y;
        // If the face is taller than the icon, don't upscale past native height —
        // center vertically (mirrors MFC's rDest.Height() > rSrc.Height() branch).
        if (faceRect.h > iconH) {
            dstH = iconH;
            dstY = faceRect.y + (faceRect.h - iconH) / 2;
        }

        SDL_Rect dr = {
            faceRect.x + (faceRect.w - dstW) / 2 + shift,
            dstY + shift,
            dstW, dstH
        };
        SDL_BlitScaled(m_iconSheet, &m_iconSrc, dst, &dr);
    }

    // Text rect: overlays the bottom of the face (on top of icon) or full face if no icon
    SDL_Rect textRect = faceRect;
    if (hasIcon) {
        int textStripH = 14;
        textRect.y = faceRect.y + faceRect.h - textStripH;
        textRect.h = textStripH;
    }

    // Original uses black shadow then white text, both DT_CENTER | DT_WORDBREAK.
    // We draw white centered text (shadow omitted for clarity on dark backgrounds).
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color actualColor = m_btnSheet ? white : textColor;

    if (m_rect.h >= 32 && font) {
        int textW = 0, textH = 0;
        TTF_SizeText(font, m_text.c_str(), &textW, &textH);
        if (textW > textRect.w)
            RenderTextWrapped(dst, font, m_text.c_str(), textRect, actualColor, true);
        else
            RenderText(dst, font, m_text.c_str(), textRect, actualColor, true, true);
    } else {
        RenderText(dst, font, m_text.c_str(), textRect, actualColor, true, true);
    }
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
            if (PointInRect(event.button.x, event.button.y, m_rect)) {
                Uint32 now = SDL_GetTicks();
                // Double-click: two clicks within 500ms fire m_onDblClick instead of m_onClick
                if (m_onDblClick && (now - m_lastClickTime) < 500) {
                    m_lastClickTime = 0;
                    m_onDblClick();
                } else {
                    m_lastClickTime = now;
                    if (m_onClick)
                        m_onClick();
                }
            }
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

        // Radio circle: white background, blue border
        static const SDL_Color kRadioBg     = { 255, 255, 255, 255 };
        static const SDL_Color kRadioBorder = {  48,  58, 148, 255 };
        SDL_Rect radioRect = { m_rect.x + 2, iy + (itemH - radioSize)/2, radioSize, radioSize };
        FillRect(dst, radioRect, kRadioBg);
        DrawBevel(dst, radioRect, 1, kRadioBorder, kRadioBorder);

        // Selected indicator: blue dot
        if (i == m_selected) {
            SDL_Rect dotRect = { radioRect.x + 3, radioRect.y + 3, radioSize - 6, radioSize - 6 };
            FillRect(dst, dotRect, kRadioBorder);
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

    static const SDL_Color kListBg  = { 255, 255, 255, 255 };  // white background
    static const SDL_Color kSelRow  = {  48,  58, 148, 255 };  // blue selection row
    static const SDL_Color kSelText = { 225, 182,  55, 255 };  // gold text when selected

    // Background
    FillRect(dst, m_rect, kListBg);
    DrawBevel(dst, m_rect, 1, UIColors::BtnDark, UIColors::BtnLight);

    int visibleItems = m_rect.h / m_itemHeight;
    for (int i = 0; i < visibleItems && (i + m_scrollOffset) < (int)m_items.size(); i++) {
        int idx = i + m_scrollOffset;
        int iy = m_rect.y + i * m_itemHeight;
        bool selected = (idx == m_selected);

        // Highlight selected row
        if (selected) {
            SDL_Rect hlRect = { m_rect.x + 1, iy, m_rect.w - 2, m_itemHeight };
            FillRect(dst, hlRect, kSelRow);
        }

        // Item text — gold on selected, blue otherwise
        SDL_Color textColor = !m_enabled ? UIColors::Disabled
                              : selected ? kSelText
                                         : UIColors::LabelText;
        SDL_Rect textRect = { m_rect.x + 6, iy, m_rect.w - 12, m_itemHeight };
        RenderText(dst, font, m_items[idx].text.c_str(), textRect, textColor, false, true);
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

    // Mouse wheel scrolling. Use the wheel event's own mouse position (shifted to
    // main-window coords by the dialog) rather than SDL_GetMouseState — the latter
    // returns dialog-window-local coords for dialogs in their own window, which
    // never match m_rect, so wheel scrolling silently did nothing there.
    if (event.type == SDL_MOUSEWHEEL) {
        int mx = event.wheel.mouseX, my = event.wheel.mouseY;
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
    : SDL2Widget(x, y, w, h), m_text(text), m_onChange(onChange),
      m_cursorPos((int)text.size()), m_selStart((int)text.size()), m_selEnd((int)text.size()) {}

int SDL2EditBox::CharIndexToX(int index) const {
    int x = m_rect.x + 4;
    if (m_cachedFont && !m_text.empty() && index > 0) {
        int tw, th;
        std::string sub = m_text.substr(0, index);
        TTF_SizeText(m_cachedFont, sub.c_str(), &tw, &th);
        x += tw;
    }
    return x;
}

int SDL2EditBox::XToCharIndex(int pixelX) const {
    if (!m_cachedFont || m_text.empty()) return 0;
    int textX = m_rect.x + 4;
    // Binary-ish search: measure progressively to find best fit
    for (int i = 1; i <= (int)m_text.size(); i++) {
        int tw, th;
        std::string sub = m_text.substr(0, i);
        TTF_SizeText(m_cachedFont, sub.c_str(), &tw, &th);
        int charRight = textX + tw;
        // If click is before the midpoint of this character, place cursor before it
        int prevTw = 0;
        if (i > 1) {
            int dummy;
            std::string prevSub = m_text.substr(0, i - 1);
            TTF_SizeText(m_cachedFont, prevSub.c_str(), &prevTw, &dummy);
        }
        int charMid = textX + prevTw + (tw - prevTw) / 2;
        if (pixelX < charMid) return i - 1;
    }
    return (int)m_text.size();
}

void SDL2EditBox::DeleteSelection() {
    if (!HasSelection()) return;
    int lo = SelMin(), hi = SelMax();
    m_text.erase(lo, hi - lo);
    m_cursorPos = lo;
    ClearSelection();
    if (m_onChange) m_onChange(m_text);
}

void SDL2EditBox::Render(SDL_Surface* dst, TTF_Font* font) {
    if (!m_visible) return;
    m_cachedFont = font;  // Cache for hit-testing in HandleEvent

    // Background (white by default; dark dialogs can override via SetColors)
    SDL_Color bg   = m_customColors ? m_bgColor   : SDL_Color{ 255, 255, 255, 255 };
    SDL_Color text = m_customColors ? m_textColor : UIColors::LabelText;
    FillRect(dst, m_rect, bg);
    DrawBevel(dst, m_rect, 1, UIColors::BtnDark, UIColors::BtnLight);

    // Selection highlight
    if (m_focused && HasSelection() && font) {
        int selMinX = CharIndexToX(SelMin());
        int selMaxX = CharIndexToX(SelMax());
        SDL_Rect selRect = { selMinX, m_rect.y + 2, selMaxX - selMinX, m_rect.h - 4 };
        FillRect(dst, selRect, { 80, 120, 160, 255 });
    }

    // Text
    SDL_Rect textRect = { m_rect.x + 4, m_rect.y + 2, m_rect.w - 8, m_rect.h - 4 };
    if (!m_text.empty()) {
        RenderText(dst, font, m_text.c_str(), textRect,
                   m_enabled ? text : UIColors::Disabled, false, true);
    }

    // Cursor blink (don't blink when there's an active selection). On a dark
    // field draw a light caret, otherwise a dark one.
    if (m_focused && !HasSelection() && ((SDL_GetTicks() / 500) % 2 == 0)) {
        int cursorX = CharIndexToX(m_cursorPos);
        SDL_Rect cursorRect = { cursorX, m_rect.y + 3, 1, m_rect.h - 6 };
        bool darkBg = (bg.r + bg.g + bg.b) < 256;
        FillRect(dst, cursorRect, darkBg ? SDL_Color{ 230, 230, 230, 255 }
                                         : SDL_Color{ 30, 30, 30, 255 });
    }
}

bool SDL2EditBox::HandleEvent(const SDL_Event& event) {
    if (!m_visible || !m_enabled) return false;

    // Manage SDL text input based on focus state
    static SDL2EditBox* s_activeEdit = nullptr;
    if (m_focused && s_activeEdit != this) {
        SDL_StartTextInput();
        s_activeEdit = this;
        // Select all on initial focus so typing replaces existing text
        m_selStart = 0;
        m_selEnd = m_cursorPos = (int)m_text.size();
    } else if (!m_focused && s_activeEdit == this) {
        SDL_StopTextInput();
        s_activeEdit = nullptr;
    }

    // Mouse down — click to position cursor, start drag
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (PointInRect(event.button.x, event.button.y, m_rect)) {
            m_cursorPos = XToCharIndex(event.button.x);
            m_selStart = m_selEnd = m_cursorPos;
            m_dragging = true;
            return true;
        }
    }

    // Mouse motion — extend selection while dragging
    if (event.type == SDL_MOUSEMOTION && m_dragging) {
        m_cursorPos = XToCharIndex(event.motion.x);
        m_selEnd = m_cursorPos;
        return true;
    }

    // Mouse up — end drag
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_dragging) {
            m_dragging = false;
            m_cursorPos = XToCharIndex(event.button.x);
            m_selEnd = m_cursorPos;
            return true;
        }
    }

    if (!m_focused) return false;

    // Text input — replace selection if any
    if (event.type == SDL_TEXTINPUT) {
        if (HasSelection()) DeleteSelection();
        m_text.insert(m_cursorPos, event.text.text);
        m_cursorPos += (int)strlen(event.text.text);
        ClearSelection();
        if (m_onChange) m_onChange(m_text);
        return true;
    }

    // Key handling
    if (event.type == SDL_KEYDOWN) {
        bool shift = (event.key.keysym.mod & KMOD_SHIFT) != 0;
        bool ctrl = (event.key.keysym.mod & KMOD_CTRL) != 0;

        switch (event.key.keysym.sym) {
            case SDLK_a:
                if (ctrl) {
                    // Ctrl+A: select all
                    m_selStart = 0;
                    m_selEnd = m_cursorPos = (int)m_text.size();
                    return true;
                }
                break;
            case SDLK_c:
                if (ctrl && HasSelection()) {
                    SDL_SetClipboardText(m_text.substr(SelMin(), SelMax() - SelMin()).c_str());
                    return true;
                }
                break;
            case SDLK_x:
                if (ctrl && HasSelection()) {
                    SDL_SetClipboardText(m_text.substr(SelMin(), SelMax() - SelMin()).c_str());
                    DeleteSelection();
                    return true;
                }
                break;
            case SDLK_v:
                if (ctrl) {
                    char* clip = SDL_GetClipboardText();
                    if (clip && clip[0]) {
                        if (HasSelection()) DeleteSelection();
                        m_text.insert(m_cursorPos, clip);
                        m_cursorPos += (int)strlen(clip);
                        ClearSelection();
                        if (m_onChange) m_onChange(m_text);
                    }
                    SDL_free(clip);
                    return true;
                }
                break;
            case SDLK_BACKSPACE:
                if (HasSelection()) {
                    DeleteSelection();
                } else if (m_cursorPos > 0) {
                    m_text.erase(m_cursorPos - 1, 1);
                    m_cursorPos--;
                    ClearSelection();
                    if (m_onChange) m_onChange(m_text);
                }
                return true;
            case SDLK_DELETE:
                if (HasSelection()) {
                    DeleteSelection();
                } else if (m_cursorPos < (int)m_text.size()) {
                    m_text.erase(m_cursorPos, 1);
                    ClearSelection();
                    if (m_onChange) m_onChange(m_text);
                }
                return true;
            case SDLK_LEFT:
                if (m_cursorPos > 0) m_cursorPos--;
                if (shift) m_selEnd = m_cursorPos;
                else ClearSelection();
                return true;
            case SDLK_RIGHT:
                if (m_cursorPos < (int)m_text.size()) m_cursorPos++;
                if (shift) m_selEnd = m_cursorPos;
                else ClearSelection();
                return true;
            case SDLK_HOME:
                m_cursorPos = 0;
                if (shift) m_selEnd = m_cursorPos;
                else ClearSelection();
                return true;
            case SDLK_END:
                m_cursorPos = (int)m_text.size();
                if (shift) m_selEnd = m_cursorPos;
                else ClearSelection();
                return true;
        }
    }

    return false;
}

// ============================================================================
// SDL2GroupBox
// ============================================================================
SDL2GroupBox::SDL2GroupBox(int x, int y, int w, int h, const std::string& label)
    : SDL2Widget(x, y, w, h), m_label(label) {}

void SDL2GroupBox::Render(SDL_Surface* dst, TTF_Font* font) {
    if (!m_visible) return;

    // Blue-purple border matching the original game group box style
    static const SDL_Color kBorder = { 75, 62, 175, 255 };

    // Measure label text so we can cut a gap in the top border line
    int labelW = 0, labelH = 0;
    if (font && !m_label.empty())
        TTF_SizeText(font, m_label.c_str(), &labelW, &labelH);

    const int textPadX = 4;   // gap between border end and text
    const int textOffX = 8;   // distance from left edge to start of text
    int gapStart = m_rect.x + textOffX - textPadX;
    int gapEnd   = m_rect.x + textOffX + labelW + textPadX;

    // Top border — left segment (2px)
    FillRect(dst, { m_rect.x, m_rect.y,     gapStart - m_rect.x, 1 }, kBorder);
    FillRect(dst, { m_rect.x, m_rect.y + 1, gapStart - m_rect.x, 1 }, kBorder);
    // Top border — right segment (2px)
    if (gapEnd < m_rect.x + m_rect.w) {
        FillRect(dst, { gapEnd, m_rect.y,     m_rect.x + m_rect.w - gapEnd, 1 }, kBorder);
        FillRect(dst, { gapEnd, m_rect.y + 1, m_rect.x + m_rect.w - gapEnd, 1 }, kBorder);
    }
    // Left border (2px)
    FillRect(dst, { m_rect.x,     m_rect.y, 1, m_rect.h }, kBorder);
    FillRect(dst, { m_rect.x + 1, m_rect.y, 1, m_rect.h }, kBorder);
    // Right border (2px)
    FillRect(dst, { m_rect.x + m_rect.w - 1, m_rect.y, 1, m_rect.h }, kBorder);
    FillRect(dst, { m_rect.x + m_rect.w - 2, m_rect.y, 1, m_rect.h }, kBorder);
    // Bottom border (2px)
    FillRect(dst, { m_rect.x, m_rect.y + m_rect.h - 1, m_rect.w, 1 }, kBorder);
    FillRect(dst, { m_rect.x, m_rect.y + m_rect.h - 2, m_rect.w, 1 }, kBorder);

    // Label text — vertically centered on the top border line
    if (font && !m_label.empty()) {
        SDL_Rect textRect = { m_rect.x + textOffX, m_rect.y - labelH / 2, labelW, labelH };
        RenderText(dst, font, m_label.c_str(), textRect, kBorder, false, false);
    }
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
    if (m_dlgWindow) {
        SDL_DestroyWindow(m_dlgWindow);
        m_dlgWindow = nullptr;
    }
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

void SDL2Dialog::LoadDialogArt() {
    if (s_artLoaded) return;
    s_artLoaded = true;

    // Convert game CDIBs to SDL surfaces using the same helper as the main menu
    auto convert = [](int dibIndex) -> SDL_Surface* {
        CDIB* pDib = theBitmaps.GetByIndex(dibIndex);
        if (!pDib) return nullptr;
        return SDL2MainMenu::CreateSurfaceFromDIB(pDib);
    };

    s_dlgBkgnd   = convert(CBitmapLib::DLG_BKGND);
    s_dlgGold    = convert(DIB_GOLD);
    s_borderHorz = convert(DIB_BORDER_HORZ);
    s_borderVert = convert(DIB_BORDER_VERT);
}

void SDL2Dialog::FreeDialogArt() {
    if (s_dlgBkgnd)   { SDL_FreeSurface(s_dlgBkgnd);   s_dlgBkgnd = nullptr; }
    if (s_dlgGold)     { SDL_FreeSurface(s_dlgGold);     s_dlgGold = nullptr; }
    if (s_borderHorz)  { SDL_FreeSurface(s_borderHorz);  s_borderHorz = nullptr; }
    if (s_borderVert)  { SDL_FreeSurface(s_borderVert);  s_borderVert = nullptr; }
    s_artLoaded = false;
}

void SDL2Dialog::PaintGameBorder(SDL_Surface* dst, SDL_Rect rect) {
    // Gold background fill (stretched to fill dialog area)
    if (s_dlgGold) {
        SDL_BlitScaled(s_dlgGold, nullptr, dst, &rect);
    }

    // Carved-gold frame (shared corner-correct routine — fills the corners with
    // no gap; the side strips overlap the top/bottom).
    SDL2MainMenu::DrawGoldBorder(dst, rect.x, rect.y, rect.w, rect.h,
                                 s_borderHorz, s_borderVert);
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
    // Lazy-load game art on first render
    if (!s_artLoaded) LoadDialogArt();

    // Draw the dialog into the main window's surface at (m_x, m_y).
    // Widget coords are baked relative to (m_x, m_y) so we always use the main surface.
    SDL_Surface* mainSurface = SDL_GetWindowSurface(m_gameWindow->GetWindow());
    if (!mainSurface) return;

    // Restore captured background if falling back to main window display
    if (!m_dlgWindow && m_background) {
        SDL_BlitSurface(m_background, nullptr, mainSurface, nullptr);
    }

    SDL_Rect dlgRect = { m_x, m_y, m_width, m_height };
    int borderTop = s_borderHorz ? s_borderHorz->h : 3;
    int borderSide = s_borderVert ? s_borderVert->w : 3;

    const int titleBarH = 26;

    if (s_artLoaded && (s_dlgGold || s_borderHorz)) {
        PaintGameBorder(mainSurface, dlgRect);

        SDL_Rect interior = {
            dlgRect.x + borderSide, dlgRect.y + borderTop,
            dlgRect.w - 2 * borderSide, dlgRect.h - 2 * borderTop
        };
        if (m_customBg) {
            // Custom background goes BELOW the title bar at natural size
            SDL_Rect bgRect = {
                interior.x, interior.y + titleBarH,
                interior.w, interior.h - titleBarH
            };
            SDL_BlitScaled(m_customBg, nullptr, mainSurface, &bgRect);
        } else if (s_dlgGold) {
            SDL_BlitScaled(s_dlgGold, nullptr, mainSurface, &interior);
        }
    } else {
        FillRect(mainSurface, dlgRect, UIColors::DialogBg);
        DrawBevel(mainSurface, dlgRect, 3, UIColors::DialogFrame, UIColors::DialogDark);
    }

    SDL_Rect titleRect = { m_x + borderSide, m_y + borderTop, m_width - 2 * borderSide, titleBarH };
    FillRect(mainSurface, titleRect, UIColors::TitleBg);

    TTF_Font* titleFont = GetFont(16);
    if (titleFont) {
        SDL_Rect titleTextRect = { titleRect.x + 6, titleRect.y, titleRect.w - 12, titleRect.h };
        RenderText(mainSurface, titleFont, m_title.c_str(), titleTextRect,
                   UIColors::TitleText, false, true);
    }

    TTF_Font* widgetFont = GetFont(13);
    for (auto& widget : m_widgets)
        widget->Render(mainSurface, widgetFont);
    // Focus highlight in a SECOND pass, after all widgets are drawn — otherwise a
    // neighbouring widget (e.g. the next stacked button) renders over the bottom
    // edge of the focus box and the box looks clipped.
    for (auto& widget : m_widgets) {
        if (widget->HasFocus()) {
            SDL_Rect r = widget->GetRect();
            r.x -= 2; r.y -= 2; r.w += 4; r.h += 4;
            DrawBevel(mainSurface, r, 1, {180, 200, 220, 255}, {180, 200, 220, 255});
        }
    }

    if (m_dlgWindow) {
        // Keep the dialog above the game/sibling windows each frame.
        //
        // NON-MODAL dialogs (build menu, etc.): do it WITHOUT stealing focus —
        // SDL_RaiseWindow() calls SetForegroundWindow() on Windows, which yanked
        // OS focus back ~60x/sec and broke the Start menu / interacting with other
        // windows. Use SWP_NOACTIVATE, and only when our process is foreground.
        //
        // MODAL dialogs (research, options, etc.) likewise MUST sit on top of the
        // detached map/unit windows — they block the game loop, so a modal that
        // opened BEHIND a detached map left the frozen map looking "unresponsive"
        // while the real dialog was hidden. The one-time SDL_RaiseWindow() in
        // DoModal() gives the modal its initial focus; here we only need to keep
        // it above siblings. Use the SAME non-activating bump as the non-modal
        // path — calling SDL_RaiseWindow() per frame re-runs SetForegroundWindow()
        // ~60x/sec, which yanked OS focus back and stopped the user from alt-tabbing
        // away or interacting with other windows while the Game Options dialog was up.
#ifdef _WIN32
        SDL_SysWMinfo wm; SDL_VERSION(&wm.version);
        if (SDL_GetWindowWMInfo(m_dlgWindow, &wm)) {
            HWND hDlg = wm.info.win.window;
            DWORD forePid = 0;
            ::GetWindowThreadProcessId(::GetForegroundWindow(), &forePid);
            if (forePid == ::GetCurrentProcessId())
                ::SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0,
                               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
#else
        SDL_RaiseWindow(m_dlgWindow);
#endif

        // If the window has moved to a different monitor, SDL's cached window
        // framebuffer surface is stale (it's only rebuilt on resize). Destroy
        // it so the SDL_GetWindowSurface below recreates one bound to the new
        // monitor — otherwise a dialog dragged to monitor 2/3 renders nothing.
        int disp = SDL_GetWindowDisplayIndex(m_dlgWindow);
        if (disp >= 0 && disp != m_dlgLastDisplay) {
            if (m_dlgLastDisplay != -1)
                SDL_DestroyWindowSurface(m_dlgWindow);
            m_dlgLastDisplay = disp;
        }

        // Copy the dialog rectangle from the main surface into the dedicated dialog window
        SDL_Surface* dlgSurface = SDL_GetWindowSurface(m_dlgWindow);
        if (dlgSurface) {
            SDL_Rect src = { m_x, m_y, m_width, m_height };
            SDL_BlitSurface(mainSurface, &src, dlgSurface, nullptr);
            SDL_UpdateWindowSurface(m_dlgWindow);
        }
    } else {
        SDL_UpdateWindowSurface(m_gameWindow->GetWindow());
    }
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
    // --- Title-bar drag: move the borderless dialog window (incl. to another
    // monitor). The window is borderless so there's no OS title bar to grab;
    // we drive the move ourselves via SDL_SetWindowPosition. Only the OS window
    // position changes — m_x/m_y (render offset + coord shift) stay put, so the
    // content keeps rendering correctly and widget hit-testing is unaffected.
    if (m_dlgWindow) {
        uint32_t dlgWinID = SDL_GetWindowID(m_dlgWindow);
        int borderTop  = s_borderHorz ? s_borderHorz->h : 3;
        int borderSide = s_borderVert ? s_borderVert->w : 3;
        const int titleBarH = 26;

        if (event.type == SDL_MOUSEBUTTONDOWN &&
            event.button.button == SDL_BUTTON_LEFT &&
            event.button.windowID == dlgWinID) {
            // event coords are dialog-window-local (0,0)-based
            int lx = event.button.x, ly = event.button.y;
            if (lx >= borderSide && lx < m_width - borderSide &&
                ly >= borderTop && ly < borderTop + titleBarH) {
                m_dlgDragging = true;
                SDL_GetGlobalMouseState(&m_dlgDragMouseX, &m_dlgDragMouseY);
                SDL_GetWindowPosition(m_dlgWindow, &m_dlgDragWinX, &m_dlgDragWinY);
                SDL_CaptureMouse(SDL_TRUE);   // keep motion events if cursor outruns window
                return true;
            }
        }
        if (event.type == SDL_MOUSEMOTION && m_dlgDragging) {
            int gx, gy;
            SDL_GetGlobalMouseState(&gx, &gy);
            SDL_SetWindowPosition(m_dlgWindow,
                                  m_dlgDragWinX + (gx - m_dlgDragMouseX),
                                  m_dlgDragWinY + (gy - m_dlgDragMouseY));
            return true;
        }
        if (event.type == SDL_MOUSEBUTTONUP && m_dlgDragging &&
            event.button.button == SDL_BUTTON_LEFT) {
            m_dlgDragging = false;
            SDL_CaptureMouse(SDL_FALSE);
            return true;
        }
    }

    // When using a dedicated dialog window, mouse coordinates arrive relative to
    // that window's (0,0). Widgets use coordinates relative to the main window
    // (m_x + offset). Shift mouse events so they match widget rects.
    SDL_Event shifted = event;
    if (m_dlgWindow) {
        uint32_t dlgWinID = SDL_GetWindowID(m_dlgWindow);
        switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (event.button.windowID == dlgWinID) {
                shifted.button.x += m_x;
                shifted.button.y += m_y;
            }
            break;
        case SDL_MOUSEMOTION:
            if (event.motion.windowID == dlgWinID) {
                shifted.motion.x += m_x;
                shifted.motion.y += m_y;
            }
            break;
        case SDL_MOUSEWHEEL:
            // Wheel events carry the mouse position (SDL 2.0.18+) in dialog-window
            // coords; shift to main-window coords so widgets hit-test correctly.
            if (event.wheel.windowID == dlgWinID) {
                shifted.wheel.mouseX += m_x;
                shifted.wheel.mouseY += m_y;
            }
            break;
        }
        event = shifted;
    }

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

    // Create a dedicated borderless top-level window for this dialog, owned by
    // the main game window. Being its own OS window (not a composited panel) is
    // intentional: the user can drag it out onto a second monitor. The title
    // bar drag is implemented in HandleEvent (the window is borderless, so there
    // is no OS caption to grab).
    int mainX = 0, mainY = 0;
    if (m_gameWindow->GetWindow())
        SDL_GetWindowPosition(m_gameWindow->GetWindow(), &mainX, &mainY);

    const std::string& titleStr = m_title.empty() ? "Dialog" : m_title;
    m_dlgWindow = GameWindow::CreateSDLWindow(
        titleStr.c_str(),
        mainX + m_x, mainY + m_y, m_width, m_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SKIP_TASKBAR);

    if (!m_dlgWindow) {
        // Fallback: render on the main window with a background snapshot
        CaptureBackground();
    } else {
#ifdef _WIN32
        // Set the background window as owner so the dialog stays above it
        if (m_gameWindow->GetWindow()) {
            SDL_SysWMinfo mainInfo, dlgInfo;
            SDL_VERSION(&mainInfo.version);
            SDL_VERSION(&dlgInfo.version);
            if (SDL_GetWindowWMInfo(m_gameWindow->GetWindow(), &mainInfo) &&
                SDL_GetWindowWMInfo(m_dlgWindow, &dlgInfo))
                ::SetWindowLongPtr(dlgInfo.info.win.window, GWLP_HWNDPARENT,
                                   (LONG_PTR)mainInfo.info.win.window);
        }
#endif
        SDL_RaiseWindow(m_dlgWindow);
    }

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

        OnFrame();

        Render();

        // Don't burn CPU
        SDL_Delay(16);

        // SDL receives mouse/keyboard events for its own windows via its
        // registered WndProc — no MFC message pump needed here.
        // Pumping MFC messages inside DoModal causes re-entrant BuildUnit()
        // calls (WM_KICKIDLE → OnIdle → BaseYield → second DoModal), which
        // creates nested dialogs that block each other.
    }

    if (m_dlgWindow) {
        SDL_DestroyWindow(m_dlgWindow);
        m_dlgWindow = nullptr;
    }

    return m_result;
}

void SDL2Dialog::EndDialog(int result) {
    m_result = result;
    m_running = false;

    if (m_nonModal) {
        // Destroy window immediately so it vanishes this frame
        if (m_dlgWindow) {
            SDL_DestroyWindow(m_dlgWindow);
            m_dlgWindow = nullptr;
        }
        // Fire callback (e.g., clears the caller's pointer to us)
        if (m_onDone) m_onDone(result);
        // GameWindow will delete this during its next cleanup pass
        // (UnregisterDialog detects IsNonModalActive() == false)
    }
}

void SDL2Dialog::ShowNonModal(std::function<void(int)> onDone) {
    m_nonModal = true;
    m_onDone   = onDone;
    m_running  = true;
    m_result   = 0;

    int mainX = 0, mainY = 0;
    if (m_gameWindow->GetWindow())
        SDL_GetWindowPosition(m_gameWindow->GetWindow(), &mainX, &mainY);

    const std::string& titleStr = m_title.empty() ? "Dialog" : m_title;
    m_dlgWindow = GameWindow::CreateSDLWindow(
        titleStr.c_str(),
        mainX + m_x, mainY + m_y, m_width, m_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SKIP_TASKBAR);

    if (!m_dlgWindow) {
        CaptureBackground();
    } else {
#ifdef _WIN32
        // Set the background window as owner so the dialog stays above it
        if (m_gameWindow->GetWindow()) {
            SDL_SysWMinfo mainInfo, dlgInfo;
            SDL_VERSION(&mainInfo.version);
            SDL_VERSION(&dlgInfo.version);
            if (SDL_GetWindowWMInfo(m_gameWindow->GetWindow(), &mainInfo) &&
                SDL_GetWindowWMInfo(m_dlgWindow, &dlgInfo))
                ::SetWindowLongPtr(dlgInfo.info.win.window, GWLP_HWNDPARENT,
                                   (LONG_PTR)mainInfo.info.win.window);
        }
#endif
        SDL_RaiseWindow(m_dlgWindow);
    }

    OnInit();

    // Register with GameWindow for per-frame event routing and rendering
    if (m_gameWindow)
        m_gameWindow->RegisterDialog(this);
}

uint32_t SDL2Dialog::GetSDLWindowID() const {
    return m_dlgWindow ? SDL_GetWindowID(m_dlgWindow) : 0;
}
