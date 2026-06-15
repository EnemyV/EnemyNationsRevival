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

    // Per-widget font size override (in points). 0 = use the dialog's default
    // widget font. SDL2Dialog::Render() resolves this to a cached TTF_Font.
    void SetFontSize(int pt) { m_fontOverride = pt; }
    int  GetFontSize() const { return m_fontOverride; }

protected:
    SDL_Rect m_rect;
    bool m_visible = true;
    bool m_enabled = true;
    bool m_focused = false;
    int  m_fontOverride = 0;
};

// ============================================================================
// SDL2Label - Static text display
// ============================================================================
class SDL2Label : public SDL2Widget {
public:
    SDL2Label(int x, int y, int w, int h, const std::string& text,
              SDL_Color color = {48, 58, 148, 255});
    ~SDL2Label() override;

    void Render(SDL_Surface* dst, TTF_Font* font) override;
    void SetText(const std::string& text) { m_text = text; }
    const std::string& GetText() const { return m_text; }
    void SetColor(SDL_Color c) { m_color = c; }
    void SetCentered(bool c) { m_centered = c; }
    void SetWrapped(bool w) { m_wrapped = w; }
    void SetTopAligned(bool t) { m_topAligned = t; }
    void SetRightAligned(bool r) { m_rightAligned = r; }
    // Faux-bold: render the glyphs a second time shifted 1px (no font-style mutation,
    // so it can't affect other labels sharing the cached font). Default off.
    void SetBold(bool b) { m_bold = b; }

private:
    std::string m_text;
    SDL_Color m_color;
    bool m_centered = false;
    bool m_wrapped = false;
    bool m_topAligned = false;
    bool m_rightAligned = false;
    bool m_bold = false;

    // Cached rasterized text. Re-rendering TTF every frame (especially the wrapped
    // multi-line variant) is expensive enough to visibly drop the game's framerate
    // while a non-modal dialog is open. We keep the rendered surface and only
    // regenerate it when an input that affects the glyphs changes.
    SDL_Surface* m_cache = nullptr;
    std::string  m_cacheText;
    SDL_Color    m_cacheColor = {0, 0, 0, 0};
    TTF_Font*    m_cacheFont = nullptr;
    int          m_cacheWrapW = -1;     // rect.w used for the cached wrap, or -1
    bool         m_cacheWrapped = false;
    bool         m_cacheEnabled = true;
};

// ============================================================================
// SDL2Button - Clickable button with optional double-click support
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
    // Override the default gold button-text color (research buttons use red, matching
    // CDlgResearch::OnDrawItem's PALETTERGB(255,33,8)).
    void SetTextColor(SDL_Color c) { m_textColor = c; m_customTextColor = true; }

    // Set a double-click callback. Fires when the button is clicked twice
    // within 500ms (matching the MFC MSG_BTN_DBLCLK behavior).
    void SetOnDblClick(ClickCallback cb) { m_onDblClick = cb; }

    // Set an icon surface and source rect to render above the text.
    // The surface is NOT owned by the button — caller must keep it alive.
    void SetIcon(SDL_Surface* sheet, SDL_Rect srcRect) { m_iconSheet = sheet; m_iconSrc = srcRect; }
    void ClearIcon() { m_iconSheet = nullptr; }

    // Set a 3-state sprite sheet as the button background (normal | pressed | disabled).
    // The sheet is divided into 3 equal horizontal sections. NOT owned by the button.
    void SetBtnSheet(SDL_Surface* sheet) { m_btnSheet = sheet; }

    // Toggle state — keeps button visually pressed (like original m_cState & 0x01)
    void SetToggled(bool t) { m_toggled = t; }
    bool IsToggled() const { return m_toggled; }

