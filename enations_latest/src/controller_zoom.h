#pragma once
#include <SDL.h>
#include <cstdlib>

// Nonmodal dialogs do not block a focused map; modal dialogs always do.
inline bool ControllerZoomMayRoute(bool focusedMap, bool modalActive) {
    return focusedMap && !modalActive;
}

// Opt-in native controller zoom coexists with wheel and keyboard input.
// Configure Steam Input to emit shoulders without duplicate wheel/key actions.
class ControllerZoom {
public:
    bool enabled() const { return active_; }
    bool attached() const { return active_ && pad_ && SDL_GameControllerGetAttached(pad_); }
    // A modal loop may consume button-up events. Reconcile only released buttons;
    // never synthesize a press or rearm a button that is still physically held.
    void reconcile() {
        if (!attached()) return;
        if (!SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) held_ &= ~1u;
        if (!SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) held_ &= ~2u;
    }
    void init() {
        const char* value = SDL_getenv("EN_GAMEPAD_ZOOM");
        if (!value || value[0] != '1' || value[1] != '\0') return;
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) return;
        active_ = true;
        openFirst();
    }
    void close() {
        if (pad_) SDL_GameControllerClose(pad_);
        pad_ = nullptr; id_ = -1; held_ = 0;
        if (active_) SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        active_ = false;
    }
    // Returns -1 for zoom in, +1 for zoom out, 0 for no action.
    int event(const SDL_Event& e, bool allowed) {
        if (!active_) return 0;
        if (e.type == SDL_CONTROLLERDEVICEREMOVED && e.cdevice.which == id_) {
            SDL_GameControllerClose(pad_); pad_ = nullptr; id_ = -1; held_ = 0;
            openFirst(); return 0;
        }
        if (e.type == SDL_CONTROLLERDEVICEADDED) { if (!pad_) openFirst(); return 0; }
        if (!attached()) return 0;
        if ((e.type != SDL_CONTROLLERBUTTONDOWN && e.type != SDL_CONTROLLERBUTTONUP) ||
            e.cbutton.which != id_) return 0;
        unsigned bit = e.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER ? 1u :
                       e.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER ? 2u : 0u;
        if (!bit) return 0;
        if (e.type == SDL_CONTROLLERBUTTONUP) { held_ &= ~bit; return 0; }
        bool fresh = (held_ & bit) == 0; held_ |= bit;
        return fresh && allowed ? (bit == 1 ? -1 : 1) : 0;
    }
private:
    void openFirst() {
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (!SDL_IsGameController(i)) continue;
            pad_ = SDL_GameControllerOpen(i);
            if (pad_) { id_ = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad_)); return; }
        }
    }
    bool active_ = false;
    SDL_GameController* pad_ = nullptr;
    SDL_JoystickID id_ = -1;
    unsigned held_ = 0;
};
