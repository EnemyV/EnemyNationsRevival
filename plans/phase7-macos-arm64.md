# Phase 7 — macOS (Apple Silicon / ARM64) port

Status: **planning** — no code changes yet. Created 2026-06-07.

Goal: get *Enemy Nations* compiling and running on **macOS ARM64**
(Apple M-series), single-player first, multiplayer later.

This doc scopes the cross-platform build that Phase 6
([phase6-sdl-rendering-port.md](phase6-sdl-rendering-port.md)) explicitly
defers ("After this, we can attempt a Linux build (probably as Phase 7)").
Phase 6 Stage 5 (GDI removal) is a **hard prerequisite** for the engine
render path here; Stages 0–4 (DirectDraw removal) **already landed** — the
off-screen `CDIB` is backed by an `SDL_Surface`, no `ddraw.h`/`DDRAW.DLL`.
Linux falls out of the same work — nothing below is Mac-specific except the
toolchain/packaging section, so treat "macOS ARM64" as the first
non-Windows target, not a one-off.

## TL;DR effort shape

The hard 1996-era blockers are already gone or irrelevant. The port is
dominated by **one task: getting Win32 out of the engine.** Rough shape:

| Area | State | Effort |
|------|-------|--------|
| 64-bit correctness | **done** (x64 builds, asm removed, ptr-trunc fixed) | — |
| Endianness | **non-issue** (both LE) | — |
| SDL2 window/audio/dialogs/input | **done** (already SDL2) | — |
| DirectDraw removal | **done** (Phase 6 St.0–4 landed; CDIB backed by SDL_Surface) | — |
| GDI removal | not started (Phase 6 St.5) | open-ended |
| **Win32 API abstraction seam** | **not started** | **weeks (dominant)** |
| Video (VFW/Indeo) | likely drop → SDL2VideoPlayer | days |
| Multiplayer net (vdmplay/NetBIOS/IPX) | TCP rewrite or defer | weeks (or defer) |
| Audio codecs (ACM/mmio) | verify dead vs SDL_mixer | small/unknown |
| Toolchain + .app packaging | not started | days |

**Recommended target for first milestone: single-player, no cutscenes,
no netplay, reaching the "world generation" smoke-test on ARM64.**

## What already helps (don't redo this)

- **Already 64-bit.** `./build.ps1 -x64` produces working PE32+ binaries;
  inline asm removed, pointer-truncation fixed, CMake arch-aware
  ([project_x64_build.md]). ARM64 is 64-bit too → that bug class handled.
- **Endianness is a non-issue.** Win-x64 and macOS-ARM64 are both
  little-endian. Save files, `ENATIONS.DAT`, baked terrain PNGs round-trip
  with no byte-swapping.
- **No live inline asm** (only dead `wind22/include/thielen.h`).
- **SDL2 already owns** window creation, compositor, input routing, audio
  (SDL_mixer), TTF dialog text, and all 18 `SDL2*` dialog reimplementations.
  None of that needs porting.
- **MFC is gone** — `mfc_compat.h` replaced it (no `mfc*.dll`). But the
  shim itself is Win32-based; see item 1.

## Work items

### 1. Win32 API abstraction seam — the dominant task

There is **no `#ifdef _WIN32` portability seam anywhere yet.** The whole
game compiles as a Win32 program. Surface to abstract:

- **`mfc_compat.h` (2118 lines) is built directly on `windows.h`.**
  `CFile` wraps `CreateFileA`/`ReadFile`/`HANDLE`; `CWinThread` /
  `AfxBeginThread` wrap `CreateThread`; `CString::LoadString` →
  `LoadStringA`; `CRect`/`CPoint`/`CSize` inherit Win32 `RECT`/`POINT`/`SIZE`;
  the `CDC` shim calls `DrawTextA`/`TextOutA`. Each needs a POSIX/SDL backing.
- **~462 raw `HWND`/`HDC`/`WM_*`/`SendMessage` refs across 37 game `.cpp`
  files** (`unit_wnd.cpp`=51, `area.cpp`=45, `lastplnt.cpp`=32,
  `toolbar.cpp`=16, `world.cpp`=17…). The window/message glue is Win32
  throughout the game-side UI.
