#include "stdafx.h"

#include "SDL2GameDialogs.h"
#include "SDL2Panel.h"
#include "SDL2Compositor.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "bitmaps.h"
#include "lastplnt.h"
#include "player.h"
#include "research.h"
#include "relation.h"
#include "building.inl"
#include "vehicle.inl"
#include "unit.inl"
#include "area.h"
#include "netcmd.h"
#include "base.h"

#include <SDL_ttf.h>

// ============================================================================
// SDL2ResearchDialog
// ============================================================================

SDL2ResearchDialog::SDL2ResearchDialog(GameWindow* gw)
    : SDL2Dialog(gw, "Research", 400, 340)
{}

void SDL2ResearchDialog::OnInit() {
    m_list = AddWidget<SDL2Listbox>(m_x + 10, m_y + 34, 170, 240,
        [this](int idx) { SelectItem(idx); });

    m_lblDesc = AddWidget<SDL2Label>(m_x + 190, m_y + 34, 200, 100, "");
    m_lblDesc->SetWrapped(true);

    m_lblProgress = AddWidget<SDL2Label>(m_x + 190, m_y + 140, 200, 40, "");

    m_btnStart = AddWidget<SDL2Button>(m_x + 190, m_y + 190, 100, 28, "Research",
        [this]() { OnStart(); });
    m_btnStart->SetEnabled(false);

    AddWidget<SDL2Button>(m_x + 300, m_y + 190, 90, 28, "Close",
        [this]() { EndDialog(0); });

    PopulateList();

    // Highlight current research
    int curRsrch = theGame.GetMe()->GetRsrchItem();
    for (int i = 0; i < (int)m_items.size(); i++) {
        if (m_items[i].index == curRsrch) {
            m_list->SetSelected(i);
            SelectItem(i);
            break;
        }
    }
}

void SDL2ResearchDialog::PopulateList() {
    m_items.clear();
    m_list->Clear();

    for (int i = 0; i < theRsrch.GetSize(); i++) {
        CRsrchItem const& item = theRsrch[i];

        // Check if prerequisites are met
        bool available = true;
        for (int r = 0; r < item.m_iNumRsrchRequired; r++) {
            if (theGame.GetMe()->GetRsrch(item.m_piRsrchRequired[r]).m_iPtsDiscovered <
                theRsrch[item.m_piRsrchRequired[r]].m_iPtsRequired)
            {
                available = false;
                break;
            }
        }

        // Check building prerequisites
        if (available) {
            for (int b = 0; b < item.m_iNumBldgsRequired; b++) {
                if (!theGame.GetMe()->GetExists(item.m_piBldgsRequired[b])) {
                    available = false;
                    break;
                }
            }
        }

        // Already discovered?
        bool discovered = theGame.GetMe()->GetRsrch(i).m_iPtsDiscovered >= item.m_iPtsRequired;

        if (available || discovered) {
            RsrchEntry entry;
            entry.index = i;
            entry.name = (const char*)item.m_sName;
            entry.available = available && !discovered;

            std::string displayName = entry.name;
            if (discovered) displayName += " [Done]";
            else if (!available) displayName += " [Locked]";

            m_items.push_back(entry);
            m_list->AddItem(displayName);
        }
    }
}

void SDL2ResearchDialog::SelectItem(int idx) {
    if (idx < 0 || idx >= (int)m_items.size()) return;
    m_selected = idx;

    CRsrchItem const& item = theRsrch[m_items[idx].index];
    m_lblDesc->SetText((const char*)item.m_sDesc);

    int discovered = theGame.GetMe()->GetRsrch(m_items[idx].index).m_iPtsDiscovered;
    int required = item.m_iPtsRequired;
    int pct = required > 0 ? (discovered * 100) / required : 100;
    m_lblProgress->SetText("Progress: " + std::to_string(pct) + "%");

    m_btnStart->SetEnabled(m_items[idx].available);
}

void SDL2ResearchDialog::OnStart() {
    if (m_selected < 0 || m_selected >= (int)m_items.size()) return;
    theGame.GetMe()->SetRsrchItem(m_items[m_selected].index);
    EndDialog(1);
}

// ============================================================================
// SDL2RelationsDialog
// ============================================================================

