#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

class GameWindow;

// Forward declarations
class SDL2Widget;
class SDL2Dialog;

// ============================================================================
// SDL2Widget - Base class for all UI widgets
// ============================================================================
class SDL2Widget {
public:
    SDL2Widget(int x, int y, int w, int h) : m_rect{x, y, w, h} {}
    virtual ~SDL2Widget() = default;

    virtual void Render(SDL_Surface* dst, TTF_Font* font) = 0;
    virtual bool HandleEvent(const SDL_Event& event) { return false; }

    void SetVisible(bool v) { m_visible = v; }
    bool IsVisible() const { return m_visible; }
    void SetEnabled(bool e) { m_enabled = e; }
    bool IsEnabled() const { return m_enabled; }

    // Focus support for keyboard navigation
    virtual bool IsFocusable() const { return false; }
    bool HasFocus() const { return m_focused; }
    void SetFocus(bool f) { m_focused = f; }

    SDL_Rect GetRect() const { return m_rect; }
    void SetRect(int x, int y, int w, int h) { m_rect = {x, y, w, h}; }

protected:
    SDL_Rect m_rect;
    bool m_visible = true;
    bool m_enabled = true;
    bool m_focused = false;
};

// ============================================================================
// SDL2Label - Static text display
// ============================================================================
class SDL2Label : public SDL2Widget {
public:
    SDL2Label(int x, int y, int w, int h, const std::string& text,
              SDL_Color color = {255, 255, 255, 255});

    void Render(SDL_Surface* dst, TTF_Font* font) override;
    void SetText(const std::string& text) { m_text = text; }
    const std::string& GetText() const { return m_text; }
    void SetColor(SDL_Color c) { m_color = c; }
    void SetCentered(bool c) { m_centered = c; }
    void SetWrapped(bool w) { m_wrapped = w; }

private:
    std::string m_text;
    SDL_Color m_color;
    bool m_centered = false;
    bool m_wrapped = false;
};

// ============================================================================
// SDL2Button - Clickable button
// ============================================================================
class SDL2Button : public SDL2Widget {
public:
    using ClickCallback = std::function<void()>;

    SDL2Button(int x, int y, int w, int h, const std::string& text,
               ClickCallback onClick = nullptr);

    void Render(SDL_Surface* dst, TTF_Font* font) override;
    bool HandleEvent(const SDL_Event& event) override;
    bool IsFocusable() const override { return m_visible && m_enabled; }

    void SetText(const std::string& text) { m_text = text; }
    void SetOnClick(ClickCallback cb) { m_onClick = cb; }

private:
    std::string m_text;
    ClickCallback m_onClick;
    bool m_pressed = false;
    bool m_hovered = false;
};

// ============================================================================
// SDL2Slider - Horizontal slider control
// ============================================================================
class SDL2Slider : public SDL2Widget {
public:
    using ChangeCallback = std::function<void(int)>;

    SDL2Slider(int x, int y, int w, int h, int minVal, int maxVal, int value,
               ChangeCallback onChange = nullptr);

    void Render(SDL_Surface* dst, TTF_Font* font) override;
    bool HandleEvent(const SDL_Event& event) override;

    int GetValue() const { return m_value; }
    void SetValue(int v);
    void SetOnChange(ChangeCallback cb) { m_onChange = cb; }

private:
    int m_minVal, m_maxVal, m_value;
    ChangeCallback m_onChange;
    bool m_dragging = false;

    int ThumbX() const;
    int ValueFromX(int x) const;
};

// ============================================================================
// SDL2Checkbox - Toggle checkbox
// ============================================================================
class SDL2Checkbox : public SDL2Widget {
public:
    using ChangeCallback = std::function<void(bool)>;

    SDL2Checkbox(int x, int y, int w, int h, const std::string& text, bool checked = false,
                 ChangeCallback onChange = nullptr);

    void Render(SDL_Surface* dst, TTF_Font* font) override;
    bool HandleEvent(const SDL_Event& event) override;
    bool IsFocusable() const override { return m_visible && m_enabled; }

    bool IsChecked() const { return m_checked; }
    void SetChecked(bool c) { m_checked = c; }

private:
    std::string m_text;
    bool m_checked;
    ChangeCallback m_onChange;
};

// ============================================================================
// SDL2RadioGroup - Group of mutually exclusive radio buttons
// ============================================================================
class SDL2RadioGroup : public SDL2Widget {
public:
    using ChangeCallback = std::function<void(int)>;

    SDL2RadioGroup(int x, int y, int w, int h, const std::vector<std::string>& options,
                   int selected = 0, ChangeCallback onChange = nullptr);

    void Render(SDL_Surface* dst, TTF_Font* font) override;
    bool HandleEvent(const SDL_Event& event) override;
    bool IsFocusable() const override { return m_visible && m_enabled; }

