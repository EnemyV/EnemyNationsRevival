#include "minvarq.h"

MinimalVarianceQuant::MinimalVarianceQuant( void )
{
	//  Allocate space for the color histogram,
	//  clear it, clear the number of colors used.
	pHistogram = new word[ HISTSIZE ];
	if ( pHistogram == NULL )
		return;
	memset( pHistogram, 0, HISTSIZE * sizeof( word ) );
	colorsUsed = 0;

	( void )memset( &originalColorRect, 0, sizeof( originalColorRect ) );

	//  Gets allocated later.
	pColorRects = NULL;
	pColorList = NULL;
}

MinimalVarianceQuant::~MinimalVarianceQuant( void )
{
	//  Delete the color histogram.
	delete[] pHistogram;

	//  Delete the color rect table.
	delete[] pColorRects;

	//  Delete the list of colors.
	delete[] pColorList;
}

bool MinimalVarianceQuant::AddImage( const Image *pImage )
{
	assert( pImage != NULL );

	if ( pHistogram == NULL )
		return false;

	//  Loop through every pixel in the image, finding the 15-bit index corresponding
	//  to the 24-bit color and incrementing the histogram count at that pixel.
	byte *pPixels = pImage->_pPixels;
	int pixIndex;
	for( pixIndex = 0; pixIndex < pImage->_width * pImage->_height; pixIndex++ )
	{
		byte red = *pPixels++;
		byte green = *pPixels++;
		byte blue = *pPixels++;

		int colorIndex = HistIndex( red, green, blue );
		
		if ( pHistogram[ colorIndex ] == 0 )
			colorsUsed++;
		
		if ( pHistogram[ colorIndex ] < 65535 )
			pHistogram[ colorIndex ]++;

		//  Update the original color rect.
		originalColorRect.redFrequency[ red ]++;
		originalColorRect.greenFrequency[ green ]++;
		originalColorRect.blueFrequency[ blue ]++;
		originalColorRect.totalWeight++;
		totalHistogramWeight++;
	}

	return true;
}

void MinimalVarianceQuant::AdjustRect( int rectIndex )
{
	ColorRect *pRect = &pColorRects[ rectIndex ];

	int colorIndex, mean, var;
	for ( colorIndex = pRect->redMin; colorIndex < pRect->redMax; colorIndex++ )
	{
		mean += colorIndex * pRect->redFrequency[ colorIndex ];
		var += colorIndex * colorIndex * pRect->redFrequency[ colorIndex ];
	}
	pRect->redMean = mean / pRect->totalWeight;
	pRect->variance = var - mean * mean / pRect->totalWeight;

	int colorIndex, mean, var;
	for ( colorIndex = pRect->greenMin; colorIndex < pRect->greenMax; colorIndex++ )
	{
		mean += colorIndex * pRect->greenFrequency[ colorIndex ];
		var += colorIndex * colorIndex * pRect->greenFrequency[ colorIndex ];
	}
	pRect->greenMean = mean / pRect->totalWeight;
	pRect->variance += var - mean * mean / pRect->totalWeight;

	int colorIndex, mean, var;
	for ( colorIndex = pRect->blueMin; colorIndex < pRect->blueMax; colorIndex++ )
	{
		mean += colorIndex * pRect->blueFrequency[ colorIndex ];
		var += colorIndex * colorIndex * pRect->blueFrequency[ colorIndex ];
	}
	pRect->blueMean = mean / pRect->totalWeight;
	pRect->variance += var - mean * mean / pRect->totalWeight;

	pRect->variance /= totalHistogramWeight;
}

void MinimalVarianceQuant::SplitColorRect( int oldIndex, int newIndex )
{
	ColorRect *pOldRect = &pColorRects[ oldIndex ];
	ColorRect *pNewRect = &pColorRects[ newIndex ];


}

bool MinimalVarianceQuant::CreatePalette( RGBCOLOR *pPalette, int *pPalSize )
{
	assert( pPalette != NULL );
	assert( pPalSize != NULL );

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

	//  Set up the initial box.
	pColorRects[ 0 ] = originalColorRect;
	pColorRects[ 0 ].redMin = 0;	
	pColorRects[ 0 ].greenMin = 0;	
	pColorRects[ 0 ].blueMin = 0;	
	pColorRects[ 0 ].redMax = 255;	
	pColorRects[ 0 ].greenMax = 255;	
	pColorRects[ 0 ].blueMax = 255;	
	AdjustRect( 0 );

	//  Loop until I have enough colors, choosing
	//  the box with the highest variance to split
	//  each time.
	int rectsUsed = 1;
	while( rectsUsed < actualPalSize )
	{
		float maxVariance = 0.0f;
		int maxIndex = -1;
		int rectIndex;
		for( rectIndex = 0; rectIndex < rectsUsed; rectIndex++ )
		{
			if ( pColorRects[ rectIndex ].redMin != pColorRects[ rectIndex ].redMax ||
				pColorRects[ rectIndex ].redMin != pColorRects[ rectIndex ].redMax ||
				pColorRects[ rectIndex ].redMin != pColorRects[ rectIndex ].redMax )
			{
				if ( pColorRects[ rectIndex ].variance > maxVariance )
				{
					maxVariance = pColorRects[ rectIndex ].variance;
					maxIndex = rectIndex;
				}
			}
		}
		assert( maxIndex != -1 );

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