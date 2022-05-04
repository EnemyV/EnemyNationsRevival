//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

// sprite.cpp : the sprites
//

#pragma intrinsic( memset )
#pragma intrinsic( memcpy )

#include "stdafx.h"

#include <scanlist.h>
#include <codec.h>

#include "sprite.h"
#include "lastplnt.h"
#include "error.h"

#include "terrain.inl"
#include "unit.inl"
#include "building.inl"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

const  int 	FRAME_RATE = 24;
const  int 	SHADE_CONTRAST = 14;
const  int 	MAX_SPRITE_HEIGHT = 1280;
const  int 	MAX_SPRITE_WIDTH  = 1024;
const  int 	MAX_SPRITE_SIZE   = MAX_SPRITE_HEIGHT * MAX_SPRITE_WIDTH;
const	 int	MAX_SHADE = N_SHADES - 1;
const	 int	NORMAL_SHADE = N_SHADES / 2;
const  int 	MAX_INVISIBLE_SHADE = 3;	// Shade level decrement for invisible terrain
const  int 	MAX_BYTES_PER_PIXEL = 4;
const  int 	TERRAIN_CURSOR_COLOR_VALID   = 255;	// FIXIT: Use color defines, when available
const  int 	TERRAIN_CURSOR_COLOR_INVALID = 249;	// FIXIT: Use color defines, when available

// in a perfect world the following would all be in their correct classes.
// however, for performance reasons they're global variables.
BOOL			bShowAmb = TRUE;		// if false only re-paint sprite whose status has changed
BOOL			bForceDraw = FALSE;	// forces all tiles to redraw
BOOL			bInvAmb = FALSE;		// for windows other than Area - invalidates all ambient sprites

// these are set in CAnimAtr::SetCurrent for the window being drawn to
int					xiZoom = 0;					// the level of zoom we are at
int					xiDir = 0;						// the direction we are rotated
CDIBWnd 			*xpdibwnd = NULL;
CAnimAtr	const	*xpanimatr;

unsigned			CDrawParms::g_uUpdateFlags;
CRect				CDrawParms::g_rectClip;

const	int	MAX_TRIANGLE_HT = 1000;

static	ScanList	xscanlist( MAX_TRIANGLE_HT );

// In and Out apply to the left triangle. Switch for the right triangle

const		int	FEATHER_DIM = 256;
static	int	xaaiFeather   [ NUM_ZOOM_LEVELS ][ FEATHER_DIM ];
static	int	xaiFeatherNone[ FEATHER_DIM ];

static	int * xpiFeatherEdgeLeft;
static	int * xpiFeatherEdgeRight;
static	int * xpiFeatherCenter;


//------------------------------ S i n I n i t ------------------------------

Fixed	g_afixSin[ 256 ];

class SinInit
{
public:

	SinInit();
};

static SinInit xsininit;

//---------------------------------------------------------------------------
// SinInit::SinInit
//---------------------------------------------------------------------------
SinInit::SinInit()
{
	for ( int i = 0; i < 256; ++i )
		g_afixSin[ i ] = sin( i * 2 * 3.14150265 / 256 );
}

//-------------------------- F e a t h e r I n i t --------------------------

class FeatherInit
{
public:

	FeatherInit();
};

static FeatherInit xfeatherinit;

//---------------------------------------------------------------------------
// FeatherInit::FeatherInit
//---------------------------------------------------------------------------
FeatherInit::FeatherInit()
{
	for ( int i = 0; i < FEATHER_DIM; ++i )
		xaaiFeather[ 0 ][ i ] = 6 + RandNum( 11 );

	for ( int j = 1; j < NUM_ZOOM_LEVELS; ++j )
		for ( i = 0; i < FEATHER_DIM; ++i )
			xaaiFeather[ j ][ i ] = xaaiFeather[ 0 ][ i ] >> j;
}

const int x_aiInvisibleShadeIndex[] =
{
	0 * MAX_INVISIBLE_SHADE / MAX_SHADE,
	1 * MAX_INVISIBLE_SHADE / MAX_SHADE,
	2 * MAX_INVISIBLE_SHADE / MAX_SHADE,
	3 * MAX_INVISIBLE_SHADE / MAX_SHADE,
	4 * MAX_INVISIBLE_SHADE / MAX_SHADE,
	5 * MAX_INVISIBLE_SHADE / MAX_SHADE,
	6 * MAX_INVISIBLE_SHADE / MAX_SHADE,
	7 * MAX_INVISIBLE_SHADE / MAX_SHADE
};

CRect 	*prVp = NULL;

CDrawParms			* CSpriteDIB::G_pdrawparms = NULL;
CSpriteView const * CSpriteDIB::G_pspriteview;

static CColorBuffer	  x_colorbuffer;

//-------------------------- C S p r i t e D I B ---------------------------

//-------------------------------------------------------------------------
// CSpriteDIBParms::CSpriteDIBParms
//-------------------------------------------------------------------------
CSpriteDIBParms::CSpriteDIBParms(
	unsigned	uTime,
	int		iType,
	int		iBitsPerPixel )
	:
		m_uTime        ( uTime ),
		m_iType        ( iType ),
		m_iBitsPerPixel( iBitsPerPixel ),
		m_pspriteview  ( NULL )
{
}

//--------------------------------------------------------------------------
// CSpriteDIB::SpriteViewToDIB
//--------------------------------------------------------------------------
CPoint
CSpriteDIB::SpriteViewToDIB(
	CPoint 		const & ptView ) const
{
	ASSERT( GetView() );

	return ptView + GetView()->Rect().TopLeft() - Rect().TopLeft();
}

//--------------------------------------------------------------------------
// CSpriteDIB::DIBToSpriteView
//--------------------------------------------------------------------------
CPoint
CSpriteDIB::DIBToSpriteView(
	CPoint 		const & ptDIB ) const
{
	ASSERT( GetView() );

	return ptDIB - GetView()->Rect().TopLeft() + Rect().TopLeft();
}

//--------------------------------------------------------------------------
// CSpriteDIB::StructureGetWindowPos
//--------------------------------------------------------------------------
CPoint									// Window (x,y) of bitmap's UL corner
CSpriteDIB::StructureGetWindowPos() const
{
	CPoint	pt( X(), Y() );				// UL corner in sprite.exe coords

	pt -= GetView()->Rect().TopLeft();		// To bounded view coords
	pt += GetParms()->m_ptOffset;				// To window client coords

	return pt;
}

//--------------------------------------------------------------------------
// CSpriteDIB::CalcBoundingRect
//--------------------------------------------------------------------------
CRect										// Bounding rect in window coords
CSpriteDIB::CalcBoundingRect() const
{
	switch ( m_iType )
	{
		case CSprite::EFFECT: 
		case CSprite::STRUCTURE:	return StructureCalcBoundingRect();
		case CSprite::TERRAIN:		return TerrainCalcBoundingRect();
		case CSprite::VEHICLE:		return VehicleCalcBoundingRect();

		default:	ASSERT( 0 );
	}

	return CRect( 0, 0, 0, 0 );
}

//--------------------------------------------------------------------------
// CSpriteDIB::StructureCalcBoundingRect
//--------------------------------------------------------------------------
CRect										// Bounding rect in window coords
CSpriteDIB::StructureCalcBoundingRect() const
{
	CPoint	ptUL = StructureGetWindowPos();

	return CRect( ptUL.x, ptUL.y, ptUL.x + Width(), ptUL.y + Height() );
}

//--------------------------------------------------------------------------
// CSpriteDIB::TerrainCalcBoundingRect
//--------------------------------------------------------------------------
CRect										// Bounding rect in window coords
CSpriteDIB::TerrainCalcBoundingRect() const
{
	ASSERT_STRICT_VALID( this );

	CPoint		aptHex[4];
	CHexCoord	hexcoord( TerrainGetParms()->m_hexcoord );

	if ( !xpanimatr->MapToWindowHex( hexcoord, aptHex ))
		return CRect( 0, 0, 0, 0 );

	int	iMinYIndex = 0;
	int	iMaxYIndex = 0;
	int	iMinY 	  = aptHex[0].y;
	int	iMaxY 	  = aptHex[0].y;

	for ( int i = 1; i < 4; ++i )
		if ( aptHex[i].y < iMinY )
		{
			iMinY = aptHex[i].y;
			iMinYIndex = i;
		}
		else if ( aptHex[i].y > iMaxY )
		{
			iMaxY = aptHex[i].y;
			iMaxYIndex = i;
		}

	return CRect( aptHex[ 0 ].x,
					  aptHex[ iMinYIndex ].y,
					  aptHex[ 2 ].x,
					  aptHex[ iMaxYIndex ].y );
}

//--------------------------------------------------------------------------
// CSpriteDIB::IsHit
//--------------------------------------------------------------------------
BOOL
CSpriteDIB::IsHit(
	CPoint const &	ptCursor ) const
{
	switch ( m_iType )
	{
		case CSprite::EFFECT:		
		case CSprite::STRUCTURE:	return StructureIsHit( ptCursor );
		case CSprite::TERRAIN:		return TerrainIsHit  ( ptCursor );
		case CSprite::VEHICLE:		return VehicleIsHit  ( ptCursor );
	}

	return FALSE;
}

//--------------------------------------------------------------------------
// CSpriteDIB::StructureIsHit
//--------------------------------------------------------------------------
BOOL
CSpriteDIB::StructureIsHit(
	CPoint const &	ptCursor ) const
{
	CRect	rectBound = CalcBoundingRect();

	int	x = ptCursor.x - rectBound.left;
	int	y = ptCursor.y - rectBound.top;

	if	( x < 0 || x >= rectBound.Width() ||
		  y < 0 || y >= rectBound.Height() )
		return FALSE;

	if ( GetParms()->m_bTransparentHitTest )
		return TRUE;

	BYTE	const * pbyData   		  = GetDIBPixels();
	int	const * piRowStartOffsets = ( int const * )pbyData;
	BYTE	const * pbyPixels 		  = ( BYTE const * )( piRowStartOffsets + Height() );
	BYTE	const * pbySrc 			  = pbyPixels + piRowStartOffsets[ y ];
	int			  xBytes   			  = x * m_iBytesPerPixel;
	int			  xBytesCur			  = 0;

	for ( ;; )
	{
		xBytesCur += *( long * )pbySrc;

		if ( xBytes < xBytesCur )
			return FALSE;

		pbySrc += sizeof( long );

		int	iDataBytes = *( long * )pbySrc;

		xBytesCur += iDataBytes;

		if ( xBytes < xBytesCur )
			return TRUE;

		pbySrc += sizeof( long ) + iDataBytes;
	}

	return FALSE;
}


//--------------------------------------------------------------------------
// CSpriteDIB::TerrainIsHit
//--------------------------------------------------------------------------
BOOL
CSpriteDIB::TerrainIsHit(
	CPoint const & ptCursor ) const
{
	ASSERT_STRICT_VALID( this );

	CHexCoord	hexcoord( TerrainGetParms()->m_hexcoord );

	return xpanimatr->IsHexHit( hexcoord, ptCursor, CRect( 0, 0, 0, 0 ) );
}

//--------------------------------------------------------------------------
// CSpriteDIB::VehicleIsHit
//--------------------------------------------------------------------------
BOOL
CSpriteDIB::VehicleIsHit(
	CPoint const & ptWindow ) const
{
	ASSERT_STRICT_VALID( this );

	// Convert the window point to view coords

	ASSERT( GetParms() );
	ASSERT( GetView()  );

	CQuadDrawParms * pdrawparms = VehicleGetParms();

	CPoint	ptDib = pdrawparms->WindowToSpriteView( ptWindow, *GetView() );

	// Convert the view point to dib coords

	ptDib = SpriteViewToDIB( ptDib );

	if ( ptDib.x < 0 || Width()  <= ptDib.x ||
		  ptDib.y < 0 || Height() <= ptDib.y )
		return FALSE;

	if ( pdrawparms->m_bTransparentHitTest )
		return TRUE;

	BYTE			*pbyMagenta = thePal.GetDeviceColor( 253 );
	BYTE 	const *pSrc 		= GetDIBPixels();

	if ( !pSrc )
		return FALSE;

	pSrc += m_iBytesPerPixel * ( ptDib.y * Width() + ptDib.x );

	return memcmp( pSrc, pbyMagenta, m_iBytesPerPixel );
}

//--------------------------------------------------------------------------
// CSpriteDIB::TerrainGetShadeIndex - Shade index for a terrain triangle
//
// Light source is from the right
//--------------------------------------------------------------------------
int					// Shade index in range [0-7] (dark to bright)
CSpriteDIB::TerrainGetShadeIndex(
	CMapLoc3D	amaploc3d[4],	// Input: hex corners in world coords
	BOOL	 		bLeft )			// Compute shading for left triangle, or right triangle
{
	int	iDY1;
	int	iDY2;
	int	k;

	if ( bLeft )
	{
		iDY1 = SHADE_CONTRAST *	( amaploc3d[3].m_fixZ - amaploc3d[0].m_fixZ ).Round();
		iDY2 = SHADE_CONTRAST *	( amaploc3d[1].m_fixZ - amaploc3d[0].m_fixZ ).Round();

		k = -iDY1 - iDY2 + 128;
	}
	else
	{
		iDY1 = SHADE_CONTRAST * ( amaploc3d[1].m_fixZ - amaploc3d[2].m_fixZ ).Round();
		iDY2 = SHADE_CONTRAST *	( amaploc3d[3].m_fixZ - amaploc3d[2].m_fixZ ).Round();

		k = iDY1 + iDY2 + 128;
	}

	if ( k < 0 )
		return 0;
	else
		k = (( k * k ) << 1 ) / ( iDY1*iDY1 + iDY2*iDY2 + 128 * 64 );

	return Min( k, N_SHADES - 1 );
}

