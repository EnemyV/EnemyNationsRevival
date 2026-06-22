#include "stdafx.h"
#include "SDL2BuildingWindow.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "player.h"
#include "building.h"
#include "vehicle.h"
#include "base.h"
#include "icons.h"
#include "bitmaps.h"      // theIcons, ICON_MATERIALS
#include "SDL2MainMenu.h"
#include "area.h"         // CWndArea::SetShowRange (weapon-range overlay)

#include "building.inl"
#include "vehicle.inl"
#include "unit.inl"
#include "terrain.inl"

#include <SDL.h>
#include <string>

// windows.h (via stdafx) defines min/max macros that break std::min/std::max.
#undef min
#undef max
#include <algorithm>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

// ----------------------------------------------------------------------------
// Layout constants — kept in one place so computeHeight() and OnInit() stay in
// lock-step.
// ----------------------------------------------------------------------------
static const int WIN_W   = 380;
static const int FIRST_Y = 36;
static const int HDR_H   = 30;   // header row; tall enough for a clear, large category glyph
static const int ROW_H   = 20;   // row height; sized for the 15pt widget font
static const int GRAPH_H = 72;
static const int SEC_PAD = 8;
static const int BOX_PAD = 6;    // inner padding inside a section's outline box
static const int CLOSE_H = 40;
static const int COL_GAP      = 12;    // gap between the two columns when split
static const int TWO_COL_MAX_H = 560;  // stacked body taller than this -> go 2-column (rocket et al.)

static const int STORAGE_H    = BOX_PAD + HDR_H + SDL2BuildingWindow::kNumStoreMats * ROW_H + BOX_PAD;
static const int POWERLIKE_H  = BOX_PAD + HDR_H + GRAPH_H + BOX_PAD;   // power / office / apartment
static const int TURRET_H     = BOX_PAD + HDR_H + 2 * ROW_H + 34 + BOX_PAD;   // 2x2 stats + Show-Range
static const int PRODUCTION_H = BOX_PAD + HDR_H + 2 * ROW_H + 6 + 16 + BOX_PAD;   // text + progress bar
static const int MILITARY_H   = BOX_PAD + HDR_H + 3 * ROW_H + BOX_PAD;   // strength + infantry + vehicles
static const int FERTILITY_H  = BOX_PAD + HDR_H + ROW_H + BOX_PAD;       // one row: green-X bar + %
static const int UNITS_ROW_H  = 30;                                      // tall enough for unit icons
static const int UNITS_H      = BOX_PAD + HDR_H + UNITS_ROW_H + BOX_PAD; // seaport: docked-units strip
static const int BUILD_BAR_H  = 26;
static const int BUILDING_H   = BOX_PAD + HDR_H + ROW_H + 4 + BUILD_BAR_H + BOX_PAD;  // name + bar
static const int REPAIR_H     = BOX_PAD + HDR_H + 16 + 6 +
                                SDL2BuildingWindow::kRepairRows * ROW_H + BOX_PAD;   // bar + queue rows

static const int PORTRAIT  = 64;
static const int HEADER_H   = 92;   // portrait + name + 3-line flavor + status line

// Category accent colors for section headers — saturated darks that read on the
// light parchment interior, replacing the one-size-fits-all blue.
static const SDL_Color kHeaderBlue = { 40, 60, 150, 255 };   // power / utility
static const SDL_Color kAccentGold = { 150, 95, 18, 255 };   // resources / production
static const SDL_Color kAccentGrn  = { 28, 104, 48, 255 };   // population / fertility
static const SDL_Color kAccentRed  = { 158, 32, 32, 255 };   // weapon / military

// Status-line colors (mirror CLR_STATUS_TEXT_* from the original status bar).
static const SDL_Color kStatusOk   = { 30, 120, 40, 255 };
static const SDL_Color kStatusWarn = { 170, 120, 0, 255 };
static const SDL_Color kStatusBad  = { 170, 30, 30, 255 };

// Group a number into thousands with commas (1250 -> "1,250").
static std::string FmtNum(int v) {
    bool neg = ( v < 0 );
    std::string d = std::to_string( neg ? -(long long)v : (long long)v );
    std::string out;
    int c = 0;
    for ( int i = (int)d.size() - 1; i >= 0; i-- ) {
        out.push_back( d[i] );
        if ( ++c % 3 == 0 && i > 0 ) out.push_back( ',' );
    }
    if ( neg ) out.push_back( '-' );
    std::reverse( out.begin(), out.end() );
    return out;
}

// The six stored materials (food / gas are colony-wide, so excluded), in the same
// order/labels the Load Truck dialog uses.
static const int  kStoreMats[SDL2BuildingWindow::kNumStoreMats] = {
    CMaterialTypes::lumber, CMaterialTypes::steel, CMaterialTypes::copper,
    CMaterialTypes::coal,   CMaterialTypes::iron,  CMaterialTypes::oil };
static const char* const kStoreNames[SDL2BuildingWindow::kNumStoreMats] = {
    "Lumber", "Steel", "Xilitium", "Coal", "Iron", "Oil" };

// ----------------------------------------------------------------------------
// Section detection
// ----------------------------------------------------------------------------
static bool secStorage(CBuilding* b) {
    return ( b->GetData()->GetUnionType() == CStructureData::UTwarehouse );   // warehouse + rocket
}
static bool secPower(CBuilding* b) {
    return ( b->GetData()->GetType() == CStructureData::rocket ) ||
           ( b->GetData()->GetUnionType() == CStructureData::UTpower );
}
static bool secApt(CBuilding* b) {
    if ( b->GetData()->GetType() == CStructureData::rocket ) return true;
    return ( b->GetData()->GetUnionType() == CStructureData::UThousing ) &&
           ( b->GetData()->GetBldgType()  == CStructureData::apartment );
}
static bool secOfc(CBuilding* b) {
    if ( b->GetData()->GetType() == CStructureData::rocket ) return true;
    return ( b->GetData()->GetUnionType() == CStructureData::UThousing ) &&
           ( b->GetData()->GetBldgType()  == CStructureData::office );
}
static bool secTurret(CBuilding* b) {
    // Any armed building (rocket, forts, pillboxes, bunkers, ...) shows the weapon widget.
    return ( b->GetFireRate() > 0 );
}
static bool secProduction(CBuilding* b) {
    int ut = b->GetData()->GetUnionType();
    return ( ut == CStructureData::UTmaterials ) || ( ut == CStructureData::UTmine ) ||
           ( ut == CStructureData::UTfarm );
}
static bool secMilitary(CBuilding* b) {
    return ( b->GetData()->GetUnionType() == CStructureData::UTcommand );
}
static bool secFertility(CBuilding* b) {
    return ( b->GetData()->GetUnionType() == CStructureData::UTfarm );   // farms + lumber mills
}
static bool secRepair(CBuilding* b) {
    return ( b->GetData()->GetUnionType() == CStructureData::UTrepair );
}

// Collect the material types this building consumes (GetInputs > 0), e.g. oil for a
// refinery, iron + coal for a smelter. Returns how many were written to outMats.
static int collectInputMats(CBuilding* b, int* outMats, int maxOut) {
    int vals[CMaterialTypes::num_types] = {};
    b->GetInputs( vals );
    int n = 0;
    for ( int i = 0; ( i < CMaterialTypes::GetNumTypes() ) && ( n < maxOut ); i++ )
        if ( vals[i] > 0 ) outMats[n++] = i;
    return n;
}

// The input-stock widget appears on producers (smelter/refinery) and on the unit
// factories (vehicle plant / shipyard, opened via their (I) button) — anything that
// actually consumes materials. Raw mines/farms and storage buildings have no inputs.
static bool secInputs(CBuilding* b) {
    int ut = b->GetData()->GetUnionType();
    bool eligible = secProduction(b) ||
                    ( ut == CStructureData::UTvehicle ) ||
                    ( ut == CStructureData::UTshipyard );
    if ( !eligible ) return false;
    int tmp[SDL2BuildingWindow::kMaxInputs];
    return ( collectInputMats(b, tmp, SDL2BuildingWindow::kMaxInputs) > 0 );
}
static int inputsHeight(CBuilding* b) {
    int tmp[SDL2BuildingWindow::kMaxInputs];
    int n = collectInputMats(b, tmp, SDL2BuildingWindow::kMaxInputs);
    if ( n <= 0 ) return 0;
    return BOX_PAD + HDR_H + n * ROW_H + BOX_PAD;
}

