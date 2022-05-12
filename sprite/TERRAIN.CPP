#include "stdafx.h"

#include "bmpfile.h"
#include "tgafile.h"
#include "sprite.h"
#include "terrain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CSpriteApp theApp;

/*****************************************************
 *
 *  Terrain Image
 *
 *****************************************************/

bool TerrainImage::Shade()
{
	//  This operation is only implemented on 24-bit images.
	ASSERT( _bitDepth == 24 );

	//  Allocate new storage for pixel buffer large enough to hold all eight
	//  shaded tiles.
	byte *pNewBits = new byte[ _width * _height * 8 * 3 ];
	if ( pNewBits == NULL )
		return false;

	//  Create the eight shaded tiles.
	byte *pSrc = _pPixels;
	int pixIndex;
	for( pixIndex = 0; pixIndex < _width * _height; pixIndex++ )
	{
		float h, l, s;

		//  Convert this pixel to HLS.
		(( RGBColor * )pSrc)->ToHLS( &h, &l, &s );

		//  Shade the pixel to the eight levels and save each shaded
		//  pixel in the appropriate place.
		int shade;
		for( shade = 0; shade < 8; shade++ )
		{
			float shadedL = CLAMP( 0.0f, l * ( 1.0f + 0.1f * ( shade - 4 ) ), 1.0f );
			RGBColor	res;
			res.FromHLS( h, shadedL, s );

			byte *pDest = pNewBits + ( shade * _width * _height + pixIndex ) * 3;
			pDest[ RED_OFFSET ] = res.Red();
			pDest[ GREEN_OFFSET ] = res.Green();
			pDest[ BLUE_OFFSET ] = res.Blue();
		}

		pSrc += 3;
	}

	//  Adjust the bitmap to reflect the new size and point to
	//  the new shaded tiles.
	_height = _height * 8;
	delete[] _pPixels;
	_pPixels = pNewBits;

	return true;
}

//  "Square" the tile by copying each of the four 
//  triangles in the diamond to the opposite corner,
//  as though we were tiling another tile of the
//  same type in the corner.  This is done before
//  scaling to prevent averaging in the magenta pixels
//  to the rest of the image.
BOOL TerrainImage::Square(char const * pFile)
{
	//  This operation is only implemented on 24-bit images.
	ASSERT( _bitDepth == 24 );
	
	int width, row;

	//  First make sure that the entire tile is filled where it's supposed to be.
	//  Set the initial value to the color of the center pixel in the
	//  tile.  If this happens to be transparent, well, the tile has more problems
	//  than this was designed to fix.
	//  Undefine check_tiles if all of the tile art is known to match the specifications.
#ifdef CHECK_TILES
	int pass = 0;
recheck_tile :;
	byte prevR = _pPixels[ ( _height / 2 * _width + _width / 2 ) * 3 + 0 ];
	byte prevG = _pPixels[ ( _height / 2 * _width + _width / 2 ) * 3 + 1 ];
	byte prevB = _pPixels[ ( _height / 2 * _width + _width / 2 ) * 3 + 2 ];
	int badPixelCnt = 0;
	int start = _width / 2 - 2;
	width = 4;
	int startInc = -2;
	int widthInc = 4;
	row = 0;
	while( row < _height )
	{
		ASSERT( 0 <= start && start <= _width / 2  );
		ASSERT( 4 <= width && width <= _width );

		byte *pPix = &_pPixels[ ( row * _width + start ) * 3 ];
		int col;
		for( col = start; col < start + width; col++, pPix += 3 )
		{
			if ( IsTransparent( pPix ) ||
//  TODO :  Remove this kludge, which is in for temp art.
				( pPix[ 0 ] == 254 && pPix[ 1 ] == 0 &&	pPix[ 2 ] == 254 ) )
			{
				badPixelCnt++;
				pPix[ 0 ] = prevR;
				pPix[ 1 ] = prevG;
				pPix[ 2 ] = prevB;
			}
			else
			{
				prevR = pPix[ 0 ];
				prevG = pPix[ 1 ];
				prevB = pPix[ 2 ];
			}
		}

		row++;
		width += widthInc;
		start += startInc;
		if ( start < 0 )
		{
			startInc = -startInc;
			widthInc = -widthInc;
			start += startInc;
			width += widthInc;
		}
	}

	//  Either this is the first pass or all bad pixels should have been fixed by now.
	// GG: Fires all the time	ASSERT( pass == 0 || badPixelCnt == 0 );
	pass++;
	if ( badPixelCnt > 0 )
	{
		CString str;
		str.Format( "%s - tile contains %d magenta pixels inside the diamond!", pFile, badPixelCnt );
		theApp.AddErrorDialog(str);
		if (pass > 2)
			return (FALSE);
		goto recheck_tile;
	}
#endif

	//  Copy the pixels into the triangle pattern described by Geoff before scaling.
	for( width = _width / 2 - 2, row = 0; row < _height / 2 - 1; row++, width -= 2 )
	{
		memcpy( &_pPixels[ ( row * _width ) * 3 ],
			&_pPixels[ ( ( row + _height / 2 + 1 ) * _width + _width / 2 ) * 3 ],
			width * 3 );

		memcpy( &_pPixels[ ( row * _width + _width - width ) * 3 ],
			&_pPixels[ ( ( row + _height / 2 + 1 ) * _width + _width / 2 - width ) * 3 ],
			width * 3 );

		memcpy( &_pPixels[ ( ( row + _height / 2 + 1 ) * _width ) * 3 ],
			&_pPixels[ ( row * _width + _width / 2 ) * 3 ],
			( _width / 2 - width ) * 3 );

		memcpy( &_pPixels[ ( ( row + _height / 2 + 1 ) * _width + _width / 2 + width ) * 3 ],
			&_pPixels[ ( row * _width + width ) * 3 ],
			( _width / 2  - width ) * 3 );
	}

	return (TRUE);
}


