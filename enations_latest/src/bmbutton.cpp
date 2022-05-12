//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// bmbutton.cpp : implementation file
//

#include "stdafx.h"

#include <subclass.h>

#include "bmbutton.h"
#include "sfx.h"
#include "sprite.h"
#include "bitmaps.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


/////////////////////////////////////////////////////////////////////////////
// FitDrawText - smaller font if needed
void FitDrawText ( CDC * pDc, CFont & fnt, CString & sText, CRect & rect )
{

	CFont * pOldFont = pDc->SelectObject ( & fnt );

	CRect rFit ( rect );
	pDc->DrawText (sText, -1, &rFit, DT_CALCRECT | DT_LEFT | DT_NOPREFIX | DT_WORDBREAK );
	if ( (rFit.right <= rect.right) && (rFit.bottom <= rect.bottom) )
		{
		pDc->DrawText (sText, -1, &rect, DT_LEFT | DT_NOPREFIX | DT_WORDBREAK );
		pDc->SelectObject ( pOldFont );
		return;
		}

	// make it smaller
	CFont fntTemp;
	LOGFONT lf;
	memset ( &lf, 0, sizeof (lf) );
	fnt.GetLogFont ( &lf );
	int iHt = lf.lfHeight + lf.lfHeight / 2;
	while ( iHt-- > 10 )
		{
		memset ( &lf, 0, sizeof (lf) );
		fnt.GetLogFont ( &lf );
		strncpy (lf.lfFaceName, "Arial Narrow", LF_FACESIZE-1);
		lf.lfHeight = iHt;
		if (fntTemp.m_hObject != NULL)
			{
			pDc->SelectObject ( pOldFont );
			fntTemp.DeleteObject ();
			}
		fntTemp.CreateFontIndirect (&lf);

		// see if this works
		pDc->SelectObject ( &fntTemp );
		rFit = rect;
		pDc->DrawText (sText, -1, &rFit, DT_CALCRECT | DT_LEFT | DT_NOPREFIX | DT_WORDBREAK );
		if ( (rFit.right <= rect.right) && (rFit.bottom <= rect.bottom) )
			break;
		}

	pDc->DrawText (sText, -1, &rect, DT_LEFT | DT_NOPREFIX | DT_WORDBREAK );
	pDc->SelectObject ( pOldFont );
}

/////////////////////////////////////////////////////////////////////////////
// PaintBorder - paint a border

void PaintBorder ( CDIB * pDib, CRect const & rect, BOOL bBack )
{

	// draw the background
	if (bBack)
	{
		// GG		theBitmaps.GetByIndex (DIB_GOLD)->Tile ( pDib, rect, CPoint (0, 0) );

		CDIB	*pdibBackground = theBitmaps.GetByIndex( DIB_GOLD );

		CRect	rectSrc = CGlobalSubClass::GetBackgroundSrcRect( pDib->GetRect(), rect, pdibBackground->GetRect() );

		pdibBackground->StretchBlt( pDib, rect, rectSrc );
	}

	// draw the border
	CDIB * pdibHorz = theBitmaps.GetByIndex (DIB_BORDER_HORZ);
	CDIB * pdibVert = theBitmaps.GetByIndex (DIB_BORDER_VERT);
	// top
	pdibHorz->BitBlt ( pDib, rect, CPoint (0, 0) );
	// bottom
	CRect rBrdr ( rect );
	rBrdr.top = rect.bottom - pdibHorz->GetHeight ();
	pdibHorz->BitBlt ( pDib, rBrdr, CPoint (0, 0) );

	// left
	CRect rDest ( rect );
	for (int iOn=0; iOn<pdibVert->GetWidth (); iOn++)
		{
		rDest.right = rDest.left + 1;
		rDest.top ++;
		rDest.bottom --;
		pdibVert->BitBlt ( pDib, rDest, CPoint (iOn, iOn) );
		rDest.left ++;
		}

	// right
	rDest = CRect ( rect.right - pdibVert->GetWidth (), rect.top, rect.right, rect.bottom );
	rDest.left = rDest.right - 1;
	for (iOn=0; iOn<pdibVert->GetWidth (); iOn++)
		{
		rDest.right = rDest.left + 1;
		rDest.top ++;
		rDest.bottom --;
		pdibVert->BitBlt ( pDib, rDest, CPoint (iOn, iOn) );
		rDest.left --;
		}
}


