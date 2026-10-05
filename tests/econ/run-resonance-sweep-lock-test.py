"""Source guard: the Resonance Sweep call in GraphicsEnginePump must run under `cs`.

ResonanceSweep walks theBuildingMap and can delete a dead rocket (and, through
SweepLight -> EnHexBecameVisible, other dead buildings). AI worker threads walk the
same map under the global CRITICAL_SECTION `cs`, so the main-thread call must hold
it. The check strips comments, extracts GraphicsEnginePump, and requires the call to
sit after an EnterCriticalSection( &cs ) with no LeaveCriticalSection( &cs ) between
them, inside the Enter's block, and to be followed by a Leave in that same block.
Exit codes: 0 pass, 1 check failed.
"""
import argparse
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-ref', help='read mainloop.cpp from this Git revision')
args = parser.parse_args()

REL = 'enations_latest/src/mainloop.cpp'
if args.baseline_ref:
    text = subprocess.check_output(['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + REL]).decode('utf-8')
else:
    text = (ROOT / REL).read_text(encoding='utf-8')


def strip_comments(src):
    # Blank comments and string/char literals with spaces so offsets and braces stay honest.
    def blank(m):
        return re.sub(r'[^\n]', ' ', m.group(0))
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', blank, src, flags=re.S)


def fail(msg):
    print('FAIL:', msg)
    sys.exit(1)


code = strip_comments(text)
sig = code.index('void CConquerApp::GraphicsEnginePump( )')
opening = code.index('{', sig)
depth, i = 1, opening + 1
depths = [0] * len(code)
while depth:
    c = code[i]
    if c == '{':
        depth += 1
    elif c == '}':
        depth -= 1
    depths[i] = depth
    i += 1
body_start, body_end = opening + 1, i
body = code[body_start:body_end]


def line_of(pos):
    return text.count('\n', 0, pos) + 1


calls = [m.start() + body_start for m in re.finditer(r'ResonanceSweep\s*\(', body)]
if len(calls) != 1:
    fail(f'expected exactly one ResonanceSweep call in GraphicsEnginePump, found {len(calls)}')
call = calls[0]
enters = [m.start() + body_start for m in re.finditer(r'EnterCriticalSection\s*\(\s*&\s*cs\s*\)', body)]
leaves = [m.start() + body_start for m in re.finditer(r'LeaveCriticalSection\s*\(\s*&\s*cs\s*\)', body)]
print(f'ResonanceSweep call at mainloop.cpp:{line_of(call)}')


def on_path(a, b, pos):
    """True when the statement at pos sits in a block that is still open at the other end."""
    lo, hi = (pos, b) if pos < b else (a, pos)
    return min(depths[lo:hi + 1]) >= depths[pos]


# Only Enter/Leave statements whose block encloses the call are on the straight-line
# path to it. Ones in nested branches (Leave + return, Leave ... re-Enter) are skipped.
path_enters = [e for e in enters if e < call and on_path(e, call, e)]
if not path_enters:
    fail('no EnterCriticalSection( &cs ) on the path to the ResonanceSweep call in GraphicsEnginePump')
enter = path_enters[-1]
d_enter = depths[enter]
released = [l for l in leaves if enter < l < call and on_path(l, call, l)]
if released:
    fail(f'cs is released at :{line_of(released[0])} after Enter :{line_of(enter)}, before the call')
path_leaves = [l for l in leaves if l > call and depths[l] <= depths[call]
               and on_path(call, l, l) and depths[l] >= d_enter]
if not path_leaves:
    fail(f'no LeaveCriticalSection( &cs ) after the call in the block of Enter :{line_of(enter)}')
leave = path_leaves[0]
print(f'PASS: call is held under cs (Enter :{line_of(enter)}, Leave :{line_of(leave)})')
