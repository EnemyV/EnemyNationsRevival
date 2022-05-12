#ifndef __GAMES_H__
#define __GAMES_H__


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "thielen.h"

/////////////////////////////////////////////////////////////////////////////
// assembler calls
extern "C" {
void WINAPI _mem32set (void  *pMem, BYTE bChr, long lNum);
void WINAPI _mem32cpy (void  *pDest, void  *pSrc, long lNum);
}

struct LOGPALETTE256
{
	WORD         palVersion;
	WORD         palNumEntries;
	PALETTEENTRY palPalEntry[256];
};

class CAppPalette : public CObject
{
public:
					CAppPalette ();
				  ~CAppPalette ();
	BOOL		Init (BOOL bUseHalf = FALSE, BOOL bUseSys = FALSE);
	void		Exit ();
	int			NumSys () {return (m_iNumSys);}
	HPALETTE hPal () {return (m_hPal);}
	DWORD		GetColorValue( COLORREF, int iBitsPerPixel ) const;

	void		Animate();
	void		Activate (HWND hWnd, HDC hDC, BOOL bActive);
	void		Paint (HDC hDC );
	void		EndPaint (HDC hDC );
	long		PalMsg (HDC hDC, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) const;

	void		SetColors (RGBQUAD *pRgb, int iFirst, int iNumClrs);
	void		GetColors (RGBQUAD *pRgb, int iFirst, int iNumClrs) const;
	void		NewWnd (BITMAPINFO256 & bmi);
	void		Fadeout ();
	void		Fadein ();
	COLORREF	GetColor (int iIndex) const {return (RGB (m_lPal.palPalEntry[iIndex].peRed,
						m_lPal.palPalEntry[iIndex].peGreen, m_lPal.palPalEntry[iIndex].peBlue));}
	DWORD		GetDeviceColor( int	iIndex, int	iBitsPerPixel ) const;
	BYTE	  *GetDeviceColor( int iIndex ) const
				{
					ASSERT( m_pdwColors ); 
					ASSERT( 0 <= iIndex && iIndex < 256 );

					return ( BYTE * )( m_pdwColors + iIndex );
				}

	void		UpdateDeviceColors( int iStart, int iCount );

private:
	void		SysColors (BOOL bActive, HDC hdc);

private:
	HPALETTE				m_hPal;
	HPALETTE				m_hNotActivePal;
	int							m_iNumSys;
	LOGPALETTE256		m_lPal;
	BOOL						m_bHalf;
	BOOL						m_bSysClrs;
	DWORD					*m_pdwColors;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

//------------------------- C C o l o r B u f f e r ------------------------
//
// Buffer filled with single device-dependent color for memcpy's

class CColorBuffer
{

public:

	CColorBuffer( int iIndexColor = 0 );
  ~CColorBuffer();

	void			 SetColor ( int iIndexColor, int nPixels = 0 );
	BYTE const * GetBuffer( int nPixels );
	BYTE const * GetBuffer() { return m_pby; }

protected:

	void	FillColor( int iFirst, int iLast );
	void	SetSize  ( int nPixelCount );

private:

	BYTE	* m_pby;
	int	  m_nPixelCount;
	int	  m_nPixelCapacity;
	int	  m_iIndexColor;
};


extern CAppPalette 	thePal;

inline CPoint operator + ( CPoint const &pt1, CPoint const &pt2 )
{
	return CPoint( pt1.x + pt2.x, pt1.y + pt2.y );
}

inline CPoint operator - ( CPoint const &pt1, CPoint const &pt2 )
{
	return CPoint( pt1.x - pt2.x, pt1.y - pt2.y );
}

#endif
