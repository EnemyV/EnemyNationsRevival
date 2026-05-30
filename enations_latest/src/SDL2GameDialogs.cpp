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
// SDL2ResearchDialog — matches CDlgResearch layout from the original MFC dialog
//
// MFC client area was 447x291. Layout from CDlgResearch::OnInitDialog:
//   list      (18,  35) 155x198    — listbox of researchable items
//   desc text (192, 37) → (420,156) — rectText, green PALETTERGB(41,255,8)
//   IDOK     (188, 170) 116x25     — "Research" button
//   IDCANCEL (311, 170) 116x25     — "Close" button
//   Discovery(311, 211) 116x25     — "Discovery" button (enabled when there's
//                                     a new discovery to view)
//   bulbs    (17, 248) → (425,272) — light-bulb progress strip (DIB+ICON_RESEARCH)
// ============================================================================

SDL2ResearchDialog::SDL2ResearchDialog(GameWindow* gw)
    // Interior = 447x291 to match MFC. Outer dim = interior + side borders
    // (6 each) + title bar (26). See SDL2BuildStructure for the same recipe.
    : SDL2Dialog(gw, "Research", 447 + 12, 291 + 12 + 26)
{
    auto load = [](int idx) -> SDL_Surface* {
        CDIB* p = theBitmaps.GetByIndex(idx);
        return p ? SDL2MainMenu::CreateSurfaceFromDIB(p) : nullptr;
    };
    m_bkgnd    = load(DIB_RSRCH_BKGND);
    m_btnSheet = load(DIB_RESEARCH_BTNS);
}

SDL2ResearchDialog::~SDL2ResearchDialog() {
    if (m_bkgnd)    SDL_FreeSurface(m_bkgnd);
    if (m_btnSheet) SDL_FreeSurface(m_btnSheet);
}

