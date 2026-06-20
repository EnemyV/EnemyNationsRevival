# macOS (Apple Silicon) — build, run & the LLM driving harness

This is the macOS counterpart to the Windows notes in `CLAUDE.md`. It covers how
the game is built and run on macOS ARM64, and — importantly — how to **drive and
screenshot the running game** from a shell/agent, which is how the macOS port was
brought up and verified.

The macOS port lives on the `mac-build` branch (branched from `linux-build`).

---

## 1. Build

Toolchain (Homebrew + Apple clang):

```sh
brew install cmake sdl2 sdl2_ttf sdl2_mixer
cmake -S . -B build-mac -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build-mac --target enations -j8
# binary: build-mac/enations_latest/src/enations  (arm64 Mach-O)
```

SDL2 comes from Homebrew via pkg-config (the bundled Windows `tools/sdl2` is
MSVC-only). The build is the same single `enations` target as Linux.

## 2. Game data + run directory

The game needs the real **`ENations.dat`** (~516 MB) — it is NOT in the repo.
The download link is in `readme.adoc` (a Google Drive `.zip` containing the 1997
CD `.iso`; `ENATIONS.DAT` is on the ISO). Mount the ISO and copy it out.

Set up a run dir whose `data/` holds only the baked GPU terrain set (all the
`.rif` assets are read from `ENations.dat`; the loose `enations/data` snapshot is
an 8-bit-only legacy set that shadows the master, so do NOT use it as the patch
dir):

```sh
mkdir -p run-mac/data
ln -sfn "$PWD/enations_latest/data/terrain_gpu" run-mac/data/terrain_gpu
cp /Volumes/EnemyNations/ENATIONS.DAT run-mac/ENations.dat   # from the mounted ISO
```

**Trial gate (one-time):** the 1997 CD `ENATIONS.DAT` is the 30-day-trial
pressing, so the engine runs its install-date check (a Windows registry value the
installer would have written) and shows the intro movie (Indeo/VFW — unsupported
here). Seed the registry shim — which is just a flat file at
`~/.config/enations/registry.ini` — to emulate the installer and skip the intro:

```
HKLM\SOFTWARE\Microsoft\DOS Emulation\xCompatibility\CD-ROM=4:41
HKCU\Software\Second Chance\Second Chance\Game\NoIntro=4:1
```

(`4:41` = REG_DWORD 41, the "fresh install, start the timer on first launch"
sentinel; `NoIntro=1` skips the cutscene and lands on the main menu.)

## 3. Run

```sh
cd run-mac
EN_HARNESS=1 EN_HARNESS_PORT=7070 SDL_RENDER_DRIVER=opengl \
  ../build-mac/enations_latest/src/enations
```

### Environment knobs

