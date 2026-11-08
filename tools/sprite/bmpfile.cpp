#include "stdafx.h"

#include "bmpfile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DW_ALIGN( X ) ( ( ( X ) + 3 ) & ~3 )

word BMPFile::BMP_MAGIC = 0x4D42;

/*
 *  Get the number of bytes in a row of the DIB.
 */
int BMPFile::BytesPerRow()
{
	int rowBytes = 0;
	switch( bmpInfoHeader.biBitCount )
	{
		default :
			assert( FALSE );
			break;

		case 1 :
			rowBytes = ( bmpInfoHeader.biWidth + 7 ) / 8;
			break;

		case 2 :
			rowBytes = ( bmpInfoHeader.biWidth + 3 ) / 4;
			break;

		case 4 :
			rowBytes = ( bmpInfoHeader.biWidth + 1 ) / 2;
			break;

		case 8 :
			rowBytes = bmpInfoHeader.biWidth;
			break;

		case 16 :
			rowBytes = bmpInfoHeader.biWidth * 2;
			break;

		case 24 :
			rowBytes = bmpInfoHeader.biWidth * 3;
			break;

		case 32 :
			rowBytes = bmpInfoHeader.biWidth * 4;
			break;
	}

	return ( ( rowBytes + 3 ) & ~3 );
}

int BMPFile::SizeInBytes()
{
	//  Return the size of the image, taking into account
	//  that rows must be DWORD aligned.
	return BytesPerRow() * bmpInfoHeader.biHeight;
}

Image *BMPFile::Load( const char *pFilename )
{
	assert( pFilename != NULL );
	
	FILE *	pFile = NULL;
	Image *	pImage = NULL;
	bool 	bOS2BMP = false;
	bool	bTopDown = false;

	//  Open the filename for reading.  If this fails,
	//  return NULL
	pFile = fopen( pFilename, "rb" );
	if ( pFile == NULL )
		return NULL;

	//  Read the file header and make sure it's a BMP file.
	if ( fread( &bmpFileHeader, 1, sizeof( bmpFileHeader ), pFile ) != sizeof( bmpFileHeader ) )
		goto HandleError;
	if ( bmpFileHeader.bfType != BMP_MAGIC )
		goto HandleError;

	//  Read the bitmap info header.
	if ( fread( &bmpInfoHeader, 1, sizeof( bmpInfoHeader), pFile ) != sizeof( bmpInfoHeader ) )
		goto HandleError;

	//  If the size field is wrong, this is actually an OS/2 DIB.
	if ( bmpInfoHeader.biSize != sizeof( BITMAPINFOHEADER ) )
	{
		bOS2BMP = true;
		
		//  Back up to the point right after the file header.
		fseek( pFile, sizeof( BITMAPFILEHEADER ), SEEK_SET );

		//  Read the bitmap core header.
		BITMAPCOREHEADER	bmpCoreHeader;
		if ( fread( &bmpCoreHeader, 1, sizeof( bmpCoreHeader ), pFile ) != sizeof( bmpCoreHeader ) )
			goto HandleError;

		//  Convert the core header to a bitmap info header.
		memset( &bmpInfoHeader, 0, sizeof( bmpInfoHeader ) );
		bmpInfoHeader.biSize		= sizeof( BITMAPINFOHEADER );
		bmpInfoHeader.biWidth 		= ( int )bmpCoreHeader.bcWidth;
		bmpInfoHeader.biHeight 		= ( int )bmpCoreHeader.bcHeight;
		bmpInfoHeader.biPlanes 		= bmpCoreHeader.bcPlanes;
		bmpInfoHeader.biBitCount 	= bmpCoreHeader.bcBitCount;
		bmpInfoHeader.biCompression	= BI_RGB;
	}

	//  Decide whether the image is stored top-down or bottom-up.  If it's
	//  bottom-up, read the lines in reverse order.
	bTopDown = false;
	if ( bmpInfoHeader.biHeight < 0 )
	{
		bTopDown = true;
		bmpInfoHeader.biHeight = -bmpInfoHeader.biHeight;
	}

	//  Compute the size in bytes of the BMP file.
	bmpInfoHeader.biSizeImage = SizeInBytes();

	//  Read the color table if necessary.
	int colorCnt;
	colorCnt = 0;
	if ( bmpInfoHeader.biClrUsed != 0 )
		colorCnt = bmpInfoHeader.biClrUsed;
	else if ( bmpInfoHeader.biBitCount <= 8 )
		colorCnt = ( 1 << bmpInfoHeader.biBitCount );

	//  I don't know why this happens, but it does.
	if ( colorCnt > 256 )
		colorCnt = 256;

	if ( colorCnt != 0 )
	{
		if ( bOS2BMP == false )
		{
			//  If it's not an OS/2 BMP, the color table is colorCnt
			//  RGBQUADS.
			RGBQUAD	aRGBTable[ 256 ];
			size_t tableSize = colorCnt * sizeof( RGBQUAD );
			if ( fread( &aRGBTable[ 0 ], 1, tableSize, pFile ) != tableSize )
				goto HandleError;

			int colorIndex;
			for( colorIndex = 0; colorIndex < colorCnt; colorIndex++ )
			{
				aPalette[ colorIndex ].Set( 
					aRGBTable[ colorIndex ].rgbRed,
					aRGBTable[ colorIndex ].rgbGreen,
					aRGBTable[ colorIndex ].rgbBlue );
			}
		}
		else
		{
			//  If it is an OS/2 BMP, the color table is colorCnt
			//  RGBTRIPLES.
			RGBTRIPLE	aRGBTable[ 256 ];
			size_t tableSize = colorCnt * sizeof( RGBTRIPLE );
			if ( fread( &aRGBTable[ 0 ], 1, tableSize, pFile ) != tableSize )
				goto HandleError;

			int colorIndex;
			for( colorIndex = 0; colorIndex < colorCnt; colorIndex++ )
			{
				aPalette[ colorIndex ].Set( 
					aRGBTable[ colorIndex ].rgbtRed,
					aRGBTable[ colorIndex ].rgbtGreen,
					aRGBTable[ colorIndex ].rgbtBlue );
			}
		}
	}

	//  Create an image
	pImage = new Image( bmpInfoHeader.biWidth, bmpInfoHeader.biHeight, 24 );
	if ( pImage == NULL )
		goto HandleError;

	pImage->strFileName = pFilename;
	pImage->SetSkip( RGBColor( 255, 0, 255 ) );

	//  Read the actual pixel data.
	if ( ReadPixels( pImage, bTopDown, pFile ) == false )
		goto HandleError;

	( void )fclose( pFile );

	//  Return a pointer to the newly loaded bitmap.
	return pImage;

HandleError:;

	delete pImage;
	( void )fclose( pFile );

	//  Return NULL to indicate failure.
	return NULL;
}

