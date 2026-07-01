#include "stdafx.h"
#include "SDL2Dialogs.h"
#include "RenderBackend.h"   // RenderBackendOpenGLAvailable() — grey out OpenGL until done
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "SDL2CreateStatus.h"   // GetCreateStatus()->Hide() before the pick-player dialog
#include "en_harness.h"   // HarnessPendingLoadPath (headless load skips the pick-player modal)
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
#include "SDL2GameDialogs.h"   // SDL2Chat_Send / Count / Line for lobby chat

#undef min
#undef max

// World-type presets shown in the Create dialogs. The list index is the EWorldType
// value (terrain.h) and is stored on m_iWorldType / synced via CNetStart, so order
// here must match the enum. Entry 0 keeps the original random behavior.
static const std::vector<std::string>& WorldTypeLabels() {
    static const std::vector<std::string> labels = {
        "Default (random)",
        "Big Ocean",
        "Strip Ocean",
        "Scattered Ocean",
        "Islands",
        "Mountain Planet",
        "Badlands Planet",
        "Desert Planet",
    };
    return labels;
}

// Fill a (freshly created) World Type listbox and preselect the saved choice.
static void PopulateWorldTypeList(SDL2Listbox* lst) {
    for (const auto& s : WorldTypeLabels())
        lst->AddItem(s);
    int sel = (int)EnGetProfileInt("Create", "WorldType", 0);
    if (sel < 0 || sel >= (int)WorldTypeLabels().size()) sel = 0;
    lst->SetSelected(sel);
    lst->EnsureVisible(sel);
}

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
    : SDL2Dialog(gameWindow, "Advanced Options", 400, 430) {}

void SDL2AdvOptionsDialog::OnInit() {
    int lx = m_x + 20, y = m_y + 45, w = m_width - 40, rowH = 24;

    AddWidget<SDL2Label>(lx, y, 120, rowH, "Zoom Levels:");
    m_radZoom = AddWidget<SDL2RadioGroup>(lx + 130, y, 200, rowH * 3,
        std::vector<std::string>{"All 4 levels", "3 levels", "2 levels"},
        EnGetProfileInt("Advanced", "Zoom", 0));   // default: All 4 levels (matches the engine's auto-zoom default)
    if (theApp.GetFirstZoom() == 1) {
        m_radZoom->SetEnabled(0, false); m_radZoom->SetEnabled(1, false); m_radZoom->SetSelected(2);
    }
    y += rowH * 3 + 8;

    // Renderer backend: 0=Software, 1=SDL2, 2=OpenGL. OpenGL is greyed out until the
    // GL backend is finished (RenderBackendOpenGLAvailable()); selecting it has no
    // effect (GetRenderBackend maps 2→SDL2). Restart required — handled by OnOK's
    // bWarn path. See plans/opengl-rendering-backend.md.
    AddWidget<SDL2Label>(lx, y, 120, rowH, "Renderer:");
    m_radRenderer = AddWidget<SDL2RadioGroup>(lx + 130, y, 200, rowH * 3,
        std::vector<std::string>{"Software", "SDL2 (GPU)", "OpenGL"},
        std::max(0, std::min(2, (int)GetRenderBackend())));   // show the engine's ACTUAL backend (SDL2 by default on Linux/mac), not a hardcoded Software
    if (!RenderBackendOpenGLAvailable())
        m_radRenderer->SetEnabled(2, false);   // OpenGL not implemented yet
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
    check("Advanced", "Zoom", m_radZoom->GetSelected(), 0);
    check("Advanced", "Renderer", m_radRenderer->GetSelected(), (int)GetRenderBackend());
    check("Advanced", "Scroll", m_chkScroll->IsChecked() ? 1 : 0, 0);
    check("Advanced", "Pause", m_chkPause->IsChecked() ? 1 : 0, 1);
    check("Game", "NoIntro", m_chkNoIntro->IsChecked() ? 1 : 0, 0);
    // Parent the message box to THIS dialog's window (not nullptr): with a null
    // parent the OS box opened UNDECORATED-and-unfocusable BEHIND the fullscreen
    // game window on Linux/XWayland/Mutter, so it could never be clicked — the game
    // blocked forever in SDL_ShowSimpleMessageBox and looked HUNG (operator-reported
    // after changing an Advanced setting). Transient-for the dialog window stacks it
    // ABOVE the fullscreen game (same as the panels' transient-above-fullscreen) so
    // it's visible and dismissable.
    if (bWarn) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Second Chance",
        "You need to exit and restart Second Chance for these changes to take effect",
        m_gameWindow ? m_gameWindow->GetWindow() : nullptr);
    EndDialog(1);
}

// ============================================================================
// SDL2CreateSingleDialog
// ============================================================================
SDL2CreateSingleDialog::SDL2CreateSingleDialog(GameWindow* gameWindow)
    : SDL2Dialog(gameWindow, "Create Single Player Game", 480, 530) {}

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

    // World type — full-width preset list below the two columns
    int wtY = y + rowH * 8 + 28;
    AddWidget<SDL2GroupBox>(lx - 8, wtY - 8, m_width - 24, rowH * 5 + 16, "World Type");
    m_lstWorldType = AddWidget<SDL2Listbox>(lx, wtY + 4, m_width - 40, rowH * 5);
    PopulateWorldTypeList(m_lstWorldType);

    // Rivers — density slider (0 = none, 60 = classic baseline, 100 = lots).
    // Value feeds MakeRiversFlow's accumulation threshold; synced via CNetStart.
    int rvY = wtY + rowH * 5 + 18;
    int savedRivers = std::max(0, std::min(100, (int)EnGetProfileInt("Create", "Rivers", 60)));
    m_lblRivers = AddWidget<SDL2Label>(lx, rvY, 110, rowH, "Rivers: " + std::to_string(savedRivers) + "%");
    m_sldRivers = AddWidget<SDL2Slider>(lx + 115, rvY, m_width - 40 - 115, rowH, 0, 100, savedRivers,
        [this](int v) { if (m_lblRivers) m_lblRivers->SetText("Rivers: " + std::to_string(v) + "%"); });
    if (m_sldRivers) m_sldRivers->SetShowValue(false);  // label shows the value; slider's own readout overflows the dialog

    // Ocean — size slider (0 = none, 50 = baseline ~= current average, 100 = lots).
    // Value biases GenerateOcean's random block count; keeps randomness.
    int ocY = rvY + rowH + 6;
    int savedOcean = std::max(0, std::min(100, (int)EnGetProfileInt("Create", "Ocean", 50)));
    m_lblOcean = AddWidget<SDL2Label>(lx, ocY, 110, rowH, "Ocean: " + std::to_string(savedOcean) + "%");
    m_sldOcean = AddWidget<SDL2Slider>(lx + 115, ocY, m_width - 40 - 115, rowH, 0, 100, savedOcean,
        [this](int v) { if (m_lblOcean) m_lblOcean->SetText("Ocean: " + std::to_string(v) + "%"); });
    if (m_sldOcean) m_sldOcean->SetShowValue(false);  // label shows the value; slider's own readout overflows the dialog

    AddOKCancelButtons();
}

void SDL2CreateSingleDialog::OnOK() {
    m_iAiLevel = m_radAiLevel->GetSelected();
    m_iWorldSize = m_radWorldSize->GetSelected();
    m_iStartPos = m_radStartPos->GetSelected();
    m_iWorldType = m_lstWorldType ? std::max(0, m_lstWorldType->GetSelected()) : 0;
    m_iRivers = m_sldRivers ? m_sldRivers->GetValue() : 60;
    m_iOcean = m_sldOcean ? m_sldOcean->GetValue() : 50;
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
    m_lstRaces->SetFontSize(16);   // the default 13pt race names were hard to read

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
    m_lblDesc->SetFontSize(15);   // larger, more readable race description

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
    // Bring the main window forward as we paint the menu background between the
    // setup dialogs. Otherwise, in the gap after the always-on-top loading window
    // is hidden and before the next dialog raises itself, the main window isn't
    // pinned and a foreign OS window can flash over the background for a frame.
    if (gameWindow && gameWindow->GetWindow())
        SDL_RaiseWindow(gameWindow->GetWindow());
}

