//---------------------------------------------------------------------------
//
// wndstub.h — Non-MFC HWND wrapper, Phase 1 keystone for the wind22 strip.
//
// This is the eventual replacement for inheriting wind22's CWndBase from
// MFC's CWnd. CWndStub provides the surface CWndBase / CWndPrimary /
// CWndAnim and their 15 game-side derived classes actually use, but with
// zero MFC dependency.
//
// STATUS as of 2026-05-17: skeleton only. NOT WIRED IN YET. Default build
// path still has `class CWndBase : public CWnd` (MFC). When the next phase
// is ready to land, define ENATIONS_USE_STUB_WND in the project and flip
// wndbase.h to inherit from CWndStub instead of CWnd.
//
// The CDIBWnd class at wind22/include/dibwnd.h is the template that
// proves this works: it owns an HWND directly, dispatches via a custom
// WindowProc, and never touches MFC.
//
// DESIGN
// ------
// CWndStub:
//   - Owns HWND m_hWnd.
//   - Registers a Win32 WNDCLASS the first time Create() is called for
//     a given class name, with a single static WndProc.
//   - The static WndProc looks up `this` from GWLP_USERDATA (stashed at
//     WM_NCCREATE time) and forwards to virtual WindowProc.
//   - Virtual WindowProc dispatches WM_* to virtual handlers:
//       OnCreate(LPCREATESTRUCT), OnDestroy(), OnPaint(), OnSize(UINT,int,int),
//       OnEraseBkgnd(HDC), OnMouseMove(UINT, int x, int y), OnLButtonDown,
//       OnRButtonDown, OnMButtonDown, OnPaletteChanged(HWND focus),
//       OnQueryNewPalette(), OnKeyDown, OnChar, OnTimer, OnActivateApp,
//       OnQueryEndSession, OnClose, OnSysCommand.
//   - For custom messages (ON_MESSAGE(WM_FOO, OnFoo) in current code),
//     derived classes override virtual WindowProc and handle their own
//     switch cases before falling through to base.
//
// MIGRATION STEPS (to be done in a later session)
// -----------------------------------------------
//   1. Implement CWndStub fully (this file + wndstub.cpp).
//   2. In wndbase.h, add `#ifdef ENATIONS_USE_STUB_WND` to inherit from
//      CWndStub instead of CWnd. Replace `LRESULT WindowProc(UINT, WPARAM, LPARAM)`
//      override to keep its CFramePainter passthrough behavior.
//   3. In wndbase.cpp, redefine BEGIN_MESSAGE_MAP / END_MESSAGE_MAP /
//      ON_WM_* macros to expand to no-ops (since CWndStub uses direct
//      virtual dispatch, not a message map).
//   4. Walk the 15 derived classes (CWndMain, CWndArea, CWndWorld, etc.).
//      For each one that uses `ON_MESSAGE(WM_FOO, OnFoo)`, add a
//      WindowProc override that handles those messages by switch+call.
//      ~30 ON_MESSAGE / ON_COMMAND / ON_NOTIFY entries in main.cpp's
//      CWndMain map alone — those need explicit dispatch.
//   5. Build, fix, iterate.
//   6. Drop CWnd's `Default*` window message handlers (DefWindowProc
//      handles those).
//   7. Remove `#include <afxwin.h>` from places where it's only there
//      for CWnd.
//
//---------------------------------------------------------------------------

#ifndef __WNDSTUB_H__
#define __WNDSTUB_H__

#include <windows.h>

// Forward declarations
class CWndStub;

// Function pointer for the global mouse-move blanking callback that the
// game registers via CWndBase::SetFnMouseMove. Kept on CWndStub so it
// survives the eventual flip.
typedef void ( FNMOUSEMOVE_STUB )( CWndStub* pWnd, UINT nFlags, int x, int y );


class CWndStub
{
public:
    HWND m_hWnd;  // Window handle (NULL until Create succeeds)

    CWndStub();
    virtual ~CWndStub();

    // ----- Creation / destruction -----
    // Win32 wrappers around RegisterClassEx + CreateWindowEx. The derived
    // class supplies the class name; CWndStub registers it lazily and
    // stashes `this` in GWLP_USERDATA at WM_NCCREATE time so the static
    // WndProc can dispatch back to virtual WindowProc.
    BOOL CreateEx( DWORD dwExStyle, LPCSTR lpszClassName, LPCSTR lpszWindowName,
                   DWORD dwStyle, int x, int y, int cx, int cy,
                   HWND hwndParent, HMENU hMenu, LPVOID lpParam );
    BOOL DestroyWindow();

