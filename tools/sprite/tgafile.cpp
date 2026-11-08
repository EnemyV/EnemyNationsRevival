#include "stdafx.h"

#include "tgafile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma pack( 1 )
struct TGAFILEHEADER
{
	BYTE	identifierSize;
	BYTE	colorMapType;
	BYTE	imageType;
	WORD	colorMapStart, colorMapEnd;
	BYTE	colorMapSize;
	WORD	xOrigin;
	WORD	yOrigin;
	WORD	width;
	WORD	height;
	BYTE	bitDepth;
	BYTE	flags;
};
#pragma pack()

//  Possible values of flags byte.  There
//  are more, but these are all we care about.
#define TOP_DOWN	0x20
#define RIGHT_LEFT	0x10

BOOL IsTrans ( BYTE red, BYTE green, BYTE blue )
{

	if (green > 50)
		return ( FALSE );

	if (red < 205)
		return ( FALSE );

	int iDif = abs ( red - blue );
	if ( iDif > 20 )
		return ( FALSE );

	if ( 2 * iDif + green > 60 )
		return ( FALSE );
		
	return (TRUE);
}

Image *TGAFile::Load( const char *pFilename )
{
	assert( pFilename != NULL );

	Image *	pImage = NULL;
	FILE *	pFile = NULL;
	byte *	pScanline = NULL;
	int		colorBytes;
	int		scanlineLength;

	//  Open the filename for reading.  If this fails,
	//  return NULL.
	pFile = fopen( pFilename, "rb" );
	if ( pFile == NULL )
		return NULL;

	//  Read the file header.  Unfortunately, there's no
	//  signature, so we just have to go with it and hope
	//  it's right.
	TGAFILEHEADER tgaFileHeader;
	if ( fread( &tgaFileHeader, 1, sizeof( tgaFileHeader ), pFile ) != sizeof( tgaFileHeader ) )
		goto HandleError;

	//  Make sure it's a type 2 file, which is the raw RGB ( or RGBA )
	//  or a type 10, which is compressed RGB or RGBA.
	if ( tgaFileHeader.imageType != 2  &&
		tgaFileHeader.imageType != 10 )
		goto HandleError;

	//  Make sure that it's either a 24 or 32 bit image.
	if ( tgaFileHeader.bitDepth != 24 && tgaFileHeader.bitDepth != 32 )
		goto HandleError;

	//  Create the image.
	pImage = new Image( tgaFileHeader.width, tgaFileHeader.height, 24 );
	if ( pImage == NULL )
		goto HandleError;

	pImage->strFileName = pFilename;

	pImage->SetSkip( RGBColor( 255, 0, 255 ) );

	//  Create a scan line buffer.
	colorBytes = tgaFileHeader.bitDepth / 8;
	scanlineLength = tgaFileHeader.width * colorBytes;
	pScanline = new byte[ scanlineLength ];
	if ( pScanline == NULL )
		goto HandleError;

	int startRow, endRow, incRow;
	if ( tgaFileHeader.flags & TOP_DOWN )
	{
		startRow = 0;
		endRow = tgaFileHeader.height;
		incRow = 1;
	}
	else
	{
		startRow = tgaFileHeader.height - 1;
		endRow = -1;
		incRow = -1;
	}

	int startCol, endCol, incCol;
	if ( tgaFileHeader.flags & RIGHT_LEFT )
	{
		startCol = tgaFileHeader.width - 1;
		endCol = -1;
		incCol = -1;
	}
	else
	{
		startCol = 0;
		endCol = tgaFileHeader.width;
		incCol = 1;
	}

	//  Skip the identifier.
	fseek( pFile, tgaFileHeader.identifierSize, SEEK_CUR );

	if ( tgaFileHeader.imageType == 2 )
	{
		int row;
		for( row = startRow; row != endRow; row += incRow )
		{
			byte *pRow = pImage->_pPixels + row * pImage->_width * pImage->_bytesPerPixel;
			
			//  Read a scanline.
			if ( fread( pScanline, 1, scanlineLength, pFile ) != ( size_t )scanlineLength )
				goto HandleError;

			int col;
			for( col = startCol; col != endCol; col += incCol )
			{
				byte blue = pScanline[ col * colorBytes ];
				byte green = pScanline[ col * colorBytes + 1 ];
				byte red = pScanline[ col * colorBytes + 2 ];

				/* GG 8/8/96
				if ( colorBytes == 4 )
				{
					byte alpha = pScanline[ col * colorBytes + 3 ];

					if ( alpha )
					{
						red = 255;
						green = 0;
						blue = 255;
					}
				}
				*/

				// see if transparent
				if ( IsTrans ( red, green, blue ) )
					{
					red = 255;
					green = 0;
					blue = 255;
					}

				pRow[ col * pImage->_bytesPerPixel + Image::RED_OFFSET ] = red;
				pRow[ col * pImage->_bytesPerPixel + Image::GREEN_OFFSET ] = green;
				pRow[ col * pImage->_bytesPerPixel + Image::BLUE_OFFSET ] = blue;
			}
		}
	}
	else
	{
		int row;
		for( row = startRow; row != endRow; row += incRow )
		{
			byte *pRow = pImage->_pPixels + row * pImage->_width * pImage->_bytesPerPixel;

			int col = startCol;
			while( col != endCol )
			{
				int tag = fgetc( pFile );
				if ( tag == EOF )
					goto HandleError;

				if ( tag & 0x80 )
				{
					int cnt = ( tag & 0x7F ) + 1;

					byte	bgra[ 4 ];
					if ( fread( bgra, 1, colorBytes, pFile ) != ( size_t )colorBytes )
						goto HandleError;

					byte red = bgra[ 2 ];
					byte green = bgra[ 1 ];
					byte blue = bgra[ 0 ];

					/* GG 9/7/96
					if ( colorBytes == 4 )
					{
						byte alpha = bgra[ 3 ];
						if ( alpha )
						{
							red = 255;
							green = 0;
							blue = 255;
						}
					}
					*/

					// see if transparent
					if ( IsTrans ( red, green, blue ) )
						{
						red = 255;
						green = 0;
						blue = 255;
						}

					while( cnt-- )
					{
						pRow[ col * pImage->_bytesPerPixel + Image::RED_OFFSET ] = red;
						pRow[ col * pImage->_bytesPerPixel + Image::GREEN_OFFSET ] = green;
						pRow[ col * pImage->_bytesPerPixel + Image::BLUE_OFFSET ] = blue;
						col += incCol;
					}
				}
				else
				{
					int cnt = ( tag & 0x7F ) + 1;

					if ( fread( pScanline, 1, cnt * colorBytes, pFile ) != ( size_t )( cnt * colorBytes ) )
						goto HandleError;

					int byteIndex = 0;
					while( cnt-- )
					{
						byte red = pScanline[ byteIndex + 2 ];
						byte green = pScanline[ byteIndex + 1 ];
						byte blue = pScanline[ byteIndex + 0 ];

						/* GG 9/8/96
						if ( colorBytes == 4 )
						{
							byte alpha = pScanline[ byteIndex + 3 ];
							if ( alpha )
							{
								red = 255;
								green = 0;
								blue = 255;
							}
						}
						*/

						// see if transparent
						if ( IsTrans ( red, green, blue ) )
							{
							red = 255;
							green = 0;
							blue = 255;
							}

						pRow[ col * pImage->_bytesPerPixel + Image::RED_OFFSET ] = red;
						pRow[ col * pImage->_bytesPerPixel + Image::GREEN_OFFSET ] = green;
						pRow[ col * pImage->_bytesPerPixel + Image::BLUE_OFFSET ] = blue;
						col += incCol;

						byteIndex += colorBytes;
					}
				}
			}
		}
	}

	delete[] pScanline;
	fclose( pFile );

	return pImage;

HandleError:;

	delete pImage;
	delete[] pScanline;
	fclose( pFile );

	return NULL;
}