// Collect the material types this building produces: a smelter/refinery's GetOutput
// list, or a mine's single mined material. Farms are skipped — their output is food,
// which is a colony-wide resource, not a per-building stockpile.
static int collectOutputMats(CBuilding* b, int* outMats, int maxOut) {
    int n = 0;
    int ut = b->GetData()->GetUnionType();
    if ( ut == CStructureData::UTmaterials ) {
        CBuildMaterials* pm = b->GetData()->GetBldMaterials();
        if ( pm )
            for ( int i = 0; ( i < CMaterialTypes::GetNumTypes() ) && ( n < maxOut ); i++ )
                if ( pm->GetOutput(i) > 0 ) outMats[n++] = i;
    } else if ( ut == CStructureData::UTmine ) {
        CBuildMine* pmn = b->GetData()->GetBldMine();
        if ( pmn ) { int m = pmn->GetTypeMines(); if ( ( m >= 0 ) && ( n < maxOut ) ) outMats[n++] = m; }
    }
    return n;
}
static bool secOutputs(CBuilding* b) {
    int tmp[SDL2BuildingWindow::kMaxInputs];
    return ( collectOutputMats(b, tmp, SDL2BuildingWindow::kMaxInputs) > 0 );
}
static int outputsHeight(CBuilding* b) {
    int tmp[SDL2BuildingWindow::kMaxInputs];
    int n = collectOutputMats(b, tmp, SDL2BuildingWindow::kMaxInputs);
    if ( n <= 0 ) return 0;
    return BOX_PAD + HDR_H + n * ROW_H + BOX_PAD;
}

// The seaport docks vehicles; its window lists what's currently inside.
static bool secUnits(CBuilding* b) {
    return ( b->GetData()->GetType() == CStructureData::seaport );
}

// Vehicle plants / shipyards build units; show what's under construction + progress.
static bool secBuilding(CBuilding* b) {
    int ut = b->GetData()->GetUnionType();
    return ( ut == CStructureData::UTvehicle ) || ( ut == CStructureData::UTshipyard );
}

// ----------------------------------------------------------------------------
// Section layout. Multi-role buildings (above all the rocket) enable so many
// sections that a single stacked column runs off the bottom of the screen. When
// the stacked body would exceed TWO_COL_MAX_H we split the sections into two
// side-by-side columns, each exactly WIN_W wide so section internals are
// unchanged. computeWidth/computeHeight/OnInit all derive their geometry from
// computeLayout() so they stay in lock-step.
// ----------------------------------------------------------------------------
enum {
    SEC_STORAGE, SEC_PRODUCTION, SEC_BUILDING, SEC_FERTILITY, SEC_INPUTS,
    SEC_OUTPUTS, SEC_UNITS, SEC_REPAIR, SEC_MILITARY, SEC_POWER, SEC_OFFICE,
    SEC_APT, SEC_TURRET
};

struct SecRec { int id; int h; };

struct BldgLayout {
    SecRec secs[16];
    int    n         = 0;
    int    colOf[16] = {};     // 0 = left column, 1 = right column
    bool   twoCol    = false;
    int    bodyH     = 0;      // height of the tallest column (incl. SEC_PAD)
    int    width     = WIN_W;
    int    height    = 0;
};

static BldgLayout computeLayout(CBuilding* b) {
    BldgLayout L;
    int& n = L.n;
    // Order here is the display order; must match BuildSection's dispatch.
    if ( secStorage(b)    ) L.secs[n++] = { SEC_STORAGE,    STORAGE_H };
    if ( secProduction(b) ) L.secs[n++] = { SEC_PRODUCTION, PRODUCTION_H };
    if ( secBuilding(b)   ) L.secs[n++] = { SEC_BUILDING,   BUILDING_H };
    if ( secFertility(b)  ) L.secs[n++] = { SEC_FERTILITY,  FERTILITY_H };
    if ( secInputs(b)     ) L.secs[n++] = { SEC_INPUTS,     inputsHeight(b) };
    if ( secOutputs(b)    ) L.secs[n++] = { SEC_OUTPUTS,    outputsHeight(b) };
    if ( secUnits(b)      ) L.secs[n++] = { SEC_UNITS,      UNITS_H };
    if ( secRepair(b)     ) L.secs[n++] = { SEC_REPAIR,     REPAIR_H };
    if ( secMilitary(b)   ) L.secs[n++] = { SEC_MILITARY,   MILITARY_H };
    if ( secPower(b)      ) L.secs[n++] = { SEC_POWER,      POWERLIKE_H };
    if ( secOfc(b)        ) L.secs[n++] = { SEC_OFFICE,     POWERLIKE_H };
    if ( secApt(b)        ) L.secs[n++] = { SEC_APT,        POWERLIKE_H };
    if ( secTurret(b)     ) L.secs[n++] = { SEC_TURRET,     TURRET_H };

    int total = 0;
    for ( int i = 0; i < n; i++ ) total += L.secs[i].h + SEC_PAD;

    if ( total <= TWO_COL_MAX_H || n < 4 ) {
        // single column (the common case)
        L.twoCol = false;
        L.bodyH  = total;
        L.width  = WIN_W;
    } else {
        // two columns, preserving display order: col0 = leading sections up to
        // ~half the stack, col1 = the rest.
        int half = total / 2, run = 0, split = n;
        for ( int i = 0; i < n; i++ ) {
            run += L.secs[i].h + SEC_PAD;
            if ( run >= half ) { split = i + 1; break; }
        }
        int h0 = 0, h1 = 0;
        for ( int i = 0; i < n; i++ ) {
            if ( i < split ) { L.colOf[i] = 0; h0 += L.secs[i].h + SEC_PAD; }
            else             { L.colOf[i] = 1; h1 += L.secs[i].h + SEC_PAD; }
        }
        L.twoCol = true;
        L.bodyH  = std::max( h0, h1 );
        L.width  = 2 * WIN_W + COL_GAP;
    }
    L.height = FIRST_Y + HEADER_H + L.bodyH + CLOSE_H;
    return L;
}

static int computeWidth(CBuilding* b)  { return computeLayout(b).width; }
static int computeHeight(CBuilding* b) { return computeLayout(b).height; }

static std::string makeTitle(CBuilding* b) {
    return std::string( b->GetData()->GetDesc().c_str() );
}

static void lineOnSurface(SDL_Surface* s, int x0, int y0, int x1, int y1, Uint32 color) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        if ( x0 >= 0 && x0 < s->w && y0 >= 0 && y0 < s->h ) {
            SDL_Rect r = { x0, y0, 1, 1 };
            SDL_FillRect(s, &r, color);
        }
        if ( x0 == x1 && y0 == y1 ) break;
        int e2 = 2 * err;
        if ( e2 > -dy ) { err -= dy; x0 += sx; }
        if ( e2 <  dx ) { err += dx; y0 += sy; }
    }
}

// ============================================================================
SDL2BuildingWindow::SDL2BuildingWindow(GameWindow* gw, CBuilding* pBldg, bool bOnTop)
    : SDL2Dialog(gw, makeTitle(pBldg), computeWidth(pBldg), computeHeight(pBldg))
    , m_pBldg(pBldg)
{
    // Tuckable behind the map by default (like Relations); but when launched from a
    // build dialog's (I) button it must float on top of that dialog.
    SetKeepOnTop(bOnTop);
    SetWidgetFontSize(15); // slightly larger than the 13pt default for readability

    m_bldgID      = pBldg->GetID();
    m_bStorage    = secStorage(pBldg);
    m_bProduction = secProduction(pBldg);
    m_nInputMats  = collectInputMats(pBldg, m_inputMats, kMaxInputs);
    m_bInputs     = secInputs(pBldg);
    m_nOutputMats = collectOutputMats(pBldg, m_outputMats, kMaxInputs);
    m_bOutputs    = ( m_nOutputMats > 0 );
    m_bFertility  = secFertility(pBldg);
    m_bUnits      = secUnits(pBldg);
    m_bBuilding   = secBuilding(pBldg);
    m_bRepair     = secRepair(pBldg);
    m_bMilitary   = secMilitary(pBldg);
    m_bPower      = secPower(pBldg);
    m_bOffice     = secOfc(pBldg);
    m_bApt        = secApt(pBldg);
    m_bTurret     = secTurret(pBldg);
}

SDL2BuildingWindow::~SDL2BuildingWindow() {
    // Stop visualizing this building's range when the window closes.
    if ( CWndArea::GetShowRange() == m_bldgID )
        CWndArea::SetShowRange( 0 );
    if ( m_matIcons )  SDL_FreeSurface( m_matIcons );
    if ( m_densIcon )  SDL_FreeSurface( m_densIcon );
    if ( m_unitIcons ) SDL_FreeSurface( m_unitIcons );
    if ( m_bldgSheet ) SDL_FreeSurface( m_bldgSheet );
    if ( m_buildIcon ) SDL_FreeSurface( m_buildIcon );
    for ( int i = 0; i < 16; i++ )
        if ( m_hdrIcon[i] ) SDL_FreeSurface( m_hdrIcon[i] );
}