/////////////////////////////////////////////////////////////////////////////
// PaintBevel - paint a beveled border

void PaintBevel (CDC & dc, CRect & rEdge, unsigned uWid, CBrush & brTop, CBrush & brBottom)
{
CRect rSide;
POINT pt[4];

	if (uWid == 0)
		return;

	// left side
	rSide.left = rEdge.left;
	rSide.top = rEdge.top;
	rSide.right = rEdge.left + uWid;
	rSide.bottom = rEdge.bottom;
	dc.FillRect (&rSide, &brTop);

	// top side
	rSide.right = rEdge.right;
	rSide.bottom = rEdge.top + uWid;
	dc.FillRect (&rSide, &brTop);

	// right side
	rSide.left = rEdge.right - uWid;
	rSide.top = rEdge.top + uWid;
	rSide.bottom = rEdge.bottom;
	dc.FillRect (&rSide, &brBottom);

	// upper right triangle
	pt[0].x = pt[3].x = pt[1].x = rEdge.right;
	pt[0].y = pt[3].y = rEdge.top;
	pt[1].y = pt[2].y = rEdge.top + uWid;
	pt[2].x = rEdge.right - uWid;
	CRgn rgn;
	rgn.CreatePolygonRgn (pt, 4, WINDING);
	dc.FillRgn (&rgn, &brBottom);
	rgn.DeleteObject ();

	// bottom side
	rSide.left = rEdge.left + uWid;
	rSide.top = rEdge.bottom - uWid;
	dc.FillRect (&rSide, &brBottom);

	// lower left triangle
	pt[0].x = pt[3].x = pt[1].x = rEdge.left + uWid;
	pt[0].y = pt[3].y = rEdge.bottom - uWid;
	pt[1].y = pt[2].y = rEdge.bottom;
	pt[2].x = rEdge.left;
	rgn.CreatePolygonRgn (pt, 4, WINDING);
	dc.FillRgn (&rgn, &brBottom);
	rgn.DeleteObject ();
}


/////////////////////////////////////////////////////////////////////////////
// CBmBtnData

CBmBtnData::CBmBtnData () 
{ 

	m_pcDib = NULL; 
	m_pDib = NULL;
}

void CBmBtnData::Init (CMmio * pMmio)
{

	ASSERT (m_pcDib == NULL);

	// read in the bitmap

	ASSERT( ptrthebltformat.Value() );

//	CSpriteDIBData dibData (CSprite::STRUCTURE, ptrthebltformat->GetBitsPerPixel () );
//	m_pcDib = new CCompressedDIB (* pMmio, dibData);

	pMmio->DescendChunk ('D', 'A', 'T', 'A');
	int cx = pMmio->ReadShort ();
	int cy = pMmio->ReadShort ();

	m_pcDib = new CDIB ( ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
				ptrthebltformat->GetMemDirection() );

	m_pcDib->Load( *pMmio );

	pMmio->AscendChunk ();

	// size of a button
	m_rDest = CRect (0, 0, cx, cy);

	// scratch space to create a button in
	m_pDib = new CDIB ( ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
																				ptrthebltformat->GetMemDirection(), cx, cy);
}

CBmBtnData::~CBmBtnData ()
{

	delete m_pcDib;
	delete m_pDib;
}


/////////////////////////////////////////////////////////////////////////////
// CTextBtnData

CTextBtnData::CTextBtnData ()
{

	m_pcDib = NULL;
}

