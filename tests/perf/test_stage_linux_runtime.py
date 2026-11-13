"""Synthetic loader/copy qualification; never launches a game or real ldd."""
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

import stage_linux_runtime as staging


class StagingTests(unittest.TestCase):
    def run_case(self, response='valid', mutate=None, error=None, publish_failure=False):
        with tempfile.TemporaryDirectory(prefix='runtime with spaces ') as directory:
            root = Path(directory)
            build = root / 'build'
            build.mkdir()
            binary = build / 'enations'
            library = build / 'libvdmplay_posix.so'
            binary.write_bytes(b'trusted-game')
            library.write_bytes(b'trusted-library')
            out = root / 'runtime'

            def loader(command, **kwargs):
                self.assertEqual(command, ['ldd', str((out / 'enations').resolve())])
                self.assertEqual(kwargs['env']['LD_LIBRARY_PATH'], str(out.resolve()))
                self.assertNotIn('LD_PRELOAD', kwargs['env'])
                self.assertEqual(kwargs['timeout'], 20)
                if mutate:
                    (out / mutate if mutate == 'enations' else library).write_bytes(b'changed')
                resolved = out / library.name if response != 'wrong' else library
                stdout = f'libvdmplay_posix.so => {resolved} (0x1234)\n'
                if response == 'missing':
                    stdout = 'libvdmplay_posix.so => not found\n'
                if response == 'duplicate':
                    stdout += stdout
                if response == 'malformed':
                    stdout = f'libvdmplay_posix.so => {resolved}\n'
                return subprocess.CompletedProcess(command, 0, stdout,
                                                   'loader warning' if response == 'diagnostic' else '')

            with mock.patch.dict(staging.os.environ, {'LD_PRELOAD': 'inherited.so'}), \
                 mock.patch.object(staging.subprocess, 'run', side_effect=loader) as check, \
                 mock.patch.object(staging.os, 'replace', wraps=staging.os.replace) as publish:
                if publish_failure:
                    publish.side_effect = RuntimeError('publication failed')
                if error:
                    with self.assertRaisesRegex(RuntimeError, error):
                        staging.stage(binary, library, out)
                    self.assertFalse((out / 'runtime-manifest.json').exists())
                else:
                    hashes = staging.stage(binary, library, out)
                    receipt = json.loads((out / 'runtime-manifest.json').read_text())
                    self.assertEqual(receipt['sha256'], hashes)
                    self.assertEqual(receipt['qualification'], 'copy-integrity-and-loader-resolution-only')
                    self.assertIn('rehash immediately before use', receipt['consumption_requires'])
                    self.assertFalse((out / 'runtime-manifest.json.tmp').exists())
                check.assert_called_once()
                if not error:
                    publish.assert_called_once_with(out / 'runtime-manifest.json.tmp', out / 'runtime-manifest.json')

    def test_matching_staged_resolution(self):
        self.run_case()

    def test_loader_failures_never_publish_manifest(self):
        for response in ('missing', 'wrong', 'duplicate', 'diagnostic', 'malformed'):
            with self.subTest(response=response):
                self.run_case(response=response, error='dependency|resolve')

    def test_mutation_during_loader_never_publishes_manifest(self):
        for target in ('enations', 'library-source'):
            with self.subTest(target=target):
                self.run_case(mutate=target, error='changed during loader')

    def test_failed_publication_leaves_no_final_manifest(self):
        self.run_case(publish_failure=True, error='publication failed')


if __name__ == '__main__':
    unittest.main()
