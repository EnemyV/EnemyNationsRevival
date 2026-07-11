#pragma once

#include "SDL2UI.h"
#include <vector>

class GameWindow;
class CRaceDef;
class CPlayer;
class CJoinMulti;

// ============================================================================
// SDL2 Version Dialog (replaces CDlgVer)
// ============================================================================
class SDL2VersionDialog : public SDL2Dialog {
public:
    SDL2VersionDialog(GameWindow* gameWindow);
protected:
    void OnInit() override;
};

// ============================================================================
// SDL2 Advanced Options Dialog (replaces CDlgAdvOptions)
// ============================================================================
class SDL2AdvOptionsDialog : public SDL2Dialog {
public:
    SDL2AdvOptionsDialog(GameWindow* gameWindow);
protected:
    void OnInit() override;
    void OnOK() override;
private:
    SDL2Checkbox* m_chkScroll = nullptr;
    SDL2Checkbox* m_chkPause = nullptr;
    SDL2Checkbox* m_chkNoIntro = nullptr;
    SDL2RadioGroup* m_radZoom = nullptr;
    SDL2RadioGroup* m_radRenderer = nullptr;
};

// ============================================================================
// SDL2WorldGenWidgets — the shared "world generation" settings group: the
// World Type preset list plus the Rivers and Ocean sliders. ONE implementation
// (AddTo/readers in SDL2Dialogs.cpp) builds and reads the group for BOTH the
// single-player and network create dialogs, so a change here updates both.
// ============================================================================
struct SDL2WorldGenWidgets {
    SDL2Listbox* lstWorldType = nullptr;
    SDL2Label*   lblRivers = nullptr;   // live "Rivers: N%" readout
    SDL2Slider*  sldRivers = nullptr;
    SDL2Label*   lblOcean = nullptr;    // live "Ocean: N%" readout
    SDL2Slider*  sldOcean = nullptr;

    // Build the group in dlg: World Type group box + list at (lx, y), width w,
    // then the Rivers/Ocean slider rows. Returns the y just below the group.
    // The slider lambdas capture `this` — embed the struct in the dialog (it
    // must outlive the widgets and not move).
    int AddTo(SDL2Dialog* dlg, int lx, int y, int w, int rowH);

    // Read back the selections, with the shared clamps/fallbacks.
    int WorldType() const;   // EWorldType preset (0 on no selection)
    int Rivers() const;      // 0-100 (60 = classic baseline)
    int Ocean() const;       // 0-100 (50 = baseline ~= old average)
};

// ============================================================================
// SDL2 Create Single Player Dialog (replaces CDlgCreateSingle)
// ============================================================================
class SDL2CreateSingleDialog : public SDL2Dialog {
public:
    SDL2CreateSingleDialog(GameWindow* gameWindow);
protected:
    void OnInit() override;
    void OnOK() override;
private:
    SDL2RadioGroup* m_radAiLevel = nullptr;
    SDL2RadioGroup* m_radWorldSize = nullptr;
    SDL2RadioGroup* m_radStartPos = nullptr;
    SDL2EditBox* m_edtNumAi = nullptr;
    SDL2WorldGenWidgets m_worldGen;   // shared with SDL2CreateNetDialog

    // Results stored for the caller
public:
    int m_iAiLevel = 0;
    int m_iWorldSize = 1;
    int m_iStartPos = 1;
    int m_iWorldType = 0;   // EWorldType preset
    int m_iNumAi = 2;
    int m_iRivers = 60;     // river density 0-100 (60 = baseline)
    int m_iOcean = 50;      // ocean size 0-100 (50 = baseline ~= current average)
};

// ============================================================================
// SDL2 Pick Race Dialog (replaces CDlgPickRace)
// ============================================================================
class SDL2PickRaceDialog : public SDL2Dialog {
public:
    SDL2PickRaceDialog(GameWindow* gameWindow);
    ~SDL2PickRaceDialog();
protected:
    void OnInit() override;
    void OnOK() override;
private:
    SDL2Listbox* m_lstRaces = nullptr;
    SDL2EditBox* m_edtName = nullptr;
    SDL2Label* m_lblDesc = nullptr;
    SDL2Image* m_imgPicture = nullptr;
    SDL2Button* m_btnOK = nullptr;

    void OnRaceSelected(int index);
    void OnNameChanged(const std::string& name);
    void UpdateOKButton();

    // Race picture surfaces (converted from CDIB, owned by us)
    std::vector<SDL_Surface*> m_racePictures;

public:
    int m_iSelectedRace = -1;
    std::string m_playerName;
};

