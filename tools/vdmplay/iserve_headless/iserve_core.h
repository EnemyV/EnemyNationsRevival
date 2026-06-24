// iserve_core.h — headless, no-MFC registration-server core.
//
// Extracts the server logic that lived inside the MFC dialog CIserveDlg
// (tools/vdmplay/ISERVE/ISERVDLG.CPP) into a plain class with no CWnd/MFC
// dependency, so the same core drives both a CLI daemon (Linux + Windows) and
// — if ever wanted — a thin GUI. The legacy MFC dialog is left frozen.
//
// The actual server is tiny: vpStartup + vpStartRegistrationServer + a
// vpPumpNet() service loop. Everything else in the old 487-line dialog was GUI
// chrome (the server-list listbox, caption/icon updates). See
// docs/plans/iserve-headless-optional.md.

#ifndef ISERVE_CORE_H
#define ISERVE_CORE_H

#include <string>

class CIserveCore {
public:
    CIserveCore();
    ~CIserveCore();

    // Start the registration server. protocol = VPT_TCP (default) or VPT_IPX.
    // port is the registration listen port — HEADLESS DEFAULT 1707 (DEF_IPX_PORT),
    // deliberately NOT the legacy [TCP]WellKnownPort default of 2346 (== the
    // game-session port DEF_TCP_PORT). gameId = the VDMPLAY game GUID the server
    // keys sessions by — must match the game's GUID. Returns true on success.
    bool Start(unsigned protocol, int port, const char* gameId);

    // Service the server once: drains ready sockets (accept registrations, answer
    // SenumREQ) via the vdmplay POSIX select() pump. Call in a loop.
    // timeout_ms is the select() block time. Returns the number of events serviced.
    int  Pump(int timeout_ms);

    // Stop + clean up the registration server.
    void Stop();

    bool IsRunning() const { return m_vpH != nullptr; }
    const std::string& Address() const { return m_addr; }  // our station address (for logging)

private:
    void*       m_vpH;          // VPHANDLE from vpStartup
    unsigned    m_protocol;
    int         m_port;
    std::string m_addr;
};

#endif // ISERVE_CORE_H
