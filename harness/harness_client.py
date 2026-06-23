#!/usr/bin/env python3
"""
harness_client.py — thin client for the in-game control socket (en_harness.h).

The game, launched with EN_HARNESS=1, listens on 127.0.0.1:EN_HARNESS_PORT
(default 7070). This sends one command per connection (newline-terminated) and
prints the reply. Used to screenshot and drive the game on macOS/Linux where the
Windows PowerShell harness (screenshot.ps1/click.ps1) does not apply.

Protocol (see harness/control_socket.cpp):
  shot <path>                 -> grab the active/largest window to <path> (BMP)
  shotid <winid> <path>       -> grab a specific SDL window id (BMP)
  click  <x> <y> [right]      -> click the active window at client px (x,y)
  clickid <winid> <x> <y> [right]   -> click a specific window
  dblclickid <winid> <x> <y>  -> double-click a specific window
  move   <x> <y>              -> mouse-move the active window to (x,y)
  key    <sdl_keycode>        -> press an SDL keycode once
  text   <string>            -> type literal text
  quit                        -> ask the game to quit

Window ids in-game (single player): 1 = main window (toolbar/UI/clock),
5 = area map (terrain+units), 6 = radar/minimap. Prefer `shotid`.

Capture is BMP. Convert with harness/bmp2png.py (sips/Pillow are unreliable for
these 32-bit BMPs). The main window and detached panels dump their CPU
back-surface (reliable); the area-map terrain is a GPU window, so launch with
SDL_RENDER_DRIVER=opengl for its read-back to work.

Usage:
  python3 harness_client.py shot /tmp/s.bmp
  python3 harness_client.py shotid 5 /tmp/area.bmp
  python3 harness_client.py clickid 5 410 340 ; python3 harness_client.py dblclickid 5 410 340
  EN_HARNESS_PORT=7071 python3 harness_client.py click 314 405
"""
import os, socket, sys, time

def cmd(args, port=None, timeout=15):
    port = port or int(os.environ.get("EN_HARNESS_PORT", "7070"))
    s = socket.socket(); s.settimeout(timeout); s.connect(("127.0.0.1", port))
    s.sendall((" ".join(args) + "\n").encode())
    time.sleep(0.2)
    data = b""
    try:
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            data += chunk
    except socket.timeout:
        pass
    s.close()
    return data.decode(errors="replace").strip()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(2)
    # save/load do heavy serialize+compress+disk work and can take well over the
    # default 15s; give them a long client socket timeout so the client doesn't
    # bail before the server replies.
    _timeout = 120 if sys.argv[1] in ("save", "load") else 15
    print(cmd(sys.argv[1:], timeout=_timeout))
