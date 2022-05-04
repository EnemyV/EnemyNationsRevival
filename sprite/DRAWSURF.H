#ifndef DRAWSURF_H
#define DRAWSURF_H

#include "common.h"
#include "rgbcolor.h"

class DrawSurface
{
	public :

	//  Create a drawable surface of the given size.
	virtual bool	Create( int width, int height, int bitDepth, RGBColor *pPal ) = 0;

	//  Blit the drawable surface to the given window.
	virtual void	Blit( HDC hDC ) = 0;

	virtual byte *	GetPixels() = 0;	//  Pointer to pixels
	virtual int		GetWidth() = 0;		//  Width in pixels
	virtual int		GetHeight() = 0;	//  Height in pixels
	virtual int		GetStride() = 0;	//  Number of bytes between one row and the next - 
										//  negative if surface is bottom-up.
	virtual int		GetBitDepth() = 0;	//  Bit depth ( 8, 15, 16, 24 ).
};

#endif
