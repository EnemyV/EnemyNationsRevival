#!/usr/bin/env python3
"""crashcheck.py — detect & symbolize macOS crashes of the enations build.

macOS writes a JSON ".ips" crash report to ~/Library/Logs/DiagnosticReports/ for
every SIGSEGV/SIGABRT/etc. This tool finds enations reports and prints the
faulting thread's stack with source lines (via atos against the local binary),
so an agent driving the game can DETECT a crash (exit code 134/139, or a fresh
report) and immediately see WHERE it died — no lldb attach needed (the hardened
runtime blocks that).

Usage:
  crashcheck.py latest [binary]          # symbolize the newest enations report
  crashcheck.py since <ISO-or-epoch> [binary]   # list reports newer than a time
  crashcheck.py count                    # number of enations reports (for baselining)

Default binary: run-mac/enations (resolve symbols against the exact build you ran).
Exit code: 0 if no (matching) crash, 2 if a crash report was found+printed.
"""
import sys, os, json, glob, subprocess

REPORT_DIR = os.path.expanduser("~/Library/Logs/DiagnosticReports")
DEFAULT_BIN = os.path.join(os.path.dirname(__file__), "..", "run-mac", "enations")


def reports():
    fs = glob.glob(os.path.join(REPORT_DIR, "enations*.ips"))
    return sorted(fs, key=os.path.getmtime)


def load(path):
    with open(path) as f:
        head, body = f.read().split("\n", 1)
    return json.loads(head), json.loads(body)


def symbolize(path, binary):
    head, p = load(path)
    exc = p.get("exception", {})
    sig = exc.get("signal") or exc.get("type")
    sub = exc.get("subtype", "")
    print(f"# {os.path.basename(path)}")
    print(f"  signal: {sig}   {sub}")
    term = p.get("termination", {})
    if term:
        print(f"  termination: {term.get('indicator','')}")
    fc = p.get("faultingThread", 0)
    threads = p.get("threads", [])
    imgs = p.get("usedImages", [])
    if fc is None or fc >= len(threads):
        print("  (no faulting thread)")
        return
    # find the slide for the main binary so atos offsets line up
    frames = threads[fc].get("frames", [])
    print("  faulting thread:")
    # batch atos for enations frames
    en_offsets, en_idx = [], []
    for i, fr in enumerate(frames[:24]):
        nm = imgs[fr["imageIndex"]].get("name", "?") if fr.get("imageIndex") is not None else "?"
        if nm == "enations":
            en_offsets.append(hex(0x100000000 + fr["imageOffset"]))
            en_idx.append(i)
    sym = {}
    if en_offsets and os.path.exists(binary):
        try:
            out = subprocess.check_output(
                ["atos", "-o", binary, "-arch", "arm64", "-l", "0x100000000", *en_offsets],
                stderr=subprocess.DEVNULL).decode().splitlines()
            for i, line in zip(en_idx, out):
                sym[i] = line.strip()
        except Exception:
            pass
    for i, fr in enumerate(frames[:24]):
        nm = imgs[fr["imageIndex"]].get("name", "?") if fr.get("imageIndex") is not None else "?"
        off = fr.get("imageOffset", 0)
        label = sym.get(i) or fr.get("symbol", "")
        print(f"    {nm:18} +{off:#08x}  {label}")


def main():
    args = sys.argv[1:]
    cmd = args[0] if args else "latest"
    if cmd == "count":
        print(len(reports()))
        return 0
    binary = None
    if cmd == "latest":
        binary = args[1] if len(args) > 1 else DEFAULT_BIN
        rs = reports()
        if not rs:
            print("no enations crash reports")
            return 0
        symbolize(rs[-1], os.path.abspath(binary))
        return 2
    if cmd == "since":
        when = args[1]
        binary = args[2] if len(args) > 2 else DEFAULT_BIN
        try:
            thresh = float(when)
        except ValueError:
            import datetime
            thresh = datetime.datetime.fromisoformat(when).timestamp()
        new = [r for r in reports() if os.path.getmtime(r) > thresh]
        if not new:
            print("no new enations crash reports")
            return 0
        for r in new:
            symbolize(r, os.path.abspath(binary))
            print()
        return 2
    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main())
