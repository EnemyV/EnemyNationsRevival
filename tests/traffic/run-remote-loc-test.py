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
args = parser.parse_args()
suite = 'parking' if args.parking else ('clearance' if args.clearance else 'locations')
out = HERE / 'remote-loc-out' / suite / ('baseline' if args.baseline_ref else 'candidate')
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


if args.parking:
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
batch.write_text(f'@echo off\ncall "{vs}" >nul 2>&1\nif errorlevel 1 exit /b 2\n'
                 f'cl /nologo /EHsc /std:c++17 /W4 /I"{out}" "{HERE / case}" '
                 f'/Fo"{out / "remote_loc.obj"}" /Fe"{exe}"\nexit /b %errorlevel%\n', encoding='utf-8')
compiled = subprocess.run(['cmd', '/c', str(batch)], cwd=out)
if compiled.returncode:
    sys.exit(2)
sys.exit(subprocess.run([str(exe)], cwd=out, timeout=20).returncode)
