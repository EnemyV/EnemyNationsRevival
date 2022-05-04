//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

#ifndef __SPRITE_H__
#define __SPRITE_H__

// sprite.h : header file for sprites
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "base.h"

const int N_SHADES		  			= 8;	// GG: # of shade levels for terrain triangles
const int NUM_ZOOM_LEVELS 			= 4;
const int NUM_BLDG_DIRECTIONS 	= 4;
const int NUM_TERRAIN_DIRECTIONS = NUM_BLDG_DIRECTIONS;
const int NUM_VEHICLE_TILTS      = 3;
const int NUM_VEHICLE_DIRECTIONS = 8;
const int NUM_DAMAGE_LEVELS 		= 4;
const int NUM_TERRAIN_DAMAGE 		= NUM_DAMAGE_LEVELS;
const int NUM_VEHICLE_DAMAGE 		= NUM_DAMAGE_LEVELS;
const int NUM_BLDG_DAMAGE 			= NUM_DAMAGE_LEVELS;
const int NUM_BLDG_STAGES 			= 4;
const int NUM_BLDG_LAYERS 			= 2;
const int MAX_DIB_TYPES	 			= 5;
const int MAX_SPRITE_TYPES	 		= MAX_DIB_TYPES;
const int DUMMY_SPRITE				= -1;

extern int			xiZoom;
extern int			xiDir;
extern BOOL			bForceDraw;
extern BOOL			bInvAmb;

extern 			CDIBWnd  *xpdibwnd;
extern const 	CAnimAtr	*xpanimatr;

//------------------------ C S p r i t e D I B D a t a --------------------

class CSpriteView;

class CSpriteDIBParms
#ifdef _DEBUG
: public CObject
#endif
{

public:

	CSpriteDIBParms( unsigned uTime,
						  int		  iType,
				  		  int		  iBitsPerPixel );

	unsigned		  m_uTime;
	int			  m_iType;
	int			  m_iBitsPerPixel;
	CSpriteView	* m_pspriteview;

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif
};

//---------------------------- C D r a w P a r m s ------------------------

class CTile;
class CSpriteView;

class CDrawParms
{

public:

	enum	UPDATE_MODE
	{
		invalidate = 0x01,	// Invalidate bounding rects
		draw		  = 0x02 	// Draw and update ambients
	};

	CDrawParms()	{}

	CDrawParms( CTile & tile, CPoint & ptOffset )
		:
			m_ptile   ( &tile        ),
			m_ptOffset(  ptOffset    ),
			m_bTransparentHitTest( FALSE )
	{}

	virtual CPoint SpriteViewToWindow( CPoint const & pt, CSpriteView const & ) const { return pt + m_ptOffset; }

	CTile		  * m_ptile;
	CPoint	  	 m_ptOffset;
	BOOL			 m_bTransparentHitTest;

	static void	SetUpdateFlags( unsigned uUpdateFlags ) { g_uUpdateFlags	= uUpdateFlags; }

	static BOOL		IsInvalidateMode() { return 0 != ( invalidate & g_uUpdateFlags ); }
	static BOOL		IsDrawMode() 		 { return 0 != ( draw       & g_uUpdateFlags ); }
	static CRect	GetClipRect() 		 { return g_rectClip; }

	static void		SetClipRect( CRect const & rect ) { g_rectClip = rect; }

private:

	static unsigned	g_uUpdateFlags;
	static CRect		g_rectClip;
};

//--------------------- C T e r r a i n D r a w P a r m s ------------------

class CTerrainDrawParms : public CDrawParms
{

public:

	enum FEATHER_TYPE
	{
		FEATHER_INOUT,
		FEATHER_IN,
		FEATHER_OUT,
		FEATHER_NONE
	};

	enum FEATHER_DIR
	{
		FEATHER_TL,
		FEATHER_BL,
		FEATHER_TR,
		FEATHER_BR,
	};

	CTerrainDrawParms() {}

	CTerrainDrawParms( CTile           & tile,
							 CHexCoord const & hexcoord,
							 BOOL					 bDrawVert,
							 BOOL					 bShade,
							 FEATHER_TYPE		 aeFeather[4] ) // Neighbor edge feathering, in order of FEATHER_DIR
		:
			CDrawParms 		( tile, CPoint( 0, 0 )),
			m_hexcoord 		( hexcoord ),
			m_bDrawVert		( bDrawVert ),
			m_bShade   		( bShade )
	{
		memcpy( m_aeFeather, aeFeather, sizeof( m_aeFeather ));
	}

