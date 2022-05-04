#include "stdafx.h"

#include "stucki.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//  Turn this on if you're suspicious about the colormatching.
//  This will do the color matching the fast way and the slow
//  way and verify that the same results are achieved both ways.
//#define TEST_COLORMATCH

//  Function to compare two colors for sorting.  Returns < 0 if color 0 is less than
//  color 1, > 0 if color 0 is greater than color 1, and 0 if the two colors are equal.
//  NOTE :  It would be nice to make this a member function, but that seems to cause problems
//  in sorting for some reason.
static int __cdecl CompareColors( const void *p0, const void *p1 )
{
	sortedColor *pColor0 = ( sortedColor * )p0;
	sortedColor *pColor1 = ( sortedColor * )p1;

	if ( pColor0->color.Red() != pColor1->color.Red() )
		return pColor0->color.Red() - pColor1->color.Red();
	else
		return pColor0->originalIndex - pColor1->originalIndex;
}

void StuckiDither::CreateSortedPalette( const RGBColor *pPalette, int oldPalSize )
{
	assert( pPalette != NULL );
	assert( 0 < oldPalSize && oldPalSize <= 256 );

	palSize = oldPalSize;

	//  Copy the original palette into the space 
	//  allocated for the sorted palette.  Set the 
	//  original index field for each color.
	int palIndex;
	for( palIndex = 0; palIndex < palSize; palIndex++ )
	{
		aSortedPalette[ palIndex ].color = pPalette[ palIndex ];
		aSortedPalette[ palIndex ].originalIndex = ( byte )palIndex;
	}

	//  Sort the palette on red.
	qsort( &aSortedPalette[ 0 ], palSize, sizeof( sortedColor ), CompareColors );

	//  Initialize a lookup table which contains the 
	//  index of the closest red to the red at the 
	//  index.  ( ie, for any given level of red 
	//  [0,256), the lookup returns the index of the 
	//  color which has the red level closest to the
	//  given level )
	int minPalIndex = 0;
	int colIndex;
	for( colIndex = 0; colIndex < 256; colIndex++ )
	{
		int minDeltaRSquared = ( aSortedPalette[ minPalIndex ].color.Red() - colIndex ) *
			( aSortedPalette[ minPalIndex ].color.Red() - colIndex );

		palIndex = minPalIndex + 1;
		while( palIndex < palSize )
		{
			int deltaRSquared = ( aSortedPalette[ palIndex ].color.Red() - colIndex ) *
				( aSortedPalette[ palIndex ].color.Red() - colIndex );
			if ( deltaRSquared > minDeltaRSquared )
				break;
			else if ( deltaRSquared < minDeltaRSquared )
			{
				minDeltaRSquared = deltaRSquared;
				minPalIndex = palIndex;
				if ( minDeltaRSquared == 0 )
					break;
			}

			palIndex++;
		}

		redLookup[ colIndex ] = ( byte )minPalIndex;
	}
}