void SDL2BuildingWindow::LoadIcons() {
    if ( m_matIcons ) return;
    CStatData* pMat = theIcons.GetByIndex( ICON_MATERIALS );
    if ( pMat && pMat->m_pcDib ) {
        m_matIcons = SDL2MainMenu::CreateSurfaceFromDIB( pMat->m_pcDib );
        m_matIconW = pMat->m_cxIcon;
        m_matIconH = pMat->m_cyIcon;
    }
    if ( m_bFertility && !m_densIcon ) {
        // Wheat/food icon (not the green density "X") — a farm's fertility maps to
        // how much food it can grow, so the food sheaf reads more naturally here.
        CStatData* pFood = theIcons.GetByIndex( ICON_FOOD );
        if ( pFood && pFood->m_pcDib ) {
            m_densIcon  = SDL2MainMenu::CreateSurfaceFromDIB( pFood->m_pcDib );
            m_densIconW = pFood->m_cxIcon;
            m_densIconH = pFood->m_cyIcon;
        }
    }
    if ( m_bUnits && !m_unitIcons ) {
        CStatData* pVeh = theIcons.GetByIndex( ICON_VEHICLES );
        if ( pVeh && pVeh->m_pcDib ) {
            m_unitIcons = SDL2MainMenu::CreateSurfaceFromDIB( pVeh->m_pcDib );
            m_unitIconW = pVeh->m_cxIcon;
            m_unitIconH = pVeh->m_cyIcon;
        }
    }
    if ( !m_bldgSheet ) {
        CDIB* pSheet = theBitmaps.GetByIndex( DIB_LIST_UNIT_BUILDINGS );
        if ( pSheet ) m_bldgSheet = SDL2MainMenu::CreateSurfaceFromDIB( pSheet );
    }
    if ( m_bBuilding && !m_buildIcon ) {
        CStatData* pBv = theIcons.GetByIndex( ICON_BUILD_VEH );
        if ( pBv && pBv->m_pcDib ) {
            m_buildIcon     = SDL2MainMenu::CreateSurfaceFromDIB( pBv->m_pcDib );
            m_buildIconW    = pBv->m_cxIcon;
            m_buildIconH    = pBv->m_cyIcon;
            m_buildLeftOff  = pBv->m_leftOff;
            m_buildRightOff = pBv->m_rightOff;
        }
    }
}

// Lazily convert a status-bar icon (theIcons sprite) to a surface for header glyphs.
SDL_Surface* SDL2BuildingWindow::HdrIcon(int idx) {
    if ( idx < 0 || idx >= 16 ) return nullptr;
    if ( m_hdrIcon[idx] ) return m_hdrIcon[idx];
    CStatData* pSd = theIcons.GetByIndex( idx );
    if ( pSd && pSd->m_pcDib )
        m_hdrIcon[idx] = SDL2MainMenu::CreateSurfaceFromDIB( pSd->m_pcDib );
    return m_hdrIcon[idx];
}

void SDL2BuildingWindow::OnInit() {
    LoadIcons();

    BldgLayout L = computeLayout( m_pBldg );

    // Identity band (portrait/name/flavor/status) spans the full width on top.
    int fullW = m_width - 20;
    int yTop  = BuildHeaderBand( m_x + 10, m_y + FIRST_Y, fullW );

    // One column (fullW) normally; two WIN_W-wide columns when the stack is tall.
    int colW = L.twoCol ? ( WIN_W - 20 ) : fullW;
    int x0   = m_x + 10;
    int x1   = m_x + 10 + WIN_W + COL_GAP;
    int y0   = yTop, y1 = yTop;

    for ( int i = 0; i < L.n; i++ ) {
        bool right = ( L.colOf[i] == 1 );
        int  cx    = right ? x1 : x0;
        int& cy    = right ? y1 : y0;
        cy = BuildSection( L.secs[i].id, cx, cy, colW );
    }

    int yClose = std::max( y0, y1 );
    AddWidget<SDL2Button>(m_x + m_width / 2 - 45, yClose + 2, 90, 28, "Close",
        [this]() {
            if ( CWndArea::GetShowRange() == m_bldgID )   // stop the range overlay now
                CWndArea::SetShowRange( 0 );
            EndDialog(0);
        });

    Refresh();
}

// Dispatch a section id to its builder. Order of ids matches computeLayout().
int SDL2BuildingWindow::BuildSection(int id, int x, int y, int w) {
    switch ( id ) {
        case SEC_STORAGE:    return BuildStorage   (x, y, w);
        case SEC_PRODUCTION: return BuildProduction(x, y, w);
        case SEC_BUILDING:   return BuildBuilding  (x, y, w);
        case SEC_FERTILITY:  return BuildFertility (x, y, w);
        case SEC_INPUTS:     return BuildInputs    (x, y, w);
        case SEC_OUTPUTS:    return BuildOutputs   (x, y, w);
        case SEC_UNITS:      return BuildUnits     (x, y, w);
        case SEC_REPAIR:     return BuildRepair    (x, y, w);
        case SEC_MILITARY:   return BuildMilitary  (x, y, w);
        case SEC_POWER:      return BuildPower     (x, y, w);
        case SEC_OFFICE:     return BuildOffice    (x, y, w);
        case SEC_APT:        return BuildApt       (x, y, w);
        case SEC_TURRET:     return BuildTurret    (x, y, w);
    }
    return y;
}

void SDL2BuildingWindow::OnFrame() {
    // [bw-throttle] cap Refresh() to ~6.7Hz instead of every frame — the live numbers
    // (build %, material/gas counts, contained-unit count) look identical to the user but
    // stop the per-frame SetText/texture/vehicle-map-scan work that tanked fps with a
    // building window open. Lossless: same data, sane cadence.
    Uint64 now = SDL_GetTicks64();
    if ( now < m_nextRefreshMs ) return;
    m_nextRefreshMs = now + 150;   // ms
    Refresh();
}

// ----------------------------------------------------------------------------
// chrome
// ----------------------------------------------------------------------------
void SDL2BuildingWindow::AddOutline(int x, int y, int w, int h) {
    auto* img = AddWidget<SDL2Image>(x, y, w, h);
    SDL_Surface* s = SDL_CreateRGBSurface(0, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if ( !s ) return;
    SDL_FillRect(s, nullptr, SDL_MapRGBA(s->format, 0, 0, 0, 0));   // transparent interior
    Uint32 dark  = SDL_MapRGBA(s->format,  60,  48,  28, 255);
    Uint32 light = SDL_MapRGBA(s->format, 150, 128,  78, 255);
    // simple 2px raised frame
    SDL_Rect t = { 0, 0, w, 1 }, l = { 0, 0, 1, h };
    SDL_Rect b = { 0, h - 1, w, 1 }, r = { w - 1, 0, 1, h };
    SDL_FillRect(s, &t, light); SDL_FillRect(s, &l, light);
    SDL_FillRect(s, &b, dark);  SDL_FillRect(s, &r, dark);
    SDL_Rect t2 = { 1, 1, w - 2, 1 }, l2 = { 1, 1, 1, h - 2 };
    SDL_FillRect(s, &t2, dark);  SDL_FillRect(s, &l2, dark);
    SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_BLEND);
    img->SetSurface(s, true);
}

