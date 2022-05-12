//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// credits.cpp : implementation file
//

#include "stdafx.h"
#include "credits.h"
#include "lastplnt.h"
#include "sfx.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CWndCredits

CWndCredits::CWndCredits ()
{

}

CWndCredits::~CWndCredits ()
{

}

BEGIN_MESSAGE_MAP(CWndCredits, CWndBase)
	//{{AFX_MSG_MAP(CWndCredits)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CWndCredits::Create ()
{

	const char *pCls = AfxRegisterWndClass (0, NULL, NULL, theApp.LoadIcon (IDI_MAIN));

	const DWORD dwSty = WS_POPUP;
	if (CreateEx (0, pCls, theApp.m_sAppName, dwSty, 0, 0, theApp.m_iScrnX, 
												theApp.m_iScrnY, theApp.m_wndMain.m_hWnd, NULL, NULL) == 0)
		ThrowError (ERR_RES_CREATE_WND);
}

int CWndCredits::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

	GetClientRect ( &m_rText );
	int iHt = m_rText.Width () / 120;

	// get the font
	LOGFONT lf;
	memset (&lf, 0, sizeof (lf));
	lf.lfHeight = 3 * iHt;
	strcpy (lf.lfFaceName, "Newtown Italic");
	m_font[0].CreateFontIndirect (&lf);
	memset (&lf, 0, sizeof (lf));
	lf.lfHeight = 4 * iHt;
	strcpy (lf.lfFaceName, "Newtown Italic");
	m_font[1].CreateFontIndirect (&lf);
	memset (&lf, 0, sizeof (lf));
	lf.lfHeight = 5 * iHt;
	strcpy (lf.lfFaceName, "Newtown Italic");
	m_font[2].CreateFontIndirect (&lf);

	// get the text
	CMmio *pMmio = theDataFile.OpenAsMMIO ("create", "CRAT");

	pMmio->DescendRiff ('C', 'R', 'A', 'T');

	pMmio->DescendList  ( 'C', 'R', 'E', 'D' );
	pMmio->DescendChunk ('N', 'U', 'M', 'L');
	m_arrLines.SetSize ( pMmio->ReadShort () );
	pMmio->AscendChunk ();

	for (int iOn=0; iOn<m_arrLines.GetSize (); iOn++)
		{
		CCreditLine * pCcl = & m_arrLines [iOn];
		pMmio->DescendChunk ('L', 'I', 'N', 'E');
		pCcl->m_iAlign = pMmio->ReadShort ();
		pCcl->m_iSize = pMmio->ReadShort ();
		pMmio->ReadString ( pCcl->m_sText );
		pCcl->m_iRtn = pMmio->ReadShort ();
		pMmio->AscendChunk ();
		}

	delete pMmio;

	// vars for drawing
	m_iLineOn = 0;
	m_iTopHt = 3 * iHt;
	m_iOffOn = - m_rText.Height ();
	int iWid = m_rText.Width ();
	m_rText.left += iWid / 5;
	m_rText.right -= iWid / 5;

	// start the music
	theMusicPlayer.PlayExclusiveMusic (MUSIC::GetID (MUSIC::credits));

	// set a timer
	m_dwLastTime = timeGetTime ();
	SetTimer (99, m_iSpeed = 90 / iHt, NULL);

	ShowWindow (SW_SHOW);
	InvalidateRect (NULL);
	return 0;
}

void CWndCredits::OnDestroy()
{

	KillTimer ( 99 );
	m_font[0].DeleteObject ();
	m_font[1].DeleteObject ();
	m_font[2].DeleteObject ();
}

BOOL CWndCredits::OnEraseBkgnd(CDC *) 
{
	
	return (1);
}

