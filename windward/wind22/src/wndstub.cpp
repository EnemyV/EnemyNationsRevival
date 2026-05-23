//---------------------------------------------------------------------------
//
// wndstub.cpp — Implementation of the non-MFC HWND wrapper.
//
// Skeleton committed 2026-05-17 alongside wndstub.h. NOT WIRED IN YET —
// CWndBase still inherits from CWnd. When ENATIONS_USE_STUB_WND lands,
// this file becomes the runtime back-end for all wind22 window classes.
//
// The implementation is intentionally thin: Win32 wrappers, a static
// WndProc + GWLP_USERDATA stash, and virtual dispatch via WindowProc().
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "wndstub.h"
#include <set>
#include <string>


FNMOUSEMOVE_STUB* CWndStub::sm_fnMouseMove = NULL;


CWndStub::CWndStub()
    : m_hWnd( NULL )
{
}

CWndStub::~CWndStub()
{
    // Note: do NOT auto-destroy the HWND here. Two reasons:
    // 1) Win32 cleanup happens via WM_NCDESTROY in StaticWndProc, which
    //    nulls m_hWnd and calls PostNcDestroy. So m_hWnd is typically
    //    already NULL by the time ~CWndStub runs.
    // 2) Temporary CWndStub objects (like the thread-local one used by
    //    GetParent() for non-stub parents) hold a *borrowed* HWND they
    //    don't own. Destroying it here would tear down somebody else's
    //    window. Callers that want explicit destruction should call
    //    DestroyWindow() before ~CWndStub.
}


//-------------------------- W i n d o w P r o c ----------------------------
//
// The static window proc — one for every CWndStub-derived window. Looks up
// `this` via GWLP_USERDATA (stashed at WM_NCCREATE) and forwards to the
// virtual WindowProc().
//
LRESULT CALLBACK CWndStub::StaticWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    // WM_NCCREATE: lParam is CREATESTRUCT*; lpCreateParams is the `this` we passed.
    if ( msg == WM_NCCREATE )
    {
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
        CWndStub* pSelf = (CWndStub*)pcs->lpCreateParams;
        if ( pSelf != NULL )
        {
            pSelf->m_hWnd = hwnd;
            ::SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pSelf );
        }
        return ::DefWindowProc( hwnd, msg, wParam, lParam );
    }

    CWndStub* pSelf = (CWndStub*)::GetWindowLongPtr( hwnd, GWLP_USERDATA );
    if ( pSelf == NULL )
        return ::DefWindowProc( hwnd, msg, wParam, lParam );

    LRESULT result = pSelf->WindowProc( msg, wParam, lParam );

    // WM_NCDESTROY: window is fully gone; CWnd's pattern is to PostNcDestroy
    // (typically `delete this`) and then null m_hWnd.
    if ( msg == WM_NCDESTROY )
    {
        ::SetWindowLongPtr( hwnd, GWLP_USERDATA, 0 );
        pSelf->m_hWnd = NULL;
        pSelf->PostNcDestroy();
    }
    return result;
}


