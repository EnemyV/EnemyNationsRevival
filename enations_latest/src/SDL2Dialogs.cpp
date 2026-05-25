#include "stdafx.h"
#include "SDL2Dialogs.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "sfx.h"
#include "player.h"
#include "racedata.h"
#include "new_game.h"
#include "creatsin.h"
#include "creatmul.h"
#include "join.h"
#include "scenario.h"
#include "creatmul.inl"
#include "netcmd.h"
#include "netapi.h"

#undef min
#undef max

// ============================================================================
// SDL2VersionDialog
// ============================================================================
SDL2VersionDialog::SDL2VersionDialog(GameWindow* gameWindow)
    : SDL2Dialog(gameWindow, "Version Information", 520, 380) {}

void SDL2VersionDialog::OnInit() {
    int y = m_y + 40, lx = m_x + 20, w = m_width - 40, rowH = 22;
    SDL_Color tc = { 200, 200, 200, 255 };

    std::string sVer = "Version: " VER_STRING;
#ifdef _DEBUG
    sVer += " (debug, cheat)";
#elif defined(_CHEAT)
    sVer += " (cheat)";
#endif
    sVer += " - " __DATE__ "  " __TIME__;
    AddWidget<SDL2Label>(lx, y, w, rowH, sVer, tc); y += rowH;

    std::string sOs;
    switch (iWinType) {
        case W32s: sOs = "Win32s"; break;
        case W95:  sOs = "Windows95"; break;
        case WNT:  sOs = "Win/NT"; break;
        default:   sOs = "Unknown"; break;
    }
    OSVERSIONINFO ovi = {};
    ovi.dwOSVersionInfoSize = sizeof(ovi);
    if (GetVersionEx(&ovi))
        sOs += " " + std::to_string(ovi.dwMajorVersion) + "." +
               std::to_string(ovi.dwMinorVersion) + " (" + std::to_string(ovi.dwBuildNumber) + ")";
    AddWidget<SDL2Label>(lx, y, w, rowH, sOs, tc); y += rowH;

    long lVer = CNetApi::GetVersion();
    AddWidget<SDL2Label>(lx, y, w, rowH,
        "VDMPlay API " + std::to_string(HIBYTE(HIWORD(lVer))) + "." +
        std::to_string(LOBYTE(HIWORD(lVer))) + "." + std::to_string(LOWORD(lVer)), tc);
    y += rowH;

    std::string sRif = "Data Ver: " + std::to_string(theApp.GetRifVer()) + "." + std::to_string(VER_RIFF);
    sRif += theApp.HaveWAV() ? ", WAV" : ", MIDI";
    sRif += theApp.Have24Bit() ? ", 24-bit" : ", 8-bit";
    AddWidget<SDL2Label>(lx, y, w, rowH, sRif, tc); y += rowH;

    std::string sVideo = "Video: ";
    switch (ptrthebltformat->GetType()) {
        case CBLTFormat::DIB_WING:        sVideo += "WinG"; break;
        case CBLTFormat::DIB_DIBSECTION:  sVideo += "CreateDIBSection"; break;
        case CBLTFormat::DIB_MEMORY:      sVideo += "StretchDIBits"; break;
        case CBLTFormat::DIB_SDL_SURFACE: sVideo += "SDL_Surface"; break;
        default:                          sVideo += "?"; break;
    }
    sVideo += (ptrthebltformat->GetDirection() == CBLTFormat::DIR_TOPDOWN) ? " (top-down)" : " (bottom-up)";
    sVideo += ", " + std::to_string(ptrthebltformat->GetBitsPerPixel()) + "-bit";
    sVideo += " (" + std::to_string(GetSystemMetrics(SM_CXSCREEN)) + "x" +
              std::to_string(GetSystemMetrics(SM_CYSCREEN)) + ")";
    AddWidget<SDL2Label>(lx, y, w, rowH, sVideo, tc); y += rowH;

    std::string sSound = "Sound: ";
    switch (theMusicPlayer.GetMode()) {
        case CMusicPlayer::MUSIC_MODE::midi_only: sSound += "MIDI Music"; break;
        case CMusicPlayer::MUSIC_MODE::mixed:     sSound += "MIDI & Digital Music"; break;
        case CMusicPlayer::MUSIC_MODE::wav_only:  sSound += "Digital Music"; break;
    }
    AddWidget<SDL2Label>(lx, y, w, rowH, sSound, tc); y += rowH;

    {
        int iRate, iChannels; std::string sDriverName;
        theMusicPlayer.GetDigitalConfig(&iRate, &iChannels, sDriverName);
        std::string sv = "Audio: " + std::string(theMusicPlayer.GetVersion());
        if (iRate > 0)
            sv += " " + std::to_string(iRate) + "Hz/" + std::to_string(iChannels) + "ch, " + sDriverName;
        else sv += " {off}";
        AddWidget<SDL2Label>(lx, y, w, rowH, sv, tc); y += rowH;
    }

    AddWidget<SDL2Label>(lx, y, w, rowH,
        "CPU Speed: ~" + std::to_string(theApp.GetCpuSpeed()) +
        "  CD-ROM Speed: ~" + std::to_string(theApp.GetCdSpeed()) + "X", tc); y += rowH;

    MEMORYSTATUS ms = {}; ms.dwLength = sizeof(ms); GlobalMemoryStatus(&ms);
    const int MB = 1024 * 1024;
    AddWidget<SDL2Label>(lx, y, w, rowH,
        "Memory Physical: " + std::to_string(ms.dwAvailPhys/MB) + "M/" + std::to_string(ms.dwTotalPhys/MB) +
        "M Virtual: " + std::to_string(ms.dwAvailPageFile/MB) + "M/" + std::to_string(ms.dwTotalPageFile/MB) + "M", tc);

    AddWidget<SDL2Button>(m_x + (m_width - 90) / 2, m_y + m_height - 45, 90, 30, "OK",
                          [this]() { EndDialog(1); });
}