SDL2RelationsDialog::SDL2RelationsDialog(GameWindow* gw)
    : SDL2Dialog(gw, "Relations", 350, 280)
{}

void SDL2RelationsDialog::OnInit() {
    m_list = AddWidget<SDL2Listbox>(m_x + 10, m_y + 34, 150, 180,
        [this](int idx) { SelectPlayer(idx); });

    m_lblInfo = AddWidget<SDL2Label>(m_x + 170, m_y + 34, 170, 40, "");

    // Radio buttons for relation level
    std::vector<std::string> options = {"War", "Neutral", "Peace", "Alliance"};
    m_radRelations = AddWidget<SDL2RadioGroup>(m_x + 170, m_y + 80, 160, 120, options,
        0, [this](int sel) { SetRelation(sel); });
    for (int r = 0; r < 4; r++) m_radRelations->SetEnabled(r, false);

    AddWidget<SDL2Button>(m_x + 170, m_y + 210, 90, 28, "Close",
        [this]() { EndDialog(0); });

    // Populate player list
    POSITION pos = theGame.GetAll().GetHeadPosition();
    while (pos != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        if (pPlr->IsMe()) continue;

        PlayerEntry entry;
        entry.pPlr = pPlr;
        entry.name = (const char*)pPlr->GetName();
        m_players.push_back(entry);
        m_list->AddItem(entry.name);
    }
}

void SDL2RelationsDialog::SelectPlayer(int idx) {
    if (idx < 0 || idx >= (int)m_players.size()) return;
    m_selected = idx;
    CPlayer* pPlr = m_players[idx].pPlr;

    m_lblInfo->SetText(m_players[idx].name);
    // Enable all radio options
    for (int r = 0; r < 4; r++) m_radRelations->SetEnabled(r, true);
    m_radRelations->SetSelected(pPlr->GetRelations());
}

void SDL2RelationsDialog::SetRelation(int level) {
    if (m_selected < 0) return;
    CPlayer* pPlr = m_players[m_selected].pPlr;

    CDlgRelations::NewRelations(pPlr, level);
}

// ============================================================================
// SDL2LoadTruckDialog
// ============================================================================

SDL2LoadTruckDialog::SDL2LoadTruckDialog(GameWindow* gw, CVehicle* pVeh)
    : SDL2Dialog(gw, "Load Truck", 300, 300)
    , m_pVeh(pVeh)
    , m_pBldg(nullptr)
{
    m_pBldg = theBuildingHex._GetBuilding(pVeh->GetPtHead());
}

void SDL2LoadTruckDialog::OnInit() {
    if (!m_pBldg) {
        AddWidget<SDL2Label>(m_x + 10, m_y + 40, 280, 24, "No building found.");
        AddWidget<SDL2Button>(m_x + 100, m_y + 80, 90, 28, "Close",
            [this]() { EndDialog(0); });
        return;
    }

    const char* matNames[] = {"Coal", "Iron", "Lumber", "Oil", "Steel", "Copper"};
    int y = m_y + 36;
    int maxCargo = m_pVeh->GetData()->GetMaxMaterials();

    for (int i = 0; i < 6; i++) {
        int vehHas = m_pVeh->GetStore(i);
        int bldgHas = m_pBldg->GetStore(i);
        int maxVal = vehHas + bldgHas;

        AddWidget<SDL2Label>(m_x + 10, y, 70, 20, matNames[i]);
        m_sliders[i] = AddWidget<SDL2Slider>(m_x + 85, y, 140, 20, 0, maxVal, vehHas);
        m_lblAmounts[i] = AddWidget<SDL2Label>(m_x + 230, y, 60, 20,
            std::to_string(vehHas) + "/" + std::to_string(maxVal));
        y += 28;
    }

    y += 8;
    AddWidget<SDL2Button>(m_x + 10, y, 80, 26, "Load All",
        [this]() { OnLoad(); });
    AddWidget<SDL2Button>(m_x + 100, y, 80, 26, "Unload",
        [this]() { OnUnload(); });
    y += 34;

    AddWidget<SDL2Button>(m_x + 10, y, 80, 26, "OK",
        [this]() {
            // Transfer materials from sliders to vehicle/building
            for (int i = 0; i < 6; i++) {
                int iAmount = m_sliders[i]->GetValue();
                int iTotal = m_pVeh->GetStore(i) + m_pBldg->GetStore(i);
                if (iAmount > iTotal) iAmount = iTotal;
                m_pBldg->SetStore(i, iTotal - iAmount);
                m_pVeh->SetStore(i, iAmount);
            }
            // Tell the building it can build now
            m_pBldg->MaterialMessage();
            m_pBldg->EventOff();
            // Send truck out the exit
            m_pVeh->ExitBuilding();
            // Clear pointer so MFC dialog also goes away
            m_pVeh->NullLoadWindow();
            EndDialog(1);
        });
    AddWidget<SDL2Button>(m_x + 100, y, 80, 26, "Cancel",
        [this]() {
            m_pVeh->NullLoadWindow();
            EndDialog(0);
        });
}

