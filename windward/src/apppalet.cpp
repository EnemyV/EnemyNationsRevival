//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "_windwrd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#include "blt.h"
#include "dib.h"

// BUGBUG - do we need reserved (except for BLACK) since we use 254?

CAppPalette  thePal;

CAppPalette::CAppPalette ()
{
	m_iNumSys = 20;
	m_hPal = NULL;
	m_hNotActivePal = NULL;
	m_bHalf = FALSE;

	m_pdwColors = new DWORD [ 256 ];
}

CAppPalette::~CAppPalette ()
{
	delete [] m_pdwColors;
}


BOOL CAppPalette::Init (BOOL bHalf, BOOL bUseSys)
{

	HDC hDC = GetDC (NULL);
	if (hDC == NULL)
		return (FALSE);
	if ((m_bSysClrs = bUseSys) == TRUE)
		m_iNumSys = 2;
	else
		m_iNumSys = GetDeviceCaps (hDC,	NUMCOLORS);

	// First we put in a bogus 256 entry palette to fill up the table
	// we read in the system palette
  GetSystemPaletteEntries (hDC, 0, 256, &m_lPal.palPalEntry[0]);
	m_lPal.palVersion = 0x300;
	m_lPal.palNumEntries = 256;

	// set the flags on all of it to PC_NOCOLLAPSE
	// leave the color values alone we we don't change the screen
	PALETTEENTRY *pPeDest = &m_lPal.palPalEntry[0];
	for (int iOn=0; iOn<256; iOn++, pPeDest++)
		pPeDest->peFlags = PC_NOCOLLAPSE;

	// and select/realize it so it fills the palette
	HPALETTE hPalOrig = CreatePalette ((LOGPALETTE *) &m_lPal);
	HPALETTE hPalTmp = SelectPalette (hDC, hPalOrig, FALSE);
	RealizePalette (hDC);
	// back to the old one
	hPalTmp = SelectPalette (hDC, hPalTmp, FALSE);
	DeleteObject (hPalTmp);

	// if its the halftone palette we set it and get its values
	if (bHalf)
	  {

		ASSERT( 0 );	// BUGBUG: Unilt I figure out a dynamic link solution
		/*
	  m_bHalf = TRUE;
		if ((m_hPal = WinGCreateHalftonePalette ()) == NULL)
			{
			ReleaseDC (NULL, hDC);
			return (FALSE);
			}
		GetPaletteEntries (m_hPal, 0, 256, &m_lPal.palPalEntry[0]);
		*/
	  }

	// we are setting up our own palette - starting with whats presently out there
	else
		{
	  m_bHalf = FALSE;
		// now we are setting it up for our window palette
		// reset flags so we can use m_lPal
		pPeDest = &m_lPal.palPalEntry[0];
		for (iOn=0; iOn<m_iNumSys/2; iOn++, pPeDest++)
			pPeDest->peFlags = 0;
		for (; iOn<256-m_iNumSys/2; iOn++, pPeDest++)
			pPeDest->peFlags = PC_RESERVED;

		// create it
		if ((m_hPal = CreatePalette ((LOGPALETTE *) &m_lPal)) == NULL)
			{
			ReleaseDC (NULL, hDC);
			return (FALSE);
			}
		}

	if (m_bSysClrs)
	  {
		SetSystemPaletteUse (hDC, SYSPAL_STATIC);
		SysColors (TRUE, hDC);
	  }
	else
		{
		SetSystemPaletteUse (hDC, SYSPAL_NOSTATIC);
		SetSystemPaletteUse (hDC, SYSPAL_STATIC);
		}

	SelectPalette (hDC, m_hPal, FALSE);
	RealizePalette (hDC);
	SelectPalette (hDC, hPalOrig, FALSE);

	ReleaseDC (NULL, hDC);

	UpdateDeviceColors( 0, 256 );

	ASSERT_STRICT_VALID (this);
	return (TRUE);
}

