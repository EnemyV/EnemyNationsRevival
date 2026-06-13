# OpenGL backend — orchestration runbook (all-Opus fleet)

Companion to [opengl-rendering-backend.md](opengl-rendering-backend.md) (the WHAT).
This is the WHO/WHEN/HOW for executing it with **Opus subagents orchestrated by the
main session**. Prepared 2026-06-09; **not started** — launch only on the user's go.

## Fleet

| Lane | Who | Tasks | When |
|---|---|---|---|
| Orchestrator/Integrator | main session (Opus) | step-0 scaffolding, T1, T7, all reviews/commits/merges, T10 | continuous |
| **F1 “GL plumbing”** | Opus subagent, background | T0 (GLLoader) + T2 (CompileProgram) | Wave A |
| **F2 “math lane”** | Opus subagent, background | T3 (TerrainMeshGL) + T4 (terrain GLSL) | Wave A |
| **F3 “assets + harness”** | Opus subagent, background | T5 (TileTexGL) + T6 (glparity.ps1) | Wave A |
| **C1 “sprites”** | Opus subagent | T8 | Wave C (after T7 green) |
| **C2 “water/fog”** | Opus subagent | T9 | Wave C (after T7 green) |

Bundling rationale: T3+T4 share the same study material (the transform); T0+T2 are
both small GL infra; T5+T6 are the only lanes touching assets/the live game. Three
parallel cold-starts instead of six halves the redundant context acquisition.

## Step 0 — Integrator scaffolding (BEFORE launching any feeder)

The one serial prerequisite. On “go”, the orchestrator:

1. Writes `enations_latest/src/SDL2GL.h` — the frozen contract header, exactly the
   contracts in the plan doc (“Contracts” section): `SetContextAttributes()`,
   `Context` wrapper, `CompileProgram`, `TerrainMeshGL` struct + `BuildTerrainMesh`,
   `kTerrainVS`/`kTerrainFS` externs + frozen uniform names + frozen attribute
   locations, `TileTexGL` class.
2. Creates compiling no-op stubs for every feeder TU so they’re pre-registered in
   the build: `GLLoader.{h,cpp}`, `GLUtil.cpp`, `TerrainMeshGL.{h,cpp}` (header may
   fold into SDL2GL.h), `TerrainShadersGL.cpp`, `TileTexGL.{h,cpp}`.
3. CMakeLists edit (Integrator-owned): add the TUs, link `opengl32.lib`, drop the
   stale `glew-2.2.0/include` dir.
4. `build.ps1 -x64` green + `mfc-status.ps1` unchanged → commit
   (“GL0 scaffolding: contract header, stub TUs, opengl32 link”).
5. Launch F1–F3 in parallel (background), then start T1 in the main session against
   the stub loader (its failure path = the SDL2 fallback, so T1 plumbing can land
   before F1 delivers).

## Coordination protocol (encoded in every prompt)

- **Main tree, NEW/assigned files only.** No worktrees: feeders only touch their
  pre-registered files, so collisions are impossible by construction, and they get
  real single-file compiles (`build.ps1 -x64 -File <tu>.cpp`) without a fresh
  cmake configure.
- **No commits by agents.** Orchestrator reviews each diff and commits per lane
  immediately (the user commits from a parallel terminal with `git add -A`; landing
  agent work uncommitted invites bundling — commit fast after review).
- **Build lock.** Advisory lockfile `d:\tmp\enbuild.lock` (create with task id;
  delete after; if present wait ~30 s and retry). Covers agent↔agent contention;
  the second human/Claude session is uncovered — on a weird build failure, retry
  once before investigating.
- **Game launches serialized.** In Wave A only F3 may run the game (baseline
  captures for T6), x64 Debug under `dbgcatch.ps1`, per CLAUDE.md. Everyone else:
  compile-only verification.
- **Stop-and-report rule.** If a feeder believes an EXISTING file must change, it
  stops and reports — only the Integrator edits existing files.
- **Report format** (every agent): what was built; verification commands + actual
  results; contract deviations (expected: none); discoveries the Integrator needs.

## Gates / abort criteria

- Per-lane gate: single-file compile green, `mfc-status.ps1` unchanged,
  `git status` shows only the lane’s assigned files touched.
