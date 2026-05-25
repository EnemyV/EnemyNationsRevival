#ifndef __WNDBASE_H__
#define __WNDBASE_H__

//#include "..\lib\_res.h"
#include <subclass.h>

// wndbase.h : header file
//

// CWndBase inherits from the non-MFC CWndStub. The FNMOUSEMOVE callback
// takes CWndStub* and (x,y) ints (not CWnd*/CPoint, which were MFC types).
#include "wndstub.h"
typedef CWndStub CWndBaseSuper;
typedef void ( FNMOUSEMOVE )( CWndStub* pWnd, UINT nFlags, int x, int y );

/////////////////////////////////////////////////////////////////////////////
// CWndBase window

class CWndBase: public CWndBaseSuper {
    // Construction
public:

    CWndBase();

    // Attributes
public:
    // HDC-based DC management
    HDC GetDC() { if ( m_pDc != NULL ) return ( m_pDc ); return CWndStub::GetDC(); }
    int ReleaseDC( HDC h ) { if ( h == m_pDc ) return TRUE; return CWndStub::ReleaseDC( h ); }

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
    // Virtual overrides of CWndStub handlers. Signatures must match
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

    LRESULT WindowProc( UINT Message, WPARAM wParam, LPARAM lParam );

    static FNMOUSEMOVE* sm_fnMouseMove;

    CFramePainter m_framepainter;

    HDC m_pDc;   // for own DC windows
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
    virtual int  OnCreate( LPCREATESTRUCT lpCreateStruct );
    virtual void OnDestroy();
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
    virtual void OnDestroy();
};


#include <list>
#include <unordered_map>
extern std::list<CWndAnim*> theAnimList;
extern std::unordered_map<CWndPrimary*, CWndPrimary*> thePrimaryMap;


#endif
