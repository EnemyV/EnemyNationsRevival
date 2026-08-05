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
// unit OWNED BY the local player to `out`:  "<id> <screenX> <screenY> <kind>\n"
// where screenX/Y are area-window pixels (feed clickid/dblclickid) and kind is
// crane|transport|vehicle|building. Used by the `units` control_socket command
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
// left. Lets a headless driver poll "am I still alive / how hard am I being hit" each
// tick. Render/game thread only. Backs the `gamestate` cmd.
void HarnessDumpGameState(std::string& out);

#endif // EN_HARNESS_H
