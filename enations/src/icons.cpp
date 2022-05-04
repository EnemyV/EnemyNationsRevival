//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// icons.cpp : implementation file
//

#include "stdafx.h"

#include <blt.h>

#include "icons.h"
#include "sprite.h"
#include "lastplnt.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CBitmapLib

CBitmapLib::CBitmapLib ()
{

	m_iNumDibs = 0;
	m_ppDibs = NULL;
}

CBitmapLib::~CBitmapLib ()
{

	CDIB * * ppDib = m_ppDibs;
	for (int ID=0; ID<m_iNumDibs; ID++, ppDib++)
		delete (*ppDib);
	delete [] m_ppDibs;

	m_iNumDibs = 0;
	m_ppDibs = NULL;
}

CDIB * CBitmapLib::GetByIndex (int iInd)
{

	ASSERT ((0 <= iInd) && (iInd < m_iNumDibs));
	return (* (m_ppDibs + iInd));
}

void CBitmapLib::Init (CMmio * pMmio)
{

	ASSERT (m_ppDibs == NULL);

	// get how many
	pMmio->DescendChunk ('N', 'U', 'M', 'D');
	m_iNumDibs = pMmio->ReadShort ();
	pMmio->AscendChunk ();
	m_ppDibs = new CDIB * [m_iNumDibs];

	CDIB * * ppDib = m_ppDibs;
	for (int ID=0; ID<m_iNumDibs; ID++, ppDib++)
		{
		pMmio->DescendChunk ('D', 'I', 'B', 'B');
		// read in the bitmap
		ASSERT( ptrthebltformat.Value() );

//		CSpriteDIBData dibData (CSprite::STRUCTURE, ptrthebltformat->GetBitsPerPixel () );
//	m_pcDib = new CCompressedDIB (* pMmio, dibData);

		(*ppDib) = new CDIB ( ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
								ptrthebltformat->GetMemDirection() );
		( *ppDib )->Load( *pMmio );

		pMmio->AscendChunk ();
		}
}

void CBitmapLib::Close ()
{

	TRAP ();
	CDIB * * ppDib = m_ppDibs;
	for (int ID=0; ID<m_iNumDibs; ID++, ppDib++)
		delete (* ppDib);

	delete [] m_ppDibs;
	m_ppDibs = NULL;
	m_iNumDibs = 0;
}


/////////////////////////////////////////////////////////////////////////////
// CStatData

CStatData::CStatData ()
{


	m_ID = -1;
	m_leftOff = m_rightOff = 0;
	m_cxIcon = m_cyIcon = m_nNeedIcon = 1;
	m_iTypIcon = done;

	m_cxLeft = m_cxBack = m_cxRight = m_cyBack = 1;
	m_iTypBack = tile;

	m_pcDib = NULL;
	m_pFnt = NULL;
	for (int iInd=0; iInd<6; iInd++)
		m_clr[iInd] = PALETTERGB (0, 0, 0);
}

CStatData::~CStatData ()
{

	delete m_pcDib;
	delete m_pFnt;
}