// Paint a "please wait" status onto the wallpaper and present one frame, so the
// blocking vpStartup/OpenServer/OpenClient calls (gethostbyname can stall for
// seconds) show feedback instead of a bare wallpaper. Single-threaded: this is the
// last frame drawn before the blocking call returns and the next dialog opens.
static void ShowConnectingMessage(GameWindow* gameWindow, const char* msg) {
    if (theApp.m_sdlMainMenu && theApp.m_sdlMainMenu->IsInitialized())
        theApp.m_sdlMainMenu->RenderWallpaperWithMessage(msg);
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
    theGame.m_iWorldType = pCreate->m_iWorldType = createDlg.m_iWorldType;
    theGame.m_iRivers = pCreate->m_iRivers = createDlg.m_iRivers;
    theGame.m_iOcean = pCreate->m_iOcean = createDlg.m_iOcean;
    pCreate->m_iNumAi = createDlg.m_iNumAi;
    pCreate->m_iNet = -1;

    EnWriteProfileInt("Create", "Difficultity", createDlg.m_iAiLevel);
    EnWriteProfileInt("Create", "Size", createDlg.m_iWorldSize);
    EnWriteProfileInt("Create", "AiOpponents", createDlg.m_iNumAi);
    EnWriteProfileInt("Create", "StartPosition", createDlg.m_iStartPos);
    EnWriteProfileInt("Create", "WorldType", createDlg.m_iWorldType);
    EnWriteProfileInt("Create", "Rivers", createDlg.m_iRivers);
    EnWriteProfileInt("Create", "Ocean", createDlg.m_iOcean);

    // Step 4: Set player race (same as CDlgPickRace::OnOK)
    CRaceDef* pRace = &ptheRaces[raceDlg.m_iSelectedRace];
    pCreate->m_sName = raceDlg.m_playerName.c_str();
    pCreate->m_sRace = pRace->GetLine();

    theGame.GetMe()->SetName(raceDlg.m_playerName.c_str());
    theGame.GetMe()->m_InitData.Set(pRace, pCreate->m_iPos);
    pCreate->GetNew()->m_InitData.Set(pRace, pCreate->m_iPos);

    // Step 5: Start game creation via the real game logic path
    // ReadyToCreate() creates AI players and calls StartCreateWorld().
    // Fill the wallpaper-only gap between the race pick closing and the "Creating
    // world..." progress dialog appearing with a status line — same style as the
    // online "Searching for games..." message.
    ShowConnectingMessage(gameWindow, "Starting game...");
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

    // Fill the wallpaper-only gap before the "Creating world..." progress dialog
    // (same style as the online "Searching for games..." status line).
    ShowConnectingMessage(gameWindow, "Starting game...");
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
    : SDL2Dialog(gameWindow, "Pick Your Player", 580, 500) {}

SDL2PickPlayerDialog::~SDL2PickPlayerDialog() {
    for (auto* surf : m_racePictures)
        if (surf) SDL_FreeSurface(surf);
}

// Saved players don't store a race index, but m_InitData keeps the race's
// permanent multipliers (m_fRace[], untouched during play). Match those against
// the loaded race table to recover which race this player chose, so we can show
// the matching portrait — same picture the Pick Race dialog uses.
static int FindPlayerRaceIndex(CPlayer* pPlr) {
    if (!pPlr) return -1;
    int numRaces = CRaceDef::GetNumRaces();
    for (int r = 0; r < numRaces; r++) {
        bool match = true;
        for (int i = 0; i < CRaceDef::num_race; i++) {
            float a = pPlr->m_InitData.GetRace(i);
            float b = ptheRaces[r].GetRace(i);
            float d = a - b; if (d < 0) d = -d;
            if (d > 0.0001f) { match = false; break; }
        }
        if (match) return r;
    }
    return -1;
}

void SDL2PickPlayerDialog::OnInit() {
    int lx = m_x + 20, y = m_y + 45, w = m_width - 40;

    // Name is the saved player's own name — read-only here (you pick which player
    // slot to take, you don't rename it), so show it as a label, not an edit box.
    AddWidget<SDL2Label>(lx, y, 60, 24, "Name:");
    m_lblName = AddWidget<SDL2Label>(lx + 65, y, w - 65, 24, "", SDL_Color{48, 58, 148, 255});
    y += 34;

    int listW = 180, bottomY = m_y + m_height - 55, listH = bottomY - y;
    m_lstPlayers = AddWidget<SDL2Listbox>(lx, y, listW, listH,
        [this](int idx) { OnPlayerSelected(idx); },
        [this](int idx) { OnPlayerSelected(idx); if (m_btnOK && m_btnOK->IsEnabled()) OnOK(); });

    // Pre-convert every race portrait once, indexed by race (same pictures the
    // Pick Race dialog shows). Players are matched to a race below.
    int numRaces = CRaceDef::GetNumRaces();
    m_racePictures.resize(numRaces, nullptr);
    for (int r = 0; r < numRaces; r++) {
        CDIB* pPic = ptheRaces[r].GetPicture();
        if (pPic)
            m_racePictures[r] = SDL2MainMenu::CreateSurfaceFromDIB(pPic);
    }

    POSITION pos;
    for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL;) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        CNetPlyrJoin* pData = CNetPlyrJoin::Alloc(pPlr);
        PlayerInfo pi;
        pi.plyrNum = pData->m_iPlyrNum;
        pi.available = pData->m_bAvail;
        pi.numBldgs = pData->m_iNumBldgs;
        pi.numVeh = pData->m_iNumVeh;
        pi.raceIdx = FindPlayerRaceIndex(pPlr);
        pi.name = pPlr->GetName();
        // Resources the player holds (only non-zero), matching the original
        // CDlgPickPlayer::OnSelchangeRaceList material loop.
        for (int iMat = 0; iMat < CMaterialTypes::GetNumTypes(); iMat++)
            if (pData->m_iMat[iMat] > 0)
                pi.resources.push_back({ CMaterialTypes::GetDesc(iMat), pData->m_iMat[iMat] });
        m_players.push_back(pi);
        m_lstPlayers->AddItem(pi.name);
        delete[] (char*)pData;
    }

    // Right side: race portrait on top, stats/description below — mirrors the
    // Pick Race dialog so loading a game shows the same race art.
    int rightX = lx + listW + 15, rightW = w - listW - 15;
    int picH = 170;
    m_imgPicture = AddWidget<SDL2Image>(rightX, y, rightW, picH);

    int descY = y + picH + 8;
    int descH = bottomY - descY;
    // Dark blue (the default label color, also used by SDL2PickRaceDialog) — the
    // old light gray {180,190,200} was nearly illegible on the gold background.
    m_lblDesc = AddWidget<SDL2Label>(rightX, descY, rightW, descH, "", SDL_Color{48, 58, 148, 255});
    m_lblDesc->SetWrapped(true);
    // Anchor the stats to the top of the panel (aligned with the portrait) instead
    // of letting Render vertically center them in the tall rect.
    m_lblDesc->SetTopAligned(true);

    m_btnOK = AddWidget<SDL2Button>(m_x + m_width / 2 - 100, m_y + m_height - 45, 90, 30, "OK",
                                     [this]() { OnOK(); });
    m_btnOK->SetEnabled(false);
    AddWidget<SDL2Button>(m_x + m_width / 2 + 10, m_y + m_height - 45, 90, 30, "Cancel",
                          [this]() { OnCancel(); });

    for (int i = 0; i < (int)m_players.size(); i++) {
        CPlayer* pPlr = theGame.GetPlayerByPlyr(m_players[i].plyrNum);
        if (pPlr && pPlr == theGame._GetMe()) {
            m_lstPlayers->SetSelected(i);
            OnPlayerSelected(i);
            break;
        }
    }
}

