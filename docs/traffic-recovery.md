# Traffic recovery

Automatic ground trucks and inactive cranes try to keep two-way corridors flowing.
Roads have two lanes and four subhexes per hex. A two-subhex vehicle still needs
space for both ends of its body.

The existing vehicle update handles recovery locally:

- Wait about one second (24 game frames) behind a moving blocker, then retry the
  actual blocked step. Waiting does not authorize a turn into the opposing queue.
- If ordinary movement cannot recover, reverse physically, keeping the nose
  direction consistent with the body. Touching eligible vehicles can pass a
  temporary request to make space; each chooses its own legal movement.
- Clear the whole body off the road before holding. Resume the saved job after
  the hold, with staggered timing. A new player order replaces the saved job.
  If the parking route fails and stops with the body on the road, cancel the
  unusable hold and retry the saved job; do not wait indefinitely on pavement.
- Idle automatic trucks and cranes try to leave pavement, bridges and city tiles.
  Player Stop, manual control and active construction are respected.

Combat fleeing may use reachable pavement as an emergency destination. Ordinary
off-road parking runs after arrival; a new combat-flee loop is unnecessary. Ground
flee targets must remain traversable, outside buildings and reachable.

Parking checks at most eight terrain paths per invocation. A transient per-vehicle
cursor lets a later attempt continue beyond failed candidates. Movement, a new
destination, success or an exhausted scan resets the cursor. Failed courtesy
requests also consume their five-second cooldown. No unchecked parking target is
accepted, and there is no shared manager, reservation allocator or map sweeper.

A remote vehicle's interpolation can stop while awaiting another location packet.
That transient mode does not qualify it for the fixed-blocker passing exception.
Only the locally authoritative owner runs arrival recovery and hold/resume logic;
physical map occupancy alone is not authority, including after loading a save.
Full two-peer behavior still needs runtime verification.

Save format8 stores the interrupted job, movement sense and recovery budgets.
Earlier saves remain loadable. Builds that only understand format7 cannot read
new format8 saves; coordinate this with other release serialization changes.

## Diagnostics

Keep both facilities available for development. CMake can remove them from a build:

```powershell
cmake -S . -B cmakeBuild-x64 -DEN_BUILD_HARNESS=OFF -DEN_TRAFFIC_PROBES=OFF
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Release -x64
```

Both options default ON to preserve developer builds. With them ON, the harness
still requires runtime `EN_HARNESS`; traffic event logging requires `EN_WAIT_LOG`.
With them OFF, the socket/game-side harness and traffic log calls are compiled out.
Other existing engine probes retain their separate switches in `enprobes.h`.

## Verification

See [native mechanism checks](../tests/traffic/README.md). Use the original clogged
`savegame012-v9.en` for traffic comparisons, never a save made after recovery.
The discussion board holds per-build replay, geometry, hold, delivery, performance
and client save/load evidence. Passing a finite replay does not prove all corridor
shapes or permanent freedom from jams. Separate empty-route stalls and combined015
integration remain outside the verified bridge result.
