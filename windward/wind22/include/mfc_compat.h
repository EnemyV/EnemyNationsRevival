#ifndef __MFC_COMPAT_H__
#define __MFC_COMPAT_H__

//---------------------------------------------------------------------------
// mfc_compat.h — Drop-in replacements for MFC types we still depend on
//
// CRect/CPoint/CSize match MFC's API and inherit RECT/POINT/SIZE so Win32
// APIs accept them by pointer. CString wraps std::string with the subset
// of MFC's CString surface that the live code calls. CObject is a stub
// inheritance base for the cai*.hpp AI classes.
//
// Everything is guarded by `#ifndef __AFX_H__` so MFC's real types win
// whenever afx.h was included (current production path).
//---------------------------------------------------------------------------

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <vector>

#ifndef __AFX_H__  // Only define if MFC is not included

//------------------------------- C P o i n t --------------------------------

class CPoint : public POINT
{
public:
    CPoint() { x = 0; y = 0; }
    CPoint( int _x, int _y ) { x = _x; y = _y; }
    CPoint( POINT pt ) { x = pt.x; y = pt.y; }
    CPoint( SIZE sz ) { x = sz.cx; y = sz.cy; }
    // Extract from a packed LPARAM (CPoint(lParam) in WM_LBUTTONDOWN etc.)
    CPoint( DWORD dwPoint ) { x = (short)LOWORD( dwPoint ); y = (short)HIWORD( dwPoint ); }
    CPoint( LPARAM lParam ) { x = (short)LOWORD( lParam ); y = (short)HIWORD( lParam ); }

    void Offset( int dx, int dy ) { x += dx; y += dy; }
    BOOL operator==( POINT pt ) const { return x == pt.x && y == pt.y; }
    BOOL operator!=( POINT pt ) const { return x != pt.x || y != pt.y; }
    CPoint operator+( POINT pt ) const { return CPoint( x + pt.x, y + pt.y ); }
    CPoint operator-( POINT pt ) const { return CPoint( x - pt.x, y - pt.y ); }
    CPoint& operator+=( POINT pt ) { x += pt.x; y += pt.y; return *this; }
    CPoint& operator-=( POINT pt ) { x -= pt.x; y -= pt.y; return *this; }
};

//-------------------------------- C S i z e ---------------------------------

class CSize : public SIZE
{
public:
    CSize() { cx = 0; cy = 0; }
    CSize( int _cx, int _cy ) { cx = _cx; cy = _cy; }
    CSize( SIZE sz ) { cx = sz.cx; cy = sz.cy; }

    BOOL operator==( SIZE sz ) const { return cx == sz.cx && cy == sz.cy; }
    BOOL operator!=( SIZE sz ) const { return cx != sz.cx || cy != sz.cy; }
    CSize operator+( SIZE sz ) const { return CSize( cx + sz.cx, cy + sz.cy ); }
    CSize operator-( SIZE sz ) const { return CSize( cx - sz.cx, cy - sz.cy ); }
};

//-------------------------------- C R e c t ---------------------------------

class CRect : public RECT
{
public:
    CRect() { left = top = right = bottom = 0; }
    CRect( int l, int t, int r, int b ) { left = l; top = t; right = r; bottom = b; }
    CRect( const RECT& rc ) { left = rc.left; top = rc.top; right = rc.right; bottom = rc.bottom; }
    CRect( LPCRECT prc ) { if ( prc ) { left = prc->left; top = prc->top; right = prc->right; bottom = prc->bottom; } else { left = top = right = bottom = 0; } }
    CRect( POINT pt, SIZE sz ) { left = pt.x; top = pt.y; right = pt.x + sz.cx; bottom = pt.y + sz.cy; }
    CRect( POINT topLeft, POINT bottomRight ) { left = topLeft.x; top = topLeft.y; right = bottomRight.x; bottom = bottomRight.y; }

    // Implicit conversion to (const RECT*) — MFC has this and Win32 APIs
    // like ExtTextOut take RECT* params; without it call sites that pass
    // a CRect by value won't compile.
    operator LPCRECT() const { return (LPCRECT)this; }
    operator LPRECT()        { return (LPRECT)this; }

    int Width() const { return right - left; }
    int Height() const { return bottom - top; }

    CPoint TopLeft() const { return CPoint( left, top ); }
    CPoint BottomRight() const { return CPoint( right, bottom ); }
    CSize Size() const { return CSize( right - left, bottom - top ); }
    CPoint CenterPoint() const { return CPoint( (left + right) / 2, (top + bottom) / 2 ); }

    BOOL IsRectEmpty() const { return ::IsRectEmpty( this ); }
    BOOL IsRectNull() const { return left == 0 && top == 0 && right == 0 && bottom == 0; }
    BOOL PtInRect( POINT pt ) const { return ::PtInRect( this, pt ); }

    void SetRect( int l, int t, int r, int b ) { left = l; top = t; right = r; bottom = b; }
    void SetRectEmpty() { left = top = right = bottom = 0; }
    void NormalizeRect() {
        if ( left > right ) { int t = left; left = right; right = t; }
        if ( top > bottom ) { int t = top; top = bottom; bottom = t; }
    }

    void InflateRect( int dx, int dy ) { left -= dx; top -= dy; right += dx; bottom += dy; }
    void InflateRect( const RECT* prc ) { left -= prc->left; top -= prc->top; right += prc->right; bottom += prc->bottom; }
    void DeflateRect( int dx, int dy ) { InflateRect( -dx, -dy ); }
    void OffsetRect( int dx, int dy ) { left += dx; top += dy; right += dx; bottom += dy; }

    BOOL IntersectRect( const RECT* prc1, const RECT* prc2 ) { return ::IntersectRect( this, prc1, prc2 ); }
    BOOL UnionRect( const RECT* prc1, const RECT* prc2 ) { return ::UnionRect( this, prc1, prc2 ); }
    BOOL EqualRect( const RECT* prc ) const { return ::EqualRect( this, prc ); }

    // Operators
    CRect operator&( const RECT& rc ) const {
        CRect result;
        ::IntersectRect( &result, this, &rc );
        return result;
    }
    CRect& operator&=( const RECT& rc ) {
        ::IntersectRect( this, this, &rc );
        return *this;
    }
    CRect operator|( const RECT& rc ) const {
        CRect result;
        ::UnionRect( &result, this, &rc );
        return result;
    }
    CRect& operator|=( const RECT& rc ) {
        ::UnionRect( this, this, &rc );
        return *this;
    }
    CRect operator+( POINT pt ) const { return CRect( left + pt.x, top + pt.y, right + pt.x, bottom + pt.y ); }
    CRect operator-( POINT pt ) const { return CRect( left - pt.x, top - pt.y, right - pt.x, bottom - pt.y ); }
    CRect& operator+=( POINT pt ) { OffsetRect( pt.x, pt.y ); return *this; }
    CRect& operator-=( POINT pt ) { OffsetRect( -pt.x, -pt.y ); return *this; }
    CRect& operator+=( SIZE sz )  { OffsetRect( sz.cx, sz.cy ); return *this; }
    CRect& operator-=( SIZE sz )  { OffsetRect( -sz.cx, -sz.cy ); return *this; }
    CRect& operator+=( const RECT* prc ) { InflateRect( prc ); return *this; }

    BOOL operator==( const RECT& rc ) const { return ::EqualRect( this, &rc ); }
    BOOL operator!=( const RECT& rc ) const { return !::EqualRect( this, &rc ); }
};

//------------------------------ C S t r i n g -------------------------------
// Drop-in replacement for MFC's CString, backed by std::string. Covers the
// surface the live code actually calls: ctor variants, Empty/IsEmpty/
// GetLength, GetAt/SetAt, operator[], operator LPCSTR, MakeUpper/Lower/
// Reverse, TrimLeft/Right, Find/ReverseFind/FindOneOf, Left/Right/Mid,
// Format/FormatV, GetBuffer/ReleaseBuffer, LoadString (Win32), Compare/
// CompareNoCase, and the arithmetic/comparison operators.
//
// GetBuffer uses &m_str[0] for mutable contiguous access (legal since
// C++11). ReleaseBuffer rescans for the null terminator unless given an
// explicit length.

class CString
{
public:
    CString() {}
    CString( const CString& src ) : m_str( src.m_str ) {}
    CString( LPCSTR psz ) { if ( psz ) m_str.assign( psz ); }
    CString( LPCSTR psz, int nLength ) { if ( psz && nLength > 0 ) m_str.assign( psz, nLength ); }
    CString( char ch, int nRepeat = 1 ) : m_str( (size_t)( nRepeat > 0 ? nRepeat : 0 ), ch ) {}

    operator LPCSTR() const { return m_str.c_str(); }
    LPCSTR GetString() const { return m_str.c_str(); }

    int  GetLength() const { return (int)m_str.size(); }
    BOOL IsEmpty()   const { return m_str.empty() ? TRUE : FALSE; }
    void Empty()           { m_str.clear(); }

    char GetAt( int nIndex ) const { return m_str[nIndex]; }
    void SetAt( int nIndex, char ch ) { m_str[nIndex] = ch; }
    char operator[]( int nIndex ) const { return m_str[nIndex]; }

    CString& operator=( const CString& src ) { m_str = src.m_str; return *this; }
    CString& operator=( LPCSTR psz )         { m_str = ( psz ? psz : "" ); return *this; }
    CString& operator=( char ch )            { m_str.assign( 1, ch ); return *this; }

    CString& operator+=( const CString& src ) { m_str += src.m_str; return *this; }
    CString& operator+=( LPCSTR psz )         { if ( psz ) m_str += psz; return *this; }
    CString& operator+=( char ch )            { m_str += ch; return *this; }

    void MakeUpper()   { for ( auto& c : m_str ) c = (char)std::toupper( (unsigned char)c ); }
    void MakeLower()   { for ( auto& c : m_str ) c = (char)std::tolower( (unsigned char)c ); }
    void MakeReverse() { std::reverse( m_str.begin(), m_str.end() ); }

    void TrimLeft()  { size_t i = 0; while ( i < m_str.size() && (unsigned char)m_str[i] <= ' ' ) ++i; m_str.erase( 0, i ); }
    void TrimRight() { while ( !m_str.empty() && (unsigned char)m_str.back() <= ' ' ) m_str.pop_back(); }
    void TrimLeft( char ch )  { size_t i = 0; while ( i < m_str.size() && m_str[i] == ch ) ++i; m_str.erase( 0, i ); }
    void TrimRight( char ch ) { while ( !m_str.empty() && m_str.back() == ch ) m_str.pop_back(); }

    int Find( char ch, int nStart = 0 ) const {
        size_t p = m_str.find( ch, (size_t)( nStart > 0 ? nStart : 0 ) );
        return ( p == std::string::npos ) ? -1 : (int)p;
    }
    int Find( LPCSTR psz, int nStart = 0 ) const {
        if ( !psz ) return -1;
        size_t p = m_str.find( psz, (size_t)( nStart > 0 ? nStart : 0 ) );
        return ( p == std::string::npos ) ? -1 : (int)p;
    }
    int ReverseFind( char ch ) const {
        size_t p = m_str.rfind( ch );
        return ( p == std::string::npos ) ? -1 : (int)p;
    }
    int FindOneOf( LPCSTR pszSet ) const {
        if ( !pszSet ) return -1;
        size_t p = m_str.find_first_of( pszSet );
        return ( p == std::string::npos ) ? -1 : (int)p;
    }

