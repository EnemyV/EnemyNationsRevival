#include "stdafx.h"

#include "SDL2BuildStructure.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "area.h"
#include "player.h"
#include "building.inl"
#include "bitmaps.h"
#include "unit.inl"
#include "vehicle.inl"
#include "netcmd.h"

SDL2BuildStructure::SDL2BuildStructure(GameWindow* gw, CVehicle* pVeh)
    : SDL2Dialog(gw, "Build Structure", 465 + 12, 345 + 12 + 26)
    , m_pVeh(pVeh)
{
    memset(m_bldgs, 0, sizeof(m_bldgs));

    // Load art from the bitmap library (same assets the MFC CDlgBuildStructure uses)
    auto load = [](int idx) -> SDL_Surface* {
        CDIB* p = theBitmaps.GetByIndex(idx);
        return p ? SDL2MainMenu::CreateSurfaceFromDIB(p) : nullptr;
    };
    m_bldgIconSheet = load(DIB_LIST_UNIT_BUILDINGS);
    m_structBkgnd   = load(DIB_STRUCTURE_BKGND);
    m_catBtnSheet   = load(DIB_STRUCTURE_BTNS_1);
    m_bldgBtnSheet  = load(DIB_STRUCTURE_BTNS_2);
    m_okBtnSheet    = load(DIB_STRUCTURE_BTNS_3);
}

SDL2BuildStructure::~SDL2BuildStructure() {
    if (m_bldgIconSheet) SDL_FreeSurface(m_bldgIconSheet);
    if (m_structBkgnd)   SDL_FreeSurface(m_structBkgnd);
    if (m_catBtnSheet)   SDL_FreeSurface(m_catBtnSheet);
    if (m_bldgBtnSheet)  SDL_FreeSurface(m_bldgBtnSheet);
    if (m_okBtnSheet)    SDL_FreeSurface(m_okBtnSheet);
}

bool SDL2BuildStructure::CanBuild(int iCat, const CStructureData* pSd) {
    if (pSd->GetCat() != iCat || !pSd->IsDiscovered())
        return false;
    if (theGame.GetScenario() != -1 && pSd->GetScenario() > theGame.GetScenario())
        return false;

    // Factories: must have at least 1 buildable vehicle
    if (pSd->GetUnionType() == CStructureData::UTvehicle ||
        pSd->GetUnionType() == CStructureData::UTshipyard) {
        CBuildVehicle const* pBv = pSd->GetBldVehicle();
        for (int i = 0; i < pBv->GetSize(); i++) {
            CTransportData const* pTd = theTransports.GetData(pBv->GetUnit(i)->GetVehType());
            if (pTd->IsDiscovered() &&
                (theGame.GetScenario() == -1 || pTd->GetScenario() <= theGame.GetScenario()))
                return true;
        }
        return false;
    }
    return true;
}

