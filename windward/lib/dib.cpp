//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "_windwrd.h"

#pragma intrinsic( memset )
#pragma intrinsic( memcpy )

#include "dib.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


//------------------------------- C D I B i t s ---------------------------
//
// Lock/unlock wrapper for CDIB bits. This is the only way to access
// them.
//
// Example: CDIBits	dibits = pdib->GetBits();	// Locks the bits
//				memset( dibits, 0, 20 );				// conversion to BYTE *
//																// Unlocks when leaves scope


//------------------------------- C D I B H D C ---------------------------
//
// Get/Release wrapper for CDIB hDC. 
//
// The destructor releases the hDC when the CDIB type is a DirectDraw
// surface (whereupon DirectDraw unlocks the bits)
//


//---------------------------------- C D I B ------------------------------
//
// A DIB using memory, WinG, CreateDibSection, or DirectDraw
// This class is WIP

//-------------------------------------------------------------------------
// CDIB::CDIB
//-------------------------------------------------------------------------
CDIB::CDIB(
	CColorFormat const  		  &colorformat,	// Bits/bytes-per-pixel
	CBLTFormat::DIB_TYPE	 		eType,			// Surface type
	CBLTFormat::DIB_DIRECTION 	eDirection,		// Top-down or bottom-up
	int								cx,				// Dimensions
	int								cy )
	:
		m_colorformat	  ( colorformat ),
		m_eType		 	  ( eType       ),
		m_iDir		 	  ( eDirection  ),
		m_pBits		 	  ( NULL ),
		m_pddsurfaceBack ( NULL ),
		m_hDCDib     	  ( NULL ),
		m_hOrigBm	 	  ( NULL ),
		m_hTextBm	 	  ( NULL ),
		m_cx			 	  ( 0 ),
		m_cy			 	  ( 0 ),
		m_iLock			  ( 0 ),
		m_bBitmapSelected( FALSE )
{
	switch ( GetType() )
	{
		case CBLTFormat::DIB_WING:

			if ( !CWinG::GetTheWinG() || NULL == ( m_hDCDib = CWinG::GetTheWinG()->CreateDC() ))
				m_eType = CBLTFormat::DIB_MEMORY;
			else
				m_ptrwing = ptrtheWinG;

			break;

		case CBLTFormat::DIB_DIRECTDRAW:

			if ( !CDirectDraw::GetTheDirectDraw() )
				m_eType = CBLTFormat::DIB_MEMORY;
			else
				m_ptrdirectdraw = ptrtheDirectDraw;

			break;
	}

	switch ( GetType() )
	{
		case CBLTFormat::DIB_DIBSECTION:
		case CBLTFormat::DIB_MEMORY:

			m_hDCDib = CreateCompatibleDC( NULL );

			break;
	}

	// get the BITMAPINFO

	memset( &m_bmi, 0, sizeof( m_bmi ));

	m_bmi.hdr.biSize        = sizeof( m_bmi.hdr );
	m_bmi.hdr.biPlanes 		= 1;
	m_bmi.hdr.biBitCount 	= 8 * GetBytesPerPixel();
	m_bmi.hdr.biCompression = BI_RGB;

	// if it's a fixed size create the bitmap now

	if ( cx && cy )
		Resize( cx, cy );

	ASSERT_STRICT_VALID( this );
}

//-------------------------------------------------------------------------
// CDIB::~CDIB
//-------------------------------------------------------------------------
CDIB::~CDIB()
{

	if ( m_hOrigBm != NULL )
		{
		HBITMAP hPrevBm = ( HBITMAP )SelectObject( m_hDCDib, m_hOrigBm );

		if ( hPrevBm != m_hOrigBm )
			{
			if (hPrevBm == m_hTextBm)
				m_hTextBm = NULL;
			DeleteObject( hPrevBm );
			}
		}

	if ( m_hTextBm != NULL )
		DeleteObject( m_hTextBm );

	if (m_hDCDib != NULL)
		{
		thePal.EndPaint( m_hDCDib );
		DeleteDC( m_hDCDib );
		}

	if ( m_pddsurfaceBack )
		m_pddsurfaceBack->Release();

	if ( m_pBits && CBLTFormat::DIB_MEMORY == GetType() )
		delete [] m_pBits;
}

//-------------------------------------------------------------------------
// CDIB::Resize
//-------------------------------------------------------------------------
BOOL
CDIB::Resize(
	int	cx,
	int	cy )
{
	ASSERT( ! IsLocked() );

	#ifdef DEBUG
	
	if ( m_hOrigBm != NULL )		// first time in the bitmap isn't created yet
		ASSERT_STRICT_VALID( this );

	#endif

	if ( cx == m_cx && cy == m_cy )
		return TRUE;

	if ( m_hTextBm != NULL )
		{
		DeleteObject( m_hTextBm );
		m_hTextBm = NULL;
		}

	if ( m_pBits && CBLTFormat::DIB_MEMORY == GetType() )
	{
		delete [] m_pBits;

		m_pBits = NULL;
	}

	// select orig bm & delete old size bm first so we don't have 2 giant Allocs at once

	if ( m_hOrigBm != NULL )
	{
		HBITMAP hPrevBm = ( HBITMAP )SelectObject( m_hDCDib, m_hOrigBm );

		m_hOrigBm = NULL;

		DeleteObject( hPrevBm );
	}

	m_cx = Max( 1, cx );
	m_cy = Max( 1, cy );

	m_bmi.hdr.biWidth  = m_cx;
	m_bmi.hdr.biHeight = m_cy * m_iDir;

	HBITMAP hbm = NULL;
	int	  i;
	WORD	 *pw;

	//-----------------------------------
	// Create identity translation table
	//-----------------------------------

	if ( !m_ptrbmiIdentity.Value() )
		m_ptrbmiIdentity = new BITMAPINFO256;

	memcpy( m_ptrbmiIdentity.Value(), &m_bmi, sizeof( BITMAPINFO256 ));

	pw = ( WORD * )m_ptrbmiIdentity->rgb;

	for ( i = 0; i < 256; ++i )
		*pw++ = ( WORD )i;

	//-------------------
	// Create the bitmap
	//-------------------

	switch ( GetType() )
	{
		case CBLTFormat::DIB_WING:

			ASSERT_STRICT_VALID( CWinG::GetTheWinG() );

			m_bmi.hdr.biWidth = ( m_bmi.hdr.biWidth + 3 ) & ~ 3;
			hbm = CWinG::GetTheWinG()->CreateBitmap( m_hDCDib,
													  			  ( BITMAPINFO * )&m_bmi,
													  			  ( void ** )&m_pBits );
			break;

		case CBLTFormat::DIB_DIBSECTION:
		{
			SelectPalette( m_hDCDib, thePal.hPal(), FALSE );

			hbm = CreateDIBSection( m_hDCDib,
											( BITMAPINFO * )m_ptrbmiIdentity.Value(),
											DIB_PAL_COLORS,
											// GetColorUse(),
											( void ** )&m_pBits,
											NULL,
											0 );
			break;
		}

		case CBLTFormat::DIB_DIRECTDRAW:

			//
			//	create an off-screen surface
			//

			if ( m_pddsurfaceBack )
			{
				m_hRes = GetDDSurface()->Release();

				m_pddsurfaceBack = NULL;

				if ( FAILED( m_hRes ))
				{
					TRACE( "Off-screen surface release failed." );

					return FALSE;
				}
			}

			memset( &m_ddOffSurfDesc, 0, sizeof( DDSURFACEDESC ));

			m_ddOffSurfDesc.dwSize			 = sizeof( DDSURFACEDESC );
			m_ddOffSurfDesc.dwFlags 	  	 = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
			m_ddOffSurfDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
			m_ddOffSurfDesc.dwWidth			 = GetWidth();
			m_ddOffSurfDesc.dwHeight		 = GetHeight();

			m_hRes = CDirectDraw::GetTheDirectDraw()->GetDD()->CreateSurface( &m_ddOffSurfDesc, &m_pddsurfaceBack, NULL );

			if ( FAILED( m_hRes ))
			{
				TRACE( "Off-screen surface create failed." );

				return FALSE;
			}

			//
			//	get a full description of the surface
			//

			m_hRes = m_pddsurfaceBack->GetSurfaceDesc( &m_ddOffSurfDesc );

			if ( FAILED( m_hRes ))
			{
				TRACE( "Off-screen Surface GetSurfaceDesc failed." );

				return FALSE;
			}

			break;

		case CBLTFormat::DIB_MEMORY:

			m_pBits = new BYTE [ GetHeight() * (( GetBytesPerPixel() * m_bmi.hdr.biWidth + 3 ) & ~3 )];

			break;

	}

	if ( CBLTFormat::DIB_MEMORY != GetType() && hbm == NULL && m_pddsurfaceBack == NULL )
		return FALSE;

	if ( 1 == GetBytesPerPixel() )
	{
		if ( 0 == m_bmi.hdr.biClrUsed )
			m_bmi.hdr.biClrUsed = 256;

		if ( 0 == m_bmi.hdr.biClrImportant )
			m_bmi.hdr.biClrImportant = 256;
	}

	if ( CBLTFormat::DIB_DIRECTDRAW == GetType() )
		m_lPitch = m_ddOffSurfDesc.lPitch;
	else
		m_lPitch = ( GetBytesPerPixel() * m_bmi.hdr.biWidth + 3 ) & ~3;

	m_lDirPitch = -m_iDir * m_lPitch;

	m_bmi.hdr.biSizeImage = GetHeight() * m_lPitch;

	if ( hbm )
		m_hOrigBm = ( HBITMAP )SelectObject( m_hDCDib, hbm );

	memcpy( &m_ptrbmiIdentity->hdr, &m_bmi.hdr, sizeof( BITMAPINFOHEADER ));

  	SyncPalette();

	return TRUE;
}

