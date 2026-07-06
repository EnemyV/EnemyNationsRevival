#include "stdafx.h"
#include "SDL2BuildingWindow.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "player.h"
#include "building.h"
#include "altoutput.h"
#include "vehicle.h"
#include "base.h"
#include "icons.h"
#include "bitmaps.h"      // theIcons, ICON_MATERIALS
#include "edicts.h"       // Edicts v1: civ-wide edict catalog for the Edicts section
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

// Slot icons display at 3/4 of the native art size — full-size status-bar icons
// crowded the rows and read as clipped (operator: "a bit too big now"). dispIconH
// is THE display height; slotRowH and every Draw* call size from it so layout and
// rendering can't drift apart.
static int dispIconH(int nativeH) { return __max( 8, ( nativeH * 3 ) / 4 ); }

// Icon-stack rows size themselves to the STATUS-BAR art: icon at display size plus
// the full background-bar height. (The fixed 20px ROW_H integer-halved the material
// icons and clipped the bottom off the bar art.) Falls back to ROW_H when the icon
// entry isn't available. theIcons is loaded at app init, well before any window opens.
static int slotRowH(int iconIdx) {
    CStatData* sd = ( iconIdx >= 0 ) ? theIcons.GetByIndex( iconIdx ) : nullptr;
    int h = ROW_H;
    if ( sd ) {
        h = __max( h, dispIconH( sd->m_cyIcon ) + 4 );
        h = __max( h, sd->m_cyBack + 4 );
    }
    return h;
}
static int matRowH()       { return slotRowH( ICON_MATERIALS ); }
static int storageHeight() { return BOX_PAD + HDR_H + SDL2BuildingWindow::kNumStoreMats * matRowH() + BOX_PAD; }
// graph + the tiny time-range button row underneath it
static const int RANGE_ROW_H  = 18;   // height of the tiny 10m/1h/6h/24h/7d button row (fits a ~12pt crisp label)
static const int GRAPHAREA_H  = GRAPH_H + RANGE_ROW_H + 2;
static const int POWERLIKE_H  = BOX_PAD + HDR_H + GRAPHAREA_H + BOX_PAD;   // power / apartment (graph + range row)
// Offices + Workforce are SEPARATE sections now (operator): Offices = desk capacity vs
// office workers, Workforce = colony workforce need/have. 2 rows + graph each; both
// fit inside the graph-area height.
static const int OFFICE_H     = BOX_PAD + HDR_H + __max( 6 + 2 * ROW_H, GRAPHAREA_H ) + BOX_PAD;
static const int WORKFORCE_H  = BOX_PAD + HDR_H + __max( 6 + 2 * ROW_H, GRAPHAREA_H ) + BOX_PAD;
static const int TURRET_H     = BOX_PAD + HDR_H + 2 * ROW_H + 34 + BOX_PAD;   // 2x2 stats + Show-Range
static const int PRODUCTION_H = BOX_PAD + HDR_H + 2 * ROW_H + 6 + 16 + BOX_PAD;   // text + progress bar
static const int MILITARY_H   = BOX_PAD + HDR_H + 4 * ROW_H + BOX_PAD;   // strength + infantry + vehicles + energy-need (#39)
// Fertility row: lumber mills use the density "X" art, food farms the wheat sheaf —
// size to whichever this building shows (same art-aware sizing as the storage rows).
static bool fertIsLumber(CBuilding* b) {
    CBuildFarm* pBf = b ? b->GetData()->GetBldFarm() : nullptr;
    return ( pBf && pBf->GetTypeFarm() == CMaterialTypes::lumber );
}
static int fertRowHFor(CBuilding* b)    { return slotRowH( fertIsLumber( b ) ? ICON_DENSITY : ICON_FOOD ); }
static int fertilityHeight(CBuilding* b){ return BOX_PAD + HDR_H + fertRowHFor( b ) + BOX_PAD; }
static const int UNITS_ROW_H  = 30;                                      // min height for unit icons
static int unitsRowH()   { return __max( UNITS_ROW_H, slotRowH( ICON_VEHICLES ) ); }
static int unitsHeight() { return BOX_PAD + HDR_H + unitsRowH() + BOX_PAD; }   // seaport strip
static const int BUILD_BAR_H  = 26;
static const int BUILDING_H   = BOX_PAD + HDR_H + ROW_H + 4 + BUILD_BAR_H + BOX_PAD;  // name + bar
static const int REPAIR_H     = BOX_PAD + HDR_H + 16 + 6 +
                                SDL2BuildingWindow::kRepairRows * ROW_H + BOX_PAD;   // bar + queue rows

static const int PORTRAIT_SRC = 64;   // tile size in the DIB_LIST_UNIT_BUILDINGS sheet
static const int PORTRAIT     = 72;   // displayed size (88 read too big and clipped; 64 too small)
// Band height DERIVES from the portrait (portrait + 3px gap + 10px condition bar +
// 6px breathing room) so resizing the portrait can never clip it against the first
// section again; floor of 89 keeps room for the name + 3-line flavor + status text.
static const int HEADER_H     = __max( 107, PORTRAIT + 3 + 10 + 6 + 16 );   // +16px: operating-cost line (power/workers) under the flavor

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
static bool secCoalLiqActive(CBuilding* b);   // fwd (defined below; used to gate secPower for C6)
static bool secPower(CBuilding* b) {
    // C6: a coal plant in Coal-Liquefaction mode is shown as a PRODUCER (Production/Inputs/Outputs),
    // not as a Power section — so suppress Power when liq is active. (The rocket always has Power.)
    if ( ( b->GetData()->GetUnionType() == CStructureData::UTpower ) && secCoalLiqActive( b ) )
        return false;
    return ( b->GetData()->GetType() == CStructureData::rocket ) ||
           ( b->GetData()->GetUnionType() == CStructureData::UTpower );
}
// #43: is this building actively in Coal Liquefaction mode? i.e. its AltOutput toggle is ON,
// the def is available (researched), AND it's the coal-liq def (oil output via consuming coal).
// In this mode the plant stops generating power and converts coal->oil, so the Power section
// switches to an oil readout. (Returns false for BioFuel/Charcoal/Fracking — those don't host
// a Power section — and for any plant with the toggle OFF.)
// C6 (operator): the coal-liq window must render the SAME Production/Inputs/Outputs sections as a
// refinery (not the repurposed Power section). Because the section SET differs between liq-on and
// liq-off, computeLayout must be able to size the window for BOTH modes without mutating the real
// game flag. This override lets the layout math force "coal-liq on/off" for height calc only:
//   -1 = read the live alt_oil flag (normal);  0/1 = force off/on (set+restored synchronously).
static int s_coalLiqLayoutOverride = -1;

static bool secCoalLiqActive(CBuilding* b) {
    bool altOn = ( s_coalLiqLayoutOverride >= 0 ) ? ( s_coalLiqLayoutOverride != 0 )
                                                  : b->IsFlag( CUnit::alt_oil );
    if ( !altOn ) return false;
    const AltOutput::AltOutputDef* pDef = AltOutput::Available( b );
    return ( pDef != nullptr ) && ( pDef->m_eMode == AltOutput::eRatioConsume )
           && ( pDef->m_iInputMat == CMaterialTypes::coal )
           && ( pDef->m_iOutputMat == CMaterialTypes::oil );
}
// C6: can this plant do coal-liq AT ALL (regardless of the current toggle)? Such a plant's window
// flips its section SET on toggle, so it gets the max-of-both-modes frame + the relayout-on-toggle.
static bool coalLiqCapable(CBuilding* b) {
    if ( b->GetData()->GetUnionType() != CStructureData::UTpower ) return false;
    const AltOutput::AltOutputDef* pDef = AltOutput::Available( b );
    return ( pDef != nullptr ) && ( pDef->m_eMode == AltOutput::eRatioConsume )
           && ( pDef->m_iInputMat == CMaterialTypes::coal )
           && ( pDef->m_iOutputMat == CMaterialTypes::oil );
}
// Desperate Measures (rocket EDICT) / Scrounging (warehouse AltOutput toggle): is the scrounge ON?
// Its Production section shows only while ON, so the section SET flips on toggle — same as coal-liq.
// The override lets computeLayout size for both on/off modes without touching game state.
static int s_scroungeLayoutOverride = -1;   // -1 = live; 0/1 = force off/on (height calc only)
static bool scroungeActive(CBuilding* b) {
    if ( s_scroungeLayoutOverride >= 0 ) return ( s_scroungeLayoutOverride != 0 );
    if ( b->GetData()->GetType() == CStructureData::rocket )
        return ( b->GetOwner() != nullptr ) && b->GetOwner()->IsEdictActive( EDICT_DESPERATE_MEASURES );
    if ( !b->IsFlag( CUnit::alt_oil ) ) return false;
    const AltOutput::AltOutputDef* pDef = AltOutput::Available( b );
    return ( pDef != nullptr ) && ( pDef->m_eMode == AltOutput::eMultiTrickle );
}
// Can this building scrounge at all (rocket, or a warehouse with the Scrounging def)? Gets the
// max-of-both-modes frame + relayout-on-toggle, like coalLiqCapable.
static bool scroungeCapable(CBuilding* b) {
    if ( b->GetData()->GetType() == CStructureData::rocket ) return true;
    const AltOutput::AltOutputDef* pDef = AltOutput::Available( b );
    return ( pDef != nullptr ) && ( pDef->m_eMode == AltOutput::eMultiTrickle );
}
// #51: the building's PRIMARY output rate, per in-game minute — the exact figure the info
// window's status line shows for a normal producer. Mirrors the per-class ShowStatusText math
// (CFarmBuilding / CMaterialBuilding, new_unit.cpp) so the alt-mode and fallback readouts stay
// in sync with the stock "<resource>: <N>/min" line. Returns -1 when the building has no
// per-minute rate (idle / non-producer) so callers can fall back to a plain label.
static int primaryRatePerMin(CBuilding* b) {
    int ut = b->GetData()->GetUnionType();
    if ( ut == CStructureData::UTfarm ) {
        CBuildFarm* pf = b->GetData()->GetBldFarm();
        if ( !pf || pf->GetTimeToFarm() <= 0 ) return -1;
        float fRate = b->GetOwner()->GetFarmProd() * (float)( (CFarmBuilding*)b )->GetTerMult()
                      * float( 24 * 60 * pf->GetQuantity() ) / float( pf->GetTimeToFarm() );
        return (int)b->GetFrameProd( fRate );
    }
    if ( ut == CStructureData::UTmaterials ) {
        CBuildMaterials const* pm = b->GetData()->GetBldMaterials();
        if ( !pm || pm->GetTime() <= 0 ) return -1;
        // I1 (operator): a converter with an empty required-input store produces NOTHING —
        // show 0/min, not the theoretical max. Mirrors the runtime BuildMaterials gate
        // (mainloop.cpp:2361) and the Inputs-widget "missing" check (GetStore(input)<=0).
        for ( int iIn = 0; iIn < CMaterialTypes::GetNumTypes(); iIn++ )
            if ( pm->GetInput( iIn ) > 0 && b->GetStore( iIn ) <= 0 )
                return 0;
        int iTyp = -1;
        for ( int i = 0; i < CMaterialTypes::GetNumTypes(); i++ )
            if ( pm->GetOutput( i ) > 0 ) { iTyp = i; break; }
        if ( iTyp < 0 ) return -1;
        float fRate = b->GetOwner()->GetMtrlsProd() * float( 24 * 60 * pm->GetOutput( iTyp ) )
                      / float( pm->GetTime() );
        return (int)b->GetFrameProd( fRate );
    }
    return -1;
}

