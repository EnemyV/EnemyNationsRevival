"""Launch one P1 fixture run. This is not an M0 measurement runner."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import secrets
import shutil
import socket
import subprocess
import time


class ObservedInvariant(RuntimeError):
    pass


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for block in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def read_response(client):
    response = bytearray()
    while len(response) < 512:
        part = client.recv(512 - len(response))
        if not part:
            break
        response.extend(part)
        if b"\n" in part:
            break
    if not response.endswith(b"\n") or response.count(b"\n") != 1:
        raise RuntimeError("incomplete or oversized response")
    return response.decode("utf-8", errors="strict").strip()


def scrub_inherited_en(environ):
    removed = sorted(key for key in environ if key.upper().startswith("EN_"))
    return {key: value for key, value in environ.items() if key not in removed}, removed


def observed_requested_duration(samples, requested):
    return len(samples) >= 2 and samples[-1]["monotonic"] - samples[0]["monotonic"] >= requested


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", required=True, type=Path)
    parser.add_argument("--run-dir", required=True, type=Path, help="private prepared runtime directory")
    parser.add_argument("--fixture", type=Path)
    parser.add_argument("--fixture-sha256")
    parser.add_argument("--manifest", type=Path, help="JSON fixture manifest")
    parser.add_argument("--warmup", type=float, default=0, help="excluded warm-up seconds")
    parser.add_argument("--label", default="unlabelled")
    parser.add_argument("--env", action="append", default=[], metavar="NAME=VALUE")
    parser.add_argument("--output", required=True, type=Path, help="new per-run directory")
    parser.add_argument("--seconds", type=float, default=60)
    parser.add_argument("--dummy-sdl", action="store_true")
    parser.add_argument("--camera-x", type=int)
    parser.add_argument("--camera-y", type=int)
    parser.add_argument("--speed", type=int)
    args = parser.parse_args()
    if (args.camera_x is None) != (args.camera_y is None):
        parser.error("--camera-x and --camera-y must be supplied together")
    if not math.isfinite(args.seconds) or not 0 < args.seconds <= 86400:
        parser.error("seconds must be finite and in (0, 86400]")
    if not math.isfinite(args.warmup) or not 0 <= args.warmup <= 86400:
        parser.error("warmup must be finite and in [0, 86400]")
    if not args.label or len(args.label) > 128 or not args.label.isprintable():
        parser.error("label must be 1-128 printable characters")
    manifest = None
    if args.manifest:
        if args.fixture or args.fixture_sha256:
            parser.error("--manifest cannot be combined with --fixture or --fixture-sha256")
        manifest_path = args.manifest.resolve()
        manifest_bytes = manifest_path.read_bytes()
        manifest_hash = hashlib.sha256(manifest_bytes).hexdigest()
        manifest = json.loads(manifest_bytes)
        if not isinstance(manifest, dict) or not all(isinstance(manifest.get(k), str) and manifest[k] for k in ("id", "path", "sha256")):
            parser.error("manifest needs nonempty string id, path and sha256")
        args.fixture = (manifest_path.parent / manifest["path"]).resolve()
        args.fixture_sha256 = manifest["sha256"]
        if args.camera_x is None and "camera" in manifest:
            camera = manifest["camera"]
            if not isinstance(camera, dict) or type(camera.get("x")) is not int or type(camera.get("y")) is not int:
                parser.error("manifest camera requires integer x and y")
            args.camera_x, args.camera_y = camera["x"], camera["y"]
        if args.speed is None and "speed" in manifest:
            if type(manifest["speed"]) is not int:
                parser.error("manifest speed must be an integer")
            args.speed = manifest["speed"]
    elif not args.fixture or not args.fixture_sha256:
        parser.error("supply --manifest or both --fixture and --fixture-sha256")
    expected = args.fixture_sha256.lower()
    if len(expected) != 64 or any(c not in "0123456789abcdef" for c in expected):
        parser.error("fixture sha256 must be 64 hexadecimal digits")
    overrides = {}
    reserved = {"EN_P1_TOKEN", "EN_P1_PORT", "EN_P1_PORT_FILE", "EN_P1_FIXTURE", "EN_LOG_DIR", "HOME", "USERPROFILE", "APPDATA", "LOCALAPPDATA", "TEMP", "TMP", "TMPDIR"}
    for item in args.env:
        name, sep, value = item.partition("=")
        if not sep or not name or not (name[0].isalpha() or name[0] == "_") or not all(c.isascii() and (c.isalnum() or c == "_") for c in name) or name.upper() in reserved:
            parser.error("invalid or reserved environment override")
        if name.upper() in (key.upper() for key in overrides):
            parser.error("duplicate environment override")
        overrides[name] = value
    if args.dummy_sdl and any(k.upper() in {"SDL_VIDEODRIVER", "SDL_AUDIODRIVER"} for k in overrides):
        parser.error("--dummy-sdl conflicts with SDL driver overrides")
    executable, fixture, run_dir = args.executable.resolve(), args.fixture.resolve(), args.run_dir.resolve()
    if sha256(fixture) != expected:
        raise SystemExit("fixture hash mismatch")
    args.output.mkdir(parents=True, exist_ok=False)
    out = args.output.resolve()
    (out / "tmp").mkdir()
    private_fixture = out / "fixture.en"
    shutil.copyfile(fixture, private_fixture)
    if sha256(private_fixture) != expected:
        raise SystemExit("copied fixture hash mismatch")
    port_file = out / "listener.port"
    port = None
    token = secrets.token_hex(24)
    env, removed_en = scrub_inherited_en(os.environ)
    env.update(overrides)
    env.update(EN_P1_TOKEN=token, EN_P1_PORT="0", EN_P1_PORT_FILE=str(port_file),
               EN_P1_FIXTURE=str(private_fixture), EN_LOG_DIR=str(out), HOME=str(out),
               USERPROFILE=str(out), APPDATA=str(out), LOCALAPPDATA=str(out),
               TEMP=str(out / "tmp"), TMP=str(out / "tmp"), TMPDIR=str(out / "tmp"))
    if args.dummy_sdl:
        env.update(SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
        if os.name != "nt":
            # The POSIX registry shim reads this private per-run profile.
            # Skip startup movies so headless startup has a bounded budget.
            registry = out / ".config" / "enations" / "registry.ini"
            registry.parent.mkdir(parents=True)
            registry.write_text("HKCU\\Software\\Second Chance\\Second Chance\\Game\\NoIntro=4:1\n", encoding="utf-8")
    result = {"status": "incomplete", "fixture_sha256": expected,
              "executable_sha256": sha256(executable), "samples": [],
              "label": args.label, "warmup_seconds": args.warmup,
              "measurement_seconds": args.seconds, "environment_overrides": overrides,
              "inherited_en_removed": removed_en,
              "fixture_id": manifest["id"] if manifest else None,
              "manifest_sha256": manifest_hash if manifest else None}
    process = None
    deadline = time.monotonic() + 180 + args.warmup + args.seconds

    def request(command, timeout=3):
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("run deadline")
        with socket.create_connection(("127.0.0.1", port), timeout=min(timeout, remaining)) as client:
            client.settimeout(min(timeout, remaining))
            client.sendall((token + " " + command + "\n").encode())
            response = read_response(client)
        if command == "STATUS":
            parts = response.split()
            values = [part.partition("=")[2] for part in parts[1:]
                      if part.partition("=")[0] == "assert_fires" and "=" in part]
            if not parts or parts[0] != "OK" or len(values) != 1 or not values[0].isascii() or not values[0].isdecimal():
                raise RuntimeError("missing, duplicate or invalid assert_fires")
            count = int(values[0])
            if "last_assert_fires" in result and count < result["last_assert_fires"]:
                raise RuntimeError("assert_fires counter decreased")
            result["last_assert_fires"] = count
            result["max_assert_fires"] = max(result.get("max_assert_fires", 0), count)
            if count:
                raise ObservedInvariant("invariant fires observed")
        return response

    try:
        with (out / "game.log").open("wb") as log:
            process = subprocess.Popen([str(executable)], cwd=run_dir, env=env, stdout=log, stderr=subprocess.STDOUT)
            result["pid"] = process.pid
            startup = min(deadline, time.monotonic() + 90)
            while True:
                if process.poll() is not None:
                    raise RuntimeError("game exited before ready")
                try:
                    if port is None:
                        try:
                            published = port_file.read_text(encoding="ascii").strip()
                        except FileNotFoundError:
                            published = ""
                        if published:
                            if not published.isdecimal() or not 1024 <= int(published) <= 65535:
                                raise RuntimeError("invalid published listener port")
                            port = int(published)
                    if port is not None and request("READY") == "OK protocol=1 fixture=fixture":
                        break
                except OSError:
                    pass
                if time.monotonic() >= startup:
                    raise TimeoutError("startup deadline")
                time.sleep(.2)
            answer = request("LOAD fixture", timeout=150)
            if answer != "OK loaded":
                raise RuntimeError("load: " + answer)
            # LOAD completes on the main thread after its previous snapshot;
            # wait for a fresh in-game snapshot before starting the interval.
            ready_deadline = min(deadline, time.monotonic() + 30)
            while True:
                state = request("STATUS")
                if "ingame=1" in state.split():
                    break
                if time.monotonic() >= ready_deadline:
                    raise TimeoutError("in-game state deadline")
                time.sleep(.2)
            if args.camera_x is not None:
                actual = request(f"CAMERA {args.camera_x} {args.camera_y}")
                if actual != f"OK camera_x={args.camera_x} camera_y={args.camera_y}":
                    raise RuntimeError("camera readback: " + actual)
                result["camera_readback"] = actual
            if args.speed is not None:
                actual = request(f"SPEED {args.speed}")
                if actual != f"OK speed={args.speed}":
                    raise RuntimeError("speed readback: " + actual)
                result["speed_readback"] = actual
            # Control acknowledgements precede the next published snapshot.
            # Do not lock measurement settings to a stale speed/camera.
            sync_deadline = min(deadline, time.monotonic() + 30)
            while True:
                state = request("STATUS")
                state_fields = dict(part.split("=", 1) for part in state.split()[1:] if "=" in part)
                expected_settings = {}
                if args.camera_x is not None:
                    expected_settings.update(camera_x=str(args.camera_x), camera_y=str(args.camera_y))
                if args.speed is not None:
                    expected_settings["speed"] = str(args.speed)
                if state_fields.get("ingame") == "1" and all(state_fields.get(k) == v for k, v in expected_settings.items()):
                    break
                if time.monotonic() >= sync_deadline:
                    raise TimeoutError("settings snapshot deadline")
                time.sleep(.1)
            result["pre_warmup"] = {"response": state, "monotonic": time.monotonic()}
            if "ingame=1" not in result["pre_warmup"]["response"].split():
                raise RuntimeError("game left before warmup")
            warmup_start = time.monotonic()
            warmup_end = warmup_start + args.warmup
            while time.monotonic() < warmup_end:
                if "ingame=1" not in request("STATUS").split():
                    raise RuntimeError("game left during warmup")
                time.sleep(min(5, max(0, warmup_end - time.monotonic())))
            result["warmup_elapsed_seconds"] = time.monotonic() - warmup_start
            baseline = request("STATUS")
            result["post_warmup"] = {"response": baseline, "monotonic": time.monotonic()}
            settings = dict(part.split("=", 1) for part in baseline.split()[1:] if "=" in part)
            if settings.get("ingame") != "1" or settings.get("camera_available") != "1":
                raise RuntimeError("camera unavailable before measurement")
            if any(settings.get(k) != v for k, v in expected_settings.items()):
                raise RuntimeError("requested settings changed during warmup")
            locked = {key: settings[key] for key in ("camera_x", "camera_y", "speed")}
            result["measurement_settings"] = locked
            result["mark_begin"] = request("MARK BEGIN")
            if not result["mark_begin"].startswith("OK mark=begin "):
                raise RuntimeError("measurement begin rejected: " + result["mark_begin"])
            end = None
            while True:
                sample = request("STATUS")
                sample_fields = dict(part.split("=", 1) for part in sample.split()[1:] if "=" in part)
                if sample_fields.get("ingame") != "1" or sample_fields.get("camera_available") != "1" or any(sample_fields.get(k) != v for k, v in locked.items()):
                    raise RuntimeError("measurement settings changed")
                observed = time.monotonic()
                result["samples"].append({"monotonic": observed, "response": sample})
                if end is None:
                    end = observed + args.seconds
                if time.monotonic() >= end:
                    break
                time.sleep(max(0, min(5, end - time.monotonic(), deadline - time.monotonic())))
            result["mark_end"] = request("MARK END")
            if not result["mark_end"].startswith("OK mark=end "):
                raise RuntimeError("measurement end rejected: " + result["mark_end"])
            # Include assertions that fired after the last periodic sample and
            # before the measurement-end marker. Teardown remains separately
            # unobserved by this STATUS check.
            result["post_mark_status"] = request("STATUS")
            if request("QUIT") != "OK quitting":
                raise RuntimeError("quit rejected")
            # Large saves can need more than ten seconds for cooperative AI
            # shutdown. Keep the overall deadline and require a clean exit.
            result["exit_code"] = process.wait(timeout=max(0.1, min(30, deadline - time.monotonic())))
            if result["exit_code"] != 0:
                raise RuntimeError("game did not exit cleanly")
            if sha256(private_fixture) != expected or sha256(fixture) != expected:
                raise RuntimeError("fixture changed")
            if manifest and sha256(manifest_path) != manifest_hash:
                raise RuntimeError("manifest changed")
            def fields(sample):
                return dict(part.split("=", 1) for part in sample["response"].split()[1:] if "=" in part)
            first, last = fields(result["samples"][0]), fields(result["samples"][-1])
            log_text = (out / "game.log").read_text(encoding="utf-8", errors="replace")
            elapsed = result["samples"][-1]["monotonic"] - result["samples"][0]["monotonic"]
            result["qualification_checks"] = {
                "observed_60_seconds": elapsed >= 60,
                "observed_requested_seconds": observed_requested_duration(result["samples"], args.seconds),
                "in_game": first.get("ingame") == "1" and last.get("ingame") == "1",
                "player_census": int(last.get("players", "0")) > 0,
                "unit_census_present": "buildings" in last and "vehicles" in last,
                "game_frame_advanced": int(last.get("game_frame", "0")) > int(first.get("game_frame", "0")),
                "game_time_advanced": int(last.get("game_seconds", "0")) > int(first.get("game_seconds", "0")),
                "serialize_marker": "[P1-HARNESS] Serialize complete" in log_text,
                "ai_load_marker": "[P1-HARNESS] AiLoadGame complete" in log_text,
                "no_reported_invariant_fires": result["max_assert_fires"] == 0,
                "no_ignored_assertions": "[ASSERT-IGNORED]" not in log_text,
                "no_teardown_stragglers": "straggler(s)" not in log_text,
            }
            result["status"] = ("qualified" if all(result["qualification_checks"].values())
                                else "qualification-failed")
    except Exception as error:
        result["error"] = str(error)
        if isinstance(error, ObservedInvariant) and process is not None and process.poll() is None:
            try:
                result["abort_quit_response"] = request("QUIT")
                if result["abort_quit_response"] != "OK quitting":
                    raise RuntimeError("quit rejected")
                result["abort_exit_code"] = process.wait(timeout=max(0.1, min(30, deadline - time.monotonic())))
            except Exception as quit_error:
                result["abort_quit_error"] = str(quit_error)
    finally:
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        (out / "result.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(result["status"])
    return 0 if result["status"] == "qualified" else 1


if __name__ == "__main__":
    raise SystemExit(main())
