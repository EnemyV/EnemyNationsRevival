"""Compile the PRODUCTION road-step rule (queue-road ghost), driven verbatim.

Same technique as run-order-lifecycle.py / run-drag-place.py: CVehicle::RoadStepToward
is extracted verbatim out of enations_latest/src/vehicle.cpp into roadghost_actual.inc
and compiled against the CHexCoord stand-in in test_road_ghost.cpp (terrain.inl's
torus arithmetic, drag-place's technique: mask = eX-1, half = eX/2). So a check here is
a statement about the shipped step rule, not a mirror of it.

This script ALSO lints vehicle.cpp: CVehicle::_NextRoadHex (the function the crane's own
BuildRoad/NextRoadHex actually call) must still call RoadStepToward(...) rather than
carrying its own copy of the arithmetic -- the road-ghost draw (SDL2Terrain.cpp) calls
RoadStepToward too, and the whole point of the extraction is that the two callers cannot
drift apart. A reversion that re-inlines the arithmetic into _NextRoadHex fails this
lint even if RoadStepToward itself still exists and still passes every other check.

    python tests/orders/run-road-ghost.py
    python tests/orders/run-road-ghost.py --opt /O2
    python tests/orders/run-road-ghost.py --baseline-ref <commit>

--baseline-ref reads production vehicle.cpp from an older revision: 89772cb1 (the
integration tip this work is based on) has no RoadStepToward at all -- the positive
control for "this is reading the real source", not a stand-in.

Exit codes: 0 all pass, 1 a check failed, 2 toolchain / compile / extraction / lint error.
"""
import argparse
import hashlib
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-ref', help='Read the production source from this Git revision')
parser.add_argument('--opt', default='/Od', help='cl optimisation switch (default /Od)')
args = parser.parse_args()

VEH = 'enations_latest/src/vehicle.cpp'


def read(path):
    if args.baseline_ref:
        return subprocess.check_output(
            ['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + path]).decode('utf-8', 'replace')
    return (ROOT / path).read_text(encoding='utf-8', errors='replace')


source = read(VEH)


def brace_match(src, start):
    opening = src.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (src[end] == '{') - (src[end] == '}')
        end += 1
    return opening, end


def method(signature, src=source):
    """Verbatim text of one method, located by signature and brace-matched."""
    if signature not in src:
        print('[orders] NOT FOUND in %s: %s' % (VEH, signature))
        sys.exit(2)
    start = src.index(signature)
    opening, end = brace_match(src, start)
    header = '\n'.join(l for l in src[start:opening].splitlines() if not l.lstrip().startswith('#'))
    return header + '\n' + src[opening:end] + '\n'


# THE step rule -- lifted verbatim, compiled and driven by test_road_ghost.cpp.
actual = method('CHexCoord CVehicle::RoadStepToward(')

out = HERE / 'roadghost-out' / ('baseline' if args.baseline_ref else 'candidate')
out.mkdir(parents=True, exist_ok=True)
(out / 'roadghost_actual.inc').write_text(actual, encoding='utf-8')
print('[orders] production CVehicle::RoadStepToward (vehicle.cpp) SHA256:',
      hashlib.sha256(actual.encode()).hexdigest(), flush=True)

# LINT: _NextRoadHex must still call the shared function, not its own arithmetic.
next_road_hex = method('CHexCoord CVehicle::_NextRoadHex(CHexCoord const &_hexOn)')
if 'RoadStepToward(' not in next_road_hex:
    print('[orders] LINT FAILED: CVehicle::_NextRoadHex no longer calls RoadStepToward() -- '
          'the road-ghost draw and the crane\'s own walk can drift apart again')
    sys.exit(2)
print('[orders] lint OK: _NextRoadHex calls RoadStepToward()', flush=True)

roots = [Path('C:/Program Files/Microsoft Visual Studio/2022') / e for e in
         ('Community', 'Enterprise', 'Professional')]
vs = next((r for r in roots if (r / 'VC/Auxiliary/Build/vcvars64.bat').exists()), None)
if vs is None:
    print('[orders] VS 2022 vcvars64.bat not found')
    sys.exit(2)

batch = out / 'compile.cmd'
exe = out / 'road_ghost.exe'
batch.write_text(
    '@echo off\ncall "%s" >nul 2>&1\nif errorlevel 1 exit /b 2\n'
    'cl /nologo /EHsc /std:c++17 /W4 /D_CRT_SECURE_NO_WARNINGS %s /I"%s" "%s" /Fo"%s" /Fe"%s"\n'
    'exit /b %%errorlevel%%\n'
    % (vs / 'VC/Auxiliary/Build/vcvars64.bat', args.opt, out,
       HERE / 'test_road_ghost.cpp', out / 'road_ghost.obj', exe),
    encoding='utf-8')
if subprocess.run(['cmd', '/c', str(batch)], cwd=str(out)).returncode:
    sys.exit(2)
sys.exit(subprocess.run([str(exe)], cwd=str(out), timeout=30).returncode)