//---------------------------- W i n d o w P r o c --------------------------
//
// Virtual dispatcher. Routes WM_* messages to the matching virtual handler.
// For messages the base class doesn't know, falls through to DefWindowProc.
// Derived classes that need custom messages (ON_MESSAGE) override this and
// switch on those values, calling CWndStub::WindowProc as the default.
//
LRESULT CWndStub::WindowProc( UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
        case WM_CREATE:
            return OnCreate( (LPCREATESTRUCT)lParam );
        case WM_DESTROY:
            OnDestroy();
            return 0;
        case WM_PAINT: {
            // Default: route through OnPaint but DON'T validate the update region
            // here — derived class either uses CPaintDC pattern or calls
            // ::ValidateRect itself.
            OnPaint();
            return 0;
        }
        case WM_SIZE:
            OnSize( (UINT)wParam, (int)LOWORD( lParam ), (int)HIWORD( lParam ) );
            return 0;
        case WM_ERASEBKGND:
            return OnEraseBkgnd( (HDC)wParam ) ? 1 : 0;
        case WM_MOUSEMOVE: {
            int x = (int)(short)LOWORD( lParam ), y = (int)(short)HIWORD( lParam );
            if ( sm_fnMouseMove != NULL )
                sm_fnMouseMove( this, (UINT)wParam, x, y );
            OnMouseMove( (UINT)wParam, x, y );
            return 0;
        }
        case WM_LBUTTONDOWN:
            OnLButtonDown( (UINT)wParam, (int)(short)LOWORD( lParam ), (int)(short)HIWORD( lParam ) );
            return 0;
        case WM_LBUTTONDBLCLK:
            OnLButtonDblClk( (UINT)wParam, (int)(short)LOWORD( lParam ), (int)(short)HIWORD( lParam ) );
            return 0;
        case WM_RBUTTONDOWN:
            OnRButtonDown( (UINT)wParam, (int)(short)LOWORD( lParam ), (int)(short)HIWORD( lParam ) );
            return 0;
        case WM_MBUTTONDOWN:
            OnMButtonDown( (UINT)wParam, (int)(short)LOWORD( lParam ), (int)(short)HIWORD( lParam ) );
            return 0;
        case WM_PALETTECHANGED:
            OnPaletteChanged( wParam ? CWnd::FromHandle( (HWND)wParam ) : (CWnd*)NULL );
            return 0;
        case WM_QUERYNEWPALETTE:
            return OnQueryNewPalette() ? 1 : 0;
        case WM_KEYDOWN:
            OnKeyDown( (UINT)wParam, (UINT)LOWORD( lParam ), (UINT)HIWORD( lParam ) );
            return 0;
        case WM_CHAR:
            OnChar( (UINT)wParam, (UINT)LOWORD( lParam ), (UINT)HIWORD( lParam ) );
            return 0;
        case WM_TIMER:
            OnTimer( (UINT_PTR)wParam );
            return 0;
        case WM_ACTIVATEAPP:
            OnActivateApp( (BOOL)wParam, (DWORD)lParam );
            return 0;
        case WM_QUERYENDSESSION:
            return OnQueryEndSession() ? 1 : 0;
        case WM_CLOSE:
            OnClose();
            return 0;
        case WM_SYSCOMMAND:
            OnSysCommand( (UINT)wParam, lParam );
            return ::DefWindowProc( m_hWnd, msg, wParam, lParam );
        case WM_SETFOCUS:
            OnSetFocus( (HWND)wParam );
            return 0;
        case WM_KILLFOCUS:
            OnKillFocus( (HWND)wParam );
            return 0;
        case WM_MOVE:
            OnMove( (int)(short)LOWORD( lParam ), (int)(short)HIWORD( lParam ) );
            return 0;
        case WM_GETMINMAXINFO:
            OnGetMinMaxInfo( (MINMAXINFO*)lParam );
            return 0;
        case WM_COMMAND:
            if ( OnCommand( wParam, lParam ) )
                return 0;
            break;
        case WM_NOTIFY: {
            LRESULT result = 0;
            if ( OnNotify( wParam, lParam, &result ) )
                return result;
            break;
        }
        case WM_HSCROLL:
            // Use MFC-typed virtual so game-side overrides taking CScrollBar*
            // actually dispatch. FromHandle on lParam returns NULL for scrollbar
            // notifications that aren't from a control (in which case lParam=0).
            OnHScroll( (UINT)LOWORD( wParam ), (UINT)HIWORD( wParam ),
                       lParam ? (CScrollBar*)CWnd::FromHandle( (HWND)lParam ) : (CScrollBar*)NULL );
            return 0;
        case WM_VSCROLL:
            OnVScroll( (UINT)LOWORD( wParam ), (UINT)HIWORD( wParam ),
                       lParam ? (CScrollBar*)CWnd::FromHandle( (HWND)lParam ) : (CScrollBar*)NULL );
            return 0;
        case WM_SETCURSOR:
            // Game-side OnSetCursor declared with CWnd* signature — use MFC-typed virtual.
            if ( OnSetCursor( wParam ? CWnd::FromHandle( (HWND)wParam ) : (CWnd*)NULL,
                              (UINT)LOWORD( lParam ), (UINT)HIWORD( lParam ) ) )
                return TRUE;
            break;
        case WM_ACTIVATE:
            OnActivate( (UINT)LOWORD( wParam ),
                        lParam ? CWnd::FromHandle( (HWND)lParam ) : (CWnd*)NULL,
                        (BOOL)HIWORD( wParam ) );
            return 0;
        case WM_MOUSEWHEEL: {
            short zDelta = (short)HIWORD( wParam );
            UINT  flags  = (UINT)LOWORD( wParam );
            int   x      = (int)(short)LOWORD( lParam );
            int   y      = (int)(short)HIWORD( lParam );
            if ( OnMouseWheel( flags, zDelta, x, y ) )
                return 0;
            break;
        }
        case WM_KEYUP:
            OnKeyUp( (UINT)wParam, (UINT)LOWORD( lParam ), (UINT)HIWORD( lParam ) );
            return 0;
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC: {
            // Game-side OnCtlColor declared with CDC*/CWnd* — use MFC-typed virtual.
            CDC*  pdc  = wParam ? CDC::FromHandle( (HDC)wParam ) : (CDC*)NULL;
            CWnd* pCtl = lParam ? CWnd::FromHandle( (HWND)lParam ) : (CWnd*)NULL;
            HBRUSH hbr = OnCtlColor( pdc, pCtl, msg - WM_CTLCOLORMSGBOX );
            if ( hbr != NULL )
                return (LRESULT)hbr;
            break;
        }
    }
    return ::DefWindowProc( m_hWnd, msg, wParam, lParam );
}


