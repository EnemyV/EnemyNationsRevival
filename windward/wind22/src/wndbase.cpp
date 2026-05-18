// wndbase.cpp : implementation file
//

#include "stdafx.h"
#include "_windwrd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "wndbase.h"
#include <algorithm>

// windows to animate
std::list<CWndAnim*> theAnimList;

// windows to accelerate, swap between (like MDI clients)
std::unordered_map<CWndPrimary*, CWndPrimary*> thePrimaryMap;


/////////////////////////////////////////////////////////////////////////////
// CWndBase

FNMOUSEMOVE * CWndBase::sm_fnMouseMove = NULL;

CWndBase::CWndBase()
{

 m_pDc = NULL;
}

CWndBase::~CWndBase()
{
}

#ifndef ENATIONS_USE_STUB_WND
BEGIN_MESSAGE_MAP(CWndBase, CWnd)
 //{{AFX_MSG_MAP(CWndBase)
 ON_WM_ERASEBKGND()
 ON_WM_MOUSEMOVE()
 ON_WM_PALETTECHANGED()
 ON_WM_QUERYNEWPALETTE()
 //}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif // !ENATIONS_USE_STUB_WND

/////////////////////////////////////////////////////////////////////////////
// CWndBase message handlers

int CWndBase::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

#ifdef ENATIONS_USE_STUB_WND
 if (CWndStub::OnCreate(lpCreateStruct) == -1)
  return -1;
#else
 if (CWnd::OnCreate(lpCreateStruct) == -1)
  return -1;
#endif

 // if it's OWN_DC we grab a [HDC|CDC*]
 if ( GetClassLong (m_hWnd, GCL_STYLE) & CS_OWNDC )
  m_pDc = GetDC ();

 return 0;
}

void CWndBase::OnDestroy()
{

#ifdef ENATIONS_USE_STUB_WND
 CWndStub::OnDestroy();
#else
 CWnd::OnDestroy();
#endif

 // delete dc
 if ( m_pDc != NULL )
  {
  ReleaseDC (m_pDc);
  m_pDc = NULL;
  }
}

#ifdef ENATIONS_USE_STUB_WND
BOOL CWndBase::OnEraseBkgnd(HDC)
#else
BOOL CWndBase::OnEraseBkgnd(CDC *)
#endif
{

 // we fully draw all of our windows
 return TRUE;
}

//---------------------------------------------------------------------------
// CWndBase::WindowProc
//---------------------------------------------------------------------------
LRESULT
CWndBase::WindowProc(
 UINT   Message,
 WPARAM  wParam,
 LPARAM lParam )
{
 LRESULT result;

 if ( m_framepainter.WindowProc( m_hWnd, Message, wParam, lParam, &result ))
  return result;

#ifdef ENATIONS_USE_STUB_WND
 return CWndStub::WindowProc( Message, wParam, lParam );
#else
 return CWnd::WindowProc( Message, wParam, lParam );
#endif
}

#ifdef ENATIONS_USE_STUB_WND
void CWndBase::OnMouseMove(UINT nFlags, int x, int y)
{
 // if we have a global handler, call it
 if (sm_fnMouseMove != NULL)
  sm_fnMouseMove (this, nFlags, x, y);

 CWndStub::OnMouseMove(nFlags, x, y);
}
#else
void CWndBase::OnMouseMove(UINT nFlags, CPoint point)
{
 // if we have a global handler, call it
 if (sm_fnMouseMove != NULL)
  sm_fnMouseMove (this, nFlags, point);

 CWnd::OnMouseMove(nFlags, point);
}
#endif

