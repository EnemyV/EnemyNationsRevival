#include "stdafx.h"

#include "SDL2BuildTransport.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "player.h"
#include "building.h"
#include "building.inl"
#include "unit.inl"     // CUnit::GetStore / CUnitData::GetDesc/GetText (needed at /Ob2)
#include "vehicle.inl"
#include "bitmaps.h"
#include "icons.h"      // CStatData / CIcons / theIcons — construction-progress strip
#include "netcmd.h"

// ============================================================================
// SDL2BuildTransport — matches CDlgBuildTransport layout from the original
// MFC dialog. Interior client area = 380x332.
//
// MFC layout (from CDlgBuildTransport::OnInitDialog):
//   vehicle buttons (10, 24) 108x48, six slots offset 48px vertically
//   description     (136, 26, 361, 128)   — green wrap text
//   cost grid       (136, 142, ..., 245)  — cost/have/need right-aligned
//                                            at x = 270 / 315 / 360
//   Build button    (133, 258, 84, 23)
//   Cancel button   (282, 258, 84, 23)
//   quantity edit   (282, 295, 40, 22)    — we substitute -/+ buttons
//   progress strip  (134, 292, 253, 318)  — vehicle-being-built icon (skipped)
// ============================================================================

SDL2BuildTransport::SDL2BuildTransport(GameWindow* gw, CVehicleBuilding* pBldg)
    // Interior 380x332 + 6px side borders + 26px title bar — same recipe as
    // SDL2BuildStructure so the dialog chrome aligns with the rest.
    : SDL2Dialog(gw, "Build Vehicle", 380 + 12, 332 + 12 + 26)
    , m_pBldg(pBldg)
{
    memset(m_vehs, 0, sizeof(m_vehs));

    auto load = [](int idx) -> SDL_Surface* {
        CDIB* p = theBitmaps.GetByIndex(idx);
        return p ? SDL2MainMenu::CreateSurfaceFromDIB(p) : nullptr;
    };
    m_vehBkgnd     = load(DIB_VEHICLE_BKGND);
    m_okBtnSheet   = load(DIB_VEHICLE_BTNS_1);
    m_vehBtnSheet  = load(DIB_VEHICLE_BTNS_2);
    m_vehIconSheet = load(DIB_LIST_UNIT_VEHICLES);
}

SDL2BuildTransport::~SDL2BuildTransport() {
    if (m_vehBkgnd)     SDL_FreeSurface(m_vehBkgnd);
    if (m_okBtnSheet)   SDL_FreeSurface(m_okBtnSheet);
    if (m_vehBtnSheet)  SDL_FreeSurface(m_vehBtnSheet);
    if (m_vehIconSheet) SDL_FreeSurface(m_vehIconSheet);
    if (m_iconStrip)    SDL_FreeSurface(m_iconStrip);
    // m_imgProgress' surface is owned by the SDL2Image (takeOwnership=true).
}