void SDL2LoadTruckDialog::OnLoad() {
    if (!m_pBldg) return;
    int maxCargo = m_pVeh->GetData()->GetMaxMaterials();
    // Load everything we can
    int total = 0;
    for (int i = 0; i < 6; i++)
        total += m_pVeh->GetStore(i) + m_pBldg->GetStore(i);

    for (int i = 0; i < 6; i++) {
        int available = m_pVeh->GetStore(i) + m_pBldg->GetStore(i);
        m_sliders[i]->SetValue(available);
    }
}

void SDL2LoadTruckDialog::OnUnload() {
    for (int i = 0; i < 6; i++)
        m_sliders[i]->SetValue(0);
}

void SDL2LoadTruckDialog::OnAuto() {
    m_pVeh->DumpContents();
    EndDialog(1);
}

// ============================================================================
// SDL2PauseDialog
// ============================================================================

SDL2PauseDialog::SDL2PauseDialog(GameWindow* gw, const std::string& message)
    : SDL2Dialog(gw, "Game Paused", 300, 120)
    , m_message(message)
{}

void SDL2PauseDialog::OnInit() {
    auto* lbl = AddWidget<SDL2Label>(m_x + 10, m_y + 36, 280, 40, m_message);
    lbl->SetCentered(true);

    AddWidget<SDL2Button>(m_x + 105, m_y + 80, 90, 28, "OK",
        [this]() { EndDialog(1); });
}

// ============================================================================
// SDL2DiscoverDialog
// ============================================================================

SDL2DiscoverDialog::SDL2DiscoverDialog(GameWindow* gw,
    const std::string& title, const std::string& description)
    : SDL2Dialog(gw, "Discovery!", 350, 200)
    , m_discTitle(title)
    , m_discDesc(description)
{}

void SDL2DiscoverDialog::OnInit() {
    auto* lblTitle = AddWidget<SDL2Label>(m_x + 10, m_y + 36, 330, 28, m_discTitle);
    lblTitle->SetCentered(true);

    auto* lblDesc = AddWidget<SDL2Label>(m_x + 10, m_y + 68, 330, 80, m_discDesc);
    lblDesc->SetWrapped(true);

    AddWidget<SDL2Button>(m_x + 130, m_y + 160, 90, 28, "OK",
        [this]() { EndDialog(1); });
}

// ============================================================================
// SDL2MessageBox
// ============================================================================

SDL2MessageBox::SDL2MessageBox(GameWindow* gw, const std::string& message, Style style)
    : SDL2Dialog(gw, "Enemy Nations", 360, 140)
    , m_message(message)
    , m_style(style)
{}

