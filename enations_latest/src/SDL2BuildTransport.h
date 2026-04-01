#pragma once

#include "SDL2UI.h"

class CVehicleBuilding;
class CTransportData;

// Native SDL2 replacement for CDlgBuildTransport.
// Opened when double-clicking a vehicle factory or shipyard.

class SDL2BuildTransport : public SDL2Dialog {
public:
    SDL2BuildTransport(GameWindow* gw, CVehicleBuilding* pBldg);

protected:
    void OnInit() override;
    void OnCancel() override;

private:
    void SelectVehicle(int idx);
    void OnBuild();
    void UpdateDescription();

    CVehicleBuilding* m_pBldg;

    int m_iVehOn = -1;
    const CTransportData* m_pTd = nullptr;

    struct VehEntry {
        int vehType;
        const CTransportData* pData;
    };
    VehEntry m_vehs[6];
    int m_numVehs = 0;
    int m_buildNum = 1;

    SDL2Button* m_vehBtns[6] = {};
    SDL2Label*  m_lblDesc = nullptr;
    SDL2Label*  m_lblCosts = nullptr;
    SDL2Button* m_btnBuild = nullptr;
    SDL2Label*  m_lblNum = nullptr;
};
