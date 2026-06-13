# GPU terrain — rocky ("hill"/"rough") shorelines look wrong at the water

Status: **RESOLVED (2026-06-05).** Root cause found and fixed in worldgen; confirmed
visually (user) and by instrumentation (gap count → 0 on fresh maps). It was NOT a GPU/
render bug — it was an original-game worldgen bug.

## Resolution (the actual root cause)

`CGameMap::AddCoastlines` shores the coast in passes. **Pass 1** converts ocean→coastline on
the water side (works — water hexes take `SetType`'s simple-store path). **Pass 2** ("corner
fill") tries to turn coastal *land* corners into shore via `pHex->SetType(CHex::coastline)`,
comment *"we just blast coastline."*

But `CHex::SetType` ([wrldinit.cpp:56](../enations_latest/src/wrldinit.cpp#L56)) does **not**
blast — for a **land** hex it runs an altitude/**slope** re-derivation: `slope>15 → mountain`,
`slope>8 → hill`, else → the requested type. So on **steep** coastal ground the intended shore
tile is silently kept as **hill/mountain** (`oldtype==newtype`), leaving rock hard against the
water with no shore. Gentle (grassy) slopes convert fine — which is exactly why grass shores
always looked right and rocky/steep ones didn't. It was *slope*, not elevation.

Proven by per-pass instrumentation on fresh maps: ocean-adjacent-to-rock = **0 after pass 1**,
jumps to the final value **immediately after pass 2**; a direct probe showed pass-2's
`SetType(coastline)` calls coming back as hill/rough/mountain (all touching ocean), matching
the gap.

**Fix (one line):** treat `coastline` as a force-store water-EDGE type in `SetType`, like
city/road — add `|| iType == coastline` to the early-out at
[wrldinit.cpp:66-69](../enations_latest/src/wrldinit.cpp#L66) (tagged `[shore-fix]`). Pass 2's
corner fill now produces real coastline that flows through pass-4 sprite assignment and gets
proper shore art. Worldgen-only → **new maps only** (existing saves bake the old terrain; the
user confirmed only new maps matter). After-fix: pass-2 fail count and end gap both 0.

Why earlier attempts failed: they fought the GPU edge-feather, or post-hoc converted ocean→
coastline *after* pass-4 had assigned sprites (→ river-coast garbage). This fixes the real
cause at the right time. All `[SHOREDBG]`/`[SHOREP*]`/`[SHOREGEN]` probes have been removed;
only the one-line fix remains.

---

## (Historical — original investigation notes below)

Owner notes: this doc was the single source of truth for the bug; update it, don't spawn new summaries.

---

## 1. The bug, in plain terms (user-reported)

On an island, where **grass** meets the water the coast looks right — a soft, natural
shoreline where the land eases into the sea. Where **rocky / hilly** ground meets the
water, it looks wrong — the rock just **stops dead at the waterline with a hard, blocky
edge**, no shore transition. Along the same coast the grass stretches look finished and
the rocky stretches look abrupt/unfinished.

User hover-tooltip over the bad terrain reads **"hill"**; user also refers to **"rough"**
tiles still lacking shores. (hill and rough are two distinct `CHex` terrain types — see §3.)

User's own hypothesis (worth taking seriously): *"the shore placing is being done
correctly to the land [grass] but not to the hill. It's likely a matter of how the shores
are placed."*

### Latest, sharper observations (2026-06-05) — the important new clues
- **"rough" tiles STILL don't have shores** after attempt #2.
- **"the water tile under the shore is blending with the grass tiles — that's wrong"**:
  attempt #2 (bleeding open water into land edges) made ocean visibly smear onto grass.
  That is a *new* defect introduced by the attempt, and it has since been reverted.

So: the real defect is NOT the edge feather (see §5 — the feather is likely a red
herring). There is a genuine underlying issue we have **not** located yet.

---

## 2. Relevant rendering architecture

GPU terrain lives in [SDL2Terrain.cpp](../enations_latest/src/SDL2Terrain.cpp). It renders
the terrain mesh once into an off-screen render-target texture (`s_rt`) and pan-blits it;
it only rebuilds on a view change. Gated by INI **`[Advanced] Renderer`** (see
[SDL2Panel.cpp:862](../enations_latest/src/SDL2Panel.cpp#L862),
`MaybeCreateOwnRenderer`). `Renderer=0` → original **software** composite path
(`CGameMap::UpdateRect`); `Renderer=1` → GPU path. **This toggle is the key A/B test we
have not yet run** (see §6).

Two concepts matter for shores:

### (a) Coastline placement (worldgen, shared by BOTH renderers)
[CGameMap::AddCoastlines, wrldinit.cpp:2508](../enations_latest/src/wrldinit.cpp#L2508):
- Pass 1: every **`ocean`** hex adjacent (8-way) to a non-water, non-coastline hex is
  itself **converted to type `coastline`**. Rivers get riverbanks. **`lake` and `swamp`
  are NOT converted in this pass** (the comment says "oceans (and lakes)" but the code
  only tests `CHex::ocean`).
- Pass 2 (×2): fills in land-side coastline at corners — a land hex wedged between
  coastline/water on perpendicular sides becomes coastline.
- Pass 3: coastline hexes that touch no water (even diagonally) revert to `plain`.
- Pass 4 (2690+): assigns the coastline **sprite** from a 4-bit water/coast neighbour
  bitmask → `land_up/dn/lf/rt/ll/ul/lr/.../island` facings.

Key art fact: the coastline tile's **land side is grassy/sandy shore art**. It is the same
art regardless of whether the land behind it is grass or rock. So a coastline tile sitting
next to a **rough/hill** hex shows grass-shore meeting rock = inherent mismatch. There is
**no rocky-shore art**.

### (b) Edge feather (GPU only)
[SDL2Terrain.cpp ~825-880](../enations_latest/src/SDL2Terrain.cpp#L825). For each
land hex, bleeds a thin (`kBand=0.38`), low-alpha (`kFeatherA=135`) band of a *differing*
featherable neighbour's texture along the shared diamond edge — approximating the original
software rasterizer's 1-px checkerboard dither
([sprite.cpp ~660-690](../enations_latest/src/sprite.cpp#L660), `FEATHER_IN/OUT/INOUT`).
Predicates:
- `Featherable(t)` = `t != road && t != city && t != resources`.
- `IsOpenWater(t)` = ocean|lake|river|swamp.
- Receiver gate: `Featherable(type) && !IsOpenWater(type) && type != coastline`.
- Source gate (current/reverted state): `!Featherable(ntype) || IsOpenWater(ntype)` → skip.

The ORIGINAL software path feathers land↔ocean as `FEATHER_INOUT`
([terrain.cpp:2424](../enations_latest/src/terrain.cpp#L2424)) — ocean is *not* excluded
as a feather participant there.

---

## 3. Terrain types involved

`kTypeName[]` ([SDL2Terrain.cpp:29](../enations_latest/src/SDL2Terrain.cpp#L29)):
index 4 = **hill**, 5 = mountain, 10 = **rough**, 12 = **coastline**, 6 = ocean, 3 = lake.
`type` used in the renderer = `phex->GetSprite()->GetID()` (the SPRITE id), matching the
original's `m_psprite->GetID()`. hill and rough are *different* types but both render as
rocky/gray tiles. The diagnostic currently lumps them (`type==hill || type==rough`); a
next step is to split them, since the user now specifically calls out **rough**.

---

## 4. Diagnostic evidence (text instrumentation, not screenshots)

Added a `[SHOREDBG]` log line per terrain rebuild
([SDL2Terrain.cpp](../enations_latest/src/SDL2Terrain.cpp), search `SHOREDBG`) →
written to `d:\Enemy Nations\SDL2Terrain.log`. Counts per rendered frame:
hill/rough hexes drawn, how many border a coastline neighbour, how many border open
water directly, total feather bands, hill→coast bands, hill→ocean bands.

Observed:
- **Inland city view:** `hillRough=206 waterAdj=0 coastNbr=0 oceanNbr=0` — lots of rocky
  terrain, none at a shore. (Confirms the bug is shore-specific.)
- **Island view (zoom=0):** `hillRough=10 waterAdj=9 coastNbr=9 oceanNbr=4
  bandsTotal=331 hill->coastBands=15`.
  - 9/10 rocky hexes touch a **coastline** tile and DO get feathered.
  - **4 touch raw OPEN WATER directly** (no coastline tile between rock and ocean).
    My feather skipped open-water neighbours → those 4 rock-vs-water edges got zero
    softening.

So *some* rocky hexes border raw ocean with no coastline buffer. Open question: **why**
do these ocean hexes not become coastline next to rock, when they apparently do next to
grass? (See §6 hypotheses — could be a `lake`/`swamp` body, an altitude condition, or a
worldgen ordering effect. The `oceanNbr` counter currently lumps all open-water types;
split it to learn whether these are truly `ocean` or are `lake`/`swamp`.)

---

## 5. Attempts so far (both failed)

### Attempt #1 (prior session) — feather predicate inversion
Excluded `coastline` as a feather *receiver*; let land tiles pull the coastline grass IN
along their water-facing edge. Built clean. **No visible change** (user confirmed via a
pasted island screenshot). → The hill hexes that border coastline were *already* getting
softened, so this was effectively a no-op for the visible defect.

### Attempt #2 (this session) — feather toward open water
Removed the `IsOpenWater(ntype)` skip so land tiles also feather toward raw-ocean
neighbours (matching the original's land↔ocean `FEATHER_INOUT`). Built clean.
**Result (user):** "rough tiles still don't have shores" AND "the water tile under the
shore is blending with the grass tiles — that's wrong." → Did not fix rough; introduced
a water-smears-onto-grass regression. **Reverted.**

**Conclusion:** tuning the GPU edge-feather is not the fix. The grass-vs-rock difference
is not (only) about the 1-px edge blend. The real issue is structural — most likely in
**how/whether shore (coastline) tiles exist around rocky terrain**, or how the rocky tile
art itself meets the shore.

---

## 6. Hypotheses to investigate next (ranked)

1. **A/B against the software renderer — do FIRST.** Set INI `[Advanced] Renderer = 0`
   (see [SDL2Panel.cpp:862](../enations_latest/src/SDL2Panel.cpp#L862)) and view the
   *same* island save.
   - If rocky shores look **correct** in software → the bug is purely in our GPU path
     (feather/coastline rendering/tile selection), and we have a reference to match.
   - If they look **the same (bad)** in software → the original always rendered rocky
     shores this way; the "fix" is an *enhancement* (e.g. give rough/hill a real shore
     transition), not a regression repair. This single test decides the whole direction.

2. **Coastline not placed around rock the way it is around grass.** Re-examine
   `AddCoastlines`: does an `ocean` hex adjacent to a `hill`/`rough` hex reliably become
   `coastline`? Check the altitude drop logic (pass 1, the `> sea_level+1` guards) and
   whether high rocky hexes block the land-side fill (pass 2). Instrument worldgen: for
   every water-adjacent hill/rough hex, log whether its water-side neighbour is coastline
   vs raw ocean/lake/swamp, over the WHOLE map (not just the view).

3. **The 4 raw-ocean-adjacent hexes are actually `lake`/`swamp`,** which `AddCoastlines`
   never converts. Split the `oceanNbr` counter by water type to confirm. If so, the fix
   is to extend coastline conversion to lake/swamp (worldgen) — but that changes generated
   data and may not apply to existing saves (coastline is baked into the save).

4. **Rough tile art / shading meeting the coastline.** rough may render unshaded or with a
   distinct tile that visually clashes with the grass-shore tile more than grass does.
   Split hill vs rough in the diagnostic; check `TypeShade`/`TypeDrawVert` for rough.

5. **Coastline sprite facing wrong next to rock** (pass 4 bitmask) — less likely since
   facing depends only on water neighbours, not land type, but verify the chosen facing
   for a known-bad hex.

---

## 7. Current code state

- GPU feather **reverted** to skipping open water (attempt #2 backed out). Receiver still
  excludes open water + coastline.
- `[SHOREDBG]` text diagnostic was **REMOVED 2026-06-05** (it wrote a log line per
  terrain rebuild — avoidable per-rebuild file I/O). The key numbers it produced are
  recorded in §4 above. Re-add a temporary counter if continuing the investigation.
- Build target: **x64 Debug** (`./build.ps1 -x64`). Game runs from `d:\Enemy Nations\`.
  Log path: `d:\Enemy Nations\SDL2Terrain.log`.

## 8. Working agreement (process)
- **Do not** drive the game with repeated screenshots to "verify" a visual change — the
  agent cannot reliably perceive deltas. Use **text diagnostics** (logs) for facts; ask
  the **user** to judge the visual and to provide a screenshot only when explicitly needed.
- One change at a time; confirm with the user before claiming a fix.