void CWndCredits::OnPaint()
{

	CPaintDC dc(this); // device context for painting

	thePal.Paint (dc.m_hDC);

	dc.SetBkColor ( PALETTERGB (0, 0, 0) );
	dc.SetTextColor ( PALETTERGB (255, 255, 255) );

	CRect rect (m_rText);
	rect.top -= m_iOffOn;
	int iLeft = rect.left;
	int iRight = rect.right;
	int iBottom = rect.bottom;

	CBrush * pbrBlack = CBrush::FromHandle ( (HBRUSH) ::GetStockObject ( BLACK_BRUSH ) );
	if (pbrBlack == NULL)
		{
		thePal.EndPaint (dc.m_hDC);
		return;
		}
	// the top
	if (rect.top > 0)
		{
		CRect rSide (iLeft, 0, iRight, rect.top);
		dc.FillRect ( &rSide, pbrBlack );
		}
	// the sides
	CRect rSide;
	GetClientRect (&rSide);
	rect.left = iRight;
	dc.FillRect ( &rSide, pbrBlack );
	rSide.left = 0;
	rSide.right = iLeft;
	dc.FillRect ( &rSide, pbrBlack );

	CFont * pOldFont = dc.SelectObject ( &m_font [0] );

	BOOL bFigRect = TRUE;
	for (int iOn=m_iLineOn; iOn<m_arrLines.GetSize () && rect.Height()>0; iOn++)
		{
		CCreditLine * pCcl = & m_arrLines [iOn];
		dc.SelectObject ( &m_font [pCcl->m_iSize] );

		if (bFigRect)
			{
			rect.bottom = iBottom;
			dc.DrawText (pCcl->m_sText, -1, &rect, DT_CALCRECT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_LEFT);
			rect.left = iLeft;
			rect.right = iRight;
			if (! pCcl->m_iRtn)
				rect.bottom += rect.Height () / 3;
			}

		dc.DrawText (pCcl->m_sText, -1, &rect, DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX |
						(pCcl->m_iAlign == 0 ? DT_LEFT : (pCcl->m_iAlign == 1 ? DT_CENTER : DT_RIGHT)) );

		int iHt = rect.Height () + rect.Height () / 5;
		if (iOn == m_iLineOn)
			m_iTopHt = iHt;
		if (pCcl->m_iRtn)
			{
			bFigRect = TRUE;
			int i = rect.top + iHt;
			rect.top = rect.bottom;
			rect.bottom = i;
			dc.FillRect ( &rect, pbrBlack );

			rect.top = i;
			rect.bottom = iBottom;
			}
		else
			bFigRect = FALSE;
		}

	// the bottom
	if (rect.Height () > 0)
		dc.FillRect ( &rect, pbrBlack );

	dc.SelectObject ( pOldFont );
	thePal.EndPaint (dc.m_hDC);
}

void CWndCredits::OnTimer(UINT nIDEvent) 
{

	DWORD dwTime = timeGetTime ();
	int iNum = ( dwTime - m_dwLastTime ) / m_iSpeed;
	m_dwLastTime = dwTime;
	iNum = __max ( 1, iNum );
	int iTmp = iNum;

	while ( iTmp-- > 0 )
		{
		m_iOffOn ++;
		if (m_iOffOn > m_iTopHt)
			{
			m_iOffOn -= m_iTopHt;
			CCreditLine * pCcl;
			do
				{
				pCcl = & m_arrLines [m_iLineOn];
				m_iLineOn ++;

				if (m_iLineOn >= m_arrLines.GetSize ())
					break;
				}
			while ( ! pCcl->m_iRtn);
			}

		if (m_iLineOn >= m_arrLines.GetSize ())
			{
			KillTimer ( 99 );
			DestroyWindow ();
			theApp.CreateMain ();
			return;
			}
		}

	ScrollWindow ( 0, - iNum, &m_rText );
  
	CRect rect (m_rText);
	rect.top = rect.bottom - iNum;
	InvalidateRect (&rect);
	UpdateWindow ();

	CWndBase::OnTimer (nIDEvent);
}

void CWndCredits::OnKeyDown(UINT , UINT , UINT )
{

	KillTimer ( 99 );
	DestroyWindow ();
	theApp.CreateMain ();
}

void CWndCredits::OnLButtonDown(UINT , CPoint ) 
{

	KillTimer ( 99 );
	DestroyWindow ();
	theApp.CreateMain ();
}

void CWndCredits::OnMButtonDown(UINT , CPoint )
{

	TRAP ();
	KillTimer ( 99 );
	DestroyWindow ();
	theApp.CreateMain ();
}

void CWndCredits::OnRButtonDown(UINT , CPoint )
{

	KillTimer ( 99 );
	DestroyWindow ();
	theApp.CreateMain ();
}