- **GL0 gate** (T1, orchestrator): GL window shows the CPU composite, moves,
  resizes; menu/dialogs/video/radar unaffected; context-create failure falls back
  to SDL2 cleanly. If GL↔SDL_Renderer coexistence fails here (plan risk #1) →
  HALT all waves, report to user; do not burn Wave C.
- **T6 gate before T7 sign-off**: pixel-diff + click-align at 4 dirs × 4 zooms.
- Backends 0/1 byte-identical at every commit.

---

## Launch prompts (verbatim, ready to fire)

### Common preamble (prepended to every feeder prompt)

```
You are a feeder agent building the OpenGL rendering backend for Enemy Nations,
working in d:\Enemy Nations\src (git repo, branch sdl2-port — do NOT switch branches).

MANDATORY reads, in order, before any code:
1. d:\Enemy Nations\src\plans\opengl-rendering-backend.md — the whole doc. The
   "Review addendum" items #1-#11 OVERRIDE the body. Then re-read the "Agent
   onboarding & shared context" section and your row of the per-task map.
2. d:\Enemy Nations\src\CLAUDE.md — project rules (x64 Debug only, build wrapper,
   no msbuild, staging discipline).
3. d:\Enemy Nations\src\enations_latest\src\SDL2GL.h — the FROZEN contract header.
   Code against it exactly. Do not rename or re-sign anything it declares; if a
   contract seems wrong, STOP and report instead of changing it.

HARD RULES:
- Edit ONLY your assigned files (listed below). They already exist as compiling
  stubs and are registered in CMakeLists. Do NOT edit CMakeLists.txt, SDL2Panel.*,
  terrain.cpp, or any other existing file — if you think one must change, stop and
  report why.
- No git commits, no git add. The orchestrator reviews and commits.
- Build check: & 'd:\Enemy Nations\src\build.ps1' -x64 -File <your>.cpp   (single
  file; full -x64 build only if a header change forces it). BEFORE any build,
  acquire the advisory lock: if d:\tmp\enbuild.lock exists, wait 30s and retry;
  else create it (write your task id), build, then delete it.
- Do not launch the game unless your task section explicitly allows it.
- Line numbers in all docs DRIFT (two sessions edit this tree) — grep symbols.
- Something broken outside your lane: report it, don't fix it.

FINAL REPORT: what you built; verification commands + their actual output;
deviations from the contract (should be none); discoveries the Integrator must know.
```

### F1 — T0 GLLoader + T2 CompileProgram

```
<common preamble>

YOUR TASKS: T0 and T2 (plan doc "Tasks" section; addendum #10 defines T0).
YOUR FILES: enations_latest/src/GLLoader.h, GLLoader.cpp, GLUtil.cpp.

T0 — GLLoader: function-pointer declarations for the GL 3.3-core entry points this
backend needs, resolved once via SDL_GL_GetProcAddress in bool GLLoaderInit()
(false if ANY required symbol fails to resolve; log which via SDL_Log +
OutputDebugString). Use the real gl* names so call sites read normally; remember
GL 1.1 symbols (glViewport/glClear/glTexImage2D/...) are exported directly by
opengl32.lib — only declare pointers for post-1.1 entry points; include <gl/GL.h>
(after windows.h — the PCH provides it) for the 1.1 set and the GL typedefs.
Function set to cover (drive it from what T4/T7/T8/T9 in the plan actually need):
shaders/programs (CreateShader/ShaderSource/CompileShader/GetShaderiv/
GetShaderInfoLog/CreateProgram/AttachShader/LinkProgram/GetProgramiv/
GetProgramInfoLog/UseProgram/DeleteShader/DeleteProgram), uniforms
(GetUniformLocation/Uniform{1i,1f,2i,2f,Matrix2fv}/ActiveTexture), buffers+VAO
(GenBuffers/BindBuffer/BufferData/BufferSubData/DeleteBuffers/GenVertexArrays/
BindVertexArray/DeleteVertexArrays/EnableVertexAttribArray/VertexAttribPointer/
VertexAttribIPointer), textures beyond 1.1 (TexImage3D/TexSubImage3D/
GenerateMipmap — GL_TEXTURE_2D_ARRAY needs TexImage3D), draw/state
(DrawElements is 1.1; BlendFuncSeparate, BlendEquation), plus GenFramebuffers/
BindFramebuffer/FramebufferTexture2D/CheckFramebufferStatus/DeleteFramebuffers
(GL3 fog/water may render-to-texture). Define needed post-1.1 GLenum constants
(GL_FRAGMENT_SHADER, GL_TEXTURE_2D_ARRAY, GL_ARRAY_BUFFER, ...) guarded with
#ifndef. NO GLEW, NO downloads, NO new libs.

T2 — GLUtil.cpp: implement SDL2GL::CompileProgram per the contract (compile vs+fs,
link, log infologs on failure, return 0; never throw/crash). Pure GL-via-GLLoader.

VERIFY: single-file compile of all three TUs green. You cannot create a GL context
in this task — that's T1 — so runtime verification is out of scope; compensate
with care on signatures (compare against Khronos docs from memory) and note any
uncertainty in your report.
```

### F2 — T3 TerrainMeshGL + T4 terrain GLSL

```
<common preamble>

YOUR TASKS: T3 and T4 (plan doc "Tasks" section; addenda #4 and #5 are YOUR
correctness core).
YOUR FILES: enations_latest/src/TerrainMeshGL.cpp, TerrainShadersGL.cpp (and
TerrainMeshGL.h only if the stub set includes it).

STUDY FIRST (grep, don't trust line numbers): CAnimAtr::WorldToView and
WorldToViewContent in terrain.cpp (~:2335 — re-verified 2026-06-09: the 4-dir
integer formulas, Fixed fixY = z << TERRAIN_HT_SHIFT; iNewY -= fixY.Round();
>> m_iZoom); theMap/CGameMap accessors (_GetHex, Get_eX/Get_eY, GetSideShift) in
terrain.h; CHex::GetAltDraw; TerrainShadeBrightness in SDL2Terrain.cpp (the shade
curve you must mirror); plans/gpu-terrain-plan.md Appendices A and C.

T3 — BuildTerrainMesh per the SDL2GL.h contract: shared-corner world-space verts
(x,y in map px, hex = 64 px/axis; z = corner GetAltDraw), 1-float Gouraud shade
per vert (average the adjacent per-triangle brightnesses; non-shaded types
exactly 1.0), indexed triangles matching the CPU triangle split. Torus note:
build the FLAT world once (no duplicated seam geometry) — wrapping is the
uWrapOffset uniform at draw time (addendum #5), not your problem beyond not
breaking shared corners at the map edge (edge corners do NOT wrap-share).
Include: bool TerrainMeshGL_SelfTest(const CGameMap&, const CAnimAtr&) — for N
sample hexes at all 4 dirs, recompute each corner via WorldToViewContent and
assert your vert + CPU math agree exactly; log pass/fail counts. The Integrator
wires it at GL0 runtime; you only need it to compile.

T4 — kTerrainVS/kTerrainFS GLSL (#version 330 core) string constants per the
frozen uniform/attribute contract in SDL2GL.h. THE #1 GATE IS INTEGER PARITY
(addendum #4): the CPU uses arithmetic shifts (>>1, >>zoom) and fixed-point
round for the altitude term; your vertex shader must reproduce the exact same
integers (use floor()-based idioms that match >> on NEGATIVE values; document
each idiom in a comment). Fragment shader: sample uTiles (sampler2DArray) with
the per-vertex aUV, multiply the Gouraud shade, leave uFog/uTime hooks inert
but compiling (GL3 fills them). Also provide, in TerrainShadersGL.cpp, a CPU
reference of the vertex transform: CPoint GlVertexRef(int x,int y,int z,int dir,
int zoom, CPoint ul[, wrap]) — bit-identical to what the GLSL computes — plus
bool TerrainShader_SelfTest() comparing GlVertexRef against the WorldToView
formulas for a grid of sample points (all 4 dirs × 4 zooms, including negative
view coords). If the self-test can't reach exactness for some input class,
REPORT IT — that's a finding, not a failure to hide.

VERIFY: single-file compiles green; self-tests compile; report any spot where
GLSL exactness is in doubt (the T6 harness will arbitrate at runtime).
```

### F3 — T5 TileTexGL + T6 glparity.ps1

```
<common preamble>

YOUR TASKS: T5 and T6 (plan doc "Tasks" section).
YOUR FILES: enations_latest/src/TileTexGL.cpp (+TileTexGL.h if stubbed separately),
d:\Enemy Nations\src\glparity.ps1 (NEW — lives next to screenshot.ps1/click.ps1,
not under tools/, matching the existing harness scripts).

T5 — TileTexGL per the SDL2GL.h contract. Mirror SDL2Terrain::Load (grep it in
SDL2Terrain.cpp): same PNG dir discovery, same stbi_load usage, same
<type>_<variant>_<stem>_z<n>.png convention and MakeKey(type,variant,stem)
string keying — but into one GL_TEXTURE_2D_ARRAY per zoom (TexImage3D via
GLLoader; tiles differ in size, so pad layers to the per-zoom max dims and
record per-layer UV scale for UVScale()). No SDL_Renderer anywhere. Runtime GL
verification is impossible until T1/T7 — structure Load() so the file-reading/
layout logic is separable from the GL upload, and self-check the loader half
(file count vs SDL2Terrain's, key set equality) in a
bool TileTexGL_SelfTestFiles() the Integrator can call. Compile-verify the rest.

T6 — glparity.ps1: the parity harness that will gate T7/T9. Reuse
harness-common.ps1 (window resolver), screenshot.ps1 -Screen (GL/D3D windows are
BLACK via PrintWindow — addendum/onboarding), load-game.ps1, keys/click.
Behavior: -Baseline (capture SDL2-backend terrain shots at all 4 dirs × 4 zooms
at a FIXED camera: load save via load-game.ps1, set dir/zoom via keys, save
PNGs + a manifest of camera state) and -Compare (same captures on the current
backend, per-pixel diff vs baseline within the terrain viewport rect, emit
pass/fail per dir×zoom + a diff image to d:\tmp\glparity\), plus -ClickAlign
(click N known hex centers, read the game's hex-under-cursor feedback, assert
alignment — study CAnimAtr::_WindowToHex and pick an observable: the status/
title readout or a probe log line; document which you chose). YOU MAY RUN THE
GAME for this task ONLY: x64 Debug, ALWAYS under dbgcatch.ps1 (long -Seconds;
it kills the debuggee on timeout), one instance at a time. Validate -Baseline
end-to-end against the CURRENT SDL2 backend before reporting done; -Compare
self-test = compare SDL2 against its own baseline (must pass 100%).
```

### C1 — T8 sprites (DRAFT — refine after T7 lands; reality will have moved)

```
<common preamble + a fresh "state of the spine" paragraph written post-T7>
YOUR TASK: T8. YOUR FILES: enations_latest/src/SDL2SpritesGL.{h,cpp} (new).
Port SDL2Sprites::Submit semantics to GL per addendum #6: atlas quads with CPU
y-sort preserved, then the eye-candy passes in the SAME order/blends as the SDL2
path (study Submit + g_trails/g_shadows/g_flashes + blend-mode setup in
SDL2Sprites.cpp): blend-UNDER drop-shadows, additive tracers, additive flashes,
rotated bullet quads, in-layer rubber-band. Chrome overlay quad on top. The
Integrator owns the SDL2Panel dispatch — you produce the module + a written
integration note (exact call sequence + GL state you assume/restore).
```

### C2 — T9 water/fog (DRAFT — refine after T7 lands)

```
<common preamble + post-T7 state paragraph>
YOUR TASK: T9. YOUR FILES: agreed with the Integrator at launch (T9 extends
SDL2TerrainGL.* — by Wave C the Integrator will either hand you those files for
the water/fog functions or have you deliver TerrainFxGL.cpp; decided then).
Water: flipbook via uTime (cross-fade the 8 existing frames). Fog: per-hex
CHex::GetVisibility → low-res texture, bilinear-sampled in the terrain FS
(replaces per-corner fog geometry); update only changed texels. T6 harness must
still pass after both.
```

---

## Orchestrator checklist at “go”

1. Step 0 scaffolding (above) → build green → commit.
2. Fire F1, F2, F3 (`Agent`, `subagent_type: general-purpose`, `model: opus`,
   `run_in_background: true`).
3. Start T1 in the main session (Detach flag + context + GL PresentOwn +
   `HasGpuPresent()` + predicate phasing per addenda #7–#9) against the stub loader.
4. As each feeder reports: review diff → lane gate → commit. Wire F1’s loader into
   T1; run F2’s self-tests at GL0 runtime; hold F3’s harness for the T7 gate.
5. GL0 gate (manual + screenshots via -Screen). HALT here if coexistence fails.
6. T7 (integrate mesh+shaders+tiles; uniforms; 1–4× wrap draws) → run glparity
   -Compare → iterate until 4×4 green + click-align.
7. Refine + fire C1/C2. Integrate, T6 re-gate.
8. T10 polish matrix → flip `RenderBackendOpenGLAvailable()` → user sign-off.