void CTextBtnData::Init (CMmio * pMmio)
{

	ASSERT (m_pcDib == NULL);

	// read in the bitmap
//	CSpriteDIBData dibData (CSprite::STRUCTURE, ptrthebltformat->GetBitsPerPixel () );
//	m_pcDib = new CCompressedDIB (* pMmio, dibData);

	pMmio->DescendChunk ('D', 'A', 'T', 'A');
	int cx = pMmio->ReadShort ();
	int cy = pMmio->ReadShort ();

	// font & colors
	LOGFONT lf;
	memset (&lf, 0, sizeof (lf));
	lf.lfHeight = pMmio->ReadShort ();
	lf.lfWidth = lf.lfHeight / 2;
	lf.lfWeight = pMmio->ReadShort ();
	CString sName;
	pMmio->ReadString (sName);
	strcpy (lf.lfFaceName, sName);
	m_fntText.CreateFontIndirect (&lf);
	for (int iInd=0; iInd<2; iInd++)
		{
		short r = pMmio->ReadShort ();
		short g = pMmio->ReadShort ();
		short b = pMmio->ReadShort ();
		m_clr[iInd] = PALETTERGB (r, g, b);
		}

	m_pcDib = new CDIB ( ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
				ptrthebltformat->GetMemDirection() );

	m_pcDib->Load( *pMmio );

	pMmio->AscendChunk ();

	// size of bitmap
	m_rDest = CRect (0, 0, cx, cy);
}

CTextBtnData::~CTextBtnData ()
{

	delete m_pcDib;
}


/////////////////////////////////////////////////////////////////////////////
// MyButton

BEGIN_MESSAGE_MAP(CMyButton, CButton)
	//{{AFX_MSG_MAP(CMyButton)
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMyButton message handlers

CMyButton::CMyButton() 
{ 

	m_cState = 0; 
	m_pBackDib = NULL;
}

BOOL CMyButton::Create (char const * psText, int idHelp, CRect & rect, CDIB * pBackDib, CWnd *pPar, int ID)
{

	ASSERT_VALID (pPar);

	// the help text
	m_sHelp.LoadString (idHelp);

	m_pBackDib = pBackDib;

	return (CButton::Create (psText, BS_PUSHBUTTON | BS_OWNERDRAW | WS_CHILD | WS_VISIBLE, 
																																rect, pPar, ID));
}

void CMyButton::OnMouseMove (UINT nFlags, CPoint point)
{

	// tell the parent
	GetParent()->SendMessage (WM_BUTTONMOUSEMOVE, 0, (LPARAM)  GetHelp () );

	CButton::OnMouseMove(nFlags, point);
}

void CMyButton::SetToggleState (BOOL bDown)
{ 

	if (bDown) 
		m_cState |= 0x01; 
	else 
		m_cState &= ~ 0x01; 
	InvalidateRect (NULL);
}

BOOL CMyButton::OnEraseBkgnd(CDC*) 
{
	
	return (TRUE);
}

void CMyButton::OnLButtonDown(UINT nFlags, CPoint point) 
{
	
	// if its a toggle - toggle it
	if (m_cState & 0x80)
		m_cState = (m_cState & ~ 0x01) | ((~ m_cState) & 0x01);

	theMusicPlayer.PlayForegroundSound (SOUNDS::GetID (SOUNDS::button), SFXPRIORITY::selected_pri );

	CButton::OnLButtonDown(nFlags, point);
}


/////////////////////////////////////////////////////////////////////////////
// CBmButton

BEGIN_MESSAGE_MAP(CBmButton, CMyButton)
	//{{AFX_MSG_MAP(CBmButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBmButton message handlers

BOOL CBmButton::Create (int iBtnNum, int idHelp, CBmBtnData * pBbd, CRect & rect, CDIB * pBackDib, CWnd *pPar, int ID)
{

	ASSERT_VALID (pPar);

	// the row of the button in the dib
	m_iBtnNum = iBtnNum;
	m_pBmBtnData = pBbd;

	return (CMyButton::Create ("", idHelp, rect, pBackDib, pPar, ID));
}