// ============================================================================
// SDL2AdvOptionsDialog
// ============================================================================
SDL2AdvOptionsDialog::SDL2AdvOptionsDialog(GameWindow* gameWindow)
    : SDL2Dialog(gameWindow, "Advanced Options", 400, 340) {}

void SDL2AdvOptionsDialog::OnInit() {
    int lx = m_x + 20, y = m_y + 45, w = m_width - 40, rowH = 24;

    AddWidget<SDL2Label>(lx, y, 120, rowH, "Zoom Levels:");
    m_radZoom = AddWidget<SDL2RadioGroup>(lx + 130, y, 200, rowH * 3,
        std::vector<std::string>{"All 4 levels", "3 levels", "2 levels"},
        EnGetProfileInt("Advanced", "Zoom", 2));
    if (theApp.GetFirstZoom() == 1) {
        m_radZoom->SetEnabled(0, false); m_radZoom->SetEnabled(1, false); m_radZoom->SetSelected(2);
    }
    y += rowH * 3 + 8;
    m_chkScroll = AddWidget<SDL2Checkbox>(lx, y, w, rowH, "Smooth Scrolling",
        (bool)EnGetProfileInt("Advanced", "Scroll", 0)); y += rowH + 4;
    m_chkPause = AddWidget<SDL2Checkbox>(lx, y, w, rowH, "Pause on Inactive",
        (bool)EnGetProfileInt("Advanced", "Pause", 1)); y += rowH + 4;
    m_chkNoIntro = AddWidget<SDL2Checkbox>(lx, y, w, rowH, "Skip Intro Movie",
        (bool)EnGetProfileInt("Game", "NoIntro", 0));
    AddOKCancelButtons();
}

void SDL2AdvOptionsDialog::OnOK() {
    BOOL bWarn = FALSE;
    auto check = [&](const char* sec, const char* key, int val, int def) {
        if ((int)EnGetProfileInt(sec, key, def) != val) { EnWriteProfileInt(sec, key, val); bWarn = TRUE; }
    };
    check("Advanced", "Zoom", m_radZoom->GetSelected(), 2);
    check("Advanced", "Scroll", m_chkScroll->IsChecked() ? 1 : 0, 0);
    check("Advanced", "Pause", m_chkPause->IsChecked() ? 1 : 0, 1);
    check("Game", "NoIntro", m_chkNoIntro->IsChecked() ? 1 : 0, 0);
    if (bWarn) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Second Chance",
        "You need to exit and restart Second Chance for these changes to take effect", nullptr);
    EndDialog(1);
}

// ============================================================================
// SDL2CreateSingleDialog
// ============================================================================
SDL2CreateSingleDialog::SDL2CreateSingleDialog(GameWindow* gameWindow)
    : SDL2Dialog(gameWindow, "Create Single Player Game", 480, 380) {}

void SDL2CreateSingleDialog::OnInit() {
    int lx = m_x + 20, y = m_y + 45, rowH = 24;
    int colW = (m_width - 60) / 2;
    int rx = lx + colW + 20;

    // Group boxes first (render behind widgets). Titles replace the old section labels.
    AddWidget<SDL2GroupBox>(lx - 8, y - 8, colW + 16, rowH * 6 + 8, "AI Difficulty");
    AddWidget<SDL2GroupBox>(rx - 8, y - 8, colW + 16, rowH * 3 + 8, "World Size");
    AddWidget<SDL2GroupBox>(rx - 8, y + rowH * 3 + 8, colW + 16, rowH * 5 + 8, "Starting Position");

    // AI difficulty — no separate title label, group box carries it
    int savedAi = std::max(0, std::min(3, (int)EnGetProfileInt("Create", "Difficultity", 0)));
    m_radAiLevel = AddWidget<SDL2RadioGroup>(lx, y, colW, rowH * 4,
        std::vector<std::string>{"Easy", "Moderate", "Difficult", "Impossible"}, savedAi);

    int numAi = std::max(1, std::min(20, (int)EnGetProfileInt("Create", "AiOpponents", 2)));
    AddWidget<SDL2Label>(lx, y + rowH * 4 + 4, colW - 60, rowH, "AI Players:");
    m_edtNumAi = AddWidget<SDL2EditBox>(lx + colW - 50, y + rowH * 4 + 4, 50, rowH, std::to_string(numAi));

    // World size
    int savedSize = std::max(0, std::min(2, (int)EnGetProfileInt("Create", "Size", 1)));
    m_radWorldSize = AddWidget<SDL2RadioGroup>(rx, y, colW, rowH * 3,
        std::vector<std::string>{"Small", "Medium", "Large"}, savedSize);

    // Starting position
    int savedPos = std::max(0, std::min(3, (int)EnGetProfileInt("Create", "StartPosition", 1)));
    m_radStartPos = AddWidget<SDL2RadioGroup>(rx, y + rowH * 3 + 14, colW, rowH * 4,
        std::vector<std::string>{"Minimal Civilian", "Full Civilian", "Minimal Military", "Full Military"}, savedPos);

    AddOKCancelButtons();
}

