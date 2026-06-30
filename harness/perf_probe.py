#!/usr/bin/env python3
"""perf_probe.py — launch a build, load a save, sample perf.log, report median fps + key costs.

Usage:
    python3 harness/perf_probe.py <binary> <label> [save] [seconds]

Drives the in-process TCP harness (127.0.0.1:$EN_HARNESS_PORT, default 7070):
launches <binary> from run-mac/ with EN_PERF=1, loads <save> from the menu, lets the
sim run <seconds>, then parses run-mac/perf.log (the lines from THIS session) and prints
median fps / avg-ms / operV / the cy.wait counter. Built for the @9ab0210c WaitForSingleObject
A/B (researched_mac2: gcc 10.7->48.8 fps); reusable for any perf check.
"""
import os, sys, socket, subprocess, time, signal, statistics, re

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RUN  = os.path.join(REPO, "run-mac")
PORT = int(os.environ.get("EN_HARNESS_PORT", "7070"))

def cmd(verb, timeout=15):
    """One command per connection: send a line, read until the server closes."""
    s = socket.create_connection(("127.0.0.1", PORT), timeout=timeout)
    s.sendall((verb + "\n").encode())
    buf = b""
    try:
        while True:
            chunk = s.recv(4096)
            if not chunk: break
            buf += chunk
    except socket.timeout:
        pass
    s.close()
    return buf.decode(errors="replace").strip()

def wait_socket(deadline):
    while time.time() < deadline:
        try:
            socket.create_connection(("127.0.0.1", PORT), timeout=2).close()
            return True
        except OSError:
            time.sleep(0.5)
    return False

def main():
    if len(sys.argv) < 3:
        print("usage: perf_probe.py <binary> <label> [save] [seconds]"); sys.exit(2)
    binary  = os.path.abspath(sys.argv[1])
    label   = sys.argv[2]
    save    = sys.argv[3] if len(sys.argv) > 3 else "researched_mac2.en"
    seconds = int(sys.argv[4]) if len(sys.argv) > 4 else 30
    savepath = save if os.path.isabs(save) else os.path.join(RUN, save)
    perflog  = os.path.join(RUN, "perf.log")

    # fresh perf.log so we only read this session
    open(perflog, "w").close()
    env = dict(os.environ, EN_PERF="1", EN_HARNESS="1",
               SDL_RENDER_DRIVER="opengl", EN_FULLSCREEN="0")
    gamelog = open(f"/tmp/perf_{label}.gamelog", "w")
    print(f"[{label}] launching {binary}")
    proc = subprocess.Popen([binary], cwd=RUN, env=env,
                            stdout=gamelog, stderr=subprocess.STDOUT)
    try:
        if not wait_socket(time.time() + 40):
            print(f"[{label}] ERR: harness socket never came up"); return 2
        print(f"[{label}] socket up; loading {save}")
        r = cmd(f"load {savepath}", timeout=120)
        if not r.startswith("ok"):
            print(f"[{label}] ERR: load -> {r!r}"); return 2
        st = cmd("pstats")
        if st.startswith("err") or not st:
            print(f"[{label}] ERR: not in-game after load -> {st!r}"); return 2
        print(f"[{label}] in-game; sampling {seconds}s ...")
        time.sleep(seconds)
    finally:
        try: cmd("quit", timeout=5)
        except OSError: pass
        time.sleep(1)
        proc.send_signal(signal.SIGKILL)
        subprocess.run(["pkill", "-9", "-f", "enations_latest/src/enations"],
                       stderr=subprocess.DEVNULL)
        gamelog.close()

    # parse the in-game perf lines (fps>0)
    fps, avg, operV, cyw = [], [], [], []
    cyw_name = None
    for ln in open(perflog):
        m = re.search(r"fps=\s*([\d.]+)\s+avg=\s*([\d.]+)", ln)
        if not m: continue
        f = float(m.group(1))
        if f <= 0: continue          # menu / not-yet-in-game frames
        fps.append(f); avg.append(float(m.group(2)))
        mv = re.search(r"operV=\s*([\d.]+)", ln)
        if mv: operV.append(float(mv.group(1)))
        mc = re.search(r"\bcy\.?wait=\s*([\d.]+)", ln)   # the CheckYield poll cost counter
        if mc: cyw.append(float(mc.group(1))); cyw_name = "cy.wait"

    def med(xs): return statistics.median(xs) if xs else float("nan")
    print(f"\n==== perf [{label}]  (n={len(fps)} in-game samples) ====")
    print(f"  fps    median={med(fps):6.1f}   min={min(fps) if fps else float('nan'):6.1f}  max={max(fps) if fps else float('nan'):6.1f}")
    print(f"  avg ms median={med(avg):6.2f}")
    if operV: print(f"  operV  median={med(operV):6.1f} ms/s")
    if cyw:   print(f"  {cyw_name} median={med(cyw):.0f}")
    else:     print(f"  cy.wait counter: not present in perf.log")
    return 0

if __name__ == "__main__":
    sys.exit(main())
