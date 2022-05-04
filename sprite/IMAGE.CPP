#include <math.h>

#include "stdafx.h"

#include "bmpfile.h"
#include "tgafile.h"
#include "image.h"
#include "sprite.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

char * g_apszHotSpotName[ NUM_HOTSPOTS ] =
{
	"Smoke & Flame",
	"Turret",
	"Flag",
};

Image::Image()
	:
		_pPalette( NULL ),
		_pPixels( NULL )
{
	_width = 0;
	_height = 0;
	_xoff = 0;
	_yoff = 0;
	_holdTime = 1;
	_pPixels = NULL;
	_bytesPerPixel = 0;
	_bitDepth = 0;
	_pPalette = NULL;

	ResetHotSpots();
}

		// GG
void
Image::ResetHotSpots()
{
	nAttachedSpriteCnt  = 0;
	bAnchorPointIsValid = FALSE;
 	_bScannedForHotSpots = FALSE;
}

Image::Image( int width, int height, int bitDepth, RGBColor *pPal )
	:
		_pPalette( NULL ),
		_pPixels( NULL )
{
	ResetHotSpots();

	assert( bitDepth == 8 || bitDepth == 15 ||
		bitDepth == 16 || bitDepth == 24 || bitDepth == 32 );
	assert( bitDepth != 8 || pPal != NULL );

	_width = width;
	_height = height;
	_xoff = 0;
	_yoff = 0;
	_holdTime = 1;
	_bitDepth = bitDepth;
	switch( _bitDepth )
	{
		default :
			assert( false );
			break;
		case 8 :
			_bytesPerPixel = 1;
			_pPalette = new RGBColor[ 256 ];
			( void )memcpy( _pPalette, pPal, 256 * sizeof( RGBColor ) );
			break;
		case 15 :
		case 16 :
			_pPalette = NULL;
			_bytesPerPixel = 2;
			break;
		case 24 :
			_pPalette = NULL;
			_bytesPerPixel = 3;
			break;
		case 32 :
			_pPalette = NULL;
			_bytesPerPixel = 4;
			break;
	}

	_pPixels = new byte[ _width * _height * _bytesPerPixel ];
}

Image::Image( const Image &oldImage )
	:
		_pPalette( NULL ),
		_pPixels( NULL )
{
	strFileName	= oldImage.strFileName;

	_skip32 = oldImage._skip32;
	_skip24 = oldImage._skip24;
	_skip16 = oldImage._skip16;
	_skip15 = oldImage._skip15;
	memcpy( _skip8, oldImage._skip8, 256 * sizeof( bool ) );
	_skipIndex = oldImage._skipIndex;

	CopyHotSpots( & oldImage );

	_width = oldImage._width;
	_height = oldImage._height;
	_xoff = oldImage._xoff;
	_yoff = oldImage._yoff;
	_holdTime = oldImage._holdTime;
	_bytesPerPixel = oldImage._bytesPerPixel;
	_bitDepth = oldImage._bitDepth;

	_pPixels = new byte[ _width * _height * _bytesPerPixel ];
	if ( _pPixels == NULL )
		return;
	memcpy( _pPixels, oldImage._pPixels, _width * _height * _bytesPerPixel );

	_pPalette = NULL;
	if ( _bitDepth == 8 )
	{
		assert( oldImage._pPalette != NULL );
		_pPalette = new RGBColor[ 256 ];
		if ( _pPalette == NULL )
			return;
		memcpy( _pPalette, oldImage._pPalette, 256 * sizeof( RGBColor ) );
	}
}

// GG
void
Image::CopyHotSpots(
	Image const * pimage )
{
	nAttachedSpriteCnt  = pimage->nAttachedSpriteCnt;
	bAnchorPointIsValid = pimage->bAnchorPointIsValid;
	anchorPoint         = pimage->anchorPoint;

	memcpy( aAttachedSprites, pimage->aAttachedSprites, sizeof( aAttachedSprites ));

	_bScannedForHotSpots = pimage->_bScannedForHotSpots;
}

