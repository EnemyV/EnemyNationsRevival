#!/usr/bin/env python3
"""
mac_regress_all.py — one command for the whole mac regression battery.

There are three tools because they answer three questions no single one can:

  mac_verify.py    did it RENDER correctly   (colour key, blank windows, repaint liveness)
  mac_playtest.py  does it PLAY              (orders, construction, save/load roundtrip)
  mac_soak.py      does it HOLD              (sim rate, movement, war, resources over time)

Each has caught a real defect, so none is redundant — but three commands with three flag
sets is friction, and friction is how a check gets skipped. This runs them in order against
one tree and prints a single verdict.

It GATES on the SDL linkage first: on the sdl2-compat shim the colour key is ignored and
every sprite renders on magenta, so a run against it is not merely suspect, it is invalid.
Better to refuse in two seconds than to produce twenty minutes of meaningless green.

  python3 harness/mac_regress_all.py --tree run-mac --exe ./enations_realsdl --save _big_out.en
  python3 harness/mac_regress_all.py --tree <artifact-dir> --exe ./enations --soak-min 30

NOTE: mac_regress.py is deliberately NOT included. Its only failure signal is a crash report
or a dead socket, so it reported CLEAN through magenta rendering, multi-second stalls and a
map that looked frozen. Do not treat its output as a regression result.
"""
import os, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))

def run(title, argv, port):
    print(f"\n{'='*72}\n== {title}\n{'='*72}", flush=True)
    t0 = time.time()
    p = subprocess.run([sys.executable] + argv, capture_output=True, text=True)
    out = (p.stdout or "") + (p.stderr or "")
    # echo only the verdict lines; the full transcript is noise at this level
    for ln in out.splitlines():
        if ln.startswith(("[ok]", "[FAIL]", "[note]", "===", "load:")) or "CHECKS" in ln or "SOAK" in ln:
            print("  " + ln, flush=True)
    print(f"  -- {title}: {'PASS' if p.returncode == 0 else 'FAIL'} in {time.time()-t0:.0f}s", flush=True)
    return p.returncode == 0

def main():
    a = sys.argv[1:]
    def opt(n, d=None): return next((a[i+1] for i, x in enumerate(a) if x == n), d)
    tree = opt("--tree", "run-mac")
    exe  = opt("--exe", "./enations_realsdl")
    save = opt("--save", "_big_out.en")
    port = opt("--port", "7073")
    soak = opt("--soak-min", "10")

    # --- gate: refuse to spend twenty minutes producing invalid results ---
    sys.path.insert(0, HERE)
    from mac_sdlcheck import check_sdl
    ok, detail = check_sdl(os.path.join(tree, os.path.basename(exe)))
    print(("[ok]   " if ok else "[FAIL] ") + "SDL linkage — " + detail, flush=True)
    if not ok:
        print("\n=== ABORTED: results against the sdl2-compat shim are invalid ===")
        sys.exit(2)

    from mac_crashcheck import snapshot, new_since, summarise
    crash_before = snapshot()

    subprocess.run(["pkill", "-9", "-f", "enations"], capture_output=True); time.sleep(2)
    results = {}
    results["render (mac_verify)"] = run(
        "RENDER — mac_verify.py",
        [os.path.join(HERE, "mac_verify.py"), "--launch", tree, "--exe", exe,
         "--port", port, "--save", save, "--bldgs", "3"], port)

    subprocess.run(["pkill", "-9", "-f", "enations"], capture_output=True); time.sleep(3)
    results["gameplay (mac_playtest)"] = run(
        "GAMEPLAY — mac_playtest.py",
        [os.path.join(HERE, "mac_playtest.py"), "--launch", tree, "--exe", exe,
         "--port", port, "--soak", "45"], port)

    subprocess.run(["pkill", "-9", "-f", "enations"], capture_output=True); time.sleep(3)
    results[f"soak {soak}min (mac_soak)"] = run(
        f"SOAK {soak} min — mac_soak.py",
        [os.path.join(HERE, "mac_soak.py"), "--launch", tree, "--exe", exe,
         "--port", port, "--save", save, "--minutes", soak], port)

    subprocess.run(["pkill", "-9", "-f", "enations"], capture_output=True)

    # Did anything CRASH? None of the three stages looks for this: I replaced
    # mac_regress.py — whose only signal was a new .ips — with tools that check
    # rendering, gameplay and endurance, and in doing so dropped its one real check.
    # The game then SIGSEGV'd twice on shutdown and every stage still reported PASS.
    time.sleep(3)   # reports are written a moment after the process dies
    crashes = new_since(crash_before)
    results["no crash reports"] = not crashes
    for c in crashes:
        print(f"  [FAIL] crash report {c}\n         {summarise(c)}", flush=True)

    print(f"\n{'='*72}\n== SUMMARY  (tree={tree} exe={exe})\n{'='*72}")
    for k, v in results.items():
        print(f"  {'PASS' if v else 'FAIL'}  {k}")
    bad = [k for k, v in results.items() if not v]
    print(f"\n=== {'ALL GREEN' if not bad else 'FAILED: ' + ', '.join(bad)} ===")
    sys.exit(1 if bad else 0)

if __name__ == "__main__":
    main()
