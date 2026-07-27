#ifndef __MMIO_INL__
#define __MMIO_INL__

#include "mmio.h"

//---------------------------------------------------------------------------
//
// File:   mmio.inl
// Programmer:  Dave Thielen 
// Create Date: 
// Last Mod Date: 02/29/96
// Description: error code constants for CMmio 
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------


_RELEASE_INLINE FourCC::FourCC( void ) {
    Copy( "    " );
}

_RELEASE_INLINE FourCC::FourCC( const FOURCC& cc ) {
    Copy( cc );
}

_RELEASE_INLINE FourCC::FourCC( const FourCC* cc ) {
    Copy( cc );
}

_RELEASE_INLINE FourCC::FourCC( char a, char b, char c, char d ) {
    four_cc = mmioFOURCC( a, b, c, d );
}

_RELEASE_INLINE FourCC::FourCC( const char* s ) {
    Copy( s );
}

_RELEASE_INLINE void FourCC::Copy( const FOURCC& cc ) {
    four_cc = cc;
}

_RELEASE_INLINE void FourCC::Copy( const FourCC& cc ) {
    four_cc = cc.four_cc;
}

_RELEASE_INLINE void FourCC::Copy( const char* s ) {
    ASSERT( s );
    ASSERT( strlen( s ) == sizeof( FOURCC ) );
    four_cc = mmioFOURCC( s[0], s[1], s[2], s[3] );
}

_RELEASE_INLINE FourCC& FourCC::operator = ( const FOURCC& cc ) {
    Copy( cc );
    return *this;
}

_RELEASE_INLINE FourCC& FourCC::operator = ( const FourCC& cc ) {
    Copy( cc );
    return *this;
}

_RELEASE_INLINE FourCC& FourCC::operator = ( const char* s ) {
    Copy( s );
    return *this;
}

_RELEASE_INLINE BOOL FourCC::operator == ( const FourCC& cc ) const {
    return four_cc == cc.four_cc;
}

_RELEASE_INLINE FourCC::operator FOURCC() const {
    return four_cc;
}


///////////////////////////////////////////////////////////////////
//

_RELEASE_INLINE DWORD CMmio::DescendRiff( const FourCC& cc ) {
    ASSERT_STRICT_VALID( this );
    return DescendRiff( (FOURCC)cc );
}

_RELEASE_INLINE DWORD CMmio::DescendList( const FourCC& cc ) {
    ASSERT_STRICT_VALID( this );
    return DescendList( (FOURCC)cc );
}

_RELEASE_INLINE DWORD CMmio::DescendChunk( const FourCC& cc ) {
    ASSERT_STRICT_VALID( this );
    return DescendChunk( (FOURCC)cc );
}

_RELEASE_INLINE void CMmio::Read( void* pBuf, LONG lNum ) {

#ifdef _DEBUG
    ASSERT( m_hMmio != NULL );
    ASSERT( (DWORD)lNum <= m_mckiChunk.cksize );
    ASSERT_VALID( this );
    memset( pBuf, 0, lNum );
#endif

    if ( mmioRead( m_hMmio, (char*)pBuf, lNum ) != lNum )
        throw ( ERR_MMIO_READ );
}

_RELEASE_INLINE short int CMmio::ReadShort() {
    short int iRtn;

    Read( &iRtn, 2 );
    return ( iRtn );
}

_RELEASE_INLINE int CMmio::ReadInt() {
    short int iRtn;

    Read( &iRtn, 2 );
    return ( iRtn );
}

_RELEASE_INLINE long CMmio::ReadLong() {
#ifdef _WIN32
    long lRtn;            // LLP64: long is 4 bytes, matches the on-disk width.

    Read( &lRtn, 4 );
    return ( lRtn );
#else
    // LP64: `long` is 8 bytes but the on-disk value is 4. Reading 4 bytes into an
    // 8-byte local leaves the upper word as stack garbage (the DEBUG memset only
    // clears the 4 bytes it's told to), producing a huge bogus value. Read the
    // 4-byte on-disk word into a fixed-width int and sign-extend.
    int32_t lRtn = 0;

    Read( &lRtn, 4 );
    return ( (long)lRtn );
#endif
}

