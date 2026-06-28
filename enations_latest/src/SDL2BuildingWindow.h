#pragma once

#include "SDL2UI.h"

class CBuilding;
struct SDL_Surface;

// ============================================================================
// SDL2BuildingWindow — the generic, read-only "what does this building do" window.
//
// Opened by double-clicking a warehouse / rocket, power plant, office, apartment,
// or a resource producer (mine / farm / smelter / refinery / oil well). It composes
// only the sections a building actually provides; the rocket shows all of them.
//
// Each section sits in an outlined box with a bold blue header. Storage shows the
// six stored materials as icon stacks; power / housing show this-building + colony
// numbers beside a history graph (CPlayer::GetHist*); a producer shows its live
// production status; the rocket also shows its turret. All numbers refresh per frame.
// ============================================================================

class SDL2BuildingWindow : public SDL2Dialog {
public:
    SDL2BuildingWindow(GameWindow* gw, CBuilding* pBldg, bool bOnTop = false);
    ~SDL2BuildingWindow();

    // The six stored, transportable materials (food / gas / power / people are
    // COLONY-wide, not per-building, so they're intentionally excluded here).
    static const int kNumStoreMats = 6;

    // Repair queue: the unit being serviced plus this many waiting slots shown.
    static const int kRepairRows = 7;

    // Most an input list shows (refinery=1 oil, smelter=2, etc.).
    static const int kMaxInputs = 8;

protected:
    void OnInit() override;
    void OnFrame() override;   // refresh live numbers + redraw graphs/icons

private:
    CBuilding* m_pBldg;
    DWORD      m_bldgID = 0;       // for clearing the range overlay on close
    bool       m_bShowRange = false;
    // [bw-throttle] OnFrame() ran the full Refresh() (label SetText churn, material-icon
    // texture redraws, a whole theVehicleMap scan) EVERY frame → fps cratered with any
    // building window open (build-veh worst). Live numbers don't need 60Hz; gate Refresh
    // to this interval. Per-instance so each window throttles independently.
    unsigned long long m_nextRefreshMs = 0;   // SDL_GetTicks64() ms of next allowed Refresh

    bool m_bStorage    = false;
    bool m_bPower      = false;
    bool m_bOffice     = false;
    bool m_bApt        = false;
    bool m_bTurret     = false;
    bool m_bProduction = false;
    bool m_bMilitary   = false;   // command center: military strength + unit counts
    bool m_bRepair     = false;   // repair building: live repair queue
    bool m_bInputs     = false;   // production building: stocks of its input materials
    bool m_bOutputs    = false;   // production building: stocks of what it produces
    bool m_bFertility  = false;   // farm: soil fertility (green ICON_DENSITY "X"s)
    bool m_bUnits      = false;   // seaport: vehicles currently docked inside
    bool m_bBuilding   = false;   // vehicle plant / shipyard: unit under construction

    // chrome helpers
    void LoadIcons();
    void AddOutline(int x, int y, int w, int h);
    // Section header: bold accent-coloured text, with a category glyph to its left
    // (iconIdx is an ICON_* index, -1 for none; iconFrame picks a frame within a
    // multi-frame strip, e.g. a specific material). Returns the y below.
    int  Header(int x, int y, int w, const char* text, SDL_Color color,
                int iconIdx = -1, int iconFrame = 0, SDL2Label** ppLabelOut = nullptr,
                SDL2Image** ppIconOut = nullptr);
    // #51 C1: (re)build a header icon's glyph surface in place, so the coal-liq Power<->Oil header
    // icon can swap LIVE in Refresh (not just at window-build). Safe on a null img.
    void SetHdrIcon(SDL2Image* img, int iconIdx, int iconFrame);
    // Build the portrait + name + flavor text + status-line band under the title bar.
    int  BuildHeaderBand(int x, int y, int w);
    int  BuildSection(int id, int x, int y, int w);   // dispatch one section by id (2-column layout)
    // Lazily load + cache a status-bar icon (theIcons) as a surface for header glyphs.
    SDL_Surface* HdrIcon(int idx);