void SDL2PickPlayerDialog::OnPlayerSelected(int index) {
    if (index < 0 || index >= (int)m_players.size()) return;
    auto& pi = m_players[index];
    // Mirror CDlgPickPlayer::OnSelchangeRaceList: optional "taken" line, then
    // buildings, vehicles, and every resource the player holds.
    std::string desc;
    if (!pi.available)
        desc += "(player taken)\n";
    desc += "Buildings: " + std::to_string(pi.numBldgs) + "\n";
    desc += "Vehicles: " + std::to_string(pi.numVeh) + "\n";
    for (const auto& r : pi.resources)
        desc += r.name + ": " + std::to_string(r.amount) + "\n";
    m_lblDesc->SetText(desc);

    // Reflect the selected player's saved name (read-only).
    if (m_lblName) m_lblName->SetText(pi.name);

    // Show the selected player's race portrait.
    if (m_imgPicture) {
        if (pi.raceIdx >= 0 && pi.raceIdx < (int)m_racePictures.size() && m_racePictures[pi.raceIdx])
            m_imgPicture->SetSurface(m_racePictures[pi.raceIdx], false);  // not owned by widget
        else
            m_imgPicture->Clear();
    }
    UpdateOKButton();
}

void SDL2PickPlayerDialog::UpdateOKButton() {
    int sel = m_lstPlayers->GetSelected();
    // Original enabled OK only for an *available* player (taken slots can't be
    // picked when loading a saved game). The name comes from the save, so there's
    // nothing for the user to fill in.
    bool valid = sel >= 0 && sel < (int)m_players.size() && m_players[sel].available;
    if (m_btnOK) m_btnOK->SetEnabled(valid);
}

void SDL2PickPlayerDialog::OnOK() {
    int sel = m_lstPlayers->GetSelected();
    if (sel < 0 || sel >= (int)m_players.size()) return;
    m_iSelectedPlyrNum = m_players[sel].plyrNum;
    m_playerName = m_players[sel].name;
    EnWriteProfileString("Create", "Name", m_playerName.c_str());
    EndDialog(1);
}

// ============================================================================
// SDL2CreateNetDialog
// ============================================================================
SDL2CreateNetDialog::SDL2CreateNetDialog(GameWindow* gameWindow)
    : SDL2Dialog(gameWindow, "Create Network Game (TCP/IP)", 520, 590) {}

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

    // World type — full-width preset list below the two columns
    int wtY = y + rowH * 9 + 16;
    AddWidget<SDL2Label>(lx, wtY, colW, rowH, "World Type:");
    m_lstWorldType = AddWidget<SDL2Listbox>(lx, wtY + rowH, m_width - 40, rowH * 5);
    PopulateWorldTypeList(m_lstWorldType);

    // Rivers — density slider (0 = none, 60 = classic baseline, 100 = lots).
    // Host's value rides CNetStart so every client generates the identical map.
    int rvY = wtY + rowH * 6 + 10;
    int savedRivers = std::max(0, std::min(100, (int)EnGetProfileInt("Create", "Rivers", 60)));
    m_lblRivers = AddWidget<SDL2Label>(lx, rvY, 110, rowH, "Rivers: " + std::to_string(savedRivers) + "%");
    m_sldRivers = AddWidget<SDL2Slider>(lx + 115, rvY, m_width - 40 - 115, rowH, 0, 100, savedRivers,
        [this](int v) { if (m_lblRivers) m_lblRivers->SetText("Rivers: " + std::to_string(v) + "%"); });

    AddOKCancelButtons();
}

void SDL2CreateNetDialog::OnOK() {
    m_gameName = m_edtGameName->GetText();
    m_playerName = m_edtPlayerName->GetText();
    if (m_gameName.empty() || m_playerName.empty()) return;
    // Persist the name so the race picker (which pre-fills from this profile) shows
    // the name just entered here instead of reverting to the previously saved one.
    EnWriteProfileString("Create", "Name", m_playerName.c_str());
    m_iAiLevel = m_radAiLevel->GetSelected();
    m_iWorldSize = m_radWorldSize->GetSelected();
    m_iStartPos = m_radStartPos->GetSelected();
    m_iWorldType = m_lstWorldType ? std::max(0, m_lstWorldType->GetSelected()) : 0;
    m_iRivers = m_sldRivers ? m_sldRivers->GetValue() : 60;
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

    // Hide the loading-progress window before the pick-player dialog. The
    // progress window is topmost (always-on-top) and would otherwise render over
    // the dialog. StartGame() below re-shows it for the AI-startup progress.
    if (gameWindow->GetCreateStatus())
        gameWindow->GetCreateStatus()->Hide();

    // Headless harness load (HarnessLoadGame): skip the modal pick-player dialog and
    // auto-select the human — theGame._GetMe(), which the dialog itself defaults its
    // selection to. nullptr for any normal menu load, so the dialog runs as before.
    CPlayer* pPlr = NULL;
    std::string sPickName;
    if (HarnessPendingLoadPath()) {
        pPlr = theGame._GetMe();
        if (!pPlr) { theGame.Close(); delete theApp.m_pCreateGame; theApp.m_pCreateGame = NULL; return false; }
        sPickName = pPlr->GetName() ? pPlr->GetName() : "";   // keep the loaded name
    } else {
        ShowWallpaperBackground(gameWindow);
        SDL2PickPlayerDialog pickDlg(gameWindow);
        if (pickDlg.DoModal() != 1) {
            theGame.Close();
            delete theApp.m_pCreateGame;
            theApp.m_pCreateGame = NULL;
            return false;
        }
        pPlr = theGame.GetPlayerByPlyr(pickDlg.m_iSelectedPlyrNum);
        if (!pPlr) { theGame.Close(); delete theApp.m_pCreateGame; theApp.m_pCreateGame = NULL; return false; }
        sPickName = pickDlg.m_playerName;
    }

    theGame.SetHP(TRUE); theGame.SetScenario(-1);
    if (pPlr != theGame._GetMe()) {
        theGame._SetMe(pPlr);
        if (theGame.AmServer()) theGame._SetServer(pPlr);
    }
    pPlr->SetState(CPlayer::ready);
    pPlr->SetName(sPickName.c_str());

    POSITION pos;
    for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL;) {
        CPlayer* pP = theGame.GetAll().GetNext(pos);
        if (pP->IsMe()) { pP->SetRelations(RELATIONS_ALLIANCE); pP->SetTheirRelations(RELATIONS_ALLIANCE); }
        else { pP->SetRelations(RELATIONS_NEUTRAL); pP->SetTheirRelations(RELATIONS_NEUTRAL); }
    }

    // Raise main window so progress dialog is visible
    if (gameWindow->GetWindow())
        SDL_RaiseWindow(gameWindow->GetWindow());

    // Fill the wallpaper-only gap after the player pick (before the AI-startup
    // progress dialog re-shows) with a status line — same style as the online
    // "Searching for games..." message.
    ShowConnectingMessage(gameWindow, "Loading game...");

    try {
        theGame.IncTry();
        pCreate->ClosePick();
        // Re-show the loading dialog for StartGame's AI-startup phase. It was
        // Hidden before the pick-player dialog (it's topmost and would cover it),
        // and StartGame only calls SetPer — never ShowDlgStatus — so without this
        // the player stares at a blank background while the AI initializes. Mirror
        // the network load path (netapi.cpp CmdSelectOk): reset the bar to 0
        // (bypasses the don't-go-backward guard after the ~86% load reached),
        // set a message, then show.
        if (SDL2CreateStatus* pDlg = pCreate->GetDlgStatus()) {
            pDlg->SetPer(0);
            pDlg->SetMsg(IDS_START_AI1);
            pCreate->ShowDlgStatus();
        }
        if (theGame.StartGame(FALSE) != IDOK) { theGame.DecTry(); throw(0); }
        theGame.DecTry();
    } catch (int iNum) { CatchNum(iNum); theApp.CloseWorld(); return false; }
    catch (...) { CatchOther(); theApp.CloseWorld(); return false; }

    return true;
}

