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
//   1. Implement CWndStub fully (this file + wndstub.cpp).   ✅ DONE
//   2. In wndbase.h, gate on ENATIONS_USE_STUB_WND.          ✅ DONE
//   3. In wndbase.h, replace `CDC* GetDC()` / `int ReleaseDC(CDC*)`
//      overrides with HDC-returning versions when gate on. The current
//      overrides use CWnd::GetDC() and m_pDc is a CDC*.
//   4. In wndbase.cpp, replace BEGIN_MESSAGE_MAP / END_MESSAGE_MAP with
//      `#ifdef ENATIONS_USE_STUB_WND` blocks. When the gate is on, the
//      message map disappears entirely and CWndStub's virtual dispatch
//      handles routing.
//   5. In wndbase.cpp, replace `CWnd::OnCreate(lpCs)` / `CWnd::OnDestroy()`
//      / `CWnd::WindowProc(...)` / `CWnd::OnMouseMove(...)` /
//      `CWnd::OnPaletteChanged(...)` / `CWnd::OnQueryNewPalette()` calls
//      with CWndStub:: versions (which themselves no-op or call
//      DefWindowProc as appropriate). The CFramePainter passthrough in
//      WindowProc stays.
//   6. Replace `CClientDC dc(this)` with raw `HDC hdc = GetDC(); ...
//      ReleaseDC(hdc)`. Two sites in wndbase.cpp (OnPaletteChanged +
//      OnQueryNewPalette).
//   7. Change FNMOUSEMOVE typedef from `void(CWnd*, UINT, CPoint)` to
//      `void(CWndStub*, UINT, int x, int y)` when gate on.
//   8. Walk the 15 derived classes (CWndMain, CWndArea, CWndWorld, etc.).
//      For each one that uses `ON_MESSAGE(WM_FOO, OnFoo)`, add a
//      WindowProc override that handles those messages by switch+call.
//      ~30 ON_MESSAGE / ON_COMMAND / ON_NOTIFY entries in main.cpp's
//      CWndMain map alone — those need explicit dispatch.
//   9. For each derived class that overrides OnEraseBkgnd(CDC*), update
//      signature to OnEraseBkgnd(HDC) when gate on.
//  10. Build, fix, iterate.
//  11. Remove `#include <afxwin.h>` from places where it's only there
//      for CWnd.
//
// ERROR SURFACE (when gate is flipped, before the above is done)
// --------------------------------------------------------------
// Captured 2026-05-17 by temporarily flipping ENATIONS_USE_STUB_WND on:
//   ~30 errors, all in wndbase.h (3 errors) and wndbase.cpp (~27).
//   No errors in derived classes because their override signatures
//   already match CWndStub's virtual signatures (OnCreate takes
//   LPCREATESTRUCT, OnDestroy takes void, etc.). The CDC*-vs-HDC
//   issue only bites in wndbase.cpp/h plus any derived class that
//   overrides OnEraseBkgnd(CDC*) — those will surface when the wndbase
//   layer compiles clean.
//
//   Categories:
//   - C3668 (1):  GetMessageMap override has no base — message map dead
//   - C2737 (2):  _messageEntries / messageMap const-init — message map dead
//   - C2440 (4):  static_cast CWndBase::Method to CWnd::Method — msg-map
//   - C2352 (~12): CWnd::OnCreate/OnDestroy/WindowProc/etc. need this ptr
//   - C2248 (~6): CWnd::OnCreate/OnDestroy/etc. are protected in CWnd
//   - C2665 (2):  CClientDC ctor wants CWnd*, has CWndBase*
//   - C2664 (1):  FNMOUSEMOVE expects CWnd*, has CWndBase*
//
//---------------------------------------------------------------------------

#ifndef __WNDSTUB_H__
#define __WNDSTUB_H__

#include <windows.h>

