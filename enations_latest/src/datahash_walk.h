#ifndef DATAHASH_WALK_H
#define DATAHASH_WALK_H

//---------------------------------------------------------------------------
//
//  The byte-level half of the gameplay data hash (015 area 3, phase 3).
//
//  Header-only and dependency-free ON PURPOSE: datahash.cpp compiles it into
//  the game and tests/data/test_data_hash.cpp compiles the same text with
//  cl.exe alone, so the fixture exercises the shipped walker rather than a
//  copy of it.
//
//  Everything here works on RAW BYTES - never a parsed struct - so Windows,
//  mac and linux produce the same DWORD for the same data.
//
//---------------------------------------------------------------------------

#include <stddef.h>
#include <string.h>

namespace endatahash {

//  FNV-1a, 32 bit. `unsigned long` is 32 bit on Win32/Win64 and 64 bit on
//  LP64, so every step masks back to 32 bits and the result is the same
//  number on all three platforms.
const unsigned long kFnvOffset = 2166136261UL;
const unsigned long kFnvPrime  = 16777619UL;
const unsigned long kMask32    = 0xffffffffUL;

inline void FnvByte( unsigned long& h, unsigned char b )
{
    h = ( ( h ^ (unsigned long)b ) * kFnvPrime ) & kMask32;
}

inline void FnvBytes( unsigned long& h, const unsigned char* p, size_t cb )
{
    for ( size_t i = 0; i < cb; i++ ) FnvByte( h, p[i] );
}

//  A 32-bit value fed in little-endian order, so the hash does not depend on
//  the host's byte order.
inline void FnvU32LE( unsigned long& h, unsigned long v )
{
    FnvByte( h, (unsigned char)( v & 0xff ) );
    FnvByte( h, (unsigned char)( ( v >> 8 ) & 0xff ) );
    FnvByte( h, (unsigned char)( ( v >> 16 ) & 0xff ) );
    FnvByte( h, (unsigned char)( ( v >> 24 ) & 0xff ) );
}

//  A little-endian 32-bit field read out of a RIFF header. Byte-wise, so it is
//  alignment- and endian-independent.
inline unsigned long ReadLE32( const unsigned char* p )
{
    return ( (unsigned long)p[0] ) | ( (unsigned long)p[1] << 8 ) | ( (unsigned long)p[2] << 16 ) |
           ( (unsigned long)p[3] << 24 );
}

inline bool IsName( const unsigned char* p4, const char* pName ) { return memcmp( p4, pName, 4 ) == 0; }

inline bool IsWanted( const unsigned char* p4, const char* const* ppNames, int cNames )
{
    for ( int i = 0; i < cNames; i++ )
        if ( IsName( p4, ppNames[i] ) ) return true;
    return false;
}

//  An element of a RIFF's top level, as the walk sees it.
struct RiffElement
{
    const unsigned char* pName;     //  the 4 characters that NAME it (see below)
    const unsigned char* pPayload;  //  first payload byte
    unsigned long        cbPayload; //  payload length as stored in the header
};

//  Walks the top level of a RIFF image and calls back for every element.
//
//  An element is NAMED by its chunk id, except a LIST (or a nested RIFF), which
//  is named by its form type - that is how the gameplay tables are actually
//  stored: 9.rif's text lists and create.rif's RACE list are LIST chunks, not
//  plain chunks (racedata.cpp:36, research.cpp:231).
//
//  Returns false for anything that is not a well-formed RIFF: bad magic, a
//  header running off the end, a payload longer than the image. The walk is
//  side-effect free, which is what lets HashNamedChunks below validate the
//  whole image BEFORE it feeds a single byte into the hash.
template <class TSink>
inline bool WalkRiffTopLevel( const unsigned char* pData, size_t cb, TSink& sink )
{
    if ( pData == NULL || cb < 12 ) return false;
    if ( !IsName( pData, "RIFF" ) ) return false;

    //  The RIFF's own size field bounds the walk, so trailing bytes (the
    //  container is a concatenation, so this matters) are not walked into.
    size_t              cbEnd  = cb;
    const unsigned long riffSz = ReadLE32( pData + 4 );
    //  cb >= 12, so cb - 8 cannot underflow, and the compare keeps riffSz + 8
    //  inside size_t even on a 32-bit build.
    if ( riffSz < (unsigned long)( cb - 8 ) ) cbEnd = (size_t)riffSz + 8;
    if ( cbEnd < 12 ) return false;

    size_t pos = 12;  // past "RIFF", the size, and the form type
    while ( pos + 8 <= cbEnd )
    {
        const unsigned char* pId     = pData + pos;
        const unsigned long  cbChunk = ReadLE32( pData + pos + 4 );
        const size_t         payload = pos + 8;

        if ( cbChunk > cbEnd - payload ) return false;  // truncated element

        RiffElement el;
        el.pName     = pId;
        el.pPayload  = pData + payload;
        el.cbPayload = cbChunk;
        if ( IsName( pId, "LIST" ) || IsName( pId, "RIFF" ) )
        {
            if ( cbChunk < 4 ) return false;
            el.pName = el.pPayload;  // the form type names the list
        }
        sink( el );

        //  RIFF pads every element to an even length; the pad byte is not
        //  payload and is never hashed.
        size_t next = payload + (size_t)cbChunk;
        if ( cbChunk & 1 ) next++;
        if ( next <= pos ) return false;  // no forward progress: malformed
        pos = next;
    }
    return true;
}

namespace detail {

struct NullSink
{
    void operator( )( const RiffElement& ) { }
};

struct HashSink
{
    unsigned long*     pH;
    const char* const* ppNames;
    int                cNames;
    void               operator( )( const RiffElement& el )
    {
        if ( !IsWanted( el.pName, ppNames, cNames ) ) return;
        FnvBytes( *pH, el.pName, 4 );
        FnvU32LE( *pH, el.cbPayload );
        FnvBytes( *pH, el.pPayload, (size_t)el.cbPayload );
    }
};

}  // namespace detail

//  Feeds only the NAMED top-level elements of a RIFF image into the hash, IN
//  FILE ORDER: the 4 name characters, the payload length as a little-endian
//  32-bit value, then the payload bytes exactly as they sit on disk.
//
//  Two consequences, both deliberate (WinAstra spec refinement (i)): moving a
//  named element past UNNAMED neighbours does not change the hash, because
//  unnamed elements contribute nothing; reordering named elements AMONG
//  THEMSELVES does change it, because they are fed in file order.
//
//  Validates first and feeds second, so a malformed image leaves the hash
//  untouched and the caller can substitute its own marker.
inline bool HashNamedChunks( unsigned long& h, const unsigned char* pData, size_t cb, const char* const* ppNames,
                             int cNames )
{
    detail::NullSink check;
    if ( !WalkRiffTopLevel( pData, cb, check ) ) return false;

    detail::HashSink feed;
    feed.pH      = &h;
    feed.ppNames = ppNames;
    feed.cNames  = cNames;
    return WalkRiffTopLevel( pData, cb, feed );
}

//  Fed in place of a resource that could not be read or could not be parsed.
//  Deterministic, and it cannot collide with a readable resource's bytes
//  because the framing byte in front of it says which list member it is.
inline void HashMiss( unsigned long& h ) { FnvBytes( h, (const unsigned char*)"MISS", 4 ); }

//  Feeds ONE member of the gameplay set.
//
//  iIndex is the member's position in the fixed list and goes in first, so
//  moving bytes from one member to the next cannot leave the hash unchanged.
//  ppNames == NULL means "hash the whole image"; otherwise only the named
//  top-level elements are hashed. Returns false (having fed the miss marker)
//  when the image is absent or malformed.
inline bool HashSetMember( unsigned long& h, int iIndex, const unsigned char* pData, size_t cb,
                           const char* const* ppNames, int cNames )
{
    FnvByte( h, (unsigned char)iIndex );

    if ( pData == NULL || cb == 0 )
    {
        HashMiss( h );
        return false;
    }

    if ( ppNames == NULL || cNames == 0 )
    {
        FnvBytes( h, pData, cb );
        return true;
    }

    if ( !HashNamedChunks( h, pData, cb, ppNames, cNames ) )
    {
        HashMiss( h );
        return false;
    }
    return true;
}

}  // namespace endatahash

#endif  // DATAHASH_WALK_H