bool BMPFile::ReadPixels( Image *pImage, bool bTopDown, FILE *pFile )
{
	assert( pImage != NULL );
	assert( pFile != NULL );

	//  If the BMP is not one of the supported formats, return FALSE
	//  ( failed to load ).
	if ( bmpInfoHeader.biBitCount != 8 &&
		bmpInfoHeader.biBitCount != 24 )
		return false;

	//  Get the width of the BMP file in bytes, and create a single-line read buffer.
	int bmpRowBytes = BytesPerRow();
	byte *pBMPBytes = new byte[ bmpRowBytes ];
	if ( pBMPBytes == NULL )
		return false;

	int startLine, endLine, incLine;
	if ( bTopDown == true )
	{
		startLine = 0;
		endLine = bmpInfoHeader.biHeight;
		incLine = 1;
	}
	else
	{
		startLine = bmpInfoHeader.biHeight - 1;
		endLine = -1;
		incLine = -1;
	}

	//  Load the pixels, translating to a 24-bit buffer on the fly.
	int row, col;
	switch( bmpInfoHeader.biBitCount )
	{
		default :
			//  If we get an unsupported bitdepth, assert in debug
			//  mode and quietly return in release mode - of course,
			//  this should be caught by the check above.
			assert( false );
			return false;
			break;

		case 8 :
			for( row = startLine; row != endLine; row += incLine )
			{
				byte *pBitmapBytes = pImage->_pPixels + row * pImage->_width * pImage->_bytesPerPixel;
				if ( fread( pBMPBytes, 1, bmpRowBytes, pFile ) != ( size_t )bmpRowBytes )
					return false;

 				byte *pCurrentBMPByte = pBMPBytes;
				for( col = 0; col < bmpInfoHeader.biWidth; col++ )
				{
					pBitmapBytes[ Image::RED_OFFSET ] = aPalette[ *pCurrentBMPByte ].Red();
					pBitmapBytes[ Image::GREEN_OFFSET ] = aPalette[ *pCurrentBMPByte ].Green();
					pBitmapBytes[ Image::BLUE_OFFSET ] = aPalette[ *pCurrentBMPByte ].Blue();
					pBitmapBytes += 3;
					pCurrentBMPByte++;
				}
			}
			break;

		case 24 :
			for( row = startLine; row != endLine; row += incLine )
			{
				byte *pBitmapBytes = pImage->_pPixels + row * pImage->_width * pImage->_bytesPerPixel;
				if ( fread( pBMPBytes, 1, bmpRowBytes, pFile ) != ( size_t )bmpRowBytes )
					return false;

 				byte *pCurrentBMPByte = pBMPBytes;
				for( col = 0; col < bmpInfoHeader.biWidth; col++ )
				{
					pBitmapBytes[ Image::RED_OFFSET ] = *( pCurrentBMPByte + 2 );
					pBitmapBytes[ Image::GREEN_OFFSET ] = *( pCurrentBMPByte + 1 );
					pBitmapBytes[ Image::BLUE_OFFSET ] = *pCurrentBMPByte;
					pBitmapBytes += 3;
					pCurrentBMPByte += 3;
				}
			}
			break;
	}

	delete[] pBMPBytes;

	return true;
}