    CString Left( int nCount ) const {
        if ( nCount <= 0 ) return CString();
        if ( (size_t)nCount >= m_str.size() ) return *this;
        return CString( m_str.c_str(), nCount );
    }
    CString Right( int nCount ) const {
        if ( nCount <= 0 ) return CString();
        if ( (size_t)nCount >= m_str.size() ) return *this;
        return CString( m_str.c_str() + ( m_str.size() - nCount ), nCount );
    }
    CString Mid( int nFirst ) const {
        if ( nFirst < 0 ) nFirst = 0;
        if ( (size_t)nFirst >= m_str.size() ) return CString();
        return CString( m_str.c_str() + nFirst, (int)( m_str.size() - nFirst ) );
    }
    CString Mid( int nFirst, int nCount ) const {
        if ( nFirst < 0 ) nFirst = 0;
        if ( (size_t)nFirst >= m_str.size() || nCount <= 0 ) return CString();
        size_t remain = m_str.size() - (size_t)nFirst;
        if ( (size_t)nCount > remain ) nCount = (int)remain;
        return CString( m_str.c_str() + nFirst, nCount );
    }

    LPSTR GetBuffer( int nMinBufLength = 0 ) {
        if ( nMinBufLength < 0 ) nMinBufLength = 0;
        if ( (size_t)nMinBufLength > m_str.size() )
            m_str.resize( (size_t)nMinBufLength, '\0' );
        // m_str must have at least one byte addressable; an empty string still
        // has a writable null terminator via &m_str[0] under C++11.
        if ( m_str.empty() ) m_str.resize( 1, '\0' );
        return &m_str[0];
    }
    LPSTR GetBufferSetLength( int nNewLength ) {
        if ( nNewLength < 0 ) nNewLength = 0;
        m_str.resize( (size_t)nNewLength, '\0' );
        if ( m_str.empty() ) m_str.resize( 1, '\0' );
        return &m_str[0];
    }
    void ReleaseBuffer( int nNewLength = -1 ) {
        if ( nNewLength < 0 ) nNewLength = (int)std::strlen( m_str.c_str() );
        m_str.resize( (size_t)nNewLength );
    }
    int GetAllocLength() const { return (int)m_str.capacity(); }

    void Format( LPCSTR pszFmt, ... ) {
        va_list ap;
        va_start( ap, pszFmt );
        FormatV( pszFmt, ap );
        va_end( ap );
    }
    void FormatV( LPCSTR pszFmt, va_list ap ) {
        if ( !pszFmt ) { m_str.clear(); return; }
        va_list ap2;
        va_copy( ap2, ap );
        int n = std::vsnprintf( nullptr, 0, pszFmt, ap2 );
        va_end( ap2 );
        if ( n <= 0 ) { m_str.clear(); return; }
        m_str.resize( (size_t)n );
        std::vsnprintf( &m_str[0], (size_t)n + 1, pszFmt, ap );
    }
    void AppendFormat( LPCSTR pszFmt, ... ) {
        va_list ap;
        va_start( ap, pszFmt );
        CString tmp;
        tmp.FormatV( pszFmt, ap );
        va_end( ap );
        m_str += tmp.m_str;
    }

    BOOL LoadString( UINT nID ) {
        char buf[1024];
        int n = ::LoadStringA( ::GetModuleHandleA( NULL ), nID, buf, (int)sizeof( buf ) );
        if ( n <= 0 ) { m_str.clear(); return FALSE; }
        m_str.assign( buf, (size_t)n );
        return TRUE;
    }

    int Compare( LPCSTR psz ) const       { return std::strcmp( m_str.c_str(), psz ? psz : "" ); }
    int CompareNoCase( LPCSTR psz ) const { return _stricmp( m_str.c_str(), psz ? psz : "" ); }
    int Collate( LPCSTR psz ) const       { return std::strcoll( m_str.c_str(), psz ? psz : "" ); }

    int Insert( int nIndex, char ch )     { m_str.insert( (size_t)nIndex, 1, ch ); return (int)m_str.size(); }
    int Insert( int nIndex, LPCSTR psz )  { if ( psz ) m_str.insert( (size_t)nIndex, psz ); return (int)m_str.size(); }
    int Delete( int nIndex, int nCount = 1 ) {
        if ( nIndex < 0 || (size_t)nIndex >= m_str.size() || nCount <= 0 ) return (int)m_str.size();
        m_str.erase( (size_t)nIndex, (size_t)nCount );
        return (int)m_str.size();
    }
    int Replace( char chOld, char chNew ) {
        int n = 0;
        for ( auto& c : m_str ) if ( c == chOld ) { c = chNew; ++n; }
        return n;
    }
    int Remove( char ch ) {
        size_t before = m_str.size();
        m_str.erase( std::remove( m_str.begin(), m_str.end(), ch ), m_str.end() );
        return (int)( before - m_str.size() );
    }

    friend CString operator+( const CString& a, const CString& b ) { CString r( a ); r += b; return r; }
    friend CString operator+( const CString& a, LPCSTR b )         { CString r( a ); r += b; return r; }
    friend CString operator+( LPCSTR a, const CString& b )         { CString r( a ); r += b; return r; }
    friend CString operator+( const CString& a, char b )           { CString r( a ); r += b; return r; }
    friend CString operator+( char a, const CString& b )           { CString r; r += a; r += b; return r; }

    friend BOOL operator==( const CString& a, const CString& b ) { return a.m_str == b.m_str; }
    friend BOOL operator==( const CString& a, LPCSTR b )         { return a.Compare( b ) == 0; }
    friend BOOL operator==( LPCSTR a, const CString& b )         { return b.Compare( a ) == 0; }
    friend BOOL operator!=( const CString& a, const CString& b ) { return a.m_str != b.m_str; }
    friend BOOL operator!=( const CString& a, LPCSTR b )         { return a.Compare( b ) != 0; }
    friend BOOL operator!=( LPCSTR a, const CString& b )         { return b.Compare( a ) != 0; }
    friend BOOL operator< ( const CString& a, const CString& b ) { return a.m_str <  b.m_str; }
    friend BOOL operator<=( const CString& a, const CString& b ) { return a.m_str <= b.m_str; }
    friend BOOL operator> ( const CString& a, const CString& b ) { return a.m_str >  b.m_str; }
    friend BOOL operator>=( const CString& a, const CString& b ) { return a.m_str >= b.m_str; }

private:
    std::string m_str;
};

//------------------------- C E x c e p t i o n ------------------------------
// MFC exception base class. CFileException/CArchiveException are siblings
// (and inherit from CException in MFC). Live catch sites use `CException*`
// as the generic catch type; we make those siblings inherit from it.

class CException
{
public:
    virtual ~CException() {}
    virtual BOOL GetErrorMessage( LPSTR pBuf, UINT n, PUINT /*pHelp*/ = NULL ) const
    {
        if ( pBuf && n > 0 ) pBuf[0] = '\0';
        return TRUE;
    }
    virtual void Delete() { delete this; }
};

//------------------------- C F i l e E x c e p t i o n -----------------------
// Minimal stub. Callers pass it as an out-param to CFile::Open but only
// ever inspect m_cause / m_lOsError. We populate both on failure.

class CFileException : public CException
{
public:
    enum {
        none = 0, generic = 1, fileNotFound = 2, badPath = 3,
        tooManyOpenFiles = 4, accessDenied = 5, invalidFile = 6,
        removeCurrentDir = 7, directoryFull = 8, badSeek = 9,
        hardIO = 10, sharingViolation = 11, lockViolation = 12,
        diskFull = 13, endOfFile = 14
    };

    int  m_cause = none;
    LONG m_lOsError = 0;
};

//-------------------------------- C F i l e ---------------------------------
// Drop-in replacement for MFC's CFile, backed by a Win32 HANDLE. The mode
// and seek-from constants below match MFC's bit layout where it matters
// (the live code passes them straight to Open/Seek without arithmetic).

class CFile
{
public:
    // Open-mode flags (matches MFC layout)
    enum OpenFlags {
        modeRead         = 0x0000,
        modeWrite        = 0x0001,
        modeReadWrite    = 0x0002,
        shareCompat      = 0x0000,
        shareExclusive   = 0x0010,
        shareDenyWrite   = 0x0020,
        shareDenyRead    = 0x0030,
        shareDenyNone    = 0x0040,
        modeNoInherit    = 0x0080,
        modeCreate       = 0x1000,
        modeNoTruncate   = 0x2000,
        typeText         = 0x4000,
        typeBinary       = (int)0x8000,
    };

    // Seek-from constants. MFC uses these names; we forward to SetFilePointer.
    static const UINT begin   = 0;
    static const UINT current = 1;
    static const UINT end     = 2;

    typedef LONGLONG SeekPosition;

    CFile() : m_hFile( INVALID_HANDLE_VALUE ) {}
    CFile( LPCSTR pszFileName, UINT nOpenFlags ) : m_hFile( INVALID_HANDLE_VALUE )
    {
        // Open throws on failure to match MFC; live code wraps in TRY/CATCH.
        if ( !Open( pszFileName, nOpenFlags, NULL ) ) {
            CFileException* e = new CFileException();
            e->m_cause = CFileException::generic;
            e->m_lOsError = (LONG)::GetLastError();
            throw e;
        }
    }
    virtual ~CFile() { Close(); }

    BOOL Open( LPCSTR pszFileName, UINT nOpenFlags, CFileException* pError = nullptr )
    {
        Close();

        DWORD access = 0;
        if ( ( nOpenFlags & modeReadWrite ) == modeReadWrite )      access = GENERIC_READ | GENERIC_WRITE;
        else if ( ( nOpenFlags & modeWrite ) == modeWrite )         access = GENERIC_WRITE;
        else                                                         access = GENERIC_READ;

        DWORD share = 0;
        UINT shareBits = nOpenFlags & 0x70;
        if      ( shareBits == shareDenyWrite ) share = FILE_SHARE_READ;
        else if ( shareBits == shareDenyRead )  share = FILE_SHARE_WRITE;
        else if ( shareBits == shareDenyNone )  share = FILE_SHARE_READ | FILE_SHARE_WRITE;
        else                                     share = 0;          // exclusive / compat

        DWORD disposition;
        if ( nOpenFlags & modeCreate )
            disposition = ( nOpenFlags & modeNoTruncate ) ? OPEN_ALWAYS : CREATE_ALWAYS;
        else
            disposition = OPEN_EXISTING;

        m_hFile = ::CreateFileA( pszFileName, access, share, NULL,
                                 disposition, FILE_ATTRIBUTE_NORMAL, NULL );
        if ( m_hFile == INVALID_HANDLE_VALUE ) {
            if ( pError ) {
                pError->m_lOsError = (LONG)::GetLastError();
                pError->m_cause    = CFileException::generic;
            }
            return FALSE;
        }
        return TRUE;
    }

    virtual void Close()
    {
        if ( m_hFile != INVALID_HANDLE_VALUE ) {
            ::CloseHandle( m_hFile );
            m_hFile = INVALID_HANDLE_VALUE;
        }
    }

    virtual UINT Read( void* pBuf, UINT nCount )
    {
        if ( m_hFile == INVALID_HANDLE_VALUE || nCount == 0 ) return 0;
        DWORD got = 0;
        if ( !::ReadFile( m_hFile, pBuf, nCount, &got, NULL ) ) return 0;
        return (UINT)got;
    }

    virtual void Write( const void* pBuf, UINT nCount )
    {
        if ( m_hFile == INVALID_HANDLE_VALUE || nCount == 0 ) return;
        DWORD wrote = 0;
        ::WriteFile( m_hFile, pBuf, nCount, &wrote, NULL );
    }

