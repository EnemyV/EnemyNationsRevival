#ifndef BOXSPLIT_H
#define BOXSPLIT_H

#include "quant.h"

class BoxSplitQuant : public ColorQuant
{
	private :

		struct ColorRect
		{
//BUGBUG			int 	first, last;		//  Indices into color list.
			byte	redMin, redMax;		//  Min and max color values define the enclosed region
			byte	greenMin, greenMax;
			byte	blueMin, blueMax;

			int		widestColor;
		};

//BUGBUG		int *		pReservedColors;
		int			nReservedColorCount;
 		word 		*pHistogram;
		int			colorsUsed;
		int			*pColorList;
		ColorRect	*pColorRects;

		void AdjustRect( int rectIndex );
		void SplitColorRect( int splitRectIndex, int newRectIndex );
		RGBQUAD GetRGBColor( int rectIndex );

		enum { HISTSIZE = 64 * 64 * 64 };
		static int HistIndex( byte r, byte g, byte b )
		{
			return ( ( r & 0xFC ) << 10 ) | ( ( g & 0xFC ) << 4 ) | ( ( b & 0xFC ) >> 2 );
		}

	public :

		BoxSplitQuant( void );
		virtual ~BoxSplitQuant( void );

		//  Initialize the quantizer.  The palette is used to pre-load the 
		//  quantizer with certain colors, which are used as-is - if there is
		//  an exact match with a color known to be in the palette, no entry
		//  will be added to the histogram.
		virtual bool Init( const RGBQUAD *pPalette, int palSize );

		virtual bool AddImage( const Image *pImage );
		virtual bool CreatePalette( RGBQUAD *pPalette, int *pPalSize );

		//  I don't really want to make these public, but the system qsort
		//  routine seems to have problems with class members, and these are
		//  needed by the compare routines for the sort, so here they are...
		static byte RGB18Red( dword rgb18 )		{  return ( byte )( ( rgb18 >> 10 ) & 0xFC );  }
		static byte RGB18Green( dword rgb18 )	{  return ( byte )( ( rgb18 >> 4 ) & 0xFC );  }
		static byte RGB18Blue( dword rgb18 )	{  return ( byte )( ( rgb18 << 2 ) & 0xFC );  }
};

#endif  //  BOXSPLIT_H
