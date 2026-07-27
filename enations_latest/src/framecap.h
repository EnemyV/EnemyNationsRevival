#pragma once
// Frame-capture debug mode (#45): dump every rendered frame to BMP while enabled, so
// transient render bugs during a fast zoom/pan (fog cache, black lines, ghost trails,
// road preview, water-blend flicker) can be scrubbed frame-by-frame.
//
// OFF by default. Enable with env EN_FRAMECAP=1 (optionally EN_FRAMECAP_MAX=<frames>,
// EN_FRAMECAP_DIR=<path>), or toggle at runtime via FrameCap::Toggle() (wire to a hotkey).
// Capped at a frame budget so it can't fill the disk; auto-stops + logs when the cap is hit.
struct SDL_Renderer;

namespace FrameCap {
    // Capture the current (composited, pre-present) frame of `r` to
    // <dir>/<tag>_<seq>.bmp when enabled. Cheap no-op when disabled.
    void Capture( SDL_Renderer* r, const char* tag );
    void Toggle();        // flip on/off at runtime (resets the sequence on enable)
    bool Enabled();
}
