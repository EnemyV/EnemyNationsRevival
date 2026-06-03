#!/usr/bin/env python3
"""
terrainbake — offline TGA -> PNG terrain tile baker for the GPU terrain path.

Input : EnemyNationsRevival/enations/data/TERRAIN/<TYPE>/<variant>/<file>.tga
        (24-bit uncompressed masters, 128x64, 2:1 iso diamonds).
Output: <out>/<type>_<variant>_<stem>_z{0..3}.png  (RGBA)  +  manifest.json

Pipeline per tile (mirrors tools/sprite/terrain.cpp, with two deliberate
modernizations called out in the GPU terrain plan):
  1. Load 24-bit TGA -> RGBA.  magenta key (255,0,255) and the (254,0,254)
     temp-art kludge -> alpha 0.  (original kept magenta; we carry real alpha)
  2. Square() corner-fill: copy the diamond's four triangles into the opposite
     rect corners so downscale/bilinear never averages in the key colour.
     (faithful port of TerrainImage::Square, carrying alpha through the copies)
  3. Per-zoom LOD: emit z0=128x64, z1=64x32, z2=32x16, z3=16x8 via an
     alpha-weighted box average (== the original ScaleDownAvg for opaque tiles;
     for overlays it averages RGB only over covered texels so edges don't fringe).
  We deliberately SKIP Pack (runtime-layout opt) and Shade (per-vertex on GPU now).

Manifest records, per tile: type, variant, direction, damage, the 4 png paths,
and the render flags {shade, drawVert, transparent} the runtime needs.

Pure stdlib (struct/zlib) — no PIL, no build step.  Re-run any time.
"""
import struct, zlib, sys, os, json, re

# ---- terrain type render flags (from enations_latest/src/terrain.cpp CHex::Draw) ----
# bDrawVert = road || resources          (U/V axes swapped vs the diamond)
# bShade    = NOT (river|coastline|swamp|ocean|resources|lake)
# transparent (colour-key -> alpha): the overlay types that sit over base terrain.
DRAWVERT_TYPES    = {"road", "resources"}
NO_SHADE_TYPES    = {"river", "coastline", "swamp", "ocean", "resources", "lake"}
TRANSPARENT_TYPES = {"road", "coastline", "river", "resources"}

# directory name -> canonical engine type name (terrain.h CHex type enum)
DIR_TO_TYPE = {
    "DESERT": "desert", "HILL": "hill", "MOUNTAIN": "mountain", "PLAIN": "plain",
    "ROUGH": "rough", "city": "city", "coastlne": "coastline", "fields": "fields",
    "lake": "lake", "ocean": "ocean", "resource": "resources", "river": "river",
    "road": "road", "swamp": "swamp",
}

TILE_W, TILE_H = 128, 64
KEYS = [(255, 0, 255), (254, 0, 254)]   # magenta transparency + temp-art kludge


def read_tga(path):
    with open(path, "rb") as f:
        d = f.read()
    idlen, cmaptype, imgtype = d[0], d[1], d[2]
    xorg, yorg, w, h = struct.unpack_from("<HHHH", d, 8)
    bpp, desc = d[16], d[17]
    rle = (imgtype == 10)                 # 2 = uncompressed true-colour, 10 = RLE
    if imgtype not in (2, 10) or bpp not in (24, 32):
        raise ValueError(f"{path}: unsupported TGA (imgtype={imgtype} bpp={bpp})")
    off = 18 + idlen
    if cmaptype != 0:
        cmap_len, cmap_entsz = struct.unpack_from("<HB", d, 5)
        off += cmap_len * ((cmap_entsz + 7) // 8)
    top_origin = bool(desc & 0x20)
    bpp_b = bpp // 8
    npix = w * h
    if rle:                               # decode RLE packets -> flat BGR(A) stream
        pix = bytearray()
        i = off
        while len(pix) < npix * bpp_b:
            hdr = d[i]; i += 1
            cnt = (hdr & 0x7f) + 1
            if hdr & 0x80:                # run packet: one pixel x cnt
                pix += d[i:i + bpp_b] * cnt; i += bpp_b
            else:                         # raw packet: cnt pixels
                pix += d[i:i + bpp_b * cnt]; i += bpp_b * cnt
    else:
        pix = d[off:]
    rowbytes = w * bpp_b
    rgba = bytearray(w * h * 4)
    for y in range(h):
        srcy = y if top_origin else (h - 1 - y)
        row = pix[srcy * rowbytes:(srcy + 1) * rowbytes]
        for x in range(w):
            p = row[x * bpp_b:x * bpp_b + bpp_b]
            b, g, r = p[0], p[1], p[2]           # TGA is BGR(A)
            o = (y * w + x) * 4
            if (r, g, b) in KEYS:
                rgba[o:o + 4] = b"\x00\x00\x00\x00"   # key -> transparent
            else:
                rgba[o] = r; rgba[o + 1] = g; rgba[o + 2] = b; rgba[o + 3] = 255
    return w, h, rgba


def _cp(buf, dst_idx, src_idx, n, w):
    """copy n RGBA pixels (used by square corner-fill); dst/src are pixel indices."""
    d0 = dst_idx * 4
    s0 = src_idx * 4
    buf[d0:d0 + n * 4] = bytes(buf[s0:s0 + n * 4])


def square(buf, w=TILE_W, h=TILE_H):
    """Faithful port of TerrainImage::Square — fill the 4 rect corners from the
    diamond's opposite triangles so the key colour is never sampled at edges."""
    hw, hh = w // 2, h // 2            # 64, 32
    width = hw - 2                     # 62
    for row in range(0, hh - 1):       # 0..30
        # top-left  <- bottom-half centre
        _cp(buf, row * w,                (row + hh + 1) * w + hw,          width, w)
        # top-right <- bottom-half centre
        _cp(buf, row * w + (w - width),  (row + hh + 1) * w + (hw - width), width, w)
        # bottom-left <- top-half centre
        _cp(buf, (row + hh + 1) * w,     row * w + hw,                      hw - width, w)
        # bottom-right <- top-half centre
        _cp(buf, (row + hh + 1) * w + (hw + width), row * w + width,        hw - width, w)
        width -= 2


def scale_down(buf, w, h, scale):
    """alpha-weighted box average -> (nw, nh, RGBA bytearray)."""
    nw, nh = w // scale, h // scale
    out = bytearray(nw * nh * 4)
    for ny in range(nh):
        for nx in range(nw):
            r = g = b = a = cov = 0
            for dy in range(scale):
                for dx in range(scale):
                    o = ((ny * scale + dy) * w + (nx * scale + dx)) * 4
                    av = buf[o + 3]
                    a += av
                    if av:                      # only opaque texels feed RGB
                        r += buf[o]; g += buf[o + 1]; b += buf[o + 2]; cov += 1
            o = (ny * nw + nx) * 4
            n = scale * scale
            if cov:
                out[o] = r // cov; out[o + 1] = g // cov; out[o + 2] = b // cov
            out[o + 3] = a // n
    return nw, nh, out


def write_png(path, w, h, rgba):
    raw = bytearray()
    for y in range(h):
        raw.append(0)                            # filter: none
        raw += rgba[y * w * 4:(y + 1) * w * 4]

    def chunk(typ, data):
        return (struct.pack(">I", len(data)) + typ + data +
                struct.pack(">I", zlib.crc32(typ + data) & 0xffffffff))

    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)   # 8-bit, colour-type 6 (RGBA)
    out = sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(out)


