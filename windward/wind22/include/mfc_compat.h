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
#include <string>

#ifndef __AFX_H__  // Only define if MFC is not included

//------------------------------- C P o i n t --------------------------------

class CPoint : public POINT
{
public:
    CPoint() { x = 0; y = 0; }
    CPoint( int _x, int _y ) { x = _x; y = _y; }
    CPoint( POINT pt ) { x = pt.x; y = pt.y; }
    CPoint( SIZE sz ) { x = sz.cx; y = sz.cy; }

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
    CRect( POINT pt, SIZE sz ) { left = pt.x; top = pt.y; right = pt.x + sz.cx; bottom = pt.y + sz.cy; }
    CRect( POINT topLeft, POINT bottomRight ) { left = topLeft.x; top = topLeft.y; right = bottomRight.x; bottom = bottomRight.y; }

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
    void ReleaseBuffer( int nNewLength = -1 ) {
        if ( nNewLength < 0 ) nNewLength = (int)std::strlen( m_str.c_str() );
        m_str.resize( (size_t)nNewLength );
    }

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

//------------------------------ C O b j e c t -------------------------------
// Phase 1g step 2: minimal CObject stub. The cai*.hpp AI classes inherit
// CObject for legacy reasons but don't use serialization, runtime class info,
// or IsKindOf in the live code paths. A virtual dtor is enough.

class CObject
{
public:
    virtual ~CObject() {}

protected:
    CObject() {}
    CObject( const CObject& ) {}
    CObject& operator=( const CObject& ) { return *this; }
};

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
