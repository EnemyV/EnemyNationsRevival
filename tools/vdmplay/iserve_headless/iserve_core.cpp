// iserve_core.cpp — headless registration-server core (no MFC).
// Mirrors CIserveDlg::OnStart/OnStop (ISERVDLG.CPP) without the CWnd.

#ifndef _WIN32
#include "win32_compat.h"   // DWORD/HWND/BOOL/LPCSTR + the win32 shim types (POSIX)
#else
#include <winsock2.h>       // BEFORE windows.h — base.h→stdafx.h pulls afxwin.h, which #errors
#include <windows.h>        // unless winsock2 (not the v1 winsock.h windows.h would pull) is in first.
#endif                      // real Win32 types (MSVC headless build — win32_compat.h #errors on _WIN32)
#include "vdmplay.h"        // vp* API, VPGUID, VPT_TCP/VPT_IPX, VPNETADDRESS
#ifndef _WIN32
#include "base.h"           // DEF_TCP_PORT / DEF_IPX_PORT (POSIX)
#else
// base.h → stdafx.h → <afxwin.h> drags MFC (mfc140.dll) on MSVC, defeating the
// no-MFC goal of the headless server. base.h gives us only these two port constants,
// so define them directly on Windows and skip the MFC-laden include.
static const int DEF_TCP_PORT = 2346;
static const int DEF_IPX_PORT = 1707;
#endif

#include "iserve_core.h"

#include <cstdio>
#include <cstring>

#ifndef _WIN32
// vpPumpNet is the POSIX select() service pump (vp_netpump_posix.cpp); it is an
// extern "C" entry in the vdmplay engine but not in vdmplay.h's Win-centric API.
// On Windows there is NO vpPumpNet (it's #ifndef _WIN32 in vdmplay.cpp): the engine's
// own hidden message window + the host's Win32 message loop service sockets instead.
extern "C" int vpPumpNet(int timeout_ms);
#endif

CIserveCore::CIserveCore() : m_vpH(nullptr), m_protocol(VPT_TCP), m_port(DEF_IPX_PORT) {}
CIserveCore::~CIserveCore() { Stop(); }

bool CIserveCore::Start(unsigned protocol, int port, const char* gameId) {
    if (m_vpH) return true;  // already running
    m_protocol = protocol;
    m_port     = port;

    // The registration server keys sessions by game GUID — it must match the GUID
    // the game registers/enumerates with, else the host's session is never served
    // to a client. Default = Enemy Nations' tlpGUID (netapi.h); override via --guid
    // for a different game. We do NOT touch vdmplay.ini (port/sizes are passed
    // explicitly below) so iserve can share a directory with the game without
    // clobbering its config.
    VPGUID guid;
    memset(&guid, 0, sizeof(guid));
    strncpy(guid.buf, gameId, sizeof(guid.buf) - 1);

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
#ifndef _WIN32
    return vpPumpNet(timeout_ms);
#else
    // Windows: the host's Win32 message loop pumps the engine's hidden window
    // (WSAAsyncSelect/WM_TIMER); Pump() is a no-op here (see iserve_main_win.cpp).
    (void)timeout_ms;
    return 0;
#endif
}

void CIserveCore::Stop() {
    if (!m_vpH) return;
    vpStopRegistrationServer(m_vpH);
    vpCleanup(m_vpH);
    m_vpH = nullptr;
}