	CHexCoord		m_hexcoord;
	BOOL				m_bDrawVert;
	BOOL				m_bShade;
	FEATHER_TYPE	m_aeFeather[4];
};

//------------------------ C Q u a d D r a w P a r m s --------------------

class CQuadDrawParms : public CDrawParms
{

public:

	CQuadDrawParms() {}

	CQuadDrawParms( CTile  			& tile,
						 CPoint 			& ptOffset,
						 CPoint 			& ptCenter,
						 BYTE				  byRotAngle )
		:
			CDrawParms  ( tile, ptOffset ),
			m_ptCenter  ( ptCenter ),
			m_byRotAngle( byRotAngle )
	{}

	Fixed		GetSin() const;
	Fixed		GetCos() const;

	CPoint	SpriteViewToWindow( CPoint const &, CSpriteView const & ) const;
	CPoint	WindowToSpriteView( CPoint const &, CSpriteView const & ) const;

	CPoint	m_ptCenter;
	BYTE		m_byRotAngle;
};

//---------------------------- C B l o c k I n f o ---------------------------

struct CBlockInfo
{
	long	m_lOffset;
	long	m_lLength;

	#ifdef _DEBUG
	void CheckValid() const;
	#endif
};

//--------------------------- C L a y o u t I n f o -------------------------

struct CLayoutInfo
{
	CBlockInfo	m_blockinfoCompressed;
	int			m_aiDecompressedLength[ 4 ];	// 1-byte, 2-bytes, 3-bytes, 4-bytes

	#ifdef _DEBUG
	void	CheckValid() const;
	#endif
};

//------------------------ C D I B L a y o u t I n f o ----------------------

struct CDIBLayoutInfo
{
	CBlockInfo	m_blockinfoDecompressed[ 4 ];	// 1-byte, 2-bytes, 3-bytes, 4-bytes

	#ifdef _DEBUG
	void	CheckValid() const;
	#endif
};

//--------------------------- C S p r i t e D I B ----------------------------
//

struct CSpriteDIB
{
	int				Type() 		 			const	{ return m_iType; 				}
	int				BytesPerPixel() 		const	{ return m_iBytesPerPixel; 	}
	unsigned			Time() 		 			const	{ return m_uTime; 				}
	void				Time( unsigned uTime )		{ m_uTime = uTime; 				}

	CRect	const &	Rect  ( int iZoom ) const	{ return m_arect[ iZoom  ];  	   }
	int	  			X     ( int iZoom ) const	{ return Rect( iZoom ).left; 	   }
	int				Y	   ( int iZoom ) const	{ return Rect( iZoom ).top;  	   }
	int				Width ( int iZoom ) const	{ return Rect( iZoom ).Width();  }
	int				Height( int iZoom ) const	{ return Rect( iZoom ).Height();	}
	long				Length( int iZoom ) const  { return m_adiblayoutinfo[ iZoom ].m_blockinfoDecompressed[ m_iBytesPerPixel - 1 ].m_lLength; }

	CRect	const &	Rect() 		 		  const	{ return m_arect[ xiZoom ]; }
	int	  			X() 			 		  const	{ return Rect().left;		 }
	int				Y() 			 		  const	{ return Rect().top;			 }
	int				Width() 		 		  const	{ return Rect().Width();	 }
	int				Height() 	 		  const	{ return Rect().Height();	 }
	long				Length()				  const  { return Length( xiZoom );  }

	CPoint			SpriteViewToDIB( CPoint const & ) const;
	CPoint			DIBToSpriteView( CPoint const & ) const;

	static void		SetParms( CDrawParms & drawparms, CSpriteView const * pspriteview )
						{
							G_pdrawparms  = &drawparms;
						  	G_pspriteview =  pspriteview;
						}

	void				Init( CSpriteDIBParms const & );

	CRect	__stdcall Draw()							const;
	CRect				 CalcBoundingRect()			const;
	BOOL				 IsHit(	CPoint const & )	const;

	#ifdef _DEBUG
	void	CheckValid() const;
	#endif

	static CDrawParms 		 * GetParms()	{ return G_pdrawparms;  }
	static CSpriteView const * GetView()	{ return G_pspriteview;	}

protected:

	BYTE const	 * GetDIBPixels() const;

	// Functions specific to type STRUCTURE

	CRect	__stdcall StructureDraw()							const;
	CRect				 StructureCalcBoundingRect()			const;
	BOOL				 StructureIsHit(	CPoint const & )	const;
	CRect				 StructureDrawToDIB( CDIB *, CPoint const & ptDst, CRect const & rectClip ) const;
	CPoint			 StructureGetWindowPos() const;

	// Functions specific to type TERRAIN

