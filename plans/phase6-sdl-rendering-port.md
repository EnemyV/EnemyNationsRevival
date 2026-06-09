# Phase 6 — Port wind22 rendering from DirectDraw/GDI to SDL2

Status: **Stages 0–4 DONE** (DirectDraw removed; CDIB backed by
`SDL_Surface` — `CalcBltMethod` returns `DIB_SDL_SURFACE`, no `ddraw.h`/
`DDRAW.DLL`, see init.cpp:78 / blt.cpp:163). **Stage 5 (GDI removal) not
started** — `dib.cpp` still has ~100 `BitBlt`/`HDC`/`GetDC` sites and the
CDIB still keeps an HDC. Goal: get the game's in-engine rendering off
DirectDraw + Win32 GDI so the binary can build and run on Linux/macOS.
(Cross-platform build itself is scoped in
[phase7-macos-arm64.md](phase7-macos-arm64.md).)

Created 2026-05-24 after the gate-collapse pass landed. Supersedes the
2026-03-22 "rendering rewrite" attempt (archived to
`archive/2026-03-rendering-rewrite/`) which tried a clean-room GPU
rewrite and never integrated.

## Background — current rendering pipeline

Today's pipeline is a hybrid:

```
CAnimAtr::Render()                  [enations_latest/src/terrain.cpp:584]
  ├─ ::GetDC(m_hwndOwner)            HDC used ONLY for RectVisible() clip test
  ├─ for each dirty rect:
  │     theMap.UpdateRect(this, rect, draw)
  │       └─ paints terrain/sprites into the window CDIB's backing
  │          (DD off-screen surface), via raw-pixel sprite loops +
  │          some CDIB/GDI blits
  ├─ ::ReleaseDC
  └─ RenderingAdapter::RenderToPanel(this, m_sdlPanel)
        └─ copies the WHOLE CDIB (GetBits) into the SDL_Surface
           backing the panel  [RenderingAdapter.cpp:77,128]

GameWindow main loop                [SDL_Window-owned thread]
  └─ SDL2Compositor::Render()
        ├─ blit each SDL2Panel into the window surface
        └─ SDL_UpdateWindowSurface
```

The game world (terrain, units, buildings, fog) is drawn by the old
`wind22` library into the window `CDIB`, whose backing today is a
DirectDraw off-screen surface. The hot path (sprite.cpp) writes raw
pixels straight into that backing; text and some UI blits go through a
GDI HDC on the same `CDIB`. SDL2 owns the window and the composite path,
but the world drawing itself is still wind22/DDraw/GDI. (The `GetDC` in
`Render` is *only* a clip test — drawing does not target it.) See
"The persistence contract" and "Blit taxonomy" below for the details
that drive the port.

## The persistence contract (verified 2026-05-24)

This is the single most important thing to understand before touching
Stage 2-3. The dirty-rect renderer **depends on the off-screen surface
retaining its pixels between frames.** Traced end-to-end:

1. `CGameMap::Update` (terrain.cpp:3362) calls
   `UpdateRect(aa, GetRect(), invalidate)` — on the first frame this
   marks the **whole** window dirty; afterward only changed regions get
   invalidated.
2. `CAnimAtr::Render` (terrain.cpp:584) loops the dirty-rect list and
   calls `theMap.UpdateRect(rect, draw)` — which actually paints terrain
   tiles / buildings / sprites **only inside each dirty rect**, into the
   window's `CDIB` (the one created at world.cpp:403-408).
3. `RenderingAdapter::RenderToPanel` → `BlitDIBToSurface`
   (RenderingAdapter.cpp:77) copies the **entire** `CDIB`
   (`SDL_BlitSurface` with a `nullptr` src rect, line 128) into the
   `SDL2Panel` surface every frame.

So **the `CDIB`'s own backing buffer is the persistent store.** Dirty
rects overwrite sub-regions; everything else is whatever the `CDIB`
held last frame; the whole `CDIB` is then copied out. There is no
dirty-rect-limited blit-back — it's always the full surface.

**The persistence contract any replacement backing must honor:**
- allocated once, lives as long as the `CDIB` (not reallocated/cleared
  per frame);
