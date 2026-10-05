"""Extract and exercise the production Resonance Sweep timer at /Od and /O2.

The timer prefix and reload calculation are taken verbatim from player.cpp. A
small fake CPlayer supplies only the state and methods those statements touch;
no gameplay body is copied into the fixture. The frame-clock callsite is also
guarded so simulation speed cannot scale the elapsed real-time argument.
"""
import hashlib
import argparse
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-ref', help='read production sources from this Git revision')
args = parser.parse_args()


def read_source(relative):
    if args.baseline_ref:
        return subprocess.check_output(
            ['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + relative]).decode('utf-8')
    return (ROOT / relative).read_text(encoding='utf-8')


source = read_source('enations_latest/src/player.cpp')
mainloop = read_source('enations_latest/src/mainloop.cpp')


def body(signature, text):
    start = text.index(signature)
    opening = text.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


call_starts = list(re.finditer(r'ResonanceSweep\s*\(', mainloop))
if len(call_starts) != 1:
    raise SystemExit(f'expected exactly one ResonanceSweep callsite, found {len(call_starts)}')
call_start = call_starts[0].start()
call_open = mainloop.index('(', call_start)
depth, call_end = 1, call_open + 1
while depth:
    depth += (mainloop[call_end] == '(') - (mainloop[call_end] == ')')
    call_end += 1
call_arg = mainloop[call_open + 1:call_end - 1]
if not re.fullmatch(r'\s*theGame\.m_dwFramesElapsed\s*\*\s*\(\s*1000\s*/\s*FRAME_RATE\s*\)\s*', call_arg):
    raise SystemExit('mainloop must pass unscaled elapsed frame milliseconds to ResonanceSweep')
scope_start = mainloop.rfind('if (theGame.ShouldOperate()', 0, call_start)
if scope_start < 0:
    raise SystemExit('sweep call must run inside the ShouldOperate block')
opening = mainloop.index('{', scope_start)
depth, scope_end = 1, opening + 1
while depth:
    depth += (mainloop[scope_end] == '{') - (mainloop[scope_end] == '}')
    scope_end += 1
if call_start >= scope_end:
    raise SystemExit('sweep call is outside the ShouldOperate block')
if re.search(r'ResonanceSweep\s*\(\s*theGame\.GetOperSecElapsed\s*\(', mainloop):
    raise SystemExit('legacy game-second ResonanceSweep call remains')
clock_assignment = re.search(r'm_dwFramesElapsed\s*=\s*dtFrame\.quot\s*;', mainloop)
if not clock_assignment:
    raise SystemExit('mainloop frame elapsed must come from wall-clock dtFrame, not simulation speed')
oper_assignment = re.search(r'm_dwOpersElapsed\s*=\s*theGame\.m_dwFramesElapsed\s*\*\s*theGame\.m_iSpeedMul\s*;', mainloop)
if not oper_assignment:
    raise SystemExit('expected simulation-speed scaling to remain separate from elapsed real frames')
print('Callsite guard: sweep receives dtFrame frames; simulation speed is applied only to oper elapsed.', flush=True)

reload_body = body('int CPlayer::GetSweepReloadSecs( ) const', source)
sweep_body = body('void CPlayer::ResonanceSweep( int iElapsedMillis )', source)
opening = sweep_body.index('{')
prefix_end = sweep_body.index('    // Pass 1 - count the candidates.')
timer_prefix = sweep_body[opening:prefix_end]
if 'm_iSweepMillis = 0;' not in timer_prefix or 'm_iSweepLitMillis -= iElapsedMillis;' not in timer_prefix:
    raise SystemExit('timer prefix extraction did not find the expected timer state')
selection_start = sweep_body.index('    // Pass 1 - count the candidates.')
selection_end = sweep_body.index('    std::string sWho = pRocket->GetOwner( )->GetName( );')
selection = sweep_body[selection_start:selection_end]
# The production method returns void. Adapt only its two no-selection exits to
# null for the standalone selector seam; its counting, filtering, RNG, and
# chosen-pointer walk remain verbatim.
selection = selection.replace('if ( nRocket <= 0 )\n        return;', 'if ( nRocket <= 0 )\n        return NULL;')
selection = selection.replace('if ( pRocket == NULL )\n        return;', 'if ( pRocket == NULL )\n        return NULL;')

# Keep the production reload method verbatim. The timer prefix is the beginning
# of the production method body; a final marker records that it reached a ping.
actual = reload_body + '\n'
actual += 'void CPlayer::TimerTick(int iElapsedMillis)\n' + timer_prefix + '    m_bPingTriggered = true;\n}\n'
actual += 'CBuilding* CPlayer::SelectRocketForTest() {\n' + selection + '    return pRocket;\n}\n'
print('Production timer SHA256:', hashlib.sha256(actual.encode()).hexdigest(), flush=True)

vs_roots = [
    Path('C:/Program Files/Microsoft Visual Studio/2022/Community'),
    Path('C:/Program Files/Microsoft Visual Studio/2022/Enterprise'),
    Path('C:/Program Files/Microsoft Visual Studio/2022/Professional'),
]
vcvars = next((p / 'VC/Auxiliary/Build/vcvars64.bat' for p in vs_roots
               if (p / 'VC/Auxiliary/Build/vcvars64.bat').exists()), None)
if vcvars is None:
    print('VS 2022 vcvars64.bat not found.', file=sys.stderr)
    sys.exit(2)

out_root = Path('D:/Enemy Nations/local-notes/resonance-sweep-test')
if args.baseline_ref:
    out_root = out_root / 'baseline'
for opt in ('Od', 'O2'):
    out = out_root / opt
    out.mkdir(parents=True, exist_ok=True)
    (out / 'sweep_actual.inc').write_text(actual, encoding='utf-8')
    batch = out / 'compile.cmd'
    exe = out / 'sweep_test.exe'
    batch.write_text(
        '@echo off\r\n'
        f'call "{vcvars}" >nul 2>&1\r\n'
        'if errorlevel 1 exit /b 2\r\n'
        f'cl /nologo /EHsc /std:c++17 /W4 /{opt} /I"{out}" "{HERE / "test_resonance_sweep.cpp"}" '
        f'/Fo"{out / "sweep_test.obj"}" /Fe"{exe}"\r\n'
        'exit /b %errorlevel%\r\n', encoding='utf-8')
    print(f'--- /{opt} ---', flush=True)
    env = {k.upper(): v for k, v in __import__('os').environ.items()}
    if subprocess.run(['cmd', '/c', str(batch)], cwd=out, env=env).returncode:
        sys.exit(2)
    if subprocess.run([str(exe)], cwd=out, env=env, timeout=20).returncode:
        sys.exit(1)
