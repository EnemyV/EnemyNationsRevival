#include "stdafx.h"

#include "SDL2SaveDialog.h"
#include "GameWindow.h"
#include "lastplnt.h"

SDL2SaveDialog::SDL2SaveDialog(GameWindow* gw, const std::string& defaultName)
    : SDL2Dialog(gw, "Save Game", 350, 150)
    , m_filename(defaultName)
{
}

void SDL2SaveDialog::OnInit() {
    AddWidget<SDL2Label>(m_x + 15, m_y + 36, 320, 20, "Enter filename:");

    m_editName = AddWidget<SDL2EditBox>(m_x + 15, m_y + 60, 320, 24, m_filename);

    int btnW = 100, btnH = 26;
    AddWidget<SDL2Button>(m_x + 70, m_y + 100, btnW, btnH, "Save",
        [this]() { OnSave(); });
    AddWidget<SDL2Button>(m_x + 180, m_y + 100, btnW, btnH, "Cancel",
        [this]() { EndDialog(0); });
}

void SDL2SaveDialog::OnSave() {
    if (m_editName) {
        m_filename = m_editName->GetText();
        if (m_filename.empty())
            return;

        // Add .en extension if missing (matches IDS_SAVE_EXT)
        if (m_filename.find('.') == std::string::npos)
            m_filename += ".en";

        // Add save game directory if no path
        if (m_filename.find('\\') == std::string::npos &&
            m_filename.find('/') == std::string::npos) {
            char szPath[MAX_PATH];
            GetCurrentDirectoryA(MAX_PATH, szPath);
            m_filename = std::string(szPath) + "\\" + m_filename;
        }
    }
    m_saved = true;
    EndDialog(1);
}
