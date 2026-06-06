# GPU Sprites / Buildings — plan

Status: **starting (2026-06-05).** Follow-on to the GPU terrain path
([gpu-terrain-plan.md](gpu-terrain-plan.md)), which declared native-GPU sprite
rendering as the real CPU/scaling win but left it out of scope. Terrain is now on
the GPU (renders to a cached render-target, composited per the contract below).
This doc plans replacing the **per-frame CPU sprite raster** with cached textures +
quads.

**Goal:** stop re-rasterizing every visible unit/building/tree/effect into a CPU DIB
each frame. Instead cache each sprite frame as an `SDL_Texture` once, and each frame
draw textured quads through the area-panel's `SDL_Renderer` — the same renderer the
GPU terrain already uses — in the existing y-sorted order, on top of the terrain.

**Why:** profiling (`project_pathing_perf_and_render_bound`,
`project_render_sim_decouple_plan`) showed the zoomed-out frame is **render/present
bound** — specifically the software sprite composite (`CGameMap::UpdateRect` draws
every visible sprite into a DIB) → ~6 fps zoomed out. Terrain is already cached, so
the remaining big lever is the sprite layer.

---

## Where we are (verified 2026-06-05)

### The compositing seam already exists (terrain T1, done)
- `CAnimAtr` splits the world paint into **terrain** (`m_dibwnd`) and a **color-keyed
  sprite layer** (`m_dibSprite`) whenever the panel presents through its own GPU
  renderer — `CAnimAtr::UseSplitLayer()` /
  [terrain.cpp:720](../enations_latest/src/terrain.cpp#L720).
- `CAnimAtr::GetSpriteLayerSurface()`
  ([terrain.cpp:727](../enations_latest/src/terrain.cpp#L727)) hands the magenta-keyed
  sprite DIB's `SDL_Surface` to the compositor.
- Today [SDL2Panel.cpp:1025-1032](../enations_latest/src/SDL2Panel.cpp#L1025) blits
  that one big sprite surface over the GPU terrain (`SDL_SetColorKey` magenta →
  `SDL_BlitSurface`). **This single blit is what GPU sprites replace** — with per-sprite
  textured quads rendered directly into `m_ownRenderer`.

### The interception point: the draw-list
- `CGameMap::UpdateRect` walks visible hexes and, in `bDraw` mode, accumulates every
  sprite into a **`CTileDraw`** list as **`CTileDrawInfo`** entries via
  `CDrawInfoPool::Get*DrawInfo(...)` — buildings (BACKGROUND/FOREGROUND layers),
  foundations, bridges, vehicles, trees, projectiles/explosions
  ([terrain.cpp:3540-3812](../enations_latest/src/terrain.cpp#L3540)).
- The list is flushed **y-sorted, back-to-front** via `tiledraw.DrawRow()` per row and
  `tiledraw.Flush()` at the end
  ([terrain.cpp:3815-3828](../enations_latest/src/terrain.cpp#L3815)); in split mode
  `xpdibwnd` is pointed at `m_dibSprite` around those calls so sprites raster into the
  sprite layer.
- **This y-sorted draw-list is the GPU interception point.** Each `CTileDrawInfo` knows
  its `CSpriteView`, map position, layer, and dir/zoom — enough to emit a quad.

### Sprite pixel model
- A `CSprite` holds `CSpriteView`s (per dir / damage / stage / anim);
  [sprite.h:441](../enations_latest/src/sprite.h#L441). A view draws **base DIBs +
  overlay DIBs + animation DIBs** in order
  ([sprite.cpp:2464-2499](../enations_latest/src/sprite.cpp#L2464)).
- Pixels live in `CSpriteDIB`, stored **pre-decompressed at the runtime color depth**
  (`m_iBytesPerPixel`, per-zoom `m_adiblayoutinfo`) with magenta/index-253 color-key
  transparency. So a view → a fixed-size pixel rect per zoom we can upload to a texture.
- **TEAM/PLAYER COLOR — RESOLVED 2026-06-05: not applied in the main view.** Player
  color `CPlayer::GetPalColor()` is used **only** for minimap pixels
  ([world.cpp:1384/1871/1950](../enations_latest/src/world.cpp#L1384)). `ColorConvert`
  ([sprite.cpp:2811](../enations_latest/src/sprite.cpp#L2811)) is a plain 24bpp→depth
  palette convert with no player param, and `StructureDrawToDIB` just memcpys pixels.
  So main-view building/vehicle sprites draw straight from their (shared, non-per-
  player) pixel data. **Consequence:** a pixel-level capture is exactly faithful to the
  CPU output, and the per-`CSpriteDIB*` texture cache is safe (a DIB always yields the
  same pixels). Decision #3's per-player variants are **not needed**.

---

## Decisions (proposed — confirm before S1)

1. **Renderer: reuse the panel's `SDL_Renderer` + `SDL_RenderCopyEx`/`RenderGeometry`.**
   Same library as GPU terrain; no new dependency. Quads, not a shader pipeline.
2. **Texture cache keyed by `CSpriteDIB*` (or view+layer+zoom).** Upload lazily on first
   use: `CSpriteDIB` pixels → `SDL_Surface` (color-key → alpha) →
   `SDL_CreateTextureFromSurface`, cached. ~hundreds–few thousand small textures; VRAM
   trivial. Invalidate on asset reload (mirror `s_loadGen`).
3. **Team color (pick one at S0 once the mechanism is known):**
   - **A. Per-player baked textures** — simplest to draw, ×N players memory, recolor at
     bake.
   - **B. Base texture + overlay mask texture, tinted via vertex color** — one base set
     + one mask set; tint the overlay quad by the player color at draw. *Recommended* if
     color is an overlay DIB (matches the engine's base+overlay split exactly).
   - **C. Keep team-colored sprites on CPU, GPU-draw the rest** — hybrid, ugly seam.
4. **Keep the y-sort on the CPU; GPU only draws.** Build the quad list in the exact order
   `DrawRow`/`Flush` would draw, then submit. No GPU depth sort → ordering identical to
   today, so no regressions in occlusion.
5. **Fog dim / shading via per-quad vertex color modulation** (the sprite layer's
   per-pixel dim becomes a quad tint). Match the CPU dim factor.
6. **Flag-gated sub-path** under the existing `[Advanced] Renderer`, e.g. a new
   `[Advanced] GpuSprites` int, so the CPU sprite layer (current blit) stays the
   fallback until parity holds. Bisectable like the terrain sub-flags.

---

## Phasing

- **S0 — Recon + scaffolding (no behavior change).** Pin down: exact `CSpriteDIB` pixel
  format at true-color, the color-key value, the **team-color mechanism**, and the
  screen-rect math a `CTileDrawInfo` implies (position/anchor/hotspot →
  `SpriteViewToWindow`). Stand up an empty `SDL2Sprites` module + the `GpuSprites` flag.
  Decide decision #3. **Deliverable: this section filled in + flag wired, software path
  unchanged.**
- **S1 — Texture cache + one element end-to-end. ✅ DONE 2026-06-05.** Trees render on
  the GPU, no flicker (static or scrolling), positions/transparency correct (user-
  verified). Implementation notes (the two non-obvious bits):
  - **Capture** at the lowest blit (`CSpriteDIB::StructureDrawToDIB`), gated by two
    globals set in terrain.cpp (`g_enSpriteSplitPass` = in the split GPU pass;
    `g_enCurTileIsTree` = current tile is a tree, set in `CStructureDrawInfo::Draw`).
    Pixels decoded by `CSpriteDIB::DecodeToRGBA` (RLE skip/run → ARGB, transparent in
    the skips), cached per `CSpriteDIB*`+zoom.
  - **VIEW-SPACE list, re-projected each present** (NOT a content-coords RT). The world
    scrolls by blit-shifting `m_dibwnd` + repainting only a strip, and the terrain RT
    pans every present — so a content-coords cache flickers. Storing each tree at
    `screen+UL` (scroll-invariant) and re-projecting to the current UL in `Submit`
    makes trees track the panning terrain. Flag: `[Advanced] GpuSprites`. Module:
    [SDL2Sprites.cpp](../enations_latest/src/SDL2Sprites.cpp).
  - Gotcha for later stages: this capture/cache/list design generalizes to any sprite
    whose pixels come from `StructureDrawToDIB`; **vehicles use `VehicleDraw`** (a
    different path) and need their own hook at S4.
- **S2 — Whole sprite layer on GPU (revised 2026-06-05).** Buildings can't go GPU alone:
  the CPU sprite overlay composites *after* the GPU sprites, so foundations (drawn to
  m_dibSprite outside the tiledraw flush) and units that should be occluded by a building
  would wrongly appear on top — a visible z-seam. So the correct next step moves the
  ENTIRE sprite layer to GPU as one y-ordered list (also where the perf win lands —
  zero CPU sprite raster). Design, now fully de-risked:
  - **Capture every sprite, not just trees:** broaden the gate to all `StructureDrawToDIB`
    sprites (buildings/bridges/trees/effects/foundations) + add a hook in
    `VehicleDraw` ([sprite.cpp:1493](../enations_latest/src/sprite.cpp#L1493)).
  - **Vehicles are an affine-warped quad** (4 vertices, rotated/sheared) → submit via
    `SDL_RenderGeometry` (4 verts + texcoords), like terrain. Structures stay axis-
    aligned `SDL_RenderCopy`.
  - **Per-present y-sort:** store each captured sprite's view-space position/verts PLUS
    its sort key (the engine's projected `m_ptCenter`, scroll-invariant), then sort the
    list each present so list order = z-order regardless of partial-repaint capture
    order. (A flat capture-order list — fine for sparse trees — would mis-order dense
    buildings/units across dirty-rect boundaries.)
  - When all sprites divert to GPU, the CPU `m_dibSprite` stays empty (its overlay blit
    is a transparent no-op) — no removal needed; flag-off restores the CPU path.
- **S3 — Frame-level skip** (terrain-plan decision): if nothing moved/animated, re-present
  without rebuilding the list.
- *(Team color / per-player variants: not needed — see §"Where we are".)*

### S2 status (2026-06-05): renders correctly, but DRAW-CALL BOUND — needs batching
Implemented and **visually correct** (user-verified static render; z-order fixed via a
capture-sequence tiebreak so a sprite's base/overlay/animation/bg-fg layers, which share
the m_ptCenter sort key, stay stable instead of flickering). A composite render-target
cache (`g_rt`) re-presents the layer as a single blit and only rebuilds when the set
changed or the view panned.

**But it is ~10–20× slower than the software sprite composite** (user). Diagnostic root
cause: the scene animates continuously (smoke/flags invalidate every frame) → the set is
"dirty" every repaint → the RT rebuilds **~33×/sec doing ~900 individual draw calls**
each. In a **Debug build with SDL's default D3D9 backend, per-draw-call overhead
dominates** — the software path does one big DIB blit, so per-sprite GPU draws lose badly
until the draw calls are **batched**.

**The real fix (next focused effort): batch the sprite draws.** Options, best first:
1. **Texture atlas** — pack all sprite frames into one (few) large texture(s); draw the
   whole layer with **one `SDL_RenderGeometry`** (all quads, per-sprite UVs into the
   atlas). Turns ~900 draws into ~1. This is the decisive win and the right architecture.
2. **Incremental RT update** — only clear+redraw the dirty rect's sprites into `g_rt`
   each repaint (like the CPU m_dibSprite), instead of a full rebuild on any change. Cuts
   the per-rebuild cost; pairs with a scroll-shift of the RT. More moving parts.
3. Try a faster SDL render backend (D3D11/opengl via `SDL_HINT_RENDER_DRIVER`) — may cut
   per-call overhead a lot, but affects the shared terrain renderer; test carefully.

**Current state:** code is flag-gated under `[Advanced] GpuSprites`. With it **OFF**, the
fast software-sprite + GPU-terrain path (the prior good state) is unchanged. Recommend
leaving it OFF until the atlas batching lands. Module: [SDL2Sprites.cpp](../enations_latest/src/SDL2Sprites.cpp).

Each stage: `./build.ps1 -x64` clean, reach gameplay, user-verified visual parity, keep
the CPU sprite-layer fallback behind the flag until the stage's parity holds.

---

## Risks / unknowns

- **Team color** (S0 blocker for S3 — see Decisions #3).
- **Draw-order fidelity:** the CPU y-sort within/across rows must be reproduced exactly
  or units occlude wrong. Mitigated by keeping the sort on CPU (decision #4).
- **Screen-position math:** quads must use the *same* `CAnimAtr` transform as the CPU
  raster (and as GPU terrain / mouse-picking) or sprites drift from terrain/clicks.
- **Texture count / churn:** many views × zooms; cache must be bounded/stable. Lazy
  upload + keep-alive should suffice (art is static once loaded).
- **Animations** mutate the chosen view per frame — the cache is keyed per *frame DIB*,
  so animation just selects a different cached texture; no per-frame upload.

## Process agreement (carried over)
- One stage at a time; **user verifies the visual** — I do not screenshot-spin to
  self-verify. Use text diagnostics for facts; ask the user for a screenshot only when
  explicitly needed.
- Build target **x64 Debug** (`./build.ps1 -x64`); game runs from `d:\Enemy Nations\`.
