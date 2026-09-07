//---------------------------------------------------------------------------
// en_harness.h — in-process LLM-driving harness (Windows / Linux / macOS).
//
// A small TCP control server compiled into the game. It lets an external client
// (thin shell/python scripts) screenshot the running game, inject mouse/key
// events, and query live game state — the Wayland-safe, focus-free counterpart
// to the Windows PostMessage/PrintWindow .ps1 harness (which stays: it is still
// the better image-capture path on Windows). Input is injected via SDL_PushEvent
// (thread-safe); state reads and screenshots are serviced on the main/render
// thread via EnHarness_Service().
//
// EVERYTHING here is inert unless EN_HARNESS is set in the environment.
// EnHarness_Start returns before touching the socket library or creating a
// thread, and the per-frame hooks below early-out on one relaxed atomic load.
//---------------------------------------------------------------------------
#ifndef EN_HARNESS_H
#define EN_HARNESS_H

#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Surface;

// Start the control server if the EN_HARNESS env var is set. Safe to call once
// after the game window exists. Port comes from EN_HARNESS_PORT (default 7070).
void EnHarness_Start(SDL_Window* window, SDL_Renderer* renderer);

// Call every frame on the main thread (e.g. from GameWindow::PollEvents). Services
// pending screenshot requests, which must touch SDL on the render thread.
void EnHarness_Service();

// Call once per main-loop iteration from the TOP of the loop (CConquerApp::Run),
// BEFORE event-pumping/render. Services harness ops that themselves pump the event
// loop (currently `save` — CGame::SaveGame re-pumps via BaseYield + shows a
// progress dialog), which must NOT run re-entrantly from the render/PollEvents path.
void EnHarness_ServiceMainLoop();

// Register the main window's CPU back-buffer (GameWindow::GetPresentSurface in
// renderer mode). When set, `shot` of the main window dumps this surface directly
// instead of SDL_RenderReadPixels — needed on macOS, where the Metal/GL render
// target reads back blank, and on headless sessions with no on-screen drawable.
// The compositor draws the full frame into this surface every present, so it
// always holds the real composited image. Pass nullptr to clear.
void EnHarness_SetMainSurface(SDL_Surface* surface);

// Register a detached panel's CPU back-surface by its SDL window id, so `shotid`
// can dump it directly (reliable on macOS, where GPU read-back of child windows
// is blank/garbage). Call each frame from the panel's render. Pass nullptr to
// clear (e.g. on panel destroy).
void EnHarness_RegisterWindowSurface(unsigned int windowId, SDL_Surface* surface);

// Game-side unit enumerator (implemented in area.cpp — needs theVehicleMap/
// theBuildingMap/CWndArea, which the harness TU can't see). Appends one line per
// unit to `out`:  "<id> <screenX> <screenY> <kind> <me|other> <hexX> <hexY>\n"
// where screenX/Y are area-window pixels (feed clickid/dblclickid) and kind is
// crane|transport|carrier|infantry|vehicle|building (buildings carry a build-state
// word before the hex). Vehicle lines carry seven more trailing ints, appended
// after the original seven so positional parsers keep working:
// "<mode> <event> <destHexX> <destHexY> <inbldg 0|1> <oppo id or 0> <hp>"
// (CVehicle::VEH_MODE / VEH_EVENT; inbldg = head on a building hex = undrawn;
// oppo = CUnit::GetOppo() id, 0 = none; hp = GetDamagePoints()). Vehicle rows
// therefore END with <oppo id or 0> <hp>. Used by the `units` control_socket command
// (Linux/mac) and a Windows debug hotkey (same fn) to make crane/unit location
// deterministic instead of a blind dblclick-sweep. Call on the game/render thread.
void HarnessDumpUnits(std::string& out);

// Report the CURRENT SELECTION (count + primary unit description) from live game
// state. Added because there is no way to read "what is selected" on Linux: the
// area window's title carries it on Windows only, `textid` returns empty, and
// pixel-diffing the selection bracket proved unreliable (a building's signature
// does not even toggle). Read-only; no game behaviour touched.
void HarnessDumpSelection(std::string& out);

// Dump ALL buildings (mine + AI) with combat/construction state, so a headless
// driver can verify fire-control fixes (e.g. #60: a finished+stopped armed camp
// must compute fireRate>0) without driving live combat. Backs `bldgstate`.
// One line per building:
//   "<id> t<type> <me|other> cd<constDone> stop<0|1> bfr<baseFR> fr<fireRate> ev<0|1>"
// ev = CUnit::event, the materials-halt / waiting flag, appended LAST so positional
// parsers keep working; ResumeUnit clears it alongside stopped, so an idle building
// with stop0 is explained by this field and nothing else in the line.
// Render/game thread only.
void HarnessDumpBldgState(std::string& out);

