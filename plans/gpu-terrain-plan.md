# GPU Terrain Path — plan

Status: **planning, ready to start** — no code yet. Created 2026-05-31.
Single canonical plan (consolidates the former `gpu-terrain-path.md` scoping
doc and `gpu-terrain-implementation.md`).

**Goal:** render the **terrain layer** on the GPU at full fidelity — fixing the
four degradations the 1996 software rasterizer applies to the (already 24-bit)
art — while everything else (units, buildings, effects, HUD, dialogs) keeps its
existing CPU rasterizer and is composited on top as a GPU texture.

**Scope:** terrain only. **Native-GPU sprite rendering is the declared
follow-on phase** — out of scope *here*, but it's the real CPU/scaling win for
huge worlds + many players (raster each sprite frame to a cached texture once,
then draw textured quads — no per-frame CPU raster), and it's what makes the
"no dirty rects" choice correct (below). See the compositing contract for the
seam.

**Why:** the terrain art is already full 24-bit, but the 1996 software
rasterizer degrades it (the four degradations below). This is a **different
effort** than phase 6 (`phase6-sdl-rendering-port.md`), which moves the CPU
present off DirectDraw onto `SDL_BlitSurface` (still CPU). This introduces a
**GPU** present path. See "Relationship to phase 6."

**For whoever picks this up:** codebase conventions and build/run tooling are in
`src/CLAUDE.md`; porting context in `src/MFC_TO_SDL_PORT_GUIDE.md`. Builds use
`./build.ps1` (never raw msbuild). The "why" detail and the verification of
every claim here is also captured in the project-memory note
`project_terrain_render_fidelity`.

---

## The four degradations we're fixing

All in `CSpriteDIB::TerrainDrawQuad` (sprite.cpp ~585-1100), verified 2026-05-31:

1. **Checkerboard edge feathering** — type transitions are 50/50 pixel-
   interleaved (`(byOffset ^ iLeft) & 1`, stride `+=2`), not alpha-blended →
   dithered noise at boundaries. **→ GPU: alpha splat.**
2. **Nearest-neighbor resampling** — vertical `iV = fixV>>16` truncated,
   horizontal raw `memcpy`. **→ GPU: bilinear.**
3. **8-level baked flat shading** — `TerrainGetShadeIndex` picks 1 of 8 pre-
   shaded tile copies per triangle. **→ GPU: smooth per-vertex Gouraud.**
4. **Tile repeat at low effective resolution** — 128×64 tile stretched per hex.
   **→ GPU: bilinear + per-zoom LOD; same texels, far better sampling.**

Art is full 24-bit and intact — none of this needs art changes.