// When the gate is on, MFC's message-map macros (DECLARE_MESSAGE_MAP /
// BEGIN_MESSAGE_MAP / END_MESSAGE_MAP / ON_WM_*) need to dissolve because
// they reference CWnd internals (GetMessageMap virtual, _messageEntries
// array, etc.) that CWndStub doesn't provide. CWndStub uses direct virtual
// dispatch instead — derived classes' OnCreate/OnDestroy/OnPaint/etc. just
// override the virtuals declared in CWndStub.
//
// For custom messages (ON_MESSAGE(WM_FOO, OnFoo)) we drop the registration
// here; derived classes that need custom WM_ values must override virtual
// WindowProc and switch on those values themselves.
//
// Phase 4c removed CConquerApp's CWinApp inheritance; its old
// BEGIN_MESSAGE_MAP(CConquerApp, CWinApp) referenced CWinApp internals
// that are gone. These macro overrides keep BEGIN_MESSAGE_MAP / ON_WM_*
// as cheap no-ops so any remaining declarations still compile.
  // Undef first, then define empty so afx headers cannot redefine these
  // out from under us — these overrides must take final precedence.
  #ifdef DECLARE_MESSAGE_MAP
    #undef DECLARE_MESSAGE_MAP
  #endif
  #define DECLARE_MESSAGE_MAP()
  #ifdef BEGIN_MESSAGE_MAP
    #undef BEGIN_MESSAGE_MAP
  #endif
  // BEGIN_MESSAGE_MAP needs to swallow its body up to END_MESSAGE_MAP. The
  // cleanest no-op form is to open an anonymous namespace that nothing can
  // see, then close it at END_MESSAGE_MAP — but that's fragile. Instead,
  // wrap the body in a static-const inline-init that the compiler discards.
  // Even simpler: leave the body in place but redirect to nothing useful.
  // We use a trick: define BEGIN_MESSAGE_MAP to start a struct, and the
  // ON_* macros to declare unused members; END_MESSAGE_MAP closes the struct.
  // The struct is local to the function-level scope or namespace.
  #define BEGIN_MESSAGE_MAP(theClass, baseClass) \
      namespace __msgmap_##theClass { struct _ignore_msgmap {
  #define END_MESSAGE_MAP() }; }
  // ON_WM_* and ON_MESSAGE / ON_COMMAND / ON_NOTIFY: emit harmless typedefs
  // inside the struct above.