- only the dirty sub-rects are overwritten each frame;
- readable as contiguous pixels at `GetPitch()`, top-down
  (`BlitDIBToSurface` ignores `GetDirection()`, so the surface must be
  top-down — today's window `CDIB` is, via `GetDirection()`).

A plain long-lived `SDL_Surface` (the proposed `DIB_SDL_SURFACE`)
satisfies this **by construction** — SDL surface pixels persist until
freed.

### Why the 2026-05-24 DIBSECTION experiment went black

Forcing `CalcBltMethod()` → `DIB_DIBSECTION` (one-line hack at
blt.cpp:166) compiled but rendered only freshly-dirtied rects on black.
The earlier read of this as "DirectDraw is magic and persistent" was
**half right**: persistence matters (verified above), but it lives in the
`CDIB`, not in DirectDraw specifically. The forced-DIBSECTION path reused
the **legacy** `CreateDIBSection(..., DIB_PAL_COLORS, ...)` path
(dib.cpp:281-294), which is wired for 8-bit palettized sections.
**Leading hypothesis** (not fully root-caused — would need re-running the
experiment): at the screen's 32-bit `CColorFormat`, `DIB_PAL_COLORS` +
`biBitCount=32` is contradictory, so the section is mis-created or the
full-window initial paint doesn't land in it the way it lands in the DD
surface.

**Takeaway for the plan:** the exact failure is left open deliberately —
don't root-cause or revive the legacy DIBSECTION path, because we are
**not reusing it**. `DIB_SDL_SURFACE` is a fresh
backing type that meets the persistence contract above. The smoke test
that catches a persistence regression is unchanged: reach gameplay,
screenshot, look for black gaps in the map area (minimap/toolbar keep
working even when the main surface doesn't, so they don't tell you).

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

## Pixel format & transparency (verified 2026-05-24)

Nail this before Stage 0 — getting it wrong means every blit is
color-swapped or transparency breaks.

- **Surface format: 32-bit BGRX.** The runtime depth is the screen depth
  (`CColorFormat::CalcScreenFormat`, blt.cpp:536 → `GetDeviceCaps(BITSPIXEL)`),
  32-bit on modern Windows. In-memory byte order is B,G,R,X. The existing
  bridge already uses the matching SDL masks — copy them verbatim for
  `DIB_SDL_SURFACE`:
  `rmask=0x00FF0000, gmask=0x0000FF00, bmask=0x000000FF, amask=0`
  (RenderingAdapter.cpp:107-110). `SDL_CreateRGBSurfaceWithFormat` with
  `SDL_PIXELFORMAT_RGB888` is the equivalent.
- **No alpha channel.** Surfaces are opaque (`amask=0`). Blending is *not*
  used; transparency is a color-key (below).
- **Transparency = software color-key on index 253.** There is no alpha
  and no per-pixel flag. Both the hot sprite path and the `CDIB::TranBlt`
  family treat a source pixel as transparent iff it equals
  `thePal.GetDeviceColor(253)` — the 32-bit device-color of palette index
  253 ("magenta"). See sprite.cpp:1664-1745 (`case 4` runs at runtime):
  `if (src_dword != magenta_dword) dst = src`.
- **This maps 1:1 to SDL.** `SDL_SetColorKey(surf, SDL_TRUE, key)` +
  `SDL_BlitSurface` reproduces the exact behavior, where `key` is the
  `Uint32` from `thePal.GetDeviceColor(253)`. No `INDEX8` surfaces, no
  manual per-pixel loop needed in the ported path.

## Blit taxonomy — what actually needs porting

"Reroute `CDIB::Blt`" is too vague: there is no single `Blt`. The real
work splits into three distinct categories with different strategies.

1. **Hot path — sprite/terrain inner loops (NOT `CDIB::TranBlt`).**
   sprite.cpp hand-rolls scan-converted blits that write raw pixels
   straight into the window `CDIB` via `GetBits()` / `GetOffset()` /
   `GetPitch()`, with the index-253 software color-key (sprite.cpp:1535-1745).
   These paint the map every frame. **Stage 2 concern:** they keep working
   if the window `CDIB`'s backing becomes an `SDL_Surface` and
   `GetBits()`/pitch still expose lockable contiguous pixels. They do
   *not* call SDL — leave the inner loops, just change what they write
   into. This is why Lock/Unlock matters (they go through `CDIBits`,
   which locks).
2. **CDIB→CDIB composition — `TranBlt` / `StretchTranBlt` / `StretchBlt`.**
   Compose UI art (`_pcDib` source) into an off-screen `CDIB` (`m_pDib`):
   icons.cpp:321-580, bmbutton.cpp:91-525, new_unit.cpp:510-945,
   lastplnt.cpp:2329-2351. **Stage 1 target:** replace with
   `SDL_BlitSurface` (+ colorkey for the `Tran` variants) once both ends
   are `DIB_SDL_SURFACE`.
3. **CDIB→HDC present — `BitBlt(hdc,...)` / `StretchBlt(hdc,...)`.**
   Blit a composed `CDIB` onto a window/control HDC: icons.cpp:614,
   bmbutton.cpp:423/490/529, new_unit.cpp:518+, area.cpp:5403. Many sit on
   paths that are now SDL2 or runtime-dead (e.g. the owner-draw button
   present at bmbutton.cpp:423/490/529 is downstream of
   `CGlobalSubClass::DrawButton`, which the runtime audit showed **never
   fires** — see Risks). **Stage 2/5:** audit each for liveness; the live
   ones (world present) are already replaced by `BlitDIBToSurface`, the
   dead ones are exclude-from-build.

## Proposed phasing

### Stage 0 — Add SDL_Surface as a parallel CDIB backing (no behavior change)

Add a new `DIB_TYPE` value `DIB_SDL_SURFACE`. **Insert it *before*
`DIB_NUM_TYPES` in the enum (blt.h:154-158), not after.** `CalcBltMethod`
clamps the profile value with `__min(DIB_NUM_TYPES, iType)` (blt.cpp:144),
so the sentinel must auto-bump to keep the new type in range; added after
the sentinel, the clamp silently eats it. With it inserted before, the
enum becomes `…DIB_MEMORY, DIB_SDL_SURFACE(=4), DIB_NUM_TYPES(=5)` and the
profile opt-in (Stage 3 note) is `BLT=5` (1-based; `eType = iType-1 = 4`).
In `CDIB`:

- Add `SDL_Surface* m_psdlsurfaceBack = nullptr;`.
- On `Resize()` when `m_eType == DIB_SDL_SURFACE`, create
  `SDL_CreateRGBSurfaceWithFormat(0, m_cx, m_cy, 32, SDL_PIXELFORMAT_RGB888)`
  (BGRX, matching the format section above). Free + recreate on resize;
  free in destructor. Point `m_pBits` at `surface->pixels` and set
  `m_lPitch = surface->pitch` so `GetBits()`/`GetPitch()`/`GetOffset()`
  keep working for the raw-pointer callers (sprite hot path, category 1).
- `Lock()`/`Unlock()` → `SDL_LockSurface`/`SDL_UnlockSurface` for this
  type (the only types needing a real lock are this and `DIB_DIRECTDRAW`;
  the others are GdiFlush no-ops — dib.cpp:436-441).
- Don't route ANY blit through it yet. Keep `CBLTFormat::CalcBltMethod`
  returning `DIB_DIRECTDRAW` for the default path.

Goal: prove the scaffolding compiles and doesn't disturb anything. One
small commit.

### Stage 1 — Add the SDL_BlitSurface path for CDIB→CDIB composition (category 2)

In `CDIB::BitBlt`/`TranBlt`/`StretchBlt`/`StretchTranBlt` (the
`CDIB*`-dest overloads, dib.h:120-123), add an `SDL_BlitSurface` branch
that runs when both source and dest are `DIB_SDL_SURFACE`:
- plain blits → `SDL_BlitSurface`;
- `Tran` variants → `SDL_SetColorKey(src, SDL_TRUE, GetDeviceColor(253))`
  then `SDL_BlitSurface`;
- stretch variants → `SDL_BlitScaled`.

This covers category 2 (UI bitmap composition: icons, buttons, unit
panels). The sprite/terrain hot path (category 1) is **not** here — it
writes raw pixels and is handled by Stage 2's backing swap, not by a
blit reroute.

**Colorkey hygiene (matters once Stage 2 lands).** The colorkey set here
goes on category-2 *source* surfaces only. The window CDIB surface that
Stage 2 introduces gets blitted whole into the panel and must stay
**opaque — never carry a colorkey.** If a key ever leaks onto the window
backing (e.g. a `SetColorKey` on a surface that later becomes the window
backing, or a shared/reused surface), the window→panel blit will start
treating index-253 magenta as transparent and punch holes in the map.
Set the key per-blit on the source, or clear it after; don't leave it
sticky on a surface that doubles as a blit destination.

Still safe — only active when both ends are `DIB_SDL_SURFACE`, which
nothing produces until Stage 3.

### Stage 2 — Back the window CDIB with an SDL_Surface; drop the HDC capture

The meaningful pivot. Today `CAnimAtr::Render` paints into the window
`CDIB` (DD-backed), and `RenderingAdapter::BlitDIBToSurface` copies that
whole `CDIB` into the `SDL2Panel` surface each frame
(RenderingAdapter.cpp:128). Switch the window `CDIB` to `DIB_SDL_SURFACE`
so:
- the category-1 sprite loops write straight into the SDL surface's
  pixels (via the existing `GetBits()` lock — no loop changes);
- `BlitDIBToSurface` can blit `CDIB`'s `SDL_Surface*` directly into the
  panel surface (`SDL_BlitSurface`) instead of wrapping `GetBits()` in a
  throwaway `SDL_CreateRGBSurfaceFrom` every frame.

**`BlitDIBToSurface` is not a single blit — keep all three branches.**
It currently has three paths (RenderingAdapter.cpp:127-135): exact-size
full blit (`nullptr`→`nullptr`), `SDL_BlitScaled` when the panel differs
in size from the DIB, and an offset `SDL_BlitSurface`. "Blit the
`SDL_Surface*` directly instead of wrapping" means swap the *source* (the
real backing surface instead of the throwaway wrap) while **preserving
the size/scale branching** — drop it to a single `SDL_BlitSurface` and
differently-sized panels break.

