// wndbase.cpp : implementation file
//

#include "stdafx.h"
#include "_windwrd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "wndbase.h"

// windows to animate
CList <CWndAnim *, CWndAnim*> theAnimList;

// windows to accelerate, swap between (like MDI clients)
CMap <CWndPrimary *, CWndPrimary*, CWndPrimary *, CWndPrimary*> thePrimaryMap;


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

BEGIN_MESSAGE_MAP(CWndBase, CWnd)
 //{{AFX_MSG_MAP(CWndBase)
 ON_WM_ERASEBKGND()
 ON_WM_MOUSEMOVE()
 ON_WM_PALETTECHANGED()
 ON_WM_QUERYNEWPALETTE()
 //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWndBase message handlers

int CWndBase::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{

 if (CWnd::OnCreate(lpCreateStruct) == -1)
  return -1;
 
 // if it's OWN_DC we grab a CDC *
 if ( GetClassLong (m_hWnd, GCL_STYLE) & CS_OWNDC )
  m_pDc = GetDC ();
 
 return 0;
}

void CWndBase::OnDestroy() 
{

 CWnd::OnDestroy();

 // delete dc
 if ( m_pDc != NULL )
  {
  ReleaseDC (m_pDc);
  m_pDc = NULL;
  }
}

BOOL CWndBase::OnEraseBkgnd(CDC *) 
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

 return CWnd::WindowProc( Message, wParam, lParam );
}

void CWndBase::OnMouseMove(UINT nFlags, CPoint point) 
{
 // if we have a global handler, call it
 if (sm_fnMouseMove != NULL)
  sm_fnMouseMove (this, nFlags, point);
 
 CWnd::OnMouseMove(nFlags, point);
}

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

BOOL CWndBase::OnQueryNewPalette() 
{
 
 if (iWinType == W32s)
  return CWnd::OnQueryNewPalette();

 CClientDC dc (this);
 thePal.PalMsg (dc.m_hDC, m_hWnd, WM_QUERYNEWPALETTE, 0, 0);

 return CWnd::OnQueryNewPalette();
}

/////////////////////////////////////////////////////////////////////////////
// CWndPrimary - the main windows shown in the game

CWndPrimary::CWndPrimary () 
{ 

 m_hAccel = NULL; 
}

BEGIN_MESSAGE_MAP(CWndPrimary, CWndBase)
 //{{AFX_MSG_MAP(CWndPrimary)
 ON_WM_CREATE()
 ON_WM_DESTROY()
 //}}AFX_MSG_MAP
END_MESSAGE_MAP()

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
 thePrimaryMap.SetAt (this, this);
 
 return 0;
}

void CWndPrimary::OnDestroy() 
{

 // remove from list 
 thePrimaryMap.RemoveKey (this);

 CWndBase::OnDestroy();
}


/////////////////////////////////////////////////////////////////////////////
// CWndAnim - base class for animated windows

BEGIN_MESSAGE_MAP(CWndAnim, CWndPrimary)
 //{{AFX_MSG_MAP(CWndAnim)
 ON_WM_DESTROY()
 //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CWndAnim::InvalidateAllWindows ()
{

 POSITION pos = theAnimList.GetHeadPosition ();
 while (pos != NULL)
  {
  CWndAnim *pWnd = theAnimList.GetNext (pos);
  pWnd->InvalidateWindow (NULL);
  }
}

void CWndAnim::OnDestroy() 
{

 POSITION pos = theAnimList.Find (this);
 if (pos != NULL)
  theAnimList.RemoveAt (pos);

 CWndPrimary::OnDestroy();
}

