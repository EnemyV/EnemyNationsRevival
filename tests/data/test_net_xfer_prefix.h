// Stands in for the game's stdafx.h when run-data-tests.ps1 compiles the SHIPPED
// enations_latest/src/dxfer.cpp and vpxfer.cpp (the load-join save transfer).
// The runner copies dxfer.h / vpxfer.h / dxfer.cpp / vpxfer.cpp into its scratch
// directory with their `#include "stdafx.h"` pointed here, and nothing else
// changed. vpSendData is the only vdmplay call they make; it is recorded so the
// test can deliver each packet to the other side by hand.
#ifndef TEST_NET_XFER_PREFIX_H
#define TEST_NET_XFER_PREFIX_H

#include <windows.h>
#include <deque>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifndef GetCurrentTime
#define GetCurrentTime( ) GetTickCount( )
#endif
#define TRACE( ... ) ( (void)0 )

typedef void* VPSESSIONHANDLE;
typedef WORD  VPPLAYERID;

// vdmplay.h (VP_TIMESTAMP 0)
typedef struct VPMsgHdr
{
    WORD       msgSize;
    BYTE       msgKind;
    BYTE       msgFlags;
    VPPLAYERID msgFrom;
    VPPLAYERID msgTo;
    WORD       msgId;
} VPMSGHDR, *LPVPMSGHDR;
const DWORD VP_MAXSENDDATA = 700;
enum { VP_READDATA = 1, VP_LEAVE, VP_SESSIONCLOSE };
enum { VP_MUSTDELIVER = 2 };

struct VPPlayerInfo
{
    VPPLAYERID playerId;
};

typedef struct VPMessage
{
    DWORD           notificationCode;
    VPSESSIONHANDLE session;
    LPVOID          userData;
    VPPLAYERID      senderId;
    VPPLAYERID      toId;
    DWORD           dataLen;
    DWORD           itemCount;
    union {
        const VPPlayerInfo* playerInfo;
        LPVOID              data;
    } u;
} VPMESSAGE, *LPVPMESSAGE;

struct XferPacket
{
    VPPLAYERID        from;
    VPPLAYERID        to;
    std::vector<char> bytes;   // VPMSGHDR + payload, as vpSendData got it
};
static std::deque<XferPacket> g_xferWire;

inline BOOL vpSendData( VPSESSIONHANDLE, VPPLAYERID to, VPPLAYERID from, LPCVOID data, DWORD dataLen, DWORD,
                        LPCVOID )
{
    XferPacket pkt;
    pkt.from = from;
    pkt.to   = to;
    pkt.bytes.assign( (const char*)data, (const char*)data + dataLen );
    g_xferWire.push_back( pkt );
    return TRUE;
}

#endif
