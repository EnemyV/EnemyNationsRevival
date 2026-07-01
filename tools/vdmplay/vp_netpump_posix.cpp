//---------------------------------------------------------------------------
// vp_netpump_posix.cpp — select()-based replacement for WSAAsyncSelect.
// MP POSIX port, step 5. See vp_netpump_posix.h for the rationale.
//
// Edge emulation: Win32 WSAAsyncSelect delivers EDGE-triggered FD_* notifications
// (FD_CONNECT once on completion, FD_WRITE once when sendable, etc.). select() is
// LEVEL-triggered, so we keep a tiny per-socket state machine:
//   - connecting:  set while FD_CONNECT is pending; the first writable/error edge
//                  resolves it (getsockopt SO_ERROR) and fires FD_CONNECT once.
//   - writeArmed:  set when FD_WRITE is (re)requested; cleared after one FD_WRITE,
//                  so a perpetually-writable socket is NOT selected-for-write every
//                  pump (which would busy-loop). The engine re-arms when it has
//                  more to send (it re-calls ConfigureSocket -> vpNetSelectSet).
//   - FD_READ vs FD_CLOSE: a readable connected socket is MSG_PEEK'd — 0 bytes
//                  (or a hard error) => FD_CLOSE, >0 => FD_READ.
//---------------------------------------------------------------------------
#ifndef _WIN32

#include "vp_sock_posix.h"
#include "vp_netpump_posix.h"

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <map>
#include <vector>
#include <mutex>

namespace {

struct Reg {
    vpSelectDispatchFn fn   = nullptr;
    void*              ctx  = nullptr;
    void*              hWnd = nullptr;
    long               mask = 0;
    bool               connecting = false;  // FD_CONNECT pending
    bool               writeArmed = false;  // fire one FD_WRITE on next writable
};

std::map<SOCKET, Reg> g_reg;
std::mutex            g_mtx;

// One edge-emulated event to dispatch after the lock is dropped.
struct Pending {
    vpSelectDispatchFn fn;
    void*              ctx;
    void*              hWnd;
    SOCKET             s;
    long               lParam;   // MAKELONG(FD_event, error)
};

} // namespace

void vpNetSelectSet(SOCKET s, vpSelectDispatchFn fn, void* ctx, void* hWnd, long mask)
{
    std::lock_guard<std::mutex> lk(g_mtx);
    if (mask == 0) {                 // WSAAsyncSelect(s,...,0) cancels notifications
        g_reg.erase(s);
        return;
    }
    Reg& r = g_reg[s];
    r.fn   = fn;
    r.ctx  = ctx;
    r.hWnd = hWnd;
    r.mask = mask;
    if (mask & FD_CONNECT)
        r.connecting = true;         // resolve on the first writable/error edge
    if (mask & FD_WRITE && !r.connecting)
        r.writeArmed = true;         // one FD_WRITE once it's sendable
}

void vpNetSelectClear(SOCKET s)
{
    std::lock_guard<std::mutex> lk(g_mtx);
    g_reg.erase(s);
}

// Purge every registration whose dispatch ctx is the given object. Called from
// ~CNetInterface: teardown paths exist that delete the net object while one of
// its sockets is still registered (seen live: game-start error -> vpCleanup ->
// next pump dispatched FD_READ into the freed interface). Socket-by-socket
// closesocket() cleanup covers the normal paths; this backstops the leaks.
void vpNetSelectClearCtx(void* ctx)
{
    std::lock_guard<std::mutex> lk(g_mtx);
    for (auto it = g_reg.begin(); it != g_reg.end(); )
        it = (it->second.ctx == ctx) ? g_reg.erase(it) : std::next(it);
}