void CStatData::Init (CMmio * pMmio)
{

	pMmio->DescendChunk ('I', 'C', 'O', 'N');
	m_ID = pMmio->ReadShort ();
	m_leftOff = pMmio->ReadShort ();
	m_rightOff = pMmio->ReadShort ();
	m_cxIcon = pMmio->ReadShort ();
	m_cyIcon = pMmio->ReadShort ();
	m_nNeedIcon = pMmio->ReadShort ();
	m_iTypIcon = (CStatData::TYP_ICON) pMmio->ReadShort ();

	m_cxLeft = pMmio->ReadShort ();
	m_cxBack = pMmio->ReadShort ();
	m_cxRight = pMmio->ReadShort ();
	m_cyBack = pMmio->ReadShort ();
	m_iTypBack = (CStatData::TYP_BACK) pMmio->ReadShort ();

	// fonts for the icons
	if ( (m_iTypIcon == CStatData::text) || (m_iTypIcon == CStatData::text_right) )
		{
		int iHt = pMmio->ReadShort ();
		int iWt = pMmio->ReadShort ();
		CString sName;
		pMmio->ReadString (sName);
		m_pFnt = new CFont ();

		if (iWt == -1)
			{
			// get the main font - we try sName, then Arial, then Arial condensed till we fit
			for (int iTry=0; iTry<10; iTry++)
				{
				LOGFONT lf;
				memset (&lf, 0, sizeof (lf));
				switch (iTry)
				  {
					case 0 :
					case 1 :
						lf.lfHeight = iHt - 2 * iTry;
						strncpy (lf.lfFaceName, sName, LF_FACESIZE-1);
						break;
					case 2 :
					case 3 :
						lf.lfHeight = iHt - 2 * ( iTry - 2 );
						strncpy (lf.lfFaceName, "Arial", LF_FACESIZE-1);
						break;
					default:
						lf.lfHeight = iHt - ( iTry - 4 );
						strncpy (lf.lfFaceName, "Arial Narrow", LF_FACESIZE-1);
						break;
				  }

				if (m_pFnt->m_hObject != NULL)
					m_pFnt->DeleteObject ();
				m_pFnt->CreateFontIndirect (&lf);

				// see if it will fit
				CClientDC dc ( NULL );
				CFont * pOldFont = dc.SelectObject (m_pFnt);
				CString sTest;
				sTest.LoadString (IDS_LONGEST_STRING);
				CRect rect (0, 0, theApp.m_iScrnX, theApp.m_iScrnY);
				dc.DrawText (sTest, -1, &rect, DT_CALCRECT | DT_LEFT | DT_SINGLELINE);
				dc.SelectObject ( pOldFont );
				if (rect.Width () + rect.Width () / 8 < theApp.m_iScrnX / 2)
					break;
				}
			}
		else

			{
			LOGFONT lf;
			memset (&lf, 0, sizeof (lf));
			lf.lfHeight = iHt;
			lf.lfWeight = iWt;
			strcpy (lf.lfFaceName, sName);
			m_pFnt->CreateFontIndirect (&lf);
			}

		for (int iInd=0; iInd<6; iInd+=2)
			{
			short r = pMmio->ReadShort ();
			short g = pMmio->ReadShort ();
			short b = pMmio->ReadShort ();
			m_clr[iInd] = PALETTERGB (r, g, b);
			m_clr[iInd+1] = PALETTERGB (r/2, g/2, b/2);
			}
		}

	delete m_pcDib;
	m_pcDib = new CDIB ( ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
							ptrthebltformat->GetMemDirection() );

	m_pcDib->Load( *pMmio );
	pMmio->AscendChunk ();
}


/////////////////////////////////////////////////////////////////////////////
// CIcons

CIcons::CIcons ()
{

	m_iNumBars = 0;
	m_pStatData = NULL;
}

CStatData * CIcons::GetByIndex (int ID)
{

	CStatData * pSb = m_pStatData;
	for (int iInd=0; iInd<m_iNumBars; iInd++, pSb++)
		if (pSb->m_ID == ID)
			return (pSb);

	// better than a GP fault
	ASSERT (FALSE);
	return (m_pStatData);
}

void CIcons::Init (CMmio * pMmio)
{

	ASSERT (m_pStatData == NULL);

	// get how many
	pMmio->DescendChunk ('N', 'U', 'M', 'I');
	m_iNumBars = pMmio->ReadShort ();
	pMmio->AscendChunk ();
	m_pStatData = new CStatData [m_iNumBars];

	CStatData * pSd = m_pStatData;
	for (int ID=0; ID<m_iNumBars; ID++, pSd++)
		pSd->Init ( pMmio );
}

void CIcons::Close ()
{

	delete [] m_pStatData;
	m_pStatData = NULL;
	m_iNumBars = 0;
}


/////////////////////////////////////////////////////////////////////////////
// CStatInst

CStatInst::CStatInst () : m_ptBackOff (0, 0), m_rDest (0, 0, 0, 0)
{

	m_iPerDone = m_iNeed = m_iIconOn = m_iBaseOn = 0;
	m_pDib = NULL;
	m_pStatData = NULL;
	m_iImp = status;
	m_pBackDib = NULL;

	m_iImp = status;
}

CStatInst::~CStatInst ()
{

	delete m_pDib;
}

void CStatInst::Attach (CIcons * pIcons, int iIndex)
{

	m_pStatData = pIcons->GetByIndex (iIndex);
}