//--------------------------------------------------------------------------
// CSpriteDIB::Draw
//--------------------------------------------------------------------------
CRect										// Bounding rect in window coords
CSpriteDIB::Draw() const
{
	#ifdef _DEBUG
	CheckValid();
	#endif

	if ( !Rect().IsRectEmpty() )
		switch ( m_iType )
		{
			case CSprite::EFFECT:
			case CSprite::STRUCTURE:	return StructureDraw();
			case CSprite::TERRAIN:		return TerrainDraw();
			case CSprite::VEHICLE:		return VehicleDraw();

			default:	ASSERT( 0 );
		}

	return CRect( 0, 0, 0, 0 );
}

//--------------------------------------------------------------------------
// CSpriteDIB::StructureDraw
//--------------------------------------------------------------------------
CRect										// Bounding rect in window coords
CSpriteDIB::StructureDraw() const
{
	CDIB	 * pdib  = xpdibwnd->GetDIB();
	CPoint	ptUL  = StructureGetWindowPos();

	return StructureDrawToDIB( pdib, ptUL, *prVp );
}

//--------------------------------------------------------------------------
// CSpriteDIB::TerrainDraw
//--------------------------------------------------------------------------
CRect										// Bounding rect in window coords
CSpriteDIB::TerrainDraw() const
{
	CMapLoc3D	amaploc3d[4];
	CPoint		aptHex[4];

	CHexCoord	hexcoord( TerrainGetParms()->m_hexcoord );

	hexcoord.GetWorldHex( amaploc3d );

	if ( !xpanimatr->WorldToWindowHex( amaploc3d, aptHex ))
		return CRect( 0, 0, 0, 0 );

	CRect	rectBound( aptHex[0].x,
						  Min( Min( aptHex[0].y, aptHex[1].y ), aptHex[2].y ),
						  aptHex[2].x,
						  Max( Max( aptHex[0].y, aptHex[2].y ), aptHex[3].y ));

	CRect	rectIntersect = rectBound & *prVp;

	if ( rectBound.IsRectEmpty() )
		return rectBound;

	CHex	*phex       = theMap.GetHex( hexcoord );
	BOOL	 bInvisible = 0 == phex->GetVisibility();

	int	aiShadeIndex[2];
	
	if ( TerrainGetParms()->m_bShade )
	{
		aiShadeIndex[0] = TerrainGetShadeIndex( amaploc3d, TRUE  );
		aiShadeIndex[1] = TerrainGetShadeIndex( amaploc3d, FALSE );
	}
	else
	{
		aiShadeIndex[0] = NORMAL_SHADE;
		aiShadeIndex[1] = NORMAL_SHADE;
	}

	int	iCursorMode = phex->GetCursorMode( );
	int	iColor;

	switch ( iCursorMode )
	{
		case CHex::no_cur:			iColor =  -1; break;
		case CHex::ok_cur:			iColor = 255; break;
		case CHex::warn_cur:			iColor = 250; break;
		case CHex::bad_cur:			iColor = 249; break;
		case CHex::lousy_cur:		iColor = 251; break;
		case CHex::land_exit_cur:	iColor =   0; break;
		case CHex::sea_exit_cur:	iColor = 252; break;
	}

	BOOL	bHatch = -1 != iColor;

	if ( TerrainGetParms()->m_bDrawVert )
	{
		if ( bHatch )
			TerrainDrawQuadVertHatched( aiShadeIndex, aptHex, bInvisible, iColor );
		else
			TerrainDrawQuadVert( aiShadeIndex, aptHex, bInvisible );
	}
	else
		TerrainDrawQuad( aiShadeIndex, aptHex, bInvisible, bHatch, iColor );

	return rectBound;
}

//--------------------------------------------------------------------------
// CSpriteDIB::StructureDrawToDIB
//--------------------------------------------------------------------------
CRect										// Bounding rect in window client coords
CSpriteDIB::StructureDrawToDIB(
	CDIB 	 		 * pdib,
	CPoint const &	ptDst,
	CRect	 const & rectClip ) const	// Clipping rect in window client coords
{
	CRect	rectBound( ptDst, CSize( Width(), Height() ));
	CRect	rectDraw  = rectClip & rectBound;

	if ( rectDraw.left >= rectDraw.right ||
		  rectDraw.top  >= rectDraw.bottom )
      return CRect( 0, 0, 0, 0 );

	int	iTopSrcY  		  = rectDraw.top - rectBound.top;
	int	iTopDstY  		  = rectDraw.top;
	int	iClippedH 		  = rectDraw.Height();
	int	iLeftBytesClipX  = m_iBytesPerPixel * rectDraw.left;
	int	iRightBytesClipX = m_iBytesPerPixel * rectDraw.right;
	int	iLeftBytesX 	  = m_iBytesPerPixel * rectBound.left;
	int	iDirPitch		  = pdib->GetDirPitch();

	BYTE	const * pbyData   		  = GetDIBPixels();
	int	const * piRowStartOffsets = ( int const * )pbyData;
	BYTE	const * pbyPixels 		  = ( BYTE const * )( piRowStartOffsets + Height() );
	BYTE	const * pbySrc 			  = pbyPixels + piRowStartOffsets[ iTopSrcY ];
	CDIBits		  dibits 			  = pdib->GetBits();
	BYTE			* pbyDst 			  = dibits + pdib->GetOffset( 0, iTopDstY );

	for ( int iY = 0; iY < iClippedH; ++iY )
	{
		int	iBytesX = iLeftBytesX;

		for ( ;; )
		{
			iBytesX += *( int * )pbySrc;
			pbySrc  += sizeof( int );

			int	nDataBytes = *( int * )pbySrc;

			pbySrc += sizeof( int );

			if ( 0 == nDataBytes )
				break;

			int	iLeftBytesX  = Max( iLeftBytesClipX,  iBytesX );
			int	iRightBytesX = Min( iRightBytesClipX, iBytesX + nDataBytes );
			int	nBytes       = iRightBytesX - iLeftBytesX;

			if ( nBytes > 0 )
				memcpy( pbyDst + iLeftBytesX, pbySrc + iLeftBytesX - iBytesX, nBytes );

			iBytesX += nDataBytes;
			pbySrc  += nDataBytes;
		}

		pbyDst += iDirPitch;
	}

	return rectBound;
}

