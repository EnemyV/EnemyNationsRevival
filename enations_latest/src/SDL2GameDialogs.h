#pragma once

#include "SDL2UI.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <SDL_ttf.h>

// Called from netapi.cpp when a cmd_chat network message arrives.
void SDL2Chat_AddMessage(const std::string& line);

class CVehicle;
class CBuilding;
class CPlayer;
class CUnit;
class SDL2Panel;
class SDL2Compositor;

// ============================================================================
// SDL2ResearchDialog — replaces CDlgResearch
// ============================================================================
class SDL2ResearchDialog : public SDL2Dialog {
public:
    SDL2ResearchDialog(GameWindow* gw);
    ~SDL2ResearchDialog();
protected:
    void OnInit() override;
private:
    void PopulateList();
    void SelectItem(int idx);
    void OnStart();
    void OnDiscover();

    struct RsrchEntry { int index; std::string name; bool available; };
    std::vector<RsrchEntry> m_items;
    int m_selected = -1;

    SDL2Listbox* m_list = nullptr;
    SDL2Label*   m_lblDesc = nullptr;
    SDL2Button*  m_btnStart = nullptr;
    SDL2Button*  m_btnClose = nullptr;
    SDL2Button*  m_btnDiscover = nullptr;

    // Art (loaded from theBitmaps — matches CDlgResearch::OnPaint / OnDrawItem)
    SDL_Surface* m_bkgnd     = nullptr;   // DIB_RSRCH_BKGND — PCB circuit-board art
    SDL_Surface* m_btnSheet  = nullptr;   // DIB_RESEARCH_BTNS — 3-state button sheet
    SDL_Surface* m_flaskSheet = nullptr;  // ICON_RESEARCH — flask/bulb progress sprite
};

// ============================================================================
// SDL2RelationsDialog — replaces CDlgRelations
// ============================================================================
class SDL2RelationsDialog : public SDL2Dialog {
public:
    SDL2RelationsDialog(GameWindow* gw);
protected:
    void OnInit() override;
private:
    void SelectPlayer(int idx);
    void SetRelation(int level);
    void OnGive();

    struct PlayerEntry { CPlayer* pPlr; std::string name; };
    std::vector<PlayerEntry> m_players;
    int m_selected = -1;

    SDL2Listbox*    m_list = nullptr;
    SDL2RadioGroup* m_radRelations = nullptr;
    SDL2Label*      m_lblInfo = nullptr;
    SDL2Button*     m_btnGive = nullptr;
};

// ============================================================================
// SDL2LoadTruckDialog — replaces CDlgLoadTruck
// ============================================================================
class SDL2LoadTruckDialog : public SDL2Dialog {
public:
    SDL2LoadTruckDialog(GameWindow* gw, CVehicle* pVeh);
protected:
    void OnInit() override;
private:
    void OnLoad();        // Load proportionally up to vehicle capacity
    void OnLoadBldg();    // Preset: 50/50 Steel/Lumber (construction trip)
    void OnLoadVeh();     // Preset: 80/20 Steel/Copper (vehicle-factory trip)
    void OnUnload();      // Clear all amounts
    void OnAuto();        // Hand back to auto-router
    void OnOK();
    void OnCancel();
    void RefreshTotals();

    CVehicle*  m_pVeh;
    CBuilding* m_pBldg;
    SDL2Slider* m_sliders[6] = {};
    SDL2Label*  m_lblAmounts[6] = {};
    SDL2Label*  m_lblCapacity = nullptr;
};

// ============================================================================
// SDL2PauseDialog — replaces CDlgPause (simple notification)
// ============================================================================
class SDL2PauseDialog : public SDL2Dialog {
public:
    SDL2PauseDialog(GameWindow* gw, const std::string& message);
protected:
    void OnInit() override;
private:
    std::string m_message;
};

// ============================================================================
// SDL2DiscoverDialog — replaces CDlgDiscover (research completion)
// ============================================================================
class SDL2DiscoverDialog : public SDL2Dialog {
public:
    SDL2DiscoverDialog(GameWindow* gw, const std::string& title, const std::string& description);
protected:
    void OnInit() override;
private:
    std::string m_discTitle;
    std::string m_discDesc;
};

