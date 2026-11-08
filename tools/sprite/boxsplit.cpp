#include <assert.h>

#include "stdafx.h"

#include "boxsplit.h"

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

	int 	r0 = BoxSplitQuant::RGB18Red( w0 );
	int 	r1 = BoxSplitQuant::RGB18Red( w1 );

	return r0 - r1;
}
static int __cdecl SortByGreen( const void *p0, const void *p1 )
{
	int		w0 = *( int * )p0;
	int		w1 = *( int * )p1;

	int 	g0 = BoxSplitQuant::RGB18Green( w0 );
	int 	g1 = BoxSplitQuant::RGB18Green( w1 );

	return g0 - g1;
}
static int __cdecl SortByBlue( const void *p0, const void *p1 )
{
	int		w0 = *( int * )p0;
	int		w1 = *( int * )p1;

	int		b0 = BoxSplitQuant::RGB18Blue( w0 );
	int 	b1 = BoxSplitQuant::RGB18Blue( w1 );

	return b0 - b1;
}
static int (*sortCols[ 3 ])( const void *, const void * );

// shrink the bounding box down around all points inside it
void BoxSplitQuant::AdjustRect( int rectIndex )
{
	//  Get a pointer to the rect for convenience.
	ColorRect *pRect = &pColorRects[ rectIndex ];

	//  Initialize the sum and bounding coordinates.  The actual sum and
	//  bounding coordinates are computed below.
	byte redMax, greenMax, blueMax, redMin, greenMin, blueMin;
	redMin = greenMin = blueMin = 255;
	redMax = greenMax = blueMax = 0;

	//  Find the bounding coordinates of this region.
	int index;
	for( index = 0; index < colorsUsed; index++ )
	{
		byte red 	= RGB18Red( pColorList[ index ] );
		byte green 	= RGB18Green( pColorList[ index ] );
		byte blue 	= RGB18Blue( pColorList[ index ] );

		// if not in our box, ignore it
		if ( ( red < pRect->redMin ) || ( red > pRect->redMax ) ||
										( green < pRect->greenMin ) || ( green > pRect->greenMax ) ||
										( blue < pRect->blueMin ) || ( blue > pRect->blueMax ) )
			continue;

		// get the smallest box that holds everything
		if ( red < redMin )
			redMin = red;
		if ( red > redMax )
			redMax = red;

		if ( green < greenMin )
			greenMin = green;
		if ( green > greenMax )
			greenMax = green;

		if ( blue < blueMin )
			blueMin = blue;
		if ( blue > blueMax )
			blueMax = blue;
	}

	assert( redMax <= pRect->redMax );
	assert( redMin >= pRect->redMin );
	assert( greenMax <= pRect->greenMax );
	assert( greenMin >= pRect->greenMin );
	assert( blueMax <= pRect->blueMax );
	assert( blueMin >= pRect->blueMin );

	// give it the new tighter values
	pRect->redMax = redMax;
	pRect->redMin = redMin;
	pRect->greenMax = greenMax;
	pRect->greenMin = greenMin;
	pRect->blueMax = blueMax;
	pRect->blueMin = blueMin;

	//  Determine which axis of the old cube is longest.
	int width = pRect->redMax - pRect->redMin;
	pRect->widestColor = 0;
	if ( width < pRect->greenMax - pRect->greenMin )
	{
		pRect->widestColor = 1;
		width = pRect->greenMax - pRect->greenMin;
	}
	if ( width < pRect->blueMax - pRect->blueMin )
	{
		pRect->widestColor = 2;
		width = pRect->blueMax - pRect->blueMin;
	}

	assert( pRect->redMin <= pRect->redMax );
	assert( pRect->greenMin <= pRect->greenMax );
	assert( pRect->blueMin <= pRect->blueMax );
}

void BoxSplitQuant::SplitColorRect( int oldRect, int newRect )
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

	// dup it into the new one
	* pNewRect = * pOldRect;

	//  Split the box along the longest axis.
	switch (pOldRect->widestColor)
	  {
		case 0 :
			pOldRect->redMax = (byte) ( pOldRect->redMin + ( pOldRect->redMax - pOldRect->redMin ) / 2 );
			pNewRect->redMin = (byte) ( pOldRect->redMax + 1 );
			break;
		case 1 :
			pOldRect->greenMax = (byte) ( pOldRect->greenMin + ( pOldRect->greenMax - pOldRect->greenMin ) / 2 );
			pNewRect->greenMin = (byte) ( pOldRect->greenMax + 1 );
			break;
		case 2 :
			pOldRect->blueMax = (byte) ( pOldRect->blueMin + ( pOldRect->blueMax - pOldRect->blueMin ) / 2 );
			pNewRect->blueMin = (byte) ( pOldRect->blueMax + 1 );
			break;
	  }
	
	//  Set up the new color rect based on the old one.
	AdjustRect( newRect );
	AdjustRect( oldRect );
}

RGBQUAD BoxSplitQuant::GetRGBColor( int rectIndex )
{
	ColorRect *pRect = &pColorRects[ rectIndex ];

	RGBQUAD	col;
	col.rgbRed 		= ( byte ) (pRect->redMax == 255 ? 255 : pRect->redMin + ( pRect->redMax - pRect->redMin ) / 2);
	col.rgbGreen	= ( byte ) (pRect->greenMax == 255 ? 255 : pRect->greenMin + ( pRect->greenMax - pRect->greenMin ) / 2);
	col.rgbBlue		= ( byte ) (pRect->blueMax == 255 ? 255 : pRect->blueMin + ( pRect->blueMax - pRect->blueMin ) / 2);

	return col;
}