void CStatInst::SetPerBase (int iIcon)
{

	m_iBaseOn = iIcon * m_pStatData->m_cxIcon;
}

void CStatInst::DrawBackground () const
{

	// a 0-based rect
	CRect rBase (m_rDest);
	rBase.OffsetRect ( - rBase.left, - rBase.top );

	// draw the background (for transparent areas)
	if (m_pBackDib != NULL)
		m_pBackDib->Tile (m_pDib, rBase, m_ptBackOff);

	switch (m_pStatData->m_iTypBack)
	  {
		case CStatData::full_back : {
			CRect rBack (0, m_pStatData->m_cyIcon, m_pStatData->m_cxBack, m_pStatData->m_cyIcon + m_pStatData->m_cyBack);
			m_pStatData->m_pcDib->StretchTranBlt (m_pDib, rBase, rBack);
			return; }

		case CStatData::back_3 : {
			// left
			CRect rBack (0, 0, m_pStatData->m_cxLeft, m_pStatData->m_cyBack);
			rBack.bottom = __min (rBack.bottom, m_pDib->GetHeight ());
			rBack.right = __min (rBack.right, m_pDib->GetWidth ());
			m_pStatData->m_pcDib->TranBlt (m_pDib, rBack, CPoint (0, m_pStatData->m_cyIcon));

			// center
			rBack.OffsetRect (m_pStatData->m_cxLeft, 0);
			rBack.right = rBack.left + m_pStatData->m_cxBack;
			CPoint ptTile (m_pStatData->m_cxLeft, m_pStatData->m_cyIcon);
			int iEnd = rBase.right - m_pStatData->m_cxRight;
			while (rBack.left < iEnd)
				{
				rBack.right = __min (rBack.right, m_pDib->GetWidth ());
				m_pStatData->m_pcDib->TranBlt (m_pDib, rBack, ptTile);
				rBack.OffsetRect (m_pStatData->m_cxBack, 0);
				}

			// right
			rBack.right = __min (rBase.right, m_pDib->GetWidth ());
			rBack.left = rBase.right - m_pStatData->m_cxRight;
			rBack.left = __max (rBack.left, rBase.right / 2);
			m_pStatData->m_pcDib->TranBlt (m_pDib, rBack, 
							CPoint (m_pStatData->m_cxLeft + m_pStatData->m_cxBack, m_pStatData->m_cyIcon));
			return; }

		case CStatData::tile : {
			for (int y=0; y<m_pDib->GetHeight(); y+=m_pStatData->m_cyBack)
				{
				CRect rBack (0, y, m_pStatData->m_cxBack, y + m_pStatData->m_cyBack);
				rBack.bottom = __min (rBack.bottom, m_pDib->GetHeight ());

				for (int x=0; x<m_pDib->GetWidth(); x+=m_pStatData->m_cxBack)
					{
					rBack.right = __min (rBack.right, m_pDib->GetWidth ());
					m_pStatData->m_pcDib->TranBlt (m_pDib, rBack, CPoint (0, m_pStatData->m_cyIcon));
					rBack.OffsetRect (m_pStatData->m_cxBack, 0);
					}
				}
			return; }

#ifdef _DEBUG
		default:
			ASSERT (FALSE);
#endif
	  }

}

void CStatInst::DrawStatDone () const
{

	// show the % done
	ASSERT ((0 <= m_iPerDone) && (m_iPerDone <= 100));
	CRect rPer (m_rDest);
	rPer.OffsetRect ( - m_rDest.left, - m_rDest.top );

	rPer.top += (m_rDest.Height () - m_pStatData->m_cyIcon) / 2;
	rPer.bottom = rPer.top + m_pStatData->m_cyIcon;
	rPer.left += m_pStatData->m_leftOff;
	rPer.right -= m_pStatData->m_rightOff;
	int iEnd = rPer.right;
	if (m_iPerDone < 100)		// last only when hit 100
		iEnd -= m_pStatData->m_cxIcon / 2;
	int iRight = (rPer.Width () * m_iPerDone) / 100;
	if (m_iPerDone > 0)			// get at least 1 if non zero
		iRight = __max (rPer.left + 1, iRight);
	rPer.right = rPer.left + m_pStatData->m_cxIcon;

	int iAdd = m_pStatData->m_cxIcon / 2;
	for (int x = rPer.left; x < iRight; x += m_pStatData->m_cxIcon / 2)
		{
		if (rPer.right > iEnd)
			break;
		m_pStatData->m_pcDib->TranBlt (m_pDib, rPer, CPoint (m_iBaseOn, 0));
		rPer.OffsetRect (iAdd, 0);
		}
}

