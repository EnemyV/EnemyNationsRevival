
#include "stdafx.h"
#include <assert.h>
#include "sp.h"
#include "codec.h"
#include "sprite.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int NUM_DAMAGE_LEVELS = 4;
const int NUM_ZOOM_LEVELS = 4;
const int NUM_BLDG_STAGES = 4;
const int NUM_BLDG_LAYERS = 3;
//const int NUM_BLDG_LAYERS = 2;
const int NUM_BLDG_DIRS   = 4;
const int NUM_BLDG_DAMAGES = NUM_DAMAGE_LEVELS;
const int NUM_VEHICLE_DAMAGES = NUM_DAMAGE_LEVELS;
const int NUM_TERRAIN_DAMAGES = NUM_DAMAGE_LEVELS;
const int NUM_VEHICLE_TILTS = 3;
const int NUM_VEHICLE_DIRS = 8;
const int NUM_TERRAIN_DIRS = 4;
const int NUM_COLOR_REZ_BYTES = 4;

enum { BUILDING = 4, VEHICLE = 5, TERRAIN = 1 }; 

static int  xiFirstZoom;
static BOOL xbReportErrors;

//------------------------- C S o u r c e B i t m a p -----------------------

class CSourceBitmap
{

public:

	CSourceBitmap();
   virtual ~CSourceBitmap();

	virtual BOOL	Read( FILE * fp );
	virtual int		GetSize( int iSrcBytesPerPixel, int iDstBytesPerPixel, int iType ) const;

	short	m_iID;
	short	m_iTime;
	short	m_iX;
	short	m_iY;
	short	m_iW;
	short	m_iH;
	int	m_iLen;

	BYTE * m_pbyPixels;
};

//------------------------ C S o u r c e B i t m a p s ----------------------

class CSourceBitmaps
{

public:

	virtual BOOL	Read( FILE * fp );

	CSourceBitmap	m_abitmap[ NUM_ZOOM_LEVELS ];
};

//----------------------- C S o u r c e H o t S p o t -----------------------

class CSourceHotSpot
{
	
public:

	virtual BOOL	Read( FILE * fp );

	int	m_iType;
	int	m_iX;
	int	m_iY;
	int	m_iIndex;
};

//-------------------------- C S o u r c e V i e w --------------------------

class CSourceView
{

public:

	CSourceView();
   virtual ~CSourceView();

	virtual BOOL	Read( FILE * fp );
	virtual int		GetDataSize( int iZoom ) const;
	virtual int		GetData( int iZoom, BYTE * ) const;

	int	m_iW;
	int	m_iH;
	int	m_iBitsPerPixel;
	int	m_iAnchorX;
	int	m_iAnchorY;
	int	m_nHotSpots;

	CSourceHotSpot	* m_photspots;

	int				  m_nBase;
	CSourceBitmaps	* m_pbitmapsBase;

	int				  m_nOverlay;
	CSourceBitmaps	* m_pbitmapsOverlay;

	int				  m_nFront1;
	CSourceBitmaps	* m_pbitmapsFront1;

	int				  m_nFront2;
	CSourceBitmaps	* m_pbitmapsFront2;

	int				  m_nBack1;
	CSourceBitmaps	* m_pbitmapsBack1;

	int				  m_nBack2;
	CSourceBitmaps	* m_pbitmapsBack2;
};

//------------------------- C S o u r c e S p r i t e -----------------------

class CSourceSprite
{

public:

   virtual ~CSourceSprite();

	virtual BOOL	Read( FILE * );

	int	m_iType;
	int	m_iW;
	int	m_iH;
	int	m_iBitsPerPixel;
	int	m_nViews;

	CSourceView	* m_pviews;
	int			* m_piViewIndices;

protected:

	CSourceSprite( int iType );
};

//------------------ C S o u r c e B u i l d i n g S p r i t e --------------

class CSourceBuildingSprite : public CSourceSprite
{

public:

	CSourceBuildingSprite() : CSourceSprite( BUILDING ) {}

	virtual BOOL	Read( FILE * );

	int	m_aaaaiViewIndices[ NUM_BLDG_STAGES ][ NUM_BLDG_LAYERS ][ NUM_BLDG_DIRS ][ NUM_BLDG_DAMAGES ];
};

//------------------ C S o u r c e V e h i c l e S p r i t e ----------------

class CSourceVehicleSprite : public CSourceSprite
{

public:

	CSourceVehicleSprite() : CSourceSprite( VEHICLE ) {}

	virtual BOOL	Read( FILE * );

	int	m_aaaiViewIndices[ NUM_VEHICLE_TILTS ][ NUM_VEHICLE_DIRS ][ NUM_VEHICLE_DAMAGES ];
};

//------------------ C S o u r c e T e r r a i n S p r i t e ----------------

class CSourceTerrainSprite : public CSourceSprite
{

public:

	CSourceTerrainSprite() : CSourceSprite( TERRAIN ) {}

	virtual BOOL	Read( FILE * );

	int	m_aaiViewIndices[ NUM_TERRAIN_DIRS ][ NUM_TERRAIN_DAMAGES ];
};

