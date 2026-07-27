#!/usr/bin/env python3
# Raw uinput virtual ABSOLUTE mouse — injects REAL evdev input (via /dev/uinput) so it
# drives Mutter's Wayland pointer grab (e.g. a _NET_WM_MOVERESIZE window move), which
# XTEST (XTestFakeMotion) cannot do. Needs write access to /dev/uinput (ACL grants it).
#
# Usage:
#   python3 uinput_mouse.py drag X1 Y1 X2 Y2 [steps] [holdms]
#   python3 uinput_mouse.py move X Y
#   python3 uinput_mouse.py click X Y
import ctypes, struct, os, sys, time, fcntl

SCREEN_W, SCREEN_H = 1741, 1081   # matches xdpyinfo dimensions on :0

libc = ctypes.CDLL("libc.so.6", use_errno=True)

EV_SYN, EV_KEY, EV_REL, EV_ABS = 0, 1, 2, 3
SYN_REPORT = 0
ABS_X, ABS_Y = 0, 1
BTN_LEFT = 0x110
ABS_CNT = 64

# _IOW('U', n, int) => dir=1(write)<<30 | size(4)<<16 | 'U'(0x55)<<8 | n
def _iow(n): return (1 << 30) | (4 << 16) | (ord('U') << 8) | n
def _io(n):  return (ord('U') << 8) | n
UI_SET_EVBIT  = _iow(100)
UI_SET_KEYBIT = _iow(101)
UI_SET_ABSBIT = _iow(103)
UI_DEV_CREATE  = _io(1)
UI_DEV_DESTROY = _io(2)

class UserDev(ctypes.Structure):
    _fields_ = [("name", ctypes.c_char * 80),
                ("id_bustype", ctypes.c_uint16),
                ("id_vendor",  ctypes.c_uint16),
                ("id_product", ctypes.c_uint16),
                ("id_version", ctypes.c_uint16),
                ("ff_effects_max", ctypes.c_uint32),
                ("absmax",  ctypes.c_int32 * ABS_CNT),
                ("absmin",  ctypes.c_int32 * ABS_CNT),
                ("absfuzz", ctypes.c_int32 * ABS_CNT),
                ("absflat", ctypes.c_int32 * ABS_CNT)]

def open_dev():
    fd = os.open("/dev/uinput", os.O_WRONLY | os.O_NONBLOCK)
    fcntl.ioctl(fd, UI_SET_EVBIT, EV_KEY)
    fcntl.ioctl(fd, UI_SET_KEYBIT, BTN_LEFT)
    fcntl.ioctl(fd, UI_SET_EVBIT, EV_ABS)
    fcntl.ioctl(fd, UI_SET_ABSBIT, ABS_X)
    fcntl.ioctl(fd, UI_SET_ABSBIT, ABS_Y)
    fcntl.ioctl(fd, UI_SET_EVBIT, EV_SYN)
    ud = UserDev()
    ud.name = b"en-virtual-mouse"
    ud.id_bustype = 0x03  # BUS_USB
    ud.id_vendor, ud.id_product, ud.id_version = 0x1234, 0x5678, 1
    ud.absmax[ABS_X] = SCREEN_W; ud.absmax[ABS_Y] = SCREEN_H
    os.write(fd, bytes(ud))
    fcntl.ioctl(fd, UI_DEV_CREATE)
    time.sleep(1.0)   # give libinput/Mutter time to enumerate the new device
    return fd

def emit(fd, etype, code, value):
    # struct input_event: timeval(16) + u16 type + u16 code + s32 value = 24 bytes
    os.write(fd, struct.pack("llHHi", 0, 0, etype, code, value))

def syn(fd): emit(fd, EV_SYN, SYN_REPORT, 0)

def move(fd, x, y):
    emit(fd, EV_ABS, ABS_X, int(x)); emit(fd, EV_ABS, ABS_Y, int(y)); syn(fd)

def button(fd, down):
    emit(fd, EV_KEY, BTN_LEFT, 1 if down else 0); syn(fd)

def close_dev(fd):
    try: fcntl.ioctl(fd, UI_DEV_DESTROY)
    except Exception: pass
    os.close(fd)

def drag(x1, y1, x2, y2, steps=50, holdms=600):
    fd = open_dev()
    move(fd, x1, y1); time.sleep(0.2)
    button(fd, True); time.sleep(holdms/1000.0)   # hold so the game hands off to Mutter
    for i in range(1, steps+1):
        move(fd, x1 + (x2-x1)*i/steps, y1 + (y2-y1)*i/steps); time.sleep(0.02)
    time.sleep(0.3)
    button(fd, False); time.sleep(0.2)
    close_dev(fd)

if __name__ == "__main__":
    a = sys.argv
    if a[1] == "drag":
        steps = int(a[6]) if len(a) > 6 else 50
        hold  = int(a[7]) if len(a) > 7 else 600
        drag(int(a[2]), int(a[3]), int(a[4]), int(a[5]), steps, hold)
    elif a[1] == "move":
        fd = open_dev(); move(fd, int(a[2]), int(a[3])); time.sleep(0.2); close_dev(fd)
    elif a[1] == "click":
        fd = open_dev(); move(fd, int(a[2]), int(a[3])); time.sleep(0.2)
        button(fd, True); time.sleep(0.1); button(fd, False); time.sleep(0.2); close_dev(fd)
    print("ok", a[1])