void CStatInst::DrawStatHave () const
{

	// if we have nothing - show nothing
	if ((m_iPerDone <= 0) && (m_iNeed <= 0))
		return;

	// do we have enough?
	int iDone;
	CPoint ptNeed;
	if ((m_iPerDone < m_iNeed) && (m_iNeed > 0))		// not enough
		{
		iDone = (m_iPerDone * 100) / m_iNeed;
		ptNeed = CPoint (m_pStatData->m_cxIcon * (2 + m_iIconOn), 0);
		}
	else		// too much
		{
		if ((m_iPerDone <= 0) || (m_iNeed <= 0))
			iDone = 100;
		else
			{
			iDone = (m_iNeed * 100) / m_iPerDone;
			if ((iDone == 0) && (m_iNeed > 0))
				iDone = 1;
			}
		ptNeed = CPoint (m_pStatData->m_cxIcon, 0);
		}

	// show the % done
	ASSERT ((0 <= iDone) && (iDone <= 100));
	CRect rPer (m_rDest);
	rPer.OffsetRect ( - m_rDest.left, - m_rDest.top );

	rPer.top += (m_rDest.Height () - m_pStatData->m_cyIcon) / 2;
	rPer.bottom = rPer.top + m_pStatData->m_cyIcon;
	if (m_pStatData->m_iTypBack == CStatData::full_back)
		{
		rPer.left += (m_pStatData->m_leftOff * m_rDest.Width () + m_pStatData->m_cxBack - 1) / m_pStatData->m_cxBack;
		rPer.right -= (m_pStatData->m_rightOff * m_rDest.Width () + m_pStatData->m_cxBack - 1) / m_pStatData->m_cxBack;
		}
	else
		{
		rPer.left += m_pStatData->m_leftOff;
		rPer.right -= m_pStatData->m_rightOff;
		}

	// set up for have icon
	int iRight = (rPer.Width () * iDone) / 100;
	CRect rIcon (rPer);
	rIcon.right = rIcon.left + m_pStatData->m_cxIcon;

	int iAdd = m_pStatData->m_cxIcon / 2;
	while (rIcon.left < iRight)
		{
		if (rIcon.right > rPer.right)
			break;
		m_pStatData->m_pcDib->TranBlt (m_pDib, rIcon, CPoint (0, 0));
		rIcon.OffsetRect (iAdd, 0);
		}

	// show the remainder
	while (rIcon.left < rPer.right)
		{
		if (rIcon.right > rPer.right)
			break;
		m_pStatData->m_pcDib->TranBlt (m_pDib, rIcon, ptNeed);
		rIcon.OffsetRect (iAdd, 0);
		}
}

