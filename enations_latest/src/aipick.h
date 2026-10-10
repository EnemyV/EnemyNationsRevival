#ifndef AIPICK_H
#define AIPICK_H

//---------------------------------------------------------------------------
//
//  AI tie-break picking (bug #73).
//
//  Header-only and dependency-free ON PURPOSE: caigmgr.cpp compiles it into
//  the game and tests/ai/test_ai_rsrchpick.cpp compiles the same text with
//  cl.exe alone, so the fixture exercises the shipped helper rather than a
//  copy of it.
//
//  WHY THIS EXISTS: the engine's random source is INCLUSIVE of its bound --
//  RandNum( iMax ) returns 0..iMax (windward/wind22/src/rand.cpp:55-68, the
//  final branch clamps *to* iMax), and CAIData::GetRandom is a straight
//  passthrough to it (caidata.cpp:1208-1212). So a "pick one of the n entries
//  I just filled in" draw must be taken over n-1, and the resulting index must
//  still land inside the filled prefix [0, n-1] -- indexing with a raw
//  GetRandom( n ) reads one element PAST the last written slot of a stack
//  array and hands the server an uninitialised topic id.
//
//---------------------------------------------------------------------------

namespace enaipick {

//  Maps a random draw onto a slot of an array whose first nFilled entries are
//  initialised. In range -> returned unchanged (so a correct draw picks exactly
//  the element it always picked); out of range -> clamped into the filled
//  prefix; nothing filled -> -1, which the caller must treat as "no choice".
inline int PickFilled( int nFilled, int iRandom )
{
    if ( nFilled <= 0 )
        return ( -1 );
    if ( iRandom < 0 )
        return ( 0 );
    if ( iRandom >= nFilled )
        return ( nFilled - 1 );
    return ( iRandom );
}

}  // namespace enaipick

#endif  // AIPICK_H