//--------------------- R e g i s t e r C l a s s O n c e -------------------
//
ATOM CWndStub::RegisterClassOnce( LPCSTR pszName, UINT style,
                                  HCURSOR hCursor, HBRUSH hbrBackground, HICON hIcon )
{
    static std::set<std::string> registered;
    if ( registered.find( pszName ) != registered.end() )
        return (ATOM)1;  // already registered, treat as success

    WNDCLASS wc;
    ZeroMemory( &wc, sizeof( wc ) );
    wc.style         = style;
    wc.lpfnWndProc   = &CWndStub::StaticWndProc;
    wc.hInstance     = ::GetModuleHandle( NULL );
    wc.hCursor       = hCursor;
    wc.hbrBackground = hbrBackground;
    wc.hIcon         = hIcon;
    wc.lpszClassName = pszName;
    ATOM atom = ::RegisterClass( &wc );
    if ( atom != 0 )
        registered.insert( pszName );
    return atom;
}


//----------------------------- C r e a t e E x ----------------------------
//
BOOL CWndStub::CreateEx( DWORD dwExStyle, LPCSTR lpszClassName, LPCSTR lpszWindowName,
                        DWORD dwStyle, int x, int y, int cx, int cy,
                        HWND hwndParent, HMENU hMenu, LPVOID lpParam )
{
    // Pass `this` as the create param so WM_NCCREATE can stash it.
    LPVOID actualParam = ( lpParam != NULL ) ? lpParam : (LPVOID)this;
    HWND hwnd = ::CreateWindowEx( dwExStyle, lpszClassName, lpszWindowName, dwStyle,
                                  x, y, cx, cy, hwndParent, hMenu,
                                  ::GetModuleHandle( NULL ), actualParam );
    if ( hwnd == NULL )
        return FALSE;
    // Set m_hWnd directly. StaticWndProc would also set it during WM_NCCREATE,
    // but only if the window class points at StaticWndProc — many game-side
    // classes register with DefWindowProc (e.g. EnemyNationsMainWindow), in
    // which case WM_NCCREATE never reaches us. Setting it here ensures m_hWnd
    // is valid post-CreateEx regardless of which wndproc the class uses.
    // (If StaticWndProc also fires, it will re-set the same value — harmless.)
    if ( m_hWnd == NULL )
        m_hWnd = hwnd;
    return TRUE;
}

BOOL CWndStub::DestroyWindow()
{
    if ( m_hWnd == NULL ) return FALSE;
    HWND h = m_hWnd;
    m_hWnd = NULL;
    return ::DestroyWindow( h );
}


//------------------------ T r i v i a l   w r a p p e r s -----------------
//
BOOL CWndStub::GetClientRect( RECT* pRect ) const           { return ::GetClientRect( m_hWnd, pRect ); }
BOOL CWndStub::GetWindowRect( RECT* pRect ) const           { return ::GetWindowRect( m_hWnd, pRect ); }
BOOL CWndStub::SetWindowPos( HWND hwndAfter, int x, int y, int cx, int cy, UINT flags )
    { return ::SetWindowPos( m_hWnd, hwndAfter, x, y, cx, cy, flags ); }
BOOL CWndStub::InvalidateRect( const RECT* pRect, BOOL bErase ) const
    { return ::InvalidateRect( m_hWnd, pRect, bErase ); }
BOOL CWndStub::UpdateWindow() const                         { return ::UpdateWindow( m_hWnd ); }
BOOL CWndStub::ShowWindow( int nCmdShow )                   { return ::ShowWindow( m_hWnd, nCmdShow ); }
BOOL CWndStub::EnableWindow( BOOL bEnable )                 { return ::EnableWindow( m_hWnd, bEnable ); }
BOOL CWndStub::MoveWindow( int x, int y, int cx, int cy, BOOL bRepaint )
    { return ::MoveWindow( m_hWnd, x, y, cx, cy, bRepaint ); }
