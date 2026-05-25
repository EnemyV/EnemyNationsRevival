# Enemy Nations Codex Notes

This repo is an SDL2 port of the original MFC/Win32 Enemy Nations codebase. The live tree is `enations_latest/src`; `enations/src` is an old snapshot for reference only.

## Current State

- Release currently builds with no MFC link dependency. `mfc-status.ps1` reports `mfc_linked: false` and `mfc_imports: 0` as of 2026-05-23.
- `ENATIONS_USE_STUB_WND` and `ENATIONS_USE_STUB_APP` are enabled. `CMAKE_MFC_FLAG` and `_AFXDLL` are commented out.
- `WinMain.cpp` owns startup instead of MFC's `AfxWinMain`.
- Remaining `CString`, `CFile`, `CArchive`, `CWnd`, and `CDialog` names are largely compatibility-surface stubs from `windward/wind22/include/mfc_compat.h`, not proof of MFC linkage.
- Debug and Release both build as of 2026-05-23. Debug relies on compatibility shims for legacy `_DEBUG` MFC macros such as `BASED_CODE`, `ASSERT_VALID`, `AfxGetThreadState`, and `AfxAssertFailedLine`.

## Build And Status

PowerShell script execution may be disabled in this environment. Use a process-local bypass when needed:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Release -Quiet
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Release -First 10
powershell -ExecutionPolicy Bypass -File .\mfc-status.ps1 -Json
```

The normal project guidance says to use `build.ps1` rather than raw MSBuild because the wrapper keeps output manageable.

## Working Rules

- Do not delete legacy MFC source just to remove it. Prefer excluding `.cpp` files from CMake or wrapping dead class bodies in `#if 0 // MFC_LEGACY_*`.
- Do not edit `enations/src`; edit `enations_latest/src`.
- Preserve the SDL2 visual/functionality replacements against the old `.RC` templates where practical.
- Stage specific files only. This repo has many untracked generated/test/video files.
- Do not add `Co-Authored-By` trailers.

## Current WIP To Notice

- `enations_latest/src/SDL2FileBrowser.cpp` and `.h` are untracked but referenced from CMake. Commit them with the related save/load dialog work or a clean checkout will break.
- Recent uncommitted changes focus on SDL detached-panel cursor/input behavior, transparent click-through stub windows, manual command dispatch, and excluding dead chat/license MFC declarations.

## Runtime Testing

The game runs from `D:\Enemy Nations`, with the exe under `cmakeBuild/enations_latest/src/Release/enations.exe`. Useful local helpers:

```powershell
powershell -ExecutionPolicy Bypass -File .\screenshot.ps1 -Screen -Out d:\tmp\screen.png
powershell -ExecutionPolicy Bypass -File .\click.ps1 -X 1280 -Y 720
powershell -ExecutionPolicy Bypass -File .\keys.ps1 -Key Enter
```

The practical smoke milestone is reaching world generation without crashing.
Last checked 2026-05-23: Release launched, reached the game view/world generation, rendered the area map/world map/toolbars, accepted a posted click, and remained responsive.
