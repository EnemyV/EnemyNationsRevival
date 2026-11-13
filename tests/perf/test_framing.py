"""Framing tests for bounded line responses, without a game."""
import socket
import threading
import time
from run_fixture import read_response


def exchange(chunks, expected=None):
    a, b = socket.socketpair()
    def send():
        try:
            for chunk in chunks:
                b.sendall(chunk)
                time.sleep(.01)
        finally:
            b.close()
    thread = threading.Thread(target=send)
    thread.start()
    try:
        if expected is None:
            try:
                read_response(a)
            except RuntimeError:
                pass
            else:
                raise AssertionError("invalid response accepted")
        else:
            assert read_response(a) == expected
    finally:
        a.close()
        thread.join()

exchange([b"O", b"K mark=", b"begin\n"], "OK mark=begin")
exchange([b"truncated"])
exchange([b"x" * 512])
print("fragmented and invalid response framing passed")