// gcc/standard preprocessors do NOT expand __LINE__ inside a direct `##` paste
// (MSVC does, nonstandardly). Two-level indirection forces the expansion so each
// message-map dummy typedef gets a unique per-line name (no redeclaration).
#ifndef EN_LINEPASTE
#define EN_LINEPASTE_(a,b) a##b
#define EN_LINEPASTE2_(a,b) EN_LINEPASTE_(a,b)
#define EN_LINEPASTE(pfx) EN_LINEPASTE2_(pfx, __LINE__)
#endif
// Five of these are also stubbed by mfc_compat.h (as dummy `int _x = 0;` variables)
// when include order puts it first; both sets are equivalent no-ops, but the silent
// redefinition warned C4005 in every TU that saw both. Undef so this canonical
// unique-typedef set wins cleanly.
#undef ON_WM_PAINT
#undef ON_MESSAGE
#undef ON_COMMAND
#undef ON_NOTIFY
#undef ON_BN_CLICKED
  #define ON_WM_CREATE()              typedef int   EN_LINEPASTE(_wm_create_dummy_);
  #define ON_WM_DESTROY()             typedef int   EN_LINEPASTE(_wm_destroy_dummy_);
  #define ON_WM_PAINT()               typedef int   EN_LINEPASTE(_wm_paint_dummy_);
  #define ON_WM_SIZE()                typedef int   EN_LINEPASTE(_wm_size_dummy_);
  #define ON_WM_ERASEBKGND()          typedef int   EN_LINEPASTE(_wm_erasebkgnd_dummy_);
  #define ON_WM_MOUSEMOVE()           typedef int   EN_LINEPASTE(_wm_mousemove_dummy_);
  #define ON_WM_LBUTTONDOWN()         typedef int   EN_LINEPASTE(_wm_lbtndn_dummy_);
  #define ON_WM_LBUTTONDBLCLK()       typedef int   EN_LINEPASTE(_wm_lbtndbl_dummy_);
  #define ON_WM_RBUTTONDOWN()         typedef int   EN_LINEPASTE(_wm_rbtndn_dummy_);
  #define ON_WM_MBUTTONDOWN()         typedef int   EN_LINEPASTE(_wm_mbtndn_dummy_);
  #define ON_WM_PALETTECHANGED()      typedef int   EN_LINEPASTE(_wm_palchg_dummy_);
  #define ON_WM_QUERYNEWPALETTE()     typedef int   EN_LINEPASTE(_wm_qnewpal_dummy_);
  #define ON_WM_KEYDOWN()             typedef int   EN_LINEPASTE(_wm_keydn_dummy_);
  #define ON_WM_CHAR()                typedef int   EN_LINEPASTE(_wm_char_dummy_);
  #define ON_WM_TIMER()               typedef int   EN_LINEPASTE(_wm_timer_dummy_);
  #define ON_WM_ACTIVATEAPP()         typedef int   EN_LINEPASTE(_wm_actapp_dummy_);
  #define ON_WM_QUERYENDSESSION()     typedef int   EN_LINEPASTE(_wm_qend_dummy_);
  #define ON_WM_CLOSE()               typedef int   EN_LINEPASTE(_wm_close_dummy_);
  #define ON_WM_SYSCOMMAND()          typedef int   EN_LINEPASTE(_wm_syscmd_dummy_);
  #define ON_WM_SETFOCUS()            typedef int   EN_LINEPASTE(_wm_setfocus_dummy_);
  #define ON_WM_KILLFOCUS()           typedef int   EN_LINEPASTE(_wm_killfocus_dummy_);
  #define ON_WM_MOVE()                typedef int   EN_LINEPASTE(_wm_move_dummy_);
  #define ON_WM_GETMINMAXINFO()       typedef int   EN_LINEPASTE(_wm_getminmax_dummy_);
  #define ON_WM_DRAWITEM()            typedef int   EN_LINEPASTE(_wm_drawitem_dummy_);
  #define ON_WM_HSCROLL()             typedef int   EN_LINEPASTE(_wm_hscroll_dummy_);
  #define ON_WM_VSCROLL()             typedef int   EN_LINEPASTE(_wm_vscroll_dummy_);
  #define ON_WM_LBUTTONUP()           typedef int   EN_LINEPASTE(_wm_lbtnup_dummy_);
  #define ON_WM_MBUTTONUP()           typedef int   EN_LINEPASTE(_wm_mbtnup_dummy_);
  #define ON_WM_RBUTTONUP()           typedef int   EN_LINEPASTE(_wm_rbtnup_dummy_);
  #define ON_WM_ACTIVATE()            typedef int   EN_LINEPASTE(_wm_activate_dummy_);
  #define ON_WM_NCLBUTTONDOWN()       typedef int   EN_LINEPASTE(_wm_ncbtndn_dummy_);
  #define ON_WM_NCMOUSEMOVE()         typedef int   EN_LINEPASTE(_wm_ncmm_dummy_);
  #define ON_WM_RBUTTONDBLCLK()       typedef int   EN_LINEPASTE(_wm_rbtndbl_dummy_);
  #define ON_WM_MBUTTONDBLCLK()       typedef int   EN_LINEPASTE(_wm_mbtndbl_dummy_);
  #define ON_WM_SETCURSOR()           typedef int   EN_LINEPASTE(_wm_setcursor_dummy_);
  #define ON_WM_KEYUP()               typedef int   EN_LINEPASTE(_wm_keyup_dummy_);
  #define ON_WM_CTLCOLOR()            typedef int   EN_LINEPASTE(_wm_ctlcolor_dummy_);
  #define ON_WM_MOUSEWHEEL()          typedef int   EN_LINEPASTE(_wm_mousewheel_dummy_);
  #define ON_WM_NOTIFY()              typedef int   EN_LINEPASTE(_wm_notify_dummy_);
  #define ON_WM_CONTEXTMENU()         typedef int   EN_LINEPASTE(_wm_ctxmenu_dummy_);
  #define ON_WM_INITDIALOG()          typedef int   EN_LINEPASTE(_wm_initdlg_dummy_);
  #define ON_WM_HELPINFO()            typedef int   EN_LINEPASTE(_wm_helpinfo_dummy_);
  #define ON_WM_RBUTTONDBLCLK_EX()    typedef int   EN_LINEPASTE(_wm_rdblex_dummy_);
  #define ON_WM_LBUTTONDBLCLK_EX()    typedef int   EN_LINEPASTE(_wm_ldblex_dummy_);
  #define ON_WM_MEASUREITEM()         typedef int   EN_LINEPASTE(_wm_measureitem_dummy_);
  #define ON_WM_COMPAREITEM()         typedef int   EN_LINEPASTE(_wm_compareitem_dummy_);
  #define ON_WM_DELETEITEM()          typedef int   EN_LINEPASTE(_wm_deleteitem_dummy_);
  #define ON_WM_PARENTNOTIFY()        typedef int   EN_LINEPASTE(_wm_parentnotify_dummy_);
  #define ON_WM_WINDOWPOSCHANGING()   typedef int   EN_LINEPASTE(_wm_winposchg_dummy_);
  #define ON_WM_WINDOWPOSCHANGED()    typedef int   EN_LINEPASTE(_wm_winposchgd_dummy_);
  #define ON_WM_SHOWWINDOW()          typedef int   EN_LINEPASTE(_wm_showwindow_dummy_);
  #define ON_WM_NCCALCSIZE()          typedef int   EN_LINEPASTE(_wm_nccalcsize_dummy_);
  #define ON_WM_NCHITTEST()           typedef int   EN_LINEPASTE(_wm_nchittest_dummy_);
  #define ON_WM_NCPAINT()             typedef int   EN_LINEPASTE(_wm_ncpaint_dummy_);
  #define ON_WM_VKEYTOITEM()          typedef int   EN_LINEPASTE(_wm_vkeytoitem_dummy_);
  #define ON_WM_CHARTOITEM()          typedef int   EN_LINEPASTE(_wm_chartoitem_dummy_);
  #define ON_MESSAGE(msg, fn)         typedef int   EN_LINEPASTE(_on_msg_dummy_);
  #define ON_COMMAND(id, fn)          typedef int   EN_LINEPASTE(_on_cmd_dummy_);
  #define ON_COMMAND_RANGE(lo, hi, fn) typedef int  EN_LINEPASTE(_on_cmdrng_dummy_);
  #define ON_NOTIFY(code, id, fn)     typedef int   EN_LINEPASTE(_on_ntfy_dummy_);
  #define ON_NOTIFY_RANGE(code, lo, hi, fn) typedef int EN_LINEPASTE(_on_ntfyrng_dummy_);
  #define ON_BN_CLICKED(id, fn)       typedef int   EN_LINEPASTE(_on_bnclk_dummy_);
  #define ON_EN_CHANGE(id, fn)        typedef int   EN_LINEPASTE(_on_enchg_dummy_);
  #define ON_LBN_SELCHANGE(id, fn)    typedef int   EN_LINEPASTE(_on_lbnsc_dummy_);
  #define ON_LBN_DBLCLK(id, fn)       typedef int   EN_LINEPASTE(_on_lbndb_dummy_);
  #define ON_CBN_SELCHANGE(id, fn)    typedef int   EN_LINEPASTE(_on_cbnsc_dummy_);
  // Also kill the AFX_MSG enclosure markers
  #ifdef afx_msg
    #undef afx_msg
  #endif
  #define afx_msg

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

    // MFC-compatible Create overloads. CWndBase::Create / CWndStatBar::Create
    // and friends use this signature; we delegate to CreateEx with the parent
    // unwrapped to HWND. Accepts CWnd* (MFC widget parents) or CWndStub*.
    BOOL Create( LPCSTR lpszClassName, LPCSTR lpszWindowName, DWORD dwStyle,
                 const RECT& rect, CWndStub* pParent, UINT nID, void* pContext = NULL )
    {
        (void)pContext;
        return CreateEx( 0, lpszClassName, lpszWindowName, dwStyle,
                         rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                         pParent ? pParent->m_hWnd : NULL, (HMENU)(UINT_PTR)nID, NULL );
    }
    BOOL Create( LPCSTR lpszClassName, LPCSTR lpszWindowName, DWORD dwStyle,
                 const RECT& rect, CWnd* pParent, UINT nID, void* pContext = NULL )
    {
        (void)pContext;
        return CreateEx( 0, lpszClassName, lpszWindowName, dwStyle,
                         rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                         pParent ? pParent->GetSafeHwnd() : NULL, (HMENU)(UINT_PTR)nID, NULL );
    }

    // ----- Geometry / state -----
    BOOL GetClientRect( RECT* pRect ) const;
    BOOL GetWindowRect( RECT* pRect ) const;
    BOOL SetWindowPos( HWND hwndAfter, int x, int y, int cx, int cy, UINT flags );
    BOOL InvalidateRect( const RECT* pRect, BOOL bErase = TRUE ) const;
    BOOL UpdateWindow() const;
    BOOL ShowWindow( int nCmdShow );
    BOOL EnableWindow( BOOL bEnable = TRUE );
    BOOL MoveWindow( int x, int y, int cx, int cy, BOOL bRepaint = TRUE );
    BOOL ValidateRect( const RECT* pRect = NULL ) const;

    // Coordinate conversion - MFC-compatible 4 overloads
    BOOL ScreenToClient( LPPOINT pt ) const;
    BOOL ScreenToClient( LPRECT  rc ) const;
    BOOL ClientToScreen( LPPOINT pt ) const;
    BOOL ClientToScreen( LPRECT  rc ) const;

    // Placement
    BOOL SetWindowPlacement( const WINDOWPLACEMENT* pwp );
    BOOL GetWindowPlacement( WINDOWPLACEMENT* pwp ) const;

    // Parent / sibling / dialog item lookup. GetParent returns CWndStub*
    // (MFC-compatible) so call sites like `GetParent()->SendMessage(...)`
    // keep working. For parents that aren't stub-managed (no GWLP_USERDATA),
    // a thread-local temporary CWndStub wraps the HWND and exposes the same
    // SendMessage/PostMessage/InvalidateRect/etc. surface.
    CWndStub* GetParent() const;
    HWND GetParentHwnd() const                                    { return ::GetParent( m_hWnd ); }
    HWND GetTopLevelParent() const;
    HWND GetSafeHwnd() const                                      { return m_hWnd; }
    // GetDlgItem returns CWndStub* (MFC-compatible) so `pBtn->ShowWindow(...)`
    // patterns work. Non-stub-managed dialog items get a thread-local temp.
    CWndStub* GetDlgItem( int nID ) const;
    HWND GetDlgItemHwnd( int nID ) const                          { return ::GetDlgItem( m_hWnd, nID ); }
    int  GetDlgCtrlID() const;
    static HWND FindWindow( LPCSTR lpszClassName, LPCSTR lpszWindowName );

    // Long-ptr access (raw Win32; useful for subclassing / GWLP_USERDATA games)
    LONG_PTR GetWindowLongPtr( int idx ) const                    { return ::GetWindowLongPtr( m_hWnd, idx ); }
    LONG_PTR SetWindowLongPtr( int idx, LONG_PTR v )              { return ::SetWindowLongPtr( m_hWnd, idx, v ); }

    // ----- Input / focus -----
    HWND SetFocus();
    HWND SetCapture();
    static BOOL ReleaseCapture();
    HWND SetForegroundWindow();
    BOOL BringWindowToTop();
    // SetActiveWindow has MFC semantics: takes no args, returns previous active.
    CWndStub* SetActiveWindow();
    CWndStub* ChildWindowFromPoint( POINT pt ) const;
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

    // MFC compat — Default() forwards to DefWindowProc with the current message
    // context (used inside WindowProc overrides). For non-message-context calls
    // it's a no-op. SetRedraw is a simple WM_SETREDRAW dispatch.
    LRESULT Default()                                             { return 0; }
    BOOL SetRedraw( BOOL bRedraw = TRUE )                         { ::SendMessage( m_hWnd, WM_SETREDRAW, (WPARAM)bRedraw, 0 ); return TRUE; }
    // MFC-compatible AssertValid for ASSERT_VALID(this) in Debug builds.
    void AssertValid() const                                      { }

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

    // CPoint-taking forwarders for OnMouseMove/OnLButtonDown/etc — game
    // code with MFC-style signatures (`CWnd::OnMouseMove(UINT, CPoint)`)
    // can call these. The forwarders unpack to (UINT, int x, int y).
    void OnMouseMove( UINT flags, POINT point )                   { OnMouseMove( flags, point.x, point.y ); }
    void OnLButtonDown( UINT flags, POINT point )                 { OnLButtonDown( flags, point.x, point.y ); }
    void OnLButtonDblClk( UINT flags, POINT point )               { OnLButtonDblClk( flags, point.x, point.y ); }
    void OnRButtonDown( UINT flags, POINT point )                 { OnRButtonDown( flags, point.x, point.y ); }
    void OnMButtonDown( UINT flags, POINT point )                 { OnMButtonDown( flags, point.x, point.y ); }

    BOOL OnMouseWheel( UINT flags, short zDelta, POINT point )    { return OnMouseWheel( flags, zDelta, point.x, point.y ); }

    // MFC-type virtual handlers — these are the ACTUAL dispatch targets for
    // WM_SETCURSOR / WM_HSCROLL / WM_VSCROLL / WM_ACTIVATE / WM_CTLCOLOR* /
    // WM_PALETTECHANGED. Game-side classes (CWndArea/CWndWorld/etc.) declare
    // these with CWnd* / CScrollBar* / CDC* signatures (MFC style), so the
    // virtual dispatch needs to USE those signatures. CWndStub::WindowProc
    // wraps the raw HWND/HDC via FromHandle and calls these.
    //
    // The HWND/HDC-typed virtuals above (OnPaletteChanged(HWND), OnSetCursor
    // (HWND,...), etc.) are still virtual so HWND-style overrides work too,
    // but the standard message dispatch goes through these MFC-typed virtuals.
    virtual void OnPaletteChanged( CWnd* pFocus )                 { OnPaletteChanged( pFocus ? pFocus->GetSafeHwnd() : (HWND)NULL ); }
    virtual void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pSB ) { OnHScroll( nSBCode, nPos, pSB ? pSB->GetSafeHwnd() : (HWND)NULL ); }
    virtual void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pSB ) { OnVScroll( nSBCode, nPos, pSB ? pSB->GetSafeHwnd() : (HWND)NULL ); }
    virtual BOOL OnSetCursor( CWnd* pWnd, UINT nHit, UINT msg )   { return OnSetCursor( pWnd ? pWnd->GetSafeHwnd() : (HWND)NULL, nHit, msg ); }
    virtual void OnActivate( UINT nState, CWnd* pOther, BOOL bMin ) { OnActivate( nState, pOther ? pOther->GetSafeHwnd() : (HWND)NULL, bMin ); }
    virtual HBRUSH OnCtlColor( CDC* pDC, CWnd* pCtrl, UINT nType ) { return OnCtlColor( pDC ? pDC->GetSafeHdc() : (HDC)NULL, pCtrl ? pCtrl->GetSafeHwnd() : (HWND)NULL, nType ); }

    // Additional virtual handlers covering common WM_* the game uses.
    // BOOL-returning ones: return TRUE if the message was handled; FALSE
    // causes WindowProc to fall through to DefWindowProc (so derived
    // classes don't have to remember to invoke base behavior).
    virtual BOOL OnCommand( WPARAM wParam, LPARAM lParam )        { return FALSE; }
    virtual BOOL OnNotify ( WPARAM wParam, LPARAM lParam,
                            LRESULT* pResult )                    { return FALSE; }
    virtual void OnHScroll( UINT nSBCode, UINT nPos, HWND hwndSB ){ }
    virtual void OnVScroll( UINT nSBCode, UINT nPos, HWND hwndSB ){ }
    virtual BOOL OnSetCursor( HWND hwnd, UINT nHitTest, UINT msg ){ return FALSE; }
    virtual void OnActivate( UINT nState, HWND hwndOther, BOOL bMin ) { }
    virtual BOOL OnMouseWheel( UINT nFlags, short zDelta,
                               int x, int y )                     { return FALSE; }
    virtual void OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags ) { }
    virtual HBRUSH OnCtlColor( HDC hdc, HWND hwndCtrl, UINT nCtlColor ) { return NULL; }
    virtual BOOL PreCreateWindow( CREATESTRUCT& cs )              { return TRUE; }
    virtual BOOL PreTranslateMessage( MSG* pMsg )                 { return FALSE; }

    // PostNcDestroy: MFC's hook for `delete this` after WM_NCDESTROY.
    virtual void PostNcDestroy()                                  { }

    // ----- Static helpers -----
    static CWndStub* FromHandle( HWND hwnd );
    static CWndStub* GetActiveWindow();
    static void SetFnMouseMove( FNMOUSEMOVE_STUB* fn );

    // The single static window procedure all CWndStub-derived windows use.
    // Public so game-side window classes can register it as their wndproc
    // (e.g. `wc.lpfnWndProc = &CWndStub::StaticWndProc;`).
    static LRESULT CALLBACK StaticWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