//-------------------------------------------------------------------------
// CDIB::Lock
//-------------------------------------------------------------------------
BOOL
CDIB::Lock()
{
	ASSERT_STRICT_VALID( this );

	m_iLock++;

	switch ( GetType() )
	{
		case CBLTFormat::DIB_DIRECTDRAW:

			ASSERT_STRICT( m_pddsurfaceBack );

			if ( m_pBits )
				return TRUE;

			m_hRes = GetDDSurface()->Lock( 0, &m_ddOffSurfDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, 0 );

			if ( SUCCEEDED( m_hRes ))
			{
				m_pBits = ( LPBYTE )m_ddOffSurfDesc.lpSurface;

				return TRUE;						
			}

			return FALSE;

		case CBLTFormat::DIB_WING:
		case CBLTFormat::DIB_DIBSECTION:

			GdiFlush();
			break;
	}

	return TRUE;
}

//-------------------------------------------------------------------------
// CDIB::Unlock
//-------------------------------------------------------------------------
BOOL CDIB::Unlock()
{
	ASSERT_STRICT_VALID( this );

	ASSERT( 0 < m_iLock );

	m_iLock--;

	if ( CBLTFormat::DIB_DIRECTDRAW == GetType() )
	{
		if ( m_pBits )
		{
			m_hRes = GetDDSurface()->Unlock( m_pBits );

			if ( SUCCEEDED( m_hRes ))
			{
				m_pBits = 0;

				return TRUE;
			}
		}

		return FALSE;
	}

	return TRUE;
}

//-------------------------------------------------------------------------
// CDIB::Clear
//-------------------------------------------------------------------------
void
CDIB::Clear(
	CRect const * prect,					// NULL means the entire window
	int			  iPaletteIndex )		// Global palette index of desired color
{
	ASSERT_STRICT_VALID( this );

	ASSERT( !IsLocked() );

	CRect	rect( GetRect() );

	if ( prect )
		rect &= *prect;

	if ( rect.IsRectEmpty() )
		return;

	CDIBits	bits = GetBits();

	int		iSrcW  	 = rect.Width() * GetBytesPerPixel();
	int		i;
	BYTE	  *pbyDstOrg = bits + GetPitch() * rect.top + GetBytesPerPixel() * rect.left;
	BYTE	  *pbyDst	 = pbyDstOrg;

	// memset for byte-oriented ops

	if ( 0 == iPaletteIndex || 1 == GetBytesPerPixel() )
	{
		if ( prect == NULL )
		{
			if ( CBLTFormat::DIB_DIRECTDRAW != GetType() ||
				  m_ddOffSurfDesc.lPitch == ( LONG )m_ddOffSurfDesc.dwWidth )

				memset( pbyDst, iPaletteIndex, m_bmi.hdr.biSizeImage );

			return;
		}

		for ( i = 0; i < rect.Height(); ++i, pbyDst += GetDirPitch() )
			memset( pbyDst, iPaletteIndex, iSrcW );

		return;
	}

	BYTE	*pbyColor = thePal.GetDeviceColor( iPaletteIndex );

	// Do the first row

	for ( i = 0; i < rect.Width(); ++i, pbyDst += GetBytesPerPixel() )
		memcpy( pbyDst, pbyColor, GetBytesPerPixel() );

	// Copy the others

	pbyDst = pbyDstOrg + GetDirPitch();

	for ( i = 1; i < rect.Height(); ++i, pbyDst += GetDirPitch() )
		memcpy( pbyDst, pbyDstOrg, iSrcW );
}

//-------------------------------------------------------------------------
// CDIB::Scroll
//-------------------------------------------------------------------------
CRect					// Dest rect that needs only blitting, not re-rendering
CDIB::Scroll(
	int	iDelX,	// # of columns to move the pixels, + for right, - for left
	int	iDelY )	// # of rows to move the pixels, + for down, - for up
{
	CRect	rect( 0, 0, 0, 0 );

	int	iW		 = GetWidth();
	int	iH		 = GetHeight();
	int	iRectW = iW - abs( iDelX );
	int	iRectH = iH - abs( iDelY );

	if ( iRectW <= 0 || iRectH <= 0 )
		return rect;

	int	iWBytes = iRectW * GetBytesPerPixel();
	int	iPitch  = GetDirPitch();

	if ( iDelY > 0 )
		iPitch *= -1;

	CDIBits	dibits = GetBits();

	int	iSrcX, iDstX, iSrcY, iDstY;

	if ( iDelX > 0 )
	{
		iSrcX = 0;
		iDstX = iDelX;
	}
	else
	{
		iSrcX = -iDelX;
		iDstX = 0;
	}

	if ( iDelY > 0 )
	{
		iSrcY = iRectH - 1;
		iDstY = iH - 1;
	}
	else
	{
		iSrcY = -iDelY;
		iDstY = 0;
	}

	BYTE	 * pbySrc = dibits + GetOffset( iSrcX, iSrcY );
	BYTE	 * pbyDst = dibits + GetOffset( iDstX, iDstY );

	for ( int i = 0; i < iRectH; ++i )
	{
		memmove( pbyDst, pbySrc, iWBytes );

		pbySrc += iPitch;
		pbyDst += iPitch;
	}

	int	iLeft, iRight, iTop, iBottom;

	if ( iDelX > 0 )
	{
		iLeft   = iDelX;
		iRight  = iW;
	}
	else
	{
		iLeft   = 0;
		iRight  = iRectW;
	}

	if ( iDelY > 0 )
	{
		iTop    = iDelY;
		iBottom = iH;
	}
	else
	{
		iTop    = 0;
		iBottom = iRectH;
	}

	rect.SetRect( iLeft, iTop, iRight, iBottom );

	return rect;
}

