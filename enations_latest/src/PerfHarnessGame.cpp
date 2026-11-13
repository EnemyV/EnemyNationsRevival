#include "stdafx.h"
#include "PerfHarness.h"
#include "PerfHarnessZoom.h"
#include "SDL2Dialogs.h"
#include "SDL2Panel.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "player.h"
#include "building.h"
#include "vehicle.h"
#include "area.h"
#include "terrain.h"
#include "en_assert.h"
#include <chrono>
#include <cstdio>
#include <sstream>
#include <string>

static std::string pendingPath;
static bool measurementActive = false;
static bool measurementDrift = false;
static int measurementCameraX = 0, measurementCameraY = 0, measurementSpeed = 0;
static std::string zoomTrial;
static bool zoomSettledLogged = false;
static bool zoomTrialFailed = false;
static float zoomTrialTarget = 0.0f;
static CWndArea* zoomTrialArea = nullptr;

static void RecordZoomResult(CWndArea* area, long long ns)
{
    if (zoomTrial.empty() || zoomSettledLogged || (area == zoomTrialArea && area && area->IsZoomAnimating())) return;
    const float actual = area ? area->GetZoomVisualLevel() : -1.0f;
    const bool reached = PerfHarnessZoomReached(area && area == zoomTrialArea, actual, zoomTrialTarget);
    zoomTrialFailed = zoomTrialFailed || !reached;
    std::fprintf(stderr, "[P1-HARNESS] zoom %s id=%s monotonic_ns=%lld level=%.6f target=%.6f\n",
                 reached ? "settled" : "interrupted", zoomTrial.c_str(), ns, actual, zoomTrialTarget);
    zoomSettledLogged = true;
}

static bool MeasurementSettingsChanged()
{
    CWndArea* area = theApp.AmInGame() ? theAreaList.GetTop() : nullptr;
    if (!area) return true;
    CMapLoc center = area->GetAA().GetCenter();
    return center.x != measurementCameraX || center.y != measurementCameraY ||
           theGame.GetGameMul() != measurementSpeed;
}

const char* PerfHarnessPendingLoadPath()
{
    return pendingPath.empty() ? nullptr : pendingPath.c_str();
}

bool PerfHarnessLoadFixture(const char* path)
{
    if (!path || !*path || !theApp.m_gameWindow || theApp.m_pCreateGame || theApp.AmInGame())
        return false;
    FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    std::fclose(file);
    pendingPath = path;
    bool ok = false;
    try { ok = SDL2_RunLoadSinglePlayerFlow(theApp.m_gameWindow.get()); }
    catch (...) { ok = false; }
    pendingPath.clear();
    std::fprintf(stderr, "[P1-HARNESS] load %s\n", ok ? "OK" : "FAILED");
    return ok;
}

bool PerfHarnessReturnToMenu()
{
    if (!theApp.AmInGame()) return false;
    measurementActive = false;
    theApp.CloseWorld();
    return !theApp.AmInGame();
}

