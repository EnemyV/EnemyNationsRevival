# Building Enemy Nations (Release 3)

Enemy Nations is being revived as a **64-bit, cross-platform (Windows / Linux / macOS),
SDL2-based** build of the 1996 Windward Studios RTS. No MFC, no DirectDraw. This guide
takes you from a clean checkout to a running game.

> **Just want to play?** Grab a prebuilt archive from the **Releases** page instead. Each
> is self-contained (game + data + libraries) for Windows x64, Linux x64, and macOS arm64.
> This guide is for building **from source**.

---

## TL;DR

| Platform | Configure | Build | Output binary |
|----------|-----------|-------|---------------|
| **Windows x64** (MSVC) | `cmake -S . -B cmakeBuild-x64 -A x64` | `./build.ps1 -Release -x64` *(or open `cmakeBuild-x64\Enations.sln` in VS 2022 and build **enations**, `Release`/`x64`)* | `cmakeBuild-x64\enations_latest\src\Release\enations.exe` |
| **Linux x64** (gcc) | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` | `cmake --build build --target enations -j$(nproc)` | `build/enations_latest/src/enations` |
| **macOS** (Apple Silicon, clang) | `cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Release` | `cmake --build build-mac --target enations -j8` | `build-mac/enations_latest/src/enations` |

All platforms then need the game-data file **`ENations.dat`** at runtime. See
[Game data & running](#3-game-data--running).

---

## 0. Prerequisites

**Windows (the primary developer build):**
- **Visual Studio 2022** with the **"Desktop development with C++"** workload (MSVC v143
  toolset + a Windows 10/11 SDK).
- **CMake >= 3.11** (the one bundled with VS works, or install standalone and put it on `PATH`).
- **Git**.
- SDL2 is **vendored** in `tools/sdl2/` (SDL2 2.30.12 + ttf + mixer + glew). **Nothing to install.**

**Linux (x64):**
- `gcc`/`g++` (e.g. `build-essential`), **CMake**, **Ninja** or Make, **pkg-config**.
- SDL2 dev packages: `libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev`.
- Debian/Ubuntu: `sudo apt install build-essential cmake ninja-build pkg-config libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev`

**macOS (Apple Silicon):**
- Xcode command-line tools (`xcode-select --install`) and **Homebrew**.
- `brew install cmake sdl2 sdl2_ttf sdl2_mixer`

---

## 1. Get the source

```sh
git clone https://github.com/EnemyV/EnemyNationsRevival.git
cd EnemyNationsRevival
```

The repo root holds the top-level `CMakeLists.txt`. The live game code is in
**`enations_latest/src/`** (other `enations/src*` trees are read-only historical snapshots,
don't build those).

---

## 2. Configure & build

CMake generates everything. **Always configure through CMake; do not open the legacy
`.dsp` / `.mak` / hand-written project files directly** (that's the usual cause of the
errors in section 4).

### Windows

```powershell
# From the repo root. Generates the VS solution into cmakeBuild-x64\ for the 64-bit target.
cmake -S . -B cmakeBuild-x64 -A x64

# Build it, either with the wrapper (Release, x64):
./build.ps1 -Release -x64
#   ...or open cmakeBuild-x64\Enations.sln in Visual Studio 2022,
#   set the configuration to Release / x64, and build the "enations" project.
```

- Result: `cmakeBuild-x64\enations_latest\src\Release\enations.exe`.
- `build.ps1` is a thin MSBuild wrapper that prints the first N parsed errors with source
  context (full flags: `./build.ps1 -h`, or see the header of the script). `-Release` selects
  the Release config; **`-x64`** selects the 64-bit target (the release target; without it
  the wrapper builds the legacy Win32 target in `cmakeBuild\`).
- 32-bit (legacy) instead: `cmake -S . -B cmakeBuild -A Win32` then `./build.ps1 -Release`.
- Available configurations: `Debug`, `Release`, `Profile`, `Sanitize`.

### Linux

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # omit -G Ninja to use Make
cmake --build build --target enations -j"$(nproc)"
```