BOOL CWndStub::ValidateRect( const RECT* pRect ) const
    { return ::ValidateRect( m_hWnd, pRect ); }

BOOL CWndStub::ScreenToClient( LPPOINT pt ) const
    { return ::ScreenToClient( m_hWnd, pt ); }
BOOL CWndStub::ScreenToClient( LPRECT  rc ) const
{
    POINT p1 = { rc->left,  rc->top    };
    POINT p2 = { rc->right, rc->bottom };
    BOOL b1 = ::ScreenToClient( m_hWnd, &p1 );
    BOOL b2 = ::ScreenToClient( m_hWnd, &p2 );
    rc->left = p1.x;  rc->top    = p1.y;
    rc->right= p2.x;  rc->bottom = p2.y;
    return b1 && b2;
}
BOOL CWndStub::ClientToScreen( LPPOINT pt ) const
    { return ::ClientToScreen( m_hWnd, pt ); }
BOOL CWndStub::ClientToScreen( LPRECT  rc ) const
{
    POINT p1 = { rc->left,  rc->top    };
    POINT p2 = { rc->right, rc->bottom };
    BOOL b1 = ::ClientToScreen( m_hWnd, &p1 );
    BOOL b2 = ::ClientToScreen( m_hWnd, &p2 );
    rc->left = p1.x;  rc->top    = p1.y;
    rc->right= p2.x;  rc->bottom = p2.y;
    return b1 && b2;
}

BOOL CWndStub::SetWindowPlacement( const WINDOWPLACEMENT* pwp )
    { return ::SetWindowPlacement( m_hWnd, pwp ); }
BOOL CWndStub::GetWindowPlacement( WINDOWPLACEMENT* pwp ) const
    { return ::GetWindowPlacement( m_hWnd, pwp ); }

CWndStub* CWndStub::GetParent() const
{
    HWND hParent = ::GetParent( m_hWnd );
    if ( hParent == NULL ) return NULL;
    CWndStub* p = (CWndStub*)::GetWindowLongPtr( hParent, GWLP_USERDATA );
    if ( p != NULL ) return p;
    // Parent isn't stub-managed (likely an MFC widget or a foreign window).
    // Return a thread-local temp that wraps just the HWND so call patterns
    // like GetParent()->SendMessage(...) still dispatch to the right window.
    thread_local CWndStub s_tempParent;
    s_tempParent.m_hWnd = hParent;
    return &s_tempParent;
}
HWND CWndStub::GetTopLevelParent() const
{
    HWND h = m_hWnd;
    for ( HWND p = ::GetParent( h ); p != NULL; p = ::GetParent( p ) )
        h = p;
    return h;
}
CWndStub* CWndStub::GetDlgItem( int nID ) const
{
    HWND h = ::GetDlgItem( m_hWnd, nID );
    // CRITICAL: must NOT return NULL even if the child doesn't exist yet.
    // Live code uses MFC's GetDlgItem idiom: `pBtn = GetDlgItem(ID);
    // pBtn->EnableWindow(FALSE);` — MFC always returns a non-NULL temp
    // wrapping the (possibly NULL) HWND, so the EnableWindow call becomes
    // a harmless ::EnableWindow(NULL, ...). Caught in area.cpp:4383
    // SetButtonState calling EnableButton(IDC_UNIT_RESUME) during the
    // area window's WM_CREATE — IDC_UNIT_RESUME isn't created yet, our
    // GetDlgItem returned NULL, and the caller AV'd dereferencing it.
    if ( h == NULL ) {
        thread_local CWndStub s_nullItem;
        s_nullItem.m_hWnd = NULL;
        return &s_nullItem;
    }
    // Stub-managed (GWLP_USERDATA was stashed at WM_NCCREATE)?
    CWndStub* p = (CWndStub*)::GetWindowLongPtr( h, GWLP_USERDATA );
    if ( p != NULL ) return p;
    // MFC-managed child (e.g. CBmButton, CScrollBar, CListBox)? CWnd::FromHandle
    // returns the registered MFC CWnd* if it's in MFC's permanent handle map.
    // The caller often does `(CBmButton*)GetDlgItem(ID)` — the cast through
    // CWndStub* is a type lie, but reinterpreting the returned CWnd* as
    // CBmButton* works because CBmButton derives from CWnd, so the cast
    // back to the concrete MFC type lands on the right object.
    CWnd* pMfc = CWnd::FromHandle( h );
    if ( pMfc != NULL )
        return (CWndStub*)pMfc;
    // Last resort: thread-local temp wrapping just the HWND.
    thread_local CWndStub s_tempItem;
    s_tempItem.m_hWnd = h;
    return &s_tempItem;
}
int  CWndStub::GetDlgCtrlID() const                           { return ::GetDlgCtrlID( m_hWnd ); }
HWND CWndStub::FindWindow( LPCSTR lpszClassName, LPCSTR lpszWindowName )
    { return ::FindWindowA( lpszClassName, lpszWindowName ); }