def parse_stem(stem):
    """'aa010000' -> (direction, damage). dir = first 2 chars (AA/AC/AE/AG);
    trailing digit = damage (always 0 in CHex::Draw, but recorded)."""
    s = stem.lower()
    direction = s[:2] if len(s) >= 2 else "aa"
    m = re.search(r"(\d)\D*$", s)
    damage = int(m.group(1)) if m else 0
    return direction, damage


def main():
    if len(sys.argv) < 3:
        print("usage: terrainbake.py <TERRAIN_dir> <out_dir>")
        return 1
    src_root, out_dir = sys.argv[1], sys.argv[2]
    os.makedirs(out_dir, exist_ok=True)

    manifest = {"tile_w": TILE_W, "tile_h": TILE_H, "zooms": [1, 2, 4, 8], "tiles": []}
    count = 0
    for dirname, typ in sorted(DIR_TO_TYPE.items()):
        type_dir = os.path.join(src_root, dirname)
        if not os.path.isdir(type_dir):
            print(f"  skip (missing): {dirname}")
            continue
        is_transparent = typ in TRANSPARENT_TYPES
        for variant in sorted(os.listdir(type_dir)):
            vdir = os.path.join(type_dir, variant)
            if not os.path.isdir(vdir):
                continue
            for fn in sorted(os.listdir(vdir)):
                if not fn.lower().endswith(".tga"):
                    continue
                stem = os.path.splitext(fn)[0]
                direction, damage = parse_stem(stem)
                w, h, rgba = read_tga(os.path.join(vdir, fn))
                if (w, h) != (TILE_W, TILE_H):
                    print(f"  WARN {dirname}/{variant}/{fn}: {w}x{h} (expected {TILE_W}x{TILE_H})")
                square(rgba, w, h)
                # Key the output by the FULL source stem (lowercased) so every
                # distinct source tile survives — the char-4 letter (A..H) in
                # coastline/river/swamp names is a distinct shape, not a frame,
                # so collapsing to dir+damage was lossy. stem has no underscores.
                base = f"{typ}_{variant}_{stem.lower()}"
                pngs = []
                cw, ch, cbuf = w, h, rgba
                for zi, scale in enumerate(manifest["zooms"]):
                    if scale == 1:
                        zw, zh, zbuf = w, h, rgba
                    else:
                        zw, zh, zbuf = scale_down(rgba, w, h, scale)
                    name = f"{base}_z{zi}.png"
                    write_png(os.path.join(out_dir, name), zw, zh, zbuf)
                    pngs.append(name)
                manifest["tiles"].append({
                    "type": typ, "variant": variant, "direction": direction,
                    "damage": damage, "stem": stem.lower(), "png": pngs,
                    "shade": typ not in NO_SHADE_TYPES,
                    "drawVert": typ in DRAWVERT_TYPES,
                    "transparent": is_transparent,
                })
                count += 1
        print(f"  {dirname:9s} -> {typ}")

    with open(os.path.join(out_dir, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=1)
    print(f"baked {count} tiles ({count * len(manifest['zooms'])} PNGs) -> {out_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