private:
    std::string m_text;
    ClickCallback m_onClick;
    ClickCallback m_onDblClick;
    bool m_pressed = false;
    bool m_toggled = false;
    bool m_hovered = false;
    bool m_customTextColor = false;
    SDL_Color m_textColor = { 225, 182, 55, 255 };  // default gold
    SDL_Surface* m_iconSheet = nullptr;
    SDL_Rect m_iconSrc = {0, 0, 0, 0};
    SDL_Surface* m_btnSheet = nullptr;  // 3-state sprite sheet background

    // Double-click tracking
    Uint32 m_lastClickTime = 0;
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
    // Hide the slider's built-in value number (drawn just right of the track).
    // Callers that render their own readout next to the slider (e.g. the Load
    // Truck dialog's "loaded/available" column) turn this off to avoid a second,
    // overlapping number.
    void SetShowValue(bool s) { m_showValue = s; }

private:
    int m_minVal, m_maxVal, m_value;
    ChangeCallback m_onChange;
    bool m_dragging = false;
    bool m_showValue = true;

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
    // Scroll so that item idx is within the visible window (no-op if already visible).
    void EnsureVisible(int idx);
    // A persistently-highlighted "active" row, independent of the browse selection
    // (e.g. the research item currently being worked on). -1 = none.
    void SetMarked(int m) { m_marked = m; }
    int GetMarked() const { return m_marked; }
    // Override the default white-bg / blue-text / gold-on-blue-selection scheme.
    // The research list uses this to match the original red-on-white (normal) /
    // red-on-black (selected) look from CDlgResearch::OnDrawItem.
    void SetColors(SDL_Color bg, SDL_Color selBg, SDL_Color text, SDL_Color selText) {
        m_colBg = bg; m_colSelBg = selBg; m_colText = text; m_colSelText = selText;
    }
    int GetCount() const { return (int)m_items.size(); }
    void* GetItemData(int index) const;
    const std::string& GetItemText(int index) const;

private:
    struct Item { std::string text; void* data; };
    std::vector<Item> m_items;
    int m_selected = -1;
    int m_marked = -1;
    int m_scrollOffset = 0;

    // Row colors (defaults mirror the historic white/blue/gold scheme).
    SDL_Color m_colBg      = { 255, 255, 255, 255 };
    SDL_Color m_colSelBg   = {  48,  58, 148, 255 };
    SDL_Color m_colText    = {  48,  58, 148, 255 };
    SDL_Color m_colSelText = { 225, 182,  55, 255 };
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
    void SetText(const std::string& t) { m_text = t; m_cursorPos = (int)t.size(); ClearSelection(); }
    void SetFocused(bool f) { m_focused = f; }
    bool IsFocused() const { return m_focused; }

    // Override the default white-bg / blue-text scheme. Used by dark in-game
    // dialogs (e.g. Build Vehicle) where a stock white field looks out of place.
    void SetColors(SDL_Color bg, SDL_Color text) {
        m_bgColor = bg; m_textColor = text; m_customColors = true;
    }

    // Multi-line mode: Enter inserts a newline (instead of triggering the dialog's
    // OK), text renders on multiple rows, and the cursor moves in 2D. Used by the
    // email-compose body field.
    void SetMultiline(bool m) { m_multiline = m; }
    bool IsMultiline() const { return m_multiline; }

private:
    // Map a pixel X coordinate to a character index in m_text
    int XToCharIndex(int pixelX) const;
    // Get pixel X offset for a character index
    int CharIndexToX(int index) const;
    // --- Multi-line helpers (used only when m_multiline) ---
    void ComputeLineStarts(std::vector<int>& starts) const;  // text index of each line start
    void IndexToLineCol(int index, int& line, int& col) const;
    int  XYToCharIndex(int px, int py) const;                 // pixel -> flat index
    int  LineHeight() const;                                  // one row's pixel height
    // Delete the selected text range, leaving cursor at selection start
    void DeleteSelection();
    // Return true if there is an active selection
    bool HasSelection() const { return m_selStart != m_selEnd; }
    // Clear selection (no text deleted)
    void ClearSelection() { m_selStart = m_selEnd = m_cursorPos; }
    // Get ordered selection bounds
    int SelMin() const { return m_selStart < m_selEnd ? m_selStart : m_selEnd; }
    int SelMax() const { return m_selStart > m_selEnd ? m_selStart : m_selEnd; }

    std::string m_text;
    ChangeCallback m_onChange;
    int m_cursorPos = 0;
    int m_selStart = 0;       // Anchor point of selection
    int m_selEnd = 0;         // Moving end of selection (== cursorPos during selection)
    bool m_dragging = false;  // Mouse button held for drag-select
    TTF_Font* m_cachedFont = nullptr;  // Cached from last Render for hit-testing

    bool m_customColors = false;
    SDL_Color m_bgColor   = { 255, 255, 255, 255 };
    SDL_Color m_textColor = {  48,  58, 148, 255 };

    bool m_multiline = false;  // Enter inserts newline; 2D cursor (email-compose body)
};