    virtual LONG Seek( LONG lOff, UINT nFrom )
    {
        if ( m_hFile == INVALID_HANDLE_VALUE ) return -1;
        DWORD method = FILE_BEGIN;
        if      ( nFrom == current ) method = FILE_CURRENT;
        else if ( nFrom == end )     method = FILE_END;
        DWORD ret = ::SetFilePointer( m_hFile, lOff, NULL, method );
        return (LONG)ret;
    }
    void SeekToBegin() { Seek( 0, begin ); }
    LONG SeekToEnd()   { return Seek( 0, end ); }

    virtual DWORD GetPosition() const
    {
        if ( m_hFile == INVALID_HANDLE_VALUE ) return 0;
        return ::SetFilePointer( m_hFile, 0, NULL, FILE_CURRENT );
    }

    virtual DWORD GetLength() const
    {
        if ( m_hFile == INVALID_HANDLE_VALUE ) return 0;
        return ::GetFileSize( m_hFile, NULL );
    }

    virtual void SetLength( DWORD dwNewLen )
    {
        if ( m_hFile == INVALID_HANDLE_VALUE ) return;
        ::SetFilePointer( m_hFile, (LONG)dwNewLen, NULL, FILE_BEGIN );
        ::SetEndOfFile( m_hFile );
    }

    virtual void Flush()
    {
        if ( m_hFile != INVALID_HANDLE_VALUE )
            ::FlushFileBuffers( m_hFile );
    }

    virtual void Abort()
    {
        Close();
    }

    // Reads up to nMax chars including any embedded newline. Returns the
    // buffer pointer on success, NULL at EOF (matches MFC behavior).
    LPSTR ReadString( LPSTR pBuf, UINT nMax )
    {
        if ( m_hFile == INVALID_HANDLE_VALUE || !pBuf || nMax == 0 ) return nullptr;
        UINT i = 0;
        char ch = 0;
        while ( i + 1 < nMax ) {
            DWORD got = 0;
            if ( !::ReadFile( m_hFile, &ch, 1, &got, NULL ) || got == 0 ) {
                if ( i == 0 ) return nullptr;
                break;
            }
            pBuf[i++] = ch;
            if ( ch == '\n' ) break;
        }
        pBuf[i] = '\0';
        return pBuf;
    }

    // Reads one line into the CString. Returns FALSE at EOF.
    BOOL ReadString( CString& rString )
    {
        rString.Empty();
        char buf[1024];
        BOOL gotAny = FALSE;
        for ( ;; ) {
            if ( !ReadString( buf, sizeof( buf ) ) ) break;
            gotAny = TRUE;
            int n = (int)std::strlen( buf );
            // Strip trailing newline (and CR, if CRLF).
            BOOL doneLine = ( n > 0 && buf[n - 1] == '\n' );
            if ( doneLine ) {
                buf[--n] = '\0';
                if ( n > 0 && buf[n - 1] == '\r' ) buf[--n] = '\0';
            }
            rString += buf;
            if ( doneLine ) break;
        }
        return gotAny;
    }

    void WriteString( LPCSTR psz )
    {
        if ( psz ) Write( psz, (UINT)std::strlen( psz ) );
    }

    // Static query: populate `status` with file metadata. Returns TRUE on
    // success. Matches MFC's CFile::GetStatus static signature.
    static BOOL GetStatus( LPCSTR pszFileName, struct CFileStatus& status );

    HANDLE m_hFile;
};

//------------------------------ C A r c h i v e -----------------------------
// Minimal CArchive: thin wrapper around a CFile* with operator<</>> for
// every primitive type the codebase serializes (BYTE/WORD/DWORD, signed
// equivalents, char, short, int, long, LONGLONG, float, double, bool),
// plus CString (length-prefixed, matches MFC's wire format closely enough
// for our save-files since 5c's std::string bridge already paid that tax).

class CArchive
{
public:
    enum Mode { load = 0, store = 1 };

    CArchive( CFile* pFile, UINT nMode ) : m_pFile( pFile ), m_bStoring( nMode == store ) {}
    virtual ~CArchive() { Close(); }

    BOOL IsLoading() const { return m_bStoring ? FALSE : TRUE; }
    BOOL IsStoring() const { return m_bStoring ? TRUE : FALSE; }

    CFile* GetFile() const { return m_pFile; }

    void Flush() { if ( m_pFile ) m_pFile->Flush(); }
    void Close() { if ( m_pFile ) { m_pFile->Flush(); m_pFile = nullptr; } }

    // CArchive::Read returns the byte count actually transferred (matches
    // MFC); callers in opfor save/load use it for short-read detection.
    UINT Read( void* p, UINT n )       { return m_pFile ? m_pFile->Read( p, n ) : 0; }
    void Write( const void* p, UINT n ){ if ( m_pFile ) m_pFile->Write( p, n ); }

    // MFC's variable-length count encoding. We use a simple fixed DWORD —
    // good enough for our use (round-trip works because both sides use the
    // same helper).
    void  WriteCount( DWORD n ) { Write( &n, sizeof n ); }
    DWORD ReadCount()           { DWORD n = 0; Read( &n, sizeof n ); return n; }

    // --- primitive write/read ---
    #define MFC_COMPAT_AR_OP( T ) \
        CArchive& operator<<( T v )       { Write( &v, sizeof v ); return *this; } \
        CArchive& operator>>( T& v )      { Read( &v, sizeof v );  return *this; }

    // NOTE: ULONG and DWORD are both `typedef unsigned long` on Win32 (so
    // are typedef-identical) — declaring both would be a redefinition. The
    // DWORD overload binds for ULONG callers transparently. Same story for
    // any future BOOL (== int) overload.
    MFC_COMPAT_AR_OP( BYTE )
    MFC_COMPAT_AR_OP( WORD )
    MFC_COMPAT_AR_OP( DWORD )
    MFC_COMPAT_AR_OP( char )
    MFC_COMPAT_AR_OP( short )
    MFC_COMPAT_AR_OP( int )
    MFC_COMPAT_AR_OP( UINT )
    MFC_COMPAT_AR_OP( LONG )
    MFC_COMPAT_AR_OP( LONGLONG )
    MFC_COMPAT_AR_OP( ULONGLONG )
    MFC_COMPAT_AR_OP( float )
    MFC_COMPAT_AR_OP( double )
    MFC_COMPAT_AR_OP( bool )

    #undef MFC_COMPAT_AR_OP

    // CString: length-prefixed (DWORD) + payload bytes. MFC's real wire
    // format uses a variable-width length prefix; this simpler scheme is
    // self-consistent and matches what the Phase 5c std::string bridge
    // already does via the temporary CString round-trip.
    CArchive& operator<<( const CString& s )
    {
        DWORD n = (DWORD)s.GetLength();
        Write( &n, sizeof n );
        if ( n ) Write( (LPCSTR)s, n );
        return *this;
    }
    CArchive& operator>>( CString& s )
    {
        DWORD n = 0;
        Read( &n, sizeof n );
        s.Empty();
        if ( n ) {
            LPSTR buf = s.GetBuffer( (int)n );
            Read( buf, n );
            s.ReleaseBuffer( (int)n );
        }
        return *this;
    }


private:
    CFile* m_pFile;
    bool   m_bStoring;
};

//-------------------------------- C M a p -----------------------------------
// MFC's CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>. The live code instantiates it
// as e.g. `CMap<DWORD, DWORD, CBuilding*, CBuilding*>` for per-hex maps
// keyed by entity ID. Backed by std::map (ordered iteration, which matches
// CMap's hashed iteration order well enough — code that depends on order
// already sorts at the call site).
//
// POSITION is an opaque pointer in MFC. We use a heap-allocated wrapper
// holding the current iterator. GetNextAssoc returns the value, advances,
// and frees the wrapper when iteration is exhausted (setting the user's
// POSITION to nullptr — matching MFC's "POSITION goes NULL at end").

typedef void* POSITION;

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class CMap
{
public:
    CMap() {}
    ~CMap() {}

    UINT GetCount() const  { return (UINT)m_map.size(); }
    BOOL IsEmpty()  const  { return m_map.empty() ? TRUE : FALSE; }

    BOOL Lookup( ARG_KEY key, VALUE& rValue ) const
    {
        auto it = m_map.find( key );
        if ( it == m_map.end() ) return FALSE;
        rValue = it->second;
        return TRUE;
    }

    VALUE& operator[]( ARG_KEY key ) { return m_map[key]; }

    void SetAt( ARG_KEY key, ARG_VALUE newValue ) { m_map[key] = newValue; }

    BOOL RemoveKey( ARG_KEY key )
    {
        return m_map.erase( key ) ? TRUE : FALSE;
    }

    void RemoveAll() { m_map.clear(); }

    // MFC's hash-map tuning hooks. NOP under std::map backing.
    void InitHashTable( UINT /*nHashSize*/, BOOL /*bAllocNow*/ = TRUE ) {}
    void SetHashTableSize( UINT /*nHashSize*/ ) {}

    POSITION GetStartPosition() const
    {
        if ( m_map.empty() ) return nullptr;
        auto* pos = new IterPos{ m_map.begin(), &m_map };
        return (POSITION)pos;
    }

    void GetNextAssoc( POSITION& rNextPosition, KEY& rKey, VALUE& rValue ) const
    {
        IterPos* p = (IterPos*)rNextPosition;
        if ( !p ) return;
        rKey   = p->it->first;
        rValue = p->it->second;
        ++p->it;
        if ( p->it == p->map->end() ) {
            delete p;
            rNextPosition = nullptr;
        }
    }

    // No-op Serialize: the gate-on save format doesn't round-trip MFC
    // collections through CArchive — game state goes through bespoke
    // CFile reads/writes elsewhere. Just lets prototype declarations link.
    void Serialize( class CArchive& /*ar*/ ) {}

private:
    typedef std::map<KEY, VALUE> MapT;
    struct IterPos {
        typename MapT::const_iterator it;
        const MapT* map;
    };
    MapT m_map;
};

// Named instantiations matching MFC's collection typedefs. Add more as the
// gate-on build surfaces them.
typedef CMap<CString, LPCSTR, void*, void*>     CMapStringToPtr;
typedef CMap<WORD,    WORD,   void*, void*>     CMapWordToPtr;
typedef CMap<void*,   void*,  void*, void*>     CMapPtrToPtr;
typedef CMap<void*,   void*,  WORD,  WORD>      CMapPtrToWord;

//------------------------------ C O b j e c t -------------------------------
// Phase 1g step 2: minimal CObject stub. The cai*.hpp AI classes inherit
// CObject for legacy reasons but don't use serialization, runtime class info,
// or IsKindOf in the live code paths. A virtual dtor is enough.

class CArchive;  // forward — CObject::Serialize takes one
class CObject
{
public:
    virtual ~CObject() {}
    virtual void Serialize( CArchive& /*ar*/ ) {}

protected:
    CObject() {}
    CObject( const CObject& ) {}
    CObject& operator=( const CObject& ) { return *this; }
};

// CObject pointer-graph serialization. MFC's real implementation walks a
// class-tag table; ours just writes/reads a placeholder DWORD so prototype
// declarations link. The gate-on save format doesn't actually round-trip
// CObject graphs through CArchive — game state goes through bespoke
// CFile reads/writes elsewhere.
inline CArchive& operator<<( CArchive& ar, const CObject* /*pOb*/ )
{
    DWORD zero = 0;
    ar.Write( &zero, sizeof zero );
    return ar;
}
inline CArchive& operator>>( CArchive& ar, CObject*& pOb )
{
    DWORD ignore = 0;
    ar.Read( &ignore, sizeof ignore );
    pOb = NULL;
    return ar;
}

