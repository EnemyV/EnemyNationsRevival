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
| Materials (smelt/refine) | `m_fMtrlsProd` | `GetMtrlsProd()` | [mainloop.cpp:2225](../enations_latest/src/mainloop.cpp#L2225) |
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
| `toolbar.cpp` | open hook gated on the access building, guard-pointer like `_GotoScience` |
| an entry-point UI | a button on the office/command-center info panel or toolbar |

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
