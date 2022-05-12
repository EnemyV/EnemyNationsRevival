#include <assert.h>

#include "stdafx.h"

#include "medcut.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
 *  Define the functions that will be used by the system qsort routine to
 *  sort lists of colors by red, green, or blue.  These cannot be class members,
 *  or for some reason qsort doesn't work ( everything is pretty much random ).
 */
static int __cdecl SortByRed( const void *p0, const void *p1 )
{
	int		w0 = *( int * )p0;
	int		w1 = *( int * )p1;

	int 	r0 = MedianCutQuant::RGB18Red( w0 );
	int 	r1 = MedianCutQuant::RGB18Red( w1 );

	return r0 - r1;
}
static int __cdecl SortByGreen( const void *p0, const void *p1 )
{
	int		w0 = *( int * )p0;
	int		w1 = *( int * )p1;

	int 	g0 = MedianCutQuant::RGB18Green( w0 );
	int 	g1 = MedianCutQuant::RGB18Green( w1 );

	return g0 - g1;
}
static int __cdecl SortByBlue( const void *p0, const void *p1 )
{
	int		w0 = *( int * )p0;
	int		w1 = *( int * )p1;

	int		b0 = MedianCutQuant::RGB18Blue( w0 );
	int 	b1 = MedianCutQuant::RGB18Blue( w1 );

	return b0 - b1;
}
static int (*sortCols[ 3 ])( const void *, const void * );

void MedianCutQuant::AdjustRect( int rectIndex )
{
	//  Get a pointer to the rect for convenience.
	ColorRect *pRect = &pColorRects[ rectIndex ];

	//  Make sure the rectangle is valid.
	assert( 0 <= pRect->first && pRect->first < colorsUsed );
	assert( pRect->first <= pRect->last && pRect->last < colorsUsed );

	//  Initialize the sum and bounding coordinates.  The actual sum and
	//  bounding coordinates are computed below.
	pRect->sum = 0;
	pRect->redMin = pRect->greenMin = pRect->blueMin = 255;
	pRect->redMax = pRect->greenMax = pRect->blueMax = 0;

	//  Compute the sum of all of the histogram entries, and find the bounding
	//  coordinates of this region.
	int index;
	for( index = pRect->first; index <= pRect->last; index++ )
	{
		pRect->sum += pHistogram[ pColorList[ index ] ];
		
		byte red 	= RGB18Red( pColorList[ index ] );
		byte green 	= RGB18Green( pColorList[ index ] );
		byte blue 	= RGB18Blue( pColorList[ index ] );

		if ( red < pRect->redMin )
			pRect->redMin = red;
		if ( red > pRect->redMax )
			pRect->redMax = red;

		if ( green < pRect->greenMin )
			pRect->greenMin = green;
		if ( green > pRect->greenMax )
			pRect->greenMax = green;

		if ( blue < pRect->blueMin )
			pRect->blueMin = blue;
		if ( blue > pRect->blueMax )
			pRect->blueMax = blue;
	}

	assert( pRect->redMin <= pRect->redMax );
	assert( pRect->greenMin <= pRect->greenMax );
	assert( pRect->blueMin <= pRect->blueMax );
}

void MedianCutQuant::SplitColorRect( int oldRect, int newRect )
{
	//  Get pointers to the new and old rects for convenience.
	ColorRect *pOldRect = &pColorRects[ oldRect ];
	ColorRect *pNewRect = &pColorRects[ newRect ];

	//  Determine which axis of the old cube is longest.
	int width = pOldRect->redMax - pOldRect->redMin;
	pOldRect->widestColor = 0;
	if ( width < pOldRect->greenMax - pOldRect->greenMin )
	{
		pOldRect->widestColor = 1;
		width = pOldRect->greenMax - pOldRect->greenMin;
	}
	if ( width < pOldRect->blueMax - pOldRect->blueMin )
	{
		pOldRect->widestColor = 2;
		width = pOldRect->blueMax - pOldRect->blueMin;
	}

	//  Sort the color list spanned by the old cube according to the color which
	//  lies along the longest axis of the cube.
	qsort( &pColorList[ pOldRect->first ], pOldRect->last - pOldRect->first + 1, sizeof( int ), sortCols[ pOldRect->widestColor ] );

	//  Find the median histogram count of the given rect.
	int medianSum = 0;
	int colIndex;
	for( colIndex = pOldRect->first; colIndex < pOldRect->last; colIndex++ )
	{
		if ( pOldRect->sum / 2 <= medianSum )
			break;
		medianSum += pHistogram[ pColorList[ colIndex ] ];
	}

	//  Set up the new color rect based on the old one.
	pNewRect->first = colIndex;
	pNewRect->last = pOldRect->last;
	AdjustRect( newRect );

	//  Adjust the old color rect.
	pOldRect->last = colIndex - 1;
	AdjustRect( oldRect );
}