// CDialog and CDialogBar are defined below after CWnd — they inherit it
// so that `CDlgSelCD::OnOK` etc. can call CWnd methods like
// `SetDlgItemText` and `UpdateWindow` directly.
class CDataExchange;
class CDialog;
class CDialogBar;

//------------------------- C W i n T h r e a d ------------------------------
// MFC's worker-thread class. Live code (CMusicPlayer) uses these methods:
// m_hThread (HANDLE), ResumeThread, SuspendThread, m_bAutoDelete (bool).
// AfxBeginThread starts a new thread.

// MFC uses __cdecl for its thread procs (matches the _beginthreadex signature
// it wraps). Match that so caller cast-free assignment works.
typedef UINT( __cdecl* AFX_THREADPROC )( LPVOID );

// CWinThread inherits CObject because legacy code stuffs them into CObList
// (CList<CObject*>). The Diamond is intentional — virtual dtor only.
class CWinThread : public CObject
{
public:
    CWinThread() : m_hThread( NULL ), m_nThreadID( 0 ), m_bAutoDelete( TRUE ) {}
    virtual ~CWinThread() { if ( m_hThread ) ::CloseHandle( m_hThread ); }

    DWORD ResumeThread() {
        if (!m_hThread) return (DWORD)-1;
        DWORD prev = ::ResumeThread(m_hThread);
        // Phase 1g step 2 race-fix: legacy code (music.cpp's read-ahead
        // worker) uses the SuspendThread/ResumeThread self-suspend pattern
        // for synchronization. That pattern races inherently — if main
        // calls ResumeThread BEFORE the worker has entered its
        // ::SuspendThread syscall, our resume returns prev=0 (no-op) and
        // the worker then self-suspends with nobody to wake it.
        //
        // MFC's AfxBeginThread used to mask this via _beginthreadex's CRT
        // setup latency; our raw CreateThread is faster, so the race fires
        // every time. Retry up to ~50ms until we've actually decremented a
        // suspended thread (prev >= 1) or we time out.
        if (prev == 0 && ::GetCurrentThreadId() != m_nThreadID) {
            for (int i = 0; i < 50 && prev == 0; ++i) {
                ::Sleep(1);
                prev = ::ResumeThread(m_hThread);
            }
        }
        return prev;
    }
    DWORD SuspendThread() { return m_hThread ? ::SuspendThread(m_hThread) : (DWORD)-1; }
    BOOL  SetThreadPriority( int nPri ) { return m_hThread ? ::SetThreadPriority( m_hThread, nPri ) : FALSE; }
    int   GetThreadPriority() const { return m_hThread ? ::GetThreadPriority( m_hThread ) : THREAD_PRIORITY_NORMAL; }

    HANDLE m_hThread;
    DWORD  m_nThreadID;
    BOOL   m_bAutoDelete;
};

inline CWinThread* AfxBeginThread( AFX_THREADPROC pfnThreadProc, LPVOID pParam,
                                   int nPriority = THREAD_PRIORITY_NORMAL,
                                   UINT /*nStackSize*/ = 0,
                                   DWORD dwCreateFlags = 0,
                                   LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL )
{
    CWinThread* p = new CWinThread();
    HANDLE h = ::CreateThread( lpSecurityAttrs, 0,
                               (LPTHREAD_START_ROUTINE)pfnThreadProc, pParam,
                               dwCreateFlags, &p->m_nThreadID );
    if ( !h ) { delete p; return NULL; }
    p->m_hThread = h;
    if ( nPriority != THREAD_PRIORITY_NORMAL ) ::SetThreadPriority( h, nPriority );
    return p;
}

// AfxGetThread/AfxGetApp: live code only checks for nullptr or pokes m_hThread
// for logging — returning NULL is correct under the gate (we don't have an
// MFC thread/app object once CWinAppStub took over).
inline CWinThread* AfxGetThread() { return NULL; }
inline void        AfxEndThread( UINT nExitCode, BOOL /*bDelete*/ = TRUE ) { ::ExitThread( nExitCode ); }
inline HINSTANCE   AfxGetResourceHandle()                 { return ::GetModuleHandleA( NULL ); }
inline void        AfxSetResourceHandle( HINSTANCE /*h*/ ) {}
inline HINSTANCE   AfxGetInstanceHandle()                 { return ::GetModuleHandleA( NULL ); }

//------------------------- C C r i t i c a l S e c t i o n ------------------
// MFC's sync primitives. CSyncObject is the base; CCriticalSection wraps
// CRITICAL_SECTION; CMutex wraps a named mutex; CSingleLock RAII-acquires.

class CSyncObject
{
public:
    virtual ~CSyncObject() {}
    virtual BOOL Lock( DWORD /*ms*/ = INFINITE )   { return TRUE; }
    virtual BOOL Unlock()                          { return TRUE; }
};

class CCriticalSection : public CSyncObject
{
public:
    CCriticalSection()  { ::InitializeCriticalSection( &m_cs ); }
    ~CCriticalSection() { ::DeleteCriticalSection( &m_cs ); }
    BOOL Lock( DWORD /*ms*/ = INFINITE ) override { ::EnterCriticalSection( &m_cs ); return TRUE; }
    BOOL Unlock()                        override { ::LeaveCriticalSection( &m_cs ); return TRUE; }
    operator CRITICAL_SECTION*() { return &m_cs; }

    CRITICAL_SECTION m_cs;
};

class CMutex : public CSyncObject
{
public:
    CMutex( BOOL bInitiallyOwn = FALSE, LPCSTR pszName = NULL, LPSECURITY_ATTRIBUTES psa = NULL )
    {
        m_hMutex = ::CreateMutexA( psa, bInitiallyOwn, pszName );
    }
    ~CMutex() { if ( m_hMutex ) ::CloseHandle( m_hMutex ); }
    BOOL Lock( DWORD ms = INFINITE ) override { return m_hMutex && ::WaitForSingleObject( m_hMutex, ms ) == WAIT_OBJECT_0; }
    BOOL Unlock()                    override { return m_hMutex && ::ReleaseMutex( m_hMutex ); }
    operator HANDLE() const { return m_hMutex; }

    HANDLE m_hMutex;
};

class CSingleLock
{
public:
    CSingleLock( CSyncObject* pObj, BOOL bInitialLock = FALSE ) : m_pObj( pObj ), m_bAcquired( FALSE )
    {
        if ( bInitialLock ) Lock();
    }
    ~CSingleLock() { Unlock(); }
    BOOL Lock( DWORD ms = INFINITE ) { if ( m_pObj && !m_bAcquired ) m_bAcquired = m_pObj->Lock( ms ); return m_bAcquired; }
    BOOL Unlock()                    { if ( m_pObj && m_bAcquired ) { m_pObj->Unlock(); m_bAcquired = FALSE; } return TRUE; }
    BOOL IsLocked() const { return m_bAcquired; }

private:
    CSyncObject* m_pObj;
    BOOL         m_bAcquired;
};

//-------------------------------- C A r r a y -------------------------------
// MFC's CArray<TYPE, ARG_TYPE> — dynamic array. CRITICAL: must match MFC's
// memcpy-on-grow + ConstructElements/DestructElements pattern (NOT std::
// vector's move-construct-then-destroy). Live code (music.cpp's CRawData)
// has classes with raw pointer members and trivial copy semantics; MFC
// relies on bitwise copy when growing the storage. A std::vector backing
// triggers move-then-dtor cycles that double-free those pointers.
//
// Storage: raw malloc/free buffer. ConstructElements/DestructElements are
// called for new/removed tail elements, and unqualified-name lookup +
// ADL pick up per-type overloads (e.g. music.cpp's CRawData overrides)
// when present, falling back to the catch-all templates below.

template<class TYPE, class ARG_TYPE>
class CArray
{
public:
    CArray() : m_pData( nullptr ), m_nSize( 0 ), m_nMaxSize( 0 ) {}
    ~CArray()
    {
        if ( m_nSize > 0 ) DestructElements( m_pData, m_nSize );
        if ( m_pData ) std::free( m_pData );
    }

    int  GetSize() const  { return m_nSize; }
    int  GetCount() const { return m_nSize; }
    BOOL IsEmpty() const  { return m_nSize == 0 ? TRUE : FALSE; }
    int  GetUpperBound() const { return m_nSize - 1; }

    void SetSize( int nNewSize, int /*nGrowBy*/ = -1 )
    {
        if ( nNewSize < 0 ) nNewSize = 0;
        if ( nNewSize == m_nSize ) return;
        if ( nNewSize < m_nSize ) {
            DestructElements( m_pData + nNewSize, m_nSize - nNewSize );
            m_nSize = nNewSize;
            return;
        }
        // Growing — ensure capacity, then construct the tail.
        if ( nNewSize > m_nMaxSize ) {
            TYPE* pNew = (TYPE*)std::malloc( (size_t)nNewSize * sizeof( TYPE ) );
            if ( m_nSize > 0 ) std::memcpy( pNew, m_pData, (size_t)m_nSize * sizeof( TYPE ) );
            if ( m_pData ) std::free( m_pData );
            m_pData = pNew;
            m_nMaxSize = nNewSize;
        }
        ConstructElements( m_pData + m_nSize, nNewSize - m_nSize );
        m_nSize = nNewSize;
    }

    void RemoveAll()
    {
        if ( m_nSize > 0 ) DestructElements( m_pData, m_nSize );
        m_nSize = 0;
    }
    void RemoveAt( int nIndex, int nCount = 1 )
    {
        if ( nIndex < 0 || nCount <= 0 || nIndex >= m_nSize ) return;
        if ( nIndex + nCount > m_nSize ) nCount = m_nSize - nIndex;
        DestructElements( m_pData + nIndex, nCount );
        int nTail = m_nSize - ( nIndex + nCount );
        if ( nTail > 0 )
            std::memmove( m_pData + nIndex, m_pData + nIndex + nCount, (size_t)nTail * sizeof( TYPE ) );
        m_nSize -= nCount;
    }

    int Add( ARG_TYPE v )
    {
        SetSize( m_nSize + 1 );
        m_pData[m_nSize - 1] = v;
        return m_nSize - 1;
    }
    void InsertAt( int nIndex, ARG_TYPE v, int nCount = 1 )
    {
        if ( nIndex < 0 || nCount <= 0 ) return;
        if ( nIndex >= m_nSize ) { SetSize( nIndex + nCount ); for ( int i = 0; i < nCount; ++i ) m_pData[nIndex + i] = v; return; }
        int nOld = m_nSize;
        SetSize( m_nSize + nCount );
        std::memmove( m_pData + nIndex + nCount, m_pData + nIndex, (size_t)( nOld - nIndex ) * sizeof( TYPE ) );
        for ( int i = 0; i < nCount; ++i ) m_pData[nIndex + i] = v;
    }

    const TYPE& GetAt( int nIndex ) const { return m_pData[nIndex]; }
    TYPE&       GetAt( int nIndex )       { return m_pData[nIndex]; }
    void SetAt( int nIndex, ARG_TYPE v )  { m_pData[nIndex] = v; }
    void SetAtGrow( int nIndex, ARG_TYPE v )
    {
        if ( nIndex >= m_nSize ) SetSize( nIndex + 1 );
        m_pData[nIndex] = v;
    }
    TYPE&       ElementAt( int nIndex )       { return m_pData[nIndex]; }
    const TYPE& operator[]( int nIndex ) const { return m_pData[nIndex]; }
    TYPE&       operator[]( int nIndex )       { return m_pData[nIndex]; }

