// iserve_core.cpp — headless registration-server core (no MFC).
// Mirrors CIserveDlg::OnStart/OnStop (ISERVDLG.CPP) without the CWnd.

#include "win32_compat.h"   // DWORD/HWND/BOOL/LPCSTR + the win32 shim types
#include "vdmplay.h"        // vp* API, VPGUID, VPT_TCP/VPT_IPX, VPNETADDRESS
#include "base.h"           // DEF_TCP_PORT / DEF_IPX_PORT

#include "iserve_core.h"

#include <cstdio>
#include <cstring>

// vpPumpNet is the POSIX select() service pump (vp_netpump_posix.cpp); it is an
// extern "C" entry in the vdmplay engine but not in vdmplay.h's Win-centric API.
extern "C" int vpPumpNet(int timeout_ms);

CIserveCore::CIserveCore() : m_vpH(nullptr), m_protocol(VPT_TCP), m_port(DEF_IPX_PORT) {}
CIserveCore::~CIserveCore() { Stop(); }

// Write the minimal vdmplay.ini this process needs (in the cwd, the path the
// engine reads). We set the registration port EXPLICITLY so we never inherit the
// legacy [TCP]WellKnownPort default of 2346 (== the game-session port). The
// engine reads [TCP]/[IPX] WellKnownPort + [ISERVE] GUID/InfoSizes from here.
static void WriteIni(unsigned protocol, int port) {
    FILE* f = fopen("vdmplay.ini", "w");
    if (!f) return;
    fprintf(f, "[TCP]\nWellKnownPort=%d\n\n", protocol == VPT_TCP ? port : DEF_TCP_PORT);
    fprintf(f, "[IPX]\nWellKnownPort=%d\n\n", protocol == VPT_IPX ? port : DEF_IPX_PORT);
    fprintf(f, "[ISERVE]\nGUID=TESTGAME\nSessionInfoSize=32\nPlayerInfoSize=32\n");
    fclose(f);
}

bool CIserveCore::Start(unsigned protocol, int port) {
    if (m_vpH) return true;  // already running
    m_protocol = protocol;
    m_port     = port;

    WriteIni(protocol, port);

    VPGUID guid;
    memset(&guid, 0, sizeof(guid));
    strncpy(guid.buf, "TESTGAME", sizeof(guid.buf) - 1);

    // Pass the registration port EXPLICITLY via protocolData so the engine binds it
    // directly — NOT via the legacy [TCP]WellKnownPort ini lookup, which defaults to
    // DEF_TCP_PORT (2346 = the game-session port). serverAddress=0: a registration
    // server doesn't dial out, it just listens on wellKnownPort.
    const void* protoData = nullptr;
    VPTCPDATA tcp;
    if (protocol == VPT_TCP) {
        memset(&tcp, 0, sizeof(tcp));
        tcp.serverAddress = 0;
        tcp.wellKnownPort = (unsigned short)port;
        protoData = &tcp;
    }

    // sessionDataSize / playerDataSize match the dialog's defaults (32/32).
    m_vpH = vpStartup(1, &guid, 32, 32, protocol, protoData);
    if (!m_vpH) {
        fprintf(stderr, "[iserve] vpStartup failed (TCP/IP available?)\n");
        return false;
    }

    // hWnd = 0: a headless server needs no notification window. The registration
    // server's work is socket-driven (serviced by vpPumpNet); WM_VPNOTIFY was
    // only the GUI list feed.
    if (!vpStartRegistrationServer(m_vpH, (HWND)0, nullptr)) {
        fprintf(stderr, "[iserve] vpStartRegistrationServer failed\n");
        vpCleanup(m_vpH);
        m_vpH = nullptr;
        return false;
    }

    // Record our station address for the startup banner.
    VPNETADDRESS addr;
    memset(&addr, 0, sizeof(addr));
    if (vpGetAddress(m_vpH, &addr)) {
        char buf[256] = {0};
        vpGetAddressString(m_vpH, &addr, buf, sizeof(buf));
        m_addr = buf;
    }
    return true;
}

int CIserveCore::Pump(int timeout_ms) {
    if (!m_vpH) return 0;
    return vpPumpNet(timeout_ms);
}

void CIserveCore::Stop() {
    if (!m_vpH) return;
    vpStopRegistrationServer(m_vpH);
    vpCleanup(m_vpH);
    m_vpH = nullptr;
}
