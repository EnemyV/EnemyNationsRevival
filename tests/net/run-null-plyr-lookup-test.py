"""Peer messages naming an unknown player, on the SHIPPED code, at /Od and /O2.

Extracts VERBATIM: CGame::_GetPlayer, GetPlayer, _GetPlayerByPlyr and GetPlayerByPlyr
(player.cpp), the ProcessMessage receive cases below and the StartFile / ErrPlaceBldg handlers
(netapi.cpp), then compiles them into
test_null_plyr_lookup.cpp, which runs each case on a player number nobody has (an access
violation is a FAIL) and on real players.

    python tests/net/run-null-plyr-lookup-test.py
    python tests/net/run-null-plyr-lookup-test.py --baseline-ref f5bd4a09   # expected to FAIL

Nothing is written into the source tree; artifacts go to %TEMP%/en-null-plyr-lookup-test.
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
parser.add_argument('--out-dir', type=Path, default=Path(tempfile.gettempdir()) / 'en-null-plyr-lookup-test')
parser.add_argument('--baseline-ref', help='Extract from this Git revision instead of the working tree.')
args = parser.parse_args()

# The ProcessMessage cases under test, by their first line.
CASES = [
    '    case CNetCmd::plyr_dying: {',
    '    case CNetCmd::ai_msg: {',
    '    case CNetCmd::cmd_to_hp: {',
    '    case CNetCmd::ai_gpf_takeover: {',
]
# Cases that only call a static handler, and those handlers.
CALL_CASES = ['start_file', 'err_place_bldg']
FUNCS = [
    'static void StartFile( CMsgStartFile* pMsg )',
    'static void ErrPlaceBldg( CMsgPlaceBldg* pMsg )',
]
LOOKUPS = [
    'CPlayer* CGame::_GetPlayer( int iNetNum ) const',
    'CPlayer* CGame::GetPlayer( int iNetNum ) const',
    'CPlayer* CGame::_GetPlayerByPlyr( int iPlyrNum ) const',
    'CPlayer* CGame::GetPlayerByPlyr( int iPlyrNum ) const',
]


def read_source(name):
    rel = 'enations_latest/src/' + name
    if args.baseline_ref:
        return subprocess.check_output(
            ['git', '-C', str(args.source_root), 'show', args.baseline_ref + ':' + rel]).decode('utf-8', 'replace').replace('\r\n', '\n')
    return (args.source_root / rel).read_text(encoding='utf-8', errors='replace').replace('\r\n', '\n')


def block(source, start_text):
    """From start_text through the brace that closes the first brace after it, verbatim."""
    if source.count(start_text) != 1:
        raise ValueError(f'{start_text.strip()!r} found {source.count(start_text)} times')
    start = source.index(start_text)
    depth, end = 1, source.index('{', start) + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


try:
    netapi = read_source('netapi.cpp')
    player = read_source('player.cpp')
    incs = {
        'npl_lookup.inc': ''.join(block(player, s) + '\n\n' for s in LOOKUPS),
        'npl_funcs.inc': ''.join(block(netapi, s) + '\n\n' for s in FUNCS),
        'npl_cases.inc': ''.join(block(netapi, s) + '\n' for s in CASES),
    }
    for c in CALL_CASES:
        m = re.findall(r'^    case CNetCmd::' + c + r':\n[^\n]*\n\s*break;\n', netapi, re.M)
        if len(m) != 1:
            raise ValueError(f'case {c} found {len(m)} times')
        incs['npl_cases.inc'] += m[0]
except ValueError as e:
    print('extraction failed:', e)
    sys.exit(2)

print('Production text SHA256:', hashlib.sha256(''.join(incs.values()).encode()).hexdigest(), flush=True)

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
    exe = out / 'null_plyr_lookup_test.exe'
    batch = out / 'compile.cmd'
    batch.write_text(
        '@echo off\r\n'
        f'call "{vcvars}" >nul 2>&1\r\n'
        'if errorlevel 1 exit /b 2\r\n'
        f'cl /nologo /EHsc /std:c++17 /W4 /wd4100 /wd4456 /{opt} /I"{out}" "{HERE / "test_null_plyr_lookup.cpp"}" '
        f'/Fo"{out / "null_plyr_lookup.obj"}" /Fe"{exe}"\r\n'
        'exit /b %errorlevel%\r\n', encoding='utf-8')
    print(f'--- /{opt} ---', flush=True)
    if subprocess.run(['cmd', '/c', str(batch)], cwd=out).returncode:
        sys.exit(2)
    if subprocess.run([str(exe)], cwd=out, timeout=60).returncode:
        failed = 1

sys.exit(failed)