int SDL2BuildingWindow::Header(int x, int y, int w, const char* text, SDL_Color color,
                               int iconIdx, int iconFrame) {
    int textX = x;

    // Category glyph: blit ONE frame of the icon strip (status sprites are multi-
    // frame; iconFrame picks which) into its own surface, scaled up to fill the
    // header height so it reads clearly next to the bold text.
    SDL_Surface* ico = ( iconIdx >= 0 ) ? HdrIcon( iconIdx ) : nullptr;
    CStatData*   pSd = ( iconIdx >= 0 ) ? theIcons.GetByIndex( iconIdx ) : nullptr;
    if ( ico && pSd && pSd->m_cxIcon > 0 && pSd->m_cyIcon > 0 ) {
        int fw = pSd->m_cxIcon, fh = pSd->m_cyIcon;
        // Some status icons (ICON_DAMAGE, ICON_CONSTRUCTION) are WIDE bar sprites;
        // scaled by height alone they'd stretch into a long bar that overruns the
        // header. Crop the frame to a near-square (left portion) so every glyph is
        // a compact chip regardless of the source aspect.
        int cropW = ( fw > fh ) ? fh : fw;
        int gh = HDR_H - 2;
        int gw = ( fh > 0 ) ? ( cropW * gh / fh ) : cropW;
        auto* img = AddWidget<SDL2Image>( x, y + 1, gw, gh );
        SDL_Surface* s = SDL_CreateRGBSurface( 0, cropW, fh, 32,
                                               0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 );
        if ( s ) {
            SDL_FillRect( s, nullptr, SDL_MapRGBA( s->format, 0, 0, 0, 0 ) );
            SDL_SetSurfaceBlendMode( ico, SDL_BLENDMODE_BLEND );
            int srcX = iconFrame * fw;
            if ( srcX + cropW > ico->w ) srcX = 0;     // frame out of range -> first frame
            SDL_Rect sr = { srcX, 0, cropW, fh };
            SDL_BlitSurface( ico, &sr, s, nullptr );
            SDL_SetSurfaceBlendMode( s, SDL_BLENDMODE_BLEND );
            img->SetSurface( s, true );
        }
        textX = x + gw + 6;
    }

    auto* h = AddWidget<SDL2Label>( textX, y, w - ( textX - x ), HDR_H, text );
    h->SetColor( color );
    h->SetBold( true );
    return y + HDR_H;
}

// The band under the title bar: the building's portrait on the left, its name (bold)
// and one-line flavor text to the right, and a live colored status line beneath them.
int SDL2BuildingWindow::BuildHeaderBand(int x, int y, int w) {
    // Portrait from the building-list sprite sheet (row per building type, the icon
    // region starts at srcX=20, each tile 64px tall — same as the unit-list panel).
    if ( m_bldgSheet && m_pBldg ) {
        int type = m_pBldg->GetData()->GetType();
        int srcY = PORTRAIT * type;
        if ( srcY + PORTRAIT <= m_bldgSheet->h ) {
            int srcX = 20;
            int srcW = m_bldgSheet->w - srcX;
            if ( srcW > 0 ) {
                auto* img = AddWidget<SDL2Image>( x, y, PORTRAIT, PORTRAIT );
                SDL_Surface* s = SDL_CreateRGBSurface( 0, srcW, PORTRAIT, 32,
                                                       0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 );
                if ( s ) {
                    SDL_FillRect( s, nullptr, SDL_MapRGBA( s->format, 0, 0, 0, 0 ) );
                    SDL_SetSurfaceBlendMode( m_bldgSheet, SDL_BLENDMODE_BLEND );
                    SDL_Rect sr = { srcX, srcY, srcW, PORTRAIT };
                    SDL_BlitSurface( m_bldgSheet, &sr, s, nullptr );
                    SDL_SetSurfaceBlendMode( s, SDL_BLENDMODE_BLEND );
                    img->SetSurface( s, true );
                }
            }
        }
    }

    // Condition bar directly under the portrait (this is the real "health" display —
    // the green/amber/red fill shows how damaged the building is).
    m_imgHealth = AddWidget<SDL2Image>( x, y + PORTRAIT + 3, PORTRAIT, 10 );

    int tx = x + PORTRAIT + 10;
    int tw = w - PORTRAIT - 10;

    auto* lblName = AddWidget<SDL2Label>( tx, y, tw, 20, m_pBldg->GetData()->GetDesc().c_str() );
    lblName->SetColor( kHeaderBlue );
    lblName->SetBold( true );

    // Flavor: smaller font + 3 lines of room so longer descriptions aren't clipped.
    std::string flavor = m_pBldg->GetData()->GetText().c_str();
    if ( !flavor.empty() ) {
        auto* lblFlavor = AddWidget<SDL2Label>( tx, y + 21, tw, 42, flavor.c_str() );
        lblFlavor->SetWrapped( true );
        lblFlavor->SetTopAligned( true );
        lblFlavor->SetFontSize( 12 );
        lblFlavor->SetColor( { 70, 56, 30, 255 } );   // muted brown, "parchment ink"
    }

    m_lblStatus = AddWidget<SDL2Label>( tx, y + HEADER_H - 18, tw, 16, "" );
    m_lblStatus->SetBold( true );

    return y + HEADER_H;
}

// ----------------------------------------------------------------------------
// Section builders (each draws its outline first, then its content)
// ----------------------------------------------------------------------------
int SDL2BuildingWindow::BuildStorage(int x, int y, int w) {
    AddOutline(x, y, w, STORAGE_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Storage", kAccentGold,
                    ICON_MATERIALS, CMaterialTypes::steel);

    // each row: name (left) | icon stack (middle) | amount (right)
    int nameW  = 64;
    int countW = 48;
    int iconsX = x + BOX_PAD + nameW + 4;
    int iconsW = ( x + w - BOX_PAD - countW - 4 ) - iconsX;

    m_imgStorage = AddWidget<SDL2Image>(iconsX, yh, iconsW, kNumStoreMats * ROW_H);
    for (int i = 0; i < kNumStoreMats; i++) {
        int ry = yh + i * ROW_H;
        m_lblStoreName[i]  = AddWidget<SDL2Label>(x + BOX_PAD, ry, nameW, ROW_H, kStoreNames[i]);
        m_lblStoreCount[i] = AddWidget<SDL2Label>(x + w - BOX_PAD - countW, ry, countW, ROW_H, "0");
        m_lblStoreCount[i]->SetRightAligned(true);
    }
    return y + STORAGE_H + SEC_PAD;
}

int SDL2BuildingWindow::BuildProduction(int x, int y, int w) {
    AddOutline(x, y, w, PRODUCTION_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Production", kAccentGold, ICON_CONSTRUCTION);
    m_lblProduction = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh, w - 2 * BOX_PAD - 8, 2 * ROW_H, "");
    m_lblProduction->SetWrapped(true);
    m_lblProduction->SetTopAligned(true);
    // Progress toward the next output batch (mines/farms/smelters work on a timer).
    m_progProduction = AddWidget<SDL2ProgressBar>(x + BOX_PAD + 4, yh + 2 * ROW_H + 4,
                                                  w - 2 * BOX_PAD - 8, 16);
    return y + PRODUCTION_H + SEC_PAD;
}

// power / office / apartment share a layout: two text lines on the left, graph on
// the right.
int SDL2BuildingWindow::BuildPower(int x, int y, int w) {
    AddOutline(x, y, w, POWERLIKE_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Power", kHeaderBlue, ICON_POWER);
    int graphW = 168, graphX = x + w - graphW - BOX_PAD;
    int textW  = graphX - ( x + BOX_PAD + 4 ) - 6;
    m_lblPowerBldg   = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6,         textW, ROW_H, "This building: 0");
    m_lblPowerColony = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6 + ROW_H, textW, ROW_H, "Colony: 0 / 0");
    m_imgPowerGraph  = AddWidget<SDL2Image>(graphX, yh, graphW, GRAPH_H);
    return y + POWERLIKE_H + SEC_PAD;
}

int SDL2BuildingWindow::BuildOffice(int x, int y, int w) {
    AddOutline(x, y, w, POWERLIKE_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Offices", kAccentGrn, ICON_PEOPLE);
    int graphW = 168, graphX = x + w - graphW - BOX_PAD;
    int textW  = graphX - ( x + BOX_PAD + 4 ) - 6;
    m_lblOfcBldg   = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6,         textW, ROW_H, "This building: 0");
    m_lblOfcColony = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6 + ROW_H, textW, ROW_H, "Colony: 0 / 0");
    m_imgOfcGraph  = AddWidget<SDL2Image>(graphX, yh, graphW, GRAPH_H);
    return y + POWERLIKE_H + SEC_PAD;
}

int SDL2BuildingWindow::BuildApt(int x, int y, int w) {
    AddOutline(x, y, w, POWERLIKE_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Population", kAccentGrn, ICON_PEOPLE);
    int graphW = 168, graphX = x + w - graphW - BOX_PAD;
    int textW  = graphX - ( x + BOX_PAD + 4 ) - 6;
    m_lblAptBldg   = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6,         textW, ROW_H, "This building: 0");
    m_lblAptColony = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6 + ROW_H, textW, ROW_H, "Colony: 0 / 0");
    m_imgAptGraph  = AddWidget<SDL2Image>(graphX, yh, graphW, GRAPH_H);
    return y + POWERLIKE_H + SEC_PAD;
}