// Per-player AI economy/population probe (T-0068 AI-stall investigation). READ-ONLY:
// pop, workforce have/need, m_fPplMult (understaffing), and per-player building tallies
// (apartments / offices / camps / refinery / oil) + how many of the 7 housing-gate
// prerequisites each player owns. Backs the `aistate` verb. Render/game thread only.
void HarnessDumpAIStates(std::string& out);

// List every civ-wide edict + whether it's active for the local player, so a QA
// driver can verify an edict toggle (read state, click the checkbox, read again).
// Backs `edicts`. Render/game thread only.
void HarnessDumpEdicts(std::string& out);

// List every AltOutput-capable building owned by the local player (id, alt_oil on/off, mode,
// label) so a headless driver can find a coal plant / BioFuel / charcoal / fracking building
// by id directly — a 500+ stacking-window showinfo scan is impractical. Backs `altbldgs`.
// Render/game thread only.
void HarnessDumpAltBuildings(std::string& out);

// Dump the local player's live colony stats (workforce/power/food need+have) so a QA
// driver can read an EXACT before/after delta that the in-game readouts clip (e.g. the
// Office "Workforce Need" 4th digit is hidden behind the history graph). Backs `pstats`.
// Render/game thread only.
void HarnessDumpPlayerStats(std::string& out);

// Center the focused area view on the unit with this id (vehicle or building),
// so a headless driver can then click view-center to select it — sidesteps the
// view-relative/wrapped screen coords from HarnessDumpUnits. Returns false if no
// area window or no unit with that id. Call on the game/render thread (mutates
// the view). Backs the `center <id>` control_socket command + (future) a Windows
// trigger. Pairs with `units`:  units -> pick a crane id -> center <id> ->
// clickid <area> <center> -> keyid <area> 98 (Build).
bool HarnessCenterUnit(unsigned long id);

// Open a building's read-only info window by unit id (CBuilding::ShowInfoWindow) so
// a QA driver can deterministically open a specific building (e.g. an edict host)
// and clickid its widgets. Backs `showinfo <bldgid>`. Render thread only.
bool HarnessShowInfoWindow(unsigned long id);

// Set/clear a building's alt_oil (AltOutput) flag directly by unit id, bypassing the
// info-window checkbox — QA verification of AltOutput effects via pstats. Backs `setalt`.
// Render/game thread only.
bool HarnessSetAltOil(unsigned long id, bool on);

// Set/clear a civ-wide edict for the local player directly by edict id (0..EDICT_COUNT-1),
// bypassing the info-window checkbox — deterministic QA of edict-downside deltas (workforce/
// power/food upkeep) via pstats, no per-window click-coord hunting. Routes through the real
// ToggleEdictNet path (ToggleEdict + RecomputeEdictMults). Backs `setedict`. Render/game thread.
bool HarnessSetEdict(int edictId, bool on);

// Report the map hex under an area-window client pixel (same coords as clickid):
// appends "hex <hx> <hy> alt <n> vis <0|1> unit <0|1> water <0|1> tree <n>\n" to
// `out` (or "err ..."). READ-ONLY (no game/view mutation). Lets a headless driver
// find a BUILDABLE footprint (water 0, tree 0, matching neighbour alts = flat, vis 1,
// unit 0) or a slope (adjacent hexes with differing alt) deterministically instead of
// eyeballing the placement OK/no-build cursor sprite. NOTE water/tree matter: a flat,
// unit-free, visible hex still fails FoundationCost if it's water or trees (trees are
// not "units") — that was the hidden placement-capture blocker. Backs the
// `hexinfo <areaWin> <x> <y>` cmd. Render thread only.
void HarnessHexInfo(int x, int y, std::string& out);

// Scan the map for the first hex of terrain type <id> (CHex terrain enum: lake=3,
// ocean=6, river=8, swamp=11, ...); if adjId>=0 require a 4-neighbour of that type
// (e.g. river adjacent to lake = the #8 blend boundary). Centers the focused area
// view on the found hex so the caller can shotid the area window. Backs `findterr
// <id> [adjId]`. Render thread only (reads map + mutates the view).
void HarnessFindTerrain(int id, int adjId, std::string& out);

// Center the focused area view on an arbitrary hex (x,y) for a headless pixel
// eyes-on. Backs `centerhex <x> <y>`. Render thread only (reads map + mutates view).
void HarnessCenterHex(int x, int y, std::string& out);

