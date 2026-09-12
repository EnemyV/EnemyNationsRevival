"""Compile the shipped Scrounging / Slash-and-Burn bodies against a minimal scene.

Checks that CFarmBuilding::ApplySlash invalidates the warehouse terrain cache
(CWarehouseBuilding::m_iScr*) so the next production tick re-scans the cut ground,
and that a cut outside a warehouse's rings leaves that warehouse untouched.

The production function bodies are extracted VERBATIM from new_unit.cpp and
altoutput.cpp (the tests/traffic extraction pattern) -- nothing here mirrors them.
Only the world around them (CHex, theMap, theTerrain, theStructures,
theBuildingMap, the building classes) is replaced by a small fake scene, because
the real ones drag in the whole game/window/net layer.

Self-contained: cl.exe is invoked directly, nothing is written into the source
tree or any game build dir, and the game is never launched.  Artifacts go to
d:\\tmp\\scroungecache\\<opt>\\ (override with --out-dir).

Built and run twice, at /Od and at /O2: the cache is a set of lazily filled
sentinel fields, and that is exactly the shape an optimiser can reorder.

Exit codes: 0 all pass, 1 a check failed, 2 toolchain / compile error.
"""
import argparse
import hashlib
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root', type=Path, default=ROOT)
parser.add_argument('--out-dir', type=Path, default=Path('d:/tmp/scroungecache'))
parser.add_argument('--baseline-ref',
                    help='Extract the production bodies from this Git revision instead of the '
                         'working tree (639610a9 is the pre-fix integration tip and FAILS).')
args = parser.parse_args()


def read_source(name):
    rel = 'enations_latest/src/' + name
    if args.baseline_ref:
        return subprocess.check_output(
            ['git', '-C', str(args.source_root), 'show', args.baseline_ref + ':' + rel]).decode('utf-8')
    return (args.source_root / rel).read_text(encoding='utf-8')


unit = read_source('new_unit.cpp')
alt = read_source('altoutput.cpp')


def body(signature, source):
    """The whole definition from its signature to the closing brace, verbatim."""
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    text = source[start:end]
    if signature.startswith('struct'):
        text += ';'   # body() stops at the closing brace; a struct definition needs its semicolon
    return text + '\n'


# The scan weights, the ring geometry, the cache, the invalidation and the hex retype.
NAMES = [
    'struct ScrapAccum',
    'static int fnFarmFromGround(',
    'static int fnLumberFromGround(',
    'static int fnScrapFromGround(',
    'static int fnScroungeSoilFromGround(',
    'static void LumberBox(',
    'int CFarmBuilding::ForestMultAt(',
    'int CFarmBuilding::SoilMultAt(',
    'int CFarmBuilding::ScroungeSoilMultAt(',
    'void CFarmBuilding::ScrapMultsAt(',
    'int CFarmBuilding::LandMult(',
    'void CFarmBuilding::UpdateFarm(',
    'void CWarehouseBuilding::UpdateScrounge(',
    'void CWarehouseBuilding::InvalidateScrounge(',
    'int CWarehouseBuilding::GetScroungeForestMult(',
    'int CWarehouseBuilding::GetScroungeIronMult(',
    'int CWarehouseBuilding::GetScroungeCoalMult(',
    'int CWarehouseBuilding::GetScroungeSoilMult(',
    'static BOOL SlashHex(',
    'BOOL CFarmBuilding::ApplySlash(',
]

parts = []
missing = []
for name in NAMES:
    try:
        parts.append(body(name, unit))
    except ValueError:
        missing.append(name)
if missing:
    print('NOT FOUND in the extracted source: ' + ', '.join(missing), flush=True)
    if not args.baseline_ref:
        sys.exit(2)
    # Pre-fix revisions have no InvalidateScrounge at all, and their ApplySlash never calls one.
    # Give the fixture an empty definition purely so the baseline LINKS: the point of a baseline
    # run is to watch the cache assertions fail, not to watch the compiler fail.
    if missing == ['void CWarehouseBuilding::InvalidateScrounge(']:
        parts.append('void CWarehouseBuilding::InvalidateScrounge( ) { }\n')
    else:
        sys.exit(2)

# MultiLinesFor is the consumer: it turns the four cached mults into the actual per-minute rates.
parts.append('namespace AltOutput {\n' + body('int MultiLinesFor(', alt) + '}\n')

actual = '\n'.join(parts)
digest = hashlib.sha256(actual.encode()).hexdigest()
print('Production bodies SHA256:', digest, flush=True)

vs_roots = [
    Path('C:/Program Files/Microsoft Visual Studio/2022/Community'),
    Path('C:/Program Files/Microsoft Visual Studio/2022/Enterprise'),
    Path('C:/Program Files/Microsoft Visual Studio/2022/Professional'),
]
vcvars = next((r / 'VC/Auxiliary/Build/vcvars64.bat' for r in vs_roots
               if (r / 'VC/Auxiliary/Build/vcvars64.bat').exists()), None)
if vcvars is None:
    print('VS 2022 vcvars64.bat not found.', file=sys.stderr)
    sys.exit(2)

failed = 0
for opt in ('Od', 'O2'):
    out = args.out_dir / ('baseline-' if args.baseline_ref else '') / opt
    out.mkdir(parents=True, exist_ok=True)
    (out / 'scrounge_cache_actual.inc').write_text(actual, encoding='utf-8')
    exe = out / 'scrounge_cache_test.exe'
    batch = out / 'compile.cmd'
    batch.write_text(
        '@echo off\r\n'
        f'call "{vcvars}" >nul 2>&1\r\n'
        'if errorlevel 1 exit /b 2\r\n'
        f'cl /nologo /EHsc /std:c++17 /W4 /{opt} /I"{out}" "{HERE / "test_scrounge_cache.cpp"}" '
        f'/Fo"{out / "scrounge_cache.obj"}" /Fe"{exe}"\r\n'
        'exit /b %errorlevel%\r\n', encoding='utf-8')
    print(f'--- /{opt} ---', flush=True)
    if subprocess.run(['cmd', '/c', str(batch)], cwd=out).returncode:
        sys.exit(2)
    if subprocess.run([str(exe)], cwd=out, timeout=60).returncode:
        failed = 1

sys.exit(failed)