protected:
    // Helper: register a WNDCLASS with the given style + cursor + icon + brush.
    // Returns the class atom or 0. The class is registered with our static
    // WndProc. Idempotent — calling with the same name twice is fine.
    static ATOM RegisterClassOnce( LPCSTR pszName, UINT style,
                                   HCURSOR hCursor = NULL,
                                   HBRUSH hbrBackground = NULL,
                                   HICON hIcon = NULL );

    static FNMOUSEMOVE_STUB* sm_fnMouseMove;
};


//---------------------------------------------------------------------------
// CStubPaintDC / CStubClientDC / CStubWindowDC — RAII HDC holders that
// replace MFC's CPaintDC / CClientDC / CWindowDC when the gate is on.
//
// Operator HDC() conversion lets call sites that previously used `dc` as
// a CDC* (e.g. `pdib->BltDC(dc)`) keep compiling — anything taking HDC
// also takes a CStubXxxDC because of the conversion. Game code using
// `dc.m_hDC` directly works because m_hDC is a public member.
//
// CDC*-style method calls (`dc.TextOut(...)`, `dc.SetBkColor(...)`) are
// NOT supported here — those derived classes that use such methods must
// be updated to call the global ::TextOut / ::SetBkColor instead, passing
// `dc.m_hDC` (or `dc` directly thanks to operator HDC()) as the first arg.
//---------------------------------------------------------------------------

