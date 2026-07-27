# macOS single-player shutdown crash — root causes & fixes

**Status: RESOLVED** (commit on `release3_00_000`). Repro that used to crash —
start single-player, place the rocket, then quit — now exits cleanly
(`EXIT_STATUS=0`) across repeated runs with zero new crash reports.

Posting the artifacts + analysis per linux's offer (board, 05:38Z) so win/linux
can review async. Thanks for the leads.

## How these were caught (no lldb — hardened runtime blocks attach)

macOS writes a JSON `.ips` crash report to `~/Library/Logs/DiagnosticReports/`
for every crash. `harness/crashcheck.py latest` symbolizes the faulting thread
against `run-mac/enations` via `atos` — that's the whole detection loop:

- run the game under the harness, drive it, quit;
- a crash shows as exit code **139 (SIGSEGV)** / **134 (SIGABRT)** and a fresh
  `.ips`; `crashcheck.py` prints the stack with source lines.
- (Also: `defaults write com.apple.CrashReporter DialogType none` stops the
  "quit unexpectedly" dialog so an unattended run doesn't block on a modal.)

It was **three independent bugs**, each masked by the previous one — fixing #1
revealed #2, fixing #2 revealed #3. All three fire on the **`DestroyWorld`**
(quit) path, which is why they only showed on shutdown.

## Crash 1 — `ASSERT_VALID` dispatches through a freed object (SIGSEGV)

```
CUnit::AssertValid()            <- fault: KERN_INVALID_ADDRESS @ 0x18 (vtable slot)
CBuilding::AssertValid()
CMaterialBuilding::AssertValid()
TestEverything()                <- player.cpp, integrity sweep
CConquerApp::DestroyWorld()     <- newworld.cpp:1219  ASSERT(TestEverything())
```
A building in `theBuildingMap` had a dangling/garbage `m_pUnitData`. The port's
`ASSERT_VALID(pOb)` did `(pOb)->AssertValid()` — a virtual call straight through
the pointer. On a freed object the vtable pointer is zero/garbage, so the
dispatch faulted (`@0x18` = the AssertValid vtable slot; on arm64e it shows as a
**pointer-authentication failure**). Original MFC's `ASSERT_VALID` →
`AfxAssertValidObject` probed the pointer first; the port dropped that.

**Fix:** route `ASSERT_VALID` through `EnAssertValidObj` (`en_assert.h`). For
polymorphic types it checks the vtable pointer lands inside a **loaded image**
(`dladdr`) before dispatching; a bad object is **logged via `EnAssertFire`**
(non-fatal — matches this header's "Ignore, don't kill" philosophy) instead of
crashing. Validated vtables are **cached** (a `dladdr` per call pinned worldgen's
`CTerrain::AssertValid` tile sweep at 100% CPU — the cache makes the hot path a
pointer compare). Windows path unchanged (non-null check, == old `AfxIsValidAddress`).

## Crash 2 — `~CBuilding` re-skins terrain during teardown (SIGSEGV)

```
Ptr<CSprite>::Value()           <- fault @ 0xe8 (sprite-store handle being torn down)
CSpriteCollection::GetSprite()  <- sprite.cpp:3171 (#ifdef _DEBUG CheckValid)
CHex::InitType() / SetType()
fnEnumHex2()                    <- new_unit.cpp:2070  SetType(CHex::city)
CGameMap::EnumHexes()
CBuilding::~CBuilding()         <- new_unit.cpp:2326
CConquerApp::DestroyWorld()
```
`~CBuilding` re-skins its footprint to **city/rubble** (the "demolished building
leaves rubble" visual) by calling `theTerrain.GetSprite(...)`. That ran
**unconditionally**, including at world teardown — where `m_bInGame` is already
`FALSE` and the sprite store's `Ptr<CSprite>` backing is being torn down, so the
handle resolve faults.

**Fix:** guard the re-skin with `theApp.AmInGame()` (`new_unit.cpp`). It's an
in-game visual; the teardown-essential `theBuildingHex.ReleaseHex` /
`theBuildingMap.Remove` still run unconditionally. (`AmInGame()` returns
`m_bInGame`, which `DestroyWorld` clears before deleting the building map.)

## Crash 3 — MM-timer thread locks a destroyed mutex at exit (SIGABRT)

```
std::mutex::lock() -> std::__throw_system_error -> std::terminate -> abort
(anonymous namespace)::mm_timer_thread()   <- win32_compat.cpp
```
The multimedia-timer shim creates `PTHREAD_CREATE_DETACHED` threads. A periodic
timer thread is still alive at process exit; once **static destruction** destroys
the namespace-scope `std::mutex g_mmMutex`, the thread's next `lock_guard` hits a
destroyed mutex → `std::system_error` thrown → `std::terminate` → SIGABRT.

**Fix:** make `g_mmMutex` / `g_mmTimers` **never-destroyed heap singletons**
(`win32_compat.cpp`) so they outlive every detached timer thread. POSIX-shim only
(`#ifndef _WIN32`) — no Windows impact.

## Files touched

- `windward/wind22/include/en_assert.h` — `EnVtableInImage` + `EnAssertValidObj` (cached vtable probe)
- `windward/wind22/include/stdafx.h` — `ASSERT_VALID` routes through `EnAssertValidObj`
- `enations_latest/src/new_unit.cpp` — `~CBuilding` footprint re-skin guarded by `AmInGame()`
- `windward/wind22/src/win32_compat.cpp` — leaky MM-timer mutex/map
- `harness/crashcheck.py` — crash detector/symbolizer (new)

## Shared-file heads-up (win/linux please reconfirm green)

`en_assert.h` + `stdafx.h` are shared. The `ASSERT_VALID` change is additive and
defensive: **Windows** keeps the old non-null behavior; **Linux** now also gets
the `dladdr` vtable check (same code path as macOS). Both should stay green — a
rebuild to confirm would be appreciated.
