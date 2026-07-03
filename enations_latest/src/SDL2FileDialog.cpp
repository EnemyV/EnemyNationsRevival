#include "stdafx.h"

#include "SDL2FileDialog.h"
#include "SDL2SaveDialog.h"
#include "SDL2FileBrowser.h"
#include "SDL2Dialogs.h"
#include "GameWindow.h"
#include "SDL2Compositor.h"
#include "lastplnt.h"
#include "player.h"
#include "sfx.h"

SDL2FileDialog::SDL2FileDialog(GameWindow* gw)
    : SDL2Dialog(gw, "Game Options", 360, 344)
{
}

void SDL2FileDialog::OnInit() {
    // Anchor to the usable interior (below the title bar, inside the gold border)
    // instead of guessing offsets off m_y — that previously slid the Settings box
    // under the title bar. The four button rows are then distributed evenly down
    // to the bottom border so there's no dead space at the foot of the dialog.
    const int cl = ContentLeft(), cr = ContentRight();
    const int cTop = ContentTop(), cBot = ContentBottom();

    // --- Settings group: speed + volumes, boxed together -------------------
    const int gbX = cl + 6, gbW = (cr - cl) - 12;
    const int gbY = cTop + 8, gbH = 118;
    AddWidget<SDL2GroupBox>(gbX, gbY, gbW, gbH, "Settings");

    int labelW = 86;
    int sliderX = gbX + 14 + labelW + 8;
    int sliderRight = gbX + gbW - 48;        // leave room for the value text
    int sliderW = sliderRight - sliderX;
    int y = gbY + 22;

    AddWidget<SDL2Label>(gbX + 14, y, labelW, 24, "Game Speed:");
    m_sldSpeed = AddWidget<SDL2Slider>(sliderX, y, sliderW, 24,
        0, NUM_SPEEDS - 1, theGame.GetGameMul());
    y += 32;

    AddWidget<SDL2Label>(gbX + 14, y, labelW, 24, "Sound:");
    m_sldSound = AddWidget<SDL2Slider>(sliderX, y, sliderW, 24,
        0, 100, theMusicPlayer.GetSoundVolume());
    y += 32;

    AddWidget<SDL2Label>(gbX + 14, y, labelW, 24, "Music:");
    m_sldMusic = AddWidget<SDL2Slider>(sliderX, y, sliderW, 24,
        0, 100, theMusicPlayer.GetMusicVolume());

    // --- Action buttons: a tidy 2-column grid filling the rest of the dialog
    //   Save | Load           (file)
    //   Minimize | Help        (utility)
    //   Reset Windows          (full width — restore default window layout)
    //   Exit Game | Resume     (leave vs return)
    const int btnW = 152, btnH = 30, colGap = 12;
    const int gridX = m_x + (m_width - (btnW * 2 + colGap)) / 2;
    const int col2 = gridX + btnW + colGap;
    const int fullW = btnW * 2 + colGap;

    // Distribute the 4 rows evenly between the settings box and the bottom border.
    const int areaTop = gbY + gbH + 12;
    const int areaBot = cBot - 8;
    int rowGap = (areaBot - areaTop - 4 * btnH) / 3;
    if (rowGap < 8)  rowGap = 8;
    if (rowGap > 16) rowGap = 16;
    const int rowStep = btnH + rowGap;
    int by = areaTop;

    AddWidget<SDL2Button>(gridX, by, btnW, btnH, "Save Game", [this]() { OnSave(); });
    AddWidget<SDL2Button>(col2,  by, btnW, btnH, "Load Game", [this]() { OnLoad(); });
    by += rowStep;

    AddWidget<SDL2Button>(gridX, by, btnW, btnH, "Minimize", [this]() { OnMinimize(); });
    AddWidget<SDL2Button>(col2,  by, btnW, btnH, "Help",     [this]() { OnHelp(); });
    by += rowStep;

    AddWidget<SDL2Button>(gridX, by, fullW, btnH, "Reset Windows", [this]() { OnResetWindows(); });
    by += rowStep;

    AddWidget<SDL2Button>(gridX, by, btnW, btnH, "Exit Game",   [this]() { OnExit(); });
    AddWidget<SDL2Button>(col2,  by, btnW, btnH, "Resume Game", [this]() { OnOK(); });
}

void SDL2FileDialog::ApplySettings() {
    if (m_sldSpeed) {
        int speed = m_sldSpeed->GetValue();
        if (theGame.IsNetGame() && theGame.GetMyNetNum() != 0) {
            // Net game: negotiate via the server (GameSpeed, netapi.cpp) instead
            // of setting only our local sim rate — otherwise host/client run at
            // different speeds (speed desync). Consensus returns via broadcast.
            CMsgGameSpeed msg(speed);
            theGame.PostToServer(&msg, sizeof(msg));
        } else {
            theGame.SetGameMul(speed);
        }
        EnWriteProfileInt("Game", "Speed", speed);
    }
    if (m_sldSound) {
        int vol = m_sldSound->GetValue();
        theMusicPlayer.SetSoundVolume(vol);
        EnWriteProfileInt("Game", "Sound", vol);
    }
    if (m_sldMusic) {
        int vol = m_sldMusic->GetValue();
        theMusicPlayer.SetMusicVolume(vol);
        EnWriteProfileInt("Game", "Music", vol);
    }
}

void SDL2FileDialog::OnOK() {
    ApplySettings();
    EndDialog(1);
}

