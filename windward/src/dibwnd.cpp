/*
/----------------------------------------------------------------
|
|      DIBWND.CPP     Window containing a bitmap.
|
|      Child window containing a bitmap. Handles
|      scrolling and resize.
|
|      Copyright (c) 1996-1998 Ulrich von Zadow
|
----------------------------------------------------------------
*/
#include "stdafx.h"
#include "dibwnd.h"
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
//////////////////////////////////////////////////////////////////////
// CDIBWnd
IMPLEMENT_DYNAMIC (CDIBWnd, CWnd);
//CTOR in H takes nothing.
CDIBWnd::CDIBWnd
    ( CWinBmp * pBmp
    )
  : m_BmpPos (0,0)
{
  m_pBmp = pBmp;
  m_bScrolling = FALSE;
  m_BmpSize =  m_pBmp->GetSize();
}
CDIBWnd::~CDIBWnd
    ()
{
}
void CDIBWnd::NewDIBNotify
    ()
    // Call this whenever the bitmap is changed outside of the object.
{
  CRect rect;
  GetClientRect (&rect);
  int cx = rect.right;
  int cy = rect.bottom;
  m_BmpSize =  m_pBmp->GetSize();
  m_BmpPos = CPoint (0, 0);
  initScrollBars (cx, cy);
  InvalidateRect (FALSE);
}
/////////////////////////////////////////////////////////////////////////////
// CDIBWnd operations
BEGIN_MESSAGE_MAP(CDIBWnd, CWnd)
  //{{AFX_MSG_MAP(CDIBWnd)
  ON_WM_CREATE()
  ON_WM_SIZE()
  ON_WM_VSCROLL()
  ON_WM_PAINT()
  ON_WM_HSCROLL()
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CDIBWnd message handlers
int CDIBWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CWnd::OnCreate(lpCreateStruct) == -1)
    return -1;
  return 0;
}
void CDIBWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
  m_BmpPos.y = handleScroll (SB_VERT, nSBCode, nPos,
                             m_Size.cy, m_BmpSize.cy, m_BmpPos.y);
  CWnd::OnVScroll (nSBCode, nPos, pScrollBar);
}
void CDIBWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
  m_BmpPos.x = handleScroll (SB_HORZ, nSBCode, nPos,
                             m_Size.cx, m_BmpSize.cx, m_BmpPos.x);
  CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}
BOOL CDIBWnd::PreCreateWindow (CREATESTRUCT& cs)
{
  cs.style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL;
  return CWnd::PreCreateWindow (cs);
}
void CDIBWnd::OnSize(UINT nType, int cx, int cy)
{
  CWnd::OnSize(nType, cx, cy);
  m_Size = CSize (cx, cy);
  initScrollBars (cx, cy);
  InvalidateRect (FALSE);
}
void CDIBWnd::OnPaint()
{
  CPaintDC DC(this); // device context for painting
  // Hintergrund
  if (!m_bScrolling &&
      (m_Size.cx>m_BmpSize.cx || m_Size.cy>m_BmpSize.cy))
  {
    CRect Rect;
    GetClientRect (&Rect);
    Rect.InflateRect (2, 2); // Ist n?ig, sonst fehlt was ;-)
    CPen * pOldPen = (CPen *) DC.SelectStockObject (NULL_PEN);
    CBrush * pOldBrush = (CBrush *) DC.SelectStockObject (LTGRAY_BRUSH);
    DC.Rectangle (&Rect);
    DC.SelectObject (pOldPen);
    DC.SelectObject (pOldBrush);
  }
  else
    m_bScrolling = FALSE;
  // Bitmap
  m_pBmp->Draw (&DC, -m_BmpPos.x, -m_BmpPos.y);
}
/////////////////////////////////////////////////////////////////////////////
// CDIBWnd internals
void CDIBWnd::initScrollBars
    ( int cx,
      int cy
    )
{
  initScrollBar (SB_VERT, cy, m_BmpSize.cy, m_BmpPos.y);
  initScrollBar (SB_HORZ, cx, m_BmpSize.cx, m_BmpPos.x);
}
void CDIBWnd::initScrollBar
    ( int ID,
      int Page,
      int Range,
      int Pos
    )
{
  if (Page >= Range)
    ShowScrollBar (ID, FALSE);
  else
  {
    ShowScrollBar (ID, TRUE);
    SCROLLINFO SI;
    SI.cbSize = sizeof (SCROLLINFO);
    SI.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
    SI.nMin = 0;
    SI.nMax = Range-1;
    SI.nPage = Page;
    SetScrollInfo (ID, &SI, FALSE);
    SetScrollPos (ID, Pos);
  }
}
int CDIBWnd::handleScroll
    ( int ID,
      UINT nSBCode,
      UINT nPos,
      int Page,
      int Range,
      int Pos
    )
{
  int NewPos = Pos;
  switch (nSBCode)
  {
    case SB_TOP:
      NewPos = 0;
      break;
    case SB_BOTTOM:
      NewPos = Range-Page;
      break;
    case SB_LINEUP:
      NewPos -= 10;
      break;
    case SB_LINEDOWN:
      NewPos += 10;
      break;
    case SB_PAGEUP:
      NewPos -= Range;
      break;
    case SB_PAGEDOWN:
      NewPos += Range;
      break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
      if (Pos == (int) nPos) return Pos;
      NewPos = nPos;
      break;
    default:
      // Ignore SB_ENDSCROLL.
      return Pos;
  }
  if (NewPos > Range-Page)
    NewPos = Range-Page;
  if (NewPos < 0)
    NewPos = 0;
  m_bScrolling = TRUE;
  InvalidateRect (FALSE);
  SetScrollPos (ID, NewPos);
  return NewPos;
}