#include "stdafx.h"

#include "drawdib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma warning( disable: 4702 )	//  Getting this warning in CreateIdentPal in release mode?

DrawDIB::DrawDIB()
{
	_pPixels = NULL;
	hWinPalette = NULL;
}

DrawDIB::~DrawDIB()
{
	delete[] _pPixels;
	if ( hWinPalette )
		DeleteObject( hWinPalette );
}

bool DrawDIB::CreateIdentPal( RGBColor *pPalette )
{
	assert( pPalette != NULL );

	int i;
	struct {
		WORD Version;
		WORD NumberOfEntries;
		PALETTEENTRY aEntries[256];
	} Palette =
	{
		0x300,
		256
	};

	//*** Just use the screen DC where we need it
	HDC hdc = GetDC(NULL);

	//*** For SYSPAL_NOSTATIC, just copy the color table into
	//*** a PALETTEENTRY array and replace the first and last entries
	//*** with black and white
	if (GetSystemPaletteUse(hdc) == SYSPAL_NOSTATIC)

	{
		//*** Fill in the palette with the given values, marking each
		//*** as PC_NOCOLLAPSE
		for(i = 0; i < 256; i++)
		{
			Palette.aEntries[i].peRed = pPalette[i].Red();
			Palette.aEntries[i].peGreen = pPalette[i].Green();
			Palette.aEntries[i].peBlue = pPalette[i].Blue();
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;
		}

		//*** Mark any unused entries PC_NOCOLLAPSE
		for (; i < 256; ++i)
		{
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;

		}

		//*** Make sure the last entry is white
		//*** This may replace an entry in the array!
		Palette.aEntries[255].peRed = 255;
		Palette.aEntries[255].peGreen = 255;
		Palette.aEntries[255].peBlue = 255;
		Palette.aEntries[255].peFlags = 0;

		//*** And the first is black
		//*** This may replace an entry in the array!
		Palette.aEntries[0].peRed = 0;
		Palette.aEntries[0].peGreen = 0;
		Palette.aEntries[0].peBlue = 0;
		Palette.aEntries[0].peFlags = 0;

	}
	else
	//*** For SYSPAL_STATIC, get the twenty static colors into
	//*** the array, then fill in the empty spaces with the
	//*** given color table
	{
		int nStaticColors;
		int nUsableColors;

		//*** Get the static colors from the system palette
		nStaticColors = GetDeviceCaps(hdc, NUMCOLORS);
		GetSystemPaletteEntries(hdc, 0, 256, Palette.aEntries);

		//*** Set the peFlags of the lower static colors to zero
		nStaticColors = nStaticColors / 2;

		for (i=0; i<nStaticColors; i++)
			Palette.aEntries[i].peFlags = 0;

		//*** Fill in the entries from the given color table
		nUsableColors = 256 - nStaticColors;
		for (; i<nUsableColors; i++)
		{
			Palette.aEntries[i].peRed = pPalette[i].Red();
			Palette.aEntries[i].peGreen = pPalette[i].Green();
			Palette.aEntries[i].peBlue = pPalette[i].Blue();
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;
		}

		//*** Mark any empty entries as PC_NOCOLLAPSE

		for (; i<256 - nStaticColors; i++)
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;

		//*** Set the peFlags of the upper static colors to zero
		for (i = 256 - nStaticColors; i<256; i++)
			Palette.aEntries[i].peFlags = 0;
	}

	//*** Remember to release the DC!
	ReleaseDC(NULL, hdc);

	//*** Return the palette
	if ( hWinPalette )
		DeleteObject( hWinPalette );	//  Delete existing palette.
	hWinPalette = CreatePalette( ( LOGPALETTE * ) &Palette );
	if ( hWinPalette == NULL )
		return false;

	return true;
}

bool DrawDIB::Create( int width, int height, int bitDepth, RGBColor *pPal )
{
	assert( 0 < width );
	assert( 0 < height );
	assert( bitDepth == 8 || bitDepth == 15 || bitDepth == 16 || bitDepth == 24 || bitDepth == 32 );

	//  Save width, height, bit depth.
	_width = width;
	_height = height;
	_bitDepth = bitDepth;
	
	//  Compute stride.
	switch( _bitDepth )
	{
		default :
			assert( false );
			break;

		case 8 :
			_stride = ( _width + 3 ) & ~0x3;
			break;

		case 15 :
		case 16 :
			_stride = ( _width * 2 + 3 ) & ~0x3;
			break;

		case 24 :
			_stride = ( _width * 3 + 3 ) & ~0x3;
			break;

		case 32 :
			_stride = _width * 4;
			break;
	}

	//  Set up BITMAPINFO structure.
	( void )memset( &bi.bih, 0, sizeof( bi.bih ) );
	bi.bih.biSize = sizeof( BITMAPINFOHEADER );
	bi.bih.biWidth = _width;
	bi.bih.biHeight = -_height;	//  Negative for top-down.
	bi.bih.biPlanes = 1;
	bi.bih.biBitCount = ( WORD )_bitDepth;
	bi.bih.biSizeImage = _stride * _height;
	bi.bih.biCompression = BI_RGB;
	if ( _bitDepth == 32 )
	{
		bi.bih.biCompression = BI_BITFIELDS;
		bi.aRGBMasks[ 0 ] = 0x00FF0000;
		bi.aRGBMasks[ 1 ] = 0x0000FF00;
		bi.aRGBMasks[ 2 ] = 0x000000FF;
	}
	if ( _bitDepth == 16 )
	{
		bi.bih.biCompression = BI_BITFIELDS;
		bi.aRGBMasks[ 0 ] = 0x0000F800;
		bi.aRGBMasks[ 1 ] = 0x000007E0;
		bi.aRGBMasks[ 2 ] = 0x0000001F;
	}
	else if ( _bitDepth == 15 )
	{
		bi.bih.biBitCount = 16;
		bi.bih.biCompression = BI_BITFIELDS;
		bi.aRGBMasks[ 0 ] = 0x00007C00;
		bi.aRGBMasks[ 1 ] = 0x000003E0;
		bi.aRGBMasks[ 2 ] = 0x0000001F;
	}
	else if ( _bitDepth == 8 )
	{
		assert( pPal != NULL );
		
		int palIndex;
		for( palIndex = 0; palIndex < 256; palIndex++ )
		{
			bi.aPalette[ palIndex ].rgbRed = pPal[ palIndex ].Red();
			bi.aPalette[ palIndex ].rgbGreen = pPal[ palIndex ].Green();
			bi.aPalette[ palIndex ].rgbBlue = pPal[ palIndex ].Blue();
			bi.aPalette[ palIndex ].rgbReserved = 0;
		}

		if ( CreateIdentPal( pPal ) == false )
			return false;
	}

	delete[] _pPixels;	//  Delete any existing buffer.
	_pPixels = new byte[ _stride * _height ];
	if ( _pPixels == NULL )
		return false;

	return true;
}

void DrawDIB::Blit( HDC hDC )
{
	HPALETTE	hOldPalette = NULL;
	if ( _bitDepth == 8 )
	{
		hOldPalette = SelectPalette( hDC, hWinPalette, FALSE );
		RealizePalette( hDC );
	}

	StretchDIBits( hDC,  
		0, 0, _width, _height, 
		0, 0, _width, _height, 
		_pPixels, ( BITMAPINFO * )&bi, 
		DIB_RGB_COLORS, SRCCOPY );

	if ( _bitDepth == 8 )
		SelectPalette( hDC, hOldPalette, FALSE );
}