//-------------------------------------------------------------------------
// CDIB::SyncPalette - Call when the thePal changes
//-------------------------------------------------------------------------
void
CDIB::SyncPalette()
{
	// Copy the palette to the dib header

	thePal.NewWnd( m_bmi );

	switch ( GetType() )
	{
		case CBLTFormat::DIB_WING:

			SelectPalette( m_hDCDib, thePal.hPal(), FALSE );
			CWinG::GetTheWinG()->SetDIBColorTable( m_hDCDib, 0, ( int )m_bmi.hdr.biClrUsed, m_bmi.rgb );
			break;

		case CBLTFormat::DIB_DIBSECTION:
		case CBLTFormat::DIB_MEMORY:

			SetDIBColorTable( m_hDCDib, 0, ( int )m_bmi.hdr.biClrUsed, m_bmi.rgb );

			break;
	}

	if ( m_hDCDib != NULL )
		{
		thePal.Paint( m_hDCDib );
//BUGBUG		thePal.EndPaint( m_hDCDib );
		}
}

//-------------------------------------------------------------------------
// CDIB::NewPalette
//-------------------------------------------------------------------------
void
CDIB::NewPalette(
	RGBQUAD const * pRgb,
	int 				 iNumEntries )
{
	ASSERT_STRICT( 0 );	// Should go in CDIBWnd?

	/*
	ASSERT_STRICT_VALID (this);

	ASSERT_STRICT( 1 == GetBytesPerPixel() );

	thePal.SetColors ((RGBQUAD *) pRgb, 0, iNumEntries);
	CWinG::GetTheWinG()->SetDIBColorTable( m_hDCDib, 0, iNumEntries, pRgb );
	thePal.Paint (m_hDC);
	*/
}

//-------------------------------------------------------------------------
// CDIB::BitBlt
//-------------------------------------------------------------------------
int
CDIB::BitBlt(
	HDC    			hdcDst,
	CRect  const & rectDst,
	CPoint const & ptSrc )
{
	ASSERT_STRICT_VALID( this );

	CDIBits	bits = GetBits();

	CPoint	ptSrcAdjusted = ptSrc;

	switch ( GetType() )
	{
		case CBLTFormat::DIB_WING:

			return CWinG::GetTheWinG()->BitBlt( hdcDst,
												 			rectDst.left,
													 		rectDst.top,
													 		rectDst.Width(),
													 		rectDst.Height(),
													 		m_hDCDib,
													 		ptSrcAdjusted.x,
													 		ptSrcAdjusted.y );

		case CBLTFormat::DIB_DIBSECTION:

			return ::BitBlt( hdcDst,
								  rectDst.left,
								  rectDst.top,
								  rectDst.Width(),
								  rectDst.Height(),
								  m_hDCDib,
								  ptSrcAdjusted.x,
								  ptSrcAdjusted.y,
								  SRCCOPY );

		case CBLTFormat::DIB_DIRECTDRAW:

			TRAP();

			ASSERT_STRICT( 0 );	// FIXIT: Implement

			return 0;

		case CBLTFormat::DIB_MEMORY:

			if ( !IsTopDown() )
				ptSrcAdjusted.y += GetHeight() - ptSrcAdjusted.y - ptSrcAdjusted.y - rectDst.Height();

			::SetStretchBltMode( hdcDst, COLORONCOLOR );

			if ( IsBitmapSelected() )
				return ::BitBlt( hdcDst,
									  rectDst.left,
									  rectDst.top,
								  	  rectDst.Width(),
								  	  rectDst.Height(),
								  	  m_hDCDib,
								  	  ptSrcAdjusted.x,
								  	  ptSrcAdjusted.y,
								  	  SRCCOPY );

			return StretchDIBits( hdcDst,
										 rectDst.left,
										 rectDst.top,
										 rectDst.Width(),
										 rectDst.Height(),
										 ptSrcAdjusted.x,
										 ptSrcAdjusted.y,
										 rectDst.Width(),
										 rectDst.Height(),
										 ( void * )( BYTE * )bits,
										 ( BITMAPINFO * )m_ptrbmiIdentity.Value(),
										 DIB_PAL_COLORS,
										 SRCCOPY );

		default:

			ASSERT_STRICT( 0 );

			return 0;
	}
}

//-------------------------------------------------------------------------
// CDIB::StretchBlt
//-------------------------------------------------------------------------
int
CDIB::StretchBlt(
	HDC    			hdcDst,
	CRect const & rectDst,
	CRect const & rectSrc )
{
	ASSERT_STRICT_VALID (this);

	CDIBits	bits = GetBits();

	CRect	rectSrcAdjusted = rectSrc;

	::SetStretchBltMode( hdcDst, COLORONCOLOR );

	switch ( GetType() )
	{
		case CBLTFormat::DIB_WING:

			return CWinG::GetTheWinG()->StretchBlt( hdcDst,
												 		  		 rectDst.left,
													 		  	 rectDst.top,
													 		  	 rectDst.Width(),
													 		  	 rectDst.Height(),
													 		  	 m_hDCDib,
													 		  	 rectSrcAdjusted.left,
													 		  	 rectSrcAdjusted.top,
													 		  	 rectSrcAdjusted.Width(),
													 		  	 rectSrcAdjusted.Height() );

		case CBLTFormat::DIB_DIBSECTION:

			return ::StretchBlt( hdcDst,
									 	rectDst.left,
									 	rectDst.top,
									 	rectDst.Width(),
									 	rectDst.Height(),
						 		    	m_hDCDib,
									 	rectSrcAdjusted.left,
									 	rectSrcAdjusted.top,
									 	rectSrcAdjusted.Width(),
									 	rectSrcAdjusted.Height(),
									 	SRCCOPY );

		case CBLTFormat::DIB_DIRECTDRAW:

			TRAP();

			ASSERT_STRICT( 0 );	// FIXIT: Implement

			return 0;

		default:

//			if ( !IsTopDown() )
			// GGTESTING
			if ( rectDst.Height() == rectSrc.Height() )
				rectSrcAdjusted += CPoint( 0, GetHeight() - rectSrcAdjusted.top - rectSrcAdjusted.bottom );

			if ( IsBitmapSelected() )
				return ::StretchBlt( hdcDst,
										 	rectDst.left,
										 	rectDst.top,
										 	rectDst.Width(),
										 	rectDst.Height(),
							 		    	m_hDCDib,
										 	rectSrcAdjusted.left,
										 	rectSrcAdjusted.top,
										 	rectSrcAdjusted.Width(),
										 	rectSrcAdjusted.Height(),
									 		SRCCOPY );

			return StretchDIBits( hdcDst,
									 	 rectDst.left,
									 	 rectDst.top,
								 		 rectDst.Width(),
								 		 rectDst.Height(),
									 	 rectSrcAdjusted.left,
									 	 rectSrcAdjusted.top,
								 		 rectSrcAdjusted.Width(),
								 		 rectSrcAdjusted.Height(),
									 	 ( void * )( BYTE * )bits,
							 			 ( BITMAPINFO * )m_ptrbmiIdentity.Value(),
										 DIB_PAL_COLORS,
									 	 // GetColorUse(),
										 SRCCOPY );
	}
}

