"""Compile the actual router abort functions in an isolated, single-thread fixture."""
from pathlib import Path
import argparse,hashlib,re,subprocess,sys
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser()
parser.add_argument('--baseline',action='store_true')
parser.add_argument('--baseline-ref',default='5cb9e069')
args=parser.parse_args()
root=HERE.parents[1]
raw=(subprocess.check_output(['git','-C',str(root),'show',args.baseline_ref+':enations_latest/src/chproute.cpp']).decode()
     if args.baseline else (root/'enations_latest/src/chproute.cpp').read_text(encoding='utf-8'))
names=['IsShip','IsVehicleCargo','UnAssignShip','ReleasePickupShips','UnassignTrucks']
pieces=[]
for name in names:
    for match in re.finditer(r'^(?:BOOL|void) CHPRouter::'+name+r'\(',raw,re.M):
        # Function closing braces are unindented in this source.
        end=raw.index('\n}',match.end())+2
        pieces.append(raw[match.start():end])
body='\n\n'.join(pieces)
out=HERE/'seek-scan-out'/('ship-baseline' if args.baseline else 'ship-candidate')
out.mkdir(parents=True,exist_ok=True)
(out/'ship_abort_actual.inc').write_text(body,encoding='utf-8')
print('Production function SHA256:',hashlib.sha256(body.encode()).hexdigest(),flush=True)
vs=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
exe=out/'ship_abort_test.exe'
batch=out/'compile.cmd'
batch.write_text(f'@echo off\ncall "{vs}" >nul 2>&1\nif errorlevel 1 exit /b 2\ncl /nologo /EHsc /std:c++17 /W4 /I"{out}" "{HERE / "test_ship_abort.cpp"}" /Fo"{out / "ship_abort.obj"}" /Fe"{exe}"\nexit /b %errorlevel%\n',encoding='utf-8')
if subprocess.run(['cmd','/c',str(batch)],cwd=out).returncode:sys.exit(2)
sys.exit(subprocess.run([str(exe)],cwd=out,timeout=20).returncode)
