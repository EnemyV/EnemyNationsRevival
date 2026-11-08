#include <assert.h>

#include "stdafx.h"

#include "varcut.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// shrink the bounding box down around all points inside it
void VarSplitQuant::AdjustRect( int rectIndex )
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

	assert( pRect->redMin <= pRect->redMax );
	assert( pRect->greenMin <= pRect->greenMax );
	assert( pRect->blueMin <= pRect->blueMax );

	Sum ( pRect->redMin, pRect->greenMin, pRect->blueMin, 
				pRect->redMax, pRect->greenMax, pRect->blueMax, 
				pRect->m_iTotalWeight, pRect->m_ttSum,
				pRect->m_tRed, pRect->m_tGreen, pRect->m_tBlue );
	pRect->m_iVariance = Variance (pRect->m_iTotalWeight, pRect->m_ttSum,
				pRect->m_tRed, pRect->m_tGreen, pRect->m_tBlue );
}

// shrink the bounding box down around all points inside it
void VarSplitQuant::Sum(int ir, int ig, int ib,
                int jr, int jg, int jb,
                __int64 & total_weight,
                __int64 & tt_sum, __int64 & t_ur, __int64 & t_ug, __int64 & t_ub)
{
  __int64 rs, ts;

  total_weight = tt_sum = t_ur = t_ug = t_ub = 0;

  for (int r = ir; r <= jr; r+=4)
	  {
    rs = r * r;

    for (int g = ig; g <= jg; g+=4)
  	  {
      ts = rs + (g * g);

      for (int b = ib; b <= jb; b+=4)
				{
				int w = pHistogram [ HistIndex ( r, g, b ) ];
        if ( w )
    	    {
					// if we've maxed out, time to leave
					__int64 iTmp = ( ts + b * b ) * w;
					if ( tt_sum > _I64_MAX-iTmp )
						{
						TRAP ();
						return;
						}

          total_weight += w;
          t_ur += r * w;
          t_ug += g * w;
          t_ub += b * w;
          tt_sum += iTmp;
      	  }
				}
	    }
  	}
}

__int64 VarSplitQuant::Variance(__int64 tw, __int64 tt_sum,
                     __int64 t_ur, __int64 t_ug, __int64 t_ub)
{
  double temp;

  /* the following calculations can be performed in fixed point
   * if needed - just be sure to preserve enough precision!
   */

  temp  = (double)t_ur * (double)t_ur;
  temp += (double)t_ug * (double)t_ug;
  temp += (double)t_ub * (double)t_ub;
  temp /= (double)tw;

	TRAP ( (double)tt_sum < temp);
  return ((__int64)((double)tt_sum - temp));
}

void VarSplitQuant::GetMinVariance (ColorRect * pRect, int iDimen, int & iIndex, __int64 & iVariance )
{

  /* left box's initial statistics */

  __int64 total_weight1         = 0;
  __int64 tt_sum1               = 0;
  __int64 t_ur1                 = 0;
  __int64 t_ug1                 = 0;
  __int64 t_ub1                 = 0;

  /* right box's initial statistics */

  __int64 total_weight2         = pRect->m_iTotalWeight;
  __int64 tt_sum2               = pRect->m_ttSum;
  __int64 t_ur2                 = pRect->m_tRed;
  __int64 t_ug2                 = pRect->m_tGreen;
  __int64 t_ub2                 = pRect->m_tBlue;

  /* locate optimum split point on axis */

  iVariance = _I64_MAX;

	int iMax, iMin;
	switch (iDimen)
	  {
		case 0 :
			iMin = pRect->redMin;
			iMax = pRect->redMax;
			break;
		case 1 :
			iMin = pRect->greenMin;
			iMax = pRect->greenMax;
			break;
		case 2 :
			iMin = pRect->blueMin;
			iMax = pRect->blueMax;
			break;
		default:
			ASSERT (FALSE);
			break;
	  }

	int i;
  for (i = iMin; i < iMax; i+=4)
  {
    /* calculate the statistics for the area being taken
     * away from the right box and given to the left box
     */

	  __int64 total_weight3;
  	__int64 tt_sum3, t_ur3, t_ug3, t_ub3;
		switch (iDimen)
		  {
			case 0 :
		    Sum (i, pRect->greenMin, pRect->blueMin, i, pRect->greenMax, pRect->blueMax,
  			      total_weight3, tt_sum3, t_ur3, t_ug3, t_ub3);
				break;
			case 1 :
		    Sum (pRect->redMin, i, pRect->blueMin, pRect->redMax, i, pRect->blueMax,
  			      total_weight3, tt_sum3, t_ur3, t_ug3, t_ub3);
				break;
			case 2 :
		    Sum (pRect->redMin, pRect->greenMin, i, pRect->redMax, pRect->greenMax, i,
  			      total_weight3, tt_sum3, t_ur3, t_ug3, t_ub3);
				break;
			}

    ASSERT (total_weight3 <= pRect->m_iTotalWeight);

    /* update left and right box's statistics */

    total_weight1 += total_weight3;
    tt_sum1       += tt_sum3;
    t_ur1         += t_ur3;
    t_ug1         += t_ug3;
    t_ub1         += t_ub3;

    total_weight2 -= total_weight3;
    tt_sum2       -= tt_sum3;
    t_ur2         -= t_ur3;
    t_ug2         -= t_ug3;
    t_ub2         -= t_ub3;

    ASSERT ((total_weight1 + total_weight2) == pRect->m_iTotalWeight);

    /* calculate left and right box's overall variance */

    __int64 total_variance = Variance(total_weight1, tt_sum1, t_ur1, t_ug1, t_ub1) +
                     Variance(total_weight2, tt_sum2, t_ur2, t_ug2, t_ub2);

    /* found better split point? if so, remember it */

    if (total_variance < iVariance)
    {
      iVariance = total_variance;
      iIndex = i;
    }
  }

#ifdef _DEBUG
	  __int64 total_weight3;
  	__int64 tt_sum3, t_ur3, t_ug3, t_ub3;
		switch (iDimen)
		  {
			case 0 :
		    Sum (i, pRect->greenMin, pRect->blueMin, i, pRect->greenMax, pRect->blueMax,
  			      total_weight3, tt_sum3, t_ur3, t_ug3, t_ub3);
				break;
			case 1 :
		    Sum (pRect->redMin, i, pRect->blueMin, pRect->redMax, i, pRect->blueMax,
  			      total_weight3, tt_sum3, t_ur3, t_ug3, t_ub3);
				break;
			case 2 :
		    Sum (pRect->redMin, pRect->greenMin, i, pRect->redMax, pRect->greenMax, i,
  			      total_weight3, tt_sum3, t_ur3, t_ug3, t_ub3);
				break;
			}

    total_weight2 -= total_weight3;
    tt_sum2       -= tt_sum3;
    t_ur2         -= t_ur3;
    t_ug2         -= t_ug3;
    t_ub2         -= t_ub3;

 ASSERT ( total_weight2 == 0 );
 ASSERT ( tt_sum2 == 0 );
 ASSERT ( t_ur2 == 0 );
 ASSERT ( t_ug2 == 0 );
 ASSERT ( t_ub2 == 0 );
#endif
}

