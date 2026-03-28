#pragma once

#include "SDL2UI.h"
#include <vector>

class GameWindow;
class CRaceDef;
class CPlayer;

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

    // Results stored for the caller
public:
    int m_iAiLevel = 0;
    int m_iWorldSize = 1;
    int m_iStartPos = 1;
    int m_iNumAi = 2;
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
protected:
    void OnInit() override;
    void OnOK() override;
private:
    SDL2Listbox* m_lstPlayers = nullptr;
    SDL2EditBox* m_edtName = nullptr;
    SDL2Label* m_lblDesc = nullptr;
    SDL2Button* m_btnOK = nullptr;

    struct PlayerInfo {
        int plyrNum;
        bool available;
        int numBldgs;
        int numVeh;
        std::string name;
    };
    std::vector<PlayerInfo> m_players;

    void OnPlayerSelected(int index);
    void OnNameChanged(const std::string& name);
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
public:
    int m_iAiLevel = 0;
    int m_iWorldSize = 1;
    int m_iStartPos = 1;
    int m_iNumAi = 2;
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