bool BMPFile::Save( const char *pFilename, const Image *pImage )
{
	assert( pFilename != NULL );
	assert( pImage != NULL );
	//  Currently only saves 8 and 24-bit DIBs.
	assert( pImage->_bitDepth == 8 || pImage->_bitDepth == 24 );

	size_t bytesPerRow = 0;
	byte *pRowBuffer = NULL;

	//  Open the filename for writing.  If this fails,
	//  return NULL
	FILE *pFile = fopen( pFilename, "wb" );
	if ( pFile == NULL )
		return false;

	//  Write the bitmap file header.
	memset( &bmpFileHeader, 0, sizeof( bmpFileHeader ) );
	bmpFileHeader.bfType = BMP_MAGIC;
	bmpFileHeader.bfSize = sizeof( BITMAPFILEHEADER ) + 
		sizeof( BITMAPINFOHEADER ) +
		DW_ALIGN( pImage->_bytesPerPixel * pImage->_width ) * pImage->_height +
		( pImage->_bitDepth == 8 ? ( 256 * sizeof( RGBQUAD ) ) : 0 );
	bmpFileHeader.bfOffBits = sizeof (BITMAPFILEHEADER) + sizeof( BITMAPINFOHEADER ) + 
		( pImage->_bitDepth == 8 ? ( 256 * sizeof( RGBQUAD ) ) : 0 );
	if ( fwrite( &bmpFileHeader, 1, sizeof( bmpFileHeader ), pFile ) != sizeof( bmpFileHeader ) )
		goto HandleError;

	//  Write the bitmap info header.
	memset( &bmpInfoHeader, 0, sizeof( bmpInfoHeader ) );
	bmpInfoHeader.biSize = sizeof( BITMAPINFOHEADER );
	bmpInfoHeader.biWidth = pImage->_width;
	bmpInfoHeader.biHeight = pImage->_height;
	bmpInfoHeader.biPlanes = 1;
	bmpInfoHeader.biSizeImage = DW_ALIGN( pImage->_bytesPerPixel * pImage->_width ) * pImage->_height;
	switch( pImage->_bitDepth )
	{
		default :
			assert( false );
			goto HandleError;

		case 8 :
			bmpInfoHeader.biBitCount = 8;
			bmpInfoHeader.biCompression = BI_RGB;
			bmpInfoHeader.biClrUsed = 256;
			break;

		case 24 :
			bmpInfoHeader.biBitCount = 24;
			bmpInfoHeader.biCompression = BI_RGB;
			break;
	}

	if ( fwrite( &bmpInfoHeader, 1, sizeof( bmpInfoHeader), pFile ) != sizeof( bmpInfoHeader ) )
		goto HandleError;

	//  Write the color table.
	if ( pImage->_bitDepth == 8 )
	{
		assert( pImage->_pPalette != NULL );

		RGBQUAD	aRGBQPalette[ 256 ];
		int palIndex;
		for( palIndex = 0; palIndex < 256; palIndex++ )
		{
			aRGBQPalette[ palIndex ].rgbRed = pImage->_pPalette[ palIndex ].Red();
			aRGBQPalette[ palIndex ].rgbGreen = pImage->_pPalette[ palIndex ].Green();
			aRGBQPalette[ palIndex ].rgbBlue = pImage->_pPalette[ palIndex ].Blue();
		}
		if ( fwrite( aRGBQPalette, 1, sizeof( aRGBQPalette ), pFile ) != sizeof( aRGBQPalette ) )
			goto HandleError;
	}

	//  The output must be dword-aligned in bytes.  Allocate a buffer of the proper
	//  size.
	bytesPerRow = BytesPerRow();
	pRowBuffer = new byte[ bytesPerRow ];
	if ( pRowBuffer == NULL )
		goto HandleError;

	//  Write the image pixels, reversing the order of the row in the process.
	int row;
	for( row = pImage->_height - 1; row >= 0; row-- )
	{
		byte *pBytes = pImage->_pPixels + row * pImage->_width * pImage->_bytesPerPixel;
		( void )memcpy( pRowBuffer, pBytes, pImage->_width * pImage->_bytesPerPixel );
		if ( fwrite( pRowBuffer, 1, bytesPerRow, pFile ) != bytesPerRow )
			goto HandleError;
	}

	delete[] pRowBuffer;

	//  All done.  Close the file and 
	//  return success.
	fclose( pFile );
	return true;

HandleError:;
	fclose( pFile );
	delete[] pRowBuffer;
	return false;
}