> **Bake finding (2026-06-02): terrain is single-layer opaque.** All 530 TGA
> masters bake to **fully opaque** 128×64 diamonds (0 transparent texels after
> `Square`). Roads / coastline / river / resources are **full-tile type
> replacements** the engine swaps into `CHex::m_psprite` — *not* transparent
> overlay strips. So the terrain layer draws **exactly one opaque tile per hex,
> single pass**: the "overlays as a 2nd alpha pass" idea below is dropped, and the
> `transparent` colorkey concern belongs only to the sprite layer (T1), not
> terrain. Tiles are still baked RGBA (uniform loader; T5 splat may add per-vertex
> alpha). The only inter-tile blending is the optional edge feather (degradation
> #1 → T5).

---

## Decisions (locked 2026-05-31)

1. **Library: `SDL_Renderer` + `SDL_RenderGeometry`** (no custom shaders). Gets
   bilinear, per-vertex Gouraud (vertex colors), and multi-pass alpha
   transitions — 3-4 of the 4 fixes — with least risk and free cross-platform.
   Raw OpenGL (GLEW is vendored) stays a later option **only** if single-pass
   splatting / per-pixel lighting is wanted; the mesh + tiles + camera work is
   shared, so it's not a throwaway choice.
2. **No dirty rects — full redraw, with one cheap CPU optimization.** Terrain:
   none (GPU draws the visible field cheaply; scroll invalidates all anyway).
   Sprite layer: clear to the color-key and re-rasterize all *visible* sprites
   each frame. Removes the phase-6 "persistence contract" bug class wholesale.
   - **The one optimization: a frame-level skip** — a single "did anything
     change this frame (move/scroll/animate)?" flag; if not, don't repaint, just
     re-present. A static screen costs ~zero CPU. One boolean, not bookkeeping.
   - **No per-rect dirty tracking.** Deliberately rejected: on a transparent
     sprite layer it's bug-prone (stale pixels, clear-old-rect, overlap repaint),
     its win erodes as more sprites animate, and the case it would help (many
     units, high motion) is solved better and more simply by the **GPU-sprite
     follow-on** (cached sprite textures + quads → no per-frame CPU raster at
     all). So the CPU-frugality path is *offload* + *frame-skip*, **not**
     rect bookkeeping. ("No dirty rects" is correct **because** GPU sprites are
     the scaling path; if sprites were to stay CPU-rasterized indefinitely, dirty
     rects would be worth reconsidering.) The `UpdateRect` machinery still exists
     if a profile ever forces the issue.
3. **Present path: convert the *main game window* to `SDL_Renderer` wholesale.**
   Everything that draws to it becomes textures/geometry. (Per-window — secondary
   own-windows can stay software; see §2-T0.)
4. **Render loop: decoupled sim + VSync'd present.** Sim runs on its own fixed
   ~24 FPS timer (unchanged; preserves multiplayer lockstep); render present is
   independent and VSync'd. The present **must never throttle the sim** (doubly
   important given the known sim-side slowdown from an unbounded AI message queue
   — a separate, confirmed issue; memory: `project_ai_msgqueue_slowdown_confirmed`).
