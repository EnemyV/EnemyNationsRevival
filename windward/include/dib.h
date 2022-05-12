#ifndef __DIB_H__
#define __DIB_H__

//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "thielen.h"
#include "blt.h"

//------------------------- B I T M A P I N F O 2 5 6 -----------------------

struct BITMAPINFO256
{
	BITMAPINFOHEADER    hdr;
	RGBQUAD             rgb[256];
};

//-------------------------------- C D I B i t s ---------------------------
//
// Lock/Unlock/Access wrapper for CDIB bits

class CDIBits
{

public:

	operator BYTE * () { ASSERT_STRICT( m_pbyBits ); return m_pbyBits; }

  ~CDIBits();

protected:

	friend class CDIB;

	CDIBits( CDIB *pdib );

private:

	CDIB	* m_pdib;
	BYTE	* m_pbyBits;
};

//-------------------------------- C D I B H D C ---------------------------
//
// Get/Release wrapper for CDIB hDC

class CDIBHDC
{

public:

	operator HDC () { return m_hDC; }

  ~CDIBHDC();

protected:

	friend class CDIB;

	CDIBHDC( CDIB *pdib );

private:

	CDIB	* m_pdib;
	HDC	  m_hDC;
};

//--------------------------------- C D I B --------------------------------
//
// Device-independent bitmap

