"""POSIX fake-game regression for a clean shutdown taking over ten seconds."""
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

if not __debug__:
    raise SystemExit('Assertions must be enabled for this regression.')
if os.name != 'posix':
    raise SystemExit('This fake-game socket regression requires POSIX.')

runner = Path(__file__).resolve().with_name('run_fixture.py')
fake_source = r'''#!/usr/bin/env python3
import os, socket, time
s = socket.socket(); s.bind(('127.0.0.1', 0)); s.listen(2)
with open(os.environ['EN_P1_PORT_FILE'], 'w') as f: f.write(str(s.getsockname()[1]) + '\n')
count = 0
while True:
    c, _ = s.accept()
    line = c.recv(512).decode().strip().split(' ', 1)[1]
    if line == 'READY': reply = 'OK protocol=1 fixture=fixture'
    elif line == 'LOAD fixture': reply = 'OK loaded'
    elif line == 'STATUS':
        count += 1
        reply = ('OK ingame=1 camera_available=1 camera_x=10 camera_y=20 speed=8 '
                 'players=1 buildings=1 vehicles=1 game_frame=%d game_seconds=%d assert_fires=0' % (count, count))
    elif line == 'MARK BEGIN': reply = 'OK mark=begin monotonic_ns=1 game_frame=1 game_seconds=1 speed=8'
    elif line == 'MARK END': reply = 'OK mark=end monotonic_ns=2 game_frame=2 game_seconds=2 speed=8'
    elif line == 'QUIT':
        c.sendall(b'OK quitting\n'); c.close(); time.sleep(11); break
    else: reply = 'ERR unexpected'
    c.sendall((reply+'\n').encode()); c.close()
'''

with tempfile.TemporaryDirectory(prefix='en-runner-shutdown-') as tmp:
    root = Path(tmp)
    fixture, fake, out = root/'input.en', root/'fake-game', root/'out'
    fixture.write_bytes(b'fixture')
    fake.write_text(fake_source, encoding='utf-8')
    fake.chmod(0o700)
    cmd = [sys.executable, str(runner), '--executable', str(fake), '--run-dir', str(root),
           '--fixture', str(fixture), '--fixture-sha256', hashlib.sha256(fixture.read_bytes()).hexdigest(),
           '--output', str(out), '--seconds', '1', '--dummy-sdl']
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, start_new_session=True)
    try:
        stdout, stderr = proc.communicate(timeout=35)
    finally:
        if proc.poll() is None:
            os.killpg(proc.pid, signal.SIGKILL)
            proc.communicate(timeout=3)
    result = json.loads((out/'result.json').read_text())
    assert proc.returncode != 0 and result['status'] == 'qualification-failed', (stdout, stderr, result)
    assert 'error' not in result and result['exit_code'] == 0, result
print('clean fake-game exit after 11 seconds accepted; no game qualification claimed')
