#!/usr/bin/env python3
"""Summarize legacy Perf interval logs without inventing frame percentiles.

Usage: python summarize_log.py perf.log [--start 120] [--end 840]
Times are Perf's integer process-relative seconds. Only complete intervals
whose previous and current timestamps lie within the window are included.
The first row anchors time and is excluded because its start is unknown.
This is exploratory analysis, not an M0 qualification or comparison gate.
"""

import argparse
import json
import math
import re
from pathlib import Path


PAIR = re.compile(r"([A-Za-z_][A-Za-z_0-9.]*)=\s*(-?\d+(?:\.\d+)?)")
BUCKETS = ("frame.lt20", "frame.20_41", "frame.41_60",
           "frame.60_100", "frame.ge100")
SECTIONS = ("pump", "sim", "render", "present", "msg", "sleep",
            "operB", "operV", "operP")


def summarize(lines, start=0, end=math.inf):
    previous = None
    rows = []
    for number, line in enumerate(lines, 1):
        if not line.startswith("t="):
            continue
        row = {key: float(value) for key, value in PAIR.findall(line)}
        if not all(key in row for key in ("t", "fps", "avg", "max")):
            raise ValueError(f"incomplete interval at line {number}")
        t = row["t"]
        if previous is not None and t <= previous:
            raise ValueError(f"non-increasing timestamp at line {number}; split concatenated runs")
        if previous is not None and previous >= start and t <= end:
            rows.append((t - previous, row))
        previous = t
    if not rows:
        raise ValueError("no complete intervals in requested window")

    elapsed = sum(dt for dt, _ in rows)
    frames = {key: int(sum(row.get(key, 0) for _, row in rows)) for key in BUCKETS}
    # The registry retains registered names even at zero. An absent bucket can
    # mean no such frame has occurred, but an absent histogram means unknown.
    has_histogram = all(any(key in row for key in BUCKETS) for _, row in rows)
    count = sum(frames.values())
    recorded_drops = [row["perf.counters.dropped"] for _, row in rows
                      if "perf.counters.dropped" in row]
    drops = max(recorded_drops) if recorded_drops else None
    counter_names = sorted({key for _, row in rows for key in row
                            if key.endswith(".us")})
    return {
        "intervals": len(rows),
        "elapsed_integer_timestamp_seconds": elapsed,
        "timestamp_resolution_seconds": 1,
        "max_frame_ms": max(row["max"] for _, row in rows),
        "intervals_with_max_ge100ms": sum(row["max"] >= 100 for _, row in rows),
        "histogram_frames": frames if has_histogram else None,
        "histogram_frame_count": count if has_histogram else None,
        "frame_ge100_share": frames["frame.ge100"] / count if has_histogram and count else None,
        "histogram_fps": count / elapsed if has_histogram and count else None,
        "section_ms_per_elapsed_second": {
            key: (sum(row.get(key, 0) for _, row in rows) / elapsed
                  if all(key in row for _, row in rows) else None)
            for key in SECTIONS
        },
        "counter_us_per_elapsed_second": {
            key: sum(row.get(key, 0) for _, row in rows) / elapsed for key in counter_names
        },
        "counter_drops_max": drops,
        "limitations": [
            "Integer timestamps quantize elapsed rates; retain long windows.",
            "Section and named-counter times can overlap and must not be summed.",
            "Interval maxima do not provide frame p99 or path-search percentiles.",
            "Absent counters are unknown; counter drops invalidate zero-based conclusions.",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--start", type=float, default=0)
    parser.add_argument("--end", type=float, default=math.inf)
    args = parser.parse_args()
    if not math.isfinite(args.start) or args.start < 0 or args.end <= args.start or math.isnan(args.end):
        parser.error("require finite start >= 0 and end > start")
    try:
        with args.log.open(encoding="utf-8", errors="replace") as stream:
            result = summarize(stream, args.start, args.end)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    print(json.dumps(result, indent=2, allow_nan=False))


if __name__ == "__main__":
    main()