void SDL2BuildStructure::OnInit() {
    // Use the structure-specific background art instead of generic DLG_BKGND
    if (m_structBkgnd) SetCustomBackground(m_structBkgnd);

    // Dialog is sized so interior = 465x345 (matching MFC client area exactly).
    // Border ~6px each side, title bar 26px at top of interior.
    // ox/oy = origin of the background art (== MFC client area origin).
    const int bdrSide = 6, bdrTop = 6, titleH = 26;
    int ox = m_x + bdrSide;
    int oy = m_y + bdrTop + titleH;

    // MFC layout (from CDlgBuildStructure::OnInitDialog) — coords map 1:1

    // Category buttons (left column): CRect(11,22,115,71) offset 50px each
    for (int i = 0; i < 6; i++) {
        const char* catName = theStructureType.GetDesc(i);
        m_catBtns[i] = AddWidget<SDL2Button>(ox + 11, oy + 22 + i * 50, 104, 49,
            catName,
            [this, i]() { SelectCategory(i); });
        if (m_catBtnSheet) m_catBtns[i]->SetBtnSheet(m_catBtnSheet);
    }

    // Building buttons (middle column): CRect(129,22,233,71) offset 50px each
    for (int i = 0; i < 6; i++) {
        m_bldgBtns[i] = AddWidget<SDL2Button>(ox + 129, oy + 22 + i * 50, 104, 49, "",
            [this, i]() { SelectBuilding(i); });
        m_bldgBtns[i]->SetOnDblClick(
            [this, i]() { if (m_iBldgOn == i) OnBuild(); });
        m_bldgBtns[i]->SetEnabled(false);
        if (m_bldgBtnSheet) m_bldgBtns[i]->SetBtnSheet(m_bldgBtnSheet);
    }

    // Description — green text, top-aligned, CRect(252,22,450,144)
    m_lblDesc = AddWidget<SDL2Label>(ox + 252, oy + 22, 198, 122, "");
    m_lblDesc->SetWrapped(true);
    m_lblDesc->SetTopAligned(true);
    m_lblDesc->SetColor({41, 255, 8, 255});

    // --- Cost table with per-column labels (proportional-font safe) ---
    // MFC column centers: name ~287, cost ~348, have ~388, need ~427
    // We use left-aligned per-column labels so numbers line up correctly
    // with Book Antiqua (proportional) instead of relying on space padding.

    // Header row: "cost" "have" "need" — right-aligned to match number columns.
    // Starts empty: like the MFC dialog, the whole cost table only appears once a
    // building is selected (CDlgBuildStructure::OnPaint draws it inside
    // `if (m_pSd != NULL)`).
    m_lblCostColHdr = AddWidget<SDL2Label>(ox + 332, oy + 158, 42, 15, "");
    m_lblCostColHdr->SetTopAligned(true);
    m_lblCostColHdr->SetRightAligned(true);
    m_lblCostColHdr->SetColor({41, 255, 8, 255});
    m_lblHaveColHdr = AddWidget<SDL2Label>(ox + 377, oy + 158, 38, 15, "");
    m_lblHaveColHdr->SetTopAligned(true);
    m_lblHaveColHdr->SetRightAligned(true);
    m_lblHaveColHdr->SetColor({41, 255, 8, 255});
    m_lblNeedColHdr = AddWidget<SDL2Label>(ox + 417, oy + 158, 35, 15, "");
    m_lblNeedColHdr->SetTopAligned(true);
    m_lblNeedColHdr->SetRightAligned(true);
    m_lblNeedColHdr->SetColor({41, 255, 8, 255});

    // Grid lines (green). The original draws just ONE vertical divider (between
    // the name column and the value columns) plus one horizontal line under the
    // header — not a line between every column. Both are hidden until a building
    // is selected (see UpdateDescription), matching the MFC `if (m_pSd != NULL)`.
    {
        m_gridH = AddWidget<SDL2Image>(ox + 264, oy + 173, 175, 1);
        SDL_Surface* s = SDL_CreateRGBSurface(0, 175, 1, 32, 0xFF0000, 0xFF00, 0xFF, 0);
        if (s) { SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 41, 255, 8)); m_gridH->SetSurface(s, true); }
        m_gridH->SetVisible(false);
    }
    {
        // Vertical divider. Its length is set per-selection in UpdateDescription
        // to span from the header down to the bottom of the (variable) rows, so
        // it neither overshoots the top nor stops short of the bottom.
        int lineH = 1;
        m_gridV = AddWidget<SDL2Image>(ox + 330, oy + 158, 1, lineH);
        SDL_Surface* s = SDL_CreateRGBSurface(0, 1, lineH, 32, 0xFF0000, 0xFF00, 0xFF, 0);
        if (s) { SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 41, 255, 8)); m_gridV->SetSurface(s, true); }
        m_gridV->SetVisible(false);
    }

    // Cost name column (e.g. "Time", "Gold", "Wood")
    m_lblCosts = AddWidget<SDL2Label>(ox + 255, oy + 175, 72, 95, "");
    m_lblCosts->SetWrapped(true);
    m_lblCosts->SetTopAligned(true);
    m_lblCosts->SetColor({41, 255, 8, 255});

    // Cost value column (right-aligned numbers)
    m_lblCostCol = AddWidget<SDL2Label>(ox + 332, oy + 175, 42, 95, "");
    m_lblCostCol->SetWrapped(true);
    m_lblCostCol->SetTopAligned(true);
    m_lblCostCol->SetRightAligned(true);
    m_lblCostCol->SetColor({41, 255, 8, 255});

    // Have value column
    m_lblHaveCol = AddWidget<SDL2Label>(ox + 377, oy + 175, 38, 95, "");
    m_lblHaveCol->SetWrapped(true);
    m_lblHaveCol->SetTopAligned(true);
    m_lblHaveCol->SetRightAligned(true);
    m_lblHaveCol->SetColor({41, 255, 8, 255});

    // Need/deficit column
    m_lblNeedCol = AddWidget<SDL2Label>(ox + 417, oy + 175, 35, 95, "");
    m_lblNeedCol->SetWrapped(true);
    m_lblNeedCol->SetTopAligned(true);
    m_lblNeedCol->SetRightAligned(true);
    m_lblNeedCol->SetColor({41, 255, 8, 255});

    // Operating costs — name column (blue PALETTERGB(71,71,225)).
    // Shares the same name column as build costs (MFC rect.left=264; "Colonists"
    // is the longest entry and fits in the ~65px gap to the vertical divider).
    // Positioned just below the build-cost rows so it doesn't run into the
    // Build/Cancel buttons at oy+300. MFC stacks these dynamically after the
    // last material row; oy+240 leaves room for 3 materials + half-row gap.
    m_lblOperNames = AddWidget<SDL2Label>(ox + 255, oy + 240, 72, 50, "");
    m_lblOperNames->SetWrapped(true);
    m_lblOperNames->SetTopAligned(true);
    m_lblOperNames->SetColor({71, 71, 225, 255});

    // Operating costs — value column (right-aligned numbers).
    // MUST share the same column as m_lblCostCol so "15" / "4" line up under
    // the green "cost" header. MFC draws all values right-aligned at x=366.
    m_lblOperVals = AddWidget<SDL2Label>(ox + 332, oy + 240, 42, 50, "");
    m_lblOperVals->SetWrapped(true);
    m_lblOperVals->SetTopAligned(true);
    m_lblOperVals->SetRightAligned(true);
    m_lblOperVals->SetColor({71, 71, 225, 255});

    // Build (249,300) 98x23   Cancel (359,300) 98x23
    m_btnBuild = AddWidget<SDL2Button>(ox + 249, oy + 300, 98, 23, "Build",
        [this]() { OnBuild(); });
    m_btnBuild->SetEnabled(false);
    if (m_okBtnSheet) m_btnBuild->SetBtnSheet(m_okBtnSheet);

    auto* btnCancel = AddWidget<SDL2Button>(ox + 359, oy + 300, 98, 23, "Cancel",
        [this]() { OnCancel(); });
    if (m_okBtnSheet) btnCancel->SetBtnSheet(m_okBtnSheet);
}

