# Enemy Nations — Claude Code project guide

Auto-loaded into every Claude conversation. Keep it short, keep it current.

## ⛔ FIX WORKFLOW — the operator's law (dictated 2026-07-11; READ BEFORE ANY FIX)

Repeated 10+ times across June–July 2026; violations burned a week of churn.
When you find an issue, you are NOT to do the first fix you can think of:

1. **Analyze the issue. Determine the root cause.** Confirm it with evidence
   (logs, probes, live memory reads) — *"always try to confirm a root cause if
   you can before you jump into a fix."*
2. **Can't find the root cause?** Log the bug (discussion repo,
   `docs/investigations/`) and move on — we deal with it later. A band-aid is
   NEVER the fallback.
3. **The fix must be maintainable, professional, minimal, and at the root** —
   at the line where the code detects/creates the broken state. Sweepers,
   watchdogs, rescues, and anything that interrupts a unit's task from outside
   are band-aids BY DEFINITION: *"sweepers are catch-all bandaids... if we just
   keep bandaiding it, it'll completely fall apart at some point."* A real root
   fix should RETIRE an existing band-aid, never add one.
4. **Confirm the fix: run the game and soak ~30 minutes** (not multi-hour
   unless asked). The original symptom must be measurably gone.
5. **Then check for regressions in the same soak** — trucks moving-%,
   deliveries, new buildings, crane welds, war activity. *"We had one bug with
   trucks... you bandaided that and that broke cranes... and we basically
   churned like that for multiple days."*