void SDL2FileDialog::OnSave() {
    ApplySettings();
    DismissModalNow();   // close this menu before the (modal) save file browser opens

    // Get filename via SDL2 file browser with full directory navigation
    std::string defaultName = theGame.m_sFileName;
    // Strip path to just filename for the edit box default
    size_t lastSlash = defaultName.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        defaultName = defaultName.substr(lastSlash + 1);
    if (defaultName.empty()) defaultName = "savegame";

    SDL2FileBrowser browser(m_gameWindow, SDL2FileBrowser::Save,
                            "Save Game", "", defaultName, ".en");
    browser.DoModal();

    if (browser.WasConfirmed()) {
        // Pre-set the filename so SaveGame can skip its file dialog
        theGame.m_sFileName = browser.GetSelectedPath().c_str();
        theGame.SaveGame( (CWnd*)NULL );
    }
}

// Confirmation dialog for exit
class SDL2ConfirmExit : public SDL2Dialog {
public:
    SDL2ConfirmExit(GameWindow* gw) : SDL2Dialog(gw, "Exit Game", 280, 120), m_confirmed(false) {}
    bool WasConfirmed() const { return m_confirmed; }
protected:
    void OnInit() override {
        auto* lbl = AddWidget<SDL2Label>(m_x + 15, m_y + 36, 250, 30, "Are you sure you want to exit?");
        lbl->SetCentered(true);
        AddWidget<SDL2Button>(m_x + 40, m_y + 78, 90, 26, "Yes",
            [this]() { m_confirmed = true; EndDialog(1); });
        AddWidget<SDL2Button>(m_x + 150, m_y + 78, 90, 26, "No",
            [this]() { EndDialog(0); });
    }
private:
    bool m_confirmed;
};

void SDL2FileDialog::OnExit() {
    SDL2ConfirmExit confirm(m_gameWindow);
    confirm.DoModal();
    if (!confirm.WasConfirmed()) return;

    ApplySettings();
    DismissModalNow();   // vanish now; CloseWorld tears down to the main menu
    theApp.CloseWorld();
}

// Confirmation dialog for load (discards current game)
class SDL2ConfirmLoad : public SDL2Dialog {
public:
    SDL2ConfirmLoad(GameWindow* gw) : SDL2Dialog(gw, "Load Game", 320, 140), m_confirmed(false) {}
    bool WasConfirmed() const { return m_confirmed; }
protected:
    void OnInit() override {
        auto* lbl = AddWidget<SDL2Label>(m_x + 15, m_y + 36, 290, 40,
            "Loading will discard your current game.\nContinue?");
        lbl->SetCentered(true);
        lbl->SetWrapped(true);
        AddWidget<SDL2Button>(m_x + 60, m_y + 98, 90, 26, "Yes",
            [this]() { m_confirmed = true; EndDialog(1); });
        AddWidget<SDL2Button>(m_x + 170, m_y + 98, 90, 26, "No",
            [this]() { EndDialog(0); });
    }
private:
    bool m_confirmed;
};

void SDL2FileDialog::OnLoad() {
    SDL2ConfirmLoad confirm(m_gameWindow);
    confirm.DoModal();
    if (!confirm.WasConfirmed()) return;

    ApplySettings();
    // Take this dialog's window down NOW — the load flow below runs a long nested
    // loop (file picker + player pick) before DoModal can unwind, so a plain
    // EndDialog would leave the Game Options menu on-screen through the whole flow.
    DismissModalNow();

    // Tear down the current world (same path as Exit), then run the main-menu
    // load flow. CloseWorld → DestroyWorld + CreateMain, after which
    // m_pCreateGame is NULL and we satisfy SDL2_RunLoadSinglePlayerFlow's
    // precondition. If the user cancels the file picker the main menu stays
    // up — same behavior as clicking "Load Single Player Game" there directly.
    theApp.CloseWorld();
    SDL2_RunLoadSinglePlayerFlow(m_gameWindow);
}

void SDL2FileDialog::OnHelp() {
    // The original WinHelp .hlp file is long gone; point players at the revival
    // project's issue tracker instead so Help opens something useful.
#ifdef _WIN32
    // The game holds the foreground, and its borderless map/dialog windows are
    // owned by (and therefore drawn above) the main window — so the browser that
    // SDL_OpenURL launches opens *behind* the game and Help looks like it did
    // nothing. Grant the next process the right to take the foreground so the
    // browser can raise itself to the front.
    ::AllowSetForegroundWindow(ASFW_ANY);
#endif
    if (SDL_OpenURL("https://github.com/EnemyV/EnemyNationsRevival/issues") != 0)
        SDL_Log("Help: SDL_OpenURL failed: %s", SDL_GetError());
}

void SDL2FileDialog::OnResetWindows() {
    // Restore the default on-screen layout for the in-game windows (area/world/
    // radar maps, vehicle/building lists) and forget any remembered positions.
    if (m_gameWindow && m_gameWindow->GetCompositor())
        m_gameWindow->GetCompositor()->ResetWindowLayout();
    EndDialog(1);
}

void SDL2FileDialog::OnMinimize() {
    ApplySettings();
    EndDialog(1);
    // Minimize the whole window group — in-game the detached ALWAYS_ON_TOP
    // panels (area map, radar, ...) cover the screen, so minimizing only the
    // main window looked like the button did nothing (operator-reported, mac).
    if (m_gameWindow)
        m_gameWindow->MinimizeAll();
}