class CDIB
#ifdef _DEBUG
: public CObject
#endif
{

public:

	CDIB( CColorFormat const &,
			CBLTFormat::DIB_TYPE,
			CBLTFormat::DIB_DIRECTION,
			int	cx = 0,
			int	cy = 0 );

  ~CDIB();

	BOOL		IsLocked() const	{ return m_iLock > 0; }

	HDC	 	GetDC();
	void		ReleaseDC( BOOL bSaveChanges = FALSE );

	BOOL		IsBitmapSelected() const { return m_bBitmapSelected; }

	DWORD		GetColorValue( COLORREF colorref ) const
				{
					return thePal.GetColorValue( colorref, GetBitsPerPixel() );
				}

	void		Copy	 (	CDIB const * pdib );
	void		Copy	 ( LPBITMAPINFO lpBmi );
	void		Copy	 (	LPBITMAPINFO lpBmi, void const * pvBits );
	void 		Copy	 ( CBitmap const & bmp );

	BOOL		Resize ( int cx, int cy );
	void		Load	 ( CMmio & mmio );

	void 		SetBits  ( BITMAPINFOHEADER * pBih );
	void 		SetBits  ( BITMAPINFOHEADER * pBih, void const * pvBits );
	void		SetBits  ( BITMAPFILEHEADER * pBfhSrc );
	void 		SetBits24( BITMAPINFOHEADER * pBih, void const * pvBits );

	void 		Tile (CDC & dc, CRect const & rectDest, CPoint const & ptSrc = CPoint (0, 0));
	void		Tile (CDIB * pDibDest, CRect const & rectDest, CPoint const & ptSrc = CPoint (0,0 ));

	//--------------------------- D r a w i n g -----------------------------

	void		Clear( CRect const * = NULL, int iPaletteIndex = 0 );
	CRect		Scroll( int iDelX, int iDelY );

	//-------------------------- B l i t t i n g ----------------------------

	void		SetBits 			( BYTE const * pbyBits, CRect const * prect = NULL );
	BOOL		BitBlt 			( CDIB * pdibDst, CRect const & rectDst, CPoint const & ptSrc );
	BOOL		TranBlt 			( CDIB * pdibDst, CRect const & rectDst, CPoint const & ptSrc,   int iTransColor = 253 );
	BOOL		StretchTranBlt	( CDIB * pdibDst, CRect const & rectDst, CRect  const & rectSrc, int iTransClr   = 253 );
	BOOL		StretchBlt		( CDIB * pdibDst, CRect const & rectDst, CRect  const & rectSrc );
	void		CopyBits	 		( CDIB * );

	int		BitBlt 	 		( HDC hdcDst, CRect const & rectDst, CPoint const & ptSrc   );
	int		StretchBlt		( HDC hdcDst, CRect const & rectDst, CRect  const & rectSrc );

	//--------------------------- P a l e t t e -----------------------------

	void		SyncPalette();

	void		NewPalette( BITMAPINFOHEADER const *pBmih )	{ NewPalette(( const BITMAPINFO256 * )pBmih); }
	void		NewPalette( BITMAPINFO256 const *pBmih )		{ NewPalette( pBmih->rgb, ( int )pBmih->hdr.biClrUsed ); }
	void		NewPalette( RGBQUAD const * pRrgb, int iNumEntries );

	//------------------------- A c c e s s o r s ---------------------------

	CDIBits			 GetBits()			{ return CDIBits( this ); }
	BITMAPINFO256 * GetBitmapInfo()  { return &m_bmi; }

	// Warning: WIll be NULL for DIB_MEMORY (Fixit: use DIB.DRV?)

	CDIBHDC							GetDIBHDC()					   { return CDIBHDC( this ); }

	CRect								GetRect()				const { return CRect( 0, 0, m_cx, m_cy ); }
	int								GetHeight()				const { return m_cy; }
	int								GetWidth()				const { return m_cx; }
	CBLTFormat::DIB_TYPE			GetType()				const { return m_eType; }
	LONG								GetPitch()  			const	{ return m_lPitch; }
	LONG								GetDirPitch()  		const	{ return m_lDirPitch; }
	CBLTFormat::DIB_DIRECTION	GetDirection()  		const	{ return ( CBLTFormat::DIB_DIRECTION )m_iDir; }
	BOOL								IsTopDown() 			const	{ return -1 == m_iDir; }
	int								GetBitsPerPixel() 	const	{ return m_colorformat.GetBitsPerPixel();  }
	int								GetBytesPerPixel() 	const	{ return m_colorformat.GetBytesPerPixel(); }
	CColorFormat					GetColorFormat()	   const { return m_colorformat; }
	int								GetRow( int iRow )	const { return IsTopDown() ? iRow : GetHeight() - iRow - 1; }

	BOOL								IsInRange( BYTE const *, int iBytes ) const;	
	int								GetOffset( int x, int y ) { return GetRow( y ) * GetPitch() + x * GetBytesPerPixel(); }

	LPDIRECTDRAWSURFACE			GetDDSurface();

//--------------------------------------------------------------------------
//	
//	CDIB helpers
//
//

inline DWORD GetNumColors( LPBITMAPINFOHEADER lp )
{
	ASSERT( lp );
	DWORD biClrUsed = lp->biClrUsed;
	return biClrUsed ? biClrUsed : 1UL << lp->biBitCount;
}

inline DWORD GetColorTableSize( LPBITMAPINFOHEADER lp )
{
	ASSERT( lp );
	return lp->biBitCount < ((WORD)24) ? GetNumColors( lp ) : 0;
}

inline DWORD GetColorTableSizeBytes( LPBITMAPINFOHEADER lp )
{
	ASSERT( lp );
	return GetColorTableSize( lp ) * sizeof( RGBQUAD );
}

inline DWORD GetPitch( LPBITMAPINFOHEADER lp )
{
	ASSERT( lp );
	// BOBTODO: not sure about this
	return ( ( ( lp->biWidth * lp->biBitCount ) / 8 ) + 3 ) & ~3;
}

inline DWORD GetImageBytes( LPBITMAPINFOHEADER lp )
{
	ASSERT( lp );
	DWORD size = lp->biSizeImage;
	if ( size )
		return size;
	else
		return GetPitch( lp ) * abs( lp->biHeight );
}

inline DWORD GetImageOffset( LPBITMAPINFOHEADER lp )
{
	ASSERT( lp );
	return lp->biSize + GetColorTableSizeBytes( lp );
}

inline DWORD GetPackedDibSize( LPBITMAPINFOHEADER lp )
{
	ASSERT( lp );
	return GetImageOffset( lp ) + GetImageBytes( lp );
}

inline LPBYTE GetImage( LPBITMAPINFOHEADER lp )
{
	ASSERT( lp );
	return (LPBYTE)lp + GetImageOffset( lp );
}

#ifdef _DEBUG
	virtual void AssertValid() const;
#endif

protected:

	friend CDIBits;
	friend CDIBHDC;

	BYTE   * GetDIBits() { return m_pBits; }

	BOOL		Lock();
	BOOL		Unlock();

private:

	CColorFormat			  m_colorformat;
	BYTE  					* m_pBits;
	int						  m_iDir;					// DIB direction
	int						  m_cx;
	int						  m_cy;
	LONG						  m_lPitch;					// Span in bytes between start of consecutive rows
	LONG						  m_lDirPitch;				// Pitch * direction
	CBLTFormat::DIB_TYPE	  m_eType;
	HDC						  m_hDCDib;	
	HBITMAP					  m_hOrigBm;				// the original bitmap attached to hDCWinG
	HBITMAP					  m_hTextBm;				// the original bitmap attached to hDCWinG
	BITMAPINFO256			  m_bmi;						// WinG DIB header
	DDSURFACEDESC			  m_ddOffSurfDesc;		// off-screen surface desc
	LPDIRECTDRAWSURFACE	  m_pddsurfaceBack;		// off-screen surface (the bitmap)
	HRESULT					  m_hRes;
	Ptr< BITMAPINFO256 >	  m_ptrbmiIdentity;
	int						  m_iLock;
	BOOL						  m_bBitmapSelected;

	Ptr< CWinG >			  m_ptrwing;
	Ptr< CDirectDraw >	  m_ptrdirectdraw;
};