int vpNetPumpPosix(int timeout_ms)
{
    fd_set rfds, wfds, efds;
    FD_ZERO(&rfds); FD_ZERO(&wfds); FD_ZERO(&efds);
    int maxfd = -1;

    {
        std::lock_guard<std::mutex> lk(g_mtx);
        if (g_reg.empty())
            return 0;
        for (auto& kv : g_reg) {
            SOCKET s = kv.first;
            const Reg& r = kv.second;
            bool inSet = false;
            if (r.mask & (FD_READ | FD_ACCEPT | FD_CLOSE) || r.connecting) {
                FD_SET(s, &rfds); inSet = true;
            }
            // Only select-for-write when there is a reason to (connect pending or
            // an armed FD_WRITE) — otherwise an idle writable socket spins.
            if (r.connecting || (r.mask & FD_WRITE && r.writeArmed)) {
                FD_SET(s, &wfds); inSet = true;
            }
            if (inSet) {
                FD_SET(s, &efds);
                if ((int)s > maxfd) maxfd = (int)s;
            }
        }
    }

    if (maxfd < 0)
        return 0;

    timeval tv;
    timeval* ptv = nullptr;
    if (timeout_ms >= 0) {
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        ptv = &tv;
    }

    int n = select(maxfd + 1, &rfds, &wfds, &efds, ptv);
    if (n <= 0)
        return 0;   // timeout or EINTR — nothing to dispatch

    std::vector<Pending> out;
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        for (auto& kv : g_reg) {
            SOCKET s = kv.first;
            Reg& r = kv.second;
            bool isR = FD_ISSET(s, &rfds);
            bool isW = FD_ISSET(s, &wfds);
            bool isE = FD_ISSET(s, &efds);
            if (!isR && !isW && !isE)
                continue;

            auto emit = [&](long ev, long err) {
                out.push_back(Pending{ r.fn, r.ctx, r.hWnd, s,
                                       (long)MAKELONG((unsigned short)ev,
                                                      (unsigned short)err) });
            };

            // Resolve a pending connect first (writable or error edge).
            if (r.connecting && (isW || isE)) {
                int soerr = 0; socklen_t sl = sizeof(soerr);
                getsockopt(s, SOL_SOCKET, SO_ERROR, &soerr, &sl);
                emit(FD_CONNECT, soerr);
                r.connecting = false;
                if (soerr == 0) r.writeArmed = true;   // sendable after connect
                continue;   // hand off; read/write handled on the next pump
            }

            if (isE) {
                // Drain the pending socket error in BOTH cases so the exception can't
                // re-fire on the next pump (busy-loop / spurious late close).
                int soerr = 0; socklen_t sl = sizeof(soerr);
                getsockopt(s, SOL_SOCKET, SO_ERROR, &soerr, &sl);

                // Only treat exceptfds as a CLOSE when there's nothing readable. Linux
                // reports a UDP socket's async error (e.g. ICMP port-unreachable from a
                // prior send to a closed port) in exceptfds *while a valid datagram is
                // still queued readable*; macOS/BSD do not. The old code emitted FD_CLOSE
                // and `continue`d unconditionally, so the `continue` skipped the FD_READ
                // dispatch below -> the iserve registration datagram was consumed in this
                // test and never reached OnUnsafeData -> "serving 0" on Linux ONLY (mac
                // received fine). A datagram socket has no connection to close anyway.
                // When readable, fall through to the readability test, which emits FD_READ
                // (data) — a genuine peer close still surfaces there as MSG_PEEK == 0.
                if (!isR) {
                    emit(FD_CLOSE, soerr);
                    // Edge-emulate FD_CLOSE like FD_WRITE (fire ONCE): a closed peer
                    // stays "readable" forever under level-triggered select(), so
                    // without this the pump re-emits FD_CLOSE every tick with zero
                    // backoff (measured 1.3M dispatches/2min on a failed join),
                    // starving the SDL event loop — the window looks frozen/vanished.
                    r.mask &= ~(FD_READ | FD_CLOSE);
                    continue;
                }
            }

            if (isR) {
                if (r.mask & FD_ACCEPT) {
                    emit(FD_ACCEPT, 0);
                } else {
                    char b;
                    ssize_t pk = recv(s, &b, 1, MSG_PEEK);
                    if (pk == 0) {
                        emit(FD_CLOSE, 0);
                        r.mask &= ~(FD_READ | FD_CLOSE);   // edge: FD_CLOSE fires once
                    } else if (pk > 0) {
                        if (r.mask & FD_READ) emit(FD_READ, 0);
                    } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
                        emit(FD_CLOSE, errno);
                        r.mask &= ~(FD_READ | FD_CLOSE);   // edge: FD_CLOSE fires once
                    }
                }
            }

            if (isW && (r.mask & FD_WRITE) && r.writeArmed) {
                emit(FD_WRITE, 0);
                r.writeArmed = false;   // edge: don't re-fire until re-armed
            }
        }
    }

    // Dispatch outside the lock — handlers may re-enter vpNetSelectSet/Clear.
    // A handler may also close ANOTHER pending socket and destroy its ctx (the
    // join browser's enum poll: a SenumREP handler triggers CNetApi::Close ->
    // vpCleanup deletes the CNetInterface while its TCP-enum socket still has a
    // queued event in `out`), so re-validate each entry against the registry
    // right before firing and drop ones whose registration vanished or changed
    // — dispatching a stale entry is a use-after-free in vpTcpSelectThunk.
    int fired = 0;
    for (const Pending& p : out) {
        {
            std::lock_guard<std::mutex> lk(g_mtx);
            auto it = g_reg.find(p.s);
            if (it == g_reg.end() || it->second.ctx != p.ctx || it->second.fn != p.fn)
                continue;   // unregistered (or re-registered) by an earlier handler
        }
        p.fn(p.ctx, p.s, p.hWnd, p.lParam);
        ++fired;
    }

    return fired;
}

