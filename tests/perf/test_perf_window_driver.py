"""Synthetic file/protocol tests; no game, subprocess or network listener."""
from pathlib import Path
import tempfile
import shutil
import json
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import uuid

from perf_window_driver import PerfWindow, attach, main


def row(t):
    return f"t={t} fps=10 avg=100 max=100 | frame.ge100=10\n".encode()


class WindowTests(unittest.TestCase):
    def setUp(self):
        # Python 3.14's Windows mkdtemp(0700) ACL can exclude a sandbox's
        # restricted SID. A normal workspace directory avoids that test-only ACL.
        parent = Path(tempfile.gettempdir()).resolve()
        self.directory = parent / ("perf-window-test-" + uuid.uuid4().hex)
        self.directory.mkdir()
        self.assertEqual(self.directory.resolve().parent, parent)
        self.addCleanup(shutil.rmtree, self.directory)
        self.window = PerfWindow(self.directory)
        self.log = self.window.log

    def append(self, data):
        with self.log.open("ab") as stream:
            stream.write(data)

    def finish_marks(self):
        self.window.before_end_send()
        self.window.end_confirmed = True

    def test_partial_begin_and_end_rows_and_post_end_data(self):
        self.log.write_bytes(b"# session\n" + row(119) + row(120)[:9])
        self.window.begin_reply()
        self.append(row(120)[9:] + row(121) + row(122) + row(123)[:7])
        self.finish_marks()
        self.append(row(123)[7:] + row(124))
        result = self.window.finish()
        self.assertEqual(result["buffer_guard_t"], 120)
        self.assertEqual(result["anchor_t"], 121)
        self.assertEqual(result["last_included_t"], 122)
        self.assertEqual(result["included_intervals"], 1)
        self.assertEqual((self.directory / "perf-window.log").read_bytes(),
                         row(121) + row(122))

    def test_begin_excludes_preexisting_complete_rows(self):
        self.log.write_bytes(row(1) + row(2))
        self.window.begin_reply()
        self.append(row(3) + row(4) + row(5))
        self.finish_marks()
        self.assertEqual(self.window.finish()["included_intervals"], 1)

    def test_prefix_rewrite_and_truncation_rejected(self):
        for replacement in (b"", row(99) + row(2) + row(3)):
            with self.subTest(replacement=replacement):
                self.window = PerfWindow(self.directory)
                self.log.write_bytes(row(1))
                self.window.begin_reply()
                self.append(row(2) + row(3))
                self.finish_marks()
                self.log.write_bytes(replacement)
                with self.assertRaisesRegex(ValueError, "truncated or rewritten"):
                    self.window.finish()

    def test_missing_or_rejected_marks_fail_closed(self):
        self.log.write_bytes(row(1))
        with self.assertRaises(ValueError):
            self.window.before_end_send()
        self.window.begin_reply()
        with self.assertRaises(ValueError):
            self.window.begin_reply()
        self.append(row(2) + row(3))
        self.window.before_end_send()
        with self.assertRaisesRegex(ValueError, "successful measurement marks"):
            self.window.finish()

    def test_too_short_and_restarted_logs_rejected(self):
        for data in (row(2), row(2) + row(3), row(2) + row(3) + row(3),
                     row(2) + row(3) + row(1)):
            with self.subTest(data=data):
                self.window = PerfWindow(self.directory)
                self.log.write_bytes(row(1))
                self.window.begin_reply()
                self.append(data)
                self.finish_marks()
                with self.assertRaises(ValueError):
                    self.window.finish()

    def test_socket_hook_observes_real_request_reply_order(self):
        self.log.write_bytes(row(1))
        events = []
        window = self.window

        class Client:
            def __enter__(self):
                return self

            def __exit__(self, *args):
                pass

            def sendall(self, command):
                events.append(command)
                if command.endswith(b" MARK BEGIN\n"):
                    self.response = "OK mark=begin monotonic_ns=1"
                else:
                    # Prove end snapshot preceded the actual send: this row
                    # must not enter the selected window.
                    assert window.end is not None
                    with window.log.open("ab") as stream:
                        stream.write(row(5))
                    self.response = "OK mark=end monotonic_ns=2"

        runner = SimpleNamespace(socket=SimpleNamespace(create_connection=lambda: Client()),
                                 read_response=lambda client: client.response)
        attach(runner, window)
        with runner.socket.create_connection() as client:
            client.sendall(b"token MARK BEGIN\n")
            self.assertIsNone(window.begin)
            runner.read_response(client)
            self.assertIsNotNone(window.begin)
        self.append(row(2) + row(3) + row(4))
        with runner.socket.create_connection() as client:
            client.sendall(b"token MARK END\n")
            runner.read_response(client)
        result = window.finish()
        self.assertEqual(result["last_included_t"], 4)
        self.assertEqual(result["included_intervals"], 1)
        self.assertEqual(events, [b"token MARK BEGIN\n", b"token MARK END\n"])

    def test_driver_failure_does_not_rewrite_runner_status(self):
        runner = self.directory / "fake_runner.py"
        output = self.directory / "output"
        runner.write_text("""
import socket
from pathlib import Path
import sys
def read_response(client):
    return 'unused'
def main():
    out = Path(sys.argv[sys.argv.index('--output') + 1])
    out.mkdir()
    (out / 'result.json').write_text('{"status":"qualified"}')
    return 0
""")
        with patch.object(sys, "argv", ["driver", "--runner", str(runner),
                                        "--output", str(output), "--env", "EN_PERF=1"]):
            self.assertEqual(main(), 1)
        self.assertEqual(json.loads((output / "result.json").read_text())["status"], "qualified")
        record = json.loads((output / "perf-boundaries.json").read_text())
        self.assertEqual(record["status"], "invalid")
        self.assertEqual(record["runner_exit_code"], 0)


if __name__ == "__main__":
    unittest.main()