void TerrainImage::UnSquare()
{
	//  This operation is only implemented on 24-bit images.
	ASSERT( _bitDepth == 24 );

	int rowIndex, shadeIndex;
	for( shadeIndex = 0; shadeIndex < 8; shadeIndex++ )
	{
		int width = _width / 2 - 2;
		int widthInc = -2;
		for( rowIndex = 0; rowIndex < _height / 8; rowIndex++ )
		{
			byte *pLeft = _pPixels + ( shadeIndex * _height / 8 * _width + rowIndex * _width ) * 3;
			byte *pRight = pLeft + ( _width - width ) * 3;
			int colIndex;
			for( colIndex = 0; colIndex < width; colIndex++ )
			{
				*pLeft++ = 255;
				*pLeft++ = 0;
				*pLeft++ = 255;
				*pRight++ = 255;
				*pRight++ = 0;
				*pRight++ = 255;
			}

			width += widthInc;
			if ( width < 0 )
			{
				widthInc = -widthInc;
				width += widthInc;
			}
		}
	}
}

bool TerrainImage::Pack()
{
	//  The old and new heights are the same, at 1 / 8 the bitmap height.
	//  The bitmap is assumed to contain all eight shaded levels of the sprite.
	int height = _height / 8;

	//  The old width is the width of the sprite.
	int oldWidth = _width;

	//  The new width is the height of the sprite plus two.
	int newWidth = height + 2;

	//  Allocate a chunk of memory for the ( n + 2 ) x n packed sprite.
	byte *pNewBits = new byte[ newWidth * height * 8 * _bytesPerPixel ];
	if ( pNewBits == NULL )
		return false;

	//  For each of the eight shade levels, create a packed tile.
	int shade;
	for( shade = 0; shade < 8; shade++ )
	{

		int start, end;

		start = height / 2 - 1;
		end = height / 2 + 1;

		int row, col;
		for( col = 0; col < oldWidth / 2; col += 2, start--, end++ )
		{
			ASSERT( 0 <= start );
			ASSERT( end <= height );

			byte *pDest = pNewBits + ( height * shade  + col ) * newWidth * _bytesPerPixel;

			for( row = start; row < end; row++ )
			{
				byte *pSrc = _pPixels + ( ( height * shade + row ) * oldWidth + col ) * _bytesPerPixel;
				switch( _bytesPerPixel )
				{
					default :
						assert( false );
					case 4 :
						*pDest++ = *pSrc++;
					case 3 :
						*pDest++ = *pSrc++;
					case 2 :
						*pDest++ = *pSrc++;
					case 1 :
						*pDest++ = *pSrc++;
				}
			}

			pDest = pNewBits + ( height * shade + ( col + 1 ) ) * newWidth * _bytesPerPixel;

			for( row = start; row < end; row++ )
			{
				byte *pSrc = _pPixels + ( ( height * shade + row ) * oldWidth + ( col + 1 ) ) * _bytesPerPixel;
				switch( _bytesPerPixel )
				{
					default :
						assert( false );
					case 4 :
						*pDest++ = *pSrc++;
					case 3 :
						*pDest++ = *pSrc++;
					case 2 :
						*pDest++ = *pSrc++;
					case 1 :
						*pDest++ = *pSrc++;
				}
			}
		}

		start = 0;
		end = height;

		for( col = oldWidth / 2; col < oldWidth; col += 2, start++, end-- )
		{
			ASSERT( start <= height / 2 - 1 );
			ASSERT( height / 2 + 1 <= end );

			byte *pDest = pNewBits + ( ( height * shade + ( col - oldWidth / 2 ) ) * newWidth +
				( newWidth - ( end - start ) ) ) * _bytesPerPixel;

			for( row = start; row < end; row++ )
			{
				byte *pSrc = _pPixels + ( ( height * shade + row ) * oldWidth + col ) * _bytesPerPixel;
				switch( _bytesPerPixel )
				{
					default :
						assert( false );
					case 4 :
						*pDest++ = *pSrc++;
					case 3 :
						*pDest++ = *pSrc++;
					case 2 :
						*pDest++ = *pSrc++;
					case 1 :
						*pDest++ = *pSrc++;
				}
			}

			pDest = pNewBits + ( ( height * shade + ( ( col + 1 ) - oldWidth / 2 ) ) * newWidth +
				( newWidth - ( end - start ) ) ) * _bytesPerPixel;

			for( row = start; row < end; row++ )
			{
				byte *pSrc = _pPixels + ( ( height * shade + row ) * oldWidth + ( col + 1 ) ) * _bytesPerPixel;
				switch( _bytesPerPixel )
				{
					default :
						assert( false );
					case 4 :
						*pDest++ = *pSrc++;
					case 3 :
						*pDest++ = *pSrc++;
					case 2 :
						*pDest++ = *pSrc++;
					case 1 :
						*pDest++ = *pSrc++;
				}
			}
		}
	}

	//  Fix up the bitmap.
	_width = newWidth;
	delete[] _pPixels;
	_pPixels = pNewBits;

	return true;
}

