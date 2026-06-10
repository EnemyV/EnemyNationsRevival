# Standalone AI logic tests

A tiny, **fully isolated** test suite for the AI's staging unit-classification
logic. It is self-contained in this folder and **does not touch the game, the
game build, or CMake** -- it compiles a header-only, dependency-free module with
`cl.exe` directly. Safe to develop and run while the main game is being worked on.

## Run

```powershell
& '.\tests\ai\run-ai-tests.ps1'
```

Exit code: `0` = all pass, `1` = a test failed, `2` = toolchain/compile error.
Build artifacts go to `d:\tmp\aitests\` (nothing is written into the source tree
or the game build dirs).

## What it covers

The pure decision-table logic across `AssignTask`, `GetNavyTask`,
`IsStagingCompete`, `ContinueStaging`, `GetStagingArea`, and `LoadCargo` -- the
class of bug where the same unit type / goal is classified differently in
different functions. Currently ~186 checks:

- amphibious-assault eligibility (`{med_tank, rangers}` only)
- SEAINVADE / LANDWAR / PIRATE+SEAWAR staging buckets (the canonical table that
  all four production switches must equal)
- LANDWAR == ADVDEFENSE table equality (whole-enum sweep)
- bucket -> `CAI_TF_*` param-slot alignment for every goal family
- bucket bounds (every classified unit lands in `[0, STAGING_UNITTYPES)`)
- `CAI_*` material slots == `CMaterialTypes` order (the cai.h contract that
  shipped as commented-out `ASSERT`s)
- **core invariant:** anything routed to a sea invade must be able to board a
  landing craft (assignment subset of loadable) -- the rule the original
  `light_tank`/`light_art` bug broke
- **sizer/counter goal-set invariant** -- see "Latent bug found" below

### Behavioral simulation

A tiny pure state-machine (`ai_staging_sim.h`) models the staging-completion
loop (`IsStagingComplete` + a rendezvous driver) so behavior can be tested, not
just the static tables. Scenarios:

- a normally-sized amphibious force converges and launches with real units
- over-supply of one bucket still completes
- a required bucket that cannot be filled **stalls** the whole assault (the
  generalized form of the "re-enabling infantry adds a required type that may be
  unavailable" risk)
- the SEAWAR sizer bug's runtime consequence: zero requirements + a staging area
  => `IsStagingComplete` declares "complete" with **zero units** (empty launch)

It is a faithful model of the algorithm, not the live game loop (which needs the
world singletons + threads); see "What it does NOT cover".

## SEAWAR sizer gap: RETRACTED after data verification (2026-06-09)

An earlier revision recorded a `KNOWN_BUG`: "`GetStagingArea` sizes only
`IDG_PIRATE` while the completion counters also classify `IDG_SEAWAR` -> a
SEAWAR staging task gets zero requirements -> empty launch." Before fixing it,
**parsing the shipped `stdgta.dat`** (40 goals / 75 tasks; format per
`CAISavLd::LoadBinaryData`) disproved the premise:

- `IDT_PREPAREWAR` (2325) is attached to exactly **{LANDWAR 1018,
  ADVDEFENSE 1022, SEAINVADE 1033, PIRATE 1034}** — precisely the goals
  `GetStagingArea` sizes.
- **`IDG_SEAWAR` (1019) owns no staging task** (its list: make-gunship /
  shipyard / cruiser / destroyer / seek-at-sea / patrol / escort), so a SEAWAR
  `IDT_PREPAREWAR` task cannot exist and nothing was missing. The counters'
  `PIRATE || SEAWAR` grouping is dead-defensive code; the *"only 2 ocean based
  staging tasks"* comment (caigmgr.cpp:7529) matches the data.

The suite now asserts the data-grounded invariant instead
(`test_sizer_covers_data_staging_goals`): every goal the data attaches
`IDT_PREPAREWAR` to must be handled by both the sizer and the counters —
which **passes**. Lesson recorded: verify task/goal reachability against the
data file before declaring a code-path bug; the proposed "fix" would have
added support for a pairing that never occurs.

## Dead / unimplemented code (documented, not testable in isolation)

These were verified by tracing every reference in the game source, not by an
isolated unit test (they are "this code path is disconnected / was never
written" facts, not decision-table invariants the harness can reach). Recorded
here so the findings aren't lost.

- **`IDT_ESCORT` is never assigned (escort behavior is dead).** A full
  escort-assignment function exists, `CAIGoalMgr::GetSupport`
  ([caigmgr.cpp:8955](../../enations_latest/src/caigmgr.cpp#L8955)) -- units rush
  to defend an attacked ally -- but its **only call site is commented out**
  ([caigmgr.cpp:742](../../enations_latest/src/caigmgr.cpp#L742)) and its header
  decl is commented. `GetCombatTask` explicitly skips `IDT_ESCORT`
  ([caitask.cpp:623](../../enations_latest/src/caitask.cpp#L623), "no code handles
  this yet"), and there is **no live `SetTask(IDT_ESCORT)`** anywhere. So escort
  is built-then-unplugged: never assigned; if it ever were, it only falls into
  generic `StageUnit`, not real convoy escorting.

- **`FindMarineStagingArea` was never implemented.** A whole-tree search finds
  only a single **commented call** ([caigmgr.cpp:7790](../../enations_latest/src/caigmgr.cpp#L7790))
  -- there is no function definition anywhere. Shore-adjacent amphibious staging
  does not exist; sea invades use whatever generic location `GetNewStagingArea`
  produces.

## Notes / refined understanding (verified, not bugs)

- **Fuzzy launch is off** (commented block in `IsStagingCompete`,
  [caitmgr.cpp:3793](../../enations_latest/src/caitmgr.cpp#L3793)), so a *staged*
  task force (`IDT_PREPAREWAR`) must be 100% assembled to launch. This does NOT
  stop the AI attacking with partial forces -- `CAIGoalMgr::AttackPlayer`
  reassigns patrol units to attack independently of staging.
- **Message priority is unused** (`m_iPriority` read for ordering only in the
  dead `#if 0` `PrioritizeMessage`; processing is FIFO). But weapon firing is
  **synchronous** (`AiOppoFire`->`AutoFire`, [ai.cpp:667](../../enations_latest/src/ai.cpp#L667)),
  not queued -- so the AI fires back immediately; only higher-level strategic
  reactions are FIFO-delayed.
- **Infantry DO attack** (force-zero only excludes them from the *staged*
  combined-arms task force; they still attack via patrol-reassignment / defense /
  auto-fire). The AI's *core* combat loop is live; its *combined-arms staging
  doctrine* is the heavily-disabled part.

## Concurrency model tests (separate, opt-in)

```powershell
& '.\tests\ai\run-ai-concurrency.ps1'
```

`test_ai_concurrency.cpp` uses **real `std::thread`** to exercise faithful models
of the AI thread patterns (it does NOT run the real `CAIMgr::Manage`):

- **message queue** — producer/consumer with the `tmp -> main` FIFO hand-off and
  the backlog gauge (`g_aiMsgBacklog`): no lost/duplicated messages, gauge
  returns to 0, per-producer FIFO preserved (mirrors the stub `PrioritizeMessage`).
- **shared path map** — readers vs. a rebuilder that holds the lock across the
  whole teardown+rebuild (the `thePathMap` fix), with a **mock-pathing sleep**
  inside the critical section to widen the race window: a locked reader never
  sees a half-rebuilt map.
- **stuck-vehicle timeout** — the 5-min / 10-min thresholds via a **mock clock**
  (deterministic, no sleep), including the faithful boundary quirk that an
  *exact* 300000/600000 ms interval triggers no action.

**Hard caveats (don't over-trust a green run):**
- These test the **model**, not the game code. A pass means "this locking
  discipline is race-free under stress," not "`CAIMgr::Manage` is correct."
- MSVC has no usable ThreadSanitizer, so race detection here is **stress-based
  (probabilistic)**, not sound. (Verified the scaffolding *can* surface a race: a
  temporary unsynchronized two-atomic writer was caught with ~10^8 torn reads.)
- `sleep` is used only to widen race windows, never to assert on timing; timeout
  logic uses a mock clock.

## What it does NOT cover

Anything stateful or world-coupled: staging convergence/completion, the threaded
`Manage()` loop, router truck/ship movement, pathing, the message queue. Those
need the live game singletons (`theGame`, `theMap`, `theVehicleMap`, locks) and
are out of scope for a standalone tester.

## Files

| File | Purpose |
|------|---------|
| `ai_staging_logic.h` | Pure logic module (mirror enum + free functions). Executable spec of the canonical unit sets. |
| `ai_staging_sim.h` | Pure state-machine model of staging completion (`IsStagingComplete` + rendezvous driver) for behavioral tests. |
| `microtest.h` | `CHECK` / `CHECK_EQ` / `KNOWN_BUG` harness, no dependencies. |
| `test_ai_staging.cpp` | Pure logic + simulation test cases. |
| `test_ai_concurrency.cpp` | Threaded concurrency model tests (real `std::thread`). |
| `test_ai_data.cpp` | Parses the SHIPPED `stdgta.dat`; asserts integrity + mirror agreement. |
| `test_ai_paths.cpp` | Source-lint of the 4 research-path arrays in caigmgr.cpp: no duplicate topics, tier chains complete (`range_1/2/3`), `landing_craft` present. Caught + now guards the 30-year `range_2`-missing bug. |
| `run-ai-tests.ps1` | Compiles & runs the pure/sim suite + the data suite. |
| `run-ai-concurrency.ps1` | Compiles & runs the threaded suite. |
| `run-ai-smoke.ps1` | **Runtime smoke gate**: launches the real game under dbgcatch with `EN_PERF=1`, loads a save, asserts AI-health invariants from perf.log (queue gauge >= 0, queue drains, snapshot miss < 5%, no AVs, no crash). |

## Data semantics learned from the data suite

Duplicate task ids within a goal's task list are **intentional — they encode
quantity** (each listed id becomes one assignable task instance). Shipped data:
`IDG_BASICFEED` lists `IDT_BUILDFARM` x3 (= build three farms), `IDG_CARGOSHIP`
lists `IDT_MAKELCARGOSHIP` x3. The data suite asserts multiplicity only appears
on ORDER-type tasks.

## Keeping it in sync

`ai_staging_logic.h` **mirrors** the production logic; it does not link it (the
real `CTransportData` enum lives in `vehicle.h`, which pulls in the whole
game/window/net layer). If the production decision tables or the
`CTransportData::TRANS_TYPE` enum change, update `ai_staging_logic.h` to match.
The `static_assert` blocks in that header pin the load-bearing enum values so an
accidental edit fails the compile. Source references for every mirrored table are
in the header comments.
