#pragma once

#include "SDL2UI.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <SDL_ttf.h>

// Called from netapi.cpp when a cmd_chat network message arrives.
void SDL2Chat_AddMessage(const std::string& line);

// Shared chat log accessors so any UI (in-game chat window OR the pre-game
// lobbies) can render the same message history.
int         SDL2Chat_Count();
std::string SDL2Chat_Line(int i);

// Clear the chat backlog — called on entering a network room so the new room
// starts fresh instead of showing the previous session's history.
void        SDL2Chat_Clear();

// Send a chat line: broadcasts to the other players (net game) and echoes it
// locally. Usable pre-game (lobby) and in-game.
void SDL2Chat_Send(const std::string& text);

// Called from netapi.cpp when an ipc_msg (email/chat) network message arrives.
// Reconstructs the message from the wire buffer and routes it to the inbox
// (email) or the chat log (chat). Replaces the old CWndComm::IncomingMessage.
void SDL2Mail_HandleIncoming(class CMsgIPC* pMsg);
// Number of unread messages currently in the inbox.
int  SDL2Mail_UnreadCount();

// Non-modal comms openers. Each keeps a single live instance (re-opening just
// raises the existing one) so the game keeps running while comms are up — the
// original CWndComm/CWndMailRead were modeless for exactly this reason. The
// GameWindow owns the dialog lifetime once shown.
class GameWindow;
void SDL2Comms_OpenChat(GameWindow* gw);
void SDL2Comms_OpenMail(GameWindow* gw);
void SDL2Comms_OpenCompose(GameWindow* gw, int toPlyr,
                           const std::string& subject, const std::string& body);
// Close any open comms windows (called from CWndBar::OnDestroy on quit-to-menu).
void SDL2Comms_CloseAll();

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

    // Re-sync the list from current research state, preserving the browse
    // selection. Called when a topic is discovered so an open window updates live.
    void Refresh();

    // A topic was just discovered for the local player: re-sync the list and arm
    // the "Discovery" button to (re)show that topic's result text.
    void NotifyDiscovered(int iItem);
protected:
    void OnInit() override;
private:
    void PopulateList();
    void SelectItem(int idx);
    void OnStart();
    void OnDiscover();

    int m_lastDiscovered = 0;   // topic the "Discovery" button re-shows (0 = none)

    struct RsrchEntry { int index; std::string name; bool available; };
    std::vector<RsrchEntry> m_items;
    int m_selected = -1;
    bool m_showDone = false;   // already-discovered ("[Done]") items are hidden by default

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
    // Slider drag handler: clamp this slider so the COMBINED load across all six
    // materials never exceeds the vehicle's cargo capacity (GetMaxMaterials).
    void OnSliderChanged(int idx, int val);
    void RefreshTotals();
    // Re-derive m_pBldg from the (still-alive) vehicle's hex. Called at the top of
    // each action handler so a cached building pointer can't dangle if the building
    // is destroyed while this non-modal dialog is open (the vehicle owns the dialog
    // and closes it on its own death, so m_pVeh stays valid).
    void RefreshBldg();

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
    SDL_Surface* m_vehIcons = nullptr;  // DIB_LIST_UNIT_VEHICLES strip (vertical, one 64px tile per vehType)
    int m_vehTileW = 0;                 // tile width in the vehicle strip (= sheet width)
    int m_vehTileH = 0;                 // tile height in the vehicle strip (64)
    bool m_artLoaded = false;
    void LoadArt();

    struct TextLine {
        std::string text;
        bool red;
        int matIdx = -1;   // material type index → draw ICON_MATERIALS icon before text; -1 = none
        int vehIcon = -1;  // vehType → draw DIB_LIST_UNIT_VEHICLES icon before text; -1 = none
    };
    std::vector<TextLine> m_lines;
    static const int LINE_HT = 16;
    // Contained-vehicle rows are taller so the unit icon is legible.
    static const int VEH_ROW_HT = 30;
    static const int VEH_ICON_SZ = 26;  // square box the vehicle icon is scaled into
    static const int PAD = 6;
    // Height of a single content row (taller for vehicle-icon rows).
    int LineHeight(const TextLine& l) const { return l.vehIcon >= 0 ? VEH_ROW_HT : LINE_HT; }
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
    // Pre-filled compose for Reply/Forward: preselect a recipient (plyr num, -1
    // = none) and seed the subject/body.
    SDL2ComposeDialog(GameWindow* gw, int toPlyr,
                      const std::string& subject, const std::string& body);
protected:
    void OnInit() override;
    void OnOK() override;   // Enter from a single-line field (e.g. Subject) sends
private:
    void OnSend();
    SDL2Listbox* m_recipientList = nullptr;
    SDL2EditBox* m_editSubject = nullptr;
    SDL2EditBox* m_editBody = nullptr;
    std::vector<int> m_recipientPlyrs;   // plyr num for each recipient list row
    int          m_preToPlyr = -1;       // preselected recipient (reply/forward)
    std::string  m_preSubject;
    std::string  m_preBody;
};

// ============================================================================
// SDL2MailDialog — inbox + reader. Replaces the CWndComm mail hub and
// CWndMailRead: a message list on the left, the selected message on the right,
// with Compose / Reply / Forward / Delete actions.
// ============================================================================
class SDL2MailDialog : public SDL2Dialog {
public:
    SDL2MailDialog(GameWindow* gw);
protected:
    void OnInit() override;
    void OnFrame() override;   // refresh the list when new mail arrives while open
private:
    void RefreshList();
    int  m_lastInboxCount = -1;
    void ShowSelected();
    void OnCompose();
    void OnReply();
    void OnForward();
    void OnDelete();

    SDL2Listbox* m_list = nullptr;
    SDL2Label*   m_lblFrom = nullptr;
    SDL2Label*   m_lblSubject = nullptr;
    SDL2Label*   m_lblBody = nullptr;
    int          m_sel = -1;
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

    // Full-screen win/lose/scenario-end layout (mirrors CWndCutScene::OnPaintWin/Lose).
    void OnInitEndScreen();
    // Full-screen scenario INTRO layout: CS painting + briefing text (CWndCutScene::OnPaintCut).
    void OnInitIntro();
    // Load the CS/WN/LS/DN full-screen painting from misc.mif (WL wallpaper fallback).
    static SDL_Surface* LoadEndArt(int typ);

    int m_typ;
    std::string m_text;
    int m_scenario;
    int m_result = 1; // CUT_OK by default
};