RGBQUAD MedianCutQuant::GetRGBColor( int rectIndex )
{
	ColorRect *pRect = &pColorRects[ rectIndex ];

	int	rectSum = 0;
	int redSum = 0;
	int greenSum = 0;
	int blueSum = 0;

	int colorIndex;
	for( colorIndex = pRect->first; colorIndex <= pRect->last; colorIndex++ )
	{
		redSum 		+= RGB18Red( pColorList[ colorIndex ] ) * pHistogram[ pColorList[ colorIndex ] ];
		greenSum 	+= RGB18Green( pColorList[ colorIndex ] ) * pHistogram[ pColorList[ colorIndex ] ];
		blueSum 	+= RGB18Blue( pColorList[ colorIndex ] ) * pHistogram[ pColorList[ colorIndex ] ];
		rectSum		+= pHistogram[ pColorList[ colorIndex ] ];
	}
	assert( rectSum == pRect->sum );

	RGBQUAD	col;
	col.rgbRed 		= ( byte )( redSum / pRect->sum );
	col.rgbGreen 	= ( byte )( greenSum / pRect->sum );
	col.rgbBlue 	= ( byte )( blueSum / pRect->sum );

	return col;
}

MedianCutQuant::MedianCutQuant( void )
{
	pReservedColors = NULL;
	nReservedColorCount = 0;
	pHistogram = NULL;
	colorsUsed = 0;
	pColorList = NULL;
	pColorRects = NULL;
}

MedianCutQuant::~MedianCutQuant( void )
{
	//  Delete the color histogram.
	delete[] pHistogram;
	
	//  Delete the color list.
	delete[] pColorList;

	//  Delete the color rects.
	delete[] pColorRects;

	//  Delete the list of reserved colors
	delete[] pReservedColors;
}

bool MedianCutQuant::Init( const RGBQUAD *pPalette, int palSize )
{
	//  Allocate space for the color histogram
	pHistogram = new word[ HISTSIZE ];
	if ( pHistogram == NULL )
		return false;

	//  Clear the color histogram
	memset( pHistogram, 0, HISTSIZE * sizeof( word ) );

	//  Set up the color sort function table.
	sortCols[ 0 ] = SortByRed;
	sortCols[ 1 ] = SortByGreen;
	sortCols[ 2 ] = SortByBlue;

	//  Find the locked colors in the given palette.
	//  Any color marked PC_RESERVED should be used as-is.
	int palIndex;
	for( palIndex = 0; palIndex < palSize; palIndex++ )
	{
		if ( pPalette[ palIndex ].rgbReserved & PC_RESERVED )
			nReservedColorCount++;
	}
	if ( nReservedColorCount )
	{
		pReservedColors = new int[ nReservedColorCount ];
		if ( pReservedColors == NULL )
			return false;

		int reservedIndex = 0;
		for( palIndex = 0; palIndex < palSize; palIndex++ )
		{
			if ( pPalette[ palIndex ].rgbReserved & PC_RESERVED )
			{
				pReservedColors[ reservedIndex ] = HistIndex( 
					pPalette[ palIndex ].rgbRed, pPalette[ palIndex ].rgbGreen, pPalette[ palIndex ].rgbBlue );
				reservedIndex++;
			}
		}
		assert( reservedIndex == nReservedColorCount );
	}

	return true;
}

bool MedianCutQuant::AddImage( const Image *pImage )
{
	if ( pHistogram == NULL )
		return false;

	//  Loop through every pixel in the image, finding the 15-bit index corresponding
	//  to the 24-bit color and incrementing the histogram count at that pixel.
	byte *pPixels = pImage->_pPixels;
	int pixIndex;
	for( pixIndex = 0; pixIndex < pImage->_width * pImage->_height; pixIndex++ )
	{
		byte red = pPixels[ Image::RED_OFFSET ];
		byte green = pPixels[ Image::GREEN_OFFSET ];
		byte blue = pPixels[ Image::BLUE_OFFSET ];

		pPixels += 3;

		int colorIndex = HistIndex( red, green, blue );
		
		//  Search the table of reserved colors to see if
		//  this color matches one that we know is in the
		//  palette.
		int reservedIndex;
		for( reservedIndex = 0; reservedIndex < nReservedColorCount; reservedIndex++ )
		{
			if ( colorIndex == pReservedColors[ reservedIndex ] )
				goto $nextPixel;
		}

		if ( pHistogram[ colorIndex ] == 0 )
			colorsUsed++;
		
		if ( pHistogram[ colorIndex ] < 65535 )
			pHistogram[ colorIndex ]++;

$nextPixel:;
	}

	return true;
}