bool TerrainImage::UnPack()
{
	//  We'll want the height of a single bitmap fairly often.
	int	height = _height / 8;
	int oldWidth = height + 2;
	int newWidth = height * 2;
	int oldTileSize = height * oldWidth;
	int newTileSize = height * _width;

	byte *pNewPixels = new byte[ newTileSize * 8 * _bytesPerPixel ];
	if ( pNewPixels == NULL )
		return false;

	int shade;
	for( shade = 0; shade < 8; shade++ )
	{
		int start = height / 2 - 1;
		int end = height / 2 + 1;

		int row, col;
		for( row = 0; row < height; row += 2, start--, end++ )
		{
			ASSERT( 0 <= start );
			ASSERT( end <= height );
		
			const byte *pSrc = _pPixels + ( oldTileSize * shade + row * oldWidth ) * _bytesPerPixel;

			for( col = start; col < end; col++ )
			{
				byte *pDest = pNewPixels + ( newTileSize * shade + col * newWidth + row ) * _bytesPerPixel;
				switch( _bytesPerPixel )
				{
					default :
						assert( false );
					case 4 :
						*pDest++ = *pSrc++;
					case 3 :
						*pDest++ = *pSrc++;
					case 2 :
						*pDest++ = *pSrc++;
					case 1 :
						*pDest++ = *pSrc++;
				}
			}
			
			pSrc = _pPixels + ( oldTileSize * shade + ( row + 1 ) * oldWidth ) * _bytesPerPixel;

			for( col = start; col < end; col++ )
			{
				byte *pDest = pNewPixels + ( newTileSize * shade + col * newWidth + ( row + 1 ) ) * _bytesPerPixel;
				switch( _bytesPerPixel )
				{
					default :
						assert( false );
					case 4 :
						*pDest++ = *pSrc++;
					case 3 :
						*pDest++ = *pSrc++;
					case 2 :
						*pDest++ = *pSrc++;
					case 1 :
						*pDest++ = *pSrc++;
				}
			}
		}

		start = 0;
		end = height;

		for( row = 0; row < height; row += 2, start++, end-- )
		{
			ASSERT( start <= height / 2 - 1 );
			ASSERT( height / 2 + 1 <= end );

			const byte *pSrc = _pPixels + ( oldTileSize * shade + row * oldWidth + ( oldWidth - ( end - start ) ) ) * _bytesPerPixel;

			for( col = start; col < end; col++ )
			{
				byte *pDest = pNewPixels + ( newTileSize * shade + col * newWidth +
					( row + newWidth / 2 ) ) * _bytesPerPixel;
				switch( _bytesPerPixel )
				{
					default :
						assert( false );
					case 4 :
						*pDest++ = *pSrc++;
					case 3 :
						*pDest++ = *pSrc++;
					case 2 :
						*pDest++ = *pSrc++;
					case 1 :
						*pDest++ = *pSrc++;
				}
			}
			
			pSrc = _pPixels + ( oldTileSize * shade + ( row + 1 ) * oldWidth + ( oldWidth - ( end - start ) ) ) * _bytesPerPixel;

			for( col = start; col < end; col++ )
			{
				byte *pDest = pNewPixels + ( newTileSize * shade + col * newWidth +
					( row + 1 + newWidth / 2 ) ) * _bytesPerPixel;
				switch( _bytesPerPixel )
				{
					default :
						assert( false );
					case 4 :
						*pDest++ = *pSrc++;
					case 3 :
						*pDest++ = *pSrc++;
					case 2 :
						*pDest++ = *pSrc++;
					case 1 :
						*pDest++ = *pSrc++;
				}
			}
		}
	}

	//  Fix up the image
	_width = newWidth;
	//  _height stays the same.
	delete[] _pPixels;
	_pPixels = pNewPixels;

	return true;
}

