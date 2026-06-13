# Abandoned & Incomplete Features in Enemy Nations

*Investigation compiled 2026-05-31 from the `enations_latest/src` codebase (~208 .cpp/.h files). Deep pass v2.*

---

## 1. Triple Name Change: "Second Chance" → "The Last Planet" → "Enemy Nations"

The game went through **three** titles during development:

| Name | Evidence |
|---|---|
| **Second Chance** | MFC `m_pszAppName`, registry `HKCU\Software\Second Chance\Second Chance`, message box titles, `SDL2Dialogs.cpp:148` ("restart Second Chance for these changes to take effect") |
| **The Last Planet** | Main header `lastplnt.h` ("main header file for The Last Planet application"), all source `#include "lastplnt.h"`, AI comments ("the Last Planet AI") |
| **Enemy Nations** | Final binary `enations.exe`, CMake project name |

Second Chance was likely the first working title (frozen in registry keys and MFC app names). The Last Planet was the main development title. Enemy Nations was the final retail name.

**`resource.h:700`**: `#define IDC_MAIN_CAMPAIGN 1165` — a Campaign button exists in main menu resource IDs, suggesting a single-player campaign was planned.

---

## 2. Copper → Xilitium Rename (The "Copper Problem")

The resource `copper` was renamed to **Xilitium** in the UI layer, but the codebase was never fully updated.

- **`base.h:632`**: `copper, // Xil` — the enum still says copper
- **`resource.h:761-762`**: `IDC_TRUCK_XILITIUM`, `IDC_TRUCK_SPIN_XILITIUM` — UI controls use Xilitium
- **`resource.h:1243`**: `IDC_TRUCK_HAVE_XIL` — abbreviated "Xil" in resource IDs
- **`toolbar.cpp:582`**: `// Hide xilitium / copper until the player has researched it`
- **`research.cpp:68-69`**: `// copper discovered` → calls `theAreaList.XilDiscovered()`
- **Everywhere else**: ~100+ references still use `copper` in enums, AI, serialization, networking, building types

**Verdict**: Cosmetic rename started in UI layer and never completed in game logic.

---

## 3. Air Units — Researched But Never Built

The research tree has five aerial types, but **no vehicle units or production buildings exist**.

### Research exists for:
| Research Item | Index | In AI Path? |
|---|---|---|
| `balloons` | 1 | Combat + Economic |
| `gliders` | 2 | Combat + Economic |
| `prop_planes` | 3 | Combat + Economic |
| `jet_planes` | 4 | Combat + Economic |
| `rockets` | 5 | Combat + Economic |

### Missing:
- **`vehicle.h:99-120`** (`CTransportData::TRANS_TYPE`): No air vehicle types — only ground and naval
- **`building.h:150-200`** (`CStructureData::BLDG_TYPE`): No hangar, airport, or airfield
- **`building.h:229-242`** (`BLDG_UNION_TYPE`): No `UTaircraft` factory type

The AI invests research points in air units it can never build. The `rocket` building in-game is the **colonization rocket** (end-game win condition), not an attack craft.

---

## 4. The Phantom Vehicle Fleet

Beyond air units, many ground/naval vehicles were planned but never got enum entries in `CTransportData::TRANS_TYPE`:

| AI Task | Vehicle | Evidence |
|---|---|---|
| `IDT_MAKEBATTLESHIP` (2224) | **Battleship** | Not in vehicle enum. `IDG_BATTLESHIP` (1066) AI goal exists. |
| `IDT_MAKEBRIDTRUCK` (2217) | **Bridging Truck** | Not in vehicle enum. `IDG_BRIDGING` (1061) goal exists. Bridge work done by construction crane instead. |
| `IDT_MAKELTRUCK` (2204) | **Light Truck** | Not in enum. AI handler: `// case IDT_MAKELTRUCK:` — **explicitly disabled**. |
| `IDT_MAKEMTRUCK` (2205) | **Medium Truck** | Not in enum. `med_truck` is "obsolete" in `vehicle.h:100`. Handler disabled. |
| `IDT_MAKEMCARGOSHIP` (2219) | **Medium Cargo Ship** | Not in enum. Handler: `// case IDT_MAKEMCARGOSHIP:` — disabled. Only `light_cargo` exists. |
| `IDT_MAKEHCARGOSHIP` (2220) | **Heavy Cargo Ship** | Not in enum. |
| `IDT_MAKEMARINES` (2202) | **Marines** | In enum but "obsolete" (`vehicle.h:119`). Handler: `// case IDT_MAKEMARINES:` — disabled. |

