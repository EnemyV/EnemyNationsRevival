#!/usr/bin/env python3
"""
mac_verify.py — regression checks that inspect WHAT RENDERED, not just whether
the process survived.

Why this exists (2026-08-04, mac): mac_regress.py's only failure signal is a new
.ips crash report or a dead socket. It reported "CLEAN — 0 crashes" through a run
the operator watched render with magenta where transparency should be, a frozen
area map, and heavy lag. A sweep that cannot fail on what it is looking at is a
false-green generator, which is exactly what the project rules forbid.

Three checks, each encoding a failure actually observed:

  COLOURKEY  % of pixels exactly (255,0,255). The game keys magenta as
             transparent (SDL2MainMenu::CreateSurfaceFromDIB:368). Magenta
             surviving to a window surface means the colour key was ignored —
             the signature of running against sdl2-compat instead of real SDL2.
  LIVENESS   the area map captured twice, N seconds apart, while state==playing.
             Byte-identical => the surface stopped being repainted (#63's proven
             instrument: the capture reads the window's OWN surface, so occlusion
             cannot cause this).
  BLANK      % of the single most common colour. A window that is ~entirely one
             colour never drew its content.

Usage (game already up, or use --launch):
  python3 harness/mac_verify.py --port 7071
  python3 harness/mac_verify.py --port 7071 --save _big_out.en --bldgs 8
  python3 harness/mac_verify.py --launch <dir> --save _big_out.en

Exit 0 = all checks passed, 1 = at least one FAIL.
"""
import os, sys, time, socket, struct, subprocess, collections

MAGENTA = (255, 0, 255)
COLOURKEY_MAX_PCT = 0.05   # any real magenta area is far above this
BLANK_MAX_PCT     = 99.0   # a window that is ~all one colour drew nothing
LIVENESS_GAP_S    = 8

def cmd(args, port, timeout=25):
    s = socket.socket(); s.settimeout(timeout)
    try:
        s.connect(("127.0.0.1", port))
    except OSError:
        return "ERR_NO_SOCKET"
    s.sendall((" ".join(str(a) for a in args) + "\n").encode())
    time.sleep(0.2)
    data = b""
    try:
        while True:
            c = s.recv(65536)
            if not c: break
            data += c
    except socket.timeout:
        pass
    s.close()
    return data.decode(errors="replace").strip()

def decode_bmp(path):
    """32-bpp BITMAPINFOHEADER BMP -> (w, h, [(r,g,b), ...]). Same layout
    bmp2png.py handles; sips/Pillow choke on these."""
    d = open(path, "rb").read()
    off = struct.unpack("<I", d[10:14])[0]
    w   = struct.unpack("<i", d[18:22])[0]
    h   = struct.unpack("<i", d[22:26])[0]
    bpp = struct.unpack("<H", d[28:30])[0]
    if bpp != 32:
        raise ValueError("expected 32-bpp BMP, got %d" % bpp)
    H = abs(h)
    px = d[off:]
    out = []
    for y in range(H):
        row = px[y * w * 4: (y + 1) * w * 4]
        for x in range(0, len(row), 4):
            out.append((row[x + 2], row[x + 1], row[x]))   # BGRA -> RGB
    return w, H, out

def analyse(pixels):
    n = len(pixels) or 1
    counts = collections.Counter(pixels)
    magenta = counts.get(MAGENTA, 0)
    top_col, top_n = counts.most_common(1)[0]
    return (100.0 * magenta / n), (100.0 * top_n / n), top_col

class Report:
    def __init__(self): self.fails = 0; self.lines = []
    def check(self, ok, name, detail):
        tag = "[ok]  " if ok else "[FAIL]"
        if not ok: self.fails += 1
        self.lines.append(f"{tag} {name} — {detail}")
        print(f"{tag} {name} — {detail}", flush=True)

def grab(port, win, tmp):
    r = cmd(["shotid", win, tmp], port)
    return os.path.exists(tmp) and r.startswith("ok")