    const TYPE* GetData() const { return m_pData; }
    TYPE*       GetData()       { return m_pData; }

private:
    TYPE* m_pData;
    int   m_nSize;
    int   m_nMaxSize;
};

//-------------------------------- C L i s t ---------------------------------
// MFC's doubly-linked list. std::list-backed. POSITION here is a heap-
// allocated iterator wrapper, same scheme as CMap. Surface kept narrow to
// what the live code uses: AddTail, AddHead, GetCount, IsEmpty, RemoveAll,
// GetHead, GetTail, RemoveHead, RemoveTail, GetHeadPosition, GetNext,
// GetPrev, Find, RemoveAt.

template<class TYPE, class ARG_TYPE>
class CList
{
public:
    int  GetCount() const { return (int)m_list.size(); }
    BOOL IsEmpty()  const { return m_list.empty() ? TRUE : FALSE; }
    void RemoveAll()      { m_list.clear(); }

    TYPE& GetHead()             { return m_list.front(); }
    const TYPE& GetHead() const { return m_list.front(); }
    TYPE& GetTail()             { return m_list.back(); }
    const TYPE& GetTail() const { return m_list.back(); }

    POSITION AddHead( ARG_TYPE v ) { m_list.push_front( v ); return wrap( m_list.begin() ); }
    POSITION AddTail( ARG_TYPE v ) { m_list.push_back( v );  return wrap( --m_list.end() ); }

    TYPE RemoveHead() { TYPE v = m_list.front(); m_list.pop_front(); return v; }
    TYPE RemoveTail() { TYPE v = m_list.back();  m_list.pop_back();  return v; }

    POSITION GetHeadPosition() const { return m_list.empty() ? nullptr : wrap( m_list.begin() ); }
    POSITION GetTailPosition() const { return m_list.empty() ? nullptr : wrap( --m_list.end() ); }

    TYPE& GetNext( POSITION& pos )
    {
        IterPos* p = (IterPos*)pos;
        TYPE& ref = *p->it;
        ++p->it;
        if ( p->it == m_list.end() ) { delete p; pos = nullptr; }
        return ref;
    }
    const TYPE& GetNext( POSITION& pos ) const
    {
        IterPos* p = (IterPos*)pos;
        const TYPE& ref = *p->it;
        ++p->it;
        if ( p->it == m_list.end() ) { delete p; pos = nullptr; }
        return ref;
    }
    TYPE& GetPrev( POSITION& pos )
    {
        IterPos* p = (IterPos*)pos;
        TYPE& ref = *p->it;
        if ( p->it == m_list.begin() ) { delete p; pos = nullptr; }
        else --p->it;
        return ref;
    }
    const TYPE& GetPrev( POSITION& pos ) const
    {
        IterPos* p = (IterPos*)pos;
        const TYPE& ref = *p->it;
        if ( p->it == m_list.begin() ) { delete p; pos = nullptr; }
        else --p->it;
        return ref;
    }
    TYPE& GetAt( POSITION pos )       { return *( (IterPos*)pos )->it; }
    const TYPE& GetAt( POSITION pos ) const { return *( (IterPos*)pos )->it; }
    void SetAt( POSITION pos, ARG_TYPE v ) { *( (IterPos*)pos )->it = v; }

    POSITION Find( ARG_TYPE v, POSITION /*after*/ = nullptr ) const
    {
        for ( auto it = m_list.begin(); it != m_list.end(); ++it )
            if ( *it == v ) return wrap( it );
        return nullptr;
    }
    POSITION FindIndex( int nIndex ) const
    {
        if ( nIndex < 0 || (size_t)nIndex >= m_list.size() ) return nullptr;
        auto it = m_list.begin();
        std::advance( it, nIndex );
        return wrap( it );
    }
    POSITION InsertBefore( POSITION pos, ARG_TYPE v )
    {
        IterPos* p = (IterPos*)pos;
        auto inserted = const_cast<ListT&>( m_list ).insert( p->it, v );
        return wrap( inserted );
    }
    POSITION InsertAfter( POSITION pos, ARG_TYPE v )
    {
        IterPos* p = (IterPos*)pos;
        auto next = p->it; ++next;
        auto inserted = const_cast<ListT&>( m_list ).insert( next, v );
        return wrap( inserted );
    }
    void RemoveAt( POSITION pos )
    {
        IterPos* p = (IterPos*)pos;
        // const_cast OK: we own the list, the const iterator is just our wire format
        m_list.erase( m_list.erase( p->it, p->it ) );
        delete p;
    }

    // No-op Serialize (see CMap::Serialize comment).
    void Serialize( class CArchive& /*ar*/ ) {}

private:
    typedef std::list<TYPE> ListT;
    struct IterPos { typename ListT::iterator it; };
    POSITION wrap( typename ListT::const_iterator it ) const
    {
        auto* p = new IterPos;
        // const_cast: stored iterators are mutable; the POSITION wire is opaque
        p->it = const_cast<ListT&>( m_list ).erase( it, it );  // no-op erase returns mutable iterator
        return (POSITION)p;
    }
    ListT m_list;
};

//------------------------- G D I   o b j e c t   w r a p p e r s ------------
// Thin classes around Win32 GDI handles. Pattern: each holds an HXXX
// member, supports FromHandle (returns a freshly-allocated wrapper that
// the caller does NOT delete — leaks one per call, like MFC's temp-handle
// map but simpler), Attach/Detach, and GetSafeHandle / m_hObject access.
//
// These don't own the handle by default; ctor with a handle takes ownership.
// Stubs are deliberately minimal — most live calls are owner-draw paths
// that target hidden MFC windows in the gate-on game.

class CGdiObject
{
public:
    CGdiObject() : m_hObject( NULL ) {}
    virtual ~CGdiObject() {}
    HGDIOBJ GetSafeHandle() const { return m_hObject; }
    BOOL    Attach( HGDIOBJ h )   { m_hObject = h; return h != NULL; }
    HGDIOBJ Detach()              { HGDIOBJ h = m_hObject; m_hObject = NULL; return h; }
    BOOL    DeleteObject()        { BOOL r = ::DeleteObject( m_hObject ); m_hObject = NULL; return r; }
    operator HGDIOBJ() const { return m_hObject; }

    HGDIOBJ m_hObject;
};

class CFont : public CGdiObject
{
public:
    CFont() {}
    BOOL CreateFontIndirect( const LOGFONT* plf )
    {
        m_hObject = (HGDIOBJ)::CreateFontIndirectA( plf );
        return m_hObject != NULL;
    }
    int GetLogFont( LOGFONT* plf ) const { return ::GetObjectA( m_hObject, sizeof( LOGFONT ), plf ); }
    static CFont* FromHandle( HFONT h ) { auto* p = new CFont(); p->m_hObject = (HGDIOBJ)h; return p; }
    operator HFONT() const { return (HFONT)m_hObject; }
};

class CBrush : public CGdiObject
{
public:
    CBrush() {}
    CBrush( COLORREF cr ) { m_hObject = (HGDIOBJ)::CreateSolidBrush( cr ); }
    BOOL CreateSolidBrush( COLORREF cr )
    {
        m_hObject = (HGDIOBJ)::CreateSolidBrush( cr );
        return m_hObject != NULL;
    }
    static CBrush* FromHandle( HBRUSH h ) { auto* p = new CBrush(); p->m_hObject = (HGDIOBJ)h; return p; }
    operator HBRUSH() const { return (HBRUSH)m_hObject; }
};

class CPen : public CGdiObject
{
public:
    CPen() {}
    CPen( int nPenStyle, int nWidth, COLORREF cr )
    {
        m_hObject = (HGDIOBJ)::CreatePen( nPenStyle, nWidth, cr );
    }
    BOOL CreatePen( int nPenStyle, int nWidth, COLORREF cr )
    {
        m_hObject = (HGDIOBJ)::CreatePen( nPenStyle, nWidth, cr );
        return m_hObject != NULL;
    }
    static CPen* FromHandle( HPEN h ) { auto* p = new CPen(); p->m_hObject = (HGDIOBJ)h; return p; }
    operator HPEN() const { return (HPEN)m_hObject; }
};

class CBitmap : public CGdiObject
{
public:
    CBitmap() {}
    int GetBitmap( BITMAP* pBitmap ) const { return ::GetObjectA( m_hObject, sizeof( BITMAP ), pBitmap ); }
    static CBitmap* FromHandle( HBITMAP h ) { auto* p = new CBitmap(); p->m_hObject = (HGDIOBJ)h; return p; }
    operator HBITMAP() const { return (HBITMAP)m_hObject; }
};

class CPalette : public CGdiObject
{
public:
    CPalette() {}
    BOOL CreatePalette( LPLOGPALETTE plp ) { m_hObject = (HGDIOBJ)::CreatePalette( plp ); return m_hObject != NULL; }
    static CPalette* FromHandle( HPALETTE h ) { auto* p = new CPalette(); p->m_hObject = (HGDIOBJ)h; return p; }
    operator HPALETTE() const { return (HPALETTE)m_hObject; }
};

class CRgn : public CGdiObject
{
public:
    CRgn() {}
    BOOL CreateRectRgn( int x1, int y1, int x2, int y2 )
    {
        m_hObject = (HGDIOBJ)::CreateRectRgn( x1, y1, x2, y2 );
        return m_hObject != NULL;
    }
    BOOL CreatePolygonRgn( LPPOINT pPts, int nCount, int nMode )
    {
        m_hObject = (HGDIOBJ)::CreatePolygonRgn( pPts, nCount, nMode );
        return m_hObject != NULL;
    }
    BOOL CreateEllipticRgn( int x1, int y1, int x2, int y2 )
    {
        m_hObject = (HGDIOBJ)::CreateEllipticRgn( x1, y1, x2, y2 );
        return m_hObject != NULL;
    }
    static CRgn* FromHandle( HRGN h ) { auto* p = new CRgn(); p->m_hObject = (HGDIOBJ)h; return p; }
    operator HRGN() const { return (HRGN)m_hObject; }
};

//-------------------------------- C D C -------------------------------------
// Drawing context. Holds m_hDC (and m_hAttribDC, both same in MFC for
// non-metafile cases). Minimal surface — most rendering already moved to
// SDL2; what remains is owner-draw helpers in subclass.cpp that target
// hidden MFC windows under the stub gates.

class CDC
{
public:
    CDC() : m_hDC( NULL ), m_hAttribDC( NULL ) {}
    virtual ~CDC() {}

    HDC GetSafeHdc() const { return m_hDC; }
    operator HDC() const   { return m_hDC; }

    BOOL Attach( HDC h ) { m_hDC = h; m_hAttribDC = h; return h != NULL; }
    HDC  Detach()        { HDC h = m_hDC; m_hDC = m_hAttribDC = NULL; return h; }

    static CDC* FromHandle( HDC h ) { auto* p = new CDC(); p->m_hDC = h; p->m_hAttribDC = h; return p; }

    // SelectObject — MFC returns the previous CXxx*. We return a fresh
    // wrapper around the previously-selected handle.
    CFont*    SelectObject( CFont* pFont )       { return CFont::FromHandle( (HFONT)::SelectObject( m_hDC, pFont ? pFont->m_hObject : NULL ) ); }
    CBrush*   SelectObject( CBrush* pBrush )     { return CBrush::FromHandle( (HBRUSH)::SelectObject( m_hDC, pBrush ? pBrush->m_hObject : NULL ) ); }
    CPen*     SelectObject( CPen* pPen )         { return CPen::FromHandle( (HPEN)::SelectObject( m_hDC, pPen ? pPen->m_hObject : NULL ) ); }
    CBitmap*  SelectObject( CBitmap* pBitmap )   { return CBitmap::FromHandle( (HBITMAP)::SelectObject( m_hDC, pBitmap ? pBitmap->m_hObject : NULL ) ); }
    CPalette* SelectPalette( CPalette* pPalette, BOOL bForceBackground )
    {
        return CPalette::FromHandle( ::SelectPalette( m_hDC, pPalette ? (HPALETTE)pPalette->m_hObject : NULL, bForceBackground ) );
    }
    UINT RealizePalette() { return ::RealizePalette( m_hDC ); }