//-----------------------------------------------------------------------
// CSpriteDIB::TerrainDrawQuad
//-----------------------------------------------------------------------
void
CSpriteDIB::TerrainDrawQuad(
	int		aiShadeIndex[2],
	CPoint	aptHex[4],
	BOOL		bInvisible,
	BOOL		bHatch,
	int		iColorIndex ) const
{
	BYTE const * pbyHatchColor;

	if ( bHatch )
	{
		x_colorbuffer.SetColor( iColorIndex, Width() >> 1 );

		pbyHatchColor    = x_colorbuffer.GetBuffer();
		xpiFeatherCenter = xaiFeatherNone;
	}
	else
		xpiFeatherCenter = xaaiFeather[ xiZoom ];

	CDIB	* pdib = xpdibwnd->GetDIB();

	CPoint	aptHexTri[4];

	int	top 			= prVp->top;
	int	bottom 		= prVp->bottom;
	int	left	 		= prVp->left;
	int	right	 		= prVp->right;
	int	iFrame 		= theGame.GetFrame();

	if ( bInvisible )
	{
		aiShadeIndex[0] = x_aiInvisibleShadeIndex[ aiShadeIndex[0] ];
		aiShadeIndex[1] = x_aiInvisibleShadeIndex[ aiShadeIndex[1] ];
	}

	CDIBits	dibits = pdib->GetBits();

	BYTE 			* pDstOrg	= dibits;
	BYTE 			* pDstOrg2;
	BYTE const	* pData;

	ASSERT_STRICT( Width() == ( MAX_HEX_HT << 1 ) >> xiZoom );

	int	iTopY;
	int	iBotY;
	int	fixDY;
	int	fixV = 0;
	int	fixDV;
	int	fixSrcH2 	= MAX_HEX_HT << 15 - xiZoom;
	int	iSrcW  		= MAX_HEX_HT << 1 >> xiZoom;
	int	iDstH 		= pdib->GetHeight() >> 3;
	int	iSrcWPower 	= HEX_HT_PWR + 1 - xiZoom;

	int	iBytesPerPixel  = m_iBytesPerPixel;
	int	iBytesPerPixel2 = iBytesPerPixel << 1;
	int	i;
	int	iDirPitch = pdib->GetDirPitch();
	int	iRows;

	int * piLeft;	
	int * piRight;

	int	iCount;

	int	iLeftMax;
	int	iRightMin;
	BYTE	byOffset;

	// Do left triangle

	aptHexTri[0] = aptHex[0];
	aptHexTri[1] = aptHex[1];
	aptHexTri[2] = aptHex[3];

	xscanlist.ScanPoly( aptHexTri, 3 );

	pData = TerrainGetDIBPixels( aiShadeIndex[ 0 ] ) - aptHex[0].x * m_iBytesPerPixel;

	for ( i = 0; i < 2; ++i )
	{
		switch ( TerrainGetParms()->m_aeFeather[ CTerrainDrawParms::FEATHER_TL + i ] )
		{
			case CTerrainDrawParms::FEATHER_INOUT:

				xpiFeatherEdgeLeft  = xaaiFeather[ xiZoom ];
				xpiFeatherEdgeRight = xaaiFeather[ xiZoom ];

				break;

			case CTerrainDrawParms::FEATHER_IN:

				xpiFeatherEdgeLeft  = xaiFeatherNone;
				xpiFeatherEdgeRight = xaaiFeather[ xiZoom ];

				break;

			case CTerrainDrawParms::FEATHER_OUT:

				xpiFeatherEdgeLeft  = xaaiFeather[ xiZoom ];
				xpiFeatherEdgeRight = xaiFeatherNone;

				break;

			case CTerrainDrawParms::FEATHER_NONE:

				xpiFeatherEdgeLeft  = xaiFeatherNone;
				xpiFeatherEdgeRight = xaiFeatherNone;

				break;
		}

		if ( i == 0 )
		{
			iTopY = xscanlist.m_iTopY;
			iBotY = aptHexTri[0].y;
			fixV  = 0;
		}
		else
		{
			iTopY = iBotY;
			iBotY = xscanlist.m_iBottomY;
			fixV  = fixSrcH2;
		}

		iRows = iBotY - iTopY;

		if ( iRows > 0 )
		{
			iBotY = Min( bottom, iBotY );

			fixDY = iRows << 16;
			fixDV = fixSrcH2;

			FIXDIV( fixDV, fixDY );

			if ( iTopY < top )
			{
				int	fixDelY = ( top - iTopY ) << 16;
				int	fixTemp = fixDV;

				FIXMUL( fixTemp, fixDelY );

				fixV  += fixTemp;
				iTopY  = top;
			}

			iCount = iBotY - iTopY;

			if ( 0 >= iCount )
				continue;

			pDstOrg2  = pDstOrg + pdib->GetOffset( 0, iTopY );
			piLeft  	 = xscanlist.m_piLeft  + iTopY - xscanlist.m_iTopY;
			piRight 	 = xscanlist.m_piRight + iTopY - xscanlist.m_iTopY;
			byOffset  = xpanimatr->GetUL().y + iTopY;
			iLeftMax  = Max(( long )left,  aptHexTri[0].x );
			iRightMin = Min(( long )right, aptHexTri[1].x );

			switch ( m_iBytesPerPixel )
			{
				case 1:

				{
					for ( int y = iTopY; y < iBotY; ++y, ++byOffset )
					{
						int	iV = fixV >> 16 << iSrcWPower;

						int	iLeft  = Max( iLeftMax,  *piLeft - *( xpiFeatherEdgeLeft  + byOffset ));
						int	iRight = Min( iRightMin, *piLeft + *( xpiFeatherEdgeRight + byOffset ));

						iLeft += ( byOffset ^ iLeft ) & 0x0001;

						BYTE const * pbySrc = pData + iV + iLeft;
						BYTE		  * pbyDst = pDstOrg2 + iLeft;

						iLeft -= iRight;

						if ( iLeft < 0 )
							do
							{
								*pbyDst = *pbySrc;

								pbyDst += 2;
								pbySrc += 2;
								iLeft  += 2;

							} while ( iLeft < 0 );

						iLeft  = Max( iLeftMax, iRight );
						iRight = Min( iRightMin, *piRight - *( xpiFeatherCenter + byOffset ));

						int	iDelX = iRight - iLeft;

						if ( iDelX > 0 )
							if ( bHatch && 0 == (( y + iFrame ) & 0x04 ))
								memset( pDstOrg2 + iLeft, iColorIndex, iDelX );
							else
								memcpy( pDstOrg2 + iLeft, pData + iV + iLeft, iDelX );

						iLeft  = Max( iLeftMax,  iRight );
						iRight = Min( right, 	*piRight + *( xpiFeatherCenter + byOffset ));

						iLeft += ( byOffset ^ iLeft ) & 0x0001;

						pbySrc = pData + iV + iLeft;
						pbyDst = pDstOrg2 + iLeft;

						iLeft -= iRight;

						if ( iLeft < 0 )
							do
							{
								*pbyDst = *pbySrc;

								pbyDst += 2;
								pbySrc += 2;
								iLeft  += 2;

							} while ( iLeft < 0 );

						piLeft++;
						piRight++;

						fixV     += fixDV;
						pDstOrg2 += iDirPitch;
					}

					break;
				}

				default:

				{
					int	iSrcWBytes	= m_iBytesPerPixel * iSrcW;

					for ( int y = iTopY; y < iBotY; ++y, ++byOffset )
					{
						int	iV = fixV >> 16;

						int	iLeft  = Max( iLeftMax,  *piLeft - *( xpiFeatherEdgeLeft  + byOffset ));
						int	iRight = Min( iRightMin, *piLeft + *( xpiFeatherEdgeRight + byOffset ));

						iLeft += ( byOffset ^ iLeft ) & 0x0001;

						int	iLeftBytes = iBytesPerPixel * iLeft;
						int	iRowBytes  = iV * iSrcWBytes;

						BYTE const 	* pbySrc = pData + iRowBytes + iLeftBytes;
						BYTE			* pbyDst = pDstOrg2 + iLeftBytes;

						iLeft -= iRight;

						if ( iLeft < 0 )
							do
							{
								memcpy( pbyDst, pbySrc, iBytesPerPixel );

								pbyDst += iBytesPerPixel2;
								pbySrc += iBytesPerPixel2;
								iLeft  += 2;

							} while ( iLeft < 0 );

						iLeft  = Max( iLeftMax,  iRight );
						iRight = Min( iRightMin, *piRight - *( xpiFeatherCenter + byOffset ));

						int	iDelXBytes = iBytesPerPixel * ( iRight - iLeft );

						iLeftBytes = iBytesPerPixel * iLeft;

						if ( iDelXBytes > 0 )
							if ( bHatch && 0 == (( y + iFrame ) & 0x04 ))
								memcpy( pDstOrg2 + iLeftBytes, pbyHatchColor, iDelXBytes );
							else
								memcpy( pDstOrg2 + iLeftBytes, pData + iRowBytes + iLeftBytes, iDelXBytes );

						iLeft  = Max( iLeftMax,  iRight );
						iRight = Min( right, 	*piRight + *( xpiFeatherCenter + byOffset ));

						iLeft += ( byOffset ^ iLeft ) & 0x0001;

						iLeftBytes = iBytesPerPixel * iLeft;

						pbySrc = pData + iRowBytes + iLeftBytes;
						pbyDst = pDstOrg2 + iLeftBytes;

						iLeft -= iRight;

						if ( iLeft < 0 )
							do
							{
								memcpy( pbyDst, pbySrc, iBytesPerPixel );

								pbyDst += iBytesPerPixel2;
								pbySrc += iBytesPerPixel2;
								iLeft  += 2;

							} while ( iLeft < 0 );

						piLeft++;
						piRight++;

						fixV     += fixDV;
						pDstOrg2 += iDirPitch;
					}
				}
			}
		}
	}

	// Do right triangle

	aptHexTri[0] = aptHex[1];
	aptHexTri[1] = aptHex[2];
	aptHexTri[2] = aptHex[3];

	xscanlist.ScanPoly( aptHexTri, 3 );

	pData = TerrainGetDIBPixels( aiShadeIndex[ 1 ] ) - aptHex[0].x * m_iBytesPerPixel;

	for ( i = 0; i < 2; ++i )
	{
		switch ( TerrainGetParms()->m_aeFeather[ CTerrainDrawParms::FEATHER_TR + i ] )
		{
			case CTerrainDrawParms::FEATHER_INOUT:

				xpiFeatherEdgeLeft  = xaaiFeather[ xiZoom ];
				xpiFeatherEdgeRight = xaaiFeather[ xiZoom ];

				break;

			case CTerrainDrawParms::FEATHER_IN:

				xpiFeatherEdgeLeft  = xaaiFeather[ xiZoom ];
				xpiFeatherEdgeRight = xaiFeatherNone;

				break;

			case CTerrainDrawParms::FEATHER_OUT:

				xpiFeatherEdgeLeft  = xaiFeatherNone;
				xpiFeatherEdgeRight = xaaiFeather[ xiZoom ];

				break;

			case CTerrainDrawParms::FEATHER_NONE:

				xpiFeatherEdgeLeft  = xaiFeatherNone;
				xpiFeatherEdgeRight = xaiFeatherNone;

				break;
		}

		if ( i == 0 )
		{
			iTopY = xscanlist.m_iTopY;
			iBotY = aptHexTri[1].y;
			fixV  = 0;
		}
		else
		{
			iTopY = iBotY;
			iBotY = xscanlist.m_iBottomY;
			fixV  = fixSrcH2;
		}

		iRows = iBotY - iTopY;

		if ( iRows > 0 )
		{
			iBotY = Min( bottom, iBotY );

			fixDY = iRows << 16;
			fixDV = fixSrcH2;

			FIXDIV( fixDV, fixDY );

			if ( iTopY < top )
			{
				int	fixDelY = ( top - iTopY ) << 16;
				int	fixTemp = fixDV;

				FIXMUL( fixTemp, fixDelY );

				fixV  += fixTemp;
				iTopY  = top;
			}

			iCount = iBotY - iTopY;

			if ( 0 >= iCount )
				continue;

			pDstOrg2  = pDstOrg + pdib->GetOffset( 0, iTopY );
			piLeft    = xscanlist.m_piLeft  + iTopY - xscanlist.m_iTopY;
			piRight   = xscanlist.m_piRight + iTopY - xscanlist.m_iTopY;
			byOffset  = xpanimatr->GetUL().y + iTopY;
			iLeftMax  = Max(( long )left,  aptHexTri[0].x );
			iRightMin = Min(( long )right, aptHexTri[1].x );

			switch ( m_iBytesPerPixel )
			{
				case 1:

				{
					for ( int y = iTopY; y < iBotY; ++y, ++byOffset )
					{
						int	iV = fixV >> 16 << iSrcWPower;

						int	iLeft  = Max( left,      *piLeft - *( xpiFeatherCenter + byOffset ));
						int	iRight = Min( iRightMin, *piLeft + *( xpiFeatherCenter + byOffset ));

						iLeft += !(( byOffset ^ iLeft ) & 0x0001 );

						BYTE const * pbySrc = pData + iV + iLeft;
						BYTE		  * pbyDst = pDstOrg2 + iLeft;

						iLeft -= iRight;

						if ( iLeft < 0 )
							do
							{
								*pbyDst = *pbySrc;

								pbyDst += 2;
								pbySrc += 2;
								iLeft  += 2;

							} while ( iLeft < 0 );

						iLeft  = Max( iLeftMax,  iRight );
						iRight = Min( iRightMin, *piRight - *( xpiFeatherEdgeLeft + byOffset ));

						int	iDelX = iRight - iLeft;
	
						if ( iDelX > 0 )
							if ( bHatch && 0 == (( y + iFrame ) & 0x04 ))
								memset( pDstOrg2 + iLeft, iColorIndex, iDelX );
							else
								memcpy( pDstOrg2 + iLeft, pData + iV + iLeft, iDelX );

						iLeft  = Max( iLeftMax,  iRight );
						iRight = Min( iRightMin, *piRight + *( xpiFeatherEdgeRight + byOffset ));

						iLeft += !(( byOffset ^ iLeft ) & 0x0001 );

						pbySrc = pData + iV + iLeft;
						pbyDst = pDstOrg2 + iLeft;

						iLeft -= iRight;

						if ( iLeft < 0 )
							do
							{
								*pbyDst = *pbySrc;

								pbyDst += 2;
								pbySrc += 2;
								iLeft  += 2;

							} while ( iLeft < 0 );

						piLeft++;
						piRight++;

						fixV     += fixDV;
						pDstOrg2 += iDirPitch;
					}

					break;
				}

				default:

				{
					int	iSrcWBytes	= m_iBytesPerPixel * iSrcW;

					for ( int y = iTopY; y < iBotY; ++y, ++byOffset )
					{
						int	iV = fixV >> 16;

						int	iLeft  = Max( left,  	 *piLeft - *( xpiFeatherCenter + byOffset ));
						int	iRight = Min( iRightMin, *piLeft + *( xpiFeatherCenter + byOffset ));

						iLeft += !(( byOffset ^ iLeft ) & 0x0001 );

						int	iLeftBytes = iBytesPerPixel * iLeft;
						int	iRowBytes  = iV * iSrcWBytes;

						BYTE const * pbySrc = pData + iRowBytes + iLeftBytes;
						BYTE		  * pbyDst = pDstOrg2 + iLeftBytes;

						iLeft -= iRight;

						if ( iLeft < 0 )
							do
							{
								memcpy( pbyDst, pbySrc, iBytesPerPixel );

								pbyDst += iBytesPerPixel2;
								pbySrc += iBytesPerPixel2;

								iLeft += 2;

							} while ( iLeft < 0 );

						iLeft  = Max( iLeftMax,  iRight );
						iRight = Min( iRightMin, *piRight - *( xpiFeatherEdgeLeft + byOffset ));

						int	iDelXBytes = iBytesPerPixel * ( iRight - iLeft );

						iLeftBytes = iBytesPerPixel * iLeft;

						if ( iDelXBytes > 0 )
							if ( bHatch && 0 == (( y + iFrame ) & 0x04 ))
								memcpy( pDstOrg2 + iLeftBytes, pbyHatchColor, iDelXBytes );
							else
								memcpy( pDstOrg2 + iLeftBytes, pData + iRowBytes + iLeftBytes, iDelXBytes );

						iLeft  = Max( iLeftMax, iRight );
						iRight = Min( iRightMin, *piRight + *( xpiFeatherEdgeRight + byOffset ));

						iLeft += !(( byOffset ^ iLeft ) & 0x0001 );

						iLeftBytes = iBytesPerPixel * iLeft;

						pbySrc = pData + iRowBytes + iLeftBytes;
						pbyDst = pDstOrg2 + iLeftBytes;

						iLeft -= iRight;

						if ( iLeft < 0 )
							do
							{
								memcpy( pbyDst, pbySrc, iBytesPerPixel );

								pbyDst += iBytesPerPixel2;
								pbySrc += iBytesPerPixel2;
								iLeft  += 2;

							} while ( iLeft < 0 );

						piLeft++;
						piRight++;

						fixV     += fixDV;
						pDstOrg2 += iDirPitch;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------
// CSpriteDIB::TerrainDrawQuadVert
//-----------------------------------------------------------------------
void 
CSpriteDIB::TerrainDrawQuadVert(
	int		aiShadeIndex[2],
	CPoint	aptHex[4],
	BOOL		bInvisible ) const
{
	int	iDelY = aptHex[ 3 ].y - aptHex[ 1 ].y;

	if ( 0 >= iDelY )
		return;

	if ( bInvisible )
	{
		aiShadeIndex[0] = x_aiInvisibleShadeIndex[ aiShadeIndex[0] ];
	   aiShadeIndex[1] = x_aiInvisibleShadeIndex[ aiShadeIndex[1] ];
	}

	CDIB			 * pdib 			   = xpdibwnd->GetDIB();
	CDIBits			dibits   	   = pdib->GetBits();
	BYTE const	 * pSrcOrg;
	int				iDstDirPitch	= pdib->GetDirPitch();
	int				iBytesPerPixel = m_iBytesPerPixel;
	int				iSrcWBytes		= Width() * iBytesPerPixel;
	int				fixDV				= ( Height() >> 3 ) << 16;
	int				fixDelY			= iDelY    << 16;

	FIXDIV( fixDV, fixDelY );

	// right/left-dependent vars

	int		iLeftX;
	int		iRightX;
	int		fixTopDV;
	int		fixTopV;
	CPoint	aptHex2[3];

	static int		x_aiVintVfracStepV[2];

	x_aiVintVfracStepV[1] = iSrcWBytes * ( fixDV >> 16 );
	x_aiVintVfracStepV[0] = x_aiVintVfracStepV[1] + iSrcWBytes;

	for ( int i = 0; i < 2; ++i )	// Left then right triangle
	{
		pSrcOrg = TerrainGetDIBPixels( aiShadeIndex[i] );

		if ( !pSrcOrg )
			return;

		if ( 0 == i )
		{
			iLeftX   =  aptHex[ 0 ].x;
			iRightX	=  aptHex[ 1 ].x;
			fixTopV  =  (( Height() >> 3 ) << 15 ) - 0x00008000;
			fixTopDV = -0x00008000;

			aptHex2[0] = CPoint( aptHex[ 0 ].y, aptHex[ 0 ].x );
			aptHex2[1] = CPoint( aptHex[ 1 ].y, aptHex[ 1 ].x );
			aptHex2[2] = CPoint( aptHex[ 3 ].y, aptHex[ 3 ].x );
		}
		else
		{
			iLeftX   =  aptHex[ 1 ].x;
			iRightX	=  aptHex[ 2 ].x;
			fixTopV 	= 	0;
			fixTopDV =  0x00008000;
		
			aptHex2[0] = CPoint( aptHex[ 1 ].y, aptHex[ 1 ].x );
			aptHex2[1] = CPoint( aptHex[ 2 ].y, aptHex[ 2 ].x );
			aptHex2[2] = CPoint( aptHex[ 3 ].y, aptHex[ 3 ].x );
		}

		xscanlist.ScanPoly( aptHex2, 3 );

		iLeftX  = Max(( long )iLeftX,  prVp->left  );
		iRightX = Min(( long )iRightX, prVp->right );

		int			  iDelX		 = iLeftX - aptHex2[0].y;
		int		   * piLeft  	 = xscanlist.m_piLeft  + iDelX;
		int			* piRight 	 = xscanlist.m_piRight + iDelX;
		BYTE const	* pSrcOrgTop = pSrcOrg + iBytesPerPixel * ( iLeftX - aptHex[0].x );

		if ( iDelX != 0 )
		{
			int	fixTemp = iDelX << 16;

			FIXMUL( fixTemp, fixTopDV );

			fixTopV += fixTemp;
		}

		for ( int iDstX = iLeftX; iDstX < iRightX; ++iDstX )
		{
			int	iDstBotY  = *piLeft++;
			int	iDstTopY  = *piRight++;

			int 	iDstBotYClipped = Min(( long )iDstBotY, prVp->bottom - 1L );
			int	iDstTopYClipped = Max(( long )iDstTopY, prVp->top    );

			BYTE *pDst  	 = dibits + pdib->GetOffset( iDstX, iDstTopYClipped );

			int	iDelY		 = iDstTopYClipped - iDstTopY;
			int	fixV      = fixTopV;
			
			if ( iDelY )
			{
				int	fixTemp = iDelY << 16;

				FIXMUL( fixTemp, fixDV );

				fixV += fixTemp;					
			}

			fixTopV += fixTopDV;
			
			static int	nPixels;

			nPixels  = iDstBotYClipped - iDstTopYClipped + 1;

			__asm
			{
				mov	  eax, [nPixels]
				cmp	  eax, 0
				jle	  TexLoopDone2

				; setup initial coordinates
  
 				mov     ecx, [fixV]                	; get v 16.16 fixedpoint coordinate
				mov	  edx, ecx							; copy it
 				shl     ecx, 16                     ; get fractional part
				sar	  edx, 16							; integer part
				imul	  edx, iSrcWBytes				   ; offset of start source pixel

  				mov     esi, [pSrcOrgTop]    			; source address
				add	  esi, edx							; point to start source pixel

				mov	  edi, [pDst]						; dest pointer
				mov	  ebx, [iDstDirPitch]			; Dest row length in bytes
				mov	  edx, [fixDV]						; v delta
 				shl     edx, 16                     ; get fractional part
				mov	  eax, iBytesPerPixel

				push    ebp                         ; free up another register

  				; can't access stack frame

 				; edi = dest dib bits at current pixel
				; esi = texture pointer at current u,v
				; ebx = dest row length, in bytes
				; ecx = v fraction
				; edx = v frac
 				; ebp = v carry scratch

				cmp	eax,	4
				je		TexLoop4Bytes
				cmp	eax,	3
				je		TexLoop3Bytes
				cmp	eax,	2
				je		TexLoop2Bytes

				; 1-byte per pixel case

				mov	ebp, [nPixels]		; # of rows
				add	[nPixels], 3		; unrolled loop count is
				shr	[nPixels], 2		; 	( count + 3 ) / 4
				bt		ebp, 0				; 1 or 3 extra rows?
				jnc	EvenRows				; no, 2 or 4
				bt		ebp, 1				; 3 extra rows?
				jc		TexLoopOneByte3	; yes
				jmp	TexLoopOneByte1	; 1 extra row

				EvenRows:

				bt		ebp, 1				; 2 extra rows?
				jc		TexLoopOneByte2	; yes

				; edi = dest dib bits at current pixel
				; esi = texture pointer at current u,v
				; ebx = dest row length, in bytes
				; ecx = v fraction
				; edx = v frac
  				; ebp = v carry scratch

				mov	al,  	 [edi]								; preread the destination cache line

				TexLoopOneByte4:

				mov	al,    [esi]                    		; get texture pixel 1
  				add   ecx,   edx			            		; increment v fraction
 				sbb   ebp,   ebp                     		; get -1 if carry
				mov   [edi], al                  			; store pixel 1
				add   esi,   x_aiVintVfracStepV[4+ebp*4]	; add in step ints & carries
				add	edi,	 ebx									; bump dest pointer to next row

				TexLoopOneByte3:

				mov	al,    [esi]                    		; get texture pixel 1
  				add   ecx,   edx			            		; increment v fraction
 				sbb   ebp,   ebp                     		; get -1 if carry
				mov   [edi], al                  			; store pixel 1
				add   esi,   x_aiVintVfracStepV[4+ebp*4]	; add in step ints & carries
				add	edi,	 ebx									; bump dest pointer to next row

				TexLoopOneByte2:

				mov	al,    [esi]                    		; get texture pixel 1
  				add   ecx,   edx			            		; increment v fraction
 				sbb   ebp,   ebp                     		; get -1 if carry
				mov   [edi], al                  			; store pixel 1
				add   esi,   x_aiVintVfracStepV[4+ebp*4]	; add in step ints & carries
				add	edi,	 ebx									; bump dest pointer to next row

				TexLoopOneByte1:

				mov	al,    [esi]                    		; get texture pixel 1
  				add   ecx,   edx			            		; increment v fraction
 				sbb   ebp,   ebp                     		; get -1 if carry
				mov   [edi], al                  			; store pixel 1
				add   esi,   x_aiVintVfracStepV[4+ebp*4]	; add in step ints & carries
				add	edi,	 ebx									; bump dest pointer to next row

				dec	[nPixels]									; dec loop counter
				jnz	TexLoopOneByte4							; loop if not done
  				jmp	TexLoopDone

				; 2 bytes per pixel case

				TexLoop2Bytes:

				mov	al,    	[esi+0]                  		; get texture pixel 1
				mov   [edi+0], al                  				; store pixel 1
				mov	al,    	[esi+1]                  		; get texture pixel 1
				mov   [edi+1], al                  				; store pixel 1
  				add   ecx,   	edx			            		; increment v fraction
 				sbb   ebp,   	ebp                     		; get -1 if carry
				add   esi,   	x_aiVintVfracStepV[4+ebp*4]	; add in step ints & carries
				add	edi,	 	ebx									; bump dest pointer to next row
				dec	[nPixels]										; dec loop counter
				jnz	TexLoop2Bytes 									; loop if not done
				jmp	TexLoopDone

				; 3 bytes per pixel case

				TexLoop3Bytes:

				mov	al,    	[esi+0]                  		; get texture pixel 1
				mov   [edi+0], al                  				; store pixel 1
				mov	al,    	[esi+1]                  		; get texture pixel 1
				mov   [edi+1], al                  				; store pixel 1
				mov	al,    	[esi+2]                  		; get texture pixel 1
				mov   [edi+2], al                  				; store pixel 1
 				add   ecx,   	edx			            		; increment v fraction
 				sbb   ebp,   	ebp                     		; get -1 if carry
				add   esi,   	x_aiVintVfracStepV[4+ebp*4]	; add in step ints & carries
				add	edi,	 	ebx									; bump dest pointer to next row
				dec	[nPixels]										; dec loop counter
				jnz	TexLoop3Bytes									; loop if not done
				jmp	TexLoopDone

				; 4 bytes per pixel case

				TexLoop4Bytes:

				mov	al,    	[esi+0]                  		; get texture pixel 1
  				mov   [edi+0], al                  				; store pixel 1
				mov	al,    	[esi+1]                  		; get texture pixel 1
  				mov   [edi+1], al                  				; store pixel 1
				mov	al,    	[esi+2]                  		; get texture pixel 1
  				mov   [edi+2], al                  				; store pixel 1
				mov	al,    	[esi+3]                  		; get texture pixel 1
  				mov   [edi+3], al                  				; store pixel 1
   			add   ecx,   	edx			            		; increment v fraction
 				sbb   ebp,   	ebp                     		; get -1 if carry
  				add   esi,   	x_aiVintVfracStepV[4+ebp*4]	; add in step ints & carries
				add	edi,	 	ebx									; bump dest pointer to next row
				dec	[nPixels]										; dec loop counter
				jnz	TexLoop4Bytes									; loop if not done

				TexLoopDone:

   			pop	ebp												; stack available again

				TexLoopDone2:

				mov	eax, iBytesPerPixel
				add	pSrcOrgTop,	eax								; Bump source pointer to top of next row
			}

		  /*
				while ( nPixels-- )
				{
					memcpy( pDst, pSrcOrgTop + ( fixV >> 16 ) * iSrcWBytes, m_iBytesPerPixel );
				
					pDst += iDstDirPitch;
					fixV += fixDV;
				}

				pSrcOrgTop += iBytesPerPixel;
			*/
		}
	}
}

//-----------------------------------------------------------------------
// CSpriteDIB::TerrainDrawQuadVertHatched
//-----------------------------------------------------------------------
void 
CSpriteDIB::TerrainDrawQuadVertHatched(
	int		aiShadeIndex[2],
	CPoint	aptHex[4],
	BOOL		bInvisible,
	int		iColorIndex ) const
{
	int	iDelY = aptHex[ 3 ].y - aptHex[ 1 ].y;

	if ( 0 >= iDelY )
		return;

	if ( bInvisible )
	{
		aiShadeIndex[0] = x_aiInvisibleShadeIndex[ aiShadeIndex[0] ];
	   aiShadeIndex[1] = x_aiInvisibleShadeIndex[ aiShadeIndex[1] ];
	}

	CDIB			 * pdib 			 = xpdibwnd->GetDIB();
	CDIBits			dibits   	 = pdib->GetBits();
	BYTE const	 * pSrcOrg;
	int				iDstDirPitch	= pdib->GetDirPitch();
	int				iBytesPerPixel = m_iBytesPerPixel;
	int				iSrcWBytes		= Width() * iBytesPerPixel;
	int				fixDV				= ( Height() >> 3 ) << 16;
	int				fixDelY			= iDelY    << 16;
	int				iFrame 			= theGame.GetFrame();

	FIXDIV( fixDV, fixDelY );

	// right/left-dependent vars

	int		iLeftX;
	int		iRightX;
	int		fixTopDV;
	int		fixTopV;
	CPoint	aptHex2[3];

	x_colorbuffer.SetColor( iColorIndex, iBytesPerPixel );

	BYTE const * pbyHatchColor = x_colorbuffer.GetBuffer();

	for ( int i = 0; i < 2; ++i )	// Left then right triangle
	{
		pSrcOrg = TerrainGetDIBPixels( aiShadeIndex[i] );

		if ( !pSrcOrg )
			return;

		if ( 0 == i )
		{
			iLeftX   =  aptHex[ 0 ].x;
			iRightX	=  aptHex[ 1 ].x;
			fixTopV  =  (( Height() >> 3 ) << 15 ) - 0x00008000;
			fixTopDV = -0x00008000;

			aptHex2[0] = CPoint( aptHex[ 0 ].y, aptHex[ 0 ].x );
			aptHex2[1] = CPoint( aptHex[ 1 ].y, aptHex[ 1 ].x );
			aptHex2[2] = CPoint( aptHex[ 3 ].y, aptHex[ 3 ].x );
		}
		else
		{
			iLeftX   =  aptHex[ 1 ].x;
			iRightX	=  aptHex[ 2 ].x;
			fixTopV 	= 	0;
			fixTopDV =  0x00008000;
		
			aptHex2[0] = CPoint( aptHex[ 1 ].y, aptHex[ 1 ].x );
			aptHex2[1] = CPoint( aptHex[ 2 ].y, aptHex[ 2 ].x );
			aptHex2[2] = CPoint( aptHex[ 3 ].y, aptHex[ 3 ].x );
		}

		xscanlist.ScanPoly( aptHex2, 3 );

		iLeftX  = Max(( long )iLeftX,  prVp->left  );
		iRightX = Min(( long )iRightX, prVp->right );

		int			  iDelX		 = iLeftX - aptHex2[0].y;
		int			* piLeft  	 = xscanlist.m_piLeft  + iDelX;
		int			* piRight 	 = xscanlist.m_piRight + iDelX;
		BYTE const	* pSrcOrgTop = pSrcOrg + iBytesPerPixel * ( iLeftX - aptHex[0].x );

		if ( iDelX != 0 )
		{
			int	fixTemp = iDelX << 16;

			FIXMUL( fixTemp, fixTopDV );

			fixTopV += fixTemp;
		}

		for ( int iDstX = iLeftX; iDstX < iRightX; ++iDstX )
		{
			int	iDstBotY  = *piLeft++;
			int	iDstTopY  = *piRight++;

			int 	iDstBotYClipped = Min(( long )iDstBotY, prVp->bottom );
			int	iDstTopYClipped = Max(( long )iDstTopY, prVp->top    );

			BYTE *pDst  	 = dibits + pdib->GetOffset( iDstX, iDstTopYClipped );

			int	iDelY		 = iDstTopYClipped - iDstTopY;
			int	fixV      = fixTopV;
			
			if ( iDelY )
			{
				int	fixTemp = iDelY << 16;

				FIXMUL( fixTemp, fixDV );

				fixV += fixTemp;					
			}

			fixTopV += fixTopDV;
			
			for ( int y = iDstTopYClipped; y < iDstBotYClipped; ++y )
			{
				if ( 0 == (( y + iFrame ) & 0x04 ))
					memcpy( pDst, pbyHatchColor, m_iBytesPerPixel );
				else
					memcpy( pDst, pSrcOrgTop + ( fixV >> 16 ) * iSrcWBytes, m_iBytesPerPixel );
				
				pDst += iDstDirPitch;
				fixV += fixDV;
			}

			pSrcOrgTop += iBytesPerPixel;
		}
	}
}

//--------------------------------------------------------------------------
// CSpriteDIB::VehicleDraw
//--------------------------------------------------------------------------
CRect
CSpriteDIB::VehicleDraw() const
{
	ASSERT_STRICT_VALID( this );

	CPoint	aptVertex[4];

	if ( !VehicleGetWindowVertices( aptVertex ))	// Bitmap faces away from viewer
		return CRect( 0, 0, 0, 0 );

	return VehicleDraw( aptVertex );
}

//--------------------------------------------------------------------------
// CSpriteDIB::VehicleDraw
//--------------------------------------------------------------------------
CRect
CSpriteDIB::VehicleDraw(
	CPoint const	aptVertex[4] ) const
{
	CDIB * pdib = xpdibwnd->GetDIB();

	CRect	rectBound = VehicleCalcBoundingRect( aptVertex );

	// Scan the polygon edges into a buffer

	xscanlist.ScanPolyFixed( aptVertex, 4 );	// static to avoid memory alloc

	// Texture-map the bitmap into the polygon. Walk the destination
	// row by row. 

	int	fixA = ( aptVertex[1].x - aptVertex[0].x ) << 16;
	int	fixB = ( aptVertex[3].x - aptVertex[0].x ) << 16;
	int	fixC = ( aptVertex[1].y - aptVertex[0].y ) << 16;
	int	fixD = ( aptVertex[3].y - aptVertex[0].y ) << 16;

	int	fixTemp = fixB;

	FIXMUL( fixTemp, fixC );

	int	fixDen = fixA;
	
	FIXMUL( fixDen, fixD );
	
	fixDen -= fixTemp;

	if ( 0 == fixDen )
		return CRect( 0, 0, 0, 0 );

	fixB = -fixB;
	fixC = -fixC;

	int	fixSrcW = int( Width()  - 1 ) << 16;
	int	fixSrcH = int( Height() - 1 ) << 16;

	int	fixDUX = fixSrcW;
	int	fixDUY = fixSrcW;
	int	fixDVX = fixSrcH;
	int	fixDVY = fixSrcH;

	FIXMULDIV( fixDUX, fixD, fixDen );
	FIXMULDIV( fixDUY, fixB, fixDen );
	FIXMULDIV( fixDVX, fixC, fixDen );
	FIXMULDIV( fixDVY, fixA, fixDen );

	int	fixU;
	int	fixV;

	int	iDstPitch  = pdib->GetPitch();
	int	iDstH 	  = pdib->GetHeight();
	int	iSrcWBytes = m_iBytesPerPixel * Width();

	int	iY;
	int	iSrcCount = iSrcWBytes * Height();

	// Setup delta values

	static int		x_aiUVintVfracStepV[2];
	static int		x_iDVFrac;
	static int		x_iDUFrac;

	x_iDVFrac = fixDVX << 16;
	x_iDUFrac = fixDUX << 16;

	x_aiUVintVfracStepV[1] = iSrcWBytes * ( fixDVX >> 16 ) + ( fixDUX >> 16 );
	x_aiUVintVfracStepV[0] = x_aiUVintVfracStepV[1] + iSrcWBytes;
    
	BYTE 	const *pSrc = GetDIBPixels();

	if ( !pSrc )
		return CRect( 0, 0, 0, 0 );

	CDIBits	dibits = pdib->GetBits();

	BYTE *pDstOrg = dibits;

	BYTE *pDst;	

	int	iTopY 	= Max( int( prVp->top ),        xscanlist.TopY()    );
	int	iBottomY = Min( int( prVp->bottom - 1 ), xscanlist.BottomY() );
	int	iLength	= Length();

	int *	pfixLeft  = xscanlist.m_piLeft  + iTopY - xscanlist.m_iTopY;
	int *	pfixRight = xscanlist.m_piRight + iTopY - xscanlist.m_iTopY;

	for ( iY = iTopY; iY <= iBottomY; ++iY )
	{
		int fixX 	= Max(( prVp->left << 16 ) + 0x00008000, *pfixLeft++ );
		int iX		= fixX >> 16;
		int fixDelX = fixX - ( aptVertex[0].x << 16 ) - 0x00008000;
		int fixDelY = ( iY - aptVertex[0].y ) << 16;

		fixTemp = fixDelY;

		FIXMUL( fixTemp, fixDUY );

		fixU = fixDelX;

		FIXMUL( fixU, fixDUX );

		fixU += fixTemp + 0x00008000;

		fixTemp = fixDelY;

		FIXMUL( fixTemp, fixDVY );

		fixV = fixDelX;

		FIXMUL( fixV, fixDVX );

		fixV += fixTemp + 0x00008000;

		static int nCols;

		int	fixRight = Min( *pfixRight++, ( prVp->right << 16 ) + 0x00008000 );

		if ( fixX >= fixRight )
			continue;

		nCols	= 1 + (( fixRight - fixX ) >> 16 );
//		nCols = Min( prVp->right - 1, *pfixRight++ >> 16 ) + 1 - iX;

		pDst  = pDstOrg + pdib->GetOffset( iX, iY );

		switch ( m_iBytesPerPixel )
		{
			case 1:

				/*
				while ( nCols-- )
				{
					int	iV = fixV >> 16;
					int	iU = fixU >> 16;

					int iDist = iSrcWBytes * iV + iU;

					if ( 0 <= iDist && iDist < iLength )
					{
						BYTE	byPixel = *( pSrc + iDist );

						if ( 253 != byPixel )
							*pDst = byPixel;
					}

					fixU += fixDUX;
					fixV += fixDVX;

					pDst++;
				}
				*/

				__asm
				{
					; setup initial coordinates
    
    				mov     esi, [fixU]          			; get u 16.16 fixedpoint coordinate
    				mov     ebx, esi                    ; copy it
    				sar     esi, 16                     ; get integer part
    				shl     ebx, 16                     ; get fractional part
    
    				mov     ecx, [fixV]                	; get v 16.16 fixedpoint coordinate
    				mov     edx, ecx                    ; copy it
    				sar     edx, 16                     ; get integer part
    				shl     ecx, 16                     ; get fractional part

    				imul    edx, [iSrcWBytes]      		; calc texture scanline address
    				add     esi, edx                    ; calc texture offset
    				add     esi, [pSrc]         			; calc address

    				mov     edx, [x_iDUFrac]				; get register copy
					mov	  edi, [pDst]						; dest pointer

    				push    ebp                         ; free up another register

    				; can't access stack frame

					; Determine entry point into loop

					mov	ebp, [nCols]		; # of rows
					add	[nCols], 3			; unrolled loop count is
					shr	[nCols], 2			; 	( count + 3 ) / 4
					bt		ebp, 0				; 1 or 3 extra rows?
					jnc	EvenRows				; no, 2 or 4
					bt		ebp, 1				; 3 extra rows?
					jc		TexLoop3				; yes
					jmp	TexLoop1				; 1 extra row

					EvenRows:

					bt		ebp, 1				; 2 extra rows?
					jc		TexLoop2				; yes

    				; edi = dest dib bits at current pixel
    				; esi = texture pointer at current u,v
    				; ebx = u fraction
    				; ecx = v fraction
    				; edx = u frac step
    				; ebp = v carry scratch

					mov	al,  	 [edi]								; preread the destination cache line

					; Texture-map loop, unrolled 4x
TexLoop4:
					mov	al,    [esi]                    		; get texture pixel 1
    				add   ecx,   [x_iDVFrac]            		; increment v fraction
    				sbb   ebp,   ebp                     		; get -1 if carry
					cmp	al,	 253									; a transparent pixel?
					je		SkipPixel4									;	yes, skip it		
    				mov   [edi], al                  			; store pixel 1
SkipPixel4:
    				add   ebx,   edx                     		; increment u fraction
    				adc   esi,   x_aiUVintVfracStepV[4+ebp*4]	; add in step ints & carries
					inc	edi
TexLoop3:
					mov	al,    [esi]                    		; get texture pixel 1
    				add   ecx,   [x_iDVFrac]            		; increment v fraction
    				sbb   ebp,   ebp                     		; get -1 if carry
					cmp	al,	 253									; a transparent pixel?
					je		SkipPixel3									;	yes, skip it		
    				mov   [edi], al                  			; store pixel 1
SkipPixel3:
    				add   ebx,   edx                     		; increment u fraction
    				adc   esi,   x_aiUVintVfracStepV[4+ebp*4]	; add in step ints & carries
					inc	edi
TexLoop2:
					mov	al,    [esi]                    		; get texture pixel 1
    				add   ecx,   [x_iDVFrac]            		; increment v fraction
    				sbb   ebp,   ebp                     		; get -1 if carry
					cmp	al,	 253									; a transparent pixel?
					je		SkipPixel2									;	yes, skip it		
    				mov   [edi], al                  			; store pixel 1
SkipPixel2:
    				add   ebx,   edx                     		; increment u fraction
    				adc   esi,   x_aiUVintVfracStepV[4+ebp*4]	; add in step ints & carries
					inc	edi
TexLoop1:
					mov	al,    [esi]                    		; get texture pixel 1
    				add   ecx,   [x_iDVFrac]            		; increment v fraction
    				sbb   ebp,   ebp                     		; get -1 if carry
					cmp	al,	 253									; a transparent pixel?
					je		SkipPixel1									;	yes, skip it		
    				mov   [edi], al                  			; store pixel 1
SkipPixel1:
    				add   ebx,   edx                     		; increment u fraction
    				adc   esi,   x_aiUVintVfracStepV[4+ebp*4]	; add in step ints & carries
					inc	edi

					dec	[nCols]
					jnz	TexLoop4

    				pop	ebp											; stack available again
				}

				break;

			default:

				BYTE	*pbyMagenta = thePal.GetDeviceColor( 253 );

				while ( nCols-- )
				{
					int	iV = fixV >> 16;
					int	iU = fixU >> 16;

					int iDist = iSrcWBytes * iV + m_iBytesPerPixel * iU;

					if ( 0 <= iDist && iDist < iLength )
						if ( memcmp( pSrc + iDist, pbyMagenta, m_iBytesPerPixel ))
							memcpy( pDst, pSrc + iDist, m_iBytesPerPixel );

					fixU += fixDUX;
					fixV += fixDVX;

					pDst += m_iBytesPerPixel;
				}
		}
	}

	return rectBound;
}

//------------------------- C Q u a d D r a w P a r m s --------------------

//--------------------------------------------------------------------------
// CQuadDrawParms::GetSin
//--------------------------------------------------------------------------
Fixed
CQuadDrawParms::GetSin() const
{
	return g_afixSin[ m_byRotAngle ];
}

//--------------------------------------------------------------------------
// CQuadDrawParms::GetCos
//--------------------------------------------------------------------------
Fixed
CQuadDrawParms::GetCos() const
{
	return g_afixSin[( 64 - m_byRotAngle ) & 0x00ff ];
}

//--------------------------------------------------------------------------
// CQuadDrawParms::SpriteViewToWindow - Sprite view coords -> window client coords
//--------------------------------------------------------------------------
CPoint
CQuadDrawParms::SpriteViewToWindow(
	CPoint      const & ptView,
	CSpriteView const & spriteview ) const
{
	CPoint	ptWindow = ptView;

	ptWindow -= CPoint( spriteview.Width()  >> 1,
							  spriteview.Height() >> 1 );	// To view center coords

	ptWindow += m_ptOffset;

	// Rotate

	Fixed	fixX   = short( ptWindow.x );
	Fixed	fixY   = short( ptWindow.y );

	// GGFIXIT
//	Fixed	fixSin = GetSin();
//	Fixed	fixCos = GetCos();

//	fixX =  fixX * fixCos + fixY * fixSin;
//	fixY = -fixX * fixSin + fixY * fixCos;

	ptWindow.x = fixX.Round();
	ptWindow.y = fixY.Round();

	// Convert to client coords

	ptWindow += m_ptCenter;

	return ptWindow;
}

//--------------------------------------------------------------------------
// CQuadDrawParms::WindowToSpriteView - Window client coords ->  Sprite view coords
//--------------------------------------------------------------------------
CPoint
CQuadDrawParms::WindowToSpriteView(
	CPoint      const & ptWindow,
	CSpriteView const & spriteview ) const
{
	CPoint	ptView = ptWindow;

	// Convert to rotation center window coords

	ptView -= m_ptCenter;

	// Un-rotate

	/*
	Fixed	fixX   = short( ptView.x );
	Fixed	fixY   = short( ptView.y );
	Fixed	fixSin = GetSin();
	Fixed	fixCos = GetCos();

	fixX =  fixX * fixCos - fixY * fixSin;
	fixY =  fixX * fixSin + fixY * fixCos;

	ptView.x = fixX.Round();
	ptView.y = fixY.Round();
	*/

	// Convert to view center coords

	ptView -= m_ptOffset;

	// Convert to view UL coords

	ptView += CPoint( spriteview.Width()  >> 1,
		 			  		spriteview.Height() >> 1 );

	return ptView;
}

//--------------------------------------------------------------------------
// CSpriteDIB::VehicleCalcBoundingRect
//--------------------------------------------------------------------------
CRect										// Bounding rect in window coords
CSpriteDIB::VehicleCalcBoundingRect() const
{
	CPoint	aptVertex[4];

	if ( !VehicleGetWindowVertices( aptVertex ))
		return CRect( 0, 0, 0, 0 );

	return VehicleCalcBoundingRect( aptVertex );
}

//--------------------------------------------------------------------------
// CSpriteDIB::VehicleCalcBoundingRect
//--------------------------------------------------------------------------
CRect										// Bounding rect in window coords
CSpriteDIB::VehicleCalcBoundingRect(
	CPoint const	aptVertex[4] ) const		// Transformed vehicle bitmap 
{														// corners in window coords
	ASSERT_STRICT_VALID( this );

	int	iMinX = aptVertex[0].x;
	int	iMinY = aptVertex[0].y;
	int	iMaxX = aptVertex[0].x;
	int	iMaxY = aptVertex[0].y;
	
	for ( int i = 1; i < 4; ++i )
	{
		if ( aptVertex[i].x < iMinX )
			iMinX = aptVertex[i].x;
		else if ( aptVertex[i].x > iMaxX )
			iMaxX = aptVertex[i].x;

		if ( aptVertex[i].y < iMinY )
			iMinY = aptVertex[i].y;
		else if ( aptVertex[i].y > iMaxY )
			iMaxY = aptVertex[i].y;
	}

	return CRect( iMinX, iMinY, iMaxX + 1, iMaxY + 1 );
}

//--------------------------------------------------------------------------
// CSpriteDIB::VehicleGetWindowVertices - Calc vertices based on offset & rotation
//--------------------------------------------------------------------------
BOOL
CSpriteDIB::VehicleGetWindowVertices(
	CPoint 	aptVertex[4] )	// output
	const
{
	// Origin is center of view bounding rect

	CRect		rectView = GetView()->Rect();

	CPoint	ptCenter( rectView.Width()  >> 1,
							 rectView.Height() >> 1 );

	aptVertex[0] = Rect().TopLeft()
						- rectView.TopLeft()			// to view coords
						+ GetParms()->m_ptOffset	// move the view 
						- ptCenter;						// to view center coords

	aptVertex[1].x = aptVertex[0].x + Width() - 1;
	aptVertex[1].y = aptVertex[0].y;

	aptVertex[2].x = aptVertex[1].x;
	aptVertex[2].y = aptVertex[0].y + Height() - 1;

	aptVertex[3].x = aptVertex[0].x;
	aptVertex[3].y = aptVertex[2].y;

	int	i;

	/*
	Fixed	fixX;
	Fixed	fixY;
	Fixed	fixSin = VehicleGetParms()->GetSin();
	Fixed	fixCos = VehicleGetParms()->GetCos();

	for ( i = 0; i < 3; ++i )
	{
		fixX = short( aptVertex[i].x );
		fixY = short( aptVertex[i].y );

		fixX =  fixX * fixCos + fixY * fixSin;
		fixY = -fixX * fixSin + fixY * fixCos;

		aptVertex[i].x = fixX.Round();
		aptVertex[i].y = fixY.Round();
	}

	aptVertex[3].x = aptVertex[0].x + aptVertex[2].x - aptVertex[1].x;
	aptVertex[3].y = aptVertex[0].y + aptVertex[2].y - aptVertex[1].y;
	*/

	for ( i = 0; i < 4; ++i )
	{
		aptVertex[i].x += VehicleGetParms()->m_ptCenter.x;
		aptVertex[i].y += VehicleGetParms()->m_ptCenter.y;
	}

	ASSERT_STRICT( abs( aptVertex[0].x - aptVertex[1].x - ( aptVertex[3].x - aptVertex[2].x )) <= 1 );
	ASSERT_STRICT( abs( aptVertex[0].x - aptVertex[3].x - ( aptVertex[1].x - aptVertex[2].x )) <= 1 );
	ASSERT_STRICT( abs( aptVertex[0].y - aptVertex[1].y - ( aptVertex[3].y - aptVertex[2].y )) <= 1 );
	ASSERT_STRICT( abs( aptVertex[0].y - aptVertex[3].y - ( aptVertex[1].y - aptVertex[2].y )) <= 1 );

	return ScanList::CheckPoly( aptVertex, 4 );
}

void CTile::Serialize (CArchive & ar)
{
	if (ar.IsStoring ())	
		{
		ar << m_byType;
		}

	else
	  {
		ar >> m_byType;
	  }
}

//-------------------------------------------------------------------------
// CEffectTile::Serialize
//-------------------------------------------------------------------------
void
CEffectTile::Serialize(
	CArchive & ar )
{
	CUnitTile::Serialize( ar );

	if ( !ar.IsStoring() && 0 <= m_iSpriteID )
		m_psprite = theEffects.GetSprite( m_iSpriteID, m_iSpriteIndex );
}

//-------------------------------------------------------------------------
// CAmbient::CAmbient
//-------------------------------------------------------------------------
CAmbient::CAmbient(
	CSpriteView::ANIM_TYPE	eAnim,
	BOOL							bOneShot )
	:
		m_eAnim   ( eAnim ),
		m_bEnabled( TRUE  ),
		m_bPaused ( FALSE ),
		m_bOneShot( FALSE )
{
	Reset();
	SetOneShot( bOneShot );
}

//-------------------------------------------------------------------------
// CAmbient::UpdateFrame
//-------------------------------------------------------------------------
int
CAmbient::UpdateFrame(
	CSpriteView const	* pspriteview )
{
	if ( m_bEnabled && ! m_bPaused )
	{
		unsigned	uHolds = pspriteview->GetAnim( m_eAnim, 0 )->Time();

		DWORD	dwCurTime     = theGame.GettimeGetTime();
		DWORD	dwElapsedTime = dwCurTime - m_dwLastTime;
		DWORD	dwHoldTime	  = uHolds * 1000 / FRAME_RATE;

		if ( dwElapsedTime >= dwHoldTime )
		{
			m_iFrame++;
			m_dwLastTime = dwCurTime;
		}
	}

	int	nFrames = pspriteview->AnimCount( m_eAnim );

	if ( m_bOneShot && m_iFrame >= nFrames )
	{
		Enable( FALSE );

		m_iFrame = nFrames - 1;
	}
	else if ( m_iFrame >= nFrames )
		m_iFrame = 0;

	return m_iFrame;
}

//-------------------------------------------------------------------------
// CAmbient::SetAnimType
//-------------------------------------------------------------------------
void
CAmbient::SetAnimType(
	CSpriteView::ANIM_TYPE eAnim )
{
	m_eAnim = eAnim;
}

//-------------------------------------------------------------------------
// CAmbient::SetFrame
//-------------------------------------------------------------------------
void
CAmbient::SetFrame(
	CSpriteView const * pspriteview,
	int 					  iFrame )
{
	ASSERT( iFrame < pspriteview->AnimCount( m_eAnim ));

	m_iFrame     = iFrame;
	m_dwLastTime = theGame.GettimeGetTime();
}

//-------------------------------------------------------------------------
// CAmbient::Reset
//-------------------------------------------------------------------------
void	
CAmbient::Reset()
{
	m_dwLastTime = theGame.GettimeGetTime();
	m_iFrame     = 0;
}

//-------------------------------------------------------------------------
// CAmbient::Enable
//-------------------------------------------------------------------------
void	
CAmbient::Enable( 
	BOOL bEnable )	
{ 
	m_bEnabled = bEnable;

	Pause( !bEnable );
}

//-------------------------------------------------------------------------
// CAmbient::SetOneShot
//-------------------------------------------------------------------------
void	
CAmbient::SetOneShot( 
	BOOL bOneShot )	
{
//	GGTESTING if ( m_bOneShot == bOneShot )
//		return;

	m_bOneShot = bOneShot;

	if ( m_bOneShot )
		Reset();
}

//-------------------------------------------------------------------------
// CAmbient::Pause
//-------------------------------------------------------------------------
void	
CAmbient::Pause( 
	BOOL bPause )	
{ 
	if ( m_bPaused == bPause || m_bPaused && !m_bEnabled )
		return;

	m_bPaused = bPause;
}

//-------------------------------------------------------------------------
// CAmbient::IsOneShotFinished
//-------------------------------------------------------------------------
BOOL
CAmbient::IsOneShotFinished(
	CSpriteView const	* pspriteview ) const
{
	return m_bOneShot && !m_bEnabled;
}

//-------------------------------------------------------------------------
// CUnitTile::GetFrame
//-------------------------------------------------------------------------
int
CUnitTile::GetFrame(
	CSpriteView	const	     * pspriteview,
	CSpriteView::ANIM_TYPE   eAnim ) const
{
	CAmbient	*pambient = (( CUnitTile * )this )->GetAmbient( eAnim );

	return pambient->UpdateFrame( pspriteview );
}

//-------------------------------------------------------------------------
// CUnitTile::SetFrame
//-------------------------------------------------------------------------
void
CUnitTile::SetFrame(
	CSpriteView	const	     * pspriteview,
	CSpriteView::ANIM_TYPE   eAnim,
	int							 iFrame )
{
	CAmbient	*pambient = (( CUnitTile * )this )->GetAmbient( eAnim );

	pambient->SetFrame( pspriteview, iFrame );
}

//-------------------------------------------------------------------------
// CAmbient::Serialize
//-------------------------------------------------------------------------
void
CAmbient::Serialize(
	CArchive & ar )
{
	if ( ar.IsStoring() )	
	{
		ar << ( int )m_eAnim;

		DWORD	dwDelTime = theGame.GettimeGetTime() - m_dwLastTime;

		ar << dwDelTime;

		ar << m_iFrame;
		ar << m_bEnabled;
		ar << m_bOneShot;
		ar << m_bPaused;
	}
	else
	{
		int	i;

		ar >> i;
		
		m_eAnim = CSpriteView::ANIM_TYPE( i );

		DWORD	dwDelTime;

		ar >> dwDelTime;

		m_dwLastTime = theGame.GettimeGetTime() - dwDelTime;

		ar >> m_iFrame;

		BOOL	b;

		ar >> b;
		Enable( b );
		ar >> b;
		SetOneShot( b );
		ar >> b;
		Pause( b );
	}
}

//-------------------------------------------------------------------------
// CUnitTile::Serialize
//-------------------------------------------------------------------------
void CUnitTile::Serialize( CArchive & ar )
{
	CTile::Serialize( ar );

	if ( ar.IsStoring() )	
	{
		ar << m_bDamage;
		ar << ( BYTE )m_eLayer;
		ar << m_maploc;

		for ( int i = 0; i < CSpriteView::ANIM_COUNT; ++i )
			m_aambient[i].Serialize( ar );

		if ( m_psprite )
		{
			ar << ( short )m_psprite->GetID();
			ar << ( short )m_psprite->GetIndex();
		}
		else
		{
			ar << ( short )-1;
			ar << ( short )-1;
		}
	}

	else
	{
		ar >> m_bDamage;
		BYTE	by;
		ar >> by;
		ar >> m_maploc;
		
		m_eLayer = CStructureSprite::LAYER_TYPE( by );

		for ( int i = 0; i < CSpriteView::ANIM_COUNT; ++i )
			m_aambient[i].Serialize( ar );

		ar >> m_iSpriteID;
		ar >> m_iSpriteIndex;
	}
}

//-------------------------------------------------------------------------
// CSimpleTile::Serialize
//-------------------------------------------------------------------------
void CSimpleTile::Serialize( CArchive & ar )
{
	CTile::Serialize( ar );
}

//-------------------------------------------------------------------------
// CUnitTile::CUnitTile
//-------------------------------------------------------------------------
CUnitTile::CUnitTile()
	:
		CTile( UNIT_TILE ),
		m_eLayer ( CStructureSprite::FOREGROUND_LAYER ), 
		m_maploc ( 0, 0 ),
		m_bDamage( 0 )
{
	for ( int i = 0; i < CSpriteView::ANIM_COUNT; ++i )
		m_aambient[i].SetAnimType( CSpriteView::ANIM_TYPE( i ));
}

//-------------------------------------------------------------------------
// CUnitTile::EnableAnimations
//-------------------------------------------------------------------------
void
CUnitTile::EnableAnimations(
	BOOL	bEnable )
{
	for ( int i = 0; i < CSpriteView::ANIM_COUNT; ++i )
		m_aambient[i].Enable( bEnable );
}

//-------------------------------------------------------------------------
// CUnitTile::PauseAnimations
//-------------------------------------------------------------------------
void
CUnitTile::PauseAnimations(
	BOOL	bPause )
{
	for ( int i = 0; i < CSpriteView::ANIM_COUNT; ++i )
		m_aambient[i].Pause( bPause );
}

//-------------------------------------------------------------------------
// CUnitTile::ResetAnimations
//-------------------------------------------------------------------------
void
CUnitTile::ResetAnimations()
{
	for ( int i = 0; i < CSpriteView::ANIM_COUNT; ++i )
		m_aambient[i].Reset();
}

//-------------------------------------------------------------------------
// CUnitTile::SetOneShotAnimations
//-------------------------------------------------------------------------
void
CUnitTile::SetOneShotAnimations(
	BOOL	bOneShot )
{
	for ( int i = 0; i < CSpriteView::ANIM_COUNT; ++i )
		m_aambient[i].SetOneShot( bOneShot );
}

//-------------------------------------------------------------------------
// CSimpleTile::CSimpleTile
//-------------------------------------------------------------------------
CSimpleTile::CSimpleTile()
	:
		CTile( SIMPLE_TILE )
{
}

//-------------------------------------------------------------------------
// CSpriteView::GetBase
//-------------------------------------------------------------------------
CSpriteDIB const *
CSpriteView::GetBase(
	int iIndex ) const
{
	return m_pspritedibBase + iIndex;
}

//-------------------------------------------------------------------------
// CSpriteView::GetOverlay
//-------------------------------------------------------------------------
CSpriteDIB const *
CSpriteView::GetOverlay(
	int iIndex ) const
{
	return m_pspritedibOverlay + iIndex;
}

//-------------------------------------------------------------------------
// CSpriteView::GetAnim
//-------------------------------------------------------------------------
CSpriteDIB const *
CSpriteView::GetAnim(
	CSpriteView::ANIM_TYPE	eAnim,
	int 							iIndex ) const
{
	return m_apspritedibAnim[ eAnim ] + iIndex;
}

//-------------------------------------------------------------------------
// CSpriteView::GetHotSpotPoint
//-------------------------------------------------------------------------
CPoint
CSpriteView::GetHotSpotPoint(
	CViewCoord const & hotspot,
	int					 iZoom ) const
{
	// Convert view coords to bounded view coords

	CPoint	pt = hotspot.GetPoint( iZoom ) - Rect( iZoom ).TopLeft();

	return pt;
}

//-------------------------------------------------------------------------
// CSpriteView::GetAnchorPoint
//-------------------------------------------------------------------------
CPoint
CSpriteView::GetAnchorPoint(
	int	iZoom ) const
{
	CAnchor const	* panchor = GetAnchor();

	ASSERT( panchor );

	return GetHotSpotPoint( *panchor, iZoom );
}

//-------------------------------------------------------------------------
// CSpriteView::GetHotSpot
//-------------------------------------------------------------------------
CHotSpot	const *
CSpriteView::GetHotSpot(
	CHotSpotKey const & hotspotkey ) const
{
	// FIXIT: Optimize

	for ( int i = 0; i < m_nHotSpots; ++i )
		if ( hotspotkey == m_photspots[ i ].m_key )
			return m_photspots + i;

	return NULL;
}

//---------------------------------------------------------------------------
// CSpriteView::DrawClip
//---------------------------------------------------------------------------
CRect
CSpriteView::DrawClip(
	CDrawParms & drawparms,
	CRect		  * prectClip )
{
	CRect	rect = CDrawParms::GetClipRect();

	if ( prectClip )
		rect &= *prectClip;

	prVp = &rect;

	CRect	rectBound = _Draw( drawparms );

	return rectBound;
}

//---------------------------------------------------------------------------
// CSpriteView::IsHitClip
//---------------------------------------------------------------------------
BOOL
CSpriteView::IsHitClip(
	CDrawParms 		  & drawparms,
	CPoint	  const & ptCursor,
	CRect		  		  * prectClip )
	const
{
	ASSERT_STRICT_VALID (this);

	CRect rect = CDrawParms::GetClipRect();

	if ( prectClip )
	{
		if ( prectClip->IsRectEmpty() )
			return FALSE;

		rect &= *prectClip;
	}

	prVp = &rect;

	CRect		rectBound = m_arect[ xiZoom ];

	CSpriteDIB::SetParms( drawparms, this );

	CSpriteDIB const	*pspritedib;
	int					 i, j;

	for ( i = 0; i < BaseCount(); ++i )
	{
		pspritedib = GetBase( i );

		if ( pspritedib->IsHit( ptCursor ))
			return TRUE;
	}

	CTile	* ptile = drawparms.m_ptile;

	for ( j = 0; j < ANIM_COUNT; ++j )
	{
		BOOL	bCheckLayer = TRUE;

		if ( CTile::UNIT_TILE == ptile->GetTileType() )
			bCheckLayer = (( CUnitTile * )ptile )->GetAmbient(( CSpriteView::ANIM_TYPE )j )->IsEnabled();

		if ( bCheckLayer )
			for ( i = 0; i < AnimCount( ANIM_TYPE( j )); ++i )
			{
				pspritedib = GetAnim( ANIM_TYPE( j ), i );

				if ( pspritedib->IsHit( ptCursor ))
					return TRUE;
			}
	}

	return FALSE;
}

//---------------------------------------------------------------------------
// CSpriteView::IsHit
//---------------------------------------------------------------------------
BOOL
CSpriteView::IsHit(
	CDrawParms 		  & drawparms,
	CPoint	  const & ptCursor )
	const
{
	return IsHitClip( drawparms, ptCursor, NULL );
}

//---------------------------------------------------------------------------
// CSpriteView::Draw
//---------------------------------------------------------------------------
CRect										// Bounding rect for bitmaps drawn
CSpriteView::Draw(
	CDrawParms & drawparms )
{
	CRect rect = CDrawParms::GetClipRect();

	prVp = &rect;

	CRect	rectBound = _Draw( drawparms );

	return rectBound;
}

//---------------------------------------------------------------------------
// CSpriteView::_Draw
//---------------------------------------------------------------------------
CRect									// Bounding rect for bitmaps drawn
CSpriteView::_Draw(
	CDrawParms & drawparms )
{
	ASSERT_STRICT_VALID (this);

	CSpriteDIB::SetParms( drawparms, this );

	CSpriteDIB const	*pspritedib;
	int					 i;

	CRect	rectBound = CRect( 0, 0, 0, 0 );

	// Note: everything puts x,y in the U.L. corner (Windows coordinates)

	// Draw back animations

	if ( 0 < AnimCount( ANIM_BACK_1 ))
	{
		ASSERT_STRICT( CTile::UNIT_TILE == CSpriteDIB::GetParms()->m_ptile->GetTileType() );
		rectBound |= DrawUnitAnimation( ANIM_BACK_1 );
	}

	if ( 0 < AnimCount( ANIM_BACK_2 ))
	{
		ASSERT_STRICT( CTile::UNIT_TILE == CSpriteDIB::GetParms()->m_ptile->GetTileType() );
		rectBound |= DrawUnitAnimation( ANIM_BACK_2 );
	}

	// draw the base bitmaps

	for ( i = 0; i < BaseCount(); ++i )
	{
		pspritedib = GetBase( i );
		
		if ( CDrawParms::IsDrawMode() )
			rectBound |= pspritedib->Draw();

		if ( CDrawParms::IsInvalidateMode() )
			rectBound |= pspritedib->CalcBoundingRect();
	}

	// Draw overlays

	for ( i = 0; i < OverlayCount(); ++i )
	{
		pspritedib = GetOverlay( i );
		
		if ( CDrawParms::IsDrawMode() )
			rectBound |= pspritedib->Draw();

		if ( CDrawParms::IsInvalidateMode() )
			rectBound |= pspritedib->CalcBoundingRect();
	}

	// Draw front animations

	if ( 0 < AnimCount( ANIM_FRONT_1 ))
		if (  CTile::UNIT_TILE == CSpriteDIB::GetParms()->m_ptile->GetTileType() )
			rectBound |= DrawUnitAnimation( ANIM_FRONT_1 );
		else
			rectBound |= DrawSimpleAnimation();

	if ( 0 < AnimCount( ANIM_FRONT_2 ))
	{
		ASSERT_STRICT( CTile::UNIT_TILE == CSpriteDIB::GetParms()->m_ptile->GetTileType() );
		rectBound |= DrawUnitAnimation( ANIM_FRONT_2 );
	}

	return rectBound;
}

//---------------------------------------------------------------------------
// CSpriteView::DrawSelected
//---------------------------------------------------------------------------
void
CSpriteView::DrawSelected(
	CRect const & rect,
	int			  iColor )
{
	CDIB * pdib = xpdibwnd->GetDIB();

	CRect rectWnd = CDrawParms::GetClipRect();

	prVp = &rectWnd;

	if ( rect.left   >= prVp->right  ||
		  rect.right  <= prVp->left   ||
		  rect.top    >= prVp->bottom ||
		  rect.bottom <= prVp->top )

		return;

	int	i;
	int	iCount   = 16 >> xiZoom;

	x_colorbuffer.SetColor( iColor, rect.Width() );

	BYTE const * pbyColor = x_colorbuffer.GetBuffer();
	CDIBits	    dibits   = pdib->GetBits();

	BYTE *pDstOrg  		= dibits;
	BYTE *pDst;	

	int	iBytesPerPixel = ptrthebltformat->GetBytesPerPixel();
	int	iDstH 	  		= pdib->GetHeight();
	int	iDstDirPitch	= pdib->GetDirPitch();

	int	iLeftX1      = Max( 0L, rect.left );
	int	iLeftX2      = Min( rectWnd.right, (long )( rect.left + iCount ));
	int	iRightX1     = Max( 0L, ( long )( rect.right - iCount ));
	int	iRightX2     = Min( rectWnd.right, rect.right );
	int	iTopY1       = Max( 0L, rect.top );
	int	iTopY2       = Min( rectWnd.bottom, ( long )( rect.top + iCount ));
	int	iBottomY1    = Max( 0L, ( long )( rect.bottom - iCount ));
	int	iBottomY2    = Min( rectWnd.bottom, rect.bottom );

	int	iLeftCount   = ( iLeftX2   - iLeftX1   ) * iBytesPerPixel;
	int	iRightCount  = ( iRightX2  - iRightX1  ) * iBytesPerPixel;
	int	iTopCount    = ( iTopY2    - iTopY1    ) * iBytesPerPixel;
	int	iBottomCount = ( iBottomY2 - iBottomY1 ) * iBytesPerPixel;

	// Horizontal Lines

	if ( rect.top >= 0 )
	{
		if ( iLeftCount > 0 )
		{
			ASSERT_STRICT( iLeftX1 >= 0 && iLeftX1 < rectWnd.right );

			pDst = pDstOrg + pdib->GetOffset( iLeftX1, iTopY1 );

			memcpy( pDst, pbyColor, iLeftCount );
		}

		if ( iRightCount > 0 )
		{
			ASSERT_STRICT( iRightX1 >= 0 && iRightX1 < rectWnd.right );

			pDst = pDstOrg + pdib->GetOffset( iRightX1, iTopY1 );

			memcpy( pDst, pbyColor, iRightCount );
		}
	}

	if ( rect.bottom <= rectWnd.bottom )
	{
		if ( iLeftCount > 0 )
		{
			ASSERT_STRICT( iLeftX1 >= 0 && iLeftX1 < rectWnd.right );

			pDst = pDstOrg + pdib->GetOffset( iLeftX1, rect.bottom - 1 );

			memcpy( pDst, pbyColor, iLeftCount );
		}

		if ( iRightCount > 0 )
		{
			ASSERT_STRICT( iRightX1 >= 0 && iRightX1 < rectWnd.right );

			pDst = pDstOrg + pdib->GetOffset( iRightX1, rect.bottom - 1 );

			memcpy( pDst, pbyColor, iRightCount );
		}
	}

	// Vertical Lines

	if ( rect.left >= 0 )
	{
		if ( iTopCount > 0 )
		{
			ASSERT_STRICT( rect.left >= 0 && rect.left < rectWnd.right );

			pDst = pDstOrg + pdib->GetOffset( rect.left, iTopY1 );

  			for ( i = iTopY1; i < iTopY2; ++i, pDst += iDstDirPitch )
				memcpy( pDst, pbyColor, iBytesPerPixel );
		}

		if ( iBottomCount > 0 )
		{
			ASSERT_STRICT( rect.left >= 0 && rect.left < rectWnd.right );

			pDst = pDstOrg + pdib->GetOffset( rect.left, iBottomY1 );

			for ( i = iBottomY1; i < iBottomY2; ++i, pDst += iDstDirPitch )
				memcpy( pDst, pbyColor, iBytesPerPixel );
		}
	}

	if ( rect.right <= rectWnd.right )
	{
		if ( iTopCount > 0 )
		{
			ASSERT_STRICT( rect.right > 0 && rect.right <= rectWnd.right );

			pDst = pDstOrg + pdib->GetOffset( rect.right - 1, iTopY1 );

			for ( i = iTopY1; i < iTopY2; ++i, pDst += iDstDirPitch )
				memcpy( pDst, pbyColor, iBytesPerPixel );
		}

		if ( iBottomCount > 0 )
		{
			ASSERT_STRICT( rect.right > 0 && rect.right <= rectWnd.right );

			pDst = pDstOrg + pdib->GetOffset( rect.right - 1, iBottomY1 );

			for ( i = iBottomY1; i < iBottomY2; ++i, pDst += iDstDirPitch )
				memcpy( pDst, pbyColor, iBytesPerPixel );
		}
	}
}

//---------------------------------------------------------------------------
// CSpriteView::DrawUnitAnimation
//---------------------------------------------------------------------------
CRect
CSpriteView::DrawUnitAnimation(
	ANIM_TYPE	 eAnim )
{
	CUnitTile	* punittile = ( CUnitTile * )CSpriteDIB::GetParms()->m_ptile;
	CAmbient		* pambient  = punittile->GetAmbient( eAnim );

	int	iFrame = pambient->UpdateFrame( this );	// Disables when one-shot finished

	CSpriteDIB const *pspritedib = GetAnim( eAnim, iFrame );
	CRect					rectBound;

	if ( CDrawParms::IsInvalidateMode() )
	{
		rectBound = pspritedib->CalcBoundingRect();

		xpanimatr->GetDirtyRects()->AddRect( &rectBound, CDirtyRects::LIST_PAINT_BOTH );
	}

	if ( CDrawParms::IsDrawMode() && pambient->IsEnabled() )
		rectBound = pspritedib->Draw();

	return rectBound;
}

//---------------------------------------------------------------------------
// CSpriteView::DrawSimpleAnimation
//---------------------------------------------------------------------------
CRect
CSpriteView::DrawSimpleAnimation()
{
	ANIM_TYPE			  eAnim		  = ANIM_FRONT_1;
	int					  nHold		  = GetAnim( eAnim, 0 )->Time(); // GGFIXIT? Assumes all frames have same hold time
	int					  iFrame      = theGame.GetFrame() % ( nHold * AnimCount( eAnim ));
	int					  iAnim       = iFrame / nHold;
	int					  iHoldLeft   = iFrame % nHold;
	CSpriteDIB const  * pspritedib  = GetAnim( eAnim, iAnim );
	CRect					  rectBound;

	rectBound = pspritedib->CalcBoundingRect();

	if ( CDrawParms::IsInvalidateMode() && 0 == iHoldLeft )
			xpanimatr->GetDirtyRects()->AddRect( &rectBound );

	if ( CDrawParms::IsDrawMode() )
		rectBound = pspritedib->Draw();

	return rectBound;
}

//---------------------------------------------------------------------------
// CSprite
							 
//-------------------------------------------------------------------------
// CSprite::GetSpriteView
//-------------------------------------------------------------------------
CSpriteView *
CSprite::GetSpriteView(
	int   iIndex,
	BOOL	bCheckValid ) const
{
	CSpriteView	* pspriteview = ( CSpriteView * )(( BYTE * )m_ptrspritehdr.Value() + m_piViewOffsets[ iIndex ] );

	#ifdef _DEBUG
	if ( bCheckValid )
		pspriteview->CheckValid();
	#endif

	return pspriteview;
}

//-------------------------------------------------------------------------
// CSprite::GetDIBPixels
//-------------------------------------------------------------------------
BYTE const *
CSprite::GetDIBPixels(
	int 					  	  iSuperviewIndex,
	CDIBLayoutInfo const & diblayoutinfo )
{
	#ifdef _DEBUG
	diblayoutinfo.CheckValid();
	#endif

	BYTE const	*pbySuperview = GetDecompressedSuperview( iSuperviewIndex, xiZoom );

	if ( pbySuperview )
	{
		BYTE const * pbyDIB = pbySuperview + diblayoutinfo.m_blockinfoDecompressed[ m_iBytesPerPixel - 1 ].m_lOffset;

		ASSERT( AfxIsValidAddress( pbyDIB, diblayoutinfo.m_blockinfoDecompressed[ m_iBytesPerPixel - 1 ].m_lLength ));

		return pbyDIB;
	}

	return NULL;
}

//---------------------------------------------------------------------------
// CSprite::ColorConvert
//---------------------------------------------------------------------------
void
CSprite::ColorConvert(
	BYTE 		  	* pbyDst,	
	BYTE const 	* pbySrc,		// 24 bpp
	long			  lLenDst,
	long			  lLenSrc )
{
	BOOL	bCompressed = TERRAIN != m_iType &&
							  VEHICLE != m_iType;

	BYTE const	* pbySrcEnd	= pbySrc + lLenSrc;
	BYTE const	* pbyDstEnd	= pbyDst + lLenDst;
	WORD			* pwDst    	= ( WORD * )pbyDst;

	if ( !bCompressed )
	{
		ASSERT( 0 == lLenSrc % 3 );

		switch ( m_iBitsPerPixel )
		{
			case 15:
		
				for ( ; pbySrc < pbySrcEnd; pbySrc += 3 )
				{
					ASSERT(( BYTE * )pwDst + 1 < pbyDstEnd );

					*pwDst++ = ( WORD( pbySrc[2] >> 3 ) << 10 ) |
								  ( WORD( pbySrc[1] >> 3 ) <<  5 ) |
								  ( WORD( pbySrc[0] >> 3 ));
				}

				break;

			case 16:

				for ( ; pbySrc < pbySrcEnd; pbySrc += 3 )
				{
					ASSERT( ( BYTE * )pwDst + 1 < pbyDstEnd );

					*pwDst++ = ( WORD( pbySrc[2] >> 3 ) << 11 ) |
								  ( WORD( pbySrc[1] >> 2 ) <<  5 ) |
								  ( WORD( pbySrc[0] >> 3 ));
				}

				break;

			case 32:

				for ( ; pbySrc < pbySrcEnd; pbySrc += 3 )
				{
					ASSERT( pbyDst + 3 < pbyDstEnd );

					*pbyDst++ = pbySrc[0];
					*pbyDst++ = pbySrc[1];
					*pbyDst++ = pbySrc[2];
					*pbyDst++ = 0;
				}

				break;

			default: ASSERT( 0 );
		}
	}
	else
	{
		#ifdef _GG
		// GGTRAP(); ok
		#endif

		long	 lCount;

		while ( pbySrc < pbySrcEnd )
			for ( ;; )
			{
				ASSERT( pbyDst + 3 < pbyDstEnd );
				ASSERT( pbySrc + 3 < pbySrcEnd );

				*( long * )pbyDst = m_iBytesPerPixel * *( long * )pbySrc / 3;

				pbySrc += sizeof( long );
				pbyDst += sizeof( long );

				ASSERT( pbyDst + 3 < pbyDstEnd );
				ASSERT( pbySrc + 3 < pbySrcEnd );

				*( long * )pbyDst = m_iBytesPerPixel * *( long * )pbySrc / 3;

				lCount = *( long * )pbySrc;

				pbySrc += sizeof( long );
				pbyDst += sizeof( long );

				if ( 0L == lCount )
					break;

				BYTE const * pbySrcBlockEnd = pbySrc + lCount;

				ASSERT_STRICT( pbySrcBlockEnd <= pbyData + lLen );
				ASSERT_STRICT( 0 == lCount % 3 );

				switch ( m_iBitsPerPixel )
				{
					case 15:

						pwDst = ( WORD * )pbyDst;

						for ( ; pbySrc < pbySrcBlockEnd; pbySrc += 3 )
						{
							ASSERT(( BYTE * )pwDst + 1 < pbyDstEnd );
							ASSERT( pbySrc + 2 < pbySrcEnd );

							*pwDst++ = ( WORD( pbySrc[2] >> 3 ) << 10 ) |
									  	  ( WORD( pbySrc[1] >> 3 ) <<  5 ) |
									  	  ( WORD( pbySrc[0] >> 3 ));
						}

						pbyDst = ( BYTE * )pwDst;

						break;

					case 16:

						pwDst = ( WORD * )pbyDst;
					
						for ( ; pbySrc < pbySrcBlockEnd; pbySrc += 3 )
						{
							ASSERT(( BYTE * )pwDst + 1 < pbyDstEnd );
							ASSERT( pbySrc + 2 < pbySrcEnd );

							*pwDst++ = ( WORD( pbySrc[2] >> 3 ) << 11 ) |
										  ( WORD( pbySrc[1] >> 2 ) <<  5 ) |
										  ( WORD( pbySrc[0] >> 3 ));
						}

						pbyDst = ( BYTE * )pwDst;

						break;

					case 32:

						for ( ; pbySrc < pbySrcBlockEnd; pbySrc += 3 )
						{
							ASSERT( pbyDst + 3 < pbyDstEnd );
							ASSERT( pbySrc + 2 < pbySrcEnd );

							*pbyDst++ = pbySrc[0];
							*pbyDst++ = pbySrc[1];
							*pbyDst++ = pbySrc[2];
							*pbyDst++ = 0;
						}		

						break;

					default: ASSERT_STRICT( 0 );
				}
			}
	}
}

//---------------------------------------------------------------------------
// CTerrainSprite

//-------------------------------------------------------------------------
// CTerrainSprite::GetView
//-------------------------------------------------------------------------
CSpriteView *
CTerrainSprite::GetView(
	int iDir,
	int iDamage )
{
	return m_aapspriteview[ iDir ][ iDamage ];
}

//---------------------------------------------------------------------------
// CEffectSprite

//---------------------------------------------------------------------------
// CStructureSprite

//-------------------------------------------------------------------------
// CStructureSprite::GetView
//-------------------------------------------------------------------------
CSpriteView *
CStructureSprite::GetView(
	int 	iDir,
	int 	iLayer,
	int 	iStage,
	int 	iDamage )
{
	return m_aaaapspriteview[ iDir ][ iLayer ][ iStage ][ iDamage ];
}

//-------------------------------------------------------------------------
// CVehicleSprite

//-------------------------------------------------------------------------
// CVehicleSprite::GetView
//-------------------------------------------------------------------------
CSpriteView *
CVehicleSprite::GetView(
	int iDir,
	int iTilt,
	int iDamage )
{
	return m_aaapspriteview[ iDir ][ iTilt ][ iDamage ];
}

//-------------------------------------------------------------------------
// CSpriteCollection::CSpriteCollection
//-------------------------------------------------------------------------
CSpriteCollection::CSpriteCollection(
	const char * pszRif )
	:
		m_pptrsprite( NULL ),
		m_nSprite   ( 0 ),
		m_bOpen		( FALSE ),
		m_strRif		( pszRif )
{
	m_bLoaded = 0;
}

//-------------------------------------------------------------------------
// CSpriteCollection::GetSprite
//-------------------------------------------------------------------------
CSprite const *
CSpriteCollection::GetSprite(
	int 	iID,
	int	iIndex,
	BOOL	bStrict ) const
{
	return (( CSpriteCollection * )this )->GetSprite( iID, iIndex, bStrict );
}

//-------------------------------------------------------------------------
// CSpriteCollection::GetSprite
//-------------------------------------------------------------------------
CSprite *
CSpriteCollection::GetSprite(
	int 	iID,
	int	iIndex,
	BOOL	bStrict )
{
	ASSERT_VALID( this );
	ASSERT( m_bOpen );

	ASSERT_VALID( m_ptrspritecollectioninfo.Value() );

	int iSpriteIndex = m_ptrspritecollectioninfo->GetIndex ( iID, iIndex, bStrict );

	if ( -1 == iSpriteIndex )
		return NULL;

	#ifdef _DEBUG
	m_pptrsprite[ iSpriteIndex ]->CheckValid();
	#endif

	return m_pptrsprite[ iSpriteIndex ].Value();
}

//-------------------------------------------------------------------------
// CSpriteCollection::GetCount
//-------------------------------------------------------------------------
int
CSpriteCollection::GetCount(
	int 	iID ) const
{
	ASSERT_VALID( this );

	return m_ptrspritecollectioninfo->GetCount( iID );
}

//-------------------------------------------------------------------------
// CSpriteCollection::GetFile
//-------------------------------------------------------------------------
CFile *
CSpriteCollection::GetFile()
{
	if ( !m_ptrfile.Value() )
		m_ptrfile = theDataFile.OpenAsFile( GetRifName() );

	return m_ptrfile.Value();
}

//--------------------------------------------------------------------------
// CSpriteCollectionInfo::GetCount
//--------------------------------------------------------------------------
int
CSpriteCollectionInfo::GetCount(
	int iID ) const
{
	ASSERT( iID <= m_iMaxID );

	iID = MapID( iID, FALSE );

	return m_pspriteidinfo[ iID ].GetCount();
}

//--------------------------------------------------------------------------
// CSpriteCollectionInfo::GetIndex
//--------------------------------------------------------------------------
int
CSpriteCollectionInfo::GetIndex(
	int	iID,
	int	iOffset,
	BOOL	bStrict ) const
{
	iID = MapID( iID, bStrict );

	if ( -1 == iID )
		return -1;

	iOffset = MapOffset( iID, iOffset, bStrict );

	if ( -1 == iOffset )
		return -1;

	return m_pspriteidinfo[ iID ].GetIndex() + iOffset;
}

//-------------------------------------------------------------------------
// CSpriteCollectionInfo::MapID
//-------------------------------------------------------------------------
int
CSpriteCollectionInfo::MapID(
	int 	iID,
	BOOL	bStrict ) const
{
	ASSERT_VALID( this );

	if ( iID <= m_iMaxID && 0 < m_pspriteidinfo[ iID ].GetCount() )
		return iID;

	if ( bStrict )
		return -1;

	for ( iID = 0; iID <= m_iMaxID; ++iID )
		if ( 0 < m_pspriteidinfo[ iID ].GetCount() )
			return iID;

	ASSERT( FALSE );
	ThrowError( ERR_TLP_BAD_SPRITE_ID );

	return -1;
}

//-------------------------------------------------------------------------
// CSpriteCollectionInfo::MapOffset
//-------------------------------------------------------------------------
int
CSpriteCollectionInfo::MapOffset(
	int 	iID,
	int	iOffset,
	BOOL	bStrict ) const
{
	ASSERT_STRICT_VALID( this );

	ASSERT( iID <= m_iMaxID );

	if ( m_pspriteidinfo[ iID ].GetCount() <= iOffset )
		if ( bStrict || 0 == m_pspriteidinfo[ iID ].GetCount() )
			return -1;
		else
			return 0;

	return iOffset;
}

// .SPR sprite layout

/*

Old Format
----------

type
w
h
bitsperpixel
# views
<views>
<view indices>

<view>
------
w
h
bitsperpixel
<anchor>
# hotspots
<hotspots>
# base
<base bitmaps>
#overlay
<overlay bitmaps>
#anim front 1
<anim front 1 bitmaps>	4 zooms
#anim front 2
<anim front 2 bitmaps>	4 zooms
#anim back 1
<anim back 1 bitmaps>	4 zooms
#anim back 2
<anim back 2 bitmaps>	4 zooms

<bitmap>
--------
id
time
x
y
w
h
len
<pixel data>

<view indices>
--------------
<building view indices> OR
<vehicle view indices> OR
<terrain view indices>

<building view indices> - views for a structure sprite
-----------------------
stage x layer x dir x damage

<vehicle view indices> - views for a vehicle sprite
----------------------
tilt x dir x damage

<terrain view indices> - views for a terrain sprite (as indices into the array of actual views) 
---------------------- - -1 = this view not present
dir x damage

<hotspot>
---------
type
x
y
index

-----------------------------------------------------------------------

New format
----------

<sprite>	variable-size			
----------------------
id							
total len
<-----------.SPR starts here
type
hdr len

compression type
# views
<# superviews>

<zoom layouts>						4 zooms
<superview layouts>				# superviews x 4 zooms
view hdr offsets					# views
<view hdrs>							# views
<view hdr indices>				variable

data len
<superviews>						4 zooms (3 to 0) x # superviews per zoom

<view hdr> variable-size									
--------------------
reserved - 128 bytes
<anchor point>						
superview index
# hotspots							
# base bitmaps						
# overlay bitmaps
# anim front 1 bitmaps
# anim front 2 bitmaps
# anim back 1 bitmaps
# anim back 2 bitmaps
<hotspots>							# hotspots
<base bitmap hdrs>				# base bitmaps
<overlay bitmap hdrs>			# overlay bitmaps
<anim front 1 bitmap hdrs>		# anim front 1 bitmaps
<anim front 2 bitmap hdrs>		# anim front 2 bitmaps
<anim back 1 bitmap hdrs>		# anim back 1 bitmaps
<anim back 2 bitmap hdrs>		# anim back 2 bitmaps

<# superviews>
--------------
4 for BUILDING, 1 for others

<hotspot> fixed-size	
--------------------
<hotspot points>					4 zooms
index			
type

<point>
-------
x
y

<bitmap hdr> fixed-size
-----------------------
<reserved 8>
type
bytes-per-pixel
hold time
<rects>								4 zooms
<bitmap layouts>					4 zooms

<view hdr indices>
------------------
<building view indices> OR
<vehicle  view indices> OR
<terrain  view indices>

<building view indices> - views for a structure sprite
-----------------------
dir x stage x layer x damage

<vehicle view indices> - views for a vehicle sprite
----------------------
dir x tilt x damage

<terrain view indices> - views for a terrain sprite (as indices into the array of actual views) 
---------------------- - -1 = this view not present
dir x damage

<zoom layout>
-------------
offset 	(from start of zoom layouts)
length

<bitmap layout>	Offsets are from the start of the bitmap's superview
---------------
decompressed offset - 1 bpp
decompressed length - 1 bpp
decompressed offset - 2 bpp
decompressed length - 2 bpp
decompressed offset - 3 bpp
decompressed length - 3 bpp
decompressed offset - 4 bpp
decompressed length - 4 bpp

<superview layout>
------------------
compressed offset			Offset from start of first superview
compressed length
decompressed length - 1 bpp
decompressed length - 2 bpp
decompressed length - 3 bpp
decompressed length - 4 bpp

<superview>
-----------
block of compressed dibs (data only)

*/