/*****************************************************
 *
 *  Terrain View
 *
 *****************************************************/

BOOL TerrainView::LoadFile( const char *pFullPath, int index, int type )
{
	TerrainImage *pImage = new TerrainImage;
	if ( pImage == NULL ||
		pImage->Load( pFullPath ) == false )
		return FALSE;

	//  Make sure the file has the correct dimensions.
	if ( pImage->_width != pImage->_height * 2 )
	{
		theApp.AddErrorDialog(CString (pFullPath) + CString ("Terrain tiles must have 2:1 ratio!") );
		delete pImage;
		return FALSE;
	}

	//  Update the width and height if necessary.
	if ( width < pImage->_width )
		width = pImage->_width;
	if ( height < pImage->_height )
		height = pImage->_height;

	//  Flush the message queue.
	if ( theApp.FlushMessageQueue() == FALSE )
	{
		delete pImage;
		return FALSE;
	}

	//  Square the tile.
	pImage->Square(pFullPath);

	//  Flush the message queue.
	if ( theApp.FlushMessageQueue() == FALSE )
	{
		delete pImage;
		return FALSE;
	}

	//  Create three copies of the image for scaling.
	TerrainImage *pImage1_2 = new TerrainImage( *pImage );
	TerrainImage *pImage1_4 = new TerrainImage( *pImage );
	TerrainImage *pImage1_8 = new TerrainImage( *pImage );

	if ( pImage1_2 == NULL || pImage1_4 == NULL || pImage1_8 == NULL )
	{
		delete pImage;
		delete pImage1_2;
		delete pImage1_4;
		delete pImage1_8;
		return FALSE;
	}

	if ( theApp.FlushMessageQueue() == FALSE )
	{
		delete pImage;
		delete pImage1_2;
		delete pImage1_4;
		delete pImage1_8;
		return FALSE;
	}

	if ( pImage1_2->ScaleDownAvg( 2 ) == false ||
		pImage1_4->ScaleDownAvg( 4 ) == false ||
		pImage1_8->ScaleDownAvg( 8 ) == false )
	{
		delete pImage;
		delete pImage1_2;
		delete pImage1_4;
		delete pImage1_8;
		return FALSE;
	}

	//  Flush the message queue.
	if ( theApp.FlushMessageQueue() == FALSE )
	{
		delete pImage;
		delete pImage1_2;
		delete pImage1_4;
		delete pImage1_8;
		return FALSE;
	}

	//  Create the 8 levels of shaded bitmaps.
	if ( pImage->Shade() == false ||
		pImage1_2->Shade() == false ||
		pImage1_4->Shade() == false ||
		pImage1_8->Shade() == false )
	{
		delete pImage;
		delete pImage1_2;
		delete pImage1_4;
		delete pImage1_8;
		return FALSE;
	}

	//  Flush the message queue.
	if ( theApp.FlushMessageQueue() == FALSE )
	{
		delete pImage;
		delete pImage1_2;
		delete pImage1_4;
		delete pImage1_8;
		return FALSE;
	}

	//  Dither the images.
	if ( _bitDepth == 8 )
	{
		TerrainImage *pDithered = ( TerrainImage * )theApp.ditherer.DitherImage( pImage );
		TerrainImage *pDithered1_2 = ( TerrainImage * )theApp.ditherer.DitherImage( pImage1_2 );
		TerrainImage *pDithered1_4 = ( TerrainImage * )theApp.ditherer.DitherImage( pImage1_4 );
		TerrainImage *pDithered1_8 = ( TerrainImage * )theApp.ditherer.DitherImage( pImage1_8 );

		if ( pDithered == NULL || pDithered1_2 == NULL ||
			pDithered1_4 == NULL || pDithered1_8 == NULL )
			return FALSE;

		delete pImage;
		delete pImage1_2;
		delete pImage1_4;
		delete pImage1_8;
		pImage = pDithered;
		pImage1_2 = pDithered1_2;
		pImage1_4 = pDithered1_4;
		pImage1_8 = pDithered1_8;
	}
	else if ( _bitDepth == 15 )
	{
		if ( pImage->ReduceTo15() == false ||
			pImage1_2->ReduceTo15() == false ||
			pImage1_4->ReduceTo15() == false ||
			pImage1_8->ReduceTo15() == false )
		{
			delete pImage;
			delete pImage1_2;
			delete pImage1_4;
			delete pImage1_8;
			return FALSE;
		}
	}
	else if ( _bitDepth == 16 )
	{
		if ( pImage->ReduceTo16() == false ||
			pImage1_2->ReduceTo16() == false ||
			pImage1_4->ReduceTo16() == false ||
			pImage1_8->ReduceTo16() == false )
		{
			delete pImage;
			delete pImage1_2;
			delete pImage1_4;
			delete pImage1_8;
			return FALSE;
		}
	}
	else if ( _bitDepth == 32 )
	{
		if ( pImage->IncreaseTo32() == false ||
			pImage1_2->IncreaseTo32() == false ||
			pImage1_4->IncreaseTo32() == false ||
			pImage1_8->IncreaseTo32() == false )
		{
			delete pImage;
			delete pImage1_2;
			delete pImage1_4;
			delete pImage1_8;
			return FALSE;
		}
	}

	//  Flush the message queue.
	if ( theApp.FlushMessageQueue() == FALSE )
	{
		delete pImage;
		delete pImage1_2;
		delete pImage1_4;
		delete pImage1_8;
		return FALSE;
	}

	//  Save the image pointers.
	if ( type == 0 )
	{
		apBases[ index ][ 0 ] = pImage;
		apBases[ index ][ 1 ] = pImage1_2;
		apBases[ index ][ 2 ] = pImage1_4;
		apBases[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 1 )
	{
		apAnim1[ index ][ 0 ] = pImage;
		apAnim1[ index ][ 1 ] = pImage1_2;
		apAnim1[ index ][ 2 ] = pImage1_4;
		apAnim1[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 2 )
	{
		apAnim2[ index ][ 0 ] = pImage;
		apAnim2[ index ][ 1 ] = pImage1_2;
		apAnim2[ index ][ 2 ] = pImage1_4;
		apAnim2[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 3 )
	{
		apOverlay[ index ][ 0 ] = pImage;
		apOverlay[ index ][ 1 ] = pImage1_2;
		apOverlay[ index ][ 2 ] = pImage1_4;
		apOverlay[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 4 )
	{
		apUnderAnim1[ index ][ 0 ] = pImage;
		apUnderAnim1[ index ][ 1 ] = pImage1_2;
		apUnderAnim1[ index ][ 2 ] = pImage1_4;
		apUnderAnim1[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 5 )
	{
		apUnderAnim2[ index ][ 0 ] = pImage;
		apUnderAnim2[ index ][ 1 ] = pImage1_2;
		apUnderAnim2[ index ][ 2 ] = pImage1_4;
		apUnderAnim2[ index ][ 3 ] = pImage1_8;
	}
	else
		ASSERT( false );

	return TRUE;
}

BOOL TerrainView::SaveImage( FILE *pFile, Image *pImage )
{
	ASSERT( pFile != NULL );
	ASSERT( pImage != NULL );

	char 	id[ 2 ] = { '0', '0' };
	short	holdTime = 1;
	short	xoff = ( short )0;
	short	yoff = ( short )0;
	short	width = ( short )pImage->_width;
	short	height = ( short )( pImage->_height / 8 );

	int size = pImage->_height * pImage->_width * pImage->_bytesPerPixel;

	//  Write everything in the FB except the data.
	if ( fwrite( &id, 1, sizeof( id ), pFile ) != sizeof( id ) ||
		fwrite( &holdTime, 1, sizeof( holdTime ), pFile ) != sizeof( holdTime ) ||
		fwrite( &xoff, 1, sizeof( xoff ), pFile ) != sizeof( xoff ) ||
		fwrite( &yoff, 1, sizeof( yoff ), pFile ) != sizeof( yoff ) ||
		fwrite( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fwrite( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fwrite( &size, 1, sizeof( size ), pFile ) != sizeof( size ) )
 		return FALSE;

	if ( fwrite( pImage->_pPixels, 1, ( size_t )size, pFile ) != ( size_t )size )
		return FALSE;

	return TRUE;
}

Image *TerrainView::LoadImage( FILE *pFile, char const * pszFileName )
{
	char 	id[ 2 ];
	short	holdTime;
	short	xoff, yoff;
	short	width, height;
	int		size;

	if ( fread( &id, 1, sizeof( id ), pFile ) != sizeof( id ) ||
		fread( &holdTime, 1, sizeof( holdTime ), pFile ) != sizeof( holdTime ) ||
		fread( &xoff, 1, sizeof( xoff ), pFile ) != sizeof( xoff ) ||
		fread( &yoff, 1, sizeof( yoff ), pFile ) != sizeof( yoff ) ||
		fread( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fread( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fread( &size, 1, sizeof( size ), pFile ) != sizeof( size ) )
 		return FALSE;

	Image *pImage = new Image( width, height * 8, _bitDepth, theApp.aPalette );
	if ( pImage == NULL )
		return FALSE;

	pImage->strFileName = pszFileName;

	pImage->SetSkip( RGBColor( 255, 0, 255 ) );

	ASSERT( xoff == 0 && yoff == 0 );

	if ( fread( pImage->_pPixels, 1, ( size_t )size, pFile ) != ( size_t )size )
		return NULL;

	//  Return a pointer to the image.
	return pImage;
}

/******************************************
 *
 *  TerrainSprite
 *
 ******************************************/

char *TerrainSprite::damName[ MAX_DAMAGE ] =
{
	"No Damage",
	"Mild Damage",
	"Heavy Damage",
	"Engulfed In Flames"
};

char *TerrainSprite::dirName[ MAX_DIR ] = 
{
	"NorthWest",
	"NorthEast",
	"SouthEast",
	"SouthWest",
};

TerrainSprite::TerrainSprite( void )
{
	//  Set type.
	type = TERRAIN_SPRITE;

	//  Make sure all views are initially NULL.
	memset( aViews, 0, MAX_DAMAGE * MAX_DIR * sizeof( View * ) );
}

BOOL TerrainSprite::CreateSprite( const char *pDirectory, int bitDepth, const RGBColor *pPal )
{
	int damIndex, dirIndex;

	if ( Sprite::CreateSprite( pDirectory, bitDepth, pPal ) == FALSE )
		return FALSE;

	for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
	{
		for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
		{
			char aPattern[ 8 ];
			aPattern[ 0 ] = 'A';
			aPattern[ 1 ] = ( char )( 'A' + dirIndex * 2 );
			aPattern[ 2 ] = ( char )( '0' + damIndex );
			aPattern[ 3 ] = '\0';

			TerrainView *pView = new TerrainView;
			if ( pView == NULL )
				return FALSE;

			if ( pView->CreateView( this, pDirectory, aPattern, _bitDepth ) == FALSE )
			{
				if ( GetLastError() != USER_ERROR_NO_FILES )
				{
					delete pView;
					return FALSE;
				}

				delete pView;
/*
				int tempDamIndex = damIndex - 1;
				while( tempDamIndex >= 0 )
				{
					if ( aViews[ dirIndex ][ tempDamIndex ] )
						aViews[ dirIndex ][ damIndex ] = aViews[ dirIndex ][ tempDamIndex ];
					tempDamIndex--;
				}
*/
			}
			else
			{
				//  Check overlay conditions.
				if ( pView->overlayCnt )
				{
					if ( pView->baseCnt )
					{
						theApp.AddErrorDialog(CString (pDirectory) + CString (" - cannot have base and overlay for same damage level - Check filenames." ));
						return FALSE;
					}
					if ( damIndex == 0 )
					{
						theApp.AddErrorDialog(CString (pDirectory) + CString (" - cannot have overlay for damage level 0\n" ));
						return FALSE;
					}
					if ( aViews[ dirIndex ][ 0 ] == NULL ||
						aViews[ dirIndex ][ 0 ]->baseCnt == 0 )
					{
						theApp.AddErrorDialog(CString (pDirectory) + CString (" - must have base bitmaps for damage level 0 in order to use overlay bitmaps." ));
						return FALSE;
					}
				}

				if ( damIndex != 0 )
					pView->BorrowFromView( aViews[ dirIndex ][ 0 ], TRUE );

				if ( AddView( pView ) == FALSE )
					return FALSE;

				aViews[ dirIndex ][ damIndex ] = pView;
			}

			theApp.UpdateProgressDialog();
		}
	}

	//  If no matching files were found, return false.
	if ( viewCnt == 0 )
	{
		theApp.AddErrorDialog(CString (pDirectory) + CString (" - No files found!" ));
		return FALSE;
	}

	return TRUE;
}

BOOL TerrainSprite::AddToTreeView( SpriteTreeCtrl *pTreeCtrl, HTREEITEM hParent, BOOL bSelectThisView )
{
	assert( pTreeCtrl != NULL );

	//  Create the root for this sprite.
	CString	str;
	str.Format( "%d bit", _bitDepth );
	HTREEITEM hTreeRoot = pTreeCtrl->AddNewItem( hParent, str, 0, ( HTREEITEM )TVI_LAST );

	HTREEITEM hDam = NULL, hDir = NULL;

	BOOL	bFirstView = bSelectThisView;

	int damIndex, dirIndex;
	for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
	{
		hDir = NULL;
		for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
		{
			if ( aViews[ dirIndex ][ damIndex ] )
			{
				//  Add this direction if necessary.
				if ( hDir == NULL )
					hDir = pTreeCtrl->AddNewItem( hTreeRoot, dirName[ dirIndex ], 0, ( HTREEITEM )TVI_LAST );

				//  Add this damage
				hDam = pTreeCtrl->AddNewItem( hDir, damName[ damIndex ], 0, ( HTREEITEM )TVI_LAST );

				//  Add the view.
				aViews[ dirIndex ][ damIndex ]->AddToTree( pTreeCtrl, hDam, bFirstView );
				bFirstView = FALSE;
			}
		}
	}

	return TRUE;
}

BOOL TerrainSprite::SaveOldSPR( const char *pPath, BOOL bShareware )
{
	assert( pPath != NULL );

	FILE *pFile = fopen( pPath, "wb" );
	if ( pFile == NULL )
		return FALSE;

	if ( fwrite( &type, 1, sizeof( type ), pFile ) != sizeof( type ) ||
		fwrite( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fwrite( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fwrite( &_bitDepth, 1, sizeof( _bitDepth ), pFile ) != sizeof( _bitDepth ) )
		return false;

	//  Save the view count
	if ( fwrite( &viewCnt, 1, sizeof( viewCnt ), pFile ) != sizeof( viewCnt ) )
		return false;

	//  Save the views.
	int viewIndex;
	for( viewIndex = 0; viewIndex < viewCnt; viewIndex++ )
		if ( ppViews[ viewIndex ]->Save( pFile, bShareware ) == FALSE )
			return FALSE;

	//  Save the pointers to the views.
	int dirIndex, damIndex;
	for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
	{
		for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
		{
			if ( aViews[ dirIndex ][ damIndex ] == NULL )
			{
				int null = -1;
				if ( fwrite( &null, 1, sizeof( null ), pFile ) != sizeof( null ) )
					return FALSE;
			}
			else
			{
				for( viewIndex = 0; viewIndex < viewCnt; viewIndex++ )
				{
					if ( ppViews[ viewIndex ] == aViews[ dirIndex ][ damIndex ] )
					{
						if ( fwrite( &viewIndex, 1, sizeof( viewIndex ), pFile ) != sizeof( viewIndex ) )
							return FALSE;
						break;
					}
				}
				assert( viewIndex < viewCnt );
			}
		}
	}

	fflush( pFile );
	fclose( pFile );

	return TRUE;
}

BOOL TerrainSprite::Load( const char *pPath )
{
	assert( pPath != NULL );

	FILE *pFile = fopen( pPath, "rb" );
	if ( pFile == NULL )
		return FALSE;

	if ( fread( &type, 1, sizeof( type ), pFile ) != sizeof( type ) ||
		type != TERRAIN_SPRITE )
		return FALSE;

	if ( fread( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fread( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fread( &_bitDepth, 1, sizeof( _bitDepth ), pFile ) != sizeof( _bitDepth ) )
		return FALSE;

	//  Read the count.
	if ( fread( &viewCnt, 1, sizeof( viewCnt ), pFile ) != sizeof( viewCnt ) )
		return FALSE;

	//  Alloc the view array.
	ppViews = new View * [ viewCnt ];
	if ( ppViews == NULL )
		return FALSE;
	( void )memset( ppViews, 0, viewCnt * sizeof( View * ) );

	//  Read the views.
	int viewIndex;
	for( viewIndex = 0; viewIndex < viewCnt; viewIndex++ )
	{
		ppViews[ viewIndex ] = new TerrainView;
		if ( ppViews[ viewIndex ]->Load( pFile, pPath ) == FALSE )
			return FALSE;
	}

	//  Read the pointers to the views.
	int damIndex, dirIndex;
	for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
	{
		for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
		{
			int index;
			if ( fread( &index, 1, sizeof( index ), pFile ) != sizeof( index ) )
				return false;
			if ( index == -1 )
				aViews[ dirIndex ][ damIndex ] = NULL;
			else
			{
				//  Check overlay conditions.
				if ( ppViews[ index ]->overlayCnt )
				{
					if ( ppViews[ index ]->baseCnt )
					{
						theApp.AddErrorDialog(CString (pPath) + CString (" - cannot have base and overlay for same damage level - Check filenames." ));
						return FALSE;
					}
					if ( damIndex == 0 )
					{
						theApp.AddErrorDialog(CString (pPath) + CString (" - cannot have overlay for damage level 0\n" ));
						return FALSE;
					}
					if ( aViews[ dirIndex ][ 0 ] == NULL ||
						aViews[ dirIndex ][ 0 ]->baseCnt == 0 )
					{
						theApp.AddErrorDialog(CString (pPath) + CString (" - must have base bitmaps for damage level 0 in order to use overlay bitmaps." ));
						return FALSE;
					}
				}

				if ( damIndex != 0 )
					ppViews[ index ]->BorrowFromView( aViews[ dirIndex ][ 0 ], TRUE );

				aViews[ dirIndex ][ damIndex ] = ppViews[ index ];
			}
		}
	}

	fflush( pFile );
	fclose( pFile );
	
	return TRUE;
}

