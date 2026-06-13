# GPU max-zoom perf — handoff / work plan

**Goal:** Area Map at **max zoom-out must run ≥ 30 fps**, measured in **x64 Debug under the debugger** (dbgcatch). The user runs this config exclusively. The 1996 *software* renderer hit full-screen perf in the debugger, so the GPU path (doing less work) can too — the gap is per-frame O(visible-hexes) work that the software path avoided via dirty rects.

**Status when this was written (2026-06-05):** all the changes below are **UNCOMMITTED** (working tree). The GPU sprite/terrain conversion is functionally done and rendering-correct; this doc is about the *optimize* phase to reach 30 fps zoomed out.

---

## How to build / run / measure (do exactly this)

- **Build:** `& 'd:\Enemy Nations\src\build.ps1' -x64 -Quiet` (full path — cwd is not always src). x64 Debug only.
- **Run under the debugger (required):** set `EN_PERF` in the SAME shell as dbgcatch, long window:
  ```powershell
  $env:EN_PERF=1; & 'd:\Enemy Nations\src\dbgcatch.ps1' -Exe 'd:\Enemy Nations\src\cmakeBuild-x64\enations_latest\src\Debug\enations.exe' -Seconds 1800
  ```
  Run it in the background. **Gotchas:**
  - `EN_PERF` MUST be in the same PowerShell command as dbgcatch (each tool call is a fresh shell; env doesn't persist) — else `perf.log` is never written.
  - dbgcatch **kills the game when its `-Seconds` elapses** (debug-detach), so use a long window; it can also idle-out if the game produces no debug events — keep the game active.
  - Wrong-exe trap: launching a stale build (e.g. a `cmakeBuild-x04` dir) makes the Load dialog open in that build's folder → "save not found". Use the `cmakeBuild-x64` exe.
- **Load a save:** `& 'd:\Enemy Nations\src\load-game.ps1' -NoLaunch -Save "savegame" -PickDelaySec 14`
- **Measure:** user zooms ALL the way out, keeps it active (rendering is input-gated when idle), then read `Get-Content "D:\Enemy Nations\perf.log" -Tail 6`.
- **Screenshots of the Area Map are usually BLACK** via PrintWindow (D3D surface) — use `-Screen` with the window visible, or have the user eyeball it. Don't trust `screenshot.ps1` for the area map.

### Profiling counters (added this session — keep them)
`Perf::ScopeCounter("name")` accumulates µs/interval into a named perf.log counter (≈ ms/s, comparable to the render/present columns). Added in `Perf.h`/`Perf.cpp` (`CounterAddElapsedUs`). Current probes:
- `r.inval` / `r.draw` — invalidate pass vs draw pass (mainloop.cpp SEC_RENDER).
- `walk.hexes` / `walk.rows` / `walk.flush` — the UpdateRect walk's per-frame iteration count + tiledraw flush time (terrain.cpp).
- `p.terrain` / `p.sprites` / `p.overlay` / `p.present` — PresentOwn sub-phases (SDL2Panel.cpp).
- `t.rebuild` — GPU terrain mesh rebuild (SDL2Terrain.cpp).

---

## Measured breakdown — max zoom, static, ~2.8 fps (µs/s ≈ ms/s)

```
render 750 = r.inval 178  +  r.draw 570            <- the walk dominates
present 300 = p.terrain 150 + p.sprites 120 + p.overlay 5 + p.present 7
walk.hexes = 133,902 / frame   (to find 12 buildings + 35 vehicles + trees)
t.rebuild = 0   (mesh cached when static; NOT the bottleneck)
```

**Confirmed conclusions:**
- The **full-map hex scan** (`r.draw` 570, scanning ~134k hexes to find ~47 units) is #1.
- **Overlay upload is NOT a problem** (`p.overlay` 5 ms) — do NOT bother caching the chrome overlay; that idea was killed by measurement.
- Three more real costs, all the **same O(visible-hexes/sprites) pattern**: `r.inval` 178, `p.sprites` 120 (≈ thousands of trees drawn), `p.terrain` 150 (fog re-sample over all visible hexes).

---

## Work items (implement SERIALLY: change → user verifies zoomed-out render is correct → re-measure → next)

### 1. `r.draw` 570 ms → object/linear-scan sprite capture  *(biggest; do first)*
**Where:** `CGameMap::UpdateRect` in `terrain.cpp` — the `for(y){ for(x){...} }` walk (~line 3609+). It only runs for the GPU area panel (`bSplit`), and in that mode the terrain is the GPU mesh, so **the walk's only job is to DISCOVER sprites**.
**Problem:** at max zoom it iterates the whole map (~134k hexes) to find ~47 units + trees.
**Fix options (pick the lower-risk that works):**
- **(a) Linear map-array scan:** iterate the contiguous hex array (`m_pHex`, `m_eX`×`m_eY`), cheaply read each hex's `byUnits`/type, and only do the expensive projection + dispatch for non-empty/forest hexes. Convert map hex → view frame via `hex.ToNearestHex(refHex)` where `refHex` = `aa._WindowToHex(viewport center)`. Cull with `aa.CalcWindowHexBound`. Reuse the existing per-type dispatch (building foundation+BG/FG layers, bridge, vehicle, tree, projectile).
- **(b) Object-driven:** iterate `theBuildingMap` / `theVehicleMap` / `theProjMap` / `theBridgeMap` (global `CMap`s; `GetStartPosition`/`GetNextAssoc`) for units, and a **cached forest-hex list** (built once per map load / `s_loadGen`) for trees. Same ToNearestHex + cull + dispatch.
**z-order:** the GPU sprite layer already sorts by the engine's projected `m_ptCenter` (`g_enSprSortX/Y`) + capture `seq`, so **capture order doesn't matter** — no need to reproduce the row-by-row interleave. This is what makes the rewrite tractable.
**Risks:** torus wrapping (handle via `ToNearestHex`); buildings span multiple hexes (guard so each is captured once — the existing code uses `hexBuilding == hexcoord`); vehicles are at sub-hex granularity (`GetVehicleDrawInfo(pveh, hex)`).
**Likely also cuts `p.sprites`** if combined with tree culling (below).

**CONFIRMED facts for the linear-scan (approach a) — verified 2026-06-05:**
- **Hex array stride = `1 << m_iSideShift`, NOT `m_eX`** (rows padded to a power of 2). Iterate `for(my<m_eY){ CHex* row = m_pHex + (my<<m_iSideShift); for(mx<m_eX) row[mx]; }`. Members `m_pHex/m_eX/m_eY/m_iSideShift` are protected but `UpdateRect` is a member (or use `Get_eX()/Get_eY()/GetSideShift()`). On THIS 512² map stride==m_eX so a flat scan works here, but use the shifted form for correctness.
- **Sprite-interest mask = `bldg|veh|bridge|proj` = 0xDF.** Flags (CHex, `GetUnits()` returns `m_bUnit`, trivial inline): `ul=01 ur=02 ll=04 lr=08 bldg=10 minerals=20 bridge=40 proj=80`; `veh=ul|ur|ll|lr=0x0F`. **Exclude `minerals` (0x20)** — not a sprite. Trees are separate: `GetType()==CHex::forest`.
- **`GetType()` = `int(m_bType)&0x0F`** (cheap); terrain enum `city=0,desert=1,forest=2,…`.
- **Coord pairing in dispatch:** `hexcoordWrapped = CHexCoord(mx,my)` (map coords → used for `theBuildingHex._GetBuilding`/`theBridgeHex.GetBridge`/vehicle subhex lookups); `hexcoord = hexcoordWrapped.ToNearestHex(refHex)` (view-frame → projection/draw). `refHex = aa._WindowToHex(viewport center)`.
- **Building draw-once guard:** keep `hexBuilding == hexcoord`; DROP the `x==iLeftX || y==iTopY` view-edge part (linear scan visits the anchor directly).
- **DROPPED-SPRITE TRAP:** buildings span multiple hexes AND extend up by `iMaxBuildingHeight`; do NOT cull on the bare anchor hex — a building whose anchor is off-screen but body is visible must still draw. Cull with margin = `iMaxBuildingHeight` (top) + footprint (`GetCX/GetCY` hexes) on the sides, or cull generously. Over-inclusion is free (Submit prunes off-screen); under-inclusion drops a sprite you can't screenshot.

### 2. `p.sprites` 120 ms → cull trees at extreme zoom
**Where:** tree capture in the walk (terrain.cpp, `CHex::forest` branch ~line 3870) + `SDL2Sprites::Submit` (draws all captured sprites in one `RenderGeometry`).
**Hypothesis (CONFIRM with a tree count):** at max zoom every forest hex on the visible map is captured → thousands of ~2px tree sprites drawn every frame. Add a counter for captured-tree count to confirm.
**Fix:** skip tree capture below a zoom threshold (they're sub-pixel and invisible anyway), or decimate. Verify visually with the user that culled trees aren't missed at that zoom.

### 3. `p.terrain` 150 ms → throttle/skip the fog re-sample
**Where:** `SDL2Terrain::Render` in `SDL2Terrain.cpp` — the fog re-sample block (`s_fogVis`/`s_fogVerts`, throttled ~150 ms) loops over all visible fog hexes.
**Fix:** skip the re-sample when no unit vision changed (dirty flag), and/or lengthen the throttle when zoomed out. `t.rebuild=0` confirms the mesh itself is fine — only the fog loop is O(hexes).

### 4. `r.inval` 178 ms → skip/cheapen the invalidate pass in GPU mode
**Where:** `CWndArea::ReRender` → `theMap.Update(m_aa)` (the invalidate pass that builds dirty rects). In GPU full-redraw mode we **ignore** the dirty rects (we redraw the whole viewport every frame), so this pass is largely wasted.
**Fix:** investigate what `theMap.Update` scans; if it's O(visible hexes) and only produces dirty rects we don't use, gate it off (or minimize) when `bGpuFull`. Careful: it may also advance animation/visibility bookkeeping — confirm before skipping.

---

## What's already landed this session (uncommitted — context for the diff)
- **Terrain no longer double-rendered:** CPU `phex->Draw` skipped when `bSplit` (GPU mesh draws it). terrain.cpp.
- **Per-row `iRowTopY`:** one projection per row instead of per hex (was the first zoomed-out win). terrain.cpp.
- **Full-viewport capture per frame** (replaced the 620-dirty-rect walks): `CAnimAtr::Render` bGpuFull branch does one `UpdateRect` over the whole client rect; `RenderToPanel` skipped (unused m_surface). terrain.cpp.
- **Sprite layer draws DIRECT to the window** (removed the `g_rt` render-target round-trip; deleted `BuildFull`/`BuildIncremental`). `SDL2Sprites::Submit`.
- **Construction "swype" fixed:** capture hook uses `rectClip & rectBound` band (correct now that the walk is full-viewport — the old `g_enSprClip*` globals were removed). sprite.cpp / unit.cpp / SDL2Sprites.*.
- **Area Map toolbar restored:** RenderDetached composites the bar as bottom-chrome (`SetBottomChromePanel`). SDL2Panel.*/area.cpp/SDL2AreaBar.
- **Perf sub-timers** (above). Perf.h/Perf.cpp + probes.

**These are uncommitted.** Consider committing the working set before the perf rewrites (stage specific files; git status has ~50 untracked junk files — do NOT `git add -A`). No "Co-Authored-By" trailers.

## Don't
- Don't cache the overlay (p.overlay = 5 ms; measured non-issue).
- Don't run msbuild directly (hook-blocked); use build.ps1.
- Don't parallelize the 4 fixes across agents editing code — they touch the same hot functions and each needs serial live verification. Read-only investigation can be parallel.
- **Don't "fix" perf by culling/disabling/staling content (no tree culling at zoom, no fog throttle).** Parity with the original is the bar — its software renderer drew every tree at max zoom fast, via dirty rects. Solve the root cause. (User feedback, 2026-06-06.)

---

# PROGRESS LOG (2026-06-06)

## Committed
- **Item 1 — `2bd5ba7`** GPU sprite discovery (object-iterate buildings/vehicles + viewport-bbox hex scan for bridge/forest/proj, replacing the full per-hex view walk). `r.draw` 570→~224 ms at max zoom (10x heavier save). `CGameMap::DiscoverSpritesGpu`.
- **Item 4 — `4c443c2`** Skip the invalidate pass (`theMap.Update`) in GPU full-redraw mode (`CAnimAtr::IsGpuFull()`); ambients advance in the DRAW pass, so safe. `r.inval` 335→~0.4 ms; render pass 565→~280 ms. Verified: animations/fog/cursors all correct.

## Uncommitted (working tree)
- **Fog vision-dirty flag** (legit, event-driven): `g_enFogVisGen` bumped on hex visible↔invisible transitions (terrain.inl); SDL2Terrain fog block skips re-sample/re-render when fog unchanged + 1 s self-heal. Helps a static view; does NOT help active play (fog churns). **Superseded in spirit by the dirty-rect work below** — keep as a building block (it's a precise "fog changed?" signal the dirty-rect pass can reuse). The zoom-scaled fog throttle that was added on top was REVERTED (it was a stale-content workaround).

## Remaining cost @ max zoom (heavy active save, after items 1+4)
```
render ~280 = r.inval ~0 + r.draw ~224   (r.draw = re-DISCOVER 27k trees/frame)
present ~540 = p.terrain 220-450 (FOG re-render 300k verts) + p.sprites ~112 (re-SUBMIT 27k trees) + ...
walk.rows ~27,446  (sprite count, overwhelmingly TREES)
```
Root cause: full-redraw-every-frame redoes O(all sprites)+O(all fog) each frame though trees are static and fog barely changes.

## CHOSEN FIX — Item 5: full GPU dirty-rects (the original's model) — IN PROGRESS
User chose dirty-rects over a cached-static-layer because it's O(changed) not O(all sprites) AND it fixes fog with the same mechanism (a fog change is just a dirty rect). Cached-static-layer (option A) can't reach 30 fps because it leaves fog untouched and still rebuilds the 27k-vertex buffer every frame.

**Present path today (detached area window):** `SDL2Panel::PresentOwn()` — clear → `SDL2Terrain::Render` (mesh+fog) → `SDL2Sprites::Submit` (sprites) → RenderCopy CPU overlay (`m_ownBackTex` = chrome/title/frame, content-transparent) → `RenderPresent`. Everything redrawn every frame; `RenderPresent` swaps buffers so nothing persists. Terrain mesh is already cached (`s_rt`, margin-space, pan-offset composite; rebuilt on pan-past-margin/zoom/`m_iDir`/`s_loadGen`). Fog is a cached texture `s_fogRT` re-rendered on a throttle. Sprites live in `g_sprites` (map keyed `{dib,vx,vy}`), recaptured every frame, sorted, emitted as one atlas-batched `RenderGeometry`.

**Target architecture:** a persistent content render target `s_contentRT` that is NOT cleared each frame. Each frame, compute dirty rects and redraw only those (scissored): terrain sub-rect (RenderCopy cached `s_rt`+shade) + overlapping sprites (z-sorted) + fog sub-rect. Then RenderCopy `s_contentRT` → window, overlay chrome, present. Static trees in untouched regions persist.

### Staged plan (each stage behind `g_enGpuDirty`, default OFF until validated; commit per stage)
- **S0 — scaffolding (no behavior change).** Add persistent `s_contentRT` + runtime flag `g_enGpuDirty` (env `EN_DIRTY`). When ON, route the EXISTING full composite (terrain+fog+sprites) into `s_contentRT`, then RenderCopy to window + overlay + present. Proves the RT indirection is parity-correct; costs +1 copy/frame. When OFF, today's path verbatim.
- **S1 — sprite spatial index + old/new tracking.** In SDL2Sprites, keep per-sprite previous-frame bbox and a coarse grid index (hex-cell buckets) so we can answer "which sprites changed this frame" (moved/added/removed/animated) and "which sprites overlap rect R" cheaply. Still full-redraw; just building data. Validate counts vs brute force.
- **S2 — dirty-rect sprites.** With `s_contentRT` persistent, redraw only sprite regions that changed (union of each changed sprite's old+new bbox, coalesced). For each dirty rect: RenderCopy terrain sub-rect from `s_rt`, then re-emit sprites overlapping the rect (z-sorted, scissored). Static trees never touched. Animated sprites (vehicles) dirty every frame — expected, O(units).
- **S3 — dirty-rect fog.** Fog frontier change = dirty rect (reuse the `g_enFogVisGen` signal, but per-hex region not whole mesh). Redraw only changed fog sub-rects into `s_fogRT`/content. Kills the 300k-vert/frame fog cost.
- **S4 — tune.** Rect coalescing heuristics, full-invalidate on pan/zoom/resize/`s_loadGen`, cursor-overlay handling, the secondary-window present throttle. Then flip `g_enGpuDirty` default ON, keep OFF as fallback.

**Z-order parity:** preserved — within each dirty rect we re-emit ALL overlapping sprites in full depth order (tree rear/front around a unit stays correct), exactly like the 1996 dirty-rect renderer. No flattening.

**Known follow-on optimizations (after S0-S4, per user):** atlas-batch the per-rect emits, coalesce nearby moving-unit rects, cap rect count, skip frames with zero dirty.

### S0 DONE — `886dbde` (persistent `m_contentRT` + `EN_DIRTY`, full-redraw through RT, parity). Verified runs in-game.

### KEY DISCOVERY (2026-06-06) — the dirty-rect machinery already exists, just bypassed
`SDL2Sprites` was BUILT for dirty-rects and the recent full-viewport work *bypassed* it rather than removing it:
- `g_sprites` (unordered_map keyed by `{dib, view-x, view-y}`) is a **persistent across-frames** store. `BeginFrame(zoom,dir, dvx,dvy,dw,dh)` already **erases only sprites overlapping the given dirty rect**; captures re-insert. So static sprites SHOULD persist frame-to-frame.
- **Why it's O(all) today:** `CAnimAtr::Render`'s `bGpuFull` branch calls `UpdateRect` over the WHOLE client rect every frame → `BeginFrame` is handed the whole viewport → it erases ~all sprites → `DiscoverSpritesGpu` recaptures ~all → `Submit` sorts+emits ~all. Three O(all-sprites) passes.
- `g_rt` (cached layer texture) + `AccumDirty` still exist but are **dead** (Submit draws direct to the window and ignores `g_accum`). `BuildFull`/`BuildIncremental` were deleted.
- Plan doc's stated reason the OLD dirty-rect path was dropped: *"hundreds of tiny UpdateRect calls each running BeginFrame's O(all-sprites) overlap scan (~620×280)."* **That overlap scan is the ONE real flaw, and the S1 spatial index fixes exactly it.**

### THE TENSION with item 4 (must resolve in S1) — dirty rects need a CHANGE source
The original got its dirty rects from the **invalidate pass** (`theMap.Update` → `UpdateRect(invalidate)`), which **item 4 (`4c443c2`) DISABLED** in GPU-full mode (it was a 335 ms full per-hex scan). Dirty-rects need to know *what changed*, so we need a change source again — but NOT the 335 ms full-hex scan. The right source is **the units themselves**: a unit that moves/animates this frame reports its old+new view-space bbox into a per-frame dirty-rect list — O(moving units), not O(all hexes). Trees never report (static) so they're never dirtied. Options to get old+new bbox cheaply: (a) diff this-frame vs last-frame captured pos per unit id; (b) have the sim mark a unit dirty on move and the render read its previous render bbox. Investigate which is cleanest.

### PROGRESS (2026-06-06)
- **Fog vision-dirty flag — `37bf5a6`** (committed standalone).
- **S0 — `886dbde`** persistent `m_contentRT` + `EN_DIRTY`. Runs in-game.
- **S1 — `fee67cb`** `CHexValidMatrix` dirty-hex list + `inval.hexes` probe. **MEASURED: `inval.hexes`=0/frame at max zoom, 179 vehicles.** Conclusion: hex flags are silent during play; the dominant dirty source is the **visible-unit sweep (~180)**, not hex flags. Approach validated — dirty set ~180 vs 55k hexes + 27k trees.
- `BeginFrame(zoom,dir, dvx,dvy,dw,dh)` confirmed: erases `g_sprites` overlapping the rect (O(all sprites) scan = the flaw S1's spatial index target fixes), `AccumDirty`s it. Today it's handed the WHOLE viewport so it erases everything; S2 hands it the small unit rects.

### S2 spec (the substantial build — touches capture + present together)
Per frame in `EN_DIRTY` mode:
1. **Dirty-rect list** = visible-unit sweep (each vehicle/proj/constructing-bldg bbox, `PAINT_BOTH` = also carry last-frame's rects so vacated spots repaint) ∪ hex-flag list ∪ (full rect on pan/zoom/scroll). Build it in/around `DiscoverSpritesGpu` (already iterates the units).
2. **Incremental capture:** call `BeginFrame` per dirty rect (not whole viewport); walk/recapture only those rects. Static trees keep their `g_sprites` entries. Needs a spatial index over `g_sprites` so the per-rect erase is O(in-rect).
3. **Incremental render:** DON'T clear `m_contentRT`. For each dirty rect: scissor; RenderCopy terrain sub-rect from `s_rt` (+ shade) + re-emit `g_sprites` overlapping (z-sorted) + fog sub-rect. Persist the rest.
4. Composite `m_contentRT` → window + overlay (unchanged).
Touch points: `terrain.cpp` UpdateRect/DiscoverSpritesGpu, `SDL2Sprites` (BeginFrame/Submit + spatial index + dirty-list API), `SDL2Terrain::Render` (per-rect terrain+fog), `SDL2Panel::PresentOwn`. Verify each step in-game (a missed dirty rect = a visible smear).

### S2.1 / S2.3 DONE — breakdown shifted (2026-06-06)
- **S2.1 — `55d97d8`** dirty-rect generation (vehicle sweep, `PAINT_BOTH` carryover) + `dirty.rects` probe. ~80 rects/frame.
- **S2.3 — `248c646`** incremental CAPTURE. Trees/bridges persist in `g_sprites` (view-space keys scroll-invariant); only dynamic objects (buildings/vehicles/projectiles, tracked in `g_dynKeys`) drop+refresh; forest scan skipped on static frames. Full capture on zoom/dir/pan. **r.draw 270-377 → 83-99 ms; walk.rows 27,592 → 960.** Verified clean in-game (trees present, no smears).

**Remaining @ max zoom (active save), in priority order NOW:**
```
p.terrain (FOG re-render) ~457-594 ms   <- #1 (fog changes most frames in active play)
p.sprites (Submit emit-all) ~124-230 ms <- #2
r.draw (capture) 83-99 ms               <- fixed (S2.3)
```
So the render side is the remaining cost, and **fog (S3) is now bigger than sprite-emit (S2.4)**.

### S2.4 spec (incremental sprite EMIT) — needs a spatial index
Revive `g_rt` as a PERSISTENT sprite-layer texture (it already exists, currently dead). Submit:
- full frame: clear `g_rt`, sort+emit all sprites into it.
- incremental: for the dirty rects (cur ∪ prev, coalesced), scissor+clear each in `g_rt` and re-emit ONLY the sprites overlapping (z-sorted) — preserves per-sprite depth (tree rear/front around a unit). The rest of `g_rt` persists.
- PresentOwn composites `terrain → g_rt → fog` (RenderCopy `g_rt` instead of Submit-to-window).
**Blocker:** "sprites overlapping rect R" is O(all sprites) without a spatial index (S2.2). Naive = 3,940 × ~80 rects = 315k checks/frame (try coalesced first; add a view-space grid index if too slow). Two flat layers (static-under-dynamic) was REJECTED — it breaks the unit-between-tree-rear/front z-order (that's why per-rect, not layered).

### S3 spec (incremental FOG) — the bigger win now
Fog re-render rebuilds all of `s_fogRT` (~300k verts) whenever `g_enFogVisGen` bumps. Make it per-region: track which hexes transitioned visible<->invisible (a fog dirty-hex list, like S1's `CHexValidMatrix` list) and re-render only those hexes' fog quads into `s_fogRT` (clear their sub-rects first; mind the altitude-overlap OVERWRITE blend). Likely the single biggest remaining fps win.

### S2.4 + S3 DONE; the bottleneck is now the TERRAIN MESH REBUILD (2026-06-06)
Committed since the staging above: S2.1 `55d97d8`, S2.3 `248c646`, fog-rot-fix `8d74e64`, S3 `b117906`. S2.4 (incremental sprite EMIT, persistent g_rt + per-rect re-emit) **DONE** (uncommitted at time of writing — p.sprites 190ms→~5ms).

**MEASURED win condition status (x64 Debug, max zoom, GPU): goal = 30 fps.**
```
NO mesh rebuild (static):   fps 13-19   t.rebuild=0   p.sprites~22k p.terrain~4k   <- sprites+fog now ~free
ONE mesh rebuild fires:     fps 0.7-11  t.rebuild = 296,000 .. 1,300,000 us (0.3-1.3 SECONDS, single frame)
```
Instrumented `rebuild.cnt/key/edit/pan` (SDL2Terrain.cpp after needRebuild). In active combat `rebuild.edit=1 EVERY frame` → 0.7 fps. So:
- The dirty-rect work succeeded — when the mesh isn't rebuilding we're at 13-19 fps.
- **The lone remaining killer is the terrain mesh rebuild: a full re-mesh of all ~50k hexes (0.3-1.3 s) fires on EVERY terrain edit** (g_enTerrainEditGen, bumped in CHex::SetAlt / SetVisibleType — combat craters/scorch + roads). I did NOT touch the rebuild; my opts just exposed it.

### NEXT: incremental terrain mesh patch (the fix; user-aligned)
The mesh is already a cached texture `s_rt` (panning = a blit at offset, no rebuild). Rebuild only fires on zoom / dir / s_loadGen / terrain-edit. **Edits are the combat problem.** The terrain code ALREADY patches in place for water animation ("re-drawn IN PLACE into s_rt on the wave-tick instead of rebuilding"). Generalize it:
1. The edit sites are CHex members (`SetAlt`/`SetVisibleType` in terrain.inl:238/274), so `this` = the changed hex. Record changed hexes into a list there (like the fog/CHexValidMatrix lists) instead of (only) bumping the rebuild key.
2. In SDL2Terrain::Render, when ONLY g_enTerrainEditGen changed (no zoom/dir/loadgen/pan), PATCH the changed hexes (+ their feather neighbours, + their fog/shade quads) into s_rt/s_fogRT/s_shadeRT in place — the water-tile path is the template — and DON'T full-rebuild. Cost O(hexes changed) ≈ handful, sub-ms.
This keeps combat at the static fps (13-19) instead of 0.7. THEN profile the static frame to push 13-19 → 30 (zoom/dir rebuilds are user-driven + occasional; secondary — could cache 4 dir copies / async later).

### REVISED stage plan (revive, don't rebuild)
- **S1 — spatial index + change source.** (a) Add a coarse grid (hex-cell buckets) over `g_sprites` so `BeginFrame`'s "erase sprites overlapping rect R" is O(sprites in R), not O(all) — kills the old flaw. (b) Build the per-frame dirty-rect list from moving/animating units (old+new bbox), replacing the whole-viewport `BeginFrame`. Keep `EN_DIRTY` OFF default; validate the dirty-rect list matches reality (every moved unit covered).
- **S2 — incremental capture + emit.** Drive `DiscoverSpritesGpu` to re-walk ONLY the dirty rects (static trees keep their cached `g_sprites` entries). Re-render only the dirty sub-rects of `m_contentRT`: per rect RenderCopy terrain from `s_rt` + re-emit `g_sprites` overlapping the rect (z-sorted via the spatial index), scissored. Submit stops doing the whole viewport.
- **S3 — fog dirty rects.** Fog frontier change → dirty rect (the `g_enFogVisGen`/per-hex transition signal); redraw only changed fog sub-rects, not the 300k-vert mesh.
- **S4 — tune + flip `EN_DIRTY` default ON** (keep OFF as fallback). Coalesce rects, cap counts, skip zero-dirty frames.

**Open uncommitted:** the fog vision-dirty flag (terrain.inl/terrain.h/terrain.cpp/SDL2Terrain.cpp) — fold into S3, or commit standalone first. It's a precise per-hex "fog changed" signal S3 will reuse.

### HOW THE ORIGINAL DIRTY-RECTS WORK (read the code, 2026-06-06) — the template to copy
`CDirtyRects` (wind22/dibwnd.h): 3 coalescing rect lists — `PAINT_CUR` (repaint this frame), `PAINT_NEXT` (also next frame), `BLT` (copy to screen). Two passes/frame:
- **Invalidate pass** (`UpdateRect(invalidate)`) walks visible hexes; things that CHANGED add their bound:
  - terrain hex adds a rect only if `IsInvalidated()` ([terrain.cpp:2536]); buildings only if invalidated.
  - animated sprites (units/fire/effects) add `PAINT_BOTH` ([sprite.cpp:2782]).
  - **static sprites (trees/idle buildings) compute their bound but NEVER self-dirty** → only repainted when another dirty rect overlaps them.
- **Draw pass**: `for r in PAINT_CUR: UpdateRect(r, draw)` redraws terrain+overlapping sprites in z-order, clipped to r. No-rect regions untouched (trees persist). `BltRects` copies the rects.
- **`PAINT_BOTH` = the movement-trail trick:** a mover dirties its CURRENT pos into CUR (draw now) AND NEXT (UpdateLists promotes NEXT→CUR next frame, so the just-vacated spot repaints and the trail self-erases). No old-position bookkeeping needed.
- The only costly part is the invalidate pass's O(all-hexes) discovery scan (= the 335 ms item 4 removed). Replace it with push-based sources, NOT the scan.

### DIRTY SOURCES for the GPU port (two kinds, both O(changed))
1. **hex flags** — `CHexValidMatrix::SetInvalidated(x,y)` ([base.h:148], a `CBitMatrix` bit). Many sites fire from SIM code as units move / vision updates. **S1a: also push (x,y) to a list (dedup on the bit), Clear() empties it** → enumerate invalidated hexes in O(changed) instead of scanning. Covers terrain/fog/cursor/building changes.
2. **moving/animated sprites** — iterate the visible unit maps (vehicles/projectiles/constructing buildings; already done in `DiscoverSpritesGpu`, ~hundreds) and emit each one's `PAINT_BOTH` rect. Covers movement; no hex walk. (Idle units that don't animate/move add nothing → cached.)

Union of (1)+(2) = the per-frame dirty-rect list. Feed it to `BeginFrame` (per-rect sprite erase via the S1 spatial index), re-capture only those rects, re-render only those sub-rects of `m_contentRT`. `PAINT_BOTH` semantics handle scroll/pan trails too.