6. **Only after 4 AND 5 pass do you move to the next task.** (The next task's
   soak may overlap the previous task's verification.)

Standing corrections from the same dictations:
- *"Do not add another Band Aid. We have to fix root causes, otherwise this is
  never gonna end."*
- **(2026-07-12, absolute) Never suggest or implement band-aid solutions.** If the
  operator explicitly orders one, PUSH BACK first ("that's a band-aid, we're not
  gonna do that") and only proceed if they insist. Sweepers, rescue layers, and
  parallel re-implementations of broken mechanisms are band-aids by definition —
  fix the mechanism that's broken instead. Corollary: never implement ahead of
  approval; propose in plain words (what/size/risk), get the go, then change
  exactly that.
- *"I'm not telling you to do anything that I haven't explicitly told you to
  do."* — no unrequested actions; "answer only" / "only respond" means ZERO
  tool calls in the reply.
- *"It worked in an older version. We made a change and broke it."* — when a
  symptom appears, suspect OUR recent changes first; find the build boundary
  where the metric regressed before theorizing.
- One data point is not "fixed": sustained data + counterexamples, and
  periodically re-verify old fixes for returning bugs.

## 📦 RELEASE RULES (added 2026-07-20 after the 3.00.007 incident)

**Before packaging ANY artifact, prove the branch contains every other lane.** Not the
version number, not the board, not memory — the commit graph:

```powershell
git fetch revival --prune
# every active branch must report 0
foreach ($b in git branch -r --list 'revival/release3_00_*' 'revival/master' 'revival/mac-build' 'revival/linux-build') {
  "$b : $(git rev-list --count HEAD..$($b.Trim()))"
}
```

Non-zero on any lane = **DO NOT CUT**. Merge or cherry-pick first, then re-check.
(`git cherry` marks conflict-resolved picks as `+` even when the content IS present —
when that happens, verify per-symbol, not by patch-id.)

**Then verify the BUILT BINARY, not the source.** Scan the shipped exe for a string
unique to each lane's work (e.g. `[TRAP-REMOVED]` = 006 lineage, `SCROLLBAR` = the create
fix, `Player %1 has died` = the POSIX STRINGTABLE fix). Source correctness does not prove
the artifact was built from that source.

**Version numbers do NOT imply recency.** They're hand-typed. 3.00.007 looked newer than
3.00.006 while containing 113 fewer commits. Never reason "higher number = newer code."

**Version fields — three DIFFERENT numbers, don't conflate them** (`enations_latest/src/version.h`):
- `VER_STRING` / `RES_VER_STRING` — the display build number ("3.00.010"). Bump freely;
  **no effect on saves**. Both must be edited together; it's the only file with the string.
  The packaged `README.txt` is generated from this value (see **Release packaging** below),
  so bumping it here is the only place the shipped version header comes from.
- `VER_RELEASE` — the **save-format** counter (stored as `m_dwVer`). Bump ONLY when adding
  serialized fields, and gate every new read on `theGame.m_dwVer >= N`. Currently 7.
- `VER_MAJOR` / `VER_MINOR` — the only fields the save loader REJECTS on
  (`player.cpp:3465`). Bumping either invalidates every existing save.

**If a bad artifact ships:** convert the GitHub release to a **draft** (hides it, assets
404 publicly, nothing is destroyed) rather than deleting it — other agents' good assets
may be attached to it.

### Release packaging — the readme is GENERATED, do not hand-write one

Every zip ships `README.txt`, produced by CMake from `packaging/README.txt.in` with the
version parsed out of `version.h`. **Same filename on all three platforms.** Just zip what
the build staged next to the binary; the file is already there and already correct.

This replaced three hand-maintained readmes under three different names
(`README-FIRST.txt` on Windows, `README.txt` on Linux, none at all on macOS), which drifted
independently — the shipped **3.00.009 Linux zip** carried 3.00.000-era text stamped
*"release 3.00.008"* that still told users multiplayer was unavailable on Linux, the
headline feature of that very release. Nobody noticed because no two platforms shared a file.

- Platform wording (requirements, contents) lives in the `if (WIN32) / elseif (APPLE) / else`
  block in `enations_latest/src/CMakeLists.txt`. Edit it there, never in a packaged copy.
- An unparseable `VER_STRING` is a **configure-time FATAL_ERROR**, not a wrong header in a
  shipped zip.
- If you find yourself editing a readme inside a staging directory, stop — your change will
  be silently overwritten by the next build and will not reach the other platforms.

**⚠️ Re-run cmake configure after ANY merge/rebase, before packaging.** The staging rules use
`file(GLOB ...)` (cursors, `res/*.ttf`, the licence), and a glob is evaluated at **configure**
time only. Pull a commit that adds a staged file and build without reconfiguring, and the
glob still holds its old list — the new file never reaches the output and the zip ships
without it, with a green build and no warning. Hit on 2026-07-27: after rebasing onto
linux2's `9777ee27` the output `res/` still had 50 files instead of 52, i.e. the bundled
DejaVuSans no-text safety net was missing from the Windows staging. **Verify by counting the
staged files, not by reading CMakeLists** — the source looked correct the whole time.

## ⚠️ Cross-platform integration + multi-agent coordination (READ FIRST)

We are merging **three platform codebases into one tree** for release **3.00.000**:
**Windows (MSVC x64)**, **Linux (gcc x64)**, **macOS (clang/ARM64)**. Multiple agents
work in parallel — at least one per platform, sometimes more — each on its own machine,
all sharing the integration branch.

- **Release lane: `release3_00_011`** (as of 2026-07-31). This is the single source of truth.
  Pull it before you work; build before you push; keep all three platforms compiling.
  **`release3_00_000` / `_005` / `_006` / `_007` / `_008` / `_009` / `_010` are DEAD** — do
  not commit to them. 3.00.009 shipped from `release3_00_009`. **3.00.010 was never
  published** — it existed only as a draft, and was re-cut as 011 once the GH#8 data-file
  diagnostics landed, so there is exactly one build per version number.
- **⛔ NEVER cut a release without running the containment check** (see the release rules
  below). On 2026-07-20 the 3.00.007 Windows asset shipped from a branch **113 commits
  behind** the real development line, missing a month of verified crash fixes.
- **You are one of several agents.** Coordinate — don't silently change shared files.
- **Creating a branch, or publishing a GitHub release, REQUIRES a board post.** The 007
  incident happened because a second release branch was created — and a release shipped
  from it — with no board announcement, so the other agents kept working on the old lane.
- **Live cross-agent message board + build-status table: [AGENT_SYNC.md](AGENT_SYNC.md).**
  Read it at the start of every loop; post there when you change shared code, get blocked,
  or finish a task. Message format is defined at the top of that file — follow it exactly.
- **Plans, design & investigation docs now live in the private discussion repo `EnemyV/EnemyNationsDiscussion` under `docs/`** (e.g. `docs/plans/cross-platform-integration.md`, `docs/plans/multiplayer-cross-platform.md`, `docs/design/`, `docs/investigations/`). **Put new plans/ideas/design notes there, not in this repo.**

**Platform-detection convention (use the existing macros — do NOT invent new ones):**
`_WIN32` = Windows/MSVC, `__APPLE__` = macOS, `__linux__` = Linux. The Win/POSIX split is
`#ifdef _WIN32 … #else /* POSIX */ … #endif`; the POSIX branch pulls the Win32 shim
(`windward/wind22/include/win32_compat.h`). Use `__APPLE__`/`__linux__` only for OS-specific
divergence *inside* the POSIX branch. **Golden rule: a change that fixes your platform must
not break the other two** — prefer a portable expression over an `#ifdef`.

**Decision-making (peers, not hierarchy):** the Windows/lead agent coordinates and breaks
ties, but the three agents are **equally capable peers**. Disagree on the board with
reasoning, not deference. **If BOTH other platform agents disagree with a call, the lead
re-evaluates** — two-against-one means rethink, don't override. Default to consensus.
And **"green" on the build-status board means *verified running* (eyes-on or a real
smoke test), not merely *compiled*** — don't mark green on a heuristic.

**Avoid churn & code rot (serious risks).** Do **not** make speculative fixes for bugs you
cannot **reproduce** and **verify** — especially in fragile areas (render/terrain/fog cache,
serialization). **Repro first; if you can't repro it, park it, don't touch it.** Prefer
parking/reverting over piling changes on; keep every edit scoped and verifiable. A change you
can't confirm helped is churn at best and a regression at worst.

**Verify fixes twice over — static + dynamic, by more than one agent.** A fix is "done" only
when it's verified by **(1) static analysis** (read the diff, reason about correctness) **and
(2) actually running it** (the original failure is reproducibly gone) — and confirmed by the
**implementor *and* at least one other agent** (a different platform for cross-platform fixes).
Independent eyes catch false "green." **Idle time → improve your harness** (faster/leaner/more
reliable game-driving + capture); share harness improvements on the board only if genuinely
useful to others (signal, not noise).

## What this project is

*Enemy Nations* is a 1996 RTS by **Windward Studios**. The original was a 16-bit-era Windows game built on **MFC + DirectDraw/DirectSound + VFW (Indeo .avi)**. We've been porting it to **SDL2** so it can eventually run on Linux and macOS.

Where we are now:
- **SDL2 rendering, audio, video, and dialog toolkit are in place** and the game runs.
- **MFC link dependency is stripped in Release** — `./mfc-status.ps1` reports `mfc_linked: false`, `mfc_imports: 0`. `CMAKE_MFC_FLAG` / `_AFXDLL` are commented out.
- **Gates collapsed (2026-05-24)**: `ENATIONS_USE_STUB_WND` / `ENATIONS_USE_STUB_APP` are gone — the non-gate (MFC) branches were removed from source, since they had been dead since Phase 1g (2026-05-21). The codebase is now single-pathed through `CWndStub` / `CWinAppStub`.
- **Still finishing**: runtime polish and source cleanup around remaining MFC-shaped compatibility types in `mfc_compat.h`.
- **Not yet started**: actual Linux/macOS build (Phase 6) — needs Win32 APIs abstracted.

So: **was MFC, is becoming SDL2-only, will be cross-platform.** The active goal is finishing the MFC removal; cross-platform is the longer-term payoff.

> **macOS (Apple Silicon) — `mac-build` branch.** The game now builds and runs
> natively on macOS ARM64: single-player starts and the menu, toolbar/UI, area
> map (terrain + units), radar/minimap, build windows and zoom all render. Build
> and — crucially — **how to drive/screenshot the running game from a shell**
> (the Windows `*.ps1` harness does NOT work on macOS; use the in-process TCP
> harness + `harness/harness_client.py` + `harness/bmp2png.py`) are documented in
> **[MACOS_BUILD_AND_HARNESS.md](MACOS_BUILD_AND_HARNESS.md)** — read it before
> doing macOS build/run/test work.

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
Detailed porting guide: `docs/design/MFC_TO_SDL_PORT_GUIDE.md` in the discussion repo (`EnemyV/EnemyNationsDiscussion`). **Read this before doing significant porting work.**

## User preferences (durable)

- **No "Co-Authored-By" trailers in commits.** Plain commit messages.
- **SDL windows must be movable/resizable** like their MFC counterparts.
- **Don't delete MFC source files** — exclude from build instead. See [feedback_mfc_removal_strategy.md](file:///C:/Users/tyboy/.claude/projects/d--Enemy-Nations-src/memory/feedback_mfc_removal_strategy.md).
- **Visual fidelity for ported dialogs**: read the `.RC` template and match positions/colors/fonts.

## File landmarks

> ⚠️ **Which source tree? There are duplicates — always edit `enations_latest/src/`.**
> The same filenames (`terrain.cpp`, `area.cpp`, `world.cpp`, `unit.cpp`, …) exist in
> **multiple** trees. A `Glob`/search for one of these will return several hits — the
> wrong ones are dead snapshots. Only `enations_latest/src/` is compiled and live.
>
> - ✅ **`enations_latest/src/`** — the live, compiled game code. **Edit here.**
> - ❌ `enations/src/` — old snapshot, read-only reference. **Never edit.**
> - ❌ `enations/src/dave/` — even older per-dev snapshot. **Never edit.**
>
> Before editing any file with a generic name, confirm the path starts with
> `enations_latest/src/`. The RenderingAdapter, `SDL2*` windows, and all working
> SDL2/cursor/build-menu code live only in `enations_latest/src/`.

- `enations_latest/src/` — **live game code (edit this)**
- `enations_latest/src/SDL2*.cpp/h` — SDL2 toolkit + game UI replacements
- `enations/src/` — old snapshot, do NOT edit (historical reference only)
- `enations/src/dave/` — older per-dev snapshot, do NOT edit
- `windward/wind22/` — support library (CDIB, CMmio, codecs, BTree). Keep, strip MFC.
- `tools/sdl2/` — SDL2-2.30.12 + ttf + mixer + glew
- `cmakeBuild/enations_latest/src/{Release,Debug}/enations.exe` — build outputs
- `dbgcatch.ps1` — custom P/Invoke debugger for runtime crashes (catches OutputDebugString, exceptions, walks stacks)
- design docs / plans / audits / investigations → moved to the discussion repo `EnemyV/EnemyNationsDiscussion` under `docs/` (kept out of this repo to keep it lean)

## Runtime testing

The game runs from `d:\Enemy Nations\` (the run dir has the DLLs). The smoke-test milestone is **"reaches world generation"** — if the game gets that far without crashing, you didn't regress anything obvious.

When something crashes during init, `dbgcatch.ps1` is the right tool — it catches `OutputDebugString` and exception events, walks the stack via dbghelp.

## UI harness (screenshot + click the live game)

Drive the running game yourself instead of asking the user what they see. **Just act**: screenshot → Read the PNG → click → screenshot to confirm. Each call is ~1s; don't narrate or ask between steps. ([feedback_harness_act_fast](file:///C:/Users/tyboy/.claude/projects/d--Enemy-Nations-src/memory/feedback_harness_act_fast.md))

```powershell
& '.\screenshot.ps1' -ListWindows          # list the game's SDL windows + roles
& '.\screenshot.ps1' -Out d:\tmp\s.png      # 50% shot of the auto-target window
& '.\click.ps1' -X 1850 -Y 145              # click client-px coords (read from the PNG)
& '.\click.ps1' -Window map -X 800 -Y 600 -Right -Ctrl   # Ctrl+RMB (rotate a building being placed)
& '.\click.ps1' -Window map -X 1000 -Y 600 -ToX 600 -ToY 400 -Middle  # DRAG (here: MMB pan; -Right drag = line move, LMB drag = box select)
& '.\keys.ps1'  -Key Enter                  # named key; -Text "foo" types; -ListKeys
& '.\keys.ps1'  -Window map -Key "."         # in-game hotkey: OEM punctuation , . [ ] = - now work
& '.\keys.ps1'  -Window map -Key A -Ctrl     # modifier combos: -Ctrl/-Shift/-Alt
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

### One-shot load (`load-game.ps1`)

Get in-game without screenshotting the menu — fixed button coords, encodes the whole flow:

```powershell
& '.\load-game.ps1' -Save 5-3               # launch (if needed) -> Load -> pick save -> wait 5s -> click OK to confirm player
& '.\load-game.ps1' -Save 5-3 -PickDelaySec 8   # longer settle if the save loads slowly
& '.\load-game.ps1' -Save 5-3 -PickDelaySec 0   # stop at the player-select screen (no auto-confirm)
& '.\load-game.ps1' -NoLaunch               # game already at menu; don't relaunch
```

Flow: main menu **Load Single Player Game** (1850,145) → **Load Game** dialog → double-click the save row (substring match on `-Save`) → after `-PickDelaySec` seconds, confirm the auto-selected player on the **Pick Your Player** dialog via a **click-OK (235,470) → Enter → click-OK** chain, then prints `Player confirmed; in-game.` (or warns if the dialog is still up). Saves the list shot to `d:\tmp\savelist.png`. **Launches the x64 Debug exe but WITHOUT profiling env vars** — for `perf.log`/leak data, set `EN_PERF=1` (and `EN_PERF_ALLOC=64`) and launch the exe yourself first, then `./load-game.ps1 -NoLaunch -Save 5-3`.

> **Harness gotcha — keyboard vs. focus on SDL dialogs.** SDL delivers *mouse* events to the window a message targets (so `click.ps1` works on any child/dialog), but routes *keyboard* events to its keyboard-**focus** window. A modal dialog now grabs keyboard focus on open via `SDL_SetWindowInputFocus` ([SDL2UI.cpp DoModal](enations_latest/src/SDL2UI.cpp)), so Enter=OK / Esc=Cancel work by default — **but** that (like `SDL_RaiseWindow`) calls `SetForegroundWindow` on Windows, which the OS **blocks for a background app**. So a `keys.ps1 -Key Enter` can still be dropped when the game is driven in the background. **Guaranteed path: click the button** (mouse events are window-targeted, focus-independent). `load-game.ps1` confirms the player with a click-OK → Enter → click-OK chain for exactly this reason.

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