// ============================================================================
// SDL2HostLoadedDialog
// ============================================================================
SDL2HostLoadedDialog::SDL2HostLoadedDialog(GameWindow* gw, const std::string& gameName)
    : SDL2Dialog(gw, "Host Saved Game (TCP/IP)", 400, 200)
    , m_gameName(gameName) {}

void SDL2HostLoadedDialog::OnInit() {
    int lx = m_x + 16, y = m_y + 36, w = m_width - 32, rowH = 28;

    AddWidget<SDL2Label>(lx, y, w, rowH, ("Game: " + m_gameName).c_str());
    y += rowH + 4;

    AddWidget<SDL2Label>(lx, y, 110, rowH, "Your Name:");
    std::string savedName = EnGetProfileStdString("Create", "Name", "");
    m_edtPlayerName = AddWidget<SDL2EditBox>(lx + 115, y, w - 115, 24, savedName);
    y += rowH + 4;

    AddWidget<SDL2Label>(lx, y, 110, rowH, "Port:");
    m_edtPort = AddWidget<SDL2EditBox>(lx + 115, y, 80, 24, "2346");

    AddOKCancelButtons();
}

void SDL2HostLoadedDialog::OnOK() {
    m_playerName = m_edtPlayerName ? m_edtPlayerName->GetText() : "";
    if (m_playerName.empty()) return;
    m_iPort = m_edtPort ? atoi(m_edtPort->GetText().c_str()) : 2346;
    if (m_iPort <= 0) m_iPort = 2346;
    EndDialog(1);
}

// Best-effort: identify a player's race by matching their InitData racial
// attributes against the loaded race definitions. CInitData stores only the
// numeric attributes (not a race index/name), but each race's attribute vector
// is distinct, so this round-trips a name for both the local host and remote
// joined players (whose InitData arrives over the wire via CNetReady).
static const char* RaceNameForPlayer(CPlayer* pPlr) {
    if (!pPlr) return "";
    for (int r = 0; r < CRaceDef::GetNumRaces(); ++r) {
        const CRaceDef& rd = ptheRaces[r];
        bool match = true;
        for (int i = 0; i < CRaceDef::num_race; ++i) {
            float d = rd.GetRace(i) - pPlr->m_InitData.GetRace(i);
            if (d < 0) d = -d;
            if (d > 0.0001f) { match = false; break; }
        }
        if (match) return rd.GetLine();
    }
    return "";   // not yet chosen (InitData still zeroed)
}

// ============================================================================
// SDL2LobbyDialog
// ============================================================================
SDL2LobbyDialog::SDL2LobbyDialog(GameWindow* gw, const std::string& gameName)
    : SDL2Dialog(gw, "Waiting for Players", 470, 484)
    , m_gameName(gameName) {}

void SDL2LobbyDialog::OnInit() {
    int lx = m_x + 12, y = m_y + 36, w = m_width - 24, rowH = 22;

    AddWidget<SDL2Label>(lx, y, w, rowH, ("Game: " + m_gameName).c_str());
    y += rowH + 2;

    // TCP/IP config: the address clients connect to + the well-known port from
    // vdmplay.ini. theNet.GetAddress() yields "ip:streamPort,dgPort"; the port a
    // client actually types is the well-known port, so show it on its own line
    // (the address string is long enough to clip a trailing "Port:" off the row).
    std::string addr = theNet.GetAddress();
    int port = GetPrivateProfileInt("TCP", "WellKnownPort", 2346, ".\\vdmplay.ini");
    AddWidget<SDL2Label>(lx, y, w, rowH,
        ("Server address: " + (addr.empty() ? std::string("(local)") : addr)).c_str());
    y += rowH + 2;
    AddWidget<SDL2Label>(lx, y, w, rowH, ("Port: " + std::to_string(port)).c_str());
    y += rowH + 6;

    AddWidget<SDL2Label>(lx, y, w, rowH, "Players (name - race):");
    y += rowH + 2;

    m_lstPlayers = AddWidget<SDL2Listbox>(lx, y, w, 104);
    y += 112;

    m_lblStatus = AddWidget<SDL2Label>(lx, y, w, rowH, "Server ready. Click Start when all players have joined.");
    m_lblStatus->SetWrapped(true);
    y += rowH + 10;

    int btnW = 110, gap = 10;
    AddWidget<SDL2Button>(lx, y, btnW, rowH, "Start Game",
        [this]() { OnStart(); });
    AddWidget<SDL2Button>(lx + btnW + gap, y, btnW, rowH, "Change Race",
        [this]() { EndDialog(2); });   // create flow loops back to the race picker
    AddWidget<SDL2Button>(lx + (btnW + gap) * 2, y, btnW, rowH, "Cancel",
        [this]() { EndDialog(0); });
    y += rowH + 12;

    // --- Lobby chat ---------------------------------------------------------
    AddWidget<SDL2Label>(lx, y, w, rowH, "Chat:");
    y += rowH + 2;
    m_lstChat = AddWidget<SDL2Listbox>(lx, y, w, 92);
    y += 100;
    int sendW = 70;
    m_edtChat = AddWidget<SDL2EditBox>(lx, y, w - sendW - 6, rowH, "");
    AddWidget<SDL2Button>(lx + w - sendW, y, sendW, rowH, "Send",
        [this]() { SendChat(); });
}

void SDL2LobbyDialog::OnFrame() {
    UpdatePlayerList();
    RefreshChat();
}

void SDL2LobbyDialog::RefreshChat() {
    if (!m_lstChat) return;
    int n = SDL2Chat_Count();
    if (n == m_chatCount) return;
    m_chatCount = n;
    m_lstChat->Clear();
    for (int i = 0; i < n; i++)
        m_lstChat->AddItem(SDL2Chat_Line(i));
    if (n > 0) m_lstChat->SetSelected(n - 1);   // scroll to newest
}

void SDL2LobbyDialog::SendChat() {
    if (!m_edtChat) return;
    std::string text = m_edtChat->GetText();
    if (text.empty()) return;
    SDL2Chat_Send(text);
    m_edtChat->SetText("");
    RefreshChat();
}

void SDL2LobbyDialog::OnOK() {
    // Enter's default action is EndDialog(1), which would start the game the
    // instant the user hits Enter to send a chat message (skipping OnStart's
    // opponent-count check). Route to Send when the chat box has focus.
    if (m_edtChat && m_edtChat->HasFocus()) { SendChat(); return; }
    OnStart();
}

void SDL2LobbyDialog::OnStart() {
    // Don't allow starting an online game with no opponents: 0 AI players AND no
    // other human has joined (only the host). The game needs someone to fight.
    int otherHumans = 0;
    POSITION pos = theGame.GetAll().GetHeadPosition();
    while (pos != NULL) {
        CPlayer* p = theGame.GetAll().GetNext(pos);
        if (p && !p->IsMe() && !p->IsAI() && p->GetNetNum() != 0) otherHumans++;
    }
    int aiCount = theApp.m_pCreateGame ? theApp.m_pCreateGame->m_iNumAi : 0;
    if (otherHumans == 0 && aiCount == 0) {
        if (m_lblStatus)
            m_lblStatus->SetText("Can't start with no opponents - wait for a player to join, "
                                 "or Cancel and set AI opponents.");
        return;
    }
    EndDialog(1);
}

