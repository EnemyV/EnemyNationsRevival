#pragma once

#include "SDL2UI.h"

class CVehicle;
class CStructureData;

// Native SDL2 replacement for CDlgBuildStructure.
// Shows building categories, building selection, costs, and Build button.
// Opened when a crane clicks Build or double-clicking a construction building.

class SDL2BuildStructure : public SDL2Dialog {
public:
    SDL2BuildStructure(GameWindow* gw, CVehicle* pVeh);
    ~SDL2BuildStructure();

protected:
    void OnInit() override;
    void OnCancel() override;

private:
    void SelectCategory(int cat);
    void SelectBuilding(int idx);
    void OnBuild();
    void UpdateDescription();
    bool CanBuild(int iCat, const CStructureData* pSd);

    CVehicle* m_pVeh;

    // Current state
    int m_iCatOn = -1;
    int m_iBldgOn = -1;
    const CStructureData* m_pSd = nullptr;

    // Building list for current category
    struct BldgEntry {
        int structureIndex;              // Index into theStructures
        const CStructureData* pData;
    };
    BldgEntry m_bldgs[6];
    int m_numBldgs = 0;

    // Widgets
    SDL2Button* m_catBtns[6] = {};
    SDL2Button* m_bldgBtns[6] = {};
    SDL2Label*  m_lblDesc = nullptr;
    SDL2Label*  m_lblCostHdr = nullptr;  // "cost have need" header
    SDL2Label*  m_lblCosts = nullptr;
    SDL2Label*  m_lblOper = nullptr;     // Operating costs (blue)
    SDL2Button* m_btnBuild = nullptr;

    // Art surfaces (from theBitmaps)
    SDL_Surface* m_bldgIconSheet = nullptr;    // DIB_LIST_UNIT_BUILDINGS - building icons
    SDL_Surface* m_structBkgnd = nullptr;      // DIB_STRUCTURE_BKGND - dialog background
    SDL_Surface* m_catBtnSheet = nullptr;      // DIB_STRUCTURE_BTNS_1 - category button sprites
    SDL_Surface* m_bldgBtnSheet = nullptr;     // DIB_STRUCTURE_BTNS_2 - building button sprites
    SDL_Surface* m_okBtnSheet = nullptr;       // DIB_STRUCTURE_BTNS_3 - ok/cancel button sprites
};
