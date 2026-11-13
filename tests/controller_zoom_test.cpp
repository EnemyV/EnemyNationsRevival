#ifdef NDEBUG
#undef NDEBUG
#endif
#define SDL_MAIN_HANDLED
#include "../enations_latest/src/controller_zoom.h"
#include <cassert>
#include <cstdio>

static SDL_Event buttonEvent(SDL_Joystick* joystick, int button, bool pressed) {
    assert(SDL_JoystickSetVirtualButton(joystick, button, pressed ? 1 : 0) == 0);
    SDL_JoystickUpdate();
    SDL_Event event;
    // SDL may end a poll cycle at an internal sentinel while newer events remain
    // queued behind it. Begin another poll cycle before declaring delivery absent.
    for (int cycle = 0; cycle < 4; ++cycle) {
        while (SDL_PollEvent(&event)) {
            if (event.type == (pressed ? SDL_CONTROLLERBUTTONDOWN : SDL_CONTROLLERBUTTONUP) &&
                event.cbutton.button == button) return event;
        }
    }
    std::fprintf(stderr, "virtual controller event missing: button=%d pressed=%d actual=%d error=%s\n",
                 button, pressed ? 1 : 0, (int)SDL_JoystickGetButton(joystick, button), SDL_GetError());
    assert(false && "virtual controller event missing");
    return SDL_Event{};
}

int main() {
    assert(ControllerZoomMayRoute(true, false));
    assert(!ControllerZoomMayRoute(false, false));
    assert(!ControllerZoomMayRoute(true, true));
    SDL_SetMainReady();
    // This headless test has no focused window; allow controller events anyway.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_setenv("EN_GAMEPAD_ZOOM", "1", 1);
    assert(SDL_Init(SDL_INIT_JOYSTICK) == 0);
    int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 15, 1);
    assert(index >= 0);
    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(index);
    SDL_Joystick* joystick = SDL_JoystickOpen(index); assert(joystick);
    ControllerZoom zoom; zoom.init(); assert(zoom.enabled() && zoom.attached());
    SDL_Event e = buttonEvent(joystick, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, true);
    assert(e.cbutton.which == id);
    assert(zoom.event(e, true) == -1);
    zoom.reconcile();
    assert(zoom.event(e, true) == 0); // still physically held
    e = buttonEvent(joystick, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, false);
    assert(zoom.event(e, true) == 0);
    e = buttonEvent(joystick, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, true);
    assert(zoom.event(e, false) == 0); // press while dialog/focus blocks input
    assert(zoom.event(e, true) == 0);  // focus returns while still held
    (void)buttonEvent(joystick, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, false); // modal consumes release
    zoom.reconcile();
    e = buttonEvent(joystick, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, true);
    assert(zoom.event(e, true) == -1);
    e = buttonEvent(joystick, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, false);
    zoom.event(e, true);
    e = buttonEvent(joystick, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, true);
    assert(zoom.event(e, true) == 1);
    SDL_JoystickClose(joystick);
    assert(SDL_JoystickDetachVirtual(index) == 0);
    assert(!zoom.attached());
    assert(zoom.event(e, true) == 0); // stale event from detached instance
    SDL_Event removed = {}; removed.type = SDL_CONTROLLERDEVICEREMOVED; removed.cdevice.which = id;
    assert(zoom.event(removed, true) == 0);
    assert(!zoom.attached());
    zoom.close(); SDL_Quit();
}
