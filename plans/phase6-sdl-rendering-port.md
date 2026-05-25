# Phase 6 — Port wind22 rendering from DirectDraw/GDI to SDL2

Status: **planning** — no code changes yet. Goal: get the game's
in-engine rendering off DirectDraw + Win32 GDI so the binary can build
and run on Linux/macOS.

Created 2026-05-24 after the gate-collapse pass landed. Supersedes the
2026-03-22 "rendering rewrite" attempt (archived to
`archive/2026-03-rendering-rewrite/`) which tried a clean-room GPU
rewrite and never integrated.

## Background — current rendering pipeline

Today's pipeline is a hybrid:

```
CAnimAtr::Render()                  [enations_latest/src/terrain.cpp:584]
  ├─ ::GetDC(m_hwndOwner)            Win32 HDC tied to SDL window's HWND
  ├─ for each dirty rect:
  │     theMap.UpdateRect(this, rect, draw)
  │       └─ CDIB::Blt → DD surface blit OR GDI BitBlt
  ├─ ::ReleaseDC
  └─ RenderingAdapter::RenderToPanel(this, m_sdlPanel)
        └─ blit HDC content into SDL_Surface backing the panel

GameWindow main loop                [SDL_Window-owned thread]
  └─ SDL2Compositor::Render()
        ├─ blit each SDL2Panel into the window surface
        └─ SDL_UpdateWindowSurface
```

The game world (terrain, units, buildings, fog) is drawn by the old
`wind22` library into HDCs. The HDC is owned by the SDL window's HWND
on Windows. SDL2 owns the window and the composite path, but
the actual sprite/terrain drawing is still GDI/DDraw.

## Experiment 2026-05-24 (regression observed)

Forcing `CBLTFormat::CalcBltMethod()` to return `DIB_DIBSECTION`
instead of `DIB_DIRECTDRAW` (a one-line change at
`windward/wind22/src/blt.cpp:166`) compiled cleanly but produced
**fragmented rendering at runtime**: only freshly-dirtied rects
showed the map; the rest of the screen was black.

**Conclusion**: DirectDraw isn't vestigial. The off-screen DD surface
is *persistent* — unchanged regions keep their pixels between frames
because the DD surface stays alive. DIBSECTION mode either doesn't
have the same persistence in `CDIB`'s current code, or the blit-back
path differs.

This means Phase 6 has to actually port the off-screen surface, not
just delete the DD init code.

## File landmarks

### Rendering backend (the targets)

- `windward/wind22/include/blt.h` — `CBLTFormat`, `CDirectDraw`. Enums:
  `DIB_DIRECTDRAW`, `DIB_WING`, `DIB_DIBSECTION`, `DIB_MEMORY`.
- `windward/wind22/src/blt.cpp` — DD init (`LoadLibrary("DDRAW")`,
  `DirectDrawCreate`, `SetCooperativeLevel`), `CalcBltMethod`.
- `windward/wind22/include/dib.h` — `CDIB` class. Holds
  `LPDIRECTDRAWSURFACE m_pddsurfaceBack` plus the HDC + DIB section
  members. Inline `GetDDSurface()` at the bottom.
- `windward/wind22/src/dib.cpp` — `CDIB` implementations. Init, Resize,
  Blt, Lock/Unlock for direct pixel access. The `DIB_DIRECTDRAW` case
  at line 297 is the surface allocation.
- `windward/wind22/include/dibwnd.h` — `CDIBWnd`,
  `LPDIRECTDRAWCLIPPER m_pddclipper` (the screen clipper).

### Game-side rendering callers

- `enations_latest/src/terrain.cpp:584` `CAnimAtr::Render` — the main
  per-frame paint loop. Uses HDC.
- `enations_latest/src/RenderingAdapter.cpp` — the HDC→SDL_Surface
  bridge (`RenderToPanel`).
- `enations_latest/src/area.cpp`, `toolbar.cpp`, `icons.cpp`,
  `credits.cpp`, etc. — many `CDC*` paint calls that go through wind22
  rendering primitives.

### Where SDL2 already lives

- `enations_latest/src/SDL2Panel.cpp`, `SDL2Compositor.cpp` — SDL panel
  + compositor (these own SDL_Surface, no need to change).
- `enations_latest/src/GameWindow.cpp` — main SDL window, event loop,
  RegisterDialog/UnregisterDialog for non-modal SDL dialogs.

## Proposed phasing

### Stage 0 — Add SDL_Surface as a parallel CDIB backing (no behavior change)

Add a new `DIB_TYPE` value `DIB_SDL_SURFACE`. In `CDIB`:

- Add `SDL_Surface* m_psdlsurfaceBack = nullptr;`.
- On `Init()` when `m_eType == DIB_SDL_SURFACE`, create an
  `SDL_CreateRGBSurfaceWithFormat` matching `m_cx × m_cy` at the
  configured bit depth. Free in destructor.