// Order the player's first owned crane to build a road from hex (x1,y1) to (x2,y2),
// driving CVehicle::SetRoad directly (the same commit the road_set mouse-release does)
// — the crane road-build gesture (press-R + drag) can't be delivered headlessly
// (keyboard focus + capture-drag). Backs `road <x1> <y1> <x2> <y2>`. Mutates game
// state → serviced on the main/render thread like center/hexinfo.
void HarnessBuildRoad(int x1, int y1, int x2, int y2, std::string& out);

// List every bridge hex (CHex::bridge unit bit) as `bridge <x> <y> vis <0|1>
// seen <0|1>`, then center the focused area view on the first NEVER-SEEN one
// (vis=0 seen=0; else the first found). Backs `findbridge` = the BUGS #30
// bridge-fog verify (a never-seen bridge must not draw until scouted). Read-only
// of the map; mutates only the view. Render thread only.
void HarnessFindBridge(std::string& out);

// Scroll the focused area view by a pixel delta (grab-style: positive dx/dy move
// the view center right/down). Drives the same PanByPixels path as the macOS
// trackpad two-finger pan, so a headless driver can verify the scroll mechanic
// (arrow-key scroll doesn't reach the area map through the offscreen focus path).
// Backs `pan <dx> <dy>`. Render/game thread only (mutates the view).
bool HarnessPan(int dxPix, int dyPix);

// Save the current in-game state to <path> (a .en save file) headlessly — no
// file-browser modal (CGame::SaveGame skips it when the filename is pre-set).
// Lets a headless driver snapshot a DEVELOPED/researched game so it can be shared
// (one such save unblocks research-gated work team-wide). Returns true on a
// written save; false if not in-game. Call on the game/render thread (touches UI +
// game state). Backs the `save <path>` control_socket command.
bool HarnessSaveGame(const char* path);

// Load a .en save <path> headlessly from the MAIN MENU — runs the normal single-
// player load flow (SDL2_RunLoadSinglePlayerFlow) but skips its two modals: the
// file-browser (uses <path> directly) and the pick-player dialog (auto-selects the
// human, theGame._GetMe(), which that dialog already defaults to). Lets a headless
// driver CONSUME a shared developed save (the menu file-browser isn't harness-
// drivable on POSIX). Returns true on a loaded+started game. Call from the main
// loop (it re-pumps events like save). Backs the `load <path>` control_socket cmd.
bool HarnessLoadGame(const char* path);

// While a headless load is in progress, returns the target .en path; else nullptr.
// CGame::LoadGame and SDL2_RunLoadSinglePlayerFlow check this to take the headless
// (no-modal) path. Always nullptr during a normal menu-driven load (unaffected).
const char* HarnessPendingLoadPath(void);

// DEV/harness (SP only): discover ALL research for the local human player instantly
// (CPlayer::DebugDiscoverAllResearch) — unblocks the research-gated tail (AltOutput
// toggles, fort/seaport/shipyard/heavy-factory/embassy, edicts) without the multi-
// hour grind. POSIX analogue of win's Windows F12 hotkey. Returns false if not
// in-game or not single-player (MP would desync). Backs the `research` cmd.
bool HarnessGrantResearch(void);

// Start a fresh SINGLE-PLAYER game headlessly from the MAIN MENU — mirrors
// SDL2_RunCreateSinglePlayerFlow but takes params instead of the create + pick-race
// modals, so an autonomous driver can start e.g. a HARD, Full-Military game with no UI.
//   ai    = AI difficulty 0..3  (0 Easy, 1 Moderate, 2 Difficult/HARD, 3 Impossible)
//   pos   = starting force 0..3 (0 Minimal Civilian .. 3 Full Military)
//   size  = world size 0..2     (0 Small, 1 Medium, 2 Large)
//   numai = number of AI opponents (>=1)
// Uses race 0 and name "mac2". Must run from the main loop (world-gen re-pumps events)
// and only at the menu (no game in progress). Returns true once the game is in play.
// Backs the `newgame` control_socket cmd.
bool HarnessNewGame(int ai, int pos, int size, int numai, int worldType = 0, int ocean = 50, int rivers = 60);

// Dump the local human's single-player game-over / progress state (READ-ONLY):
//   "state <n> hp <0|1> bldgshave <n> vehshave <n> bldgsdest <n> vehsdest <n> \
//    players <n> ai <n> elapsed <sec> <playing|LOST|WON>\n"
// Defeat = bldgshave<=0 or CGame state left `play` (== CGame::other); win = <=1 player
// left. Lets a headless driver poll "am I still alive" each tick.
//   *have = what this player currently HOLDS  -> watch these FALL to detect losses.
//   *dest = kills scored BY this player       -> NOT damage taken. The only increment
//           site (netapi.cpp, "track the kill") is guarded by pPlr != pUnit->GetOwner(),
//           so it credits the killer. A driver watching vehsdest for "am I under attack"
//           reads 0 while being wiped out (observed in a live-combat soak: vehshave
//           35 -> 5 with vehsdest stuck at 0).
// Render/game thread only. Backs the `gamestate` cmd.
void HarnessDumpGameState(std::string& out);

