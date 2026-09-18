"""Compile PathService's production bodies against a counting scaffold.

What this tests is the PLUMBING step C adds - the queue, the threads, the ownership
transfer, the generation fields, the start/stop/quiesce lifecycle - not the A* search,
which has no business running inside a unit test and whose snapshot-vs-live equivalence
is Phase 1's gate (run-pathworld-snapshot.py) and the live [worker] DIFF counter.

So the scaffold's CPathMgr::SearchSnapshot is a small deterministic walk over the
snapshot, called by the workers AND by the main-thread reference. Every result must
equal the reference for its own request: that is the statement "the service routed the
right answer to the right request, intact, under concurrency".

Production text used verbatim: PathRequest, PathResult and class PathService from
pathservice.h; every PathService method body, ResolveOnOff, Enabled and
ConfiguredWorkers from pathservice.cpp; CPathMgr::SNAPSEARCH from cpathmgr.h;
PathWorld::Hex from pathworld.h; CTransport::GetData and CTransport::GetIndex from
vehicle.inl / new_unit.cpp (the iVehType proof).
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
parser.add_argument('--perturb', choices=['result', 'noqueuedfree', 'vehtype'],
                    help='Corrupt one thing, to run the test in its failing direction')
args = parser.parse_args()

out = HERE / 'pathservice-out' / (args.perturb or 'candidate')
out.mkdir(parents=True, exist_ok=True)


def read(rel):
    if args.baseline_ref:
        return subprocess.check_output(['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + rel]).decode()
    return (ROOT / rel).read_text(encoding='utf-8')


def body(text, signature, start_at=0):
    """The signature line plus its brace-matched body, verbatim (preprocessor lines dropped from the head)."""
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


svc_h = read('enations_latest/src/pathservice.h')
svc_cpp = read('enations_latest/src/pathservice.cpp')
pathmgr_h = read('enations_latest/src/cpathmgr.h')
pathworld_h = read('enations_latest/src/pathworld.h')
vehicle_inl = read('enations_latest/src/vehicle.inl')
new_unit_cpp = read('enations_latest/src/new_unit.cpp')

# --- the record the snapshot search reads, and the search's out-parameter -------
shared = block(pathworld_h, 'struct Hex') + ';\n\n'
shared += block(pathmgr_h, 'struct SNAPSEARCH') + ';\n'
(out / 'ps_shared.inc').write_text(shared, encoding='utf-8')

# --- the request/result contract and the class, verbatim -----------------------
decls = block(svc_h, 'struct PathRequest') + ';\n\n'
decls += block(svc_h, 'struct PathResult') + ';\n\n'
decls += block(svc_h, 'class PathService') + ';\n'
(out / 'ps_decls.inc').write_text(decls, encoding='utf-8')

# --- every method body, verbatim ----------------------------------------------
bodies = '\n\n'.join(body(svc_cpp, s) for s in (
    'static int ResolveOnOff( const char* pszEnv )',
    'BOOL PathService::Enabled( void )',
    'int PathService::ConfiguredWorkers( void )',
    'PathService::PathService( )',
    'PathService::~PathService( )',
    'void PathService::FreeResult( PathResult& r )',
    'BOOL PathService::Start( int iMapEX, int iMapEY, int nWorkers )',
    'void PathService::Stop( void )',
    'uint64_t PathService::Submit( PathRequest const& req )',
    'BOOL PathService::PopResult( PathResult& out )',
    'int PathService::QueueDepth( void ) const',
    'void PathService::Quiesce( void )',
    'void PathService::Resume( void )',
    'void PathService::WorkerMain( void )',
))

if args.perturb == 'noqueuedfree':
    # Stop stops freeing the results nobody popped. The leak counters must see it.
    assert 'FreeResult( m_qRes.front( ) );' in bodies
    bodies = bodies.replace('FreeResult( m_qRes.front( ) );', '/* leaked on purpose */', 1)
(out / 'ps_bodies.inc').write_text(bodies, encoding='utf-8')

# --- the iVehType proof: the two production accessors, verbatim ----------------
# A worker gets no CVehicle, so the request must carry the integer that makes
# theTransports.GetData( iVehType ) return the SAME CTransportData* that
# pVehicle->GetData() returns. That integer is GetIndex(), not GetType().
veh = body(vehicle_inl, 'inline CTransportData const * CTransport::GetData (int iIndex) const') + '\n\n'
veh += body(new_unit_cpp, 'int CTransport::GetIndex( CTransportData const* pData ) const')
if args.perturb == 'vehtype':
    # Exactly the mistake the round trip exists to prevent: hand back the unit's
    # TRANS_TYPE where its slot in theTransports was wanted.
    veh = veh.replace('            return ( iInd );', '            return ( ( m_pData + iInd )->m_iType );')
(out / 'ps_vehtype.inc').write_text(veh, encoding='utf-8')

digest = hashlib.sha256((shared + decls + bodies + veh).encode()).hexdigest()
print('Production bodies SHA256:', digest, flush=True)
if args.perturb:
    print('PERTURBED:', args.perturb, '- this run is EXPECTED to fail', flush=True)

vs = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
batch = out / 'compile.cmd'
exe = out / 'pathservice_test.exe'
extra = ' /DPS_PERTURB_RESULT' if args.perturb == 'result' else ''
# C4100: several production bodies take arguments the scaffold's search ignores.
# C4459/C4456: production locals named like the fixture's globals.
batch.write_text(f'@echo off\ncall "{vs}" >nul 2>&1\nif errorlevel 1 exit /b 2\n'
                 f'cl /nologo /EHsc /std:c++17 /W4 /wd4100 /wd4127 /wd4456 /wd4459 /MD /Od{extra} /I"{out}" '
                 f'"{HERE / "test_pathservice.cpp"}" '
                 f'/Fo"{out / "pathservice.obj"}" /Fe"{exe}"\nexit /b %errorlevel%\n', encoding='utf-8')
compiled = subprocess.run(['cmd', '/c', str(batch)], cwd=out)
if compiled.returncode:
    sys.exit(2)
rc = subprocess.run([str(exe)], cwd=out, timeout=300).returncode
if args.perturb:
    if rc == 0:
        print('THE PERTURBED RUN PASSED - the test does not actually check that case')
        sys.exit(1)
    print('perturbed run failed as required')
    sys.exit(0)
sys.exit(rc)
