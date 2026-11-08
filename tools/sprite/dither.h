#ifndef DITHER_H
#define DITHER_H

#include "image.h"

class Dither
{
	public :

	virtual bool Init( const RGBColor *pPalette, int palSize ) = 0;
	virtual Image *DitherImage( const Image *pOriginalImage ) = 0;
};

#endif //  DITHER_H