void SDL2BuildStructure::SelectCategory(int cat) {
    m_iCatOn = cat;
    m_iBldgOn = -1;
    m_pSd = nullptr;
    m_numBldgs = 0;
    m_btnBuild->SetEnabled(false);
    m_lblDesc->SetText("");
    m_lblCostColHdr->SetText("");
    m_lblHaveColHdr->SetText("");
    m_lblNeedColHdr->SetText("");
    m_lblCosts->SetText("");
    m_lblCostCol->SetText("");
    m_lblHaveCol->SetText("");
    m_lblNeedCol->SetText("");
    m_lblOperNames->SetText("");
    m_lblOperVals->SetText("");
    if (m_gridH) m_gridH->SetVisible(false);
    if (m_gridV) m_gridV->SetVisible(false);

    // Toggle selected category (red dot highlight), clear building selection
    for (int i = 0; i < 6; i++) {
        m_catBtns[i]->SetEnabled(true);
        m_catBtns[i]->SetToggled(i == cat);
    }
    for (int i = 0; i < 6; i++)
        m_bldgBtns[i]->SetToggled(false);

    // Populate building buttons for this category
    int slot = 0;
    for (int i = 0; i < theStructures.GetNumBuildings() && slot < 6; i++) {
        CStructureData const* pSd = theStructures.GetData(i);
        if (CanBuild(cat, pSd)) {
            m_bldgs[slot].structureIndex = i;
            m_bldgs[slot].pData = pSd;
            m_bldgBtns[slot]->SetText(pSd->GetDesc().c_str());
            m_bldgBtns[slot]->SetEnabled(true);
            m_bldgBtns[slot]->SetVisible(true);

            // Set building icon from the sprite sheet (64px strips)
            if (m_bldgIconSheet && m_bldgIconSheet->h > 0) {
                int iconW = m_bldgIconSheet->w;
                int iconH = 64;
                int srcY = i * iconH;
                if (srcY + iconH <= m_bldgIconSheet->h) {
                    SDL_Rect iconSrc = {0, srcY, iconW, iconH};
                    m_bldgBtns[slot]->SetIcon(m_bldgIconSheet, iconSrc);
                } else {
                    m_bldgBtns[slot]->ClearIcon();
                }
            }
            slot++;
        }
    }
    m_numBldgs = slot;

    // Clear unused building buttons
    for (int i = slot; i < 6; i++) {
        m_bldgs[i].structureIndex = -1;
        m_bldgs[i].pData = nullptr;
        m_bldgBtns[i]->SetText("");
        m_bldgBtns[i]->SetEnabled(false);
        m_bldgBtns[i]->ClearIcon();
    }
}

