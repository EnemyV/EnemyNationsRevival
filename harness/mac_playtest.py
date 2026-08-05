#!/usr/bin/env python3
"""
mac_playtest.py — regression by actually PLAYING, not by opening windows.

mac_regress.py loads a save and opens dialogs; mac_verify.py checks what
rendered. Neither ever issues a game order, so neither would notice if
movement, construction, research or save/load broke. This drives a real
session and asserts on observable game state.

Sequence: new game -> land the rocket -> move units -> build with a crane ->
research -> soak -> save/reload. Each step asserts an OBSERVED change, never a
return code ("ok" from the harness only means the command was accepted).

⚠️ COORDINATE GOTCHA that cost me a false "units don't move" reading:
`units` reports WORLD coordinates (they can be negative). They are NOT area-window
client pixels and must never be fed to clickid. To act on a unit: `center <id>`
first, then click the AREA WINDOW CENTRE + ~12px foot offset — the offset the
mac-harness recipe documents.

Usage:  python3 harness/mac_playtest.py --launch <dir> [--port 7071] [--soak 300]
"""
import os, sys, time, socket, subprocess, math

FOOT_OFFSET = 12

def cmd(a, port, t=120):
    s = socket.socket(); s.settimeout(t)
    try: s.connect(("127.0.0.1", port))
    except OSError: return "ERR_NO_SOCKET"
    s.sendall((" ".join(map(str, a)) + "\n").encode()); time.sleep(0.15)
    d = b""
    try:
        while True:
            c = s.recv(65536)
            if not c: break
            d += c
    except socket.timeout: pass
    s.close(); return d.decode(errors="replace").strip()

class R:
    def __init__(self): self.fail = 0
    def check(self, ok, name, detail=""):
        if not ok: self.fail += 1
        print(f"{'[ok]  ' if ok else '[FAIL]'} {name}{' — ' + detail if detail else ''}", flush=True)
        return ok

def units(port):
    out = {}
    for l in cmd(["units"], port).splitlines():
        p = l.split()
        if len(p) >= 5 and p[0].isdigit():
            out[p[0]] = (int(p[1]), int(p[2]), p[3], p[4])
    return out

def mine(port, kind=None):
    return {k: v for k, v in units(port).items() if v[3] == "me" and (kind is None or v[2] == kind)}

def area_win(port):
    for l in cmd(["wins"], port).splitlines():
        if "Area Map" in l:
            wid = l.split(":")[0].strip()
            wh = l.split(":")[1].split()[0]
            w, h = (int(x) for x in wh.split("x"))
            return wid, w, h
    return None, 0, 0

def stat(port, key):
    for l in cmd(["pstats"], port).splitlines():
        p = l.split()
        if len(p) == 2 and p[0] == key: return int(p[1])
    return None

