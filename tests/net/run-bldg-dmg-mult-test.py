"""Civil Defence replication (CNetBldgDmgMult) on the SHIPPED code, at /Od and /O2.

Extracts VERBATIM: VPMsgHdr (vdmplay.h), class CNetCmd and CNetBldgDmgMult
(netcmd.h), the CNetBldgDmgMult ctor and CNetCmd::FitsBuffer (netcmd.cpp), the bldg_dmg_mult
receive case (netapi.cpp), the Civil Defence constants and permille helpers (edicts.h) and
CPlayer::GetEdictBldgDmgMult / ReportBldgDmgMult / SetRemoteBldgDmgPermille (player.cpp).
FitsBuffer's other message types: the real netcmd.h class wherever FitsBuffer reads its fields
(pinned to its Release size), else a stand-in sized from wire_layout_assert.cpp. Also lints
wire_layout_assert.cpp for the CNetBldgDmgMult size pin in both build configs.

    python tests/net/run-bldg-dmg-mult-test.py
    python tests/net/run-bldg-dmg-mult-test.py --baseline-ref e4ea13a7   # expected to FAIL

At a pre-fix ref there is no message: the fixture is built with EN_BASELINE and the old inline
GetEdictBldgDmgMult from player.h, and only the server-side multiplier checks run.
Nothing is written into the source tree; artifacts go to %TEMP%/en-bldg-dmg-mult-test.
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
parser.add_argument('--out-dir', type=Path, default=Path(tempfile.gettempdir()) / 'en-bldg-dmg-mult-test')
parser.add_argument('--baseline-ref', help='Extract from this Git revision instead of the working tree.')
args = parser.parse_args()


def read_source(name):
    rel = 'enations_latest/src/' + name
    if args.baseline_ref:
        return subprocess.check_output(
            ['git', '-C', str(args.source_root), 'show', args.baseline_ref + ':' + rel]).decode('utf-8', 'replace').replace('\r\n', '\n')
    return (args.source_root / rel).read_text(encoding='utf-8', errors='replace')


def block(source, start_text, open_ch='{'):
    """From start_text through the brace that closes the first brace after it, verbatim."""
    start = source.index(start_text)
    opening = source.index(open_ch, start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


vdm = read_source('vdmplay.h')
netcmd_h = read_source('netcmd.h')
netcmd_cpp = read_source('netcmd.cpp')
netapi = read_source('netapi.cpp')
edicts_h = read_source('edicts.h')
player_cpp = read_source('player.cpp')
player_h = read_source('player.h')
wire = read_source('wire_layout_assert.cpp')

incs = {}
baseline = 'class CNetBldgDmgMult' not in netcmd_h
try:
    if baseline:
        if not args.baseline_ref:
            raise ValueError('CNetBldgDmgMult not found in the working tree')
        incs['bdm_player.inc'] = block(player_h, 'float GetEdictBldgDmgMult( ) const') + '\n'
    else:
        hdr = block(vdm, 'typedef struct VPMsgHdr')
        hdr = vdm[vdm.index(hdr):vdm.index(';', vdm.index(hdr) + len(hdr)) + 1]
        cmd_cls = block(netcmd_h, 'class CNetCmd : public VPMsgHdr') + ';'
        fwd = sorted(set(re.findall(r'\b(C(?:Net|Msg)\w+)\b', cmd_cls)) - {'CNetCmd'})
        cmd_cls = ''.join(f'class {n};\n' for n in fwd) + cmd_cls
        msg_cls = block(netcmd_h, 'class CNetBldgDmgMult : public CNetCmd') + ';'
        ctor = block(netcmd_cpp, 'CNetBldgDmgMult::CNetBldgDmgMult(')
        fits = block(netcmd_cpp, 'BOOL CNetCmd::FitsBuffer( int cbAvail ) const')
        rx = block(netapi, '    case CNetCmd::bldg_dmg_mult: {')
        i = edicts_h.index('const int CIVDEF_BASE_DRAFT')
        civdef = block(edicts_h[i:], 'inline int ClampBldgDmgPermille(')
        civdef = edicts_h[i:edicts_h.index(civdef, i) + len(civdef)]
        player = ''.join(block(player_cpp, s) + '\n' for s in (
            'float CPlayer::GetEdictBldgDmgMult( ) const',
            'void CPlayer::ReportBldgDmgMult( )',
            'void CPlayer::SetRemoteBldgDmgPermille( int iPermille )'))

        # FitsBuffer's other message types. A type whose fields it READS (other than a compound
        # header's m_iNumMsgs) is the real class from netcmd.h, verbatim, pinned in the generated
        # text to its Release size from wire_layout_assert.cpp. A type it only sizes, or reads
        # m_iNumMsgs from, is a stand-in with m_iNumMsgs at the real offset 12, sized from its pin.
        helper = ''
        if 'static BOOL TextEndsBefore(' in netcmd_cpp:
            helper = block(netcmd_cpp, 'static BOOL TextEndsBefore(') + '\n'
        rel = wire[wire.index('#else'):]
        pin = {m.group(1): int(m.group(2)) for m in re.finditer(
            r'static_assert\(\s*sizeof\(\s*(\w+)\s*\)\s*==\s*(\d+)', rel)}
        var_type = {m.group(2): m.group(1) for m in re.finditer(
            r'const\s+(\w+)\s*\*\s*(\w+)\s*=\s*\(\s*const\s+\1\s*\*\s*\)\s*this', fits)}
        reads = {}
        for m in re.finditer(r'\(\s*\(\s*const\s+(\w+)\s*\*\s*\)\s*this\s*\)\s*->\s*(\w+)', fits):
            reads.setdefault(m.group(1), set()).add(m.group(2))
        for v, t in var_type.items():
            for f in re.findall(r'\b' + v + r'\s*->\s*(\w+)', fits):
                reads.setdefault(t, set()).add(f)
        named = set(re.findall(r'sizeof\(\s*(\w+)\s*\)', fits)) | set(reads) | set(var_type.values())
        named -= {'CNetCmd', 'CNetBldgDmgMult', 'int', 'char'}
        real, stand = [], []
        for t in sorted(named):
            if reads.get(t, set()) - {'m_iNumMsgs'}:
                real.append(t)
                stand.append(block(netcmd_h, f'class {t} : public CNetCmd') + ';\n')
                if t in pin:
                    stand.append(f'static_assert( sizeof( {t} ) == {pin[t]}, "{t}: Release pin" );\n')
            else:
                stand.append(f'class {t} : public CNetCmd {{ public: int m_iNumMsgs; '
                             f'char m_pad[{max(pin.get(t, 64), 16) - 16} + 1]; }};\n')
        stubs = ''.join(f'const int {n} = 8;\n' for n in sorted(set(re.findall(r'\b(NUM_\w+_ELEM)\b', fits))))
        for name in sorted(set(re.findall(r'\b(MAX_\w+)\b', fits))):
            line = re.search(r'^const int ' + name + r'\s*=.*$', netcmd_h, re.M)
            if line is None:
                raise ValueError(name + ' not found in netcmd.h')
            stubs += line.group(0) + '\n'
        body_text = ''.join(stand)
        # What the extracted classes embed: CMaterialTypes verbatim from base.h; CInitData
        # (racedata.h) only by size -- 124 bytes, held to it by CNetPlayer's Release pin above.
        if 'CMaterialTypes' in body_text:
            stubs = block(read_source('base.h'), 'class CMaterialTypes') + ';\n' + stubs
        if 'CInitData' in body_text:
            stubs = 'class CInitData { public: char m_ab[124]; };\n' + stubs
        stubs += body_text
        print('FitsBuffer types extracted as real classes:', ', '.join(real) or '-')

        incs['bdm_hdr.inc'] = hdr + '\n'
        incs['bdm_cmd.inc'] = cmd_cls + '\n'
        incs['bdm_msg.inc'] = msg_cls + '\n'
        incs['bdm_stubs.inc'] = stubs
        incs['bdm_netcmd.inc'] = ctor + '\n' + helper + fits + '\n'
        incs['bdm_rx.inc'] = rx + '\n'
        incs['bdm_civdef.inc'] = civdef + '\n'
        incs['bdm_player.inc'] = player
        pins = wire.count('static_assert(sizeof(CNetBldgDmgMult)==20,')
        if pins != 2:
            print(f'wire_layout_assert.cpp: CNetBldgDmgMult pinned {pins} times, want 2 (Debug + Release)')
            sys.exit(1)
except ValueError as e:
    print('extraction failed:', e)
    sys.exit(2)

print('baseline mode (no message)' if baseline else 'wire_layout_assert.cpp: size pinned in both configs')
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
    exe = out / 'bldg_dmg_mult_test.exe'
    batch = out / 'compile.cmd'
    define = '/DEN_BASELINE ' if baseline else ''
    batch.write_text(
        '@echo off\r\n'
        f'call "{vcvars}" >nul 2>&1\r\n'
        'if errorlevel 1 exit /b 2\r\n'
        f'cl /nologo /EHsc /std:c++17 /W4 /{opt} {define}/I"{out}" "{HERE / "test_bldg_dmg_mult.cpp"}" '
        f'/Fo"{out / "bldg_dmg_mult.obj"}" /Fe"{exe}"\r\n'
        'exit /b %errorlevel%\r\n', encoding='utf-8')
    print(f'--- /{opt} ---', flush=True)
    if subprocess.run(['cmd', '/c', str(batch)], cwd=out).returncode:
        sys.exit(2)
    if subprocess.run([str(exe)], cwd=out, timeout=60).returncode:
        failed = 1

sys.exit(failed)
