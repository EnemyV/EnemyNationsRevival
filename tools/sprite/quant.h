#ifndef QUANT_H
#define QUANT_H

#include "image.h"

/*
 *  Abstract base class for color quantization routines.
 */
class ColorQuant
{
	public :

		virtual ~ColorQuant() = 0;

		//  Initialize the quantizer.  The palette is used to pre-load the 
		//  quantizer with certain colors, which are used as-is - if there is
		//  an exact match with a color known to be in the palette, no entry
		//  will be added to the histogram.
		virtual bool Init( const RGBQUAD *pPalette, int palSize ) = 0;

		//  Adds the given image to pixels to be quantized.
		virtual bool AddImage( const Image *pImage ) = 0;

		//  Generate a palette of the given size from the pixels previously loaded.
		virtual bool CreatePalette( RGBQUAD *pPalette, int *pPalSize ) = 0;
};

inline ColorQuant::~ColorQuant()
{
}

#endif
