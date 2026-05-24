#include "stdafx.h"

#include "SDL2FileBrowser.h"
#include "GameWindow.h"
#include "lastplnt.h"

#include <algorithm>
#include <io.h>        // _findfirst, _findnext
#include <direct.h>    // _getcwd
#include <sys/stat.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Case-insensitive ends-with check
static bool EndsWithCI(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    for (size_t i = 0; i < suffix.size(); i++) {
        char a = str[str.size() - suffix.size() + i];
        char b = suffix[i];
        if (tolower((unsigned char)a) != tolower((unsigned char)b))
            return false;
    }
    return true;
}

// Last directory the user actually navigated to — persists across browser
// instances so back-to-back Save/Load operations open in the same place.
// Initialized lazily to the running exe's directory the first time the
// browser opens (more reliable than _getcwd, which CFileDialog and friends
// can change as a side effect).
static std::string& LastBrowserDir() {
    static std::string s_dir;
    return s_dir;
}

static std::string ExeDirectory() {
    char buf[MAX_PATH] = { 0 };
    DWORD len = ::GetModuleFileNameA(NULL, buf, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return std::string();
    std::string path(buf, len);
    size_t slash = path.find_last_of("\\/");
    return (slash == std::string::npos) ? path : path.substr(0, slash);
}

SDL2FileBrowser::SDL2FileBrowser(GameWindow* gw, Mode mode,
                                 const std::string& title,
                                 const std::string& defaultDir,
                                 const std::string& defaultFile,
                                 const std::string& filter)
    : SDL2Dialog(gw, title, DLG_W, DLG_H)
    , m_mode(mode)
    , m_currentDir(defaultDir)
    , m_filter(filter)
{
    if (m_currentDir.empty()) {
        // Prefer the last directory the user navigated to, then the exe's
        // own directory (where the game's saves live by convention), then
        // fall back to _getcwd as a last resort.
        if (!LastBrowserDir().empty())
            m_currentDir = LastBrowserDir();
        else
            m_currentDir = ExeDirectory();
        if (m_currentDir.empty()) {
            char buf[MAX_PATH];
            if (_getcwd(buf, MAX_PATH))
                m_currentDir = buf;
        }
    }
    // Ensure no trailing backslash (unless root like "C:\")
    while (m_currentDir.size() > 3 && (m_currentDir.back() == '\\' || m_currentDir.back() == '/'))
        m_currentDir.pop_back();

    if (!defaultFile.empty())
        m_selectedPath = defaultFile;
}

void SDL2FileBrowser::OnInit() {
    int brdSide = 8;
    int y = m_y + 34;  // below title bar
    int innerW = m_width - brdSide * 2;
    int lx = m_x + brdSide;

    // Path label (shows current directory)
    m_lblPath = AddWidget<SDL2Label>(lx, y, innerW, 20, m_currentDir,
        SDL_Color{30, 25, 15, 255});
    y += 24;

    // Up button — handles three cases:
    //   "C:\Foo\Bar" → "C:\Foo"        (pos > 2: normal parent)
    //   "C:\Foo"      → "C:\\"          (pos == 2: parent is drive root)
    //   "C:\\"        → no-op           (already at root)
    AddWidget<SDL2Button>(lx, y, 60, 24, "Up ..",
        [this]() {
            size_t pos = m_currentDir.find_last_of("\\/");
            if (pos == std::string::npos) return;
            if (pos > 2)
                NavigateTo(m_currentDir.substr(0, pos));
            else if (pos == 2 && m_currentDir.size() > 3)
                NavigateTo(m_currentDir.substr(0, 3));  // "X:\"
        });

    // Drive buttons — one per available logical drive (C:, D:, ...). The
    // "Up .." button alone can't escape the current drive tree, which made
    // users stuck on whichever drive the dialog opened on.
    {
        DWORD mask = ::GetLogicalDrives();
        int dx = lx + 70;  // start just right of Up button
        for (int i = 0; i < 26 && dx + 36 <= lx + innerW; i++) {
            if (!(mask & (1u << i))) continue;
            std::string label;
            label += static_cast<char>('A' + i);
            label += ':';
            std::string target = label + "\\";
            AddWidget<SDL2Button>(dx, y, 32, 24, label,
                [this, target]() { NavigateTo(target); });
            dx += 34;
        }
    }
    y += 28;

    // File list
    int listH = m_height - 180;  // leave room for edit + buttons
    m_listFiles = AddWidget<SDL2Listbox>(lx, y, innerW, listH,
        [this](int idx) { OnFileSelected(idx); },
        [this](int idx) { OnFileDblClick(idx); });
    y += listH + 6;

    // Filename label + edit + extension suffix label.
    // In Save mode we pre-fill with just the base name (extension stripped)
    // and render the extension as a non-editable suffix label so the user can
    // see what the final filename will be without it cluttering the editor.
    AddWidget<SDL2Label>(lx, y, 70, 24, "Filename:",
        SDL_Color{30, 25, 15, 255});

    std::string editText = m_selectedPath;
    int extLabelW = 0;
    if (m_mode == Save && !m_filter.empty()) {
        if (EndsWithCI(editText, m_filter))
            editText = editText.substr(0, editText.size() - m_filter.size());
        extLabelW = 40;  // room for ".en " etc.
    }

    int editW = innerW - 75 - extLabelW;
    m_editFilename = AddWidget<SDL2EditBox>(lx + 75, y, editW, 24, editText);

    if (extLabelW > 0) {
        AddWidget<SDL2Label>(lx + 75 + editW + 4, y, extLabelW, 24, m_filter,
            SDL_Color{30, 25, 15, 255});
    }
    y += 30;

    // Buttons
    int btnW = 100, btnH = 28;
    int btnGap = 10;
    int btnX = lx + (innerW - btnW * 2 - btnGap) / 2;

    std::string confirmText = (m_mode == Open) ? "Open" : "Save";
    AddWidget<SDL2Button>(btnX, y, btnW, btnH, confirmText,
        [this]() { OnConfirm(); });
    AddWidget<SDL2Button>(btnX + btnW + btnGap, y, btnW, btnH, "Cancel",
        [this]() { OnCancel(); });

    // Populate the file list
    PopulateList();
}

void SDL2FileBrowser::NavigateTo(const std::string& dir) {
    m_currentDir = dir;
    // Strip trailing separators
    while (m_currentDir.size() > 3 && (m_currentDir.back() == '\\' || m_currentDir.back() == '/'))
        m_currentDir.pop_back();

    if (m_lblPath)
        m_lblPath->SetText(m_currentDir);
    PopulateList();
    // Remember for the next browser instance so Save→Load round-trips return
    // to the same directory.
    LastBrowserDir() = m_currentDir;
}

void SDL2FileBrowser::PopulateList() {
    m_entries.clear();
    if (m_listFiles)
        m_listFiles->Clear();

    // Use Win32 _findfirst / _findnext to enumerate directory contents.
    std::string searchPath = m_currentDir + "\\*";

    struct _finddata_t fd;
    intptr_t hFind = _findfirst(searchPath.c_str(), &fd);
    if (hFind == -1) return;

    std::vector<FileEntry> dirs;
    std::vector<FileEntry> files;

    do {
        std::string name = fd.name;
        if (name == ".") continue;

        FileEntry entry;
        entry.name = name;
        entry.fullPath = m_currentDir + "\\" + name;
        entry.isDir = (fd.attrib & _A_SUBDIR) != 0;

        if (entry.isDir) {
            dirs.push_back(entry);
        } else {
            // Filter to the matching extension in BOTH modes — in Save mode
            // this is what lets the user see "what saves already exist" without
            // drowning in unrelated files (DLLs, the .dat, docs, logs).
            if (!m_filter.empty() && !EndsWithCI(name, m_filter))
                continue;
            files.push_back(entry);
        }
    } while (_findnext(hFind, &fd) == 0);
    _findclose(hFind);

    // Sort: directories first (alpha), then files (alpha)
    std::sort(dirs.begin(), dirs.end(),
        [](const FileEntry& a, const FileEntry& b) { return _stricmp(a.name.c_str(), b.name.c_str()) < 0; });
    std::sort(files.begin(), files.end(),
        [](const FileEntry& a, const FileEntry& b) { return _stricmp(a.name.c_str(), b.name.c_str()) < 0; });

    // Add directories with [folder] prefix
    for (auto& d : dirs) {
        m_entries.push_back(d);
        if (m_listFiles)
            m_listFiles->AddItem("[" + d.name + "]");
    }

    // Add files
    for (auto& f : files) {
        m_entries.push_back(f);
        if (m_listFiles)
            m_listFiles->AddItem(f.name);
    }
}

void SDL2FileBrowser::OnFileSelected(int index) {
    if (index < 0 || index >= (int)m_entries.size()) return;

    const auto& entry = m_entries[index];
    if (!entry.isDir && m_editFilename) {
        // Put the filename (not full path) into the edit box. In Save mode
        // we strip the extension so it matches the suffix label we render.
        std::string name = entry.name;
        if (m_mode == Save && !m_filter.empty() && EndsWithCI(name, m_filter))
            name = name.substr(0, name.size() - m_filter.size());
        m_editFilename->SetText(name);
    }
}

void SDL2FileBrowser::OnFileDblClick(int index) {
    if (index < 0 || index >= (int)m_entries.size()) return;

    const auto& entry = m_entries[index];
    if (entry.isDir) {
        NavigateTo(entry.fullPath);
    } else {
        // Double-click on a file = select and confirm
        if (m_editFilename)
            m_editFilename->SetText(entry.name);
        OnConfirm();
    }
}

void SDL2FileBrowser::OnConfirm() {
    std::string filename;
    if (m_editFilename)
        filename = m_editFilename->GetText();

    if (filename.empty()) return;

    // If the filename has no path separator, prepend current directory
    if (filename.find('\\') == std::string::npos &&
        filename.find('/') == std::string::npos) {
        filename = m_currentDir + "\\" + filename;
    }

    // In Save mode, add extension if missing
    if (m_mode == Save && !m_filter.empty()) {
        if (!EndsWithCI(filename, m_filter))
            filename += m_filter;
    }

    // In Open mode, verify file exists
    if (m_mode == Open) {
        struct _stat st;
        if (_stat(filename.c_str(), &st) != 0 || (st.st_mode & _S_IFREG) == 0) {
            // File doesn't exist — don't close
            return;
        }
    }

    m_selectedPath = filename;
    m_confirmed = true;
    EndDialog(1);
}

void SDL2FileBrowser::OnCancel() {
    m_confirmed = false;
    EndDialog(0);
}
