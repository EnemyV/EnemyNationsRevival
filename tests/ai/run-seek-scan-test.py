"""Compile production GetOpForUnitScan and SeekOpfor in a small fake world.

The algorithm bodies are extracted verbatim, not mirrored. The fake world is
single-threaded and preserves the raw-spatial versus dying-filtered-ID distinction.
Outputs go to a temp directory (default %TEMP%/aitests/seek-scan), never the repo.
"""
from pathlib import Path
import argparse
import hashlib
import subprocess
import os
import sys
import tempfile

HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser()
parser.add_argument('--source-root',type=Path,default=HERE.parents[1])
parser.add_argument('--baseline',action='store_true',help='Extract the original 015 baseline rather than the working tree')
parser.add_argument('--baseline-ref',default='5cb9e069',help='Git revision used with --baseline')
parser.add_argument('--out-dir',type=Path,default=Path(tempfile.gettempdir())/'aitests'/'seek-scan')
parser.add_argument('--vcvars',type=Path,help='vcvars64.bat (default: same VS 2022 roots as run-ai-tests.ps1)')
args=parser.parse_args()
out=args.out_dir/('baseline' if args.baseline else 'candidate')
out.mkdir(exist_ok=True,parents=True)
source=args.source_root/'enations_latest'/'src'
def read_source(name):
    if args.baseline:
        return subprocess.check_output(['git','-C',str(args.source_root),'show',args.baseline_ref+':enations_latest/src/'+name]).decode('utf-8')
    return (source/name).read_text(encoding='utf-8')
goal=read_source('caigmgr.cpp')
task=read_source('caitmgr.cpp')
scan=goal[goal.index('DWORD CAIGoalMgr::GetOpForUnitScan('):goal.index('int CAIGoalMgr::AssessThreat( CVehicle*')]
seek=task[task.index('void CAITaskMgr::SeekOpfor('):task.index('void CAITaskMgr::ClearTaskUnit(')]
(out/'seek_scan_actual.inc').write_text(scan+'\n'+seek,encoding='utf-8')
print('Production function SHA256:',hashlib.sha256((scan+'\n'+seek).encode()).hexdigest(),flush=True)
def find_vcvars():
    if args.vcvars:return args.vcvars
    for base in (os.environ.get('ProgramFiles',''),os.environ.get('ProgramFiles(x86)','')):
        for ed in ('Community','Enterprise','Professional','BuildTools'):
            p=Path(base)/'Microsoft Visual Studio'/'2022'/ed/'VC'/'Auxiliary'/'Build'/'vcvars64.bat'
            if p.is_file():return p
    print('VS 2022 vcvars64.bat not found (pass --vcvars)');sys.exit(2)
vs=find_vcvars()
batch=out/'compile.cmd'
exe=out/'seek_scan_test.exe'
batch.write_text(f'@echo off\ncall "{vs}" >nul 2>&1\nif errorlevel 1 exit /b 2\ncl /nologo /EHsc /std:c++17 /W4 /I"{out}" "{HERE / "test_seek_scan.cpp"}" /Fo"{out / "seek_scan.obj"}" /Fe"{exe}"\nexit /b %errorlevel%\n',encoding='utf-8')
result=subprocess.run(['cmd','/c',str(batch)],cwd=out)
if result.returncode:sys.exit(2)
sys.exit(subprocess.run([str(exe)],cwd=out,timeout=20).returncode)