**Match the panel surface format to the backing (perf).** `SDL_BlitSurface`
does per-pixel format conversion when src/dst differ. Keep the `SDL2Panel`
surface at `SDL_PIXELFORMAT_RGB888` too so the window→panel copy stays a
fast (near-memcpy) path rather than a converting blit every frame.

Add an `SDL_Surface* GetSDLSurface()` accessor on `CDIB` (returns
`m_psdlsurfaceBack`, or `nullptr` for non-SDL types so callers can fall
back). Honor the **persistence contract** above: do not clear the
surface per frame.

**Unresolved thread — what replaces `RectVisible()` if the HDC goes.**
The stage title says "drop the HDC capture," but `CAnimAtr::Render` uses
the `GetDC` HDC for exactly one thing: the `RectVisible()` dirty-rect clip
test (terrain.cpp:584). Nothing in this stage specifies its replacement.
On **Windows** the cheap answer is to *keep* the HDC purely for the clip
test (the backing swap doesn't require dropping it) — so for Stage 2,
prefer keeping it and rename the stage's intent to "stop the HDC being a
*draw* target," which it already isn't. The genuine removal (no HDC at
all) is a **Linux/Stage 5 problem**: `RectVisible` would become a manual
rect-vs-clip intersection. Flagging so it isn't assumed solved here.