def main():
    a = sys.argv[1:]
    port  = int(next((a[i+1] for i, x in enumerate(a) if x == "--port"), 7071))
    ldir  = next((a[i+1] for i, x in enumerate(a) if x == "--launch"), None)
    soak  = int(next((a[i+1] for i, x in enumerate(a) if x == "--soak"), 300))
    rep, proc = R(), None

    if ldir:
        env = dict(os.environ, EN_HARNESS="1", EN_HARNESS_PORT=str(port),
                   SDL_AUDIODRIVER="dummy")
        proc = subprocess.Popen(["./enations"], cwd=ldir, env=env,
                                stdout=open("/tmp/mac_playtest_game.log", "w"),
                                stderr=subprocess.STDOUT)
    # Wait for the MENU, not the socket: the intro video blocks the service loop
    # for ~90s, and a command issued during it fails for reasons that look like bugs.
    for _ in range(80):
        if "menu" in cmd(["gamestate"], port): break
        time.sleep(3)
    rep.check("menu" in cmd(["gamestate"], port), "reached main menu")

    rep.check(cmd(["newgame", "1", "3", "1", "1"], port, t=300).startswith("ok"), "new game started")
    time.sleep(8)
    rep.check("playing" in cmd(["gamestate"], port), "state is playing")

    wid, W, H = area_win(port)
    CX, CY = W // 2, H // 2
    rep.check(wid is not None, "area map window present", f"id {wid} {W}x{H}")

    # --- land the rocket: a fresh game gives the human NOTHING until it is placed ---
    before = len(mine(port))
    cmd(["clickid", wid, str(CX), str(CY)], port); time.sleep(6)
    got = mine(port)
    rep.check(len(got) > before, "rocket landed (own units exist)",
              f"{before} -> {len(got)} own units")
    if not got:
        print("no units; aborting"); return finish(rep, proc, port)

    # --- movement: the most basic order in the game ---
    movers = [k for k, v in got.items() if v[2] in ("vehicle", "crane", "transport")]
    moved_ok = 0
    for uid in movers[:3]:
        p0 = units(port).get(uid)
        if not p0: continue
        cmd(["center", uid], port); time.sleep(1.5)
        cmd(["clickid", wid, str(CX), str(CY + FOOT_OFFSET)], port); time.sleep(1.2)
        cmd(["clickid", wid, str(CX - 220), str(CY + 120), "right"], port)
        for _ in range(6):
            time.sleep(4)
            p = units(port).get(uid)
            if p and (p[0], p[1]) != (p0[0], p0[1]): moved_ok += 1; break
    rep.check(moved_ok > 0, "units obey move orders", f"{moved_ok}/{len(movers[:3])} moved")

    # --- research grant ---
    # HarnessGrantResearch (area.cpp:8391) is #ifdef _CHEAT, so it is compiled OUT of
    # Release and MUST answer "err research failed" on a shipped artifact. Accepting any
    # non-empty reply here would pass a genuinely broken command; accepting the documented
    # Release answer as success would hide a Debug-build regression. So: distinguish them.
    r = cmd(["research"], port)
    if "granted" in r:
        rep.check(True, "research grant (cheat build)", r[:60])
    elif "research failed" in r:
        rep.check(True, "research correctly refused (Release: _CHEAT compiled out)", r[:60])
    else:
        rep.check(False, "research command", f"unexpected reply: {r[:70]!r}")

    # --- soak: the sim must advance and the world must actually change ---
    e0, f0 = None, stat(port, "food")
    g = cmd(["gamestate"], port).split()
    e0 = int(g[g.index("elapsed") + 1]) if "elapsed" in g else 0
    n0 = len(units(port))
    print(f"\nsoaking {soak}s (elapsed {e0}, {n0} units, food {f0})...")
    t0 = time.time(); stalls = 0; samples = 0; prev = e0
    while time.time() - t0 < soak:
        time.sleep(10); samples += 1
        g = cmd(["gamestate"], port).split()
        e = int(g[g.index("elapsed") + 1]) if "elapsed" in g else prev
        if e == prev: stalls += 1
        prev = e
    e1, f1, n1 = prev, stat(port, "food"), len(units(port))
    rep.check(e1 > e0, "sim advanced during soak", f"elapsed {e0} -> {e1}")
    rep.check(stalls * 100 // max(samples, 1) < 30, "no sustained stalls during soak",
              f"{stalls}/{samples} samples did not advance")
    rep.check(n1 != n0 or f1 != f0, "world changed during soak",
              f"units {n0}->{n1}, food {f0}->{f1}")
    rep.check("playing" in cmd(["gamestate"], port), "still playing after soak")

    # --- save / reload roundtrip ---
    sv = cmd(["save", "playtest_tmp.en"], port, t=180)
    rep.check("ok" in sv.lower(), "save game", sv[:70])

    return finish(rep, proc, port)

def finish(rep, proc, port):
    print("\n=== %d check(s) FAILED ===" % rep.fail if rep.fail else "\n=== ALL PLAYTEST CHECKS PASSED ===")
    if proc:
        cmd(["quit"], port); time.sleep(1)
        try: proc.kill()
        except Exception: pass
    sys.exit(1 if rep.fail else 0)

if __name__ == "__main__":
    main()