/////////////////////////////////////////////////////////////
// inline functions

//-------------------------------------------------------------------------
// CDIBits::CDIBits
//-------------------------------------------------------------------------
inline CDIBits::CDIBits( 
	CDIB *pdib )
{
	ASSERT_STRICT_VALID( pdib );

	if ( !pdib->Lock() )
		ASSERT_STRICT( 0 );	// FIXIT: exception

	m_pdib    = pdib;
	m_pbyBits = pdib->GetDIBits();
}

//-------------------------------------------------------------------------
// CDIBits::~CDIBits
//-------------------------------------------------------------------------
inline CDIBits::~CDIBits()
{
	m_pdib->Unlock();
}

//-------------------------------------------------------------------------
// CDIBHDC::CDIBHDC
//-------------------------------------------------------------------------
inline CDIBHDC::CDIBHDC( 
	CDIB *pdib )
{
	ASSERT_STRICT_VALID( pdib );

	m_hDC  = pdib->GetDC();
	m_pdib = pdib;
}

//-------------------------------------------------------------------------
// CDIBHDC::~CDIBHDC
//-------------------------------------------------------------------------
inline CDIBHDC::~CDIBHDC()
{
	m_pdib->ReleaseDC();
}

//-------------------------------------------------------------------------
// CDIB::GetDDSurface
//-------------------------------------------------------------------------
inline LPDIRECTDRAWSURFACE
CDIB::GetDDSurface()
{
	ASSERT( CBLTFormat::DIB_DIRECTDRAW == m_eType );
   ASSERT( m_pddsurfaceBack );

	m_hRes = m_pddsurfaceBack->IsLost();

	if ( m_hRes == DDERR_SURFACELOST )
		m_hRes = m_pddsurfaceBack->Restore();

	if ( FAILED( m_hRes ))
		;	// GGFIXIT: throw

	return m_pddsurfaceBack;
}

/////////////////////////////////////////////////////////////////////////////
// init a CDIB with a DIB
inline void CDIB::SetBits ( BITMAPFILEHEADER * pBfhSrc)
{

	ASSERT_STRICT_VALID (this);

	BITMAPINFOHEADER * pBih = (BITMAPINFOHEADER *) (pBfhSrc + 1);

	SetBits( pBih );
}

//--------------------------------------------------------------------------
// CDIB::SetBits
//--------------------------------------------------------------------------
inline void 
CDIB::SetBits( 
	BITMAPINFOHEADER* pBih )
{
	SetBits( pBih, GetImage( pBih ));
}

//-------------------------------------------------------------------------
// CDIB::Copy	
//-------------------------------------------------------------------------
inline void
CDIB::Copy(
	CDIB const * pdib )
{
	ASSERT( !IsLocked() );

	CDIBits	dibits = (( CDIB * )pdib )->GetBits();

	Copy(( BITMAPINFO * )(( CDIB * )pdib )->GetBitmapInfo(), ( BYTE * )dibits );
}

//-------------------------------------------------------------------------
// CDIB::Copy												
//-------------------------------------------------------------------------
inline void
CDIB::Copy(
	LPBITMAPINFO lpBmi )
{
	Copy( lpBmi, GetImage(( LPBITMAPINFOHEADER )lpBmi )); 
}


#endif
