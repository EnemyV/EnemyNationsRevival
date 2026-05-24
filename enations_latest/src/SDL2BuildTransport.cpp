#include "stdafx.h"

#include "SDL2BuildTransport.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "player.h"
#include "building.h"
#include "building.inl"
#include "vehicle.inl"
#include "bitmaps.h"
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

    m_lblNeedColHdr = AddWidget<SDL2Label>(ox + 318, oy + 142, 42, 16, "need");
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

    m_lblNeedCol = AddWidget<SDL2Label>(ox + 318, oy + 164, 42, 90, "");
    m_lblNeedCol->SetWrapped(true);
    m_lblNeedCol->SetTopAligned(true);
    m_lblNeedCol->SetRightAligned(true);
    m_lblNeedCol->SetColor({255, 41, 8, 255});  // red — only shows when deficit

    // --- Build / Cancel buttons --------------------------------------------
    m_btnBuild = AddWidget<SDL2Button>(ox + 133, oy + 258, 84, 23, "Build",
        [this]() { OnBuild(); });
    m_btnBuild->SetEnabled(false);
    if (m_okBtnSheet) m_btnBuild->SetBtnSheet(m_okBtnSheet);

    auto* btnCancel = AddWidget<SDL2Button>(ox + 282, oy + 258, 84, 23, "Cancel",
        [this]() { OnCancel(); });
    if (m_okBtnSheet) btnCancel->SetBtnSheet(m_okBtnSheet);

    // --- Quantity control --------------------------------------------------
    // MFC used an edit box + spinner at (282, 295). We provide -/+ buttons
    // flanking a count label in the same horizontal band.
    m_lblNum = AddWidget<SDL2Label>(ox + 240, oy + 295, 40, 22,
        std::to_string(m_buildNum));
    m_lblNum->SetCentered(true);
    m_lblNum->SetColor({41, 255, 8, 255});

    AddWidget<SDL2Button>(ox + 220, oy + 295, 20, 22, "-",
        [this]() {
            if (m_buildNum > 1) { m_buildNum--; RefreshQty(); }
        });
    AddWidget<SDL2Button>(ox + 285, oy + 295, 20, 22, "+",
        [this]() { m_buildNum++; RefreshQty(); });

    // Auto-select first vehicle if available
    if (m_numVehs > 0)
        SelectVehicle(0);
}

void SDL2BuildTransport::RefreshQty() {
    if (m_lblNum) m_lblNum->SetText(std::to_string(m_buildNum));
    // Cost values depend on quantity, so re-render the grid
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

    for (int i = 0; i < CMaterialTypes::GetNumBuildTypes(); i++) {
        int need = m_pBu->GetInput(i) * m_buildNum;
        if (need > 0) {
            int have = m_pBldg->GetStore(i);
            names += CMaterialTypes::GetDesc(i);
            names += "\n";
            costs += std::to_string(need) + "\n";
            haves += std::to_string(have) + "\n";
            int deficit = have - need;
            deficits += (deficit < 0 ? "(" + std::to_string(deficit) + ")" : "") + "\n";
        }
    }

    m_lblCosts->SetText(names);
    m_lblCostCol->SetText(costs);
    m_lblHaveCol->SetText(haves);
    m_lblNeedCol->SetText(deficits);
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