	CRect	__stdcall TerrainDraw()								const;
	CRect				 TerrainCalcBoundingRect()				const;
	BOOL				 TerrainIsHit(	CPoint const & )		const;
	BYTE const	  * TerrainGetDIBPixels( int iShade ) 	const;

	void	__stdcall TerrainDrawQuad				( int [2], CPoint [4], BOOL, BOOL, int ) const;
	void	__stdcall TerrainDrawQuadVert	 		( int [2], CPoint [4], BOOL 				) const;
	void 	__stdcall TerrainDrawQuadVertHatched( int [2], CPoint	[4], BOOL, int 		) const;

	static int		 			  TerrainGetShadeIndex( CMapLoc3D [4],	BOOL bLeft );
	static CTerrainDrawParms *TerrainGetParms() { return ( CTerrainDrawParms * )GetParms(); }

	// Functions specific to type VEHICLE

	CRect	__stdcall VehicleDraw()							const;
	CRect				 VehicleCalcBoundingRect()			const;
	BOOL				 VehicleIsHit(	CPoint const & )	const;

	BOOL				 VehicleGetWindowVertices( CPoint       [4] ) const;
	CRect				 VehicleCalcBoundingRect ( CPoint const [4] ) const;
	CRect				 VehicleDraw             ( CPoint const [4] ) const;

	static CQuadDrawParms *VehicleGetParms()  { return ( CQuadDrawParms * )GetParms(); }

private:

	CSpriteDIB();	// Can't construct a CSpriteDIB, can only point to a block of memory

	// 8 bytes of reserved spave follows

	int				m_aiReserved[1];
	CSpriteView	 * m_pspriteview;

	// File image follows

	int				m_iType;
	int				m_iBytesPerPixel;

	unsigned			m_uTime;										// # game frames for this animation frame
	CRect				m_arect[ NUM_ZOOM_LEVELS ];			// Bounding rects in arbitrary coordinate system
	CDIBLayoutInfo	m_adiblayoutinfo[ NUM_ZOOM_LEVELS ];	// Decompressed pixel data Offsets/lengths for all color resolutions

	static CDrawParms 		 * G_pdrawparms;
	static CSpriteView const * G_pspriteview;
};

//------------------------- C H o t S p o t K e y ---------------------------

class CHotSpotKey
{

public:

	enum	HOTSPOT_TYPE
	{
		UNDEFINED_HOTSPOT   = -1,
		SMOKE_FLAME_HOTSPOT = 0,
		TURRET_HOTSPOT      = 1,
		MUZZLE_HOTSPOT      = 1,
		FLAG_HOTSPOT        = 2,
		NUM_HOTSPOT_TYPE
	};

	CHotSpotKey()
		:
			m_eType ( UNDEFINED_HOTSPOT ),
			m_iIndex( - 1 )
	{}

	CHotSpotKey( HOTSPOT_TYPE eType, int iIndex )
		:
			m_eType ( eType ),
			m_iIndex( iIndex )
	{}

	BOOL	operator == ( CHotSpotKey const & hotspotkey ) const
			{
				return m_iIndex == hotspotkey.m_iIndex &&
				       m_eType  == hotspotkey.m_eType;
			}

	#ifdef _DEBUG
	void CheckValid() const;
	#endif

	int				m_iIndex;	// 0, 1, 2 etc.
	HOTSPOT_TYPE	m_eType;
};

//-------------------------- C V i e w C o o r d ----------------------------

class CViewCoord
{

public:

	CViewCoord() { memset( m_apt, 0, sizeof( m_apt )); }
	CViewCoord( CPoint const & );

	CPoint	GetPoint( int iZoom = xiZoom ) const { return m_apt[ iZoom ]; }

	CPoint	m_apt[ NUM_ZOOM_LEVELS ];

	#ifdef _DEBUG
	void CheckValid() const;
	#endif
};

//---------------------------- C H o t S p o t ------------------------------

class CHotSpot : public CViewCoord
{

public:

	CHotSpot() {}
	CHotSpot( CHotSpotKey const & hotspotkey, CPoint const & ptOffset )
		:
			CViewCoord( ptOffset   ),
			m_key     ( hotspotkey )
	{}

	#ifdef _DEBUG
	void CheckValid() const;
	#endif

	CHotSpotKey	m_key;	
};

//----------------------------- C A n c h o r -------------------------------

typedef CViewCoord	CAnchor;

//------------------------- C S p r i t e V i e w ---------------------------

class CUnitTile;
class CSimpleTile;

class CSpriteView
{

public:

