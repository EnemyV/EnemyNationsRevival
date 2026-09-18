"""Compile the PathWorld snapshot view's cost/range bodies against the production ones.

The snapshot view (ennavview.h CEnSnapNavView) reimplements CGameMap::_GetTerrainCost
and CGameMap::GetRangeDistance over snapshot facts. Both copies are extracted verbatim
here and compiled side by side, so the test fails if either drifts - including if the
snapshot copy quietly "improves" a special case.

It also proves the thing the snapshot exists for: a published snapshot does not follow
a live mutation, and a republished one does.
"""
import argparse
import hashlib
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-ref', help='Read production bodies from this Git revision')
parser.add_argument('--perturb', choices=['coastline', 'samehex', 'altitude', 'unwrapped', 'anchoronly',
                                          'dirtyrow', 'dirtyocc', 'dirtyseam'],
                    help='Corrupt one special case of the SNAPSHOT copy, to run the test in its failing direction')
args = parser.parse_args()

out = HERE / 'pathworld-out' / (args.perturb or 'candidate')
out.mkdir(parents=True, exist_ok=True)


def read(rel):
    if args.baseline_ref:
        return subprocess.check_output(['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + rel]).decode()
    return (ROOT / rel).read_text(encoding='utf-8')


def body(text, signature, start_at=0):
    """The signature line plus its brace-matched body, verbatim (preprocessor lines dropped)."""
    start = text.index(signature, start_at)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    head = '\n'.join(line for line in text[start:opening].splitlines()
                     if not line.lstrip().startswith('#'))
    return head + '\n' + text[opening:end]


def block(text, anchor, start_at=0):
    """A brace-matched { ... } block that begins at `anchor`, verbatim."""
    start = text.index(anchor, start_at)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


terrain_cpp = read('enations_latest/src/terrain.cpp')
terrain_h = read('enations_latest/src/terrain.h')
terrain_inl = read('enations_latest/src/terrain.inl')
base_h = read('enations_latest/src/base.h')
navview_h = read('enations_latest/src/ennavview.h')
pathworld_h = read('enations_latest/src/pathworld.h')
pathworld_cpp = read('enations_latest/src/pathworld.cpp')

# --- the live bodies, verbatim ------------------------------------------------
live = body(terrain_cpp, 'int CGameMap::_GetTerrainCost( CHex const* pHex, CHex const* pHexDest, int iDir, int iWheel )')
live += '\n\n' + body(terrain_cpp, 'int CGameMap::GetRangeDistance( CHexCoord& hex1, CHexCoord& hex2 )')
(out / 'pw_livecost.inc').write_text(live, encoding='utf-8')

# --- the snapshot bodies, verbatim, from inside class CEnSnapNavView -----------
snap_at = navview_h.index('class CEnSnapNavView')
snap = body(navview_h,
            'int GetTerrainCost( CHexCoord const& hexFrom, CHexCoord const& hexTo, int iDir, int iWheel ) const',
            snap_at)
snap += '\n\n' + body(navview_h, 'int GetRangeDistance( CHexCoord const& hex1, CHexCoord const& hex2 ) const', snap_at)

if args.perturb == 'coastline':
    snap = snap.replace('m_pw.WheelMult( CHex::ocean, iWheel ) * 8', 'm_pw.WheelMult( CHex::ocean, iWheel )')
elif args.perturb == 'samehex':
    snap = snap.replace('            iDir  = 0;\n', '')
elif args.perturb == 'altitude':
    snap = snap.replace('( iRtn * iAlt ) / 16', '( iRtn * iAlt ) / 8')
(out / 'pw_snapcost.inc').write_text(snap, encoding='utf-8')

# --- the shared constants, verbatim -------------------------------------------
wheels = re.search(r'^const int NUM_WHEEL_TYPES\s*=.*?;', base_h, re.M).group(0)
wheels += '\n' + block(base_h, 'class CWheelTypes') + ';\n'
(out / 'pw_wheels.inc').write_text(wheels, encoding='utf-8')

hexenums = block(terrain_h, 'enum {\t\t\t// terrain types') + ';\n'
hexenums += block(terrain_h, 'enum { ul=0x01') + ';\n'
(out / 'pw_hexenums.inc').write_text(hexenums, encoding='utf-8')

structs = block(pathworld_h, 'struct Hex') + ';\n' + block(pathworld_h, 'struct Bldg') + ';\n'
(out / 'pw_structs.inc').write_text(structs, encoding='utf-8')

# --- the two hex INDEXERS, verbatim: the snapshot's and the live one it mirrors ----
# PathWorld::At must land on the same hex CGameMap::GetHex(int,int) lands on, for any
# coordinate the search can form - including one outside [0, side), which is what a
# search crossing the map's wrap seam produces.
snap_index = body(pathworld_h, 'Hex const& At( int x, int y ) const')
if args.perturb == 'unwrapped':
    # The pre-fix form: index straight off the caller's coordinates.
    snap_index = 'Hex const& At( int x, int y ) const\n' \
                 '{ return ( m_aHex[( (size_t)y << m_iSideShift ) + (size_t)x] ); }'
(out / 'pw_snapindex.inc').write_text(snap_index, encoding='utf-8')

live_index = body(terrain_inl, 'inline CHex const* CGameMap::GetHex( int x, int y ) const')
live_index += '\n\n' + body(terrain_inl, 'inline CHex const* CGameMap::_GetHex( int x, int y ) const')
live_index += '\n\n' + body(terrain_inl, 'inline int CHexCoord::Wrap( int iVal )')
(out / 'pw_livehex.inc').write_text(live_index, encoding='utf-8')

# --- the open-addressed hex index, verbatim ---------------------------------------
pwindex = block(pathworld_h, 'class CPwIndex') + ';\n\n'
pwindex += body(pathworld_cpp, 'void CPwIndex::Reserve( size_t nEntries )') + '\n\n'
pwindex += body(pathworld_cpp, 'void CPwIndex::Insert( DWORD dwKey, DWORD dwVal )') + '\n\n'
pwindex += body(pathworld_cpp, 'DWORD CPwIndex::Find( DWORD dwKey ) const')
(out / 'pw_pwindex.inc').write_text(pwindex, encoding='utf-8')

# --- the INCREMENTAL rebuild, verbatim --------------------------------------------
# An incremental build copies the previous snapshot's hex array and re-encodes only the
# rows the dirty set names. It has to come out byte-identical to a full rebuild of the
# same live map, so the dirty tracker and the fill are extracted and driven side by side
# against a from-scratch fill in the fixture.
dirty = block(pathworld_h, 'class PwDirty') + ';\n'
if args.perturb == 'dirtyseam':
    # A tracker that silently drops row 0 - the y-wrap seam row - instead of recording it.
    dirty = dirty.replace('    void Row( int y )\n    {\n        if ( m_bAll )',
                          '    void Row( int y )\n    {\n        if ( y == 0 )\n            return;\n'
                          '        if ( m_bAll )')
    assert 'if ( y == 0 )' in dirty, 'dirtyseam perturbation did not apply'
(out / 'pw_dirty.inc').write_text(dirty, encoding='utf-8')

fill = body(pathworld_cpp, 'static void PwEncodeRow( PathWorld::Hex* pOut, CHex const* pRow, int iWidth )')
fill += '\n\n' + body(pathworld_cpp, 'static int PwFillHexes(')
if args.perturb == 'dirtyrow':
    fill = fill.replace('if ( dirty.IsRow( y ) )', 'if ( false )')
    assert 'if ( false )' in fill, 'dirtyrow perturbation did not apply'
elif args.perturb == 'dirtyocc':
    before = fill
    fill = re.sub(r'\n *for \( size_t i = 0; i < nPrevOcc; \+\+i \)[^\n]*\n', '\n', fill)
    assert fill != before, 'dirtyocc perturbation did not apply'
(out / 'pw_fill.inc').write_text(fill, encoding='utf-8')

digest = hashlib.sha256((live + snap + wheels + hexenums + structs + snap_index + live_index
                         + pwindex + dirty + fill).encode()).hexdigest()
print('Production bodies SHA256:', digest, flush=True)
if args.perturb:
    print('PERTURBED:', args.perturb, '- this run is EXPECTED to fail', flush=True)

vs = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
batch = out / 'compile.cmd'
exe = out / 'pathworld_snapshot_test.exe'
extra = ' /DPW_PERTURB_ANCHORONLY' if args.perturb == 'anchoronly' else ''
batch.write_text(f'@echo off\ncall "{vs}" >nul 2>&1\nif errorlevel 1 exit /b 2\n'
                 f'cl /nologo /EHsc /std:c++17 /W4 /wd4100 /wd4127 /Od{extra} /I"{out}" '
                 f'"{HERE / "test_pathworld_snapshot.cpp"}" '
                 f'/Fo"{out / "pathworld_snapshot.obj"}" /Fe"{exe}"\nexit /b %errorlevel%\n', encoding='utf-8')
compiled = subprocess.run(['cmd', '/c', str(batch)], cwd=out)
if compiled.returncode:
    sys.exit(2)
rc = subprocess.run([str(exe)], cwd=out, timeout=60).returncode
if args.perturb:
    if rc == 0:
        print('THE PERTURBED RUN PASSED - the test does not actually check that case')
        sys.exit(1)
    print('perturbed run failed as required')
    sys.exit(0)
sys.exit(rc)