void SDL2BuildTransport::OnInit() {
    if (m_vehBkgnd) SetCustomBackground(m_vehBkgnd);

    // Dialog interior origin == MFC client area origin (skip chrome)
    const int bdrSide = 6, bdrTop = 6, titleH = 26;
    const int ox = m_x + bdrSide;
    const int oy = m_y + bdrTop + titleH;

    // --- Vehicle buttons (left column) -------------------------------------
    CBuildVehicle const* pBv = m_pBldg->GetData()->GetBldVehicle();
    int slot = 0;
    for (int i = 0; i < pBv->GetSize() && slot < 6; i++) {
        CBuildUnit const* pBu = pBv->GetUnit(i);
        CTransportData const* pTd = theTransports.GetData(pBu->GetVehType());
        if (!pTd->IsDiscovered())
            continue;
        if (theGame.GetScenario() != -1 && pTd->GetScenario() > theGame.GetScenario())
            continue;

        m_vehs[slot].vehType = pBu->GetVehType();
        m_vehs[slot].pData   = pTd;
        m_vehs[slot].pBu     = pBu;

        int y = oy + 24 + slot * 48;
        m_vehBtns[slot] = AddWidget<SDL2Button>(ox + 10, y, 108, 48,
            pTd->GetDesc().c_str(),
            [this, slot]() { SelectVehicle(slot); });
        m_vehBtns[slot]->SetOnDblClick(
            [this, slot]() { if (m_iVehOn == slot) OnBuild(); });
        if (m_vehBtnSheet) m_vehBtns[slot]->SetBtnSheet(m_vehBtnSheet);

        // Vehicle icon — DIB_LIST_UNIT_VEHICLES is a vertical strip of 64px
        // tiles. m_iOvrlyNum in MFC = vehType, so srcY = vehType * 64.
        if (m_vehIconSheet && m_vehIconSheet->h > 0) {
            int iconW = m_vehIconSheet->w;
            int iconH = 64;
            int srcY  = pBu->GetVehType() * iconH;
            if (srcY + iconH <= m_vehIconSheet->h) {
                SDL_Rect iconSrc = { 0, srcY, iconW, iconH };
                m_vehBtns[slot]->SetIcon(m_vehIconSheet, iconSrc);
            }
        }
        slot++;
    }
    m_numVehs = slot;

    // Hide unused slots so the empty 3-state button art doesn't show
    for (int i = slot; i < 6; i++) {
        int y = oy + 24 + i * 48;
        m_vehBtns[i] = AddWidget<SDL2Button>(ox + 10, y, 108, 48, "", nullptr);
        m_vehBtns[i]->SetVisible(false);
    }

    // --- Description (green wrap text) -------------------------------------
    // MFC rect (136, 26, 361, 128) → x=136, y=26, w=225, h=102
    m_lblDesc = AddWidget<SDL2Label>(ox + 136, oy + 26, 225, 102, "");
    m_lblDesc->SetWrapped(true);
    m_lblDesc->SetTopAligned(true);
    m_lblDesc->SetColor({41, 255, 8, 255});

    // --- Cost grid: header row "cost have need" ----------------------------
    // MFC right-aligns at x = 270 / 315 / 360. Mirror that with per-column
    // right-aligned labels (same pattern as SDL2BuildStructure).
    m_lblCostColHdr = AddWidget<SDL2Label>(ox + 228, oy + 142, 42, 16, "cost");
    m_lblCostColHdr->SetTopAligned(true);
    m_lblCostColHdr->SetRightAligned(true);
    m_lblCostColHdr->SetColor({41, 255, 8, 255});

    m_lblHaveColHdr = AddWidget<SDL2Label>(ox + 273, oy + 142, 42, 16, "have");
    m_lblHaveColHdr->SetTopAligned(true);
    m_lblHaveColHdr->SetRightAligned(true);
    m_lblHaveColHdr->SetColor({41, 255, 8, 255});

    m_lblNeedColHdr = AddWidget<SDL2Label>(ox + 308, oy + 142, 52, 16, "need");
    m_lblNeedColHdr->SetTopAligned(true);
    m_lblNeedColHdr->SetRightAligned(true);
    m_lblNeedColHdr->SetColor({41, 255, 8, 255});

    // Horizontal divider line (under header row)
    {
        auto* hLine = AddWidget<SDL2Image>(ox + 136, oy + 161, 224, 1);
        SDL_Surface* s = SDL_CreateRGBSurface(0, 224, 1, 32, 0xFF0000, 0xFF00, 0xFF, 0);
        if (s) { SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 41, 255, 8)); hLine->SetSurface(s, true); }
    }
    // Vertical divider at x=210 (separates names from values)
    {
        int lineH = 90;
        auto* vLine = AddWidget<SDL2Image>(ox + 210, oy + 142, 1, lineH);
        SDL_Surface* s = SDL_CreateRGBSurface(0, 1, lineH, 32, 0xFF0000, 0xFF00, 0xFF, 0);
        if (s) { SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 41, 255, 8)); vLine->SetSurface(s, true); }
    }

    // --- Cost name column (Time, Lumber, …, Colonists) ---------------------
    m_lblCosts = AddWidget<SDL2Label>(ox + 136, oy + 164, 72, 90, "");
    m_lblCosts->SetWrapped(true);
    m_lblCosts->SetTopAligned(true);
    m_lblCosts->SetColor({41, 255, 8, 255});

    // --- Value columns (right-aligned numbers) -----------------------------
    m_lblCostCol = AddWidget<SDL2Label>(ox + 228, oy + 164, 42, 90, "");
    m_lblCostCol->SetWrapped(true);
    m_lblCostCol->SetTopAligned(true);
    m_lblCostCol->SetRightAligned(true);
    m_lblCostCol->SetColor({41, 255, 8, 255});

    m_lblHaveCol = AddWidget<SDL2Label>(ox + 273, oy + 164, 42, 90, "");
    m_lblHaveCol->SetWrapped(true);
    m_lblHaveCol->SetTopAligned(true);
    m_lblHaveCol->SetRightAligned(true);
    m_lblHaveCol->SetColor({41, 255, 8, 255});

    // 52px wide (not 42 like cost/have): big deficits like -1710 word-wrapped
    // at 42px, pushing every following line down a row and breaking the 1:1
    // row alignment with the name column (showed as the deficit sitting next
    // to the Colonists row).
    m_lblNeedCol = AddWidget<SDL2Label>(ox + 308, oy + 164, 52, 90, "");
    m_lblNeedCol->SetWrapped(true);
    m_lblNeedCol->SetTopAligned(true);
    m_lblNeedCol->SetRightAligned(true);
    m_lblNeedCol->SetColor({255, 41, 8, 255});  // red — only shows when deficit

    // --- Blue operating row: "Colonists" (vehicle crew) ---------------------
    // Vehicles need people to operate, same as buildings — operator-reported
    // that this window never showed the population requirement. Same blue
    // operating-row style as SDL2BuildStructure (a standing need, not a
    // consumed material). Y is repositioned under the last cost row in
    // UpdateDescription.
    m_lblOperNames = AddWidget<SDL2Label>(ox + 136, oy + 238, 72, 16, "");
    m_lblOperNames->SetTopAligned(true);
    m_lblOperNames->SetColor({71, 71, 225, 255});

    m_lblOperVals = AddWidget<SDL2Label>(ox + 228, oy + 238, 42, 16, "");
    m_lblOperVals->SetTopAligned(true);
    m_lblOperVals->SetRightAligned(true);
    m_lblOperVals->SetColor({71, 71, 225, 255});

    // --- Build / Cancel buttons --------------------------------------------
    m_btnBuild = AddWidget<SDL2Button>(ox + 133, oy + 258, 84, 23, "Build",
        [this]() { OnBuild(); });
    m_btnBuild->SetEnabled(false);
    if (m_okBtnSheet) m_btnBuild->SetBtnSheet(m_okBtnSheet);

    auto* btnCancel = AddWidget<SDL2Button>(ox + 282, oy + 258, 84, 23, "Cancel",
        [this]() { OnCancel(); });
    if (m_okBtnSheet) btnCancel->SetBtnSheet(m_okBtnSheet);

    // --- Info (i) button ---------------------------------------------------
    // Top-right of the CLIENT area, just under the title bar. (It can't live in the
    // title bar itself — that strip is the window-drag region and swallows clicks.)
    // Opens the same read-only status window the other buildings use — inputs + the
    // weapon widget for this factory — and asks it to float ON TOP of this dialog
    // (which otherwise bumps itself to front each frame). Double-click here is taken
    // by this build interface, so the (I) is the only path to it.
    auto* btnInfo = AddWidget<SDL2Button>(ox + 380 - 25, oy + 2, 22, 22, "",
        [this]() { if (m_pBldg) m_pBldg->ShowInfoWindow( true ); });
    if (SDL_Surface* g = MakeInfoGlyph(22))
        btnInfo->SetIcon(g, { 0, 0, g->w, g->h });

    // --- Quantity control --------------------------------------------------
    // MFC: editable number field (282,295,40,22) + a vertical up/down spinner
    // (msctls_updown32, 307,295,15,22) sitting on its right edge. We restore the
    // typeable field at the original coords and put a 15px vertical up/down
    // spinner immediately to its right (two stacked blue-arrow buttons).
    const int qy = oy + 295, qH = 22, qEditW = 40;
    const int qEditX = ox + 282;
    m_edtNum = AddWidget<SDL2EditBox>(qEditX, qy, qEditW, qH,
        std::to_string(m_buildNum),
        [this](const std::string& s) { OnQtyEdited(s); });
    // Black field with white text — fits the dark vehicle-build chrome far
    // better than the stock white box (the MFC original was a plain CEdit).
    m_edtNum->SetColors({ 0, 0, 0, 255 }, { 255, 255, 255, 255 });

    // Vertical spinner: up arrow (top half) increments, down arrow (bottom half)
    // decrements — the original IDC_BUILD_SPIN behaviour. Placed on the arrow
    // widget PAINTED INTO the background art: measured in veh_bknd.png the blue
    // up/down triangles sit at x≈346..368, y≈292..318 (NOT at the MFC control
    // rect 307,295 — that spot is the black edit box in the art). The live
    // buttons must cover the painted pair, or a second dead pair shows.
    const int spX = ox + 346, spY = oy + 292, spW = 22, spH = 13;
    auto* btnUp = AddWidget<SDL2Button>(spX, spY, spW, spH, "",
        [this]() { m_buildNum++; RefreshQty(); });
    auto* btnDn = AddWidget<SDL2Button>(spX, spY + spH, spW, spH, "",
        [this]() { if (m_buildNum > 1) { m_buildNum--; RefreshQty(); } });
    if (SDL_Surface* up = MakeArrow(spW, spH, true))
        btnUp->SetIcon(up, { 0, 0, up->w, up->h });
    if (SDL_Surface* dn = MakeArrow(spW, spH, false))
        btnDn->SetIcon(dn, { 0, 0, dn->w, dn->h });

    // --- Construction-progress strip ("black box" left of the quantity) -----
    // MFC: m_statInst (CStatInst) at rectTranStat (134,292,253,318) drawing
    // ICON_BUILD_VEH icons filled to GetBuildPer(). Load the icon metadata + its
    // bitmap once; OnFrame advances the fill live.
    m_pStatData = theIcons.GetByIndex(ICON_BUILD_VEH);
    if (m_pStatData && m_pStatData->m_pcDib)
        m_iconStrip = SDL2MainMenu::CreateSurfaceFromDIB(m_pStatData->m_pcDib);
    m_imgProgress = AddWidget<SDL2Image>(ox + 134, oy + 292, 119, 26);
    // Only show progress when a vehicle is actually being built. GetBuildPer() returns
    // a STALE m_iLastPer when idle, which drew phantom progress cars on open (bug:
    // window shows build progress with nothing queued). GetBldUnt()==NULL means idle.
    RebuildProgress( m_pBldg->GetBldUnt() ? __max(0, m_pBldg->GetBuildPer()) : 0 );

    // Auto-select first vehicle if available
    if (m_numVehs > 0)
        SelectVehicle(0);
}

