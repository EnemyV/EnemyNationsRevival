# Enemy Nations: MFC → SDL2 Port Guide

A complete handoff document for an agent picking up the migration of *Enemy Nations* (1996, Windward Studios) from a Win32/MFC/DirectDraw codebase to a portable SDL2 implementation. Captures everything learned from ~167 commits of incremental work.

---

## Table of contents

1. [The goal](#the-goal)
2. [What is done](#what-is-done)
3. [What is left](#what-is-left)
4. [The 6-phase plan](#the-6-phase-plan)
5. [Core strategy: exclude, don't delete](#core-strategy-exclude-dont-delete)
6. [Architecture: how the codebase is layered](#architecture-how-the-codebase-is-layered)
7. [SDL2 toolkit reference](#sdl2-toolkit-reference)
8. [Pattern catalog](#pattern-catalog)
9. [Gotchas and traps](#gotchas-and-traps)
10. [Build, test, commit workflow](#build-test-commit-workflow)
11. [Phase-by-phase guidance](#phase-by-phase-guidance)
12. [Specific file landmarks](#specific-file-landmarks)
13. [User preferences](#user-preferences)

---

## The goal

> **"to get the game running in sdl2 with no mfc"**

Every tactical decision serves that single goal. When a side-quest looks attractive (e.g. cleaning up unused code), check it against this north star. The user has corrected agents who chipped away at low-value targets (CString-counting in dead code, refactoring style) instead of moving the needle on MFC removal.

**MFC removal complete** = the linker output of `enations.exe` has no `mfc*.dll` import. That requires:

- `CMAKE_MFC_FLAG 2` removed from `enations_latest/src/CMakeLists.txt`
- `_AFXDLL` macro removed from compile definitions
- Every `#include <afx*.h>` gone or behind a guard
- Every reachable code path uses non-MFC types (no `CString`, `CFile`, `CRect`, `CDialog`, `CWnd`, `CWinApp`, `CList`, `CMap`, ...)

The current binary still depends on MFC because Phase 5 isn't fully done.

---

## What is done

| Area | Status | Notes |
|---|---|---|
| **Phase 2 — MFC dialog/window removal** | ~95% (live runtime: 100%) | 19 of 25 classes removed/excluded; remaining 5 are dead at runtime |
| **Phase 4a — Registry shim** | DONE | `EnGetProfileInt/EnWriteProfileInt` replace `theApp.GetProfileInt/WriteProfileInt` |
| **Phase 4b — LoadString shim** | DONE | `EnLoadStdString` wraps `Win32 LoadStringA`; CString shim deleted |
| **Phase 5 — CString purge** | DONE for live code (0 refs) | 21 remaining refs are: comments (15), SaveCompat.h infrastructure (6). |
| **Phase 2d-cont — Chat cluster structural exclusion** | DONE (2026-05-11) | ipcchat/ipccomm/ipcread/ipcsend/chatbar .cpp dropped from build. ipccomm.h replaced with minimal non-MFC stub CWndComm. CIPCPlayer::m_pwndChat retyped to void*. |
| **Phase 4c prep — small MFC API removals** | DONE (2026-05-11) | AfxBeginThread → _beginthreadex (sprtinit.cpp); AfxOleGetUserCtrl removed (mainloop.cpp); m_pLogFile CFile* → FILE* (lastplnt.cpp). |
| **Phase 1 wind22 recon** | DONE (2026-05-11) | Hierarchy + surface mapped; 2-3 day implementation plan in [phase1_wind22_strip_plan.md](file:///C:/Users/tyboy/.claude/projects/d--Enemy-Nations-src/memory/phase1_wind22_strip_plan.md). CDIBWnd at wind22/include/dibwnd.h is the non-MFC template. |
| **SDL2 dialog toolkit** | DONE | SDL2Dialog, SDL2Button, SDL2Label, SDL2EditBox, SDL2Listbox, SDL2RadioGroup, SDL2Checkbox, SDL2Slider, SDL2Image, SDL2GroupBox |
| **SDL2 game window infrastructure** | DONE | GameWindow, Compositor, Panels, RenderingAdapter, MainMenu, Toolbar |
| **SDL2 video player** | DONE | Replaces `CWndMovie` (Indeo .avi → MPEG-1 .mpg via pl_mpeg) |
| **SDL2 audio** | DONE | SDL_mixer migration |
| **MFC compat header** | IN PLACE BUT DORMANT | `windward/wind22/include/mfc_compat.h` provides drop-in CRect/CPoint/CSize, gated by `#ifndef __AFX_H__` |

**Removed/excluded MFC dialogs (run before this guide):**

CDlgVer, CDlgLoadTruck, CDlgTestSounds, CDlgReg, CDlgRandNum, CDlgStats, CDlgCdLoc, CDlgDiscover, CDlgScnDesc, CDlgFlic, CDlgStackDump, CDlgAiPos, CDlgOptions, CDlgAdvOptions, CDlgRelations, CDlgCreateStatus, CDlgScenario, CDlgCreateSingle, CDlgCreateMulti, CDlgCreatePublish, CDlgLoadMulti, CDlgJoinPublish, CDlgJoinGame, CDlgJoinPlayers, CDlgPickRace, CDlgPickPlayer, CDlgPickWait, CDlgPlayerList, CDlgPlyrList, CPlyrMsgStatusDlg, CDlgFile, CDlgResearch, CDlgBuildStructure, CDlgBuildTransport, CDlgMain, CWndMovie, CDlgChatAll.

---

## What is left

### Major work
- **Phase 4c — `CConquerApp : CWinApp` removal.** The app singleton still inherits CWinApp. Removing it requires reimplementing WinMain, accelerator handling, message-loop entry. 37 callsites across 12 files. See the Phase 4c section below for the full reconnaissance and per-member replacement table.
- **Phase 3 — Drop CWnd from live gameplay windows.** ~20 CWnd-derived classes still in the binary (CWndMain, CWndWorld, CWndBar, CWndArea, CWndRoute, CWndCredits, CWndCutScene, CWndInfo, CWndStatBar, CWndStatLine, CWndUnitStat, CWndAreaStatic, CWndListBuildings, CWndListVehicles, CWndListUnits, CUnitButton, CMyButton). Many inherit from wind22's CWndBase/CWndAnim/CWndPrimary — so Phase 3 is tightly coupled to Phase 1 (wind22 MFC strip). Could be done in one bang by changing wind22's CWndBase to a non-MFC stub.
- **Phase 1 — Strip MFC from wind22.** Originally gated by Phase 5; with Phase 5 now done for live code, this is unblocked. The wind22 library still has `CMAKE_MFC_FLAG 2`. Replacing wind22's CWnd-derived base classes with non-MFC stubs unblocks both Phase 3 and Phase 1g.

### Secondary work
- **CList / CMap / CArray** (40 sites in 19 files): Phase 5d. Mostly mechanical `using` aliases or template replacements with `std::list/unordered_map/vector`.
- **Debug macros**: `ASSERT/ASSERT_STRICT → assert()`, `TRACE → SDL_Log/OutputDebugStringA`, `VERIFY`, `DEBUG_NEW`. Phase 5e.
- **CObject inheritance**: CIPCPlayer, CIPCPlayerList, CAIGoal, CAITask, CAIGoalList, CAITaskList, CAISavLd, CGame, CPlayer all inherit CObject. The DECLARE_SERIAL macros tie into MFC's CArchive serialization. Phase 5c partial work — needs a non-MFC serialization replacement first.

---

## The 6-phase plan

The full plan lives at `C:\Users\tyboy\.claude\plans\hidden-waddling-petal.md`. Summary:

| Phase | What | Risk | Status |
|---|---|---|---|
| 1 | Strip MFC from wind22 (keep CDIB/CMmio/codecs/BTree) | Medium | Pending — unblocked now |
| 2 | Replace 48 CDialog + 21 CWnd/CFrameWnd/CDialogBar | Low-Med | ~98% — chat cluster excluded; only inherited CWnd-derived gameplay windows remain (=Phase 3) |
| 3 | Drop CWnd from live gameplay windows | High | Pending — ~20 classes; tightly coupled to Phase 1 |
| 4 | Replace registry / LoadString / CWinApp | High | 4a ✅ 4b ✅ 4c pending |
| 5 | Replace CString (99) / CRect (58) / CFile (58) | Very High | 5a ✅ 5b compat ready 5c ✅ for live code (caisavld done) |
| 6 | Cross-platform (Linux/macOS) | Medium | Pending |

**Phase order matters**: Phase 1g (remove `_AFXDLL` from wind22) must come *after* Phase 5 because wind22 functions take MFC types as parameters, called by game code that still uses MFC. They have to use the same types until both sides flip simultaneously.

---

## Core strategy: exclude, don't delete

**User correction (2026-05-03)**: "the goal is to get it running without mfc. we dont need to delete, just not include for now."

Practical implications:

1. **Drop the .cpp from `enations_latest/src/CMakeLists.txt`** — preferred when the file is whole-class scope (`movie.cpp`, `chat.cpp`, `pickrace.cpp`).
2. **Wrap the class body in `#if 0 // MFC_LEGACY_<NAME>`** — when the .cpp also has live SDL2 code (`unit_wnd.cpp` has both `CDlgBuildTransport` and `CWndRoute`).
3. **Delete only files that were already commented-out dead code** before the MFC pass started (e.g. `advdlg.cpp`, `loadtruk.cpp` — both had `#if 0` wrappers from a prior incarnation).
4. **Strip live call-site references** so the rest of the codebase compiles. Member pointers (`CConquerApp::m_pdlgFile`) get removed; `ShowWindow/Create/Destroy` push notifications become no-ops.

Why: keeping the source on disk preserves the historical record. The .RC dialog templates capture the original layouts (use them as visual specs when porting to SDL2). Linker exclusion is enough to remove MFC dependency.

---

## Architecture: how the codebase is layered

```
enations_latest/src/        ← game code (this is what you edit)
  lastplnt.cpp/h            ← CConquerApp singleton (CWinApp)
  main.cpp                  ← CWndMain (the main game window)
  area.cpp/h                ← gameplay area window
  world.cpp/h               ← world (map) window
  toolbar.cpp/h             ← toolbar window
  unit*.cpp/h               ← CUnit, CBuilding, CVehicle hierarchy
  netapi.cpp/h              ← VDMPLAY networking
  player.cpp/h              ← CGame state (massive file, 80% of save logic)
  cai*.cpp/h                ← AI (separate threads)
  SDL2*.cpp/h               ← new SDL2 code (don't edit MFC paths to "fix" SDL2)

windward/wind22/             ← support library (DIB, MMIO, codecs, BTree)
  include/                   ← headers; mfc_compat.h is here
  src/                       ← implementation
  src/scanlist.cpp           ← inline-asm hot path; ignore the persistent C4731 warnings

tools/sdl2/                  ← SDL2-2.30.12 + SDL2_ttf + SDL2_mixer + glew
tools/vdmplay/               ← VDMPLAY (network library, MFC-dependent)
cmakeBuild/                  ← MSVC project files (cmake-generated)
  enations_latest/src/Release/enations.exe   ← build output
```

### Key classes and where state lives

- **`CConquerApp theApp`** (lastplnt.h:217) — application singleton, equivalent to a service locator.
  Owns `m_gameWindow` (SDL2), `m_sdlMainMenu`, `m_pCreateGame` (game-creation state holder), ResIni path, font, palette, etc.
- **`CGame theGame`** (player.h:550) — game state: players, all units, save filename, scenario index, network state.
  This is what `Serialize` reads/writes — touching the layout breaks save-file compat.
- **`CWndMain m_wndMain`** (CConquerApp member) — the main game window. In SDL2 path it's hidden (`SW_HIDE`) and the SDL2 GameWindow takes over rendering. The HWND is still created for Win32 messaging plumbing (timers, accelerator routing, etc.).
- **`SDL2MainMenu`** (SDL2MainMenu.cpp) — replaces the MFC main menu.
- **`SDL2Compositor`** (SDL2Compositor.cpp) — owns the wallpaper and routes input/render to panels.
- **`GameWindow`** (GameWindow.cpp) — wraps SDL_Window + SDL_Surface; provides `PollEvents()` (called from `CConquerApp::BaseYield`).

### The dual-build oddity

The repo contains two trees:
- `enations_latest/src/` — the live tree we work on
- `enations/src/` — older snapshot (don't edit; sometimes useful for historical reference)

There's also a `.dsp` / `.mak` / `.bak` set of legacy MSVC 6 project files in `enations_latest/src/`. These are not used by the cmake build. The `lastplnt.clw` file (ClassWizard) is dead but not deleted.

---

## SDL2 toolkit reference

Located in `enations_latest/src/SDL2*.cpp/h`. Key types:

### Widgets

- **`SDL2Dialog`** (SDL2UI.h) — modal base class. Override `OnInit()`, optionally `OnOK()`. Call `DoModal()` to run. Returns 1 on OK, 0 on Cancel.
- **`SDL2Button`** — standard button with click callback.
- **`SDL2Label`** — text label; `SetWrapped(true)` for multi-line.
- **`SDL2EditBox`** — single-line text input with optional callback on change. `GetText()` returns `std::string`.
- **`SDL2Listbox`** — list with selection + double-click callbacks. `AddItem(const char*, void* userData)`.
- **`SDL2RadioGroup`** — vector of radio buttons.
- **`SDL2Checkbox`** — boolean toggle.
- **`SDL2Slider`** — value slider.
- **`SDL2Image`** — surface display; `SetSurface(SDL_Surface*, bool owned)`.
- **`SDL2GroupBox`** — visual-only frame (render before contained widgets).

### Non-modal patterns

- **`SDL2CreateStatus`** — non-modal progress dialog with cancel. Driven externally by `SetMsg()/SetPer()`.
- **`SDL2BuildStructure` / `SDL2BuildTransport`** — non-modal build dialogs: `ShowNonModal(callback)` with disposal callback.
- **`SDL2RouteWindow`** — full-screen panel that lives in the compositor.

### Flow helpers

In `SDL2Dialogs.cpp`:
- `SDL2_RunCreateSinglePlayerFlow(GameWindow*)`
- `SDL2_RunCreateScenarioFlow(GameWindow*)`
- `SDL2_RunLoadSinglePlayerFlow(GameWindow*)`
- `SDL2_RunCreateNetworkFlow(GameWindow*)` (placeholder)
- `SDL2_RunJoinNetworkFlow(GameWindow*)` (placeholder)
- `SDL2_RunLoadNetworkFlow(GameWindow*)` (placeholder)
- `SDL2_RunCredits(GameWindow*)`

These functions construct the same `CCreateSingle/CCreateScenario` orchestrator state classes the MFC path used, then run SDL2 dialogs in sequence (via `DoModal()`), then call `theApp.ReadyToCreate()` to engage the real game-setup logic.

### Visual fidelity

User feedback (2026-05-02): **"the new ones should match the visuals and functionality of the old ones"**.

When porting an MFC dialog, read its `.RC` template (find with `grep IDD_NAME enations_latest/src/lastplnt.rc -A 30`). The template gives:
- Pixel positions (in dialog units; convert: 1 DLU x ≈ 1.4 px, 1 DLU y ≈ 1.6 px at 8pt MS Sans Serif)
- Control list and labels
- Style flags (`WS_GROUP`, `BS_AUTORADIOBUTTON` etc.)
- The CAPTION string

Then read the dialog's MFC source (`OnInitDialog`, `DoDataExchange`, message map handlers) for behavior. The `DDX_*` calls are direct member ↔ control mappings; `OnXXX` handlers are click/select callbacks.

Match the visual style: dark purple title bar (RGB 42,22,65), gold interior (DIB_GOLD asset), blue label text (RGB 48,58,148), gold button text (RGB 225,182,55). The art assets are in the game's `.DAT` file under `MISC` chunks; `bitmaps.h` exposes them.

---

## Pattern catalog

Concrete idioms for the most common conversions.

### CString → std::string

| MFC | std::string equivalent |
|---|---|
| `CString s;` | `std::string s;` |
| `s = "literal";` | `s = "literal";` (works) |
| `s.Empty()` / `s.IsEmpty()` | `s.empty()` |
| `s.GetLength()` | `(int)s.length()` (cast usually needed for `__max`) |
| `s.GetBuffer(N+2); ... ReleaseBuffer(N)` | `s.resize(N+2); ... s.resize(N)` |
| `s.Format("%s", arg)` | `s = strPrintf("%s", arg)` |
| `(LPCSTR)s` / `(const char*)s` | `s.c_str()` |
| `csPrintf(&pCstr, fmt, args)` | `pStr = strPrintf(fmt, args)` |
| `EnLoadString(IDS_X)` | `EnLoadStdString(IDS_X)` (shim removed) |
| `EnGetProfileString(...)` | `EnGetProfileStdString(...)` (shim removed) |
| `ar << s; ar >> s;` (CArchive) | **blocker** — Phase 5c work |
| `ASSERT_VALID_CSTRING(&s)` | remove the line |

For class members, also remove the corresponding `ASSERT_VALID_CSTRING` from the class's `AssertValid()` method.

### Dialog body removal (`#if 0` wrap)

```cpp
/////////////////////////////////////////////////////////////////////////////
// CDlgFooBar — excluded from build (Phase 2d). Replaced by SDL2FooBar.
#if 0  // MFC_LEGACY_FOOBAR

CDlgFooBar::CDlgFooBar(CWnd* pParent /*=NULL*/)
    : CDialog(CDlgFooBar::IDD, pParent) { ... }

// ... all method bodies ...

#endif // MFC_LEGACY_FOOBAR
```

### Whole-file exclusion

Edit `enations_latest/src/CMakeLists.txt`, replace the source file line with a comment:

```cmake
        chat.cpp
        chatbar.cpp
        # movie.cpp excluded (Phase 2d) — CWndMovie replaced by SDL2VideoPlayer
        netapi.cpp
```

After CMakeLists.txt changes, run `MSBuild ZERO_CHECK.vcxproj` first to regenerate, then build the main project.

### Strip member pointer references

Member: `CConquerApp::m_pdlgFile` (CDlgFile*). Caller pattern:

```cpp
// Before
if (theApp.m_pdlgFile != NULL && theApp.m_pdlgFile->m_hWnd != NULL)
    theApp.m_pdlgFile->SetState();

// After (replacement is modal, no live update needed)
// CDlgFile removed (Phase 2d) — SDL2FileDialog rebuilds state on each open.
```

For destruction sites (`if (m_pdlg != NULL) { m_pdlg->DestroyWindow(); m_pdlg = NULL; }`): replace with comment.

Then remove the member declaration from the header. Then remove the `m_pdlg = NULL` from the constructor.

### Wiring an SDL2 dialog at a call site

```cpp
// MFC pattern:
m_pdlgRsrch = new CDlgResearch(&m_wndMain);
m_pdlgRsrch->Create(IDD_RESEARCH, &m_wndMain);

// SDL2 modal replacement:
SDL2ResearchDialog dlg(theApp.m_gameWindow.get());
dlg.DoModal();
```

The MFC path was modeless (lived alongside game windows); SDL2 path is modal. Visually equivalent, simpler lifecycle.

### CMmio::ReadString overload

`CMmio::ReadString(std::string&)` is already defined in `windward/wind22/include/mmio.inl:146`. Just convert the member to `std::string` and the call site keeps working without changes.

### Network message structure literals

`CNetPublish::Alloc` builds variable-length packets:

```cpp
int iLen = sizeof(CNetPublish) + 2 + (int)pCm->m_sName.length() + ...;
// ...
strcpy(pMsg->m_sPlyrName, pCm->m_sName.c_str());
char* pBuf = pMsg->m_sPlyrName + strlen(pMsg->m_sPlyrName) + 1;
strcpy(pBuf, pCm->m_sPw.c_str());
```

When converting a CString member to std::string, fix `GetLength()` → `(int)length()` and `strcpy(buf, cstring)` → `strcpy(buf, str.c_str())`.

---

## Gotchas and traps

Concrete issues that bit us and the fixes.

### Build / tooling

1. **PDB collision under parallel msbuild.** Spawning two background MSBuild calls produces `error C1041: cannot open program database`. Wait for `cl.exe` processes to drain (or `Stop-Process -Name cl,msbuild -Force`) before retrying.

2. **Build command must be PowerShell + Select-String.** `bash`-invoked builds swallow output silently (the redirected stderr from native exes wraps each line in a NativeCommandError ErrorRecord). Always:
   ```powershell
   $msb='C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe'
   & $msb 'D:\Enemy Nations\src\cmakeBuild\enations_latest\src\enations.vcxproj' \
       /p:Configuration=Release /p:Platform=Win32 /v:m 2>&1 |
       Select-String -Pattern 'error C[0-9]|fatal error|FAILED|enations\.exe ->' |
       Select-Object -First 30
   ```

3. **CMake regen needed when files are dropped from CMakeLists.txt.** Run `MSBuild ZERO_CHECK.vcxproj` (in `cmakeBuild/`) first. Otherwise the build still tries to compile the deleted file.

4. **LF/CRLF git warnings are noise.** Windows git auto-converts. Don't try to fix.

5. **Persistent benign warnings to ignore:**
   - `scanlist.cpp(205,254): warning C4731: ... ebp modified by inline assembly` — wind22 hot path, intentional.
   - `caidata.cpp(669) / lastplnt.cpp(1230): warning C4067: unexpected tokens following preprocessor directive` — comment-on-preprocessor-line glitch, harmless.

### MFC-isms that look wrong

6. **`#define new DEBUG_NEW` after includes.** Many .cpp files have this. It's MFC's allocation-tracking hack. Leave it.

7. **`THIS_FILE` macros at the top of .cpp.** MFC's debug-message file pinning. Leave it.

8. **`PURE_FUNC` on the same line as the next declaration.** `unit.h:479` had:
   ```cpp
   virtual BOOL IsHit( CHexCoord, CPoint ) const PURE_FUNC virtual void GetDesc( CString& sText ) const PURE_FUNC
   ```
   Two declarations smashed into one line. When changing `GetDesc(CString&)` to `GetDesc(std::string&)`, you have to edit this exact line — it's not a typo.

9. **`csPrintf(CString*, fmt, ...)` only takes CString.** When converting a member to std::string, `csPrintf(&dlgMsg.m_sText, ...)` won't compile. Replace with:
   ```cpp
   m_sText = strPrintf(EnLoadStdString(IDS_X).c_str(), arg.c_str());
   ```
   `strPrintf` is defined in `EnSettings.h`.

10. **`ar << CString` and `ar >> CString` go through MFC CArchive serialization.** This is the Phase 5c blocker. You can't unilaterally convert a `CString` member that's `Serialize`d — the binary save format has to migrate too. Skip those members for Phase 5a.

### SDL2 pitfalls

11. **`SDL_PushEvent` for cross-thread signalling.** AI threads run on background pthreads (`AI_THREADS_ENABLED` define). They cannot touch SDL state directly. Use SDL custom events to marshal back to the main thread.

12. **`SDL_GetWindowSurface` returns a surface owned by SDL.** Don't `SDL_FreeSurface` it. Free is automatic when the window is destroyed.

13. **TTF_Font caching is per-size.** `SDL2CreateStatus::GetFont(int size)` keeps an `unordered_map<int, TTF_Font*>`. Don't open fonts in a hot path.

14. **Pre-converted assets live next to enations.exe.** Logo/intro use `assets\videos\logo.mpg` + `intro.mpg` (MPEG-1 + MP2 audio). The original Indeo 4 .avi files were re-encoded to MPEG-1 for `pl_mpeg` portability. Don't try to play .avi.

15. **`SDL_WINDOW_ALWAYS_ON_TOP` plus `SDL_WINDOW_SKIP_TASKBAR`** is the standard pattern for modal-feeling SDL dialogs (used by `SDL2CreateStatus`).

### Game-state pitfalls

16. **`theGame.Open(TRUE)`** must be called before the SDL2 pre-game flow runs. The MFC flows did this in `CCreateSingle::Init()`; the SDL2 flows have to do it explicitly because `Init()` is now a no-op.

17. **`m_pCreateGame` is a global state holder.** `theApp.m_pCreateGame` must be NULL when entering a Create* flow, and explicitly `delete`'d when the flow aborts. This is the MFC-era contract; the SDL2 flows respect it.

18. **`theApp.m_gameWindow` is the SDL2-active sentinel.** Code paths gate on `if (theApp.m_gameWindow) { /* SDL2 */ } else { /* MFC */ }`. When excluding MFC bodies, the `else` branch becomes dead — strip it.

19. **`theGame.Serialize` writes raw fixed-size `CString` members directly.** The save-file format depends on CString's binary layout. Phase 5c needs a careful migration, not a blind type swap.

20. **Network message handlers run on the main thread via `theGame.ProcessAllMessages()`.** Called from inside `BaseYield`. Don't do blocking work there.

### Class hierarchy / inheritance

21. **`CWndBase` inherits `CWnd`** in wind22. Phase 1f makes it not. Until then, all game windows that inherit `CWndBase` are still `CWnd`s. You can't drop the HWND for any game window until wind22 is fixed (Phase 1f), and Phase 1 is gated on Phase 5.

22. **`CCreateSingle/Multi/Scenario` are state holders, not dialogs.** They inherit `CCreateBase : public CObject`. They must be kept (used by SDL2 flows). The `CDlg*` dialog members on them are gone, but the orchestrators stay.

23. **Pure virtual signature changes cascade.** `CUnit::ShowStatusText(CString&)` was pure-virtual; changing it to `(std::string&)` forced updating all 11 derived classes simultaneously (CBuilding, CVehicle, CHousingBuilding, ...). Coordinate the change in one commit.

### Headers that pull MFC

24. **`stdafx.h` is the precompiled header.** It includes `afxwin.h`. Until `_AFXDLL` is removed (Phase 1g), every .cpp pulls all of MFC. Removing one `#include <afx.h>` from a header (like EnSettings.h) helps decouple but doesn't remove MFC from the build.

25. **`<windows.h>` vs `<afxwin.h>`.** Once Phase 1g is done, callers that need Win32 API directly (e.g. `LoadStringA`, `RegSetValueExA`) should use `<windows.h>`. EnSettings.cpp is already there.

### Chat cluster — special case

26. **The chat cluster (chatbar.cpp, ipcchat.cpp, ipccomm.cpp, ipcread.cpp, ipcsend.cpp) is dead at runtime but still in the build.** `CWndComm m_wndChat` is a value-type member of `CConquerApp` and is only constructed in `newworld.cpp` when `!m_gameWindow`, which never happens in the SDL2 path. The full chat-MFC tree compiles and links but never executes.

27. **Excluding the chat cluster is a ~60-call-site refactor.** `CIPCPlayer m_pwndChat` member, `CChatWnd*` references in network handlers, `CWndComm::On*` chat/email handlers — all need stubs. Either do this together with Phase 5c (when CFile/CArchive forces touching ipccomm.cpp anyway) OR replace the UI side with SDL2Chat that calls the same network senders.

### Build-time pitfalls

28. **The build is 32-bit (Win32).** SDL2 ships x86 libs in `tools/sdl2/SDL2-2.30.12/lib/x86`. Don't switch to x64 — VDMPLAY is 32-bit only.

29. **`/SAFESEH:NO` linker flag.** Required because some linked libs don't support it. In the CMakeLists.txt `target_link_options`.

30. **Post-build copies SDL2.dll, SDL2_mixer.dll, SDL2_ttf.dll, vdmplay.dll** to the output directory. If you add a new SDL2 component dependency, add the copy step.

---

## Build, test, commit workflow

### One-time

```powershell
# Generate cmakeBuild
cmake -B cmakeBuild -G "Visual Studio 17 2022" -A Win32
```

### Per-edit cycle

```powershell
$msb='C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe'

# After CMakeLists.txt change only
& $msb 'D:\Enemy Nations\src\cmakeBuild\ZERO_CHECK.vcxproj' /p:Configuration=Release /p:Platform=Win32 /v:m

# Main build
& $msb 'D:\Enemy Nations\src\cmakeBuild\enations_latest\src\enations.vcxproj' `
    /p:Configuration=Release /p:Platform=Win32 /v:m 2>&1 |
    Select-String -Pattern 'error C[0-9]|fatal error|FAILED|enations\.exe ->' |
    Select-Object -First 30
```

Successful build ends with: `enations.vcxproj -> D:\Enemy Nations\src\cmakeBuild\enations_latest\src\Release\enations.exe`

### Commit style

User preference: **no `Co-Authored-By` trailers**. One commit per logical conversion; commit messages reference the phase (e.g. `Phase 5a:`, `Phase 2d:`).

```bash
git -C '/d/Enemy Nations/src' commit -m "Phase 5a: <Class>::<member> CString to std::string

<one-line summary>
<file list with site count>"
```

Don't push to remote unless asked.

---

## Phase-by-phase guidance

### Phase 2d — Dialog removal (mostly done)

Remaining: chat-cluster windows. Pattern for each:

1. Find class definition. Find construction sites (`new C<X>` / member fields).
2. Find SDL2 replacement (search `SDL2*Dialog`).
3. If construction is gated `if (!m_gameWindow)`, the MFC path is dead — wrap the class body in `#if 0`.
4. Strip any push-notification call sites (`m_pdlg->Update*`, `m_pdlg->Set*`).
5. Build, fix any link errors (usually require adding `.c_str()` or removing static method calls).
6. Commit.

### Phase 5a — CString purge (mostly done)

Remaining: members blocked by CArchive serialization. Don't try to convert these in isolation; they need Phase 5c first.

For new `CString` references that show up in incoming code, convert immediately using the patterns above.

### Phase 5b — CRect/CPoint/CSize

The compat header is in place. To activate:

1. Make `_AFXDLL` and `CMAKE_MFC_FLAG` removable from CMakeLists.txt (gated on Phase 1g).
2. After Phase 5a + 5c land, replace the `_AFXDLL` define with `#include "mfc_compat.h"` in the precompiled header.
3. Rebuild — CRect/CPoint/CSize from the compat header take over. The interface matches MFC for the methods that are actually used (Width, Height, OffsetRect, IsRectEmpty, PtInRect, ...).

If the compat header is missing a method that the game uses, add it to `windward/wind22/include/mfc_compat.h`. The header is short (~130 lines).

### Phase 5c — CFile/CArchive (the big lift)

This is the gating work. Approach:

**Footprint:** 45 `CFile`, 119 `CArchive`, 71 `CFileException`, 6 `CMemFile` references across 42 files. Most live in `player.cpp` (CGame::Serialize), `caisavld.cpp` (AI save/load), `racedata.cpp` (CRaceDef::Serialize), `minerals.cpp`, `research.cpp`, `terrain.cpp`. Plus a handful of file-existence checks in `lastplnt.cpp`, `cailog.cpp`, etc.

**Two distinct usage classes:**

1. **Plain file I/O** — `CFile::GetStatus`, file-exists checks, raw `Read/Write(buf, len)`. These are easy to swap for Win32 (`GetFileAttributesExA`, `CreateFileA`) or `<filesystem>`. Pattern for size check (already applied in `lastplnt.cpp:861`):
   ```cpp
   WIN32_FILE_ATTRIBUTE_DATA wfad;
   if ( ::GetFileAttributesExA( name, GetFileExInfoStandard, &wfad ) ) {
       ULARGE_INTEGER size;
       size.HighPart = wfad.nFileSizeHigh;
       size.LowPart  = wfad.nFileSizeLow;
       // size.QuadPart now has the file size in bytes
   }
   ```

2. **CArchive serialization** — `Serialize(CArchive& ar)` methods + `ar << field`, `ar >> field` chains. This is the save-file format. Can NOT be replaced piecemeal because the binary format must remain stable for save-file compatibility.

**Phase 5c progress (introduced 2026-05-10): `SaveCompat.h` bridge.**

`enations_latest/src/SaveCompat.h` provides:

```cpp
inline CArchive& operator<<( CArchive& ar, const std::string& s ) {
    CString cs( s.c_str() ); ar << cs; return ar;
}
inline CArchive& operator>>( CArchive& ar, std::string& s ) {
    CString cs; ar >> cs; s.assign( (LPCSTR)cs ); return ar;
}
```

These overloads route std::string serialization through a temporary CString, which means **the on-disk binary format is byte-for-byte identical to the pre-migration CString format.** Saved files load correctly regardless of whether the member was CString or std::string when written.

Cost: one extra string copy per serialize boundary (the CString temp). Acceptable — serialization happens at save/load, not in a hot path.

To migrate a CArchive-serialized CString member:

1. `#include "SaveCompat.h"` in the .cpp that defines the class's Serialize method.
2. Change the member type from `CString` to `std::string` in the header.
3. Fix immediate call sites (`.IsEmpty()` → `.empty()`, `(LPCSTR)` casts → `.c_str()` where const char* is expected).
4. `ar << m_member` and `ar >> m_member` keep working — the new overloads kick in.
5. Build. Most files compile clean.

Already migrated this way (session 2026-05-10):
- `CGame::m_sFileName` / `m_sGameName` / `m_sGameDesc` / `m_sPwJoin`
- `CPlayer::m_sName`
- `CRaceDef::m_sLine` / `m_sDesc` + the file-header `static std::string sHdr`
- `CIPCPlayer::m_sName`

**Catalog the save format.** `theGame.Serialize` is the entry point in `player.cpp:~2680-2880`. It writes raw binary via `CFile::Write` / `CArchive::operator<<`. The format is NOT XML or JSON — it's raw `sizeof(struct)` writes plus length-prefixed strings.

2. **Replace `CFile` with `std::fstream` (binary mode).** API:
   ```cpp
   // CFile fil(path, CFile::modeRead | CFile::shareExclusive | CFile::typeBinary);
   std::ifstream fil(path, std::ios::binary);
   // CFile fil(path, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeBinary);
   std::ofstream fil(path, std::ios::binary | std::ios::trunc);
   ```

3. **Replace `CArchive` parameter with `std::fstream&`.** Provide `operator<<` / `operator>>` overloads for the few primitive types serialized (int, BYTE, std::string with length prefix).

4. **CRITICAL: maintain exact binary save compatibility.** Save a game in MFC build, load in std::string build (and vice-versa). Compare hex dumps if anything looks off.

5. **Files to touch:**
   - `player.cpp` (CGame::Serialize, CPlayer::Serialize, ~80% of the work)
   - `caisavld.cpp` (AI save/load — independent codepath)
   - `racedata.cpp` (CRaceDef::Serialize, header magic check)
   - `ipcplay.cpp` (CIPCPlayer::Serialize for chat-cluster compat)

6. **MMIO.h is separate** from CArchive. `CMmio` is wind22's reader for the game's `.DAT` data file. Don't conflate. CMmio doesn't depend on MFC for its read path; it just happens to use CString for its return types. CString → std::string overloads were already added (`mmio.inl:146`).

### Phase 4c — CWinApp removal

`CConquerApp : public CWinApp` is the last MFC inheritance in the live runtime. Phase 5 (CString purge) is now structurally complete, so 4c is the next major task.

**Exact CWinApp surface in use** (reconnaissance done 2026-05-11, 37 callsites across 12 files):

| Inherited member/method | Callsites | Replacement |
|---|---|---|
| `m_pMainWnd` (CWnd*) | 11 reads, 2 writes (lastplnt:1153, main:629) | Own as `CWnd* m_pMainWnd = nullptr` member; or replace with HWND once CWnd disappears |
| `m_pszAppName` (LPTSTR) | 1 write (lastplnt:380) | `std::string m_sAppName` member (already exists alongside) |
| `m_hInstance` (HINSTANCE) | 4 reads (lastplnt, area, world) | Own as `HINSTANCE m_hInstance` member; set from WinMain |
| `m_msgCur` (MSG) | 4 reads in mainloop.cpp | Own as `MSG m_msgCur` member |
| `m_hAccel` (HACCEL) | already an owned member | No change |
| `SetRegistryKey()` | 1 call (lastplnt:382) | No-op (EnSettings already handles registry path) |
| `AfxGetInstanceHandle()` | 3 calls (lastplnt) | `theApp.m_hInstance` |
| `AfxOleGetUserCtrl()` | 1 call (mainloop:81) | Return TRUE (no OLE in this game) |
| `AfxBeginThread()` | 1 call (sprtinit:2168) | `std::thread` or `_beginthreadex` |
| `InitInstance/ExitInstance/Run/PreTranslateMessage` virtuals | overrides | Become plain methods called by hand-written WinMain |
| `BEGIN_MESSAGE_MAP(CConquerApp, CWinApp)` | 1 (lastplnt:266) | Empty map, delete the macros |

**Implementation steps** (do in this order with a build check after each):

1. **Add owned shadow members alongside the inherited ones.** While CWinApp inheritance is still in place, add a parallel `m_hInstanceOwned`, `m_msgCurOwned`, etc. Don't switch callsites yet. (Build sanity.)

2. **Change `class CConquerApp : public CWinApp` to `class CConquerApp : public CWinAppStub`**, where CWinAppStub is a new header you provide. CWinAppStub declares the surface above as members — `CWnd* m_pMainWnd; HINSTANCE m_hInstance; MSG m_msgCur; LPTSTR m_pszAppName;` plus stub methods. No inheritance from MFC. Keep the BEGIN_MESSAGE_MAP path working by also defining empty stubs for the macros (or removing the macro invocation entirely).

3. **Write WinMain in a new file** `WinMain.cpp`. Calls `theApp.m_hInstance = hInstance; theApp.InitInstance(); theApp.Run(); theApp.ExitInstance();`. Drop the `CMAKE_MFC_FLAG`'s auto-WinMain.

4. **Replace `AfxBeginThread` callsite** in `sprtinit.cpp` with std::thread.

5. **Update the 11 `m_pMainWnd` reads** to handle nullptr gracefully (some already do via HWND checks).

6. **Verify enations.exe import table** — `dumpbin /imports Release\enations.exe | findstr mfc` should return nothing after this AND Phase 3 (CWnd removal in gameplay windows) are done.

**Blockers BEFORE Phase 4c can fully land:**
- The chat cluster's `CWndComm m_wndChat` member in CConquerApp is CWnd-derived; CWnd is gated on MFC. The chat cluster must be excluded structurally first (~60 stub call sites — see Phase 2d-cont).
- 8 other CWnd-derived member windows (`m_wndWorld`, `m_wndBar`, `m_wndBldgs`, `m_wndVehicles`, `m_wndMain`, `m_wndCredits`, `m_wndCutScene`) are part of Phase 3.

**Realistic estimate:** 3-5 focused sessions. Phase 4c on its own is one session; the prerequisite chat exclusion + Phase 3 CWnd removal in gameplay windows are the bigger chunks.

### Phase 1 — Strip MFC from wind22

Read the lessons-learned at the top of the original plan (`plans/hidden-waddling-petal.md`). Phase 1g (remove `CMAKE_MFC_FLAG` from wind22 CMakeLists.txt) cannot be done incrementally. Setting MFC flag off forces removing ALL MFC references in wind22 simultaneously: CString, CFile, CWnd, CList, CDC, CCriticalSection, CBitmap.

This is gated on Phase 5 because wind22 functions take MFC types as parameters, called by game code. Both sides have to flip together.

---

## Specific file landmarks

| File | What | Notes |
|---|---|---|
| `lastplnt.cpp:1576-1626` | CConquerApp::CreateMain | The SDL2-vs-MFC main-menu fork. SDL2 is the only path now. |
| `lastplnt.cpp:1697+` | GetDlgChat / CloseDlgChat | Stubs returning NULL. CDlgChatAll excluded. |
| `lastplnt.cpp:1963+` | CDlgMain | Wrapped in `#if 0 // MFC_LEGACY_MAIN_MENU`. ~800 lines of dead code kept for reference. |
| `main.cpp:1100-1125` | post-license intro | Plays SDL2VideoPlayer.PlayVideo("logo.mpg", "intro.mpg"). |
| `main.cpp:723-727` | CDlgFile excluded comment | The body was deleted (one of the few full deletes). |
| `unit_wnd.cpp:798-1944` | CDlgBuildStructure / CDlgBuildTransport | Wrapped in `#if 0 // MFC_LEGACY_BUILD_DIALOGS`. |
| `new_game.h:170-192` | CMultiBase | `m_wndPlyrList` is now a stub-struct, not CDlgPlayerList. |
| `creatmul.cpp` | CCreateMulti / CCreateLoadMulti | Pure state holders; Init() is no-op. |
| `creatsin.cpp` | CCreateSingle / CCreateLoadSingle | Pure state holders. |
| `pickrace.cpp` | DELETED | Replaced by SDL2PickRaceDialog + SDL2PickPlayerDialog. |
| `chat.cpp` | EXCLUDED FROM BUILD | (`# chat.cpp excluded` in CMakeLists.txt) |
| `movie.cpp` | EXCLUDED FROM BUILD | Replaced by SDL2VideoPlayer. |
| `chatbar.cpp` etc. | DEAD AT RUNTIME | Compiles and links but never runs. |
| `EnSettings.h/.cpp` | Win32-only settings shim | Replaces theApp.GetProfileInt + LoadStringA. No MFC dep. |
| `windward/wind22/include/mfc_compat.h` | CRect/CPoint/CSize replacements | Dormant; activates when afxwin.h is gone. |

---

## User preferences

Captured from explicit corrections during the work:

- **No `Co-Authored-By` trailers in commits.** ([feedback_commit_style.md](C:\Users\tyboy\.claude\projects\d--Enemy-Nations-src\memory\feedback_commit_style.md))
- **Build with PowerShell + MSBuild + Select-String.** cmd/bash swallow output silently. ([feedback_build_command.md](C:\Users\tyboy\.claude\projects\d--Enemy-Nations-src\memory\feedback_build_command.md))
- **SDL windows must be movable/resizable** like their MFC counterparts. ([feedback_sdl_windows_movable.md](C:\Users\tyboy\.claude\projects\d--Enemy-Nations-src\memory\feedback_sdl_windows_movable.md))
- **Exclude from build, don't delete source.** ([feedback_mfc_removal_strategy.md](C:\Users\tyboy\.claude\projects\d--Enemy-Nations-src\memory\feedback_mfc_removal_strategy.md))
- **The goal is "running without MFC", not "removing all CString refs".** Don't optimize the wrong metric.
- **SDL2 dialogs match visuals + functionality of the MFC originals.** Use the .RC templates as visual specs.
- **Don't rush.** Take time to understand each removal's impact before excluding.

---

## Quick-start checklist for new sessions

1. Read this guide.
2. `git -C 'D:\Enemy Nations\src' log --oneline -20` — see what was done last.
3. Check `C:\Users\tyboy\.claude\projects\d--Enemy-Nations-src\memory\phase5a_cstring_progress.md` for Phase 5a state.
4. Run a sanity build: should produce `enations.exe` clean.
5. Pick a phase. Don't bounce between phases mid-session.
6. Commit per logical unit. One CString cluster = one commit. One dialog removal = one commit.
7. After ~20 commits or mid-major-refactor, update `phase5a_cstring_progress.md`.

---

## Honest caveats

This guide reflects the state of the work at session 2026-05-08. The codebase has ~167 commits worth of incremental work since 2026-04. Some specifics will drift:

- File line numbers cited above are approximate. `grep` for the symbol if line numbers don't match.
- "Dead at runtime" claims are based on tracing call sites; if you find a path that actually exercises chat-cluster code, the assumption was wrong.
- Save-file compatibility hasn't been re-tested since Phase 5a started touching CString members. Phase 5c work should validate against pre-migration saves first.
- The plan timeline ("17-26 weeks") was an estimate. Actual cadence has been slower because test feedback requires running the game; some bugs only surface at gameplay time.
