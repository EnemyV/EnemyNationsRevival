"""Compile the shipped UpdateStagingTasks lifecycle-completion block in a fake world.

The block is extracted verbatim from caigmgr.cpp (not mirrored) into
staging_release_actual.inc and #included by test_staging_release.cpp.
Outputs go to a temp directory (default %TEMP%/aitests/staging-release), never the repo.
"""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import tempfile

HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser()
parser.add_argument('--source-root',type=Path,default=HERE.parents[1])
parser.add_argument('--out-dir',type=Path,default=Path(tempfile.gettempdir())/'aitests'/'staging-release')
parser.add_argument('--vcvars',type=Path,help='vcvars64.bat (default: same VS 2022 roots as run-ai-tests.ps1)')
args=parser.parse_args()
out=args.out_dir
out.mkdir(exist_ok=True,parents=True)
goal=(args.source_root/'enations_latest'/'src'/'caigmgr.cpp').read_text(encoding='utf-8')
fn=goal.index('void CAIGoalMgr::UpdateStagingTasks( void )')
beg=goal.index('    // LIFECYCLE COMPLETION',fn)
end=goal.index('    // now go thru task list and find active IDT_PREPAREWAR tasks',beg)
(out/'staging_release_actual.inc').write_text(goal[beg:end],encoding='utf-8')
def find_vcvars():
    if args.vcvars:return args.vcvars
    for base in (os.environ.get('ProgramFiles',''),os.environ.get('ProgramFiles(x86)','')):
        for ed in ('Community','Enterprise','Professional','BuildTools'):
            p=Path(base)/'Microsoft Visual Studio'/'2022'/ed/'VC'/'Auxiliary'/'Build'/'vcvars64.bat'
            if p.is_file():return p
    print('VS 2022 vcvars64.bat not found (pass --vcvars)');sys.exit(2)
vs=find_vcvars()
batch=out/'compile.cmd'
exe=out/'staging_release_test.exe'
batch.write_text(f'@echo off\ncall "{vs}" >nul 2>&1\nif errorlevel 1 exit /b 2\ncl /nologo /EHsc /std:c++17 /W4 /I"{out}" "{HERE / "test_staging_release.cpp"}" /Fo"{out / "staging_release.obj"}" /Fe"{exe}"\nexit /b %errorlevel%\n',encoding='utf-8')
if subprocess.run(['cmd','/c',str(batch)],cwd=out).returncode:sys.exit(2)
sys.exit(subprocess.run([str(exe)],cwd=out,timeout=20).returncode)