//-------------------------------------------------------------------------
// CDIB::SetBits - Copy bits from buffer to CDIB rect
//-------------------------------------------------------------------------
void
CDIB::SetBits(
	BYTE	const * pbyBits,
	CRect const * prectDst )
{
	ASSERT_STRICT_VALID( this );
	ASSERT_STRICT( !prectDst || AfxIsValidAddress( prectDst, sizeof( CRect )));

	ASSERT( !IsLocked() );

	CRect	rectDst   		= prectDst ? *prectDst : GetRect();
	CRect	rectDstClipped = rectDst & GetRect();

	ASSERT_STRICT( AfxIsValidAddress( pbyBits, m_lPitch * rectDst.Height() ));

	int	iDX = rectDstClipped.left - rectDst.left;
	int	iDY = rectDstClipped.top  - rectDst.top;

	CDIBits	bitsDst 	= GetBits();

	BYTE 		  * pbyDst 	= bitsDst + GetOffset( rectDstClipped.left, rectDstClipped.top );
	BYTE const * pbySrc 	= pbyBits + GetOffset( iDX, iDY );

	int	iWBytes 		= rectDstClipped.Width() * GetBytesPerPixel();
	int	iPitchSrc 	= rectDst.Width() * GetBytesPerPixel();
	int	iH      		= rectDstClipped.Height();

	for ( int i = 0; i < iH; ++i, pbyDst += GetDirPitch(), pbySrc += iPitchSrc )
		memcpy( pbyDst, pbySrc, iWBytes );
}

//--------------------------------------------------------------------------
// CDIB::SetBits
//--------------------------------------------------------------------------
void 
CDIB::SetBits( 
	BITMAPINFOHEADER * pBih,
	void const		  * pvBits )
{
	ASSERT_STRICT_VALID (this );

	ASSERT( GetWidth()  == pBih->biWidth  );
	ASSERT( GetHeight() == abs( pBih->biHeight ));
	ASSERT( 8 * GetBytesPerPixel() == pBih->biBitCount );
	ASSERT( !IsLocked() );

	CDIBits	  dibits = GetBits();
	BYTE 		* pbyDst = dibits + GetOffset( 0, 0 );

	int	 iWBytes = pBih->biWidth * GetBytesPerPixel();
	int 	 iAdd 	= (iWBytes + 3) & ~ 0x03;
	BYTE * pbySrc;
	int	 iH = abs( pBih->biHeight );
	BOOL	 bSrcIsTopDown = pBih->biHeight < 0;

	if ( bSrcIsTopDown )
		pbySrc = ( BYTE * )pvBits;
	else
	{
		pbySrc = ( BYTE * )pvBits + ( iH - 1 ) * iAdd;
		iAdd   = -iAdd;
	}

	for ( int i = 0; i < iH; ++i, pbyDst += GetDirPitch(), pbySrc += iAdd)
		memcpy( pbyDst, pbySrc, iWBytes );
}

//--------------------------------------------------------------------------
// CDIB::SetBits24 - Copy 24bpp bits to 15/16/32bpp DIB
//--------------------------------------------------------------------------
void 
CDIB::SetBits24( 
	BITMAPINFOHEADER * pBih,
	void const		  * pvBits )
{
	ASSERT( 24 == pBih->biBitCount );

	ASSERT( 15 == GetBitsPerPixel() ||
		     16 == GetBitsPerPixel() ||
			  32 == GetBitsPerPixel() );

	ASSERT( !IsLocked() );

	CDIBits	  dibits = GetBits();
	BYTE 		* pbyDst = dibits + GetOffset( 0, 0 );
	BYTE 		* pbySrc = NULL;

	int	iW					= pBih->biWidth;
	int	iH					= abs( pBih->biHeight );
	int 	iSrcWBytes   	= 3 * iW;
	int 	iDstWBytes	  	= GetBytesPerPixel() * GetWidth();
	int	iSrcPitch    	= ( iSrcWBytes + 3 ) & ~ 0x03;
	int	iDstPitch	  	= GetPitch();
	int	iSrcAddBytes;
	int	iDstAddBytes;

	ASSERT( GetWidth()  == iW );
	ASSERT( GetHeight() == iH );

	BOOL	 bSrcIsTopDown = pBih->biHeight < 0;
	BOOL	 bDstIsTopDown = IsTopDown();

	if ( bSrcIsTopDown )
		iSrcAddBytes = iSrcPitch - iSrcWBytes;
	else
		iSrcAddBytes = -iSrcPitch - iSrcWBytes;

	if ( bDstIsTopDown )
		iDstAddBytes = iDstPitch - iDstWBytes;
	else
		iDstAddBytes = -iDstPitch - iDstWBytes;

	if ( bSrcIsTopDown )
		pbySrc = ( BYTE * )pvBits;
	else
		pbySrc = ( BYTE * )pvBits + ( iH - 1 ) * iSrcPitch;

	WORD	* pwDst = ( WORD * )pbyDst;
	int	  iRow, iCol;

	switch ( GetBitsPerPixel() )
	{
		case 15:
		
			for ( iRow = 0; iRow < iH; ++iRow )
			{
				for ( iCol = 0; iCol < iW; ++iCol, pbySrc += 3 )
				
					*pwDst++ = ( WORD( pbySrc[2] >> 3 ) << 10 ) |
								  ( WORD( pbySrc[1] >> 3 ) <<  5 ) |
								  ( WORD( pbySrc[0] >> 3 ));

				pbySrc += iSrcAddBytes;
				pwDst  += iDstAddBytes >> 1;
			}

			break;

		case 16:

			for ( iRow = 0; iRow < iH; ++iRow )
			{
				for ( iCol = 0; iCol < iW; ++iCol, pbySrc += 3 )

					*pwDst++ = ( WORD( pbySrc[2] >> 3 ) << 11 ) |
								  ( WORD( pbySrc[1] >> 2 ) <<  5 ) |
								  ( WORD( pbySrc[0] >> 3 ));

				pbySrc += iSrcAddBytes;
				pwDst  += iDstAddBytes >> 1;
			}

			break;

		case 32:

			for ( iRow = 0; iRow < iH; ++iRow )
			{
				for ( iCol = 0; iCol < iW; ++iCol, pbySrc += 3 )
				{
					*pbyDst++ = pbySrc[0];
					*pbyDst++ = pbySrc[1];
					*pbyDst++ = pbySrc[2];
					*pbyDst++ = 0;
				}

				pbySrc += iSrcAddBytes;
				pbyDst += iDstAddBytes;
			}

			break;

		default: ASSERT( 0 );
	}
}

