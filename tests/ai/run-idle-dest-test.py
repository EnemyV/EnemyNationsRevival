"""Compile cairoute.cpp's IdleDestUsesMaterial verbatim against stock-shaped building data.

IdleTruckTask( ..., bAllowEmptyDest = TRUE ) parks a truckload in an EMPTY factory, yard
or repair bay. The predicate decides which of those can ever use the material. This runs
the production text against the stock ENATIONS.DAT build/repair tables (transcribed in
test_idle_dest.cpp) and checks every building in IdleTruckTask's num_types set.

Not covered: the IdleTruckTask loop itself (CAIRouter, AiSnap, the unit lists) - only
the predicate it calls.

Build output goes to the system temp directory, never into the source tree.
"""
import argparse
import hashlib
import re
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-ref', help='Read cairoute.cpp from this Git revision')
parser.add_argument('--perturb', choices=['anyfactory'],
                    help='Corrupt the predicate, to run the test in its failing direction')
args = parser.parse_args()

rel = 'enations_latest/src/cairoute.cpp'
raw = (subprocess.check_output(['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + rel]).decode()
       if args.baseline_ref else (ROOT / rel).read_text(encoding='utf-8'))
m = re.search(r'^static BOOL IdleDestUsesMaterial\(', raw, re.M)
if m is None:
    print('FAIL: IdleDestUsesMaterial is not in', args.baseline_ref or 'the working tree')
    sys.exit(1)
body = raw[m.start():raw.index('\n}', m.end()) + 2]   # the closing brace is unindented
if args.perturb == 'anyfactory':
    old = 'return ( pSd->GetBldVehicle( )->GetTotalInput( iMat ) > 0 );'
    assert old in body
    body = body.replace(old, 'return ( TRUE );')

out = Path(tempfile.gettempdir()) / 'en-idle-dest-test' / (args.perturb or args.baseline_ref or 'candidate')
out.mkdir(parents=True, exist_ok=True)
(out / 'idle_dest_actual.inc').write_text(body, encoding='utf-8')
print('Production function SHA256:', hashlib.sha256(body.encode()).hexdigest(), flush=True)

vs = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
exe = out / 'idle_dest_test.exe'
batch = out / 'compile.cmd'
batch.write_text(f'@echo off\ncall "{vs}" >nul 2>&1\nif errorlevel 1 exit /b 2\n'
                 f'cl /nologo /EHsc /std:c++17 /W4 /I"{out}" "{HERE / "test_idle_dest.cpp"}" '
                 f'/Fo"{out / "idle_dest.obj"}" /Fe"{exe}"\nexit /b %errorlevel%\n', encoding='utf-8')
if subprocess.run(['cmd', '/c', str(batch)], cwd=out).returncode:
    sys.exit(2)
rc = subprocess.run([str(exe)], cwd=out, timeout=60).returncode
if args.perturb:
    if rc == 0:
        print('THE PERTURBED RUN PASSED - the test does not check that case')
        sys.exit(1)
    print('perturbed run failed as required')
    sys.exit(0)
sys.exit(rc)
