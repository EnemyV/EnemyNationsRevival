#!/usr/bin/env python3
"""xwd2png.py — convert an X11 `xwd -root` dump to PNG (full-desktop capture).

The Linux box has xwd (x11-utils) but no xwdtopnm/ImageMagick/ffmpeg/numpy, and
PIL can't read XWD directly. This parses the XWD v7 header and hands the pixel
data to PIL's raw decoder (BGRX, honoring the row stride). Lets the agent capture
the whole desktop as the user sees it (NOT headless — display :0).
Usage: python3 xwd2png.py in.xwd out.png
"""
import struct, sys
from PIL import Image

def main(src, dst):
    data = open(src, "rb").read()
    H = struct.unpack(">25I", data[:100])
    (header_size, ver, pmformat, depth, width, height, xoff, byte_order,
     bunit, bbitorder, bpad, bpp, bytes_per_line, vclass,
     red_mask, green_mask, blue_mask, bits_rgb, cmap_ent, ncolors,
     win_w, win_h, win_x, win_y, win_bw) = H
    pix_off = header_size + ncolors * 12
    msb = (byte_order == 1)
    if bpp == 32:
        rawmode = "XRGB" if msb else "BGRX"
    elif bpp == 24:
        rawmode = "RGB" if msb else "BGR"
    else:
        raise SystemExit(f"unsupported bpp={bpp}")
    if red_mask not in (0xff0000, 0):
        print(f"warn: red_mask={red_mask:#x} byte_order={byte_order} bpp={bpp}; rawmode={rawmode}", file=sys.stderr)
    img = Image.frombuffer("RGB", (width, height), data[pix_off:], "raw", rawmode, bytes_per_line, 1)
    img.save(dst)
    print(f"wrote {dst} {width}x{height} ({rawmode})")

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