// #51: status line for the Production widget when an AltOutput mode is ACTIVE, so the widget
// shows the ALT resource (charcoal coal / bio-oil / fracking oil) instead of the building's
// primary output (lumber/gas). Emits the same "<resource>: <N> / min" RATE phrasing as the
// stock CMaterialBuilding::ShowStatusText (reuse, not replace) — the operator's blocker was the
// alt mode dropping the rate for a conversion-ratio sentence. The rebranded conversion outputs
// use the def's display label ("Charcoal"/"Bio Oil") per docs/plans/charcoal-sawmill-tech.md;
// Fracking keeps the plain resource name (it just revives an oil well). Coal-liq is excluded
// (its host is a power plant with no Production section — it shows oil in the Power section).
// Caller guarantees pDef != null and the alt flag is ON.
static std::string AltProductionStatus(CBuilding* b, const AltOutput::AltOutputDef* pDef) {
    CPlayer*    p       = b->GetOwner();
    std::string matName = CMaterialTypes::GetDesc( pDef->m_iOutputMat ).c_str();
    if ( pDef->m_eMode == AltOutput::ePctAdditive ) {
        int pct = pDef->m_pfnPct ? pDef->m_pfnPct( p ) : 0;
        return "Producing " + matName + ": " + std::to_string( pct ) + "% of output";
    }
    if ( pDef->m_eMode == AltOutput::eFlatTrickle ) {
        int rate = pDef->m_pfnFlat ? pDef->m_pfnFlat( p ) : 0;
        return "Producing " + matName + ": " + FmtNum( rate ) + " / min";
    }
    if ( pDef->m_eMode == AltOutput::eMultiTrickle ) {
        // Desperate Measures / Scrounging: list each scrounged line, e.g. "+10 lumber +5 iron…/min".
        std::string s = "Producing:";
        for ( int i = 0; i < pDef->m_nMulti; i++ )
            s += " +" + std::to_string( pDef->m_aMulti[i].m_iPerMin ) + " " +
                 std::string( CMaterialTypes::GetDesc( pDef->m_aMulti[i].m_iMat ).c_str() );
        return s + " / min";
    }
    // eRatioConsume / eGlobalConsume: a conversion. Show the OUTPUT RATE (units/min) using the
    // rebranded display label, computed from the building's primary throughput exactly as the
    // production hook (mainloop.cpp) feeds Convert():
    //   Charcoal (lumber mill): a GetCharcoalPct% slice of the lumber harvest -> coal at ratio:1.
    //   Bio Oil  (refinery)   : each production batch -> 1/ratio oil (food burned at ratio:1).
    std::string label = pDef->m_szLabel;   // "Charcoal" / "Bio Oil" (UI label, not the raw mat)
    int rate = -1;
    int ut   = b->GetData()->GetUnionType();
    if ( ut == CStructureData::UTfarm ) {
        int lumber = primaryRatePerMin( b );   // lumber/min the mill harvests (then diverts to kiln)
        if ( lumber > 0 && pDef->m_iRatioIn > 0 )
            rate = ( lumber * p->GetCharcoalPct() ) / ( 100 * pDef->m_iRatioIn );
    } else if ( ut == CStructureData::UTmaterials ) {
        CBuildMaterials const* pm = b->GetData()->GetBldMaterials();
        if ( pm && pm->GetTime() > 0 && pDef->m_iRatioIn > 0 ) {
            int batches = (int)b->GetFrameProd( p->GetMtrlsProd() * float( 24 * 60 )
                                                / float( pm->GetTime() ) );
            rate = batches / pDef->m_iRatioIn;
        }
    }
    if ( rate >= 0 )
        return "Producing " + label + ": " + FmtNum( rate ) + " / min";
    return "Producing " + label;   // producing but rate unavailable — never fall back to the ratio sentence
}
static bool secApt(CBuilding* b) {
    if ( b->GetData()->GetType() == CStructureData::rocket ) return true;
    return ( b->GetData()->GetUnionType() == CStructureData::UThousing ) &&
           ( b->GetData()->GetBldgType()  == CStructureData::apartment );
}
// Edicts v1: how many CIV-WIDE edicts are hosted at this building's type?
// (Building-scoped edicts live in AltOutput, not here.)
static int nCivEdictsFor(CBuilding* b) {
    // Match the edict's host either by GetBldgType() (normalised: apartment/office) OR
    // GetType() (per-instance: rocket/command_center) — the rocket only matches via GetType.
    CStructureData::BLDG_TYPE bt = b->GetData()->GetBldgType();
    CStructureData::BLDG_TYPE gt = b->GetData()->GetType();
    CPlayer* o = b->GetOwner();
    int n = 0;
    for ( int id = 0; id < EDICT_COUNT; ++id ) {
        if ( g_aEdicts[id].scope != EDICT_CIVWIDE ) continue;
        CStructureData::BLDG_TYPE host = g_aEdicts[id].hostBuilding;
        if ( host != bt && host != gt ) continue;
        // Research-gate (#2, §10): edict hidden until its topic is discovered.
        if ( o && !o->GetRsrch( g_aEdicts[id].researchTopic ).m_bDiscovered ) continue;
        ++n;
    }
    return n;
}
// The UNGATED host count (research ignored): how many civ-wide edicts COULD this building
// type ever host. computeLayout/BuildEdicts reserve section height for this maximum so a
// topic discovered while the window is open can add its row live (Refresh → Rebuild)
// without an SDL window resize — the same size-for-the-max trick the coal-liq relayout uses.
static int nCivEdictsHostMax(CBuilding* b) {
    CStructureData::BLDG_TYPE bt = b->GetData()->GetBldgType();
    CStructureData::BLDG_TYPE gt = b->GetData()->GetType();
    int n = 0;
    for ( int id = 0; id < EDICT_COUNT; ++id ) {
        if ( g_aEdicts[id].scope != EDICT_CIVWIDE ) continue;
        CStructureData::BLDG_TYPE host = g_aEdicts[id].hostBuilding;
        if ( host == bt || host == gt ) ++n;
    }
    return n;
}
// Show the Edicts section only on an edict-host building that I own.
static bool secEdicts(CBuilding* b) {
    return ( b->GetOwner() && b->GetOwner()->IsMe() ) && ( nCivEdictsFor(b) > 0 );
}
// Show the building-scoped AltOutput toggle (the "Production Mode" section: BioFuel /
// Coal-Liquefaction / Charcoal / Fracking) only on a building I own that has an available
// (matched + tech-researched) AltOutput def. Mirrors secEdicts so the toggle lives in the
// section layout (height-reserved) instead of overlapping the Close row — bug #40.
static bool secAltOutput(CBuilding* b) {
    return ( b->GetOwner() && b->GetOwner()->IsMe() ) && ( AltOutput::Available( b ) != nullptr );
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
    // C6: a coal-liq-active power plant is a producer (coal -> oil), so it gets the Production
    // section + bar like a refinery instead of the Power section.
    if ( ( ut == CStructureData::UTpower ) && secCoalLiqActive( b ) ) return true;
    // Desperate Measures (rocket edict) / Scrounging (warehouse): Production section shown ONLY
    // while the scrounge is ON (section set flips -> relayout on toggle; sized for max-of-both).
    if ( scroungeCapable( b ) ) return scroungeActive( b );
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
    // C6: a coal-liq-active power plant consumes coal (the def's input mat) -> oil. Its GetInputs()
    // doesn't list a material (power "input" is fuel, tracked on CBuildPower), so name coal directly
    // from the def so the Inputs section shows "Coal" like a refinery's "Oil".
    if ( ( b->GetData()->GetUnionType() == CStructureData::UTpower ) && secCoalLiqActive( b ) ) {
        if ( const AltOutput::AltOutputDef* pDef = AltOutput::Available( b ) ) {
            if ( maxOut > 0 ) { outMats[0] = pDef->m_iInputMat; return 1; }
        }
        return 0;
    }
    int vals[CMaterialTypes::num_types] = {};
    b->GetInputs( vals );
    int n = 0;
    for ( int i = 0; ( i < CMaterialTypes::GetNumTypes() ) && ( n < maxOut ); i++ )
        if ( vals[i] > 0 ) outMats[n++] = i;
    // operator B2/B3/B4 (2026-06-28): do NOT synthesize a material Inputs widget for an alt host
    // that has no real material input. The lumber mill is OUTPUT-ONLY — its "input" is fertility
    // (terrain), and Charcoal mode just SWAPS the output (lumber->coal), it doesn't consume lumber
    // as a material. So a UTfarm charcoal host shows NO Inputs section. (The refinery has a genuine
    // oil/food input -> n>0 already -> its Inputs section is real and the refresh swaps it.)
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
    return BOX_PAD + HDR_H + n * matRowH() + BOX_PAD;
}

// Collect the material types this building produces: a smelter/refinery's GetOutput
// list, or a mine's single mined material. Farms are skipped — their output is food,
// which is a colony-wide resource, not a per-building stockpile.
static int collectOutputMats(CBuilding* b, int* outMats, int maxOut) {
    int n = 0;
    int ut = b->GetData()->GetUnionType();
    // C6: a coal-liq-active power plant outputs oil (the def's output mat) — show it as the Output
    // section like a refinery, so the window reads coal IN / oil OUT.
    if ( ( ut == CStructureData::UTpower ) && secCoalLiqActive( b ) ) {
        if ( const AltOutput::AltOutputDef* pDef = AltOutput::Available( b ) )
            if ( maxOut > 0 ) { outMats[0] = pDef->m_iOutputMat; return 1; }
        return 0;
    }
    if ( ut == CStructureData::UTmaterials ) {
        CBuildMaterials* pm = b->GetData()->GetBldMaterials();
        if ( pm )
            for ( int i = 0; ( i < CMaterialTypes::GetNumTypes() ) && ( n < maxOut ); i++ )
                if ( pm->GetOutput(i) > 0 ) outMats[n++] = i;
    } else if ( ut == CStructureData::UTmine ) {
        CBuildMine* pmn = b->GetData()->GetBldMine();
        if ( pmn ) { int m = pmn->GetTypeMines(); if ( ( m >= 0 ) && ( n < maxOut ) ) outMats[n++] = m; }
    }
    // #51 follow-up (gap 1): a UTfarm's product isn't in the CBuildMaterials GetOutput list above.
    // A LUMBER MILL keeps its lumber in a per-building store (like a smelter's steel), so it must
    // ALWAYS show an Output widget -- lumber in wood mode, coal in charcoal mode (the refresh at
    // RefreshDynamic swaps slot 0 to coal when the toggle is on). Only FOOD farms are skipped: food
    // is a colony-wide resource with no per-building stockpile. (Previously this was gated on
    // AltOutput::Available, so a lumber mill with no Charcoal research showed NO Output section at
    // all -- operator nit: "lumber mill is missing the output widget".)
    if ( n == 0 && ut == CStructureData::UTfarm ) {
        CBuildFarm* pf = b->GetData()->GetBldFarm();
        if ( pf && pf->GetTypeFarm() != CMaterialTypes::food && n < maxOut )
            outMats[n++] = pf->GetTypeFarm();
    }
    return n;
}
static bool secOutputs(CBuilding* b) {
    int tmp[SDL2BuildingWindow::kMaxInputs];
    return ( collectOutputMats(b, tmp, SDL2BuildingWindow::kMaxInputs) > 0 );
}
// #51: primary-output status line for the Production widget when NO AltOutput mode is active.
// CBuilding::ShowStatusText returns EMPTY for several producers on this build, which left the
// Production label blank — and an empty SDL2Label early-returns in Render (SDL2UI.cpp) without
// clearing, so the PRIOR alt glyph lingered on the window surface = the operator's "still shows
// food->oil after toggling Bio-fuel OFF" / "doesn't say wood when Charcoal is off". Naming the
// building's first output material keeps the label non-empty so it always shows the real product
// and never strands a stale glyph. Empty only for non-material producers (caller keeps the
// ShowStatusText text in that case, e.g. the rocket's own status).
static std::string PrimaryProductionStatus(CBuilding* b) {
    int outMats[SDL2BuildingWindow::kMaxInputs];
    int n   = collectOutputMats( b, outMats, SDL2BuildingWindow::kMaxInputs );
    int mat = ( n > 0 ) ? outMats[0] : -1;
    // UTfarm (farms + lumber mills) isn't covered by collectOutputMats (its output isn't a
    // CBuildMaterials list) — its product is CBuildFarm::GetTypeFarm() (food / lumber). This is
    // the charcoal-host case the operator hit: a lumber mill whose status went blank when off.
    if ( mat < 0 && b->GetData()->GetUnionType() == CStructureData::UTfarm ) {
        CBuildFarm* pf = b->GetData()->GetBldFarm();
        if ( pf ) mat = pf->GetTypeFarm();
    }
    if ( mat < 0 ) return std::string();
    // #51: prefer the per-minute RATE (the figure ShowStatusText shows) so the fallback never
    // strands a rateless "Producing gas" — that was the operator's regression. Only drop to the
    // bare name when the building genuinely has no production rate (e.g. an idle/exhausted mine).
    int rate = primaryRatePerMin( b );
    if ( rate >= 0 )
        return std::string( "Producing " ) + CMaterialTypes::GetDesc( mat ).c_str() + ": " + FmtNum( rate ) + " / min";
    return std::string( "Producing " ) + CMaterialTypes::GetDesc( mat ).c_str();
}

// #51 follow-up: amount of a material to show in the Inputs/Outputs sections. Global resources
// (gas, food) aren't stocked per-building (GetStore == 0) so show the colony total, like the
// existing gas readout; everything else is the building's own stock.
static int matAmount(CBuilding* b, int mat) {
    if ( mat == CMaterialTypes::gas )  return (int)b->GetOwner()->GetGasHave();
    if ( mat == CMaterialTypes::food ) return (int)b->GetOwner()->GetFood();
    return b->GetStore( mat );
}
static int outputsHeight(CBuilding* b) {
    int tmp[SDL2BuildingWindow::kMaxInputs];
    int n = collectOutputMats(b, tmp, SDL2BuildingWindow::kMaxInputs);
    if ( n <= 0 ) return 0;
    return BOX_PAD + HDR_H + n * matRowH() + BOX_PAD;
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
    SEC_WORKFORCE, SEC_APT, SEC_TURRET, SEC_EDICTS, SEC_ALTOUTPUT
};