// CDC-style methods exposed via this mixin so all three DC RAII classes
// (Paint/Client/Window) get the same surface. Game-side code calls things
// like `dc.SetTextColor(...)`, `dc.SelectObject(...)`, `dc.FillRect(...)`
// — these all forward to the global Win32 GDI calls using m_hDC.
#define STUBDC_CDC_METHODS()                                                                  \
    COLORREF SetBkColor( COLORREF c )         { return ::SetBkColor( m_hDC, c ); }            \
    COLORREF SetTextColor( COLORREF c )       { return ::SetTextColor( m_hDC, c ); }          \
    int  SetBkMode( int m )                   { return ::SetBkMode( m_hDC, m ); }             \
    HGDIOBJ SelectObject( HGDIOBJ h )         { return ::SelectObject( m_hDC, h ); }          \
    CFont*   SelectObject( CFont*   p )       { return (CFont*  )CFont  ::FromHandle( (HFONT  )::SelectObject( m_hDC, p->GetSafeHandle() ) ); }    \
    CPen*    SelectObject( CPen*    p )       { return (CPen*   )CPen   ::FromHandle( (HPEN   )::SelectObject( m_hDC, p->GetSafeHandle() ) ); }    \
    CBrush*  SelectObject( CBrush*  p )       { return (CBrush* )CBrush ::FromHandle( (HBRUSH )::SelectObject( m_hDC, p->GetSafeHandle() ) ); }    \
    CBitmap* SelectObject( CBitmap* p )       { return (CBitmap*)CBitmap::FromHandle( (HBITMAP)::SelectObject( m_hDC, p->GetSafeHandle() ) ); }    \
    int  FillRect( const RECT* r, HBRUSH b )  { return ::FillRect( m_hDC, r, b ); }           \
    int  FillRect( const RECT* r, CBrush* pb ){ return ::FillRect( m_hDC, r, (HBRUSH)pb->GetSafeHandle() ); } \
    void FillSolidRect( const RECT* r, COLORREF c ) {                                          \
        COLORREF old = ::SetBkColor( m_hDC, c );                                              \
        RECT rc = *r;                                                                          \
        ::ExtTextOutA( m_hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL );                          \
        ::SetBkColor( m_hDC, old );                                                            \
    }                                                                                          \
    void FillSolidRect( int x, int y, int cx, int cy, COLORREF c ) {                          \
        RECT rc = { x, y, x + cx, y + cy };                                                    \
        FillSolidRect( &rc, c );                                                               \
    }                                                                                          \
    int  DrawTextA( LPCSTR s, int n, LPRECT r, UINT fmt ) { return ::DrawTextA( m_hDC, s, n, r, fmt ); } \
    BOOL GetTextMetricsA( LPTEXTMETRICA pm )  { return ::GetTextMetricsA( m_hDC, pm ); }      \
    BOOL TextOutA( int x, int y, LPCSTR s, int n ) { return ::TextOutA( m_hDC, x, y, s, n ); }\
    BOOL Rectangle( int l, int t, int r, int b ) { return ::Rectangle( m_hDC, l, t, r, b ); } \
    BOOL MoveToEx( int x, int y, LPPOINT pt ) { return ::MoveToEx( m_hDC, x, y, pt ); }       \
    BOOL MoveTo  ( int x, int y )             { return ::MoveToEx( m_hDC, x, y, NULL ); }     \
    BOOL LineTo  ( int x, int y )             { return ::LineTo( m_hDC, x, y ); }             \
    COLORREF GetNearestColor( COLORREF c )    { return ::GetNearestColor( m_hDC, c ); }       \
    HPALETTE SelectPalette( HPALETTE hp, BOOL bFB ) { return ::SelectPalette( m_hDC, hp, bFB ); } \
    CPalette* SelectPalette( CPalette* p, BOOL bFB ) { return (CPalette*)CPalette::FromHandle( ::SelectPalette( m_hDC, (HPALETTE)p->GetSafeHandle(), bFB ) ); } \
    UINT RealizePalette()                     { return ::RealizePalette( m_hDC ); }           \
    HDC  GetSafeHdc() const                   { return m_hDC; }

