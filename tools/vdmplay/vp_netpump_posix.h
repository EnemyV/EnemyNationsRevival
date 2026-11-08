#ifndef __VP_NETPUMP_POSIX_H__
#define __VP_NETPUMP_POSIX_H__
//---------------------------------------------------------------------------
// vp_netpump_posix.h — POSIX replacement for WSAAsyncSelect (MP port step 5).
//
// On Windows the vp* TCP engine arms async socket I/O with WSAAsyncSelect, which
// posts WM_WINSOCK messages (carrying an FD_* event) to a hidden message window;
// the window proc routes them to C<net>::OnMessage. There is no message window on
// POSIX, so instead we record each socket's requested FD_* mask here and a
// select()-based pump synthesizes the SAME FD_* dispatch by calling back into the
// engine — the existing OnMessage handlers fire unchanged.
//
// The module is deliberately decoupled from the engine's C++ classes: callers
// register a plain (fn, ctx) callback. CWinTcpNet::ConfigureSocket supplies a
// thunk that forwards to CNetInterface::OnMessage(hWnd, WM_WINSOCK, s, lParam).
// This also lets the self-test drive the pump with a mock sink.
//---------------------------------------------------------------------------
#ifndef _WIN32

#include "vp_sock_posix.h"   // SOCKET, FD_*, WSAGETSELECT*, MAKELONG

// Dispatch callback: lParam == MAKELONG(FD_event, error), matching the lParam the
// Win32 WSAAsyncSelect message would have carried. Return value is ignored.
typedef long (*vpSelectDispatchFn)(void* ctx, SOCKET s, void* hWnd, long lParam);

// Arm/re-arm/disarm a socket. mask is an OR of FD_READ/FD_WRITE/FD_CONNECT/
// FD_ACCEPT/FD_CLOSE (the same values ConfigureSocket passes to WSAAsyncSelect).
// mask == 0 deregisters the socket (mirrors WSAAsyncSelect(s,...,0)).
void vpNetSelectSet(SOCKET s, vpSelectDispatchFn fn, void* ctx, void* hWnd, long mask);

// Forget a socket entirely (call when the socket is closed).
void vpNetSelectClear(SOCKET s);

// select() all armed sockets (up to timeout_ms; 0 = poll, <0 = block) and
// dispatch the edge-emulated FD_* events. Returns the number of events dispatched.
int vpNetPumpPosix(int timeout_ms);

// Self-contained localhost loopback exercise of the registry + pump + edge
// emulation (connect/accept/read/write/close). Returns 0 on PASS, else a code.
// Exported from the .so (default visibility) so a standalone driver can run it
// as a build-gate, mirroring CSockets::SelfTest.
__attribute__((visibility("default"))) int vpNetPumpSelfTest();

#endif // !_WIN32
#endif // __VP_NETPUMP_POSIX_H__
