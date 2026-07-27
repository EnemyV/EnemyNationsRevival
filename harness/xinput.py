#!/usr/bin/env python3
"""
xinput.py — REAL X11 input injection via XTEST (ctypes -> libXtst/libX11).

Unlike harness_client.py (which posts synthetic SDL events to a specific window),
this moves the ACTUAL cursor and presses REAL buttons through the X server, so it
exercises window-manager-level behavior: click-to-raise, title-bar drag, stacking.
Needed to reproduce/verify the Linux panel drag + raise bugs (the app's manual
drag reads SDL_GetGlobalMouseState, so it only responds to a real cursor move).

No root, no apt: XTEST fake input goes through the X server as the current user.
Works on native X11 and on XWayland (DISPLAY=:0 under a Wayland/Mutter session).

Usage:
  python3 harness/xinput.py move  X Y
  python3 harness/xinput.py click X Y [button]          # button 1=L 2=M 3=R (default 1)
  python3 harness/xinput.py drag  X1 Y1 X2 Y2 [button]  # press, glide, release
  python3 harness/xinput.py pos                          # print current cursor pos
"""
import ctypes, sys, time

x11 = ctypes.CDLL("libX11.so.6")
xtst = ctypes.CDLL("libXtst.so.6")
x11.XOpenDisplay.restype = ctypes.c_void_p
x11.XOpenDisplay.argtypes = [ctypes.c_char_p]

dpy = x11.XOpenDisplay(None)
if not dpy:
    print("ERR: cannot open display", file=sys.stderr); sys.exit(1)

def flush(): x11.XFlush(dpy); x11.XSync(dpy, 0)

def move(x, y):
    # screen -1 = default screen; absolute root coords.
    xtst.XTestFakeMotionEvent(ctypes.c_void_p(dpy), -1, int(x), int(y), 0)
    flush()

def button(b, press):
    xtst.XTestFakeButtonEvent(ctypes.c_void_p(dpy), int(b), 1 if press else 0, 0)
    flush()

def pos():
    # XQueryPointer to read the cursor (root coords).
    root = x11.XDefaultRootWindow(ctypes.c_void_p(dpy))
    rr = ctypes.c_ulong(); cr = ctypes.c_ulong()
    rx = ctypes.c_int(); ry = ctypes.c_int(); wx = ctypes.c_int(); wy = ctypes.c_int()
    mask = ctypes.c_uint()
    x11.XQueryPointer(ctypes.c_void_p(dpy), ctypes.c_ulong(root),
                      ctypes.byref(rr), ctypes.byref(cr),
                      ctypes.byref(rx), ctypes.byref(ry),
                      ctypes.byref(wx), ctypes.byref(wy), ctypes.byref(mask))
    return rx.value, ry.value

def drag(x1, y1, x2, y2, b=1, steps=24):
    move(x1, y1); time.sleep(0.10)
    button(b, True); time.sleep(0.12)
    for i in range(1, steps + 1):
        x = x1 + (x2 - x1) * i / steps
        y = y1 + (y2 - y1) * i / steps
        move(x, y); time.sleep(0.012)
    time.sleep(0.10)
    button(b, False); flush()

if __name__ == "__main__":
    a = sys.argv
    if len(a) < 2:
        print(__doc__); sys.exit(2)
    cmd = a[1]
    if cmd == "move":  move(a[2], a[3])
    elif cmd == "pos": print(pos())
    elif cmd == "click":
        b = int(a[4]) if len(a) > 4 else 1
        move(a[2], a[3]); time.sleep(0.08)
        button(b, True); time.sleep(0.06); button(b, False)
    elif cmd == "drag":
        b = int(a[6]) if len(a) > 6 else 1
        drag(int(a[2]), int(a[3]), int(a[4]), int(a[5]), b)
    else:
        print("unknown cmd", cmd); sys.exit(2)
    print("ok", cmd)