- **Threading: ~959 Win32 sync refs** (`CreateThread`, `CRITICAL_SECTION`,
  `InterlockedIncrement`, `WaitForSingleObject`) across the AI threads,
  pathing, and `wind22/threads.cpp`. Needs a `std::thread`/`std::mutex`/
  `std::atomic` (or pthread) backing. The AI is genuinely multithreaded
  and has a history of races already fixed ([project_pathmap_heap_corruption],
  [project_ai_borrowed_list_uaf]) — a primitives swap risks reopening them,
  so it must be correct, not stubbed.
- **Settings:** `GetProfileInt`/`WriteProfileString` (`EN_*`, `[Advanced]`
  knobs) — already partly funneled through `EnSettings`/`w22_settings`, so
  closer to done than the rest.

**Approach:** introduce a `platform/` seam — a `win32_compat.h` that on
`_WIN32` just `#include`s `windows.h`, and elsewhere provides POSIX/SDL
backings. Move `mfc_compat.h`'s Win32 dependencies behind it so the Win32
surface becomes *countable* and shrinkable. Land it on Windows first
(no behavior change) before flipping to a Mac build.

> **LP64 → LLP64 trap (do this while building the seam).** Windows x64 is
> LLP64 (`long` == 32 bits); macOS ARM64 is LP64 (`long` == 64 bits). The
> `CArchive` serializers use `MFC_COMPAT_AR_OP(LONG)` and `sizeof(DWORD)`.
> If `DWORD`/`LONG`/`UINT` get redefined as `unsigned long` on Mac, **the
> save-file and network wire formats silently change width and corrupt.**
> Pin every Win32 typedef in the shim to fixed-width (`uint32_t`,
> `int32_t`, …), not `long`.

### 2. DirectDraw removal — DONE (Phase 6 Stages 0–4 landed)

