#ifndef __MFC_COMPAT_H__
#define __MFC_COMPAT_H__

//---------------------------------------------------------------------------
// mfc_compat.h — Drop-in replacements for MFC geometry types
//
// These match MFC's CRect/CPoint/CSize API but don't require afxwin.h.
// CRect inherits from RECT for Win32 API compatibility.
//---------------------------------------------------------------------------

#include <windows.h>

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