int SDL2BuildingWindow::BuildTurret(int x, int y, int w) {
    AddOutline(x, y, w, TURRET_H);
    // No glyph here: ICON_DAMAGE is the building's health-bar sprite (it stretched
    // into a confusing red bar), and the building's condition now has its own bar
    // under the portrait. The bold red "Weapon" header is clear on its own.
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Weapon", kAccentRed);

    // 2x2 grid so each stat sits in its own column and never runs past the box edge
    // (the old single-line "Range: N  Damage: N/shot" was overflowing on the right).
    int gx   = x + BOX_PAD + 4;
    int colW = ( w - 2 * BOX_PAD - 8 ) / 2;
    m_lblTurretRange  = AddWidget<SDL2Label>(gx,          yh,         colW, ROW_H, "");
    m_lblTurretDmg    = AddWidget<SDL2Label>(gx + colW,   yh,         colW, ROW_H, "");
    m_lblTurretReload = AddWidget<SDL2Label>(gx,          yh + ROW_H, colW, ROW_H, "");
    m_lblTurretDps    = AddWidget<SDL2Label>(gx + colW,   yh + ROW_H, colW, ROW_H, "");
    // 13pt keeps each "Label: value" comfortably inside its column.
    m_lblTurretRange->SetFontSize(13);  m_lblTurretDmg->SetFontSize(13);
    m_lblTurretReload->SetFontSize(13); m_lblTurretDps->SetFontSize(13);

    // Toggle: draw a red range circle around this building on the map.
    m_btnShowRange = AddWidget<SDL2Button>(gx, yh + 2 * ROW_H + 6, 140, 26, "Show Range",
        [this]() {
            m_bShowRange = !m_bShowRange;
            m_btnShowRange->SetToggled( m_bShowRange );
            CWndArea::SetShowRange( m_bShowRange ? m_bldgID : 0 );
        });
    return y + TURRET_H + SEC_PAD;
}

// Command center: colony military summary (it's the military HQ).
int SDL2BuildingWindow::BuildMilitary(int x, int y, int w) {
    AddOutline(x, y, w, MILITARY_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Military", kAccentRed, ICON_VEHICLES);
    int tw = w - 2 * BOX_PAD - 8;
    m_lblMilStrength = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh,             tw, ROW_H, "Strength: 0");
    m_lblInfantry    = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + ROW_H,     tw, ROW_H, "Infantry: 0");
    m_lblVehicles    = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 2 * ROW_H, tw, ROW_H, "Vehicles: 0");
    return y + MILITARY_H + SEC_PAD;
}

// Repair building: the unit being serviced (with a repair bar) plus the wait queue.
int SDL2BuildingWindow::BuildRepair(int x, int y, int w) {
    AddOutline(x, y, w, REPAIR_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Repair Queue", kHeaderBlue, ICON_REPAIR_VEH);
    int tw = w - 2 * BOX_PAD - 8;
    m_progRepair = AddWidget<SDL2ProgressBar>(x + BOX_PAD + 4, yh, tw, 16);
    int ly = yh + 16 + 6;
    for (int i = 0; i < kRepairRows; i++) {
        m_lblRepair[i] = AddWidget<SDL2Label>(x + BOX_PAD + 4, ly + i * ROW_H, tw, ROW_H, "");
    }
    return y + REPAIR_H + SEC_PAD;
}

// Production buildings: a limited storage widget for just the materials they
// consume (e.g. oil for a refinery), so you can see whether they're being fed.
int SDL2BuildingWindow::BuildInputs(int x, int y, int w) {
    int sectH = BOX_PAD + HDR_H + m_nInputMats * ROW_H + BOX_PAD;
    AddOutline(x, y, w, sectH);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Inputs", kAccentGold,
                    ICON_MATERIALS, ( m_nInputMats > 0 ) ? m_inputMats[0] : 0);

    int nameW  = 80;
    int countW = 48;
    int iconsX = x + BOX_PAD + nameW + 4;
    int iconsW = ( x + w - BOX_PAD - countW - 4 ) - iconsX;

    m_imgInputs = AddWidget<SDL2Image>(iconsX, yh, iconsW, m_nInputMats * ROW_H);
    for (int i = 0; i < m_nInputMats; i++) {
        int ry = yh + i * ROW_H;
        m_lblInputName[i]  = AddWidget<SDL2Label>(x + BOX_PAD, ry, nameW, ROW_H,
                                CMaterialTypes::GetDesc( m_inputMats[i] ).c_str());
        m_lblInputCount[i] = AddWidget<SDL2Label>(x + w - BOX_PAD - countW, ry, countW, ROW_H, "0");
        m_lblInputCount[i]->SetRightAligned(true);
    }
    return y + sectH + SEC_PAD;
}

// Production buildings: a limited storage widget for the materials they produce
// (steel for a smelter, ore for a mine), so you can see the stockpile awaiting pickup.
int SDL2BuildingWindow::BuildOutputs(int x, int y, int w) {
    int sectH = BOX_PAD + HDR_H + m_nOutputMats * ROW_H + BOX_PAD;
    AddOutline(x, y, w, sectH);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Output", kAccentGold,
                    ICON_MATERIALS, ( m_nOutputMats > 0 ) ? m_outputMats[0] : 0);

    int nameW  = 80;
    int countW = 48;
    int iconsX = x + BOX_PAD + nameW + 4;
    int iconsW = ( x + w - BOX_PAD - countW - 4 ) - iconsX;

    m_imgOutputs = AddWidget<SDL2Image>(iconsX, yh, iconsW, m_nOutputMats * ROW_H);
    for (int i = 0; i < m_nOutputMats; i++) {
        int ry = yh + i * ROW_H;
        m_lblOutputName[i]  = AddWidget<SDL2Label>(x + BOX_PAD, ry, nameW, ROW_H,
                                 CMaterialTypes::GetDesc( m_outputMats[i] ).c_str());
        m_lblOutputCount[i] = AddWidget<SDL2Label>(x + w - BOX_PAD - countW, ry, countW, ROW_H, "0");
        m_lblOutputCount[i]->SetRightAligned(true);
    }
    return y + sectH + SEC_PAD;
}

// Seaport: a strip of icons for the vehicles currently docked inside, plus a count.
int SDL2BuildingWindow::BuildUnits(int x, int y, int w) {
    AddOutline(x, y, w, UNITS_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Units Inside", kHeaderBlue, ICON_VEHICLES);

    int countW = 48;
    int iconsX = x + BOX_PAD + 4;
    int iconsW = ( x + w - BOX_PAD - countW - 6 ) - iconsX;

    m_imgUnits = AddWidget<SDL2Image>(iconsX, yh, iconsW, UNITS_ROW_H);
    m_lblUnits = AddWidget<SDL2Label>(x + w - BOX_PAD - countW, yh, countW, UNITS_ROW_H, "0");
    m_lblUnits->SetRightAligned(true);
    return y + UNITS_H + SEC_PAD;
}

// Vehicle plant / shipyard: what unit is being built + a construction-progress bar
// drawn with the ICON_BUILD_VEH art on a black, gold-bordered box (the same look as
// the bottom status bar and the build dialog).
int SDL2BuildingWindow::BuildBuilding(int x, int y, int w) {
    AddOutline(x, y, w, BUILDING_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Building", kAccentGold, ICON_BUILD_VEH);
    m_lblBuildName = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh, w - 2 * BOX_PAD - 8, ROW_H, "");
    m_imgBuildBar  = AddWidget<SDL2Image>(x + BOX_PAD + 4, yh + ROW_H + 4,
                                          w - 2 * BOX_PAD - 8, BUILD_BAR_H);
    return y + BUILDING_H + SEC_PAD;
}

// Black box + gold border + ICON_BUILD_VEH icons tiled left-to-right to `per`% —
// replays the original CStatInst::DrawStatDone look inside our own surface.
void SDL2BuildingWindow::DrawBuildBar(SDL2Image* img, int per) {
    if ( !img ) return;
    if ( per < 0 ) per = 0; if ( per > 100 ) per = 100;
    SDL_Rect rc = img->GetRect();
    int gw = rc.w, gh = rc.h;
    if ( gw <= 4 || gh <= 4 ) return;

    SDL_Surface* s = SDL_CreateRGBSurface(0, gw, gh, 32,
                                          0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if ( !s ) return;
    SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 12, 10, 8));        // black recess
    Uint32 gold = SDL_MapRGB(s->format, 150, 128, 78);
    SDL_Rect t = { 0,0,gw,1 }, b = { 0,gh-1,gw,1 }, l = { 0,0,1,gh }, r = { gw-1,0,1,gh };
    SDL_FillRect(s,&t,gold); SDL_FillRect(s,&b,gold); SDL_FillRect(s,&l,gold); SDL_FillRect(s,&r,gold);

    if ( m_buildIcon && m_buildIconW > 0 && m_buildIconH > 0 ) {
        int top   = ( gh - m_buildIconH ) / 2;
        int left  = m_buildLeftOff + 1;
        int right = gw - m_buildRightOff - 1;
        int iEnd  = right;
        if ( per < 100 ) iEnd -= m_buildIconW / 2;       // last icon only lands at 100%
        int iRight = left + ( ( right - left ) * per ) / 100;
        if ( per > 0 && iRight < left + 1 ) iRight = left + 1;
        SDL_SetSurfaceBlendMode( m_buildIcon, SDL_BLENDMODE_BLEND );
        SDL_Rect src = { 0, 0, m_buildIconW, m_buildIconH };
        for ( int ix = left; ix < iRight; ix += m_buildIconW / 2 ) {
            if ( ix + m_buildIconW > iEnd ) break;
            SDL_Rect dr = { ix, top, m_buildIconW, m_buildIconH };
            SDL_BlitSurface( m_buildIcon, &src, s, &dr );
        }
    }
    img->SetSurface(s, true);
}

