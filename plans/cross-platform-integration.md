# Cross-platform integration plan — Windows + Linux + macOS (release 3.00.000)

High-level plan for merging the three platform codebases into **one** source tree that
builds and runs single-player on all three. Keep this doc high-level; track live status
and per-task chatter in [AGENT_SYNC.md](../AGENT_SYNC.md).

## Goal

One tree in `enations_latest/src` (+ `windward/`, `harness/`) builds & runs single-player on:
- **Windows** — MSVC, x64 (`build.ps1 -x64`)
- **Linux** — gcc, x64 (CMake non-MSVC)
- **macOS** — clang, Apple Silicon/ARM64 (CMake; see `MACOS_BUILD_AND_HARNESS.md`)

Integration branch: **`release3_00_000`**, cut from `mac-build`. Target display version
**3.00.000** (`version.h`: `VER_MAJOR=3`, `VER_MINOR=0`, `VER_STRING="3.00.000"`;
`VER_RELEASE` stays the save-format counter — do **not** reset it, see the note in `version.h`).

## What each platform already has (so we don't re-derive it)

- **Windows (original):** full game + SDL2 migration; GPU terrain & sprite layers, now
  **per-renderer** so multiple area maps work (`SDL2Terrain`/`SDL2Sprites` RCtx). Build:
  MSVC, PCH = `stdafx.h`, real `<windows.h>`.
- **Linux:** Win32-on-POSIX shim — `windward/wind22/include/win32_compat.h` (+`.cpp`):
  LP64 fixed-width typedefs, calling-convention macros, global `min`/`max` function
  templates. Plus `linux_main.cpp` (`main`→`WinMain`), `win32_resources_linux.cpp`
  (STRINGTABLE), `vdmplay_stubs.cpp`, and the in-process TCP harness
  (`harness/control_socket.cpp`, `EN_HARNESS=1`). Build: CMake non-MSVC, force-include
  `stdafx.h`, no PCH.
- **macOS (ARM64):** built on the Linux POSIX layer — fullscreen-desktop main window for
  multi-window, exe-relative `res/` cursors, 8-bit DIB color-table fix, 32-bit render path
  via `GlobalMemoryStatus`, multi-window harness capture. Docs: `MACOS_BUILD_AND_HARNESS.md`.

## Principles

1. **One tree.** Platform differences via `_WIN32`/`__APPLE__`/`__linux__` + the compat
   shims (`win32_compat.h`, `mfc_compat.h`) — never divergent files or long-lived branches.
2. **Don't break the other platforms.** Any change must keep all three compiling. The
   `min`/`max` regression below is the cautionary tale.
3. **Portable first, `#ifdef` last.** Reach for an `#ifdef` only when the APIs truly differ.
4. **Keep the two include surfaces in sync** — MSVC's PCH (`stdafx.h`) and the non-MSVC
   `-include stdafx.h`.
5. **Build before you push; update the build-status table in AGENT_SYNC.md.**
6. **Shared files** (`win32_compat.{h,cpp}`, `stdafx.h`, `CMakeLists.txt`, `version.h`,
   `mfc_compat.h`): announce on the board before/after editing.

## Known integration blockers (live)

**P0 — `min`/`max` breaks the Windows build (147 errors).** The port routes `min`/`max`
through the `win32_compat.h` global templates and rewrote ~97 sites across 8 files to
`::max(...)`/`::min(...)`. `win32_compat.h` is excluded on MSVC, where `<windows.h>` defines
`max`/`min` as **macros**, so `::max(a,b)` → `::(…)` → syntax error. (Compiled fine on
clang/gcc; Windows silently regressed.) Fix options, all-platforms:
- **(a, recommended)** Define `NOMINMAX` for the MSVC build **and** expose the same global
  `min`/`max` templates on MSVC (via `stdafx.h`), mirroring the Linux design. Also fixes the
  legacy unqualified `max(a,b)` call sites that relied on the macros.
- (b) Replace `::max`/`::min` with `(std::max)`/`(std::min)` (parens defeat the macro) — but
  needs same-type args; mixed int/unsigned sites would need casts. (a) is cleaner.

## Plan of record (sequence)

1. **Snapshot builds (now).** Each agent builds `release3_00_000` for its platform and posts
   pass/fail + first errors to the build-status table. (Windows = known P0 fail.)
2. **Fix P0 min/max** (Windows agent, option (a)); verify Windows builds; Linux/Mac agents
   confirm still-green from the board.
3. **Green on all three.** Resolve remaining Win-vs-POSIX divergences one break at a time,
   coordinated on the board.
4. **Smoke test** each platform reaches world-gen + renders the area/world map.
5. **Tag/release 3.00.000** once all three are green and smoke-pass.

## Ownership (default; adjust on the board)

- **Windows agent(s):** MSVC build, the P0 min/max fix, game-logic regressions.
- **Linux agent:** gcc build, POSIX shim.
- **macOS agent:** clang/ARM64 build, macOS window/input/harness.
- **Shared files:** whoever touches them announces it on the board.
