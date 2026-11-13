#pragma once

// Only a fresh shoulder-down can produce an action; all other controller events
// still reach ControllerZoom::event for held state and device lifecycle.
inline bool ControllerZoomNeedsTarget(bool buttonDown, bool shoulder) {
    return buttonDown && shoulder;
}

// Stateless selector for one event. Identities are borrowed only for this call.
// Attached maps share the main window and retain the existing top-area policy.
class ControllerZoomFocusSelection {
public:
    ControllerZoomFocusSelection(const void* focus, const void* mainWindow,
                                 const void* topArea, bool eligible)
        : focus_(focus), main_(mainWindow), top_(topArea), eligible_(eligible) {}
    void consider(const void* area, const void* window, bool detached, bool visible) {
        if (!eligible_ || !focus_ || !area || !visible) return;
        const bool matches = focus_ == main_
            ? (!detached && area == top_)
            : (detached && window && focus_ == window);
        if (!matches) return;
        if (match_ && match_ != area) ambiguous_ = true;
        else match_ = area;
    }
    const void* result() const { return ambiguous_ ? nullptr : match_; }
private:
    const void* focus_;
    const void* main_;
    const void* top_;
    bool eligible_;
    const void* match_ = nullptr;
    bool ambiguous_ = false;
};
