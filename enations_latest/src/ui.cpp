//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// area.cpp : area window
//

#include "stdafx.h"
#include "lastplnt.h"
#include "world.h"
#include "bitmaps.h"

#include "ui.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


#ifdef BUGBUG
/////////////////////////////////////////////////////////////////////////////
// CWndBtnStatic

CWndBtnStatic::CWndBtnStatic (int iNumBtns, BOOL bStatus)
{

	m_iBtnEnd = bStatus ? 0 : -1;
	m_pWnd = NULL;
	m_rgb[0] = m_rgb[1] = RGB (0,0,0);

	ASSERT (iNumBtns > 0);
	m_pBmBtns = new CBmButton [m_iNumBtns = iNumBtns];
}

BOOL CWndBtnStatic::PreCreate ()
{

	// make sure we have enough room for all the buttons
	CRect rect (0, 0, theBmBtnData.Width () * m_iNumBtns, theBmBtnData.Height ());
	int iTmp = rect.Width ();
	AdjustWindowRect (&rect, dwStatusWndStyle, FALSE);
	m_iXmin = rect.Width ();
	m_iYmin = rect.Height ();

	if (m_iBtnEnd != -1)
		{
		m_iBtnEnd = iTmp;
		CRect rect (0, 0, m_iXmin + theApp.TextWid () * 40, m_iYmin);
		AdjustWindowRect (&rect, dwStatusWndStyle, FALSE);
		m_iXmin = rect.Width ();
		m_iYmin = rect.Height ();
		}

	// finally, if its wider than the screen - we bring it in
	m_iXmin = __min (m_iXmin, theApp.m_iScrnX);

	return (TRUE);
}

CWndBtnStatic::~CWndBtnStatic()
{

	delete [] m_pBmBtns;
}

BEGIN_MESSAGE_MAP(CWndBtnStatic, CWndBase)
	//{{AFX_MSG_MAP(CWndBtnStatic)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWndBtnStatic message handlers

BOOL CWndBtnStatic::OnCommand (WPARAM wParam, LPARAM lParam)
{

	if (HIWORD (wParam) == BN_CLICKED)
		{
		GetParent()->SendMessage (WM_COMMAND, wParam, lParam);
		return (TRUE);
		}

	return (CWndBase::OnCommand (wParam, lParam));
}

int CWndBtnStatic::OnCreate(LPCREATESTRUCT lpCS)
{
	if (CWndBase::OnCreate(lpCS) == -1)
		return -1;

	// create the buttons
	CRect rect (0, 0, lpCS->cx, lpCS->cy);
	rect.right = rect.left + theBmBtnData.Width ();
	rect.bottom -= theApp.FlatDimen ();
	rect.top = rect.bottom - theBmBtnData.Height ();

	CWndBtnBar * pPar = (CWndBtnBar *) GetParent ();

	CBmButton * pBtn = m_pBmBtns;
	for (int iOn=0; iOn<m_iNumBtns; iOn++, pBtn++)
		{
		pBtn->Create (pPar->GetBtn (iOn), pPar->GetHelp (iOn), &theBmBtnData, rect, NULL, this, pPar->GetID (iOn));
		rect.OffsetRect (theBmBtnData.Width (), 0);
		}

	return 0;
}

void CWndBtnStatic::SetExtra (CWnd *pWnd)
{

	ASSERT_VALID (this);
	ASSERT_VALID (pWnd);

	m_pWnd = pWnd;
	CRect rect;
	pWnd->GetClientRect (&rect);
	int iWid = rect.Width ();

	// adjust minimum window size
	m_iXmin += iWid;
	m_iXmin = __min (m_iXmin, theApp.m_iScrnX);

	// adjust status window size
	m_rcStatus.right -= iWid;

	SizeStatus ();
	InvalidateRect (NULL);
}

void CWndBtnStatic::OnPaint()
{

	ASSERT_VALID (this);
	CPaintDC dc(this); // device context for painting

	CBrush brFace (GetSysColor (COLOR_BTNFACE));
	CBrush brBottom (GetSysColor (COLOR_BTNHIGHLIGHT));

	CRect rect;
	CWndBase::GetClientRect (&rect);

	// bevel above bottom bar
	CPen pen;
	pen.CreateStockObject (BLACK_PEN);
	dc.SelectObject (&pen);
	dc.MoveTo (0, 0);
	dc.LineTo (rect.right, 0);
	rect.top++;
	int iBottom = rect.bottom;
	rect.bottom = rect.top + theApp.BevelDimen ();
	dc.FillRect (&rect, &brBottom);

	// grey background for bottom bar
	rect.top = rect.bottom;
	rect.bottom = iBottom;
	dc.FillRect (&rect, &brFace);

	if (m_iBtnEnd == -1)
		return;

	// draw the status bars
	dc.SelectObject (&theApp.TextFont ());
	dc.SetBkColor (GetSysColor (COLOR_BTNFACE));

	CBrush brTop (GetSysColor (COLOR_BTNSHADOW));

	// draw the status window
	rect = m_rcStatus;
	rect.InflateRect (theApp.BevelDimen (), theApp.BevelDimen ());
	PaintBevel (dc, rect, theApp.BevelDimen (), brTop, brBottom);

	// draw the text
	rect = m_rcStatus;
	rect.left += theApp.BevelDimen ();
	rect.bottom = rect.top + theApp.TextHt ();
	dc.SetTextColor (m_rgb[0]);
	dc.ExtTextOut (rect.left, rect.top, ETO_CLIPPED | ETO_OPAQUE, &rect,
												m_sStatus[0], m_sStatus[0].GetLength (), NULL);
	rect.OffsetRect (0, theApp.TextHt ());
	dc.SetTextColor (m_rgb[1]);
	dc.ExtTextOut (rect.left, rect.top, ETO_CLIPPED | ETO_OPAQUE, &rect,
												m_sStatus[1], m_sStatus[1].GetLength (), NULL);

	// Do not call CWndBase::OnPaint() for painting messages
}