| Var | Effect |
|-----|--------|
| `EN_HARNESS=1` | start the in-process TCP control server (screenshot/click/keys) |
| `EN_HARNESS_PORT=N` | harness port (default 7070) |
| `SDL_RENDER_DRIVER=opengl` | use the GL renderer. **Needed for harness read-back of the GPU area-map window** — SDL's Metal renderer reads back blank |
| `EN_FULLSCREEN=0` | stay windowed (default is borderless fullscreen-desktop) |
| `EN_SOFTWARE=1` | force the legacy software (window-surface) render path |
| `EN_SINGLEWIN=1` | suppress panel detaching — composite every panel into the main window (debug; the game's real model is multi-window) |
| `EN_DIAG=1` | gated stderr diagnostics (compositor panel dump, DIB/format info, cursor-dir resolution) |

---

## 4. The driving harness (this is the key part)

The Windows harness (`screenshot.ps1`, `click.ps1`, `keys.ps1`) does NOT work on
macOS. Instead the game compiles in a tiny TCP control server
(`harness/control_socket.cpp`, `en_harness.h`) enabled by `EN_HARNESS=1`.

Talk to it with **`harness/harness_client.py`**:

```sh
python3 harness/harness_client.py shotid 5 /tmp/area.bmp     # grab the area map
python3 harness/harness_client.py clickid 5 410 340          # click in the area map
python3 harness/harness_client.py dblclickid 5 410 340       # double-click (place rocket)
python3 harness/harness_client.py text Mac                   # type into a focused field
```

Commands: `shot <path>`, `shotid <winid> <path>`, `click x y [right]`,
`clickid <winid> x y [right]`, `dblclickid <winid> x y`, `move x y`,
`key <sdl_keycode>`, `text <string>`, `quit`.

### Screenshots are BMP — convert with the bundled tool

`sips` and Pillow both choke on SDL's 32-bit BMPs. Use **`harness/bmp2png.py`**:

```sh
python3 harness/bmp2png.py /tmp/area.bmp /tmp/area.png 820   # -> PNG, ≤820px wide
```

### Which window is which (single player)

The game is **multi-window** (intentional — it is multi-window / multi-monitor by
design). In a single-player game the SDL window ids are:

| id | window | capture path |
|----|--------|--------------|
| 1 | main (toolbar, resource bars, clock, wallpaper) | reliable — dumps the CPU back-buffer |
| 5 | **area map** (terrain, units, area button bar) | GPU window — needs `SDL_RENDER_DRIVER=opengl` |
| 6 | **radar / minimap** | reliable — dumps the panel's CPU back-surface |

Always target a specific window with `shotid <id>` — the auto-target (`shot`)
picks the focused-or-largest window, which is usually not the one you want.

> Capture reliability: the main window and detached non-terrain panels register
> their CPU back-surface with the harness, so `shotid` dumps the true composited
> image regardless of GPU read-back support. The area-map terrain is a real GPU
> mesh, so its window only reads back under the GL driver (Metal returns blank/
> garbage). `screencapture` from a shell only sees the desktop unless the host
> terminal app has been granted **Screen Recording** (and restarted) — the
> in-process harness avoids needing that.

### Start a single-player game (click flow)

Coordinates are client px of the targeted window at the default sizes:

```sh
H="python3 harness/harness_client.py"
$H click 314 405          # main menu: "Create Single Player Game"
$H click 185 499          # "Create Single Player Game" dialog (480x530): OK
$H clickid <race-win> 45 89    # "Pick Your Race" dialog (580x460): select Human
$H clickid <race-win> 300 55   # focus the Name field
$H text Mac                     # type a name
$H clickid <race-win> 235 428   # OK -> world generation runs
# after worldgen, PLACE THE ROCKET on the area map:
$H dblclickid 5 410 340         # 1st click only focuses the window; 2nd places
```

(The dialog windows are their own SDL windows; grab one with `shot` first to read
its current id, or just click the auto-targeted window which is the open dialog.)

---

## 4b. Full-screen & multi-monitor

The engine already renders at the desktop resolution (`en_SetScreenMetrics` from
`linux_main`), so the main game view opens as a **borderless fullscreen-desktop**
window at the display origin (set `EN_FULLSCREEN=0` to stay windowed). macOS
fullscreen **Spaces are disabled** (`SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES=0`) so
the detached map/radar/panel windows still overlay it — without that, a fullscreen
Space would hide every other window.

Multi-monitor is inherent to the design and needs no per-display mode switch:
- Detached panels are top-level `ALWAYS_ON_TOP` SDL windows, dragged by their
  title bar via **global** mouse coordinates with no monitor clamping, so they can
  be moved to any display.
- Initial placement is display-aware (`SDL_GetWindowDisplayIndex` +
  `SDL_GetDisplayUsableBounds`), so a panel is clamped to the monitor it spawns
  on, not the primary.

(Multi-monitor can't be exercised in a single-display VM — verify on a real
multi-display host.)

---

## 5. macOS-specific fixes (why these mattered)

Captured here so the next person doesn't re-derive them. All on `mac-build`:

- **8-bit DIB art rendered blank.** The UI art (menu, toolbar, buttons, dialogs)
  is 8-bit palettized; the Win32/WinG palette pipeline is stubbed out, so the
  color table was dropped on load and `CreateSurfaceFromDIB` ignored palettes.
  Fix: preserve each DIB's color table (`CDIB::Copy`) and apply it when building
  the SDL surface.
- **Radar/world map blank.** `GlobalMemoryStatus` returned 0 MB → engine forced
  8-bit color → CPU render targets became paletted. Fix: report ample memory so
  the 32-bit render path is used.
- **Cursors fell back to the system arrow.** `FindCursorDir` only searched
  cwd-relative paths; now it also walks exe-relative to find `enations_latest/src/res/*.cur`.
- **Spurious "demo expired".** LP64 bug — a 4-byte `REG_DWORD` was read into an
  8-byte `time_t`; fixed to read a 32-bit `DWORD`.
- **8-bit-only menu fallback (96×96 tile).** The loose `enations/data` snapshot
  lacks the 24-bit art; run from `ENations.dat` (see §2).
- Toolchain/shim: drop `librt` on Apple, guard AVX2 behind `__AVX2__` (ARM has no
  AVX2), `<malloc/malloc.h>`, `_NSGetExecutablePath` / `fcntl(F_GETPATH)` for the
  `/proc`-less shim, `GetDeviceCaps` reports 32bpp, Apple-Silicon CPU-speed via
  sysctl, clang `__intN` typedef + access-declaration fixes.

## Debugging a crash on macOS (runbook)

If the game crashes, **do not guess — get a stack first, then fix the function it
names.** Windows and Linux currently run, so a macOS-only crash is almost always
macOS-specific code or latent UB that only bites on clang/ARM64. Process:

### 1. Reproduce deterministically (from a terminal, NOT the harness first)
Run it by hand so you SEE stdout/stderr and the OS crash message:
```
cd run-mac
SDL_RENDER_DRIVER=opengl ../build-mac/enations_latest/src/enations
```
Drive the minimal flow and note the EXACT step it dies at:
launch → main menu → Create/Load single player → race/player pick → world-gen → in-game.
Run it 3× the same way: is it the **same step every time** (deterministic) or random
(intermittent → likely a memory/race bug)? Write down the last log line before it dies.

### 2. Get a backtrace with lldb (the key skill)
```
cd run-mac
lldb -- ../build-mac/enations_latest/src/enations
(lldb) settings set target.env-vars SDL_RENDER_DRIVER=opengl
(lldb) run
        # reproduce the crash; when lldb stops at the fault:
(lldb) bt                     # backtrace of the crashing thread
(lldb) thread backtrace all   # ALL threads — this game runs AI worker threads
(lldb) frame variable         # locals in the crashing frame
(lldb) quit
```
Copy the top ~20 frames of `bt` (the symbolized `file:line` ones) — that's the report.

### 3. Or read the OS crash report (if it died outside lldb)
macOS writes a full symbolized crash log per crash:
```
ls -t ~/Library/Logs/DiagnosticReports/ | head
# open the newest enations-*.ips
```
Read **Exception Type** (`EXC_BAD_ACCESS` = null/dangling deref; `SIGABRT` = assert/abort)
and the **"Thread N Crashed"** backtrace + faulting address.

### 4. For memory bugs (intermittent, EXC_BAD_ACCESS, heap corruption) → AddressSanitizer
Highest-signal tool — prints the exact bad access + allocation site, symbolized:
```
cmake -S . -B build-mac-asan -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
cmake --build build-mac-asan --target enations -j8
cd run-mac && SDL_RENDER_DRIVER=opengl ../build-mac-asan/enations_latest/src/enations
```

### 5. Report back on AGENT_SYNC.md
Post: the **crashing function `file:line`** (from the stack), the **exception type**,
whether it's **deterministic** + the repro step, and the top frames. THEN we (or you)
fix that specific spot. A one-line "it crashed" is not actionable — a stack is.

### Likely suspect areas (only after the stack points near them)
- macOS window/init (fullscreen-desktop creation), the GL renderer path.
- 8-bit DIB / palette paths (`CDIB::Copy`, `CreateSurfaceFromDIB`) — a null palette on
  some art path.
- Unaligned / struct-packing reads of `ENations.dat` records (clang/ARM64 is stricter).
- The POSIX shim (`win32_compat.cpp`) returning something a caller didn't expect.
