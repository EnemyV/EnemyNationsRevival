"""Compile the PRODUCTION vehicle window teardown + SDL2RouteWindow Close/dtor against a scene.

A vehicle that died with its Routes window open left the SDL2RouteWindow holding a dangling
CVehicle*: CVehicle::DestroyAllWindows closed the build and load dialogs but not the route
window, and every route button (and Close itself) dereferences m_pVeh. This suite extracts
the shipped bodies verbatim (tests/orders/run-order-lifecycle.py's technique):

    new_unit.cpp        CVehicle::DestroyRouteWindow, CVehicle::DestroyBuildWindow
    vehicle.cpp         CVehicle::DestroyAllWindows
    SDL2RouteWindow.cpp s_openRouteWindows, ~SDL2RouteWindow, Hide, Close

and drives them through vehicle death (PrepareToDie then ~CVehicle), the DestroyWorld path,
and a user Close followed by vehicle death. The real constructor is not compiled (it needs
the compositor, fonts and GameWindow); the scene's constructor does the one thing teardown
depends on - registering in s_openRouteWindows.

    python tests/ui/run-route-window-teardown.py
    python tests/ui/run-route-window-teardown.py --baseline-ref e4ea13a7   # fails: the bug

Exit codes: 0 all pass, 1 a check failed, 2 toolchain / compile error.
"""
import argparse
import hashlib
import os
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-ref', help='Read production bodies from this Git revision')
parser.add_argument('--opt', default='/Od', help='cl optimisation switch (default /Od)')
args = parser.parse_args()


def read(path):
    if args.baseline_ref:
        return subprocess.check_output(
            ['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + path]).decode('utf-8', 'replace')
    return (ROOT / path).read_text(encoding='utf-8', errors='replace')


def method(src, path, signature):
    """Verbatim body of one method, located by its signature and brace-matched."""
    if signature not in src:
        raise SystemExit('not found in %s: %s' % (path, signature))
    start = src.index(signature)
    opening = src.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (src[end] == '{') - (src[end] == '}')
        end += 1
    return src[start:end] + '\n'


parts = []
RW = 'enations_latest/src/SDL2RouteWindow.cpp'
rw = read(RW)
reg = 'static std::vector<SDL2RouteWindow*> s_openRouteWindows;'
if reg not in rw:
    raise SystemExit('registry declaration not found in ' + RW)
parts.append(reg + '\n')

for path, sigs in (('enations_latest/src/new_unit.cpp',
                    ('void CVehicle::DestroyRouteWindow( )', 'void CVehicle::DestroyBuildWindow( )')),
                   ('enations_latest/src/vehicle.cpp', ('void CVehicle::DestroyAllWindows()',)),
                   (RW, ('SDL2RouteWindow::~SDL2RouteWindow()', 'void SDL2RouteWindow::Hide()',
                         'void SDL2RouteWindow::Close()'))):
    src = read(path)
    for sig in sigs:
        parts.append(method(src, path, sig))

actual = '\n'.join(parts)
out = Path(tempfile.gettempdir()) / 'en-route-teardown' / ('baseline' if args.baseline_ref else 'candidate')
out.mkdir(parents=True, exist_ok=True)
(out / 'teardown_actual.inc').write_text(actual, encoding='utf-8')
print('[route-teardown] production bodies SHA256:', hashlib.sha256(actual.encode()).hexdigest(), flush=True)

roots = [Path(base) / 'Microsoft Visual Studio/2022' / e
         for base in (os.environ.get('ProgramFiles', 'C:/Program Files'),
                      os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)'))
         for e in ('Community', 'Enterprise', 'Professional', 'BuildTools')]
vs = next((r for r in roots if (r / 'VC/Auxiliary/Build/vcvars64.bat').exists()), None)
if vs is None:
    print('[route-teardown] VS 2022 vcvars64.bat not found')
    sys.exit(2)

batch = out / 'compile.cmd'
exe = out / 'route_window_teardown.exe'
batch.write_text(
    '@echo off\ncall "%s" >nul 2>&1\nif errorlevel 1 exit /b 2\n'
    'cl /nologo /EHsc /std:c++17 /W4 %s /I"%s" "%s" /Fo"%s" /Fe"%s"\n'
    'exit /b %%errorlevel%%\n'
    % (vs / 'VC/Auxiliary/Build/vcvars64.bat', args.opt, out,
       HERE / 'test_route_window_teardown.cpp', out / 'route_window_teardown.obj', exe),
    encoding='utf-8')
if subprocess.run(['cmd', '/c', str(batch)], cwd=str(out)).returncode:
    sys.exit(2)
sys.exit(subprocess.run([str(exe)], cwd=str(out), timeout=30).returncode)