    int  GetDeviceCaps( int nIndex ) const { return ::GetDeviceCaps( m_hDC, nIndex ); }

    COLORREF SetTextColor( COLORREF cr )      { return ::SetTextColor( m_hDC, cr ); }
    COLORREF GetTextColor() const             { return ::GetTextColor( m_hDC ); }
    COLORREF SetBkColor( COLORREF cr )        { return ::SetBkColor( m_hDC, cr ); }
    int      SetBkMode( int nBkMode )         { return ::SetBkMode( m_hDC, nBkMode ); }

    BOOL TextOut( int x, int y, LPCSTR psz, int n ) { return ::TextOutA( m_hDC, x, y, psz, n ); }
    BOOL TextOut( int x, int y, const CString& s )  { return ::TextOutA( m_hDC, x, y, (LPCSTR)s, s.GetLength() ); }
    BOOL DrawText( LPCSTR psz, int n, LPRECT pr, UINT uFormat ) { return ::DrawTextA( m_hDC, psz, n, pr, uFormat ); }
    BOOL DrawText( const CString& s, LPRECT pr, UINT uFormat )  { return ::DrawTextA( m_hDC, (LPCSTR)s, s.GetLength(), pr, uFormat ); }
    BOOL ExtTextOut( int x, int y, UINT u, LPCRECT pr, LPCSTR psz, UINT cb, const int* lpDx )
    {
        return ::ExtTextOutA( m_hDC, x, y, u, pr, psz, cb, lpDx );
    }

    int  FillRect( LPCRECT pr, CBrush* pBr )  { return ::FillRect( m_hDC, pr, pBr ? (HBRUSH)pBr->m_hObject : NULL ); }
    BOOL FillRgn( CRgn* pRgn, CBrush* pBr )   { return ::FillRgn( m_hDC, pRgn ? (HRGN)pRgn->m_hObject : NULL, pBr ? (HBRUSH)pBr->m_hObject : NULL ); }
    BOOL FillSolidRect( LPCRECT pr, COLORREF cr )
    {
        ::SetBkColor( m_hDC, cr );
        return ::ExtTextOutA( m_hDC, 0, 0, ETO_OPAQUE, pr, "", 0, NULL );
    }
    BOOL Rectangle( int l, int t, int r, int b ) { return ::Rectangle( m_hDC, l, t, r, b ); }
    BOOL RoundRect( int l, int t, int r, int b, int w, int h ) { return ::RoundRect( m_hDC, l, t, r, b, w, h ); }
    BOOL RoundRect( LPCRECT pr, POINT pt ) { return ::RoundRect( m_hDC, pr->left, pr->top, pr->right, pr->bottom, pt.x, pt.y ); }
    BOOL MoveTo( int x, int y, LPPOINT pOld = NULL ) { return ::MoveToEx( m_hDC, x, y, pOld ); }
    BOOL LineTo( int x, int y ) { return ::LineTo( m_hDC, x, y ); }
    CSize GetTextExtent( LPCSTR psz, int n ) const { SIZE sz = {0,0}; ::GetTextExtentPoint32A( m_hDC, psz, n, &sz ); return CSize( sz.cx, sz.cy ); }
    CSize GetTextExtent( const CString& s )   const { return GetTextExtent( (LPCSTR)s, s.GetLength() ); }
    BOOL  RectVisible( LPCRECT pr ) const           { return ::RectVisible( m_hDC, pr ); }
    COLORREF GetNearestColor( COLORREF cr ) const   { return ::GetNearestColor( m_hDC, cr ); }

    BOOL BitBlt( int x, int y, int w, int h, CDC* pSrc, int xSrc, int ySrc, DWORD rop )
    {
        return ::BitBlt( m_hDC, x, y, w, h, pSrc ? pSrc->m_hDC : NULL, xSrc, ySrc, rop );
    }

    HDC m_hDC;
    HDC m_hAttribDC;
};

// CClientDC / CPaintDC / CWindowDC — accept a CWnd* (we don't have a real
// CWnd here, so we accept anything that has m_hWnd via a forward-declared
// HWND-bearing template). In practice the live code passes CWnd::FromHandle
// returning a temporary CWnd-like object; we accept HWND directly.

class CClientDC : public CDC
{
public:
    explicit CClientDC( HWND hWnd ) { m_hWndDC = hWnd; m_hDC = ::GetDC( hWnd ); m_hAttribDC = m_hDC; }
    template<class WndT> explicit CClientDC( WndT* pWnd ) { m_hWndDC = pWnd ? pWnd->m_hWnd : NULL; m_hDC = ::GetDC( m_hWndDC ); m_hAttribDC = m_hDC; }
    ~CClientDC() { if ( m_hDC ) ::ReleaseDC( m_hWndDC, m_hDC ); }

    HWND m_hWndDC;
};

class CPaintDC : public CDC
{
public:
    explicit CPaintDC( HWND hWnd ) { m_hWndDC = hWnd; m_hDC = ::BeginPaint( hWnd, &m_ps ); m_hAttribDC = m_hDC; }
    template<class WndT> explicit CPaintDC( WndT* pWnd ) { m_hWndDC = pWnd ? pWnd->m_hWnd : NULL; m_hDC = ::BeginPaint( m_hWndDC, &m_ps ); m_hAttribDC = m_hDC; }
    ~CPaintDC() { if ( m_hDC ) ::EndPaint( m_hWndDC, &m_ps ); }

    HWND        m_hWndDC;
    PAINTSTRUCT m_ps;
};

class CWindowDC : public CDC
{
public:
    explicit CWindowDC( HWND hWnd ) { m_hWndDC = hWnd; m_hDC = ::GetWindowDC( hWnd ); m_hAttribDC = m_hDC; }
    template<class WndT> explicit CWindowDC( WndT* pWnd ) { m_hWndDC = pWnd ? pWnd->m_hWnd : NULL; m_hDC = ::GetWindowDC( m_hWndDC ); m_hAttribDC = m_hDC; }
    ~CWindowDC() { if ( m_hDC ) ::ReleaseDC( m_hWndDC, m_hDC ); }

    HWND m_hWndDC;
};

//------------------------------ C W n d -------------------------------------
// Minimal CWnd stub. Phase 1's CWndStub (wndstub.h) is the real replacement
// for game-side classes; this stub exists for the legacy paths that still
// reference `CWnd*` as a parameter type (subclass.cpp owner-draw helpers,
// wndstub.h's MFC-typed virtuals, etc.). The CWnd::FromHandle factory
// returns a freshly-allocated wrapper, matching MFC's temp-handle map
// semantics (the returned pointer is short-lived; callers don't delete).

class CWnd
{
public:
    CWnd() : m_hWnd( NULL ) {}
    virtual ~CWnd() {}

    HWND  GetSafeHwnd() const { return this ? m_hWnd : NULL; }
    operator HWND() const { return m_hWnd; }

    BOOL  Attach( HWND h )  { m_hWnd = h; return h != NULL; }
    HWND  Detach()          { HWND h = m_hWnd; m_hWnd = NULL; return h; }

    static CWnd* FromHandle( HWND h ) { auto* p = new CWnd(); p->m_hWnd = h; return p; }
    CWnd*  GetParent()         const { return FromHandle( ::GetParent( m_hWnd ) ); }
    CWnd*  GetDlgItem( int id) const { return FromHandle( ::GetDlgItem( m_hWnd, id ) ); }
    BOOL   EnableWindow( BOOL b = TRUE ) { return ::EnableWindow( m_hWnd, b ); }
    BOOL   ShowWindow( int n ) { return ::ShowWindow( m_hWnd, n ); }
    BOOL   IsWindow()        const { return ::IsWindow( m_hWnd ); }
    BOOL   IsWindowVisible() const { return ::IsWindowVisible( m_hWnd ); }
    void   GetClientRect( LPRECT pr ) const { ::GetClientRect( m_hWnd, pr ); }
    void   GetWindowRect( LPRECT pr ) const { ::GetWindowRect( m_hWnd, pr ); }
    BOOL   InvalidateRect( LPCRECT pr, BOOL bErase = TRUE ) { return ::InvalidateRect( m_hWnd, pr, bErase ); }
    int    GetWindowText( LPSTR pBuf, int nMax ) const  { return ::GetWindowTextA( m_hWnd, pBuf, nMax ); }
    void   GetWindowText( CString& s ) const            { char buf[1024]; int n = ::GetWindowTextA( m_hWnd, buf, sizeof( buf ) ); s = CString( buf, n ); }
    BOOL   SetWindowText( LPCSTR psz )                  { return ::SetWindowTextA( m_hWnd, psz ); }
    HICON  GetIcon( BOOL bBigIcon ) const               { return (HICON)::SendMessageA( m_hWnd, WM_GETICON, bBigIcon ? ICON_BIG : ICON_SMALL, 0 ); }
    void   ClientToScreen( LPPOINT p )  const           { ::ClientToScreen( m_hWnd, p ); }
    void   ClientToScreen( LPRECT  r )  const           { POINT tl = { r->left, r->top }, br = { r->right, r->bottom }; ::ClientToScreen( m_hWnd, &tl ); ::ClientToScreen( m_hWnd, &br ); r->left = tl.x; r->top = tl.y; r->right = br.x; r->bottom = br.y; }
    void   ScreenToClient( LPPOINT p )  const           { ::ScreenToClient( m_hWnd, p ); }
    void   ScreenToClient( LPRECT  r )  const           { POINT tl = { r->left, r->top }, br = { r->right, r->bottom }; ::ScreenToClient( m_hWnd, &tl ); ::ScreenToClient( m_hWnd, &br ); r->left = tl.x; r->top = tl.y; r->right = br.x; r->bottom = br.y; }
    LRESULT SendMessage( UINT msg, WPARAM wp = 0, LPARAM lp = 0 ) { return ::SendMessageA( m_hWnd, msg, wp, lp ); }
    BOOL   PostMessage( UINT msg, WPARAM wp = 0, LPARAM lp = 0 )  { return ::PostMessageA( m_hWnd, msg, wp, lp ); }
    BOOL   UpdateWindow()                       { return ::UpdateWindow( m_hWnd ); }
    BOOL   SetDlgItemText( int nID, LPCSTR ps ) { return ::SetDlgItemTextA( m_hWnd, nID, ps ); }
    UINT   GetDlgItemText( int nID, LPSTR ps, int nMax ) const { return ::GetDlgItemTextA( m_hWnd, nID, ps, nMax ); }
    int    MessageBox( LPCSTR psz, LPCSTR pTitle = NULL, UINT uType = MB_OK ) { return ::MessageBoxA( m_hWnd, psz, pTitle, uType ); }
    void   SetRedraw( BOOL bRedraw = TRUE )                     { ::SendMessageA( m_hWnd, WM_SETREDRAW, (WPARAM)bRedraw, 0 ); }
    BOOL   MoveWindow( int x, int y, int w, int h, BOOL bRepaint = TRUE ) { return ::MoveWindow( m_hWnd, x, y, w, h, bRepaint ); }
    BOOL   MoveWindow( LPCRECT pr, BOOL bRepaint = TRUE )       { return ::MoveWindow( m_hWnd, pr->left, pr->top, pr->right - pr->left, pr->bottom - pr->top, bRepaint ); }
    int    SetScrollRange( int nBar, int nMin, int nMax, BOOL bRedraw = TRUE ) { return ::SetScrollRange( m_hWnd, nBar, nMin, nMax, bRedraw ); }
    int    SetScrollPos  ( int nBar, int nPos, BOOL bRedraw = TRUE )           { return ::SetScrollPos  ( m_hWnd, nBar, nPos, bRedraw ); }
    int    GetScrollPos  ( int nBar ) const                                    { return ::GetScrollPos  ( m_hWnd, nBar ); }
    void   GetScrollRange( int nBar, LPINT pMin, LPINT pMax ) const            { ::GetScrollRange( m_hWnd, nBar, pMin, pMax ); }
    BOOL   IsWindowEnabled() const                                             { return ::IsWindowEnabled( m_hWnd ); }
    BOOL   SetWindowPos( const CWnd* /*pInsertAfter*/, int x, int y, int cx, int cy, UINT flags )
                                                                               { return ::SetWindowPos( m_hWnd, NULL, x, y, cx, cy, flags ); }
    HWND   SetFocus()                                                          { return ::SetFocus( m_hWnd ); }
    int    GetDlgCtrlID() const                                                { return ::GetDlgCtrlID( m_hWnd ); }
    virtual BOOL Create( LPCSTR /*pszClassName*/, LPCSTR /*pszWindowName*/,
                         DWORD /*dwStyle*/, const RECT& /*rect*/,
                         CWnd* /*pParentWnd*/, UINT /*nID*/, void* /*pContext*/ = NULL )
                                                                               { return FALSE; }
    // Listbox/Button-style Create (dwStyle, rect, pParent, nID)
    virtual BOOL Create( DWORD /*dwStyle*/, const RECT& /*rect*/, CWnd* /*pParentWnd*/, UINT /*nID*/ )
                                                                               { return FALSE; }
    // Static-style Create (pszText, dwStyle, rect, pParent, nID)
    virtual BOOL Create( LPCSTR /*pszText*/, DWORD /*dwStyle*/, const RECT& /*rect*/, CWnd* /*pParentWnd*/, UINT /*nID*/ )
                                                                               { return FALSE; }
    // Standard MFC message handlers as virtuals so derived classes can
    // override and chain to base.
    virtual void OnDestroy()                                                   {}
    virtual void OnPaint()                                                     {}
    virtual void OnSize( UINT /*nType*/, int /*cx*/, int /*cy*/ )              {}
    virtual void OnLButtonDown( UINT /*nFlags*/, POINT /*pt*/ )                {}
    virtual void OnLButtonUp  ( UINT /*nFlags*/, POINT /*pt*/ )                {}
    virtual void OnLButtonDblClk( UINT /*nFlags*/, POINT /*pt*/ )              {}
    virtual void OnRButtonDown( UINT /*nFlags*/, POINT /*pt*/ )                {}
    virtual void OnMouseMove  ( UINT /*nFlags*/, POINT /*pt*/ )                {}
    virtual BOOL OnEraseBkgnd ( CDC* /*pDC*/ )                                 { return FALSE; }
    virtual BOOL OnQueryNewPalette()                                           { return FALSE; }
    virtual void OnPaletteChanged( CWnd* /*pFocusWnd*/ )                       {}

