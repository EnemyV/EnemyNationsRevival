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