    // ----- Geometry / state -----
    BOOL GetClientRect( RECT* pRect ) const;
    BOOL GetWindowRect( RECT* pRect ) const;
    BOOL SetWindowPos( HWND hwndAfter, int x, int y, int cx, int cy, UINT flags );
    BOOL InvalidateRect( const RECT* pRect, BOOL bErase = TRUE ) const;
    BOOL UpdateWindow() const;
    BOOL ShowWindow( int nCmdShow );
    BOOL EnableWindow( BOOL bEnable = TRUE );
    BOOL MoveWindow( int x, int y, int cx, int cy, BOOL bRepaint = TRUE );

    // ----- Input / focus -----
    HWND SetFocus();
    HWND SetCapture();
    static BOOL ReleaseCapture();
    HWND SetForegroundWindow();
    BOOL BringWindowToTop();
    UINT_PTR SetTimer( UINT_PTR id, UINT msElapsed, TIMERPROC proc = NULL );
    BOOL KillTimer( UINT_PTR id );

    // ----- DC / paint -----
    HDC  GetDC() const;
    int  ReleaseDC( HDC hdc ) const;

    // ----- Messaging -----
    LRESULT SendMessage( UINT msg, WPARAM wParam = 0, LPARAM lParam = 0 );
    BOOL    PostMessage( UINT msg, WPARAM wParam = 0, LPARAM lParam = 0 );

    // ----- Text -----
    BOOL SetWindowText( LPCSTR psz );
    int  GetWindowText( LPSTR psz, int nMaxCount ) const;
    int  GetWindowTextLength() const;

    // ----- Class info / styles -----
    LONG GetStyle() const;
    LONG GetExStyle() const;
    BOOL IsIconic() const;
    BOOL IsWindowVisible() const;
    BOOL IsWindowEnabled() const;

    // ----- Virtual handlers (override in derived classes) -----
    // Default implementations call DefWindowProc.
    virtual LRESULT WindowProc( UINT msg, WPARAM wParam, LPARAM lParam );

    virtual int  OnCreate( LPCREATESTRUCT lpCs )                  { return 0; }
    virtual void OnDestroy()                                      { }
    virtual void OnPaint()                                        { }
    virtual void OnSize( UINT nType, int cx, int cy )             { }
    virtual BOOL OnEraseBkgnd( HDC hdc )                          { return FALSE; }
    virtual void OnMouseMove( UINT flags, int x, int y )          { }
    virtual void OnLButtonDown( UINT flags, int x, int y )        { }
    virtual void OnLButtonDblClk( UINT flags, int x, int y )      { }
    virtual void OnRButtonDown( UINT flags, int x, int y )        { }
    virtual void OnMButtonDown( UINT flags, int x, int y )        { }
    virtual void OnPaletteChanged( HWND hwndFocus )               { }
    virtual BOOL OnQueryNewPalette()                              { return FALSE; }
    virtual void OnKeyDown( UINT nChar, UINT nRepCnt, UINT flags ){ }
    virtual void OnChar( UINT nChar, UINT nRepCnt, UINT flags )   { }
    virtual void OnTimer( UINT_PTR id )                           { }
    virtual void OnActivateApp( BOOL bActive, DWORD dwThread )    { }
    virtual BOOL OnQueryEndSession()                              { return TRUE; }
    virtual void OnClose()                                        { DestroyWindow(); }
    virtual void OnSysCommand( UINT cmdId, LPARAM lParam )        { }
    virtual void OnSetFocus( HWND hwndOld )                       { }
    virtual void OnKillFocus( HWND hwndNew )                      { }
    virtual void OnMove( int x, int y )                           { }
    virtual void OnGetMinMaxInfo( MINMAXINFO* pMmi )               { }

    // PostNcDestroy: MFC's hook for `delete this` after WM_NCDESTROY.
    virtual void PostNcDestroy()                                  { }

    // ----- Static helpers -----
    static CWndStub* FromHandle( HWND hwnd );
    static CWndStub* GetActiveWindow();
    static void SetFnMouseMove( FNMOUSEMOVE_STUB* fn );

protected:
    // Helper: register a WNDCLASS with the given style + cursor + icon + brush.
    // Returns the class atom or 0. The class is registered with our static
    // WndProc. Idempotent — calling with the same name twice is fine.
    static ATOM RegisterClassOnce( LPCSTR pszName, UINT style,
                                   HCURSOR hCursor = NULL,
                                   HBRUSH hbrBackground = NULL,
                                   HICON hIcon = NULL );

    // The single static window procedure all CWndStub-derived windows use.
    static LRESULT CALLBACK StaticWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

    static FNMOUSEMOVE_STUB* sm_fnMouseMove;
};


#endif // __WNDSTUB_H__
