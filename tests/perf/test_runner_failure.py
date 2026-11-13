"""Linux runner failure/ownership checks with a fake game; not a game-load test."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

runner = Path(__file__).resolve().with_name("run_fixture.py")
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    fixture = root / "source.en"
    fixture.write_bytes(b"stub fixture")
    digest = hashlib.sha256(fixture.read_bytes()).hexdigest()
    fake = root / "fake-game"
    fake.write_text('''#!/usr/bin/env python3
import os, socket, time
s = socket.socket()
s.bind(("127.0.0.1", int(os.environ["EN_P1_PORT"])))
s.listen(2)
port_file = os.environ["EN_P1_PORT_FILE"]
with open(port_file + ".tmp", "w") as f: f.write(str(s.getsockname()[1]) + "\\n")
os.replace(port_file + ".tmp", port_file)
for response in (b"OK protocol=1 fixture=fixture\\n", b"ERR load_failed\\n"):
    c, _ = s.accept()
    c.recv(512)
    c.sendall(response)
    c.close()
while True: time.sleep(1)
''', encoding="utf-8")
    fake.chmod(0o700)
    base = [sys.executable, str(runner), "--executable", str(fake), "--run-dir", str(root),
            "--fixture", str(fixture), "--dummy-sdl", "--seconds", "1"]
    other = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(150)"])
    try:
        bad_output = root / "bad-hash"
        bad = subprocess.run(base + ["--fixture-sha256", "0" * 64, "--output", str(bad_output)],
                             capture_output=True, text=True, timeout=10)
        assert bad.returncode != 0 and not bad_output.exists()
        output = root / "failed-load"
        result = subprocess.run(base + ["--fixture-sha256", digest, "--output", str(output)],
                                capture_output=True, text=True, timeout=15)
        record = json.loads((output / "result.json").read_text())
        assert result.returncode != 0 and record["error"] == "load: ERR load_failed"
        try:
            os.kill(record["pid"], 0)
        except ProcessLookupError:
            pass
        else:
            raise AssertionError("runner did not reap its fake game")
        assert other.poll() is None, "runner affected unrelated process"
        assert hashlib.sha256(fixture.read_bytes()).hexdigest() == digest
        print("runner hash rejection and owned-child failure cleanup passed (fake game)")
        hanging = root / "hanging-game"
        hanging.write_text("#!/usr/bin/env python3\nimport time\ntime.sleep(150)\n", encoding="utf-8")
        hanging.chmod(0o700)
        timeout_base = base.copy()
        timeout_base[timeout_base.index(str(fake))] = str(hanging)
        output = root / "startup-timeout"
        result = subprocess.run(timeout_base + ["--fixture-sha256", digest, "--output", str(output)],
                                capture_output=True, text=True, timeout=105)
        record = json.loads((output / "result.json").read_text())
        assert result.returncode != 0 and record["error"] == "startup deadline"
        try:
            os.kill(record["pid"], 0)
        except ProcessLookupError:
            pass
        else:
            raise AssertionError("runner did not reap its timed-out fake game")
        assert other.poll() is None, "timeout affected unrelated process"
        print("runner startup timeout and owned-child cleanup passed (fake game)")
    finally:
        other.terminate()
        other.wait(timeout=3)
