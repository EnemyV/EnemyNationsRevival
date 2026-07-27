#ifndef __VPNATCAND_H__
#define __VPNATCAND_H__

// NAT candidate extension carried in the spare tail of a VPNETADDRESS/VPSESSIONID.
//
// A VPNETADDRESS is an opaque 28-byte machineAddress[], but the TCP transport
// only uses the first 8 bytes (tcpaddress_s: 4-byte IPv4 + 2-byte stream port +
// 2-byte dg port, all network order) and IPX uses the first 14. Bytes 16..27 are
// never read by any transport or old build, so the registration server (iserve)
// stamps the host's OBSERVED public UDP endpoint there when a registration
// (SenumREP) datagram arrives — the observed source of that datagram IS the
// host's public NAT mapping for its game dg socket. The stamped sessionId then
// flows to joiners through the existing enum replies with ZERO wire-layout
// change: old clients ignore the tail, old iserves never write it.
//
// Design doc: EnemyNationsDiscussion/docs/plans/nat-holepunch-iserve-feasibility.md
//
// Tail layout (offsets within machineAddress[28], multi-byte fields NETWORK order):
//   [16] 'N'  [17] 'P'          magic
//   [18] version (1)
//   [19] flags (EN_NATCAND_F_*)
//   [20..23] public IPv4 of the host as observed by iserve
//   [24..25] public dg (UDP) port as observed by iserve
//   [26..27] reserved (0) — future observed/mapped stream port
//
// All helpers are raw byte ops on the 28-byte array: no socket headers, no
// endian conversion needed by callers (values stay network-order pass-through).

#include "vdmplay.h"    // VPNETADDRESS
#include <string.h>     // memset (raw byte ops below)
#include <stdio.h>      // snprintf (EnNatCandFmt)

#define EN_NATCAND_OFF        16
#define EN_NATCAND_VER        1

// flags
#define EN_NATCAND_F_SAMENAT  0x01  // requester's observed public IP == host's
                                    // (same NAT: dial the private candidate)

inline unsigned char* EnNatCandBytes( VPNETADDRESS* a )             { return (unsigned char*)a->machineAddress; }
inline const unsigned char* EnNatCandBytes( const VPNETADDRESS* a ) { return (const unsigned char*)a->machineAddress; }

inline int EnNatCandPresent( const VPNETADDRESS* a ) {
    const unsigned char* b = EnNatCandBytes( a );
    return b[EN_NATCAND_OFF] == 'N' && b[EN_NATCAND_OFF + 1] == 'P' &&
           b[EN_NATCAND_OFF + 2] == EN_NATCAND_VER;
}

// Stamp the observed public candidate (both values network order, as read from
// the wire/sockaddr). Zeroes flags + reserved; overwrites any prior stamp — the
// registration server is authoritative, a host cannot spoof its public mapping.
inline void EnNatCandStamp( VPNETADDRESS* a, const unsigned char ipNbo[4], const unsigned char portNbo[2] ) {
    unsigned char* b = EnNatCandBytes( a ) + EN_NATCAND_OFF;
    b[0] = 'N'; b[1] = 'P'; b[2] = EN_NATCAND_VER; b[3] = 0;
    b[4] = ipNbo[0]; b[5] = ipNbo[1]; b[6] = ipNbo[2]; b[7] = ipNbo[3];
    b[8] = portNbo[0]; b[9] = portNbo[1];
    b[10] = 0; b[11] = 0;
}

inline unsigned char EnNatCandFlags( const VPNETADDRESS* a ) {
    return EnNatCandBytes( a )[EN_NATCAND_OFF + 3];
}

inline void EnNatCandSetFlags( VPNETADDRESS* a, unsigned char f ) {
    EnNatCandBytes( a )[EN_NATCAND_OFF + 3] = f;
}

// 4 raw network-order bytes of the stamped public IP / private (transport) IP.
inline const unsigned char* EnNatCandPubIp( const VPNETADDRESS* a )  { return EnNatCandBytes( a ) + EN_NATCAND_OFF + 4; }
inline const unsigned char* EnNatCandPubDgPort( const VPNETADDRESS* a ) { return EnNatCandBytes( a ) + EN_NATCAND_OFF + 8; }
inline const unsigned char* EnNatCandPrivIp( const VPNETADDRESS* a ) { return EnNatCandBytes( a ); }

inline int EnNatCandIpEq( const unsigned char* a, const unsigned char* b ) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

inline int EnNatCandIpZero( const unsigned char* a ) {
    return !a[0] && !a[1] && !a[2] && !a[3];
}

// Build the dialable PUBLIC candidate from a stamped id: public IP + the host's
// self-claimed stream port (a router port-forward / UPnP mapping targets the
// same number — see [TCP] StreamPort pinning) + observed public dg port.
// Returns 0 (and leaves out untouched) when no stamp is present.
inline int EnNatCandPublicAddr( const VPNETADDRESS* a, VPNETADDRESS* out ) {
    if ( !EnNatCandPresent( a ) )
        return 0;
    const unsigned char* b = EnNatCandBytes( a );
    unsigned char* o = EnNatCandBytes( out );
    memset( out, 0, sizeof( *out ) );
    o[0] = b[EN_NATCAND_OFF + 4]; o[1] = b[EN_NATCAND_OFF + 5];   // public IP
    o[2] = b[EN_NATCAND_OFF + 6]; o[3] = b[EN_NATCAND_OFF + 7];
    o[4] = b[4]; o[5] = b[5];                                      // claimed stream port
    o[6] = b[EN_NATCAND_OFF + 8]; o[7] = b[EN_NATCAND_OFF + 9];   // observed public dg port
    return 1;
}

// Printable "a.b.c.d:p" of a raw ip+port byte pair, for [punch]/[natcand] logs.
inline void EnNatCandFmt( char* out, int outSz, const unsigned char* ipNbo, const unsigned char* portNbo ) {
    snprintf( out, (size_t)outSz, "%u.%u.%u.%u:%u",
              ipNbo[0], ipNbo[1], ipNbo[2], ipNbo[3],
              (unsigned)( ( portNbo[0] << 8 ) | portNbo[1] ) );
}

#endif /* __VPNATCAND_H__ */
