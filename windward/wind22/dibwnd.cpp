#include "dibwnd.h"
//#include "include/ptr.h"

// this is not the original file
// implemented it using claude sonnet 4

BOOL CDIBWnd::Init( HWND hWnd, Ptr<CDIB> const& dib, int cx, int cy ) {
    if ( hWnd == NULL || dib.Value() == NULL) {
        return FALSE;
    }
    m_hWnd = hWnd;
    m_ptrdib = dib;

    // release saved HDC if we have one to prevent leaking (shouldn't be one, but just in case)
    if ( m_hDC != NULL && m_hWnd != NULL )
    {
        ::ReleaseDC( m_hWnd, m_hDC );
        m_hDC = NULL;
    }

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

    // Use cached DC if available (from Init), otherwise acquire temporarily
    HDC hdcWnd = m_hDC;
    BOOL bNeedRelease = FALSE;
    if ( !hdcWnd )
    {
        hdcWnd = ::GetDC( m_hWnd );
        if ( !hdcWnd )
            return;
        bNeedRelease = TRUE;
    }

    // intersect with window client rect
    CRect rcClient;
    ::GetClientRect( m_hWnd, &rcClient );
    rect &= rcClient;
    if ( rect.IsRectEmpty( ) )
    {
        if ( bNeedRelease )
            ::ReleaseDC( m_hWnd, hdcWnd );
        return;
    }

    // make sure palette is in sync
    if ( m_paletteDirty )
    {
        m_ptrdib->SyncPalette( );
        m_paletteDirty = false;
    }

    // blit from DIB to window
    m_ptrdib->BitBlt( hdcWnd, rect, rect.TopLeft( ) );

    if ( bNeedRelease )
        ::ReleaseDC( m_hWnd, hdcWnd );
}

void CDIBWnd::SetDirtyPalette( )
{
    m_paletteDirty = true;
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
}

// --------------------------------------------------
// CDirtyRects
// --------------------------------------------------

#define MAX_DIRTY_RECTS 320

CDirtyRects::CDirtyRects( CDIBWnd* dibwnd )
{
    m_pdibwnd = dibwnd;

    // allocate arrays (simple fixed-size approach)
    m_nRectPaintCur  = 0;
    m_nRectPaintNext = 0;
    m_nRectBlt       = 0;

    // Single allocation for all three arrays - better cache locality and fewer allocations
    // Total: MAX_DIRTY_RECTS * 3 CRects in one contiguous block
    CRect* pAllRects = new CRect[MAX_DIRTY_RECTS * 3];
    m_prectPaintCur  = pAllRects;
    m_prectPaintNext = pAllRects + MAX_DIRTY_RECTS;
    m_prectBlt       = pAllRects + MAX_DIRTY_RECTS * 2;
}

CDirtyRects::~CDirtyRects( )
{
    // Single allocation was made in constructor, only delete the base pointer
    delete[] m_prectPaintCur;
    // m_prectPaintNext and m_prectBlt point into the same allocation - don't delete them

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
    // overwrite current using memcpy for efficiency (CRect is trivially copyable)
    if ( copyCount > 0 )
        memcpy( m_prectPaintCur, m_prectPaintNext, copyCount * sizeof( CRect ) );
    m_nRectPaintCur = copyCount;
    // clear next
    m_nRectPaintNext = 0;
}

/* void CDirtyRects::BltRects( )
{
    if ( !m_pdibwnd || !m_pdibwnd->GetDIB( ) || !m_pdibwnd->GetHWND( ) )
        return;

    HDC hdcWnd = ::GetDC( m_pdibwnd->GetHWND( ) );
    if ( !hdcWnd )
        return;

    // m_pdibwnd->GetDIB( )->SyncPalette( );

    for ( int i = 0; i < m_nRectBlt; ++i )
    {
        CRect& r = m_prectBlt[i];
        m_pdibwnd->GetDIB( )->BitBlt( hdcWnd, r, r.TopLeft( ) );
    }

    ::ReleaseDC( m_pdibwnd->GetHWND( ), hdcWnd );
    m_nRectBlt = 0;
}*/