//-------------------------------------------------------------------------
// CDIB::BitBlt - Copy bits between 2 CDIBs with the same color format
//-------------------------------------------------------------------------
BOOL
CDIB::BitBlt(
	CDIB	        * pdibDst,
	CRect   const & rectDst,
	CPoint  const & ptSrc )
{
	ASSERT_STRICT_VALID( this    );
	ASSERT_STRICT_VALID( pdibDst );

	ASSERT_STRICT( GetBitsPerPixel() == pdibDst->GetBitsPerPixel() );

	ASSERT( !pdibDst->IsLocked() );

	// Color resolution must be the same

	if ( GetBitsPerPixel() != pdibDst->GetBitsPerPixel() )
		return FALSE;

	// Clip the dest rect and adjust source UL corner accordingly

	CRect		rectDstClipped = rectDst & pdibDst->GetRect();
	CPoint	ptSrcAdj       = ptSrc + rectDstClipped.TopLeft() - (( CRect & )rectDst ).TopLeft();
	ptSrcAdj.x = __max ( 0, ptSrcAdj.x );
	ptSrcAdj.y = __max ( 0, ptSrcAdj.y );

	// Clip the source rect and adjust dest rect accordingly

	CRect		rectSrcClipped = CRect( ptSrcAdj, rectDstClipped.Size() ) & GetRect();

	rectDstClipped = CRect( rectDstClipped.TopLeft(), rectSrcClipped.Size() );

	if ( rectDstClipped.IsRectEmpty() )
		return TRUE;

	// BLT it

	CDIBits	bitsDst = pdibDst->GetBits();
	CDIBits	bitsSrc = GetBits();

	BYTE *pbyDst  = bitsDst + pdibDst->GetOffset( rectDstClipped.left, rectDstClipped.top );
	BYTE *pbySrc  = bitsSrc + GetOffset( ptSrcAdj.x, ptSrcAdj.y );

	int	iWBytes = rectDstClipped.Width() * pdibDst->GetBytesPerPixel();
	int	iH      = rectDstClipped.Height();

	int iSrcAdd = GetDirPitch() - iWBytes;
	int iDstAdd = pdibDst->GetDirPitch() - iWBytes;

	int iNumDword = iWBytes / 4;
	int iNumBytes = iWBytes & 3;

	try
		{
		switch ( iNumBytes )
			{
			case 0 :
				_asm
					{
					mov		esi, pbySrc
					mov		edi, pbyDst
					mov		edx, iH

DoNextLine0:
					mov		ecx, iNumDword
					rep		movsd

					add		esi, iSrcAdd
					add		edi, iDstAdd
					dec		edx
					jnz		DoNextLine0
					}
				return TRUE;

			case 1 :
				iSrcAdd++;		// extra byte to move
				iDstAdd++;

				_asm
					{
					mov		esi, pbySrc
					mov		edi, pbyDst
					mov		edx, iH

DoNextLine1:
					mov		ecx, iNumDword
					rep		movsd
					mov		al, [esi]
					mov		[edi], al

					add		esi, iSrcAdd
					add		edi, iDstAdd
					dec		edx
					jnz		DoNextLine1
					}
				return TRUE;

			case 2 :
				iSrcAdd += 2;		// extra word to move
				iDstAdd += 2;

				_asm
					{
					mov		esi, pbySrc
					mov		edi, pbyDst
					mov		edx, iH

DoNextLine2:
					mov		ecx, iNumDword
					rep		movsd
					mov		ax, [esi]
					mov		[edi], ax

					add		esi, iSrcAdd
					add		edi, iDstAdd
					dec		edx
					jnz		DoNextLine2
					}
				return TRUE;

			case 3 :
				_asm
					{
					mov		esi, pbySrc
					mov		edi, pbyDst
					mov		edx, iH

DoNextLine3:
					mov		ecx, iNumDword
					rep		movsd
								movsw
								movsb

					add		esi, iSrcAdd
					add		edi, iDstAdd
					dec		edx
					jnz		DoNextLine3
					}
				return TRUE;
			}
		}

	catch (...)
		{
		TRAP ();
		}

	return TRUE;
}

//---------------------------------------------------------------------------
// CDIB::StretchBlt - Dest rect is clipped
//---------------------------------------------------------------------------
BOOL
CDIB::StretchBlt(
	CDIB			 * pdibDst,
	CRect  const & rectDst,
	CRect  const & rectSrc )
{
	ASSERT_STRICT_VALID( this );
	ASSERT_STRICT( GetBitsPerPixel() == pdibDst->GetBitsPerPixel() );

	ASSERT( !pdibDst->IsLocked() );

	// Source and dest must have same color resolution

	if ( GetBitsPerPixel() != pdibDst->GetBitsPerPixel() )
		return FALSE;

	// Source DIB must contain source rect

	if (( rectSrc & GetRect() ) != rectSrc )
		return FALSE;

	// Cant compute scale

	if ( rectSrc.IsRectEmpty() )
		return rectDst.IsRectEmpty();

	CRect	rectDstClip = rectDst & pdibDst->GetRect();

	// return if nothing to do
	if ( rectDstClip.IsRectEmpty() )
		return TRUE;
		
	// Calc scale using pre-clipped rects

	int	fixDU   = rectSrc.Width()  << 16;
	int	fixDV   = rectSrc.Height() << 16;
	int	fixDstW = rectDst.Width()  << 16;
	int	fixDstH = rectDst.Height() << 16;

	FIXDIV( fixDU, fixDstW );
	FIXDIV( fixDV, fixDstH );

	// Clip destination rect

	CDIBits	bitsSrc = GetBits();
	CDIBits	bitsDst = pdibDst->GetBits();

	BYTE * pbyDstLine    = bitsDst + pdibDst->GetOffset( rectDstClip.left, rectDstClip.top );
	BYTE * pbyDstLineEnd = bitsDst + pdibDst->GetOffset( rectDstClip.left, rectDstClip.bottom );

	int	fixV = ( rectSrc.top << 16 );

	int	iBytesPerPixel = GetBytesPerPixel();

	try
		{
		while ( pbyDstLine != pbyDstLineEnd )
		{
			// we are GPF'ing so we check
			int iRow = fixV >> 16;
			int iNum = rectDstClip.Width();

			BYTE * pbySrcLine = bitsSrc + GetOffset( rectSrc.left, iRow );
			BYTE * pbyDst     = pbyDstLine;
			BYTE * pbyDstEnd;
			BYTE * pbySrc;

			int	 fixU = (1 << 8) - 1;

			switch ( iBytesPerPixel )
			{
				case 1:

#ifdef IN_ASM
					pbyDstEnd  = pbyDstLine + rectDstClip.Width();
	
					while ( pbyDst < pbyDstEnd )
					{
						pbySrc  = pbySrcLine + ( fixU >> 16 );

						ASSERT_STRICT( IsInRange( pbySrc, iBytesPerPixel ));
						ASSERT_STRICT( pdibDst->IsInRange( pbyDst, iBytesPerPixel ));

						*pbyDst = *pbySrc;
						fixU   += fixDU;

						pbyDst++;
					}
#endif

					TRAP ();
					_asm
						{
						mov		edi, [pbyDst]
						mov		ecx, [iNum]
						mov		ebx, [pbySrcLine]
						mov		edx, [fixDU]
_doline:
						mov		esi, edx
						shr		esi, 16
						mov		al, [ esi + ebx ]
						mov		[edi], al
						add		edx, [fixDU]
						inc		edi
						dec		ecx
						jnz		_doline
						}

					break;

				default:

					pbyDstEnd  = pbyDstLine + iBytesPerPixel * rectDstClip.Width();

					while ( pbyDst < pbyDstEnd )
					{
						pbySrc  = pbySrcLine + ( fixU >> 16 ) * iBytesPerPixel;

						ASSERT_STRICT( IsInRange( pbySrc, iBytesPerPixel ));
						ASSERT_STRICT( pdibDst->IsInRange( pbyDst, iBytesPerPixel ));

						memcpy( pbyDst, pbySrc, GetBytesPerPixel() );
		
						fixU   += fixDU;
						pbyDst += GetBytesPerPixel();
					}
					break;
			}

			fixV       += fixDV;
			pbyDstLine += pdibDst->GetDirPitch();
		}
		}
	catch (...)
		{
		TRAP ();
		}

	return TRUE;
}

