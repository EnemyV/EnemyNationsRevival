"""Exercise the opt-in listener against a stub game, not a gameplay test."""
import os
import secrets
import socket
import subprocess
import sys
import time
import tempfile
from pathlib import Path

binary = sys.argv[1]
token = secrets.token_hex(16)
# Use a unique file in the test temp root, rather than a private directory:
# Windows sandboxed subprocesses may be unable to traverse tempfile's ACL.
fd, port_name = tempfile.mkstemp(prefix="p1-port-")
os.close(fd)
port_file = Path(port_name)
port_file.unlink()
port = None
env = dict(os.environ, EN_P1_PORT="0", EN_P1_PORT_FILE=str(port_file), EN_P1_TOKEN=token, EN_P1_FIXTURE="known-fixture")
process = subprocess.Popen([binary], env=env)

def call(message):
    with socket.create_connection(("127.0.0.1", port), timeout=2) as conn:
        conn.sendall((message + "\n").encode())
        conn.settimeout(2)
        deadline = time.monotonic() + 2
        data = bytearray()
        try:
            while b"\n" not in data:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise TimeoutError("listener response deadline exceeded")
                conn.settimeout(remaining)
                chunk = conn.recv(min(512, 2049 - len(data)))
                if not chunk:
                    raise AssertionError("listener closed before newline")
                data.extend(chunk)
                if len(data) > 2048:
                    raise AssertionError("listener response exceeds cap")
            if data.count(b"\n") != 1 or data[-1:] != b"\n":
                raise AssertionError("invalid response framing")
            return data.decode()
        except ConnectionResetError:
            # Windows may reset when the server closes with malformed unread data.
            return "RESET"

try:
    for _ in range(100):
        try:
            if port is None:
                port = int(port_file.read_text())
            if call(token + " READY").startswith("OK protocol=1"):
                break
        except (OSError, ValueError):
            time.sleep(0.02)
    else:
        raise AssertionError("listener did not start")
    assert call("wrong STATUS") == "ERR unauthorized\n"
    assert call("wrong ZOOM trial -0.25") == "ERR unauthorized\n"
    assert call(token + " " + "X" * 512) in ("ERR invalid_request\n", "RESET")
    assert call(token + " STATUS\x00ignored") in ("ERR invalid_request\n", "RESET")
    assert call(token + " LOAD arbitrary-path") == "ERR unknown_command\n"
    assert call(token + " LOAD fixture") == "OK loaded\n"
    assert call(token + " MENU") == "OK menu\n"
    assert call(token + " CAMERA 64 128") == "OK CAMERA 64 128\n"
    assert call(token + " SPEED 8") == "OK SPEED 8\n"
    assert call(token + " MARK BEGIN") == "OK MARK BEGIN\n"
    assert call(token + " ZOOM trial_1 -0.25") == "OK ZOOM trial_1 -0.25\n"
    assert call(token + " ZOOM STATUS") == "OK ZOOM STATUS\n"
    assert call(token + " MARK END") == "OK MARK END\n"
    assert "players=1" in call(token + " STATUS")
    assert call(token + " QUIT") == "OK quitting\n"
    assert process.wait(timeout=3) == 0
    print("listener protocol checks passed")
finally:
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

# Exercise the failure response with a stub; this does not validate game save parsing.
env["EN_P1_FIXTURE"] = "invalid-fixture"
port_file.unlink()
port = None
process = subprocess.Popen([binary], env=env)
try:
    for _ in range(100):
        try:
            if port is None:
                port = int(port_file.read_text())
            if call(token + " READY").startswith("OK protocol=1"):
                break
        except (OSError, ValueError):
            time.sleep(0.02)
    else:
        raise AssertionError("listener did not start")
    assert call(token + " LOAD fixture") == "ERR load_failed\n"
    assert call(token + " QUIT") == "OK quitting\n"
    assert process.wait(timeout=3) == 0
    print("stub load failure response passed")
finally:
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

# Exercise exact buffer boundaries and malformed formatter output over the listener.
for mode in ("boundary", "short", "unterminated", "multiline"):
    env["EN_P1_TEST_STATUS_MODE"] = mode
    port_file.unlink()
    port = None
    process = subprocess.Popen([binary], env=env)
    try:
        for _ in range(100):
            try:
                if port is None:
                    port = int(port_file.read_text())
                if call(token + " READY").startswith("OK protocol=1"):
                    break
            except (OSError, ValueError):
                time.sleep(0.02)
        else:
            raise AssertionError("listener did not start")
        expected = "OK " + "x" * 1019 + "\n" if mode == "boundary" else "ERR snapshot_truncated\n"
        for _ in range(100):
            response = call(token + " STATUS")
            if response == expected:
                break
            time.sleep(0.01)
        else:
            raise AssertionError(f"{mode}: unexpected snapshot: {response!r}")
        assert call(token + " QUIT") == "OK quitting\n"
        assert process.wait(timeout=3) == 0
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

port_file.unlink(missing_ok=True)