void SDL2CreateSingleDialog::OnOK() {
    m_iAiLevel = m_radAiLevel->GetSelected();
    m_iWorldSize = m_radWorldSize->GetSelected();
    m_iStartPos = m_radStartPos->GetSelected();
    m_iNumAi = atoi(m_edtNumAi->GetText().c_str());
    if (m_iNumAi <= 0) m_iNumAi = 1;
    EndDialog(1);
}

// ============================================================================
// SDL2PickRaceDialog
// ============================================================================
SDL2PickRaceDialog::SDL2PickRaceDialog(GameWindow* gameWindow)
    : SDL2Dialog(gameWindow, "Pick Your Race", 580, 460) {}

SDL2PickRaceDialog::~SDL2PickRaceDialog() {
    for (auto* surf : m_racePictures)
        if (surf) SDL_FreeSurface(surf);
}

void SDL2PickRaceDialog::OnInit() {
    int lx = m_x + 20, y = m_y + 45, w = m_width - 40;

    // Player name
    AddWidget<SDL2Label>(lx, y, 60, 24, "Name:");
    std::string savedName = EnGetProfileStdString("Create", "Name", "");
    m_edtName = AddWidget<SDL2EditBox>(lx + 65, y, w - 65, 24, savedName,
        [this](const std::string& name) { OnNameChanged(name); });
    y += 34;

    // Race list on the left — fill from here down to the OK/Cancel buttons
    int listW = 160;
    int bottomY = m_y + m_height - 55;  // leave room for buttons
    int listH = bottomY - y;
    m_lstRaces = AddWidget<SDL2Listbox>(lx, y, listW, listH,
        [this](int idx) { OnRaceSelected(idx); },
        [this](int idx) { OnRaceSelected(idx); if (m_btnOK && m_btnOK->IsEnabled()) OnOK(); });

    // Pre-convert all race pictures to SDL surfaces
    int numRaces = CRaceDef::GetNumRaces();
    m_racePictures.resize(numRaces, nullptr);
    for (int i = 0; i < numRaces; i++) {
        m_lstRaces->AddItem(ptheRaces[i].GetLine(), (void*)&ptheRaces[i]);
        CDIB* pPic = ptheRaces[i].GetPicture();
        if (pPic)
            m_racePictures[i] = SDL2MainMenu::CreateSurfaceFromDIB(pPic);
    }

    // Right side: picture on top, description below
    int rightX = lx + listW + 15;
    int rightW = w - listW - 15;
    int picH = 200;  // larger portrait

    // Race picture
    m_imgPicture = AddWidget<SDL2Image>(rightX, y, rightW, picH);

    // Race description (wrapped text below picture, fills remaining space) — blue text
    int descY = y + picH + 8;
    int descH = bottomY - descY;
    m_lblDesc = AddWidget<SDL2Label>(rightX, descY, rightW, descH, "", SDL_Color{48, 58, 148, 255});
    m_lblDesc->SetWrapped(true);

    // OK / Cancel
    m_btnOK = AddWidget<SDL2Button>(m_x + m_width / 2 - 100, m_y + m_height - 45, 90, 30, "OK",
                                     [this]() { OnOK(); });
    m_btnOK->SetEnabled(false);
    AddWidget<SDL2Button>(m_x + m_width / 2 + 10, m_y + m_height - 45, 90, 30, "Cancel",
                          [this]() { OnCancel(); });
}

void SDL2PickRaceDialog::OnRaceSelected(int index) {
    if (index >= 0 && index < CRaceDef::GetNumRaces()) {
        m_iSelectedRace = index;
        m_lblDesc->SetText(ptheRaces[index].GetDesc());

        // Update race picture
        if (index < (int)m_racePictures.size() && m_racePictures[index])
            m_imgPicture->SetSurface(m_racePictures[index], false);  // not owned by widget
        else
            m_imgPicture->Clear();
    }
    UpdateOKButton();
}

void SDL2PickRaceDialog::OnNameChanged(const std::string& name) {
    m_playerName = name;
    UpdateOKButton();
}

void SDL2PickRaceDialog::UpdateOKButton() {
    bool valid = !m_edtName->GetText().empty() && m_iSelectedRace >= 0;
    if (m_btnOK) m_btnOK->SetEnabled(valid);
}

void SDL2PickRaceDialog::OnOK() {
    if (m_iSelectedRace < 0 || m_edtName->GetText().empty()) return;
    m_playerName = m_edtName->GetText();
    EnWriteProfileString("Create", "Name", m_playerName.c_str());
    EndDialog(1);
}

// ============================================================================
// SDL2ScenarioDialog
// ============================================================================
SDL2ScenarioDialog::SDL2ScenarioDialog(GameWindow* gameWindow)
    : SDL2Dialog(gameWindow, "Training Scenario", 400, 300) {}

void SDL2ScenarioDialog::OnInit() {
    int lx = m_x + 20, y = m_y + 45, rowH = 24, colW = (m_width - 60) / 2;
    int rx = lx + colW + 20;

    // Group boxes first (render behind other widgets)
    AddWidget<SDL2GroupBox>(lx - 8, y - 8, colW + 16, rowH * 4 + 8, "AI Difficulty");
    AddWidget<SDL2GroupBox>(rx - 8, y - 8, colW + 16, rowH * 3 + 8, "World Size");

    m_radAiLevel = AddWidget<SDL2RadioGroup>(lx, y, colW, rowH * 4,
        std::vector<std::string>{"Easy", "Moderate", "Difficult", "Impossible"}, 0);

    m_radWorldSize = AddWidget<SDL2RadioGroup>(rx, y, colW, rowH * 3,
        std::vector<std::string>{"Small", "Medium", "Large"}, 1);

    AddOKCancelButtons();
}