//---------------------------------------------------------------------------
// CDIB::StretchTranBlt - Dest rect is clipped
//---------------------------------------------------------------------------
BOOL
CDIB::StretchTranBlt(
	CDIB			 * pdibDst,
	CRect  const & rectDst,
	CRect  const & rectSrc,
	int				iTransColor )	// Palette index
{
	ASSERT_STRICT_VALID( this );
	ASSERT_STRICT( GetBitsPerPixel() == pdibDst->GetBitsPerPixel() );
	ASSERT_STRICT( 0 <= iTransColor && iTransColor < 256 );

	ASSERT( !pdibDst->IsLocked() );

	// Source and dest must have same color resolution

	if ( GetBitsPerPixel() != pdibDst->GetBitsPerPixel() )
		return FALSE;

	// Source DIB must contain source rect

	if (( rectSrc & GetRect() ) != rectSrc )
		return FALSE;

	// Can't compute scale

	if ( rectSrc.IsRectEmpty() )
		return rectDst.IsRectEmpty();

	// Clip destination rect

	CRect	rectDstClip = rectDst & pdibDst->GetRect();

	// return if nothing to do
	if ( rectDstClip.IsRectEmpty() )
		return TRUE;
		
	// Calc scale using pre-clipped rects

	int	fixDU   = rectSrc.Width()  << 16;
	int	fixDV   = rectSrc.Height() << 16;
	int	fixDstW = rectDst.Width()  << 16;
	int	fixDstH = rectDst.Height() << 16;

	FIXDIV( fixDU, fixDstW );
	FIXDIV( fixDV, fixDstH );

	CDIBits	bitsSrc = GetBits();
	CDIBits	bitsDst = pdibDst->GetBits();

	BYTE * pbyDstLine    = bitsDst + pdibDst->GetOffset( rectDstClip.left, rectDstClip.top );
	BYTE * pbyDstLineEnd = bitsDst + pdibDst->GetOffset( rectDstClip.left, rectDstClip.bottom );
	BYTE * pbyTransColor;
	DWORD	 dwTransColor;
		
	if ( ptrthebltformat->GetColorFormat() == GetColorFormat() )
		{
		pbyTransColor = thePal.GetDeviceColor( iTransColor );
		dwTransColor = * ((DWORD *) pbyTransColor);
		}
	else
	{
		dwTransColor  = thePal.GetDeviceColor( iTransColor, GetBitsPerPixel() );
		pbyTransColor = ( BYTE * )&dwTransColor;
	}

	int	fixV = ( rectSrc.top << 16 );

	int	iBytesPerPixel = GetBytesPerPixel();

	try
		{
		while ( pbyDstLine != pbyDstLineEnd )
		{
			// we are GPF'ing so we check
			int iRow = fixV >> 16;

			BYTE * pbySrcLine = bitsSrc + GetOffset( rectSrc.left, iRow );
			BYTE * pbyDst     = pbyDstLine;
			BYTE * pbyDstEnd;
			BYTE * pbySrc;
		
			int	fixU = fixDU >> 1;
			int iNum = rectDstClip.Width();

			switch ( iBytesPerPixel )
			{
				case 1:

					TRAP ();

					while ( iNum -- )
					{
						pbySrc  = pbySrcLine + ( fixU >> 16 );

						ASSERT_STRICT( IsInRange( pbySrc, 1 ));
						ASSERT_STRICT( pdibDst->IsInRange( pbyDst, 1 ));

						if ( (BYTE) dwTransColor != *pbySrc )
							*pbyDst = *pbySrc;

						fixU   += fixDU;

						pbyDst++;
					}

					break;

				default:
			
					pbyDstEnd  = pbyDstLine + iBytesPerPixel * rectDstClip.Width();

					while ( pbyDst < pbyDstEnd )
					{
						pbySrc  = pbySrcLine + ( fixU >> 16 ) * iBytesPerPixel;

						ASSERT_STRICT( IsInRange( pbySrc, iBytesPerPixel ));
						ASSERT_STRICT( pdibDst->IsInRange( pbyDst, iBytesPerPixel ));

						if ( memcmp( pbyTransColor, pbySrc, iBytesPerPixel ))
							memcpy( pbyDst, pbySrc, iBytesPerPixel );
		
						fixU   += fixDU;
						pbyDst += GetBytesPerPixel();
					}
			}

			fixV += fixDV;
	
			pbyDstLine += pdibDst->GetDirPitch();
		}
		}
	catch (...)
		{
		TRAP ();
		}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// TileDib - tile a bitmap
void CDIB::Tile (CDC & dc, CRect const & rect, CPoint const & ptSrc)
{

	ASSERT_STRICT_VALID( this );

	CPoint pt;
	pt.y = ptSrc.y % GetHeight ();
	if (pt.y < 0)
		pt.y += GetHeight ();
	int xStrt = ptSrc.x % GetWidth ();
	if (pt.x < 0)
		pt.x += GetWidth ();

	for (int y=rect.top; y<rect.bottom; )
		{
		CRect rBack (rect.left, y, rect.left + GetWidth (), y + GetHeight ());
		rBack.bottom = __min (rBack.bottom, rect.bottom);
		pt.x = xStrt;

		for (int x=rect.left; x<rect.right; )
			{
			rBack.right = __min (rBack.right, rect.right);
			BitBlt (dc.m_hDC, rBack, pt);
			rBack.OffsetRect (GetWidth () - pt.x, 0);
			x += GetWidth () - pt.x;
			pt.x = 0;
			}

		y += GetHeight () - pt.y;
		pt.y = 0;
		}
}

void CDIB::Tile (CDIB * pDest, CRect const & rect, CPoint const & ptSrc)
{

	ASSERT_STRICT_VALID( this );

	ASSERT( !pDest->IsLocked() );

	CPoint pt;
	pt.y = ptSrc.y % GetHeight ();
	if (pt.y < 0)
		pt.y += GetHeight ();
	int xStrt = ptSrc.x % GetWidth ();
	if (xStrt < 0)
		xStrt += GetWidth ();

	for (int y=rect.top; y<rect.bottom; )
		{
		CRect rBack (rect.left, y, rect.left + GetWidth (), y + GetHeight ());
		rBack.bottom = __min (rBack.bottom, rect.bottom);
		pt.x = xStrt;

		for (int x=rect.left; x<rect.right; )
			{
			rBack.right = __min (rBack.right, rect.right);
			BitBlt (pDest, rBack, pt);
			rBack.OffsetRect (GetWidth () - pt.x, 0);
			x += GetWidth () - pt.x;
			pt.x = 0;
			}

		y += GetHeight () - pt.y;
		pt.y = 0;
		}
}

//---------------------------------------------------------------------------------
// CDIB::TranBlt - BitBlt with transparent background
//-------------------------------------------------------------------------
BOOL
CDIB::TranBlt(
	CDIB	        * pdibDst,
	CRect   const & rectDst,
	CPoint  const & ptSrc,
	int				 iTransColor )
{
	ASSERT_STRICT_VALID( this    );
	ASSERT_STRICT_VALID( pdibDst );

	ASSERT_STRICT( GetBitsPerPixel() == pdibDst->GetBitsPerPixel() );

	ASSERT( !pdibDst->IsLocked() );

	// Color resolution must be the same

	if ( GetBitsPerPixel() != pdibDst->GetBitsPerPixel() )
		return FALSE;

	// Clip the dest rect and adjust source UL corner accordingly

	CRect		rectDstClip	= rectDst & pdibDst->GetRect();
	CPoint	ptSrcAdj    = ptSrc + rectDstClip.TopLeft() - (( CRect & )rectDst ).TopLeft();

	// Clip the source rect and adjust dest rect accordingly

	CRect		rectSrcClip = CRect( ptSrcAdj, rectDstClip.Size() ) & GetRect();

	rectDstClip = CRect( rectDstClip.TopLeft(), rectSrcClip.Size() );

	if ( rectDstClip.IsRectEmpty() )
		return TRUE;

	// Blit

	CDIBits	bitsSrc = GetBits();
	CDIBits	bitsDst = pdibDst->GetBits();

	BYTE * pbyDstLine    = bitsDst + pdibDst->GetOffset( rectDstClip.left, rectDstClip.top );
	BYTE * pbySrcLine    = bitsSrc +          GetOffset( ptSrcAdj.x, ptSrcAdj.y );

	int	iBytesPerPixel = GetBytesPerPixel();
	int iNumLines = rectDstClip.Height ();
	int iNumPixels = rectDstClip.Width ();

	switch ( iBytesPerPixel )
	{
		case 1: {

			int iSrcLineAdd = GetDirPitch () - iNumPixels;
			int iDestLineAdd = pdibDst->GetDirPitch () - iNumPixels;

			_asm
				{
				mov		esi, pbySrcLine
				mov		edi, pbyDstLine
				mov		edx, iNumLines

DoNextLine:
				mov		ecx, iNumPixels

DoLine:
				mov		al, byte ptr [esi]
				inc		esi
				cmp		al, 0FDh
				je		IsTran
				mov		[edi], al
IsTran:
				inc		edi

				dec		ecx
				jnz		DoLine

				add		esi, iSrcLineAdd
				add		edi, iDestLineAdd
				dec		edx
				jnz		DoNextLine
				}
			break; }

		case 2: {
			int iSrcLineAdd = GetDirPitch () - iNumPixels * 2;
			int iDestLineAdd = pdibDst->GetDirPitch () - iNumPixels * 2;

			// in 16-bit mode the transparent color can have several values
			DWORD	 dwTransColor = thePal.GetDeviceColor( iTransColor, GetBitsPerPixel() );

			_asm
				{
				mov		esi, pbySrcLine
				mov		edi, pbyDstLine
				mov		edx, iNumLines
				mov		ebx, dwTransColor

DoNextLine2:
				mov		ecx, iNumPixels

DoLine2:
				mov		eax, [esi]
				add		esi, 2
				cmp		ax, bx
				je		IsTran2
				mov		[edi], ax
IsTran2:
				add		edi, 2

				dec		ecx
				jnz		DoLine2

				add		esi, iSrcLineAdd
				add		edi, iDestLineAdd
				dec		edx
				jnz		DoNextLine2
				}
			break; }

		case 3: {
			int iSrcLineAdd = GetDirPitch () - iNumPixels * 3;
			int iDestLineAdd = pdibDst->GetDirPitch () - iNumPixels * 3;

			_asm
				{
				mov		esi, pbySrcLine
				mov		edi, pbyDstLine
				mov		edx, iNumLines

DoNextLine3:
				mov		ecx, iNumPixels

DoLine3:
				mov		eax, [esi]
				add		esi, 3
				and		eax, 0FFFFFFh
				cmp		eax, 0FF00FFh
				je		IsTran3
				mov		ebx, [edi]
				and		ebx, 0FF000000h
				or		ebx, eax
				mov		[edi], eax
IsTran3:
				add		edi, 3

				dec		ecx
				jnz		DoLine3

				add		esi, iSrcLineAdd
				add		edi, iDestLineAdd
				dec		edx
				jnz		DoNextLine3
				}
			break; }

		case 4: {
			int iSrcLineAdd = GetDirPitch () - iNumPixels * 4;
			int iDestLineAdd = pdibDst->GetDirPitch () - iNumPixels * 4;

			_asm
				{
				mov		esi, pbySrcLine
				mov		edi, pbyDstLine
				mov		edx, iNumLines

DoNextLine4:
				mov		ecx, iNumPixels

DoLine4:
				mov		eax, [esi]
				add		esi, 4
				cmp		eax, 0FF00FFh
				je		IsTran4
				mov		[edi], eax
IsTran4:
				add		edi, 4

				dec		ecx
				jnz		DoLine4

				add		esi, iSrcLineAdd
				add		edi, iDestLineAdd
				dec		edx
				jnz		DoNextLine4
				}
			break; }

	}
			
	return TRUE;
}

//-------------------------------------------------------------------------
// CDIB::GetDC
//-------------------------------------------------------------------------
HDC
CDIB::GetDC()
{
	ASSERT( 0 == m_iLock );
	ASSERT( !m_bBitmapSelected );

	m_iLock++;

	switch ( GetType() )
	{
		case CBLTFormat::DIB_DIRECTDRAW:

			m_hRes = GetDDSurface()->GetDC( &m_hDCDib );
			thePal.Paint( m_hDCDib );	// GG 9/11/96 - Just to be consistent, not sure if we need it

			break;

		case CBLTFormat::DIB_MEMORY: {

			CDIBits	dibits = GetBits();

			// do we need a bitmap?
			if (m_hTextBm == NULL)
				{
				HWND hWnd = GetActiveWindow ();
				HDC hdc = ::GetDC (hWnd);
				thePal.Paint( hdc );

				m_hTextBm = ::CreateCompatibleBitmap( hdc, GetWidth(), GetHeight() );
				thePal.EndPaint( hdc );
				::ReleaseDC ( hWnd, hdc );
				}

			thePal.Paint( m_hDCDib );

			::SetDIBits ( m_hDCDib, m_hTextBm, 0, abs( m_bmi.hdr.biHeight ), 
													  ( void * )( BYTE * )dibits,
										 			  ( BITMAPINFO * )m_ptrbmiIdentity.Value(),
													  DIB_PAL_COLORS );
			m_hOrigBm = ( HBITMAP )::SelectObject( m_hDCDib, m_hTextBm );
			break; }
	}

	SyncPalette ();

	m_bBitmapSelected = TRUE;

	return m_hDCDib;
}

//-------------------------------------------------------------------------
// CDIB::ReleaseDC
//-------------------------------------------------------------------------
void
CDIB::ReleaseDC(
	BOOL bSaveChanges )
{
	ASSERT( 1 == m_iLock );
	ASSERT( m_bBitmapSelected );

	m_iLock--;

	switch ( GetType() )
	{
		case CBLTFormat::DIB_DIRECTDRAW:

			thePal.EndPaint( m_hDCDib );
			m_hRes = GetDDSurface()->ReleaseDC( m_hDCDib );
			break;

		case CBLTFormat::DIB_MEMORY: {

			// read the data back (same format we delivered it as
			CDIBits	dibits = GetBits();

			thePal.Paint (m_hDCDib);
			::SelectObject( m_hDCDib, m_hOrigBm );

			thePal.Paint (m_hDCDib);	// GGTESTING

			if ( bSaveChanges )
				::GetDIBits( m_hDCDib, m_hTextBm, 0, (WORD)abs( m_bmi.hdr.biHeight ), 
								 dibits, ( BITMAPINFO * )m_ptrbmiIdentity.Value(), DIB_PAL_COLORS );
			thePal.EndPaint( m_hDCDib );

			break; }

		default:
			thePal.EndPaint( m_hDCDib );
			break;
	}

	m_bBitmapSelected = FALSE;
}

//-------------------------------------------------------------------------
// CDIB::IsInRange
//-------------------------------------------------------------------------
BOOL
CDIB::IsInRange(
	BYTE const * pby,
	int			 iBytes ) const
{
	ASSERT_STRICT_VALID( this );

	ASSERT_STRICT( 0 < iBytes );

	CDIBits	dibits = (( CDIB * )this )->GetBits();

	int	iDelta = pby - dibits;

	int	iRow = iDelta / GetPitch();
	int	iCol = iDelta % GetPitch() / GetBytesPerPixel();

	BOOL	bOK = iRow >= 0 && iRow <  GetHeight() &&
			 		iCol >= 0 && iCol <= GetWidth() - ( iBytes / GetBytesPerPixel() );

	ASSERT_STRICT( bOK );

	return bOK;
}

//-------------------------------------------------------------------------
// CDIB::Load												BobP - 4/30/96
//-------------------------------------------------------------------------
void
CDIB::Load( CMmio& mmio )
{
	ASSERT( !IsLocked() );

	DWORD dwLen;
	BITMAPFILEHEADER bmfh;
	LPBITMAPINFO lp = 0;
	if ( ( dwLen = mmio.GetChunkInfo().cksize ) != 0 )
	{
		long l;			// I'm not sure that MakeRiff is creating correct riff files
		mmio >> l;		// it's sticking an extra length in there???
		if ( dwLen > sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFO ) + sizeof( long ) )
		{
			dwLen -= ( sizeof( BITMAPFILEHEADER ) + sizeof( long ) );

			lp = ( LPBITMAPINFO ) new BYTE[ dwLen ];

			mmio.Read( &bmfh, sizeof( BITMAPFILEHEADER ) );

			if ( bmfh.bfType == MAKEWORD( 'B', 'M' ) )
			{
				mmio.Read( lp, dwLen );
				Copy( lp );
			}

			delete [] (LPBYTE)lp;
		}
	}
}