    // section builders (each adds its widgets and returns the y below itself)
    int BuildStorage   (int x, int y, int w);
    int BuildPower     (int x, int y, int w);
    int BuildOffice    (int x, int y, int w);
    int BuildApt       (int x, int y, int w);
    int BuildTurret    (int x, int y, int w);
    int BuildEdicts    (int x, int y, int w);   // Edicts v1: civ-wide policy toggles (host buildings)
    int BuildAltOutput (int x, int y, int w);   // #40: building-scoped AltOutput toggle (checkbox + scope)
    int BuildProduction(int x, int y, int w);
    int BuildMilitary  (int x, int y, int w);
    int BuildRepair    (int x, int y, int w);
    int BuildInputs    (int x, int y, int w);
    int BuildOutputs   (int x, int y, int w);
    int BuildFertility (int x, int y, int w);
    int BuildUnits     (int x, int y, int w);
    int BuildBuilding  (int x, int y, int w);
    // Render the construction-progress bar (ICON_BUILD_VEH art on a black/gold box).
    void DrawBuildBar(SDL2Image* img, int per);

    void Refresh();

    // Recount my combat units (one vehicle-map pass) only ~once a second, so the
    // command-center window never costs the running game noticeable time.
    void ComputeMilitary();

    // this building's own current contribution (computed live; never stored)
    int  PerBldgPower () const;
    int  PerBldgAptCap() const;
    int  PerBldgOfcCap() const;

    enum HistSeries { kNone, kPwrHave, kPwrNeed, kPplTotal, kPplBldg, kAptCap, kOfcCap };
    void DrawGraph(SDL2Image* img, HistSeries a, HistSeries b);
    // Draw stacked material icons (~250/icon per row) into img for the given
    // material-type list. Shared by the storage and input widgets.
    void DrawMatIcons(SDL2Image* img, const int* mats, int n);
    // Tile the green ICON_DENSITY "X" art across pct% of img (farm fertility).
    void DrawDensityIcons(SDL2Image* img, int pct);
    // Draw the icons of the vehicles parked inside this building (seaport).
    void DrawContainedUnits(SDL2Image* img);

    // ICON_MATERIALS sprite strip (one icon per material type), for the storage stacks.
    SDL_Surface* m_matIcons = nullptr;
    int m_matIconW = 0;
    int m_matIconH = 0;

    // ICON_DENSITY art (the green "X"), tiled to show farm fertility.
    SDL_Surface* m_densIcon = nullptr;
    int m_densIconW = 0;
    int m_densIconH = 0;
    SDL2Image* m_imgFertility = nullptr;
    SDL2Label* m_lblFertility = nullptr;

    // live widgets refreshed each frame
    SDL2Image* m_imgStorage    = nullptr;
    SDL2Label* m_lblStoreName[kNumStoreMats] = {};
    SDL2Label* m_lblStoreCount[kNumStoreMats] = {};

    SDL2Label* m_lblPowerHdr    = nullptr;   // #6: section header, swapped Oil<->Power live in Refresh
    SDL2Image* m_imgPowerHdrIcon = nullptr;  // #51 C1: the Power/Oil header glyph, swapped live in Refresh
    SDL2Label* m_lblPowerBldg   = nullptr;
    SDL2Label* m_lblPowerColony = nullptr;
    SDL2Image* m_imgPowerGraph  = nullptr;
    // #43: Coal Liquefaction mode-awareness. When a coal power plant has its Coal-Liq
    // toggle ON it STOPS generating power and converts coal->oil, so the Power section
    // hides the power readout/graph above and shows these oil rows instead.
    SDL2Label* m_lblPowerOilHdr = nullptr;   // "Converting coal to oil" status row
    SDL2Label* m_lblPowerOil    = nullptr;   // this building's oil produced (store)
    SDL2Label* m_lblPowerOilCol = nullptr;   // colony oil (have / made)

    SDL2Label* m_lblOfcBldg     = nullptr;
    SDL2Label* m_lblOfcColony   = nullptr;
    SDL2Label* m_lblOfcNeed     = nullptr;
    SDL2Label* m_lblOfcEnergy   = nullptr;   // colony energy NEED (#37/#39: surfaces Mining Subsidy's +20% energy)
    SDL2Image* m_imgOfcGraph    = nullptr;

    SDL2Label* m_lblAptBldg     = nullptr;
    SDL2Label* m_lblAptColony   = nullptr;
    SDL2Label* m_lblAptNeed     = nullptr;   // colony food NEED (#37/#39: surfaces Nutrition's +20% food)
    SDL2Image* m_imgAptGraph    = nullptr;