// Compat conversions for code that takes CDC* or CDC&.
// CDC::FromHandle(HDC) returns a CDC* wrapping the raw HDC, so those
// game-defined functions still work — they end up calling CDC methods
// that forward to the same HDC via ::GDI calls.
#define STUBDC_CDC_CONVERSIONS()                                                              \
    operator CDC*() const                     { return CDC::FromHandle( m_hDC ); }            \
    operator CDC&() const                     { return *CDC::FromHandle( m_hDC ); }

class CStubPaintDC {
public:
    CStubPaintDC( CWndStub* p ) : m_hWnd( p->m_hWnd ) { m_hDC = ::BeginPaint( m_hWnd, &m_ps ); }
    CStubPaintDC( CWnd* p )     : m_hWnd( p ? p->GetSafeHwnd() : NULL ) { m_hDC = ::BeginPaint( m_hWnd, &m_ps ); }
    ~CStubPaintDC()                                   { ::EndPaint( m_hWnd, &m_ps ); }
    operator HDC() const                              { return m_hDC; }
    HDC          m_hDC;
    PAINTSTRUCT  m_ps;
    STUBDC_CDC_METHODS()
    STUBDC_CDC_CONVERSIONS()
private:
    HWND m_hWnd;
};

class CStubClientDC {
public:
    CStubClientDC( CWndStub* p ) : m_hWnd( p->m_hWnd ) { m_hDC = ::GetDC( m_hWnd ); }
    CStubClientDC( CWnd* p )     : m_hWnd( p ? p->GetSafeHwnd() : NULL ) { m_hDC = ::GetDC( m_hWnd ); }
    ~CStubClientDC()                                   { ::ReleaseDC( m_hWnd, m_hDC ); }
    operator HDC() const                               { return m_hDC; }
    HDC m_hDC;
    STUBDC_CDC_METHODS()
    STUBDC_CDC_CONVERSIONS()
private:
    HWND m_hWnd;
};