std::string PerfHarnessControl(const std::string& command)
{
    if (!theApp.AmInGame()) return "ERR not_in_game\n";
    std::istringstream input(command);
    std::string op, extra;
    input >> op;
    char response[256];
    if (op == "CAMERA") {
        if (measurementActive) return "ERR measurement_active\n";
        int x, y;
        if (!(input >> x >> y) || (input >> extra)) return "ERR invalid_camera\n";
        CWndArea* area = theAreaList.GetTop();
        // GetSideSize is one player block, not the complete world.
        const int boundX = theMap.Get_eX() * MAX_HEX_HT;
        const int boundY = theMap.Get_eY() * MAX_HEX_HT;
        if (!area || x < 0 || y < 0 || x >= boundX || y >= boundY) return "ERR invalid_camera\n";
        area->Center(CMapLoc(x, y));
        CMapLoc actual = area->GetAA().GetCenter();
        std::snprintf(response, sizeof(response), "OK camera_x=%d camera_y=%d\n", actual.x, actual.y);
        return response;
    }
    if (op == "SPEED") {
        if (measurementActive) return "ERR measurement_active\n";
        int speed;
        if (!(input >> speed) || (input >> extra) || speed < 0 || speed >= NUM_SPEEDS)
            return "ERR invalid_speed\n";
        if (theGame.IsNetGame()) return "ERR network_speed_unsupported\n";
        theGame.SetGameMul(speed);
        std::snprintf(response, sizeof(response), "OK speed=%d\n", theGame.GetGameMul());
        return response;
    }
    if (op == "ZOOM") {
        if (!measurementActive) return "ERR measurement_inactive\n";
        CWndArea* area = theAreaList.GetTop();
        if (!area) return "ERR camera_unavailable\n";
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        if (command == "ZOOM STATUS") {
            RecordZoomResult(area, (long long)ns);
            if (zoomTrialFailed) return "ERR zoom_interrupted\n";
            bool active = area->IsZoomAnimating();
            std::snprintf(response, sizeof(response), "OK zoom_id=%s active=%d monotonic_ns=%lld level=%.6f\n",
                          zoomTrial.empty() ? "none" : zoomTrial.c_str(), active ? 1 : 0,
                          (long long)ns, area->GetZoomVisualLevel());
            return response;
        }
        std::string id;
        float delta;
        if (!PerfHarnessParseZoom(command, id, delta)) return "ERR invalid_zoom\n";
        RecordZoomResult(area, (long long)ns);
        if (zoomTrialFailed) return "ERR zoom_interrupted\n";
        if (area->IsZoomAnimating()) return "ERR zoom_active\n";
        float before = area->GetZoomVisualLevel();
        float target = before + delta;
        if (target < theApp.GetZoomData()->GetFirstZoom() || target > NUM_ZOOM_LEVELS - 1)
            return "ERR zoom_out_of_range\n";
        if (!area->GetAA().m_sdlPanel || !area->GetAA().m_sdlPanel->CanSmoothZoom())
            return "ERR zoom_unavailable\n";
        area->RequestSmoothZoom(delta);
        if (!area->IsZoomAnimating()) return "ERR zoom_unavailable\n";
        zoomTrial = id; zoomSettledLogged = false; zoomTrialTarget = target; zoomTrialArea = area;
        std::fprintf(stderr, "[P1-HARNESS] zoom begin id=%s monotonic_ns=%lld level=%.6f delta=%.6f\n",
                     id.c_str(), (long long)ns, before, delta);
        std::snprintf(response, sizeof(response), "OK zoom_id=%s monotonic_ns=%lld level=%.6f delta=%.6f\n",
                      id.c_str(), (long long)ns, before, delta);
        return response;
    }
    if (command == "MARK BEGIN" || command == "MARK END") {
        const bool begin = command == "MARK BEGIN";
        if (begin == measurementActive) return "ERR invalid_mark_state\n";
        if (begin) {
            CWndArea* area = theAreaList.GetTop();
            if (!area) return "ERR camera_unavailable\n";
            CMapLoc center = area->GetAA().GetCenter();
            measurementCameraX = center.x;
            measurementCameraY = center.y;
            measurementSpeed = theGame.GetGameMul();
            measurementDrift = false;
            zoomTrial.clear(); zoomSettledLogged = false; zoomTrialFailed = false; zoomTrialArea = nullptr;
            measurementActive = true;
        } else {
            CWndArea* area = theAreaList.GetTop();
            if (area && area->IsZoomAnimating()) return "ERR zoom_active\n";
            const auto resultNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            RecordZoomResult(area, (long long)resultNs);
            if (!zoomTrial.empty() && !PerfHarnessZoomReached(area && area == zoomTrialArea,
                    area ? area->GetZoomVisualLevel() : -1.0f, zoomTrialTarget))
                zoomTrialFailed = true;
            const bool drift = measurementDrift || MeasurementSettingsChanged();
            if (!zoomTrial.empty()) {
                const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                std::fprintf(stderr, "[P1-HARNESS] zoom end id=%s monotonic_ns=%lld level=%.6f\n",
                             zoomTrial.c_str(), (long long)ns, area ? area->GetZoomVisualLevel() : -1.0f);
            }
            measurementActive = false;
            if (zoomTrialFailed) return "ERR zoom_interrupted\n";
            if (drift) return "ERR measurement_settings_changed\n";
        }
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::snprintf(response, sizeof(response), "OK mark=%s monotonic_ns=%lld game_frame=%lu game_seconds=%lu speed=%d\n",
                      begin ? "begin" : "end", (long long)ns, (unsigned long)theGame.GetFrame(),
                      (unsigned long)theGame.GetElapsedSeconds(), theGame.GetGameMul());
        return response;
    }
    return "ERR unknown_command\n";
}

void PerfHarnessSnapshot(char* out, size_t size, unsigned long long frames)
{
    CWndArea* area = theApp.AmInGame() ? theAreaList.GetTop() : nullptr;
    if (measurementActive && !zoomTrial.empty() && !zoomSettledLogged) {
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        RecordZoomResult(area, (long long)ns);
    }
    CMapLoc center = area ? area->GetAA().GetCenter() : CMapLoc(0, 0);
    if (measurementActive && MeasurementSettingsChanged()) measurementDrift = true;
    std::snprintf(out, size, "OK loop_iterations=%llu ingame=%d players=%d buildings=%d vehicles=%d game_frame=%lu game_seconds=%lu camera_available=%d camera_x=%d camera_y=%d speed=%d assert_fires=%ld\n", frames,
                  theApp.AmInGame() ? 1 : 0,
                  theApp.AmInGame() ? (int)theGame.GetAll().GetCount() : 0,
                  theApp.AmInGame() ? (int)theBuildingMap.GetCount() : 0,
                  theApp.AmInGame() ? (int)theVehicleMap.GetCount() : 0,
                  theApp.AmInGame() ? (unsigned long)theGame.GetFrame() : 0UL,
                  theApp.AmInGame() ? (unsigned long)theGame.GetElapsedSeconds() : 0UL,
                  area ? 1 : 0, center.x, center.y, theApp.AmInGame() ? theGame.GetGameMul() : -1,
                  (long)InterlockedCompareExchange(&g_enInvariantFires, 0L, 0L));
}

void PerfHarnessQuit()
{
    if (theApp.m_gameWindow) theApp.m_gameWindow->RequestQuit();
}