5. **Terrain art: offline TGA bake → PNG folder + manifest** (a new asset set,
   **not** `ENATIONS.DAT`). See "Asset pipeline."
   - **PNG loader: vendor SDL2_image** (not currently vendored — only
     SDL2/ttf/mixer). `IMG_Load → SDL_Surface → SDL_CreateTextureFromSurface`.
   - **Baked PNGs are checked in** (≈98 tiles × ~4 zoom sizes is small/
     versionable); the bake tool regenerates on demand.
   - **Per-zoom LOD, not mipmaps** — SDL_Renderer has no mipmap API, so the bake
     emits pre-scaled sizes (the original's 4 zoom levels) and the renderer binds
     the right size per zoom + samples bilinear.

---

## Pre-T0 prerequisites (do these first)

1. **Implement the decoupled render loop** (decision 4) as part of T0's first
   step — sim timer independent of the VSync'd present.
2. **Vendor SDL2_image** (CMake + dll + headers) — gates the PNG loader (T2).
3. **Build `tools/terrainbake`** (§ Asset pipeline) — gates T2.
4. **Build a deterministic-camera parity harness** (fix dir/zoom/center/seed,
   screenshot) — gates T2 validation; without it "parity" is eyeballed.

---

## ⚠ Present-target finding (2026-06-02) — terrain lives in the area panel's OWN window

Verified in code: the interactive terrain is **not** drawn to the main game
window. `CWndArea` creates `m_aa.m_sdlPanel` and **unconditionally `Detach()`es it**
into its own borderless OS window (area.cpp:2488), then renders the terrain via
`RenderingAdapter::RenderToPanel(&m_aa, m_sdlPanel)` (area.cpp:1831,
terrain.cpp:624). Pixel path: `CAnimAtr::m_dibwnd` → `BlitDIBToSurface` → the
panel's backing `SDL_Surface` → `SDL2Panel::RenderDetached` →
`SDL_GetWindowSurface(m_ownWindow)` + `SDL_UpdateWindowSurface` (SDL2Panel.cpp
872-915) — **software, on the panel's own window.** The main window only carries
toolbar/chrome, the menu, dialogs, video, and *docked* panels.

**Consequence for this plan:** the **T0 work (main game window → `SDL_Renderer`)
is correct and reusable infra for chrome/menu/dialogs/video, but it is NOT the
window the terrain draws to.** The GPU terrain renderer must live on the
**area-map panel's own window**. So:
- T0 (done) stands: `GameWindow::GetPresentSurface()/PresentSurface()`, flag-gated,
  parity-validated for the main window.
- **T0b (new): mirror that present abstraction onto `SDL2Panel`'s detached window**
  — an optional per-panel `SDL_Renderer` + back-buffer, same `GetPresentSurface/
  PresentSurface` shape, gated by the same `Renderer` flag. The area panel opts in.
- T1 sprite-layer split + T2 GPU terrain then happen **against the area panel's
  surface/renderer**, not the main window. The compositing contract below is
  unchanged in substance — only the target surface/window moves from "main window"
  to "area panel window."
- Keeping the area map as its own movable window honors the standing "SDL windows
  must be movable/resizable" requirement, so re-docking into the main window
  (the alternative) is rejected.

## The hard constraint that shapes everything

The window is presented in **software**: `SDL_GetWindowSurface` +
`SDL_UpdateWindowSurface` (SDL2Compositor.cpp:216,249). SDL2 does **not** allow
mixing a software window surface with `SDL_Renderer`/GL on the **same window**.
The constraint is **per-window**.

So the moment terrain renders on the GPU, the **main game window's present path
must become a renderer**, and everything drawing to that window must arrive as a
texture/geometry. (The only alternative — GPU terrain → per-frame readback to a
CPU surface — is a GPU→CPU stall that negates the benefit. Ruled out.) This does
**not** require rewriting the sprite rasterizer — its output is uploaded as a
texture.

---

## Compositing contract (the main integration cost)

**Verified:** terrain is the **bottom layer**. In `CGameMap::UpdateRect`, terrain
(`phex->Draw`) is drawn **immediately** during the row scan, while **every**
sprite — buildings, bridges, vehicles, trees — is accumulated into the `tiledraw`
(`CTileDraw`) list and flushed/y-sorted **after** (terrain.cpp:3496-3681). So
terrain never draws over a sprite; the background-layer split is faithful.

Per-frame GPU composite order:
1. **GPU terrain** (base tiles + overlays + fog).
2. **CPU sprite layer** (units/buildings/effects + in-world HUD) as a
   **color-keyed texture** on top.
3. **Chrome** (toolbar, minimap/area bar, status, dialogs) — separate panels,
   each its own texture.

**The catch:** today the CPU renderer composites sprites *onto* terrain in one
buffer. To layer them over GPU terrain, the sprite pass must render into its
**own `CDIB`, cleared to the index-253 key each frame, with terrain omitted**.
This sprite-layer separation is the biggest non-terrain change and the main
code-risk — bounded (one render target + clear + "skip terrain" in the paint
loop), but it touches `CAnimAtr::Render`/`UpdateRect`. Color-key → alpha 0 maps
1:1 (index 253, the reserved transparency color — phase-6 finding).

---

## Render loop / frame pacing

Per decision 4: **sim on a fixed ~24 FPS timer, present decoupled and VSync'd.**
Today the world repaints via `InvalidateRect`→`OnPaint` at the game cadence; the
renderer's `PRESENT_VSYNC` ties present to the display refresh. Structure the
loop so the sim advances on its own clock and the present cannot block it. Both
run on the main thread (see Threading), so the decoupling is about *timers*, not
threads: don't gate a sim tick on the present completing.

**Frame-level skip (decision 2):** each frame, if nothing changed
(no move/scroll/zoom/rotate/animation), skip the sprite re-raster + terrain
submit and just re-present the last frame. One dirty flag — the whole CPU
render cost goes to ~zero on idle frames, which is the cheap way to keep CPU
free for the sim without per-rect bookkeeping.

---

## Architecture target (per-frame)

```
renderer.Clear()
── World viewport ───────────────────────────────
1. terrainMesh.Draw()       // base tiles: textured RenderGeometry, Gouraud vertex colors
2. terrainOverlays.Draw()   // road/coastline/river/resources: 2nd pass, alpha
3. fog.Draw()               // per-hex visibility darken/black (or folded into vertex color)
4. spriteLayerTex.Draw()    // CPU-rendered units/buildings/effects, colorkey→alpha
5. buildCursor/selection    // build-placement hatch + rubber-band as GPU primitives
── Chrome ───────────────────────────────────────
6. minimap/area, toolbars, dialogs  // panels as textures
renderer.Present()
```

Two GPU resources own terrain: the **mesh** (world-static vertex grid) and the
**per-tile textures**.

**Texturing model — SDL_Renderer constraint.** `SDL_RenderGeometry` binds **one**
`SDL_Texture` per call, with **no array textures** (that's SDL3-GPU / raw GL).
So: **one texture per tile, batched by texture** — sort visible hexes by tile,
issue one `SDL_RenderGeometry` per distinct tile. Only tiles *on screen* draw →
~dozens of calls/frame, trivial. Per-tile textures also avoid bilinear edge-bleed
(no neighbor sampling). It supports indices → use an indexed corner mesh.

---

## Target terrain architecture

### Mesh
- Each hex = a quad (2 triangles) over 4 corners; corners are **shared** between
  adjacent diamonds → a continuous vertex grid (indexed).
- Per-vertex attributes: world `(x, y, altitude)`, tile texcoord, fog scalar,
  and a per-vertex shade color (Gouraud).
- **Camera = transform, not baked positions** (Appendix A). Build vertices in
  world space; scroll/zoom/rotate are uniforms. First cut may compute corner
  screen positions on CPU via the exact `WorldToView`/`ViewToWindow` for
  correctness, then move to a uniform matrix.
- Submit only the **visible** hex window (engine already culls to `*prVp`);
  handle torus wrap (`WrapX/WrapY`) when emitting vertices.

### Texturing
- Per-tile textures from the **offline PNG bake** (decision 5) — the un-shaded
  24-bit tile (shading is per-vertex now), corner-filled (`Square`) so bilinear
  doesn't bleed, with pre-scaled per-zoom sizes. Bind the zoom-appropriate size,
  sample **bilinear** (`SDL_SetTextureScaleMode(LINEAR)`).
- Road/coastline/river/resources are **full opaque tiles** the engine already
  swapped into the hex (bake finding above) — they draw in the **same single
  pass** as any other tile, not a separate alpha overlay. No 2nd pass.

### Shading (replaces 8-level baked)
- Per-vertex normal from corner-altitude deltas (the engine already computes
  these, terrain.cpp:2102-2121). Light **from the right** to match the original
  (`SHADE_CONTRAST=14`, `TerrainGetShadeIndex`). Continuous brightness multiplier
  in ~[0.6, 1.3] (the original `Shade()` range), **Gouraud-interpolated** — smooth,
  no banding. Keep slope-sensitivity + clamp as tunables to match the mood then
  improve. Bake into per-vertex colors on the CPU when camera/terrain changes.

### Type transitions (replaces checkerboard feather)
- **Alpha splat**: each vertex carries blend weight; boundaries interpolate alpha
  across the shared edge. Driven by the engine's per-edge `FEATHER_*` rules in
  `CHex::Draw` (terrain.cpp:2363-2406): no feather for road/city/resources/
  building-cursor/building hexes; coastline FEATHER_IN/OUT special-case; else
  INOUT. **Ship hard edges at T2, add splat at T5.** (Route A does this multi-pass
  per-edge.)

### Fog of war
- Per-hex `CHex::GetVisibility` → per-vertex fog scalar; darken in shader/vertex
  color; unexplored = black (mirrors `x_aiInvisibleShadeIndex`). Units in
  unexplored hexes are already CPU-culled — unchanged.

---

## Phasing

- **T0 — Renderer present path (no terrain).** Stand up `SDL_CreateRenderer` on
  the **main game window** behind a flag (`[Advanced] Renderer=1`); render today's
  composite as one fullscreen texture (parity with software present). Implement
  the decoupled loop here. Riskiest plumbing, isolated.
- **T1 — Sprite-layer separation (still CPU terrain).** Split the world paint into
  terrain (existing CDIB) and a transparent color-keyed sprite CDIB; composite the
  two textures. De-risks the compositing contract before any GPU terrain.
- **T2 — GPU terrain mesh, parity mode.** Bake tool → PNGs; build the vertex grid;
  render the visible window with **nearest + NORMAL_SHADE + hard edges** to *match
  current output*. Composite the T1 sprite layer on top. **Parity (screenshot-diff
  + click alignment) = success.**
- **T3 — Bilinear + per-zoom LOD** → fixes #2, #4.
- **T4 — Gouraud per-vertex shading** → fixes #3.
- **T5 — Alpha splat transitions** → fixes #1.
- **T6 — Fog, overlays (road/coastline/river/resources), build-cursor hatch,
  selection rubber-band.**
- **T7 — Minimap/area view + remaining chrome.** Likely stays CPU→texture (small,
  different projection).

Each phase: `./build.ps1` clean + reach gameplay + screenshot. Keep the software
present behind the flag until T2 parity holds, so regressions bisect. T3-T5 each
behind a sub-flag.

**Rough effort (estimates, not commitments):** prereqs ~1.5 days (vendor
SDL2_image ~0.5d, parity harness ~1d); T0 ~3-5d (the big plumbing); T1 ~2-3d;
T2 ~3-5d (bake tool + mesh + camera + parity); T3 ~1d; T4 ~1-2d; T5 ~2-4d
(feather/splat); T6 ~2-3d; T7 ~1-2d. The **visible terrain win lands at T4**
(~2 weeks in); T5-T7 are polish. Total ~3-4 weeks focused. T0+T1 carry the most
risk; everything after T2 is incremental and flag-gated.

---

## Asset pipeline — `tools/terrainbake`

> **BUILT 2026-06-02.** `tools/terrainbake/terrainbake.py` (pure stdlib — no
> PIL/build step). Baked **530 tiles × 4 zoom sizes = 2120 RGBA PNGs +
> `manifest.json`** → `enations_latest/data/terrain_gpu/`. Faithfully ports
> `Square` + the box `ScaleDownAvg`; handles RLE + raw TGA; keys magenta→alpha
> (none present). Re-run: `python tools/terrainbake/terrainbake.py <TERRAIN_dir>
> <out_dir>`. Manifest entry per tile: `{type,variant,direction,damage, png[z0..z3],
> shade,drawVert,transparent}`.

- **In:** `EnemyNationsRevival/enations/data/TERRAIN/<type>/<variant>/<file>.tga`
  (24-bit masters). Filename convention: `AA/AC/AE/AG`=direction, trailing
  digit=damage (`TerrainSprite::CreateSprite`, tools/sprite/terrain.cpp:808-816).
- **Per tile:** run the original `Square` corner-fill (tools/sprite/terrain.cpp:76)
  so the diamond content fills the 128×64 rect — the padding that stops bilinear
  bleeding magenta at edges. **Skip** `Pack` (runtime layout opt) and `Shade`
  (per-vertex now). Magenta key (255,0,255 + the 254 temp-art kludge) → alpha 0
  defensively — **in practice every master is fully opaque after `Square`**
  (bake finding), so terrain tiles are opaque RGBA.
- **Per-zoom LOD:** emit pre-scaled sizes per tile (128×64 / 64×32 / 32×16 / 16×8,
  the original's 4 zoom levels via `ScaleDownAvg`).
- **Variants:** bake **every engine-referenced variant as its own PNG**, including
  `MakeRotated` road/coastline copies. Do **not** synthesize them by runtime UV
  rotation (non-trivial on 2:1 tiles; interacts with `bDrawVert`). (Map-view
  `m_iDir` rotation is camera geometry, not rotated art — Appendix A / §edge cases.)
- **Out:** a **PNG folder + `manifest.json`** — one PNG per tile×zoom-size (e.g.
  `plain_0_z0.png`); manifest maps `type,variant → {png per zoom}` plus flags
  `{shade, drawVert, transparent}` (the per-type `bShade`, terrain.cpp:2353, and
  `bDrawVert` road/resources). Checked in; regenerable.
- **Load:** at terrain-init, each PNG → `SDL_Surface` → `SDL_CreateTextureFromSurface`
  (via SDL2_image).
- KTX2 was considered and **rejected** (SDL_Renderer can't use its
  compression/mip/transcode features; revisit only if we move to raw GL — keep the
  tool able to emit it then).

---

## Mesh build & lifecycle

- **Visible set:** cull to the world viewport, **extended upward by max altitude**
  — a tall mountain (`alt`≤104, `<<TERRAIN_HT_SHIFT`) raises its screen-Y above its
  hex, so hexes below the viewport can be visible. Margin = `maxAlt<<3 >> zoom`.
- **Shared corners + index buffer** → continuous, seamless mesh.
- **Torus wrap:** emit wrapped world coords for seam-crossing hexes (transform is
  continuous; offset by map size on the wrapped side).
- **Per-vertex attrs from `CHex`:** corner alt (`GetAltDraw`), tile
  (`m_psprite` `GetID`/`GetIndex` → manifest), shade normal (neighbor `GetAltDraw`
  deltas), fog (`GetVisibility`).
- **Rebuild triggers:**
  - **Altitude changes (user-confirmed):** height changes on building placement
    and other situations → **geometry** (vertex Z) rebuild for affected hexes +
    shared-corner neighbors (a corner's Z feeds 4 diamonds and the shade normals).
  - **Road building** (`ChangeToRoad` → road `m_psprite`, terrain.cpp:2588).
  - **Fields** growing around farms (type change).
  - **Mining/minerals (user-confirmed, conditional):** changes the rendered tile
    **only when the resource-view overlay is ON**; off → unchanged. Treat the
    resource overlay as a view-mode that swaps those tiles (toggling it re-skins
    affected hexes).
  - **Fog/visibility:** per-vertex attr as units explore (frequent, cheap).
  - Most touch a few hexes → partial vertex-buffer update.
  - **NOT triggers:** bridges (sprite-layer structures + a `CHex` flag; don't
    change the tile, terrain.cpp:3529-3552); terrain damage (not rendered).
- **On load:** rebuild the whole mesh (new map/altitudes).

---

## Edge cases & integrations

### Present / compositor
- **Video** (`SDL2Video`, draws to game window): per-frame → streaming texture.
  Verify intro + in-game cutscenes.
- **Main menu** (`SDL2MainMenu`, draws to game window): via the renderer.
- **Own-window components** (loading `SDL2CreateStatus`, detached `SDL2Panel`s,
  `SDL2Dialogs`/`SDL2UI`): **separate windows** — may stay software for now (per-
  window constraint). Caveat: `SDL2CreateStatus` has a fallback that draws on the
  **game** window when it has no own-window — if live at gameplay entry it needs
  the renderer; verify which path runs.
- **Dialogs over the map:** panels → textures, composited last; preserve z-order
  and "modal raises to front" (commit `75acfac`).
- **Resize/move/fullscreen:** SDL windows must stay movable/resizable like their
  MFC counterparts (standing user requirement); the renderer viewport must track.
  Use `SDL_GetRendererOutputSize` (≠ window size on HiDPI) for the terrain viewport.
- **World-panel offset:** terrain renders into the world panel, **not** at window
  (0,0); the terrain viewport/scissor must use the panel's rect within the window.
- **Screenshot tools:** `screenshot.ps1` (`PrintWindow`) can return black on a
  GL-backed window → use `-Screen` fallback.

### Sprite layer
- **Colorkey:** index-253 is the reserved transparency color → safe; verify
  nothing draws opaque 253.
- **Building fore/back layers, y-sort, effects, health bars, selection** all live
  in the sprite layer (world-viewport only, **not** chrome) — no sort changes.

### Terrain content
- **Per-type shading:** only shade land types; flat for river/coastline/swamp/
  ocean/resources/lake (`bShade`, terrain.cpp:2353) → manifest flag.
- **`bDrawVert` (road/resources):** `TerrainDrawQuadVert` (sprite.cpp:1105-1266)
  **transposes the scan** (`aptHex2` swaps x↔y, 1157-1170) and iterates column-
  major — net: the tile's **U/V axes are swapped** vs the diamond (source height
  runs along screen-X). Keeps the two triangle halves seamless for roads/resources.
  **GPU: assign road/resource corners the swapped (V,U) texcoord** — a per-tile
  manifest boolean, no separate path.
- **Damage:** not rendered — `CHex::Draw` always uses `GetView(xiDir, 0)`
  (terrain.cpp:2411). Ignore.
- **Variant parity:** pick the **same** variant the engine chose
  (`CHex::m_psprite`); don't re-roll.
- **Overlay auto-tiling** (road facings, coastline types, rivers): the *which-tile*
  choice is already resolved in `CHex` — just draw the chosen tile.
- **Two rotations — don't conflate:** (a) **map-view `m_iDir`** = camera geometry
  (corner reorder via `WorldToWindowHex` xaaiIndex); same tile; the UV→corner
  assignment per `m_iDir` is a **T2 parity detail**. (b) **`MakeRotated` variants**
  = distinct baked PNGs (above).
- **Build-cursor hatch:** was in `TerrainDrawQuad` (`bHatch`/cursor modes) → a GPU
  overlay tint pass over the cursor hexes, wired to `SetBldgCur`/cursor state.
- **No terrain animation:** base terrain is static (trees are sprite-layer effects,
  terrain.cpp:3664); `TERRAIN_TIME` is a load-time default. Animated water would be
  a *new feature*, not parity.

### Mesh / camera
- **All 4 `m_iDir` + 4 zooms:** matrix/shift uniforms (Appendix A); test each.
- **Mouse picking stays CPU and must match the GPU display.** `WindowToHex`/
  `IsHexHit`/`GetHit` use the inverse `CAnimAtr` transform; we *read* `CAnimAtr`
  for the mesh, so picking stays correct **iff the GPU transform reproduces it
  exactly**. Divergence = clicks land off. (Another reason T2 parity matters.)
- **Integer vs float truncation:** original uses `>>`; match for pixel-exact T2
  parity, else accept sub-pixel drift (decide only if the diff/clicks show it).

### Threading — single-threaded rendering (verified)
- The Win32 pump (`CConquerApp::BaseYield`/`Run`, `PeekMessage`→`DispatchMessage`,
  mainloop.cpp:280-331) and the SDL loop (`GameWindow::PollEvents`) share the
  **main thread** (lastplnt.cpp:1651). `WM_PAINT`→`OnPaint`→`UpdateRect` (CPU
  paint/sprite raster) **and** SDL present are both main-thread. AI/sim run on
  worker threads but never render. **No cross-thread GPU hand-off** — keep the
  paint on the main thread.

### Other
- **Scroll:** `CAnimAtr::Scroll` may blit-shift the surface to reuse pixels; that
  optimization retires under full redraw (scroll = update `m_ptUL` only). Verify
  and remove the assumption.
- **`thePal.Paint(dc.m_hDC)`** (area.cpp:1877) is a GDI palette realize — dead in a
  true-color renderer world. Drop, don't port.
- **Save/load & multiplayer:** render-only change; sim untouched; safe.
- **sRGB/gamma:** filter & the 0.6–1.3 shade multiply in **gamma space** (as the
  original) — matches the mood; don't linearize unless deliberately changing it.

---

## Verification

- Per phase: `./build.ps1` Debug+Release clean; `./mfc-status.ps1` unchanged;
  reach gameplay; screenshot. Smoke milestone unchanged: reach world generation
  and render without crashing.
- **T2 parity harness (most important test):** same camera (dir/zoom/scroll/seed)
  on software vs GPU path; pixel-diff the terrain region (tolerance band for
  resampling) **and** click-alignment check at each zoom/rotation.

## Flag / rollback

- `[Advanced] Renderer` profile int gates the GPU present path (mirrors the
  `BLT`/`Zoom`/`ColorDepth` pattern, blt.cpp): `0` legacy software, `1` renderer.
  Keep until T2 parity is signed off, then default it and remove the legacy path
  in a later cleanup. T3-T5 each behind a sub-flag.

---

## Appendix A — Camera / projection math (the T2 parity gate)

Exact transform the GPU mesh must reproduce. Authority: `CAnimAtr::WorldToView`
(terrain.cpp:2210), `GetWorldHex` (2178), `ViewToWindow` (665, verified =
`ptView - m_ptUL`; the torus-unwrap code after it is dead — early return).

A hex → `CMapLoc (x,y)` map pixels; spans `MAX_HEX_HT=64` per axis (`HEX_HT_PWR=6`).
Corners `(x,y) (x+64,y) (x+64,y+64) (x,y+64)`; corner **Z = `GetAltDraw()`**;
corners shared between hexes → seamless grid.

```
dir 0:  vx =  x + y ;  vy = ((-x + y) >> 1)
dir 1:  vx = -x + y ;  vy = ((-x - y) >> 1)
dir 2:  vx = -x - y ;  vy = (( x - y) >> 1)
dir 3:  vx =  x - y ;  vy = (( x + y) >> 1)
vy -= round(z << TERRAIN_HT_SHIFT)     // TERRAIN_HT_SHIFT = 3
vx >>= m_iZoom ; vy >>= m_iZoom        // zoom 0..3
screen = (vx, vy) - m_ptUL             // m_ptUL = WorldToView(center) - (winW/2, winH/2)
```

**GPU decomposition:** screen = affine(x,y) + linear altitude-Y term, then zoom
shift, then scroll subtract. So: vertex buffer = world `(x,y,z)` per shared corner
(rebuild only on altitude change). Uniforms: `M(dir)=[a b; c d]` (sign table, `>>1`
folded into c,d), `altScale=2^3`, `zoomShift`, `ptUL`. Vertex shader:
`v = M·(x,y); v.y -= z*altScale; screen = v/2^zoomShift - ptUL`. Scroll/zoom/rotate
= uniform changes; no mesh rebuild.

## Appendix B — Tile inventory & budget

Masters in `data/TERRAIN`: 14 type dirs (no `forest` — forest = tree effect
sprites; `CHex::num_types=15` counts forest + resources). Variants: PLAIN/DESERT/
HILL/MOUNTAIN/ROUGH 8 each; city 15; coastlne 12; swamp 7; road 5; lake/ocean/
river/resource 4 each; fields 3 → **≈98 base tiles**, 128×64. As RGBA32 ~32 KB
each → a few MB total even ×zoom-sizes. **One `SDL_Texture` per tile, batched by
texture** (SDL_Renderer has no array textures). VRAM is a non-issue; draw-call
count (only on-screen tiles) is trivial.

## Appendix C — Shading model

Original `TerrainGetShadeIndex` (sprite.cpp:378): light from the right,
`SHADE_CONTRAST=14`, 8 levels; `Shade()` bakes lightness × `(1 + 0.1·(shade−4))` ≈
0.6×…1.3×. GPU: per-vertex normal from the same corner-altitude deltas
(terrain.cpp:2102-2121) → continuous brightness → multiplier in ~[0.6,1.3],
Gouraud-interpolated. Slope-sensitivity + clamp are tunables.

---

## Relationship to phase 6

Phase 6 removes DirectDraw and moves the CPU present to `SDL_BlitSurface`. This
effort **supersedes the present path** phase 6 builds but **reuses its findings**
(32-bit BGRX, index-253 colorkey, `CDIB` `GetBits()` lock model). If both proceed:
let phase 6 land DDraw removal + an SDL memory-surface `CDIB` backing (the CPU
sprite layer still needs it), and build the GPU present/terrain on top. The GPU
present is the layer above phase-6's surfaces; they are not in conflict. Phase-6
stages 4-5 (DDraw/GDI removal) remain relevant after this lands.
```