// Farm soil fertility, drawn as a row of the green ICON_DENSITY "X"s (the same art
// the original status bar used) plus a "NN%" readout on the right.
int SDL2BuildingWindow::BuildFertility(int x, int y, int w) {
    AddOutline(x, y, w, FERTILITY_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Fertility", kAccentGrn, ICON_FOOD);

    int countW = 48;
    int iconsX = x + BOX_PAD + 4;
    int iconsW = ( x + w - BOX_PAD - countW - 6 ) - iconsX;

    m_imgFertility = AddWidget<SDL2Image>(iconsX, yh, iconsW, ROW_H);
    m_lblFertility = AddWidget<SDL2Label>(x + w - BOX_PAD - countW, yh, countW, ROW_H, "0%");
    m_lblFertility->SetRightAligned(true);
    return y + FERTILITY_H + SEC_PAD;
}

// ----------------------------------------------------------------------------
// Per-building live values
// ----------------------------------------------------------------------------
int SDL2BuildingWindow::PerBldgPower() const {
    if ( !m_pBldg ) return 0;
    CStructureData* pData = (CStructureData*)m_pBldg->GetData();
    if ( pData->GetType() == CStructureData::rocket )
        return (int)( 15.0 * m_pBldg->GetFrameProd(1) );
    if ( pData->GetUnionType() == CStructureData::UTpower ) {
        CBuildPower* pBp = (CBuildPower*)pData->GetBldPower();
        if ( pBp ) return (int)( (float)pBp->GetPower() * m_pBldg->GetFrameProd(1) );
    }
    return 0;
}

int SDL2BuildingWindow::PerBldgAptCap() const {
    if ( !m_pBldg ) return 0;
    CStructureData* pData = (CStructureData*)m_pBldg->GetData();
    if ( pData->GetType() == CStructureData::rocket ) return ROCKET_APT_CAP;
    if ( ( pData->GetUnionType() == CStructureData::UThousing ) &&
         ( pData->GetBldgType()  == CStructureData::apartment ) )
        return m_pBldg->GetOwner()->GetHousingCap( pData->GetPopHoused() );
    return 0;
}

int SDL2BuildingWindow::PerBldgOfcCap() const {
    if ( !m_pBldg ) return 0;
    CStructureData* pData = (CStructureData*)m_pBldg->GetData();
    if ( pData->GetType() == CStructureData::rocket ) return ROCKET_OFC_CAP;
    if ( ( pData->GetUnionType() == CStructureData::UThousing ) &&
         ( pData->GetBldgType()  == CStructureData::office ) )
        return m_pBldg->GetOwner()->GetHousingCap( pData->GetPopHoused() );
    return 0;
}

// ----------------------------------------------------------------------------
// Storage icon stacks
// ----------------------------------------------------------------------------
void SDL2BuildingWindow::DrawMatIcons(SDL2Image* img, const int* mats, int n) {
    if ( !img || !m_pBldg || n <= 0 ) return;
    SDL_Rect rc = img->GetRect();
    int gw = rc.w, gh = rc.h;
    if ( gw <= 2 || gh <= 2 ) return;

    SDL_Surface* s = SDL_CreateRGBSurface(0, gw, gh, 32,
                                          0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if ( !s ) return;
    SDL_FillRect(s, nullptr, SDL_MapRGBA(s->format, 0, 0, 0, 0));
    SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_BLEND);

    if ( m_matIcons && m_matIconW > 0 && m_matIconH > 0 ) {
        const int PER_ICON = 250;   // each stacked icon ~= 250 units
        int rowH  = gh / n;
        int iconH = std::min( m_matIconH, rowH - 2 );
        int iconW = ( iconH > 0 ) ? ( m_matIconW * iconH / m_matIconH ) : m_matIconW;
        int step  = iconW + 1;
        int maxFit = ( step > 0 ) ? ( gw / step ) : 0;

        SDL_SetSurfaceBlendMode(m_matIcons, SDL_BLENDMODE_BLEND);
        for (int i = 0; i < n; i++) {
            int amount = m_pBldg->GetStore( mats[i] );
            if ( amount <= 0 ) continue;
            int nIcons = amount / PER_ICON;
            if ( nIcons < 1 )      nIcons = 1;
            if ( nIcons > maxFit ) nIcons = maxFit;
            int iy = i * rowH + ( rowH - iconH ) / 2;
            SDL_Rect src = { mats[i] * m_matIconW, 0, m_matIconW, m_matIconH };
            for (int k = 0; k < nIcons; k++) {
                SDL_Rect dr = { k * step, iy, iconW, iconH };
                SDL_BlitScaled(m_matIcons, &src, s, &dr);
            }
        }
    }
    img->SetSurface(s, true);
}

// Tile the green ICON_DENSITY "X" across pct% of the image width — the farm-window
// echo of the original status bar's fertility display.
void SDL2BuildingWindow::DrawDensityIcons(SDL2Image* img, int pct) {
    if ( !img ) return;
    if ( pct < 0 )   pct = 0;
    if ( pct > 100 ) pct = 100;
    SDL_Rect rc = img->GetRect();
    int gw = rc.w, gh = rc.h;
    if ( gw <= 2 || gh <= 2 ) return;

    SDL_Surface* s = SDL_CreateRGBSurface(0, gw, gh, 32,
                                          0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if ( !s ) return;
    SDL_FillRect(s, nullptr, SDL_MapRGBA(s->format, 0, 0, 0, 0));
    SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_BLEND);

    if ( m_densIcon && m_densIconW > 0 && m_densIconH > 0 ) {
        int iconH = std::min( m_densIconH, gh - 2 );
        int iconW = ( iconH > 0 ) ? ( m_densIconW * iconH / m_densIconH ) : m_densIconW;
        int step  = iconW + 1;
        int fillW = gw * pct / 100;             // how far the "X"s extend
        int iy    = ( gh - iconH ) / 2;
        SDL_SetSurfaceBlendMode(m_densIcon, SDL_BLENDMODE_BLEND);
        SDL_Rect src = { 0, 0, m_densIconW, m_densIconH };
        for (int dx = 0; ( dx + iconW ) <= fillW && step > 0; dx += step) {
            SDL_Rect dr = { dx, iy, iconW, iconH };
            SDL_BlitScaled(m_densIcon, &src, s, &dr);
        }
    }
    img->SetSurface(s, true);
}

// Draw the icons of my vehicles parked inside this building, left to right — the
// window echo of the toolbar's seaport "contained units" strip. One ICON_VEHICLES
// tile per vehicle (srcX = GetType() * iconW), so you see exactly what's docked.
void SDL2BuildingWindow::DrawContainedUnits(SDL2Image* img) {
    if ( !img || !m_pBldg ) return;
    SDL_Rect rc = img->GetRect();
    int gw = rc.w, gh = rc.h;
    if ( gw <= 2 || gh <= 2 ) return;

    SDL_Surface* s = SDL_CreateRGBSurface(0, gw, gh, 32,
                                          0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if ( !s ) return;
    SDL_FillRect(s, nullptr, SDL_MapRGBA(s->format, 0, 0, 0, 0));
    SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_BLEND);

    if ( m_unitIcons && m_unitIconW > 0 && m_unitIconH > 0 ) {
        int iconH = std::min( m_unitIconH, gh - 2 );
        int iconW = ( iconH > 0 ) ? ( m_unitIconW * iconH / m_unitIconH ) : m_unitIconW;
        int step  = iconW + 2;
        int iy    = ( gh - iconH ) / 2;
        int drawX = 0;
        SDL_SetSurfaceBlendMode( m_unitIcons, SDL_BLENDMODE_BLEND );
        POSITION pos = theVehicleMap.GetStartPosition();
        while ( pos != NULL ) {
            DWORD dwID; CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            if ( !pVeh || !pVeh->GetOwner() || !pVeh->GetOwner()->IsMe() ) continue;
            if ( pVeh->GetHexOwnership() ) continue;
            if ( theBuildingHex._GetBuilding( pVeh->GetPtHead() ) != m_pBldg ) continue;
            if ( drawX + iconW > gw ) break;       // strip full
            int srcX = pVeh->GetData()->GetType() * m_unitIconW;
            if ( srcX + m_unitIconW <= m_unitIcons->w ) {
                SDL_Rect src = { srcX, 0, m_unitIconW, m_unitIconH };
                SDL_Rect dr  = { drawX, iy, iconW, iconH };
                SDL_BlitScaled( m_unitIcons, &src, s, &dr );
            }
            drawX += step;
        }
    }
    img->SetSurface(s, true);
}

// Building condition bar: a dark track with a fill proportional to the building's
// remaining health, colored green (healthy) -> amber -> red (badly damaged).
void SDL2BuildingWindow::DrawHealthBar() {
    if ( !m_imgHealth || !m_pBldg ) return;
    SDL_Rect rc = m_imgHealth->GetRect();
    int gw = rc.w, gh = rc.h;
    if ( gw <= 2 || gh <= 2 ) return;

    int hp = m_pBldg->GetDamagePer();   // remaining-health percent (0..100)
    if ( hp < 0 ) hp = 0; if ( hp > 100 ) hp = 100;

    SDL_Surface* s = SDL_CreateRGBSurface(0, gw, gh, 32,
                                          0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if ( !s ) return;
    SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 24, 22, 18));          // dark track
    SDL_Rect frame = { 0, 0, gw, gh };
    Uint32 gold = SDL_MapRGB(s->format, 150, 128, 78);
    SDL_Rect t = { 0,0,gw,1 }, b = { 0,gh-1,gw,1 }, l = { 0,0,1,gh }, r = { gw-1,0,1,gh };
    SDL_FillRect(s,&t,gold); SDL_FillRect(s,&b,gold); SDL_FillRect(s,&l,gold); SDL_FillRect(s,&r,gold);

    int fillW = ( ( gw - 2 ) * hp ) / 100;
    if ( fillW > 0 ) {
        Uint32 col = ( hp >= 66 ) ? SDL_MapRGB(s->format, 70, 200, 80)
                   : ( hp >= 33 ) ? SDL_MapRGB(s->format, 220, 190, 40)
                                  : SDL_MapRGB(s->format, 210, 60, 40);
        SDL_Rect fr = { 1, 1, fillW, gh - 2 };
        SDL_FillRect(s, &fr, col);
    }
    m_imgHealth->SetSurface(s, true);
}