bool MedianCutQuant::CreatePalette( RGBQUAD *pPalette, int *pPalSize )
{
	assert( pPalette != NULL );
	assert( pPalSize != NULL );
	assert( 0 < *pPalSize && *pPalSize <= 256 );

	//  Allocate the array of colors actually used.
	pColorList = new int[ colorsUsed ];
	if ( pColorList == NULL )
		return false;
		
	//  Create the array of colors actually used.
	int usedIndex, histIndex;
	for( histIndex = 0, usedIndex = 0; usedIndex < colorsUsed && histIndex < HISTSIZE; histIndex++ )
	{
		if ( pHistogram[ histIndex ] != 0 )
			pColorList[ usedIndex++ ] = histIndex;
	}

#ifdef _DEBUG
	//  There should be no other non-zero entries in the histogram.
	assert( usedIndex == colorsUsed );
	for( ; histIndex < HISTSIZE; histIndex++ )
		assert( pHistogram[ histIndex ] == 0 );
#endif

	//  Figure out how many available slots are actually in the palette.
	//  Any slot marked reserved or no collapse cannot hold a new color,
	//  although reserved colors can be used as is.  No collapse colors
	//  cannot be used at all.
	int palIndex;
	int actualPalSize = *pPalSize;
	for( palIndex = 0; palIndex < *pPalSize; palIndex++ )
	{
		if ( ( pPalette[ palIndex ].rgbReserved & PC_RESERVED ) ||
			( pPalette[ palIndex ].rgbReserved & PC_NOCOLLAPSE ) )
			actualPalSize--;
	}
	//  Make sure we have some available entries.
	if ( actualPalSize <= 0 )
		return false;

	if ( colorsUsed <= actualPalSize )
	{
		//  There are fewer colors in the histogram buffer than entries in
		//  the palette, so just translate each directly.
		int colIndex, palIndex;
		for( palIndex = 0, colIndex = 0; palIndex < *pPalSize && colIndex < colorsUsed; palIndex++ )
		{
			if ( ( pPalette[ palIndex ].rgbReserved & PC_RESERVED ) == 0 &&
				( pPalette[ palIndex ].rgbReserved & PC_NOCOLLAPSE ) == 0 )
			{
				pPalette[ palIndex ].rgbRed 	= RGB18Red( pColorList[ colIndex ] );
				pPalette[ palIndex ].rgbGreen 	= RGB18Green( pColorList[ colIndex ] );
				pPalette[ palIndex ].rgbBlue	= RGB18Blue( pColorList[ colIndex ] );
				colIndex++;
			}
		}
		assert( colIndex == colorsUsed );
		
		//  Palette is complete.
		*pPalSize = colorsUsed;

		return true;
	}

	//  Allocate the array of color rects based on the number of desired palette entries.
	pColorRects = new ColorRect[ actualPalSize ];
	if ( pColorRects == NULL )
		return false;

	//  Set up the first rect.
	pColorRects[ 0 ].first = 0;
	pColorRects[ 0 ].last = colorsUsed - 1;
	AdjustRect( 0 );

	int rectsUsed = 1;
	while( rectsUsed < actualPalSize )
	{
		//  Choose the rect to split.  The split rect will be the one with the
		//  highest sum.
		int maxSum = 0, maxIndex = -1;
		int rectIndex;
		for( rectIndex = 0; rectIndex < rectsUsed; rectIndex++ )
		{
			//  Can't split a single color.
			if ( pColorRects[ rectIndex ].first != pColorRects[ rectIndex ].last )
			{
				if ( maxSum < pColorRects[ rectIndex ].sum )
				{
					maxSum = pColorRects[ rectIndex ].sum;
					maxIndex = rectIndex;
				}
			}
		}

		//  Make sure index is valid.
		assert( maxIndex != -1 );

		//  Split the rect and increment the count of rects used.
		SplitColorRect( maxIndex, rectsUsed );
		rectsUsed++;
	}

	//  Set the palette values based on the rects.
	int rectIndex;
	for( palIndex = 0, rectIndex = 0; palIndex < *pPalSize && rectIndex < rectsUsed; palIndex++ )
	{
		if ( ( pPalette[ palIndex ].rgbReserved & PC_RESERVED ) == 0 &&
			( pPalette[ palIndex ].rgbReserved & PC_NOCOLLAPSE ) == 0 )
		{
			pPalette[ palIndex ] = GetRGBColor( rectIndex );
			rectIndex++;
		}
	}
	assert( rectIndex == rectsUsed );

	*pPalSize = actualPalSize;
	
	return true;
}