bool Image::Load( const char *pFilename )
{
	assert( pFilename != NULL );

	ResetHotSpots();

	strFileName = pFilename;

	//  Clean up my current contents.
	delete[] _pPixels;
	delete[] _pPalette;

	char *pExt = strrchr( pFilename, '.' );
	if ( pExt == NULL )
		return false;		//  Can't guess at file type.

	Image *pImage = NULL;
	if ( _stricmp( pExt, ".dib" ) == 0 ||
		_stricmp( pExt, ".bmp" ) == 0 )
	{
		BMPFile file;
		pImage = file.Load( pFilename );
	}
	else if ( _stricmp( pExt, ".tga" ) == 0 )
	{
		TGAFile file;
		pImage = file.Load( pFilename );
	}

	if ( pImage == NULL )
		return false;

	_skip32 = pImage->_skip32;
	_skip24 = pImage->_skip24;
	_skip16 = pImage->_skip16;
	_skip15 = pImage->_skip15;
	memcpy( _skip8, pImage->_skip8, 256 * sizeof( bool ) );
	_skipIndex = pImage->_skipIndex;

	_width = pImage->_width;
	_height = pImage->_height;
	_xoff = pImage->_xoff;
	_yoff = pImage->_yoff;
	_holdTime = pImage->_holdTime;
	_bytesPerPixel = pImage->_bytesPerPixel;
	_bitDepth = pImage->_bitDepth;

	_pPixels = new byte[ _width * _height * _bytesPerPixel ];
	if ( _pPixels == NULL )
		return false;
	memcpy( _pPixels, pImage->_pPixels, _width * _height * _bytesPerPixel );

	_pPalette = NULL;
	if ( _bitDepth == 8 )
	{
		assert( pImage->_pPalette != NULL );
		_pPalette = new RGBColor[ 256 ];
		if ( _pPalette == NULL )
			return false;
		memcpy( _pPalette, pImage->_pPalette, 256 * sizeof( RGBColor ) );
	}

	delete pImage;	//  Done with the loaded image.

	return true;
}

Image::~Image()
{
	delete[] _pPalette;
	delete[] _pPixels;
}

void Image::SetSkip( RGBColor skip )
{
	//  Set all skips, so I have them if I need them later.
	//  Note can only set 8-bit skip if I have a palette;  otherwise
	//  I ignore it.
	_skip32 = ( skip.Red() << 16 ) | ( skip.Green() << 8 ) | ( skip.Blue() );
	_skip24 = skip;
	_skip16 = skip.To16();
	_skip15 = skip.To15();

	if ( _pPalette )
	{
		int palIndex;
		_skipIndex = 0;
		for( palIndex = 0; palIndex < 256; palIndex++ )
		{
			if ( _pPalette[ palIndex ] == skip )
			{
				_skip8[ palIndex ] = true;
				_skipIndex = ( byte )palIndex;
			}
			else
				_skip8[ palIndex ] = false;
		}
	}
}

void Image::Render( int x, int y, DrawSurface *pDrawSurf ) const
{
	//  Make sure the draw surface is the right render depth.
	assert( pDrawSurf->GetBitDepth() == _bitDepth );
	
	int renderWidth = _width;
	int renderHeight = _height;
	x += _xoff;
	y += _yoff;

	int left = 0;
	if ( x < 0 )
	{
		left = -x;
		x = 0;
	}
	int width = renderWidth - left;
	if ( width + x > pDrawSurf->GetWidth() )
		width -= ( width + x - pDrawSurf->GetWidth() );

	int top = 0;
	if ( y < 0 )
	{
		top = -y;
		y = 0;
	}
	int height = renderHeight - top;
	if ( height + y > pDrawSurf->GetHeight() )
		height -= ( height + y - pDrawSurf->GetHeight() );

	int row, col;
	for( row = 0; row < height; row++ )
	{
		byte *pDest = pDrawSurf->GetPixels() + ( row + y ) * pDrawSurf->GetStride() + x * _bytesPerPixel;
		byte *pSrc = _pPixels + ( ( row + top ) * renderWidth + left ) * _bytesPerPixel;

		for( col = 0; col < width; col++ )
		{
			if ( !IsTransparent( pSrc ) )
			{
//  HACK - because $#@*$^#@* windoze expects 24 bit to be BGR?
#if 0
				if ( _bytesPerPixel == 4 )
				{
					*pDest++ = *pSrc++;
					*pDest++ = *pSrc++;
					*pDest++ = *pSrc++;
					*pDest++ = *pSrc++;
				}
				else if ( _bytesPerPixel == 3 )
				{
					pDest[ 0 ] = pSrc[ 2 ];
					pDest[ 1 ] = pSrc[ 1 ];
					pDest[ 2 ] = pSrc[ 0 ];
					pDest += 3;
					pSrc += 3;
				}
				else if ( _bytesPerPixel == 2 )
				{
					*pDest++ = *pSrc++;
					*pDest++ = *pSrc++;
				}
				else
					*pDest++ = *pSrc++;

#else
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
#endif
			}
			else
			{
				pDest += _bytesPerPixel;
				pSrc += _bytesPerPixel;
			}
		}
	}
}