// ----------------------------------------------------------------------------
// History graph
// ----------------------------------------------------------------------------
void SDL2BuildingWindow::DrawGraph(SDL2Image* img, HistSeries a, HistSeries b) {
    if ( !img || !m_pBldg ) return;
    CPlayer* p = m_pBldg->GetOwner();
    if ( !p ) return;

    SDL_Rect rc = img->GetRect();
    int gw = rc.w, gh = rc.h;
    if ( gw <= 2 || gh <= 2 ) return;

    SDL_Surface* s = SDL_CreateRGBSurface(0, gw, gh, 32,
                                          0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if ( !s ) return;
    SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 26, 24, 20));
    Uint32 frame = SDL_MapRGB(s->format, 90, 80, 55);
    SDL_Rect top = { 0, 0, gw, 1 }, bot = { 0, gh - 1, gw, 1 };
    SDL_Rect lft = { 0, 0, 1, gh }, rgt = { gw - 1, 0, 1, gh };
    SDL_FillRect(s, &top, frame); SDL_FillRect(s, &bot, frame);
    SDL_FillRect(s, &lft, frame); SDL_FillRect(s, &rgt, frame);

    auto valOf = [&](HistSeries hs, int i) -> long {
        switch (hs) {
            case kPwrHave:  return p->GetHistPwrHave(i);
            case kPwrNeed:  return p->GetHistPwrNeed(i);
            case kPplTotal: return p->GetHistPplTotal(i);
            case kPplBldg:  return p->GetHistPplBldg(i);
            case kAptCap:   return p->GetHistAptCap(i);
            case kOfcCap:   return p->GetHistOfcCap(i);
            default:        return 0;
        }
    };

    int n = p->GetHistCount();
    if ( n >= 2 ) {
        long maxV = 1;
        for ( int i = 0; i < n; i++ ) {
            maxV = __max( maxV, valOf(a, i) );
            if ( b != kNone ) maxV = __max( maxV, valOf(b, i) );
        }
        const int pad = 2;
        int plotW = gw - 2 * pad, plotH = gh - 2 * pad;
        HistSeries series[2] = { a, b };
        Uint32     colors[2] = { SDL_MapRGB(s->format, 90, 220, 110),
                                 SDL_MapRGB(s->format, 235, 180, 60) };
        for ( int sIdx = 0; sIdx < 2; sIdx++ ) {
            if ( series[sIdx] == kNone ) continue;
            int prevX = 0, prevY = 0;
            for ( int i = 0; i < n; i++ ) {
                int px = pad + ( i * ( plotW - 1 ) ) / ( n - 1 );
                int py = pad + ( plotH - 1 ) - (int)( ( valOf(series[sIdx], i) * (long)( plotH - 1 ) ) / maxV );
                if ( i > 0 ) lineOnSurface(s, prevX, prevY, px, py, colors[sIdx]);
                prevX = px; prevY = py;
            }
        }
    }
    img->SetSurface(s, true);
}

// ----------------------------------------------------------------------------
// Count my combat units in one pass over the vehicle map. "Strength" is the
// summed firepower (attack across all target types) of every unit that can shoot;
// infantry vs. vehicles are split by IsPeople(). Called ~once a second, not per
// frame, so a parked command-center window never taxes the running game.
// ----------------------------------------------------------------------------
void SDL2BuildingWindow::ComputeMilitary() {
    int strength = 0, infantry = 0, vehicles = 0;
    POSITION pos = theVehicleMap.GetStartPosition();
    while ( pos != NULL ) {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        if ( !pVeh || !pVeh->GetOwner() || !pVeh->GetOwner()->IsMe() ) continue;
        if ( pVeh->GetFireRate() <= 0 ) continue;   // unarmed (trucks / cranes / scouts)
        strength += pVeh->GetAttack(0) + pVeh->GetAttack(1) + pVeh->GetAttack(2);
        if ( pVeh->GetData()->IsPeople() ) infantry++;
        else                               vehicles++;
    }
    m_iMilStrength = strength;
    m_iInfantry    = infantry;
    m_iVehicles    = vehicles;
}