void SDL2BuildStructure::SelectBuilding(int idx) {
    if (idx < 0 || idx >= m_numBldgs)
        return;

    m_iBldgOn = idx;
    m_pSd = m_bldgs[idx].pData;
    m_btnBuild->SetEnabled(true);

    // Toggle selected building (red dot highlight)
    for (int i = 0; i < 6; i++)
        m_bldgBtns[i]->SetToggled(i == idx);

    UpdateDescription();
}

void SDL2BuildStructure::UpdateDescription() {
    if (!m_pSd) {
        m_lblDesc->SetText("");
        m_lblCostColHdr->SetText("");
        m_lblHaveColHdr->SetText("");
        m_lblNeedColHdr->SetText("");
        m_lblCosts->SetText("");
        m_lblCostCol->SetText("");
        m_lblHaveCol->SetText("");
        m_lblNeedCol->SetText("");
        m_lblOperNames->SetText("");
        m_lblOperVals->SetText("");
        if (m_gridH) m_gridH->SetVisible(false);
        if (m_gridV) m_gridV->SetVisible(false);
        return;
    }

    // A building is selected — the cost table + grid become visible.
    if (m_gridH) m_gridH->SetVisible(true);
    if (m_gridV) m_gridV->SetVisible(true);

    // Description text (green)
    m_lblDesc->SetText(m_pSd->GetText().c_str());

    // Header row for buy-cost columns
    m_lblCostColHdr->SetText("cost");
    m_lblHaveColHdr->SetText("have");
    m_lblNeedColHdr->SetText("need");

    // Build costs — per-column lines for proportional-font alignment.
    // Lines MUST match 1:1 across the four column labels
    // so the multiline blocks align vertically.
    int timeSecs = m_pSd->GetTimeBuild() / 24;

    std::string names = "Time\n";
    std::string needsCol = std::to_string(timeSecs) + "\n";
    std::string havesCol = "\n";   // No "have" for time
    std::string deficitsCol = "\n";

    int rows = 1;  // the "Time" row
    for (int i = 0; i < CMaterialTypes::GetNumBuildTypes(); i++) {
        int need = m_pSd->GetBuild(i);
        if (need > 0) {
            int have = theGame.GetMe()->GetMaterialHave(i);
            names += CMaterialTypes::GetDesc(i);
            names += "\n";
            needsCol += std::to_string(need) + "\n";
            havesCol += std::to_string(have) + "\n";
            int deficit = have - need;
            deficitsCol += (deficit < 0 ? std::to_string(deficit) : "") + "\n";
            rows++;
        }
    }

    m_lblCosts->SetText(names);
    m_lblCostCol->SetText(needsCol);
    m_lblHaveCol->SetText(havesCol);
    m_lblNeedCol->SetText(deficitsCol);

    // Stack the blue operating-cost text directly below the (variable-length)
    // build-cost rows, like the MFC original which advanced rect.top per row plus
    // a half-row gap. The previous fixed y (oy+240) sat too low for buildings with
    // few materials. ~16px per line matches the size-13 widget font's line skip.
    const int lineH = 16;
    int costTop  = m_lblCosts->GetRect().y;            // == oy + 175
    int operTop  = costTop + rows * lineH + lineH / 2;  // below rows + half-row gap
    SDL_Rect rN = m_lblOperNames->GetRect();
    SDL_Rect rV = m_lblOperVals->GetRect();
    m_lblOperNames->SetRect(rN.x, operTop, rN.w, rN.h);
    m_lblOperVals->SetRect(rV.x, operTop, rV.w, rV.h);

    // Size the vertical divider to span from the header down to the bottom of the
    // two blue operating-cost rows — matching the MFC line that ran y=158..rect.top.
    // (SDL2Image scales aspect-preserving, so recreate the surface at the exact
    // height rather than stretching a fixed one.)
    if (m_gridV) {
        SDL_Rect rv = m_gridV->GetRect();
        int vTop = m_lblCostColHdr->GetRect().y;        // header top
        int vBot = operTop + 2 * lineH;                 // below Colonists + Power
        int vH = vBot - vTop;
        if (vH > 0) {
            SDL_Surface* s = SDL_CreateRGBSurface(0, 1, vH, 32, 0xFF0000, 0xFF00, 0xFF, 0);
            if (s) { SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 41, 255, 8)); m_gridV->SetSurface(s, true); }
            m_gridV->SetRect(rv.x, vTop, 1, vH);
        }
    }

    // Operating costs (blue) — per-column
    std::string operNames;
    operNames += "Colonists\n";
    operNames += "Power";

    std::string operVals;
    operVals += std::to_string(m_pSd->GetPeople()) + "\n";
    operVals += std::to_string(m_pSd->GetPower());

    m_lblOperNames->SetText(operNames);
    m_lblOperVals->SetText(operVals);
}

void SDL2BuildStructure::OnBuild() {
    if (m_iBldgOn < 0 || !m_pSd)
        return;

    int structIndex = m_bldgs[m_iBldgOn].structureIndex;

    // Tell the area window to enter build placement mode
    CWndArea* pWndArea = theAreaList.GetTop();
    if (pWndArea)
        pWndArea->BuildOn(structIndex);

    EndDialog(1);
}

void SDL2BuildStructure::OnCancel() {
    // The original CDlgBuildStructure::OnCancel() just closes the dialog
    // (DestroyWindow()) — it does NOT destroy the construction vehicle. An earlier
    // port misread it as issuing ID_UNIT_DESTROY, which made canceling the build
    // menu kill the selected crane. Cancel simply dismisses the menu.
    EndDialog(0);
}
