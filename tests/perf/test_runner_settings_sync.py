"""Real socket regression for stale settings and warmup drift; uses a fake game."""
import hashlib
import json
import os
import signal
import subprocess
import sys
import tempfile
from pathlib import Path

if os.name != 'posix':
    raise SystemExit('This fake-game socket regression requires POSIX.')

runner = Path(__file__).resolve().with_name('run_fixture.py')
fake_source = r'''#!/usr/bin/env python3
import os, socket, sys
drift = 'drift' in os.path.basename(sys.argv[0])
s = socket.socket(); s.bind(('127.0.0.1', 0)); s.listen(2)
portfile = os.environ['EN_P1_PORT_FILE']
with open(portfile + '.tmp', 'w') as f: f.write(str(s.getsockname()[1]) + '\n')
os.replace(portfile + '.tmp', portfile)
count = 0
while True:
    c, _ = s.accept()
    line = c.recv(512).decode().strip().split(' ', 1)[1]
    if line == 'READY': reply = 'OK protocol=1 fixture=fixture'
    elif line == 'LOAD fixture': reply = 'OK loaded'
    elif line.startswith('CAMERA '): reply = 'OK camera_x=10 camera_y=20'
    elif line.startswith('SPEED '): reply = 'OK speed=8'
    elif line == 'STATUS':
        count += 1
        speed = 16 if count <= 2 or (drift and count >= 4) else 8
        reply = ('OK ingame=1 camera_available=1 camera_x=10 camera_y=20 speed=%d '
                 'players=1 buildings=1 vehicles=1 game_frame=%d game_seconds=%d assert_fires=0' % (speed, count, count))
    elif line == 'MARK BEGIN':
        print('MARK_BEGIN_RECEIVED', flush=True); reply = 'OK mark=begin monotonic_ns=1 game_frame=1 game_seconds=1 speed=8'
    elif line == 'MARK END': reply = 'OK mark=end monotonic_ns=2 game_frame=2 game_seconds=2 speed=8'
    elif line == 'QUIT':
        c.sendall(b'OK quitting\n'); c.close(); break
    else: reply = 'ERR unexpected'
    c.sendall((reply + '\n').encode()); c.close()
'''

with tempfile.TemporaryDirectory(prefix='en-runner-settings-') as tmp:
    root = Path(tmp)
    fixture = root/'input.en'
    fixture.write_bytes(b'unmodified fixture')
    digest = hashlib.sha256(fixture.read_bytes()).hexdigest()
    for mode in ('stable', 'drift'):
        fake = root/(mode+'-game')
        fake.write_text(fake_source, encoding='utf-8')
        fake.chmod(0o700)
        out = root/(mode+'-out')
        cmd = [sys.executable, str(runner), '--executable', str(fake), '--run-dir', str(root),
               '--fixture', str(fixture), '--fixture-sha256', digest, '--output', str(out),
               '--seconds', '1', '--warmup', '.2', '--dummy-sdl', '--camera-x', '10', '--camera-y', '20', '--speed', '8']
        run = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, start_new_session=True)
        try:
            stdout, stderr = run.communicate(timeout=30)
        except subprocess.TimeoutExpired:
            os.killpg(run.pid, signal.SIGKILL)
            run.communicate()
            raise
        assert run.returncode != 0, (stdout, stderr)  # Stub lacks real qualification markers.
        result = json.loads((out/'result.json').read_text())
        if mode == 'drift':
            assert result.get('error') == 'requested settings changed during warmup', result
            assert 'MARK_BEGIN_RECEIVED' not in (out/'game.log').read_text()
        else:
            assert 'error' not in result, result
            assert result['status'] == 'qualification-failed', result
            assert result['measurement_settings']['speed'] == '8'
            assert result['exit_code'] == 0
            assert 'MARK_BEGIN_RECEIVED' in (out/'game.log').read_text()
        assert hashlib.sha256(fixture.read_bytes()).hexdigest() == digest
print('stale snapshot synchronized; warmup drift rejected; fixture hash preserved')
