#include "dibwnd.h"
//#include "include/ptr.h"

// this is not the original file
// it cant be

BOOL CDIBWnd::Init( HWND hWnd, Ptr<CDIB> const& dib, int cx, int cy ) {
    if ( hWnd == NULL || dib.Value() == NULL) {
        return FALSE;
    }
    m_hWnd = hWnd;
    m_ptrdib = dib;

    // attempt to get a DC for the window - store it for quick use (if available)
    HDC hdc = ::GetDC( m_hWnd );
    if ( hdc != NULL )
    {
        // create a compatible DC to keep around (optional). We'll keep the window DC
        // stored so Paint() / Update() can use it. Note: keeping a window DC across
        // lifetime is acceptable here since original header stored one.
        m_hDC = hdc;
    }

    if (cx > 0 && cy > 0)
    {
        m_ptrdib->Resize( cx, cy );
        m_iWinWid = cx;
        m_iWinHt  = cy;
    }

    return TRUE;
}

void CDIBWnd::Exit( )
{
    // release saved HDC if we have one
    if ( m_hDC != NULL && m_hWnd != NULL )
    {
        ::ReleaseDC( m_hWnd, m_hDC );
        m_hDC = NULL;
    }

    // release DIB smart pointer
    m_ptrdib = Ptr<CDIB>( );

    // release DirectDraw clipper pointer if present
    if ( m_pddclipper )
    {
        m_pddclipper->Release( );
        m_pddclipper = nullptr;
    }
}

BOOL CDIBWnd::Size( LPARAM lParam ){
    int width  = LOWORD( lParam );  // same as "right"
    int height = HIWORD( lParam );  // same as "bottom"
    return Size(width, height );
}

BOOL CDIBWnd::Size( int cx, int cy )
{
    if ( !m_ptrdib.Value( ) )
        return FALSE;
    if ( cx <= 0 || cy <= 0 )
        return FALSE;

    m_iWinWid = cx;
    m_iWinHt  = cy;

    return m_ptrdib->Resize( cx, cy );
}

void CDIBWnd::Paint( CRect rect )
{
    ::ValidateRect( m_hWnd, &rect );

    if ( m_hWnd == NULL || m_ptrdib.Value( ) == NULL )
        return;

    HDC hdcWnd = ::GetDC( m_hWnd );
    if ( !hdcWnd )
        return;

    // intersect with window client rect
    CRect rcClient;
    ::GetClientRect( m_hWnd, &rcClient );
    rect &= rcClient;
    if ( rect.IsRectEmpty( ) )
    {
        ::ReleaseDC( m_hWnd, hdcWnd );
        return;
    }

    // make sure palette is in sync
    m_ptrdib->SyncPalette( );

    // blit from DIB to window
    m_ptrdib->BitBlt( hdcWnd, rect, rect.TopLeft( ) );

    ::ReleaseDC( m_hWnd, hdcWnd );
}


void CDIBWnd::Invalidate( RECT const* pRect ) const
{
    if ( m_hWnd == NULL )
        return;

    if ( pRect == NULL )
    {
        ::InvalidateRect( m_hWnd, NULL, FALSE );
    }
    else
    {
        ::InvalidateRect( m_hWnd, pRect, FALSE );
    }
}

void CDIBWnd::Invalidate( int iLeft, int iTop, int iRight, int iBottom ) const
{
    if ( m_hWnd == NULL )
        return;
    RECT rect = { iLeft, iTop, iRight, iBottom };
    ::InvalidateRect( m_hWnd, &rect, FALSE );
}

void CDIBWnd::Update() const {

    if ( m_hWnd == NULL )
        return;
    ::UpdateWindow( m_hWnd );
}

#ifdef _DEBUG
void CDIBWnd::AssertValid( ) const
{
    // Perform light checks equivalent to MFC-style AssertValid
    // In debug builds we ensure stored pointers make sense.
    ASSERT( m_hWnd != NULL );
    ASSERT( m_ptrdib.Value( ) != NULL );
}
#endif

void CDIBWnd::ctor( )
{
    m_hWnd       = NULL;
    m_hDC        = NULL;
    m_iWinWid    = 0;
    m_iWinHt     = 0;
    m_ptrdib     = Ptr<CDIB>( );  // empty
    m_hRes       = S_OK;
    m_pddclipper = nullptr;
}

// --------------------------------------------------
// CDirtyRects
// --------------------------------------------------

#define MAX_DIRTY_RECTS 256