bool Image::Crop()
{
	int row, col;
	int top, bottom, left, right;

	//  Make sure we have pixels to check.
	if ( (_width <= 0) || (_height <= 0) )
		{
		top = left = right = bottom = 0;
		goto AllDone;
		}
		
	assert( _pPixels != NULL );

	//  Search down from the top of the bitmap for 
	//  the first row which contains non-blank pixels.
	for( row = 0; row < _height; row++ )
	{
		byte *pRowBytes = _pPixels + row * _width * _bytesPerPixel;
		for( col = 0; col < _width; col++ )
		{
			if ( !IsTransparent( pRowBytes ) )
				break;
			pRowBytes += _bytesPerPixel;
		}

		if ( col < _width )
			break;
	}
	top = row;
	
	//  Search up from the bottom of the bitmap for 
	//  the last row which contains non-blank pixels.
	for( row = _height - 1; row >= top; row-- )
	{
		byte *pRowBytes = _pPixels + row * _width * _bytesPerPixel;
		for( col = 0; col < _width; col++ )
		{
			if ( !IsTransparent( pRowBytes ) )
				break;
			pRowBytes += _bytesPerPixel;
		}

		if ( col < _width )
			break;
	}
	bottom = row;

	//  Search from the left of the bitmap for the 
	//  first column which contains non-blank pixels.
	for( col = 0; col < _width; col++ )
	{
		byte *pRowBytes = _pPixels + ( top * _width + col ) * _bytesPerPixel;
		for( row = top; row <= bottom; row++ )
		{
			if ( !IsTransparent( pRowBytes ) )
				break;
			pRowBytes += _width * _bytesPerPixel;
		}

		if ( row <= bottom )
			break;
	}
	left = col;	

	//  Search from the left of the bitmap for the first
	//  column which contains non-blank pixels.
	for( col = _width - 1; col >= left; col-- )
	{
		byte *pRowBytes = _pPixels + ( top * _width + col ) * _bytesPerPixel;
		for( row = top; row <= bottom; row++ )
		{
			if ( !IsTransparent( pRowBytes ) )
				break;
			pRowBytes += _width * _bytesPerPixel;
		}

		if ( row <= bottom )
			break;
	}
	right = col;	 

	//  Create a new data buffer to hold the cropped image.
AllDone:
	if ( (right < left) || (bottom < top) )
		top = left = right = bottom = 0;
	int croppedWidth = __max ( 1, right - left + 1 );
	int croppedHeight = __max ( 1, bottom - top + 1 );
	byte *pCroppedBuf = new byte[ croppedWidth * croppedHeight * _bytesPerPixel ];
	if ( pCroppedBuf == NULL )
		return false;

	//  Copy the cropped area to the new buffer.
	byte *pDest = pCroppedBuf;
	byte *pSrc = _pPixels + ( top * _width + left ) * _bytesPerPixel;
	for( row = 0; row < croppedHeight; row++ )
	{
		( void )memcpy( pDest, pSrc, croppedWidth * _bytesPerPixel );
		pDest += croppedWidth * _bytesPerPixel;
		pSrc += _width * _bytesPerPixel;
	}

	//  Free the old buffer.
	delete[] _pPixels;
	
	//  Reconfigure the bitmap.
	_width = croppedWidth;
	_height = croppedHeight;
	_xoff = left;
	_yoff = top;
	_pPixels = pCroppedBuf;

	return true;
}