// ----------------------------------------------------------------------------
void SDL2BuildingWindow::Refresh() {
    if ( !m_pBldg ) return;
    CPlayer* p = m_pBldg->GetOwner();
    if ( !p ) return;

    // At-a-glance status line: under construction, starved for an input, or the
    // building's own status text ("making gas, 60%" / "Idle"), color-coded.
    if ( m_lblStatus ) {
        std::string st; SDL_Color col = kStatusOk;
        if ( m_pBldg->IsConstructing() ) {
            st = "Under construction"; col = kStatusWarn;
        } else {
            int missing = -1;
            for ( int i = 0; i < m_nInputMats; i++ )
                if ( m_pBldg->GetStore( m_inputMats[i] ) <= 0 ) { missing = m_inputMats[i]; break; }
            if ( missing >= 0 ) {
                st  = "Low on " + std::string( CMaterialTypes::GetDesc( missing ).c_str() );
                col = kStatusWarn;
            } else {
                std::string s2;
                m_pBldg->ShowStatusText( s2 );
                if ( !s2.empty() ) {
                    st  = s2;
                    col = ( s2.find( "dle" ) != std::string::npos ) ? kStatusWarn : kStatusOk;
                } else {
                    st = "Operating"; col = kStatusOk;
                }
            }
        }
        m_lblStatus->SetText( st );
        m_lblStatus->SetColor( col );
    }

    DrawHealthBar();

    if ( m_bStorage ) {
        DrawMatIcons( m_imgStorage, kStoreMats, kNumStoreMats );
        for ( int i = 0; i < kNumStoreMats; i++ )
            if ( m_lblStoreCount[i] )
                m_lblStoreCount[i]->SetText( FmtNum( m_pBldg->GetStore( kStoreMats[i] ) ) );
    }

    if ( m_bProduction && m_lblProduction ) {
        std::string str;
        m_pBldg->ShowStatusText( str );
        m_lblProduction->SetText( str );
        if ( m_progProduction ) {
            int per = m_pBldg->GetProductionPer();
            if ( per >= 0 ) { m_progProduction->SetVisible( true ); m_progProduction->SetProgress( per ); }
            else            { m_progProduction->SetVisible( false ); }
        }
    }

    if ( m_bInputs ) {
        DrawMatIcons( m_imgInputs, m_inputMats, m_nInputMats );
        for ( int i = 0; i < m_nInputMats; i++ )
            if ( m_lblInputCount[i] )
                m_lblInputCount[i]->SetText( FmtNum( m_pBldg->GetStore( m_inputMats[i] ) ) );
    }

    if ( m_bOutputs ) {
        DrawMatIcons( m_imgOutputs, m_outputMats, m_nOutputMats );
        for ( int i = 0; i < m_nOutputMats; i++ )
            if ( m_lblOutputCount[i] )
            {
                // Gas is a COLONY-WIDE resource — it isn't stored per-building, so
                // GetStore() is always 0 for it (e.g. the refinery's gas output read 0).
                // Show the colony's gas-on-hand instead. (Fuller have/usage + history
                // graph treatment, like power/apt, is the follow-up.)
                int amt = ( m_outputMats[i] == CMaterialTypes::gas )
                              ? m_pBldg->GetOwner()->GetGasHave()
                              : m_pBldg->GetStore( m_outputMats[i] );
                m_lblOutputCount[i]->SetText( FmtNum( amt ) );
            }
    }

    if ( m_bUnits ) {
        DrawContainedUnits( m_imgUnits );
        if ( m_lblUnits ) {
            int n = 0;
            POSITION pos = theVehicleMap.GetStartPosition();
            while ( pos != NULL ) {
                DWORD dwID; CVehicle* pVeh;
                theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
                if ( pVeh && pVeh->GetOwner() && pVeh->GetOwner()->IsMe() &&
                     !pVeh->GetHexOwnership() &&
                     theBuildingHex._GetBuilding( pVeh->GetPtHead() ) == m_pBldg )
                    n++;
            }
            m_lblUnits->SetText( n > 0 ? FmtNum( n ) : std::string( "none" ) );
        }
    }

    if ( m_bBuilding ) {
        CVehicleBuilding* pVb = (CVehicleBuilding*)m_pBldg;
        CBuildUnit const* pBu = pVb->GetBldUnt();
        int per = pVb->GetBuildPer();
        if ( per < 0 ) per = 0;
        if ( pBu ) {
            int vt = pBu->GetVehType();
            std::string nm = ( vt >= 0 && vt < theTransports.GetNumTransports() )
                             ? std::string( theTransports.GetData( vt )->GetDesc().c_str() )
                             : std::string();
            if ( m_lblBuildName )
                m_lblBuildName->SetText( "Building: " + nm + "  (" + std::to_string( per ) + "%)" );
            DrawBuildBar( m_imgBuildBar, per );
        } else {
            if ( m_lblBuildName ) m_lblBuildName->SetText( "Idle - nothing in production" );
            DrawBuildBar( m_imgBuildBar, 0 );
        }
    }

    if ( m_bFertility ) {
        int pct = ( (CFarmBuilding*)m_pBldg )->GetTerMult() * 10;   // 0..10 -> 0..100%
        if ( pct < 0 )   pct = 0;
        if ( pct > 100 ) pct = 100;
        DrawDensityIcons( m_imgFertility, pct );
        if ( m_lblFertility ) m_lblFertility->SetText( std::to_string( pct ) + "%" );
    }

    if ( m_bPower ) {
        int bldg  = PerBldgPower();
        int total = (int)p->GetPwrHave();
        int pct   = ( total > 0 ) ? ( bldg * 100 / total ) : 0;
        m_lblPowerBldg->SetText( "This building: " + FmtNum( bldg ) +
                                 " (" + std::to_string( pct ) + "%)" );
        m_lblPowerColony->SetText( "Colony: " + FmtNum( total ) +
                                   " / " + FmtNum( (int)p->GetPwrNeed() ) );
        DrawGraph( m_imgPowerGraph, kPwrHave, kPwrNeed );
    }

    if ( m_bOffice ) {
        m_lblOfcBldg->SetText( "This building: " + FmtNum( PerBldgOfcCap() ) + " desks" );
        m_lblOfcColony->SetText( "Colony: " + FmtNum( (int)p->GetPplBldg() ) +
                                 " / " + FmtNum( (int)p->m_iOfcCap ) );
        DrawGraph( m_imgOfcGraph, kOfcCap, kPplBldg );
    }

    if ( m_bApt ) {
        m_lblAptBldg->SetText( "This building: " + FmtNum( PerBldgAptCap() ) + " beds" );
        m_lblAptColony->SetText( "Colony: " + FmtNum( (int)p->GetPplTotal() ) +
                                 " / " + FmtNum( (int)p->m_iAptCap ) );
        DrawGraph( m_imgAptGraph, kAptCap, kPplTotal );
    }

    if ( m_bTurret && m_lblTurretRange && m_lblTurretDps ) {
        int range = m_pBldg->GetRange();
        int dmg   = m_pBldg->GetAttack( 0 );
        int rate  = m_pBldg->GetFireRate();
        int dps   = ( rate > 0 ) ? ( dmg * AVG_SPEED_MUL ) / rate : 0;
        m_lblTurretRange->SetText(  "Range: "  + std::to_string( range ) );
        if ( m_lblTurretDmg )    m_lblTurretDmg->SetText(    "Damage: " + FmtNum( dmg ) );
        if ( m_lblTurretReload ) m_lblTurretReload->SetText( "Reload: " + std::to_string( rate ) );
        m_lblTurretDps->SetText(    "DPS: ~"   + FmtNum( dps ) );
    }

    if ( m_bMilitary ) {
        // Recount only every ~30 frames (about once a second) — see ComputeMilitary.
        if ( ( m_iMilTick++ % 30 ) == 0 ) ComputeMilitary();
        if ( m_lblMilStrength ) m_lblMilStrength->SetText( "Strength: " + FmtNum( m_iMilStrength ) );
        if ( m_lblInfantry )    m_lblInfantry->SetText(    "Infantry: " + FmtNum( m_iInfantry ) );
        if ( m_lblVehicles )    m_lblVehicles->SetText(    "Vehicles: " + FmtNum( m_iVehicles ) );
    }

    if ( m_bRepair ) {
        CRepairBuilding* pRep = (CRepairBuilding*)m_pBldg;
        CVehicle* pCur = pRep->GetVehRepairing();

        // Row 0: the unit on the bench + its repair %. The bar tracks that %.
        if ( pCur ) {
            int maxHp = pCur->GetData()->GetDamagePoints();
            int per   = ( maxHp > 0 ) ? ( pCur->GetDamagePoints() * 100 / maxHp ) : 0;
            if ( m_lblRepair[0] )
                m_lblRepair[0]->SetText( "Now: " + std::string( pCur->GetData()->GetDesc().c_str() ) +
                                         "  (" + std::to_string( per ) + "%)" );
            if ( m_progRepair ) { m_progRepair->SetVisible( true ); m_progRepair->SetProgress( per ); }
        } else {
            if ( m_lblRepair[0] ) m_lblRepair[0]->SetText( "Idle - no vehicle being repaired" );
            if ( m_progRepair )   m_progRepair->SetVisible( false );
        }

        // Remaining rows: the waiting queue (last row collapses any overflow).
        int qCount   = pRep->GetRepairQueueCount();
        int waitRows = kRepairRows - 1;   // rows 1..kRepairRows-1
        for ( int i = 0; i < waitRows; i++ ) {
            SDL2Label* lbl = m_lblRepair[i + 1];
            if ( !lbl ) continue;
            if ( i == waitRows - 1 && qCount > waitRows ) {
                lbl->SetText( "+ " + std::to_string( qCount - i ) + " more waiting..." );
            } else if ( i < qCount ) {
                CVehicle* pq = pRep->GetRepairQueueAt( i );
                lbl->SetText( std::to_string( i + 1 ) + ". " +
                              ( pq ? std::string( pq->GetData()->GetDesc().c_str() ) : std::string() ) );
            } else {
                lbl->SetText( "" );
            }
        }
    }
}
