#!/usr/bin/env python3
"""Stage a Linux game and its matching vdmplay shared library for a private run.

Use binaries from the SAME build; this checks copy integrity and dynamic-loader
resolution, not ABI compatibility or game functionality. Run only with trusted
build outputs: ldd can execute code from a supplied binary on some systems.
The caller must exclusively own the output directory throughout staging and
consumption. Rehash every staged file against the manifest immediately before
launch/use; this receipt cannot guarantee post-publication immutability.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def stage(binary, library, out):
    if binary.parent != library.parent:
        raise ValueError('game and library must come from the same build directory')
    if library.name != 'libvdmplay_posix.so':
        raise ValueError('expected matching build output libvdmplay_posix.so')
    out.mkdir(parents=True, exist_ok=False)
    copied = {}
    for source, name in ((binary, 'enations'), (library, 'libvdmplay_posix.so')):
        before = digest(source)
        dest = out / name
        shutil.copy2(source, dest)
        if digest(dest) != before or digest(source) != before:
            raise RuntimeError('copy or source hash mismatch: ' + name)
        copied[name] = before
    # Require co-located resolution, not a coincidental library in the build tree.
    env = dict(os.environ)
    env.pop('LD_PRELOAD', None)
    env['LD_LIBRARY_PATH'] = str(out.resolve())
    result = subprocess.run(['ldd', str((out / 'enations').resolve())], env=env,
                            capture_output=True, text=True, timeout=20, check=True)
    if 'not found' in result.stdout or result.stderr.strip():
        raise RuntimeError('unresolved or diagnostic dynamic dependency check:\n' + result.stdout + result.stderr)
    lines = [line for line in result.stdout.splitlines() if line.strip().startswith('libvdmplay_posix.so =>')]
    if len(lines) != 1:
        raise RuntimeError('vdmplay dependency not uniquely resolved:\n' + result.stdout)
    match = re.fullmatch(r'\s*(.*?)\s+\(0x[0-9a-fA-F]+\)\s*', lines[0].split('=>', 1)[1])
    if not match:
        raise RuntimeError('malformed vdmplay dependency resolution: ' + lines[0])
    resolved = match.group(1)
    if Path(resolved).resolve() != (out / 'libvdmplay_posix.so').resolve():
        raise RuntimeError('vdmplay did not resolve to staged library: ' + resolved)
    # Loader inspection is a separate subprocess: reject changes after copying
    # rather than certify the earlier bytes as the current staged closure.
    for source, name in ((binary, 'enations'), (library, 'libvdmplay_posix.so')):
        if digest(source) != copied[name] or digest(out / name) != copied[name]:
            raise RuntimeError('source or staged output changed during loader check: ' + name)
    receipt = json.dumps({
        'qualification': 'copy-integrity-and-loader-resolution-only',
        'consumption_requires': 'exclusive directory ownership and rehash immediately before use',
        'sha256': copied, 'ldd': result.stdout}, indent=2) + '\n'
    pending = out / 'runtime-manifest.json.tmp'
    with pending.open('x', encoding='utf-8') as file:
        file.write(receipt)
        file.flush()
        os.fsync(file.fileno())
    os.replace(pending, out / 'runtime-manifest.json')
    return copied


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--library', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(stage(args.binary.resolve(), args.library.resolve(), args.output.resolve()), indent=2))

if __name__ == '__main__':
    main()