HWND CWndStub::SetFocus()                                   { return ::SetFocus( m_hWnd ); }
HWND CWndStub::SetCapture()                                 { return ::SetCapture( m_hWnd ); }
BOOL CWndStub::ReleaseCapture()                             { return ::ReleaseCapture(); }
HWND CWndStub::SetForegroundWindow()                        { return ::SetForegroundWindow( m_hWnd ) ? m_hWnd : NULL; }
BOOL CWndStub::BringWindowToTop()                           { return ::BringWindowToTop( m_hWnd ); }

CWndStub* CWndStub::SetActiveWindow()
{
    HWND hPrev = ::SetActiveWindow( m_hWnd );
    if ( hPrev == NULL ) return NULL;
    CWndStub* p = (CWndStub*)::GetWindowLongPtr( hPrev, GWLP_USERDATA );
    if ( p != NULL ) return p;
    thread_local CWndStub s_tempPrevActive;
    s_tempPrevActive.m_hWnd = hPrev;
    return &s_tempPrevActive;
}

CWndStub* CWndStub::ChildWindowFromPoint( POINT pt ) const
{
    HWND h = ::ChildWindowFromPoint( m_hWnd, pt );
    if ( h == NULL ) return NULL;
    CWndStub* p = (CWndStub*)::GetWindowLongPtr( h, GWLP_USERDATA );
    if ( p != NULL ) return p;
    thread_local CWndStub s_tempChild;
    s_tempChild.m_hWnd = h;
    return &s_tempChild;
}

UINT_PTR CWndStub::SetTimer( UINT_PTR id, UINT msElapsed, TIMERPROC proc )
    { return ::SetTimer( m_hWnd, id, msElapsed, proc ); }
BOOL CWndStub::KillTimer( UINT_PTR id )                     { return ::KillTimer( m_hWnd, id ); }

HDC  CWndStub::GetDC() const                                { return ::GetDC( m_hWnd ); }
int  CWndStub::ReleaseDC( HDC hdc ) const                   { return ::ReleaseDC( m_hWnd, hdc ); }

LRESULT CWndStub::SendMessage( UINT msg, WPARAM wParam, LPARAM lParam )
    { return ::SendMessage( m_hWnd, msg, wParam, lParam ); }
BOOL    CWndStub::PostMessage( UINT msg, WPARAM wParam, LPARAM lParam )
    { return ::PostMessage( m_hWnd, msg, wParam, lParam ); }

BOOL CWndStub::SetWindowText( LPCSTR psz )                  { return ::SetWindowTextA( m_hWnd, psz ); }
int  CWndStub::GetWindowText( LPSTR psz, int nMaxCount ) const
    { return ::GetWindowTextA( m_hWnd, psz, nMaxCount ); }
int  CWndStub::GetWindowTextLength() const                  { return ::GetWindowTextLengthA( m_hWnd ); }

LONG CWndStub::GetStyle() const                             { return (LONG)::GetWindowLong( m_hWnd, GWL_STYLE ); }
LONG CWndStub::GetExStyle() const                           { return (LONG)::GetWindowLong( m_hWnd, GWL_EXSTYLE ); }
BOOL CWndStub::IsIconic() const                             { return ::IsIconic( m_hWnd ); }
BOOL CWndStub::IsWindowVisible() const                      { return ::IsWindowVisible( m_hWnd ); }
BOOL CWndStub::IsWindowEnabled() const                      { return ::IsWindowEnabled( m_hWnd ); }


//------------------------- S t a t i c   h e l p e r s ---------------------
//
CWndStub* CWndStub::FromHandle( HWND hwnd )
{
    if ( hwnd == NULL ) return NULL;
    return (CWndStub*)::GetWindowLongPtr( hwnd, GWLP_USERDATA );
}

CWndStub* CWndStub::GetActiveWindow()
{
    return FromHandle( ::GetActiveWindow() );
}

void CWndStub::SetFnMouseMove( FNMOUSEMOVE_STUB* fn )
{
    sm_fnMouseMove = fn;
}