// Per-local-player traffic census, one line per player:
//   "traffic plyr N ai 0|1 trucks N cranes N moving N blocked N stop N cantdeploy N traffic N
//    inbldg N inbldg_notdest N r0_4 N r5_9 N r10_14 N r15p N maxstag_ms N inbldg_wrong N
//    maxwrong_ms N oldstamp_ms N oldstamp_veh N oldstamp_mode N oldstamp_type N
//    oldstamp_hex X,Y contention N deployit N run N aiticks N scans N skipb N
//    scanms_avg N scanms_max N walks N walksdone N cand_avg N cand_max N
//    opfor_avg N opfor_max N tgtheld N scanspm N seekloops N orders_ok N
//    orders_wake N steps N deliveries N stranded N stranded_ms N sweep_ms_max N
//    path_e_free N path_e_aware N path_rp_free N path_rp_aware N
//    path_d_free N path_d_aware N path_ap_free N path_ap_aware N
//    path_f_free N path_f_aware N"
// inbldg_notdest = vehicles inside a building that is neither their destination nor their
// construction site (the DOORSTEP hypothesis, docs/plans/015-focus-investigation.md 1.2);
// inbldg_wrong = vehicles still inside a building they entered wrongly (stamped by
// [ENTERWRONG], cleared by ExitBuilding); maxwrong_ms = the longest such dwell;
// r* = blocked vehicles bucketed by HandleBlocked retry rung (>= 11 = the turn/180 rungs).
// maxstag_ms is over BLOCKED vehicles ONLY. oldstamp_* is the oldest stagnation stamp in
// ANY mode and who holds it (id / VEH_MODE / vehicle type / head hex, all 0 when none):
// nothing clears the stamp on the way out of blocked, so a leaked stamp used to inflate
// maxstag_ms forever - the two are now separate numbers. contention/deployit/run are the
// three VEH_MODE values that previously fell into the switch default and were invisible.
// aiticks = that player's AI Manage() counter (ai.cpp g_alAiManageTicks), 0 for humans:
// frozen while vehicles are alive = a dead AI thread, not a jam.
// The seek-scan block (all 0 for humans) answers "is CAIGoalMgr::GetOpForUnitScan why
// AssignUnits never finishes a walk, so trucks are never dispatched":
//   scans / skipb        = GetOpForUnitScan calls run / declined by the per-walk budget
//   scanms_avg, _max     = cost of one scan, ms (raw timeGetTime around the call)
//   walks / walksdone    = AssignUnits entries vs normal exits. walks climbing while
//                          walksdone stands still IS the stall - not a traffic jam
//   cand_avg, cand_max   = per-candidate AssessThreat/AssessTarget cs takes per scan.
//                          The row-hold change hoists the per-HEX takes only; a flat
//                          lock-wait number cannot be read without knowing this
//   opfor_avg, opfor_max = CAIUnitList::GetOpForUnit calls per scan. It takes cs on
//                          every index miss and allocates under it, and is unchanged,
//                          so "one acquisition per row" holds only while this is small
//   tgtheld              = seek units seen holding a target (DataDW != 0) so far in the
//                          current/last walk; published as it counts, so a walk that
//                          never finishes still reports what it saw
//   scanspm              = scans per minute over the window since the previous census
//                          call for that player (0 on the first sample)
//   seekloops            = SeekOpfor wedges caught by the [SEEKLOOP] instrument: the
//                          same target id rejected on two consecutive turns of one
//                          call's `goto SeekNDestroy` loop (caitmgr.cpp). One bump
//                          per wedge, NOT per loop turn - non-zero here while
//                          aiticks is frozen and scans climb IS the spin. The
//                          matching [SEEKLOOP] line (OutputDebugString + traffic.log)
//                          names the id and what the unfiltered maps hold for it
// The R10 denominator block (015 T2 item 4) is EXACT event counts, deliberately kept
// apart from the throttled/sampled detail lines - a rate needs a denominator that is
// not itself throttled. All of them are cumulative since process start and only move
// while EN_TRAFFIC_LOG is set:
//   orders_ok    = CAIUnit::SetDestination orders that survived the 30 s dedupe AND
//                  the same-location drop and reached theGame.PostToServer
//   orders_wake  = the subset posted although the target hex is where the unit already
//                  stands (the deliberate truck "wake"): separate, because a wake asks
//                  for no movement and would otherwise flatter the order rate
//   steps        = physical sub-step completions (CVehicle::ArrivedNextHex, at the line
//                  where the head sub advances). Placement, carried movement and the
//                  stuck teleport are excluded because they never reach that line
//   deliveries   = truck unloads completed at BOTH routers: the AI path
//                  (CAIMgr::DestinationResponse -> CAIRouter::UnloadMaterials) and the
//                  human auto-router path (CHPRouter::DestinationResponse ->
//                  CHPRouter::UnloadMaterials, chproute.cpp - both its normal branch
//                  and its post-restore branch). It is NO LONGER AI-only: a human row
//                  of 0 now means no deliveries, not an absent counter
//   stranded     = vehicles right now in stop/blocked that still hold a destination,
//                  a pending arrival event or an unfinished route
//   stranded_ms  = that cohort INTEGRATED over elapsed census intervals, in
//                  vehicle-milliseconds (headcount x interval, accumulated). It is not
//                  a sum of current ages, so calling `traffic` more often does not
//                  inflate it; the first sample for a player has no window and adds 0
//   sweep_ms_max = longest CAIMgr::HandleStuckVehicles call for that player, ms, as
//                  measured by the [SWEEP] enter/exit pair in traffic.log
// New fields are all APPENDED, so positional parsers keep working.
// Same text the 30 s [CENSUS] line writes to traffic.log when EN_TRAFFIC_LOG is set.
// The same pass also emits [STUCK] to traffic.log for every blocked vehicle stagnant
// over 60 s, naming the vehicle occupying the hex it wants next, and now also carrying
// `lastsub <seq|unknown> lastsub_age_ms <ms|stale|unknown>` (the last real
// CVehicle::FindSub attempt on that vehicle, from the [SUBATTEMPT] record) plus
// `ctx` = the eight sub-hex neighbours of the head as `<dx>,<dy> t<terrain> b<bldgid>
// v<vehid> m<mode> s<self>`, in the fixed order (-1,-1)(0,-1)(1,-1)(-1,0)(1,0)(-1,1)
// (0,1)(1,1). The ctx block is a SNAPSHOT, not a cause: it is taken at census time,
// not at the failed attempt, terrain is printed for every neighbour (an empty passable
// grass sub has a terrain type too), and an occupant does not rule out a simultaneous
// terrain/door constraint. `stale`/`unknown` mark exactly when the two must not be
// read together. No cause label is emitted and none should be inferred.
// traffic.log also carries [SUBATTEMPT], one line per FindSub failure TRANSITION (the
// first failure after a run of successes, never per frame), with the direction-by-
// direction candidate + first-rejecting-gate list, and [SWEEP] enter/exit pairs tagged
// `idle` (AI idle-function rotation) or `wall` (240 s wall-clock gate).
// The GetPath return-class block (015 T2b item 1) classifies EVERY exit of
// CVehicle::GetPath exactly once, from state that function has already computed - no
// second pathfind and no re-query of the map. The ten fields sum to the number of
// GetPath calls for that player, which is what makes them a denominator.
// SUFFIX (renamed 015 R11 item 3 - the T2b `_a`/`_f` pair named these BACKWARDS):
// GetPath's bNoOcc is handed straight to CPathMgr::GetPath's `bVehBlock`, documented
// at cpathmgr.h:134-135 as "default (FALSE) means that path goes thru vehicles, TRUE
// means vehicles will block" and applied that way at cpathmgr.cpp:1072, so
//   `_free`  = bNoOcc FALSE = the search may route THROUGH occupied hexes
//   `_aware` = bNoOcc TRUE  = an occupied hex is no-entry
// Old logs and old analyses read `_a` as aware and `_f` as free: they have the two
// columns swapped, and any conclusion drawn from that pairing has to be re-read.
//   path_e_*   EMPTY - the "we're stuck" exit: no path at all, or a path whose first
//              and last hex are the same. Both of that block's returns count here
//   path_rp_*  REPEATED PARTIAL - the "can't reach dest, same path as 2 ago" exit: the
//              search stopped short of the destination, at the same hex as last time,
//              so the path is thrown away. Both of that block's exits count here
//   path_d_*   DEGENERATE but ACCEPTED - m_iPathLen == 1, a one-hex "path" (source and
//              destination coincide after the building exit-hex adjustment)
//   path_ap_*  ACCEPTED PARTIAL - the path was kept although it ends somewhere other
//              than the hex asked for, and not at the same short end as last time.
//              This is GetPath's own truncation test, not an added computation
//   path_f_*   FULL - accepted, length > 1, ending on the requested hex
// A high path_e_aware with a low path_e_free is the vehicle-aware search failing on
// occupancy alone; the two must never be added together, they are different questions.
//
// traffic.log also carries the 015 R11 per-vehicle path JOIN, which is what the census
// cannot give: the census says how many searches failed, not whether the SAME order
// failed twice, and only the second question separates a bounded-search miss from a
// target that is really unreachable (015-winastra-traffic-brief-review.md (e)).
//   [PATHRES] veh N plyr N ai 0|1 seq N t N aware 0|1 req X,Y from X,Y ret X,Y
//             len N class <empty|repeated_partial|degenerate|accepted_partial|full>
//             reason <nopath_cantdeploy|nopath_blocked|samepath_cantdeploy|
//                     samepath_blocked|accepted> retries N bc N
//     One line per CVehicle::GetPath exit, local owners only. `seq` counts GetPath
//     CALLS on that vehicle; `aware` is bNoOcc (1 = vehicles block, the `_aware`
//     column above); `req`/`from`/`ret` are HEXES - the hex asked for, the hex started
//     from, and the last hex of the path that actually came back (-1,-1 = nothing came
//     back). `len` is m_iPathLen AT THE EXIT, so the repeated-partial exits show len 0
//     beside a real `ret` because that branch frees the path on its way out.
//     `retries`/`bc` are read at GetPath ENTRY (the rung the search was issued from),
//     NOT after the blocked exits force them to MAX_NUM_RETRIES.
//     RATE BOUND: every non-full result is printed; a `full` result is printed only
//     when the previous result was not full. A gap in `seq` is a run of suppressed
//     successes, not a lost line.
//   [REISSUE] veh N plyr N seq N newdest X,Y olddest X,Y mode N
//     Written at CVehicle::SetDestAndMode when the vehicle is still carrying an
//     unjoined NON-FULL [PATHRES]; `seq` names that failed search and the marker is
//     cleared, so one failure yields at most ONE line. Coordinates are SUB-hexes
//     (hex = sub / 2), the same space as [GIVEUP] dest, NOT the hex space of
//     [PATHRES]. Because the marker-clear is the only throttle, a line here proves a
//     destination change followed a failed search - it does NOT by itself prove a
//     player or router order, since the engine's own route advance and
//     SetRouteMode(stop) also reach SetDestAndMode. Read newdest/olddest/mode.
//   [GIVEUP] ... now also ends with `pathseq N pathcls <class|none>`, naming the last
//     [PATHRES] that vehicle wrote. `none` = no record yet (probe off for its whole
//     life, or it only ever produced suppressed full paths). pathcls is the class of
//     THAT RECORD, not of the give-up.
//
// 015 R12 INSTRUMENT-ONLY records. Same EN_TRAFFIC_LOG gate, same EnTrafficLog sink,
// and none of them changes a value the game reads.
//   [NONOTIFY] veh N vtype N plyr N event N mode N at X,Y dest X,Y hexdest X,Y
//              retries N bc N
//              why <giveup|endpath_fig|endpath_retry|enterdest|arriveddest|?>
//     CVehicle::PostArrivedOrBlocked (vehmove.cpp) has TWO arms on its blocked branch
//     (m_ptDest != m_ptHead): an AI owner gets CMsgVehGoto, a transport gets the HP
//     router. There is no third arm, so a HUMAN NON-TRANSPORT vehicle is told by
//     NOBODY that it stopped - one line here IS that event. UNTHROTTLED on purpose:
//     the open question is how often it happens at all, so the count is the datum.
//     `at`/`dest` are SUB-hexes printed RAW (hex = sub / 2, the [GIVEUP] dest space);
//     `hexdest` is m_hexDest, already a HEX - the two are deliberately in different
//     spaces, do not compare them without dividing. `event` is m_iEvent, `mode` the
//     m_cMode the caller left behind (stop at every tagged caller).
//     `why` is the CALLER, set around the call by a vehmove.cpp file-static:
//       giveup        HandleBlocked retries>15 && bc>5. That caller ALSO fires
//                     theGame.Event(EVENT_GOTO_CANT, ...) - but only for GetOwner()
//                     ->IsMe(). So why=giveup is silent ONLY for a human who is not
//                     me (an ally/remote human in MP); the local player did get told.
//                     Every other tag is silent for the local player too.
//       endpath_fig   at_end_of_path and within one sub-hex of m_hexNext
//       endpath_retry m_hexNext impassable after both fig_step tries were spent
//       enterdest     EnterBuilding reaching the destination. AI-gated at the call, so
//                     it CANNOT reach this arm; tagged so a "?" is never this caller
//       arriveddest   CVehicle::ArrivedDest (only reachable here on m_iEvent == load)
//       stopbackup    vehicle.cpp stop-mode backup (told_ai_stop not yet set)   \
//       cantdeploy30s vehicle.cpp cant_deploy, blocked > 30 s                    > R13
//       deployed      vehicle.cpp deploy_it, m_hexDest.SameHex(m_ptHead)        /
//                     R12 shipped these three untagged, so a "?" in an R12 log is one
//                     of them; from R13 they name themselves and "?" means no tag at
//                     all (a caller neither file tags, i.e. a NEW one).
//       ?             no tag set around the call
//   [NONOTIFY_CHG] veh N plyr N from N to N
//     The SAME vehicle at its next real mode change (CVehicle::_SetRouteMode). A
//     [NONOTIFY] with no [NONOTIFY_CHG] after it is a vehicle that NEVER left the mode
//     the silent fall-through left it in - that is the whole point of the pair. At
//     most one line per [NONOTIFY] (the flag is cleared when it prints).
//     CAVEAT: _SetRouteMode is not the only writer of m_cMode - new_unit.cpp:5140/5875
//     (construct/reset), new_unit.cpp:6481 (deserialize) and vehmove.cpp:3266 (the
//     remote-mode net message) assign it directly, and a change through one of those
//     is invisible here. A missing [NONOTIFY_CHG] therefore means "no mode change via
//     _SetRouteMode", which is weaker than "never moved again".
// 015 R13 INSTRUMENT-ONLY record, RE-SITED in R16 by the BUGS #100 fix. Same
// EN_TRAFFIC_LOG gate, same EnTrafficLog sink, same `why` tag set as [NONOTIFY]
// above, and it changes nothing the game reads.
//   [ARRIVEMISS] veh N plyr N ai 0|1 transport 0|1 vtype N event N mode N
//                at X,Y dest X,Y hexdest X,Y why <tag> fixed 1
//     R13 PLACEMENT (runs 13 and earlier): the top of
//     CVehicle::PostArrivedOrBlocked's BLOCKED branch, fired when
//     m_hexDest.SameHex(m_ptHead) was TRUE there - i.e. the vehicle WAS in its
//     destination hex and the function was about to report it blocked anyway. That
//     is a units disagreement, not a race: the callers decide "we are there" with a
//     HEX test (vehicle.cpp deploy_it, ~890: m_hexDest.SameHex(m_ptHead)) and the
//     function decided with a SUB test (m_ptDest == m_ptHead), so a vehicle parked
//     on the OTHER sub-hex of the right hex satisfied the caller and failed the
//     callee. What each owner kind was then told is what the record was for:
//       ai 1              CMsgVehGoto::ToErr - the AI is told its goto FAILED
//       ai 0 transport 1  m_pHpRtr->MsgErrGoto - the router is told the same
//       ai 0 transport 0  NOTHING (this is the [NONOTIFY] arm)
//     [NONOTIFY] can only ever see the third of those, because it is the only arm
//     that logs; [ARRIVEMISS] sees all three, which is why the AI/transport counts
//     were new information and not a re-count of R12's 32 records.
//     R16 PLACEMENT (run 14 onwards): BUGS #100 widened the success condition to
//     ((m_ptDest == m_ptHead) || m_hexDest.SameHex(m_ptHead)), so exactly those
//     cases now take the SUCCESS arm. The probe moved with them: it now sits at the
//     top of that arm, gated on EnTrafficLogOn() && !(m_ptDest == m_ptHead), and
//     carries the SAME fields plus a trailing `fixed 1`. So a run-14 line means "the
//     #100 fix redirected this one" and the count of [ARRIVEMISS] lines is exactly
//     the number of cases the fix changed; an exact-sub arrival (the always-worked
//     case) never logs. There is NO [ARRIVEMISS] line in the blocked branch any
//     more - after the fix that branch cannot be same-hex - so a missing line there
//     is the fix working, not the probe failing.
//     `at`/`dest` are SUB-hexes printed RAW and `hexdest` is already a HEX (hex =
//     sub / 2), the same deliberate mismatch as [NONOTIFY] - do not compare them
//     without dividing. UNTHROTTLED: the count is the datum.
//   [PWRNEG] site <cheat|rocket|plant_nofuel|plant> plyr N pre N add N post N
//            src <cheat|frameprod=G|power=N fpower=G>
//     Every CPlayer::AddPwrHave call site in the game (all four are in mainloop.cpp:
//     the _CHEAT max-power key, the rocket free-power add in CBuilding::Operate, and
//     the two adds in CPowerBuilding::BuildPower). pre/post bracket that ONE call, so
//     add != post - pre means something else wrote m_iPwrHave in between. Printed when
//     the add is negative, or the stock crosses from >= 0 to < 0. A CROSSING always
//     prints; other negative-add lines are bounded to one per site per second.
//   [PWRLOOP] plyr N have N need N
//     CPlayer::StartLoop, read immediately BEFORE the per-loop reset (which is
//     untouched) and ONLY when m_iPwrHave < 0: a negative power stock that survived a
//     whole accumulation cycle, which no sequence of non-negative adds can produce.
//   [PPLADD] site <bldg_dtor|bldg_remove|veh_dtor|veh_remove> plyr N
//            field <pplbldg|pplveh> pre N add N post N
//            src <people=N pplmult=G|people=N>
//     Every CPlayer::AddPplBldg / AddPplVeh call site (all four are in new_unit.cpp,
//     all on destroy paths: the two unit destructors and the two RemoveUnit). Printed
//     when the add is outside +/-10000, or the stock crosses 0 downward, or crosses
//     1e9 upward - the #82 signature (m_iPplBldg found at ~2,137,000,000 on live
//     players). A crossing always prints; other qualifying lines are bounded to one
//     per site per second. pre/post bracket the one call, as with [PWRNEG].
//   [PPLLOOP] plyr N pplbldg N pplneed N pplveh N
//     CPlayer::StartLoop, the same point as [PWRLOOP], only when m_iPplBldg < 0 or
//     > 1e9, bounded to one line per player per 10 s. m_iPplBldg is deliberately NOT
//     reset by StartLoop - it is a running stock, unlike m_iPwrHave - which is exactly
//     why a bad value persists across loops, and this line is how a persisting one
//     becomes visible without a [PPLADD] having been captured.
// 015 R15 INSTRUMENT-ONLY record. Same EN_TRAFFIC_LOG gate, same EnTrafficLog sink,
// and neither line writes anything the game reads - no clamping, no repair.
//   [HISTLOAD] plyr N ver N fine head N count N
//              hr head N,N,N count N,N,N tick N,N,N bad 0|1
//     CPlayer::Serialize LOAD path (player.cpp), the first statement after the whole
//     colony-stat history block is deserialized: the fine ring head/count (save
//     release >= 4) and the three coarse rings' head/count/tick (release >= 6). It is
//     placed BEFORE SeedHRFromHist and long before any SampleHistory, so the indices
//     printed are the ones the SAVE produced, not ones the load path repaired.
//     One line per player per load. `ver` is theGame.m_dwVer, the loaded save's
//     release - on a save < 6 the hr fields are ctor defaults, NOT saved data (the
//     load gate did not read them and SeedHRFromHist rebuilds them right after).
//     `bad` is 1 when any head is outside [0, HIST_LEN) or any count outside
//     [0, HIST_LEN] - HIST_LEN is 120 (player.h) - and 0 otherwise. Those indices are
//     used UNCHECKED as array subscripts by SampleHistory, so bad 1 is a save that
//     will corrupt memory on the next sample tick, visible here before it happens.
//   [HISTBAD] plyr N fine head N
//   [HISTBAD] plyr N ring N head N
//     CPlayer::SampleHistory (player.cpp), immediately before the writes each head
//     indexes: the `fine` form guards m_iHistHead (the m_aHist* arrays), the `ring`
//     form guards m_iHRHead[r] (m_aHR[r][s][h]) inside the per-ring loop. Printed
//     only when that head is outside [0, HIST_LEN). The out-of-range write then goes
//     ahead exactly as before - these lines LOG, they do not fix - so the pairing to
//     look for is a [HISTLOAD] ... bad 1 followed by [HISTBAD] on the same plyr.
//     UNTHROTTLED: SampleHistory runs about once per game-minute per player.
// Census line buffer: 1536 bytes (raised from 1024 with the R10 block). Worst case
// with the T2b block and the R11 suffix rename is 1226 bytes including the NUL - 621
// literal chars, 24 %d at 11 (INT_MIN) and 34 %lu at 10 (ULONG_MAX on this
// 32-bit-long target) - so 310 bytes of headroom remain and the line still cannot be
// truncated. The R11 lines above are written by EnTrafficLog, not into this buffer.
// Backs the `traffic` cmd. Render/game thread only. Implemented in vehicle.cpp.
void HarnessDumpTraffic(std::string& out);

#endif // EN_HARNESS_H