//-------------------------------------------------------------------------
// CAppPalette::GetColorValue - Convert RGB color to device color or palette index
//
// Example: DWORD		 dw = thePal.GetColor( colorref, dib.GetBitsPerPixel() );
//				BYTE		*pbyClr = ( BYTE * )&dw;
//				CDIBits	 dibits = dib.GetBits();
//				BYTE		*pby = dibits + dib.GetOffset( x, y );
//				
//				memcpy( pby, pbyClr, dib.GetBytesPerPixel() );
//-------------------------------------------------------------------------
DWORD
CAppPalette::GetColorValue(
	COLORREF colorref,
	int		iBitsPerPixel ) const
{
	DWORD	dwColor = 0;

	switch ( iBitsPerPixel )
	{
		case 8:	// Get index of best match

			// We don't use GetNearestPaletteIndex() - it assumes a linear response function

			{
				int	iMinDiff = INT_MAX;
				int	iIndex   = 0;

				for ( int i = 0; i < 256; ++i )
				{
					COLORREF	colorrefPal = GetColor( i );

					int	iDelRed   = ( int )GetRValue( colorref ) - ( int )GetRValue( colorrefPal );
					int	iDelGreen = ( int )GetGValue( colorref ) - ( int )GetGValue( colorrefPal );
					int	iDelBlue  = ( int )GetBValue( colorref ) - ( int )GetBValue( colorrefPal );

					int	iDiff =   3 * iDelRed   * iDelRed   		 +
									      ( iDelGreen * iDelGreen << 2 ) +
									      ( iDelBlue  * iDelBlue  << 1 );

					if ( iDiff < iMinDiff )
					{
						iMinDiff = iDiff;
						iIndex   = i;

						if ( 0 == iDiff )
							break;
					}
				}

				dwColor = iIndex;
			}

			break;

		case 15:

			dwColor = ( WORD )( GetRValue( colorref ) >> 3 ) << 10 |
						 ( WORD )( GetGValue( colorref ) >> 3 ) <<  5 |
						 ( WORD )( GetBValue( colorref ) >> 3 );

			break;

		case 16:

			dwColor = ( WORD )( GetRValue( colorref ) >> 3 ) << 11 |
						 ( WORD )( GetGValue( colorref ) >> 2 ) <<  5 |
						 ( WORD )( GetBValue( colorref ) >> 3 );

			break;

		case 24:
		case 32:
			
			dwColor = ( DWORD )GetRValue( colorref ) << 16 |
						 ( DWORD )GetGValue( colorref ) <<  8 |
						 ( DWORD )GetBValue( colorref );

			break;

		default:
			
			ASSERT( 0 );
	}

	return dwColor;
}

//-------------------------------------------------------------------------
// CAppPalette::UpdateDeviceColors	-  Parallel array of device-dependent
//													color bit-patterns
//-------------------------------------------------------------------------
void	CAppPalette::UpdateDeviceColors(
	int	iFirst,
	int	iCount )
{
	if ( !ptrthebltformat.Value() )
		return;

	int	iBitsPerPixel = ptrthebltformat->GetBitsPerPixel();

	for ( int i = iFirst; i < iFirst + iCount; ++i )
		m_pdwColors[ i ] = GetDeviceColor( i, iBitsPerPixel );
}

//-------------------------------------------------------------------------
// CAppPalette::GetDeviceColor
//-------------------------------------------------------------------------
DWORD
CAppPalette::GetDeviceColor(
	int	iIndex,
	int	iBitsPerPixel ) const
{
	DWORD	dwClr = GetColor( iIndex );

	switch ( iBitsPerPixel )
	{
		case 8:	return iIndex;

		case 15:	return	(  dwClr >> 19 			     ) |	// blue
								(( dwClr & 0x000000f8 ) << 7 ) |	// red
								(( dwClr & 0x0000f800 ) >> 6 );	// green
						
		// GGFIXIT: 16-bpp format is device-dependent
		case 16:	return 	(  dwClr >> 19 			     ) |	// blue
								(( dwClr & 0x000000f8 ) << 8 ) |	// red
								(( dwClr & 0x0000fc00 ) >> 5 );	// green
			
		case 32:
		case 24:	return 	(  dwClr >> 16 				   ) | // blue
								(( dwClr & 0x000000ff ) << 16 ) | // red
								(( dwClr & 0x0000ff00 )       );  // green

		default:	ASSERT( 0 );
	}

	return 0;
}

void CAppPalette::Exit ()
{

	ASSERT_STRICT_VALID (this);

	HDC hdc = GetDC (NULL);
	if (hdc != NULL)
		{
		SysColors (FALSE, hdc);
		ReleaseDC (NULL, hdc);
		}

	if (m_hPal != NULL)
		{
		DeleteObject (m_hPal);
		m_hPal = NULL;
		}
	if (m_hNotActivePal != NULL)
		{
		DeleteObject (m_hNotActivePal);
		m_hNotActivePal = NULL;
		}
}

