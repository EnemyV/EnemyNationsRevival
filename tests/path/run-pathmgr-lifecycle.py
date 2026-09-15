"""Compile CPathMgr's production lifecycle bodies against a counting scaffold.

Tests the critical-section lifetime, not the search: construct / Init / search /
Close / Init / search / destruct, on one instance and on two at once. The ctors,
Init, Close, ClearArray and the destructor are extracted verbatim from
cpathmgr.cpp, and class CCell plus MAX_BOTH_INDEX from cpathmgr.h, so the test
fails if the production text stops being balanced - it cannot pass against a copy
that has drifted.
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
parser.add_argument('--o2', action='store_true', help='Compile the fixture optimised (/O2) instead of /Od')
args = parser.parse_args()

out = HERE / 'pathmgr-out' / (('baseline' if args.baseline_ref else 'candidate') + ('-o2' if args.o2 else ''))
out.mkdir(parents=True, exist_ok=True)


def read(rel):
    if args.baseline_ref:
        return subprocess.check_output(['git', '-C', str(ROOT), 'show', args.baseline_ref + ':' + rel]).decode()
    return (ROOT / rel).read_text(encoding='utf-8')


source = read('enations_latest/src/cpathmgr.cpp')
header = read('enations_latest/src/cpathmgr.h')


def method(signature, text=source):
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    head = '\n'.join(line for line in text[start:opening].splitlines()
                     if not line.lstrip().startswith('#'))
    return head + '\n' + text[opening:end]


# class CCell and the open-list bucket bound, verbatim from the header.
cell = re.search(r'^const int MAX_BOTH_INDEX\s*=.*?;', header, re.M).group(0)
cell += '\n' + method('class CCell', header) + ';\n'
(out / 'pathmgr_cell.inc').write_text(cell, encoding='utf-8')

actual = '\n'.join(method(s) for s in (
    'void CPathMgr::ClearArray( void )',
    'CPathMgr::CPathMgr( int iMapEX, int iMapEY )',
    'CPathMgr::CPathMgr( void )',
    'CPathMgr::~CPathMgr( )',
    'void CPathMgr::Close( )',
    'BOOL CPathMgr::Init( int iMapEX, int iMapEY )',
    'CCell::CCell( )',
    'CCell::CCell( int iX, int iY )',
))
(out / 'pathmgr_actual.inc').write_text(actual, encoding='utf-8')
print('Production bodies SHA256:', hashlib.sha256((cell + actual).encode()).hexdigest(), flush=True)

vs = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
batch = out / 'compile.cmd'
exe = out / 'pathmgr_lifecycle_test.exe'
opt = '/O2' if args.o2 else '/Od'
# C4100: Init's iMapEY argument is only read through m_iHeight in the shipped body.
batch.write_text(f'@echo off\ncall "{vs}" >nul 2>&1\nif errorlevel 1 exit /b 2\n'
                 f'cl /nologo /EHsc /std:c++17 /W4 /wd4100 {opt} /I"{out}" "{HERE / "test_pathmgr_lifecycle.cpp"}" '
                 f'/Fo"{out / "pathmgr_lifecycle.obj"}" /Fe"{exe}"\nexit /b %errorlevel%\n', encoding='utf-8')
compiled = subprocess.run(['cmd', '/c', str(batch)], cwd=out)
if compiled.returncode:
    sys.exit(2)
sys.exit(subprocess.run([str(exe)], cwd=out, timeout=20).returncode)
