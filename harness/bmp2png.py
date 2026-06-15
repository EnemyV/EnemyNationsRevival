#!/usr/bin/env python3
"""
bmp2png.py — convert a 32-bit BMP (as written by the harness `shot`/`shotid`)
to PNG, with optional nearest-neighbour downscale.

Why this exists: on macOS `sips` and Pillow both choke on the 32-bit
BITMAPINFOHEADER BMPs SDL_SaveBMP produces, so we decode + write PNG by hand
using only the standard library (struct + zlib).

Usage:
  python3 bmp2png.py in.bmp out.png [maxwidth]
"""
import sys, struct, zlib

def bmp2png(src, dst, maxw=1000):
    d = open(src, "rb").read()
    off = struct.unpack("<I", d[10:14])[0]
    w = struct.unpack("<i", d[18:22])[0]
    h = struct.unpack("<i", d[22:26])[0]
    bpp = struct.unpack("<H", d[28:30])[0]
    topdown = h < 0
    H = abs(h)
    assert bpp == 32, "expected 32-bpp BMP, got %d" % bpp
    rowsize = w * 4
    px = d[off:]
    rows = []
    for y in range(H):
        sy = y if topdown else (H - 1 - y)
        row = px[sy * rowsize: sy * rowsize + rowsize]
        out = bytearray(w * 4)
        for x in range(w):
            b = row[x*4]; g = row[x*4+1]; r = row[x*4+2]
            out[x*4] = r; out[x*4+1] = g; out[x*4+2] = b; out[x*4+3] = 255
        rows.append(bytes(out))
    if w > maxw:
        scale = w / maxw
        nw = maxw; nh = int(H / scale)
        nrows = []
        for ny in range(nh):
            srow = rows[int(ny * scale)]
            o = bytearray(nw * 4)
            for nx in range(nw):
                sx = int(nx * scale)
                o[nx*4:nx*4+4] = srow[sx*4:sx*4+4]
            nrows.append(bytes(o))
        rows = nrows; w = nw; H = nh
    raw = bytearray()
    for r in rows:
        raw.append(0); raw += r
    comp = zlib.compress(bytes(raw), 6)
    def chunk(typ, data):
        c = typ + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xffffffff)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, H, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", comp)
    png += chunk(b"IEND", b"")
    open(dst, "wb").write(png)
    print("wrote", dst, w, "x", H)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__); sys.exit(2)
    bmp2png(sys.argv[1], sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 1000)