    SDL2Label*  m_lblTurretRange  = nullptr;   // 2x2 grid: range / damage / reload / dps
    SDL2Label*  m_lblTurretDmg    = nullptr;
    SDL2Label*  m_lblTurretReload = nullptr;
    SDL2Label*  m_lblTurretDps    = nullptr;
    SDL2Button* m_btnShowRange     = nullptr;
    // Generic AltOutput (alt-output toggle) -- shown on any building that has an available
    // AltOutput def (BioFuel farm, Coal-Liquefaction coal plant, Charcoal lumber mill,
    // Fracking exhausted oil well). Rendered as a CHECKBOX (bug #40 — was a button that
    // overlapped Close + didn't toggle) in its own "Production Mode" section, mirroring the
    // civ-wide Edicts checkbox pattern. Flips the building's runtime-only alt_oil flag (which
    // the shared AltOutput::Convert production hook reads); label comes from the def. Scope
    // ("This building only") is shown via an adjacent SDL2InfoIcon (#36).
    SDL2Checkbox* m_chkAltOut      = nullptr;
    // #43-audit item 2: a mode-aware OUTPUT readout in the "Production Mode" section, mirroring
    // the coal-liq Power-section swap for the BioFuel (oil) / Charcoal (coal) / Fracking (oil)
    // hosts — they have no Power section, so without this enabling the toggle showed no output.
    // Created always; shown only when the toggle is ON and the def is NOT the coal-liq one (which
    // already shows its readout in the Power section). Reflects the def's real output material.

    SDL2Label*       m_lblProduction  = nullptr;
    SDL2ProgressBar* m_progProduction = nullptr;

    // Command-center military summary (counts refreshed by ComputeMilitary).
    SDL2Label* m_lblMilStrength = nullptr;
    SDL2Label* m_lblInfantry    = nullptr;
    SDL2Label* m_lblVehicles    = nullptr;
    SDL2Label* m_lblMilEnergy   = nullptr;   // colony energy NEED (#39: surfaces +energy-upkeep edicts)
    int m_iMilTick     = 0;     // frame counter for throttling the recount
    int m_iMilStrength = 0;
    int m_iInfantry    = 0;
    int m_iVehicles    = 0;

    // Repair-building queue (row 0 = currently servicing, rest = waiting).
    SDL2Label*       m_lblRepair[kRepairRows] = {};
    SDL2ProgressBar* m_progRepair = nullptr;

    // Production-building input stocks (e.g. oil for a refinery).
    int        m_inputMats[kMaxInputs] = {};
    int        m_nInputMats = 0;
    SDL2Image* m_imgInputs = nullptr;
    SDL2Label* m_lblInputName[kMaxInputs]  = {};
    SDL2Label* m_lblInputCount[kMaxInputs] = {};

    // Production-building output stocks (steel for a smelter, ore for a mine).
    int        m_outputMats[kMaxInputs] = {};
    int        m_nOutputMats = 0;
    SDL2Image* m_imgOutputs = nullptr;
    SDL2Label* m_lblOutputName[kMaxInputs]  = {};
    SDL2Label* m_lblOutputCount[kMaxInputs] = {};

    // Seaport: vehicles docked inside (ICON_VEHICLES strip, one tile per type).
    SDL_Surface* m_unitIcons = nullptr;
    int m_unitIconW = 0;
    int m_unitIconH = 0;
    SDL2Image* m_imgUnits = nullptr;
    SDL2Label* m_lblUnits  = nullptr;

    // Vehicle plant / shipyard: the unit under construction + its progress bar.
    SDL_Surface* m_buildIcon = nullptr;   // ICON_BUILD_VEH strip
    int m_buildIconW = 0, m_buildIconH = 0, m_buildLeftOff = 0, m_buildRightOff = 0;
    SDL2Image* m_imgBuildBar = nullptr;
    SDL2Label* m_lblBuildName = nullptr;

    // Header band: building portrait, flavor text, live status line, health bar.
    SDL_Surface* m_bldgSheet = nullptr;   // DIB_LIST_UNIT_BUILDINGS (row per bldg type)
    SDL2Label*   m_lblStatus = nullptr;   // colored "Operating / Idle / Needs X"
    SDL2Image*   m_imgHealth = nullptr;   // condition bar under the portrait
    void DrawHealthBar();

    // Cache of status-bar icon surfaces used as header glyphs (indexed by ICON_*).
    SDL_Surface* m_hdrIcon[16] = {};
};
