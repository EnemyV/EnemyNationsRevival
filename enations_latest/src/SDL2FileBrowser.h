#pragma once

#include "SDL2UI.h"
#include <string>
#include <vector>

// Native SDL2 file browser dialog, replacing MFC CFileDialog.
// Shows a directory listing with file/folder entries, a path bar,
// and a filename edit box. Supports both Open and Save modes.
//
// Used by:
//   - Main menu "Load Single Player Game" / "Load Network Game"
//   - In-game "Save Game" (via SDL2FileDialog)

class SDL2FileBrowser : public SDL2Dialog {
public:
    enum Mode { Open, Save };

    // filter: e.g. ".en" (only show files with this extension in Open mode)
    SDL2FileBrowser(GameWindow* gw, Mode mode,
                    const std::string& title,
                    const std::string& defaultDir,
                    const std::string& defaultFile = "",
                    const std::string& filter = ".en");

    // After DoModal returns 1, call this to get the chosen path.
    std::string GetSelectedPath() const { return m_selectedPath; }
    bool WasConfirmed() const { return m_confirmed; }

protected:
    void OnInit() override;

private:
    void PopulateList();
    void NavigateTo(const std::string& dir);
    void OnFileSelected(int index);
    void OnFileDblClick(int index);
    void OnConfirm();
    void OnCancel();

    // Entry in the file list
    struct FileEntry {
        std::string name;       // Display name
        std::string fullPath;   // Full filesystem path
        bool isDir;
    };

    Mode m_mode;
    std::string m_currentDir;
    std::string m_filter;       // Extension filter (e.g. ".en")
    std::string m_selectedPath; // Result

    std::vector<FileEntry> m_entries;
    bool m_confirmed = false;

    // Widgets
    SDL2Label*   m_lblPath = nullptr;
    SDL2Listbox* m_listFiles = nullptr;
    SDL2EditBox* m_editFilename = nullptr;

    static const int DLG_W = 480;
    static const int DLG_H = 420;
};