def main():
    a = sys.argv[1:]
    port  = int(next((a[i+1] for i,x in enumerate(a) if x=="--port"), os.environ.get("EN_HARNESS_PORT", 7070)))
    save  = next((a[i+1] for i,x in enumerate(a) if x=="--save"), None)
    nb    = int(next((a[i+1] for i,x in enumerate(a) if x=="--bldgs"), 6))
    ldir  = next((a[i+1] for i,x in enumerate(a) if x=="--launch"), None)
    shots = next((a[i+1] for i,x in enumerate(a) if x=="--shots"), None)
    if shots: os.makedirs(shots, exist_ok=True)
    tmp = "/tmp/_verify.bmp"
    rep = Report()
    proc = None

    if ldir:
        env = dict(os.environ, EN_HARNESS="1", EN_HARNESS_PORT=str(port),
                   SDL_AUDIODRIVER="dummy", SDL_RENDER_DRIVER="opengl")
        proc = subprocess.Popen(["./enations"], cwd=ldir, env=env,
                                stdout=open("/tmp/mac_verify_game.log","w"),
                                stderr=subprocess.STDOUT)
        for _ in range(160):
            if cmd(["pstats"], port) != "ERR_NO_SOCKET": break
            if proc.poll() is not None:
                print("game died before socket; see /tmp/mac_verify_game.log"); sys.exit(2)
            time.sleep(0.25)

    if cmd(["pstats"], port) == "ERR_NO_SOCKET":
        print(f"no game on :{port}"); sys.exit(2)

    # Report which SDL is actually loaded — the difference between a real result
    # and a wasted run (real SDL2 == 2.32.10-ish; sdl2-compat reports 2.32.70).
    if ldir:
        exe = os.path.join(ldir, "enations")
        try:
            out = subprocess.run(["otool","-L",exe], capture_output=True, text=True).stdout
            for ln in out.splitlines():
                if "SDL2-2.0.0" in ln: print("SDL linkage:", ln.strip())
        except Exception: pass

    if save:
        r = cmd(["load", save], port, timeout=90)
        print("load:", r)
        for _ in range(40):
            if "playing" in cmd(["gamestate"], port): break
            time.sleep(1.0)

    st = cmd(["gamestate"], port)
    rep.check("playing" in st, "in-game", st[:90])

    wins = cmd(["wins"], port)
    print("windows:\n  " + "\n  ".join(wins.splitlines()))
    ids = {}
    for ln in wins.splitlines():
        if ":" not in ln: continue
        wid = ln.split(":")[0].strip()
        name = ln.split('"')[1] if '"' in ln else ""
        ids[name] = wid

    # --- per-window COLOURKEY + BLANK ---
    for name, wid in ids.items():
        if not grab(port, wid, tmp):
            rep.check(False, f"capture {name!r}", "shotid failed"); continue
        try:
            w, h, px = decode_bmp(tmp)
        except Exception as e:
            rep.check(False, f"decode {name!r}", str(e)); continue
        mag, flat, col = analyse(px)
        if shots:
            subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "bmp2png.py"),
                            tmp, os.path.join(shots, f"win_{wid}_{name.replace(' ','_')}.png")],
                           capture_output=True)
        rep.check(mag <= COLOURKEY_MAX_PCT, f"COLOURKEY {name!r} ({w}x{h})",
                  f"{mag:.3f}% magenta (limit {COLOURKEY_MAX_PCT}%)")
        rep.check(flat <= BLANK_MAX_PCT, f"BLANK     {name!r}",
                  f"{flat:.1f}% single colour rgb{col}")

    # --- LIVENESS: area map must change while playing ---
    amid = ids.get("Area Map")
    if amid:
        p1 = "/tmp/_live1.bmp"; p2 = "/tmp/_live2.bmp"
        grab(port, amid, p1); time.sleep(LIVENESS_GAP_S); grab(port, amid, p2)
        try:
            b1 = open(p1,"rb").read(); b2 = open(p2,"rb").read()
            diff = sum(1 for x, y in zip(b1, b2) if x != y)
            rep.check(diff > 0, f"LIVENESS  'Area Map' over {LIVENESS_GAP_S}s",
                      f"{diff} bytes changed (0 = surface stopped repainting)")
        except Exception as e:
            rep.check(False, "LIVENESS 'Area Map'", str(e))

    # --- building info windows: the path the operator saw magenta in ---
    mine = [l.split()[0] for l in cmd(["units"], port).splitlines()
            if len(l.split()) >= 5 and l.split()[3] == "building" and l.split()[4] == "me"]
    print(f"my buildings: {len(mine)} (sampling {min(nb,len(mine))})")
    for bid in mine[:nb]:
        if not cmd(["showinfo", bid], port).startswith("ok"): continue
        time.sleep(0.6)
        w2 = cmd(["wins"], port)
        info = [l for l in w2.splitlines() if l.split(":")[0] not in ids.values()]
        if not info: continue
        iw = info[-1].split(":")[0].strip()
        iname = info[-1].split('"')[1] if '"' in info[-1] else iw
        if not grab(port, iw, tmp): continue
        try:
            _, _, px = decode_bmp(tmp)
        except Exception: continue
        mag, flat, col = analyse(px)
        if shots:
            subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "bmp2png.py"),
                            tmp, os.path.join(shots, f"info_{bid}_{iname.replace(' ','_')}.png")],
                           capture_output=True)
        rep.check(mag <= COLOURKEY_MAX_PCT, f"COLOURKEY info {iname!r} (bldg {bid})",
                  f"{mag:.3f}% magenta")

    print("\n=== %d check(s) FAILED ===" % rep.fails if rep.fails else "\n=== ALL CHECKS PASSED ===")
    if proc:
        cmd(["quit"], port); time.sleep(1)
        try: proc.kill()
        except Exception: pass
    sys.exit(1 if rep.fails else 0)

if __name__ == "__main__":
    main()