### Motorcycle audio exists — no motorcycle:
- **`sfx.h:65,73,81`**: `motorcycle_idle`, `motorcycle_go`, `motorcycle_running` — three sound categories for a non-existent vehicle.

### Confirmation in `caiopfor.cpp:142-197`:
A large commented-out table lists combat power for every planned unit. Asterisked items were never finished:
```
*light_truck, med_truck, heavy_truck, light_scout, med_scout, heavy_scout,
infantry_carrier, light_tank, med_tank, heavy_tank, light_art, med_art, heavy_art,
*bridging, light_cargo, med_cargo, *heavy_cargo, gun_boat, destroyer, cruiser,
*battleship, landing_craft, infantry, rangers, marines
```

**Verdict**: Light/medium/heavy variants across trucks, cargo ships, and capital ships were dramatically simplified before ship.

---

## 5. The Espionage System — Fully Stubbed, Spy Commented Out

A complete espionage/intelligence subsystem was designed but **never implemented**:

| Constant | Value | Description |
|---|---|---|
| `IDG_ESPIONAGE` | 1021 | Conduct espionage against OPFORs |
| `IDT_MAKESPY` | 2226 | **COMMENTED OUT** — `//#define IDT_MAKESPY 2226` |
| `IDT_GETINTEL` | 2332 | Gather Intelligence |
| `IDT_STEALTECH` | 2333 | Steal Technology |
| `IDT_SABOTAGE` | 2334 | Conduct Sabotage |

### UI references:
- **`resource.h:97`**: `IDC_BAR_SPYING` — spy bar UI element
- **`resource.h:202`**: `IDH_BAR_SPYING` — spy bar help ID

### Implementation status: **ZERO**
The AI task handler (`caitmgr.cpp`) has NO case handlers for `GETINTEL`, `STEALTECH`, `SABOTAGE`, or `MAKESPY`. The spy task was literally commented out of the header before any code was written.

---

## 6. The Diplomacy & Trade System — Defined, Never Implemented

A full negotiation system was designed but only the relations framework was built:

### What EXISTS (working):
- Four relations states: `ALLIANCE` (0), `PEACE` (1), `NEUTRAL` (2), `WAR` (3) — `player.h:27-30`
- Fifth state `HOSTILE` used internally by AI
- Network message `CMsgSetRelations` for syncing
- Embassy-triggered relations changes
- `SDL2RelationsDialog` for player interaction

### What DOESN'T exist (only AI task IDs):
| Constant | Value | Description |
|---|---|---|
| `IDG_DIPLOMACY` | 1009 | Find other players and cities |
| `IDG_TRADE` | 1010 | Find, offer and assign trade |
| `IDG_AGREEMENT` | 1011 | Select player, offer agreement |
| `IDT_FINDOPFORTRADE` | 2312 | Find Trade OpFor |
| `IDT_OFFERTRADE` | 2320 | Offer Trade |
| `IDT_ASSIGNTRADE` | 2321 | Assign Trade Route |
| `IDT_SELECTAGREE` | 2322 | Select Agreement |
| `IDT_OFFERAGREE` | 2323 | Offer Agreement |
| `IDT_ACCEPTAGREE` | 2324 | Accept Agreement |
| `IDT_BREAKAGREE` | 2326 | Break Agreements |

None of these tasks have case handlers in the AI dispatcher.

---

## 7. Abandoned Building Types (AI Tasks Only)