void SDL2MessageBox::OnInit() {
    auto* lbl = AddWidget<SDL2Label>(m_x + 15, m_y + 34, m_width - 30, 55, m_message);
    lbl->SetWrapped(true);
    lbl->SetCentered(true);

    int btnW = 80, btnH = 28;
    int btnY = m_y + m_height - btnH - 15;

    if (m_style == YesNoCancel) {
        int spacing = 10;
        int totalW = btnW * 3 + spacing * 2;
        int startX = m_x + (m_width - totalW) / 2;
        AddWidget<SDL2Button>(startX, btnY, btnW, btnH, "Yes",
            [this]() { EndDialog(IDYES); });
        AddWidget<SDL2Button>(startX + btnW + spacing, btnY, btnW, btnH, "No",
            [this]() { EndDialog(IDNO); });
        AddWidget<SDL2Button>(startX + (btnW + spacing) * 2, btnY, btnW, btnH, "Cancel",
            [this]() { EndDialog(IDCANCEL); });
    } else {
        int spacing = 20;
        int totalW = btnW * 2 + spacing;
        int startX = m_x + (m_width - totalW) / 2;
        AddWidget<SDL2Button>(startX, btnY, btnW, btnH, "Yes",
            [this]() { EndDialog(IDYES); });
        AddWidget<SDL2Button>(startX + btnW + spacing, btnY, btnW, btnH, "No",
            [this]() { EndDialog(IDNO); });
    }
}

// ============================================================================
// SDL2UnitInfoPanel — right-click unit tooltip
// ============================================================================

SDL2UnitInfoPanel::SDL2UnitInfoPanel() {
    const char* fonts[] = {"C:\\Windows\\Fonts\\arial.ttf", nullptr};
    for (int i = 0; fonts[i]; i++) {
        FILE* f = fopen(fonts[i], "rb");
        if (f) { fclose(f); m_fontPath = fonts[i]; break; }
    }
}

SDL2UnitInfoPanel::~SDL2UnitInfoPanel() {
    Hide();
    if (m_bgGold) SDL_FreeSurface(m_bgGold);
    if (m_borderH) SDL_FreeSurface(m_borderH);
    if (m_borderV) SDL_FreeSurface(m_borderV);
    for (auto& p : m_fontCache)
        if (p.second) TTF_CloseFont(p.second);
}

void SDL2UnitInfoPanel::LoadArt() {
    if (m_artLoaded) return;
    m_artLoaded = true;

    // DIB_GOLD = 24 (gold background), DIB_BORDER_HORZ = 10, DIB_BORDER_VERT = 11
    CDIB* pGold = theBitmaps.GetByIndex(DIB_GOLD);
    if (pGold) m_bgGold = SDL2MainMenu::CreateSurfaceFromDIB(pGold);

    CDIB* pH = theBitmaps.GetByIndex(DIB_BORDER_HORZ);
    if (pH) m_borderH = SDL2MainMenu::CreateSurfaceFromDIB(pH);

    CDIB* pV = theBitmaps.GetByIndex(DIB_BORDER_VERT);
    if (pV) m_borderV = SDL2MainMenu::CreateSurfaceFromDIB(pV);
}

TTF_Font* SDL2UnitInfoPanel::GetFont(int size) {
    if (m_fontPath.empty()) return nullptr;
    auto it = m_fontCache.find(size);
    if (it != m_fontCache.end()) return it->second;
    TTF_Font* f = TTF_OpenFont(m_fontPath.c_str(), size);
    m_fontCache[size] = f;
    return f;
}

void SDL2UnitInfoPanel::Show(CUnit* pUnit, int screenX, int screenY) {
    if (!pUnit || !theApp.m_gameWindow || !theApp.m_gameWindow->GetCompositor())
        return;

    LoadArt();
    m_pUnit = pUnit;
    BuildContent();

    int borderH = m_borderH ? m_borderH->h : 3;
    int borderV = m_borderV ? m_borderV->w : 3;
    int w = 240;
    int h = borderH * 2 + 8 + (int)m_lines.size() * LINE_HT;

    // Clamp to screen
    int scrW = theApp.m_gameWindow->GetWidth();
    int scrH = theApp.m_gameWindow->GetHeight();
    if (screenX + w > scrW) screenX = scrW - w;
    if (screenY + h > scrH) screenY = scrH - h;
    if (screenX < 0) screenX = 0;
    if (screenY < 0) screenY = 0;

    // Remove old panel if size changed
    if (m_panel) {
        SDL_Surface* surf = m_panel->GetSurface();
        if (surf && (surf->w != w || surf->h != h)) {
            theApp.m_gameWindow->GetCompositor()->RemovePanel(m_panel);
            m_panel = nullptr;
        }
    }

    if (m_panel) {
        m_panel->SetPosition(screenX, screenY);
        m_panel->SetVisible(true);
    } else {
        m_panel = theApp.m_gameWindow->GetCompositor()->AddPanel(
            "unit_info", screenX, screenY, w, h, 40);
        m_panel->SetMovable(false);
        m_panel->SetResizable(false);
        m_panel->SetClosable(false);
        m_panel->SetTitle("");
    }

    Render();
}