### Stage 3 — Flip the default `CBLTFormat::CalcBltMethod` to `DIB_SDL_SURFACE`

Flip the runtime default (blt.cpp:160-171). DD surface still allocates as
a fallback for any code path that hasn't opted in. **This is the first
stage where Stages 1-2 code actually executes** (see testability note).
Verify the game runs and the map has no black gaps.

> **Testability note (stage ordering).** Stages 1-2 add code that only
> activates when surfaces are `DIB_SDL_SURFACE`-typed, and nothing
> produces those until this flip. So Stages 0-2 are verifiable only as
> "still compiles, still runs unchanged on the DD default"; the
> SDL path is first *exercised* here. Two options: (a) land 0-3 as one
> reviewable milestone and treat the flip as the integration test, or
> (b) add a temporary `[Advanced] BLT=N` profile value (CalcBltMethod
> already reads `w22::GetProfileInt("Advanced","BLT",0)`, blt.cpp:142) to
> opt a dev build into `DIB_SDL_SURFACE` early. Recommend (b) — it lets
> you bisect Stage 1 vs Stage 2 regressions instead of debugging both at
> the flip.

### Stage 4 — Remove DirectDraw entirely

- Drop `LoadLibrary("DDRAW")` + `pfnDirectDrawCreate` from blt.cpp.
- Drop `LPDIRECTDRAWSURFACE m_pddsurfaceBack` + `m_ddOffSurfDesc` from CDIB
  and the `DIB_DIRECTDRAW` cases in ctor/Resize/Lock/Unlock/GetDC (dib.cpp).