//-------------------------------------------------------------------------
// CDIB::Copy												BobP - 4/30/96
//-------------------------------------------------------------------------
void
CDIB::Copy(
	LPBITMAPINFO lpBmi,
	void const * pvBits )
{
	ASSERT( !IsLocked() );

	ASSERT_STRICT_VALID( this );
	     
	// GG: Shortcut - also handles 8-bit DIB w/ non-palettized screen
	if ( lpBmi->bmiHeader.biBitCount == GetBitsPerPixel() && 
		  lpBmi->bmiHeader.biBitCount != 16 )
	{
		Resize ( lpBmi->bmiHeader.biWidth, abs( lpBmi->bmiHeader.biHeight ));
		SetBits( &lpBmi->bmiHeader, pvBits );

		return;
	}

	// GGTESTING: Handle 42 -> 15/16/32
	if ( lpBmi->bmiHeader.biBitCount == 24 && GetBitsPerPixel() > 8 )
	{
		Resize   ( lpBmi->bmiHeader.biWidth, abs( lpBmi->bmiHeader.biHeight ));
		SetBits24( &lpBmi->bmiHeader, pvBits );

		return;
	}

	CBitmap* bmp = 0;
	CPalette* pal = 0;			// palette if palette device

	//
	//	Start a new scope to release DC before call to Copy
	//

	{

		CClientDC dc( 0 );

		BOOL fPalDevice = dc.GetDeviceCaps( RASTERCAPS ) & RC_PALETTE;

		/* use default palette */
		if ( fPalDevice )
		{
			pal = CPalette::FromHandle( thePal.hPal() );

			/* select and realize our palette */
			pal = dc.SelectPalette( pal, FALSE );
			dc.RealizePalette();
		}

		HBITMAP hBmp = ::CreateDIBitmap( dc.GetSafeHdc(),
													( LPBITMAPINFOHEADER )lpBmi, 
										 			CBM_INIT,
													pvBits,
										 			lpBmi,
													DIB_RGB_COLORS );

		if ( hBmp )
		{
			bmp = CBitmap::FromHandle( hBmp );
		}
		else
		{
			// BOBTODO: throw something
			TRACE( "::CreateDIBitmap failed.\n" );
			TRAP();
			return;
		}

		if ( fPalDevice )
		{
			dc.SelectPalette( pal, TRUE );
			dc.RealizePalette();
		}

	}

	Copy( *bmp );

	ASSERT_STRICT_VALID( this );
	
}

