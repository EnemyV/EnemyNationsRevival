// Standalone driver for CSockets::SelfTest() — NOT part of the game build.
// Compiled directly (cl/g++) against sockets.cpp to dynamically verify the TCP
// loopback round-trip (plan P1 "unit-smoke: loopback send/recv"). Provides the two
// out-of-line CProtocol members (normally in davenet.cpp) so we don't drag in the
// whole davenet/naInit unit + its resource-string MsgBox path.
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include "stdafx.h"
#include "_davenet.h"
#include "sockets.h"
#include <cstdio>

CProtocol::~CProtocol () {}
void CProtocol::MsgBox ( int )          {}
void CProtocol::MsgBox ( char const * ) {}

int main ()
{
    BOOL ok = CSockets::SelfTest ();
    printf ( "CSockets::SelfTest -> %s\n", ok ? "PASS" : "FAIL" );
    return ( ok ? 0 : 1 );
}