Already complete. The off-screen `CDIB` is backed by an `SDL_Surface`:
`CalcBltMethod` returns `DIB_SDL_SURFACE` by default (blt.cpp:163), no
`ddraw.h` include, no `DirectDrawCreate`/`LoadLibrary("DDRAW")`, no
`ddraw.lib` link → no `DDRAW.DLL` import. init.cpp:78 confirms
*"Phase 6 Stage 4: DirectDraw removed entirely."* Nothing to do for the Mac
port here. (One vestige: a DirectSound version check still calls
`GetDllVersion("dsound.dll", …)` at init.cpp:81 — guard it out on non-Windows
since `dsound.dll` won't exist.)

### 3. GDI removal — Phase 6 Stage 5 (open-ended)

Remaining `BitBlt`/`StretchBlt`/`DrawText`/`CreateCompatibleDC` in `wind22`
(`dib.cpp`, `blt.cpp`, `apppalet.cpp`) plus game-world HUD text
(`area.cpp`, `toolbar.cpp`, `icons.cpp`, `credits.cpp`). The Phase 6 audit
found `subclass.cpp` (2250 lines of GDI) and palette-cycling **runtime-dead
→ exclude, don't port**, cutting the work. The live HUD-text path and the
`RectVisible()` clip test still need SDL/portable replacements. This is the
genuine non-Windows blocker Phase 6 flags as open-ended.

### 4. Video (VFW / Indeo) — drop, don't port

`stdafx.h` includes `vfw.h`; CMake links `vfw32.lib` + the `vdmplay` DLL.
Cutscenes use Indeo `.avi` via Video-for-Windows, which **does not exist on
macOS**. A `SDL2VideoPlayer` already replaced `CWndMovie`
([project archived note]). Path: confirm VFW is fully dead in the live
path, route all video through the SDL2 player (or ship without cutscenes
for the first milestone). Drop `vfw.h`/`vfw32.lib` from the non-Windows
build.

### 5. Multiplayer networking — TCP rewrite or defer (biggest non-render unknown)

The net layer (`m_vpHdl`/`m_vpSession`, 22 refs in `netapi.cpp`) goes
through the **`vdmplay` Win32 DLL**, which speaks TCP / **IPX / NetBIOS /
COMM / MODEM / TAPI** (`tools/vdmplay/base.h`). CMake links `Netapi32.lib`;
`network/netbios.cpp` exists. IPX, NetBIOS, and Netapi32 are **Windows-only
with no macOS equivalent.** TCP is portable (Winsock ≈ BSD sockets).

**Decision:** for the first milestone, **defer multiplayer** (stub the net
layer, single-player only). Later: **TCP-only multiplayer on BSD sockets**,
dropping the IPX/NetBIOS/COMM/MODEM transports. This is effectively a net
rewrite, not a port — keep it off the critical path.

### 6. Audio codecs — verify, probably small

`wind22/acmutil.cpp` + `mmio.cpp` use Windows ACM (`acmStreamOpen`) and
`mmio`. Most audio already moved to SDL_mixer
([sdl-audio-migration-plan.md]). **Verify** whether any sound-effect/music
decode still routes through ACM at runtime; if so, replace with a portable
decoder (SDL_mixer/SDL_sound already vendored). Likely small.

### 7. Toolchain + packaging (days, mechanical)

The CMakeLists are 100% MSVC and need a Clang/Apple branch:
- Flags: `/MD /O2 /Ob2 /arch:AVX2 /fp:fast /EHsc` → `-O2 -ffast-math`;
  `/arch:AVX2` → `-mcpu=apple-m1` (NEON, no AVX on ARM).
- `/SAFESEH`, `/LARGEADDRESSAWARE`, `/DYNAMICBASE`, `/NXCOMPAT`,
  `/NODEFAULTLIB:nafxcw.lib` — all MSVC-link-only, guard behind `if(MSVC)`.
- `__declspec(selectany)` → `__attribute__((weak))` / `inline`; `__cdecl`,
  `WINAPI`, `BASED_CODE` → empty macros on non-Windows.
- `add_executable(enations WIN32 …)` → a macOS `.app` bundle
  (`MACOSX_BUNDLE`) with `Info.plist`.
- SDL2/SDL2_ttf/SDL2_mixer via Homebrew or vendored `.framework` (the
  vendored win32 import-libs and DLL-copy POST_BUILD steps are Windows-only).
- `target_precompile_headers(enations PRIVATE stdafx.h)` works under Clang
  but `stdafx.h` itself must be made portable first (item 1).
- Eventually: codesigning + notarization for distribution.

## Recommended phasing

1. ~~Phase 6 Stages 0–4 (DirectDraw removal)~~ — **already done.**
2. **Build the `platform/` seam** (item 1) on Windows, no behavior change.
   Pin the shim to fixed-width types (LP64 trap). Land incrementally so the
   Win32 surface is countable.
3. **Phase 6 Stage 5** (GDI → SDL), excluding dead `subclass.cpp`/palette.
4. **Drop VFW**, confirm video routes through `SDL2VideoPlayer`.
5. **First Mac build: single-player only**, netplay + cutscenes stubbed.
   Target the "reaches world generation" smoke-test on ARM64.
6. **TCP-only multiplayer** on BSD sockets as a follow-up.

## Biggest risks / unknowns

- **GDI HUD text** (Stage 5) — the genuine open-ended render item.
- **Threading correctness** under non-Win32 primitives — the AI is
  real-multithreaded with a history of races; a primitives swap can reopen
  them. Stress-test combat + teardown after the swap.
- **LP64 width change** silently corrupting save/wire formats if the shim
  isn't fixed-width. Add a save-file round-trip test across the two builds.
- **Multiplayer** is a rewrite, not a port — kept off the critical path.

## Verification

- Build clean under Clang for ARM64; then under MSVC for Win32/x64
  (the seam must not regress Windows — same rule as the MFC removal).
- Reach the **world-generation** smoke-test milestone on macOS.
- Save-file round-trip parity between the Windows and Mac builds (load a
  Windows save on Mac and vice-versa) — guards the LP64 trap.
- `./mfc-status.ps1` still `mfc linked: NO` on the Windows build.