// ============================================================================
// SDL2GroupBox - Labeled rectangular border (like MFC SS_GROUPBOX)
// ============================================================================
class SDL2GroupBox : public SDL2Widget {
public:
    SDL2GroupBox(int x, int y, int w, int h, const std::string& label);
    void Render(SDL_Surface* dst, TTF_Font* font) override;
private:
    std::string m_label;
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
// SDL2ProgressBar - Horizontal determinate progress bar (0..100%)
// ============================================================================
class SDL2ProgressBar : public SDL2Widget {
public:
    SDL2ProgressBar(int x, int y, int w, int h) : SDL2Widget(x, y, w, h) {}

    void Render(SDL_Surface* dst, TTF_Font* font) override;

    // Determinate mode: clamped to [0,100]. Set bShowText=false to hide "NN%".
    void SetProgress(int pct) { m_pct = pct < 0 ? 0 : (pct > 100 ? 100 : pct); }
    int  GetProgress() const { return m_pct; }
    void SetShowText(bool b) { m_showText = b; }

    // Indeterminate mode: a highlight block sweeps left-to-right to signal
    // ongoing work when the total isn't known ahead of time (e.g. the save
    // compressor reports a block index but no block count). No percentage shown.
    void SetIndeterminate(bool b) { m_indeterminate = b; }

private:
    int  m_pct = 0;
    bool m_showText = true;
    bool m_indeterminate = false;
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

    // Open the dialog non-modally. Returns immediately; the dialog renders each
    // frame via GameWindow. onDone is called with the result when EndDialog fires.
    // The dialog must be heap-allocated; GameWindow deletes it after onDone.
    void ShowNonModal(std::function<void(int)> onDone = nullptr);

    // True while the non-modal dialog is still open (EndDialog not yet called)
    bool IsNonModalActive() const { return m_nonModal && m_running; }

    // Force this dialog above the ALWAYS_ON_TOP detached Area/World windows and
    // give it focus. The per-frame z-order bump only uses HWND_TOP, which leaves a
    // plain dialog BELOW the topmost detached map; an auto-opened alert (e.g. the
    // first research center finishing) would then hide behind the map. This joins
    // the topmost band so the per-frame HWND_TOP bump keeps it on top, and takes
    // focus once so the player actually notices. No-op if the window isn't open.
    void RaiseAndAlert();

    // SDL window ID for this dialog's dedicated window (0 if not open)
    uint32_t GetSDLWindowID() const;

