"""Compile production GetOpForUnitScan and SeekOpfor in a small fake world.

The algorithm bodies are extracted verbatim, not mirrored. The fake world is
single-threaded and preserves the raw-spatial versus dying-filtered-ID distinction.
Outputs remain under this isolated checkout's tests/ai/seek-scan-out directory.
"""
from pathlib import Path
import argparse
import hashlib
import subprocess
import sys

HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser()
parser.add_argument('--source-root',type=Path,default=HERE.parents[1])
parser.add_argument('--baseline',action='store_true',help='Extract the original 015 baseline rather than the working tree')
parser.add_argument('--baseline-ref',default='5cb9e069',help='Git revision used with --baseline')
args=parser.parse_args()
out=HERE/'seek-scan-out'/('baseline' if args.baseline else 'candidate')
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
vs=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
batch=out/'compile.cmd'
exe=out/'seek_scan_test.exe'
batch.write_text(f'@echo off\ncall "{vs}" >nul 2>&1\nif errorlevel 1 exit /b 2\ncl /nologo /EHsc /std:c++17 /W4 /I"{out}" "{HERE / "test_seek_scan.cpp"}" /Fo"{out / "seek_scan.obj"}" /Fe"{exe}"\nexit /b %errorlevel%\n',encoding='utf-8')
result=subprocess.run(['cmd','/c',str(batch)],cwd=out)
if result.returncode:sys.exit(2)
sys.exit(subprocess.run([str(exe)],cwd=out,timeout=20).returncode)
