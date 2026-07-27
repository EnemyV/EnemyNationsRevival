// iserve_main_win.cpp — headless CLI entry for the VDMPLAY registration server, WINDOWS variant.
//
// The Windows analog of iserve_main.cpp. Same CIserveCore (vpStartup +
// vpStartRegistrationServer + WriteIni), but the service pump is the NATIVE Win32
// message loop instead of vpPumpNet(): vpPumpNet is POSIX-only (#ifndef _WIN32 in
// vdmplay.cpp). On Windows the engine self-creates a hidden message window
// (CVdmPlay::InitWindowsStuff) that owns the WSAAsyncSelect socket notifications
// (WM_WINSOCK) and a 250ms timer (WM_TIMER); a standard GetMessage/DispatchMessage
// loop on this thread drives it. No MFC, console subsystem.
//
//   iserve_headless [--port N] [--proto tcp|ipx]
//
// Ctrl-C / console close shuts down cleanly (posts WM_QUIT to break the loop).

#include <winsock2.h>       // BEFORE windows.h — base.h→stdafx.h pulls afxwin.h (needs winsock2, not v1)
#include <windows.h>
#include "vdmplay.h"        // VPT_TCP / VPT_IPX
// NB: not "base.h" — on MSVC it pulls stdafx.h → <afxwin.h> → MFC (mfc140.dll), which
// defeats the no-MFC headless goal. We need only DEF_IPX_PORT, so define it directly.
static const int DEF_IPX_PORT = 1707;

// Default game GUID = Enemy Nations' tlpGUID (enations_latest/src/netapi.h). The
// registration server must use the SAME GUID the game registers/enumerates with, or
// the host's session is never served back to a client. Override with --guid.
// (Mirrors iserve_main.cpp's EN_GAME_GUID, kept in sync.)
static const char* EN_GAME_GUID = "F9FC4F00843C11cfB82000AA0047F4";

#include "iserve_core.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// The main thread's id, so the console control handler (which runs on its own
// thread) can post WM_QUIT to break our GetMessage loop.
static DWORD g_mainTid = 0;

static BOOL WINAPI on_ctrl(DWORD type) {
    switch (type) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            if (g_mainTid)
                PostThreadMessage(g_mainTid, WM_QUIT, 0, 0);
            return TRUE;
        default:
            return FALSE;
    }
}

static void usage(const char* argv0) {
    fprintf(stderr,
        "Usage: %s [--port N] [--proto tcp|ipx] [--guid G]\n"
        "  --port N       registration listen port (default %d; do NOT use 2346 — that's the game-session port)\n"
        "  --proto tcp    TCP/IP (default) | ipx\n"
        "  --guid G       game GUID to serve (default = Enemy Nations; must match the game)\n",
        argv0, DEF_IPX_PORT);
}

int main(int argc, char** argv) {
    int         port  = DEF_IPX_PORT;   // 1707 — the registration port, NOT 2346
    unsigned    proto = VPT_TCP;
    const char* guid  = EN_GAME_GUID;   // must match the game's GUID; override with --guid

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--port") && i + 1 < argc) {
            port = atoi(argv[++i]);
            if (port <= 0 || port > 65535) { fprintf(stderr, "[iserve] bad --port\n"); return 2; }
        } else if (!strcmp(argv[i], "--proto") && i + 1 < argc) {
            const char* p = argv[++i];
            if      (!strcmp(p, "tcp")) proto = VPT_TCP;
            else if (!strcmp(p, "ipx")) proto = VPT_IPX;
            else { fprintf(stderr, "[iserve] bad --proto\n"); return 2; }
        } else if (!strcmp(argv[i], "--guid") && i + 1 < argc) {
            guid = argv[++i];
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]); return 0;
        } else {
            fprintf(stderr, "[iserve] unknown arg: %s\n", argv[i]); usage(argv[0]); return 2;
        }
    }

    setvbuf(stdout, nullptr, _IONBF, 0);   // unbuffered so diagnostics survive an abnormal exit
    setvbuf(stderr, nullptr, _IONBF, 0);

    g_mainTid = GetCurrentThreadId();
    SetConsoleCtrlHandler(on_ctrl, TRUE);

    // CIserveCore::Start → vpStartup (which, on Windows, runs InitWindowsStuff and
    // creates the engine's hidden message window on THIS thread) → vpStartRegistrationServer.
    CIserveCore core;
    if (!core.Start(proto, port, guid)) {
        fprintf(stderr, "[iserve] failed to start registration server\n");
        return 1;
    }

    printf("[iserve] registration server up: proto=%s port=%d addr=%s\n",
           proto == VPT_TCP ? "TCP" : "IPX", port,
           core.Address().empty() ? "(unknown)" : core.Address().c_str());
    printf("[iserve] waiting for host registrations / client queries - Ctrl-C to stop\n");

    // Win32 service pump: dispatch the engine's hidden-window messages (WM_WINSOCK from
    // WSAAsyncSelect + the 250ms WM_TIMER). The window was created on this thread by
    // vpStartup, so GetMessage on this thread receives them. WM_QUIT (posted by the
    // console ctrl handler) makes GetMessage return 0 and ends the loop.
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    printf("\n[iserve] shutting down\n");
    core.Stop();
    return 0;
}
