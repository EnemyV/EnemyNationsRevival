# Enemy Nations — Claude Code project guide

Auto-loaded into every Claude conversation. Keep it short, keep it current.

## What this project is

*Enemy Nations* is a 1996 RTS by **Windward Studios**. The original was a 16-bit-era Windows game built on **MFC + DirectDraw/DirectSound + VFW (Indeo .avi)**. We've been porting it to **SDL2** so it can eventually run on Linux and macOS.

Where we are now:
- **SDL2 rendering, audio, video, and dialog toolkit are in place** and the game runs.
- **MFC link dependency is stripped in Release** — `./mfc-status.ps1` reports `mfc_linked: false`, `mfc_imports: 0`. `CMAKE_MFC_FLAG` / `_AFXDLL` are commented out.
- **Gates collapsed (2026-05-24)**: `ENATIONS_USE_STUB_WND` / `ENATIONS_USE_STUB_APP` are gone — the non-gate (MFC) branches were removed from source, since they had been dead since Phase 1g (2026-05-21). The codebase is now single-pathed through `CWndStub` / `CWinAppStub`.
- **Still finishing**: runtime polish and source cleanup around remaining MFC-shaped compatibility types in `mfc_compat.h`.
- **Not yet started**: actual Linux/macOS build (Phase 6) — needs Win32 APIs abstracted.

So: **was MFC, is becoming SDL2-only, will be cross-platform.** The active goal is finishing the MFC removal; cross-platform is the longer-term payoff.

## The goal

> Get *Enemy Nations* building and running in **SDL2 with no MFC**, then port to Linux/macOS.

"No MFC" specifically = no `mfc*.dll` in the import table of `enations.exe`. Verify with `./mfc-status.ps1`. Release currently satisfies this; keep checking after runtime fixes.

## Building (read this before running anything)

**Always use `./build.ps1`**. Never invoke `msbuild` directly — it's blocked by a PreToolUse hook because raw MSBuild output is 5000+ lines and burns context. The wrapper returns the first N parsed errors with source context.

```powershell
./build.ps1                       # Debug Win32, first 10 errors
./build.ps1 -Release              # Release Win32
./build.ps1 -File wndbase.cpp     # Single-file compile (much faster — use during cascades)
./build.ps1 -First 5              # Cap errors shown
./build.ps1 -Json                 # JSON output (for parsing)
./build.ps1 -Quiet                # One-line summary
```

If the hook blocks you and you genuinely need raw output (e.g., link errors that the wrapper drops), ask the user to disable the hook temporarily — don't try to bypass it.

## Status check

```powershell
./mfc-status.ps1                  # MFC import count, CString/CFile/afx_msg counts
./mfc-status.ps1 -Json
```

Run this after every commit. The finish line is `mfc linked: NO` in the binary section.

## Anti-patterns observed in prior sessions