RGBColor *BMPFile::LoadPalette( const char *pFilename )
{
	assert( pFilename != NULL );
	
	FILE *		pFile = NULL;
	RGBColor *	pPalette = NULL;
	bool 		bOS2BMP = false;

	//  Open the filename for reading.  If this fails,
	//  return NULL
	pFile = fopen( pFilename, "rb" );
	if ( pFile == NULL )
		return NULL;

	//  Read the file header and make sure it's a BMP file.
	if ( fread( &bmpFileHeader, 1, sizeof( bmpFileHeader ), pFile ) != sizeof( bmpFileHeader ) )
		goto HandleError;
	if ( bmpFileHeader.bfType != BMP_MAGIC )
		goto HandleError;

	//  Read the bitmap info header.
	if ( fread( &bmpInfoHeader, 1, sizeof( bmpInfoHeader), pFile ) != sizeof( bmpInfoHeader ) )
		goto HandleError;

	//  If the size field is wrong, this is actually an OS/2 DIB.
	if ( bmpInfoHeader.biSize != sizeof( BITMAPINFOHEADER ) )
	{
		bOS2BMP = true;
		
		//  Back up to the point right after the file header.
		fseek( pFile, sizeof( BITMAPFILEHEADER ), SEEK_SET );

		//  Read the bitmap core header.
		BITMAPCOREHEADER	bmpCoreHeader;
		if ( fread( &bmpCoreHeader, 1, sizeof( bmpCoreHeader ), pFile ) != sizeof( bmpCoreHeader ) )
			goto HandleError;

		//  Convert the core header to a bitmap info header.
		memset( &bmpInfoHeader, 0, sizeof( bmpInfoHeader ) );
		bmpInfoHeader.biSize		= sizeof( BITMAPINFOHEADER );
		bmpInfoHeader.biWidth 		= ( int )bmpCoreHeader.bcWidth;
		bmpInfoHeader.biHeight 		= ( int )bmpCoreHeader.bcHeight;
		bmpInfoHeader.biPlanes 		= bmpCoreHeader.bcPlanes;
		bmpInfoHeader.biBitCount 	= bmpCoreHeader.bcBitCount;
		bmpInfoHeader.biCompression	= BI_RGB;
	}

	//  Read the color table.  If there is no color table, we have
	//  a problem.
	int colorCnt;
	colorCnt = 0;
	if ( bmpInfoHeader.biClrUsed != 0 )
		colorCnt = bmpInfoHeader.biClrUsed;
	else if ( bmpInfoHeader.biBitCount <= 8 )
		colorCnt = ( 1 << bmpInfoHeader.biBitCount );
	else
		goto HandleError;

	//  I don't know why this happens, but it does.
	if ( colorCnt > 256 )
		colorCnt = 256;

	pPalette = new RGBColor[ 256 ];
	if ( pPalette == NULL )
		goto HandleError;

	if ( bOS2BMP == false )
	{
		//  If it's not an OS/2 BMP, the color table is colorCnt
		//  RGBQUADS.
		RGBQUAD	aRGBTable[ 256 ];
		size_t tableSize = colorCnt * sizeof( RGBQUAD );
		if ( fread( &aRGBTable[ 0 ], 1, tableSize, pFile ) != tableSize )
			goto HandleError;

		int colorIndex;
		for( colorIndex = 0; colorIndex < colorCnt; colorIndex++ )
		{
			pPalette[ colorIndex ].Set( 
				aRGBTable[ colorIndex ].rgbRed,
				aRGBTable[ colorIndex ].rgbGreen,
				aRGBTable[ colorIndex ].rgbBlue );
		}
	}
	else
	{
		//  If it is an OS/2 BMP, the color table is colorCnt
		//  RGBTRIPLES.
		RGBTRIPLE	aRGBTable[ 256 ];
		size_t tableSize = colorCnt * sizeof( RGBTRIPLE );
		if ( fread( &aRGBTable[ 0 ], 1, tableSize, pFile ) != tableSize )
			goto HandleError;

		int colorIndex;
		for( colorIndex = 0; colorIndex < colorCnt; colorIndex++ )
		{
			pPalette[ colorIndex ].Set( 
				aRGBTable[ colorIndex ].rgbtRed,
				aRGBTable[ colorIndex ].rgbtGreen,
				aRGBTable[ colorIndex ].rgbtBlue );
		}
	}

	//  All done.  Close the file and return the palette.
	fclose( pFile );

	return pPalette;

HandleError:;
	//  If we get here, there's an error.  Close the file,
	//  delete the palette, and return NULL.
	fclose( pFile );
	delete[] pPalette;

	return NULL;
}