- Drop `class CDirectDraw`, `ptrtheDirectDraw`.
- Drop `CDIBWnd`'s `LPDIRECTDRAWCLIPPER m_pddclipper` (dibwnd.h) — the DD
  screen clipper. The SDL path doesn't need it (panel/window bounds clip
  via SDL_Rect / `SDL_BlitSurface` dst clipping); confirm no remaining
  caller relies on `m_pddclipper` before removing.
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

Two findings from the 2026-05-24 runtime audit cut work here:
- `subclass.cpp` (`CGlobalSubClass`/`CFramePainter`, 2250 lines of GDI) is
  runtime-dead — exclude from build, don't port (see Risks).
- `CAppPalette::Animate`/cycling is dead — drop, don't port (see Risks).

After this, we can attempt a Linux build (probably as `Phase 7`).

## Risks / known unknowns

These were audited in code and verified at runtime on 2026-05-24 (Release,
under `dbgcatch_rel.ps1`, with temporary `PHASE6-PROBE` `OutputDebugString`
probes since removed). The headline result: **the game does not run at 8-bit
at runtime.** `CColorFormat::CalcScreenFormat` (blt.cpp:536) reads
`GetDeviceCaps(BITSPIXEL)` from the screen — 32-bit on modern Windows. Every
game surface is allocated at that depth (world DIBs at world.cpp:1085-1135;
sprites converted to `ptrthebltformat->GetBitsPerPixel()` at *load* time,
sprtinit.cpp:111/561/777). The 8-bit palette is a **load-time decode**, not a
runtime surface format. This recalibrates several risks below.

- **8-bit palette art — LOW (was flagged high).** The index→32-bit-BGR
  conversion already happens today via `CAppPalette::GetDeviceColor`
  (apppalet.cpp:224) building a 256-entry table that blits look up through.
  By the time SDL sees a surface it's already true-color. **Don't use
  `SDL_PIXELFORMAT_INDEX8`; target 32-bit `SDL_Surface` and keep the
  existing load-time palette decode.** The 8-bit nearest-match search in
  `GetColorValue` only runs when `bpp==8`, which never happens at runtime.
- **Palette animation — DISMISSED (no work).** Three independent signals:
  (1) `CAppPalette::Animate()` → `AnimatePalette` has **zero callers** and is
  dead-stripped from the Release exe by `/OPT:REF`; (2) `SetColors` →
  `SetPaletteEntries` fired **exactly once** at runtime (the load-time
  `count=256` palette init) and never again through live gameplay; (3)
  `Fadein`/`Fadeout` are empty stubs (apppalet.cpp:463-477). There is no
  FLC/FLI animation in the live build (`flcanim` is only in `.mak`/`.bak`,
  not either CMakeLists). **Drop the cycling code, don't port it.**
- **Lock/Unlock semantics — MEDIUM, mechanical.** ~76 `GetBits()`/`CDIBits`
  sites, but they all go through the RAII `CDIBits` wrapper (dib.h:263-278),
  so call sites don't change — only `CDIB`'s internals do. For
  `DIB_MEMORY`/`DIB_DIBSECTION`, `Lock()` is essentially a `GdiFlush()` no-op
  (dib.cpp:436-441); only `DIB_DIRECTDRAW` does a real surface lock
  (dib.cpp:426). So the `SDL_LockSurface` port is contained to the class.
  Note the known-fragile site at world.cpp:1686 (`GetDDSurface null` comment).
  **Caveat:** the Stage 2 `GetSDLSurface()` accessor hands out the raw
  `SDL_Surface*`. Callers that read/modify its `->pixels` directly bypass
  the `CDIBits` lock; keep direct-pixel access going through `GetBits()`
  (which locks) and reserve `GetSDLSurface()` for `SDL_BlitSurface` calls
  (which lock internally).
