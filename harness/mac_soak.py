#!/usr/bin/env python3
"""
mac_soak.py — the ~30 minute soak the fix workflow requires, with the regression
metrics it names: vehicles actually moving, construction progressing, war activity,
and resources moving. Plus a mid-soak stress of the path that was fixed.

The rule is "run the game and soak ~30 minutes; the original symptom must be
measurably gone, then check for regressions in the same soak". A soak that only
proves the process is alive does not do that, so this samples game state on a
cadence and reports per-metric verdicts at the end.

Metrics per sample (default every 60s):
  sim         elapsed delta / wall second, and whether it stalled at all
  moving%     fraction of MY vehicles whose world position changed since last sample
  bldgs       my building count (construction / destruction)
  kills       cumulative vehsdest + bldgsdest — NOTE these are KILLS I MADE, not losses.
              IncVehsDest is called on the killer (netapi.cpp:2574, guarded by
              pPlr != pUnit->GetOwner()), and only for CUnit::vehicle, so cranes and
              infantry never appear. I first read this column as "losses" and was
              puzzled that my fleet shrank 159 -> 141 while it showed 1; gamestate
              exposes NO loss counter at all, so read attrition off myVeh instead.
  myVeh       my own mobile-unit count — this is where losses actually show
  food        resource movement

Mid-soak it opens N building info windows and leaves them open for one interval —
that is the exact configuration that used to stall the game (5+ keep-on-top dialogs)
— then closes them. If the stall regressed, the sample straddling it shows it.

Usage: python3 harness/mac_soak.py --launch run-mac --exe ./enations_realsdl \
                                   --port 7073 --save _big_out.en --minutes 30
"""
import os, re, sys, time, socket, subprocess

def cmd(a, port, t=90):
    s = socket.socket(); s.settimeout(t)
    try: s.connect(("127.0.0.1", port))
    except OSError: return "ERR_NO_SOCKET"
    s.sendall((" ".join(map(str, a)) + "\n").encode()); time.sleep(0.12)
    d = b""
    try:
        while True:
            c = s.recv(65536)
            if not c: break
            d += c
    except socket.timeout: pass
    s.close(); return d.decode(errors="replace").strip()

def gs(port):
    g = cmd(["gamestate"], port).split()
    out = {}
    for k in ("elapsed", "bldgshave", "vehshave", "bldgsdest", "vehsdest"):
        if k in g:
            try: out[k] = int(g[g.index(k) + 1])
            except (IndexError, ValueError): out[k] = -1
    return out

def my_vehicles(port):
    out = {}
    for l in cmd(["units"], port).splitlines():
        p = l.split()
        if len(p) >= 5 and p[0].isdigit() and p[4] == "me" and p[3] in (
                "vehicle", "crane", "transport", "carrier", "infantry"):
            out[p[0]] = (int(p[1]), int(p[2]))
    return out

def food(port):
    for l in cmd(["pstats"], port).splitlines():
        p = l.split()
        if len(p) == 2 and p[0] == "food":
            return int(p[1])
    return -1

