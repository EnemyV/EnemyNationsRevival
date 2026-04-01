#include "stdafx.h"

#include "SDL2BuildStructure.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "area.h"
#include "player.h"
#include "building.inl"
#include "bitmaps.h"
#include "vehicle.inl"

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
        CString catName = theStructureType.GetDesc(i);
        m_catBtns[i] = AddWidget<SDL2Button>(ox + 11, oy + 22 + i * 50, 104, 49,
            (const char*)catName,
            [this, i]() { SelectCategory(i); });
        if (m_catBtnSheet) m_catBtns[i]->SetBtnSheet(m_catBtnSheet);
    }

    // Building buttons (middle column): CRect(129,22,233,71) offset 50px each
    for (int i = 0; i < 6; i++) {
        m_bldgBtns[i] = AddWidget<SDL2Button>(ox + 129, oy + 22 + i * 50, 104, 49, "",
            [this, i]() { SelectBuilding(i); });
        m_bldgBtns[i]->SetEnabled(false);
        if (m_bldgBtnSheet) m_bldgBtns[i]->SetBtnSheet(m_bldgBtnSheet);
    }

    // Description — green text, top-aligned, CRect(252,22,450,144)
    m_lblDesc = AddWidget<SDL2Label>(ox + 252, oy + 22, 198, 122, "");
    m_lblDesc->SetWrapped(true);
    m_lblDesc->SetTopAligned(true);
    m_lblDesc->SetColor({41, 255, 8, 255});

    // Cost header
    m_lblCostHdr = AddWidget<SDL2Label>(ox + 252, oy + 150, 198, 16, "");
    m_lblCostHdr->SetTopAligned(true);
    m_lblCostHdr->SetColor({41, 255, 8, 255});

    // Table grid lines (green, matching original pen color PALETTERGB(41,255,8))
    // Horizontal separator below header
    {
        auto* hLine = AddWidget<SDL2Image>(ox + 264, oy + 167, 175, 1);
        SDL_Surface* s = SDL_CreateRGBSurface(0, 175, 1, 32, 0xFF0000, 0xFF00, 0xFF, 0);
        if (s) { SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 41, 255, 8)); hLine->SetSurface(s, true); }
    }
    // Vertical line: between labels and "cost" column (x=366 in MFC)
    {
        int lineH = 100;  // spans header + cost rows
        auto* vLine = AddWidget<SDL2Image>(ox + 330, oy + 150, 1, lineH);
        SDL_Surface* s = SDL_CreateRGBSurface(0, 1, lineH, 32, 0xFF0000, 0xFF00, 0xFF, 0);
        if (s) { SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 41, 255, 8)); vLine->SetSurface(s, true); }
    }
    // Vertical line: between "cost" and "have" columns (x=402 in MFC)
    {
        int lineH = 100;
        auto* vLine = AddWidget<SDL2Image>(ox + 375, oy + 150, 1, lineH);
        SDL_Surface* s = SDL_CreateRGBSurface(0, 1, lineH, 32, 0xFF0000, 0xFF00, 0xFF, 0);
        if (s) { SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 41, 255, 8)); vLine->SetSurface(s, true); }
    }
    // Vertical line: between "have" and "need" columns (x=439 in MFC)
    {
        int lineH = 100;
        auto* vLine = AddWidget<SDL2Image>(ox + 415, oy + 150, 1, lineH);
        SDL_Surface* s = SDL_CreateRGBSurface(0, 1, lineH, 32, 0xFF0000, 0xFF00, 0xFF, 0);
        if (s) { SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 41, 255, 8)); vLine->SetSurface(s, true); }
    }

    // Cost values — green for build costs
    m_lblCosts = AddWidget<SDL2Label>(ox + 252, oy + 170, 198, 65, "");
    m_lblCosts->SetWrapped(true);
    m_lblCosts->SetTopAligned(true);
    m_lblCosts->SetColor({41, 255, 8, 255});

    // Operating costs — blue PALETTERGB(71,71,225)
    m_lblOper = AddWidget<SDL2Label>(ox + 252, oy + 235, 198, 55, "");
    m_lblOper->SetWrapped(true);
    m_lblOper->SetTopAligned(true);
    m_lblOper->SetColor({71, 71, 225, 255});

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
    m_lblCosts->SetText("");
    m_lblOper->SetText("");

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
            m_bldgBtns[slot]->SetText((const char*)pSd->GetDesc());
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
        m_lblCostHdr->SetText("");
        m_lblCosts->SetText("");
        m_lblOper->SetText("");
        return;
    }

    // Description text (green)
    m_lblDesc->SetText((const char*)m_pSd->GetText());

    // Cost header (separate label above the line)
    m_lblCostHdr->SetText("            cost  have  need");

    // Build costs (green) — below separator line
    std::string costs;
    int timeSecs = m_pSd->GetTimeBuild() / 24;
    costs += "Time        " + std::to_string(timeSecs) + "\n";

    for (int i = 0; i < CMaterialTypes::GetNumBuildTypes(); i++) {
        int need = m_pSd->GetBuild(i);
        if (need > 0) {
            int have = theGame.GetMe()->GetMaterialHave(i);
            CString matName = CMaterialTypes::GetDesc(i);
            std::string line = (const char*)matName;
            while (line.size() < 12) line += ' ';
            line += std::to_string(need);
            while (line.size() < 18) line += ' ';
            line += std::to_string(have);
            int deficit = have - need;
            if (deficit < 0) {
                while (line.size() < 24) line += ' ';
                line += std::to_string(deficit);
            }
            costs += line + "\n";
        }
    }
    m_lblCosts->SetText(costs);

    // Operating costs (blue) — matching original PALETTERGB(71,71,225)
    std::string oper;
    oper += "Operating costs\n";
    {
        std::string line = "Colonists   ";
        line += std::to_string(m_pSd->GetPeople());
        while (line.size() < 18) line += ' ';
        line += std::to_string(theGame.GetMe()->GetPplTotal());
        oper += line + "\n";
    }
    {
        std::string line = "Power       ";
        line += std::to_string(m_pSd->GetPower());
        while (line.size() < 18) line += ' ';
        line += std::to_string(theGame.GetMe()->GetPwrHave());
        oper += line + "\n";
    }
    m_lblOper->SetText(oper);
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
    EndDialog(0);
}
