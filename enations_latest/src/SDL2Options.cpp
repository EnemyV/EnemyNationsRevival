#include "stdafx.h"
#include "SDL2Options.h"
#include "SDL2Dialogs.h"
#include "lastplnt.h"
#include "sfx.h"

SDL2OptionsDialog::SDL2OptionsDialog(GameWindow* gameWindow)
    : SDL2Dialog(gameWindow, "Options", 420, 320) {
}

void SDL2OptionsDialog::OnInit() {
    int lx = m_x + 30;   // Label x
    int sx = m_x + 140;  // Slider x
    int sw = 200;         // Slider width
    int rowH = 40;
    int startY = m_y + 50;

    // Game Speed
    AddWidget<SDL2Label>(lx, startY, 100, rowH, "Game Speed:");
    m_sldSpeed = AddWidget<SDL2Slider>(sx, startY + 8, sw, 24,
                                        0, NUM_SPEEDS - 1,
                                        EnGetProfileInt("Game", "Speed", NUM_SPEEDS/2));

    // Sound Volume
    AddWidget<SDL2Label>(lx, startY + rowH, 100, rowH, "Sound:");
    m_sldSound = AddWidget<SDL2Slider>(sx, startY + rowH + 8, sw, 24,
                                        0, 100,
                                        EnGetProfileInt("Game", "Sound", 50));

    // Music Volume
    AddWidget<SDL2Label>(lx, startY + 2*rowH, 100, rowH, "Music:");
    m_sldMusic = AddWidget<SDL2Slider>(sx, startY + 2*rowH + 8, sw, 24,
                                        0, 100,
                                        EnGetProfileInt("Game", "Music", 50));

    // Buttons
    int btnW = 90, btnH = 30;
    int btnY = m_y + m_height - btnH - 15;

    // Version button
    AddWidget<SDL2Button>(m_x + 20, btnY - 45, btnW, btnH, "Version", [this]() {
        SDL2VersionDialog dlg(m_gameWindow);
        dlg.DoModal();
    });

    // Advanced button
    AddWidget<SDL2Button>(m_x + 120, btnY - 45, btnW, btnH, "Advanced", [this]() {
        SDL2AdvOptionsDialog dlg(m_gameWindow);
        dlg.DoModal();
    });

    // Help button
    AddWidget<SDL2Button>(m_x + 220, btnY - 45, btnW, btnH, "Help", []() {
        theApp.WinHelp(0, HELP_CONTENTS);
    });

    // OK / Cancel
    AddOKCancelButtons();
}

void SDL2OptionsDialog::OnOK() {
    int iSpeed = m_sldSpeed->GetValue();
    int iSound = m_sldSound->GetValue();
    int iMusic = m_sldMusic->GetValue();

    ASSERT((0 <= iSpeed) && (iSpeed < NUM_SPEEDS));
    EnWriteProfileInt("Game", "Speed", iSpeed);
    if (theGame.IsNetGame() && theGame.GetMyNetNum() != 0) {
        // Net game: route the change through the server's speed negotiation
        // (GameSpeed, netapi.cpp) so every player converges on a consensus rate.
        // Calling SetGameMul locally only changes OUR sim tick rate = a speed
        // desync (host and client running at different speeds). The negotiated
        // result comes back via a broadcast CMsgGameSpeed.
        CMsgGameSpeed msg(iSpeed);
        theGame.PostToServer(&msg, sizeof(msg));
    } else {
        theGame.SetGameMul(iSpeed);
    }

    EnWriteProfileInt("Game", "Sound", iSound);
    theMusicPlayer.SetSoundVolume(iSound);

    EnWriteProfileInt("Game", "Music", iMusic);
    theMusicPlayer.SetMusicVolume(iMusic);

    EndDialog(1);
}
