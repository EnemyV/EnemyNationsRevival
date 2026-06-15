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
struct SDL_Surface;

// Start the control server if the EN_HARNESS env var is set. Safe to call once
// after the game window exists. Port comes from EN_HARNESS_PORT (default 7070).
void EnHarness_Start(SDL_Window* window, SDL_Renderer* renderer);

// Call every frame on the main thread (e.g. from GameWindow::PollEvents). Services
// pending screenshot requests, which must touch SDL on the render thread.
void EnHarness_Service();

// Register the main window's CPU back-buffer (GameWindow::GetPresentSurface in
// renderer mode). When set, `shot` of the main window dumps this surface directly
// instead of SDL_RenderReadPixels — needed on macOS, where the Metal/GL render
// target reads back blank, and on headless sessions with no on-screen drawable.
// The compositor draws the full frame into this surface every present, so it
// always holds the real composited image. Pass nullptr to clear.
void EnHarness_SetMainSurface(SDL_Surface* surface);

// Register a detached panel's CPU back-surface by its SDL window id, so `shotid`
// can dump it directly (reliable on macOS, where GPU read-back of child windows
// is blank/garbage). Call each frame from the panel's render. Pass nullptr to
// clear (e.g. on panel destroy).
void EnHarness_RegisterWindowSurface(unsigned int windowId, SDL_Surface* surface);

#endif // EN_HARNESS_H