	enum	ANIM_TYPE {	ANIM_FRONT_1,	// This must match the order they're stored in the .SPR files
							ANIM_FRONT_2,
							ANIM_BACK_1,
							ANIM_BACK_2,
							ANIM_COUNT };

	void	CopyScale( int iSrc, int iDst );

	CRect			Rect()				const { return m_arect[ xiZoom ]; }
	CRect			Rect( int iZoom )	const { return m_arect[  iZoom ]; }
	int			Width() 				const	{ return m_arect[ xiZoom ].Width();  }
	int			Height() 			const	{ return m_arect[ xiZoom ].Height(); }
	short			BaseCount() 		const { return m_nBase; }
	short			OverlayCount() 	const { return m_nOverlay; }

	short			AnimCount( ANIM_TYPE eAnim ) const
					{
						ASSERT( 0 <= eAnim && eAnim < ANIM_COUNT );
						return m_anAnim[ eAnim ];
					}

	void			SetOverlayBase( CSpriteView const * pspriteviewNoDamage );
	void			SetAnimBase   ( CSpriteView const * pspriteviewNoDamage );
	void			CopyBase		  ( CSpriteView const * pspriteviewSrc );
	void			CopyAnims	  ( CSpriteView const * pspriteviewSrc, ANIM_TYPE );

	CRect			Draw 	  		  ( CDrawParms & );
	CRect			DrawClip		  ( CDrawParms &, CRect * );

	void			DrawSelected  ( CRect const & rect, int iColor );

	BOOL			IsHit	  		  ( CDrawParms &, CPoint const & ptCursor ) const;
	BOOL			IsHitClip	  ( CDrawParms &, CPoint const & ptCursor, CRect * ) const;

	CSpriteDIB const * GetBase	  ( int iIndex ) const;
	CSpriteDIB const * GetOverlay( int iIndex ) const;
	CSpriteDIB const * GetAnim	  ( ANIM_TYPE, int iIndex ) const;

	void Rect( int iZoom, CRect const &rect )	{ m_arect[ iZoom ] = rect; }

	// Hotspot support

	CPoint			  GetHotSpotPoint( CViewCoord const &, int iZoom = xiZoom )  const;
	CPoint			  GetAnchorPoint ( int iZoom = xiZoom )  const;

	CHotSpot	const * GetHotSpot( CHotSpotKey const & ) 	  const;

	CAnchor	const * GetAnchor()							  		  const
				 		  { return & m_anchor; }

	#ifdef _DEBUG
	void CheckValid() const;
	#endif

protected:

	friend class CSprite;
	friend CSpriteDIB;

	void	Init(	CSprite *, CSpriteDIBParms * );

	CRect	_Draw( CDrawParms & );

	CRect	DrawUnitAnimation  ( ANIM_TYPE );
	CRect	DrawSimpleAnimation();

	void	CalcRect();

	BYTE const	 * GetDIBPixels( CDIBLayoutInfo const & diblayoutinfo ) const;

private:

	CSpriteView();	// Can't construct, only cast pointer to memory block

	CSprite		 * m_psprite;
	CRect				m_arect[ NUM_ZOOM_LEVELS ];
	CHotSpot		 *	m_photspots;
	CSpriteDIB	 * m_pspritedibBase;
	CSpriteDIB 	 * m_pspritedibOverlay;
	CSpriteDIB   * m_apspritedibAnim[ ANIM_COUNT ];

	int				m_aiReserved[ 8 ];

	CAnchor	 		m_anchor;
	int				m_iSuperviewIndex;
	int				m_nHotSpots;
	int				m_nBase;
	int				m_nOverlay;
	int				m_anAnim[ ANIM_COUNT ];
};

//----------------------------------------------------------------------------
// CSpriteDIB::GetDIBPixels
//----------------------------------------------------------------------------
inline BYTE const	 * 
CSpriteDIB::GetDIBPixels() const	
{ 
	return m_pspriteview->GetDIBPixels( m_adiblayoutinfo[ xiZoom ] ); 
}

//----------------------------------------------------------------------------
// CSpriteDIB::TerrainGetDIBPixels
//----------------------------------------------------------------------------
inline BYTE const	 * 
CSpriteDIB::TerrainGetDIBPixels(
	int	iShade ) const	
{ 
	BYTE const * pby = GetDIBPixels();

	if (pby == NULL)
		return NULL;

	return pby + iShade * ( Length() >> 3 );
}

//--------------------------- C S p r i t e H d r -----------------------------

struct CSpriteHdr
{
	int			m_iCompressionType;
	int			m_nViews;
	int			m_nSuperViews;
	CBlockInfo	m_blockinfoZoom[ NUM_ZOOM_LEVELS ];