// ============================================================================
// SDL2 Scenario Dialog (replaces CDlgScenario)
// ============================================================================
class SDL2ScenarioDialog : public SDL2Dialog {
public:
    SDL2ScenarioDialog(GameWindow* gameWindow);
protected:
    void OnInit() override;
    void OnOK() override;
private:
    SDL2RadioGroup* m_radAiLevel = nullptr;
    SDL2RadioGroup* m_radWorldSize = nullptr;
public:
    int m_iAiLevel = 0;
    int m_iWorldSize = 1;
};

// ============================================================================
// SDL2 Pick Player Dialog (for loaded games - replaces CDlgPickPlayer)
// ============================================================================
class SDL2PickPlayerDialog : public SDL2Dialog {
public:
    SDL2PickPlayerDialog(GameWindow* gameWindow);
    ~SDL2PickPlayerDialog();
protected:
    void OnInit() override;
    void OnOK() override;
private:
    SDL2Listbox* m_lstPlayers = nullptr;
    SDL2Label* m_lblName = nullptr;   // saved player's name — read-only when loading
    SDL2Label* m_lblDesc = nullptr;
    SDL2Image* m_imgPicture = nullptr;
    SDL2Button* m_btnOK = nullptr;

    struct PlayerInfo {
        int plyrNum;
        bool available;
        int numBldgs;
        int numVeh;
        int raceIdx;          // race this player chose (-1 if unknown)
        std::string name;
        // Stored resources (material name + amount, only those > 0) so the
        // description can list them like the original CDlgPickPlayer did.
        struct ResEntry { std::string name; int amount; };
        std::vector<ResEntry> resources;
    };
    std::vector<PlayerInfo> m_players;

    // Race portrait surfaces, indexed by race (converted from CDIB, owned by us).
    std::vector<SDL_Surface*> m_racePictures;

    void OnPlayerSelected(int index);
    void UpdateOKButton();

public:
    int m_iSelectedPlyrNum = -1;
    std::string m_playerName;
};

// ============================================================================
// SDL2 Create Network Game Dialog (replaces CDlgCreateMulti + CDlgCreatePublish)
// TCP/IP only.
// ============================================================================
class SDL2CreateNetDialog : public SDL2Dialog {
public:
    SDL2CreateNetDialog(GameWindow* gameWindow);
protected:
    void OnInit() override;
    void OnOK() override;
private:
    SDL2RadioGroup* m_radAiLevel = nullptr;
    SDL2RadioGroup* m_radWorldSize = nullptr;
    SDL2RadioGroup* m_radStartPos = nullptr;
    SDL2EditBox* m_edtNumAi = nullptr;
    SDL2EditBox* m_edtGameName = nullptr;
    SDL2EditBox* m_edtPlayerName = nullptr;
    SDL2EditBox* m_edtPort = nullptr;
    SDL2WorldGenWidgets m_worldGen;   // shared with SDL2CreateSingleDialog
public:
    int m_iAiLevel = 0;
    int m_iWorldSize = 1;
    int m_iStartPos = 1;
    int m_iWorldType = 0;   // EWorldType preset
    int m_iNumAi = 2;
    int m_iRivers = 60;     // river density 0-100 (60 = baseline)
    int m_iOcean = 50;      // ocean size 0-100 (50 = baseline ~= old average)
    std::string m_gameName;
    std::string m_playerName;
    int m_iPort = 0;
};

// ============================================================================
// SDL2 Join Network Game Dialog (replaces CDlgJoinPublish + CDlgJoinGame)
// TCP/IP only.
// ============================================================================
class SDL2JoinNetDialog : public SDL2Dialog {
public:
    SDL2JoinNetDialog(GameWindow* gameWindow);
protected:
    void OnInit() override;
    void OnOK() override;
private:
    SDL2EditBox* m_edtPlayerName = nullptr;
    SDL2EditBox* m_edtServerAddr = nullptr;
    SDL2EditBox* m_edtPort = nullptr;
public:
    std::string m_playerName;
    std::string m_serverAddr;
    int m_iPort = 0;
};

// ============================================================================
// SDL2HostLoadedDialog — minimal server dialog for Load Network Game flow.
// Collects player name and TCP port only (game settings come from the save).
// ============================================================================
class SDL2HostLoadedDialog : public SDL2Dialog {
public:
    SDL2HostLoadedDialog(GameWindow* gw, const std::string& gameName);
protected:
    void OnInit() override;
    void OnOK() override;
private:
    std::string  m_gameName;
    SDL2EditBox* m_edtPlayerName = nullptr;
    SDL2EditBox* m_edtPort       = nullptr;
public:
    std::string m_playerName;
    int         m_iPort = 2346;
};