void SDL2LobbyDialog::UpdatePlayerList() {
    if (!m_lstPlayers) return;

    // Rebuild on any change to the name/race set, not just the count — a player's
    // race name appears only after they pick, which doesn't change the count.
    std::string sig;
    int human = 0;
    POSITION pos = theGame.GetAll().GetHeadPosition();
    while (pos != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        if (!pPlr) continue;
        const char* race = RaceNameForPlayer(pPlr);
        sig += pPlr->GetName(); sig += '|'; sig += race; sig += '\n';
        if (!pPlr->IsAI() && pPlr->GetNetNum() != 0) human++;
    }
    if (sig == m_lastSig) return;
    m_lastSig = sig;

    m_lstPlayers->Clear();
    pos = theGame.GetAll().GetHeadPosition();
    while (pos != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        if (!pPlr) continue;
        std::string line = pPlr->GetName();
        if (pPlr->IsMe())           line += " (you)";
        else if (pPlr->IsAI())      line += " (AI)";
        else if (pPlr->GetNetNum()) line += " (joined)";
        const char* race = RaceNameForPlayer(pPlr);
        if (race && race[0]) { line += " - "; line += race; }
        m_lstPlayers->AddItem(line);
    }

    if (m_lblStatus) {
        std::string status = human <= 1
            ? "Waiting for clients to join... Click Start when ready."
            : std::to_string(human) + " player(s) connected. Click Start when ready.";
        m_lblStatus->SetText(status);
    }
}

// ============================================================================
// SDL2ClientLobbyDialog — the joining player's waiting room
// ============================================================================
SDL2ClientLobbyDialog::SDL2ClientLobbyDialog(GameWindow* gw, const std::string& gameName)
    : SDL2Dialog(gw, "Network Game - Waiting Room", 440, 458)
    , m_gameName(gameName) {}

void SDL2ClientLobbyDialog::OnInit() {
    int lx = m_x + 12, y = m_y + 36, w = m_width - 24, rowH = 22;

    AddWidget<SDL2Label>(lx, y, w, rowH, ("Game: " + m_gameName).c_str());
    y += rowH + 4;

    AddWidget<SDL2Label>(lx, y, w, rowH, "Players (name - race):");
    y += rowH + 2;

    m_lstPlayers = AddWidget<SDL2Listbox>(lx, y, w, 110);
    y += 118;

    m_lblStatus = AddWidget<SDL2Label>(lx, y, w, rowH * 2,
        "Connected. Waiting for the host to start the game...");
    m_lblStatus->SetWrapped(true);
    y += rowH * 2 + 6;

    AddWidget<SDL2Button>(lx, y, 110, rowH, "Leave", [this]() { EndDialog(0); });
    y += rowH + 12;

    // --- Lobby chat ---------------------------------------------------------
    AddWidget<SDL2Label>(lx, y, w, rowH, "Chat:");
    y += rowH + 2;
    m_lstChat = AddWidget<SDL2Listbox>(lx, y, w, 92);
    y += 100;
    int sendW = 70;
    m_edtChat = AddWidget<SDL2EditBox>(lx, y, w - sendW - 6, rowH, "");
    AddWidget<SDL2Button>(lx + w - sendW, y, sendW, rowH, "Send",
        [this]() { SendChat(); });
}

void SDL2ClientLobbyDialog::OnOK() {
    // Enter's default action is EndDialog(1), which this dialog's caller reads as
    // "the host started the game" (see class comment) — hitting Enter to send a
    // chat message must not trigger that. There's no other Enter-triggered action
    // here (only the explicit Leave button), so swallow it when chat isn't focused.
    if (m_edtChat && m_edtChat->HasFocus()) SendChat();
}

void SDL2ClientLobbyDialog::RefreshChat() {
    if (!m_lstChat) return;
    int n = SDL2Chat_Count();
    if (n == m_chatCount) return;
    m_chatCount = n;
    m_lstChat->Clear();
    for (int i = 0; i < n; i++)
        m_lstChat->AddItem(SDL2Chat_Line(i));
    if (n > 0) m_lstChat->SetSelected(n - 1);
}

void SDL2ClientLobbyDialog::SendChat() {
    if (!m_edtChat) return;
    std::string text = m_edtChat->GetText();
    if (text.empty()) return;
    SDL2Chat_Send(text);
    m_edtChat->SetText("");
    RefreshChat();
}

void SDL2ClientLobbyDialog::OnFrame() {
    // Drain queued network setup messages here. Incoming game data (other players
    // joining, and the host's CNetStart) is QUEUED by AddToQueue and normally
    // drained by the main game loop — but ReadyToJoin paused that and we're now
    // sitting in this modal loop, so nothing would drain it. Do it ourselves.
    // The host's CNetStart is deferred (CmdStart sees g_bClientLobbyWaiting), so
    // this only flips the start flag rather than building the world nested here.
    BOOL wasProc = theGame.ShouldProcessMessages();
    theGame.SetShouldProcessMessages(TRUE);
    theApp.ProcessAllMessages();
    theGame.SetShouldProcessMessages(wasProc);

    UpdatePlayerList();
    RefreshChat();

    extern bool g_bClientStartReceived;
    if (g_bClientStartReceived)
        EndDialog(1);   // host started -> flow builds the world outside this loop
}

void SDL2ClientLobbyDialog::UpdatePlayerList() {
    if (!m_lstPlayers) return;

    std::string sig;
    POSITION pos = theGame.GetAll().GetHeadPosition();
    while (pos != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        if (!pPlr) continue;
        const char* race = RaceNameForPlayer(pPlr);
        sig += pPlr->GetName(); sig += '|'; sig += race; sig += '\n';
    }
    if (sig == m_lastSig) return;
    m_lastSig = sig;

    m_lstPlayers->Clear();
    pos = theGame.GetAll().GetHeadPosition();
    while (pos != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        if (!pPlr) continue;
        std::string line = pPlr->GetName();
        if (pPlr->IsMe())           line += " (you)";
        else if (pPlr->IsAI())      line += " (AI)";
        else if (pPlr->GetNetNum()) line += " (host/player)";
        const char* race = RaceNameForPlayer(pPlr);
        if (race && race[0]) { line += " - "; line += race; }
        m_lstPlayers->AddItem(line);
    }
}

// ============================================================================
// SDL2SessionBrowseDialog
// ============================================================================
SDL2SessionBrowseDialog::SDL2SessionBrowseDialog(GameWindow* gw, CJoinMulti* pJoin)
    : SDL2Dialog(gw, "Join Network Game - Available Games", 500, 360)
    , m_pJoin(pJoin) {}

void SDL2SessionBrowseDialog::OnInit() {
    int lx = m_x + 12, y = m_y + 36, w = m_width - 24, rowH = 22;

    // Editable address + port, pre-filled from the ini the join flow wrote. The
    // user can change these and hit Search to re-target without backing out.
    char srvAddr[128] = {0};
    GetPrivateProfileString("TCP", "ServerAddress", "localhost",
                            srvAddr, sizeof(srvAddr), ".\\vdmplay.ini");
    int port = GetPrivateProfileInt("TCP", "WellKnownPort", 2346, ".\\vdmplay.ini");

    AddWidget<SDL2Label>(lx, y, 70, rowH, "Address:");
    m_edtAddr = AddWidget<SDL2EditBox>(lx + 74, y, 180, rowH, srvAddr);
    AddWidget<SDL2Label>(lx + 266, y, 40, rowH, "Port:");
    m_edtPort = AddWidget<SDL2EditBox>(lx + 308, y, 70, rowH, std::to_string(port));
    AddWidget<SDL2Button>(lx + 386, y, w - 386, rowH, "Search",
        [this]() { OnSearch(); });
    y += rowH + 6;

    AddWidget<SDL2Label>(lx, y, w, rowH, "Available games (searching...):");
    y += rowH + 2;

    m_lstSessions = AddWidget<SDL2Listbox>(lx, y, w, 126,
        [this](int idx) { SelectIndex(idx); });
    y += 132;

    m_lblInfo = AddWidget<SDL2Label>(lx, y, w, 36, "");
    m_lblInfo->SetWrapped(true);
    y += 42;

    int btnW = 110, gap = 12;
    m_btnJoin = AddWidget<SDL2Button>(lx, y, btnW, rowH, "Join",
        [this]() { OnJoin(); });
    m_btnJoin->SetEnabled(false);

    AddWidget<SDL2Button>(lx + btnW + gap, y, btnW, rowH, "Cancel",
        [this]() { EndDialog(0); });
}

