# Traffic mechanism checks

Run from the repository root on Windows with Visual Studio 2022 Community:

```powershell
python tests/traffic/run-remote-loc-test.py
python tests/traffic/run-remote-loc-test.py --clearance
python tests/traffic/run-remote-loc-test.py --parking
python tests/traffic/run-remote-loc-test.py --junction
python tests/traffic/run-remote-loc-test.py --junction --o2
```

The runner compiles the actual production method bodies against minimal scene
dependencies. The default suite checks remote facing, interpolation fields, local
facing controls, and moving-versus-explicitly-stopped path occupancy. The clearance
suite checks eligibility, physical contact, ownership boundaries, retained wait
targets, and expiry of cyclic requests. Its timer constants come from vehicle.h.
The parking suite checks bounded path searches and continuation, failed-request
throttling, remote blocker classification, and the civilian flee target guard.
It also checks that saved recovery cannot run on remote copies, that ownerless
blockers are safe, and that local arrival/hold behavior remains enabled.

The junction suite compiles the whole of `GetNextHex` - plus `Rotate`, `GetAngle` and
`aiBaseDir`, so the angle limit is the real one - and drives two trucks round a paved
bend. Both decide from the same scene state in the same tick, and what each ASKS for is
recorded; nothing is reserved or interpolated, and a pair whose requests coincide stops
there. It covers all four rotations of the bend in both travel directions, and requires
the step to be unchanged off pavement, on a bridge deck, where `MustKeepLane` governs,
within three sub-hexes of the destination, while reversing, and for a straight step
through a crossroads. `--o2` compiles the fixture optimised; both must pass. The
correction is toggled through `EN_TRAFFIC` bit 16, which is how one binary prints the
before and after columns.

Use `--baseline-ref <commit>` to run the same assertions against older source.
For example, bfad1d28 fails four explicit-Stop checks that bd62368e fixes.

These are mechanism tests, not full-engine or multiplayer tests. They do not
prove that a target can be reached, that every jam clears, or that interpolation
renders correctly across two connected clients. Keep fresh original-clog-save
replays, native geometry/hold/delivery audits, and client save/load checks separate.