_RELEASE_INLINE float CMmio::ReadFloat() {
    float fRtn;

    Read( &fRtn, 4 );
    return ( fRtn );
}

_RELEASE_INLINE void CMmio::ReadString( CString& sRtn ) {

    short int iLen;
    Read( &iLen, 2 );

    Read( sRtn.GetBufferSetLength( iLen ), iLen );
    sRtn.ReleaseBuffer( -1 );
}

_RELEASE_INLINE void CMmio::ReadString( std::string& sRtn ) {

    short int iLen;
    Read( &iLen, 2 );

    sRtn.resize( iLen );
    if ( iLen > 0 )
        Read( &sRtn[0], iLen );

    // The CString overload above calls `ReleaseBuffer(-1)`, which scans for an
    // embedded null and trims length there. Data files written by the legacy
    // game store strings as length-prefixed C-strings whose length includes a
    // trailing '\0'; without this trim, comparisons against literal strings
    // (e.g. version-check "Enemy Nations") fail by one byte.
    size_t nullPos = sRtn.find( '\0' );
    if ( nullPos != std::string::npos )
        sRtn.resize( nullPos );
}

_RELEASE_INLINE const MMCKINFO& CMmio::GetRiffChunkInfo() const {
    ASSERT_STRICT_VALID( this );
    return m_mckiRiff;
}

_RELEASE_INLINE const MMCKINFO& CMmio::GetListChunkInfo( int i ) const {
    ASSERT_STRICT_VALID( this );
    ASSERT( i < m_iListDepth );
    return m_mckiList[i];
}

_RELEASE_INLINE const MMCKINFO& CMmio::GetChunkInfo() const {
    ASSERT_STRICT_VALID( this );
    return m_mckiChunk;
}

_RELEASE_INLINE CMmio& CMmio::operator >> ( SHORT& s ) {
    ASSERT_STRICT_VALID( this );
    Read( &s, sizeof( SHORT ) );
    return *this;
}

_RELEASE_INLINE CMmio& CMmio::operator >> ( USHORT& s ) {
    ASSERT_STRICT_VALID( this );
    Read( &s, sizeof( USHORT ) );
    return *this;
}

#ifdef _WIN32
_RELEASE_INLINE CMmio& CMmio::operator >> ( LONG& l ) {
    ASSERT_STRICT_VALID( this );
    Read( &l, sizeof( LONG ) );
    return *this;
}

_RELEASE_INLINE CMmio& CMmio::operator >> ( ULONG& l ) {
    ASSERT_STRICT_VALID( this );
    Read( &l, sizeof( ULONG ) );
    return *this;
}
#else
// Linux LP64: read the 4-byte on-disk value and widen into the 64-bit long.
_RELEASE_INLINE CMmio& CMmio::operator >> ( long& l ) {
    ASSERT_STRICT_VALID( this );
    int32_t v = 0; Read( &v, sizeof( v ) ); l = v;
    return *this;
}
_RELEASE_INLINE CMmio& CMmio::operator >> ( unsigned long& l ) {
    ASSERT_STRICT_VALID( this );
    uint32_t v = 0; Read( &v, sizeof( v ) ); l = v;
    return *this;
}
#endif

_RELEASE_INLINE CMmio& CMmio::operator >> ( INT& i ) {
    ASSERT_STRICT_VALID( this );
    Read( &i, sizeof( INT ) );
    return *this;
}

_RELEASE_INLINE CMmio& CMmio::operator >> ( UINT& i ) {
    ASSERT_STRICT_VALID( this );
    Read( &i, sizeof( UINT ) );
    return *this;
}

_RELEASE_INLINE CMmio& CMmio::operator >> ( CString& s ) {
    ASSERT_STRICT_VALID( this );
    ReadString( s );
    return *this;
}

#endif
