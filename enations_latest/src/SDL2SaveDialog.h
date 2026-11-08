#pragma once

#include "SDL2UI.h"

// Native SDL2 save game dialog. Replaces CFileDialog for save.
// Shows a filename edit box and Save/Cancel buttons.

class SDL2SaveDialog : public SDL2Dialog {
public:
    SDL2SaveDialog(GameWindow* gw, const std::string& defaultName);

    std::string GetFilename() const { return m_filename; }
    bool WasSaved() const { return m_saved; }

protected:
    void OnInit() override;
    void OnOK() override;   // Enter -> Save (not the base EndDialog(1), which skips the save)

private:
    void OnSave();

    std::string m_filename;
    bool m_saved = false;
    SDL2EditBox* m_editName = nullptr;
};