    int GetSelected() const { return m_selected; }
    void SetSelected(int s) { m_selected = s; }
    void SetEnabled(int index, bool enabled);

private:
    std::vector<std::string> m_options;
    std::vector<bool> m_optEnabled;
    int m_selected;
    ChangeCallback m_onChange;
};

// ============================================================================
// SDL2Listbox - Scrollable list of items
// ============================================================================
class SDL2Listbox : public SDL2Widget {
public:
    using SelectCallback = std::function<void(int)>;
    using DblClickCallback = std::function<void(int)>;

    SDL2Listbox(int x, int y, int w, int h,
                SelectCallback onSelect = nullptr,
                DblClickCallback onDblClick = nullptr);

    void Render(SDL_Surface* dst, TTF_Font* font) override;
    bool HandleEvent(const SDL_Event& event) override;
    bool IsFocusable() const override { return m_visible && m_enabled; }

    void AddItem(const std::string& text, void* data = nullptr);
    void Clear();
    int GetSelected() const { return m_selected; }
    void SetSelected(int s) { m_selected = s; }
    int GetCount() const { return (int)m_items.size(); }
    void* GetItemData(int index) const;
    const std::string& GetItemText(int index) const;

private:
    struct Item { std::string text; void* data; };
    std::vector<Item> m_items;
    int m_selected = -1;
    int m_scrollOffset = 0;
    int m_itemHeight = 22;
    SelectCallback m_onSelect;
    DblClickCallback m_onDblClick;
    Uint32 m_lastClickTime = 0;
    int m_lastClickIndex = -1;
};

// ============================================================================
// SDL2EditBox - Single-line text input
// ============================================================================
class SDL2EditBox : public SDL2Widget {
public:
    using ChangeCallback = std::function<void(const std::string&)>;

    SDL2EditBox(int x, int y, int w, int h, const std::string& text = "",
                ChangeCallback onChange = nullptr);

    void Render(SDL_Surface* dst, TTF_Font* font) override;
    bool HandleEvent(const SDL_Event& event) override;
    bool IsFocusable() const override { return m_visible && m_enabled; }

    const std::string& GetText() const { return m_text; }
    void SetText(const std::string& t) { m_text = t; }
    void SetFocused(bool f) { m_focused = f; }
    bool IsFocused() const { return m_focused; }

private:
    std::string m_text;
    ChangeCallback m_onChange;
    int m_cursorPos = 0;
};

// ============================================================================
// SDL2Image - Displays an SDL_Surface (e.g., race picture from CDIB)
// ============================================================================
class SDL2Image : public SDL2Widget {
public:
    SDL2Image(int x, int y, int w, int h);
    ~SDL2Image();

    void Render(SDL_Surface* dst, TTF_Font* font) override;

    // Set the image surface. If takeOwnership is true, this widget frees it on destruction.
    void SetSurface(SDL_Surface* surf, bool takeOwnership = false);
    void Clear();

private:
    SDL_Surface* m_surface = nullptr;
    bool m_owned = false;
};

// ============================================================================
// SDL2Dialog - Modal dialog container
// ============================================================================
class SDL2Dialog {
public:
    SDL2Dialog(GameWindow* gameWindow, const std::string& title, int w, int h);
    virtual ~SDL2Dialog();

    // Run the dialog modally. Returns the result ID (e.g., IDOK, IDCANCEL).
    int DoModal();

    // Close the dialog with a result
    void EndDialog(int result);

protected:
    // Override to initialize widgets
    virtual void OnInit() {}

    // Override to handle OK
    virtual void OnOK() { EndDialog(1); }

    // Override to handle Cancel
    virtual void OnCancel() { EndDialog(0); }

    // Add a widget to the dialog
    template<typename T, typename... Args>
    T* AddWidget(Args&&... args) {
        auto widget = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = widget.get();
        m_widgets.push_back(std::move(widget));
        return ptr;
    }

    // Convenience: add OK and Cancel buttons at the bottom
    void AddOKCancelButtons();

    // Get font at a given size (cached via GameWindow)
    TTF_Font* GetFont(int size);

    GameWindow* m_gameWindow;
    std::string m_title;
    int m_width, m_height;

    // Dialog position (centered on screen)
    int m_x, m_y;

private:
    void Render();
    bool HandleEvent(SDL_Event& event);
    void CaptureBackground();
    void FocusNext();
    void FocusPrev();

    std::vector<std::unique_ptr<SDL2Widget>> m_widgets;
    int m_focusIndex = -1;  // Index of currently focused widget, -1 = none
    int m_result = 0;
    bool m_running = false;

    // Background snapshot (captured when dialog opens)
    SDL_Surface* m_background = nullptr;

    // Font cache
    std::string m_fontPath;
    std::unordered_map<int, TTF_Font*> m_fontCache;
};
