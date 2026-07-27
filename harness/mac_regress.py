#!/usr/bin/env python3
"""
mac_regress.py — scripted regression sweep for the macOS TCP harness.

Drives an ALREADY-RUNNING game (EN_HARNESS=1 ./enations) through a battery of
load / render / scroll / zoom / building-window / back-to-back-reload steps and
reports PASS/FAIL plus any NEW crash report (.ips) that appears mid-run. Pure
client-side (existing socket verbs only) — no game rebuild needed.

Usage (game already up on 127.0.0.1:7070):
  python3 harness/mac_regress.py
  python3 harness/mac_regress.py --saves coalliq_diag,mac2_ck6_developed
  python3 harness/mac_regress.py --shots /tmp/regress   # also save PNGs
  python3 harness/mac_regress.py --launch               # launch+wait+sweep+KILL (one-shot)

Exit code 0 = clean (no crash), 1 = a crash/.ips appeared. Each step prints
[ok]/[CRASH]. Designed for the mac2 idle-loop: one command per regression tick.
"""
import os, sys, glob, time, socket, subprocess, signal

PORT = int(os.environ.get("EN_HARNESS_PORT", "7070"))
HERE = os.path.dirname(os.path.abspath(__file__))
CLIENT = os.path.join(HERE, "client_unavailable")  # we talk to the socket directly
IPS_DIR = os.path.expanduser("~/Library/Logs/DiagnosticReports")

def cmd(args, timeout=20):
    s = socket.socket(); s.settimeout(timeout)
    try:
        s.connect(("127.0.0.1", PORT))
    except OSError:
        return "ERR_NO_SOCKET"
    s.sendall((" ".join(str(a) for a in args) + "\n").encode())
    time.sleep(0.2)
    data = b""
    try:
        while True:
            c = s.recv(4096)
            if not c: break
            data += c
    except socket.timeout:
        pass
    s.close()
    return data.decode(errors="replace").strip()

def newest_ips_mtime():
    f = sorted(glob.glob(os.path.join(IPS_DIR, "enations-*.ips")), key=os.path.getmtime)
    return os.path.getmtime(f[-1]) if f else 0.0

def alive():
    return cmd(["pstats"]) not in ("ERR_NO_SOCKET",)

def launch_game():
    # Launch the harness build from run-mac, wait for the socket, return the pid.
    run = os.path.join(os.path.dirname(HERE), "run-mac")
    env = dict(os.environ, EN_HARNESS="1", SDL_RENDER_DRIVER="opengl")
    log = open("/tmp/mac_regress_game.log", "w")
    p = subprocess.Popen(["./enations"], cwd=run, env=env, stdout=log, stderr=subprocess.STDOUT)
    for _ in range(120):  # up to ~30s
        if cmd(["pstats"]) != "ERR_NO_SOCKET": return p.pid
        if p.poll() is not None: print("game died before socket; see /tmp/mac_regress_game.log"); sys.exit(2)
        time.sleep(0.25)
    print("socket never came up"); sys.exit(2)

def main():
    saves = ["coalliq_diag", "mac2_ck6_developed"]
    shots = None; do_launch = False
    a = sys.argv[1:]
    for i, x in enumerate(a):
        if x == "--saves" and i+1 < len(a): saves = a[i+1].split(",")
        if x == "--shots" and i+1 < len(a): shots = a[i+1]
        if x == "--launch": do_launch = True
    if shots: os.makedirs(shots, exist_ok=True)
    launched_pid = launch_game() if do_launch else None

    base_ips = newest_ips_mtime()
    crashes = 0
    def step(name, note=""):
        # The ONLY real failure signal is a crash (new .ips) or a dead socket.
        # Load-failures are expected harness semantics (load is from-MENU only), so
        # they are reported as info, never as a regression.
        nonlocal crashes
        if newest_ips_mtime() > base_ips:
            crashes += 1; print(f"[CRASH] {name}"); return False
        if cmd(["pstats"]) == "ERR_NO_SOCKET":
            crashes += 1; print(f"[DEAD]  {name} (process gone, no .ips)"); return False
        print(f"[ok]    {name}{(' — ' + note) if note else ''}"); return True

    if cmd(["pstats"]) == "ERR_NO_SOCKET":
        print("no game on :%d — launch EN_HARNESS=1 ./enations from run-mac first" % PORT); sys.exit(2)

    for sv in saves:
        r = cmd(["load", sv + ".en"])
        if not step(f"load {sv}", note=r): break
        wins = cmd(["wins"])
        # area-map window id = the 'Area Map' line
        amid = next((l.split(":")[0] for l in wins.splitlines() if "Area Map" in l), None)
        if amid:
            if shots:
                cmd(["shotid", amid, "/tmp/_r.bmp"])
                subprocess.run([sys.executable, os.path.join(HERE, "bmp2png.py"), "/tmp/_r.bmp",
                                os.path.join(shots, f"am_{sv}.png")], capture_output=True)
            step(f"  area-map render ({sv})")
            cmd(["scroll", amid, "500", "0"]); step(f"  scroll ({sv})")
            cmd(["wheel", amid, "2", "410", "340"]); cmd(["wheel", amid, "-2", "410", "340"]); step(f"  zoom in/out ({sv})")
        # building-window sweep (open each of my buildings, capture)
        n = 0
        for ln in cmd(["units"]).splitlines():
            p = ln.split()
            # format: <id> <x> <y> <type> <owner> [state] — owner is field 4 (not last;
            # buildings carry a trailing state e.g. "241 570 300 building me operational")
            if len(p) >= 5 and p[4] == "me" and p[3] == "building":
                cmd(["showinfo", p[0]]); cmd(["shotid", "6", "/tmp/_b.bmp"]); n += 1
        step(f"  building-window sweep ({sv})", note=f"{n} bldgs")

    # back-to-back reload stress (exercises the load-while-in-game crash path)
    if len(saves) >= 2:
        cmd(["showinfo", "195"])
        cmd(["load", saves[0] + ".en"]); cmd(["load", saves[1] + ".en"])
        step("back-to-back reload (load-crash path)")

    print(f"=== regression sweep: {'CLEAN — 0 crashes' if crashes == 0 else str(crashes)+' CRASH(es)'} ===")
    if launched_pid:
        cmd(["quit"]); time.sleep(1)
        try: os.kill(launched_pid, signal.SIGKILL)
        except OSError: pass
        subprocess.run(["pkill", "-9", "-f", "enations"], capture_output=True)
        print("[launch] game KILLed")
    sys.exit(1 if crashes else 0)

if __name__ == "__main__":
    main()
