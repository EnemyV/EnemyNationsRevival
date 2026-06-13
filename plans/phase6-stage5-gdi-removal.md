# Phase 6 Stage 5 — Finish the GDI removal

Status: **planning** — no code changes yet. Created 2026-06-08.

Goal: remove the remaining Win32 **GDI** dependency from the live render/UI
path so the binary stops importing `gdi32.dll` and the engine compiles on
non-Windows. This is the last engine-render blocker before a Linux/macOS
build (Stages 0–4, DirectDraw removal, already landed).

Prereqs done: DirectDraw removed (CDIB backed by `SDL_Surface`),
DirectSound removed, and the render loop's only per-frame GDI call
(`RectVisible`) replaced with integer bounds math (terrain.cpp).

## The big picture — and the load-bearing complication: `DIB_MEMORY`

> **What's verified vs. assumed (be honest):** items marked ✔ were read in
> source this session. The **live-vs-dead runtime split is NOT statically
> knowable** — that is precisely what Stage 5a instruments. Treat any
> "live"/"dead" label below as a hypothesis until 5a confirms it.

The Phase 6 plan split blitting into three categories. Refined status:

1. **Text — DUAL-PATHED, but only the WORLD surface is actually off GDI.**
   `CDC::TextOut`/`DrawText`/`GetTextExtent` try the SDL_ttf helpers first and
   fall back to `::TextOutA`/`::DrawTextA`/`::GetTextExtentPoint32A` when the
   HDC has no registered SDL surface (mfc_compat.h:1535-1576 ✔). The
   `DIB_SDL_SURFACE` world surface takes the SDL_ttf path. **But the legacy
   UI panels draw text onto `DIB_MEMORY` surfaces, whose `GetDC` returns a
   real GDI HDC with no SDL surface → `Wind22_HdcHasSdlSurface` is false →
   `::DrawTextA` (GDI) fires.** So "text is mostly ported" is true for the
   HUD/world only; legacy-UI text is still GDI.
2. **CDIB→CDIB composition — SDL *only when both ends are `DIB_SDL_SURFACE`*.**
   `BitBlt`/`StretchBlt`/`TranBlt`/`StretchTranBlt` with a `CDIB*` dest use
   `SDL_BlitSurface`/`SDL_BlitScaled`/`SDL_SetColorKey` **iff** both surfaces
   are `DIB_SDL_SURFACE` (dib.cpp:930, 1007, 1149, 1350 ✔). UI art surfaces
   are `DIB_MEMORY`, so those blits fall through to the **software pixel
   loops** — portable (no GDI in the loop itself), but not SDL_BlitSurface.
3. **CDIB→HDC present + raw CDC drawing + the `DIB_MEMORY` HDC machinery —
   STILL GDI.** This is the remaining work.

### The crux: `DIB_MEMORY` is live and GDI-bearing (verified ✔)
~25 live `new CDIB(…, DIB_MEMORY, …)` sites across in-build files
(`SDL2Compositor.cpp`, `SDL2MainMenu.cpp`, `SDL2GameDialogs.cpp`, `icons.cpp`,
`bmbutton.cpp`, `unit_wnd.cpp`, `lastplnt.cpp`, `cutscene.cpp`, `racedata.cpp`,
`main.cpp`). It is the **UI-composition surface type**, and it carries GDI:
its ctor calls `CreateCompatibleDC(NULL)` (dib.cpp:87 ✔), and its `GetDC` does
`CreateCompatibleBitmap`+`SetDIBits`+`SelectObject` (dib.cpp:1501-1524 ✔).
**So `DIB_MEMORY` must be KEPT — the Stage-5 work is to strip GDI *out of* it**
(make it a plain buffer, route its text to SDL_ttf and its present to SDL),
or convert the UI surfaces that draw text/present to `DIB_SDL_SURFACE`.

Note: the `SDL2*.cpp` `DIB_MEMORY` surfaces are pure bitmap-art sources
(blitted via the software loops); their *text* is drawn by the SDL2 layer in
SDL_ttf directly, not via `CDC::DrawText`, so they don't hit the GDI text
fallback. The fallback-firing sites are the **legacy** panels (`icons.cpp`,
`bmbutton.cpp`, `unit_wnd.cpp`, `lastplnt.cpp`) — *if* live (a 5a output).

