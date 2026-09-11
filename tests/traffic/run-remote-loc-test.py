"""Compile the production location/apply methods against a minimal scene.

Tests the receiving contract and local-facing control cases, not the network
transport or a two-peer game. Production method bodies are extracted verbatim.
"""
import argparse
import hashlib
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-ref', help='Read production methods from this Git revision')
args = parser.parse_args()
out = HERE / 'remote-loc-out' / ('baseline' if args.baseline_ref else 'candidate')
out.mkdir(parents=True, exist_ok=True)
path = 'enations_latest/src/vehmove.cpp'
source = (subprocess.check_output(['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + path]).decode()
          if args.baseline_ref else (ROOT / path).read_text(encoding='utf-8'))


def method(signature, source=source):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    # SetLoc's release signature is inside a debug/release preprocessor branch.
    header = '\n'.join(line for line in source[start:opening].splitlines()
                       if not line.lstrip().startswith('#'))
    return header + '\n' + source[opening:end]


actual = method('void CVehicle::SetLoc(BOOL)') + '\n' + method('void CVehicle::SetFromMsg(')
path_source = (subprocess.check_output(['git', '-C', str(ROOT), 'show',
                                      args.baseline_ref + ':enations_latest/src/cpathmgr.cpp']).decode()
               if args.baseline_ref else (ROOT / 'enations_latest/src/cpathmgr.cpp').read_text(encoding='utf-8'))
actual += '\n' + method('BOOL CPathMgr::IsHexMovingVehicle(', path_source)
(out / 'remote_loc_actual.inc').write_text(actual, encoding='utf-8')
print('Production methods SHA256:', hashlib.sha256(actual.encode()).hexdigest(), flush=True)
vs = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
batch = out / 'compile.cmd'
exe = out / 'remote_loc_test.exe'
batch.write_text(f'@echo off\ncall "{vs}" >nul 2>&1\nif errorlevel 1 exit /b 2\n'
                 f'cl /nologo /EHsc /std:c++17 /W4 /I"{out}" "{HERE / "test_remote_loc.cpp"}" '
                 f'/Fo"{out / "remote_loc.obj"}" /Fe"{exe}"\nexit /b %errorlevel%\n', encoding='utf-8')
compiled = subprocess.run(['cmd', '/c', str(batch)], cwd=out)
if compiled.returncode:
    sys.exit(2)
sys.exit(subprocess.run([str(exe)], cwd=out, timeout=20).returncode)