bool Image::ScaleUp ( float fMul )
{
	//  Use correct width value
	int oldWidth = _width;
	int oldHeight = _height;

	//  Calculate new width and height
	int newWidth = (int) ( (float) oldWidth * fMul );
	int newHeight = (int) ( (float) oldHeight * fMul );

	//  Allocate a new buffer to hold the scaled bitmap.
	byte *pNewBuffer = new byte[ newWidth * newHeight * _bytesPerPixel ];
	if ( pNewBuffer == NULL )
		return false;

	int newRow, newCol;
	float foldRow, foldCol;
	float fInc = 1.0f / fMul;
	for( newRow = 0, foldRow = 0.0f; newRow < newHeight; newRow++, foldRow += fInc )
	{
		for( newCol = 0, foldCol = 0.0f; newCol < newWidth; newCol++, foldCol += fInc )
		{
			byte *pDest = pNewBuffer + ( newRow * newWidth + newCol ) * _bytesPerPixel;
			byte *pSrc = _pPixels + ( (int) foldRow * oldWidth + (int) foldCol ) * _bytesPerPixel;
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

	//  Free the old buffer.
	delete[] _pPixels;

	//  Save the new buffer and adjust the parameters of the image.
	_width = newWidth;
	_height = newHeight;
	_xoff = (int) ( (float) _xoff * fMul );
	_yoff = (int) ( (float) _yoff * fMul );
	_pPixels = pNewBuffer;

	return true;
}

bool Image::ScaleDown( int scale )
{
	//  Use correct width value
	int oldWidth = _width;
	int oldHeight = _height;

	//  Calculate new width and height
	int newWidth = oldWidth / scale;
	int newHeight = oldHeight / scale;

	//  Allocate a new buffer to hold the scaled bitmap.
	byte *pNewBuffer = new byte[ newWidth * newHeight * _bytesPerPixel ];
	if ( pNewBuffer == NULL )
		return false;

	int newRow, oldRow, newCol, oldCol;
	for( newRow = 0, oldRow = 0; newRow < newHeight; newRow++, oldRow += scale )
	{
		for( newCol = 0, oldCol = 0; newCol < newWidth; newCol++, oldCol += scale )
		{
			byte *pDest = pNewBuffer + ( newRow * newWidth + newCol ) * _bytesPerPixel;
			byte *pSrc = _pPixels + ( oldRow * oldWidth + oldCol ) * _bytesPerPixel;
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

	//  Free the old buffer.
	delete[] _pPixels;

	//  Save the new buffer and adjust the parameters of the image.
	_width = newWidth;
	_height = newHeight;
	_xoff = _xoff / scale;
	_yoff = _yoff / scale;
	_pPixels = pNewBuffer;

	return true;
}

bool Image::ScaleDownAvg( int scale )
{
	//  This operation is only implemented on 24-bit images.
	assert( _bitDepth == 24 );

	//  Use correct width value
	int oldWidth = _width;
	int oldHeight = _height;

	//  Calculate new width and height
	int newWidth = oldWidth / scale;
	int newHeight = oldHeight / scale;

	//  Allocate a new buffer to hold the scaled bitmap.
	byte *pNewBuffer = new byte[ newWidth * newHeight * _bytesPerPixel ];
	if ( pNewBuffer == NULL )
		return false;

	int newRow, oldRow, newCol, oldCol;
	for( newRow = 0, oldRow = 0; newRow < newHeight; newRow++, oldRow += scale )
	{
		for( newCol = 0, oldCol = 0; newCol < newWidth; newCol++, oldCol += scale )
		{
			//  Sum together all of the pixels being compressed into a single pixel.
			int r = 0, g = 0, b = 0;
			int row, col;
			for( row = oldRow; row < oldRow + scale; row++ )
			{
				for( col = oldCol; col < oldCol + scale; col++ )
				{
					r += _pPixels[ ( row * oldWidth + col ) * 3 + RED_OFFSET ];
					g += _pPixels[ ( row * oldWidth + col ) * 3 + GREEN_OFFSET ];
					b += _pPixels[ ( row * oldWidth + col ) * 3 + BLUE_OFFSET ];
				}
			}

			//  The value of the new pixel is the average of the old values.
			pNewBuffer[ ( newRow * newWidth + newCol ) * 3 + RED_OFFSET ] = ( BYTE )( r / ( scale * scale ) );
			pNewBuffer[ ( newRow * newWidth + newCol ) * 3 + GREEN_OFFSET ] = ( BYTE )( g / ( scale * scale ) );
			pNewBuffer[ ( newRow * newWidth + newCol ) * 3 + BLUE_OFFSET ] = ( BYTE )( b / ( scale * scale ) );
		}
	}

	//  Free the old buffer.
	delete[] _pPixels;

	//  Save the new buffer and adjust the parameters of the image.
	_width = newWidth;
	_height = newHeight;
	_xoff = _xoff / scale;
	_yoff = _yoff / scale;
	_pPixels = pNewBuffer;

	return true;
}

bool Image::ReduceTo15()
{
	//  This is only implemented for 24-bit images.
	assert( _bitDepth == 24 );

	_bytesPerPixel = 2;
	_bitDepth = 15;

	byte *pNewPixels = new byte[ _width * _height * _bytesPerPixel ];
	if ( pNewPixels == NULL )
		return false;

	word *pPix = ( word * )pNewPixels;
	byte *pOldPix = _pPixels;
	int row, col;
	for( row = 0; row < _height; row++ )
	{
		for( col = 0; col < _width; col++ )
		{
			*pPix++ = ( ( RGBColor * )pOldPix )->To15();
			pOldPix += 3;
		}
	}

	delete[] _pPixels;
	_pPixels = pNewPixels;

	return true;
}

bool Image::ReduceTo16()
{
	//  This is only implemented for 24-bit images.
	assert( _bitDepth == 24 );

	_bytesPerPixel = 2;
	_bitDepth = 16;

	byte *pNewPixels = new byte[ _width * _height * _bytesPerPixel ];
	if ( pNewPixels == NULL )
		return false;

	word *pPix = ( word * )pNewPixels;
	byte *pOldPix = _pPixels;
	int row, col;
	for( row = 0; row < _height; row++ )
	{
		for( col = 0; col < _width; col++ )
		{
			*pPix++ = ( ( RGBColor * )pOldPix )->To16();
			pOldPix += 3;
		}
	}

	delete[] _pPixels;
	_pPixels = pNewPixels;

	return true;
}

bool Image::IncreaseTo32()
{
	//  This is only implemented for 24-bit images.
	assert( _bitDepth == 24 );

	_bytesPerPixel = 4;
	_bitDepth = 32;

	byte *pNewPixels = new byte[ _width * _height * _bytesPerPixel ];
	if ( pNewPixels == NULL )
		return false;

	byte *pPix = pNewPixels;
	byte *pOldPix = _pPixels;
	int row, col;
	for( row = 0; row < _height; row++ )
	{
		for( col = 0; col < _width; col++ )
		{
			pPix[ 0 ] = pOldPix[ Image::BLUE_OFFSET ];
			pPix[ 1 ] = pOldPix[ Image::GREEN_OFFSET ];
			pPix[ 2 ] = pOldPix[ Image::RED_OFFSET ];
			pPix[ 3 ] = 0;
			pPix += 4;
			pOldPix += 3;
		}
	}

	delete[] _pPixels;
	_pPixels = pNewPixels;

	return true;
}

// GG
void Image::ScanForHotSpots(
	BOOL bReportErrors, float fMul )
{
	if ( _bScannedForHotSpots )
		return;

	fMul = 1.0f;

	_bScannedForHotSpots = TRUE;

	assert( _bitDepth == 24 );	//  Only works on 24-bit images.

	int row, col;
	byte *pPixels = _pPixels;
	for( row = 0; row < _height; row++ )
	{
		for( col = 0; col < _width; col++, pPixels += 3 )
		{
			BOOL bReplacePixel = FALSE;

			//  First, check for anchor point ( cyan )
			if ( pPixels[ RED_OFFSET ] == 0 &&
				pPixels[ GREEN_OFFSET ] == 255 &&
				pPixels[ BLUE_OFFSET ] == 255 )
			{
				//  We have an anchor point.
				if ( bAnchorPointIsValid && bReportErrors )
				{
					CString str;
					str.Format( "Multiple anchor points defined! - at ( %d, %d )\n"
						"File %s\n", row, col, ( LPCTSTR )strFileName );
					theApp.AddErrorDialog( str );	// GG: 8/25/96
					// AfxMessageBox( "Multiple anchor points defined!" );
				}

				//  Only the x and y are actually valid
				//  for anchor points.
				anchorPoint.iX = _xoff + (int) ( (float) col * fMul );
				anchorPoint.iY = _yoff + (int) ( (float) row * fMul );

				//  We now have a valid anchor point.
				bAnchorPointIsValid = TRUE;
			
				//  We need to replace this pixel.
				bReplacePixel = TRUE;
			}
			//  Next, check for smoke/flame registration 
			//  points.
			else if ( pPixels[ RED_OFFSET ] == 255 &&
				pPixels[ BLUE_OFFSET ] == 0 )
			{
				//  Determine if this is actually a 
				//  smoke/fire point
				int index;
				for( index = 0; index < MAX_SMOKE_FIRE; index++ )
				{
					if ( pPixels[ GREEN_OFFSET ] == index )
					{
						//  We have a smoke/fire point
						
						//  Make sure this isn't a duplicate of a point
						//  we already have.
						assert( nAttachedSpriteCnt < MAX_ATTACHED_SPRITES );
						BOOL bDuplicateAttachment = FALSE;
						for( int existingAttachmentsIndex = 0; existingAttachmentsIndex < nAttachedSpriteCnt; existingAttachmentsIndex++ )
						{
							if ( aAttachedSprites[ existingAttachmentsIndex ].iType == HOTSPOT_SMOKE_FIRE &&
								aAttachedSprites[ existingAttachmentsIndex ].iIndex == index )
							{
								if ( bReportErrors )
								{
									CString str;
									str.Format( "Duplicate smoke/fire at ( %d, %d ), index %d\n"
										"File %s\n", row, col, index, ( LPCTSTR )strFileName );
									theApp.AddErrorDialog( str );	// GG 8/25/96
								}

								bDuplicateAttachment = TRUE;
								break;
							}
						}

						if ( bDuplicateAttachment == FALSE )
						{
							//  Add this point.
							aAttachedSprites[ nAttachedSpriteCnt ].iType = HOTSPOT_SMOKE_FIRE;
							aAttachedSprites[ nAttachedSpriteCnt ].iX = _xoff + (int) ( (float) col * fMul );
							aAttachedSprites[ nAttachedSpriteCnt ].iY = _yoff + (int) ( (float) row * fMul );
							aAttachedSprites[ nAttachedSpriteCnt ].iIndex = index;
							nAttachedSpriteCnt++;
						}

						//  Whether or not we added a hotspot,
						//  we need to replace the pixel.
						bReplacePixel = TRUE;

						//  No need to check the rest of the indices.
						break;
					}
				}
			}
			//  Next, check for turret registration 
			//  points.
			else if ( pPixels[ GREEN_OFFSET ] == 255 &&
				pPixels[ BLUE_OFFSET ] == 0 )
			{
				//  Determine if this is actually a 
				//  turret point
				int index;
				for( index = 0; index < MAX_TURRETS; index++ )
				{
					if ( pPixels[ RED_OFFSET ] == index )
					{
						//  We have a turret point
						
						//  Make sure this isn't a duplicate 
						//  of a point we already have.
						assert( nAttachedSpriteCnt < MAX_ATTACHED_SPRITES );
						BOOL bDuplicateAttachment = FALSE;
						for( int existingAttachmentsIndex = 0; existingAttachmentsIndex < nAttachedSpriteCnt; existingAttachmentsIndex++ )
						{
							if ( aAttachedSprites[ existingAttachmentsIndex ].iType == HOTSPOT_TURRET &&
								aAttachedSprites[ existingAttachmentsIndex ].iIndex == index )
							{
								if ( bReportErrors )
								{
									CString str;
									str.Format( "Duplicate turret at ( %d, %d ) index %d\n"
										"File %s\n", row, col, index, ( LPCTSTR )strFileName );
									theApp.AddErrorDialog( str );	// GG: 8/25/96
								}

								bDuplicateAttachment = TRUE;
								break;
							}
						}

						if ( bDuplicateAttachment == FALSE )
						{
							//  Add this point.
							aAttachedSprites[ nAttachedSpriteCnt ].iType = HOTSPOT_TURRET;
							aAttachedSprites[ nAttachedSpriteCnt ].iX = _xoff + (int) ( (float) col * fMul );
							aAttachedSprites[ nAttachedSpriteCnt ].iY = _yoff + (int) ( (float) row * fMul );
							aAttachedSprites[ nAttachedSpriteCnt ].iIndex = index;
							nAttachedSpriteCnt++;
						}

						//  Whether or not we added a hotspot,
						//  we need to replace the pixel.
						bReplacePixel = TRUE;

						//  No need to check the rest of the indices.
						break;
					}
				}
			}
			//  Finally, check for flag registration 
			//  points.
			else if ( pPixels[ GREEN_OFFSET ] == 0 &&
				pPixels[ BLUE_OFFSET ] == 255 )
			{
				//  Determine if this is actually a 
				//  flag point
				int index;
				for( index = 0; index < MAX_FLAGS; index++ )
				{
					if ( pPixels[ RED_OFFSET ] == index )
					{
						//  We have a flag point
						
						//  Make sure this isn't a duplicate of a point
						//  we already have.
						assert( nAttachedSpriteCnt < MAX_ATTACHED_SPRITES );
						BOOL bDuplicateAttachment = FALSE;
						for( int existingAttachmentsIndex = 0; existingAttachmentsIndex < nAttachedSpriteCnt; existingAttachmentsIndex++ )
						{
							if ( aAttachedSprites[ existingAttachmentsIndex ].iType == HOTSPOT_FLAG &&
								aAttachedSprites[ existingAttachmentsIndex ].iIndex == index )
							{
								if ( bReportErrors )
								{
									CString str;
									str.Format( "Duplicate flag at ( %d, %d ) index %d\nFile %s\n", row, col, index, ( LPCTSTR )strFileName );
									theApp.AddErrorDialog( str );	// GG: 8/25/96
								}

								bDuplicateAttachment = TRUE;
								break;
							}
						}

						if ( bDuplicateAttachment == FALSE )
						{
							//  Add this point.
							aAttachedSprites[ nAttachedSpriteCnt ].iType = HOTSPOT_FLAG;
							aAttachedSprites[ nAttachedSpriteCnt ].iX = _xoff + (int) ( (float) col * fMul );
							aAttachedSprites[ nAttachedSpriteCnt ].iY = _yoff + (int) ( (float) row * fMul );
							aAttachedSprites[ nAttachedSpriteCnt ].iIndex = index;
							nAttachedSpriteCnt++;
						}

						//  Whether or not we added a hotspot,
						//  we need to replace the pixel.
						bReplacePixel = TRUE;

						//  No need to check the rest of the indices.
						break;
					}
				}
			}

			if ( bReplacePixel )
			{
				if ( col == 0 )
				{
					//  Need to use pixel to the right.
					//  This can cause problems if the 
					//  next pixel is also a hotspot, 
					//  but I'm just going to hope it 
					//  doesn't happen.
					pPixels[ RED_OFFSET ] = ( pPixels + 3 )[ RED_OFFSET ];
					pPixels[ GREEN_OFFSET ] = ( pPixels + 3 )[ GREEN_OFFSET ];
					pPixels[ BLUE_OFFSET ] = ( pPixels + 3 )[ BLUE_OFFSET ];
				}
				else
				{
					//  Use pixel to the left.  This will
					//  not cause problems in general because
					//  the pixel to the left will have been 
					//  processed ( unless the pixel to the left
					//  is column 0, in which case this will cause
					//  both pixels to have the same value that this
					//  pixel does ).
					pPixels[ RED_OFFSET ] = ( pPixels - 3 )[ RED_OFFSET ];
					pPixels[ GREEN_OFFSET ] = ( pPixels - 3 )[ GREEN_OFFSET ];
					pPixels[ BLUE_OFFSET ] = ( pPixels - 3 )[ BLUE_OFFSET ];
				}
			}
		}
	}
}