// Replays CStatInst::DrawStatDone (icons.cpp): a row of ICON_BUILD_VEH icons,
// vertically centred in the box, drawn left-to-right up to `per` percent. The
// black box behind comes from the dialog's background art; we paint icons over
// a transparent overlay so the recessed box still shows through where empty.
void SDL2BuildTransport::RebuildProgress(int per) {
    if (!m_imgProgress) return;
    if (per < 0)   per = 0;
    if (per > 100) per = 100;

    const int boxW = 119, boxH = 26;
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, boxW, boxH, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) return;
    SDL_FillRect(surf, nullptr, SDL_MapRGBA(surf->format, 0, 0, 0, 0));   // transparent

    if (m_iconStrip && m_pStatData && m_pStatData->m_cxIcon > 0) {
        const int cxIcon = m_pStatData->m_cxIcon;
        const int cyIcon = m_pStatData->m_cyIcon;
        // 0-based geometry, mirroring DrawStatDone exactly.
        int top  = (boxH - cyIcon) / 2;
        int left = m_pStatData->m_leftOff;
        int right = boxW - m_pStatData->m_rightOff;
        int iEnd = right;
        if (per < 100)                       // last icon only lands at 100%
            iEnd -= cxIcon / 2;
        int iRight = ((right - left) * per) / 100;
        if (per > 0)                         // at least 1px when non-zero
            iRight = __max(left + 1, iRight);

        SDL_SetSurfaceBlendMode(m_iconStrip, SDL_BLENDMODE_BLEND);
        SDL_Rect src = { 0, 0, cxIcon, cyIcon };   // m_iBaseOn==0 (SetPerBase never called)
        int x = left;
        for (; x < iRight; x += cxIcon / 2) {
            if (x + cxIcon > iEnd)
                break;
            SDL_Rect dst = { x, top, cxIcon, cyIcon };
            SDL_BlitSurface(m_iconStrip, &src, surf, &dst);
        }
    }

    m_imgProgress->SetSurface(surf, true);   // image takes ownership
}