void CStatInst::DrawStatText () const
{

	if (m_sText.IsEmpty ())
		return;

	CRect rText (m_rDest);
	rText.OffsetRect ( - m_rDest.left, - m_rDest.top );

	rText.left += m_pStatData->m_leftOff;
	rText.right -= m_pStatData->m_rightOff;

	// we need a DC so we can draw text
	CDC * pDc = CDC::FromHandle (m_pDib->GetDC ());
	if (pDc == NULL)
		{
		ASSERT (FALSE);
		return;
		}
	thePal.Paint (pDc->m_hDC);

	int dtStyle = (m_pStatData->m_iTypIcon == CStatData::text_right ? DT_RIGHT : DT_LEFT) | DT_SINGLELINE | DT_VCENTER;

	// no font - just draw it
	pDc->SetBkMode (TRANSPARENT);
	if (m_pStatData->m_pFnt == NULL)
		{
		pDc->DrawText (m_sText, -1, rText, dtStyle);
		return;
		}

	// select the font - smaller if necessary
	CFont * pOld = pDc->SelectObject (m_pStatData->m_pFnt);
	CFont fntTemp;
	CRect rFit ( rText );
	pDc->DrawText (m_sText, -1, &rFit, DT_CALCRECT | dtStyle );
	if ( (rFit.right > rText.right) || (rFit.bottom > rText.bottom) )
		{
		// make it smaller
		LOGFONT lf;
		memset ( &lf, 0, sizeof (lf) );
		m_pStatData->m_pFnt->GetLogFont ( &lf );
		int iHt = lf.lfHeight;
		while ( iHt-- > 10 )
			{
			memset ( &lf, 0, sizeof (lf) );
			m_pStatData->m_pFnt->GetLogFont ( &lf );
			lf.lfHeight = iHt;
			if (fntTemp.m_hObject != NULL)
				{
				pDc->SelectObject ( pOld );
				fntTemp.DeleteObject ();
				}
			fntTemp.CreateFontIndirect (&lf);

			// see if this works
			pDc->SelectObject ( &fntTemp );
			pDc->DrawText (m_sText, -1, &rFit, DT_CALCRECT | dtStyle );
			if ( (rFit.right <= rText.right) && (rFit.bottom <= rText.bottom) )
				break;
			}
		}

	COLORREF * pClr = &(m_pStatData->m_clr [m_iImp]);

	// not shaded letters
	if (*pClr == *(pClr+1))
		{
		pDc->SetTextColor (*pClr);
		pDc->DrawText (m_sText, -1, rText, dtStyle);
		pDc->SelectObject ( pOld );
		return;
		}

	// shaded letters
	pDc->SetTextColor (*(pClr+1));
	rText.OffsetRect (1, 1);
	pDc->DrawText (m_sText, -1, rText, dtStyle);

	pDc->SetTextColor (*pClr);
	rText.OffsetRect (-1, -1);
	pDc->DrawText (m_sText, -1, rText, dtStyle);

	pDc->SelectObject ( pOld );
}

void CStatInst::DrawStatBar () const
{

	// show the % done
	ASSERT ((0 <= m_iPerDone) && (m_iPerDone <= 100));
	CRect rDest (m_rDest);
	rDest.OffsetRect ( - m_rDest.left, - m_rDest.top );

	rDest.top += (m_rDest.Height () - m_pStatData->m_cyIcon) / 2;
	rDest.bottom = rDest.top + m_pStatData->m_cyIcon;
	rDest.left += m_pStatData->m_leftOff;
	rDest.right -= m_pStatData->m_rightOff;

	rDest.right = rDest.left + ( rDest.Width () * m_iPerDone ) / 100;
	if ( rDest.Width () <= 0 )
		return;

	// what of the bar do we use?
	CRect rBar ( 0, 0, ( m_pStatData->m_cxIcon * m_iPerDone ) / 100, m_pStatData->m_cyIcon );

	m_pStatData->m_pcDib->StretchTranBlt ( m_pDib, rDest, rBar );
}

void CStatInst::DrawIcon (CDC * pDc) const
{

	// blt background to internal bitmap
	DrawBackground ();

	// blt the status info
	switch (m_pStatData->m_iTypIcon)
	  {
		case CStatData::done :
			DrawStatDone ();
			break;
		case CStatData::have_all :
			DrawStatHave ();
			break;
		case CStatData::text :
		case CStatData::text_right :
			DrawStatText ();
			break;
		case CStatData::bar :
			DrawStatBar ();
			break;

#ifdef _DEBUG
		default:
			ASSERT (FALSE);
			break;
#endif
	  }

	// blt the internal bitmap to the screen
	m_pDib->BitBlt (pDc->m_hDC, &(m_rDest), CPoint (0, 0));

	if ( m_pDib->IsBitmapSelected() )
		m_pDib->ReleaseDC();
}

void CStatInst::SetPer (int iPer)
{

	ASSERT ((0 <= iPer) && (iPer <= 100));
	ASSERT ((m_pStatData->m_iTypIcon == CStatData::done) || (m_pStatData->m_iTypIcon == CStatData::bar));
	if (iPer == m_iPerDone)
		return;

	m_iPerDone = iPer;
}

void CStatInst::SetHaveNeed (int iHave, int iNeed)
{

	ASSERT (m_pStatData->m_iTypIcon == CStatData::have_all);
	if ((iHave == m_iPerDone) && (iNeed == m_iNeed))
		return;

	m_iPerDone = iHave;
	m_iNeed = iNeed;
}

