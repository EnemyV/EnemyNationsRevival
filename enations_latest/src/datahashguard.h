#ifndef DATAHASHGUARD_H
#define DATAHASHGUARD_H

//---------------------------------------------------------------------------
//
//  The two gameplay-data-hash decisions (015 area 3, phase 3).
//
//  They are one line each and they live here so both sides of the handshake
//  read from the SAME text, and so tests/data/test_data_net.cpp can compile the
//  shipped predicates instead of a copy of them.
//
//  Where they are asked, and why there:
//
//  * JoinAllowed - on the JOINER, in SDL2_RunJoinNetworkFlow, against the hash
//    the host published in its CNetPublish record, BEFORE any wire call. The
//    joiner is the only machine that holds both numbers at that moment, so it
//    is the only one that can tell the player why. The browser deliberately
//    still LISTS a mismatched game: dropping the row reproduces the
//    undiagnosable "No games found", where the player has a running host in
//    front of them and no reason.
//
//  * AcceptJoin - on the HOST, in OnMsgJoin, and this one is the AUTHORITY. The
//    joiner's check is a courtesy that a modified or older client can simply
//    not perform; the host refuses regardless. It also bounds m_iLen first,
//    because the record it is about to read arrives in a fixed-size wire buffer.
//
//  Dependency-free on purpose: plain integer types, no windows.h. Callers pass
//  DWORDs, which widen exactly.
//
//---------------------------------------------------------------------------

namespace endataguard {

//  May this client dial a game that published dwTheirs?
inline bool JoinAllowed( unsigned long dwTheirs, unsigned long dwMine ) { return dwTheirs == dwMine; }

//  May this host admit a join record of length iLen carrying dwTheirs?
inline bool AcceptJoin( int iLen, int iMinLen, unsigned long dwTheirs, unsigned long dwMine )
{
    if ( iLen < iMinLen ) return false;
    return dwTheirs == dwMine;
}

}  // namespace endataguard

#endif  // DATAHASHGUARD_H
