//---------------------------------------------------------------------------
// win32_resources_linux.cpp — LoadStringA backing for the Linux build.
//
// The Windows build compiles lastplnt.rc, whose STRINGTABLE feeds runtime
// ::LoadStringA(GetModuleHandleA(NULL), id, ...) calls (e.g. EnSettings.cpp).
// gcc/CMake cannot compile a .RC, so lastplnt.rc is dropped from the Linux
// source list and the string table is provided here as a compiled-in map.
//
// The table is intentionally empty for the initial bring-up: a missing IDS_*
// degrades gracefully to an empty string (callers treat that as "no override").
// Once the game runs, enumerate which IDS_* are actually loaded in single-player
// and populate g_stringTable from lastplnt.rc's STRINGTABLE. Linux build only.
//---------------------------------------------------------------------------

#ifdef _WIN32
#error "win32_resources_linux.cpp is Linux-only and must not be compiled on Windows"
#endif

#include "win32_compat.h"
#include "resource.h"   // IDS_* ids (plain #defines; no Win32/MFC pulls)

#include <unordered_map>
#include <string>

namespace {
// id -> string. Populated from lastplnt.rc's STRINGTABLE as runtime needs surface.
// MP-network error strings (IDS_VP*_FAILED, 230-234) surfaced EMPTY on Linux during the
// discover/create/join flow (EnMessageBox in netapi.cpp) — e.g. a failed join showed a
// blank message box. Text copied verbatim from lastplnt.rc's STRINGTABLE.
const std::unordered_map<UINT, std::string>& string_table() {
    static const std::unordered_map<UINT, std::string> t = {
        { IDS_VPSTARTUP_FAILED, "The Network layer could not initialize.\nYou may want to try a different network connection type." },
        { IDS_VPCREATE_FAILED,  "The network layer could not create a communication channel.\nYou might want to try closing another network application\nyou are running and then try again." },
        { IDS_VPENUM_FAILED,    "The network layer could not ask for a list of games.\nYou may want to try a different network connection type." },
        { IDS_VPJOIN_FAILED,    "The network layer could not connect to the requested game.\nThe game may have already started." },
        { IDS_VPSEND_FAILED,    "The network layer could not talk to one of the other player's computers.\nYou may want to try to reconnect." },
        // MP session-close / game-start strings: these surfaced as BLANK message
        // boxes during the live Win-host↔Linux-client test (the rand-mismatch
        // disconnect showed an empty box — undiagnosable from the operator's
        // seat). Keep the %1 placeholder VERBATIM from lastplnt.rc: this port's
        // strPrintf/EnLoadStdString path uses MFC FormatMessage-style %1/%2
        // positional args, NOT printf %s (a live disconnect printed literal
        // "The game %s" when I wrongly used %s here).
        { IDS_RAND_MISMATCH,    "Did not generate identical world to server.\nYou have been disconnected from the game.\nYou can rejoin the game and it will then copy the world to\nyou (much slower but a guaranteed match)." },
        { IDS_JOIN_UNJOIN,      "The game %1\nhas disconnected from the net." },
        { IDS_SAVE_CLOSE,       "The game %1\nhas disconnected from the net.\nDo you wish to save the existing game?" },
        { IDS_JOIN_FILE_ERROR,  "Could not receive game from server" },
        { IDS_UNKNOWN,          "{unknown}" },
    };
    return t;
}
} // namespace

extern "C" int LoadStringA(HINSTANCE /*inst*/, UINT id, LPSTR buf, int bufMax) {
    if (!buf || bufMax <= 0) return 0;
    const std::unordered_map<UINT, std::string>& t = string_table();
    std::unordered_map<UINT, std::string>::const_iterator it = t.find(id);
    if (it == t.end()) { buf[0] = '\0'; return 0; }
    int n = (int)it->second.size();
    if (n > bufMax - 1) n = bufMax - 1;
    memcpy(buf, it->second.data(), n);
    buf[n] = '\0';
    return n;
}
