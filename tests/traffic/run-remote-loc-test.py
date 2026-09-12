"""Compile the production location/apply methods against a minimal scene.

Tests the receiving contract and local-facing control cases, not the network
transport or a two-peer game. Production method bodies are extracted verbatim.
"""
import argparse
import hashlib
import re
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-ref', help='Read production methods from this Git revision')
parser.add_argument('--clearance', action='store_true', help='Check eligibility, touching propagation and expiry instead')
parser.add_argument('--parking', action='store_true', help='Check bounded parking, failed-request cooldown, lane authority and fleeing')
parser.add_argument('--junction', action='store_true', help='Check road-turn lane discipline at an open paved junction')
parser.add_argument('--o2', action='store_true', help='Compile the fixture optimised (/O2) instead of /Od')
args = parser.parse_args()
suite = ('junction' if args.junction else
         'parking' if args.parking else ('clearance' if args.clearance else 'locations'))
out = (HERE / 'remote-loc-out' / suite /
       (('baseline' if args.baseline_ref else 'candidate') + ('-o2' if args.o2 else '')))
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


if args.junction:
    def other(rel):
        return (subprocess.check_output(['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + rel]).decode()
                if args.baseline_ref else (ROOT / rel).read_text(encoding='utf-8'))
    veh = other('enations_latest/src/vehicle.cpp')
    inl = other('enations_latest/src/vehicle.inl')
    unit = other('enations_latest/src/unit.cpp')
    actual = re.search(r'^int aiBaseDir\[9\][^;]*;', veh, re.M).group(0)
    actual += '\n' + method('_inline int GetDirIndex ', inl)
    actual += '\n' + method('_inline int GetAngle ', inl)
    actual += '\n' + method('CSubHex Rotate( int iDir,', unit)
    actual += '\n' + method('BOOL CVehicle::GetNextHex(')
    case, include = 'test_junction.cpp', 'junction_actual.inc'
elif args.parking:
    actual = '\n'.join(method(s) for s in (
        'BOOL CVehicle::FindOffRoadSpot(', 'BOOL CVehicle::AskToMove(', 'BOOL CVehicle::MustKeepLane('))
    path = 'enations_latest/src/netapi.cpp'
    net = (subprocess.check_output(['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + path]).decode()
           if args.baseline_ref else (ROOT / path).read_text(encoding='utf-8'))
    guard = method('if ( !pVeh->GetData()->IsBoat() )', net)
    actual += '\nvoid ApplyFleeTarget(CVehicle* pVeh,CSubHex _dest,BOOL& accepted) {\n'
    actual += 'CVehicle* pAttacker=pVeh; CVehicle* pTarget=pVeh;\n' + guard + '\naccepted=TRUE;\n}\n'
    actual += 'bool TestFleeTarget(CVehicle* v,CSubHex p) { BOOL accepted=FALSE; ApplyFleeTarget(v,p,accepted); return accepted!=FALSE; }\n'
    path = 'enations_latest/src/vehicle.cpp'
    operate = (subprocess.check_output(['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + path]).decode()
               if args.baseline_ref else (ROOT / path).read_text(encoding='utf-8'))
    hold = method('if ((m_iHoldFrames > 0) && (m_cMode == stop)', operate)
    start = operate.index('    if ((m_unitFlags & (dying | stopped))')
    end = operate.index('return;', start) + len('return;')
    actual += '\nvoid CVehicle::TickHold() {\n' + operate[start:end] + '\n' + hold + '\n}\n'
    arrival = source[source.index('void CVehicle::ArrivedDest()'):]
    start = arrival.index('    // Saved recovery metadata') if '    // Saved recovery metadata' in arrival else arrival.index('    // Keep reverse geometry')
    end = arrival.index("    // we're stopped", start)
    actual += '\nvoid CVehicle::TestArrivalRecovery() {\n' + arrival[start:end] + '\n}\n'
    backup = source[source.index('BOOL CVehicle::BackUp()'):]
    start = backup.index('BOOL bFixed = ') + len('BOOL bFixed = ')
    end = backup.index(';', start)
    actual += '\nBOOL CVehicle::TestFixedBlocker(CVehicle* pIn, BOOL bAgainstLane) { return ' + backup[start:end] + '; }\n'
    case, include = 'test_parking.cpp', 'parking_actual.inc'
elif args.clearance:
    header_path = 'enations_latest/src/vehicle.h'
    header = (subprocess.check_output(['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + header_path]).decode()
              if args.baseline_ref else (ROOT / header_path).read_text(encoding='utf-8'))
    actual = '\n'.join(re.findall(r'^const int JAM_\w+\s*=.*?;', header, re.M))
    actual += '\n' + '\n'.join(method(s) for s in (
        'BOOL CVehicle::JamEligible() const', 'void CVehicle::JamForward()', 'void CVehicle::JamWatch()'))
    case, include = 'test_clearance.cpp', 'clearance_actual.inc'
else:
    actual = method('void CVehicle::SetLoc(BOOL)') + '\n' + method('void CVehicle::SetFromMsg(')
    path_source = (subprocess.check_output(['git', '-C', str(ROOT), 'show',
                                          args.baseline_ref + ':enations_latest/src/cpathmgr.cpp']).decode()
                   if args.baseline_ref else (ROOT / 'enations_latest/src/cpathmgr.cpp').read_text(encoding='utf-8'))
    actual += '\n' + method('BOOL CPathMgr::IsHexMovingVehicle(', path_source)
    case, include = 'test_remote_loc.cpp', 'remote_loc_actual.inc'
(out / include).write_text(actual, encoding='utf-8')
print('Production methods SHA256:', hashlib.sha256(actual.encode()).hexdigest(), flush=True)
vs = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
batch = out / 'compile.cmd'
exe = out / 'remote_loc_test.exe'
opt = '/O2' if args.o2 else '/Od'
# The junction suite compiles a whole shipped function rather than a small method,
# so it carries that function's own /W4 noise: C4456/C4457 (the production body
# redeclares xDif/yDif in inner scopes) and C4100 (an argument only a debug build
# uses). Suppressed here, never in the game build.
noise = ' /wd4456 /wd4457 /wd4100' if args.junction else ''
batch.write_text(f'@echo off\ncall "{vs}" >nul 2>&1\nif errorlevel 1 exit /b 2\n'
                 f'cl /nologo /EHsc /std:c++17 /W4 {opt}{noise} /I"{out}" "{HERE / case}" '
                 f'/Fo"{out / "remote_loc.obj"}" /Fe"{exe}"\nexit /b %errorlevel%\n', encoding='utf-8')
compiled = subprocess.run(['cmd', '/c', str(batch)], cwd=out)
if compiled.returncode:
    sys.exit(2)
sys.exit(subprocess.run([str(exe)], cwd=out, timeout=20).returncode)