void CStatInst::SetText (char const * pText, IMPORTANCE iImp)
{

	if (pText == NULL)
		pText = "";

	m_iImp = iImp;
	m_sText = pText;
}

void CStatInst::IncIcon ()
{

	m_iIconOn++;
	if (m_iIconOn >= m_pStatData->m_nNeedIcon)
		m_iIconOn = 0;
}

void CStatInst::SetSize (CRect const & rect)
{

	if ((m_rDest == rect) && (m_pDib != NULL))
		return;

	m_rDest = rect;

	// make a DIB the size of the window for faster paints
	if (m_pDib != NULL)
		m_pDib->Resize ( rect.Width (), rect.Height () );
	else
		m_pDib = new CDIB ( ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
					ptrthebltformat->GetMemDirection(), rect.Width (), rect.Height ());
}



/////////////////////////////////////////////////////////////////////////////
// CWndStatBar

CWndStatBar::CWndStatBar()
{

}

CWndStatBar::~CWndStatBar()
{

}


void CWndStatBar::SetPer (int iPer)
{

	ASSERT ((0 <= iPer) && (iPer <= 100));
	ASSERT (m_statInst.m_pStatData->m_iTypIcon == CStatData::done);
	if (iPer == m_statInst.m_iPerDone)
		return;

	m_statInst.SetPer (iPer);
	InvalidateRect (NULL);
}

void CWndStatBar::SetHaveNeed (int iHave, int iNeed)
{

	ASSERT (m_statInst.m_pStatData->m_iTypIcon == CStatData::have_all);
	if ((iHave == m_statInst.m_iPerDone) && (iNeed == m_statInst.m_iNeed))
		return;

	m_statInst.SetHaveNeed (iHave, iNeed);
	InvalidateRect (NULL);
}

void CWndStatBar::SetText (char const * pText, CStatInst::IMPORTANCE iImp)
{

	if (pText == NULL)
		pText = "";

	ASSERT ( (m_statInst.m_pStatData->m_iTypIcon == CStatData::text) || (m_statInst.m_pStatData->m_iTypIcon == CStatData::text_right));
	if ((iImp == m_statInst.m_iImp) && (! strcmp (m_statInst.m_sText, pText)))
		return;

	m_statInst.SetText (pText, iImp);
	InvalidateRect (NULL);
}


BEGIN_MESSAGE_MAP(CWndStatBar, CWndBase)
	//{{AFX_MSG_MAP(CWndStatBar)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndStatBar message handlers

BOOL CWndStatBar::OnEraseBkgnd (CDC *) 
{

	// don't erase
	return (TRUE);
}

void CWndStatBar::OnPaint() 
{

	CPaintDC dc(this); // device context for painting
	thePal.Paint (dc.m_hDC);
	
	m_statInst.DrawIcon (&dc);

	thePal.EndPaint (dc.m_hDC);
	// Do not call CWndBase::OnPaint() for painting messages
}

void CWndStatBar::Create (CIcons * pIcons, int iIndex, CRect & rect, CWnd * pPar, CDIB * pBack)
{

	m_statInst.Attach (pIcons, iIndex);
	m_statInst.m_pBackDib = pBack;
	m_statInst.SetSize (CRect (0, 0, rect.Width (), rect.Height ()));

	CWndBase::Create (NULL, NULL, WS_CHILD | WS_VISIBLE, rect, pPar, 100);
}

void CWndStatBar::OnSize(UINT nType, int cx, int cy) 
{

	CWndBase::OnSize(nType, cx, cy);
	
	m_statInst.SetSize (CRect (0, 0, cx, cy));
}

void CWndStatBar::OnMouseMove(UINT nFlags, CPoint point) 
{
	
	// tell the parent
	GetParent()->SendMessage ( WM_ICONMOUSEMOVE, 0, (LPARAM) this );

	CWnd::OnMouseMove(nFlags, point);
}

int CWndStatBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{

	if (CWndBase::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// init bitmaps
	CRect rect;
	GetClientRect ( &rect );
	m_statInst.SetSize (CRect (0, 0, rect.Width (), rect.Height ()));
	
	return 0;
}

void CWndStatBar::CheckSize ()
{

	CRect rect;
	GetClientRect (&rect);
	m_statInst.SetSize ( rect );
}