void SDL2ScenarioDialog::OnOK() {
    m_iAiLevel = m_radAiLevel->GetSelected();
    m_iWorldSize = m_radWorldSize->GetSelected();
    EndDialog(1);
}

// ============================================================================
// Flow helpers - these use CCreateSingle/CCreateScenario under the hood
// so that ReadyToCreate() and all the game setup logic still works.
// We just replace the MFC dialog UI with SDL2 dialogs.
// ============================================================================

// Helper: render wallpaper-only background before showing a dialog
static void ShowWallpaperBackground(GameWindow* gameWindow) {
    if (theApp.m_sdlMainMenu && theApp.m_sdlMainMenu->IsInitialized())
        theApp.m_sdlMainMenu->RenderWallpaperOnly();
}

bool SDL2_RunCreateSinglePlayerFlow(GameWindow* gameWindow) {
    // Create the game object FIRST - its constructor loads race data (CRaceDef::Init)
    ASSERT(theApp.m_pCreateGame == NULL);
    CCreateSingle* pCreate = new CCreateSingle();
    theApp.m_pCreateGame = pCreate;

    // Initialize game state (same as CCreateSingle::Init minus the MFC dialog)
    theGame.ctor();
    theGame.SetServer(TRUE);
    theGame._SetIsNetGame(FALSE);
    theGame.Open(TRUE);

    // Render wallpaper-only (no menu buttons) as the background behind dialogs
    ShowWallpaperBackground(gameWindow);

    // Step 1: Show create dialog
    SDL2CreateSingleDialog createDlg(gameWindow);
    if (createDlg.DoModal() != 1) {
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    // Refresh wallpaper background for next dialog
    ShowWallpaperBackground(gameWindow);

    // Step 2: Show pick race dialog
    SDL2PickRaceDialog raceDlg(gameWindow);
    if (raceDlg.DoModal() != 1) {
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    // Raise the main window so the progress dialog (rendered on it) is visible
    // after the ALWAYS_ON_TOP DoModal dialogs are destroyed.
    if (gameWindow->GetWindow())
        SDL_RaiseWindow(gameWindow->GetWindow());

    // Step 3: Set parameters from our SDL2 dialogs (same as CDlgCreateSingle::OnOK)
    theGame.m_iAi = pCreate->m_iAi = createDlg.m_iAiLevel;
    theGame.m_iSize = pCreate->m_iSize = createDlg.m_iWorldSize;
    theGame.m_iPos = pCreate->m_iPos = createDlg.m_iStartPos;
    pCreate->m_iNumAi = createDlg.m_iNumAi;
    pCreate->m_iNet = -1;

    EnWriteProfileInt("Create", "Difficultity", createDlg.m_iAiLevel);
    EnWriteProfileInt("Create", "Size", createDlg.m_iWorldSize);
    EnWriteProfileInt("Create", "AiOpponents", createDlg.m_iNumAi);
    EnWriteProfileInt("Create", "StartPosition", createDlg.m_iStartPos);

    // Step 4: Set player race (same as CDlgPickRace::OnOK)
    CRaceDef* pRace = &ptheRaces[raceDlg.m_iSelectedRace];
    pCreate->m_sName = raceDlg.m_playerName.c_str();
    pCreate->m_sRace = pRace->GetLine();

    theGame.GetMe()->SetName(raceDlg.m_playerName.c_str());
    theGame.GetMe()->m_InitData.Set(pRace, pCreate->m_iPos);
    pCreate->GetNew()->m_InitData.Set(pRace, pCreate->m_iPos);

    // Step 5: Start game creation via the real game logic path
    // ReadyToCreate() creates AI players and calls StartCreateWorld()
    try {
        theGame.IncTry();
        theApp.ReadyToCreate();
        theGame.DecTry();
    } catch (int iNum) {
        CatchNum(iNum);
        theApp.CloseWorld();
        return false;
    } catch (...) {
        CatchOther();
        theApp.CloseWorld();
        return false;
    }

    return true;
}

bool SDL2_RunCreateScenarioFlow(GameWindow* gameWindow) {
    ASSERT(theApp.m_pCreateGame == NULL);
    CCreateScenario* pCreate = new CCreateScenario();
    theApp.m_pCreateGame = pCreate;

    theGame.ctor();
    theGame.SetServer(TRUE);
    theGame._SetIsNetGame(FALSE);
    theGame.Open(TRUE);

    ShowWallpaperBackground(gameWindow);

    SDL2ScenarioDialog scnDlg(gameWindow);
    if (scnDlg.DoModal() != 1) {
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    ShowWallpaperBackground(gameWindow);

    SDL2PickRaceDialog raceDlg(gameWindow);
    if (raceDlg.DoModal() != 1) {
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    // Raise main window so progress dialog is visible
    if (gameWindow->GetWindow())
        SDL_RaiseWindow(gameWindow->GetWindow());

    theGame.m_iAi = pCreate->m_iAi = scnDlg.m_iAiLevel;
    theGame.m_iSize = pCreate->m_iSize = scnDlg.m_iWorldSize;
    theGame.m_iPos = pCreate->m_iPos = 1;
    pCreate->m_iNumAi = 1;
    pCreate->m_iNet = -1;

    CRaceDef* pRace = &ptheRaces[raceDlg.m_iSelectedRace];
    pCreate->m_sName = raceDlg.m_playerName.c_str();
    pCreate->m_sRace = pRace->GetLine();

    theGame.GetMe()->SetName(raceDlg.m_playerName.c_str());
    theGame.GetMe()->m_InitData.Set(pRace, 1);
    pCreate->GetNew()->m_InitData.Set(pRace, 1);

    try {
        theGame.IncTry();
        theApp.ReadyToCreate();
        theGame.DecTry();
    } catch (int iNum) {
        CatchNum(iNum);
        theApp.CloseWorld();
        return false;
    } catch (...) {
        CatchOther();
        theApp.CloseWorld();
        return false;
    }

    return true;
}

// ============================================================================
// SDL2PickPlayerDialog (for loaded games)
// ============================================================================
SDL2PickPlayerDialog::SDL2PickPlayerDialog(GameWindow* gameWindow)
    : SDL2Dialog(gameWindow, "Pick Your Player", 520, 420) {}

void SDL2PickPlayerDialog::OnInit() {
    int lx = m_x + 20, y = m_y + 45, w = m_width - 40;

    AddWidget<SDL2Label>(lx, y, 60, 24, "Name:");
    std::string savedName = EnGetProfileStdString("Create", "Name", "");
    m_edtName = AddWidget<SDL2EditBox>(lx + 65, y, w - 65, 24, savedName,
        [this](const std::string& name) { OnNameChanged(name); });
    y += 34;

    int listW = 180, bottomY = m_y + m_height - 55, listH = bottomY - y;
    m_lstPlayers = AddWidget<SDL2Listbox>(lx, y, listW, listH,
        [this](int idx) { OnPlayerSelected(idx); },
        [this](int idx) { OnPlayerSelected(idx); if (m_btnOK && m_btnOK->IsEnabled()) OnOK(); });

    POSITION pos;
    for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL;) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        CNetPlyrJoin* pData = CNetPlyrJoin::Alloc(pPlr);
        PlayerInfo pi;
        pi.plyrNum = pData->m_iPlyrNum;
        pi.available = pData->m_bAvail;
        pi.numBldgs = pData->m_iNumBldgs;
        pi.numVeh = pData->m_iNumVeh;
        pi.name = pPlr->GetName();
        m_players.push_back(pi);
        m_lstPlayers->AddItem(pi.name);
        delete[] (char*)pData;
    }

    int rightX = lx + listW + 15, rightW = w - listW - 15;
    m_lblDesc = AddWidget<SDL2Label>(rightX, y, rightW, listH, "", SDL_Color{180, 190, 200, 255});
    m_lblDesc->SetWrapped(true);

    m_btnOK = AddWidget<SDL2Button>(m_x + m_width / 2 - 100, m_y + m_height - 45, 90, 30, "OK",
                                     [this]() { OnOK(); });
    m_btnOK->SetEnabled(false);
    AddWidget<SDL2Button>(m_x + m_width / 2 + 10, m_y + m_height - 45, 90, 30, "Cancel",
                          [this]() { OnCancel(); });

    for (int i = 0; i < (int)m_players.size(); i++) {
        CPlayer* pPlr = theGame.GetPlayerByPlyr(m_players[i].plyrNum);
        if (pPlr && pPlr == theGame._GetMe()) {
            m_lstPlayers->SetSelected(i);
            m_edtName->SetText(m_players[i].name);
            OnPlayerSelected(i);
            break;
        }
    }
}

void SDL2PickPlayerDialog::OnPlayerSelected(int index) {
    if (index < 0 || index >= (int)m_players.size()) return;
    auto& pi = m_players[index];
    std::string desc = "Buildings: " + std::to_string(pi.numBldgs) + "\nVehicles: " + std::to_string(pi.numVeh);
    m_lblDesc->SetText(desc);
    UpdateOKButton();
}

void SDL2PickPlayerDialog::OnNameChanged(const std::string&) { UpdateOKButton(); }

void SDL2PickPlayerDialog::UpdateOKButton() {
    bool valid = !m_edtName->GetText().empty() && m_lstPlayers->GetSelected() >= 0;
    if (m_btnOK) m_btnOK->SetEnabled(valid);
}

void SDL2PickPlayerDialog::OnOK() {
    int sel = m_lstPlayers->GetSelected();
    if (sel < 0 || m_edtName->GetText().empty()) return;
    m_iSelectedPlyrNum = m_players[sel].plyrNum;
    m_playerName = m_edtName->GetText();
    EnWriteProfileString("Create", "Name", m_playerName.c_str());
    EndDialog(1);
}

// ============================================================================
// SDL2CreateNetDialog
// ============================================================================
SDL2CreateNetDialog::SDL2CreateNetDialog(GameWindow* gameWindow)
    : SDL2Dialog(gameWindow, "Create Network Game (TCP/IP)", 520, 440) {}

void SDL2CreateNetDialog::OnInit() {
    int lx = m_x + 20, y = m_y + 45, rowH = 24, colW = (m_width - 60) / 2, rx = lx + colW + 20;

    AddWidget<SDL2Label>(lx, y, 100, rowH, "Game Name:");
    m_edtGameName = AddWidget<SDL2EditBox>(lx + 105, y, colW - 105, rowH, "My Game");
    y += rowH + 4;
    AddWidget<SDL2Label>(lx, y, 100, rowH, "Your Name:");
    std::string savedName = EnGetProfileStdString("Create", "Name", "");
    m_edtPlayerName = AddWidget<SDL2EditBox>(lx + 105, y, colW - 105, rowH, savedName);
    AddWidget<SDL2Label>(rx, m_y + 45, 50, rowH, "Port:");
    m_edtPort = AddWidget<SDL2EditBox>(rx + 55, m_y + 45, 80, rowH, "2346");
    y += rowH + 8;

    AddWidget<SDL2Label>(lx, y, colW, rowH, "AI Difficulty:");
    m_radAiLevel = AddWidget<SDL2RadioGroup>(lx, y + rowH, colW, rowH * 4,
        std::vector<std::string>{"Easy", "Moderate", "Difficult", "Impossible"},
        std::max(0, std::min(3, (int)EnGetProfileInt("Create", "Difficultity", 0))));
    int numAi = std::max(0, std::min(20, (int)EnGetProfileInt("Create", "AiOpponents", 2)));
    AddWidget<SDL2Label>(lx, y + rowH * 5 + 8, colW - 60, rowH, "AI Players:");
    m_edtNumAi = AddWidget<SDL2EditBox>(lx + colW - 50, y + rowH * 5 + 8, 50, rowH, std::to_string(numAi));

    AddWidget<SDL2Label>(rx, y, colW, rowH, "World Size:");
    m_radWorldSize = AddWidget<SDL2RadioGroup>(rx, y + rowH, colW, rowH * 3,
        std::vector<std::string>{"Small", "Medium", "Large"},
        std::max(0, std::min(2, (int)EnGetProfileInt("Create", "Size", 1))));
    AddWidget<SDL2Label>(rx, y + rowH * 4 + 10, colW, rowH, "Starting Condition:");
    m_radStartPos = AddWidget<SDL2RadioGroup>(rx, y + rowH * 5 + 10, colW, rowH * 4,
        std::vector<std::string>{"Minimal Civilian", "Full Civilian", "Minimal Military", "Full Military"},
        std::max(0, std::min(3, (int)EnGetProfileInt("Create", "StartPosition", 1))));

    AddOKCancelButtons();
}

void SDL2CreateNetDialog::OnOK() {
    m_gameName = m_edtGameName->GetText();
    m_playerName = m_edtPlayerName->GetText();
    if (m_gameName.empty() || m_playerName.empty()) return;
    m_iAiLevel = m_radAiLevel->GetSelected();
    m_iWorldSize = m_radWorldSize->GetSelected();
    m_iStartPos = m_radStartPos->GetSelected();
    m_iNumAi = atoi(m_edtNumAi->GetText().c_str());
    if (m_iNumAi < 0) m_iNumAi = 0;
    m_iPort = atoi(m_edtPort->GetText().c_str());
    if (m_iPort <= 0) m_iPort = 2346;
    EndDialog(1);
}

// ============================================================================
// SDL2JoinNetDialog
// ============================================================================
SDL2JoinNetDialog::SDL2JoinNetDialog(GameWindow* gameWindow)
    : SDL2Dialog(gameWindow, "Join Network Game (TCP/IP)", 420, 240) {}

void SDL2JoinNetDialog::OnInit() {
    int lx = m_x + 20, y = m_y + 45, w = m_width - 40, rowH = 28;
    AddWidget<SDL2Label>(lx, y, 110, rowH, "Your Name:");
    std::string savedName = EnGetProfileStdString("Create", "Name", "");
    m_edtPlayerName = AddWidget<SDL2EditBox>(lx + 115, y, w - 115, 24, savedName);
    y += rowH + 4;
    AddWidget<SDL2Label>(lx, y, 110, rowH, "Server Address:");
    m_edtServerAddr = AddWidget<SDL2EditBox>(lx + 115, y, w - 115, 24, "localhost");
    y += rowH + 4;
    AddWidget<SDL2Label>(lx, y, 110, rowH, "Port:");
    m_edtPort = AddWidget<SDL2EditBox>(lx + 115, y, 80, 24, "2346");
    AddOKCancelButtons();
}

void SDL2JoinNetDialog::OnOK() {
    m_playerName = m_edtPlayerName->GetText();
    m_serverAddr = m_edtServerAddr->GetText();
    if (m_playerName.empty() || m_serverAddr.empty()) return;
    m_iPort = atoi(m_edtPort->GetText().c_str());
    if (m_iPort <= 0) m_iPort = 2346;
    EndDialog(1);
}

// ============================================================================
// Load Single Player flow
// ============================================================================
bool SDL2_RunLoadSinglePlayerFlow(GameWindow* gameWindow) {
    ASSERT(theApp.m_pCreateGame == NULL);
    CCreateLoadSingle* pCreate = new CCreateLoadSingle();
    theApp.m_pCreateGame = pCreate;

    theGame.ctor();
    theGame.SetServer(TRUE);
    theGame._SetIsNetGame(FALSE);

    ShowWallpaperBackground(gameWindow);

    if (theGame.LoadGame(theApp.m_pMainWnd, FALSE) != IDOK) {
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    if (theGame.GetScenario() >= 0) {
        theGame.SetHP(TRUE); theGame.SetAI(TRUE);
        theGame.GetMe()->SetState(CPlayer::ready);
        try {
            theGame.IncTry();
            if (theGame.StartGame(FALSE) != IDOK) { theGame.DecTry(); throw(0); }
            theGame.DecTry();
        } catch (...) {
            theApp.CloseWorld();
            delete theApp.m_pCreateGame;
            theApp.m_pCreateGame = NULL;
            return false;
        }
        return true;
    }

    ShowWallpaperBackground(gameWindow);
    SDL2PickPlayerDialog pickDlg(gameWindow);
    if (pickDlg.DoModal() != 1) {
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    CPlayer* pPlr = theGame.GetPlayerByPlyr(pickDlg.m_iSelectedPlyrNum);
    if (!pPlr) { theGame.Close(); delete theApp.m_pCreateGame; theApp.m_pCreateGame = NULL; return false; }

    theGame.SetHP(TRUE); theGame.SetScenario(-1);
    if (pPlr != theGame._GetMe()) {
        theGame._SetMe(pPlr);
        if (theGame.AmServer()) theGame._SetServer(pPlr);
    }
    pPlr->SetState(CPlayer::ready);
    pPlr->SetName(pickDlg.m_playerName.c_str());

    POSITION pos;
    for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL;) {
        CPlayer* pP = theGame.GetAll().GetNext(pos);
        if (pP->IsMe()) { pP->SetRelations(RELATIONS_ALLIANCE); pP->SetTheirRelations(RELATIONS_ALLIANCE); }
        else { pP->SetRelations(RELATIONS_NEUTRAL); pP->SetTheirRelations(RELATIONS_NEUTRAL); }
    }

    // Raise main window so progress dialog is visible
    if (gameWindow->GetWindow())
        SDL_RaiseWindow(gameWindow->GetWindow());

    try {
        theGame.IncTry();
        pCreate->ClosePick();
        if (theGame.StartGame(FALSE) != IDOK) { theGame.DecTry(); throw(0); }
        theGame.DecTry();
    } catch (int iNum) { CatchNum(iNum); theApp.CloseWorld(); return false; }
    catch (...) { CatchOther(); theApp.CloseWorld(); return false; }

    return true;
}

// ============================================================================
// Create Network Game flow (TCP/IP only)
// ============================================================================
bool SDL2_RunCreateNetworkFlow(GameWindow* gameWindow) {
    ShowWallpaperBackground(gameWindow);
    SDL2CreateNetDialog createDlg(gameWindow);
    if (createDlg.DoModal() != 1) return false;

    ASSERT(theApp.m_pCreateGame == NULL);
    CCreateMulti* pCreate = new CCreateMulti();
    theApp.m_pCreateGame = pCreate;

    theGame.ctor(); theGame.SetServer(TRUE); theGame._SetIsNetGame(TRUE); theGame.Open(TRUE);

    theGame.m_iAi = pCreate->m_iAi = createDlg.m_iAiLevel;
    theGame.m_iSize = pCreate->m_iSize = createDlg.m_iWorldSize;
    theGame.m_iPos = pCreate->m_iPos = createDlg.m_iStartPos;
    pCreate->m_iNumAi = createDlg.m_iNumAi;
    pCreate->m_iNet = 0;

    EnWriteProfileInt("Create", "Difficultity", createDlg.m_iAiLevel);
    EnWriteProfileInt("Create", "Size", createDlg.m_iWorldSize);
    EnWriteProfileInt("Create", "AiOpponents", createDlg.m_iNumAi);
    EnWriteProfileInt("Create", "StartPosition", createDlg.m_iStartPos);

    std::string sPort = std::to_string(createDlg.m_iPort);
    WritePrivateProfileString("TCP", "WellKnownPort", sPort.c_str(), "vdmplay.ini");

    if (theNet.OpenServer(VPT_TCP, theApp.m_wndMain.m_hWnd,
                          (char*)createDlg.m_gameName.c_str(), NULL, NULL)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Second Chance",
            "Failed to open TCP/IP server.", nullptr);
        theGame.Close(); delete theApp.m_pCreateGame; theApp.m_pCreateGame = NULL;
        return false;
    }

    ShowWallpaperBackground(gameWindow);
    SDL2PickRaceDialog raceDlg(gameWindow);
    if (raceDlg.DoModal() != 1) {
        theNet.Close(FALSE); theGame.Close();
        delete theApp.m_pCreateGame; theApp.m_pCreateGame = NULL;
        return false;
    }

    CRaceDef* pRace = &ptheRaces[raceDlg.m_iSelectedRace];
    pCreate->m_sName = raceDlg.m_playerName.c_str();
    pCreate->m_sRace = pRace->GetLine();
    theGame.GetMe()->SetName(raceDlg.m_playerName.c_str());
    theGame.GetMe()->m_InitData.Set(pRace, pCreate->m_iPos);
    pCreate->GetNew()->m_InitData.Set(pRace, pCreate->m_iPos);

    // Raise main window so progress dialog is visible
    if (gameWindow->GetWindow())
        SDL_RaiseWindow(gameWindow->GetWindow());

    try {
        theGame.IncTry(); theApp.ReadyToCreate(); theGame.DecTry();
    } catch (int iNum) { CatchNum(iNum); theApp.CloseWorld(); return false; }
    catch (...) { CatchOther(); theApp.CloseWorld(); return false; }
    return true;
}

// ============================================================================
// Join Network Game flow (TCP/IP only)
// ============================================================================
bool SDL2_RunJoinNetworkFlow(GameWindow* gameWindow) {
    ShowWallpaperBackground(gameWindow);
    SDL2JoinNetDialog joinDlg(gameWindow);
    if (joinDlg.DoModal() != 1) return false;

    WritePrivateProfileString("TCP", "ServerAddress", joinDlg.m_serverAddr.c_str(), "vdmplay.ini");
    std::string sPort2 = std::to_string(joinDlg.m_iPort);
    WritePrivateProfileString("TCP", "WellKnownPort", sPort2.c_str(), "vdmplay.ini");

    // Join flow requires async session enumeration — delegate to MFC for now
    ASSERT(theApp.m_pCreateGame == NULL);
    CJoinMulti* pJoin = new CJoinMulti();
    theApp.m_pCreateGame = pJoin;
    pJoin->Init();
    return true;
}

// ============================================================================
// Load Network Game flow
// ============================================================================
bool SDL2_RunLoadNetworkFlow(GameWindow* gameWindow) {
    ShowWallpaperBackground(gameWindow);
    ASSERT(theApp.m_pCreateGame == NULL);
    theApp.m_pCreateGame = new CCreateLoadMulti();
    theApp.m_pCreateGame->Init();
    return true;
}

// ============================================================================
// SDL2 Credits - Full-screen scrolling text
// ============================================================================
void SDL2_RunCredits(GameWindow* gameWindow) {
    if (!gameWindow || !gameWindow->GetWindow()) return;

    SDL_Window* window = gameWindow->GetWindow();
    int winW = gameWindow->GetWidth();
    int winH = gameWindow->GetHeight();

    struct CreditLine { int align, size, hasReturn; std::string text; };
    std::vector<CreditLine> lines;

    try {
        CMmio* pMmio = theDataFile.OpenAsMMIO("create", "CRAT");
        pMmio->DescendRiff('C', 'R', 'A', 'T');
        pMmio->DescendList('C', 'R', 'E', 'D');
        pMmio->DescendChunk('N', 'U', 'M', 'L');
        int numLines = pMmio->ReadShort();
        pMmio->AscendChunk();
        lines.resize(numLines);
        for (int i = 0; i < numLines; i++) {
            pMmio->DescendChunk('L', 'I', 'N', 'E');
            lines[i].align = pMmio->ReadShort();
            lines[i].size = pMmio->ReadShort();
            std::string sText; pMmio->ReadString(sText);
            lines[i].text = sText;
            lines[i].hasReturn = pMmio->ReadShort();
            pMmio->AscendChunk();
        }
        delete pMmio;
    } catch (...) { return; }

    if (lines.empty()) return;
    theMusicPlayer.PlayExclusiveMusic(MUSIC::GetID(MUSIC::credits));

    int baseHt = std::max(8, winW / 120);
    TTF_Font* fonts[3] = {};
    static const char* fc[] = { "C:\\Windows\\Fonts\\BKANT.TTF", "C:\\Windows\\Fonts\\BOOKOS.TTF",
                                "C:\\Windows\\Fonts\\times.ttf", nullptr };
    const char* fp = nullptr;
    for (int i = 0; fc[i]; i++) { FILE* f = fopen(fc[i], "rb"); if (f) { fclose(f); fp = fc[i]; break; } }
    if (!fp) return;
    fonts[0] = TTF_OpenFont(fp, 3 * baseHt);
    fonts[1] = TTF_OpenFont(fp, 4 * baseHt);
    fonts[2] = TTF_OpenFont(fp, 5 * baseHt);
    if (!fonts[0]) return;

    struct RenderedLine { SDL_Surface* surface; int align, height; };
    std::vector<RenderedLine> rendered;
    SDL_Color white = {255, 255, 255, 255};
    int totalHeight = 0, marginX = winW / 5;

    for (auto& cl : lines) {
        int fi = std::max(0, std::min(2, cl.size));
        SDL_Surface* surf = cl.text.empty() ? nullptr : TTF_RenderText_Blended(fonts[fi], cl.text.c_str(), white);
        int h = surf ? surf->h : baseHt;
        int lineH = h + h / 5;
        if (!cl.hasReturn) lineH += h / 3;
        rendered.push_back({surf, cl.align, lineH});
        totalHeight += lineH;
    }

    float scrollY = (float)winH;
    Uint32 lastTick = SDL_GetTicks();
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { running = false; ::PostQuitMessage(0); break; }
            if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) { running = false; break; }
        }
        if (!running) break;

        Uint32 now = SDL_GetTicks();
        scrollY -= 0.8f * (float)(now - lastTick) / 16.0f;
        lastTick = now;
        if (scrollY < -(float)totalHeight) { running = false; break; }

        SDL_Surface* ws = SDL_GetWindowSurface(window);
        if (!ws) break;
        SDL_FillRect(ws, nullptr, SDL_MapRGB(ws->format, 0, 0, 0));

        float drawY = scrollY;
        int textW = winW - 2 * marginX;
        for (auto& rl : rendered) {
            if (drawY + rl.height > 0 && drawY < winH && rl.surface) {
                int dx = marginX;
                if (rl.align == 1) dx += (textW - rl.surface->w) / 2;
                else if (rl.align == 2) dx += textW - rl.surface->w;
                SDL_Rect dst = {dx, (int)drawY, 0, 0};
                SDL_BlitSurface(rl.surface, nullptr, ws, &dst);
            }
            drawY += rl.height;
        }

        SDL_UpdateWindowSurface(window);
        SDL_Delay(16);
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { running = false; break; }
            ::TranslateMessage(&msg); ::DispatchMessage(&msg);
        }
    }

    for (auto& rl : rendered) if (rl.surface) SDL_FreeSurface(rl.surface);
    for (int i = 0; i < 3; i++) if (fonts[i]) TTF_CloseFont(fonts[i]);
    theMusicPlayer.SoundsOff();
}