// Capture the edited address/port and close with result 3. The join flow does
// the actual Close + OpenClient OUTSIDE this modal loop: OpenClient blocks and
// pumps Win32 messages internally, so calling it from inside a dialog event
// callback re-enters the loop and hangs. Re-opening between dialogs (as the
// initial OpenClient does) is safe.
void SDL2SessionBrowseDialog::OnSearch() {
    m_searchAddr = m_edtAddr ? m_edtAddr->GetText() : "localhost";
    if (m_searchAddr.empty()) m_searchAddr = "localhost";
    m_searchPort = m_edtPort ? atoi(m_edtPort->GetText().c_str()) : 2346;
    if (m_searchPort <= 0) m_searchPort = 2346;
    EndDialog(3);   // join flow re-targets, then re-opens this dialog
}

void SDL2SessionBrowseDialog::OnFrame() {
    UpdateList();
}

void SDL2SessionBrowseDialog::SelectIndex(int idx) {
    m_selectedIdx = idx;
    bool valid = idx >= 0 && idx < (int)m_pJoin->m_sessions.size();
    if (m_btnJoin) m_btnJoin->SetEnabled(valid);
    if (!m_lblInfo) return;

    if (!valid) { m_lblInfo->SetText(""); return; }

    const auto& s = m_pJoin->m_sessions[idx];
    static const char* aiNames[] = { "Easy", "Moderate", "Difficult", "Impossible" };
    static const char* szNames[]  = { "Small", "Medium", "Large" };
    std::string info = s.gameName;
    if (s.aiLevel  >= 0 && s.aiLevel  < 4) info += std::string("  AI: ") + aiNames[s.aiLevel];
    if (s.worldSize >= 0 && s.worldSize < 3) info += std::string("  Size: ") + szNames[s.worldSize];
    if (s.numOpponents > 0) info += "  Opp: " + std::to_string(s.numOpponents);
    m_lblInfo->SetText(info);
}

void SDL2SessionBrowseDialog::UpdateList() {
    if (!m_lstSessions) return;
    int count = (int)m_pJoin->m_sessions.size();
    if (count == m_lastCount) return;
    m_lastCount = count;

    // Preserve selection by name if possible
    std::string selName;
    if (m_selectedIdx >= 0 && m_selectedIdx < (int)m_pJoin->m_sessions.size())
        selName = m_pJoin->m_sessions[m_selectedIdx].gameName;

    m_lstSessions->Clear();
    int newSel = -1;
    for (int i = 0; i < count; i++) {
        m_lstSessions->AddItem(m_pJoin->m_sessions[i].gameName);
        if (!selName.empty() && m_pJoin->m_sessions[i].gameName == selName)
            newSel = i;
    }

    m_selectedIdx = newSel;
    if (newSel >= 0) m_lstSessions->SetSelected(newSel);
    if (m_btnJoin) m_btnJoin->SetEnabled(newSel >= 0);

    if (count == 0 && m_lblInfo)
        m_lblInfo->SetText("No games found - check the server address and port, then Refresh.");
}

void SDL2SessionBrowseDialog::OnJoin() {
    if (m_selectedIdx < 0 || m_selectedIdx >= (int)m_pJoin->m_sessions.size()) return;
    m_chosenIdx = m_selectedIdx;
    EndDialog(1);
}

void SDL2SessionBrowseDialog::OnRefresh() {
    theNet.StopEnum();
    m_pJoin->m_sessions.clear();
    m_lastCount = -1;
    m_selectedIdx = -1;
    if (m_btnJoin) m_btnJoin->SetEnabled(false);
    if (m_lblInfo) m_lblInfo->SetText("Searching...");
    theNet.StartEnum();
}

// P5 — optional ISERVE registration server (cross-platform headless discovery).
// If vdmplay.ini [vdmplay]RegistrationServerAddr is set, a HOST writes it to
// [TCP]RegistrationAddress so the engine registers its session there (InitTcp ->
// SetRegistrationAddress -> the host SendTo(m_registrationAddress) path), letting
// clients on other subnets find it via the iserve daemon instead of LAN broadcast.
// UNSET => RegistrationAddress stays empty => no registration => unchanged behavior
// (LAN broadcast + direct Join). Registration port (1707) stays distinct from the
// game-session port (2346). Clients point at the same server via the Join dialog's
// Server Address field (already written to [TCP]ServerAddress).
static void ApplyOptionalRegistrationServer() {
    char regSrv[80] = {0};
    GetPrivateProfileString("vdmplay", "RegistrationServerAddr", "",
                            regSrv, sizeof(regSrv), ".\\vdmplay.ini");
    if (regSrv[0])
        WritePrivateProfileString("TCP", "RegistrationAddress", regSrv, ".\\vdmplay.ini");
}

