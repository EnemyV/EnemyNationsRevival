#ifndef __WNDBASE_H__
#define __WNDBASE_H__

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
#else
typedef CWnd     CWndBaseSuper;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWndBase window

// this is a function that is called for all mouse move messages that aren't handled
// by their window (used in Enemy Nations to blank out the status help
typedef void ( FNMOUSEMOVE )( CWnd* pWnd, UINT nFlags, CPoint point );

class CWndBase: public CWndBaseSuper {
    // Construction
public:

    CWndBase();

    // Attributes
public:
    CDC* GetDC() { if ( m_pDc != NULL ) return ( m_pDc ); return CWnd::GetDC(); }
    int  ReleaseDC( CDC* pDc ) { if ( pDc == m_pDc ) return TRUE; return CWnd::ReleaseDC( pDc ); }

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
    //{{AFX_MSG(CWndBase)
    afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );
    afx_msg void OnDestroy();
    afx_msg BOOL OnEraseBkgnd( CDC* pDC );
    afx_msg void OnMouseMove( UINT nFlags, CPoint point );
    afx_msg void OnPaletteChanged( CWnd* pFocusWnd );
    afx_msg BOOL OnQueryNewPalette();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    LRESULT WindowProc( UINT Message, WPARAM wParam, LPARAM lParam );

    static FNMOUSEMOVE* sm_fnMouseMove;

    CFramePainter m_framepainter;

    CDC* m_pDc;   // for own DC windows
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
    // Generated message map functions
    //{{AFX_MSG(CWndPrimary)
    afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
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
    // Generated message map functions
    //{{AFX_MSG(CWndAnim)
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


#include <list>
#include <unordered_map>
extern std::list<CWndAnim*> theAnimList;
extern std::unordered_map<CWndPrimary*, CWndPrimary*> thePrimaryMap;


#endif
