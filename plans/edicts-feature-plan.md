# Edicts — Feasibility Study & Implementation Plan

> **Status:** Design / feasibility (2026-06-07). Not yet started.
> **Verdict:** Highly feasible. Most of the machinery already exists; an edict is
> essentially a *player-toggleable, upkeep-costing research upgrade*. Estimated
> **~3–6 days** for a polished v1 with a handful of edicts (human-only; AI deferred).

## 1. Concept

Edicts are civilization-wide modifiers the player toggles on/off from an
administrative building. Each active edict applies a global bonus (and usually a
matching penalty) and charges a continuous upkeep in **energy**, **food**, and/or
**workforce**. Presented as a checkbox list.

Example edicts (from the original ask):

| Edict | Effect | Cost |
|---|---|---|
| **Fortify the Border** | +50% fort construction speed | +20% energy usage |
| **Nutritional Plenitude** | +20% population growth | +50% food consumption |
| **Mining Subsidies** | +25% mine production | +50% mining energy, +25% mining workforce |
| **Research Subsidies** | +25% research speed | +50% research energy, +25% research workforce |

## 2. Why this is cheap: the engine already has the levers

Every core mechanic an edict needs already exists and is **already serialized and
already modified at runtime** (by research upgrades and difficulty). An edict is a
new *source* of the same kind of modification.

### 2.1 Global production multipliers (the "bonus" half) — VERIFIED

`CPlayer` carries per-player productivity floats (default `1.0`), each consumed
through a single accessor at a single chokepoint:

| Edict lever | Field ([player.h:497-509](../enations_latest/src/player.h#L497-L509)) | Accessor | Sole consumer |
|---|---|---|---|
| Construction speed | `m_fConstProd` | `GetConstProd()` | [vehicle.cpp:938](../enations_latest/src/vehicle.cpp#L938) (crane build), [mainloop.cpp:1388](../enations_latest/src/mainloop.cpp#L1388) (demolish) |
| Mining | `m_fMineProd` | `GetMineProd()` | [mainloop.cpp:2416](../enations_latest/src/mainloop.cpp#L2416) |
| Farming | `m_fFarmProd` | `GetFarmProd()` | [mainloop.cpp:2486](../enations_latest/src/mainloop.cpp#L2486) |
| Manufacturing | `m_fManfProd` | `GetManfProd()` | [mainloop.cpp:1826](../enations_latest/src/mainloop.cpp#L1826) |
| Materials (smelt/refine) | `m_fMtrlsProd` | `GetMtrlsProd()` | [mainloop.cpp:2232](../enations_latest/src/mainloop.cpp#L2232) (in `BuildMaterials`, fn starts :2220) |
| Population growth | `m_fPopGrowth` | `GetPopGrowth()` | [player.cpp:653](../enations_latest/src/player.cpp#L653) |
| Food consumption rate | `m_fEatingRate` | `GetEatingRate()` | [player.cpp:585](../enations_latest/src/player.cpp#L585), [:588](../enations_latest/src/player.cpp#L588) |
| Combat attack/defense | `m_fAttack` / `m_fDefense` | `GetAttackMult()` / `GetDefenseMult()` | [new_unit.cpp:1181](../enations_latest/src/new_unit.cpp#L1181), [:1185](../enations_latest/src/new_unit.cpp#L1185) |

**Precedent:** research upgrades already do exactly `m_fConstProd *= 1.5` etc. at
[player.cpp:504-528](../enations_latest/src/player.cpp#L504-L528), and difficulty scales
them at [player.cpp:297-305](../enations_latest/src/player.cpp#L297-L305). An edict is
the same kind of modifier, just player-controlled and reversible.

**Application strategy — fold into the accessors, do NOT mutate the base field.**
Mutating `m_fConstProd` directly would race with research/difficulty and make
toggling-off lossy. Instead keep an independent edict multiplier and combine it in
the accessor:

```cpp
float GetConstProd() const { return m_fConstProd * m_fEdictConstMult; }
```

Because every consumer goes through `GetProd()` / `GetFrameProd()` wrappers
([building.inl:253-276](../enations_latest/src/building.inl#L253-L276)) which already
multiply in the power/people/damage throttles, the edict bonus composes correctly with
everything else for free.

### 2.2 Upkeep (the "cost" half) — VERIFIED, and it maps cleanly

There is **no calendar / "month"** in the engine — time is per-second/per-minute ticks
(`GetOperSecElapsed()`, `GetOpersElapsed()`). That is *good*: two of the three costs are
naturally **continuous demand**, which fits the engine better than a periodic lump sum.

- **Energy** is not a stockpile — it's a per-loop **demand** total. Buildings call
  `AddPwrNeed()` ([player.h:280](../enations_latest/src/player.h#L280)); `StartLoop`
  recomputes `m_fPwrMult = have/need` ([player.cpp:397](../enations_latest/src/player.cpp#L397))
  which throttles **all** production when underpowered. Edict energy cost = add to
  `m_iPwrNeed`.
- **Workforce** is identical: `m_iPplNeedBldg` demand → `m_fPplMult = have/need`
  ([player.cpp:393](../enations_latest/src/player.cpp#L393)). Edict workforce cost = add to
  people-needed (it competes with buildings for the labor pool).
- **Food** *is* a real stockpile (`m_iFood`), drained per-second in `PeopleAndFood`
  ([mainloop.cpp:911](../enations_latest/src/mainloop.cpp#L911) → [player.cpp:585](../enations_latest/src/player.cpp#L585)).
  Edict food cost = extra drain there.

`StartLoop` clears the demand totals at
[player.cpp:424-427](../enations_latest/src/player.cpp#L424-L427) and they are re-accumulated
by every building's `Operate`. **The clean upkeep hook is right after that clear** — add
each active edict's energy/workforce upkeep alongside the buildings, so the existing
`m_fPwrMult`/`m_fPplMult` throttling absorbs it automatically.

> **Design reframing to confirm with stakeholder:** "monthly cost" is implemented as
> **continuous upkeep** (a permanent extra power draw + labor demand + food drain while
> the edict is on). This is cheaper and more consistent with the engine than a
> lump-sum bill. If a literal periodic charge is required, it can be layered on a
> 60-second accumulator, but it's not recommended for v1.

### 2.3 Category-specific bonuses (e.g. forts only) — VERIFIED

"Fortify the Border" must boost *fort* construction, not all construction.
`CVehicle::ConstructBuilding` ([vehicle.cpp:929](../enations_latest/src/vehicle.cpp#L929))
has the target building `m_pBldg`, so:

```cpp
float fConst = GetOwner()->GetConstProd();
if (m_pBldg->GetData()->GetBldgType() == CStructureData::fort)
    fConst *= GetOwner()->GetEdictFortBuildMult();
int iInc = GetProd(fConst);
```

⚠️ **Naming note:** Enemy Nations has **no separate "bunker" or "pillbox" building**.
The defensive structures are three fort tiers `fort_1/fort_2/fort_3`
([building.h:172-174](../enations_latest/src/building.h#L172-L174)), which `GetBldgType()`
collapses to `fort` ([building.h:216](../enations_latest/src/building.h#L216)). So
"Fortify the Border" buffs all forts. The edict's flavor text can still say
"forts, bunkers & pillboxes" — it just maps to the fort family.

### 2.4 UI — VERIFIED the pieces exist

- **`SDL2Checkbox`** toggle widget already exists with an `onChange(bool)` callback
  ([SDL2UI.h:176-194](../enations_latest/src/SDL2UI.h#L176-L194)).
- Mature dialog framework (11 `SDL2*Dialog` classes in
  [SDL2GameDialogs.h](../enations_latest/src/SDL2GameDialogs.h)). Model the new
  `SDL2EdictsDialog` on `SDL2ResearchDialog`.
- **Must be non-modal** — `DoModal` runs its own loop and freezes the sim (memory:
  `project_ingame_dialogs_nonmodal`). Follow the `ShowNonModal` + guard-pointer pattern
  at [toolbar.cpp:918-920](../enations_latest/src/toolbar.cpp#L918-L920).
- **Access gating** mirrors how Research gates on owning a research center:
  `GetExists(CStructureData::research)` at [toolbar.cpp:908](../enations_latest/src/toolbar.cpp#L908).
  `GetExists(int)` ([player.h:416](../enations_latest/src/player.h#L416)) indexes the
  per-player building-exists table by `BLDG_TYPE`.

### 2.5 Multiplayer — net command template VERIFIED

Edicts change a player's simulation, and every client simulates every player
deterministically, so toggles **must** go through a net command (not applied on click).
The template is `CNetRsrchDisc`:

- **Type enum:** add `edict_toggle` to the command enum near
  [netcmd.h:153-155](../enations_latest/src/netcmd.h#L153-L155).
- **Message class:** mirror [netcmd.h:1466-1472](../enations_latest/src/netcmd.h#L1466-L1472):
  ```cpp
  class CNetEdictToggle : public CNetCmd {
    public:
      CNetEdictToggle(CPlayer const* pPlyr, int iEdict, bool bOn);
      int m_iPlyrNum, m_iEdict, m_bOn;
  };
  ```
  Constructor mirrors [netcmd.cpp:952](../enations_latest/src/netcmd.cpp#L952).
- **Sender:** `CNetEdictToggle msg(this, iEdict, bOn); theGame.PostToAll(&msg, sizeof(msg), FALSE);`
  (same as [player.cpp:463-464](../enations_latest/src/player.cpp#L463-L464)).
- **Handler:** add a `case CNetCmd::edict_toggle:` to the dispatch switch in
  [netapi.cpp](../enations_latest/src/netapi.cpp#L3439) (next to `research_disc`),
  resolve the player via `_GetPlayerByPlyr`, and set the edict bit. Apply on **all**
  clients (including local) so the click only sends and the handler is the single
  mutation point — this guarantees ordering consistency.

### 2.6 Persistence — VERIFIED, positional append

`CPlayer::Serialize` ([player.cpp:740+](../enations_latest/src/player.cpp#L740)) is a flat
positional read/write (the multipliers are already serialized at
[:778-789](../enations_latest/src/player.cpp#L778-L789) / [:890-900](../enations_latest/src/player.cpp#L890)).
Add the edict bitmask (and any per-edict scalar state) by appending matching `ar <<` /
`ar >>` lines on **both** store and load branches in the same order.

> ⚠️ The format has **no per-record version tag** visible in `CPlayer::Serialize` — it is
> strictly positional. Appending fields therefore breaks reading *older* saves. See
> Research Gap RG-4 for whether a global save-version guard exists; otherwise accept that
> in-development saves are invalidated (consistent with prior phases of this port).

## 3. Architecture

### 3.1 Edict definition table (static data)

A small static table — no `.DAT` editing needed (unlike new resources/buildings):

```cpp
enum EdictId { EDICT_FORTIFY_BORDER, EDICT_NUTRITION, EDICT_MINING_SUB,
               EDICT_RESEARCH_SUB, /* ... */ EDICT_COUNT };

struct EdictDef {
    const char* name;
    const char* desc;
    // bonus multipliers (1.0 = no change)
    float fConstMult, fFortConstMult, fMineMult, fRsrchMult, fPopGrowthMult;
    // upkeep, expressed as a fraction of the relevant base demand or a flat amount
    float fEnergyUpkeepPct;     // added to m_iPwrNeed as pct of current need
    float fWorkforceUpkeepPct;  // added to m_iPplNeedBldg
    float fFoodUpkeepPct;       // extra drain in PeopleAndFood
};
static const EdictDef g_aEdicts[EDICT_COUNT] = { ... };
```

(Percentage-of-current-demand upkeep makes "+20% energy usage" literal and scales with
empire size, matching the examples. Flat costs are also an option per-edict.)

### 3.2 CPlayer state

```cpp
DWORD m_dwEdicts;            // bitmask of active edicts (≤32; widen if needed)
// derived per-loop edict multipliers, recomputed when m_dwEdicts changes:
float m_fEdictConstMult, m_fEdictFortBuildMult, m_fEdictMineMult,
      m_fEdictRsrchMult, m_fEdictPopGrowthMult;
```

`RecomputeEdictMults()` walks active bits and folds the table into the cached floats;
call it on toggle and after load.

### 3.3 Touch list (small)

| File | Change |
|---|---|
| `player.h` | add `m_dwEdicts` + edict mult fields; fold mults into `Get*Prod()` accessors; `ToggleEdict`/`RecomputeEdictMults`/`GetEdictFortBuildMult` |
| `player.cpp` | init in ctor; `RecomputeEdictMults`; upkeep in `StartLoop` (after [:427](../enations_latest/src/player.cpp#L427)) and `PeopleAndFood`; research-edict needs the multiplier wired into `Research()` (see RG-1); serialize append |
| `vehicle.cpp` | fort-specific construction mult in `ConstructBuilding` ([:938](../enations_latest/src/vehicle.cpp#L938)) |
| `netcmd.h/.cpp` | `edict_toggle` enum + `CNetEdictToggle` class |
| `netapi.cpp` | dispatch case for `edict_toggle` |
| `SDL2GameDialogs.h/.cpp` | new `SDL2EdictsDialog` (checkbox list + live upkeep readout) |
| `area.cpp` | **access hook**: add `UTcommand` + `UThousing` cases to the `OnLButtonDblClk` union-type switch ([area.cpp:4538](../enations_latest/src/area.cpp#L4538)) → open the edict dialog non-modally (mirrors the `UTresearch`/`UTembassy` cases). Differentiate office vs apartment by `GetBldgType()`. |
| `SDL2GameDialogs.*` | `OpenEdicts(category)` helper + guard-pointer like `m_pSdlResearch`/`_GotoScience` |

## 4. Verified assumptions

- ✅ **Global production multipliers exist, are serialized, are runtime-modified.**
  (player.h:497-509, player.cpp:504-528, 778-900.)
- ✅ **All production flows through `Get*Prod()` accessors at single chokepoints** → folding
  the edict mult into the accessor reaches every consumer. (Grep across mainloop/vehicle/new_unit.)
- ✅ **Energy & workforce are per-loop demand totals, cleared+reaccumulated each loop**, so
  upkeep added at the clear point is throttled correctly. (player.cpp:393-400, 424-427.)
- ✅ **Food is a real stockpile drained in `PeopleAndFood`.** (player.cpp:585-588.)
- ✅ **`GetProd`/`GetFrameProd` wrappers already compose the power/people/damage throttles**,
  so edict bonus + underpaid-upkeep penalty interact sensibly. (building.inl:253-276.)
- ✅ **Net-command pattern is small and well-defined** (enum + class + sender + dispatch case).
  (netcmd.h:153-155, 1466-1472; netcmd.cpp:952; netapi.cpp:3439.)
- ✅ **`SDL2Checkbox` widget + non-modal dialog pattern exist.** (SDL2UI.h:176; toolbar.cpp:918.)
- ✅ **`GetBldgType()==fort` distinguishes forts; bunker/pillbox don't exist as buildings.**
  (building.h:172-174, 216.)
- ✅ **`GetRsrchMult()` (m_fRsrchProd) is currently DEAD** — defined + serialized but never
  read; `Research()` ([player.cpp:443](../enations_latest/src/player.cpp#L443)) ignores it.
  A research-speed edict must wire a multiplier into that formula itself (see RG-1).

## 5. Research gaps / open questions

- **RG-1 — Research multiplier is an unwired latent bug (verified both ends).** The race
  research bonus `m_fRsrchProd` is read from `CRaceDef::research`
  ([player.cpp:262](../enations_latest/src/player.cpp#L262)), difficulty-adjusted
  ([:305](../enations_latest/src/player.cpp#L305)) and serialized
  ([:788](../enations_latest/src/player.cpp#L788)) — but **never consumed**. Traced both
  chokepoints:
    1. Production — research building `Operate` accumulates
       `fTmp = pBr->GetRate() * m_fDamPerfMult * GetPplMult()`
       ([mainloop.cpp:1517-1519](../enations_latest/src/mainloop.cpp#L1517-L1519)) — no race mult.
    2. Conversion — `Research()` uses `iNum = m_iRsrchHave * iNumSec * 2`
       ([player.cpp:443](../enations_latest/src/player.cpp#L443)) — no race mult.
  `GetRsrchMult()` has **zero callers**. (Contrast: `m_fAttack`/`m_fDefense` *are* consumed at
  [new_unit.cpp:1181-1185](../enations_latest/src/new_unit.cpp#L1181-L1185) — so this is research
  specifically, not the whole family.) **This means races that should research faster currently
  do not — a pre-existing bug.**
  **Clinching evidence:** (a) `research` is a peer of 9 other `CRaceDef::RACE` attributes
  ([racedata.h:23-37](../enations_latest/src/racedata.h#L23-L37)) and it is the *only one* with
  no live consumer (`build_bldgs`/`mine_prod`/`farm_prod`/`manf_*`/`pop_*`/`attack`/`defense`
  are all wired through their accessors). (b) The identical dead accessor exists in the 1996
  reference snapshot ([enations/src/PLAYER.H:160](../../enations/src/PLAYER.H#L160), set at
  Player.cpp:249, scaled at :291, no callers) → **this shipped broken in the original game; it
  is not a port regression.** Side effect: the AI's intended 20% research handicap
  (`m_fRsrchProd *= 0.8`, [player.cpp:305](../enations_latest/src/player.cpp#L305)) is also
  silently dropped.
  **Data verified (ENATIONS.DAT `CRAT`→`RACE`, 12 races):** the `research` column is
  hand-authored and race-flavored, *not* placeholder — e.g. Diolian 1.50 (the science race,
  everything else 0.9), Trisekan 0.90 (industrialists: build 1.5/manf 1.35), Kartugan 1.40
  (tech powerhouse), Human 1.00 (baseline), Mendari 0.0 = the random race. Each float is
  clamped to `[0.7, 1.2]` at load ([racedata.cpp:116](../enations_latest/src/racedata.cpp#L116)),
  so effective research multipliers are **0.9–1.2** for humans — the same magnitude band as
  the 10 sibling attributes that are *already live and shipped*. Enabling it is therefore
  in-pattern and balanced (AI stacks to 0.72–0.96 with the 0.8 handicap). This confirms a
  genuine missing-application bug, not a deliberate balance cut.
  **FIXED 2026-06-07** at [mainloop.cpp:1517-1525](../enations_latest/src/mainloop.cpp#L1517-L1525)
  — research accumulation now multiplies by `GetOwner()->GetRsrchMult()`. *Action (edict):*
  fold the Research Subsidies edict factor into `GetRsrchMult()` (combined with the race
  factor) into the research-building accumulation at mainloop.cpp:1518 — the cleaner of the two
  spots, since it's the productivity point. Doing so fixes the race bonus *and* enables the
  Research Subsidies edict in one change. Deterministic per-player, so no MP-desync concern as
  long as all clients run the same formula.
- **RG-2 — Access building: is "office" player-buildable?** building.h:166 comments that
  apartments/offices are *"only placed by computer."* If the human never owns an
  `office_*`, `GetExists(office)` would gate the human out. *Action:* verify whether the human
  build menu places offices (the `dlg_admin` build group) and whether offices register in
  `m_piBldgExists` for human players. **Recommendation:** gate on
  `CStructureData::command_center` instead — every player owns exactly one, it's
  unambiguously player-owned, and "edicts issued from your command center" is the cleaner
  RTS metaphor. Resolve before wiring the entry point.
- **RG-3 — Workforce-cost semantics.** Adding to `m_iPplNeedBldg` makes an edict compete with
  buildings for labor, lowering `m_fPplMult` and thus throttling **all** production
  empire-wide — not just the edict's category. *Action:* confirm that's the intended "tax"
  feel. If a category-local penalty is wanted instead, model the workforce cost as a local
  multiplier rather than global demand. (Energy has the same global-throttle property.)
- **RG-4 — Save versioning.** *Partially resolved.* `CPlayer::Serialize` itself is
  **schema-less / positional** (no per-record version tag), so appended fields will mis-read
  older saves byte-for-byte. **However, a file-level save-version mechanism DOES exist** — the
  game carries a global `VER_STRING` ([main.cpp:440](../enations_latest/src/main.cpp#L440)),
  there is a "saved by another version" path (`IDS_SAVE_VER` in lastplnt.rc) and an explicit
  "save-version mismatch" load error noted at
  [EnSettings.cpp:100](../enations_latest/src/EnSettings.cpp#L100). *Action:* locate the
  file-header version int written/checked on save-load (not in `CPlayer::Serialize`; likely the
  doc/game-level serialize) and **bump it** so pre-edict saves are cleanly *rejected* with the
  mismatch dialog rather than silently corrupted. Accepting old-save invalidation is consistent
  with prior port phases.
- **RG-5 — AI participation.** The AI mirrors these multipliers in its own manager
  ([caimgr.cpp:3731-3736](../enations_latest/src/caimgr.cpp#L3731-L3736)) and will not use
  edicts unless taught. *Action:* ship v1 **human-only** (acceptable asymmetry — the AI
  already gets difficulty bonuses); design an AI edict-selection heuristic as a follow-up.
- **RG-6 — `GetExists` index space.** Confirm `m_piBldgExists` is indexed by `BLDG_TYPE`
  enum value (toolbar.cpp:908 uses it that way for `research`), and that the chosen access
  building's enum index lands in-range for both human and AI players.
- **RG-7 — Upkeep "percentage of what?"** "+20% energy usage" / "+50% mining energy" need a
  defined base. Cheapest: pct of the player's *current total* `m_iPwrNeed` (scales with
  empire). Alternative: pct of only the relevant category's demand (needs per-category
  demand tracking, which doesn't exist today). *Action:* pick the base with the designer;
  total-demand pct is recommended for v1.

## 6. Phased implementation

1. **Vertical slice (de-risk the net path).** One edict — *Mining Subsidies* — end to end:
   `m_dwEdicts` bit + `RecomputeEdictMults` + `GetMineProd()` fold + energy/workforce upkeep
   in `StartLoop` + `CNetEdictToggle` + dispatch case + a throwaway one-checkbox dialog +
   serialize. Validate: bonus applies, upkeep shows in the power/people bars, toggle survives
   save/load, and a 2-client net game stays in sync. **Resolve RG-2 and RG-7 here.**
2. **Edict table + full dialog.** Populate `g_aEdicts`, build `SDL2EdictsDialog` (checkbox per
   edict, live "+X energy / +Y workers / +Z food/min" upkeep readout, gray-out unaffordable),
   wire the access button on the chosen building, guard-pointer non-modal open.
3. **Category-specific & food/pop edicts.** Fort-construction hook (vehicle.cpp), Nutritional
   Plenitude (`m_fPopGrowth`/food drain), Research Subsidies (RG-1 wiring).
4. **Polish.** Tooltips/flavor text, affordability warnings, status feedback when an edict is
   being under-supplied (mult < 1).
5. **(Later) AI.** Teach the AI manager to enable edicts when it has surplus energy/food/labor.

## 7. Risks

- **Multiplayer desync (highest).** Mitigated by routing every toggle through `CNetEdictToggle`
  and applying state *only* in the dispatch handler (single mutation point, all clients).
- **Save-format break (medium).** Positional append invalidates old saves (RG-4); acceptable
  mid-port, but bump/guard if a version mechanism exists.
- **Balance / runaway feedback (low-medium).** A production edict whose upkeep is paid by the
  same category it boosts can spiral; the global-throttle nature of energy/people upkeep is a
  natural brake. Tunable via the table.
- **AI asymmetry (low).** Human-only v1 means the AI never pays/benefits; acceptable given
  existing difficulty multipliers.

## 8. Effort estimate

| Phase | Estimate |
|---|---|
| 1 — vertical slice | ~1 day |
| 2 — table + dialog | ~1.5 days |
| 3 — category/food/pop edicts | ~1 day |
| 4 — polish | ~0.5–1 day |
| **v1 total (human-only)** | **~3–4 days** (~5–6 with thorough MP testing) |
| 5 — AI (separate) | ~2–4 days |

The expensive infrastructure (multipliers, demand tracking, serialization, dialog
framework, checkbox widget, net-command pattern) **already exists**. New work is mostly
wiring + one data table + one dialog + one net message + a few serialize lines.

---

# Part B — Expanded edict catalog & research gating

> Added 2026-06-07. **PLAN STATE ONLY — do not implement.** Lots of prerequisites
> (Part A v1, the new engine levers in §11, and the research-table work in §10) come first.
> This section catalogs the full proposed edict set and classifies each by how much *new*
> engine work it needs, so we can sequence them.

## 9. Edict catalog & per-edict feasibility

Feasibility tiers:
- 🟢 **Easy** — reuses an existing per-player multiplier; fold the edict factor into the
  accessor (the Part A pattern). No new engine mechanism.
- 🟡 **Moderate** — needs *scoped* application at an existing production hook
  (by building type / material / unit type), i.e. the "Fortify the Border = forts only"
  pattern (§2.3). Repeatable but per-case.
- 🔴 **Novel** — needs an engine mechanism that does **not** exist yet (see §11). Must be
  investigated/built before the edict is possible.

### Core set (Part A)
| Edict | Effects | Cost | Tier | Hook |
|---|---|---|---|---|
| Fortify the Border | +50% fort construction | +20% energy | 🟡 | type-scope `ConstructBuilding` on `fort` |
| Nutritional Plenitude | +20% pop growth | +50% food | 🟢 | `m_fPopGrowth`, `m_fEatingRate` |
| Mining Subsidies | +25% mine output | +50% mine energy, +25% mine workforce | 🟢 | `m_fMineProd` + upkeep |
| Research Subsidies | +25% research | +50% rsrch energy, +25% rsrch workforce | 🟢 | `GetRsrchMult()` (now wired) + upkeep |

### Logistics & Transportation
| Edict | Effects | Cost | Tier | Notes |
|---|---|---|---|---|
| **High-Octane Routing** | +25% truck speed *(drop "collision avoidance" — no engine knob)* | +25% global gas use | 🟡 | truck-speed lever resolved → inject at [vehmove.cpp:69](../enations_latest/src/vehmove.cpp#L69), scoped to trucks (RG-10). Gas-use side 🟢. |
| **Overloaded Axles** | +25% truck cargo capacity, −15% truck speed | none | 🟡 | capacity = scale `GetParam(i)` at router fill ([chproute.cpp:5430](../enations_latest/src/chproute.cpp#L5430)); speed = vehmove.cpp:69 (RG-10). Thematic gate: `cargo_handling` research. |

### Industrial Output
| Edict | Effects | Cost | Tier | Notes |
|---|---|---|---|---|
| **Fossil Fuel Overdrive** | +30% output from fossil power plants | +30% coal & gas burn at those plants | 🟡 | fossil test resolved: `GetBldPower()->GetInput() >= 0` (RG-8). Scale output `pBp->GetPower()` + burn `iNum` in `BuildPower` ([mainloop.cpp:2333](../enations_latest/src/mainloop.cpp#L2333)). |
| **Just-In-Time Manufacturing** | buildings halt when local store >25% full | none | 🔴 | needs conditional production gate in `Operate` keyed on local `m_aiStore` fullness (§11.5). |
| **Furnace Subsidies** | +25% steel/iron refinery output | +50% power at those refineries | 🟡 | scope `BuildMaterials` ([mainloop.cpp:2232](../enations_latest/src/mainloop.cpp#L2232)) by **building type `smelter`** (NOT union type — `UTmaterials` also covers the oil→gas refinery; verified there are exactly 2 converters, smelter #41 + refinery #29, see [[project_resource_system_overview]]) + per-building power add. |
| **Strip Mining Protocols** | +40% coal/iron extraction | −20% deposit lifespan | 🟡/🔴 | mineral-scope `BuildMine` on coal/iron (🟡) **+** a deposit depletion-rate lever in `CMinerals` (🔴, §11.6). |

### Population & Sustenance
| Edict | Effects | Cost | Tier | Notes |
|---|---|---|---|---|
| **Synthetic Rations** | −30% food consumption, −10% worker efficiency | +10% global electricity | 🟢/🟡 | `m_fEatingRate` (🟢) + global power upkeep (🟢) + global worker-efficiency mult (🟡, §11.1). |
| **Mandatory Overtime** | +15% global production, −15% pop growth | +10% global food | 🟢 | all existing levers: global prod mult (§11.1), `m_fPopGrowth`, `m_fEatingRate`. |
| **Automated Draft** | +50% infantry build speed, −20% worker efficiency | none | 🟡 | unit-type-scope barracks/manf build on infantry + global worker-efficiency mult (§11.1). |

## 10. Research gating (edicts unlocked by research)

**Most edicts should be locked behind research.** The research system already supports this:
- Topics are a fixed-index enum `CRsrchArray` ([research.h:70-123](../enations_latest/src/research.h#L70-L123)),
  loaded from ENATIONS.DAT, with `CRsrchItem` carrying prerequisites
  (`m_piRsrchRequired`, `m_piBldgsRequired`, `m_iScenarioReq`,
  [research.h:50-55](../enations_latest/src/research.h#L50-L55)) and name/desc/result strings.
- Discovery fires `UpdateRacialAttributes(topic)` ([player.cpp:498](../enations_latest/src/player.cpp#L498)).
  **Edict-unlock topics need no case there** — they're inert "unlock flags"; the edict itself
  applies the modifier. The dialog just checks `GetMe()->GetRsrch(topic).m_bDiscovered`.
- **Save-friendly:** research status is serialized with a dynamic count
  ([player.cpp:767-768](../enations_latest/src/player.cpp#L767-L768)), not a fixed-size array —
  so growing the topic list is *less* save-breaking than the material/building fixed arrays
  (verify, RG-9).

**Two gating paths:**
- **(A) Reuse existing topics** (zero data-file work). Several map thematically:
  `fortification` → Fortify the Border; `cargo_handling` → Overloaded Axles;
  `gas_turbine`/`nuclear` → Fossil Fuel Overdrive; `mine_2` → Strip Mining;
  `large_facilities`/`advanced_facilities` → Furnace Subsidies / industrial edicts.
- **(B) Add new dedicated "Edicts" research topics** — grow the `CRsrchArray` enum + the
  ENATIONS.DAT research LIST. This is the **same fixed-index data-file debt** as adding
  materials/buildings, and it is **enforced**: `CRsrchArray::Open` reads a `NUMI` count from the
  .DAT and asserts `iSize + 1 == num_types` ([research.cpp:189](../enations_latest/src/research.cpp#L189)),
  so the enum and the .DAT must grow together or load aborts. (Topics also carry
  `m_piRsrchRequired`/`m_piBldgsRequired` prereqs read from the same chunk, :201-211.)
  See [[project_new_resources_plan]] and [[project_resource_system_overview]]. Bundle this with
  the new-resources .DAT work.

**Recommendation:** v1 uses path (A) — gate the first edicts on existing topics, no .DAT
changes. Add a dedicated edict research branch (path B) later, once the research-table .DAT
growth is tackled.

## 11. New engine levers required (TODO — investigate before building)

These don't exist yet. Each is a prerequisite for the 🟡/🔴 edicts above.

1. **Global production multiplier** `m_fEdictGlobalProdMult` — folded into *all* `Get*Prod()`
   accessors. Enables "global production speed" and "worker efficiency" effects (Mandatory
   Overtime, Synthetic Rations, Automated Draft). **Easy** — same pattern as Part A §2.1.
2. **Repeatable type-scoped production** — generalize the fort pattern (§2.3) into a tidy helper
   so scoping by building type (smelter), union type, mined material (coal/iron), or built-unit
   type (infantry) is uniform. **Moderate.**
3. **Truck speed & cargo-capacity multipliers — RESOLVED (was Novel, now 🟡).** Speed injects at
   the single consumption site [vehmove.cpp:69](../enations_latest/src/vehmove.cpp#L69)
   (`m_fVehMove += … GetData()->GetSpeed() …`), scoped to truck/transport unit type; MP-safe
   (deterministic accumulation + net-synced edict). Capacity scales the per-material fill cap
   `GetParam(i)` at the router fill site ([chproute.cpp:5430](../enations_latest/src/chproute.cpp#L5430)).
   Drop the "collision avoidance" sub-effect (no scalar knob).
4. **Per-power-plant-type output + fuel-burn scaling — RESOLVED (was Novel, now 🟡).** Fossil test
   is `GetBldPower()->GetInput() >= 0` ([mainloop.cpp:2343](../enations_latest/src/mainloop.cpp#L2343));
   scale output `pBp->GetPower()` (:2354) and burn `iNum` (:2387) inside `CPowerBuilding::BuildPower`.
   No need to enumerate `power_1/2/3` — the fuel-input branch is the discriminator.
5. **Conditional production gate on local store fullness** — add a check in `CBuilding::Operate`
   that halts output when the building's `m_aiStore` for its product exceeds a % of capacity.
   Needed by Just-In-Time Manufacturing. **Novel** (also a balance question: interacts with the
   CHPRouter truck dispatch logic).
6. **Mineral-deposit depletion-rate multiplier** — a knob on `CMinerals` quantity drain so
   Strip Mining can trade deposit lifespan for extraction rate. **Novel.**
7. **"Collision avoidance" (High-Octane Routing)** — no clean engine knob; pathing behavior is
   not a simple scalar. **Recommend dropping this sub-effect or re-spec'ing** the edict to
   speed + gas-cost only.

## 12. Investigations — RESOLVED (2026-06-07)

All Part B open questions were investigated and resolved against source.

- **RG-2 — access point. RESOLVED → gate on `command_center`, not "office".**
  - The human starts with only a crane + trucks from race **supplies**
    ([wrldinit.cpp](../enations_latest/src/wrldinit.cpp), `GetSupplies`); there is **no
    `command_center` in the supplies list**, so the player *builds* everything including the CC.
  - The human **does** own offices — the SDL info panel draws office occupancy bars via
    `GetBldgType()==office` ([SDL2Toolbar.cpp:635,939](../enations_latest/src/SDL2Toolbar.cpp#L635)).
    ("only placed by computer" means the game auto-picks the *variant*, not that the AI owns them.)
  - **But `GetExists(CStructureData::office)` is unreliable:** `AddExists` is keyed on the
    *specific* type `GetData()->GetType()` ([mainloop.cpp:1628](../enations_latest/src/mainloop.cpp#L1628),
    new_unit.cpp:1957/3259), and `office == office_2_1`, so it counts only one of the 5 office
    variants. To gate on "any office" you must **sum `GetExists` over the office variant indices**
    or iterate buildings by `GetBldgType()`.
  - **`command_center` is a single building type** → `GetExists(command_center)` is reliable, and
    "edicts from your command center" is the cleaner metaphor. **Use it.** (The legacy
    `BLDG_DLG_GRP` build-group enum is dead in the SDL port, so build-menu availability is a UI
    detail, not a gating concern.)
- **RG-8 — fossil power plants. RESOLVED.** `CPowerBuilding::BuildPower`
  ([mainloop.cpp:2343-2354](../enations_latest/src/mainloop.cpp#L2343)) branches on
  `pBp->GetInput()`: **`GetInput() < 0` = non-fossil** (free power, no fuel); **`GetInput() >= 0`
  = fossil**, burning the stored input material (coal/gas) whose index *is* `GetInput()`. So Fossil
  Fuel Overdrive scopes on `GetBldPower()->GetInput() >= 0` and scales output `pBp->GetPower()`
  (:2354) + burn `iNum` (:2387-2388), all in one function. **Upgrades 🔴 → 🟡.**
- **RG-9 — research-table growth. RESOLVED, favorable.** Saves **auto-migrate**: on load, if
  `m_aRsrch.GetSize() < theRsrch.GetSize()` the array is resized up and new topics default to
  undiscovered ([player.cpp:857-866](../enations_latest/src/player.cpp#L857-L866)). So adding
  edict-unlock topics does **not** break old saves. The only hard constraint is the
  enum/.DAT match assert ([research.cpp:189](../enations_latest/src/research.cpp#L189)) — the RSRH
  chunk's `NUMI` + per-item `DATA` chunks must be regenerated to match the grown enum (same .DAT
  tooling as the new-resources work). Net discovery is per-index (`CNetRsrchDisc`), fine across a
  shared build.
- **RG-10 — truck speed injection. RESOLVED.** Vehicle data-speed is consumed at a single site,
  [vehmove.cpp:69](../enations_latest/src/vehmove.cpp#L69):
  `m_fVehMove += (m_fDamPerfMult * 0.9 * GetOpersElapsed() * GetData()->GetSpeed()) / …`. A
  per-player truck-speed multiplier (scoped to truck/transport unit type) injects there; it's
  MP-safe (deterministic accumulation, net-synced edict bit). **Upgrades truck-speed 🔴 → 🟡.**
  Truck **capacity** (Overloaded Axles) scales the per-material fill cap `GetParam(i)` at the
  router fill site ([chproute.cpp:5430](../enations_latest/src/chproute.cpp#L5430)) — 🟡.
  *"Collision avoidance" has no engine knob — drop it.*
- **RG-11 — JIT vs CHPRouter. RESOLVED, no conflict.** The router is **demand/pull-driven** —
  producers fire `MsgOutMat` only when an *input* store crosses the "1-minute" threshold
  ([mainloop.cpp:2266-2281](../enations_latest/src/mainloop.cpp#L2266)); output pickup is driven by
  *consumers'* needs (`GetNextMinuteMat`, chproute.cpp:4335+). Halting a producer when its **output**
  store exceeds 25% of `GetCapacity()` is a clean soft-pause added at the top of the `Build*`
  operate path — it doesn't fight routing (trucks still pull existing stock). **Still 🔴 (new
  behavior) but architecturally unblocked.**

## 13. Verification log (2026-06-07)

Full audit of every claim/approach in this plan against the current source.

**Confirmed correct (load-bearing):**
- Production multipliers + single-chokepoint accessors: `GetConstProd` ([vehicle.cpp:938](../enations_latest/src/vehicle.cpp#L938)),
  `GetMineProd`/`GetMtrlsProd` ([mainloop.cpp:2423](../enations_latest/src/mainloop.cpp#L2423)/[:2232](../enations_latest/src/mainloop.cpp#L2232)),
  `GetManfProd` (:1826), `GetFarmProd` (:2486). Folding an edict factor into the accessor reaches all consumers. ✔
- `GetProd`/`GetFrameProd` wrappers compose power/people/damage throttles ([building.inl:253-276](../enations_latest/src/building.inl#L253-L276)). ✔
- Upkeep hooks: `m_iPwrNeed`/`m_iPplNeedBldg` cleared at [player.cpp:424-427](../enations_latest/src/player.cpp#L424-L427) then re-accumulated per building; food drained in `PeopleAndFood`. ✔
- `GetBldgType()` **collapses** `fort_1/2/3`→`fort`, offices→`office`, plants→`power` ([unit.cpp:2274-2285](../enations_latest/src/unit.cpp#L2274-L2285)) — §2.3 Fortify scoping is valid. ✔
- Net path: `theGame.PostToAll(CNetCmd const*, int, BOOL)` ([player.h:621](../enations_latest/src/player.h#L621)); dispatch switch + `CNetRsrchDisc` template (netapi.cpp:3439, netcmd.h:1466). ✔
- **Research gating pattern already exists**: `GetRsrch(CRsrchArray::X).m_bDiscovered` is exactly how `CanBridge`/`CanCopper`/… work ([player.h:421-426](../enations_latest/src/player.h#L421-L426)). ✔
- Levers for the novel edicts exist as fields: `m_iMinerals` deposit lifespan ([building.h:1149](../enations_latest/src/building.h#L1149)), `GetCapacity` store cap ([building.h:421](../enations_latest/src/building.h#L421)) for JIT, vehicle speed/cargo (above). ✔
- `SDL2Checkbox` widget + non-modal dialog pattern. ✔
- Research bug (RG-1) fixed + committed (8322ccb); race data verified balanced (Part A §RG-1). ✔

**Corrected during this audit:**
- §11.4 `BuildPower` is `CPowerBuilding::BuildPower` at **mainloop.cpp:2333**, not ~1478 (1478 is the rocket free-power line). Fixed.
- §2.1 `GetMtrlsProd` consumer is **:2232** (call site), not :2225 (an assert). Fixed.
- §9 Furnace Subsidies must scope by **building type `smelter`**, not the `UTmaterials` union type (which also includes the oil→gas refinery). Fixed.
- §9/§11.4 Fossil distinction can't use `GetBldgType()` (collapses to `power`) — must read fuel input. Clarified (RG-8).
- §10 path-B debt is **assert-enforced** at research.cpp:189. Strengthened.

**Follow-up investigations (RG-2, RG-8, RG-9, RG-10, RG-11): all RESOLVED — see §12.**
- RG-2 → gate on `command_center` (single type; `GetExists(office)` only counts one variant).
- RG-8 → fossil = `GetBldPower()->GetInput() >= 0`. RG-10 → truck speed at vehmove.cpp:69.
  RG-9 → saves auto-migrate research topics. RG-11 → JIT is a soft-pause, no router conflict.
- Net effect: Fossil Fuel Overdrive, High-Octane Routing, Overloaded Axles all dropped from
  🔴 to 🟡. Only **Just-In-Time Manufacturing** remains 🔴 (genuinely new behavior, but unblocked).

**Overall:** every Part A approach is verified buildable, and after the §12 investigations
**all but one Part B edict (JIT) are 🟢/🟡** with concrete injection sites identified. The plan is
fully grounded in the current source.

---

# Part C — Theorized edict expansion, organized by host building

> Added 2026-06-08. **PLAN STATE / brainstorm.** Per design direction, edicts are hosted at
> **three** administrative buildings, each with a thematic category. This both fits the
> buildings' real roles and spreads the unlocks across the tech/build tree.

## 14. Host-building architecture

| Host building | Theme | Why it fits |
|---|---|---|
| **Command Center** | Military / strategic / global directives | the HQ; single building type → reliable `GetExists(command_center)` gate |
| **Office** | Economy / industry / production / logistics | offices = white-collar workplaces; the production-tuning edicts live here |
| **Apartments** | Population / sustenance / workforce / civil | apartments = housing; population & food edicts live here |

**Dialog/access — double-click the building (VERIFIED hook).** Per design direction, the edict
dialog opens on **double-clicking the host building on the map**, exactly like double-clicking a
factory opens build-unit or a crane opens build-building. The dispatch point is
`CWndArea::OnLButtonDblClk` ([area.cpp:4538](../enations_latest/src/area.cpp#L4538)), which already
switches on the building's **union type** and opens non-build dialogs for two cases — a direct
precedent:
```cpp
switch (GetData()->GetUnionType()) {
case UTvehicle: case UTshipyard: ((CVehicleBuilding*)punit)->GetDlgBuild(); return; // factory
case UTresearch: theApp.m_wndBar._GotoScience(); return;   // research dialog (non-modal)
case UTembassy:  theApp.m_wndBar.GotoRelations(); return;  // diplomacy dialog
// --- NEW ---
case UTcommand:  OpenEdicts(EDICT_CAT_MILITARY);   return; // command center
case UThousing:                                            // office + apartment share UThousing
    OpenEdicts( GetBldgType()==office ? EDICT_CAT_ECONOMY : EDICT_CAT_POPULATION );
    return;
}
```
- `command_center` → `UTcommand`; `office`/`apartment` → `UThousing`
  (differentiate by `GetBldgType()`). **Both `UTcommand` and `UThousing` are currently unhandled
  in that switch**, so adding cases is non-invasive (the only `UThousing` fall-through today is the
  truck-load check, which doesn't apply to housing).
- The handler already requires the building be **visible, owned by me, and the selected unit**
  (area.cpp:4502-4529) — exactly the right guard. Open **non-modal** (like `_GotoScience`), so the
  sim keeps running. Ownership/research gating is enforced inside `OpenEdicts` (and by graying
  checkboxes), so no extra build-menu wiring is needed.

**Gating reliability (from RG-2):** `command_center` is a single type — `GetExists` works directly.
`office` and `apartment` are multi-variant (`office_2_1…`, `apartment_1_1…`) and `GetExists(office)`
counts only the first variant, so ownership must be tested by **summing `GetExists` over the
variant indices** or iterating buildings by `GetBldgType()` (the SDL panel already uses
`GetBldgType()==office`/`==apartment`). Add a small helper `OwnsBldgFamily(BLDG_TYPE)`.

## 15. Command Center edicts (military / strategic)

| Edict | Effects | Cost | Tier | Hook |
|---|---|---|---|---|
| **Fortify the Border** *(Part A)* | +50% fort construction | +20% energy | 🟡 | type-scope `ConstructBuilding` on `fort` |
| **Total Surveillance** *(your radar idea)* | +20% unit/building vision | +flat energy (units use gas not power → add to `m_iPwrNeed`) | 🟡 | scale `m_iSpottingRange` at [new_unit.cpp:1176](../enations_latest/src/new_unit.cpp#L1176); **cap at `MAX_SPOTTING=15`** (hardcoded arrays, [base.h:84](../enations_latest/src/base.h#L84)); force per-unit recompute on toggle |
| **War Footing** | +15% attack & +15% defense | +food + workforce | 🟢 | `m_fAttack`/`m_fDefense` (`GetAttackMult`/`GetDefenseMult`, consumed [new_unit.cpp:1181-1185](../enations_latest/src/new_unit.cpp#L1181)) |
| **Forced March** | +20% military move speed | +gas | 🟡 | same lever as truck speed — [vehmove.cpp:69](../enations_latest/src/vehmove.cpp#L69), scoped to combat units |
| **Conscription** *(= Automated Draft)* | +50% infantry build speed | −20% global worker efficiency | 🟡 | unit-type-scope barracks build + global prod mult (§11.1) |
| **Defensive Doctrine** | +25% defense | +energy | 🟢 | `m_fDefense` only (cheaper, defensive counterpart to War Footing) |

## 16. Office edicts (economy / industry / logistics)

Hosts the Part A subsidies + all of Part B's industrial/logistics set. Additional ideas:

| Edict | Effects | Cost | Tier | Hook |
|---|---|---|---|---|
| **Mining / Research / Furnace Subsidies** *(A/B)* | +25% that sector | sector energy + workforce | 🟢/🟡 | `m_fMineProd` / `GetRsrchMult` / smelter-scoped `BuildMaterials` |
| **Fossil Fuel Overdrive, JIT, Strip Mining, High-Octane Routing, Overloaded Axles** *(B)* | — | — | 🟡/🔴 | see §9, §12 |
| **Assembly Line** | +25% vehicle manufacturing | +energy + materials drain | 🟢 | `m_fManfProd` (mainloop.cpp:1826) |
| **Lean Operations** | −20% building power *consumption* | −10% global production | 🟡 | scale `AddPwrNeed` amount globally + global prod mult; lets a power-starved economy trade output for grid headroom |
| **Materials Stockpiling** | +25% building store capacity | +energy | 🟡 | scale `GetCapacity` (m_iCapacity) read; pairs with JIT |

## 17. Apartment edicts (population / sustenance)

Hosts the Part A/B population set + civil-life ideas:

| Edict | Effects | Cost | Tier | Hook |
|---|---|---|---|---|
| **Nutritional Plenitude, Mandatory Overtime, Synthetic Rations** *(A/B)* | — | — | 🟢 | `m_fPopGrowth`/`m_fEatingRate` + global prod mult |
| **Baby Boom** | +30% population growth | +food consumption | 🟢 | `m_fPopGrowth`, `m_fEatingRate` |
| **Rationing** | −25% food consumption | −10% pop growth, −5% worker efficiency | 🟢 | `m_fEatingRate`, `m_fPopGrowth`, global prod mult |
| **High-Density Housing** | +25% housing capacity | +energy | 🟡 | scale where `m_iAptCap`/`m_iOfcCap` are accumulated ([player.h:443-444](../enations_latest/src/player.h#L443)); *verify the accumulation site* |
| **Public Health** | −pop death rate (fewer starvation/overcrowding deaths) | +food | 🟢 | `m_fPopDeath` (`GetPopDeath`, [player.h:333](../enations_latest/src/player.h#L333)) |

## 18. New levers introduced by Part C (beyond §11)

1. **Vision multiplier at `new_unit.cpp:1176`** — scale `m_iSpottingRange`; **must clamp to
   `MAX_SPOTTING=15`** and recompute on toggle (spotting is computed per-unit, not read live).
   🟡 — *verify when this line runs and how to trigger a re-scan after a toggle (RG-12).*
2. **Global building-power-consumption multiplier** (Lean Operations) — scale every `AddPwrNeed`
   amount. Distinct from the upkeep-*adds* in §2.2. 🟡.
3. **Store-capacity multiplier** (Materials Stockpiling) — scale `GetCapacity()` reads. 🟡.
4. **Housing-capacity multiplier** (High-Density Housing) — scale `m_iAptCap`/`m_iOfcCap`
   accumulation. 🟡 — *verify accumulation site (RG-13).*

## 19. Part C open questions
- **RG-12 — vision recompute.** Where/when is `m_iSpottingRange` (new_unit.cpp:1176) computed, and
  what forces a re-scan so a toggled radar edict affects already-built units immediately?
- **RG-13 — housing-cap accumulation site.** Confirm where `m_iAptCap`/`m_iOfcCap` are summed from
  apartment/office buildings (analogous to `AddPwrHave`) before speccing High-Density Housing.
- **Balance — vision cap.** With `MAX_SPOTTING=15`, a +20% radar edict only helps units whose base
  spotting × multiplier stays ≤ 15; high-base units clamp. Confirm that's acceptable (it is, given
  the engine's own ceiling).