// ============================================================================
// Create Network Game flow (TCP/IP only)
// ============================================================================
bool SDL2_RunCreateNetworkFlow(GameWindow* gameWindow) {
    // Step 1: collect game settings + player name from the server
    ShowWallpaperBackground(gameWindow);
    SDL2CreateNetDialog createDlg(gameWindow);
    if (createDlg.DoModal() != 1) return false;

    ASSERT(theApp.m_pCreateGame == NULL);
    CCreateMulti* pCreate = new CCreateMulti();
    theApp.m_pCreateGame = pCreate;

    theGame.ctor(); theGame.SetServer(TRUE); theGame._SetIsNetGame(TRUE); theGame.Open(TRUE);

    theGame.m_iAi  = pCreate->m_iAi  = createDlg.m_iAiLevel;
    theGame.m_iSize = pCreate->m_iSize = createDlg.m_iWorldSize;
    theGame.m_iPos  = pCreate->m_iPos  = createDlg.m_iStartPos;
    theGame.m_iWorldType = pCreate->m_iWorldType = createDlg.m_iWorldType;
    theGame.m_iRivers    = pCreate->m_iRivers    = createDlg.m_iRivers;
    pCreate->m_iNumAi    = createDlg.m_iNumAi;
    pCreate->m_iNet      = 0;
    pCreate->m_sName     = createDlg.m_playerName;
    pCreate->m_sGameName = createDlg.m_gameName;

    EnWriteProfileInt("Create", "Difficultity", createDlg.m_iAiLevel);
    EnWriteProfileInt("Create", "Size",          createDlg.m_iWorldSize);
    EnWriteProfileInt("Create", "AiOpponents",   createDlg.m_iNumAi);
    EnWriteProfileInt("Create", "StartPosition", createDlg.m_iStartPos);
    EnWriteProfileInt("Create", "WorldType",     createDlg.m_iWorldType);
    EnWriteProfileInt("Create", "Rivers",        createDlg.m_iRivers);

    std::string sPort = std::to_string(createDlg.m_iPort);
    WritePrivateProfileString("TCP", "WellKnownPort", sPort.c_str(), ".\\vdmplay.ini");
    ApplyOptionalRegistrationServer();   // P5: register with iserve if configured

    // Step 2: publish the session - pass CNetPublish so clients can read game
    // metadata (AI level, world size, etc.) from VP_SESSIONENUM callbacks.
    // OpenServer blocks (vpStartup -> gethostbyname + Listen); show a status frame.
    ShowConnectingMessage(gameWindow, "Starting network game...");
    CNetPublish* pPub = CNetPublish::Alloc(pCreate);
    BOOL bErr = theNet.OpenServer(VPT_TCP, theApp.m_wndMain.m_hWnd,
                                  (LPCSTR)pPub, NULL, NULL);
    delete[] pPub;
    if (bErr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Network Error",
            "Failed to open TCP/IP server.", nullptr);
        theGame.Close(); delete theApp.m_pCreateGame; theApp.m_pCreateGame = NULL;
        return false;
    }

    // Register the host as the SERVER player in the VDMPLAY session. Without this
    // the host has no vdmplay player, so joining clients never enumerate it and
    // the client's player list shows only itself. (The client registers itself
    // via theNet.Join.) Needs a name on GetMe() — Alloc reads GetName().
    theGame.GetMe()->SetName(createDlg.m_playerName.c_str());
    {
        CNetJoin* pHostJn = CNetJoin::Alloc(theGame.GetMe(), TRUE);  // TRUE = server
        VPPLAYERID hostId = theNet.AddPlayer(pHostJn);
        delete[] pHostJn;
        theGame.GetMe()->SetNetNum(hostId);
    }

    // Steps 3-4: pick race, then sit in the lobby. The lobby's "Change Race"
    // button (DoModal returns 2) loops back here so the host can re-pick before
    // starting; "Start" returns 1, "Cancel"/close returns 0.
    for (;;) {
        // Step 3: server picks race
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
        EnWriteProfileString("Create", "Name", raceDlg.m_playerName.c_str());

        // Step 4: lobby — wait for clients to join, then server clicks Start
        ShowWallpaperBackground(gameWindow);
        SDL2LobbyDialog lobbyDlg(gameWindow, createDlg.m_gameName);
        int lobbyResult = lobbyDlg.DoModal();
        if (lobbyResult == 2) continue;          // Change Race -> re-pick
        if (lobbyResult != 1) {                  // Cancel / closed
            theNet.Close(FALSE); theGame.Close();
            delete theApp.m_pCreateGame; theApp.m_pCreateGame = NULL;
            return false;
        }
        break;                                   // Start
    }

    // Step 5: create world
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

    // Step 1: collect player name + server address
    SDL2JoinNetDialog joinDlg(gameWindow);
    if (joinDlg.DoModal() != 1) return false;

    // Step 2: write TCP config for VDMPLAY
    WritePrivateProfileString("TCP", "ServerAddress", joinDlg.m_serverAddr.c_str(), ".\\vdmplay.ini");
    std::string sPort = std::to_string(joinDlg.m_iPort);
    WritePrivateProfileString("TCP", "WellKnownPort", sPort.c_str(), ".\\vdmplay.ini");

    // Step 3: create orchestrator + initialise game state for a joining client
    ASSERT(theApp.m_pCreateGame == NULL);
    CJoinMulti* pJoin = new CJoinMulti();
    theApp.m_pCreateGame = pJoin;

    // Step 4: open the VDMPLAY client transport; this starts async session
    //         enumeration - VP_SESSIONENUM messages will fire into
    //         CJoinMulti::OnSessionEnum() as results arrive.
    //         OpenClient blocks (vpStartup -> gethostbyname); show a status frame.
    ShowConnectingMessage(gameWindow, "Searching for games...");
    if (theNet.OpenClient(VPT_TCP, theApp.m_wndMain.m_hWnd, NULL)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Network Error",
            "Failed to open TCP/IP client.", nullptr);
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    // Step 5: show session browser — OnFrame() polls m_pJoin->m_sessions each
    //         frame and updates the list as VP_SESSIONENUM callbacks fire.
    //         The browser's Search button returns 3: we re-target the transport
    //         here (between modal loops, so OpenClient doesn't re-enter and hang)
    //         and re-open the browser.
    int chosenIdx = -1;
    for (;;) {
        ShowWallpaperBackground(gameWindow);
        SDL2SessionBrowseDialog browseDlg(gameWindow, pJoin);
        int r = browseDlg.DoModal();
        if (r == 3) {
            // Re-target: write the edited address/port and re-open the client.
            WritePrivateProfileString("TCP", "ServerAddress", browseDlg.m_searchAddr.c_str(), ".\\vdmplay.ini");
            WritePrivateProfileString("TCP", "WellKnownPort",
                                      std::to_string(browseDlg.m_searchPort).c_str(), ".\\vdmplay.ini");
            theNet.Close(FALSE);
            pJoin->m_sessions.clear();
            ShowConnectingMessage(gameWindow,
                ("Searching " + browseDlg.m_searchAddr + ":" +
                 std::to_string(browseDlg.m_searchPort) + " ...").c_str());
            if (theNet.OpenClient(VPT_TCP, theApp.m_wndMain.m_hWnd, NULL)) {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Network Error",
                    "Failed to open TCP/IP client for that address/port.", nullptr);
                delete theApp.m_pCreateGame; theApp.m_pCreateGame = NULL;
                return false;
            }
            continue;   // re-open the browser against the new target
        }
        if (r != 1) {
            theNet.Close(FALSE);
            delete theApp.m_pCreateGame;
            theApp.m_pCreateGame = NULL;
            return false;
        }
        chosenIdx = browseDlg.m_chosenIdx;
        break;
    }

    // Step 6: join the selected session
    const CJoinMulti::SessionEntry& chosen = pJoin->m_sessions[chosenIdx];
    pJoin->m_ID   = chosen.id;
    pJoin->m_iAi  = chosen.aiLevel;
    pJoin->m_iSize = chosen.worldSize;
    pJoin->m_iPos  = chosen.startPos;

    theNet.StopEnum();
    theGame.ctor();
    theGame.SetServer(FALSE);
    theGame._SetIsNetGame(TRUE);
    theGame.Open(TRUE);
    theGame.m_iAi   = pJoin->m_iAi;
    theGame.m_iSize = pJoin->m_iSize;
    theGame.m_iPos  = pJoin->m_iPos;
    theGame.m_sGameName = chosen.gameName;

    theGame.GetMe()->SetName(joinDlg.m_playerName.c_str());
    CNetJoin* pJn = CNetJoin::Alloc(theGame.GetMe(), FALSE);
    // theGame.ctor()/Open(TRUE) above re-default theNet's mode to closed; OpenClient had
    // set it to client. Re-assert client right before Join so netapi.cpp:178's
    // ASSERT(m_iType==client) holds (the vp handle survives, so no re-open needed). Fixes
    // the iserve/TCP-enum-discovered Join hitting the assert. [linux2 datapoint 2026-06-26]
    theNet.SetClientType();
    BOOL bJoinErr = theNet.Join(&pJoin->m_ID, pJn);
    delete[] pJn;
    if (bJoinErr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Network Error",
            "Failed to join the selected game.", nullptr);
        theNet.Close(FALSE);
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    // Step 7: player picks their race
    ShowWallpaperBackground(gameWindow);
    SDL2PickRaceDialog raceDlg(gameWindow);
    if (raceDlg.DoModal() != 1) {
        theNet.Close(FALSE);
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    // Step 8: wire race data into the player and signal readiness to server.
    // CNetReady carries our InitData to the server; CmdReady on the server
    // marks us ready and fires StartCreateWorld once all players are ready.
    CRaceDef* pRace = &ptheRaces[raceDlg.m_iSelectedRace];
    pJoin->m_sName = raceDlg.m_playerName.c_str();
    pJoin->m_sRace = pRace->GetLine();
    theGame.GetMe()->SetName(raceDlg.m_playerName.c_str());
    theGame.GetMe()->m_InitData.Set(pRace, pJoin->m_iPos);
    pJoin->GetNew()->m_InitData.Set(pRace, pJoin->m_iPos);

    EnWriteProfileString("Create", "Name", raceDlg.m_playerName.c_str());

    // Send race selection to server (mirrors original CDlgPickRace::OnOK join path)
    CNetReady readyMsg(&theGame.GetMe()->m_InitData);
    theNet.Send(theGame.GetServerNetNum(), &readyMsg, sizeof(readyMsg));

    if (gameWindow->GetWindow())
        SDL_RaiseWindow(gameWindow->GetWindow());

    try {
        theGame.IncTry();
        theApp.ReadyToJoin();      // signals readiness to the host, sets wait_start
        theGame.DecTry();
    } catch (int iNum) { CatchNum(iNum); theApp.CloseWorld(); return false; }
    catch (...) { CatchOther(); theApp.CloseWorld(); return false; }

    // Step 9: waiting-room lobby. Sit here (showing the connected players) until
    // the host clicks Start. The host's CNetStart is DEFERRED while g_bClientLobby-
    // Waiting is set (netapi.cpp CmdStart), so the world build happens here, after
    // the modal lobby closes — not nested inside its loop.
    extern bool g_bClientLobbyWaiting, g_bClientStartReceived;
    extern void RunDeferredClientStart();

    g_bClientLobbyWaiting  = true;
    g_bClientStartReceived = false;
    // Hide the always-on-top "creating world" status window so it doesn't cover
    // the waiting room; the deferred world build re-shows it for real progress.
    if (theApp.m_pCreateGame && theApp.m_pCreateGame->GetDlgStatus())
        theApp.m_pCreateGame->GetDlgStatus()->Hide();
    int lobbyResult;
    {
        SDL2ClientLobbyDialog clientLobby(gameWindow, theGame.m_sGameName.c_str());
        lobbyResult = clientLobby.DoModal();
    }
    g_bClientLobbyWaiting = false;

    if (lobbyResult != 1) {
        // Player left the waiting room before the game started. ReadyToJoin tore
        // down the main menu, so rebuild it instead of leaving a blank wallpaper.
        theNet.Close(FALSE);
        theGame.Close();
        delete theApp.m_pCreateGame; theApp.m_pCreateGame = NULL;
        theApp.CreateMain();
        return false;
    }

    // Host started: build the world now (outside the modal loop).
    if (gameWindow->GetWindow())
        SDL_RaiseWindow(gameWindow->GetWindow());
    try {
        theGame.IncTry();
        RunDeferredClientStart();
        theGame.DecTry();
    } catch (int iNum) { CatchNum(iNum); theApp.CloseWorld(); return false; }
    catch (...) { CatchOther(); theApp.CloseWorld(); return false; }

    return true;
}