	#ifdef _DEBUG
	void CheckValid() const;
	#endif

	BYTE	m_abyTheRest[1];
};

//------------------------ C S p r i t e P a r m s --------------------------

class CSpriteCollection;

class CSpriteParms
{
	public:

	CSpriteParms( CMmio *, unsigned uTime, int iTypeOverride, int iBitsPerPixel );

	CSpriteCollection * m_pspritecollection;
	unsigned				  m_uTime;
	int					  m_iType;
	int					  m_iID;
	int					  m_iBitsPerPixel;
	Ptr< CSpriteHdr >	  m_ptrspritehdr;
	SimplePtr< BYTE >	  m_ptrbyCompressedSuperviews;
	BOOL					  m_bDummy;

	#ifdef _DEBUG
	void CheckValid() const;
	#endif
};

//----------------------- C S u p e r V i e w I n f o -----------------------

struct CSuperviewInfo
{
	CLayoutInfo	m_alayoutinfo[ NUM_ZOOM_LEVELS ];

	#ifdef _DEBUG
	void CheckValid() const;
	#endif
};

//----------------------------- C S p r i t e -------------------------------
//

class CSprite
{

public:

	enum SPRITE_TYPES
	{
		TERRAIN 		= 1,
		EFFECT 		= 2,
		STRUCTURE 	= 4,
		VEHICLE		= 5
	};

	enum LAYER_TYPE {	FOREGROUND_LAYER,
							BACKGROUND_LAYER };

	static Ptr< CSprite >	VirtualConstruct( CSpriteParms const & );

	CSprite(	CSpriteParms const & );

	void	PostConstruct();

	int	GetIndex() const			{ return m_iIndex;	}
	void	SetIndex( int iIndex )	{ m_iIndex = iIndex;	}

	void	SetTime( unsigned uTime );

	int	GetID() 			const	{ return m_iID; }
	void	SetID( int iID )		{ m_iID = iID; }

	int	GetNumViews() 		 const { return m_ptrspritehdr->m_nViews; 	  }
	int	GetNumSuperviews() const { return m_ptrspritehdr->m_nSuperViews; }

	// Hot spots

	int	GetNumHotSpots( CHotSpotKey::HOTSPOT_TYPE eType ) const
			{ return m_anHotSpots[ eType ]; }

	void	AddHotSpotKey ( CHotSpotKey const & hotspotkey );

	CSpriteView	* GetSpriteView( int iIndex, BOOL bCheckValid = TRUE ) const;

	#ifdef _DEBUG
	void CheckValid() const;
	#endif

protected:

	friend	CSpriteView;

	int  const	* GetViewIndices			  () const { return m_piViewIndices; }
	BYTE const	* GetDIBPixels	 			  ( int iSuperviewIndex, CDIBLayoutInfo const & );
	BYTE const	* GetDecompressedSuperview( int iSuperviewIndex, int iZoom )
	{
		ASSERT( 0 <= iSuperviewIndex && iSuperviewIndex < GetNumSuperviews() );
		ASSERT( 0 <= iZoom && iZoom < NUM_ZOOM_LEVELS );
		ASSERT( m_apbyDecompressedSuperviews[ iZoom ][ iSuperviewIndex ] );

		return m_apbyDecompressedSuperviews[ iZoom ][ iSuperviewIndex ];
	}

	void	ColorConvert( BYTE * pbyDst, BYTE const * pbySrc, long lLenDst, long	lLenSrc, int iZoom, int iSuperviewIndex );

	int			m_iID;
	int			m_iIndex;
	int			m_iType;
	int			m_iBytesPerPixel;
	int			m_iBitsPerPixel;

	int			m_anHotSpots[ CHotSpotKey::NUM_HOTSPOT_TYPE ];

	int 			* m_piViewOffsets;		// Pointer to sequence of view offsets
	int 			* m_piViewIndices;		// Pointer to indices of views

	CSpriteCollection  * m_pspritecollection;
	CSuperviewInfo		 * m_psuperviewinfo;
	Ptr< CSpriteHdr > 	m_ptrspritehdr;
	SimplePtr< BYTE >	 	m_ptrbyDecompressedSuperviews;
	BYTE 	 	          * m_apbyDecompressedSuperviews[ NUM_ZOOM_LEVELS ][ NUM_BLDG_DIRECTIONS ];	// Cached decompressed pixel chunks
};

