"""Verify the exact child environment passed by the runner without /proc access."""
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
from unittest import mock

import run_fixture

with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    executable = root / "game"
    fixture = root / "fixture.en"
    executable.write_bytes(b"dummy executable")
    fixture.write_bytes(b"dummy fixture")
    output = root / "out"
    argv = ["run_fixture.py", "--executable", str(executable), "--run-dir", str(root),
            "--fixture", str(fixture), "--fixture-sha256", hashlib.sha256(fixture.read_bytes()).hexdigest(),
            "--output", str(output), "--dummy-sdl", "--env", "EN_PATH_ASYNC=0"]
    captured = {}
    def capture(*args, **kwargs):
        captured.update(kwargs["env"])
        raise RuntimeError("intentional launch interception")
    with mock.patch.object(sys, "argv", argv), mock.patch.dict(os.environ, {"EN_UNREQUESTED_TEST": "sentinel"}), \
         mock.patch.object(run_fixture.subprocess, "Popen", side_effect=capture):
        assert run_fixture.main() == 1
    record = json.loads((output / "result.json").read_text())
    assert record["error"] == "intentional launch interception"
    assert "EN_UNREQUESTED_TEST" not in captured
    assert captured["EN_PATH_ASYNC"] == "0"
    assert captured["SDL_VIDEODRIVER"] == "dummy"
    assert "EN_UNREQUESTED_TEST" in record["inherited_en_removed"]
    assert record["environment_overrides"] == {"EN_PATH_ASYNC": "0"}
print("runner child environment at Popen verified without /proc; game was not launched")