void SDL2UnitInfoPanel::Hide() {
    if (m_panel) {
        m_panel->SetVisible(false);
    }
    m_pUnit = nullptr;
}

bool SDL2UnitInfoPanel::IsVisible() const {
    return m_panel && m_panel->IsVisible();
}

void SDL2UnitInfoPanel::Update() {
    if (!m_pUnit || !m_panel || !m_panel->IsVisible()) return;
    BuildContent();
    Render();
}

void SDL2UnitInfoPanel::BuildContent() {
    m_lines.clear();
    if (!m_pUnit) return;

    // Unit name
    m_lines.push_back({(const char*)m_pUnit->GetData()->GetDesc(), false});

    // Vehicle transport status
    if (m_pUnit->GetUnitType() == CUnit::vehicle) {
        CVehicle* pVeh = (CVehicle*)m_pUnit;
        if (pVeh->GetData()->IsTransport()) {
            std::string sText;
            if (!pVeh->IsHpControl())
                sText = CTransportData::m_sAuto.c_str();
            else if (pVeh->GetEvent() == CVehicle::route)
                sText = CTransportData::m_sRoute.c_str();

            CBuilding* pBldg = theBuildingHex.GetBuilding(pVeh->GetPtHead());
            if (pBldg == NULL || pVeh->GetHexOwnership())
                pBldg = theBuildingHex.GetBuilding(pVeh->GetHexDest());
            if (pBldg != NULL && pBldg->GetOwner()->IsMe())
                sText += std::string("[") + (const char*)pBldg->GetData()->GetDesc() + "]";
            else if (pVeh->GetRouteMode() == CVehicle::stop)
                sText += CTransportData::m_sIdle.c_str();
            else
                sText += CTransportData::m_sTravel.c_str();

            m_lines.push_back({sText, false});
        }
    }

    // Damage
    int dmg = __min(99, 100 - m_pUnit->GetDamagePer());
    m_lines.push_back({"Damage: " + std::to_string(dmg) + "%", m_pUnit->GetDamagePer() < 50});

    // Building status text
    if (m_pUnit->GetUnitType() == CUnit::building) {
        CBuilding* pBldg = (CBuilding*)m_pUnit;
        switch (pBldg->GetData()->GetUnionType()) {
        case CStructureData::UTvehicle:
        case CStructureData::UThousing:
        case CStructureData::UTpower:
        case CStructureData::UTresearch:
        case CStructureData::UTrepair:
        case CStructureData::UTmine:
        case CStructureData::UTfarm:
        case CStructureData::UTshipyard: {
            std::string sText;
            pBldg->ShowStatusText(sText);
            m_lines.push_back({sText, m_pUnit->GetDamagePer() < 50});
            break;
        }
        }
    }

    // Materials
    for (int i = 0; i < CMaterialTypes::GetNumTypes(); i++) {
        int iNeed = 0;
        if (i < CMaterialTypes::GetNumBuildTypes() && m_pUnit->GetUnitType() == CUnit::building)
            iNeed = ((CBuilding*)m_pUnit)->GetBldgResReq(i, FALSE);
        if (m_pUnit->GetStore(i) != 0 || iNeed > 0) {
            std::string s = (const char*)CMaterialTypes::GetDesc(i);
            s += ": " + std::to_string(m_pUnit->GetStore(i));
            if (iNeed > 0)
                s += " (" + std::to_string(iNeed) + ")";
            m_lines.push_back({s, false});
        }
    }

    // Carrier cargo
    if (m_pUnit->GetUnitType() == CUnit::vehicle) {
        POSITION pos = ((CVehicle*)m_pUnit)->GetCargoHeadPosition();
        while (pos != NULL) {
            CVehicle* pVeh = ((CVehicle*)m_pUnit)->GetCargoNext(pos);
            m_lines.push_back({(const char*)pVeh->GetData()->GetDesc(), false});
        }
    }

    // Vehicles inside building
    if (m_pUnit->GetUnitType() == CUnit::building) {
        POSITION pos = theVehicleMap.GetStartPosition();
        while (pos != NULL) {
            DWORD dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
            if (pVeh->GetOwner()->IsMe() && !pVeh->GetHexOwnership() &&
                theBuildingHex._GetBuilding(pVeh->GetPtHead()) == m_pUnit)
                m_lines.push_back({(const char*)pVeh->GetData()->GetDesc(), false});
        }
    }
}