// ============================================================================
// SDL2LobbyDialog — server waiting room shown after OpenServer / race pick,
// before ReadyToCreate(). Polls theGame.GetAll() each frame to show who has
// joined. Server clicks "Start" when ready.
// ============================================================================
class SDL2LobbyDialog : public SDL2Dialog {
public:
    SDL2LobbyDialog(GameWindow* gw, const std::string& gameName);
protected:
    void OnInit() override;
    void OnFrame() override;
    void OnOK() override;   // Enter in the chat box must Send, not fall through to Start
private:
    void OnStart();
    void UpdatePlayerList();
    void RefreshChat();
    void SendChat();

    std::string  m_gameName;
    SDL2Listbox* m_lstPlayers = nullptr;
    SDL2Label*   m_lblStatus  = nullptr;
    SDL2Listbox* m_lstChat    = nullptr;
    SDL2EditBox* m_edtChat    = nullptr;
    int          m_lastCount  = -1;
    int          m_chatCount  = -1;
    std::string  m_lastSig;   // name|race signature, rebuild list only when it changes
};

// ============================================================================
// SDL2ClientLobbyDialog — joining player's waiting room. Shown after the client
// joins + picks a race, while it waits for the host to click Start. Closes with
// result 1 when the host starts (CNetStart, deferred) so the flow can build the
// world; result 0 if the player leaves.
// ============================================================================
class SDL2ClientLobbyDialog : public SDL2Dialog {
public:
    SDL2ClientLobbyDialog(GameWindow* gw, const std::string& gameName);
protected:
    void OnInit() override;
    void OnFrame() override;
    void OnOK() override;   // Enter in the chat box must Send, not close the dialog (result 1 = host started)
private:
    void UpdatePlayerList();
    void RefreshChat();
    void SendChat();
    std::string  m_gameName;
    SDL2Listbox* m_lstPlayers = nullptr;
    SDL2Label*   m_lblStatus  = nullptr;
    SDL2Listbox* m_lstChat    = nullptr;
    SDL2EditBox* m_edtChat    = nullptr;
    int          m_chatCount  = -1;
    std::string  m_lastSig;
    // Host-presence tracking: surface "the host left / no host responding" instead
    // of sitting on "Waiting for the host..." forever (a joined-but-hostless game).
    Uint32       m_startTicks  = 0;
    bool         m_sawHost     = false;
    int          m_statusState = 0;   // 0=waiting, 1=no-response-yet, 2=host-left
};

// ============================================================================
// SDL2SessionBrowseDialog — TCP/IP session browser for Join flow
// Enumerates available games via VDMPLAY, lets the user pick one and join.
// OnFrame() refreshes the list each frame as VP_SESSIONENUM callbacks fire.
// ============================================================================
class SDL2SessionBrowseDialog : public SDL2Dialog {
public:
    SDL2SessionBrowseDialog(GameWindow* gw, CJoinMulti* pJoin);
protected:
    void OnInit() override;
    void OnFrame() override;
private:
    void OnJoin();
    void OnRefresh();
    void UpdateList();
    void SelectIndex(int idx);

    void OnSearch();   // re-target the client to the edited address/port and re-enum

    CJoinMulti*  m_pJoin;
    SDL2Listbox* m_lstSessions = nullptr;
    SDL2Label*   m_lblInfo     = nullptr;
    SDL2Button*  m_btnJoin     = nullptr;
    SDL2EditBox* m_edtAddr     = nullptr;
    SDL2EditBox* m_edtPort     = nullptr;

    int m_lastCount  = -1;
    int m_selectedIdx = -1;

public:
    int m_chosenIdx = -1;   // index into m_pJoin->m_sessions on OK
    // On Search the dialog closes with result 3 and the join flow re-targets the
    // client to these (re-opening the transport OUTSIDE the modal loop, since
    // OpenClient blocks + pumps messages and would re-enter the loop -> hang).
    std::string m_searchAddr;
    int         m_searchPort = 2346;
};

// ============================================================================
// Flow helpers
// ============================================================================
bool SDL2_RunCreateSinglePlayerFlow(GameWindow* gameWindow);
bool SDL2_RunCreateScenarioFlow(GameWindow* gameWindow);
bool SDL2_RunLoadSinglePlayerFlow(GameWindow* gameWindow);
bool SDL2_RunCreateNetworkFlow(GameWindow* gameWindow);
bool SDL2_RunJoinNetworkFlow(GameWindow* gameWindow);
bool SDL2_RunLoadNetworkFlow(GameWindow* gameWindow);

// Full-screen scrolling credits
void SDL2_RunCredits(GameWindow* gameWindow);