// AltOutput "Production Mode" section: one outlined box with a checkbox row (+ scope (i)
// icon) plus a 3-row mode-aware OUTPUT readout (#43-audit item 2) below it, sized so the
// readout (shown when the toggle is ON) never overflows the box.
static const int ALTOUTPUT_H = BOX_PAD + HDR_H + ROW_H + BOX_PAD;

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
    if ( secStorage(b)    ) L.secs[n++] = { SEC_STORAGE,    storageHeight() };
    if ( secProduction(b) ) L.secs[n++] = { SEC_PRODUCTION, PRODUCTION_H };
    if ( secBuilding(b)   ) L.secs[n++] = { SEC_BUILDING,   BUILDING_H };
    if ( secFertility(b)  ) L.secs[n++] = { SEC_FERTILITY,  fertilityHeight(b) };
    if ( secInputs(b)     ) L.secs[n++] = { SEC_INPUTS,     inputsHeight(b) };
    if ( secOutputs(b)    ) L.secs[n++] = { SEC_OUTPUTS,    outputsHeight(b) };
    if ( secUnits(b)      ) L.secs[n++] = { SEC_UNITS,      unitsHeight() };
    if ( secRepair(b)     ) L.secs[n++] = { SEC_REPAIR,     REPAIR_H };
    if ( secMilitary(b)   ) L.secs[n++] = { SEC_MILITARY,   MILITARY_H };
    if ( secPower(b)      ) L.secs[n++] = { SEC_POWER,      POWERLIKE_H };
    if ( secOfc(b)        ) L.secs[n++] = { SEC_OFFICE,     OFFICE_H };
    // Workforce rides with the office sections (offices + the rocket's offices):
    // capacity/workers and needed/have are two different colony readings (operator).
    if ( secOfc(b)        ) L.secs[n++] = { SEC_WORKFORCE,  WORKFORCE_H };
    if ( secApt(b)        ) L.secs[n++] = { SEC_APT,        POWERLIKE_H };
    if ( secTurret(b)     ) L.secs[n++] = { SEC_TURRET,     TURRET_H };
    // Height reserved for the UNGATED host maximum (not just the discovered rows) so a
    // research discovery while the window is open can add its row via Rebuild in place.
    if ( secEdicts(b)     ) L.secs[n++] = { SEC_EDICTS,     BOX_PAD + HDR_H + nCivEdictsHostMax(b) * ROW_H + BOX_PAD };
    if ( secAltOutput(b)  ) L.secs[n++] = { SEC_ALTOUTPUT,  ALTOUTPUT_H };

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

// C6: a coal-liq-capable plant has DIFFERENT section sets (hence sizes) in liq-on vs liq-off mode.
// Size the window for the MAX of both so the on-toggle relayout never needs an SDL window resize —
// the smaller mode just leaves slack at the bottom. Computed via the layout override so the real
// alt_oil flag is never touched. (Restored even though callers always pass -1, for safety.)
static void coalLiqMaxDims(CBuilding* b, int& wMax, int& hMax) {
    int save = s_coalLiqLayoutOverride;
    s_coalLiqLayoutOverride = 1; BldgLayout on  = computeLayout(b);
    s_coalLiqLayoutOverride = 0; BldgLayout off = computeLayout(b);
    s_coalLiqLayoutOverride = save;
    wMax = __max(on.width,  off.width);
    hMax = __max(on.height, off.height);
}
// Scrounge hosts flip their section SET on toggle too (Production appears/disappears); size for
// the max of both modes so the relayout never needs an SDL window resize (mirrors coalLiqMaxDims).
static void scroungeMaxDims(CBuilding* b, int& wMax, int& hMax) {
    int save = s_scroungeLayoutOverride;
    s_scroungeLayoutOverride = 1; BldgLayout on  = computeLayout(b);
    s_scroungeLayoutOverride = 0; BldgLayout off = computeLayout(b);
    s_scroungeLayoutOverride = save;
    wMax = __max(on.width,  off.width);
    hMax = __max(on.height, off.height);
}
static int computeWidth(CBuilding* b) {
    if ( coalLiqCapable(b) )  { int w, h; coalLiqMaxDims(b, w, h);  return w; }
    if ( scroungeCapable(b) ) { int w, h; scroungeMaxDims(b, w, h); return w; }
    return computeLayout(b).width;
}
static int computeHeight(CBuilding* b) {
    if ( coalLiqCapable(b) )  { int w, h; coalLiqMaxDims(b, w, h);  return h; }
    if ( scroungeCapable(b) ) { int w, h; scroungeMaxDims(b, w, h); return h; }
    return computeLayout(b).height;
}

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

// 50/50 blend a color rect into an OPAQUE ARGB surface (SDL_FillRect can't blend,
// and the graph surfaces are opaque). Used for the legend backdrop plates so the
// series lines stay faintly visible through them.
static void fillRect50(SDL_Surface* s, SDL_Rect rc, Uint8 pr, Uint8 pg, Uint8 pb) {
    if ( !s || s->format->BytesPerPixel != 4 ) return;
    int x0 = __max( 0, (int)rc.x ),              y0 = __max( 0, (int)rc.y );
    int x1 = __min( s->w, (int)rc.x + rc.w ),    y1 = __min( s->h, (int)rc.y + rc.h );
    if ( x0 >= x1 || y0 >= y1 ) return;
    if ( SDL_MUSTLOCK( s ) && SDL_LockSurface( s ) != 0 ) return;
    Uint32 halfPlate = ( (Uint32)( pr >> 1 ) << 16 ) | ( (Uint32)( pg >> 1 ) << 8 ) | ( pb >> 1 );
    for ( int y = y0; y < y1; y++ ) {
        Uint32* px = (Uint32*)( (Uint8*)s->pixels + y * s->pitch ) + x0;
        for ( int x = x0; x < x1; x++, px++ )
            *px = 0xFF000000u | ( ( ( *px >> 1 ) & 0x007F7F7Fu ) + halfPlate );
    }
    if ( SDL_MUSTLOCK( s ) ) SDL_UnlockSurface( s );
}

// "Slot" track behind icon stacks / strips (storage rows, fertility, docked
// units): black box with a gold 1px border — the same look the original status
// bar art uses for its gauge boxes (and DrawHealthBar's track), so the window's
// gauges match the game's established chrome.
static void drawSlot(SDL_Surface* s, int x, int y, int w, int h) {
    if ( !s || w <= 2 || h <= 2 ) return;
    SDL_Rect fill = { x, y, w, h };
    SDL_FillRect(s, &fill, SDL_MapRGBA(s->format, 24, 22, 18, 255));
    Uint32 gold = SDL_MapRGBA(s->format, 150, 128, 78, 255);
    SDL_Rect t = { x, y, w, 1 },         l = { x, y, 1, h };
    SDL_Rect b = { x, y + h - 1, w, 1 }, r = { x + w - 1, y, 1, h };
    SDL_FillRect(s, &t, gold); SDL_FillRect(s, &l, gold);
    SDL_FillRect(s, &b, gold); SDL_FillRect(s, &r, gold);
}

// Fit a sprite under maxH keeping aspect; never upscales (pixel art reads best
// at <=1x). Proportional rather than integer-divisor scaling: the icons now
// display at the FIXED 3/4-native dispIconH, so the scale ratio is a constant
// clean 4->3 pixel mapping — the old ceil-divisor rule would have jumped
// straight to 1/2 size ("a bit smaller", not half).
static void intFitIcon(int srcW, int srcH, int maxH, int& outW, int& outH) {
    outW = srcW; outH = srcH;
    if ( srcH <= 0 || srcW <= 0 || srcH <= maxH ) return;
    outH = __max( 1, maxH );
    outW = __max( 1, ( srcW * maxH + srcH / 2 ) / srcH );
}

// ============================================================================
SDL2BuildingWindow::SDL2BuildingWindow(GameWindow* gw, CBuilding* pBldg, bool bOnTop)
    : SDL2Dialog(gw, makeTitle(pBldg), computeWidth(pBldg), computeHeight(pBldg))
    , m_pBldg(pBldg)
{
    // Tuckable behind the map by default (like Relations); but when launched from a
    // build dialog's (I) button it must float on top of that dialog. Note 26 (operator,
    // root-caused by mac2): ALSO float when this window shows interactable EDICTS —
    // otherwise the edict (i)-icon tooltip is occluded by the ALWAYS_ON_TOP area-map
    // window and the effect text is unreadable ("renders behind the area map, only +").
    // secEdicts() is true exactly when an edict row (+ its (i) tooltip) is present, so
    // this targets edict-hosting windows only; non-edict building windows stay
    // tuckable-by-design (no blanket keep-on-top).
    SetKeepOnTop(bOnTop || secEdicts(pBldg));
    SetWidgetFontSize(15); // slightly larger than the 13pt default for readability

    m_bldgID      = pBldg->GetID();
    RecomputeSections();
}

// C6: (re)derive which sections this building shows from its CURRENT state. Called at construction
// and again on a coal-liq relayout (the toggle changes the section SET: Power <-> Production/Inputs/
// Outputs). Keep in lock-step with computeLayout()'s predicate list.
void SDL2BuildingWindow::RecomputeSections() {
    m_bStorage    = secStorage(m_pBldg);
    m_bProduction = secProduction(m_pBldg);
    m_nInputMats  = collectInputMats(m_pBldg, m_inputMats, kMaxInputs);
    m_bInputs     = secInputs(m_pBldg);
    m_nOutputMats = collectOutputMats(m_pBldg, m_outputMats, kMaxInputs);
    m_bOutputs    = ( m_nOutputMats > 0 );
    m_bFertility  = secFertility(m_pBldg);
    m_bUnits      = secUnits(m_pBldg);
    m_bBuilding   = secBuilding(m_pBldg);
    m_bRepair     = secRepair(m_pBldg);
    m_bMilitary   = secMilitary(m_pBldg);
    m_bPower      = secPower(m_pBldg);
    m_bOffice     = secOfc(m_pBldg);
    m_bWorkforce  = secOfc(m_pBldg);   // workforce section rides with the office sections
    m_bApt        = secApt(m_pBldg);
    m_bTurret     = secTurret(m_pBldg);
}

