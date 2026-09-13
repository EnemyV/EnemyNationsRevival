"""Compile the PRODUCTION drag-place bodies (#38 step 6) against a minimal scene.

Same technique as run-order-lifecycle.py: the bodies come out of
enations_latest/src/area.cpp VERBATIM, so a check in test_drag_place.cpp is a statement
about the shipped code rather than about a mirror of it. Their SHA256 is printed, so a
silent drift is visible.

What is lifted:

    struct CBuildVerdict              the shared verdict's result type
    BuildSiteVerdict()                THE per-site verdict (the hover's own rules)
    BuildDragLine()                   the line generator
    HasMoveStops()                    the one-list predicate
    CWndArea::ToBuildUL()             the footprint anchor
    CWndArea::UpdateBuildDrag()       lay the line, judge every site
    CWndArea::EndBuildDrag()
    OnLButtonUp's drag-commit block -> wrapped as CWndArea::CommitDragTest()

    python tests/orders/run-drag-place.py
    python tests/orders/run-drag-place.py --opt /O2
    python tests/orders/run-drag-place.py --baseline-ref <commit>

--baseline-ref reads the production source from an older revision: 69b30f39 (the
integration tip this work is based on) has none of these bodies, so the extraction fails
loudly there - that is the positive control for "these tests are reading the real source".

Exit codes: 0 all pass, 1 a check failed, 2 toolchain / compile / extraction error.
"""
import argparse
import hashlib
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-ref', help='Read the production bodies from this Git revision')
parser.add_argument('--opt', default='/Od', help='cl optimisation switch (default /Od)')
args = parser.parse_args()

AREA = 'enations_latest/src/area.cpp'
AREA_H = 'enations_latest/src/area.h'


def read(path):
    if args.baseline_ref:
        return subprocess.check_output(
            ['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + path]).decode('utf-8', 'replace')
    return (ROOT / path).read_text(encoding='utf-8', errors='replace')


source = read(AREA)


def brace_match(src, start):
    """End offset (exclusive) of the braced block that opens after `start`."""
    opening = src.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (src[end] == '{') - (src[end] == '}')
        end += 1
    return opening, end


def body(signature, src=source, trailing=''):
    """Verbatim text of one function / struct, located by signature and brace-matched."""
    if signature not in src:
        print('[orders] NOT FOUND in %s: %s' % (AREA, signature))
        sys.exit(2)
    start = src.index(signature)
    opening, end = brace_match(src, start)
    header = '\n'.join(l for l in src[start:opening].splitlines() if not l.lstrip().startswith('#'))
    return header + '\n' + src[opening:end] + trailing + '\n'


def block(first, last, src=source):
    """Verbatim source from `first` through the end of `last`."""
    if first not in src:
        print('[orders] marker NOT FOUND in %s: %r' % (AREA, first[:70]))
        sys.exit(2)
    start = src.index(first)
    end = src.index(last, start) + len(last)
    return src[start:end]


parts = []

# the verdict type and the two pure functions
parts.append(body('struct CBuildVerdict', trailing=';'))
parts.append(body('static void BuildSiteVerdict('))
parts.append(body('static int BuildDragLine('))
parts.append(body('static BOOL HasMoveStops('))

# the window's own halves
parts.append(body('CHexCoord CWndArea::ToBuildUL('))
parts.append(body('void CWndArea::UpdateBuildDrag('))
parts.append(body('void CWndArea::EndBuildDrag('))

# OnLButtonUp's commit block, wrapped so the test can call it. The lifted text is the
# whole `if ( bDragBuild ) { ... NextOrder(); ` run; the closing brace is added back here.
commit = block('        if ( bDragBuild )\n        {\n            if ( HasMoveStops( pVehBuild ) )',
               '            pVehBuild->NextOrder( );')
parts.append('void CWndArea::CommitDragTest( )\n{\n' + commit + '\n    }\n}\n')

actual = '\n'.join(parts)
out = HERE / 'dragplace-out' / ('baseline' if args.baseline_ref else 'candidate')
out.mkdir(parents=True, exist_ok=True)
(out / 'dragplace_actual.inc').write_text(actual, encoding='utf-8')
print('[orders] production drag-place bodies (area.cpp) SHA256:',
      hashlib.sha256(actual.encode()).hexdigest(), flush=True)

roots = [Path('C:/Program Files/Microsoft Visual Studio/2022') / e for e in
         ('Community', 'Enterprise', 'Professional')]
vs = next((r for r in roots if (r / 'VC/Auxiliary/Build/vcvars64.bat').exists()), None)
if vs is None:
    print('[orders] VS 2022 vcvars64.bat not found')
    sys.exit(2)

batch = out / 'compile.cmd'
exe = out / 'drag_place.exe'
batch.write_text(
    '@echo off\ncall "%s" >nul 2>&1\nif errorlevel 1 exit /b 2\n'
    'cl /nologo /EHsc /std:c++17 /W4 /D_CRT_SECURE_NO_WARNINGS %s /I"%s" "%s" /Fo"%s" /Fe"%s"\n'
    'exit /b %%errorlevel%%\n'
    % (vs / 'VC/Auxiliary/Build/vcvars64.bat', args.opt, out,
       HERE / 'test_drag_place.cpp', out / 'drag_place.obj', exe),
    encoding='utf-8')
if subprocess.run(['cmd', '/c', str(batch)], cwd=str(out)).returncode:
    sys.exit(2)

# the lint reads the WORKING TREE source (it is about the wiring that surrounds the
# extracted bodies), so it is skipped on a --baseline-ref run
argv = [str(exe)]
if not args.baseline_ref:
    argv += [str(ROOT / AREA), str(ROOT / AREA_H)]
sys.exit(subprocess.run(argv, cwd=str(out), timeout=60).returncode)