// ============================================================================
// SDL2MessageBox — generic Yes/No or Yes/No/Cancel message box
// Replaces AfxMessageBox for in-game confirmation dialogs.
// Result: IDYES (6), IDNO (7), IDCANCEL (2)
// ============================================================================
class SDL2MessageBox : public SDL2Dialog {
public:
    enum Style { YesNo, YesNoCancel };

    SDL2MessageBox(GameWindow* gw, const std::string& message, Style style = YesNo);
protected:
    void OnInit() override;
private:
    std::string m_message;
    Style m_style;
};

// ============================================================================
// SDL2UnitInfoPanel — replaces CWndInfo (right-click unit tooltip)
// Non-modal panel showing unit name, damage, status, materials, cargo.
// ============================================================================
class SDL2UnitInfoPanel {
public:
    SDL2UnitInfoPanel();
    ~SDL2UnitInfoPanel();

    void Show(CUnit* pUnit, int screenX, int screenY);
    void Hide();
    void Update();  // Refresh if unit state changed
    bool IsVisible() const;
    CUnit* GetUnit() const { return m_pUnit; }

private:
    void BuildContent();
    void Render();

    CUnit*     m_pUnit = nullptr;
    SDL2Panel* m_panel = nullptr;
    std::string m_fontPath;
    std::unordered_map<int, TTF_Font*> m_fontCache;
    TTF_Font* GetFont(int size);

    // Original art surfaces
    SDL_Surface* m_bgGold = nullptr;
    SDL_Surface* m_borderH = nullptr;
    SDL_Surface* m_borderV = nullptr;
    SDL_Surface* m_matIcons = nullptr;  // ICON_MATERIALS strip (one icon per material type)
    int m_matIconW = 0;                 // width of each material icon in the strip
    int m_matIconH = 0;                 // height of each material icon
    bool m_artLoaded = false;
    void LoadArt();

    struct TextLine {
        std::string text;
        bool red;
        int matIdx = -1;  // material type index → draw ICON_MATERIALS icon before text; -1 = none
    };
    std::vector<TextLine> m_lines;
    static const int LINE_HT = 16;
    static const int PAD = 6;
};

// ============================================================================
// SDL2ChatWindow — replaces CWndChat (multiplayer chat)
// Non-modal panel with message history and input field.
// ============================================================================
class SDL2ChatWindow : public SDL2Dialog {
public:
    SDL2ChatWindow(GameWindow* gw);
protected:
    void OnInit() override;
    void OnFrame() override;
private:
    void OnSend();
    void RefreshMessages();

    SDL2Listbox* m_msgList = nullptr;
    SDL2EditBox* m_editMsg = nullptr;
    int          m_lastMsgCount = -1;
};

// ============================================================================
// SDL2PlayerListDialog — replaces CDlgPlyrList (in-game player list)
// Shows all players with status info.
// ============================================================================
class SDL2PlayerListDialog : public SDL2Dialog {
public:
    SDL2PlayerListDialog(GameWindow* gw);
protected:
    void OnInit() override;
private:
    void PopulateList();
    SDL2Listbox* m_list = nullptr;
    SDL2Label*   m_lblInfo = nullptr;
};

// ============================================================================
// SDL2ComposeDialog — replaces CDlgCompose (multiplayer message compose)
// ============================================================================
class SDL2ComposeDialog : public SDL2Dialog {
public:
    SDL2ComposeDialog(GameWindow* gw);
protected:
    void OnInit() override;
private:
    void OnSend();
    SDL2Listbox* m_recipientList = nullptr;
    SDL2EditBox* m_editSubject = nullptr;
    SDL2EditBox* m_editBody = nullptr;
};

// ============================================================================
// SDL2CutSceneDialog — replaces CWndCutScene (win/lose/scenario screens)
// Shows the end/scenario screen text with appropriate buttons.
// Returns CUT_OK (1) or CUT_CANCEL (2).
// ============================================================================
class SDL2CutSceneDialog : public SDL2Dialog {
public:
    // typ matches CWndCutScene enum: cut=0, repeat=1, scenario_end=2, win=3, lose=4
    SDL2CutSceneDialog(GameWindow* gw, int typ, const std::string& text, int scenario);
    int GetResult() const { return m_result; }
protected:
    void OnInit() override;
private:
    void OnOK();
    void OnCancel();
    void OnSave();

    int m_typ;
    std::string m_text;
    int m_scenario;
    int m_result = 1; // CUT_OK by default
};