    HWND m_hWnd;
};

//----------------------------- C B u t t o n  / C S c r o l l B a r ---------
// MFC button + scrollbar controls. Live code casts HWND to (CButton*) via
// CWnd::FromHandle. We inherit CWnd so the cast picks up m_hWnd and
// GetSafeHwnd transparently.

//------------------------------ C D i a l o g -------------------------------
// CDialog stub inheriting CWnd so call sites can use CWnd methods
// (SetDlgItemText, UpdateWindow, GetDlgItem) directly. The CDialog-derived
// classes still in the live build (datafile.h's CDlgSelCD plus a handful
// of CDlg* whose .cpp files are excluded) only need the inheritance plus
// stubbed DoModal/EndDialog/UpdateData/Create.

class CDialog : public CWnd
{
public:
    CDialog() {}
    explicit CDialog( UINT /*nIDTemplate*/, void* /*pParent*/ = NULL ) {}
    virtual ~CDialog() {}

    virtual BOOL    OnInitDialog()                { return TRUE; }
    virtual void    OnOK()                        {}
    virtual void    OnCancel()                    {}
    virtual void    DoDataExchange( CDataExchange* /*pDX*/ ) {}
    virtual INT_PTR DoModal()                     { return IDCANCEL; }
    // Wind22's CGlobalSubClassX::CGlobalSubClassX needs `m_dlg.Create(
    // IDD_SUBCLASS)` to produce a non-NULL m_hWnd so `Init()` doesn't
    // throw ERR_SUBCLASS_DLG_CREATE. The subclass machinery does
    // `::GetDlgItem(m_dlg.m_hWnd, IDC_SUBCLASS_BUTTON)` — but only uses
    // the result to compare against the HWND being subclassed, so NULL
    // is fine.
    //
    // We DELIBERATELY don't use ::CreateDialogA from the .RC template:
    // even with ShowWindow(SW_HIDE) immediately after, the dialog (with
    // WS_CAPTION from the template) briefly grabs activation, which
    // breaks input routing to the SDL2 main menu — user-reported
    // mouse-input regression at the main menu.
    //
    // Instead create a 1×1 hidden tool window with WS_EX_NOACTIVATE so
    // it never enters the activation list and never steals foreground
    // from the SDL2 window.
    virtual BOOL Create( UINT /*nIDTemplate*/, CWnd* pParent = NULL )
    {
        m_hWnd = ::CreateWindowExA(
            WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
            "STATIC", "",
            WS_POPUP,  // no WS_VISIBLE, no WS_CAPTION
            0, 0, 1, 1,
            pParent ? pParent->m_hWnd : NULL,
            NULL, ::GetModuleHandleA( NULL ), NULL );
        return m_hWnd != NULL;
    }
    void EndDialog( int /*nResult*/ )             {}
    BOOL UpdateData( BOOL /*bSaveAndValidate*/ = TRUE ) { return TRUE; }
};

class CDialogBar : public CDialog {};

class CButton    : public CWnd
{
public:
    void SetButtonStyle( UINT nStyle, BOOL bRedraw = TRUE )
    {
        ::SendMessageA( m_hWnd, BM_SETSTYLE, (WPARAM)nStyle, (LPARAM)bRedraw );
    }
    UINT GetButtonStyle() const { return (UINT)::GetWindowLongA( m_hWnd, GWL_STYLE ); }
    UINT GetState() const { return (UINT)::SendMessageA( m_hWnd, BM_GETSTATE, 0, 0 ); }
    void SetState( BOOL bChecked ) { ::SendMessageA( m_hWnd, BM_SETSTATE, (WPARAM)bChecked, 0 ); }
    int  GetCheck() const         { return (int)::SendMessageA( m_hWnd, BM_GETCHECK, 0, 0 ); }
    void SetCheck( int nCheck )   { ::SendMessageA( m_hWnd, BM_SETCHECK, (WPARAM)nCheck, 0 ); }
};
class CScrollBar : public CWnd {};
class CListBox : public CWnd
{
public:
    void ResetContent()           { ::SendMessageA( m_hWnd, LB_RESETCONTENT, 0, 0 ); }
    int  AddString( LPCSTR psz )  { return (int)::SendMessageA( m_hWnd, LB_ADDSTRING, 0, (LPARAM)psz ); }
    int  DeleteString( UINT n )   { return (int)::SendMessageA( m_hWnd, LB_DELETESTRING, n, 0 ); }
    int  Delete( UINT n )         { return DeleteString( n ); }
    int  GetCount() const         { return (int)::SendMessageA( m_hWnd, LB_GETCOUNT, 0, 0 ); }
    int  GetCurSel() const        { return (int)::SendMessageA( m_hWnd, LB_GETCURSEL, 0, 0 ); }
    int  SetCurSel( int n )       { return (int)::SendMessageA( m_hWnd, LB_SETCURSEL, n, 0 ); }
    int  SetSel( int n, BOOL bSelect = TRUE ) { return (int)::SendMessageA( m_hWnd, LB_SETSEL, (WPARAM)bSelect, (LPARAM)n ); }
    DWORD_PTR GetItemData( int n ) const      { return (DWORD_PTR)::SendMessageA( m_hWnd, LB_GETITEMDATA, n, 0 ); }
    int  SetItemData( int n, DWORD_PTR data ) { return (int)::SendMessageA( m_hWnd, LB_SETITEMDATA, n, (LPARAM)data ); }
    void* GetItemDataPtr( int n ) const       { return (void*)GetItemData( n ); }
    int  SetItemDataPtr( int n, void* p )     { return SetItemData( n, (DWORD_PTR)p ); }
    int  GetTopIndex() const                  { return (int)::SendMessageA( m_hWnd, LB_GETTOPINDEX, 0, 0 ); }
    int  SetTopIndex( int n )                 { return (int)::SendMessageA( m_hWnd, LB_SETTOPINDEX, n, 0 ); }
    int  GetSelCount() const                  { return (int)::SendMessageA( m_hWnd, LB_GETSELCOUNT, 0, 0 ); }
    int  GetItemHeight( int n = 0 ) const     { return (int)::SendMessageA( m_hWnd, LB_GETITEMHEIGHT, n, 0 ); }
    int  SetItemHeight( int n, UINT h )       { return (int)::SendMessageA( m_hWnd, LB_SETITEMHEIGHT, n, MAKELPARAM( h, 0 ) ); }
    BOOL SetTabStops( int nCount, LPINT pTabs ){ return (BOOL)::SendMessageA( m_hWnd, LB_SETTABSTOPS, (WPARAM)nCount, (LPARAM)pTabs ); }
    int  GetSelItems( int nMaxItems, LPINT pIndexes ) const { return (int)::SendMessageA( m_hWnd, LB_GETSELITEMS, (WPARAM)nMaxItems, (LPARAM)pIndexes ); }
};
class CComboBox : public CWnd
{
public:
    void ResetContent()           { ::SendMessageA( m_hWnd, CB_RESETCONTENT, 0, 0 ); }
    int  AddString( LPCSTR psz )  { return (int)::SendMessageA( m_hWnd, CB_ADDSTRING, 0, (LPARAM)psz ); }
    int  DeleteString( UINT n )   { return (int)::SendMessageA( m_hWnd, CB_DELETESTRING, n, 0 ); }
    int  Delete( UINT n )         { return DeleteString( n ); }
    int  GetCount() const         { return (int)::SendMessageA( m_hWnd, CB_GETCOUNT, 0, 0 ); }
    int  GetCurSel() const        { return (int)::SendMessageA( m_hWnd, CB_GETCURSEL, 0, 0 ); }
    int  SetCurSel( int n )       { return (int)::SendMessageA( m_hWnd, CB_SETCURSEL, n, 0 ); }
};
class CEdit      : public CWnd {};
class CStatic    : public CWnd {};

//----------------------------- C F i l e D i a l o g ------------------------
// MFC's file-open/save common dialog. Live wind22 code instantiates it for
// "select-CD" prompts. Under gate-on we never actually show MFC dialogs —
// the SDL2 file browser replaced these. Stub just makes the call sites
// compile; DoModal returns IDCANCEL.

