# City Park — "part building" design

Status: **design only, not yet implemented.** Captures the decisions from the
2026-06 investigation so they survive into the implementation session. No game
code has been changed.

## What this is

A player-placeable **City Park**: a fixed **3×3** decorative patch that levels the
ground under it (like a building foundation), paints grass/plaza ground, and
scatters a few tree clusters on top. Unlike a building it has **no occupant, no
construction, no function** — it is cosmetic terrain that happens to be placed
with a building-like cursor.

It is the first instance of a more general idea — a **"part"**: reuse the
foundation/placement machinery to stamp art onto the map without a functional
`CBuilding` behind it. City Park is the concrete MVP of that idea.

### Player-facing rules (locked with the user)

1. **Fixed 3×3 footprint.** Not freeform, not variable-size.
2. **Auto-levels** the footprint, exactly like a building foundation. This is fine
   /expected — a park sits on flat ground.
3. **Not buildable on.** You cannot place a building (or another park) on park
   hexes.
4. **Passable.** Vehicles and infantry can drive/walk across it.
5. **Movement speed = dirt/plain, NOT road.** A park does not act as a road.

## The architecture decision: terrain stamp, *not* a `CBuilding`

The three gameplay properties (**not buildable**, **passable**, **dirt speed**)
are *exactly* the three knobs on `CTerrainData`:

