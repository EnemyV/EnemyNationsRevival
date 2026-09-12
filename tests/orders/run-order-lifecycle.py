"""Compile the PRODUCTION order-queue method bodies against a minimal scene.

tests/orders/test_order_dispatch.cpp checks a hand-written MIRROR of the dispatcher
(order_model.h). A mirror cannot catch a defect that lives in the production body it is
mirroring - WinAstra finding 3 is exactly that: the mirror was green while the shipped
OrderComplete returned at its identity guard for a crane loaded mid-build. So this suite
extracts the real bodies verbatim, the way tests/traffic/run-remote-loc-test.py does, and
runs the whole order LIFECYCLE against them: dispatch, work, completion, the next order,
failure, the arrival-give-up hook and the save-mid-construction path.

The scene (test_order_lifecycle.cpp) is a stand-in - a small list, hex, route, building
and game. The METHODS are the shipped ones, byte for byte, and their SHA256 is printed so
a silent drift is visible.

    python tests/orders/run-order-lifecycle.py
    python tests/orders/run-order-lifecycle.py --opt /O2
    python tests/orders/run-order-lifecycle.py --baseline-ref <commit>

--baseline-ref runs the same assertions against an older source: a7e21d09 (the commit
before the identity fix) fails the finding-3 checks that d416d9cd passes.

Exit codes: 0 all pass, 1 a check failed, 2 toolchain / compile error.
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
parser.add_argument('--baseline-ref', help='Read production methods from this Git revision')
parser.add_argument('--opt', default='/Od', help='cl optimisation switch (default /Od)')
args = parser.parse_args()

VEH = 'enations_latest/src/vehicle.cpp'


def read(path):
    if args.baseline_ref:
        return subprocess.check_output(
            ['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + path]).decode('utf-8', 'replace')
    return (ROOT / path).read_text(encoding='utf-8', errors='replace')


source = read(VEH)


def method(signature, src=source, optional=False):
    """Verbatim body of one method, located by its signature and brace-matched."""
    if signature not in src:
        if optional:
            return ''
        raise SystemExit('not found in %s: %s' % (VEH, signature))
    start = src.index(signature)
    opening = src.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (src[end] == '{') - (src[end] == '}')
        end += 1
    header = '\n'.join(l for l in src[start:opening].splitlines() if not l.lstrip().startswith('#'))
    return header + '\n' + src[opening:end] + '\n'


def block(first, last, src=source, fallback=None):
    """Verbatim source between two markers, inclusive of the first line's indentation."""
    if first not in src:
        if fallback is not None:
            return fallback
        raise SystemExit('marker not found in %s: %r' % (VEH, first[:60]))
    start = src.index(first)
    end = src.index(last, start) + len(last)
    return src[start:end]


# ArrivedDest's repair_bldg case (vehmove.cpp) - finding 4's arrival-side transition.
# Brace-matched from the case label so the whole block comes over verbatim, then wrapped
# in a switch so its `break`s are legal.
VEHMOVE = 'enations_latest/src/vehmove.cpp'
move_src = read(VEHMOVE)


def case_block(label, src):
    start = src.index(label)
    opening = src.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (src[end] == '{') - (src[end] == '}')
        end += 1
    return src[start:end]


parts = []

# the probe macro exactly as production defines it (EN_GAMEPLAY_PROBES is 0 in the scene,
# so this exercises the real preprocessor gate rather than a test-local stub)
parts.append(block('#if EN_GAMEPLAY_PROBES\n  #define ORDER_PROBE', '#endif',
                   fallback='#define ORDER_PROBE(w, y)  ((void) 0)'))

# the watchdog's tuning constants
parts += re.findall(r'^static (?:DWORD|int)\s+const ORDER_STALL_\w+\s*=.*?;.*$', source, re.M)