BoxSplitQuant::BoxSplitQuant( void )
{
//bugbug	pReservedColors = NULL;
	nReservedColorCount = 0;
	pHistogram = NULL;
	colorsUsed = 0;
	pColorList = NULL;
	pColorRects = NULL;
}

BoxSplitQuant::~BoxSplitQuant( void )
{
	//  Delete the color histogram.
	delete[] pHistogram;
	
	//  Delete the color list.
	delete[] pColorList;

	//  Delete the color rects.
	delete[] pColorRects;

	//  Delete the list of reserved colors
//bugbug	delete[] pReservedColors;
}

bool BoxSplitQuant::Init( const RGBQUAD *pPalette, int palSize )
{
	//  Allocate space for the color histogram
	pHistogram = new word[ HISTSIZE + 2 ];
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
	nReservedColorCount = 0;
	for( palIndex = 0; palIndex < palSize; palIndex++ )
	{
		if ( pPalette[ palIndex ].rgbReserved & PC_RESERVED )
			{
			nReservedColorCount++;
			int colorIndex = HistIndex( pPalette[ palIndex ].rgbRed, pPalette[ palIndex ].rgbGreen, pPalette[ palIndex ].rgbBlue );
			assert (colorIndex < HISTSIZE);
			pHistogram[ colorIndex ]++;
			colorsUsed++;
			}
	}
#ifdef BUGBUG
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
#endif

	return true;
}

bool BoxSplitQuant::AddImage( const Image *pImage )
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
#ifdef BUGBUG
		int reservedIndex;
		for( reservedIndex = 0; reservedIndex < nReservedColorCount; reservedIndex++ )
		{
			if ( colorIndex == pReservedColors[ reservedIndex ] )
				goto $nextPixel;
		}
#endif

		assert (colorIndex < HISTSIZE);
		if ( pHistogram[ colorIndex ] == 0 )
			colorsUsed++;
		
		if ( pHistogram[ colorIndex ] < SHRT_MAX )
			pHistogram[ colorIndex ]++;

	}

	return true;
}

bool BoxSplitQuant::CreatePalette( RGBQUAD *pPalette, int *pPalSize )
{
	assert( pPalette != NULL );
	assert( pPalSize != NULL );
	assert( 0 < *pPalSize && *pPalSize <= 256 );

	//  Allocate the array of colors actually used.
	pColorList = new int[ colorsUsed + 2];
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
	actualPalSize += nReservedColorCount;
	pColorRects = new ColorRect[ actualPalSize + 2];
	if ( pColorRects == NULL )
		return false;

	//  Set up the first rect.
	pColorRects[0].redMin = pColorRects[0].greenMin = pColorRects[0].blueMin = 0;
	pColorRects[0].redMax = pColorRects[0].greenMax = pColorRects[0].blueMax = 255;
	AdjustRect( 0 );

	int rectsUsed = 1;
	while( rectsUsed < actualPalSize )
	{
		//  Choose the rect to split.  The split rect will be the one which is widest along
		//  any axis.
		int maxWidth = 0, maxIndex = -1;
		int rectIndex;
		for( rectIndex = 0; rectIndex < rectsUsed; rectIndex++ )
		{
				int width;
				if ( pColorRects[ rectIndex ].widestColor == 0 )
					width = pColorRects[ rectIndex ].redMax - pColorRects[ rectIndex ].redMin;
				else if ( pColorRects[ rectIndex ].widestColor == 1 )
					width = pColorRects[ rectIndex ].greenMax - pColorRects[ rectIndex ].greenMin;
				else
					width = pColorRects[ rectIndex ].blueMax - pColorRects[ rectIndex ].blueMin;

				if ( maxWidth < width )
				{
					maxWidth = width;
					maxIndex = rectIndex;
				}
		}

		//  Make sure index is valid.
		assert( maxIndex != -1 );

		//  Split the rect and increment the count of rects used.
		SplitColorRect( maxIndex, rectsUsed );
		rectsUsed++;
	}

	//  Set the palette values based on the rects.
	//  We have to watch for blocks that belong to a reserved color
	int rectIndex;
	for( palIndex = 0, rectIndex = 0; rectIndex < rectsUsed; rectIndex++ )
	{
		// is it a reserved one?
		for( int resIndex = 0; resIndex < *pPalSize; resIndex++ )
			if ( ( pPalette[ resIndex ].rgbReserved & PC_RESERVED ) != 0 )
				{
				int iIndex = HistIndex ( pPalette[ resIndex ].rgbRed, pPalette[ resIndex ].rgbGreen, pPalette[ resIndex ].rgbBlue );
				byte red = RGB18Red ( iIndex );
				byte green = RGB18Green ( iIndex );
				byte blue = RGB18Blue ( iIndex );
				if ( (pColorRects[rectIndex].redMin <= red) && (red <= pColorRects[rectIndex].redMax) &&
						 (pColorRects[rectIndex].greenMin <= green) && (green <= pColorRects[rectIndex].greenMax) &&
						 (pColorRects[rectIndex].blueMin <= blue) && (blue <= pColorRects[rectIndex].blueMax) )
					goto got_it;
				}

		// not reserved - next free palette entry
		while ( ( pPalette[ palIndex ].rgbReserved & PC_RESERVED ) != 0 ||
			( pPalette[ palIndex ].rgbReserved & PC_NOCOLLAPSE ) != 0 )
			palIndex++;

		if (palIndex >= *pPalSize)
			break;

		assert (palIndex <= 245);
		pPalette[ palIndex ] = GetRGBColor( rectIndex );
		palIndex++;

got_it:
		;
	}
	assert( rectIndex == rectsUsed );

	*pPalSize = actualPalSize;
	
	delete[] pHistogram;
	pHistogram = NULL;
	delete[] pColorList;
	pColorList = NULL;


	return true;
}
