//---------------------------------------------------------------------------
//
// subclass.cpp — Phase 6 Stage 5 (Phase A.5) STUB
//
// The original 2250 lines of GDI-heavy owner-draw / window-subclass code
// have been excluded as part of the Phase 6 GDI removal. Per the
// 2026-05-24 runtime audit, CGlobalSubClass::DrawButton (the WM_DRAWITEM
// paint path) never fires in the live game — owner-draw button controls
// lived on MFC dialogs that are now SDL2. The init call sites
// (lastplnt.cpp InitCustomUI -> Subclass/SetDrawInfo) and the one
// bmbutton.cpp DrawButton call site still exist at link time, but at
// runtime they install hooks that never get invoked.
//
// This file therefore keeps only:
//   - g_bSubclassing static (referenced by inline IsSubclassing())
//   - CTextColors::CTextColors (a pure-data POD ctor, many call sites)
//   - CButtonTracker::CButtonTracker (member of CFramePainter array)
//   - CFramePainter ctor / WindowProc / SetDrawInfo / IsInitialized
//     — no-op / minimal stubs
//   - CGlobalSubClass::Subclass / UnSubClass — no-op stubs
//   - CGlobalSubClass::DrawButton (6-arg) — no-op stub returning FALSE
//   - CGlobalSubClass::GetBackgroundSrcRect — pure math, still active
//     (called from bmbutton.cpp:89 and unit_wnd.cpp:116 for live
//     background-rect scaling; no GDI involved)
//
// All other CGlobalSubClass*, CFramePainter Track internals,
// CWndOD<T>::OnChildNotify template body, and the GDI bitmap composition
// code are intentionally omitted. To inspect or revive any of it, see
// the previous commit on this file in git history.
//
// This is the user's "exclude don't delete" pattern: the file remains in
// the build; the GDI content is what's excluded.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "_windwrd.h"
#include "subclass.h"
#include "dib.h"

//------------------------ C G l o b a l S u b C l a s s ----------------------

BOOL CGlobalSubClass::g_bSubclassing = FALSE;

// Global flag toggled by mainloop / lastplnt to enable/disable subclassing
// at runtime. With the GDI path stubbed out it has no effect — the writes
// are still in the live code paths, so the symbol must be defined.
BOOL bDoSubclass = FALSE;

void CGlobalSubClass::Subclass( CDIB* /*pdibBkgnd*/,
                                CDIB* /*pdibPushButtonSmall*/,
                                CDIB* /*pdibPushButtonLarge*/,
                                CDIB* /*pdibRadioButton*/,
                                CDIB* /*pdibCheckBox*/,
                                CFont* /*pfontButton*/,
                                int    /*iSoundID*/,
                                CTextColors const& /*textcolorsButton*/,
                                CTextColors const& /*textcolorsStatic*/ )
{
    // Audit-verified runtime-dead. Originally installed CallWndProc hooks
    // for WM_DRAWITEM on owner-draw Win32 buttons; those buttons no longer
    // exist (SDL2 dialogs handle their own painting).
}

void CGlobalSubClass::UnSubClass()
{
    // Pair for Subclass(); same audit applies.
}

BOOL CGlobalSubClass::DrawButton( DRAWITEMSTRUCT* /*lpDIS*/,
                                  CDIB* /*pdibBtnSmall*/,
                                  CDIB* /*pdibBtnLarge*/,
                                  CDIB* /*pdibBackground*/,
                                  CFont* /*pfont*/,
                                  CTextColors const& /*textcolors*/ )
{
    // Audit-verified runtime-dead via lastplnt.cpp/bmbutton.cpp chain.
    return FALSE;
}

//---------------------------------------------------------------------------
// CGlobalSubClass::GetBackgroundSrcRect
//
// Given a window rect and a client rect, return the dib src rect to use.
// PURE MATH — no GDI. Live callers: bmbutton.cpp:89, unit_wnd.cpp:116.
//---------------------------------------------------------------------------
CRect CGlobalSubClass::GetBackgroundSrcRect( CRect const& rectWnd,
                                             CRect const& rectClient,
                                             CRect const& rectDIB )
{
    int iW = Min( rectDIB.Width(), rectWnd.Width() );
    int iH = Min( rectDIB.Height(), rectWnd.Height() );

    CRect rect = rectClient;

    rect.left   = MulDiv( rect.left,   iW, rectWnd.Width() );
    rect.right  = MulDiv( rect.right,  iW, rectWnd.Width() );
    rect.top    = MulDiv( rect.top,    iH, rectWnd.Height() );
    rect.bottom = MulDiv( rect.bottom, iH, rectWnd.Height() );

    return rect;
}

//------------------------------ C T e x t C o l o r s ------------------------

CTextColors::CTextColors( COLORREF colorrefHighlight,
                          COLORREF colorrefText,
                          COLORREF colorrefShadow )
    : m_colorrefHighlight( colorrefHighlight )
    , m_colorrefText( colorrefText )
    , m_colorrefShadow( colorrefShadow )
{
}

//------------------------- C B u t t o n T r a c k e r ----------------------

CButtonTracker::CButtonTracker()
    : m_nHitTest( 0 )
    , m_bCursorIn( FALSE )
    , m_bTracking( FALSE )
{
}

void CButtonTracker::SetTracking( BOOL bTracking )
{
    m_bTracking = bTracking;
}

//----------------------------- C F r a m e P a i n t e r --------------------

CDIB** CFramePainter::s_ppdib = NULL;

void CFramePainter::SetDrawInfo( CDIB* adib[NUM_OD_BITMAPS] )
{
    s_ppdib = adib;  // Stored but never read; WindowProc is runtime-dead
                     // per audit.
}

BOOL CFramePainter::IsInitialized()
{
    return ( s_ppdib != NULL );
}

CFramePainter::CFramePainter()
    : m_pbuttontracker( NULL )
    , m_bActive( FALSE )
{
    // m_abuttontracker[] elements default-construct.
}

BOOL CFramePainter::WindowProc( HWND /*hWnd*/, UINT /*Message*/,
                                WPARAM /*wParam*/, LPARAM /*lParam*/,
                                LRESULT* /*presult*/ )
{
    // Called every message from CWndBase::WindowProc (wndbase.cpp:84) but
    // returns FALSE so the original window proc handles everything.
    // Audit: the painting branches never fired.
    return FALSE;
}