    // Called by GameWindow each frame for active non-modal dialogs
    void ProcessEventNonModal(SDL_Event& event) {
        HandleEvent(event);
        // Force an immediate repaint for meaningful interaction (clicks, keys,
        // text, wheel) and for a live title-bar drag, so input stays responsive.
        // Plain hover MOTION is intentionally left to the fixed-interval throttle:
        // otherwise moving the mouse across the dialog re-renders + re-presents its
        // window at full rate, and that per-frame present visibly drags the gameplay
        // framerate (e.g. lag while the build-structure menu is open). The hover
        // state still updates immediately above; only the repaint is throttled.
        if (event.type != SDL_MOUSEMOTION || m_dlgDragging)
            m_forceFrame = true;
    }
    // Throttle the per-frame repaint: a non-modal dialog renders into its own
    // window and presents it every game frame, which steals frames from the game.
    // Repaint on a fixed interval (enough for the research flask animation) and
    // immediately after any event so input stays responsive.
    void RenderFrameNonModal() {
        Uint32 now = SDL_GetTicks();
        if (!m_forceFrame && (now - m_lastFrameMs) < FRAME_INTERVAL_MS)
            return;
        m_lastFrameMs = now;
        m_forceFrame = false;
        OnFrame();   // per-frame logic (e.g. chat/mail live-refresh) — modal got
                     // this from DoModal; the non-modal path needs it too.
        Render();
    }

    // Close the dialog with a result
    void EndDialog(int result);

protected:
    // Tear down a MODAL dialog's window IMMEDIATELY and stop it rendering again.
    // EndDialog only flags DoModal's loop to exit — the window survives until the
    // loop unwinds. A button handler that then runs a long NESTED flow (Load/Exit:
    // world teardown + the main-menu load flow) blocks that unwind, so the dialog
    // would otherwise linger on-screen through the whole flow (including the player
    // pick). Call this instead of EndDialog when kicking off such a flow.
    void DismissModalNow();

    // Override to initialize widgets
    virtual void OnInit() {}

    // Called once per frame while the dialog is running (both modal and
    // non-modal). Override to do per-frame polling (e.g. refresh a list
    // from an async data source). Default is a no-op.
    virtual void OnFrame() {}

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

    // Set a custom interior background surface (stretched to fit dialog interior).
    // Used by specialized dialogs like Build Structure that have their own background art.
    // NOT owned by the dialog — caller keeps it alive.
    void SetCustomBackground(SDL_Surface* bg) { m_customBg = bg; }

    // Opt out of the per-frame HWND_TOP bump (see m_keepOnTop). A non-modal dialog
    // that calls this can be tucked behind the area map instead of floating on top.
    void SetKeepOnTop(bool v) { m_keepOnTop = v; }

    // Override the default 13pt widget font for this dialog's labels/buttons. Lets
    // a specific window (e.g. the building-info windows) run slightly larger text
    // without changing every other dialog. A per-widget SetFontSize still wins.
    void SetWidgetFontSize(int pt) { m_widgetFontSize = pt; }

    // Full-bleed background: fills the ENTIRE dialog rect — no gold border, no title
    // bar — with this surface, stretched to fit. Used by the full-screen end-game
    // (win/lose/scenario) screens that mirror CWndCutScene's full-screen painting.
    // If takeOwnership is true the dialog frees the surface on destruction.
    void SetFullscreenBackground(SDL_Surface* surf, bool takeOwnership) {
        m_fullscreenBg = surf;
        m_ownFullscreenBg = takeOwnership;
        m_chromeless = true;
    }

    // Usable interior geometry — the area below the title bar and inside the gold
    // border. Layout code should anchor to these instead of guessing pixel offsets
    // from m_x/m_y, so content never slides under the title bar (the border art
    // height varies). LoadDialogArt() is idempotent; calling it here guarantees the
    // metrics match what Render() uses even when OnInit runs before the first paint.
    static const int kTitleBarH = 26;
    int DlgBorderTop()  { LoadDialogArt(); return s_borderHorz ? s_borderHorz->h : 6; }
    int DlgBorderSide() { LoadDialogArt(); return s_borderVert ? s_borderVert->w : 6; }
    int ContentTop()    { return m_y + DlgBorderTop() + kTitleBarH; }
    int ContentLeft()   { return m_x + DlgBorderSide(); }
    int ContentRight()  { return m_x + m_width  - DlgBorderSide(); }
    int ContentBottom() { return m_y + m_height - DlgBorderTop(); }

public:
    // Load game art bitmaps (DLG_BKGND, DIB_GOLD, borders) for dialog rendering.
    // Called once after theBitmaps is initialized; all dialogs share the cached surfaces.
    static void LoadDialogArt();
    static void FreeDialogArt();

protected:

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

