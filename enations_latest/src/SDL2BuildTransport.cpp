#include "stdafx.h"

#include "SDL2BuildTransport.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "player.h"
#include "building.inl"
#include "vehicle.inl"
#include "bitmaps.h"
#include "netcmd.h"

SDL2BuildTransport::SDL2BuildTransport(GameWindow* gw, CVehicleBuilding* pBldg)
    : SDL2Dialog(gw, "Build Vehicle", 380, 330)
    , m_pBldg(pBldg)
{
    memset(m_vehs, 0, sizeof(m_vehs));
}

void SDL2BuildTransport::OnInit() {
    int vehX = m_x + 10;
    int infoX = m_x + 130;
    int btnW = 110, btnH = 40;

    // Vehicle buttons (left column)
    CBuildVehicle const* pBv = m_pBldg->GetData()->GetBldVehicle();
    int slot = 0;
    for (int i = 0; i < pBv->GetSize() && slot < 6; i++) {
        CTransportData const* pTd = theTransports.GetData(pBv->GetUnit(i)->GetVehType());
        if (!pTd->IsDiscovered())
            continue;
        if (theGame.GetScenario() != -1 && pTd->GetScenario() > theGame.GetScenario())
            continue;

        m_vehs[slot].vehType = pBv->GetUnit(i)->GetVehType();
        m_vehs[slot].pData = pTd;

        int y = m_y + 34 + slot * 46;
        m_vehBtns[slot] = AddWidget<SDL2Button>(vehX, y, btnW, btnH,
            (const char*)pTd->GetDesc(),
            [this, slot]() { SelectVehicle(slot); });
        slot++;
    }
    m_numVehs = slot;

    // Fill unused buttons
    for (int i = slot; i < 6; i++) {
        int y = m_y + 34 + i * 46;
        m_vehBtns[i] = AddWidget<SDL2Button>(vehX, y, btnW, btnH, "", nullptr);
        m_vehBtns[i]->SetVisible(false);
    }

    // Description (right side)
    m_lblDesc = AddWidget<SDL2Label>(infoX, m_y + 34, 240, 100, "");
    m_lblDesc->SetWrapped(true);

    // Cost info
    m_lblCosts = AddWidget<SDL2Label>(infoX, m_y + 140, 240, 100, "");
    m_lblCosts->SetWrapped(true);

    // Quantity
    m_lblNum = AddWidget<SDL2Label>(infoX, m_y + 250, 100, 24,
        "Qty: " + std::to_string(m_buildNum));

    AddWidget<SDL2Button>(infoX + 100, m_y + 250, 30, 24, "-",
        [this]() {
            if (m_buildNum > 1) m_buildNum--;
            m_lblNum->SetText("Qty: " + std::to_string(m_buildNum));
        });
    AddWidget<SDL2Button>(infoX + 135, m_y + 250, 30, 24, "+",
        [this]() {
            m_buildNum++;
            m_lblNum->SetText("Qty: " + std::to_string(m_buildNum));
        });

    // Build and Cancel buttons
    int bottomY = m_y + m_height - 38;
    m_btnBuild = AddWidget<SDL2Button>(m_x + 130, bottomY, 95, 28, "Build",
        [this]() { OnBuild(); });
    m_btnBuild->SetEnabled(false);

    AddWidget<SDL2Button>(m_x + 240, bottomY, 95, 28, "Cancel",
        [this]() { OnCancel(); });

    // Auto-select first vehicle if available
    if (m_numVehs > 0)
        SelectVehicle(0);
}

void SDL2BuildTransport::SelectVehicle(int idx) {
    if (idx < 0 || idx >= m_numVehs)
        return;
    m_iVehOn = idx;
    m_pTd = m_vehs[idx].pData;
    m_btnBuild->SetEnabled(true);
    UpdateDescription();
}

void SDL2BuildTransport::UpdateDescription() {
    if (!m_pTd) {
        m_lblDesc->SetText("");
        m_lblCosts->SetText("");
        return;
    }

    m_lblDesc->SetText((const char*)m_pTd->GetDesc());

    // Show basic vehicle info (detailed costs would require CBuildUnit access)
    std::string info;
    if (m_pTd->GetPeople() > 0)
        info += "Crew: " + std::to_string(m_pTd->GetPeople()) + "\n";
    info += "Select and click Build to produce.";
    m_lblCosts->SetText(info);
}

void SDL2BuildTransport::OnBuild() {
    if (m_iVehOn < 0 || !m_pTd)
        return;

    int iVehType = m_vehs[m_iVehOn].vehType;

    // Start vehicle construction
    m_pBldg->ResumeUnit();
    m_pBldg->StartVehicle(iVehType, m_buildNum);

    // Tell server in network games
    if (!theGame.AmServer()) {
        CMsgBuildVeh msg(m_pBldg, iVehType);
        theGame.PostToServer(&msg, sizeof(msg));
    }

    EndDialog(1);
}

void SDL2BuildTransport::OnCancel() {
    EndDialog(0);
}