CDirtyRects::CDirtyRects( CDIBWnd* dibwnd )
{
    m_pdibwnd = dibwnd;

    // allocate arrays (simple fixed-size approach)
    m_nRectPaintCur  = 0;
    m_nRectPaintNext = 0;
    m_nRectBlt       = 0;

    m_prectPaintCur  = new CRect[MAX_DIRTY_RECTS];
    m_prectPaintNext = new CRect[MAX_DIRTY_RECTS];
    m_prectBlt       = new CRect[MAX_DIRTY_RECTS];
}

CDirtyRects::~CDirtyRects( )
{
    delete[] m_prectPaintCur;
    delete[] m_prectPaintNext;
    delete[] m_prectBlt;

    m_prectPaintCur  = nullptr;
    m_prectPaintNext = nullptr;
    m_prectBlt       = nullptr;
    m_nRectPaintCur = m_nRectPaintNext = m_nRectBlt = 0;
}

void CDirtyRects::UpdateLists( )
{
    // Move next list to current list and clear next
    // We'll copy next array into cur array (coalescing not applied here).

    int copyCount = (m_nRectPaintNext < MAX_DIRTY_RECTS) ? m_nRectPaintNext : MAX_DIRTY_RECTS;
    // overwrite current
    for ( int i = 0; i < copyCount; ++i )
    {
        m_prectPaintCur[i] = m_prectPaintNext[i];
    }
    m_nRectPaintCur = copyCount;
    // clear next
    m_nRectPaintNext = 0;
}

void CDirtyRects::BltRects( )
{
    if ( !m_pdibwnd || !m_pdibwnd->GetDIB( ) || !m_pdibwnd->GetHWND( ) )
        return;

    HDC hdcWnd = ::GetDC( m_pdibwnd->GetHWND( ) );
    if ( !hdcWnd )
        return;

    m_pdibwnd->GetDIB( )->SyncPalette( );

    for ( int i = 0; i < m_nRectBlt; ++i )
    {
        CRect& r = m_prectBlt[i];
        m_pdibwnd->GetDIB( )->BitBlt( hdcWnd, r, r.TopLeft( ) );
    }

    ::ReleaseDC( m_pdibwnd->GetHWND( ), hdcWnd );
    m_nRectBlt = 0;
}


#ifdef _DEBUG
void CDirtyRects::AssertValid( ) const
{
    ASSERT( m_pdibwnd != NULL );
    ASSERT( m_prectPaintCur != NULL );
    ASSERT( m_prectPaintNext != NULL );
    ASSERT( m_prectBlt != NULL );
}
#endif

// Add rect to current or cur/next lists. Coalesce with overlapping rects
void CDirtyRects::AddRect( CRect const* prect, CDirtyRects::RECT_LIST eList )
{
    if ( prect == NULL )
        return;

    switch ( eList )
    {
    case RECT_LIST::LIST_PAINT_CUR:
        AddRect( prect, m_prectPaintCur, m_nRectPaintCur );
        break;
    case RECT_LIST::LIST_PAINT_NEXT:
        AddRect( prect, m_prectPaintNext, m_nRectPaintNext );
        break;
    case RECT_LIST::LIST_PAINT_BOTH:
        AddRect( prect, m_prectPaintCur, m_nRectPaintCur );
        AddRect( prect, m_prectPaintNext, m_nRectPaintNext );
        break;
    case RECT_LIST::LIST_BLT:
        AddRect( prect, m_prectBlt, m_nRectBlt );
        break;
    }
}

void CDirtyRects::AddRect( CRect const* prect, CRect arect[], int& nCount )
{
    if ( prect == NULL || arect == nullptr )
        return;
    if ( nCount >= MAX_DIRTY_RECTS )
    {
        // too many rects; coalesce to single union rect
        CRect unionRect = arect[0];
        for ( int i = 1; i < nCount; ++i ) unionRect |= arect[i];
        unionRect |= *prect;
        arect[0] = unionRect;
        nCount   = 1;
        return;
    }

    // Try to coalesce with an existing rect that intersects or is adjacent
    for ( int i = 0; i < nCount; ++i )
    {
        CRect r = arect[i];
        // If intersects or touches, merge
        CRect intersect;
        if ( intersect.IntersectRect( r, *prect ) || r.left <= prect->right + 1 && r.right + 1 >= prect->left &&
                                                         r.top <= prect->bottom + 1 && r.bottom + 1 >= prect->top )
        {
            arect[i] |= *prect;  // union
            return;
        }
    }

    // otherwise append
    arect[nCount++] = *prect;
}