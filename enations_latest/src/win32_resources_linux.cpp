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

#include <unordered_map>
#include <string>

namespace {
// id -> string. Populate from lastplnt.rc STRINGTABLE as runtime needs surface.
const std::unordered_map<UINT, std::string>& string_table() {
    static const std::unordered_map<UINT, std::string> t = {
        // { IDS_FOO, "Foo" },
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
