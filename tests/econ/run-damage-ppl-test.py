"""Compile the shipped building-damage and population bodies against a minimal scene.

Extracts VERBATIM: CUnit::DecDamagePoints (projbase.cpp), its declaration (unit.h), the
UnitDamage / UnitSetDamage net handlers (netapi.cpp), CPlayer::GetBldgArmorMult / AddPplBldg (player.h)
and the m_fPplMult statement from CPlayer::StartLoop (player.cpp).
Only the world around them (players, units, net messages, theGame) is a fake scene in
test_damage_ppl.cpp. Built and run at /Od and /O2.

    python tests/econ/run-damage-ppl-test.py
    python tests/econ/run-damage-ppl-test.py --baseline-ref e4ea13a7   # expected to FAIL

Nothing is written into the source tree; artifacts go to %TEMP%/en-damage-ppl-test.
Exit codes: 0 all pass, 1 a check failed, 2 toolchain / compile / extraction error.
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
parser.add_argument('--source-root', type=Path, default=ROOT)
parser.add_argument('--out-dir', type=Path, default=Path(tempfile.gettempdir()) / 'en-damage-ppl-test')
parser.add_argument('--baseline-ref', help='Extract from this Git revision instead of the working tree.')
args = parser.parse_args()


def read_source(name):
    rel = 'enations_latest/src/' + name
    if args.baseline_ref:
        return subprocess.check_output(
            ['git', '-C', str(args.source_root), 'show', args.baseline_ref + ':' + rel]).decode('utf-8', 'replace')
    return (args.source_root / rel).read_text(encoding='utf-8', errors='replace')


def body(signature, source):
    """The whole definition from its signature to the closing brace, verbatim."""
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


projbase = read_source('projbase.cpp')
netapi = read_source('netapi.cpp')
unit_h = read_source('unit.h')
player_h = read_source('player.h')
player_cpp = read_source('player.cpp')

try:
    decl = re.search(r'void\s+DecDamagePoints\s*\([^;]*\);', unit_h).group(0)
    unit_bodies = body('void CUnit::DecDamagePoints (', projbase)
    net_bodies = body('static void UnitDamage(', netapi) + body('static void UnitSetDamage(', netapi)
    player_bodies = body('float GetBldgArmorMult( )', player_h) + body('void AddPplBldg( int iAdd )', player_h)
    # CPlayer::StartLoop's workforce ratio, the consumer of m_iPplBldg (statement verbatim).
    i = player_cpp.index('    if ( m_iPplNeedBldg > 0 && m_iPplBldg < m_iPplNeedBldg )')
    ppl_mult = player_cpp[i:player_cpp.index('m_fPplMult = 1.0;', i) + len('m_fPplMult = 1.0;')] + '\n'
except (ValueError, AttributeError) as e:
    print('extraction failed:', e)
    sys.exit(2)

incs = {
    'dmg_decl.inc': decl + '\n',
    'dmg_unit.inc': unit_bodies,
    'dmg_net.inc': net_bodies,
    'dmg_player.inc': player_bodies,
    'dmg_pplmult.inc': ppl_mult,
}
print('Production bodies SHA256:', hashlib.sha256(''.join(incs.values()).encode()).hexdigest(), flush=True)

vs_roots = [Path('C:/Program Files/Microsoft Visual Studio/2022/' + e)
            for e in ('Community', 'Enterprise', 'Professional', 'BuildTools')]
vcvars = next((r / 'VC/Auxiliary/Build/vcvars64.bat' for r in vs_roots
               if (r / 'VC/Auxiliary/Build/vcvars64.bat').exists()), None)
if vcvars is None:
    print('VS 2022 vcvars64.bat not found.', file=sys.stderr)
    sys.exit(2)

failed = 0
for opt in ('Od', 'O2'):
    out = args.out_dir / ('baseline' if args.baseline_ref else 'tree') / opt
    out.mkdir(parents=True, exist_ok=True)
    for name, text in incs.items():
        (out / name).write_text(text, encoding='utf-8')
    exe = out / 'damage_ppl_test.exe'
    batch = out / 'compile.cmd'
    batch.write_text(
        '@echo off\r\n'
        f'call "{vcvars}" >nul 2>&1\r\n'
        'if errorlevel 1 exit /b 2\r\n'
        f'cl /nologo /EHsc /std:c++17 /W4 /{opt} /I"{out}" "{HERE / "test_damage_ppl.cpp"}" '
        f'/Fo"{out / "damage_ppl.obj"}" /Fe"{exe}"\r\n'
        'exit /b %errorlevel%\r\n', encoding='utf-8')
    print(f'--- /{opt} ---', flush=True)
    if subprocess.run(['cmd', '/c', str(batch)], cwd=out).returncode:
        sys.exit(2)
    if subprocess.run([str(exe)], cwd=out, timeout=60).returncode:
        failed = 1

sys.exit(failed)