//-------------------------------------------------------------------------
// CDIB::Copy												BobP - 4/30/96
//-------------------------------------------------------------------------
void 
CDIB::Copy(
	CBitmap const & bmp )
{
	ASSERT_STRICT_VALID( this );

	ASSERT( !IsLocked() );

	CPalette* pal = 0;			// palette if palette device

	BITMAP bm;					// bitmap structure
	BITMAPINFOHEADER bih;		// bitmapinfoheader

	LPBITMAPINFOHEADER lpbih;	// pointer to BITMAPINFOHEADER for packed DIB

	ASSERT_STRICT_VALID( &bmp );

	(( CBitmap & )bmp ).GetBitmap( &bm );

	/* initialize BITMAPINFOHEADER */

	memset( &bih, 0, sizeof( BITMAPINFOHEADER ) );

	bih.biSize			= sizeof( BITMAPINFOHEADER );
	bih.biWidth			= bm.bmWidth;
	bih.biHeight		= bm.bmHeight * GetDirection();
	bih.biPlanes		= 1;
//	bih.biBitCount		= bm.bmBitsPixel;
	bih.biBitCount		= 8 * GetBytesPerPixel();	// GG
	bih.biCompression	= BI_RGB;

	lpbih = (LPBITMAPINFOHEADER)new BYTE[ GetPackedDibSize( &bih ) ];

	if ( !lpbih )
		return;

	*lpbih = bih;

	//
	//	start new scope so DC is released
	//

	{

		/* get a DC */

		CClientDC dc( 0 );

		BOOL fPalDevice = dc.GetDeviceCaps( RASTERCAPS ) & RC_PALETTE;

		/* use default palette */
		if ( fPalDevice )
		{
			pal = CPalette::FromHandle( thePal.hPal() );

			/* select and realize our palette */
			pal = dc.SelectPalette( pal, FALSE );
			dc.RealizePalette();
		}

		/*  
		 *  get the bits into the temporary packed dib
		 */

		if ( ::GetDIBits( dc.GetSafeHdc(), (HBITMAP)bmp.GetSafeHandle(), 0, (WORD)abs( bih.biHeight ), 
			 GetImage( lpbih ), (LPBITMAPINFO)lpbih, DIB_RGB_COLORS ) == 0 )	// GG
		{
			// BOBTODO: throw here?
			DWORD dwError = ::GetLastError();
			TRACE( "::GetDIBits failed.\n" );
			TRAP();
			delete [] (LPBYTE)lpbih;
			lpbih = 0;
		}
		else
		{
			Resize( bm.bmWidth, bm.bmHeight );
			SetBits( lpbih );
		}

		if ( fPalDevice )
		{
			dc.SelectPalette( pal, TRUE );
			dc.RealizePalette();
		}
	}

	delete [] (LPBYTE)lpbih;

	ASSERT_STRICT_VALID( this );
	
}

//-------------------------------------------------------------------------
// CDIB::AssertValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CDIB::AssertValid () const
{
	CObject::AssertValid();

	// FIXIT - finish
}
#endif