void SDL2UnitInfoPanel::Render() {
    if (!m_panel) return;
    SDL_Surface* dst = m_panel->GetSurface();
    if (!dst) return;

    int w = dst->w, h = dst->h;
    int borderH = m_borderH ? m_borderH->h : 3;
    int borderV = m_borderV ? m_borderV->w : 3;

    // --- Gold background (stretched, matching original PaintBorder) ---
    if (m_bgGold) {
        SDL_Rect sr = {0, 0, m_bgGold->w, m_bgGold->h};
        SDL_Rect dr = {0, 0, w, h};
        SDL_BlitScaled(m_bgGold, &sr, dst, &dr);
    } else {
        SDL_FillRect(dst, nullptr, SDL_MapRGB(dst->format, 160, 140, 90));
    }

    // --- Border: horizontal bars top/bottom ---
    if (m_borderH) {
        // Top border (tile across width)
        for (int tx = 0; tx < w; tx += m_borderH->w) {
            int bw = __min(m_borderH->w, w - tx);
            SDL_Rect sr = {0, 0, bw, m_borderH->h};
            SDL_Rect dr = {tx, 0, bw, m_borderH->h};
            SDL_BlitSurface(m_borderH, &sr, dst, &dr);
        }
        // Bottom border
        for (int tx = 0; tx < w; tx += m_borderH->w) {
            int bw = __min(m_borderH->w, w - tx);
            SDL_Rect sr = {0, 0, bw, m_borderH->h};
            SDL_Rect dr = {tx, h - m_borderH->h, bw, m_borderH->h};
            SDL_BlitSurface(m_borderH, &sr, dst, &dr);
        }
    }

    // --- Border: vertical bars left/right (beveled inward) ---
    if (m_borderV) {
        for (int ix = 0; ix < m_borderV->w && ix < w / 2; ix++) {
            int top = borderH + ix;
            int bot = h - borderH - ix;
            if (top >= bot) break;
            // Left side
            SDL_Rect sr = {ix, ix, 1, m_borderV->h};
            for (int ty = top; ty < bot; ty += m_borderV->h) {
                int bh = __min(m_borderV->h, bot - ty);
                SDL_Rect sr2 = {ix, ix, 1, bh};
                SDL_Rect dr = {ix, ty, 1, bh};
                SDL_BlitSurface(m_borderV, &sr2, dst, &dr);
            }
            // Right side
            for (int ty = top; ty < bot; ty += m_borderV->h) {
                int bh = __min(m_borderV->h, bot - ty);
                SDL_Rect sr2 = {ix, ix, 1, bh};
                SDL_Rect dr = {w - 1 - ix, ty, 1, bh};
                SDL_BlitSurface(m_borderV, &sr2, dst, &dr);
            }
        }
    }

    // --- Text (matching original: gray shadow at +1,+1, then black or red) ---
    TTF_Font* font = GetFont(11);
    if (!font) { m_panel->SetDirty(); return; }

    int textX = borderV + 4;
    int textMaxW = w - textX * 2;
    int y = borderH + 4;

    for (auto& line : m_lines) {
        if (line.text.empty()) { y += LINE_HT; continue; }

        SDL_Color shadow = {128, 128, 128, 255};
        SDL_Color fg = line.red ? SDL_Color{255, 50, 27, 255} : SDL_Color{0, 0, 0, 255};

        SDL_Surface* ts = TTF_RenderText_Blended(font, line.text.c_str(), shadow);
        if (ts) {
            SDL_Rect sr = {0, 0, __min(ts->w, textMaxW), ts->h};
            SDL_Rect dr = {textX + 1, y + 1, sr.w, sr.h};
            SDL_BlitSurface(ts, &sr, dst, &dr);
            SDL_FreeSurface(ts);
        }
        ts = TTF_RenderText_Blended(font, line.text.c_str(), fg);
        if (ts) {
            SDL_Rect sr = {0, 0, __min(ts->w, textMaxW), ts->h};
            SDL_Rect dr = {textX, y, sr.w, sr.h};
            SDL_BlitSurface(ts, &sr, dst, &dr);
            SDL_FreeSurface(ts);
        }
        y += LINE_HT;
    }

    m_panel->SetDirty();
}