int StuckiDither::FindClosestColor( int red, int green, int blue )
{
#ifdef TEST_COLORMATCH
	//  Find the closest red color.
	int minDeltaRSquared = 256 * 256;
	int testMinColIndex = palSize;
	int colIndex;
	for( colIndex = 0; colIndex < palSize; colIndex ++ )
	{
		int deltaRSquared = ( aSortedPalette[ colIndex ].color.Red() - red ) *
			( aSortedPalette[ colIndex ].color.Red() - red );
		if ( deltaRSquared < minDeltaRSquared )
		{
			minDeltaRSquared = deltaRSquared;
			testMinColIndex = colIndex;
		}
		else if ( deltaRSquared > minDeltaRSquared )
			break;	//  Since R is monotonically increasing,  we're done.
	}

	assert( testMinColIndex == redLookup[ red ] );
#endif

	int minColIndex = redLookup[ red ];

	int minDeltaRGBSquared = ( aSortedPalette[ minColIndex ].color.Red() - red ) * 
		( aSortedPalette[ minColIndex ].color.Red() - red ) + 
		( aSortedPalette[ minColIndex ].color.Green() - green ) * 
		( aSortedPalette[ minColIndex ].color.Green() - green ) + 
		( aSortedPalette[ minColIndex ].color.Blue() - blue ) * 
		( aSortedPalette[ minColIndex ].color.Blue() - blue );

	//  Can't get much closer than this.
	if ( minDeltaRGBSquared == 0 )
		return aSortedPalette[ minColIndex ].originalIndex;

	int prevIndex = minColIndex, nextIndex = minColIndex;
	int prevDeltaRSquared = 0, nextDeltaRSquared = 0;
	int deltaRGBSquared = 0;
	bool bPrevDone = false, bNextDone = false;

	//  Test early out on previous index.
	if ( prevIndex <= 0 )
		bPrevDone = true;
		
	//  Test early out on next index.
	if ( palSize - 1 <= nextIndex )
		bNextDone = true;

	while( bPrevDone == false || bNextDone == false )
	{
		if ( bPrevDone == false )
		{
			prevIndex--;
			prevDeltaRSquared = ( aSortedPalette[ prevIndex ].color.Red() - red ) *
				( aSortedPalette[ prevIndex ].color.Red() - red );
			deltaRGBSquared = prevDeltaRSquared + 
				( aSortedPalette[ prevIndex ].color.Green() - green ) * 
				( aSortedPalette[ prevIndex ].color.Green() - green ) + 
				( aSortedPalette[ prevIndex ].color.Blue() - blue ) * 
				( aSortedPalette[ prevIndex ].color.Blue() - blue );
			if ( deltaRGBSquared <= minDeltaRGBSquared )	//  This = is important to
			{												//  make sure we get the same results we do with the other test.
				minDeltaRGBSquared = deltaRGBSquared;
				minColIndex = prevIndex;
				if ( minDeltaRGBSquared == 0 )
					break;
			}

			if ( prevIndex <= 0 || prevDeltaRSquared > minDeltaRGBSquared )
				bPrevDone = true;
		}

		if ( bNextDone == false )
		{
			nextIndex++;
			nextDeltaRSquared = ( aSortedPalette[ nextIndex ].color.Red() - red ) *
				( aSortedPalette[ nextIndex ].color.Red() - red );
			deltaRGBSquared = nextDeltaRSquared + 
				( aSortedPalette[ nextIndex ].color.Green() - green ) * 
				( aSortedPalette[ nextIndex ].color.Green() - green ) + 
				( aSortedPalette[ nextIndex ].color.Blue() - blue ) * 
				( aSortedPalette[ nextIndex ].color.Blue() - blue );
			if ( deltaRGBSquared < minDeltaRGBSquared )
			{
				minDeltaRGBSquared = deltaRGBSquared;
				minColIndex = nextIndex;
				if ( minDeltaRGBSquared == 0 )
					break;
			}

			if ( palSize - 1 <= nextIndex || nextDeltaRSquared > minDeltaRGBSquared )
				bNextDone = true;
		}
	}

#ifdef TEST_COLORMATCH
 
	//  Find the closest color using the slow method which does not
	//  take advantage of the sorted palette.
	int minDeltaSquared = 256 * 256 * 3;
	testMinColIndex = palSize;

	colIndex;
	for( colIndex = 0; colIndex < palSize; colIndex++ )
	{
		int deltaSquared = ( aSortedPalette[ colIndex ].color.Red()- red ) * 
			( aSortedPalette[ colIndex ].color.Red() - red ) + 
			( aSortedPalette[ colIndex ].color.Green() - green ) * 
			( aSortedPalette[ colIndex ].color.Green() - green ) + 
			( aSortedPalette[ colIndex ].color.Blue() - blue ) * 
			( aSortedPalette[ colIndex ].color.Blue() - blue );
		if ( deltaSquared < minDeltaSquared )
		{
			minDeltaSquared = deltaSquared;
			testMinColIndex = colIndex;
			if ( minDeltaSquared == 0 )
				break;
		}
	}

	assert( minColIndex == testMinColIndex );
#endif

	assert( minColIndex < palSize );

	return aSortedPalette[ minColIndex ].originalIndex;
}