void SDL2BuildTransport::OnFrame() {
    // LOAD-while-in-game crash (mac2): skip per-frame refresh while CGame::LoadGame tears down the
    // world — m_pBldg (a CVehicleBuilding*) is freed by NewWorld's RemoveAll, but LoadGame
    // BaseYield()s before the dialogs close, so this deref crashes. Same flag/family as
    // SDL2BuildingWindow::OnFrame. (newwin's 4th-path audit.)
    if ( theApp.IsWorldTearingDown( ) ) return;
    // Poll the building's live build %; advance the strip + title only on change.
    // Idle (no vehicle queued) -> 0, so the stale m_iLastPer never shows phantom cars.
    int per = m_pBldg->GetBldUnt() ? __max(0, m_pBldg->GetBuildPer()) : 0;
    if (per == m_lastPer)
        return;
    m_lastPer = per;
    RebuildProgress(per);

    // Title: "Building <vehicle>..." while constructing, else the plain caption —
    // barracks say "Build People", everything else "Build Vehicle" (mirrors
    // CDlgBuildTransport::UpdateStatus / OnInitDialog).
    if (per != 0) {
        CBuildUnit const* pBu = m_pBldg->GetBldUnt();
        const char* pDesc = pBu ? theTransports.GetData(pBu->GetVehType())->GetDesc().c_str() : "";
        m_title = strPrintf(EnLoadStdString(IDS_BUILD_UNIT).c_str(), pDesc);
    } else if (m_pBldg->GetData()->GetBldgType() == CStructureData::barracks) {
        m_title = EnLoadStdString(IDS_BUILD_PEOPLE);
    } else {
        m_title = EnLoadStdString(IDS_BUILD_VEHICLE);
    }
}