- **HDC text — LOW for dialogs, MEDIUM for game HUD.** SDL2 dialogs are
  already off GDI (all 18 `SDL2*` files use SDL_ttf; zero `DrawText`/`TextOut`).
  Remaining GDI text is game-world/HUD (area.cpp, toolbar.cpp, icons.cpp,
  credits.cpp) via the `CDC` compat shim (mfc_compat.h:1357-1372 →
  `::DrawTextA`/`::TextOutA`). These draw into the off-screen DIB's HDC and
  ride the pipeline; they survive on Windows as long as `CDIB` keeps an HDC.
  This is Stage 5 work, only forced by the eventual Linux build.
- **`CFramePainter` / `subclass.h` — RUNTIME-DEAD, exclude don't port.**
  `subclass.cpp` is 2250 lines of GDI, and `InitCustomUI` still calls
  `Subclass()`/`SetDrawInfo()` at startup (lastplnt.cpp:1633-1637) — **but**
  `CGlobalSubClass::DrawButton` (the paint path) **never fired** at runtime
  across init, gameplay, and toolbar/unit interaction. It triggers on
  `WM_DRAWITEM` from Win32 owner-draw button *controls*, and those lived on
  the MFC dialogs that are now SDL2. **Treat `subclass.cpp` as
  exclude-from-build in Stage 5, not a porting target** — removes a large
  chunk of GDI work. (Verify `DrawButton` stays dead if any new Win32 dialog
  is ever introduced.)

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
  toolbar updates on hover — same smoke test we've used all session).
  **Use the Release exe for the runtime smoke test:** as of 2026-05-24 the
  Debug build access-violates at startup in `StatBar::SetText`
  (icons.cpp:721) before reaching gameplay — a pre-existing Debug-only bug,
  unrelated to this work. `dbgcatch_rel.ps1` drives the Release exe.
- Persistence check (Stages 2-3): screenshot at gameplay and confirm the
  map area has **no black gaps** — that's the failure signature if the
  SDL surface backing isn't persisting (minimap/toolbar keep working
  regardless, so they won't reveal it).
- For stage 4 (DDraw removal): also verify no `DDRAW.DLL` import in
  the exe (e.g. `dumpbin /imports` or `Dependencies` tool)

## Effort estimate

- Stage 0: 2-4 hours (scaffold `DIB_SDL_SURFACE` alloc/lock + one commit)
- Stage 1: 4-8 hours (CDIB→CDIB `SDL_BlitSurface` + colorkey, category 2)
- Stage 2: 1-2 days (back window CDIB with SDL surface; cut the
  per-frame `SDL_CreateRGBSurfaceFrom` wrap in BlitDIBToSurface)
- Stage 3: 1 day (flip default + bug-fix runtime issues)
- Stage 4: half day (mechanical cleanup once stage 3 stable)
- Stage 5: open-ended (remaining live GDI; subclass.cpp/palette excluded
  per audit, which removes a big slice)

**What the estimate actually buys.** Stages 0-4 (~1 week) get you a
**DirectDraw-free Windows build** — no `DDRAW.DLL` import. That is *not*
the same as Linux-running: SDL2 already owns the window and composite, so
removing DD is a dependency cleanup, not portability. The real Linux
blocker is **Stage 5** (GDI still pervades game-world/HUD text and the
CDIB HDC, plus the `RectVisible` thread from Stage 2), which is correctly
marked open-ended and is the genuine unknown. So:

- ~1 week → DDraw-free Windows build (Stages 0-4).
- Linux is gated on Stage 5 + the non-Windows build setup (CMakeLists for
  non-Windows, SDL2 install patterns, etc.), which is open-ended — don't
  fold it into a "2 weeks to Linux" number.