| AI Task | Building | Notes |
|---|---|---|
| `IDT_BUILDEMERGREP` (2005) | **Emergency Response** | No building type. `IDG_EMERPOWER` goal exists. |
| `IDT_BUILDGOODSFAC` (2009) | **Goods Factory** | No building type. "Goods" = `// idk, electricity?` |
| `IDT_BUILDLOGCAMP` (2012) | **Logging Camp** | Separate from lumber mill (`IDT_BUILDLUMBMILL`) |
| `IDT_BUILDMOLYMINE` (2017) | **Moly Mine** | No building type. "Moly" = `// idk, people?` |
| `IDT_BUILDRECCNTR` (2020) | **Recreation Center** | No building type |
| `IDT_BUILDMARKET` (2025) | **Supermarket** | Separate from warehouse |

---

## 8. Obsolete Building Tiers

Three tiers were planned (frontier/established/city). Tier 3 was cut:

| Building | Status |
|---|---|
| `apartment_3_1` | obsolete |
| `apartment_3_2` | obsolete |
| `office_3_2` | obsolete |
| `barracks_3` | obsolete |

Shareware limited to `num_shareware_civ = 3` choices.

---

## 9. Abandoned AI Threading (`THREADS_ENABLED`)

- **`cai.h:17`**: `#define THREADS_ENABLED 1` — but all thread code is `#if 0`
- **`caigmgr.cpp`**: 12 disabled thread blocks
- **`caiopfor.cpp`**: 8 blocks
- **`cairoute.cpp`**: 4 blocks
- **`caidata.cpp`**: 2 blocks

Each contains a `myYieldThread()` pattern. Parallelized AI goal evaluation was started, proved unstable, and walled off with `#if 0`.

---

## 10. Partially Implemented AI Functions (`#if 0`)

### `caigmgr.cpp`:
- **`GetSupport()`** (line ~8939): Coordinate nearby combat units to assist attacked friendlies. Algorithm described in comments, body `#if 0`'d.
- **`UpdateTaskForce()`** (line ~6880): Assign non-overlapping staging destinations for task force units.
- **`IsTargetReachable()`** (line ~7209): Check if assault force can reach target (land vs island vs inland). Partial sea/land detection branch exists.

### `cairoute.cpp`:
- **`FillPriorities()`** (line ~442): Old resource routing priority system, replaced by newer approach.

### `caimaput.cpp`:
- **`LoadAroundHexes()`** (line ~8624): Load adjacent hex positions for area analysis.

### `citizen.cpp`:
- **`InitialRoad()`** (line 120): `#if 0 // FIXME: What even is going on here?` — spiral outward from city center.
- **`BuildCcBldg()`** (line 599): Same FIXME — build adjacent to city.

**Verdict**: The AI was actively being expanded when development stopped mid-implementation.

---

## 11. Abandoned Chat & Mail System

### In-Game Chat: Fully #if 0'd
- **`chat.h`**: `CDlgChatAll` in `#if 0 // MFC_LEGACY_CHAT_DIALOG`
- **`chatbar.h`**: `CDlgChatBar` disabled
- **`ipcchat.cpp`**: 7 `#if 0` blocks with TODO draw code markers

### Email: Partially functional
- `ProcessEmail()`, `OnEmailArrived`, `OnReplyEmailWnd`, `OnForwardEmail` in `ipccomm.cpp`
- `CEMsgList *plEmailMsgs` global message list
- `EVENT_HAVE_MAIL`, `EVENT_HAVE_CALL` notifications
- `IDS_MAIL_OPTIONS` settings string exists

---

## 12. Abandoned Networking Protocols

Seven transports defined in `vdmplay.h`. Only TCP/IP remains:

| Protocol | Status |
|---|---|
| TCP/IP (`VPT_TCP`) | **Active** |
| IPX (`VPT_IPX`) | Config dialog in `advanced.cpp:250-344` |
| NetBIOS (`VPT_NETBIOS`) | Config dialog in `advanced.cpp:354-425` |
| Modem (`VPT_MODEM`) | Dial settings partially `#if 0`'d |
| Serial/COMM, TAPI, DirectPlay | Abandoned |

---

## 13. Shareware / Demo Limits

| Limit | Value | Location |
|---|---|---|
| Single-player time | 90 min | `base.h:26` |
| Multiplayer time | 60 min | `base.h:27` |
| Research items | 6 max | `base.h:28` |
| Building choices | 3 civilian | `building.h:207` |