void CAppPalette::Activate (HWND hWnd, HDC hDC, BOOL bActive)
{
int iOn;
PALETTEENTRY *pPeDest;

	ASSERT_STRICT_VALID (this);

	SysColors (bActive, hDC);

	// if no palette then nothing to do
	if (m_hPal == NULL)
		return;

	// if not active we set our palette flags to 0 so we are displayable
	if (! bActive)
		{
		if (m_hNotActivePal == NULL)
			{
			iOn = m_iNumSys / 2;
			pPeDest = &m_lPal.palPalEntry [iOn];
			for (; iOn<256-m_iNumSys/2; iOn++, pPeDest++)
				pPeDest->peFlags = 0;
			if ((m_hNotActivePal = CreatePalette ((LOGPALETTE *) &m_lPal)) == NULL)
				return;
			}

		SetSystemPaletteUse (hDC, SYSPAL_STATIC);
		UnrealizeObject (m_hPal);
		HPALETTE hOldPal = SelectPalette (hDC, m_hNotActivePal, FALSE);
		RealizePalette (hDC);
		SelectPalette ( hDC, hOldPal, TRUE );

		// switch them
		hOldPal = m_hPal;
		m_hPal = m_hNotActivePal;
		m_hNotActivePal = hOldPal;
//BUGBUG - NT went into an infinite loop		SendMessage ((HWND) -1, WM_PALETTECHANGED, (WPARAM) hWnd, 0);
		return;
		}

	// back to a PC_RESERVED palette - reset to make sure its identity
	// clear palette so we will be an identity palette again
	SetSystemPaletteUse (hDC, SYSPAL_NOSTATIC);
	if (! m_bSysClrs)
		SetSystemPaletteUse (hDC, SYSPAL_STATIC);

	iOn = m_iNumSys / 2;
	pPeDest = &m_lPal.palPalEntry [iOn];
	for (; iOn<256-m_iNumSys/2; iOn++, pPeDest++)
		pPeDest->peFlags = PC_RESERVED;

	// switch them
	if (m_hNotActivePal != NULL)
		{
		HPALETTE hOldPal = m_hPal;
		m_hPal = m_hNotActivePal;
		m_hNotActivePal = hOldPal;
		}

	HPALETTE hOldPal = SelectPalette (hDC, m_hPal, FALSE);
	RealizePalette (hDC);
	SelectPalette ( hDC, hOldPal, TRUE );
}

void CAppPalette::Paint (HDC hDC)
{

	ASSERT_STRICT_VALID (this);

	if ( m_hPal )
	{
		SelectPalette (hDC, m_hPal, FALSE);
		RealizePalette (hDC);
	}
}

void CAppPalette::EndPaint (HDC hDC)
{

	ASSERT_STRICT_VALID (this);

	if ( m_hPal )
		SelectPalette ( hDC, (HPALETTE) GetStockObject (DEFAULT_PALETTE), TRUE );
}

long CAppPalette::PalMsg (HDC hDC, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) const
{

	ASSERT_STRICT_VALID (this);

	if (m_hPal == NULL)
		return (DefWindowProc (hWnd, uMsg, wParam, lParam));

	switch (uMsg)
		{
		case WM_PALETTECHANGED :
			if ((HWND)wParam == hWnd)
				break;
			// fall through to WM_QUERYNEWPALETTE
		case WM_QUERYNEWPALETTE : {
			HPALETTE hOldPal = SelectPalette (hDC, m_hPal, FALSE);
			int iNum = RealizePalette (hDC);
			if (iNum)
				InvalidateRect (hWnd, NULL, FALSE);
			SelectPalette (hDC, hOldPal, TRUE);
			return (iNum); }
		}

	return (DefWindowProc (hWnd, uMsg, wParam, lParam));
}

void CAppPalette::SetColors (RGBQUAD  *pRgb, int iFirst, int iNumClrs)
{

	ASSERT_STRICT_VALID (this);

  m_bHalf = FALSE;

	// begining system colors -> pRgb
	pRgb += iFirst;
	PALETTEENTRY *pPeDest = &m_lPal.palPalEntry [iFirst];
	int iNumLeft = iNumClrs;
	if (iFirst < m_iNumSys/2)
	  {
	  int iNum = m_iNumSys / 2 - iFirst;
		if (iNum > iNumLeft)
			iNum = iNumLeft;
		iNumLeft -= iNum;
		while (iNum--)
		  {
			pRgb->rgbRed = pPeDest->peRed;
			pRgb->rgbGreen = pPeDest->peGreen;
			pRgb->rgbBlue = pPeDest->peBlue;
			pRgb++;
			pPeDest++;
		  }
	  }

	int iNum = iNumLeft;
	if (iNum > 256 - m_iNumSys)
		iNum = 256 - m_iNumSys;
	iNumLeft -= iNum;
	while (iNum--)
	  {
		pPeDest->peRed = pRgb->rgbRed;
		pPeDest->peGreen = pRgb->rgbGreen;
		pPeDest->peBlue = pRgb->rgbBlue;
		pRgb++;
		pPeDest++;
	  }

	while (iNumLeft--)
	  {
		pRgb->rgbRed = pPeDest->peRed;
		pRgb->rgbGreen = pPeDest->peGreen;
		pRgb->rgbBlue = pPeDest->peBlue;
		pRgb++;
		pPeDest++;
	  }

	// if no palette then can't animate it
	if (m_hPal == NULL)
		return;

	// and animate to it
	SetPaletteEntries (m_hPal, m_iNumSys/2, 256-m_iNumSys, &m_lPal.palPalEntry [m_iNumSys/2]);
//	AnimatePalette (m_hPal, m_iNumSys/2, 256-m_iNumSys, &m_lPal.palPalEntry [m_iNumSys/2]);

	UpdateDeviceColors( iFirst, iNumClrs );
}

