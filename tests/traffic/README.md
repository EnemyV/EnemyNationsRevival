# Traffic mechanism checks

Run from the repository root on Windows with Visual Studio 2022 Community:

```powershell
python tests/traffic/run-remote-loc-test.py
python tests/traffic/run-remote-loc-test.py --clearance
python tests/traffic/run-remote-loc-test.py --parking
```

The runner compiles the actual production method bodies against minimal scene
dependencies. The default suite checks remote facing, interpolation fields, local
facing controls, and moving-versus-explicitly-stopped path occupancy. The clearance
suite checks eligibility, physical contact, ownership boundaries, retained wait
targets, and expiry of cyclic requests. Its timer constants come from vehicle.h.
The parking suite checks bounded path searches and continuation, failed-request
throttling, remote blocker classification, and the civilian flee target guard.

Use `--baseline-ref <commit>` to run the same assertions against older source.
For example, bfad1d28 fails four explicit-Stop checks that bd62368e fixes.

These are mechanism tests, not full-engine or multiplayer tests. They do not
prove that a target can be reached, that every jam clears, or that interpolation
renders correctly across two connected clients. Keep fresh original-clog-save
replays, native geometry/hold/delivery audits, and client save/load checks separate.
