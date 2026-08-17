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
import os, re, sys, time, socket, struct, subprocess, collections

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
    exe   = next((a[i+1] for i,x in enumerate(a) if x=="--exe"), "./enations")
    shots = next((a[i+1] for i,x in enumerate(a) if x=="--shots"), None)
    if shots: os.makedirs(shots, exist_ok=True)
    tmp = "/tmp/_verify.bmp"
    rep = Report()
    proc = None

    if ldir:
        env = dict(os.environ, EN_HARNESS="1", EN_HARNESS_PORT=str(port),
                   SDL_AUDIODRIVER="dummy", SDL_RENDER_DRIVER="opengl")
        proc = subprocess.Popen([exe], cwd=ldir, env=env,
                                stdout=open("/tmp/mac_verify_game.log","w"),
                                stderr=subprocess.STDOUT)
        for _ in range(160):
            if cmd(["pstats"], port) != "ERR_NO_SOCKET": break
            if proc.poll() is not None:
                print("game died before socket; see /tmp/mac_verify_game.log"); sys.exit(2)
            time.sleep(0.25)

    if cmd(["pstats"], port) == "ERR_NO_SOCKET":
        print(f"no game on :{port}"); sys.exit(2)

    # HARD CHECK, not a printed line: on the sdl2-compat shim the colour key is silently
    # ignored and every DIB sprite renders on magenta, so the entire run is invalid rather
    # than merely odd. It used to just print the linkage, which is easy to scroll past — I
    # scrolled past it once and "discovered" magenta on a build I had just patched.
    if ldir:
        try:
            sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
            from mac_sdlcheck import check_sdl
            ok, detail = check_sdl(os.path.join(ldir, os.path.basename(exe)))
            rep.check(ok, "SDL linkage", detail)
        except Exception as e:
            rep.check(False, "SDL linkage", f"could not be determined: {e}")

    if save:
        # Wait for the MENU, not merely for the socket. The harness starts listening
        # early, but the intro video blocks the service loop for a long time (~87 s
        # measured on this node), so a launch that only waits for `pstats` to answer
        # will fire `load` while PlayVideo still owns the main thread and get back
        # "err load failed" — a spurious failure that looks like a bad save.
        # mac_regress.py's launch_game() has exactly this bug, which is why the FIRST
        # save in a sweep can fail while later ones load fine.
        for _ in range(80):
            if "menu" in cmd(["gamestate"], port): break
            time.sleep(3)
        else:
            print("WARNING: never reached the menu; load will probably fail")
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
    #
    # A byte-identical pair at a FIXED sampling gap cannot distinguish "rendering
    # stopped" from "the game is merely slower than my sampling rate" — I called a
    # throughput collapse a permanent freeze on exactly that mistake (2026-08-04).
    # So a zero is not a verdict, it is a trigger for two controls:
    #   1. does `gamestate elapsed` advance? (is the sim running at all)
    #   2. does an explicit `pan` force a redraw? (is the renderer alive)
    # Only "no repaint AND pan does nothing" is a genuine freeze.
    amid = ids.get("Area Map")
    if amid:
        p1 = "/tmp/_live1.bmp"; p2 = "/tmp/_live2.bmp"
        e0 = cmd(["gamestate"], port).split()
        grab(port, amid, p1); time.sleep(LIVENESS_GAP_S); grab(port, amid, p2)
        try:
            b1 = open(p1,"rb").read(); b2 = open(p2,"rb").read()
            diff = sum(1 for x, y in zip(b1, b2) if x != y)
            if diff > 0:
                rep.check(True, f"LIVENESS  'Area Map' over {LIVENESS_GAP_S}s",
                          f"{diff} bytes changed")
            else:
                e1 = cmd(["gamestate"], port).split()
                def el(t): return int(t[t.index("elapsed")+1]) if "elapsed" in t else -1
                simadv = el(e1) - el(e0)
                cmd(["pan", amid, "400", "300"], port); time.sleep(2.5)
                grab(port, amid, p1)
                pandiff = sum(1 for x, y in zip(open(p1,"rb").read(), b2) if x != y)
                if pandiff > 0:
                    rep.check(False, f"LIVENESS  'Area Map' over {LIVENESS_GAP_S}s",
                              f"0 bytes spontaneously, but an explicit pan redrew {pandiff} "
                              f"bytes and sim advanced {simadv} ticks => STALLED/SLOW, "
                              f"renderer alive (throughput problem, NOT a freeze)")
                else:
                    rep.check(False, f"LIVENESS  'Area Map' over {LIVENESS_GAP_S}s",
                              f"0 bytes spontaneously AND pan forced no redraw "
                              f"(sim advanced {simadv} ticks) => GENUINELY FROZEN")
        except Exception as e:
            rep.check(False, "LIVENESS 'Area Map'", str(e))

    # --- building info windows: the path the operator saw magenta in ---
    mine = [l.split()[0] for l in cmd(["units"], port).splitlines()
            if len(l.split()) >= 5 and l.split()[3] == "building" and l.split()[4] == "me"]
    print(f"my buildings: {len(mine)} (sampling {min(nb,len(mine))})")
    # NOTE: each info window is CLOSED again after capture. Leaving them open is not
    # merely untidy — 5+ open keep-on-top dialogs (edict-hosting buildings) drive the
    # POSIX per-frame SDL_RaiseWindow path into multi-second stalls (root-caused
    # 2026-08-05), so a sweep that accumulates them measures its own side effect.
    # mac_regress.py opens 141 and closes none; this must not repeat that.
    for bid in mine[:nb]:
        if not cmd(["showinfo", bid], port).startswith("ok"): continue
        time.sleep(0.6)
        w2 = cmd(["wins"], port)
        info = [l for l in w2.splitlines() if l.split(":")[0] not in ids.values()]
        if not info: continue
        iw = info[-1].split(":")[0].strip()
        iname = info[-1].split('"')[1] if '"' in info[-1] else iw
        m = re.match(r'\d+:(\d+)x(\d+)', info[-1].strip())
        iwid, ihgt = (int(m.group(1)), int(m.group(2))) if m else (0, 0)
        if not grab(port, iw, tmp):
            if iwid: cmd(["clickid", iw, str(iwid // 2), str(ihgt - 24)], port)
            continue
        try:
            _, _, px = decode_bmp(tmp)
            mag, flat, col = analyse(px)
            if shots:
                subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "bmp2png.py"),
                                tmp, os.path.join(shots, f"info_{bid}_{iname.replace(' ','_')}.png")],
                               capture_output=True)
            rep.check(mag <= COLOURKEY_MAX_PCT, f"COLOURKEY info {iname!r} (bldg {bid})",
                      f"{mag:.3f}% magenta")
        except Exception:
            pass
        finally:
            # Close button sits bottom-centre; height varies per building so it must be
            # computed, not hard-coded. `finally` so a decode failure still closes it.
            # Escape fallback: the Rocket Ship window is 772x821 (BUGS #67) — taller than
            # the usable-bounds screen — so its Close button is not reachable and the
            # click alone leaves it open.
            if iwid:
                cmd(["clickid", iw, str(iwid // 2), str(ihgt - 24)], port); time.sleep(0.5)
                if any(l.split(":")[0].strip() == iw for l in cmd(["wins"], port).splitlines()):
                    cmd(["keyid", iw, "27", "0"], port); time.sleep(0.5)

    left = [l for l in cmd(["wins"], port).splitlines() if l.split(":")[0] not in ids.values()]
    rep.check(len(left) == 0, "info windows closed after sweep",
              f"{len(left)} still open (leaving them open perturbs later measurements)")

    print("\n=== %d check(s) FAILED ===" % rep.fails if rep.fails else "\n=== ALL CHECKS PASSED ===")
    if proc:
        cmd(["quit"], port); time.sleep(1)
        try: proc.kill()
        except Exception: pass
    sys.exit(1 if rep.fails else 0)

if __name__ == "__main__":
    main()