// A small filled triangle (blue) on a transparent RGBA surface — up or down.
SDL_Surface* SDL2BuildTransport::MakeArrow(int w, int h, bool up) {
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!s) return nullptr;
    SDL_FillRect(s, nullptr, SDL_MapRGBA(s->format, 0, 0, 0, 0));
    SDL_LockSurface(s);
    Uint32* px = (Uint32*)s->pixels;
    int pitch = s->pitch / 4;
    const Uint32 blue = SDL_MapRGBA(s->format, 64, 132, 255, 255);
    // 2px vertical padding so the glyph doesn't touch the button bevel.
    int pad = 2, gh = h - 2 * pad;
    if (gh < 1) gh = 1;
    for (int row = 0; row < gh; row++) {
        // up: widen toward the bottom; down: widen toward the top.
        float t = up ? (float)row / (gh - 1 > 0 ? gh - 1 : 1)
                     : 1.0f - (float)row / (gh - 1 > 0 ? gh - 1 : 1);
        int half = (int)((w / 2) * t + 0.5f);
        int cx = w / 2;
        int y = pad + row;
        for (int x = cx - half; x <= cx + half; x++)
            if (x >= 0 && x < w && y >= 0 && y < h)
                px[y * pitch + x] = blue;
    }
    SDL_UnlockSurface(s);
    return s;
}

// A small circled-"i": a gold disc with a darker rim and a blue "i" (dot + stem),
// tying the info button to the blue section headers of the status window it opens.
SDL_Surface* SDL2BuildTransport::MakeInfoGlyph(int d) {
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, d, d, 32, SDL_PIXELFORMAT_RGBA32);
    if (!s) return nullptr;
    SDL_FillRect(s, nullptr, SDL_MapRGBA(s->format, 0, 0, 0, 0));
    SDL_LockSurface(s);
    Uint32* px = (Uint32*)s->pixels;
    int pitch = s->pitch / 4;
    const Uint32 gold   = SDL_MapRGBA(s->format, 214, 184, 112, 255);
    const Uint32 border = SDL_MapRGBA(s->format,  70,  54,  28, 255);
    const Uint32 blue   = SDL_MapRGBA(s->format,  40,  60, 150, 255);

    float cx = (d - 1) / 2.0f, cy = (d - 1) / 2.0f;
    float r  = d / 2.0f - 0.5f;
    float r2 = r * r;
    float inner = r - 1.4f; float inner2 = ( inner > 0 ) ? inner * inner : 0;
    for (int y = 0; y < d; y++)
        for (int x = 0; x < d; x++) {
            float dx = x - cx, dy = y - cy;
            float dd = dx * dx + dy * dy;
            if (dd > r2) continue;
            px[y * pitch + x] = ( dd > inner2 ) ? border : gold;
        }

    // the "i": a dot near the top and a stem below, both blue, centred.
    int icx  = d / 2;
    int dotT = (int)(d * 0.26f), dotB = dotT + ( ( d / 8 > 1 ) ? d / 8 : 1 );
    int stemT = (int)(d * 0.44f), stemB = (int)(d * 0.74f);
    auto vbar = [&](int y0, int y1) {
        for (int yy = y0; yy <= y1; yy++)
            for (int xx = icx - 1; xx <= icx; xx++)
                if (xx >= 0 && xx < d && yy >= 0 && yy < d) px[yy * pitch + xx] = blue;
    };
    vbar(dotT, dotB);
    vbar(stemT, stemB);

    SDL_UnlockSurface(s);
    return s;
}