// C6: after ClearWidgets() frees every widget, null ALL cached raw widget pointers so a stale one
// can never be dereferenced before BuildBody re-creates it. Belt-and-suspenders on top of Refresh's
// per-section flag gating. Keep complete — one missed pointer is a dangling deref on relayout.
void SDL2BuildingWindow::NullSectionWidgets() {
    m_imgFertility = nullptr; m_lblFertility = nullptr;
    m_imgStorage = nullptr;
    for ( int i = 0; i < kNumStoreMats; i++ ) { m_lblStoreName[i] = nullptr; m_lblStoreCount[i] = nullptr; }
    m_lblPowerHdr = nullptr; m_imgPowerHdrIcon = nullptr; m_lblPowerBldg = nullptr;
    m_lblPowerColony = nullptr; m_imgPowerGraph = nullptr;
    m_lblPowerOilHdr = nullptr; m_lblPowerOil = nullptr; m_lblPowerOilCol = nullptr;
    m_progPowerOil = nullptr; m_lblPowerFuel = nullptr;
    m_lblOfcBldg = nullptr; m_lblOfcColony = nullptr; m_imgOfcGraph = nullptr;
    m_lblWfHave = nullptr; m_lblWfNeed = nullptr; m_imgWfGraph = nullptr;
    m_lblAptBldg = nullptr; m_lblAptColony = nullptr; m_lblAptNeed = nullptr; m_imgAptGraph = nullptr;
    m_rangeBtns.clear();   // buttons are owned by the widget list (cleared on rebuild); drop stale ptrs
    m_lblTurretRange = nullptr; m_lblTurretDmg = nullptr; m_lblTurretReload = nullptr;
    m_lblTurretDps = nullptr; m_btnShowRange = nullptr;
    m_chkAltOut = nullptr;
    for ( int i = 0; i < kMaxEdictRows; i++ ) { m_chkEdict[i] = nullptr; m_edictIds[i] = 0; }
    m_nEdictRows = 0;
    m_lblProduction = nullptr; m_progProduction = nullptr;
    m_lblMilStrength = nullptr; m_lblInfantry = nullptr; m_lblVehicles = nullptr; m_lblMilEnergy = nullptr;
    for ( int i = 0; i < kRepairRows; i++ ) m_lblRepair[i] = nullptr;
    m_progRepair = nullptr;
    m_imgInputs = nullptr; m_imgInputHdrIcon = nullptr; m_imgOutputHdrIcon = nullptr;
    for ( int i = 0; i < kMaxInputs; i++ ) {
        m_lblInputName[i] = nullptr;  m_lblInputCount[i] = nullptr;
        m_lblOutputName[i] = nullptr; m_lblOutputCount[i] = nullptr;
    }
    m_imgOutputs = nullptr;
    m_imgUnits = nullptr; m_lblUnits = nullptr;
    m_imgBuildBar = nullptr; m_lblBuildName = nullptr;
    m_lblStatus = nullptr; m_lblOperCost = nullptr; m_imgHealth = nullptr;
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
        // A FOOD farm's fertility maps to how much food it can grow, so the wheat
        // sheaf (ICON_FOOD) reads naturally. A LUMBER MILL's "fertility" is really
        // tree density, so the food sheaf is misleading there — use the green
        // density "X" (ICON_DENSITY), the same art the original status bar used.
        auto* pBf = m_pBldg->GetData()->GetBldFarm();
        bool bLumber = ( pBf && pBf->GetTypeFarm() == CMaterialTypes::lumber );
        m_densIconIdx = bLumber ? ICON_DENSITY : ICON_FOOD;
        CStatData* pIco = theIcons.GetByIndex( m_densIconIdx );
        if ( pIco && pIco->m_pcDib ) {
            m_densIcon  = SDL2MainMenu::CreateSurfaceFromDIB( pIco->m_pcDib );
            m_densIconW = pIco->m_cxIcon;
            m_densIconH = pIco->m_cyIcon;
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
    BuildBody();
    Refresh();
}

// Build the header band + every active section + the Close button into the (already-sized) window.
// Factored out of OnInit so Rebuild() can re-run it after a coal-liq relayout. Uses the LIVE layout
// (override = -1), i.e. the sections for the building's current mode; the window FRAME was sized for
// the max of both coal-liq modes (computeWidth/Height) so a smaller mode just leaves slack at the
// bottom and the toggle never needs an SDL window resize.
void SDL2BuildingWindow::BuildBody() {
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

    // NB the building-scoped AltOutput toggle (BioFuel / Coal-Liquefaction / Charcoal /
    // Fracking) is NOT placed here anymore. It used to be a 150px-wide SDL2Button squeezed
    // into this Close row at (m_x+14) — which overlapped the centred Close button at narrow
    // (single-column) widths, so clicks hit Close (and there was no checkbox affordance or
    // scope shown). It is now a proper SDL2Checkbox in the SEC_ALTOUTPUT "Production Mode"
    // section (BuildAltOutput), height-reserved by computeLayout so it can never collide
    // with Close. See bug #40.
}

// C6: rebuild the window body in place after the coal-liq toggle changed the section SET
// (Power <-> Production/Inputs/Outputs). MUST be called from OnFrame (NOT the checkbox callback) —
// ClearWidgets frees the checkbox whose lambda would still be on the stack. Every cached raw widget
// pointer is freed by ClearWidgets, so null them all first; RecomputeSections + BuildBody re-create
// the ones the new mode needs, and Refresh (flag-gated) only touches those. The window FRAME size is
// unchanged (sized for the max of both modes), so no SDL resize.
void SDL2BuildingWindow::Rebuild() {
    ClearWidgets();
    NullSectionWidgets();
    RecomputeSections();
    BuildBody();
    Refresh();
}

// #40: building-scoped AltOutput "Production Mode" toggle. Renders like a civ-wide edict
// (BuildEdicts) — an SDL2Checkbox reflecting the building's current alt_oil state plus an
// (i) icon whose tooltip leads with the SCOPE ("This building only") — but lives in its own
// outlined section so it never overlaps the Close button. Toggling flips CUnit::alt_oil,
// the exact flag the shared AltOutput::Convert production hook reads (mainloop.cpp), so the
// effect (BioFuel oil / Coal-Liq oil / Charcoal coal / Fracking oil) applies immediately.
int SDL2BuildingWindow::BuildAltOutput(int x, int y, int w) {
    const AltOutput::AltOutputDef* pDef = AltOutput::Available( m_pBldg );
    if ( !pDef )                       // detector already gated this, but stay defensive
        return y;

    int H  = ALTOUTPUT_H;
    AddOutline( x, y, w, H );
    int yh = Header( x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Production Mode", kAccentGold );

    // Reserve the row's right edge for the (i) scope icon so the checkbox label never runs
    // under it — identical spacing to the Edicts rows.
    const int kInfoSz = 14;
    int cbX = x + BOX_PAD + 4;
    int cbW = w - 2 * BOX_PAD - 8 - ( kInfoSz + 4 );

    bool checked = m_pBldg->IsFlag( CUnit::alt_oil );
    CBuilding* pBldg = m_pBldg;        // capture by value for the callback
    // C6: a coal-liq plant's toggle changes the section SET (Power <-> Production/Inputs/Outputs),
    // so it needs a window relayout — but NOT here (clearing widgets mid-callback frees this very
    // checkbox). Just request it; OnFrame does the rebuild next frame. Other alt hosts (charcoal/
    // bio-oil/fracking) keep the same sections, so they only need the existing live-Refresh swap.
    bool bRelayout = coalLiqCapable( m_pBldg ) || scroungeCapable( m_pBldg );  // Scrounging: Production appears on toggle
    m_chkAltOut = AddWidget<SDL2Checkbox>(
        cbX, yh, cbW, ROW_H, pDef->m_szLabel, checked,
        [this, pBldg, bRelayout]( bool on ) {
            // Flip the runtime-only alt_oil flag; the production hooks (BioFuel/Coal-Liq/
            // Charcoal/Fracking, all in mainloop.cpp) read it via IsFlag(alt_oil).
            if ( on ) pBldg->SetFlag( CUnit::alt_oil );
            else      pBldg->ClrFlag( CUnit::alt_oil );
            if ( bRelayout ) m_bNeedRelayout = true;
        } );

    // (i) icon — tooltip leads with the building-only scope (#36), then names the effect, then
    // describes what the toggle actually does (the desc fixes the previously-empty (i) — #43 audit).
    std::string tip = "This building only";
    if ( pDef->m_szLabel && pDef->m_szLabel[0] ) { tip += "\n"; tip += pDef->m_szLabel; }
    if ( pDef->m_szDesc  && pDef->m_szDesc[0]  ) { tip += "\n"; tip += pDef->m_szDesc; }
    AddWidget<SDL2InfoIcon>( cbX + cbW + 4, yh + ( ROW_H - kInfoSz ) / 2,
                             kInfoSz, kInfoSz, tip );

    // Mode-aware OUTPUT readout (#43-audit item 2). The coal-liq host shows its conversion in
    // the Power section; the BioFuel / Charcoal / Fracking hosts have no Power section, so mirror
    // that readout here: a status row + this-building store + colony have/made for the def's

    return y + H + SEC_PAD;
}

// Dispatch a section id to its builder. Order of ids matches computeLayout().
// Edicts v1 — civ-wide policy toggles hosted at this building (rocket/command-center/embassy).
// One SDL2Checkbox per edict; toggling calls CPlayer::ToggleEdictNet, which applies the
// bonus+upkeep locally (RecomputeEdictMults) and, in a net game, broadcasts CNetEdictToggle
// so every client converges deterministically.
int SDL2BuildingWindow::BuildEdicts(int x, int y, int w) {
    // Box + returned height use the UNGATED host maximum (matches computeLayout's
    // reservation) so a row a later research discovery adds always fits in place.
    int H = BOX_PAD + HDR_H + nCivEdictsHostMax(m_pBldg) * ROW_H + BOX_PAD;
    AddOutline(x, y, w, H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Edicts", kAccentGold);
    CStructureData::BLDG_TYPE bt = m_pBldg->GetData()->GetBldgType();
    CStructureData::BLDG_TYPE gt = m_pBldg->GetData()->GetType();
    CPlayer* me = m_pBldg->GetOwner();
    int cy = yh;
    m_nEdictRows = 0;
    for ( int id = 0; id < EDICT_COUNT; ++id ) {
        const EdictDef& e = g_aEdicts[id];
        if ( e.scope != EDICT_CIVWIDE || ( e.hostBuilding != bt && e.hostBuilding != gt ) )
            continue;
        // Research-gate (#2, §10): hide the edict until its topic is discovered (matches nCivEdictsFor).
        if ( me && !me->GetRsrch( e.researchTopic ).m_bDiscovered )
            continue;
        bool checked = me->IsEdictActive(id);
        int  eid     = id;   // capture by value for the callback
        // Reserve the row's right edge for an (i) info icon (bug #3) so the
        // checkbox label never runs under it.
        const int kInfoSz = 14;
        int cbX = x + BOX_PAD + 4;
        int cbW = w - 2 * BOX_PAD - 8 - ( kInfoSz + 4 );
        SDL2Checkbox* chk = AddWidget<SDL2Checkbox>( cbX, cy, cbW, ROW_H,
                                 e.name, checked,
                                 [this, me, eid]( bool on ){
                                     me->ToggleEdictNet( eid, on );
                                     // Desperate Measures adds/removes the rocket's Production section;
                                     // defer the relayout to OnFrame (don't free this checkbox mid-callback).
                                     if ( eid == EDICT_DESPERATE_MEASURES ) m_bNeedRelayout = true;
                                 } );
        // Track the row so Refresh() can re-sync the checkbox from the player bitmask
        // (external toggles: harness setedict, last-host auto-revoke §29).
        if ( m_nEdictRows < kMaxEdictRows ) {
            m_chkEdict[m_nEdictRows]  = chk;
            m_edictIds[m_nEdictRows] = id;
            m_nEdictRows++;
        }
        // (i) info icon — hover reveals the edict's scope (#36) then its effect
        // text (EdictDef::desc), one per line.
        std::string tip = ( e.scope == EDICT_CIVWIDE ) ? "Civilization-wide"
                                                        : "This building only";
        if ( e.desc && e.desc[0] ) { tip += "\n"; tip += e.desc; }
        AddWidget<SDL2InfoIcon>( cbX + cbW + 4, cy + ( ROW_H - kInfoSz ) / 2,
                                 kInfoSz, kInfoSz, tip );
        cy += ROW_H;
    }
    return y + H + SEC_PAD;
}

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
        case SEC_WORKFORCE:  return BuildWorkforce (x, y, w);
        case SEC_APT:        return BuildApt       (x, y, w);
        case SEC_TURRET:     return BuildTurret    (x, y, w);
        case SEC_EDICTS:     return BuildEdicts    (x, y, w);
        case SEC_ALTOUTPUT:  return BuildAltOutput (x, y, w);
    }
    return y;
}

void SDL2BuildingWindow::OnFrame() {
    // LOAD-while-in-game crash #3 (mac2 mac_regress repro): skip the per-frame refresh while
    // CGame::LoadGame is tearing down/rebuilding the world — LoadGame BaseYield()s (renders this
    // OnFrame) BEFORE NewWorld's CloseActiveDialogs runs, so Refresh()->ShowStatusText->
    // CPlayer::GetPplTotal (and the C6 Rebuild) deref m_pBldg/the player on a freed/half-loaded
    // world -> SIGSEGV. m_bWorldTearingDown is set for all of LoadGame (events gated in
    // SDL2Compositor::RouteEventInner; OnFrame gated here).
    if ( theApp.IsWorldTearingDown( ) ) return;
    // C6: a pending coal-liq relayout (the checkbox flipped the section SET) is performed HERE,
    // not in the checkbox callback — Rebuild() clears widgets, which would free the live checkbox
    // mid-callback. Doing it at frame top means the callback has fully returned. Rebuild() re-runs
    // Refresh itself, so reset the throttle and return for this frame.
    if ( m_bNeedRelayout ) {
        m_bNeedRelayout = false;
        Rebuild();
        m_nextRefreshMs = SDL_GetTicks64() + 150;
        return;
    }
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
    // Translucent dark interior so each section reads as a card on the parchment
    // (was fully transparent — the window looked like one continuous field).
    SDL_FillRect(s, nullptr, SDL_MapRGBA(s->format, 20, 14, 6, 18));
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
                               int iconIdx, int iconFrame, SDL2Label** ppLabelOut,
                               SDL2Image** ppIconOut) {
    int textX = x;

    // Category glyph: blit ONE frame of the icon strip (status sprites are multi-
    // frame; iconFrame picks which) into its own surface, scaled up to fill the
    // header height so it reads clearly next to the bold text.
    CStatData* pSd = ( iconIdx >= 0 ) ? theIcons.GetByIndex( iconIdx ) : nullptr;
    if ( pSd && pSd->m_cxIcon > 0 && pSd->m_cyIcon > 0 ) {
        int fw = pSd->m_cxIcon, fh = pSd->m_cyIcon;
        // Some status icons (ICON_DAMAGE, ICON_CONSTRUCTION) are WIDE bar sprites;
        // scaled by height alone they'd stretch into a long bar that overruns the
        // header. Crop the frame to a near-square (left portion) so every glyph is
        // a compact chip regardless of the source aspect.
        int cropW = ( fw > fh ) ? fh : fw;
        int gh = HDR_H - 2;
        int gw = ( fh > 0 ) ? ( cropW * gh / fh ) : cropW;
        auto* img = AddWidget<SDL2Image>( x, y + 1, gw, gh );
        SetHdrIcon( img, iconIdx, iconFrame );   // #51 C1: blit the glyph (factored so Refresh can re-blit live)
        if ( ppIconOut ) *ppIconOut = img;       // hand the caller the icon for a live swap
        textX = x + gw + 6;
    }

    auto* h = AddWidget<SDL2Label>( textX, y, w - ( textX - x ), HDR_H, text );
    h->SetColor( color );
    h->SetBold( true );
    if ( ppLabelOut ) *ppLabelOut = h;   // let the caller live-update the header text (#6 Oil<->Power)
    return y + HDR_H;
}

// #51 C1: (re)build a header icon's glyph surface and set it on `img`. Factored out of Header()
// so Refresh can swap a header icon LIVE — the coal-liq plant's power-bulb<->oil (C1) AND the
// Inputs/Outputs material chip when an AltOutput mode flips (operator B5/J4). No-op on null img/icon.
void SDL2BuildingWindow::SetHdrIcon(SDL2Image* img, int iconIdx, int iconFrame) {
    if ( !img ) return;
    SDL_Surface* ico = ( iconIdx >= 0 ) ? HdrIcon( iconIdx ) : nullptr;
    CStatData*   pSd = ( iconIdx >= 0 ) ? theIcons.GetByIndex( iconIdx ) : nullptr;
    if ( !ico || !pSd || pSd->m_cxIcon <= 0 || pSd->m_cyIcon <= 0 ) return;
    int fw = pSd->m_cxIcon, fh = pSd->m_cyIcon;
    int cropW = ( fw > fh ) ? fh : fw;
    SDL_Surface* s = SDL_CreateRGBSurface( 0, cropW, fh, 32,
                                           0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 );
    if ( !s ) return;
    SDL_FillRect( s, nullptr, SDL_MapRGBA( s->format, 0, 0, 0, 0 ) );
    SDL_SetSurfaceBlendMode( ico, SDL_BLENDMODE_BLEND );
    int srcX = iconFrame * fw;
    if ( srcX < 0 || srcX + cropW > ico->w ) srcX = 0;     // frame out of range -> first frame
    SDL_Rect sr = { srcX, 0, cropW, fh };
    SDL_BlitSurface( ico, &sr, s, nullptr );
    SDL_SetSurfaceBlendMode( s, SDL_BLENDMODE_BLEND );
    img->SetSurface( s, true );
}

// The band under the title bar: the building's portrait on the left, its name (bold)
// and one-line flavor text to the right, and a live colored status line beneath them.
int SDL2BuildingWindow::BuildHeaderBand(int x, int y, int w) {
    // Portrait from the building-list sprite sheet (row per building type, the icon
    // region starts at srcX=20, each tile 64px tall — same as the unit-list panel).
    // Crop to the SQUARE 64x64 tile: the sheet row is wider than the tile, and
    // SDL2Image aspect-fits the whole surface into the widget rect — blitting the
    // full remaining row width shrank the visible portrait well below 64px (the
    // "building icons too small" report). Square crop = the tile fills the widget.
    if ( m_bldgSheet && m_pBldg ) {
        int type = m_pBldg->GetData()->GetType();
        int srcY = PORTRAIT_SRC * type;
        if ( srcY + PORTRAIT_SRC <= m_bldgSheet->h ) {
            int srcX = 20;
            int srcW = __min( PORTRAIT_SRC, m_bldgSheet->w - srcX );
            if ( srcW > 0 ) {
                auto* img = AddWidget<SDL2Image>( x, y, PORTRAIT, PORTRAIT );
                SDL_Surface* s = SDL_CreateRGBSurface( 0, srcW, PORTRAIT_SRC, 32,
                                                       0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 );
                if ( s ) {
                    SDL_FillRect( s, nullptr, SDL_MapRGBA( s->format, 0, 0, 0, 0 ) );
                    SDL_SetSurfaceBlendMode( m_bldgSheet, SDL_BLENDMODE_BLEND );
                    SDL_Rect sr = { srcX, srcY, srcW, PORTRAIT_SRC };
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

    // Operating cost (operator): the building's power + worker draw, in the description area.
    // STATE-AWARE — filled/updated by RefreshDynamic so it shows the LIVE draw: a fracked
    // exhausted well reads 2x its base (so the +100% is visible), an idle exhausted well
    // reads 0, a stopped well half, a running well its base. (Text set in RefreshDynamic.)
    m_lblOperCost = AddWidget<SDL2Label>( tx, y + 63, tw, 15, "" );
    m_lblOperCost->SetFontSize( 12 );
    m_lblOperCost->SetBold( true );
    m_lblOperCost->SetColor( { 90, 66, 30, 255 } );   // parchment ink, a touch bolder than the flavor

    m_lblStatus = AddWidget<SDL2Label>( tx, y + HEADER_H - 18, tw, 16, "" );
    m_lblStatus->SetBold( true );

    return y + HEADER_H;
}

// ----------------------------------------------------------------------------
// Section builders (each draws its outline first, then its content)
// ----------------------------------------------------------------------------
int SDL2BuildingWindow::BuildStorage(int x, int y, int w) {
    int rowH = matRowH();
    int H    = storageHeight();
    AddOutline(x, y, w, H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Storage", kAccentGold,
                    ICON_MATERIALS, CMaterialTypes::steel);

    // each row: name (left) | icon stack (middle) | amount (right)
    // countW fits a fully comma-grouped 7-digit value ("9,999,999") at the 15pt
    // widget font so big stockpiles aren't clipped; the flexible icon strip yields
    // the space (it already truncates to whole icons, never a half icon).
    int nameW  = 64;
    int countW = 72;
    int iconsX = x + BOX_PAD + nameW + 4;
    int iconsW = ( x + w - BOX_PAD - countW - 4 ) - iconsX;

    m_imgStorage = AddWidget<SDL2Image>(iconsX, yh, iconsW, kNumStoreMats * rowH);
    for (int i = 0; i < kNumStoreMats; i++) {
        int ry = yh + i * rowH;
        m_lblStoreName[i]  = AddWidget<SDL2Label>(x + BOX_PAD, ry, nameW, rowH, kStoreNames[i]);
        m_lblStoreCount[i] = AddWidget<SDL2Label>(x + w - BOX_PAD - countW, ry, countW, rowH, "0");
        m_lblStoreCount[i]->SetRightAligned(true);
    }
    return y + H + SEC_PAD;
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
    // #43/#6: header reflects the live mode — "Oil" when this coal plant is liquefying coal,
    // else "Power". Captured into m_lblPowerHdr so Refresh() swaps the header text in lockstep
    // with the body rows when coal-liq is toggled while the window is open (was build-time-only).
    bool bOil = secCoalLiqActive( m_pBldg );
    // operator C1/C3 (2026-06-28): in oil (coal-liq) mode show the OIL material icon, NOT the power
    // bulb — the plant produces oil, not power. (Header icon is build-time = correct when the window
    // opens with coal-liq already ON, the operator's case.)
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD,
                    bOil ? "Oil" : "Power", kHeaderBlue,
                    bOil ? ICON_MATERIALS : ICON_POWER, bOil ? CMaterialTypes::oil : 0,
                    &m_lblPowerHdr, &m_imgPowerHdrIcon);
    int graphW = 168, graphX = x + w - graphW - BOX_PAD;
    int textW  = graphX - ( x + BOX_PAD + 4 ) - 6;
    // Power readout (shown when NOT in coal-liq mode): "This building / Colony" + graph.
    m_lblPowerBldg   = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6,         textW, ROW_H, "This building: 0");
    m_lblPowerColony = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6 + ROW_H, textW, ROW_H, "Colony: 0 / 0");
    m_imgPowerGraph  = AddWidget<SDL2Image>(graphX, yh, graphW, GRAPH_H);
    AddGraphRangeRow(graphX, yh + GRAPH_H + 2, graphW);
    // Oil readout (shown INSTEAD when in coal-liq mode): a status row + this-building / colony
    // oil totals, mirroring the power rows' two-line "This building / Colony" style. Created
    // always so a live toggle can swap visibility without rebuilding the window; the full-width
    // textW spans where the power graph sits (the graph is hidden in oil mode).
    int oilW = w - 2 * BOX_PAD - 8;
    m_lblPowerOilHdr = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh,             oilW, ROW_H, "Converting coal to oil");
    m_lblPowerOil    = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + ROW_H,     oilW, ROW_H, "This building: 0");
    m_lblPowerOilCol = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 2 * ROW_H, oilW, ROW_H, "Colony: 0 / 0");
    // C4: oil-mode conversion progress bar — placed where the power graph sits (hidden in oil mode),
    // so the manufacturing progress (coal->oil) is visible like any producer. Shown only in oil mode.
    m_progPowerOil   = AddWidget<SDL2ProgressBar>(graphX, yh + 3 * ROW_H + 2, graphW, 14);
    // C5: fuel-input line shown in NORMAL/power mode (3rd left-column row; GRAPH_H=72 fits 3 rows).
    m_lblPowerFuel   = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6 + 2 * ROW_H, textW, ROW_H, "");
    return y + POWERLIKE_H + SEC_PAD;
}