1. **Claiming progress without building.** Memory entries decay; the source on disk is authoritative. Before answering "how far along are we?" or "is X done?", run `./build.ps1 -Quiet` and `./mfc-status.ps1`.
2. **Anchoring on memory over reality.** [memory/MEMORY.md](file:///C:/Users/tyboy/.claude/projects/d--Enemy-Nations-src/memory/MEMORY.md) is a useful index but entries are snapshots in time. If memory says "gate is OFF" but `git diff` shows the gate is ON, trust the file.
3. **Re-greppping the same things.** `./mfc-status.ps1` answers most "where are we" questions in one call. Use it instead of running 5 separate Select-String commands.
4. **Reaching for msbuild directly.** The training default. The hook will block you; use `./build.ps1`.

## Core strategy: exclude, don't delete

User rule (2026-05-03): **"the goal is to get it running without mfc. we dont need to delete, just not include for now."**

When removing an MFC-dependent class:
1. Drop the `.cpp` from `enations_latest/src/CMakeLists.txt` — preferred when the whole file is dead at runtime.
2. Wrap dead bodies in `#if 0 // MFC_LEGACY_<NAME>` when the file has mixed live + dead code.
3. Stub headers (see [ipccomm.h](windward/wind22/include/ipccomm.h) for the canonical template).
4. **Never delete source files** unless they were already commented-out before the MFC pass started.

Why: the `.RC` dialog templates and source files capture original layouts and behavior; they're the reference for SDL2 reimplementation.

## Current phase

**Post-gate-collapse cleanup**, in progress. The MFC removal is complete:
- `CWndBase` inherits from [CWndStub](windward/wind22/include/wndstub.h) unconditionally.
- `CConquerApp` inherits from `CWinAppStub`; entry point is the hand-written [WinMain.cpp](enations_latest/src/WinMain.cpp).
- `CMAKE_MFC_FLAG` and `_AFXDLL` are commented out in both CMake targets.
- `ENATIONS_USE_STUB_WND` / `ENATIONS_USE_STUB_APP` gates and their `#else` branches were removed 2026-05-24 (waves 1-5).

Next: continue dialog porting, then start abstracting the remaining Win32 APIs in `mfc_compat.h` toward a cross-platform layer.

Full plan: `C:\Users\tyboy\.claude\plans\hidden-waddling-petal.md`.
Detailed porting guide: [MFC_TO_SDL_PORT_GUIDE.md](MFC_TO_SDL_PORT_GUIDE.md). **Read this before doing significant porting work.**

## User preferences (durable)

- **No "Co-Authored-By" trailers in commits.** Plain commit messages.
- **SDL windows must be movable/resizable** like their MFC counterparts.
- **Don't delete MFC source files** — exclude from build instead. See [feedback_mfc_removal_strategy.md](file:///C:/Users/tyboy/.claude/projects/d--Enemy-Nations-src/memory/feedback_mfc_removal_strategy.md).
- **Visual fidelity for ported dialogs**: read the `.RC` template and match positions/colors/fonts.

## File landmarks

- `enations_latest/src/` — **live game code (edit this)**
- `enations_latest/src/SDL2*.cpp/h` — SDL2 toolkit + game UI replacements
- `enations/src/` — old snapshot, do NOT edit (historical reference only)
- `windward/wind22/` — support library (CDIB, CMmio, codecs, BTree). Keep, strip MFC.
- `tools/sdl2/` — SDL2-2.30.12 + ttf + mixer + glew
- `cmakeBuild/enations_latest/src/{Release,Debug}/enations.exe` — build outputs
- `dbgcatch.ps1` — custom P/Invoke debugger for runtime crashes (catches OutputDebugString, exceptions, walks stacks)
- `plans/` — design docs, plans, audits

## Runtime testing

The game runs from `d:\Enemy Nations\` (the run dir has the DLLs). The smoke-test milestone is **"reaches world generation"** — if the game gets that far without crashing, you didn't regress anything obvious.

When something crashes during init, `dbgcatch.ps1` is the right tool — it catches `OutputDebugString` and exception events, walks the stack via dbghelp.

## UI harness (screenshot + click the live game)

Drive the running game yourself instead of asking the user what they see. **Just act**: screenshot → Read the PNG → click → screenshot to confirm. Each call is ~1s; don't narrate or ask between steps. ([feedback_harness_act_fast](file:///C:/Users/tyboy/.claude/projects/d--Enemy-Nations-src/memory/feedback_harness_act_fast.md))

```powershell
& '.\screenshot.ps1' -ListWindows          # list the game's SDL windows + roles
& '.\screenshot.ps1' -Out d:\tmp\s.png      # 50% shot of the auto-target window
& '.\click.ps1' -X 1850 -Y 145              # click client-px coords (read from the PNG)
& '.\keys.ps1'  -Key Enter                  # named key; -Text "foo" types; -ListKeys
```

**The game is multi-window** — target by role, not "the big window." In-game the main "Game View" window is only toolbar/chrome; the **map, radar, detached panels, and dialogs are each their own child `SDL_app` window** (SDL routes input by windowID, and `PrintWindow` on the parent can't capture a child's GL surface — you get blank blue). Pick the window with `-Window`:

```powershell
& '.\screenshot.ps1' -Window map -Full      # native-res Area Map (the interactive map)
& '.\click.ps1' -Window map -X 800 -Y 600   # click lands on the map child
& '.\screenshot.ps1' -Composite             # main + all children stitched = true view
& '.\keys.ps1' -Window "Load Game" -Text x   # any title substring also works
```

- Roles: `map`/`area`, `radar`, `vehicles`, `buildings`, `research`, `main`/`menu`, `pick`/`player`, or any title substring. **Omit `-Window`** → auto (Area Map in-game, else the sole menu/dialog window).
- Shared resolver is [harness-common.ps1](harness-common.ps1); HWNDs change on reload so it re-resolves by class+title each call. `dbgshot.ps1` = token-cheap wrapper (33% + JPEG).
- **Coords are client px of the target window**, at native res. Read at `-Full` to pick exact coords (small dialogs have ~19px rows — unforgiving). Screenshots are **50% by default** to save vision-tokens (`-Scale N`, `-Full`; each shot prints a token estimate).
- `PostMessage`-based: **no focus needed**, runs in background. **No modifier combos** (Ctrl/Shift+key). **Escape on the main menu quits the game.** `-Screen` is the fallback if PrintWindow returns black.

## Don't do

- Don't run `msbuild` directly (hook blocks it)
- Don't `git add -A` or `git commit *` blindly — git status has ~50 untracked files (Indeo experiments, video transcodes, test binaries). Stage specific files.
- Don't `git push --force` to anything
- Don't amend committed commits without asking
- Don't create new "FINAL_PLAN" or "SESSION_SUMMARY" markdown files — there are already 40+ memory entries. If a status note is needed, update an existing memory file or [memory/MEMORY.md](file:///C:/Users/tyboy/.claude/projects/d--Enemy-Nations-src/memory/MEMORY.md).
- Don't delete the legacy `.dsp` / `.mak` / `.bak` MSVC 6 project files — they're dead but harmless

## Memory hygiene

Memory entries are point-in-time observations. Before citing one as current truth:
- Check the memory's date vs today
- If it makes a specific claim (file exists, function works), verify against the codebase or run a build
- Stale memory > no memory, but only as a starting point

The MEMORY.md index has ~40 entries. Several are session summaries that have been superseded. **Don't add new session summaries by default** — only add memory when there's a durable lesson, not a status snapshot.