So Stage 5 is **not "port 100 BitBlt sites"**, but it's also **not "text is
basically done"**: the real spine is **decoupling `DIB_MEMORY` from GDI**,
plus the live shape/line/fill draws, the dead HDC-present paths, the screen-
format probe, and finally severing the CDC/CWnd shim behind `_WIN32`.

## Remaining GDI surface (inventory, verified 2026-06-08)

### A. The `CDC` compat shim (mfc_compat.h) — the game-side GDI funnel
Every game draw call goes through `CDC`. Method routing today:
- **Text** (`TextOut`/`DrawText`/`GetTextExtent`): SDL_ttf + GDI fallback. ✔ mostly ported.
- **Color/mode** (`SetTextColor`/`SetBkColor`/`SetBkMode`): mirror to SDL state **and** call GDI.
- **Blit** (`BitBlt`): raw `::BitBlt` — no SDL path.
- **Shapes** (`Rectangle`/`RoundRect`/`MoveTo`/`LineTo`/`FillRect`/`FillRgn`/`FillSolidRect`): raw GDI, no SDL path.
- **Palette/objects** (`SelectObject`/`SelectPalette`/`RealizePalette`): raw GDI.
- **Caps/misc** (`GetDeviceCaps`/`GetNearestColor`/`RectVisible`): raw GDI.
- Window DCs `CClientDC`/`CPaintDC`/`CWindowDC` wrap `::GetDC`/`::BeginPaint`/`::GetWindowDC`.

### B. wind22 library core
- **dib.cpp** — CDIB HDC machinery: `CreateCompatibleDC`, `CreateDIBSection`,
  `SelectObject`, `SetDIBits`/`GetDIBits`, `GetDC`/`ReleaseDC`. Splits two
  ways: the `DIB_DIBSECTION`/`DIB_WING` branches are **dead at runtime**
  (never produced), but the `DIB_MEMORY` branch is **live and GDI-bearing**
  (ctor `CreateCompatibleDC`, dib.cpp:87; `GetDC` GDI-bitmap dance,
  dib.cpp:1501-1524). The `SDL_SURFACE` branch creates no HDC
  (`m_hDCDib==NULL`). Also the CDIB→HDC present overloads
  `BitBlt(HDC,…)`/`StretchBlt(HDC,…)` (dib.cpp:592/654).
- **apppalet.cpp** — palette GDI (`CreatePalette`/`SelectPalette`/
  `RealizePalette`/`SetDIBColorTable`). For `SDL_SURFACE`, `thePal.Paint`/
  `EndPaint` run only when `m_hDCDib!=NULL`, so this is **vestigial at
  runtime**. Colors are baked to true-color at load via
  `GetDeviceColor`. `CAppPalette::Animate` cycling is dead (audited).
- **blt.cpp** — `GetDeviceCaps(BITSPIXEL)` in `CColorFormat::CalcScreenFormat`
  (the screen-depth probe). Needs an SDL/display equivalent or a fixed 32.
- **msg_box.cpp**, **wndbase.cpp**, **wndstub.cpp** — small GDI bits in the
  window/message-box base; audit for liveness (most superseded by SDL dialogs).

### C. Game-side raw draws (the live work)
~38 `pdc->BitBlt`/`Rectangle`/`LineTo`/`FillRect`/`StretchBlt`/`TranBlt`
sites. Excluding the **non-built** files (`ipccomm.cpp`, `ipcsend.cpp` — not
in CMakeLists), the live-candidate spread is: `unit_wnd.cpp` (10),
`icons.cpp` (7), `world.cpp` (7), `new_unit.cpp` (3), `bmbutton.cpp` (2),
`lastplnt.cpp` (2), `cutscene.cpp` (1). Liveness to be confirmed in 5a.

### D. Already-audited dead (exclude, never port)
- `subclass.cpp` (2250 lines owner-draw GDI) — `DrawButton` never fires.
- `CAppPalette::Animate` / palette cycling.
- The `DIB_DIBSECTION`/`DIB_WING` surface types (not produced at runtime).