```
m_iWheelMult[NUM_WHEEL_TYPES]   // movement speed per vehicle type  → dirt speed
m_iBuildMult                    // foundation cost                  → buildability
m_iDefenseMult[NUM_WHEEL_TYPES] // cover bonus (optional for a park)
```
([terrain.h:269-272](../enations_latest/src/terrain.h#L269))

A real `CBuilding` is the **wrong** base: a building flags its hexes `CHex::bldg`,
which makes them **block movement** — directly violating "passable." So the park
must be modeled as **terrain**, not a unit:

> **City Park = a new `park` terrain type + a few tree overlay sprites, stamped
> over a flattened 3×3 by a building-style placement command.**

It is a *hybrid*: building-like **placement UX**, terrain-like **semantics**. It
is strictly *less* code than a building (no construction stages, no materials, no
operate/power/people, no save of a unit object — just hex state, which already
serializes).

This mirrors the existing **Fields-around-farms** model
([fields-around-farms.md](fields-around-farms.md)): a cosmetic terrain type
(`CHex::fields`) painted onto hexes with a tree/crop overlay. City Park is the
same shape of feature, **player-placed instead of farm-driven**.

## The art (what's actually available)

Decoded 2026-06 from the original masters in
`EnemyNationsRevival/enations/data/`. Building sprite filename convention:
**1st char A/D/G = stage** (A=completed, D=skeleton, **G=foundation**),
**2nd char A/C/E/G = the 4 facings**.

| Source | Path | What it is | Use |
|---|---|---|---|
| **Tree clusters** ✅ | `data/EFFECT/tree/{0,1,4,5,6,7}` | 6 variants × 2 frames (`BA`/`CA`), 177×108, isolated + color-keyed. Dense clumps (0,1,4) → individual trees (6,7) → sparse scatter (5). | **Overlay sprites.** This is the engine's existing tree-on-terrain art (used by `forest`). Reuse directly. |
| **Grass / crops** | `data/TERRAIN/fields/{0,1,2}` | 128×64 hex tiles, green cultivated ground (+growth stages). | Candidate **ground tile** for the park surface. |
| **Office plaza pads** | `building/office{1,3,4,6,7}/G*010000.TGA` | Foundation stage = leveled ground *without* the building: concrete plazas, circular courtyards, walkways. office7 = circular plaza+paths; office6 = pool feature. | **Path/plaza ground**, but baked per whole-office footprint — would need cropping to tile-sized pieces. |
| Completed offices | `building/office{6,7}/A*010000.TGA` | Full park-with-building (office7 trees+plaza, office6 pool). | Reference look only; trees are baked in here. |

**Dead end:** the `city` *terrain* type art is **destruction rubble** (bombed
buildings), not pavement — do **not** use it for a park.

PNG renders of all of the above are in `D:/tmp/park/`.

### Art decision (recommended)
MVP surface = **grass (`fields`-style tile) + 2–3 `EFFECT/tree` clusters** placed
on the 3×3. Plaza/path pads from the office G-frames are a **phase 2** polish —
they need an art pass to cut the per-office composites into reusable tiles.

## How placement works (flow)

Reuses the building/bridge placement spine end-to-end.

1. **Toolbar mode + cursor.** New build-cursor mode that draws a 3×3 placement
   ghost and validity tint, modeled on the existing road/bridge cursors.
2. **Validation** (client preview + server authority). Reuse the footprint sweep
   in `FoundationCost` ([terrain.cpp:3046](../enations_latest/src/terrain.cpp#L3046)):
   slope limit (`MAX_ALT_CHANGE`), no overlap with buildings/water/existing park.
   Returns the flat target altitude for the 3×3.
3. **Command.** New `CNetCmd` subtype `place_park` modeled on `build_bridge`
   ([netcmd.h:133](../enations_latest/src/netcmd.h#L133) — enum is built to
   extend). Carries the UL hex + the computed altitude.
4. **Server applies, clients replay** (lockstep determinism — same as every
   terrain edit today):
   - **Level:** flatten the 3×3 vertices to the target altitude via the
     `fnEnumSetAlt` path used by `AssignToHex`
     ([new_unit.cpp:1434](../enations_latest/src/new_unit.cpp#L1434)).
   - **Retype:** set the 9 hexes' terrain type to `park`.
   - **Decorate:** assign 2–3 tree-overlay sprites at chosen hex offsets
     (deterministic — derive variant/placement from hex coords, not RNG, so all
     clients match).
   - **Invalidate** the 3×3 + neighbors for redraw (`SetInvalidated`).

## How the three properties are enforced

| Property | Mechanism | Touch point |
|---|---|---|
| **Dirt speed** | `park`'s `CTerrainData.m_iWheelMult[]` = copy of `plain`. Pathing/movement already read this per hex; nothing else to change. | terrain data row |
| **Passable** | Park hexes are **not** flagged `CHex::bldg`. Trees are pure overlay sprites — do **NOT** inherit `forest`'s infantry-block ([terrain.cpp:2791](../enations_latest/src/terrain.cpp#L2791)). | hex flags / collision |
| **Not buildable** | One guard in `FoundationCost`: reject `pHex->GetType() == CHex::park`, same pattern `river` already uses ([terrain.cpp:3174](../enations_latest/src/terrain.cpp#L3174)). | FoundationCost |

## Rendering

The `park` ground tile draws through the **normal terrain path** (it's a terrain
type, so the existing tile renderer + the GPU-terrain split handle it for free —
see [gpu-terrain-plan.md](gpu-terrain-plan.md)). Tree clusters draw as
**sprite-layer overlays** above terrain and below units — the same layer the
foundation already routes to when the split is active
([terrain.cpp:3551-3555](../enations_latest/src/terrain.cpp#L3551)).

## Save / load

**Free.** Terrain type and altitude are already serialized per hex with the map.
The tree-overlay placement must either be (a) re-derived deterministically from
hex coords on load, or (b) stored per hex. Prefer (a) — no new save format.

## Open questions

1. **Tree collision.** Confirmed approach: park is fully passable, trees are
   decoration only (do not reuse `forest` blocking). Flag if the user later wants
   trees to provide cover/slow movement — that's a `m_iDefenseMult` tweak, not a
   redesign.
2. **Ground tile choice.** Grass (`fields`) vs a cropped office-plaza tile for the
   MVP surface. Grass is zero-art-work; plaza needs cutting. → MVP = grass.
3. **Cost / limits.** Does a park cost materials or build time? Simplest MVP =
   instant + free (pure decoration). Revisit if it should be an economy/morale
   item.
4. **Does it count as "city"?** The original has a `city` building/terrain concept
   for AI civ placement. Decide whether player parks interact with that or are
   inert.

## Implementation phases

- **P0 — terrain slot.** Add `park` to the `CHex` terrain enum
  ([terrain.h:86](../enations_latest/src/terrain.h#L86)); author its `CTerrainData`
  row (wheel mults = plain, mark non-buildable). Reuse a grass tile for art.
  *Validate: a hand-set park hex renders, is passable at dirt speed, rejects
  building.*
- **P1 — stamp command.** `place_park` `CNetCmd` + server handler that levels +
  retypes the 3×3. *Validate: place via a debug key, survives save/load, syncs in
  a 2-player game.*
- **P2 — placement UX.** Toolbar entry + 3×3 ghost cursor + validity tint.
- **P3 — decoration.** Deterministic tree-cluster overlays from `EFFECT/tree`.
- **P4 — polish (optional).** Crop office G-frame plaza/path tiles for nicer
  surfaces; cost/morale hooks.

## Relationship to terraforming (the other feasibility study)

City Park and player **terraforming** (raise/lower/level) share the same spine —
server-authoritative command → mutate the hex grid (altitude and/or type) →
invalidate → redraw. The park's **auto-level** step *is* a constrained terraform.
If terraforming lands first, the park reuses its level primitive; if the park
lands first, it proves the command/stamp pattern terraforming will generalize.
See the terraforming notes for the altitude system (`CHexCoord::Flatten`,
`FoundationCost` cut/fill, the bridge `m_iAlt` network precedent).
