#pragma once

#include "SDL2UI.h"

// Native SDL2 replacement for CDlgFile (save/load/options dialog).
// Opened from the toolbar "File" button.

class SDL2FileDialog : public SDL2Dialog {
public:
    SDL2FileDialog(GameWindow* gw);

protected:
    void OnInit() override;
    void OnOK() override;

private:
    void OnSave();
    void OnExit();
    void OnHelp();
    void OnMinimize();

    void ApplySettings();

    SDL2Slider* m_sldSpeed = nullptr;
    SDL2Slider* m_sldSound = nullptr;
    SDL2Slider* m_sldMusic = nullptr;
};