void VarSplitQuant::SplitColorRect( int oldRect, int newRect )
{
	//  Get pointers to the new and old rects for convenience.
	ColorRect *pOldRect = &pColorRects[ oldRect ];
	ColorRect *pNewRect = &pColorRects[ newRect ];

	// where do we split this one?
	int redIndex, greenIndex, blueIndex;
	__int64 redVar, greenVar, blueVar;
	GetMinVariance ( pOldRect, 0, redIndex, redVar );
	GetMinVariance ( pOldRect, 1, greenIndex, greenVar );
	GetMinVariance ( pOldRect, 2, blueIndex, blueVar );

	int iIndex;
	if ( (blueVar < redVar) && (blueVar < greenVar) )
		iIndex = 2;
	else
		if ( redVar < greenVar )
			iIndex = 0;
		else
			iIndex = 1;

	// dup it into the new one
	* pNewRect = * pOldRect;

	//  Split the box along the longest axis.
	switch ( iIndex )
	  {
		case 0 :
			pOldRect->redMax = redIndex;
			pNewRect->redMin = redIndex + 4;
			break;
		case 1 :
			pOldRect->greenMax = greenIndex;
			pNewRect->greenMin = greenIndex + 4;
			break;
		case 2 :
			pOldRect->blueMax = blueIndex;
			pNewRect->blueMin = blueIndex + 4;
			break;
	  }
	
	//  Set up the new color rect based on the old one.
	AdjustRect( newRect );
	AdjustRect( oldRect );
}

RGBQUAD VarSplitQuant::GetRGBColor( int rectIndex )
{
	ColorRect *pRect = &pColorRects[ rectIndex ];

	RGBQUAD	col;
	col.rgbRed 		= ( byte ) (pRect->redMax == 255 ? 255 : pRect->redMin + ( pRect->redMax - pRect->redMin ) / 2);
	col.rgbGreen	= ( byte ) (pRect->greenMax == 255 ? 255 : pRect->greenMin + ( pRect->greenMax - pRect->greenMin ) / 2);
	col.rgbBlue		= ( byte ) (pRect->blueMax == 255 ? 255 : pRect->blueMin + ( pRect->blueMax - pRect->blueMin ) / 2);

	return col;
}

VarSplitQuant::VarSplitQuant( void )
{
//bugbug	pReservedColors = NULL;
	nReservedColorCount = 0;
	pHistogram = NULL;
	colorsUsed = 0;
	pColorList = NULL;
	pColorRects = NULL;
}

VarSplitQuant::~VarSplitQuant( void )
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

bool VarSplitQuant::Init( const RGBQUAD *pPalette, int palSize )
{
	//  Allocate space for the color histogram
	pHistogram = new word[ HISTSIZE + 2 ];
	if ( pHistogram == NULL )
		return false;

	//  Clear the color histogram
	memset( pHistogram, 0, HISTSIZE * sizeof( word ) );

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

bool VarSplitQuant::AddImage( const Image *pImage )
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

bool VarSplitQuant::CreatePalette( RGBQUAD *pPalette, int *pPalSize )
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
		__int64 maxVar = 0;
		int maxIndex = -1;
		int rectIndex;
		for( rectIndex = 0; rectIndex < rectsUsed; rectIndex++ )
			{
			if ( pColorRects[ rectIndex ].m_iVariance > maxVar )
				{
				maxVar = pColorRects[ rectIndex ].m_iVariance;
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