void CAppPalette::Animate()
{
	AnimatePalette( m_hPal, m_iNumSys/2, 256-m_iNumSys, &m_lPal.palPalEntry [m_iNumSys/2] );
}

void CAppPalette::GetColors (RGBQUAD  *pRgb, int iFirst, int iNumClrs) const
{
	ASSERT_STRICT_VALID (this);

	pRgb += iFirst;

	PALETTEENTRY const * pPeSrc = &m_lPal.palPalEntry [iFirst];

	for ( int i = 0; i < iNumClrs; ++i )
	{
		pRgb->rgbRed	= pPeSrc->peRed;
		pRgb->rgbGreen = pPeSrc->peGreen; 
		pRgb->rgbBlue  = pPeSrc->peBlue; 

		pRgb++;
		pPeSrc++;
	}
}

void CAppPalette::NewWnd (BITMAPINFO256 & bmi)
{

	PALETTEENTRY *pPeDest = &m_lPal.palPalEntry [0];
	int iNum = (int) bmi.hdr.biClrUsed;
	RGBQUAD * pRgb = & bmi.rgb [0];
	while (iNum--)
	  {
		pRgb->rgbRed = pPeDest->peRed;
		pRgb->rgbGreen = pPeDest->peGreen;
		pRgb->rgbBlue = pPeDest->peBlue;
		pRgb++;
		pPeDest++;
	  }
}

void CAppPalette::Fadeout ()
{

	// if no palette then nothing to do
	if (m_hPal == NULL)
		return;

}

void CAppPalette::Fadein ()
{

	// if no palette then nothing to do
	if (m_hPal == NULL)
		return;

}

static int SysPalIndex[] = {
	COLOR_ACTIVEBORDER,
	COLOR_ACTIVECAPTION,
	COLOR_APPWORKSPACE,
	COLOR_BACKGROUND,
	COLOR_BTNFACE,
	COLOR_BTNSHADOW,
	COLOR_BTNTEXT,
	COLOR_CAPTIONTEXT,
	COLOR_GRAYTEXT,
	COLOR_HIGHLIGHT,
	COLOR_HIGHLIGHTTEXT,
	COLOR_INACTIVEBORDER,

	COLOR_INACTIVECAPTION,
	COLOR_MENU,
	COLOR_MENUTEXT,
	COLOR_SCROLLBAR,
	COLOR_WINDOW,
	COLOR_WINDOWFRAME,
	COLOR_WINDOWTEXT
};
const int NumSysColors = sizeof (SysPalIndex) / sizeof (int);

//*** This array translates the display elements to black and white
#define rgbBlack RGB(0,0,0)
#define	rgbWhite RGB(255,255,255)
static COLORREF MonoColors[] = {
	rgbBlack,
	rgbWhite,
	rgbWhite,
	rgbWhite,
	rgbWhite,
	rgbBlack,
	rgbBlack,
	rgbBlack,
	rgbBlack,
	rgbBlack,
	rgbWhite,
	rgbWhite,
	rgbWhite,
	rgbWhite,
	rgbBlack,
	rgbWhite,
	rgbWhite,
	rgbBlack,
	rgbBlack,
	rgbBlack
};

//*** This array holds the old color mapping so we can restore them
static COLORREF OldColors[NumSysColors+1];