//----------------------------------------------------------------------------
// CSpriteView::GetDIBPixels
//----------------------------------------------------------------------------
inline BYTE const	 * 
CSpriteView::GetDIBPixels( CDIBLayoutInfo const & diblayoutinfo ) const
{
	return m_psprite->GetDIBPixels( m_iSuperviewIndex, diblayoutinfo );
}

//----------------------- C E f f e c t S p r i t e ---------------------
//
// Sprite with no direction or damage

class CEffectSprite : public CSprite
{

public:

	CEffectSprite( CSpriteParms const & );

	CSpriteView * GetView( int iLayer = CSprite::FOREGROUND_LAYER ) const
	{
		return m_apspriteview[ iLayer ];
	}

	BOOL	IsTwoPiece() const { return NULL != m_apspriteview[ BACKGROUND_LAYER ]; }

	#ifdef _DEBUG
	void CheckValid() const;
	#endif

private:

	CSpriteView	* m_apspriteview[ NUM_BLDG_LAYERS ];
};

//---------------------- C S t r u c t u r e S p r i t e --------------------

class CStructureSprite : public CSprite
{

public:

	enum STAGE_TYPE { COMPLETED_STAGE,
							CONSTRUCTION_STAGE,
							SKELETON_STAGE,
							FOUNDATION_STAGE };

	CStructureSprite( CSpriteParms const & );
   CStructureSprite(	CStructureSprite const & structuresprite,
	                  int	 					    iID,
	                  int	 					    iIndex,
	                  int 						    iRotRight );

	CSpriteView * GetView( int iDir, int iLayer, int iStage, int iDamage );

	BOOL	IsTwoPiece( int iDir, int iStage, int iDamage ) const
			{
				return NULL != m_aaaapspriteview[ iDir ]
														  [ BACKGROUND_LAYER ]
														  [ iStage ]
														  [ iDamage ];
			}

	#ifdef _DEBUG
	void CheckValid() const;
	#endif

private:

	CSpriteView	* m_aaaapspriteview[ NUM_BLDG_DIRECTIONS ]
							 			    [ NUM_BLDG_LAYERS ]
										    [ NUM_BLDG_STAGES ]
										    [ NUM_BLDG_DAMAGE ];
};

//---------------------- C T e r r a i n S p r i t e ------------------------
//
class CTerrainSprite : public CSprite
{

public:

	CTerrainSprite( CSpriteParms const & );
	CTerrainSprite( CTerrainSprite const &, int iID, int iIndex, int iRotRight );

	CSpriteView * GetView( int iDir, int iDamage );

	#ifdef _DEBUG
	void CheckValid() const;
	#endif

private:

	CSpriteView	* m_aapspriteview[ NUM_TERRAIN_DIRECTIONS ]
							 			  [ NUM_TERRAIN_DAMAGE ];
};

//---------------------- C V e h i c l e S p r i t e ------------------------
//
class CVehicleSprite : public CSprite
{

public:

	enum TILT_TYPE { TILT_FLAT, TILT_FORWARD, TILT_BACK };

	CVehicleSprite( CSpriteParms const & );

	CSpriteView * GetView( int iDir, int iTilt, int iDamage );

	#ifdef _DEBUG
	void CheckValid() const;
	#endif

private:

	CSpriteView	* m_aaapspriteview[ NUM_VEHICLE_DIRECTIONS ]
							  				[ NUM_VEHICLE_TILTS   	 ]
							  			   [ NUM_VEHICLE_DAMAGE     ];
};

//----------------------- C S p r i t e I D I n f o ----------------------
//
// # of sprites with the same ID and index into the sprite collection of the first sprite of the group

class CSpriteIDInfo
{

public:

	CSpriteIDInfo()	{ SetInfo( 0, 0 ); }

	void	SetInfo( int iIndex, int nCount )
	{
		m_iIndex = iIndex;
		m_nCount = nCount;
	}

	int	GetIndex() const { return m_iIndex; }
	int	GetCount() const { return m_nCount; }

private:

	int	m_iIndex;
	int	m_nCount;
};

//--------------- C S p r i t e C o l l e c t i o n I n f o --------------
//
// Get info on a group of sprites with the same ID

class CSpriteCollectionInfo
#ifdef _DEBUG
: public CObject
#endif
{

public:

	CSpriteCollectionInfo( int iMaxID );
  ~CSpriteCollectionInfo();

	void	SetInfo( int iID, int iIndex, int nCount );

	int	GetIndex ( int iID, int iOffset, BOOL bStrict = FALSE ) const;
	int	GetCount ( int iID )												  const;

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

protected:

	int	MapOffset( int iID, int	iOffset, BOOL bStrict = FALSE ) const;
	int	MapID	   ( int iID, BOOL bStrict ) 							  const;

private:

	CSpriteIDInfo	 *m_pspriteidinfo;
	int				  m_iMaxID;
};

