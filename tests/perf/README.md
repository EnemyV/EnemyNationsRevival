# P1 fixture harness (work in progress)

This code is a P1 build derived from `src-p0-017`, not SRC-P0 or M0-BUILD.
The loopback listener is disabled unless `EN_P1_TOKEN` (16-128 alphanumeric
characters), `EN_P1_PORT`, and `EN_P1_FIXTURE` are all present. A fixed port
must be 1024-65535; port 0 requires `EN_P1_PORT_FILE` for publication.
Each line is `<token> READY`, `<token> LOAD fixture`, `<token> STATUS`,
`<token> MENU`, or `<token> QUIT`; the client cannot choose a path over the
socket. Load, menu, and quit are serviced on the main loop. The listener serializes clients, bounds request
length and I/O time, and joins its thread on application shutdown.

`CAMERA x y` sets the top area's center in world map units and returns the
actual center; `SPEED n` sets and returns the single-player speed index
(0 through 16; network speed changes are rejected). `STATUS` includes both
readbacks. `MARK BEGIN` and `MARK END` bracket one in-game window and return
steady-clock nanoseconds, game frame, game seconds and speed. These are run
boundaries, not P2 timing metrics. Marks cannot be nested or ended twice.

`run_fixture.py` accepts a separately prepared **private** runtime directory,
an executable, a fixture and its SHA-256, and a new output directory. It creates
an isolated fixture copy and profile/temp paths, chooses a loopback port and
token, owns only its child process, and records status samples and a result.
`run_fixture.ps1` is its Windows entry point. `--dummy-sdl` is available for
headless environments. Optional `--camera-x`, `--camera-y` and `--speed` set
and check readbacks; the runner records both measurement marks. Qualification requires the real-game load markers,
advancing simulation time and census, unchanged fixture hash, clean exit, and
no ignored assertion or teardown stragglers. Consult STATE-017.md for platform-specific run evidence and remaining gates.

`test_listener.py` plus `listener_stub.cpp` test the listener protocol without
SDL or the game. They do not validate the load seam or game behavior.

The runner accepts `--manifest fixture.json` instead of the direct `--fixture`
and `--fixture-sha256` pair. The minimum JSON fields are `id`, `path` (relative
to the manifest, or absolute), and `sha256`; optional `camera: {x, y}` is in
world map units and `speed` is the single-player speed index. This is a runner
input schema, not yet the complete P3 frozen-fixture manifest. CLI camera and
speed, when supplied, override manifest values and are recorded in readbacks.
`--warmup` excludes seconds before BEGIN; `--seconds` specifies measurement
seconds (up to 86400); `--label` and repeatable `--env NAME=VALUE` are recorded
in result.json. Reserved harness/profile variables cannot be overridden. A
120-second warm-up plus 600-second measurement is supported. The PowerShell
entry point exposes the same inputs, including `-EnvOverride`.

During a marked interval, harness CAMERA and SPEED commands are rejected; the
game checks camera and speed at each harness service/snapshot and at END, and
END rejects any observed change. The runner also verifies every STATUS sample.
This detects changes visible on the game-loop service boundary; it does not
prove the absence of a change that is made and reversed between those checks.
Run noninteractive and treat any external interaction as invalidating a series.
These functions are not an M0 measurement collector or a frozen M0 build.
The runner requests an OS-assigned loopback listener port. The listener keeps
its socket bound and atomically publishes the selected port into a private
per-run file, avoiding bind-close-rebind port selection. Responses are read
through a bounded newline and the listener retries partial sends; truncated or
oversized responses fail the run. Fixed-port listener operation remains for
manual protocol tests.

Inherited `EN_*` variables are removed from the game environment; their names
are recorded as `inherited_en_removed`, and only explicit `--env` overrides
are reintroduced. Harness-owned values are still generated per run. Game-side
resolved configuration remains a P2 requirement. Results record STATUS and
monotonic timestamps immediately before and after warm-up. Qualification
requires both the 60-second bootstrap minimum and the requested sample span;
mark and sample span interpretation remains a pre-registration decision.