void CAppPalette::SysColors (BOOL bActive, HDC hdc)
{

	if (! m_bSysClrs)
		return;

	if ((bActive) && (GetSystemPaletteUse(hdc) == SYSPAL_STATIC))
		{
		for (int i=0; i<NumSysColors; i++)
			OldColors[i] = GetSysColor(SysPalIndex[i]);
		SetSystemPaletteUse (hdc, SYSPAL_NOSTATIC);
//		SetSysColors(NumSysColors, SysPalIndex, MonoColors);
		}

	else
		if (! bActive)
			{
			SetSystemPaletteUse (hdc, SYSPAL_STATIC);
//			SetSysColors(NumSysColors, SysPalIndex, OldColors);
			}

	SendMessage ((HWND) -1, WM_SYSCOLORCHANGE, 0, 0);
}

#ifdef _DEBUG
void CAppPalette::AssertValid () const
{

	// assert base object
	CObject::AssertValid ();

	ASSERT (m_iNumSys == 20);
}
#endif

//------------------------- C C o l o r B u f f e r ------------------------
//
// Buffer filed with a color from thePal in device-dependent format.
// Allows you to do a memcpy for drawing lines or filling in a single color.
// The capacity of the buffer is the max size it has been set to, during the
// object's lifetime, using SetColor() or GetBuffer().

//--------------------------------------------------------------------------
// CColorBuffer::CColorBuffer
//--------------------------------------------------------------------------
CColorBuffer::CColorBuffer(
	int	iIndexColor )		// Index of the color in thePal
	:
		m_pby       	 ( NULL ),
		m_nPixelCapacity( 0 ),
		m_nPixelCount   ( 0 )
{
	SetColor( iIndexColor, m_nPixelCount );
}

//--------------------------------------------------------------------------
// CColorBuffer::~CColorBuffer
//--------------------------------------------------------------------------
CColorBuffer::~CColorBuffer()
{
	delete [] m_pby;
}

//--------------------------------------------------------------------------
// CColorBuffer::SetColor
//--------------------------------------------------------------------------
void
CColorBuffer::SetColor(
	int	iIndexColor,	// Index of the color in thePal
	int	nPixels )		// # of pixels to set the color
{
	if ( nPixels > m_nPixelCapacity )	// Realloc and set all to the color
	{
		SetSize( nPixels + ( nPixels >> 1 ));

		m_iIndexColor = iIndexColor;
		m_nPixelCount = nPixels;

		FillColor( 0, nPixels - 1 );
	}
	else if ( iIndexColor != m_iIndexColor )	// Set requested # pixels to new color
	{
		m_iIndexColor = iIndexColor;
		m_nPixelCount = nPixels;

		FillColor( 0, nPixels - 1 );
	}
	else if ( nPixels > m_nPixelCount )	// Set additional pixels to current color
	{
		FillColor( m_nPixelCount, nPixels - 1 );

		m_nPixelCount = nPixels;
	}
}

//--------------------------------------------------------------------------
// CColorBuffer::FillColor
//--------------------------------------------------------------------------
void
CColorBuffer::FillColor(
	int	iFirst,
	int	iLast )
{
	// GGTODO: optimize

	int	iBytesPerPixel = ptrthebltformat->GetBytesPerPixel();
	int	nCount 		   = iLast - iFirst + 1;

	BYTE *pbyColor = thePal.GetDeviceColor( m_iIndexColor );
	BYTE *pby      = m_pby + iBytesPerPixel * iFirst;

	for ( int i = 0; i < nCount; ++i, pby += iBytesPerPixel )
		memcpy( pby, pbyColor, iBytesPerPixel );
}

//--------------------------------------------------------------------------
// CColorBuffer::SetSize
//--------------------------------------------------------------------------
void
CColorBuffer::SetSize(
	int nPixelCapacity )
{
	m_nPixelCapacity = nPixelCapacity;

	int	iBytesPerPixel = ptrthebltformat->GetBytesPerPixel();

	delete [] m_pby;

	m_pby = new BYTE [ m_nPixelCapacity * iBytesPerPixel ];
}

//--------------------------------------------------------------------------
// CColorBuffer::GetBuffer - Get pointer to buffer containing specified # pixels of current color.
//									  The buffer is grown if necessary.	
//--------------------------------------------------------------------------
BYTE const *
CColorBuffer::GetBuffer(
	int	nPixels )			// Max # pixels of the current color required
{
	if ( nPixels > m_nPixelCapacity )
	{
		SetSize  ( nPixels + ( nPixels >> 1 ));
		FillColor( 0, nPixels - 1 );

		m_nPixelCount = nPixels;
	}
	else if ( nPixels > m_nPixelCount )
	{
		FillColor( m_nPixelCount, nPixels - 1 );

		m_nPixelCount = nPixels;
	}

	return m_pby;
}