void CDirtyRects::BltRects( )
{
    if ( !m_pdibwnd || !m_pdibwnd->GetDIB( ) || !m_pdibwnd->GetHWND( ) )
        return;

    // Early exit if nothing to blit
    if ( m_nRectBlt == 0 )
        return;

    // Cache these outside the loop to avoid repeated accessor calls
    HWND hWnd = m_pdibwnd->GetHWND( );
    CDIB* pdib = m_pdibwnd->GetDIB( );

    HDC hdcWnd = ::GetDC( hWnd );
    if ( !hdcWnd )
        return;

    // ⚠️ CRITICAL OPTIMIZATION: Coalesce adjacent rects BEFORE blitting
    // This reduces the number of BitBlt calls significantly
    CoalesceRects( m_prectBlt, m_nRectBlt );

// Optional: Sort rects by vertical position for cache coherency
// (This helps if you're reading from system RAM -> VRAM)
#ifdef OPTIMIZE_BLIT_ORDER
    SortRectsByPosition( m_prectBlt, m_nRectBlt );
#endif

#ifdef _DEBUG
    CRect rcClient;
    ::GetClientRect( hWnd, &rcClient );
#endif

    // Now blit all the (coalesced) rectangles
    for ( int i = 0; i < m_nRectBlt; ++i )
    {
        CRect& r = m_prectBlt[i];

// Optional: Validate rect is within window bounds
#ifdef _DEBUG
        CRect rcClipped;
        if ( !rcClipped.IntersectRect( &r, &rcClient ) )
        {
            OutputDebugStringA( "CDirtyRects::BltRects: Rect outside window bounds!\n" );
            continue;
        }
#endif

        pdib->BitBlt( hdcWnd, r, r.TopLeft( ) );
    }

    ::ReleaseDC( hWnd, hdcWnd );
    m_nRectBlt = 0;
}

#ifdef OPTIMIZE_BLIT_ORDER
void CDirtyRects::SortRectsByPosition( CRect arect[], int nCount )
{
    // Simple insertion sort (good enough for small arrays like 320 rects)
    for ( int i = 1; i < nCount; ++i )
    {
        CRect key = arect[i];
        int   j   = i - 1;

        // Sort by top-to-bottom, then left-to-right
        while ( j >= 0 && ( arect[j].top > key.top || ( arect[j].top == key.top && arect[j].left > key.left ) ) )
        {
            arect[j + 1] = arect[j];
            j--;
        }
        arect[j + 1] = key;
    }
}
#endif

void CDirtyRects::CoalesceRects( CRect arect[], int& nCount )
{
    if ( nCount <= 1 )
        return;

    // Use the same merge logic as AddRect, but in batch mode
    int writeIdx = 0;

    for ( int i = 0; i < nCount; ++i )
    {
        CRect newRect    = arect[i];
        CRect searchRect = newRect;
        searchRect.InflateRect( 1, 1 );

        // Try to merge with existing rects in the output array
        bool merged = false;
        for ( int j = 0; j < writeIdx; ++j )
        {
            CRect intersection;
            if ( intersection.IntersectRect( &arect[j], &searchRect ) )
            {
                arect[j].UnionRect( &arect[j], &newRect );
                merged = true;
                break;  // Only merge with first match to avoid O(n²) behavior
            }
        }

        // If not merged, add as new rect
        if ( !merged )
        {
            arect[writeIdx++] = newRect;
        }
    }

    nCount = writeIdx;
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


// Optimized AddRect with improved merging logic
// Uses single-pass merge with limited re-scan to reduce worst-case complexity
inline void CDirtyRects::AddRect( const CRect* prect, CRect arect[], int& nCount )
{
    // 1. Basic Validation
    if ( !prect || !arect )
        return;

    // Handle empty or NULL rect input - invalidate entire window
    if ( prect->IsRectEmpty() || prect->IsRectNull() )
        return;

    // 2. HARD LIMIT: Panic mode if full
    if ( nCount >= MAX_DIRTY_RECTS )
    {
        // Safety: Handle 0-size limit case to prevent uninitialized read
        if ( nCount == 0 )
        {
            arect[0] = *prect;
            nCount   = 1;
            return;
        }

        // Collapse all existing into arect[0]
        for ( int i = 1; i < nCount; ++i )
        {
            arect[0].UnionRect( &arect[0], &arect[i] );
        }

        // Merge the new one
        arect[0].UnionRect( &arect[0], prect );
        nCount = 1;

// Log this critical warning in Debug mode
#ifdef _DEBUG
        OutputDebugStringA( "CDirtyRects: Hit MAX_DIRTY_RECTS limit! Performance degradation likely.\n" );
#endif
        return;
    }

    // 3. OPTIMIZED MERGE LOGIC
    // Limit re-scans to avoid O(n²) worst case with many overlapping rects
    CRect newRect    = *prect;
    CRect searchRect = newRect;
    searchRect.InflateRect( 1, 1 );

    // Track how many merges to limit cascading
    int mergeCount = 0;
    const int MAX_MERGE_PASSES = 3;  // Limit cascading merges

    for ( int i = 0; i < nCount; ++i )
    {
        CRect intersection;
        if ( intersection.IntersectRect( &arect[i], &searchRect ) )
        {
            newRect.UnionRect( &newRect, &arect[i] );
            searchRect = newRect;
            searchRect.InflateRect( 1, 1 );

            // Remove current by swapping with last
            arect[i] = arect[nCount - 1];
            nCount--;

            // Only re-check if we haven't done too many merge passes
            if ( ++mergeCount < MAX_MERGE_PASSES )
                i--;  // Re-check this index
            // Otherwise continue forward to avoid excessive rescanning
        }
    }

    // 4. Final Insert
    arect[nCount++] = newRect;
}
