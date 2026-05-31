#pragma once

#include "SDL2UI.h"

class CVehicleBuilding;
class CTransportData;
class CBuildUnit;

// Native SDL2 replacement for CDlgBuildTransport.
// Opened when double-clicking a vehicle factory or shipyard.

class SDL2BuildTransport : public SDL2Dialog {
public:
    SDL2BuildTransport(GameWindow* gw, CVehicleBuilding* pBldg);
    ~SDL2BuildTransport();

protected:
    void OnInit() override;
    void OnCancel() override;

private:
    void SelectVehicle(int idx);
    void OnBuild();
    void UpdateDescription();
    void RefreshQty();
    void OnQtyEdited(const std::string& text);

    CVehicleBuilding* m_pBldg;

    int m_iVehOn = -1;
    const CTransportData* m_pTd = nullptr;
    const CBuildUnit* m_pBu = nullptr;

    struct VehEntry {
        int vehType;
        const CTransportData* pData;
        const CBuildUnit* pBu;
    };
    VehEntry m_vehs[6];
    int m_numVehs = 0;
    int m_buildNum = 1;

    SDL2Button* m_vehBtns[6] = {};
    SDL2Label*  m_lblDesc = nullptr;

    // Per-column cost labels (proportional-font safe — see SDL2BuildStructure)
    SDL2Label* m_lblCostColHdr = nullptr;   // "cost"
    SDL2Label* m_lblHaveColHdr = nullptr;   // "have"
    SDL2Label* m_lblNeedColHdr = nullptr;   // "need"
    SDL2Label* m_lblCosts      = nullptr;   // names: Time / Lumber / Steel / Colonists
    SDL2Label* m_lblCostCol    = nullptr;   // cost values (right-aligned)
    SDL2Label* m_lblHaveCol    = nullptr;   // have values (right-aligned)
    SDL2Label* m_lblNeedCol    = nullptr;   // deficit values (right-aligned, red if neg)

    SDL2Button*  m_btnBuild = nullptr;
    SDL2EditBox* m_edtNum   = nullptr;   // editable quantity field (+/- also adjust it)

    // Art (from theBitmaps — matches CDlgBuildTransport OnPaint / OnInitDialog)
    SDL_Surface* m_vehBkgnd     = nullptr;  // DIB_VEHICLE_BKGND — purple metallic bg
    SDL_Surface* m_okBtnSheet   = nullptr;  // DIB_VEHICLE_BTNS_1 — Build/Cancel
    SDL_Surface* m_vehBtnSheet  = nullptr;  // DIB_VEHICLE_BTNS_2 — vehicle slot buttons
    SDL_Surface* m_vehIconSheet = nullptr;  // DIB_LIST_UNIT_VEHICLES — vehicle icons
};
