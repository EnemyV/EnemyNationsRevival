# Incremental terrain re-bake (SDL2 path) — implementation plan

Status: **spec / not started** (2026-06-06). Independent of the OpenGL backend
([opengl-rendering-backend.md](opengl-rendering-backend.md)) and collision-free with
the other in-progress GPU migration.

**Problem.** The SDL2 GPU terrain renders the visible hex mesh once into a cached
render-target texture (`s_rt`) and pan-blits it; it only rebuilds when the signature
changes ([SDL2Terrain.cpp `Render`](../enations_latest/src/SDL2Terrain.cpp), `sig =
zoom ^ dir ^ loadGen ^ g_enTerrainEditGen`). **Any** terrain edit bumps
`g_enTerrainEditGen` → the **entire** mesh re-bakes (2 ms zoomed in … ~225 ms zoomed
out @ ~43k hexes), even though usually only a handful of hexes changed (place a
building, lay a road, toggle resource view, a citizen builds a road, AI grows a
city). The original software path only repainted the changed dirty-rects.

**Goal.** Re-bake **only the changed hexes** into the cached textures in place,
instead of rebuilding the whole mesh — so frequent/continuous edits (AI roads, city
growth) stop costing a full-frame rebuild each.

**Precedent already in the code.** The animated water already does exactly this trick:
on each wave tick it re-draws just the captured open-water tiles **in place into
`s_rt`** (`curWaterTick` path, [SDL2Terrain.cpp](../enations_latest/src/SDL2Terrain.cpp))
— no full rebuild. Incremental edit re-bake generalizes that mechanism to arbitrary
edited hexes.

---

## What changes the rendered terrain (recap)

The renderer reads two things per hex: its **tile** (`m_psprite` → `TileForHex`) and
its **altitude** (corner Z → screen position + occlusion). Edits now funnel through
([wired this session](../enations_latest/src/terrain.inl)):

- `CHex::SetVisibleType` → tile change (city, fields, road via `ChangeToRoad`,
  coastline, bridge revert).
- `CHex::SetAlt` → **geometry** change (bridges, leveling, terraform, network).
- `ResClicked` / road-preview (`ClrRoadIcons`) → direct `m_psprite` swaps.

Two classes with very different difficulty:

| Class | Examples | Effect | Difficulty |
|---|---|---|---|
| **Tile-only** (same altitude) | road build/preview, city/rubble swap, resource-view toggle, fields | repaint the diamond(s) | **Easy** — opaque tile overwrites in place |
| **Altitude** | building leveling, bridges, terraform | corner Z moves → position **and occlusion** change (a raised tile covers tiles behind it; a lowered one reveals them) | **Hard** — needs a bounded region redraw, not just the diamond |

---

## Design

### 1. Track dirty hexes, not just a counter
Replace the bare `++g_enTerrainEditGen` with a small **dirty-hex set** the renderer
can consume:
- A global `std::vector<CHexCoord> g_enTerrainDirty` (or a fixed ring) appended by the
  same setters that bump the gen today. Keep the gen as the "something changed" signal;
  add the list as the "what changed" detail.
- Tag each dirty entry as **tile-only** or **altitude** (the setter knows which —
  `SetAlt` = altitude, `SetVisibleType`/`m_psprite` = tile-only).
- **Cap + fallback:** if the set exceeds N (e.g. 64) in one frame, or contains an
  altitude edit near the camera, **fall back to a full rebuild** (worldgen/load/big
  terraform aren't worth incrementalizing). Simpler and safe.

### 2. Incremental path in `Render`
When `sig` changed but the dirty set is small and **tile-only**, skip the full
rebuild and instead, into the existing cached textures:
- For each dirty hex **and its 4 feather neighbours** (their bleed bands reference this
  tile — [feather loop](../enations_latest/src/SDL2Terrain.cpp)): recompute its diamond
  corners (`MapToWindowHex`, in texture space at the current pan/offset), then
  **re-draw base tile + feather + shade quad** into `s_rt`/`s_shadeRT` over the old
  diamond. Terrain tiles are opaque, so the new diamond overwrites the old cleanly
  (same rule the water redraw relies on).
- The feather is the only cross-hex dependency: an edited hex changes the bleed *into*
  its neighbours, so the redraw set = dirty hexes ∪ their feather neighbours.
- **Shade:** re-render the affected shade quads into `s_shadeRT`, then re-run the
  global blur round-trip (it's 2 `RenderCopy`s — cheap, no need to blur partially).
- **Fog:** untouched — it's a separate throttled overlay
  ([s_fogRT](../enations_latest/src/SDL2Terrain.cpp)) and edits don't change visibility.
- Update the cache's reference position bookkeeping so the next pan-blit stays aligned.

### 3. Altitude edits → bounded region rebuild (stage 2)
Altitude moves corners (shared by 4 diamonds) and changes occlusion, so an in-place
diamond redraw isn't enough. Re-bake a **bounded band**: the changed hexes' rows plus
the rows below/behind that their new height could occlude or reveal (a few rows, using
the existing back-to-front row scan restricted to that y-range). If the band is large,
fall back to full rebuild. This stage is optional — until it lands, **altitude edits
keep the full-rebuild path** (correct, just not optimized); only tile-only edits go
incremental (which already covers roads/city/resource-view, the common cases).

---

## Phasing

- **S1 — dirty-hex tracking + cap/fallback.** Add the dirty list + tile/altitude tag at
  the setters; renderer reads it but still does a full rebuild (no behavior change).
  Verifies the plumbing. **~0.5–1 day.**
- **S2 — tile-only incremental redraw.** The §2 path: redraw dirty ∪ feather-neighbours
  into `s_rt`/`s_shadeRT` + re-blur, fall back on cap/altitude. Covers road build +
  preview, city build/destroy, resource-view toggle, fields. **~2–3 days** (the meat:
  texture-space coords, overwrite correctness, parity with a full rebuild).
- **S3 — altitude bounded-region rebuild (optional).** §3. **~1–2 days.**

Each stage: `./build.ps1 -x64` clean; the incremental result must be **pixel-identical
to a full rebuild** for the same edit (A/B: force full rebuild vs incremental, diff).
Keep the full-rebuild fallback always reachable.

---

## Risks / notes

- **Overwrite correctness:** terrain tiles are opaque (verified at bake) so a redrawn
  diamond fully covers the old one — *for tile-only edits at the same altitude*. This is
  the load-bearing assumption; altitude edits break it (hence S3).
- **Feather neighbour set:** must include the edited hex's neighbours or a seam's bleed
  goes stale. Cheap (4 lookups/hex).
- **Shade blur is global** — re-running it each incremental update is fine (2 copies);
  don't try to blur partially.
- **Parity is the gate** — incremental output must match a full rebuild exactly, or
  edits leave visual crumbs. The A/B diff harness is the acceptance test.
- **Not worth it for:** worldgen, load, large terraform — the cap/fallback sends those
  to the full rebuild, which is correct there anyway.

## Relationship to other work

- This is the **cheap half of "keep a living vertex buffer"** — it avoids the full
  re-bake on *edits* without needing GPU-side transforms (camera moves still recompute,
  which only the OpenGL backend removes). See
  [opengl-rendering-backend.md](opengl-rendering-backend.md).
- **Collision-free:** touches only `SDL2Terrain.cpp` + the edit setters, not the render
  loop / present path the other migration owns.
- Directly reduces the cost of the terrain-edit-gen fixes landed this session
  (building/road/city placement no longer pays a full-frame re-bake).