// ============================================================================
// SDL2ChatWindow — multiplayer chat
// ============================================================================

SDL2ChatWindow::SDL2ChatWindow(GameWindow* gw)
    : SDL2Dialog(gw, "Chat", 350, 300)
{}

void SDL2ChatWindow::OnInit() {
    // Message history
    m_msgList = AddWidget<SDL2Listbox>(m_x + 10, m_y + 36, m_width - 20, 200);

    // Input field
    AddWidget<SDL2Label>(m_x + 10, m_y + 240, 50, 24, "Say:");
    m_editMsg = AddWidget<SDL2EditBox>(m_x + 60, m_y + 240, m_width - 130, 24);

    AddWidget<SDL2Button>(m_x + m_width - 60, m_y + 240, 50, 24, "Send",
        [this]() { OnSend(); });

    AddWidget<SDL2Button>(m_x + m_width / 2 - 40, m_y + m_height - 36, 80, 28, "Close",
        [this]() { EndDialog(0); });

    RefreshMessages();
}

void SDL2ChatWindow::OnSend() {
    if (!m_editMsg) return;
    std::string msg = m_editMsg->GetText();
    if (msg.empty()) return;

    // Send via the MFC chat window (it handles network messaging)
    if (theGame.IsNetGame() && theApp.m_wndChat.m_hWnd != NULL) {
        // Post the message text to the MFC chat system
        theApp.m_wndChat.PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), 0);
    }

    m_editMsg->SetText("");
    RefreshMessages();
}

void SDL2ChatWindow::RefreshMessages() {
    // Chat messages are managed by the MFC CWndComm system
    // Just show a placeholder until messages are available
    if (!m_msgList) return;
    m_msgList->Clear();
    m_msgList->AddItem("(Chat messages appear here)");
}

// ============================================================================
// SDL2PlayerListDialog — in-game player list
// ============================================================================

SDL2PlayerListDialog::SDL2PlayerListDialog(GameWindow* gw)
    : SDL2Dialog(gw, "Players", 360, 320)
{}

void SDL2PlayerListDialog::OnInit() {
    m_list = AddWidget<SDL2Listbox>(m_x + 10, m_y + 36, 200, 240);
    m_lblInfo = AddWidget<SDL2Label>(m_x + 220, m_y + 36, 130, 240, "");
    m_lblInfo->SetWrapped(true);

    AddWidget<SDL2Button>(m_x + m_width / 2 - 40, m_y + m_height - 36, 80, 28, "Close",
        [this]() { EndDialog(0); });

    PopulateList();

    // Show info for all players in the info label
    std::string allInfo;
    POSITION pos2 = theGame.GetAll().GetHeadPosition();
    while (pos2 != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos2);
        if (!pPlr) continue;
        allInfo += (const char*)pPlr->GetName();
        if (pPlr->IsMe()) allInfo += " (You)";
        else if (pPlr->IsAI()) allInfo += " (AI)";
        allInfo += "\n";
    }
    m_lblInfo->SetText(allInfo);
}

void SDL2PlayerListDialog::PopulateList() {
    if (!m_list) return;
    m_list->Clear();

    POSITION pos = theGame.GetAll().GetHeadPosition();
    while (pos != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        if (pPlr)
            m_list->AddItem((const char*)pPlr->GetName());
    }
}

// ============================================================================
// SDL2ComposeDialog — multiplayer message compose
// ============================================================================

SDL2ComposeDialog::SDL2ComposeDialog(GameWindow* gw)
    : SDL2Dialog(gw, "Compose Message", 380, 340)
{}

