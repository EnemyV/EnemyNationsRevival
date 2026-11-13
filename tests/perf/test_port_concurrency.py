"""OS-assigned ports stay bound and distinct across concurrent listeners."""
import os
from pathlib import Path
import secrets
import socket
import subprocess
import sys
import tempfile
import time

with tempfile.TemporaryDirectory() as tmp:
    processes = []
    try:
        for i in range(8):
            token = secrets.token_hex(16)
            file = Path(tmp) / str(i)
            env = dict(os.environ, EN_P1_PORT='0', EN_P1_PORT_FILE=str(file),
                       EN_P1_TOKEN=token, EN_P1_FIXTURE='fixture')
            processes.append((subprocess.Popen([sys.argv[1]], env=env), file, token))
        deadline = time.monotonic() + 4
        while any(not f.exists() for _, f, _ in processes):
            if time.monotonic() > deadline or any(p.poll() is not None for p, _, _ in processes):
                raise AssertionError('listener publication failed')
            time.sleep(.01)
        ports = [int(f.read_text()) for _, f, _ in processes]
        assert len(set(ports)) == len(ports)
        for (p, _, token), port in zip(processes, ports):
            with socket.create_connection(('127.0.0.1', port), timeout=2) as s:
                s.sendall((token + ' QUIT\n').encode())
                assert s.recv(512) == b'OK quitting\n'
            assert p.wait(timeout=3) == 0
        print('eight simultaneous listeners published distinct bound ports')
    finally:
        for p, _, _ in processes:
            if p.poll() is None:
                p.terminate()
                try: p.wait(timeout=2)
                except subprocess.TimeoutExpired: p.kill(); p.wait()
