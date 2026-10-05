"""A load-join seat claim (cmd_select_plyr) made in another joiner's name, on the SHIPPED host
handler, at /Od and /O2.

Extracts VERBATIM: VPMsgHdr (vdmplay.h), class CNetCmd and class CNetSelectPlyr (netcmd.h) and
CmdSelectPlyr (netapi.cpp), then compiles them into test_select_plyr_sender.cpp.

    python tests/net/run-select-plyr-sender-test.py
    python tests/net/run-select-plyr-sender-test.py --baseline-ref ac14f15c   # expected to FAIL

Nothing is written into the source tree; artifacts go to %TEMP%/en-select-plyr-sender-test.
Exit codes: 0 all pass, 1 a check failed, 2 toolchain / compile / extraction error.
"""
import argparse
import hashlib
import re
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root', type=Path, default=ROOT)
parser.add_argument('--out-dir', type=Path, default=Path(tempfile.gettempdir()) / 'en-select-plyr-sender-test')
parser.add_argument('--baseline-ref', help='Extract from this Git revision instead of the working tree.')
args = parser.parse_args()


def read_source(name):
    rel = 'enations_latest/src/' + name
    if args.baseline_ref:
        return subprocess.check_output(
            ['git', '-C', str(args.source_root), 'show', args.baseline_ref + ':' + rel]).decode('utf-8', 'replace').replace('\r\n', '\n')
    return (args.source_root / rel).read_text(encoding='utf-8', errors='replace').replace('\r\n', '\n')


def block(source, start_text):
    """From start_text through the brace that closes the first brace after it, verbatim."""
    if source.count(start_text) != 1:
        raise ValueError(f'{start_text.strip()!r} found {source.count(start_text)} times')
    start = source.index(start_text)
    depth, end = 1, source.index('{', start) + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


try:
    vdm = read_source('vdmplay.h')
    netcmd_h = read_source('netcmd.h')
    netapi = read_source('netapi.cpp')
    hdr = block(vdm, 'typedef struct VPMsgHdr')
    hdr = vdm[vdm.index(hdr):vdm.index(';', vdm.index(hdr) + len(hdr)) + 1]
    cmd_cls = block(netcmd_h, 'class CNetCmd : public VPMsgHdr') + ';'
    fwd = sorted(set(re.findall(r'\b(C(?:Net|Msg)\w+)\b', cmd_cls)) - {'CNetCmd'})
    incs = {
        'sps_hdr.inc': hdr + '\n',
        'sps_cmd.inc': ''.join(f'class {n};\n' for n in fwd) + cmd_cls + '\n',
        'sps_msg.inc': block(netcmd_h, 'class CNetSelectPlyr : public CNetCmd') + ';\n',
        'sps_handler.inc': block(netapi, 'static void CmdSelectPlyr( CNetSelectPlyr* pMsg )') + '\n',
    }
except ValueError as e:
    print('extraction failed:', e)
    sys.exit(2)

print('Production text SHA256:', hashlib.sha256(''.join(incs.values()).encode()).hexdigest(), flush=True)

vs_roots = [Path('C:/Program Files/Microsoft Visual Studio/2022/' + e)
            for e in ('Community', 'Enterprise', 'Professional', 'BuildTools')]
vcvars = next((r / 'VC/Auxiliary/Build/vcvars64.bat' for r in vs_roots
               if (r / 'VC/Auxiliary/Build/vcvars64.bat').exists()), None)
if vcvars is None:
    print('VS 2022 vcvars64.bat not found.', file=sys.stderr)
    sys.exit(2)

failed = 0
for opt in ('Od', 'O2'):
    out = args.out_dir / ('baseline' if args.baseline_ref else 'tree') / opt
    out.mkdir(parents=True, exist_ok=True)
    for name, text in incs.items():
        (out / name).write_text(text, encoding='utf-8')
    exe = out / 'select_plyr_sender_test.exe'
    batch = out / 'compile.cmd'
    batch.write_text(
        '@echo off\r\n'
        f'call "{vcvars}" >nul 2>&1\r\n'
        'if errorlevel 1 exit /b 2\r\n'
        f'cl /nologo /EHsc /std:c++17 /W4 /{opt} /I"{out}" "{HERE / "test_select_plyr_sender.cpp"}" '
        f'/Fo"{out / "select_plyr_sender.obj"}" /Fe"{exe}"\r\n'
        'exit /b %errorlevel%\r\n', encoding='utf-8')
    print(f'--- /{opt} ---', flush=True)
    if subprocess.run(['cmd', '/c', str(batch)], cwd=out).returncode:
        sys.exit(2)
    if subprocess.run([str(exe)], cwd=out, timeout=60).returncode:
        failed = 1

sys.exit(failed)
