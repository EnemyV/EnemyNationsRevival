#ifndef __BLT_H__
#define __BLT_H__


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "thielen.h"

//------------------------------ C B L T B a s e ---------------------------

class CBLTBase
#ifdef _DEBUG
: public CObject
#endif
{

public:

	CBLTBase()					{ SetValid( FALSE );	}

	BOOL	IsValid() const 	{ return m_bValid; }

protected:

	void	SetValid( BOOL bValid )	{ m_bValid = bValid;	}

	BOOL	m_bValid;
};

//-------------------------------- C W i n G -------------------------------

class CWinG : public CBLTBase
{

public:

	static CWinG * GetTheWinG();

  ~CWinG();

	HDC		CreateDC				();
	BOOL		RecommendDIBFormat( BITMAPINFO * );
	HBITMAP	CreateBitmap		( HDC, BITMAPINFO FAR *, void FAR *FAR * );
	void *	GetDIBPointer		( HBITMAP, BITMAPINFO FAR * );
	UINT		SetDIBColorTable	( HDC, UINT, UINT, RGBQUAD const FAR * );
	BOOL		BitBlt				( HDC, int, int, int, int, HDC, int, int );
	BOOL		StretchBlt			( HDC, int, int, int, int, HDC, int, int, int, int );

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

protected:

	CWinG();

private:

	HINSTANCE	m_hInstLib;
};

//---------------------------- C D i r e c t D r a w ------------------------

class CDirectDraw : public CBLTBase
{

public:

	static CDirectDraw * GetTheDirectDraw();

  ~CDirectDraw();

	LPDIRECTDRAW			GetDD() 						{ return m_pdirectdraw;    }
	LPDIRECTDRAWSURFACE	GetFrontSurface();

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

protected:

	CDirectDraw();

private:

	HRESULT					m_hRes;				// last COM return code
	HINSTANCE				m_hInstDDrawLib;
	LPDIRECTDRAW 			m_pdirectdraw;		// Direct Draw object
	DDSURFACEDESC			m_ddPrimSurfDesc;	// primary surface description
	LPDIRECTDRAWSURFACE	m_pddsurfacePrim;	// primary surface (the screen)
	LPDIRECTDRAWCLIPPER	m_pddclipper;		// primary surface clipper
};

//------------------------- C C o l o r F o r m a t -----------------------
//
// Device-dependent color format info

class CColorFormat
#ifdef _DEBUG
: public CObject
#endif
{

public:

	enum COLOR_DEPTH
	{
		DEPTH_EIGHT 		= 8,
		DEPTH_FIFTEEN 		= 15,
		DEPTH_SIXTEEN  	= 16,
		DEPTH_TWENTYFOUR 	= 24,
		DEPTH_THIRTYTWO	= 32
	};

	CColorFormat() { CalcScreenFormat(); }
	CColorFormat( COLOR_DEPTH );

	#ifdef _DEBUG	// VC++ braindeadness
	CColorFormat( const CColorFormat &colorformat ) : m_iBitsPerPixel ( colorformat.GetBitsPerPixel()  ),
	                                                  m_iBytesPerPixel( colorformat.GetBytesPerPixel() ) {} 
	#endif

	BOOL	operator == ( CColorFormat const & colorformat ) const	{ return m_iBitsPerPixel == colorformat.m_iBitsPerPixel; }
	BOOL	operator != ( CColorFormat const & colorformat ) const	{ return m_iBitsPerPixel != colorformat.m_iBitsPerPixel; }

	void	CalcScreenFormat();

	void	SetBitsPerPixel(int iBits);
	int	GetBitsPerPixel()  const { return m_iBitsPerPixel;  }
	int	GetBytesPerPixel() const { return m_iBytesPerPixel; }

#ifdef _DEBUG
	virtual void AssertValid() const;
#endif

protected:

	void	CalcBytesPerPixel();

private:

	int	m_iBitsPerPixel;
	int	m_iBytesPerPixel;
};

//--------------------------- C B L T F o r m a t ---------------------------
//

class CBLTFormat
#ifdef _DEBUG
: public CObject
#endif
{

public:

	enum DIB_TYPE
	{
		DIB_DIRECTDRAW,
		DIB_WING,
		DIB_DIBSECTION,
		DIB_MEMORY,
		DIB_NUM_TYPES
	};

	enum DIB_DIRECTION
	{
		DIR_TOPDOWN	 = -1,
		DIR_BOTTOMUP =  1
	};

	CBLTFormat();
	CBLTFormat( CColorFormat const & );

	BOOL	Init();

	DIB_TYPE			GetType()			 const { return m_eType;       }
	DIB_DIRECTION	GetDirection()		 const { return m_eDirection;  }
	DIB_DIRECTION	GetMemDirection()		 const;
	CColorFormat	GetColorFormat()	 const { return m_colorformat; }
	int				GetBitsPerPixel()  const { return m_colorformat.GetBitsPerPixel();  }
	int				GetBytesPerPixel() const { return m_colorformat.GetBytesPerPixel(); }

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

protected:

	DIB_TYPE	CalcBltMethod();

private:

	DIB_TYPE			m_eType;
	DIB_DIRECTION	m_eDirection;
	CColorFormat	m_colorformat;
};

extern Ptr< CBLTFormat >	ptrthebltformat;
extern Ptr< CWinG >			ptrtheWinG;
extern Ptr< CDirectDraw >	ptrtheDirectDraw;

#endif