- On `Resize()`, free + recreate.
- Don't route ANY blit through it yet. Keep `CBLTFormat::CalcBltMethod`
  returning `DIB_DIRECTDRAW` for the default path.

Goal: prove the scaffolding compiles and doesn't disturb anything. One
small commit.

### Stage 1 — Reroute the off-screen blit path through SDL

For `CDIB::Blt` (and BltDC) where source/dest are both `CDIB` objects,
add an `SDL_BlitSurface` path that runs when both are
`DIB_SDL_SURFACE`-typed. This is the inner loop that paints the map.

Still safe — only active when both ends opt in.

### Stage 2 — Switch the screen-facing surface from HDC to SDL_Surface

This is the meaningful pivot. Instead of `CAnimAtr::Render` blitting
into the HDC (which a downstream `RenderingAdapter::RenderToPanel`
then captures into an SDL_Surface), have `CAnimAtr::Render` write
**directly** into the SDL_Surface that backs its `SDL2Panel`. Cuts
out the HDC step.

Requires `CDIB` to expose an `SDL_Surface*` accessor for its bits,
and the per-frame composite call has to know to flush the panel's
surface to the window.

### Stage 3 — Switch the default `CBLTFormat::CalcBltMethod` to `DIB_SDL_SURFACE`

Flip the runtime default. DD surface still allocates as a fallback
for any code path that hasn't been ported. Verify the game runs.

### Stage 4 — Remove DirectDraw entirely

- Drop `LoadLibrary("DDRAW")` + `pfnDirectDrawCreate` from blt.cpp.
- Drop `LPDIRECTDRAWSURFACE m_pddsurfaceBack` from CDIB.
- Drop `class CDirectDraw`, `ptrtheDirectDraw`.
- Remove `#include <ddraw.h>` from `stdafx.h`.
- Drop `DIB_DIRECTDRAW` from the enum.
- Verify `mfc-status.ps1` (which doesn't check DDraw but is the
  shipping smoke gate) and dependency-walk the binary to confirm no
  `DDRAW.DLL` import.

### Stage 5 — Win32 GDI removal

The remaining `BitBlt` / `StretchBlt` / `DrawText` / `CreateCompatibleDC`
/ etc. calls in wind22 and game code need SDL or platform-portable
equivalents. This is a separate sub-phase — needs its own pass once
DDraw is out.

After this, we can attempt a Linux build (probably as `Phase 7`).

## Risks / known unknowns

- **8-bit palette art**: Source assets are 8-bit indexed-color
  (palette). `SDL_CreateRGBSurfaceWithFormat` with `SDL_PIXELFORMAT_INDEX8`
  supports this, but the per-pixel color mapping needs to happen
  somewhere — currently it happens in DD or GDI BitBlt with color
  conversion. We may need to upconvert to 32-bit RGBA on load and let
  SDL handle blending in true-color.
- **Palette animation**: Some art uses palette-cycling effects
  (water, smoke). `SDL_SetPaletteColors` works on indexed surfaces;
  needs verification.
- **Lock/Unlock semantics**: Game code locks `CDIB` for direct pixel
  access. `SDL_LockSurface` is the analog but has different
  guarantees. Audit the lock sites.
- **HDC interop**: Several SDL2 dialogs render text using GDI via
  `CDIB::GetDC()`. As long as `CDIB` retains an HDC path on Windows,
  this works. On Linux we'd need SDL_ttf for those text paths too.
- **CFrameP ainter and subclass.h**: The owner-drawn frame/button
  paint code goes through GDI in `CGlobalSubClass`. Needs porting
  alongside.

## What this plan is NOT

- Not a green-field rewrite of the renderer. The 2026-03 attempt at
  that approach failed (archived). The plan above modifies wind22
  in place, one stage at a time.
- Not GPU-accelerated. Stays on `SDL_BlitSurface` (CPU). GPU comes
  later if needed.

## Verification per stage

After each stage:

- `./build.ps1` Debug + Release clean
- `./mfc-status.ps1` still `mfc linked: NO`
- Launch the game and reach gameplay (terrain renders, units render,
  toolbar updates on hover — same smoke test we've used all session)
- For stage 4 (DDraw removal): also verify no `DDRAW.DLL` import in
  the exe (e.g. `dumpbin /imports` or `Dependencies` tool)

## Effort estimate

- Stage 0: 2-4 hours (scaffold + one commit)
- Stage 1: 4-8 hours (blit path with both backings)
- Stage 2: 1-2 days (HDC removal from the render loop)
- Stage 3: 1 day (flip default + bug-fix runtime issues)
- Stage 4: half day (mechanical cleanup once stage 3 stable)
- Stage 5: open-ended (every remaining GDI call)

Total: ~2 weeks of focused work to reach a working Linux build, plus
the actual Linux build setup which is separate (CMakeLists for non-
Windows, SDL2 install patterns, etc.).
