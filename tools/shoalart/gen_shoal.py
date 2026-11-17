#!/usr/bin/env python3
"""Generate the shoal (shallow water over sand) water tiles: <river|lake|ocean>_7_*.

Each frame is the matching variant-0 water frame with a soft sand tint added, so the
wave animation stays in sync with neighbouring water. Writes into the baked terrain set.
Needs ImageMagick (`magick`). Run from anywhere: python3 tools/shoalart/gen_shoal.py
"""
import math, subprocess, pathlib

SRC = pathlib.Path(__file__).resolve().parents[2] / 'enations_latest' / 'data' / 'terrain_gpu'
OUT = SRC

def run(*args, data=None):
    return subprocess.run(args, input=data, stdout=subprocess.PIPE, check=True).stdout

def smooth(a, b, x):
    t = max(0, min(1, (x-a)/(b-a)))
    return t*t*(3-2*t)

def strength(x, y):
    # Coordinates are in the original 128x64 tile, aligned across all zooms.
    u, v = (x-63.5)/64, (y-31.5)/32
    edge = 1 - abs(u) - abs(v)
    fade = smooth(0.04, 0.30, edge)
    # A broken, meandering submerged ridge and a smaller companion bank.
    ridge = math.exp(-(((u+0.10)/0.53)**2 + ((v-0.13*math.sin(u*4.5)+0.03)/0.29)**2)*1.9)
    spur = math.exp(-(((u-0.31)/0.24)**2 + ((v+0.19)/0.18)**2)*2.0)
    irregular = 0.82 + 0.12*math.sin(x*0.22+y*0.31) + 0.08*math.sin(x*0.43-y*0.23) + 0.06*math.sin(x*0.77+y*0.53)
    return min(1, fade * (ridge*irregular + spur*0.48) * 1.65)

for kind in ('river', 'lake', 'ocean'):
    for frame in 'abcdefgh':
        for z in range(4):
            w, h = 128 >> z, 64 >> z
            source = SRC / f'{kind}_0_aa00{frame}000_z{z}.png'
            pixels = bytearray(run('magick', str(source), '-depth', '8', 'rgba:-'))
            assert len(pixels) == w*h*4
            for y in range(h):
                for x in range(w):
                    # Average coverage for small sprites to preserve the feature at z3.
                    scale = 1 << z
                    s = sum(strength((x+(i+.5)/4)*scale-.5, (y+(j+.5)/4)*scale-.5) for i in range(4) for j in range(4))/16
                    p = 4*(y*w+x)
                    # Add warm sand light while retaining all original wave contrast.
                    for c, delta in enumerate((68, 52, 18)):
                        pixels[p+c] = min(255, round(pixels[p+c] + delta*s))
            dest = OUT / f'{kind}_7_aa00{frame}000_z{z}.png'
            run('magick', '-size', f'{w}x{h}', '-depth', '8', 'rgba:-', str(dest), data=pixels)
print('Generated 96 shoal tiles in', OUT)
