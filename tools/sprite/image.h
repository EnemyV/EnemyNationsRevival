#ifndef IMAGE_H
#define IMAGE_H

#include "common.h"
#include "drawsurf.h"
#include "rgbcolor.h"

enum HOTSPOT_TYPE
{
	HOTSPOT_SMOKE_FIRE = 0,
	HOTSPOT_TURRET = 1,
	HOTSPOT_MUZZLE = 1,
	HOTSPOT_FLAG = 2,
	NUM_HOTSPOTS
};

struct THotSpot
{
	int	iType;		// 0 - smoke.flame, 1 - turret, 2 - flag
	int	iX;			// x coord relative to UL corner of view
	int	iY;			// y
	int	iIndex;		// The k value
};

extern char * g_apszHotSpotName[ NUM_HOTSPOTS ];

enum
{
	MAX_SMOKE_FIRE = 10,
	MAX_TURRETS = 10,
	MAX_FLAGS = 10, 
	MAX_ATTACHED_SPRITES = 30	//  Should be sum of smoke/fire, flags, and turrets
};

class Image
{
	private :

	dword		_skip32;
	RGBColor	_skip24;
	word 		_skip16;
	word 		_skip15;
	bool 		_skip8[ 256 ];
	byte		_skipIndex;
	BOOL		_bScannedForHotSpots;

	public :

	enum { RED_OFFSET = 2, GREEN_OFFSET = 1, BLUE_OFFSET = 0 };

	int			_width, _height;
	int			_xoff, _yoff;
	int			_holdTime;
	byte *		_pPixels;
	int			_bytesPerPixel;
	int			_bitDepth;
	RGBColor *	_pPalette;

	// GG
	int			nAttachedSpriteCnt;
	THotSpot	aAttachedSprites[ MAX_ATTACHED_SPRITES ];
	BOOL		bAnchorPointIsValid;
	THotSpot	anchorPoint;
	CString	strFileName;
	void 		ScanForHotSpots( BOOL bReportErrors, float fMul = 1.0 );
	//

	Image();
	Image( int width, int height, int bitDepth, RGBColor *pPalette = NULL );
	Image( const Image &oldImage );
	~Image();

	void	ResetHotSpots();						// GG
	void	CopyHotSpots( Image const * );	// GG

	bool Load( const char *pFilename );

	void SetSkip( RGBColor skip );
	inline bool IsTransparent( const byte *pPixel ) const;
	inline void SetTransparent( byte *pPixel ) const;
	
	void Render( int x, int y, DrawSurface *pDrawSurf ) const;

	bool Crop();
	bool ScaleUp ( float fMul );
	bool ScaleDown( int scale );
	bool ScaleDownAvg( int scale );

	bool ReduceTo15();
	bool ReduceTo16();
	bool IncreaseTo32();
};

inline bool Image::IsTransparent( const byte *pPixel ) const
{
	switch( _bitDepth )
	{
		default :
			assert( false );
			break;

		case 32 :

			if ( *( dword * )pPixel == _skip32 )
				return true;

			break;

		case 24 :

			if ( *( RGBColor * )pPixel == _skip24 )
				return true;

			break;

		case 16 :
			if ( *( word * )pPixel == _skip16 )
				return true;
			break;

		case 15 :
			if ( *( word * )pPixel == _skip15 )
				return true;
			break;

		case 8 :
			if ( _skip8[ *pPixel ] )
				return true;
			break;
	}

	return false;
}

inline void Image::SetTransparent( byte *pPixel ) const
{
	switch( _bitDepth )
	{
		default :
			assert( false );

		case 32 :
			*( dword * )pPixel = _skip32;
			break;

		case 24 :
			*( RGBColor * )pPixel = _skip24;
			break;

		case 16 :
			*( word * )pPixel = _skip16;
			break;

		case 15 :
			*( word * )pPixel = _skip15;
			break;

		case 8 :
			*pPixel = _skipIndex;
			break;
	}
}

#endif