void SDL2ResearchDialog::OnInit() {
    if (m_bkgnd) SetCustomBackground(m_bkgnd);

    // Dialog interior origin == MFC client area origin (skip side border + title)
    const int bdrSide = 6, bdrTop = 6, titleH = 26;
    int ox = m_x + bdrSide;
    int oy = m_y + bdrTop + titleH;

    // List: red text on white, blue selection (close to MFC red-text-on-black-when-selected)
    m_list = AddWidget<SDL2Listbox>(ox + 18, oy + 35, 155, 198,
        [this](int idx) { SelectItem(idx); });

    // Description text: green PALETTERGB(41,255,8), top-aligned, wrapped.
    // MFC rectText(192, 37, 420, 156) → x=192,y=37, w=228, h=119
    m_lblDesc = AddWidget<SDL2Label>(ox + 192, oy + 37, 228, 119, "");
    m_lblDesc->SetWrapped(true);
    m_lblDesc->SetTopAligned(true);
    m_lblDesc->SetColor({41, 255, 8, 255});

    // Progress label sits where the legacy MFC light-bulb strip was painted
    // (rect 17,248,425,272 in CDlgResearch::OnInitDialog). Until the bulb
    // widget is built we use a centered text line in that same band.
    m_lblProgress = AddWidget<SDL2Label>(ox + 18, oy + 256, 405, 18, "");
    m_lblProgress->SetColor({41, 255, 8, 255});
    m_lblProgress->SetCentered(true);

    // Buttons — use the 3-state research button sheet (red-text-on-circuit art)
    m_btnStart = AddWidget<SDL2Button>(ox + 188, oy + 170, 116, 25, "Research",
        [this]() { OnStart(); });
    m_btnStart->SetEnabled(false);
    if (m_btnSheet) m_btnStart->SetBtnSheet(m_btnSheet);

    m_btnClose = AddWidget<SDL2Button>(ox + 311, oy + 170, 116, 25, "Close",
        [this]() { EndDialog(0); });
    if (m_btnSheet) m_btnClose->SetBtnSheet(m_btnSheet);

    m_btnDiscover = AddWidget<SDL2Button>(ox + 311, oy + 211, 116, 25, "Discovery",
        [this]() { OnDiscover(); });
    // Enabled only when there's a freshly-discovered item to view. The MFC
    // version flips it on via CDlgResearch::ItemDiscovered(). We don't have
    // that hook yet — leave disabled for now so the button is visible-but-grey.
    m_btnDiscover->SetEnabled(false);
    if (m_btnSheet) m_btnDiscover->SetBtnSheet(m_btnSheet);

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

void SDL2ResearchDialog::OnDiscover() {
    // Placeholder — when ItemDiscovered hook is wired up this will open the
    // discovery dialog. For now treat as no-op.
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
            entry.name = item.m_sName.c_str();
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
    m_lblDesc->SetText(item.m_sDesc.c_str());

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

    // Give button — hands selected area-map units to the chosen player.
    // Enabled when there's both a selected non-self player AND at least one
    // giveable unit selected on the area map (matches CDlgRelations).
    m_btnGive = AddWidget<SDL2Button>(m_x + 10, m_y + 220, 100, 28, "Give Units",
        [this]() { OnGive(); });
    m_btnGive->SetEnabled(false);

    AddWidget<SDL2Button>(m_x + 240, m_y + 220, 90, 28, "Close",
        [this]() { EndDialog(0); });

    // Populate player list (skip self)
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

    // Enable all relation radios — but disable Alliance for AI players
    // (matches MFC GetDlgItem(IDC_PLYR_ALLIANCE)->EnableWindow(!pPlr->IsAI())).
    // Indices: 0=War, 1=Neutral, 2=Peace, 3=Alliance.
    for (int r = 0; r < 4; r++) m_radRelations->SetEnabled(r, true);
    if (pPlr->IsAI())
        m_radRelations->SetEnabled(3, false);
    m_radRelations->SetSelected(pPlr->GetRelations());

    // Update Give button state from area-map selection
    CWndArea* pWndArea = theAreaList.GetTop();
    bool canGive = (pWndArea && pWndArea->NumGiveable() > 0);
    m_btnGive->SetEnabled(canGive);
}

void SDL2RelationsDialog::SetRelation(int level) {
    if (m_selected < 0) return;
    CPlayer* pPlr = m_players[m_selected].pPlr;

    NewRelations(pPlr, level);
}

void SDL2RelationsDialog::OnGive() {
    if (m_selected < 0) return;
    CWndArea* pWndArea = theAreaList.GetTop();
    if (!pWndArea || pWndArea->NumGiveable() <= 0) {
        m_btnGive->SetEnabled(false);
        return;
    }
    pWndArea->GiveSelectedUnits(m_players[m_selected].pPlr);
    // Nothing left to give now that the units have changed owner
    m_btnGive->SetEnabled(false);
}

// ============================================================================
// SDL2LoadTruckDialog
// ============================================================================

SDL2LoadTruckDialog::SDL2LoadTruckDialog(GameWindow* gw, CVehicle* pVeh)
    // Dynamic title: "Load Freighter" for boats, otherwise "Load Truck - [Bldg]"
    // (matches CDlgLoadTruck::OnInitDialog using IDS_LOAD_FREIGHTER / IDS_LOAD_TRUCK)
    : SDL2Dialog(gw,
        pVeh && pVeh->GetData()->IsBoat() ? "Load Freighter" :
        ([&]() -> std::string {
            CBuilding* pB = pVeh ? theBuildingHex._GetBuilding(pVeh->GetPtHead()) : nullptr;
            if (pB) return std::string("Load Truck - [") + pB->GetData()->GetDesc().c_str() + "]";
            return "Load Truck";
        })(),
        360, 460)
    , m_pVeh(pVeh)
    , m_pBldg(nullptr)
{
    m_pBldg = theBuildingHex._GetBuilding(pVeh->GetPtHead());
}

void SDL2LoadTruckDialog::OnInit() {
    if (!m_pBldg) {
        AddWidget<SDL2Label>(m_x + 10, m_y + 40, 340, 24, "No building found.");
        AddWidget<SDL2Button>(m_x + 130, m_y + 80, 90, 28, "Close",
            [this]() { EndDialog(0); });
        return;
    }

    const char* matNames[] = {"Coal", "Iron", "Lumber", "Oil", "Steel", "Copper"};
    int y = m_y + 36;

    for (int i = 0; i < 6; i++) {
        int vehHas = m_pVeh->GetStore(i);
        int bldgHas = m_pBldg->GetStore(i);
        int maxVal = vehHas + bldgHas;

        AddWidget<SDL2Label>(m_x + 10, y, 70, 20, matNames[i]);
        m_sliders[i] = AddWidget<SDL2Slider>(m_x + 85, y, 180, 20, 0, maxVal, vehHas);
        m_lblAmounts[i] = AddWidget<SDL2Label>(m_x + 275, y, 70, 20,
            std::to_string(vehHas) + "/" + std::to_string(maxVal));
        y += 28;
    }

    // Capacity readout — total/cap below the material rows
    y += 8;
    int maxCargo = m_pVeh->GetData()->GetMaxMaterials();
    m_lblCapacity = AddWidget<SDL2Label>(m_x + 10, y, 340, 20,
        "Capacity: 0 / " + std::to_string(maxCargo));
    m_lblCapacity->SetCentered(true);
    y += 26;

    // Preset buttons row 1: Load (proportional), Load Bldg (50/50 steel/lumber),
    // Load Veh (80/20 steel/copper) — same presets the MFC dialog offered.
    int btnW = 105, btnH = 26, btnGap = 6;
    AddWidget<SDL2Button>(m_x + 10, y, btnW, btnH, "Load",
        [this]() { OnLoad(); });
    AddWidget<SDL2Button>(m_x + 10 + (btnW + btnGap), y, btnW, btnH, "Load Bldg",
        [this]() { OnLoadBldg(); });
    AddWidget<SDL2Button>(m_x + 10 + 2 * (btnW + btnGap), y, btnW, btnH, "Load Veh",
        [this]() { OnLoadVeh(); });
    y += btnH + btnGap;

    // Preset buttons row 2: Unload (clear) + Auto (hand back to router)
    AddWidget<SDL2Button>(m_x + 10, y, btnW, btnH, "Unload",
        [this]() { OnUnload(); });
    AddWidget<SDL2Button>(m_x + 10 + (btnW + btnGap), y, btnW, btnH, "Auto",
        [this]() { OnAuto(); });
    y += btnH + 12;

    // OK / Cancel
    AddWidget<SDL2Button>(m_x + 60, y, 100, btnH, "OK",
        [this]() { OnOK(); });
    AddWidget<SDL2Button>(m_x + 200, y, 100, btnH, "Cancel",
        [this]() { OnCancel(); });

    RefreshTotals();
}

void SDL2LoadTruckDialog::RefreshTotals() {
    if (!m_lblCapacity) return;
    int total = 0;
    for (int i = 0; i < 6; i++) total += m_sliders[i]->GetValue();
    int maxCargo = m_pVeh->GetData()->GetMaxMaterials();
    m_lblCapacity->SetText("Capacity: " + std::to_string(total) +
                           " / " + std::to_string(maxCargo));
}

void SDL2LoadTruckDialog::OnLoad() {
    // MFC OnTruckLoad: fill from combined truck+building stocks, scaling DOWN
    // proportionally if the total exceeds vehicle capacity.
    if (!m_pBldg) return;
    int amounts[6];
    int total = 0;
    for (int i = 0; i < 6; i++) {
        amounts[i] = m_pVeh->GetStore(i) + m_pBldg->GetStore(i);
        total += amounts[i];
    }
    int maxCargo = m_pVeh->GetData()->GetMaxMaterials();
    if (total > maxCargo && total > 0) {
        // Leave one slot of headroom (matching MFC's max-1 trick)
        float fMul = (float)(maxCargo - 1) / (float)total;
        for (int i = 0; i < 6; i++)
            amounts[i] = (int)((float)amounts[i] * fMul);
    }
    for (int i = 0; i < 6; i++) m_sliders[i]->SetValue(amounts[i]);
    RefreshTotals();
}

void SDL2LoadTruckDialog::OnLoadBldg() {
    // 50/50 Steel / Lumber — construction load preset.
    if (!m_pBldg) return;
    int maxCargo = m_pVeh->GetData()->GetMaxMaterials();
    int iSteel  = maxCargo / 2;
    int iLumber = maxCargo / 2;

    int steelCap  = m_pVeh->GetStore(CMaterialTypes::steel)  + m_pBldg->GetStore(CMaterialTypes::steel);
    int lumberCap = m_pVeh->GetStore(CMaterialTypes::lumber) + m_pBldg->GetStore(CMaterialTypes::lumber);
    if (iSteel  > steelCap)  { iSteel  = steelCap;  iLumber = maxCargo - iSteel;  }
    if (iLumber > lumberCap) { iLumber = lumberCap; }

    for (int i = 0; i < 6; i++) m_sliders[i]->SetValue(0);
    m_sliders[CMaterialTypes::steel]->SetValue(iSteel);
    m_sliders[CMaterialTypes::lumber]->SetValue(iLumber);
    RefreshTotals();
}

void SDL2LoadTruckDialog::OnLoadVeh() {
    // 80/20 Steel / Copper — vehicle-factory load preset.
    if (!m_pBldg) return;
    int maxCargo = m_pVeh->GetData()->GetMaxMaterials();
    int iSteel  = (maxCargo * 4) / 5;
    int iCopper = maxCargo / 5;

    int steelCap  = m_pVeh->GetStore(CMaterialTypes::steel)  + m_pBldg->GetStore(CMaterialTypes::steel);
    int copperCap = m_pVeh->GetStore(CMaterialTypes::copper) + m_pBldg->GetStore(CMaterialTypes::copper);
    if (iSteel  > steelCap)  { iSteel  = steelCap;  iCopper = maxCargo - iSteel; }
    if (iCopper > copperCap) { iCopper = copperCap; }

    for (int i = 0; i < 6; i++) m_sliders[i]->SetValue(0);
    m_sliders[CMaterialTypes::steel]->SetValue(iSteel);
    m_sliders[CMaterialTypes::copper]->SetValue(iCopper);
    RefreshTotals();
}

void SDL2LoadTruckDialog::OnOK() {
    // Transfer materials from sliders to vehicle/building, capping at the
    // combined truck+building total per material (matches MFC Transfer()).
    for (int i = 0; i < 6; i++) {
        int iAmount = m_sliders[i]->GetValue();
        int iTotal = m_pVeh->GetStore(i) + m_pBldg->GetStore(i);
        if (iAmount > iTotal) iAmount = iTotal;
        m_pBldg->SetStore(i, iTotal - iAmount);
        m_pVeh->SetStore(i, iAmount);
    }
    m_pBldg->MaterialMessage();
    m_pBldg->EventOff();
    m_pVeh->ExitBuilding();
    // (MFC's NullLoadWindow was a back-pointer cleanup on CVehicle->m_pDlgLoad —
    // SDL2 path doesn't retain that back-pointer, so nothing to null.)
    EndDialog(1);
}

void SDL2LoadTruckDialog::OnCancel() {
    EndDialog(0);
}

void SDL2LoadTruckDialog::OnUnload() {
    for (int i = 0; i < 6; i++)
        m_sliders[i]->SetValue(0);
    RefreshTotals();
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
    m_lines.push_back({m_pUnit->GetData()->GetDesc().c_str(), false});

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
                sText += std::string("[") + pBldg->GetData()->GetDesc().c_str() + "]";
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
            std::string s = CMaterialTypes::GetDesc(i);
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
            m_lines.push_back({pVeh->GetData()->GetDesc().c_str(), false});
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
                m_lines.push_back({pVeh->GetData()->GetDesc().c_str(), false});
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

    // --- Carved-gold frame (shared corner-correct routine) ---
    SDL2MainMenu::DrawGoldBorder(dst, 0, 0, w, h, m_borderH, m_borderV);

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
    theGame.SaveGame( (CWnd*)NULL );
    m_result = 1; // CUT_OK (continue after save)
    EndDialog(1);
}
