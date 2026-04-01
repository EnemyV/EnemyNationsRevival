#include "stdafx.h"

#include "SDL2FileDialog.h"
#include "SDL2SaveDialog.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "player.h"
#include "sfx.h"

SDL2FileDialog::SDL2FileDialog(GameWindow* gw)
    : SDL2Dialog(gw, "Game Options", 360, 310)
{
}

void SDL2FileDialog::OnInit() {
    int y = m_y + 36;
    int labelW = 90;
    int sliderX = m_x + labelW + 10;
    // Leave 45px right margin for value text (e.g. "100")
    int sliderW = m_width - labelW - 55;

    // Game Speed
    AddWidget<SDL2Label>(m_x + 15, y, labelW, 24, "Game Speed:");
    m_sldSpeed = AddWidget<SDL2Slider>(sliderX, y, sliderW, 24,
        0, NUM_SPEEDS - 1, theGame.GetGameMul());
    y += 34;

    // Sound Volume
    AddWidget<SDL2Label>(m_x + 15, y, labelW, 24, "Sound:");
    m_sldSound = AddWidget<SDL2Slider>(sliderX, y, sliderW, 24,
        0, 100, theMusicPlayer.GetSoundVolume());
    y += 34;

    // Music Volume
    AddWidget<SDL2Label>(m_x + 15, y, labelW, 24, "Music:");
    m_sldMusic = AddWidget<SDL2Slider>(sliderX, y, sliderW, 24,
        0, 100, theMusicPlayer.GetMusicVolume());
    y += 40;

    // Buttons in a centered 2x2 grid + close button
    int btnW = 100, btnH = 26, btnGap = 6;
    int gridW = btnW * 2 + btnGap;
    int gridX = m_x + (m_width - gridW) / 2;

    AddWidget<SDL2Button>(gridX, y, btnW, btnH, "Save Game",
        [this]() { OnSave(); });
    AddWidget<SDL2Button>(gridX + btnW + btnGap, y, btnW, btnH, "Exit Game",
        [this]() { OnExit(); });
    y += btnH + btnGap;

    AddWidget<SDL2Button>(gridX, y, btnW, btnH, "Help",
        [this]() { OnHelp(); });
    AddWidget<SDL2Button>(gridX + btnW + btnGap, y, btnW, btnH, "Minimize",
        [this]() { OnMinimize(); });
    y += btnH + btnGap + 4;

    AddWidget<SDL2Button>(m_x + (m_width - btnW) / 2, y, btnW, btnH, "Close",
        [this]() { OnOK(); });
}

void SDL2FileDialog::ApplySettings() {
    if (m_sldSpeed) {
        int speed = m_sldSpeed->GetValue();
        theGame.SetGameMul(speed);
        theApp.WriteProfileInt("Game", "Speed", speed);
    }
    if (m_sldSound) {
        int vol = m_sldSound->GetValue();
        theMusicPlayer.SetSoundVolume(vol);
        theApp.WriteProfileInt("Game", "Sound", vol);
    }
    if (m_sldMusic) {
        int vol = m_sldMusic->GetValue();
        theMusicPlayer.SetMusicVolume(vol);
        theApp.WriteProfileInt("Game", "Music", vol);
    }
}

void SDL2FileDialog::OnOK() {
    ApplySettings();
    EndDialog(1);
}

void SDL2FileDialog::OnSave() {
    ApplySettings();
    EndDialog(1);

    // Get filename via SDL2 dialog, then let SaveGame handle the rest
    std::string defaultName = (const char*)theGame.m_sFileName;
    if (defaultName.empty()) defaultName = "savegame";

    SDL2SaveDialog saveDlg(m_gameWindow, defaultName);
    saveDlg.DoModal();

    if (saveDlg.WasSaved()) {
        // Pre-set the filename so SaveGame can skip its file dialog
        theGame.m_sFileName = saveDlg.GetFilename().c_str();
        theGame.SaveGame(NULL);
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
    EndDialog(0);
    theApp.CloseWorld();
}

void SDL2FileDialog::OnHelp() {
    theApp.WinHelp(0, HELP_CONTENTS);
}

void SDL2FileDialog::OnMinimize() {
    ApplySettings();
    EndDialog(1);
    // Minimize the SDL window
    if (m_gameWindow && m_gameWindow->GetWindow())
        SDL_MinimizeWindow(m_gameWindow->GetWindow());
}