//----------------- C S p r i t e C o l l e c t i o n --------------------
//
// A collection of sprites

class CSpriteCollection
#ifdef _DEBUG
: public CObject
#endif
{

public:

	CSpriteCollection( const char * pszRif );

   virtual ~CSpriteCollection() { Close(); }

	void	Close();

	BOOL	Read			  ( CMmio &,
								 unsigned 	uTime,
								 int 	  		iPer,
								 int			iPerRange,
								 int	  		iBitsPerPixel,
								 int	  		iType = -1 );

	int	GetCount() 		  	  const	{ return m_nSprite; }
	int	GetCount( int iID ) const;

	CSprite       *	GetSpriteByIndex( int iIndex )
							{
								ASSERT_STRICT_VALID( this );
								ASSERT_STRICT( m_bOpen );

								ASSERT( 0 <= iIndex && iIndex < m_nSprite );

								#ifdef _DEBUG
								m_pptrsprite[ iIndex ]->CheckValid();
								#endif

								return m_pptrsprite[ iIndex ].Value();
							}
		
	CSprite const *	GetSpriteByIndex( int iIndex ) const
							{
								return (( CSpriteCollection * )this )->GetSpriteByIndex( iIndex );
							}

	CSprite       *	GetSprite( int iID, int iIndex = 0, BOOL bStrict = FALSE );
	CSprite const *	GetSprite( int iID, int iIndex = 0, BOOL bStrict = FALSE ) const;
	void					SetSprite( Ptr< CSprite > const &, int iID, int iIndex = 0 );
	char 	  const *	GetRifName() const { return m_strRif; }
	CFile 		  *	GetFile();

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

protected:

	class CThreadParms
	{
		public:

			CThreadParms( CMmio * pmmio, unsigned uTime, int iTypeOverride, int iBitsPerPixel, int nSprites )
				:
					m_pmmio			( pmmio ),
					m_uTime			( uTime ),
					m_iTypeOverride( iTypeOverride ),
					m_iBitsPerPixel( iBitsPerPixel ),
					m_nSprites		( nSprites )
				{
				}

			CMmio * m_pmmio;
			unsigned m_uTime;
			int		m_iTypeOverride;
			int		m_iBitsPerPixel;
			int		m_nSprites;
	};

	static UINT	ReadThreadFunc( void * pvParam );

	int				  m_bLoaded;		// 0x01 data loaded, 0x02 sprites loaded, 0x04 lang loaded
	int				  m_nSprite;
	BOOL				  m_bOpen;
	CString			  m_strRif;
	Ptr< CSprite > * m_pptrsprite;
	Ptr< CFile >	  m_ptrfile;
	Ptr< CSpriteCollectionInfo	> m_ptrspritecollectioninfo;

	static	CCriticalSection	g_cs;
	static	BOOL					g_bTerminateReadThread;
	static	BOOL					g_bReadThreadError;

	static 	CList< Ptr< CSpriteParms >, Ptr< CSpriteParms > & >	g_listptrspriteparms;
};

//-------------------- C S p r i t e R e a d R e q u e s t ------------------

class CSpriteView;
class CSprite;

//------------------------------ C A m b i e n t ---------------------------
// 

class CAmbient
#ifdef _DEBUG
: public CObject
#endif
{

public:

	CAmbient( CSpriteView::ANIM_TYPE eAnim = CSpriteView::ANIM_FRONT_1, BOOL bOneShot = FALSE );

	void	SetAnimType( CSpriteView::ANIM_TYPE );

	void	SetFrame   ( CSpriteView const *, int iAmbient );
	int	UpdateFrame( CSpriteView const * );

	void	Reset		 ();
	void	Enable	 ( BOOL bEnable  );
	void	Pause		 ( BOOL bPause   );
	void	SetOneShot( BOOL bOneShot );

	BOOL	IsOneShot() const	{ return m_bOneShot; }
	BOOL	IsEnabled() const	{ return m_bEnabled; }
	BOOL	IsPaused()  const	{ return m_bPaused;  }

	BOOL	IsOneShotFinished( CSpriteView const	* pspriteview ) const;

	void 	Serialize( CArchive & );

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

protected:

	friend class CSpriteView;

private:

	CSpriteView::ANIM_TYPE	m_eAnim;

	DWORD	m_dwLastTime;
	int	m_iFrame;
	BOOL	m_bEnabled;	
	BOOL	m_bOneShot;
	BOOL	m_bPaused;
};

