#ifndef DRAWDIB_H
#define DRAWDIB_H

#include "drawsurf.h"

class DrawDIB : public DrawSurface
{
	private :

	struct bitmapInfo
	{
		BITMAPINFOHEADER	bih;
		union
		{
			RGBQUAD			aPalette[ 256 ];
			DWORD			aRGBMasks[ 3 ];
		};
	};

	int _width, _height;
	int _stride;
	int _bitDepth;
	byte *_pPixels;
	bitmapInfo	bi;
	HPALETTE	hWinPalette;

	bool CreateIdentPal( RGBColor *pPalette );

	public :

	DrawDIB();
	virtual	~DrawDIB();

	//  Create a drawable surface of the given size.
	virtual bool	Create( int width, int height, int bitDepth, RGBColor *pPal = NULL );

	//  Blit the drawable surface to the given window.
	virtual void	Blit( HDC hDC );

	virtual byte *	GetPixels() 	{ return _pPixels; }	//  Pointer to pixels
	virtual int		GetWidth()		{ return _width; }		//  Width in pixels
	virtual int		GetHeight()		{ return _height; }		//  Height in pixels
	virtual int		GetStride()		{ return _stride; }		//  Number of bytes between one row and the next - 
															//  negative if surface is bottom-up.
	virtual int		GetBitDepth()	{ return _bitDepth; }	//  Bit depth ( 8, 15, 16, 24 ).
};

#endif
