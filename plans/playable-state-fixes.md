# Playable-state fixes — gate-on visual/input bugs

Scope: getting Enemy Nations back to a fully playable state now that
`ENATIONS_USE_STUB_WND` + `ENATIONS_USE_STUB_APP` are permanent (MFC linked: **NO**).

The Phase 1g milestone (no MFC in import table) is achieved. The remaining
gate-on bugs are artifacts of the MFC-stub layer no longer being a full MFC
runtime — code that depended on MFC-only behavior (PreCreate, message maps,
CBT hook, MFC's WM_SETCURSOR cycle) now fails subtly.

This doc captures what we learned, what we fixed today, and what's left.

## The shape of the problem

Every gate-on window has two parallel surfaces:

```
┌────────────────────────────────────────────┐
│ CWndArea (MFC stub) — HWND, message pump   │  ← input/state, layered alpha=0
│ ┌────────────────────────────────────────┐ │
│ │ SDL2Panel (detached SDL_app window)    │ │  ← actual rendering
│ │ - own SDL_Window                       │ │
│ │ - own event callback                   │ │
│ └────────────────────────────────────────┘ │
└────────────────────────────────────────────┘
```

The MFC stub HWND drives game logic (message maps, OnCreate, OnCommand,
SetMouseState). The SDL panel renders the actual pixels and owns the OS-level
cursor. Bugs appear at the seam between them:

- the stub's `Create()` doesn't run MFC's `PreCreate()` flow, so layout
  members like `m_iYmin` stay uninitialized
- `BEGIN_MESSAGE_MAP` / `ON_BN_CLICKED` expand to compile-time no-ops, so
  button commands aren't dispatched
- the layered MFC stub catches mouse events instead of the SDL window
  underneath (unless `WS_EX_TRANSPARENT` is set)
- SDL owns the cursor on its own windows; the game's Win32 `::SetCursor(...)`
  calls get overridden by SDL's `WM_SETCURSOR` handler

## Fixed in this session (2026-05-23)

### 1. `click.ps1` / `keys.ps1` were sending input to the wrong HWND

The scripts picked the largest visible window. The MFC stub
`EnemyNationsMainWindow` and the SDL2 `SDL_app` window are both
2560×1440, so picking order was unstable. Messages sent to the MFC stub
never reach SDL.

**Fix**: prefer `Class == 'SDL_app'` in the picker. Now consistently
hits the SDL window where SDL translates Win32 messages into SDL events.

### 2. Area-map toolbar was missing (visual bug, regression-prone)

The area's toolbar panel was being `SetSize()`-d to garbage height
(`-1163005939` observed in diagnostics) because `m_WndStatic.m_iYmin`
was uninitialized memory.

**Root cause**: `CWndAreaStatic::PreCreate()` sets `m_iYmin = rect.Height()`,
but `CWndStub::Create()` calls `::CreateWindowEx` directly — it never calls
`PreCreateWindow`. Real MFC `CWnd::Create()` would. So the bar's layout
state was never initialized.

**Fix** (two-part):
- inline-default `m_iXmin/m_iYmin/m_iStatusStrt/...` in `CWndAreaStatic`
- explicit `m_WndStatic.PreCreate()` call before `m_WndStatic.Create()` in
  `CWndArea::OnCreate` at [area.cpp:2087](enations_latest/src/area.cpp#L2087)

### 3. Bottom toolbar (`CWndBar`) buttons didn't respond

The MFC stub for the bottom bar was `WS_EX_LAYERED` + `alpha=1` (invisible)
but still captured mouse events because it lacked `WS_EX_TRANSPARENT`.
Clicks landed on the MFC stub, never reaching the SDL2Toolbar underneath.

**Fix**: add `WS_EX_TRANSPARENT` to `CWndBar` at
[toolbar.cpp:299](enations_latest/src/toolbar.cpp#L299).

### 4. Area-map toolbar buttons pressed but did nothing

Buttons rendered, animated on press, posted `WM_COMMAND` — but the command
was never dispatched. With stub mode, `BEGIN_MESSAGE_MAP` /
`ON_BN_CLICKED(IDC_AREA_ZOOM_IN, ZoomIn)` etc. all expand to compile-time
no-ops. The only handler is `CWndArea::OnCommand` which only handled `IDA_SAVE`.

**Fix**: extended `CWndArea::OnCommand` at
[area.cpp:5506](enations_latest/src/area.cpp#L5506) to manually dispatch
all toolbar button + accelerator IDs (`IDC_AREA_ZOOM_IN → ZoomIn()` etc.).

### 5. `WS_EX_TRANSPARENT` for `CWndAreaStatic` and `CWndArea`

Same root cause as #3 — the area's MFC stubs were intercepting mouse
events. Added `WS_EX_TRANSPARENT` at
[area.cpp:675](enations_latest/src/area.cpp#L675) and
[area.cpp:2283](enations_latest/src/area.cpp#L2283).

### 6. SDL window subclass extended to all SDL windows (not just main)

The `SdlSubclassWndProc` (intercepts `WM_SETCURSOR` + `WM_MOUSEACTIVATE`)
was only installed on the main SDL window. Detached SDL panel windows
(area map, world map) had SDL's default wndproc, so their cursors weren't
under our control.

**Fix**: in `GameWindow::CreateSDLWindow`, after `SDL_CreateWindow` returns,
look up the HWND via `SDL_GetWindowWMInfo`, store the original wndproc in
a window property (`EN_origWndProc`), and install `SdlSubclassWndProc`.
The subclass routes the right original wndproc per HWND via the property.

## Still broken

### Area-map cursor invisible during normal navigation

**Symptom**: building-placement / rocket-placement cursor sprites work fine
(they're drawn on the canvas), but the system cursor is invisible during
normal navigation, selection, and unit commands. Clicking briefly shows
the system arrow, then it disappears on the next mouse move.

**Root cause hypothesis**:
- The game uses Win32 `::SetCursor(HCURSOR)` with MFC-loaded cursor handles
  (`m_hCurReg`, `m_hCurMove`, `m_hCurSelect`, ...) inside `CWndArea::SetMouseState`
- SDL owns the cursor on its detached SDL window. `WM_SETCURSOR` on the SDL
  window goes through our subclass, which forces `::SetCursor(IDC_ARROW)`
  and returns TRUE.
- But `SDL_MOUSEMOTION` callback then runs the game's `SetMouseState`, which
  calls `::SetCursor(m_hCurReg)` or `::SetCursor(NULL)` — overriding whatever
  the subclass just set.
- Diagnostics showed `WM_SETCURSOR` fires on the main SDL window but does
  NOT fire for subsequent motion on the area's detached window. So once a
  game cursor (or NULL) is set, it sticks until next click.

**Path forward** (per user direction — migrate to SDL2 native, don't patch
the MFC stub layer):
- Convert the area's HCURSOR resources to `SDL_Cursor*` once at load time
  (use `SDL_CreateColorCursor` from a Win32 cursor bitmap, or
  `SDL_CreateSystemCursor` for the standard ones)
- Replace `::SetCursor(hCur)` inside `CWndArea::SetMouseState` with a call
  through a small abstraction layer that calls `SDL_SetCursor(...)` when
  the area is rendered through a detached SDL window
- Drop the WM_SETCURSOR subclass hack; let SDL manage its windows normally

This is a chunk of work but it permanently fixes the seam and aligns with
the broader MFC → SDL2 migration.

## Pending after playable-state

- **Shutdown crash** at `myThreadClose` ([threads.cpp:94](enations_latest/src/threads.cpp#L94)) on
  `ExitInstance`. Minor; doesn't affect gameplay.
- **Debug build broken** — `BASED_CODE` macro undefined after dropping
  `afx*.h`. One-line fix in `mfc_compat.h` (`#define BASED_CODE`).
- **Phase 6 — Linux/macOS port**. Win32 APIs (`HWND`, `::CreateWindowEx`,
  `Wow64SuspendThread`, `::SetCursor`, ...) still need abstracting. Now
  unblocked since MFC is gone.

## Where the broader plan lives

- **`MFC_TO_SDL_PORT_GUIDE.md`** at the repo root is the canonical overview
  (580+ lines: 6-phase plan, file landmarks, gotchas, patterns, user prefs).
  The phase status table is slightly stale post-Phase-1g; refer to
  `./mfc-status.ps1` for current import counts.
- **`CLAUDE.md`** has the active session-level guidance.
- **`memory/MEMORY.md`** indexes ~40 point-in-time memory snapshots (treat
  as starting points, not authoritative state).
- **No active separate plan file** — `plans/hidden-waddling-petal.md`
  referenced in memory doesn't exist on disk. This file is the current
  working plan for the playable-state phase.