// ============================================================================
// Load Network Game flow
// Host a saved game over TCP/IP so clients can join and resume play.
// ============================================================================
bool SDL2_RunLoadNetworkFlow(GameWindow* gameWindow) {
    ShowWallpaperBackground(gameWindow);
    ASSERT(theApp.m_pCreateGame == NULL);

    CCreateLoadMulti* pCreate = new CCreateLoadMulti();
    theApp.m_pCreateGame = pCreate;

    // Step 1: load a saved game (same as single-player load)
    theGame.ctor();
    theGame.SetServer(TRUE);
    theGame._SetIsNetGame(TRUE);

    if (theGame.LoadGame(theApp.m_pMainWnd, FALSE) != IDOK) {
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    // Step 2: collect player name and port (game settings come from the save)
    ShowWallpaperBackground(gameWindow);
    SDL2HostLoadedDialog hostDlg(gameWindow, theGame.m_sGameName);
    if (hostDlg.DoModal() != 1) {
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    pCreate->m_sName     = hostDlg.m_playerName;
    pCreate->m_sGameName = theGame.m_sGameName;
    std::string sPort = std::to_string(hostDlg.m_iPort);
    WritePrivateProfileString("TCP", "WellKnownPort", sPort.c_str(), ".\\vdmplay.ini");
    ApplyOptionalRegistrationServer();   // P5: register with iserve if configured

    // Step 3: publish the loaded game session for clients to enumerate.
    // OpenServer blocks (vpStartup -> gethostbyname + Listen); show a status frame.
    ShowConnectingMessage(gameWindow, "Starting network game...");
    CNetPublish* pPub = CNetPublish::Alloc(pCreate);
    pPub->m_cFlags |= CNetPublish::fload;
    pPub->m_iNumPlayers = theGame.GetAll().GetCount();
    BOOL bErr = theNet.OpenServer(VPT_TCP, theApp.m_wndMain.m_hWnd,
                                  (LPCSTR)pPub, NULL, NULL);
    delete[] pPub;
    if (bErr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Network Error",
            "Failed to open TCP/IP server.", nullptr);
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    // Step 4: server picks which player they control from the save
    if (gameWindow->GetCreateStatus())
        gameWindow->GetCreateStatus()->Hide();
    ShowWallpaperBackground(gameWindow);

    SDL2PickPlayerDialog pickDlg(gameWindow);
    if (pickDlg.DoModal() != 1) {
        theNet.Close(FALSE);
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    CPlayer* pPlr = theGame.GetPlayerByPlyr(pickDlg.m_iSelectedPlyrNum);
    if (!pPlr) {
        theNet.Close(FALSE);
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    theGame.SetHP(TRUE);
    theGame.SetScenario(-1);
    if (pPlr != theGame._GetMe()) {
        theGame._SetMe(pPlr);
        if (theGame.AmServer()) theGame._SetServer(pPlr);
    }
    pPlr->SetState(CPlayer::ready);
    pPlr->SetName(pickDlg.m_playerName.c_str());
    pCreate->m_sName = pickDlg.m_playerName.c_str();

    // Mark other players as AI-controlled until a client claims them
    POSITION pos;
    for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL;) {
        CPlayer* pP = theGame.GetAll().GetNext(pos);
        if (!pP->IsMe() && pP->GetNetNum() == 0) {
            pP->SetAI(TRUE);
            pP->SetLocal(TRUE);
        }
    }

    // Step 5: lobby — wait for clients to claim saved-game players
    ShowWallpaperBackground(gameWindow);
    SDL2LobbyDialog lobbyDlg(gameWindow, theGame.m_sGameName);
    if (lobbyDlg.DoModal() != 1) {
        theNet.Close(FALSE);
        theGame.Close();
        delete theApp.m_pCreateGame;
        theApp.m_pCreateGame = NULL;
        return false;
    }

    if (gameWindow->GetWindow())
        SDL_RaiseWindow(gameWindow->GetWindow());

    try {
        theGame.IncTry();
        pCreate->ClosePick();
        theApp.ReadyToCreate();
        theGame.DecTry();
    } catch (int iNum) { CatchNum(iNum); theApp.CloseWorld(); return false; }
    catch (...) { CatchOther(); theApp.CloseWorld(); return false; }

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
    static const char* fc[] = { "/System/Library/Fonts/Supplemental/Arial.ttf", "/System/Library/Fonts/Supplemental/Times New Roman.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "C:\\Windows\\Fonts\\BKANT.TTF", "C:\\Windows\\Fonts\\BOOKOS.TTF",
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
        SDL_Surface* surf = cl.text.empty() ? nullptr : TTF_RenderUTF8_Blended(fonts[fi], cl.text.c_str(), white);
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

        SDL_Surface* ws = gameWindow->GetPresentSurface();  // T0: software surface or renderer back-buffer
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

        gameWindow->PresentSurface();
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