bool BMPFile::SavePalette( const RGBColor *pPalette, const char *pFilename )
{
	assert( pPalette != NULL );
	assert( pFilename != NULL );
	
	//  Open the filename for writing.  If this fails,
	//  return NULL
	FILE *pFile = fopen( pFilename, "wb" );
	if ( pFile == NULL )
		return false;

	//  Write the bitmap file header.
	memset( &bmpFileHeader, 0, sizeof( bmpFileHeader ) );
	bmpFileHeader.bfType = BMP_MAGIC;
	bmpFileHeader.bfSize = sizeof( BITMAPFILEHEADER ) + 
		sizeof( BITMAPINFOHEADER ) +
		256 * sizeof( RGBQUAD ) + 256;
	if ( fwrite( &bmpFileHeader, 1, sizeof( bmpFileHeader ), pFile ) != sizeof( bmpFileHeader ) )
		goto HandleError;

	//  Write the bitmap info header.
	memset( &bmpInfoHeader, 0, sizeof( bmpInfoHeader ) );
	bmpInfoHeader.biSize = sizeof( BITMAPINFOHEADER );
	bmpInfoHeader.biWidth = 256;
	bmpInfoHeader.biHeight = 1;
	bmpInfoHeader.biPlanes = 1;
	bmpInfoHeader.biBitCount = 8;
	bmpInfoHeader.biCompression = BI_RGB;
	bmpInfoHeader.biSizeImage = 256 * 1;
	bmpInfoHeader.biClrUsed = 256;
	if ( fwrite( &bmpInfoHeader, 1, sizeof( bmpInfoHeader), pFile ) != sizeof( bmpInfoHeader ) )
		goto HandleError;

	//  Write the color table.
	RGBQUAD	aRGBQPalette[ 256 ];
	int palIndex;
	for( palIndex = 0; palIndex < 256; palIndex++ )
	{
		aRGBQPalette[ palIndex ].rgbRed = pPalette[ palIndex ].Red();
		aRGBQPalette[ palIndex ].rgbGreen = pPalette[ palIndex ].Green();
		aRGBQPalette[ palIndex ].rgbBlue = pPalette[ palIndex ].Blue();
	}
	if ( fwrite( aRGBQPalette, 1, sizeof( aRGBQPalette ), pFile ) != sizeof( aRGBQPalette ) )
		goto HandleError;

	//  Write out 256 pixels, one for each
	//  palette index.
	byte	aIndices[ 256 ];
	for( palIndex = 0; palIndex < 256; palIndex++ )
		aIndices[ palIndex ] = ( byte )palIndex;
	if ( fwrite( aIndices, 1, sizeof( aIndices ), pFile ) != sizeof( aIndices ) )
		goto HandleError;

	//  All done.  Close the file and 
	//  return success.
	fclose( pFile );
	return true;

HandleError:;
	fclose( pFile );
	return false;
}