void CBmButton::DrawItem (LPDRAWITEMSTRUCT lpDIS)
{

	int iCol = 0;
	BOOL bDown = lpDIS->itemState & ODS_SELECTED;
	if (m_cState == 0x81)		// toggle and toggleD
		bDown = ! bDown;
	if (lpDIS->itemState & (ODS_GRAYED | ODS_DISABLED))
		iCol += m_pBmBtnData->Width () * 2;
	else
		if (bDown)
			iCol += m_pBmBtnData->Width ();

	CRect rDraw (lpDIS->rcItem);
	if (rDraw.Width () > m_pBmBtnData->Width ())
		rDraw.right = rDraw.left + m_pBmBtnData->Width ();
	if (rDraw.Height () > m_pBmBtnData->Height ())
		rDraw.bottom = rDraw.top + m_pBmBtnData->Height ();

	// draw the background (for transparent areas)
	if (m_pBackDib == NULL)
		m_pBmBtnData->m_pDib->Clear ();
	else

		{
		// get the point to tile from
		CRect rMe, rPar;
		GetWindowRect (&rMe);
		GetParent ()->GetWindowRect (&rPar);
		m_pBackDib->Tile (m_pBmBtnData->m_pDib, m_pBmBtnData->m_rDest, 
															CPoint (rMe.left - rPar.left, rMe.top - rPar.top));
		}

	// draw the button
	m_pBmBtnData->m_pcDib->TranBlt (m_pBmBtnData->m_pDib, &(m_pBmBtnData->m_rDest), 
																	CPoint (iCol, m_pBmBtnData->Height () * m_iBtnNum));

	thePal.Paint (lpDIS->hDC);
	m_pBmBtnData->m_pDib->BitBlt (lpDIS->hDC, &rDraw, CPoint (0, 0));
	thePal.EndPaint (lpDIS->hDC);
}


/////////////////////////////////////////////////////////////////////////////
// CTextButton message handlers

BEGIN_MESSAGE_MAP(CTextButton, CMyButton)
	//{{AFX_MSG_MAP(CTextButton)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CTextButton::CTextButton ()
{

	m_pDib = NULL;
}

CTextButton::~CTextButton ()
{

	delete m_pDib;
}

BOOL CTextButton::Create (int idText, int idHelp, CTextBtnData * pTbd, CRect & rect, CDIB * pBackDib, CWnd *pPar, int ID)
{

	m_pTextBtnData = pTbd;

	CString sTitle;
	sTitle.LoadString (idText);

	return (CMyButton::Create (sTitle, idHelp, rect, pBackDib, pPar, ID));
}