// Offices: desk capacity vs office workers — strictly this-building + colony
// capacity readings. The colony workforce needed/have readout is its own
// SEPARATE section below (operator: "these are two separate things").
int SDL2BuildingWindow::BuildOffice(int x, int y, int w) {
    AddOutline(x, y, w, OFFICE_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Offices", kAccentGrn, ICON_PEOPLE);
    int graphW = 168, graphX = x + w - graphW - BOX_PAD;
    int textW  = graphX - ( x + BOX_PAD + 4 ) - 6;
    m_lblOfcBldg   = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6,             textW, ROW_H, "This building: 0");
    m_lblOfcColony = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6 + ROW_H,     textW, ROW_H, "Colony: 0 / 0");
    m_imgOfcGraph  = AddWidget<SDL2Image>(graphX, yh, graphW, GRAPH_H);
    AddGraphRangeRow(graphX, yh + GRAPH_H + 2, graphW);
    return y + OFFICE_H + SEC_PAD;
}

// Workforce: the colony's labor balance — workers needed by all buildings vs
// workers available. Same layout as Offices (rows left, history graph right).
// Strictly workforce numbers (operator): the colony Energy Need readout lives
// in the Power/Military sections, not here.
int SDL2BuildingWindow::BuildWorkforce(int x, int y, int w) {
    AddOutline(x, y, w, WORKFORCE_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Workforce", kAccentGrn, ICON_PEOPLE);
    int graphW = 168, graphX = x + w - graphW - BOX_PAD;
    int textW  = graphX - ( x + BOX_PAD + 4 ) - 6;
    m_lblWfHave   = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6,             textW, ROW_H, "Have: 0");
    m_lblWfNeed   = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6 + ROW_H,     textW, ROW_H, "Need: 0");
    m_imgWfGraph  = AddWidget<SDL2Image>(graphX, yh, graphW, GRAPH_H);
    AddGraphRangeRow(graphX, yh + GRAPH_H + 2, graphW);
    return y + WORKFORCE_H + SEC_PAD;
}

int SDL2BuildingWindow::BuildApt(int x, int y, int w) {
    AddOutline(x, y, w, POWERLIKE_H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Population", kAccentGrn, ICON_PEOPLE);
    int graphW = 168, graphX = x + w - graphW - BOX_PAD;
    int textW  = graphX - ( x + BOX_PAD + 4 ) - 6;
    m_lblAptBldg   = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6,         textW, ROW_H, "This building: 0");
    m_lblAptColony = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6 + ROW_H, textW, ROW_H, "Colony: 0 / 0");
    // Food demand (#37/#39): colony food NEED — the figure Nutrition's +20%-food upkeep raises.
    // Nutrition is hosted on the apartment, so without this row the bump is invisible in the
    // window that toggles it (the FOOD sibling of the Office's Workforce/Energy need rows).
    m_lblAptNeed   = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 6 + 2 * ROW_H, textW, ROW_H, "Food Need: 0");
    m_imgAptGraph  = AddWidget<SDL2Image>(graphX, yh, graphW, GRAPH_H);
    AddGraphRangeRow(graphX, yh + GRAPH_H + 2, graphW);
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
    // Energy demand (#39): colony power NEED — the figure a +energy-upkeep edict
    // (e.g. Fortify Border, hosted here on the Command Center) raises. The Command
    // Center has no Power section, so without this row the +20%-energy-upkeep bump
    // is invisible in the very window that toggles the edict (the ENERGY sibling of
    // the #37 "Workforce Need" row in the Office section).
    m_lblMilEnergy   = AddWidget<SDL2Label>(x + BOX_PAD + 4, yh + 3 * ROW_H, tw, ROW_H, "Energy Need: 0");
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
    int rowH  = matRowH();
    int sectH = BOX_PAD + HDR_H + m_nInputMats * rowH + BOX_PAD;
    AddOutline(x, y, w, sectH);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Inputs", kAccentGold,
                    ICON_MATERIALS, ( m_nInputMats > 0 ) ? m_inputMats[0] : 0,
                    nullptr, &m_imgInputHdrIcon);
    m_inputGlyphMat = ( m_nInputMats > 0 ) ? m_inputMats[0] : -1;

    int nameW  = 80;
    int countW = 72;   // fit big amounts ("9,999,999"); icon strip yields the space
    int iconsX = x + BOX_PAD + nameW + 4;
    int iconsW = ( x + w - BOX_PAD - countW - 4 ) - iconsX;

    m_imgInputs = AddWidget<SDL2Image>(iconsX, yh, iconsW, m_nInputMats * rowH);
    for (int i = 0; i < m_nInputMats; i++) {
        int ry = yh + i * rowH;
        m_lblInputName[i]  = AddWidget<SDL2Label>(x + BOX_PAD, ry, nameW, rowH,
                                CMaterialTypes::GetDesc( m_inputMats[i] ).c_str());
        m_lblInputCount[i] = AddWidget<SDL2Label>(x + w - BOX_PAD - countW, ry, countW, rowH, "0");
        m_lblInputCount[i]->SetRightAligned(true);
    }
    return y + sectH + SEC_PAD;
}