void SDL2ComposeDialog::OnInit() {
    int lx = m_x + 10;
    int y = m_y + 36;
    int w = m_width - 20;

    AddWidget<SDL2Label>(lx, y, 80, 20, "To:");
    m_recipientList = AddWidget<SDL2Listbox>(lx + 30, y, w - 30, 80);

    // Populate recipients
    POSITION pos = theGame.GetAll().GetHeadPosition();
    while (pos != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        if (pPlr && !pPlr->IsMe() && !pPlr->IsAI())
            m_recipientList->AddItem((const char*)pPlr->GetName());
    }

    y += 90;
    AddWidget<SDL2Label>(lx, y, 80, 20, "Subject:");
    m_editSubject = AddWidget<SDL2EditBox>(lx + 60, y, w - 60, 22);

    y += 30;
    AddWidget<SDL2Label>(lx, y, 80, 20, "Message:");
    m_editBody = AddWidget<SDL2EditBox>(lx, y + 22, w, 80);

    y += 110;
    int btnW = 80, btnH = 28;
    AddWidget<SDL2Button>(m_x + m_width / 2 - btnW - 10, y, btnW, btnH, "Send",
        [this]() { OnSend(); });
    AddWidget<SDL2Button>(m_x + m_width / 2 + 10, y, btnW, btnH, "Cancel",
        [this]() { EndDialog(0); });
}

void SDL2ComposeDialog::OnSend() {
    // Send message via the MFC chat system
    if (m_editBody && theGame.IsNetGame() && theApp.m_wndChat.m_hWnd != NULL) {
        theApp.m_wndChat.PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), 0);
    }
    EndDialog(1);
}

// ============================================================================
// SDL2CutSceneDialog — win/lose/scenario text screen
// ============================================================================

SDL2CutSceneDialog::SDL2CutSceneDialog(GameWindow* gw, int typ, const std::string& text, int scenario)
    : SDL2Dialog(gw, "", 600, 400)
    , m_typ(typ)
    , m_text(text)
    , m_scenario(scenario)
{
    // No title bar text — the content IS the message
}

void SDL2CutSceneDialog::OnInit() {
    // Large text area — most of the dialog
    int textH = m_height - 100;
    SDL2Label* lbl = AddWidget<SDL2Label>(m_x + 20, m_y + 20, m_width - 40, textH, m_text,
                                          SDL_Color{220, 200, 160, 255});
    lbl->SetWrapped(true);

    // Buttons at the bottom
    int btnY = m_y + m_height - 46;
    int btnW = 90, btnH = 32;

    // cut=0, repeat=1, scenario_end=2, win=3, lose=4
    bool isEndScreen = (m_typ == 2 || m_typ == 3 || m_typ == 4);
    bool canCancel   = (m_typ == 0);  // cut (not repeat, not end screens)
    bool canSave     = (m_typ == 0) && (m_scenario > 0);  // first scenario has no save

    if (isEndScreen) {
        // Just a Continue button centred
        AddWidget<SDL2Button>(m_x + m_width/2 - btnW/2, btnY, btnW, btnH, "Continue",
            [this]() { OnOK(); });
    } else {
        int bx = m_x + 20;
        AddWidget<SDL2Button>(bx, btnY, btnW, btnH, "OK",
            [this]() { OnOK(); });
        bx += btnW + 10;

        if (canCancel) {
            AddWidget<SDL2Button>(bx, btnY, btnW, btnH, "Cancel",
                [this]() { OnCancel(); });
            bx += btnW + 10;
        }
        if (canSave) {
            AddWidget<SDL2Button>(bx, btnY, btnW, btnH, "Save",
                [this]() { OnSave(); });
        }
    }
}

void SDL2CutSceneDialog::OnOK() {
    m_result = 1; // CUT_OK
    EndDialog(1);
}

void SDL2CutSceneDialog::OnCancel() {
    m_result = 2; // CUT_CANCEL
    EndDialog(2);
}

void SDL2CutSceneDialog::OnSave() {
    // Save game, then continue
    theGame.SaveGame(NULL);
    m_result = 1; // CUT_OK (continue after save)
    EndDialog(1);
}