## Staged plan

### Stage 5a — Liveness audit (keystone, no behavior change)
Instrument every remaining GDI draw/fallback site with a one-shot
`OutputDebugString` probe (or an atomic counter), the same method that
de-risked the 2026-05-24 DDraw/subclass audit:
- the `::DrawTextA`/`::TextOutA`/`::GetTextExtentPoint32A` fallbacks in CDC;
- `CDC::BitBlt`, `CDC::Rectangle`/`RoundRect`/`LineTo`/`MoveTo`/`FillRect`/`FillSolidRect`/`FillRgn`;
- `CDIB::BitBlt(HDC,…)` / `StretchBlt(HDC,…)` (the present overloads);
- `apppalet` `SelectPalette`/`RealizePalette`/`SetDIBColorTable`;
- the `DIB_MEMORY` CDIB ctor/GetDC path.

Then drive the game through **full gameplay + every dialog/HUD/toolbar/unit
panel/credits/cutscene** via the harness and record which fire. Output:
the definitive live-vs-dead split. **Do not port anything blind — this
stage decides what 5b–5e actually touch.** (Use the Release exe per the
audit caveat; or the x64 Debug under dbgcatch.)

Risk: low (probes only). Effort: ~half day instrument + 1–2 hours driving.

### Stage 5b — Finish the text path; delete the GDI text fallback
For every CDIB that draws text (per 5a), ensure it is `DIB_SDL_SURFACE` so
`CDIB::GetDC` registers it (`Wind22_RegisterDibForHdc`) and the SDL_ttf
helpers handle the draw. Then **remove the `::TextOutA`/`::DrawTextA`/
`::GetTextExtentPoint32A` fallback** from `CDC` (replace with a logged
assert during bring-up, delete once clean). Verify all HUD + dialog text
still renders (screenshot-diff a few text-heavy panels).

Risk: medium — any text path that wasn't SDL-backed goes blank when the
fallback is removed; 5a is what surfaces those first. Effort: 1–2 days.

### Stage 5c — Port the live CDC shape/line/fill draws to SDL
For the shape sites that fired in 5a (selection boxes, HUD rects/lines,
solid fills), route `CDC` drawing into the target CDIB's SDL surface the
same way text does: register HDC→CDIB, and when SDL-backed draw via
`SDL_FillRect` (rects/`FillSolidRect`/`FillRect`), a software/`SDL_RenderDrawLine`
line for `LineTo`/`MoveTo`, and a rect outline for `Rectangle`. Honor the
index-253 color-key for any transparent fills. Dead shape sites: exclude.

Risk: medium — SDL has no styled pens; check any dashed/patterned draws
(e.g. selection marquee) and reproduce the pattern manually. Most engine
draws are 1px solid. Effort: 1–2 days.

### Stage 5d — CDIB→HDC present + window DCs
- Confirm the **live** world/panel present is fully on
  `RenderingAdapter`→`SDL` (it is — `BlitDIBToSurface`). 
- Exclude/stub the **dead** `CDIB::BitBlt(HDC,…)`/`StretchBlt(HDC,…)` present
  paths (owner-draw buttons, etc. — per 5a).
- Audit `CClientDC`/`CPaintDC`/`CWindowDC` + `CWnd` GDI usage; stub the dead
  legacy `WM_PAINT`/`OnPaint` handlers (SDL owns the windows now).

Risk: low-medium (mostly exclude-not-port). Effort: ~1 day.

### Stage 5e — Decouple `DIB_MEMORY` from GDI; drop the truly-dead branches
`DIB_MEMORY` is **live (verified) and must be kept** — it's the UI-composition
surface. The work is to remove the GDI it carries, which is only reachable
once 5b (text→SDL_ttf) and 5d (present→SDL) remove the reasons it holds an HDC:
- Drop `CreateCompatibleDC(NULL)` from the `DIB_MEMORY` ctor branch
  (dib.cpp:87) and the `CreateCompatibleBitmap`/`SetDIBits`/`SelectObject`
  GDI path in `GetDC`/`ReleaseDC` (dib.cpp:1501-1524). `DIB_MEMORY`'s pixels
  are already a plain `new BYTE[]` buffer (dib.cpp:286) — the HDC exists only
  to service GDI text/present on the buffer, which 5b/5d replace.
