# Economy mechanism checks

Standalone, like `tests/ai` and `tests/traffic`: `cl.exe` is invoked directly on the
shipped function bodies plus a small fake scene. Nothing touches the game build, CMake,
or a running game, and no artifact is written inside the source tree.

```powershell
python tests/econ/run-scrounge-cache-test.py
python tests/econ/run-scrounge-cache-test.py --baseline-ref 639610a9   # expected to FAIL
```

Build artifacts go to `d:\tmp\scroungecache\<Od|O2>\` (`--out-dir` to move them).
Exit codes: `0` all pass, `1` a check failed, `2` toolchain / compile error.

## Scrounging terrain cache vs Slash and Burn

`CWarehouseBuilding` caches four terrain multipliers (`m_iScrForest`, `m_iScrSoil`,
`m_iScrIron`, `m_iScrCoal`) behind a negative sentinel, and `AltOutput::MultiLinesFor`
turns them into the warehouse's actual per-minute output lines. Slash and Burn retypes
forest hexes to plain underneath them, so `CFarmBuilding::ApplySlash` has to invalidate
every warehouse whose scan ring covers the cut hex. Before that invalidation existed, a
warehouse kept producing from the pre-cut forest/scrap/soil mix until something else
rebuilt the cache — a save/load would silently change the yield.

The runner extracts the production bodies **verbatim** (the `tests/traffic` pattern):
the scan weight functions, `LumberBox`, `ForestMultAt` / `SoilMultAt` /
`ScroungeSoilMultAt` / `ScrapMultsAt`, `LandMult` / `UpdateFarm`, `UpdateScrounge` /
`InvalidateScrounge` and the four getters, `SlashHex`, `ApplySlash`, and
`MultiLinesFor` from `altoutput.cpp`. It prints their SHA256 so a run can be tied to a
source state. Only the world around them is fake, because the real `theMap`,
`theTerrain`, `theStructures` and `theBuildingMap` pull in the whole game/window/net layer.

What it checks (754 assertions, run at `/Od` and `/O2`):

- **instrument check first** — retype hexes with `SlashHex` alone, with no invalidation,
  and confirm the getters still hand back the pre-cut answer. Without this, every
  "cache equals a fresh scan" assertion below could be passing vacuously.
- one cut refreshes **all four** multipliers, and moves all four in the right direction
  (forest cover down, scrap iron and coal down because forest feeds both, scrounge-soil
  up because forest and plain have different `FarmMult`).
- one cut does **not** yet move the rounded per-minute rate — the explicit demonstration
  of why a rate-level test has to cut more than once.
- 27 cuts cross a rounding threshold on **every** line (lumber 1→0, food 2→3, iron 1→0,
  coal 1→0), and the cache still equals a fresh scan of the cut terrain. The pre-cut
  values are then written back by hand to show the defect was visible in the *rate*, not
  only in the multiplier.
- **control:** a cut far outside every ring leaves the cache alone. The fields are
  poisoned with recognisable values first, because an invalidate-and-rescan would land on
  the same numbers and be indistinguishable from not touching them.
- the mill refresh and the warehouse invalidation share one walk over `theBuildingMap`
  and neither breaks the other: after the same cuts, the mill's `m_iTerMult` equals a
  fresh `LandMult` *and* the warehouse cache equals a fresh scan.
- a cut inside the warehouse's forest ring but outside its narrower scrap/soil rings —
  over-invalidation is harmless and the narrow rings come back with their correct
  (unchanged) values.
- **exhaustive sweep:** every hex in a 27x27 window around both buildings, cut one at a
  time from a fresh scene, must leave the cache agreeing with a fresh scan. 716 hexes are
  actually cut, 68 of them move at least one multiplier. This is the general form of the
  finding: it fails for any hex whose rings the reach test misses, whatever the ring
  geometry, and for any fix that invalidates only some of the four fields.

`--baseline-ref 639610a9` (the pre-fix integration tip) fails 85 of the 754 checks,
including every rate-threshold check and 68 hexes of the exhaustive sweep.

## Caveats

- The terrain `FarmMult` table here is a **stand-in**. The shipped one lives in
  `ENATIONS.DAT`, not in source. Only its shape is load-bearing (forest and plain differ,
  city/road/water are 0, rough beats forest); every assertion compares a cached answer
  against a fresh scan through the same table, so the absolute numbers printed are
  fixture numbers, not game numbers.
- This is a mechanism test. It does not prove a live game produces the right lumber, that
  the UI rate readout agrees, or that two MP clients converge — the netapi `hex_retype` RX
  path reaches the same `ApplySlash`, but that is read from the source, not exercised here.