Shareware detected via .dat file size (< 400MB). `IsSecondDisk()` for "friend disk" multiplayer-only installs.

---

## 14. Abandoned DRM & Registration

- **`CdLoc.cpp:20`**: `#if 0 // CD check disabled — no physical media requirement`
- `IDD_CD_LOC`, `IDD_SELECT_CD` — CD-ROM dialogs
- `IDD_LICENSE`, `IDD_REGISTER` — license/registration dialogs
- **`lastplnt.cpp:944-957`**: Month-long demo time-bomb
- **`main.cpp:320`**: Separate `demo_license` vs `retail_license` program positions
- **`lastplnt.cpp:1307`**: `#ifndef _GG && 0` — disabled CD speed benchmark

---

## 15. "Moly" & "Goods" — Mystery Materials

```cpp
moly,   // idk, people?
goods,  // idk, electricity?
```

Developers were uncertain what these represented. Both have AI goals (`IDG_ADDMOLY`, `IDG_ADDGOODS`) and mining tasks but their game meaning was never finalized. "Moly" = Molybdenum, possibly a population/labor proxy.

---

## 16. Communication Research — Prerequisites for Missing Diplomacy

`radio`, `mail`, `email`, `telephone` — four research items with no mechanical effect. They sit at the end of AI social research paths and were likely prerequisites for the unimplemented diplomacy/trade system.

---

## 17. The `_GG` Developer Build Flag

A developer-specific build (`_GG`, likely initials):

- **`lastplnt.cpp:886`**: Forces `m_bShareware = FALSE`, potentially 24-bit color
- **`area.cpp:3345`**, **`bridge.cpp:440`**, **`netcmd.cpp:1074`**: `#ifndef _GG` guards strict ASSERTs — GG build was more relaxed
- **`lastplnt.cpp:1307`**: `#ifndef _GG && 0` — disabled CD speed test

---

## 18. `HACK_TEST_AI` — AI Spectator Mode

**`base.h:24`**: `// #define HACK_TEST_AI 1`

When enabled at `lastplnt.cpp:2238`, disables ALL main menu buttons (Create, Join, Load, Load Multi), forcing AI-only "spectator" mode for testing.

---

## 19. Diagnostic Macros

| Macro | Purpose | Status |
|---|---|---|
| `LOGGINGON` | Extensive diagnostic logging | `#ifdef` guarded, never defined |
| `_LOG_LAG` | Network lag debugging | `// #define` — commented out |
| `_LOGOUT` | Pathfinding/routing log output | Referenced in `#ifdef` blocks |
| `_CHEAT` | Debug/cheat build | Used in ~30 locations |

---

## 20. Unused Sound Assets

- **Motorcycle**: `motorcycle_idle/go/running` — three sound states, no vehicle
- **24 shooting sounds** (`shoot_0` through `shoot_23`): Suggests many planned weapon types
- **5 explosion sounds** (`explosion_0` through `explosion_4`)
- **12 cutscene voices** (`cut_1` through `cut_12`)

---

## 21. MFC → SDL2 Migration

~25+ MFC dialogs replaced. Old IDs remain in `resource.h`:

