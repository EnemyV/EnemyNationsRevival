#!/usr/bin/env python3
"""Run an unchanged P1 runner with conservative complete-row Perf boundaries.

python perf_window_driver.py --runner run_fixture.py [ordinary runner arguments]
Require --env EN_PERF=1; default EN_PERF_INTERVAL_MS=1000 is supported.
This driver is exploratory tooling, not frozen M0 qualification.
Use a local append-only log: current Perf fflushes after each interval. One
possibly buffered pre-BEGIN row is guarded out, then the next row is only an
anchor. Arbitrarily delayed visibility on a network filesystem is unsupported.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys

MAX_LOG_BYTES = 128 * 1024 * 1024
TIME = re.compile(rb"^t=(\d+)\s")


def digest(data):
    return hashlib.sha256(data).hexdigest()


class PerfWindow:
    def __init__(self, output):
        self.output = Path(output)
        self.log = self.output / "perf.log"
        self.begin = None
        self.end = None
        self.end_confirmed = False

    def snapshot(self):
        with self.log.open("rb") as stream:
            data = stream.read(MAX_LOG_BYTES + 1)
        if len(data) > MAX_LOG_BYTES:
            raise ValueError("perf.log exceeds 128 MiB capture bound")
        offset = data.rfind(b"\n") + 1
        return {"complete_offset": offset, "prefix_sha256": digest(data[:offset])}

    def begin_reply(self):
        if self.begin is not None:
            raise ValueError("duplicate MARK BEGIN")
        self.begin = self.snapshot()

    def before_end_send(self):
        if self.begin is None or self.end is not None:
            raise ValueError("missing begin or duplicate MARK END")
        self.end = self.snapshot()

    def finish(self):
        if self.begin is None or self.end is None or not self.end_confirmed:
            raise ValueError("missing successful measurement marks")
        if self.end["complete_offset"] <= self.begin["complete_offset"]:
            raise ValueError("no appended complete Perf rows")
        with self.log.open("rb") as stream:
            data = stream.read(MAX_LOG_BYTES + 1)
        if len(data) > MAX_LOG_BYTES:
            raise ValueError("perf.log exceeds 128 MiB capture bound")
        for boundary in (self.begin, self.end):
            offset = boundary["complete_offset"]
            if len(data) < offset or digest(data[:offset]) != boundary["prefix_sha256"]:
                raise ValueError("perf.log truncated or rewritten after boundary capture")
        selected = data[self.begin["complete_offset"]:self.end["complete_offset"]]
        rows = [line for line in selected.splitlines(keepends=True) if line.startswith(b"t=")]
        if len(rows) < 3:
            raise ValueError("need a buffering guard, anchor row and one complete inner interval")
        times = []
        for row in rows:
            match = TIME.match(row)
            if match is None or any(field not in row for field in (b"fps=", b"avg=", b"max=")):
                raise ValueError("malformed Perf interval row")
            times.append(int(match.group(1)))
        if any(right <= left for left, right in zip(times, times[1:])):
            raise ValueError("non-increasing Perf times; restarted log or subsecond cadence")
        # Current Perf fflushes every interval, so at most one completed row can
        # remain in its stdio buffer. Drop a whole first row as that guard. The
        # next row may still straddle BEGIN: keep only its timestamp as anchor;
        # summarize_log.py excludes its accumulated values. All later row work
        # begins after that anchor and completes before the END request is sent.
        window = b"".join(rows[1:])
        (self.output / "perf-window.log").write_bytes(window)
        return {
            "status": "captured", "begin": self.begin, "end": self.end,
            "buffer_guard_t": times[0], "anchor_t": times[1], "last_included_t": times[-1],
            "included_intervals": len(rows) - 2,
            "elapsed_integer_timestamp_seconds": times[-1] - times[1],
            "perf_window_sha256": digest(window),
            "rule": "Drop first appended row as buffering guard; second is anchor only; include third onward before MARK END send.",
            "limitations": ["Integer Perf timestamps quantize elapsed rates.",
                            "The inner Perf window is shorter than the runner's marked span.",
                            "Requires local append-only visibility and current Perf per-row fflush; network logs unsupported.",
                            "This provides no frame p99 or per-search percentile."],
        }


class MarkClient:
    def __init__(self, client, window):
        self.client, self.window = client, window

    def __getattr__(self, name):
        return getattr(self.client, name)

    def __enter__(self):
        self.client.__enter__()
        return self

    def __exit__(self, *args):
        return self.client.__exit__(*args)

    def sendall(self, data, *args, **kwargs):
        if data.endswith(b" MARK END\n"):
            self.window.before_end_send()
        return self.client.sendall(data, *args, **kwargs)


class MarkSockets:
    def __init__(self, sockets, window):
        self.sockets, self.window = sockets, window

    def __getattr__(self, name):
        return getattr(self.sockets, name)

    def create_connection(self, *args, **kwargs):
        return MarkClient(self.sockets.create_connection(*args, **kwargs), self.window)


def attach(runner, window):
    read_response = runner.read_response

    def read(client):
        response = read_response(client)
        if response.startswith("OK mark=begin "):
            window.begin_reply()
        elif response.startswith("OK mark=end "):
            window.end_confirmed = True
        return response

    runner.read_response = read
    runner.socket = MarkSockets(runner.socket, window)


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--runner", type=Path, required=True)
    options, forwarded = parser.parse_known_args()
    details = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    details.add_argument("--output", type=Path, required=True)
    details.add_argument("--env", action="append", default=[])
    args, _ = details.parse_known_args(forwarded)
    overrides = dict(item.split("=", 1) for item in args.env if "=" in item)
    if overrides.get("EN_PERF") != "1":
        parser.error("forward explicit --env EN_PERF=1")
    if overrides.get("EN_PERF_INTERVAL_MS", "1000") != "1000":
        parser.error("this driver requires default or explicit EN_PERF_INTERVAL_MS=1000")
    runner_path = options.runner.resolve()
    spec = importlib.util.spec_from_file_location("p1_runner", runner_path)
    runner = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(runner)
    window = PerfWindow(args.output.resolve())
    attach(runner, window)
    sys.argv = [str(runner_path), *forwarded]
    code = runner.main()
    record = {"runner_sha256": digest(runner_path.read_bytes()),
              "driver_sha256": digest(Path(__file__).read_bytes()),
              "runner_exit_code": code}
    try:
        if code != 0:
            raise ValueError("P1 runner did not qualify")
        record.update(window.finish())
    except (OSError, ValueError) as error:
        record.update(status="invalid", error=str(error))
        code = 1
    if args.output.is_dir():
        (args.output / "perf-boundaries.json").write_text(json.dumps(record, indent=2) + "\n")
    return code


if __name__ == "__main__":
    raise SystemExit(main())