for sig in ('void CVehicle::AddOrder(',
            'void CVehicle::ClearOrders()',
            'BOOL CVehicle::RepairTargetLives(',
            'void CVehicle::ReestablishOrderIdentity()',
            'BOOL CVehicle::ArmedForOrder() const',
            'void CVehicle::OrderArrivalFailed()',
            'void CVehicle::CheckOrderStall()',
            'BOOL CVehicle::NextOrder()',
            'void CVehicle::OrderEnded()',
            'void CVehicle::OrderComplete()',
            'void CVehicle::OrderFailed('):
    # the finding-3 and finding-4 methods do not exist on an older --baseline-ref; the
    # scene supplies an inert stand-in for whichever is missing (see MISSING_* below).
    parts.append(method(sig, optional=sig.split('::')[1].split('(')[0] in
                        ('ReestablishOrderIdentity', 'ArmedForOrder', 'OrderArrivalFailed', 'CheckOrderStall')))

missing = [n for n in ('ReestablishOrderIdentity', 'ArmedForOrder', 'OrderArrivalFailed', 'CheckOrderStall')
           if ('CVehicle::' + n) not in source]

# Operate's two order blocks, lifted verbatim and wrapped as callable methods so the test
# drives the SHIPPED poll order rather than a re-typed one.
parts.append('void CVehicle::TickCompletion() {\n' +
             block('    if (GetOwner()->IsLocal()) {\n        if ((m_iOrderState == order_work)',
                   '            OrderComplete();\n    }') + '\n}\n')
parts.append('void CVehicle::TickIdlePoll() {\n' +
             block('            CheckOrderStall();' if 'CheckOrderStall();' in source
                   else '            if ((m_iOrderState == order_work) && (m_pBldg == NULL))',
                   'if (NextOrder())') + '\n        m_bDispatched = TRUE;\n}\n')

parts.append('void CVehicle::TestRepairArrival() {'+chr(10)+'    switch (m_iEvent) {'+chr(10)+'' +
             case_block('        case repair_bldg : {', move_src) +
             chr(10)+'        default: break;'+chr(10)+'    }'+chr(10)+'}'+chr(10))

actual = '\n'.join(p for p in parts if p)
out = HERE / 'lifecycle-out' / ('baseline' if args.baseline_ref else 'candidate')
out.mkdir(parents=True, exist_ok=True)
(out / 'lifecycle_actual.inc').write_text(actual, encoding='utf-8')
(out / 'lifecycle_missing.inc').write_text(
    '\n'.join('#define MISSING_' + n for n in missing) + '\n', encoding='utf-8')
print('[orders] production methods (vehicle.cpp + vehmove.cpp) SHA256:', hashlib.sha256(actual.encode()).hexdigest(), flush=True)
if missing:
    print('[orders] absent from this source (stand-ins used):', ', '.join(missing), flush=True)

roots = [Path('C:/Program Files/Microsoft Visual Studio/2022') / e for e in
         ('Community', 'Enterprise', 'Professional')]
vs = next((r for r in roots if (r / 'VC/Auxiliary/Build/vcvars64.bat').exists()), None)
if vs is None:
    print('[orders] VS 2022 vcvars64.bat not found')
    sys.exit(2)

batch = out / 'compile.cmd'
exe = out / 'order_lifecycle.exe'
batch.write_text(
    '@echo off\ncall "%s" >nul 2>&1\nif errorlevel 1 exit /b 2\n'
    'cl /nologo /EHsc /std:c++17 /W4 /D_CRT_SECURE_NO_WARNINGS %s /I"%s" "%s" /Fo"%s" /Fe"%s"\n'
    'exit /b %%errorlevel%%\n'
    % (vs / 'VC/Auxiliary/Build/vcvars64.bat', args.opt, out,
       HERE / 'test_order_lifecycle.cpp', out / 'order_lifecycle.obj', exe),
    encoding='utf-8')
if subprocess.run(['cmd', '/c', str(batch)], cwd=str(out)).returncode:
    sys.exit(2)
sys.exit(subprocess.run([str(exe)], cwd=str(out), timeout=30).returncode)
