#!/usr/bin/env python3
"""
mac_sdlcheck.py — is this binary about to run on REAL SDL2, or on the sdl2-compat shim?

Why this is a hard gate and not a warning: Homebrew replaced the `sdl2` formula with
**sdl2-compat** (SDL2-API-over-SDL3), which accepts SDL_SetColorKey and then IGNORES it on
every blit. Every DIB sprite then renders on a bright magenta block. A whole regression run
against the shim is not "a run with one odd result" — it is worthless, and worse, it looks
exactly like a game bug. It cost a full session to identify the first time.

Two ways to get the shim without noticing:
  * build normally — CMake links /opt/homebrew/opt/sdl2/... which now IS sdl2-compat;
  * copy a fresh build into the shipped artifact tree — the artifact's OWN binary uses
    @loader_path and is fine, but yours still has the absolute Homebrew paths.

Discriminator: the shim's dylib contains the literal string "sdl2-compat" (38 occurrences
when measured); real SDL2 contains none. That is structural, unlike a version-number
threshold (real 2.32.10 vs shim 2.32.70), which is a guess about future releases.

Usage:  python3 harness/mac_sdlcheck.py <path-to-exe>     -> exit 0 real, 1 shim/unknown
        from mac_sdlcheck import check_sdl                -> (ok: bool, detail: str)
"""
import os, subprocess, sys

def _resolve(exe, install_name):
    if install_name.startswith("@loader_path/"):
        return os.path.join(os.path.dirname(os.path.abspath(exe)),
                            install_name[len("@loader_path/"):])
    if install_name.startswith("@executable_path/"):
        return os.path.join(os.path.dirname(os.path.abspath(exe)),
                            install_name[len("@executable_path/"):])
    return install_name

def check_sdl(exe):
    """(ok, detail). ok=False means the run would be invalid — do not trust its output."""
    if not os.path.exists(exe):
        return False, f"executable not found: {exe}"
    try:
        out = subprocess.run(["otool", "-L", exe], capture_output=True, text=True).stdout
    except Exception as e:
        return False, f"otool failed: {e}"
    name = next((l.split()[0] for l in out.splitlines()
                 if "libSDL2-2.0.0.dylib" in l and "libSDL2_" not in l), None)
    if not name:
        return False, "no libSDL2-2.0.0.dylib dependency found"
    lib = _resolve(exe, name)
    if not os.path.exists(lib):
        return False, f"SDL2 resolves to a missing file: {name} -> {lib}"
    try:
        s = subprocess.run(["strings", "-a", lib], capture_output=True, text=True).stdout
    except Exception as e:
        return False, f"strings failed on {lib}: {e}"
    hits = s.count("sdl2-compat")
    if hits:
        return False, (f"SDL2 is the sdl2-compat SHIM ({hits} marker strings) via {name}. "
                       f"Colour keying is broken; results are invalid. Relink with "
                       f"install_name_tool to a real SDL2 and re-codesign.")
    return True, f"real SDL2 via {name}"

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__.strip().splitlines()[-2]); sys.exit(2)
    ok, detail = check_sdl(sys.argv[1])
    print(("[ok]   " if ok else "[FAIL] ") + "SDL linkage — " + detail)
    sys.exit(0 if ok else 1)
