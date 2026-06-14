//---------------------------------------------------------------------------
// en_harness.h — in-process LLM-driving harness (Linux/Debug only).
//
// A small TCP control server compiled into the game. It lets an external client
// (thin shell/python scripts) screenshot the running game and inject mouse/key
// events — the Wayland-safe, focus-free replacement for the Windows PostMessage/
// PrintWindow harness. Input is injected via SDL_PushEvent (thread-safe);
// screenshots are serviced on the main/render thread via EnHarness_Service().
//---------------------------------------------------------------------------
#ifndef EN_HARNESS_H
#define EN_HARNESS_H

struct SDL_Window;
struct SDL_Renderer;

// Start the control server if the EN_HARNESS env var is set. Safe to call once
// after the game window exists. Port comes from EN_HARNESS_PORT (default 7070).
void EnHarness_Start(SDL_Window* window, SDL_Renderer* renderer);

// Call every frame on the main thread (e.g. from GameWindow::PollEvents). Services
// pending screenshot requests, which must touch SDL on the render thread.
void EnHarness_Service();

#endif // EN_HARNESS_H
