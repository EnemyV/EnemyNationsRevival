#ifndef __WNDBASE_H__
#define __WNDBASE_H__

// Phase 1 migration gate. To flip on for testing, either define
// ENATIONS_USE_STUB_WND in the build system (CMakeLists.txt) or uncomment
// the line below. Default is off.
// #define ENATIONS_USE_STUB_WND

//#include "..\lib\_res.h"
#include <subclass.h>

// wndbase.h : header file
//

// Phase 1 migration gate. When ENATIONS_USE_STUB_WND is defined, CWndBase
// inherits from the non-MFC CWndStub (see wndstub.h) instead of MFC's CWnd.
// Default is off — CWndBase still inherits from CWnd. Flipping this on
// also requires updating the 15 derived classes' message-map macros and
// the BEGIN_MESSAGE_MAP machinery in wndbase.cpp; see wndstub.h's header
// comment for the per-class migration recipe.
#ifdef ENATIONS_USE_STUB_WND
#include "wndstub.h"
typedef CWndStub CWndBaseSuper;
// In gate-on mode, the FNMOUSEMOVE callback takes CWndStub* (not CWnd*)
// and (x,y) ints (not CPoint), since CPoint is an MFC type.
typedef void ( FNMOUSEMOVE )( CWndStub* pWnd, UINT nFlags, int x, int y );
#else
typedef CWnd     CWndBaseSuper;
// MFC mode: original signature
typedef void ( FNMOUSEMOVE )( CWnd* pWnd, UINT nFlags, CPoint point );
#endif

/////////////////////////////////////////////////////////////////////////////
// CWndBase window

class CWndBase: public CWndBaseSuper {
    // Construction
public:

    CWndBase();

    // Attributes
public:
#ifdef ENATIONS_USE_STUB_WND
    // Gate-on: HDC-based DC management
    HDC GetDC() { if ( m_pDc != NULL ) return ( m_pDc ); return CWndStub::GetDC(); }
    int ReleaseDC( HDC h ) { if ( h == m_pDc ) return TRUE; return CWndStub::ReleaseDC( h ); }
#else
    CDC* GetDC() { if ( m_pDc != NULL ) return ( m_pDc ); return CWnd::GetDC(); }
    int  ReleaseDC( CDC* pDc ) { if ( pDc == m_pDc ) return TRUE; return CWnd::ReleaseDC( pDc ); }
#endif

    // Operations
public:
    static void SetFnMouseMove( FNMOUSEMOVE fnMouseMove ) { sm_fnMouseMove = fnMouseMove; }

    // Overrides
     // ClassWizard generated virtual function overrides
     //{{AFX_VIRTUAL(CWndBase)
     //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CWndBase();

    // Generated message map functions
protected:
#ifdef ENATIONS_USE_STUB_WND
    // Gate-on: virtual overrides of CWndStub handlers. Signatures must match
    // CWndStub's virtuals exactly (HDC, int x,y instead of CDC*, CPoint).
    virtual int  OnCreate( LPCREATESTRUCT lpCreateStruct );
    virtual void OnDestroy();
    virtual BOOL OnEraseBkgnd( HDC hdc );
    virtual void OnMouseMove( UINT nFlags, int x, int y );
    virtual void OnPaletteChanged( HWND hwndFocus );
    virtual BOOL OnQueryNewPalette();
    // Bring CWndStub's POINT-taking forwarders into scope so derived calls
    // like `CWndBase::OnMouseMove(nFlags, point)` resolve to the forwarder
    // that unpacks to (UINT, int, int). C++ name-hiding would otherwise
    // make CWndBase::OnMouseMove only match the (UINT, int, int) override.
public:
    using CWndStub::OnMouseMove;
    using CWndStub::OnLButtonDown;
    using CWndStub::OnLButtonDblClk;
    using CWndStub::OnRButtonDown;
    using CWndStub::OnMButtonDown;
    using CWndStub::OnPaletteChanged;
protected:
    // No DECLARE_MESSAGE_MAP() in gate-on mode — CWndStub uses virtual dispatch.
#else
    //{{AFX_MSG(CWndBase)
    afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );
    afx_msg void OnDestroy();
    afx_msg BOOL OnEraseBkgnd( CDC* pDC );
    afx_msg void OnMouseMove( UINT nFlags, CPoint point );
    afx_msg void OnPaletteChanged( CWnd* pFocusWnd );
    afx_msg BOOL OnQueryNewPalette();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
#endif

    LRESULT WindowProc( UINT Message, WPARAM wParam, LPARAM lParam );

    static FNMOUSEMOVE* sm_fnMouseMove;

    CFramePainter m_framepainter;

#ifdef ENATIONS_USE_STUB_WND
    HDC  m_pDc;   // for own DC windows (gate-on: raw HDC)
#else
    CDC* m_pDc;   // for own DC windows
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CWndPrimary - the main windows shown in the game

class CWndPrimary: public CWndBase {
    // Construction
public:
    CWndPrimary();

    // Attributes
public:
    HACCEL  m_hAccel;       // window accelerators

   // Operations
public:

    // Implementation
public:
    virtual ~CWndPrimary();

protected:
#ifdef ENATIONS_USE_STUB_WND
    virtual int  OnCreate( LPCREATESTRUCT lpCreateStruct );
    virtual void OnDestroy();
#else
    //{{AFX_MSG(CWndPrimary)
    afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CWndAnim window

class CWndAnim: public CWndPrimary {
    // Construction
public:
    CWndAnim() {}

    // Attributes
public:

    // Operations
public:
    virtual void  InvalidateWindow( RECT* pRect = NULL ) { ASSERT( FALSE ); }
    virtual void  ReRender() { ASSERT( FALSE ); }
    virtual void  Draw() { ASSERT( FALSE ); }

#ifdef BUGBUG
    virtual void  InvalidateMap();
    virtual void  Update() { ASSERT( FALSE ); }
    virtual void  Show() { ASSERT( FALSE ); }
#endif

    static void InvalidateAllWindows();

    // Implementation
public:
    virtual ~CWndAnim() {}

protected:
#ifdef ENATIONS_USE_STUB_WND
    virtual void OnDestroy();
#else
    //{{AFX_MSG(CWndAnim)
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
#endif
};


#include <list>
#include <unordered_map>
extern std::list<CWndAnim*> theAnimList;
extern std::unordered_map<CWndPrimary*, CWndPrimary*> thePrimaryMap;


#endif
