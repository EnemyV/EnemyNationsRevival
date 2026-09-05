# Enemy Nations — Claude Code project guide

Auto-loaded into every Claude conversation. Keep it short, keep it current.

## What this is

*Enemy Nations*, a 1996 RTS by Windward Studios, ported from MFC + DirectDraw to **SDL2**,
now building and running natively on **Windows (MSVC x64), Linux (gcc x64), and macOS (clang ARM64)**
from one tree. MFC is fully stripped (`./mfc-status.ps1` must report `mfc_linked: false`).

- **Release lane: `release3_00_014`** on `EnemyV/EnemyNationsRevival`. Older `release3_00_*` lanes are dead.
- Multiple agents (one per platform) share the lane. Coordinate on **[AGENT_SYNC.md](AGENT_SYNC.md)**
  (live board in the discussion repo `EnemyV/EnemyNationsDiscussion`, branch matches the release):
  pull at loop start, post when you change shared code, get blocked, or finish.
- Plans/design/investigation docs live in the discussion repo under `docs/` — not here.

## Fix workflow (the law)

1. **Root-cause first.** Confirm the cause with evidence (logs, probes, live reads) before touching code.
2. Can't find the root cause? **Log it and move on.** A band-aid is never the fallback — no sweepers,
   watchdogs, or rescue layers. If explicitly ordered to band-aid, push back once first.
3. Fixes are **minimal, at the root**, and should retire old band-aids, not add new ones.
4. **Verify by running the game** (~30-min soak): symptom gone AND no regressions
   (trucks, deliveries, construction, cranes, war activity).
5. Verify twice over — **static + dynamic, implementor + one other agent** (other platform for
   cross-platform fixes). "Green" on the board means *verified running*, not merely compiled.
6. **No speculative fixes for unreproduced bugs.** Repro first; if you can't repro, park it.
7. When a symptom appears, suspect *our recent changes* first — find the build boundary where it regressed.

**Platform macros:** `_WIN32` / `__APPLE__` / `__linux__`; Win-vs-POSIX split is
`#ifdef _WIN32 … #else` (POSIX pulls `windward/wind22/include/win32_compat.h`).
A fix for your platform must not break the other two — prefer portable expressions over `#ifdef`.

## Release rules

- **Containment check before cutting anything:** every active `revival/*` branch must show
  `git rev-list --count HEAD..<branch>` = 0. Non-zero = do not cut. (3.00.007 shipped 113 commits behind.)
- **Verify the built binary, not the source** — scan the shipped exe for strings unique to each lane's work.
- **Version numbers are hand-typed; they do not imply recency.**
- `version.h` fields: `VER_STRING`/`RES_VER_STRING` = display number (bump freely, edit both);
  `VER_RELEASE` = save-format counter (bump only for new serialized fields, gate reads on
  `theGame.m_dwVer >= N`); `VER_MAJOR`/`VER_MINOR` = save-loader rejection — bumping invalidates all saves.
- `README.txt` in the zips is **generated** by CMake from `packaging/README.txt.in` — never hand-edit a
  staged copy; platform wording lives in `enations_latest/src/CMakeLists.txt`.
- **Re-run cmake configure after any merge, before packaging** — staging globs are configure-time;
  verify by *counting staged files*, not reading CMakeLists.
- Release PRs into `master`: **"Create a merge commit," never squash** (squash breaks lane history);
  after each release merge run `git merge -s ours revival/master` on the lane.
- Bad artifact shipped? Convert the release to **draft** (hides it non-destructively), don't delete.
- Creating a branch or publishing a release **requires a board post**. Publishing is the operator's call only.

## Building

**Always `./build.ps1`** — never raw `msbuild` (a hook blocks it; raw output burns context).

```powershell
./build.ps1                # Debug Win32
./build.ps1 -Release       # Release  (add -x64 for the canonical x64 build)
./build.ps1 -File x.cpp    # single-file compile — fast, but does NOT update the canonical exe
./build.ps1 -Json / -Quiet
```

Status: `./mfc-status.ps1`. Build outputs: `cmakeBuild-x64/enations_latest/src/{Release,Debug}/enations.exe`.
Before answering "is X done?" — build and check the tree; memory and boards are snapshots, source is truth.

## Runtime testing

Run dir = the folder with the DLLs next to the exe. Smoke milestone: reaches world generation.

- **`dbgcatch.ps1`** — catches OutputDebugString + exceptions, walks stacks. Use for crashes.
- **`cdb.exe`** (`C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\`) — attach to the LIVE game
  instead of probe+rebuild cycles. Rules: **ask before attaching**; every breakpoint command list ends
  in `gc`; pre-arm `sxe -c "k 6; gc" bpe`; detach with `qd` (never `q`, never kill cdb); read-only.
  Debug builds are `/O2` — locals often optimized out; read args at function entry, state through pointers.
- `OutputDebugString` is a trap for live reading (needs a DBWIN listener, blocks ~10s on a dead one) —
  probes you need to read live should write to a file. Probe gates: `enations_latest/src/enprobes.h`.
- Diagnostic files land in the game's *working directory*, not always the run dir — search, don't assume.

## UI harness (screenshot + drive the live game)

Act directly: screenshot → Read the PNG → click → screenshot to confirm. Don't narrate between steps.

```powershell
& '.\screenshot.ps1' -Window map -Full     # roles: map, radar, main, research, or any title substring
& '.\click.ps1' -Window map -X 800 -Y 600  # client-px coords; -Right/-Middle/-Ctrl/-Shift; -ToX/-ToY drags
& '.\keys.ps1' -Window map -Key A -Ctrl    # named keys, -Text types
& '.\load-game.ps1' -Save 5-3              # one-shot: menu -> load -> pick save -> confirm player
```

- The game is **multi-window**; target by role (`-Window`), coords are client-px of that window.
  Screenshots default to 50% scale to save tokens (`-Full` for native).
- Mouse events are window-targeted and work in the background; **keyboard needs focus** — when a
  keypress may drop, click the button instead. Escape on the main menu quits the game.
- One game instance at a time: check `tasklist | findstr /i enations` before launch and after kill.

## Don't

- Don't run `msbuild` directly.
- Don't `git add -A` / commit blindly — stage specific files (the tree has many untracked artifacts).
- Don't force-push; don't amend pushed commits without asking.
- Don't delete legacy MFC-era source or project files — exclude from build instead
  (`.RC` templates are the reference for SDL2 dialog work).
- Don't create new summary/plan markdown files here — update memory or the discussion repo.
- No "Co-Authored-By"/"Generated with" trailers in commits. Single-line commit messages, the gist only.

## File landmarks

> ⚠️ Duplicate trees exist. **Only `enations_latest/src/` is compiled and live — edit there.**
> `enations/src/` and `enations/src/dave/` are dead snapshots, read-only reference.

- `enations_latest/src/SDL2*.cpp/h` — SDL2 toolkit + game UI
- `windward/wind22/` — support library (CDIB, compat shims)
- `tools/sdl2/` — SDL2 + ttf + mixer + glew
- macOS build/run/harness: **[MACOS_BUILD_AND_HARNESS.md](MACOS_BUILD_AND_HARNESS.md)**
  (the `.ps1` harness is Windows-only; mac uses the in-process TCP harness)