void SDL2BuildTransport::RefreshQty() {
    // Called by the -/+ buttons: push the new count into the edit field and
    // re-render the cost grid (costs scale with quantity).
    if (m_edtNum) m_edtNum->SetText(std::to_string(m_buildNum));
    UpdateDescription();
}

void SDL2BuildTransport::OnQtyEdited(const std::string& text) {
    // The user typed into the count field. Parse + clamp to >= 1, recompute the
    // cost grid, but DON'T rewrite the field here (that would fight live typing).
    int n = atoi(text.c_str());
    if (n < 1) n = 1;
    m_buildNum = n;
    UpdateDescription();
}

void SDL2BuildTransport::SelectVehicle(int idx) {
    if (idx < 0 || idx >= m_numVehs)
        return;
    m_iVehOn = idx;
    m_pTd = m_vehs[idx].pData;
    m_pBu = m_vehs[idx].pBu;
    m_btnBuild->SetEnabled(true);

    for (int i = 0; i < m_numVehs; i++)
        m_vehBtns[i]->SetToggled(i == idx);

    UpdateDescription();
}

void SDL2BuildTransport::UpdateDescription() {
    if (!m_pTd || !m_pBu) {
        m_lblDesc->SetText("");
        m_lblCosts->SetText("");
        m_lblCostCol->SetText("");
        m_lblHaveCol->SetText("");
        m_lblNeedCol->SetText("");
        m_lblOperNames->SetText("");
        m_lblOperVals->SetText("");
        return;
    }

    m_lblDesc->SetText(m_pTd->GetText().c_str());

    // Build cost rows. Lines stay 1:1 across all four column labels so the
    // multi-line blocks align vertically.
    int timeSecs = m_pBu->GetTime() / 24;
    std::string names    = "Time\n";
    std::string costs    = std::to_string(timeSecs * m_buildNum) + "\n";
    std::string haves    = "\n";   // No "have" for time
    std::string deficits = "\n";

    int rows = 1;  // the "Time" row
    for (int i = 0; i < CMaterialTypes::GetNumBuildTypes(); i++) {
        int need = m_pBu->GetInput(i) * m_buildNum;
        if (need > 0) {
            int have = m_pBldg->GetStore(i);
            names += CMaterialTypes::GetDesc(i);
            names += "\n";
            costs += std::to_string(need) + "\n";
            haves += std::to_string(have) + "\n";
            // No parens around the deficit — matches SDL2BuildStructure, and the
            // extra 2 chars made big values wrap the narrow column (row shear).
            int deficit = have - need;
            deficits += (deficit < 0 ? std::to_string(deficit) : "") + "\n";
            rows++;
        }
    }

    m_lblCosts->SetText(names);
    m_lblCostCol->SetText(costs);
    m_lblHaveCol->SetText(haves);
    m_lblNeedCol->SetText(deficits);

    // Colonists (crew) — population needed to operate the vehicle(s), scaled by
    // quantity like the costs. Placed directly under the last cost row with a
    // half-row gap (SDL2BuildStructure's dynamic stacking), clamped so it can
    // never reach the Build button row even with many materials.
    const int lineH = 16;
    int costTop = m_lblCosts->GetRect().y;
    int operTop = costTop + rows * lineH + lineH / 2;
    const int maxTop = costTop + 90 - lineH;   // grid area is 90px tall
    if (operTop > maxTop) operTop = maxTop;
    SDL_Rect rN = m_lblOperNames->GetRect();
    SDL_Rect rV = m_lblOperVals->GetRect();
    m_lblOperNames->SetRect(rN.x, operTop, rN.w, rN.h);
    m_lblOperVals->SetRect(rV.x, operTop, rV.w, rV.h);
    m_lblOperNames->SetText("Colonists");
    m_lblOperVals->SetText(std::to_string(m_pTd->GetPeople() * m_buildNum));
}

void SDL2BuildTransport::OnBuild() {
    if (m_iVehOn < 0 || !m_pTd)
        return;

    int iVehType = m_vehs[m_iVehOn].vehType;

    m_pBldg->ResumeUnit();
    m_pBldg->StartVehicle(iVehType, m_buildNum);

    if (!theGame.AmServer()) {
        CMsgBuildVeh msg(m_pBldg, iVehType);
        theGame.PostToServer(&msg, sizeof(msg));
    }

    EndDialog(1);
}

void SDL2BuildTransport::OnCancel() {
    EndDialog(0);
}
