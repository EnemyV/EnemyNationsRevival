# WinAstra: isolated AI regressions

These tests compile production function bodies extracted verbatim from the source.
The surrounding game world is a small, single-threaded fixture. They establish
specific algorithm regressions, not gameplay, race safety, or the cause of a
particular saved game's stall.

Requirements: Python 3 and Visual Studio 2022 Community C++ tools at the path in
the runners. Each runner writes only to `tests/ai/seek-scan-out/`.

```powershell
python tests/ai/run-seek-scan-test.py --baseline
python tests/ai/run-seek-scan-test.py
python tests/ai/run-ship-abort-test.py --baseline
python tests/ai/run-ship-abort-test.py
```

Baseline is pinned to `5cb9e069`; override with `--baseline-ref REV`. Baseline runs
are expected to exit 1, with 10 targeting failures and 5 ship-abort failures.
Candidate runs must exit 0. A compilation failure exits 2.

The targeting fixture preserves the important real-world accessor distinction:
spatial maps retain dying units, while the ID maps reject them. A previously
known enemy remains in the AI unit cache. The original nearest scan can keep
selecting that enemy; actual `SeekOpfor` rejects it and retries in the same call.
A fixture-only 100,000-lock-call ceiling detects nonreturning cases; it is not
compiled into the game. The candidate returns normally in all six task/target
combinations. Live-target and exclusion checks run in the same suite.

The ship fixture covers both `UnassignTrucks` overloads and both branches of the
building overload. It also checks absent truck records, idempotence, unrelated
pairings, owner isolation, dying ships, cargo on the truck, cargo on the ship,
and an active unloading leg. It does not simulate asynchronous network loading.

Actual game build validation uses the repository wrapper:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ./build.ps1 -Release -x64 -Quiet
```

Before accepting either patch, the runtime tester must exercise the trigger in
the real game and verify normal live targeting or ship loading/unloading still
works. In particular, this suite does not prove that dying targets caused the
recorded traffic-soak scan storm.