/*
 *  Initializes the ditherer.
 */
bool StuckiDither::Init( const RGBColor *pPalette, int palSize )
{
	assert( pPalette != NULL );
	assert( 0 < palSize && palSize <= 256 );

	//  Make a copy of the original palette.
	( void )memcpy( aOriginalPalette, pPalette, palSize * sizeof( RGBColor ) );

	//  Create the sorted palette.
	CreateSortedPalette( pPalette, palSize );
	return true;
}

/*
 *  Dithers a 24-bit bitmap to an 8-bit bitmap.
 */
Image *StuckiDither::DitherImage( const Image *pOriginalImage )
{
	//  Make sure the incoming image is the proper format.
	assert( pOriginalImage->_bitDepth == 24 );

	//  We use these a lot, so let's keep them handy.
	int iWidth = pOriginalImage->_width;
	int iHeight = pOriginalImage->_height;

	//  Create a new image to contain the dithered image.
	Image *pNewImage = new Image( iWidth, iHeight, 8, aOriginalPalette );
	if ( pNewImage == NULL )
		return NULL;

	pNewImage->CopyHotSpots( pOriginalImage );	// GG

	pNewImage->SetSkip( RGBColor( 255, 0, 255 ) );
	pNewImage->_width = pOriginalImage->_width;
	pNewImage->_height = pOriginalImage->_height;
	pNewImage->_xoff = pOriginalImage->_xoff;
	pNewImage->_yoff = pOriginalImage->_yoff;
			
	//  Allocate the temp buffers which will be used to hold the error.
	//  Note that they are ints, not bytes, and that I allocate four
	//  more than the width of the bitmap.
	int *pErrorBuf[ 3 ];
	pErrorBuf[ 0 ] = new int[ 3 * ( iWidth + 4 ) ];
	pErrorBuf[ 1 ] = new int[ 3 * ( iWidth + 4 ) ];
	pErrorBuf[ 2 ] = new int[ 3 * ( iWidth + 4 ) ];
	if ( pErrorBuf[ 0 ] == NULL || pErrorBuf[ 1 ] == NULL ||
		pErrorBuf[ 2 ] == NULL )
	{
		delete[] pErrorBuf[ 0 ];
		delete[] pErrorBuf[ 1 ];
		delete[] pErrorBuf[ 2 ];
		delete pNewImage;
		return NULL;
	}

	//  Clear all three error buffers initially.
	( void )memset( pErrorBuf[ 0 ], 0,  sizeof( int ) * 3 * ( iWidth + 4 ) );
	( void )memset( pErrorBuf[ 1 ], 0,  sizeof( int ) * 3 * ( iWidth + 4 ) );
	( void )memset( pErrorBuf[ 2 ], 0,  sizeof( int ) * 3 * ( iWidth + 4 ) );

	//  Start by using the 0th error row as the current row.
	int thisErrorRowIndex = 0;
	int nextErrorRowIndex = 1;
	int secondErrorRowIndex = 2;

	//  Start by going left-to-right.
	int direction = 0;

	int sourceRowWidthBytes = iWidth * 3;
	int destRowWidthBytes = iWidth;

	int row;
	for( row = 0; row < iHeight; row++ )
	{
		//  Get pointers to the current row in both the destination DIB
		//  and the source DIB.
		byte *pDestRow = pNewImage->_pPixels + row * destRowWidthBytes;
		byte *pSourceRow = pOriginalImage->_pPixels + row * sourceRowWidthBytes;

		//  Set up the pointers to the error buffers.
		int *pThisErrorRow = pErrorBuf[ thisErrorRowIndex ] + 6;
		int *pNextErrorRow = pErrorBuf[ nextErrorRowIndex ] + 6;
		int *pSecondErrorRow = pErrorBuf[ secondErrorRowIndex ] + 6;

		//  Set up the start/end/increment values based on the current
		//  direction.
		int start, end, inc;
		if ( direction == 0 )
		{
			start = 0;
			end = iWidth;
			inc = 1;
		}
		else
		{
			start = iWidth - 1;
			end = -1;
			inc = -1;
		}

		//  Clear the second error row.  We could avoid this clear
		//  by getting tricky with setting vs adding of errors on this
		//  row, but this simplifies life without too much of a speed hit.
		//  This won't clear the extra pixels to either side, but we don't
		//  really care about those.
		( void )memset( pSecondErrorRow, 0, sizeof( int ) * iWidth * 3 );

		int col;
		for( col = start; col != end; col += inc )
		{
			byte thisRed, thisGreen, thisBlue;

#if 1
			//  If the original pixel is transparent, _don't_ propogate 
			//  error into it - that can cause a slide just far enough 
			//  to give ugly magenta pixels.
			if ( pOriginalImage->IsTransparent( pSourceRow + col * 3 ) )
			{
				thisRed = 255;
				thisGreen = 0;
				thisBlue = 255;
			}
			else
#endif
			{
				//  Find the new RGB for this pixel taking into account
				//  the diffused error.
				thisRed = ( byte )CLAMP( 0, pSourceRow[ col * 3 + Image::RED_OFFSET ] + pThisErrorRow[ col * 3 + Image::RED_OFFSET ], 255 );
				thisGreen = ( byte )CLAMP( 0, pSourceRow[ col * 3 + Image::GREEN_OFFSET ] + pThisErrorRow[ col * 3 + Image::GREEN_OFFSET ], 255 );
				thisBlue = ( byte )CLAMP( 0, pSourceRow[ col * 3 + Image::BLUE_OFFSET ] + pThisErrorRow[ col * 3 + Image::BLUE_OFFSET ], 255 );
			}

			//  Find the closest color in our palette to this color.
			byte newIndex = ( byte )FindClosestColor( thisRed, thisGreen, thisBlue );
			pDestRow[ col ] = newIndex;
			
			//  Compute the error.  Note that the index returned from FindClosestColor
			//  is the index in the original palette, so we use that to find our
			//  actual R, G, and B.
			int redError = pSourceRow[ col * 3 + Image::RED_OFFSET ] - aOriginalPalette[ newIndex ].Red();
			int greenError = pSourceRow[ col * 3 + Image::GREEN_OFFSET ] - aOriginalPalette[ newIndex ].Green();
			int blueError = pSourceRow[ col * 3 + Image::BLUE_OFFSET ] - aOriginalPalette[ newIndex ].Blue();

			//  Diffuse the red component of the error.
			//  Note that since both the next and second error
			//  lines are symmetric, only the first line needs
			//  to use inc to determine which pixels to add the
			//  error to.
			int err1_42 = redError / 42;
			int err2_42 = (2 * redError) / 42;
			int err4_42 = (4 * redError) / 42;
			int err8_42 = (8 * redError) / 42;
			pThisErrorRow[ ( col + inc ) * 3 + Image::RED_OFFSET ] += err8_42;
			pThisErrorRow[ ( col + inc * 2 ) * 3 + Image::RED_OFFSET ] += err4_42;
			pNextErrorRow[ ( col - 2 ) * 3 + Image::RED_OFFSET ] += err2_42;
			pNextErrorRow[ ( col - 1 ) * 3 + Image::RED_OFFSET ] += err4_42;
			pNextErrorRow[ col * 3 + Image::RED_OFFSET ] += err8_42;
			pNextErrorRow[ ( col + 1 ) * 3 + Image::RED_OFFSET ] += err4_42;
			pNextErrorRow[ ( col + 2 ) * 3 + Image::RED_OFFSET ] += err2_42;
			pSecondErrorRow[ ( col - 2 ) * 3 + Image::RED_OFFSET ] += err1_42;
			pSecondErrorRow[ ( col - 1 ) * 3 + Image::RED_OFFSET ] += err2_42;
			pSecondErrorRow[ col * 3 + Image::RED_OFFSET ] += err4_42;
			pSecondErrorRow[ ( col + 1 ) * 3 + Image::RED_OFFSET ] += err2_42;
			pSecondErrorRow[ ( col + 2 ) * 3 + Image::RED_OFFSET ] += err1_42;

			//  Diffuse the green component of the error.
			err1_42 = greenError / 42;
			err2_42 = (2 * greenError) / 42;
			err4_42 = (4 * greenError) / 42;
			err8_42 = (8 * greenError) / 42;
			pThisErrorRow[ ( col + inc ) * 3 + Image::GREEN_OFFSET ] += err8_42;
			pThisErrorRow[ ( col + inc * 2 ) * 3 + Image::GREEN_OFFSET ] += err4_42;
			pNextErrorRow[ ( col - 2 ) * 3 + Image::GREEN_OFFSET ] += err2_42;
			pNextErrorRow[ ( col - 1 ) * 3 + Image::GREEN_OFFSET ] += err4_42;
			pNextErrorRow[ col * 3 + Image::GREEN_OFFSET ] += err8_42;
			pNextErrorRow[ ( col + 1 ) * 3 + Image::GREEN_OFFSET ] += err4_42;
			pNextErrorRow[ ( col + 2 ) * 3 + Image::GREEN_OFFSET ] += err2_42;
			pSecondErrorRow[ ( col - 2 ) * 3 + Image::GREEN_OFFSET ] += err1_42;
			pSecondErrorRow[ ( col - 1 ) * 3 + Image::GREEN_OFFSET ] += err2_42;
			pSecondErrorRow[ col * 3 + Image::GREEN_OFFSET ] += err4_42;
			pSecondErrorRow[ ( col + 1 ) * 3 + Image::GREEN_OFFSET ] += err2_42;
			pSecondErrorRow[ ( col + 2 ) * 3 + Image::GREEN_OFFSET ] += err1_42;

			//  Diffuse the blue component of the error.
			err1_42 = blueError / 42;
			err2_42 = (2 * blueError) / 42;
			err4_42 = (4 * blueError) / 42;
			err8_42 = (8 * blueError) / 42;
			pThisErrorRow[ ( col + inc ) * 3 + Image::BLUE_OFFSET ] += err8_42;
			pThisErrorRow[ ( col + inc * 2 ) * 3 + Image::BLUE_OFFSET ] += err4_42;
			pNextErrorRow[ ( col - 2 ) * 3 + Image::BLUE_OFFSET ] += err2_42;
			pNextErrorRow[ ( col - 1 ) * 3 + Image::BLUE_OFFSET ] += err4_42;
			pNextErrorRow[ col * 3 + Image::BLUE_OFFSET ] += err8_42;
			pNextErrorRow[ ( col + 1 ) * 3 + Image::BLUE_OFFSET ] += err4_42;
			pNextErrorRow[ ( col + 2 ) * 3 + Image::BLUE_OFFSET ] += err2_42;
			pSecondErrorRow[ ( col - 2 ) * 3 + Image::BLUE_OFFSET ] += err1_42;
			pSecondErrorRow[ ( col - 1 ) * 3 + Image::BLUE_OFFSET ] += err2_42;
			pSecondErrorRow[ col * 3 + Image::BLUE_OFFSET ] += err4_42;
			pSecondErrorRow[ ( col + 1 ) * 3 + Image::BLUE_OFFSET ] += err2_42;
			pSecondErrorRow[ ( col + 2 ) * 3 + Image::BLUE_OFFSET ] += err1_42;
		}

		//  Go in the opposite direction next time.
		direction = 1 - direction;

		//  Move the error buffer indices.
		thisErrorRowIndex = nextErrorRowIndex;
		nextErrorRowIndex = secondErrorRowIndex;
		secondErrorRowIndex++;
		if ( secondErrorRowIndex >= 3 )
			secondErrorRowIndex = 0;
	}

	//  Clean up the temp storage which was allocated.
	delete[] pErrorBuf[ 0 ];
	delete[] pErrorBuf[ 1 ];
	delete[] pErrorBuf[ 2 ];

	//  Return the pointer to the dithered image
	return pNewImage;
}