- Result: `build/enations_latest/src/enations`.
- SDL2 is found via `pkg-config` (the vendored Windows `tools/sdl2` is MSVC-only). Only the
  game (`enations`) and its support library (`wind22`) build on Linux; the Windows-only
  asset/build tools (`rif_converter`, `makeriff`, `sprite`, `cdf`, `compit`, `vdmplay`,
  `iserve` GUI) are skipped automatically.

### macOS (Apple Silicon)

```sh
cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac --target enations -j8
```

- Result: `build-mac/enations_latest/src/enations` (arm64 Mach-O). SDL2 comes from Homebrew
  via `pkg-config`. Same single `enations` target as Linux.

---

## 3. Game data & running

The engine needs the original data archive **`ENations.dat`** (~516 MiB). It is **not in the
repo** (it ships on the 1997 game CD / the later freeware release; a download link is in
`readme.adoc`). The renderer also wants the baked GPU terrain set under `data/terrain_gpu/`.

**Easiest path:** download the matching platform archive from the **Releases** page (it already
contains `ENations.dat`, `data/`, and the runtime libraries) and **drop your freshly-built
binary in over the one in that folder.** Then run it from that directory.

Otherwise, assemble a run directory yourself:

```sh
# Linux/macOS example
mkdir -p run/data
ln -sfn "$PWD/enations_latest/data/terrain_gpu" run/data/terrain_gpu
cp /path/to/ENATIONS.DAT run/ENations.dat
cd run && ../build/enations_latest/src/enations      # (build-mac/... on macOS)
```

On Windows, run `enations.exe` from a directory that contains `ENations.dat`, `data/`, and the
SDL2 DLLs (the Releases archive layout).

> **Trial-data note (Linux/macOS).** The 1997 CD `ENATIONS.DAT` is the 30-day-trial pressing,
> so the engine checks for an installer-written registry value and tries to play the (VFW/Indeo)
> intro movie. On POSIX the "registry" is a flat file at `~/.config/enations/registry.ini`; seed
> it to emulate a fresh install and skip the intro:
> ```
> HKLM\SOFTWARE\Microsoft\DOS Emulation\xCompatibility\CD-ROM=4:41
> HKCU\Software\Second Chance\Second Chance\Game\NoIntro=4:1
> ```

---

## 4. Troubleshooting (incl. the common first-time errors)

**`wind40d.lib` (or `wind22.lib`) not found / unresolved externals.**
That's the support library (`windward/wind22/`). It is built **as part of the CMake build**, and
CMake wires it as a dependency of `enations`. If it's "missing," you almost certainly opened the
game project file directly instead of building the **CMake-generated** solution/target. Run the
`cmake -S . -B ...` configure step first, then build `enations` from the generated solution (or via
`build.ps1` / `cmake --build`).

**"needs `stdafx.h` at the top of each file" / "compiler options are incompatible."**
These come from trying to compile the **legacy MSVC-6-era project files** (`.dsp`/`.mak`) or a
stale hand-made `.vcxproj`. The current build does **not** use precompiled `stdafx.h` and sets all
flags/macros itself, so use the CMake-generated project, not the old ones. The dead `.dsp`/`.mak`
files are kept for history but are **not** the build system.

**MSBuild prints thousands of lines / a hook blocks raw `msbuild`.**
That's a developer-convenience hook in this repo. Use **`./build.ps1`** (the wrapper) or open the
solution in Visual Studio. It does not affect building from a fresh clone elsewhere.

**Linux/macOS: `Could NOT find PkgConfig` / SDL2 packages.**
Install `pkg-config` and the SDL2 dev packages (see section 0). On macOS make sure Homebrew's
`pkg-config` is on `PATH`.

**Want a debug/developer build?** Use the `Debug` configuration (`./build.ps1 -x64` without
`-Release` on Windows, or `-DCMAKE_BUILD_TYPE=Debug` on Linux/macOS). Cheat/debug mode is a
compile-time flag (it then reads which cheats to enable from runtime settings); it is not enabled
in a stock Release build.

---

*Questions or a step that didn't work? Open an issue. A PR improving this guide is welcome.*
