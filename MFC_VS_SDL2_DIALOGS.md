# MFC Original vs SDL2 Replacement — Complete In-Game Dialog Reference

Generated 2026-05-23 from source analysis of `enations_latest/src`.
Last verified 2026-05-24 — three "Actionable Gaps" that had been listed
as missing were found already implemented in source and are now marked
DONE in §29.

**Total: 23 MFC dialog classes replaced or reimplemented in SDL2, plus 6 removed/cheat-only dialogs.**

---

## Table of Contents

### Pre-Game / Main Menu Dialogs
1. [CDlgMain → SDL2MainMenu](#1-cdlgmain--sdl2mainmenu)
2. [CDlgVer → SDL2VersionDialog](#2-cdlgver--sdl2versiondialog)
3. [CDlgAdvOptions → SDL2AdvancedOptionsDialog](#3-cdlgadvoptions--sdl2advancedoptionsdialog)
4. [CDlgCreateSingle → SDL2CreateSingleDialog](#4-cdlgcreatesingle--sdl2createsingledialog)
5. [CDlgPickRace → SDL2PickRaceDialog](#5-cdlgpickrace--sdl2pickracedialog)
6. [CDlgScenario → SDL2ScenarioDialog](#6-cdlgscenario--sdl2scenariodialog)
7. [CDlgPickPlayer → SDL2PickPlayerDialog](#7-cdlgpickplayer--sdl2pickplayerdialog)
8. [CDlgCreateMulti + CDlgCreatePublish → SDL2CreateNetDialog](#8-cdlgcreatemulti--cdlgcreatepublish--sdl2createnetdialog)
9. [CDlgJoinPublish + CDlgJoinGame → SDL2JoinNetDialog](#9-cdlgjoinpublish--cdlgjoingame--sdl2joinnetdialog)

### In-Game Panels & Dialogs
10. [CDlgBuildStructure → SDL2BuildStructure](#10-cdlgbuildstructure--sdl2buildstructure)
11. [CDlgBuildTransport → SDL2BuildTransport](#11-cdlgbuildtransport--sdl2buildtransport)
12. [CDlgResearch → SDL2ResearchDialog](#12-cdlgresearch--sdl2researchdialog)
13. [CDlgRelations → SDL2RelationsDialog](#13-cdlgrelations--sdl2relationsdialog)
14. [CDlgDiscover → SDL2DiscoverDialog](#14-cdlgdiscover--sdl2discoverdialog)
15. [CDlgLoadTruck → SDL2LoadTruckDialog](#15-cdlgloadtruck--sdl2loadtruckdialog)
16. [CDlgPause → SDL2PauseDialog](#16-cdlgpause--sdl2pausedialog)
17. [CDlgCompose → SDL2ComposeDialog](#17-cdlgcompose--sdl2composedialog)
18. [CDlgChatAll + CWndChat → SDL2ChatWindow](#18-cdlgchatall--cwndchat--sdl2chatwindow)
19. [CWndCutScene → SDL2CutSceneDialog](#19-cwndcutscene--sdl2cutscenedialog)
20. [CWndCredits → SDL2_RunCredits](#20-cwndcredits--sdl2_runcredits)
21. [CDlgPlyrList → SDL2PlayerListDialog](#21-cdlgplyrlist--sdl2playerlistdialog)
22. [CDlgFile → SDL2FileDialog / SDL2FileBrowser / SDL2SaveDialog](#22-cdlgfile--sdl2filedialog--sdl2filebrowser--sdl2savedialog)
23. [CDlgOptions → SDL2OptionsDialog](#23-cdlgoptions--sdl2optionsdialog)

### Modeless Overlays (Non-Dialog Refactors)
24. [CDlgCreateStatus → SDL2CreateStatus](#24-cdlgcreatestatus--sdl2createstatus)
25. [CDlgSaveMsg (Modeless)](#25-cdlgsavemsg-modeless)
26. [CDlgModelessMsg (Modeless)](#26-cdlgmodelessmsg-modeless)
27. [CDlgLicense → MessageBox-based](#27-cdlglicense--messagebox-based)

### Removed / Cheat-Only Dialogs
28. [Dialogs Removed Entirely](#28-dialogs-removed-entirely)

### Cross-Cutting Changes
29. [Summary of Cross-Cutting Changes](#29-summary-of-cross-cutting-changes)

---

## 1. CDlgMain → SDL2MainMenu

**MFC origin:** `lastplnt.h` / `lastplnt.cpp` (`#if 0 // MFC_LEGACY_MAIN_MENU`)

**SDL2 replacement:** `SDL2MainMenu.cpp/.h`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Base class** | `CDialog` with `IDD_MAIN` resource | `SDL2Dialog`-based, renders on SDL window |
| **Buttons** | MFC `CBitmapButton` from DIB art | Same button art, same layout (`_btnData` array) |
| **Wallpaper** | Loaded from MISC data file | Same MISC data file, rendered via SDL_Texture |
| **Flow dispatch** | Modal dialog returns ID, caller switches | Same flow, menu returns button index |

**Status:** ✅ Replaced. SDL2MainMenu is the sole main-menu path.

---

## 2. CDlgVer → SDL2VersionDialog

**MFC origin:** `lastplnt.h` (declared, body excluded)

**SDL2 replacement:** `SDL2Dialogs.h` — `SDL2VersionDialog`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Content** | Version/build info from .rc string table | Same info, SDL2-rendered text |
| **Layout** | Simple OK dialog | Same — title + version text + OK button |

**Status:** ✅ Replaced.

---

## 3. CDlgAdvOptions → SDL2AdvancedOptionsDialog

**MFC origin:** `lastplnt.h` (declared, body excluded)

**SDL2 replacement:** `SDL2Dialogs.h` — `SDL2AdvancedOptionsDialog`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Content** | Advanced game options (AI, world, etc.) | Same settings, SDL2 widgets |

**Status:** ✅ Replaced.

---

## 4. CDlgCreateSingle → SDL2CreateSingleDialog

**MFC origin:** `creatsin.h` (removed in Phase 2d)

**SDL2 replacement:** `SDL2Dialogs.h` — `SDL2CreateSingleDialog`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Flow** | MFC modal dialog | Driven by `SDL2_RunCreateSinglePlayerFlow()` |
| **Fields** | AI level, world size, num AI players | Same fields: radio groups + edit boxes |

**Status:** ✅ Replaced.

---

## 5. CDlgPickRace → SDL2PickRaceDialog

**MFC origin:** `new_game.h` (removed in Phase 2d)

**SDL2 replacement:** `SDL2Dialogs.h` — `SDL2PickRaceDialog`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Purpose** | Choose player race before game start | Same |
| **Layout** | Race buttons from DIB art | Same art, SDL2-rendered |

**Status:** ✅ Replaced.

---

## 6. CDlgScenario → SDL2ScenarioDialog

**MFC origin:** `scenario.h` (removed in Phase 2d)

**SDL2 replacement:** `SDL2Dialogs.h` — `SDL2ScenarioDialog`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Flow** | MFC modal dialog | Driven by `SDL2_RunCreateScenarioFlow()` |
| **Content** | Scenario selection list | Same |

**Status:** ✅ Replaced.

---

## 7. CDlgPickPlayer → SDL2PickPlayerDialog

**MFC origin:** `new_game.h` / `creatmul.h` (removed in Phase 2d)

**SDL2 replacement:** `SDL2Dialogs.h` — `SDL2PickPlayerDialog`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Purpose** | Select player slot when loading a game | Same |
| **Fields** | Player list, name edit, description | `SDL2Listbox` + `SDL2EditBox` + `SDL2Label` |
| **Result** | `m_iSelectedPlyrNum` + `m_playerName` | Same public members |

**Status:** ✅ Replaced.

---

## 8. CDlgCreateMulti + CDlgCreatePublish → SDL2CreateNetDialog

**MFC origin:** `creatmul.h` (both removed in Phase 2d)

**SDL2 replacement:** `SDL2Dialogs.h` — `SDL2CreateNetDialog`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Combined** | Two separate MFC dialogs | Single SDL2 dialog |
| **Flow** | MFC modal dialogs (TCP/IP only) | `SDL2_RunCreateNetworkFlow()` |
| **Fields** | AI level, world size, start pos, num AI, game name, player name, port | Same fields |

**Status:** ✅ Replaced. TCP/IP only (same as original).

---

## 9. CDlgJoinPublish + CDlgJoinGame → SDL2JoinNetDialog

**MFC origin:** `join.h` (both removed in Phase 2d)

**SDL2 replacement:** `SDL2Dialogs.h` — `SDL2JoinNetDialog`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Combined** | Two separate MFC dialogs | Single SDL2 dialog |
| **Flow** | MFC modal dialogs (TCP/IP only) | `SDL2_RunJoinNetworkFlow()` |
| **Fields** | Player name, server address, port | Same |

**Status:** ✅ Replaced.

---

## 10. CDlgBuildStructure → SDL2BuildStructure

**MFC origin:** `unit_wnd.cpp` lines 810–1370 (`#if 0 // MFC_LEGACY_BUILD_DIALOGS`)

**SDL2 replacement:** `SDL2BuildStructure.cpp/.h`

### Widget Inventory

| Widget | MFC (IDC_*) | SDL2 |
|--------|------------|------|
| Category buttons | 6× `CBitmapButton` (IDC_BUILD_LIST_CAT, IDs 100–105) | 6× `SDL2Button` (m_catBtns[6]) |
| Building buttons | 6× `CBitmapButton` (IDC_BUILD_LIST_BLDGS, IDs 110–115) | 6× `SDL2Button` (m_bldgBtns[6]) |
| Build button | `CButton` (IDOK) — 98×23 px | `SDL2Button` (m_btnBuild) |
| Cancel button | `CButton` (IDCANCEL) — 98×23 px | `SDL2Button` (m_btnCancel) |
| Description text | GDI `TextOut`/`DrawText` in `OnPaint()` | `SDL2Label` (m_lblDesc) |
| Cost header | GDI drawn in `OnPaint()` | `SDL2Label` (m_lblCostHdr) |
| Cost lines | GDI multi-line in `OnPaint()` | `SDL2Label` (m_lblCosts) |
| Operating costs | GDI multi-line in `OnPaint()` | `SDL2Label` (m_lblOper) |
| Building icon | Drawn via `OnDrawItem()` from `DIB_LIST_UNIT_BUILDINGS` | `SDL2Button` icon sheet |

### Layout Coordinates (MFC)

| Element | MFC Rect / Position | SDL2 |
|--------|-------------------|------|
| Dialog size | 465 × 345 client | 465+12 × 345+12+26 (with frame) |
| Category buttons | (11,22)..(115,71) stepped +50 Y each | Same |
| Building buttons | (129,22)..(233,71) stepped +50 Y each | Same |
| Build button | (249, 300, 98, 23) | Same |
| Cancel button | (359, 300, 98, 23) | Same |
| Description rect | (246, 19, 457, 286) | Same |

### Art DIB Indices (MFC)

| Asset | DIB Index |
|-------|----------|
| Background | `DIB_STRUCTURE_BKGND` |
| Category button art | `DIB_STRUCTURE_BTNS_1` |
| Building button art | `DIB_STRUCTURE_BTNS_2` |
| OK/Cancel button art | `DIB_STRUCTURE_BTNS_3` |
| Building icon sheet | `DIB_LIST_UNIT_BUILDINGS` |

### Behavioral Differences

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Modality** | Modal (`CDialog::DoModal`) — blocks game loop | Non-modal — can stay open during gameplay |
| **Message handling** | `MSG_BTN_CLICKED` / `MSG_BTN_DBLCLK` custom messages via `ON_MESSAGE` | Direct virtual method overrides (`SelectCategory`, `SelectBuilding`) |
| **Double-click** | `OnDblClk()` — builds immediately on double-click | Not implemented (single-click select + Build button) |
| **Draw** | `OnDrawItem()` per-button owner-draw for icons | Pre-rendered icon sheets applied to button widgets |
| **Palette** | `OnPaletteChanged`/`OnQueryNewPalette` for 256-color mode | Not needed (SDL2 always true-color) |

**Key UX gap:** MFC original supported **double-click to build** on building buttons. SDL2 version requires selecting then clicking Build.

**Status:** ✅ Replaced. `m_pSdlBuild` in vehicle.h replaces `m_pDlgStructure`.

---

## 11. CDlgBuildTransport → SDL2BuildTransport

**MFC origin:** `unit_wnd.cpp` lines 1398–1770 (`#if 0 // MFC_LEGACY_BUILD_DIALOGS`)

**SDL2 replacement:** `SDL2BuildTransport.cpp/.h`

### Widget Inventory

| Widget | MFC (IDC_*) | SDL2 |
|--------|------------|------|
| Vehicle buttons | 6× `CBitmapButton` (IDC_BUILD_LIST, IDs 110–115) | 6× `SDL2Button` (m_vehBtns[6]) |
| Build button | `CButton` (IDOK) — 84×23 px | `SDL2Button` (m_btnBuild) |
| Cancel button | `CButton` (IDCANCEL) — 84×23 px | `SDL2Button` (m_btnCancel) |
| Quantity edit | `CEdit` (IDC_BUILD_NUM) — 40×22 px | `SDL2EditBox` (m_edtNum) |
| Spin control | `CSpinButtonCtrl` (IDC_BUILD_SPIN) — 15×22 px | `SDL2Slider` (m_sldNum) |
| Description text | GDI `DrawText` in `OnPaint()` | `SDL2Label` (m_lblDesc) |
| Cost header/lines | GDI in `OnPaint()` | `SDL2Label` (m_lblCosts) |
| Status icon | `CStatusIcon` (ICON_BUILD_VEH) | Custom render from same icon |
| Vehicle icon | `OnDrawItem()` from `DIB_LIST_UNIT_VEHICLES` | `SDL2Button` icon sheet |

### Layout Coordinates (MFC)

| Element | MFC Rect / Position | SDL2 |
|--------|-------------------|------|
| Dialog size | 380 × 332 client | 380+12 × 332+12+26 (with frame) |
| Vehicle buttons | (10,24)..(118,72) stepped +48 Y each | Same |
| Build button | (133, 258, 84, 23) | Same |
| Cancel button | (282, 258, 84, 23) | Same |
| Quantity edit | (282, 295, 40, 22) | Same |
| Spin control | (307, 295, 15, 22) | Same |
| Description/text rect | (128, 20, 369, 251) | Same |
| Status rect | (134, 292, 253, 318) | Same |

### Art DIB Indices (MFC)

| Asset | DIB Index |
|-------|----------|
| Button background | `DIB_VEHICLE_BKGND` |
| Button art states | `DIB_VEHICLE_BTNS_2` |
| OK/Cancel button art | `DIB_VEHICLE_BTNS_1` |
| Vehicle icon sheet | `DIB_LIST_UNIT_VEHICLES` |
| Status icon | `ICON_BUILD_VEH` |

### Behavioral Differences

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Modality** | `DoModal` — blocks game loop | Non-modal — stays open during gameplay |
| **Double-click** | `OnDblClk` via `MSG_BTN_DBLCLK` — builds immediately | Not implemented |
| **Barracks title** | `SetWindowText(IDS_BUILD_PEOPLE)` when building troops | Implemented via `SDL2BuildTransport::OnInit` title check |
| **Spin range** | `SetRange(1, UD_MAXVAL)` — 1 to 32767 | Same range, slider widget |
| **Unit update** | `UpdateStatus(int iPer)` updates build progress icon | Same callback, refreshes progress widget |

**Key UX gap:** MFC original supported **double-click to build** on vehicle buttons. SDL2 version requires selecting then clicking Build.

**Status:** ✅ Replaced. `m_pSdlBuildTransport` in building.h replaces `m_pDlgTransport`.

---

## 12. CDlgResearch → SDL2ResearchDialog

**MFC origin:** `research.h/cpp` (removed from build in Phase 2d)

**SDL2 replacement:** `SDL2GameDialogs.h` — `SDL2ResearchDialog`

### MFC Widget Inventory

| Widget | MFC Control | Purpose |
|--------|------------|---------|
| Category list | `CListBox` (IDC_RESEARCH_CAT) | 8 research categories |
| Topics list | `CListBox` (IDC_RESEARCH_TOPICS) | Available research items for selected category |
| Description | `CStatic` (IDC_RESEARCH_DESC) | Multi-line description of selected topic |
| Progress text | `CStatic` (IDC_RESEARCH_PROGRESS) | "X% complete" for current research |
| Start button | `CButton` (IDOK) | Begin/switch research |

### SDL2 Widget Mapping

| MFC Widget | SDL2 Replacement |
|-----------|-----------------|
| IDC_RESEARCH_CAT | `SDL2Listbox` (m_list) — combined cat+items in one scoped list |
| IDC_RESEARCH_DESC | `SDL2Label` (m_lblDesc) |
| IDC_RESEARCH_PROGRESS | `SDL2Label` (m_lblProgress) |
| IDOK | `SDL2Button` (m_btnStart) |

### Data State

| Class | Purpose | Status |
|-------|---------|--------|
| `CRsrchArray` | Tracks which research items are available/completed | Unchanged |
| `CRsrchItem` | Individual research topic data | Unchanged |
| `CRsrchStatus` | Current research progress (item index, percent) | Unchanged |
| `CResearchData` | Static research tree definitions | Unchanged |

**Idiom preserved:** The SDL2 version calls `theResearch.SetStatus()` identical to the MFC version.

**Status:** ✅ Replaced.

---

## 13. CDlgRelations → SDL2RelationsDialog

**MFC origin:** `relation.h/cpp` (removed from build)

**SDL2 replacement:** `SDL2GameDialogs.h` — `SDL2RelationsDialog`

### MFC Widgets

| Widget | MFC Control | Purpose |
|--------|------------|---------|
| Player list | `CListBox` (IDC_RELATION_LIST) | List of other players |
| Relation radios | 3× `CButton` BS_AUTORADIOBUTTON (Neutral/Enemy/Ally) | Set diplomatic stance |
| Player info | `CStatic` (IDC_RELATION_INFO) | Player race, name, status summary |
| OK button | `CButton` (IDOK) | Confirm changes |

### SDL2 Mapping

| MFC Widget | SDL2 Replacement |
|-----------|-----------------|
| IDC_RELATION_LIST | `SDL2Listbox` (m_list) |
| Relation radios | `SDL2RadioGroup` (m_radRelations) — 3 options |
| IDC_RELATION_INFO | `SDL2Label` (m_lblInfo) |

**Idiom preserved:** The SDL2 version calls `NewRelations(int playerIndex, int relationLevel)` — same free function that the MFC `CDlgRelations` class called via its static method.

**Status:** ✅ Replaced.

---

## 14. CDlgDiscover → SDL2DiscoverDialog

**MFC origin:** `research.h` — `CDlgDiscover` declared, body excluded

**SDL2 replacement:** `SDL2GameDialogs.h` — `SDL2DiscoverDialog`

**Minimal dialog** with only: title text, description text, and an OK button. No widget inventory beyond that.

**Trigger:** `theResearch.CompleteResearch()` calls `SDL2_RunDiscovery()` instead of MFC's `CDlgDiscover::DoModal()`.

**Status:** ✅ Replaced.

---

## 15. CDlgLoadTruck → SDL2LoadTruckDialog

**MFC origin:** `unit_wnd.cpp` (excluded from build)

**SDL2 replacement:** `SDL2GameDialogs.h` — `SDL2LoadTruckDialog`

### MFC Load/Unload Interface

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Purpose** | Load/unload infantry from transport vehicle | Same |
| **Available list** | `CListBox` of units in building/at location | `SDL2Listbox` |
| **Loaded list** | `CListBox` of units in vehicle | `SDL2Listbox` |
| **Material sliders** | 6 resource types × quantity | `SDL2Slider` array (m_sliders[6]) |
| **Load button** | Single unit load | Same |
| **Unload button** | Single unit unload | Same |
| **Auto button** | Load until full | Same |
| **Material readout** | Per-resource text labels | `SDL2Label` array (m_lblAmounts[6]) |

### Constructor — Vehicle Context

| MFC | SDL2 |
|-----|------|
| `CDlgLoadTruck(CWnd*, CVehicle*)` | `SDL2LoadTruckDialog(GameWindow*, CVehicle*)` |
| Auto-detects parent building via `m_pVeh->GetBldgPar()` | Same |

**Status:** ✅ Replaced.

---

## 16. CDlgPause → SDL2PauseDialog

**MFC origin:** `ui.h` / `ui.cpp` — modeless `CDialog`-based popup

**SDL2 replacement:** `SDL2GameDialogs.h` — `SDL2PauseDialog`

**Minimal dialog** — just a "Game Paused" message string. No widget table needed.

**Trigger:** `theGame.Pause()` / `SDL2_HandlePause()` → shows this dialog; pressing Pause again or clicking closes it.

**Status:** ✅ Replaced. `CDlgPause` class still exists as stub in `ui.h` but no longer inherits `CDialog`.

---

## 17. CDlgCompose → SDL2ComposeDialog

**MFC origin:** `ipcsend.h/cpp` (excluded from build)

**SDL2 replacement:** `SDL2GameDialogs.h` — `SDL2ComposeDialog`

### Widget Inventory

| Widget | MFC (IDC_*) | SDL2 |
|--------|------------|------|
| Recipient list | `CListBox` (IDC_MAIL_TO) | `SDL2Listbox` (m_recipientList) |
| Subject edit | `CEdit` (IDC_MAIL_SUBJECT) | `SDL2EditBox` (m_editSubject) |
| Body edit | `CEdit` multiline (IDC_MAIL_BODY) | `SDL2EditBox` multiline (m_editBody) |
| Send button | `CButton` (IDOK) | `SDL2Button` (m_btnSend) |
| Cancel button | `CButton` (IDCANCEL) | `SDL2Button` (m_btnCancel) |

**Trigger:** `EnMessage` system → `SDL2ComposeDialog(CGameWnd* gw)` instead of `new CDlgCompose(&theApp.m_wndMain)`.

**Status:** ✅ Replaced.

---

## 18. CDlgChatAll + CWndChat → SDL2ChatWindow

**MFC origin:** `chat.h/cpp` (excluded from build), `chatbar.h` (`CChatBar` — `CDialogBar` subclass, unused in SDL2)

**SDL2 replacement:** `SDL2GameDialogs.h` — `SDL2ChatWindow`

### Widget Inventory

| Widget | MFC (IDC_*) | SDL2 |
|--------|------------|------|
| Message history | `CListBox` (IDC_CHAT_HISTORY) | `SDL2Listbox` (m_msgList) |
| Input field | `CEdit` (IDC_CHAT_MSG) | `SDL2EditBox` (m_editMsg) |
| Send button | `CButton` (IDC_CHAT_SEND) | `SDL2Button` (m_btnSend) |

### Architecture Note

MFC had **two** chat classes:
- `CDlgChatAll` — modal dialog for "chat to all players" 
- `CWndChat` — modeless chat window (in `chatbar.h` as `CChatBar`, a `CDialogBar`)

SDL2 replaces both with a single `SDL2ChatWindow` (non-modal).

**Status:** ✅ Replaced. `CDlgChatAll` excluded; `CChatBar` declared but unused.

---

## 19. CWndCutScene → SDL2CutSceneDialog

**MFC origin:** `cutscene.h/cpp`

**SDL2 replacement:** `SDL2GameDialogs.h` — `SDL2CutSceneDialog`

### Scene Type Enum (Identical)

| Value | Name | Meaning |
|-------|------|---------|
| 0 | `cut` | Intro/transition cut scene |
| 1 | `repeat` | Replay scenario prompt |
| 2 | `scenario_end` | End-of-scenario summary |
| 3 | `win` | Victory screen |
| 4 | `lose` | Defeat screen |

### Button Visibility by Type

| Type | OK | Cancel | Save |
|------|----|--------|------|
| cut (0) | ✅ | ✅ | ❌ |
| repeat (1) | ✅ | ✅ | ❌ |
| scenario_end (2) | ✅ | ✅ | ✅ |
| win (3) | ✅ | ✅ | ❌ |
| lose (4) | ✅ | ✅ | ❌ |

### Return Values (Identical)

| Return | Name | Meaning |
|--------|------|---------|
| 1 | `CUT_OK` | Player clicked OK/Continue |
| 2 | `CUT_CANCEL` | Player clicked Cancel/Quit |


## 20. CWndCredits → SDL2_RunCredits

**MFC origin:** `credits.cpp`

**SDL2 replacement:** `SDL2Dialogs.h` — `SDL2_RunCredits(GameWindow*)`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Type** | CWnd-based window with scrolling text | Free function, full-screen SDL2 render |
| **Content** | Scrolling credits text | Same |

**Status:** ✅ Replaced.

---

## 21. CDlgPlyrList → SDL2PlayerListDialog

**MFC origin:** `lastplnt.h` (removed in Phase 2d)

**SDL2 replacement:** `SDL2GameDialogs.h` — `SDL2PlayerListDialog`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Purpose** | Show all players with status in multiplayer | Same |
| **Layout** | Player list + info label | `SDL2Listbox` + `SDL2Label` |

**Status:** ✅ Replaced.

---

## 22. CDlgFile → SDL2FileDialog / SDL2FileBrowser / SDL2SaveDialog

**MFC origin:** `unit_wnd.cpp` / `player.h` (removed in Phase 2d)

**SDL2 replacements:**

| SDL2 Class | Purpose |
|-----------|---------|
| `SDL2FileBrowser` | Native OS file open/save (uses system dialogs) |
| `SDL2FileDialog` | In-game file menu (Save, Load, Options, Speed, Sound, Music, Exit) |
| `SDL2SaveDialog` | Save game with name entry |

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **File browser** | MFC `CFileDialog` | `SDL2FileBrowser` (platform-native) |
| **In-game menu** | CDlgFile with 7 buttons | SDL2FileDialog with same 7 options |
| **Save dialog** | MFC modal with edit box | SDL2SaveDialog with `SDL2EditBox` |

**Status:** ✅ Replaced. All three paths functional.

---

## 23. CDlgOptions → SDL2OptionsDialog

**MFC origin:** `options.cpp` (excluded)

**SDL2 replacement:** `SDL2Options.h/.cpp` — `SDL2OptionsDialog`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Purpose** | Game settings (speed, sound, music, etc.) | Same |
| **Widgets** | MFC sliders, checkboxes, radio buttons | SDL2 equivalents |
| **Persistence** | Writes to .ini / registry | Same |

**Status:** ✅ Replaced.

---

## 24. CDlgCreateStatus → SDL2CreateStatus

**MFC origin:** `new_game.h` (removed)

**SDL2 replacement:** `SDL2CreateStatus.h/.cpp`

| Aspect | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Type** | Non-modal progress dialog | Non-modal SDL2 overlay |
| **Purpose** | "Creating world..." progress bar | Same |
| **Key method** | `SetPer(int percent, BOOL bYield)` | Same signature |
| **Cancel** | Throws `ERR_TLP_QUIT` if user cancels | Same behavior |

**Status:** ✅ Replaced.

---

## 25. CDlgSaveMsg (Modeless)

**MFC origin:** `ui.h` — originally `CDialog`-based

**Current state:** Class still exists but no longer inherits `CDialog`. Uses a Win32 popup window.

| Aspect | MFC Original | Current |
|--------|-------------|---------|
| **Base** | `CDialog` | Standalone class |
| **Purpose** | Modeless save progress indicator | Same |

**Status:** ⚠️ Refactored to remove CDialog inheritance. Functional with Win32 popup.

---

## 26. CDlgModelessMsg (Modeless)

**MFC origin:** `DlgMsg.h` — originally `CDialog`-based

**Current state:** Class still exists but no longer inherits `CDialog`. Uses a Win32 popup window.

| Aspect | MFC Original | Current |
|--------|-------------|---------|
| **Base** | `CDialog` | Standalone class |
| **Purpose** | Auto-closing notification popup | Same |
| **Persistence** | Remembers "don't show again" per entry | Same via `EnMessageBoxOnce()` |

**Status:** ⚠️ Refactored to remove CDialog inheritance.

---

## 27. CDlgLicense → MessageBox-based

**MFC origin:** `license.h/cpp`

**Current state:** No longer inherits `CDialog`. Uses `MessageBoxA` to display license text from LANG data.

| Aspect | MFC Original | Current |
|--------|-------------|---------|
| **Base** | `CDialog` | Standalone class |
| **Content** | License text from LANG data | Same via `MessageBoxA` |

**Status:** ⚠️ Simplified to MessageBox. Functional.

---

## 28. Dialogs Removed Entirely (MFC-Coupled Cheat/Debug Tools)

These dialogs were removed because they are **MFC-coupled** — they inherit `CDialog`, use `.rc` dialog
template resources (`IDD_*` constants), MFC `DDX_*` data-exchange macros, and MFC message maps
(`BEGIN_MESSAGE_MAP` / `ON_BN_CLICKED`). Porting them to SDL2 would require a from-scratch
rewrite of each dialog. Since most are also `#ifdef _CHEAT` debug tools (not player-facing),
the cost/benefit of porting was deemed too low.

### Cheat/Debug Dialogs (`#ifdef _CHEAT`)

| MFC Class | Purpose | MFC Coupling |
|-----------|---------|-------------|
| `CDlgRandNum` | Override random-number seed at world-gen time. Gated behind `theApp.GetProfileInt("Debug", "SetRand", 0)`. Defined in `newworld.cpp`. | `CDialog`, `IDD_RAND_EDIT`, `DDX_Text`, message map |
| `CDlgAiPos` | Show AI starting positions as a selectable list during world generation. Gated behind `theApp.GetProfileInt("Debug", "ShowAIStart", 0)`. Defined in `newworld.cpp`. | `CDialog`, `IDD_LIST_AI` / `IDOK`, `DDX_Control`, message map with `ON_LBN_DBLCLK` |
| `CDlgStats` | Display per-player statistics (food, etc.) from the relation screen. Opened via `new CDlgStats(&theApp.m_wndMain)`. Defined in `relation.cpp`. | `CDialog`, `IDD_STAT_FOOD`, `DDX_Text`, message map |

All three are wrapped in `#ifdef _CHEAT` / `#endif` in the original codebase and were
never visible to end users. They could be recreated as SDL2 debug overlays if needed,
but their MFC implementations cannot be reused.

### Exception Handler Dialog

| MFC Class | Purpose | MFC Coupling |
|-----------|---------|-------------|
| `CDlgStackDump` | Display exception stack trace with a "Copy to Clipboard" button. Defined in `lastplnt.cpp`. | `CDialog`, `IDD_STACK_DUMP` / `IDC_COPY_STACK`, `DDX_Text`, message map |

This was used in the MFC exception-handling path (`CConquerApp::ProcessException`).
The SDL2 port has a different exception path, and the dialog has no live callers in the
current codebase. A native SDL2 crash reporter could replace it if desired.

### Dead / Unreachable Dialogs

| MFC Class | Reason |
|-----------|--------|
| `CDlgCdLoc` | "CD not found — insert disc" prompt in `CdLoc.h/cpp`. `CheckForCD()` always returns `TRUE` in the SDL2 port (no CD check), making this dialog permanently unreachable. |
| `CDlgScnDesc` | Declared in `cutscene.h` but never defined or instantiated in any `.cpp` file. Placeholder that was never implemented even in the MFC codebase. |

### Additional Build Exclusions (Phase 2d)

| Class | Reason |
|-------|--------|
| `CDlgPlayerList` | Multiplayer pre-game waiting room. The SDL2 network flow drives this state directly without a dedicated dialog. |
| `CChatBar` | `CDialogBar` subclass in `chatbar.h`. Unused in the SDL2 port — `SDL2ChatWindow` handles all chat UI. |

---

## 29. Summary of Cross-Cutting Changes

| Pattern | MFC Original | SDL2 Replacement |
|--------|-------------|-----------------|
| **Base class** | `CDialog`, `CDialogBar`, `CWnd`, `CFormView` | `SDL2Dialog`, standalone classes, or free functions |
| **Resource loading** | `.rc` dialog templates, `IDD_*` constants | All coordinates/computed in code |
| **Widgets** | MFC `CButton`, `CListBox`, `CEdit`, `CStatic`, `CBitmapButton` | `SDL2Button`, `SDL2Listbox`, `SDL2EditBox`, `SDL2Label`, `SDL2RadioGroup` |
| **Rendering** | GDI via `CDC*`, `OnPaint()`, `CPaintDC` | SDL2 via `SDL_Renderer*`, `SDL_Texture` |
| **Art loading** | DIB sections loaded from MISC data file | Same DIB art, converted to `SDL_Texture` |
| **Modal behavior** | `CDialog::DoModal()` blocks game loop | SDL2 modal loops pump events manually |
| **Event dispatch** | MFC message maps (`ON_BN_CLICKED`, etc.) | Virtual method overrides (`OnOK()`, `OnInit()`) |
| **Text rendering** | GDI `TextOut()`, `DrawText()` | `TextRenderer` via SDF fonts or bitmap fonts |
| **Parent window** | `CWnd* pParent`, `theApp.m_wndMain` | `GameWindow*` or `SDL2Panel*` |

### Architectural Patterns Preserved

- **Button layout coordinates** — matched exactly from original `CRect` values
- **Dialog flow order** — single player flow, network flow, scenario flow all preserved
- **Data structures** — `CRsrchArray`, `CRsrchItem`, `CRsrchStatus`, `CNetPlyrStatus` unchanged
- **Return value semantics** — `IDOK`/`IDCANCEL` → `true`/`false` or dedicated result members
- **Art assets** — same DIB sections, same MISC data file, same button images
- **Persistence** — same .ini/registry save patterns for settings and "don't show again" flags

### Not Yet Replaced

- **CDlgModelessMsg** — still uses Win32 popup (not SDL2-rendered)
- **CDlgSaveMsg** — still uses Win32 popup (not SDL2-rendered)
- **CDlgLicense** — uses MessageBoxA (not SDL2-rendered)

### Actionable Gaps — Features Present in MFC But Missing in SDL2

Revised 2026-05-24 after verifying each claim against the source. Items
1, 2, and 9 were marked DONE — they had been implemented before this
doc was written.

| # | Gap | MFC Behavior | SDL2 State | Priority |
|---|-----|-------------|-----------|---------|
| 1 | **Build dialog double-click** | Double-clicking a building/vehicle button in `CDlgBuildStructure`/`CDlgBuildTransport` triggers `OnDblClk()` and immediately builds the item | ✅ **DONE.** `SDL2BuildStructure.cpp:88-89` and `SDL2BuildTransport.cpp:81-82` wire `SetOnDblClick` to call `OnBuild()` when the same slot is double-clicked. | — |
| 2 | **BuildStructure "Cancel" handled as "Destroy"** | `OnCancel()` in MFC calls `SendMessage(WM_COMMAND, ID_UNIT_DESTROY)` plus `DestroyWindow()`, aborting selection AND issuing a destroy command | ✅ **DONE.** `SDL2BuildStructure::OnCancel()` (lines 358-371) calls `pUnit->SetDestroyUnit()` and posts `CMsgDestroyUnit` for network games. | — |
| 3 | **CDlgPause is still a Win32 popup** | MFC modeless dialog with game-paused notice | SDL2 replaces with `SDL2PauseDialog` rendered in SDL. (#16) | Low — already functional |
| 4 | **CDlgSaveMsg still uses Win32** | MFC `CDialog`-based progress indicator during saves | Current implementation uses Win32 popup, not SDL2 overlay. (#25) | Low — cosmetic |
| 5 | **CDlgModelessMsg still uses Win32** | MFC `CDialog`-based auto-dismiss notification | Current implementation uses Win32 popup. (#26) | Low — cosmetic |
| 6 | **CDlgLicense uses MessageBoxA** | MFC `CDialog` showing license text from LANG data | Current implementation uses `MessageBoxA`. (#27) | Very Low — rarely seen |
| 7 | **BuildStructure resizes on open** | `OnInitDialog()` calls `SetWindowPos` to expand the dialog based on content, and `m_btnBuild`/`m_btnCancel` are repositioned | SDL2 version hard-codes dialog size; no dynamic resize | Low — edge case, only affects rare large-cost buildings |
| 8 | **MFC palette management removed** | `OnPaletteChanged`/`OnQueryNewPalette` in BuildStructure, BuildTransport, LoadTruck for 256-color mode | SDL2 always true-color — not needed | None — intentional simplification |
| 9 | **Double-click to select race** | `CDlgPickRace` supported `ON_LBN_DBLCLK` to immediately confirm selection | ✅ **DONE.** `SDL2Dialogs.cpp:224` wires the listbox `DblClickCallback` to select the race and call `OnOK()` if the button is enabled. | — |
| 10 | **Cheat dialogs entirely gone** | `CDlgRandNum`, `CDlgAiPos`, `CDlgStats` accessible via `_CHEAT` build + registry flags | Removed. No SDL2 replacement. (#28) | Low — debug-only |

### Verification Checklist

- [x] All 23 replaced dialog sections verified against MFC source (`unit_wnd.cpp`, `lastplnt.cpp`, `research.cpp`, etc.)
- [x] All widget counts verified against `.rc` dialog templates and MFC `OnInitDialog` implementations
- [x] All layout coordinates verified from the MFC source (`SetWindowPos`, `CRect` constants)
- [x] All art indices verified from `OnDrawItem()` and `theBitmaps.GetByIndex()` calls
- [x] Behavioral differences documented (modality, double-click, cancel behavior)
- [x] Cross-cutting patterns table reviewed and expanded
- [x] Removed/cheat dialogs table verified against build exclusions and `#ifdef _CHEAT` guards

### Quick Reference — File Mapping

| MFC File | SDL2 File |
|----------|----------|
| `lastplnt.h/cpp` (CDlgMain) | `SDL2MainMenu.cpp/.h` |
| `lastplnt.h/cpp` (CDlgVer) | `SDL2Dialogs.h` |
| `lastplnt.h/cpp` (CDlgAdvOptions) | `SDL2Dialogs.h` |
| `creatsin.h/cpp` (CDlgCreateSingle) | `SDL2Dialogs.h` |
| `new_game.h/cpp` (CDlgPickRace) | `SDL2Dialogs.h` |
| `scenario.h/cpp` (CDlgScenario) | `SDL2Dialogs.h` |
| `new_game.h/cpp` (CDlgPickPlayer) | `SDL2Dialogs.h` |
| `creatmul.h/cpp` (CDlgCreateMulti/Publish) | `SDL2Dialogs.h` |
| `join.h/cpp` (CDlgJoinPublish/Game) | `SDL2Dialogs.h` |
| `unit_wnd.cpp` (CDlgBuildStructure) | `SDL2BuildStructure.cpp/.h` |
| `unit_wnd.cpp` (CDlgBuildTransport) | `SDL2BuildTransport.cpp/.h` |
| `unit_wnd.cpp` (CDlgLoadTruck) | `SDL2GameDialogs.h` |
| `research.h/cpp` (CDlgResearch) | `SDL2GameDialogs.h` |
| `research.h` (CDlgDiscover) | `SDL2GameDialogs.h` |
| `relation.h/cpp` (CDlgRelations) | `SDL2GameDialogs.h` |
| `ui.h/cpp` (CDlgPause) | `SDL2GameDialogs.h` |
| `ipcsend.h/cpp` (CDlgCompose) | `SDL2GameDialogs.h` |
| `chat.h/cpp` (CDlgChatAll) | `SDL2GameDialogs.h` |
| `cutscene.h/cpp` (CWndCutScene) | `SDL2GameDialogs.h` |
| `credits.cpp` (CWndCredits) | `SDL2Dialogs.h` (free function) |
| `lastplnt.h` (CDlgPlyrList) | `SDL2GameDialogs.h` |
| `unit_wnd.cpp` (CDlgFile) | `SDL2FileDialog.cpp/.h` |
| `options.cpp` (CDlgOptions) | `SDL2Options.h/.cpp` |
| `new_game.h` (CDlgCreateStatus) | `SDL2CreateStatus.h/.cpp` |
| `license.h/cpp` (CDlgLicense) | Still in `license.h/cpp` (MessageBox-based) |
| `DlgMsg.h` (CDlgModelessMsg) | Still in `DlgMsg.h` (Win32 popup) |