// Production buildings: a limited storage widget for the materials they produce
// (steel for a smelter, ore for a mine), so you can see the stockpile awaiting pickup.
int SDL2BuildingWindow::BuildOutputs(int x, int y, int w) {
    int rowH  = matRowH();
    int sectH = BOX_PAD + HDR_H + m_nOutputMats * rowH + BOX_PAD;
    AddOutline(x, y, w, sectH);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Output", kAccentGold,
                    ICON_MATERIALS, ( m_nOutputMats > 0 ) ? m_outputMats[0] : 0,
                    nullptr, &m_imgOutputHdrIcon);
    m_outputGlyphMat = ( m_nOutputMats > 0 ) ? m_outputMats[0] : -1;

    int nameW  = 80;
    int countW = 72;   // fit big amounts ("9,999,999"); icon strip yields the space
    int iconsX = x + BOX_PAD + nameW + 4;
    int iconsW = ( x + w - BOX_PAD - countW - 4 ) - iconsX;

    m_imgOutputs = AddWidget<SDL2Image>(iconsX, yh, iconsW, m_nOutputMats * rowH);
    for (int i = 0; i < m_nOutputMats; i++) {
        int ry = yh + i * rowH;
        m_lblOutputName[i]  = AddWidget<SDL2Label>(x + BOX_PAD, ry, nameW, rowH,
                                 CMaterialTypes::GetDesc( m_outputMats[i] ).c_str());
        m_lblOutputCount[i] = AddWidget<SDL2Label>(x + w - BOX_PAD - countW, ry, countW, rowH, "0");
        m_lblOutputCount[i]->SetRightAligned(true);
    }
    return y + sectH + SEC_PAD;
}