def main():
    a = sys.argv[1:]
    def opt(n, d=None): return next((a[i+1] for i, x in enumerate(a) if x == n), d)
    port    = int(opt("--port", 7073))
    ldir    = opt("--launch")
    exe     = opt("--exe", "./enations")
    save    = opt("--save")
    minutes = float(opt("--minutes", 30))
    every   = float(opt("--every", 60))
    proc = None

    if ldir:
        env = dict(os.environ, EN_HARNESS="1", EN_HARNESS_PORT=str(port), SDL_AUDIODRIVER="dummy")
        proc = subprocess.Popen([exe], cwd=ldir, env=env,
                                stdout=open("/tmp/mac_soak_game.log", "w"), stderr=subprocess.STDOUT)
    for _ in range(90):
        if "menu" in cmd(["gamestate"], port): break
        time.sleep(3)
    if save:
        print("load:", cmd(["load", save], port, t=240), flush=True)
        for _ in range(60):
            if "playing" in cmd(["gamestate"], port): break
            time.sleep(3)

    n_samples = max(1, int(minutes * 60 / every))
    stress_at = n_samples // 2
    print(f"soaking {minutes:g} min, sampling every {every:g}s "
          f"({n_samples} samples), info-window stress at sample {stress_at}\n", flush=True)
    print(f"{'t(min)':>7} {'sim/s':>6} {'moving%':>8} {'myVeh':>6} {'bldgs':>6} "
          f"{'kills':>5} {'food':>7}  note", flush=True)

    prev_pos = my_vehicles(port); prev = gs(port); t0 = time.time()
    stalled_samples = 0; moved_any = 0; war0 = prev.get("vehsdest", 0) + prev.get("bldgsdest", 0)
    bldg0 = prev.get("bldgshave", 0); food0 = food(port); opened = []
    veh0 = len(prev_pos)
    rows = 0
    for i in range(n_samples):
        note = ""
        if i == stress_at:
            mineb = [l.split()[0] for l in cmd(["units"], port).splitlines()
                     if len(l.split()) >= 5 and l.split()[3] == "building" and l.split()[4] == "me"]
            for b in mineb[:8]:
                cmd(["showinfo", b], port); time.sleep(0.3)
            opened = [l for l in cmd(["wins"], port).splitlines()]
            note = f"opened 8 info windows ({len(opened)} total)"
        if i == stress_at + 1 and opened:
            for l in cmd(["wins"], port).splitlines():
                m = re.match(r'(\d+):(\d+)x(\d+)\s+\w+\s+"(.*)"', l.strip())
                if m and m.group(4) not in ("Enemy Nations - Game View", "Area Map") \
                     and not m.group(4).startswith("Radar"):
                    cmd(["clickid", m.group(1), str(int(m.group(2))//2), str(int(m.group(3))-24)], port)
                    time.sleep(0.4)
            note = "closed them"

        time.sleep(every)
        cur = gs(port); pos = my_vehicles(port); f = food(port)
        dt = every
        sim = (cur.get("elapsed", 0) - prev.get("elapsed", 0)) / dt
        if sim <= 0: stalled_samples += 1
        common = set(pos) & set(prev_pos)
        movers = sum(1 for k in common if pos[k] != prev_pos[k])
        movpct = 100.0 * movers / max(len(common), 1)
        if movers: moved_any += 1
        war = cur.get("vehsdest", 0) + cur.get("bldgsdest", 0)
        print(f"{(time.time()-t0)/60:7.1f} {sim:6.2f} {movpct:7.1f}% {len(pos):6} "
              f"{cur.get('bldgshave',-1):6} {war:5} {f:7}  {note}", flush=True)
        prev, prev_pos = cur, pos; rows += 1

    war1 = prev.get("vehsdest", 0) + prev.get("bldgsdest", 0)
    fails = 0
    def check(ok, name, detail):
        nonlocal fails
        if not ok: fails += 1
        print(f"{'[ok]  ' if ok else '[FAIL]'} {name} — {detail}", flush=True)
    print()
    check(stalled_samples * 100 // max(rows, 1) < 20, "sim advanced throughout",
          f"{stalled_samples}/{rows} samples showed no advance")
    check(moved_any > rows // 2, "vehicles kept moving",
          f"{moved_any}/{rows} samples had movement")
    # Combat and construction are SLOW: across two 30-minute soaks the first kill appeared
    # around minute 12 and the first unit loss around minute 7. So on a short soak this
    # legitimately shows nothing, and requiring it produced a FAIL on a run where the sim
    # advanced with 0/5 stalls, vehicles moved in 5/5 samples and food went 6687 -> 0 —
    # i.e. the world was obviously progressing and the check was simply asking the wrong
    # question for the duration. Only demand combat/construction evidence when the soak is
    # long enough to expect it; movement and resources are already checked separately.
    progressed = (prev.get("bldgshave", 0) != bldg0 or war1 != war0
                  or len(prev_pos) != veh0)
    if minutes >= 15:
        check(progressed, "world progressed (combat or construction)",
              f"buildings {bldg0} -> {prev.get('bldgshave')}, kills {war0} -> {war1}, "
              f"my mobile units {veh0} -> {len(prev_pos)}")
    else:
        print(f"[note]  combat/construction {'observed' if progressed else 'NOT observed'} "
              f"in {minutes:g} min — not asserted below 15 min (first kill was ~min 12 in "
              f"the 30-min soaks); buildings {bldg0} -> {prev.get('bldgshave')}, "
              f"kills {war0} -> {war1}, units {veh0} -> {len(prev_pos)}", flush=True)
    check(food(port) != food0, "resources moved", f"food {food0} -> {food(port)}")
    check("playing" in cmd(["gamestate"], port), "still playing at the end", "")
    # "new buildings" is one of the regression metrics the fix workflow names, but on a
    # developed save with nothing queued it never moves — and then "world progressed"
    # passes on the other terms and the gap is invisible. Say so explicitly instead:
    # this is NOT a failure (there may be nothing to build), it is a COVERAGE statement.
    if prev.get("bldgshave", 0) == bldg0:
        print(f"[note]  construction NOT exercised — buildings stayed {bldg0} for the whole "
              f"soak, so that metric is untested here; mac_playtest.py covers it directly",
              flush=True)
    else:
        print(f"[note]  construction observed — buildings {bldg0} -> {prev.get('bldgshave')}",
              flush=True)
    print("\n=== SOAK %s ===" % ("FAILED" if fails else "CLEAN"))
    if proc:
        cmd(["quit"], port); time.sleep(1)
        try: proc.kill()
        except Exception: pass
    sys.exit(1 if fails else 0)

if __name__ == "__main__":
    main()