void CWndBtnStatic::OnSize(UINT nType, int cx, int cy)
{

	CWndBase::OnSize(nType, cx, cy);

	if (m_iBtnEnd == -1)
		return;

	SizeStatus ();
}

void CWndBtnStatic::SizeStatus ()
{

	if (m_iBtnEnd == -1)
		return;

	// adjust status text window
	CWndBase::GetClientRect (&m_rcStatus);
	m_rcStatus.left = m_iBtnEnd + theApp.FlatDimen ();
	m_rcStatus.right -= theApp.BevelDimen ();
	m_rcStatus.bottom -= theApp.FlatDimen () + theApp.BevelDimen ();
	m_rcStatus.top = m_rcStatus.bottom - theApp.TextHt () * 2;

	if ((m_pWnd != NULL) && (m_pWnd->m_hWnd != NULL))
		{
		CRect rect;
		m_pWnd->GetClientRect (&rect);
		m_rcStatus.right -= rect.Width ();
		}
}

void CWndBtnStatic::EnableButton (int ID, BOOL bEnable)
{

	CWnd *pBtn = GetDlgItem (ID);
	ASSERT_VALID (pBtn);
	pBtn->EnableWindow (bEnable);
}


/////////////////////////////////////////////////////////////////////////////
// CWndBtnBar

CBmButton * CWndBtnBar::GetBtnWnd (int iBtn)
{

	ASSERT_VALID (this);
	ASSERT ((0 <= iBtn) && (iBtn < m_WndStatic.m_iNumBtns));

	return (m_WndStatic.m_pBmBtns + iBtn);
}

BOOL CWndBtnBar::PreCreateWindow (CREATESTRUCT & cs)
{

	CWndAnim::PreCreateWindow (cs);

	// get the mins for the static window
	m_WndStatic.PreCreate ();

	CRect rect (0, 0, m_WndStatic.m_iXmin, 
											m_WndStatic.m_iYmin + theBmBtnData.Height ());
	AdjustWindowRect (&rect, dwPopWndStyle, FALSE);
	m_iXmin = rect.Width ();
	m_iYmin = rect.Height ();

	return (TRUE);
}

void CWndBtnBar::GetClientRect (LPRECT lpRect) const
{

	CWndAnim::GetClientRect (lpRect);
	CRect rStatic;
	m_WndStatic.GetClientRect (&rStatic);
	if (rStatic.Height () < lpRect->bottom)
		lpRect->bottom -= rStatic.Height ();
	else
		lpRect->bottom = 0;
}

BEGIN_MESSAGE_MAP(CWndBtnBar, CWndBase)
	//{{AFX_MSG_MAP(CWndBtnBar)
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_WM_SYSCOMMAND()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndBtnBar message handlers

int CWndBtnBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

	if (CWndAnim::OnCreate(lpCreateStruct) == -1)
		return -1;

	// button & status bar
	CRect rect;
	CWndAnim::GetClientRect (&rect);
	rect.top = rect.bottom - m_WndStatic.m_iYmin;
	m_WndStatic.Create (NULL, NULL, dwStatusWndStyle, rect, this, 0, NULL);

	return 0;
}

void CWndBtnBar::OnSize(UINT nType, int cx, int cy)
{

	ASSERT_VALID (this);
	CWndAnim::OnSize(nType, cx, cy);

	// move/re-size the status window
	m_WndStatic.SetWindowPos (NULL, 0, cy - m_WndStatic.m_iYmin, cx,
												m_WndStatic.m_iYmin, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CWndBtnBar::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{

	// we limit how small it can be
	if (lpMMI->ptMinTrackSize.x < m_iXmin)
		lpMMI->ptMinTrackSize.x = m_iXmin;
	if (lpMMI->ptMinTrackSize.y < m_iYmin)
		lpMMI->ptMinTrackSize.y = m_iYmin;

	CWndAnim::OnGetMinMaxInfo(lpMMI);
}

void CWndBtnBar::OnSysCommand(UINT nID, LPARAM lParam)
{

	// for minimize hide it
	if ((nID == SC_MINIMIZE) || (nID == SC_CLOSE))
		{
		ShowWindow (SW_HIDE);
		return;
		}

	CWndAnim::OnSysCommand(nID, lParam);
}
#endif



/////////////////////////////////////////////////////////////////////////////
// CWndBtnStatusBar message handlers