//------------------------------- C T i l e -------------------------------
//
// Tile specific data the CSprite uses
//	anything on a tile (hex, bldg, vehicle) inherits from this.

#pragma pack( push, ctile, 1 )
#ifdef _DEBUG
class CTile : public CObject
#else
class CTile
#endif
{

public:

	enum TILE_TYPE { SIMPLE_TILE = 0, UNIT_TILE = 1 };

	void 			Serialize (CArchive & ar);
	TILE_TYPE	GetTileType() const { return ( TILE_TYPE )( m_byType & 0x01 ); }

	CSprite *	GetSprite() const { return m_psprite; }

	CSprite *	m_psprite;		// sprite for this tile (hex, bldg, car)
	BYTE	  		m_byType;		// bit 0 used for TILE_TYPE, subclasses can use other bits

protected:

	CTile( TILE_TYPE );

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};
#pragma pack( pop, ctile )

//----------------------------- C U n i t T i l e --------------------------
//
// Tile represented by sprite with up to 4 animation layers and 
// optional front/back piece (eg. structures, vehicles)

class CUnitTile : public CTile
{

public:

	CUnitTile();

	virtual CRect	Draw( CHexCoord const & ) { ASSERT( 0 ); return CRect( 0, 0, 0, 0 ); }

	CRect					 	DrawLayer( CHexCoord const				 	 &hexcoord,
											  CStructureSprite::LAYER_TYPE  eLayer )
								{
									ASSERT_VALID( this );

									m_eLayer = eLayer;

									return Draw( hexcoord );
								}

	void 					 	Serialize( CArchive & ar );
	CAmbient				 *	GetAmbient( CSpriteView::ANIM_TYPE eAnim )
								{
									ASSERT( 0 <= eAnim && eAnim < CSpriteView::ANIM_COUNT );
									ASSERT_VALID( m_aambient + eAnim );
									return m_aambient + eAnim;
								}

	int	GetFrame( CSpriteView const *, CSpriteView::ANIM_TYPE ) const; 
	void	SetFrame( CSpriteView const *, CSpriteView::ANIM_TYPE, int iFrame );

	CStructureSprite::LAYER_TYPE	GetLayer() const { return m_eLayer; }

	void	ResetAnimations();
	void	EnableAnimations( BOOL bEnable = TRUE );
	void	PauseAnimations( BOOL bPause = TRUE );
	void	SetOneShotAnimations( BOOL bOneShot = TRUE );

	CMapLoc const & GetMapLoc () const { return (m_maploc); }

	void			SetDamage (int iDamage)
									{ ASSERT ((0 <= iDamage) && (iDamage < NUM_DAMAGE_LEVELS));
									m_bDamage = (BYTE) iDamage; }
		int				GetDamage () const
									{ ASSERT_STRICT ((0 <= m_bDamage) && (m_bDamage < NUM_DAMAGE_LEVELS));
									return (((int) m_bDamage) & 0x0F); }

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

protected:

	CMapLoc	m_maploc;						// world pixel coord of middle of unit

	BYTE									m_bDamage;					// only use 0 - 4 of BYTE
	CStructureSprite::LAYER_TYPE	m_eLayer;
	CAmbient								m_aambient[ CSpriteView::ANIM_COUNT ];

	// for serializing

	short	m_iSpriteID;
	short	m_iSpriteIndex;
};

//------------------------- C E f f e c t T i l e --------------------------
//
// A unit tile with only one view

class CEffectTile : public CUnitTile
{

public:

	int	GetFrame  ( CSpriteView::ANIM_TYPE eLayer ) const 		  { return CUnitTile::GetFrame( GetView(), eLayer  ); }
	void	SetFrame  ( CSpriteView::ANIM_TYPE eLayer, int iFrame ) { CUnitTile::SetFrame( GetView(), eLayer, iFrame ); }
	int	AnimCount ( CSpriteView::ANIM_TYPE eLayer ) const 		  { ASSERT( GetView() ); 
																				 		 return GetView()->AnimCount( eLayer ); }

	CEffectSprite * GetSprite() const { return ( CEffectSprite * )m_psprite; }
	CSpriteView   * GetView()   const { return GetSprite()->GetView(); }

	void	Serialize( CArchive & ar );
};

//-------------------------- C S i m p l e T i l e -------------------------
//
// Tile represented by sprite with at most 1 animation layer

#pragma pack( push, csimpletile, 1 )
class CSimpleTile : public CTile
{

public:

	CSimpleTile();

	void	Serialize( CArchive & ar );

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif
};
#pragma pack( pop, csimpletile )

#endif
