#!/usr/bin/env python3
"""
mac_crashcheck.py — did the game leave a crash report behind?

WHY THIS EXISTS, and it is not a flattering reason. I criticised mac_regress.py for
having exactly ONE failure signal — a new .ips crash report or a dead socket — and built
three replacements that check what RENDERED, whether it PLAYS, and whether it HOLDS.
Then the game SIGSEGV'd twice on shutdown (2026-08-05 07:01 and 07:04, in
SDL2Panel::RememberPlacement via a destroyed static map) and none of my tools noticed,
because in replacing mac_regress.py's narrow check I dropped its one real signal.
The operator spotted it; I did not.

So: a crash check belongs alongside the others, not instead of them. Crash reports land
in ~/Library/Logs/DiagnosticReports as <procname>-<timestamp>.ips, so attribution by
process name is free.

  from mac_crashcheck import snapshot, new_since
  before = snapshot()
  ... run the game ...
  crashes = new_since(before)      # list of basenames, empty is good
"""
import glob, os

IPS_DIR = os.path.expanduser("~/Library/Logs/DiagnosticReports")

def snapshot(pattern="enations*"):
    """Set of crash-report paths matching the game, taken BEFORE a run."""
    return set(glob.glob(os.path.join(IPS_DIR, pattern + ".ips")))

def new_since(before, pattern="enations*"):
    """Basenames of crash reports that appeared since `before`."""
    return sorted(os.path.basename(p) for p in snapshot(pattern) - before)

def summarise(name):
    """One-line cause + faulting frame for a report, for the failure message."""
    import json
    try:
        raw = open(os.path.join(IPS_DIR, name)).read()
        body = json.loads(raw[raw.index("\n") + 1:])
        exc = body.get("exception", {})
        th = body["threads"][body.get("faultingThread", 0)]
        frame = next((f.get("symbol", "") for f in th.get("frames", []) if f.get("symbol")), "")
        return f"{exc.get('signal','?')} {exc.get('subtype','')} in {frame[:70]}"
    except Exception as e:
        return f"(could not parse: {e})"

if __name__ == "__main__":
    for p in sorted(snapshot()):
        n = os.path.basename(p)
        print(f"{n}\n    {summarise(n)}")