// Seaport: a strip of icons for the vehicles currently docked inside, plus a count.
int SDL2BuildingWindow::BuildUnits(int x, int y, int w) {
    int rowH = unitsRowH();
    int H    = unitsHeight();
    AddOutline(x, y, w, H);
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Units Inside", kHeaderBlue, ICON_VEHICLES);

    int countW = 48;
    int iconsX = x + BOX_PAD + 4;
    int iconsW = ( x + w - BOX_PAD - countW - 6 ) - iconsX;

    m_imgUnits = AddWidget<SDL2Image>(iconsX, yh, iconsW, rowH);
    m_lblUnits = AddWidget<SDL2Label>(x + w - BOX_PAD - countW, yh, countW, rowH, "0");
    m_lblUnits->SetRightAligned(true);
    return y + H + SEC_PAD;
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
    int rowH = fertRowHFor(m_pBldg);
    int H    = fertilityHeight(m_pBldg);
    AddOutline(x, y, w, H);
    // Lumber-mill "fertility" = tree density -> green density "X"; food farm -> wheat sheaf.
    int yh = Header(x + BOX_PAD, y + BOX_PAD, w - 2 * BOX_PAD, "Fertility", kAccentGrn,
                    fertIsLumber(m_pBldg) ? ICON_DENSITY : ICON_FOOD);

    int countW = 48;
    int iconsX = x + BOX_PAD + 4;
    int iconsW = ( x + w - BOX_PAD - countW - 6 ) - iconsX;

    m_imgFertility = AddWidget<SDL2Image>(iconsX, yh, iconsW, rowH);
    m_lblFertility = AddWidget<SDL2Label>(x + w - BOX_PAD - countW, yh, countW, rowH, "0%");
    m_lblFertility->SetRightAligned(true);
    return y + H + SEC_PAD;
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
// Slot background from the REAL status-bar art (operator: "look at how the
// vehicle window does it"): each theIcons entry carries its status-bar background
// pieces in the sheet row below the icon row (m_cxLeft/m_cxBack/m_cxRight,
// m_cyBack, m_iTypBack) — blit them exactly like SDL2UnitList::Render3PieceBg.
// Falls back to the drawn black/gold box when the entry has no back art.
void SDL2BuildingWindow::DrawSlotBg(SDL_Surface* dst, int iconIdx, SDL_Surface* sheet,
                                    int x, int y, int w, int h) {
    CStatData* sd = ( iconIdx >= 0 ) ? theIcons.GetByIndex( iconIdx ) : nullptr;
    if ( !dst || !sheet || !sd || sd->m_cyBack <= 0 ) {
        drawSlot( dst, x, y, w, h );
        return;
    }
    int bgSrcY = sd->m_cyIcon;               // background row is below the icon row
    int bh = __min( sd->m_cyBack, h );
    int by = y + ( h - bh ) / 2;
    SDL_SetSurfaceBlendMode( sheet, SDL_BLENDMODE_BLEND );
    if ( sd->m_iTypBack == CStatData::back_3 ) {   // left cap + tiled middle + right cap
        if ( sd->m_cxLeft > 0 ) {
            SDL_Rect sr = { 0, bgSrcY, sd->m_cxLeft, bh };
            SDL_Rect dr = { x, by, sd->m_cxLeft, bh };
            SDL_BlitSurface( sheet, &sr, dst, &dr );
        }
        if ( sd->m_cxBack > 0 ) {
            int midX = x + sd->m_cxLeft, midEnd = x + w - sd->m_cxRight;
            for ( int tx = midX; tx < midEnd; tx += sd->m_cxBack ) {
                int bw = __min( sd->m_cxBack, midEnd - tx );
                SDL_Rect sr = { sd->m_cxLeft, bgSrcY, bw, bh };
                SDL_Rect dr = { tx, by, bw, bh };
                SDL_BlitSurface( sheet, &sr, dst, &dr );
            }
        }
        if ( sd->m_cxRight > 0 ) {
            SDL_Rect sr = { sd->m_cxLeft + sd->m_cxBack, bgSrcY, sd->m_cxRight, bh };
            SDL_Rect dr = { x + w - sd->m_cxRight, by, sd->m_cxRight, bh };
            SDL_BlitSurface( sheet, &sr, dst, &dr );
        }
    } else if ( sd->m_iTypBack == CStatData::full_back ) {   // stretch to fit
        SDL_Rect sr = { 0, bgSrcY, sd->m_cxBack, bh };
        SDL_Rect dr = { x, by, w, bh };
        SDL_BlitScaled( sheet, &sr, dst, &dr );
    } else if ( sd->m_cxBack > 0 ) {                          // tile
        for ( int tx = 0; tx < w; tx += sd->m_cxBack ) {
            int bw = __min( sd->m_cxBack, w - tx );
            SDL_Rect sr = { 0, bgSrcY, bw, bh };
            SDL_Rect dr = { x + tx, by, bw, bh };
            SDL_BlitSurface( sheet, &sr, dst, &dr );
        }
    }
}

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
        int iconW, iconH;
        intFitIcon( m_matIconW, m_matIconH, __min( rowH - 4, dispIconH( m_matIconH ) ), iconW, iconH );
        // Coin-stack overlap: each icon covers ~40% of the previous one, so a big
        // stockpile reads as a dense stack instead of a sparse picket line.
        int step   = __max( 3, ( iconW * 3 ) / 5 );
        int maxFit = ( gw - 4 - iconW >= 0 ) ? ( ( gw - 4 - iconW ) / step + 1 ) : 0;

        SDL_SetSurfaceBlendMode(m_matIcons, SDL_BLENDMODE_BLEND);
        for (int i = 0; i < n; i++) {
            // Slot track for every row — an empty slot IS the "none stored" reading.
            DrawSlotBg( s, ICON_MATERIALS, m_matIcons, 0, i * rowH + 1, gw, rowH - 2 );
            // Use matAmount(), NOT GetStore(): food/gas are COLONY-wide (a building's local
            // GetStore(food) is 0), so raw GetStore left the food/gas rows with no icons even
            // when the colony was flush (operator: 343k food, number shows, no icons). matAmount
            // returns the colony total for food/gas and the building store for everything else.
            int amount = matAmount( m_pBldg, mats[i] );
            if ( amount <= 0 ) continue;
            int nIcons = amount / PER_ICON;
            if ( nIcons < 1 )      nIcons = 1;
            if ( nIcons > maxFit ) nIcons = maxFit;
            int iy = i * rowH + ( rowH - iconH ) / 2;
            SDL_Rect src = { mats[i] * m_matIconW, 0, m_matIconW, m_matIconH };
            for (int k = 0; k < nIcons; k++) {
                SDL_Rect dr = { 2 + k * step, iy, iconW, iconH };
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

    DrawSlotBg( s, m_densIconIdx, m_densIcon, 0, 0, gw, gh );   // gauge track (also the 0% reading)
    if ( m_densIcon && m_densIconW > 0 && m_densIconH > 0 ) {
        int iconW, iconH;
        intFitIcon( m_densIconW, m_densIconH, __min( gh - 4, dispIconH( m_densIconH ) ), iconW, iconH );
        int step  = __max( 3, ( iconW * 3 ) / 5 );   // overlapped, like the storage stacks
        int fillW = ( gw - 4 ) * pct / 100;          // how far the icons extend
        int iy    = ( gh - iconH ) / 2;
        SDL_SetSurfaceBlendMode(m_densIcon, SDL_BLENDMODE_BLEND);
        SDL_Rect src = { 0, 0, m_densIconW, m_densIconH };
        for (int dx = 2; ( dx + iconW ) <= 2 + fillW && step > 0; dx += step) {
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

    DrawSlotBg( s, ICON_VEHICLES, m_unitIcons, 0, 0, gw, gh );  // empty slot = "nothing docked"
    if ( m_unitIcons && m_unitIconW > 0 && m_unitIconH > 0 ) {
        int iconW, iconH;
        intFitIcon( m_unitIconW, m_unitIconH, __min( gh - 4, dispIconH( m_unitIconH ) ), iconW, iconH );
        int step  = iconW + 2;                  // no overlap: each docked unit stays identifiable
        int iy    = ( gh - iconH ) / 2;
        int drawX = 2;
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
// A tiny row of 5 range buttons placed under a graph. Subtle, ~33px each; all
// rows in the window share m_iGraphRange, so clicking any one rescales every graph.
void SDL2BuildingWindow::AddGraphRangeRow(int x, int y, int w) {
    // Labels are REAL time, not game time. One history sample lands per game-minute
    // (~1 real second), so the ranges work out to: 10 samples ~10s, 60 ~1m, and the
    // 5/15/150-game-min rings x120 samples ~10m / ~30m / ~5h.
    static const char* kLbl[5] = { "10s", "1m", "10m", "30m", "5h" };
    int bw = w / 5;
    for ( int r = 0; r < 5; r++ ) {
        SDL2Button* b = AddWidget<SDL2Button>( x + r * bw, y, bw - 2, RANGE_ROW_H, kLbl[r],
            [this, r]() { SetGraphRange( r ); } );
        b->SetToggled( r == m_iGraphRange );
        m_rangeBtns.push_back( b );
    }
}

// Click handler: switch the time range, re-highlight every range button, redraw graphs.
void SDL2BuildingWindow::SetGraphRange(int range) {
    if ( range < 0 || range > 4 ) return;
    m_iGraphRange = range;
    for ( size_t i = 0; i < m_rangeBtns.size(); i++ )
        m_rangeBtns[i]->SetToggled( (int)( i % 5 ) == range );
    Refresh();   // re-runs the DrawGraph calls with the new range
}

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
    // Chart panel: dark warm brown (tinted toward the parchment palette instead of
    // the old near-black hole), quarter-height gridlines, gold-toned frame.
    SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 38, 34, 26));
    Uint32 grid = SDL_MapRGB(s->format, 56, 50, 38);
    for ( int g = 1; g <= 3; g++ ) {
        SDL_Rect gl = { 1, ( gh * g ) / 4, gw - 2, 1 };
        SDL_FillRect(s, &gl, grid);
    }
    Uint32 frame = SDL_MapRGB(s->format, 90, 80, 55);
    SDL_Rect top = { 0, 0, gw, 1 }, bot = { 0, gh - 1, gw, 1 };
    SDL_Rect lft = { 0, 0, 1, gh }, rgt = { gw - 1, 0, 1, gh };
    SDL_FillRect(s, &top, frame); SDL_FillRect(s, &bot, frame);
    SDL_FillRect(s, &lft, frame); SDL_FillRect(s, &rgt, frame);

    // Selected time-range -> data source. 10s/1m read the EXISTING serialized
    // per-minute buffer (so a freshly-loaded save shows its saved history
    // immediately) — last 10 / last 60 samples. 10m/30m/5h read the runtime rings
    // 0/1/2 (5/15/150-game-min cadence), which fill in as the game is played.
    // A coarse ring needs 2 samples before it can plot (the 5h ring's second
    // sample lands ~5 real minutes in) — rather than show an empty panel ("3h is
    // blank"), degrade to the finest source that HAS data: it's the same series,
    // just a shorter span.
    int grSel = ( m_iGraphRange >= 0 && m_iGraphRange < 5 ) ? m_iGraphRange : 1;
    int gr    = grSel;
    while ( gr > 1 && p->GetHRCount( gr - 2 ) < 2 )
        gr--;
    // R = the SELECTED button's span (game-min); c = the SOURCE's sample cadence.
    // x is mapped by TIME below (not sample index), so fallback data on a coarse
    // view compresses into its true sliver of the axis instead of stretching —
    // otherwise 30m and 5h render identically whenever 5h degrades to the 30m ring.
    static const long kRangeGm[5] = { 10, 60, 600, 1800, 18000 };
    static const int  kRingCad[3] = { 5, 15, 150 };   // lock-step with player.cpp _hrCad
    long R = kRangeGm[grSel];
    int ring, cnt, nShow, off, c;
    if ( gr <= 1 ) {                              // 10s / 1m -> saved per-minute buffer
        ring = -1;
        c    = 1;
        cnt  = p->GetHistCount();
    } else {                                      // 10m / 30m / 5h -> runtime rings
        ring = gr - 2;
        c    = kRingCad[ring];
        cnt  = p->GetHRCount( ring );
    }
    long maxSamp = R / c;                         // samples that fit the selected span
    if ( maxSamp > 120 ) maxSamp = 120;
    nShow = __min( cnt, (int)maxSamp );
    off   = cnt - nShow;                          // plot the LAST nShow samples
    auto valOf = [&]( HistSeries hs, int i ) -> long {
        int s = (int)hs - (int)kPwrHave;         // kPwrHave->0 ... kPplNeed->6
        if ( s < 0 || s > 6 ) return 0;
        if ( ring < 0 ) {                        // per-minute buffer
            switch ( hs ) {
                case kPwrHave:  return p->GetHistPwrHave ( off + i );
                case kPwrNeed:  return p->GetHistPwrNeed ( off + i );
                case kPplTotal: return p->GetHistPplTotal( off + i );
                case kPplBldg:  return p->GetHistPplBldg ( off + i );
                case kAptCap:   return p->GetHistAptCap  ( off + i );
                case kOfcCap:   return p->GetHistOfcCap  ( off + i );
                case kPplNeed:  return p->GetHistPplNeed ( off + i );
                default:        return 0;
            }
        }
        return p->GetHR( ring, s, off + i );
    };

    int n = nShow;
    if ( n >= 2 ) {
        long maxV = 1;
        for ( int i = 0; i < n; i++ ) {
            maxV = __max( maxV, valOf(a, i) );
            if ( b != kNone ) maxV = __max( maxV, valOf(b, i) );
        }
        const int pad = 2;
        int plotW = gw - 2 * pad, plotH = gh - 2 * pad;
        HistSeries      series[2]  = { a, b };
        Uint32          colors[2]  = { SDL_MapRGB(s->format,  90, 220, 110),
                                       SDL_MapRGB(s->format, 235, 180,  60) };

        // TIME-TRUE x mapping, right-anchored: sample i's x position comes from its
        // AGE within the selected span R (newest at the right edge). A young game /
        // fresh load draws only the portion of the axis it has data for; the blank
        // left is honestly "no data yet". (The old stretch-to-fit made 18 minutes
        // of data span a 30-minute axis.)
        auto xAt = [&]( int i ) -> int {
            long age = (long)( n - 1 - i ) * c;   // game-min before "now"
            return pad + (int)( ( ( R - age ) * (long)( plotW - 1 ) ) / R );
        };

        // No area fill under the primary series (operator): the green line stands
        // alone and the panel background/gridlines show through beneath it — the
        // old pre-blended fill read as a solid green block.

        for ( int sIdx = 0; sIdx < 2; sIdx++ ) {
            if ( series[sIdx] == kNone ) continue;
            int prevX = 0, prevY = 0;
            for ( int i = 0; i < n; i++ ) {
                int px = xAt( i );
                int py = pad + ( plotH - 1 ) - (int)( ( valOf(series[sIdx], i) * (long)( plotH - 1 ) ) / maxV );
                if ( i > 0 ) {
                    // 2px stroke (drawn twice, 1px apart) — a bare 1px Bresenham
                    // polyline was near-invisible at 168x72.
                    lineOnSurface(s, prevX, prevY, px, py, colors[sIdx]);
                    lineOnSurface(s, prevX, prevY + 1, px, py + 1, colors[sIdx]);
                }
                prevX = px; prevY = py;
            }
        }

        // Legend with live values, top-left: a color swatch + "<name> <current>"
        // per series. Without this the two lines were unlabeled and unreadable.
        if ( TTF_Font* f = GetFont( 10 ) ) {
            static const char* kSeriesName[7] =
                { "Have", "Need", "People", "Workers", "Capacity", "Capacity", "Need" };
            SDL_Color txtC = { 214, 204, 174, 255 };
            int ly = pad + 2;
            for ( int sIdx = 0; sIdx < 2; sIdx++ ) {
                if ( series[sIdx] == kNone ) continue;
                int nameIdx = (int)series[sIdx] - (int)kPwrHave;
                if ( nameIdx < 0 || nameIdx > 6 ) continue;
                std::string txt = std::string( kSeriesName[nameIdx] ) + " " +
                                  FmtNum( (int)valOf( series[sIdx], n - 1 ) );
                if ( SDL_Surface* ts = TTF_RenderUTF8_Blended( f, txt.c_str(), txtC ) ) {
                    // Backdrop plate, 50% transparent (operator req): keeps the text
                    // readable where lines cross it without fully hiding the plot.
                    SDL_Rect plate = { pad + 1, ly - 1, 11 + ts->w + 4, ts->h + 2 };
                    fillRect50( s, plate, 22, 20, 15 );
                    SDL_Rect sw = { pad + 3, ly + 2, 6, 6 };
                    SDL_FillRect(s, &sw, colors[sIdx]);
                    SDL_Rect dr = { pad + 12, ly, ts->w, ts->h };
                    SDL_BlitSurface( ts, nullptr, s, &dr );
                    ly += __max( 11, ts->h ) + 1;
                    SDL_FreeSurface( ts );
                } else {
                    ly += 11;
                }
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

    // Edicts rows stay live: re-sync each checkbox from the player's bitmask (the state
    // can change outside this window — harness setedict, last-host auto-revoke §29), and
    // if the research-gated row COUNT changed (topic discovered with the window open),
    // request the deferred Rebuild — the frame was sized for the ungated host maximum,
    // so the new row fits without a window resize. (Same OnFrame relayout path as C6.)
    if ( m_nEdictRows > 0 ) {
        for ( int i = 0; i < m_nEdictRows; i++ )
            if ( m_chkEdict[i] )
                m_chkEdict[i]->SetChecked( p->IsEdictActive( m_edictIds[i] ) );
        if ( nCivEdictsFor( m_pBldg ) != m_nEdictRows )
            m_bNeedRelayout = true;
    }
    // Same external-change sync for the building-scoped AltOutput toggle (harness setalt).
    if ( m_chkAltOut )
        m_chkAltOut->SetChecked( m_pBldg->IsFlag( CUnit::alt_oil ) );
    // Scrounge hosts (rocket Desperate / warehouse Scrounging): the Production section appears/
    // disappears with the toggle. If the active state no longer matches what's shown (checkbox OR
    // an external toggle: harness, rocket-death auto-revoke), relayout so the section set matches.
    if ( scroungeCapable( m_pBldg ) && ( scroungeActive( m_pBldg ) != m_bProduction ) )
        m_bNeedRelayout = true;

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

    // Live operating cost (operator): mirror the sim's per-state power/worker draw so the line
    // shows the ACTUAL draw, not just the base spec — a fracked exhausted well reads 2x (the
    // +100% is now visible), an idle exhausted well 0, a stopped well half. Matches FrackTick /
    // the stopped|abandoned path in mainloop.cpp.
    if ( m_lblOperCost ) {
        int basePwr = m_pBldg->GetData()->GetPower();
        int basePpl = m_pBldg->GetData()->GetPeople();
        int curPwr = basePwr, curPpl = basePpl;
        const char* mode = "";
        bool fracking = m_pBldg->IsFlag( CUnit::alt_oil )
                     && ( m_pBldg->IsFlag( CUnit::stopped ) || m_pBldg->IsFlag( CUnit::abandoned ) )
                     && ( m_pBldg->GetData()->GetUnionType() == CStructureData::UTmine )
                     && ( AltOutput::Available( m_pBldg ) != nullptr );
        if ( fracking ) {
            curPwr = basePwr * 2; curPpl = basePpl * 2; mode = "  (fracking)";
        } else if ( m_pBldg->IsFlag( CUnit::abandoned ) ) {
            curPwr = 0; curPpl = 0; mode = "  (exhausted)";
        } else if ( m_pBldg->IsFlag( CUnit::stopped ) ) {
            curPwr = basePwr / 2; curPpl = basePpl / 2; mode = "  (stopped)";
        }
        m_lblOperCost->SetText( ( "Power required: " + std::to_string( curPwr )
                              + "     Workers: "     + std::to_string( curPpl ) + mode ).c_str() );
    }

    DrawHealthBar();

    if ( m_bStorage ) {
        DrawMatIcons( m_imgStorage, kStoreMats, kNumStoreMats );
        for ( int i = 0; i < kNumStoreMats; i++ )
            if ( m_lblStoreCount[i] )
                m_lblStoreCount[i]->SetText( FmtNum( m_pBldg->GetStore( kStoreMats[i] ) ) );
    }

    if ( m_bProduction && m_lblProduction ) {
        // #51: when an AltOutput mode is ON (charcoal/bio-oil/fracking), the Production widget
        // must show the ALT resource, not the primary output — the operator's #1 blocker was a
        // charcoal-ON lumber mill still reading "~486 lumber/min" here. ShowStatusText returns
        // ONLY the primary output (not AltOutput-aware), so override it for the active mode.
        // Coal-liq is excluded (its power-plant host has no Production section; it shows oil in
        // the Power section).
        const AltOutput::AltOutputDef* pAlt = AltOutput::Available( m_pBldg );
        // C6: a coal-liq-active power plant is now rendered HERE as a producer (coal -> oil), so it
        // gets a real Production status + bar instead of the old repurposed Power section.
        bool bCoalLiq = secCoalLiqActive( m_pBldg );
        bool bAlt = ( pAlt != nullptr ) && m_pBldg->IsFlag( CUnit::alt_oil ) && !bCoalLiq;
        // Desperate Measures (rocket edict): not an AltOutput def, so show its fixed scrounge rate here.
        bool bScroungeRocket = ( m_pBldg->GetData()->GetType() == CStructureData::rocket )
                               && m_pBldg->GetOwner() && m_pBldg->GetOwner()->IsEdictActive( EDICT_DESPERATE_MEASURES );
        std::string str;
        if ( bScroungeRocket ) {
            str = "Producing: +10 lumber +5 iron +5 food +5 coal / min";
        } else if ( bCoalLiq ) {
            // Name the oil product and show its per-minute rate so it matches every other producer
            // widget ("Producing Oil: N / min"), per the operator. The coal plant burns coal at
            // GetRate() build-units per coal (BuildPower) and Convert credits 1 oil per m_iRatioIn
            // coal (eRatioConsume). Effective oil/min = GetFrameProd(framesPerMin / GetRate()) /
            // ratioIn — the SAME GetFrameProd(speed * 24*60 * out / batchTime) form the materials
            // status uses (new_unit.cpp:167), with power's prod basis of 1.
            int oilMat = pAlt ? pAlt->m_iOutputMat : CMaterialTypes::oil;
            str = std::string( "Producing " ) + CMaterialTypes::GetDesc( oilMat ).c_str();
            CBuildPower* pBp     = m_pBldg->GetData()->GetBldPower();
            int          ratioIn = pAlt ? pAlt->m_iRatioIn : 3;
            if ( pBp && pBp->GetRate() > 0 && ratioIn > 0 ) {
                // Reflect ACTUAL production, like the other producers: an empty coal store means the
                // plant converts NOTHING -> show 0/min, not the theoretical max. Mirrors the I1 fix
                // in primaryRatePerMin (empty input -> 0) and the runtime BuildPower gate
                // (mainloop.cpp:2476: GetStore(input)<=0 -> no burn, no oil).
                int inMat     = pAlt ? pAlt->m_iInputMat : CMaterialTypes::coal;
                int oilPerMin = ( m_pBldg->GetStore( inMat ) <= 0 ) ? 0
                                : (int)( m_pBldg->GetFrameProd( float( 24 * 60 ) / (float)pBp->GetRate() )
                                         / (float)ratioIn );
                str += ": " + FmtNum( oilPerMin ) + " / min";
            }
        } else if ( bAlt ) {
            str = AltProductionStatus( m_pBldg, pAlt );
        } else {
            m_pBldg->ShowStatusText( str );
            // #51: ShowStatusText is empty for several producers on this build -> the label went
            // blank (and an empty SDL2Label leaves the prior alt glyph on screen). Fall back to
            // naming the primary output so toggling a mode OFF reliably reverts to the real
            // product instead of blank-or-stale-alt-text.
            if ( str.empty() )
                str = PrimaryProductionStatus( m_pBldg );
        }
        m_lblProduction->SetText( str );
        if ( m_progProduction ) {
            // Operator (H2): the Production widget must ALWAYS keep its progress bar — hiding it
            // for a flat-trickle alt mode (Fracking) was a regression. Keep the bar visible; show
            // the real cycle progress when there is one, else 0 (the bar stays present).
            // C6: the coal-liq plant has no GetProductionPer cycle, so drive it from the conversion
            // accumulator (GetAltProgressPer, the same source as the old C4 power-section bar).
            int per;
            if ( bCoalLiq )
                per = m_pBldg->GetAltProgressPer();
            else if ( bScroungeRocket || ( bAlt && pAlt && ( pAlt->m_eMode == AltOutput::eMultiTrickle ) ) )
                per = m_pBldg->GetAltProgressPerMulti();   // rocket edict / warehouse scrounge trickle
            else
                per = m_pBldg->GetProductionPer();
            m_progProduction->SetVisible( true );
            m_progProduction->SetProgress( per < 0 ? 0 : per );
        }
    }

    // #51 follow-up (gap 2): when an AltOutput conversion is ON, the Inputs/Outputs sections show
    // the ALT chain's materials (bio-oil refinery: food in -> oil out) instead of the static primary
    // chain (oil in -> gas out) — the operator's "inputs/outputs still show the oil->gas chain". The
    // alt input/output is a single material each, so we swap slot 0; other slots (multi-input smelter,
    // which isn't alt-capable) are untouched. Coal-liq is excluded (shows in the Power section).
    const AltOutput::AltOutputDef* pIO = AltOutput::Available( m_pBldg );
    bool bAltIO = ( pIO != nullptr ) && m_pBldg->IsFlag( CUnit::alt_oil ) && !secCoalLiqActive( m_pBldg );
    int altInMat  = ( bAltIO && ( pIO->m_eMode == AltOutput::eRatioConsume ||
                                  pIO->m_eMode == AltOutput::eGlobalConsume ) ) ? pIO->m_iInputMat : -1;
    int altOutMat = bAltIO ? pIO->m_iOutputMat : -1;

    if ( m_bInputs ) {
        int inMats[kMaxInputs];
        for ( int i = 0; i < m_nInputMats; i++ ) inMats[i] = m_inputMats[i];
        if ( altInMat >= 0 && m_nInputMats > 0 ) inMats[0] = altInMat;
        // operator B5/J4: swap the section-HEADER glyph to the active input material (bio-oil: oil→food)
        if ( m_nInputMats > 0 && inMats[0] != m_inputGlyphMat ) {
            SetHdrIcon( m_imgInputHdrIcon, ICON_MATERIALS, inMats[0] );
            m_inputGlyphMat = inMats[0];
        }
        DrawMatIcons( m_imgInputs, inMats, m_nInputMats );
        for ( int i = 0; i < m_nInputMats; i++ ) {
            if ( m_lblInputName[i]  ) m_lblInputName[i]->SetText( CMaterialTypes::GetDesc( inMats[i] ).c_str() );
            if ( m_lblInputCount[i] ) m_lblInputCount[i]->SetText( FmtNum( matAmount( m_pBldg, inMats[i] ) ) );
        }
    }

    if ( m_bOutputs ) {
        int outMats[kMaxInputs];
        for ( int i = 0; i < m_nOutputMats; i++ ) outMats[i] = m_outputMats[i];
        if ( altOutMat >= 0 && m_nOutputMats > 0 ) outMats[0] = altOutMat;
        // operator B5/J4: swap the section-HEADER glyph to the active output material
        // (bio-oil: gas→oil; charcoal: lumber→coal) so the chip matches the real product.
        if ( m_nOutputMats > 0 && outMats[0] != m_outputGlyphMat ) {
            SetHdrIcon( m_imgOutputHdrIcon, ICON_MATERIALS, outMats[0] );
            m_outputGlyphMat = outMats[0];
        }
        DrawMatIcons( m_imgOutputs, outMats, m_nOutputMats );
        for ( int i = 0; i < m_nOutputMats; i++ ) {
            // operator B6: a rebranded conversion output uses the def's display LABEL
            // ("Charcoal"/"Bio Oil"), not the raw material name ("Coal"/"Oil"). eFlatTrickle
            // (Fracking → plain oil) and coal-liq (excluded) keep the real resource name.
            std::string oname = CMaterialTypes::GetDesc( outMats[i] ).c_str();
            if ( i == 0 && altOutMat >= 0 && pIO && pIO->m_szLabel &&
                 pIO->m_eMode != AltOutput::eFlatTrickle )
                oname = pIO->m_szLabel;
            if ( m_lblOutputName[i]  ) m_lblOutputName[i]->SetText( oname );
            if ( m_lblOutputCount[i] ) m_lblOutputCount[i]->SetText( FmtNum( matAmount( m_pBldg, outMats[i] ) ) );
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
        // #43: a coal plant in Coal-Liquefaction mode produces OIL, not power — so swap the
        // section's readout live. In oil mode: hide the power rows + graph, show the oil rows
        // (status + this-building store + colony have/made). Otherwise: the normal power
        // readout, oil rows hidden. (Both row sets exist from BuildPower; we just toggle
        // visibility so a live checkbox toggle needs no window rebuild.)
        bool bOil = secCoalLiqActive( m_pBldg );

        // #6: swap the section HEADER text live too (was fixed at build → showed the open-time
        // mode even after toggling). Mirrors the row-visibility swap below.
        if ( m_lblPowerHdr )    m_lblPowerHdr->SetText( bOil ? "Oil" : "Power" );
        // operator C1 (live): swap the header ICON in lockstep with the text — toggling coal-liq
        // while the window is open now updates the glyph (oil <-> power bulb), not just the label.
        if ( m_imgPowerHdrIcon )
            SetHdrIcon( m_imgPowerHdrIcon, bOil ? ICON_MATERIALS : ICON_POWER, bOil ? CMaterialTypes::oil : 0 );
        if ( m_lblPowerBldg )   m_lblPowerBldg->SetVisible( !bOil );
        if ( m_lblPowerColony ) m_lblPowerColony->SetVisible( !bOil );
        if ( m_imgPowerGraph )  m_imgPowerGraph->SetVisible( !bOil );
        if ( m_lblPowerOilHdr ) m_lblPowerOilHdr->SetVisible( bOil );
        if ( m_lblPowerOil )    m_lblPowerOil->SetVisible( bOil );
        if ( m_lblPowerOilCol ) m_lblPowerOilCol->SetVisible( bOil );
        if ( m_progPowerOil )   m_progPowerOil->SetVisible( bOil );    // C4: oil-mode progress bar
        if ( m_lblPowerFuel )   m_lblPowerFuel->SetVisible( !bOil );   // C5: fuel input in power mode

        if ( bOil ) {
            int oilHere = m_pBldg->GetStore( CMaterialTypes::oil );
            int oilHave = p->GetMaterialHave( CMaterialTypes::oil );
            // Derive the ratio from the live AltOutput def (m_iRatioIn) so the readout never
            // goes stale after a balance change (was hardcoded "2 coal -> 1 oil"; def is now 3:1).
            int coalPerOil = 2;
            if ( const AltOutput::AltOutputDef* pDefCl = AltOutput::Available( m_pBldg ) )
                if ( pDefCl->m_iRatioIn > 0 ) coalPerOil = pDefCl->m_iRatioIn;
            // operator C2 (2026-06-28): a coal plant in oil mode is a PRODUCTION building — show the
            // conversion as INPUT (coal consumed from its store) -> OUTPUT (oil), not just an oil total.
            int coalHere = m_pBldg->GetStore( CMaterialTypes::coal );
            if ( m_lblPowerOilHdr ) m_lblPowerOilHdr->SetText(
                "Converting coal -> oil (" + std::to_string( coalPerOil ) + " coal : 1 oil)" );
            if ( m_lblPowerOil )    m_lblPowerOil->SetText( "Input - Coal: " + FmtNum( coalHere ) );
            if ( m_lblPowerOilCol ) m_lblPowerOilCol->SetText( "Output - Oil: " + FmtNum( oilHere ) +
                                                               "  (colony " + FmtNum( oilHave ) + ")" );
            // C4: conversion progress toward the next oil unit (the AltOutput accumulator).
            if ( m_progPowerOil ) m_progPowerOil->SetProgress( m_pBldg->GetAltProgressPer() );
        } else {
            int bldg  = PerBldgPower();
            int total = (int)p->GetPwrHave();
            int pct   = ( total > 0 ) ? ( bldg * 100 / total ) : 0;
            m_lblPowerBldg->SetText( "This building: " + FmtNum( bldg ) +
                                     " (" + std::to_string( pct ) + "%)" );
            m_lblPowerColony->SetText( "Colony: " + FmtNum( total ) +
                                       " / " + FmtNum( (int)p->GetPwrNeed() ) );
            DrawGraph( m_imgPowerGraph, kPwrHave, kPwrNeed );
            // C5: a fuel-burning plant (coal plant) shows its fuel INPUT even in power mode — the
            // operator's "there's no input at all". Show the plant's input material + on-hand store.
            if ( m_lblPowerFuel ) {
                // OOB-READ FIX (mac2, re linux(newwin-mp) ASan report 13:05Z): the Power section also
                // shows for the ROCKET (secPower() includes GetType()==rocket, not just UTpower), but the
                // rocket's structure-data union is NOT UTpower. GetBldPower()'s unchecked (CBuildPower*)this
                // would then read m_iInput 4 bytes PAST the real (non-power) object -> heap-buffer-overflow
                // READ (UB; ASan-flagged every open, latent crash on a bad page). Only a true UTpower plant
                // has CBuildPower fuel data — gate the cast exactly like PerBldgPower() (line ~1346) does.
                CBuildPower* pBp = ( m_pBldg->GetData()->GetUnionType() == CStructureData::UTpower )
                                   ? m_pBldg->GetData()->GetBldPower() : nullptr;
                int iFuel = pBp ? pBp->GetInput() : -1;
                // CRASH FIX (mac2 23:22): GetInput() returns the raw m_iInput, which for a fuel-less
                // plant can be a sentinel/garbage index >= num_types (not just -1). GetDesc()/GetStore()
                // only ASSERT_STRICT the range — ignored in this build — so an OOB iFuel indexed
                // m_saDesc[] out of bounds -> garbage std::string -> .c_str() segfault. Bound BOTH ends.
                if ( iFuel >= 0 && iFuel < CMaterialTypes::GetNumTypes() )
                    m_lblPowerFuel->SetText( "Input - " + std::string( CMaterialTypes::GetDesc( iFuel ).c_str() ) +
                                             ": " + FmtNum( m_pBldg->GetStore( iFuel ) ) );
                else
                    m_lblPowerFuel->SetText( "" );
            }
        }
    }

    if ( m_bOffice ) {
        m_lblOfcBldg->SetText( "This building: " + FmtNum( PerBldgOfcCap() ) + " desks" );
        m_lblOfcColony->SetText( "Colony: " + FmtNum( (int)p->GetPplBldg() ) +
                                 " / " + FmtNum( (int)p->m_iOfcCap ) );
        DrawGraph( m_imgOfcGraph, kOfcCap, kPplBldg );
    }

    if ( m_bWorkforce ) {
        // Colony labor balance: workers available vs workers all buildings ask for.
        // The Need figure is what a +workforce-upkeep edict (Austerity) raises.
        if ( m_lblWfHave )
            m_lblWfHave->SetText( "Have: " + FmtNum( (int)p->GetPplBldg() ) );
        if ( m_lblWfNeed )
            m_lblWfNeed->SetText( "Need: " + FmtNum( (int)p->GetPplNeedBldg() ) );
        // Green = workers have, gold = workers needed (kPplNeed is runtime-only:
        // flat-at-current for pre-load history, live-tracked from then on).
        DrawGraph( m_imgWfGraph, kPplBldg, kPplNeed );
    }

    if ( m_bApt ) {
        m_lblAptBldg->SetText( "This building: " + FmtNum( PerBldgAptCap() ) + " beds" );
        m_lblAptColony->SetText( "Colony: " + FmtNum( (int)p->GetPplTotal() ) +
                                 " / " + FmtNum( (int)p->m_iAptCap ) );
        if ( m_lblAptNeed )
            m_lblAptNeed->SetText( "Food Need: " + FmtNum( (int)p->GetFoodNeed() ) );
        DrawGraph( m_imgAptGraph, kAptCap, kPplTotal );
    }

    // #43-audit item 2: mode-aware OUTPUT readout in the "Production Mode" section, mirroring the
    // coal-liq Power-section swap for the hosts that lack a Power section (BioFuel farm -> oil,
    // Charcoal lumber mill -> coal, Fracking exhausted well -> oil). Shown only when the toggle is
    // ON and the def is NOT the coal-liq one (that one already shows its readout in the Power
    // section). Uses the def's real conversion values so enabling the toggle visibly shows output.
    // #51 (half-b): the duplicate production readout inside the toggle box (m_lblAltOut*) is removed — toggle box = checkbox + (i) only; production results live in the Production/Inputs/Outputs widgets.

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
        // #39: colony power demand (includes any +energy-upkeep edict bump applied in
        // CPlayer::StartLoop). Toggling Fortify Border here now visibly moves this number.
        if ( m_lblMilEnergy )   m_lblMilEnergy->SetText(   "Energy Need: " + FmtNum( (int)p->GetPwrNeed() ) );
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