//---------------------------------------------------------------------------
// CSourceBitmap::Read
//---------------------------------------------------------------------------
BOOL
CSourceBitmap::Read(
	FILE *fp )
{
	if ( 1 != fread( &m_iID,   sizeof( m_iID   ), 1, fp ) ||
		  1 != fread( &m_iTime, sizeof( m_iTime ), 1, fp ) ||
		  1 != fread( &m_iX, 	sizeof( m_iX    ), 1, fp ) ||
		  1 != fread( &m_iY, 	sizeof( m_iY    ), 1, fp ) ||
		  1 != fread( &m_iW, 	sizeof( m_iW    ), 1, fp ) ||
		  1 != fread( &m_iH, 	sizeof( m_iH    ), 1, fp ) ||
		  1 != fread( &m_iLen,  sizeof( m_iLen  ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	ASSERT( 0 <= m_iTime && m_iTime <= 240 );
	ASSERT( -1024 < m_iX && m_iX < 1024 );
	ASSERT( -1024 < m_iY && m_iY < 1024 );
	ASSERT(  0 <= m_iW && m_iW < 1024 );
	ASSERT(  0 <= m_iH && m_iH < 1024 );
	ASSERT(  0 <= m_iLen && m_iLen < 1024 * 1024 * 4 );

	ASSERT( NULL == m_pbyPixels );

	if ( m_iLen )
	{
		m_pbyPixels = new BYTE [ m_iLen ];

		return 1 == fread( m_pbyPixels, m_iLen, 1, fp );
	}

	return TRUE;
}

//---------------------------------------------------------------------------
// CSourceBitmap::GetSize
//---------------------------------------------------------------------------
int
CSourceBitmap::GetSize(
	int	iSrcBytesPerPixel,
	int	iDstBytesPerPixel,
	int	iType ) const
{
	int	iSize = 0;

	switch ( iType )
	{
		case TERRAIN:	iSize = iDstBytesPerPixel * m_iW * m_iH * 8; 
							break;

		case VEHICLE:	iSize = iDstBytesPerPixel * m_iW * m_iH; 
							break;

		case BUILDING:
		{
			long			 lCount;
			BYTE const	*pby = m_pbyPixels;

			if ( !pby )
				break;

			iSize = ( 2 + m_iH ) * sizeof( int );
			pby  += iSize;

			for ( int iY = 0; iY < m_iH; ++iY )
				for ( ;; )
				{
					iSize += 2 * sizeof( long );
					pby   += sizeof( long );

					lCount = *( long * )pby;

					pby += sizeof( long );

					if ( 0 == lCount )
						break;

					iSize += iDstBytesPerPixel * lCount / iSrcBytesPerPixel;

					pby += lCount;
				}
		}

		break;
	}

	ASSERT( iSrcBytesPerPixel != iDstBytesPerPixel || iSize == m_iLen );

	return iSize;
}

//---------------------------------------------------------------------------
// CSourceBitmap::CSourceBitmap
//---------------------------------------------------------------------------
CSourceBitmap::CSourceBitmap()
	:
		m_pbyPixels( NULL ),
		m_iID		  ( 0 ),
		m_iTime	  ( 0 ),
		m_iX		  ( 0 ),
		m_iY		  ( 0 ),
		m_iW		  ( 0 ),
		m_iH		  ( 0 ),
		m_iLen	  ( 0 )
{
}

//---------------------------------------------------------------------------
// CSourceBitmap::~CSourceBitmap
//---------------------------------------------------------------------------
CSourceBitmap::~CSourceBitmap()
{
	delete m_pbyPixels;
}

//---------------------------------------------------------------------------
// CSourceHotSpot::Read
//---------------------------------------------------------------------------
BOOL
CSourceHotSpot::Read(
	FILE *fp )
{
	if ( 1 != fread( &m_iType,  sizeof( m_iType  ), 1, fp ) ||
		  1 != fread( &m_iX, 	 sizeof( m_iX     ), 1, fp ) ||
		  1 != fread( &m_iY, 	 sizeof( m_iY     ), 1, fp ) ||
		  1 != fread( &m_iIndex, sizeof( m_iIndex ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	ASSERT( 0 <= m_iType && m_iType < NUM_HOTSPOTS );

	ASSERT( -1024 < m_iX && m_iX < 1024 );
	ASSERT( -1024 < m_iY && m_iY < 1024 );
	ASSERT( 0 <= m_iIndex && m_iIndex < 32 );

	return TRUE;
}

//---------------------------------------------------------------------------
// CSourceView::CSourceView
//---------------------------------------------------------------------------
CSourceView::CSourceView()
	:
		m_photspots      ( NULL ),
		m_pbitmapsBase   ( NULL ),
		m_pbitmapsOverlay( NULL ),
		m_pbitmapsFront1 ( NULL ),
		m_pbitmapsFront2 ( NULL ),
		m_pbitmapsBack1  ( NULL ),
		m_pbitmapsBack2  ( NULL )
{
}

//---------------------------------------------------------------------------
// CSourceView::~CSourceView
//---------------------------------------------------------------------------
CSourceView::~CSourceView()
{
	delete [] m_photspots;         
	delete [] m_pbitmapsBase;      
	delete [] m_pbitmapsOverlay;   
	delete [] m_pbitmapsFront1;
	delete [] m_pbitmapsFront2;
	delete [] m_pbitmapsBack1; 
	delete [] m_pbitmapsBack2; 
}

//---------------------------------------------------------------------------
// CSourceView::Read
//---------------------------------------------------------------------------
BOOL
CSourceView::Read(
	FILE *fp )
{
	if ( 1 != fread( &m_iW, 			  sizeof( m_iW ),				  1, fp ) ||
		  1 != fread( &m_iH, 			  sizeof( m_iH ),				  1, fp ) ||
		  1 != fread( &m_iBitsPerPixel, sizeof( m_iBitsPerPixel ), 1, fp ) ||
		  1 != fread( &m_iAnchorX, 	  sizeof( m_iAnchorX ),		  1, fp ) ||
		  1 != fread( &m_iAnchorY, 	  sizeof( m_iAnchorY ),		  1, fp ) ||
		  1 != fread( &m_nHotSpots, 	  sizeof( m_nHotSpots ),	  1, fp ))
	{
		TRAP();
		return FALSE;
	}

	ASSERT( 0 <= m_iW && m_iW < 1024 );
	ASSERT( 0 <= m_iH && m_iH < 1024 );
	ASSERT( 8  == m_iBitsPerPixel ||
			  15 == m_iBitsPerPixel ||	
			  16 == m_iBitsPerPixel ||	
			  24 == m_iBitsPerPixel ||	
			  32 == m_iBitsPerPixel );
	ASSERT( -1024 < m_iAnchorX && m_iAnchorX < 1024 );
	ASSERT( -1024 < m_iAnchorY && m_iAnchorY < 1024 );
	ASSERT( 0 <= m_nHotSpots && m_nHotSpots < 1000 );

	if ( m_nHotSpots )
		m_photspots = new CSourceHotSpot [ m_nHotSpots ];

	int	i;

	for ( i = 0; i < m_nHotSpots; ++i )
		if ( !m_photspots[i].Read( fp ))
		{
			TRAP();
			return FALSE;
		}

	if ( 1 != fread( &m_nBase, sizeof( m_nBase ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	ASSERT( 0 <= m_nBase && m_nBase < 10 );

	if ( m_nBase )
		m_pbitmapsBase = new CSourceBitmaps [ m_nBase ];

	for ( i = 0; i < m_nBase; ++i )
		if ( !m_pbitmapsBase[i].Read( fp ))
		{
			TRAP();
			return FALSE;
		}

	if ( 1 != fread( &m_nOverlay, sizeof( m_nOverlay ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	ASSERT( 0 <= m_nOverlay && m_nOverlay < 10 );

	if ( m_nOverlay )
		m_pbitmapsOverlay = new CSourceBitmaps [ m_nOverlay ];

	for ( i = 0; i < m_nOverlay; ++i )
		if ( !m_pbitmapsOverlay[i].Read( fp ))
		{
			TRAP();
			return FALSE;
		}

	if ( 1 != fread( &m_nFront1, sizeof( m_nFront1 ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	ASSERT( 0 <= m_nFront1 && m_nFront1 < 27 );

	if ( m_nFront1 )
		m_pbitmapsFront1 = new CSourceBitmaps [ m_nFront1 ];

	for ( i = 0; i < m_nFront1; ++i )
		if ( !m_pbitmapsFront1[i].Read( fp ))
		{
			TRAP();
			return FALSE;
		}

	if ( 1 != fread( &m_nFront2, sizeof( m_nFront2 ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	ASSERT( 0 <= m_nFront2 && m_nFront2 < 27 );

	if ( m_nFront2 )
		m_pbitmapsFront2 = new CSourceBitmaps [ m_nFront2 ];

	for ( i = 0; i < m_nFront2; ++i )
		if ( !m_pbitmapsFront2[i].Read( fp ))
		{
			TRAP();
			return FALSE;
		}

	if ( 1 != fread( &m_nBack1, sizeof( m_nBack1 ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	ASSERT( 0 <= m_nBack1 && m_nBack1 < 27 );

	if ( m_nBack1 )
		m_pbitmapsBack1 = new CSourceBitmaps [ m_nBack1 ];

	for ( i = 0; i < m_nBack1; ++i )
		if ( !m_pbitmapsBack1[i].Read( fp ))
		{
			TRAP();
			return FALSE;
		}

	if ( 1 != fread( &m_nBack2, sizeof( m_nBack2 ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	ASSERT( 0 <= m_nBack2 && m_nBack2 < 27 );

	if ( m_nBack2 )
		m_pbitmapsBack2 = new CSourceBitmaps [ m_nBack2 ];

	for ( i = 0; i < m_nBack2; ++i )
		if ( !m_pbitmapsBack2[i].Read( fp ))
		{
			TRAP();
			return FALSE;
		}

	return TRUE;
}

//---------------------------------------------------------------------------
// CSourceView::GetDataSize
//---------------------------------------------------------------------------
int
CSourceView::GetDataSize(
	int	iZoom ) const
{
	int	iSize = 0;
	int	i;

	for ( i = 0; i < m_nBase; ++i )
		iSize += m_pbitmapsBase[i].m_abitmap[ iZoom ].m_iLen;

	for ( i = 0; i < m_nOverlay; ++i )
		iSize += m_pbitmapsOverlay[i].m_abitmap[ iZoom ].m_iLen;

	for ( i = 0; i < m_nFront1; ++i )
		iSize += m_pbitmapsFront1[i].m_abitmap[ iZoom ].m_iLen;

	for ( i = 0; i < m_nFront2; ++i )
		iSize += m_pbitmapsFront2[i].m_abitmap[ iZoom ].m_iLen;

	for ( i = 0; i < m_nBack1; ++i )
		iSize += m_pbitmapsBack1[i].m_abitmap[ iZoom ].m_iLen;

	for ( i = 0; i < m_nBack2; ++i )
		iSize += m_pbitmapsBack2[i].m_abitmap[ iZoom ].m_iLen;

	return iSize;
}

//---------------------------------------------------------------------------
// CSourceView::GetData
//---------------------------------------------------------------------------
int
CSourceView::GetData(
	int    iZoom,
	BYTE * pby ) const
{
	int	iSize		  = 0;
	int	iTotalSize = 0;
	int	i;

	for ( i = 0; i < m_nBase; ++i )
	{
		iSize = m_pbitmapsBase[i].m_abitmap[ iZoom ].m_iLen;

		ASSERT( 0 == iSize ^ NULL != m_pbitmapsBase[i].m_abitmap[ iZoom ].m_pbyPixels );

		if ( iSize )
			memcpy( pby, m_pbitmapsBase[i].m_abitmap[ iZoom ].m_pbyPixels, iSize );

		iTotalSize += iSize;
		pby 		  += iSize;
	}

	for ( i = 0; i < m_nOverlay; ++i )
	{
		iSize = m_pbitmapsOverlay[i].m_abitmap[ iZoom ].m_iLen;

		ASSERT( 0 == iSize ^ NULL != m_pbitmapsOverlay[i].m_abitmap[ iZoom ].m_pbyPixels );

		if ( iSize )
			memcpy( pby, m_pbitmapsOverlay[i].m_abitmap[ iZoom ].m_pbyPixels, iSize );

		iTotalSize += iSize;
		pby 		  += iSize;
	}

	for ( i = 0; i < m_nFront1; ++i )
	{
		iSize = m_pbitmapsFront1[i].m_abitmap[ iZoom ].m_iLen;

		ASSERT( 0 == iSize ^ NULL != m_pbitmapsFront1[i].m_abitmap[ iZoom ].m_pbyPixels );

		if ( iSize )
			memcpy( pby, m_pbitmapsFront1[i].m_abitmap[ iZoom ].m_pbyPixels, iSize );

		iTotalSize += iSize;
		pby 		  += iSize;
	}

	for ( i = 0; i < m_nFront2; ++i )
	{
		iSize = m_pbitmapsFront2[i].m_abitmap[ iZoom ].m_iLen;

		ASSERT( 0 == iSize ^ NULL != m_pbitmapsFront2[i].m_abitmap[ iZoom ].m_pbyPixels );

		if ( iSize )
			memcpy( pby, m_pbitmapsFront2[i].m_abitmap[ iZoom ].m_pbyPixels, iSize );

		iTotalSize += iSize;
		pby 		  += iSize;
	}

	for ( i = 0; i < m_nBack1; ++i )
	{
		iSize = m_pbitmapsBack1[i].m_abitmap[ iZoom ].m_iLen;

		ASSERT( 0 == iSize ^ NULL != m_pbitmapsBack1[i].m_abitmap[ iZoom ].m_pbyPixels );

		if ( iSize )
			memcpy( pby, m_pbitmapsBack1[i].m_abitmap[ iZoom ].m_pbyPixels, iSize );

		iTotalSize += iSize;
		pby 		  += iSize;
	}

	for ( i = 0; i < m_nBack2; ++i )
	{
		iSize = m_pbitmapsBack2[i].m_abitmap[ iZoom ].m_iLen;

		ASSERT( 0 == iSize ^ NULL != m_pbitmapsBack2[i].m_abitmap[ iZoom ].m_pbyPixels );

		if ( iSize )
			memcpy( pby, m_pbitmapsBack2[i].m_abitmap[ iZoom ].m_pbyPixels, iSize );

		iTotalSize += iSize;
		pby 		  += iSize;
	}

	return iTotalSize;
}

//---------------------------------------------------------------------------
// CSourceBitmaps::Read
//---------------------------------------------------------------------------
BOOL
CSourceBitmaps::Read(
	FILE *fp )
{
	for ( int i = xiFirstZoom; i < NUM_ZOOM_LEVELS; ++i )
		if ( !m_abitmap[i].Read( fp ))
		{
			TRAP();
			return FALSE;
		}

	return TRUE;
}

//---------------------------------------------------------------------------
// CSourceSprite::CSourceSprite
//---------------------------------------------------------------------------
CSourceSprite::CSourceSprite(
	int	iType )
	:
		m_iType        ( iType ),
		m_pviews       ( NULL ),
		m_piViewIndices( NULL )
{
}

//---------------------------------------------------------------------------
// CSourceSprite::~CSourceSprite
//---------------------------------------------------------------------------
CSourceSprite::~CSourceSprite()
{
	delete [] m_pviews;
	delete 	 m_piViewIndices;
}

//---------------------------------------------------------------------------
// CSourceSprite::Read
//---------------------------------------------------------------------------
BOOL
CSourceSprite::Read(
	FILE * fp )
{
	if ( 1 != fread( &m_iW, 	 		  sizeof( m_iW    		  ), 1, fp ) ||
	 	  1 != fread( &m_iH, 	 		  sizeof( m_iH	           ), 1, fp ) ||
		  1 != fread( &m_iBitsPerPixel, sizeof( m_iBitsPerPixel ), 1, fp ) ||
		  1 != fread( &m_nViews, 		  sizeof( m_nViews		  ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	ASSERT( 0 <= m_iW && m_iW < 1024 );
	ASSERT( 0 <= m_iH && m_iH < 1024 );

	ASSERT( 8  == m_iBitsPerPixel ||
			  15 == m_iBitsPerPixel ||	
			  16 == m_iBitsPerPixel ||	
			  24 == m_iBitsPerPixel ||	
			  32 == m_iBitsPerPixel );

	ASSERT( 0 < m_nViews && m_nViews <= 256 );

	m_pviews = new CSourceView [ m_nViews ];

	for ( int i = 0; i < m_nViews; ++i )
		if ( !m_pviews[i].Read( fp ))
		{
			TRAP();
			return FALSE;
		}

	return TRUE;
}

//---------------------------------------------------------------------------
// CSourceBuildingSprite::Read
//---------------------------------------------------------------------------
BOOL
CSourceBuildingSprite::Read(
	FILE * fp )
{
	if ( !CSourceSprite::Read( fp ))
	{
		TRAP();
		return FALSE;
	}

	for ( int iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
		for ( int iLayer = 0; iLayer < NUM_BLDG_LAYERS; ++iLayer )
			for ( int iDir = 0; iDir < NUM_BLDG_DIRS; ++iDir )
				for ( int iDamage = 0; iDamage < NUM_BLDG_DAMAGES; ++iDamage )
				{
					if ( 1 != fread( &m_aaaaiViewIndices[ iStage ][ iLayer ][ iDir ][ iDamage ], sizeof( int ), 1, fp ))
					{
						TRAP();
						return FALSE;
					}

					ASSERT( -1 <= m_aaaaiViewIndices[ iStage ][ iLayer ][ iDir ][ iDamage ] );
					ASSERT( m_aaaaiViewIndices[ iStage ][ iLayer ][ iDir ][ iDamage ] < m_nViews );
				}

	return TRUE;
}

//---------------------------------------------------------------------------
// CSourceVehicleSprite::Read
//---------------------------------------------------------------------------
BOOL
CSourceVehicleSprite::Read(
	FILE * fp )
{
	if ( !CSourceSprite::Read( fp ))
	{
		TRAP();
		return FALSE;
	}

	for ( int iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
		for ( int iDir = 0; iDir < NUM_VEHICLE_DIRS; ++iDir )
			for ( int iDamage = 0; iDamage < NUM_VEHICLE_DAMAGES; ++iDamage )
			{
				if ( 1 != fread( &m_aaaiViewIndices[ iTilt ][ iDir ][ iDamage ], sizeof( int ), 1, fp ))
				{
					TRAP();
					return FALSE;
				}

				ASSERT( -1 <= m_aaaiViewIndices[ iTilt ][ iDir ][ iDamage ] );
				ASSERT( m_aaaiViewIndices[ iTilt ][ iDir ][ iDamage ] < m_nViews );
			}

	return TRUE;
}

//---------------------------------------------------------------------------
// CSourceTerrainSprite::Read
//---------------------------------------------------------------------------
BOOL
CSourceTerrainSprite::Read(
	FILE * fp )
{
	if ( !CSourceSprite::Read( fp ))
	{
		TRAP();
		return FALSE;
	}

	for ( int iDir = 0; iDir < NUM_TERRAIN_DIRS; ++iDir )
		for ( int iDamage = 0; iDamage < NUM_TERRAIN_DAMAGES; ++iDamage )
		{
			if ( 1 != fread( &m_aaiViewIndices[ iDir ][ iDamage ], sizeof( int ), 1, fp ))
			{
				TRAP();
				return FALSE;
			}

			ASSERT( -1 <= m_aaiViewIndices[ iDir ][ iDamage ] );
			ASSERT( m_aaiViewIndices[ iDir ][ iDamage ] < m_nViews );
		}

	return TRUE;
}

//------------------------ C D e s t B l o c k I n f o ----------------------

struct CDestBlockInfo
{
	virtual BOOL	Write( FILE * fp ) const;
	virtual int		GetHdrSize() const { return sizeof( m_iOffset ) + sizeof( m_iLength ); }

	int	m_iOffset;
	int	m_iLength;
};

//----------------------- C D e s t L a y o u t I n f o ---------------------

struct CDestLayoutInfo
{
	virtual BOOL	Write( FILE * fp ) const;
	virtual int		GetHdrSize() const;

	CDestBlockInfo	m_blockinfoCompressed;
	int				m_aiDecompressedLength[ NUM_COLOR_REZ_BYTES ];	// 1-byte, 2-bytes, 3-bytes, 4-bytes
};

//-------------------- C D e s t D I B L a y o u t I n f o ------------------

struct CDestDIBLayoutInfo
{
	virtual BOOL	Write( FILE * fp ) const;
	virtual int		GetHdrSize() const;

	CDestBlockInfo	m_ablockinfoDecompressed[ NUM_COLOR_REZ_BYTES ];	// 1-byte, 2-bytes, 3-bytes, 4-bytes
};

//------------------------- C D e s t H o t S p o t ------------------------

struct CDestHotSpot
{
	virtual BOOL	Write( FILE * fp ) const;
	virtual void	Convert( CSourceHotSpot const * );
	virtual int		GetHdrSize() const { return sizeof( m_apt ) + sizeof( m_iIndex ) + sizeof( m_iType ); }

	CPoint	m_apt[ NUM_ZOOM_LEVELS ];
	int		m_iIndex;
	int		m_iType;
};

//----------------- C D e s t L a y o u t I n f o Z o o m s -----------------

struct CDestLayoutInfoZooms
{
	virtual BOOL	Write( FILE * fp ) const;
	virtual int		GetHdrSize() const;

	CDestLayoutInfo	m_alayoutinfo[ NUM_ZOOM_LEVELS ];
};

//------------------------ C D e s t B i t m a p H d r ----------------------

class CDestBitmapHdr
{
	
public:

	virtual void	Convert( CSourceBitmaps *, int iBytesPerPixel, int iType );
	virtual BOOL	Write  ( FILE * fp ) const;
	virtual int		GetHdrSize() const;

	BYTE	m_abyReserved[16];

	int	m_iTime;
	CRect	m_arect[4];

	CDestDIBLayoutInfo	m_adiblayoutinfo[ NUM_ZOOM_LEVELS ];
};

//-------------------------- C D e s t V i e w H d r ------------------------

class CDestViewHdr
{

public:

	CDestViewHdr();
   virtual ~CDestViewHdr();

	virtual void	Convert( int iSuperviewIndex, CSourceView const *, int iBytesPerPixel, int iType );
	virtual BOOL	Write( FILE * ) const;
	virtual int		GetHdrSize() const;
	virtual void	CalcBitmapOffsets( int iZoom,	int aiOffsetRez[ NUM_COLOR_REZ_BYTES ] );

	BYTE		m_abyReserved[ 128 ];
	CPoint	m_aptAnchor[ NUM_ZOOM_LEVELS ];
	int		m_iSuperviewIndex;
	int		m_nHotSpots;
	int		m_nBase;
	int		m_nOverlay;
	int		m_nFront1;
	int		m_nFront2;
	int		m_nBack1;
	int		m_nBack2;

	CDestHotSpot	* m_photspots;
	CDestBitmapHdr * m_pbitmaphdrBase;
	CDestBitmapHdr * m_pbitmaphdrOverlay;
	CDestBitmapHdr * m_pbitmaphdrFront1;
	CDestBitmapHdr * m_pbitmaphdrFront2;
	CDestBitmapHdr * m_pbitmaphdrBack1;
	CDestBitmapHdr * m_pbitmaphdrBack2;
};

//--------------------------- C D e s t S p r i t e -------------------------

class CDestSprite
{

public:

   virtual ~CDestSprite();

	virtual BOOL	Write    ( FILE * ) 		const;
	virtual BOOL	WriteInfo( FILE * ) 		const { return TRUE; }
	virtual int		GetHdrSize() 	 		const;
	virtual int		GetNumViewIndices() 	const = 0;

	int	m_iType;
	int	m_iHdrLen;
	int	m_iCompressionType;
	int	m_nViews;
	int	m_nSuperviews;

	CDestBlockInfo	  		  m_zoomblockinfo[NUM_ZOOM_LEVELS];
	CDestLayoutInfoZooms * m_playoutinfozoomsSuperview;
	int						* m_piViewOffsets;
	CDestViewHdr	 		* m_pviewhdr;

	BYTE *	CompressSuperview( BYTE * pbySrc, int iLenSrc, int * piLenDst ) const;

	int	   m_iDataLen;
	BYTE	** m_appbySuperviews[ NUM_ZOOM_LEVELS ];

	void	Convert( CSourceSprite const * );

	virtual void AssignViewIndices( CSourceSprite const * ) = 0;
	virtual BOOL WriteViewIndices( FILE * fp ) const = 0;
	virtual void AssignSuperviewIndices();
	virtual void MakeSuperviews( CSourceSprite const * psourcesprite ) = 0;

protected:

	CDestSprite();
};

//-------------------- C D e s t B u i l d i n g S p r i t e ----------------

class CDestBuildingSprite : public CDestSprite
{

public:

	void	AssignViewIndices( CSourceSprite const * );
	BOOL 	WriteViewIndices( FILE * fp ) const;
	BOOL	WriteInfo( FILE * ) const;
	void 	AssignSuperviewIndices();
	int	GetNumViewIndices() const;

	void	MakeSuperviews( CSourceSprite const * psourcesprite );

	int	m_aaaaiViewIndices[ NUM_BLDG_DIRS ][ NUM_BLDG_STAGES ][ NUM_BLDG_LAYERS ][ NUM_BLDG_DAMAGES ];
};

//-------------------- C D e s t V e h i c l e S p r i t e ------------------

class CDestVehicleSprite : public CDestSprite
{

public:

	void	AssignViewIndices( CSourceSprite const * );
	BOOL 	WriteViewIndices( FILE * fp ) const;
	BOOL	WriteInfo( FILE * ) const;
	void	MakeSuperviews( CSourceSprite const * psourcesprite );
	int	GetNumViewIndices() const;

	int	m_aaaiViewIndices[ NUM_VEHICLE_DIRS ][ NUM_VEHICLE_TILTS ][ NUM_VEHICLE_DAMAGES ];
};

//-------------------- C D e s t T e r r a i n S p r i t e ------------------

class CDestTerrainSprite : public CDestSprite
{

public:

	void	AssignViewIndices( CSourceSprite const * );
	BOOL 	WriteViewIndices( FILE * fp ) const;
	void	MakeSuperviews( CSourceSprite const * psourcesprite );
	int	GetNumViewIndices() const;

	int	m_aaiViewIndices[ NUM_TERRAIN_DIRS ][ NUM_TERRAIN_DAMAGES ];
};

//---------------------------------------------------------------------------
// CDestSprite::CDestSprite
//---------------------------------------------------------------------------
CDestSprite::CDestSprite()
	:
		m_playoutinfozoomsSuperview( NULL ),
		m_piViewOffsets            ( NULL ),
		m_pviewhdr						( NULL )
{
	for ( int i = 0; i < NUM_ZOOM_LEVELS; ++i )
		m_appbySuperviews[ i ] = NULL;
}

//---------------------------------------------------------------------------
// CDestSprite::Convert
//---------------------------------------------------------------------------
void
CDestSprite::Convert(
	CSourceSprite const * psourcesprite )
{
	int	i;

	m_iType   		= psourcesprite->m_iType;
	m_nViews  		= psourcesprite->m_nViews;
	m_nSuperviews 	= BUILDING == m_iType ? 4 : 1;

	ASSERT( 0 < m_nViews );

	m_playoutinfozoomsSuperview = new CDestLayoutInfoZooms [ m_nSuperviews ];
	m_piViewOffsets 				 = new int [ m_nViews ];
	m_pviewhdr 						 = new CDestViewHdr [ m_nViews ];

	AssignViewIndices( psourcesprite );
	AssignSuperviewIndices();

	int	iBytesPerPixel = ( psourcesprite->m_iBitsPerPixel + 7 ) >> 3;

	ASSERT( 1 == iBytesPerPixel || 3 == iBytesPerPixel );

	m_iCompressionType = 1 == iBytesPerPixel ? CoDec::SPRITE_8 : CoDec::SPRITE_24;

	for ( i = 0; i < m_nViews; ++i )
	{
		ASSERT( AfxIsValidAddress( &psourcesprite->m_pviews[i], sizeof( CSourceSprite )));
		m_pviewhdr[i].Convert( m_pviewhdr[i].m_iSuperviewIndex, &psourcesprite->m_pviews[i], iBytesPerPixel, m_iType );
	}

	MakeSuperviews( psourcesprite );

	m_iHdrLen = GetHdrSize();
}

//---------------------------------------------------------------------------
// CDestSprite::CompressSuperview
//---------------------------------------------------------------------------
BYTE *
CDestSprite::CompressSuperview(
	BYTE * pbySrc, 
	int	 iLenSrc, 
	int  * piLenDst ) const
{
	return ( BYTE * )CoDec::Compress(( CoDec::CODEC )m_iCompressionType, pbySrc, iLenSrc, *piLenDst );
}

//---------------------------------------------------------------------------
// CDestSprite::AssignSuperviewIndices
//---------------------------------------------------------------------------
void
CDestSprite::AssignSuperviewIndices()
{
	for ( int i = 0; i < m_nViews; ++i )
		m_pviewhdr[i].m_iSuperviewIndex = 0;
}

//---------------------------------------------------------------------------
// CDestSprite::~CDestSprite
//---------------------------------------------------------------------------
CDestSprite::~CDestSprite()
{
	delete [] m_playoutinfozoomsSuperview;
	delete [] m_pviewhdr;
	delete    m_piViewOffsets;

	int	i, j;

	for ( i = 0; i < NUM_ZOOM_LEVELS; ++i )
		if ( m_appbySuperviews[i] )
		{
			for ( j = 0; j < m_nSuperviews; ++j )
				delete m_appbySuperviews[i][j];

			delete m_appbySuperviews[i];
		}
}

//---------------------------------------------------------------------------
// CDestSprite::Write
//---------------------------------------------------------------------------
BOOL
CDestSprite::Write(
	FILE * fp ) const
{
	ASSERT( BUILDING == m_iType ||
			  VEHICLE  == m_iType ||
			  TERRAIN  == m_iType );

	ASSERT( 0 < m_iHdrLen );
	ASSERT( 0 < m_nViews && m_nViews <= 256 );
	ASSERT( BUILDING == m_iType || 1 == m_nSuperviews );
	ASSERT( BUILDING != m_iType || 4 == m_nSuperviews );

	if ( 1 != fwrite(( void * )&m_iType,   			sizeof( m_iType            ), 1, fp ) ||
		  1 != fwrite(( void * )&m_iHdrLen, 			sizeof( m_iHdrLen          ), 1, fp ) ||
		  1 != fwrite(( void * )&m_iCompressionType, sizeof( m_iCompressionType ), 1, fp ) ||
		  1 != fwrite(( void * )&m_nViews,  			sizeof( m_nViews           ), 1, fp ) ||
		  1 != fwrite(( void * )&m_nSuperviews,  	   sizeof( m_nSuperviews      ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	int	i, j;

	for ( i = 0; i < NUM_ZOOM_LEVELS; ++i )
		if ( !m_zoomblockinfo[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	for ( i = 0; i < m_nSuperviews; ++i )
		if ( !m_playoutinfozoomsSuperview[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	for ( i = 0; i < m_nViews; ++i )
		if ( 1 != fwrite(( void * )&m_piViewOffsets[i], sizeof( m_piViewOffsets[i] ), 1, fp ))
		{
			TRAP();
			return FALSE;
		}

	if ( !WriteViewIndices( fp ))
	{
		TRAP();
		return FALSE;
	}

	for ( i = 0; i < m_nViews; ++i )
		if ( !m_pviewhdr[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	ASSERT( 0 < m_iDataLen );

	if ( 1 != fwrite(( void * )&m_iDataLen, sizeof( m_iDataLen ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	for ( int iZoom = NUM_ZOOM_LEVELS - 1; iZoom >= 0; iZoom-- )
		for ( j = 0; j < m_nSuperviews; ++j )
		{
			int	iLength = m_playoutinfozoomsSuperview[j].m_alayoutinfo[ iZoom ].m_blockinfoCompressed.m_iLength;

			if ( 0 != iLength && 1 != fwrite( m_appbySuperviews[ iZoom ][j], iLength, 1, fp ))
			{
				TRAP();
				return FALSE;
			}
		}

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestSprite::GetHdrSize
//---------------------------------------------------------------------------
int
CDestSprite::GetHdrSize() const
{
	int	iSize = 0;

	iSize += sizeof( m_iCompressionType );
	iSize += sizeof( m_nViews );
	iSize += sizeof( m_nSuperviews );

	int i;

	for ( i = 0; i < NUM_ZOOM_LEVELS; ++i )
		iSize += m_zoomblockinfo[i].GetHdrSize();

	for ( i = 0; i < m_nSuperviews; ++i )
		iSize += m_playoutinfozoomsSuperview[i].GetHdrSize();

	iSize += m_nViews * sizeof( *m_piViewOffsets );
	iSize += GetNumViewIndices() * sizeof( int );

	for ( i = 0; i < m_nViews; ++i )
	{
		m_piViewOffsets[i] = iSize;

		iSize += m_pviewhdr[i].GetHdrSize();
	}

	return iSize;
}

//---------------------------------------------------------------------------
// CDestBuildingSprite::AssignViewIndices
//---------------------------------------------------------------------------
void
CDestBuildingSprite::AssignViewIndices(
	CSourceSprite const * psourcesprite )
{
	static	int	aiStageMap[] = { 0, 3, 2, 1 };
	static	int	aiDirMap[]   = { 0, 3, 2, 1 };

	CSourceBuildingSprite const * psourcebuildingsprite = ( CSourceBuildingSprite const * )psourcesprite;

	int	* piViewReferenced = NULL;

	piViewReferenced = new int [ m_nViews ];

	memset( piViewReferenced, 0, m_nViews * sizeof( int ));

	for ( int iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
		for ( int iLayer = 0; iLayer < NUM_BLDG_LAYERS; ++iLayer )
			for ( int iDir = 0; iDir < NUM_BLDG_DIRS; ++iDir )
				for ( int iDamage = 0; iDamage < NUM_BLDG_DAMAGES; ++iDamage )
				{
					int	iIndex = psourcebuildingsprite->m_aaaaiViewIndices[ iStage ][ iLayer ][ iDir ][ iDamage ];

					ASSERT( -1 <= iIndex && iIndex < m_nViews );

					m_aaaaiViewIndices[ aiDirMap[ iDir ]][ aiStageMap[ iStage ]][ iLayer ][ iDamage ] = iIndex;

					if ( 0 <= iIndex )
						piViewReferenced[ iIndex ]++;
				}

	for ( int i = 0; i < m_nViews; ++i )
		ASSERT( 1 == piViewReferenced[i] );

	delete piViewReferenced;
}

//---------------------------------------------------------------------------
// CDestBuildingSprite::GetNumViewIndices
//---------------------------------------------------------------------------
int
CDestBuildingSprite::GetNumViewIndices() const
{
	return sizeof( m_aaaaiViewIndices ) / sizeof( int );
}

//---------------------------------------------------------------------------
// CDestBuildingSprite::MakeSuperviews
//---------------------------------------------------------------------------
void
CDestBuildingSprite::MakeSuperviews(
	CSourceSprite const * psourcesprite )
{
	int	iZoomOffset 			= 0;
	int	iTotalCompressedSize = 0;
	int	iDir;
	int	iStage;
	int	iLayer;
	int	iDamage;
	int	iZoom;
	int	i;

	for ( iZoom = NUM_ZOOM_LEVELS - 1; iZoom >= 0; iZoom-- )
	{
		int	iCompressedZoomSize = 0;

		ASSERT( 1 == m_nSuperviews || 4 == m_nSuperviews );

		m_appbySuperviews[iZoom] = new BYTE * [ m_nSuperviews ];

		for ( i = 0; i < m_nSuperviews; ++i )
			m_appbySuperviews[iZoom][i] = NULL;

		for ( iDir = 0; iDir < NUM_BLDG_DIRS; ++iDir )
		{
			int	iUncompressedSuperviewSize = 0;

			for ( iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
				for ( iLayer = 0; iLayer < NUM_BLDG_LAYERS; ++iLayer )
					for ( iDamage = 0; iDamage < NUM_BLDG_DAMAGES; ++iDamage )
					{
						int	iSpriteIndex =	m_aaaaiViewIndices[ iDir ][ iStage ][ iLayer ][ iDamage ];

						if ( -1 != iSpriteIndex )
						{
							CSourceView * psourceview = &psourcesprite->m_pviews[iSpriteIndex];

							iUncompressedSuperviewSize += psourceview->GetDataSize( iZoom );
						}
					}

			BYTE	* pbyUncompressedSuperview = new BYTE [ iUncompressedSuperviewSize ];
			BYTE	* pby = pbyUncompressedSuperview;

			int	  aiDIBOffsetRez[ NUM_COLOR_REZ_BYTES ];

			for ( i = 0; i < NUM_COLOR_REZ_BYTES; ++i )
				aiDIBOffsetRez[i] = 0;

			for ( iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
				for ( iLayer = 0; iLayer < NUM_BLDG_LAYERS; ++iLayer )
					for ( iDamage = 0; iDamage < NUM_BLDG_DAMAGES; ++iDamage )
					{
						int	iSpriteIndex =	m_aaaaiViewIndices[ iDir ][ iStage ][ iLayer ][ iDamage ];

						if ( -1 != iSpriteIndex )
						{
							CSourceView  * psourceview = &psourcesprite->m_pviews[iSpriteIndex];
							CDestViewHdr * pdestview   = &m_pviewhdr[iSpriteIndex];

							pby += psourceview->GetData( iZoom, pby );

							pdestview->CalcBitmapOffsets( iZoom, aiDIBOffsetRez );
						}
					}

			int	iCompressedSuperviewSize = 0;

			if ( iUncompressedSuperviewSize )
				m_appbySuperviews[iZoom][iDir] = CompressSuperview( pbyUncompressedSuperview, iUncompressedSuperviewSize, &iCompressedSuperviewSize );

			m_playoutinfozoomsSuperview[ iDir ].m_alayoutinfo[ iZoom ].m_blockinfoCompressed.m_iLength = iCompressedSuperviewSize;
			m_playoutinfozoomsSuperview[ iDir ].m_alayoutinfo[ iZoom ].m_blockinfoCompressed.m_iOffset = iCompressedZoomSize;

			iTotalCompressedSize += iCompressedSuperviewSize;
			iCompressedZoomSize  += iCompressedSuperviewSize;

			for ( i = 0; i < NUM_COLOR_REZ_BYTES; ++i )
				m_playoutinfozoomsSuperview[ iDir ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[i] = aiDIBOffsetRez[i];

			ASSERT( m_playoutinfozoomsSuperview[ iDir ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[((psourcesprite->m_iBitsPerPixel + 7 ) >>3)-1] == iUncompressedSuperviewSize );

			delete pbyUncompressedSuperview;

			pbyUncompressedSuperview = NULL;
		}

		m_zoomblockinfo[ iZoom ].m_iOffset = iZoomOffset;
		m_zoomblockinfo[ iZoom ].m_iLength = iCompressedZoomSize;

		iZoomOffset += iCompressedZoomSize;
	}

	m_iDataLen = iTotalCompressedSize;
}

//---------------------------------------------------------------------------
// CDestBuildingSprite::WriteViewIndices
//---------------------------------------------------------------------------
BOOL
CDestBuildingSprite::WriteViewIndices(
	FILE * fp ) const
{
	for ( int iDir = 0; iDir < NUM_BLDG_DIRS; ++iDir )
		for ( int iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
			for ( int iLayer = 0; iLayer < NUM_BLDG_LAYERS; ++iLayer )
				for ( int iDamage = 0; iDamage < NUM_BLDG_DAMAGES; ++iDamage )
					if ( 1 != fwrite( &m_aaaaiViewIndices[iDir][iStage][iLayer][iDamage], sizeof( int ), 1, fp ))
						return FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestBuildingSprite::WriteInfo
//---------------------------------------------------------------------------
BOOL
CDestBuildingSprite::WriteInfo(
	FILE * fp ) const
{
	int	nAnchors = 0;
	int	nViews	= 0;

	int	anViewsWithHotspots[ NUM_HOTSPOTS ];

	for ( int i = 0; i < NUM_HOTSPOTS; ++i )
		anViewsWithHotspots[ i ] = 0;

	for ( int iDir = 0; iDir < NUM_BLDG_DIRS; ++iDir )
		for ( int iDamage = 0; iDamage < NUM_BLDG_DAMAGES; ++iDamage )
		{
			BOOL	abHotSpot[ NUM_HOTSPOTS ];
			BOOL	bAnchor = FALSE;
			BOOL	bView   = FALSE;

			for ( i = 0; i < NUM_HOTSPOTS; ++i )
				abHotSpot[ i ] = FALSE;

			for ( int iLayer = 0; iLayer < NUM_BLDG_LAYERS; ++iLayer )
			{
				int	iIndex = m_aaaaiViewIndices[ iDir ][ 0 ][ iLayer ][ iDamage ];

				if ( -1 == iIndex )
					continue;

				CDestViewHdr	* pdestviewhdr = m_pviewhdr + iIndex;

				bView = TRUE;

				CPoint	ptAnchor = pdestviewhdr->m_aptAnchor[ xiFirstZoom ];

				if ( 0 != ptAnchor.x || 0!= ptAnchor.y )
					bAnchor = TRUE;

				for ( int iHotSpot = 0; iHotSpot < pdestviewhdr->m_nHotSpots; iHotSpot++ )
				{
					CDestHotSpot * pdesthotspot = pdestviewhdr->m_photspots + iHotSpot;

					abHotSpot[ pdesthotspot->m_iType ] = TRUE;
				}
			}

			if ( bView )
			{
				if ( ! abHotSpot[ HOTSPOT_FLAG ] && xbReportErrors )
					theApp.AddErrorDialog( "Building view missing flag hotspot\n" );	// GG: 8/25/96

				++nViews;

				nAnchors += bAnchor;

				for ( i = 0; i < NUM_HOTSPOTS; ++i )
					anViewsWithHotspots[ i ] += abHotSpot[ i ];
			}
		}

	fprintf( fp, "Building hotspot/anchor info.\nUsing only Finished Construction views.\n\n" );
	fprintf( fp, "Anchor points          -> %d out of %d views.\n", nAnchors, nViews );
	fprintf( fp, "Damage hotspots        -> %d out of %d views.\n", anViewsWithHotspots[0], nViews );
	fprintf( fp, "Turret/Muzzle hotspots -> %d out of %d views.\n", anViewsWithHotspots[1], nViews );
	fprintf( fp, "Flag hotspots          -> %d out of %d views.\n", anViewsWithHotspots[2], nViews );

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestBuildingSprite::AssignSuperviewIndices
//---------------------------------------------------------------------------
void
CDestBuildingSprite::AssignSuperviewIndices()
{
	for ( int iDir = 0; iDir < NUM_BLDG_DIRS; ++iDir )
		for ( int iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
			for ( int iLayer = 0; iLayer < NUM_BLDG_LAYERS; ++iLayer )
				for ( int iDamage = 0; iDamage < NUM_BLDG_DAMAGES; ++iDamage )
				{
					int	iIndex = m_aaaaiViewIndices[ iDir ][ iStage ][ iLayer ][ iDamage ];

					if ( -1 != iIndex )
						m_pviewhdr[iIndex].m_iSuperviewIndex = iDir;
				}
}

//---------------------------------------------------------------------------
// CDestVehicleSprite::AssignViewIndices
//---------------------------------------------------------------------------
void
CDestVehicleSprite::AssignViewIndices(
	CSourceSprite const * psourcesprite )
{
	CSourceVehicleSprite const * psourcevehiclesprite = ( CSourceVehicleSprite const * )psourcesprite;

	int	* piViewReferenced = NULL;

	piViewReferenced = new int [ m_nViews ];

	memset( piViewReferenced, 0, m_nViews * sizeof( int ));

	for ( int iDir = 0; iDir < NUM_VEHICLE_DIRS; ++iDir )
		for ( int iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
			for ( int iDamage = 0; iDamage < NUM_VEHICLE_DAMAGES; ++iDamage )
				{
					int	iIndex = psourcevehiclesprite->m_aaaiViewIndices[ iTilt ][ iDir ][ iDamage ];

					ASSERT( -1 <= iIndex && iIndex < m_nViews );

					m_aaaiViewIndices[ iDir ][ iTilt ][ iDamage ] = iIndex;

					if ( 0 <= iIndex )
						piViewReferenced[ iIndex ]++;
				}

	for ( int i = 0; i < m_nViews; ++i )
		ASSERT( 1 == piViewReferenced[i] );

	delete piViewReferenced;
}

//---------------------------------------------------------------------------
// CDestVehicleSprite::WriteInfo
//---------------------------------------------------------------------------
BOOL
CDestVehicleSprite::WriteInfo(
	FILE * fp ) const
{
	int	nAnchors = 0;
	int	nViews	= 0;

	int	anViewsWithHotspots[ NUM_HOTSPOTS ];

	for ( int i = 0; i < NUM_HOTSPOTS; ++i )
		anViewsWithHotspots[ i ] = 0;

	for ( int iDir = 0; iDir < NUM_VEHICLE_DIRS; ++iDir )
		for ( int iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
			for ( int iDamage = 0; iDamage < NUM_VEHICLE_DAMAGES; ++iDamage )
			{
				BOOL	abHotSpot[ NUM_HOTSPOTS ];
				BOOL	bAnchor = FALSE;
				BOOL	bView   = FALSE;

				for ( i = 0; i < NUM_HOTSPOTS; ++i )
					abHotSpot[ i ] = FALSE;

				int	iIndex = m_aaaiViewIndices[ iDir ][ iTilt ][ iDamage ];

				if ( -1 == iIndex )
					continue;

				CDestViewHdr	* pdestviewhdr = m_pviewhdr + iIndex;

				bView = TRUE;

				CPoint	ptAnchor = pdestviewhdr->m_aptAnchor[ xiFirstZoom ];

				if ( 0 != ptAnchor.x || 0!= ptAnchor.y )
					bAnchor = TRUE;

				for ( int iHotSpot = 0; iHotSpot < pdestviewhdr->m_nHotSpots; iHotSpot++ )
				{
					CDestHotSpot * pdesthotspot = pdestviewhdr->m_photspots + iHotSpot;

					abHotSpot[ pdesthotspot->m_iType ] = TRUE;
				}

				if ( bView )
				{
					if ( ! abHotSpot[ HOTSPOT_FLAG ] && xbReportErrors )
						theApp.AddErrorDialog( "Vehicle view missing flag hotspot\n" );	// GG: 8/25/96

					++nViews;

					nAnchors += bAnchor;

					for ( i = 0; i < NUM_HOTSPOTS; ++i )
						anViewsWithHotspots[ i ] += abHotSpot[ i ];
				}
			}

	fprintf( fp, "Vehicle hotspot/anchor info.\n\n" );
	fprintf( fp, "Anchor points          -> %d out of %d views.\n", nAnchors, nViews );
	fprintf( fp, "Damage hotspots        -> %d out of %d views.\n", anViewsWithHotspots[0], nViews );
	fprintf( fp, "Turret/Muzzle hotspots -> %d out of %d views.\n", anViewsWithHotspots[1], nViews );
	fprintf( fp, "Flag hotspots          -> %d out of %d views.\n", anViewsWithHotspots[2], nViews );

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestVehicleSprite::GetNumViewIndices
//---------------------------------------------------------------------------
int
CDestVehicleSprite::GetNumViewIndices() const
{
	return sizeof( m_aaaiViewIndices ) / sizeof( int );
}

//---------------------------------------------------------------------------
// CDestVehicleSprite::MakeSuperviews
//---------------------------------------------------------------------------
void
CDestVehicleSprite::MakeSuperviews(
	CSourceSprite const * psourcesprite )
{
	int	iZoomOffset 			= 0;
	int	iTotalCompressedSize = 0;
	int	iDir;
	int	iTilt;
	int	iDamage;
	int	iZoom;
	int	i;

	for ( iZoom = NUM_ZOOM_LEVELS - 1; iZoom >= 0; iZoom-- )
	{
		int	iCompressedZoomSize = 0;
		int	iUncompressedSuperviewSize = 0;

		m_appbySuperviews[ iZoom ] = new BYTE * [ m_nSuperviews ];

		for ( i = 0; i < m_nSuperviews; ++i )
			m_appbySuperviews[ iZoom ][i] = NULL;

		for ( iDir = 0; iDir < NUM_VEHICLE_DIRS; ++iDir )
			for ( iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
				for ( iDamage = 0; iDamage < NUM_VEHICLE_DAMAGES; ++iDamage )
				{
					int	iSpriteIndex =	m_aaaiViewIndices[ iDir ][ iTilt ][ iDamage ];

					if ( -1 != iSpriteIndex )
					{
						CSourceView * psourceview = &psourcesprite->m_pviews[iSpriteIndex];

						iUncompressedSuperviewSize += psourceview->GetDataSize( iZoom );
					}
				}

		BYTE	* pbyUncompressedSuperview = new BYTE [ iUncompressedSuperviewSize ];
		BYTE	* pby = pbyUncompressedSuperview;

		int	  aiDIBOffsetRez[ NUM_COLOR_REZ_BYTES ];

		for ( i = 0; i < NUM_COLOR_REZ_BYTES; ++i )
			aiDIBOffsetRez[i] = 0;

		for ( iDir = 0; iDir < NUM_VEHICLE_DIRS; ++iDir )
			for ( iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
				for ( iDamage = 0; iDamage < NUM_VEHICLE_DAMAGES; ++iDamage )
				{
					int	iSpriteIndex =	m_aaaiViewIndices[ iDir ][ iTilt ][ iDamage ];

					if ( -1 != iSpriteIndex )
					{
						CSourceView * psourceview = &psourcesprite->m_pviews[iSpriteIndex];
						CDestViewHdr * pdestview  = &m_pviewhdr[iSpriteIndex];

						pby += psourceview->GetData( iZoom, pby );

						pdestview->CalcBitmapOffsets( iZoom, aiDIBOffsetRez );
					}
				}

		int	iCompressedSuperviewSize = 0;

		if ( iUncompressedSuperviewSize )
			m_appbySuperviews[iZoom][0] = CompressSuperview( pbyUncompressedSuperview, iUncompressedSuperviewSize, &iCompressedSuperviewSize );

		m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_blockinfoCompressed.m_iLength = iCompressedSuperviewSize;
		m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_blockinfoCompressed.m_iOffset = iCompressedZoomSize;

		iTotalCompressedSize += iCompressedSuperviewSize;
		iCompressedZoomSize  += iCompressedSuperviewSize;

		for ( i = 0; i < NUM_COLOR_REZ_BYTES; ++i )
			m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[i] = aiDIBOffsetRez[i];

		ASSERT( m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[((psourcesprite->m_iBitsPerPixel + 7 ) >>3)-1] == iUncompressedSuperviewSize );

		delete pbyUncompressedSuperview;

		pbyUncompressedSuperview = NULL;

		m_zoomblockinfo[ iZoom ].m_iOffset = iZoomOffset;
		m_zoomblockinfo[ iZoom ].m_iLength = iCompressedZoomSize;

		iZoomOffset += iCompressedZoomSize;
	}

	m_iDataLen = iTotalCompressedSize;
}

//---------------------------------------------------------------------------
// CDestVehicleSprite::WriteViewIndices
//---------------------------------------------------------------------------
BOOL
CDestVehicleSprite::WriteViewIndices(
	FILE * fp ) const
{
	for ( int iDir = 0; iDir < NUM_VEHICLE_DIRS; ++iDir )
		for ( int iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
			for ( int iDamage = 0; iDamage < NUM_VEHICLE_DAMAGES; ++iDamage )
				if ( 1 != fwrite( &m_aaaiViewIndices[iDir][iTilt][iDamage], sizeof( int ), 1, fp ))
					return FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestTerrainSprite::AssignViewIndices
//---------------------------------------------------------------------------
void
CDestTerrainSprite::AssignViewIndices(
	CSourceSprite const * psourcesprite )
{
	static	int	aiDirMap[]   = { 0, 3, 2, 1 };

	CSourceTerrainSprite const * psourceterrainsprite = ( CSourceTerrainSprite const * )psourcesprite;

	int	* piViewReferenced = NULL;

	piViewReferenced = new int [ m_nViews ];

	memset( piViewReferenced, 0, m_nViews * sizeof( int ));

	for ( int iDir = 0; iDir < NUM_TERRAIN_DIRS; ++iDir )
		for ( int iDamage = 0; iDamage < NUM_TERRAIN_DAMAGES; ++iDamage )
			{
				int	iIndex = psourceterrainsprite->m_aaiViewIndices[ iDir ][ iDamage ];

				ASSERT( -1 <= iIndex && iIndex < m_nViews );

				m_aaiViewIndices[ aiDirMap[ iDir ]][ iDamage ] = iIndex;

				if ( 0 <= iIndex )
					piViewReferenced[ iIndex ]++;
			}

	for ( int i = 0; i < m_nViews; ++i )
		ASSERT( 1 == piViewReferenced[i] );

	delete piViewReferenced;
}

//---------------------------------------------------------------------------
// CDestTerrainSprite::GetNumViewIndices
//---------------------------------------------------------------------------
int
CDestTerrainSprite::GetNumViewIndices() const
{
	return sizeof( m_aaiViewIndices ) / sizeof( int );
}

//---------------------------------------------------------------------------
// CDestTerrainSprite::MakeSuperviews
//---------------------------------------------------------------------------
void
CDestTerrainSprite::MakeSuperviews(
	CSourceSprite const * psourcesprite )
{
	int	iZoomOffset 			= 0;
	int	iTotalCompressedSize = 0;
	int	iDir;
	int	iDamage;
	int	iZoom;
	int	i;

	for ( iZoom = NUM_ZOOM_LEVELS - 1; iZoom >= 0; iZoom-- )
	{
		int	iCompressedZoomSize = 0;
		int	iUncompressedSuperviewSize = 0;

		m_appbySuperviews[iZoom] = new BYTE * [ m_nSuperviews ];

		for ( i = 0; i < m_nSuperviews; ++i )
			m_appbySuperviews[iZoom][i] = NULL;

		for ( iDir = 0; iDir < NUM_TERRAIN_DIRS; ++iDir )
			for ( iDamage = 0; iDamage < NUM_TERRAIN_DAMAGES; ++iDamage )
			{
				int	iSpriteIndex =	m_aaiViewIndices[ iDir ][ iDamage ];

				if ( -1 != iSpriteIndex )
				{
					CSourceView * psourceview = &psourcesprite->m_pviews[iSpriteIndex];

					iUncompressedSuperviewSize += psourceview->GetDataSize( iZoom );
				}
		}

		BYTE	* pbyUncompressedSuperview = new BYTE [ iUncompressedSuperviewSize ];
		BYTE	* pby = pbyUncompressedSuperview;

		int	  aiDIBOffsetRez[ NUM_COLOR_REZ_BYTES ];

		for ( i = 0; i < NUM_COLOR_REZ_BYTES; ++i )
			aiDIBOffsetRez[i] = 0;

		for ( iDir = 0; iDir < NUM_TERRAIN_DIRS; ++iDir )
			for ( iDamage = 0; iDamage < NUM_TERRAIN_DAMAGES; ++iDamage )
			{
				int	iSpriteIndex =	m_aaiViewIndices[ iDir ][ iDamage ];

				if ( -1 != iSpriteIndex )
				{
					CSourceView  * psourceview = &psourcesprite->m_pviews[iSpriteIndex];
					CDestViewHdr * pdestview   = &m_pviewhdr[iSpriteIndex];

					pby += psourceview->GetData( iZoom, pby );

					pdestview->CalcBitmapOffsets( iZoom, aiDIBOffsetRez );
				}
			}

		int	iCompressedSuperviewSize = 0;

		if ( iUncompressedSuperviewSize )
			m_appbySuperviews[iZoom][0] = CompressSuperview( pbyUncompressedSuperview, iUncompressedSuperviewSize, &iCompressedSuperviewSize );

		m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_blockinfoCompressed.m_iLength = iCompressedSuperviewSize;
		m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_blockinfoCompressed.m_iOffset = iCompressedZoomSize;

		iTotalCompressedSize += iCompressedSuperviewSize;
		iCompressedZoomSize  += iCompressedSuperviewSize;

		for ( i = 0; i < NUM_COLOR_REZ_BYTES; ++i )
			m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[i] = aiDIBOffsetRez[i];

		ASSERT( m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[((psourcesprite->m_iBitsPerPixel + 7 ) >>3)-1] == iUncompressedSuperviewSize );

		delete pbyUncompressedSuperview;

		pbyUncompressedSuperview = NULL;

		m_zoomblockinfo[ iZoom ].m_iOffset = iZoomOffset;
		m_zoomblockinfo[ iZoom ].m_iLength = iCompressedZoomSize;

		iZoomOffset += iCompressedZoomSize;
	}

	m_iDataLen = iTotalCompressedSize;
}

//---------------------------------------------------------------------------
// CDestTerrainSprite::WriteViewIndices
//---------------------------------------------------------------------------
BOOL
CDestTerrainSprite::WriteViewIndices(
	FILE * fp ) const
{
	for ( int iDir = 0; iDir < NUM_TERRAIN_DIRS; ++iDir )
		for ( int iDamage = 0; iDamage < NUM_TERRAIN_DAMAGES; ++iDamage )
			if ( 1 != fwrite( &m_aaiViewIndices[iDir][iDamage], sizeof( int ), 1, fp ))
				return FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestViewHdr::CDestViewHdr
//---------------------------------------------------------------------------
CDestViewHdr::CDestViewHdr()
	:
		m_photspots			 ( NULL ),
		m_pbitmaphdrBase	 ( NULL ),
		m_pbitmaphdrOverlay( NULL ),
		m_pbitmaphdrFront1 ( NULL ),
		m_pbitmaphdrFront2 ( NULL ),
		m_pbitmaphdrBack1  ( NULL ),
		m_pbitmaphdrBack2  ( NULL )
{
}

//---------------------------------------------------------------------------
// CDestViewHdr::~CDestViewHdr
//---------------------------------------------------------------------------
CDestViewHdr::~CDestViewHdr()
{
	delete [] m_photspots;		  
	delete [] m_pbitmaphdrBase;	  
	delete [] m_pbitmaphdrOverlay;
	delete [] m_pbitmaphdrFront1; 
	delete [] m_pbitmaphdrFront2; 
	delete [] m_pbitmaphdrBack1;  
	delete [] m_pbitmaphdrBack2;  
}

//---------------------------------------------------------------------------
// CDestViewHdr::Convert
//---------------------------------------------------------------------------
void
CDestViewHdr::Convert(
	int					  iSuperviewIndex,
	CSourceView const * psourceview,
	int					  iBytesPerPixel,
	int					  iType )
{
	int	i;

	for ( i = 0; i < NUM_ZOOM_LEVELS; ++i )
		m_aptAnchor[i] = CPoint( psourceview->m_iAnchorX >> i,
									 	 psourceview->m_iAnchorY >> i );

	m_iSuperviewIndex = iSuperviewIndex;

	m_nHotSpots = psourceview->m_nHotSpots;
	m_nBase		= psourceview->m_nBase;
	m_nOverlay  = psourceview->m_nOverlay;
	m_nFront1 	= psourceview->m_nFront1;
	m_nFront2	= psourceview->m_nFront2;
	m_nBack1		= psourceview->m_nBack1;
	m_nBack2		= psourceview->m_nBack2;

	if ( m_nHotSpots )
	{
		ASSERT( psourceview->m_photspots );
		m_photspots = new CDestHotSpot [ m_nHotSpots ];
	}

	for ( i = 0; i < m_nHotSpots; ++i )
		m_photspots[i].Convert( &psourceview->m_photspots[i] );

	if ( m_nBase )
	{
		ASSERT( psourceview->m_pbitmapsBase );
		m_pbitmaphdrBase = new CDestBitmapHdr [ m_nBase ];
	}

	for ( i = 0; i < m_nBase; ++i )
		m_pbitmaphdrBase[i].Convert( &psourceview->m_pbitmapsBase[i], iBytesPerPixel, iType );

	if ( m_nOverlay )
	{
		ASSERT( psourceview->m_pbitmapsOverlay );
		m_pbitmaphdrOverlay = new CDestBitmapHdr[ m_nOverlay ];
	}

	for ( i = 0; i < m_nOverlay; ++i )
		m_pbitmaphdrOverlay[i].Convert( &psourceview->m_pbitmapsOverlay[i], iBytesPerPixel, iType );

	if ( m_nFront1 )
	{
		ASSERT( psourceview->m_pbitmapsFront1 );
		m_pbitmaphdrFront1 = new CDestBitmapHdr [ m_nFront1 ];
	}

	for ( i = 0; i < m_nFront1; ++i )
		m_pbitmaphdrFront1[i].Convert( &psourceview->m_pbitmapsFront1[i], iBytesPerPixel, iType );

	if ( m_nFront2 )
	{
		ASSERT( psourceview->m_pbitmapsFront2 );
		m_pbitmaphdrFront2 = new CDestBitmapHdr [ m_nFront2 ];
	}

	for ( i = 0; i < m_nFront2; ++i )
		m_pbitmaphdrFront2[i].Convert( &psourceview->m_pbitmapsFront2[i], iBytesPerPixel, iType );

	if ( m_nBack1 )
	{
		ASSERT( psourceview->m_pbitmapsBack1 );
		m_pbitmaphdrBack1 = new CDestBitmapHdr [ m_nBack1 ];
	}

	for ( i = 0; i < m_nBack1; ++i )
		m_pbitmaphdrBack1[i].Convert( &psourceview->m_pbitmapsBack1[i], iBytesPerPixel, iType );

	if ( m_nBack2 )
	{
		ASSERT( psourceview->m_pbitmapsBack2 );
		m_pbitmaphdrBack2 = new CDestBitmapHdr [ m_nBack2 ];
	}

	for ( i = 0; i < m_nBack2; ++i )
		m_pbitmaphdrBack2[i].Convert( &psourceview->m_pbitmapsBack2[i], iBytesPerPixel, iType );
}

//---------------------------------------------------------------------------
// CDestViewHdr::CalcBitmapOffsets
//---------------------------------------------------------------------------
void
CDestViewHdr::CalcBitmapOffsets(
	int	iZoom,
	int	aiOffset[ NUM_COLOR_REZ_BYTES ] )
{
	int	i, j;

	for ( i = 0; i < m_nBase; ++i )
		for ( j = 0; j < NUM_COLOR_REZ_BYTES; ++j )
		{
			ASSERT( m_pbitmaphdrBase );

			CDestBlockInfo * pdestblockinfo = &m_pbitmaphdrBase[i].m_adiblayoutinfo[ iZoom ].m_ablockinfoDecompressed[ j ];

			pdestblockinfo->m_iOffset = aiOffset[j];

			aiOffset[j] += pdestblockinfo->m_iLength;
		}

	for ( i = 0; i < m_nOverlay; ++i )
		for ( j = 0; j < NUM_COLOR_REZ_BYTES; ++j )
		{
			ASSERT( m_pbitmaphdrOverlay );

			CDestBlockInfo * pdestblockinfo = &m_pbitmaphdrOverlay[i].m_adiblayoutinfo[ iZoom ].m_ablockinfoDecompressed[ j ];

			pdestblockinfo->m_iOffset = aiOffset[j];

			aiOffset[j] += pdestblockinfo->m_iLength;
		}

	for ( i = 0; i < m_nFront1; ++i )
		for ( j = 0; j < NUM_COLOR_REZ_BYTES; ++j )
		{
			ASSERT( m_pbitmaphdrFront1 );

			CDestBlockInfo * pdestblockinfo = &m_pbitmaphdrFront1[i].m_adiblayoutinfo[ iZoom ].m_ablockinfoDecompressed[ j ];

			pdestblockinfo->m_iOffset = aiOffset[j];

			aiOffset[j] += pdestblockinfo->m_iLength;
		}

	for ( i = 0; i < m_nFront2; ++i )
		for ( j = 0; j < NUM_COLOR_REZ_BYTES; ++j )
		{
			ASSERT( m_pbitmaphdrFront2 );

			CDestBlockInfo * pdestblockinfo = &m_pbitmaphdrFront2[i].m_adiblayoutinfo[ iZoom ].m_ablockinfoDecompressed[ j ];

			pdestblockinfo->m_iOffset = aiOffset[j];

			aiOffset[j] += pdestblockinfo->m_iLength;
		}

	for ( i = 0; i < m_nBack1; ++i )
		for ( j = 0; j < NUM_COLOR_REZ_BYTES; ++j )
		{
			ASSERT( m_pbitmaphdrBack1 );

			CDestBlockInfo * pdestblockinfo = &m_pbitmaphdrBack1[i].m_adiblayoutinfo[ iZoom ].m_ablockinfoDecompressed[ j ];

			pdestblockinfo->m_iOffset = aiOffset[j];

			aiOffset[j] += pdestblockinfo->m_iLength;
		}

	for ( i = 0; i < m_nBack2; ++i )
		for ( j = 0; j < NUM_COLOR_REZ_BYTES; ++j )
		{
			ASSERT( m_pbitmaphdrBack2 );

			CDestBlockInfo * pdestblockinfo = &m_pbitmaphdrBack2[i].m_adiblayoutinfo[ iZoom ].m_ablockinfoDecompressed[ j ];

			pdestblockinfo->m_iOffset = aiOffset[j];

			aiOffset[j] += pdestblockinfo->m_iLength;
		}
}

//---------------------------------------------------------------------------
// CDestViewHdr::Write
//---------------------------------------------------------------------------
BOOL
CDestViewHdr::Write(
	FILE * fp ) const
{
	int	i;

	#ifdef _DEBUG
	for ( i = 0; i < NUM_ZOOM_LEVELS; ++i )
	{
		ASSERT( -1024 < m_aptAnchor[i].x && m_aptAnchor[i].x < 1024 );
		ASSERT( -1024 < m_aptAnchor[i].y && m_aptAnchor[i].y < 1024 );
	}
	#endif

	ASSERT( 0 <= m_iSuperviewIndex );
	ASSERT( 0 <= m_nHotSpots && m_nHotSpots <  1000 );
	ASSERT( 0 <= m_nBase     && m_nBase     <  10 );
	ASSERT( 0 <= m_nOverlay  && m_nOverlay  <  10 );
	ASSERT( 0 <= m_nFront1   && m_nFront1   <= 26 );
	ASSERT( 0 <= m_nFront2   && m_nFront2   <= 28 );
	ASSERT( 0 <= m_nBack1    && m_nBack1    <= 26 );
	ASSERT( 0 <= m_nBack2    && m_nBack2    <= 28 );

	if ( 1 != fwrite(( void * )m_abyReserved, 		sizeof( m_abyReserved     ), 1, fp ) ||
		  1 != fwrite(( void * )&m_aptAnchor, 			sizeof( m_aptAnchor       ), 1, fp ) ||
		  1 != fwrite(( void * )&m_iSuperviewIndex,	sizeof( m_iSuperviewIndex ), 1, fp ) ||
		  1 != fwrite(( void * )&m_nHotSpots, 			sizeof( m_nHotSpots       ), 1, fp ) ||
		  1 != fwrite(( void * )&m_nBase, 				sizeof( m_nBase           ), 1, fp ) ||
		  1 != fwrite(( void * )&m_nOverlay,			sizeof( m_nOverlay        ), 1, fp ) ||
		  1 != fwrite(( void * )&m_nFront1,				sizeof( m_nFront1         ), 1, fp ) ||
		  1 != fwrite(( void * )&m_nFront2,				sizeof( m_nFront2 	     ), 1, fp ) ||
		  1 != fwrite(( void * )&m_nBack1,				sizeof( m_nBack1 		     ), 1, fp ) ||
		  1 != fwrite(( void * )&m_nBack2,				sizeof( m_nBack2 		     ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	for ( i = 0; i < m_nHotSpots; ++i )
		if ( !m_photspots[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	for ( i = 0; i < m_nBase; ++i )
		if ( !m_pbitmaphdrBase[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	for ( i = 0; i < m_nOverlay; ++i )
		if ( !m_pbitmaphdrOverlay[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	for ( i = 0; i < m_nFront1; ++i )
		if ( !m_pbitmaphdrFront1[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	for ( i = 0; i < m_nFront2; ++i )
		if ( !m_pbitmaphdrFront2[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	for ( i = 0; i < m_nBack1; ++i )
		if ( !m_pbitmaphdrBack1[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	for ( i = 0; i < m_nBack2; ++i )
		if ( !m_pbitmaphdrBack2[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestViewHdr::GetHdrSize
//---------------------------------------------------------------------------
int
CDestViewHdr::GetHdrSize() const
{
	int	iSize = 0;

	iSize += sizeof( m_abyReserved );
	iSize += sizeof( m_aptAnchor );
	iSize += sizeof( m_iSuperviewIndex );
	iSize += sizeof( m_nHotSpots );
	iSize += sizeof( m_nBase );
	iSize += sizeof( m_nOverlay );
	iSize += sizeof( m_nFront1 );
	iSize += sizeof( m_nFront2 );
	iSize += sizeof( m_nBack1 );
	iSize += sizeof( m_nBack2 );

	int	i;

	for ( i = 0; i < m_nHotSpots; ++i )
		iSize += m_photspots[i].GetHdrSize();

	for ( i = 0; i < m_nBase; ++i )
		iSize += m_pbitmaphdrBase[i].GetHdrSize();

	for ( i = 0; i < m_nOverlay; ++i )
		iSize += m_pbitmaphdrOverlay[i].GetHdrSize();

	for ( i = 0; i < m_nFront1; ++i )
		iSize += m_pbitmaphdrFront1[i].GetHdrSize();

	for ( i = 0; i < m_nFront2; ++i )
		iSize += m_pbitmaphdrFront2[i].GetHdrSize();

	for ( i = 0; i < m_nBack1; ++i )
		iSize += m_pbitmaphdrBack1[i].GetHdrSize();

	for ( i = 0; i < m_nBack2; ++i )
		iSize += m_pbitmaphdrBack2[i].GetHdrSize();

	return iSize;
}

//---------------------------------------------------------------------------
// CDestBitmapHdr::Convert
//---------------------------------------------------------------------------
void
CDestBitmapHdr::Convert(
	CSourceBitmaps * psourcebitmaps,
	int				  iBytesPerPixel,
	int				  iType	)
{
	int	i, j;

	m_iTime = psourcebitmaps->m_abitmap[0].m_iTime;

	int	iMultH = TERRAIN == iType ? 8 : 1;

	for ( i = 0; i < NUM_ZOOM_LEVELS; ++i )
	{
		m_arect[i].left   = psourcebitmaps->m_abitmap[i].m_iX;
		m_arect[i].top    = psourcebitmaps->m_abitmap[i].m_iY;
		m_arect[i].right  = psourcebitmaps->m_abitmap[i].m_iW + m_arect[i].left;
		m_arect[i].bottom = psourcebitmaps->m_abitmap[i].m_iH * iMultH + m_arect[i].top;
	}

	for ( i = 0; i < NUM_ZOOM_LEVELS; ++i )
		for ( j = 0; j < NUM_COLOR_REZ_BYTES; ++j )
			m_adiblayoutinfo[i].m_ablockinfoDecompressed[j].m_iLength =
				psourcebitmaps->m_abitmap[i].GetSize( iBytesPerPixel, 1 + j, iType );
}

//---------------------------------------------------------------------------
// CDestBitmapHdr::Write
//---------------------------------------------------------------------------
BOOL
CDestBitmapHdr::Write(
	FILE * fp ) const
{
	ASSERT( 0 <= m_iTime && m_iTime < 240 );

	if ( 1 != fwrite(( void * )m_abyReserved, sizeof( m_abyReserved ), 1, fp ) ||
		  1 != fwrite(( void * )&m_iTime, 	   sizeof( m_iTime       ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	int	i;

	for ( i = 0; i < NUM_ZOOM_LEVELS; ++i )
	{
		ASSERT( -1024 < m_arect[i].left   && m_arect[i].left   < 1024 );
		ASSERT( -1024 < m_arect[i].top    && m_arect[i].top    < 1024 );
		ASSERT( -1024 < m_arect[i].right  && m_arect[i].right  < 1024 );
		ASSERT( -1024 < m_arect[i].bottom && m_arect[i].bottom < 1024 );

		if ( 1 != fwrite(( void * )&m_arect[i].left,   sizeof( long ), 1, fp ) ||
			  1 != fwrite(( void * )&m_arect[i].top,    sizeof( long ), 1, fp ) ||
			  1 != fwrite(( void * )&m_arect[i].right,  sizeof( long ), 1, fp ) ||
			  1 != fwrite(( void * )&m_arect[i].bottom, sizeof( long ), 1, fp ))
		{
			TRAP();
			return FALSE;
		}
	}

	for ( i = 0; i < NUM_ZOOM_LEVELS; ++i )
		if ( !m_adiblayoutinfo[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestBitmapHdr::GetHdrSize
//---------------------------------------------------------------------------
int
CDestBitmapHdr::GetHdrSize() const
{
	int	iSize = 0;

	iSize += sizeof( m_abyReserved );
	iSize += sizeof( m_iTime );
	iSize += sizeof( m_arect );

	for ( int i = 0; i < NUM_ZOOM_LEVELS; ++i )
		iSize += m_adiblayoutinfo[i].GetHdrSize();

	return iSize;
}

//---------------------------------------------------------------------------
// CDestLayoutInfoZooms::Write
//---------------------------------------------------------------------------
BOOL
CDestLayoutInfoZooms::Write(
	FILE * fp ) const
{
	for ( int i = 0; i < NUM_ZOOM_LEVELS; ++i )
		if ( !m_alayoutinfo[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestLayoutInfoZooms::GetHdrSize
//---------------------------------------------------------------------------
int
CDestLayoutInfoZooms::GetHdrSize() const
{
	int	iSize = 0;

	for ( int i = 0; i < NUM_ZOOM_LEVELS; ++i )
		iSize += m_alayoutinfo[i].GetHdrSize();

	return iSize;
}

//---------------------------------------------------------------------------
// CDestHotSpot::Convert
//---------------------------------------------------------------------------
void
CDestHotSpot::Convert(
	CSourceHotSpot const * psourcehotspot )
{
	for ( int i = 0; i < NUM_ZOOM_LEVELS; ++i )
	{
		m_apt[i].x = psourcehotspot->m_iX >> i;
		m_apt[i].y = psourcehotspot->m_iY >> i;
	}

	m_iType  = psourcehotspot->m_iType;
	m_iIndex = psourcehotspot->m_iIndex;
}

//---------------------------------------------------------------------------
// CDestHotSpot::Write
//---------------------------------------------------------------------------
BOOL
CDestHotSpot::Write(
	FILE * fp ) const
{
	for ( int i = 0; i < NUM_ZOOM_LEVELS; ++i )
	{
		ASSERT( -1024 < m_apt[i].x && m_apt[i].x < 1024 );
		ASSERT( -1024 < m_apt[i].y && m_apt[i].y < 1024 );

		if ( 1 != fwrite(( void * )&m_apt[i].x, sizeof( m_apt[i].x ), 1, fp ) ||
			  1 != fwrite(( void * )&m_apt[i].y, sizeof( m_apt[i].y ), 1, fp ))
		{
			TRAP();
			return FALSE;
		}
	}

	ASSERT( 0 <= m_iIndex && m_iIndex < 100 );
	ASSERT( 0 <= m_iType && m_iType < NUM_HOTSPOTS );

	if ( 1 != fwrite(( void * )&m_iIndex, sizeof( m_iIndex ), 1, fp ) ||
		  1 != fwrite(( void * )&m_iType,  sizeof( m_iType  ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestLayoutInfo::Write
//---------------------------------------------------------------------------
BOOL
CDestLayoutInfo::Write(
	FILE * fp ) const
{
	m_blockinfoCompressed.Write( fp );

	for ( int i = 0; i < NUM_COLOR_REZ_BYTES; ++i )
		if ( 1 != fwrite(( void * )&m_aiDecompressedLength[i], sizeof( m_aiDecompressedLength[i] ), 1, fp ))
		{
			TRAP();
			return FALSE;
		}

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestLayoutInfo::GetHdrSize
//---------------------------------------------------------------------------
int
CDestLayoutInfo::GetHdrSize() const
{
	int	iSize = 0;

	iSize += m_blockinfoCompressed.GetHdrSize();
	iSize += sizeof( m_aiDecompressedLength );

	return iSize;
}

//---------------------------------------------------------------------------
// CDestDIBLayoutInfo::Write
//---------------------------------------------------------------------------
BOOL
CDestDIBLayoutInfo::Write(
	FILE * fp ) const
{
	for ( int i = 0; i < NUM_COLOR_REZ_BYTES; ++i )
		if ( !m_ablockinfoDecompressed[i].Write( fp ))
		{
			TRAP();
			return FALSE;
		}

	return TRUE;
}

//---------------------------------------------------------------------------
// CDestDIBLayoutInfo::GetHdrSize
//---------------------------------------------------------------------------
int
CDestDIBLayoutInfo::GetHdrSize() const
{
	int	iSize = 0;

	for ( int i = 0; i < NUM_COLOR_REZ_BYTES; ++i )
		iSize += m_ablockinfoDecompressed[i].GetHdrSize();

	return iSize;
}

//---------------------------------------------------------------------------
// CDestBlockInfo::Write
//---------------------------------------------------------------------------
BOOL
CDestBlockInfo::Write(
	FILE * fp ) const
{
	ASSERT( 0 <= m_iOffset );
	ASSERT( 0 <= m_iLength );

	if ( 1 != fwrite(( void * )&m_iOffset, sizeof( m_iOffset ), 1, fp ) ||
		  1 != fwrite(( void * )&m_iLength, sizeof( m_iLength ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	return TRUE;
}

//--------------------------------------------------------------------------
// Sprite::SaveNewSPR
//--------------------------------------------------------------------------
BOOL
Sprite::SaveNewSPR(
	char const * pPathSPR,
	BOOL			 bShareware,
	BOOL			 bReportErrors )
{
	xbReportErrors = bReportErrors;
	xiFirstZoom 	= bShareware ? 1 : 0;

	FILE	*fp = fopen( pPathSPR, "rb" );

	if ( !fp )
	{
		TRAP();
		return FALSE;
	}

	int	iType;

	if ( 1 != fread( &iType, sizeof( iType ), 1, fp ))
	{
		TRAP();
		return FALSE;
	}

	ASSERT( BUILDING == iType ||
			  VEHICLE  == iType ||
			  TERRAIN  == iType );

	CSourceSprite * psourcesprite = NULL;

	switch ( iType )
	{
		case BUILDING:	psourcesprite = new CSourceBuildingSprite(); break;
		case VEHICLE:	psourcesprite = new CSourceVehicleSprite();  break;
		case TERRAIN:	psourcesprite = new CSourceTerrainSprite();  break;
	}

	if ( !psourcesprite->Read( fp ))
	{
		TRAP();
		return FALSE;
	}

	fclose( fp );

	CDestSprite	* pdestsprite = NULL;

	switch ( iType )
	{
		case BUILDING:	pdestsprite = new CDestBuildingSprite(); break;
		case VEHICLE:	pdestsprite = new CDestVehicleSprite (); break;
		case TERRAIN:	pdestsprite = new CDestTerrainSprite (); break;
	}

	pdestsprite->Convert( psourcesprite );

	delete psourcesprite;

	#define NEW_SPR_FORMAT
	#ifdef NEW_SPR_FORMAT
	fp = fopen( pPathSPR, "wb" );

	if ( !fp )
	{
		TRAP();
		return FALSE;
	}

	if ( !pdestsprite->Write( fp ))
	{
		TRAP();
		return FALSE;
	}

	fclose( fp );
	#endif

	char * pszTxt = new char [ strlen( pPathSPR ) + 5 ];
	
	strcpy( pszTxt, pPathSPR );

	char * psz = strrchr( pszTxt, '.' );

	if ( !psz )
		strcat( pszTxt, ".txt" );
	else
		strcpy( psz, ".txt" );

	fp = fopen( pszTxt, "wt" );

	if ( !fp )
	{
		TRAP();
		return FALSE;
	}

	pdestsprite->WriteInfo( fp );

	fclose( fp );

	delete pszTxt;
	delete pdestsprite;

	return TRUE;
}