| Old | SDL2 Replacement |
|---|---|
| `IDD_MAIN` | `SDL2MainMenu` (#if 0'd MFC original) |
| `IDD_RESEARCH` | `SDL2ResearchDialog` |
| `IDD_SCENARIO` | `SDL2ScenarioDialog` |
| `IDD_FILE` / `IDD_FILE1` | `SDL2FileDialog` |
| `IDD_OPTIONS` / `IDD_OPTIONS1` | `SDL2Options` |
| `IDD_CREATE_SINGLE/MULTI/PUBLISH` | SDL2 creation flows |
| `IDD_JOIN_GAME/PLAYERS/PUBLISH` | SDL2 join flows |
| `IDD_PICK_RACE` | `SDL2PickRaceDialog` |
| `IDD_RELATIONS` | `SDL2RelationsDialog` |
| `IDD_BUILD_STRUCTURE/TRANSPORT` | `SDL2BuildStructure/Transport` |

---

## 22. Other Abandoned Dialogs

| Dialog | Purpose |
|---|---|
| `IDC_MAIN_CAMPAIGN` | Campaign button — no code |
| `IDD_GPF_DATA` | Win95 crash reporter |
| `IDD_PLAY_FLIC` | FLIC animation player |
| `IDD_TEST_SOUNDS` | Dev sound test (removed) |
| `IDD_MAIL_COMPOSE` | Email compose |
| `IDD_DIALOG_RAND` | Random number dev tool? |
| `IDD_PICK_WAIT` | "Please wait" dialog |
| `IDD_MSG_STATUS` | Message status |
| `IDD_MODELESS_MSG` | Modeless message box |
| `IDD_AI_POS` | AI position debug |

---

## 23. Event Placeholders

**`event.h:84-87`**:
```cpp
EVENT_EMPT1,
EVENT_EMPT2,
EVENT_EMPT3, // "in case i didn't add enough"
```

Literal placeholder events with a candid developer comment.

---

## 24. Terrain Types

15 terrain types in `CHex` (`terrain.h:68-84`):
```
city, desert, forest, lake, hill, mountain, ocean, plain,
river, road, rough, swamp, coastline, fields, resources
```
`fields` grows around farms. `resources` is a special view overlay. `steep` placement constraint (`terrain.h:367`).

---

## 25. Publish/Unpublish Game System

Working game-lobby system (`CNetPublish`, `CNetUnpublish`):
- `IDD_CREATE_PUBLISH`, `IDD_JOIN_PUBLISH` dialogs
- `netapi.cpp:3449-3497` — publish with name, password, settings
- `IDS_UNPUBLISH` — remove published game

Used VDMPLAY session enumeration for lobby discovery.

---

## Summary Table

| # | Feature | Status | Impact |
|---|---|---|---|
| 1 | Triple name change | Second Chance → Last Planet → Enemy Nations | Historical |
| 2 | Copper → Xilitium | Incomplete rename | Cosmetic |
| 3 | Air units (5 types) | Research only, no units/buildings | AI wastes points |
| 4 | Phantom vehicles (7 types) | AI tasks exist, no enum entries | Battleship, bridger, etc. |
| 5 | Espionage system | Fully stubbed, spy commented out | Dead AI goals |
| 6 | Diplomacy/Trade | Tasks defined, no implementation | Dead AI goals |
| 7 | Abandoned buildings (6 types) | AI tasks only | Emerg Response, Goods Factory, etc. |
| 8 | Tier 3 buildings | Marked obsolete | Cut from gameplay |
| 9 | AI threading | All `#if 0`'d | Performance loss |
| 10 | Partial AI functions | 5+ functions `#if 0`'d mid-write | Incomplete task force AI |
| 11 | In-game chat | All `#if 0`'d | No chat |
| 12 | Old network protocols | 6 of 7 abandoned | TCP/IP only |
| 13 | Shareware limits | Bypassed in port | N/A |
| 14 | CD DRM / Registration | Disabled | N/A |
| 15 | "Moly" / "Goods" | Devs unsure of purpose | Undefined mechanics |
| 16 | Comm research | No effect | Dead tech tree items |
| 17 | `_GG` build flag | Developer-specific | Historical |
| 18 | AI spectator mode | `HACK_TEST_AI` | Dev feature |
| 19 | Diagnostic macros | LOGGINGON, _LOG_LAG, _CHEAT | Debug-only |
| 20 | Motorcycle audio | 3 sounds, no vehicle | Unused assets |
| 21 | MFC dialogs | Replaced by SDL2 | Migration complete |
| 22 | Abandoned dialogs | ~10 unused resource IDs | Dead UI |
| 23 | Event placeholders | EVENT_EMPT1/2/3 | "in case i didn't add enough" |
| 24 | Campaign mode | Button ID exists, no code | Planned single-player campaign |
| 25 | Game publish lobby | Working but legacy | Pre-matchmaking |

---

*Deep code analysis against `enations_latest/src`. Old `enations/src` snapshot not consulted.*
