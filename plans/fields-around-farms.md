# Fields grown around farms — design note

Status: **design only, not yet implemented.** Captures the decisions from the
2026-06 investigation so they survive into the implementation session. No game
code has been changed.

## What this is

The original 1996 game reserved a complete-but-unwired feature: when you build a
food farm, the farmable hexes around it visually convert to cultivated **Field**
terrain, and the crop animates as it grows. The art, the terrain slot, the data
tuning, and the player-facing strings all shipped — only the gameplay code that
assigns Field to hexes and ticks the growth was never written.

### Evidence the feature was real (all original, found across 5 source copies)
- `CHex::fields` is terrain type **13**, commented `// grown around farms`
  ([terrain.h:82](../enations_latest/src/terrain.h#L82)). Present identically in
  the original dump (`EnemyNationsRevival/enations/src`, uppercase `FIELDS`) and
  every fork.
- Art is authored and **compiled into the shipping archive**: `TERRAIN.MIF`
  packs `fields\0..2` as terrain art ID 13 in all zoom/bit-depth blocks.
- Player strings exist: name **"Field"**, description **"Wheat field for farm"**
  (`data/LANGUAGE/9/terrain.prn`).
- Data row is fully tuned (`data/UNITS/terrain.prn`, decoded against
  [sprtinit.cpp:83](../enations_latest/src/sprtinit.cpp#L83)):
  `BuildMult 1, FarmMult 10` (= Plain, the best farmland), plus its own 5 wheel
  + 5 defense mults.
- Referenced **nowhere** in code except the enum declaration — never assigned to
  a hex, never drawn.

### What the art actually is (decoded from the pixels)
`fields/0` is **12 full-hex tiles** = a **3 × 4 matrix**, not 3 flat variants:
- **Rows (A / D / G) = growth stage.** D = young (bare furrows, few sprouts) →
  G = growing (furrows greening) → A = mature (dense green, furrows buried).
- **Columns (A / C / E / G) = rotation.** Furrow direction alternates per column
  (4 rotation states collapsing to 2 visible furrow diagonals, since furrows are
  symmetric lines). Pre-baked in the art (not engine `MakeRotated`), so each
  growth stage carries its own 4 rotations.

`fields/1` and `fields/2` are one tile each — extra crop/variety looks.

PNG renders of all 14 tiles + montages are in `D:/tmp/fields_png/`.

## Design decisions (locked)

1. **Fertility is sacred and is saved.** Converting a hex to Field must NOT lose
   the original terrain's fertility. Store the original terrain per hex in a
   serialized side-table; all yield reads through to it.
2. **Plots are a cosmetic overlay.** The Field surface never feeds the yield
   math — yield always comes from the stored original terrain.
3. **One shared growth stage per farm**, driven by the farm's own fertility
   snapshot. Better fertility → faster growth.
4. **Only place on farmable hexes.** A hex is farmable iff its original terrain's
   `GetFarmMult() > 0`. This auto-excludes rocks/water/road/mountain/city (all 0)
   with no special-casing — "don't place it on rocks" for free.
5. **Food farms only.** Lumber mills (the other `UTfarm`) read forest hexes via a
   different summer and are out of scope.

### The single-number principle
The averaged fertility (`m_iTerMult`, already snapshotted at placement) decides
**where** fields appear, **how much** the farm yields, and **how fast** plots
grow. One source of truth.

## How it maps to existing code

### Fertility already snapshotted at placement — no new work
`CFarmBuilding::UpdateFarm()` ([new_unit.cpp:3690](../enations_latest/src/new_unit.cpp#L3690))
runs at placement and on load (called from
[new_unit.cpp:2982](../enations_latest/src/new_unit.cpp#L2982)) and stores the
averaged land fertility in `m_iTerMult` via `LandMult()`. That value already
scales output in `BuildFarm()`
([mainloop.cpp:2418](../enations_latest/src/mainloop.cpp#L2418)). Growth rate is
just a function of this existing value.

### Yield read-through (decision 1 & 2) — the one correctness change
In `fnFarmFromGround` ([new_unit.cpp:3646](../enations_latest/src/new_unit.cpp#L3646)),
if a hex is `CHex::fields`, look up its stored original type and sum *that*
terrain's `GetFarmMult()` instead. Then `LandMult()` averages real soil even
after the surface is painted. (Plain→Field is already neutral at 10→10; this
makes desert/swamp/hill correct too.)

### The side-table (decision 1) — clone of `theMinerals`
Model on `CMineralHex` ([minerals.h:71](../enations_latest/src/minerals.h#L71)):
a `CMap<DWORD,DWORD,…>` keyed by the packed hex coord, with its **own
`Serialize` override** — the MFC-compat `CMap::Serialize` is a no-op, so without
the override the table silently never persists (the documented minerals/bridge
trap, [minerals.h:80](../enations_latest/src/minerals.h#L80)). Serialize it next
to `theMinerals.Serialize(ar)` in
[player.cpp:2768 / 2923](../enations_latest/src/player.cpp#L2768), **behind a
save-version gate** so existing saves still load (the flat `CArchive` stream
shifts if a new block is inserted ungated).

### Per-hex save record (stays tiny)
| field | why | bytes |
|---|---|---|
| `originalTerrain` | fertility read-through + clean revert | 1 |
| `ownerFarmID` | find the farm; ref-count on overlapping rings | 4 |

Everything else **derives, so it is not serialized**:
- **rotation** (which of 4 furrow orientations) = deterministic hash of the hex
  coord → identical on every client, survives reload, no storage.
- **growth stage** = computed at draw time from the owner farm's state (one
  shared stage per farm), so it's automatically correct after load because the
  farm is already serialized.

### Rendering
Field sprite for a hex = `GetSprite(CHex::fields, index)` where the index selects
{growth stage, rotation}. Stage = shared per-farm value from `m_iTerMult`-driven
timing; rotation = hash(hex). Repaint on stage change invalidates only the farm's
plots.

## Multiplayer / determinism
Terrain type is sim state, not just cosmetics. Field conversion and stage ticks
must be deterministic across clients: derive rotation from hex coords (not
`RandNum`), and tick growth from synced game state / the farm's own counters —
mirroring how road building is networked and `ChangeToRoad` runs locally and
deterministically on each client.

## Open / deferred
- Exact growth-rate curve: `stage_time = BASE / m_iTerMult` vs. tying stage
  directly to the farm's production accumulation (`m_iBuildDone / GetTimeToFarm`).
  The latter needs zero new tuning and makes the field a literal production
  progress-bar, but pauses when the farm isn't producing. Pick during impl.
- Ring shape/size of the "grown" area around the footprint.
- Whether `fields/1` & `fields/2` are used as alt crops or dropped.
- Save-version constant + back-compat path for pre-feature saves.

## Effort
~1.5 days. The side-table is a copy of `CMineralHex`; most of the risk is the
save-version gating (don't brick existing saves) and determinism. The fertility
read-through is a few lines. Growth-by-fertility is nearly free because
`m_iTerMult` already exists and already scales production.
