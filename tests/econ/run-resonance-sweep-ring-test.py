"""Compile and run the production Resonance Sweep light/unlight methods at /Od and /O2."""
import os
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SRC = ROOT / "enations_latest/src/player.cpp"
source = SRC.read_text(encoding="utf-8")
start = source.index("struct SweepLightContext")
end = source.index("void CPlayer::ResonanceSweep(", start)
actual = source[start:end]
if "pContext->pLitHexes->push_back( hex )" not in actual:
    raise SystemExit("ring source extraction missed coordinate recording")
if source.count("m_aSweepLitHexes.clear( );") < 4:
    raise SystemExit("ring coordinates must clear at construction, load, light, and release")

vs_roots = [
    Path("C:/Program Files/Microsoft Visual Studio/2022/Community"),
    Path("C:/Program Files/Microsoft Visual Studio/2022/Enterprise"),
    Path("C:/Program Files/Microsoft Visual Studio/2022/Professional"),
]
vcvars = next((p / "VC/Auxiliary/Build/vcvars64.bat" for p in vs_roots
               if (p / "VC/Auxiliary/Build/vcvars64.bat").exists()), None)
if vcvars is None:
    print("VS 2022 vcvars64.bat not found.", file=sys.stderr)
    sys.exit(2)

out_root = Path("D:/Enemy Nations/local-notes/resonance-ring-test")
for opt in ("Od", "O2"):
    out = out_root / opt
    out.mkdir(parents=True, exist_ok=True)
    (out / "ring_actual.inc").write_text(actual, encoding="utf-8")
    batch = out / "compile.cmd"
    exe = out / "ring_test.exe"
    batch.write_text(
        "@echo off\r\n"
        f'call "{vcvars}" >nul 2>&1\r\n'
        "if errorlevel 1 exit /b 2\r\n"
        f'cl /nologo /EHsc /std:c++17 /W4 /{opt} /I"{out}" '
        f'"{HERE / "test_resonance_sweep_ring.cpp"}" '
        f'/Fo"{out / "ring_test.obj"}" /Fe"{exe}"\r\n'
        "exit /b %errorlevel%\r\n", encoding="utf-8")
    print(f"--- /{opt} ---", flush=True)
    env = {k.upper(): v for k, v in os.environ.items()}
    if subprocess.run(["cmd", "/c", str(batch)], cwd=out, env=env).returncode:
        sys.exit(2)
    if subprocess.run([str(exe)], cwd=out, env=env, timeout=20).returncode:
        sys.exit(1)
