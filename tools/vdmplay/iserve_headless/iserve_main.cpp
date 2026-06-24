// iserve_main.cpp — headless CLI entry for the VDMPLAY registration server.
// Replaces the MFC CWinApp/DoModal entry (ISERVE.CPP) with a plain daemon:
//   iserve [--port N] [--proto tcp|ipx]
// Listens for host registrations + answers client SenumREQ, logging to stdout.
// SIGINT (Ctrl-C) shuts down cleanly.

#include "win32_compat.h"
#include "vdmplay.h"        // VPT_TCP / VPT_IPX
#include "base.h"           // DEF_IPX_PORT (1707)

#include "iserve_core.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <ctime>

static volatile sig_atomic_t g_run = 1;
static void on_sigint(int) { g_run = 0; }

static void usage(const char* argv0) {
    fprintf(stderr,
        "Usage: %s [--port N] [--proto tcp|ipx]\n"
        "  --port N       registration listen port (default %d; do NOT use 2346 — that's the game-session port)\n"
        "  --proto tcp    TCP/IP (default) | ipx\n",
        argv0, DEF_IPX_PORT);
}

int main(int argc, char** argv) {
    int      port  = DEF_IPX_PORT;   // 1707 — the registration port, NOT 2346
    unsigned proto = VPT_TCP;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--port") && i + 1 < argc) {
            port = atoi(argv[++i]);
            if (port <= 0 || port > 65535) { fprintf(stderr, "[iserve] bad --port\n"); return 2; }
        } else if (!strcmp(argv[i], "--proto") && i + 1 < argc) {
            const char* p = argv[++i];
            if      (!strcmp(p, "tcp")) proto = VPT_TCP;
            else if (!strcmp(p, "ipx")) proto = VPT_IPX;
            else { fprintf(stderr, "[iserve] bad --proto\n"); return 2; }
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]); return 0;
        } else {
            fprintf(stderr, "[iserve] unknown arg: %s\n", argv[i]); usage(argv[0]); return 2;
        }
    }

    setvbuf(stdout, nullptr, _IONBF, 0);   // unbuffered so diagnostics survive an abnormal exit
    setvbuf(stderr, nullptr, _IONBF, 0);

    signal(SIGINT,  on_sigint);
    signal(SIGTERM, on_sigint);

    CIserveCore core;
    if (!core.Start(proto, port)) {
        fprintf(stderr, "[iserve] failed to start registration server\n");
        return 1;
    }

    printf("[iserve] registration server up: proto=%s port=%d addr=%s\n",
           proto == VPT_TCP ? "TCP" : "IPX", port,
           core.Address().empty() ? "(unknown)" : core.Address().c_str());
    printf("[iserve] waiting for host registrations / client queries — Ctrl-C to stop\n");
    fflush(stdout);

    while (g_run) {
        core.Pump(200);   // 200ms select() block; returns when a socket is ready or on timeout
    }

    printf("\n[iserve] shutting down\n");
    core.Stop();
    return 0;
}
