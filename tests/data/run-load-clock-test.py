"""The game clock across LetsGo, on the SHIPPED text, at /Od and /O2.

CGame::Serialize restores m_dwElapsedTime from the save; LetsGo then runs for every game start.
A loaded game (load_single, load_multi host, load_join seat) must keep that clock; a new or
joined game (scenario, single, create_net, join_net) must start at 0.

Extracts VERBATIM: the CCreateBase type enum (new_game.h), Get/SetElapsedSeconds (player.h) and
the clock statement(s) in CConquerApp::LetsGo between the window show and the sim-clock-debt
stamp (newworld.cpp), then compiles them into test_load_clock.cpp.

    python tests/data/run-load-clock-test.py
    python tests/data/run-load-clock-test.py --baseline-ref 8267d09d   # expected to FAIL

Nothing is written into the source tree; artifacts go to %TEMP%/en-load-clock-test.
Exit codes: 0 all pass, 1 a check failed, 2 toolchain / compile / extraction error.
"""
import argparse
import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root', type=Path, default=ROOT)
parser.add_argument('--out-dir', type=Path, default=Path(tempfile.gettempdir()) / 'en-load-clock-test')
parser.add_argument('--baseline-ref', help='Extract from this Git revision instead of the working tree.')
args = parser.parse_args()


def read_source(name):
    rel = 'enations_latest/src/' + name
    if args.baseline_ref:
        return subprocess.check_output(
            ['git', '-C', str(args.source_root), 'show', args.baseline_ref + ':' + rel]).decode('utf-8', 'replace').replace('\r\n', '\n')
    return (args.source_root / rel).read_text(encoding='utf-8', errors='replace').replace('\r\n', '\n')


def once(source, text):
    if source.count(text) != 1:
        raise ValueError(f'{text.strip()[:60]!r} found {source.count(text)} times')
    return source.index(text)


try:
    new_game = read_source('new_game.h')
    player_h = read_source('player.h')
    newworld = read_source('newworld.cpp')
    player = read_source('player.cpp')

    e0 = once(new_game, 'enum { scenario,')
    enum = new_game[e0:new_game.index(';', e0) + 1]

    accessors = ''
    for sig in ('DWORD GetElapsedSeconds( ) const', 'void  SetElapsedSeconds( int iSec )'):
        a = once(player_h, sig)
        accessors += player_h[a:player_h.index('}', a) + 1] + '\n'

    lets_go = once(newworld, 'void CConquerApp::LetsGo() {')
    show_end = once(newworld, '        _UpdateWin(&m_wndBldgs);\n    }\n') + len('        _UpdateWin(&m_wndBldgs);\n    }\n')
    debt = once(newworld, '    // ZERO THE SIM-CLOCK DEBT')
    destroy = newworld.index('    DestroyExceptMain();', debt)
    tail = newworld[show_end:debt]
    incs = {
        'lc_enum.inc': enum + '\n',
        'lc_accessors.inc': accessors,
        'lc_tail.inc': tail,
    }
except ValueError as e:
    print('extraction failed:', e)
    sys.exit(2)

# SOURCE LINTS: the facts the scene relies on, read from the same revision.
lints = [
    # the clock statement runs INSIDE LetsGo, before DestroyExceptMain deletes m_pCreateGame
    (lets_go < show_end < debt < destroy and newworld.rfind('\nvoid CConquerApp::', 0, debt) == lets_go - 1,
     'clock statement sits in LetsGo before DestroyExceptMain'),
    # Serialize restores the clock on every load (host file read AND load_join's in-memory copy)
    ('ar >> m_dwOperSecElapsed >> m_dwOperSecFrames >> m_dwElapsedTime >> m_dwFrame;' in player,
     'CGame::Serialize reads m_dwElapsedTime'),
    # the load-join seat reads the save the host sent through the same Serialize
    ('theGame.Serialize( ar );' in read_source('join.cpp'), 'CJoinMulti::GameLoaded runs CGame::Serialize'),
]
lint_fail = 0
for ok, name in lints:
    print(('PASS' if ok else 'FAIL') + ' lint: ' + name)
    lint_fail += not ok

print('Production text SHA256:', hashlib.sha256(''.join(incs.values()).encode()).hexdigest(), flush=True)

vs_roots = [Path('C:/Program Files/Microsoft Visual Studio/2022/' + e)
            for e in ('Community', 'Enterprise', 'Professional', 'BuildTools')]
vcvars = next((r / 'VC/Auxiliary/Build/vcvars64.bat' for r in vs_roots
               if (r / 'VC/Auxiliary/Build/vcvars64.bat').exists()), None)
if vcvars is None:
    print('VS 2022 vcvars64.bat not found.', file=sys.stderr)
    sys.exit(2)

failed = 1 if lint_fail else 0
for opt in ('Od', 'O2'):
    out = args.out_dir / ('baseline' if args.baseline_ref else 'tree') / opt
    out.mkdir(parents=True, exist_ok=True)
    for name, text in incs.items():
        (out / name).write_text(text, encoding='utf-8')
    exe = out / 'load_clock_test.exe'
    batch = out / 'compile.cmd'
    batch.write_text(
        '@echo off\r\n'
        f'call "{vcvars}" >nul 2>&1\r\n'
        'if errorlevel 1 exit /b 2\r\n'
        f'cl /nologo /EHsc /std:c++17 /W4 /{opt} /I"{out}" "{HERE / "test_load_clock.cpp"}" '
        f'/Fo"{out / "load_clock.obj"}" /Fe"{exe}"\r\n'
        'exit /b %errorlevel%\r\n', encoding='utf-8')
    print(f'--- /{opt} ---', flush=True)
    if subprocess.run(['cmd', '/c', str(batch)], cwd=out).returncode:
        sys.exit(2)
    if subprocess.run([str(exe)], cwd=out, timeout=60).returncode:
        failed = 1

sys.exit(failed)
