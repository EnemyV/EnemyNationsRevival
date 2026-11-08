#ifndef VARCUT_H
#define VARCUT_H

#include "quant.h"

class VarSplitQuant : public ColorQuant
{
	private :

		struct ColorRect
		{
			__int64		m_iVariance;
			__int64		m_iTotalWeight;
			__int64		m_ttSum;
			__int64		m_tRed;
			__int64		m_tGreen;
			__int64		m_tBlue;

			int		redMin, redMax;		//  Min and max color values define the enclosed region
			int		greenMin, greenMax;
			int		blueMin, blueMax;
		};

		int			nReservedColorCount;
 		word 		*pHistogram;
		int			colorsUsed;
		int			*pColorList;
		ColorRect	*pColorRects;

		void AdjustRect( int rectIndex );
		void SplitColorRect( int splitRectIndex, int newRectIndex );
		void Sum(int ir, int ig, int ib,
                int jr, int jg, int jb,
                __int64 & total_weight,
                __int64 & tt_sum, __int64 & t_ur, __int64 & t_ug, __int64 & t_ub);
		void GetMinVariance (ColorRect * pRect, int iDimen, int & iIndex, __int64 & iVariance );
		__int64 Variance(__int64 tw, __int64 tt_sum, __int64 t_ur, __int64 t_ug, __int64 t_ub);
		RGBQUAD GetRGBColor( int rectIndex );

		enum { HISTSIZE = 64 * 64 * 64 };
		static int HistIndex( int r, int g, int b )
		{
			return ( ( r & 0xFC ) << 10 ) | ( ( g & 0xFC ) << 4 ) | ( ( b & 0xFC ) >> 2 );
		}

	public :

		VarSplitQuant( void );
		virtual ~VarSplitQuant( void );

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

#endif  //  VarSplit_H
