# OpenGL Rendering Backend — implementation plan

Status: **Part 1 DONE; Part 2 (the GL backend) not started.** Spec reviewed &
corrected 2026-06-08; **seam audit 2026-06-09 added addenda #7–#11** (window-flag
site, GPU-gate predicates, GLEW dropped for an SDL_GL_GetProcAddress loader). See
the **Review addendum** section directly below before implementing (it overrides
any stale claims in the body, esp. "GLEW is vendored").

**Goal:** add **OpenGL** as a third, *selectable* rendering backend alongside the
existing two, exposed in the main-menu options like the original game's renderer
dropdown:

| Value | Backend | What it is |
|------:|---------|------------|
| `0` | **Software** | the original CPU rasterizer + window-surface present (the always-safe fallback) |
| `1` | **SDL2** | current GPU path — `SDL_Renderer` + `SDL_RenderGeometry`, cached terrain RT + sprite atlas |
| `2` | **OpenGL** | new — raw GL via GLEW: world-space mesh + vertex/fragment shaders |

The win OpenGL unlocks (and why it's worth a backend, not a tweak): a **GPU-side
camera transform** (no CPU vertex rebuild on zoom/rotate/scroll), **shader water**,
and **fog-as-texture** — the three things `SDL_Renderer` structurally can't do
(no shaders, no uniform transform). See the architecture notes in
[gpu-terrain-plan.md](gpu-terrain-plan.md) Appendix A and `project_gpu_terrain_execution`.

> **Scope note:** SDL2 forbids mixing raw GL and `SDL_Renderer` on the **same
> window**. The terrain-bearing window (the detached area panel) also draws the
> sprite atlas and a CPU chrome overlay through `SDL_Renderer`. So "OpenGL terrain"
> is really "**the area-panel window's entire present path becomes GL**": terrain,
> sprites, overlay, and present. This plan scopes that whole window, not just terrain.

---

## Review addendum (2026-06-08) — corrections before handoff

This spec was written 2026-06-05, before the big SDL2-path perf work landed and
before Part 1 shipped. The core (phasing, the per-window present seam, and the
verified Appendix A/B/C math in `gpu-terrain-plan.md`) is **sound and was
re-verified against the code**. The following corrections **override** the body:

1. **Part 1 is COMPLETE** (was "do first"). `RenderBackend.h` (`GetRenderBackend()`,
   `RenderBackendIsGpu()`, `RenderBackendOpenGLAvailable()==false`) exists and is
   routed (`GameWindow.cpp:343`, `SDL2Panel.cpp:893`); the Advanced dialog has the
   `Software / SDL2 (GPU) / OpenGL` radio with OpenGL greyed and the restart-warning
   save (`SDL2Dialogs.cpp:163-186`). **Skip Part 1; start at GL0.**

2. **GLEW is NOT actually vendored** — the body's "GLEW is vendored, no new
   dependency" is **FALSE today**. `tools/sdl2/` has SDL2/mixer/ttf only; the
   `glew-2.2.0/` dir is *referenced* in `enations_latest/src/CMakeLists.txt` (~L209)
   but **physically absent** (the build only survives because nothing includes it
   yet). **GL0 must actually vendor GLEW 2.2.0** (headers + `glew32` lib/dll, or build
   GLEW static) and confirm the CMake link. *(Aside: `tools/sdl2/coexistspike.obj`
   exists — someone spiked GL↔SDL_Renderer coexistence — but no source/notes survive,
   so re-validate coexistence at GL0; don't assume it's proven.)*
   *Update 2026-06-09: resolved by DROPPING GLEW entirely — see #10.*

3. **Shader-source management — DECISION (was unspecified):** embed GLSL as
   `const char*` string literals in the `*GL` translation units; add one
   `GLuint CompileProgram(const char* vs, const char* fs)` helper that logs
   `glGetShaderInfoLog`/`glGetProgramInfoLog` on failure and returns 0 → the panel
   **falls back to SDL2** (never a hard crash). No `.glsl` files / no runtime path I/O
   (matters for the Linux/macOS end-goal). Hot-reload optional, later.

4. **Integer-rounding parity is the #1 GL1 correctness gate (was under-stated).**
   Appendix A's transform uses *integer* `>>1`, `>>m_iZoom`, and `round(z<<3)`. A
   naive float shader (`/2`, `/2^zoom`) diverges sub-pixel, and because **mouse
   picking stays CPU (`WindowToHex`)**, the terrain will then drift from clicks. The
   vertex shader MUST reproduce the integer truncation (use `floor`/`round` to match
   the arithmetic shift, mind sign). Make this an explicit GL1 acceptance gate, tested
   at all 4 dirs × 4 zooms with click-alignment, not just a pixel-diff.

5. **Torus wrap vs "build the world VBO once" — resolve the tension (was glossed).**
   The same hex sits on *either* side of the map seam depending on scroll, so a single
   static world-space VBO can't cover the seam for every camera position. Do **NOT**
   rebuild the VBO on scroll. Instead: keep the static world VBO, and per frame draw it
   **1–4× with a `worldWrapOffset` uniform** (0, ±mapWidthWorld, ±mapHeightWorld) for
   the seam-crossing copies — decided CPU-side from the current view (a few extra draw
   calls, no rebuild). The CPU walk does the screen-space equivalent today via
   `ToNearestHex`; GL moves it to a uniform offset.

6. **GL2 scope is understated** — since this spec, the sprite layer gained a
   persistent static-sprite grid, dirty-rect incremental capture, and combat eye-candy:
   **additive tracers, blend-UNDER drop-shadows, additive impact flashes, rotated
   bullets, and an in-layer rubber-band selection**. GL2 must port these passes with
   correct blend/z ordering (additive vs alpha vs under-sprite), not just "one atlas
   draw call." Bump GL2's estimate to ~4–6 d. *Offset:* GL1 **supersedes** the
   SDL2 incremental-pan/edit-patch terrain machinery (camera moves are free uniforms in
   GL), so terrain gets simpler — that complexity is not re-ported.

7. **(2026-06-09) T1's window-creation claim is wrong — the GL flag goes in `Detach`,
   not `MaybeCreateOwnRenderer`.** The detached window is created in
   `SDL2Panel::Detach` (`winFlags = BORDERLESS|RESIZABLE|SKIP_TASKBAR`, then
   `GameWindow::CreateSDLWindow(...)`); `MaybeCreateOwnRenderer` runs AFTER and
   requires `m_ownWindow` to already exist. `SDL_WINDOW_OPENGL` cannot be added to an
   existing window — so in GL mode `Detach` must call `SDL2GL::SetContextAttributes()`
   and add `SDL_WINDOW_OPENGL` to `winFlags` BEFORE `CreateSDLWindow`.

8. **(2026-06-09) The seam is wider than create/present/destroy — three
   `m_ownRenderer != nullptr` predicates silently mis-route GL mode.**
   `CAnimAtr::UseSplitLayer()` (terrain.cpp → `SDL2Panel::HasOwnRenderer()`),
   `CAnimAtr::IsGpuFull()` (terrain.cpp), and the compositor's render-rate gate
   (`HasGpuTerrain()`, `SDL2Compositor::RenderAll`) all test for an SDL_Renderer. In
   GL mode that's null → the engine CPU-rasters terrain into `m_dibwnd` again AND the
   compositor throttles the area window to the 10 fps secondary-window path. T1 must
   add a backend-agnostic **`HasGpuPresent()`** (SDL_Renderer OR valid GL context) for
   the compositor gate, and **phase the split gates**: GL0 keeps `UseSplitLayer()` /
   `IsGpuFull()` FALSE (CPU composites everything; GL just presents it); T7/GL1 turns
   them on once the GL terrain renderer is live. SDL2-mode semantics stay bit-identical.

9. **(2026-06-09) `MaybeCreateOwnRenderer` has SDL2-only side effects the GL branch
   must skip:** `SDL2Terrain::Load(m_ownRenderer)` and
   `SDL2Sprites::SetRenderer(m_ownRenderer)` (both create SDL_Textures against the
   SDL_Renderer). Their GL equivalents arrive in T7/T8 — calling them in GL mode is
   wrong from day 1.

10. **(2026-06-09) T0 reframed: GL function loading WITHOUT GLEW.** Two reasons:
    (a) the agent environment blocks external downloads (the GLEW fetch was denied),
    so "vendor GLEW" is not agent-executable; (b) dll-linking glew32 makes
    `glew32.dll` a process-LOAD dependency for ALL backends — a missing dll would
    then break Software/SDL2 too, violating the "never break backends 0/1" rule
    (static `glew32s.lib` avoids that but is /MT vs our /MDd Debug CRT — friction).
    **Decision: hand-rolled loader.** `GLLoader.{h,cpp}` declares the ~50 GL 3.3-core
    function pointers the backend uses; `bool GLLoaderInit()` resolves them once via
    `SDL_GL_GetProcAddress` after context creation (SDL ships this; zero new
    dependencies, nothing new to ship, identical code path on Linux/macOS). Link
    `opengl32.lib` only; the stale `glew-2.2.0/include` entry in CMakeLists can be
    dropped. GLEW remains a fallback ONLY if the user manually drops it at
    `tools/sdl2/glew-2.2.0/`.

11. **(2026-06-09) New TUs inherit the PCH.** `target_precompile_headers(stdafx.h)`
    applies target-wide, so every new GL .cpp compiles `stdafx.h` (windows.h etc.)
    first. Harmless with the loader approach — just don't fight the include order,
    and don't `#include <GL/gl.h>` before the loader header.

---

## Part 0 — what already exists (reduces the work)

- ~~**GLEW is vendored**~~ — **STALE/FALSE (see addendum #2): GLEW is referenced in
  CMakeLists but physically absent from `tools/sdl2/`. GL0 must vendor it.**
- **The renderer flag is already an int selector**, not a bool:
  `w22::GetProfileInt("Advanced", "Renderer", 0)`. Read in exactly two places:
  - [GameWindow.cpp:342](../enations_latest/src/GameWindow.cpp#L342) — `m_useRenderer = (… != 0)` (main window).
  - [SDL2Panel.cpp:864](../enations_latest/src/SDL2Panel.cpp#L864) — `MaybeCreateOwnRenderer` (`== 0` → software).
  - Plus the sub-flag `[Advanced] GpuSprites` ([SDL2Sprites.cpp:168](../enations_latest/src/SDL2Sprites.cpp#L168)).
- **The present seam is already abstracted per-window**:
  `SDL2Panel::MaybeCreateOwnRenderer` / `EnsureOwnBack` / `PresentOwn` /
  `RenderDetached` / `DestroyOwnRenderer` ([SDL2Panel.cpp:861-960](../enations_latest/src/SDL2Panel.cpp#L861)).
  A GL backend is a third branch here.
- **The options UI exists**: `SDL2OptionsDialog` → **Advanced** →
  `SDL2AdvOptionsDialog` ([SDL2Options.cpp](../enations_latest/src/SDL2Options.cpp),
  [SDL2Dialogs.cpp:142](../enations_latest/src/SDL2Dialogs.cpp#L142)), with a
  ready-made widget set (`SDL2RadioGroup`, `SDL2Checkbox`) and a
  **"changed → restart required"** warning pattern in `OnOK`
  ([SDL2Dialogs.cpp:165-173](../enations_latest/src/SDL2Dialogs.cpp#L165)).
- **Algorithms are verified and transfer 1:1** to GLSL: the camera/projection math
  (`WorldToWindowHex` corner permutation, [terrain.cpp:1075](../enations_latest/src/terrain.cpp#L1075)),
  the tile bake + sprite atlas, and the shade / feather / fog / water formulas
  (validated against the original this session).

So this is "translate known math into GLSL + manage a GL context," not "design the
renderer from scratch."

---

## Decisions

1. **Three discrete backends, flag-selected, all kept working.** Software stays the
   reference + fallback forever; SDL2 stays the broad-compatibility GPU path; OpenGL
   is the high-fidelity / cross-platform path. Never delete a backend.
2. **Centralize backend selection** behind one accessor (today the `Renderer` int is
   re-read ad hoc in 3 spots). Introduce `enum RenderBackend { Software, SDL2, OpenGL }`
   + `RenderBackend GetRenderBackend()` and route all reads through it. Prevents the
   "binary `!= 0`" assumption from leaking into the 3-way world.
3. **GL lives behind the same per-window seam** (`MaybeCreateOwnRenderer` &
   friends) — no parallel render loop. The panel owns an `SDL_GLContext` instead of
   an `SDL_Renderer` when backend == OpenGL.
4. **GL 3.3 core (or 2.1 + extensions) — decide at GL0** based on target hardware;
   core profile preferred for portability (matches the Linux/macOS end-goal).
5. **Flag-gated, parity-first, bisectable** — same discipline as the SDL2 terrain
   sub-flags: each GL phase behind verification, software/SDL2 untouched.
6. **Restart-to-apply**, not live-swap. Switching backend tears down/rebuilds the
   window's present path; doing that live is fragile. The Advanced dialog already
   warns "restart required" — reuse it.

---

## Part 1 — Backend selection refactor + Options menu  ✅ DONE (2026-06-07)

**Already implemented — skip.** `RenderBackend.h` + the Advanced-dialog renderer
radio (Software/SDL2/OpenGL, OpenGL greyed) both shipped. Kept below for reference.

This part ships independently and is low-risk; it makes the SDL2/Software choice
user-visible and prepares the 3-way switch before any GL exists.

### 1a. Central backend accessor
- Add `enum class RenderBackend { Software = 0, SDL2 = 1, OpenGL = 2 };` and
  `RenderBackend GetRenderBackend();` (small new header, e.g. `RenderBackend.h`, or
  fold into an existing settings header). Implementation reads
  `GetProfileInt("Advanced","Renderer",0)`, clamps to `[0,2]`, and — until GL lands —
  **treats `2` as `1`** (so selecting OpenGL early just runs SDL2, never crashes).
- Replace the raw reads at [GameWindow.cpp:342](../enations_latest/src/GameWindow.cpp#L342)
  and [SDL2Panel.cpp:864](../enations_latest/src/SDL2Panel.cpp#L864) with the accessor.
  Behavior identical for 0/1.

### 1b. Options menu — renderer selector
- In `SDL2AdvOptionsDialog::OnInit` ([SDL2Dialogs.cpp:145](../enations_latest/src/SDL2Dialogs.cpp#L145))
  add a labeled `SDL2RadioGroup` (or a small dropdown) **"Renderer:"** with options
  `Software`, `SDL2`, `OpenGL`, initialized from `GetProfileInt("Advanced","Renderer",0)`.
  Grey out / disable **OpenGL** until the GL backend is in (a `bool kOpenGLAvailable`).
- In `OnOK`, use the existing `check("Advanced","Renderer", sel, 0)` pattern
  ([SDL2Dialogs.cpp:165](../enations_latest/src/SDL2Dialogs.cpp#L165)) so changing it
  sets `bWarn` → the dialog's "restart to apply" message fires.
- Layout: the Advanced dialog is compact (Zoom radio + 3 checkboxes); add a row, bump
  the dialog height if needed.

**Deliverable:** users can pick Software vs SDL2 from the menu today; OpenGL is a
disabled option that becomes live when Part 2 lands. `./build.ps1 -x64` clean, both
existing backends behave exactly as now.

---

## Part 2 — The OpenGL backend (phased)

Each phase: `./build.ps1 -x64` clean, reach gameplay, **user-verified** visual
parity, Software + SDL2 untouched behind the flag.

### GL0 — Context + present plumbing (riskiest, isolated)
- **First: GL function loading via `SDL_GL_GetProcAddress`** (addendum #10 — GLEW
  vendoring is DROPPED: not agent-fetchable, and a glew32.dll link would add a
  load-time dependency to ALL backends). `GLLoader.{h,cpp}` resolves the ~50 GL 3.3
  core functions once after context creation; link `opengl32.lib` only. Add a
  `CompileProgram` shader helper (addendum #3) now so GL1 has it.
- Add an `m_glContext` path to `SDL2Panel`: when `GetRenderBackend()==OpenGL`,
  `Detach` creates the window WITH `SDL_WINDOW_OPENGL` (addendum #7 — the flag can't
  be added later), then `MaybeCreateOwnRenderer` creates an `SDL_GLContext`
  **instead of** `SDL_CreateRenderer` and runs `GLLoaderInit()` once.
- Mirror the present abstraction: a GL `PresentOwn` that, for now, just uploads the
  existing CPU composite (`m_ownBack`) as one GL texture and draws a fullscreen quad,
  then `SDL_GL_SwapWindow`. This is the GL equivalent of T0/T0b — **parity with the
  software composite, zero terrain shaders yet** — so the riskiest plumbing is proven
  before any shader work.
- Handle the lifecycle: `DestroyOwnRenderer` GL branch, window move/resize →
  `glViewport`, the detached-window create/destroy churn, and the fact that **other**
  windows (main menu, dialogs, radar) stay on `SDL_Renderer`/software (per-window is
  fine; just never mix on one window).
- **Risk centred here.** Validate: area window shows, moves, resizes; main
  menu/dialogs/video unaffected; screenshot tools (`-Screen` fallback for GL).

### GL1 — Terrain shader (the payoff)
- Build the terrain mesh **once in world space** (shared hex corners `x,y,z`, indexed
  VBO/IBO). Rebuild only on map load / terrain edit (`g_enTerrainEditGen`), **never on
  camera move**.
- **Vertex shader** = the Appendix A transform: `M(dir)·(x,y)` + altitude-Y term,
  zoom shift, scroll subtract — all uniforms. Zoom/rotate/scroll become uniform
  updates, no CPU rebuild.
- **Fragment shader**: sample the tile texture (per-tile or atlas), apply per-vertex
  Gouraud shade (free), fog scalar, and the edge feather. Bilinear sampling with the
  corner-fill padding from the bake (fixes the SDL2 nearest-only limitation, #7).
- Port the baked tiles to GL textures (reuse the existing PNG set / atlas).
- **Parity gate:** screenshot-diff + click-alignment vs SDL2/Software at all 4 dirs ×
  4 zooms. Mouse picking is unchanged CPU (`WindowToHex`) and must still line up.

### GL2 — Sprites + chrome on GL
- Port the sprite **atlas** quad submission from `SDL_RenderGeometry`
  ([SDL2Sprites.cpp](../enations_latest/src/SDL2Sprites.cpp)) to GL — same atlas, same
  view-space verts + UVs, one draw call; the data model already fits.
- Draw the CPU chrome/overlay (`m_ownBack`) as a textured fullscreen quad on top.
- Keep the **CPU y-sort** (decision unchanged) — GL only draws.

### GL3 — Water + fog in-shader (the SDL2-impossible wins)
- **Water:** flipbook sampled by a `time` uniform (smooth cross-fade of the existing
  8 frames) or a scrolling flow/normal map. Replaces the "re-draw water tiles per
  wave-tick" workaround.
- **Fog:** upload per-hex visibility as a **low-res texture**, sample bilinear over the
  terrain in the fragment shader (smooth gradient free; deletes the per-corner fog
  geometry). Update only changed-visibility texels; skip on a no-change frame.

### GL4 — Parity polish, edge cases, enable the menu option
- Verify: video/menu (main window), dialogs over the map, resize/fullscreen,
  multi-window z-order, save/load, multiplayer (render-only, safe), HiDPI
  (`glViewport` from drawable size, not window size).
- Flip `kOpenGLAvailable = true` so the Advanced dialog's OpenGL option enables.
- Default stays `Software`/`SDL2`; OpenGL opt-in until it's proven on the user's box.

---

## Integration seam (where GL plugs in)

| Concern | Today (SDL2) | OpenGL branch |
|---|---|---|
| Create present | `SDL_CreateRenderer` ([SDL2Panel.cpp:868](../enations_latest/src/SDL2Panel.cpp#L868)) | `SDL_GL_CreateContext` + `GLLoaderInit()` |
| Per-frame compose | `PresentOwn` → terrain RT + atlas + overlay tex | GL draw: terrain VBO + atlas + overlay quad |
| Present | `SDL_RenderPresent` | `SDL_GL_SwapWindow` |
| Teardown | `DestroyOwnRenderer` | destroy GL context/VBOs/textures |
| Terrain | `SDL2Terrain::Render` (CPU mesh → RT) | `SDL2TerrainGL::Render` (shader) |
| Sprites | `SDL2Sprites::Submit` (atlas, RenderGeometry) | `SDL2SpritesGL::Submit` (atlas, GL) |

The dispatch point is `SDL2Panel` (and the area panel's `m_terrainAA` hookup). Keep
the SDL2 classes intact; add parallel `*GL` implementations chosen by
`GetRenderBackend()`. No edits to the sim, save/load, or input.

---

## Agent task breakdown (handoff structure)

**Why this parallelizes (unlike the perf work, which edited shared hot functions):**
the GL backend is almost entirely **NEW files** (`SDL2TerrainGL.*`, `SDL2SpritesGL.*`,
a GL context wrapper, GLSL strings) selected by a *single* dispatch branch. The **only
shared edit is the `SDL2Panel` create/present dispatch** — confine that to ONE lane and
agents never collide. Everything else is file-isolated and worktree-parallel-safe.

**Honest caveat:** GL has a serial "doesn't work until it all integrates" spine
(T1→T7→T8). The feeder tasks (shaders, VBO, tiles, harness) are real parallel work that
*de-risks and front-loads* the integration, but they don't remove the spine. Plan for
the Integrator to absorb integration churn.

### Agent onboarding & shared context (every agent reads this first)

**Build / run / test (this project's loop — don't reinvent):**
- Build: `& 'd:\Enemy Nations\src\build.ps1' -x64 -Quiet` (full path; **x64 Debug only**;
  raw `msbuild` is hook-blocked). `./mfc-status.ps1` must stay `mfc linked: NO`.
- Run for visual verification: launch under the debugger with the harness — the loop,
  gotchas (dbgcatch kills the game on idle/`-Seconds`; `EN_PERF=1` must be in the *same*
  shell as dbgcatch), and the `load-game.ps1`/`screenshot.ps1` usage are documented in
  **[gpu-maxzoom-perf.md](gpu-maxzoom-perf.md) → "How to build / run / measure."** Reuse it.
- **Area-window screenshots are BLACK via PrintWindow** (D3D/GL surface) — use
  `screenshot.ps1 -Screen` with the window visible. This is why T6 (parity harness) uses
  `-Screen`. Same gotcha for the GL window.
- Project rules live in **[CLAUDE.md](../CLAUDE.md)**: x64 Debug, exclude-don't-delete,
  no co-author trailers, stage specific files (the tree has ~50 untracked junk files).

**Line numbers in this doc DRIFT** (it predates much churn) — **grep the symbol, don't
trust `file:line`.** Anchors below are by symbol name for that reason.

**Pin these decisions (resolve Decision #4 now):**
- **GL 3.3 core profile, GLSL `#version 330`.** (Needs ≥3.0 for `sampler2DArray`; 3.3
  core is the portable floor and is what macOS supports.) Before `SDL_CreateWindow(...
  SDL_WINDOW_OPENGL)`: `SDL_GL_SetAttribute` `CONTEXT_MAJOR_VERSION=3`,
  `CONTEXT_MINOR_VERSION=3`, `CONTEXT_PROFILE_MASK=SDL_GL_CONTEXT_PROFILE_CORE`,
  `DOUBLEBUFFER=1`, `DEPTH_SIZE=24`.
- **Contract header = new `enations_latest/src/SDL2GL.h`** (the Integrator creates it in
  T1, structs/uniform-names stubbed; every feeder `#include`s it). Add `SDL2GL.cpp` and
  the feeder `.cpp`s to `enations_latest/src/CMakeLists.txt` (Integrator owns that edit).

**The transform every shader/mesh agent needs (inlined from `gpu-terrain-plan.md`
Appendix A — authoritative; verified against the original this project):**
World coords of a hex = its `CMapLoc` map pixels; a hex spans `MAX_HEX_HT=64` px/axis
(`HEX_HT_PWR=6`). The 4 shared corners are `(x,y) (x+64,y) (x+64,y+64) (x,y+64)`, corner
**Z = `CHex::GetAltDraw()`**. CPU transform (`CAnimAtr::WorldToView`→`ViewToWindow`):
```
dir 0:  vx =  x + y ;  vy = ((-x + y) >> 1)
dir 1:  vx = -x + y ;  vy = ((-x - y) >> 1)
dir 2:  vx = -x - y ;  vy = (( x - y) >> 1)
dir 3:  vx =  x - y ;  vy = (( x + y) >> 1)
vy -= round(z << 3)               // altitude (TERRAIN_HT_SHIFT = 3)
vx >>= m_iZoom ; vy >>= m_iZoom    // zoom 0..3 (ARITHMETIC shift — see addendum #4)
screen = (vx, vy) - m_ptUL         // m_ptUL = CAnimAtr::GetUL()
```
Camera state for the uniforms: `aa.m_iDir`, `aa.m_iZoom`, `aa.GetUL()` (`CAnimAtr`,
`base.h`). **Torus wrap (addendum #5):** add `worldWrapOffset` to **world (x,y) BEFORE
M** — candidates `{0, ±eX*64}`×`{0, ±eY*64}` (`eX=GetSize().cx` hexes). Pick the 1–4
copies whose projected bounds touch the viewport, CPU-side, per frame.

### Per-task "read this first" map (concrete entry points for cold agents)
| Task | Study these (grep the symbol) | Mirror / produce |
|---|---|---|
| **T1** | `SDL2Panel::Detach` (window creation — addendum #7), `MaybeCreateOwnRenderer`, `EnsureOwnBack`, `PresentOwn`, `RenderDetached`, `DestroyOwnRenderer`; members `m_ownRenderer/m_ownWindow/m_ownBack/m_ownBackTex`, `GetTitleBarHeight()`; the GPU gates (addendum #8): `CAnimAtr::UseSplitLayer`/`IsGpuFull` (terrain.cpp), `HasGpuTerrain` use in `SDL2Compositor::RenderAll` | a GL branch that mirrors this structure; `SDL2GL.h` context wrapper; `HasGpuPresent()` |
| **T3** | `theMap` (`CGameMap`, `terrain.h`): `_GetHex`, `Get_eX/eY`, `GetSideShift`; `CHex::GetAltDraw`; base.h:590 (content-space corner note) | `TerrainMeshGL::Build` (xyz+nrm+idx) |
| **T4** | the transform box above; `gpu-terrain-plan.md` Appendices A/C; `TerrainGetShadeIndex` (`sprite.cpp`) for the shade model | vs/fs GLSL strings + a CPU-reference parity test |
| **T5** | `SDL2Terrain::Load` (`SDL2Terrain.cpp` ~:310): PNG dir discovery, `stbi_load`, the `Tile{ tex[zoom] }` struct, filename convention `<type>_<variant>_<stem>_z<n>.png` | same loader → `GL_TEXTURE_2D_ARRAY` |
| **T6** | `screenshot.ps1 -Screen`, `load-game.ps1`; `CAnimAtr::_WindowToHex` (the CPU pick) | `glparity.ps1` pixel-diff + click-align |
| **T7** | T1 seam + T3/T4/T5 outputs; `SDL2Terrain::Render` (the SDL2 version) as the structural reference | `SDL2TerrainGL::Render` |
| **T8** | `SDL2Sprites.cpp`: the `Sprite` struct, `EmitOrder`, `Submit`, the `g_trails`/`g_shadows`/`g_flashes` lists + their blend modes, `ZLess` sort | `SDL2SpritesGL::Submit` |
| **T9** | `SDL2Terrain.cpp` fog block (`s_fogVis`/`s_fogVerts`); `CHex::GetVisibility()`; water (`tile->tex[zoom]` flipbook) | fog visibility texture + water `time` uniform |

### Roles
- **Integrator (lead, 1 agent).** Owns the *only* shared seam — the `SDL2Panel` GL
  branch (`MaybeCreateOwnRenderer`/`PresentOwn`/`DestroyOwnRenderer` 3-way dispatch) —
  and sequences the integration spine. The only agent permitted to edit existing files.
- **Feeder agents (parallel, worktree-isolated).** Each builds one self-contained,
  unit-testable artifact in NEW files against a contract below; the Integrator wires it
  in. They never touch `SDL2Panel`/`terrain.cpp`/existing code.

### Dependency DAG
```
T0 GL loader ──┬─► T1 GL context (GL0, Integrator) ─► T7 GL1 terrain ─┬─► T8 GL2 sprites ─► T10 GL4 polish/enable
               │                                          ▲           └─► T9 GL3 water/fog ─┘
   feeders (parallel, against the T1 contract header):    │
   T2 shader/CompileProgram util ────────────────────────┤
   T3 world-space terrain VBO builder (pure CPU) ─────────┤
   T4 terrain GLSL (vs/fs from Appendix A) ───────────────┤
   T5 tile → GL-texture loader ───────────────────────────┘
   T6 parity harness (screenshot-diff + click-align) ──► gates T7/T8/T9
```

### Tasks
Each: **scope · depends · files · contract · DoD/verify · parallel-safe?**

- **T0 — GL function loader (replaces "vendor GLEW", addendum #10).** New
  `GLLoader.{h,cpp}`: declare the GL 3.3-core entry points the backend uses
  (function-pointer style, `gl*` names so call sites read normally); `bool
  GLLoaderInit()` resolves them once via `SDL_GL_GetProcAddress` (call after context
  creation; returns false on any miss → caller falls back to SDL2). The Integrator
  applies the CMake edit (add TU, link `opengl32.lib`, drop the stale glew include
  dir). NO download, NO new dll. *Depends:* none. *Files:* `GLLoader.{h,cpp}` (new).
  *DoD:* `build.ps1 -x64` clean; Software/SDL2 unaffected (the opengl32 link is inert
  for backends 0/1). *Parallel:* yes. Small.

- **T1 — GL context + present plumbing (GL0). [Integrator]** When
  `GetRenderBackend()==OpenGL`: `Detach` adds `SDL_WINDOW_OPENGL` to `winFlags` after
  `SDL2GL::SetContextAttributes()` (addendum #7 — `MaybeCreateOwnRenderer` is too late,
  the window already exists there); `MaybeCreateOwnRenderer` then creates an
  `SDL_GLContext` + `GLLoaderInit()` instead of `SDL_CreateRenderer`, SKIPPING the
  SDL2-only `SDL2Terrain::Load` / `SDL2Sprites::SetRenderer` side effects (addendum
  #9); a GL `PresentOwn` uploads the CPU composite `m_ownBack` as ONE GL texture →
  fullscreen quad → `SDL_GL_SwapWindow`. Generalize the gates (addendum #8): add
  `HasGpuPresent()` for the compositor's render-rate gate; `UseSplitLayer()` /
  `IsGpuFull()` stay FALSE in GL0 (T7 flips them when GL terrain is live). Lifecycle:
  GL branch in `DestroyOwnRenderer`, move/resize→`glViewport`, detached create/destroy
  churn; **other windows stay SDL_Renderer (never mix on one window)**. *Depends:* T0.
  *Files:* `SDL2Panel.cpp/.h`, `SDL2Compositor.cpp`, `terrain.cpp` (predicates) + new
  `SDL2GL.{h,cpp}`. *Publishes:* the **GL-context contract** (`SDL2GL.h`). *DoD:* area
  window shows/moves/resizes byte-identical to the software composite (no shaders yet);
  menu/dialogs/video/radar unaffected; fallback to SDL2 on context-create failure.
  *Parallel:* no — spine root.

- **T2 — Shader util (feeder).** `GLuint CompileProgram(const char* vs,const char* fs)`
  (logs infolog, returns 0 on fail) + uniform/VBO helpers; assumes only "a context is
  current." *Depends:* T0. *Files:* `GLUtil.{h,cpp}` (new). *DoD:* compiles a passthrough
  program in an offscreen test. *Parallel:* yes.

- **T3 — World-space terrain VBO builder (feeder, pure CPU).** From `theMap`, build
  shared-corner `(x,y,z=GetAltDraw())` verts + indices + per-corner shade normal
  (Appendix C). NO GL. *Depends:* none. *Files:* `TerrainMeshGL.{h,cpp}` (new).
  *Satisfies:* the **terrain-mesh contract**. *DoD:* unit test — counts + spot-check
  corners vs `WorldToView`; deterministic; rebuild only on load/edit gen. *Parallel:* yes.

- **T4 — Terrain GLSL (feeder).** Author vs/fs = Appendix A **with integer-rounding
  parity** (addendum #4) + Gouraud shade (Appendix C) + fog scalar + edge feather +
  `worldWrapOffset` uniform (addendum #5). *Depends:* Appendix A/C. *Files:* GLSL string
  consts. *Satisfies:* the **shader-uniform contract**. *DoD:* compiles via T2; CPU-
  reference parity 0px on sample verts at all 4 dirs×4 zooms. *Parallel:* yes.

- **T5 — Tile → GL texture loader (feeder).** Load baked tiles (existing PNGs/`CDIB`s)
  into a **`GL_TEXTURE_2D_ARRAY`** (GL has arrays; SDL_Renderer didn't — Appendix B),
  tile id → layer+UV. *Depends:* T0. *Files:* `TileTexGL.{h,cpp}` (new). *Satisfies:*
  the **tile-texture contract**. *DoD:* all ~98 tiles load; dump an array screenshot.
  *Parallel:* yes.

- **T6 — Parity harness (feeder, tooling).** Capture the area window (`-Screen` for GL),
  fixed camera (dir/zoom/scroll/seed), pixel-diff the terrain region + click→hex
  alignment, across dir×zoom, vs an SDL2 baseline. *Depends:* none. *Files:*
  `tools/…/glparity.ps1` (new). *DoD:* pass/fail + diff image. *Parallel:* yes. **Gate
  for T7/T9.**

- **T7 — GL1 terrain integrate. [Integrator]** `SDL2TerrainGL::Render` wires T3+T4+T5
  through the T1 context; uniforms M(dir)/altScale/zoomShift/ptUL/worldWrapOffset (draw
  1–4× for the seam). *Depends:* T1,T3,T4,T5,(T6). *Files:* `SDL2TerrainGL.{h,cpp}` (new)
  + `SDL2Panel` dispatch (shared). *DoD:* **T6 gate passes** (pixel+click) all dir×zoom;
  SDL2/Software untouched. *Parallel:* no (spine).

- **T8 — GL2 sprites.** `SDL2SpritesGL::Submit` — atlas quads (same view-space verts/UVs
  + CPU y-sort) PLUS eye-candy (addendum #6: additive tracers, blend-under shadows,
  additive flashes, rotated bullets, rubber-band) with correct blend/z order; chrome
  overlay as a top quad. *Depends:* T1,T7. *Files:* `SDL2SpritesGL.{h,cpp}` (new) +
  dispatch. *DoD:* sprite parity vs SDL2 (z-order, blends). *Parallel:* partly (draft vs
  contracts).

- **T9 — GL3 water/fog shaders.** Water flipbook via `time` uniform; fog as a low-res
  visibility texture sampled bilinear (deletes per-corner fog geometry). *Depends:* T7.
  *Files:* `SDL2TerrainGL.*`. *DoD:* parity/improvement; T6 still passes. *Parallel:*
  with T8.

- **T10 — GL4 polish + enable.** Edge cases (dialogs over map, resize/fullscreen, HiDPI
  `glViewport` from *drawable* size, multi-window z-order, save/load, MP render-only);
  flip `RenderBackendOpenGLAvailable()→true`. *Depends:* T7,T8,T9. *Files:* assorted +
  `RenderBackend.h`. *DoD:* full matrix; default stays Software/SDL2. *Parallel:* no.

### Contracts (freeze these in a header during T1, BEFORE feeders start)
- **GL-context wrapper** (T1): `MakeCurrent()`, `SwapWindow()`, `Viewport(w,h)`,
  `IsValid()`; owns `SDL_GLContext`. Feeders assume only "a context is current."
- **Terrain mesh** (T3→T7): `struct TerrainMeshGL { std::vector<float> xyz, nrm;
  std::vector<uint32_t> idx; uint64_t builtGen; };` + `bool Build(const CGameMap&,
  TerrainMeshGL&)`.
- **Shader uniforms** (T4→T7): fixed up front — `uniform mat2 uM; uniform float
  uAltScale; uniform int uZoomShift; uniform ivec2 uPtUL; uniform ivec2 uWrapOffset;
  uniform sampler2DArray uTiles; uniform sampler2D uFog; uniform float uTime;`
- **Tile texture** (T5→T7): `int LayerFor(tileId)` + per-corner UV; one
  `GL_TEXTURE_2D_ARRAY`.

### Coordination rules (so parallel agents don't collide)
1. **Only the Integrator edits existing files** (`SDL2Panel.*`, `terrain.cpp`,
   `RenderBackend.h`, `CMakeLists.txt`). Feeders add NEW files only.
2. **Freeze the contract header in T1 day 1** (structs + uniform names, stubbed). Feeders
   code against it; the Integrator implements the consumer.
3. **Flag discipline unchanged:** default Software/SDL2; GL only under
   `GetRenderBackend()==OpenGL`; any GL failure logs + falls back to SDL2 (never crash).
4. **Each task ends green:** `build.ps1 -x64` clean, `mfc-status.ps1` unchanged,
   Software/SDL2 byte-for-byte unchanged, reach gameplay.
5. **Worktrees:** feeders in isolated worktrees (new files → trivial merges); the
   Integrator merges onto the spine and owns the dispatch-point conflicts.

### Suggested wave plan
- **Wave A (parallel feeders + infra):** T0, T2, T3, T4, T5, T6 — against the T1
  contract header (the one serial prerequisite — write it first).
- **Wave B (Integrator, serial):** T1 → T7 (gate on T6) — the spine + first payoff.
- **Wave C (parallel):** T8, T9 — once T7 is green.
- **Wave D:** T10 — polish + enable the menu option.

---

## Risks / unknowns

- **GL-context ↔ SDL_Renderer coexistence** in a multi-window app (other windows stay
  SDL_Renderer/software). Per-window isolation should hold, but validate early (GL0).
- **First shaders in the codebase** — GLSL authoring/debugging tooling is new here.
- **Pixel/click parity** across dir×zoom must match the CPU picking transform exactly.
- **Driver variance** for raw GL vs the SDL_Renderer abstraction we lean on today.
- **Screenshot/harness**: GL windows need the `-Screen` capture fallback (PrintWindow
  goes black on GL-backed windows — already a known gotcha).
- **macOS deprecates OpenGL** (since 10.14; capped at GL 4.1, could be removed). GL 3.3
  core still runs there today, but long-term the mac path may be SDL_Renderer's Metal
  backend — one more reason Software/SDL2 stay first-class and GL stays opt-in.

## Effort (rough, not commitments)

- **Part 1** (refactor + menu): ✅ **DONE** (0 remaining).
- **GL0**: ~3–5 days (the risky plumbing) **+ T0 loader, small** (addendum #10).
- **GL1**: ~4–6 days (shaders + projection port + integer-rounding parity gate,
  addendum #4 + torus-wrap uniform, addendum #5).
- **GL2**: ~4–6 days (atlas **+ tracers/shadows/flashes/rotation/rubber-band**,
  addendum #6 — was 2–4).
- **GL3**: ~2–4 days.
- **GL4**: ~2–4 days.
- **Total: ~3–4 weeks** focused (Part 1 already done). Front-loaded risk (GL0) + most
  tuning (GL1/GL4).

## Verification (every phase)

- `./build.ps1 -x64` clean; `./mfc-status.ps1` unchanged; reach world generation and
  gameplay without crashing.
- Software (`Renderer=0`) and SDL2 (`Renderer=1`) **byte-for-byte unchanged** until GL
  parity is signed off.
- Parity harness for GL: same camera (dir/zoom/scroll/seed) vs SDL2 + Software;
  pixel-diff the terrain region + click-alignment at each dir/zoom.

## Flag / rollback

- `[Advanced] Renderer` int: `0` Software, `1` SDL2, `2` OpenGL (selectable in the
  Advanced options dialog; OpenGL greyed until GL4). `[Advanced] GpuSprites` sub-flag
  unchanged. Any GL failure (context create, shader compile) **falls back to SDL2**
  with a log line — never a hard crash.

## Relationship to other efforts

- The **other in-progress GPU migration is NOT OpenGL** (confirmed) — so this is
  additive, not a collision, and slots in as a separate selectable backend later.
- The **incremental edit re-bake** (keep the SDL2 vertex list, patch only changed
  hexes) is an independent SDL2-path optimization that can land *before* this and
  reduces the terrain re-bake cost we hit on building/road placement. Recommended as
  the next collision-free task regardless of OpenGL timing.
- Aligns with the project's stated end-goal (CLAUDE.md): cross-platform Linux/macOS —
  raw GL is more portable than `SDL_Renderer`'s default D3D backend.