- **Remove** the genuinely-dead `DIB_DIBSECTION`/`DIB_WING` branches in
  ctor/Resize/GetDC/Lock/SyncPalette (these types are never produced at
  runtime — `CalcBltMethod` yields only `SDL_SURFACE`, and explicit
  constructions use `SDL_SURFACE` or `DIB_MEMORY`).
- Drop the apppalet palette-realization GDI (vestigial under `SDL_SURFACE`;
  `thePal.Paint`/`EndPaint` run only when `m_hDCDib!=NULL`) and exclude
  `CAppPalette::Animate`. Keep `GetDeviceColor` (GDI-free ✔; the SDL blit path
  calls it).
- Replace the screen-format probe in `blt.cpp::CalcScreenFormat` —
  `GetDC(NULL)` + `GetDeviceCaps(PLANES/BITSPIXEL/NUMCOLORS)` (blt.cpp:377-392)
  — with a fixed 32-bit, or `SDL_GetDesktopDisplayMode` format query. (Also
  audit the apppalet `GetDeviceCaps(NUMCOLORS)` at init, apppalet.cpp:47.)
- Exclude `subclass.cpp` from the build.

Risk: low (dead/vestigial). Effort: ~1 day.

### Stage 5f — Sever the CDC/CWnd shim from GDI behind `_WIN32`
After 5b–5e remove the live callers, the raw-GDI bodies in `CDC`/`CWnd`/
`CClientDC`/`CPaintDC` (`::BitBlt`/`::Rectangle`/`::SelectObject`/`::GetDC`/
`::BeginPaint`/…) are unused on the SDL path. Guard them behind `_WIN32`
(or replace the bodies with the SDL routing from 5b–5c) so non-Windows
compiles. **This is the boundary that lets `gdi32` drop** and dovetails with
Phase 7's `platform/` seam — do this as part of, or just before, that seam.

Risk: medium — this is where "I thought it was dead" bites; gated by a clean
5a audit. Effort: folds into Phase 7 item 1.

## Verification (per stage)
- `./build.ps1 -x64` Debug + Release clean.
- `./mfc-status.ps1` still `mfc linked: NO`.
- Reach gameplay and exercise **every** dialog/HUD/toolbar/unit-panel/credits
  via the harness; screenshot-diff text/box/line fidelity against a baseline.
- **End gate:** `dumpbin /imports enations.exe` shows **no `GDI32.DLL`** (and
  no `USER32` GDI-adjacent draw imports). That is the Stage-5 done signal.

## Effort estimate
- 5a audit: ~1 day (the keystone)
- 5b text finish: 2–4 days — bigger than it looks: the live legacy-UI text
  draws onto `DIB_MEMORY` buffers that have no SDL surface, so SDL_ttf needs
  a path there (wrap the buffer in an `SDL_CreateRGBSurfaceFrom` for the text
  render, or convert those panels to `DIB_SDL_SURFACE`). Gated by 5a's list
  of which panels are actually live.
- 5c shapes/lines: 1–2 days
- 5d present + window DCs: ~1 day
- 5e decouple `DIB_MEMORY` + dead branches + screen-format: 2–3 days
- 5f shim seam: folds into Phase 7

**~1.5–2 weeks** of focused work to a **GDI-free Windows build**, modulo
whatever 5a surfaces (the `DIB_MEMORY` text path is the main reason this isn't
the ~1 week I first estimated). That removes the last engine-render Win32
dependency; the residual non-render Win32 (threads, files, settings,
windowing) is Phase 7 item 1.

## What this is NOT
- Not a renderer rewrite — the software blitter + SDL_ttf already draw the
  game. This is dependency removal, one category at a time.
- Not blind porting — 5a's liveness audit drives every later stage. The big
  risk is treating dead code as live (wasted work) or live as dead (blank
  text / missing HUD); instrumentation is the mitigation.
