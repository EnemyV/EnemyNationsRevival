#pragma once

#include "SDL2UI.h"

// SDL2 replacement for CDlgOptions
class SDL2OptionsDialog : public SDL2Dialog {
public:
    SDL2OptionsDialog(GameWindow* gameWindow);

protected:
    void OnInit() override;
    void OnOK() override;

private:
    SDL2Slider* m_sldSpeed = nullptr;
    SDL2Slider* m_sldSound = nullptr;
    SDL2Slider* m_sldMusic = nullptr;
};