#ifdef ENATIONS_USE_STUB_WND
void CWndBase::OnPaletteChanged(HWND hwndFocus)
{
static BOOL bInFunc = FALSE;

 CWndStub::OnPaletteChanged(hwndFocus);

 // Win32s locks up if we do the below code
 if (iWinType == W32s)
  return;

 // stop infinite recursion
 if (bInFunc)
  return;
 bInFunc = TRUE;

 // Raw HDC pattern replaces CClientDC dc(this).
 HDC hdc = ::GetDC( m_hWnd );
 int iRtn = thePal.PalMsg (hdc, m_hWnd, WM_PALETTECHANGED, (WPARAM)hwndFocus, 0);
 ::ReleaseDC( m_hWnd, hdc );

 // invalidate the window
 if (iRtn)
  InvalidateRect (NULL);

 bInFunc = FALSE;
}
#else
void CWndBase::OnPaletteChanged(CWnd* pFocusWnd)
{
static BOOL bInFunc = FALSE;

 CWnd::OnPaletteChanged(pFocusWnd);

 // Win32s locks up if we do the below code
 if (iWinType == W32s)
  return;

 // stop infinite recursion
 if (bInFunc)
  return;
 bInFunc = TRUE;

 CClientDC dc (this);
 int iRtn = thePal.PalMsg (dc.m_hDC, m_hWnd, WM_PALETTECHANGED, (WPARAM) pFocusWnd->m_hWnd, 0);

 // invalidate the window
 if (iRtn)
  InvalidateRect (NULL);

 bInFunc = FALSE;
}
#endif

BOOL CWndBase::OnQueryNewPalette()
{

#ifdef ENATIONS_USE_STUB_WND
 if (iWinType == W32s)
  return CWndStub::OnQueryNewPalette();

 HDC hdc = ::GetDC( m_hWnd );
 thePal.PalMsg (hdc, m_hWnd, WM_QUERYNEWPALETTE, 0, 0);
 ::ReleaseDC( m_hWnd, hdc );

 return CWndStub::OnQueryNewPalette();
#else
 if (iWinType == W32s)
  return CWnd::OnQueryNewPalette();

 CClientDC dc (this);
 thePal.PalMsg (dc.m_hDC, m_hWnd, WM_QUERYNEWPALETTE, 0, 0);

 return CWnd::OnQueryNewPalette();
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CWndPrimary - the main windows shown in the game

CWndPrimary::CWndPrimary ()
{

 m_hAccel = NULL;
}

#ifndef ENATIONS_USE_STUB_WND
BEGIN_MESSAGE_MAP(CWndPrimary, CWndBase)
 //{{AFX_MSG_MAP(CWndPrimary)
 ON_WM_CREATE()
 ON_WM_DESTROY()
 //}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif // !ENATIONS_USE_STUB_WND

CWndPrimary::~CWndPrimary()
{

}

int CWndPrimary::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
#ifdef LOGGINGON
    OutputDebugStringA( "CWndPrimary::OnCreate\n" );
#endif

    int baseCreate = CWndBase::OnCreate( lpCreateStruct );
    if ( baseCreate == -1 )
    {
#ifdef LOGGINGON
        // Print a message with the cx and cy values
        char buf[128];
        sprintf_s( buf, sizeof( buf ), "CWndBase::OnCreate( lpCreateStruct ) %d\n", 
            baseCreate );
        OutputDebugStringA( buf );
#endif
        return -1;
    }
 
 // add to list
 thePrimaryMap[this] = this;

 return 0;
}

void CWndPrimary::OnDestroy()
{

 // remove from list
 thePrimaryMap.erase( this );

 CWndBase::OnDestroy();
}


/////////////////////////////////////////////////////////////////////////////
// CWndAnim - base class for animated windows

#ifndef ENATIONS_USE_STUB_WND
BEGIN_MESSAGE_MAP(CWndAnim, CWndPrimary)
 //{{AFX_MSG_MAP(CWndAnim)
 ON_WM_DESTROY()
 //}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif // !ENATIONS_USE_STUB_WND

void CWndAnim::InvalidateAllWindows ()
{

 for ( CWndAnim* pWnd : theAnimList )
  pWnd->InvalidateWindow (NULL);
}

void CWndAnim::OnDestroy()
{

 auto it = std::find( theAnimList.begin(), theAnimList.end(), this );
 if ( it != theAnimList.end() )
  theAnimList.erase( it );

 CWndPrimary::OnDestroy();
}