    // Paint dialog frame using game art (gold background + horizontal/vertical borders)
    void PaintGameBorder(SDL_Surface* dst, SDL_Rect rect);

    std::vector<std::unique_ptr<SDL2Widget>> m_widgets;
    int m_focusIndex = -1;  // Index of currently focused widget, -1 = none
    int m_result = 0;
    bool m_running = false;
    int m_widgetFontSize = 13;  // default widget text size (see SetWidgetFontSize)

    // Non-modal state
    bool m_nonModal = false;
    std::function<void(int)> m_onDone;

    // Per-frame z-order: modal dialogs always re-assert HWND_TOP each frame so they
    // float above the detached Area/World map windows. Non-modal dialogs do too by
    // default (keeps the build menu visible over the map), but a dialog can opt out
    // via SetKeepOnTop(false) so the user can tuck it behind the map — e.g. the
    // Relations window, which otherwise felt "stuck on top".
    bool m_keepOnTop = true;

    // Per-frame repaint throttle for the non-modal path (see RenderFrameNonModal).
    Uint32 m_lastFrameMs = 0;
    bool   m_forceFrame = true;
    static const Uint32 FRAME_INTERVAL_MS = 142;  // ~7 fps (forced repaint on input keeps it responsive)

    // Dedicated SDL_Window for this dialog (ALWAYS_ON_TOP so it floats above
    // the Area View and World View detached panels).
    SDL_Window*  m_dlgWindow = nullptr;

    // Set by DismissModalNow(): the window is gone and Render() must be a no-op
    // (otherwise the final Render() in DoModal's loop would paint this dialog onto
    // the MAIN window — see Render()'s m_dlgWindow==null fallback).
    bool         m_dismissed = false;

    // Title-bar drag state. The dialog window is borderless, so we move it
    // ourselves when the user drags its title bar. m_x/m_y (render offset +
    // event-coord shift) are intentionally NOT touched — only the OS window
    // position moves, so the dialog can be dragged anywhere, even onto a
    // second monitor.
    bool m_dlgDragging = false;
    int  m_dlgDragMouseX = 0, m_dlgDragMouseY = 0;  // global mouse pos at grab
    int  m_dlgDragWinX = 0,   m_dlgDragWinY = 0;    // window pos at grab

    // Which display the dialog window is currently on. SDL caches the window
    // framebuffer surface and only rebuilds it on resize, NOT when the window
    // crosses to another monitor — so a dialog dragged to a second monitor
    // keeps presenting through the stale (monitor-1) surface and shows nothing.
    // We watch the display index and force the surface to be recreated on change.
    int  m_dlgLastDisplay = -1;

    // Background snapshot (captured when dialog opens; only used when rendering
    // to the main window as a fallback)
    SDL_Surface* m_background = nullptr;

    // Font cache
    std::string m_fontPath;
    std::unordered_map<int, TTF_Font*> m_fontCache;

    // Custom interior background (stretched instead of tiled DLG_BKGND)
    SDL_Surface* m_customBg = nullptr;

    // Full-screen chromeless background (see SetFullscreenBackground). When set,
    // Render() skips the gold border + title bar and stretches this over the whole
    // dialog rect, then draws the widgets on top.
    SDL_Surface* m_fullscreenBg = nullptr;
    bool m_ownFullscreenBg = false;
    bool m_chromeless = false;

    // Shared game art surfaces (loaded once from theBitmaps)
    static SDL_Surface* s_dlgBkgnd;      // DLG_BKGND — dialog background texture
    static SDL_Surface* s_dlgGold;        // DIB_GOLD — gold fill behind borders
    static SDL_Surface* s_borderHorz;     // DIB_BORDER_HORZ — horizontal border strip
    static SDL_Surface* s_borderVert;     // DIB_BORDER_VERT — vertical border strip
    static bool s_artLoaded;
};