class CFileDialog
{
public:
    CFileDialog( BOOL /*bOpenFileDialog*/, LPCSTR /*pszDefExt*/ = NULL,
                 LPCSTR /*pszFileName*/ = NULL, DWORD /*dwFlags*/ = 0,
                 LPCSTR /*pszFilter*/ = NULL, void* /*pParent*/ = NULL ) {}
    INT_PTR DoModal()        { return IDCANCEL; }
    CString GetPathName()    const { return CString(); }
    CString GetFileName()    const { return CString(); }
    CString GetFileExt()     const { return CString(); }
    CString GetFileTitle()   const { return CString(); }
};

//----------------------------- C S t d i o F i l e --------------------------
// CFile variant that uses C runtime FILE*. Live code uses it only as a
// drop-in for CFile (no fgets/fputs-style usage). Inherit from CFile.

class CStdioFile : public CFile {};

//----------------------------- C M e m F i l e ------------------------------
// MFC's in-memory CFile. Backed by std::vector<BYTE>. Supports Read/Write/
// Seek/GetPosition/GetLength matching CFile's interface so callers can
// treat it polymorphically.

class CMemFile : public CFile
{
public:
    CMemFile( UINT /*nGrowBytes*/ = 1024 ) : m_pos( 0 ) {}
    ~CMemFile() override {}

    void Close() override { m_buf.clear(); m_pos = 0; }
    UINT Read( void* p, UINT n ) override
    {
        if ( !p || n == 0 || m_pos >= m_buf.size() ) return 0;
        UINT got = (UINT)std::min<size_t>( n, m_buf.size() - m_pos );
        std::memcpy( p, m_buf.data() + m_pos, got );
        m_pos += got;
        return got;
    }
    void Write( const void* p, UINT n ) override
    {
        if ( !p || n == 0 ) return;
        if ( m_pos + n > m_buf.size() ) m_buf.resize( m_pos + n );
        std::memcpy( m_buf.data() + m_pos, p, n );
        m_pos += n;
    }
    LONG Seek( LONG lOff, UINT nFrom ) override
    {
        size_t target = (size_t)lOff;
        if ( nFrom == current ) target = m_pos + lOff;
        else if ( nFrom == end ) target = m_buf.size() + lOff;
        m_pos = target;
        return (LONG)m_pos;
    }
    DWORD GetPosition() const override { return (DWORD)m_pos; }
    DWORD GetLength()   const override { return (DWORD)m_buf.size(); }
    void  Flush() override {}

    BYTE* Detach( UINT* pNewLen = NULL ) {
        if ( pNewLen ) *pNewLen = (UINT)m_buf.size();
        // Caller takes ownership of the bytes — we malloc and copy so the
        // free() / LocalFree() pattern MFC documents still works.
        BYTE* out = (BYTE*)std::malloc( m_buf.size() );
        if ( out ) std::memcpy( out, m_buf.data(), m_buf.size() );
        m_buf.clear();
        m_pos = 0;
        return out;
    }
    void Attach( BYTE* pBuf, UINT n, UINT /*nGrow*/ = 0 ) {
        m_buf.assign( pBuf, pBuf + n );
        m_pos = 0;
    }

private:
    std::vector<BYTE> m_buf;
    size_t            m_pos;
};

//----------------------------- C F i l e S t a t u s ------------------------
// File metadata struct. Live code populates one via CFile::GetStatus, then
// reads m_size / m_mtime / m_szFullName. Stub it as POD.

struct CFileStatus
{
    FILETIME m_ctime{};
    FILETIME m_mtime{};
    FILETIME m_atime{};
    LONGLONG m_size = 0;
    BYTE     m_attribute = 0;
    BYTE     _m_padding = 0;
    char     m_szFullName[260] = {0};
};

inline BOOL CFile::GetStatus( LPCSTR pszFileName, CFileStatus& status )
{
    WIN32_FILE_ATTRIBUTE_DATA fad{};
    if ( !::GetFileAttributesExA( pszFileName, GetFileExInfoStandard, &fad ) )
        return FALSE;
    status.m_ctime = fad.ftCreationTime;
    status.m_atime = fad.ftLastAccessTime;
    status.m_mtime = fad.ftLastWriteTime;
    status.m_size  = ( (LONGLONG)fad.nFileSizeHigh << 32 ) | fad.nFileSizeLow;
    status.m_attribute = (BYTE)( fad.dwFileAttributes & 0xFF );
    std::strncpy( status.m_szFullName, pszFileName, sizeof( status.m_szFullName ) - 1 );
    return TRUE;
}

//------------------------------ C O b L i s t / C P t r L i s t -------------
// MFC's named list typedefs.
typedef CList<CObject*, CObject*> CObList;
typedef CList<void*,    void*>    CPtrList;
typedef CList<CString,  LPCSTR>   CStringList;

// MFC named typedefs for primitive CArray instantiations.
typedef CArray<WORD,  WORD>  CWordArray;
typedef CArray<DWORD, DWORD> CDWordArray;
typedef CArray<BYTE,  BYTE>  CByteArray;
typedef CArray<int,   int>   CUIntArray;
typedef CArray<void*, void*> CPtrArray;

//------------------------ C A r c h i v e E x c e p t i o n -----------------
// Same shape as CFileException; CArchive throws it on serialization errors.
// Live save/load just checks m_cause for logging.

class CArchiveException : public CException
{
public:
    enum { none = 0, generic = 1, readOnly = 2, endOfFile = 3, writeOnly = 4,
           badIndex = 5, badClass = 6, badSchema = 7 };
    int m_cause = none;
};

//----------------------------- M I S C   M A C R O S ------------------------
// DEBUG_NEW: MFC's debug allocator macro. Maps to plain new under the gate.
#ifndef DEBUG_NEW
#define DEBUG_NEW new
#endif

// Message-map macros: MFC's wiring for WM_* handlers. wndstub.h already
// defines a stronger set for files that include it; this just makes sure
// headers that declare DECLARE_MESSAGE_MAP/AFX_MSG/AFX_DATA without including
// wndstub.h (e.g. datafile.h's CDlgSelCD) still parse cleanly.
#ifndef DECLARE_MESSAGE_MAP
#define DECLARE_MESSAGE_MAP()
#endif
#ifndef AFX_MSG
#define AFX_MSG
#endif
#ifndef afx_msg
#define afx_msg
#endif
// _T / TEXT: TCHAR helpers. We always use ANSI here so they're pass-through.
#ifndef _T
#define _T(x) x
#endif
#ifndef TEXT
#define TEXT(x) x
#endif
#ifndef _tcsdup
#define _tcsdup _strdup
#endif
// DDX_* / message-map entry macros. The dialog data-exchange and message-map
// bodies are dead at runtime under the gate (SDL2 replaced the dialogs); the
// macros just need to be parseable. Wrap as no-op statements.
#ifndef DDX_Text
#define DDX_Text(pDX, nIDC, value)        ((void)0)
#endif
#ifndef DDX_Control
#define DDX_Control(pDX, nIDC, var)       ((void)0)
#endif
#ifndef DDX_Check
#define DDX_Check(pDX, nIDC, value)       ((void)0)
#endif
#ifndef DDX_Radio
#define DDX_Radio(pDX, nIDC, value)       ((void)0)
#endif
#ifndef BEGIN_MESSAGE_MAP
#define BEGIN_MESSAGE_MAP(theClass, baseClass) \
    namespace { struct theClass##_msg_dummy {
#endif
#ifndef END_MESSAGE_MAP
#define END_MESSAGE_MAP() }; }
#endif
#ifndef ON_BN_CLICKED
#define ON_BN_CLICKED(id, memberFn)       int _##memberFn##_##id = 0;
#endif
#ifndef ON_COMMAND
#define ON_COMMAND(id, memberFn)          int _##memberFn##_##id = 0;
#endif
#ifndef ON_WM_PAINT
#define ON_WM_PAINT()                     int _on_paint = 0;
#endif
#ifndef ON_MESSAGE
#define ON_MESSAGE(msg, memberFn)         int _##memberFn##_msg = 0;
#endif
#ifndef ON_NOTIFY
#define ON_NOTIFY(code, id, memberFn)     int _##memberFn##_##id = 0;
#endif
// MFC exception macros. The live save/load paths use TRY/CATCH wrappers to
// catch CFileException / CArchiveException — map them onto C++ try/catch.
#ifndef TRY
#define TRY                             try
#endif
#ifndef CATCH
#define CATCH(eType, eName)             catch ( eType* eName )
#endif
#ifndef AND_CATCH
#define AND_CATCH(eType, eName)         catch ( eType* eName )
#endif
#ifndef END_CATCH
#define END_CATCH
#endif
#ifndef THROW
#define THROW(e)                        throw (e)
#endif
#ifndef THROW_LAST
#define THROW_LAST()                    throw
#endif
// AFXAPI/AFX_CDECL: MFC calling-convention markers. Empty under the gate.
#ifndef AFXAPI
#define AFXAPI __stdcall
#endif
#ifndef AFX_CDECL
#define AFX_CDECL __cdecl
#endif
// SerializeElements/ConstructElements/DestructElements: CArray template
// hooks. CArray<TYPE,ARG_TYPE>::SetSize calls these unqualified for
// growing/shrinking; per-type overloads (e.g. music.cpp's CRawData) are
// picked up via ADL at instantiation. The default templates handle the
// trivial cases — placement new for construct, explicit dtor for destruct.
//
// Calling-convention note: no AFXAPI / __stdcall here. Live code's
// per-type overloads use the default __cdecl convention; matching it
// avoids overload-resolution ambiguity at instantiation.
template<class T> inline void SerializeElements( CArchive& /*ar*/, T* /*pElements*/, int /*nCount*/ ) {}
template<class T> inline void ConstructElements( T* pElements, int nCount )
{
    for ( int i = 0; i < nCount; ++i ) new ( pElements + i ) T();
}
template<class T> inline void DestructElements( T* pElements, int nCount )
{
    for ( int i = 0; i < nCount; ++i ) ( pElements + i )->~T();
}

// MFC's runtime-class / dynamic-creation / serialization framework. Live code
// uses DECLARE_SERIAL/IMPLEMENT_SERIAL on a handful of classes to support
// MFC's CArchive object-graph serialization, but our CArchive stub doesn't
// implement that protocol; the macros are no-ops here.
#ifndef DECLARE_DYNAMIC
#define DECLARE_DYNAMIC(class_name)
#endif
#ifndef DECLARE_DYNCREATE
#define DECLARE_DYNCREATE(class_name)
#endif
#ifndef DECLARE_SERIAL
#define DECLARE_SERIAL(class_name)
#endif
#ifndef IMPLEMENT_DYNAMIC
#define IMPLEMENT_DYNAMIC(class_name, base_class_name)
#endif
#ifndef IMPLEMENT_DYNCREATE
#define IMPLEMENT_DYNCREATE(class_name, base_class_name)
#endif
#ifndef IMPLEMENT_SERIAL
#define IMPLEMENT_SERIAL(class_name, base_class_name, wSchema)
#endif
#ifndef RUNTIME_CLASS
#define RUNTIME_CLASS(class_name) (NULL)
#endif

//--------------------------- M F C  H e l p e r s ---------------------------

// AfxIsValidAddress — simple pointer validation (replaces MFC's version)
inline BOOL AfxIsValidAddress( const void* p, UINT_PTR nBytes, BOOL bReadWrite = TRUE )
{
    return ( p != NULL );
}

inline BOOL AfxIsValidString( const char* p, int nMaxChars = -1 )
{
    return ( p != NULL );
}

#endif // __AFX_H__

#endif // __MFC_COMPAT_H__