void CTextButton::DrawItem (LPDRAWITEMSTRUCT lpDIS)
{
	CTextColors	textcolors( m_pTextBtnData->m_clr[1],
									m_pTextBtnData->m_clr[0],
									RGB( 50, 50, 50 ));

	CGlobalSubClass::DrawButton( lpDIS,
										  m_pTextBtnData->m_pcDib,	// small
										  m_pTextBtnData->m_pcDib,	// large
										  m_pBackDib,
										& m_pTextBtnData->m_fntText,
										  textcolors );

	/*  GG
	if (m_pDib == NULL)
		{
		TRAP ();
		return;
		}

	// draw the background (for transparent areas)
	if (m_pBackDib == NULL)
		m_pDib->Clear ();
	else

	{
		CRect rect;
		GetClientRect (&rect);

		// get the point to tile from
		CRect rMe, rPar;
		GetWindowRect (&rMe);
		GetParent ()->GetWindowRect (&rPar);
		m_pBackDib->Tile (m_pDib, rect, CPoint (rMe.left - rPar.left, rMe.top - rPar.top));
		}

	m_pDib->BitBlt( lpDIS->hDC, &(lpDIS->rcItem), CPoint (0, 0));

	// make a CDC
	CDC *pDc = CDC::FromHandle (lpDIS->hDC);
	if (pDc == NULL)
		return;

	// if toggled, we handle that
	BOOL bDown = lpDIS->itemState & ODS_SELECTED;
	if (m_cState == 0x81)		// toggle and toggleD
		bDown = ! bDown;
	CRect rSrc (m_pTextBtnData->m_rDest);
	if (lpDIS->itemState & (ODS_GRAYED | ODS_DISABLED))
		rSrc.OffsetRect (m_pTextBtnData->m_rDest.Width () * 2, 0);
	else
		if (bDown)
			rSrc.OffsetRect (m_pTextBtnData->m_rDest.Width (), 0);

	CRect rect;
	GetClientRect (&rect);

	// draw the background (for transparent areas)
	if (m_pBackDib == NULL)
		m_pDib->Clear ();
	else

		{
		// get the point to tile from
		CRect rMe, rPar;
		GetWindowRect (&rMe);
		GetParent ()->GetWindowRect (&rPar);
		m_pBackDib->Tile (m_pDib, rect, CPoint (rMe.left - rPar.left, rMe.top - rPar.top));
		}

	// draw the button
	m_pTextBtnData->m_pcDib->StretchTranBlt (m_pDib, &rect, &rSrc);

	// paint it
	thePal.Paint (lpDIS->hDC);
	m_pDib->BitBlt (pDc->m_hDC, &(lpDIS->rcItem), CPoint (0, 0));

	// draw the text
	CString sText;
	GetWindowText (sText);
	pDc->SetBkMode (TRANSPARENT);
	pDc->SelectObject (&(m_pTextBtnData->m_fntText));

	// in 2 so we don't write on the edge (better to crop)
	rect = lpDIS->rcItem;
	rect.InflateRect (2, 2);

	if (m_pTextBtnData->m_clr [0] == m_pTextBtnData->m_clr[1])
		{
		if (lpDIS->itemState & (ODS_GRAYED | ODS_DISABLED))
			pDc->SetTextColor (PALETTERGB (GetRValue (m_pTextBtnData->m_clr [0]) / 2,
																	GetGValue (m_pTextBtnData->m_clr [0]) / 2,
																	GetBValue (m_pTextBtnData->m_clr [0]) / 2));
		else
			pDc->SetTextColor (m_pTextBtnData->m_clr [0]);
		pDc->DrawText (sText,	-1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
	else
		{
		rect.top++;
		rect.left++;

		if (lpDIS->itemState & (ODS_GRAYED | ODS_DISABLED))
			pDc->SetTextColor (PALETTERGB (GetRValue (m_pTextBtnData->m_clr [0]) / 2,
																	GetGValue (m_pTextBtnData->m_clr [0]) / 2,
																	GetBValue (m_pTextBtnData->m_clr [0]) / 2));
		else
			pDc->SetTextColor (m_pTextBtnData->m_clr [0]);
		pDc->DrawText (sText,	-1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		rect.OffsetRect (-1, -1);

		if (lpDIS->itemState & (ODS_GRAYED | ODS_DISABLED))
			pDc->SetTextColor (PALETTERGB (GetRValue (m_pTextBtnData->m_clr [1]) / 2,
																	GetGValue (m_pTextBtnData->m_clr [1]) / 2,
																	GetBValue (m_pTextBtnData->m_clr [1]) / 2));
		else
			pDc->SetTextColor (m_pTextBtnData->m_clr [1]);
		pDc->DrawText (sText,	-1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
	*/
}

void CTextButton::OnSize(UINT nType, int cx, int cy) 
{

	CMyButton::OnSize(nType, cx, cy);
	
	// scratch space to create a button in

	// GG delete m_pDib;

	// GG m_pDib = new CDIB ( ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
	//																			ptrthebltformat->GetDirection(), cx, cy);
}
