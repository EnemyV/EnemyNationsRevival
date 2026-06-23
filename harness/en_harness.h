//---------------------------------------------------------------------------
// en_harness.h — in-process LLM-driving harness (Linux/Debug only).
//
// A small TCP control server compiled into the game. It lets an external client
// (thin shell/python scripts) screenshot the running game and inject mouse/key
// events — the Wayland-safe, focus-free replacement for the Windows PostMessage/
// PrintWindow harness. Input is injected via SDL_PushEvent (thread-safe);
// screenshots are serviced on the main/render thread via EnHarness_Service().
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

// Center the focused area view on the unit with this id (vehicle or building),
// so a headless driver can then click view-center to select it — sidesteps the
// view-relative/wrapped screen coords from HarnessDumpUnits. Returns false if no
// area window or no unit with that id. Call on the game/render thread (mutates
// the view). Backs the `center <id>` control_socket command + (future) a Windows
// trigger. Pairs with `units`:  units -> pick a crane id -> center <id> ->
// clickid <area> <center> -> keyid <area> 98 (Build).
bool HarnessCenterUnit(unsigned long id);

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

#endif // EN_HARNESS_H
