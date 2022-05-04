#ifndef STUCKI_H
#define STUCKI_H

#include "dither.h"

//  The position of each color channel in the image.
/*#define RED_OFFSET		0
#define GREEN_OFFSET	1
#define BLUE_OFFSET		2
*/

//  Should be nested class when supported by compiler.
struct sortedColor
{
	RGBColor 	color;
	byte		originalIndex;
};

class StuckiDither : public Dither
{
	private :

		RGBColor	aOriginalPalette[ 256 ];
		sortedColor	aSortedPalette[ 256 ];
		int			palSize;
		byte		redLookup[ 256 ];

		void 		CreateSortedPalette( const RGBColor *pPalette, int palSize );
		int			FindClosestColor( int r, int g, int b );

	public :

		//  Default ctor because compiler can't 
		//  generate one.  Why not?
		StuckiDither()	{}

		virtual bool Init( const RGBColor *pPalette, int palSize );
		virtual Image *DitherImage( const Image *pOriginalImage );
};

#endif  //  STUCKI_H