class CStubWindowDC {
public:
    CStubWindowDC( CWndStub* p ) : m_hWnd( p->m_hWnd ) { m_hDC = ::GetWindowDC( m_hWnd ); }
    // Guard p BEFORE the member call: GetSafeHwnd()'s `return this ? m_hWnd : NULL`
    // is UB (the compiler may assume this!=NULL and delete the check). In plain
    // Release the call inlines and constant-folds a (CWnd*)NULL arg, but a non-
    // inlined build (ASan/Sanitize) does a real null-`this` read of m_hWnd -> SEGV
    // in InitInstance's `CWindowDC dc((CWnd*)NULL)` (lastplnt.cpp:1153) = the whole
    // Sanitize/ASan build died at startup. Checking p here is UB-free and matches
    // the intended "NULL window -> screen DC" semantics.
    CStubWindowDC( CWnd* p )     : m_hWnd( p ? p->GetSafeHwnd() : NULL ) { m_hDC = ::GetWindowDC( m_hWnd ); }
    ~CStubWindowDC()                                   { ::ReleaseDC( m_hWnd, m_hDC ); }
    operator HDC() const                               { return m_hDC; }
    HDC m_hDC;
    STUBDC_CDC_METHODS()
    STUBDC_CDC_CONVERSIONS()
private:
    HWND m_hWnd;
};

// Macro aliases — game code keeps using CPaintDC / CClientDC / CWindowDC
// verbatim; they resolve to the stub classes. Position the macros AFTER
// the class declarations so the macro definitions don't recursively
// replace tokens inside the class bodies.
#define CPaintDC   CStubPaintDC
#define CClientDC  CStubClientDC
#define CWindowDC  CStubWindowDC


#endif // __WNDSTUB_H__