//---------------------------------------------------------------------------
// Loopback self-test
//---------------------------------------------------------------------------
namespace {

struct TestState {
    // last event seen per socket (FD_*), plus role tags
    SOCKET lis = (SOCKET)-1, cli = (SOCKET)-1, srv = (SOCKET)-1;
    bool accepted = false, connected = false, gotRead = false, gotClose = false;
    bool cliSent = false;
};

long testDispatch(void* ctx, SOCKET s, void* /*hWnd*/, long lParam)
{
    TestState* t = (TestState*)ctx;
    long ev = WSAGETSELECTEVENT(lParam);

    if (s == t->lis && ev == FD_ACCEPT) {
        sockaddr_in a; socklen_t al = sizeof(a);
        SOCKET srv = accept(t->lis, (sockaddr*)&a, &al);
        if (srv != (SOCKET)-1) {
            int fl = 1; ioctl(srv, FIONBIO, &fl);
            t->srv = srv;
            t->accepted = true;
            vpNetSelectSet(srv, testDispatch, ctx, nullptr, FD_READ | FD_CLOSE);
        }
    } else if (s == t->cli && ev == FD_CONNECT) {
        t->connected = true;
    } else if (s == t->srv && ev == FD_READ) {
        char buf[64];
        recv(t->srv, buf, sizeof(buf), 0);   // drain so FD_READ doesn't re-fire
        t->gotRead = true;
        // Close the server end to provoke FD_CLOSE on the client.
        vpNetSelectClear(t->srv);
        close(t->srv);
        t->srv = (SOCKET)-1;
    } else if (s == t->cli && ev == FD_CLOSE) {
        t->gotClose = true;
    }
    return 0;
}

} // namespace

int vpNetPumpSelfTest()
{
    TestState t;

    // 1. Listener on 127.0.0.1:<ephemeral>.
    t.lis = socket(AF_INET, SOCK_STREAM, 0);
    if (t.lis == (SOCKET)-1) return 1;
    int one = 1; setsockopt(t.lis, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(t.lis, (sockaddr*)&addr, sizeof(addr)) != 0) return 2;
    if (listen(t.lis, 4) != 0) return 3;
    socklen_t al = sizeof(addr);
    if (getsockname(t.lis, (sockaddr*)&addr, &al) != 0) return 4;
    int fl = 1; ioctl(t.lis, FIONBIO, &fl);
    vpNetSelectSet(t.lis, testDispatch, &t, nullptr, FD_ACCEPT);

    // 2. Non-blocking client connect.
    t.cli = socket(AF_INET, SOCK_STREAM, 0);
    if (t.cli == (SOCKET)-1) return 5;
    ioctl(t.cli, FIONBIO, &fl);
    int cr = connect(t.cli, (sockaddr*)&addr, sizeof(addr));
    if (cr != 0 && errno != EINPROGRESS) return 6;
    vpNetSelectSet(t.cli, testDispatch, &t, nullptr,
                   FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);

    // 3. Drive the pump: accept -> connect -> client sends -> server reads+closes
    //    -> client sees close. Bounded so a regression can't hang the build.
    for (int i = 0; i < 400 && !(t.accepted && t.connected && t.gotRead && t.gotClose); ++i) {
        vpNetPumpPosix(10);
        if (t.connected && !t.cliSent) {
            send(t.cli, "hi", 2, 0);
            t.cliSent = true;
        }
    }

    int rc = 0;
    if (!t.accepted)  rc = 10;
    else if (!t.connected) rc = 11;
    else if (!t.gotRead)   rc = 12;
    else if (!t.gotClose)  rc = 13;

    fprintf(stderr,
            "[vpNetPumpSelfTest] accept=%d connect=%d read=%d close=%d -> %s (rc=%d)\n",
            t.accepted, t.connected, t.gotRead, t.gotClose,
            rc == 0 ? "PASS" : "FAIL", rc);

    if (t.cli != (SOCKET)-1) { vpNetSelectClear(t.cli); close(t.cli); }
    if (t.srv != (SOCKET)-1) { vpNetSelectClear(t.srv); close(t.srv); }
    if (t.lis != (SOCKET)-1) { vpNetSelectClear(t.lis); close(t.lis); }
    return rc;
}

#endif // !_WIN32
